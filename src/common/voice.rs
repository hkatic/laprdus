//! Voices as a host lists them, and the list of all of them.
//!
//! A voice's `language` is its native one, spoken until the host asks for
//! another: every voice speaks every language. Display names carry no
//! language and no "Laprdus" prefix: the platforms add what they need. Ids
//! are plain ASCII (dj for đ, c for č and ć, s for š, z for ž) so that no
//! registry, settings file or locale object ever has to carry a diacritic.
//! The voices themselves are defined by their languages
//! (`languages/<code>/formant`, `languages/<code>/concatenative`).

use crate::formant::voice::FormantVoice;
use crate::language::Language;
use crate::languages::{bs, hr, sr};
use crate::params::FORMANT_NOMINAL_WPM;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Gender {
    Male,
    Female,
}

impl Gender {
    pub fn as_str(self) -> &'static str {
        match self {
            Self::Male => "Male",
            Self::Female => "Female",
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Age {
    Child,
    Adult,
    Senior,
}

impl Age {
    pub fn as_str(self) -> &'static str {
        match self {
            Self::Child => "Child",
            Self::Adult => "Adult",
            Self::Senior => "Senior",
        }
    }
}

/// How a voice produces sound.
#[derive(Clone, Copy, Debug, PartialEq)]
pub enum Synthesis {
    /// Recorded sounds from a packed data file, rendered by TD-PSOLA.
    Concatenative {
        data_file: &'static str,
        /// Words per minute at speed 1.0, measured on the same text as the
        /// formant voices.
        nominal_wpm: f32,
    },
    /// Synthesized by rule; no data file.
    Formant(&'static FormantVoice),
}

/// One voice of the engine.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Voice {
    /// Internal id: "josip", "djedo", "lika_velebit".
    pub id: &'static str,
    /// Name shown to the user: "Josip", "Đedo", "Dalmacija Klapa".
    pub display_name: &'static str,
    /// Native language (the default; any language may be set).
    pub language: Language,
    pub gender: Gender,
    pub age: Age,
    /// The voice a derived voice or a singing preset is made of.
    pub base_voice_id: Option<&'static str>,
    /// Pitch multiplier of the voice's character: 1.0, 1.5 (child), 1.2, 0.75.
    pub base_pitch: f32,
    pub synthesis: Synthesis,
}

impl Voice {
    /// A recorded voice with a data file of its own.
    pub const fn recorded(
        id: &'static str,
        name: &'static str,
        language: Language,
        data_file: &'static str,
        nominal_wpm: f32,
    ) -> Self {
        Self {
            id,
            display_name: name,
            language,
            gender: Gender::Male,
            age: Age::Adult,
            base_voice_id: None,
            base_pitch: 1.0,
            synthesis: Synthesis::Concatenative { data_file, nominal_wpm },
        }
    }

    /// A voice derived from this recorded one by a pitch of its character.
    pub const fn derived(
        &self,
        id: &'static str,
        name: &'static str,
        gender: Gender,
        age: Age,
        base_pitch: f32,
    ) -> Self {
        Self { id, display_name: name, gender, age, base_voice_id: Some(self.id), base_pitch, ..*self }
    }

    /// A formant voice.
    pub const fn formant(speaker: &'static FormantVoice, name: &'static str, language: Language) -> Self {
        Self {
            id: speaker.id,
            display_name: name,
            language,
            gender: Gender::Male,
            age: Age::Adult,
            base_voice_id: None,
            base_pitch: 1.0,
            synthesis: Synthesis::Formant(speaker),
        }
    }

    /// A singing preset of this formant voice.
    pub const fn preset(&self, speaker: &'static FormantVoice, name: &'static str) -> Self {
        Self { base_voice_id: Some(self.id), ..Self::formant(speaker, name, self.language) }
    }

    pub fn is_formant(&self) -> bool {
        matches!(self.synthesis, Synthesis::Formant(_))
    }

