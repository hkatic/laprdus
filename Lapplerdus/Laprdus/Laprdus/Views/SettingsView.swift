// SettingsView.swift - Settings tab: engine configuration.
//
// VoiceOver notes: every row here is meant to be a *single* stop. Rows that
// pair a visual caption with a control (sliders, toggles) hide the caption
// from accessibility and put the same information on the control itself as
// label/value/hint, so flicking never lands on a caption, then its value, and
// only then on the control. This mirrors what the Android screen does with
// clearAndSetSemantics.

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
            speechSection
            overridesSection
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
                    .accessibilityElement(children: .ignore)
                    .accessibilityLabel("\(voice.localizedName), \(voice.localizedDetails)")
                    .tag(voice.id)
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
        }
    }

    // MARK: Speech

    private var speechSection: some View {
        Section("Speech") {
            SliderRow(
                title: String(localized: "Speech rate"),
                value: $settings.speed,
                range: 0.5...2.0,
                step: 0.1,
                format: { String(format: "%.1fx", $0) },
                restoreLabel: String(localized: "Restore default speech rate"),
                restore: { settings.speed = 1.0 }
            )
            SliderRow(
                title: String(localized: "Speech pitch"),
                value: $settings.pitch,
                range: 0.5...2.0,
                step: 0.1,
                format: { String(format: "%.1fx", $0) },
                restoreLabel: String(localized: "Restore default speech pitch"),
                restore: { settings.pitch = 1.0 }
            )
            SliderRow(
                title: String(localized: "Speech volume"),
                value: $settings.volume,
                range: 0.0...1.0,
                step: 0.05,
                format: { "\(Int(($0 * 100).rounded()))%" },
                restoreLabel: String(localized: "Restore default speech volume"),
                restore: { settings.volume = 1.0 }
            )

            Button("Restore default speech rate") { settings.speed = 1.0 }
            Button("Restore default speech pitch") { settings.pitch = 1.0 }
            Button("Restore default speech volume") { settings.volume = 1.0 }
        }
    }

    // MARK: Application overrides

    private var overridesSection: some View {
        Section("Application Overrides") {
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
        }
    }

    // MARK: Advanced

    private var advancedSection: some View {
        Section("Advanced") {
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

/// One slider row, laid out the way each platform's Form expects. A macOS
/// grouped Form owns a label column, so the title goes there and the slider,
/// its value and any description share the control column; trying to span the
/// full width there just strands the slider on the right, away from its title.
/// iOS has no such column, so the title and value sit above the slider — and
/// stack vertically at accessibility text sizes, where side by side would
/// squeeze both into narrow wrapped columns.
///
/// The visible title and value are always hidden from VoiceOver: the slider
/// itself carries both, so the row is a single stop.
private struct SliderRowLayout<SliderView: View>: View {
    @Environment(\.dynamicTypeSize) private var dynamicTypeSize

    let title: String
    let value: String
    var subtitle: String?
    @ViewBuilder var slider: SliderView

    var body: some View {
        #if os(macOS)
        LabeledContent {
            VStack(alignment: .leading, spacing: 4) {
                HStack(spacing: 8) {
                    slider
                    valueText
                }
                subtitleText
            }
        } label: {
            Text(title)
                .accessibilityHidden(true)
        }
        #else
        VStack(alignment: .leading, spacing: 4) {
            caption
            slider
            subtitleText
        }
        #endif
    }

    private var caption: some View {
        Group {
            if dynamicTypeSize.isAccessibilitySize {
                VStack(alignment: .leading, spacing: 2) {
                    Text(title)
                    valueText
                }
                .frame(maxWidth: .infinity, alignment: .leading)
            } else {
                HStack {
                    Text(title)
                    Spacer()
                    valueText
                }
            }
        }
        .accessibilityHidden(true)
    }

    private var valueText: some View {
        Text(value)
            .foregroundStyle(.secondary)
            .monospacedDigit()
            .accessibilityHidden(true)
    }

    @ViewBuilder
    private var subtitleText: some View {
        if let subtitle {
            Text(subtitle)
                .font(.footnote)
                .foregroundStyle(.secondary)
                .fixedSize(horizontal: false, vertical: true)
                .accessibilityHidden(true)
        }
    }
}

/// `step` makes the announced value match the stored one exactly — without it
/// VoiceOver adjusts by 10% of the range and lands on values the format string
/// rounds away.
private struct SliderRow: View {
    let title: String
    @Binding var value: Float
    let range: ClosedRange<Float>
    let step: Float
    let format: (Float) -> String
    let restoreLabel: String
    let restore: () -> Void

    var body: some View {
        SliderRowLayout(title: title, value: format(value)) {
            Slider(value: $value, in: range, step: step)
                .accessibilityLabel(title)
                .accessibilityValue(format(value))
                // Reaching the per-setting reset without leaving the slider
                // saves a trip to the button at the end of the section.
                .accessibilityAction(named: Text(restoreLabel), restore)
        }
    }
}

/// Same layout; the subtitle is a VoiceOver hint rather than a separate
/// element, so this row is also a single stop.
private struct PauseSliderRow: View {
    let title: String
    let subtitle: String
    @Binding var value: Int

    var body: some View {
        SliderRowLayout(title: title, value: String(localized: "\(value) ms"), subtitle: subtitle) {
            Slider(
                value: Binding(
                    get: { Double(value) },
                    set: { value = Int($0.rounded()) }
                ),
                in: 0...2000,
                // 50 ms steps keep end-to-end VoiceOver adjustment to 40
                // increments instead of 200.
                step: 50
            )
            .accessibilityLabel(title)
            .accessibilityValue("\(value) ms")
            .accessibilityHint(subtitle)
        }
    }
}

/// The subtitle is hidden and re-attached as the toggle's hint, so VoiceOver
/// announces "Title, switch button, on" and only then, after its usual pause,
/// the explanation — instead of reading the whole paragraph up front.
private struct ToggleRow: View {
    let title: String
    let subtitle: String
    @Binding var isOn: Bool

    var body: some View {
        Toggle(isOn: $isOn) {
            VStack(alignment: .leading, spacing: 2) {
                Text(title)
                Text(subtitle)
                    .font(.footnote)
                    .foregroundStyle(.secondary)
                    // Without this the switch squeezes the subtitle into a
                    // single truncated line.
                    .fixedSize(horizontal: false, vertical: true)
                    .accessibilityHidden(true)
            }
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .accessibilityLabel(title)
        .accessibilityHint(subtitle)
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
