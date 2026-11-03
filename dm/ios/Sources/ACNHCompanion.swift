// Native Apple presentation of Eden Duo's live ACNH module outputs.
import SwiftUI
import Combine
import CoreGraphics
#if os(iOS)
import UIKit
#endif

func companionText(_ key: String) -> String {
    var bundle = Bundle.main
    let language = ProcessInfo.processInfo.environment["DM_LANG"] ?? (bundle.object(forInfoDictionaryKey: "DMLanguage") as? String ?? "")
    if language.hasPrefix("zh"), let path = bundle.path(forResource: "zh-Hans", ofType: "lproj"), let translated = Bundle(path: path) { bundle = translated }
    return bundle.localizedString(forKey: key, value: key, table: "Companion")
}
enum CompanionPage: Int, CaseIterable {
    case map = 1, bag, critters, diy, today, phone
    var title: String { companionText(["Map", "Pockets", "Critterpedia", "DIY", "Today", "NookPhone"][rawValue - 1]) }
    var symbol: String { ["map", "bag", "fish", "hammer", "sun.max", "apps.iphone"][rawValue - 1] }
}
struct CompanionPixels { let data: Data; let width: Int; let height: Int }
struct CompanionMarker: Identifiable {
    let id: String
    let key: String
    let x: Double
    let y: Double
    let box: Double
}
@MainActor final class CompanionModel: ObservableObject {
    typealias Reader = () -> [String: Any]
    typealias Sender = (String, Int64) -> Void
    typealias ImageLoader = (String, @escaping (Data?, Int, Int) -> Void) -> Void
    @Published var values: [String: Any] = [:]
    @Published var page = CompanionPage.map
    @Published var imageEpoch = 0
    @Published var islandName = ""
    @Published var controlsHidden = false
    @Published var externalController = false
    @Published private(set) var presented = true
    var uiActivity: ((Int, Bool) -> Void)?
    private var nativeActivity = false
    private let read: Reader
    private let send: Sender
    let load: ImageLoader
    private var timer: Timer?
    private let native: DMCompanion?
    init(reader: Reader? = nil, sender: Sender? = nil, images: ImageLoader? = nil) {
        let engine = reader == nil ? DMCompanion.shared() : nil
        native = engine
        read = reader ?? { engine?.snapshot as? [String: Any] ?? [:] }
        send = sender ?? { name, value in engine?.action(name, argument: value) }
        load = images ?? { key, completion in
            guard let engine else { completion(nil, 0, 0); return }
            engine.image(key) { data, w, h in completion(data, Int(w), Int(h)) }
        }
        engine?.start()
        poll()
        schedulePolling()
    }
    private func schedulePolling() {
        timer?.invalidate();timer=nil
        guard presented else { return }
        let interval:TimeInterval = [.map,.bag,.phone].contains(page) ? 0.2 : 0.5
        timer = Timer.scheduledTimer(withTimeInterval: interval, repeats: true) { [weak self] _ in Task { @MainActor in self?.poll() } }
        if let timer { RunLoop.main.add(timer, forMode: .common) }
    }
    deinit { timer?.invalidate() }
    func poll() {
        guard presented else { return }
        var next = read()
        next["bag.can_write"] = 0; next["bag.can_use"] = 0
        if !(next as NSDictionary).isEqual(to: values) { values = next }
        let name=text("island.name")
        if !name.isEmpty && name != islandName { islandName = name }
        updateNativeActivity()
    }
    func visible(_ value: Bool) { if !value && page == .bag { action("bag_cancel", 0) }; presented=value;native?.setVisible(value);schedulePolling();if value {poll()} }
    func setPresentation(controlsHidden: Bool, physicalController: Bool, presented: Bool) {
        if self.controlsHidden != controlsHidden { self.controlsHidden = controlsHidden }
        if externalController != physicalController { externalController = physicalController }
        // The root controller reports presentation independently of SwiftUI appearance.
        // Route transitions through the owner of polling and native visibility.
        if self.presented != presented { visible(presented) }
    }
    func select(_ next: CompanionPage) { if page == .bag && next != .bag { action("bag_cancel", 0) }; page = next; updateNativeActivity(); action("ui_open", next.rawValue);schedulePolling();poll() }
    func updateNativeActivity(force: Bool = false) {
        let busy = page == .bag && (number("bag.native.open") != 0 || number("bag.busy") != 0)
            || page == .phone && (values["pdrv.idle"] as? NSNumber)?.intValue == 0
        if force || busy != nativeActivity {nativeActivity=busy;uiActivity?(4,busy)}
    }
    func action(_ name: String, _ value: Int) { send(name, Int64(value)) }
    func pocketToken(_ slot: Int) -> String { text("bag.\(slot).token") }
    private func validToken(_ token: String) -> Bool { token.utf8.count == 16 && token.utf8.allSatisfy { (48...57).contains($0) || (97...102).contains($0) } }
    // This presentation is read-only, even if a future module advertises write support.
    func movePocket(from: Int, to: Int, expected: String) -> Bool { false }
    func usePocket(_ slot: Int) {}
    func pocketCommand(_ index: Int) {}
    func text(_ key: String, _ fallback: String = "") -> String {
        let value = values[key] as? String ?? fallback
        // The native text sits beside decoded app/item icons. Nintendo's private icon
        // codepoints have no glyph in the system font and must not become empty squares.
        return String(String.UnicodeScalarView(value.unicodeScalars.filter { $0.properties.generalCategory != .privateUse }))
    }
    func number(_ key: String) -> Int { (values[key] as? NSNumber)?.intValue ?? 0 }
    func decimal(_ key: String) -> Double { (values[key] as? NSNumber)?.doubleValue ?? 0 }
    func count(_ key: String, max limit: Int = 1000) -> Int { min(max(0, number(key)), limit) }
    var ready: Bool { number("acnh.ready") != 0 }
    var turnipPrice: Int {
        guard let slot = values["kabu.now"] as? NSNumber else { return 0 }
        if slot.intValue < 0 { return max(0, number("kabu.sun")) }
        return slot.intValue < 12 ? max(0, number("kabu.\(slot.intValue)")) : 0
    }
    func questMiles(_ index: Int) -> Int {
        let prefix = "nmp.\(index)."
        let multiplier = number(prefix + "x5") != 0 ? 5 : text(prefix + "card").contains("&bonus=1") ? 2 : 1
        return number(prefix + "miles") * multiplier
    }
    var markers: [CompanionMarker] {
        values.keys.filter { $0.hasPrefix("map.ib.") && $0.hasSuffix(".k0") }.sorted().compactMap { key in
            let stem = String(key.dropLast(2))
            guard let image = values[key] as? String, !image.isEmpty else { return nil }
            let parts = key.split(separator: ".")
            return CompanionMarker(id: key, key: image, x: decimal(stem + "x") / 10, y: decimal(stem + "y") / 10, box: parts.count > 2 ? Double(parts[2]) ?? 64 : 64)
        }
    }
}

struct ACNHCompanionView: View {
    @ObservedObject var model: CompanionModel
    var body: some View { EdenCompanionView(model: model) }
}

#if os(iOS)
@MainActor private weak var activeCompanionModel: CompanionModel?
@MainActor @_cdecl("dm_companion_ui_state")
public func dmCompanionUIState(_ hidden: Int32, _ physical: Int32, _ presented: Int32) {
    guard let model=activeCompanionModel else{return}
    model.setPresentation(controlsHidden: hidden != 0, physicalController: physical != 0, presented: presented != 0)
}
@MainActor
@_cdecl("dm_companion_view_controller")
public func dmCompanionViewController() -> UnsafeMutableRawPointer {
    let model=CompanionModel();activeCompanionModel=model
    model.uiActivity={dm_companion_ui_activity(Int32($0),$1 ? 1:0)}
    model.updateNativeActivity(force:true)
    let controller = UIHostingController(rootView: ACNHCompanionView(model: model))
    controller.view.clipsToBounds = true
    return Unmanaged.passRetained(controller).toOpaque()
}
#endif
