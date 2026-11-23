//! Letters: case, accent marks, foreign letters, and the small helpers the
//! rules use to look at a word as a row of letters.

use crate::formant::phones::Ph;
use crate::language::Language;
use crate::text::cyrillic;

// Accent marks found in the text (and in lexicon entries), one byte per
// letter. Shared by every language's letter-to-sound code.
/// The letter carries the stress.
pub const MARK_STRESS: u8 = 1;
/// The vowel is long.
pub const MARK_LONG: u8 = 2;
/// Falling accent (Slovenian: an open long e/o, ê ô).
pub const MARK_FALL: u8 = 4;
/// Rising accent (Slovenian: acute = close long e/o, grave = short).
pub const MARK_RISE: u8 = 8;

/// Lowercase of the Latin letters the languages and their neighbours use
/// (Basic Latin, Latin-1, Latin Extended-A and the letters with double grave
/// and inverted breve); anything else comes back as it is.
pub fn to_lower(c: char) -> char {
    let u = u32::from(c);
    let lower = match u {
        0x41..=0x5A => u + 32,
        0xC0..=0xDE if u != 0xD7 => u + 32,
        0x100..=0x137 | 0x14A..=0x177 | 0x200..=0x217 => u | 1,
        0x139..=0x148 | 0x179..=0x17E => u + (u & 1),
        _ => u,
    };
    char::from_u32(lower).unwrap_or(c)
}

pub fn is_vowel_letter(c: char) -> bool {
    matches!(c, 'a' | 'e' | 'i' | 'o' | 'u')
}

/// A lowercase letter of the alphabets: a to z and č, ć, đ, š, ž.
pub fn is_base_letter(c: char) -> bool {
    c.is_ascii_lowercase() || matches!(c, 'č' | 'ć' | 'đ' | 'š' | 'ž')
}

pub fn is_consonant_letter(c: char) -> bool {
    is_base_letter(c) && !is_vowel_letter(c)
}

const RISE: u8 = MARK_STRESS | MARK_RISE;
const LONG_RISE: u8 = MARK_STRESS | MARK_RISE | MARK_LONG;
const FALL: u8 = MARK_STRESS | MARK_FALL;
const LONG_FALL: u8 = MARK_STRESS | MARK_FALL | MARK_LONG;

/// Accent marks as used in dictionaries and grammars: grave = short rising,
/// acute = long rising, double grave = short falling, inverted breve or
/// circumflex = long falling, macron = unstressed length. The letter, its
/// base letter and its marks.
const ACCENTED: [(char, char, u8); 33] = [
    ('à', 'a', RISE),
    ('è', 'e', RISE),
    ('ì', 'i', RISE),
    ('ò', 'o', RISE),
    ('ù', 'u', RISE),
    ('á', 'a', LONG_RISE),
    ('é', 'e', LONG_RISE),
    ('í', 'i', LONG_RISE),
    ('ó', 'o', LONG_RISE),
    ('ú', 'u', LONG_RISE),
    ('ŕ', 'r', LONG_RISE),
    ('â', 'a', LONG_FALL),
    ('ê', 'e', LONG_FALL),
    ('î', 'i', LONG_FALL),
    ('ô', 'o', LONG_FALL),
    ('û', 'u', LONG_FALL),
    ('ȁ', 'a', FALL),
    ('ȅ', 'e', FALL),
    ('ȉ', 'i', FALL),
    ('ȍ', 'o', FALL),
    ('ȕ', 'u', FALL),
    ('ȑ', 'r', FALL),
    ('ȃ', 'a', LONG_FALL),
    ('ȇ', 'e', LONG_FALL),
    ('ȋ', 'i', LONG_FALL),
    ('ȏ', 'o', LONG_FALL),
    ('ȗ', 'u', LONG_FALL),
    ('ȓ', 'r', LONG_FALL),
    ('ā', 'a', MARK_LONG),
    ('ē', 'e', MARK_LONG),
    ('ī', 'i', MARK_LONG),
    ('ō', 'o', MARK_LONG),
    ('ū', 'u', MARK_LONG),
];

