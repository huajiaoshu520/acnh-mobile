import SwiftUI

struct ReleaseCheckView: View {
    @State private var busy = false
    @State private var status = ""
    @State private var update: URL?
    private func text(_ english: String, _ chinese: String) -> String { LauncherText.text(english, chinese) }
    var body: some View {
        VStack(spacing: 4) {
            if busy {
                HStack(spacing: 6) {
                    ProgressView().controlSize(.small)
                    Text(text("Checking for updates…", "正在检测版本…"))
                }
            }
            if let update {
                Link(text("Update available", "有更新"), destination: update)
            } else if !status.isEmpty {
                Text(status).foregroundStyle(.secondary).multilineTextAlignment(.center)
            }
        }.font(.footnote).task { await check() }
    }
    @MainActor private func check() async {
        guard !busy else { return }
        busy = true; status = ""; update = nil
        defer { busy = false }
        do {
            // This index is advanced only after a formal GitHub Release is published.
            // main/config/version.json may describe a build not yet released.
            let url = URL(string: "https://cdn.jsdelivr.net/gh/huajiaoshu520/acnh-mobile@main/config/latest-release.json")!
            var request = URLRequest(url: url, cachePolicy: .reloadIgnoringLocalCacheData, timeoutInterval: 12)
            request.setValue("application/json", forHTTPHeaderField: "Accept")
            let (data, response) = try await URLSession.shared.data(for: request)
            guard (response as? HTTPURLResponse)?.statusCode == 200, data.count < 16384,
                  let local = ReleaseVersion(Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "") else { throw URLError(.badServerResponse) }
            let release = try JSONDecoder().decode(PublishedRelease.self, from: data)
            guard let latest = release.parsed else { throw URLError(.cannotParseResponse) }
            if local < latest { update = release.url }
            else { status = text("Already up to date", "已是最新版") }
        } catch {
            status = text("Update check failed.", "检测失败")
        }
    }
}
