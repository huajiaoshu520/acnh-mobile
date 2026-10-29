// Networking (out of scope: online play/friends/store/cloud saves): all hooked up as "network unavailable", the game takes the offline branch.
// Cuts: nifm reports no network; socket only hooks up Initialize (returns success, creates no session), the rest of socket/ssl/friends are hooked up as failures when they come into use.
#include "hle.h"
#include "comm_test_trace.h"
#include "lan_socket.h"
#include "lan_room.h"
#include <arpa/inet.h>
#include <stdatomic.h>

HLE_FORCE("_ZN2nn6socket10InitializeERKNS0_6ConfigE", "connects to the bsd socket service; offline, creates no session (cut)", Result, h_socket_Initialize, const void *cfg) { ACNH_NET_TRACE(); (void)cfg; return RES_OK; }
HLE_FORCE("_ZN2nn4nifm10InitializeEv", "connects to the nifm network management service; offline (cut)", Result, h_nifm_Initialize, void) { ACNH_NET_TRACE(); return RES_OK; }
// The request is asynchronous on Horizon: SubmitNetworkRequest only hands it to the nifm service, and IsNetworkRequestOnHold stays true until the service has evaluated it, so a request
// never settles within microseconds of its submission. Pikmin 4 (Unreal Engine 4.26, main+0x203f3b0) relies on that: it starts its BCAT thread, whose Run (main+0x203f510) sets its status
// to 2, submits, waits while on hold and sets 6 (done, offline), and only then stores status 1 itself; answered at once, a thread that ran first finished before that store, the 1 stayed,
// and the boot state machine (main+0x1f2f764, state 7) waited for the status forever at the black loading screen (about 1 boot in 6, 3 in 4 with four at once). The request now stays
// on hold for NIFM_HOLD_NS after it is submitted, then settles as unavailable
enum { NIFM_HOLD_NS = 50000000 };
static _Atomic uint64_t g_nifm_submit;   // when the pending request was submitted (now_ns), 0 = none
HLE_FORCE("_ZN2nn4nifm20SubmitNetworkRequestEv", "offline: the request is on hold for 50 ms, then settles as unavailable", void, h_nifm_Submit, void) { ACNH_NET_TRACE(); atomic_store(&g_nifm_submit, now_ns() | 1); }
HLE_FORCE("_ZN2nn4nifm20CancelNetworkRequestEv", "offline: drops the pending request", void, h_nifm_Cancel, void) { ACNH_NET_TRACE(); atomic_store(&g_nifm_submit, 0); }
HLE_FORCE("_ZN2nn4nifm22IsNetworkRequestOnHoldEv", "offline: on hold until 50 ms after the submission", int, h_nifm_OnHold, void) { ACNH_NET_TRACE();
    uint64_t t = atomic_load(&g_nifm_submit);
    return t && now_ns() - t < NIFM_HOLD_NS;
}
HLE_FORCE("_ZN2nn4nifm18IsNetworkAvailableEv", "offline: the network is always unavailable", int, h_nifm_Available, void) { ACNH_NET_TRACE(); uint32_t ip,mask; return dm_room_network_ip(&ip,&mask)==0; }
HLE_FORCE("_ZN2nn4nifm31HandleNetworkRequestErrorResultEv", "offline: reports failure, shows no system network screen", Result, h_nifm_HandleErr, void) { ACNH_NET_TRACE(); uint32_t ip,mask; return dm_room_network_ip(&ip,&mask)==0?RES_OK:RES_FAIL; }
HLE_FORCE("_ZN2nn4nifm26GetCurrentPrimaryIpAddressEPNS_6socket6InAddrE", "offline: no IP", Result, h_nifm_GetIp, uint32_t *a) { ACNH_NET_TRACE(); uint32_t mask,ip; if(!a||dm_room_network_ip(&ip,&mask))return RES_FAIL; *a=htonl(ip); return RES_OK; }
HLE_FORCE("_ZN2nn4nifm22GetCurrentProxySettingEPNS0_12ProxySettingE", "offline: no proxy setting", Result, h_nifm_GetProxy, void *p) { ACNH_NET_TRACE(); (void)p; return RES_FAIL; }
HLE_FORCE("nnsslInitialize", "connects to the ssl service; the network library's global init needs it to succeed, offline creates no session (cut)", Result, h_ssl_Initialize, void) { return RES_OK; }
HLE_FORCE("nnsslFinalize", "pairs with nnsslInitialize (cut)", Result, h_ssl_Finalize, void) { return RES_OK; }

