//! Pronunciation dictionary: words replaced by a spelling the voices read
//! right ("Facebook" → "Fejzbuk").
//!
//! A whole-word entry is a regular expression between word boundaries, so
//! one entry can cover the case forms of a word: `Messenger(a|u|e|om)?` →
//! `Mesindžer$1`. As in the dictionaries written for the C++ engine, the
//! boundaries and the case-insensitive match know ASCII letters only: an
//! entry that begins or ends with č, ć, đ, š or ž is matched as a whole word
//! only next to an ASCII letter or digit.

use super::{entries, read};
use regex::bytes::{Regex, RegexBuilder};
use serde_json::Value;
use std::path::Path;

/// One replacement.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct DictionaryEntry {
    /// Text to match.
    pub grapheme: String,
    /// What is spoken instead.
    pub phoneme: String,
    pub case_sensitive: bool,
    /// Match whole words only.
    pub whole_word: bool,
}

#[derive(Clone, Debug)]
struct Rule {
    entry: DictionaryEntry,
    /// The compiled whole-word pattern and its replacement; `None` for a
    /// substring entry or a pattern that does not compile (which is skipped).
    pattern: Option<(Regex, Vec<u8>)>,
}

/// The replacement of an entry in the syntax of the regex crate: `$1`, `$&`
/// and `$$` mean what they do in an ECMAScript replacement, any other `$` is
/// a dollar sign.
fn replacement(phoneme: &str) -> Vec<u8> {
    let mut out = String::with_capacity(phoneme.len() + 4);
    let mut chars = phoneme.chars().peekable();
    while let Some(c) = chars.next() {
        if c != '$' {
            out.push(c);
            continue;
        }
        match chars.peek() {
            Some('&') => {
                chars.next();
                out.push_str("${0}");
            }
            Some(digit) if digit.is_ascii_digit() => {
                let mut group = String::new();
                while group.len() < 2
                    && let Some(digit) = chars.next_if(char::is_ascii_digit)
                {
                    group.push(digit);
                }
                out.push_str(&format!("${{{}}}", group.parse::<u32>().unwrap_or(0)));
            }
            Some('$') => {
                chars.next();
                out.push_str("$$");
            }
            _ => out.push_str("$$"),
        }
    }
    out.into_bytes()
}

impl Rule {
    fn new(entry: DictionaryEntry) -> Self {
        let pattern = entry.whole_word.then(|| {
            RegexBuilder::new(&format!(r"\b{}\b", entry.grapheme))
                .unicode(false)
                .case_insensitive(!entry.case_sensitive)
                .build()
                .ok()
                .map(|regex| (regex, replacement(&entry.phoneme)))
        });
        Self { entry, pattern: pattern.flatten() }
    }

    fn apply(&self, text: Vec<u8>) -> Vec<u8> {
        let entry = &self.entry;
        if entry.whole_word {
            return match &self.pattern {
                Some((regex, replacement)) => regex.replace_all(&text, replacement.as_slice()).into_owned(),
                None => text,
            };
        }
        // Substring matching; without case sensitivity ASCII letters fold.
        let fold = |bytes: &[u8]| if entry.case_sensitive { bytes.to_vec() } else { bytes.to_ascii_lowercase() };
        let (needle, phoneme) = (fold(entry.grapheme.as_bytes()), entry.phoneme.as_bytes());
        let mut text = text;
        let mut from = 0;
        while let Some(found) = text.get(from..).and_then(|rest| find(&fold(rest), &needle)) {
            let at = from + found;
            text.splice(at..at + needle.len(), phoneme.iter().copied());
            from = at + phoneme.len();
        }
        text
    }
}

fn find(haystack: &[u8], needle: &[u8]) -> Option<usize> {
    haystack.windows(needle.len()).position(|window| window == needle)
}

/// The replacements, applied in the order they were loaded.
#[derive(Clone, Debug, Default)]
pub struct PronunciationDictionary {
    rules: Vec<Rule>,
}

impl PronunciationDictionary {
    /// Replaces the entries by those of a JSON file's content. False when
    /// the dictionary is empty afterwards.
    pub fn load(&mut self, json: &str) -> bool {
        self.rules.clear();
        self.append(json)
    }

    /// Adds the entries of a JSON file's content.
    pub fn append(&mut self, json: &str) -> bool {
        for entry in entries(json) {
            let field = |key| entry.get(key).and_then(Value::as_str).unwrap_or_default().to_owned();
            let flag = |key, default| entry.get(key).and_then(Value::as_bool).unwrap_or(default);
            self.add(DictionaryEntry {
                grapheme: field("grapheme"),
                phoneme: field("phoneme"),
                case_sensitive: flag("caseSensitive", false),
                whole_word: flag("wholeWord", true),
            });
        }
        !self.rules.is_empty()
    }

    pub fn load_file(&mut self, path: &Path) -> bool {
        read(path).is_some_and(|json| self.load(&json))
    }

    pub fn append_file(&mut self, path: &Path) -> bool {
        read(path).is_some_and(|json| self.append(&json))
    }

    /// Adds one entry; one without a grapheme or a phoneme is ignored.
    pub fn add(&mut self, entry: DictionaryEntry) {
        if !entry.grapheme.is_empty() && !entry.phoneme.is_empty() {
            self.rules.push(Rule::new(entry));
        }
    }

    /// The text with every entry's replacement made.
    pub fn apply(&self, text: &str) -> String {
        if self.rules.is_empty() || text.is_empty() {
            return text.to_owned();
        }
        let replaced = self.rules.iter().fold(text.as_bytes().to_vec(), |text, rule| rule.apply(text));
        String::from_utf8(replaced).unwrap_or_else(|error| String::from_utf8_lossy(error.as_bytes()).into_owned())
    }

    pub fn clear(&mut self) {
        self.rules.clear();
    }

    pub fn len(&self) -> usize {
        self.rules.len()
    }

    pub fn is_empty(&self) -> bool {
        self.rules.is_empty()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn dictionary(entries: &str) -> PronunciationDictionary {
        let mut dictionary = PronunciationDictionary::default();
        assert!(dictionary.load(&format!(r#"{{"version": "1.0", "entries": [{entries}]}}"#)));
        dictionary
    }

    #[test]
    fn whole_words_and_case_forms() {
        let d = dictionary(
            r#"{"grapheme": "Messenger(a|u|e|om)?", "phoneme": "Mesindžer$1"},
               {"grapheme": "OK", "phoneme": "Okej", "caseSensitive": true},
               {"grapheme": "broken(", "phoneme": "x"}, {"grapheme": "", "phoneme": "x"}"#,
        );
        assert_eq!(d.len(), 3);
        assert_eq!(
            d.apply("na messengeru i Messengerom, OK? ok. TOKEN"),
            "na Mesindžeru i Mesindžerom, Okej? ok. TOKEN"
        );
    }

    #[test]
    fn substrings() {
        let d = dictionary(r#"{"grapheme": "Ph", "phoneme": "f", "wholeWord": false}"#);
        assert_eq!(d.apply("Photo graph PHONE"), "foto graf fONE");
    }

    #[test]
    fn replacement_syntax() {
        assert_eq!(replacement("a$1b $& $$ $x"), b"a${1}b ${0} $$ $$x");
    }
}
