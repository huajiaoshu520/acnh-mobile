// Private, two-device ACNH LAN room transport. Bonjour discovers control UDP
// endpoints. All game packets use unicast TCP/UDP via the BSD socket bridge.
// No wireless access-point emulation or Switch interoperability is implied.
#include "lan_room.h"
#include <dns_sd.h>
#include <CommonCrypto/CommonHMAC.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdatomic.h>
static _Atomic int room_local_mode;

#define ROOM_SERVICE "_acnhldn._udp"
#define ROOM_CACHE 16
#define ROOM_MAGIC 0x4c4e4341u
#define ROOM_LOG(...) do {fprintf(stderr,"[ACNH LAN] " __VA_ARGS__);fputc('\n',stderr);}while(0)
enum { R_QUERY=1,R_INFO,R_JOIN,R_UPDATE,R_LEAVE,R_DENY };
// Fixed wire size, versioned separately from the game; HMAC covers everything
// except the final tag. Zero all padding before sending. Limit datagrams to MTU.
typedef struct {uint32_t magic,version,type,reserved;uint64_t sequence;uint8_t session[16],nonce[16];lan_user user;int32_t game_version,reason;lan_info info;uint8_t tag[32];} room_packet;
_Static_assert(sizeof(room_packet)<1400,"control datagram must fit LAN MTU");
typedef struct {char name[256];uint32_t interface;DNSServiceRef resolve,address;struct sockaddr_in endpoint;lan_info info;uint64_t seen;int valid;} room_cache;
static struct {
    pthread_mutex_t lock;
    pthread_t thread;
    int running,fd,state,reason,join_result,policy;
    uint8_t filters[8][6];
    int nfilters;
    uint32_t ip,mask;
    struct sockaddr_in peer;
    int peer_active;
    uint64_t peer_seen,last_ping,sequence,peer_sequence;
    uint8_t nonce[16],peer_nonce[16],retired_nonce[16];
    int retired;
    lan_info info;
    lan_config config;
    lan_security security;
    lan_user user;
    DNSServiceRef browse,registration;
    room_cache cache[ROOM_CACHE];
    void (*changed)(void);
} room={.lock=PTHREAD_MUTEX_INITIALIZER,.fd=-1};
__attribute__((weak)) void front_lan_failed(int error,const char *stage){ROOM_LOG("%s failed=%d",stage?stage:"LAN",error);}
static uint64_t room_now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return(uint64_t)t.tv_sec*1000+t.tv_nsec/1000000;}
static int same_endpoint(const struct sockaddr_in *a,const struct sockaddr_in *b){return a->sin_addr.s_addr==b->sin_addr.s_addr&&a->sin_port==b->sin_port;}
static void changed(void){if(room.changed)room.changed();}
static void state(int s){if(room.state!=s){room.state=s;ROOM_LOG("state=%d",s);changed();}}
static int local_ip(uint32_t *ip,uint32_t *mask){
    struct ifaddrs *ifs=NULL;if(getifaddrs(&ifs))return -1;
    int found=0;
    for(struct ifaddrs *p=ifs;p;p=p->ifa_next){
        if(!p->ifa_addr||!p->ifa_netmask||p->ifa_addr->sa_family!=AF_INET||!(p->ifa_flags&IFF_UP)||(p->ifa_flags&IFF_LOOPBACK)||strncmp(p->ifa_name,"en",2))continue;
        *ip=ntohl(((struct sockaddr_in*)p->ifa_addr)->sin_addr.s_addr);*mask=ntohl(((struct sockaddr_in*)p->ifa_netmask)->sin_addr.s_addr);found=1;break;
    }
    freeifaddrs(ifs);return found?0:-1;
}
static void sign_packet(room_packet *p){CCHmac(kCCHmacAlgSHA256,room.security.key,room.security.length,p,offsetof(room_packet,tag),p->tag);}
static int authenticated(const room_packet *p){uint8_t tag[32];CCHmac(kCCHmacAlgSHA256,room.security.key,room.security.length,p,offsetof(room_packet,tag),tag);unsigned diff=0;for(int i=0;i<32;i++)diff|=tag[i]^p->tag[i];return diff==0;}
static room_packet packet(int type){room_packet p;memset(&p,0,sizeof p);p.magic=ROOM_MAGIC;p.version=1;p.type=type;p.sequence=++room.sequence;memcpy(p.session,room.info.id.session,16);memcpy(p.nonce,room.nonce,16);p.user=room.user;p.game_version=room.config.version;return p;}
static void send_packet(int type,const struct sockaddr_in *to,int reason){
    room_packet p=packet(type);p.reason=reason;
    if(type==R_INFO||type==R_UPDATE)p.info=room.info;
    if(type!=R_QUERY&&type!=R_INFO)sign_packet(&p);
    ssize_t n=sendto(room.fd,&p,sizeof p,0,(const void*)to,sizeof *to);
    if(n!=(ssize_t)sizeof p)ROOM_LOG("control send failed errno=%d",errno);
}
static lan_node node(const lan_user *u,uint32_t ip,int id,int version){lan_node n;memset(&n,0,sizeof n);n.ip=ip;n.id=id;n.connected=1;n.version=version;memcpy(n.name,u->name,32);n.name[32]=0;n.mac[0]=2;memcpy(n.mac+1,&ip,4);n.mac[5]=id;return n;}
static void remove_peer(int reason){
    if(room.peer_active){memcpy(room.retired_nonce,room.peer_nonce,16);room.retired=1;}
    room.peer_active=0;room.peer_seen=0;room.reason=reason;
    if(room.state==3){memset(&room.info.nodes[1],0,sizeof(lan_node));room.info.count=1;changed();}
    else if(room.state==5){memset(&room.info,0,sizeof room.info);state(4);}
}
static void received(const room_packet *p,const struct sockaddr_in *from){
    if(p->magic!=ROOM_MAGIC||p->version!=1)return;
    uint64_t now=room_now();
    if(p->type==R_INFO && (room.state==1||room.state==4)){
        if(p->info.count<1||p->info.count>2||p->info.max_nodes<1||p->info.max_nodes>2||p->info.advert_size>384||memcmp(p->session,p->info.id.session,16))return;
        for(int i=0;i<ROOM_CACHE;i++){room_cache *c=&room.cache[i];if(c->valid&&same_endpoint(&c->endpoint,from)){c->info=p->info;c->seen=now;break;}}
        return;
    }
    if(p->type==R_QUERY && room.state==3){send_packet(R_INFO,from,0);return;}
    if(memcmp(p->session,room.info.id.session,16))return;
    if(!authenticated(p))return;
    if(room.state==3 && p->type==R_JOIN){
        if(!room.peer_active&&room.retired&&!memcmp(room.retired_nonce,p->nonce,16)){send_packet(R_DENY,from,6);return;}
        lan_node incoming=node(&p->user,ntohl(from->sin_addr.s_addr),1,p->game_version);
        int listed=0;for(int i=0;i<room.nfilters;i++)if(!memcmp(incoming.mac,room.filters[i],6))listed=1;
        if(!room.peer_active && (room.policy==1 || (room.policy==2&&listed) || (room.policy==3&&!listed))){send_packet(R_DENY,from,5);return;}
        if(p->game_version!=room.config.version){send_packet(R_DENY,from,5);return;}
        if(room.peer_active && (!same_endpoint(&room.peer,from)||memcmp(p->nonce,room.peer_nonce,16))){send_packet(R_DENY,from,5);return;}
        if(!room.peer_active){
            if(same_endpoint(&room.peer,from)&&!memcmp(p->nonce,room.peer_nonce,16)&&p->sequence<=room.peer_sequence)return;
            if(room.info.max_nodes<2){send_packet(R_DENY,from,5);return;}
            room.peer=*from;room.peer_active=1;room.peer_sequence=0;memcpy(room.peer_nonce,p->nonce,16);
            room.info.nodes[1]=node(&p->user,ntohl(from->sin_addr.s_addr),1,p->game_version);room.info.count=2;changed();ROOM_LOG("visitor joined");
        }
        if(p->sequence<=room.peer_sequence)return;
        room.peer_sequence=p->sequence;room.peer_seen=now;send_packet(R_UPDATE,from,0);return;
    }
    if((room.state==4 || room.state==5) && same_endpoint(from,&room.peer)){
        if(p->sequence<=room.peer_sequence)return;
        if(p->type==R_DENY){room.peer_sequence=p->sequence;room.join_result=-1;room.reason=p->reason;return;}
        if(p->type==R_UPDATE){
            if(p->info.count!=2||p->info.max_nodes!=2||p->info.advert_size>384||memcmp(p->info.id.session,p->session,16)||p->info.nodes[0].ip!=ntohl(from->sin_addr.s_addr)||p->info.nodes[1].ip!=room.ip)return;
            int diff=memcmp(&room.info,&p->info,sizeof room.info);room.info=p->info;room.peer_sequence=p->sequence;room.peer_seen=now;room.peer_active=1;room.join_result=1;
            int was=room.state;state(5);if(diff&&was==5)changed();return;
        }
        if(p->type==R_LEAVE){room.peer_sequence=p->sequence;room.join_result=-1;remove_peer(4);return;}
    }
    if(room.state==3&&room.peer_active&&same_endpoint(from,&room.peer)&&!memcmp(p->nonce,room.peer_nonce,16)&&p->sequence>room.peer_sequence){
        room.peer_sequence=p->sequence;
        if(p->type==R_LEAVE)remove_peer(1);
    }
}
static void DNSSD_API address_cb(DNSServiceRef ref,DNSServiceFlags flags,uint32_t idx,DNSServiceErrorType err,const char *host,const struct sockaddr *addr,uint32_t ttl,void *ctx){
    (void)ref;(void)idx;(void)host;(void)ttl;room_cache *c=ctx;
    if(err||!addr||addr->sa_family!=AF_INET)return;
    c->endpoint.sin_addr=((const struct sockaddr_in*)addr)->sin_addr;
    c->valid=(flags&kDNSServiceFlagsAdd)!=0;
}
static void DNSSD_API resolve_cb(DNSServiceRef ref,DNSServiceFlags flags,uint32_t idx,DNSServiceErrorType err,const char *full,const char *host,uint16_t port,uint16_t txtlen,const unsigned char *txt,void *ctx){
    (void)ref;(void)flags;(void)full;(void)txtlen;(void)txt;room_cache *c=ctx;
    if(err)return;c->endpoint.sin_len=sizeof c->endpoint;c->endpoint.sin_family=AF_INET;c->endpoint.sin_port=port;
    if(c->address)DNSServiceRefDeallocate(c->address);c->address=NULL;
    DNSServiceErrorType e=DNSServiceGetAddrInfo(&c->address,0,idx,kDNSServiceProtocol_IPv4,host,address_cb,c);
    if(e){c->address=NULL;ROOM_LOG("Bonjour address error=%d",e);}
}
static void clear_cache(room_cache *c){if(c->resolve)DNSServiceRefDeallocate(c->resolve);if(c->address)DNSServiceRefDeallocate(c->address);memset(c,0,sizeof *c);}
static void DNSSD_API browse_cb(DNSServiceRef ref,DNSServiceFlags flags,uint32_t idx,DNSServiceErrorType err,const char *name,const char *type,const char *domain,void *ctx){
    (void)ref;(void)ctx;if(err){ROOM_LOG("Bonjour browse error=%d",err);front_lan_failed(err,"Bonjour browse");room.reason=4;state(6);return;}
    room_cache *slot=NULL;
    for(int i=0;i<ROOM_CACHE;i++){room_cache *c=&room.cache[i];if(c->name[0]&&!strcmp(c->name,name)&&c->interface==idx){if(!(flags&kDNSServiceFlagsAdd))clear_cache(c);return;}if(!c->name[0]&&!slot)slot=c;}
    if(!(flags&kDNSServiceFlagsAdd)||!slot)return;
    snprintf(slot->name,sizeof slot->name,"%s",name);slot->interface=idx;
    DNSServiceErrorType e=DNSServiceResolve(&slot->resolve,0,idx,name,type,domain,resolve_cb,slot);
    if(e)clear_cache(slot);
}
static void DNSSD_API register_cb(DNSServiceRef ref,DNSServiceFlags flags,DNSServiceErrorType err,const char *name,const char *type,const char *domain,void *ctx){
    (void)ref;(void)flags;(void)name;(void)type;(void)domain;(void)ctx;
    if(err){ROOM_LOG("Bonjour registration error=%d",err);front_lan_failed(err,"Bonjour register");room.reason=4;state(6);}
}
static void *worker(void *unused){
    (void)unused;
    for(;;){
        pthread_mutex_lock(&room.lock);if(!room.running){pthread_mutex_unlock(&room.lock);break;}
        // poll/process while holding the lock: callbacks mutate room/cache but
        // never acquire the lock or call guest code. Processing is nonblocking.
        DNSServiceRef refs[2+ROOM_CACHE*2];int nr=0;
        if(room.browse)refs[nr++]=room.browse;if(room.registration)refs[nr++]=room.registration;
        for(int i=0;i<ROOM_CACHE;i++){if(room.cache[i].resolve)refs[nr++]=room.cache[i].resolve;if(room.cache[i].address)refs[nr++]=room.cache[i].address;}
        // Process one at a time and rebuild the snapshot after callbacks: a
        // browse removal can destroy resolve/address refs in the same batch.
        for(int i=0;i<nr;i++){struct pollfd p={DNSServiceRefSockFD(refs[i]),POLLIN,0};if(p.fd>=0&&poll(&p,1,0)>0&&(p.revents&POLLIN)){DNSServiceErrorType e=DNSServiceProcessResult(refs[i]);if(e)ROOM_LOG("Bonjour process error=%d",e);break;}}
        for(int budget=0;budget<32;budget++){room_packet p;struct sockaddr_in from;socklen_t z=sizeof from;
            ssize_t n=recvfrom(room.fd,&p,sizeof p,0,(void*)&from,&z);if(n<0)break;if(n==(ssize_t)sizeof p)received(&p,&from);}
        uint64_t now=room_now();
        if(now-room.last_ping>=500){room.last_ping=now;
            uint32_t current,mask;
            if(room.state>=1&&room.state<=5 && (local_ip(&current,&mask)||current!=room.ip)) {
                if(room.peer_active)send_packet(R_LEAVE,&room.peer,4);
                remove_peer(4);state(6);
            }
            if(room.state==4 && room.join_result==0 && room.peer.sin_port)send_packet(R_JOIN,&room.peer,0);
            if(room.state==5)send_packet(R_JOIN,&room.peer,0);
            if(room.state==1||room.state==4)for(int i=0;i<ROOM_CACHE;i++)if(room.cache[i].valid){room_packet p=packet(R_QUERY);sendto(room.fd,&p,sizeof p,0,(void*)&room.cache[i].endpoint,sizeof(struct sockaddr_in));}
        }
        if(room.peer_active&&now-room.peer_seen>4000){ROOM_LOG("peer timed out");remove_peer(room.state==3?6:4);}
        pthread_mutex_unlock(&room.lock);
        struct timespec sleep={0,20000000};nanosleep(&sleep,NULL);
    }
    return NULL;
}
int dm_room_init(void (*notify)(void)){
    pthread_mutex_lock(&room.lock);if(room.running){pthread_mutex_unlock(&room.lock);return -1;}
    if(local_ip(&room.ip,&room.mask)){pthread_mutex_unlock(&room.lock);ROOM_LOG("no active LAN IPv4 interface");front_lan_failed(ENETDOWN,"Wi-Fi IPv4");return -1;}
    room.fd=socket(AF_INET,SOCK_DGRAM,0);if(room.fd<0){pthread_mutex_unlock(&room.lock);return -1;}
    struct sockaddr_in a={.sin_len=sizeof a,.sin_family=AF_INET,.sin_port=0,.sin_addr={htonl(room.ip)}};
    if(bind(room.fd,(void*)&a,sizeof a)||fcntl(room.fd,F_SETFL,O_NONBLOCK)<0){close(room.fd);room.fd=-1;pthread_mutex_unlock(&room.lock);return -1;}
    DNSServiceErrorType e=DNSServiceBrowse(&room.browse,0,0,ROOM_SERVICE,"local.",browse_cb,NULL);
    if(e){close(room.fd);room.fd=-1;room.browse=NULL;pthread_mutex_unlock(&room.lock);ROOM_LOG("Bonjour init failed=%d",e);front_lan_failed(e,"Bonjour browse");return -1;}
    room.changed=notify;room.running=1;room.state=1;room.reason=0;room.peer_active=0;room.peer_sequence=0;room.join_result=-1;room.last_ping=0;room.policy=0;room.nfilters=0;room.retired=0;
    arc4random_buf(room.nonce,16);arc4random_buf(&room.sequence,sizeof room.sequence);room.sequence>>=1;
    int error=pthread_create(&room.thread,NULL,worker,NULL);
    if(error){DNSServiceRefDeallocate(room.browse);room.browse=NULL;close(room.fd);room.fd=-1;room.running=0;room.state=0;pthread_mutex_unlock(&room.lock);return -1;}
    ROOM_LOG("initialized ip=%08x",room.ip);pthread_mutex_unlock(&room.lock);return 0;
}
void dm_room_fini(void){
    pthread_mutex_lock(&room.lock);if(!room.running){pthread_mutex_unlock(&room.lock);return;}
    if(room.peer_active)send_packet(R_LEAVE,&room.peer,0);room.running=0;pthread_mutex_unlock(&room.lock);pthread_join(room.thread,NULL);
    pthread_mutex_lock(&room.lock);if(room.registration)DNSServiceRefDeallocate(room.registration);if(room.browse)DNSServiceRefDeallocate(room.browse);room.registration=room.browse=NULL;
    for(int i=0;i<ROOM_CACHE;i++)clear_cache(&room.cache[i]);close(room.fd);room.fd=-1;room.state=0;room.peer_active=0;room.changed=NULL;
    memset(&room.security,0,sizeof room.security);memset(&room.info,0,sizeof room.info);atomic_store(&room_local_mode,0);pthread_mutex_unlock(&room.lock);
}
int dm_room_state(void){pthread_mutex_lock(&room.lock);int s=room.state;pthread_mutex_unlock(&room.lock);return s;}
int dm_room_open(int ap){pthread_mutex_lock(&room.lock);int ok=room.state==1;if(ok){memset(&room.info,0,sizeof room.info);state(ap?2:4);}pthread_mutex_unlock(&room.lock);return ok?0:-1;}
int dm_room_close(int ap){pthread_mutex_lock(&room.lock);int ok=room.state==(ap?2:4);if(ok){room.join_result=-1;memset(&room.peer,0,sizeof room.peer);state(1);}pthread_mutex_unlock(&room.lock);return ok?0:-1;}
int dm_room_create(const lan_config *c,const lan_security *s,const lan_user *u,const void *priv){
    if(!c||!s||!u||s->length>64||s->length<16||c->max_nodes<1||c->max_nodes>8||c->version<0||s->mode>3)return -1;
    pthread_mutex_lock(&room.lock);if(room.state!=2){pthread_mutex_unlock(&room.lock);return -1;}
    room.config=*c;room.config.max_nodes=c->max_nodes>2?2:c->max_nodes;room.security=*s;room.user=*u;room.user.name[32]=0;room.peer_active=0;room.peer_sequence=0;room.reason=0;
    // The game publishes its island data after OpenAccessPoint, before Create.
    // Preserve it across the transition into an actual advertised session.
    uint8_t pending_advert[384];uint16_t pending_size=room.info.advert_size;
    memcpy(pending_advert,room.info.advert,sizeof pending_advert);
    memset(&room.info,0,sizeof room.info);room.info.id.intent=c->intent;
    memcpy(room.info.advert,pending_advert,sizeof pending_advert);room.info.advert_size=pending_size;
    if(priv){memcpy(room.info.random,priv,16);memcpy(room.info.id.session,(const uint8_t*)priv+16,16);}else{arc4random_buf(room.info.random,16);arc4random_buf(room.info.id.session,16);}
    // NACP default (-1) is left for the HLE adapter to resolve against game ID.
    room.info.common.channel=c->channel?c->channel:1;room.info.common.link=3;room.info.common.type=2;room.info.common.mac[0]=2;memcpy(room.info.common.mac+1,&room.ip,4);
    room.info.common.ssid_length=32;for(int i=0;i<16;i++)snprintf(room.info.common.ssid+i*2,3,"%02x",room.info.id.session[i]);
    room.info.security=s->mode;room.info.policy=room.policy;room.info.version=1;room.info.max_nodes=room.config.max_nodes;room.info.count=1;room.info.nodes[0]=node(u,room.ip,0,c->version);room.retired=0;
    struct sockaddr_in addr;socklen_t n=sizeof addr;getsockname(room.fd,(void*)&addr,&n);
    char name[64];snprintf(name,sizeof name,"acnh-%08x-%04x",room.ip,ntohs(addr.sin_port));
    DNSServiceErrorType e=DNSServiceRegister(&room.registration,0,0,name,ROOM_SERVICE,"local.",NULL,addr.sin_port,0,NULL,register_cb,NULL);
    if(e){room.registration=NULL;memset(&room.info,0,sizeof room.info);pthread_mutex_unlock(&room.lock);ROOM_LOG("create registration failed=%d",e);front_lan_failed(e,"Bonjour register");return -1;}
    ROOM_LOG("room created version=%d capacity=%d advertise=%u policy=%d type=%d members=%u",room.config.version,room.info.max_nodes,room.info.advert_size,room.policy,room.info.common.type,room.info.count);
    state(3);pthread_mutex_unlock(&room.lock);return 0;
}
static int matches(const lan_info *i,const lan_filter *f,int priv){
    if(!f)return 1;uint32_t flags=f->flags;
    if((flags&1)&&i->id.intent.communication_id!=f->id.intent.communication_id)return 0;
    if((flags&2)&&memcmp(i->id.session,f->id.session,16))return 0;
    if((flags&4)&&i->common.type!=f->type)return 0;
    if((flags&8)&&(!priv||memcmp(i->common.mac,f->mac,6)))return 0;
    if((flags&16)&&(f->ssid_length>32||i->common.ssid_length!=f->ssid_length||memcmp(i->common.ssid,f->ssid,f->ssid_length)))return 0;
    if((flags&32)&&i->id.intent.scene!=f->id.intent.scene)return 0;
    return 1;
}
int dm_room_scan(lan_info *out,int *count,int cap,const lan_filter *f,int priv){
    if(!count||cap<0||cap>64||(!out&&cap))return -1;*count=0;
    pthread_mutex_lock(&room.lock);if(room.state!=1&&room.state!=4){pthread_mutex_unlock(&room.lock);return -1;}uint64_t now=room_now();
    for(int i=0;i<ROOM_CACHE&&*count<cap;i++){room_cache *c=&room.cache[i];if(c->valid&&c->seen&&now-c->seen<2000&&matches(&c->info,f,priv)){int duplicate=0;for(int j=0;j<*count;j++)if(!memcmp(out[j].id.session,c->info.id.session,16))duplicate=1;if(!duplicate)out[(*count)++]=c->info;}}
    pthread_mutex_unlock(&room.lock);return 0;
}
int dm_room_connect(const lan_info *info,const lan_security *sec,const lan_user *user,int version){
    if(!info||!sec||!user||sec->length<16||sec->length>64||version<0)return -1;
    pthread_mutex_lock(&room.lock);if(room.state!=4){pthread_mutex_unlock(&room.lock);return -1;}
    room_cache *match=NULL;for(int i=0;i<ROOM_CACHE;i++)if(room.cache[i].valid&&room.cache[i].seen&&room_now()-room.cache[i].seen<2000&&!memcmp(room.cache[i].info.id.session,info->id.session,16)){match=&room.cache[i];break;}
    if(!match){pthread_mutex_unlock(&room.lock);return -1;}
    room.info=*info;room.security=*sec;room.user=*user;room.config.intent=info->id.intent;room.config.channel=info->common.channel;room.config.version=version;room.config.max_nodes=info->max_nodes;
    room.peer=match->endpoint;room.reason=0;room.peer_sequence=0;room.join_result=0;room.peer_active=0;room.last_ping=0;arc4random_buf(room.nonce,16);send_packet(R_JOIN,&room.peer,0);
    pthread_mutex_unlock(&room.lock);
    uint64_t end=room_now()+3000;int result=0;
    while(room_now()<end){pthread_mutex_lock(&room.lock);result=room.join_result;pthread_mutex_unlock(&room.lock);if(result)break;struct timespec t={0,20000000};nanosleep(&t,NULL);}
    pthread_mutex_lock(&room.lock);int ok=result==1&&room.state==5;if(!ok){room.join_result=-1;memset(&room.peer,0,sizeof room.peer);memset(&room.info,0,sizeof room.info);room.reason=6;}
    pthread_mutex_unlock(&room.lock);return ok?0:-1;
}
int dm_room_disconnect(int destroy){
    pthread_mutex_lock(&room.lock);if(room.state!=(destroy?3:5)){pthread_mutex_unlock(&room.lock);return -1;}
    if(room.peer_active)send_packet(R_LEAVE,&room.peer,0);room.peer_active=0;room.join_result=-1;memset(&room.peer,0,sizeof room.peer);
    if(destroy&&room.registration){DNSServiceRefDeallocate(room.registration);room.registration=NULL;}
    memset(&room.info,0,sizeof room.info);room.reason=destroy?3:1;state(destroy?2:4);pthread_mutex_unlock(&room.lock);return 0;
}
int dm_room_info(lan_info *p){if(!p)return -1;pthread_mutex_lock(&room.lock);int ok=room.state==3||room.state==5;if(ok)*p=room.info;pthread_mutex_unlock(&room.lock);return ok?0:-1;}
int dm_room_config(lan_config *p){if(!p)return -1;pthread_mutex_lock(&room.lock);int ok=room.state==3||room.state==5;if(ok)*p=room.config;pthread_mutex_unlock(&room.lock);return ok?0:-1;}
int dm_room_security(void *p){if(!p)return -1;pthread_mutex_lock(&room.lock);int ok=room.state==3||room.state==5;if(ok){memcpy(p,room.info.random,16);memcpy((uint8_t*)p+16,room.info.id.session,16);}pthread_mutex_unlock(&room.lock);return ok?0:-1;}
int dm_room_advert(const void *p,size_t n){if(n>384||(!p&&n))return -1;pthread_mutex_lock(&room.lock);int ok=room.state==2||room.state==3;if(ok){memset(room.info.advert,0,384);if(n)memcpy(room.info.advert,p,n);room.info.advert_size=n;if(room.peer_active)send_packet(R_UPDATE,&room.peer,0);}pthread_mutex_unlock(&room.lock);return ok?0:-1;}
int dm_room_policy(int p){if(p<0||p>3)return -1;pthread_mutex_lock(&room.lock);int ok=room.state==2||room.state==3;if(ok){room.policy=p;room.info.policy=p;}pthread_mutex_unlock(&room.lock);return ok?0:-1;}
int dm_room_reject(uint32_t ip){pthread_mutex_lock(&room.lock);int ok=room.state==3&&room.peer_active&&room.info.nodes[1].ip==ip;if(ok){send_packet(R_LEAVE,&room.peer,5);remove_peer(5);}pthread_mutex_unlock(&room.lock);return ok?0:-1;}
int dm_room_reason(void){pthread_mutex_lock(&room.lock);int r=room.reason;pthread_mutex_unlock(&room.lock);return r;}
int dm_room_ip(uint32_t *ip,uint32_t *mask){if(!ip||!mask)return -1;pthread_mutex_lock(&room.lock);int ok=room.running;if(ok){*ip=room.ip;*mask=room.mask;}pthread_mutex_unlock(&room.lock);return ok?0:-1;}
uint32_t dm_room_peer_ip(void){pthread_mutex_lock(&room.lock);uint32_t ip=room.peer_active?ntohl(room.peer.sin_addr.s_addr):0;pthread_mutex_unlock(&room.lock);return ip;}

void dm_room_local_mode(int enabled){atomic_store(&room_local_mode,enabled!=0);}
int dm_room_network_ip(uint32_t *ip,uint32_t *mask){
    pthread_mutex_lock(&room.lock);int local=room.running;pthread_mutex_unlock(&room.lock);
    if(!local&&!atomic_load(&room_local_mode))return -1;
    return ip&&mask?local_ip(ip,mask):-1;
}

int dm_room_filter(uint64_t mac){
    uint8_t bytes[8];memcpy(bytes,&mac,8);pthread_mutex_lock(&room.lock);
    int ok=(room.state==2||room.state==3)&&room.nfilters<8;
    if(ok)memcpy(room.filters[room.nfilters++],bytes,6);
    pthread_mutex_unlock(&room.lock);return ok?0:-1;
}
int dm_room_filters(const void *entries,int n){
    if(n<0||n>8||(!entries&&n))return -1;pthread_mutex_lock(&room.lock);
    int ok=room.state==2;if(ok){room.nfilters=n;for(int i=0;i<n;i++)memcpy(room.filters[i],(const uint8_t*)entries+i*12+4,6);if(n)room.policy=3;}
    pthread_mutex_unlock(&room.lock);return ok?0:-1;
}
