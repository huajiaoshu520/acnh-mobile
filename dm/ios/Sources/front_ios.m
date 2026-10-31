extern void acnh_wide_set_aspect(float aspect);
extern void acnh_wide_set_present_full(int on);
// iOS front end: a clipped Metal picture with selectable fit/stretch/crop, plus the existing independent virtual gamepad layer.
// The host runs on the big-stack thread started by main.m; front_run runs the guest directly on the calling thread.
#import <UIKit/UIKit.h>
#import "ACNHResourceGate.h"
#import "AdaptiveGameLayout.h"
int front_ios_load_resources(const char *root);
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>
#import <AVFAudio/AVFAudio.h>
#include <errno.h>
#include <stdatomic.h>
#include "front.h"
#include "hle.h"
#include "dm.h"
#include "haptics_apple.h"
#include "save_sync.h"
#import "CloudSaves.h"

UIView *dm_gamepad_view(CGRect frame);
void dm_gamepad_reset(void);
void dm_gamepad_active(void);
void front_ios_boot(void);
int front_ios_boot_ok(void);
const char *front_ios_device_message(void);
bool dm_show_launcher(void *view_controller, void (*done)(void));

static CAMetalLayer *g_layer,*g_minimap_layer;
static _Atomic int g_minimap_enabled;
void *front_acnh_minimap_layer(void) { return atomic_load(&g_minimap_enabled)?(__bridge void *)g_minimap_layer:NULL; }
static void front_ios_power_watch(void);
static void front_ios_audio_watch(void);
static UILabel *front_ios_fps_label(void);

@interface DmGameView : UIView
@property(nonatomic) BOOL separateTouch;
@property(nonatomic) BOOL stretchPicture;
- (void)resetScreenTouches;
@end
@implementation DmGameView { NSMapTable<UITouch *,NSNumber *> *_screenFingers; }
+ (Class)layerClass { return CAMetalLayer.class; }
- (instancetype)initWithFrame:(CGRect)f {
    if ((self = [super initWithFrame:f])) {
        CAMetalLayer *l = (CAMetalLayer *)self.layer;
        l.device = MTLCreateSystemDefaultDevice(); l.pixelFormat = MTLPixelFormatBGRA8Unorm;
        l.contentsScale = acnh_view_display_scale(self); l.framebufferOnly = NO; l.opaque = YES;
        self.backgroundColor = UIColor.blackColor;
        self.multipleTouchEnabled=YES;_screenFingers=[NSMapTable strongToStrongObjectsMapTable];
    }
    return self;
}
- (void)resetScreenTouches { [_screenFingers removeAllObjects];dm_touch_set(NULL,0); }
- (void)publishScreenTouches {
 if(!self.separateTouch)return;
 dm_touch points[16];int n=0;
 for(UITouch *t in _screenFingers){
  CGPoint p=[t locationInView:self];
  if(self.bounds.size.width<=0||self.bounds.size.height<=0)continue;
  points[n++]=(dm_touch){.id=[_screenFingers objectForKey:t].unsignedIntValue,
   .x=(float)fmin(fmax(p.x/self.bounds.size.width,0),.999),.y=(float)fmin(fmax(p.y/self.bounds.size.height,0),.999)};
 }
 dm_touch_set(points,n);
}
- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
 (void)event;if(!self.separateTouch)return;
 for(UITouch *t in touches){BOOL used[16]={0};for(NSNumber *slot in _screenFingers.objectEnumerator)used[slot.intValue]=YES;
  for(int i=0;i<16;i++)if(!used[i]){[_screenFingers setObject:@(i) forKey:t];break;}}
 [self publishScreenTouches];
}
- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { (void)touches;(void)event;[self publishScreenTouches]; }
- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
 (void)event;for(UITouch *t in touches)[_screenFingers removeObjectForKey:t];[self publishScreenTouches];
}
- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { [self touchesEnded:touches withEvent:event]; }
- (void)didMoveToWindow { [super didMoveToWindow];[self setNeedsLayout]; }
- (void)traitCollectionDidChange:(UITraitCollection *)previous {
    [super traitCollectionDidChange:previous];
    if(previous.displayScale!=self.traitCollection.displayScale)[self setNeedsLayout];
}
- (void)layoutSubviews {
    [super layoutSubviews];
    CAMetalLayer *l = (CAMetalLayer *)self.layer;
    CGFloat scale=acnh_view_display_scale(self);
    if(self.window.windowScene.session.role==UIWindowSceneSessionRoleExternalDisplayNonInteractive || [self.window.windowScene.session.role isEqualToString:UIWindowSceneSessionRoleExternalDisplayNonInteractive]) scale=MIN(scale,1920.0/MAX(self.bounds.size.width,1));
    l.contentsScale = scale;
    CGSize pixels=CGSizeMake(self.bounds.size.width*l.contentsScale,self.bounds.size.height*l.contentsScale);
    // Metal renders the original aspect; Core Animation scales it into the selected view.
    if(self.stretchPicture)pixels=acnh_fit_game(CGRectMake(0,0,pixels.width,pixels.height)).size;
    l.contentsGravity=kCAGravityResize;
    if(!CGSizeEqualToSize(l.drawableSize,pixels))l.drawableSize=pixels;
}
@end

