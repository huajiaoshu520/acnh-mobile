// iOS App entry: set the environment (game files, saves, shader libraries), write the host log to Documents/logs/log.txt, create the window (front_ios.m), then run the host's dm_main on a big-stack thread.
// The game files (RomFS) ship inside the App at <App>/romfs; Documents/romfs placed through the Files app is also accepted (lets you swap them without reinstalling).
#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import "DeviceSupport.h"
#import "ACNHLogFiles.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include "../../host/acnh_data.h"
#include "../../host/directory_visibility.h"
#include "../../host/front.h"
#include "../../host/romfs_image.h"
#include "../../host/romfs_identity.h"
#include "game_profile.h"   // game profile constants (the RomFS marker path)

int dm_main(int argc, char **argv);   // dm/host/main.c compiled in with -Dmain=dm_main
void dm_host_preload_start(int argc, char **argv);
UIViewController *front_ios_root(void);
void front_ios_focus_event(int ev);

static char *g_argv[4]; static int g_argc;

const char *front_ios_device_message(void) {
    static NSString *message; static dispatch_once_t once;
    dispatch_once(&once, ^{
        id<MTLDevice> gpu=MTLCreateSystemDefaultDevice();
        message=acnh_device_message(gpu!=nil,gpu.supportsBCTextureCompression,gpu.name,NSLocale.preferredLanguages.firstObject);
        if(message)fprintf(stderr,"[RED] boot: %s\n",message.UTF8String);
    });
    return message.UTF8String;
}


static int romfs_image_complete(NSString *path) {
    romfs_img *m = romfs_image_open(path.fileSystemRepresentation);
    if (!m) {
        if (errno != ENOENT) fprintf(stderr, "[RED] boot: invalid RomFS image %s (errno=%d); trying the folders\n", path.fileSystemRepresentation, errno);
        return 0;
    }
    romfs_image_close(m); return 1;
}
// A build made with DM_COPY_ROMFS=NO carries <App>/romfs.id (dm/host/romfs_identity.h): only a Documents/romfs.img with that identity is used. No romfs.id (the default build): any complete image
static int romfs_identity_accepts(NSString *img) {
    romfs_id want, got;
    int rc = romfs_id_check([NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"romfs.id"].fileSystemRepresentation, img.fileSystemRepresentation, &want, &got);
    if (rc != ROMFS_ID_OK && rc != ROMFS_ID_NONE) fprintf(stderr, "[RED] boot: Documents/%s is not the image this build needs (verdict %d); not used\n", img.lastPathComponent.UTF8String, rc);
    return rc == ROMFS_ID_OK || rc == ROMFS_ID_NONE;
}
// The images in Documents: romfs.img in any letter case, romfs.img itself first. The NSP script writes the image as RomFS.img (extract/<version>/RomFS.img) and the Mac's
// file system ignores letter case, but iOS's does not: a copy made through Finder or the Files app keeps the source's name, which an exact lookup of romfs.img never finds
static NSArray<NSString *> *romfs_document_images(NSString *doc) {
    NSMutableArray<NSString *> *imgs = [NSMutableArray array];
    for (NSString *n in [NSFileManager.defaultManager contentsOfDirectoryAtPath:doc error:nil])
        if ([n caseInsensitiveCompare:@"romfs.img"] == NSOrderedSame) [imgs addObject:[doc stringByAppendingPathComponent:n]];
    [imgs sortUsingComparator:^NSComparisonResult(NSString *a, NSString *b) {
        BOOL ea = [a.lastPathComponent isEqualToString:@"romfs.img"], eb = [b.lastPathComponent isEqualToString:@"romfs.img"];
        return ea != eb ? (ea ? NSOrderedAscending : NSOrderedDescending) : [a compare:b];
    }];
    return imgs;
}
static NSString *romfs_locate(void) {
    if(getenv("DM_ROMFS") && dm_acnh_data_root())return @(getenv("DM_ROMFS"));
    NSFileManager *fm = NSFileManager.defaultManager;
    NSString *doc = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
    for (NSString *img in romfs_document_images(doc)) {
        if (!romfs_image_complete(img) || !romfs_identity_accepts(img)) continue;
        // A folder copy from before images is now dead weight of up to many GB (often cut off partway). Deleting its 300,000 files here took longer than iOS allows an app to start
        // (Tears of the Kingdom: killed on a black screen), so it is moved aside at once and deleted in the background, leftovers of an interrupted delete included
        NSString *old = [doc stringByAppendingPathComponent:@"romfs"], *tmp = NSTemporaryDirectory();
        if ([fm fileExistsAtPath:old] && [fm moveItemAtPath:old toPath:[tmp stringByAppendingPathComponent:[@"romfs-old-" stringByAppendingString:NSUUID.UUID.UUIDString]] error:nil])
            fprintf(stderr, "boot: Documents/romfs moved aside for deletion (Documents/%s replaces it)\n", img.lastPathComponent.UTF8String);
        dispatch_async(dispatch_get_global_queue(QOS_CLASS_BACKGROUND, 0), ^{
            NSFileManager *bfm = [NSFileManager new];
            for (NSString *n in [bfm contentsOfDirectoryAtPath:tmp error:nil]) if ([n hasPrefix:@"romfs-old-"]) [bfm removeItemAtPath:[tmp stringByAppendingPathComponent:n] error:nil];
        });
        return img;
    }
    NSString *bundledImage = [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"romfs.img"];
    NSString *documentFolder = [doc stringByAppendingPathComponent:@"romfs"];
    if ([fm fileExistsAtPath:[documentFolder stringByAppendingPathComponent:@DM_GAME_ROMFS_MARKER]]) return documentFolder;
    if (romfs_image_complete(bundledImage)) return bundledImage;
    for (NSString *d in @[[doc stringByAppendingPathComponent:@"romfs"], [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"romfs"]])
        if ([fm fileExistsAtPath:[d stringByAppendingPathComponent:@DM_GAME_ROMFS_MARKER]]) return d;   // only a directory containing the game profile's romfs_marker path counts (Mario: System/Resource, the resource size table)
    return nil;
}

