//! The Slovenian pronunciation lexicon.
//!
//! Declarations only: the bodies are placeholders until the Slovenian rules
//! and the generated tables are ported from `formant_slovenian.cpp` and
//! `formant_slovenian.inc`.

use std::collections::HashMap;

/// The pronunciation lexicon. An entry is the word spelled as it is
/// pronounced, in the notation of Slovenian dictionaries (SSKJ, Slovenski
/// pravopis):
///
/// ```text
///   á í ú ŕ   long stressed vowel (ŕ: the syllabic r)
///   é ó       long stressed close e, o            (mésto, góra)
///   ê ô       long stressed open e, o             (člôvek, ôkno)
///   à è ì ò ù short stressed vowel (e, o open)    (bràt, pès, otròk)
///   ə         the schwa written e                 (vétər, pəs)
///   ə̀         stressed schwa (ə + U+0300)         (pə̀s)
///   w         an l pronounced [w]                 (bíw, délaw)
///   l         an l read by the rules (before a vowel [l]; at the end and
///             before a consonant [w] in the participles, after a schwa and
///             in the words the Pravopis lists, [l] otherwise)
///   v         v: [v] before a vowel, [w] elsewhere, by the rule
///   '  :      ASCII alternatives: ' before the stressed vowel, : after a
///             long one ("m'é:sto" = mésto, "p'əs" = pəs, "'r:" = ŕ)
///   *         at the end: a stem; forms with up to five more letters are
///             the stem plus the ending read by the rules of unstressed
///             endings (d'é:la* covers délal, délala, délajo)
///   |         prefix|ending|ending: one exact form per ending, marks in
///             either part (zač|éti|éla|nèm; an empty ending is the bare prefix)
///   =         spelling=pronunciation, when the pronunciation cannot be
///             turned back into the spelling (vse=wsè is not needed: v is by
///             rule; hotel=hotél is)
/// ```
///
/// The spelling the entry is for is the entry without its marks (ə -> e,
/// w -> l). Keys are lowercase; the lexicon does not know capitals.
#[derive(Clone, Debug, Default)]
pub struct Lexicon {
    exact: HashMap<Vec<char>, Vec<char>>,
    stems: HashMap<Vec<char>, Vec<char>>,
}

/// One entry of the lexicon as [`Lexicon::parse`] reads it.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Entry {
    /// The spelling the entry is for.
    pub key: Vec<char>,
    /// The pronunciation, in the notation.
    pub pron: Vec<char>,
    /// A stem (`*`).
    pub stem: bool,
}

impl Lexicon {
    /// The longest ending a stem takes.
    pub const MAX_ENDING: usize = 5;

    /// Parses one entry (a word or a stem, not a form list); the reason for
    /// a malformed one.
    pub fn parse(_marked: &[char]) -> Result<Entry, String> {
        // TODO(sl)
        Err(String::from("the Slovenian lexicon is not available yet"))
    }

    /// Adds one entry (a word, a stem or a form list); false if malformed.
    pub fn add(&mut self, _marked: &[char]) -> bool {
        // TODO(sl)
        false
    }

    pub fn clear(&mut self) {
        self.exact.clear();
        self.stems.clear();
    }

    pub fn is_empty(&self) -> bool {
        self.exact.is_empty() && self.stems.is_empty()
    }

    /// The pronunciation of an exact form.
    pub fn find_exact(&self, _word: &[char]) -> Option<Vec<char>> {
        // TODO(sl): the hand-added entries, then the generated table of forms
        None
    }

    /// The longest stem the word starts with (an ending of up to
    /// [`Self::MAX_ENDING`] letters): its length and its pronunciation.
    /// `accept`, when given, passes only the stems whose ending it accepts.
    pub fn find_stem(&self, _word: &[char], _accept: Option<fn(&[char]) -> bool>) -> Option<(usize, Vec<char>)> {
        // TODO(sl): the hand-added stems, then the generated table of stems
        None
    }
}

/// The built-in lexicon (the generated tables of Sloleks 3.1 plus the
/// hand-written entries, which win).
pub fn builtin_lexicon() -> &'static Lexicon {
    // TODO(sl)
    static EMPTY: std::sync::LazyLock<Lexicon> = std::sync::LazyLock::new(Lexicon::default);
    &EMPTY
}
