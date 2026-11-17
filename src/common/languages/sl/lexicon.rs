//! The Slovenian pronunciation lexicon: its notation, the generated tables
//! of Sloleks 3.1 (CJVT, University of Ljubljana; CLARIN.SI,
//! hdl 11356/2080; CC BY-SA 4.0) and the hand-written entries.
//!
//! Sources (see "Slovenian" in `docs/formant.md`): SSKJ and the Slovenski
//! pravopis on Fran.si for the accents of the hand-written words, and the
//! Sloleks lexicon for the generated tables in `tables/`, which
//! `tools/formant/slovenian_words.py` writes.

use crate::text::frontend::letters::{MARK_LONG, MARK_STRESS, is_base_letter, is_vowel_letter};
use std::cmp::Ordering;
use std::collections::HashMap;
use std::sync::LazyLock;

// Per-letter flags of a word being analysed.
/// The letter carries the stress.
pub(super) const F_STRESS: u8 = MARK_STRESS;
/// The vowel is long.
pub(super) const F_LONG: u8 = MARK_LONG;
/// l: [l] (notation ł), whatever the rules would say.
pub(super) const F_L: u8 = 4;
/// e, o: close [e], [o] (acute).
pub(super) const F_CLOSE: u8 = 16;
/// e, o: open, said explicitly (circumflex, grave).
pub(super) const F_OPEN: u8 = 32;
/// e: the schwa.
pub(super) const F_SCHWA: u8 = 64;
/// l: [w] (notation w); a notation l follows the rules.
pub(super) const F_W: u8 = 128;

const COMBINING_GRAVE: char = '\u{300}';
const COMBINING_ACUTE: char = '\u{301}';
const COMBINING_CIRCUMFLEX: char = '\u{302}';
const COMBINING_MACRON: char = '\u{304}';

/// The generated table of word forms: entries in the notation, sorted by the
/// spelling they are for and front-coded in blocks (see [`find_in_table`]).
static FORMS: &str = include_str!("tables/forms.txt");

/// The generated table of stems, in the same format; the entries carry no `*`.
static STEMS: &str = include_str!("tables/stems.txt");

