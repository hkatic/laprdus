//! Serbian recorded voices: Vlado and the voice derived from him. The
//! recordings are in `data/sr/Vlado`.

use crate::language::Language;
use crate::voice::{Age, Gender, Voice};

pub static VLADO: Voice = Voice::recorded("vlado", "Vlado", Language::Serbian, "Vlado.bin", 97.0);
/// A grandfather; "djed" before version 2.0.
pub static DJEDO: Voice = VLADO.derived("djedo", "Đedo", Gender::Male, Age::Senior, 0.75);
