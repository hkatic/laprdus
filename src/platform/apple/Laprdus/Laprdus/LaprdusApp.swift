// LaprdusApp.swift - App entry point.

import AVFAudio
import SwiftUI

@main
struct LaprdusApp: App {
    @StateObject private var model = AppModel()
    @Environment(\.scenePhase) private var scenePhase

    var body: some Scene {
        WindowGroup {
            RootView()
                .environmentObject(model)
                .environmentObject(model.settings)
                .task {
                    await model.initialize()
                }
                .task {
                    SystemVoices.update()
                }
        }
        .onChange(of: scenePhase) { newPhase in
            // Preview playback stops when the app leaves the foreground.
            if newPhase == .background {
                model.stop()
            }
        }
        #if os(macOS)
        .defaultSize(width: 640, height: 720)
        #endif
    }
}

/// The system's list of voices (Spoken Content, VoiceOver, every app).
enum SystemVoices {
    /// Tells the system to ask the LaprdusVoices extension for its voices.
    /// Registering the extension is not enough: without this call the voices
    /// never show up. Not on the main thread, because the call waits for a
    /// system service and can take seconds.
    static func update() {
        Task.detached(priority: .utility) {
            AVSpeechSynthesisProviderVoice.updateSpeechVoices()
        }
    }
}
