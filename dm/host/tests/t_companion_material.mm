// Owned pixels compare CPU layout material evaluation with an sRGB Metal target.
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <cassert>
#include <cstdio>
#include <cmath>
#include "../../third_party/eden_duo/acnh/acnh_draw.h"
using namespace acnh;
int main(){@autoreleasepool{
    const float unit[4]={1,1,1,1};
    Image ramp{256,1};for(int x=0;x<256;x++)for(int c=0;c<4;c++)ramp.At(x,0)[c]=x;
    auto identity=draw::TintLinear(ramp,{0,0,0,0},{255,255,255,255},true,unit);
    assert(identity.rgba==ramp.rgba);
    auto linear=draw::TintLinear(ramp,{0,0,0,0},{255,255,255,255},false,unit);
    assert(linear.At(128,0)[0]==188 && linear.At(128,0)[3]==128);
    Image texture{4,4},mask{4,4};
    for(int y=0;y<4;y++)for(int x=0;x<4;x++){
        const uint8_t pixel[4]={231,27,77,255};memcpy(texture.At(x,y),pixel,4);
        mask.At(x,y)[3]=x==0?0:x==1?128:255;
    }
    const double shift[6]={1,0,0,0,1,-2};auto globe=draw::TextureLayer(texture,mask,shift);
    for(int y=0;y<4;y++)for(int x=0;x<4;x++)assert(globe.At(x,y)[3]==(y<2?0:mask.At(x,y)[3]));
    assert(globe.At(2,2)[0]==231 && globe.At(2,2)[1]==27);
    texture.At(1,0)[3]=128;globe=draw::TextureLayer(texture,mask,shift);assert(globe.At(1,2)[3]==64);
    puts("companion material: identity, linear alpha, transformed colour stage and alpha product PASS");
    id<MTLDevice> device=MTLCreateSystemDefaultDevice();
    if(!device){puts("companion material: Metal reference SKIP (no device)");return 0;}
    NSString *source=@"#include <metal_stdlib>\nusing namespace metal;\nstruct Material{float4 black,white,colour;};\nvertex float4 vertexMain(uint i [[vertex_id]]){const float2 p[3]={float2(-1,-1),float2(3,-1),float2(-1,3)};return float4(p[i],0,1);}\nfragment float4 fragmentMain(float4 p [[position]],texture2d<float,access::read> tex [[texture(0)]],constant Material &m [[buffer(0)]]){return (m.black+(m.white-m.black)*tex.read(uint2(p.xy)))*m.colour;}";
    NSError *error=nil;id<MTLLibrary> library=[device newLibraryWithSource:source options:nil error:&error];if(!library)fprintf(stderr,"%s\n",error.localizedDescription.UTF8String);assert(library);
    MTLRenderPipelineDescriptor *descriptor=[MTLRenderPipelineDescriptor new];descriptor.vertexFunction=[library newFunctionWithName:@"vertexMain"];descriptor.fragmentFunction=[library newFunctionWithName:@"fragmentMain"];descriptor.colorAttachments[0].pixelFormat=MTLPixelFormatRGBA8Unorm_sRGB;
    id<MTLRenderPipelineState> pipeline=[device newRenderPipelineStateWithDescriptor:descriptor error:&error];assert(pipeline);id<MTLCommandQueue> queue=[device newCommandQueue];
    Image input{16,1};for(int x=0;x<16;x++){auto p=input.At(x,0);p[0]=x*17;p[1]=(x*73)%256;p[2]=255-x*17;p[3]=(x*31)%256;}
    const Rgba black[]={{0,0,0,0},{17,10,7,0},{205,198,169,0},{255,60,20,40}};
    const Rgba white[]={{255,255,255,255},{237,229,193,255},{55,171,127,255},{10,190,250,210}};
    unsigned comparisons=0;
    for(int encoded=0;encoded<2;encoded++)for(int k=0;k<4;k++)for(int vertex=0;vertex<2;vertex++){@autoreleasepool{
        const float v[4]={vertex?.3f:1,vertex?.6f:1,vertex?.9f:1,vertex?.4f:1};
        auto expected=draw::TintLinear(input,black[k],white[k],encoded,v);
        auto td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:encoded?MTLPixelFormatRGBA8Unorm_sRGB:MTLPixelFormatRGBA8Unorm width:16 height:1 mipmapped:NO];td.storageMode=MTLStorageModeShared;td.usage=MTLTextureUsageShaderRead;id<MTLTexture> tex=[device newTextureWithDescriptor:td];assert(tex);[tex replaceRegion:MTLRegionMake2D(0,0,16,1) mipmapLevel:0 withBytes:input.rgba.data() bytesPerRow:64];
        td.pixelFormat=MTLPixelFormatRGBA8Unorm_sRGB;td.usage=MTLTextureUsageRenderTarget;id<MTLTexture> target=[device newTextureWithDescriptor:td];assert(target);
        struct{float b[4],w[4],v[4];} material;
        const uint8_t *b=reinterpret_cast<const uint8_t *>(&black[k]),*w=reinterpret_cast<const uint8_t *>(&white[k]);for(int c=0;c<4;c++){material.b[c]=b[c]/255.0f;material.w[c]=w[c]/255.0f;material.v[c]=v[c];}
        MTLRenderPassDescriptor *pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.colorAttachments[0].texture=target;pass.colorAttachments[0].loadAction=MTLLoadActionClear;pass.colorAttachments[0].storeAction=MTLStoreActionStore;
        id<MTLCommandBuffer> cb=[queue commandBuffer];id<MTLRenderCommandEncoder> encoder=[cb renderCommandEncoderWithDescriptor:pass];[encoder setRenderPipelineState:pipeline];[encoder setFragmentTexture:tex atIndex:0];[encoder setFragmentBytes:&material length:sizeof material atIndex:0];[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[encoder endEncoding];[cb commit];[cb waitUntilCompleted];assert(cb.status==MTLCommandBufferStatusCompleted);
        uint8_t actual[64];[target getBytes:actual bytesPerRow:64 fromRegion:MTLRegionMake2D(0,0,16,1) mipmapLevel:0];
        for(size_t p=0;p<64;p++){if(std::abs(int(actual[p])-int(expected.rgba[p]))>1){fprintf(stderr,"material mismatch encoded=%d case=%d vertex=%d byte=%zu gpu=%u cpu=%u\n",encoded,k,vertex,p,actual[p],expected.rgba[p]);abort();}comparisons++;}
    }}
    printf("companion material: %u Metal colour/alpha comparisons PASS\n",comparisons);return 0;
}}
