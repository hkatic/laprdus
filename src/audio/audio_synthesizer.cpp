// -*- coding: utf-8 -*-
// audio_synthesizer.cpp - Phoneme concatenation implementation

#include "audio_synthesizer.hpp"
#include "sonic_processor.hpp"
#include "formant_pitch.hpp"
#include <algorithm>
#include <cmath>

namespace laprdus {

// =============================================================================
// Constructor
// =============================================================================

AudioSynthesizer::AudioSynthesizer(const PhonemeData& phoneme_data)
    : m_phoneme_data(phoneme_data)
{
    // Default voice parameters are already set in VoiceParams
}

// =============================================================================
// Synthesize from Phoneme Tokens
// =============================================================================

AudioBuffer AudioSynthesizer::synthesize(const std::vector<PhonemeToken>& tokens) {
    AudioBuffer result;
    result.sample_rate = SAMPLE_RATE;
    result.bits_per_sample = BITS_PER_SAMPLE;
    result.channels = NUM_CHANNELS;

    if (tokens.empty()) {
        return result;
    }

    // Estimate total size for efficiency
    size_t estimated_samples = 0;
    for (const auto& token : tokens) {
        auto samples = m_phoneme_data.get_phoneme(token.phoneme);
        estimated_samples += samples.size();
    }
    result.samples.reserve(estimated_samples);

    // Crossfade settings
    constexpr size_t CROSSFADE_SAMPLES = 64;  // ~3ms at 22050Hz

    AudioBuffer prev_phoneme;

    for (const auto& token : tokens) {
        // Get audio for this phoneme
        AudioBuffer phoneme_audio = get_phoneme_audio(token.phoneme);

        if (phoneme_audio.empty()) {
            continue;
        }

        // Apply crossfade with previous phoneme
        if (!prev_phoneme.empty() && !result.samples.empty()) {
            apply_crossfade(result, phoneme_audio, CROSSFADE_SAMPLES);
        } else {
            // First phoneme or no crossfade needed
            result.samples.insert(result.samples.end(),
                                 phoneme_audio.samples.begin(),
                                 phoneme_audio.samples.end());
        }

        prev_phoneme = std::move(phoneme_audio);

        // Emit chunk if streaming
        if (m_stream_callback && m_stream_chunk_samples > 0) {
            while (result.samples.size() >= m_stream_chunk_samples) {
                AudioBuffer chunk;
                chunk.sample_rate = result.sample_rate;
                chunk.bits_per_sample = result.bits_per_sample;
                chunk.channels = result.channels;
                chunk.samples.assign(result.samples.begin(),
                                    result.samples.begin() + m_stream_chunk_samples);

                emit_chunk(chunk);

                result.samples.erase(result.samples.begin(),
                                    result.samples.begin() + m_stream_chunk_samples);
            }
        }
    }

    // Apply global voice parameters
    if (std::abs(m_voice_params.volume - 1.0f) > 0.01f) {
        result = apply_volume(result, m_voice_params.volume);
    }

    if (std::abs(m_voice_params.speed - 1.0f) > 0.01f) {
        result = apply_rate(result, m_voice_params.speed);
    }

    if (std::abs(m_voice_params.pitch - 1.0f) > 0.01f) {
        result = apply_pitch(result, m_voice_params.pitch);
    }

    // Apply user pitch preference (formant-preserving)
    // This is separate from voice character pitch (above) which shifts formants
    if (std::abs(m_voice_params.user_pitch - 1.0f) > 0.01f) {
        result = apply_user_pitch(result, m_voice_params.user_pitch);
    }

    // Flush remaining samples if streaming
    if (m_stream_callback && !result.samples.empty()) {
        emit_chunk(result);
        result.samples.clear();
    }

    return result;
}

// =============================================================================
// Synthesize Segment with Inflection
// =============================================================================

AudioBuffer AudioSynthesizer::synthesize_segment(
    const TextSegment& segment,
    const std::vector<PhonemeToken>& tokens) {

    // First, synthesize raw audio
    AudioBuffer raw_audio = synthesize(tokens);

    if (raw_audio.empty()) {
        return raw_audio;
    }

    // Apply inflection based on segment punctuation
    AudioBuffer inflected = m_inflection.apply_inflection(
        raw_audio,
        segment.inflection,
        tokens.size()
    );

    // Add pause after segment if needed
    if (segment.trailing_punct != Punctuation::NONE) {
        uint32_t pause_ms = m_inflection.get_pause_duration(segment.trailing_punct);

        if (pause_ms > 0) {
            AudioBuffer pause = generate_silence(pause_ms);

            inflected.samples.insert(inflected.samples.end(),
                                    pause.samples.begin(),
                                    pause.samples.end());
        }
    }

    return inflected;
}

// =============================================================================
// Generate Silence
// =============================================================================

AudioBuffer AudioSynthesizer::generate_silence(uint32_t duration_ms) const {
    AudioBuffer silence;
    silence.sample_rate = SAMPLE_RATE;
    silence.bits_per_sample = BITS_PER_SAMPLE;
    silence.channels = NUM_CHANNELS;

    // Calculate number of samples
    size_t num_samples = (static_cast<size_t>(SAMPLE_RATE) * duration_ms) / 1000;

    silence.samples.resize(num_samples, 0);

    return silence;
}

// =============================================================================
// Voice Parameters
// =============================================================================

void AudioSynthesizer::set_voice_params(const VoiceParams& params) {
    m_voice_params = params;
    m_voice_params.clamp();  // Use VoiceParams::clamp() for consistency

    // Propagate pause settings to inflection processor
    m_inflection.set_pause_settings(m_voice_params.pause_settings);
}

// =============================================================================
// Streaming Support
// =============================================================================

void AudioSynthesizer::set_stream_callback(
    std::function<void(const AudioBuffer&)> callback,
    uint32_t chunk_size_ms) {

    m_stream_callback = std::move(callback);
    m_stream_chunk_samples = (SAMPLE_RATE * chunk_size_ms) / 1000;
}

void AudioSynthesizer::clear_stream_callback() {
    m_stream_callback = nullptr;
    m_stream_chunk_samples = 0;
}

void AudioSynthesizer::emit_chunk(const AudioBuffer& chunk) {
    if (m_stream_callback && !chunk.empty()) {
        m_stream_callback(chunk);
    }
}

void AudioSynthesizer::flush_stream() {
    // Nothing to do - chunks are emitted as they're ready
}

// =============================================================================
// Phoneme Truncation
// =============================================================================

bool AudioSynthesizer::should_truncate(Phoneme phoneme) {
    switch (phoneme) {
        case Phoneme::L:
        case Phoneme::M:
        case Phoneme::N:
        case Phoneme::S:
        case Phoneme::SH:
        case Phoneme::V:
        case Phoneme::Z:
        case Phoneme::ZH:
            return true;
        default:
            return false;
    }
}

uint32_t AudioSynthesizer::get_truncation_limit(Phoneme phoneme) {
    if (should_truncate(phoneme)) {
        return TRUNCATION_BYTES;
    }
    return 0;  // No limit
}

// =============================================================================
// Get Phoneme Audio
// =============================================================================

AudioBuffer AudioSynthesizer::get_phoneme_audio(Phoneme phoneme) const {
    AudioBuffer result;
    result.sample_rate = SAMPLE_RATE;
    result.bits_per_sample = BITS_PER_SAMPLE;
    result.channels = NUM_CHANNELS;

    // Handle silence specially
    if (phoneme == Phoneme::SILENCE) {
        // Generate a short silence (50ms)
        return generate_silence(50);
    }

    // Get truncation limit
    uint32_t max_bytes = get_truncation_limit(phoneme);

    // Get phoneme samples
    span<const AudioSample> samples;
    if (max_bytes > 0) {
        samples = m_phoneme_data.get_phoneme_truncated(phoneme, max_bytes);
    } else {
        samples = m_phoneme_data.get_phoneme(phoneme);
    }

    if (samples.empty()) {
        return result;
    }

    // Copy to result buffer
    result.samples.assign(samples.begin(), samples.end());

    return result;
}

// =============================================================================
// Crossfade Blending
// =============================================================================

void AudioSynthesizer::apply_crossfade(
    AudioBuffer& dest,
    const AudioBuffer& src,
    size_t overlap_samples) const {

    if (dest.empty() || src.empty()) {
        dest.samples.insert(dest.samples.end(),
                          src.samples.begin(),
                          src.samples.end());
        return;
    }

    // Determine actual overlap
    size_t actual_overlap = std::min({
        overlap_samples,
        dest.samples.size(),
        src.samples.size()
    });

    if (actual_overlap == 0) {
        dest.samples.insert(dest.samples.end(),
                          src.samples.begin(),
                          src.samples.end());
        return;
    }

    // Crossfade the overlapping region
    size_t dest_start = dest.samples.size() - actual_overlap;

    for (size_t i = 0; i < actual_overlap; ++i) {
        // Linear crossfade
        float t = static_cast<float>(i) / static_cast<float>(actual_overlap);

        float dest_sample = static_cast<float>(dest.samples[dest_start + i]);
        float src_sample = static_cast<float>(src.samples[i]);

        // Blend: fade out dest, fade in src
        float blended = dest_sample * (1.0f - t) + src_sample * t;

        // Clamp and store
        blended = std::clamp(blended, -32768.0f, 32767.0f);
        dest.samples[dest_start + i] = static_cast<AudioSample>(std::round(blended));
    }

    // Append remaining source samples (after overlap)
    if (src.samples.size() > actual_overlap) {
        dest.samples.insert(dest.samples.end(),
                          src.samples.begin() + actual_overlap,
                          src.samples.end());
    }
}

// =============================================================================
// Apply Volume
// =============================================================================

AudioBuffer AudioSynthesizer::apply_volume(
    const AudioBuffer& samples,
    float volume) const {

    AudioBuffer result = samples;

    for (auto& sample : result.samples) {
        float adjusted = static_cast<float>(sample) * volume;
        adjusted = std::clamp(adjusted, -32768.0f, 32767.0f);
        sample = static_cast<AudioSample>(std::round(adjusted));
    }

    return result;
}

// =============================================================================
// Apply Rate (Sonic Time-Stretching)
// =============================================================================

AudioBuffer AudioSynthesizer::apply_rate(
    const AudioBuffer& samples,
    float rate) const {

    if (samples.empty() || std::abs(rate - 1.0f) < 0.01f) {
        return samples;
    }

    // Use Sonic for time-stretching: changes speed WITHOUT changing pitch
    // Rate > 1.0 = faster speech, same pitch
    // Rate < 1.0 = slower speech, same pitch
    return sonic::change_speed(samples, rate);
}

// =============================================================================
// Apply Pitch (Sonic Pitch-Shifting)
// =============================================================================

AudioBuffer AudioSynthesizer::apply_pitch(
    const AudioBuffer& samples,
    float pitch) const {

    if (samples.empty() || std::abs(pitch - 1.0f) < 0.01f) {
        return samples;
    }

    // Use Sonic for pitch-shifting: changes pitch WITHOUT changing duration
    // Higher pitch (>1.0) = higher frequency, same duration
    // Lower pitch (<1.0) = lower frequency, same duration
    // Note: This also shifts formants (chipmunk effect) - used for voice character
    return sonic::change_pitch(samples, pitch);
}

// =============================================================================
// Apply User Pitch (Formant-Preserving)
// =============================================================================

AudioBuffer AudioSynthesizer::apply_user_pitch(
    const AudioBuffer& samples,
    float pitch) const {

    if (samples.empty() || std::abs(pitch - 1.0f) < 0.01f) {
        return samples;
    }

    // Use formant-preserving pitch shifting for user preference
    // This changes pitch WITHOUT shifting formants (no chipmunk effect)
    // Voice character stays the same, just the pitch changes
    return formant::change_pitch_preserve_formants(samples, pitch);
}

} // namespace laprdus
