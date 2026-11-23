//! Generated tables embedded in the program as text: one entry per line,
//! `key` or `key<TAB>value`, the lines sorted by their keys in code point
//! order. They are searched in place, so no copy of a table is built at
//! start, however large it is.

use std::cmp::Ordering;

/// The value of `key` in a table (empty for a line that is a key alone).
pub fn lookup<'a>(table: &'a str, key: &str) -> Option<&'a str> {
    let bytes = table.as_bytes();
    // `lo` is always the start of a line.
    let (mut lo, mut hi) = (0, bytes.len());
    while lo < hi {
        let mid = lo + (hi - lo) / 2;
        let start = bytes[..mid].iter().rposition(|&b| b == b'\n').map_or(0, |at| at + 1);
        let end = bytes[mid..].iter().position(|&b| b == b'\n').map_or(bytes.len(), |at| mid + at);
        let line = &table[start..end];
        let (entry, value) = line.split_once('\t').unwrap_or((line, ""));
        match key.cmp(entry) {
            Ordering::Equal => return Some(value),
            Ordering::Less => hi = start,
            Ordering::Greater => lo = end + 1,
        }
    }
    None
}

/// Whether a table has `key`.
pub fn contains(table: &str, key: &str) -> bool {
    lookup(table, key).is_some()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn finds_every_line_and_nothing_else() {
        let table = "a\nab\t1\nb\t\nšuma\t7\nžaba\n";
        for (key, value) in [("a", ""), ("ab", "1"), ("b", ""), ("šuma", "7"), ("žaba", "")] {
            assert_eq!(lookup(table, key), Some(value));
        }
        for key in ["", "aa", "abc", "c", "š", "žabac", "1"] {
            assert_eq!(lookup(table, key), None);
        }
        assert!(!contains("", "a") && contains("a", "a") && !contains("a\n", ""));
    }
}
