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
 rootObj[0]=base+0x474d1f8;fput((void*)(up+0x48),1);fput((void*)(up+0x4c),1);fput((void*)(up+0x50),1920);fput((void*)(up+0x54),1080);memcpy((void*)(up+0xb0),"RootPane",9);r[0]=up;ui_pane(r,0);
 assert(fabsf(ui_float(up,0x48)*uk-1)<1e-6f);assert(fabsf(ui_float(up,0x50)-1920*uk)<.001f);
 ui_pane(r,0);assert(fabsf(ui_float(up,0x48)*uk-1)<1e-6f);
 fput((void*)(up+0x70),ui_float(up,0x48));fput((void*)(up+0x84),1);
 bgObj[0]=base+0x474d418;*(uintptr_t*)(bp+0x18)=up;fput((void*)(bp+0x48),1);fput((void*)(bp+0x4c),1);fput((void*)(bp+0x50),1920);fput((void*)(bp+0x54),1080);r[0]=bp;ui_pane(r,0);assert(fabsf(ui_float(bp,0x48)*ui_float(up,0x48)-1)<1e-6f);
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

 // Tablets retain horizontal FOV and reveal vertical scenery without stretching.
 const float tablets[]={4.f/3.f,2360.f/1640.f,2266.f/1488.f};
 for(unsigned i=0;i<sizeof tablets/sizeof *tablets;i++){
  float target=tablets[i],baseFov=.384f;acnh_wide_set_aspect(target);
  r[0]=p;r[101]=base+0x1000;fput((void*)(p+0xb0),16.f/9.f);fput((void*)(p+0xa0),baseFov);perspective(r,0);
  float fy=fget((void*)(p+0xa0));assert(fy>baseFov&&fabsf(fget((void*)(p+0xb0))-target)<1e-6);
  assert(fabsf(tanf(fy/2)*target-tanf(baseFov/2)*(16.f/9.f))<1e-6);
  float px=target/(target*tanf(fy/2)),py=1.f/tanf(fy/2);assert(fabsf(px-py)<1e-6);
  perspective(r,0);assert(fget((void*)(p+0xa0))==fy);
  r[0]=up;ui_pane(r,0);float ky=(16.f/9.f)/target;
  assert(ui_float(up,0x50)==1920&&ui_float(up,0x48)==1);
  assert(fabsf(ui_float(up,0x54)-1080*ky)<.002&&fabsf(ui_float(up,0x4c)*ky-1)<1e-6);
  ui_pane(r,0);assert(fabsf(ui_float(up,0x54)-1080*ky)<.002);
  fput((void*)(up+0x70),ui_float(up,0x48));fput((void*)(up+0x84),ui_float(up,0x4c));
  r[0]=bp;ui_pane(r,0);assert(fabsf(ui_float(bp,0x4c)*ui_float(up,0x4c)-1)<1e-6);
  r[0]=sp;ui_pane(r,0);assert(fabsf(ui_float(sp,0x54)-1080*ky)<.002&&ui_float(sp,0x50)==1098);
  acnh_wide_set_aspect(16.f/9.f);r[0]=p;perspective(r,0);assert(fabsf(fget((void*)(p+0xa0))-baseFov)<1e-6);
  r[0]=up;ui_pane(r,0);assert(ui_float(up,0x54)==1080&&ui_float(up,0x4c)==1);
  r[0]=bp;ui_pane(r,0);assert(ui_float(bp,0x48)==1&&ui_float(bp,0x4c)==1);
  r[0]=sp;ui_pane(r,0);assert(ui_float(sp,0x54)==1080);
 }
 // Real Storage.bflyt header: extend decoration only, including reset/animation.
 uint64_t storageGroup[40]={0},headerParent[40]={0},headerPane[40]={0};
 uintptr_t sg=(uintptr_t)storageGroup,hp=(uintptr_t)headerParent,hd=(uintptr_t)headerPane;
 *(uintptr_t*)(sg+0x18)=up;strcpy((char*)(sg+0xb0),"N_Root_00");
 *(uintptr_t*)(hp+0x18)=sg;strcpy((char*)(hp+0xb0),"N_InOut_01");
 headerPane[0]=base+0x474d418;*(uintptr_t*)(hd+0x18)=hp;strcpy((char*)(hd+0xb0),"P_Header_00");
 fput((void*)(hd+0x50),1920);fput((void*)(hd+0x54),290);fput((void*)(hd+0x34),550);
 fput((void*)(hd+0x48),1);fput((void*)(hd+0x4c),1);
 const float headerAspects[]={2868.f/1320.f,4.f/3.f,2360.f/1640.f,16.f/9.f};
 for(unsigned i=0;i<sizeof headerAspects/sizeof *headerAspects;i++){
  acnh_wide_set_aspect(headerAspects[i]);r[0]=up;ui_pane(r,0);
  fput((void*)(up+0x84),ui_float(up,0x4c));fput((void*)(hp+0x84),ui_float(up,0x4c));
  r[0]=hd;ui_pane(r,0);
  float kx=fmaxf(1,headerAspects[i]/(16.f/9.f)),ky=fmaxf(1,(16.f/9.f)/headerAspects[i]);
  assert(fabsf(ui_float(hd,0x50)-1920*kx)<.002);
  assert(fabsf(ui_float(hd,0x34)-(550+540*(ky-1)))<.002);
  assert(ui_float(hd,0x54)==290&&ui_float(hd,0x48)==1&&ui_float(hd,0x4c)==1);
  float oldY=ui_float(hd,0x34);ui_pane(r,0);assert(ui_float(hd,0x34)==oldY);
 }
 // The live Storage cursor uses a direct-root parts pane and world-derived
 // position. Check both edges, a new target, repeated layout and mode reset.
 uint64_t cursorObj[40]={0};uintptr_t cp=(uintptr_t)cursorObj;
 cursorObj[0]=base+0x4a8f128;*(uintptr_t*)(cp+0x18)=up;
 strcpy((char*)(cp+0xb0),"L_Cursor_00");
 fput((void*)(cp+0x48),1);fput((void*)(cp+0x4c),1);
 const float cursorAspects[]={2868.f/1320.f,4.f/3.f,16.f/9.f};
 for(unsigned i=0;i<sizeof cursorAspects/sizeof *cursorAspects;i++){
  acnh_wide_set_aspect(cursorAspects[i]);r[0]=up;ui_pane(r,0);
  float ex=fmaxf(1,cursorAspects[i]/(16.f/9.f)),ey=fmaxf(1,(16.f/9.f)/cursorAspects[i]);
  for(int side=-1;side<=1;side+=2){
   float x=side*600.f,y=247.f;
   fput((void*)(cp+0x30),x);fput((void*)(cp+0x34),y);r[0]=cp;ui_pane(r,0);
   assert(fabsf(ui_float(cp,0x30)-x*ex)<.002&&fabsf(ui_float(cp,0x34)-y*ey)<.002);
   ui_pane(r,0);assert(fabsf(ui_float(cp,0x30)-x*ex)<.002);
   assert(ui_float(cp,0x48)==1&&ui_float(cp,0x4c)==1);
  }
 }
 // Captured item-name bubbles have a custom parts transform. Compensate
 // only their replay picture, retaining capture-source and glyph scales.
 uint64_t balloonObj[40]={0},replayObj[40]={0},captureParent[40]={0},altObj[40]={0},beakObj[40]={0};
 uintptr_t bl=(uintptr_t)balloonObj,rp=(uintptr_t)replayObj,ca=(uintptr_t)captureParent,ap=(uintptr_t)altObj,bk=(uintptr_t)beakObj;
 balloonObj[0]=base+0x4a8f128;*(uintptr_t*)(bl+0x18)=up;strcpy((char*)(bl+0xb0),"L_ItemBalloon_00");
 replayObj[0]=altObj[0]=beakObj[0]=base+0x4a87220;
 *(uintptr_t*)(rp+0x18)=bl;strcpy((char*)(rp+0xb0),"P_Cap_00");
 *(uintptr_t*)(ca+0x18)=bl;strcpy((char*)(ca+0xb0),"N_Capture_01");
 *(uintptr_t*)(ap+0x18)=ca;strcpy((char*)(ap+0xb0),"P_Capture_00");
 *(uintptr_t*)(bk+0x18)=bl;strcpy((char*)(bk+0xb0),"P_Beak_00");
 uintptr_t panes[]={rp,ap,bk};
 for(int j=0;j<3;j++){
  fput((void*)(panes[j]+0x48),1);fput((void*)(panes[j]+0x4c),1);
  fput((void*)(panes[j]+0x50),740);fput((void*)(panes[j]+0x54),148);
 }
 const float bubbleAspects[]={2868.f/1320.f,4.f/3.f,2420.f/1668.f,16.f/9.f,2868.f/1320.f};
 for(unsigned i=0;i<sizeof bubbleAspects/sizeof *bubbleAspects;i++){
  acnh_wide_set_aspect(bubbleAspects[i]);r[0]=up;ui_pane(r,0);
  float factor=fminf(1,bubbleAspects[i]/(16.f/9.f));
  for(int j=0;j<3;j++){
   r[0]=panes[j];ui_pane(r,0);float expected=j==2?1:factor;
   assert(fabsf(ui_float(panes[j],0x4c)-expected)<1e-6);
   ui_pane(r,0);assert(fabsf(ui_float(panes[j],0x4c)-expected)<1e-6);
   float expectedX=j==2?1:fminf(1,(16.f/9.f)/bubbleAspects[i]);
   assert(fabsf(ui_float(panes[j],0x48)-expectedX)<1e-6);
   assert(ui_float(panes[j],0x50)==740&&ui_float(panes[j],0x54)==148);
   if(j<2&&bubbleAspects[i]<16.f/9.f)assert(fabsf(ui_float(panes[j],0x4c)*(16.f/9.f)/bubbleAspects[i]-1)<1e-6);
  }
 }
 // PocketMenu held item: world-derived position under N_Main_00 must
 // land on the same slot after horizontal/vertical root compensation.
 uint64_t pocketMain[40]={0},dragObj[40]={0};
 uintptr_t pm=(uintptr_t)pocketMain,dp=(uintptr_t)dragObj;
 pocketMain[0]=base+0x474d1f8;*(uintptr_t*)(pm+0x18)=up;
 strcpy((char*)(pm+0xb0),"N_Main_00");
 dragObj[0]=base+0x4a8f128;*(uintptr_t*)(dp+0x18)=pm;
 strcpy((char*)(dp+0xb0),"L_DragBtn_00");
 fput((void*)(dp+0x48),.95f);fput((void*)(dp+0x4c),.95f);
 const float dragAspects[]={2622.f/1206.f,4.f/3.f,2360.f/1640.f,16.f/9.f};
 for(unsigned i=0;i<sizeof dragAspects/sizeof *dragAspects;i++){
  acnh_wide_set_aspect(dragAspects[i]);r[0]=up;ui_pane(r,0);
  float ex=fmaxf(1,dragAspects[i]/(16.f/9.f)),ey=fmaxf(1,(16.f/9.f)/dragAspects[i]);
  for(int edge=-1;edge<=1;edge+=2){
   float x=edge*600.f,y=edge*240.f;
   fput((void*)(dp+0x30),x);fput((void*)(dp+0x34),y);r[0]=dp;ui_pane(r,0);
   assert(fabsf(ui_float(dp,0x30)/ex-x)<.002f);
   assert(fabsf(ui_float(dp,0x34)/ey-y)<.002f);
   float dx=ui_float(dp,0x30),dy=ui_float(dp,0x34);ui_pane(r,0);
   assert(ui_float(dp,0x30)==dx&&ui_float(dp,0x34)==dy);
   assert(ui_float(dp,0x48)==.95f&&ui_float(dp,0x4c)==.95f);
  }
 }
 acnh_wide_set_aspect(16.f/9.f);r[0]=up;ui_pane(r,0);r[0]=dp;ui_pane(r,0);
 assert(ui_float(dp,0x30)==600&&ui_float(dp,0x34)==240);
 // Freshly opened CmnSubWin already receives the compensated item anchor.
 // Preserve it and the local left/right avoidance offsets across reopening.
 uint64_t actionObj[40]={0};uintptr_t act=(uintptr_t)actionObj;
 actionObj[0]=base+0x4a85590;*(uintptr_t*)(act+0x18)=up;
 strcpy((char*)(act+0xb0),"N_TypeH_00");
 fput((void*)(act+0x50),30);fput((void*)(act+0x54),40);
 fput((void*)(act+0x48),1);fput((void*)(act+0x4c),1);
 for(unsigned i=0;i<sizeof dragAspects/sizeof *dragAspects;i++){
  acnh_wide_set_aspect(dragAspects[i]);r[0]=up;ui_pane(r,0);
  float ex=fmaxf(1,dragAspects[i]/(16.f/9.f)),ey=fmaxf(1,(16.f/9.f)/dragAspects[i]);
  fput((void*)(up+0x70),ui_float(up,0x48));fput((void*)(up+0x84),ui_float(up,0x4c));
  for(int side=-1;side<=1;side+=2){
   float anchor=side*693.f/ex;
   fput((void*)(up+0x30),anchor);fput((void*)(up+0x34),219.f/ey);
   r[0]=act;ui_pane(r,0);
   float origin=ui_float(up,0x30)+ui_float(up,0x70)*ui_float(act,0x30);
   assert(fabsf(origin-anchor)<.002f);
   assert(fabsf(ui_float(up,0x34)+ui_float(up,0x84)*ui_float(act,0x34)-219.f/ey)<.002f);
   // Original menu bounds: right of the left item, left of the right item.
   float offset=side<0?90.f:-450.f, menuLeft=origin+offset/ex,menuRight=menuLeft+360.f/ex;
   if(side<0)assert(menuLeft>anchor+28.f/ex);
   else assert(menuRight<anchor-28.f/ex);
   float x=ui_float(act,0x30),y=ui_float(act,0x34);ui_pane(r,0);
   assert(ui_float(act,0x30)==x&&ui_float(act,0x34)==y);
   assert(ui_float(act,0x48)==1&&ui_float(act,0x4c)==1);
  }
 }
 acnh_wide_set_aspect(16.f/9.f);r[0]=up;ui_pane(r,0);r[0]=act;ui_pane(r,0);
 assert(fabsf(ui_float(act,0x30))<.002f&&fabsf(ui_float(act,0x34))<.002f);
 fput((void*)(up+0x30),0);fput((void*)(up+0x34),0);
 // The 740x128 string variant uses the same final replay, not its source.
 fput((void*)(rp+0x54),128);acnh_wide_set_aspect(2622.f/1206.f);
 r[0]=up;ui_pane(r,0);r[0]=rp;ui_pane(r,0);
 assert(fabsf(ui_float(rp,0x48)*(2622.f/1206.f)/(16.f/9.f)-1)<1e-6);
 acnh_wide_set_aspect(16.f/9.f);r[0]=up;ui_pane(r,0);r[0]=rp;ui_pane(r,0);
 assert(ui_float(rp,0x48)==1&&ui_float(rp,0x4c)==1);
 puts("PASS: world proportions, fixed vertical FOV, minimap exclusion, square views, UI proportion, idempotence and restore");
}