// ---------- socket: offline, all calls that contact the service return -1 in BSD convention, error code ENETDOWN ----------
// Not cut: Inet* / nnsocketInet* byte order and address text conversion (pure computation).
#define SK_ENETDOWN 100                  // This ACNH SDK uses Linux/POSIX error codes

static int sk_fail(void) { return dm_lan_error(SK_ENETDOWN); }
#define SKF(m, cname, ...) HLE_FORCE(m, "private LAN socket service; unsupported operations still fail", int, cname, __VA_ARGS__)
// Poll / Select with nothing to wait on: the host creates no socket, so a game's descriptors are all -1, or it passes none. POSIX and Horizon's bsd service ignore negative
// descriptors and wait out the timeout, returning 0. Failing at once made Dark Souls Remastered's network thread ("NexusRevolution Socket", main+0x92c7c4) spin a core at 100%
// on Poll(fds, 0, 33 ms), 11,000 calls a second. An endless wait on nothing never returns on hardware; here it returns 0 after 100 ms: a thread in a host hook pauses for a garbage
// collector (IL2CPP's, hle_thread.c) only once the hook returns, and the caller can still see a shutdown
void dm_guest_sleep_ns(int64_t ns);
static int sk_idle(int64_t ms) { dm_guest_sleep_ns((ms < 0 ? 100 : ms) * 1000000); return 0; }
typedef struct { int32_t fd; int16_t events, revents; } sk_pollfd;
int dm_sk_poll(void *fds, uint64_t n, int ms) { return dm_lan_poll(fds,n,ms); }
int dm_sk_select(int nfds, void *r, void *w, void *e, const int64_t *tv) { return dm_lan_select(nfds,r,w,e,tv); }
SKF("_ZN2nn6socket6SocketENS0_6FamilyENS0_4TypeENS0_8ProtocolE", h_sk_Socket, int a, int b, int c) { ACNH_NET_TRACE(); return dm_lan_socket(a,b,c); }
SKF("_ZN2nn6socket4BindEiPKNS0_8SockAddrEj", h_sk_Bind, int s, const void *a, unsigned n) { ACNH_NET_TRACE(); return dm_lan_bind(s,a,n); }
SKF("_ZN2nn6socket7ConnectEiPKNS0_8SockAddrEj", h_sk_Connect, int s, const void *a, unsigned n) { ACNH_NET_TRACE(); return dm_lan_connect(s,a,n); }
SKF("_ZN2nn6socket6ListenEii", h_sk_Listen, int s, int n) { ACNH_NET_TRACE(); return dm_lan_listen(s,n); }
SKF("_ZN2nn6socket6AcceptEiPNS0_8SockAddrEPj", h_sk_Accept, int s, void *a, unsigned *n) { ACNH_NET_TRACE(); return dm_lan_accept(s,a,n); }
SKF("_ZN2nn6socket5CloseEi", h_sk_Close, int s) { ACNH_NET_TRACE(); return dm_lan_close(s); }
SKF("_ZN2nn6socket11GetPeerNameEiPNS0_8SockAddrEPj", h_sk_GetPeerName, int s, void *a, unsigned *n) { ACNH_NET_TRACE(); return dm_lan_name(s,a,n,1); }
SKF("_ZN2nn6socket11GetSockNameEiPNS0_8SockAddrEPj", h_sk_GetSockName, int s, void *a, unsigned *n) { ACNH_NET_TRACE(); return dm_lan_name(s,a,n,0); }
SKF("_ZN2nn6socket10GetSockOptEiNS0_5LevelENS0_6OptionEPvPj", h_sk_GetSockOpt, int s, int l, int o, void *v, unsigned *n) { ACNH_NET_TRACE(); return dm_lan_opt(s,l,o,v,n,0); }
SKF("_ZN2nn6socket10SetSockOptEiNS0_5LevelENS0_6OptionEPKvj", h_sk_SetSockOpt, int s, int l, int o, const void *v, unsigned n) { ACNH_NET_TRACE(); return dm_lan_opt(s,l,o,(void*)v,&n,1); }
SKF("_ZN2nn6socket5FcntlEiNS0_12FcntlCommandEz", h_sk_Fcntl, int s, int c, int arg) { ACNH_NET_TRACE(); return dm_lan_fcntl(s,c,arg); }
SKF("_ZN2nn6socket4PollEPNS0_6PollFdEmi", h_sk_Poll, void *f, unsigned long n, int t) { ACNH_NET_TRACE(); return dm_sk_poll(f, n, t); }
SKF("_ZN2nn6socket6SelectEiPNS0_5FdSetES2_S2_PNS0_7TimeValE", h_sk_Select, int n, void *r, void *w, void *e, void *t) { ACNH_NET_TRACE(); return dm_sk_select(n, r, w, e, t); }
SKF("_ZN2nn6socket7EventFdEmNS0_12EventFdFlagsE", h_sk_EventFd, unsigned long v, int f) { ACNH_NET_TRACE(); (void)v; (void)f; return sk_fail(); }
SKF("_ZN2nn6socket11EventFdReadEiPm", h_sk_EventFdRead, int s, unsigned long *v) { ACNH_NET_TRACE(); (void)s; (void)v; return sk_fail(); }
SKF("_ZN2nn6socket12EventFdWriteEim", h_sk_EventFdWrite, int s, unsigned long v) { ACNH_NET_TRACE(); (void)s; (void)v; return sk_fail(); }
SKF("_ZN2nn6socket17ResolverGetOptionEPNS0_14ResolverOptionENS0_17ResolverOptionKeyE", h_sk_ResGet, void *o, int k) { ACNH_NET_TRACE(); (void)o; (void)k; return sk_fail(); }
SKF("_ZN2nn6socket17ResolverSetOptionERKNS0_14ResolverOptionE", h_sk_ResSet, const void *o) { ACNH_NET_TRACE(); (void)o; return sk_fail(); }
HLE_FORCE("_ZN2nn6socket4RecvEiPvmNS0_7MsgFlagE", "private LAN: real BSD socket operation", long, h_sk_Recv, int s, void *b, unsigned long n, int f) { ACNH_NET_TRACE(); return dm_lan_recv(s,b,n,f,NULL,NULL); }
HLE_FORCE("_ZN2nn6socket8RecvFromEiPvmNS0_7MsgFlagEPNS0_8SockAddrEPj", "private LAN: real BSD socket operation", long, h_sk_RecvFrom, int s, void *b, unsigned long n, int f, void *a, unsigned *al) { ACNH_NET_TRACE(); return dm_lan_recv(s,b,n,f,a,al); }
HLE_FORCE("_ZN2nn6socket4SendEiPKvmNS0_7MsgFlagE", "private LAN: real BSD socket operation", long, h_sk_Send, int s, const void *b, unsigned long n, int f) { ACNH_NET_TRACE(); return dm_lan_send(s,b,n,f,NULL,0); }
HLE_FORCE("_ZN2nn6socket11GetAddrInfoEPKcS2_PKNS0_8AddrInfoEPPS3_", "name resolution (cut): EAI_FAIL", int, h_sk_GetAddrInfo, const char *h, const char *s, const void *hint, void **res) { ACNH_NET_TRACE();
    (void)h; (void)s; (void)hint; if (res) *res = NULL; return 4;   // EAI_FAIL
}
HLE_FORCE("_ZN2nn6socket12FreeAddrInfoEPNS0_8AddrInfoE", "pairs with GetAddrInfo (cut)", void, h_sk_FreeAddrInfo, void *ai) { ACNH_NET_TRACE(); (void)ai; }
HLE_FORCE("_ZN2nn6socket16GetHostEntByNameEPKcPKNS0_14ResolverOptionEm", "name resolution (cut): NULL", void *, h_sk_GetHostEnt, const char *n, const void *o, unsigned long c) { ACNH_NET_TRACE(); (void)n; (void)o; (void)c; dm_lan_error(SK_ENETDOWN); return NULL; }
HLE_FORCE("_ZN2nn6socket12GetLastErrorEv", "private LAN: real BSD socket operation", int, h_sk_GetLastError, void) { ACNH_NET_TRACE(); return dm_lan_errno(); }
// C interface
SKF("nnsocketSocket", h_nsk_Socket, int a, int b, int c) { ACNH_NET_TRACE(); return dm_lan_socket(a,b,c); }
SKF("nnsocketBind", h_nsk_Bind, int s, const void *a, unsigned n) { ACNH_NET_TRACE(); return dm_lan_bind(s,a,n); }
SKF("nnsocketConnect", h_nsk_Connect, int s, const void *a, unsigned n) { ACNH_NET_TRACE(); return dm_lan_connect(s,a,n); }
SKF("nnsocketClose", h_nsk_Close, int s) { ACNH_NET_TRACE(); return dm_lan_close(s); }
SKF("nnsocketGetPeerName", h_nsk_GetPeerName, int s, void *a, unsigned *n) { ACNH_NET_TRACE(); return dm_lan_name(s,a,n,1); }
SKF("nnsocketGetSockName", h_nsk_GetSockName, int s, void *a, unsigned *n) { ACNH_NET_TRACE(); return dm_lan_name(s,a,n,0); }
SKF("nnsocketGetSockOpt", h_nsk_GetSockOpt, int s, int l, int o, void *v, unsigned *n) { ACNH_NET_TRACE(); return dm_lan_opt(s,l,o,v,n,0); }
SKF("nnsocketSetSockOpt", h_nsk_SetSockOpt, int s, int l, int o, const void *v, unsigned n) { ACNH_NET_TRACE(); return dm_lan_opt(s,l,o,(void*)v,&n,1); }
SKF("nnsocketFcntl", h_nsk_Fcntl, int s, int c, int arg) { ACNH_NET_TRACE(); return dm_lan_fcntl(s,c,arg); }
SKF("nnsocketPoll", h_nsk_Poll, void *f, unsigned long n, int t) { ACNH_NET_TRACE(); return dm_sk_poll(f, n, t); }
SKF("nnsocketResolverGetOption", h_nsk_ResGet, void *o, int k) { ACNH_NET_TRACE(); (void)o; (void)k; return sk_fail(); }
SKF("nnsocketResolverSetOption", h_nsk_ResSet, const void *o) { ACNH_NET_TRACE(); (void)o; return sk_fail(); }
HLE_FORCE("nnsocketRecv", "private LAN: real BSD socket operation", long, h_nsk_Recv, int s, void *b, unsigned long n, int f) { ACNH_NET_TRACE(); return dm_lan_recv(s,b,n,f,NULL,NULL); }
HLE_FORCE("nnsocketSend", "private LAN: real BSD socket operation", long, h_nsk_Send, int s, const void *b, unsigned long n, int f) { ACNH_NET_TRACE(); return dm_lan_send(s,b,n,f,NULL,0); }
HLE_FORCE("nnsocketGetHostByNameWithOptions", "name resolution (cut): NULL", void *, h_nsk_GetHost, const char *n, const void *o, unsigned long c) { (void)n; (void)o; (void)c; dm_lan_error(SK_ENETDOWN); return NULL; }
HLE_FORCE("nnsocketGetLastErrno", "socket error code kept by the host", int, h_nsk_GetErrno, void) { return dm_lan_errno(); }
HLE_FORCE("nnsocketSetLastErrno", "socket error code kept by the host", void, h_nsk_SetErrno, int e) { dm_lan_error(e); }

