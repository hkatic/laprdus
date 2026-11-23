//! Marks the number converter leaves in the text for the front end and the
//! synthesizers.
//!
//! All are Unicode noncharacters (U+FDD0..U+FDEF are set aside for a
//! program's internal use and are not interchanged), so no text a host sends
//! has them: a private use character can come from an icon font in the label
//! of a button. They sit between two spaces or after a word, and the clause
//! splitter passes them on as it does any letter.

/// A short pause inside a clause, before the next word: between the hours,
/// minutes and seconds of a clock time (12:30:45) and between the day, month
/// and year of a numeric date (10.11.2026.). The melody of the sentence goes
/// on over it; a comma would end the clause there ("u dvanaest, trideset u
/// uredu"). Written as a word of its own, `" <mark> "`.
pub const MINOR_BREAK_MARK: char = '\u{FDD0}';

/// Directly after the letters of a designation written with capital letters
/// glued to a number (A17, F5, S22, AB12): they are letters, read by their
/// names or sounds as the spelling mode says, never the words they are
/// spelled like (the conjunctions a and i, the prepositions u, o, s and k,
/// the words na, do, on).
pub const LETTER_CODE_MARK: char = '\u{FDD1}';

/// A pause between two items a screen reader says in one breath, before the
/// next word: the contents of a spreadsheet cell and its address (Marjan
/// A17), an address and the number after it (A20 5). As
/// [`MINOR_BREAK_MARK`] in every way but its length, the user's whole comma
/// pause: half of it, 54 ms at a screen reader's settings, was not heard as a
/// pause between the two (a listener's report through NVDA in Excel,
/// November 2026).
pub const ITEM_BREAK_MARK: char = '\u{FDD2}';