static CGRect fit_16_9(CGRect b) { return acnh_fit_game(b); }
static void tv_attach(void);
static BOOL tv_active(void);

@interface DmRootVC : UIViewController
@property (nonatomic, strong) DmGameView *game;
@property (nonatomic, strong) UIView *gameContainer;
@property (nonatomic, strong) UIView *pad;
@property (nonatomic, strong) DmGameView *miniMap;
@property (nonatomic, strong) UILabel *fps;
@property (nonatomic,strong) UILabel *loadLabel;
@property (nonatomic) BOOL preparing;
@property (nonatomic) BOOL launched;
@property (nonatomic,strong) id tv;
- (void)startGame;
@end
static DmRootVC *g_root;
static void launcher_done(void) { [g_root startGame]; }
@implementation DmRootVC
- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = UIColor.blackColor;
    acnh_observe_display(self.view);
    front_ios_power_watch();
#if defined(__IPHONE_27_0)
    if (@available(iOS 27.0, *)) {
        UISceneConfiguration *c = [[UISceneConfiguration alloc] initWithName:@"TV" sessionRole:UIWindowSceneSessionRoleExternalDisplayNonInteractive];
        c.delegateClass = NSClassFromString(@"DmTVSceneDelegate");
        self.tv = [self registerSceneAccessory:[UISceneAccessory externalNonInteractiveSceneAccessoryWithConfiguration:c]];
    }
