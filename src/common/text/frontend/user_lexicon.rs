//! Accent entries supplied by the user.

use super::letters::chars;
use super::notation::{LexEntry, for_each_form, parse_lex_entry};
use crate::language::Language;
use crate::languages::{self, sl};
use std::sync::Arc;

/// The user's accent lexicon.
///
/// The entries use the notation of the built-in lexicon and come from a JSON
/// file next to the user dictionaries:
///
/// ```text
///   { "version": "1.0", "entries": [
///       { "word": "kontr'o:l*", "comment": "kontróla, kontróle" },
///       { "word": "sign'a:l|a|u|om|e|i|ima" },
///       { "verb": "ur'e:d=i<p", "comment": "uréditi; see VERBS" },
///       { "verb": "dijel:i", "language": "hr" }
///   ] }
/// ```
///
/// "word" is a word, a stem ("*") or a paradigm ("|"); "verb" a whole stem
/// ("st'e:m=classes") or a root with a long ije ("root:classes[:prefixes]");
/// "language" ("hr", "sr", "bs", "sl") limits an entry to one language. A
/// Slovenian "word" is written in the notation of [`sl::lexicon::Lexicon`]
/// (mésto, člôvek, p'əs, d'é:la*); an entry without a language is used for
/// Slovenian too when it reads in that notation.
///
/// Every entry is checked when the file is parsed, and a malformed one is
/// skipped and counted, so a typo can neither crash the engine nor silently
/// produce a word without stress. The lexicon is parsed once; synthesis
/// only looks entries up in hash maps and scans the user's verbs, so there
/// is no cost per utterance beyond that of the built-in tables.
#[derive(Debug, Default)]
pub struct UserLexicon {
    pub(crate) words: Vec<WordEntry>,
    pub(crate) verbs: Vec<VerbEntry>,
    /// The entries for Slovenian, in its notation.
    pub(crate) slovenian: sl::lexicon::Lexicon,
}

/// What [`UserLexicon::parse`] made of the file.
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Report {
    /// Word forms accepted.
    pub words: usize,
    /// Verb entries accepted.
    pub verbs: usize,
    /// Entries skipped.
    pub rejected: usize,
    /// What was wrong with the first one.
    pub first_error: String,
}

impl Report {
    fn reject(&mut self, text: &str, why: &str) {
        self.rejected += 1;
        if self.first_error.is_empty() {
            self.first_error = format!("{text}: {why}");
        }
    }
}

#[derive(Clone, Debug)]
pub(crate) struct WordEntry {
    pub plain: Vec<char>,
    pub entry: LexEntry,
    pub stem: bool,
    /// Bit per [`Language`], 0 = all.
    pub languages: u8,
}

#[derive(Clone, Debug)]
pub(crate) struct VerbEntry {
    /// The entry as written; each language that can use it reads it by its
    /// own verb rules.
    pub text: Vec<char>,
    pub languages: u8,
}

/// The bit of a language in an entry's `languages`.
pub(crate) fn language_bit(language: Language) -> u8 {
    1 << language as u8
}

impl UserLexicon {
    // Limits that keep a huge file from slowing every word down.
    pub const MAX_WORDS: usize = 20000;
    pub const MAX_VERBS: usize = 2000;

    /// Parses the JSON content. Never fails: bad entries are counted.
    pub fn parse(json: &str) -> (Arc<UserLexicon>, Report) {
        let mut lexicon = UserLexicon::default();
        let mut report = Report::default();

        for object in json_entries(json.as_bytes()) {
            let language = json_string(object, "language");
            let languages = match language.as_str() {
                "" => 0,
                "hr" => language_bit(Language::Croatian),
                "sr" => language_bit(Language::Serbian),
                "bs" => language_bit(Language::Bosnian),
                "sl" => language_bit(Language::Slovenian),
                _ => {
                    report.reject(&language, "unknown language (hr, sr, bs, sl)");
                    continue;
                }
            };
            let word = json_string(object, "word");
            let verb = json_string(object, "verb");
            if !word.is_empty() && !verb.is_empty() {
                report.reject(&word, "both word and verb");
                continue;
            }

            // A Slovenian entry is in the notation of its own lexicon
            if language == "sl" {
                if word.is_empty() {
                    report.reject(&verb, "Slovenian has word entries only");
                    continue;
                }
                let marked = chars(&word);
                if !marked.contains(&'|')
                    && let Err(why) = sl::lexicon::Lexicon::parse(&marked)
                {
                    report.reject(&word, why);
                } else if lexicon.slovenian.add(&marked) {
                    report.words += 1;
                } else {
                    report.reject(&word, "malformed form list");
                }
                continue;
            }

            if !word.is_empty() {
                let marked = chars(&word);
                if language.is_empty() {
                    // Without a language the entry counts for Slovenian too when it
                    // reads in that notation (kontr'o:l*: stress and length)
                    lexicon.slovenian.add(&marked);
                }
                let mut forms: Vec<WordEntry> = Vec::new();
                let mut failed: Option<&'static str> = None;
                for_each_form(&marked, |form| {
                    if failed.is_none() {
                        match parse_lex_entry(form) {
                            Ok(parsed) => forms.push(WordEntry {
                                plain: parsed.plain,
                                entry: parsed.entry,
                                stem: parsed.stem,
                                languages,
                            }),
                            Err(reason) => failed = Some(reason),
                        }
                    }
                });
                if let Some(reason) = failed {
                    report.reject(&word, reason);
                } else if lexicon.words.len() + forms.len() > Self::MAX_WORDS {
                    report.reject(&word, "too many words");
                } else {
                    report.words += forms.len();
                    lexicon.words.extend(forms);
                }
            } else if !verb.is_empty() {
                // A verb entry is good when a language it is for can use it;
                // otherwise the first of them says what is wrong with it
                let text = chars(&verb);
                let mut checks = Language::ALL
                    .into_iter()
                    .filter(|&language| languages == 0 || languages & language_bit(language) != 0)
                    .map(|language| languages::rules(language).check_verb_entry(&text));
                let first = checks.next().unwrap_or(Err("no language for the entry"));
                match first.or_else(|why| if checks.any(|check| check.is_ok()) { Ok(()) } else { Err(why) }) {
                    Err(why) => report.reject(&verb, why),
                    Ok(()) if lexicon.verbs.len() >= Self::MAX_VERBS => report.reject(&verb, "too many verbs"),
                    Ok(()) => {
                        lexicon.verbs.push(VerbEntry { text, languages });
                        report.verbs += 1;
                    }
                }
            } else {
                let shown = if object.len() > 60 {
                    format!("{}...", String::from_utf8_lossy(&object[..60]))
                } else {
                    String::from_utf8_lossy(object).into_owned()
                };
                report.reject(&shown, "neither word nor verb");
            }
        }
        (Arc::new(lexicon), report)
    }

