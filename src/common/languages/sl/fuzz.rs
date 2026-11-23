//! Awkward text through the Slovenian front end. The engine speaks whatever a
//! screen reader hands it, so nothing here may panic, whatever the letters,
//! the marks and the words around a word are. The texts are drawn by a small
//! generator with a fixed seed: every run reads the same ones.

use super::frontend::process;
use super::lexicon::{Lexicon, builtin_lexicon};
use super::marks::{is_address, opens_complement_clause};
use super::phonemize::{Capital, assimilate, phonemize};
use super::readings::{self, Clause};
use super::words;
use crate::text::Punctuation;
use crate::text::frontend::UserLexicon;

/// xorshift64*: deterministic, and good enough to mix words.
struct Rng(u64);

impl Rng {
    fn next(&mut self) -> u64 {
        self.0 ^= self.0 >> 12;
        self.0 ^= self.0 << 25;
        self.0 ^= self.0 >> 27;
        self.0.wrapping_mul(0x2545_F491_4F6C_DD1D)
    }

    fn below(&mut self, n: usize) -> usize {
        usize::try_from(self.next() % (n.max(1) as u64)).unwrap_or(0)
    }

    fn pick<'a, T>(&mut self, items: &'a [T]) -> &'a T {
        &items[self.below(items.len())]
    }
}

/// The words the rules of readings look for, clitics, names, numerals,
/// spellings with a `=` entry or a stem, and strings no rule expects.
#[rustfmt::skip] // rows of words
const WORDS: &[&str] = &[
    "meni", "lahko", "je", "samo", "roka", "roki", "roke", "glasu", "miru", "gradu", "vprašaj", "gori", "leti",
    "veseli", "afna", "lepo", "težko", "obvestil", "vozil", "predal", "žalosti", "sledi", "ni", "ne", "se", "si", "me",
    "ga", "mi", "ti", "bo", "bi", "so", "sem", "bilo", "bil", "v", "na", "z", "s", "k", "h", "pod", "za", "do", "brez",
    "in", "pa", "ali", "da", "tudi", "zelo", "veliko", "rad", "start", "nvda", "gumb", "glavni", "zaprti", "odpri",
    "pojdi", "narekovaj", "dvojni", "začetni", "oklepaj", "zaklepaj", "ena", "dve", "enaindvajset", "petinpetdesetimi",
    "dvakrat", "prvi", "torbo", "hrano", "delo", "noč", "krompir", "juho", "kosilo", "z", "vilicami", "pero", "janez",
    "vesna", "smode", "stare", "petrović", "stojanovićevega", "zupančič", "dolenc", "bogomir", "gabriel", "albina",
    "jurij", "ferdolenc", "sobota", "murska", "živijo", "naredi", "posodobi", "govori", "menu", "laprdus", "internetom",
    "hotel", "stol", "bralec", "gledališče", "zaganjalnik", "študirati", "vsak", "vrt", "rdeč", "prst", "ljubljana",
    "konj", "dž", "lj", "nj", "r", "l", "v", "e", "a", "www", "https", "usb", "ž", "ć", "đ", "q", "w", "x", "y", "ir",
    "ij", "naj", "najbolj", "ejši", "ši", "ovati", "ovc", "ič", "ic", "ović", "iel", "al", "il", "ol", "el", "lišč",
    "krat", "inkrat", "eti", "niti", "ati", "ščina", "ica", "ota", "c", "j",
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "rrrrrr", "vvvv", "lll", "ieaou", "ljljlj", "mésto",
    "mêsto", "mèsto", "mestó", "àbiótski", "p\u{259}s", "e\u{300}\u{301}\u{302}\u{304}", "\u{301}a", "ł", "ə", "Meni",
    "MENI", "Samo", "Roka", "Glasu", "Lahko", "Pero", "NVDA", "SDS", "Je", "V", "S", "Z", "Č", "Đ", "Ǆ", "ǈ", "мени",
    "Њ", "ß", "ñ", "ÿ", "İ", "ı", "ﬁ", "𝓐", "😀", "0", "7", "21", "3.14", "12:30", "5kg", "A17",
];

/// What stands between and around words.
#[rustfmt::skip] // rows of words
const BETWEEN: &[&str] = &[
    " ", " ", " ", "  ", "\t", "-", "–", ".", ":", "!", "?", ",", ";", "'", "\"", "(", ")", "„", "“", "»", "«", "/",
    "@", "%", "€", "°", "+", "=", "&", "\u{2024}", "\u{FDD0}", " \u{FDD0} ", "\u{FDD1}", " \u{FDD2} ", "\u{E000}",
    "\u{E001}", "\u{E002}", "\u{E003}", "\u{E004}", "\u{E010}", "\u{0}", "\u{A0}", "\u{AD}", "\u{200B}", "\u{300}", "\u{301}",
    "\u{FFFD}", "",
];

