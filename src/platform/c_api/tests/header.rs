//! `include/laprdus/laprdus_api.h` is written by hand: every function it
//! declares must be exported by the library, and nothing else may be.

use std::collections::BTreeSet;

/// The `laprdus_*` names that stand right before `(` in a source text.
fn functions(source: &str, prefix: &str) -> BTreeSet<String> {
    source
        .match_indices(prefix)
        .filter_map(|(at, _)| {
            let name = &source[at + prefix.len() - "laprdus_".len()..];
            let end = name.find(|c: char| !(c.is_ascii_alphanumeric() || c == '_'))?;
            name[end..].starts_with('(').then(|| name[..end].to_owned())
        })
        .collect()
}

#[test]
fn the_header_declares_what_the_library_exports() {
    let declared = functions(include_str!("../include/laprdus/laprdus_api.h"), "LAPRDUS_CALL laprdus_");
    let exported = functions(include_str!("../src/lib.rs"), "fn laprdus_");
    assert!(declared.len() > 90, "only {} declarations were found in the header", declared.len());
    assert_eq!(declared, exported);
}
