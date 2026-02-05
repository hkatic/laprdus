// -*- coding: utf-8 -*-
// audio_synthesizer.hpp - Phoneme concatenation and audio synthesis
// Combines phoneme samples into continuous speech audio

#ifndef LAPRDUS_AUDIO_SYNTHESIZER_HPP
#define LAPRDUS_AUDIO_SYNTHESIZER_HPP

#include "laprdus/types.hpp"
#include "phoneme_data.hpp"
#include "../core/inflection.hpp"
#include <vector>
#include <string>
#include <functional>

namespace laprdus {

/**
 * AudioSynthesizer - Concatenates phoneme audio samples.
 *
 * Takes a sequence of phoneme tokens and produces continuous
 * audio output with appropriate smoothing between phonemes.
 *
 * Features:
 * - Phoneme truncation for long consonants
 * - Crossfade blending between phonemes
 * - Silence insertion for pauses
 * - Inflection application per segment
 */
class AudioSynthesizer {
public:
    /**
     * Create synthesizer with phoneme data.
     * @param phoneme_data Reference to loaded phoneme audio data.
     */
    explicit AudioSynthesizer(const PhonemeData& phoneme_data);
    ~AudioSynthesizer() = default;

    // Non-copyable
    AudioSynthesizer(const AudioSynthesizer&) = delete;
    AudioSynthesizer& operator=(const AudioSynthesizer&) = delete;

    /**
     * Synthesize audio from phoneme tokens.
     * @param tokens Sequence of phoneme tokens with timing info.
     * @return Combined audio buffer.
     */
    AudioBuffer synthesize(const std::vector<PhonemeToken>& tokens);

    /**
     * Synthesize a single text segment with inflection.
     * @param segment Text segment with inflection markers.
     * @param tokens Phonemes for this segment.
     * @return Processed audio with inflection applied.
     */
    AudioBuffer synthesize_segment(const TextSegment& segment,
                                   const std::vector<PhonemeToken>& tokens);

    /**
     * Generate silence of specified duration.
     * @param duration_ms Duration in milliseconds.
     * @return Audio buffer containing silence.
     */
    AudioBuffer generate_silence(uint32_t duration_ms) const;

    /**
     * Set voice parameters.
     * @param params Voice parameters (rate, pitch, volume).
     */
    void set_voice_params(const VoiceParams& params);

    /**
     * Get current voice parameters.
     * @return Current voice parameters.
     */
    const VoiceParams& voice_params() const { return m_voice_params; }

    /**
     * Set streaming callback for real-time output.
     * @param callback Function to receive audio chunks.
     * @param chunk_size_ms Approximate chunk duration.
     */
    void set_stream_callback(std::function<void(const AudioBuffer&)> callback,
                            uint32_t chunk_size_ms = 100);

    /**
     * Clear streaming callback.
     */
    void clear_stream_callback();

private:
    const PhonemeData& m_phoneme_data;
    VoiceParams m_voice_params{};
    InflectionProcessor m_inflection;

    std::function<void(const AudioBuffer&)> m_stream_callback;
    uint32_t m_stream_chunk_samples = 0;

    // Phonemes that should be truncated (long consonants)
    static constexpr uint32_t TRUNCATION_BYTES = 2000;
    static bool should_truncate(Phoneme phoneme);
    static uint32_t get_truncation_limit(Phoneme phoneme);

    // Audio processing helpers
    AudioBuffer get_phoneme_audio(Phoneme phoneme) const;
    void apply_crossfade(AudioBuffer& dest, const AudioBuffer& src,
                        size_t overlap_samples) const;
    AudioBuffer apply_volume(const AudioBuffer& samples, float volume) const;
    AudioBuffer apply_rate(const AudioBuffer& samples, float rate) const;
    AudioBuffer apply_pitch(const AudioBuffer& samples, float pitch) const;
    AudioBuffer apply_user_pitch(const AudioBuffer& samples, float pitch) const;

    // Streaming support
    void emit_chunk(const AudioBuffer& chunk);
    void flush_stream();
};

} // namespace laprdus

#endif // LAPRDUS_AUDIO_SYNTHESIZER_HPP
