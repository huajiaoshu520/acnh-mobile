// Shader hashing needs to know whether a guest range is mapped without making its pool resident.
#include "../nvn/nvn_mtl.m"

void *front_metal_layer(void) { return NULL; }
module_t *module_of(const void *p) { (void)p; return NULL; }
module_t *g_mod; int g_nmod;
void svc_thread_dump(void) {}
uint64_t now_ns(void) { return bp_now(); }

static int checks, failed;
#define CHECK(label, condition) do { \
    checks++; \
    if (!(condition)) { \
        failed++; \
        fprintf(stderr, "FAIL: %s (line %d)\n", label, __LINE__); \
    } \
} while (0)

int main(void) {
    @autoreleasepool {
        mtl_init();
        const size_t size = 4u << 20;
        uint8_t *mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
        if (mem == MAP_FAILED) abort();
        const uint64_t base = (uint64_t)(uintptr_t)mem;
        mtl_pool_add(mem, size);

        const int initial_residency = g_pool_res_n;
        CHECK("a range inside the pool is known", mtl_addr_known(base + 4096, 2048));
        CHECK("the pool's last bytes are known", mtl_addr_known(base + size - 16, 16));
        CHECK("a range past the pool's end is unknown", !mtl_addr_known(base + size - 16, 32));
        CHECK("an address outside every pool is unknown", !mtl_addr_known(base + size + 4096, 16));
        CHECK("an empty range is unknown", !mtl_addr_known(base, 0));
        CHECK("a range whose end overflows is unknown", !mtl_addr_known(base + 4096, UINT64_MAX - base));
        CHECK("checking the mapping makes no pool resident", g_pool_res_n == initial_residency);

        void *buffer = NULL;
        uint64_t offset = 0;
        CHECK("mtl_addr finds the same range", mtl_addr(base + 4096, 2048, &buffer, &offset) && offset == 4096);
        CHECK("mtl_addr makes the pool resident", g_pool_res_n == initial_residency + 1);

        mtl_pool_del(mem);
        CHECK("a removed pool holds nothing", !mtl_addr_known(base + 4096, 2048));
        printf("t_addr_known: %d checks, %d failed\n", checks, failed);
        return failed != 0;
    }
}
