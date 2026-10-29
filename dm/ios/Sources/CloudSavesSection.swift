import SwiftUI
import UIKit
/// Cloud saves: an iCloud Drive folder shared with the Mac app (chosen in its settings window's
/// Cloud saves too). The host brings the save in from there when the game starts and
/// sends it there after each save (dm/host/save_sync.h); the folder is a bookmark (CloudSaves.m).
struct CloudSavesSection: View {
    @State private var folder: String = ""
    @State private var note: String = ""
    let chooseFolder: (@escaping (Int) -> Void) -> Void

    var body: some View {
        Section {
            HStack {
                Text(LauncherText.text("Cloud saves", "云存档同步"))
                Spacer()
                Text(folder.isEmpty ? LauncherText.text("Off", "关闭") : (folder as NSString).lastPathComponent)
                    .foregroundStyle(.secondary).lineLimit(1).truncationMode(.middle)
            }
            Button(folder.isEmpty ? LauncherText.text("Choose a folder in iCloud Drive…", "选择 iCloud Drive 文件夹…") : LauncherText.text("Choose a different folder…", "选择其他文件夹…")) { choose() }
            if !folder.isEmpty {
                Button(LauncherText.text("Turn off cloud saves", "关闭云存档同步"), role: .destructive) {
                    dm_clear_cloud_saves(); refresh(); note = ""
                }
            }
        } footer: {
            Text(note.isEmpty
                 ? LauncherText.text("Choose a folder in iCloud Drive, and the same one in the Mac app's Cloud saves. The save comes in when the game starts and goes out after each save; a save that is replaced is kept in save-backups.", "请选择 iCloud Drive 文件夹，并在 Mac 上选择同一文件夹。启动时同步存档，保存后上传；被替换的存档保留在 save-backups。")
                 : note)
        }
        .onAppear(perform: refresh)
    }

    private func choose() {
        chooseFolder { result in
            if result > 0 {
                note = LauncherText.text("Saved. The game uses this folder when you press Play.", "已保存，开始游戏时使用此文件夹。")
                refresh()
            } else if result < 0 {
                note = LauncherText.text("That folder could not be remembered — try a different one.", "无法保存文件夹访问权限，请选择其他文件夹。")
            }
        }
    }

    private func refresh() {
        var stale: ObjCBool = false
        folder = dm_cloud_saves_path(&stale) ?? ""
        if stale.boolValue && folder.isEmpty {
            note = LauncherText.text("The folder you chose before is no longer readable (the app was signed differently). Please choose it again.", "之前的文件夹无法访问（应用签名可能已改变），请重新选择。")
        }
    }
}

