#include <assert.h>
#include <math.h>
#include "../../gfx/nvn/acnh_present_math.h"
#include "../hle_acnh_wide.c"
module_t g_mod[MAX_MOD];int g_nmod;
void gate_obs_set(int id,void(*fn)(uint64_t*,int)){assert(id==26||id==28);(void)fn;}
int main(void){
 g_nmod=1;strcpy(g_mod[0].name,"main");strcpy(g_mod[0].build_id,"ff1d1c05670db6021c85b624a710b963");g_mod[0].base=(void*)0x100000000ull;
 uint64_t obj[32]={0},r[102]={0};uintptr_t p=(uintptr_t)obj,base=(uintptr_t)g_mod[0].base;r[0]=p;r[101]=base+0x1000;obj[0]=base+0x4a65608;
 fput((void*)(p+0xa0),.384f);fput((void*)(p+0xb0),16.f/9.f);acnh_wide_set_aspect(2868.f/1320.f);perspective(r,0);
 assert(fabsf(fget((void*)(p+0xb0))-2868.f/1320.f)<1e-6f);assert(fget((void*)(p+0xa0))==.384f);
 float a=fget((void*)(p+0xb0));perspective(r,0);assert(fget((void*)(p+0xb0))==a);
 float sx=(1320*a)/(a*tanf(.384f/2)),sy=1320/tanf(.384f/2);assert(fabsf(sx-sy)<.002f);
 fput((void*)(p+0xb0),16.f/9.f);r[101]=base+0x376d224;perspective(r,0);assert(fget((void*)(p+0xb0))==16.f/9.f);
 r[101]=base+0x1000;fput((void*)(p+0xb0),1.f);perspective(r,0);assert(fget((void*)(p+0xb0))==1.f);
 acnh_wide_set_aspect(NAN);assert(wanted()==16.f/9.f);
 double rect[4],native_aspect=2868./1320.;
 acnh_present_rect(2868,1320,1280,720,0,rect);
 double old_ratio=(rect[2]/native_aspect)/rect[3];assert(old_ratio<.83&&old_ratio>.80);
 acnh_wide_set_present_full(1);acnh_present_rect(2868,1320,1280,720,acnh_wide_present_full(),rect);
 assert(rect[0]==0&&rect[1]==0&&rect[2]==2868&&rect[3]==1320);
 assert(fabs((rect[2]/native_aspect)/rect[3]-1)<1e-6);
 acnh_wide_set_present_full(0);acnh_present_rect(2868,1320,1280,720,acnh_wide_present_full(),rect);assert(rect[0]>0);

 acnh_wide_set_aspect(2868.f/1320.f);float uk=wanted()/(16.f/9.f);
 uint64_t rootObj[40]={0},bgObj[40]={0},groupObj[40]={0};uintptr_t up=(uintptr_t)rootObj,bp=(uintptr_t)bgObj,gp=(uintptr_t)groupObj;
 rootObj[0]=base+0x474d1f8;fput((void*)(up+0x48),1);fput((void*)(up+0x50),1920);fput((void*)(up+0x54),1080);memcpy((void*)(up+0xb0),"RootPane",9);r[0]=up;ui_pane(r,0);
 assert(fabsf(ui_float(up,0x48)*uk-1)<1e-6f);assert(fabsf(ui_float(up,0x50)-1920*uk)<.001f);
 ui_pane(r,0);assert(fabsf(ui_float(up,0x48)*uk-1)<1e-6f);
 fput((void*)(up+0x70),ui_float(up,0x48));fput((void*)(up+0x84),1);
 bgObj[0]=base+0x474d418;*(uintptr_t*)(bp+0x18)=up;fput((void*)(bp+0x48),1);fput((void*)(bp+0x50),1920);fput((void*)(bp+0x54),1080);r[0]=bp;ui_pane(r,0);assert(fabsf(ui_float(bp,0x48)*ui_float(up,0x48)-1)<1e-6f);
 groupObj[0]=base+0x474d1f8;*(uintptr_t*)(gp+0x18)=up;fput((void*)(gp+0x30),746);fput((void*)(gp+0x50),30);r[0]=gp;ui_pane(r,0);assert(ui_float(gp,0x30)==746);memcpy((void*)(gp+0xb0),"L_Island_00",12);ui_pane(r,0);float shifted=ui_float(gp,0x30);assert(shifted>746);ui_pane(r,0);assert(ui_float(gp,0x30)==shifted);
 uint64_t wipeParent[40]={0},sideObj[40]={0};uintptr_t wp=(uintptr_t)wipeParent,sp=(uintptr_t)sideObj;
 wipeParent[0]=base+0x474d1f8;*(uintptr_t*)(wp+0x18)=up;memcpy((void*)(wp+0xb0),"P_Circle_00",12);
 fput((void*)(wp+0x70),ui_float(up,0x70));
 sideObj[0]=base+0x474d418;*(uintptr_t*)(sp+0x18)=wp;memcpy((void*)(sp+0xb0),"P_L_00",7);*(uint8_t*)(sp+0x5b)=0x12;
 fput((void*)(sp+0x50),1098);fput((void*)(sp+0x54),1080);fput((void*)(sp+0x48),1);r[0]=sp;ui_pane(r,0);
 float extended=1098+960*(uk-1);assert(fabsf(ui_float(sp,0x50)-extended)<.001f);assert(ui_float(sp,0x30)==0&&ui_float(sp,0x48)==1);
 ui_pane(r,0);assert(fabsf(ui_float(sp,0x50)-extended)<.001f);
 acnh_wide_set_aspect(16.f/9.f);r[0]=up;ui_pane(r,0);r[0]=sp;ui_pane(r,0);assert(ui_float(sp,0x50)==1098);
 acnh_wide_set_aspect(16.f/9.f);r[0]=up;ui_pane(r,0);assert(ui_float(up,0x48)==1&&ui_float(up,0x50)==1920);
 puts("PASS: world proportions, fixed vertical FOV, minimap exclusion, square views, UI proportion, idempotence and restore");
}
