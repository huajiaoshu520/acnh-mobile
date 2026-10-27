// ACNH 3.0.3 map alpha-test replay. Included only by an opted-in game profile.
// Fragment buffer 30 is reserved by map_alpha.py in this game profile. Snapshot state in the fenced frame arena;
// never overwrite a shared constant while earlier draws can still read it. Bind before argument
// buffer reuse (and even for shaders without descriptors), including geometry replay's raster draw.
static uint64_t alpha_gpu(void) {
    static uint64_t pass, gpu; static uint32_t previous[2];
    uint32_t value[2] = {S.color.alpha_func, 0}; memcpy(&value[1], &S.alpha_ref, 4);
    if (!gpu || pass != g_pass.serial || memcmp(value, previous, sizeof value)) {
        id<MTLBuffer> b; size_t off = arena_alloc(sizeof value, &b);
        memcpy((uint8_t *)b.contents + off, value, sizeof value);
        memcpy(previous, value, sizeof value); pass = g_pass.serial; gpu = b.gpuAddress + off;
    }
    return gpu;
}
static void game_bind_fragment_patch(id<MTLCommandEncoder> en, NvShader *sh) {
    enc_r_args((id<MTL4RenderCommandEncoder>)en, MTLRenderStageFragment, alpha_gpu(), 30);
    if (passdump()) PD("A %llu key=%s func=%u ref=%g", g_stat_pass, sh->key, S.color.alpha_func, S.alpha_ref);
}
#define DM_GAME_BIND_FRAGMENT_PATCH(en, sh) game_bind_fragment_patch(en, sh)
#define DM_GAME_FRAGMENT_MAY_DISCARD (S.color.alpha_func != NVN_ALPHA_FUNC_ALWAYS)
