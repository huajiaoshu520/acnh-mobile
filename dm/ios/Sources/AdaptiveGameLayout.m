#import "AdaptiveGameLayout.h"
#import <objc/runtime.h>
#include <stdlib.h>
static char hingeKey,observerKey,partialKey;
BOOL acnh_has_hinge(UIView *view) {
 return [objc_getAssociatedObject(view.window.windowScene,&hingeKey) boolValue];
}
CGFloat acnh_view_display_scale(UIView *view) {
 CGFloat scale=acnh_has_hinge(view)?view.traitCollection.displayScale:view.window.windowScene.screen.nativeScale;
 return scale>0?scale:MAX(1,view.traitCollection.displayScale);
}
static BOOL tabletop(UIView *view,CGRect *upper,CGRect *lower) {
 if(!acnh_has_hinge(view)||![objc_getAssociatedObject(view.window.windowScene,&partialKey) boolValue])return NO;
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 270100
 if(@available(iOS 27.1,*)) {
  CGRect divisions[16];int count=0;
  for(UIViewReservedRegion *r in [view reservedRegionsOfKind:[UIViewReservedRegionKind divisionRegionKind]])
   if(r.active&&count<16)divisions[count++]=r.frame;
  return acnh_tabletop_regions(UIEdgeInsetsInsetRect(view.bounds,view.safeAreaInsets),divisions,count,upper,lower);
 }
#endif
 return NO;
}
BOOL acnh_tabletop_active(UIView *view) { CGRect top,bottom;return tabletop(view,&top,&bottom); }
int acnh_display_mode(void) { return acnh_display_mode_value(getenv("ACNH_DISPLAY_MODE")); }
CGRect acnh_game_viewport(UIView *view) {
 CGRect top,bottom;
 if(tabletop(view,&top,&bottom))return top;
 CGRect b=view.bounds;
 if(acnh_has_hinge(view))b=UIEdgeInsetsInsetRect(b,view.safeAreaInsets);
 return b;
}
BOOL acnh_expanded_view_active(UIView *view) {
 CGRect b=acnh_game_viewport(view);
 UIUserInterfaceIdiom idiom=view.traitCollection.userInterfaceIdiom;
 return acnh_expanded_view_eligible(idiom==UIUserInterfaceIdiomPhone?1:idiom==UIUserInterfaceIdiomPad?2:0,
     acnh_has_hinge(view),acnh_display_mode(),b.size.width,b.size.height);
}

CGRect acnh_game_frame(UIView *view) {
 if(acnh_expanded_view_active(view))return acnh_game_viewport(view);
 return acnh_display_frame(acnh_game_viewport(view),acnh_display_mode());
}
CGRect acnh_pad_frame(UIView *view) {
 CGRect top,bottom;
 if(tabletop(view,&top,&bottom))return bottom;
 return acnh_has_hinge(view)?acnh_fit_game(acnh_game_viewport(view)):view.bounds;
}
void acnh_observe_display(UIView *view) {
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 270100
 if(@available(iOS 27.1,*)) {
  if(objc_getAssociatedObject(view,&observerKey))return;
  __weak UIView *weakView=view;
  UIHingeInteraction *observer=[[UIHingeInteraction alloc] initWithUpdateHandler:^(UIHingeInteraction *interaction,UIHingeInteractionUpdate *update){
   (void)interaction;UIView *v=weakView;UIWindowScene *scene=v.window.windowScene;
   if(!v||!scene)return;
   // Capability survives display changes. A nil update clears the posture,
   // so an outer display cannot inherit a previous tabletop layout.
   BOOL wasCapable=acnh_has_hinge(v);
   BOOL partial=update.hinge&&update.hinge.status==UIHingeStatusPartiallyOpen;
   if(update.hinge)objc_setAssociatedObject(scene,&hingeKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
   objc_setAssociatedObject(scene,&partialKey,@(partial),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
   if(!wasCapable&&update.hinge){
    NSLog(@"[display] hinge-capable scene; adaptive controls enabled");
    [v.window.rootViewController setNeedsUpdateOfSupportedInterfaceOrientations];
   }
   // Query the system's division in layout; angle alone doesn't describe
   // which way the hinge crosses this window (or a multitasking window).
   [v setNeedsLayout];[v.window.rootViewController.view setNeedsLayout];
  }];
  objc_setAssociatedObject(view,&observerKey,observer,OBJC_ASSOCIATION_RETAIN_NONATOMIC);[view addInteraction:observer];
 }
#endif
}
CGPoint acnh_control_point(UIView *view,CGPoint point,CGFloat radius) {
 if(!acnh_has_hinge(view))return point;
 CGRect safe=UIEdgeInsetsInsetRect(view.bounds,view.safeAreaInsets);CGRect regions[32];int count=0;
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 270100
 if(@available(iOS 27.1,*)) {
  for(UIViewReservedRegionKind *kind in @[[UIViewReservedRegionKind occlusionRegionKind],[UIViewReservedRegionKind divisionRegionKind]]){
   for(UIViewReservedRegion *region in [view reservedRegionsOfKind:kind]){
    if(region.active&&!CGRectIsEmpty(region.frame)&&!CGRectIsInfinite(region.frame)&&count<32)regions[count++]=region.frame;
   }
  }
 }
#endif
 return acnh_clear_control(point,safe,radius,regions,count);
}
