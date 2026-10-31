// Exercise production residency and pool retirement while the caller's pool stays alive.
#include "../nvn/nvn_mtl.m"
void *front_metal_layer(void) { return NULL; }
module_t *module_of(const void *p) { (void)p; return NULL; }
module_t *g_mod; int g_nmod;
void svc_thread_dump(void) {}
uint64_t now_ns(void) { return bp_now(); }
int main(void) { @autoreleasepool {
    mtl_init();
    int failed = 0;
    // A guest thread has one long-lived outer pool. Neither membership lookup may
    // extend a retired buffer's lifetime until that pool eventually drains.
    for (int i = 0; i < 16; i++) {
        id<MTLBuffer> b = [g_dev newBufferWithLength:4096 options:MTLResourceStorageModeShared];
        __weak id<MTLBuffer> weak = b;
        res_add(b); res_add(b); res_remove(b); b = nil;
        if (weak != nil) { failed++; printf("buffer still alive, iteration %d\n", i); }
    }
    void *mem = NULL; if (posix_memalign(&mem, 16384, 16384)) return 2;
    for (int i = 0; i < 16; i++) {
        mtl_pool_add(mem, 16384);
        id<MTLBuffer> b = g_pools[g_npool - 1].buf;
        __weak id<MTLBuffer> weak = b;
        mtl_pool_del(mem); b = nil;
        // No GPU work was submitted: advance the completed fence to retire the
        // deliberately deferred strong reference, as a completion would do.
        g_done = g_fence + 1; defer_drain();
        if (weak != nil) { failed++; printf("buffer still alive, iteration %d\n", i); }
    }
    failed += g_dead_pools.allObjects.count != 0;
    free(mem);
    printf("pool lifetime: 33 checks, %d failed (outer pool still alive)\n", failed);
    return failed != 0;
} }
