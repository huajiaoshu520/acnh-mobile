#include "../hle_acnh_text.c"
#include <assert.h>
static uint64_t tick;
uint64_t now_ns(void){return tick;}
static module_t modules[1];module_t *g_mod=modules;int g_nmod=1;
void *guest_sym(const char *name){(void)name;return NULL;}
int main(void){
 strcpy(g_mod[0].name,"main");strcpy(g_mod[0].build_id,"ff1d1c05670db6021c85b624a710b963");
 uint16_t text[]={0x5c9b,0x540d,0xd83c,0xdfdd};uint8_t state[0x30]={0};uint16_t got=0;
 assert(!dm_acnh_text_submit(NULL,1));assert(!dm_acnh_text_submit(text,257));
 assert(dm_acnh_text_submit(text,4));assert(!dm_acnh_text_submit(text,1));
 for(int i=0;i<4;i++){tick+=170000000;memset(state,0,sizeof state);dm_acnh_keyboard_state(state);assert(state[16]&(1<<4));h_acnh_GetKeyCode(&got,4,0,0);assert(got==text[i]);dm_acnh_keyboard_state(state);}
 dm_acnh_text_cancel();memset(state,0,sizeof state);dm_acnh_keyboard_state(state);assert(!state[16]);
 puts("PASS: Chinese UTF16 and surrogate pair, queue backpressure, cancel and no automatic confirmation");return 0;
}
