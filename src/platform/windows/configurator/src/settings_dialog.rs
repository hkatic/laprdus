//! The settings dialog, its "Voice quality..." dialog and the Test button.
//!
//! The dialog edits `settings.json`: OK and Apply write it, and so does Test,
//! because the SAPI5 driver speaks with the saved settings.

use crate::dark_mode;
use crate::dictionary_dialog;
use crate::ids::*;
use crate::text::{self, Number, set_texts};
use crate::win32::{
    self, BN_CLICKED, CBN_SELCHANGE, ColorOf, Dialog, Event, IDCANCEL, IDOK, Reply, VoiceToken, Window,
};
use laprdus::config::{UserConfig, UserSettings};
use laprdus::params::{
    FORMANT_EFFECTIVE_SPEED_MAX, FORMANT_NOMINAL_WPM, FORMANT_SPEED_MAX, NumberMode, PauseSettings, SpellingMode,
    VOICE_QUALITY_DEFAULT, VOICE_QUALITY_MAX, VOICE_QUALITY_MIN,
};
use laprdus::voice::{self, Age, Gender, VOICES, Voice};
use std::cell::{Cell, RefCell};

const TEXTS: [(u16, u16); 28] = [
    (IDC_GROUP_VOICE, IDS_GROUP_VOICE),
    (IDC_GROUP_PAUSES, IDS_GROUP_PAUSES),
    (IDC_GROUP_OPTIONS, IDS_GROUP_OPTIONS),
    (IDC_GROUP_DICTIONARIES, IDS_GROUP_DICTIONARIES),
    (IDC_VOICE_LABEL, IDS_VOICE_LABEL),
    (IDC_SPEED_LABEL, IDS_SPEED_LABEL),
    (IDC_FORCE_SPEED, IDS_FORCE_SPEED),
    (IDC_PITCH_LABEL, IDS_PITCH_LABEL),
    (IDC_FORCE_PITCH, IDS_FORCE_PITCH),
    (IDC_VOLUME_LABEL, IDS_VOLUME_LABEL),
    (IDC_FORCE_VOLUME, IDS_FORCE_VOLUME),
    (IDC_VOLUME_BOOST_CHECK, IDS_VOLUME_BOOST),
    (IDC_VOICE_QUALITY_BUTTON, IDS_VOICE_QUALITY_BUTTON),
    (IDC_COMMA_LABEL, IDS_COMMA_LABEL),
    (IDC_SENTENCE_LABEL, IDS_SENTENCE_LABEL),
    (IDC_NEWLINE_LABEL, IDS_NEWLINE_LABEL),
    (IDC_INFLECTION_LABEL, IDS_INFLECTION_LABEL),
    (IDC_ACCELERATION_LABEL, IDS_ACCELERATION_LABEL),
    (IDC_SPELLING_SPEED_LABEL, IDS_SPELLING_SPEED_LABEL),
    (IDC_EMOJI_CHECK, IDS_EMOJI),
    (IDC_DIGITS_CHECK, IDS_DIGITS),
    (IDC_SPELLING_SOUNDS_CHECK, IDS_SPELLING_SOUNDS),
    (IDC_USER_DICT_CHECK, IDS_USER_DICT_CHECK),
    (IDC_DICT_BUTTON, IDS_DICT_BUTTON),
    (IDC_TEST_BUTTON, IDS_TEST),
    (IDOK, IDS_OK),
    (IDCANCEL, IDS_CANCEL),
    (IDC_APPLY_BUTTON, IDS_APPLY),
];

/// How a slider's position is written next to it.
#[derive(Clone, Copy, PartialEq, Eq)]
enum Unit {
    /// "1,5x": the position is a multiplier in percent.
    Times,
    /// "50 %".
    Percent,
    /// "100 ms".
    Milliseconds,
    /// "1,5x (up to 525 WPM)": the acceleration with the top rate it gives.
    TopRate,
}

/// A slider of the settings dialog and the setting it edits.
struct Slider {
    slider: u16,
    value: u16,
    unit: Unit,
    range: (i32, i32),
    ticks: i32,
    get: fn(&UserSettings) -> i32,
    set: fn(&mut UserSettings, i32),
}

/// A setting of 1.0 as the slider position 100.
fn to_percent(value: f32) -> i32 {
    (value * 100.0).round() as i32
}

fn from_percent(position: i32) -> f32 {
    position as f32 / 100.0
}

const PAUSES: (i32, i32) = (0, PauseSettings::MAX_MS as i32);