#endif
    self.loadLabel=[[UILabel alloc] initWithFrame:CGRectInset(self.view.bounds,40,40)];
    self.loadLabel.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
    self.loadLabel.numberOfLines=0;self.loadLabel.textAlignment=NSTextAlignmentCenter;self.loadLabel.textColor=UIColor.whiteColor;
    self.loadLabel.text=@"准备游戏资源…";[self.view addSubview:self.loadLabel];
}
- (void)startGame {
    dm_cloud_saves_env();
    if(self.game || !front_ios_boot_ok())return;
    if(getenv("DM_METAL_HUD") && atoi(getenv("DM_METAL_HUD")))setenv("MTL_HUD_ENABLED","1",0);
    [self.loadLabel removeFromSuperview];
    [self setupGame];
    [self.view setNeedsLayout];[self.view layoutIfNeeded];
    UIApplication.sharedApplication.idleTimerDisabled=YES;
    tv_attach();
    front_ios_boot();
}
- (void)setupGame {
    dm_haptics_apple_init();
    front_ios_audio_watch();
    self.game = [[DmGameView alloc] initWithFrame:self.view.bounds];
    self.gameContainer=[[UIView alloc] initWithFrame:self.view.bounds];
    self.gameContainer.clipsToBounds=YES;
    [self.view addSubview:self.gameContainer];[self.gameContainer addSubview:self.game];
    g_layer = (CAMetalLayer *)self.game.layer;
    self.pad = dm_gamepad_view(self.view.bounds);
    self.pad.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    [self.view addSubview:self.pad];
    self.miniMap=[[DmGameView alloc] initWithFrame:CGRectZero];
    self.miniMap.userInteractionEnabled=NO;self.miniMap.hidden=YES;
    [self.view insertSubview:self.miniMap belowSubview:self.pad];
    g_minimap_layer=(CAMetalLayer *)self.miniMap.layer;
    g_minimap_layer.allowsNextDrawableTimeout=YES;
    self.fps = front_ios_fps_label();   // above the pad, which hides while a controller is connected
    if (self.fps) [self.view addSubview:self.fps];
}
- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    if (!self.game) return;
    if(tv_active()) {
        self.pad.frame=acnh_pad_frame(self.view);
        self.miniMap.hidden=YES;atomic_store(&g_minimap_enabled,0);
        acnh_wide_set_aspect(16.f/9.f);acnh_wide_set_present_full(0);
        return;
    }
    CGRect previous=self.pad.frame;
    CGRect viewport=acnh_game_viewport(self.view);
    self.gameContainer.frame=viewport;
    BOOL expand=acnh_expanded_view_active(self.view);
    acnh_wide_set_aspect(expand?viewport.size.width/viewport.size.height:16.f/9.f);
    acnh_wide_set_present_full(expand||acnh_display_mode()==ACNH_DISPLAY_STRETCH);
    self.game.stretchPicture=NO; // Render into the native drawable; final GPU viewport controls presentation.
    self.game.frame=CGRectOffset(acnh_game_frame(self.view),-viewport.origin.x,-viewport.origin.y);
    static CGRect lastWideFrame;static int lastWideMode=-1;
    if(!CGRectEqualToRect(lastWideFrame,self.game.frame)||lastWideMode!=acnh_display_mode()){
        lastWideFrame=self.game.frame;lastWideMode=acnh_display_mode();
        fprintf(stderr,"[ACNH display] iOS mode=%d expand=%d root=%.1fx%.1f viewport=%.1fx%.1f game=%.1f,%.1f %.1fx%.1f\n",lastWideMode,expand,self.view.bounds.size.width,self.view.bounds.size.height,viewport.size.width,viewport.size.height,self.game.frame.origin.x,self.game.frame.origin.y,self.game.frame.size.width,self.game.frame.size.height);
    }
    self.pad.frame=acnh_pad_frame(self.view);
    BOOL tabletop=acnh_tabletop_active(self.view);
    self.game.separateTouch=tabletop;
    CGRect lower=self.pad.frame;
    CGFloat mw=MIN(lower.size.width*.40,lower.size.height*.62*398./310.);
    self.miniMap.frame=CGRectMake(CGRectGetMidX(lower)-mw/2,CGRectGetMidY(lower)-mw*310./398./2,mw,mw*310./398.);
    self.miniMap.hidden=!tabletop;
    [self.miniMap setNeedsLayout];[self.miniMap layoutIfNeeded];
    atomic_store(&g_minimap_enabled,tabletop);
    if(acnh_has_hinge(self.view)&&!CGRectEqualToRect(previous,self.pad.frame)){dm_gamepad_reset();[self.game resetScreenTouches];}
    [self.game setNeedsLayout];[self.pad setNeedsLayout];
    if (self.fps) {   // the top row's free stretch: right of DmGamepad.m's L3 key (102 + 17 of its units from the safe edge), level with − / + (22 units down); u is its unit
        CGRect s = acnh_has_hinge(self.view)?self.pad.frame:UIEdgeInsetsInsetRect(self.view.bounds,self.view.safeAreaInsets); CGFloat u = MIN((acnh_has_hinge(self.view)?s.size.height:self.view.bounds.size.height) / 430.0, 1.3);
        self.fps.frame = CGRectMake(floor(CGRectGetMinX(s) + 130 * u), floor(CGRectGetMinY(s) + 22 * u - 11), 64, 22);
    }
}
- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
    [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
    if(acnh_has_hinge(self.view))dm_gamepad_reset();
    __weak DmRootVC *weakSelf=self;
    [coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context){
        (void)context;[weakSelf.view setNeedsLayout];[weakSelf.view layoutIfNeeded];
    } completion:^(id<UIViewControllerTransitionCoordinatorContext> context){
        (void)context;[weakSelf.view setNeedsLayout];
        if(acnh_has_hinge(weakSelf.view))dm_gamepad_reset();
    }];
}
- (void)viewDidAppear:(BOOL)a {
    [super viewDidAppear:a];UIApplication.sharedApplication.idleTimerDisabled=YES;
    if(self.preparing || self.game)return;self.preparing=YES;
    const char *device=front_ios_device_message();
    if(device){self.loadLabel.text=@(device);return;} // before resource import, launcher or guest execution
    // Make the shared Documents directory nonempty even before game resources
    // are imported (and in a game-free shell), so Files has a visible target.
    NSString *doc=NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES).firstObject;
    NSError *directoryError=nil;
    if(![NSFileManager.defaultManager createDirectoryAtPath:[doc stringByAppendingPathComponent:@"ACNHMobile"] withIntermediateDirectories:YES attributes:nil error:&directoryError]){
        self.loadLabel.text=[NSString stringWithFormat:@"无法创建数据目录：%@",directoryError.localizedDescription];return;
    }
