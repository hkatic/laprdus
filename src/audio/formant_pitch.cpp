// -*- coding: utf-8 -*-
// formant_pitch.cpp - Formant-preserving pitch shifting using Signalsmith Stretch
//
// Uses the Signalsmith Stretch library with default preset optimized by the library authors.
// Falls back to Sonic for short audio segments where Signalsmith would produce artifacts.

#include "formant_pitch.hpp"
#include "sonic_processor.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>
#include "signalsmith-stretch.h"

namespace laprdus {
namespace formant {

AudioBuffer change_pitch_preserve_formants(const AudioBuffer& input, float pitch_factor, float /*quefrency_ms*/) {
    if (input.empty()) return input;
    if (std::abs(pitch_factor - 1.0f) < 0.01f) return input;

    pitch_factor = std::clamp(pitch_factor, 0.5f, 2.0f);

    const int N = static_cast<int>(input.samples.size());
    const float sample_rate = static_cast<float>(input.sample_rate);

    // Signalsmith-stretch needs minimum audio length to work properly
    // Default preset uses ~120ms blocks, so we need at least that much
    // For very short segments, use Sonic as fallback (it works on short audio)
    const int MIN_SAMPLES = static_cast<int>(sample_rate * 0.15f);  // 150ms minimum

    if (N < MIN_SAMPLES) {
        // Use Sonic for short segments - it works well on short audio
        // Sonic shifts formants (not ideal) but better than no pitch change
        AudioBuffer result = sonic::change_pitch(input, pitch_factor);
        // Safety: if Sonic returned empty, return original
        if (result.samples.empty()) return input;
        return result;
    }

    signalsmith::stretch::SignalsmithStretch<float> stretch;

    // Use the library's default preset - it's been optimized by the authors
    stretch.presetDefault(1, sample_rate);

    // Set pitch shift
    float semitones = 12.0f * std::log2(pitch_factor);
    stretch.setTransposeSemitones(semitones);

    // Enable formant preservation with compensation
    stretch.setFormantFactor(1.0f, true);

    // Convert input to float
    std::vector<float> input_float(N);
    for (int i = 0; i < N; i++) {
        input_float[i] = input.samples[i] / 32768.0f;
    }

    const float* input_ptr = input_float.data();
    std::vector<float> output_float(N);
    float* output_ptr = output_float.data();

    stretch.exact(&input_ptr, N, &output_ptr, N);

    // Convert back to int16
    AudioBuffer result;
    result.sample_rate = input.sample_rate;
    result.bits_per_sample = input.bits_per_sample;
    result.channels = input.channels;
    result.samples.resize(N);

    // Convert output and check if valid
    for (int i = 0; i < N; i++) {
        float val = output_float[i] * 32768.0f;
        result.samples[i] = static_cast<int16_t>(
            std::clamp(val, -32768.0f, 32767.0f));
    }

    return result;
}

} // namespace formant
} // namespace laprdus
