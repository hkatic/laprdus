// -*- coding: utf-8 -*-
// tts_engine.hpp - Main TTS engine orchestrator
// Coordinates text processing, phoneme mapping, and audio synthesis

#ifndef LAPRDUS_TTS_ENGINE_HPP
#define LAPRDUS_TTS_ENGINE_HPP

#include "laprdus/types.hpp"
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
 * 3. Text front end (letter-to-sound, stress, clitics; src/formant/)
 * 4. Prosody (durations, pitch contour)
 * 5. Waveform: TD-PSOLA over the recordings (recorded voices) or the
 *    Klatt synthesizer (formant voices)
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
     * Initialize engine with a formant voice. No phoneme data is needed:
     * speech is produced by rule (see src/formant/).
     * @param voice_id Formant voice ID ("zvonko", "stojan", "mirsad") or a
     *        singing preset ("orguljas", "klapa", "trubac", "harmonikas",
     *        "sevdalija", "sazlija", "pjevac", "pevac", "solist", "becarac").
     * @return true on success, false if the ID is not a formant voice.
     */
    bool initialize_formant(const char* voice_id);

    /**
     * Set the language of a recorded voice (Josip: Croatian, Vlado:
     * Serbian): chooses the text front end's rules and lexicon. Formant
     * voices carry their own language and ignore this.
     * @param language Voice language.
     */
    void set_language(VoiceLanguage language);

    /**
     * Check if the engine currently speaks with a formant voice.
     * @return true for formant synthesis, false for concatenative.
     */
    bool is_formant() const;

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
     * @return Version string (e.g., "2.0.0").
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
     * Load pronunciation dictionary from file (replaces existing entries).
     * @param path Path to dictionary JSON file.
     * @return true on success.
     */
    bool load_dictionary(const std::string& path);

    /**
     * Load pronunciation dictionary from memory (replaces existing entries).
     * @param json_content JSON content.
     * @param length Length of content (0 for null-terminated).
     * @return true on success.
     */
    bool load_dictionary_from_memory(const char* json_content, size_t length = 0);

    /**
     * Append pronunciation dictionary entries from file (keeps existing entries).
     * @param path Path to dictionary JSON file.
     * @return true on success.
     */
    bool append_dictionary(const std::string& path);

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
     * Add a single spelling entry of the user's (replaces an existing one for
     * the character). The user's entries win over the bundled dictionary and
     * over the built-in letter names; see synthesize_spelled() for what they
     * do when letters are spelled by their sounds.
     * @param character Character to match.
     * @param pronunciation How the character is named when spelling.
     */
    void add_spelling_entry(const std::string& character, const std::string& pronunciation);

    /**
     * Add a single emoji entry (replaces an existing one for the emoji).
     * @param emoji UTF-8 emoji.
     * @param text Spoken text.
     */
    void add_emoji_entry(const std::string& emoji, const std::string& text);

    /**
     * Clear the pronunciation dictionary.
     */
    void clear_dictionary();

    // =========================================================================
    // Spelling Dictionary (for character-by-character pronunciation)
    // =========================================================================

    /**
     * Load the bundled spelling dictionary from file (replaces all existing
     * entries, the user's too). It names digits, punctuation and symbols;
     * the letters of the alphabet are named by the engine itself in the
     * language of the voice (formant::letter_name).
     * @param path Path to spelling dictionary JSON file.
     * @return true on success.
     */
    bool load_spelling_dictionary(const std::string& path);

    /**
     * Load the bundled spelling dictionary from memory (replaces all existing
     * entries, the user's too).
     * @param json_content JSON content.
     * @param length Length of content (0 for null-terminated).
     * @return true on success.
     */
    bool load_spelling_dictionary_from_memory(const char* json_content, size_t length = 0);

    /**
     * Append the user's spelling dictionary from file (keeps existing
     * entries). The user's entries win over the bundled ones.
     * @param path Path to spelling dictionary JSON file.
     * @return true on success.
     */
    bool append_spelling_dictionary(const std::string& path);

    /**
     * Clear the spelling dictionary (bundled and user entries).
     */
    void clear_spelling_dictionary();

    /**
     * How letters are spelled: by their names ("be", "ce") or by their
     * sounds ([b], [ts]). Also VoiceParams::spelling_mode.
     */
    void set_spelling_mode(SpellingMode mode);
    SpellingMode spelling_mode() const;

    /**
     * Rate of spelled characters as a percentage (0-100, default 50): 100 is
     * the speech rate itself, 50 a little over half of it, 0 a third. Also
     * VoiceParams::spelling_speed.
     */
    void set_spelling_speed(int percent);
    int spelling_speed() const;

    /**
     * Synthesize text in spelling mode (character by character), with the
     * spelling pause between the characters and at the spelling speed.
     *
     * A letter of the alphabet (Latin or Cyrillic, any case) is read by its
     * name in the language of the voice, or by its sound when the spelling
     * mode is LetterSounds. Every other character is read by its entry in the
     * spelling dictionary, or spoken as text when it has none.
     *
     * The user's spelling entries (add_spelling_entry, append_spelling_dictionary)
     * win over all of that, with one exception: when letters are spelled by
     * their sounds, a user entry that is merely a letter's name ("be", "jot",
     * "lje") is a letter name too and gives way to the sound. An entry that
     * says anything else is spoken as written in both modes.
     * @param text UTF-8 text to spell.
     * @return Synthesis result with audio buffer.
     */
    SynthesisResult synthesize_spelled(const std::string& text);

    // =========================================================================
    // Emoji Dictionary
    // =========================================================================

    /**
     * Load emoji dictionary from file (replaces existing entries).
     * @param path Path to emoji dictionary JSON file.
     * @return true on success.
     */
    bool load_emoji_dictionary(const std::string& path);

    /**
     * Load emoji dictionary from memory (replaces existing entries).
     * @param json_content JSON content.
     * @param length Length of content (0 for null-terminated).
     * @return true on success.
     */
    bool load_emoji_dictionary_from_memory(const char* json_content, size_t length = 0);

    /**
     * Append emoji dictionary entries from file (keeps existing entries).
     * @param path Path to emoji dictionary JSON file.
     * @return true on success.
     */
    bool append_emoji_dictionary(const std::string& path);

    /**
     * Clear the emoji dictionary.
     */
    void clear_emoji_dictionary();

    // =========================================================================
    // Accent Lexicon (user accent entries for the formant voices)
    // =========================================================================

    /**
     * Load the user's accent lexicon from a JSON file (replaces the current
     * one). Entries use the notation of the built-in lexicon; see
     * formant::UserLexicon. Only the formant voices use it; the concatenative
     * voices ignore it. The file is parsed once, and each bad entry is skipped
     * and reported through accent_lexicon_report().
     * @param path Path to the accent lexicon JSON file.
     * @return true if at least one entry was accepted.
     */
    bool load_accent_lexicon(const std::string& path);

    /**
     * Load the user's accent lexicon from memory (replaces the current one).
     * @param json_content JSON content.
     * @param length Length of content (0 for null-terminated).
     * @return true if at least one entry was accepted.
     */
    bool load_accent_lexicon_from_memory(const char* json_content, size_t length = 0);

    /** Remove the user's accent lexicon. */
    void clear_accent_lexicon();

    /**
     * What the last load accepted and rejected: "N words, M verbs" and, if
     * anything was rejected, "; K rejected (first: entry: reason)".
     */
    std::string accent_lexicon_report() const;

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
    // Start of a synthesis call: a singing preset begins its song again.
    void begin_utterance();
    // Language of the current voice (letter names, symbol names).
    VoiceLanguage current_language() const;
    // One character of synthesize_spelled(): its name, its sound or its entry.
    SynthesisResult spell_character(const std::string& character);
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    // Internal synthesis steps
    std::string preprocess_text(const std::string& text);
    std::vector<TextSegment> segment_text(const std::string& processed_text);
    AudioBuffer synthesize_segments(const std::vector<TextSegment>& segments);
};

} // namespace laprdus

#endif // LAPRDUS_TTS_ENGINE_HPP
