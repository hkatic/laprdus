//! Pairs and classes of sounds the assimilation rules of the languages use.

use crate::formant::phones::Ph;

/// The voiced partner of a voiceless obstruent ([ts] has [dz], [x] has [ɣ]).
pub fn voiced_of(ph: Ph) -> Ph {
    match ph {
        Ph::P => Ph::B,
        Ph::T => Ph::D,
        Ph::K => Ph::G,
        Ph::S => Ph::Z,
        Ph::Sh => Ph::Zh,
        Ph::Sj => Ph::Zj,
        Ph::Ch => Ph::Dzh,
        Ph::Tj => Ph::Dj,
        Ph::C => Ph::Dz,
        Ph::H => Ph::Gh,
        _ => ph,
    }
}

/// The voiceless partner of a voiced obstruent.
pub fn voiceless_of(ph: Ph) -> Ph {
    match ph {
        Ph::B => Ph::P,
        Ph::D => Ph::T,
        Ph::G => Ph::K,
        Ph::Z => Ph::S,
        Ph::Zh => Ph::Sh,
        Ph::Zj => Ph::Sj,
        Ph::Dzh => Ph::Ch,
        Ph::Dj => Ph::Tj,
        Ph::Dz => Ph::C,
        Ph::Gh => Ph::H,
        _ => ph,
    }
}

/// š, ž, č, dž, ć, đ.
pub fn is_postalveolar(ph: Ph) -> bool {
    matches!(ph, Ph::Sh | Ph::Zh | Ph::Ch | Ph::Dzh | Ph::Tj | Ph::Dj)
}
