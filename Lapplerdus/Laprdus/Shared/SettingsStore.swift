// SettingsStore.swift - Persisted settings, shared with the LaprdusVoices extension.
// Key names and default values are kept identical across all Laprdus platforms.

import Combine
import Foundation

enum SettingsKey {
    static let defaultVoice = "default_voice"
    static let language = "language"
    static let speed = "speed"
    static let pitch = "pitch"
    static let volume = "volume"
    static let forceSpeed = "force_speed"
    static let forcePitch = "force_pitch"
    static let forceVolume = "force_volume"
    static let volumeBoost = "volume_boost"
    static let emojiEnabled = "emoji_enabled"
    static let inflectionLevel = "inflection_level"
    static let acceleration = "acceleration"
    static let sentencePause = "sentence_pause"
    static let commaPause = "comma_pause"
    static let newlinePause = "newline_pause"
    static let numberMode = "number_mode"
    static let spellingSpeed = "spelling_speed"
    static let spellingMode = "spelling_mode"
    static let userDictionariesEnabled = "user_dictionaries_enabled"
}

/// Immutable snapshot of all settings, safe to pass across threads and used
/// by the speech extension (which has no UI and no observation needs).
struct SettingsSnapshot: Sendable {
    var defaultVoice = VoiceCatalog.defaultVoiceID
    /// Language every voice speaks in the app: "hr-HR", "sr-RS", "bs-BA" or "sl-SI".
    /// The system voices take their language from the voice the host picked.
    var language = VoiceCatalog.defaultLanguageCode
    var speed: Float = 1.0
    var pitch: Float = 1.0
    var volume: Float = 1.0
    var forceSpeed = false
    var forcePitch = false
    var forceVolume = false
    /// Louder speech through a peak limiter, for noisy places and the phone's earpiece.
    var volumeBoost = false
    var emojiEnabled = false
    /// Size of the pitch movements, 0 (monotone) to 1, 0.5 as measured.
    var inflectionLevel: Float = SettingsSnapshot.defaultInflectionLevel
    /// Multiplier on the speech rate, 0.5 to 3.0.
    var acceleration: Float = SettingsSnapshot.defaultAcceleration
    var sentencePause = 100
    var commaPause = 100
    var newlinePause = 100
    var numberMode = 0
    /// Spelling cadence, 0 to 100; lower values add longer pauses after characters.
    var spellingSpeed = SettingsSnapshot.defaultSpellingSpeed
    /// How letters are spelled: "sounds" (b, c, d; the default) or "names" (be, ce, de).
    var spellingMode = SettingsSnapshot.spellingModeSounds
    var userDictionariesEnabled = true

    static let defaultSpellingSpeed = 50
    static let spellingModeNames = "names"
    static let spellingModeSounds = "sounds"
    static let defaultInflectionLevel: Float = 0.5
    static let defaultAcceleration: Float = 1.0
    static let accelerationRange: ClosedRange<Float> = 0.5...3.0

