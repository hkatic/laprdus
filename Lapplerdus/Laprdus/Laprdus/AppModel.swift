// AppModel.swift - App-wide state: engine lifecycle, playback, settings.
// Plays the role of the Android TTSViewModel + Hilt singletons.

import Foundation
import Observation

@MainActor
@Observable
final class AppModel {
    let settings = SettingsStore()
    let dictionaries = DictionaryStore()

    private(set) var isLoading = true
    private(set) var isInitialized = false
    private(set) var isPlaying = false
    var errorMessage: String?

    /// Same default demo text as the Android app (not persisted).
    var inputText = "Dobar dan. Ja sam Laprdus, rođen sam 2026. godine, i drago mi je da se možemo upoznati! 😁\nKako si ti? ❤\n"

    private var engine: LaprdusEngine?
    private let player = AudioPlayer()
    private var playbackTask: Task<Void, Never>?

    /// Loads the engine and the persisted default voice. Called once at launch.
    func initialize() async {
        guard engine == nil else { return }
        do {
            let engine = try LaprdusEngine()
            self.engine = engine
            let voiceID = settings.defaultVoice
            try await runOnEngineQueue { try engine.loadVoice(voiceID) }
            applyUserDictionaries()
            isInitialized = true
        } catch {
            errorMessage = String(localized: "Error starting TTS engine")
        }
        isLoading = false
    }

    func speak() {
        let text = inputText
        guard !text.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty else { return }
        guard let engine else { return }
        stop()
        isPlaying = true
        errorMessage = nil
        playbackTask = Task {
            defer { isPlaying = false }
            do {
                if !engine.isInitialized {
                    let voiceID = settings.defaultVoice
                    try await runOnEngineQueue { try engine.loadVoice(voiceID) }
                    applyUserDictionaries()
                }
                let snapshot = settings.snapshot
                let chunk = try await runOnEngineQueue {
                    engine.apply(snapshot)
                    return try engine.synthesize(text)
                }
                guard !Task.isCancelled else { return }
                try await player.play(chunk)
            } catch is CancellationError {
                // stopped by the user
            } catch {
                if !Task.isCancelled {
                    errorMessage = String(localized: "Error during speech synthesis")
                }
            }
        }
    }

    func stop() {
        playbackTask?.cancel()
        playbackTask = nil
        engine?.cancel()
        player.stop()
        isPlaying = false
    }

    /// Switches the active voice; persists the choice only on success,
    /// mirroring the Android settings screen.
    func selectVoice(_ voiceID: String) async {
        guard let engine else { return }
        do {
            try await runOnEngineQueue { try engine.loadVoice(voiceID) }
            applyUserDictionaries()
            settings.defaultVoice = voiceID
        } catch {
            errorMessage = String(localized: "Error selecting voice")
        }
    }

    /// Unlike Android (where only the system service applied user
    /// dictionaries), the in-app preview applies them too.
    private func applyUserDictionaries() {
        guard settings.userDictionariesEnabled,
              let engine,
              let url = dictionaries.userDictionaryURLIfPresent else { return }
        engine.appendUserDictionary(at: url)
    }

    /// Runs blocking engine work off the main actor.
    private func runOnEngineQueue<T: Sendable>(_ work: @escaping @Sendable () throws -> T) async throws -> T {
        try await Task.detached(priority: .userInitiated) {
            try work()
        }.value
    }
}