/// Hand-written entries. They win over the generated table, which since
/// 2026-10-30 holds every Sloleks lemma with hand-checked accents and is
/// verified form by form, so only two kinds are left: words Sloleks does
/// not list, and readings SSKJ² or the Slovenski pravopis 2001 (Fran.si)
/// give otherwise than Sloleks's first accentuation, or that a listener
/// asked for. A hand-written stem catches the words derived from it too
/// (gôr* read goreti as gôreti), so whole forms are preferred; the
/// readings that differ from Sloleks for every form of a word belong in
/// `CORRECTIONS` of `tools/formant/slovenian_words.py`, which writes them
/// into the table. Check a new entry against Fran and the table first
/// (2026-10-29: 281 of 1,605 entries written from memory contradicted
/// Sloleks, and the dictionaries sided with Sloleks every time).
#[rustfmt::skip] // rows of words
const HAND_ENTRIES: &[&str] = &[
    // ---- The commoner word where the corpus counts of a form mislead: áfna
    // the noun, also the name of @ (SSKJ², SP 2001, eSSKJ; the verb afnati's
    // afná was taken for it, clause_reading() has the verb after se), rôka
    // the hand (SSKJ², SP; the genitive róka of rók "deadline" is commoner in
    // the news corpus, clause_reading() has it after a preposition or a
    // genitive, the name Róka is in the tables of names), pòdmení the submenu
    // (SSKJ² mení; the locative podméni of podména "hypothesis" is commoner in
    // the corpus, not on a screen) ----
    "áfna", "rôka", "pòdmení",
    // ---- Numbers Sloleks does not list ----
    "pétdeset", "šéstdeset", "devétdeset", "trilijárd|a|i|e|o", "kvadrilijón|a|i|ov", "kvintilijón|a|i|ov",
    // ---- s'əm: the stressed sem. The participle of biti is the table's,
    // bilà biló bilí bilè (SSKJ² bíl bilà biló in bilò bilì in bilí bilè,
    // "stil. bíla"; SP 2001 bíl -à -ó in -ò, bilí in -ì -è); until
    // 2026-10-30 the stylistic bíla bílo bíli bíle were listed here ----
    "s'əm",
    // ---- Where Sloleks's first reading is not the dictionaries' first or
    // the commoner word: otròk (SSKJ² nom. otròk, gen. pl. otrók), dní
    // (SSKJ² rod. mn. dní), gôzda (gòzd gôzda tudi gózd), glasú (glás -ú
    // tudi -a: the genitive; genitive_u_reading() reads the dative and
    // locative glásu by the clause and leaves to this entry only a
    // capitalized Glasu with nothing to tell its case), naglàs the accent
    // (the adverb naglás "aloud" is commoner in the corpus), ôno ône (the
    // personal pronoun, óno ône the demonstrative), dà (the particle).
    // (Sobóta, here until 2026-10-30 against the town Múrska Sóbota, is
    // the table's: a name no longer wins over a common word there, and a
    // capital reads the name from the tables of names, name_reading().) ----
    "otròk", "dní", "gôzda", "glasú", "naglàs", "ôno", "ône", "dà",
    // ---- Verbs: the first variant of SSKJ² where Sloleks has the second
    // (písati in pisáti, potrdíti in potŕditi, shraníti in shrániti), the
    // present where Sloleks lists another word of the same spelling
    // (naredíjo, stojíjo, živíjo "they live"; žívijo the greeting is in
    // HEAD_READINGS), tŕdi (SP: trdíti in tŕditi -im, tŕdi), kúpite,
    // vŕnite (SP: kúpi -te, vŕni -te), povéj (SP povèj/povéj), razúmel
    // (SSKJ² razúmel tudi razumèl in razumél). Not jé "eats" (jésti jém):
    // the spelling is the auxiliary of bíti 39,000 times for once the verb
    // of eating in Sloleks, and the auxiliary has the open e, unstressed
    // and stressed alike (SP 2001: bíti sem si je, poudarjeno sèm sì jè;
    // Sloleks jè [ˈjɛ], jé [ˈjeː]); the table gives jè, je_reading() the
    // verb of eating by the clause (a listener's report of 2026-11-03) ----
    "písati", "potrdíti", "shraníti", "shraníla", "shraníli", "naredíjo", "stojíjo", "živíjo", "tŕdi", "kúpite",
    "vŕnite", "povéj", "razúmel", "gréjo", "dájo", "dobíjo", "jéjo", "učíjo",
    // ---- Words of computers and phones, and names ----
    "ínternet", "ínternet*",
    "menu=mení", // SSKJ² mení tudi menu [mení]
    "žívjo", "zarádi", "médtem", "anglésk*", "laprdus=láprdus*",
];

/// The flags a notation letter carries, and the plain letter it stands for;
/// `None` for a character that is not part of the notation.
pub(super) fn notation_letter(c: char) -> Option<(char, u8)> {
    Some(match c {
        'á' => ('a', F_STRESS | F_LONG),
        'à' => ('a', F_STRESS),
        'é' => ('e', F_STRESS | F_LONG | F_CLOSE),
        'ê' => ('e', F_STRESS | F_LONG | F_OPEN),
        'è' => ('e', F_STRESS | F_OPEN),
        'í' => ('i', F_STRESS | F_LONG),
        'ì' => ('i', F_STRESS),
        'ó' => ('o', F_STRESS | F_LONG | F_CLOSE),
        'ô' => ('o', F_STRESS | F_LONG | F_OPEN),
        'ò' => ('o', F_STRESS | F_OPEN),
        'ú' => ('u', F_STRESS | F_LONG),
        'ù' => ('u', F_STRESS),
        'ŕ' => ('r', F_STRESS | F_LONG),
        'ə' => ('e', F_SCHWA),
        'w' => ('l', F_W),
        'ł' => ('l', F_L),
        _ if is_base_letter(c) => (c, 0),
        _ => return None,
    })
}

