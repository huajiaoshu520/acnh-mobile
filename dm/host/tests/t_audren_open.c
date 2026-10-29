// A SDK may implement its event-taking audio open through the no-event form.
// The host must start once, after the outer SDK initialization has completed.
#include "../hle.h"
#undef HLE_FORCE
#define HLE_FORCE(mangled, why, ret, cname, ...) static ret cname(__VA_ARGS__)
#include "../hle_audren.c"
static int checks, fails, starts, joins, in_event, expect_event, ready, fail_open;
// These legacy cases pass no work buffer, so no host-owned backing is registered.
void *objmap_del(const void *key) { (void)key; return NULL; }
#define CHECK(c) do{checks++;if(!(c)){fails++;fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);}}while(0)
static Result sdk_open(void **h,const uint8_t *p,void *w,size_t z){
 (void)w;(void)z;CHECK(*(int32_t*)(p+P_DEVICE)==1);CHECK(*(int32_t*)(p+P_MODE)==1);
 if(fail_open)return 1;h[0]=(void*)1;h[1]=(void*)2;return 0;
}
static Result sdk_event(void **h,void *e,const uint8_t *p,void *w,size_t z){
 (void)e;in_event=1;Result r=h_ar_Open0(h,p,w,z);CHECK(!atomic_load(&g_open));in_event=0;ready=!r;return r;
}
static void sdk_close(void *o,void *s){(void)o;(void)s;CHECK(!atomic_load(&g_open));}
void *guest_sym(const char *n){
 if(strstr(n,"OpenAudioRendererEPNS0_19AudioRendererHandleEPNS_2os"))return sdk_event;
 if(strstr(n,"OpenAudioRenderer"))return sdk_open;
 if(strstr(n,"CloseAudioRenderer"))return sdk_close;
 abort();
}
void *dm_guest_thread_spawn(void (*f)(void*),void *a,int p,size_t z){(void)f;(void)a;(void)p;(void)z;starts++;CHECK(!in_event);CHECK(!expect_event||ready);return (void*)3;}
void dm_guest_thread_join(void *t){CHECK(t==(void*)3);joins++;}
void *dm_pcmout_open(int c){(void)c;return NULL;}void dm_pcmout_close(void *p){(void)p;}void dm_pcmout_start(void *p){(void)p;}
int dm_pcmout_append(void *p,void *b){(void)p;(void)b;return 0;}void *dm_pcmout_released(void *p){(void)p;return NULL;}void dm_pcmout_wait(void *p,int m){(void)p;(void)m;}
static int notification_binds;
void hle_bind_system_event(void *ev,int signaled){CHECK(ev==(void*)9);CHECK(!signaled);notification_binds++;}
int main(void){uint8_t param[P_SIZE]={0};void *h[2]={0};
 CHECK(!h_ar_Open0(h,param,NULL,0));CHECK(starts==1);CHECK(atomic_load(&g_open));h_ar_Close(h[0],h[1]);CHECK(joins==1);
 expect_event=1;ready=0;CHECK(!h_ar_Open(h,(void*)4,param,NULL,0));CHECK(starts==2);CHECK(atomic_load(&g_open));h_ar_Close(h[0],h[1]);CHECK(joins==2);
 fail_open=1;CHECK(h_ar_Open(h,(void*)4,param,NULL,0)==1);CHECK(starts==2);CHECK(!atomic_load(&g_open));
 CHECK(h_ad_Notify((void*)9)==0);CHECK(notification_binds==1);
 CHECK(h_ad_Notify((void*)9)==0x499);CHECK(notification_binds==1);
 printf("audio renderer open: %d checks, %d failed\n",checks,fails);return fails!=0;
}
