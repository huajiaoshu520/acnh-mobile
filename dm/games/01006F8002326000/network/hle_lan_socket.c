// The socket bridge is kept separate from HLE names so the exact same code is
// exercised by local integration tests, without the game or its data.
#include "lan_socket.h"
#include "lan_room.h"
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

enum { LAN_FDS = 128 };
static pthread_mutex_t sklock = PTHREAD_MUTEX_INITIALIZER;
// 0 = unused; real descriptors stored plus one (fd 0 is a valid host descriptor).
static int skfd[LAN_FDS];
static _Thread_local int skerr;
int dm_lan_errno(void) { return skerr; }
// ACNH SDK 20.5.4 uses Linux/POSIX error numbers in both GetLastError and
// GetLastErrno. The game's receive table treats 11 (EAGAIN) as pending; Darwin
// 35 is fatal there. Never expose native errno to guest code or SO_ERROR.
static int sk_guest_error(int e) {
    if(e>=0 && e<=34 && e!=EDEADLK)return e;
    switch(e) {
    case EDEADLK:return 35; case EAGAIN:return 11;
    case EINPROGRESS:return 115; case EALREADY:return 114;
    case ENOTSOCK:return 88; case EDESTADDRREQ:return 89;
    case EMSGSIZE:return 90; case EPROTOTYPE:return 91;
    case ENOPROTOOPT:return 92; case EPROTONOSUPPORT:return 93;
    case ESOCKTNOSUPPORT:return 94; case ENOTSUP:return 95;
    case EOPNOTSUPP:return 95; case EPFNOSUPPORT:return 96;
    case EAFNOSUPPORT:return 97; case EADDRINUSE:return 98;
    case EADDRNOTAVAIL:return 99; case ENETDOWN:return 100;
    case ENETUNREACH:return 101; case ENETRESET:return 102;
    case ECONNABORTED:return 103; case ECONNRESET:return 104;
    case ENOBUFS:return 105; case EISCONN:return 106;
    case ENOTCONN:return 107; case ESHUTDOWN:return 108;
    case ETOOMANYREFS:return 109; case ETIMEDOUT:return 110;
    case ECONNREFUSED:return 111; case EHOSTDOWN:return 112;
    case EHOSTUNREACH:return 113; case ELOOP:return 40;
    case ENAMETOOLONG:return 36; case ENOTEMPTY:return 39;
    case EPROCLIM:return 11; case EUSERS:return 87;
    case EDQUOT:return 122; case ESTALE:return 116;
    case EREMOTE:return 66; case ENOSYS:return 38;
    case EOVERFLOW:return 75; case ECANCELED:return 125;
    case EILSEQ:return 84; case EPROTO:return 71;
    case ENODATA:return 61;
    default:return 22;
    }
}
static int sk_bad(int e) { skerr=sk_guest_error(e); return -1; }
// Setter arguments already come from the guest's errno namespace.
int dm_lan_error(int e) { skerr=e; return -1; }
static long sk_result(long r) {
    if(r<0){int native=errno;skerr=sk_guest_error(native);static _Thread_local uint64_t hits;uint64_t n=++hits;
        if(n<=8 || !(n&(n-1)))fprintf(stderr,"[ACNH LAN SOCKET] failure=%llu native_errno=%d guest_errno=%d\n",(unsigned long long)n,native,skerr);}
    return r;
}
static int sk_ref(int fd) {
    pthread_mutex_lock(&sklock);
    int r=(fd>=0 && fd<LAN_FDS && skfd[fd])?dup(skfd[fd]-1):-1;
    int e=(fd>=0 && fd<LAN_FDS && skfd[fd])?errno:EBADF;
    pthread_mutex_unlock(&sklock);
    if(r<0)skerr=sk_guest_error(e);
    return r;
}
static int sk_put(int real) {
    pthread_mutex_lock(&sklock);
    for(int i=0;i<LAN_FDS;i++)if(!skfd[i]){skfd[i]=real+1;pthread_mutex_unlock(&sklock);return i;}
    pthread_mutex_unlock(&sklock);close(real);return sk_bad(EMFILE);
}
static long sk_done(int fd,long r) { int e=errno;close(fd);errno=e;return sk_result(r); }
// Horizon and Darwin use BSD sockaddr: u8 length, u8 family. Only IPv4 is
// enabled for the initial ACNH backend. Do not reinterpret IPv6 family numbers.
static int sk_addr(struct sockaddr_in *out,const void *p,unsigned n) {
    if(!p || n<16)return sk_bad(EINVAL);
    const uint8_t *a=p;
    if(a[1]!=2 || (a[0] && a[0]!=16))return sk_bad(EAFNOSUPPORT);
    memset(out,0,sizeof *out);out->sin_len=sizeof *out;out->sin_family=AF_INET;
    memcpy(&out->sin_port,a+2,2);memcpy(&out->sin_addr,a+4,4);return 0;
}
static void sk_out(void *p,unsigned *n,const struct sockaddr_in *a) {
    uint8_t b[16]={16,2};memcpy(b+2,&a->sin_port,2);memcpy(b+4,&a->sin_addr,4);
    if(p && n){memcpy(p,b,*n<16?*n:16);*n=16;}
}
static int sk_flags(int f) {
    // FreeBSD MSG_NOSIGNAL is 0x20000; SO_NOSIGPIPE provides this on Darwin.
    int allowed=0x1|0x2|0x4|0x40|0x80|0x20000;
    if(f&~allowed)return sk_bad(EOPNOTSUPP);
    return f&~0x20000;
}
int dm_lan_socket(int family,int type,int protocol) {
    if(family!=2)return sk_bad(EAFNOSUPPORT);
    int kind=type&~0x30000000;
    if(kind!=SOCK_STREAM && kind!=SOCK_DGRAM)return sk_bad(EPROTONOSUPPORT);
    if(protocol && protocol!=IPPROTO_TCP && protocol!=IPPROTO_UDP)return sk_bad(EPROTONOSUPPORT);
    int fd=socket(AF_INET,kind,protocol);if(fd<0)return sk_result(fd);
    int one=1;setsockopt(fd,SOL_SOCKET,SO_NOSIGPIPE,&one,sizeof one);
    if(type&0x20000000)if(fcntl(fd,F_SETFL,O_NONBLOCK)<0){int e=errno;close(fd);return sk_bad(e);}
    return sk_put(fd);
}
int dm_lan_close(int fd) {
    pthread_mutex_lock(&sklock);
    if(fd<0 || fd>=LAN_FDS || !skfd[fd]){pthread_mutex_unlock(&sklock);return sk_bad(EBADF);}
    int real=skfd[fd]-1;skfd[fd]=0;
    // In-flight calls hold dup references; wake them before releasing ownership.
    shutdown(real,SHUT_RDWR);int r=close(real),e=errno;
    pthread_mutex_unlock(&sklock);errno=e;return sk_result(r);
}
void dm_lan_socket_fini(void) { for(int i=0;i<LAN_FDS;i++){pthread_mutex_lock(&sklock);int live=skfd[i]!=0;pthread_mutex_unlock(&sklock);if(live)dm_lan_close(i);} }
int dm_lan_bind(int fd,const void *a,unsigned n) {struct sockaddr_in sa;if(sk_addr(&sa,a,n)<0)return -1;int h=sk_ref(fd);return h<0?-1:sk_done(h,bind(h,(void*)&sa,sizeof sa));}
int dm_lan_connect(int fd,const void *a,unsigned n) {struct sockaddr_in sa;if(sk_addr(&sa,a,n)<0)return -1;int h=sk_ref(fd);return h<0?-1:sk_done(h,connect(h,(void*)&sa,sizeof sa));}
int dm_lan_listen(int fd,int n) {int h=sk_ref(fd);return h<0?-1:sk_done(h,listen(h,n));}
int dm_lan_shutdown(int fd,int how) {int h=sk_ref(fd);return h<0?-1:sk_done(h,shutdown(h,how));}
int dm_lan_accept(int fd,void *a,unsigned *n) {
    if((a && !n)||(!a && n))return sk_bad(EINVAL);
    int h=sk_ref(fd);if(h<0)return -1;struct sockaddr_in sa;socklen_t z=sizeof sa;
    int c=accept(h,(void*)&sa,&z);int e=errno;close(h);errno=e;
    if(c<0)return sk_result(c);
    int one=1;setsockopt(c,SOL_SOCKET,SO_NOSIGPIPE,&one,sizeof one);
    int g=sk_put(c);if(g>=0)sk_out(a,n,&sa);return g;
}
int dm_lan_name(int fd,void *a,unsigned *n,int peer) {
    if(!a || !n)return sk_bad(EINVAL);
    int h=sk_ref(fd);if(h<0)return -1;struct sockaddr_in sa;socklen_t z=sizeof sa;
    int r=peer?getpeername(h,(void*)&sa,&z):getsockname(h,(void*)&sa,&z);
    if(r==0)sk_out(a,n,&sa);return sk_done(h,r);
}
int dm_lan_fcntl(int fd,int c,int arg) {
    int h=sk_ref(fd);if(h<0)return -1;int r;
    // Guest F_GETFL/F_SETFL values match BSD. Deliberately expose only flags
    // supported by networking, never host descriptor duplication.
    if(c==3) {r=fcntl(h,F_GETFL);if(r>=0)r&=O_NONBLOCK;}
    else if(c==4) {int f=fcntl(h,F_GETFL);r=f<0?-1:fcntl(h,F_SETFL,(f&~O_NONBLOCK)|(arg&4?O_NONBLOCK:0));}
    else if(c==1) r=0;
    else if(c==2) r=0;
    else {errno=EINVAL;r=-1;}
    return sk_done(h,r);
}
int dm_lan_opt(int fd,int l,int o,void *v,unsigned *n,int set) {
    if(!n || (!v && *n))return sk_bad(EINVAL);
    // IPv4 and TCP have matching protocol-level values. SOL_SOCKET has a
    // different numeric value on some hosts; map it explicitly.
    int level=l==0xffff?SOL_SOCKET:l;
    if(l!=0xffff && l!=0 && l!=6)return sk_bad(ENOPROTOOPT);
    if(l==0xffff && o==0x800)o=SO_NOSIGPIPE;
    int h=sk_ref(fd);if(h<0)return -1;int r;
    if(l==0xffff && (o==SO_RCVTIMEO || o==SO_SNDTIMEO)) {
        if(set){if(*n!=16){close(h);return sk_bad(EINVAL);}int64_t t[2];memcpy(t,v,16);
            if(t[0]<0||t[1]<0||t[1]>=1000000){close(h);return sk_bad(EINVAL);}
            struct timeval tv={.tv_sec=t[0],.tv_usec=(int)t[1]};r=setsockopt(h,level,o,&tv,sizeof tv);
        }else{struct timeval tv;socklen_t z=sizeof tv;r=getsockopt(h,level,o,&tv,&z);
            if(r==0){int64_t t[2]={tv.tv_sec,tv.tv_usec};memcpy(v,t,*n<16?*n:16);*n=16;}}
    }else{socklen_t z=*n;r=set?setsockopt(h,level,o,v,z):getsockopt(h,level,o,v,&z);if(!set && r==0){*n=z;if(l==0xffff&&o==SO_ERROR&&z==sizeof(int)){int native;memcpy(&native,v,sizeof native);int guest=sk_guest_error(native);memcpy(v,&guest,sizeof guest);}}}
    return sk_done(h,r);
}
long dm_lan_send(int fd,const void *b,size_t n,int f,const void *a,unsigned al) {
    if(!b && n)return sk_bad(EINVAL);int flags=sk_flags(f);if(flags<0)return -1;
    struct sockaddr_in sa;if(a && sk_addr(&sa,a,al)<0)return -1;
    int h=sk_ref(fd);if(h<0)return -1;
    if(a) {
        uint32_t ip,mask,target=ntohl(sa.sin_addr.s_addr);
        if(!dm_room_ip(&ip,&mask) && (target==0xffffffffu || target==(ip|~mask))) {
            uint32_t peer=dm_room_peer_ip();
            if(!peer){close(h);return sk_bad(ENETUNREACH);}
            // LDN's logical broadcast reaches room members only. Unicast to
            // the joined device avoids iOS multicast/broadcast entitlements.
            sa.sin_addr.s_addr=htonl(peer);
        }
    }
    return sk_done(h,a?sendto(h,b,n,flags,(void*)&sa,sizeof sa):send(h,b,n,flags));
}
long dm_lan_recv(int fd,void *b,size_t n,int f,void *a,unsigned *al) {
    if((!b && n)||(a && !al)||(!a && al))return sk_bad(EINVAL);int flags=sk_flags(f);if(flags<0)return -1;
    int h=sk_ref(fd);if(h<0)return -1;struct sockaddr_in sa;socklen_t z=sizeof sa;
    long r=recvfrom(h,b,n,flags,a?(void*)&sa:NULL,a?&z:NULL);
    if(r>=0 && a)sk_out(a,al,&sa);return sk_done(h,r);
}
typedef struct {int32_t fd;int16_t events,revents;} lan_pollfd;
int dm_lan_poll(void *ptr,uint64_t n,int ms) {
    if(n>LAN_FDS || (!ptr && n))return sk_bad(EINVAL);
    lan_pollfd *g=ptr;struct pollfd p[LAN_FDS];int bad=0;
    for(unsigned i=0;i<n;i++){p[i]=(struct pollfd){.fd=-1,.events=g[i].events};g[i].revents=0;
        if(g[i].fd>=0){p[i].fd=sk_ref(g[i].fd);if(p[i].fd<0){g[i].revents=POLLNVAL;bad++;}}}
    // Bound unending waits so a guest can observe game shutdown/GC requests.
    int r=poll(p,(nfds_t)n,bad?0:(ms<0?100:ms)),e=errno;
    for(unsigned i=0;i<n;i++){g[i].revents|=p[i].revents;if(p[i].fd>=0)close(p[i].fd);}
    errno=e;return r<0?sk_result(r):r+bad;
}
int dm_lan_select(int n,uint64_t *r,uint64_t *w,uint64_t *e,const int64_t *tv) {
    if(n<0 || n>1024 || (tv && (tv[0]<0 || tv[1]<0 || tv[1]>=1000000)))return sk_bad(EINVAL);
    lan_pollfd p[LAN_FDS];int count=0;
    for(int fd=0;fd<n;fd++){int ev=0;if(r && ((r[fd/64]>>(fd%64))&1))ev|=POLLIN;
        if(w && ((w[fd/64]>>(fd%64))&1))ev|=POLLOUT;if(e && ((e[fd/64]>>(fd%64))&1))ev|=POLLPRI;
        if(ev){if(count==LAN_FDS)return sk_bad(EBADF);p[count++]=(lan_pollfd){fd,ev,0};}}
    int64_t ms=tv?(tv[0]>INT_MAX/1000?INT_MAX:tv[0]*1000+(tv[1]+999)/1000):-1;
    int z=dm_lan_poll(p,count,(int)ms);if(z<0)return -1;
    for(int i=0;i<count;i++)if(p[i].revents&POLLNVAL)return sk_bad(EBADF);
    unsigned bytes=((unsigned)n+63)/64*8;if(r)memset(r,0,bytes);if(w)memset(w,0,bytes);if(e)memset(e,0,bytes);
    int ready=0;
    for(int i=0;i<count;i++){int fd=p[i].fd;uint64_t bit=1ull<<(fd%64);int rev=p[i].revents;
        if(r && (p[i].events&POLLIN) && (rev&(POLLIN|POLLHUP|POLLERR))){r[fd/64]|=bit;ready++;}
        if(w && (p[i].events&POLLOUT) && (rev&(POLLOUT|POLLERR))){w[fd/64]|=bit;ready++;}
        if(e && (p[i].events&POLLPRI) && (rev&POLLPRI)){e[fd/64]|=bit;ready++;}}
    return ready;
}
