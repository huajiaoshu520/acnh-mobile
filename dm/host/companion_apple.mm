// Apple host for the portable Eden Duo module ABI and guarded inventory transactions.
#import "companion_apple.h"
#include "companion.h"
extern "C" {
#include "acnh_data.h"
}
#include "../third_party/eden_duo/core/mods/dsmod_module_abi.h"
#include "../third_party/eden_duo/core/mods/dsmod_module_extensions.h"
#import <Metal/Metal.h>
#import <ImageIO/ImageIO.h>
#import <CommonCrypto/CommonDigest.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <array>
#include <mutex>
#include <cstring>
#include <cmath>

#ifndef DM_HAS_COMPANION
#define DM_HAS_COMPANION 0
#endif
static const EdenDsmodModuleApi *provider() {
#if DM_HAS_COMPANION
    return eden_dsmod_get_module(EDEN_DSMOD_MODULE_ABI_VERSION,EDEN_DSMOD_MODULE_ABI_HASH);
#else
    return nullptr;
#endif
}
@interface DMCompanion () {
    EdenDsmodHostApi _host;
    const EdenDsmodModuleApi *_api;
    const EdenDsmodModuleExtensions *_extensions;
    const EdenDsmodModuleWriteExtensions *_writeExtensions;
    void *_instance;
    dispatch_queue_t _worker, _assets;
    dispatch_source_t _timer;
    NSMutableDictionary *_output;
    NSDictionary *_latest;
    NSCache *_images;
    NSMutableDictionary *_failedImages;
    NSMutableOrderedSet *_failedImageKeys;
    NSDictionary *_inputGates;
    NSArray *_writeBuilds;
    NSString *_romfs, *_trace;
    uint64_t _sample, _command, _pressMask, _pressUntil, _pressGeneration, _moduleMask, _visibilityGeneration;
    NSUInteger _traceTick;
    BOOL _visible, _moduleReady;
}
@property(atomic) BOOL visible;
@property(atomic,copy) NSString *page;
- (void)publish:(NSString *)key value:(id)value;
- (int64_t)query:(NSString *)key fallback:(int64_t)value;
- (BOOL)decodeASTC:(uint32_t)w height:(uint32_t)h blockWidth:(uint32_t)bw blockHeight:(uint32_t)bh input:(const uint8_t *)input length:(size_t)length output:(uint8_t *)output capacity:(size_t)capacity;
- (void)sample;
- (void)trace:(NSString *)directory;
@end
static DMCompanion *object(void *p) { return (__bridge DMCompanion *)p; }
static uint64_t tick(void *) { return (uint64_t)(NSProcessInfo.processInfo.systemUptime*1000.0); }
static uint64_t heapBegin(void *) { dm_companion_guest g;dm_companion_guest_info(&g);return g.heap_begin; }
static uint64_t heapEnd(void *) { dm_companion_guest g;dm_companion_guest_info(&g);return g.heap_end; }
static EdenDsmodBool mapped(void *,uint64_t at,uint64_t n) { return n<=SIZE_MAX && dm_companion_guest_mapped(at,(size_t)n); }
static EdenDsmodBool readMemory(void *,uint64_t at,void *out,size_t n) { return dm_companion_guest_read(at,out,n); }
static EdenDsmodBool writeBatch(void *p,const EdenDsmodWriteOp *ops,uint32_t count) {
    if(!object(p).visible || !ops || !count || count>EDEN_DSMOD_WRITE_BATCH_MAX_OPS)return 0;
    dm_companion_write_op copy[EDEN_DSMOD_WRITE_BATCH_MAX_OPS];
    for(uint32_t i=0;i<count;i++){
        if(ops[i].reserved)return 0;
        copy[i]={ops[i].address,ops[i].size,ops[i].expect,ops[i].value};
    }
    return dm_companion_guest_write_batch(copy,count);
}
static void moduleLog(void *,uint32_t level,const char *message) { fprintf(stderr,"companion [%u]: %s\n",level,message ?: ""); }
static void beginOutput(void *) {}
static void publishInt(void *p,const char *k,int64_t v) { if(k)[object(p) publish:@(k) value:@(v)]; }
static void publishFloat(void *p,const char *k,double v) { if(k && std::isfinite(v))[object(p) publish:@(k) value:@(v)]; }
static void publishText(void *p,const char *k,const char *v) { if(k && v){NSString *text=[[NSString alloc]initWithUTF8String:v];if(text)[object(p) publish:@(k) value:text];} }
static void publishAddress(void *p,const char *k,uint64_t v) { if(k)[object(p) publish:@(k) value:@(v)]; }
static int64_t getInt(void *p,const char *k,int64_t fallback) { return k ? [object(p) query:@(k) fallback:fallback] : fallback; }
static int64_t assetInt(void *,const char *k,int64_t fallback) { return k && !strcmp(k,"__relocation_delta") ? 0 : fallback; }
static double getFloat(void *p,const char *k,double fallback) { NSNumber *v=object(p).snapshot[k ? @(k) : @""];return [v isKindOfClass:NSNumber.class] ? v.doubleValue : fallback; }
static const char *getText(void *p,const char *k) {
    if(k && !strcmp(k,"__page"))return object(p).page.UTF8String;
    return nullptr;
}
static BOOL validAssetPath(NSString *p) {
    if(!p.length || p.length>4096 || [p containsString:@":"] || [p rangeOfCharacterFromSet:NSCharacterSet.controlCharacterSet].location!=NSNotFound)return NO;
    for(NSString *c in p.pathComponents)if([c isEqual:@".."] || [c isEqual:@"."])return NO;
    return YES;
}
static size_t readRomfs(void *,const char *raw,uint64_t offset,void *out,size_t n) {
    if(!raw)return 0;
    NSString *p=[[NSString alloc]initWithUTF8String:raw];
    while([p hasPrefix:@"/"])p=[p substringFromIndex:1];
    if(!validAssetPath(p))return 0;
    if(dm_acnh_data_root()) {
        int directory=0;int64_t size=0;
        if(!dm_acnh_data_stat(p.UTF8String,&directory,&size)||directory||size<=0||size>(128u<<20))return 0;
        if(!out&&!n)return (size_t)size;
        if(!out||offset>(uint64_t)size||n>(uint64_t)size-offset)return 0;
        int fd=dm_acnh_data_open(p.UTF8String);if(fd<0)return 0;
        int handled=0;ssize_t got=dm_acnh_data_pread(fd,out,n,(off_t)offset,&handled);
        dm_acnh_data_close(fd);close(fd);return handled&&got>=0?(size_t)got:0;
    }
    const char *base=getenv("DM_ROMFS");if(!base)return 0;
    NSString *path=[@(base) stringByAppendingPathComponent:p];
    int fd=open(path.fileSystemRepresentation,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
    if(fd<0)return 0;
    struct stat s;size_t result=0;
    if(!fstat(fd,&s) && S_ISREG(s.st_mode) && s.st_size>0 && (uint64_t)s.st_size<=(128u<<20)){
        if(!out && !n)result=(size_t)s.st_size;
        else if(offset<=(uint64_t)s.st_size && n<=(uint64_t)s.st_size-offset){
            size_t done=0;while(done<n){ssize_t got=pread(fd,(uint8_t *)out+done,n-done,(off_t)(offset+done));if(got<=0)break;done+=(size_t)got;}result=done;
        }
    }close(fd);return result;
}
static EdenDsmodBool decodeASTC(void *p,uint32_t w,uint32_t h,uint32_t bw,uint32_t bh,const uint8_t *in,size_t n,uint8_t *out,size_t cap) {
    return [object(p) decodeASTC:w height:h blockWidth:bw blockHeight:bh input:in length:n output:out capacity:cap];
}
struct ImageResult { NSData *data=nil; NSUInteger w=0,h=0; };
static void receiveImage(void *p,uint32_t w,uint32_t h,const uint8_t *pixels,size_t n) {
    if(!pixels || !w || !h || w>4096 || h>4096 || uint64_t(w)*h>8u*1024*1024 || n!=uint64_t(w)*h*4)return;
    auto *r=static_cast<ImageResult *>(p);r->data=[NSData dataWithBytes:pixels length:n];r->w=w;r->h=h;
}
static NSString *imageName(NSString *key) {
    NSData *d=[key dataUsingEncoding:NSUTF8StringEncoding];uint8_t hash[CC_SHA256_DIGEST_LENGTH];CC_SHA256(d.bytes,(CC_LONG)d.length,hash);
    NSMutableString *name=[NSMutableString new];for(unsigned n=0;n<sizeof hash;n++)[name appendFormat:@"%02x",hash[n]];return [name stringByAppendingString:@".png"];
}
static BOOL writePNG(NSData *rgba,NSUInteger w,NSUInteger h,NSString *path) {
    CGDataProviderRef p=CGDataProviderCreateWithCFData((__bridge CFDataRef)rgba);CGColorSpaceRef s=CGColorSpaceCreateDeviceRGB();
    CGImageRef im=CGImageCreate(w,h,8,32,w*4,s,(CGBitmapInfo)kCGBitmapByteOrder32Big|(CGBitmapInfo)kCGImageAlphaLast,p,nullptr,false,kCGRenderingIntentDefault);
    CGImageDestinationRef dst=im ? CGImageDestinationCreateWithURL((__bridge CFURLRef)[NSURL fileURLWithPath:path],CFSTR("public.png"),1,nullptr) : nullptr;
    if(dst)CGImageDestinationAddImage(dst,im,nullptr);BOOL ok=dst && CGImageDestinationFinalize(dst);
    if(dst)CFRelease(dst);if(im)CGImageRelease(im);CGColorSpaceRelease(s);CGDataProviderRelease(p);return ok;
}
@implementation DMCompanion
+ (instancetype)shared { static DMCompanion *s;static dispatch_once_t once;dispatch_once(&once,^{s=[DMCompanion new];});return s; }
- (instancetype)init {
    if((self=[super init])){
        _worker=dispatch_queue_create("dm.companion.sample",DISPATCH_QUEUE_SERIAL);
        _assets=dispatch_queue_create("dm.companion.assets",DISPATCH_QUEUE_SERIAL);
        _latest=@{};_images=[NSCache new];_images.totalCostLimit=64u<<20;_images.countLimit=1024;_failedImages=[NSMutableDictionary new];_failedImageKeys=[NSMutableOrderedSet new];
        _page=@"map";_api=provider();
        NSString *config=[NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"dm/companion.json"];
        if(![NSFileManager.defaultManager fileExistsAtPath:config] && getenv("DM_NVN_SHMAP"))config=[[@(getenv("DM_NVN_SHMAP")) stringByDeletingLastPathComponent]stringByAppendingPathComponent:@"companion.json"];
        id parsed=[NSJSONSerialization JSONObjectWithData:[NSData dataWithContentsOfFile:config] ?: NSData.data options:0 error:nil];
        NSDictionary *metadata=[parsed isKindOfClass:NSDictionary.class] ? parsed : @{};
        _inputGates=[metadata[@"input_gates"]isKindOfClass:NSDictionary.class] ? metadata[@"input_gates"] : @{};
        _writeBuilds=[metadata[@"write_builds"]isKindOfClass:NSArray.class] ? metadata[@"write_builds"] : @[];
#if DM_HAS_COMPANION
        _extensions=eden_dsmod_get_extensions(EDEN_DSMOD_EXT_VERSION,EDEN_DSMOD_EXT_HASH);
        if(_extensions && (_extensions->version!=EDEN_DSMOD_EXT_VERSION || _extensions->abi_hash!=EDEN_DSMOD_EXT_HASH || _extensions->struct_size!=sizeof *_extensions))_extensions=nullptr;
        _writeExtensions=eden_dsmod_get_write_extensions(EDEN_DSMOD_WRITE_EXT_VERSION,EDEN_DSMOD_WRITE_EXT_HASH);
        if(_writeExtensions && (_writeExtensions->version!=EDEN_DSMOD_WRITE_EXT_VERSION || _writeExtensions->abi_hash!=EDEN_DSMOD_WRITE_EXT_HASH || _writeExtensions->struct_size!=sizeof *_writeExtensions))_writeExtensions=nullptr;
#endif
    }return self;
}
- (BOOL)supported {
    // This query also works before the guest modules have been loaded.
    return _api && _api->abi_version==EDEN_DSMOD_MODULE_ABI_VERSION && _api->abi_hash==EDEN_DSMOD_MODULE_ABI_HASH && _api->struct_size==sizeof *_api && _api->title_id==dm_companion_title_id() && _api->supports_build && _api->create && _api->sample;
}
- (NSDictionary *)snapshot { @synchronized(self){return _latest;} }
- (void)publish:(NSString *)key value:(id)value { if(key.length<512 && _output.count<20000)_output[key]=value; }
- (int64_t)query:(NSString *)key fallback:(int64_t)value {
    if([key isEqual:@"__relocation_delta"])return 0;
    if([key isEqual:@"__aux_present"])return self.visible;
    NSNumber *n=self.snapshot[key];return [n isKindOfClass:NSNumber.class] ? n.longLongValue : value;
}
- (void)setVisible:(BOOL)visible {
    @synchronized(self){if(_visible!=visible)++_visibilityGeneration;_visible=visible;if(!visible){_pressMask=0;_pressUntil=0;_moduleMask=0;++_pressGeneration;dm_companion_input_reset();}}
    dispatch_async(_worker,^{[self refreshSampling];});
}
- (BOOL)visible { @synchronized(self){return _visible;} }
- (void)refreshSampling {
    if(!_timer)return;
    if(!self.visible){dispatch_source_set_timer(_timer,DISPATCH_TIME_FOREVER,DISPATCH_TIME_FOREVER,0);return;}
    // Read-only pages sample at the same cadence as their presentation.
    NSString *page=self.page;
    uint64_t ms=([page isEqual:@"bag"] || [page isEqual:@"phone"]) ? 200 : [page isEqual:@"map"] ? 200 : 500;
    dispatch_source_set_timer(_timer,DISPATCH_TIME_NOW,ms*NSEC_PER_MSEC,5*NSEC_PER_MSEC);
}
- (void)start {
    @synchronized(self){
        if(_timer || !self.supported)return;
        self.visible=YES;
        _timer=dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,0,0,_worker);
        dispatch_source_set_timer(_timer,DISPATCH_TIME_NOW,50*NSEC_PER_MSEC,5*NSEC_PER_MSEC);
        __weak DMCompanion *weak=self;dispatch_source_set_event_handler(_timer,^{@autoreleasepool{[weak sample];}});
        dispatch_resume(_timer);
        [self refreshSampling];
    }
}
- (BOOL)createModule {
    dm_companion_guest g;if(!dm_companion_guest_info(&g))return NO;
    if(!_api->supports_build(g.build_id)){
        @synchronized(self){_latest=@{@"host.unsupported":@1};}return NO;
    }
    _host={};_host.abi_version=EDEN_DSMOD_MODULE_ABI_VERSION;_host.struct_size=sizeof _host;_host.abi_hash=EDEN_DSMOD_MODULE_ABI_HASH;
    _host.capabilities=EDEN_DSMOD_CAP_ROMFS_READ|EDEN_DSMOD_CAP_EXTENSIONS;
    _host.userdata=(__bridge void *)self;_host.title_id=g.title_id;_host.main_base=g.main_base;_host.main_size=g.main_size;
    for(unsigned i=0;i<16;i++){unsigned n=0;sscanf(g.build_id+i*2,"%2x",&n);_host.build_id[i]=(uint8_t)n;}
    _host.get_tick=tick;_host.get_heap_begin=heapBegin;_host.get_heap_end=heapEnd;_host.is_mapped=mapped;_host.read_memory=readMemory;
    _host.log=moduleLog;_host.begin_output=beginOutput;_host.end_output=beginOutput;_host.publish_i64=publishInt;_host.publish_f64=publishFloat;
    _host.publish_text=publishText;_host.publish_address=publishAddress;_host.read_romfs=readRomfs;_host.get_i64=getInt;_host.get_f64=getFloat;_host.get_text=getText;
    _output=[NSMutableDictionary new];_instance=_api->create(&_host,"{}");if(!_instance)return NO;
    if(_extensions && _extensions->configure){
        EdenDsmodHostExtensions ext={};ext.version=EDEN_DSMOD_EXT_VERSION;ext.struct_size=sizeof ext;ext.abi_hash=EDEN_DSMOD_EXT_HASH;
        ext.userdata=(__bridge void *)self;ext.decode_astc=decodeASTC;_extensions->configure(_instance,&ext);
    }
    BOOL writes=NO;
    if(_writeExtensions && _writeExtensions->configure && [_writeBuilds containsObject:@(g.build_id)]){
        EdenDsmodHostWriteApi api={};api.version=EDEN_DSMOD_WRITE_EXT_VERSION;api.struct_size=sizeof api;api.abi_hash=EDEN_DSMOD_WRITE_EXT_HASH;
        api.userdata=(__bridge void *)self;api.write_batch=writeBatch;_writeExtensions->configure(_instance,&api);writes=YES;
    }
    NSInteger page=[@[@"map",@"bag",@"critters",@"diy",@"today",@"phone"]indexOfObject:self.page];
    if(_extensions && _extensions->on_action)_extensions->on_action(_instance,"ui_open",page<6 ? page+1 : 1);
    @synchronized(self){_moduleReady=YES;}
    fprintf(stderr,"companion: started %s on build %s (%s)\n",_api->name,g.build_id,writes ? "guarded inventory writes enabled" : "read-only guest access");
    return YES;
}
- (void)sample {
    if(!self.visible || !self.supported)return;
    uint64_t generation;
    @synchronized(self){if(!_visible)return;generation=_visibilityGeneration;}
    if(!_instance && ![self createModule])return;
    _output=[NSMutableDictionary new];_api->sample(_instance,&_host);if(_api->tick)_api->tick(_instance,&_host);
    ++_sample;
    // The upstream phone navigation emits ordinary controller gates. Keep this input
    // independent of physical pads and touchscreen controls.
    uint64_t mask=0;for(NSString *field in _inputGates)if([_output[field]boolValue])mask|=[_inputGates[field]unsignedLongLongValue];
    // Visibility may change during a guest sample. Serialize the last publication with
    // backgrounding so an in-flight sample cannot reintroduce a held button after reset.
    @synchronized(self){if(generation!=_visibilityGeneration)return;_moduleMask=_visible ? mask:0;if(tick(nullptr)<_pressUntil)mask|=_pressMask;else _pressMask=0;if(_visible)dm_companion_input(mask);else dm_companion_input_reset();_latest=[_output copy];}
    if(_trace && ++_traceTick%10==0){
        NSString *file=[_trace stringByAppendingPathComponent:@"command.json"];
        NSDictionary *cmd=[NSJSONSerialization JSONObjectWithData:[NSData dataWithContentsOfFile:file] ?: NSData.data options:0 error:nil];
        if([cmd isKindOfClass:NSDictionary.class] && [cmd[@"sequence"]unsignedLongLongValue]>_command){
            _command=[cmd[@"sequence"]unsignedLongLongValue];
            if([cmd[@"action"]isKindOfClass:NSString.class])[self action:cmd[@"action"] argument:[cmd[@"argument"]longLongValue]];
            NSArray *images=[cmd[@"images"]isKindOfClass:NSArray.class] ? cmd[@"images"] : @[];
            for(NSString *key in images)if([key isKindOfClass:NSString.class])[self image:key completion:^(NSData *d,NSUInteger w,NSUInteger h){
                if(d)writePNG(d,w,h,[self->_trace stringByAppendingPathComponent:imageName(key)]);
            }];
            NSMutableDictionary *names=[NSMutableDictionary new];for(NSString *key in images)if([key isKindOfClass:NSString.class])names[key]=imageName(key);
            [[NSJSONSerialization dataWithJSONObject:names options:NSJSONWritingSortedKeys error:nil]writeToFile:[_trace stringByAppendingPathComponent:@"images.json"] atomically:YES];
        }
        [[NSJSONSerialization dataWithJSONObject:self.snapshot options:NSJSONWritingSortedKeys error:nil]writeToFile:[_trace stringByAppendingPathComponent:@"snapshot.json"] atomically:YES];
    }
}
- (void)action:(NSString *)name argument:(int64_t)argument {
    if(!name.length || name.length>128)return;
    if([name hasPrefix:@"host.press."] || [name hasPrefix:@"bag_move"] ||
       [name hasPrefix:@"bag_use"] || [name hasPrefix:@"bag_command"] ||
       [name hasPrefix:@"bag_sort"])return;
    if([name hasPrefix:@"host.press."]){
        NSString *gate=[@"pdrv." stringByAppendingString:[name substringFromIndex:11]];
        uint64_t generation,duration=(uint64_t)MAX(50,MIN(500,argument));
        @synchronized(self){
            uint64_t press=[_inputGates[gate]unsignedLongLongValue];
            if(!_visible || !press)return;
            _pressMask=press;_pressUntil=tick(nullptr)+duration;generation=++_pressGeneration;
            // Button edges must not wait for a passive page's 500 ms data timer.
            dm_companion_input(_moduleMask|_pressMask);
        }
        __weak DMCompanion *weak=self;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,duration*NSEC_PER_MSEC),dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE,0),^{
            DMCompanion *strong=weak;if(!strong)return;
            @synchronized(strong){
                if(generation!=strong->_pressGeneration)return;
                strong->_pressMask=0;strong->_pressUntil=0;
                if(strong->_visible)dm_companion_input(strong->_moduleMask);else dm_companion_input_reset();
            }
        });
        return;
    }
    if([name isEqual:@"ui_open"] && argument>=1 && argument<=6)self.page=@[@"map",@"bag",@"critters",@"diy",@"today",@"phone"][argument-1];
    dispatch_async(_worker,^{
        [self refreshSampling];
        if(!self->_instance || !self->_extensions || !self->_extensions->on_action)return;
        BOOL accepted=self->_extensions->on_action(self->_instance,name.UTF8String,argument);
        if(accepted && [name isEqual:@"ui_open"] && argument>=1 && argument<=6)self.page=@[@"map",@"bag",@"critters",@"diy",@"today",@"phone"][argument-1];
        if(!accepted)fprintf(stderr,"companion: action refused: %s\n",name.UTF8String);
    });
}
- (void)image:(NSString *)key completion:(void (^)(NSData *,NSUInteger,NSUInteger))completion {
    if(![key hasPrefix:@"module:"] || key.length>4096){completion(nil,0,0);return;}
    NSDictionary *cached=[_images objectForKey:key];
    if(cached){completion(cached[@"data"],[cached[@"w"]unsignedIntegerValue],[cached[@"h"]unsignedIntegerValue]);return;}
    dispatch_async(_assets,^{@autoreleasepool{
        NSDictionary *existing=[self->_images objectForKey:key];
        if(existing){completion(existing[@"data"],[existing[@"w"]unsignedIntegerValue],[existing[@"h"]unsignedIntegerValue]);return;}
        double now=NSProcessInfo.processInfo.systemUptime;
        // Failures suppress retries for one second; expired keys must not become
        // process-lifetime metadata. Insertion order also bounds a burst of misses.
        while(self->_failedImageKeys.count){
            NSString *old=self->_failedImageKeys.firstObject;
            if(now-[self->_failedImages[old]doubleValue]<1)break;
            [self->_failedImages removeObjectForKey:old];[self->_failedImageKeys removeObjectAtIndex:0];
        }
        NSNumber *failed=self->_failedImages[key];
        if(failed && now-failed.doubleValue<1){completion(nil,0,0);return;}
        EdenDsmodHostApi h;void *instance;
        @synchronized(self){if(!self->_moduleReady){completion(nil,0,0);return;}h=self->_host;instance=self->_instance;}
        if(!instance || !self->_extensions || !self->_extensions->load_image){completion(nil,0,0);return;}
        ImageResult r;
        h.publish_i64=nullptr;h.publish_f64=nullptr;h.publish_text=nullptr;h.publish_address=nullptr;
        h.get_i64=assetInt;h.get_f64=nullptr;h.get_text=nullptr;
        BOOL ok=self->_extensions->load_image(instance,&h,key.UTF8String,&r,receiveImage);
        if(ok && r.data){NSDictionary *entry=@{@"data":r.data,@"w":@(r.w),@"h":@(r.h)};[self->_images setObject:entry forKey:key cost:r.data.length];[self->_failedImages removeObjectForKey:key];[self->_failedImageKeys removeObject:key];completion(r.data,r.w,r.h);}
        else{
            while(self->_failedImageKeys.count>=512){NSString *old=self->_failedImageKeys.firstObject;[self->_failedImages removeObjectForKey:old];[self->_failedImageKeys removeObjectAtIndex:0];}
            self->_failedImages[key]=@(NSProcessInfo.processInfo.systemUptime);[self->_failedImageKeys addObject:key];completion(nil,0,0);
        }
    }});
}
- (BOOL)decodeASTC:(uint32_t)w height:(uint32_t)h blockWidth:(uint32_t)bw blockHeight:(uint32_t)bh input:(const uint8_t *)input length:(size_t)length output:(uint8_t *)output capacity:(size_t)capacity {
    if(!w || !h || w>4096 || h>4096 || uint64_t(w)*h*4>capacity || !input || !output)return NO;
    static std::mutex mutex;std::lock_guard<std::mutex> lock(mutex);
    static id<MTLDevice> device;static id<MTLCommandQueue> queue;static id<MTLComputePipelineState> pipeline;static dispatch_once_t once;
    dispatch_once(&once,^{
        device=MTLCreateSystemDefaultDevice();queue=[device newCommandQueue];NSError *e;
        NSString *source=@"#include <metal_stdlib>\nusing namespace metal;\nkernel void decode(texture2d<float> src [[texture(0)]],device uchar4 *dst [[buffer(0)]],uint2 p [[thread_position_in_grid]]){if(p.x>=src.get_width()||p.y>=src.get_height())return;constexpr sampler s(coord::pixel,address::clamp_to_edge,filter::nearest);dst[p.y*src.get_width()+p.x]=uchar4(clamp(src.sample(s,float2(p)+0.5f),0.0f,1.0f)*255.0f+0.5f);}";
        id<MTLLibrary> lib=[device newLibraryWithSource:source options:nil error:&e];
        if(lib)pipeline=[device newComputePipelineStateWithFunction:[lib newFunctionWithName:@"decode"] error:&e];
        if(!pipeline)fprintf(stderr,"companion: ASTC decoder unavailable: %s\n",e.localizedDescription.UTF8String);
    });
    if(!pipeline)return NO;
    constexpr std::array<std::pair<uint32_t,uint32_t>,14> sizes={{{4,4},{5,4},{5,5},{6,5},{6,6},{8,5},{8,6},{8,8},{10,5},{10,6},{10,8},{10,10},{12,10},{12,12}}};
    const MTLPixelFormat formats[]={MTLPixelFormatASTC_4x4_LDR,MTLPixelFormatASTC_5x4_LDR,MTLPixelFormatASTC_5x5_LDR,MTLPixelFormatASTC_6x5_LDR,MTLPixelFormatASTC_6x6_LDR,MTLPixelFormatASTC_8x5_LDR,MTLPixelFormatASTC_8x6_LDR,MTLPixelFormatASTC_8x8_LDR,MTLPixelFormatASTC_10x5_LDR,MTLPixelFormatASTC_10x6_LDR,MTLPixelFormatASTC_10x8_LDR,MTLPixelFormatASTC_10x10_LDR,MTLPixelFormatASTC_12x10_LDR,MTLPixelFormatASTC_12x12_LDR};
    unsigned index=14;for(unsigned i=0;i<14;i++)if(sizes[i].first==bw && sizes[i].second==bh)index=i;
    if(index==14 || length<size_t((w+bw-1)/bw)*((h+bh-1)/bh)*16)return NO;
    MTLTextureDescriptor *desc=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:formats[index] width:w height:h mipmapped:NO];
    desc.storageMode=MTLStorageModeShared;desc.usage=MTLTextureUsageShaderRead;
    id<MTLTexture> tex=[device newTextureWithDescriptor:desc];if(!tex)return NO;
    [tex replaceRegion:MTLRegionMake2D(0,0,w,h) mipmapLevel:0 withBytes:input bytesPerRow:((w+bw-1)/bw)*16];
    id<MTLBuffer> result=[device newBufferWithLength:size_t(w)*h*4 options:MTLResourceStorageModeShared];if(!result)return NO;
    id<MTLCommandBuffer> cb=[queue commandBuffer];id<MTLComputeCommandEncoder> enc=[cb computeCommandEncoder];
    [enc setComputePipelineState:pipeline];[enc setTexture:tex atIndex:0];[enc setBuffer:result offset:0 atIndex:0];
    [enc dispatchThreads:MTLSizeMake(w,h,1) threadsPerThreadgroup:MTLSizeMake(8,8,1)];[enc endEncoding];[cb commit];[cb waitUntilCompleted];
    if(cb.status==MTLCommandBufferStatusError)return NO;memcpy(output,result.contents,size_t(w)*h*4);return YES;
}
- (void)trace:(NSString *)directory {
    [NSFileManager.defaultManager createDirectoryAtPath:directory withIntermediateDirectories:YES attributes:nil error:nil];
    _trace=[directory copy];[self start];
}
@end
extern "C" void dm_companion_trace_start(const char *directory) { if(directory && *directory)[DMCompanion.shared trace:@(directory)]; }