// ---------- ssl: offline, Initialize succeeds (the network library's global initialization depends on it), everything else that contacts the service fails ----------
// Not cut: Context / Connection construction and destruction, Context::GetContextId (only read and write game objects).
#define SSF(m, cname, ...) HLE_FORCE(m, "ssl service (cut): fails", Result, cname, __VA_ARGS__)
HLE_FORCE("_ZN2nn3ssl10InitializeEv", "same as nnsslInitialize: succeeds, creates no session (cut)", Result, h_ssl_InitCpp, void) { return RES_OK; }
HLE_FORCE("_ZN2nn3ssl8FinalizeEv", "pairs with Initialize (cut)", Result, h_ssl_FinCpp, void) { return RES_OK; }
SSF("_ZN2nn3ssl7Context6CreateENS1_10SslVersionE", h_ssl_CtxCreate, void *self, int v) { (void)self; (void)v; return RES_FAIL; }
SSF("_ZN2nn3ssl7Context7DestroyEv", h_ssl_CtxDestroy, void *self) { (void)self; return RES_FAIL; }
SSF("_ZN2nn3ssl7Context19RegisterInternalPkiEPmNS1_11InternalPkiE", h_ssl_CtxRegPki, void *self, uint64_t *id, int p) { (void)self; (void)id; (void)p; return RES_FAIL; }
SSF("_ZN2nn3ssl7Context9RemovePkiEm", h_ssl_CtxRemovePki, void *self, uint64_t id) { (void)self; (void)id; return RES_FAIL; }
SSF("_ZN2nn3ssl14BuiltInManager28GetBuiltInCertificateBufSizeEPjPNS0_15CaCertificateIdEj", h_ssl_BiBufSize, uint32_t *o, void *ids, unsigned n) { (void)ids; (void)n; if (o) *o = 0; return RES_FAIL; }
SSF("_ZN2nn3ssl14BuiltInManager22GetBuiltInCertificatesEPPNS1_22BuiltInCertificateInfoEPhjPNS0_15CaCertificateIdEj", h_ssl_BiCerts, void **o, uint8_t *b, unsigned bs, void *ids, unsigned n) { (void)o; (void)b; (void)bs; (void)ids; (void)n; return RES_FAIL; }
SSF("_ZN2nn3ssl10Connection6CreateEPNS0_7ContextE", h_ssl_CnCreate, void *self, void *ctx) { (void)self; (void)ctx; return RES_FAIL; }
SSF("_ZN2nn3ssl10Connection7DestroyEv", h_ssl_CnDestroy, void *self) { (void)self; return RES_FAIL; }
SSF("_ZN2nn3ssl10Connection11DoHandshakeEv", h_ssl_CnHandshake, void *self) { (void)self; return RES_FAIL; }
SSF("_ZN2nn3ssl10Connection11SetHostNameEPKcj", h_ssl_CnHost, void *self, const char *h, unsigned n) { (void)self; (void)h; (void)n; return RES_FAIL; }
SSF("_ZN2nn3ssl10Connection9SetIoModeENS1_6IoModeE", h_ssl_CnIoMode, void *self, int m) { (void)self; (void)m; return RES_FAIL; }
SSF("_ZN2nn3ssl10Connection19SetSessionCacheModeENS1_16SessionCacheModeE", h_ssl_CnCache, void *self, int m) { (void)self; (void)m; return RES_FAIL; }
SSF("_ZN2nn3ssl10Connection19SetSocketDescriptorEi", h_ssl_CnSock, void *self, int s) { (void)self; (void)s; return RES_FAIL; }
SSF("_ZN2nn3ssl10Connection12GetLastErrorEPNS_6ResultE", h_ssl_CnLastErr, void *self, uint32_t *r) { (void)self; if (r) *r = RES_FAIL; return RES_FAIL; }
SSF("_ZN2nn3ssl10Connection18GetVerifyCertErrorEPNS_6ResultE", h_ssl_CnVerifyErr, void *self, uint32_t *r) { (void)self; if (r) *r = RES_FAIL; return RES_FAIL; }
SSF("_ZN2nn3ssl10Connection4PeekEPcPij", h_ssl_CnPeek, void *self, char *b, int *o, unsigned n) { (void)self; (void)b; (void)n; if (o) *o = 0; return RES_FAIL; }
SSF("_ZN2nn3ssl10Connection4PollEPNS1_9PollEventES3_j", h_ssl_CnPoll, void *self, void *o, void *i, unsigned t) { (void)self; (void)o; (void)i; (void)t; return RES_FAIL; }
HLE_FORCE("_ZN2nn3ssl10Connection4ReadEPcj", "ssl service (cut): read returns -1", int, h_ssl_CnRead, void *self, char *b, unsigned n) { (void)self; (void)b; (void)n; return -1; }
HLE_FORCE("_ZN2nn3ssl10Connection5WriteEPKcj", "ssl service (cut): write returns -1", int, h_ssl_CnWrite, void *self, const char *b, unsigned n) { (void)self; (void)b; (void)n; return -1; }
// C interface: all return nnResult
#define SSC(m, cname) HLE_FORCE(m, "ssl service (cut): fails", Result, cname, void) { return RES_FAIL; }
SSC("nnsslConnectionCreate", h_nssl_Create) SSC("nnsslConnectionDestroy", h_nssl_Destroy)
SSC("nnsslConnectionDoHandshakeWithCertBuffer", h_nssl_Handshake) SSC("nnsslConnectionGetConnectionId", h_nssl_GetId)
SSC("nnsslConnectionGetNextAlpnProto", h_nssl_GetAlpn) SSC("nnsslConnectionGetVerifyCertError", h_nssl_VerifyErr)
SSC("nnsslConnectionPeek", h_nssl_Peek) SSC("nnsslConnectionPending", h_nssl_Pending) SSC("nnsslConnectionRead", h_nssl_Read)
SSC("nnsslConnectionSetHostName", h_nssl_SetHost) SSC("nnsslConnectionSetIoMode", h_nssl_SetIo)
SSC("nnsslConnectionSetNextAlpnProto", h_nssl_SetAlpn) SSC("nnsslConnectionSetOption", h_nssl_SetOpt)
SSC("nnsslConnectionSetSessionCacheMode", h_nssl_SetCache) SSC("nnsslConnectionSetSocketDescriptor", h_nssl_SetSock)
SSC("nnsslConnectionSetVerifyOption", h_nssl_SetVerify) SSC("nnsslConnectionWrite", h_nssl_Write)
SSC("nnsslContextDestroy", h_nssl_CtxDestroy) SSC("nnsslContextGetContextId", h_nssl_CtxGetId)
