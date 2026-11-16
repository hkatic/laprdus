//! The brightness setting as a tilt of the whole output.
//!
//! The signal is split by a one-pole low-pass into its body and its top, and
//! the top is scaled: 50 leaves the sound as it is, 0 folds the top away from
//! a pivot of 800 Hz (the setting at which a phone's speaker at full volume
//! stops biting), 100 triples it (+9.5 dB above 1.5 kHz). The recorded voices
//! have used it since the setting came; the formant voices apply it too since
//! November 2026, on top of their source emphasis, so that brightness also
//! reaches the hissing consonants and the bursts, which never pass through
//! the voice source.

use crate::params::{
    BRIGHTNESS_DULL_HZ, BRIGHTNESS_SHARP_GAIN, BRIGHTNESS_SHARP_HZ, VOICE_QUALITY_DEFAULT, VOICE_QUALITY_MAX,
    VOICE_QUALITY_MIN,
};

/// Tilts `wave` for a brightness setting of 0-100; 50 leaves it untouched.
pub fn tilt_brightness(wave: &mut [f32], fs: u32, brightness: i32) {
    let v = brightness.clamp(VOICE_QUALITY_MIN, VOICE_QUALITY_MAX);
    if v == VOICE_QUALITY_DEFAULT {
        return;
    }
    // The sharp side scales the top above BRIGHTNESS_SHARP_HZ; the dull side
    // folds the top away from a pivot that moves down to BRIGHTNESS_DULL_HZ.
    let (top_gain, pivot) = if v > 50 {
        (1.0 + (BRIGHTNESS_SHARP_GAIN - 1.0) * (v - 50) as f32 / 50.0, BRIGHTNESS_SHARP_HZ)
    } else {
        let share = (50 - v) as f32 / 50.0; // 0 at 50, 1 at 0
        (1.0 - share, BRIGHTNESS_SHARP_HZ * (BRIGHTNESS_DULL_HZ / BRIGHTNESS_SHARP_HZ).powf(share))
    };
    let coef = (-2.0 * std::f32::consts::PI * pivot / fs as f32).exp();
    let mut low = 0f32;
    for x in wave {
        low = coef * low + (1.0 - coef) * *x;
        *x = low + top_gain * (*x - low);
    }
}