// Boot prerequisites (done before UIApplicationMain): directories / environment variables / log -- the host's early load reads these
extern char **environ;
static int boot_env(void) {
    NSMutableArray<NSString *> *from_launch=[NSMutableArray array];
    for(char **e=environ;*e;e++){
        const char *eq=strchr(*e,'=');
        if(eq&&!strncmp(*e,"DM_",3)){
            NSString *key=[[NSString alloc] initWithBytes:*e length:(NSUInteger)(eq-*e) encoding:NSUTF8StringEncoding];
            if(key)[from_launch addObject:key];
        }
    }
    setenv("ACNH_HIDE_RUNTIME_DIRS","1",0);
    NSFileManager *fm = NSFileManager.defaultManager;
    NSString *doc = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
    NSString *res = NSBundle.mainBundle.resourcePath;
    for (NSString *d in @[@"save", @"temp"]) [fm createDirectoryAtPath:[doc stringByAppendingPathComponent:d] withIntermediateDirectories:YES attributes:nil error:nil];
    // Keep game files and scratch data out of device backups (Documents is backed up by default; a RomFS placed there is many GB). Saves, logs and the pipeline list stay in the backup
    for (NSString *d in @[@"romfs", @"temp"]) {
        NSURL *u = [NSURL fileURLWithPath:[doc stringByAppendingPathComponent:d] isDirectory:YES]; NSError *e = nil;
        if ([fm fileExistsAtPath:u.path] && ![u setResourceValue:@YES forKey:NSURLIsExcludedFromBackupKey error:&e]) fprintf(stderr, "boot: %s could not be excluded from backup: %s\n", d.UTF8String, e.localizedDescription.UTF8String);
    }
    // Keep diagnostics together, with safe migration from older app versions.
    NSError *logError=nil;
    NSString *logs=acnh_prepare_logs(doc,&logError) ?: doc;
    NSString *log = [logs stringByAppendingPathComponent:@"log.txt"], *prev = [logs stringByAppendingPathComponent:@"log.prev.txt"];
    [fm removeItemAtPath:prev error:nil]; [fm moveItemAtPath:log toPath:prev error:nil];
    if (freopen(log.fileSystemRepresentation, "w", stderr)) setvbuf(stderr, NULL, _IOLBF, 0);
    if(logError)fprintf(stderr,"boot: log migration failed: %s; using %s\n",logError.localizedDescription.UTF8String,logs.fileSystemRepresentation);
    if(dm_directory_hidden([doc stringByAppendingPathComponent:@"temp"].fileSystemRepresentation,1))
        fprintf(stderr,"boot: temp hidden attribute could not be set: %s\n",strerror(errno));
    // Environment switch files (for on-device diagnostics: a sideloaded app cannot be given environment variables from outside): one KEY=VALUE per line, # starts a comment; Documents/env.txt takes precedence over dm/env.txt in the bundle (put there by build_ipa.sh's DM_ENV_FILE)
    NSMutableArray<NSString *> *from_file=[NSMutableArray array];
    for (NSString *ef in @[[doc stringByAppendingPathComponent:@"env.txt"], [[res stringByAppendingPathComponent:@"dm"] stringByAppendingPathComponent:@"env.txt"]]) {
        NSString *txt = [NSString stringWithContentsOfFile:ef encoding:NSUTF8StringEncoding error:nil];
        for (NSString *raw in [txt componentsSeparatedByCharactersInSet:NSCharacterSet.newlineCharacterSet]) {
            NSString *ln = [raw stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet]; NSRange eq = [ln rangeOfString:@"="];
            if (!ln.length || [ln hasPrefix:@"#"] || eq.location == NSNotFound || !eq.location) continue;
            NSString *k = [ln substringToIndex:eq.location], *v = [ln substringFromIndex:eq.location + 1];
            if (!getenv(k.UTF8String)) { setenv(k.UTF8String, v.UTF8String, 0); [from_file addObject:k]; fprintf(stderr, "boot: env %s=%s (%s)\n", k.UTF8String, v.UTF8String, ef.lastPathComponent.UTF8String); }
        }
    }
    setenv("DM_PSO_USER",[logs stringByAppendingPathComponent:@"pso_user.tsv"].fileSystemRepresentation,0);
    setenv("DM_ENV_FILE_KEYS",[from_file componentsJoinedByString:@" "].UTF8String,1);
    setenv("DM_ENV_LAUNCH_KEYS",[from_launch componentsJoinedByString:@" "].UTF8String,1);
    front_hud_env();
    // Game language (nn::oe::GetDesiredLanguage, hle_oe.c): the one the IPA was packed with (the NSP script's LANGUAGE / --language → DMLanguage in Info.plist), unless an env file sets DM_LANG
    NSString *lang = [NSBundle.mainBundle objectForInfoDictionaryKey:@"DMLanguage"];
    if (!getenv("DM_LANG") && [lang isKindOfClass:NSString.class] && lang.length) setenv("DM_LANG", lang.UTF8String, 0);
    fprintf(stderr, "boot: game language %s\n", getenv("DM_LANG") ?: "en-US");
    if(front_ios_device_message())return 0; // refuse before early host loading
    NSString *romfs = romfs_locate();
    if (!romfs) { fprintf(stderr, "boot: game files not found (<App>/romfs or Documents/romfs)\n"); return 0; }
    fprintf(stderr, "boot: game files %s\n", romfs.fileSystemRepresentation);
    setenv("DM_ROMFS", romfs.fileSystemRepresentation, 0);
    setenv("DM_SAVE", [doc stringByAppendingPathComponent:@"save"].fileSystemRepresentation, 0);
    setenv("DM_TEMP", [doc stringByAppendingPathComponent:@"temp"].fileSystemRepresentation, 0);
    NSString *dm = [res stringByAppendingPathComponent:@"dm"];
    setenv("DM_NVN_SHMAP", [dm stringByAppendingPathComponent:@"shader_map.tsv"].fileSystemRepresentation, 0);
    setenv("DM_MSL_DIR", [res stringByAppendingPathComponent:@"msl"].fileSystemRepresentation, 0);
    NSString *pso=[res stringByAppendingPathComponent:@"pso"];if([fm fileExistsAtPath:pso])setenv("DM_PSO_BUNDLE",pso.fileSystemRepresentation,0);
    setenv("DM_FONT_DIR", [dm stringByAppendingPathComponent:@"font"].fileSystemRepresentation, 0);   // Shared-font stand-ins (standard.ttf / nintendo_ext.ttf): iOS lacks the macOS font path
    NSString *aoc = [res stringByAppendingPathComponent:@"aoc"];   // add-on content: build_ipa.sh puts the RomFS extracted from the player's DLC NSPs at <App>/aoc/<index>/
    if ([fm fileExistsAtPath:aoc]) setenv("DM_AOC_DIR", aoc.fileSystemRepresentation, 0);
    chdir(doc.fileSystemRepresentation);
    g_argv[0] = strdup("dmhost"); g_argv[1] = strdup(dm.fileSystemRepresentation); g_argc = 2;
    dm_host_preload_start(g_argc, g_argv);   // Module tables / relocation / takeover run early in the background, in parallel with UIKit startup
    return 1;
}
static int g_boot_ok;
int front_ios_load_resources(const char *root) {
    char why[256];if(!dm_acnh_data_mount(root,why,sizeof why))return 0;
    setenv("DM_ROMFS",root,1);g_boot_ok=boot_env();return g_boot_ok;
}

