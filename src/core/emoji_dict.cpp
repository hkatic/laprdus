// -*- coding: utf-8 -*-
// emoji_dict.cpp - Emoji dictionary implementation

#include "emoji_dict.hpp"
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>

namespace laprdus {

// =============================================================================
// Implementation Structure
// =============================================================================

struct EmojiDictionary::Impl {
    std::unordered_map<std::string, std::string> entries;
    bool enabled = false;  // Disabled by default

    // Parse JSON content
    bool parse_json(const std::string& json);

    // Get UTF-8 codepoint length
    static size_t utf8_char_length(unsigned char c) {
        if ((c & 0x80) == 0) return 1;
        if ((c & 0xE0) == 0xC0) return 2;
        if ((c & 0xF0) == 0xE0) return 3;
        if ((c & 0xF8) == 0xF0) return 4;
        return 1;  // Invalid, treat as 1
    }

    // Remove variation selectors (U+FE0F, U+FE0E) from emoji string
    // This creates a normalized version for fallback matching
    // U+FE0F = EF B8 8F in UTF-8 (emoji presentation)
    // U+FE0E = EF B8 8E in UTF-8 (text presentation)
    static std::string remove_variation_selectors(const std::string& emoji) {
        std::string result;
        result.reserve(emoji.size());
        size_t i = 0;
        while (i < emoji.size()) {
            // Check for variation selector-16 (FE0F) or variation selector-15 (FE0E)
            // These are 3-byte UTF-8 sequences: EF B8 8F or EF B8 8E
            if (i + 2 < emoji.size() &&
                static_cast<unsigned char>(emoji[i]) == 0xEF &&
                static_cast<unsigned char>(emoji[i + 1]) == 0xB8 &&
                (static_cast<unsigned char>(emoji[i + 2]) == 0x8F ||
                 static_cast<unsigned char>(emoji[i + 2]) == 0x8E)) {
                // Skip the variation selector
                i += 3;
                continue;
            }
            result += emoji[i];
            ++i;
        }
        return result;
    }
};

// =============================================================================
// JSON Parsing (simplified, no external dependencies)
// =============================================================================

bool EmojiDictionary::Impl::parse_json(const std::string& json) {
    // Simple JSON parser for emoji dictionary format:
    // { "version": "1.0", "entries": [ { "emoji": "😀", "text": "nasmijano lice" }, ... ] }

    entries.clear();

    // Find entries array
    size_t entries_start = json.find("\"entries\"");
    if (entries_start == std::string::npos) {
        return false;
    }

    // Find array start
    size_t array_start = json.find('[', entries_start);
    if (array_start == std::string::npos) {
        return false;
    }

    // Parse each entry object
    size_t pos = array_start + 1;
    while (pos < json.size()) {
        // Skip whitespace
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' ||
               json[pos] == '\n' || json[pos] == '\r' || json[pos] == ',')) {
            pos++;
        }

        if (pos >= json.size() || json[pos] == ']') {
            break;  // End of array
        }

        if (json[pos] != '{') {
            pos++;
            continue;
        }

        // Find object end
        size_t obj_end = json.find('}', pos);
        if (obj_end == std::string::npos) {
            break;
        }

        std::string obj = json.substr(pos, obj_end - pos + 1);

        // Extract emoji field
        std::string emoji;
        size_t emoji_key = obj.find("\"emoji\"");
        if (emoji_key != std::string::npos) {
            size_t colon = obj.find(':', emoji_key);
            if (colon != std::string::npos) {
                size_t quote1 = obj.find('"', colon + 1);
                if (quote1 != std::string::npos) {
                    size_t quote2 = obj.find('"', quote1 + 1);
                    if (quote2 != std::string::npos) {
                        emoji = obj.substr(quote1 + 1, quote2 - quote1 - 1);
                    }
                }
            }
        }

        // Extract text field
        std::string text;
        size_t text_key = obj.find("\"text\"");
        if (text_key != std::string::npos) {
            size_t colon = obj.find(':', text_key);
            if (colon != std::string::npos) {
                size_t quote1 = obj.find('"', colon + 1);
                if (quote1 != std::string::npos) {
                    size_t quote2 = obj.find('"', quote1 + 1);
                    if (quote2 != std::string::npos) {
                        text = obj.substr(quote1 + 1, quote2 - quote1 - 1);
                    }
                }
            }
        }

        // Add entry if both fields found
        if (!emoji.empty() && !text.empty()) {
            entries[emoji] = text;

            // Also add a normalized version without variation selectors
            // This ensures matching works whether the emoji is sent with or without
            // variation selectors (e.g., ❤️ vs ❤)
            std::string normalized = remove_variation_selectors(emoji);
            if (normalized != emoji && !normalized.empty()) {
                // Only add if not already present (don't override explicit entries)
                if (entries.find(normalized) == entries.end()) {
                    entries[normalized] = text;
                }
            }
        }