/// Letters of other alphabets and the digraph ligatures, with the letters
/// they are read as.
const FOREIGN: [(char, &str); 38] = [
    ('ä', "e"),
    ('ö', "e"),
    ('ü', "i"),
    ('ë', "e"),
    ('ï', "i"),
    ('ñ', "nj"),
    ('ß', "s"),
    ('ç', "s"),
    ('ł', "l"),
    ('ń', "nj"),
    ('ś', "s"),
    ('ź', "z"),
    ('ż', "ž"),
    ('ř', "rž"),
    ('ě', "e"),
    ('ů', "u"),
    ('ý', "i"),
    ('ň', "nj"),
    ('ť', "t"),
    ('ď', "d"),
    ('ľ', "l"),
    ('ő', "e"),
    ('ű', "i"),
    ('å', "o"),
    ('ø', "e"),
    ('æ', "e"),
    ('ã', "a"),
    ('õ', "o"),
    ('ÿ', "i"),
    ('ǆ', "dž"),
    ('ǅ', "dž"),
    ('Ǆ', "dž"),
    ('ǉ', "lj"),
    ('ǈ', "lj"),
    ('Ǉ', "lj"),
    ('ǌ', "nj"),
    ('ǋ', "nj"),
    ('Ǌ', "nj"),
];

/// The base letter and the marks of a lowercase letter written with an
/// accent mark.
pub fn accented(c: char) -> Option<(char, u8)> {
    ACCENTED.iter().find(|&&(letter, ..)| letter == c).map(|&(_, base, mark)| (base, mark))
}

/// What a lowercase letter of another alphabet is read as.
pub fn foreign(c: char) -> Option<&'static str> {
    FOREIGN.iter().find(|&&(letter, _)| letter == c).map(|&(_, replacement)| replacement)
}

/// The marks of a combining accent (U+0300 grave ... U+0311 inverted breve),
/// 0 for any other character.
pub fn combining_mark(c: char) -> u8 {
    match c {
        '\u{300}' => RISE,
        '\u{301}' => LONG_RISE,
        '\u{302}' | '\u{311}' => LONG_FALL,
        '\u{30F}' => FALL,
        '\u{304}' => MARK_LONG,
        _ => 0,
    }
}

// ---- A word as letters ----

pub fn chars(s: &str) -> Vec<char> {
    s.chars().collect()
}

pub fn eq(w: &[char], s: &str) -> bool {
    w.iter().copied().eq(s.chars())
}

pub fn is_one_of(w: &[char], list: &[&str]) -> bool {
    list.iter().any(|s| eq(w, s))
}

pub fn starts_with(w: &[char], prefix: &str) -> bool {
    let mut letters = w.iter();
    prefix.chars().all(|c| letters.next() == Some(&c))
}

pub fn ends_with(w: &[char], suffix: &str) -> bool {
    let mut letters = w.iter().rev();
    suffix.chars().rev().all(|c| letters.next() == Some(&c))
}

pub fn ends_with_any(w: &[char], suffixes: &[&str]) -> bool {
    suffixes.iter().any(|suffix| ends_with(w, suffix))
}

/// Whether the letters from `pos` on begin with `part`; false past the end.
pub fn has_at(w: &[char], pos: usize, part: &str) -> bool {
    w.get(pos..).is_some_and(|rest| starts_with(rest, part))
}

/// The first place at or after `from` where `part` stands.
pub fn find_from(w: &[char], part: &str, from: usize) -> Option<usize> {
    (from..=w.len()).find(|&pos| has_at(w, pos, part))
}

pub fn find(w: &[char], part: &str) -> Option<usize> {
    find_from(w, part, 0)
}

/// The letters from `pos` on; empty past the end.
pub fn tail(w: &[char], pos: usize) -> &[char] {
    w.get(pos..).unwrap_or_default()
}

/// A vowel letter or y.
pub fn has_vowel(w: &[char]) -> bool {
    w.iter().any(|&c| is_vowel_letter(c) || c == 'y')
}

/// r carries the syllable when it has no vowel on either side (prst, vrt,
/// krv, rt, žanr), and in vowel + rđ (zarđati). Before j it never does: rj is
/// the short jat after r (rječnik, rješenje, pogrješka).
pub fn is_syllabic_r(w: &[char], i: usize) -> bool {
    if w.len() < 2 {
        return false;
    }
    let prev = i.checked_sub(1).and_then(|at| w.get(at)).copied().unwrap_or('\0');
    let next = w.get(i + 1).copied().unwrap_or('\0');
    if is_vowel_letter(next) || next == 'r' || next == 'j' || prev == 'r' {
        return false;
    }
    if is_vowel_letter(prev) {
        return next == 'đ';
    }
    true
}

/// A syllabic r anywhere but at the end of the word (prst, vrt, rt; not dr).
pub fn has_inner_syllabic_r(w: &[char]) -> bool {
    (0..w.len().saturating_sub(1)).any(|i| w[i] == 'r' && is_syllabic_r(w, i))
}

