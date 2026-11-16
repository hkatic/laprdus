//! Slovenian.

mod clause;
pub mod frontend;
pub mod lexicon;
pub mod marks;
pub mod nouns;
pub mod numerals;
pub mod phonemize;
pub mod readings;
pub mod words;

#[cfg(test)]
mod fuzz;
#[cfg(test)]
mod tests;
