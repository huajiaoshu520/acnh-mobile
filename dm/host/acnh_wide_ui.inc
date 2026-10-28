// UI-only layout adaptation; derives from the documented nn::ui2d pane ABI.
// ACNHRecomp reference (MIT notice retained with this build).
typedef struct {
 uintptr_t key,vtable;int root,valid,shifted;
 float sx,last_sx,width,last_width,tx,last_tx,ty,last_ty;
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
 float k=wanted()/(16.f/9.f);uintptr_t vt=*(uintptr_t*)p-base,parent=*(uintptr_t*)(p+0x18);
 int plain=vt==0x474d1f8||vt==0x4a85590,parts=vt==0x474d310;
 int picture=vt==0x474d418||vt==0x474de58||vt==0x4a87220;
 if(parent&&!plain&&!parts&&!picture)return;
 pthread_mutex_lock(&ui_mutex);
 if(!parent){
  acnh_ui_record *c=ui_record(p,k>1.001f);if(!c){pthread_mutex_unlock(&ui_mutex);return;}
  float w=ui_float(p,0x50),h=ui_float(p,0x54),sx=ui_float(p,0x48);
  if(!c->valid||w!=c->last_width)c->width=w;
  if(!c->valid||sx!=c->last_sx)c->sx=sx;
  if(!isfinite(h)||h<1||c->width<1279||fabsf(c->width/h-16.f/9.f)>.02f){pthread_mutex_unlock(&ui_mutex);return;}
  c->root=1;c->last_width=c->width*k;c->last_sx=c->sx/k;
  ui_write(p,0x50,c->last_width);ui_write(p,0x48,c->last_sx);
  if(!c->valid)LOG("[ACNH UI] root %.24s %.0fx%.0f rootScaleX %.4f; UI aspect compensation %.4f",(char*)(p+0xb0),c->width,h,c->sx,1.f/k);
  c->valid=1;pthread_mutex_unlock(&ui_mutex);return;
 }
 uintptr_t top=parent;int nested=0;
 for(int n=0;n<32;n++){
  acnh_ui_record *a=ui_record(top,0);if(a&&a->shifted)nested=1;
  uintptr_t next=*(uintptr_t*)(top+0x18);if(!next)break;top=next;
 }
 acnh_ui_record *root=ui_record(top,0);if(!root||!root->root){pthread_mutex_unlock(&ui_mutex);return;}
 if(picture){
  float w=ui_float(p,0x50),h=ui_float(p,0x54),H=ui_float(top,0x54);
  // WipeCircle uses two inner-edge-anchored black side panels rather than
  // one full-screen picture. Extend only their outer edges; the circle,
  // animation and its aspect must remain unchanged.
  const char *name=(const char*)(p+0xb0);
  unsigned origin=(*(uint8_t*)(p+0x5b))&3;
  int side=(!strncmp(name,"P_L_",4)&&origin==2)||(!strncmp(name,"P_R_",4)&&origin==1);
  if(side&&!strncmp((const char*)(parent+0xb0),"P_Circle_",9)&&isfinite(w)&&isfinite(h)&&w>=0&&h>0){
   acnh_ui_record *c=ui_record(p,k>1.001f);
   float bw=c&&c->valid&&w==c->last_width?c->width:w;
   float ps=ui_float(parent,0x70),rs=ui_float(top,0x70);
   if(c&&isfinite(ps)&&isfinite(rs)&&fabsf(ps)>1e-5f){
    c->width=bw;c->last_width=bw+root->width*.5f*(k-1.f)*fabsf(rs/ps);
    ui_write(p,0x50,c->last_width);c->valid=1;
    pthread_mutex_unlock(&ui_mutex);return;
   }
  }
  // Full-screen backgrounds get the inverse compensation, so the game image is never squeezed again.
  // Small map/phone/item panes are deliberately excluded.
  int cover=w>=root->width*.9f&&h>=H*.9f&&w<root->width*1.4f&&h<H*1.4f&&fabsf(w/h-16.f/9.f)<.06f;
  acnh_ui_record *c=ui_record(p,cover);if(c&&cover){float sx=ui_float(p,0x48);if(!c->valid||sx!=c->last_sx)c->sx=sx;c->last_sx=c->sx*k;ui_write(p,0x48,c->last_sx);c->valid=1;}
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
