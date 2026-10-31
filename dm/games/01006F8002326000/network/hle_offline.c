// Networking and peripheral system services (out of scope: online play, local wireless, friends, telemetry, SSL) as seen through newer SDKs: all made "unavailable", the game takes its offline branch.
// Only entry points that go to a service are hooked; library functions that only read and write the game's own buffers (prepo Add / Array / Struct, err ErrorResultVariant, friends getters, socket byte-order conversion) keep running in the SDK.
// Return convention as in hle_friends.c: Result returns RES_FAIL ("saved / submitted" kinds return RES_OK so the game does not retry); bool returns false; events get a bound SystemEvent that never fires.
#include "hle.h"
#include "comm_test_trace.h"
#include "lan_socket.h"
#include "lan_room.h"
#include "lan_history.h"
#include "album_save.h"

#include <stdatomic.h>
#include <arpa/inet.h>
void dm_guest_sleep_ns(int64_t ns);
static _Atomic(void *) g_ldn_event;
static void ldn_changed(void) {
    void *ev=atomic_load(&g_ldn_event);
    if(ev) hle_signal_bound_system_event(ev);
}
static Result ldn_result(int r) { return r==0?RES_OK:RES_FAIL; }
static lan_config ldn_config(const void *p) {
    lan_config c=*(const lan_config*)p;
    // ACNH NACP LocalCommunicationId (verified in the user's 3.0.3 control).
    if(c.intent.communication_id==UINT64_MAX)c.intent.communication_id=0x01006f8002326000ull;
    return c;
}
static Result ldn_create(const void *a,const void *b,const void *c,const void *priv) {
    if(!a||!b||!c)return RES_FAIL;
    lan_config cfg=ldn_config(a);
    return ldn_result(dm_room_create(&cfg,b,c,priv));
}
static Result ldn_scan(void *out,int *n,int max,const void *filter,int priv) {
    if(!n||max<0||max>64||(!out&&max))return RES_FAIL;
    lan_filter cleaned;
    if(filter){cleaned=*(const lan_filter*)filter;cleaned.flags &= priv?0x3f:0x37;filter=&cleaned;}
    // Actual scanning has a bounded collection interval; zero results is valid.
    for(int i=0;i<50;i++) {
        int r=dm_room_scan(out,n,max,filter,priv);
        if(r||*n||!max)return ldn_result(r);
        dm_guest_sleep_ns(20000000);
    }
    return RES_OK;
}
static Result ldn_connect_private(const void *a,const void *b,const void *c,const void *d,int v) {
    if(!a||!b||!c||!d)return RES_FAIL;
    lan_config cfg=ldn_config(a);lan_filter f={0};f.flags=0x23;
    f.id.intent=cfg.intent;memcpy(f.id.session,(const uint8_t*)c+16,16);
    lan_info infos[16];int n=0;
    if(dm_room_scan(infos,&n,16,&f,1))return RES_FAIL;
    for(int i=0;i<n;i++)if(!memcmp(infos[i].random,c,16))return ldn_result(dm_room_connect(&infos[i],b,d,v));
    return RES_FAIL;
}
void dm_guest_sleep_ns(int64_t ns);


// ---------- prepo: telemetry reports. Saving / submitting all report success; nothing is sent ----------
HLE_FORCE("_ZN2nn5prepo10PlayReport4SaveERKNS_7account3UidE", "telemetry report save (user); nothing is sent, reports success", Result, h_prepo_SaveU, void *self, const void *uid) { (void)self; (void)uid; return RES_OK; }
HLE_FORCE("_ZN2nn5prepo10PlayReport4SaveEv", "telemetry report save; nothing is sent, reports success", Result, h_prepo_Save, void *self) { (void)self; return RES_OK; }
HLE_FORCE("_ZN2nn5prepo10InitializeEv", "telemetry init: the original connects to the prepo service (through sm), the host has no service process; no-op (Engage calls it explicitly)", Result, h_prepo_Init, void) { return RES_OK; }
// Add the session ID / connection status to a report: on hardware these first connect to the prepo service for the session (Super Mario 3D World + Bowser's Fury calls them entering the save select); the host adds nothing and reports success
HLE_FORCE("_ZN2nn5prepo12AddSessionIdEPNS0_10PlayReportEPKc", "telemetry session ID: no service connection, nothing added, reports success", Result, h_prepo_AddSess, void *rep, const char *key) { (void)rep; (void)key; return RES_OK; }
HLE_FORCE("_ZN2nn5prepo27AddInternetConnectionStatusEPNS0_10PlayReportEPKcS4_S4_", "telemetry internet connection status: no service connection, nothing added, reports success", Result, h_prepo_AddNet, void *rep, const char *a, const char *b, const char *c) { (void)rep; (void)a; (void)b; (void)c; return RES_OK; }
HLE_FORCE("_ZN2nn5prepo28RequestImmediateTransmissionEv", "telemetry immediate upload; nothing is sent, reports success", Result, h_prepo_Tx, void) { return RES_OK; }

