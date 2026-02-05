// -*- coding: utf-8 -*-
// types.hpp - Core type definitions for LaprdusTTS
// Croatian Text-to-Speech Engine

#ifndef LAPRDUS_TYPES_HPP
#define LAPRDUS_TYPES_HPP

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>

// Simple span implementation (works in C++17 and C++20)
// We use our own implementation to avoid MSVC STL warnings about C++20 features
namespace laprdus {
template<typename T>
class span {
public:
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using size_type = std::size_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using iterator = pointer;

    constexpr span() noexcept : data_(nullptr), size_(0) {}
    constexpr span(pointer ptr, size_type count) : data_(ptr), size_(count) {}
    template<std::size_t N>
    constexpr span(T (&arr)[N]) noexcept : data_(arr), size_(N) {}

    constexpr pointer data() const noexcept { return data_; }
    constexpr size_type size() const noexcept { return size_; }
    constexpr bool empty() const noexcept { return size_ == 0; }
    constexpr reference operator[](size_type idx) const { return data_[idx]; }
    constexpr iterator begin() const noexcept { return data_; }
    constexpr iterator end() const noexcept { return data_ + size_; }
    constexpr span subspan(size_type offset, size_type count = static_cast<size_type>(-1)) const {
        if (count == static_cast<size_type>(-1)) count = size_ - offset;
        return span(data_ + offset, count);
    }

private:
    pointer data_;
    size_type size_;
};
} // namespace laprdus

namespace laprdus {

// =============================================================================
// Audio Format Constants (matching original Python implementation)
// =============================================================================

constexpr uint32_t SAMPLE_RATE = 22050;
constexpr uint16_t BITS_PER_SAMPLE = 16;
constexpr uint16_t NUM_CHANNELS = 1;
constexpr uint16_t BYTES_PER_SAMPLE = BITS_PER_SAMPLE / 8;

// =============================================================================
// Phoneme Definitions
// =============================================================================

enum class Phoneme : uint8_t {
    // Standard Latin alphabet
    A = 0,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    // Croatian special characters
    CH,     // č (U+010D)
    TJ,     // ć (U+0107)
    DJ,     // đ (U+0111) and dž (U+01C6)
    SH,     // š (U+0161)
    ZH,     // ž (U+017E)
    LJ,     // lj digraph
    NJ,     // nj digraph

    // Special
    SILENCE,    // Pause (-.wav)
    UNKNOWN,    // Unrecognized character

