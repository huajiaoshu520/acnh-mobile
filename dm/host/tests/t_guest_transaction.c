// All registered guest threads stop during the callback; partial suspension unwinds.
#include "../hle_thread.c"
#include <assert.h>
#include <unistd.h>
static thr_t guests[3];
static _Atomic uint64_t counters[3];
static _Atomic int running=1,ready;
static int calls;
static void *work(void *arg) {
    unsigned i=(unsigned)(uintptr_t)arg;thr_t *t=&guests[i];pthread_setspecific(g_keythr,t);
    pthread_mutex_lock(&G);t->mport=pthread_mach_thread_np(pthread_self());t->started=1;t->all=g_all;g_all=t;pthread_mutex_unlock(&G);
    atomic_fetch_add(&ready,1);
    while(atomic_load(&running))atomic_fetch_add(&counters[i],1);
    pthread_mutex_lock(&G);t->exited=1;pthread_mutex_unlock(&G);return NULL;
}
static int stopped(void *unused) {
    (void)unused;++calls;uint64_t before[3];for(int i=0;i<3;i++)before[i]=atomic_load(&counters[i]);
    for(int n=0;n<10000;n++)for(int i=0;i<3;i++)assert(atomic_load(&counters[i])==before[i]);
    return 1;
}
int main(void) {
    assert(!pthread_key_create(&g_keythr,NULL));pthread_t threads[3];
    for(unsigned i=0;i<3;i++)assert(!pthread_create(&threads[i],NULL,work,(void *)(uintptr_t)i));
    while(atomic_load(&ready)!=3)usleep(100);
    for(int n=0;n<40;n++){assert(dm_guest_transaction(stopped,NULL));usleep(100);}
    pthread_mutex_lock(&G);assert(!dm_guest_transaction(stopped,NULL));pthread_mutex_unlock(&G);assert(calls==40);
    pthread_setspecific(g_keythr,&guests[0]);assert(!dm_guest_transaction(stopped,NULL));pthread_setspecific(g_keythr,NULL);
    assert(thread_suspend(guests[0].mport)==KERN_SUCCESS);
    uint64_t before=atomic_load(&counters[0]);assert(dm_guest_transaction(stopped,NULL));usleep(1000);assert(before==atomic_load(&counters[0]));
    assert(thread_resume(guests[0].mport)==KERN_SUCCESS);
    // Put an invalid port last so the valid threads have already been suspended.
    thr_t invalid={.started=1,.mport=MACH_PORT_DEAD};
    pthread_mutex_lock(&G);thr_t *tail=g_all;while(tail->all)tail=tail->all;tail->all=&invalid;pthread_mutex_unlock(&G);
    assert(!dm_guest_transaction(stopped,NULL));assert(calls==41);
    pthread_mutex_lock(&G);tail->all=NULL;pthread_mutex_unlock(&G);
    uint64_t total=0;for(int i=0;i<3;i++)total+=atomic_load(&counters[i]);usleep(1000);
    uint64_t after=0;for(int i=0;i<3;i++)after+=atomic_load(&counters[i]);assert(after>total);
    atomic_store(&running,0);for(int i=0;i<3;i++)pthread_join(threads[i],NULL);
    g_all=NULL;pthread_key_delete(g_keythr);puts("guest transaction: simultaneous stop, resume, nested suspension and failure unwind passed");return 0;
}
