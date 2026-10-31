// Launcher language follows the device/app language, independently of DM_LANG.
import Foundation

enum LauncherText {
    static func text(_ english: String, _ simplified: String, language: String = Locale.preferredLanguages.first ?? "en") -> String {
        guard language.lowercased().hasPrefix("zh") else { return english }
        let code = language.lowercased()
        let traditional = code.contains("hant") || (!code.contains("hans") && ["tw", "hk", "mo"].contains(code.split(separator: "-").last.map(String.init) ?? ""))
        return traditional ? simplified.applyingTransform(StringTransform("Hans-Hant"), reverse: false) ?? simplified : simplified
    }
    // Shared by the Mac and iPhone views; UI language never follows DM_LANG.
    static func label(env: String, fallback: String, language: String = Locale.preferredLanguages.first ?? "en") -> String {
        let names = ["DM_PRESENT_UPSCALE": "画面放大", "DM_RUMBLE_MIN": "震动阈值", "DM_DYNAMIC_RESOLUTION": "动态分辨率", "DM_PSO_MTL4": "Metal 4 管线", "DM_PSO_PREBUILD": "管线预构建", "DM_NVN_ENC_THREAD": "编码线程", "DM_FPS_HOLD": "延迟时保持帧", "DM_PRESENT_SNAP": "帧缓冲", "DM_RUMBLE": "游戏震动", "DM_RUMBLE_GAIN": "震动强度", "DM_TOUCH_HAPTICS": "触屏反馈", "ACNH_CUSTOM_BASE": "调整对象", "ACNH_AUDIO_IGNORE_SILENT": "静音模式下播放声音", "ACNH_CONTROLLER_LAYOUT": "控制器布局", "DM_FPS_TARGET": "帧率", "DM_RENDER_SCALE": "渲染比例", "DM_OPMODE": "运行模式", "DM_LANG": "游戏语言", "DM_SHOW_FPS": "显示帧率", "DM_METAL_HUD": "性能信息面板", "DM_FULLSCREEN": "显示模式", "ACNH_DISPLAY_MODE": "画面显示"]
        return text(fallback, names[env] ?? fallback, language: language)
    }
    static func option(env: String, value: String, fallback: String, language: String = Locale.preferredLanguages.first ?? "en") -> String {
        if env == "ACNH_CONTROLLER_LAYOUT" || env == "ACNH_CUSTOM_BASE" {
            return text(fallback, value == "custom" ? "自定义" : value == "generic" ? "通用" : "专属", language: language)
        }
        if env == "DM_OPMODE" { return text(fallback, value == "1" ? "主机模式" : "掌机模式", language: language) }
        if env == "ACNH_DISPLAY_MODE" { return text(fallback, ["auto":"自适应", "original":"原版", "stretch":"拉伸", "crop":"裁剪"][value] ?? fallback, language: language) }
        if env == "DM_PRESENT_UPSCALE" { return text(fallback, value == "metalfx" ? "MetalFX" : "标准", language: language) }
        if env == "DM_FULLSCREEN" { return text(fallback, value == "1" ? "全屏" : "窗口", language: language) }
        if env == "DM_LANG" { return Locale(identifier: language).localizedString(forIdentifier: value) ?? fallback }
        return fallback
    }
    static func section(_ name: String, language: String = Locale.preferredLanguages.first ?? "en") -> String {
        text(name, ["Game":"游戏", "Controls":"控制", "Performance":"性能"][name] ?? name, language: language)
    }
    static func note(env: String, fallback: String, language: String = Locale.preferredLanguages.first ?? "en") -> String {
        if language.lowercased().hasPrefix("ja"), !fallback.isEmpty {
            let japanese = ["DM_PRESENT_UPSCALE": "GPU処理時間と発熱が増える場合があります。",
                "ACNH_DISPLAY_MODE": "自動は対応する横長のiPhoneで比率を保って視野を拡張し、iPadでは既存の配置を維持します。オリジナルは16:9を維持します。引き伸ばしは画面全体に拡大し、トリミングは比率を保って端を切り取ります。",
                "DM_FPS_HOLD": "公式のフレーム制御を使う場合、遅れたフレームの表示を延長します。",
                "DM_PSO_MTL4": "パイプラインアーカイブを保持するため、メモリ使用量が増えます。",
                "DM_PSO_PREBUILD": "初回起動時に数分かかる場合があります。次回以降は再利用します。",
                "DM_PRESENT_SNAP": "表示の遅延が最大 1 フレーム増えます。",
                "DM_RUMBLE": "ゲームからの振動をスマートフォンや接続したコントローラーで再生します。",
                "DM_RUMBLE_GAIN": "ゲーム内の振動設定と組み合わせて強さを調整します。",
                "DM_TOUCH_HAPTICS": "タッチ操作のボタンやスティックを押したときに軽く振動します。",
                "DM_OPMODE": "携帯モードは低い解像度で描画するため、通常は GPU の負荷と消費電力を抑えられます。一部のゲームは携帯モードでのみタッチ操作に対応します。",
                "DM_METAL_HUD": "ゲーム画面に Apple Metal のパフォーマンス情報を表示します：フレームレート、フレーム時間、GPU 時間、メモリ使用量。",
                "DM_FPS_TARGET": "30 fps にすると消費電力と発熱を抑えられます。",
                "DM_RENDER_SCALE": "100% 未満では、3D 画面を選択した倍率で描画し、ゲームの動的解像度を無効にします。UI の文字は鮮明なままです。"
            ]
            return japanese[env] ?? fallback
        }
        let notes = ["DM_PRESENT_UPSCALE": "可能增加 GPU 耗时和发热。",
            "ACNH_DISPLAY_MODE": "自适应在宽屏 iPhone 扩展视野并保持比例，iPad 沿用原布局。原版保持16:9；拉伸填满屏幕，裁剪保持比例并隐藏边缘。",
            "DM_FPS_HOLD": "使用官方帧率步进时，延迟帧可保持更久。",
            "DM_PSO_MTL4": "保留管线归档，会增加内存占用。",
            "DM_PSO_PREBUILD": "首次启动可能需要等待几分钟，完成后后续启动复用。",
            "DM_PRESENT_SNAP": "最多增加一帧显示延迟。",
            "DM_RUMBLE": "通过手机或已连接的手柄呈现游戏内震动。",
            "DM_RUMBLE_GAIN": "配合游戏内的震动设置，调整游戏震动强度。",
            "DM_TOUCH_HAPTICS": "按下触屏按键或开始触摸摇杆时提供轻微震动反馈。",
            "DM_OPMODE": "掌机模式以较低分辨率渲染，通常能减轻 GPU 负担并节省电量；部分游戏仅在掌机模式下读取触屏操作。",
            "DM_METAL_HUD": "在游戏画面上显示 Apple Metal 性能面板：帧率、每帧耗时、GPU 耗时及内存。",
            "DM_FPS_TARGET": "30 帧可节省电量并减少发热。",
            "DM_RENDER_SCALE": "低于 100% 时，3D 画面按所选比例渲染并关闭游戏动态分辨率，界面文字保持清晰。"
        ]
        guard let chinese = notes[env], !fallback.isEmpty else { return fallback }
        return text(fallback, chinese, language: language)
    }
}
