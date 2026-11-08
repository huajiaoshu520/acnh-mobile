#include "../Sources/AdaptiveLayoutMath.h"
#include "../../host/companion_layout.h"
#include <assert.h>
#include <stdio.h>
static void fit(CGRect bounds){
 CGRect f=acnh_fit_game(bounds);
 assert(fabs(f.size.width/f.size.height-16./9.)<1e-10);
 assert(CGRectContainsRect(bounds,f));
 assert(fabs(f.size.width-bounds.size.width)<1e-8||fabs(f.size.height-bounds.size.height)<1e-8);
 assert(fabs(CGRectGetMidX(f)-CGRectGetMidX(bounds))<1.01);
 assert(fabs(CGRectGetMidY(f)-CGRectGetMidY(bounds))<1.01);
}
int main(void){
 // Official outer/inner pixel bounds, landscape phones/tablets, portrait and window transitions.
 CGSize screens[]={{2034,1398},{2670,1878},{932,430},{1024,768},{1366,1024},{1398,2034}};
 for(unsigned i=0;i<sizeof screens/sizeof *screens;i++){
  fit(CGRectMake(0,0,screens[i].width,screens[i].height));
  fit(CGRectMake(37,13,screens[i].width-74,screens[i].height-26));
 }
 for(int i=0;i<640;i++)fit(CGRectMake(0,0,678+i/3.,466+i/4.));
 assert(acnh_inset_control_x(.096,0)==.096);
 assert(acnh_inset_control_x(.904,0)==.904);
 assert(fabs(acnh_inset_control_x(.096,1)-.121)<1e-9);
 assert(fabs(acnh_inset_control_x(.904,1)-.879)<1e-9);
 assert(acnh_inset_control_x(.5,1)==.5);
 CGRect safe=CGRectMake(0,0,678,381.375),hole=CGRectMake(305,0,68,40);
 CGPoint center=CGPointMake(339,22),p=acnh_clear_control(center,safe,20,&hole,1,0);
 assert(!acnh_control_blocked(p,20,&hole,1));assert(CGRectContainsPoint(safe,p));
 assert(hypot(p.x-center.x,p.y-center.y)>0);
 p=acnh_clear_control(CGPointMake(-99,999),safe,36,NULL,0,0);
 assert(p.x>=39&&p.y<=CGRectGetMaxY(safe)-39);
 // Stock NX landscape extends into the side inset. On a hinge-capable
 // device this must not shift the stick, A, or the +/- keys.
 CGRect landscapeSafe=CGRectMake(66,0,804,409);
 CGPoint stock[]={CGPointMake(114,327),CGPointMake(870,327),
                  CGPointMake(416,22),CGPointMake(520,22)};
 CGFloat radii[]={77,27,25,25};
 CGRect camera=CGRectMake(0,185,40,60);
 for(unsigned i=0;i<sizeof stock/sizeof *stock;i++){
  assert(CGPointEqualToPoint(acnh_clear_control(stock[i],landscapeSafe,radii[i],NULL,0,1),stock[i]));
  assert(CGPointEqualToPoint(acnh_clear_control(stock[i],landscapeSafe,radii[i],&camera,1,1),stock[i]));
  assert(!CGPointEqualToPoint(acnh_clear_control(stock[i],landscapeSafe,radii[i],NULL,0,0),stock[i]));
 }
 CGRect overlapping=CGRectMake(40,300,38,60);
 p=acnh_clear_control(stock[0],landscapeSafe,radii[0],&overlapping,1,1);
 assert(!CGPointEqualToPoint(p,stock[0])&&!acnh_circle_control_blocked(p,radii[0],&overlapping,1));
 // The reference landscape layout places the stick at x=114 and the D-pad
 // at x=264. The rectangular approximation pulled the stick to x=146.
 CGRect landscape=CGRectMake(0,0,932,430);
 CGRect edgeCamera=CGRectMake(20,184,42,68);
 CGRect dpad=CGRectMake(264-52,345-52,104,104);
 assert(acnh_control_blocked(stock[0],77,&edgeCamera,1));
 assert(!acnh_circle_control_blocked(stock[0],77,&edgeCamera,1));
 CGPoint old=acnh_clear_control(stock[0],landscapeSafe,77,&edgeCamera,1,0);
 assert(old.x==146&&hypot(old.x-264,old.y-345)<77+48);
 CGRect stockRegions[]={edgeCamera,dpad};
 p=acnh_clear_control(stock[0],landscape,77,stockRegions,2,1);
 assert(CGPointEqualToPoint(p,stock[0])&&264-48-(p.x+77)==25);
 CGRect blockedRegions[]={overlapping,dpad};
 p=acnh_clear_control(stock[0],landscape,77,blockedRegions,2,1);
 assert(!CGPointEqualToPoint(p,stock[0])&&!acnh_circle_control_blocked(p,77,blockedRegions,2));
 assert(CGRectContainsRect(landscape,CGRectMake(p.x-77,p.y-77,154,154)));
 CGRect u,l,s=CGRectMake(20,30,900,1250),d=CGRectMake(0,640,942,30);
 assert(acnh_tabletop_regions(s,&d,1,&u,&l));
 assert(CGRectGetMaxY(u)==640&&CGRectGetMinY(l)==670);
 assert(!CGRectIntersectsRect(u,l));fit(u);
 d=CGRectMake(450,0,30,1400);assert(!acnh_tabletop_regions(s,&d,1,&u,&l));
 assert(!acnh_tabletop_regions(s,NULL,0,&u,&l));
 d=CGRectMake(0,40,942,30);assert(!acnh_tabletop_regions(s,&d,1,&u,&l));
 d=CGRectMake(0,640,942,0);assert(acnh_tabletop_regions(s,&d,1,&u,&l));
 CGRect tablets[]={CGRectMake(0,0,768,1024),CGRectMake(0,0,834,1194),
                   CGRectMake(12,20,1024,1367),CGRectMake(0,0,832,936.5)};
 for(unsigned i=0;i<sizeof tablets/sizeof *tablets;i++){
  CGRect canvas=tablets[i],safe=CGRectInset(canvas,0,24);
  assert(acnh_split_portrait_bounds(canvas));
  u=acnh_split_portrait_region(canvas,2,0);
  l=acnh_split_portrait_region(canvas,2,1);
  assert(fabs(u.size.height-l.size.height)<=0.5&&u.size.width==l.size.width);
  assert(u.size.height*2==round(u.size.height*2));
  assert(CGRectGetMaxY(u)==CGRectGetMinY(l)&&CGRectEqualToRect(CGRectUnion(u,l),canvas));
  assert(acnh_split_portrait_expand(ACNH_DISPLAY_AUTO,u.size));
  assert(!acnh_split_portrait_expand(ACNH_DISPLAY_ORIGINAL,u.size));
  CGRect original=acnh_display_frame(u,ACNH_DISPLAY_ORIGINAL);
  assert(CGRectContainsRect(u,original));
  assert(fabs(original.size.width/original.size.height-16./9.)<1e-10);
  assert(original.size.width==u.size.width&&original.size.height<u.size.height);
  CGRect controls=dm_companion_controls_frame(l,safe);
  assert(CGRectGetMinY(controls)==CGRectGetMinY(l));
  assert(CGRectContainsRect(safe,controls));
  assert(CGRectGetMaxY(controls)<=CGRectGetMaxY(l)-dm_companion_footer_height(l.size));
 }
 // IMG_0497: a 1668x2420 canvas at 2x with top and bottom safe areas.
 // The old upper region ended at pixel 970. Both picture modes now use 1210.
 CGRect capture=CGRectMake(0,0,834,1210),captureSafe=CGRectMake(0,16,834,1169);
 u=acnh_split_portrait_region(capture,2,0);
 l=acnh_split_portrait_region(capture,2,1);
 assert(CGRectGetMaxY(u)*2==1210&&CGRectGetMinY(l)*2==1210);
 assert(CGRectEqualToRect(CGRectUnion(u,l),capture));
 CGRect captureControls=dm_companion_controls_frame(l,captureSafe);
 assert(CGRectGetMinY(captureControls)*2==1210);
 assert(fabs(CGRectGetMaxY(captureControls)-1134.94)<0.001);
 assert(CGRectEqualToRect(dm_companion_controls_frame(l,CGRectMake(28,621,778,470)),CGRectMake(28,621,778,470)));
 assert(CGRectIsEmpty(dm_companion_controls_frame(l,CGRectMake(0,0,834,550))));
 assert(acnh_split_portrait_expand(ACNH_DISPLAY_AUTO,u.size));
 assert(!acnh_split_portrait_expand(ACNH_DISPLAY_ORIGINAL,u.size));
 assert(acnh_fit_game(u).size.height==captureSafe.size.width*9./16.);
 assert(!acnh_split_portrait_bounds(CGRectMake(0,0,1024,768)));
 assert(!acnh_split_portrait_expand(ACNH_DISPLAY_AUTO,CGSizeMake(300,400)));
 assert(acnh_split_portrait_expand(ACNH_DISPLAY_AUTO,CGSizeMake(832,468)));
 assert(acnh_split_portrait_mode(ACNH_DISPLAY_AUTO,-1)==ACNH_DISPLAY_AUTO);
 assert(acnh_split_portrait_mode(ACNH_DISPLAY_ORIGINAL,-1)==ACNH_DISPLAY_ORIGINAL);
 assert(acnh_split_portrait_mode(ACNH_DISPLAY_STRETCH,-1)==ACNH_DISPLAY_ORIGINAL);
 assert(acnh_split_portrait_mode(ACNH_DISPLAY_CROP,-1)==ACNH_DISPLAY_ORIGINAL);
 assert(acnh_split_portrait_mode(ACNH_DISPLAY_ORIGINAL,ACNH_DISPLAY_AUTO)==ACNH_DISPLAY_AUTO);
 assert(acnh_split_portrait_mode(ACNH_DISPLAY_AUTO,ACNH_DISPLAY_ORIGINAL)==ACNH_DISPLAY_ORIGINAL);
 puts("adaptive layout: 652 fit cases, iPad pixel-aligned split and picture choices, lower control body and reserved-region checks passed");
}
