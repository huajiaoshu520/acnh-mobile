#define main display_regression
#include "t_acnh_display.c"
#undef main
static void pane(uint64_t *o,uintptr_t base,uintptr_t parent,const char *name,float x,float y,float w,float h){
 uintptr_t p=(uintptr_t)o;o[0]=base+0x474d1f8;*(uintptr_t*)(p+0x18)=parent;strcpy((char*)(p+0xb0),name);
 fput((void*)(p+0x30),x);fput((void*)(p+0x34),y);fput((void*)(p+0x50),w);fput((void*)(p+0x54),h);fput((void*)(p+0x48),1);fput((void*)(p+0x4c),1);
}
int main(void){display_regression();uint64_t root[40]={0},part[40]={0},nr[40]={0},np[40]={0},right[40]={0},r[102]={0};
 uintptr_t base=(uintptr_t)g_mod[0].base,top=(uintptr_t)root,p=(uintptr_t)right;
 memset(ui_records,0,sizeof ui_records);pane(root,base,0,"RootPane",0,0,1920,1080);pane(part,base,top,"L_Footer",0,-10,1920,1080);
 pane(nr,base,(uintptr_t)part,"N_Root_00",0,-540,20,20);pane(np,base,(uintptr_t)nr,"N_Pos_00",0,0,20,20);pane(right,base,(uintptr_t)np,"N_Right",960,54,0,200);
 float aspect[]={2868.f/1320.f,4.f/3.f};
 for(int a=0;a<2;a++){
  acnh_wide_set_aspect(aspect[a]);r[0]=top;ui_pane(r,0);
  float dx=960*(fmaxf(1,aspect[a]/(16.f/9.f))-1),dy=540*(fmaxf(1,(16.f/9.f)/aspect[a])-1);
  for(int frame=0;frame<12;frame++){
   float animation=frame*2.f;fput((void*)(p+0x30),960+animation);fput((void*)(p+0x34),54+animation);
   fput((void*)((uintptr_t)nr+0x48),.5f+frame*.04f);float scale=ui_float((uintptr_t)nr,0x48);
   // Previous frame globals are intentionally zero: anchoring must not wait.
   fput((void*)((uintptr_t)np+0x70),0);fput((void*)((uintptr_t)np+0x84),0);
   r[0]=p;ui_pane(r,0);assert(fabsf(ui_float(p,0x30)-(960+animation+dx/scale))<.002f);assert(fabsf(ui_float(p,0x34)-(54+animation-dy))<.002f);
   ui_pane(r,0);assert(fabsf(ui_float(p,0x30)-(960+animation+dx/scale))<.002f);
  }
  acnh_wide_set_aspect(16.f/9.f);r[0]=top;ui_pane(r,0);r[0]=p;ui_pane(r,0);assert(ui_float(p,0x30)==982&&ui_float(p,0x34)==76);
 }
 // Packed in-game guide row must also use this frame's local scales.
 memset(ui_records,0,sizeof ui_records);
 pane(root,base,0,"RootPane",0,0,1920,1080);
 pane(nr,base,top,"N_InOut_00",0,-540,30,40);
 pane(right,base,(uintptr_t)nr,"N_Align_00",0,50,1810,100);
 acnh_wide_set_aspect(2868.f/1320.f);r[0]=top;ui_pane(r,0);
 float dx=960*((2868.f/1320.f)/(16.f/9.f)-1);
 for(int frame=0;frame<10;frame++){
  float scale=.6f+frame*.03f;fput((void*)((uintptr_t)nr+0x48),scale);
  fput((void*)((uintptr_t)nr+0x70),0);fput((void*)((uintptr_t)nr+0x84),0);
  r[0]=p;ui_pane(r,0);assert(fabsf(ui_float(p,0x30)-dx/scale)<.002f);
  ui_pane(r,0);assert(fabsf(ui_float(p,0x30)-dx/scale)<.002f);
 }
 acnh_wide_set_aspect(16.f/9.f);r[0]=top;ui_pane(r,0);r[0]=p;ui_pane(r,0);
 assert(ui_float(p,0x30)==0 && ui_float(p,0x50)==1810);
 puts("PASS: live keyboard footer path; moving/fading parent; stale globals; original restore");return 0;}
