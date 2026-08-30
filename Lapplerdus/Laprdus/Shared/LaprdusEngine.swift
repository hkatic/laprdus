// LaprdusEngine.swift - Swift wrapper around the LaprdusTTS C API.
// All engine calls are funneled through a serial queue; the C engine itself
// is not thread-safe.

import Foundation

enum LaprdusEngineError: LocalizedError {
    case createFailed
    case resourcesMissing
    case api(String)

    var errorDescription: String? {
        switch self {
        case .createFailed: return "Failed to create the TTS engine"
        case .resourcesMissing: return "Voice data is missing from the app bundle"
        case .api(let message): return message
        }
    }
}

final class LaprdusEngine: @unchecked Sendable {

    struct AudioChunk: Sendable {
        let samples: [Int16]
        let sampleRate: Double
        let channels: Int
    }

    private let handle: LaprdusHandle
    private let queue = DispatchQueue(label: "com.hrvojekatic.laprdus.engine")
    private var currentVoiceID: String?
    private var appliedDictionaryStamp: String?

    init() throws {
        guard let created = laprdus_create() else {
            throw LaprdusEngineError.createFailed
        }
        handle = created
    }

    deinit {
        laprdus_destroy(handle)
    }

    static var version: String {
        laprdus_get_version().map { String(cString: $0) } ?? "unknown"
    }

    private var lastErrorMessage: String {
        laprdus_get_error_message(handle).map { String(cString: $0) } ?? "Unknown engine error"
    }

    var isInitialized: Bool {
        queue.sync { laprdus_is_initialized(handle) != 0 }
    }

    var currentVoice: String? {
        queue.sync { currentVoiceID }
    }

    /// Loads a voice from the bundle's resources and (re)loads the bundled
    /// dictionaries, then layers the user dictionary on top.
    func loadVoice(_ voiceID: String, dictionaries state: DictionaryState) throws {
        try queue.sync {
            try performLoadVoice(voiceID, dictionaries: state)
        }
    }

    /// Reloads dictionaries when the user dictionary has changed since the last
    /// load. Editing an entry in the app must take effect in the already
    /// running speech extension, and appending alone cannot remove or amend an
    /// entry, so a changed stamp reloads the voice to reset dictionary state
    /// before re-applying the file.
    func syncDictionaries(_ state: DictionaryState) {
        queue.sync {
            guard state.stamp != appliedDictionaryStamp, let voiceID = currentVoiceID else { return }
            try? performLoadVoice(voiceID, dictionaries: state)
        }
    }

    /// Must be called on `queue`.
    private func performLoadVoice(_ voiceID: String, dictionaries state: DictionaryState) throws {
        let bundle = Bundle(for: LaprdusEngine.self)
        guard let resourcePath = bundle.resourceURL?.path else {
            throw LaprdusEngineError.resourcesMissing
        }
        guard laprdus_set_voice(handle, voiceID, resourcePath) == LAPRDUS_OK else {
            throw LaprdusEngineError.api(lastErrorMessage)
        }
        currentVoiceID = voiceID
        // set_voice may reinitialize the engine and drop dictionary state,
        // so the bundled dictionaries are reloaded after every switch.
        if let path = bundle.path(forResource: "internal", ofType: "json") {
            _ = laprdus_load_dictionary(handle, path)
        }
        if let path = bundle.path(forResource: "spelling", ofType: "json") {
            _ = laprdus_load_spelling_dictionary(handle, path)
        }
        if let path = bundle.path(forResource: "emoji", ofType: "json") {
            _ = laprdus_load_emoji_dictionary(handle, path)
        }
        if let url = state.userDictionaryURL {
            _ = laprdus_append_dictionary(handle, url.path)
        }
        appliedDictionaryStamp = state.stamp
    }

    /// Applies persisted settings. The user pitch slider maps to
    /// laprdus_set_user_pitch (formant preserving) — the voice-character pitch
    /// channel is managed by laprdus_set_voice via the registry's base_pitch.
    func apply(_ settings: SettingsSnapshot) {
        queue.sync {
            _ = laprdus_set_speed(handle, settings.speed)
            _ = laprdus_set_user_pitch(handle, settings.pitch)
            _ = laprdus_set_volume(handle, settings.volume)
            _ = laprdus_set_emoji_enabled(handle, settings.emojiEnabled ? 1 : 0)
            _ = laprdus_set_inflection_enabled(handle, settings.inflectionEnabled ? 1 : 0)
            _ = laprdus_set_sentence_pause(handle, UInt32(max(0, min(settings.sentencePause, 2000))))
            _ = laprdus_set_comma_pause(handle, UInt32(max(0, min(settings.commaPause, 2000))))
            _ = laprdus_set_newline_pause(handle, UInt32(max(0, min(settings.newlinePause, 2000))))
            _ = laprdus_set_number_mode(handle, settings.numberMode == 1 ? LAPRDUS_NUMBER_MODE_DIGIT : LAPRDUS_NUMBER_MODE_WHOLE)
        }
    }

    /// Per-request overrides used by the speech extension after resolving the
    /// host's requested rate/pitch against the "force" settings.
    func setTransientParameters(speed: Float, userPitch: Float, volume: Float) {
        queue.sync {
            _ = laprdus_set_speed(handle, speed)
            _ = laprdus_set_user_pitch(handle, userPitch)
            _ = laprdus_set_volume(handle, volume)
        }
    }

    func synthesize(_ text: String, spelled: Bool = false) throws -> AudioChunk {
        try queue.sync {
            var samples: UnsafeMutablePointer<Int16>? = nil
            var format = LaprdusAudioFormat()
            let count = spelled
                ? laprdus_synthesize_spelled(handle, text, &samples, &format)
                : laprdus_synthesize(handle, text, &samples, &format)
            guard count >= 0, let buffer = samples else {
                throw LaprdusEngineError.api(lastErrorMessage)
            }
            defer { laprdus_free_buffer(buffer) }
            let array = Array(UnsafeBufferPointer(start: buffer, count: Int(count)))
            return AudioChunk(
                samples: array,
                sampleRate: Double(format.sample_rate),
                channels: Int(max(format.channels, 1))
            )
        }
    }

    /// Deliberately bypasses the serial queue so it can interrupt a synthesis
    /// that is currently running on it.
    func cancel() {
        laprdus_cancel(handle)
    }
}
