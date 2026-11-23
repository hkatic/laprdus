//! The languages a voice can speak.
//!
//! Every voice speaks every language: the language (letter-to-sound rules,
//! accent lexicon, letter names, number words) is chosen separately from the
//! voice. A voice's own language is only the one it speaks until the host
//! asks for another.

/// A language of the engine. The order is that of the C API's language list.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub enum Language {
    #[default]
    Croatian,
    Serbian,
    Bosnian,
    Slovenian,
}

impl Language {
    pub const ALL: [Language; 4] = [Self::Croatian, Self::Serbian, Self::Bosnian, Self::Slovenian];

    /// Language and region: "hr-HR", "sr-RS", "bs-BA", "sl-SI".
    pub fn code(self) -> &'static str {
        ["hr-HR", "sr-RS", "bs-BA", "sl-SI"][self as usize]
    }

    /// ISO 639-1 code alone: "hr", "sr", "bs", "sl".
    pub fn short_code(self) -> &'static str {
        &self.code()[..2]
    }

    /// Windows language identifier (Serbian and Bosnian: Latin script).
    pub fn lcid(self) -> u16 {
        [0x041A, 0x081A, 0x141A, 0x0424][self as usize]
    }

    pub fn english_name(self) -> &'static str {
        ["Croatian", "Serbian", "Bosnian", "Slovenian"][self as usize]
    }

    /// The language's own name of itself (Latin script).
    pub fn native_name(self) -> &'static str {
        ["Hrvatski", "Srpski", "Bosanski", "Slovenščina"][self as usize]
    }

    /// Parses a language tag or name in any case: "hr", "hr-HR", "hr_HR",
    /// "hrv", "sr-Latn-RS", "sr-Cyrl", "srp", "bs", "bos", "sl", "sl-SI",
    /// "slv", "Croatian", "hrvatski", "Slovenian", "Slovene", "slovenščina".
    /// Only the first subtag of a tag counts.
    pub fn parse(tag: &str) -> Option<Self> {
        const NAMES: [(&str, Language); 12] = [
            ("croatian", Language::Croatian),
            ("hrvatski", Language::Croatian),
            ("serbian", Language::Serbian),
            ("srpski", Language::Serbian),
            ("bosnian", Language::Bosnian),
            ("bosanski", Language::Bosnian),
            ("slovenian", Language::Slovenian),
            ("slovene", Language::Slovenian),
            ("slovenski", Language::Slovenian),
            ("slovensko", Language::Slovenian),
            ("slovenscina", Language::Slovenian),
            ("slovenščina", Language::Slovenian),
        ];
        let subtag = tag.split(['-', '_', '.', '@']).next().unwrap_or_default().to_ascii_lowercase();
        match subtag.as_str() {
            "hr" | "hrv" => Some(Self::Croatian),
            "sr" | "srp" => Some(Self::Serbian),
            "bs" | "bos" => Some(Self::Bosnian),
            "sl" | "slv" => Some(Self::Slovenian),
            code if code.len() == 2 || code.len() == 3 => None,
            _ => {
                let name = tag.to_ascii_lowercase();
                NAMES.iter().find(|(known, _)| *known == name).map(|&(_, language)| language)
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::Language::{self, *};

    #[test]
    fn tags_and_names() {
        for (tag, language) in [
            ("hr", Croatian),
            ("HR-hr", Croatian),
            ("hrv", Croatian),
            ("sr-Latn-RS", Serbian),
            ("sr_RS.UTF-8", Serbian),
            ("srp", Serbian),
            ("bs", Bosnian),
            ("BOS", Bosnian),
            ("sl-SI", Slovenian),
            ("slv", Slovenian),
            ("Croatian", Croatian),
            ("hrvatski", Croatian),
            ("Slovene", Slovenian),
            ("slovenščina", Slovenian),
        ] {
            assert_eq!(Language::parse(tag), Some(language), "{tag}");
        }
        for tag in ["", "en", "bo", "srb", "sla", "deutsch", "hrvatski-HR"] {
            assert_eq!(Language::parse(tag), None, "{tag}");
        }
        assert_eq!((Slovenian.short_code(), Slovenian.lcid(), Bosnian.code()), ("sl", 0x0424, "bs-BA"));
    }
}
