//! What the front ends of all the languages share: the word model and the
//! tokenizer, the letters, the lexicon notation, the user's accent lexicon,
//! and the flattening of a clause's words into one string of phones.
//!
//! A language's front end (`crate::languages`) supplies the letter-to-sound
//! rules, the stress and the clitics between the tokenizer and the
//! flattening, and tells the shared code what it needs to know of the
//! language through [`LanguageWords`].

pub mod letters;
pub mod notation;
pub mod sounds;
pub mod tokenizer;
pub mod user_lexicon;
pub mod utterance;

#[cfg(test)]
mod tests;

pub use letters::{is_foreign_letter, is_letter_name, letter_name, spelling_letter};
pub use notation::LexEntry;
pub use user_lexicon::{Report, UserLexicon};

use crate::formant::phones::{Accent, Ph, Phone};

/// What the tokenizer and the reading of letters and abbreviations ask of a
/// language.
pub trait LanguageWords {
    /// The name of a symbol read wherever it stands inside a clause (".",
    /// ":", "%").
    fn symbol_name(&self, c: char) -> Option<&'static str>;
    /// The name of a letter ("b" -> "be"); empty for anything else.
    fn letter_name(&self, letter: &[char]) -> &'static str;
    fn is_proclitic(&self, w: &[char]) -> bool;
    fn is_enclitic(&self, w: &[char]) -> bool;
    /// A preposition of one consonant, which leans on the next word (s, k).
    fn is_consonant_preposition(&self, w: &[char]) -> bool;
    /// A real word of two letters, vowel and consonant, which in capitals is
    /// not an abbreviation (ON, IZ).
    fn is_short_word(&self, w: &[char]) -> bool;
}

/// No real word is this long; splitting keeps one runaway token from
/// becoming a single enormous prosodic word.
pub const MAX_WORD_LETTERS: usize = 48;

/// One word of a clause on its way from letters to phones.
#[derive(Clone, Debug, Default)]
pub struct Word {
    /// Plain lowercase letters.
    pub w: Vec<char>,
    /// Accent marks per letter.
    pub marks: Vec<u8>,
    pub all_caps: bool,
    /// Written with a capital first letter ("Pula").
    pub capitalized: bool,

    pub phones: Vec<Phone>,
    /// Letter index of each phone.
    pub letter: Vec<usize>,
    /// Phone index of each syllable nucleus.
    pub nuclei: Vec<usize>,
    /// Index into `nuclei`.
    pub stress: Option<usize>,
    pub accent: Accent,
    /// 0 clitic, 1 function word, 2 content word.
    pub prominence: u8,
    pub clitic: bool,
    pub explicit_stress: bool,
    /// Do not reinterpret a user's lexical accent.
    pub user_accent: bool,
    /// Part of a spelled-out abbreviation.
    pub letter_name: bool,
    /// A lone letter spoken by its sound ("Obrisano B").
    pub letter_sound: bool,
    /// A letter's name that keeps its accent (www, https, w).
    pub own_accent: bool,
    /// The first letter of an abbreviation written as one word.
    pub word_start: bool,
    /// One word with the previous ("ne znam", tiflo-).
    pub joined: bool,
    /// A letter of a dotted abbreviation ("s.a.r.s.").
    pub spell: bool,
    /// A hyphen follows directly ("s-klasa").
    pub before_hyphen: bool,
    /// After an opening quote or bracket ("„S tobom").
    pub after_opening: bool,
    /// Spelled with x, q, w or y ("Wi" in Wi-Fi).
    pub foreign: bool,
    /// Part of a word with inner capitals ("ElevenLabs").
    pub camel: bool,
    /// ... written together with the part before it.
    pub camel_join: bool,
    /// Read as an English word.
    pub english: bool,
    /// The letters of a designation glued to a number ("A17", "AB12").
    pub code: bool,
    /// A short pause stands before the word (`MINOR_BREAK_MARK`).
    pub break_before: bool,
    /// ... the longer one between two items (`ITEM_BREAK_MARK`).
    pub item_break: bool,
    /// The letters as written, before x, q, w, y are rewritten.
    pub spelled: Vec<char>,
    /// Letter of `spelled` each letter of `w` comes from.
    pub origin: Vec<usize>,
}

impl Word {
    pub fn new(text: &[char]) -> Self {
        Self { w: text.to_vec(), marks: vec![0; text.len()], prominence: 2, ..Self::default() }
    }

    /// An empty word, as the tokenizer starts one.
    fn empty() -> Self {
        Self::new(&[])
    }

    /// Whether an accent mark was written on any letter.
    pub fn is_marked(&self) -> bool {
        self.marks.iter().any(|&mark| mark != 0)
    }

    pub fn add_phone(&mut self, ph: Ph, letter: usize, nucleus: bool) {
        if nucleus {
            self.nuclei.push(self.phones.len());
        }
        self.phones.push(Phone { ph, nucleus, ..Phone::default() });
        self.letter.push(letter);
    }

    /// The phone of syllable `k`.
    pub fn nucleus_mut(&mut self, k: usize) -> Option<&mut Phone> {
        self.nuclei.get(k).and_then(|&at| self.phones.get_mut(at))
    }

    pub fn set_long(&mut self, k: usize, long: bool) {
        if let Some(phone) = self.nucleus_mut(k) {
            phone.is_long = long;
        }
    }

    /// Takes the length from every syllable.
    pub fn shorten(&mut self) {
        for k in 0..self.nuclei.len() {
            self.set_long(k, false);
        }
    }

    /// The letter of `w` each syllable's nucleus comes from.
    pub fn nucleus_letters(&self) -> Vec<usize> {
        self.nuclei.iter().filter_map(|&at| self.letter.get(at).copied()).collect()
    }
}
