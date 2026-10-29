// Launcher: the settings screen shown before the game starts (front_ios.m dm_show_launcher). Its settings come from the app's dm/launcher.json, which build_ipa.sh writes for this game
// from dm/games/settings.tsv (dm/games/settings.py): each sets an environment variable the host reads when the game starts. A variable an env file set (Documents/env.txt, the bundle's
// dm/env.txt; main.m lists them in DM_ENV_FILE_KEYS) is shown but kept. The choices are kept in UserDefaults. DM_LAUNCHER=0 in Documents/env.txt starts the game at once, as an app
// without launcher.json does
import SwiftUI
import UIKit
import os

@_silgen_name("dm_edit_pad_layout")
private func dmEditPadLayout(_ parent: UnsafeMutableRawPointer, _ base: UnsafePointer<CChar>)

private func info(_ key: String) -> Any? { Bundle.main.object(forInfoDictionaryKey: key) }
private func localized(_ english: String, _ chinese: String) -> String {
    LauncherText.text(english, chinese)
}
private func label(_ s: Setting) -> String {
    LauncherText.label(env: s.env, fallback: s.label)
}
private func optionLabel(_ s: Setting, _ option: [String]) -> String {
    LauncherText.option(env: s.env, value: option[0], fallback: option[1])
}

private extension LauncherModel {
    func binding(_ s: Setting) -> Binding<String> { Binding(get: { self.values[s.env] ?? s.def }, set: { self.values[s.env] = $0 }) }
}

private struct SettingRow: View {
    let s: Setting
    @ObservedObject var m: LauncherModel
    let edit: (String) -> Void

    private var locked: Bool { m.fromFile.contains(s.env) || s.available == false }
    private var note: String {
        if s.available == false { return localized("This game controls its own frame timing; this option is not applicable.", "动森自行控制帧率，当前不适用此选项。") }
        let translated = LauncherText.note(env: s.env, fallback: s.note)
        return locked ? (translated.isEmpty ? "" : translated + " · ") + localized("Set by an env file", "由配置文件设置") : translated
    }
    private var rangeValue: Binding<Double> {
        Binding(get: { Double(m.values[s.env] ?? s.def) ?? s.max ?? 1 },
                set: { m.values[s.env] = String(format: "%.2f", $0) })
    }
    private var percentage: String { "\(Int((rangeValue.wrappedValue * 100).rounded())) %" }

    private var settingLabel: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(label(s))
            if !note.isEmpty { Text(note).font(.footnote).foregroundStyle(.secondary) }
        }
    }

    var body: some View {
        Group {
            switch s.kind {
            case "choice" where s.env == "ACNH_DISPLAY_MODE":
                VStack(alignment: .leading, spacing: 4) {
                    HStack {
                        Text(label(s))
                        Spacer()
                        Picker(selection: m.binding(s)) {
                            ForEach(s.options ?? [], id: \.[0]) { Text(optionLabel(s, $0)).tag($0[0]) }
                        } label: {
                            Text(label(s))
                        }
                        .pickerStyle(.menu)
                        .labelsHidden()
                        .fixedSize(horizontal: true, vertical: false)
                    }
                    if !note.isEmpty { Text(note).font(.footnote).foregroundStyle(.secondary) }
                }
            case "choice":
                Picker(selection: m.binding(s)) {
                    ForEach(s.options ?? [], id: \.[0]) { Text(optionLabel(s, $0)).tag($0[0]) }
                } label: {
                    settingLabel
                }
                .pickerStyle(.menu)
            case "range":
                VStack(alignment: .leading, spacing: 8) {
                    HStack {
                        settingLabel
                        Spacer()
                        Text(percentage).monospacedDigit().foregroundStyle(.secondary)
                    }
                    Slider(value: rangeValue, in: (s.min ?? 0)...(s.max ?? 1), step: s.step ?? 0.05)
                        .accessibilityLabel(s.label)
                        .accessibilityValue(percentage)
                }
            default:
                Toggle(isOn: Binding(get: { (m.values[s.env] ?? s.def) == s.onValue }, set: { m.values[s.env] = $0 ? s.onValue : s.offValue })) {
                    settingLabel
                }
            }
            if s.env == "ACNH_CUSTOM_BASE" {
                Button(localized("Adjust positions and sizes", "调整位置与大小")) { edit(m.values[s.env] ?? "acnh") }
            }
        }
        .disabled(locked)
    }
}