// ---------- ACNH LDN over LAN: real two-device sessions ----------
HLE_FORCE("_ZN2nn3ldn10InitializeEv", "private ACNH LAN: real room lifecycle and transport", Result, h_ldn_Init, void) { ACNH_NET_TRACE(); return ldn_result(dm_room_init(ldn_changed)); }
HLE_FORCE("_ZN2nn3ldn8FinalizeEv", "private ACNH LAN: real room lifecycle and transport", void, h_ldn_Fini, void) { ACNH_NET_TRACE(); dm_room_fini(); atomic_store(&g_ldn_event,NULL); }
HLE_FORCE("_ZN2nn3ldn8GetStateEv", "private ACNH LAN: real room lifecycle and transport", int, h_ldn_State, void) { ACNH_NET_TRACE(); return dm_room_state(); }
HLE_FORCE("_ZN2nn3ldn22AttachStateChangeEventEPNS_2os15SystemEventTypeE", "private ACNH LAN: real room lifecycle and transport", void, h_ldn_Attach, void *ev) { ACNH_NET_TRACE(); if(!ev)return; hle_bind_system_event(ev,1); atomic_store(&g_ldn_event,ev); }
HLE_FORCE("_ZN2nn3ldn11OpenStationEv", "private ACNH LAN: real room lifecycle and transport", Result, h_ldn_OpenSta, void) { ACNH_NET_TRACE(); return ldn_result(dm_room_open(0)); }
HLE_FORCE("_ZN2nn3ldn12CloseStationEv", "private ACNH LAN: real room lifecycle and transport", Result, h_ldn_CloseSta, void) { ACNH_NET_TRACE(); return ldn_result(dm_room_close(0)); }
HLE_FORCE("_ZN2nn3ldn15OpenAccessPointEv", "private ACNH LAN: real room lifecycle and transport", Result, h_ldn_OpenAp, void) { ACNH_NET_TRACE(); return ldn_result(dm_room_open(1)); }
HLE_FORCE("_ZN2nn3ldn16CloseAccessPointEv", "private ACNH LAN: real room lifecycle and transport", Result, h_ldn_CloseAp, void) { ACNH_NET_TRACE(); return ldn_result(dm_room_close(1)); }
HLE_FORCE("_ZN2nn3ldn10DisconnectEv", "private ACNH LAN: real room lifecycle and transport", Result, h_ldn_Disc, void) { ACNH_NET_TRACE(); return ldn_result(dm_room_disconnect(0)); }
HLE_FORCE("_ZN2nn3ldn14DestroyNetworkEv", "private ACNH LAN: real room lifecycle and transport", Result, h_ldn_Destroy, void) { ACNH_NET_TRACE(); return ldn_result(dm_room_disconnect(1)); }
HLE_FORCE("_ZN2nn3ldn4ScanEPNS0_11NetworkInfoEPiiRKNS0_10ScanFilterEi", "private ACNH LAN: real room lifecycle and transport", Result, h_ldn_Scan, void *out, int *n, int max, const void *f, int ch) { ACNH_NET_TRACE(); (void)ch; return ldn_scan(out,n,max,f,0); }
HLE_FORCE("_ZN2nn3ldn11ScanPrivateEPNS0_11NetworkInfoEPiiRKNS0_10ScanFilterEi", "private ACNH LAN: real room lifecycle and transport", Result, h_ldn_ScanP, void *out, int *n, int max, const void *f, int ch) { ACNH_NET_TRACE(); (void)ch; return ldn_scan(out,n,max,f,1); }
HLE_FORCE("_ZN2nn3ldn13CreateNetworkERKNS0_13NetworkConfigERKNS0_14SecurityConfigERKNS0_10UserConfigE", "private ACNH LAN: real room lifecycle and transport", Result, h_ldn_Create,
          const void *a, const void *b, const void *c) { ACNH_NET_TRACE(); return ldn_create(a,b,c,NULL); }
HLE_FORCE("_ZN2nn3ldn20CreateNetworkPrivateERKNS0_13NetworkConfigERKNS0_14SecurityConfigERKNS0_17SecurityParameterERKNS0_10UserConfigEiPKNS0_12AddressEntryE", "private ACNH LAN: real room lifecycle and transport",
          Result, h_ldn_CreateP, const void *a, const void *b, const void *c, const void *d, int n, const void *e) { ACNH_NET_TRACE(); if(dm_room_filters(e,n))return RES_FAIL; return ldn_create(a,b,d,c); }
HLE_FORCE("_ZN2nn3ldn7ConnectERKNS0_11NetworkInfoERKNS0_14SecurityConfigERKNS0_10UserConfigEiNS0_13ConnectOptionE", "private ACNH LAN: real room lifecycle and transport", Result, h_ldn_Connect,
          const void *a, const void *b, const void *c, int v, int o) { ACNH_NET_TRACE(); (void)o; return ldn_result(dm_room_connect(a,b,c,v)); }
HLE_FORCE("_ZN2nn3ldn14ConnectPrivateERKNS0_13NetworkConfigERKNS0_14SecurityConfigERKNS0_17SecurityParameterERKNS0_10UserConfigEiNS0_13ConnectOptionE", "private ACNH LAN: real room lifecycle and transport",
          Result, h_ldn_ConnectP, const void *a, const void *b, const void *c, const void *d, int v, int o) { ACNH_NET_TRACE(); (void)o; return ldn_connect_private(a,b,c,d,v); }