#ifdef DM_SHELL
    if (![NSBundle.mainBundle pathForResource:@"Guest" ofType:nil inDirectory:@"Frameworks/Guest.framework"]) {
        self.loadLabel.text=@"这是无游戏空壳。请在电脑上使用自己的游戏程序、配套着色器和字体补包，再整包签名安装。仅导入 .data 无法启动。详情见 docs/SHELL.zh-CN.md。";
        return;
    }
#endif
    acnh_prepare_resources(self,^(double p){self.loadLabel.text=[NSString stringWithFormat:@"导入与校验资源 %.0f%%",p*100];},^(NSString *folder,NSString *error){
        if(error){self.loadLabel.text=error;return;}
        dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED,0),^{
            int ok=front_ios_load_resources(folder.fileSystemRepresentation);
            dispatch_async(dispatch_get_main_queue(),^{if(!ok){self.loadLabel.text=@"资源启动失败，请查看 logs/log.txt。";return;}self.launched=YES;g_root=self;
                if(!dm_show_launcher((__bridge void *)self,launcher_done))[self startGame];});
        });
    });
}
- (BOOL)prefersHomeIndicatorAutoHidden { return YES; }
- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures { return UIRectEdgeAll; }   // Edge gestures need two swipes, to avoid accidental exits
- (BOOL)prefersStatusBarHidden { return YES; }
- (UIInterfaceOrientationMask)supportedInterfaceOrientations { return acnh_has_hinge(self.view)?UIInterfaceOrientationMaskAllButUpsideDown:UIInterfaceOrientationMaskLandscape; }
@end
UIViewController *front_ios_root(void) { return [DmRootVC new]; }

// A TV (cable or AirPlay): iOS gives the app the TV as a scene of its own instead of mirroring the phone. The game's view moves there, 16:9 fills the TV, and the game is
// docked as a Switch on its dock (1080p, the operation mode change notification); the phone keeps the pad. Unplugging puts the view back and the mode the game had before
@interface DmTVVC : UIViewController
@end
@implementation DmTVVC
- (void)viewDidLoad { [super viewDidLoad]; self.view.backgroundColor = UIColor.blackColor; }
- (void)viewDidLayoutSubviews { [super viewDidLayoutSubviews]; for (UIView *v in self.view.subviews) v.frame = fit_16_9(self.view.bounds); }
@end
static UIWindow *g_tv; static int g_tv_mode_was = -1;
static BOOL tv_active(void) { return g_tv && g_root.game.superview==g_tv.rootViewController.view; }
static void tv_attach(void) {
    DmGameView *v = g_root.game; if (!g_tv || !v || v.superview == g_tv.rootViewController.view) return;
    [g_tv.rootViewController.view addSubview:v]; [g_tv.rootViewController.view setNeedsLayout];
    acnh_wide_set_aspect(16.f/9.f);acnh_wide_set_present_full(0);
    [g_root.view setNeedsLayout];
    // at most 1080p: the game renders 1080p docked, and a 4K drawable only adds an upscale and a 4K stream (an AirPlay TV showed black, the GPU went from 15 to 60 ms a frame).
    // Taken from the 16:9 picture, not the screen, so a display of another shape still gets the full 1920 pixels across the picture
    CGRect f = fit_16_9(g_tv.bounds);
    v.layer.contentsScale = MIN(v.layer.contentsScale, 1920.0 / MAX(f.size.width, 1)); [v setNeedsLayout];
    if (g_tv_mode_was < 0) g_tv_mode_was = dm_opmode();
    dm_set_opmode(1);
    LOG("front end: TV connected (%.0fx%.0f points), the picture moves there at %.0fx%.0f, console mode", g_tv.bounds.size.width, g_tv.bounds.size.height, f.size.width * v.layer.contentsScale, f.size.height * v.layer.contentsScale);
}
static void tv_detach(void) {
    DmGameView *v = g_root.game; g_tv = nil;
    if (v && v.superview != g_root.gameContainer) { [g_root.gameContainer addSubview:v]; [g_root.view setNeedsLayout]; }
    if (g_tv_mode_was >= 0) { dm_set_opmode(g_tv_mode_was); g_tv_mode_was = -1; }
    LOG("front end: TV disconnected, the picture is back on the phone");
}
@interface DmTVSceneDelegate : UIResponder <UIWindowSceneDelegate>
@property (strong, nonatomic) UIWindow *window;
@end
@implementation DmTVSceneDelegate
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)o {
    self.window = [[UIWindow alloc] initWithWindowScene:(UIWindowScene *)scene];
    self.window.rootViewController = [DmTVVC new];
    self.window.hidden = NO;   // without a window iOS goes on mirroring the phone; not key, so the phone's window keeps the keyboard
    g_tv = self.window; tv_attach();
}
- (void)sceneDidDisconnect:(UIScene *)s { if (g_tv == self.window) tv_detach(); self.window = nil; }
@end

