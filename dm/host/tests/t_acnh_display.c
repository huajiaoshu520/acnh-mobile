#include <assert.h>
#include <math.h>
#include "../../gfx/nvn/acnh_present_math.h"
#include "../hle_acnh_wide.c"
static module_t modules[1];module_t *g_mod=modules;int g_nmod;
void gate_obs_set(int id,void(*fn)(uint64_t*,int)){assert(id==32||id==33);(void)fn;}
int main(void){
 g_nmod=1;strcpy(g_mod[0].name,"main");strcpy(g_mod[0].build_id,"ff1d1c05670db6021c85b624a710b963");g_mod[0].base=(void*)0x100000000ull;
 uint64_t obj[32]={0},r[102]={0},guest_frame[2]={0};r[99]=(uintptr_t)guest_frame;uintptr_t p=(uintptr_t)obj,base=(uintptr_t)g_mod[0].base;r[0]=p;guest_frame[1]=base+0x1000;obj[0]=base+0x4a65608;
 fput((void*)(p+0xa0),.384f);fput((void*)(p+0xb0),16.f/9.f);acnh_wide_set_aspect(2868.f/1320.f);perspective(r,0);
 assert(fabsf(fget((void*)(p+0xb0))-2868.f/1320.f)<1e-6f);assert(fget((void*)(p+0xa0))==.384f);
 float a=fget((void*)(p+0xb0));perspective(r,0);assert(fget((void*)(p+0xb0))==a);
 float sx=(1320*a)/(a*tanf(.384f/2)),sy=1320/tanf(.384f/2);assert(fabsf(sx-sy)<.002f);
 fput((void*)(p+0xb0),16.f/9.f);guest_frame[1]=base+0x376d224;perspective(r,0);assert(fget((void*)(p+0xb0))==16.f/9.f);
 guest_frame[1]=base+0x1000;fput((void*)(p+0xb0),1.f);perspective(r,0);assert(fget((void*)(p+0xb0))==1.f);
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
 fput((void*)(wp+0x48),1);fput((void*)(wp+0x4c),1);wipeParent[0]=base+0x474d1f8;*(uintptr_t*)(wp+0x18)=up;memcpy((void*)(wp+0xb0),"P_Circle_00",12);
 fput((void*)(wp+0x70),0); // Matrix not calculated yet.
 sideObj[0]=base+0x474d418;*(uintptr_t*)(sp+0x18)=wp;memcpy((void*)(sp+0xb0),"P_L_00",7);*(uint8_t*)(sp+0x5b)=0x12;
 fput((void*)(sp+0x50),1098);fput((void*)(sp+0x54),1080);fput((void*)(sp+0x48),1);r[0]=sp;ui_pane(r,0);
 float extended=1098+960*(uk-1);assert(fabsf(ui_float(sp,0x50)-extended)<.001f);assert(ui_float(sp,0x30)==0&&ui_float(sp,0x48)==1);
 ui_pane(r,0);assert(fabsf(ui_float(sp,0x50)-extended)<.001f);
 acnh_wide_set_aspect(16.f/9.f);r[0]=up;ui_pane(r,0);r[0]=sp;ui_pane(r,0);assert(ui_float(sp,0x50)==1098);
 acnh_wide_set_aspect(16.f/9.f);r[0]=up;ui_pane(r,0);assert(ui_float(up,0x48)==1&&ui_float(up,0x50)==1920);

 { // Real-layout corner fixtures: preserve scale/margins, avoid moving
 // capture sources, remain idempotent, and restore original-mode positions.
 uint64_t embeddedObj[40]={0},hudObj[40]={0},guideObj[40]={0},alignObj[40]={0};
 uintptr_t hp=(uintptr_t)hudObj,kg=(uintptr_t)guideObj,ka=(uintptr_t)alignObj;
 const struct {const char *name;float x,y,w,h;int ax,ay;} hud[]={
  {"N_Pos_00",-800,-430,220,49.6f,-1,-1},
  {"N_InOut_00",-930,580,30,40,-1,1},
  {"N_InOut_01",800,-450,30,40,1,-1},
  {"N_InOut_00",-870,-430,30,40,-1,-1},
  {"L_SaveLoadIcon_00",900,480,82.492f,70.627f,1,1},
  {"N_BU_00",932,416,30,40,1,1},
  {"N_CaptureMain_00",-910,-416,600,300,0,0},
  {"L_BtnPressA_00",0,10,600,920,0,0}
 };
 const float aspects[]={2868.f/1320.f,4.f/3.f};
 for(unsigned a=0;a<4;a++)for(unsigned i=0;i<sizeof hud/sizeof *hud;i++){
  memset(ui_records,0,sizeof ui_records);memset(hudObj,0,sizeof hudObj);
  acnh_wide_set_aspect(16.f/9.f);fput((void*)(up+0x48),1);fput((void*)(up+0x4c),1);
  fput((void*)(up+0x50),1920);fput((void*)(up+0x54),1080);
  acnh_wide_set_aspect(aspects[a%2]);r[0]=up;ui_pane(r,0);
  fput((void*)(up+0x70),ui_float(up,0x48));fput((void*)(up+0x84),ui_float(up,0x4c));
  hudObj[0]=base+(hud[i].name[0]=='L'?(a>=2?0x4a8f128:0x474d310):0x474d1f8);
  *(uintptr_t*)(hp+0x18)=up;
  if(a>=2||i==0||i==3){
   uintptr_t ep=(uintptr_t)embeddedObj;memset(embeddedObj,0,sizeof embeddedObj);
   embeddedObj[0]=base+0x474d1f8;*(uintptr_t*)(ep+0x18)=up;
   fput((void*)(ep+0x48),1);fput((void*)(ep+0x4c),1);
   strcpy((char*)(ep+0xb0),i==0?"L_Clock_00":i==3?"L_Tips_00":"RootPane");
   // Global matrices deliberately stale during the first animated frame.
   fput((void*)(ep+0x70),0);fput((void*)(ep+0x84),0);
   *(uintptr_t*)(hp+0x18)=ep;
  }
  strcpy((char*)(hp+0xb0),hud[i].name);
  fput((void*)(hp+0x30),hud[i].x);fput((void*)(hp+0x34),hud[i].y);
  fput((void*)(hp+0x50),hud[i].w);fput((void*)(hp+0x54),hud[i].h);
  fput((void*)(hp+0x48),.93f);fput((void*)(hp+0x4c),.93f);
  float ex=fmaxf(1,aspects[a%2]/(16.f/9.f)),ey=fmaxf(1,(16.f/9.f)/aspects[a%2]);
  r[0]=hp;ui_pane(r,0);
  float x=hud[i].x+hud[i].ax*960*(ex-1),y=hud[i].y+hud[i].ay*540*(ey-1);
  assert(fabsf(ui_float(hp,0x30)-x)<.002f&&fabsf(ui_float(hp,0x34)-y)<.002f);
  ui_pane(r,0);assert(fabsf(ui_float(hp,0x30)-x)<.002f);
  assert(ui_float(hp,0x48)==.93f&&ui_float(hp,0x4c)==.93f);
  acnh_wide_set_aspect(16.f/9.f);r[0]=up;ui_pane(r,0);
  fput((void*)(up+0x70),1);fput((void*)(up+0x84),1);if(a>=2||i==0||i==3){fput((void*)((uintptr_t)embeddedObj+0x70),1);fput((void*)((uintptr_t)embeddedObj+0x84),1);}r[0]=hp;ui_pane(r,0);
  assert(ui_float(hp,0x30)==hud[i].x&&ui_float(hp,0x34)==hud[i].y);
 }
 memset(ui_records,0,sizeof ui_records);acnh_wide_set_aspect(2868.f/1320.f);
 r[0]=up;ui_pane(r,0);fput((void*)(up+0x70),ui_float(up,0x48));
 guideObj[0]=base+0x474d1f8;alignObj[0]=base+0x4a860d8;*(uintptr_t*)(kg+0x18)=up;
 *(uintptr_t*)(ka+0x18)=kg;strcpy((char*)(kg+0xb0),"N_InOut_00");strcpy((char*)(ka+0xb0),"N_Align_00");
 fput((void*)(kg+0x48),1);fput((void*)(kg+0x4c),1);fput((void*)(kg+0x34),-540);fput((void*)(kg+0x70),ui_float(up,0x70));fput((void*)(kg+0x84),1);
 fput((void*)(ka+0x50),1810);fput((void*)(ka+0x54),100);fput((void*)(ka+0x34),50);
 r[0]=ka;ui_pane(r,0);float guideWidth=1810;float guideX=960*(uk-1);
 assert(fabsf(ui_float(ka,0x50)-guideWidth)<.002f&&fabsf(ui_float(ka,0x30)-guideX)<.002f);ui_pane(r,0);assert(fabsf(ui_float(ka,0x50)-guideWidth)<.002f);
 acnh_wide_set_aspect(16.f/9.f);r[0]=up;ui_pane(r,0);r[0]=ka;ui_pane(r,0);assert(ui_float(ka,0x50)==1810);
 memset(ui_records,0,sizeof ui_records);fput((void*)(bp+0x48),1);fput((void*)(bp+0x4c),1);
 }
 // Tablets retain horizontal FOV and reveal vertical scenery without stretching.
 const float tablets[]={4.f/3.f,2360.f/1640.f,2266.f/1488.f};
 for(unsigned i=0;i<sizeof tablets/sizeof *tablets;i++){
  float target=tablets[i],baseFov=.384f;acnh_wide_set_aspect(target);
  r[0]=p;guest_frame[1]=base+0x1000;fput((void*)(p+0xb0),16.f/9.f);fput((void*)(p+0xa0),baseFov);perspective(r,0);
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
 // Catalog/NatteeCatalog exit curtains: a moving 320-high picture carries
 // a 500/430-high child. Both meshes must cover the wide canvas; changing
 // width on the parent does not scale its child. Preserve animation and text.
 uint64_t curtainGroup[40]={0},curtainOuter[40]={0},curtainInner[40]={0};
 uintptr_t cg=(uintptr_t)curtainGroup,co=(uintptr_t)curtainOuter,ci=(uintptr_t)curtainInner;
 curtainGroup[0]=base+0x474d1f8;*(uintptr_t*)(cg+0x18)=up;strcpy((char*)(cg+0xb0),"N_InOut_02");
 curtainOuter[0]=curtainInner[0]=base+0x474d418;
 *(uintptr_t*)(co+0x18)=cg;*(uintptr_t*)(ci+0x18)=co;
 strcpy((char*)(ci+0xb0),"P_pict_02");
 fput((void*)(co+0x50),1920);fput((void*)(co+0x54),320);
 fput((void*)(ci+0x50),1920);fput((void*)(ci+0x54),500);
 fput((void*)(co+0x48),1);fput((void*)(co+0x4c),1);
 fput((void*)(ci+0x48),1);fput((void*)(ci+0x4c),1);
 const float curtainAspects[]={2622.f/1206.f,2868.f/1320.f,4.f/3.f,16.f/9.f};
 for(int kind=0;kind<2;kind++){
  strcpy((char*)(co+0xb0),kind?"P_BG_07":"P_pict_00");
  for(unsigned i=0;i<sizeof curtainAspects/sizeof *curtainAspects;i++){
   acnh_wide_set_aspect(curtainAspects[i]);r[0]=up;ui_pane(r,0);
   float ex=fmaxf(1,curtainAspects[i]/(16.f/9.f));
   for(int frame=0;frame<=20;frame++){
    float y=frame*32.f;fput((void*)(co+0x34),y);
    r[0]=co;ui_pane(r,0);r[0]=ci;ui_pane(r,0);
    assert(fabsf(ui_float(co,0x50)-1920*ex)<.002f);
    assert(fabsf(ui_float(ci,0x50)-1920*ex)<.002f);
    assert(ui_float(co,0x34)==y&&ui_float(co,0x48)==1&&ui_float(ci,0x48)==1);
    float width=ui_float(ci,0x50);ui_pane(r,0);assert(ui_float(ci,0x50)==width);
   }
  }
 }
 // A similarly named item/preview panel is not a screen-wide curtain.
 fput((void*)(ci+0x50),600);acnh_wide_set_aspect(2622.f/1206.f);
 r[0]=up;ui_pane(r,0);r[0]=ci;ui_pane(r,0);assert(ui_float(ci,0x50)==600);
 // Real Storage.bflyt header: extend decoration only, including reset/animation.
 uint64_t storageGroup[40]={0},headerParent[40]={0},headerPane[40]={0};
 uintptr_t sg=(uintptr_t)storageGroup,hp=(uintptr_t)headerParent,hd=(uintptr_t)headerPane;
 *(uintptr_t*)(sg+0x18)=up;strcpy((char*)(sg+0xb0),"N_Root_00");
 *(uintptr_t*)(hp+0x18)=sg;strcpy((char*)(hp+0xb0),"N_InOut_01");
 fput((void*)(sg+0x4c),1);fput((void*)(hp+0x4c),1);headerPane[0]=base+0x474d418;*(uintptr_t*)(hd+0x18)=hp;strcpy((char*)(hd+0xb0),"P_Header_00");
 fput((void*)(hd+0x50),1920);fput((void*)(hd+0x54),290);fput((void*)(hd+0x34),550);
 fput((void*)(hd+0x48),1);fput((void*)(hd+0x4c),1);
 const float headerAspects[]={2868.f/1320.f,4.f/3.f,2360.f/1640.f,16.f/9.f};
 for(unsigned i=0;i<sizeof headerAspects/sizeof *headerAspects;i++){
  acnh_wide_set_aspect(headerAspects[i]);r[0]=up;ui_pane(r,0);
  fput((void*)(up+0x84),ui_float(up,0x4c));fput((void*)(hp+0x84),0); // Current local scale must govern placement.
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
 // DIY recipe cursor is nested one level below N_InOut_01. The target
 // coordinates have already passed through the root matrix, just as in
 // Storage; cover the first/last columns and multiple rows in both axes.
 uint64_t recipeGroup[40]={0};uintptr_t recipe=(uintptr_t)recipeGroup;
 recipeGroup[0]=base+0x4a85590;*(uintptr_t*)(recipe+0x18)=up;
 strcpy((char*)(recipe+0xb0),"N_InOut_01");*(uintptr_t*)(cp+0x18)=recipe;
 const float recipeX[]={-792,32},recipeY[]={178,-138,-454};
 for(unsigned i=0;i<sizeof cursorAspects/sizeof *cursorAspects;i++){
  float ex=fmaxf(1,cursorAspects[i]/(16.f/9.f)),ey=fmaxf(1,(16.f/9.f)/cursorAspects[i]);
  acnh_wide_set_aspect(cursorAspects[i]);r[0]=up;ui_pane(r,0);
  for(unsigned col=0;col<2;col++)for(unsigned row=0;row<3;row++){
   float x=recipeX[col]/ex,y=recipeY[row]/ey;
   fput((void*)(cp+0x30),x);fput((void*)(cp+0x34),y);r[0]=cp;ui_pane(r,0);
   assert(fabsf(ui_float(cp,0x30)-recipeX[col])<.002f);
   assert(fabsf(ui_float(cp,0x34)-recipeY[row])<.002f);
   ui_pane(r,0);assert(fabsf(ui_float(cp,0x30)/ex-x)<.002f);
   assert(fabsf(ui_float(cp,0x34)/ey-y)<.002f);
   assert(ui_float(cp,0x48)==1&&ui_float(cp,0x4c)==1);
  }
 }
 // A different container remains outside this observed cursor path.
 strcpy((char*)(recipe+0xb0),"N_Other_00");
 fput((void*)(cp+0x30),123);fput((void*)(cp+0x34),456);r[0]=cp;ui_pane(r,0);
 assert(ui_float(cp,0x30)==123&&ui_float(cp,0x34)==456);
 *(uintptr_t*)(cp+0x18)=up;
 // Live LPocketBalloon: final capture, intermediate capture and source
 // glyphs use distinct pane types and must not receive double scaling.
 uint64_t balloonObj[40]={0},replayObj[40]={0},captureParent[40]={0},altObj[40]={0},beakObj[40]={0},textObj[40]={0},rotateObj[40]={0};
 uintptr_t bl=(uintptr_t)balloonObj,rp=(uintptr_t)replayObj,ca=(uintptr_t)captureParent,ap=(uintptr_t)altObj,bk=(uintptr_t)beakObj,tp=(uintptr_t)textObj,rot=(uintptr_t)rotateObj;
 balloonObj[0]=base+0x4a8f128;*(uintptr_t*)(bl+0x18)=up;strcpy((char*)(bl+0xb0),"L_ItemBalloon_00");
 rotateObj[0]=base+0x4a85590;*(uintptr_t*)(rot+0x18)=bl;strcpy((char*)(rot+0xb0),"N_Rotate_00");
 replayObj[0]=altObj[0]=beakObj[0]=base+0x4a86968;
 *(uintptr_t*)(rp+0x18)=rot;strcpy((char*)(rp+0xb0),"P_Cap_00");
 *(uintptr_t*)(ca+0x18)=rot;strcpy((char*)(ca+0xb0),"N_Capture_01");
 *(uintptr_t*)(ap+0x18)=ca;strcpy((char*)(ap+0xb0),"P_Capture_00");
 *(uintptr_t*)(bk+0x18)=bl;strcpy((char*)(bk+0xb0),"P_Beak_00");
 *(uintptr_t*)(tp+0x18)=rot;strcpy((char*)(tp+0xb0),"T_Name_00");
 fput((void*)(tp+0x34),-1852);
 uintptr_t panes[]={rp,ap,bk,tp};
 for(int j=0;j<4;j++){
  fput((void*)(panes[j]+0x48),1);fput((void*)(panes[j]+0x4c),1);
  fput((void*)(panes[j]+0x50),740);fput((void*)(panes[j]+0x54),148);
 }
 const float bubbleAspects[]={2622.f/1206.f,4.f/3.f,2420.f/1668.f,16.f/9.f,2622.f/1206.f};
 for(unsigned i=0;i<sizeof bubbleAspects/sizeof *bubbleAspects;i++){
  acnh_wide_set_aspect(bubbleAspects[i]);r[0]=up;ui_pane(r,0);
  float ex=fmaxf(1,bubbleAspects[i]/(16.f/9.f)),ey=fmaxf(1,(16.f/9.f)/bubbleAspects[i]);
  for(int j=0;j<4;j++){
   r[0]=panes[j];ui_pane(r,0);ui_pane(r,0);
   int corrected=j==0||j==3;
   assert(fabsf(ui_float(panes[j],0x48)-(corrected?1/ex:1))<1e-6);
   assert(fabsf(ui_float(panes[j],0x4c)-(corrected?1/ey:1))<1e-6);
   assert(ui_float(panes[j],0x50)==740&&ui_float(panes[j],0x54)==148);
  }
  float expected=-1852+1952*(1-1/ey);
  assert(fabsf(ui_float(tp,0x34)-expected)<.001);
  // Source lane rebase must cancel the distance lost to root scaling.
  assert(fabsf(ui_float(tp,0x34)-1952*(1-1/ey)+1852)<.001);
 }
 // The string variant has visible text, not the offscreen glyph lane.
 fput((void*)(tp+0x34),118);acnh_wide_set_aspect(4.f/3.f);
 r[0]=up;ui_pane(r,0);r[0]=tp;ui_pane(r,0);assert(ui_float(tp,0x34)==118);
 acnh_wide_set_aspect(16.f/9.f);r[0]=up;ui_pane(r,0);r[0]=tp;ui_pane(r,0);
 assert(ui_float(tp,0x48)==1&&ui_float(tp,0x4c)==1&&ui_float(tp,0x34)==118);
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
 // Live storage action menu: world-space centre minus local half-width
 // must retain the original gap for both mirrored row edges.
 uint64_t actionTree[7][40]={{0}};uintptr_t nodes[7];
 const char *nodeNames[]={"N_TypeH_00","N_Re_00","N_InOut_00","N_BtnNum_01","N_In_00","N_All_00","N_Size_00"};
 for(int j=0;j<7;j++){
  nodes[j]=(uintptr_t)actionTree[j];actionTree[j][0]=base+0x4a85590;
  *(uintptr_t*)(nodes[j]+0x18)=j?nodes[j-1]:up;
  strcpy((char*)(nodes[j]+0xb0),nodeNames[j]);
  fput((void*)(nodes[j]+0x48),1);fput((void*)(nodes[j]+0x4c),1);
 }
 const float centre=381.8f, halfWidth=177.6f, requestedGap=100.f;
 for(unsigned i=0;i<sizeof dragAspects/sizeof *dragAspects;i++){
  float ex=fmaxf(1,dragAspects[i]/(16.f/9.f));
  acnh_wide_set_aspect(dragAspects[i]);r[0]=up;ui_pane(r,0);
  for(int side=-1;side<=1;side+=2){
   float worldScale=-side/ex, anchor=side*693.f/ex;
   float raw=requestedGap+halfWidth-centre/ex;
   fput((void*)(nodes[2]+0x30),0);r[0]=nodes[2];ui_pane(r,0);
   fput((void*)(nodes[2]+0x30),raw);
   fput((void*)(nodes[2]+0x70),worldScale);
   fput((void*)(nodes[2]+0x7c),anchor+worldScale*raw);
   fput((void*)(nodes[5]+0x70),worldScale*1.02f);
   fput((void*)(nodes[5]+0x7c),anchor+worldScale*(raw+centre));
   r[0]=nodes[6];ui_pane(r,0);
   r[0]=nodes[2];ui_pane(r,0);
   float canonical=requestedGap+halfWidth-centre;
   assert(fabsf(ui_float(nodes[2],0x30)-canonical)<.001f);
   ui_pane(r,0);ui_pane(r,0);
   assert(fabsf(ui_float(nodes[2],0x30)-canonical)<.001f);
   // Same local gap on the left and right; root mirrors the result.
   assert(fabsf((ui_float(nodes[2],0x30)+centre)/ex-(requestedGap+halfWidth)/ex)<.001f);
   // Changing mode while open must not apply the conversion a second time.
   acnh_wide_set_aspect(16.f/9.f);r[0]=up;ui_pane(r,0);r[0]=nodes[2];ui_pane(r,0);
   assert(fabsf(ui_float(nodes[2],0x30)-canonical)<.001f);
   acnh_wide_set_aspect(dragAspects[i]);r[0]=up;ui_pane(r,0);
  }
 }
 fput((void*)(nodes[2]+0x30),0);r[0]=nodes[2];ui_pane(r,0);
 assert(ui_float(nodes[2],0x30)==0);
 // The 740x128 string variant uses the same final replay, not its source.
 fput((void*)(rp+0x54),128);acnh_wide_set_aspect(2622.f/1206.f);
 r[0]=up;ui_pane(r,0);r[0]=rp;ui_pane(r,0);
 assert(fabsf(ui_float(rp,0x48)*(2622.f/1206.f)/(16.f/9.f)-1)<1e-6);
 acnh_wide_set_aspect(16.f/9.f);r[0]=up;ui_pane(r,0);r[0]=rp;ui_pane(r,0);
 assert(ui_float(rp,0x48)==1&&ui_float(rp,0x4c)==1);
 // The decorating window keeps its tint and furniture controls; only its
 // redundant scene-capture pane is suppressed in expanded view. Original
 // aspect restores opacity, including an originally hidden capture.
 uint64_t capParent[40]={0},capPane[40]={0};
 uintptr_t capture_parent=(uintptr_t)capParent,cc=(uintptr_t)capPane;
 capParent[0]=base+0x4a85590;*(uintptr_t*)(capture_parent+0x18)=up;
 strcpy((char*)(capture_parent+0xb0),"N_InOut_03");
 capPane[0]=base+0x4a85b38;*(uintptr_t*)(cc+0x18)=capture_parent;
 strcpy((char*)(cc+0xb0),"N_Capture_00");
 fput((void*)(cc+0x50),932);fput((void*)(cc+0x54),1000);
 *(uint8_t*)(cc+0x59)=0xff;
 const float capture_aspects[]={2622.f/1206.f,4.f/3.f};
 for(unsigned i=0;i<2;i++){
  acnh_wide_set_aspect(capture_aspects[i]);r[0]=cc;ui_pane(r,0);
  assert(*(uint8_t*)(cc+0x59)==0x00);ui_pane(r,0);
  assert(*(uint8_t*)(cc+0x59)==0x00);
  acnh_wide_set_aspect(16.f/9.f);ui_pane(r,0);
  assert(*(uint8_t*)(cc+0x59)==0xff);
 }
 *(uint8_t*)(cc+0x59)=0x00;acnh_wide_set_aspect(2622.f/1206.f);ui_pane(r,0);
 acnh_wide_set_aspect(16.f/9.f);ui_pane(r,0);assert(*(uint8_t*)(cc+0x59)==0x00);
 // Other capture dimensions and parents retain their original opacity.
 *(uint8_t*)(cc+0x59)=0xff;fput((void*)(cc+0x50),740);
 acnh_wide_set_aspect(2622.f/1206.f);ui_pane(r,0);assert(*(uint8_t*)(cc+0x59)==0xff);
 fput((void*)(cc+0x50),932);strcpy((char*)(capture_parent+0xb0),"N_Other_00");
 ui_pane(r,0);assert(*(uint8_t*)(cc+0x59)==0xff);
 // Entry/exit wipe lane geometry covers both phone edges. Keep animated
 // vertical offsets and the independent icon unchanged; repeat and restore.
 uint64_t wipeAll[40]={0},wipeType[40]={0},wipeLanes[8][40]={{0}},wipeIcon[40]={0};
 uintptr_t wa=(uintptr_t)wipeAll,wt=(uintptr_t)wipeType,wi=(uintptr_t)wipeIcon;
 *(uintptr_t*)wa=*(uintptr_t*)wt=base+0x4a85590;
 *(uintptr_t*)(wa+0x18)=up;*(uintptr_t*)(wt+0x18)=wa;
 strcpy((char*)(wa+0xb0),"N_All_00");strcpy((char*)(wt+0xb0),"N_Type_00");
 for(int lane=0;lane<8;lane++){
  uintptr_t q=(uintptr_t)wipeLanes[lane];*(uintptr_t*)q=base+0x4a87220;
  *(uintptr_t*)(q+0x18)=wt;sprintf((char*)(q+0xb0),"P_Base_%02d",lane);
  fput((void*)(q+0x30),-840+240*lane);fput((void*)(q+0x34),123+lane);
  fput((void*)(q+0x50),lane==7?242:241);fput((void*)(q+0x54),1301);
  fput((void*)(q+0x48),1);fput((void*)(q+0x4c),1);
 }
 const float wipeAspects[]={2622.f/1206.f,2868.f/1320.f,16.f/9.f,4.f/3.f};
 for(unsigned a=0;a<4;a++){
  acnh_wide_set_aspect(wipeAspects[a]);r[0]=up;ui_pane(r,0);
  float ex=fmaxf(1,wipeAspects[a]/(16.f/9.f)),previous=0;
  for(int lane=0;lane<8;lane++){
   uintptr_t q=(uintptr_t)wipeLanes[lane];r[0]=q;ui_pane(r,0);ui_pane(r,0);
   float x=ui_float(q,0x30),width=ui_float(q,0x50);
   assert(fabsf(x-(-840+240*lane)*ex)<.002f);
   assert(fabsf(width-(lane==7?242:241)*ex)<.002f);
   assert(ui_float(q,0x34)==123+lane&&ui_float(q,0x54)==1301);
   assert(ui_float(q,0x48)==1&&ui_float(q,0x4c)==1);
   if(lane)assert(x-width/2<=previous+.002f);else assert(x-width/2<=-960*ex);
   previous=x+width/2;
  }
  assert(previous>=960*ex);
 }
 *(uintptr_t*)wi=base+0x4a87220;*(uintptr_t*)(wi+0x18)=wt;
 strcpy((char*)(wi+0xb0),"P_Icon_00");fput((void*)(wi+0x50),256);fput((void*)(wi+0x54),256);
 acnh_wide_set_aspect(2622.f/1206.f);r[0]=wi;ui_pane(r,0);assert(ui_float(wi,0x50)==256);
 uint64_t hotelWipeChild[40]={0};uintptr_t hc=(uintptr_t)hotelWipeChild;
 *(uintptr_t*)hc=base+0x4a87220;*(uintptr_t*)(hc+0x18)=(uintptr_t)wipeLanes[0];
 strcpy((char*)(hc+0xb0),"P_Base_08");fput((void*)(hc+0x50),241);fput((void*)(hc+0x54),1301);
 acnh_wide_set_aspect(2622.f/1206.f);r[0]=(uintptr_t)wipeLanes[0];ui_pane(r,0);r[0]=hc;ui_pane(r,0);
 assert(fabsf(ui_float(hc,0x50)-241*(2622.f/1206.f)/(16.f/9.f))<.002f);
 assert(ui_float(hc,0x30)==0);ui_pane(r,0);
 acnh_wide_set_aspect(16.f/9.f);ui_pane(r,0);assert(ui_float(hc,0x50)==241);
 // CastMainBG's overscan has a 5:3 ratio, so the ordinary 16:9
 // full-screen rule does not apply. Extend only the named background.
 uint64_t castGroup[40]={0},castBG[40]={0};
 uintptr_t cbg=(uintptr_t)castGroup,cback=(uintptr_t)castBG;
 *(uintptr_t*)cbg=base+0x4a85590;*(uintptr_t*)(cbg+0x18)=up;
 strcpy((char*)(cbg+0xb0),"N_InOut_00");
 *(uintptr_t*)cback=base+0x4a87220;*(uintptr_t*)(cback+0x18)=cbg;
 strcpy((char*)(cback+0xb0),"P_Base_00");
 fput((void*)(cback+0x50),2000);fput((void*)(cback+0x54),1200);
 fput((void*)(cback+0x48),1);fput((void*)(cback+0x4c),1);
 for(unsigned a=0;a<4;a++){
  acnh_wide_set_aspect(wipeAspects[a]);r[0]=up;ui_pane(r,0);
  r[0]=cback;ui_pane(r,0);ui_pane(r,0);
  assert(fabsf(ui_float(cback,0x48)-fmaxf(1,wipeAspects[a]/(16.f/9.f)))<1e-6f);
  assert(ui_float(cback,0x50)==2000&&ui_float(cback,0x54)==1200);
 }
 acnh_wide_set_aspect(16.f/9.f);r[0]=cback;ui_pane(r,0);
 strcpy((char*)(cback+0xb0),"P_Character_00");
 acnh_wide_set_aspect(2622.f/1206.f);ui_pane(r,0);
 assert(ui_float(cback,0x48)==1); // equally sized non-background stays unchanged
 // The job curtain group cancels root compensation for both its meshes
 // and its animated travel. It must not expose frame-zero off-screen art.
 uint64_t jobObj[40]={0};uintptr_t job=(uintptr_t)jobObj;
 *(uintptr_t*)job=base+0x4a85590;*(uintptr_t*)(job+0x18)=up;
 strcpy((char*)(job+0xb0),"N_JobWipe_00");
 fput((void*)(job+0x48),1);fput((void*)(job+0x4c),1);
 for(unsigned a=0;a<4;a++){
  acnh_wide_set_aspect(wipeAspects[a]);r[0]=up;ui_pane(r,0);r[0]=job;ui_pane(r,0);ui_pane(r,0);
  float ex=fmaxf(1,wipeAspects[a]/(16.f/9.f)),ey=fmaxf(1,(16.f/9.f)/wipeAspects[a]);
  assert(fabsf(ui_float(job,0x48)-ex)<1e-6f&&fabsf(ui_float(job,0x4c)-ey)<1e-6f);
  assert((1280-596.f/2)*ui_float(job,0x48)>960*ex);
 }
 acnh_wide_set_aspect(16.f/9.f);r[0]=job;ui_pane(r,0);assert(ui_float(job,0x48)==1&&ui_float(job,0x4c)==1);
 // Small BgMask mesh: only the runtime screen-sized instance is extended.
 uint64_t maskObj[40]={0};uintptr_t mask=(uintptr_t)maskObj;
 *(uintptr_t*)mask=base+0x4a87220;*(uintptr_t*)(mask+0x18)=up;strcpy((char*)(mask+0xb0),"P_Mask");
 fput((void*)(mask+0x50),100);fput((void*)(mask+0x54),100);
 fput((void*)(mask+0x48),19.2f);fput((void*)(mask+0x4c),10.8f);
 acnh_wide_set_aspect(2622.f/1206.f);r[0]=up;ui_pane(r,0);r[0]=mask;ui_pane(r,0);ui_pane(r,0);
 assert(fabsf(ui_float(mask,0x48)-19.2f*(2622.f/1206.f)/(16.f/9.f))<.00001f);
 acnh_wide_set_aspect(16.f/9.f);ui_pane(r,0);assert(ui_float(mask,0x48)==19.2f);
 fput((void*)(mask+0x48),1);fput((void*)(mask+0x4c),1);acnh_wide_set_aspect(2622.f/1206.f);ui_pane(r,0);
 assert(ui_float(mask,0x48)==1&&ui_float(mask,0x4c)==1);
 // OrderList's nested upper/lower frame pictures and gradient widen only
 // their own mesh. Titles, item cards and their animation are not scaled.
 uint64_t positionObj[40]={0},orderAnim[40]={0},orderFrame[40]={0},orderInner[40]={0},orderGrad[40]={0};
 uintptr_t pos=(uintptr_t)positionObj,oa=(uintptr_t)orderAnim,of=(uintptr_t)orderFrame,oi=(uintptr_t)orderInner,og=(uintptr_t)orderGrad;
 *(uintptr_t*)pos=*(uintptr_t*)oa=base+0x4a85590;*(uintptr_t*)(pos+0x18)=up;*(uintptr_t*)(oa+0x18)=pos;
 strcpy((char*)(pos+0xb0),"N_Position_00");strcpy((char*)(oa+0xb0),"N_InOut_00");
 *(uintptr_t*)of=*(uintptr_t*)oi=*(uintptr_t*)og=base+0x4a87220;
 *(uintptr_t*)(of+0x18)=oa;*(uintptr_t*)(oi+0x18)=of;*(uintptr_t*)(og+0x18)=pos;
 strcpy((char*)(of+0xb0),"P_FrameU_00");strcpy((char*)(oi+0xb0),"P_FrameU_01");strcpy((char*)(og+0xb0),"P_Grad_00");
 fput((void*)(of+0x50),1920);fput((void*)(oi+0x50),1920);fput((void*)(og+0x50),2000);
 fput((void*)(of+0x54),80);fput((void*)(oi+0x54),80);fput((void*)(og+0x54),600);fput((void*)(of+0x34),540);
 for(unsigned a=0;a<4;a++){
  acnh_wide_set_aspect(wipeAspects[a]);r[0]=up;ui_pane(r,0);float ex=fmaxf(1,wipeAspects[a]/(16.f/9.f));
  r[0]=of;ui_pane(r,0);ui_pane(r,0);r[0]=oi;ui_pane(r,0);r[0]=og;ui_pane(r,0);
  assert(fabsf(ui_float(of,0x50)-1920*ex)<.002f&&fabsf(ui_float(oi,0x50)-1920*ex)<.002f);
  assert(fabsf(ui_float(og,0x50)-2000*ex)<.002f&&ui_float(of,0x34)==540);
 }
 // Patterned-circle fill also has zero-height top/bottom rectangles.
 strcpy((char*)(wp+0xb0),"P_SirclePattern_00");
 acnh_wide_set_aspect(2622.f/1206.f);r[0]=up;ui_pane(r,0);
 fput((void*)(up+0x70),ui_float(up,0x48));fput((void*)(wp+0x70),.5f);
 r[0]=sp;ui_pane(r,0);assert(ui_float(sp,0x50)>1098);
 uint64_t circleTopObj[40]={0};uintptr_t ct=(uintptr_t)circleTopObj;
 *(uintptr_t*)ct=base+0x4a87220;*(uintptr_t*)(ct+0x18)=wp;
 strcpy((char*)(ct+0xb0),"P_T_00");fput((void*)(ct+0x50),1980);fput((void*)(ct+0x54),0);
 r[0]=ct;ui_pane(r,0);float cw=ui_float(ct,0x50);assert(cw>1980&&ui_float(ct,0x54)==0);
 ui_pane(r,0);assert(ui_float(ct,0x50)==cw);
 acnh_wide_set_aspect(16.f/9.f);ui_pane(r,0);assert(ui_float(ct,0x50)==1980);
 // ThemeStart's two runtime-traced gradient panes must span the same
 // normalized width as the viewport while retaining opacity and y offsets.
 uint64_t tsIn[40]={0},tsOut[40]={0},tsNull[40]={0},tsSh[2][40]={{0}};
 uintptr_t ti=(uintptr_t)tsIn,to=(uintptr_t)tsOut,tn=(uintptr_t)tsNull;
 *(uintptr_t*)ti=*(uintptr_t*)to=*(uintptr_t*)tn=base+0x4a85590;
 *(uintptr_t*)(ti+0x18)=up;*(uintptr_t*)(to+0x18)=ti;*(uintptr_t*)(tn+0x18)=to;
 strcpy((char*)(ti+0xb0),"N_InOut_00");strcpy((char*)(to+0xb0),"N_OutS_00");strcpy((char*)(tn+0xb0),"N_null_00");
 for(int j=0;j<2;j++){
  uintptr_t q=(uintptr_t)tsSh[j];*(uintptr_t*)q=base+0x4a87220;*(uintptr_t*)(q+0x18)=tn;
  sprintf((char*)(q+0xb0),"P_BGSh_%02d",j);fput((void*)(q+0x50),1920);fput((void*)(q+0x54),j?300:270);
  fput((void*)(q+0x34),j?270:540);*(uint8_t*)(q+0x59)=73;
 }
 for(unsigned a=0;a<4;a++){
  acnh_wide_set_aspect(wipeAspects[a]);r[0]=up;ui_pane(r,0);float ex=fmaxf(1,wipeAspects[a]/(16.f/9.f));
  for(int j=0;j<2;j++){
   uintptr_t q=(uintptr_t)tsSh[j];r[0]=q;ui_pane(r,0);ui_pane(r,0);
   assert(fabsf(ui_float(q,0x50)-1920*ex)<.002f&&ui_float(q,0x34)==(j?270:540));
   assert(*(uint8_t*)(q+0x59)==73);
  }
 }
 // The reveal aperture dimensions are animation output. Only its scale
 // compensates the wider canvas, including the zero-sized closing frame.
 uint64_t revealGroup[40]={0},revealDemo[40]={0},revealCircle[40]={0};
 uintptr_t rg=(uintptr_t)revealGroup,rd=(uintptr_t)revealDemo,rc=(uintptr_t)revealCircle;
 *(uintptr_t*)rg=*(uintptr_t*)rd=base+0x4a85590;*(uintptr_t*)rc=base+0x4a87220;
 *(uintptr_t*)(rg+0x18)=up;*(uintptr_t*)(rd+0x18)=rg;*(uintptr_t*)(rc+0x18)=rd;
 strcpy((char*)(rg+0xb0),"N_Demo1Wipe_00");strcpy((char*)(rd+0xb0),"N_Demo_00");strcpy((char*)(rc+0xb0),"P_SirclePattern_00");
 fput((void*)(rc+0x48),1);fput((void*)(rc+0x4c),1);
 const float apertureSizes[]={2830,814,0};
 for(unsigned a=0;a<4;a++){
  acnh_wide_set_aspect(wipeAspects[a]);r[0]=up;ui_pane(r,0);float ex=fmaxf(1,wipeAspects[a]/(16.f/9.f));
  for(int j=0;j<3;j++){
   fput((void*)(rc+0x50),apertureSizes[j]);fput((void*)(rc+0x54),apertureSizes[j]);
   r[0]=rc;ui_pane(r,0);ui_pane(r,0);
   assert(fabsf(ui_float(rc,0x48)-ex)<1e-6f&&ui_float(rc,0x50)==apertureSizes[j]);
  }
 }
 // DAL borders move to the new edges; their child mesh widths, flower/trim
 // scale and the centred logo stay untouched.
 uint64_t dalIn[40]={0},dalType[40]={0},dalSide[2][40]={{0}};
 uintptr_t di=(uintptr_t)dalIn,dt=(uintptr_t)dalType;
 *(uintptr_t*)di=*(uintptr_t*)dt=base+0x4a85590;*(uintptr_t*)(di+0x18)=up;*(uintptr_t*)(dt+0x18)=di;
 fput((void*)(di+0x48),1);fput((void*)(dt+0x48),1);strcpy((char*)(di+0xb0),"N_InOut_00");strcpy((char*)(dt+0xb0),"N_Type_00");
 for(int j=0;j<2;j++){
  uintptr_t q=(uintptr_t)dalSide[j];*(uintptr_t*)q=base+0x4a85590;*(uintptr_t*)(q+0x18)=dt;
  strcpy((char*)(q+0xb0),j?"N_PatternR_00":"N_PatternL_00");fput((void*)(q+0x30),j?940:-940);
  fput((void*)(q+0x48),1);fput((void*)(q+0x4c),1);
 }
 for(unsigned a=0;a<4;a++){
  acnh_wide_set_aspect(wipeAspects[a]);r[0]=up;ui_pane(r,0);float ex=fmaxf(1,wipeAspects[a]/(16.f/9.f));
  fput((void*)(up+0x70),ui_float(up,0x48));fput((void*)(dt+0x70),ui_float(up,0x48));
  for(int j=0;j<2;j++){
   uintptr_t q=(uintptr_t)dalSide[j];r[0]=q;ui_pane(r,0);ui_pane(r,0);
   assert(fabsf(ui_float(q,0x30)-(j?1:-1)*(940+960*(ex-1)))<.002f);
   assert(ui_float(q,0x48)==1&&ui_float(q,0x4c)==1);
  }
 }
 puts("PASS: world proportions, fixed vertical FOV, minimap exclusion, square views, UI proportion, idempotence and restore");
 return 0;
}
