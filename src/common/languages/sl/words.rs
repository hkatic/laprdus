//! Slovenian letter, symbol and character names and the classes of its
//! function words.
//!
//! Declarations only: the bodies are placeholders until the Slovenian rules
//! are ported from `formant_slovenian.cpp`.

/// The name of a letter in Slovenian ("b" -> "be", "š" -> "eš", "w" ->
/// "dvojni ve"; Slovenski pravopis § 7); empty for anything else. The letter
/// is as `spelling_letter()` returns it.
pub fn letter_name(_letter: &[char]) -> &'static str {
    // TODO(sl)
    ""
}

/// The name of a symbol read wherever it stands inside a clause (".", ":",
/// "%": pika, dvopičje, odstotkov); `None` otherwise.
pub fn symbol_name(_c: char) -> Option<&'static str> {
    // TODO(sl)
    None
}

/// The name of a digit, a punctuation mark or a symbol when spelled ("1" ->
/// "ena", "." -> "pika", "@" -> "afna"); `None` for a character without one.
pub fn spelled_name(_character: &str) -> Option<&'static str> {
    // TODO(sl)
    None
}

/// A word that leans on the next one: the monosyllabic prepositions and
/// conjunctions, ne, naj.
pub fn is_proclitic(_w: &[char]) -> bool {
    // TODO(sl)
    false
}

/// A word that leans on the one before it: the forms of biti and the future,
/// bi, the short pronouns.
pub fn is_enclitic(_w: &[char]) -> bool {
    // TODO(sl)
    false
}

/// A word stressed more weakly than a content word.
pub fn is_function_word(_w: &[char]) -> bool {
    // TODO(sl)
    false
}

/// A word that opens a wh-question (kdo, kaj, kje, kako, zakaj, kateri, ...);
/// the C++ `question_word_set()` as a test.
pub fn is_question_word(_w: &[char]) -> bool {
    // TODO(sl)
    false
}
