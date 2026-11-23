//! The user's settings and dictionaries on desktop systems: `settings.json`,
//! `user.json`, `spelling.json`, `emoji.json` and `accents.json` in
//! `%APPDATA%\Laprdus` (Windows) or `~/.config/Laprdus`.

use crate::params::{
    ACCELERATION_DEFAULT, FORMANT_SPEED_MAX, FORMANT_SPEED_MIN, FORMANT_USER_PITCH_MAX, FORMANT_USER_PITCH_MIN,
    INFLECTION_LEVEL_DEFAULT, NumberMode, PauseSettings, SPELLING_SPEED_DEFAULT, SPELLING_SPEED_MAX,
    SPELLING_SPEED_MIN, SpellingMode, VOICE_QUALITY_DEFAULT, VoiceParams,
};
use serde_json::Value;
use std::path::{Path, PathBuf};
use std::time::SystemTime;

/// What `settings.json` holds.
#[derive(Clone, Debug, PartialEq)]
pub struct UserSettings {
    pub speed: f32,
    pub user_pitch: f32,
    pub volume: f32,
    pub volume_boost: bool,
    pub inflection_level: f32,
    pub acceleration: f32,
    pub head_size: i32,
    pub breathiness: i32,
    pub roughness: i32,
    pub brightness: i32,
    pub emoji_enabled: bool,
    pub number_mode: NumberMode,
    pub pauses: PauseSettings,
    pub spelling_mode: SpellingMode,
    pub spelling_speed: i32,
    /// The voice (empty = Zvonko). Every voice speaks every language; the
    /// language is the host's choice (SAPI5 token, NVDA setting, CLI -L).
    pub default_voice: String,
    // The force switches: use the Laprdus value instead of the host's.
    pub force_speed: bool,
    pub force_pitch: bool,
    pub force_volume: bool,
    /// Apply the user's dictionaries during synthesis.
    pub user_dictionaries_enabled: bool,
}

impl Default for UserSettings {
    fn default() -> Self {
        Self::from_voice_params(&VoiceParams::default())
    }
}

impl UserSettings {
    /// The voice character pitch is not a setting: the voice definition has it.
    pub fn to_voice_params(&self) -> VoiceParams {
        let mut params = VoiceParams {
            speed: self.speed,
            pitch: 1.0,
            user_pitch: self.user_pitch,
            volume: self.volume,
            volume_boost: self.volume_boost,
            inflection_level: self.inflection_level,
            acceleration: self.acceleration,
            emoji_enabled: self.emoji_enabled,
            number_mode: self.number_mode,
            pause_settings: self.pauses,
            spelling_mode: self.spelling_mode,
            spelling_speed: self.spelling_speed,
            head_size: self.head_size,
            breathiness: self.breathiness,
            roughness: self.roughness,
            brightness: self.brightness,
        };
        params.clamp();
        params
    }

    pub fn from_voice_params(params: &VoiceParams) -> Self {
        Self {
            speed: params.speed,
            user_pitch: params.user_pitch,
            volume: params.volume,
            volume_boost: params.volume_boost,
            inflection_level: params.inflection_level,
            acceleration: params.acceleration,
            head_size: params.head_size,
            breathiness: params.breathiness,
            roughness: params.roughness,
            brightness: params.brightness,
            emoji_enabled: params.emoji_enabled,
            number_mode: params.number_mode,
            pauses: params.pause_settings,
            spelling_mode: params.spelling_mode,
            spelling_speed: params.spelling_speed,
            default_voice: String::new(),
            force_speed: false,
            force_pitch: false,
            force_volume: false,
            user_dictionaries_enabled: true,
        }
    }

