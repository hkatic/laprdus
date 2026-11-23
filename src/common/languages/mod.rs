//! The languages Laprdus speaks, one folder each.
//!
//! Everything a language needs lives under its code: the text front end
//! (letter-to-sound rules, stress, lexicon), the number words and their
//! grammar, the names of the letters and symbols, and its voices (`formant`,
//! `concatenative`). The folders do not use one another: Croatian, Serbian
//! and Bosnian each have their own copy of the rules they have in common.
//!
//! A language tells the engine what it has by implementing [`Rules`]. Adding
//! a language is adding its folder here, its name to
//! [`crate::language::Language`], and one line to [`rules`].

pub mod bs;
pub mod hr;
pub mod sl;
pub mod sr;

#[cfg(test)]
mod tests;

use crate::language::Language;
use crate::prosody::Utterance;
use crate::text::Punctuation;
use crate::text::clauses::Clause;
use crate::text::frontend::{LanguageWords, UserLexicon};
use crate::text::numbers::Grammar;
use std::sync::Arc;

/// What a language gives the engine.
pub(crate) trait Rules: Sync {
    /// A front end of the language: one clause of text to phones.
    fn frontend(&self) -> Box<dyn LanguageFrontend>;

    /// The names of the letters and symbols and the short words of the
    /// language, for the shared tokenizer and the spelling of letters.
    fn words(&self) -> &'static dyn LanguageWords;

    /// The number words and the grammar of numbers in a sentence.
    fn numbers(&self) -> &'static dyn Grammar;

    /// The language's own name of a digit, mark or symbol when it is
    /// spelled, where it has one; the bundled spelling dictionary, whose
    /// names are Croatian, names the rest.
    fn spelled_name(&self, _character: &str) -> Option<&'static str> {
        None
    }

    /// Marks the clauses of a text for what the front end, which sees one
    /// clause at a time, cannot see.
    fn mark_clauses(&self, _clauses: &mut [Clause]) {}

    /// What is wrong with a verb entry of the user's accent lexicon, if the
    /// language cannot use it.
    fn check_verb_entry(&self, entry: &[char]) -> Result<(), &'static str>;
}

/// A language's front end, with the user's accent entries.
pub(crate) trait LanguageFrontend: crate::text::Frontend + Send + Sync {
    /// Uses the user's accent entries for the language; `None` removes them.
    fn set_user_lexicon(&mut self, lexicon: Option<Arc<UserLexicon>>);
}

/// The rules of a language.
pub(crate) fn rules(language: Language) -> &'static dyn Rules {
    match language {
        Language::Croatian => &hr::Croatian,
        Language::Serbian => &sr::Serbian,
        Language::Bosnian => &bs::Bosnian,
        Language::Slovenian => &sl::Slovenian,
    }
}

/// The text front end of the language spoken.
pub struct Frontend {
    rules: &'static dyn Rules,
    frontend: Box<dyn LanguageFrontend>,
}

impl Frontend {
    pub fn new(language: Language) -> Self {
        let rules = rules(language);
        Self { rules, frontend: rules.frontend() }
    }

    /// Uses the user's accent entries for this language; `None` removes them.
    pub fn set_user_lexicon(&mut self, lexicon: Option<Arc<UserLexicon>>) {
        self.frontend.set_user_lexicon(lexicon);
    }

    /// The language's own name of a digit, mark or symbol when it is
    /// spelled, where it has one; the bundled spelling dictionary, whose
    /// names are Croatian, names the rest.
    pub fn spelled_name(&self, character: &str) -> Option<&'static str> {
        self.rules.spelled_name(character)
    }

    /// Marks the clauses of a text for what the front end, which sees one
    /// clause at a time, does not see, where the language needs that.
    pub fn mark_clauses(&self, clauses: &mut [Clause]) {
        self.rules.mark_clauses(clauses);
    }
}

impl crate::text::Frontend for Frontend {
    fn process(&self, text: &[char], punct: Punctuation, letter_sounds: bool) -> Utterance {
        self.frontend.process(text, punct, letter_sounds)
    }

    fn letter_sound(&self, letter: &[char]) -> Utterance {
        self.frontend.letter_sound(letter)
    }
}
