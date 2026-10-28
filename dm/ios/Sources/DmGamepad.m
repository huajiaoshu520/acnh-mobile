// ACNH icon controls: dual sticks, separate tool D-pad, phone and reactions on the right.
#import <UIKit/UIKit.h>
#import <GameController/GameController.h>
#import <CoreText/CoreText.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "dm_input.h"
#include "pad_slots.h"
#include <stdio.h>
#define PAD_LOG(...) do { fprintf(stderr,"[gamepad] "); fprintf(stderr,__VA_ARGS__); fputc(10,stderr); } while(0)
#include "front.h"
#include "haptics_apple.h"
#include "DmMotion.h"
#include "AcnhTouchLogic.h"
#include "AcnhTextDetect.h"
#import "PadCustomization.h"
#import "AdaptiveGameLayout.h"
#import "TouchFeedback.h"
enum { B_A=0,B_B,B_X,B_Y,B_LS,B_RS,B_L,B_R,B_ZL,B_ZR,B_PLUS,B_MINUS,B_LEFT,B_UP,B_RIGHT,B_DOWN,B_L_TOP,B_CAPTURE,B_N };
_Static_assert(B_UP==13 && B_A==0,"Tool-wheel input bit mapping");
// Both visible L controls send the same Switch button.
static int inputButton(int button){return button==B_L_TOP?B_L:button;}
enum { C_NONE=0,C_STICK,C_CAMERA,C_BTN,C_TOOL,C_CAPTURE };
static pthread_mutex_t g_mu=PTHREAD_MUTEX_INITIALIZER;
static uint64_t g_touch_btn, g_shake_btn;
static int32_t g_touch_lx,g_touch_ly,g_touch_rx,g_touch_ry;
static void publish(void){
 pthread_mutex_lock(&g_mu);
 dm_input_set(g_touch_btn|g_shake_btn,g_touch_lx,g_touch_ly,g_touch_rx,g_touch_ry);
 pthread_mutex_unlock(&g_mu);
}
static CGFloat distance(CGPoint a,CGPoint b){return hypot(a.x-b.x,a.y-b.y);}
static CGPathRef circlePath(CGPoint c,CGFloat r){return CGPathCreateWithEllipseInRect(CGRectMake(c.x-r,c.y-r,2*r,2*r),NULL);}
static void setPath(CAShapeLayer *l,CGPathRef p){l.path=p;CGPathRelease(p);}
static void glyphLine(CGMutablePathRef p,const CGFloat *v,int n){CGPathMoveToPoint(p,NULL,v[0],v[1]);for(int i=2;i<n;i+=2)CGPathAddLineToPoint(p,NULL,v[i],v[i+1]);}
#define GL(...) do{const CGFloat points[]={__VA_ARGS__};glyphLine(p,points,sizeof(points)/sizeof(points[0]));}while(0)
static void glyphRect(CGMutablePathRef p,CGRect r,CGFloat radius){CGPathRef q=CGPathCreateWithRoundedRect(r,radius,radius,NULL);CGPathAddPath(p,NULL,q);CGPathRelease(q);}
static CGPathRef iconPath(int b){
 CGMutablePathRef p=CGPathCreateMutable();
 switch(b){
 case B_CAPTURE:glyphRect(p,CGRectMake(12,12,40,40),5);CGPathAddEllipseInRect(p,NULL,CGRectMake(23,23,18,18));break;
 case B_A:GL(21,33,21,11,22,8,25,7,28,9,29,12,29,29,30,22,34,21,37,24,38,28,40,25,44,26,46,31,49,30,53,33,53,44,50,52,45,57,29,57,21,51,12,39,12,35,15,33,18,34,23,40);break;
 case B_L:CGPathAddEllipseInRect(p,NULL,CGRectMake(28,5,9,9));GL(31,18,27,32,38,40,43,53);GL(30,20,20,17,14,8);GL(31,20,43,15,49,7);GL(27,32,18,41,8,36);GL(10,57,51,57);break;
 case B_N:CGPathAddEllipseInRect(p,NULL,CGRectMake(36,5,10,10));GL(37,20,29,34,41,41,45,55);GL(35,23,25,21,18,29);GL(36,23,44,29,53,25);GL(29,34,22,44,10,45);break;
 case B_B:GL(27,15,10,32,27,49);GL(11,32,40,32,48,36,52,43,52,51);break;
 case B_X:glyphRect(p,CGRectMake(12,17,40,41),9);glyphRect(p,CGRectMake(20,36,24,15),4);GL(23,17,23,9,26,6,38,6,41,9,41,17);GL(18,25,46,25);break;
 case B_Y:GL(8,43,17,34,26,34,32,37,41,37,44,39,44,42,41,44,29,44);GL(44,40,53,30,56,29,59,31,59,34,49,47,39,54,20,54,14,59);GL(5,43,15,54);break;
 case B_ZL:glyphRect(p,CGRectMake(17,5,30,54),6);GL(26,11,38,11);GL(28,52,36,52);break;
 case B_ZR:CGPathAddEllipseInRect(p,NULL,CGRectMake(9,9,46,46));CGPathAddEllipseInRect(p,NULL,CGRectMake(22,22,4,4));CGPathAddEllipseInRect(p,NULL,CGRectMake(38,22,4,4));GL(21,36,24,41,28,44,36,44,40,41,43,36);break;
 case B_RS:glyphRect(p,CGRectMake(8,19,48,34),6);GL(18,19,23,11,40,11,45,19);CGPathAddEllipseInRect(p,NULL,CGRectMake(23,27,18,18));break;
 case B_LEFT:GL(33,15,16,32,33,49);GL(17,32,51,32);break;
 case B_RIGHT:GL(31,15,48,32,31,49);GL(47,32,13,32);break;
 case B_UP:for(int i=0;i<8;i++){CGFloat a=i*M_PI/4;CGPathAddEllipseInRect(p,NULL,CGRectMake(29.5+24*cos(a),29.5+24*sin(a),5,5));}GL(26,43,37,27);GL(28,22,34,16,45,24,39,31,28,22);break;
 case B_DOWN:GL(22,46,39,23);GL(28,22,34,15,47,25,41,32,28,22);CGPathAddEllipseInRect(p,NULL,CGRectMake(7,7,50,50));GL(14,14,50,50);break;
 }
 return p;
}
#undef GL
static CAShapeLayer *acnhControlOutline(CALayer *parent){CAShapeLayer *l=[CAShapeLayer layer];l.fillColor=[UIColor colorWithWhite:0 alpha:.15].CGColor;l.strokeColor=[UIColor colorWithRed:.92 green:.87 blue:.71 alpha:.6].CGColor;l.lineWidth=1;[parent addSublayer:l];return l;}
static CAShapeLayer *glyph(CALayer *parent,int b){CAShapeLayer *l=[CAShapeLayer layer];l.fillColor=nil;l.strokeColor=[UIColor colorWithWhite:1 alpha:.8].CGColor;l.lineCap=kCALineCapRound;l.lineJoin=kCALineJoinRound;setPath(l,iconPath(b));[parent addSublayer:l];return l;}
static void positionGlyph(CAShapeLayer *l,CGPoint c,CGFloat size){l.bounds=CGRectMake(0,0,64,64);l.position=c;l.affineTransform=CGAffineTransformMakeScale(size/64,size/64);l.lineWidth=3.2;}
static void placeLabel(CATextLayer *t,CGPoint c,CGFloat size){t.fontSize=size;t.frame=CGRectMake(c.x-size*1.5,c.y-size*.7,size*3,size*1.5);}
@interface DmGamepad:UIView<ACNHEditablePad>
@end
static __weak DmGamepad *g_view;
static void pads_start(void);
@implementation DmGamepad {
 UIImpactFeedbackGenerator *_touchFeedback;TouchFeedbackEdges _feedbackEdges;
 NSMapTable<UITouch *,NSNumber *> *_owner,*_screenTouches;
 CAShapeLayer *_key[B_N],*_glyph[B_N],*_ring,*_knob,*_rightKnob;
 CATextLayer *_label[B_N];
 UITouch *_stick,*_camera,*_toolTouch,*_captureTouch;BOOL _screenshotPending;CGPoint _stickNow,_cameraNow,_toolOrigin;
 AcnhWheel _wheel;int _wheelButton;CADisplayLink *_toolLink;
 uint64_t _faceBits,_btnBits;BOOL _checkingTextField,_nativeTextOpen,_showRunIcon;
}
- (instancetype)initWithFrame:(CGRect)f{
 if((self=[super initWithFrame:f])){
  self.multipleTouchEnabled=YES;self.opaque=NO;self.backgroundColor=UIColor.clearColor;
 _touchFeedback=touch_feedback_create(acnh_pad_editing());
  _owner=[NSMapTable strongToStrongObjectsMapTable];_screenTouches=[NSMapTable strongToStrongObjectsMapTable];
  _ring=acnhControlOutline(self.layer);_knob=acnhControlOutline(self.layer);
  for(int b=0;b<B_N;b++){
   if(b==B_LS)continue;_key[b]=acnhControlOutline(self.layer);
   if(b==B_RS)continue;
   if(b==B_L_TOP||b==B_R||b==B_PLUS||b==B_MINUS){
    CATextLayer *t=[CATextLayer layer];t.string=b==B_L_TOP?@"L":b==B_R?@"R":b==B_PLUS?@"+":@"−";t.alignmentMode=kCAAlignmentCenter;t.contentsScale=acnh_view_display_scale(self);t.foregroundColor=[UIColor colorWithWhite:1 alpha:.5].CGColor;[_key[b] addSublayer:t];_label[b]=t;
   }else _glyph[b]=glyph(_key[b],b);
  }
  _rightKnob=acnhControlOutline(self.layer);_glyph[B_RS]=glyph(_rightKnob,B_RS);
  if(!acnh_pad_editing()){
  [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(padsChanged:) name:GCControllerDidConnectNotification object:nil];
  [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(padsChanged:) name:GCControllerDidDisconnectNotification object:nil];pads_start();[self padsChanged:nil]; phone_motion_start();
  }
 }
 return self;
}
- (CGFloat)baseRadius:(int)b{
 CGFloat h=self.bounds.size.height;
 if(b==B_L_TOP)return MIN(20,MAX(14,h*.0324));
 if(b==B_A)return MAX(28,h*.074);
 if(b==B_RS)return MAX(24,h*.0565);
 if(b==B_L)return MAX(20,h*.0481);
 if(b>=B_LEFT)return MAX(16,h*.0343);
 if(b==B_ZL||b==B_ZR)return MAX(19,h*.0398);
 if(b<=B_Y)return MAX(20,h*(b==B_B?.051:.0481));
 return MIN(20,MAX(14,h*.0324));
}
- (CGFloat)radius:(int)b{return [self baseRadius:b]*acnh_pad_scale(@"acnh",[NSString stringWithFormat:@"button.%d",b]);}
- (CGPoint)atX:(CGFloat)x y:(CGFloat)y radius:(CGFloat)radius{
 x=acnh_inset_control_x(x,acnh_has_hinge(self));
 CGRect r=UIEdgeInsetsInsetRect(self.bounds,self.safeAreaInsets);CGFloat m=radius+3;
 return CGPointMake(MAX(CGRectGetMinX(r)+m,MIN(CGRectGetMaxX(r)-m,x*self.bounds.size.width)),MAX(CGRectGetMinY(r)+m,MIN(CGRectGetMaxY(r)-m,y*self.bounds.size.height)));
}
- (CGPoint)defaultButtonCenter:(int)b{
 if(b==B_L_TOP){
  CGPoint minus=[self defaultButtonCenter:B_MINUS];CGFloat radius=[self baseRadius:b];
  CGRect safe=UIEdgeInsetsInsetRect(self.bounds,self.safeAreaInsets);
  return CGPointMake(MIN(CGRectGetMaxX(safe)-radius-3,minus.x+[self baseRadius:B_MINUS]+radius+12),minus.y);
 }
 CGFloat x=0,y=0;
 switch(b){
 case B_CAPTURE:x=.5;y=.847;break;
 case B_A:x=.919;y=.792;break;case B_B:x=.785;y=.775;break;case B_X:x=.875;y=.435;break;case B_Y:x=.811;y=.592;break;
 case B_ZL:x=.775;y=.40;break;case B_ZR:x=.785;y=.19;break;case B_RS:x=.919;y=.30;break;
 case B_L:x=.742;y=.658;break;case B_R:x=.849;y=.134;break;case B_PLUS:x=.904;y=.134;break;case B_MINUS:x=.096;y=.134;break;
 case B_UP:x=.15625;y=.39167;break;case B_LEFT:x=.11615;y=.463;break;case B_RIGHT:x=.19635;y=.463;break;case B_DOWN:x=.15625;y=.53426;break;
 }
 return [self atX:x y:y radius:[self radius:b]];
}
- (CGPoint)buttonCenter:(int)b{return acnh_pad_point(@"acnh",[NSString stringWithFormat:@"button.%d",b],[self defaultButtonCenter:b],self,[self radius:b]);}
// Dedicated and its custom template use the original stick size.
- (CGFloat)stickR{return MAX(38,self.bounds.size.height*.0944)*acnh_pad_scale(@"acnh",@"stick");}
- (CGFloat)toolWheelR{return MAX(38,self.bounds.size.height*.0944)*.65;}
- (CGPoint)stickHome{return acnh_pad_point(@"acnh",@"stick",[self atX:.15625 y:.74074 radius:self.stickR],self,self.stickR);}
- (NSArray<NSDictionary *> *)acnhEditableControls{
 NSMutableArray *out=[NSMutableArray array];
 NSArray *names=@[@"A · 互动",@"B · 返回/跑步",@"X · 背包",@"Y · 拾取",@"L3",@"视角",@"L · 跳跃",@"R · 输入",@"ZL · 手机",@"ZR · 表情",@"+",@"−",@"工具左",@"工具环",@"工具右",@"收起工具",@"左上 L",@"截图"];
 for(int b=0;b<B_N;b++)if(b!=B_LS)[out addObject:acnh_pad_control([NSString stringWithFormat:@"button.%d",b],names[b],[self buttonCenter:b],[self radius:b])];
 [out addObject:acnh_pad_control(@"stick",@"移动摇杆",self.stickHome,self.stickR)];return out;
}
- (int)buttonAt:(CGPoint)p{
 int hit=-1;CGFloat best=1.08;
 for(int b=0;b<B_N;b++){
  if(b==B_LS||b==B_RS)continue;CGFloat normalized=distance(p,[self buttonCenter:b])/[self radius:b];
  if(normalized<best){best=normalized;hit=b;}
 }
 return hit;
}
- (void)toolTick:(CADisplayLink *)link{[self recompute];}
- (void)beginTool:(UITouch *)t button:(int)button{
 _toolTouch=t;_wheelButton=button;_toolOrigin=[self buttonCenter:button];acnh_wheel_begin_button(&_wheel,CACurrentMediaTime(),button);
 _toolLink=[CADisplayLink displayLinkWithTarget:self selector:@selector(toolTick:)];[_toolLink addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
}
- (void)cancelTool{_toolTouch=nil;acnh_wheel_cancel(&_wheel);[_toolLink invalidate];_toolLink=nil;}
- (void)recompute{
 uint64_t buttons=0;for(UITouch *t in _owner){uint64_t owner=[_owner objectForKey:t].unsignedLongLongValue;if((owner&255)==C_BTN)buttons|=1ull<<inputButton((int)(owner>>8));}
 AcnhAxis left=_stick?acnh_axis(_stickNow.x-self.stickHome.x,_stickNow.y-self.stickHome.y,self.stickR):(AcnhAxis){0};
 CGPoint rc=[self buttonCenter:B_RS];AcnhAxis right=_camera?acnh_axis(_cameraNow.x-rc.x,_cameraNow.y-rc.y,[self radius:B_RS]):(AcnhAxis){0};
 touch_feedback_update(_touchFeedback,&_feedbackEdges,buttons|(_captureTouch?(1ull<<63):0)|(_toolTouch?(1ull<<_wheelButton):0),(_stick?1u:0)|(_camera?2u:0),NO);
 AcnhWheelOutput tool=acnh_wheel_sample(&_wheel,CACurrentMediaTime());if(tool.active){buttons|=tool.buttons;left=tool.axis;}
 if(!_wheel.active){[_toolLink invalidate];_toolLink=nil;}
 _btnBits=buttons;_faceBits=0;pthread_mutex_lock(&g_mu);g_touch_btn=buttons;g_touch_lx=(int32_t)(left.x*32767);g_touch_ly=(int32_t)(left.y*32767);g_touch_rx=(int32_t)(right.x*32767);g_touch_ry=(int32_t)(right.y*32767);pthread_mutex_unlock(&g_mu);publish();[self refresh];
}
// Normalize inside the selected game picture, excluding letterboxing; control fingers never also reach game touch.
- (void)publishScreenTouches {
    if(acnh_tabletop_active(self.superview))return; // The upper game view owns direct screen touches.
    CGRect picture=[self convertRect:acnh_game_frame(self.superview) fromView:self.superview];
    dm_touch points[16];int n=0;
    for(UITouch *t in _screenTouches){CGPoint q;
        if(!acnh_game_touch([t locationInView:self],picture,&q))continue;
        points[n++]=(dm_touch){.id=[_screenTouches objectForKey:t].unsignedIntValue,.x=q.x,.y=q.y};if(n==16)break;
    }
    dm_touch_set(points,n);
}
- (void)captureScreenTouch:(UITouch *)t {
    BOOL used[16]={0};for(UITouch *old in _screenTouches)used[[_screenTouches objectForKey:old].unsignedIntValue]=YES;
    for(int i=0;i<16;i++)if(!used[i]){[_screenTouches setObject:@(i) forKey:t];break;}
}
// Probe only after R or a field tap, including the island-name and chat layouts.
- (void)probeTextKeyboard:(NSUInteger)remaining {
    if(_checkingTextField||_nativeTextOpen||_screenshotPending)return;
    _checkingTextField=YES;
    extern void mtl_grab_next_present(void (^cb)(CGImageRef));
    __weak DmGamepad *weakSelf=self;
    mtl_grab_next_present(^(CGImageRef img){
        BOOL matched=NO;
        if(img){
            unsigned char pixels[192*108*4];
            CGColorSpaceRef cs=CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
            CGContextRef c=CGBitmapContextCreate(pixels,192,108,8,192*4,cs,kCGImageAlphaPremultipliedLast|kCGBitmapByteOrder32Big);
            if(c){CGContextDrawImage(c,CGRectMake(0,0,192,108),img);matched=acnh_text_keyboard(pixels);CGContextRelease(c);}
            CGColorSpaceRelease(cs);
        }
        dispatch_async(dispatch_get_main_queue(), ^{
            DmGamepad *view=weakSelf;if(!view)return;view->_checkingTextField=NO;
            if(matched&&!view->_nativeTextOpen)[view nativeText];
            else if(remaining&&!view->_nativeTextOpen)dispatch_after(dispatch_time(DISPATCH_TIME_NOW,180*NSEC_PER_MSEC),dispatch_get_main_queue(),^{[weakSelf probeTextKeyboard:remaining-1];});
        });
    });
}
- (void)requestChatText {
    __weak DmGamepad *weakSelf=self;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,200*NSEC_PER_MSEC),dispatch_get_main_queue(),^{[weakSelf probeTextKeyboard:7];});
}
- (BOOL)requestTextFieldAt:(CGPoint)p {
    CGPoint q;CGRect picture=[self convertRect:acnh_game_frame(self.superview) fromView:self.superview];
    if(!acnh_game_touch(p,picture,&q))return NO;
    CGFloat x=q.x,y=q.y;
    if(x<.18||x>.82||y<.14||y>.34)return NO;
    if(_checkingTextField||_nativeTextOpen)return YES;
    [self probeTextKeyboard:0];return NO;
}

