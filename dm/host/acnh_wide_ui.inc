// UI-only layout adaptation; derives from the documented nn::ui2d pane ABI.
// ACNHRecomp reference (MIT notice retained with this build).
typedef struct {
 uintptr_t key,vtable;char identity_name[24];int root,valid,shifted,corner_x,corner_y,storage_header,interior_wipe,interior_capture_hidden,interior_capture_alpha;
 float sx,last_sx,width,last_width,tx,last_tx,ty,last_ty;
 float sy,last_sy,height,last_height;
 float action_center,action_correction;
} acnh_ui_record;
static acnh_ui_record ui_records[4096];
static pthread_mutex_t ui_mutex=PTHREAD_MUTEX_INITIALIZER;
static acnh_ui_record *ui_record(uintptr_t p,int create) {
 unsigned slot=(unsigned)((p>>4)*2654435761u)&4095;
 for(unsigned n=0;n<4096;n++,slot=(slot+1)&4095){
  acnh_ui_record *c=&ui_records[slot];
  if(c->key==p){
   // Game layouts reuse allocations. Do not carry a prior pane's shift flag
   // into a new pane and incorrectly suppress its descendants' anchoring.
   if(c->vtable!=*(uintptr_t*)p||memcmp(c->identity_name,(void*)(p+0xb0),24)){
    memset(c,0,sizeof *c);c->key=p;c->vtable=*(uintptr_t*)p;
    memcpy(c->identity_name,(void*)(p+0xb0),24);
   }
   return c;
  }
  if(!c->key){if(create){c->key=p;c->vtable=*(uintptr_t*)p;memcpy(c->identity_name,(void*)(p+0xb0),24);}return create?c:NULL;}
 }return NULL;
}
static float ui_float(uintptr_t p,int off){return fget((void*)(p+off));}
static void ui_write(uintptr_t p,int off,float v){if(ui_float(p,off)!=v){fput((void*)(p+off),v);*(uint8_t*)(p+0x58)|=0x10;}}
// The observation runs before this pane's matrix calculation. Parent global
// matrices can still describe the prior animation frame, so derive the ratio
// from current local scales instead. The common root scale cancels out.
static float ui_parent_ratio(uintptr_t parent,uintptr_t top,int off){
 float scale=1.f;
 for(int depth=0;parent&&parent!=top&&depth<32;depth++,parent=*(uintptr_t*)(parent+0x18)){
  float v=ui_float(parent,off);if(!isfinite(v)||fabsf(v)<1e-5f)return 0.f;scale*=v;
 }
 return parent==top&&isfinite(scale)&&fabsf(scale)>1e-5f?1.f/scale:0.f;
}
// Match an immediate ancestor chain, allowing a nested layout RootPane.
static int ui_path(uintptr_t p,const char *path){
 const char *end=path+strlen(path);
 while(end>path&&p){
  const char *start=end;while(start>path&&start[-1]!='/')start--;
  size_t n=(size_t)(end-start);
  if(n>=24||strlen((char*)(p+0xb0))!=n||memcmp((void*)(p+0xb0),start,n))return 0;
  if(start==path)return 1;
  p=*(uintptr_t*)(p+0x18);end=start-1;
 }
 return 0;
}
static void ui_pane(uint64_t *r,int param) {
 (void)param;static uintptr_t base; if(!base)base=main_base();uintptr_t p=r[0];if(!base||!p)return;
 // Observe the authored title prompt before any aspect adaptation. In the
 // verified 3.0.3 Pane::Calculate ABI, flag bit 0 is visibility, +0x59 alpha.
 // Inspect local ancestors: global alpha can still belong to the prior frame.
 if(ui_path(p,"RootPane/L_BtnPressA_00") && ui_float(p,0x50)==600.f && ui_float(p,0x54)==920.f){
  int visible=1;uintptr_t node=p;
  for(int depth=0;node&&depth<32;depth++,node=*(uintptr_t*)(node+0x18)){
   if(!(*(uint8_t*)(node+0x58)&1)||!*(uint8_t*)(node+0x59)){visible=0;break;}
  }
  if(node)visible=0;
  atomic_store_explicit(&title_prompt_frame,visible?dm_nvn_frame_count()+1:0,memory_order_release);
 }
 float k=wanted()/(16.f/9.f),ex=fmaxf(1.f,k),ey=fmaxf(1.f,1.f/k);uintptr_t vt=*(uintptr_t*)p-base,parent=*(uintptr_t*)(p+0x18);
 int plain=vt==0x474d1f8||vt==0x4a85590||vt==0x4a860d8,parts=vt==0x474d310||vt==0x4a8f128;
 int picture=vt==0x474d418||vt==0x474de58||vt==0x4a87220;
 // Capture replays use their own Picture subclass in the live game.
 // Keep this type scoped to name balloons; it also draws unrelated UI captures.
 int capture_picture=vt==0x4a86968;
 const char *pane_name=(const char*)(p+0xb0);
 int balloon_text=parent&&!strcmp(pane_name,"T_Name_00")&&
  !strcmp((const char*)(parent+0xb0),"N_Rotate_00");
 // The edit-mode plus label is a TextBox sibling of its background.
 // Keep both in the same corner without scaling or moving unrelated text.
 int interior_top_text=parent&&(vt==0x4a875b8||vt==0x474d7c8)&&
  (!strcmp(pane_name,"T_Icon_01")||!strcmp(pane_name,"T_Icon_15")||!strcmp(pane_name,"T_Icon_21"))&&
  !strncmp((char*)(parent+0xb0),"N_InOutRight_",13);
 int cursor=vt==0x4a8f128&&!strcmp((const char*)(p+0xb0),"L_Cursor_00");
 int pocket_drag=(parts||vt==0x4a8f128)&&!strcmp((const char*)(p+0xb0),"L_DragBtn_00");
 // Interior storage's capture pane replays screen pixels using the original
 // canvas coordinates. Root UI compensation moves that replay independently
 // of the live room, creating a second character and a vertical scene seam.
 // In expanded view retain the live room under the existing tinted window;
 // suppress only this redundant capture replay, not the frame or item grid.
 // Restore its original opacity when returning to the original aspect.
 if(vt==0x4a85b38&&!strcmp(pane_name,"N_Capture_00")&&parent&&
    !strcmp((const char*)(parent+0xb0),"N_InOut_03")&&
    ui_float(p,0x50)==932.f&&ui_float(p,0x54)==1000.f){
  pthread_mutex_lock(&ui_mutex);
  acnh_ui_record *c=ui_record(p,fabsf(k-1.f)>.001f);
  uint8_t *alpha=(uint8_t*)(p+0x59);
  if(c){
   if(fabsf(k-1.f)>.001f){
    if(!c->interior_capture_hidden)c->interior_capture_alpha=*alpha;
    *alpha=0;c->interior_capture_hidden=1;
   }else if(c->interior_capture_hidden){
    *alpha=c->interior_capture_alpha;c->interior_capture_hidden=0;
   }
  }
  pthread_mutex_unlock(&ui_mutex);return;
 }
 if(parent&&!plain&&!parts&&!picture&&!capture_picture&&!balloon_text&&!cursor&&!pocket_drag&&!interior_top_text)return;
 pthread_mutex_lock(&ui_mutex);
 if(!parent){
  acnh_ui_record *c=ui_record(p,fabsf(k-1.f)>.001f);if(!c){pthread_mutex_unlock(&ui_mutex);return;}
  float w=ui_float(p,0x50),h=ui_float(p,0x54),sx=ui_float(p,0x48),sy=ui_float(p,0x4c);
  if(!c->valid||w!=c->last_width)c->width=w;
  if(!c->valid||sx!=c->last_sx)c->sx=sx;
  if(!c->valid||h!=c->last_height)c->height=h;
  if(!c->valid||sy!=c->last_sy)c->sy=sy;
  if(!isfinite(c->height)||c->height<1||c->width<1279||fabsf(c->width/c->height-16.f/9.f)>.02f){pthread_mutex_unlock(&ui_mutex);return;}
  c->root=1;c->last_width=c->width*ex;c->last_sx=c->sx/ex;
  c->last_height=c->height*ey;c->last_sy=c->sy/ey;
  ui_write(p,0x50,c->last_width);ui_write(p,0x48,c->last_sx);
  ui_write(p,0x54,c->last_height);ui_write(p,0x4c,c->last_sy);
  if(!c->valid)LOG("[ACNH UI] root %.24s %.0fx%.0f; UI scale compensation %.4f x %.4f",(char*)(p+0xb0),c->width,c->height,1.f/ex,1.f/ey);
  c->valid=1;pthread_mutex_unlock(&ui_mutex);return;
 }
 uintptr_t top=parent;int nested=0,pocket_balloon=0;
 for(int n=0;n<32;n++){
  acnh_ui_record *a=ui_record(top,0);if(a&&a->shifted)nested=1;
  if(!strcmp((const char*)(top+0xb0),"L_ItemBalloon_00"))pocket_balloon=1;
  uintptr_t next=*(uintptr_t*)(top+0x18);if(!next)break;top=next;
 }
 acnh_ui_record *root=ui_record(top,0);if(!root||!root->root){pthread_mutex_unlock(&ui_mutex);return;}
 // Translate final HUD groups, not their off-screen capture sources. The
 // baseline is retained across repeated layout visits and animation writes.
 // Names alone are not unique: pair them with the shipped pane dimensions
 // and parent path. Descendants inherit the translation exactly once.
 acnh_ui_record *hc=ui_record(p,0);
 float hx=hc&&hc->valid&&ui_float(p,0x30)==hc->last_tx?hc->tx:ui_float(p,0x30);
 float hy=hc&&hc->valid&&ui_float(p,0x34)==hc->last_ty?hc->ty:ui_float(p,0x34);
 float hw=ui_float(p,0x50),hh=ui_float(p,0x54);
 const char *pn=(const char*)(parent+0xb0);
 int ax=hc?hc->corner_x:0,ay=hc?hc->corner_y:0;
 if(!nested&&(plain||parts||picture||capture_picture||interior_top_text)){
  // Interior, HHP and revisit layouts share these side guide containers.
  // Move the whole side horizontally once; its children retain their own
  // top/bottom vertical anchors when tablets reveal additional height.
  int interior_parent=!strcmp(pn,"RootPane")||
   (!strcmp(pn,"N_All_00")&&*(uintptr_t*)(parent+0x18)&&
    !strcmp((char*)(*(uintptr_t*)(parent+0x18)+0xb0),"RootPane"))||
   ((!strcmp(pn,"N_Type_00")||!strcmp(pn,"N_Type_01"))&&
    *(uintptr_t*)(parent+0x18)&&
    !strcmp((char*)(*(uintptr_t*)(parent+0x18)+0xb0),"RootPane"));
  if(plain&&interior_parent){
   if(!strncmp(pane_name,"N_InOutLeft_",12)){ax=-1;ay=0;}
   if(!strncmp(pane_name,"N_InOutRight_",13)){ax=1;ay=0;}
  }
  if(!strncmp(pn,"N_GuideInOutJuji_",17)){
   if((plain&&!strncmp(pane_name,"N_Share_",8))||
      ((picture||capture_picture)&&!strcmp(pane_name,"P_CaptureShadow_01"))){ax=0;ay=1;}
   if((plain&&(!strcmp(pane_name,"N_GuideMouse_00")||!strcmp(pane_name,"N_GuideStudio_01")))||
      (parts&&(!strcmp(pane_name,"L_GuideMouse_00")||!strcmp(pane_name,"L_Amiibo_00")))){ax=0;ay=-1;}
  }
  if(!strncmp(pn,"N_InOutRight_",13)&&
     ((plain&&(!strcmp(pane_name,"N_Normal_00")||!strcmp(pane_name,"N_Celing_00")||!strncmp(pane_name,"N_TypePlus_",11)))||
      (picture&&!strcmp(pane_name,"P_KeyBase_03"))||interior_top_text)){ax=0;ay=1;}

  if(!strcmp(pane_name,"N_InSaveDataSetting_00")&&hw==30&&hh==30&&
     !strcmp(pn,"N_InOut_00")){ax=-1;ay=-1;}
  if(!strcmp(pane_name,"N_Pos_00")&&!strcmp(pn,"L_Clock_00")){ax=-1;ay=-1;}
  if((!strcmp(pane_name,"N_Pos_00")&&hw==30&&hh==40)||
     (!strcmp(pane_name,"P_Base_00")&&hw==412&&hh==70)){
   if(hx>850&&hy>850&&!strcmp(pn,"N_InOut_00")){ax=1;ay=1;}
  }
  if(!strncmp(pane_name,"N_InOut_",8)&&hw==30&&hh==40&&!strcmp(pn,"RootPane")&&
     hx<-700&&hy>400){ax=-1;ay=1;}
  if(!strcmp(pane_name,"N_InOut_01")&&hw==30&&hh==40&&
     !strcmp(pn,"RootPane")&&hx>600&&hy<-200){ax=1;ay=-1;}
  if(!strcmp(pane_name,"N_InOut_00")&&hw==30&&hh==40&&
     (!strcmp(pn,"RootPane")||!strncmp(pn,"L_Tips_",7))&&hx==-870&&hy==-430){ax=-1;ay=-1;}
  if(!strcmp(pn,"N_Icon_00")&&parts&&
     (!strncmp(pane_name,"L_Island_",9)||!strncmp(pane_name,"L_Airplane_",11)||
      !strncmp(pane_name,"L_Dream_",8)||!strncmp(pane_name,"L_CloudIsland_",14)||
      !strncmp(pane_name,"L_Ship_",7)||!strncmp(pane_name,"L_Kpp_",6)||
      !strncmp(pane_name,"L_Man_",6))&&hx>700&&hy<-300){ax=1;ay=-1;}
  if(!strcmp(pane_name,"L_SaveLoadIcon_00")&&parts&&!strcmp(pn,"RootPane")){ax=1;ay=1;}
  if(!strcmp(pane_name,"N_BU_00")&&hw==30&&hh==40&&
     !strcmp(pn,"RootPane")&&hx==932&&hy==416){ax=1;ay=1;}
  // Camera UI uses separate edge controls; do not move its centred frame,
  // capture target, crosshair or photo composition overlays.
  if(plain){
   if(ui_path(p,"RootPane/N_InOut_00/N_Guide_00/N_GuideOff_00/N_Invalid_00/N_Filter_00")||
      ui_path(p,"RootPane/N_InOut_00/N_Guide_00/N_GuideOff_00/N_Invalid_00/N_Frame")){ax=-1;ay=1;}
   if(ui_path(p,"RootPane/N_InOut_00/N_Guide_00/N_GuideOff_00/N_Invalid_00/N_AppTypeF5_00")||
      ui_path(p,"RootPane/N_InOut_00/N_Guide_00/N_GuideOff_00/N_Invalid_00/N_Look_00")||
      ui_path(p,"RootPane/N_InOut_00/N_Guide_00/N_GuideOff_00/N_Invalid_00/N_Reaction_00/N_AllEmoticon_00")){ax=1;ay=1;}
   if(ui_path(p,"RootPane/N_InOut_00/N_Date_00")&&hw==30&&hh==40){ax=1;ay=-1;}
   if(ui_path(p,"RootPane/N_Root_00/N_InOut_01/N_Num_00")&&hw==30&&hh==40){ax=-1;ay=-1;}
  }
  if(parts&&ui_path(p,"RootPane/L_ReleasedInsectNum_00")){ax=-1;ay=1;}
  if(parts&&!strcmp(pn,"N_All_00")){
   if(!strcmp(pane_name,"L_Timer_00")){ax=0;ay=1;}
   // Ordinary indoor play uses these MainScreen parts, independently of
   // the Interior editor side groups. Anchor the complete replay group.
   if(!strcmp(pane_name,"L_GuideRoom_00")||!strcmp(pane_name,"L_EffectClothGuide_00")||
      !strcmp(pane_name,"L_NpcOnOffGuide_00")){ax=-1;ay=1;}
   if(!strcmp(pane_name,"L_Road_00")||!strcmp(pane_name,"L_EffectCloth_00")||
      !strcmp(pane_name,"L_Album_00")||!strcmp(pane_name,"L_AlbumHotel_00")){ax=1;ay=1;}
   if(!strcmp(pane_name,"L_MealEf_00")){ax=-1;ay=1;}
  }
 }
 // The in-game software keyboard uses the system L_Footer layout, not
 // the game's N_Align_00 guide row. Anchor its packed edge group only;
 // the keyboard keys and centred input balloon remain unchanged.
 uintptr_t footer_root=*(uintptr_t*)(parent+0x18);
 uintptr_t footer_part=footer_root?*(uintptr_t*)(footer_root+0x18):0;
 if(!nested&&plain&&!strcmp(pn,"N_Pos_00")&&footer_root&&footer_part&&
    !strcmp((char*)(footer_root+0xb0),"N_Root_00")&&
    !strcmp((char*)(footer_part+0xb0),"L_Footer")){
  if(!strcmp(pane_name,"N_Right")){ax=1;ay=-1;}
  if(!strcmp(pane_name,"N_Left")){ax=-1;ay=-1;}
 }
 // Skip prompt: move its visible text and shadow replay together. Keep
 // N_CaptureShadow_00 at its original render-to-texture coordinates.
 if(!nested&&!strcmp(pn,"N_InOut_00")&&ui_float(parent,0x34)==475.f&&
    ((plain&&!strcmp(pane_name,"N_Align_00")&&hw==1790&&hh==100)||
     ((picture||capture_picture)&&!strcmp(pane_name,"P_CaptureShadow_00")&&hw==450&&hh==80))){ax=1;ay=1;}
 if((ax||ay)&&!nested){
  hc=ui_record(p,1);
  float rx=ui_parent_ratio(parent,top,0x48),ry=ui_parent_ratio(parent,top,0x4c);
  if(hc&&rx!=0.f&&ry!=0.f){
   hc->tx=hx;hc->ty=hy;hc->corner_x=ax;hc->corner_y=ay;
   hc->last_tx=hx+ax*root->width*.5f*(ex-1.f)*rx;
   hc->last_ty=hy+ay*root->height*.5f*(ey-1.f)*ry;
   ui_write(p,0x30,hc->last_tx);ui_write(p,0x34,hc->last_ty);
   hc->valid=1;hc->shifted=(ax&&ex>1.001f)||(ay&&ey>1.001f);
  }
  pthread_mutex_unlock(&ui_mutex);return;
 }
 // Shared bottom guides are packed by game code, not a responsive layout
 // manager. Move the final packed row as a unit; enlarging N_Align alone
 // leaves the already positioned B/A/Y buttons at their original locations.
 if(plain&&!strcmp(pane_name,"N_Align_00")&&hh==100&&
    *(uintptr_t*)(parent+0x18)&&!strcmp((char*)(*(uintptr_t*)(parent+0x18)+0xb0),"RootPane")&&
    ((!strcmp(pn,"N_InOut_00")&&ui_float(parent,0x34)==-540.f)||
     (!strcmp(pn,"N_All_00")&&hy==-486.f))){
  acnh_ui_record *c=ui_record(p,1);
  float px=ui_parent_ratio(parent,top,0x48),py=ui_parent_ratio(parent,top,0x4c);
  if(c&&hw==1810.f&&isfinite(px)&&isfinite(py)&&fabsf(px)>1e-5f&&fabsf(py)>1e-5f){
   c->tx=hx;c->ty=hy;
   c->last_tx=hx+root->width*.5f*(ex-1.f)*px;
   c->last_ty=hy-root->height*.5f*(ey-1.f)*py;
   ui_write(p,0x30,c->last_tx);ui_write(p,0x34,c->last_ty);c->valid=1;
  }
  pthread_mutex_unlock(&ui_mutex);return;
 }
 // School/room naming panels have a half-height header background. Extend
 // only its art and paired overlay; input text and keycaps keep their scale.
 if(picture&&hw==1920.f&&(hh==270.f||hh==540.f)&&
    ((!strcmp(pane_name,"P_Base_00")&&!strcmp(pn,"N_Dialog_00"))||
     (!strcmp(pane_name,"P_Base_01")&&!strcmp(pn,"P_Base_00")))){
  acnh_ui_record *c=ui_record(p,1);
  if(c){float sx=ui_float(p,0x48);if(!c->valid||sx!=c->last_sx)c->sx=sx;
   c->last_sx=c->sx*ex;ui_write(p,0x48,c->last_sx);c->valid=1;}
  pthread_mutex_unlock(&ui_mutex);return;
 }
 // DAL/visitor loading screen: anchor the two decorative pattern groups
 // to the wider edges. Translate each group outward; never stretch its
 // white trim, flowers, arrows or the separate centred airline logo.
 uintptr_t pattern_grand=*(uintptr_t*)(parent+0x18);
 uintptr_t pattern_great=pattern_grand?*(uintptr_t*)(pattern_grand+0x18):0;
 int dal_left=plain&&!strcmp(pane_name,"N_PatternL_00");
 int dal_right=plain&&!strcmp(pane_name,"N_PatternR_00");
 if((dal_left||dal_right)&&!strcmp((char*)(parent+0xb0),"N_Type_00")&&
    pattern_grand&&!strcmp((char*)(pattern_grand+0xb0),"N_InOut_00")&&pattern_great==top){
  acnh_ui_record *c=ui_record(p,ex>1.001f);
  float ps=1.f,rs=ui_parent_ratio(parent,top,0x48);
  if(c&&isfinite(ps)&&isfinite(rs)&&fabsf(ps)>1e-5f&&fabsf(rs)>1e-5f){
   float tx=ui_float(p,0x30);
   if(!c->valid||tx!=c->last_tx)c->tx=tx;
   c->last_tx=c->tx+(dal_right?1.f:-1.f)*root->width*.5f*(ex-1.f)*rs/ps;
   ui_write(p,0x30,c->last_tx);c->valid=1;
  }
  pthread_mutex_unlock(&ui_mutex);return;
 }
 // The job curtain's off-screen travel uses the original canvas units.
 // Compensate the whole curtain-only group so its mesh AND translation
 // follow the widened canvas together. Scaling just the root exposes the
 // parked edge earlier; changing keyframe time would hide that mismatch.
 if(plain&&!strcmp(pane_name,"N_JobWipe_00")&&parent==top){
  acnh_ui_record *c=ui_record(p,fabsf(k-1.f)>.001f);
  if(c){
   float sx=ui_float(p,0x48),sy=ui_float(p,0x4c);
   if(!c->valid||sx!=c->last_sx)c->sx=sx;
   if(!c->valid||sy!=c->last_sy)c->sy=sy;
   c->last_sx=c->sx*ex;c->last_sy=c->sy*ey;
   ui_write(p,0x48,c->last_sx);ui_write(p,0x4c,c->last_sy);c->valid=1;
  }
  pthread_mutex_unlock(&ui_mutex);return;
 }
 // The menu cursor receives an already transformed target position from
 // the selected item's world matrix. Converting it as ordinary root-local
 // coordinates applies the root compensation twice (drift toward centre).
 // Undo that second conversion for the direct-root cursor, not its animation
 // children; retain the normal icon scale and hand-tip animation offset.
 // PocketMenu's held-item layer is a sibling under N_Main_00. Like the
 // cursor, its position comes from a world-space slot, not an untouched
 // layout coordinate. Compensate that position once; its icon/animation
 // children keep their original scale and local offsets.
 int drag_target=pocket_drag&&!strcmp((const char*)(parent+0xb0),"N_Main_00")&&
  *(uintptr_t*)(parent+0x18)==top;
 // MenuRecipe puts the same world-targeted cursor under N_InOut_01,
 // unlike Storage's direct-root cursor. Its entry container still inherits
 // root scaling, so undo that extra conversion here as well. Retain the
 // container animation and cursor icon scale.
 int recipe_cursor=cursor&&!strcmp((char*)(parent+0xb0),"N_InOut_01")&&
  *(uintptr_t*)(parent+0x18)==top;
 if((cursor&&parent==top)||recipe_cursor||drag_target){
  acnh_ui_record *c=ui_record(p,fabsf(k-1.f)>.001f);
  if(c){
   float tx=ui_float(p,0x30),ty=ui_float(p,0x34);
   if(!c->valid||tx!=c->last_tx)c->tx=tx;
   if(!c->valid||ty!=c->last_ty)c->ty=ty;
   c->last_tx=c->tx*ex;c->last_ty=c->ty*ey;
   ui_write(p,0x30,c->last_tx);ui_write(p,0x34,c->last_ty);c->valid=1;
  }
  pthread_mutex_unlock(&ui_mutex);return;
 }
 // CmnSubWin measures N_Size's transformed centre, but subtracts its
 // unscaled half-width and requested gap before assigning a local offset.
 // Capture the centre relative to N_InOut (including its entry animation)
 // so that the mixed-space subtraction can be converted back once. The
 // mirrored N_Re branch gives the same local centre for either row edge.
 if(plain&&!strcmp(pane_name,"N_Size_00")&&
    !strcmp((const char*)(parent+0xb0),"N_All_00")){
  uintptr_t in=*(uintptr_t*)(parent+0x18);
  uintptr_t buttons=in?*(uintptr_t*)(in+0x18):0;
  uintptr_t out=buttons?*(uintptr_t*)(buttons+0x18):0;
  uintptr_t mirror=out?*(uintptr_t*)(out+0x18):0;
  uintptr_t type=mirror?*(uintptr_t*)(mirror+0x18):0;
  if(in&&buttons&&out&&mirror&&type&&
     !strcmp((char*)(in+0xb0),"N_In_00")&&
     !strncmp((char*)(buttons+0xb0),"N_BtnNum_",9)&&
     !strcmp((char*)(out+0xb0),"N_InOut_00")&&
     !strcmp((char*)(mirror+0xb0),"N_Re_00")&&
     !strcmp((char*)(type+0xb0),"N_TypeH_00")&&
     *(uintptr_t*)(type+0x18)==top){
   float scale=ui_float(out,0x70);
   if(isfinite(scale)&&fabsf(scale)>1e-5f){
    float x=ui_float(parent,0x7c)+ui_float(parent,0x70)*ui_float(p,0x30);
    float centre=(x-ui_float(out,0x7c))/scale;
    acnh_ui_record *c=ui_record(out,1);
    if(c&&isfinite(centre)&&centre>1.f)c->action_center=centre;
   }
  }
 }
 if(plain&&!strcmp(pane_name,"N_InOut_00")&&
    !strcmp((char*)(parent+0xb0),"N_Re_00")){
  uintptr_t type=*(uintptr_t*)(parent+0x18);
  if(type&&!strcmp((char*)(type+0xb0),"N_TypeH_00")&&
     *(uintptr_t*)(type+0x18)==top){
   acnh_ui_record *c=ui_record(p,0);
   if(c&&c->action_center>1.f){
    float tx=ui_float(p,0x30);
    if(!c->valid||tx!=c->last_tx){
     c->tx=tx;
     // A zero offset is the game's reset/no-gap case, not a measured gap.
     c->action_correction=fabsf(tx)>1e-5f?c->action_center*(1.f-1.f/ex):0.f;
    }
    c->last_tx=c->tx-c->action_correction;
    ui_write(p,0x30,c->last_tx);c->valid=1;
   }
   pthread_mutex_unlock(&ui_mutex);return;
  }
 }
 // LPocketBalloon draws its background capture and text separately.
 // P_Capture_00 is an intermediate capture: scaling it as well as the
 // final P_Cap_00 applies the correction twice. Text uses its own matrix,
 // so compensate it independently, preserving the original font resource.
 int balloon_replay=(picture||capture_picture)&&!strcmp(pane_name,"P_Cap_00");
 if(pocket_balloon&&(balloon_replay||balloon_text)){
  acnh_ui_record *c=ui_record(p,fabsf(k-1.f)>.001f);
  if(c){
   float sx=ui_float(p,0x48),sy=ui_float(p,0x4c),ty=ui_float(p,0x34);
   if(!c->valid||sx!=c->last_sx)c->sx=sx;
   if(!c->valid||sy!=c->last_sy)c->sy=sy;
   if(!c->valid||ty!=c->last_ty)c->ty=ty;
   c->last_sx=c->sx/ex;c->last_sy=c->sy/ey;
   ui_write(p,0x48,c->last_sx);ui_write(p,0x4c,c->last_sy);
   // LPocketBalloon places captured glyphs 1952 units below the visible
   // name (100-1952=-1852 in its original layout). The capture renderer
   // rebases that lane separately from the root transform. Restore the
   // lost lane distance in its source coordinates when extending vertically,
   // or tablet text lands below its bubble.
   c->last_ty=c->ty;
   if(balloon_text&&c->ty<-1000.f)c->last_ty+=1952.f*(1.f-1.f/ey);
   if(balloon_text)ui_write(p,0x34,c->last_ty);
   c->valid=1;
  }
  pthread_mutex_unlock(&ui_mutex);return;
 }
 if(picture||capture_picture){
  // BgMask is a 100x100 solid picture enlarged by the game at runtime.
  // Its screen-sized result must follow the canvas, not the source mesh size.
  if(parent==top&&!strcmp(pane_name,"P_Mask")&&
     ui_float(p,0x50)==100.f&&ui_float(p,0x54)==100.f){
   acnh_ui_record *c=ui_record(p,0);
   float sx=ui_float(p,0x48),sy=ui_float(p,0x4c);
   float bx=c&&c->valid&&sx==c->last_sx?c->sx:sx;
   float by=c&&c->valid&&sy==c->last_sy?c->sy:sy;
   if(fabsf(bx)*100>=root->width*.9f&&fabsf(by)*100>=root->height*.9f){
    if(!c)c=ui_record(p,fabsf(k-1.f)>.001f);
    if(c){c->sx=bx;c->sy=by;c->last_sx=bx*ex;c->last_sy=by*ey;
     ui_write(p,0x48,c->last_sx);ui_write(p,0x4c,c->last_sy);c->valid=1;}
   }
   pthread_mutex_unlock(&ui_mutex);return;
  }
  if(capture_picture){pthread_mutex_unlock(&ui_mutex);return;}
  float w=ui_float(p,0x50),h=ui_float(p,0x54),H=root->height;
  const char *name=pane_name;
  // The completed-home reveal shrinks the round aperture's dimensions.
  // In a wider viewport an unchanged aperture meets the new corners sooner.
  // Adapt only this mask (not its sibling logo/title), preserving keyframe
  // dimensions and time while keeping its normalized canvas coverage.
  uintptr_t demo_parent=*(uintptr_t*)(parent+0x18);
  int showcase_circle=demo_parent&&
   !strcmp((char*)(demo_parent+0xb0),"N_Demo1Wipe_00")&&
   ((!strcmp(name,"P_SirclePattern_00")&&!strcmp((char*)(parent+0xb0),"N_Demo_00"))||
    (!strcmp(name,"P_Circle_00")&&!strcmp((char*)(parent+0xb0),"N_Demo_01")));
  if(showcase_circle){
   acnh_ui_record *c=ui_record(p,fabsf(k-1.f)>.001f);
   if(c){
    float sx=ui_float(p,0x48),sy=ui_float(p,0x4c);
    if(!c->valid||sx!=c->last_sx)c->sx=sx;
    if(!c->valid||sy!=c->last_sy)c->sy=sy;
    c->last_sx=c->sx*ex;c->last_sy=c->sy*ey;
    ui_write(p,0x48,c->last_sx);ui_write(p,0x4c,c->last_sy);c->valid=1;
   }
   pthread_mutex_unlock(&ui_mutex);return;
  }
  // ThemeStart's title dimming is two gradient pictures, 270 and 300
  // high, not a full-screen BgMask. Runtime tracing identifies both under
  // N_null_00/N_OutS_00. The related Theme/facility titles share this family.
  acnh_ui_record *shade=ui_record(p,0);
  float shade_width=shade&&shade->valid&&w==shade->last_width?shade->width:w;
  int shade_family=0;uintptr_t shade_ancestor=parent;
  for(int depth=0;depth<4&&shade_ancestor;depth++){
   if(!strcmp((char*)(shade_ancestor+0xb0),"N_OutS_00"))shade_family=1;
   shade_ancestor=*(uintptr_t*)(shade_ancestor+0x18);
  }
  if(shade_family&&(!strcmp(name,"P_BGSh_00")||!strcmp(name,"P_BGSh_01"))&&
     fabsf(shade_width-1920.f)<.01f&&(fabsf(h-270.f)<.01f||fabsf(h-300.f)<.01f)){
   acnh_ui_record *c=shade?shade:ui_record(p,ex>1.001f);
   if(c){c->width=shade_width;c->last_width=shade_width*ex;
    ui_write(p,0x50,c->last_width);c->valid=1;}
   pthread_mutex_unlock(&ui_mutex);return;
  }
  unsigned origin=(*(uint8_t*)(p+0x5b))&3;
  int side=(!strncmp(name,"P_L_",4)&&origin==2)||(!strncmp(name,"P_R_",4)&&origin==1);
  if(side&&(!strncmp((const char*)(parent+0xb0),"P_Circle_",9)||
           !strcmp((const char*)(parent+0xb0),"P_SirclePattern_00"))&&isfinite(w)&&isfinite(h)&&w>=0&&h>0){
   acnh_ui_record *c=ui_record(p,fabsf(k-1.f)>.001f);
   float bw=c&&c->valid&&w==c->last_width?c->width:w;
   float ps=1.f,rs=ui_parent_ratio(parent,top,0x48);
   if(c&&isfinite(ps)&&isfinite(rs)&&fabsf(ps)>1e-5f&&fabsf(rs)>1e-5f){
    c->width=bw;c->last_width=bw+root->width*.5f*(ex-1.f)*fabsf(rs/ps);
    if(!c->valid||h!=c->last_height)c->height=h;
    c->last_height=c->height*ey;
    ui_write(p,0x50,c->last_width);ui_write(p,0x54,c->last_height);c->valid=1;
    pthread_mutex_unlock(&ui_mutex);return;
   }
  }
  // The matching top/bottom fill rectangles may start at zero height.
  // Extend their width independently from the round aperture's animation.
  int circle_parent=!strncmp((char*)(parent+0xb0),"P_Circle_",9)||
   !strcmp((char*)(parent+0xb0),"P_SirclePattern_00");
  acnh_ui_record *fill=ui_record(p,0);
  float fill_width=fill&&fill->valid&&w==fill->last_width?fill->width:w;
  if(circle_parent&&(!strncmp(name,"P_B_",4)||!strncmp(name,"P_T_",4))&&
     fabsf(fill_width-1980.f)<.01f&&h>=0){
   float ps=1.f,rs=ui_parent_ratio(parent,top,0x48);
   acnh_ui_record *c=fill?fill:ui_record(p,fabsf(k-1.f)>.001f);
   if(c&&isfinite(ps)&&isfinite(rs)&&fabsf(ps)>1e-5f&&fabsf(rs)>1e-5f){
    c->width=fill_width;c->last_width=fill_width+root->width*(ex-1.f)*fabsf(rs/ps);
    ui_write(p,0x50,c->last_width);c->valid=1;
   }
   pthread_mutex_unlock(&ui_mutex);return;
  }
  // Interior entry/exit wipes comprise eight adjacent 240-unit lanes.
  // Their 241/242-wide, 1301-high pictures cover the canvas with a one-unit
  // overlap. Compensating the UI root without these lanes leaves black sides.
  // Extend mesh widths and lane centres, retaining vertical animation and
  // the separate P_Icon. Hotel layouts also nest a second picture per lane.
  acnh_ui_record *wipe=ui_record(p,0);
  float wipe_width=wipe&&wipe->interior_wipe&&w==wipe->last_width?wipe->width:w;
  uintptr_t wipe_group=*(uintptr_t*)(parent+0x18);
  int wipe_name=!strncmp(name,"P_Base_",7)&&strlen(name)==9&&
   name[7]>='0'&&name[7]<='1'&&name[8]>='0'&&name[8]<='9'&&
   (name[7]=='0'||name[8]<='5');
  int wipe_parent=!strcmp((char*)(parent+0xb0),"N_All_00")||
   (!strcmp((char*)(parent+0xb0),"N_Type_00")&&wipe_group&&
    !strcmp((char*)(wipe_group+0xb0),"N_All_00"));
  acnh_ui_record *wipe_parent_record=ui_record(parent,0);
  int wipe_child=wipe_parent_record&&wipe_parent_record->interior_wipe;
  if(wipe_name&&(wipe_parent||wipe_child)&&fabsf(h-1301.f)<.01f&&
     (fabsf(wipe_width-241.f)<.01f||fabsf(wipe_width-242.f)<.01f)){
   acnh_ui_record *c=wipe?wipe:ui_record(p,ex>1.001f);
   if(c){
    float tx=ui_float(p,0x30);
    if(!c->interior_wipe||w!=c->last_width)c->width=w;
    if(!c->interior_wipe||tx!=c->last_tx)c->tx=tx;
    c->last_width=c->width*ex;c->last_tx=c->tx*ex;
    ui_write(p,0x50,c->last_width);ui_write(p,0x30,c->last_tx);
    c->interior_wipe=1;c->valid=1;
   }
   pthread_mutex_unlock(&ui_mutex);return;
  }
  // Storage's decorative header is only 1920x290, not a full-screen image.
  // Extend its mesh sideways and move it to the expanded top edge. Do not
  // scale the surrounding category controls, item grid or text.
  acnh_ui_record *header=ui_record(p,0);
  uintptr_t grand=*(uintptr_t*)(parent+0x18);
  int storage_header=!strcmp(name,"P_Header_00")&&
   !strcmp((const char*)(parent+0xb0),"N_InOut_01")&&grand&&
   !strcmp((const char*)(grand+0xb0),"N_Root_00")&&
   ((fabsf(w-1920.f)<.01f&&fabsf(h-290.f)<.01f)||(header&&header->storage_header));
  if(storage_header){
   acnh_ui_record *c=header?header:ui_record(p,fabsf(k-1.f)>.001f);
   float ps=1.f,rs=ui_parent_ratio(parent,top,0x4c),ty=ui_float(p,0x34);
   if(c&&isfinite(ps)&&isfinite(rs)&&fabsf(ps)>1e-5f&&fabsf(rs)>1e-5f){
    if(!c->storage_header||w!=c->last_width)c->width=w;
    if(!c->storage_header||ty!=c->last_ty)c->ty=ty;
    c->last_width=c->width*ex;
    c->last_ty=c->ty+root->height*.5f*(ey-1.f)*rs/ps;
    ui_write(p,0x50,c->last_width);ui_write(p,0x34,c->last_ty);
    c->storage_header=1;c->valid=1;
    pthread_mutex_unlock(&ui_mutex);return;
   }
  }
  // OrderList's frame pictures are decorative bands; each nested picture
  // owns its own geometry. Widen both meshes, leaving the item/text siblings.
  acnh_ui_record *frame=ui_record(p,0);
  float frame_width=frame&&frame->valid&&w==frame->last_width?frame->width:w;
  const char *frame_parent=(const char*)(parent+0xb0);
  uintptr_t frame_grand=*(uintptr_t*)(parent+0x18);
  uintptr_t frame_great=frame_grand?*(uintptr_t*)(frame_grand+0x18):0;
  int frame_name=!strncmp(name,"P_FrameU_",9)||!strncmp(name,"P_FrameD_",9);
  int frame_direct=frame_grand&&!strcmp((char*)(frame_grand+0xb0),"N_Position_00")&&
   !strncmp(frame_parent,"N_InOut_",8);
  int frame_nested=frame_great&&!strcmp((char*)(frame_great+0xb0),"N_Position_00")&&
   !strncmp((char*)(frame_grand+0xb0),"N_InOut_",8)&&
   (!strncmp(frame_parent,"P_FrameU_",9)||!strncmp(frame_parent,"P_FrameD_",9));
  int order_frame=frame_name&&(frame_direct||frame_nested)&&
   fabsf(frame_width-1920.f)<.01f&&(fabsf(h-80.f)<.01f||fabsf(h-180.f)<.01f);
  int order_gradient=!strcmp(name,"P_Grad_00")&&!strcmp(frame_parent,"N_Position_00")&&
   fabsf(frame_width-2000.f)<.01f&&fabsf(h-600.f)<.01f;
  if(order_frame||order_gradient){
   acnh_ui_record *c=frame?frame:ui_record(p,ex>1.001f);
   if(c){c->width=frame_width;c->last_width=frame_width*ex;
    ui_write(p,0x50,c->last_width);c->valid=1;}
   pthread_mutex_unlock(&ui_mutex);return;
  }
  // Catalog / Nook Miles / ABD decorate their top and bottom with
  // screen-wide P_pict bands (320/430/500 high) under N_InOut groups.
  // Widen the background meshes only; do not stretch menu contents.
  acnh_ui_record *band=ui_record(p,0);
  float band_width=band&&band->valid&&w==band->last_width?band->width:w;
  const char *band_parent=(const char*)(parent+0xb0);
  uintptr_t band_grand=*(uintptr_t*)(parent+0x18);
  int pict_name=!strncmp(name,"P_pict_",7)&&strlen(name)==9&&
   name[7]>='0'&&name[7]<='9'&&name[8]>='0'&&name[8]<='9';
  int bg_name=!strncmp(name,"P_BG_",5)&&strlen(name)==7&&
   name[5]>='0'&&name[5]<='9'&&name[6]>='0'&&name[6]<='9';
  // Catalog/NatteeCatalog exit curtains have a 320-high outer picture
  // and a 500/430-high child. Width is mesh geometry, not child scale:
  // widening the parent alone leaves the animated child at 16:9.
  int direct_band=!strncmp(band_parent,"N_InOut_",8);
  acnh_ui_record *parent_band=ui_record(parent,0);
  float parent_width=ui_float(parent,0x50);
  if(parent_band&&parent_band->valid&&parent_width==parent_band->last_width)parent_width=parent_band->width;
  int nested_band=band_grand&&!strncmp((const char*)(band_grand+0xb0),"N_InOut_",8)&&
   (!strncmp(band_parent,"P_pict_",7)||!strncmp(band_parent,"P_BG_",5))&&
   fabsf(parent_width-1920.f)<.01f&&fabsf(ui_float(parent,0x54)-320.f)<.01f;
  int terminal_band=(pict_name||(bg_name&&fabsf(h-320.f)<.01f))&&
   (direct_band||(pict_name&&nested_band&&(fabsf(h-430.f)<.01f||fabsf(h-500.f)<.01f)))&&
   fabsf(band_width-1920.f)<.01f&&
   (fabsf(h-320.f)<.01f||fabsf(h-430.f)<.01f||fabsf(h-500.f)<.01f);
  if(terminal_band){
   acnh_ui_record *c=band?band:ui_record(p,ex>1.001f);
   if(c){c->width=band_width;c->last_width=band_width*ex;ui_write(p,0x50,c->last_width);c->valid=1;}
   pthread_mutex_unlock(&ui_mutex);return;
  }
  // Additional shipped full-width decorative meshes. Scope by exact path
  // and original bounds; change geometry only, never child scale, opacity,
  // capture masks, text or keyboard keycaps. Height and animation stay intact.
  acnh_ui_record *strip=ui_record(p,0);
  float strip_width=strip&&strip->valid&&w==strip->last_width?strip->width:w;
  static const struct {const char *path;float width,height;} strips[]={
   {"RootPane/N_InOut_02/P_MatchingModeBase_00",1920.0f,320.0f},
   {"RootPane/N_InOut_01/P_Base_00",1920.0f,250.0f},
   {"RootPane/N_InOut_01/P_Base_01",1920.0f,250.0f},
   {"RootPane/N_FinishOut_00/N_TextInOut_00/N_TextType_01/P_BGShadow_00",1920.0f,540.0f},
   {"RootPane/N_FinishOut_00/N_TextInOut_00/N_TextType_01/P_BGgradation_00",1920.0f,443.3333435058594f},
   {"RootPane/N_Root_00/N_Pos_00/N_Bg/P_Bg_Black",1920.0f,108.0f},
   {"RootPane/N_Root_00/N_Pos_00/N_Bg/P_Bg_Gray",1920.0f,108.0f},
   {"RootPane/N_InOut_00/P_Base_00",1940.0f,320.0f},
   {"RootPane/N_InOut_00/P_Base_01",1940.0f,360.0f},
   {"RootPane/P_Base_00",1920.0f,600.0f},
   {"RootPane/N_InOut_00/P_Bg_00",1920.0f,700.0f},
   {"RootPane/N_InOut_00/P_Base_00",1920.0f,540.0f},
   {"RootPane/N_InOut_02/P_Bg_01",1920.0f,700.0f},
   {"RootPane/N_InOut_00/P_Base_00",1920.0f,200.0f},
   {"RootPane/N_InOut_00/P_Base_01",1920.0f,200.0f},
  };
  for(unsigned i=0;i<sizeof strips/sizeof *strips;i++){
   if(fabsf(strip_width-strips[i].width)<.01f&&fabsf(h-strips[i].height)<.01f&&ui_path(p,strips[i].path)){
    acnh_ui_record *c=strip?strip:ui_record(p,ex>1.001f);
    if(c){c->width=strip_width;c->last_width=strip_width*ex;
     ui_write(p,0x50,c->last_width);c->valid=1;}
    pthread_mutex_unlock(&ui_mutex);return;
   }
  }
  // Full-screen backgrounds get the inverse compensation, so the game image is never squeezed again.
  // Small map/phone/item panes are deliberately excluded.
  // Cast selection and its wipe deliberately overscan to 2000x1200,
  // rather than 1920x1080. They are background art, not a 3D character pane.
  // Preserve the existing size/type checks and identify these layout paths.
  const char *cover_parent=(const char*)(parent+0xb0);
  int cast_background=fabsf(w-2000.f)<.01f&&fabsf(h-1200.f)<.01f&&
   (((!strcmp(name,"P_Base_00")||!strcmp(name,"P_MatchingBg_00"))&&
     !strcmp(cover_parent,"N_InOut_00"))||
    (!strcmp(name,"P_Base_00")&&!strcmp(cover_parent,"N_Bg_00"))||
    (!strcmp(name,"P_Base_03")&&!strcmp(cover_parent,"N_BG_00")));
  int cover=w>=root->width*.9f&&h>=H*.9f&&w<root->width*1.4f&&h<H*1.4f&&
   (fabsf(w/h-16.f/9.f)<.06f||cast_background);
  acnh_ui_record *c=ui_record(p,cover);if(c&&cover){
   float sx=ui_float(p,0x48),sy=ui_float(p,0x4c);
   if(!c->valid||sx!=c->last_sx)c->sx=sx;
   if(!c->valid||sy!=c->last_sy)c->sy=sy;
   c->last_sx=c->sx*ex;c->last_sy=c->sy*ey;
   ui_write(p,0x48,c->last_sx);ui_write(p,0x4c,c->last_sy);c->valid=1;
  }
 }else if((plain||parts)&&!nested){
  float rootScale=ui_float(top,0x70),ps=ui_float(parent,0x70);
  if(fabsf(rootScale)>1e-5f&&fabsf(ps)>1e-5f){
   acnh_ui_record *c=ui_record(p,0);float tx=ui_float(p,0x30),ty=ui_float(p,0x34);
   float bx=c&&c->valid&&tx==c->last_tx?c->tx:tx,by=c&&c->valid&&ty==c->last_ty?c->ty:ty;
   float gx=(ui_float(parent,0x7c)-ui_float(top,0x7c)+ps*bx)/rootScale;
   float W=root->width*.5f;
   int load=!strncmp((char*)(p+0xb0),"L_Island_",9)||!strncmp((char*)(p+0xb0),"L_Tips_",7);
   // Ordinary menu groups already use the widened root anchors. Moving them
   // again makes independently anchored B/A guides overlap. Only loading
   // groups require the additional explicit safe-margin adjustment.
   int edge=load&&fabsf(gx)>W*.72f;
   if(edge||c){
    if(!c)c=ui_record(p,1);
    if(c){c->tx=bx;c->ty=by;unsigned anchor=(*(uint8_t*)(p+0x5b)>>4)&3;
     float extra=(k-1)*W,inset=load?64.f:36.f;
     float shift=edge?(anchor? -copysignf(fminf(inset,extra),gx):copysignf(fmaxf(0,extra-inset),gx)):0;
     c->last_tx=bx+shift*rootScale/ps;c->last_ty=by;
     if(load&&k>1.001f)c->last_ty=by+20.f;
     ui_write(p,0x30,c->last_tx);ui_write(p,0x34,c->last_ty);c->shifted=edge&&k>1.001f;c->valid=1;
    }
   }
  }
 }
 pthread_mutex_unlock(&ui_mutex);
}
