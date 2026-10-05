// DictionaryStore.swift - User dictionary persistence.
// The JSON file format is identical across all Laprdus platforms, so
// dictionary files are interchangeable between them.

import Foundation

struct DictionaryEntry: Identifiable, Hashable {
    var id = UUID()
    var grapheme = ""
    var phoneme = ""
    var caseSensitive = false
    var wholeWord = true
    var comment = ""
}

enum DictionaryType: String, CaseIterable, Identifiable {
    case main
    case spelling
    case emoji

    var id: String { rawValue }

    /// File names are shared across all Laprdus platforms.
    var fileName: String {
        switch self {
        case .main: return "user.json"
        case .spelling: return "spelling.json"
        case .emoji: return "emoji.json"
        }
    }
}

final class DictionaryStore: @unchecked Sendable {
    private let directory: URL
    private let queue = DispatchQueue(label: "com.hrvojekatic.laprdus.dictionaries")

    init(directory: URL = AppGroup.dictionariesDirectory) {
        self.directory = directory
    }

    func fileURL(for type: DictionaryType) -> URL {
        directory.appendingPathComponent(type.fileName)
    }

    /// A missing file yields an empty list.
    func load(_ type: DictionaryType) throws -> [DictionaryEntry] {
        try queue.sync {
            let url = fileURL(for: type)
            guard FileManager.default.fileExists(atPath: url.path) else { return [] }
            let data = try Data(contentsOf: url)
            guard let root = try JSONSerialization.jsonObject(with: data) as? [String: Any],
                  let entries = root["entries"] as? [[String: Any]] else {
                return []
            }
            return entries.compactMap { raw in
                guard let grapheme = raw["grapheme"] as? String, !grapheme.isEmpty else { return nil }
                return DictionaryEntry(
                    grapheme: grapheme,
                    phoneme: raw["phoneme"] as? String ?? "",
                    caseSensitive: raw["caseSensitive"] as? Bool ?? false,
                    wholeWord: raw["wholeWord"] as? Bool ?? true,
                    comment: raw["comment"] as? String ?? ""
                )
            }
        }
    }

    func save(_ entries: [DictionaryEntry], type: DictionaryType) throws {
        try queue.sync {
            var serialized: [[String: Any]] = []
            for entry in entries {
                var raw: [String: Any] = [
                    "grapheme": entry.grapheme,
                    "phoneme": entry.phoneme,
                    "caseSensitive": entry.caseSensitive,
                    "wholeWord": entry.wholeWord,
                ]
                if !entry.comment.isEmpty {
                    raw["comment"] = entry.comment
                }
                serialized.append(raw)
            }
            let root: [String: Any] = ["version": "1.0", "entries": serialized]
            let data = try JSONSerialization.data(withJSONObject: root, options: [.prettyPrinted, .sortedKeys])
            try data.write(to: fileURL(for: type), options: .atomic)
        }
    }

    /// Current state of the user dictionaries, used to detect edits made in
    /// the app while the engine (in particular the long-lived speech
    /// extension) already has an older copy loaded.
    func dictionaryState(userDictionariesEnabled: Bool) -> DictionaryState {
        guard userDictionariesEnabled else { return .bundledOnly }
        var urls: [DictionaryType: URL] = [:]
        var stamps: [String] = []
        for type in DictionaryType.allCases {
            let url = fileURL(for: type)
            guard let attributes = try? FileManager.default.attributesOfItem(atPath: url.path) else {
                continue
            }
            let modified = (attributes[.modificationDate] as? Date)?.timeIntervalSince1970 ?? 0
            let size = (attributes[.size] as? Int) ?? 0
            urls[type] = url
            stamps.append("\(type.rawValue)-\(modified)-\(size)")
        }
        return DictionaryState(urls: urls, stamp: urls.isEmpty ? "absent" : stamps.joined(separator: ","))
    }
}

/// The user dictionaries as the engine should see them, plus a stamp that
/// changes whenever one of the files does. Comparing stamps is what lets the
/// engine reload dictionaries only when they actually changed.
struct DictionaryState: Equatable, Sendable {
    /// The user dictionary files that exist on disk.
    let urls: [DictionaryType: URL]
    let stamp: String

    var userDictionaryURL: URL? { urls[.main] }

    /// Bundled dictionaries only, with no user dictionary layered on top.
    static let bundledOnly = DictionaryState(urls: [:], stamp: "disabled")
}
