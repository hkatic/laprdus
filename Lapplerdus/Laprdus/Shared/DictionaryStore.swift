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

    /// URL of the user pronunciation dictionary if it exists on disk.
    var userDictionaryURLIfPresent: URL? {
        let url = fileURL(for: .main)
        return FileManager.default.fileExists(atPath: url.path) ? url : nil
    }
}
