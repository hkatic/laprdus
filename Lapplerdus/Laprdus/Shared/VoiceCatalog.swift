// VoiceCatalog.swift - Voice and language lists sourced from the native engine.
//
// Every voice speaks every language: the language (Croatian, Serbian,
// Bosnian, Slovenian) is chosen separately from the voice, and a voice's languageCode
// is only its native language, spoken when nothing else was asked for.

import Foundation

struct Voice: Identifiable, Hashable {
    let id: String
    /// Name from the registry, shown as it is: "Josip", "Đedo", "Dalmacija Klapa".
    let displayName: String
    /// Native language ("hr-HR", "sr-RS", "bs-BA" or "sl-SI").
    let languageCode: String
    let gender: String
    let age: String
    let basePitch: Float
    let isPhysical: Bool
    /// Id of the voice this one is derived from; nil for josip, vlado,
    /// zvonko, stojan and mirsad.
    let baseVoiceID: String?
    /// Phoneme data file of a recorded voice; nil for the formant voices,
    /// which need none.
    let dataFilename: String?

    /// Identifier of the system voice for this voice in one language:
    /// "com.hrvojekatic.laprdus.zvonko.sr".
    func providerIdentifier(forLanguage language: Language) -> String {
        "\(VoiceCatalog.providerIdentifierPrefix)\(id).\(language.shortCode)"
    }

    /// Identifier used before 2.0, when a voice was one system voice in its
    /// native language: "com.hrvojekatic.laprdus.zvonko". Still accepted in
    /// requests from a host that kept the old identifier.
    var legacyProviderIdentifier: String { "\(VoiceCatalog.providerIdentifierPrefix)\(id)" }

    /// Synthesized by rule (Zvonko, Stojan, Mirsad and the singing presets):
    /// accepts the wider rate and pitch ranges.
    var isFormant: Bool { dataFilename == nil }

    /// Sings the text to a folk song instead of speaking it: a formant voice
    /// derived from another one.
    var isSinging: Bool { isFormant && baseVoiceID != nil }

    /// Words per minute at speed 1.0 and acceleration 1.0.
    var nominalWordsPerMinute: Float { laprdus_get_nominal_wpm(id) }

    /// Name shown in pickers: the registry's display name, the same on every
    /// platform.
    var localizedName: String { displayName }

    /// Secondary line: "Male, Adult, Formant voice" (localized). The language
    /// is not part of it: every voice speaks the language chosen in the
    /// Language setting.
    var localizedDetails: String {
        let localizedGender = gender == "Female"
            ? String(localized: "Female")
            : String(localized: "Male")
        let localizedAge: String
        switch age {
        case "Child": localizedAge = String(localized: "Child")
        case "Senior": localizedAge = String(localized: "Senior")
        default: localizedAge = String(localized: "Adult")
        }
        let kind: String
        if isSinging {
            kind = String(localized: "Singing voice")
        } else if isFormant {
            kind = String(localized: "Formant voice")
        } else {
            kind = String(localized: "Recorded voice")
        }
        return "\(localizedGender), \(localizedAge), \(kind)"
    }
}

/// One of the languages every voice speaks.
struct Language: Identifiable, Hashable {
    /// BCP-47 code: "hr-HR", "sr-RS", "bs-BA", "sl-SI". Stored in the settings.
    let code: String
    /// "hr", "sr", "bs", "sl": the suffix of a system voice identifier.
    let shortCode: String
    /// "Croatian", "Serbian", "Bosnian", "Slovenian".
    let englishName: String

    var id: String { code }

    /// Demo text follows the speech language, independently of the UI locale.
    var sampleText: String {
        switch shortCode {
        case "sl":
            return "Dober dan. Sem Laprdus, rojen sem bil leta 2026 in veseli me, da se lahko spoznava! 😁\nKako si? ❤\n"
        case "sr":
            return "Dobar dan. Ja sam Laprdus, rođen sam 2026. godine, i drago mi je da možemo da se upoznamo! 😁\nKako si ti? ❤\n"
        default:
            return "Dobar dan. Ja sam Laprdus, rođen sam 2026. godine, i drago mi je da se možemo upoznati! 😁\nKako si ti? ❤\n"
        }
    }

    /// Name in the app's language, from the string catalog.
    var localizedName: String {
        switch shortCode {
        case "sr": return String(localized: "Serbian")
        case "bs": return String(localized: "Bosnian")
        case "sl": return String(localized: "Slovenian")
        default: return String(localized: "Croatian")
        }
    }

    /// The languages in the engine's order: Croatian, Serbian, Bosnian, Slovenian.
    static let all: [Language] = {
        var languages: [Language] = []
        for index in 0..<laprdus_get_language_count() {
            var info = LaprdusLanguageInfo()
            guard laprdus_get_language_info(index, &info) == LAPRDUS_OK else { continue }
            languages.append(Language(
                code: info.code.map { String(cString: $0) } ?? "hr-HR",
                shortCode: info.short_code.map { String(cString: $0) } ?? "hr",
                englishName: info.english_name.map { String(cString: $0) } ?? "Croatian"
            ))
        }
        return languages
    }()