HLE_FORCE("_ZN2nn3ldn14GetNetworkInfoEPNS0_11NetworkInfoE", "private ACNH LAN: real room lifecycle and transport", Result, h_ldn_NetInfo, void *o) { ACNH_NET_TRACE(); if(!o)return 0xc0cbu; return dm_room_info(o)==0?RES_OK:0x40cbu; /* ldn module 203: InvalidArgument / InvalidState */ }
HLE_FORCE("_ZN2nn3ldn14GetIpv4AddressEPNS0_11Ipv4AddressEPNS0_10SubnetMaskE", "private ACNH LAN: real room lifecycle and transport", Result, h_ldn_Ip, uint32_t *a, uint32_t *m) { ACNH_NET_TRACE(); return ldn_result(dm_room_ip(a,m)); }
HLE_FORCE("_ZN2nn3ldn20GetSecurityParameterEPNS0_17SecurityParameterE", "private ACNH LAN: real room lifecycle and transport", Result, h_ldn_SecPrm, void *o) { ACNH_NET_TRACE(); return ldn_result(dm_room_security(o)); }
HLE_FORCE("_ZN2nn3ldn16SetAdvertiseDataEPKvm", "private ACNH LAN: real room lifecycle and transport", Result, h_ldn_Adv, const void *d, uint64_t n) { ACNH_NET_TRACE(); return ldn_result(dm_room_advert(d,n)); }
HLE_FORCE("_ZN2nn3ldn19GetDisconnectReasonEv", "private ACNH LAN: real room lifecycle and transport", int, h_ldn_DiscReason, void) { ACNH_NET_TRACE(); return dm_room_reason(); }
HLE_FORCE("_ZN2nn3ldn20AddAcceptFilterEntryENS0_10MacAddressE", "private ACNH LAN: real room lifecycle and transport", Result, h_ldn_Filter, uint64_t mac) { ACNH_NET_TRACE(); return ldn_result(dm_room_filter(mac)); }
HLE_FORCE("_ZN2nn3ldn22SetStationAcceptPolicyENS0_12AcceptPolicyE", "private ACNH LAN: real room lifecycle and transport", Result, h_ldn_Policy, int p) { ACNH_NET_TRACE(); return ldn_result(dm_room_policy(p)); }
HLE_FORCE("_ZN2nn3ldn6RejectENS0_11Ipv4AddressE", "private ACNH LAN: real room lifecycle and transport", Result, h_ldn_Reject, uint32_t a) { ACNH_NET_TRACE(); return ldn_result(dm_room_reject(a)); }
HLE_FORCE("_ZN2nn3ldn16GetNetworkConfigEPNS0_13NetworkConfigE", "private ACNH LAN: real room lifecycle and transport", Result, h_ldn_NetCfg, void *o) { ACNH_NET_TRACE(); return ldn_result(dm_room_config(o)); }