private struct LauncherView: View {
    @ObservedObject var m: LauncherModel
    let edit: (String) -> Void
    let chooseCloudFolder: (@escaping (Int) -> Void) -> Void
    let play: () -> Void
    @State private var memory = os_proc_available_memory()


    var body: some View {
        GeometryReader { geometry in
            HStack(spacing: 0) {
                VStack(spacing: 16) {
                    if let icon = appIcon() {
                        Image(uiImage: icon).resizable().frame(width: 96, height: 96)
                            .clipShape(RoundedRectangle(cornerRadius: 22)).accessibilityHidden(true)
                    }
                    Text((info("CFBundleDisplayName") as? String) ?? localized("Game", "游戏"))
                        .font(.title.weight(.bold)).multilineTextAlignment(.center)
                    Text("\(localized("Version", "版本")) \((info("CFBundleShortVersionString") as? String) ?? "") · \((info("CFBundleVersion") as? String) ?? "")")
                        .font(.callout).foregroundStyle(.secondary)
                    playButton
                }
                .padding(16)
                .frame(width: geometry.size.width / 3, height: geometry.size.height)
                NavigationStack {
                    List {
                        ForEach(m.sections, id: \.self) { section in
                            Section(LauncherText.section(section)) {
                                ForEach(m.settings.filter { $0.sectionTitle == section && ($0.env != "ACNH_CUSTOM_BASE" || m.values["ACNH_CONTROLLER_LAYOUT"] == "custom") }) { setting in
                                    SettingRow(s: setting, m: m, edit: edit)
                                }
                            }
                        }
                        CloudSavesSection(chooseFolder: chooseCloudFolder)
                        Section {
                            Button(localized("Reset to tested defaults", "恢复默认设置")) { m.reset() }
                                .disabled(!m.changed)
                        } footer: {
                            Text(localized("Brings back all settings this game was tested with.", "恢复此游戏预设的默认设置；由配置文件锁定的选项保持不变。"))
                        }
                        Section {
                            HStack {
                                Text(localized("Memory available", "可用内存"))
                                Spacer()
                                Text(ByteCountFormatter.string(fromByteCount: Int64(memory), countStyle: .memory)).monospacedDigit().foregroundStyle(.secondary)
                            }
                        }
                    }
                    .listStyle(.insetGrouped)
                    .contentMargins(.horizontal, 12, for: .scrollContent)
                    .navigationBarTitleDisplayMode(.inline)
                    .toolbar(.hidden, for: .navigationBar)
                }
                .frame(width: geometry.size.width * 2 / 3)
            }
        }
        .preferredColorScheme(.dark)
        .task {
            while !Task.isCancelled {
                memory = os_proc_available_memory()
                try? await Task.sleep(for: .seconds(2))
            }
        }
    }

    @ViewBuilder var playButton: some View {
        let button = Button(action: play) { Text(localized("Play", "开始游戏")).font(.title3.weight(.semibold)).frame(maxWidth: 220) }.controlSize(.large)
        if #available(iOS 26.0, *) { button.buttonStyle(.glassProminent) } else { button.buttonStyle(.borderedProminent) }
    }

    private func appIcon() -> UIImage? {
        if let i = UIImage(named: "AppIcon") { return i }
        guard let icons = info("CFBundleIcons") as? [String: Any], let primary = icons["CFBundlePrimaryIcon"] as? [String: Any],
              let files = primary["CFBundleIconFiles"] as? [String], let last = files.last else { return nil }
        return UIImage(named: last)
    }
}

// Keep the settings covered while the initial system orientation transition settles.
// This does not disable SpringBoard's animation or change subsequent device rotation.
private final class LandscapeLauncherController: UIHostingController<LauncherView> {
    private var startupCover: UIView?
    private var settleLink: CADisplayLink?
    private var appearedAt: CFTimeInterval = 0
    private var stableSince: CFTimeInterval?
    private var transitioning = false
    private var revealed = false

