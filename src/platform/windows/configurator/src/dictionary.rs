//! The user's dictionaries as files: `user.json`, `spelling.json`,
//! `emoji.json` (replacement entries) and `accents.json` (the accent lexicon).
//!
//! The files are read leniently, key by key, not by a strict JSON parser: a
//! hand-edited file with a stray comma keeps its entries, and saving does not
//! throw them away.

use laprdus::config::UserConfig;
use laprdus::text::frontend::UserLexicon;
use std::path::PathBuf;

/// The four dictionaries, in the order of the dialog's combo box.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Kind {
    Main,
    Spelling,
    Emoji,
    Accents,
}

impl Kind {
    pub const ALL: [Kind; 4] = [Kind::Main, Kind::Spelling, Kind::Emoji, Kind::Accents];

    pub fn path(self) -> Option<PathBuf> {
        UserConfig::new().path(match self {
            Kind::Main => UserConfig::DICTIONARY,
            Kind::Spelling => UserConfig::SPELLING,
            Kind::Emoji => UserConfig::EMOJI,
            Kind::Accents => UserConfig::ACCENTS,
        })
    }
}

/// Language codes of an accent entry, in the order of the entry dialog's
/// combo box; "" is every language.
pub const ACCENT_LANGUAGES: [&str; 5] = ["", "hr", "sr", "bs", "sl"];

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Entry {
    /// The text to match; in the accent lexicon the entry in its notation.
    pub grapheme: String,
    /// The replacement.
    pub phoneme: String,
    pub comment: String,
    pub case_sensitive: bool,
    pub whole_word: bool,
    /// Accent lexicon: a "verb" entry, not a "word".
    pub verb: bool,
    /// Accent lexicon: "hr", "sr", "bs", "sl" or "" (every language).
    pub language: String,
}

impl Default for Entry {
    fn default() -> Self {
        Self {
            grapheme: String::new(),
            phoneme: String::new(),
            comment: String::new(),
            case_sensitive: false,
            whole_word: true,
            verb: false,
            language: String::new(),
        }
    }
}

const SPACE: [char; 4] = [' ', '\t', '\n', '\r'];

/// The text from a key's value on: after a quoted name followed by a colon,
/// so that a value equal to the name ("comment": "word") is skipped.
fn value_of<'a>(json: &'a str, key: &str) -> Option<&'a str> {
    let quoted = format!("\"{key}\"");
    let mut rest = json;
    while let Some(found) = rest.find(&quoted) {
        rest = &rest[found + quoted.len()..];
        if let Some(value) = rest.trim_start_matches(SPACE).strip_prefix(':') {
            return Some(value.trim_start_matches(SPACE));
        }
    }
    None
}

/// A key's string value, or "" when it has none.
fn string_value(json: &str, key: &str) -> String {
    let Some(text) = value_of(json, key).and_then(|value| value.strip_prefix('"')) else { return String::new() };
    let mut out = String::new();
    let mut chars = text.chars();
    loop {
        match chars.next() {
            Some('"') => return out,
            Some('\\') => match chars.next() {
                Some('n') => out.push('\n'),
                Some('t') => out.push('\t'),
                Some('r') => out.push('\r'),
                Some(other) => out.push(other),
                None => return String::new(),
            },
            Some(other) => out.push(other),
            None => return String::new(), // no closing quote
        }
    }
}

fn bool_value(json: &str, key: &str, default: bool) -> bool {
    match value_of(json, key) {
        Some(value) if value.starts_with("true") => true,
        Some(value) if value.starts_with("false") => false,
        _ => default,
    }
}

/// The JSON text of every object of the "entries" array.
fn objects(json: &str) -> Vec<&str> {
    let mut found = Vec::new();
    let Some(mut rest) = value_of(json, "entries").and_then(|value| Some(&value[value.find('[')? + 1..])) else {
        return found;
    };
    while let Some(start) = rest.find('{') {
        let mut depth = 0;
        let end = rest[start..].bytes().position(|byte| {
            depth += i32::from(byte == b'{') - i32::from(byte == b'}');
            depth == 0
        });
        let Some(end) = end else { break };
        found.push(&rest[start..=start + end]);
        rest = &rest[start + end + 1..];
    }
    found
}

