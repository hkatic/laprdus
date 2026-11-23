//! How the Slovenian front end reads a sentence: the helper of
//! tools/formant/slovenian_suk.py, and a quick look at a listener's report.
//!
//! Reads one sentence per line from standard input and prints, per line, the
//! sentence as the engine prepares it (numbers as words, clauses split at the
//! punctuation and marked for one another) and its phones in the notation of
//! the tests of `src/common/languages/sl/tests.rs`: words apart, `'` before a
//! stressed nucleus, `:` after a long one, `E` `O` the open e and o, `é` `ó`
//! the close ones, `ə` the schwa, `w` the l or v that is [w]; clauses are
//! separated by ` | `:
//!
//! ```text
//!     sentence <TAB> phones
//! ```
//!
//! ```text
//!     cargo run --release --example slovenian_stress < sentences.txt
//! ```

use laprdus::formant::phones::Ph;
use laprdus::language::Language;
use laprdus::languages::Frontend;
use laprdus::text::clauses::split_clauses;
use laprdus::text::numbers::NumberReader;
use laprdus::text::{Frontend as _, Punctuation};
use std::io::{BufRead, Write};

const NAMES: [&str; Ph::COUNT] = [
    "_", "a", "e", "i", "o", "u", "@", "E", "O", "é", "ó", "ə", "w", "j", "v", "l", "lj", "m", "n", "nj", "ŋ", "r",
    "p", "b", "t", "d", "k", "g", "c", "dz", "č", "dž", "ć", "đ", "f", "s", "z", "š", "ž", "ś", "ź", "h", "ɣ",
];

fn main() -> std::io::Result<()> {
    let frontend = Frontend::new(Language::Slovenian);
    let numbers = NumberReader::new(Language::Slovenian);
    let mut out = std::io::BufWriter::new(std::io::stdout().lock());
    for line in std::io::stdin().lock().lines() {
        let line = line?;
        let line = line.trim_end_matches('\r');
        let text: Vec<char> = numbers.convert_numbers(line).chars().collect();
        let mut clauses = split_clauses(&text);
        frontend.mark_clauses(&mut clauses);
        let mut row = String::new();
        for (k, clause) in clauses.iter().enumerate() {
            if k > 0 {
                row.push_str(" | ");
            }
            let punct: Punctuation = clause.punct;
            let utterance = frontend.process(&clause.text, punct, false);
            let mut word = None;
            for phone in &utterance.phones {
                if word != Some(phone.word) {
                    if word.is_some() {
                        row.push(' ');
                    }
                    word = Some(phone.word);
                }
                if phone.nucleus && phone.stressed {
                    row.push('\'');
                }
                row.push_str(NAMES[phone.ph as usize]);
                if phone.nucleus && phone.is_long {
                    row.push(':');
                }
            }
        }
        writeln!(out, "{line}\t{row}")?;
    }
    Ok(())
}
