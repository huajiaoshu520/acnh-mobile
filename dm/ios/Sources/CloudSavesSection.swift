import SwiftUI
import UIKit
import UniformTypeIdentifiers
/// Cloud saves: an iCloud Drive folder shared with the Mac app (chosen in its settings window's
/// Cloud saves too). The host brings the save in from there when the game starts and
/// sends it there after each save (dm/host/save_sync.h); the folder is a bookmark (CloudSaves.m).
struct CloudSavesSection: View {
    @State private var folder: String = ""
    @State private var note: String = ""
    @State private var picking = false

    var body: some View {
        Section {
            HStack {
                Text(LauncherText.text("Cloud saves", "云存档同步"))
                Spacer()
                Text(folder.isEmpty ? LauncherText.text("Off", "关闭") : (folder as NSString).lastPathComponent)
                    .foregroundStyle(.secondary).lineLimit(1).truncationMode(.middle)
            }
            Button(folder.isEmpty ? LauncherText.text("Choose a folder in iCloud Drive…", "选择 iCloud Drive 文件夹…") : LauncherText.text("Choose a different folder…", "选择其他文件夹…")) { picking = true }
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
        .sheet(isPresented: $picking) {
            DocumentPicker(types: [.folder]) { url in
                picking = false
                if dm_store_cloud_saves(url) {
                    note = LauncherText.text("Saved. The game uses this folder when you press Play.", "已保存，开始游戏时使用此文件夹。")
                    refresh()
                } else {
                    note = LauncherText.text("That folder could not be remembered — try a different one.", "无法保存文件夹访问权限，请选择其他文件夹。")
                }
            } onCancel: { picking = false }
        }
        .onAppear(perform: refresh)
    }

    private func refresh() {
        var stale: ObjCBool = false
        folder = dm_cloud_saves_path(&stale) ?? ""
        if stale.boolValue && folder.isEmpty {
            note = LauncherText.text("The folder you chose before is no longer readable (the app was signed differently). Please choose it again.", "之前的文件夹无法访问（应用签名可能已改变），请重新选择。")
        }
    }
}

/// UIDocumentPickerViewController for a folder, in the two callbacks a SwiftUI sheet wants.
private struct DocumentPicker: UIViewControllerRepresentable {
    var types: [UTType] = [.folder]
    let onPick: (URL) -> Void
    let onCancel: () -> Void

    func makeUIViewController(context: Context) -> UIDocumentPickerViewController {
        // .open (not .import): the folder stays where it is and the bookmark keeps pointing at it.
        let picker = UIDocumentPickerViewController(forOpeningContentTypes: types, asCopy: false)
        picker.allowsMultipleSelection = false
        picker.delegate = context.coordinator
        return picker
    }

    func updateUIViewController(_ vc: UIDocumentPickerViewController, context: Context) {}

    func makeCoordinator() -> Coordinator { Coordinator(onPick: onPick, onCancel: onCancel) }

    final class Coordinator: NSObject, UIDocumentPickerDelegate {
        let onPick: (URL) -> Void
        let onCancel: () -> Void
        init(onPick: @escaping (URL) -> Void, onCancel: @escaping () -> Void) {
            self.onPick = onPick; self.onCancel = onCancel
        }
        func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) {
            if let url = urls.first { onPick(url) } else { onCancel() }
        }
        func documentPickerWasCancelled(_ controller: UIDocumentPickerViewController) { onCancel() }
    }
}

