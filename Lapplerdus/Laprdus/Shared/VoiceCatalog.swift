// VoiceCatalog.swift - Voice list sourced from the native voice registry.

import Foundation

struct Voice: Identifiable, Hashable {
    let id: String
    let displayName: String
    let languageCode: String
    let gender: String
    let age: String
    let basePitch: Float
    let isPhysical: Bool

    /// Stable identifier used for AVSpeechSynthesisProviderVoice.
    var providerIdentifier: String { "com.hrvojekatic.laprdus.\(id)" }

    /// Short localized name shown in pickers (mirrors the Android app).
    var localizedName: String {
        switch id {
        case "josip": return "Josip"
        case "vlado": return "Vlado"
        case "detence": return String(localized: "Dijete")
        case "baba": return String(localized: "Baka")
        case "djed": return String(localized: "Đedo")
        default: return displayName
        }
    }

    /// Secondary line: "Croatian - Male, Adult" (localized).
    var localizedDetails: String {
        let language = languageCode.hasPrefix("sr")
            ? String(localized: "Serbian")
            : String(localized: "Croatian")
        let localizedGender = gender == "Female"
            ? String(localized: "Female")
            : String(localized: "Male")
        let localizedAge: String
        switch age {
        case "Child": localizedAge = String(localized: "Child")
        case "Senior": localizedAge = String(localized: "Senior")
        default: localizedAge = String(localized: "Adult")
        }
        return "\(language) - \(localizedGender), \(localizedAge)"
    }
}

enum VoiceCatalog {
    /// All voices exposed by the native registry, in registry order.
    static let all: [Voice] = {
        var voices: [Voice] = []
        for index in 0..<laprdus_get_voice_count() {
            var info = LaprdusVoiceInfo()
            guard laprdus_get_voice_info(index, &info) == LAPRDUS_OK else { continue }
            voices.append(Voice(
                id: info.id.map { String(cString: $0) } ?? "",
                displayName: info.display_name.map { String(cString: $0) } ?? "",
                languageCode: info.language_code.map { String(cString: $0) } ?? "hr-HR",
                gender: info.gender.map { String(cString: $0) } ?? "Male",
                age: info.age.map { String(cString: $0) } ?? "Adult",
                basePitch: info.base_pitch,
                isPhysical: info.base_voice_id == nil
            ))
        }
        return voices
    }()

    static func voice(withID id: String) -> Voice? {
        all.first { $0.id == id }
    }

    /// Default voice for a BCP-47 language tag (mirrors the Android service).
    static func defaultVoiceID(forLanguage tag: String) -> String {
        let lowered = tag.lowercased()
        if lowered.hasPrefix("sr") || lowered.hasPrefix("srp") { return "vlado" }
        return "josip"
    }
}