// Capture only the game render, excluding the virtual controls; use the camera export path.
- (void)captureGameScreen {
 if(_screenshotPending||_checkingTextField||_nativeTextOpen)return;
 _screenshotPending=YES;
 extern void mtl_grab_next_present(void (^cb)(CGImageRef));
 extern int dm_caps_save_image(CGImageRef,uint8_t[32]);
 __weak DmGamepad *weakSelf=self;
 mtl_grab_next_present(^(CGImageRef image){
  if(image)CGImageRetain(image);
  dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY,0),^{
   uint8_t entry[32]={0};if(image){dm_caps_save_image(image,entry);CGImageRelease(image);}
   dispatch_async(dispatch_get_main_queue(),^{DmGamepad *view=weakSelf;if(view)view->_screenshotPending=NO;});
  });
 });
}
// Each finger owns one control; B can stay held while a second finger moves the left stick.
- (void)touchesBegan:(NSSet<UITouch *> *)ts withEvent:(UIEvent *)event{
 for(UITouch *t in ts){
  CGPoint p=[t locationInView:self];int hit=[self buttonAt:p];
  if(hit==B_CAPTURE){if(!_captureTouch){_captureTouch=t;[_owner setObject:@(C_CAPTURE) forKey:t];[self captureGameScreen];}continue;}
  if(hit==B_UP||hit==B_ZR){if(!_wheel.active){[_owner setObject:@(C_TOOL) forKey:t];[self beginTool:t button:hit];}continue;}
  if(hit>=0){if(hit==B_R)[self requestChatText];if(hit==B_B)[self cancelTool];[_owner setObject:@(C_BTN|((uint64_t)hit<<8)) forKey:t];continue;}
  if(!_camera&&distance(p,[self buttonCenter:B_RS])<=[self radius:B_RS]*1.15){_camera=t;_cameraNow=p;[_owner setObject:@(C_CAMERA) forKey:t];continue;}
  if(!_stick&&distance(p,self.stickHome)<=self.stickR*1.15){_stick=t;_stickNow=p;[_owner setObject:@(C_STICK) forKey:t];continue;}
  if([self requestTextFieldAt:p])continue;[self captureScreenTouch:t];
 }
 [self recompute];[self publishScreenTouches];
}
- (void)touchesMoved:(NSSet<UITouch *> *)ts withEvent:(UIEvent *)event{
 for(UITouch *t in ts){int owner=[_owner objectForKey:t].unsignedLongLongValue&255;CGPoint p=[t locationInView:self];
  if(owner==C_STICK)_stickNow=p;else if(owner==C_CAMERA)_cameraNow=p;
  else if(owner==C_TOOL)acnh_wheel_move(&_wheel,p.x-_toolOrigin.x,p.y-_toolOrigin.y,self.toolWheelR);
 }
 [self recompute];[self publishScreenTouches];
}
- (void)finishTouches:(NSSet<UITouch *> *)ts cancelled:(BOOL)cancelled{
 for(UITouch *t in ts){
  if(t==_toolTouch){if(cancelled)[self cancelTool];else{CGPoint p=[t locationInView:self];acnh_wheel_move(&_wheel,p.x-_toolOrigin.x,p.y-_toolOrigin.y,self.toolWheelR);_toolTouch=nil;acnh_wheel_end(&_wheel,CACurrentMediaTime());}}
  if(t==_captureTouch)_captureTouch=nil;
  if(t==_stick)_stick=nil;if(t==_camera)_camera=nil;[_owner removeObjectForKey:t];[_screenTouches removeObjectForKey:t];
 }
 [self recompute];[self publishScreenTouches];
}
- (void)touchesEnded:(NSSet<UITouch *> *)ts withEvent:(UIEvent *)event{[self finishTouches:ts cancelled:NO];}
- (void)touchesCancelled:(NSSet<UITouch *> *)ts withEvent:(UIEvent *)event{[self finishTouches:ts cancelled:YES];}
- (void)reset{
 extern void dm_acnh_text_cancel(void);dm_acnh_text_cancel();[self cancelTool];[_screenTouches removeAllObjects];dm_touch_set(NULL,0);[_owner removeAllObjects];_stick=nil;_camera=nil;_captureTouch=nil;[self recompute];
}
- (void)layoutSubviews{
 [super layoutSubviews]; UIWindowScene *ws=self.window.windowScene; if(ws) atomic_store(&g_iface_orient,(int)ws.effectiveGeometry.interfaceOrientation);[CATransaction begin];[CATransaction setDisableActions:YES];
 for(int b=0;b<B_N;b++)if(_key[b]){
  CGPoint c=[self buttonCenter:b];CGFloat r=[self radius:b];setPath(_key[b],circlePath(c,r));
  if(b!=B_RS&&_glyph[b])positionGlyph(_glyph[b],c,r*(b==B_A?.98:1.10));
  if(_label[b]){_label[b].contentsScale=acnh_view_display_scale(self);placeLabel(_label[b],c,r*.85);}
 }
 setPath(_ring,circlePath(self.stickHome,self.stickR));setPath(_knob,circlePath(CGPointZero,self.stickR*.37));
 setPath(_rightKnob,circlePath(CGPointZero,[self radius:B_RS]*.64));positionGlyph(_glyph[B_RS],CGPointZero,[self radius:B_RS]*.84);
 [CATransaction commit];[self refresh];
}
- (void)safeAreaInsetsDidChange{[super safeAreaInsetsDidChange];[self setNeedsLayout];}
- (void)refresh{
 [CATransaction begin];[CATransaction setDisableActions:YES];
 for(int b=0;b<B_N;b++)if(_key[b]){
  BOOL on=(_btnBits&(1ull<<inputButton(b)))!=0;if(b==_wheelButton&&_wheel.active)on=YES;if(b==B_RS)on=_camera!=nil;if(b==B_CAPTURE)on=_captureTouch!=nil;
  _key[b].fillColor=[UIColor colorWithWhite:on?1:0 alpha:on?.30:(b>=B_LEFT&&b<=B_DOWN?.10:.15)].CGColor;
  _key[b].strokeColor=[UIColor colorWithRed:.92 green:.87 blue:.71 alpha:b>=B_LEFT&&b<=B_DOWN?.44:.60].CGColor;
  _glyph[b].strokeColor=[UIColor colorWithWhite:on?0:1 alpha:on?.8:.8].CGColor;
 }
 CGPoint h=self.stickHome;CGFloat dx=_stick?_stickNow.x-h.x:0,dy=_stick?_stickNow.y-h.y:0,n=hypot(dx,dy),r=self.stickR;
 // Full throw shows sprint; hysteresis prevents flicker at the rim.
 BOOL showRun=_stick && n/r>=(_showRunIcon?.94:.98);
 if(showRun!=_showRunIcon){_showRunIcon=showRun;setPath(_glyph[B_B],iconPath(showRun?B_N:B_B));}
 if(n>r){dx*=r/n;dy*=r/n;}
 _knob.position=CGPointMake(h.x+dx,h.y+dy);_ring.fillColor=[UIColor colorWithWhite:0 alpha:.025].CGColor;
 _ring.strokeColor=[UIColor colorWithWhite:1 alpha:_stick?.30:.14].CGColor;
 _knob.fillColor=[UIColor colorWithWhite:1 alpha:_stick?.15:.05].CGColor;_knob.strokeColor=[UIColor colorWithWhite:1 alpha:_stick?.40:.19].CGColor;
 h=[self buttonCenter:B_RS];dx=_camera?_cameraNow.x-h.x:0;dy=_camera?_cameraNow.y-h.y:0;n=hypot(dx,dy);r=[self radius:B_RS]*.65;
 if(n>r){dx*=r/n;dy*=r/n;}_rightKnob.position=CGPointMake(h.x+dx,h.y+dy);_rightKnob.fillColor=[UIColor colorWithWhite:1 alpha:_camera?.15:.04].CGColor;
 [CATransaction commit];
}

