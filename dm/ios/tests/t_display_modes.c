#include "../Sources/AdaptiveLayoutMath.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 CGRect b=CGRectMake(0,0,932,430);CGPoint q;
 for(int mode=0;mode<4;mode++) {
  CGRect image=acnh_display_frame(b,mode);
  assert(acnh_game_touch(CGPointMake(466,215),image,&q));
  assert(fabs(q.x-.5)<.002&&fabs(q.y-.5)<.002);
  if(mode==ACNH_DISPLAY_STRETCH)assert(CGRectEqualToRect(image,b));
  else assert(fabs(image.size.width/image.size.height-16./9.)<1e-10);
  if(mode==ACNH_DISPLAY_CROP)assert(CGRectContainsRect(image,b));
  else assert(CGRectContainsRect(b,image));
 }
 assert(!acnh_game_touch(CGPointMake(0,0),acnh_display_frame(b,ACNH_DISPLAY_ORIGINAL),&q));
 assert(acnh_game_touch(CGPointMake(0,0),acnh_display_frame(b,ACNH_DISPLAY_STRETCH),&q)&&q.x==0&&q.y==0);
 assert(acnh_game_touch(CGPointMake(0,0),acnh_display_frame(b,ACNH_DISPLAY_CROP),&q)&&q.x==0&&q.y>.08&&q.y<.10);
 assert(!acnh_game_touch(CGPointMake(1,1),CGRectZero,&q));
 assert(!acnh_game_touch(CGPointMake(NAN,1),b,&q));
 assert(acnh_display_mode_value(NULL)==ACNH_DISPLAY_AUTO);
 assert(acnh_display_mode_value("bad")==ACNH_DISPLAY_AUTO);
 assert(acnh_display_mode_value("original")==ACNH_DISPLAY_ORIGINAL);
 assert(acnh_display_mode_value("stretch")==ACNH_DISPLAY_STRETCH);
 assert(acnh_display_mode_value("crop")==ACNH_DISPLAY_CROP);
 // The same transform works for portrait, rotated tablets and a nonzero Duo upper region.
 CGRect screens[]={{{0,0},{430,932}},{{0,0},{1024,768}},{{20,30},{900,610}}};
 for(unsigned i=0;i<sizeof screens/sizeof *screens;i++)for(int mode=0;mode<4;mode++){
  CGRect image=acnh_display_frame(screens[i],mode);
  assert(acnh_game_touch(CGPointMake(CGRectGetMidX(screens[i]),CGRectGetMidY(screens[i])),image,&q));
  assert(fabs(q.x-.5)<.002&&fabs(q.y-.5)<.002);
  if(mode==ACNH_DISPLAY_CROP)assert(CGRectContainsRect(image,screens[i]));
 }
 assert(acnh_expanded_view_eligible(1,0,ACNH_DISPLAY_AUTO,932,430));
 assert(!acnh_expanded_view_eligible(0,0,ACNH_DISPLAY_AUTO,1366,768));
 assert(!acnh_expanded_view_eligible(0,0,ACNH_DISPLAY_AUTO,1400,600));
 assert(!acnh_expanded_view_eligible(1,1,ACNH_DISPLAY_AUTO,932,430));
 assert(!acnh_expanded_view_eligible(1,0,ACNH_DISPLAY_AUTO,430,932));
 for(int m=1;m<4;m++)assert(!acnh_expanded_view_eligible(1,0,m,932,430));
 assert(!acnh_expanded_view_eligible(1,0,ACNH_DISPLAY_AUTO,NAN,430));

 assert(acnh_expanded_view_eligible(2,0,ACNH_DISPLAY_AUTO,1024,768));
 assert(acnh_expanded_view_eligible(2,0,ACNH_DISPLAY_AUTO,1180,820));
 assert(acnh_expanded_view_eligible(2,0,ACNH_DISPLAY_AUTO,1133,744));
 assert(!acnh_expanded_view_eligible(2,0,ACNH_DISPLAY_AUTO,768,1024));
 assert(!acnh_expanded_view_eligible(2,1,ACNH_DISPLAY_AUTO,1024,768));
 for(int m=1;m<4;m++)assert(!acnh_expanded_view_eligible(2,0,m,1024,768));
 assert(!acnh_expanded_view_eligible(2,0,ACNH_DISPLAY_AUTO,INFINITY,768));
 puts("display modes: fit/fill/stretch, clipped edges, rotation, Duo origins and inverse touch mapping passed");
}
