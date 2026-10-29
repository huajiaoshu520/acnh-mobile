// Read-only result tracing for the user's exact ACNH 3.0.3 build. The rewriter
// validates both build ID and original instructions and replays each original
// instruction after the callback. No result, state, or register is altered.
#include "hle.h"
enum { OBS_ACNH_LAN_RESULT = 29 };
#include "lan_room.h"
#include <stdatomic.h>
static pthread_mutex_t result_lock=PTHREAD_MUTEX_INITIALIZER;
static struct {uint32_t value;uintptr_t caller;int kind;uint64_t hits;} results[64];
static int result_count;
static void lan_result(uint64_t *regs,int kind){
    uintptr_t base=0;
    for(int i=0;i<g_nmod;i++)if(!strcmp(g_mod[i].name,"main")&&!strcmp(g_mod[i].build_id,"ff1d1c05670db6021c85b624a710b963")){base=(uintptr_t)g_mod[i].base;break;}
    if(!base)return;
    // entry.S allocates 800 bytes, after site_obs has saved original x29/x30
    // in 16 bytes. regs[30] is the gate-block LR; the game's LR is at +808.
    uintptr_t caller;memcpy(&caller,(const uint8_t*)regs+808,sizeof caller);
    if(caller<base+0x11f000 || caller>=base+0x210000 || !regs[1])return;
    if(dm_room_state()==0)return;
    uint32_t value;memcpy(&value,(const void*)(uintptr_t)regs[1],sizeof value);
    if(value==0 || value==0x2c09)return; // successful result / ordinary empty receive
    uintptr_t off=caller-base;uint64_t n=1;
    pthread_mutex_lock(&result_lock);int index=-1;
    for(int i=0;i<result_count;i++)if(results[i].value==value&&results[i].caller==off&&results[i].kind==kind){index=i;break;}
    if(index<0 && result_count<64){index=result_count++;results[index].value=value;results[index].caller=off;results[index].kind=kind;}
    if(index>=0)n=++results[index].hits;
    pthread_mutex_unlock(&result_lock);
    if(n<=2 || !(n&(n-1)))LOG("[ACNH LAN GAME RESULT] kind=%d caller=main+%#llx raw=%#x occurrence=%llu",kind,(unsigned long long)off,value,(unsigned long long)n);
}
__attribute__((constructor)) static void lan_result_register(void){gate_obs_set(OBS_ACNH_LAN_RESULT,lan_result);}