/// Reads a pronunciation in the notation into plain letters and flags.
/// Accepts the ASCII marks too (' before the stressed vowel, : after a long
/// one) and the combining accents. The reason when something in it is not
/// notation.
pub(super) fn read_notation(pron: &[char]) -> Result<(Vec<char>, Vec<u8>), &'static str> {
    let mut plain: Vec<char> = Vec::with_capacity(pron.len());
    let mut flags: Vec<u8> = Vec::with_capacity(pron.len());
    let mut stress_pending = false;
    for &c in pron {
        match c {
            '\'' => stress_pending = true,
            ':' => *flags.last_mut().ok_or("length mark before any letter")? |= F_LONG,
            COMBINING_GRAVE | COMBINING_ACUTE | COMBINING_CIRCUMFLEX | COMBINING_MACRON => {
                let (Some(&letter), Some(f)) = (plain.last(), flags.last_mut()) else {
                    return Err("accent before any letter");
                };
                if c == COMBINING_MACRON {
                    *f |= F_LONG;
                } else {
                    *f |= F_STRESS;
                    if c != COMBINING_GRAVE {
                        *f |= F_LONG;
                    }
                    if letter == 'e' || letter == 'o' {
                        *f |= if c == COMBINING_ACUTE { F_CLOSE } else { F_OPEN };
                    }
                }
            }
            _ => {
                let (letter, mut f) = notation_letter(c).ok_or("not a letter of the notation")?;
                if stress_pending {
                    if !is_vowel_letter(letter) && letter != 'r' {
                        return Err("stress mark before a consonant");
                    }
                    f |= F_STRESS;
                    stress_pending = false;
                }
                plain.push(letter);
                flags.push(f);
            }
        }
    }
    if stress_pending {
        return Err("stress mark at the end");
    }
    // A stressed short e or o is open; a stressed long one without a quality
    // mark (the ASCII form "'e:") is open too: the close vowel is always
    // written é, ó.
    for (&letter, f) in plain.iter().zip(&mut flags) {
        if (letter == 'e' || letter == 'o') && *f & F_STRESS != 0 && *f & (F_CLOSE | F_OPEN | F_SCHWA) == 0 {
            *f |= F_OPEN;
        }
    }
    Ok((plain, flags))
}

/// How a spelling compares with the spelling an entry of a generated table
/// is for: the spelling is read off the notation while comparing.
fn compare_key(key: &[char], entry: &str) -> Ordering {
    let mut key = key.iter();
    for c in entry.chars() {
        if matches!(c, '*' | COMBINING_GRAVE | '\'' | ':') {
            continue;
        }
        let plain = notation_letter(c).map_or(c, |(plain, _)| plain);
        match key.next() {
            None => return Ordering::Less, // the entry is longer
            Some(&letter) if letter != plain => return letter.cmp(&plain),
            Some(_) => {}
        }
    }
    if key.next().is_none() { Ordering::Equal } else { Ordering::Greater }
}

/// Whether a character of a front-coded table opens an entry: the entries
/// themselves have only characters from `a` up.
fn opens_entry(c: char) -> bool {
    c < 'a'
}

