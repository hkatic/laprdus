// -*- coding: utf-8 -*-
// phoneme_data.hpp - Phoneme audio data loader
// Loads packed .bin file or individual WAV files

#ifndef LAPRDUS_PHONEME_DATA_HPP
#define LAPRDUS_PHONEME_DATA_HPP

#include "laprdus/types.hpp"
#include <array>
#include <memory>
#include <string>

namespace laprdus {

/**
 * PhonemeData - Manages phoneme audio data.
 *
 * Supports loading from:
 * - Packed binary file (phonemes.bin)
 * - Individual WAV files (development mode)
 * - Memory buffer (embedded resources)
 *
 * Optionally decrypts data using XOR key.
 */
class PhonemeData {
public:
    PhonemeData();
    ~PhonemeData();

    // Non-copyable, moveable
    PhonemeData(const PhonemeData&) = delete;
    PhonemeData& operator=(const PhonemeData&) = delete;
    PhonemeData(PhonemeData&&) noexcept;
    PhonemeData& operator=(PhonemeData&&) noexcept;

    /**
     * Load from packed binary file.
     * @param path Path to phonemes.bin file.
     * @param key Optional decryption key (32 bytes).
     * @return true on success.
     */
    bool load_from_file(const std::string& path,
                        span<const uint8_t> key = {});

    /**
     * Load from memory buffer (for embedded resources).
     * @param data Pointer to packed data.
     * @param size Size of data.
     * @param key Optional decryption key.
     * @return true on success.
     */
    bool load_from_memory(const uint8_t* data, size_t size,
                          span<const uint8_t> key = {});

    /**
     * Load from directory of individual WAV files.
     * @param dir_path Path to directory containing PHONEME_*.wav files.
     * @return true on success.
     */
    bool load_from_directory(const std::string& dir_path);

    /**
     * Get audio data for a phoneme.
     * @param phoneme Phoneme to get.
     * @return Span of audio samples, empty if not loaded.
     */
    span<const AudioSample> get_phoneme(Phoneme phoneme) const;

    /**
     * Get audio data for a phoneme with optional truncation.
     * @param phoneme Phoneme to get.
     * @param max_bytes Maximum bytes to return (0 = no limit).
     * @return Span of audio samples.
     */
    span<const AudioSample> get_phoneme_truncated(Phoneme phoneme,
                                                       uint32_t max_bytes) const;

    /**
     * Check if all required phonemes are loaded.
     * @return true if complete.
     */
    bool is_complete() const;

    /**
     * Check if any phoneme data is loaded.
     * @return true if loaded.
     */
    bool is_loaded() const;

    /**
     * Get total memory usage.
     * @return Bytes used.
     */
    size_t memory_usage() const;

    /**
     * Clear all loaded data.
     */
    void clear();

    /**
     * Get sample rate of loaded data.
     */
    uint32_t sample_rate() const { return m_sample_rate; }

private:
    struct PhonemeEntry {
        std::vector<AudioSample> samples;
        uint32_t duration_samples = 0;
        bool loaded = false;
    };

    std::array<PhonemeEntry, static_cast<size_t>(Phoneme::COUNT)> m_phonemes;
    uint32_t m_sample_rate = SAMPLE_RATE;
    uint16_t m_bits_per_sample = BITS_PER_SAMPLE;
    uint16_t m_channels = NUM_CHANNELS;
    bool m_loaded = false;

    // Internal loading functions
    bool parse_packed_data(const uint8_t* data, size_t size,
                           span<const uint8_t> key);
    bool load_wav_file(const std::string& path, Phoneme phoneme);

    // XOR decryption
    void xor_decrypt(std::vector<uint8_t>& data,
                     span<const uint8_t> key);
};

} // namespace laprdus

#endif // LAPRDUS_PHONEME_DATA_HPP
