/**
 * @file spelling_dict.cpp
 * @brief Implementation of the spelling dictionary
 */

#include "spelling_dict.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <filesystem>

namespace laprdus {

namespace {

/**
 * @brief Convert a UTF-8 string to uppercase
 *
 * Handles Croatian characters: č->Č, ć->Ć, đ->Đ, š->Š, ž->Ž
 */
std::string to_upper_utf8(const std::string& str) {
    std::string result;
    result.reserve(str.size());

    size_t i = 0;
    while (i < str.size()) {
        unsigned char c = str[i];

        // ASCII characters
        if (c < 0x80) {
            result += static_cast<char>(std::toupper(c));
            i++;
        }
        // Two-byte UTF-8 sequences
        else if ((c & 0xE0) == 0xC0 && i + 1 < str.size()) {
            unsigned char c2 = str[i + 1];

            // Croatian lowercase letters (2-byte UTF-8)
            // č (C4 8D) -> Č (C4 8C)
            // ć (C4 87) -> Ć (C4 86)
            // đ (C4 91) -> Đ (C4 90)
            // š (C5 A1) -> Š (C5 A0)
            // ž (C5 BE) -> Ž (C5 BD)
            if (c == 0xC4 && c2 == 0x8D) {
                result += "\xC4\x8C"; // Č
            } else if (c == 0xC4 && c2 == 0x87) {
                result += "\xC4\x86"; // Ć
            } else if (c == 0xC4 && c2 == 0x91) {
                result += "\xC4\x90"; // Đ
            } else if (c == 0xC5 && c2 == 0xA1) {
                result += "\xC5\xA0"; // Š
            } else if (c == 0xC5 && c2 == 0xBE) {
                result += "\xC5\xBD"; // Ž
            } else {
                // Keep as-is
                result += str.substr(i, 2);
            }
            i += 2;
        }
        // Three-byte UTF-8 sequences
        else if ((c & 0xF0) == 0xE0 && i + 2 < str.size()) {
            result += str.substr(i, 3);
            i += 3;
        }
        // Four-byte UTF-8 sequences
        else if ((c & 0xF8) == 0xF0 && i + 3 < str.size()) {
            result += str.substr(i, 4);
            i += 4;
        }
        // Invalid UTF-8, keep as-is
        else {
            result += c;
            i++;
        }
    }

    return result;
}

/**
 * @brief Extract the next UTF-8 character from a string
 * @param str The string to extract from
 * @param pos Current position (updated to next character)
 * @return The extracted character as a string
 */
std::string extract_utf8_char(const std::string& str, size_t& pos) {
    if (pos >= str.size()) {
        return "";
    }

    unsigned char c = str[pos];

    // ASCII character (1 byte)
    if (c < 0x80) {
        return str.substr(pos++, 1);
    }
    // 2-byte UTF-8
    else if ((c & 0xE0) == 0xC0 && pos + 1 < str.size()) {
        pos += 2;
        return str.substr(pos - 2, 2);
    }
    // 3-byte UTF-8
    else if ((c & 0xF0) == 0xE0 && pos + 2 < str.size()) {
        pos += 3;
        return str.substr(pos - 3, 3);
    }
    // 4-byte UTF-8
    else if ((c & 0xF8) == 0xF0 && pos + 3 < str.size()) {
        pos += 4;
        return str.substr(pos - 4, 4);
    }
    // Invalid UTF-8, treat as single byte
    else {
        return str.substr(pos++, 1);
    }
}

// Simple JSON parsing helpers (same approach as pronunciation_dict.cpp)

std::string extract_string_value(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";

    pos = json.find(':', pos + search.length());
    if (pos == std::string::npos) return "";

    pos = json.find_first_not_of(" \t\n\r", pos + 1);
    if (pos == std::string::npos || json[pos] != '"') return "";

    size_t start = pos + 1;
    size_t end = start;

    // Handle escaped characters in JSON strings
    while (end < json.length()) {
        if (json[end] == '\\' && end + 1 < json.length()) {
            end += 2; // Skip escaped character
        } else if (json[end] == '"') {
            break;
        } else {
            end++;
        }
    }

    if (end >= json.length()) return "";

    // Unescape the string
    std::string result;
    for (size_t i = start; i < end; i++) {
        if (json[i] == '\\' && i + 1 < end) {
            char next = json[i + 1];
            switch (next) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                default: result += next; break;
            }
            i++;
        } else {
            result += json[i];
        }
    }

