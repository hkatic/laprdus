// -*- coding: utf-8 -*-
// inflection.hpp - Voice inflection for punctuation
// Applies pitch contours based on sentence punctuation

#ifndef LAPRDUS_INFLECTION_HPP
#define LAPRDUS_INFLECTION_HPP

#include "laprdus/types.hpp"
#include <vector>
#include <string>
#include <string_view>

namespace laprdus {

/**
 * InflectionProcessor - Applies pitch contours to audio.
 *
 * Croatian intonation patterns:
 * - Period (.): Falling intonation (-25% over last 4 phonemes)
 * - Question (?): Rising intonation (+35% over last 5 phonemes)
 * - Comma (,): Slight rise (+12% over last 2 phonemes)
 * - Exclamation (!): Emphatic (+15-25% overall)
 *
 * Uses simple pitch shifting via sample resampling.
 */
class InflectionProcessor {
public:
    InflectionProcessor();
    ~InflectionProcessor() = default;

    /**
     * Set pause settings for sentences, commas, and newlines.
     * @param settings Pause settings structure.
     */
    void set_pause_settings(const PauseSettings& settings);

    /**
     * Get current pause settings.
     * @return Current pause settings.
     */
    PauseSettings pause_settings() const;

    /**
     * Analyze text and detect inflection points.
     * @param text UTF-8 text to analyze.
     * @return Vector of text segments with inflection markers.
     */
    std::vector<TextSegment> analyze_text(const std::string& text);

    /**
     * Apply inflection to audio samples.
     * @param samples Input audio samples.
     * @param inflection Type of inflection to apply.
     * @param phoneme_count Number of phonemes in this segment.
     * @return Processed audio with pitch contour applied.
     */
    AudioBuffer apply_inflection(const AudioBuffer& samples,
                                 InflectionType inflection,
                                 size_t phoneme_count);

    /**
     * Apply pitch shift to audio samples.
     * Simple resampling-based pitch shift.
     * @param samples Input samples.
     * @param pitch_factor Pitch multiplier (1.0 = unchanged).
     * @return Pitch-shifted samples.
     */
    static AudioBuffer pitch_shift(const AudioBuffer& samples, float pitch_factor);

    /**
     * Generate a smooth pitch envelope.
     * @param num_samples Number of samples.
     * @param params Inflection parameters.
     * @return Vector of pitch factors for each sample.
     */
    static std::vector<float> generate_pitch_envelope(
        size_t num_samples,
        const InflectionParams& params);

    /**
     * Apply pitch envelope to audio.
     * @param samples Input samples.
     * @param envelope Pitch factor for each sample.
     * @return Processed audio.
     */
    static AudioBuffer apply_pitch_envelope(
        const AudioBuffer& samples,
        const std::vector<float>& envelope);

    /**
     * Get pause duration for punctuation type.
     * Uses custom pause settings if set.
     * @param punct Punctuation type.
     * @return Pause duration in milliseconds.
     */
    uint32_t get_pause_duration(Punctuation punct) const;

    /**
     * Get default pause duration for punctuation type (static version).
     * @param punct Punctuation type.
     * @return Default pause duration in milliseconds.
     */
    static uint32_t get_default_pause_duration(Punctuation punct);

    /**
     * Detect inflection type from punctuation.
     * @param punct Punctuation type.
     * @return Corresponding inflection type.
     */
    static InflectionType punct_to_inflection(Punctuation punct);

private:
    PauseSettings m_pause_settings;

    // Linear interpolation between pitch values
    static float lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    // Smooth step for gradual transitions
    static float smoothstep(float t) {
        return t * t * (3.0f - 2.0f * t);
    }
};

} // namespace laprdus

#endif // LAPRDUS_INFLECTION_HPP
