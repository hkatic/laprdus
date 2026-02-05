// -*- coding: utf-8 -*-
// phoneme_data.cpp - Phoneme audio data loader implementation

#include "phoneme_data.hpp"
#include "../core/phoneme_mapper.hpp"
#include <fstream>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#define PATH_SEPARATOR '\\'
#else
#include <dirent.h>
#include <sys/stat.h>
#define PATH_SEPARATOR '/'
#endif

namespace laprdus {

// =============================================================================
// Constructor / Destructor
// =============================================================================

PhonemeData::PhonemeData() = default;

PhonemeData::~PhonemeData() = default;

PhonemeData::PhonemeData(PhonemeData&&) noexcept = default;

PhonemeData& PhonemeData::operator=(PhonemeData&&) noexcept = default;

// =============================================================================
// Load from File
// =============================================================================

bool PhonemeData::load_from_file(const std::string& path,
                                  span<const uint8_t> key) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return false;
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    file.seekg(0);

    std::vector<uint8_t> data(file_size);
    file.read(reinterpret_cast<char*>(data.data()), file_size);

    if (!file) {
        return false;
    }

    return load_from_memory(data.data(), data.size(), key);
}

// =============================================================================
// Load from Memory
// =============================================================================

bool PhonemeData::load_from_memory(const uint8_t* data, size_t size,
                                    span<const uint8_t> key) {
    clear();
    return parse_packed_data(data, size, key);
}

// =============================================================================
// Parse Packed Data
// =============================================================================

bool PhonemeData::parse_packed_data(const uint8_t* data, size_t size,
                                     span<const uint8_t> key) {
    // Validate minimum size
    if (size < sizeof(PackedFileHeader)) {
        return false;
    }

    // Parse header
    const auto* header = reinterpret_cast<const PackedFileHeader*>(data);

    // Validate magic
    if (header->magic != PHONEME_FILE_MAGIC) {
        return false;
    }

    // Validate version
    if (header->version != PHONEME_FILE_VERSION) {
        return false;
    }

    // Validate size
    if (header->total_size > size) {
        return false;
    }

    // Check encryption
    bool encrypted = (header->flags & PACKED_FLAG_ENCRYPTED) != 0;
    if (encrypted && key.empty()) {
        return false;  // Need key but none provided
    }

    // Store format info
    m_sample_rate = header->sample_rate;
    m_bits_per_sample = header->bits_per_sample;
    m_channels = header->channels;

    // Get pointers to index and data sections
    const auto* index = reinterpret_cast<const PhonemeIndexEntry*>(
        data + header->index_offset);
    const uint8_t* audio_data = data + header->data_offset;
    size_t audio_size = size - header->data_offset;

    // Decrypt audio data if needed
    std::vector<uint8_t> decrypted_audio;
    if (encrypted) {
        decrypted_audio.assign(audio_data, audio_data + audio_size);
        xor_decrypt(decrypted_audio, key);
        audio_data = decrypted_audio.data();
    }

    // Load each phoneme
    for (uint32_t i = 0; i < header->phoneme_count; ++i) {
        const auto& entry = index[i];

        // Validate phoneme ID
        if (entry.phoneme_id >= static_cast<uint32_t>(Phoneme::COUNT)) {
            continue;
        }

        // Validate offset and size
        if (entry.data_offset + entry.original_size > audio_size) {
            continue;
        }

        // Get raw audio bytes
        const uint8_t* phoneme_data = audio_data + entry.data_offset;
        size_t byte_count = entry.original_size;

        // Convert bytes to samples (16-bit PCM)
        size_t sample_count = byte_count / sizeof(AudioSample);

        auto& phoneme = m_phonemes[entry.phoneme_id];
        phoneme.samples.resize(sample_count);

        // Copy and convert from little-endian
        const auto* src = reinterpret_cast<const AudioSample*>(phoneme_data);
        std::copy(src, src + sample_count, phoneme.samples.begin());

        phoneme.duration_samples = static_cast<uint32_t>(sample_count);
        phoneme.loaded = true;
    }

    m_loaded = true;
    return true;
}

// =============================================================================
// Load from Directory (Development Mode)
// =============================================================================

bool PhonemeData::load_from_directory(const std::string& dir_path) {
    clear();

    // Load each phoneme file
    for (uint8_t i = 0; i < static_cast<uint8_t>(Phoneme::COUNT); ++i) {
        Phoneme p = static_cast<Phoneme>(i);
        std::string filename = PhonemeMapper::phoneme_filename(p);

        if (filename.empty()) continue;

        std::string filepath = dir_path;
        if (!filepath.empty() && filepath.back() != PATH_SEPARATOR && filepath.back() != '/') {
            filepath += PATH_SEPARATOR;
        }
        filepath += filename;

        load_wav_file(filepath, p);
    }

    m_loaded = is_complete();
    return m_loaded;
}

