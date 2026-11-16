//! The marks the engine puts at the end of a Slovenian clause, since the
//! front end sees one clause at a time and some words are told by what the
//! sentence goes on with. Private use characters, consumed by the Slovenian
//! front end, never spoken.

/// The mark the engine puts at the end of a clause whose comma is followed
/// by a clause [`opens_complement_clause`] says yes to: a verb of saying or
/// thinking is told by what follows it. Meni, da je prav. Ni obvestil, da
/// pride.
pub const BEFORE_COMPLEMENT_MARK: char = '\u{E000}';

/// The same for a clause before a comma or a semicolon in a sentence that
/// ends in a question mark, which only its last clause shows: Lahko delo
/// opravi vsak, ki hoče?
pub const IN_QUESTION_MARK: char = '\u{E001}';

/// The same for a sentence that ends in an exclamation mark: Veseli se,
/// dragi prijatelj! (the imperative vesêli; Veseli se novega dneva. is the
/// present)
pub const IN_EXCLAMATION_MARK: char = '\u{E002}';

/// The mark of a clause that is one capitalized word before a comma or after
/// one: an address (Pero, kje si? Pero, pridi sem. Hvala, Pero.), so the
/// word is read by the tables of names as inside a clause, where the name
/// Pêro wins over the pen peró (a listener's answer of 2026-11-03: the name
/// whatever ends the sentence). Pero je na mizi. stays the pen.
pub const VOCATIVE_MARK: char = '\u{E003}';

/// Whether a clause begins with da, naj, kdaj, kako ...
pub fn opens_complement_clause(_text: &[char]) -> bool {
    // TODO(sl)
    false
}

/// Whether a clause is one capitalized word (Pero, Janez, Mama), the shape
/// of an address before a comma; for [`VOCATIVE_MARK`], with the sentence's
/// end.
pub fn is_address(_text: &[char]) -> bool {
    // TODO(sl)
    false
}
