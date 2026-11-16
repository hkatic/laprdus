//! Slovenian letters to phones for one word, the stress and the sounds that
//! depend on it, and the sound changes across a clause.
//!
//! Sources (see "Slovenian" in `docs/formant.md`): Slovenski pravopis 2001
//! (Pravila, "Glasoslovje": the pronunciation of l and v, the schwa, voicing
//! assimilation and final devoicing), Toporišič's Slovenska slovnica (stress,
//! the clitics), and the Sloleks lexicon (CJVT, CLARIN.SI) for the counts
//! behind the stress rules.

use super::lexicon::{F_CLOSE, F_L, F_LONG, F_OPEN, F_SCHWA, F_STRESS, F_W, Lexicon, builtin_lexicon, read_notation};
use super::readings::split_compound_numeral;
use crate::formant::phones::{Ph, Phone};
use crate::text::frontend::letters::{
    MARK_LONG, MARK_RISE, MARK_STRESS, consonant_phone, ends_with, ends_with_any, is_one_of, is_vowel_letter,
    starts_with,
};
use crate::text::frontend::sounds::{voiced_of, voiceless_of};
use crate::text::table;

/// Spellings whose final l is [l] though they look like a participle: the
/// transcriptions of Sloleks, without the words SSKJ² and SP mark [-u̯]
/// (generated; see `finish_word()`).
static FINAL_L: &str = include_str!("tables/final_l.txt");

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

impl WordPhones {
    fn add(&mut self, ph: Ph, letter: usize, nucleus: bool) -> &mut Phone {
        if nucleus {
            self.nuclei.push(self.phones.len());
        }
        self.letter.push(letter);
        self.phones.push(Phone { ph, nucleus, ..Phone::default() });
        let last = self.phones.len() - 1;
        &mut self.phones[last]
    }