// Physical controllers ----
static pad_slots g_slots;
static GCController *g_held[DM_PLAYERS];   // the controller of each slot, kept alive while it holds it (the slot table keys on its pointer)
static int g_merge;
static uint64_t g_chat_buttons[DM_PLAYERS];
static int32_t axis(float v) { return (int32_t)(fmaxf(-1.f, fminf(1.f, v)) * 32767); }
static void read_pad(GCExtendedGamepad *g) {
    int p = g_merge ? 0 : pad_slot_of(&g_slots, (__bridge const void *)g.controller); if (p < 0) return;
    dm_haptics_apple_controller(p, g.controller);
    uint64_t m = 0;
    #define K(btn, bit) if (btn.isPressed) m |= 1ull << bit
    K(g.buttonB, B_A); K(g.buttonA, B_B); K(g.buttonY, B_X); K(g.buttonX, B_Y);   // by position: right A, bottom B, top X, left Y
    K(g.leftShoulder, B_L); K(g.rightShoulder, B_R); K(g.leftTrigger, B_ZL); K(g.rightTrigger, B_ZR);
    K(g.buttonMenu, B_PLUS); if (g.buttonOptions) K(g.buttonOptions, B_MINUS);
    if (g.leftThumbstickButton) K(g.leftThumbstickButton, B_LS); if (g.rightThumbstickButton) K(g.rightThumbstickButton, B_RS);
    K(g.dpad.left, B_LEFT); K(g.dpad.up, B_UP); K(g.dpad.right, B_RIGHT); K(g.dpad.down, B_DOWN);
    #undef K
    BOOL chatPressed=p==0 && (m&(1ull<<B_R)) && !(g_chat_buttons[p]&(1ull<<B_R));
    g_chat_buttons[p]=m;
    if(chatPressed)dispatch_async(dispatch_get_main_queue(),^{[g_view requestChatText];});
    dm_pad_set_player(p, m, axis(g.leftThumbstick.xAxis.value), axis(g.leftThumbstick.yAxis.value), axis(g.rightThumbstick.xAxis.value), axis(g.rightThumbstick.yAxis.value));
}
// Home and Capture: without this iOS takes them as system gestures (Home opens the Games overlay, Capture a screenshot); the app gives them no function
static void pad_keep_system_buttons(GCController *c) { for (NSString *b in @[GCInputButtonHome, GCInputButtonShare]) c.physicalInputProfile.buttons[b].preferredSystemGestureState = GCSystemGestureStateDisabled; }
static const char *pad_name(GCController *c) { return c.vendorName.UTF8String ?: "?"; }
static void npad_name(int p, char *out, size_t cap) { uint32_t id = dm_player_npad(p); if (id == 0x20) snprintf(out, cap, "handheld"); else snprintf(out, cap, "%u", id); }
static void pads_start(void) {
    const char *e = getenv("DM_PAD_MERGE"); g_merge = e && atoi(e);
    PAD_LOG("front end: %s", g_merge ? "every controller is player 1 (DM_PAD_MERGE)" : "each controller is a player of its own, the touch gamepad plays with player 1");
}
// Called at start and on every connect / disconnect (gone: the controller that was just removed, nil otherwise); returns whether a controller is in use
static BOOL pads_merged(GCController *gone) {
    if (gone) { gone.extendedGamepad.valueChangedHandler = nil; dm_motion_detach(gone, 0); }
    BOOL any = NO; int motion = 0;
    for (GCController *c in GCController.controllers) if (c != gone && c.extendedGamepad) {
        if (c.motion.hasRotationRate) motion = 1;
        any = YES; pad_keep_system_buttons(c); c.extendedGamepad.valueChangedHandler = ^(GCExtendedGamepad *g, GCControllerElement *el) { (void)el; read_pad(g); };
        dm_motion_attach(c, 0);   // every controller is player 1: the last one moved drives its six-axis sensor
        read_pad(c.extendedGamepad);
    }
    if (!any || !motion) dm_motion_clear(0);   // the phone's gyro (if on) takes over from its next sample
    if (!any) { dm_haptics_apple_controller(0, nil); dm_pad_set(0, 0, 0, 0, 0); }
    return any;
}
static BOOL pads_slotted(GCController *gone) {
    NSArray<GCController *> *now = GCController.controllers; char nb[16];
    for (int p = 0; p < DM_PLAYERS; p++) if (g_held[p] && (g_held[p] == gone || ![now containsObject:g_held[p]])) {
        npad_name(p, nb, sizeof nb); PAD_LOG("controller '%s' disconnected: player %d (npad %s) is free", pad_name(g_held[p]), p + 1, nb);
        g_held[p].extendedGamepad.valueChangedHandler = nil;
        dm_motion_detach(g_held[p], p);   // player 1's: the phone's gyro (if on) takes over from its next sample
        dm_haptics_apple_controller(p, nil);
        g_chat_buttons[p]=0;
        pad_slot_free(&g_slots, (__bridge const void *)g_held[p]); g_held[p] = nil;
        dm_pad_set_player(p, 0, 0, 0, 0, 0);   // nothing stays pressed
    }
    for (GCController *c in now) if (c.extendedGamepad && c != gone && pad_slot_of(&g_slots, (__bridge const void *)c) < 0) {
        int p = pad_slot_take(&g_slots, (__bridge const void *)c, pad_name(c));
        if (p < 0) { PAD_LOG("controller '%s' connected: all %d players are taken, not used", pad_name(c), DM_PLAYERS); continue; }
        g_held[p] = c; pad_keep_system_buttons(c);
        c.extendedGamepad.valueChangedHandler = ^(GCExtendedGamepad *g, GCControllerElement *el) { (void)el; read_pad(g); };
        dm_motion_attach(c, p);
        npad_name(p, nb, sizeof nb); PAD_LOG("controller '%s' connected: player %d (npad %s)%s", pad_name(c), p + 1, nb, c.motion.hasRotationRate ? ", with motion (gyro)" : "");
        read_pad(c.extendedGamepad);
    }
    dm_pad_players(pad_slot_mask(&g_slots));
    return pad_slot_mask(&g_slots) != 0;
}

