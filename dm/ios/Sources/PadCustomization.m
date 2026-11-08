#import "PadCustomization.h"
#import "PadLayoutMath.h"
#import "AdaptiveGameLayout.h"
#include <stdlib.h>
#include <string.h>
static NSString *editBase;
static NSMutableDictionary *draft;
static NSMutableDictionary *runtime;
static NSMutableDictionary *drafts;
static NSString *geometryVariant=@"";
static NSString *storeKey(NSString *base){return pad_store_key(base,geometryVariant);}
// Landscape keeps the existing saved layout. Portrait starts from its own
// template and persists separately for iPad and hinge-capable devices.
void acnh_pad_use_geometry(UIView *view){
 NSString *variant=acnh_portrait_companion_active(view)?(acnh_has_hinge(view)?@".portrait.duo":@".portrait.ipad"):@"";
 geometryVariant=variant;
 if(editBase&&drafts){
  if(!drafts[variant]){
   NSDictionary *saved=[NSUserDefaults.standardUserDefaults dictionaryForKey:storeKey(editBase)];
   NSMutableDictionary *next=[NSMutableDictionary dictionary];
   for(id key in saved)if([key isKindOfClass:NSString.class]&&[saved[key] isKindOfClass:NSDictionary.class])next[key]=saved[key];
   drafts[variant]=next;
  }
  draft=drafts[variant];
 }
}
static NSString *baseChoice(void){const char *v=getenv("ACNH_CUSTOM_BASE");return v&&!strcmp(v,"generic")?@"generic":@"acnh";}
BOOL acnh_pad_editing(void){return editBase!=nil;}
BOOL acnh_pad_custom(NSString *base){if(editBase)return [editBase isEqual:base];const char *v=getenv("ACNH_CONTROLLER_LAYOUT");return v&&!strcmp(v,"custom")&&[baseChoice() isEqual:base];}
static NSDictionary *values(NSString *base){
 if(!acnh_pad_custom(base))return nil;if(editBase)return draft;
 if(!runtime)runtime=[NSMutableDictionary dictionary];
 NSString *cacheKey=storeKey(base);
 if(!runtime[cacheKey]){id saved=[NSUserDefaults.standardUserDefaults dictionaryForKey:storeKey(base)];runtime[cacheKey]=[saved isKindOfClass:NSDictionary.class]?saved:@{};}
 return runtime[cacheKey];
}
static CGFloat number(id v,CGFloat fallback){return pad_number(v,fallback);}
CGFloat acnh_pad_scale(NSString *base,NSString *key){return pad_scale(values(base),key);}
CGPoint acnh_pad_point_avoiding(NSString *base,NSString *key,CGPoint p,UIView *v,CGFloat margin,CGRect peer){
 CGRect safe=UIEdgeInsetsInsetRect(v.bounds,v.safeAreaInsets);
 BOOL portrait=acnh_companion_controls_active(v);
 NSDictionary *layout=values(base);
 NSDictionary *record=pad_record(layout,key);
 BOOL keepOverlayDefault=[base isEqualToString:@"generic"]&&acnh_landscape_pad_overlay(v)&&!pad_has_position(record);
 if(keepOverlayDefault){
  if(record)p=pad_clamp_center(p,v.bounds,margin);
  return acnh_control_point(v,p,margin,YES,peer);
 }
 if(portrait)margin=MAX(14,margin);
 p=pad_point(layout,key,p,v.bounds,safe,margin);
 if(portrait)p=pad_clamp_center(p,safe,margin);
 return acnh_control_point(v,p,margin,NO,CGRectNull);
}
CGPoint acnh_pad_point(NSString *base,NSString *key,CGPoint p,UIView *v,CGFloat margin){
 return acnh_pad_point_avoiding(base,key,p,v,margin,CGRectNull);
}
NSDictionary *acnh_pad_control(NSString *key,NSString *name,CGPoint p,CGFloat r){return @{ @"key":key,@"name":name,@"x":@(p.x),@"y":@(p.y),@"r":@(r)};}
static NSString *L(NSString *en,NSString *zh){return [NSLocale.preferredLanguages.firstObject hasPrefix:@"zh"]?zh:en;}
@interface PadLayoutEditor:UIViewController
@property NSString *base;
@end
@implementation PadLayoutEditor {
 UIView *_gamePreview,*_lowerPreview;UILabel *_gameLabel,*_lowerLabel;BOOL _split;
 UIView<ACNHEditablePad> *_pad;NSMutableArray<UIButton *> *_handles;NSString *_selected;UILabel *_title;UISlider *_size;UIButton *_choose;UIView *_panel;CGPoint _panelOffset;
}
// The editor is a full-screen modal, so it must declare its own directions;
// it does not inherit the landscape launcher's orientation policy.
- (UIInterfaceOrientationMask)supportedInterfaceOrientations{
 return acnh_allows_companion(self.view)?UIInterfaceOrientationMaskAllButUpsideDown:UIInterfaceOrientationMaskLandscape;
}
- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation{
 UIInterfaceOrientation current=self.presentingViewController.view.window.windowScene.interfaceOrientation;
 if(acnh_allows_companion(self.presentingViewController.view)&&current!=UIInterfaceOrientationUnknown)return current;
 return UIInterfaceOrientationIsLandscape(current)?current:UIInterfaceOrientationLandscapeRight;
}
- (BOOL)shouldAutorotate{return YES;}
- (void)viewDidAppear:(BOOL)animated{
 [super viewDidAppear:animated];[self setNeedsUpdateOfSupportedInterfaceOrientations];
 [self.view setNeedsLayout];[self.view layoutIfNeeded];
 UIWindowScene *scene=self.view.window.windowScene;
 if(!acnh_allows_companion(self.view)&&scene&&!UIInterfaceOrientationIsLandscape(scene.interfaceOrientation)){
  UIWindowSceneGeometryPreferencesIOS *geometry=[[UIWindowSceneGeometryPreferencesIOS alloc]initWithInterfaceOrientations:UIInterfaceOrientationMaskLandscape];
  [scene requestGeometryUpdateWithPreferences:geometry errorHandler:^(NSError *error){NSLog(@"Pad editor landscape request: %@",error.localizedDescription);}];
 }
}
- (void)viewSafeAreaInsetsDidChange{
 [super viewSafeAreaInsetsDidChange];[self.view setNeedsLayout];
}
- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator{
 [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
 [coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context){
  [self.view setNeedsLayout];[self.view layoutIfNeeded];
 } completion:^(id<UIViewControllerTransitionCoordinatorContext> context){
  // Recompute from final modal bounds, not the presentation/rotation snapshot.
  [self.view setNeedsLayout];[self.view layoutIfNeeded];
 }];
}
- (void)viewDidLoad{
 [super viewDidLoad];self.view.backgroundColor=[UIColor colorWithRed:.05 green:.13 blue:.15 alpha:1];acnh_observe_display(self.view);
 editBase=self.base;drafts=[NSMutableDictionary dictionary];acnh_pad_use_geometry(self.view);
 _gamePreview=[UIView new];_gamePreview.backgroundColor=UIColor.blackColor;_gamePreview.layer.cornerRadius=10;[self.view addSubview:_gamePreview];
 _lowerPreview=[UIView new];_lowerPreview.backgroundColor=[UIColor colorWithWhite:.13 alpha:1];_lowerPreview.layer.cornerRadius=10;[self.view addSubview:_lowerPreview];
 _gameLabel=[UILabel new];_gameLabel.textColor=UIColor.lightGrayColor;_gameLabel.textAlignment=NSTextAlignmentCenter;[_gamePreview addSubview:_gameLabel];
 _lowerLabel=[UILabel new];_lowerLabel.text=L(@"Lower screen · drag controls here",@"下屏 · 在此拖动按键");_lowerLabel.textColor=UIColor.lightGrayColor;_lowerLabel.textAlignment=NSTextAlignmentCenter;[_lowerPreview addSubview:_lowerLabel];
 _pad=(id)([self.base isEqual:@"generic"]?dm_generic_gamepad_view(self.view.bounds):dm_acnh_gamepad_preview(self.view.bounds));_pad.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;_pad.userInteractionEnabled=NO;[self.view addSubview:_pad];
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
 [super viewDidLayoutSubviews];_split=acnh_portrait_companion_active(self.view)||acnh_tabletop_active(self.view);
 _gamePreview.hidden=_lowerPreview.hidden=!_split;
 BOOL splitPortrait=acnh_split_portrait_companion_active(self.view);
 CGRect padFrame=acnh_pad_frame(self.view);
 _gamePreview.frame=splitPortrait?acnh_game_viewport(self.view):acnh_game_frame(self.view);
 _lowerPreview.frame=splitPortrait?acnh_companion_frame(self.view):padFrame;
 _gameLabel.text=splitPortrait?L(@"Game screen",@"游戏画面"):L(@"Game screen · 16:9",@"游戏画面 · 16:9");
 _gameLabel.frame=_gamePreview.bounds;_lowerLabel.frame=CGRectMake(0,0,_lowerPreview.bounds.size.width,30);
 _pad.frame=padFrame;_pad.clipsToBounds=_split;[_pad setNeedsLayout];[_pad layoutIfNeeded];
 CGFloat w=MIN(400,self.view.bounds.size.width-32),x=(self.view.bounds.size.width-w)/2,y=_split?MAX(self.view.safeAreaInsets.top,CGRectGetMaxY(_gamePreview.frame)-110):self.view.bounds.size.height-self.view.safeAreaInsets.bottom-110;
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
 for(NSUInteger i=0;i<MIN(controls.count,_handles.count);i++){NSDictionary *d=controls[i];UIButton *b=_handles[i];CGFloat r=MAX(14,[d[@"r"] doubleValue]);b.hidden=_split&&(2*r>_pad.bounds.size.width||2*r>_pad.bounds.size.height);CGPoint c=[_pad convertPoint:CGPointMake([d[@"x"] doubleValue],[d[@"y"] doubleValue]) toView:self.view];b.frame=CGRectMake(c.x-r,c.y-r,r*2,r*2);b.layer.cornerRadius=r;BOOL selected=[_selected isEqual:d[@"key"]];b.layer.borderColor=(selected?UIColor.systemYellowColor:UIColor.systemTealColor).CGColor;if(selected){[_choose setTitle:d[@"name"] forState:UIControlStateNormal];_size.value=acnh_pad_scale(self.base,_selected);}}
 [self.view bringSubviewToFront:_panel];
}
- (void)movePanel:(UIPanGestureRecognizer *)g{CGPoint p=[g translationInView:self.view];_panelOffset.x+=p.x;_panelOffset.y+=p.y;[g setTranslation:CGPointZero inView:self.view];[self.view setNeedsLayout];}
- (void)selectControl:(UIButton *)b{_selected=b.accessibilityIdentifier;[self refresh];}
- (void)drag:(UIPanGestureRecognizer *)g{
 UIButton *b=(id)g.view;_selected=b.accessibilityIdentifier;CGPoint delta=[g translationInView:self.view];[g setTranslation:CGPointZero inView:self.view];
 CGRect bounds=_pad.bounds,safe=UIEdgeInsetsInsetRect(bounds,_pad.safeAreaInsets);
 if(CGRectIsEmpty(safe))return;
 NSMutableDictionary *d=[draft[_selected] mutableCopy]?:[NSMutableDictionary dictionary];
 CGPoint before=[_pad convertPoint:b.center fromView:self.view];
 CGFloat radius=b.bounds.size.width/2;
 CGPoint want=pad_clamp_center(CGPointMake(before.x+delta.x,before.y+delta.y),safe,radius);
 if(acnh_portrait_companion_active(self.view)){
  // Store the visible position so a clamped legacy offset or resize cannot
  // leave dead travel when the next drag moves back from the edge.
  d[@"x"]=@((want.x-CGRectGetMinX(bounds))/bounds.size.width);
  d[@"y"]=@((want.y-CGRectGetMinY(bounds))/bounds.size.height);
 }else{
  d[@"dx"]=@(number(d[@"dx"],0)+(want.x-before.x)/bounds.size.width);
  d[@"dy"]=@(number(d[@"dy"],0)+(want.y-before.y)/bounds.size.height);
 }
 draft[_selected]=d;[self refresh];
}
- (void)resize:(UISlider *)s{if(!_selected)return;NSMutableDictionary *d=[draft[_selected] mutableCopy]?:[NSMutableDictionary dictionary];d[@"scale"]=@(s.value);
 // Keep the center fixed while resizing; the safe-area clamp is applied below.
 draft[_selected]=d;[self refresh];}
- (void)resetLayout{[draft removeAllObjects];[self refresh];}
- (void)finish{[_pad removeFromSuperview];_pad=nil;editBase=nil;draft=nil;drafts=nil;acnh_pad_use_geometry(self.presentingViewController.view);[self dismissViewControllerAnimated:YES completion:nil];}
- (void)cancel{[self finish];}
- (void)save{for(NSString *variant in drafts){NSString *key=pad_store_key(self.base,variant);[NSUserDefaults.standardUserDefaults setObject:[drafts[variant] copy] forKey:key];}runtime=nil;[self finish];}
- (BOOL)prefersHomeIndicatorAutoHidden{return YES;}
@end
void dm_edit_pad_layout(void *parent,const char *base){
 UIViewController *vc=(__bridge UIViewController *)parent;PadLayoutEditor *editor=[PadLayoutEditor new];editor.base=base&&!strcmp(base,"generic")?@"generic":@"acnh";editor.modalPresentationStyle=UIModalPresentationFullScreen;editor.modalInPresentation=YES;[vc presentViewController:editor animated:YES completion:nil];
}