    COUNT       // Total count for array sizing
};

// Phoneme truncation limits (in bytes) - matching Python implementation
// These phonemes are capped at 2000 bytes to prevent overly long sounds
constexpr uint32_t TRUNCATED_PHONEME_BYTES = 2000;

constexpr bool is_truncated_phoneme(Phoneme p) {
    return p == Phoneme::L || p == Phoneme::M || p == Phoneme::N ||
           p == Phoneme::S || p == Phoneme::SH || p == Phoneme::V ||
           p == Phoneme::Z || p == Phoneme::ZH;
}

// =============================================================================
// Punctuation and Inflection Types
// =============================================================================

enum class Punctuation : uint8_t {
    NONE = 0,
    COMMA,          // Slight rise, short pause
    PERIOD,         // Falling intonation, longer pause
    QUESTION,       // Rising intonation at end
    EXCLAMATION,    // Emphatic, higher pitch
    SEMICOLON,      // Stronger than comma
    COLON,          // Neutral pause
    ELLIPSIS,       // Trailing off
    NEWLINE         // Line break pause
};

enum class InflectionType : uint8_t {
    NEUTRAL = 0,
    COMMA_CONTINUATION,
    PERIOD_FINALITY,
    QUESTION_RISING,
    EXCLAMATION_EMPHATIC
};

// Inflection parameters
struct InflectionParams {
    float pitch_start = 1.0f;       // Starting pitch factor
    float pitch_peak = 1.0f;        // Peak pitch (for rise-fall patterns)
    float pitch_end = 1.0f;         // Ending pitch factor
    uint32_t scope_phonemes = 0;    // Number of phonemes affected
    uint32_t pause_ms = 0;          // Pause duration after punctuation
    float emphasis = 1.0f;          // Volume multiplier for emphasis
    bool has_peak = false;          // True for two-stage (rise-fall) patterns
};

// Default inflection configurations
inline InflectionParams get_inflection_params(InflectionType type) {
    switch (type) {
        case InflectionType::COMMA_CONTINUATION:
            // +5% rise: 1.0 → 1.05
            return {1.0f, 1.0f, 1.05f, 2, 40, 1.0f, false};
        case InflectionType::PERIOD_FINALITY:
            // -10% fall: 1.0 → 0.90
            return {1.0f, 1.0f, 0.90f, 4, 80, 1.0f, false};
        case InflectionType::QUESTION_RISING:
            // +10% rise: 1.0 → 1.10
            return {1.0f, 1.0f, 1.10f, 5, 60, 1.0f, false};
        case InflectionType::EXCLAMATION_EMPHATIC:
            // +10% rise then -10% fall: 1.0 → 1.10 → 0.90
            return {1.0f, 1.10f, 0.90f, 6, 70, 1.0f, true};
        case InflectionType::NEUTRAL:
        default:
            return {1.0f, 1.0f, 1.0f, 0, 0, 1.0f, false};
    }
}

// =============================================================================
// Audio Data Types
// =============================================================================

using AudioSample = int16_t;
using AudioSamples = std::vector<AudioSample>;
using AudioBytes = std::vector<uint8_t>;

// Audio buffer with metadata
struct AudioBuffer {
    AudioSamples samples;
    uint32_t sample_rate = SAMPLE_RATE;
    uint16_t bits_per_sample = BITS_PER_SAMPLE;
    uint16_t channels = NUM_CHANNELS;

    [[nodiscard]] size_t byte_size() const {
        return samples.size() * sizeof(AudioSample);
    }

    [[nodiscard]] double duration_ms() const {
        if (sample_rate == 0) return 0.0;
        return static_cast<double>(samples.size()) * 1000.0 / sample_rate;
    }

    [[nodiscard]] bool empty() const {
        return samples.empty();
    }

    void clear() {
        samples.clear();
    }

    void append(const AudioBuffer& other) {
        samples.insert(samples.end(), other.samples.begin(), other.samples.end());
    }

    void append(const AudioSample* data, size_t count) {
        samples.insert(samples.end(), data, data + count);
    }

    void append_silence(uint32_t duration_ms) {
        size_t num_samples = static_cast<size_t>(sample_rate * duration_ms / 1000);
        samples.insert(samples.end(), num_samples, 0);
    }
};

// =============================================================================
// Number Processing Mode
// =============================================================================

enum class NumberMode : uint8_t {
    WholeNumbers = 0,   // "123" -> "sto dvadeset tri" (default)
    DigitByDigit = 1    // "123" -> "jedan dva tri"
};

// =============================================================================
// Pause Settings (in milliseconds)
// =============================================================================

struct PauseSettings {
    uint32_t sentence_pause_ms = 100;   // Pause after sentence-ending punctuation (. ! ?)
    uint32_t comma_pause_ms = 100;      // Pause after commas
    uint32_t newline_pause_ms = 100;    // Pause for newlines
    uint32_t spelling_pause_ms = 200;   // Pause between spelled characters

    void clamp() {
        sentence_pause_ms = std::clamp(sentence_pause_ms, 0u, 2000u);
        comma_pause_ms = std::clamp(comma_pause_ms, 0u, 2000u);
        newline_pause_ms = std::clamp(newline_pause_ms, 0u, 2000u);
        spelling_pause_ms = std::clamp(spelling_pause_ms, 0u, 2000u);
    }
};

// =============================================================================
// Voice Parameters
// =============================================================================

struct VoiceParams {
    float speed = 1.0f;       // Speech rate (0.5 - 4.0) - Sonic time-stretching
    float pitch = 1.0f;       // Voice character pitch (0.25 - 4.0) - Sonic, shifts formants
    float user_pitch = 1.0f;  // User pitch preference (0.5 - 2.0) - Formant-preserving
    float volume = 1.0f;      // Volume (0.0 - 1.0)
    bool inflection_enabled = true;  // Enable punctuation inflection
    bool emoji_enabled = false;      // Enable emoji to text conversion (disabled by default)
    NumberMode number_mode = NumberMode::WholeNumbers;  // Number processing mode
    PauseSettings pause_settings;    // Pause duration settings

