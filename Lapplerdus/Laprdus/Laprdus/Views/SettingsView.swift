// SettingsView.swift - Settings tab: engine configuration.

import SwiftUI

struct SettingsView: View {
    @EnvironmentObject private var model: AppModel
    @EnvironmentObject private var settings: SettingsStore

    /// The picker binds straight to the persisted default voice, so there is
    /// no mirrored state to keep in sync. selectVoice persists only on
    /// success; on failure nothing changes and the next render (triggered by
    /// the published errorMessage) snaps the picker back automatically.
    private var selectedVoice: Binding<String> {
        Binding(
            get: { settings.defaultVoice },
            set: { newValue in
                guard newValue != settings.defaultVoice else { return }
                Task {
                    await model.selectVoice(newValue)
                }
            }
        )
    }

    var body: some View {
        Form {
            voiceSection
            advancedSection
            pausesSection
            dictionariesSection
        }
        .formStyle(.grouped)
        .navigationTitle("Settings")
        .inlineNavigationBarTitle()
    }

    // MARK: Voice

    private var voiceSection: some View {
        Section("Voice") {
            Picker(selection: selectedVoice) {
                ForEach(VoiceCatalog.all) { voice in
                    VStack(alignment: .leading) {
                        Text(voice.localizedName)
                        Text(voice.localizedDetails)
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    }
                    .tag(voice.id)
                    .accessibilityLabel("\(voice.localizedName), \(voice.localizedDetails)")
                }
            } label: {
                Text("Voice")
            }
            .accessibilityHint(Text("Select voice for speech synthesis"))

            // Voice selection errors must be visible on this tab; the Main
            // tab's error area is not on screen while Settings is active.
            if let error = model.errorMessage {
                Text("Error: \(error)")
                    .font(.callout)
                    .foregroundStyle(.red)
                    .accessibilityAddTraits(.updatesFrequently)
            }

            SliderRow(
                title: String(localized: "Speech rate"),
                value: $settings.speed,
                range: 0.5...2.0,
                format: { String(format: "%.1fx", $0) }
            )
            SliderRow(
                title: String(localized: "Speech pitch"),
                value: $settings.pitch,
                range: 0.5...2.0,
                format: { String(format: "%.1fx", $0) }
            )
            SliderRow(
                title: String(localized: "Speech volume"),
                value: $settings.volume,
                range: 0.0...1.0,
                format: { "\(Int($0 * 100))%" }
            )

            ToggleRow(
                title: String(localized: "Force Laprdus speech rate"),
                subtitle: String(localized: "Use Laprdus rate settings instead of application settings"),
                isOn: $settings.forceSpeed
            )
            ToggleRow(
                title: String(localized: "Force Laprdus speech pitch"),
                subtitle: String(localized: "Use Laprdus pitch settings instead of application settings"),
                isOn: $settings.forcePitch
            )
            ToggleRow(
                title: String(localized: "Force Laprdus speech volume"),
                subtitle: String(localized: "Use Laprdus volume settings instead of application settings"),
                isOn: $settings.forceVolume
            )

            Button("Restore default speech rate") { settings.speed = 1.0 }
                .accessibilityHint(Text("Sets speech rate to default value"))
            Button("Restore default speech pitch") { settings.pitch = 1.0 }
                .accessibilityHint(Text("Sets speech pitch to default value"))
            Button("Restore default speech volume") { settings.volume = 1.0 }
                .accessibilityHint(Text("Sets speech volume to default value"))
        }
    }

    // MARK: Advanced

    private var advancedSection: some View {
        Section("Advanced") {
            ToggleRow(
                title: String(localized: "Force language"),
                subtitle: String(localized: "Use selected language regardless of system settings"),
                isOn: $settings.forceLanguage
            )
            ToggleRow(
                title: String(localized: "Enable emoji reading"),
                subtitle: String(localized: "Convert emojis to their text descriptions"),
                isOn: $settings.emojiEnabled
            )
            ToggleRow(
                title: String(localized: "Voice inflection"),
                subtitle: String(localized: "Vary pitch for questions, exclamations, and pauses"),
                isOn: $settings.inflectionEnabled
            )
            ToggleRow(
                title: String(localized: "Digit-by-digit numbers"),
                subtitle: String(localized: "Read numbers as individual digits (123 → one two three)"),
                isOn: Binding(
                    get: { settings.numberMode == 1 },
                    set: { settings.numberMode = $0 ? 1 : 0 }
                )
            )
        }
    }

    // MARK: Reading pauses

    private var pausesSection: some View {
        Section("Reading Pauses") {
            PauseSliderRow(
                title: String(localized: "Pause after sentences"),
                subtitle: String(localized: "Duration of silence after periods, exclamation marks, and question marks"),
                value: $settings.sentencePause
            )
            PauseSliderRow(
                title: String(localized: "Pause after commas"),
                subtitle: String(localized: "Duration of silence after commas for natural breathing rhythm"),
                value: $settings.commaPause
            )
            PauseSliderRow(
                title: String(localized: "Pause at new lines"),
                subtitle: String(localized: "Duration of silence when moving to a new line or paragraph"),
                value: $settings.newlinePause
            )
        }
    }

    // MARK: Dictionaries

    private var dictionariesSection: some View {
        Section("Dictionaries") {
            ToggleRow(
                title: String(localized: "Apply user dictionaries"),
                subtitle: String(localized: "Use custom word pronunciations from user dictionaries"),
                isOn: $settings.userDictionariesEnabled
            )
        }
    }
}

// MARK: - Row components

private struct SliderRow: View {
    let title: String
    @Binding var value: Float
    let range: ClosedRange<Float>
    let format: (Float) -> String

    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(title)
                Spacer()
                Text(format(value))
                    .foregroundStyle(.secondary)
                    .monospacedDigit()
            }
            Slider(value: $value, in: range)
                .accessibilityLabel(title)
                .accessibilityValue(format(value))
        }
    }
}

private struct PauseSliderRow: View {
    let title: String
    let subtitle: String
    @Binding var value: Int

    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(title)
                Spacer()
                Text("\(value) ms")
                    .foregroundStyle(.secondary)
                    .monospacedDigit()
            }
            Slider(
                value: Binding(
                    get: { Double(value) },
                    set: { value = Int($0) }
                ),
                in: 0...2000,
                step: 10
            )
            .accessibilityLabel(title)
            .accessibilityValue("\(value) ms")
            .accessibilityHint(subtitle)
            Text(subtitle)
                .font(.footnote)
                .foregroundStyle(.secondary)
        }
    }
}

private struct ToggleRow: View {
    let title: String
    let subtitle: String
    @Binding var isOn: Bool

    var body: some View {
        Toggle(isOn: $isOn) {
            VStack(alignment: .leading) {
                Text(title)
                Text(subtitle)
                    .font(.footnote)
                    .foregroundStyle(.secondary)
            }
        }
        .accessibilityLabel("\(title). \(subtitle)")
    }
}

#Preview {
    let model = AppModel()
    NavigationStack {
        SettingsView()
    }
    .environmentObject(model)
    .environmentObject(model.settings)
}
