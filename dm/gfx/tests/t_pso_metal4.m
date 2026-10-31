#include "../nvn/nvn_mtl.m"
#include <assert.h>
void *front_metal_layer(void) { return NULL; }
module_t *module_of(const void *p) { (void)p; return NULL; }
module_t *g_mod; int g_nmod;
void svc_thread_dump(void) {}
uint64_t now_ns(void) { return bp_now(); }
int main(void) { @autoreleasepool {
 setenv("DM_PSO_MTL4","1",1);
 NSString *dir=[NSTemporaryDirectory() stringByAppendingPathComponent:NSUUID.UUID.UUIDString];
 setenv("DM_PSO_ARCHIVE",dir.fileSystemRepresentation,1);
 mtl_init();
 NSError *error=nil;
 NSString *source=@"#include <metal_stdlib>\nusing namespace metal; vertex float4 v(uint i [[vertex_id]]) { return float4(i==1?1.0:-1.0,i==2?1.0:-1.0,0,1); } fragment float4 f(){return float4(1,0,0,1);}";
 id<MTLLibrary> library=[g_dev newLibraryWithSource:source options:nil error:&error];assert(library);
 MTLRenderPipelineDescriptor *d=[MTLRenderPipelineDescriptor new];
 d.vertexFunction=fn4_note([library newFunctionWithName:@"v"],library,@"v",nil);
 d.fragmentFunction=fn4_note([library newFunctionWithName:@"f"],library,@"f",nil);
 d.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;
 id<MTLRenderPipelineState> a=pso_build(d,&error);assert(a && g_comp4 && g_pb_built==1);
 d.colorAttachments[0].blendingEnabled=YES;
 d.colorAttachments[0].sourceRGBBlendFactor=MTLBlendFactorSourceAlpha;
 d.colorAttachments[0].destinationRGBBlendFactor=MTLBlendFactorOneMinusSourceAlpha;
 id<MTLRenderPipelineState> b=pso_build(d,&error);assert(b && g_pb_reused>=1 && g_pb_spec>=2);
 dispatch_sync(g_arch_q, ^{ arch_flush(); });
 assert(g_arch_flush_ok);
 NSArray *files=[NSFileManager.defaultManager contentsOfDirectoryAtPath:dir error:nil];assert(files.count>0);
 ga_exit();[NSFileManager.defaultManager removeItemAtPath:dir error:nil];
 puts("Metal 4 real-device test: base compile, specialization, reuse and archive flush passed");
 }return 0;
}
