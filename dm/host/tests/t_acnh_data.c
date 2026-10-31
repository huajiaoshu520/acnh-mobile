#define MIN(a,b) ((a)<(b)?(a):(b))
#include "../hle_fs.c"
struct dm_flags g_flag;uint64_t g_t0_ns;static module_t modules[1];module_t *g_mod=modules;int g_nmod;
int dm_custom_lang_explicit(void){return -1;}int dm_sys_lang(void){return 10;}
uint64_t now_ns(void){return clock_gettime_nsec_np(CLOCK_MONOTONIC);}
void dm_prof_add(int a,uint64_t b,uint64_t c){(void)a;(void)b;(void)c;}void dm_guest_write_prepare(void*p,size_t n){(void)p;(void)n;}
static int failures;
#define CHECK(s,c) do{int ok=!!(c);printf("%s: %s\n",s,ok?"PASS":"FAIL");failures+=!ok;}while(0)
static void *reader(void *opaque){(void)opaque;for(int i=0;i<200;i++){uint64_t h;size_t got;char b[8];if(h_fs_OpenFile(&h,"content:/Dir/hello",1))return (void*)1;if(h_fs_ReadFile(&got,h,1,b,3)||got!=3||memcmp(b,"ell",3)){h_fs_CloseFile(h);return (void*)1;}h_fs_CloseFile(h);}return NULL;}
int main(int argc,char**argv){if(argc<2)return 2;char why[256];CHECK("deep resource verification",dm_acnh_data_verify(argv[1],1,NULL,why,sizeof why));if(failures){puts(why);return 1;}setenv("DM_ROMFS",argv[1],1);CHECK("mount .data without a romfs directory",h_fs_MountRom("content",NULL,0)==0);if(failures)return 1;
 uint64_t h=0;size_t got=0;int64_t size=0;char b[64]={0};int32_t type=-1;
 if(argc==2){
 CHECK("root is a virtual directory",!h_fs_GetEntryType(&type,"content:/")&&type==ENT_DIR);
 CHECK("normalize repeated separators",!h_fs_GetEntryType(&type,"content://Dir/./hello")&&type==ENT_FILE);
 CHECK("open indexed file",!h_fs_OpenFile(&h,"content:/Dir/hello",1));
 CHECK("logical size excludes adjacent data",!h_fs_GetFileSize(&size,h)&&size==5);
 CHECK("partial read at nonzero offset",!h_fs_ReadFile(&got,h,1,b,3)&&got==3&&!memcmp(b,"ell",3));
 memset(b,0x5a,sizeof b);CHECK("read clips at file boundary",!h_fs_ReadFile(&got,h,3,b,40)&&got==2&&!memcmp(b,"lo",2)&&b[2]==0x5a);
 CHECK("EOF does not leak next packed file",!h_fs_ReadFile(&got,h,5,b,8)&&got==0);
 CHECK("negative offset fails",h_fs_ReadFile(&got,h,-1,b,1)!=0);
 CHECK("read-only data rejects write",h_fs_WriteFile(h,0,"x",1,NULL)!=0);h_fs_CloseFile(h);
 CHECK("traversal rejected",h_fs_OpenFile(&h,"content:/Dir/../hello",1)!=0);
 CHECK("write open rejected",h_fs_OpenFile(&h,"content:/Dir/hello",2)!=0);
 CHECK("directory cannot be opened as a file",h_fs_OpenFile(&h,"content:/Dir",1)!=0);
 uint64_t d=0;DirectoryEntry e[8];int64_t count=0;
 CHECK("open virtual root directory",!h_fs_OpenDirectory(&d,"content:/",3));CHECK("root entry count",!h_fs_DirCount(&count,d)&&count==3);
 CHECK("enumerate root once",!h_fs_ReadDirectory(&count,e,d,8)&&count==3);CHECK("directory read position advances",!h_fs_ReadDirectory(&count,e,d,8)&&count==0);h_fs_CloseDirectory(d);
 CHECK("file-only filter",!h_fs_OpenDirectory(&d,"content:/Dir",2)&&!h_fs_DirCount(&count,d)&&count==3);h_fs_CloseDirectory(d);
 CHECK("directory-only filter",!h_fs_OpenDirectory(&d,"content:/Dir",1)&&!h_fs_DirCount(&count,d)&&count==1);h_fs_CloseDirectory(d);
 CHECK("empty file is readable",!h_fs_OpenFile(&h,"content:/Dir/empty",1)&&!h_fs_GetFileSize(&size,h)&&size==0&&!h_fs_ReadFile(&got,h,0,b,8)&&got==0);h_fs_CloseFile(h);
 CHECK("UTF8 file name",!h_fs_OpenFile(&h,"content:/Dir/中文",1)&&!h_fs_ReadFile(&got,h,0,b,3)&&got==3&&!memcmp(b,"abc",3));h_fs_CloseFile(h);
 pthread_t threads[8];for(int i=0;i<8;i++)pthread_create(&threads[i],NULL,reader,NULL);int bad=0;for(int i=0;i<8;i++){void*r;pthread_join(threads[i],&r);bad+=r!=NULL;}CHECK("parallel opens and preads keep independent file slices",bad==0);
 char absent[1200];snprintf(absent,sizeof absent,"%s/romfs",argv[1]);CHECK("no resource tree created",access(absent,F_OK)!=0);snprintf(absent,sizeof absent,"%s/.acnh-import-staging",argv[1]);CHECK("no extraction staging created",access(absent,F_OK)!=0);
 }else{
 acnh_pack*p=atomic_load(&g_acnh_pack);unsigned char*buf=malloc(1024*1024);size_t checked=0;
 for(size_t i=0;i<p->nfiles;i++){
  const acnh_file*f=&p->files[i];char path[1100];snprintf(path,sizeof path,"content:/%s",p->strings+f->path);if(h_fs_OpenFile(&h,path,1)||h_fs_GetFileSize(&size,h)||size!=(int64_t)f->size)return 3;
  CC_SHA256_CTX ctx;CC_SHA256_Init(&ctx);uint64_t off=0;while(off<f->size){size_t n=(size_t)MIN(f->size-off,1024*1024);if(h_fs_ReadFile(&got,h,off,buf,n)||got!=n)return 4;CC_SHA256_Update(&ctx,buf,(CC_LONG)n);off+=n;}
  uint8_t hash[32];CC_SHA256_Final(hash,&ctx);if(memcmp(hash,f->sha256,32)){printf("SHA mismatch %s\n",path);return 5;}h_fs_CloseFile(h);checked++;
 }
 printf("full .data HLE read verified: %zu files\n",checked);free(buf);
 }
 printf("failures=%d\n",failures);return !!failures;
}

int dm_guest_as_contains(uintptr_t address){(void)address;return 0;}
// Generic system mounts are outside the .data fixture.
int dm_main_imports(const char *prefix){(void)prefix;return 0;}
int dm_mii_standin(const char *dir){(void)dir;return 0;}