/// The entry of a generated table for a spelling. The tables are text,
/// searched in place, so no copy is built at start: one line is a block of
/// 16 entries in the notation, the lines and the entries in them sorted by
/// the spelling they are for. The first entry of a line is written whole;
/// each of the others begins with one character from `!` (0) to `` ` `` (63)
/// for the number of leading bytes it shares with the entry before it, then
/// the rest of it (`tools/formant/slovenian_tables.py`).
pub(super) fn find_in_table(table: &str, key: &[char]) -> Option<Vec<char>> {
    // The last line whose first entry is not after the key
    let bytes = table.as_bytes();
    let (mut lo, mut hi) = (0, bytes.len());
    let mut block = None;
    while lo < hi {
        let mid = lo + (hi - lo) / 2;
        let start = bytes[..mid].iter().rposition(|&b| b == b'\n').map_or(0, |at| at + 1);
        let end = bytes[mid..].iter().position(|&b| b == b'\n').map_or(bytes.len(), |at| mid + at);
        let line = table.get(start..end)?;
        let first = line.find(opens_entry).map_or(line, |at| &line[..at]);
        if compare_key(key, first) == Ordering::Less {
            hi = start;
        } else {
            block = Some(line);
            lo = end + 1;
        }
    }
    let mut rest = block?;
    let mut entry = String::new();
    while let Some(first) = rest.chars().next() {
        if opens_entry(first) {
            let shared = usize::try_from(u32::from(first)).ok()?.checked_sub(0x21)?;
            if !entry.is_char_boundary(shared) {
                return None; // not a table this program wrote
            }
            entry.truncate(shared);
            rest = rest.get(1..)?;
        } else {
            entry.clear();
        }
        let end = rest.find(opens_entry).unwrap_or(rest.len());
        entry.push_str(&rest[..end]);
        rest = &rest[end..];
        match compare_key(key, &entry) {
            Ordering::Equal => return Some(entry.chars().collect()),
            Ordering::Less => return None,
            Ordering::Greater => {}
        }
    }
    None
}

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
///   ł         an l pronounced [l], whatever the rules would say
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
    /// The generated tables of forms and of stems; the hand-added entries
    /// win over them.
    tables: Option<(&'static str, &'static str)>,
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
    pub fn parse(marked: &[char]) -> Result<Entry, &'static str> {
        if marked.is_empty() {
            return Err("empty entry");
        }
        let (text, stem) = match marked {
            [text @ .., '*'] => (text, true),
            _ => (marked, false),
        };
        let mut given_key = Vec::new();
        let mut text = text;
        if let Some(eq) = text.iter().position(|&c| c == '=') {
            // The spelling may carry the dictionary's accents too (rékel=rékəw)
            for &c in &text[..eq] {
                if matches!(c, ' ' | COMBINING_GRAVE | COMBINING_ACUTE | COMBINING_CIRCUMFLEX | COMBINING_MACRON) {
                    continue;
                }
                let (plain, _) = notation_letter(c).ok_or("the spelling has a letter that is not one")?;
                given_key.push(plain);
            }
            text = &text[eq + 1..];
        }
        if text.is_empty() {
            return Err("empty pronunciation");
        }
        if text.contains(&' ') {
            return Err("space in an entry");
        }
        let (plain, flags) = read_notation(text)?;
        // A stem needs no stress (its ending may carry it in a form list); a
        // whole word does, unless it has no vowel at all.
        let stressed = flags.iter().any(|&f| f & F_STRESS != 0);
        let vowel = plain.iter().any(|&c| is_vowel_letter(c) || c == 'r');
        if !stem && vowel && !stressed {
            return Err("no stressed vowel");
        }
        Ok(Entry { key: if given_key.is_empty() { plain } else { given_key }, pron: text.to_vec(), stem })
    }

    /// Adds one entry (a word, a stem or a form list); false if malformed.
    pub fn add(&mut self, marked: &[char]) -> bool {
        let mut insert = |form: &[char]| {
            let Ok(entry) = Self::parse(form) else { return false };
            let entries = if entry.stem { &mut self.stems } else { &mut self.exact };
            entries.insert(entry.key, entry.pron);
            true
        };
        // A form list: prefix|ending|ending; marks may sit in either part
        let mut parts = marked.split(|&c| c == '|');
        let prefix = parts.next().unwrap_or_default();
        if prefix.len() == marked.len() {
            return insert(marked);
        }
        let mut ok = true;
        for ending in parts {
            ok &= insert(&[prefix, ending].concat());
        }
        ok
    }

    pub fn clear(&mut self) {
        self.exact.clear();
        self.stems.clear();
    }

    pub fn is_empty(&self) -> bool {
        self.exact.is_empty() && self.stems.is_empty()
    }

    /// The pronunciation of an exact form.
    pub fn find_exact(&self, word: &[char]) -> Option<Vec<char>> {
        self.exact.get(word).cloned().or_else(|| find_in_table(self.tables?.0, word))
    }

    /// The longest stem the word starts with (an ending of up to
    /// [`Self::MAX_ENDING`] letters): its length and its pronunciation.
    /// `accept`, when given, passes only the stems whose ending it accepts.
    pub fn find_stem(&self, word: &[char], accept: Option<fn(&[char]) -> bool>) -> Option<(usize, Vec<char>)> {
        if self.stems.is_empty() && self.tables.is_none() {
            return None;
        }
        let shortest = if word.len() > Self::MAX_ENDING { word.len() - Self::MAX_ENDING } else { 1 }.max(2);
        (shortest..=word.len()).rev().find_map(|len| {
            let (stem, ending) = word.split_at(len);
            if accept.is_some_and(|accept| !accept(ending)) {
                return None;
            }
            let pron = self.stems.get(stem).cloned().or_else(|| find_in_table(self.tables?.1, stem))?;
            Some((len, pron))
        })
    }
}

/// The built-in lexicon (the generated tables of Sloleks 3.1 plus the
/// hand-written entries, which win).
pub fn builtin_lexicon() -> &'static Lexicon {
    static LEXICON: LazyLock<Lexicon> = LazyLock::new(|| {
        let mut lexicon = Lexicon { tables: Some((FORMS, STEMS)), ..Lexicon::default() };
        for entry in HAND_ENTRIES {
            lexicon.add(&entry.chars().collect::<Vec<char>>());
        }
        lexicon
    });
    &LEXICON
}

#[cfg(test)]
mod tests {
    use super::*;

    fn chars(s: &str) -> Vec<char> {
        s.chars().collect()
    }

