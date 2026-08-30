// AppModel.swift - App-wide state: engine lifecycle, playback, settings.

import Combine
import Foundation

@MainActor
final class AppModel: ObservableObject {
    let settings = SettingsStore()
    let dictionaries = DictionaryStore()

    @Published private(set) var isLoading = true
    @Published private(set) var isInitialized = false
    @Published private(set) var isPlaying = false
    @Published var errorMessage: String?

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
            let state = dictionaryState
            try await runOnEngineQueue { try engine.loadVoice(voiceID, dictionaries: state) }
            isInitialized = true
        } catch {
            errorMessage = String(localized: "Error starting TTS engine")
        }
        isLoading = false
    }

    func speak(_ text: String) {
        guard !text.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty else { return }
        guard let engine else { return }
        stop()
        isPlaying = true
        errorMessage = nil
        playbackTask = Task {
            // A cancelled task may resume after a newer speak() has already
            // set isPlaying back to true; stop() resets the flag itself, so
            // only a task that ran to completion may clear it here.
            defer {
                if !Task.isCancelled {
                    isPlaying = false
                }
            }
            do {
                let state = dictionaryState
                if !engine.isInitialized {
                    let voiceID = settings.defaultVoice
                    try await runOnEngineQueue { try engine.loadVoice(voiceID, dictionaries: state) }
                }
                let snapshot = settings.snapshot
                let chunk = try await runOnEngineQueue {
                    // Picks up dictionary entries edited since the last speak.
                    engine.syncDictionaries(state)
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

    /// Switches the active voice; persists the choice only on success.
    func selectVoice(_ voiceID: String) async {
        guard let engine else { return }
        do {
            let state = dictionaryState
            try await runOnEngineQueue { try engine.loadVoice(voiceID, dictionaries: state) }
            settings.defaultVoice = voiceID
        } catch {
            errorMessage = String(localized: "Error selecting voice")
        }
    }

    /// User dictionaries apply to the in-app preview as well as to the
    /// system speech extension.
    private var dictionaryState: DictionaryState {
        dictionaries.dictionaryState(userDictionariesEnabled: settings.userDictionariesEnabled)
    }

    /// Runs blocking engine work off the main actor.
    private func runOnEngineQueue<T: Sendable>(_ work: @escaping @Sendable () throws -> T) async throws -> T {
        try await Task.detached(priority: .userInitiated) {
            try work()
        }.value
    }
}
