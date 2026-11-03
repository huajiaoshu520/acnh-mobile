#pragma once
#import <Foundation/Foundation.h>
#include "companion_layout.h"
// Main-thread UI bridge; the module ABI does not receive these capabilities.
void dm_companion_ui_activity(int source,int busy);
NS_ASSUME_NONNULL_BEGIN
@interface DMCompanion : NSObject
+ (instancetype)shared;
@property(readonly) BOOL supported;
@property(readonly) NSDictionary *snapshot;
- (void)start;
- (void)setVisible:(BOOL)visible;
- (void)action:(NSString *)name argument:(int64_t)argument;
- (void)image:(NSString *)key completion:(void (^)(NSData * _Nullable pixels,NSUInteger width,NSUInteger height))completion;
@end
NS_ASSUME_NONNULL_END