// Thermal state and screen brightness for the power and present rows every 300 frames (hle_nvn.c; its weak defaults return -1 on the Mac). thermalState: 0 nominal, 1 fair, 2 serious, 3 critical.
// Brightness is read on the main thread at start and on each change; every change of the thermal state and of Low Power Mode (which lowers the CPU clock) is logged with the time since launch
static _Atomic double g_brightness = -1;
int dm_thermal_now(void) { return (int)NSProcessInfo.processInfo.thermalState; }
int dm_low_power_now(void) { return NSProcessInfo.processInfo.lowPowerModeEnabled; }   // the frame step does not probe in it (hle_nvn.c fstep_cpu_slowed)
double dm_brightness_now(void) { return g_brightness; }
static void front_ios_power_watch(void) {
    g_brightness = UIScreen.mainScreen.brightness;
    [NSNotificationCenter.defaultCenter addObserverForName:UIScreenBrightnessDidChangeNotification object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *n) { (void)n; g_brightness = UIScreen.mainScreen.brightness; }];
    [NSNotificationCenter.defaultCenter addObserverForName:NSProcessInfoThermalStateDidChangeNotification object:nil queue:nil usingBlock:^(NSNotification *n) {
        (void)n; LOG("front end: thermal state → %d (%.0f s after launch)", dm_thermal_now(), (now_ns() - g_t0_ns) / 1e9); }];
    [NSNotificationCenter.defaultCenter addObserverForName:NSProcessInfoPowerStateDidChangeNotification object:nil queue:nil usingBlock:^(NSNotification *n) {
        (void)n; LOG("front end: Low Power Mode → %s (%.0f s after launch)", dm_low_power_now() ? "on" : "off", (now_ns() - g_t0_ns) / 1e9); }];
    LOG("front end: thermal state %d, brightness %.2f, Low Power Mode %s", dm_thermal_now(), g_brightness, dm_low_power_now() ? "on" : "off");
}

// Frame-rate counter: the frames put on screen in the last second (nvn_mtl.m counts them for NVN and for the Vulkan layer alike; frames the display link merged away are not counted).
// On when the IPA was built with it (the NSP script's SHOW_FPS / --show-fps → DMShowFPS in Info.plist); DM_SHOW_FPS=0 / 1 in Documents/env.txt overrides that without a rebuild. NULL when off
uint64_t mtl_present_count(void);
static UILabel *front_ios_fps_label(void) {
    const char *e = getenv("DM_SHOW_FPS");
    if (!(e ? atoi(e) != 0 : [[NSBundle.mainBundle objectForInfoDictionaryKey:@"DMShowFPS"] boolValue])) return nil;
    UILabel *l = [UILabel new];
    l.font = [UIFont monospacedDigitSystemFontOfSize:13 weight:UIFontWeightSemibold]; l.textColor = UIColor.whiteColor; l.textAlignment = NSTextAlignmentCenter;
    l.backgroundColor = [UIColor colorWithWhite:0 alpha:0.45]; l.layer.cornerRadius = 6; l.layer.masksToBounds = YES; l.userInteractionEnabled = NO;
    l.text = @"-- fps";
    __block uint64_t n0 = mtl_present_count(); __block double t0 = CACurrentMediaTime();
    [NSTimer scheduledTimerWithTimeInterval:1.0 repeats:YES block:^(NSTimer *t) {
        (void)t; uint64_t n = mtl_present_count(); double now = CACurrentMediaTime();
        if (now - t0 < 1.5) l.text = [NSString stringWithFormat:@"%.0f fps", (n - n0) / (now - t0)];   // a longer span is the time in the background: no reading
        n0 = n; t0 = now;
    }];
    LOG("front end: frame-rate counter on");
    return l;
}