// Speed and pitch start with the range of the recorded voices; the selected
// voice then sets it (`RECORDED_RANGE`, `FORMANT_RANGE`).
#[rustfmt::skip]
const SLIDERS: [Slider; 9] = [
    Slider { slider: IDC_SPEED_SLIDER, value: IDC_SPEED_VALUE, unit: Unit::Times, range: RECORDED_RANGE, ticks: 25,
             get: |s| to_percent(s.speed), set: |s, v| s.speed = from_percent(v) },
    Slider { slider: IDC_ACCELERATION_SLIDER, value: IDC_ACCELERATION_VALUE, unit: Unit::TopRate, range: (50, 300), ticks: 25,
             get: |s| to_percent(s.acceleration), set: |s, v| s.acceleration = from_percent(v) },
    Slider { slider: IDC_PITCH_SLIDER, value: IDC_PITCH_VALUE, unit: Unit::Times, range: RECORDED_RANGE, ticks: 25,
             get: |s| to_percent(s.user_pitch), set: |s, v| s.user_pitch = from_percent(v) },
    Slider { slider: IDC_INFLECTION_SLIDER, value: IDC_INFLECTION_VALUE, unit: Unit::Percent, range: (0, 100), ticks: 10,
             get: |s| to_percent(s.inflection_level), set: |s, v| s.inflection_level = from_percent(v) },
    // The volume is cut, not rounded, to its percent, as it always was
    Slider { slider: IDC_VOLUME_SLIDER, value: IDC_VOLUME_VALUE, unit: Unit::Percent, range: (0, 100), ticks: 10,
             get: |s| (s.volume * 100.0) as i32, set: |s, v| s.volume = from_percent(v) },
    Slider { slider: IDC_COMMA_SLIDER, value: IDC_COMMA_VALUE, unit: Unit::Milliseconds, range: PAUSES, ticks: 200,
             get: |s| s.pauses.comma_pause_ms as i32, set: |s, v| s.pauses.comma_pause_ms = v.max(0) as u32 },
    Slider { slider: IDC_SENTENCE_SLIDER, value: IDC_SENTENCE_VALUE, unit: Unit::Milliseconds, range: PAUSES, ticks: 200,
             get: |s| s.pauses.sentence_pause_ms as i32, set: |s, v| s.pauses.sentence_pause_ms = v.max(0) as u32 },
    Slider { slider: IDC_NEWLINE_SLIDER, value: IDC_NEWLINE_VALUE, unit: Unit::Milliseconds, range: PAUSES, ticks: 200,
             get: |s| s.pauses.newline_pause_ms as i32, set: |s, v| s.pauses.newline_pause_ms = v.max(0) as u32 },
    Slider { slider: IDC_SPELLING_SPEED_SLIDER, value: IDC_SPELLING_SPEED_VALUE, unit: Unit::Percent, range: (0, 100), ticks: 10,
             get: |s| s.spelling_speed, set: |s, v| s.spelling_speed = v },
];

/// Speed and pitch in percent: the recorded voices stop at 0.5x and 2.0x,
/// the formant voices reach 0.25x and 4.0x.
const RECORDED_RANGE: (i32, i32) = (50, 200);
const FORMANT_RANGE: (i32, i32) = (25, 400);

/// A check box of the settings dialog and the setting it edits.
struct Check {
    id: u16,
    get: fn(&UserSettings) -> bool,
    set: fn(&mut UserSettings, bool),
}

#[rustfmt::skip]
const CHECKS: [Check; 8] = [
    Check { id: IDC_FORCE_SPEED, get: |s| s.force_speed, set: |s, on| s.force_speed = on },
    Check { id: IDC_FORCE_PITCH, get: |s| s.force_pitch, set: |s, on| s.force_pitch = on },
    Check { id: IDC_FORCE_VOLUME, get: |s| s.force_volume, set: |s, on| s.force_volume = on },
    Check { id: IDC_VOLUME_BOOST_CHECK, get: |s| s.volume_boost, set: |s, on| s.volume_boost = on },
    Check { id: IDC_SPELLING_SOUNDS_CHECK, get: |s| s.spelling_mode == SpellingMode::LetterSounds,
            set: |s, on| s.spelling_mode = if on { SpellingMode::LetterSounds } else { SpellingMode::LetterNames } },
    Check { id: IDC_EMOJI_CHECK, get: |s| s.emoji_enabled, set: |s, on| s.emoji_enabled = on },
    Check { id: IDC_DIGITS_CHECK, get: |s| s.number_mode == NumberMode::DigitByDigit,
            set: |s, on| s.number_mode = if on { NumberMode::DigitByDigit } else { NumberMode::WholeNumbers } },
    Check { id: IDC_USER_DICT_CHECK, get: |s| s.user_dictionaries_enabled, set: |s, on| s.user_dictionaries_enabled = on },
];