/// Reads a dictionary's content. Replacement entries without a text to match
/// are dropped. An accent entry is `{ "word": .. }` or `{ "verb": .. }` with
/// an optional "language" and "comment"; a hand-written one with both keys,
/// which the engine rejects, is listed as a word and a verb, so that saving
/// keeps both halves (as two entries the engine accepts).
pub fn parse(kind: Kind, json: &str) -> Vec<Entry> {
    let mut entries = Vec::new();
    for object in objects(json) {
        let comment = string_value(object, "comment");
        if kind == Kind::Accents {
            let language = string_value(object, "language");
            for (key, verb) in [("word", false), ("verb", true)] {
                let grapheme = string_value(object, key);
                if !grapheme.is_empty() {
                    let (language, comment) = (language.clone(), comment.clone());
                    entries.push(Entry { grapheme, verb, language, comment, ..Entry::default() });
                }
            }
        } else {
            let grapheme = string_value(object, "grapheme");
            if !grapheme.is_empty() {
                entries.push(Entry {
                    grapheme,
                    phoneme: string_value(object, "phoneme"),
                    comment,
                    case_sensitive: bool_value(object, "caseSensitive", false),
                    whole_word: bool_value(object, "wholeWord", true),
                    ..Entry::default()
                });
            }
        }
    }
    entries
}

fn escape(text: &str) -> String {
    let mut out = String::with_capacity(text.len() + 10);
    for c in text.chars() {
        match c {
            '"' => out += "\\\"",
            '\\' => out += "\\\\",
            '\n' => out += "\\n",
            '\r' => out += "\\r",
            '\t' => out += "\\t",
            other => out.push(other),
        }
    }
    out
}

/// One accent lexicon entry as a JSON object (also what the engine's parser checks).
fn accent_json(entry: &Entry, indent: &str) -> String {
    let key = if entry.verb { "verb" } else { "word" };
    let mut json = format!("{indent}{{\n{indent}    \"{key}\": \"{}\"", escape(&entry.grapheme));
    for (key, value) in [("language", &entry.language), ("comment", &entry.comment)] {
        if !value.is_empty() {
            json += &format!(",\n{indent}    \"{key}\": \"{}\"", escape(value));
        }
    }
    json + &format!("\n{indent}}}")
}

fn replacement_json(entry: &Entry) -> String {
    let mut json = format!(
        "        {{\n            \"grapheme\": \"{}\",\n            \"phoneme\": \"{}\",\n            \
         \"caseSensitive\": {},\n            \"wholeWord\": {}",
        escape(&entry.grapheme),
        escape(&entry.phoneme),
        entry.case_sensitive,
        entry.whole_word
    );
    if !entry.comment.is_empty() {
        json += &format!(",\n            \"comment\": \"{}\"", escape(&entry.comment));
    }
    json + "\n        }"
}

/// A dictionary's file content.
pub fn generate(kind: Kind, entries: &[Entry]) -> String {
    let objects: Vec<String> = entries
        .iter()
        .map(|entry| if kind == Kind::Accents { accent_json(entry, "        ") } else { replacement_json(entry) })
        .collect();
    let mut json = String::from("{\n    \"version\": \"1.0\",\n    \"entries\": [\n");
    json += &objects.join(",\n");
    if !objects.is_empty() {
        json.push('\n');
    }
    json + "    ]\n}\n"
}

/// What the engine's own parser finds wrong with an accent entry, so that a
/// typo is caught in the editor and not silently skipped at load time.
pub fn accent_error(entry: &Entry) -> Option<String> {
    let (_, report) = UserLexicon::parse(&format!("{{\"entries\":[{}]}}", accent_json(entry, "")));
    (report.rejected > 0).then_some(report.first_error)
}

/// A dictionary's entries; none when there is no file yet or it cannot be read.
pub fn load(kind: Kind) -> Vec<Entry> {
    let content = kind.path().and_then(|path| std::fs::read(path).ok()).unwrap_or_default();
    parse(kind, &String::from_utf8_lossy(&content))
}

/// Writes a dictionary; false when the file could not be written.
pub fn save(kind: Kind, entries: &[Entry]) -> bool {
    let Some(path) = kind.path() else { return false };
    if let Some(directory) = path.parent() {
        let _ = std::fs::create_dir_all(directory);
    }
    std::fs::write(path, generate(kind, entries)).is_ok()
}

