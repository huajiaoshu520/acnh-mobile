import Foundation
func check(_ actual: String, _ expected: String) { precondition(actual == expected, "\(actual) != \(expected)") }

@main
struct LauncherStringsTests {
    static func main() {
        check(LauncherText.label(env: "DM_OPMODE", fallback: "Console mode", language: "zh-Hans"), "运行模式")
        check(LauncherText.label(env: "DM_OPMODE", fallback: "Console mode", language: "zh-Hant"), "運行模式")
        check(LauncherText.option(env: "DM_OPMODE", value: "0", fallback: "Handheld", language: "zh-Hans"), "掌机模式")
        check(LauncherText.option(env: "DM_FULLSCREEN", value: "1", fallback: "Fullscreen", language: "zh-Hant"), "全屏")
        check(LauncherText.option(env: "ACNH_CONTROLLER_LAYOUT", value: "custom", fallback: "Custom", language: "zh-Hans"), "自定义")
        check(LauncherText.text("Advanced settings", "高级设置", language: "zh-TW"), "高級設置")
        check(LauncherText.section("Game", language: "zh-HK"), "遊戲")
        check(LauncherText.label(env: "DM_OPMODE", fallback: "Console mode", language: "en-US"), "Console mode")
        check(LauncherText.note(env: "DM_PRESENT_SNAP", fallback: "Up to one frame more display latency.", language: "ja-JP"), "表示の遅延が最大 1 フレーム増えます。")
        check(LauncherText.note(env: "DM_PSO_MTL4", fallback: "", language: "zh-Hans"), "")
        check(LauncherText.option(env: "DM_LANG", value: "en-US", fallback: "American English", language: "zh-Hans"), Locale(identifier: "zh-Hans").localizedString(forIdentifier: "en-US")!)
        check(LauncherText.label(env: "ACNH_DISPLAY_MODE", fallback: "Picture mode", language: "zh-Hans"), "画面显示")
        check(LauncherText.option(env: "ACNH_DISPLAY_MODE", value: "crop", fallback: "Crop", language: "zh-Hant"), "裁剪")
        print("Shared Mac/iPhone strings: simplified, traditional, Japanese notes, English fallback and system-language names passed")
    }
}