// Progress screen of the prepare step (front.h front_progress): a dark view over the game's view and the pad, the title, a bar and a count, centered; UIKit views, so it draws while the game's thread waits
static UIView *g_prog; static UILabel *g_prog_count; static UIProgressView *g_prog_bar; static UILabel *g_prog_title;
void front_progress(const char *title, long done, long total) {
    if (!g_layer) return;
    NSString *tt = title ? @(title) : nil;
    NSString *language=NSLocale.preferredLanguages.firstObject.lowercaseString ?: @"en";
    if([language hasPrefix:@"zh"]){
        BOOL traditional=[language containsString:@"hant"]||[language hasSuffix:@"-tw"]||[language hasSuffix:@"-hk"];
        if([tt isEqualToString:@"Preparing shaders"])tt=traditional?@"正在準備著色器":@"正在准备着色器";
        if([tt isEqualToString:@"Preparing pipelines"])tt=traditional?@"正在準備管線":@"正在准备管线";
    }else if([language hasPrefix:@"ja"]){
        if([tt isEqualToString:@"Preparing shaders"])tt=@"シェーダーを準備中";
        if([tt isEqualToString:@"Preparing pipelines"])tt=@"パイプラインを準備中";
    }
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!tt) { [g_prog removeFromSuperview]; g_prog = nil; return; }
        if (!g_prog) {
            UIView *root = g_root.view; if (!root) return;
            g_prog = [[UIView alloc] initWithFrame:root.bounds]; g_prog.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
            g_prog.backgroundColor = [UIColor colorWithWhite:0.06 alpha:0.94];
            UIView *box = [[UIView alloc] initWithFrame:CGRectMake(floor((root.bounds.size.width - 320) / 2), floor((root.bounds.size.height - 96) / 2), 320, 96)];
            box.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleBottomMargin;
            g_prog_title = [[UILabel alloc] initWithFrame:CGRectMake(0, 4, 320, 30)]; g_prog_title.textAlignment = NSTextAlignmentCenter; g_prog_title.textColor = UIColor.whiteColor;
            g_prog_title.font = [UIFont systemFontOfSize:20 weight:UIFontWeightSemibold];
            g_prog_bar = [[UIProgressView alloc] initWithProgressViewStyle:UIProgressViewStyleDefault]; g_prog_bar.frame = CGRectMake(20, 48, 280, 4);
            g_prog_count = [[UILabel alloc] initWithFrame:CGRectMake(0, 64, 320, 22)]; g_prog_count.textAlignment = NSTextAlignmentCenter;
            g_prog_count.textColor = [UIColor colorWithWhite:0.72 alpha:1]; g_prog_count.font = [UIFont monospacedDigitSystemFontOfSize:13 weight:UIFontWeightRegular];
            [box addSubview:g_prog_title]; [box addSubview:g_prog_bar]; [box addSubview:g_prog_count]; [g_prog addSubview:box];
            [root addSubview:g_prog];
        }
        g_prog_title.text = tt; g_prog_bar.progress = total > 0 ? (float)done / (float)total : 0;
        g_prog_count.text = [NSString stringWithFormat:@"%@ / %@", [NSNumberFormatter localizedStringFromNumber:@(done) numberStyle:NSNumberFormatterDecimalStyle],
                             [NSNumberFormatter localizedStringFromNumber:@(total) numberStyle:NSNumberFormatterDecimalStyle]];
    });
}
void *front_metal_layer(void) { return (__bridge void *)g_layer; }
void front_run(void (*guest)(void)) { guest(); }   // Already on main.m's big-stack thread

