// LaprdusApp.swift - App entry point.

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
