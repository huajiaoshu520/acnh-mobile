import AppKit
import Darwin

let kind = CommandLine.arguments.dropFirst().first ?? "game"
let app = NSApplication.shared
app.setActivationPolicy(.accessory)
// Follow macOS preferred UI language; keep file names such as prod.keys unchanged.
let preferred = (Locale.preferredLanguages.first ?? "en").lowercased().replacingOccurrences(of: "_", with: "-")
let language = preferred.hasPrefix("zh") ? ((preferred.contains("hant") || preferred.hasSuffix("-tw") || preferred.hasSuffix("-hk") || preferred.hasSuffix("-mo")) ? "zh-Hant" : "zh-Hans") : (preferred.hasPrefix("ja") ? "ja" : "en")
let number = kind == "keys" ? 2 : (kind == "output" ? 3 : 1)
let subjects: [String: [String]] = [
    "zh-Hans": ["动森游戏目录 · 仅支持 3.0.3", "选择 prod.keys 文件", "选择成品保存文件夹"],
    "zh-Hant": ["動森遊戲目錄 · 僅支援 3.0.3", "選擇 prod.keys 檔案", "選擇成品儲存資料夾"],
    "ja": ["ゲームフォルダ · 対応バージョン 3.0.3", "prod.keys ファイルを選択", "出力先フォルダを選択"],
    "en": ["Game folder · version 3.0.3 only", "Select the prod.keys file", "Select the output folder"]
]
let descriptions: [String: [String]] = [
    "zh-Hans": ["请选择同时包含动森本体和 3.0.3 更新 NSP／XCI／NSZ／XCZ 的文件夹。当前仅支持游戏 3.0.3。", "请选择名为 prod.keys 的密钥文件，不是文件夹，也不是 title.keys。", "请选择成品保存文件夹。生成的 IPA、数据包或 Mac 应用将放在这里，也可以新建文件夹。"],
    "zh-Hant": ["請選擇同時包含動森本體與 3.0.3 更新 NSP／XCI／NSZ／XCZ 的資料夾。目前僅支援遊戲 3.0.3。", "請選擇名為 prod.keys 的金鑰檔案，不是資料夾，也不是 title.keys。", "請選擇成品儲存資料夾。產生的 IPA、資料包或 Mac 應用程式將放在這裡，也可以新增資料夾。"],
    "ja": ["ゲーム本体と 3.0.3 更新 NSP／XCI／NSZ／XCZ が入ったフォルダを選択してください。対応するゲームバージョンは 3.0.3 のみです。", "prod.keys という名前のキーファイルを選択してください。フォルダや title.keys は選択しないでください。", "IPA、データパック、または Mac アプリの保存先フォルダを選択してください。新しいフォルダも作成できます。"],
    "en": ["Select the folder containing the Animal Crossing base game and its 3.0.3 update NSP/XCI/NSZ/XCZ. Only game version 3.0.3 is supported.", "Select the key file named prod.keys, not a folder or title.keys.", "Select a folder for the generated IPA, data pack or Mac app. You can also create a new folder."]
]
let buttons: [String: [String]] = [
    "zh-Hans": ["使用此游戏目录", "使用此密钥", "保存到此文件夹"],
    "zh-Hant": ["使用此遊戲目錄", "使用此金鑰", "儲存至此資料夾"],
    "ja": ["このゲームフォルダを使用", "このキーを使用", "このフォルダに保存"],
    "en": ["Use Game Folder", "Use This Key", "Save Here"]
]
let stepFormats = ["zh-Hans": "第 %d 步 / 共 3 步", "zh-Hant": "第 %d 步 / 共 3 步", "ja": "ステップ %d/3", "en": "Step %d/3"]
let keyErrors = ["zh-Hans": "请选择名为 prod.keys 的密钥文件。", "zh-Hant": "請選擇名為 prod.keys 的金鑰檔案。", "ja": "prod.keys という名前のキーファイルを選択してください。", "en": "Select the key file named prod.keys."]
let step = String(format: stepFormats[language]!, number)
let subject = subjects[language]![number - 1]
let detail = descriptions[language]![number - 1]
let button = buttons[language]![number - 1]
let useColor = isatty(STDERR_FILENO) != 0 && ProcessInfo.processInfo.environment["NO_COLOR"] == nil && ProcessInfo.processInfo.environment["TERM"] != "dumb"
let terminalSubject = useColor ? "\u{001B}[1;31m\(subject)\u{001B}[0m" : subject
FileHandle.standardError.write(Data("\n\(step): \(terminalSubject)\n\(detail)\n".utf8))

final class KeySelectionDelegate: NSObject, NSOpenSavePanelDelegate {
    func panel(_ sender: Any, shouldEnable url: URL) -> Bool {
        url.hasDirectoryPath || url.lastPathComponent == "prod.keys"
    }
    func panel(_ sender: Any, validate url: URL) throws {
        if url.hasDirectoryPath || url.lastPathComponent != "prod.keys" {
            throw NSError(domain: "ACNHBuildInput", code: 1,
                          userInfo: [NSLocalizedDescriptionKey: keyErrors[language]!])
        }
    }
}
let panel = NSOpenPanel()
panel.canChooseDirectories = kind != "keys"
panel.canChooseFiles = kind == "keys"
panel.allowsMultipleSelection = false
panel.canCreateDirectories = kind == "output"
panel.title = subject
panel.message = "\(step): \(detail)"
panel.prompt = button

let keyDelegate = KeySelectionDelegate()
if kind == "keys" { panel.delegate = keyDelegate }
app.activate(ignoringOtherApps: true)
let response = withExtendedLifetime(keyDelegate) { panel.runModal() }
if response == .OK, let url = panel.url { print(url.path) } else { exit(130) }