/// The consonant a letter stands for; `Ph::Sil` for anything else.
pub fn consonant_phone(c: char) -> Ph {
    match c {
        'b' => Ph::B,
        'c' => Ph::C,
        'č' => Ph::Ch,
        'ć' => Ph::Tj,
        'd' => Ph::D,
        'đ' => Ph::Dj,
        'f' => Ph::F,
        'g' => Ph::G,
        'h' => Ph::H,
        'j' => Ph::J,
        'k' => Ph::K,
        'l' => Ph::L,
        'm' => Ph::M,
        'n' => Ph::N,
        'p' => Ph::P,
        'r' => Ph::R,
        's' => Ph::S,
        'š' => Ph::Sh,
        't' => Ph::T,
        'v' => Ph::V,
        'z' => Ph::Z,
        'ž' => Ph::Zh,
        _ => Ph::Sil,
    }
}

/// The vowel a letter stands for; `Ph::Sil` for anything else.
pub fn vowel_phone(c: char) -> Ph {
    match c {
        'a' => Ph::A,
        'e' => Ph::E,
        'i' => Ph::I,
        'o' => Ph::O,
        'u' => Ph::U,
        _ => Ph::Sil,
    }
}

// ---- Spelling ----

/// The letter a character stands for when text is spelled: lowercase Latin,
/// Cyrillic transliterated, a ligature or Cyrillic љ/њ/џ as its digraph
/// ("B" -> "b", "Љ" -> "lj", "ǆ" -> "dž", "ä" -> "e"). Empty for a character
/// that is not a letter of the alphabet (digits, punctuation, symbols).
pub fn spelling_letter(c: char) -> Vec<char> {
    let mut letter = Vec::new();
    cyrillic::push_latin(c, &mut letter);
    let mut out = Vec::new();
    for c in letter.into_iter().map(to_lower) {
        if is_base_letter(c) {
            out.push(c);
        } else if let Some((base, _)) = accented(c) {
            out.push(base);
        } else if let Some(replacement) = foreign(c) {
            out.extend(replacement.chars());
        } else {
            return Vec::new();
        }
    }
    // Only a letter of the alphabet, alone or as one of the digraphs
    if out.len() == 1 || is_one_of(&out, &["lj", "nj", "dž"]) { out } else { Vec::new() }
}

/// True for q, w, x and y, the letters outside the Croatian, Serbian and
/// Bosnian alphabets. They have no sound of their own in the language, so
/// spelling reads them by their names (ku, iks, ipsilon) also when the other
/// letters are spelled by their sounds; w is duplo ve, or duplo v with the
/// sound of the v, by the front end.
pub fn is_foreign_letter(letter: &[char]) -> bool {
    is_one_of(letter, &["q", "w", "x", "y"])
}

/// The name of a letter of the alphabet in the language ("b" -> "be",
/// "lj" -> "elj" or "lje", "x" -> "iks"); empty for anything else. The
/// Croatian and Bosnian names follow the Croatian orthography (a, be, ce,
/// če, će, de, dže, đe, e, ef, ge, ha, i, je, ka, el, elj, em, en, enj, o,
/// pe, er, es, eš, te, u, ve, ze, že), the Serbian ones decision 41 of the
/// Board for the Standardization of the Serbian Language (lje, nje, ša; the
/// rest the same for Cyrillic and Latin), the Slovenian ones the Slovenski
/// pravopis (the same names for its 25 letters; dvojni ve for w).
pub fn letter_name(letter: &[char], language: Language) -> Vec<char> {
    chars(letter_name_in(letter, language))
}

/// [`letter_name`] as the table has it.
pub(crate) fn letter_name_in(letter: &[char], language: Language) -> &'static str {
    crate::languages::rules(language).words().letter_name(letter)
}

/// True when the text, lowercased, is a letter's name in any of the languages.
pub fn is_letter_name(letter: &[char], text: &[char]) -> bool {
    let lower: Vec<char> = text.iter().map(|&c| to_lower(c)).collect();
    // Trim spaces around it
    let Some(begin) = lower.iter().position(|&c| c != ' ') else { return false };
    let end = lower.iter().rposition(|&c| c != ' ').unwrap_or(begin);
    let lower = &lower[begin..=end];
    Language::ALL.iter().any(|&language| eq(lower, letter_name_in(letter, language)))
        // The name the bundled dictionary used to give j
        || (eq(letter, "j") && eq(lower, "jot"))
}
