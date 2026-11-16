//! Tests of the shared machinery: the letters and their names, the user's
//! accent lexicon, and that no text makes a front end panic.

use super::letters::{chars, to_lower};
use super::notation::parse_lex_entry;
use super::{UserLexicon, is_foreign_letter, is_letter_name, letter_name, spelling_letter};
use crate::formant::phones::{Accent, Ph};
use crate::language::Language;
use crate::languages::Frontend;
use crate::text::{Frontend as _, Punctuation};

fn string(letters: &[char]) -> String {
    letters.iter().collect()
}

#[test]
fn a_spelled_character_is_a_letter_of_the_alphabet() {
    for (c, letter) in [
        ('B', "b"),
        ('č', "č"),
        ('Š', "š"),
        ('Љ', "lj"),
        ('џ', "dž"),
        ('ǆ', "dž"),
        ('Ǌ', "nj"),
        ('ä', "e"),
        ('Ö', "e"),
        ('ȁ', "a"),
        ('Á', "a"),
        ('х', "h"),
        ('q', "q"),
        ('ß', "s"),
    ] {
        assert_eq!(string(&spelling_letter(c)), letter, "{c}");
    }
    // Not letters, or more than one letter (ř is rž, ѕ is dz)
    for c in ['1', '.', ' ', '@', 'ř', 'ѕ', '\u{301}', '€', 'щ'] {
        assert!(spelling_letter(c).is_empty(), "{c}");
    }
    assert_eq!((to_lower('Đ'), to_lower('Ž'), to_lower('Ȁ'), to_lower('ž'), to_lower('×')), ('đ', 'ž', 'ȁ', 'ž', '×'));
}

#[test]
fn letters_have_the_names_of_their_language() {
    use Language::{Bosnian, Croatian, Serbian};
    for (letter, language, name) in [
        ("b", Croatian, "be"),
        ("lj", Croatian, "elj"),
        ("lj", Bosnian, "elj"),
        ("lj", Serbian, "lje"),
        ("nj", Serbian, "nje"),
        ("š", Serbian, "ša"),
        ("š", Croatian, "eš"),
        ("dž", Serbian, "dže"),
        ("w", Croatian, "duplo ve"),
        ("y", Serbian, "ipsilon"),
        ("1", Croatian, ""),
        ("ab", Croatian, ""),
    ] {
        assert_eq!(string(&letter_name(&chars(letter), language)), name, "{letter}");
    }
    assert!(is_foreign_letter(&['x']) && is_foreign_letter(&['w']) && !is_foreign_letter(&['v']));
    for (letter, text, is_name) in [
        ("b", "be", true),
        ("b", " Be ", true),
        ("š", "ša", true),
        ("š", "EŠ", true),
        ("j", "jot", true),
        ("j", "je", true),
        ("b", "jot", false),
        ("b", "bravo", false),
        ("b", "   ", false),
        ("1", "", false),
    ] {
        assert_eq!(is_letter_name(&chars(letter), &chars(text)), is_name, "{letter} {text:?}");
    }
}

#[test]
fn lexicon_entries_are_read_and_checked() {
    let entry = parse_lex_entry(&chars("Kontr'o:l*")).expect("a stem");
    assert_eq!((string(&entry.plain).as_str(), entry.stem), ("kontrol", true));
    assert_eq!(
        (entry.entry.stress_letter, entry.entry.accent, entry.entry.long_letters),
        (Some(5), Accent::None, 1 << 5)
    );
    let entry = parse_lex_entry(&chars("d^a:n")).expect("a word");
    assert_eq!((entry.entry.stress_letter, entry.entry.accent, entry.stem), (Some(1), Accent::Falling, false));
    for (marked, reason) in [
        ("", "no letters"),
        ("kabina", "no stress or length mark"),
        ("k'ab'ina", "two stress marks"),
        ("kab:ina", "length mark not after a vowel"),
        ("k'abin**", "two stem marks"),
        ("k'abin*a", "letters after the stem mark"),
        ("k'ab1na", "not a letter"),
        ("kabina'", "stress mark after the last letter"),
    ] {
        assert_eq!(parse_lex_entry(&chars(marked)).err(), Some(reason), "{marked}");
    }
}

