// -*- coding: utf-8 -*-
// sonic_processor.hpp - Sonic library wrapper for time-stretching and pitch-shifting
// Provides independent control of speech rate and pitch

#ifndef LAPRDUS_SONIC_PROCESSOR_HPP
#define LAPRDUS_SONIC_PROCESSOR_HPP

#include "laprdus/types.hpp"

namespace laprdus {
namespace sonic {

/**
 * Change playback speed without changing pitch (time-stretching).
 * Uses Sonic's PICOLA algorithm optimized for speech.
 *
 * @param input Audio buffer to process.
 * @param speed Speed factor (1.0 = normal, 2.0 = 2x faster, 0.5 = half speed).
 *              Valid range: 0.05 to 20.0
 * @return Time-stretched audio with original pitch preserved.
 */
AudioBuffer change_speed(const AudioBuffer& input, float speed);

/**
 * Change pitch without changing duration (pitch-shifting).
 * Uses Sonic's pitch-shifting algorithm.
 *
 * @param input Audio buffer to process.
 * @param pitch Pitch factor (1.0 = normal, 1.5 = 50% higher, 0.75 = 25% lower).
 *              Valid range: 0.05 to 20.0
 * @return Pitch-shifted audio with original duration preserved.
 */
AudioBuffer change_pitch(const AudioBuffer& input, float pitch);

/**
 * Change both speed and pitch independently.
 *
 * @param input Audio buffer to process.
 * @param speed Speed factor (1.0 = normal).
 * @param pitch Pitch factor (1.0 = normal).
 * @return Processed audio with independent speed and pitch changes.
 */
AudioBuffer process(const AudioBuffer& input, float speed, float pitch);

/**
 * Apply pitch envelope to audio using Sonic.
 * Processes audio in chunks with varying pitch factors.
 *
 * @param input Audio buffer to process.
 * @param envelope Pitch factor for each sample position.
 * @return Processed audio with pitch envelope applied.
 */
AudioBuffer apply_pitch_envelope(const AudioBuffer& input,
                                  const std::vector<float>& envelope);

} // namespace sonic
} // namespace laprdus

#endif // LAPRDUS_SONIC_PROCESSOR_HPP
