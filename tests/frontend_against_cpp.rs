//! The text front end against the C++ engine: for every text of the golden
//! corpus, in every language and both spelling modes, the phones of the
//! clause must be those a dump program built from the C++ engine (removed with it) wrote.
//!
//! The reference, `frontend_full.tsv`, is too large for the repository:
//! `tools/golden/golden.py` writes it to `build/golden`, and the test passes
//! without it. `LAPRDUS_GOLDEN` names another folder.

use laprdus::language::Language;
use laprdus::languages::Frontend;
use laprdus::prosody::Utterance;
use laprdus::text::{Frontend as _, Punctuation};
use std::fmt::Write as _;
use std::path::PathBuf;

/// The languages whose front end is ported.
const LANGUAGES: [&str; 4] = ["hr", "sr", "bs", "sl"];

/// Texts a language reads otherwise than the C++ engine did, on purpose,
/// since the port (language, text). Slovenian, a listener's report of
/// 2026-11-17: petega has the open e of the ordinal pêti (the C++ lexicon
/// had the participle's pétega), and strani is read by its case, straní
/// where no preposition or adjective says the locative (the C++ engine read
/// stráni everywhere; the two texts are Croatian phrases of the corpus).
/// The same listener's report of 2026-11-18: lepo before a pronoun or a verb
/// is the adverb lepó (the C++ engine read lépo before any word).
const INTENDED: &[(&str, &str)] = &[
    ("sl", "petega"),
    ("sl", "dvanajstega do petega"),
    ("sl", "Na lijevoj strani"),
    ("sl", "Strani jezik"),
    ("sl", "Lepo je"),
];

/// Fields 4 and 5 of a line of the reference: the clause, then every phone.
fn describe(utt: &Utterance) -> (String, String) {
    let clause = format!(
        "{} {} {} {} {}",
        utt.kind as u8,
        utt.syllable_count,
        utt.focus_word,
        u8::from(utt.isolated_sound),
        u8::from(utt.slovenian)
    );
    let mut phones = String::new();
    for p in &utt.phones {
        let flags = [
            p.nucleus,
            p.nucleus_tail,
            p.stressed,
            p.is_long,
            p.short_glide,
            p.word_start,
            p.vowel_word,
            p.break_before,
            p.item_break,
        ]
        .iter()
        .enumerate()
        .fold(0u32, |flags, (bit, &set)| flags | u32::from(set) << bit);
        let _ = write!(
            phones,
            "{}:{}:{}:{}:{}:{}:{} ",
            p.ph as u8, flags, p.accent as u8, p.prominence, p.word, p.syllable, p.word_syllables
        );
    }
    (clause, phones)
}

#[test]
fn phones_match_the_cpp_front_end() {
    let golden = std::env::var_os("LAPRDUS_GOLDEN")
        .map_or_else(|| PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("build/golden"), PathBuf::from);
    let Ok(reference) = std::fs::read_to_string(golden.join("frontend_full.tsv")) else {
        eprintln!("no frontend_full.tsv in {}: nothing compared", golden.display());
        return;
    };
    let corpus = std::fs::read_to_string(PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("tests/golden/corpus.jsonl"))
        .expect("tests/golden/corpus.jsonl");
    let texts: Vec<Vec<char>> = corpus
        .lines()
        .map(|line| serde_json::from_str::<String>(line).expect("a JSON string per line").chars().collect())
        .collect();

    let mut mismatches = Vec::new();
    let mut failed = 0;
    for code in LANGUAGES {
        let frontend = Frontend::new(Language::parse(code).expect("a language code"));
        let (mut matched, mut total) = (0, 0);
        for line in reference.lines() {
            let mut fields = line.split('\t');
            if fields.next() != Some(code) {
                continue;
            }
            let (Some(sounds), Some(index), Some(clause), Some(phones)) =
                (fields.next(), fields.next(), fields.next(), fields.next())
            else {
                panic!("a line of the reference with fewer than five fields: {line}");
            };
            let text = index.parse::<usize>().ok().and_then(|index| texts.get(index.checked_sub(1)?));
            let text = text.unwrap_or_else(|| panic!("no text {index} in the corpus"));
            let written: String = text.iter().collect();
            if INTENDED.contains(&(code, written.as_str())) {
                continue;
            }
            let got = describe(&frontend.process(text, Punctuation::Period, sounds == "1"));
            total += 1;
            if got == (clause.to_string(), phones.to_string()) {
                matched += 1;
            } else if mismatches.len() < 20 {
                mismatches.push(format!(
                    "{code} sounds={sounds} #{index} {:?}\n  expected {clause}\t{phones}\n  got      {}\t{}",
                    text.iter().collect::<String>(),
                    got.0,
                    got.1
                ));
            }
        }
        eprintln!("{code}: {matched} of {total} match");
        failed += total - matched;
    }
    assert!(failed == 0, "{failed} clauses differ from the C++ front end; the first:\n{}", mismatches.join("\n"));
}
