//! Croatian recorded voices: Josip and the voices derived from him. The
//! recordings are in `data/hr/Josip`.

use crate::language::Language;
use crate::voice::{Age, Gender, Voice};

pub static JOSIP: Voice = Voice::recorded("josip", "Josip", Language::Croatian, "Josip.bin", 186.0);
/// A child.
pub static DETENCE: Voice = JOSIP.derived("detence", "Detence", Gender::Male, Age::Child, 1.5);
/// A grandmother.
pub static BABA: Voice = JOSIP.derived("baba", "Baba", Gender::Female, Age::Senior, 1.2);