    /// The letter of the nucleus of syllable `k`.
    fn nucleus_letter(&self, k: usize) -> usize {
        self.nuclei.get(k).and_then(|&at| self.letter.get(at)).copied().unwrap_or(0)
    }
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

fn is_sonorant_letter(c: char) -> bool {
    matches!(c, 'l' | 'r' | 'm' | 'n' | 'j' | 'v')
}

/// The letter at `i`, `'\0'` past either end.
fn at(w: &[char], i: usize) -> char {
    w.get(i).copied().unwrap_or('\0')
}

/// The last place where `part` stands.
fn rfind(w: &[char], part: &str) -> Option<usize> {
    (0..=w.len()).rev().find(|&pos| starts_with(&w[pos..], part))
}

/// A surname in -ović, -ević (-ovič, -evič, -ovic, -evic) in any of its
/// forms (Petrović, Petrovića, Petrovićem, Petrovićeva): where the suffix
/// begins. `tools/formant/slovenian_words.py`, `vic_surname()`, mirrors it.
fn vic_surname(w: &[char]) -> Option<usize> {
    const ENDINGS: &[&str] = &[
        "evimi", "evega", "evemu", "evem", "evih", "evim", "ema", "ama", "eva", "evo", "evi", "eve", "em", "om", "ev",
        "ih", "a", "u", "i", "e", "",
    ];
    ENDINGS.iter().find_map(|ending| {
        let core = w.len().checked_sub(ending.chars().count()).filter(|&core| core >= 5 && ends_with(w, ending))?;
        (matches!(w[core - 1], 'ć' | 'č' | 'c')
            && w[core - 2] == 'i'
            && w[core - 3] == 'v'
            && matches!(w[core - 4], 'o' | 'e'))
        .then_some(core - 4)
    })
}

/// A word read by the rules of names: written with a capital, or with a
/// letter of Croatian, Serbian and Bosnian names (ć, đ) that no Slovenian
/// common word has.
fn has_name_letter(w: &[char]) -> bool {
    w.iter().any(|&c| c == 'ć' || c == 'đ')
}

/// The endings of the declension of nouns and adjectives and of the
/// possessive adjectives, the longest first.
const NAME_ENDINGS: &[&str] = &[
    "evimi", "ovimi", "inimi", "evega", "ovega", "inega", "evemu", "ovemu", "inemu", "evem", "ovem", "inem", "evih",
    "ovih", "inih", "evim", "ovim", "inim", "eva", "evo", "evi", "eve", "ova", "ovo", "ovi", "ove", "ina", "ino",
    "ini", "ine", "ega", "emu", "imi", "ima", "ama", "ami", "ema", "oma", "ah", "am", "em", "om", "ih", "im", "ov",
    "ev", "in", "a", "e", "i", "o", "u", "",
];

/// A name as base + an ending of the declension of nouns and adjectives or
/// of a possessive adjective, the base ending with `suffix` (Albína: albin +
/// a; Dolénca: dolenc + a; Gorénjskega: gorenjsk + ega; Kováčičeva: kovačič +
/// eva): the base's length. Used by the rules of names only
/// (`default_stress()`); the generator does not simulate them.
fn name_base(w: &[char], suffix: &str) -> Option<usize> {
    NAME_ENDINGS.iter().find_map(|ending| {
        let base = w.len().checked_sub(ending.chars().count()).filter(|_| ends_with(w, ending))?;
        (base >= suffix.chars().count() && ends_with(&w[..base], suffix)).then_some(base)
    })
}

/// An ending of the declension of nouns and adjectives or of a possessive
/// adjective, also after one consonant: the j or t that names in -r and -e
/// take (Cánkarja, Cánkarjev; Jóžeta, Jóžetov) and the consonant after a
/// fleeting e (Koróšca of Koróš*ec, Júpitrov, Štúkljem): a name inside a
/// clause takes a stem of the lexicon only with one of these, not another
/// word's beginning (Ferdolenc: férd* of Ferdo + olenc; Pirnatčič).
/// `tools/formant/slovenian_words.py`, `declension_ending()`, mirrors it.
fn declension_ending(e: &[char]) -> bool {
    let listed = |e: &[char]| is_one_of(e, NAME_ENDINGS) || is_one_of(e, &["ijo", "jo"]);
    if listed(e) {
        return true;
    }
    let consonants = if starts_with(e, "lj") || starts_with(e, "nj") { 2 } else { 1 };
    e.len() > consonants && !is_vowel_letter(e[0]) && listed(&e[consonants..])
}

/// r is a syllable of its own ([ər]) between consonants, at the head of a
/// word before a consonant and at the end after one: prst, vrt, trg, rdeč,
/// rjav, Trst, četrtek.
fn is_syllabic_r(w: &[char], i: usize) -> bool {
    let prev = i.checked_sub(1).map_or('\0', |before| at(w, before));
    let next = at(w, i + 1);
    !is_vowel_letter(prev) && !is_vowel_letter(next) && prev != 'r' && next != 'r'
}

/// Where an l before a consonant is [w] (Slovenski pravopis 2001, § 650 b
/// and c): in the roots with -ol- between consonants that the rules list
/// (dolg, golčati, golk, golt, jabolko, kolcati se, kolč, kolk, kolnem, molk,
/// molčati, molsti, polh, poln, polt, polzeti, polž, solza, tolči, tolst,
/// tolšča, volk, volna, žolč, žolna, žolt), and in the -lc-, -lk-, -lsk-,
/// -lstv- of agent nouns derived from verbs (bralec, bralca, bralka, bralski,
/// bralstvo; igralec, gledalka, nosilec). Everywhere else an l before a
/// consonant is [l] (kultura, film, Alpe, polje, delno, alge: Čibej 2023
/// found [l] in 71% of the words of the Sloleks lexicon). A lexicon entry
/// settles a word either way.
fn l_vocalizes_before_consonant(w: &[char], i: usize) -> bool {
    const OL_ROOTS: &[&str] = &[
        "dolg", "golč", "golk", "golt", "jabolk", "kolc", "kolč", "kolk", "koln", "molk", "molč", "mols", "polh",
        "poln", "polt", "polz", "polž", "solz", "tolč", "tolst", "tolšč", "volk", "voln", "žolč", "žoln", "žolt",
    ];
    let before = i.checked_sub(1).map_or('\0', |k| at(w, k));
    if i >= 2 && before == 'o' {
        let in_root = |root: &&str| {
            let l = root.chars().position(|c| c == 'l').unwrap_or(0);
            i >= l && starts_with(&w[i - l..], root)
        };
        if OL_ROOTS.iter().any(in_root) {
            return true;
        }
    }
    // -alec, -alca, -alka, -alski, -alstvo, -ilec, -ilka ...: a vowel, the l,
    // then c, k or s + k/t, with at least one syllable before the vowel
    if i >= 2 && matches!(before, 'a' | 'i' | 'e') && i + 1 < w.len() {
        let c = w[i + 1];
        let agent = c == 'c' || c == 'k' || (c == 's' && matches!(at(w, i + 2), 'k' | 't'));
        return agent && w[..i - 1].iter().any(|&c| is_vowel_letter(c));
    }
    false
}

/// Nouns and adjectives whose final l after a vowel is [w] (Slovenski
/// pravopis 2001, § 650 č and d): stol, žival, sol, bel, gol, fižol, kol,
/// vol, drhal, piščal, zibel, cel, debel, gnil, vesel, zrel, pol, žal, tal;
/// the participles and the words with a schwa before the l are rules.
fn l_final_w_word(w: &[char]) -> bool {
    const WORDS: &[&str] = &[
        "stol", "žival", "sol", "bel", "gol", "fižol", "kol", "vol", "drhal", "piščal", "zibel", "cel", "debel",
        "gnil", "vesel", "zrel", "pol", "žal", "tal", "dol", "spol", "pepel", "kotel", "kozel", "orel", "osel",
        "misel", "topel", "mrzel", "posel", "kašelj",
    ];
    is_one_of(w, WORDS)
}

/// Words that keep the [l] although the rule says [w], beside the table
/// `FINAL_L`: dél (SSKJ: dél -a [tudi deu̯], the [l] first), the loans šál,
/// bál, gél, mól, and frequent foreign words without an accent in Sloleks,
/// which a stem or the rule would otherwise read (Paul, email, cocktail).
fn l_final_l_word(w: &[char]) -> bool {
    const WORDS: &[&str] = &[
        "del", "šal", "bal", "gel", "mol", "paul", "opel", "posedel", "musical", "email", "mail", "hotmail", "cocktail",
    ];
    is_one_of(w, WORDS)
}

/// A word being analysed.
#[derive(Default)]
struct Analysis {
    /// Letters.
    plain: Vec<char>,
    /// Per letter.
    flags: Vec<u8>,
    /// Letters that came from the lexicon's notation (the rest by rule).
    notation_letters: usize,
    letter_name: bool,
    /// Written with a capital and in no lexicon.
    unknown_name: bool,
}

impl Analysis {
    /// The word as it is spelled, read by the rules alone.
    fn by_rule(word: &[char]) -> Self {
        Self { plain: word.to_vec(), flags: vec![0; word.len()], ..Self::default() }
    }

