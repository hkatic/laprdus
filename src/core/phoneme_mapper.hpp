// -*- coding: utf-8 -*-
// phoneme_mapper.hpp - Character to phoneme mapping for Croatian TTS
// Replaces PLY lexer with a simple UTF-8 state machine

#ifndef LAPRDUS_PHONEME_MAPPER_HPP
#define LAPRDUS_PHONEME_MAPPER_HPP

#include "laprdus/types.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace laprdus {

/**
 * PhonemeMapper - Converts UTF-8 text to phoneme tokens.
 *
 * Handles:
 * - Standard ASCII letters (case-insensitive)
 * - Croatian special characters: č, ć, đ, š, ž
 * - Croatian digraphs: lj, nj, dž
 * - Punctuation detection for inflection
 * - Unknown character handling
 */
class PhonemeMapper {
public:
    PhonemeMapper();
    ~PhonemeMapper() = default;

    // Non-copyable, moveable
    PhonemeMapper(const PhonemeMapper&) = delete;
    PhonemeMapper& operator=(const PhonemeMapper&) = delete;
    PhonemeMapper(PhonemeMapper&&) = default;
    PhonemeMapper& operator=(PhonemeMapper&&) = default;

    /**
     * Convert UTF-8 text to a sequence of phoneme tokens.
     * @param text UTF-8 encoded input text.
     * @return Vector of phoneme tokens.
     */
    std::vector<PhonemeToken> map_text(const std::string& text);

    /**
     * Convert a single UTF-32 character to a phoneme.
     * @param ch Unicode code point.
     * @return Phoneme token (may be UNKNOWN for unrecognized chars).
     */
    PhonemeToken map_character(char32_t ch);

    /**
     * Detect punctuation type from a character.
     * @param ch Unicode code point.
     * @return Punctuation type.
     */
    static Punctuation detect_punctuation(char32_t ch);

    /**
     * Convert UTF-8 string to UTF-32 for proper character handling.
     * @param utf8 UTF-8 encoded string.
     * @return UTF-32 string.
     */
    static std::u32string utf8_to_utf32(const std::string& utf8);

    /**
     * Convert UTF-32 string to UTF-8.
     * @param utf32 UTF-32 string.
     * @return UTF-8 encoded string.
     */
    static std::string utf32_to_utf8(const std::u32string& utf32);

    /**
     * Get the name of a phoneme (for debugging/logging).
     * @param p Phoneme enum value.
     * @return Phoneme name string.
     */
    static const char* phoneme_name(Phoneme p);

    /**
     * Get the WAV filename for a phoneme.
     * @param p Phoneme enum value.
     * @return Filename like "PHONEME_A.wav".
     */
    static std::string phoneme_filename(Phoneme p);

private:
    // State machine for digraph handling
    enum class State {
        NORMAL,
        AFTER_L,    // Waiting to see if 'j' follows for 'lj'
        AFTER_N,    // Waiting to see if 'j' follows for 'nj'
        AFTER_D     // Waiting to see if 'ž' follows for 'dž'
    };

    State m_state = State::NORMAL;

    // Direct character to phoneme mapping
    std::unordered_map<char32_t, Phoneme> m_char_map;

    // Initialize character mappings
    void init_mappings();

    // Process a character with state machine for digraphs
    void process_char(char32_t ch, std::vector<PhonemeToken>& output);

    // Flush any pending state
    void flush_state(std::vector<PhonemeToken>& output);
};

// =============================================================================
// Croatian Unicode Code Points
// =============================================================================

namespace croatian {
    // Lowercase
    constexpr char32_t LETTER_C_CARON = U'\u010D';       // č
    constexpr char32_t LETTER_C_ACUTE = U'\u0107';       // ć
    constexpr char32_t LETTER_D_STROKE = U'\u0111';      // đ
    constexpr char32_t LETTER_S_CARON = U'\u0161';       // š
    constexpr char32_t LETTER_Z_CARON = U'\u017E';       // ž

    // Uppercase
    constexpr char32_t LETTER_C_CARON_UPPER = U'\u010C'; // Č
    constexpr char32_t LETTER_C_ACUTE_UPPER = U'\u0106'; // Ć
    constexpr char32_t LETTER_D_STROKE_UPPER = U'\u0110';// Đ
    constexpr char32_t LETTER_S_CARON_UPPER = U'\u0160'; // Š
    constexpr char32_t LETTER_Z_CARON_UPPER = U'\u017D'; // Ž

    // Digraph ligatures (rarely used, but included for completeness)
    constexpr char32_t LETTER_DZ_CARON = U'\u01C6';      // dž (digraph)
    constexpr char32_t LETTER_LJ = U'\u01C9';            // lj (ligature)
    constexpr char32_t LETTER_NJ = U'\u01CC';            // nj (ligature)
}

// =============================================================================
// Serbian/Macedonian Cyrillic to Latin Conversion
// =============================================================================

namespace cyrillic {
    /**
     * Convert Cyrillic text to Latin.
     * Supports Serbian and Macedonian Cyrillic alphabets.
     * @param text UTF-32 text with Cyrillic characters.
     * @return UTF-32 text with Cyrillic converted to Latin equivalents.
     */
    std::u32string to_latin(const std::u32string& text);

    /**
     * Check if a character is Cyrillic.
     * @param ch Unicode code point.
     * @return true if the character is Cyrillic.
     */
    constexpr bool is_cyrillic(char32_t ch) {
        // Cyrillic range: U+0400 to U+04FF (Cyrillic)
        // Extended Cyrillic: U+0500 to U+052F (Cyrillic Supplement)
        return (ch >= 0x0400 && ch <= 0x04FF) || (ch >= 0x0500 && ch <= 0x052F);
    }
}

} // namespace laprdus

#endif // LAPRDUS_PHONEME_MAPPER_HPP
