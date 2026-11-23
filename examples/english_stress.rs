//! Helper of tools/formant/english_words.py.
//!
//! Reads one lowercase word per line from standard input and prints, per
//! word, what the front ends of Croatian, Serbian and Bosnian do with it,
//! built with whatever `src/common/languages/{hr,sr,bs}/tables/english.txt`
//! hold (english_words.py builds it first with empty tables):
//!
//! ```text
//!     word  shape  letters  hr  sr  bs
//! ```
//!
//! shape is 1 for a word spelled the English way (`english_spelling()`),
//! letters the letter of the spelling each syllable nucleus comes from
//! (comma-separated), and hr, sr, bs the stressed nucleus and its accent
//! (N, F, R) in each language, "-" for none.
//!
//! Run by english_words.py:
//!
//! ```text
//!     cargo run --release --example english_stress < words.txt
//! ```

use laprdus::formant::phones::Accent;
use laprdus::language::Language;
use laprdus::languages::Frontend;
use laprdus::languages::hr::english::english_spelling;
use laprdus::languages::hr::phonemize::phonemize;
use laprdus::text::frontend::Word;
use laprdus::text::{Frontend as _, Punctuation};
use std::fmt::Write as _;
use std::io::{BufRead, Write};

fn main() -> std::io::Result<()> {
    let frontends = [Language::Croatian, Language::Serbian, Language::Bosnian].map(Frontend::new);
    let mut out = std::io::BufWriter::new(std::io::stdout().lock());
    for line in std::io::stdin().lock().lines() {
        let line = line?;
        let line = line.trim_end_matches('\r');
        if line.is_empty() {
            continue;
        }
        let text: Vec<char> = line.chars().collect();
        // The letters and the syllables are the same in the three languages
        let mut word = Word::new(&text);
        phonemize(&mut word);
        let mut row = format!("{line}\t{}\t", u8::from(english_spelling(&word.spelled)));
        for (k, letter) in word.nucleus_letters().into_iter().enumerate() {
            let spelled = word.origin.get(letter).copied().unwrap_or(letter);
            let _ = write!(row, "{}{spelled}", if k > 0 { "," } else { "" });
        }
        for frontend in &frontends {
            let utt = frontend.process(&text, Punctuation::Period, false);
            let stressed = utt.phones.iter().find(|phone| phone.nucleus && phone.stressed && phone.word == 0);
            let one_word = utt.phones.last().is_some_and(|phone| phone.word == 0);
            let accent = match stressed.map(|phone| phone.accent) {
                None => '-',
                Some(Accent::Falling) => 'F',
                Some(Accent::Rising) => 'R',
                Some(_) => 'N',
            };
            match stressed {
                Some(phone) if one_word && phone.syllable >= 0 => {
                    let _ = write!(row, "\t{}{accent}", phone.syllable);
                }
                _ => {
                    let _ = write!(row, "\t-{accent}");
                }
            }
        }
        writeln!(out, "{row}")?;
    }
    Ok(())
}