    fn flag(&self, letter: usize) -> u8 {
        self.flags.get(letter).copied().unwrap_or(0)
    }
}

/// The phones of the letters, and for each nucleus whether it is the vowel
/// [u] of a v at the head of a word (vsak), which never takes the stress.
/// Vowel quality and length follow the flags where they say so, the rules of
/// unstressed vowels otherwise (every unstressed e and o is open); the
/// stressed vowel's quality is settled after the stress is known
/// (`finish_word()`).
fn letters_to_phones(a: &Analysis, out: &mut WordPhones) -> Vec<bool> {
    let w = &a.plain[..];
    let n = w.len();
    let mut prothetic = Vec::new();
    let mut i = 0;
    while i < n {
        let c = w[i];
        let f = a.flag(i);
        let next = at(w, i + 1);
        if c == 'd' && next == 'ž' {
            out.add(Ph::Dzh, i, false);
            i += 2;
            continue;
        }
        if (c == 'l' || c == 'n') && next == 'j' {
            // lj, nj are the two sounds l + j, n + j before a vowel
            // (Ljubljana, polje, konja) and a plain l, n before a consonant
            // and at the end of a word (konj, Kranj, cilj, manjši, poljski):
            // Slovenski pravopis 2001, § 664-666
            out.add(if c == 'l' { Ph::L } else { Ph::N }, i, false);
            if is_vowel_letter(at(w, i + 2)) {
                out.add(Ph::J, i + 1, false);
            }
            i += 2;
            continue;
        }
        if is_vowel_letter(c) {
            let ph = match c {
                'a' => Ph::A,
                'i' => Ph::I,
                'u' => Ph::U,
                'e' if f & F_SCHWA != 0 => Ph::Ax,
                'e' if f & F_CLOSE != 0 => Ph::Ee,
                'e' => Ph::Eh,
                _ if f & F_CLOSE != 0 => Ph::Oo,
                _ => Ph::Oh,
            };
            out.add(ph, i, true).is_long = f & F_LONG != 0 && ph != Ph::Ax;
            prothetic.push(false);
            // i before another vowel in the words of Greek and Latin origin
            // is [ij]: pacient, socialen, radio, material (§ 701)
            let letters_by_rule = a.notation_letters + if a.notation_letters == 0 { n } else { 0 };
            if c == 'i' && matches!(next, 'a' | 'e' | 'o' | 'u') && i + 1 < letters_by_rule {
                out.add(Ph::J, i, false);
            }
        } else if c == 'r' && is_syllabic_r(w, i) {
            // [ər]: the schwa carries the syllable, the r follows it
            out.add(Ph::Ax, i, true);
            prothetic.push(false);
            out.add(Ph::R, i, false).nucleus_tail = true;
        } else if c == 'v' {
            // Slovenski pravopis 2001, § 642-645: [v] only before a vowel of
            // the same word (vino, vrt [vərt]); after a vowel, before a
            // consonant or at the end, the glide [u̯] (siv, avto, prav);
            // at the head of a word before a consonant [w] (vlak, vrana,
            // vzeti), which may also be the vowel [u] (vsak [usak]): the
            // vowel is used before an obstruent, where a glide alone is hard
            // to hear, and never takes the stress.
            if is_vowel_letter(next) || (next == 'r' && is_syllabic_r(w, i + 1)) {
                out.add(Ph::V, i, false);
            } else if i == 0 && next != '\0' && !is_sonorant_letter(next) {
                out.add(Ph::U, i, true);
                prothetic.push(true);
            } else {
                out.add(Ph::W, i, false);
            }
        } else if c == 'l' {
            // At the end of a word the l is settled in finish_word(), once
            // the stress is known (bil, delal: [w]; hotel: [l])
            let vocalized = f & F_W != 0
                || (!is_vowel_letter(next) && next != 'j' && next != '\0' && l_vocalizes_before_consonant(w, i));
            out.add(if vocalized { Ph::W } else { Ph::L }, i, false);
        } else {
            let ph = consonant_phone(c);
            if ph != Ph::Sil {
                out.add(ph, i, false);
            }
        }
        i += 1;
    }
    prothetic
}

/// Where the stress falls when no dictionary says: the suffix rules of the
/// Slovenian word classes whose accent is regular, then the second-last
/// syllable, the commonest place in the language. The index into `nuclei`
/// (the prothetic u of vsak, vzeti excluded).
fn default_stress(w: &[char], wp: &WordPhones, prothetic: &[bool], capital: Capital) -> Option<usize> {
    let foreign_letter = has_name_letter(w);
    let surname = capital != Capital::None || foreign_letter;
    let name = matches!(capital, Capital::Head | Capital::Inside) || foreign_letter;
    // Nucleus indexes that may carry the stress, and the letter of each
    let candidates: Vec<usize> =
        (0..wp.nuclei.len()).filter(|&k| !prothetic.get(k).copied().unwrap_or(false)).collect();
    let letters: Vec<usize> = candidates.iter().map(|&k| wp.nucleus_letter(k)).collect();
    let syllables = candidates.len();
    match candidates[..] {
        [] => return None,
        [only] => return Some(only),
        _ => {}
    }

    // Surnames in -ović, -ević: the syllable before the suffix, which is
    // the first syllable of the three-syllable ones (Pétrović, Pópovič,
    // Túrković, Srébrović) and the second of four (Stojánović, Kováčević,
    // Milóšević), as in the hand-accented names of Sloleks (63 of 65 with
    // three syllables, 69 of 72 with four, 12 of 13 with five); before
    // -ijević the syllable before that (Antónijević, Dimítrijević); an i
    // after a vowel stands for j (Mihailović as Mihájlović); without a
    // vowel before the suffix, its o (Kôvič, Nóvič). The lexicon's
    // hand-accented forms still win, the stems of common words do not
    // (phonemize()).
    if surname && let Some(mut end) = vic_surname(w) {
        if end >= 3 && w[end - 2] == 'i' && w[end - 1] == 'j' {
            end -= 2;
        }
        let Some(mut before) = letters.iter().rposition(|&letter| letter < end) else {
            return Some(candidates[0]);
        };
        let letter = letters[before];
        if before > 0 && at(w, letter) == 'i' && letter > 0 && is_vowel_letter(w[letter - 1]) {
            before -= 1;
        }
        return Some(candidates[before]);
    }

    // The other names the lexicon does not know, in any case form: the
    // patterns of the hand-accented names of Sloleks that the rules of
    // common words miss (on its 5,000 names written as one word they
    // raise the rules from 74 to 80 %; the counts are of names right by
    // the pattern against those right by the common rules)
    if name {
        // Candidate positions before a letter, and the candidate at a letter
        let before = |letter: usize| -> Vec<usize> { (0..syllables).filter(|&c| letters[c] < letter).collect() };
        let at_letter = |letter: usize| letters.iter().position(|&l| l == letter).map(|c| candidates[c]);
        let based = |suffix: &str| name_base(w, suffix);
        // -ič, -ić of three syllables and more (not -ović, above): before
        // -čič after a consonant the second-last syllable (Zupánčič,
        // Miklávčič, Gregórčič), otherwise the third-last (Gólobič,
        // Tómažič, Míhelič; Kóstelić, Níkolić, Káradžić): 117 and 76
        // against 64 and 67
        if let Some(b) = based("ič").or_else(|| based("ić")) {
            let base = before(b);
            if base.len() >= 3 {
                let cic = w[b - 1] == 'č' && b >= 4 && w[b - 3] == 'č' && !is_vowel_letter(w[b - 4]);
                return Some(candidates[base[base.len() - if cic { 2 } else { 3 }]]);
            }
        }
        // -ovec, -evec, and the oblique cases -ovca, -evcu ...: the syllable
        // before (Ájdovec, Búkovec, Gábrovca): 34 against 6, the oblique
        // forms of the names of Sloleks 226 against 100 (Debévca of the
        // contracted Debévc is the rest)
        if ends_with(w, "ovec") || ends_with(w, "evec") {
            if let Some(&last) = before(w.len() - 4).last()
                && before(w.len()).len() >= 3
            {
                return Some(candidates[last]);
            }
        } else if let Some(b) = based("ovc").or_else(|| based("evc")).filter(|&b| b < w.len())
            && let Some(&last) = before(b - 3).last()
        {
            return Some(candidates[last]);
        }
        // -ec without its e after n, v, j, l, r, m: the last syllable
        // (Dolénc, Bohínc, Debévc, Dobrájc): 39 against 13
        if let Some(b) = based("c")
            && b >= 3
            && is_vowel_letter(w[b - 3])
            && matches!(w[b - 2], 'n' | 'v' | 'j' | 'l' | 'r' | 'm')
            && before(b).len() >= 2
            && let Some(k) = at_letter(b - 3)
        {
            return Some(k);
        }
        // Slavic names in -mir, -bor, -slav, -dar, -gor after a vowel: the
        // first syllable (Bógomir, Dálibor, Ljúbomir, Bránimir): 23 against 1
        for suffix in ["mir", "bor", "slav", "dar", "gor"] {
            if let Some(b) = based(suffix)
                && b > suffix.len()
                && is_vowel_letter(w[b - suffix.len() - 1])
                && before(b).len() >= 3
            {
                return Some(candidates[0]);
            }
        }
        // -iel, -ael: the first syllable (Gábriel, Dániel, Ízrael)
        if ends_with(w, "iel") || ends_with(w, "ael") {
            return Some(candidates[0]);
        }
        // -in: its i (Albín, Avguštín, Albína, Martína): 56 against 21
        if let Some(b) = based("in")
            && before(b).len() >= 2
            && let Some(k) = at_letter(b - 2)
        {
            return Some(k);
        }
        // -ak of three syllables and more: its a (Cerkvenják, Gorenják): 18
        // against 0
        if let Some(b) = based("ak")
            && before(b).len() >= 3
            && let Some(k) = at_letter(b - 2)
        {
            return Some(k);
        }
        // -ij, -ija: the syllable before (Júrij, Antónij, Albánija,
        // Bolgárija, Ávstrija): 18 against 3, 85 against 30. A feminine
        // only inside a clause: at its head the word may be a common noun
        // the lexicon lacks, stressed on the i (biologíja)
        if let Some(b) = based("ij")
            && (b == w.len() || capital == Capital::Inside)
            && let Some(&last) = before(b - 2).last()
            && before(b).len() >= 2
        {
            return Some(candidates[last]);
        }
        // -aj, -ej, -oj, -uj: that vowel (Andréj, Jernéj, Cigój): 29
        // against 14; -al, -il, -ul of two syllables: its vowel (Abdúl,
        // Kanál, Ciríl): 19 against 3 (-el and -ol are as often on the
        // first syllable: Pável, Kárel, Bártol)
        if let Some(b) = based("j")
            && b >= 2
            && w[b - 2] != 'i'
            && is_vowel_letter(w[b - 2])
            && before(b).len() >= 2
            && let Some(k) = at_letter(b - 2)
        {
            return Some(k);
        }
        if let Some(b) = based("l")
            && b >= 2
            && matches!(w[b - 2], 'a' | 'i' | 'u')
            && before(b).len() == 2
            && let Some(k) = at_letter(b - 2)
        {
            return Some(k);
        }
    }

    // The syllable of a letter: the first nucleus at or after it
    let nucleus_of_letter =
        |letter: usize| (0..wp.nuclei.len()).find(|&k| wp.nucleus_letter(k) >= letter).unwrap_or(wp.nuclei.len() - 1);
    let last_vowel_before = |end: usize| w[..end.min(w.len())].iter().rposition(|&c| is_vowel_letter(c));
    let penult = candidates[syllables - 2];
    let antepenult = candidates[syllables.saturating_sub(3)];
    // The syllable before a suffix of so many letters
    let syllable_before = |suffix_letters: usize| {
        last_vowel_before(w.len().saturating_sub(suffix_letters)).map_or(penult, nucleus_of_letter)
    };

    // The rules below follow the counts over the manually accented lemmas
    // of the Sloleks lexicon and § 897-901 of the Pravopis: the suffixes
    // that always carry the stress, then the syllable before a suffix
    // where that is the rule, then the second-last syllable.

    // -írati, -íran, -íranje and the rest of the paradigm (študírati,
    // organizírati): the i before r, always
    if let Some(pos) = rfind(w, "ir")
        && pos >= 3
        && matches!(at(w, pos + 2), 'a' | 'e' | 'u' | 'o' | 'j')
    {
        return Some(nucleus_of_letter(pos));
    }
    // -ováti, -eváti and their nouns -ovánje, -evánje (kupováti, potováti,
    // črkovánje, pričakovánje, izpolnjeváti, poučevánje): the a of the
    // suffix; -ávati, -ávanje (predávati, spoznávati) and the -évati whose
    // e belongs to the stem, after a non-palatal consonant (zahtévati,
    // ogrévati, dozorévati): the first vowel
    const VATI: &[&str] = &[
        "ovati", "ovanje", "ovanja", "ovanju", "ovanjem", "avati", "avanje", "avanja", "avanju", "avanjem", "evati",
        "evanje", "evanja", "evanju", "evanjem",
    ];
    for suffix in VATI {
        if ends_with(w, suffix) && w.len() >= suffix.len() + 2 {
            let start = w.len() - suffix.len();
            let on_a = match suffix.as_bytes()[0] {
                b'o' => true,
                b'e' => matches!(w[start - 1], 'č' | 'š' | 'ž' | 'j' | 'c'),
                _ => false,
            };
            return Some(nucleus_of_letter(start + if on_a { 2 } else { 0 }));
        }
    }
    // -éti infinitives (živéti, želéti, hotéti, razuméti): the e
    if ends_with(w, "eti") && w.len() >= 5 && !ends_with(w, "deti") {
        return Some(nucleus_of_letter(w.len() - 3));
    }
    // -niti infinitives (pritísniti, klíkniti, dotakníti is the exception):
    // the syllable before -niti
    if ends_with(w, "niti") && w.len() >= 6 {
        return Some(syllable_before(4));
    }
    // -ati, -anje: a verb of three syllables on the a (igráti, iskáti,
    // končáti, vprašánje), a longer one on the stem (poslúšati, pomágati,
    // prihájati, poslúšanje)
    if (ends_with(w, "ati") || ends_with_any(w, &["anje", "anja", "anju", "anjem", "anjih"])) && syllables >= 4 {
        return Some(antepenult);
    }
    // Comparatives and superlatives: -ejši keeps the adjective's stress
    // before it (abótnejši, zanimívejši), the others are stressed on the
    // syllable before -ši (lépši, bóljši, najlépši: the naj- is a second,
    // weaker accent)
    const EJSI: &[&str] =
        &["ejši", "ejša", "ejše", "ejšo", "ejšu", "ejših", "ejšim", "ejšimi", "ejšega", "ejšemu", "ejšem"];
    const SI: &[&str] = &["ši", "ša", "še", "šo", "šu", "ših", "šim", "šimi", "šega", "šemu", "šem"];
    if ends_with_any(w, EJSI)
        && let Some(vowel) = rfind(w, "ejš").and_then(last_vowel_before)
    {
        return Some(nucleus_of_letter(vowel));
    }
    if ends_with_any(w, SI)
        && let Some(vowel) = rfind(w, "š").and_then(last_vowel_before)
    {
        return Some(nucleus_of_letter(vowel));
    }
    // nàjbolj, nàjmanj, nàjveč, nàjprej
    if starts_with(w, "naj") && ends_with_any(w, &["bolj", "manj", "več", "prej", "raje"]) {
        return Some(0);
    }
    // Nouns in -cija, -zija, -sija (informácija, televízija, diskúsija):
    // the syllable before; the other nouns in -íja (policíja, kemíja,
    // biologíja, Srbíja) on the i
    if ends_with_any(w, &["ija", "ije", "iji", "ijo", "ij", "ijah", "ijam", "ijami"])
        && w.len() >= 5
        && let Some(pos) = rfind(w, "ij").filter(|&pos| pos >= 2)
    {
        return Some(if matches!(w[pos - 1], 'c' | 'z' | 's') {
            syllable_before(w.len() - pos + 1)
        } else {
            nucleus_of_letter(pos)
        });
    }
    // Adjectives in -ljív, -ív (berljív, zanimív) and nouns in -íst, -áč,
    // -ár (turíst, kováč, abecedár) have the stress at the end
    if w.len() >= 5 && ends_with_any(w, &["ljiv", "iv", "ist", "ač"]) {
        return candidates.last().copied();
    }
    // -ščina on the syllable before (angléščina); -ína, -íšče, -ílo,
    // -ízem, -átor, -úra, -ácija, -étje, -álen on the suffix vowel
    if ends_with_any(w, &["ščina", "ščine", "ščini", "ščino"]) {
        return Some(syllable_before(5));
    }
    const ON_SUFFIX: &[&str] = &[
        "ina", "ine", "ini", "ino", "inah", "inam", "išče", "išča", "išču", "iščem", "išč", "ilo", "ila", "ilu",
        "ilom", "izem", "izma", "izmu", "izmom", "izmi", "ator", "atorja", "atorju", "atorjem", "atorji", "atorjev",
        "ura", "ure", "uri", "uro", "urah", "uram", "etje", "etja", "etju", "etjem", "alen", "alna", "alno", "alni",
        "alne", "alnega", "alnemu", "alnem", "alnih", "alnim", "alnimi", "ota", "ote", "oti", "oto",
    ];
    for suffix in ON_SUFFIX {
        let letters = suffix.chars().count();
        // Not a stem without a vowel
        if ends_with(w, suffix) && w.len() >= letters + 2 && last_vowel_before(w.len() - letters).is_some() {
            return Some(nucleus_of_letter(w.len() - letters));
        }
    }
    // -ica (déklica, knjížnica, učíteljica), -ičen (ístičen) and the
    // other suffixes of two syllables: the syllable before them
    const ICA: &[&str] = &[
        "ica", "ice", "ici", "ico", "icah", "icam", "icami", "ičen", "ična", "ično", "ični", "ične", "ičnega",
        "ičnemu", "ičnem", "ičnih", "ičnim",
    ];
    if ends_with_any(w, ICA) && syllables >= 3 {
        return Some(antepenult);
    }
    // Default: the second-last syllable
    Some(penult)
}

/// Settles what depends on the stress: the length and quality of the
/// stressed vowel when no mark gave them, the schwa of an unstressed final
/// -er, -el, -en, -ek, -ec, -ev, and the l at the end of a word.
fn finish_word(a: &Analysis, wp: &mut WordPhones) {
    let w = &a.plain[..];
    let n = w.len();
    let Some(stress) = wp.stress else { return };
    let (Some(&stressed_index), Some(&last_nucleus)) = (wp.nuclei.get(stress), wp.nuclei.last()) else { return };
    let stressed_letter = wp.letter.get(stressed_index).copied().unwrap_or(0);
    let sflags = a.flag(stressed_letter);
    let Some(stressed) = wp.phones.get_mut(stressed_index) else { return };

    // Length: a stressed vowel is long unless the lexicon says short; the
    // schwa is always short
    if sflags & F_STRESS == 0 {
        // Stress by rule: long, and a close e or o (the commoner quality of
        // the long stressed e and o, and that of every loanword: § 222)
        if stressed.ph != Ph::Ax {
            stressed.is_long = true;
        }
        if stressed.ph == Ph::Eh && sflags & (F_OPEN | F_SCHWA) == 0 {
            stressed.ph = Ph::Ee;
        }
        if stressed.ph == Ph::Oh && sflags & F_OPEN == 0 {
            stressed.ph = Ph::Oo;
        }
    }
    if stressed.ph == Ph::Ax {
        stressed.is_long = false;
    }

    // Letter names (bé, cé; ef, èl, ès; á é í ó ú): a long close vowel
    // when the name ends in it, short and open before a consonant (§ 7)
    if a.letter_name && sflags & F_STRESS == 0 {
        let final_vowel = stressed_letter + 1 == n;
        stressed.is_long = final_vowel;
        stressed.ph = match stressed.ph {
            Ph::Eh | Ph::Ee if final_vowel => Ph::Ee,
            Ph::Eh | Ph::Ee => Ph::Eh,
            Ph::Oh | Ph::Oo if final_vowel => Ph::Oo,
            Ph::Oh | Ph::Oo => Ph::Oh,
            ph => ph,
        };
    }

    // The schwa of an unstressed final syllable -eC after a consonant:
    // veter, misel, lačen, petek, konec, cerkev (Slovenski pravopis: the
    // "polglasnik" written e). Not where the lexicon spelled the vowel.
    let letter = wp.letter.get(last_nucleus).copied().unwrap_or(0);
    if n >= 3 && letter >= a.notation_letters && letter + 2 == n && w[letter] == 'e' && last_nucleus != stressed_index {
        let before = w[letter - 1];
        // -ev only after k and t (cerkev, bukev, molitev): the genitive
        // plural -ev (delavcev, stricev) keeps its e; -em only after z
        // and t (absolutizem, algoritem): the instrumental -em
        // (delavcem, možem) keeps its e
        let fleeting = match w[letter + 1] {
            'r' | 'l' | 'n' | 'k' | 'c' => true,
            'v' => matches!(before, 'k' | 't'),
            'm' => matches!(before, 'z' | 't'),
            _ => false,
        };
        if fleeting && !is_vowel_letter(before) && before != 'j' {
            wp.phones[last_nucleus].ph = Ph::Ax;
            wp.phones[last_nucleus].is_long = false;
        }
    }

    // l at the end of a word after a vowel (§ 650): [w] in the participles
    // (bil, dal, delal, videl, nosil), after a schwa (rekel, misel, topel)
    // and in the nouns and adjectives of the list (stol, sol, bel, vesel);
    // [l] otherwise, as in the loanwords (hotél, kanál, avtomobíl, apríl,
    // modél). A word with no stress on the last syllable, or of one syllable,
    // looks like a participle: those that are not, whatever the vowel, are in
    // the table FINAL_L, from the transcriptions of Sloleks without the words
    // SSKJ² and SP mark [-u̯] (ángel, módul, stíl, kábel, ansámbel, síl, žíl,
    // škátel), and so is a capitalized word in no lexicon, nearly always a
    // foreign name (Michael, Paul, Excel). A notation l follows the rule too
    // (the lexicon writes w only where the rule would say l).
    if let [.., before, 'l'] = *w
        && is_vowel_letter(before)
        && wp.phones.last().is_some_and(|last| last.ph == Ph::L)
    {
        let final_stress = stress + 1 == wp.nuclei.len();
        let monosyllable = wp.nuclei.len() == 1;
        let schwa = wp.phones[last_nucleus].ph == Ph::Ax;
        let participle_shape = before != 'o';
        let listed_w = l_final_w_word(w);
        let final_l_form = || table::contains(FINAL_L, &w.iter().collect::<String>());
        let keeps_l =
            a.flag(n - 1) & F_L != 0 || (!listed_w && (a.unknown_name || l_final_l_word(w) || final_l_form()));
        if !keeps_l
            && (schwa || listed_w || (participle_shape && (monosyllable || !final_stress)))
            && let Some(last) = wp.phones.last_mut()
        {
            last.ph = Ph::W;
        }
    }
}

/// A stem of the lexicon followed by a derivational suffix that begins with
/// an l before a consonant: the agent nouns and instruments in -lec, -lka,
/// -lnik, -lnica, -lski, -lstvo and the nouns in -lišče. The stem's accent
/// does not hold there: the stress moves to the vowel before the l
/// (zagánjati but zaganjálnik, napájati / napajálnik, bráti / brálec,
/// gledáti / gledálec, poslúšati / poslušálec, učíti / učílnica, spáti /
/// spálnica), or to the í of -lišče (glédati / gledalíšče). SSKJ gives every
/// one of these with that accent.
fn stress_derived_l_suffix(ending: &[char], a: &mut Analysis) {
    let stem = a.notation_letters;
    let [.., before] = a.plain[..stem.min(a.plain.len())] else { return };
    if ending.len() < 2 || ending[0] != 'l' || !is_vowel_letter(before) {
        return;
    }
    let target = if starts_with(ending, "lišč") {
        stem + 1 // gledalíšče, igríšče
    } else if matches!(ending[1], 'c' | 'k' | 'n' | 's' | 't') {
        stem - 1
    } else {
        return;
    };
    for f in &mut a.flags {
        *f &= !(F_STRESS | F_LONG | F_CLOSE | F_OPEN);
    }
    if let (Some(f), Some(&letter)) = (a.flags.get_mut(target), a.plain.get(target)) {
        *f |= F_STRESS | F_LONG;
        if letter == 'e' || letter == 'o' {
            *f |= F_CLOSE;
        }
    }
}

/// The pronunciation the lexicons have for a word, and how many of its
/// letters it covers (the rest is an ending read by rule): the user's exact
/// forms and stems, then the reading the clause chose, then the built-in
/// forms and stems; a compound numeral from its parts.
fn lexicon_reading(
    word: &[char],
    user: Option<&Lexicon>,
    letter_name: bool,
    pron_override: Option<&[char]>,
    capital: Capital,
) -> Option<(usize, Vec<char>)> {
    // The name of a letter is read by the rule (finish_word()), not as the
    // word it is spelled like (je, en, es).
    if !letter_name {
        if let Some(user) = user
            && let Some(found) =
                user.find_exact(word).map(|pron| (word.len(), pron)).or_else(|| user.find_stem(word, None))
        {
            return Some(found);
        }
        if let Some(pron) = pron_override {
            return Some((word.len(), pron.to_vec()));
        }
    }
    let builtin = builtin_lexicon();
    if !letter_name || word.len() > 2 {
        if let Some(pron) = builtin.find_exact(word) {
            return Some((word.len(), pron));
        }
        // A surname in -ović is read by its rule (default_stress()), not as
        // a common word's stem followed by an ending (petr*, turk*): only a
        // whole form of the lexicon decides it. A name inside a clause takes
        // a stem only with an ending of the declension (Kovačiča: kováčič* +
        // a), not another word's beginning (Ferdolenc: férd* of Ferdo +
        // olenc)
        let name_letter = has_name_letter(word);
        let surname = capital != Capital::None || name_letter;
        if !(surname && vic_surname(word).is_some()) {
            let accept: Option<fn(&[char]) -> bool> =
                (capital == Capital::Inside || name_letter).then_some(declension_ending);
            if let Some(found) = builtin.find_stem(word, accept) {
                return Some(found);
            }
        }
    }
    if !letter_name
        && let Some((ones, tens)) = split_compound_numeral(word)
        && let Some(tens_pron) = builtin.find_exact(&tens)
    {
        return Some((word.len(), [&ones[..], &['i', 'n'], &tens_pron[..]].concat()));
    }
    None
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
    word: &[char],
    marks: &[u8],
    user: Option<&Lexicon>,
    letter_name: bool,
    pron_override: Option<&[char]>,
    capital: Capital,
) -> WordPhones {
    let mut out = WordPhones::default();
    let mut a;
    // The syllables whose letter has the stress; the last accent of a
    // compound (àbiótski, nàjlépši) is the main one
    let marked_stress = |a: &Analysis, out: &WordPhones, letters: usize| {
        (0..out.nuclei.len()).rev().find(|&k| {
            let letter = out.nucleus_letter(k);
            letter < letters && a.flag(letter) & F_STRESS != 0
        })
    };
    let prothetic;
    if marks.iter().any(|&mark| mark != 0) {
        // 1. Accent marks written in the text. Slovenian dictionaries' marks:
        // acute = long (close e, o), circumflex = long open e, o, grave = short.
        a = Analysis { letter_name, ..Analysis::by_rule(word) };
        for ((&c, &mark), f) in word.iter().zip(marks).zip(&mut a.flags) {
            if mark & MARK_STRESS != 0 {
                *f |= F_STRESS;
            }
            if mark & MARK_LONG != 0 {
                *f |= F_LONG;
            }
            if c == 'e' || c == 'o' {
                if mark & MARK_RISE != 0 && mark & MARK_LONG != 0 {
                    *f |= F_CLOSE; // é ó
                } else if mark & MARK_STRESS != 0 {
                    *f |= F_OPEN; // ê ô è ò
                }
            }
        }
        prothetic = letters_to_phones(&a, &mut out);
        out.stress = marked_stress(&a, &out, word.len());
        out.from_lexicon = true;
    } else {
        // 2. The lexicon
        let found = lexicon_reading(word, user, letter_name, pron_override, capital).and_then(|(length, pron)| {
            let (plain, flags) = read_notation(&pron).ok()?;
            Some((length, plain, flags))
        });
        if let Some((stem_length, mut plain, mut flags)) = found {
            let ending = word.get(stem_length..).unwrap_or_default();
            let notation_letters = plain.len();
            plain.extend_from_slice(ending);
            flags.resize(plain.len(), 0);
            a = Analysis { plain, flags, notation_letters, letter_name, unknown_name: false };
            out.from_lexicon = true;
            if !ending.is_empty() {
                stress_derived_l_suffix(ending, &mut a);
            }
            // A stem and the ending -l alone is the l-participle: [w], also
            // after a stem that ends in its stressed vowel (želé*, sledí*,
            // učí*, predá*: Slovenski pravopis 2001, § 650 and the
            // transcriptions of Sloleks). finish_word() keeps [l] after a
            // final stress for the loanwords, whose stem has the l (kanál*,
            // hotél*); without this the participles were read like them.
            // A participle read by the stem of its other forms (sledíl*) is
            // listed whole with its w by the generator.
            if let [.., before, 'l'] = *word
                && stem_length + 1 == word.len()
                && is_vowel_letter(before)
                && before != 'o'
                && a.flags.len() == word.len()
                && let Some(f) = a.flags.last_mut()
            {
                *f |= F_W;
            }
        } else {
            a = Analysis { letter_name, unknown_name: capital != Capital::None, ..Analysis::by_rule(word) };
        }
        prothetic = letters_to_phones(&a, &mut out);
        out.stress = marked_stress(&a, &out, a.notation_letters);
    }
    if out.stress.is_none() {
        out.stress = default_stress(&a.plain, &out, &prothetic, capital);
    }
    finish_word(&a, &mut out);
    // The analysis's letters may differ from the word's (ə, w are the word's
    // e, l): the letter indexes refer to the spelling as given and the plain
    // letters match it one to one, so nothing to map. The stressed flag
    // itself is set by the front end, which knows whether the word is a
    // clitic.
    out
}

/// Sound changes across a clause: regressive voicing assimilation, final
/// devoicing, n before k/g, the merging of identical consonants over a
/// clitic boundary. `word_is_clitic` says, by the word index of the phones,
/// whether the word is a clitic that is spoken with its neighbour without a
/// word break.
pub fn assimilate(phones: &mut Vec<Phone>, word_is_clitic: &[bool]) {
    let clitic = |phone: &Phone| word_is_clitic.get(usize::from(phone.word)).copied().unwrap_or(false);
    // Whether two neighbours are spoken without a word break
    let bound = |left: &Phone, right: &Phone| left.word == right.word || clitic(left) || clitic(right);
    let voiced_obstruent = |phone: &Phone| phone.ph.is_obstruent() && phone.ph.def().voiced;

    // Final devoicing: an obstruent at the end of a word, before a pause or
    // before a word that begins with anything but a voiced obstruent, is
    // voiceless (grad [grat], mož [moš], grozd [grost]); before a voiced
    // obstruent of the next word it stays voiced, and so do the obstruents
    // before it (Slovenski pravopis 2001, § 650-653). The clitic
    // prepositions (v, z, k, od, pod) take the voicing of the word they
    // lean on below.
    for i in 0..phones.len() {
        let this = phones[i];
        let next = phones.get(i + 1).copied();
        if next.is_some_and(|next| next.word == this.word) || !voiced_obstruent(&this) {
            continue;
        }
        if next.is_some_and(|next| voiced_obstruent(&next)) {
            continue;
        }
        // A preposition is spoken as part of the word after it: od orača,
        // iz okna, ob materi keep the d, z, b (§ 678)
        if next.is_some_and(|next| clitic(&this) && !clitic(&next) && !next.ph.is_obstruent()) {
            continue;
        }
        for phone in phones[..=i].iter_mut().rev() {
            if !phone.ph.is_obstruent() || phone.word != this.word {
                break;
            }
            phone.ph = voiceless_of(phone.ph);
        }
    }

    for i in (1..phones.len()).rev() {
        let right = phones[i];
        let left = &mut phones[i - 1];
        if !bound(left, &right) {
            continue;
        }
        // Regressive voicing assimilation between obstruents (odkod
        // [otkot], s bratom [zbratom], k gradu written h gradu)
        if left.ph.is_obstruent() && right.ph.is_obstruent() {
            left.ph = if right.ph.def().voiced { voiced_of(left.ph) } else { voiceless_of(left.ph) };
        }
        // s, z before š, ž, č, dž are š, ž (s časom [ščasom], iz šole
        // [išole]: § 696)
        if matches!(right.ph, Ph::Sh | Ph::Zh | Ph::Ch | Ph::Dzh) {
            left.ph = match left.ph {
                Ph::S => Ph::Sh,
                Ph::Z => Ph::Zh,
                ph => ph,
            };
        }
        // Nasal place: n before k, g, h is velar (Anka, Angela, Anhovo: § 638)
        if left.ph == Ph::N && matches!(right.ph, Ph::K | Ph::G | Ph::H | Ph::Gh) {
            left.ph = Ph::Ng;
        }
    }

    // Two identical consonants across a clitic boundary merge (iz sela, od
    // toga, s sestro)
    let mut i = 0;
    while i + 1 < phones.len() {
        let (left, right) = (phones[i], phones[i + 1]);
        if left.ph == right.ph && left.ph.is_obstruent() && left.word != right.word && bound(&left, &right) {
            phones[i + 1].word_start |= left.word_start;
            phones.remove(i);
        } else {
            i += 1;
        }
    }
}