// ---------- socket: offline. Initialization succeeds (the network library's global init depends on it), every later operation fails with errno = ENETDOWN ----------
enum { SK_ENETDOWN = 100, SK_EAI_FAIL = 4 };
HLE_FORCE("_ZN2nn6socket10InitializeEPvmmi", "connects to the bsd socket service; offline, creates no session (cut)", Result, h_sk_Init4, void *mem, uint64_t sz, uint64_t alloc, int conc) { ACNH_NET_TRACE(); (void)mem; (void)sz; (void)alloc; (void)conc; return RES_OK; }
HLE_FORCE("_ZN2nn6socket8FinalizeEv", "private LAN: real BSD socket operation", Result, h_sk_Fini, void) { ACNH_NET_TRACE(); dm_lan_socket_fini(); return RES_OK; }
HLE_FORCE("_ZN2nn6socket12GetLastErrnoEv", "private LAN: real BSD socket operation", int, h_sk_Errno, void) { ACNH_NET_TRACE(); return dm_lan_errno(); }
HLE_FORCE("_ZN2nn6socket6SocketEiii", "private LAN: real BSD socket operation", int, h_sk_Socket3i, int a, int b, int c) { ACNH_NET_TRACE(); return dm_lan_socket(a,b,c); }
HLE_FORCE("_ZN2nn6socket8ShutdownEii", "private LAN: real BSD socket operation", int, h_sk_Shutdown, int s, int h) { ACNH_NET_TRACE(); return dm_lan_shutdown(s,h); }
HLE_FORCE("_ZN2nn6socket4BindEiPK8sockaddrj", "private LAN: real BSD socket operation", int, h_sk_Bind, int s, const void *a, unsigned n) { ACNH_NET_TRACE(); return dm_lan_bind(s,a,n); }
int dm_sk_poll(void *fds, uint64_t n, int ms);   // hle_net.c: nothing to wait on → the timeout passes, 0; else -1
int dm_sk_select(int nfds, void *r, void *w, void *e, const int64_t *tv);
HLE_FORCE("_ZN2nn6socket4PollEP6pollfdmi", "offline: waits out the timeout when there is nothing to wait on, else fails", int, h_sk_Poll, void *p, uint64_t n, int t) { ACNH_NET_TRACE(); return dm_sk_poll(p, n, t); }
// SDK 4's Select with plain fd_set / timeval (Dark Souls Remastered)
HLE_FORCE("_ZN2nn6socket6SelectEiP6fd_setS2_S2_P7timeval", "offline: waits out the timeout when there is nothing to wait on, else fails", int, h_sk_Select2, int n, void *r, void *w, void *e, void *t) { ACNH_NET_TRACE(); return dm_sk_select(n, r, w, e, t); }
HLE_FORCE("_ZN2nn6socket6SendToEiPKvmiPK8sockaddrj", "private LAN: real BSD socket operation", long, h_sk_SendTo, int s, const void *b, unsigned long n, int f, const void *a, unsigned al) { ACNH_NET_TRACE(); return dm_lan_send(s,b,n,f,a,al); }
HLE_FORCE("_ZN2nn6socket6SendToEiPKvmNS0_7MsgFlagEPKNS0_8SockAddrEj", "private LAN: real BSD socket operation", long, h_sk_SendTo2, int s, const void *b, unsigned long n, int f, const void *a, unsigned al) { ACNH_NET_TRACE(); return dm_lan_send(s,b,n,f,a,al); }
HLE_FORCE("_ZN2nn6socket8RecvFromEiPvmiP8sockaddrPj", "private LAN: real BSD socket operation", long, h_sk_RecvFrom2, int s, void *b, unsigned long n, int f, void *a, unsigned *al) { ACNH_NET_TRACE(); return dm_lan_recv(s,b,n,f,a,al); }
HLE_FORCE("_ZN2nn6socket11GetAddrInfoEPKcS2_PK8addrinfoPPS3_", "name resolution (cut): EAI_FAIL", int, h_sk_Gai2, const char *h, const char *s, const void *hint, void **res) { ACNH_NET_TRACE(); (void)h; (void)s; (void)hint; if (res) *res = NULL; return SK_EAI_FAIL; }
HLE_FORCE("_ZN2nn6socket12FreeAddrInfoEP8addrinfo", "pairs with GetAddrInfo (cut)", void, h_sk_FreeAi2, void *ai) { ACNH_NET_TRACE(); (void)ai; }
HLE_FORCE("_ZN2nn6socket11GAIStrErrorENS0_7AiErrnoE", "resolver error text", const char *, h_sk_GaiStr, int e) { ACNH_NET_TRACE(); (void)e; return "network unavailable"; }
// SDK 11's overloads with plain sockaddr / int arguments (Big Brain Academy: Brain vs. Brain, through Pia)
HLE_FORCE("_ZN2nn6socket7ConnectEiPK8sockaddrj", "private LAN: real BSD socket operation", int, h_sk_Connect2, int s, const void *a, unsigned n) { ACNH_NET_TRACE(); return dm_lan_connect(s,a,n); }
HLE_FORCE("_ZN2nn6socket6AcceptEiP8sockaddrPj", "private LAN: real BSD socket operation", int, h_sk_Accept2, int s, void *a, unsigned *n) { ACNH_NET_TRACE(); return dm_lan_accept(s,a,n); }
HLE_FORCE("_ZN2nn6socket11GetSockNameEiP8sockaddrPj", "private LAN: real BSD socket operation", int, h_sk_GetSockName2, int s, void *a, unsigned *n) { ACNH_NET_TRACE(); return dm_lan_name(s,a,n,0); }
HLE_FORCE("_ZN2nn6socket10GetSockOptEiiiPvPj", "private LAN: real BSD socket operation", int, h_sk_GetSockOpt2, int s, int l, int o, void *v, unsigned *n) { ACNH_NET_TRACE(); return dm_lan_opt(s,l,o,v,n,0); }
HLE_FORCE("_ZN2nn6socket10SetSockOptEiiiPKvj", "private LAN: real BSD socket operation", int, h_sk_SetSockOpt2, int s, int l, int o, const void *v, unsigned n) { ACNH_NET_TRACE(); return dm_lan_opt(s,l,o,(void*)v,&n,1); }
HLE_FORCE("_ZN2nn6socket5FcntlEiiz", "private LAN: real BSD socket operation", int, h_sk_Fcntl2, int s, int c, int arg) { ACNH_NET_TRACE(); return dm_lan_fcntl(s,c,arg); }
HLE_FORCE("_ZN2nn6socket4RecvEiPvmi", "private LAN: real BSD socket operation", long, h_sk_Recv2, int s, void *b, unsigned long n, int f) { ACNH_NET_TRACE(); return dm_lan_recv(s,b,n,f,NULL,NULL); }
HLE_FORCE("_ZN2nn6socket4SendEiPKvmi", "private LAN: real BSD socket operation", long, h_sk_Send2, int s, const void *b, unsigned long n, int f) { ACNH_NET_TRACE(); return dm_lan_send(s,b,n,f,NULL,0); }

