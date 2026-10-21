#import <Metal/Metal.h>
#include <assert.h>
#include <stdio.h>
#include "nvn_bcdec.h"
int main(void){@autoreleasepool{
 id<MTLDevice>d=MTLCreateSystemDefaultDevice();assert(d.supportsBCTextureCompression);uint8_t blocks[4096],decoded[16384];uint32_t rng=137;
 for(int i=0;i<4096;i++){rng=rng*1664525+1013904223;blocks[i]=rng>>24;}
 for(int i=0;i<256;i++){int m=i%8;blocks[16*i]=(blocks[16*i] & (0xffu<<(m+1))) | (1u<<m);}
 assert(!nvn_bc_decode(0x4d,blocks,256,64,64,decoded,256));
 MTLTextureDescriptor*td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBC7_RGBAUnorm width:64 height:64 mipmapped:NO];td.storageMode=MTLStorageModeShared;id<MTLTexture>a=[d newTextureWithDescriptor:td];assert(a);[a replaceRegion:MTLRegionMake2D(0,0,64,64) mipmapLevel:0 withBytes:blocks bytesPerRow:256];
 td.pixelFormat=MTLPixelFormatRGBA8Unorm;id<MTLTexture>b=[d newTextureWithDescriptor:td];[b replaceRegion:MTLRegionMake2D(0,0,64,64) mipmapLevel:0 withBytes:decoded bytesPerRow:256];
 NSError*e=nil;id<MTLLibrary>lib=[d newLibraryWithSource:@"#include <metal_stdlib>\nusing namespace metal; kernel void k(texture2d<float,access::read>a[[texture(0)]],texture2d<float,access::read>b[[texture(1)]],device float4*o[[buffer(0)]],uint2 p[[thread_position_in_grid]]){o[p.y*64+p.x]=abs(a.read(p)-b.read(p));}" options:nil error:&e];assert(lib);id<MTLComputePipelineState>ps=[d newComputePipelineStateWithFunction:[lib newFunctionWithName:@"k"] error:&e];assert(ps);id<MTLBuffer>out=[d newBufferWithLength:65536 options:MTLResourceStorageModeShared];id<MTLCommandQueue>q=[d newCommandQueue];id<MTLCommandBuffer>cb=[q commandBuffer];id<MTLComputeCommandEncoder>ce=[cb computeCommandEncoder];[ce setComputePipelineState:ps];[ce setTexture:a atIndex:0];[ce setTexture:b atIndex:1];[ce setBuffer:out offset:0 atIndex:0];[ce dispatchThreads:MTLSizeMake(64,64,1) threadsPerThreadgroup:MTLSizeMake(8,8,1)];[ce endEncoding];[cb commit];[cb waitUntilCompleted];assert(!cb.error);float max=0;for(int i=0;i<16384;i++){float x=((float*)out.contents)[i];if(x>max)max=x;}printf("BC7 all 8 modes CPU/native GPU max error = %.5f\n",max);assert(max<1.1f/255);return 0;
}}
