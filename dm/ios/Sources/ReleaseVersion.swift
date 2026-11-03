import Foundation

// Compare numeric components, never lexicographic strings (0.3.9 < 0.3.10).
struct ReleaseVersion: Comparable {
    let parts: [Int]
    init?(_ value: String) {
        let text = value.hasPrefix("v") ? String(value.dropFirst()) : value
        let fields = text.split(separator: ".", omittingEmptySubsequences: false)
        guard (2...4).contains(fields.count), fields.allSatisfy({ !$0.isEmpty && $0.allSatisfy(\.isNumber) }) else { return nil }
        let numbers = fields.compactMap { Int($0) }
        guard numbers.count == fields.count else { return nil }
        parts = numbers + Array(repeating: 0, count: 4 - numbers.count)
    }
    static func < (lhs: Self, rhs: Self) -> Bool { lhs.parts.lexicographicallyPrecedes(rhs.parts) }
}
struct PublishedRelease: Decodable {
    let version: String
    let url: URL
    var parsed: ReleaseVersion? {
        guard url.scheme == "https", url.host == "github.com",
              url.path == "/huajiaoshu520/acnh-mobile/releases/tag/v\(version)" else { return nil }
        return ReleaseVersion(version)
    }
}