        pos = obj_end + 1;
    }

    return !entries.empty();
}

// =============================================================================
// Constructor / Destructor
// =============================================================================

EmojiDictionary::EmojiDictionary()
    : m_impl(std::make_unique<Impl>())
{
}

EmojiDictionary::~EmojiDictionary() = default;

EmojiDictionary::EmojiDictionary(EmojiDictionary&&) noexcept = default;
EmojiDictionary& EmojiDictionary::operator=(EmojiDictionary&&) noexcept = default;

// =============================================================================
// Load from File
// =============================================================================

bool EmojiDictionary::load_from_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return load_from_memory(buffer.str().c_str(), 0);
}

// =============================================================================
// Load from Memory
// =============================================================================

bool EmojiDictionary::load_from_memory(const char* json_content, size_t length) {
    if (!json_content) {
        return false;
    }

    std::string content;
    if (length == 0) {
        content = json_content;
    } else {
        content.assign(json_content, length);
    }

    return m_impl->parse_json(content);
}

// =============================================================================
// Replace Emojis
// =============================================================================

std::string EmojiDictionary::replace_emojis(const std::string& text) const {
    // If disabled or empty dictionary, return text as-is
    if (!m_impl->enabled || m_impl->entries.empty()) {
        return text;
    }

    std::string result;
    result.reserve(text.size() * 2);  // Reserve extra space for replacements

    size_t pos = 0;
    while (pos < text.size()) {
        bool found = false;

        // Try to match emoji sequences (longest match first)
        // Emojis can be 1-4 UTF-8 chars, some are multi-codepoint sequences
        // Some emojis may span up to 25+ bytes for complex ZWJ sequences
        for (size_t len = 32; len >= 1 && !found; --len) {
            if (pos + len > text.size()) continue;

            std::string candidate = text.substr(pos, len);

            // First try exact match
            auto it = m_impl->entries.find(candidate);
            if (it != m_impl->entries.end()) {
                // Found match - add space before/after if needed
                if (!result.empty() && result.back() != ' ') {
                    result += ' ';
                }
                result += it->second;
                result += ' ';
                pos += len;
                found = true;
                break;
            }

            // If no exact match, try with variation selectors removed from candidate
            // This handles cases where the input text has variation selectors
            // but our dictionary entry doesn't (or vice versa - handled during load)
            std::string normalized = Impl::remove_variation_selectors(candidate);
            if (normalized != candidate && !normalized.empty()) {
                it = m_impl->entries.find(normalized);
                if (it != m_impl->entries.end()) {
                    if (!result.empty() && result.back() != ' ') {
                        result += ' ';
                    }
                    result += it->second;
                    result += ' ';
                    pos += len;
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            // No match - copy character as-is
            size_t char_len = Impl::utf8_char_length(static_cast<unsigned char>(text[pos]));
            result += text.substr(pos, char_len);
            pos += char_len;
        }
    }

    // Clean up extra spaces
    // Remove leading space
    size_t start = 0;
    while (start < result.size() && result[start] == ' ') start++;
    // Remove trailing space
    size_t end = result.size();
    while (end > start && result[end - 1] == ' ') end--;
    // Collapse multiple spaces
    std::string cleaned;
    cleaned.reserve(end - start);
    bool prev_space = false;
    for (size_t i = start; i < end; ++i) {
        if (result[i] == ' ') {
            if (!prev_space) {
                cleaned += ' ';
                prev_space = true;
            }
        } else {
            cleaned += result[i];
            prev_space = false;
        }
    }

    return cleaned;
}

// =============================================================================
// Add Entry
// =============================================================================

void EmojiDictionary::add_entry(const std::string& emoji, const std::string& text) {
    if (!emoji.empty() && !text.empty()) {
        m_impl->entries[emoji] = text;
    }
}

// =============================================================================
// Clear
// =============================================================================

void EmojiDictionary::clear() {
    m_impl->entries.clear();
}

// =============================================================================
// Size / Empty
// =============================================================================

size_t EmojiDictionary::size() const {
    return m_impl->entries.size();
}

bool EmojiDictionary::empty() const {
    return m_impl->entries.empty();
}

// =============================================================================
// Enable / Disable
// =============================================================================

void EmojiDictionary::set_enabled(bool enabled) {
    m_impl->enabled = enabled;
}

bool EmojiDictionary::is_enabled() const {
    return m_impl->enabled;
}

} // namespace laprdus
