//! The text front end of Serbian: one clause of text to phones.
//!
//! - Serbian Cyrillic is transliterated, digraphs (lj, nj, dž) and the
//!   ijekavian "ije" diphthong are resolved, syllabic r is detected
//! - stress comes from explicit accent marks in the text, the user's lexicon,
//!   the built-in lexicon, suffix rules, or the Neo-Štokavian default (first
//!   syllable)
//! - proclitics and enclitics lose their stress and join their host word
//! - voicing and place assimilation apply inside words and clitic groups

use super::clause;
use super::lexicon::{Entries, Lexicon};
use super::verbs::VerbRoot;
use crate::language::Language;
use crate::languages::LanguageFrontend;
use crate::prosody::Utterance;
use crate::text::Punctuation;
use crate::text::frontend::letters::vowel_phone;
use crate::text::frontend::user_lexicon::language_bit;
use crate::text::frontend::{UserLexicon, utterance};
use std::sync::Arc;

/// Converts one clause of text into phones.
pub struct Frontend {
    /// The built-in lexicon.
    pub(super) lexicon: &'static Lexicon,
    pub(super) user_exact: Entries,
    pub(super) user_stems: Entries,
    user_verbs: Vec<VerbRoot>,
}

impl Default for Frontend {
    fn default() -> Self {
        Self::new()
    }
}

impl Frontend {
    pub fn new() -> Self {
        Self {
            lexicon: Lexicon::builtin(),
            user_exact: Entries::new(),
            user_stems: Entries::new(),
            user_verbs: Vec::new(),
        }
    }

    /// The user's verbs, when there are any.
    pub(super) fn user_verbs(&self) -> Option<&[VerbRoot]> {
        (!self.user_verbs.is_empty()).then_some(&self.user_verbs[..])
    }
}

impl LanguageFrontend for Frontend {
    /// Use the user's accent entries for Serbian and those for every
    /// language. The user's exact forms and stems are consulted before the
    /// built-in ones, and the user's verbs before the built-in verb tables.
    /// Pass `None` to remove them. Costs one copy of the matching entries,
    /// nothing per utterance.
    fn set_user_lexicon(&mut self, lexicon: Option<Arc<UserLexicon>>) {
        self.user_exact.clear();
        self.user_stems.clear();
        self.user_verbs.clear();
        let Some(lexicon) = lexicon else { return };

        let mine = |languages: u8| languages == 0 || languages & language_bit(Language::Serbian) != 0;
        for word in lexicon.words.iter().filter(|word| mine(word.languages)) {
            let entries = if word.stem { &mut self.user_stems } else { &mut self.user_exact };
            entries.insert(word.plain.clone(), word.entry);
        }
        self.user_verbs = lexicon
            .verbs
            .iter()
            .filter(|verb| mine(verb.languages))
            .filter_map(|verb| VerbRoot::parse_entry(&verb.text))
            .collect();
    }
}

impl crate::text::Frontend for Frontend {
    fn process(&self, text: &[char], punct: Punctuation, letter_sounds: bool) -> Utterance {
        clause::process(self, text, punct, letter_sounds)
    }

    fn letter_sound(&self, letter: &[char]) -> Utterance {
        utterance::letter_sound(letter, vowel_phone)
    }
}
