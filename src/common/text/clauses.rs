//! Clause segmentation: text is split at punctuation and at line breaks, and
//! every clause is spoken with the melody and the pause of what ended it.

use super::Punctuation;
use crate::params::PauseSettings;

/// One clause of a text.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Clause {
    /// The clause without the punctuation that ended it.
    pub text: Vec<char>,
    pub punct: Punctuation,
    /// Ended by a period, question mark or exclamation mark.
    pub end_of_sentence: bool,
}

impl Punctuation {
    /// The punctuation mark a character is, `None` for anything else.
    pub fn of(c: char) -> Self {
        match c {
            ',' => Self::Comma,
            '.' => Self::Period,
            '?' => Self::Question,
            '!' => Self::Exclamation,
            ';' => Self::Semicolon,
            ':' => Self::Colon,
            '…' => Self::Ellipsis,
            _ => Self::None,
        }
    }
}

impl PauseSettings {
    /// The pause after a clause ended by `punct` (ms).
    pub fn after(&self, punct: Punctuation) -> u32 {
        match punct {
            Punctuation::Comma | Punctuation::Semicolon | Punctuation::Colon => self.comma_pause_ms,
            Punctuation::Period | Punctuation::Question | Punctuation::Exclamation | Punctuation::Ellipsis => {
                self.sentence_pause_ms
            }
            Punctuation::Newline => self.newline_pause_ms,
            Punctuation::None => 0,
        }
    }
}

/// Whitespace in the sense of "what follows a clause-ending mark".
fn is_space(c: char) -> bool {
    matches!(
        c,
        ' ' | '\t' | '\n' | '\r' | '\x0C' | '\x0B' | '\u{85}' | '\u{A0}' | '\u{2000}'
            ..='\u{200B}' | '\u{2028}' | '\u{2029}' | '\u{3000}'
    )
}

/// Brackets and quotation marks: a mark followed by one of these still ends
/// the clause (`(Dobro.)`, `"Idemo!"`).
fn is_bracket_or_quote(c: char) -> bool {
    matches!(
        c,
        '(' | ')'
            | '['
            | ']'
            | '{'
            | '}'
            | '"'
            | '\''
            | '<'
            | '>'
            | '«'
            | '»'
            | '‘'
            | '’'
            | '‚'
            | '“'
            | '”'
            | '„'
            | '‹'
            | '›'
    )
}

/// A line break, in any of its encodings.
fn is_line_break(c: char) -> bool {
    matches!(c, '\n' | '\r' | '\x0B' | '\x0C' | '\u{85}' | '\u{2028}' | '\u{2029}')
}

fn is_alnum(c: char) -> bool {
    c.is_ascii_alphanumeric() || c >= '\u{80}'
}

fn is_upper(c: char) -> bool {
    c.is_ascii_uppercase() || matches!(c, 'Č' | 'Ć' | 'Đ' | 'Š' | 'Ž' | '\u{400}'..='\u{42F}')
}

/// A single period glued to the next character stays in the clause, to be
/// read by name, only after a single letter (the dot of an abbreviation,
/// "s.a.r.s.", "U.S.A.", which the front end silences and spells) and before
/// a file extension or domain label: one to five letters or digits in one
/// case, with a letter among them, and nothing of a word after them
/// ("datoteka.txt", "pjesma.mp3", "www.index.hr", "README.TXT"). Any other
/// glued period ends the clause like one before a space; a screen reader
/// glues the items of a label that way ("Preslušano.Nestajuća poruka",
/// "1.Prvi"). A decimal point never gets here: the number converter writes it
/// as another character.
fn period_stays_in_word(text: &[char], i: usize) -> bool {
    if i + 1 >= text.len() {
        return false;
    }
    if i > 0 && is_alnum(text[i - 1]) && (i == 1 || !is_alnum(text[i - 2])) {
        return true;
    }
    let label: Vec<char> = text[i + 1..].iter().copied().take_while(|&c| is_alnum(c)).collect();
    let letters = || label.iter().filter(|c| !c.is_ascii_digit());
    let (upper, lower) = (letters().any(|&c| is_upper(c)), letters().any(|&c| !is_upper(c)));
    (upper || lower) && (1..=5).contains(&label.len()) && !(lower && upper)
}