// ---------- irsensor: the Joy-Con IR camera (Big Brain Academy: Brain vs. Brain's input layer makes one per controller). The host has none: every camera is unconnected,
// processors cannot run and report no states. The default-config getters and MakeRect only fill the game's structs and keep running in the SDK ----------
enum { IRS_UNCONNECTED = 2 };   // IrCameraStatus: 0 available, 1 unsupported, 2 unconnected (the game takes 0 as "available", main+0x497fd4)
HLE_FORCE("_ZN2nn8irsensor17GetIrCameraHandleERKj", "IR camera handle of a controller (player number in the low byte)", uint32_t, h_irs_Handle, const uint32_t *npad) { return *npad & 0xff; }
HLE_FORCE("_ZN2nn8irsensor10InitializeERKNS0_14IrCameraHandleE", "IR camera (cut): nothing to initialize", void, h_irs_Init, const void *h) { (void)h; }
HLE_FORCE("_ZN2nn8irsensor8FinalizeERKNS0_14IrCameraHandleE", "IR camera (cut)", void, h_irs_Fini, const void *h) { (void)h; }
HLE_FORCE("_ZN2nn8irsensor17GetIrCameraStatusERKNS0_14IrCameraHandleE", "IR camera (cut): unconnected", int, h_irs_Status, const void *h) { (void)h; return IRS_UNCONNECTED; }
HLE_FORCE("_ZN2nn8irsensor18StopImageProcessorERKNS0_14IrCameraHandleE", "IR camera (cut)", void, h_irs_Stop, const void *h) { (void)h; }
HLE_FORCE("_ZN2nn8irsensor18RunMomentProcessorERKNS0_14IrCameraHandleERKNS0_21MomentProcessorConfigE", "IR camera (cut)", Result, h_irs_RunMoment, const void *h, const void *c) { (void)h; (void)c; return RES_FAIL; }
HLE_FORCE("_ZN2nn8irsensor22RunClusteringProcessorERKNS0_14IrCameraHandleERKNS0_25ClusteringProcessorConfigE", "IR camera (cut)", Result, h_irs_RunCluster, const void *h, const void *c) { (void)h; (void)c; return RES_FAIL; }
HLE_FORCE("_ZN2nn8irsensor15RunHandAnalysisERKNS0_14IrCameraHandleERKNS0_18HandAnalysisConfigE", "IR camera (cut)", Result, h_irs_RunHand, const void *h, const void *c) { (void)h; (void)c; return RES_FAIL; }
HLE_FORCE("_ZN2nn8irsensor25RunImageTransferProcessorERKNS0_14IrCameraHandleERKNS0_28ImageTransferProcessorConfigEPvm", "IR camera (cut)", Result, h_irs_RunImage,
          const void *h, const void *c, void *b, size_t n) { (void)h; (void)c; (void)b; (void)n; return RES_FAIL; }
HLE_FORCE("_ZN2nn8irsensor25RunImageTransferProcessorERKNS0_14IrCameraHandleERKNS0_30ImageTransferProcessorExConfigEPvm", "IR camera (cut)", Result, h_irs_RunImageEx,
          const void *h, const void *c, void *b, size_t n) { (void)h; (void)c; (void)b; (void)n; return RES_FAIL; }
HLE_FORCE("_ZN2nn8irsensor24GetMomentProcessorStatesEPNS0_20MomentProcessorStateEPiiRKNS0_14IrCameraHandleE", "IR camera (cut): 0 states", Result, h_irs_Moment,
          void *s, int *n, int max, const void *h) { (void)s; (void)max; (void)h; if (n) *n = 0; return RES_FAIL; }
HLE_FORCE("_ZN2nn8irsensor28GetClusteringProcessorStatesEPNS0_24ClusteringProcessorStateEPiiRKNS0_14IrCameraHandleE", "IR camera (cut): 0 states", Result, h_irs_Cluster,
          void *s, int *n, int max, const void *h) { (void)s; (void)max; (void)h; if (n) *n = 0; return RES_FAIL; }
HLE_FORCE("_ZN2nn8irsensor25GetHandAnalysisImageStateEPNS0_22HandAnalysisImageStateEPiilRKNS0_14IrCameraHandleE", "IR camera (cut): 0 states", Result, h_irs_HandImage,
          void *s, int *n, int max, int64_t since, const void *h) { (void)s; (void)max; (void)since; (void)h; if (n) *n = 0; return RES_FAIL; }
HLE_FORCE("_ZN2nn8irsensor30GetHandAnalysisSilhouetteStateEPNS0_27HandAnalysisSilhouetteStateEPiilRKNS0_14IrCameraHandleE", "IR camera (cut): 0 states", Result, h_irs_HandSil,
          void *s, int *n, int max, int64_t since, const void *h) { (void)s; (void)max; (void)since; (void)h; if (n) *n = 0; return RES_FAIL; }
HLE_FORCE("_ZN2nn8irsensor30GetImageTransferProcessorStateEPNS0_27ImageTransferProcessorStateEPvmRKNS0_14IrCameraHandleE", "IR camera (cut)", Result, h_irs_ImageState,
          void *s, void *b, size_t n, const void *h) { (void)s; (void)b; (void)n; (void)h; return RES_FAIL; }

// ---------- ssl: offline, creating a context fails ----------
HLE_FORCE("_ZN2nn3ssl7Context15ImportClientPkiEPmPKcS4_jj", "SSL (cut)", Result, h_ssl_ImpCli, void *self, uint64_t *id, const char *a, const char *b, unsigned x, unsigned y) { (void)self; (void)a; (void)b; (void)x; (void)y; if (id) *id = 0; return RES_FAIL; }
HLE_FORCE("_ZN2nn3ssl7Context15ImportServerPkiEPmPKcjNS0_17CertificateFormatE", "SSL (cut)", Result, h_ssl_ImpSrv, void *self, uint64_t *id, const char *a, unsigned n, int f) { (void)self; (void)a; (void)n; (void)f; if (id) *id = 0; return RES_FAIL; }
HLE_FORCE("_ZN2nn3ssl7Context9ImportCrlEPmPKcj", "SSL (cut)", Result, h_ssl_ImpCrl, void *self, uint64_t *id, const char *a, unsigned n) { (void)self; (void)a; (void)n; if (id) *id = 0; return RES_FAIL; }
HLE_FORCE("_ZN2nn3ssl14BuiltInManager22GetBuiltInCertificatesEPPNS0_26BuiltInCertificateInfoBaseEPhjPNS0_15CaCertificateIdEj", "SSL (cut)", Result, h_ssl_Certs,
          void **out, uint8_t *buf, unsigned bn, void *ids, unsigned n) { (void)buf; (void)bn; (void)ids; (void)n; if (out) *out = NULL; return RES_FAIL; }