#[test]
fn the_users_lexicon_counts_what_it_takes_and_what_it_skips() {
    // A comma after the last entry and a comment inside a string do not
    // lose the entries
    let json = r#"{ "version": "1.0", "entries": [
        { "word": "balk'o:n*", "comment": "balkón, \"word\" } ]" },
        { "word": "balz'a:m|a|u|om" },
        { "word": "kabina" },
        { "verb": "xyz" },
        { "verb": "zatr'ub=i" },
        { "verb": "dijel:i", "language": "hr" },
        { "word": "kami'o:n*", "language": "sr" },
        { "word": "kami'o:n*", "language": "xx" },
        { "word": "a", "verb": "b" },
        { "comment": "nothing" },
    ] }"#;
    let (lexicon, report) = UserLexicon::parse(json);
    assert_eq!((report.words, report.verbs, report.rejected), (5, 2, 5));
    assert_eq!(report.first_error, "kabina: no stress or length mark");
    assert!(!lexicon.is_empty());

    for (json, error) in [
        (r#"{"entries":[{"verb":"xyz"}]}"#, "xyz: expected root:classes[:prefixes] with a long ije (i a t e)"),
        (r#"{"entries":[{"verb":"zatrub=i"}]}"#, "zatrub=i: expected st'e:m=classes (i a t u e n < p P)"),
        (r#"{"entries":[{"word":"a'a","language":"de"}]}"#, "de: unknown language (hr, sr, bs, sl)"),
        (r#"{"entries":[{"word":"a'a","verb":"b"}]}"#, "a'a: both word and verb"),
        (r#"{"entries":[{"verb":"dijel:i","language":"sl"}]}"#, "dijel:i: Slovenian has word entries only"),
        (r#"{"entries":[{"comment":"x"}]}"#, r#"{"comment":"x"}: neither word nor verb"#),
    ] {
        let (lexicon, report) = UserLexicon::parse(json);
        assert_eq!((report.rejected, report.first_error.as_str()), (1, error));
        assert!(lexicon.is_empty());
    }
    for json in [
        "",
        "{}",
        "[",
        r#"{"entries":"#,
        r#"{"entries":[{"word":"k'a"#,
        "\"entries\"[{{{{",
        r#"{"entries":[{"word":"\u12"#,
    ] {
        let (lexicon, report) = UserLexicon::parse(json);
        assert!(lexicon.is_empty() && report.words == 0 && report.verbs == 0, "{json}");
    }
}

#[test]
fn a_letter_alone_is_its_sound() {
    let frontend = Frontend::new(Language::Croatian);
    let sound = |letter: &str| frontend.letter_sound(&chars(letter)).phones.iter().map(|p| p.ph).collect::<Vec<_>>();
    assert_eq!(sound("b"), [Ph::B, Ph::Schwa]);
    assert_eq!(sound("dž"), [Ph::Dzh, Ph::Schwa]);
    assert_eq!(sound("a"), [Ph::A]);
    for letter in ["q", "w", "x", "y", "1", "ab", ""] {
        assert!(sound(letter).is_empty(), "{letter}");
    }
    let utt = frontend.letter_sound(&['s']);
    assert!(utt.isolated_sound && utt.syllable_count == 1 && utt.phones[0].word_start && utt.phones[1].stressed);
    assert_eq!(Frontend::new(Language::Slovenian).letter_sound(&['e']).phones[0].ph, Ph::Ee);
}

/// Texts of every kind a host may send: letters of several scripts with and
/// without accent marks, the engine's own marks, symbols, pieces the rules
/// look for, very long words. Whatever the front ends make of them, they
/// must return.
#[test]
fn no_text_makes_a_front_end_panic() {
    const PIECES: &[&str] = &[
        " ",
        " ",
        " ",
        ".",
        ":",
        "://",
        "-",
        "'",
        "\"",
        "„",
        "(",
        "!",
        "%",
        "€",
        "\n",
        "\t",
        "1",
        "2026",
        "\u{FDD0}",
        "\u{FDD1}",
        "\u{FDD2}",
        "\u{E000}",
        "\u{E001}",
        "\u{E002}",
        "\u{E003}",
        "\u{301}",
        "\u{30F}",
        "\u{304}",
        "a",
        "e",
        "i",
        "o",
        "u",
        "r",
        "s",
        "k",
        "v",
        "w",
        "x",
        "q",
        "y",
        "lj",
        "nj",
        "dž",
        "A",
        "B",
        "S",
        "K",
        "Ž",
        "Đ",
        "Ǆ",
        "ǈ",
        "á",
        "ȁ",
        "ȓ",
        "ŕ",
        "ī",
        "ß",
        "ř",
        "Љ",
        "џ",
        "ђ",
        "Њ",
        "щ",
        "ije",
        "tiflo",
        "naj",
        "ne",
        "po",
        "raz",
        "su",
        "godišnj",
        "mjeseč",
        "ljet",
        "laz",
        "laž",
        "teljstvo",
        "ljiv",
        "irati",
        "ović",
        "ica",
        "ina",
        "anin",
        "ac",
        "itet",
        "ator",
        "uj",
        "em",
        "ti",
        "mi",
        "li",
        "se",
        "boli",
        "sati",
        "nada",
        "veseli",
        "zraka",
        "pet",
        "novi",
        "sad",
        "www",
        "https",
        "tion",
        "ing",
        "ed",
        "ck",
        "th",
        "Labs",
        "Tik",
        "Tok",
        "USB",
        "dr",
        "prst",
        "krv",
        "podijeli",
        "uredi",
        "pročitan",
        "meni",
        "lahko",
        "enaindvajset",
        "tisuća",
        "hiljada",
        "znam",
    ];
    let (user, _) = UserLexicon::parse(
        r#"{"entries":[{"word":"tifl'o*"},{"word":"s'a:t|i|u"},{"verb":"ur'e:d=i<pn"},{"verb":"dijel:i"},
            {"word":"m'é:sto","language":"sl"}]}"#,
    );
    // A linear congruential generator: the same texts every run
    let mut state = 0x2545_F491_4F6C_DD1D_u64;
    let mut next = |below: usize| {
        state = state.wrapping_mul(6_364_136_223_846_793_005).wrapping_add(1_442_695_040_888_963_407);
        (state >> 33) as usize % below
    };
    let mut frontends: Vec<Frontend> = Language::ALL.into_iter().map(Frontend::new).collect();
    for frontend in frontends.iter_mut().skip(1) {
        frontend.set_user_lexicon(Some(user.clone()));
    }
    frontends.push({
        let mut croatian = Frontend::new(Language::Croatian);
        croatian.set_user_lexicon(Some(user.clone()));
        croatian
    });
    let puncts = [Punctuation::Period, Punctuation::Question, Punctuation::Comma, Punctuation::Exclamation];
    for round in 0..2000 {
        let pieces = if round % 250 == 249 { 400 } else { next(14) };
        let text: Vec<char> = (0..pieces).flat_map(|_| PIECES[next(PIECES.len())].chars()).collect();
        for (i, frontend) in frontends.iter().enumerate() {
            let utt = frontend.process(&text, puncts[(round + i) % puncts.len()], round % 2 == 0);
            assert!(utt.phones.iter().all(|p| i32::from(p.syllable) <= utt.syllable_count));
            let _ = crate::prosody::split_at_breaks(&utt);
        }
        if let Some(&c) = text.first() {
            let letter = spelling_letter(c);
            for frontend in &frontends {
                let _ = frontend.letter_sound(&letter);
                let _ = frontend.letter_sound(&text);
            }
            for language in Language::ALL {
                let _ = is_letter_name(&letter, &text);
                let _ = letter_name(&text, language);
            }
        }
    }
}
