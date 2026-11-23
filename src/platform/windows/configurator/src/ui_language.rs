//! The language of the configurator's own text.
//!
//! The string tables of `laprdus_config.rc` are in Croatian, Serbian,
//! Slovenian and English. The language follows Windows's display language;
//! the NVDA add-on passes NVDA's language with `--language=<tag>` instead. A
//! language without a table of its own reads English, except Bosnian, which
//! reads Croatian: the same script and the same ijekavian forms.

use crate::win32;

pub const ENGLISH: u16 = 0x0409;
pub const CROATIAN: u16 = 0x041A;
pub const SERBIAN: u16 = 0x241A; // sr-Latn-RS
pub const SLOVENIAN: u16 = 0x0424;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct UiLanguage {
    /// The string table to read.
    pub strings: u16,
    /// The language of the SAPI5 token the Test button speaks through
    /// (Croatian when the table's language is one Laprdus does not speak).
    pub speech: &'static str,
}

const ENGLISH_UI: UiLanguage = UiLanguage { strings: ENGLISH, speech: "hr" };

/// For a language id of Windows. Croatian, Serbian and Bosnian share one
/// primary language id (0x1A), so only their whole ids tell them apart.
pub fn from_windows(id: u16) -> UiLanguage {
    match id {
        0x001A | 0x041A | 0x101A => UiLanguage { strings: CROATIAN, speech: "hr" }, // hr, hr-HR, hr-BA
        0x141A | 0x201A | 0x641A | 0x681A | 0x781A => UiLanguage { strings: CROATIAN, speech: "bs" }, // bs
        // sr-*-CS, sr-*-BA, sr-*-RS, sr-*-ME, sr-Cyrl, sr-Latn, sr
        0x081A | 0x0C1A | 0x181A | 0x1C1A | 0x241A | 0x281A | 0x2C1A | 0x301A | 0x6C1A | 0x701A | 0x7C1A => {
            UiLanguage { strings: SERBIAN, speech: "sr" }
        }
        0x0024 | 0x0424 => UiLanguage { strings: SLOVENIAN, speech: "sl" }, // sl, sl-SI
        _ => ENGLISH_UI,
    }
}

/// For a language tag: NVDA's forms ("hr", "sr_Latn", "pt_BR") and BCP 47
/// ("sr-Latn-RS"), any case. The text after the tag on a command line ends it.
pub fn from_tag(tag: &str) -> UiLanguage {
    let primary: String = tag.chars().take_while(|c| c.is_alphabetic()).flat_map(char::to_lowercase).collect();
    match primary.as_str() {
        "hr" => UiLanguage { strings: CROATIAN, speech: "hr" },
        "bs" => UiLanguage { strings: CROATIAN, speech: "bs" },
        "sr" => UiLanguage { strings: SERBIAN, speech: "sr" },
        "sl" => UiLanguage { strings: SLOVENIAN, speech: "sl" },
        _ => ENGLISH_UI,
    }
}

/// Picks the language from the command line's arguments or from Windows and
/// makes the resource loader read its string table.
pub fn select(arguments: impl IntoIterator<Item = String>) -> UiLanguage {
    const OPTION: &str = "--language=";
    let tag = arguments.into_iter().find_map(|argument| Some(argument.split_once(OPTION)?.1.to_owned()));
    let language = tag.map_or_else(|| from_windows(win32::display_language()), |tag| from_tag(&tag));
    win32::set_thread_language(language.strings);
    language
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_windows_display_language_picks_its_table() {
        let cases: [(u16, u16, &str); 17] = [
            (0x041A, CROATIAN, "hr"),  // hr-HR
            (0x101A, CROATIAN, "hr"),  // hr-BA
            (0x241A, SERBIAN, "sr"),   // sr-Latn-RS, Windows's Serbian language pack
            (0x281A, SERBIAN, "sr"),   // sr-Cyrl-RS
            (0x081A, SERBIAN, "sr"),   // sr-Latn-CS
            (0x0C1A, SERBIAN, "sr"),   // sr-Cyrl-CS
            (0x181A, SERBIAN, "sr"),   // sr-Latn-BA
            (0x1C1A, SERBIAN, "sr"),   // sr-Cyrl-BA
            (0x2C1A, SERBIAN, "sr"),   // sr-Latn-ME
            (0x301A, SERBIAN, "sr"),   // sr-Cyrl-ME
            (0x141A, CROATIAN, "bs"),  // bs-Latn-BA
            (0x201A, CROATIAN, "bs"),  // bs-Cyrl-BA
            (0x0424, SLOVENIAN, "sl"), // sl-SI
            (0x0409, ENGLISH, "hr"),   // en-US
            (0x0809, ENGLISH, "hr"),   // en-GB
            (0x0407, ENGLISH, "hr"),   // de-DE
            (0x1000, ENGLISH, "hr"),   // a language without an LCID
        ];
        for (id, strings, speech) in cases {
            assert_eq!(from_windows(id), UiLanguage { strings, speech }, "Windows {id:#06X}");
        }
    }

    #[test]
    fn a_language_tag_picks_its_table() {
        let cases: [(&str, u16, &str); 13] = [
            ("hr", CROATIAN, "hr"),
            ("HR_hr", CROATIAN, "hr"),
            ("sr", SERBIAN, "sr"),
            ("sr_Latn", SERBIAN, "sr"),
            ("sr-Cyrl-RS", SERBIAN, "sr"),
            ("bs", CROATIAN, "bs"),
            ("sl_SI", SLOVENIAN, "sl"),
            ("en", ENGLISH, "hr"),
            ("pt_BR", ENGLISH, "hr"),
            ("srp", ENGLISH, "hr"), // NVDA and BCP 47 use the two-letter codes
            ("", ENGLISH, "hr"),
            ("sr --other", SERBIAN, "sr"),
            ("verylongtag", ENGLISH, "hr"),
        ];
        for (tag, strings, speech) in cases {
            assert_eq!(from_tag(tag), UiLanguage { strings, speech }, "tag {tag:?}");
        }
    }
}