    override var supportedInterfaceOrientations: UIInterfaceOrientationMask { .landscape }
    override var preferredInterfaceOrientationForPresentation: UIInterfaceOrientation {
        let current = presentingViewController?.view.window?.windowScene?.interfaceOrientation
        return current?.isLandscape == true ? current! : .landscapeRight
    }
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        guard !revealed, startupCover == nil else { return }
        let cover = UIView(frame: view.bounds)
        cover.backgroundColor = .black
        cover.autoresizingMask = [.flexibleWidth, .flexibleHeight]
        view.addSubview(cover)
        startupCover = cover
    }
    override func viewDidAppear(_ animated: Bool) {
        super.viewDidAppear(animated)
        guard !revealed, settleLink == nil else { return }
        appearedAt = CACurrentMediaTime()
        let link = CADisplayLink(target: self, selector: #selector(checkStartupOrientation))
        settleLink = link
        link.add(to: .main, forMode: .common)
    }
    override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
        super.viewWillTransition(to: size, with: coordinator)
        guard !revealed else { return }
        transitioning = true
        stableSince = nil
        coordinator.animate(alongsideTransition: nil) { [weak self] _ in
            self?.transitioning = false
            self?.stableSince = nil
        }
    }
    @objc private func checkStartupOrientation() {
        let now = CACurrentMediaTime()
        // The inner display does not report orientation like the outer display.
        let landscape = view.bounds.width > view.bounds.height
        let ready = UIApplication.shared.applicationState == .active && landscape && !transitioning
        if ready { if stableSince == nil { stableSince = now } } else { stableSince = nil }
        let settled = stableSince.map { now - $0 >= 0.18 } ?? false
        // Bounded fallback: an unusual container lifecycle must not leave a permanent black screen.
        guard (settled && now - appearedAt >= 0.4) || now - appearedAt >= 2 else { return }
        revealed = true
        settleLink?.invalidate()
        settleLink = nil
        UIView.performWithoutAnimation { startupCover?.removeFromSuperview() }
        startupCover = nil
        fputs("launcher: startup cover removed (landscape=\(landscape), settled=\(settled))\n", stderr)
    }
    override func viewDidDisappear(_ animated: Bool) {
        super.viewDidDisappear(animated)
        settleLink?.invalidate()
        settleLink = nil
    }
}

// Shows the launcher over the root view controller and calls done once the settings are in the environment; false (done is not called) when there is no launcher
@_cdecl("dm_show_launcher")
public func dmShowLauncher(_ viewController: UnsafeMutableRawPointer, _ done: @convention(c) () -> Void) -> Bool {
    if ProcessInfo.processInfo.environment["DM_LAUNCHER"] == "0" { fputs("launcher: off (DM_LAUNCHER=0)\n", stderr); return false }
    guard let url = Bundle.main.resourceURL?.appendingPathComponent("dm/launcher.json"), let data = try? Data(contentsOf: url) else { return false }
    guard let file = try? JSONDecoder().decode(SettingsFile.self, from: data), !file.settings.isEmpty else { fputs("[RED] launcher: dm/launcher.json cannot be read; starting the game\n", stderr); return false }
    let parent = Unmanaged<UIViewController>.fromOpaque(viewController).takeUnretainedValue()
    let model = LauncherModel(file.settings)
    var host: UIViewController?
    let view = LauncherView(m: model, edit: { base in
        guard let h = host else { return }
        base.withCString { dmEditPadLayout(Unmanaged.passUnretained(h).toOpaque(), $0) }
    }, chooseCloudFolder: { callback in
        guard let h = host else { callback(-1); return }
        dm_choose_cloud_saves(h, callback)
    }) {
        guard let h = host else { return }
        host = nil
        model.apply()
        h.dismiss(animated: false) { done() }
    }
    let h = LandscapeLauncherController(rootView: view)
    h.modalPresentationStyle = .fullScreen
    host = h
    parent.present(h, animated: false)
    fputs("launcher: \(file.settings.count) settings shown\n", stderr)
    return true
}