static void *boot_thread(void *p) {
    (void)p;
    static char alt[65536]; stack_t ss = {.ss_sp = alt, .ss_size = sizeof alt}; sigaltstack(&ss, 0);   // crash printing must work on the host thread too
    int rc = dm_main(g_argc, g_argv);
    fprintf(stderr, "boot: dm_main returned %d\n", rc);
    return NULL;
}
void front_ios_boot(void) {   // Called by front_ios.m once the window and Metal view exist
    static int once; if (once++ || !g_boot_ok) return;
    void front_ios_audio_session(void); front_ios_audio_session();   // set a short I/O buffer before the game opens audio output (front_ios.m)
    pthread_attr_t a; pthread_attr_init(&a); pthread_attr_setstacksize(&a, 64u << 20);   // the Switch main thread stack is set by npdm; the host gives plenty
    pthread_t t; pthread_create(&t, &a, boot_thread, NULL); pthread_detach(t);
}
int front_ios_boot_ok(void) { return g_boot_ok; }

@interface App : UIResponder <UIApplicationDelegate>
@end
@implementation App
- (UISceneConfiguration *)application:(UIApplication *)a configurationForConnectingSceneSession:(UISceneSession *)s options:(UISceneConnectionOptions *)o {
    return [[UISceneConfiguration alloc] initWithName:[s.role isEqualToString:UIWindowSceneSessionRoleExternalDisplayNonInteractive] ? @"TV" : @"Default" sessionRole:s.role];   // a TV (front_ios.m DmTVSceneDelegate)
}
@end

// Scene lifecycle: create the window + hand focus events to front_ios.m
@interface SceneDelegate : UIResponder <UIWindowSceneDelegate>
@property (strong, nonatomic) UIWindow *window;
@end
@implementation SceneDelegate
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)o {
    self.window = [[UIWindow alloc] initWithWindowScene:(UIWindowScene *)scene];
    self.window.rootViewController = front_ios_root();
    [self.window makeKeyAndVisible];
}
- (void)sceneWillResignActive:(UIScene *)s { front_ios_focus_event(0); }
- (void)sceneDidEnterBackground:(UIScene *)s { front_ios_focus_event(1); }
- (void)sceneWillEnterForeground:(UIScene *)s { front_ios_focus_event(2); }
- (void)sceneDidBecomeActive:(UIScene *)s { front_ios_focus_event(3); }
@end

int main(int argc, char **argv) {
    // Resource selection runs after UIKit has a visible window.
    @autoreleasepool { return UIApplicationMain(argc, argv, nil, NSStringFromClass(App.class)); }
}