// ---------- nifm / nsd: offline ----------
HLE_FORCE("_ZN2nn4nifm19SetLocalNetworkModeEb", "private LAN: enable local network status only for local play", void, h_nifm_LocalMode, unsigned on) { ACNH_NET_TRACE(); dm_room_local_mode(on&255); }
HLE_FORCE("_ZN2nn4nifm27SubmitNetworkRequestAndWaitEv", "offline: the network request is not submitted (returns at once, IsNetworkAvailable is always false)", void, h_nifm_SubmitWait, void) { ACNH_NET_TRACE();}
HLE_FORCE("_ZN2nn4nifm22GetCurrentIpConfigInfoEP7in_addrS2_S2_S2_S2_", "private LAN: local IPv4 configuration, gateway/DNS unspecified", Result, h_nifm_IpCfg, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d, uint32_t *e) { ACNH_NET_TRACE();
    uint32_t ip,mask;
    uint32_t *p[5] = {a, b, c, d, e}; for (int i = 0; i < 5; i++) if (p[i]) *p[i] = 0;
    if(!a||!b||dm_room_network_ip(&ip,&mask))return RES_FAIL;
    *a=htonl(ip);*b=htonl(mask);return RES_OK; // Local routes need no gateway/DNS.
}
// SDK 10's functions of the same name take socket::InAddr / in_addr, exactly the other way round from the generation above and in hle_net.c (Super Mario 3D World + Bowser's Fury)
HLE_FORCE("_ZN2nn4nifm22GetCurrentIpConfigInfoEPNS_6socket6InAddrES3_S3_S3_S3_", "private LAN: local IPv4 configuration (SDK 10 overload)", Result, h_nifm_IpCfg10, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d, uint32_t *e) { ACNH_NET_TRACE();
    uint32_t ip,mask;
    uint32_t *p[5] = {a, b, c, d, e}; for (int i = 0; i < 5; i++) if (p[i]) *p[i] = 0;
    if(!a||!b||dm_room_network_ip(&ip,&mask))return RES_FAIL;
    *a=htonl(ip);*b=htonl(mask);return RES_OK; // Local routes need no gateway/DNS.
}
HLE_FORCE("_ZN2nn4nifm26GetCurrentPrimaryIpAddressEP7in_addr", "offline: no IP (SDK 10 overload)", Result, h_nifm_GetIp10, uint32_t *a) { ACNH_NET_TRACE(); uint32_t mask,ip; if(!a||dm_room_network_ip(&ip,&mask))return RES_FAIL; *a=htonl(ip); return RES_OK; }
// EnvironmentIdentifier = char[8]: retail environment "lp1"
HLE_FORCE("_ZN2nn3nsd24GetEnvironmentIdentifierEPNS0_21EnvironmentIdentifierE", "server environment name: retail lp1", void, h_nsd_EnvId, char *out) { memset(out, 0, 8); memcpy(out, "lp1", 3); }

// ---------- friends: newer SDK overloads ----------
HLE_FORCE("_ZN2nn7friends12SetAllocatorEPFPvmEPFvS1_mE", "friends library allocator; no session is created, ignored", void, h_fr_SetAlloc, void *a, void *f) { (void)a; (void)f; }
HLE_FORCE("_ZN2nn7friends13GetFriendListEPiPNS0_6FriendERKNS_7account3UidEiiRKNS0_12FriendFilterE", "friend list is empty (cut)", Result, h_fr_List2,
          int *count, void *out, const void *uid, int off, int n, const void *filt) { (void)out; (void)uid; (void)off; (void)n; (void)filt; if (count) *count = 0; return RES_FAIL; }
HLE_FORCE("_ZN2nn7friends27CheckFriendListAvailabilityEPbRKNS_7account3UidE", "friend list unavailable (cut)", Result, h_fr_ListAvail, uint8_t *out, const void *uid) { (void)uid; *out = 0; return RES_OK; }
HLE_FORCE("_ZN2nn7friends25EnsureFriendListAvailableEPNS0_12AsyncContextERKNS_7account3UidE", "friend list (cut): the request fails at once", Result, h_fr_ListEnsure, void *ctx, const void *uid) { (void)ctx; (void)uid; return RES_FAIL; }
HLE_FORCE("_ZN2nn7friends12AsyncContext7HasDoneEPb", "async context: always done", Result, h_fr_AcHasDone, void *self, uint8_t *done) { (void)self; *done = 1; return RES_OK; }
HLE_FORCE("_ZN2nn7friends14AddPlayHistoryERKNS_7account3UidERKNS0_26PlayHistoryRegistrationKeyERKNS0_15InAppScreenNameESA_", "private LAN: persist local play history",
          Result, h_fr_PlayHist2, const void *uid, const void *key, const void *a, const void *b) { return dm_lan_history_add(uid,key,a,b)==0?RES_OK:RES_FAIL; }

// ---------- ngc: newer SDK overloads (with bool). Always judged as no profanity ----------
HLE_FORCE("_ZN2nn3ngc15ProfanityFilter10InitializeEPvmb", "profanity filter init; the host has no word list", Result, h_ngc_Init3, void *self, void *work, uint64_t sz, unsigned check) { (void)self; (void)work; (void)sz; (void)check; return RES_OK; }
HLE_FORCE("_ZN2nn3ngc15ProfanityFilter19CheckProfanityWordsEPjjPPKDsm", "profanity check: no profanity in anything", Result, h_ngc_Check16, void *self, uint32_t *out, unsigned mask, const void **words, uint64_t n) {
    (void)self; (void)mask; (void)words; for (uint64_t i = 0; out && i < n; i++) out[i] = 0; return RES_OK;
}

