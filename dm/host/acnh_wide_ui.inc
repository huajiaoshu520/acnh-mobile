// UI-only layout adaptation; derives from the documented nn::ui2d pane ABI.
// ACNHRecomp reference (MIT notice retained with this build).
typedef struct {
 uintptr_t key,vtable;int root,valid,shifted,storage_header;
 float sx,last_sx,width,last_width,tx,last_tx,ty,last_ty;
 float sy,last_sy,height,last_height;
 float action_center,action_correction;
} acnh_ui_record;
static acnh_ui_record ui_records[4096];
static pthread_mutex_t ui_mutex=PTHREAD_MUTEX_INITIALIZER;
static acnh_ui_record *ui_record(uintptr_t p,int create) {
 unsigned slot=(unsigned)((p>>4)*2654435761u)&4095;
 for(unsigned n=0;n<4096;n++,slot=(slot+1)&4095){
  acnh_ui_record *c=&ui_records[slot];if(c->key==p)return c;
  if(!c->key){if(create)c->key=p;return create?c:NULL;}
 }return NULL;
}
static float ui_float(uintptr_t p,int off){return fget((void*)(p+off));}
static void ui_write(uintptr_t p,int off,float v){if(ui_float(p,off)!=v){fput((void*)(p+off),v);*(uint8_t*)(p+0x58)|=0x10;}}
static void ui_pane(uint64_t *r,int param) {
 (void)param;static uintptr_t base; if(!base)base=main_base();uintptr_t p=r[0];if(!base||!p)return;
 float k=wanted()/(16.f/9.f),ex=fmaxf(1.f,k),ey=fmaxf(1.f,1.f/k);uintptr_t vt=*(uintptr_t*)p-base,parent=*(uintptr_t*)(p+0x18);
 int plain=vt==0x474d1f8||vt==0x4a85590,parts=vt==0x474d310;
 int picture=vt==0x474d418||vt==0x474de58||vt==0x4a87220;
 // Capture replays use their own Picture subclass in the live game.
 // Keep this type scoped to name balloons; it also draws unrelated UI captures.
 int capture_picture=vt==0x4a86968;
 const char *pane_name=(const char*)(p+0xb0);
 int balloon_text=parent&&!strcmp(pane_name,"T_Name_00")&&
  !strcmp((const char*)(parent+0xb0),"N_Rotate_00");
 int cursor=vt==0x4a8f128&&!strcmp((const char*)(p+0xb0),"L_Cursor_00");
 int pocket_drag=(parts||vt==0x4a8f128)&&!strcmp((const char*)(p+0xb0),"L_DragBtn_00");
 if(parent&&!plain&&!parts&&!picture&&!capture_picture&&!balloon_text&&!cursor&&!pocket_drag)return;
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
  if(capture_picture){pthread_mutex_unlock(&ui_mutex);return;}
  float w=ui_float(p,0x50),h=ui_float(p,0x54),H=root->height;
  const char *name=pane_name;
  unsigned origin=(*(uint8_t*)(p+0x5b))&3;
  int side=(!strncmp(name,"P_L_",4)&&origin==2)||(!strncmp(name,"P_R_",4)&&origin==1);
  if(side&&!strncmp((const char*)(parent+0xb0),"P_Circle_",9)&&isfinite(w)&&isfinite(h)&&w>=0&&h>0){
   acnh_ui_record *c=ui_record(p,fabsf(k-1.f)>.001f);
   float bw=c&&c->valid&&w==c->last_width?c->width:w;
   float ps=ui_float(parent,0x70),rs=ui_float(top,0x70);
   if(c&&isfinite(ps)&&isfinite(rs)&&fabsf(ps)>1e-5f){
    c->width=bw;c->last_width=bw+root->width*.5f*(ex-1.f)*fabsf(rs/ps);
    if(!c->valid||h!=c->last_height)c->height=h;
    c->last_height=c->height*ey;
    ui_write(p,0x50,c->last_width);ui_write(p,0x54,c->last_height);c->valid=1;
    pthread_mutex_unlock(&ui_mutex);return;
   }
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
   float ps=ui_float(parent,0x84),rs=ui_float(top,0x84),ty=ui_float(p,0x34);
   if(c&&isfinite(ps)&&isfinite(rs)&&fabsf(ps)>1e-5f){
    if(!c->storage_header||w!=c->last_width)c->width=w;
    if(!c->storage_header||ty!=c->last_ty)c->ty=ty;
    c->last_width=c->width*ex;
    c->last_ty=c->ty+root->height*.5f*(ey-1.f)*rs/ps;
    ui_write(p,0x50,c->last_width);ui_write(p,0x34,c->last_ty);
    c->storage_header=1;c->valid=1;
    pthread_mutex_unlock(&ui_mutex);return;
   }
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
  // Full-screen backgrounds get the inverse compensation, so the game image is never squeezed again.
  // Small map/phone/item panes are deliberately excluded.
  int cover=w>=root->width*.9f&&h>=H*.9f&&w<root->width*1.4f&&h<H*1.4f&&fabsf(w/h-16.f/9.f)<.06f;
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