/// A voice as the combo box lists it: the registry's name with its kind,
/// "Josip (recorded voice)", "Zvonko (formant voice, default)".
fn voice_label(voice: &Voice) -> String {
    let kind = match (voice.is_formant(), voice.base_voice_id.is_some()) {
        (true, true) => IDS_VOICE_KIND_SINGING,
        (true, false) => IDS_VOICE_KIND_FORMANT,
        (false, false) => IDS_VOICE_KIND_RECORDED,
        (false, true) if voice.age == Age::Child => IDS_VOICE_KIND_CHILD,
        (false, true) if voice.gender == Gender::Female => IDS_VOICE_KIND_GRANDMA,
        (false, true) => IDS_VOICE_KIND_GRANDPA,
    };
    let mut label = format!("{} ({}", voice.display_name, win32::string(kind));
    if voice.id == voice::DEFAULT_VOICE_ID {
        label += &format!(", {}", win32::string(IDS_VOICE_DEFAULT));
    }
    label + ")"
}

/// The words per minute the top of a screen reader's rate slider reaches with
/// the reference formant voice: the slider's 2.0x scales the Laprdus rate
/// (the Laprdus rate alone when it is forced), the acceleration multiplies
/// that, up to the fastest rate. All three in percent.
fn top_wpm(speed: i32, forced: bool, acceleration: i32) -> i32 {
    let speed = from_percent(speed);
    let rate = if forced { speed } else { (2.0 * speed).min(FORMANT_SPEED_MAX) };
    let top = (rate * from_percent(acceleration)).min(FORMANT_EFFECTIVE_SPEED_MAX);
    (FORMANT_NOMINAL_WPM * top).round() as i32
}

/// The SAPI5 token the Test button speaks through. The tokens are one per
/// language ("Laprdus Croatian", "Laprdus Serbian" ...); the voice they speak
/// with is the one saved in settings.json. The token of the wanted language,
/// else any Laprdus token.
fn test_token(tokens: &[VoiceToken], language: &str) -> Option<usize> {
    let ours = |token: &VoiceToken| token.id.contains("Laprdus");
    let speaks = |token: &VoiceToken| token.language.as_deref().is_some_and(|code| code.eq_ignore_ascii_case(language));
    tokens.iter().position(|token| ours(token) && speaks(token)).or_else(|| tokens.iter().position(ours))
}

/// Shows the settings dialog; true when it was closed with OK.
pub fn run(speech_language: &'static str) -> bool {
    let mut config = UserConfig::new();
    config.load_settings();
    let dialog = Settings { config: RefCell::new(config), dark: Cell::new(dark_mode::enabled()), speech_language };
    win32::run_dialog(IDD_CONFIG_DIALOG, None, &dialog) == IDOK as isize
}

struct Settings {
    config: RefCell<UserConfig>,
    dark: Cell<bool>,
    /// The language of the SAPI5 token the Test button speaks through.
    speech_language: &'static str,
}

impl Settings {
    fn init(&self, dialog: Window) {
        dialog.set_text(&win32::string(IDS_APP_TITLE));
        set_texts(dialog, &TEXTS);
        let voices = dialog.item(IDC_VOICE_COMBO);
        for voice in VOICES {
            voices.add_choice(&voice_label(voice));
        }
        for slider in &SLIDERS {
            let control = dialog.item(slider.slider);
            control.set_slider_range(slider.range.0, slider.range.1);
            control.set_slider_ticks(slider.ticks);
        }

        // Without a saved voice the default is Zvonko; an id of an earlier
        // version (djed, klapa, pjevac ...) selects the renamed voice.
        let settings = self.config.borrow().settings.clone();
        let selected = voice::find(&settings.default_voice).unwrap_or_else(voice::default_voice);
        voices.choose(VOICES.iter().position(|voice| voice.id == selected.id).unwrap_or(0));
        self.update_voice_ranges(dialog);
        for slider in &SLIDERS {
            dialog.item(slider.slider).set_slider((slider.get)(&settings));
        }
        for check in &CHECKS {
            dialog.set_checked(check.id, (check.get)(&settings));
        }
        for slider in &SLIDERS {
            self.show_value(dialog, slider);
        }

        if self.dark.get() {
            dark_mode::apply(dialog, true);
        }
        dialog.center_on_parent();
    }

