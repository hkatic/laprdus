// MainView.swift - Main screen: text input and speak/stop, like the Android app.

import SwiftUI

struct MainView: View {
    @Environment(AppModel.self) private var model

    var body: some View {
        @Bindable var model = model
        NavigationStack {
            Group {
                if model.isLoading {
                    ProgressView("Loading…")
                        .controlSize(.large)
                } else {
                    content
                }
            }
            .navigationTitle("Laprdus TTS")
            .toolbar {
                ToolbarItem {
                    NavigationLink {
                        SettingsView()
                    } label: {
                        Label("Laprdus Settings", systemImage: "gearshape")
                    }
                    .accessibilityHint(Text("Opens Laprdus TTS engine settings"))
                }
            }
        }
    }

    @ViewBuilder
    private var content: some View {
        @Bindable var model = model
        VStack(spacing: 16) {
            TextEditor(text: $model.inputText)
                .font(.body)
                .scrollContentBackground(.hidden)
                .padding(8)
                .background(.background.secondary, in: RoundedRectangle(cornerRadius: 10))
                .overlay(alignment: .topLeading) {
                    if model.inputText.isEmpty {
                        Text("Enter text you want to hear…")
                            .foregroundStyle(.secondary)
                            .padding(.horizontal, 12)
                            .padding(.vertical, 16)
                            .allowsHitTesting(false)
                    }
                }
                .frame(maxHeight: .infinity)
                .disabled(model.isPlaying)
                .accessibilityLabel(Text("Text to speak"))
                .accessibilityHint(Text("Text input field for text that will be spoken"))

            if let error = model.errorMessage {
                Text("Error: \(error)")
                    .font(.callout)
                    .foregroundStyle(.red)
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .accessibilityAddTraits(.updatesFrequently)
            }

            speakButton

            systemSpeechSettingsButton
        }
        .padding()
    }

    private var speakButton: some View {
        Button {
            if model.isPlaying {
                model.stop()
            } else {
                model.speak()
            }
        } label: {
            Label {
                model.isPlaying ? Text("Stop") : Text("Speak")
            } icon: {
                Image(systemName: model.isPlaying ? "stop.fill" : "play.fill")
            }
            .font(.title3.weight(.semibold))
            .frame(maxWidth: .infinity, minHeight: 44)
        }
        .buttonStyle(.borderedProminent)
        .tint(model.isPlaying ? .red : .accentColor)
        .disabled(!model.isPlaying && (!model.isInitialized || model.inputText.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty))
        .accessibilityHint(model.isPlaying ? Text("Stops current speech") : Text("Speaks the entered text"))
        .keyboardShortcut(.defaultAction)
    }

    /// Apple counterpart of the Android "Android TTS Settings" button.
    private var systemSpeechSettingsButton: some View {
        Button {
            openSystemSpeechSettings()
        } label: {
            Label("System Speech Settings", systemImage: "person.wave.2")
                .frame(maxWidth: .infinity, minHeight: 36)
        }
        .buttonStyle(.bordered)
        .accessibilityHint(Text("Opens the system speech settings"))
    }

    private func openSystemSpeechSettings() {
        #if os(macOS)
        // Accessibility > Spoken Content pane, where Laprdus voices appear
        // once the extension is registered.
        if let url = URL(string: "x-apple.systempreferences:com.apple.preference.universalaccess") {
            NSWorkspace.shared.open(url)
        }
        #elseif os(iOS) || os(visionOS)
        if let url = URL(string: UIApplication.openSettingsURLString) {
            UIApplication.shared.open(url)
        }
        #endif
    }
}

#Preview {
    MainView()
        .environment(AppModel())
}
