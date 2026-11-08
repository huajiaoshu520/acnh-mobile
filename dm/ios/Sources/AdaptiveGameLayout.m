#import "AdaptiveGameLayout.h"
#import "PadCustomization.h"
#import <objc/runtime.h>
#include <stdlib.h>
#include "companion_layout.h"
static char hingeKey,observerKey,partialKey;
BOOL acnh_has_hinge(UIView *view) {
 return [objc_getAssociatedObject(view.window.windowScene,&hingeKey) boolValue];
}
// Use the device idiom: a host may give an iPhone view iPad traits.
BOOL acnh_allows_companion(UIView *view) {
 UIUserInterfaceIdiom device=UIDevice.currentDevice.userInterfaceIdiom;
 return device!=UIUserInterfaceIdiomPhone && (device==UIUserInterfaceIdiomPad||acnh_has_hinge(view));
}
CGFloat acnh_view_display_scale(UIView *view) {
 CGFloat scale=acnh_has_hinge(view)?view.traitCollection.displayScale:view.window.windowScene.screen.nativeScale;
 return scale>0?scale:MAX(1,view.traitCollection.displayScale);
}
static BOOL tabletop(UIView *view,CGRect *upper,CGRect *lower) {
 if(!acnh_allows_companion(view)||!acnh_has_hinge(view)||![objc_getAssociatedObject(view.window.windowScene,&partialKey) boolValue])return NO;
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
BOOL acnh_portrait_companion_active(UIView *view) {
 // Lay out the canvas in the root view's coordinates. A host window can
 // report a different orientation, and hinge capability alone is not a division.
 return acnh_allows_companion(view)&&acnh_split_portrait_bounds(view.bounds);
}
BOOL acnh_split_portrait_companion_active(UIView *view) {
 return acnh_portrait_companion_active(view)&&!acnh_tabletop_active(view);
}
BOOL acnh_companion_controls_active(UIView *view) {
 UIViewController *controller=view.window.rootViewController;
 while(controller.presentedViewController)controller=controller.presentedViewController;
 UIView *root=controller.view;return root && acnh_portrait_companion_active(root);
}
CGRect acnh_companion_frame(UIView *view) {
 if(!acnh_portrait_companion_active(view))return CGRectZero;
 CGRect top,bottom;if(tabletop(view,&top,&bottom))return bottom;
 return acnh_split_portrait_region(view.bounds,acnh_view_display_scale(view),1);
}
int acnh_display_mode(void) { return acnh_display_mode_value(getenv("ACNH_DISPLAY_MODE")); }
static int split_portrait_display_mode=-1;
int acnh_display_mode_for_view(UIView *view) {
 int launch=acnh_display_mode();
 return acnh_split_portrait_companion_active(view)?
     acnh_split_portrait_mode(launch,split_portrait_display_mode):launch;
}
void acnh_set_split_portrait_display_mode(int mode) {
 if(mode==ACNH_DISPLAY_AUTO||mode==ACNH_DISPLAY_ORIGINAL)split_portrait_display_mode=mode;
}

CGRect acnh_game_viewport(UIView *view) {
 CGRect top,bottom;
 if(tabletop(view,&top,&bottom))return top;
 if(acnh_portrait_companion_active(view))return acnh_split_portrait_region(view.bounds,acnh_view_display_scale(view),0);
 // Hinge capability alone does not mean this scene is split. Landscape
 // uses the full canvas; actual portrait/tabletop layouts returned above.
 return view.bounds;
}
BOOL acnh_expanded_view_active(UIView *view) {
 if(acnh_tabletop_active(view))return NO;
 CGRect b=acnh_game_viewport(view);
 int mode=acnh_display_mode_for_view(view);
 if(acnh_portrait_companion_active(view))return acnh_split_portrait_expand(mode,b.size);
 UIUserInterfaceIdiom idiom=UIDevice.currentDevice.userInterfaceIdiom;
 return acnh_expanded_view_eligible(idiom==UIUserInterfaceIdiomPhone?1:idiom==UIUserInterfaceIdiomPad?2:0,
     0,mode,b.size.width,b.size.height);
}

CGRect acnh_game_frame(UIView *view) {
 if(acnh_expanded_view_active(view))return acnh_game_viewport(view);
 if(acnh_portrait_companion_active(view))return acnh_fit_game(acnh_game_viewport(view));
 return acnh_display_frame(acnh_game_viewport(view),acnh_display_mode());
}
CGRect acnh_pad_frame(UIView *view) {
 acnh_pad_use_geometry(view);
 CGRect top,bottom;
 if(acnh_portrait_companion_active(view)){
  CGRect safe=UIEdgeInsetsInsetRect(view.bounds,view.safeAreaInsets);
  return dm_companion_controls_frame(acnh_companion_frame(view),safe);
 }
 if(tabletop(view,&top,&bottom))return bottom;
 // Landscape controls use the entire display, independently of the game
 // aspect ratio. Portrait and tabletop retain their lower-screen canvas.
 return view.bounds;
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
CGPoint acnh_control_point(UIView *view,CGPoint point,CGFloat radius,BOOL keepOverlayDefault) {
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
 UIView *parent=view.superview;
 BOOL landscapeOverlay=parent&&parent.bounds.size.width>parent.bounds.size.height&&CGRectEqualToRect(view.frame,parent.bounds);
 return acnh_clear_control(point,safe,radius,regions,count,keepOverlayDefault&&landscapeOverlay);
}
