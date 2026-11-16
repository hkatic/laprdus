//! Slovenian letters to phones for one word, and the sound changes across a
//! clause.
//!
//! Declarations only: the bodies are placeholders until the Slovenian rules
//! are ported from `formant_slovenian.cpp`.

use super::lexicon::Lexicon;
use crate::formant::phones::Phone;

/// One word turned into phones. `phones` carry `ph`, `nucleus`, `is_long`
/// and `nucleus_tail`.
#[derive(Clone, Debug, Default)]
pub struct WordPhones {
    pub phones: Vec<Phone>,
    /// Letter index of each phone.
    pub letter: Vec<usize>,
    /// Phone index of each syllable nucleus.
    pub nuclei: Vec<usize>,
    /// The index into `nuclei` of the stressed syllable (`None` only for a
    /// word without a vowel).
    pub stress: Option<usize>,
    /// The lexicon (or the text's marks) gave the stress.
    pub from_lexicon: bool,
}

/// How a word is written, for the rules of names (a word the lexicon does
/// not know, written with a capital, is most likely a name).
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum Capital {
    /// Small letters.
    #[default]
    None,
    /// Text written all in capitals: only the rule of the surnames in -ović.
    AllCaps,
    /// A capital at the head of a clause (or after an opening quote).
    Head,
    /// A capital inside a clause: a name.
    Inside,
}

/// Letters to phones for one word: the marks written in the text, then the
/// user's lexicon, then the built-in one, then the rules. `marks` has the
/// accent marks of each letter (`MARK_STRESS` ...). `letter_name` marks the
/// name of a letter ("be", "eš"), which the rules read with a long close
/// vowel. `pron_override`, when given, is a pronunciation in the lexicon's
/// notation used in place of the built-in lexicon's (the user's entries
/// still win): the reading of a word that depends on its place in the
/// clause. `capital` says how the word is written: a capitalized word the
/// lexicon does not know is read by the rules of names (a surname in -ović
/// is stressed before the suffix, Pétrović, Stojánović; Gólobič, Zupánčič,
/// Dolénc, Bógomir); a word with ć or đ is read by them anyway.
pub fn phonemize(
    _word: &[char],
    _marks: &[u8],
    _user: Option<&Lexicon>,
    _letter_name: bool,
    _pron_override: Option<&[char]>,
    _capital: Capital,
) -> WordPhones {
    // TODO(sl)
    WordPhones::default()
}

/// Sound changes across a clause: regressive voicing assimilation, final
/// devoicing, n before k/g, the merging of identical consonants over a
/// clitic boundary. `word_is_clitic` says, by the word index of the phones,
/// whether the word is a clitic that is spoken with its neighbour without a
/// word break.
pub fn assimilate(_phones: &mut Vec<Phone>, _word_is_clitic: &[bool]) {
    // TODO(sl)
}