#[cfg(test)]
mod tests {
    use super::*;

    fn entry(grapheme: &str, phoneme: &str) -> Entry {
        Entry { grapheme: grapheme.into(), phoneme: phoneme.into(), ..Entry::default() }
    }

    #[test]
    fn a_replacement_dictionary_is_written_as_before() {
        let entries = [
            entry("Facebook", "Fejzbuk"),
            Entry {
                comment: "a \"quote\"\tand\\".into(), case_sensitive: true, whole_word: false, ..entry("č", "če")
            },
        ];
        let json = generate(Kind::Main, &entries);
        assert_eq!(
            json,
            "{\n    \"version\": \"1.0\",\n    \"entries\": [\n        {\n            \"grapheme\": \"Facebook\",\n            \
             \"phoneme\": \"Fejzbuk\",\n            \"caseSensitive\": false,\n            \"wholeWord\": true\n        },\n        \
             {\n            \"grapheme\": \"č\",\n            \"phoneme\": \"če\",\n            \"caseSensitive\": true,\n            \
             \"wholeWord\": false,\n            \"comment\": \"a \\\"quote\\\"\\tand\\\\\"\n        }\n    ]\n}\n"
        );
        assert_eq!(parse(Kind::Main, &json), entries);
        assert_eq!(generate(Kind::Main, &[]), "{\n    \"version\": \"1.0\",\n    \"entries\": [\n    ]\n}\n");
        assert_eq!(parse(Kind::Main, &generate(Kind::Main, &[])), []);
    }

    #[test]
    fn a_hand_edited_file_keeps_its_entries() {
        let json = r#"{ "entries" : [ { "comment": "grapheme", "grapheme": "a", "phoneme": "b", },
                      { "phoneme": "no text to match" }, {"grapheme":"c","phoneme":"d","wholeWord":false,}, ] }"#;
        let entries = parse(Kind::Main, json);
        assert_eq!(entries.len(), 2);
        assert_eq!(entries[0], Entry { comment: "grapheme".into(), ..entry("a", "b") });
        assert_eq!(entries[1], Entry { whole_word: false, ..entry("c", "d") });
        assert_eq!(parse(Kind::Main, "not json"), []);
        assert_eq!(parse(Kind::Main, r#"{"entries": [{"grapheme": "unterminated"#), []);
    }

    #[test]
    fn the_accent_lexicon_is_written_as_before() {
        let word = Entry { grapheme: "kontr'o:l*".into(), language: "hr".into(), ..Entry::default() };
        let verb = Entry { grapheme: "ur'e:d=i<p".into(), verb: true, comment: "urediti".into(), ..Entry::default() };
        let json = generate(Kind::Accents, &[word.clone(), verb.clone()]);
        assert_eq!(
            json,
            "{\n    \"version\": \"1.0\",\n    \"entries\": [\n        {\n            \"word\": \"kontr'o:l*\",\n            \
             \"language\": \"hr\"\n        },\n        {\n            \"verb\": \"ur'e:d=i<p\",\n            \
             \"comment\": \"urediti\"\n        }\n    ]\n}\n"
        );
        assert_eq!(parse(Kind::Accents, &json), [word.clone(), verb.clone()]);
        // Both keys in one object: a word and a verb
        let both = parse(Kind::Accents, r#"{"entries":[{"word":"a","verb":"b","language":"sl"}]}"#);
        assert_eq!(both.iter().map(|e| (e.grapheme.as_str(), e.verb)).collect::<Vec<_>>(), [("a", false), ("b", true)]);
    }

    #[test]
    fn the_engine_checks_an_accent_entry() {
        let good = Entry { grapheme: "kontr'o:l*".into(), ..Entry::default() };
        assert_eq!(accent_error(&good), None);
        assert_eq!(accent_error(&Entry { grapheme: "ur'e:d=i<p".into(), verb: true, ..Entry::default() }), None);
        let unknown_language = Entry { language: "xx".into(), ..good.clone() };
        assert!(accent_error(&unknown_language).is_some_and(|reason| !reason.is_empty()));
        assert!(accent_error(&Entry { grapheme: "''".into(), ..Entry::default() }).is_some());
    }
}
