// The bridge reads only published guest ranges and tolerates concurrent unmapping.
#include "../hle_companion.c"
#include <sys/mman.h>
#include <assert.h>
#include <unistd.h>
static module_t modules[1];
module_t *g_mod = modules;
int g_nmod;
int dm_guest_transaction(int (*body)(void *),void *context) { return body(context); }
static uint64_t heap_begin, heap_end;
void dm_guest_heap_range(uint64_t *begin,size_t *length) { *begin=heap_begin;*length=heap_end-heap_begin; }
void dm_hid_companion_set(uint64_t buttons) {(void)buttons;}
void dm_hid_companion_reset(void) {}
int main(void) {
    dm_companion_guest g;
    assert(!dm_companion_guest_info(&g));
    size_t size=(size_t)getpagesize();
    void *p=mmap(NULL,size,PROT_READ|PROT_WRITE,MAP_ANON|MAP_PRIVATE,-1,0);assert(p!=MAP_FAILED);
    strcpy(g_mod[0].name,"main");strcpy(g_mod[0].build_id,"ff1d1c05670db6021c85b624a710b963");
    g_mod[0].base=p;g_mod[0].size=size;g_nmod=1;
    heap_begin=(uintptr_t)p;heap_end=heap_begin+size;
    assert(!dm_companion_guest_read((uintptr_t)p,&g,1));
    dm_companion_host_ready();assert(dm_companion_guest_info(&g));
    assert(g.title_id==DM_GAME_PROGRAM_ID && !strcmp(g.build_id,g_mod[0].build_id));
    assert(g.main_base==(uintptr_t)p && g.main_size==size);
    memcpy(p,"guest data",11);char buf[11]={0};
    assert(dm_companion_guest_read((uintptr_t)p,buf,sizeof buf) && !strcmp(buf,"guest data"));
    assert(!dm_companion_guest_mapped(UINT64_MAX-1,8));
    assert(!dm_companion_guest_mapped((uintptr_t)p+size,1));
    assert(!dm_companion_guest_mapped((uintptr_t)p+size-1,2));
    assert(!dm_companion_guest_mapped(0,1));assert(!dm_companion_guest_mapped((uintptr_t)p,0));
    assert(!dm_companion_guest_read((uintptr_t)p,NULL,1));
    const char expected[]="guest", replacement[]="Guest";
    dm_companion_write_op ops[2]={{(uintptr_t)p,5,expected,replacement},{(uintptr_t)p+6,4,"data","save"}};
    assert(dm_companion_guest_write_batch(ops,2));assert(!memcmp(p,"Guest save",10));
    assert(!dm_companion_guest_write_batch(ops,2));assert(!memcmp(p,"Guest save",10));
    ops[0]=(dm_companion_write_op){(uintptr_t)p,5,replacement,expected};ops[1].expect="bad!";
    assert(!dm_companion_guest_write_batch(ops,2));assert(!memcmp(p,"Guest save",10));
    assert(!dm_companion_guest_write_batch(ops,33));
    ops[1]=ops[0];assert(!dm_companion_guest_write_batch(ops,2));
    assert(!mprotect(p,size,PROT_READ));assert(!dm_companion_guest_write_batch(ops,1));
    assert(!munmap(p,size));
    assert(!dm_companion_guest_read((uintptr_t)p,buf,1)); // Range metadata can race a guest decommit.
    puts("companion checked writes passed");return 0;
}
