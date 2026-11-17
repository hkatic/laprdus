//! The number converter against the C++ engine, text for text.
//!
//! A dump program built from the C++ engine (removed with it) wrote, for every text of the golden corpus
//! (`tests/golden/corpus.jsonl`) and every language, what the C++ converter
//! made of it in both reading modes:
//!
//! ```text
//! <lang>\t<mode>\t<index>\t<output>
//! ```
//!
//! `mode` is `w` for whole numbers and `d` for digits, `index` the 1-based
//! line of the corpus, and the output has newline, tab, carriage return and
//! backslash escaped. The dump is a local file (`build/golden`, or the folder
//! named by `LAPRDUS_GOLDEN`); the test passes without it.

use laprdus::language::Language;
use laprdus::text::numbers::NumberReader;
use std::path::{Path, PathBuf};

fn golden() -> PathBuf {
    std::env::var_os("LAPRDUS_GOLDEN")
        .map_or_else(|| Path::new(env!("CARGO_MANIFEST_DIR")).join("build/golden"), PathBuf::from)
}

fn escape(text: &str) -> String {
    text.replace('\\', "\\\\").replace('\n', "\\n").replace('\t', "\\t").replace('\r', "\\r")
}

#[test]
fn numbers_match_the_cpp_engine() {
    let Ok(expected) = std::fs::read(golden().join("numbers_full.tsv")) else { return };
    let expected = String::from_utf8_lossy(&expected);
    let corpus = std::fs::read_to_string(Path::new(env!("CARGO_MANIFEST_DIR")).join("tests/golden/corpus.jsonl"))
        .expect("the golden corpus");
    let corpus: Vec<String> =
        corpus.lines().map(|line| serde_json::from_str(line).expect("a JSON string per line")).collect();

    let (mut total, mut matching) = (0, 0);
    let mut mismatches = Vec::new();
    for line in expected.split('\n').map(|line| line.strip_suffix('\r').unwrap_or(line)).filter(|line| !line.is_empty())
    {
        let mut fields = line.splitn(4, '\t');
        let (Some(lang), Some(mode), Some(index), Some(output)) =
            (fields.next(), fields.next(), fields.next(), fields.next())
        else {
            panic!("malformed line: {line}");
        };
        let language = Language::parse(lang).unwrap_or_else(|| panic!("unknown language: {lang}"));
        let input = &corpus[index.parse::<usize>().expect("a line number") - 1];
        let reader = NumberReader::new(language);
        let got = escape(&match mode {
            "w" => reader.convert_numbers(input),
            "d" => reader.convert_digits(input),
            _ => panic!("unknown mode: {mode}"),
        });
        total += 1;
        if got == output {
            matching += 1;
        } else if mismatches.len() < 20 {
            mismatches.push(format!("{lang} {mode} {input:?}\n  expected {output}\n  got      {got}"));
        }
    }
    eprintln!("{matching} of {total} match");
    assert!(mismatches.is_empty(), "{} of {total} differ; the first:\n{}", total - matching, mismatches.join("\n"));
}