// Software keyboard: show a UIAlertController on the main thread, the calling thread waits for the result
int front_text_input(const char *title, const char *sub, const char *guide, const char *initial, int max_chars, char *out, int out_cap) {
    (void)max_chars;
    if (!g_layer) return -1;
    __block int r = 0; __block NSString *text = nil;
    dispatch_semaphore_t done = dispatch_semaphore_create(0);
    NSString *t = title && *title ? @(title) : @"", *m = [NSString stringWithFormat:@"%s%s%s", sub ? sub : "", sub && *sub && guide && *guide ? "\n" : "", guide ? guide : ""];
    NSString *init = initial ? @(initial) : @"", *ph = guide ? @(guide) : @"";
    dispatch_async(dispatch_get_main_queue(), ^{
        UIAlertController *ac = [UIAlertController alertControllerWithTitle:t message:m preferredStyle:UIAlertControllerStyleAlert];
        [ac addTextFieldWithConfigurationHandler:^(UITextField *f) { f.text = init; f.placeholder = ph; }];
        [ac addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction *x) { (void)x; r = 1; text = ac.textFields.firstObject.text; dispatch_semaphore_signal(done); }]];
        [ac addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:^(UIAlertAction *x) { (void)x; dispatch_semaphore_signal(done); }]];
        UIViewController *vc = nil;
        for (UIScene *s in UIApplication.sharedApplication.connectedScenes) if ([s isKindOfClass:UIWindowScene.class]) for (UIWindow *w in ((UIWindowScene *)s).windows) if (w.isKeyWindow) vc = w.rootViewController;
        if (!vc) { dispatch_semaphore_signal(done); return; }
        [vc presentViewController:ac animated:YES completion:nil];
    });
    // The guest clock keeps running meanwhile, as on the Switch (hle_swkbd.c): a freeze here as well stopped Octopath Traveler 0's frame loop for as long as the box was up and made its sound stutter
    // (4,977 ms and 2,577 ms frames, 5,181 wakeups a second). The inline keyboard freezes around its own call (hle_swkbd.c)
    dispatch_semaphore_wait(done, DISPATCH_TIME_FOREVER);
    if (!r) return 0;
    snprintf(out, out_cap, "%s", text.UTF8String ? text.UTF8String : "");
    return 1;
}

// Save write failed (most likely the phone's storage is full): show a notice on the main thread without blocking the game. hle_fs.c calls this when a save transaction is refused or a mount has to fall back to read-only;
// the iOS front end used not to implement it, so there was only a log line and the player never learned that progress was not being saved
void front_save_failed(int err) {
    LOG("front end: save data write failed errno=%d, telling the player", err);
    dispatch_async(dispatch_get_main_queue(), ^{
        UIViewController *vc = nil;
        for (UIScene *s in UIApplication.sharedApplication.connectedScenes) if ([s isKindOfClass:UIWindowScene.class]) for (UIWindow *w in ((UIWindowScene *)s).windows) if (w.isKeyWindow) vc = w.rootViewController;
        while (vc.presentedViewController) vc = vc.presentedViewController;
        if (!vc) { LOG("[RED] front end: no window for the save-failure alert errno=%d", err); return; }
        NSString *m = err == ENOSPC ? @"Storage is full, so progress was not saved. Free up space and save again."
                                    : [NSString stringWithFormat:@"Saving failed (errno %d); progress was not saved.", err];
        UIAlertController *a = [UIAlertController alertControllerWithTitle:@"Save failed" message:m preferredStyle:UIAlertControllerStyleAlert];
        [a addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];
        [vc presentViewController:a animated:YES completion:nil];
    });
}

// Audio session: the frames RemoteIO asks for in each callback follow the I/O buffer duration, about 21 ms by default on iOS (1024 frames at 48 kHz). The game keeps only 4 blocks × 5 ms = 20 ms in flight,
// so every callback came up 1.3 ms short and was zero-filled, about 47 gaps per second (crackling; reproducible on the Mac by forcing 1024-frame callbacks: 1999 underruns in 2000 callbacks). Ask for 5 ms and the system picks the nearest it supports;
// SoloAmbient follows the silent switch by default; the launcher can select Playback to hear game audio in silent mode. Set once at boot and again after foreground/interruption recovery.
static int g_session_off;   // the last activation failed (another app or a call still held the session): tried again when the app becomes active
void front_ios_audio_session(void) {
    AVAudioSession *s = AVAudioSession.sharedInstance; NSError *e = nil;
    const char *setting = getenv("ACNH_AUDIO_IGNORE_SILENT");
    BOOL ignoreSilent = setting && !strcmp(setting, "1");
    NSString *category = ignoreSilent ? AVAudioSessionCategoryPlayback : AVAudioSessionCategorySoloAmbient;
    if (![s setCategory:category error:&e]) LOG("[RED] audio session category could not be set: %s", e.localizedDescription.UTF8String);
    [s setPreferredSampleRate:48000 error:nil];
    if (![s setPreferredIOBufferDuration:0.005 error:&e]) LOG("[RED] audio session I/O buffer duration could not be set: %s", e.localizedDescription.UTF8String);
    g_session_off = ![s setActive:YES error:&e];
    if (g_session_off) LOG("[RED] audio session activation failed: %s", e.localizedDescription.UTF8String);
    LOG("audio session: silent-mode playback %s", ignoreSilent ? "on" : "off");
    LOG("audio session: sample rate %.0f Hz, I/O buffer %.2f ms (asked for 5 ms)", s.sampleRate, s.IOBufferDuration * 1000);
}

