//! The dictionaries that shape text before it is spoken: pronunciation
//! replacements, character names for spelling, and emoji names.
//!
//! All are JSON files of one shape, `{ "version": "1.0", "entries": [ {...} ]
//! }`. The bundled files name their fields by what they hold (`emoji` and
//! `text`, `character` and `pronunciation`); the dictionary editors of the
//! apps save every dictionary type as `grapheme` and `phoneme`, so both are
//! read. An entry that lacks a field is skipped; the rest of the file still
//! loads.

pub mod emoji;
pub mod pronunciation;
pub mod spelling;

use serde_json::Value;
use std::path::Path;

/// The entry objects of a dictionary file; empty when it cannot be parsed.
fn entries(json: &str) -> Vec<Value> {
    match serde_json::from_str::<Value>(json) {
        Ok(Value::Object(mut file)) => match file.remove("entries") {
            Some(Value::Array(entries)) => entries,
            _ => Vec::new(),
        },
        _ => Vec::new(),
    }
}

/// The first of `keys` that holds a non-empty string in an entry.
fn text<'a>(entry: &'a Value, keys: &[&str]) -> Option<&'a str> {
    keys.iter().find_map(|key| entry.get(key)?.as_str().filter(|text| !text.is_empty()))
}

/// The pairs of an entry list: what is replaced and what replaces it.
fn pairs<'a>(entries: &'a [Value], from: &'a [&str], to: &'a [&str]) -> impl Iterator<Item = (&'a str, &'a str)> {
    entries.iter().filter_map(move |entry| Some((text(entry, from)?, text(entry, to)?)))
}

/// A file's text; a byte order mark, which Windows editors write, is dropped.
fn read(path: &Path) -> Option<String> {
    let text = std::fs::read_to_string(path).ok()?;
    Some(text.strip_prefix('\u{FEFF}').map(str::to_owned).unwrap_or(text))
}
