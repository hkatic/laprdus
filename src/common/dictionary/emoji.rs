//! Emoji dictionary: emoji replaced by their names ("😀" → "nasmijano lice").

use super::{entries, pairs, read};
use std::collections::HashMap;
use std::path::Path;

/// Longest emoji sequence looked for (bytes of UTF-8); complex ZWJ sequences
/// span 25 and more.
const MAX_EMOJI_BYTES: usize = 32;

/// An emoji without its variation selectors (U+FE0F emoji presentation,
/// U+FE0E text presentation): ❤️ and ❤ are the same entry.
fn without_variation_selectors(emoji: &str) -> String {
    emoji.chars().filter(|c| !matches!(c, '\u{FE0E}' | '\u{FE0F}')).collect()
}

/// Emoji → spoken text.
#[derive(Clone, Debug, Default)]
pub struct EmojiDictionary {
    entries: HashMap<String, String>,
}

impl EmojiDictionary {
    /// Replaces the entries by those of a JSON file's content. False when
    /// the dictionary is empty afterwards.
    pub fn load(&mut self, json: &str) -> bool {
        self.entries.clear();
        self.append(json)
    }

    /// Adds the entries of a JSON file's content. An entry also matches its
    /// emoji without variation selectors, unless that form has an entry of
    /// its own.
    pub fn append(&mut self, json: &str) -> bool {
        for (emoji, text) in pairs(&entries(json), &["emoji", "grapheme"], &["text", "phoneme"]) {
            self.entries.insert(emoji.to_owned(), text.to_owned());
            let plain = without_variation_selectors(emoji);
            if plain != emoji && !plain.is_empty() {
                self.entries.entry(plain).or_insert_with(|| text.to_owned());
            }
        }
        !self.entries.is_empty()
    }

    pub fn load_file(&mut self, path: &Path) -> bool {
        read(path).is_some_and(|json| self.load(&json))
    }

    pub fn append_file(&mut self, path: &Path) -> bool {
        read(path).is_some_and(|json| self.append(&json))
    }

    /// Adds one entry, which wins over any other for the emoji with or
    /// without its variation selectors.
    pub fn add(&mut self, emoji: &str, text: &str) {
        if emoji.is_empty() || text.is_empty() {
            return;
        }
        self.entries.insert(emoji.to_owned(), text.to_owned());
        let plain = without_variation_selectors(emoji);
        if plain != emoji && !plain.is_empty() {
            self.entries.insert(plain, text.to_owned());
        }
    }

    /// The spoken text of an emoji; the input may carry variation selectors
    /// the entry lacks.
    pub fn find(&self, emoji: &str) -> Option<&str> {
        self.entries.get(emoji).map(String::as_str).or_else(|| {
            let plain = without_variation_selectors(emoji);
            self.entries.get(&plain).filter(|_| plain != emoji && !plain.is_empty()).map(String::as_str)
        })
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

/// Text with its emoji replaced by their names, set off by spaces. The first
/// layer that knows an emoji names it, and the longest sequence at a place
/// wins. With any entries at all, runs of spaces come out as one space and
/// the text is trimmed.
pub fn replace_emojis(text: &str, layers: &[&EmojiDictionary]) -> String {
    let layers: Vec<&EmojiDictionary> = layers.iter().copied().filter(|layer| !layer.is_empty()).collect();
    if layers.is_empty() {
        return text.to_owned();
    }

    let mut result = String::with_capacity(text.len() * 2);
    let mut pos = 0;
    while pos < text.len() {
        let rest = &text[pos..];
        let found = (1..=MAX_EMOJI_BYTES.min(rest.len()))
            .rev()
            .filter(|&len| rest.is_char_boundary(len))
            .find_map(|len| layers.iter().find_map(|layer| layer.find(&rest[..len])).map(|spoken| (len, spoken)));
        if let Some((len, spoken)) = found {
            if !result.is_empty() && !result.ends_with(' ') {
                result.push(' ');
            }
            result.push_str(spoken);
            result.push(' ');
            pos += len;
        } else if let Some(c) = rest.chars().next() {
            result.push(c);
            pos += c.len_utf8();
        }
    }

    // One space between words, none at the ends.
    result.split(' ').filter(|word| !word.is_empty()).collect::<Vec<_>>().join(" ")
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn replaces_the_longest_sequence() {
        let mut d = EmojiDictionary::default();
        assert!(d.load(
            r#"{"entries": [{"emoji": "❤️", "text": "srce"}, {"emoji": "👍", "text": "palac gore"},
                           {"grapheme": "👍🏽", "phoneme": "tamni palac"}, {"emoji": "x"}]}"#
        ));
        assert_eq!(replace_emojis("Bravo👍  ❤ i ❤️!👍🏽", &[&d]), "Bravo palac gore srce i srce ! tamni palac");
        let mut user = EmojiDictionary::default();
        user.add("👍", "lajk");
        assert_eq!(replace_emojis("👍", &[&user, &d]), "lajk");
        assert_eq!(replace_emojis(" a  b ", &[&EmojiDictionary::default()]), " a  b ");
    }
}
