// Checked guest access for companion modules. Kernel reads tolerate concurrent unmapping.
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
typedef struct {
    uint64_t address;uint32_t size;uint8_t expect[64],value[64],before[64];
} companion_store;
typedef struct { companion_store ops[32];uint32_t count; } companion_batch;
static int heap_writable(uint64_t at,uint32_t size) {
    dm_companion_guest g;if(!dm_companion_guest_info(&g) || at<g.heap_begin || at>=g.heap_end || size>g.heap_end-at)return 0;
    vm_address_t region=at;vm_size_t length=0;mach_port_t object=MACH_PORT_NULL;
    vm_region_basic_info_data_64_t info;mach_msg_type_number_t n=VM_REGION_BASIC_INFO_COUNT_64;
    kern_return_t r=vm_region_64(mach_task_self(),&region,&length,VM_REGION_BASIC_INFO_64,(vm_region_info_t)&info,&n,&object);
    if(object)mach_port_deallocate(mach_task_self(),object);
    return r==KERN_SUCCESS && at>=region && at-region<length && size<=length-(at-region) && (info.protection&(VM_PROT_READ|VM_PROT_WRITE))==(VM_PROT_READ|VM_PROT_WRITE);
}
static int apply_batch(void *context) {
    companion_batch *b=context;
    for(uint32_t i=0;i<b->count;i++){
        companion_store *p=&b->ops[i];vm_size_t got=0;
        if(!heap_writable(p->address,p->size) || vm_read_overwrite(mach_task_self(),p->address,p->size,(vm_address_t)p->before,&got)!=KERN_SUCCESS || got!=p->size || memcmp(p->before,p->expect,p->size))return 0;
    }
    // All guest VM mutation is stopped and every byte was checked writable above.
    // Use CPU stores, as guest code does, rather than a VM map-copy operation.
    // There are no fallible operations after the first store.
    for(uint32_t i=0;i<b->count;i++){
        companion_store *p=&b->ops[i];
        volatile uint8_t *out=(volatile uint8_t *)(uintptr_t)p->address;
        for(uint32_t j=0;j<p->size;j++)out[j]=p->value[j];
    }
    return 1;
}
int dm_companion_guest_write_batch(const dm_companion_write_op *ops,uint32_t count) {
    if(!ops || !count || count>32)return 0;
    companion_batch batch={.count=count};
    for(uint32_t i=0;i<count;i++){
        const dm_companion_write_op *p=&ops[i];
        if(!p->size || p->size>64 || !p->expect || !p->value || p->address>UINT64_MAX-p->size || !heap_writable(p->address,p->size))return 0;
        for(uint32_t j=0;j<i;j++)if(p->address<batch.ops[j].address+batch.ops[j].size && batch.ops[j].address<p->address+p->size)return 0;
        batch.ops[i].address=p->address;batch.ops[i].size=p->size;
        memcpy(batch.ops[i].expect,p->expect,p->size);memcpy(batch.ops[i].value,p->value,p->size);
    }
    return dm_guest_transaction(apply_batch,&batch);
}
void dm_hid_companion_set(uint64_t buttons);
void dm_hid_companion_reset(void);
void dm_companion_input(uint64_t buttons) { dm_hid_companion_set(buttons); }
void dm_companion_input_reset(void) { dm_hid_companion_reset(); }