// Real audio output is suspended (the silent sink paces the game) while the app is in the background or another audio session has interrupted ours (Siri, an alarm, a call).
// Without the interruption observer the AudioUnits stayed "started" while iOS no longer called them, and a game waiting for its buffers back lost its sound until restart.
// All of this runs on the main queue, like the scene callbacks
static int g_audio_bg, g_audio_intr;
static void front_ios_audio_apply(void) { dm_audio_suspend(g_audio_bg || g_audio_intr); }
static void front_ios_audio_watch(void) {
    NSNotificationCenter *nc = NSNotificationCenter.defaultCenter;
    [nc addObserverForName:AVAudioSessionInterruptionNotification object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *n) {
        NSUInteger type = [n.userInfo[AVAudioSessionInterruptionTypeKey] unsignedIntegerValue], reason = [n.userInfo[AVAudioSessionInterruptionReasonKey] unsignedIntegerValue];
        if (type == AVAudioSessionInterruptionTypeBegan) { LOG("audio session: interrupted (reason %lu)", (unsigned long)reason); g_audio_intr = 1; }
        else {
            LOG("audio session: interruption ended"); g_audio_intr = 0;
            if (!g_audio_bg) front_ios_audio_session();
        }
        front_ios_audio_apply();
    }];
    [nc addObserverForName:AVAudioSessionMediaServicesWereResetNotification object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *n) {
        (void)n; LOG("audio session: the media services were reset; setting the session and the outputs up again");
        front_ios_audio_session(); dm_audio_reset();
    }];
}

// Scene lifecycle (called by SceneDelegate in main.m): 0 resign active, 1 enter background, 2 return to foreground, 3 become active. On entering background, freeze the guest clock and suspend after in-flight GPU work completes (submitting GPU work in background gets the process killed).
// The screen stays on while the game is active: only touches reset the idle timer, so with a controller (or during a cutscene) the screen dimmed and locked mid-game
// Cloud saves (save_sync.h): what the game committed goes out before iOS suspends the app, without the sender's quiet wait. iOS gives a background task about half a minute
static void save_sync_background(void) {
    UIApplication *app = UIApplication.sharedApplication;
    __block UIBackgroundTaskIdentifier task = UIBackgroundTaskInvalid;
    void (^end)(void) = ^{ if (task != UIBackgroundTaskInvalid) { [app endBackgroundTask:task]; task = UIBackgroundTaskInvalid; } };   // on the main queue, as the expiry handler runs
    task = [app beginBackgroundTaskWithName:@"cloud save" expirationHandler:end];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{
        if (!save_sync_flush(20000)) LOG("front end: the save did not reach the cloud before the app was suspended; it goes after the next save or at the next start");
        dispatch_async(dispatch_get_main_queue(), end);
    });
}

void front_ios_focus_event(int ev) {
    if (!g_layer) return;
    switch (ev) {
        case 0: dm_gamepad_reset(); dm_focus_set(2); break;
        case 1: UIApplication.sharedApplication.idleTimerDisabled = NO; dm_focus_set(3); g_audio_bg = 1; front_ios_audio_apply(); guest_clock_freeze_for(1, 1); dm_gfx_suspend(1); save_sync_background(); break;
        case 2: dm_gfx_suspend(0); guest_clock_freeze_for(1, 0); front_ios_audio_session(); g_audio_bg = 0; front_ios_audio_apply(); dm_focus_set(2); break;
        case 3:   // Becoming active again also ends an interruption whose "ended" never came (not every one sends it), and retries a session that would not activate on the way back
            UIApplication.sharedApplication.idleTimerDisabled = YES;
            if (g_audio_intr) { LOG("audio session: active again with no interruption-ended notice; resuming"); g_audio_intr = 0; g_session_off = 1; }
            if (g_session_off) front_ios_audio_session();
            front_ios_audio_apply(); dm_gamepad_active(); dm_focus_set(1); break;
    }
}