const PUNCTUATION: &[Punctuation] = &[
    Punctuation::None,
    Punctuation::Comma,
    Punctuation::Period,
    Punctuation::Question,
    Punctuation::Exclamation,
    Punctuation::Semicolon,
    Punctuation::Colon,
];

/// A clause of up to nine words of the list, some of them cut or doubled.
fn clause(rng: &mut Rng) -> Vec<char> {
    let mut text = String::new();
    for _ in 0..rng.below(10) {
        let word = *rng.pick(WORDS);
        match rng.below(12) {
            0 => text.extend(word.chars().take(rng.below(4))),
            1 => text.extend(word.chars().rev()),
            2 => text.extend(word.chars().chain(rng.pick(WORDS).chars())),
            3 => text.push_str(&word.to_uppercase()),
            4 => text.extend(word.chars().enumerate().map(|(k, c)| if k == 0 { c.to_ascii_uppercase() } else { c })),
            _ => text.push_str(word),
        }
        text.push_str(rng.pick(BETWEEN));
    }
    for _ in 0..rng.below(3) {
        text.push_str(rng.pick(BETWEEN));
    }
    text.chars().collect()
}

#[test]
fn awkward_clauses_do_not_panic() {
    // The user's entries: stems, form lists, a spelling shorter and one
    // longer than its pronunciation, and entries that are refused
    let (user, report) = UserLexicon::parse(
        r#"{"entries":[{"word":"mení","language":"sl"},{"word":"lahk*","language":"sl"},
        {"word":"j|è|é","language":"sl"},{"word":"a=abecéda","language":"sl"},
        {"word":"enaindvajset=èn","language":"sl"},{"word":"petrović=pétrović*","language":"sl"},
        {"word":"v=və̀","language":"sl"},{"word":"stół","language":"sl"},{"word":"gor'i*"},
        {"word":"=","language":"sl"},{"word":"*","language":"sl"},{"word":"a|","language":"sl"}]}"#,
    );
    assert_eq!((report.words, report.rejected), (9, 3), "{}", report.first_error);
    let mut rng = Rng(0x5EED_1234_5678_9ABC);
    for round in 0..3000 {
        let text = clause(&mut rng);
        let punct = *rng.pick(PUNCTUATION);
        let user = (round % 3 == 0).then_some(&user.slovenian);
        for letter_sounds in [false, true] {
            let utt = process(&text, punct, letter_sounds, user);
            assert!(utt.slovenian);
            // The words of the phones are in order, and every stressed phone
            // carries a syllable (the r of a syllabic r with its schwa)
            assert!(utt.phones.windows(2).all(|pair| pair[0].word <= pair[1].word), "{text:?}");
            assert!(utt.phones.iter().all(|phone| phone.nucleus || phone.nucleus_tail || !phone.stressed), "{text:?}");
        }
    }
    // Long runs of one thing
    for part in ["a", "r", "v ", "je ", "meni ", "samo je ", "narekovaj ", "\u{301}", "\u{E000}", "ǆ", "A17 ", "- "] {
        let text: Vec<char> = part.repeat(700).chars().collect();
        process(&text, Punctuation::Question, true, None);
    }
}

