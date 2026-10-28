#import "PadCustomization.h"
#import "PadLayoutMath.h"
#import "AdaptiveGameLayout.h"
#include <stdlib.h>
#include <string.h>
static NSString *editBase;
static NSMutableDictionary *draft;
static NSMutableDictionary *runtime;
static NSString *storeKey(NSString *base){return [@"ACNH.CustomPad.v1." stringByAppendingString:base];}
static NSString *baseChoice(void){const char *v=getenv("ACNH_CUSTOM_BASE");return v&&!strcmp(v,"generic")?@"generic":@"acnh";}
BOOL acnh_pad_editing(void){return editBase!=nil;}
BOOL acnh_pad_custom(NSString *base){if(editBase)return [editBase isEqual:base];const char *v=getenv("ACNH_CONTROLLER_LAYOUT");return v&&!strcmp(v,"custom")&&[baseChoice() isEqual:base];}
static NSDictionary *values(NSString *base){
 if(!acnh_pad_custom(base))return nil;if(editBase)return draft;
 if(!runtime)runtime=[NSMutableDictionary dictionary];
 if(!runtime[base]){id saved=[NSUserDefaults.standardUserDefaults dictionaryForKey:storeKey(base)];runtime[base]=[saved isKindOfClass:NSDictionary.class]?saved:@{};}
 return runtime[base];
}
static CGFloat number(id v,CGFloat fallback){return pad_number(v,fallback);}
CGFloat acnh_pad_scale(NSString *base,NSString *key){return pad_scale(values(base),key);}
CGPoint acnh_pad_point(NSString *base,NSString *key,CGPoint p,UIView *v,CGFloat margin){
 return acnh_control_point(v,pad_point(values(base),key,p,v.bounds,UIEdgeInsetsInsetRect(v.bounds,v.safeAreaInsets),margin),margin);
}
NSDictionary *acnh_pad_control(NSString *key,NSString *name,CGPoint p,CGFloat r){return @{ @"key":key,@"name":name,@"x":@(p.x),@"y":@(p.y),@"r":@(r)};}
static NSString *L(NSString *en,NSString *zh){return [NSLocale.preferredLanguages.firstObject hasPrefix:@"zh"]?zh:en;}
@interface PadLayoutEditor:UIViewController
@property NSString *base;
@end
@implementation PadLayoutEditor {
 UIView<ACNHEditablePad> *_pad;NSMutableArray<UIButton *> *_handles;NSString *_selected;UILabel *_title;UISlider *_size;UIButton *_choose;UIView *_panel;CGPoint _panelOffset;
}
// The editor is a full-screen modal, so it must declare its own directions;
// it does not inherit the landscape launcher's orientation policy.
- (UIInterfaceOrientationMask)supportedInterfaceOrientations{
 return acnh_has_hinge(self.view)?UIInterfaceOrientationMaskAllButUpsideDown:UIInterfaceOrientationMaskLandscape;
}
- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation{
 UIInterfaceOrientation current=self.presentingViewController.view.window.windowScene.interfaceOrientation;
 if(acnh_has_hinge(self.presentingViewController.view)&&current!=UIInterfaceOrientationUnknown)return current;
 return UIInterfaceOrientationIsLandscape(current)?current:UIInterfaceOrientationLandscapeRight;
}
- (BOOL)shouldAutorotate{return YES;}
- (void)viewDidAppear:(BOOL)animated{
 [super viewDidAppear:animated];[self setNeedsUpdateOfSupportedInterfaceOrientations];
 UIWindowScene *scene=self.view.window.windowScene;
 if(UIDevice.currentDevice.userInterfaceIdiom==UIUserInterfaceIdiomPhone&&!acnh_has_hinge(self.view)&&scene&&!UIInterfaceOrientationIsLandscape(scene.interfaceOrientation)){
  UIWindowSceneGeometryPreferencesIOS *geometry=[[UIWindowSceneGeometryPreferencesIOS alloc]initWithInterfaceOrientations:UIInterfaceOrientationMaskLandscape];
  [scene requestGeometryUpdateWithPreferences:geometry errorHandler:^(NSError *error){NSLog(@"Pad editor landscape request: %@",error.localizedDescription);}];
 }
}
- (void)viewDidLoad{
 [super viewDidLoad];self.view.backgroundColor=[UIColor colorWithRed:.05 green:.13 blue:.15 alpha:1];acnh_observe_display(self.view);
 editBase=self.base;draft=[NSMutableDictionary dictionary];
 NSDictionary *saved=[NSUserDefaults.standardUserDefaults dictionaryForKey:storeKey(self.base)];
 for(id key in saved)if([key isKindOfClass:NSString.class] && [saved[key] isKindOfClass:NSDictionary.class])draft[key]=saved[key];
 _pad=(id)([self.base isEqual:@"generic"]?dm_generic_gamepad_view(self.view.bounds):dm_acnh_gamepad_preview(self.view.bounds));_pad.userInteractionEnabled=NO;[self.view addSubview:_pad];
 _handles=[NSMutableArray array];
 _panel=[UIView new];_panel.backgroundColor=[UIColor colorWithWhite:.08 alpha:.96];_panel.layer.cornerRadius=14;[self.view addSubview:_panel];
 _title=[UILabel new];_title.text=L(@"Drag controls; drag this panel to move it",@"拖动按键调整位置，也可拖动此面板");_title.userInteractionEnabled=YES;[_title addGestureRecognizer:[[UIPanGestureRecognizer alloc]initWithTarget:self action:@selector(movePanel:)]];_title.font=[UIFont systemFontOfSize:12];_title.textColor=UIColor.whiteColor;_title.textAlignment=NSTextAlignmentCenter;[_panel addSubview:_title];
 NSArray *names=@[L(@"Cancel",@"取消"),L(@"Reset",@"复位"),L(@"Save",@"保存")];SEL actions[]={@selector(cancel),@selector(resetLayout),@selector(save)};
 for(int i=0;i<3;i++){UIButton *b=[UIButton buttonWithType:UIButtonTypeSystem];[b setTitle:names[i] forState:UIControlStateNormal];b.tag=100+i;[b addTarget:self action:actions[i] forControlEvents:UIControlEventTouchUpInside];[_panel addSubview:b];}
 _choose=[UIButton buttonWithType:UIButtonTypeSystem];_choose.showsMenuAsPrimaryAction=YES;[_panel addSubview:_choose];
 _size=[UISlider new];_size.minimumValue=.5;_size.maximumValue=2;[_size addTarget:self action:@selector(resize:) forControlEvents:UIControlEventValueChanged];[_panel addSubview:_size];
 [self rebuild];
}
- (void)viewDidLayoutSubviews{
 [super viewDidLayoutSubviews];_pad.frame=acnh_pad_frame(self.view);[_pad setNeedsLayout];[_pad layoutIfNeeded];
 CGFloat w=MIN(400,self.view.bounds.size.width-32),x=(self.view.bounds.size.width-w)/2,y=self.view.bounds.size.height-self.view.safeAreaInsets.bottom-110;
 x=MAX(0,MIN(self.view.bounds.size.width-w,x+_panelOffset.x));y=MAX(self.view.safeAreaInsets.top,MIN(self.view.bounds.size.height-104,y+_panelOffset.y));
 _panel.frame=CGRectMake(x,y,w,104);_title.frame=CGRectMake(4,2,w-8,20);
 for(int i=0;i<3;i++)[_panel viewWithTag:100+i].frame=CGRectMake(i*w/3,72,w/3,30);
 _choose.frame=CGRectMake(8,26,120,38);_size.frame=CGRectMake(135,26,w-150,38);[self refresh];
}
- (void)rebuild{
 for(UIView *v in _handles)[v removeFromSuperview];[_handles removeAllObjects];
 NSArray *controls=[_pad acnhEditableControls];NSMutableArray *menu=[NSMutableArray array];
 for(NSDictionary *d in controls){UIButton *b=[UIButton buttonWithType:UIButtonTypeCustom];b.accessibilityIdentifier=d[@"key"];b.accessibilityLabel=d[@"name"];b.layer.borderWidth=1;b.backgroundColor=[[UIColor systemTealColor] colorWithAlphaComponent:.09];[b addTarget:self action:@selector(selectControl:) forControlEvents:UIControlEventTouchDown];[b addGestureRecognizer:[[UIPanGestureRecognizer alloc]initWithTarget:self action:@selector(drag:)]];[self.view insertSubview:b belowSubview:_panel];[_handles addObject:b];
  NSString *key=d[@"key"],*name=d[@"name"];__weak PadLayoutEditor *weakSelf=self;
  [menu addObject:[UIAction actionWithTitle:name image:nil identifier:nil handler:^(__kindof UIAction *a){PadLayoutEditor *editor=weakSelf;if(!editor)return;editor->_selected=key;[editor refresh];}]];
 }
 _choose.menu=[UIMenu menuWithChildren:menu];if(!_selected&&controls.count)_selected=controls[0][@"key"];[self refresh];
}
- (void)refresh{
 [_pad setNeedsLayout];[_pad layoutIfNeeded];NSArray *controls=[_pad acnhEditableControls];
 for(NSUInteger i=0;i<MIN(controls.count,_handles.count);i++){NSDictionary *d=controls[i];UIButton *b=_handles[i];CGFloat r=MAX(14,[d[@"r"] doubleValue]);CGPoint c=[_pad convertPoint:CGPointMake([d[@"x"] doubleValue],[d[@"y"] doubleValue]) toView:self.view];b.frame=CGRectMake(c.x-r,c.y-r,r*2,r*2);b.layer.cornerRadius=r;BOOL selected=[_selected isEqual:d[@"key"]];b.layer.borderColor=(selected?UIColor.systemYellowColor:UIColor.systemTealColor).CGColor;if(selected){[_choose setTitle:d[@"name"] forState:UIControlStateNormal];_size.value=acnh_pad_scale(self.base,_selected);}}
 [self.view bringSubviewToFront:_panel];
}
- (void)movePanel:(UIPanGestureRecognizer *)g{CGPoint p=[g translationInView:self.view];_panelOffset.x+=p.x;_panelOffset.y+=p.y;[g setTranslation:CGPointZero inView:self.view];[self.view setNeedsLayout];}
- (void)selectControl:(UIButton *)b{_selected=b.accessibilityIdentifier;[self refresh];}
- (void)drag:(UIPanGestureRecognizer *)g{
 UIButton *b=(id)g.view;_selected=b.accessibilityIdentifier;CGPoint delta=[g translationInView:self.view];[g setTranslation:CGPointZero inView:self.view];
 NSMutableDictionary *d=[draft[_selected] mutableCopy]?:[NSMutableDictionary dictionary];
 // Use the currently clamped position as the next origin, avoiding accumulated off-screen drag.
 CGPoint before=b.center;CGFloat r=b.bounds.size.width/2;CGRect safe=[_pad convertRect:UIEdgeInsetsInsetRect(_pad.bounds,_pad.safeAreaInsets) toView:self.view];
 CGPoint want=CGPointMake(MAX(CGRectGetMinX(safe)+r,MIN(CGRectGetMaxX(safe)-r,before.x+delta.x)),MAX(CGRectGetMinY(safe)+r,MIN(CGRectGetMaxY(safe)-r,before.y+delta.y)));
 d[@"dx"]=@(number(d[@"dx"],0)+(want.x-before.x)/_pad.bounds.size.width);d[@"dy"]=@(number(d[@"dy"],0)+(want.y-before.y)/_pad.bounds.size.height);draft[_selected]=d;[self refresh];
}
- (void)resize:(UISlider *)s{if(!_selected)return;NSMutableDictionary *d=[draft[_selected] mutableCopy]?:[NSMutableDictionary dictionary];d[@"scale"]=@(s.value);
 // Keep the center fixed while resizing; the safe-area clamp is applied below.
 draft[_selected]=d;[self refresh];}
- (void)resetLayout{[draft removeAllObjects];[self refresh];}
- (void)finish{[_pad removeFromSuperview];_pad=nil;editBase=nil;draft=nil;[self dismissViewControllerAnimated:YES completion:nil];}
- (void)cancel{[self finish];}
- (void)save{[NSUserDefaults.standardUserDefaults setObject:draft.copy forKey:storeKey(self.base)];runtime=nil;[self finish];}
- (BOOL)prefersHomeIndicatorAutoHidden{return YES;}
@end
void dm_edit_pad_layout(void *parent,const char *base){
 UIViewController *vc=(__bridge UIViewController *)parent;PadLayoutEditor *editor=[PadLayoutEditor new];editor.base=base&&!strcmp(base,"generic")?@"generic":@"acnh";editor.modalPresentationStyle=UIModalPresentationFullScreen;editor.modalInPresentation=YES;[vc presentViewController:editor animated:YES completion:nil];
}
