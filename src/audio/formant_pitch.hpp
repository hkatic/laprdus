// -*- coding: utf-8 -*-
// formant_pitch.hpp - Formant-preserving pitch shifting
// Uses STFT-based pitch shifting with cepstral formant preservation

#ifndef LAPRDUS_FORMANT_PITCH_HPP
#define LAPRDUS_FORMANT_PITCH_HPP

#include "laprdus/types.hpp"

namespace laprdus {
namespace formant {

/**
 * Pitch-shift audio while preserving formants (vocal character).
 * Uses cepstral analysis to separate fundamental frequency (F0) from
 * spectral envelope (formants), shifts only F0, and recombines.
 *
 * This prevents the "chipmunk effect" that occurs when formants shift
 * along with pitch (as in simple resampling or Sonic pitch shifting).
 *
 * @param input Audio buffer to process (16-bit PCM, 22050Hz mono).
 * @param pitch Pitch factor (1.0 = unchanged, 1.5 = 50% higher, 0.75 = 25% lower).
 *              Valid range: 0.5 to 2.0
 * @param quefrency_ms Formant preservation parameter in milliseconds.
 *                     Smaller values = sharper formant preservation.
 *                     Default: 1.0ms works well for speech at 22050Hz.
 * @return Pitch-shifted audio with preserved formants (same duration as input).
 */
AudioBuffer change_pitch_preserve_formants(
    const AudioBuffer& input,
    float pitch,
    float quefrency_ms = 1.0f);

} // namespace formant
} // namespace laprdus

#endif // LAPRDUS_FORMANT_PITCH_HPP
