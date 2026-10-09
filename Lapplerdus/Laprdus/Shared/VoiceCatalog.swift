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

    /// Synthesized by rule (Zvonko, Stojan, Mirsad and the singing presets):
    /// accepts the wider rate and pitch ranges and has the inflection level
    /// and acceleration settings.
    var isFormant: Bool { VoiceCatalog.isFormantVoice(id) }

    /// Sings the text to a folk song instead of speaking it.
    var isSinging: Bool { VoiceCatalog.isSingingVoice(id) }

    /// Words per minute at speed 1.0 and acceleration 1.0.
    var nominalWordsPerMinute: Float { laprdus_get_nominal_wpm(id) }

    /// Short localized name shown in pickers.
    var localizedName: String {
        switch id {
        case "josip": return "Josip"
        case "vlado": return "Vlado"
        case "detence": return "Detence"
        case "baba": return "Baba"
        case "djed": return "Đedo"
        case "zvonko": return "Zvonko"
        case "stojan": return "Stojan"
        case "mirsad": return "Mirsad"
        case "orguljas": return "Zvonko Orguljaš"
        case "klapa": return "Klapa Zvonko"
        case "trubac": return "Stojan Trubač"
        case "harmonikas": return "Stojan Harmonikaš"
        case "sevdalija": return "Mirsad Sevdalija"
        case "sazlija": return "Mirsad Sazlija"
        case "pjevac": return "Zvonko Pjevač"
        case "pevac": return "Stojan Pevač"
        case "solist": return "Mirsad Solist"
        case "becarac": return "Zvonko Bećarac"
        default: return displayName
        }
    }

    /// Secondary line: "Croatian - Male, Adult" (localized).
    var localizedDetails: String {
        let language: String
        if languageCode.hasPrefix("sr") {
            language = String(localized: "Serbian")
        } else if languageCode.hasPrefix("bs") {
            language = String(localized: "Bosnian")
        } else {
            language = String(localized: "Croatian")
        }
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

    static let singingVoiceIDs: Set<String> = [
        "orguljas", "klapa", "trubac", "harmonikas", "sevdalija", "sazlija",
        "pjevac", "pevac", "solist", "becarac"
    ]
    static let formantVoiceIDs: Set<String> =
        Set(["zvonko", "stojan", "mirsad"]).union(singingVoiceIDs)

    static func isFormantVoice(_ id: String) -> Bool { formantVoiceIDs.contains(id) }
    static func isSingingVoice(_ id: String) -> Bool { singingVoiceIDs.contains(id) }

    /// Speed and pitch range of the recorded voices.
    static let recordedSpeedPitchRange: ClosedRange<Float> = 0.5...2.0
    /// Speed and pitch range of the formant voices.
    static let formantSpeedPitchRange: ClosedRange<Float> = 0.25...4.0

    /// Speed and pitch range of a voice.
    static func speedPitchRange(forVoice id: String) -> ClosedRange<Float> {
        isFormantVoice(id) ? formantSpeedPitchRange : recordedSpeedPitchRange
    }

    /// Rate or pitch of a request from the system (VoiceOver, Spoken
    /// Content): the host asks relative to its normal (1.0), and the user's
    /// own Laprdus setting is that normal, so the two multiply. A forced
    /// setting ignores the host. The result stays in the voice's range.
    static func requestValue(setting: Float, requested: Float, forced: Bool, voice id: String) -> Float {
        let value = forced ? setting : setting * requested
        guard value.isFinite else { return 1.0 }
        let range = speedPitchRange(forVoice: id)
        return min(max(value, range.lowerBound), range.upperBound)
    }

    /// Default voice for a BCP-47 language tag: the formant voice of that
    /// language, Croatian for anything else.
    static func defaultVoiceID(forLanguage tag: String) -> String {
        let lowered = tag.lowercased()
        if lowered.hasPrefix("sr") { return "stojan" }
        if lowered.hasPrefix("bs") || lowered.hasPrefix("bos") { return "mirsad" }
        return "zvonko"
    }

    /// Voice of a new installation: the one matching the first of the user's
    /// preferred languages that Laprdus speaks.
    static var defaultVoiceID: String {
        let spoken = Locale.preferredLanguages.first { tag in
            let lowered = tag.lowercased()
            return ["hr", "sr", "bs"].contains { lowered.hasPrefix($0) }
        }
        return defaultVoiceID(forLanguage: spoken ?? "hr")
    }
}