    /// The data file of a recorded voice (a derived voice has its base
    /// voice's).
    pub fn data_file(&self) -> Option<&'static str> {
        match self.synthesis {
            Synthesis::Concatenative { data_file, .. } => Some(data_file),
            Synthesis::Formant(_) => None,
        }
    }

    /// Words per minute at speed 1.0 and acceleration 1.0, for showing rate
    /// limits in words per minute.
    pub fn nominal_wpm(&self) -> f32 {
        match self.synthesis {
            Synthesis::Concatenative { nominal_wpm, .. } => nominal_wpm,
            Synthesis::Formant(speaker) => FORMANT_NOMINAL_WPM * speaker.tempo,
        }
    }
}

/// Every voice, in the order hosts list them. New voices are appended: code
/// and saved settings rely on the order.
pub static VOICES: [&Voice; 18] = [
    &hr::concatenative::JOSIP,
    &sr::concatenative::VLADO,
    &hr::concatenative::DETENCE,
    &hr::concatenative::BABA,
    &sr::concatenative::DJEDO,
    &hr::formant::voices::ZVONKO,
    &sr::formant::voices::STOJAN,
    &bs::formant::voices::MIRSAD,
    &hr::formant::voices::ORGULJAS,
    &hr::formant::voices::DALMACIJA_KLAPA,
    &sr::formant::voices::TRUBAC,
    &sr::formant::voices::HARMONIKAS,
    &bs::formant::voices::SEVDALIJA,
    &bs::formant::voices::SAZLIJA,
    &hr::formant::voices::LIKA_VELEBIT,
    &sr::formant::voices::SOLUNAC,
    &bs::formant::voices::SARAJEVO_BEMBASA,
    &hr::formant::voices::BECARAC,
];

/// The voice of every language and platform until the user chooses another.
pub const DEFAULT_VOICE_ID: &str = "zvonko";

/// Ids of earlier versions, still accepted so that saved settings and
/// scripts keep working.
const ALIASES: [(&str, &str); 7] = [
    ("djed", "djedo"),
    ("dijete", "detence"),
    ("baka", "baba"),
    ("klapa", "dalmacija_klapa"),
    ("pjevac", "lika_velebit"),
    ("pevac", "solunac"),
    ("solist", "sarajevo_bembasa"),
];

/// The voice with an id, or with the id an earlier version knew it by.
pub fn find(id: &str) -> Option<&'static Voice> {
    let id = ALIASES.iter().find(|(old, _)| *old == id).map_or(id, |&(_, id)| id);
    VOICES.iter().copied().find(|voice| voice.id == id)
}

/// The default voice.
pub fn default_voice() -> &'static Voice {
    find(DEFAULT_VOICE_ID).unwrap_or(VOICES[0])
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn registry() {
        let ids: Vec<&str> = VOICES.iter().map(|voice| voice.id).collect();
        assert_eq!(
            ids,
            [
                "josip",
                "vlado",
                "detence",
                "baba",
                "djedo",
                "zvonko",
                "stojan",
                "mirsad",
                "orguljas",
                "dalmacija_klapa",
                "trubac",
                "harmonikas",
                "sevdalija",
                "sazlija",
                "lika_velebit",
                "solunac",
                "sarajevo_bembasa",
                "becarac"
            ]
        );
        assert!(ids.iter().all(|id| id.is_ascii()));
        assert_eq!(default_voice().id, "zvonko");
        assert_eq!(find("djed").map(|voice| voice.display_name), Some("Đedo"));
        assert_eq!(find("pjevac").map(|voice| (voice.id, voice.base_voice_id)), Some(("lika_velebit", Some("zvonko"))));
        assert_eq!(find("baba").map(|voice| (voice.data_file(), voice.base_pitch)), Some((Some("Josip.bin"), 1.2)));
        assert_eq!(find("nobody"), None);
        assert_eq!((find("vlado").unwrap().nominal_wpm(), find("zvonko").unwrap().nominal_wpm()), (97.0, 175.0));
    }
}
