// Read-only guest access for companion modules. Kernel reads tolerate concurrent unmapping.
#include "dm.h"
#include "companion.h"
#include <stdatomic.h>
#include <mach/mach.h>
static _Atomic int companion_ready;
static dm_companion_guest companion_guest;
uint64_t dm_companion_title_id(void) { return DM_GAME_PROGRAM_ID; }
void dm_guest_heap_range(uint64_t *base,size_t *length);
void dm_companion_host_ready(void) {
    companion_guest.title_id=DM_GAME_PROGRAM_ID;
    for(int i=0;i<g_nmod;i++)if(!strcmp(g_mod[i].name,"main")){
        companion_guest.main_base=(uintptr_t)g_mod[i].base;companion_guest.main_size=g_mod[i].size;
        strlcpy(companion_guest.build_id,g_mod[i].build_id,sizeof companion_guest.build_id);break;
    }
    atomic_store_explicit(&companion_ready,1,memory_order_release);
}
int dm_companion_guest_info(dm_companion_guest *out) {
    if(!out)return 0;
    memset(out,0,sizeof *out);
    if(!atomic_load_explicit(&companion_ready,memory_order_acquire))return 0;
    *out=companion_guest;
    size_t length=0;dm_guest_heap_range(&out->heap_begin,&length);out->heap_end=out->heap_begin+length;
    return out->main_base && out->main_size;
}
int dm_companion_guest_mapped(uint64_t at,size_t n) {
    if(!at || !n || n>UINT64_MAX-at)return 0;
    dm_companion_guest g;if(!dm_companion_guest_info(&g))return 0;
    if(at>=g.heap_begin && at<=g.heap_end && n<=g.heap_end-at)return 1;
    return at>=g.main_base && at-g.main_base<=g.main_size && n<=g.main_size-(at-g.main_base);
}
int dm_companion_guest_read(uint64_t at,void *out,size_t n) {
    if(!out || !dm_companion_guest_mapped(at,n))return 0;
    vm_size_t got=0;return vm_read_overwrite(mach_task_self(),(vm_address_t)at,n,(vm_address_t)out,&got)==KERN_SUCCESS && got==n;
}
// Portrait companion is informational. No game-memory edits are exposed.
int dm_companion_guest_write_batch(const dm_companion_write_op *ops,uint32_t count) {
    (void)ops;(void)count;return 0;
}
void dm_hid_companion_set(uint64_t buttons);
void dm_hid_companion_reset(void);
void dm_companion_input(uint64_t buttons) { dm_hid_companion_set(buttons); }
void dm_companion_input_reset(void) { dm_hid_companion_reset(); }
