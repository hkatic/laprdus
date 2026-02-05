// -*- coding: utf-8 -*-
// tts_engine.hpp - Main TTS engine orchestrator
// Coordinates text processing, phoneme mapping, and audio synthesis

#ifndef LAPRDUS_TTS_ENGINE_HPP
#define LAPRDUS_TTS_ENGINE_HPP

#include "laprdus/types.hpp"
#include "phoneme_mapper.hpp"
#include "croatian_numbers.hpp"
#include "inflection.hpp"
#include "pronunciation_dict.hpp"
#include "spelling_dict.hpp"
#include "emoji_dict.hpp"
#include "../audio/phoneme_data.hpp"
#include "../audio/audio_synthesizer.hpp"
#include <memory>
#include <string>
#include <functional>

namespace laprdus {

/**
 * TTSEngine - Main text-to-speech engine.
 *
 * Orchestrates the complete TTS pipeline:
 * 1. Text preprocessing (number expansion)
 * 2. Text segmentation (by punctuation)
 * 3. Phoneme mapping (character to phoneme)
 * 4. Audio synthesis (phoneme concatenation)
 * 5. Inflection application (pitch contours)
 *
 * Thread safety: Create one engine per thread,
 * or use external synchronization.
 */
class TTSEngine {
public:
    /**
     * Create TTS engine.
     * Call initialize() before use.
     */
    TTSEngine();
    ~TTSEngine();

    // Non-copyable, moveable
    TTSEngine(const TTSEngine&) = delete;
    TTSEngine& operator=(const TTSEngine&) = delete;
    TTSEngine(TTSEngine&&) noexcept;
    TTSEngine& operator=(TTSEngine&&) noexcept;

    /**
     * Initialize engine with phoneme data file.
     * @param phoneme_path Path to phonemes.bin or phoneme directory.
     * @param key Optional decryption key.
     * @return true on success.
     */
    bool initialize(const std::string& phoneme_path,
                   span<const uint8_t> key = {});

    /**
     * Initialize engine with phoneme data from memory.
     * @param data Pointer to packed phoneme data.
     * @param size Size of data.
     * @param key Optional decryption key.
     * @return true on success.
     */
    bool initialize_from_memory(const uint8_t* data, size_t size,
                               span<const uint8_t> key = {});

    /**
     * Check if engine is initialized and ready.
     * @return true if ready.
     */
    bool is_initialized() const;

    /**
     * Synthesize text to audio.
     * @param text UTF-8 text to synthesize.
     * @return Synthesis result with audio buffer.
     */
    SynthesisResult synthesize(const std::string& text);

    /**
     * Synthesize with streaming output.
     * @param text UTF-8 text to synthesize.
     * @param callback Function to receive audio chunks.
     * @param chunk_ms Approximate chunk duration in milliseconds.
     * @return Synthesis result (audio buffer may be empty if streamed).
     */
    SynthesisResult synthesize_streaming(
        const std::string& text,
        std::function<void(const AudioBuffer&)> callback,
        uint32_t chunk_ms = 100);

    /**
     * Set voice parameters.
     * @param params Voice parameters (rate, pitch, volume).
     */
    void set_voice_params(const VoiceParams& params);

    /**
     * Get current voice parameters.
     * @return Current voice parameters.
     */
    VoiceParams voice_params() const;

    /**
     * Get engine version string.
     * @return Version string (e.g., "1.0.0").
     */
    static const char* version();

    /**
     * Get sample rate of output audio.
     * @return Sample rate in Hz.
     */
    uint32_t sample_rate() const;

    /**
     * Get memory usage of loaded phoneme data.
     * @return Memory usage in bytes.
     */
    size_t memory_usage() const;

    /**
     * Load pronunciation dictionary from file.
     * @param path Path to dictionary JSON file.
     * @return true on success.
     */
    bool load_dictionary(const std::string& path);

    /**
     * Load pronunciation dictionary from memory.
     * @param json_content JSON content.
     * @param length Length of content (0 for null-terminated).
     * @return true on success.
     */
    bool load_dictionary_from_memory(const char* json_content, size_t length = 0);