// =============================================================================
// Load WAV File
// =============================================================================

bool PhonemeData::load_wav_file(const std::string& path, Phoneme phoneme) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    // Read WAV header
#pragma pack(push, 1)
    struct WavHeader {
        char riff[4];
        uint32_t file_size;
        char wave[4];
        char fmt[4];
        uint32_t fmt_size;
        uint16_t audio_format;
        uint16_t num_channels;
        uint32_t sample_rate;
        uint32_t byte_rate;
        uint16_t block_align;
        uint16_t bits_per_sample;
    };
    struct WavChunk {
        char id[4];
        uint32_t size;
    };
#pragma pack(pop)

    WavHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (std::memcmp(header.riff, "RIFF", 4) != 0 ||
        std::memcmp(header.wave, "WAVE", 4) != 0 ||
        std::memcmp(header.fmt, "fmt ", 4) != 0) {
        return false;
    }

    // Skip extra fmt data
    if (header.fmt_size > 16) {
        file.seekg(header.fmt_size - 16, std::ios::cur);
    }

    // Find data chunk
    WavChunk chunk;
    while (file.read(reinterpret_cast<char*>(&chunk), sizeof(chunk))) {
        if (std::memcmp(chunk.id, "data", 4) == 0) {
            break;
        }
        file.seekg(chunk.size, std::ios::cur);
    }

    if (std::memcmp(chunk.id, "data", 4) != 0) {
        return false;
    }

    // Read audio data
    size_t sample_count = chunk.size / sizeof(AudioSample);

    auto& entry = m_phonemes[static_cast<size_t>(phoneme)];
    entry.samples.resize(sample_count);
    file.read(reinterpret_cast<char*>(entry.samples.data()), chunk.size);

    entry.duration_samples = static_cast<uint32_t>(sample_count);
    entry.loaded = true;

    // Update format info from first file
    if (m_sample_rate == SAMPLE_RATE) {
        m_sample_rate = header.sample_rate;
        m_bits_per_sample = header.bits_per_sample;
        m_channels = header.num_channels;
    }

    return true;
}

// =============================================================================
// Get Phoneme Data
// =============================================================================

span<const AudioSample> PhonemeData::get_phoneme(Phoneme phoneme) const {
    size_t idx = static_cast<size_t>(phoneme);
    if (idx >= m_phonemes.size() || !m_phonemes[idx].loaded) {
        return {};
    }
    const auto& samples = m_phonemes[idx].samples;
    return span<const AudioSample>(samples.data(), samples.size());
}

span<const AudioSample> PhonemeData::get_phoneme_truncated(
    Phoneme phoneme, uint32_t max_bytes) const {

    auto samples = get_phoneme(phoneme);
    if (samples.empty()) {
        return {};
    }

    if (max_bytes == 0) {
        return samples;
    }

    size_t max_samples = max_bytes / sizeof(AudioSample);
    if (samples.size() > max_samples) {
        return samples.subspan(0, max_samples);
    }

    return samples;
}

// =============================================================================
// Status Functions
// =============================================================================

bool PhonemeData::is_complete() const {
    // Check if all essential phonemes are loaded
    // (not checking UNKNOWN which is never loaded)
    for (size_t i = 0; i < static_cast<size_t>(Phoneme::UNKNOWN); ++i) {
        // Skip LJ and NJ if they're not present (digraphs are optional)
        if (static_cast<Phoneme>(i) == Phoneme::LJ ||
            static_cast<Phoneme>(i) == Phoneme::NJ) {
            continue;
        }
        if (!m_phonemes[i].loaded) {
            return false;
        }
    }
    return true;
}

bool PhonemeData::is_loaded() const {
    return m_loaded;
}

size_t PhonemeData::memory_usage() const {
    size_t total = 0;
    for (const auto& entry : m_phonemes) {
        total += entry.samples.size() * sizeof(AudioSample);
    }
    return total;
}

void PhonemeData::clear() {
    for (auto& entry : m_phonemes) {
        entry.samples.clear();
        entry.duration_samples = 0;
        entry.loaded = false;
    }
    m_loaded = false;
    m_sample_rate = SAMPLE_RATE;
    m_bits_per_sample = BITS_PER_SAMPLE;
    m_channels = NUM_CHANNELS;
}

// =============================================================================
// XOR Decryption
// =============================================================================

void PhonemeData::xor_decrypt(std::vector<uint8_t>& data,
                               span<const uint8_t> key) {
    if (key.empty()) return;

    for (size_t i = 0; i < data.size(); ++i) {
        data[i] ^= key[i % key.size()];
    }
}

} // namespace laprdus