/// Splits text into clauses.
///
/// A line break ends the clause, with the newline pause: the lines of a post,
/// a list or a label are phrases of their own ("Javno" / "S ponosom
/// predstavljam"), and the next line starts a sentence of its own. A run of
/// breaks and spaces (a blank line, "\r\n") is one boundary; after a mark
/// that ended the clause it adds nothing.
///
/// A run of marks ("...", "?!", ".)") is one boundary. A mark ends the clause
/// only when whitespace, a bracket or quote, or the end of the text follows
/// it. A mark glued to the next character is part of a word ("12:30",
/// "Hej!ti") and stays in the clause text, where the front end reads it by
/// name; a glued period does so only in a file name or after an abbreviated
/// letter. An ellipsis character always ends the clause.
pub fn split_clauses(text: &[char]) -> Vec<Clause> {
    let mut clauses = Vec::new();
    let mut start = 0;
    let mut i = 0;
    while i < text.len() {
        if is_line_break(text[i]) {
            let run_end = i + 1 + text[i + 1..].iter().take_while(|&&c| is_space(c)).count();
            if !text[start..i].iter().all(|&c| is_space(c)) {
                clauses.push(Clause {
                    text: text[start..i].to_vec(),
                    punct: Punctuation::Newline,
                    end_of_sentence: false,
                });
            }
            (start, i) = (run_end, run_end);
            continue;
        }
        if Punctuation::of(text[i]) == Punctuation::None {
            i += 1;
            continue;
        }

        let run_end = i + text[i..].iter().take_while(|&&c| Punctuation::of(c) != Punctuation::None).count();
        let run = &text[i..run_end];
        let has = |punct| run.iter().any(|&c| Punctuation::of(c) == punct);
        let ellipsis = has(Punctuation::Ellipsis);
        let lone_period = run == ['.'];
        let ends_clause = ellipsis
            || text.get(run_end).is_none_or(|&c| is_space(c) || is_bracket_or_quote(c))
            || (lone_period && !period_stays_in_word(text, i));
        if ends_clause {
            let punct = if ellipsis || run.iter().filter(|&&c| c == '.').count() >= 3 {
                Punctuation::Ellipsis
            } else if has(Punctuation::Question) {
                Punctuation::Question
            } else if has(Punctuation::Exclamation) {
                Punctuation::Exclamation
            } else {
                Punctuation::of(text[i])
            };
            if i > start {
                clauses.push(Clause {
                    text: text[start..i].to_vec(),
                    punct,
                    end_of_sentence: matches!(
                        punct,
                        Punctuation::Period | Punctuation::Question | Punctuation::Exclamation
                    ),
                });
            }
            start = run_end;
        }
        i = run_end;
    }

    // What is left has no trailing punctuation.
    if start < text.len() {
        clauses.push(Clause { text: text[start..].to_vec(), punct: Punctuation::None, end_of_sentence: false });
    }
    clauses
}

#[cfg(test)]
mod tests {
    use super::*;

    fn split(text: &str) -> Vec<(String, Punctuation)> {
        let text: Vec<char> = text.chars().collect();
        split_clauses(&text).into_iter().map(|clause| (clause.text.into_iter().collect(), clause.punct)).collect()
    }

    #[test]
    fn marks_end_clauses_only_before_a_space() {
        use Punctuation::*;
        assert_eq!(split("Dobar dan, kako ste?"), [("Dobar dan".into(), Comma), (" kako ste".into(), Question)]);
        assert_eq!(split("u 12:30 sati"), [("u 12:30 sati".into(), None)]);
        assert_eq!(split("datoteka.txt i www.index.hr."), [("datoteka.txt i www.index.hr".into(), Period)]);
        assert_eq!(split("Preslušano.Nestajuća"), [("Preslušano".into(), Period), ("Nestajuća".into(), None)]);
        assert_eq!(split("s.a.r.s. je"), [("s.a.r.s".into(), Period), (" je".into(), None)]);
        assert_eq!(split("Što?! Da..."), [("Što".into(), Question), (" Da".into(), Ellipsis)]);
        assert_eq!(split("Javno\r\n\nS ponosom"), [("Javno".into(), Newline), ("S ponosom".into(), None)]);
        assert_eq!(split("(Dobro.) Da"), [("(Dobro".into(), Period), (") Da".into(), None)]);
    }
}