    /// Every entry of a front-coded table, in order.
    fn entries(table: &str) -> Vec<String> {
        let mut out = Vec::new();
        for line in table.lines() {
            let mut entry = String::new();
            let mut rest = line;
            while let Some(first) = rest.chars().next() {
                if opens_entry(first) {
                    entry.truncate(first as usize - 0x21);
                    rest = &rest[1..];
                } else {
                    entry.clear();
                }
                let end = rest.find(opens_entry).unwrap_or(rest.len());
                entry.push_str(&rest[..end]);
                rest = &rest[end..];
                out.push(entry.clone());
            }
        }
        out
    }

    /// The spelling an entry is for.
    fn key_of(entry: &str) -> Vec<char> {
        let kept = entry.chars().filter(|&c| !matches!(c, '*' | COMBINING_GRAVE | '\'' | ':'));
        kept.map(|c| notation_letter(c).map_or(c, |(plain, _)| plain)).collect()
    }

    #[test]
    fn every_entry_of_the_generated_tables_is_found_and_reads() {
        for (table, count) in [(FORMS, 95_144), (STEMS, 79_671)] {
            let all = entries(table);
            assert_eq!(all.len(), count);
            for entry in &all {
                assert_eq!(find_in_table(table, &key_of(entry)), Some(chars(entry)), "{entry}");
                assert!(read_notation(&chars(entry)).is_ok(), "{entry}");
            }
        }
        for key in ["", "aa", "žžž", "mest", "zzzzzzzz", "ž", "\u{10FFFF}", "!", "abdúl"] {
            assert_eq!(find_in_table(FORMS, &chars(key)), None, "{key}");
        }
        assert_eq!(find_in_table("", &chars("a")), None);
        assert_eq!(find_in_table("\n\n", &chars("a")), None);
        assert_eq!(find_in_table("b~~a\n", &chars("a")), None);
    }

    #[test]
    fn the_lexicon_reads_forms_stems_and_hand_entries() {
        let lexicon = builtin_lexicon();
        let exact = |word: &str| lexicon.find_exact(&chars(word)).map(|pron| pron.iter().collect::<String>());
        assert_eq!(exact("abdul").as_deref(), Some("abdúl"));
        assert_eq!(exact("a").as_deref(), Some("à"));
        assert_eq!(exact("roka").as_deref(), Some("rôka")); // the hand entry wins
        assert_eq!(exact("trilijarde").as_deref(), Some("trilijárde"));
        assert_eq!(exact("menu").as_deref(), Some("mení"));
        assert_eq!(exact("bruhoberko"), None);
        let (length, pron) = lexicon.find_stem(&chars("internetom"), None).expect("the stem ínternet*");
        assert_eq!((length, pron.iter().collect::<String>().as_str()), (8, "ínternet"));
        assert_eq!(lexicon.find_stem(&chars("internetom"), Some(|ending| ending.is_empty())), None);
        assert_eq!(lexicon.find_stem(&[], None), None);
    }

    #[test]
    fn entries_are_checked() {
        let parse = |entry: &str| Lexicon::parse(&chars(entry));
        let entry = parse("d'é:la*").expect("a stem");
        assert_eq!((entry.key, entry.stem), (chars("dela"), true));
        assert_eq!(parse("hotel=hotél").expect("a spelling").key, chars("hotel"));
        assert_eq!(parse("rékel=rékəw").expect("a spelling with marks").key, chars("rekel"));
        assert_eq!(parse("p'əs").expect("a schwa").key, chars("pes"));
        for (entry, why) in [
            ("", "empty entry"),
            ("*", "empty pronunciation"),
            ("mesto", "no stressed vowel"),
            ("m'esto x", "space in an entry"),
            ("Mésto", "not a letter of the notation"),
            (":a", "length mark before any letter"),
            ("\u{301}a", "accent before any letter"),
            ("'ta", "stress mark before a consonant"),
            ("ta'", "stress mark at the end"),
            ("me1=mé", "the spelling has a letter that is not one"),
        ] {
            assert_eq!(parse(entry), Err(why), "{entry}");
        }
        let mut lexicon = Lexicon::default();
        assert!(lexicon.add(&chars("zač|éti|éla|nèm")));
        assert_eq!(lexicon.find_exact(&chars("začela")), Some(chars("začéla")));
        assert!(!lexicon.add(&chars("zač|eti|éla")));
        assert!(!lexicon.add(&chars("mesto")));
        assert!(lexicon.find_stem(&chars("začela"), None).is_none() && !lexicon.is_empty());
    }
}
