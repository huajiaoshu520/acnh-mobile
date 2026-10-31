// Whole-level single-sample Downsample must scale and preserve logical RGBA,
// including BGRA-to-RGBA conversion. Expected pixels do not use copy helpers.
#include "../nvn/nvn_mtl.m"
void *front_metal_layer(void){return NULL;}
module_t *module_of(const void *p){(void)p;return NULL;}
module_t *g_mod;int g_nmod;
void svc_thread_dump(void){}
uint64_t now_ns(void){return bp_now();}
int main(void){@autoreleasepool{
    mtl_init();int failed=0;
    for(int test=0;test<3;test++){
        int dw=test==1?8:4,dh=dw;
        tex_t src={.magic=MAGIC('T','E','X',0),.serial=99600+test*2};
        src.d=(nvn_texdesc){.target=NVN_TEXTURE_TARGET_2D,.format=test==1?NVN_FORMAT_BGRA8:NVN_FORMAT_RGBA8,.width=8,.height=8,.depth=1,.levels=1,.samples=1};
        tex_t dst={.magic=MAGIC('T','E','X',0),.serial=99601+test*2};
        dst.d=(nvn_texdesc){.target=NVN_TEXTURE_TARGET_2D,.format=NVN_FORMAT_RGBA8,.width=dw,.height=dh,.depth=1,.levels=1,.samples=1};
        MTLTextureDescriptor *sd=tex_desc(&src,0);sd.storageMode=MTLStorageModeShared;
        MTLTextureDescriptor *dd=tex_desc(&dst,0);dd.storageMode=MTLStorageModeShared;
        id<MTLTexture> a=[g_dev newTextureWithDescriptor:sd],b=[g_dev newTextureWithDescriptor:dd];
        if(!a||!b)return 2;
        unsigned char bytes[8*8*4];
        for(int y=0;y<8;y++)for(int x=0;x<8;x++){
            int i=(y*8+x)*4;
            bytes[i]=test==2?(x%2?255:0):test==1?192:64;
            bytes[i+1]=128;bytes[i+2]=test==1?64:192;bytes[i+3]=255;
        }
        [a replaceRegion:MTLRegionMake2D(0,0,8,8) mipmapLevel:0 withBytes:bytes bytesPerRow:32];
        res_add(a);res_add(b);mtl_tex_init(&src);mtl_tex_init(&dst);serial_put(src.serial,a);serial_put(dst.serial,b);
        state_reset();memset(&R,0,sizeof R);mtl_sub_begin();
        p_copy_t2t copy={.src=&src,.dst=&dst,.sr={0,0,0,8,8,1},.dr={0,0,0,dw,dh,1},.flags=DM_COPY_RESOLVE};
        copy_t2t(&copy);mtl_sub_commit();mtl_drain();
        int ok=!mtl_last_commit_error();
        unsigned char out[8*8*4]={0};
        [b getBytes:out bytesPerRow:dw*4 fromRegion:MTLRegionMake2D(0,0,dw,dh) mipmapLevel:0];
        for(int i=0;i<dw*dh;i++)ok &= abs((int)out[i*4]-(test==2?128:64))<=1 && out[i*4+1]==128 && out[i*4+2]==192 && out[i*4+3]==255;
        printf("single-sample Downsample case %d: %s\n",test,ok?"PASS":"FAIL");failed+=!ok;
        mtl_tex_release(&src);mtl_tex_release(&dst);
    }
    return failed!=0;
}}
