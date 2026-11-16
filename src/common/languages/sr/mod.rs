//! Serbian: the text front end (letter-to-sound rules, stress, lexicon),
//! the numerals, the names of the letters and symbols, and the voices.

mod clause;
pub mod concatenative;
pub mod english;
pub mod formant;
pub mod frontend;
pub mod lexicon;
pub mod numerals;
pub mod phonemize;
pub mod stress;
pub mod verbs;
pub mod words;

use super::{LanguageFrontend, Rules};
use crate::text::frontend::LanguageWords;
use crate::text::numbers::Grammar;

/// Serbian, as the engine asks for it.
pub struct Serbian;

impl Rules for Serbian {
    fn frontend(&self) -> Box<dyn LanguageFrontend> {
        Box::new(frontend::Frontend::new())
    }

    fn words(&self) -> &'static dyn LanguageWords {
        &words::Words
    }

    fn numbers(&self) -> &'static dyn Grammar {
        &numerals::Numbers
    }

    fn check_verb_entry(&self, entry: &[char]) -> Result<(), &'static str> {
        verbs::VerbRoot::check_entry(entry)
    }
}