    void clamp() {
        speed = std::clamp(speed, 0.5f, 4.0f);  // 4.0x max for NVDA rate boost
        pitch = std::clamp(pitch, 0.25f, 4.0f);       // Voice character - wider range
        user_pitch = std::clamp(user_pitch, 0.5f, 2.0f);  // User preference - moderate range
        volume = std::clamp(volume, 0.0f, 1.0f);
        pause_settings.clamp();
    }
};

// =============================================================================
// Synthesis Result
// =============================================================================

struct SynthesisResult {
    AudioBuffer audio;
    bool success = false;
    std::string error_message;

    [[nodiscard]] explicit operator bool() const {
        return success;
    }
};

// =============================================================================
// Phoneme Token (output from phoneme mapper)
// =============================================================================

struct PhonemeToken {
    Phoneme phoneme = Phoneme::UNKNOWN;
    uint32_t max_bytes = 0;     // 0 = no limit, otherwise truncate
    float pitch_modifier = 1.0f; // Applied by inflection system

    PhonemeToken() = default;
    explicit PhonemeToken(Phoneme p) : phoneme(p) {
        if (is_truncated_phoneme(p)) {
            max_bytes = TRUNCATED_PHONEME_BYTES;
        }
    }
};

// =============================================================================
// Text Segment (for inflection processing)
// =============================================================================

struct TextSegment {
    std::u32string text;                // UTF-32 for proper Unicode handling
    Punctuation trailing_punct = Punctuation::NONE;
    InflectionType inflection = InflectionType::NEUTRAL;
    bool is_end_of_sentence = false;
};

// =============================================================================
// Packed Phoneme File Format Structures
// =============================================================================

constexpr uint32_t PHONEME_FILE_MAGIC = 0x4C505244;  // "LPRD"
constexpr uint16_t PHONEME_FILE_VERSION = 1;

#pragma pack(push, 1)
struct PackedFileHeader {
    uint32_t magic;             // "LPRD" (0x4C505244)
    uint16_t version;           // Format version
    uint16_t flags;             // Bit 0: encrypted, Bit 1: compressed
    uint32_t phoneme_count;     // Number of phonemes
    uint32_t index_offset;      // Offset to index table
    uint32_t data_offset;       // Offset to audio data
    uint32_t total_size;        // Total file size
    uint32_t sample_rate;       // Audio sample rate
    uint16_t bits_per_sample;   // Bits per sample
    uint16_t channels;          // Number of channels
    uint32_t checksum;          // CRC32 of audio data
    uint8_t encryption_iv[16];  // IV for AES-GCM encryption
    uint8_t reserved[12];       // Reserved for future use
};

struct PhonemeIndexEntry {
    uint32_t phoneme_id;        // Phoneme enum value
    uint32_t name_hash;         // FNV-1a hash of phoneme name
    uint32_t data_offset;       // Offset within data section
    uint32_t compressed_size;   // Size after compression
    uint32_t original_size;     // Original uncompressed size
    uint32_t duration_samples;  // Duration in samples
    uint16_t flags;             // Per-phoneme flags
    uint8_t reserved[6];        // Reserved
};
#pragma pack(pop)

static_assert(sizeof(PackedFileHeader) == 64, "PackedFileHeader must be 64 bytes");
static_assert(sizeof(PhonemeIndexEntry) == 32, "PhonemeIndexEntry must be 32 bytes");

// Packed file flags
constexpr uint16_t PACKED_FLAG_ENCRYPTED = 0x0001;
constexpr uint16_t PACKED_FLAG_COMPRESSED = 0x0002;

// Per-phoneme flags
constexpr uint16_t PHONEME_FLAG_TRUNCATED = 0x0004;

// =============================================================================
// Voice Definitions
// =============================================================================

// Voice language codes
enum class VoiceLanguage : uint8_t {
    Croatian = 0,   // hr-HR, LCID 0x041A
    Serbian = 1     // sr-RS, LCID 0x081A (Latin)
};

// Voice gender
enum class VoiceGender : uint8_t {
    Male = 0,
    Female = 1
};

// Voice age categories
enum class VoiceAge : uint8_t {
    Child = 0,
    Adult = 1,
    Senior = 2
};

// Voice definition structure
struct VoiceDefinition {
    const char* id;              // Internal ID: "josip", "vlado", etc.
    const char* display_name;    // "Laprdus Josip (Croatian)"
    VoiceLanguage language;
    VoiceGender gender;
    VoiceAge age;
    const char* base_voice_id;   // nullptr if physical voice, else "josip" or "vlado"
    float base_pitch;            // Pitch multiplier: 1.0, 1.5, 1.2, 0.75
    const char* data_filename;   // "Josip.bin", nullptr for derived voices
};

// Voice count
constexpr size_t VOICE_COUNT = 5;

// Language code helpers
inline const char* voice_language_code(VoiceLanguage lang) {
    switch (lang) {
        case VoiceLanguage::Croatian: return "hr-HR";
        case VoiceLanguage::Serbian: return "sr-RS";
        default: return "hr-HR";
    }
}

inline uint16_t voice_language_lcid(VoiceLanguage lang) {
    switch (lang) {
        case VoiceLanguage::Croatian: return 0x041A;
        case VoiceLanguage::Serbian: return 0x081A;
        default: return 0x041A;
    }
}

inline const char* voice_gender_string(VoiceGender gender) {
    switch (gender) {
        case VoiceGender::Male: return "Male";
        case VoiceGender::Female: return "Female";
        default: return "Male";
    }
}

inline const char* voice_age_string(VoiceAge age) {
    switch (age) {
        case VoiceAge::Child: return "Child";
        case VoiceAge::Adult: return "Adult";
        case VoiceAge::Senior: return "Senior";
        default: return "Adult";
    }
}

// =============================================================================
// Error Codes
// =============================================================================

enum class ErrorCode : int32_t {
    OK = 0,
    INVALID_HANDLE = -1,
    NOT_INITIALIZED = -2,
    INVALID_PATH = -3,
    LOAD_FAILED = -4,
    SYNTHESIS_FAILED = -5,
    OUT_OF_MEMORY = -6,
    CANCELLED = -7,
    INVALID_PARAMETER = -8,
    DECRYPTION_FAILED = -9,
    FILE_NOT_FOUND = -10,
    INVALID_FORMAT = -11
};

inline const char* error_code_to_string(ErrorCode code) {
    switch (code) {
        case ErrorCode::OK: return "OK";
        case ErrorCode::INVALID_HANDLE: return "Invalid handle";
        case ErrorCode::NOT_INITIALIZED: return "Not initialized";
        case ErrorCode::INVALID_PATH: return "Invalid path";
        case ErrorCode::LOAD_FAILED: return "Load failed";
        case ErrorCode::SYNTHESIS_FAILED: return "Synthesis failed";
        case ErrorCode::OUT_OF_MEMORY: return "Out of memory";
        case ErrorCode::CANCELLED: return "Cancelled";
        case ErrorCode::INVALID_PARAMETER: return "Invalid parameter";
        case ErrorCode::DECRYPTION_FAILED: return "Decryption failed";
        case ErrorCode::FILE_NOT_FOUND: return "File not found";
        case ErrorCode::INVALID_FORMAT: return "Invalid format";
        default: return "Unknown error";
    }
}

} // namespace laprdus

#endif // LAPRDUS_TYPES_HPP
