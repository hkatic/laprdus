//! Spelling dictionary: what a character is called when text is read
//! character by character. It holds digits, punctuation and symbols; letters
//! are named by the language.

use super::{entries, pairs, read};
use std::collections::HashMap;
use std::path::Path;

/// Character → pronunciation, without regard to case.
#[derive(Clone, Debug, Default)]
pub struct SpellingDictionary {
    entries: HashMap<String, String>,
}

/// The key of a character: ASCII letters and č, ć, đ, š, ž in upper case.
fn key(character: &str) -> String {
    character
        .chars()
        .map(|c| match c {
            'č' => 'Č',
            'ć' => 'Ć',
            'đ' => 'Đ',
            'š' => 'Š',
            'ž' => 'Ž',
            _ => c.to_ascii_uppercase(),
        })
        .collect()
}

impl SpellingDictionary {
    /// Replaces the entries by those of a JSON file's content. False when
    /// the dictionary is empty afterwards.
    pub fn load(&mut self, json: &str) -> bool {
        self.entries.clear();
        self.append(json)
    }

    /// Adds the entries of a JSON file's content; a later entry for a
    /// character replaces an earlier one.
    pub fn append(&mut self, json: &str) -> bool {
        for (character, pronunciation) in
            pairs(&entries(json), &["character", "grapheme"], &["pronunciation", "phoneme"])
        {
            self.add(character, pronunciation);
        }
        !self.entries.is_empty()
    }

    pub fn load_file(&mut self, path: &Path) -> bool {
        read(path).is_some_and(|json| self.load(&json))
    }

    pub fn append_file(&mut self, path: &Path) -> bool {
        read(path).is_some_and(|json| self.append(&json))
    }

    pub fn add(&mut self, character: &str, pronunciation: &str) {
        if !character.is_empty() && !pronunciation.is_empty() {
            self.entries.insert(key(character), pronunciation.to_owned());
        }
    }

    /// The pronunciation of a character, if it has an entry.
    pub fn find(&self, character: &str) -> Option<&str> {
        self.entries.get(&key(character)).map(String::as_str)
    }

    pub fn clear(&mut self) {
        self.entries.clear();
    }

    pub fn len(&self) -> usize {
        self.entries.len()
    }

    pub fn is_empty(&self) -> bool {
        self.entries.is_empty()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn both_field_names_and_any_case() {
        let mut d = SpellingDictionary::default();
        assert!(d.load(r#"{"entries": [{"character": ".", "pronunciation": "točka"}, {"character": "x"}]}"#));
        assert!(d.append(r#"{"entries": [{"grapheme": "š", "phoneme": "ša"}]}"#));
        assert_eq!((d.find("."), d.find("Š"), d.find("x"), d.len()), (Some("točka"), Some("ša"), None, 2));
        assert!(!d.load("not json"));
    }
}
