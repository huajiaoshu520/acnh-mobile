#import "../Sources/DeviceSupport.h"
#include <assert.h>
int main(void){@autoreleasepool{
 assert(acnh_device_message(YES,YES,@"GPU",@"zh-Hans")==nil);
 assert([acnh_device_message(YES,NO,@"Apple A16",@"zh-Hans") containsString:@"此设备"]);
 assert([acnh_device_message(YES,NO,@"Apple A16",@"zh-Hant") containsString:@"此裝置"] || [acnh_device_message(YES,NO,@"Apple A16",@"zh-Hant") containsString:@"此設備"]);
 assert([acnh_device_message(YES,NO,@"Apple A16",@"zh-TW") containsString:@"壓縮"]);
 assert([acnh_device_message(YES,NO,@"Apple A16",@"ja") containsString:@"サポート"]);
 assert([acnh_device_message(YES,NO,@"Apple A16",@"en") containsString:@"Apple A16"]);
 assert([acnh_device_message(NO,NO,nil,@"en") containsString:@"No Metal"]);
 puts("PASS: supported GPU, rejected BC capability, no GPU and system-language messages");
}return 0;}
