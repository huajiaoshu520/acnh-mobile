// Exercise the production graphics gate and real Metal queue, without game data.
#include "../nvn/hle_nvn.c"
#include <assert.h>
#include <time.h>
#include <unistd.h>
uint64_t g_t0_ns;
void *front_metal_layer(void){return NULL;}
module_t *module_of(const void *p){(void)p;return NULL;}
module_t *g_mod;int g_nmod;
void svc_thread_dump(void){}
uint64_t now_ns(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000000000ull+t.tv_nsec;}
static _Atomic int entered,passed;
static void *submit(void *p){(void)p;atomic_store(&entered,1);gfx_enter();atomic_store(&passed,1);gfx_leave();return NULL;}
static void launch(pthread_t *t){atomic_store(&entered,0);atomic_store(&passed,0);assert(!pthread_create(t,NULL,submit,NULL));for(int n=0;!atomic_load(&entered)&&n<1000;n++)usleep(1000);assert(atomic_load(&entered));}
int main(void){
 mtl_init();pthread_t t;
 dm_gfx_suspend_for(3,1);launch(&t);
 dm_gfx_suspend(1);dm_gfx_suspend(0); // foreground must retain picker hold
 int seconds=getenv("ACNH_AMIIBO_PAUSE_TEST_SECONDS")?atoi(getenv("ACNH_AMIIBO_PAUSE_TEST_SECONDS")):2;
 assert(seconds>0 && seconds<=180);
 for(int n=0;n<seconds*10;n++){usleep(100000);assert(!atomic_load(&passed));}
 dm_gfx_suspend_for(3,0);pthread_join(t,NULL);assert(atomic_load(&passed));
 for(int n=0;n<64;n++){
  dm_gfx_suspend(1);dm_gfx_suspend_for(3,1);launch(&t);
  dm_gfx_suspend_for(3,0);usleep(1000);assert(!atomic_load(&passed)); // cancel picker cannot release background
  dm_gfx_suspend(0);pthread_join(t,NULL);assert(atomic_load(&passed));
 }
 printf("PASS: production graphics gate stayed blocked %d seconds; 64 background/picker overlap cycles resumed\n",seconds);return 0;
}