    /// Reads the content of a `settings.json`; `None` when it is not JSON. A
    /// key that is missing keeps its default.
    pub fn parse(json: &str) -> Option<Self> {
        let file: Value =
            serde_json::from_str(json).or_else(|_| serde_json::from_str(&without_trailing_commas(json))).ok()?;
        let at = |section: &str, key: &str| file.get(section).and_then(|section| section.get(key));
        let number =
            |section, key, default: f32| at(section, key).and_then(Value::as_f64).map_or(default, |n| n as f32);
        let flag = |section, key, default| at(section, key).and_then(Value::as_bool).unwrap_or(default);
        let text = |section, key| at(section, key).and_then(Value::as_str).unwrap_or_default();
        let quality = |key| number("speech", key, VOICE_QUALITY_DEFAULT as f32).round() as i32;
        let pause = |key, default: u32| (number("pauses", key, default as f32) as u32).min(PauseSettings::MAX_MS);

        let mut settings = Self {
            default_voice: text("voice", "default").to_owned(),
            speed: number("speech", "speed", 1.0).clamp(FORMANT_SPEED_MIN, FORMANT_SPEED_MAX),
            user_pitch: number("speech", "pitch", 1.0).clamp(FORMANT_USER_PITCH_MIN, FORMANT_USER_PITCH_MAX),
            volume: number("speech", "volume", 1.0).clamp(0.0, 1.0),
            volume_boost: flag("speech", "volume_boost", false),
            inflection_level: number("speech", "inflection_level", INFLECTION_LEVEL_DEFAULT),
            acceleration: number("speech", "acceleration", ACCELERATION_DEFAULT),
            // Voice quality (November 2026); a file without the keys keeps the voice as it is.
            head_size: quality("head_size"),
            breathiness: quality("breathiness"),
            roughness: quality("roughness"),
            brightness: quality("brightness"),
            emoji_enabled: flag("speech", "emoji", false),
            number_mode: if text("numbers", "mode") == "digits" {
                NumberMode::DigitByDigit
            } else {
                NumberMode::WholeNumbers
            },
            pauses: PauseSettings {
                sentence_pause_ms: pause("sentence", 100),
                comma_pause_ms: pause("comma", 100),
                newline_pause_ms: pause("newline", 100),
                spelling_pause_ms: pause("spelling", 200),
            },
            spelling_speed: (number("spelling", "speed", SPELLING_SPEED_DEFAULT as f32) as i32)
                .clamp(SPELLING_SPEED_MIN, SPELLING_SPEED_MAX),
            // Only an explicit "names" turns the sounds off.
            spelling_mode: if text("spelling", "mode") == "names" {
                SpellingMode::LetterNames
            } else {
                SpellingMode::LetterSounds
            },
            force_speed: flag("force", "speed", false),
            force_pitch: flag("force", "pitch", false),
            force_volume: flag("force", "volume", false),
            user_dictionaries_enabled: flag("dictionaries", "user_enabled", true),
        };

        // A file of version 1.0 (written before the inflection level existed)
        // meant something else by its speed and pitch: they were ignored
        // unless forced, while now they scale the host's rate and pitch, so
        // unforced ones go back to normal instead of changing the rate after
        // the upgrade. Its "inflection" switch controlled a different effect
        // (a pitch jump at the punctuation) and is not read: the inflection
        // level starts at its default, the natural melody.
        if at("speech", "inflection_level").is_none() {
            if !settings.force_speed {
                settings.speed = 1.0;
            }
            if !settings.force_pitch {
                settings.user_pitch = 1.0;
            }
        }
        Some(settings)
    }