// ---------- err: error display applet. Only logged, no UI is shown ----------
HLE_FORCE("_ZN2nn3err9ShowErrorENS0_9ErrorCodeE", "error display applet; the host only logs it", void, h_err_ShowCode, uint64_t code) { DM_APPLET("error"); LOG("game asks to show an error: %04u-%04u", (unsigned)(code & 0xffffffff), (unsigned)(code >> 32)); }
HLE_FORCE("_ZN2nn3err9ShowErrorERKNS0_18ErrorResultVariantE", "error display applet; the host only logs it", void, h_err_ShowVar, const void *v) { DM_APPLET("error"); (void)v; LOG("game asks to show an error (ErrorResultVariant)"); }
HLE_FORCE("_ZN2nn3err40ShowUnacceptableAddOnContentVersionErrorEv", "add-on content version mismatch error applet; the host only logs it", void, h_err_AocVer, void) { DM_APPLET("error"); LOG("game asks to show an error: add-on content version mismatch"); }
HLE_FORCE("_ZN2nn3err39ShowUnacceptableApplicationVersionErrorEv", "game version mismatch error applet; the host only logs it", void, h_err_AppVer, void) { DM_APPLET("error"); LOG("game asks to show an error: game version mismatch"); }
// Pokémon Legends: Z-A (SDK 19): the application's own error screen (an error code number and two messages in the argument) goes through the error applet as well
HLE_FORCE("_ZN2nn3err20ShowApplicationErrorERKNS0_19ApplicationErrorArgE", "application error applet; the host only logs it", void, h_err_ShowApp, const void *arg) { DM_APPLET("error"); (void)arg; LOG("game asks to show an application error (ApplicationErrorArg)"); }
// Pokémon Sword (SDK 7.3.2): the error applet's network service form, after a failed online call
HLE_FORCE("_ZN2nn3err23ShowNetworkServiceErrorENS0_9ErrorCodeE", "network service error applet; the host only logs it", void, h_err_ShowNet, uint64_t code) { DM_APPLET("error"); LOG("game asks to show a network service error: %04u-%04u", (unsigned)(code & 0xffffffff), (unsigned)(code >> 32)); }

// ---------- pdm: the system's play log ----------
// Pokémon Sword asks at boot how often this user has launched two other games (main+0xc88074, one ID each call, 0x18 bytes {application ID, play time, launch count} per entry; the result is
// not read, a launch count of at least 1 is): the host keeps no play log, so every one was never played. The SDK's own call opens the pdm service (null read at sdk+0x336ccc)
HLE_FORCE("_ZN2nn3pdm30QueryApplicationPlayStatisticsEPNS0_25ApplicationPlayStatisticsERKNS_7account3UidEPKNS_13ApplicationIdEi", "play statistics of other games: never played (the host keeps no play log)",
          Result, h_pdm_QueryStats, uint64_t *out, const void *uid, const uint64_t *ids, int n) {
    (void)uid; for (int i = 0; out && ids && i < n; i++) { out[3 * i] = ids[i]; out[3 * i + 1] = 0; out[3 * i + 2] = 0; }
    return RES_OK;
}

// ---------- web: web page applet (online lobby). Unavailable ----------
HLE_FORCE("_ZN2nn3web13ShowLobbyPageEPNS0_20LobbyPageReturnValueERKNS0_16ShowLobbyPageArgE", "online lobby web applet (cut)", Result, h_web_Lobby, void *rv, const void *arg) { (void)rv; (void)arg; LOG("web::ShowLobbyPage (cut)"); return RES_FAIL; }
// Tears of the Kingdom imports the general web page applet too (the game constructs the return value itself, so it stays as constructed)
HLE_FORCE("_ZN2nn3web11ShowWebPageEPNS0_18WebPageReturnValueERKNS0_14ShowWebPageArgE", "web page applet (cut)", Result, h_web_Page, void *rv, const void *arg) { (void)rv; (void)arg; LOG("web::ShowWebPage (cut)"); return RES_FAIL; }
// Octopath Traveler 0 imports the offline HTML page applet too (pages shipped in the game's HTML document NCA, which the host does not extract)
HLE_FORCE("_ZN2nn3web19ShowOfflineHtmlPageEPNS0_26OfflineHtmlPageReturnValueERKNS0_22ShowOfflineHtmlPageArgE", "offline HTML page applet (cut)", Result, h_web_Offline, void *rv, const void *arg) {
    (void)rv; (void)arg; LOG("web::ShowOfflineHtmlPage (cut)"); return RES_FAIL;
}

