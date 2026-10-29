#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
// Show a real network setup failure at the airport instead of swallowing it in
// the legacy ErrorResultVariant log. Does not alter return codes or permissions.
#import <UIKit/UIKit.h>
#include <dns_sd.h>
#include <errno.h>
#include <stdio.h>

void front_lan_failed(int code,const char *stage) {
    NSString *where=stage?@(stage):@"LAN";
    NSLog(@"[ACNH LAN] failure %@ code=%d process=%@ bundle=%@",where,code,NSProcessInfo.processInfo.processName,NSBundle.mainBundle.bundleIdentifier);
    dispatch_async(dispatch_get_main_queue(), ^{
        static BOOL showing=NO;
        if(showing)return;
        UIViewController *vc=nil;
        for(UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
            if(![scene isKindOfClass:UIWindowScene.class])continue;
            for(UIWindow *w in ((UIWindowScene *)scene).windows)if(w.isKeyWindow)vc=w.rootViewController;
        }
        while(vc.presentedViewController)vc=vc.presentedViewController;
        if(!vc)return;
        NSString *message;
        if(code==kDNSServiceErr_NoAuth) {
            message=@"当前运行应用没有获准发现本地联机服务。\n\n如果通过 LiveContainer 运行，需要在 LiveContainer 本体声明 _acnhldn._udp，并重新签名安装宿主。仅更新动森测试包不能解决此问题。";
        } else if(code==kDNSServiceErr_PolicyDenied||code==kDNSServiceErr_NotPermitted) {
            message=@"请在系统设置中允许当前运行应用使用本地网络。如果通过 LiveContainer 运行，请检查 LiveContainer 的本地网络权限。";
        } else if(code==ENETDOWN) {
            message=@"没有找到可用的 Wi-Fi IPv4 地址。请连接同一 Wi-Fi 后重试。";
        } else {
            message=[NSString stringWithFormat:@"本地联机初始化失败（%@，%d）。请保留 logs/log.txt，以便定位原因。",where,code];
        }
        UIAlertController *a=[UIAlertController alertControllerWithTitle:@"本地联机未能启动" message:message preferredStyle:UIAlertControllerStyleAlert];
        [a addAction:[UIAlertAction actionWithTitle:@"知道了" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action){(void)action;showing=NO;}]];
        showing=YES;[vc presentViewController:a animated:YES completion:nil];
    });
}

#endif
