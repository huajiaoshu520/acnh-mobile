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
int main(void){display_regression();
 { uint64_t nodes[16][40]; memset(ui_records,0,sizeof ui_records);
 init_pane(nodes[0],0,"RootPane",0x4a85590);
 init_pane(nodes[1],(uintptr_t)nodes[0],"N_InOut_02",0x4a85590);
 init_pane(nodes[2],(uintptr_t)nodes[1],"P_MatchingModeBase_00",0x4a87220);
 uintptr_t root=(uintptr_t)nodes[0],p=(uintptr_t)nodes[2];
 fput((void*)(root+0x50),1920);fput((void*)(root+0x54),1080);fput((void*)(p+0x50),1920.0f);fput((void*)(p+0x54),320.0f);fput((void*)(p+0x34),420.0f);*(uint8_t*)(p+0x5a)=83;
 for(int k=0;k<3;k++){float aspect=k==0?2.2f:k==1?4.f/3.f:16.f/9.f;acnh_wide_set_aspect(aspect);visit(nodes[0]);
 for(int repeat=0;repeat<3;repeat++){visit(nodes[2]);assert(fabsf(ui_float(p,0x50)-1920.0f*fmaxf(1,aspect/(16.f/9.f)))<.01f);assert(ui_float(p,0x48)==1&&ui_float(p,0x4c)==1&&ui_float(p,0x54)==320.0f&&ui_float(p,0x34)==420.0f&&*(uint8_t*)(p+0x5a)==83);} }
 memset(ui_records,0,sizeof ui_records);strcpy((char*)(root+0xb0),"Unrelated");fput((void*)(p+0x50),1920.0f);acnh_wide_set_aspect(2.2f);visit(nodes[2]);assert(ui_float(p,0x50)==1920.0f); }
 { uint64_t nodes[16][40]; memset(ui_records,0,sizeof ui_records);
 init_pane(nodes[0],0,"RootPane",0x4a85590);
 init_pane(nodes[1],(uintptr_t)nodes[0],"N_InOut_01",0x4a85590);
 init_pane(nodes[2],(uintptr_t)nodes[1],"P_Base_00",0x4a87220);
 uintptr_t root=(uintptr_t)nodes[0],p=(uintptr_t)nodes[2];
 fput((void*)(root+0x50),1920);fput((void*)(root+0x54),1080);fput((void*)(p+0x50),1920.0f);fput((void*)(p+0x54),250.0f);fput((void*)(p+0x34),0.0f);*(uint8_t*)(p+0x5a)=83;
 for(int k=0;k<3;k++){float aspect=k==0?2.2f:k==1?4.f/3.f:16.f/9.f;acnh_wide_set_aspect(aspect);visit(nodes[0]);
 for(int repeat=0;repeat<3;repeat++){visit(nodes[2]);assert(fabsf(ui_float(p,0x50)-1920.0f*fmaxf(1,aspect/(16.f/9.f)))<.01f);assert(ui_float(p,0x48)==1&&ui_float(p,0x4c)==1&&ui_float(p,0x54)==250.0f&&ui_float(p,0x34)==0.0f&&*(uint8_t*)(p+0x5a)==83);} }
 memset(ui_records,0,sizeof ui_records);strcpy((char*)(root+0xb0),"Unrelated");fput((void*)(p+0x50),1920.0f);acnh_wide_set_aspect(2.2f);visit(nodes[2]);assert(ui_float(p,0x50)==1920.0f); }
 { uint64_t nodes[16][40]; memset(ui_records,0,sizeof ui_records);
 init_pane(nodes[0],0,"RootPane",0x4a85590);
 init_pane(nodes[1],(uintptr_t)nodes[0],"N_InOut_01",0x4a85590);
 init_pane(nodes[2],(uintptr_t)nodes[1],"P_Base_01",0x4a87220);
 uintptr_t root=(uintptr_t)nodes[0],p=(uintptr_t)nodes[2];
 fput((void*)(root+0x50),1920);fput((void*)(root+0x54),1080);fput((void*)(p+0x50),1920.0f);fput((void*)(p+0x54),250.0f);fput((void*)(p+0x34),0.0f);*(uint8_t*)(p+0x5a)=83;
 for(int k=0;k<3;k++){float aspect=k==0?2.2f:k==1?4.f/3.f:16.f/9.f;acnh_wide_set_aspect(aspect);visit(nodes[0]);
 for(int repeat=0;repeat<3;repeat++){visit(nodes[2]);assert(fabsf(ui_float(p,0x50)-1920.0f*fmaxf(1,aspect/(16.f/9.f)))<.01f);assert(ui_float(p,0x48)==1&&ui_float(p,0x4c)==1&&ui_float(p,0x54)==250.0f&&ui_float(p,0x34)==0.0f&&*(uint8_t*)(p+0x5a)==83);} }
 memset(ui_records,0,sizeof ui_records);strcpy((char*)(root+0xb0),"Unrelated");fput((void*)(p+0x50),1920.0f);acnh_wide_set_aspect(2.2f);visit(nodes[2]);assert(ui_float(p,0x50)==1920.0f); }
 { uint64_t nodes[16][40]; memset(ui_records,0,sizeof ui_records);
 init_pane(nodes[0],0,"RootPane",0x4a85590);
 init_pane(nodes[1],(uintptr_t)nodes[0],"N_FinishOut_00",0x4a85590);
 init_pane(nodes[2],(uintptr_t)nodes[1],"N_TextInOut_00",0x4a85590);
 init_pane(nodes[3],(uintptr_t)nodes[2],"N_TextType_01",0x4a85590);
 init_pane(nodes[4],(uintptr_t)nodes[3],"P_BGShadow_00",0x4a87220);
 uintptr_t root=(uintptr_t)nodes[0],p=(uintptr_t)nodes[4];
 fput((void*)(root+0x50),1920);fput((void*)(root+0x54),1080);fput((void*)(p+0x50),1920.0f);fput((void*)(p+0x54),540.0f);fput((void*)(p+0x34),-60.0f);*(uint8_t*)(p+0x5a)=83;
 for(int k=0;k<3;k++){float aspect=k==0?2.2f:k==1?4.f/3.f:16.f/9.f;acnh_wide_set_aspect(aspect);visit(nodes[0]);
 for(int repeat=0;repeat<3;repeat++){visit(nodes[4]);assert(fabsf(ui_float(p,0x50)-1920.0f*fmaxf(1,aspect/(16.f/9.f)))<.01f);assert(ui_float(p,0x48)==1&&ui_float(p,0x4c)==1&&ui_float(p,0x54)==540.0f&&ui_float(p,0x34)==-60.0f&&*(uint8_t*)(p+0x5a)==83);} }
 memset(ui_records,0,sizeof ui_records);strcpy((char*)(root+0xb0),"Unrelated");fput((void*)(p+0x50),1920.0f);acnh_wide_set_aspect(2.2f);visit(nodes[4]);assert(ui_float(p,0x50)==1920.0f); }
 { uint64_t nodes[16][40]; memset(ui_records,0,sizeof ui_records);
 init_pane(nodes[0],0,"RootPane",0x4a85590);
 init_pane(nodes[1],(uintptr_t)nodes[0],"N_FinishOut_00",0x4a85590);
 init_pane(nodes[2],(uintptr_t)nodes[1],"N_TextInOut_00",0x4a85590);
 init_pane(nodes[3],(uintptr_t)nodes[2],"N_TextType_01",0x4a85590);
 init_pane(nodes[4],(uintptr_t)nodes[3],"P_BGgradation_00",0x4a87220);
 uintptr_t root=(uintptr_t)nodes[0],p=(uintptr_t)nodes[4];
 fput((void*)(root+0x50),1920);fput((void*)(root+0x54),1080);fput((void*)(p+0x50),1920.0f);fput((void*)(p+0x54),443.3333435058594f);fput((void*)(p+0x34),-19.0f);*(uint8_t*)(p+0x5a)=83;
 for(int k=0;k<3;k++){float aspect=k==0?2.2f:k==1?4.f/3.f:16.f/9.f;acnh_wide_set_aspect(aspect);visit(nodes[0]);
 for(int repeat=0;repeat<3;repeat++){visit(nodes[4]);assert(fabsf(ui_float(p,0x50)-1920.0f*fmaxf(1,aspect/(16.f/9.f)))<.01f);assert(ui_float(p,0x48)==1&&ui_float(p,0x4c)==1&&ui_float(p,0x54)==443.3333435058594f&&ui_float(p,0x34)==-19.0f&&*(uint8_t*)(p+0x5a)==83);} }
 memset(ui_records,0,sizeof ui_records);strcpy((char*)(root+0xb0),"Unrelated");fput((void*)(p+0x50),1920.0f);acnh_wide_set_aspect(2.2f);visit(nodes[4]);assert(ui_float(p,0x50)==1920.0f); }
 { uint64_t nodes[16][40]; memset(ui_records,0,sizeof ui_records);
 init_pane(nodes[0],0,"RootPane",0x4a85590);
 init_pane(nodes[1],(uintptr_t)nodes[0],"N_Root_00",0x4a85590);
 init_pane(nodes[2],(uintptr_t)nodes[1],"N_Pos_00",0x4a85590);
 init_pane(nodes[3],(uintptr_t)nodes[2],"N_Bg",0x4a85590);
 init_pane(nodes[4],(uintptr_t)nodes[3],"P_Bg_Black",0x4a87220);
 uintptr_t root=(uintptr_t)nodes[0],p=(uintptr_t)nodes[4];
 fput((void*)(root+0x50),1920);fput((void*)(root+0x54),1080);fput((void*)(p+0x50),1920.0f);fput((void*)(p+0x54),108.0f);fput((void*)(p+0x34),10.0f);*(uint8_t*)(p+0x5a)=83;
 for(int k=0;k<3;k++){float aspect=k==0?2.2f:k==1?4.f/3.f:16.f/9.f;acnh_wide_set_aspect(aspect);visit(nodes[0]);
 for(int repeat=0;repeat<3;repeat++){visit(nodes[4]);assert(fabsf(ui_float(p,0x50)-1920.0f*fmaxf(1,aspect/(16.f/9.f)))<.01f);assert(ui_float(p,0x48)==1&&ui_float(p,0x4c)==1&&ui_float(p,0x54)==108.0f&&ui_float(p,0x34)==10.0f&&*(uint8_t*)(p+0x5a)==83);} }
 memset(ui_records,0,sizeof ui_records);strcpy((char*)(root+0xb0),"Unrelated");fput((void*)(p+0x50),1920.0f);acnh_wide_set_aspect(2.2f);visit(nodes[4]);assert(ui_float(p,0x50)==1920.0f); }
 { uint64_t nodes[16][40]; memset(ui_records,0,sizeof ui_records);
 init_pane(nodes[0],0,"RootPane",0x4a85590);
 init_pane(nodes[1],(uintptr_t)nodes[0],"N_Root_00",0x4a85590);
 init_pane(nodes[2],(uintptr_t)nodes[1],"N_Pos_00",0x4a85590);
 init_pane(nodes[3],(uintptr_t)nodes[2],"N_Bg",0x4a85590);
 init_pane(nodes[4],(uintptr_t)nodes[3],"P_Bg_Gray",0x4a87220);
 uintptr_t root=(uintptr_t)nodes[0],p=(uintptr_t)nodes[4];
 fput((void*)(root+0x50),1920);fput((void*)(root+0x54),1080);fput((void*)(p+0x50),1920.0f);fput((void*)(p+0x54),108.0f);fput((void*)(p+0x34),10.0f);*(uint8_t*)(p+0x5a)=83;
 for(int k=0;k<3;k++){float aspect=k==0?2.2f:k==1?4.f/3.f:16.f/9.f;acnh_wide_set_aspect(aspect);visit(nodes[0]);
 for(int repeat=0;repeat<3;repeat++){visit(nodes[4]);assert(fabsf(ui_float(p,0x50)-1920.0f*fmaxf(1,aspect/(16.f/9.f)))<.01f);assert(ui_float(p,0x48)==1&&ui_float(p,0x4c)==1&&ui_float(p,0x54)==108.0f&&ui_float(p,0x34)==10.0f&&*(uint8_t*)(p+0x5a)==83);} }
 memset(ui_records,0,sizeof ui_records);strcpy((char*)(root+0xb0),"Unrelated");fput((void*)(p+0x50),1920.0f);acnh_wide_set_aspect(2.2f);visit(nodes[4]);assert(ui_float(p,0x50)==1920.0f); }
 { uint64_t nodes[16][40]; memset(ui_records,0,sizeof ui_records);
 init_pane(nodes[0],0,"RootPane",0x4a85590);
 init_pane(nodes[1],(uintptr_t)nodes[0],"N_InOut_00",0x4a85590);
 init_pane(nodes[2],(uintptr_t)nodes[1],"P_Base_00",0x4a87220);
 uintptr_t root=(uintptr_t)nodes[0],p=(uintptr_t)nodes[2];
 fput((void*)(root+0x50),1920);fput((void*)(root+0x54),1080);fput((void*)(p+0x50),1940.0f);fput((void*)(p+0x54),320.0f);fput((void*)(p+0x34),432.0f);*(uint8_t*)(p+0x5a)=83;
 for(int k=0;k<3;k++){float aspect=k==0?2.2f:k==1?4.f/3.f:16.f/9.f;acnh_wide_set_aspect(aspect);visit(nodes[0]);
 for(int repeat=0;repeat<3;repeat++){visit(nodes[2]);assert(fabsf(ui_float(p,0x50)-1940.0f*fmaxf(1,aspect/(16.f/9.f)))<.01f);assert(ui_float(p,0x48)==1&&ui_float(p,0x4c)==1&&ui_float(p,0x54)==320.0f&&ui_float(p,0x34)==432.0f&&*(uint8_t*)(p+0x5a)==83);} }
 memset(ui_records,0,sizeof ui_records);strcpy((char*)(root+0xb0),"Unrelated");fput((void*)(p+0x50),1940.0f);acnh_wide_set_aspect(2.2f);visit(nodes[2]);assert(ui_float(p,0x50)==1940.0f); }
 { uint64_t nodes[16][40]; memset(ui_records,0,sizeof ui_records);
 init_pane(nodes[0],0,"RootPane",0x4a85590);
 init_pane(nodes[1],(uintptr_t)nodes[0],"N_InOut_00",0x4a85590);
 init_pane(nodes[2],(uintptr_t)nodes[1],"P_Base_01",0x4a87220);
 uintptr_t root=(uintptr_t)nodes[0],p=(uintptr_t)nodes[2];
 fput((void*)(root+0x50),1920);fput((void*)(root+0x54),1080);fput((void*)(p+0x50),1940.0f);fput((void*)(p+0x54),360.0f);fput((void*)(p+0x34),-379.0f);*(uint8_t*)(p+0x5a)=83;
 for(int k=0;k<3;k++){float aspect=k==0?2.2f:k==1?4.f/3.f:16.f/9.f;acnh_wide_set_aspect(aspect);visit(nodes[0]);
 for(int repeat=0;repeat<3;repeat++){visit(nodes[2]);assert(fabsf(ui_float(p,0x50)-1940.0f*fmaxf(1,aspect/(16.f/9.f)))<.01f);assert(ui_float(p,0x48)==1&&ui_float(p,0x4c)==1&&ui_float(p,0x54)==360.0f&&ui_float(p,0x34)==-379.0f&&*(uint8_t*)(p+0x5a)==83);} }
 memset(ui_records,0,sizeof ui_records);strcpy((char*)(root+0xb0),"Unrelated");fput((void*)(p+0x50),1940.0f);acnh_wide_set_aspect(2.2f);visit(nodes[2]);assert(ui_float(p,0x50)==1940.0f); }
 { uint64_t nodes[16][40]; memset(ui_records,0,sizeof ui_records);
 init_pane(nodes[0],0,"RootPane",0x4a85590);
 init_pane(nodes[1],(uintptr_t)nodes[0],"P_Base_00",0x4a87220);
 uintptr_t root=(uintptr_t)nodes[0],p=(uintptr_t)nodes[1];
 fput((void*)(root+0x50),1920);fput((void*)(root+0x54),1080);fput((void*)(p+0x50),1920.0f);fput((void*)(p+0x54),600.0f);fput((void*)(p+0x34),-540.0f);*(uint8_t*)(p+0x5a)=83;
 for(int k=0;k<3;k++){float aspect=k==0?2.2f:k==1?4.f/3.f:16.f/9.f;acnh_wide_set_aspect(aspect);visit(nodes[0]);
 for(int repeat=0;repeat<3;repeat++){visit(nodes[1]);assert(fabsf(ui_float(p,0x50)-1920.0f*fmaxf(1,aspect/(16.f/9.f)))<.01f);assert(ui_float(p,0x48)==1&&ui_float(p,0x4c)==1&&ui_float(p,0x54)==600.0f&&ui_float(p,0x34)==-540.0f&&*(uint8_t*)(p+0x5a)==83);} }
 memset(ui_records,0,sizeof ui_records);strcpy((char*)(root+0xb0),"Unrelated");fput((void*)(p+0x50),1920.0f);acnh_wide_set_aspect(2.2f);visit(nodes[1]);assert(ui_float(p,0x50)==1920.0f); }
 { uint64_t nodes[16][40]; memset(ui_records,0,sizeof ui_records);
 init_pane(nodes[0],0,"RootPane",0x4a85590);
 init_pane(nodes[1],(uintptr_t)nodes[0],"N_InOut_00",0x4a85590);
 init_pane(nodes[2],(uintptr_t)nodes[1],"P_Bg_00",0x4a87220);
 uintptr_t root=(uintptr_t)nodes[0],p=(uintptr_t)nodes[2];
 fput((void*)(root+0x50),1920);fput((void*)(root+0x54),1080);fput((void*)(p+0x50),1920.0f);fput((void*)(p+0x54),700.0f);fput((void*)(p+0x34),-540.0f);*(uint8_t*)(p+0x5a)=83;
 for(int k=0;k<3;k++){float aspect=k==0?2.2f:k==1?4.f/3.f:16.f/9.f;acnh_wide_set_aspect(aspect);visit(nodes[0]);
 for(int repeat=0;repeat<3;repeat++){visit(nodes[2]);assert(fabsf(ui_float(p,0x50)-1920.0f*fmaxf(1,aspect/(16.f/9.f)))<.01f);assert(ui_float(p,0x48)==1&&ui_float(p,0x4c)==1&&ui_float(p,0x54)==700.0f&&ui_float(p,0x34)==-540.0f&&*(uint8_t*)(p+0x5a)==83);} }
 memset(ui_records,0,sizeof ui_records);strcpy((char*)(root+0xb0),"Unrelated");fput((void*)(p+0x50),1920.0f);acnh_wide_set_aspect(2.2f);visit(nodes[2]);assert(ui_float(p,0x50)==1920.0f); }
 { uint64_t nodes[16][40]; memset(ui_records,0,sizeof ui_records);
 init_pane(nodes[0],0,"RootPane",0x4a85590);
 init_pane(nodes[1],(uintptr_t)nodes[0],"N_InOut_00",0x4a85590);
 init_pane(nodes[2],(uintptr_t)nodes[1],"P_Base_00",0x4a87220);
 uintptr_t root=(uintptr_t)nodes[0],p=(uintptr_t)nodes[2];
 fput((void*)(root+0x50),1920);fput((void*)(root+0x54),1080);fput((void*)(p+0x50),1920.0f);fput((void*)(p+0x54),540.0f);fput((void*)(p+0x34),266.0f);*(uint8_t*)(p+0x5a)=83;
 for(int k=0;k<3;k++){float aspect=k==0?2.2f:k==1?4.f/3.f:16.f/9.f;acnh_wide_set_aspect(aspect);visit(nodes[0]);
 for(int repeat=0;repeat<3;repeat++){visit(nodes[2]);assert(fabsf(ui_float(p,0x50)-1920.0f*fmaxf(1,aspect/(16.f/9.f)))<.01f);assert(ui_float(p,0x48)==1&&ui_float(p,0x4c)==1&&ui_float(p,0x54)==540.0f&&ui_float(p,0x34)==266.0f&&*(uint8_t*)(p+0x5a)==83);} }
 memset(ui_records,0,sizeof ui_records);strcpy((char*)(root+0xb0),"Unrelated");fput((void*)(p+0x50),1920.0f);acnh_wide_set_aspect(2.2f);visit(nodes[2]);assert(ui_float(p,0x50)==1920.0f); }
 { uint64_t nodes[16][40]; memset(ui_records,0,sizeof ui_records);
 init_pane(nodes[0],0,"RootPane",0x4a85590);
 init_pane(nodes[1],(uintptr_t)nodes[0],"N_InOut_02",0x4a85590);
 init_pane(nodes[2],(uintptr_t)nodes[1],"P_Bg_01",0x4a87220);
 uintptr_t root=(uintptr_t)nodes[0],p=(uintptr_t)nodes[2];
 fput((void*)(root+0x50),1920);fput((void*)(root+0x54),1080);fput((void*)(p+0x50),1920.0f);fput((void*)(p+0x54),700.0f);fput((void*)(p+0x34),-540.0f);*(uint8_t*)(p+0x5a)=83;
 for(int k=0;k<3;k++){float aspect=k==0?2.2f:k==1?4.f/3.f:16.f/9.f;acnh_wide_set_aspect(aspect);visit(nodes[0]);
 for(int repeat=0;repeat<3;repeat++){visit(nodes[2]);assert(fabsf(ui_float(p,0x50)-1920.0f*fmaxf(1,aspect/(16.f/9.f)))<.01f);assert(ui_float(p,0x48)==1&&ui_float(p,0x4c)==1&&ui_float(p,0x54)==700.0f&&ui_float(p,0x34)==-540.0f&&*(uint8_t*)(p+0x5a)==83);} }
 memset(ui_records,0,sizeof ui_records);strcpy((char*)(root+0xb0),"Unrelated");fput((void*)(p+0x50),1920.0f);acnh_wide_set_aspect(2.2f);visit(nodes[2]);assert(ui_float(p,0x50)==1920.0f); }
 { uint64_t nodes[16][40]; memset(ui_records,0,sizeof ui_records);
 init_pane(nodes[0],0,"RootPane",0x4a85590);
 init_pane(nodes[1],(uintptr_t)nodes[0],"N_InOut_00",0x4a85590);
 init_pane(nodes[2],(uintptr_t)nodes[1],"P_Base_00",0x4a87220);
 uintptr_t root=(uintptr_t)nodes[0],p=(uintptr_t)nodes[2];
 fput((void*)(root+0x50),1920);fput((void*)(root+0x54),1080);fput((void*)(p+0x50),1920.0f);fput((void*)(p+0x54),200.0f);fput((void*)(p+0x34),0.0f);*(uint8_t*)(p+0x5a)=83;
 for(int k=0;k<3;k++){float aspect=k==0?2.2f:k==1?4.f/3.f:16.f/9.f;acnh_wide_set_aspect(aspect);visit(nodes[0]);
 for(int repeat=0;repeat<3;repeat++){visit(nodes[2]);assert(fabsf(ui_float(p,0x50)-1920.0f*fmaxf(1,aspect/(16.f/9.f)))<.01f);assert(ui_float(p,0x48)==1&&ui_float(p,0x4c)==1&&ui_float(p,0x54)==200.0f&&ui_float(p,0x34)==0.0f&&*(uint8_t*)(p+0x5a)==83);} }
 memset(ui_records,0,sizeof ui_records);strcpy((char*)(root+0xb0),"Unrelated");fput((void*)(p+0x50),1920.0f);acnh_wide_set_aspect(2.2f);visit(nodes[2]);assert(ui_float(p,0x50)==1920.0f); }
 { uint64_t nodes[16][40]; memset(ui_records,0,sizeof ui_records);
 init_pane(nodes[0],0,"RootPane",0x4a85590);
 init_pane(nodes[1],(uintptr_t)nodes[0],"N_InOut_00",0x4a85590);
 init_pane(nodes[2],(uintptr_t)nodes[1],"P_Base_01",0x4a87220);
 uintptr_t root=(uintptr_t)nodes[0],p=(uintptr_t)nodes[2];
 fput((void*)(root+0x50),1920);fput((void*)(root+0x54),1080);fput((void*)(p+0x50),1920.0f);fput((void*)(p+0x54),200.0f);fput((void*)(p+0x34),0.0f);*(uint8_t*)(p+0x5a)=83;
 for(int k=0;k<3;k++){float aspect=k==0?2.2f:k==1?4.f/3.f:16.f/9.f;acnh_wide_set_aspect(aspect);visit(nodes[0]);
 for(int repeat=0;repeat<3;repeat++){visit(nodes[2]);assert(fabsf(ui_float(p,0x50)-1920.0f*fmaxf(1,aspect/(16.f/9.f)))<.01f);assert(ui_float(p,0x48)==1&&ui_float(p,0x4c)==1&&ui_float(p,0x54)==200.0f&&ui_float(p,0x34)==0.0f&&*(uint8_t*)(p+0x5a)==83);} }
 memset(ui_records,0,sizeof ui_records);strcpy((char*)(root+0xb0),"Unrelated");fput((void*)(p+0x50),1920.0f);acnh_wide_set_aspect(2.2f);visit(nodes[2]);assert(ui_float(p,0x50)==1920.0f); }
 {uint64_t n[4][40];memset(ui_records,0,sizeof ui_records);
 init_pane(n[0],0,"RootPane",0x4a85590);fput((void*)((uintptr_t)n[0]+0x50),1920);fput((void*)((uintptr_t)n[0]+0x54),1080);
 init_pane(n[1],(uintptr_t)n[0],"N_InOut_00",0x4a85590);
 init_pane(n[2],(uintptr_t)n[1],"N_Type_00",0x4a85590);
 init_pane(n[3],(uintptr_t)n[2],"N_PatternR_00",0x4a85590);
 uintptr_t p=(uintptr_t)n[3];fput((void*)(p+0x30),940);acnh_wide_set_aspect(2.2f);visit(n[0]);
 for(int k=0;k<4;k++){float scale=k%2?0.75f:1.5f;fput((void*)((uintptr_t)n[2]+0x48),scale);
 visit(n[3]);float expected=940+960*(2.2f/(16.f/9.f)-1)/scale;assert(fabsf(ui_float(p,0x30)-expected)<.01f);visit(n[3]);assert(fabsf(ui_float(p,0x30)-expected)<.01f);}
 acnh_wide_set_aspect(16.f/9.f);visit(n[0]);visit(n[3]);assert(ui_float(p,0x30)==940);
 }
 puts("PASS:15 shipped background meshes, exclusions, opacity/scale, mode restore; DAL animated locals with zero globals");return 0;}
