#pragma once
#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#include <math.h>
static inline CGFloat pad_number(id v,CGFloat fallback){if(![v isKindOfClass:NSNumber.class])return fallback;double n=[v doubleValue];return isfinite(n)?n:fallback;}
static inline NSDictionary *pad_record(NSDictionary *layout,NSString *key){id d=layout[key];return [d isKindOfClass:NSDictionary.class]?d:nil;}
static inline CGFloat pad_scale(NSDictionary *layout,NSString *key){return MIN(2,MAX(.5,pad_number(pad_record(layout,key)[@"scale"],1)));}
static inline CGPoint pad_clamp_center(CGPoint p,CGRect safe,CGFloat margin) {
 CGFloat mx=MIN(margin+2,safe.size.width/2),my=MIN(margin+2,safe.size.height/2);
 p.x=MAX(CGRectGetMinX(safe)+mx,MIN(CGRectGetMaxX(safe)-mx,p.x));
 p.y=MAX(CGRectGetMinY(safe)+my,MIN(CGRectGetMaxY(safe)-my,p.y));
 return p;
}
static inline CGPoint pad_point(NSDictionary *layout,NSString *key,CGPoint p,CGRect bounds,CGRect safe,CGFloat margin) {
 NSDictionary *d=pad_record(layout,key);
 if(!d)return p; // A new/reset template must match the original exactly, including the NX corner inset.
 CGFloat x=pad_number(d[@"x"],NAN),y=pad_number(d[@"y"],NAN);
 if(isfinite(x)&&isfinite(y)){
  p.x=CGRectGetMinX(bounds)+MAX(0,MIN(1,x))*bounds.size.width;
  p.y=CGRectGetMinY(bounds)+MAX(0,MIN(1,y))*bounds.size.height;
 }else{
  p.x+=MAX(-1,MIN(1,pad_number(d[@"dx"],0)))*bounds.size.width;
  p.y+=MAX(-1,MIN(1,pad_number(d[@"dy"],0)))*bounds.size.height;
 }
 return pad_clamp_center(p,safe,margin);
}

// Keep the original key for landscape; portrait layouts are device-specific.
static inline NSString *pad_store_key(NSString *base,NSString *variant){return [[@"ACNH.CustomPad.v1." stringByAppendingString:base] stringByAppendingString:variant];}
