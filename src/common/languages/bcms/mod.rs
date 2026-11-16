//! The text front end of Croatian, Serbian and Bosnian: one module for the
//! three, with the language as a parameter.
//!
//! - Serbian Cyrillic is transliterated, digraphs (lj, nj, dž) and the
//!   ijekavian "ije" diphthong are resolved, syllabic r is detected
//! - stress comes from explicit accent marks in the text, the user's lexicon,
//!   the built-in lexicon, suffix rules, or the Neo-Štokavian default (first
//!   syllable)
//! - proclitics and enclitics lose their stress and join their host word
//! - voicing and place assimilation apply inside words and clitic groups
//!
//! What differs between the three languages: the tables `lexicon/hr.rs`,
//! `sr.rs` and `bs.rs` and the tables of names in `tables/`, the names of
//! the symbols and letters in `words.rs`, and the branches on the language
//! in `stress.rs` and `clause.rs` (search for `Language::` and `croatian`).

mod clause;
pub mod english;
pub mod lexicon;
pub mod phonemize;
pub mod stress;
pub mod verbs;
pub mod words;

#[cfg(test)]
mod tests;

use crate::language::Language;
use crate::languages::sl;
use crate::prosody::Utterance;
use crate::text::Punctuation;
use crate::text::frontend::letters::vowel_phone;
use crate::text::frontend::user_lexicon::language_bit;
use crate::text::frontend::{UserLexicon, utterance};
use lexicon::{Entries, Lexicon};
use std::sync::Arc;
use verbs::VerbRoot;

/// Converts one clause of text into phones.
pub struct Frontend {
    language: Language,
    /// The built-in lexicon of the language.
    lexicon: &'static Lexicon,
    user_exact: Entries,
    user_stems: Entries,
    user_verbs: Vec<VerbRoot>,
    /// The user's lexicon as given, for its Slovenian entries.
    user: Option<Arc<UserLexicon>>,
}

impl Frontend {
    pub fn new(language: Language) -> Self {
        Self {
            language,
            lexicon: Lexicon::of(language),
            user_exact: Entries::new(),
            user_stems: Entries::new(),
            user_verbs: Vec::new(),
            user: None,
        }
    }

    /// Use the user's accent entries for this front end's language. The
    /// user's exact forms and stems are consulted before the built-in ones,
    /// and the user's verbs before the built-in verb tables. Pass `None` to
    /// remove them. Costs one copy of the matching entries, nothing per
    /// utterance.
    pub fn set_user_lexicon(&mut self, lexicon: Option<Arc<UserLexicon>>) {
        self.user_exact.clear();
        self.user_stems.clear();
        self.user_verbs.clear();
        self.user = lexicon;
        let Some(lexicon) = self.user.as_deref().filter(|_| self.language != Language::Slovenian) else { return };

        let mine = |languages: u8| languages == 0 || languages & language_bit(self.language) != 0;
        for word in lexicon.words.iter().filter(|word| mine(word.languages)) {
            let entries = if word.stem { &mut self.user_stems } else { &mut self.user_exact };
            entries.insert(word.plain.clone(), word.entry);
        }
        self.user_verbs =
            lexicon.verbs.iter().filter(|verb| mine(verb.languages)).map(|verb| verb.root.clone()).collect();
    }

    /// The user's verbs, when there are any.
    fn user_verbs(&self) -> Option<&[VerbRoot]> {
        (!self.user_verbs.is_empty()).then_some(&self.user_verbs[..])
    }

    /// The user's entries for Slovenian, when there are any.
    fn user_slovenian(&self) -> Option<&sl::lexicon::Lexicon> {
        self.user.as_deref().map(|user| &user.slovenian).filter(|lexicon| !lexicon.is_empty())
    }
}

impl crate::text::Frontend for Frontend {
    fn process(&self, text: &[char], punct: Punctuation, letter_sounds: bool) -> Utterance {
        if self.language == Language::Slovenian {
            sl::frontend::process(text, punct, letter_sounds, self.user_slovenian())
        } else {
            clause::process(self, text, punct, letter_sounds)
        }
    }

    fn letter_sound(&self, letter: &[char]) -> Utterance {
        if self.language == Language::Slovenian {
            sl::frontend::letter_sound(letter)
        } else {
            utterance::letter_sound(letter, vowel_phone)
        }
    }
}
