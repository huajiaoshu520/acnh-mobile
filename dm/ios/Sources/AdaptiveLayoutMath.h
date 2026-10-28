#pragma once
#include <CoreGraphics/CoreGraphics.h>
#include <math.h>
#include <string.h>
// Fit the complete game image; no anisotropic scaling or cropping.
static inline CGRect acnh_fit_game(CGRect b) {
 if(b.size.width<=0||b.size.height<=0)return CGRectMake(b.origin.x,b.origin.y,0,0);
 CGFloat w=b.size.width,h=b.size.height;
 if(w/h>16.0/9.0)w=h*16.0/9.0;else h=w*9.0/16.0;
 return CGRectMake(b.origin.x+floor((b.size.width-w)/2),b.origin.y+floor((b.size.height-h)/2),w,h);
}
enum { ACNH_DISPLAY_AUTO, ACNH_DISPLAY_ORIGINAL, ACNH_DISPLAY_STRETCH, ACNH_DISPLAY_CROP };
static inline int acnh_display_mode_value(const char *value) {
 if(!value)return ACNH_DISPLAY_AUTO;
 if(!strcmp(value,"original"))return ACNH_DISPLAY_ORIGINAL;
 if(!strcmp(value,"stretch"))return ACNH_DISPLAY_STRETCH;
 if(!strcmp(value,"crop"))return ACNH_DISPLAY_CROP;
 return ACNH_DISPLAY_AUTO;
}
static inline CGRect acnh_display_frame(CGRect viewport,int mode) {
 if(viewport.size.width<=0||viewport.size.height<=0)return CGRectMake(viewport.origin.x,viewport.origin.y,0,0);
 if(mode==ACNH_DISPLAY_STRETCH)return viewport;
 if(mode!=ACNH_DISPLAY_CROP)return acnh_fit_game(viewport);
 CGFloat scale=fmax(viewport.size.width/16.,viewport.size.height/9.);
 CGFloat w=16*scale,h=9*scale;
 return CGRectMake(CGRectGetMidX(viewport)-w/2,CGRectGetMidY(viewport)-h/2,w,h);
}
// Use the displayed image's actual rectangle, including the hidden part of aspect-fill.
static inline int acnh_game_touch(CGPoint point,CGRect image,CGPoint *normalized) {
 if(image.size.width<=0||image.size.height<=0)return 0;
 CGFloat x=(point.x-image.origin.x)/image.size.width,y=(point.y-image.origin.y)/image.size.height;
 if(!isfinite(x)||!isfinite(y)||x<0||x>=1||y<0||y>=1)return 0;
 *normalized=CGPointMake(x,y);return 1;
}
static inline CGPoint acnh_clamp_control(CGPoint p,CGRect safe,CGFloat r) {
 CGFloat mx=fmin(r+3,safe.size.width/2),my=fmin(r+3,safe.size.height/2);
 p.x=fmax(CGRectGetMinX(safe)+mx,fmin(CGRectGetMaxX(safe)-mx,p.x));
 p.y=fmax(CGRectGetMinY(safe)+my,fmin(CGRectGetMaxY(safe)-my,p.y));return p;
}
static inline int acnh_control_blocked(CGPoint p,CGFloat r,const CGRect *regions,int count) {
 for(int i=0;i<count;i++)if(CGRectContainsPoint(CGRectInset(regions[i],-r-3,-r-3),p))return 1;return 0;
}
// Move the whole circular control, not its artwork or touch region independently.
static inline CGPoint acnh_clear_control(CGPoint p,CGRect safe,CGFloat r,const CGRect *regions,int count) {
 p=acnh_clamp_control(p,safe,r);if(!acnh_control_blocked(p,r,regions,count))return p;
 CGPoint best=p;CGFloat cost=INFINITY;
 for(int i=0;i<count;i++){
  CGRect b=CGRectInset(regions[i],-r-4,-r-4);
  CGPoint candidates[]={{CGRectGetMinX(b),p.y},{CGRectGetMaxX(b),p.y},{p.x,CGRectGetMinY(b)},{p.x,CGRectGetMaxY(b)}};
  for(int j=0;j<4;j++){
   CGPoint q=acnh_clamp_control(candidates[j],safe,r);if(acnh_control_blocked(q,r,regions,count))continue;
   CGFloat d=(q.x-p.x)*(q.x-p.x)+(q.y-p.y)*(q.y-p.y);if(d<cost){best=q;cost=d;}
  }
 }
 return best;
}
static inline CGFloat acnh_inset_control_x(CGFloat x,int foldable) {
 if(!foldable)return x;return x<.45?x+.025:x>.55?x-.025:x;
}

// Split only across an active horizontal system division. Never infer a hinge
// from a screen ratio or split at a guessed midpoint (outer screens/iPads).
static inline int acnh_tabletop_regions(CGRect safe,const CGRect *divisions,int count,CGRect *upper,CGRect *lower) {
 for(int i=0;i<count;i++){
  CGRect d=CGRectIntersection(safe,divisions[i]);
  if(CGRectIsNull(d)||CGRectIsInfinite(d)||d.size.width<safe.size.width*.8||d.size.height>safe.size.height*.2)continue;
  CGFloat top=CGRectGetMinY(d)-CGRectGetMinY(safe),bottom=CGRectGetMaxY(safe)-CGRectGetMaxY(d);
  if(top<safe.size.height*.25||bottom<safe.size.height*.25)continue;
  *upper=CGRectMake(safe.origin.x,safe.origin.y,safe.size.width,top);
  *lower=CGRectMake(safe.origin.x,CGRectGetMaxY(d),safe.size.width,bottom);
  return 1;
 }
 return 0;
}

// Keep device capability and picture mode gating in a testable pure function.
static inline int acnh_expanded_view_eligible(int phone,int hinge,int mode,double width,double height) {
 if(!phone||hinge||mode!=ACNH_DISPLAY_AUTO||!isfinite(width)||!isfinite(height)||width<=0||height<=0)return 0;
 double aspect=width/height;
 return aspect>16.0/9.0+.01&&aspect<=2.6;
}
