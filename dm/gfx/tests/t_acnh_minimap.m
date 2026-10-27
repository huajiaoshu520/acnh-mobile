#include "../nvn/nvn_mtl.m"
#include <assert.h>
void *front_metal_layer(void){return NULL;}
module_t *module_of(const void *p){(void)p;return NULL;}
module_t g_mod[MAX_MOD];int g_nmod;
void svc_thread_dump(void){}
uint64_t now_ns(void){return bp_now();}
static id<MTLTexture> image(int w,int h){
 MTLTextureDescriptor *d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:w height:h mipmapped:NO];
 d.storageMode=MTLStorageModeShared;d.usage=MTLTextureUsageShaderRead|MTLTextureUsageRenderTarget;
 id<MTLTexture> t=[g_dev newTextureWithDescriptor:d];res_add(t);return t;
}
int main(void){@autoreleasepool{
 mtl_init();id<MTLTexture> src=image(1920,1080),main=image(192,108),map=image(398,310);res_commit();
 uint32_t *px=malloc(1920*1080*4);int white[4][2]={{1500,750},{1880,750},{1500,1045},{1880,1045}};
 for(int visible=0;visible<2;visible++){
  for(int y=0;y<1080;y++)for(int x=0;x<1920;x++)px[y*1920+x]=x>=1494&&x<1892&&y>=744&&y<1054?0xffff0000:0xff0000ff;
  if(visible)for(int i=0;i<4;i++)for(int y=white[i][1]-2;y<=white[i][1]+2;y++)for(int x=white[i][0]-2;x<=white[i][0]+2;x++)px[y*1920+x]=0xfffffce4;
  [src replaceRegion:MTLRegionMake2D(0,0,1920,1080) mipmapLevel:0 withBytes:px bytesPerRow:1920*4];
  [g_present_alloc4[0] reset];id<MTL4CommandBuffer> cb=g_present_cb[0];[cb beginCommandBufferWithAllocator:g_present_alloc4[0]];
  int crop[4]={1494,744,398,310};assert(present_encode_m4(cb,src,main,NULL,0));assert(present_encode_m4_(cb,src,map,crop,0,1));
  [cb endCommandBuffer];dispatch_semaphore_t done=dispatch_semaphore_create(0);__block BOOL ok=NO;
  MTL4CommitOptions *o=[MTL4CommitOptions new];[o addFeedbackHandler:^(id<MTL4CommitFeedback> f){ok=!f.error;if(f.error)puts(f.error.description.UTF8String);dispatch_semaphore_signal(done);}];
  id<MTL4CommandBuffer> cmds[]={cb};[g_q4 commit:cmds count:1 options:o];assert(!dispatch_semaphore_wait(done,dispatch_time(DISPATCH_TIME_NOW,10*NSEC_PER_SEC))&&ok);
  uint32_t a=0,b=0;[main getBytes:&a bytesPerRow:4 fromRegion:MTLRegionMake2D(96,54,1,1) mipmapLevel:0];[map getBytes:&b bytesPerRow:4 fromRegion:MTLRegionMake2D(199,155,1,1) mipmapLevel:0];
  assert((a&0xffffff)==0x0000ff);assert((b&0xffffff)==(visible?0xff0000:0));
 }
 free(px);puts("ACNH minimap GPU: independent main/mirror UV, live visible crop and absent-HUD clearing passed");
}return 0;}
