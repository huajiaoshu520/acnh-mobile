// Exercise production program combination lookup after its 64-slot ring wraps.
#include "../nvn/nvn_mtl.m"
void *front_metal_layer(void) { return NULL; }
module_t *module_of(const void *p) { (void)p; return NULL; }
module_t *g_mod; int g_nmod;
void svc_thread_dump(void) {}
uint64_t now_ns(void) { return bp_now(); }
int main(void) { @autoreleasepool {
    mtl_init();
    static prog_t vertex, fragments[96];
    vertex.magic = MAGIC('P','R','O','G'); vertex.serial = nvn_new_serial();
    NvProg *vs = [NvProg new]; vs->sh[0] = [NvShader new]; strcpy(vs->sh[0]->key,"restore-vertex"); serial_put(vertex.serial,vs);
    for(int i=0;i<96;i++) { fragments[i].magic=MAGIC('P','R','O','G');fragments[i].serial=nvn_new_serial();NvProg *fs=[NvProg new];fs->sh[1]=[NvShader new];snprintf(fs->sh[1]->key,24,"fragment-%d",i);serial_put(fragments[i].serial,fs); }
    S.prog=bind_stages(&vertex,1);S.prog=bind_stages(&fragments[0],2);
    static typeof(S) saved;memcpy(&saved,&S,sizeof S);
    for(int i=1;i<96;i++)S.prog=bind_stages(&fragments[i],2);
    memcpy(&S,&saved,sizeof S);args_dirty_all();g_pso_dirty=0;prog_resolve();
    NvProg *actual=prog_get(S.prog),*expected=prog_get(&fragments[0]);
    int failed=!actual || actual->sh[0]!=vs->sh[0] || actual->sh[1]!=expected->sh[1] || !g_pso_dirty;
    const prog_t *resolved=S.prog;g_pso_dirty=0;prog_resolve();failed+=S.prog!=resolved || g_pso_dirty;
    S.bst[0]=S.bst[1]=&vertex;prog_resolve();failed+=S.prog!=&vertex;
    printf("program restore: wrapped-ring recovery, stable cache and unified program: %d failed\n",failed);return failed!=0;
} }
