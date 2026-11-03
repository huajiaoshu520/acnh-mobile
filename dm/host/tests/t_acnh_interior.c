#define main display_regression
#include "t_acnh_display.c"
#undef main
static void init_pane(uint64_t *o,uintptr_t parent,const char *name,uintptr_t vt){
 uintptr_t p=(uintptr_t)o;memset(o,0,320);o[0]=(uintptr_t)g_mod[0].base+vt;
 *(uintptr_t*)(p+0x18)=parent;strcpy((char*)(p+0xb0),name);
 fput((void*)(p+0x48),1);fput((void*)(p+0x4c),1);
 fput((void*)(p+0x50),30);fput((void*)(p+0x54),40);
}
static void visit(uint64_t *o){uint64_t r[102]={0};r[0]=(uintptr_t)o;ui_pane(r,0);}
int main(void){
 display_regression();
 const float aspects[]={2868.f/1320.f,4.f/3.f,2360.f/1640.f};
 for(int layout=0;layout<3;layout++)for(int aspect=0;aspect<3;aspect++){
  uint64_t root[40],type[40],left[40],right[40],guide[40],share[40],mouse[40],normal[40],label[40],icon[40];
  memset(ui_records,0,sizeof ui_records);
  init_pane(root,0,"RootPane",0x4a85590);uintptr_t top=(uintptr_t)root;
  fput((void*)(top+0x50),1920);fput((void*)(top+0x54),1080);
  acnh_wide_set_aspect(aspects[aspect]);visit(root);
  init_pane(type,top,layout==2?"N_Type_01":"N_Type_00",0x4a85590);
  uintptr_t parent=layout?(uintptr_t)type:top;
  init_pane(left,parent,layout==2?"N_InOutLeft_01":"N_InOutLeft_00",0x4a85590);
  init_pane(right,parent,layout==2?"N_InOutRight_01":"N_InOutRight_00",0x4a85590);
  init_pane(guide,(uintptr_t)left,"N_GuideInOutJuji_00",0x4a85590);
  init_pane(share,(uintptr_t)guide,layout==2?"N_Share_02":"N_Share_00",0x4a85590);
  init_pane(mouse,(uintptr_t)guide,"N_GuideMouse_00",0x4a85590);
  init_pane(normal,(uintptr_t)right,layout?"N_TypePlus_00":"N_Normal_00",0x4a85590);
  init_pane(label,(uintptr_t)right,layout?"T_Icon_21":"T_Icon_01",0x4a875b8);
  init_pane(icon,(uintptr_t)right,"P_KeyBase_03",0x4a87220);
  float ex=fmaxf(1,aspects[aspect]/(16.f/9.f)),ey=fmaxf(1,(16.f/9.f)/aspects[aspect]);
  float dx=960*(ex-1),dy=540*(ey-1);
  for(int frame=0;frame<12;frame++){
   float animation=(11-frame)*3.f;
   fput((void*)((uintptr_t)left+0x30),-animation);fput((void*)((uintptr_t)right+0x30),animation);
   visit(left);visit(right);visit(share);visit(mouse);visit(normal);visit(label);visit(icon);
   assert(fabsf(ui_float((uintptr_t)left,0x30)+animation+dx)<.002f);
   assert(fabsf(ui_float((uintptr_t)right,0x30)-animation-dx)<.002f);
   assert(fabsf(ui_float((uintptr_t)share,0x34)-dy)<.002f);
   assert(fabsf(ui_float((uintptr_t)mouse,0x34)+dy)<.002f);
   assert(fabsf(ui_float((uintptr_t)normal,0x34)-dy)<.002f);
   assert(fabsf(ui_float((uintptr_t)label,0x34)-dy)<.002f);
   assert(fabsf(ui_float((uintptr_t)icon,0x34)-dy)<.002f);
   // Child guides must never receive another copy of the side translation.
   assert(ui_float((uintptr_t)share,0x30)==0 && ui_float((uintptr_t)normal,0x30)==0);
   visit(left);visit(right);assert(fabsf(ui_float((uintptr_t)right,0x30)-animation-dx)<.002f);
  }
  acnh_wide_set_aspect(16.f/9.f);visit(root);visit(left);visit(right);visit(share);visit(mouse);visit(normal);visit(label);visit(icon);
  assert(ui_float((uintptr_t)left,0x30)==0&&ui_float((uintptr_t)right,0x30)==0);
  assert(ui_float((uintptr_t)share,0x34)==0&&ui_float((uintptr_t)mouse,0x34)==0);
  assert(ui_float((uintptr_t)label,0x34)==0&&ui_float((uintptr_t)icon,0x34)==0);
 }
 // MainScreen ordinary-room guides have their own Parts containers.
 const char *names[]={"L_GuideRoom_00","L_EffectClothGuide_00","L_NpcOnOffGuide_00"};
 for(int a=0;a<3;a++)for(int n=0;n<3;n++){
  uint64_t root[40],all[40],part[40],child[40];memset(ui_records,0,sizeof ui_records);
  init_pane(root,0,"RootPane",0x4a85590);uintptr_t top=(uintptr_t)root;
  fput((void*)(top+0x50),1920);fput((void*)(top+0x54),1080);
  init_pane(all,top,"N_All_00",0x4a85590);
  init_pane(part,(uintptr_t)all,names[n],0x4a8f128);
  init_pane(child,(uintptr_t)part,"N_InOut_00",0x4a85590);
  float y=n==0?256:n==1?82:-28;
  fput((void*)((uintptr_t)part+0x30),-820);fput((void*)((uintptr_t)part+0x34),y);
  acnh_wide_set_aspect(aspects[a]);visit(root);
  float dx=960*(fmaxf(1,aspects[a]/(16.f/9.f))-1);
  float dy=540*(fmaxf(1,(16.f/9.f)/aspects[a])-1);
  for(int i=0;i<3;i++){visit(part);visit(child);
   assert(fabsf(ui_float((uintptr_t)part,0x30)+820+dx)<.002f);
   assert(fabsf(ui_float((uintptr_t)part,0x34)-y-dy)<.002f);
   assert(ui_float((uintptr_t)child,0x30)==0);
  }
  acnh_wide_set_aspect(16.f/9.f);visit(root);visit(part);
  assert(ui_float((uintptr_t)part,0x30)==-820&&ui_float((uintptr_t)part,0x34)==y);
 }
 puts("PASS: interior side groups, HHP/revisit parents, top/bottom tablet anchors, labels, animation, no double shift, original restore");return 0;
}
