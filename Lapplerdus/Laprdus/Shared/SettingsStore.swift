// SettingsStore.swift - Persisted settings, shared with the LaprdusVoices extension.
// Keys and defaults intentionally mirror the Android app's DataStore.

import Foundation
import Observation

enum SettingsKey {
    static let defaultVoice = "default_voice"
    static let speed = "speed"
    static let pitch = "pitch"
    static let volume = "volume"
    static let forceSpeed = "force_speed"
    static let forcePitch = "force_pitch"
    static let forceVolume = "force_volume"
    static let forceLanguage = "force_language"
    static let emojiEnabled = "emoji_enabled"
    static let inflectionEnabled = "inflection_enabled"
    static let sentencePause = "sentence_pause"
    static let commaPause = "comma_pause"
    static let newlinePause = "newline_pause"
    static let numberMode = "number_mode"
    static let userDictionariesEnabled = "user_dictionaries_enabled"
}

/// Immutable snapshot of all settings, safe to pass across threads and used
/// by the speech extension (which has no UI and no observation needs).
struct SettingsSnapshot: Sendable {
    var defaultVoice = "josip"
    var speed: Float = 1.0
    var pitch: Float = 1.0
    var volume: Float = 1.0
    var forceSpeed = false
    var forcePitch = false
    var forceVolume = false
    var forceLanguage = false
    var emojiEnabled = false
    var inflectionEnabled = true
    var sentencePause = 100
    var commaPause = 100
    var newlinePause = 100
    var numberMode = 0
    var userDictionariesEnabled = true

    static func load(from defaults: UserDefaults = AppGroup.defaults) -> SettingsSnapshot {
        var snapshot = SettingsSnapshot()
        if let value = defaults.string(forKey: SettingsKey.defaultVoice) { snapshot.defaultVoice = value }
        if defaults.object(forKey: SettingsKey.speed) != nil { snapshot.speed = defaults.float(forKey: SettingsKey.speed) }
        if defaults.object(forKey: SettingsKey.pitch) != nil { snapshot.pitch = defaults.float(forKey: SettingsKey.pitch) }
        if defaults.object(forKey: SettingsKey.volume) != nil { snapshot.volume = defaults.float(forKey: SettingsKey.volume) }
        snapshot.forceSpeed = defaults.bool(forKey: SettingsKey.forceSpeed)
        snapshot.forcePitch = defaults.bool(forKey: SettingsKey.forcePitch)
        snapshot.forceVolume = defaults.bool(forKey: SettingsKey.forceVolume)
        snapshot.forceLanguage = defaults.bool(forKey: SettingsKey.forceLanguage)
        snapshot.emojiEnabled = defaults.bool(forKey: SettingsKey.emojiEnabled)
        if defaults.object(forKey: SettingsKey.inflectionEnabled) != nil {
            snapshot.inflectionEnabled = defaults.bool(forKey: SettingsKey.inflectionEnabled)
        }
        if defaults.object(forKey: SettingsKey.sentencePause) != nil { snapshot.sentencePause = defaults.integer(forKey: SettingsKey.sentencePause) }
        if defaults.object(forKey: SettingsKey.commaPause) != nil { snapshot.commaPause = defaults.integer(forKey: SettingsKey.commaPause) }
        if defaults.object(forKey: SettingsKey.newlinePause) != nil { snapshot.newlinePause = defaults.integer(forKey: SettingsKey.newlinePause) }
        snapshot.numberMode = defaults.integer(forKey: SettingsKey.numberMode)
        if defaults.object(forKey: SettingsKey.userDictionariesEnabled) != nil {
            snapshot.userDictionariesEnabled = defaults.bool(forKey: SettingsKey.userDictionariesEnabled)
        }
        return snapshot
    }
}

/// Observable settings model used by the SwiftUI app. Every property change
/// is persisted immediately to the shared defaults suite.
@Observable
final class SettingsStore {
    private let defaults: UserDefaults

    var defaultVoice: String { didSet { defaults.set(defaultVoice, forKey: SettingsKey.defaultVoice) } }
    var speed: Float { didSet { defaults.set(speed, forKey: SettingsKey.speed) } }
    var pitch: Float { didSet { defaults.set(pitch, forKey: SettingsKey.pitch) } }
    var volume: Float { didSet { defaults.set(volume, forKey: SettingsKey.volume) } }
    var forceSpeed: Bool { didSet { defaults.set(forceSpeed, forKey: SettingsKey.forceSpeed) } }
    var forcePitch: Bool { didSet { defaults.set(forcePitch, forKey: SettingsKey.forcePitch) } }
    var forceVolume: Bool { didSet { defaults.set(forceVolume, forKey: SettingsKey.forceVolume) } }
    var forceLanguage: Bool { didSet { defaults.set(forceLanguage, forKey: SettingsKey.forceLanguage) } }
    var emojiEnabled: Bool { didSet { defaults.set(emojiEnabled, forKey: SettingsKey.emojiEnabled) } }
    var inflectionEnabled: Bool { didSet { defaults.set(inflectionEnabled, forKey: SettingsKey.inflectionEnabled) } }
    var sentencePause: Int { didSet { defaults.set(sentencePause, forKey: SettingsKey.sentencePause) } }
    var commaPause: Int { didSet { defaults.set(commaPause, forKey: SettingsKey.commaPause) } }
    var newlinePause: Int { didSet { defaults.set(newlinePause, forKey: SettingsKey.newlinePause) } }
    var numberMode: Int { didSet { defaults.set(numberMode, forKey: SettingsKey.numberMode) } }
    var userDictionariesEnabled: Bool { didSet { defaults.set(userDictionariesEnabled, forKey: SettingsKey.userDictionariesEnabled) } }

    init(defaults: UserDefaults = AppGroup.defaults) {
        self.defaults = defaults
        let snapshot = SettingsSnapshot.load(from: defaults)
        defaultVoice = snapshot.defaultVoice
        speed = snapshot.speed
        pitch = snapshot.pitch
        volume = snapshot.volume
        forceSpeed = snapshot.forceSpeed
        forcePitch = snapshot.forcePitch
        forceVolume = snapshot.forceVolume
        forceLanguage = snapshot.forceLanguage
        emojiEnabled = snapshot.emojiEnabled
        inflectionEnabled = snapshot.inflectionEnabled
        sentencePause = snapshot.sentencePause
        commaPause = snapshot.commaPause
        newlinePause = snapshot.newlinePause
        numberMode = snapshot.numberMode
        userDictionariesEnabled = snapshot.userDictionariesEnabled
    }

    var snapshot: SettingsSnapshot {
        SettingsSnapshot(
            defaultVoice: defaultVoice,
            speed: speed,
            pitch: pitch,
            volume: volume,
            forceSpeed: forceSpeed,
            forcePitch: forcePitch,
            forceVolume: forceVolume,
            forceLanguage: forceLanguage,
            emojiEnabled: emojiEnabled,
            inflectionEnabled: inflectionEnabled,
            sentencePause: sentencePause,
            commaPause: commaPause,
            newlinePause: newlinePause,
            numberMode: numberMode,
            userDictionariesEnabled: userDictionariesEnabled
        )
    }
}
