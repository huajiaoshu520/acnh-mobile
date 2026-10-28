// UI-only layout adaptation; derives from the documented nn::ui2d pane ABI.
// ACNHRecomp reference (MIT notice retained with this build).
typedef struct {
 uintptr_t key,vtable;int root,valid,shifted,storage_header;
 float sx,last_sx,width,last_width,tx,last_tx,ty,last_ty;
 float sy,last_sy,height,last_height;
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
 int cursor=vt==0x4a8f128&&!strcmp((const char*)(p+0xb0),"L_Cursor_00");
 if(parent&&!plain&&!parts&&!picture&&!cursor)return;
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
  if(*(uintptr_t*)top==base+0x4a8f128&&!strcmp((const char*)(top+0xb0),"L_ItemBalloon_00"))pocket_balloon=1;
  uintptr_t next=*(uintptr_t*)(top+0x18);if(!next)break;top=next;
 }
 acnh_ui_record *root=ui_record(top,0);if(!root||!root->root){pthread_mutex_unlock(&ui_mutex);return;}
 // The menu cursor receives an already transformed target position from
 // the selected item's world matrix. Converting it as ordinary root-local
 // coordinates applies the root compensation twice (drift toward centre).
 // Undo that second conversion for the direct-root cursor, not its animation
 // children; retain the normal icon scale and hand-tip animation offset.
 if(cursor&&parent==top){
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
 if(picture){
  float w=ui_float(p,0x50),h=ui_float(p,0x54),H=root->height;
  // WipeCircle uses two inner-edge-anchored black side panels rather than
  // one full-screen picture. Extend only their outer edges; the circle,
  // animation and its aspect must remain unchanged.
  const char *name=(const char*)(p+0xb0);
  // LPocketBalloon replays a 740x148 offscreen capture through a parts
  // pane whose world matrix deliberately ignores the menu root's scale.
  // Apply the missing tablet Y compensation to the replay only. Scaling
  // its text/capture source as well would apply the correction twice.
  int replay=!strcmp(name,"P_Cap_00")||
   (!strcmp(name,"P_Capture_00")&&!strcmp((const char*)(parent+0xb0),"N_Capture_01"));
  if(pocket_balloon&&replay&&fabsf(w-740.f)<.01f&&fabsf(h-148.f)<.01f){
   acnh_ui_record *c=ui_record(p,ey>1.001f);
   if(c){
    float sy=ui_float(p,0x4c);
    if(!c->valid||sy!=c->last_sy)c->sy=sy;
    c->last_sy=c->sy/ey;ui_write(p,0x4c,c->last_sy);c->valid=1;
   }
   pthread_mutex_unlock(&ui_mutex);return;
  }
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
  int terminal_band=!strncmp(name,"P_pict_",7)&&strlen(name)==9&&
   name[7]>='0'&&name[7]<='9'&&name[8]>='0'&&name[8]<='9'&&
   !strncmp((const char*)(parent+0xb0),"N_InOut_",8)&&
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
