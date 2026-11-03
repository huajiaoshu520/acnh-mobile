// The injected-reader model tests never initialize a native guest engine.
#import "../../host/companion_apple.h"
@implementation DMCompanion
+ (instancetype)shared { return nil; }
- (BOOL)supported { return NO; }
- (NSDictionary *)snapshot { return @{}; }
- (void)start {}
- (void)setVisible:(BOOL)value {}
- (void)action:(NSString *)name argument:(int64_t)value {}
- (void)image:(NSString *)key completion:(void (^)(NSData *,NSUInteger,NSUInteger))completion { completion(nil,0,0); }
@end
