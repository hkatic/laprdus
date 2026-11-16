//! Text on its way to the synthesizers: clauses, the marks the number
//! converter leaves, and what a language's front end must provide.

use crate::prosody::Utterance;

/// The punctuation that ended a clause; it selects the intonation.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum Punctuation {
    #[default]
    None,
    /// Slight rise, short pause.
    Comma,
    /// Falling intonation, longer pause.
    Period,
    /// Rising intonation at the end.
    Question,
    /// Emphatic, higher pitch.
    Exclamation,
    /// Stronger than a comma.
    Semicolon,
    /// Neutral pause.
    Colon,
    /// Trailing off.
    Ellipsis,
    /// Line break pause.
    Newline,
}

/// A language's text front end: one clause of text to phones.
pub trait Frontend {
    /// One clause. With `letter_sounds`, a consonant letter standing alone as
    /// a word ("veliko slovo B", "Obrisano B") is spoken by its sound, "bə",
    /// as when it is spelled; otherwise by its name, "be". A preposition (s,
    /// k) stays one, and the letters of an abbreviation (USB, s.a.r.s.) are
    /// always named.
    fn process(&self, text: &[char], punct: Punctuation, letter_sounds: bool) -> Utterance;

    /// The sound of one letter alone, for spelling by sounds: a vowel as a
    /// stressed syllable, a consonant followed by a short neutral vowel
    /// ("bə", "sə"). An unknown letter and the foreign letters, which are
    /// spelled by their names, give no phones.
    fn letter_sound(&self, letter: &[char]) -> Utterance;
}

/// Longest stretch synthesized in one go: a clause without any punctuation
/// can be arbitrarily long (a screen reader may send a whole unpunctuated
/// paragraph), so it is spoken in breath groups.
const MAX_GROUP_WORDS: usize = 48;
const MAX_GROUP_CHARS: usize = 600;

/// The breath groups of a clause, each with the punctuation it is spoken
/// with (continuation intonation for all but the last, which has the
/// clause's own) and whether it opens the clause. Never cuts inside a word.
pub fn breath_groups(text: &[char], punct: Punctuation) -> impl Iterator<Item = (&[char], Punctuation, bool)> {
    let is_space = |c: char| matches!(c, ' ' | '\n' | '\t');
    let mut start = 0;
    std::iter::from_fn(move || {
        if start >= text.len() {
            return None;
        }
        let (mut end, mut words, mut in_word) = (start, 0, false);
        while end < text.len() && end - start < MAX_GROUP_CHARS {
            let space = is_space(text[end]);
            if !space && !in_word {
                if words == MAX_GROUP_WORDS {
                    break;
                }
                words += 1;
            }
            in_word = !space;
            end += 1;
        }
        // Back up to the last space of the group.
        if end < text.len()
            && in_word
            && text[end] != ' '
            && let Some(space) = text[..=end].iter().rposition(|&c| c == ' ').filter(|&space| space > start)
        {
            end = space;
        }
        let group = (&text[start..end], if end >= text.len() { punct } else { Punctuation::Comma }, start == 0);
        start = end;
        Some(group)
    })
}
