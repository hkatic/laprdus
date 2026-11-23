//! Powers of 2 and 10 as the C library computes them.
//!
//! The optimizer may replace `pow(2, x)` by `exp2(x)`, whose last bit can
//! differ; the reference recordings in `tests/golden` were made with `pow`,
//! so the base is hidden from it.

use std::hint::black_box;

pub fn pow2(x: f64) -> f64 {
    black_box(2f64).powf(x)
}

pub fn pow2f(x: f32) -> f32 {
    black_box(2f32).powf(x)
}

pub fn pow10(x: f64) -> f64 {
    black_box(10f64).powf(x)
}

pub fn pow10f(x: f32) -> f32 {
    black_box(10f32).powf(x)
}
