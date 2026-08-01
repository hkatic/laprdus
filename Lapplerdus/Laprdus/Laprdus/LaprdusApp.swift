// LaprdusApp.swift - App entry point.

import SwiftUI

@main
struct LaprdusApp: App {
    @State private var model = AppModel()
    @Environment(\.scenePhase) private var scenePhase

    var body: some Scene {
        WindowGroup {
            MainView()
                .environment(model)
                .task {
                    await model.initialize()
                }
        }
        .onChange(of: scenePhase) { _, newPhase in
            // Mirrors Android: preview playback stops when the app leaves
            // the foreground.
            if newPhase == .background {
                model.stop()
            }
        }
        #if os(macOS)
        .defaultSize(width: 560, height: 640)
        #endif
    }
}
