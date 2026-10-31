// ACNH optional secondary minimap presentation.
// The present: the frame drawn into the drawable, cropped (nvn_mtl.m present_pipeline)
#include <metal_stdlib>
using namespace metal;
struct V { float4 p [[position]]; float2 uv; };
vertex V vs(uint id [[vertex_id]]) { float2 uv = float2((id << 1) & 2, id & 2); V o; o.p = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1); o.uv = uv; return o; }
struct Crop { float4 rect; float4 edge; };
fragment float4 fsmap(V in [[stage_in]], texture2d<float> t [[texture(0)]], sampler s [[sampler(0)]], constant Crop& c [[buffer(0)]]) { float2 p[4]={float2(1500.0/1920,750.0/1080),float2(1880.0/1920,750.0/1080),float2(1500.0/1920,1045.0/1080),float2(1880.0/1920,1045.0/1080)}; bool show=true; for(int i=0;i<4;i++){float3 v=t.sample(s,p[i]).rgb;show=show&&min(v.r,min(v.g,v.b))>.78&&max(v.r,max(v.g,v.b))-min(v.r,min(v.g,v.b))<.16;} return show?t.sample(s,clamp(in.uv*c.rect.zw+c.rect.xy,c.edge.xy,c.edge.zw)):float4(0,0,0,1); }