    /// The combo box lists `VOICES` in their order.
    fn selected_voice(dialog: Window) -> &'static Voice {
        let index = dialog.item(IDC_VOICE_COMBO).choice();
        index.and_then(|index| VOICES.get(index).copied()).unwrap_or_else(voice::default_voice)
    }

    /// The speed and pitch sliders follow the selected voice. A position
    /// outside the new range is pulled back inside it.
    fn update_voice_ranges(&self, dialog: Window) {
        let (min, max) = if Self::selected_voice(dialog).is_formant() { FORMANT_RANGE } else { RECORDED_RANGE };
        for slider in SLIDERS.iter().filter(|slider| slider.unit == Unit::Times) {
            let control = dialog.item(slider.slider);
            let position = control.slider();
            control.set_slider_range(min, max);
            control.set_slider(position.clamp(min, max));
            self.show_value(dialog, slider);
        }
        self.show_top_rate(dialog);
    }

    /// Writes a slider's position next to it.
    fn show_value(&self, dialog: Window, slider: &Slider) {
        let position = dialog.item(slider.slider).slider();
        let value = match slider.unit {
            Unit::Times => text::localized(&text::times(position)),
            Unit::Percent => text::percent(position),
            Unit::Milliseconds => format!("{position} ms"),
            Unit::TopRate => return self.show_top_rate(dialog),
        };
        dialog.item(slider.value).set_text(&value);
    }

    /// "1,5x (up to 525 WPM)" next to the acceleration; it depends on the
    /// rate and on whether the rate is forced.
    fn show_top_rate(&self, dialog: Window) {
        let acceleration = dialog.item(IDC_ACCELERATION_SLIDER).slider();
        let speed = dialog.item(IDC_SPEED_SLIDER).slider();
        let wpm = top_wpm(speed, dialog.checked(IDC_FORCE_SPEED), acceleration);
        let format = win32::string(IDS_VALUE_WPM);
        let format = if format.is_empty() { "%.1fx (up to %d WPM)" } else { &format };
        let value = text::printf(format, &[Number::Float(from_percent(acceleration)), Number::Int(wpm)]);
        dialog.item(IDC_ACCELERATION_VALUE).set_text(&text::localized(&value));
    }

    /// Takes the settings from the controls. The four voice quality values
    /// have no controls here: they stay as the voice quality dialog left them.
    fn collect(&self, dialog: Window) {
        let mut settings = self.config.borrow().settings.clone();
        settings.default_voice = Self::selected_voice(dialog).id.to_owned();
        for slider in &SLIDERS {
            (slider.set)(&mut settings, dialog.item(slider.slider).slider());
        }
        for check in &CHECKS {
            (check.set)(&mut settings, dialog.checked(check.id));
        }
        self.config.borrow_mut().settings = settings;
    }

    fn save(&self, dialog: Window) {
        self.collect(dialog);
        self.config.borrow().save_settings();
    }

    /// Saves the settings, which the SAPI5 driver reads when it notices the
    /// file changed, and speaks the test text through the token of the
    /// configurator's own language.
    fn test(&self, dialog: Window) {
        self.save(dialog);
        win32::speak(&win32::string(IDS_TEST_TEXT), &|tokens| test_token(tokens, self.speech_language));
    }

    /// The pending edits are collected first so that OK, which collects them
    /// again, keeps what the voice quality dialog changed.
    fn voice_quality(&self, dialog: Window) {
        self.collect(dialog);
        let mut settings = self.config.borrow().settings.clone();
        let values = QUALITY.each_ref().map(|quality| *(quality.field)(&mut settings));
        if let Some(values) = edit_voice_quality(dialog, values) {
            let mut config = self.config.borrow_mut();
            for (quality, value) in QUALITY.iter().zip(values) {
                *(quality.field)(&mut config.settings) = value;
            }
        }
    }
}

