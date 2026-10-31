// Device capability gate from NX 6d880d23. UI language is independent of game language.
#pragma once
#import <Foundation/Foundation.h>
static inline NSString *acnh_device_message(BOOL hasDevice, BOOL supportsBC, NSString *gpu, NSString *language) {
    if (hasDevice && supportsBC) return nil;
    NSString *code=language.lowercaseString ?: @"en";
    NSString *name=gpu.length?gpu:@"unknown";
    if ([code hasPrefix:@"zh"]) {
        NSString *message=hasDevice?[NSString stringWithFormat:@"此设备暂不支持运行。\n\nGPU：%@\n不支持游戏所需的 BC 压缩纹理。\n\n需要支持 BC 纹理的 GPU，例如 A17 Pro 或 M 系列芯片。升级系统不能补齐这项硬件能力。",name]:@"此设备暂不支持运行。\n未找到可用的 Metal 图形设备。";
        BOOL hant=[code containsString:@"hant"] || (![code containsString:@"hans"] && ([@[@"tw",@"hk",@"mo"] containsObject:[code componentsSeparatedByString:@"-"].lastObject]));
        return hant?([message stringByApplyingTransform:@"Hans-Hant" reverse:NO] ?: message):message;
    }
    if ([code hasPrefix:@"ja"]) return hasDevice?[NSString stringWithFormat:@"このデバイスはサポートされていません。\n\nGPU：%@\nゲームに必要な BC 圧縮テクスチャに対応していません。\n\nA17 Pro や M シリーズなど、BC テクスチャ対応の GPU が必要です。OS の更新では追加できないハードウェア機能です。",name]:@"このデバイスはサポートされていません。\n利用可能な Metal デバイスが見つかりません。";
    return hasDevice?[NSString stringWithFormat:@"This device is not supported.\n\nGPU: %@\nIt cannot read the BC-compressed textures used by the game.\n\nA BC-capable GPU is required, such as A17 Pro or an M-series chip. An OS update cannot add this hardware capability.",name]:@"This device is not supported.\nNo Metal device is available.";
}
