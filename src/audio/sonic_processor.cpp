// -*- coding: utf-8 -*-
// sonic_processor.cpp - Sonic library wrapper implementation

#include "sonic_processor.hpp"
#include "sonic/sonic.h"
#include <algorithm>
#include <cmath>

namespace laprdus {
namespace sonic {

namespace {

// Process audio through Sonic stream with given speed and pitch
AudioBuffer process_with_sonic(const AudioBuffer& input, float speed, float pitch) {
    if (input.empty()) {
        return input;
    }

    // Create Sonic stream
    sonicStream stream = sonicCreateStream(
        static_cast<int>(input.sample_rate),
        static_cast<int>(input.channels)
    );

    if (!stream) {
        // Memory allocation failed, return original
        return input;
    }

    // Configure processing parameters
    sonicSetSpeed(stream, speed);
    sonicSetPitch(stream, pitch);

    // Write input samples
    int write_result = sonicWriteShortToStream(
        stream,
        input.samples.data(),
        static_cast<int>(input.samples.size())
    );

    if (write_result == 0) {
        // Memory allocation failed
        sonicDestroyStream(stream);
        return input;
    }

    // Flush to ensure all output is generated
    sonicFlushStream(stream);

    // Read output samples
    int available = sonicSamplesAvailable(stream);

    AudioBuffer output;
    output.sample_rate = input.sample_rate;
    output.bits_per_sample = input.bits_per_sample;
    output.channels = input.channels;

    if (available > 0) {
        output.samples.resize(static_cast<size_t>(available));
        int read_count = sonicReadShortFromStream(
            stream,
            output.samples.data(),
            available
        );
        output.samples.resize(static_cast<size_t>(read_count));
    }

    sonicDestroyStream(stream);

    return output;
}

} // anonymous namespace

AudioBuffer change_speed(const AudioBuffer& input, float speed) {
    // Clamp to Sonic's valid range
    speed = std::clamp(speed, 0.05f, 20.0f);

    // Skip processing if speed is essentially 1.0
    if (std::abs(speed - 1.0f) < 0.01f) {
        return input;
    }

    // Speed only, pitch = 1.0 (unchanged)
    return process_with_sonic(input, speed, 1.0f);
}

AudioBuffer change_pitch(const AudioBuffer& input, float pitch) {
    // Clamp to Sonic's valid range
    pitch = std::clamp(pitch, 0.05f, 20.0f);

    // Skip processing if pitch is essentially 1.0
    if (std::abs(pitch - 1.0f) < 0.01f) {
        return input;
    }

    // Pitch only, speed = 1.0 (unchanged)
    return process_with_sonic(input, 1.0f, pitch);
}

AudioBuffer process(const AudioBuffer& input, float speed, float pitch) {
    // Clamp to Sonic's valid range
    speed = std::clamp(speed, 0.05f, 20.0f);
    pitch = std::clamp(pitch, 0.05f, 20.0f);

    // Skip processing if both are essentially 1.0
    if (std::abs(speed - 1.0f) < 0.01f && std::abs(pitch - 1.0f) < 0.01f) {
        return input;
    }

    return process_with_sonic(input, speed, pitch);
}

AudioBuffer apply_pitch_envelope(const AudioBuffer& input,
                                  const std::vector<float>& envelope) {
    if (input.empty() || envelope.empty()) {
        return input;
    }

    // Process in chunks, each with its own pitch factor
    // Chunk size: ~23ms at 22050Hz (512 samples)
    constexpr size_t CHUNK_SIZE = 512;

    AudioBuffer output;
    output.sample_rate = input.sample_rate;
    output.bits_per_sample = input.bits_per_sample;
    output.channels = input.channels;
    output.samples.reserve(input.samples.size());

    size_t num_samples = input.samples.size();
    size_t chunk_count = (num_samples + CHUNK_SIZE - 1) / CHUNK_SIZE;

    for (size_t chunk = 0; chunk < chunk_count; ++chunk) {
        size_t start = chunk * CHUNK_SIZE;
        size_t end = std::min(start + CHUNK_SIZE, num_samples);
        size_t chunk_len = end - start;

        // Get average pitch factor for this chunk (sample at midpoint)
        size_t mid_sample = start + chunk_len / 2;
        mid_sample = std::min(mid_sample, envelope.size() - 1);
        float pitch_factor = envelope[mid_sample];

        // Skip processing if pitch is essentially 1.0
        if (std::abs(pitch_factor - 1.0f) < 0.01f) {
            output.samples.insert(output.samples.end(),
                                  input.samples.begin() + start,
                                  input.samples.begin() + end);
            continue;
        }

        // Create temporary buffer for this chunk
        AudioBuffer chunk_buffer;
        chunk_buffer.samples.assign(input.samples.begin() + start,
                                    input.samples.begin() + end);
        chunk_buffer.sample_rate = input.sample_rate;
        chunk_buffer.bits_per_sample = input.bits_per_sample;
        chunk_buffer.channels = input.channels;

        // Apply pitch shift to chunk using Sonic
        AudioBuffer shifted = change_pitch(chunk_buffer, pitch_factor);

        // Append to result
        output.samples.insert(output.samples.end(),
                             shifted.samples.begin(),
                             shifted.samples.end());
    }

    return output;
}

} // namespace sonic
} // namespace laprdus