impl Dialog for Settings {
    fn event(&self, dialog: Window, event: Event) -> Reply {
        match event {
            Event::Init => self.init(dialog),
            Event::Command { id: IDOK, .. } => {
                self.save(dialog);
                dialog.end(IDOK);
            }
            Event::Command { id: IDCANCEL, .. } | Event::Close => dialog.end(IDCANCEL),
            Event::Command { id: IDC_APPLY_BUTTON, .. } => self.save(dialog),
            Event::Command { id: IDC_TEST_BUTTON, .. } => self.test(dialog),
            Event::Command { id: IDC_DICT_BUTTON, .. } => dictionary_dialog::run(dialog),
            Event::Command { id: IDC_VOICE_QUALITY_BUTTON, .. } => self.voice_quality(dialog),
            Event::Command { id: IDC_FORCE_SPEED, code: BN_CLICKED } => self.show_top_rate(dialog),
            Event::Command { id: IDC_VOICE_COMBO, code: CBN_SELCHANGE } => self.update_voice_ranges(dialog),
            Event::Command { id, .. } if id == IDC_VOICE_COMBO || CHECKS.iter().any(|check| check.id == id) => {}
            Event::Scroll { id } => {
                if let Some(slider) = SLIDERS.iter().find(|slider| slider.slider == id) {
                    self.show_value(dialog, slider);
                    if id == IDC_SPEED_SLIDER {
                        self.show_top_rate(dialog);
                    }
                }
            }
            Event::Color { of, dc } => {
                let kinds = [ColorOf::Static, ColorOf::Button, ColorOf::Dialog];
                return dark_mode::color(self.dark.get(), &kinds, of, dc);
            }
            // Windows was switched between its dark and light theme
            Event::SettingChange(setting) if setting == "ImmersiveColorSet" => {
                let dark = dark_mode::enabled();
                if self.dark.replace(dark) != dark {
                    dark_mode::apply(dialog, dark);
                }
            }
            _ => return Reply::Unhandled,
        }
        Reply::Handled
    }
}

// --- the voice quality dialog ---
//
// Head size, breathiness, roughness and brightness (0-100 each, 50 = the
// voice as it is). A dialog of its own because the settings dialog is already
// as tall as a screen. OK hands the sliders back, Cancel nothing.

struct Quality {
    label: u16,
    slider: u16,
    value: u16,
    string: u16,
    field: fn(&mut UserSettings) -> &mut i32,
}

#[rustfmt::skip]
const QUALITY: [Quality; 4] = [
    Quality { label: IDC_HEAD_SIZE_LABEL, slider: IDC_HEAD_SIZE_SLIDER, value: IDC_HEAD_SIZE_VALUE,
              string: IDS_HEAD_SIZE_LABEL, field: |s| &mut s.head_size },
    Quality { label: IDC_BREATHINESS_LABEL, slider: IDC_BREATHINESS_SLIDER, value: IDC_BREATHINESS_VALUE,
              string: IDS_BREATHINESS_LABEL, field: |s| &mut s.breathiness },
    Quality { label: IDC_ROUGHNESS_LABEL, slider: IDC_ROUGHNESS_SLIDER, value: IDC_ROUGHNESS_VALUE,
              string: IDS_ROUGHNESS_LABEL, field: |s| &mut s.roughness },
    Quality { label: IDC_BRIGHTNESS_LABEL, slider: IDC_BRIGHTNESS_SLIDER, value: IDC_BRIGHTNESS_VALUE,
              string: IDS_BRIGHTNESS_LABEL, field: |s| &mut s.brightness },
];

/// Opens the voice quality dialog on four values in the order of `QUALITY`;
/// the edited values when the user pressed OK.
fn edit_voice_quality(parent: Window, values: [i32; 4]) -> Option<[i32; 4]> {
    let dialog = VoiceQuality { values: Cell::new(values), dark: dark_mode::enabled() };
    (win32::run_dialog(IDD_VOICE_QUALITY_DIALOG, Some(parent), &dialog) == IDOK as isize).then(|| dialog.values.get())
}

struct VoiceQuality {
    values: Cell<[i32; 4]>,
    dark: bool,
}

impl VoiceQuality {
    fn show_values(dialog: Window) {
        for quality in &QUALITY {
            dialog.item(quality.value).set_text(&text::percent(dialog.item(quality.slider).slider()));
        }
    }
}

