// MainView.swift - Main tab: sample text input and the play button.

import SwiftUI

struct MainView: View {
    @EnvironmentObject private var model: AppModel

    /// Default demo text (not persisted). Kept as view state so typing does
    /// not republish the app-wide model on every keystroke.
    @State private var inputText = "Dobar dan. Ja sam Laprdus, rođen sam 2026. godine, i drago mi je da se možemo upoznati! 😁\nKako si ti? ❤\n"

    var body: some View {
        Group {
            if model.isLoading {
                ProgressView("Loading…")
                    .controlSize(.large)
            } else {
                content
            }
        }
        .navigationTitle("Laprdus TTS")
        .inlineNavigationBarTitle()
    }

    @ViewBuilder
    private var content: some View {
        VStack(spacing: 16) {
            TextEditor(text: $inputText)
                .font(.body)
                .scrollContentBackground(.hidden)
                .padding(8)
                .background {
                    // In dark mode on macOS the editor background is all but
                    // identical to the window, so the field needs an outline
                    // to read as a text area at all.
                    RoundedRectangle(cornerRadius: 10)
                        .fill(editorBackground)
                        .overlay {
                            RoundedRectangle(cornerRadius: 10)
                                .strokeBorder(Color.secondary.opacity(0.35), lineWidth: 1)
                        }
                }
                .overlay(alignment: .topLeading) {
                    if inputText.isEmpty {
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

            playButton
        }
        .padding()
    }

    private var playButton: some View {
        Button {
            if model.isPlaying {
                model.stop()
            } else {
                model.speak(inputText)
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
        .disabled(!model.isPlaying && (!model.isInitialized || inputText.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty))
        .accessibilityHint(model.isPlaying ? Text("Stops current speech") : Text("Speaks the entered text"))
        .keyboardShortcut(.defaultAction)
    }

    private var editorBackground: Color {
        #if os(macOS)
        Color(nsColor: .textBackgroundColor)
        #else
        Color(uiColor: .secondarySystemBackground)
        #endif
    }
}

#Preview {
    NavigationStack {
        MainView()
    }
    .environmentObject(AppModel())
}
