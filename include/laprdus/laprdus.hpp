// -*- coding: utf-8 -*-
// laprdus.hpp - Public C++ interface for LaprdusTTS
// Croatian Text-to-Speech Engine

#ifndef LAPRDUS_HPP
#define LAPRDUS_HPP

#include "types.hpp"
#include <functional>
#include <memory>
#include <string>

namespace laprdus {

// Forward declaration
class TTSEngine;

/**
 * Laprdus - High-level C++ interface for LaprdusTTS.
 *
 * This is the recommended interface for C++ applications.
 * It provides a simple, RAII-compliant wrapper around the TTS engine.
 *
 * Usage:
 * ```cpp
 * laprdus::Laprdus tts;
 * if (tts.initialize("phonemes.bin")) {
 *     auto result = tts.speak("Dobar dan!");
 *     if (result) {
 *         // Use result.audio.samples
 *     }
 * }
 * ```
 */
class Laprdus {
public:
    /**
     * Create a new Laprdus instance.
     * Call initialize() before use.
     */
    Laprdus();

    /**
     * Destructor - automatically releases resources.
     */
    ~Laprdus();

    // Non-copyable
    Laprdus(const Laprdus&) = delete;
    Laprdus& operator=(const Laprdus&) = delete;

    // Moveable
    Laprdus(Laprdus&& other) noexcept;
    Laprdus& operator=(Laprdus&& other) noexcept;

    /**
     * Initialize from a phoneme data file or directory.
     * @param path Path to phonemes.bin or directory of WAV files.
     * @param key Optional decryption key.
     * @return true on success.
     */
    bool initialize(const std::string& path,
                   span<const uint8_t> key = {});

    /**
     * Initialize from memory buffer.
     * @param data Pointer to packed phoneme data.
     * @param size Size of data in bytes.
     * @param key Optional decryption key.
     * @return true on success.
     */
    bool initialize(const uint8_t* data, size_t size,
                   span<const uint8_t> key = {});

    /**
     * Check if engine is ready for synthesis.
     * @return true if initialized.
     */
    bool isReady() const;

    /**
     * Synthesize text to audio.
     * @param text UTF-8 text to synthesize.
     * @return Synthesis result with audio buffer.
     */
    SynthesisResult speak(const std::string& text);

    /**
     * Synthesize with streaming output.
     * @param text UTF-8 text to synthesize.
     * @param callback Function called with audio chunks.
     * @param chunkMs Approximate chunk duration.
     * @return Synthesis result (audio may be empty if fully streamed).
     */
    SynthesisResult speakStreaming(
        const std::string& text,
        std::function<void(const AudioBuffer&)> callback,
        uint32_t chunkMs = 100);

    /**
     * Set speech speed.
     * @param speed Speed factor (0.5 - 2.0).
     */
    void setSpeed(float speed);

    /**
     * Get current speech speed.
     * @return Current speed factor.
     */
    float speed() const;

    /**
     * Set base pitch.
     * @param pitch Pitch factor (0.5 - 2.0).
     */
    void setPitch(float pitch);

    /**
     * Get current pitch.
     * @return Current pitch factor.
     */
    float pitch() const;

    /**
     * Set volume.
     * @param volume Volume level (0.0 - 1.0).
     */
    void setVolume(float volume);

    /**
     * Get current volume.
     * @return Current volume level.
     */
    float volume() const;

    /**
     * Enable or disable voice inflection.
     * @param enabled true to enable inflection.
     */
    void setInflectionEnabled(bool enabled);

    /**
     * Check if inflection is enabled.
     * @return true if enabled.
     */
    bool inflectionEnabled() const;

    /**
     * Get sample rate of output audio.
     * @return Sample rate in Hz.
     */
    uint32_t sampleRate() const;

    /**
     * Get library version string.
     * @return Version string.
     */
    static const char* version();

private:
    std::unique_ptr<TTSEngine> m_engine;
};

} // namespace laprdus

#endif // LAPRDUS_HPP