    pub fn is_empty(&self) -> bool {
        self.words.is_empty() && self.verbs.is_empty() && self.slovenian.is_empty()
    }
}

// ---- A small reader for the user lexicon's JSON ----
//
// It takes what a strict parser refuses (a comma after the last entry, a
// stray character between two entries), so one slip in a file written by
// hand loses one entry and not the whole lexicon.

const fn is_json_space(b: u8) -> bool {
    matches!(b, b' ' | b'\t' | b'\n' | b'\r')
}

fn find_bytes(text: &[u8], needle: &[u8], from: usize) -> Option<usize> {
    text.get(from..)?.windows(needle.len()).position(|window| window == needle).map(|at| at + from)
}

/// The string value of "key" in one JSON object, unescaped; empty if absent.
fn json_string(object: &[u8], key: &str) -> String {
    let needle = format!("\"{key}\"");
    let skip_space = |mut at: usize| {
        while object.get(at).copied().is_some_and(is_json_space) {
            at += 1;
        }
        at
    };
    let mut pos = 0;
    while let Some(found) = find_bytes(object, needle.as_bytes(), pos) {
        let after = skip_space(found + needle.len());
        if object.get(after) != Some(&b':') {
            pos = after;
            continue;
        }
        let after = skip_space(after + 1);
        if object.get(after) != Some(&b'"') {
            return String::new();
        }
        let mut value: Vec<u8> = Vec::new();
        let mut i = after + 1;
        while let Some(&c) = object.get(i) {
            match (c, object.get(i + 1)) {
                (b'"', _) => break,
                (b'\\', Some(&escape)) => {
                    i += 1;
                    match escape {
                        b'n' => value.push(b'\n'),
                        b't' => value.push(b'\t'),
                        b'r' => value.push(b'\r'),
                        b'u' => {
                            // \uXXXX: only the Basic Multilingual Plane, which
                            // holds every letter and mark an entry can use.
                            let code = object
                                .get(i + 1..i + 5)
                                .filter(|_| i + 4 < object.len())
                                .and_then(|hex| std::str::from_utf8(hex).ok())
                                .filter(|hex| hex.bytes().all(|h| h.is_ascii_hexdigit()))
                                .and_then(|hex| u32::from_str_radix(hex, 16).ok());
                            let Some(code) = code else { break };
                            i += 4;
                            // Half of a surrogate pair is no character: U+FFFD
                            let c = char::from_u32(code).unwrap_or(char::REPLACEMENT_CHARACTER);
                            value.extend_from_slice(c.encode_utf8(&mut [0; 4]).as_bytes());
                        }
                        other => value.push(other),
                    }
                }
                _ => value.push(c),
            }
            i += 1;
        }
        return String::from_utf8_lossy(&value).into_owned();
    }
    String::new()
}

/// The objects of the "entries" array, each as its own text.
fn json_entries(json: &[u8]) -> Vec<&[u8]> {
    let mut entries = Vec::new();
    let Some(array) = find_bytes(json, b"\"entries\"", 0).and_then(|at| find_bytes(json, b"[", at)) else {
        return entries;
    };
    let mut depth = 0i32;
    let mut in_string = false;
    let mut start = 0;
    let mut i = array;
    while let Some(&c) = json.get(i) {
        if in_string {
            match c {
                b'\\' => i += 1,
                b'"' => in_string = false,
                _ => {}
            }
        } else {
            match c {
                b'"' => in_string = true,
                b'{' => {
                    if depth == 1 {
                        start = i;
                    }
                    depth += 1;
                }
                b'}' => {
                    depth -= 1;
                    if depth == 1 {
                        entries.push(&json[start..=i]);
                    }
                }
                b'[' => depth += 1,
                b']' => {
                    depth -= 1;
                    if depth <= 0 {
                        break;
                    }
                }
                _ => {}
            }
        }
        i += 1;
    }
    entries
}
