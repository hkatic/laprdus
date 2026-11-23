// RootView.swift - Top-level tab layout.

import SwiftUI

struct RootView: View {
    var body: some View {
        TabView {
            NavigationStack {
                MainView()
            }
            .tabItem {
                Label("Main", systemImage: "play.circle")
            }

            NavigationStack {
                SettingsView()
            }
            .tabItem {
                Label("Settings", systemImage: "gearshape")
            }

            NavigationStack {
                DictionaryListView()
            }
            .tabItem {
                Label("Dictionaries", systemImage: "character.book.closed")
            }

            NavigationStack {
                AboutView()
            }
            .tabItem {
                Label("About", systemImage: "info.circle")
            }
        }
    }
}

/// Inline navigation bar titles keep all tabs consistent; the modifier
/// only exists on iOS, so this is a no-op elsewhere.
extension View {
    @ViewBuilder
    func inlineNavigationBarTitle() -> some View {
        #if os(iOS)
        navigationBarTitleDisplayMode(.inline)
        #else
        self
        #endif
    }
}

#Preview {
    let model = AppModel()
    RootView()
        .environmentObject(model)
        .environmentObject(model.settings)
}