    /// The content of `settings.json` for these settings, with its help
    /// section.
    pub fn to_json(&self) -> String {
        let text = |value: &str| Value::from(value).to_string();
        let words = |digits| if digits { "\"digits\"" } else { "\"words\"" };
        let values: [(&str, String); 22] = [
            ("{user_enabled}", self.user_dictionaries_enabled.to_string()),
            ("{speed}", self.speed.to_string()),
            ("{pitch}", self.user_pitch.to_string()),
            ("{volume}", self.volume.to_string()),
            ("{volume_boost}", self.volume_boost.to_string()),
            ("{inflection_level}", self.inflection_level.to_string()),
            ("{acceleration}", self.acceleration.to_string()),
            ("{head_size}", self.head_size.to_string()),
            ("{breathiness}", self.breathiness.to_string()),
            ("{roughness}", self.roughness.to_string()),
            ("{brightness}", self.brightness.to_string()),
            ("{emoji}", self.emoji_enabled.to_string()),
            ("{number_mode}", words(self.number_mode == NumberMode::DigitByDigit).to_owned()),
            ("{sentence}", self.pauses.sentence_pause_ms.to_string()),
            ("{comma}", self.pauses.comma_pause_ms.to_string()),
            ("{newline}", self.pauses.newline_pause_ms.to_string()),
            ("{spelling_pause}", self.pauses.spelling_pause_ms.to_string()),
            ("{spelling_speed}", self.spelling_speed.to_string()),
            (
                "{spelling_mode}",
                text(if self.spelling_mode == SpellingMode::LetterSounds { "sounds" } else { "names" }),
            ),
            ("{force_speed}", self.force_speed.to_string()),
            ("{force_pitch}", self.force_pitch.to_string()),
            ("{force_volume}", self.force_volume.to_string()),
        ];
        let json = values.iter().fold(SETTINGS_TEMPLATE.to_owned(), |json, (key, value)| json.replace(key, value));
        // Last, so that nothing in a voice id is taken for a placeholder.
        json.replace("{voice}", &text(&self.default_voice))
    }
}

/// JSON without the commas a hand edit leaves before a closing brace or
/// bracket, the usual way a settings file stops being JSON.
fn without_trailing_commas(json: &str) -> String {
    let mut out = String::with_capacity(json.len());
    let (mut in_string, mut escaped) = (false, false);
    for (at, c) in json.char_indices() {
        if in_string {
            in_string = escaped || c != '"';
            escaped = !escaped && c == '\\';
        } else if c == '"' {
            in_string = true;
        } else if c == ',' && json[at + 1..].trim_start().starts_with(['}', ']']) {
            continue;
        }
        out.push(c);
    }
    out
}