    static func load(from defaults: UserDefaults = AppGroup.defaults) -> SettingsSnapshot {
        var snapshot = SettingsSnapshot()
        if let value = defaults.string(forKey: SettingsKey.defaultVoice) { snapshot.defaultVoice = value }
        if let value = defaults.string(forKey: SettingsKey.language) { snapshot.language = value }
        if defaults.object(forKey: SettingsKey.speed) != nil { snapshot.speed = defaults.float(forKey: SettingsKey.speed) }
        if defaults.object(forKey: SettingsKey.pitch) != nil { snapshot.pitch = defaults.float(forKey: SettingsKey.pitch) }
        if defaults.object(forKey: SettingsKey.volume) != nil { snapshot.volume = defaults.float(forKey: SettingsKey.volume) }
        snapshot.forceSpeed = defaults.bool(forKey: SettingsKey.forceSpeed)
        snapshot.forcePitch = defaults.bool(forKey: SettingsKey.forcePitch)
        snapshot.forceVolume = defaults.bool(forKey: SettingsKey.forceVolume)
        snapshot.volumeBoost = defaults.bool(forKey: SettingsKey.volumeBoost)
        snapshot.emojiEnabled = defaults.bool(forKey: SettingsKey.emojiEnabled)
        if defaults.object(forKey: SettingsKey.inflectionLevel) != nil {
            snapshot.inflectionLevel = defaults.float(forKey: SettingsKey.inflectionLevel)
        }
        if defaults.object(forKey: SettingsKey.acceleration) != nil {
            snapshot.acceleration = defaults.float(forKey: SettingsKey.acceleration)
        }
        if defaults.object(forKey: SettingsKey.sentencePause) != nil { snapshot.sentencePause = defaults.integer(forKey: SettingsKey.sentencePause) }
        if defaults.object(forKey: SettingsKey.commaPause) != nil { snapshot.commaPause = defaults.integer(forKey: SettingsKey.commaPause) }
        if defaults.object(forKey: SettingsKey.newlinePause) != nil { snapshot.newlinePause = defaults.integer(forKey: SettingsKey.newlinePause) }
        snapshot.numberMode = defaults.integer(forKey: SettingsKey.numberMode)
        if defaults.object(forKey: SettingsKey.spellingSpeed) != nil { snapshot.spellingSpeed = defaults.integer(forKey: SettingsKey.spellingSpeed) }
        if let value = defaults.string(forKey: SettingsKey.spellingMode) { snapshot.spellingMode = value }
        if defaults.object(forKey: SettingsKey.userDictionariesEnabled) != nil {
            snapshot.userDictionariesEnabled = defaults.bool(forKey: SettingsKey.userDictionariesEnabled)
        }
        snapshot.sanitize()
        return snapshot
    }

    /// Stored defaults are external input (the suite is shared and a plist can
    /// be corrupted); out-of-range or non-finite values would break the
    /// settings sliders even though the engine clamps its own parameters.
    private mutating func sanitize() {
        // A voice saved under its pre-2.0 id ("djed", "pjevac") is the
        // renamed voice, not a reason to fall back to the default.
        defaultVoice = VoiceCatalog.canonicalVoiceID(defaultVoice)
        if VoiceCatalog.voice(withID: defaultVoice) == nil {
            defaultVoice = VoiceCatalog.defaultVoiceID
        }
        language = Language.language(matching: language)?.code ?? VoiceCatalog.defaultLanguageCode
        // The recorded voices stop at 0.5x and 2.0x, the formant voices go
        // from 0.25x to 4.0x.
        let range = VoiceCatalog.speedPitchRange(forVoice: defaultVoice)
        speed = speed.clamped(to: range, fallback: 1.0)
        pitch = pitch.clamped(to: range, fallback: 1.0)
        volume = volume.clamped(to: 0.0...1.0, fallback: 1.0)
        inflectionLevel = inflectionLevel.clamped(to: 0.0...1.0, fallback: SettingsSnapshot.defaultInflectionLevel)
        acceleration = acceleration.clamped(to: SettingsSnapshot.accelerationRange, fallback: SettingsSnapshot.defaultAcceleration)
        sentencePause = min(max(sentencePause, 0), 2000)
        commaPause = min(max(commaPause, 0), 2000)
        newlinePause = min(max(newlinePause, 0), 2000)
        numberMode = numberMode == 1 ? 1 : 0
        spellingSpeed = min(max(spellingSpeed, 0), 100)
        if spellingMode != SettingsSnapshot.spellingModeSounds { spellingMode = SettingsSnapshot.spellingModeNames }
    }
}

private extension Float {
    func clamped(to range: ClosedRange<Float>, fallback: Float) -> Float {
        guard isFinite else { return fallback }
        return min(max(self, range.lowerBound), range.upperBound)
    }
}

/// Observable settings model used by the SwiftUI app. Every property change
/// is persisted immediately to the shared defaults suite.
final class SettingsStore: ObservableObject {
    private let defaults: UserDefaults