impl Dialog for VoiceQuality {
    fn event(&self, dialog: Window, event: Event) -> Reply {
        match event {
            Event::Init => {
                dialog.set_text(&win32::string(IDS_VOICE_QUALITY_TITLE));
                set_texts(
                    dialog,
                    &[
                        (IDC_VOICE_QUALITY_HELP, IDS_VOICE_QUALITY_HELP),
                        (IDC_VOICE_QUALITY_RESTORE, IDS_VOICE_QUALITY_RESTORE),
                        (IDOK, IDS_OK),
                        (IDCANCEL, IDS_CANCEL),
                    ],
                );
                for (quality, value) in QUALITY.iter().zip(self.values.get()) {
                    set_texts(dialog, &[(quality.label, quality.string)]);
                    let slider = dialog.item(quality.slider);
                    slider.set_slider_range(VOICE_QUALITY_MIN, VOICE_QUALITY_MAX);
                    slider.set_slider_ticks(10);
                    slider.set_slider(value.clamp(VOICE_QUALITY_MIN, VOICE_QUALITY_MAX));
                }
                Self::show_values(dialog);
                if self.dark {
                    dark_mode::apply(dialog, true);
                }
                dialog.center_on_parent();
            }
            Event::Command { id: IDOK, .. } => {
                self.values.set(QUALITY.each_ref().map(|quality| dialog.item(quality.slider).slider()));
                dialog.end(IDOK);
            }
            Event::Command { id: IDCANCEL, .. } | Event::Close => dialog.end(IDCANCEL),
            // Every slider back to 50, the voice as it is
            Event::Command { id: IDC_VOICE_QUALITY_RESTORE, .. } => {
                for quality in &QUALITY {
                    dialog.item(quality.slider).set_slider(VOICE_QUALITY_DEFAULT);
                }
                Self::show_values(dialog);
            }
            Event::Scroll { .. } => Self::show_values(dialog),
            Event::Color { of, dc } => {
                let kinds = [ColorOf::Static, ColorOf::Button, ColorOf::Dialog];
                return dark_mode::color(self.dark, &kinds, of, dc);
            }
            _ => return Reply::Unhandled,
        }
        Reply::Handled
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn token(id: &str, language: Option<&str>) -> VoiceToken {
        VoiceToken { id: id.into(), language: language.map(Into::into) }
    }

    #[test]
    fn the_test_button_speaks_through_the_token_of_its_language() {
        let tokens = [
            token("HKLM\\...\\Tokens\\Other", Some("sr")),
            token("HKLM\\...\\Tokens\\LaprdusCroatian", Some("hr")),
            token("HKLM\\...\\Tokens\\LaprdusSerbian", Some("SR")),
            token("HKLM\\...\\Tokens\\LaprdusOld", None),
        ];
        assert_eq!(test_token(&tokens, "hr"), Some(1));
        assert_eq!(test_token(&tokens, "sr"), Some(2));
        assert_eq!(test_token(&tokens, "sl"), Some(1)); // any Laprdus token
        assert_eq!(test_token(&tokens[..1], "sr"), None); // SAPI's default voice
    }

    #[test]
    fn the_top_rate_follows_rate_force_and_acceleration() {
        assert_eq!(top_wpm(100, false, 100), 350); // the host's 2.0x
        assert_eq!(top_wpm(100, true, 100), 175); // the Laprdus rate alone
        assert_eq!(top_wpm(100, false, 150), 525);
        assert_eq!(top_wpm(300, false, 100), 700); // the host's slider stops at 4.0x
        assert_eq!(top_wpm(400, true, 300), 1400); // the fastest rate, 8.0x
    }

    #[test]
    fn sliders_and_settings_agree() {
        let mut settings = UserSettings { speed: 1.15, volume: 0.29, ..UserSettings::default() };
        assert_eq!((SLIDERS[0].get)(&settings), 115);
        assert_eq!((SLIDERS[4].get)(&settings), (0.29f32 * 100.0) as i32);
        for (slider, position) in SLIDERS.iter().zip([125, 300, 25, 0, 80, 2000, 0, 150, 20]) {
            (slider.set)(&mut settings, position);
            assert_eq!((slider.get)(&settings), position);
        }
        assert_eq!((settings.speed, settings.acceleration, settings.user_pitch), (1.25, 3.0, 0.25));
        for check in &CHECKS {
            for on in [true, false] {
                (check.set)(&mut settings, on);
                assert_eq!((check.get)(&settings), on);
            }
        }
        assert_eq!(settings.spelling_mode, SpellingMode::LetterNames);
        assert_eq!(settings.number_mode, NumberMode::WholeNumbers);
    }

    #[test]
    fn voices_are_listed_in_the_order_of_the_registry() {
        assert_eq!(VOICES.len(), 18);
        assert_eq!(VOICES[5].id, voice::DEFAULT_VOICE_ID);
    }
}