// Submit system-keyboard text to the in-game keyboard; never auto-confirms an irreversible name.
- (void)nativeText {
    if(_nativeTextOpen)return;_nativeTextOpen=YES;
    [self reset];
    UIViewController *vc=self.window.rootViewController;while(vc.presentedViewController)vc=vc.presentedViewController;
    UIAlertController *alert=[UIAlertController alertControllerWithTitle:@"输入文字" message:@"请在游戏的名字或文字输入界面使用。输入后回到游戏，检查文字再确认。" preferredStyle:UIAlertControllerStyleAlert];
    [alert addTextFieldWithConfigurationHandler:^(UITextField *f){f.placeholder=@"名字或文字";f.autocorrectionType=UITextAutocorrectionTypeNo;f.spellCheckingType=UITextSpellCheckingTypeNo;f.autocapitalizationType=UITextAutocapitalizationTypeNone;}];
    [alert addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:^(UIAlertAction *a){self->_nativeTextOpen=NO;}]];
    [alert addAction:[UIAlertAction actionWithTitle:@"输入" style:UIAlertActionStyleDefault handler:^(UIAlertAction *a){
        self->_nativeTextOpen=NO;NSString *text=alert.textFields.firstObject.text;if(!text.length)return;
        if(text.length>256){NSUInteger count=256;unichar last=[text characterAtIndex:count-1];if(last>=0xd800 && last<=0xdbff)count--;text=[text substringToIndex:count];}
        unichar units[256];[text getCharacters:units range:NSMakeRange(0,text.length)];
        extern int dm_acnh_text_submit(const uint16_t *,size_t);
        if(!dm_acnh_text_submit(units,text.length)){
            UIAlertController *error=[UIAlertController alertControllerWithTitle:@"尚有文字正在输入" message:@"请稍后重试。" preferredStyle:UIAlertControllerStyleAlert];[error addAction:[UIAlertAction actionWithTitle:@"好" style:UIAlertActionStyleDefault handler:nil]];[vc presentViewController:error animated:YES completion:nil];
        }
    }]];
    [vc presentViewController:alert animated:YES completion:^{[alert.textFields.firstObject becomeFirstResponder];}];
}
- (void)padsChanged:(NSNotification *)note {
 GCController *gone=[note.name isEqual:GCControllerDidDisconnectNotification]?note.object:nil;
 BOOL any=g_merge?pads_merged(gone):pads_slotted(gone);
 dispatch_async(dispatch_get_main_queue(), ^{self.hidden = any; if(any)[self reset];});
}

