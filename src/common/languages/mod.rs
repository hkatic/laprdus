//! The languages Laprdus speaks, one folder each.
//!
//! Everything a language needs lives under its code: the text front end
//! (letter-to-sound rules, stress, lexicon), the number words, the letter
//! names, and its voices (`formant`, `concatenative`). Adding a language is
//! adding a folder here and one line to the list.

pub mod bcms;
pub mod bs;
pub mod hr;
pub mod sl;
pub mod sr;

use crate::language::Language;
use crate::prosody::Utterance;
use crate::text::Punctuation;
use crate::text::frontend::UserLexicon;
use std::sync::Arc;

/// The text front end of the language spoken.
pub struct Frontend {
    language: Language,
    rules: bcms::Frontend,
}

impl Frontend {
    pub fn new(language: Language) -> Self {
        Self { language, rules: bcms::Frontend::new(language) }
    }

    /// Uses the user's accent entries for this language; `None` removes them.
    pub fn set_user_lexicon(&mut self, lexicon: Option<Arc<UserLexicon>>) {
        self.rules.set_user_lexicon(lexicon);
    }

    /// The language's own name of a digit, mark or symbol when it is
    /// spelled, where it has one; the bundled spelling dictionary, whose
    /// names are Croatian, names the rest.
    pub fn spelled_name(&self, character: &str) -> Option<&'static str> {
        match self.language {
            Language::Slovenian => sl::words::spelled_name(character),
            _ => None,
        }
    }
}

impl crate::text::Frontend for Frontend {
    fn process(&self, text: &[char], punct: Punctuation, letter_sounds: bool) -> Utterance {
        self.rules.process(text, punct, letter_sounds)
    }

    fn letter_sound(&self, letter: &[char]) -> Utterance {
        self.rules.letter_sound(letter)
    }
}