#[test]
fn word_functions_take_any_letters() {
    const LETTERS: &[char] = &[
        'a', 'e', 'i', 'o', 'u', 'r', 'l', 'v', 'j', 'n', 'c', 'č', 'ć', 'đ', 'š', 'ž', 'd', 'k', 't', 's', 'h', 'm',
        'q', 'w', 'x', 'y', 'ə', 'ł', 'é', 'ô', 'à', 'ŕ', '\'', ':', '*', '|', '=', ' ', '\u{300}', '\u{304}', 'A',
        '1', '\0', '😀',
    ];
    const CAPITALS: &[Capital] = &[Capital::None, Capital::AllCaps, Capital::Head, Capital::Inside];
    let mut rng = Rng(0x0DD_C0FFEE);
    let mut user = Lexicon::default();
    let word = |rng: &mut Rng, longest: usize| -> Vec<char> {
        if rng.below(3) == 0 {
            rng.pick(WORDS).chars().collect()
        } else {
            (0..rng.below(longest)).map(|_| *rng.pick(LETTERS)).collect()
        }
    };
    for round in 0..20_000 {
        let w = word(&mut rng, 9);
        // The lexicon takes or refuses anything, and finds only what it has
        user.add(&w);
        let _ = Lexicon::parse(&w);
        builtin_lexicon().find_exact(&w);
        builtin_lexicon().find_stem(&w, None);
        user.find_stem(&w, Some(|ending| ending.len() == 1));
        // Marks of any length, with and without a reading of the clause
        let marks: Vec<u8> = (0..rng.below(w.len() + 3)).map(|_| [0, 0, 0, 1, 3, 5, 11, 255][rng.below(8)]).collect();
        let reading = word(&mut rng, 9);
        let capital = *rng.pick(CAPITALS);
        let lexicon = (round % 2 == 0).then_some(&user);
        let mut phones = phonemize(&w, &marks, lexicon, round % 7 == 0, (round % 5 == 0).then_some(&reading), capital);
        assert_eq!(phones.phones.len(), phones.letter.len());
        assert!(phones.stress.is_none_or(|stress| stress < phones.nuclei.len()), "{w:?}");
        assert!(phones.nuclei.iter().all(|&at| phones.phones[at].nucleus), "{w:?}");
        // Word indexes the list of clitics does not have
        for (k, phone) in phones.phones.iter_mut().enumerate() {
            phone.word = u16::try_from(k / 2).unwrap_or(0);
        }
        assimilate(&mut phones.phones, &[true, false]);

        readings::name_reading(&w, round % 2 == 0);
        readings::surname_reading(&w, &reading, round % 2 == 0);
        readings::head_reading(&w);
        readings::verb_noun_readings(&w);
        readings::split_compound_numeral(&w);
        words::letter_name(&w);
        is_address(&w);
        opens_complement_clause(&w);

        // A clause whose lists of capitals are shorter than its words
        let clause_words: Vec<Vec<char>> = (0..rng.below(8)).map(|_| word(&mut rng, 6)).collect();
        let flags: Vec<bool> = (0..rng.below(9)).map(|k| (round + k) % 3 == 0).collect();
        let clause = Clause {
            words: &clause_words,
            capitalized: &flags,
            acronym: &flags[..flags.len() / 2],
            before_complement: round % 2 == 0,
            question: round % 3 == 0,
            exclamation: round % 5 == 0,
            request: round % 7 == 0,
        };
        for i in 0..clause_words.len() + 2 {
            if let Some(reading) = readings::clause_reading(&clause, i) {
                readings::clause_reading_is_content(clause_words.get(i).map_or(&[], Vec::as_slice), reading);
            }
            readings::context_reading(&clause, i, round % 2 == 0, (round % 4 != 0).then_some((round % 101) as u8));
        }
    }
    assert!(words::spelled_name("").is_none() && words::symbol_name('\0').is_none());
}

#[test]
fn the_generated_tables_are_sorted_and_whole() {
    use crate::text::table;
    let tables = [
        (include_str!("tables/final_l.txt"), 300),
        (include_str!("tables/given_names.txt"), 842),
        (include_str!("tables/definite_adjectives.txt"), 1479),
        (include_str!("tables/food.txt"), 684),
        (include_str!("tables/verb_noun.txt"), 941),
    ];
    for (text, count) in tables {
        assert_eq!(text.lines().count(), count);
        for line in text.lines() {
            let (key, value) = line.split_once('\t').unwrap_or((line, ""));
            assert_eq!(table::lookup(text, key), Some(value), "{key}");
        }
    }
    let chars = |s: &str| s.chars().collect::<Vec<char>>();
    assert!(readings::is_given_name(&chars("vesna")) && !readings::is_given_name(&chars("smode")));
    let obvestil = readings::verb_noun_readings(&chars("obvestil")).expect("obvestil is a verb and a noun");
    assert_eq!((obvestil.participle, obvestil.noun_percent.is_some()), (true, true));
    let veseli = readings::verb_noun_readings(&chars("veseli")).expect("veseli is a verb and an adjective");
    assert_eq!((veseli.verb, veseli.nominal), (chars("veselí"), chars("veséli")));
    assert_eq!((veseli.participle, veseli.noun_percent), (false, None));
    assert_eq!(readings::name_reading(&chars("sobota"), true), None);
    assert_eq!(readings::head_reading(&chars("posodobi")).map(|head| head.pron), Some(chars("posodôbi")));
}