// ---------- C interface (nnsocket* / nnssl*): offline as well. The arguments are all in registers and the host does not look at them ----------
HLE_FORCE("nnsocketAccept", "offline (cut)", int, h_c_sk_Accept, void) { return -1; }
HLE_FORCE("nnsocketListen", "offline (cut)", int, h_c_sk_Listen, void) { return -1; }
HLE_FORCE("nnsocketRecvMsg", "offline (cut)", int, h_c_sk_RecvMsg, void) { return -1; }
HLE_FORCE("nnsocketSendMsg", "offline (cut)", int, h_c_sk_SendMsg, void) { return -1; }
HLE_FORCE("nnsocketShutdown", "offline (cut)", int, h_c_sk_Shutdown, void) { return -1; }
HLE_FORCE("nnsocketGetAddrInfo", "name resolution (cut): EAI_FAIL", int, h_c_sk_Gai, const char *h, const char *s, const void *hint, void **res) { (void)h; (void)s; (void)hint; if (res) *res = NULL; return SK_EAI_FAIL; }
HLE_FORCE("nnsocketFreeAddrInfo", "pairs with GetAddrInfo (cut)", void, h_c_sk_FreeAi, void *ai) { (void)ai; }
HLE_FORCE("nnsocketGAIStrError", "resolver error text", const char *, h_c_sk_GaiStr, int e) { (void)e; return "network unavailable"; }

// ---------- bcat: news / data delivered from the server (Super Mario Odyssey). Initialize talks to the bcat service (an SDK abort without it); mounting the delivery cache fails,
// which the game treats as "nothing delivered" (main+0x6c9258: a failure skips the read unless the caller asked for an error dialog, which only logs here). Syncing needs the network, which nifm reports unavailable ----------
HLE_FORCE("_ZN2nn4bcat10InitializeEv", "data delivery init (connects to the bcat service); the host has none, no-op", void, h_bcat_Init, void) {}
HLE_FORCE("_ZN2nn4bcat25MountDeliveryCacheStorageEv", "mount delivery cache: no delivery data, fails", Result, h_bcat_Mount, void) { return RES_FAIL; }
HLE_FORCE("_ZN2nn4bcat27UnmountDeliveryCacheStorageEv", "unmount delivery cache", void, h_bcat_Unmount, void) {}

// ---------- album: the capture album (Monster Hunter Rise saves its own photo-mode pictures there). Initialize connects to the capture service (capsrv, an abort in sm::Initialize at the start of
// nnMain, main+0x6bc5478); the host has no album, so saving fails and showing it does nothing. GetImageWidth / GetImageHeight only compute and keep running in the SDK ----------
HLE_FORCE("_ZN2nn5album10InitializeEv", "capture album init (capsrv); the host has no album, no-op", void, h_album_Init, void) {}
HLE_FORCE("_ZN2nn5album8FinalizeEv", "capture album", void, h_album_Fini, void) {}
HLE_FORCE("_ZN2nn5album14SaveScreenshotEPKvmNS0_9ImageSizeENS0_16ImageOrientationENS0_17AlbumReportOptionE", "save a picture to the album: no album, fails", Result, h_album_Save,
          const void *p, size_t n, int size, int orient, int report) { (void)orient; (void)report; int saved=dm_album_save_local(p,n); LOG("album::SaveScreenshot bytes=%zu size=%d local=%s",n,size,saved?"saved":"failed"); return saved?RES_OK:RES_FAIL; }
// The SDK 7 overload, without an orientation (DRAGON QUEST XI S)
HLE_FORCE("_ZN2nn5album14SaveScreenshotEPKvmNS0_9ImageSizeENS0_17AlbumReportOptionE", "save a picture to the album: no album, fails", Result, h_album_Save7,
          const void *p, size_t n, int size, int report) { (void)report; int saved=dm_album_save_local(p,n); LOG("album::SaveScreenshot bytes=%zu size=%d local=%s",n,size,saved?"saved":"failed"); return saved?RES_OK:RES_FAIL; }
HLE_FORCE("_ZN2nn5album22SaveAndShareScreenshotEPKvmNS0_9ImageSizeENS0_16ImageOrientationENS0_17AlbumReportOptionERKNS_7account3UidEPKc", "save and share a picture: no album, fails", Result, h_album_Share,
          const void *p, size_t n, int size, int orient, int report, const void *uid, const char *text) { (void)p; (void)n; (void)size; (void)orient; (void)report; (void)uid; (void)text; return RES_FAIL; }
// The SDK 20 overload that hands back a share ID, and the URL of the last post (Animal Crossing: New Horizons 3.0.3)
HLE_FORCE("_ZN2nn5album22SaveAndShareScreenshotEPNS0_7ShareIdEPKvmNS0_9ImageSizeENS0_17AlbumReportOptionERKNS_7account3UidEPKc", "save and share a picture: no album, fails", Result, h_album_ShareId,
          void *id, const void *p, size_t n, int size, int report, const void *uid, const char *text) { (void)id; (void)p; (void)n; (void)size; (void)report; (void)uid; (void)text; return RES_FAIL; }
HLE_FORCE("_ZN2nn5album21GetSharePostedLastUrlEPcmRKNS0_7ShareIdE", "URL of the last shared post: nothing was shared, fails", Result, h_album_ShareUrl, char *url, size_t n, const void *id) {
    (void)id; if (url && n) url[0] = 0; return RES_FAIL; }
HLE_FORCE("_ZN2nn5album14ShowAlbumFilesEv", "album applet: not shown", Result, h_album_Show, void) { return RES_FAIL; }
// Pokémon Legends: Z-A (SDK 19): a screenshot of the current frame, taken by the capture service
HLE_FORCE("_ZN2nn5album21SaveCurrentScreenshotENS0_17AlbumReportOptionE", "screenshot of the current frame (capsrv): no album, fails", Result, h_album_SaveCur, int report) { (void)report; LOG("album::SaveCurrentScreenshot (no album)"); return RES_FAIL; }