    /**
     * Add a single pronunciation entry.
     * @param grapheme Written form to match.
     * @param phoneme Replacement pronunciation.
     * @param case_sensitive Whether matching is case-sensitive.
     * @param whole_word Whether to match whole words only.
     */
    void add_pronunciation(const std::string& grapheme, const std::string& phoneme,
                           bool case_sensitive = false, bool whole_word = true);

    /**
     * Clear the pronunciation dictionary.
     */
    void clear_dictionary();

    // =========================================================================
    // Spelling Dictionary (for character-by-character pronunciation)
    // =========================================================================

    /**
     * Load spelling dictionary from file.
     * @param path Path to spelling dictionary JSON file.
     * @return true on success.
     */
    bool load_spelling_dictionary(const std::string& path);

    /**
     * Load spelling dictionary from memory.
     * @param json_content JSON content.
     * @param length Length of content (0 for null-terminated).
     * @return true on success.
     */
    bool load_spelling_dictionary_from_memory(const char* json_content, size_t length = 0);

    /**
     * Clear the spelling dictionary.
     */
    void clear_spelling_dictionary();

    /**
     * Synthesize text in spelling mode (character by character).
     * Each character is converted to its pronunciation using the spelling
     * dictionary, then synthesized with a small pause between characters.
     * @param text UTF-8 text to spell.
     * @return Synthesis result with audio buffer.
     */
    SynthesisResult synthesize_spelled(const std::string& text);

    // =========================================================================
    // Emoji Dictionary
    // =========================================================================

    /**
     * Load emoji dictionary from file.
     * @param path Path to emoji dictionary JSON file.
     * @return true on success.
     */
    bool load_emoji_dictionary(const std::string& path);

    /**
     * Load emoji dictionary from memory.
     * @param json_content JSON content.
     * @param length Length of content (0 for null-terminated).
     * @return true on success.
     */
    bool load_emoji_dictionary_from_memory(const char* json_content, size_t length = 0);

    /**
     * Clear the emoji dictionary.
     */
    void clear_emoji_dictionary();

    /**
     * Enable or disable emoji processing.
     * When enabled, emojis are converted to their text representations.
     * Disabled by default on all platforms.
     * @param enabled true to enable, false to disable.
     */
    void set_emoji_enabled(bool enabled);

    /**
     * Check if emoji processing is enabled.
     * @return true if enabled.
     */
    bool is_emoji_enabled() const;

    // =========================================================================
    // Pause Settings
    // =========================================================================

    /**
     * Set pause settings for sentence, comma, and newline pauses.
     * @param settings Pause settings structure.
     */
    void set_pause_settings(const PauseSettings& settings);

    /**
     * Get current pause settings.
     * @return Current pause settings.
     */
    PauseSettings pause_settings() const;

    /**
     * Set sentence pause duration.
     * @param pause_ms Pause duration in milliseconds (0-2000, default 100).
     */
    void set_sentence_pause(uint32_t pause_ms);

    /**
     * Set comma pause duration.
     * @param pause_ms Pause duration in milliseconds (0-2000, default 100).
     */
    void set_comma_pause(uint32_t pause_ms);

    /**
     * Set newline pause duration.
     * @param pause_ms Pause duration in milliseconds (0-2000, default 100).
     */
    void set_newline_pause(uint32_t pause_ms);

    /**
     * Set spelling pause duration (pause between spelled characters).
     * @param pause_ms Pause duration in milliseconds (0-2000, default 200).
     */
    void set_spelling_pause(uint32_t pause_ms);

    /**
     * Get current spelling pause duration.
     * @return Current spelling pause in milliseconds.
     */
    uint32_t spelling_pause() const;

    // =========================================================================
    // Number Processing Mode
    // =========================================================================

    /**
     * Set number processing mode.
     * @param mode WholeNumbers (default) or DigitByDigit.
     */
    void set_number_mode(NumberMode mode);

    /**
     * Get current number processing mode.
     * @return Current number mode.
     */
    NumberMode number_mode() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    // Internal synthesis steps
    std::string preprocess_text(const std::string& text);
    std::vector<TextSegment> segment_text(const std::string& processed_text);
    AudioBuffer synthesize_segments(const std::vector<TextSegment>& segments);
};

} // namespace laprdus

#endif // LAPRDUS_TTS_ENGINE_HPP
