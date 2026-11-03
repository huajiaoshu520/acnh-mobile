// The full-size companion owns the lower screen; controls float above its content.
#pragma once
#include <CoreGraphics/CoreGraphics.h>
#include <math.h>
typedef struct { CGRect game,controls,panel,left,right; } dm_companion_layout;
static inline CGFloat dm_companion_footer_height(CGSize size) {
    if(!isfinite(size.width)||!isfinite(size.height)||size.width<=0||size.height<=0)return 0;
    return fmin(size.height*0.2,fmax(56,fmin(88,size.width*0.09)));
}
static inline dm_companion_layout dm_companion_screen_layout(CGRect safe, int portrait, int touch) {
    dm_companion_layout r={0};
    if(!isfinite(safe.size.width) || !isfinite(safe.size.height) || safe.size.width<=0 || safe.size.height<=0)return r;
    CGFloat w=safe.size.width,h=safe.size.height;
    if(!portrait){
        w=fmin(w,h*16.0/9.0);h=w*9.0/16.0;
        r.game=CGRectMake(safe.origin.x+(safe.size.width-w)/2,safe.origin.y+(safe.size.height-h)/2,w,h);
        return r;
    }
    h=fmin(safe.size.height*0.48,w*9.0/16.0);w=h*16.0/9.0;
    r.game=CGRectMake(safe.origin.x+(safe.size.width-w)/2,safe.origin.y,w,h);
    CGRect lower=CGRectMake(safe.origin.x,CGRectGetMaxY(r.game),safe.size.width,safe.size.height-h);
    r.panel=lower;
    if(touch){
        r.controls=lower;r.controls.size.height-=dm_companion_footer_height(lower.size);
        CGFloat rail=lower.size.width*0.22;
        r.left=CGRectMake(lower.origin.x,lower.origin.y,rail,r.controls.size.height);
        r.right=CGRectMake(CGRectGetMaxX(lower)-rail,lower.origin.y,rail,r.controls.size.height);
    }
    return r;
}
// UIKit drawing, input hit testing and geometry validation consume the same positions.
typedef struct {
    CGRect left,right;
    CGPoint stick,dpad,face,right_stick,shoulder[4],click[2],menu[2],swap;
    CGFloat unit,face_gap,face_r,stick_r,dpad_r,right_r,shoulder_r,click_r,swap_r,knob_r;
    CGSize menu_size;
} dm_companion_pad_layout;
static inline dm_companion_pad_layout dm_companion_pad_geometry(CGSize size) {
    dm_companion_pad_layout p={0};
    CGFloat w=size.width,h=size.height;
    if(!isfinite(w)||!isfinite(h)||w<=0||h<=0)return p;
    CGFloat rail=w*0.22;
    p.unit=fmin(1.08,fmin(rail/175.0,h/550.0));
    p.left=CGRectMake(0,0,rail,h);p.right=CGRectMake(w-rail,0,rail,h);
    p.stick=CGPointMake(rail/2,h*0.36);p.face=CGPointMake(w-rail/2,h*0.36);
    p.dpad=CGPointMake(rail/2,h*0.72);p.right_stick=CGPointMake(w-rail/2,h*0.72);
    p.shoulder[0]=CGPointMake(rail*0.30,42*p.unit);p.shoulder[1]=CGPointMake(rail*0.72,42*p.unit);
    p.shoulder[2]=CGPointMake(w-rail*0.30,42*p.unit);p.shoulder[3]=CGPointMake(w-rail*0.72,42*p.unit);
    p.click[0]=CGPointMake(rail/2,h*0.54);p.click[1]=CGPointMake(w-rail/2,h*0.54);
    p.menu[0]=CGPointMake(rail/2,h*0.91);p.menu[1]=CGPointMake(w-rail/2,h*0.91);
    p.swap=CGPointMake(rail/2,h*0.18);
    p.face_gap=42*p.unit;p.face_r=24*p.unit;p.stick_r=50*p.unit;p.right_r=46*p.unit;p.dpad_r=43*p.unit;
    p.shoulder_r=24*p.unit;p.click_r=17*p.unit;p.swap_r=15*p.unit;p.knob_r=30*p.unit;
    p.menu_size=CGSizeMake(50*p.unit,36*p.unit);
    return p;
}

static inline int dm_companion_pad_contains(CGSize size,CGPoint point) {
    dm_companion_pad_layout p=dm_companion_pad_geometry(size);
    return CGRectContainsPoint(p.left,point) || CGRectContainsPoint(p.right,point);
}