const SETTINGS_TEMPLATE: &str = r#"{
    "version": "2.0",
    "description": "LaprdusTTS user settings",

    "_help": {
        "voice.default": "Voice ID: zvonko (default), stojan, mirsad, josip, vlado, detence, baba, djedo, or a singing preset orguljas, dalmacija_klapa, trubac, harmonikas, sevdalija, sazlija, lika_velebit, solunac, sarajevo_bembasa, becarac. Every voice speaks every language; the language comes from the host (SAPI5 voice token, NVDA language setting, CLI -L)",
        "speech.speed": "Speech rate from 0.5 (slow) to 2.0 (fast), default 1.0; formant voices accept 0.25 to 4.0",
        "speech.pitch": "Voice pitch from 0.5 (low) to 2.0 (high), default 1.0; formant voices accept 0.25 to 4.0",
        "speech.volume": "Volume from 0.0 (silent) to 1.0 (full), default 1.0",
        "speech.volume_boost": "Louder speech through a peak limiter, for noisy places and a phone's earpiece (true/false, default false)",
        "speech.inflection_level": "Size of the pitch movements: 0.0 (monotone) to 1.0 (maximum), default 0.5",
        "speech.acceleration": "Rate multiplier from 0.5 to 3.0, default 1.0 (2.0 doubles the rate the rate slider reaches)",
        "speech.head_size": "Voice quality: length of the vocal tract, 0 (40 % longer, formants x0.71) to 100 (30 % shorter, formants x1.4), default 50 = the voice as it is",
        "speech.breathiness": "Voice quality: breath in the formant voices, 0 (a pressed, hard voice without breath) to 100 (a whisper), default 50 = the voice as it is",
        "speech.roughness": "Voice quality: jitter and shimmer of the formant voices, 0 (none, and no slow drift of the pitch: a perfectly steady voice) to 100 (eight times the voice's own), above 50 also a rasp (every second cycle shorter and weaker), default 50",
        "speech.brightness": "Voice quality: tone of the whole sound, 0 (dull, with softer s and sh) to 100 (sharp), default 50 = the voice as it is",
        "speech.emoji": "Convert emoji to spoken text descriptions (true/false)",
        "numbers.mode": "Number reading: 'words' (twenty-three) or 'digits' (two-three)",
        "pauses.sentence": "Pause after sentences (. ! ?) in milliseconds, 0-2000",
        "pauses.comma": "Pause after commas in milliseconds, 0-2000",
        "pauses.newline": "Pause at newlines in milliseconds, 0-2000",
        "pauses.spelling": "Pause between spelled characters in milliseconds, 0-2000",
        "spelling.speed": "Lower values add longer pauses after each character. Characters always use the normal speech rate. 100% adds no extra pause; default 50%.",
        "spelling.mode": "How letters are spelled: 'sounds' (the sound of each letter; default) or 'names' (be, ce, de)",
        "force.speed": "Use Laprdus speed setting instead of system/SAPI5 (true/false)",
        "force.pitch": "Use Laprdus pitch setting instead of system/SAPI5 (true/false)",
        "force.volume": "Use Laprdus volume setting instead of system/SAPI5 (true/false)",
        "dictionaries.user_enabled": "Apply user dictionaries (user.json, spelling.json, emoji.json, accents.json) during synthesis (true/false)"
    },

    "voice": {
        "default": {voice}
    },

    "speech": {
        "speed": {speed},
        "pitch": {pitch},
        "volume": {volume},
        "volume_boost": {volume_boost},
        "inflection_level": {inflection_level},
        "acceleration": {acceleration},
        "head_size": {head_size},
        "breathiness": {breathiness},
        "roughness": {roughness},
        "brightness": {brightness},
        "emoji": {emoji}
    },

    "numbers": {
        "mode": {number_mode}
    },

    "pauses": {
        "sentence": {sentence},
        "comma": {comma},
        "newline": {newline},
        "spelling": {spelling_pause}
    },

    "spelling": {
        "speed": {spelling_speed},
        "mode": {spelling_mode}
    },

    "force": {
        "speed": {force_speed},
        "pitch": {force_pitch},
        "volume": {force_volume}
    },

    "dictionaries": {
        "user_enabled": {user_enabled}
    }
}
"#;

/// The configuration directory and the settings read from it.
#[derive(Clone, Debug, Default)]
pub struct UserConfig {
    dir: Option<PathBuf>,
    pub settings: UserSettings,
}

