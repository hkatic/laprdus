// -*- coding: utf-8 -*-
// phoneme_mapper.cpp - Character to phoneme mapping implementation

#include "phoneme_mapper.hpp"
#include <cctype>
#include <stdexcept>

namespace laprdus {

// =============================================================================
// Constructor
// =============================================================================

PhonemeMapper::PhonemeMapper() {
    init_mappings();
}

// =============================================================================
// Initialize Character Mappings
// =============================================================================

void PhonemeMapper::init_mappings() {
    // Standard ASCII letters (lowercase)
    m_char_map[U'a'] = Phoneme::A;
    m_char_map[U'b'] = Phoneme::B;
    m_char_map[U'c'] = Phoneme::C;
    m_char_map[U'd'] = Phoneme::D;  // Note: 'd' handled specially for 'dž' digraph
    m_char_map[U'e'] = Phoneme::E;
    m_char_map[U'f'] = Phoneme::F;
    m_char_map[U'g'] = Phoneme::G;
    m_char_map[U'h'] = Phoneme::H;
    m_char_map[U'i'] = Phoneme::I;
    m_char_map[U'j'] = Phoneme::J;  // Note: 'j' handled specially for lj/nj digraphs
    m_char_map[U'k'] = Phoneme::K;
    m_char_map[U'l'] = Phoneme::L;  // Note: 'l' handled specially for 'lj' digraph
    m_char_map[U'm'] = Phoneme::M;
    m_char_map[U'n'] = Phoneme::N;  // Note: 'n' handled specially for 'nj' digraph
    m_char_map[U'o'] = Phoneme::O;
    m_char_map[U'p'] = Phoneme::P;
    m_char_map[U'q'] = Phoneme::Q;
    m_char_map[U'r'] = Phoneme::R;
    m_char_map[U's'] = Phoneme::S;
    m_char_map[U't'] = Phoneme::T;
    m_char_map[U'u'] = Phoneme::U;
    m_char_map[U'v'] = Phoneme::V;
    m_char_map[U'w'] = Phoneme::W;
    m_char_map[U'x'] = Phoneme::X;
    m_char_map[U'y'] = Phoneme::Y;
    m_char_map[U'z'] = Phoneme::Z;

    // Standard ASCII letters (uppercase) - map to same phonemes
    m_char_map[U'A'] = Phoneme::A;
    m_char_map[U'B'] = Phoneme::B;
    m_char_map[U'C'] = Phoneme::C;
    m_char_map[U'D'] = Phoneme::D;
    m_char_map[U'E'] = Phoneme::E;
    m_char_map[U'F'] = Phoneme::F;
    m_char_map[U'G'] = Phoneme::G;
    m_char_map[U'H'] = Phoneme::H;
    m_char_map[U'I'] = Phoneme::I;
    m_char_map[U'J'] = Phoneme::J;
    m_char_map[U'K'] = Phoneme::K;
    m_char_map[U'L'] = Phoneme::L;
    m_char_map[U'M'] = Phoneme::M;
    m_char_map[U'N'] = Phoneme::N;
    m_char_map[U'O'] = Phoneme::O;
    m_char_map[U'P'] = Phoneme::P;
    m_char_map[U'Q'] = Phoneme::Q;
    m_char_map[U'R'] = Phoneme::R;
    m_char_map[U'S'] = Phoneme::S;
    m_char_map[U'T'] = Phoneme::T;
    m_char_map[U'U'] = Phoneme::U;
    m_char_map[U'V'] = Phoneme::V;
    m_char_map[U'W'] = Phoneme::W;
    m_char_map[U'X'] = Phoneme::X;
    m_char_map[U'Y'] = Phoneme::Y;
    m_char_map[U'Z'] = Phoneme::Z;

    // Croatian special characters (lowercase)
    m_char_map[croatian::LETTER_C_CARON] = Phoneme::CH;      // č
    m_char_map[croatian::LETTER_C_ACUTE] = Phoneme::TJ;      // ć
    m_char_map[croatian::LETTER_D_STROKE] = Phoneme::DJ;     // đ
    m_char_map[croatian::LETTER_S_CARON] = Phoneme::SH;      // š
    m_char_map[croatian::LETTER_Z_CARON] = Phoneme::ZH;      // ž

    // Croatian special characters (uppercase)
    m_char_map[croatian::LETTER_C_CARON_UPPER] = Phoneme::CH;   // Č
    m_char_map[croatian::LETTER_C_ACUTE_UPPER] = Phoneme::TJ;   // Ć
    m_char_map[croatian::LETTER_D_STROKE_UPPER] = Phoneme::DJ;  // Đ
    m_char_map[croatian::LETTER_S_CARON_UPPER] = Phoneme::SH;   // Š
    m_char_map[croatian::LETTER_Z_CARON_UPPER] = Phoneme::ZH;   // Ž

    // Croatian digraph ligatures (Unicode single characters)
    m_char_map[croatian::LETTER_DZ_CARON] = Phoneme::DJ;     // dž as single char
    m_char_map[croatian::LETTER_LJ] = Phoneme::LJ;           // lj as single char
    m_char_map[croatian::LETTER_NJ] = Phoneme::NJ;           // nj as single char
}

// =============================================================================
// Map Text to Phonemes
// =============================================================================

std::vector<PhonemeToken> PhonemeMapper::map_text(const std::string& text) {
    std::vector<PhonemeToken> result;
    result.reserve(text.size());  // Pre-allocate for efficiency

    // Reset state machine
    m_state = State::NORMAL;

    // Convert to UTF-32 for proper character handling
    std::u32string utf32 = utf8_to_utf32(text);

    // Convert Cyrillic to Latin (supports Serbian and Macedonian)
    // This is done early in the pipeline so all subsequent processing
    // works with Latin characters (reusing existing phoneme mappings)
    utf32 = cyrillic::to_latin(utf32);

    // Process each character
    for (char32_t ch : utf32) {
        process_char(ch, result);
    }

    // Flush any remaining state
    flush_state(result);

    return result;
}

// =============================================================================
// Process Single Character with State Machine
// =============================================================================

void PhonemeMapper::process_char(char32_t ch, std::vector<PhonemeToken>& output) {
    // Convert to lowercase for consistent handling
    char32_t ch_lower = ch;
    if (ch >= U'A' && ch <= U'Z') {
        ch_lower = ch + 32;  // ASCII lowercase conversion
    }

    switch (m_state) {
        case State::AFTER_L:
            if (ch_lower == U'j') {
                // 'lj' digraph detected
                output.emplace_back(Phoneme::LJ);
                m_state = State::NORMAL;
                return;
            } else {
                // Not 'lj', emit 'l' and continue processing current char
                output.emplace_back(Phoneme::L);
                m_state = State::NORMAL;
                // Fall through to process current character
            }
            break;

        case State::AFTER_N:
            if (ch_lower == U'j') {
                // 'nj' digraph detected
                output.emplace_back(Phoneme::NJ);
                m_state = State::NORMAL;
                return;
            } else {
                // Not 'nj', emit 'n' and continue processing current char
                output.emplace_back(Phoneme::N);
                m_state = State::NORMAL;
                // Fall through to process current character
            }
            break;

        case State::AFTER_D:
            if (ch_lower == croatian::LETTER_Z_CARON ||
                ch_lower == croatian::LETTER_Z_CARON_UPPER) {
                // 'dž' digraph detected
                output.emplace_back(Phoneme::DJ);
                m_state = State::NORMAL;
                return;
            } else {
                // Not 'dž', emit 'd' and continue processing current char
                output.emplace_back(Phoneme::D);
                m_state = State::NORMAL;
                // Fall through to process current character
            }
            break;

        case State::NORMAL:
            // Continue below
            break;
    }

    // Check for digraph start characters
    if (ch_lower == U'l') {
        m_state = State::AFTER_L;
        return;
    }
    if (ch_lower == U'n') {
        m_state = State::AFTER_N;
        return;
    }
    if (ch_lower == U'd') {
        m_state = State::AFTER_D;
        return;
    }

    // Handle period followed by space as silence (matching Python behavior)
    // Note: This is handled at a higher level now, but we still recognize
    // punctuation for the inflection system

    // Look up in character map
    auto it = m_char_map.find(ch);
    if (it != m_char_map.end()) {
        output.emplace_back(it->second);
    } else {
        // Unknown character - skip it (matching Python t_error behavior)
        // output.emplace_back(Phoneme::UNKNOWN);
        // Actually, Python skips unknown chars silently, so we do too
    }
}

// =============================================================================
// Flush Pending State
// =============================================================================

void PhonemeMapper::flush_state(std::vector<PhonemeToken>& output) {
    switch (m_state) {
        case State::AFTER_L:
            output.emplace_back(Phoneme::L);
            break;
        case State::AFTER_N:
            output.emplace_back(Phoneme::N);
            break;
        case State::AFTER_D:
            output.emplace_back(Phoneme::D);
            break;
        case State::NORMAL:
            // Nothing to flush
            break;
    }
    m_state = State::NORMAL;
}

// =============================================================================
// Map Single Character
// =============================================================================

PhonemeToken PhonemeMapper::map_character(char32_t ch) {
    auto it = m_char_map.find(ch);
    if (it != m_char_map.end()) {
        return PhonemeToken(it->second);
    }

    // Check for uppercase variant
    if (ch >= U'A' && ch <= U'Z') {
        char32_t lower = ch + 32;
        it = m_char_map.find(lower);
        if (it != m_char_map.end()) {
            return PhonemeToken(it->second);
        }
    }

    return PhonemeToken(Phoneme::UNKNOWN);
}

// =============================================================================
// Detect Punctuation
// =============================================================================

Punctuation PhonemeMapper::detect_punctuation(char32_t ch) {
    switch (ch) {
        case U',':
            return Punctuation::COMMA;
        case U'.':
            return Punctuation::PERIOD;
        case U'?':
            return Punctuation::QUESTION;
        case U'!':
            return Punctuation::EXCLAMATION;
        case U';':
            return Punctuation::SEMICOLON;
        case U':':
            return Punctuation::COLON;
        case U'\u2026':  // …
            return Punctuation::ELLIPSIS;
        default:
            return Punctuation::NONE;
    }
}

// =============================================================================
// UTF-8 to UTF-32 Conversion
// =============================================================================

std::u32string PhonemeMapper::utf8_to_utf32(const std::string& utf8) {
    std::u32string result;
    result.reserve(utf8.size());  // Usually fewer chars, but good starting point

    size_t i = 0;
    while (i < utf8.size()) {
        char32_t cp = 0;
        unsigned char c = static_cast<unsigned char>(utf8[i]);

        if ((c & 0x80) == 0) {
            // ASCII (0xxxxxxx)
            cp = c;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            // 2-byte sequence (110xxxxx 10xxxxxx)
            if (i + 1 >= utf8.size()) break;
            cp = (c & 0x1F) << 6;
            cp |= (static_cast<unsigned char>(utf8[i + 1]) & 0x3F);
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            // 3-byte sequence (1110xxxx 10xxxxxx 10xxxxxx)
            if (i + 2 >= utf8.size()) break;
            cp = (c & 0x0F) << 12;
            cp |= (static_cast<unsigned char>(utf8[i + 1]) & 0x3F) << 6;
            cp |= (static_cast<unsigned char>(utf8[i + 2]) & 0x3F);
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            // 4-byte sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
            if (i + 3 >= utf8.size()) break;
            cp = (c & 0x07) << 18;
            cp |= (static_cast<unsigned char>(utf8[i + 1]) & 0x3F) << 12;
            cp |= (static_cast<unsigned char>(utf8[i + 2]) & 0x3F) << 6;
            cp |= (static_cast<unsigned char>(utf8[i + 3]) & 0x3F);
            i += 4;
        } else {
            // Invalid UTF-8, skip byte
            i += 1;
            continue;
        }

        result.push_back(cp);
    }

    return result;
}

// =============================================================================
// UTF-32 to UTF-8 Conversion
// =============================================================================

std::string PhonemeMapper::utf32_to_utf8(const std::u32string& utf32) {
    std::string result;
    result.reserve(utf32.size() * 2);  // Estimate

    for (char32_t cp : utf32) {
        if (cp < 0x80) {
            // ASCII
            result.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            // 2-byte
            result.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            // 3-byte
            result.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x110000) {
            // 4-byte
            result.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
        // Invalid code points are silently skipped
    }

    return result;
}

// =============================================================================
// Phoneme Name Lookup
// =============================================================================

const char* PhonemeMapper::phoneme_name(Phoneme p) {
    switch (p) {
        case Phoneme::A: return "A";
        case Phoneme::B: return "B";
        case Phoneme::C: return "C";
        case Phoneme::D: return "D";
        case Phoneme::E: return "E";
        case Phoneme::F: return "F";
        case Phoneme::G: return "G";
        case Phoneme::H: return "H";
        case Phoneme::I: return "I";
        case Phoneme::J: return "J";
        case Phoneme::K: return "K";
        case Phoneme::L: return "L";
        case Phoneme::M: return "M";
        case Phoneme::N: return "N";
        case Phoneme::O: return "O";
        case Phoneme::P: return "P";
        case Phoneme::Q: return "Q";
        case Phoneme::R: return "R";
        case Phoneme::S: return "S";
        case Phoneme::T: return "T";
        case Phoneme::U: return "U";
        case Phoneme::V: return "V";
        case Phoneme::W: return "W";
        case Phoneme::X: return "X";
        case Phoneme::Y: return "Y";
        case Phoneme::Z: return "Z";
        case Phoneme::CH: return "CH";
        case Phoneme::TJ: return "TJ";
        case Phoneme::DJ: return "DJ";
        case Phoneme::SH: return "SH";
        case Phoneme::ZH: return "ZH";
        case Phoneme::LJ: return "LJ";
        case Phoneme::NJ: return "NJ";
        case Phoneme::SILENCE: return "SILENCE";
        case Phoneme::UNKNOWN: return "UNKNOWN";
        default: return "INVALID";
    }
}

// =============================================================================
// Phoneme Filename Lookup
// =============================================================================

std::string PhonemeMapper::phoneme_filename(Phoneme p) {
    switch (p) {
        case Phoneme::SILENCE:
            return "-.wav";
        case Phoneme::UNKNOWN:
            return "";
        default:
            return std::string("PHONEME_") + phoneme_name(p) + ".wav";
    }
}

// =============================================================================
// Cyrillic to Latin Conversion (Serbian/Macedonian)
// =============================================================================

namespace cyrillic {

// Cyrillic to Latin mapping table
// Supports Serbian and Macedonian Cyrillic alphabets
// Using array for O(1) lookup - index by (codepoint - 0x0400)
// Returns nullptr for unmapped characters

struct CyrillicMapping {
    char32_t cyrillic;
    const char32_t* latin;  // May be multi-character (e.g., "lj", "nj", "dž")
};

// Serbian Cyrillic specific letters
constexpr char32_t CYRILLIC_LJE_LOWER = U'\u0459';  // љ
constexpr char32_t CYRILLIC_LJE_UPPER = U'\u0409';  // Љ
constexpr char32_t CYRILLIC_NJE_LOWER = U'\u045A';  // њ
constexpr char32_t CYRILLIC_NJE_UPPER = U'\u040A';  // Њ
constexpr char32_t CYRILLIC_TSHE_LOWER = U'\u045B'; // ћ (Serbian)
constexpr char32_t CYRILLIC_TSHE_UPPER = U'\u040B'; // Ћ
constexpr char32_t CYRILLIC_DJE_LOWER = U'\u0452';  // ђ (Serbian)
constexpr char32_t CYRILLIC_DJE_UPPER = U'\u0402';  // Ђ
constexpr char32_t CYRILLIC_DZHE_LOWER = U'\u045F'; // џ
constexpr char32_t CYRILLIC_DZHE_UPPER = U'\u040F'; // Џ

// Macedonian specific letters
constexpr char32_t CYRILLIC_GJE_LOWER = U'\u0453';  // ѓ (Macedonian)
constexpr char32_t CYRILLIC_GJE_UPPER = U'\u0403';  // Ѓ
constexpr char32_t CYRILLIC_KJE_LOWER = U'\u045C';  // ќ (Macedonian)
constexpr char32_t CYRILLIC_KJE_UPPER = U'\u040C';  // Ќ
constexpr char32_t CYRILLIC_S_W_DESC_LOWER = U'\u0455'; // ѕ (Macedonian dz)
constexpr char32_t CYRILLIC_S_W_DESC_UPPER = U'\u0405'; // Ѕ

std::u32string to_latin(const std::u32string& text) {
    std::u32string result;
    result.reserve(text.size() * 2);  // May expand due to digraphs

    for (char32_t ch : text) {
        // Fast path: non-Cyrillic characters pass through unchanged
        if (!is_cyrillic(ch)) {
            result.push_back(ch);
            continue;
        }

        // Serbian/Macedonian Cyrillic conversion
        // Based on standard transliteration tables
        switch (ch) {
            // Lowercase vowels
            case U'\u0430': result.push_back(U'a'); break;  // а -> a
            case U'\u0435': result.push_back(U'e'); break;  // е -> e
            case U'\u0438': result.push_back(U'i'); break;  // и -> i
            case U'\u043E': result.push_back(U'o'); break;  // о -> o
            case U'\u0443': result.push_back(U'u'); break;  // у -> u

            // Uppercase vowels
            case U'\u0410': result.push_back(U'A'); break;  // А -> A
            case U'\u0415': result.push_back(U'E'); break;  // Е -> E
            case U'\u0418': result.push_back(U'I'); break;  // И -> I
            case U'\u041E': result.push_back(U'O'); break;  // О -> O
            case U'\u0423': result.push_back(U'U'); break;  // У -> U

            // Lowercase consonants (simple mapping)
            case U'\u0431': result.push_back(U'b'); break;  // б -> b
            case U'\u0432': result.push_back(U'v'); break;  // в -> v
            case U'\u0433': result.push_back(U'g'); break;  // г -> g
            case U'\u0434': result.push_back(U'd'); break;  // д -> d
            case U'\u0436': result.push_back(croatian::LETTER_Z_CARON); break;  // ж -> ž
            case U'\u0437': result.push_back(U'z'); break;  // з -> z
            case U'\u0458': result.push_back(U'j'); break;  // ј -> j
            case U'\u043A': result.push_back(U'k'); break;  // к -> k
            case U'\u043B': result.push_back(U'l'); break;  // л -> l
            case U'\u043C': result.push_back(U'm'); break;  // м -> m
            case U'\u043D': result.push_back(U'n'); break;  // н -> n
            case U'\u043F': result.push_back(U'p'); break;  // п -> p
            case U'\u0440': result.push_back(U'r'); break;  // р -> r
            case U'\u0441': result.push_back(U's'); break;  // с -> s
            case U'\u0442': result.push_back(U't'); break;  // т -> t
            case U'\u0444': result.push_back(U'f'); break;  // ф -> f
            case U'\u0445': result.push_back(U'h'); break;  // х -> h
            case U'\u0446': result.push_back(U'c'); break;  // ц -> c
            case U'\u0447': result.push_back(croatian::LETTER_C_CARON); break;  // ч -> č
            case U'\u0448': result.push_back(croatian::LETTER_S_CARON); break;  // ш -> š

            // Uppercase consonants (simple mapping)
            case U'\u0411': result.push_back(U'B'); break;  // Б -> B
            case U'\u0412': result.push_back(U'V'); break;  // В -> V
            case U'\u0413': result.push_back(U'G'); break;  // Г -> G
            case U'\u0414': result.push_back(U'D'); break;  // Д -> D
            case U'\u0416': result.push_back(croatian::LETTER_Z_CARON_UPPER); break;  // Ж -> Ž
            case U'\u0417': result.push_back(U'Z'); break;  // З -> Z
            case U'\u0408': result.push_back(U'J'); break;  // Ј -> J
            case U'\u041A': result.push_back(U'K'); break;  // К -> K
            case U'\u041B': result.push_back(U'L'); break;  // Л -> L
            case U'\u041C': result.push_back(U'M'); break;  // М -> M
            case U'\u041D': result.push_back(U'N'); break;  // Н -> N
            case U'\u041F': result.push_back(U'P'); break;  // П -> P
            case U'\u0420': result.push_back(U'R'); break;  // Р -> R
            case U'\u0421': result.push_back(U'S'); break;  // С -> S
            case U'\u0422': result.push_back(U'T'); break;  // Т -> T
            case U'\u0424': result.push_back(U'F'); break;  // Ф -> F
            case U'\u0425': result.push_back(U'H'); break;  // Х -> H
            case U'\u0426': result.push_back(U'C'); break;  // Ц -> C
            case U'\u0427': result.push_back(croatian::LETTER_C_CARON_UPPER); break;  // Ч -> Č
            case U'\u0428': result.push_back(croatian::LETTER_S_CARON_UPPER); break;  // Ш -> Š

            // Serbian-specific letters (lowercase)
            case CYRILLIC_LJE_LOWER:  // љ -> lj
                result.push_back(U'l');
                result.push_back(U'j');
                break;
            case CYRILLIC_NJE_LOWER:  // њ -> nj
                result.push_back(U'n');
                result.push_back(U'j');
                break;
            case CYRILLIC_TSHE_LOWER:  // ћ -> ć
                result.push_back(croatian::LETTER_C_ACUTE);
                break;
            case CYRILLIC_DJE_LOWER:  // ђ -> đ
                result.push_back(croatian::LETTER_D_STROKE);
                break;
            case CYRILLIC_DZHE_LOWER:  // џ -> dž
                result.push_back(U'd');
                result.push_back(croatian::LETTER_Z_CARON);
                break;

            // Serbian-specific letters (uppercase)
            case CYRILLIC_LJE_UPPER:  // Љ -> Lj
                result.push_back(U'L');
                result.push_back(U'j');
                break;
            case CYRILLIC_NJE_UPPER:  // Њ -> Nj
                result.push_back(U'N');
                result.push_back(U'j');
                break;
            case CYRILLIC_TSHE_UPPER:  // Ћ -> Ć
                result.push_back(croatian::LETTER_C_ACUTE_UPPER);
                break;
            case CYRILLIC_DJE_UPPER:  // Ђ -> Đ
                result.push_back(croatian::LETTER_D_STROKE_UPPER);
                break;
            case CYRILLIC_DZHE_UPPER:  // Џ -> Dž
                result.push_back(U'D');
                result.push_back(croatian::LETTER_Z_CARON);
                break;

            // Macedonian-specific letters (lowercase)
            case CYRILLIC_GJE_LOWER:  // ѓ -> gj (Macedonian palatal g)
                result.push_back(croatian::LETTER_D_STROKE);  // Pronounced like đ
                break;
            case CYRILLIC_KJE_LOWER:  // ќ -> kj (Macedonian palatal k)
                result.push_back(croatian::LETTER_C_ACUTE);  // Pronounced like ć
                break;
            case CYRILLIC_S_W_DESC_LOWER:  // ѕ -> dz (Macedonian)
                result.push_back(U'd');
                result.push_back(U'z');
                break;

            // Macedonian-specific letters (uppercase)
            case CYRILLIC_GJE_UPPER:  // Ѓ -> Gj
                result.push_back(croatian::LETTER_D_STROKE_UPPER);
                break;
            case CYRILLIC_KJE_UPPER:  // Ќ -> Kj
                result.push_back(croatian::LETTER_C_ACUTE_UPPER);
                break;
            case CYRILLIC_S_W_DESC_UPPER:  // Ѕ -> Dz
                result.push_back(U'D');
                result.push_back(U'z');
                break;

            default:
                // Unknown Cyrillic character - pass through
                // This allows for future expansion or edge cases
                result.push_back(ch);
                break;
        }
    }

    return result;
}

} // namespace cyrillic

} // namespace laprdus