@end

static UIView *acnh_gamepad_view(CGRect frame) { DmGamepad *v = [[DmGamepad alloc] initWithFrame:frame]; g_view = v; return v; }
static void acnh_gamepad_reset(void) {   // On resign active: release everything (a key held while switching away and released in background gets no "release")
    dispatch_async(dispatch_get_main_queue(), ^{ [g_view reset]; });
    pthread_mutex_lock(&g_mu); g_shake_btn=0; pthread_mutex_unlock(&g_mu); publish();
    if(g_merge) dm_pad_set(0,0,0,0,0);
    else for(int p=0;p<DM_PLAYERS;p++) if(g_held[p]) dm_pad_set_player(p,0,0,0,0,0);
    memset(g_chat_buttons,0,sizeof g_chat_buttons);

}

// Select once, after the launcher applies its persisted settings. Only the active
// implementation registers controller observers and publishes input state.
extern UIView *dm_generic_gamepad_view(CGRect frame);
extern void dm_generic_gamepad_reset(void);
static BOOL g_generic_layout;
UIView *dm_gamepad_view(CGRect frame) {
    const char *layout = getenv("ACNH_CONTROLLER_LAYOUT");
    g_generic_layout = layout && (!strcmp(layout, "generic") || (!strcmp(layout, "custom") && getenv("ACNH_CUSTOM_BASE") && !strcmp(getenv("ACNH_CUSTOM_BASE"), "generic")));
    return g_generic_layout ? dm_generic_gamepad_view(frame) : acnh_gamepad_view(frame);
}
void dm_gamepad_reset(void) {
    if (g_generic_layout) dm_generic_gamepad_reset(); else acnh_gamepad_reset();
}

UIView *dm_acnh_gamepad_preview(CGRect frame){return [[DmGamepad alloc] initWithFrame:frame];}
