#include "lan_history.h"
#include "lan_room.h"
#include <CommonCrypto/CommonDigest.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
static pthread_mutex_t hist_lock=PTHREAD_MUTEX_INITIALIZER;
static const unsigned char key_magic[8]={'A','C','L','H','I','S','T','1'};
typedef struct {unsigned char magic[8];uint64_t time;unsigned char uid[16],key[64],mine[72],peer[72];} hist_record;
_Static_assert(sizeof(hist_record)==240,"local history record size");
static int history_dir(char *out,size_t size){
 const char *save=getenv("DM_SAVE");if(!save||save[0]!='/')return -1;
 size_t n=strlen(save);while(n>1&&save[n-1]=='/')n--;while(n>1&&save[n-1]!='/')n--;
 if(n<=1||n+sizeof("ACNHLocalNetwork")>size)return -1;
 memcpy(out,save,n);memcpy(out+n,"ACNHLocalNetwork",sizeof("ACNHLocalNetwork"));
 if(mkdir(out,0700)<0&&errno!=EEXIST)return -1;
 struct stat st;if(lstat(out,&st)||!S_ISDIR(st.st_mode))return -1;return 0;
}
static int write_all(int fd,const void *ptr,size_t n){const unsigned char *p=ptr;while(n){ssize_t w=write(fd,p,n);if(w<0&&errno==EINTR)continue;if(w<=0)return -1;p+=w;n-=(size_t)w;}return 0;}
static int identity(unsigned char out[16]){
 char dir[PATH_MAX],path[PATH_MAX];if(history_dir(dir,sizeof dir))return -1;
 if(snprintf(path,sizeof path,"%s/identity.bin",dir)>=(int)sizeof path)return -1;
 int fd=open(path,O_RDWR|O_CREAT|O_CLOEXEC|O_NOFOLLOW,0600);if(fd<0)return -1;
 int ok=-1;struct stat st;
 if(flock(fd,LOCK_EX)==0&&fstat(fd,&st)==0&&S_ISREG(st.st_mode)){
  if(st.st_size==0){arc4random_buf(out,16);if(write_all(fd,out,16)==0&&fsync(fd)==0)ok=0;}
  else if(st.st_size==16){ssize_t n;do{n=pread(fd,out,16,0);}while(n<0&&errno==EINTR);if(n==16)ok=0;}
 }
 flock(fd,LOCK_UN);close(fd);return ok;
}
int dm_lan_history_key(void *out,const void *uid,int local){
 if(!out||!uid)return -1;uint64_t u[2];memcpy(u,uid,16);if(!(u[0]|u[1]))return -1;uint64_t actual[2];dm_account_local_uid(actual);if(memcmp(u,actual,16))return -1;
 int s=dm_room_state();if(s<1||s>5)return -1;
 unsigned char key[64]={0},digest[CC_SHA256_DIGEST_LENGTH];memcpy(key,key_magic,8);
 pthread_mutex_lock(&hist_lock);int ok=identity(key+8);pthread_mutex_unlock(&hist_lock);if(ok)return -1;
 memcpy(key+24,uid,16);uint32_t flags=local&255,version=1;memcpy(key+40,&flags,4);memcpy(key+44,&version,4);
 // Integrity check for our opaque local identifier. Not an authentication MAC.
 CC_SHA256(key,48,digest);memcpy(key+48,digest,16);memcpy(out,key,64);
 fprintf(stderr,"[ACNH LAN HISTORY] issued local history identifier; mode=%u size=64\n",flags);return 0;
}
int dm_lan_history_add(const void *uid,const void *key,const void *mine,const void *peer){
 if(!key||!mine||!peer)return -1;const unsigned char *k=key;uint32_t version;memcpy(&version,k+44,4);
 if(memcmp(k,key_magic,8)||version!=1)return -1;
 unsigned char digest[CC_SHA256_DIGEST_LENGTH];CC_SHA256(k,48,digest);unsigned diff=0;for(int i=0;i<16;i++)diff|=digest[i]^k[48+i];if(diff)return -1;
 hist_record rec={.magic={'A','C','L','R','E','C','0','1'},.time=(uint64_t)time(NULL)};
 if(uid)memcpy(rec.uid,uid,16);else dm_account_local_uid(rec.uid);
 memcpy(rec.key,key,64);memcpy(rec.mine,mine,72);memcpy(rec.peer,peer,72);
 pthread_mutex_lock(&hist_lock);char dir[PATH_MAX],path[PATH_MAX];int ok=-1,fd=-1;
 if(history_dir(dir,sizeof dir)==0&&snprintf(path,sizeof path,"%s/play-history.bin",dir)<(int)sizeof path)fd=open(path,O_WRONLY|O_APPEND|O_CREAT|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK,0600);
 if(fd>=0){struct stat st;if(flock(fd,LOCK_EX)==0){off_t start=lseek(fd,0,SEEK_END);if(start>=0&&fstat(fd,&st)==0&&S_ISREG(st.st_mode)&&start%sizeof(rec)==0){if(write_all(fd,&rec,sizeof rec)==0&&fsync(fd)==0)ok=0;else{ftruncate(fd,start);fsync(fd);}}flock(fd,LOCK_UN);}close(fd);}
 pthread_mutex_unlock(&hist_lock);
 if(ok==0)fprintf(stderr,"[ACNH LAN HISTORY] local play record persisted; size=%zu\n",sizeof rec);
 return ok;
}
