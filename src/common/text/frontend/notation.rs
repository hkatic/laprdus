//! The notation of the accent lexicons of Croatian, Serbian and Bosnian:
//! UTF-8 words with marks.
//!
//! ```text
//!   '  before the stressed vowel (or syllabic r); tone chosen automatically
//!   ^  before the stressed vowel: falling accent
//!   /  before the stressed vowel: rising accent
//!   :  after a long vowel
//!   *  at the end: stem, also matches forms with up to three more letters
//!   |  stem|ending|ending: one exact entry per ending (an empty ending is the
//!      bare stem); for paradigms whose accent moves or whose forms would be
//!      taken by another word's stem
//! ```

use super::letters::{is_base_letter, is_vowel_letter, to_lower};
use crate::formant::phones::Accent;

/// One lexicon entry: where the stress is and which vowels are long.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct LexEntry {
    /// Letter index the stress mark precedes.
    pub stress_letter: Option<u8>,
    pub accent: Accent,
    /// Bit per letter index carrying length.
    pub long_letters: u32,
}

/// An entry as written: its letters, its marks, and whether it is a stem.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Parsed {
    pub plain: Vec<char>,
    pub entry: LexEntry,
    pub stem: bool,
}

/// The accent a stress mark asks for (`'` leaves the tone to the rules).
pub fn mark_accent(c: char) -> Option<Accent> {
    match c {
        '\'' => Some(Accent::None),
        '^' => Some(Accent::Falling),
        '/' => Some(Accent::Rising),
        _ => None,
    }
}

/// A lexicon entry in the notation: letters with `'`, `^` or `/` before the
/// stressed vowel, `:` after a long vowel, `*` for a stem. The reason comes
/// back for an entry that is not well formed; the built-in tables are assumed
/// to be, the user's are checked.
pub fn parse_lex_entry(marked: &[char]) -> Result<Parsed, &'static str> {
    let mut plain: Vec<char> = Vec::with_capacity(marked.len());
    let mut entry = LexEntry::default();
    let mut stem = false;
    let mut marked_any = false;

    for c in marked.iter().map(|&raw| to_lower(raw)) {
        if let Some(accent) = mark_accent(c) {
            if entry.stress_letter.is_some() {
                return Err("two stress marks");
            }
            entry.stress_letter = Some(u8::try_from(plain.len()).ok().filter(|&at| at <= 127).ok_or("word too long")?);
            entry.accent = accent;
            marked_any = true;
        } else if c == ':' {
            if !plain.last().is_some_and(|&last| is_vowel_letter(last) || last == 'r') {
                return Err("length mark not after a vowel");
            }
            if plain.len() > 32 {
                return Err("length mark past the 32nd letter");
            }
            entry.long_letters |= 1 << (plain.len() - 1);
            marked_any = true;
        } else if c == '*' {
            if stem {
                return Err("two stem marks");
            }
            stem = true;
        } else if stem {
            return Err("letters after the stem mark");
        } else if is_base_letter(c) {
            plain.push(c);
        } else {
            return Err("not a letter");
        }
    }
    if plain.is_empty() {
        return Err("no letters");
    }
    if !marked_any {
        return Err("no stress or length mark");
    }
    if entry.stress_letter.is_some_and(|at| usize::from(at) >= plain.len()) {
        return Err("stress mark after the last letter");
    }
    Ok(Parsed { plain, entry, stem })
}

/// "stem|ending|ending" lists one exact form per ending; anything else is one
/// form. Calls `f` for each.
pub fn for_each_form(marked: &[char], mut f: impl FnMut(&[char])) {
    let mut parts = marked.split(|&c| c == '|');
    let stem = parts.next().unwrap_or_default();
    if stem.len() == marked.len() {
        return f(marked);
    }
    let mut form = Vec::with_capacity(marked.len());
    for ending in parts {
        form.clear();
        form.extend_from_slice(stem);
        form.extend_from_slice(ending);
        f(&form);
    }
}
