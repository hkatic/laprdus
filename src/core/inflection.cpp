// -*- coding: utf-8 -*-
// inflection.cpp - Voice inflection implementation

#include "inflection.hpp"
#include "phoneme_mapper.hpp"
#include "../audio/sonic_processor.hpp"
#include "../audio/formant_pitch.hpp"
#include <cmath>
#include <algorithm>

namespace laprdus {

// =============================================================================
// Constructor
// =============================================================================

InflectionProcessor::InflectionProcessor() = default;

// =============================================================================
// Pause Settings
// =============================================================================

void InflectionProcessor::set_pause_settings(const PauseSettings& settings) {
    m_pause_settings = settings;
    m_pause_settings.clamp();
}

PauseSettings InflectionProcessor::pause_settings() const {
    return m_pause_settings;
}

// =============================================================================
// Analyze Text for Inflection Points
// =============================================================================

std::vector<TextSegment> InflectionProcessor::analyze_text(const std::string& text) {
    std::vector<TextSegment> segments;

    // Convert to UTF-32 for proper character handling
    std::u32string utf32 = PhonemeMapper::utf8_to_utf32(text);

    if (utf32.empty()) {
        return segments;
    }

    TextSegment current;
    size_t segment_start = 0;

    for (size_t i = 0; i < utf32.size(); ++i) {
        char32_t ch = utf32[i];
        Punctuation punct = PhonemeMapper::detect_punctuation(ch);

        if (punct != Punctuation::NONE) {
            // End current segment at this punctuation
            if (i > segment_start) {
                current.text = utf32.substr(segment_start, i - segment_start);
            }

            current.trailing_punct = punct;
            current.inflection = punct_to_inflection(punct);

            // Check if this ends a sentence
            current.is_end_of_sentence = (punct == Punctuation::PERIOD ||
                                           punct == Punctuation::QUESTION ||
                                           punct == Punctuation::EXCLAMATION);

            if (!current.text.empty()) {
                segments.push_back(std::move(current));
            }

            // Start new segment
            current = TextSegment{};
            segment_start = i + 1;
        }
    }

    // Handle remaining text (no trailing punctuation)
    if (segment_start < utf32.size()) {
        current.text = utf32.substr(segment_start);
        current.trailing_punct = Punctuation::NONE;
        current.inflection = InflectionType::NEUTRAL;
        current.is_end_of_sentence = false;

        if (!current.text.empty()) {
            segments.push_back(std::move(current));
        }
    }

    return segments;
}

// =============================================================================
// Apply Inflection to Audio
// =============================================================================

AudioBuffer InflectionProcessor::apply_inflection(
    const AudioBuffer& samples,
    InflectionType inflection,
    size_t phoneme_count) {

    (void)phoneme_count;  // Unused in simplified implementation

    if (samples.empty() || inflection == InflectionType::NEUTRAL) {
        return samples;  // No modification needed
    }

    InflectionParams params = get_inflection_params(inflection);

    // Calculate the last portion to affect (last ~30% of audio for the final word)
    size_t total_samples = samples.samples.size();
    size_t scope_samples = static_cast<size_t>(total_samples * 0.3f);
    scope_samples = std::max(scope_samples, size_t(1024));  // At least ~46ms
    scope_samples = std::min(scope_samples, total_samples);

    size_t split_point = total_samples - scope_samples;

    // Copy the unchanged first portion
    AudioBuffer result;
    result.sample_rate = samples.sample_rate;
    result.bits_per_sample = samples.bits_per_sample;
    result.channels = samples.channels;
    result.samples.assign(samples.samples.begin(), samples.samples.begin() + split_point);

    // Extract the last portion to pitch-shift
    AudioBuffer end_portion;
    end_portion.sample_rate = samples.sample_rate;
    end_portion.bits_per_sample = samples.bits_per_sample;
    end_portion.channels = samples.channels;
    end_portion.samples.assign(samples.samples.begin() + split_point, samples.samples.end());

    AudioBuffer shifted_end;

    if (params.has_peak) {
        // Two-stage pattern (exclamation): rise to peak, then fall to end
        // Split the end portion into two halves
        size_t half_point = end_portion.samples.size() / 2;

        // First half: pitch up to peak
        AudioBuffer first_half;
        first_half.sample_rate = end_portion.sample_rate;
        first_half.bits_per_sample = end_portion.bits_per_sample;
        first_half.channels = end_portion.channels;
        first_half.samples.assign(end_portion.samples.begin(),
                                  end_portion.samples.begin() + half_point);

        // Second half: pitch down to end
        AudioBuffer second_half;
        second_half.sample_rate = end_portion.sample_rate;
        second_half.bits_per_sample = end_portion.bits_per_sample;
        second_half.channels = end_portion.channels;
        second_half.samples.assign(end_portion.samples.begin() + half_point,
                                   end_portion.samples.end());

        // Apply formant-preserving pitch shifts
        AudioBuffer shifted_first = formant::change_pitch_preserve_formants(first_half, params.pitch_peak);
        AudioBuffer shifted_second = formant::change_pitch_preserve_formants(second_half, params.pitch_end);

        // Safety: if pitch shifting failed, use originals
        if (shifted_first.samples.empty()) shifted_first = first_half;
        if (shifted_second.samples.empty()) shifted_second = second_half;

        // Combine with crossfade
        shifted_end.sample_rate = end_portion.sample_rate;
        shifted_end.bits_per_sample = end_portion.bits_per_sample;
        shifted_end.channels = end_portion.channels;
        shifted_end.samples = std::move(shifted_first.samples);

        // Crossfade between peak and end portions
        constexpr size_t MID_CROSSFADE = 128;
        size_t mid_cf = std::min({MID_CROSSFADE, shifted_end.samples.size(), shifted_second.samples.size()});

        // Only apply crossfade if we have samples to blend
        if (mid_cf > 0) {
            for (size_t i = 0; i < mid_cf; ++i) {
                float t = static_cast<float>(i) / static_cast<float>(mid_cf);
                size_t idx = shifted_end.samples.size() - mid_cf + i;
                float blended = shifted_end.samples[idx] * (1.0f - t) + shifted_second.samples[i] * t;
                shifted_end.samples[idx] = static_cast<AudioSample>(std::clamp(blended, -32768.0f, 32767.0f));
            }
        }

        // Append rest of second portion
        if (shifted_second.samples.size() > mid_cf) {
            shifted_end.samples.insert(shifted_end.samples.end(),
                                       shifted_second.samples.begin() + mid_cf,
                                       shifted_second.samples.end());
        }
    } else {
        // Simple pattern: just shift to target pitch using formant-preserving algorithm
        shifted_end = formant::change_pitch_preserve_formants(end_portion, params.pitch_end);

        // Safety: if pitch shifting failed and returned empty, use original
        if (shifted_end.samples.empty()) {
            shifted_end = end_portion;
        }
    }

    // Crossfade between unchanged portion and shifted portion
    if (!result.samples.empty() && !shifted_end.samples.empty()) {
        constexpr size_t CROSSFADE_LEN = 256;  // ~12ms crossfade
        size_t actual_crossfade = std::min({CROSSFADE_LEN, result.samples.size(), shifted_end.samples.size()});

        // Only apply crossfade if we have samples to blend
        if (actual_crossfade > 0) {
            for (size_t i = 0; i < actual_crossfade; ++i) {
                float t = static_cast<float>(i) / static_cast<float>(actual_crossfade);
                size_t result_idx = result.samples.size() - actual_crossfade + i;
                float blended = result.samples[result_idx] * (1.0f - t) + shifted_end.samples[i] * t;
                result.samples[result_idx] = static_cast<AudioSample>(std::clamp(blended, -32768.0f, 32767.0f));
            }
        }

        // Append the rest of the shifted portion (after crossfade)
        if (shifted_end.samples.size() > actual_crossfade) {
            result.samples.insert(result.samples.end(),
                                 shifted_end.samples.begin() + actual_crossfade,
                                 shifted_end.samples.end());
        }
    } else {
        result.samples.insert(result.samples.end(),
                             shifted_end.samples.begin(),
                             shifted_end.samples.end());
    }

    // Apply emphasis (volume adjustment) if specified
    if (std::abs(params.emphasis - 1.0f) > 0.01f) {
        for (auto& sample : result.samples) {
            float adjusted = static_cast<float>(sample) * params.emphasis;
            adjusted = std::clamp(adjusted, -32768.0f, 32767.0f);
            sample = static_cast<AudioSample>(adjusted);
        }
    }

    return result;
}

// =============================================================================
// Pitch Shift via Sonic Library
// =============================================================================

AudioBuffer InflectionProcessor::pitch_shift(const AudioBuffer& samples, float pitch_factor) {
    if (samples.empty() || std::abs(pitch_factor - 1.0f) < 0.01f) {
        return samples;
    }

    // Use Sonic for pitch-shifting: changes pitch WITHOUT changing duration
    // This is the key improvement over simple resampling which coupled pitch and tempo
    return sonic::change_pitch(samples, pitch_factor);
}

// =============================================================================
// Generate Pitch Envelope
// =============================================================================

std::vector<float> InflectionProcessor::generate_pitch_envelope(
    size_t num_samples,
    const InflectionParams& params) {

    std::vector<float> envelope(num_samples, 1.0f);

    if (num_samples == 0 || params.scope_phonemes == 0) {
        return envelope;
    }

    // Calculate the affected region (last N% of samples)
    // scope_phonemes is a hint, but we use a percentage of total samples
    float scope_ratio = static_cast<float>(params.scope_phonemes) / 10.0f;
    scope_ratio = std::clamp(scope_ratio, 0.1f, 0.8f);

    size_t scope_start = static_cast<size_t>(num_samples * (1.0f - scope_ratio));

    for (size_t i = scope_start; i < num_samples; ++i) {
        // Calculate progress within the scope (0 to 1)
        float progress = static_cast<float>(i - scope_start) /
                        static_cast<float>(num_samples - scope_start);

        // Apply smoothstep for gradual transition
        float smooth_progress = smoothstep(progress);

        if (params.has_peak) {
            // Two-stage pattern: rise to peak, then fall to end
            if (smooth_progress < 0.5f) {
                // First half: start → peak
                float half_progress = smooth_progress * 2.0f;
                envelope[i] = lerp(params.pitch_start, params.pitch_peak, smoothstep(half_progress));
            } else {
                // Second half: peak → end
                float half_progress = (smooth_progress - 0.5f) * 2.0f;
                envelope[i] = lerp(params.pitch_peak, params.pitch_end, smoothstep(half_progress));
            }
        } else {
            // Simple pattern: start → end
            envelope[i] = lerp(params.pitch_start, params.pitch_end, smooth_progress);
        }
    }

    return envelope;
}

// =============================================================================
// Apply Pitch Envelope to Audio
// =============================================================================

AudioBuffer InflectionProcessor::apply_pitch_envelope(
    const AudioBuffer& samples,
    const std::vector<float>& envelope) {

    if (samples.empty() || envelope.empty()) {
        return samples;
    }

    // Use Sonic-based pitch envelope application for proper time-domain processing
    // This processes audio in chunks with varying pitch factors while preserving duration
    return sonic::apply_pitch_envelope(samples, envelope);
}

// =============================================================================
// Get Pause Duration for Punctuation (instance method with custom settings)
// =============================================================================

uint32_t InflectionProcessor::get_pause_duration(Punctuation punct) const {
    switch (punct) {
        case Punctuation::COMMA:
            return m_pause_settings.comma_pause_ms;
        case Punctuation::PERIOD:
        case Punctuation::QUESTION:
        case Punctuation::EXCLAMATION:
            return m_pause_settings.sentence_pause_ms;
        case Punctuation::SEMICOLON:
        case Punctuation::COLON:
            return m_pause_settings.comma_pause_ms;
        case Punctuation::ELLIPSIS:
            return m_pause_settings.sentence_pause_ms;
        case Punctuation::NEWLINE:
            return m_pause_settings.newline_pause_ms;
        default:
            return 0;
    }
}

// =============================================================================
// Get Default Pause Duration for Punctuation (static method)
// =============================================================================

uint32_t InflectionProcessor::get_default_pause_duration(Punctuation punct) {
    switch (punct) {
        case Punctuation::COMMA:
            return 100;  // 100ms default
        case Punctuation::PERIOD:
        case Punctuation::QUESTION:
        case Punctuation::EXCLAMATION:
            return 100;  // 100ms default
        case Punctuation::SEMICOLON:
        case Punctuation::COLON:
            return 100;  // 100ms default
        case Punctuation::ELLIPSIS:
            return 100;  // 100ms default
        case Punctuation::NEWLINE:
            return 100;  // 100ms default
        default:
            return 0;
    }
}

// =============================================================================
// Convert Punctuation to Inflection Type
// =============================================================================

InflectionType InflectionProcessor::punct_to_inflection(Punctuation punct) {
    switch (punct) {
        case Punctuation::COMMA:
            return InflectionType::COMMA_CONTINUATION;
        case Punctuation::PERIOD:
            return InflectionType::PERIOD_FINALITY;
        case Punctuation::QUESTION:
            return InflectionType::QUESTION_RISING;
        case Punctuation::EXCLAMATION:
            return InflectionType::EXCLAMATION_EMPHATIC;
        case Punctuation::SEMICOLON:
            return InflectionType::COMMA_CONTINUATION;  // Similar to comma
        case Punctuation::COLON:
            return InflectionType::NEUTRAL;
        case Punctuation::ELLIPSIS:
            return InflectionType::PERIOD_FINALITY;  // Trailing off
        default:
            return InflectionType::NEUTRAL;
    }
}

} // namespace laprdus