    return result;
}

std::vector<std::string> extract_entries(const std::string& json) {
    std::vector<std::string> entries;

    size_t pos = json.find("\"entries\"");
    if (pos == std::string::npos) return entries;

    pos = json.find('[', pos);
    if (pos == std::string::npos) return entries;

    int depth = 0;
    size_t entry_start = 0;
    bool in_entry = false;

    for (size_t i = pos; i < json.length(); ++i) {
        char c = json[i];

        if (c == '{') {
            if (depth == 1) {
                entry_start = i;
                in_entry = true;
            }
            depth++;
        } else if (c == '}') {
            depth--;
            if (depth == 1 && in_entry) {
                entries.push_back(json.substr(entry_start, i - entry_start + 1));
                in_entry = false;
            }
        } else if (c == '[') {
            depth++;
        } else if (c == ']') {
            depth--;
            if (depth == 0) break;
        }
    }

    return entries;
}

} // anonymous namespace

struct SpellingDictionary::Impl {
    // Map from uppercase character to pronunciation
    std::unordered_map<std::string, std::string> entries;
};

SpellingDictionary::SpellingDictionary()
    : m_impl(std::make_unique<Impl>()) {}

SpellingDictionary::~SpellingDictionary() = default;

SpellingDictionary::SpellingDictionary(SpellingDictionary&&) noexcept = default;
SpellingDictionary& SpellingDictionary::operator=(SpellingDictionary&&) noexcept = default;

bool SpellingDictionary::load_from_file(const std::string& path) {
    // Use std::filesystem::path for proper wide path handling on Windows
    std::filesystem::path fspath(path);
    std::ifstream file(fspath);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return load_from_memory(buffer.str().c_str(), 0);
}

bool SpellingDictionary::load_from_memory(const char* json_content, size_t length) {
    if (!json_content) return false;

    // Clear any existing entries before loading new ones
    m_impl->entries.clear();

    return parse_entries(json_content, length);
}

bool SpellingDictionary::append_from_file(const std::string& path) {
    std::filesystem::path fspath(path);
    std::ifstream file(fspath);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return append_from_memory(buffer.str().c_str(), 0);
}

bool SpellingDictionary::append_from_memory(const char* json_content, size_t length) {
    if (!json_content) return false;

    return parse_entries(json_content, length);
}

bool SpellingDictionary::parse_entries(const char* json_content, size_t length) {
    std::string json = (length > 0) ? std::string(json_content, length) : std::string(json_content);

    auto entry_jsons = extract_entries(json);

    for (const auto& entry_json : entry_jsons) {
        std::string character = extract_string_value(entry_json, "character");
        std::string pronunciation = extract_string_value(entry_json, "pronunciation");

        if (character.empty() || pronunciation.empty()) {
            continue;
        }

        // Store with uppercase key for case-insensitive matching
        std::string key = to_upper_utf8(character);
        m_impl->entries[key] = pronunciation;
    }

    return !m_impl->entries.empty();
}

std::string SpellingDictionary::get_pronunciation(const std::string& character) const {
    if (character.empty()) {
        return "";
    }

    // Lookup with uppercase key
    std::string key = to_upper_utf8(character);
    auto it = m_impl->entries.find(key);

    if (it != m_impl->entries.end()) {
        return it->second;
    }

    // Return character itself if not found
    return character;
}

std::string SpellingDictionary::spell_text(const std::string& text) const {
    if (text.empty() || m_impl->entries.empty()) {
        return text;
    }

    std::string result;
    size_t pos = 0;
    bool first = true;

    while (pos < text.size()) {
        std::string character = extract_utf8_char(text, pos);

        if (character.empty()) {
            break;
        }

        std::string pronunciation = get_pronunciation(character);

        if (!first) {
            result += " ";
        }
        result += pronunciation;
        first = false;
    }

    return result;
}

void SpellingDictionary::add_entry(const std::string& character, const std::string& pronunciation) {
    if (!character.empty() && !pronunciation.empty()) {
        std::string key = to_upper_utf8(character);
        m_impl->entries[key] = pronunciation;
    }
}

void SpellingDictionary::clear() {
    m_impl->entries.clear();
}

size_t SpellingDictionary::size() const {
    return m_impl->entries.size();
}

bool SpellingDictionary::empty() const {
    return m_impl->entries.empty();
}

} // namespace laprdus
