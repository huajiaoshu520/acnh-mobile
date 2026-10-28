// ACNH 3.0.3 expanded-view projection and UI adaptation.
// Projection layout/reference: Nycolazs/ACNHRecomp nvn_ultrawide.cpp (MIT),
// copyright 2026 ACNHRecomp contributors; license retained in the game profile.
#include "hle.h"
#include <stdatomic.h>
#include <math.h>

static _Atomic uint32_t wide_bits = 0x3fe38e39;
static _Atomic int wide_present_full;
void acnh_wide_set_present_full(int on){atomic_store_explicit(&wide_present_full,!!on,memory_order_relaxed);}
int acnh_wide_present_full(void){return atomic_load_explicit(&wide_present_full,memory_order_relaxed);}
static float fget(const void *p) { float f; memcpy(&f,p,4); return f; }
static void fput(void *p,float f) { memcpy(p,&f,4); }
void acnh_wide_set_aspect(float a) {
    if (!isfinite(a) || a < 1.f || a > 2.6f) a=16.f/9.f;
    uint32_t bits;memcpy(&bits,&a,4);atomic_store_explicit(&wide_bits,bits,memory_order_relaxed);
}
static float wanted(void) { uint32_t b=atomic_load_explicit(&wide_bits,memory_order_relaxed);return fget(&b); }
static uintptr_t main_base(void) {
    for(int i=0;i<g_nmod;i++)if(!strcmp(g_mod[i].name,"main") && !strcmp(g_mod[i].build_id,"ff1d1c05670db6021c85b624a710b963"))return (uintptr_t)g_mod[i].base;
    return 0;
}
typedef struct { uintptr_t object;float assigned[4],base[4];int valid; } wide_record;
static wide_record persp[256];
static pthread_mutex_t wide_lock=PTHREAD_MUTEX_INITIALIZER;
static wide_record *record(wide_record *table,uintptr_t object) {
    for(int i=0;i<256;i++)if(table[i].object==object)return &table[i];
    for(int i=0;i<256;i++)if(!table[i].object){table[i].object=object;return &table[i];}
    return NULL;
}
static void perspective(uint64_t *r,int param) {
    (void)param;uintptr_t base=main_base(),object=r[0];if(!base||!object||*(uintptr_t*)object!=base+0x4a65608)return;
    // site_obs pushes the original x29/x30, then dm_gate saves an 800-byte frame.
    // r[30] is the gate's own LR; the original guest LR is at r+800+8.
    uintptr_t lr=r[101]-base;if(lr==0x376d224)return; // HUD minimap: preserve its own view.
    pthread_mutex_lock(&wide_lock);wide_record *c=record(persp,object);
    if(c){float a=fget((void*)(object+0xb0)),target=wanted();
        if(isfinite(a)&&(fabsf(a-16.f/9.f)<.01f || (c->valid&&a==c->assigned[0]))){
            float fov=fget((void*)(object+0xa0));
            if(!isfinite(fov)||fov<=.01f||fov>=3.f){pthread_mutex_unlock(&wide_lock);return;}
            if(!c->valid||fov!=c->assigned[1])c->base[1]=fov;
            // Narrow tablet: keep horizontal FOV and reveal more above/below.
            // Wide phone: preserve the existing fixed vertical FOV behaviour.
            float adjusted=target<16.f/9.f?2.f*atanf(tanf(c->base[1]*.5f)*(16.f/9.f)/target):c->base[1];
            fput((void*)(object+0xb0),target);fput((void*)(object+0xa0),adjusted);
            c->assigned[0]=target;c->assigned[1]=adjusted;
            if(!c->valid)LOG("[ACNH display] perspective aspect %.4f -> %.4f; fovy %.4f -> %.4f",a,target,c->base[1],adjusted);
            c->valid=1;
        }
    }pthread_mutex_unlock(&wide_lock);
}
#include "acnh_wide_ui.inc"
__attribute__((constructor)) static void acnh_display_hooks(void) {
    gate_obs_set(OBS_ACNH_PERSPECTIVE,perspective);
    gate_obs_set(OBS_ACNH_UI_PANE,ui_pane);
}
