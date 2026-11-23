//! What a language's front end hands to the synthesizers: the phones of one
//! clause with their stress and words, and the melody drawn over them.

pub mod intonation;

use crate::formant::phones::Phone;

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum ClauseKind {
    /// Falling.
    #[default]
    Statement,
    /// Comma, semicolon, colon: suspended rise.
    Continuation,
    /// Rise on the focused word.
    YesNoQuestion,
    /// High start on the question word, then falling.
    WhQuestion,
    /// Statement with a wider pitch range.
    Exclamation,
}

/// One clause as phones.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Utterance {
    pub phones: Vec<Phone>,
    pub kind: ClauseKind,
    pub syllable_count: i32,
    /// Word carrying the question peak (-1: the last accented one).
    pub focus_word: i32,
    /// One letter's sound alone (spelling by sounds).
    pub isolated_sound: bool,
    /// False when the clause goes on a sentence (after a comma).
    pub sentence_initial: bool,
    /// Made by the Slovenian front end.
    pub slovenian: bool,
    /// Ends at a short pause inside the clause ([`split_at_breaks`]).
    pub minor_break: bool,
    /// ... the pause between two items: [`item_break_pause_ms`].
    pub item_break: bool,
    /// Goes on after such a pause.
    pub continued: bool,
}

impl Default for Utterance {
    fn default() -> Self {
        Self {
            phones: Vec::new(),
            kind: ClauseKind::Statement,
            syllable_count: 0,
            focus_word: -1,
            isolated_sound: false,
            sentence_initial: true,
            slovenian: false,
            minor_break: false,
            item_break: false,
            continued: false,
        }
    }
}

// A part that ends at a short pause: the share of a comma's boundary movement
// (a smaller fall after the last accent, a rise of about one semitone instead
// of two and a half) and the lengthening of its last syllable (1.30 before a
// comma, 1.40 at the end of a sentence).
pub const MINOR_BREAK_MOVEMENT: f32 = 0.45;
pub const MINOR_BREAK_STRETCH: f32 = 1.15;

/// The silence of a short pause as a share of the user's comma pause. It was
/// the whole comma pause at first and 65 % of it next; the listener found the
/// cases better and the pauses still too long both times (November 2026).
pub const MINOR_BREAK_PAUSE_PERCENT: u32 = 50;

/// The silence of a short pause inside a clause: half of the user's comma
/// pause at the normal rate (50 ms at the default 100), and shorter by the
/// square root of the rate when speech is faster, as the formant transitions
/// are: 35 ms at 2x, 29 ms at 3x. A clause pause is a length the user sets and
/// keeps at every rate, but this one stands inside a phrase, and at a screen
/// reader's rate a fixed one was as long as a syllable.
pub fn minor_break_pause_ms(comma_pause_ms: u32, speed: f32) -> u32 {
    let ms = comma_pause_ms as f32 * MINOR_BREAK_PAUSE_PERCENT as f32 / 100.0;
    (if speed > 1.0 { ms / speed.sqrt() } else { ms }).round() as u32
}

/// The silence between two items said in one breath (a cell's contents and
/// its address): the user's comma pause as it is, at every rate, like the
/// pause of a comma. The melody still goes on over it.
pub fn item_break_pause_ms(comma_pause_ms: u32) -> u32 {
    comma_pause_ms
}

/// The parts of a clause between its short pauses (`Phone::break_before`, from
/// the marks the number converter leaves: the colon of a clock time, the
/// periods of a numeric date, the longer pause before a spreadsheet cell's
/// address).
///
/// The front end reads the clause whole, so every word keeps its context; the
/// synthesizers render the parts one after another with a silence between
/// them. A part before a pause is a continuation that only suspends the
/// melody (`minor_break`), a part after one does not start the sentence again
/// (`continued`), and the last part keeps the clause's own kind. An utterance
/// without a pause comes back as its only part, an empty one as no part.
pub fn split_at_breaks(utt: &Utterance) -> Vec<Utterance> {
    let n = utt.phones.len();
    let mut parts = Vec::new();
    let mut begin = 0;
    for i in 1..=n {
        if i < n && !utt.phones[i].break_before {
            continue;
        }
        if begin == 0 && i == n {
            parts.push(utt.clone());
            break;
        }
        // Words and syllables are counted from the start of the part.
        let first_word = utt.phones[begin].word;
        let last_word = i32::from(utt.phones[i - 1].word);
        let syllables = || utt.phones[begin..i].iter().map(|phone| phone.syllable).filter(|&s| s >= 0);
        let first_syllable = syllables().min();
        let mut phones = utt.phones[begin..i].to_vec();
        for phone in &mut phones {
            phone.word -= first_word;
            if phone.syllable >= 0 {
                phone.syllable -= first_syllable.unwrap_or(0);
            }
            phone.break_before = false;
            phone.item_break = false;
        }
        parts.push(Utterance {
            phones,
            kind: if i == n { utt.kind } else { ClauseKind::Continuation },
            syllable_count: first_syllable.map_or(0, |first| i32::from(syllables().max().unwrap_or(first) - first) + 1),
            focus_word: if (i32::from(first_word)..=last_word).contains(&utt.focus_word) {
                utt.focus_word - i32::from(first_word)
            } else {
                -1
            },
            isolated_sound: false,
            sentence_initial: utt.sentence_initial && begin == 0,
            slovenian: utt.slovenian,
            minor_break: i < n,
            item_break: i < n && utt.phones[i].item_break,
            continued: begin > 0,
        });
        begin = i;
    }
    parts
}
