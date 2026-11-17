//! Slovenian.

mod clause;
pub mod frontend;
pub mod lexicon;
pub mod marks;
pub mod nouns;
pub mod numerals;
pub mod phonemize;
pub mod readings;
mod rules;
pub mod words;

pub use rules::Slovenian;

#[cfg(test)]
mod fuzz;
#[cfg(test)]
mod tests;
