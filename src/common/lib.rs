//! Laprdus: a Croatian, Serbian, Bosnian and Slovenian text-to-speech engine.
//!
//! * [`formant`] and `concatenative` hold the two kinds of synthesis,
//! * `languages` one module per language: adding a language is adding a
//!   folder there and one line to its list.

pub mod audio;
pub mod concatenative;
pub mod config;
pub mod dictionary;
pub mod engine;
pub mod formant;
pub mod language;
pub mod languages;
mod math;
pub mod params;
pub mod prosody;
pub mod text;
pub mod voice;

/// Sample rate of all audio the engine produces (Hz, 16-bit mono).
pub const SAMPLE_RATE: u32 = 22050;

/// Silence as long as a pause.
pub fn silence(duration_ms: u32) -> impl Iterator<Item = i16> {
    std::iter::repeat_n(0, (SAMPLE_RATE * duration_ms / 1000) as usize)
}