    @Published var defaultVoice: String { didSet { defaults.set(defaultVoice, forKey: SettingsKey.defaultVoice) } }
    @Published var language: String { didSet { defaults.set(language, forKey: SettingsKey.language) } }
    @Published var speed: Float { didSet { defaults.set(speed, forKey: SettingsKey.speed) } }
    @Published var pitch: Float { didSet { defaults.set(pitch, forKey: SettingsKey.pitch) } }
    @Published var volume: Float { didSet { defaults.set(volume, forKey: SettingsKey.volume) } }
    @Published var forceSpeed: Bool { didSet { defaults.set(forceSpeed, forKey: SettingsKey.forceSpeed) } }
    @Published var forcePitch: Bool { didSet { defaults.set(forcePitch, forKey: SettingsKey.forcePitch) } }
    @Published var forceVolume: Bool { didSet { defaults.set(forceVolume, forKey: SettingsKey.forceVolume) } }
    @Published var volumeBoost: Bool { didSet { defaults.set(volumeBoost, forKey: SettingsKey.volumeBoost) } }
    @Published var emojiEnabled: Bool { didSet { defaults.set(emojiEnabled, forKey: SettingsKey.emojiEnabled) } }
    @Published var inflectionLevel: Float { didSet { defaults.set(inflectionLevel, forKey: SettingsKey.inflectionLevel) } }
    @Published var acceleration: Float { didSet { defaults.set(acceleration, forKey: SettingsKey.acceleration) } }
    @Published var sentencePause: Int { didSet { defaults.set(sentencePause, forKey: SettingsKey.sentencePause) } }
    @Published var commaPause: Int { didSet { defaults.set(commaPause, forKey: SettingsKey.commaPause) } }
    @Published var newlinePause: Int { didSet { defaults.set(newlinePause, forKey: SettingsKey.newlinePause) } }
    @Published var numberMode: Int { didSet { defaults.set(numberMode, forKey: SettingsKey.numberMode) } }
    @Published var spellingSpeed: Int { didSet { defaults.set(spellingSpeed, forKey: SettingsKey.spellingSpeed) } }
    @Published var spellingMode: String { didSet { defaults.set(spellingMode, forKey: SettingsKey.spellingMode) } }
    @Published var userDictionariesEnabled: Bool { didSet { defaults.set(userDictionariesEnabled, forKey: SettingsKey.userDictionariesEnabled) } }

    init(defaults: UserDefaults = AppGroup.defaults) {
        self.defaults = defaults
        let snapshot = SettingsSnapshot.load(from: defaults)
        defaultVoice = snapshot.defaultVoice
        language = snapshot.language
        speed = snapshot.speed
        pitch = snapshot.pitch
        volume = snapshot.volume
        forceSpeed = snapshot.forceSpeed
        forcePitch = snapshot.forcePitch
        forceVolume = snapshot.forceVolume
        volumeBoost = snapshot.volumeBoost
        emojiEnabled = snapshot.emojiEnabled
        inflectionLevel = snapshot.inflectionLevel
        acceleration = snapshot.acceleration
        sentencePause = snapshot.sentencePause
        commaPause = snapshot.commaPause
        newlinePause = snapshot.newlinePause
        numberMode = snapshot.numberMode
        spellingSpeed = snapshot.spellingSpeed
        spellingMode = snapshot.spellingMode
        userDictionariesEnabled = snapshot.userDictionariesEnabled
    }

    var snapshot: SettingsSnapshot {
        SettingsSnapshot(
            defaultVoice: defaultVoice,
            language: language,
            speed: speed,
            pitch: pitch,
            volume: volume,
            forceSpeed: forceSpeed,
            forcePitch: forcePitch,
            forceVolume: forceVolume,
            volumeBoost: volumeBoost,
            emojiEnabled: emojiEnabled,
            inflectionLevel: inflectionLevel,
            acceleration: acceleration,
            sentencePause: sentencePause,
            commaPause: commaPause,
            newlinePause: newlinePause,
            numberMode: numberMode,
            spellingSpeed: spellingSpeed,
            spellingMode: spellingMode,
            userDictionariesEnabled: userDictionariesEnabled
        )
    }
}