impl UserConfig {
    pub const SETTINGS: &'static str = "settings.json";
    pub const DICTIONARY: &'static str = "user.json";
    pub const SPELLING: &'static str = "spelling.json";
    pub const EMOJI: &'static str = "emoji.json";
    pub const ACCENTS: &'static str = "accents.json";
    /// Every file of the configuration, in the order of
    /// [`modified_times`](Self::modified_times).
    pub const FILES: [&'static str; 5] = [Self::SETTINGS, Self::DICTIONARY, Self::SPELLING, Self::EMOJI, Self::ACCENTS];

    pub fn new() -> Self {
        Self { dir: Self::directory(), settings: UserSettings::default() }
    }

    /// The configuration of another directory than the system's.
    pub fn in_directory(dir: impl Into<PathBuf>) -> Self {
        Self { dir: Some(dir.into()), settings: UserSettings::default() }
    }

    /// When each of [`FILES`](Self::FILES) was last changed; `None` for one
    /// that is not there. A host compares them to see that the user changed
    /// the settings or a dictionary.
    pub fn modified_times(&self) -> [Option<SystemTime>; 5] {
        Self::FILES.map(|name| {
            let path = self.path(name)?;
            std::fs::metadata(path).and_then(|file| file.modified()).ok()
        })
    }

    /// The configuration directory of the system: `%APPDATA%\Laprdus` on
    /// Windows, else `$XDG_CONFIG_HOME/Laprdus` or `~/.config/Laprdus`.
    pub fn directory() -> Option<PathBuf> {
        let variable = |name| std::env::var_os(name).filter(|value| !value.is_empty()).map(PathBuf::from);
        let base = if cfg!(windows) {
            variable("APPDATA")
        } else {
            variable("XDG_CONFIG_HOME").or_else(|| Some(std::env::home_dir()?.join(".config")))
        };
        Some(base?.join("Laprdus"))
    }

    pub fn dir(&self) -> Option<&Path> {
        self.dir.as_deref()
    }

    /// The path of a file in the configuration directory.
    pub fn path(&self, file_name: &str) -> Option<PathBuf> {
        Some(self.dir.as_ref()?.join(file_name))
    }

    pub fn exists(&self, file_name: &str) -> bool {
        self.path(file_name).is_some_and(|path| path.exists())
    }

    fn ensure_directory(&self) -> Option<&Path> {
        self.dir.as_deref().filter(|dir| std::fs::create_dir_all(dir).is_ok())
    }

    /// Reads `settings.json`, writing one with the defaults when there is
    /// none or it is not a JSON object. Without a usable directory the
    /// defaults stay. False only when a default file could not be written.
    pub fn load_settings(&mut self) -> bool {
        let Some(path) = self.ensure_directory().map(|dir| dir.join(Self::SETTINGS)) else { return true };
        let content = std::fs::read_to_string(&path).unwrap_or_default();
        let content = content.trim_start_matches('\u{FEFF}');
        if content.trim_start().starts_with('{') {
            // A file that only looks like JSON keeps its content: the
            // defaults are used, and the user's edit is not thrown away.
            self.settings = UserSettings::parse(content).unwrap_or_default();
            return true;
        }
        self.settings = UserSettings::default();
        self.save_settings()
    }

    /// Writes `settings.json`.
    pub fn save_settings(&self) -> bool {
        self.ensure_directory()
            .is_some_and(|dir| std::fs::write(dir.join(Self::SETTINGS), self.settings.to_json()).is_ok())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn settings_survive_a_round_trip() {
        let settings = UserSettings {
            default_voice: "vlado".into(),
            speed: 1.5,
            user_pitch: 0.75,
            volume_boost: true,
            head_size: 70,
            number_mode: NumberMode::DigitByDigit,
            spelling_mode: SpellingMode::LetterNames,
            spelling_speed: 20,
            force_pitch: true,
            user_dictionaries_enabled: false,
            pauses: PauseSettings {
                sentence_pause_ms: 250,
                comma_pause_ms: 40,
                newline_pause_ms: 0,
                spelling_pause_ms: 300,
            },
            ..UserSettings::default()
        };
        assert_eq!(UserSettings::parse(&settings.to_json()), Some(settings));
        assert_eq!(UserSettings::parse(&UserSettings::default().to_json()), Some(UserSettings::default()));
    }

    #[test]
    fn a_file_of_version_1_keeps_the_hosts_rate() {
        let old = r#"{"version": "1.0", "speech": {"speed": 1.8, "pitch": 1.3, "inflection": false},
                      "pauses": {"comma": 5000,}, "force": {"speed": true}, }"#;
        let settings = UserSettings::parse(old).unwrap();
        assert_eq!(
            (settings.speed, settings.user_pitch, settings.inflection_level),
            (1.8, 1.0, INFLECTION_LEVEL_DEFAULT)
        );
        assert_eq!(settings.pauses.comma_pause_ms, 2000);
        assert_eq!(UserSettings::parse("no json"), None);
    }
}
