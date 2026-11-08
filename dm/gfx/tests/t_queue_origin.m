// NVN's queue initialization captures WINDOW_ORIGIN.FLIP_Y. The device origin
// used to construct later viewports can change independently. Check actual
// culling and front_facing pixels while alternating two queues on one device.
// Also verify that render enable follows its own queue across submissions.
#define nvn_game_fps nvn_game_fps_mtl_default
#include "../nvn/nvn_mtl.m"
#undef nvn_game_fps
#define g_ft g_ft_hle
#include "../nvn/hle_nvn.c"
#undef g_ft
#define P(name) ((NVNT_##name)nvn_lookup(#name))
void *front_metal_layer(void) { return NULL; }
module_t *module_of(const void *p) { (void)p; return NULL; }
module_t *g_mod; int g_nmod;
void svc_thread_dump(void) {}
uint64_t now_ns(void) { return bp_now(); }
static int checks, fails;
#define CHECK(c, label) do { checks++; if (!(c)) { fails++; fprintf(stderr,"FAIL: %s\n",label); } } while (0)
static uint8_t commands[8192]; static size_t used;
static void emit(unsigned op, const void *p, size_t n) {
    size_t size = (8 + n + 7) & ~7ull; op_hdr h = {op, (uint32_t)size};
    memcpy(commands + used, &h, 8); if (n) memcpy(commands + used + 8, p, n); used += size;
}
static void clear_on_queue(NVNqueue *queue, const tex_t *target, int render_mode, int channel) {
    used = 0;
    p_set_rt rt = {.n = 1, .color = {target}};
    emit(OP_SET_RT, &rt, sizeof rt);
    if (render_mode >= 0) {
        p_render_cond condition = {.mode = render_mode};
        emit(OP_RENDER_COND, &condition, sizeof condition);
    }
    p_clear_color clear = {.rgba = {0, 0, 0, 1}, .mask = 15};
    clear.rgba[channel] = 1;
    emit(OP_CLEAR_COLOR, &clear, sizeof clear);
    emit(OP_END, NULL, 0);
    NVNcommandHandle handle = (NVNcommandHandle)commands;
    P(nvnQueueSubmitCommands)(queue, 1, &handle);
    P(nvnQueueFinish)(queue);
}
static int clear_channel_matches(const tex_t *target, int channel) {
    id<MTLBuffer> pixels = [g_dev newBufferWithLength:2048 options:MTLResourceStorageModeShared];
    id<MTLCommandBuffer> cb = q_cb();
    id<MTLBlitCommandEncoder> blit = [cb blitCommandEncoder];
    [blit copyFromTexture:tex_mtl(target) sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0) sourceSize:MTLSizeMake(8, 8, 1)
                 toBuffer:pixels destinationOffset:0 destinationBytesPerRow:256 destinationBytesPerImage:2048];
    [blit endEncoding];
    [cb commit];
    [cb waitUntilCompleted];
    uint8_t expected[4] = {0, 0, 0, 255};
    expected[channel] = 255;
    if (cb.status != MTLCommandBufferStatusCompleted) return 0;
    for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) {
        if (memcmp((const uint8_t *)pixels.contents + 256*y + 4*x, expected, sizeof expected)) return 0;
    }
    return 1;
}
int main(void) { @autoreleasepool {
    mtl_init(); NSError *error = nil;
    id<MTLLibrary> library = [g_dev newLibraryWithSource:@"#include <metal_stdlib>\nusing namespace metal;\n"
        "struct I { float4 p [[attribute(0)]]; };\n"
        "vertex float4 v(I i [[stage_in]]) { return i.p; }\n"
        "fragment float4 f(bool front [[front_facing]]) { return front ? float4(0,1,0,1) : float4(1,0,0,1); }\n"
        options:nil error:&error];
    CHECK(library != nil,"test shaders compile"); if (!library) return 1;
    static prog_t pg = {.serial=920001}; NvProg *program = [NvProg new];
    NvShader *vs = [NvShader new]; vs->fn = [library newFunctionWithName:@"v"];
    vs->rf.nv = 1; vs->rf.v[0].ct = NVN_CT_FLOAT;
    memset(vs->rf.v[0].sh,0xff,sizeof vs->rf.v[0].sh);
    vs->rf.pos_attr = 0; vs->rf.pos_w1 = 1; program->sh[0] = vs;
    NvShader *fs = [NvShader new]; fs->fn = [library newFunctionWithName:@"f"];
    fs->stage = 1; fs->rf.color_mask = 1; program->sh[1] = fs; serial_put(pg.serial,program);
    tex_t target = {.magic=MAGIC('T','E','X',0),.serial=920002};
    target.d = (nvn_texdesc){.target=NVN_TEXTURE_TARGET_2D,.format=NVN_FORMAT_RGBA8,.width=8,.height=8,.depth=1,.levels=1,.samples=1};
    void *vertices = NULL; CHECK(!posix_memalign(&vertices,getpagesize(),65536),"vertex storage allocated");
    if (!vertices) return 1; memset(vertices,0,65536); mtl_pool_add(vertices,65536);
    static NVNdevice device; static NVNqueue queues[2]; NVNdeviceBuilder db = {0};
    P(nvnDeviceBuilderSetDefaults)(&db); CHECK(P(nvnDeviceInitialize)(&device,&db),"device initialized");
    for (int origin=0;origin<2;origin++) {
        P(nvnDeviceSetWindowOriginMode)(&device,(NVNwindowOriginMode)origin);
        NVNqueueBuilder qb = {0}; P(nvnQueueBuilderSetDefaults)(&qb); P(nvnQueueBuilderSetDevice)(&qb,&device);
        CHECK(P(nvnQueueInitialize)(&queues[origin],&qb),"queue initialized");
    }
    uint64_t first_serial = ((queue_t *)&queues[0])->serial;
    mtl_queue_activate(first_serial); S.tex_base = 123; R.has_sc = 1; R.sc.x = 77;
    mtl_queue_activate(((queue_t *)&queues[1])->serial);
    CHECK(S.tex_base == 0 && !R.has_sc && !S.bst[0],"a new queue starts with independent default state");
    S.tex_base = 456;
    mtl_queue_activate(first_serial);
    CHECK(S.tex_base == 123 && R.has_sc && R.sc.x == 77,"switching back restores the first queue state");
    mtl_queue_activate(((queue_t *)&queues[1])->serial);
    CHECK(S.tex_base == 456 && !R.has_sc,"the second queue keeps its own bindings");
    mtl_queue_activate(first_serial); S.bst[0] = S.prog = (const prog_t *)(uintptr_t)0xdead0000;
    P(nvnQueueFinalize)(&queues[0]);
    P(nvnDeviceSetWindowOriginMode)(&device,(NVNwindowOriginMode)0);
    NVNqueueBuilder fresh = {0}; P(nvnQueueBuilderSetDefaults)(&fresh); P(nvnQueueBuilderSetDevice)(&fresh,&device);
    CHECK(P(nvnQueueInitialize)(&queues[0],&fresh),"queue storage can be initialized again");
    mtl_queue_activate(((queue_t *)&queues[0])->serial);
    CHECK(((queue_t *)&queues[0])->serial != first_serial && !S.bst[0] && !S.prog && !R.has_sc,"reinitializing a queue never inherits finalized program pointers");
    // Native GPU state: matching queue/device origins keep NVN's NDC facing;
    // differing origins reverse it. CW and CCW expectations are explicit here.
    const int front_in_clip[2][2][2] = {{{0,1},{1,0}},{{1,0},{0,1}}};
    for (int current=0;current<2;current++) for (int winding=0;winding<2;winding++)
    for (int cull=0;cull<4;cull++) for (int ccw=0;ccw<2;ccw++) for (int flip=0;flip<2;flip++) for (int queue=0;queue<2;queue++) {
        P(nvnDeviceSetWindowOriginMode)(&device,(NVNwindowOriginMode)current);
        float positions[3][4] = {{-1,-1,.5,1},{3,-1,.5,1},{-1,3,.5,1}};
        if (!ccw) { float tmp[4]; memcpy(tmp,positions[1],16); memcpy(positions[1],positions[2],16); memcpy(positions[2],tmp,16); }
        memcpy(vertices,positions,sizeof positions);
        state_reset(); memset(&R,0,sizeof R); used=0;
        p_set_rt rt = {.n=1}; rt.color[0]=&target; emit(OP_SET_RT,&rt,sizeof rt);
        p_clear_color clear = {.rgba={0,0,1,1},.mask=15}; emit(OP_CLEAR_COLOR,&clear,sizeof clear);
        p_bind_program bp = {.prog=&pg,.stages=3}; emit(OP_BIND_PROGRAM,&bp,sizeof bp);
        ds_st ds = {.df=NVN_DEPTH_FUNC_ALWAYS}; p_bind_state bs = {.kind=5}; memcpy(bs.bytes,&ds,sizeof ds); emit(OP_BIND_STATE,&bs,sizeof bs);
        poly_st poly = {.cull=cull,.front=winding,.mode=NVN_POLYGON_MODE_FILL}; bs.kind=4; memcpy(bs.bytes,&poly,sizeof poly); emit(OP_BIND_STATE,&bs,sizeof bs);
        struct { int32_t first,n; va_st attr; } attr = {0,1,{.fmt=NVN_FORMAT_RGBA32F}}; emit(OP_BIND_VATTR,&attr,sizeof attr);
        struct { int32_t first,n; vs_st stream; } stream = {0,1,{.stride=16}}; emit(OP_BIND_VSTREAM,&stream,sizeof stream);
        p_bind_vbuf1 vb = {.index=0,.size=sizeof positions,.addr=(uint64_t)vertices}; emit(OP_BIND_VBUF1,&vb,sizeof vb);
        p_vp_swizzle sw = {.n=1}; sw.sw[0][1]=flip; emit(OP_VP_SWIZZLE,&sw,sizeof sw);
        p_draw draw = {.prim=NVN_DRAW_PRIMITIVE_TRIANGLES,.count=3,.instances=1}; emit(OP_DRAW,&draw,sizeof draw); emit(OP_END,NULL,0);
        NVNcommandHandle handle = (NVNcommandHandle)commands;
        P(nvnQueueSubmitCommands)(&queues[queue],1,&handle); P(nvnQueueFinish)(&queues[queue]);
        int front = ccw == (front_in_clip[queue][current][winding] ^ flip);
        int visible = cull == NVN_FACE_NONE || (cull == NVN_FACE_FRONT && !front) || (cull == NVN_FACE_BACK && front);
        g_fd_dw=&draw;
        CHECK(first_draw_covers(0,tex_mtl(&target),0)==visible,"full-coverage proof agrees with native culling");
        g_fd_dw=NULL;
        uint8_t expected[4] = {visible && !front ? 255:0,visible && front ? 255:0,visible ? 0:255,255};
        id<MTLBuffer> pixels = [g_dev newBufferWithLength:256 options:MTLResourceStorageModeShared];
        mtl_sub_begin(); res_add(pixels);
        enc_b_copy_texture_buffer(blit(),tex_mtl(&target),0,0,MTLOriginMake(0,0,0),MTLSizeMake(8,8,1),pixels,0,32,256);
        CHECK(mtl_wait(mtl_sub_commit(),5000000000ull),"pixel readback completes");
        int good=1; for (int i=0;i<64;i++) good &= !memcmp((uint8_t *)pixels.contents+4*i,expected,4);
        if (!good) fprintf(stderr,"queue origin=%d device origin=%d front=%d cull=%d triangle CCW=%d\n",queue,current,winding,cull,ccw);
        CHECK(good,"queue-latched culling and front_facing match native GPU state");
    }
    // OP_RENDER_COND modes 2 and 3 are unconditional enable and disable.
    clear_on_queue(&queues[0], &target, 2, 2);
    CHECK(clear_channel_matches(&target, 2), "enabled queue clears to blue");
    clear_on_queue(&queues[0], &target, 3, 1);
    CHECK(clear_channel_matches(&target, 2), "disable skips the current clear");
    clear_on_queue(&queues[0], &target, -1, 1);
    CHECK(clear_channel_matches(&target, 2), "disable persists into the next submission on the same queue");
    clear_on_queue(&queues[1], &target, -1, 0);
    CHECK(clear_channel_matches(&target, 0), "another queue remains enabled");
    clear_on_queue(&queues[0], &target, -1, 1);
    CHECK(clear_channel_matches(&target, 0), "switching back restores the disabled queue");
    clear_on_queue(&queues[0], &target, 2, 1);
    CHECK(clear_channel_matches(&target, 1), "re-enabling the queue restores clears");
    clear_on_queue(&queues[0], &target, 3, 0);
    P(nvnQueueFinalize)(&queues[0]);
    CHECK(P(nvnQueueInitialize)(&queues[0], &fresh), "disabled queue storage can be initialized again");
    clear_on_queue(&queues[0], &target, -1, 2);
    CHECK(clear_channel_matches(&target, 2), "a recreated queue starts enabled");
    P(nvnQueueFinalize)(&queues[0]);
    P(nvnQueueFinalize)(&queues[1]);
    mtl_pool_del(vertices); free(vertices);
    printf("queue origin: %d checks, %d failed\n",checks,fails); return fails != 0;
} }