    /// Croatian: the language of a new installation when the device prefers
    /// none of the four.
    static var croatian: Language {
        all.first ?? Language(code: "hr-HR", shortCode: "hr", englishName: "Croatian")
    }

    /// The language a tag names: its code, its short code, or any tag that
    /// starts with the short code ("hr_HR", "sr-Latn-RS", "bs-Latn"), in
    /// any case. Nil for anything else.
    static func language(matching tag: String) -> Language? {
        let lowered = tag.lowercased().replacingOccurrences(of: "_", with: "-")
        if let exact = all.first(where: { $0.code.lowercased() == lowered }) { return exact }
        return all.first { language in
            let short = language.shortCode.lowercased()
            return lowered == short || lowered.hasPrefix(short + "-")
        }
    }

    /// Language of a new installation: the first of the user's preferred
    /// languages that Laprdus speaks, Croatian otherwise.
    static var defaultCode: String {
        for tag in Locale.preferredLanguages {
            if let language = language(matching: tag) { return language.code }
        }
        return croatian.code
    }
}

/// One system voice: a voice in one language.
struct ProviderVoice: Hashable {
    let voice: Voice
    let language: Language

    var identifier: String { voice.providerIdentifier(forLanguage: language) }
}

enum VoiceCatalog {
    static let providerIdentifierPrefix = "com.hrvojekatic.laprdus."

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
                isPhysical: info.base_voice_id == nil,
                baseVoiceID: info.base_voice_id.map { String(cString: $0) },
                dataFilename: info.data_filename.map { String(cString: $0) }
            ))
        }
        return voices
    }()

    /// The voice with this id, or with an id it was known by before 2.0.
    static func voice(withID id: String) -> Voice? {
        let canonical = canonicalVoiceID(id)
        return all.first { $0.id == canonical }
    }

    /// Ids renamed in 2.0. Saved settings and old system voice identifiers
    /// still carry the old ones.
    static let renamedVoiceIDs: [String: String] = [
        "djed": "djedo",
        "dijete": "detence",
        "baka": "baba",
        "klapa": "dalmacija_klapa",
        "pjevac": "lika_velebit",
        "pevac": "solunac",
        "solist": "sarajevo_bembasa",
    ]

    /// The current id of a voice: an old id is mapped to the renamed voice,
    /// any other id is returned unchanged.
    static func canonicalVoiceID(_ id: String) -> String {
        renamedVoiceIDs[id] ?? id
    }

    static func isFormantVoice(_ id: String) -> Bool { voice(withID: id)?.isFormant ?? false }
    static func isSingingVoice(_ id: String) -> Bool { voice(withID: id)?.isSinging ?? false }

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

    /// Voice of a new installation: Zvonko, for every language.
    static var defaultVoiceID: String {
        laprdus_get_default_voice().map { String(cString: $0) } ?? "zvonko"
    }

    /// Default voice for a language: the same for all of them. Kept for
    /// callers that choose a voice from a language tag.
    static func defaultVoiceID(forLanguage tag: String) -> String {
        defaultVoiceID
    }

    /// Language of a new installation, as a code ("hr-HR").
    static var defaultLanguageCode: String { Language.defaultCode }

    // MARK: System voices

    /// The system voices: every voice in every language, grouped by
    /// language (all voices in Croatian, then in Serbian, Bosnian and Slovenian).
    static let providerVoices: [ProviderVoice] = {
        Language.all.flatMap { language in
            all.map { ProviderVoice(voice: $0, language: language) }
        }
    }()

    /// The voice and language a system voice identifier names. A 2.0
    /// identifier carries both ("com.hrvojekatic.laprdus.zvonko.sr"); one
    /// from before 2.0 names the voice alone, which then speaks its native
    /// language; an old voice id in either is mapped to the renamed voice.
    /// Nil for an identifier that is not one of ours.
    static func providerVoice(forIdentifier identifier: String) -> ProviderVoice? {
        guard identifier.hasPrefix(providerIdentifierPrefix) else { return nil }
        let rest = identifier.dropFirst(providerIdentifierPrefix.count)
        if let dot = rest.lastIndex(of: "."),
           let language = Language.language(matching: String(rest[rest.index(after: dot)...])),
           let voice = voice(withID: String(rest[..<dot])) {
            return ProviderVoice(voice: voice, language: language)
        }
        if let voice = voice(withID: String(rest)),
           let language = Language.language(matching: voice.languageCode) {
            return ProviderVoice(voice: voice, language: language)
        }
        return nil
    }
}
