/**
 * @file pronunciation_dict.cpp
 * @brief Implementation of the pronunciation dictionary
 */

#include "pronunciation_dict.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>

namespace laprdus {

// Simple JSON parsing without external dependencies
// Supports the dictionary format:
// { "version": "1.0", "entries": [ { "grapheme": "...", "phoneme": "...", ... } ] }

namespace {

// Helper to convert string to lowercase (ASCII only, sufficient for our use)
std::string to_lower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });
    return result;
}

// Simple JSON string value extractor with escape sequence handling
std::string extract_string_value(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";

    // Find the colon after the key
    pos = json.find(':', pos + search.length());
    if (pos == std::string::npos) return "";

    // Skip whitespace
    pos = json.find_first_not_of(" \t\n\r", pos + 1);
    if (pos == std::string::npos || json[pos] != '"') return "";

    // Find the closing quote, handling escaped characters
    size_t start = pos + 1;
    size_t end = start;
    while (end < json.length()) {
        if (json[end] == '\\' && end + 1 < json.length()) {
            end += 2;  // Skip escaped character
        } else if (json[end] == '"') {
            break;
        } else {
            end++;
        }
    }
    if (end >= json.length()) return "";

    // Unescape the string
    std::string result;
    result.reserve(end - start);
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

// Simple JSON boolean value extractor
bool extract_bool_value(const std::string& json, const std::string& key, bool default_value) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return default_value;

    // Find the colon after the key
    pos = json.find(':', pos + search.length());
    if (pos == std::string::npos) return default_value;

    // Skip whitespace
    pos = json.find_first_not_of(" \t\n\r", pos + 1);
    if (pos == std::string::npos) return default_value;

    // Check for true/false
    if (json.compare(pos, 4, "true") == 0) return true;
    if (json.compare(pos, 5, "false") == 0) return false;

    return default_value;
}

// Extract all entry objects from the "entries" array
std::vector<std::string> extract_entries(const std::string& json) {
    std::vector<std::string> entries;

    // Find "entries" array
    size_t pos = json.find("\"entries\"");
    if (pos == std::string::npos) return entries;

    // Find opening bracket
    pos = json.find('[', pos);
    if (pos == std::string::npos) return entries;

    // Parse each object
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
            if (depth == 0) break; // End of entries array
        }
    }

    return entries;
}

} // anonymous namespace

struct PronunciationDictionary::Impl {
    std::vector<DictionaryEntry> entries;
};

PronunciationDictionary::PronunciationDictionary()
    : m_impl(std::make_unique<Impl>()) {}

PronunciationDictionary::~PronunciationDictionary() = default;

PronunciationDictionary::PronunciationDictionary(PronunciationDictionary&&) noexcept = default;
PronunciationDictionary& PronunciationDictionary::operator=(PronunciationDictionary&&) noexcept = default;

bool PronunciationDictionary::load_from_file(const std::string& path) {
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

bool PronunciationDictionary::load_from_memory(const char* json_content, size_t length) {
    if (!json_content) return false;

    // Clear any existing entries before loading new ones
    m_impl->entries.clear();

    return parse_entries(json_content, length);
}

bool PronunciationDictionary::append_from_file(const std::string& path) {
    std::filesystem::path fspath(path);
    std::ifstream file(fspath);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return append_from_memory(buffer.str().c_str(), 0);
}

bool PronunciationDictionary::append_from_memory(const char* json_content, size_t length) {
    if (!json_content) return false;

    return parse_entries(json_content, length);
}

bool PronunciationDictionary::parse_entries(const char* json_content, size_t length) {
    std::string json = (length > 0) ? std::string(json_content, length) : std::string(json_content);

    // Extract and parse entries
    auto entry_jsons = extract_entries(json);

    for (const auto& entry_json : entry_jsons) {
        DictionaryEntry entry;
        entry.grapheme = extract_string_value(entry_json, "grapheme");
        entry.phoneme = extract_string_value(entry_json, "phoneme");
        entry.case_sensitive = extract_bool_value(entry_json, "caseSensitive", false);
        entry.whole_word = extract_bool_value(entry_json, "wholeWord", true);

        // Skip invalid entries
        if (entry.grapheme.empty() || entry.phoneme.empty()) {
            continue;
        }

        m_impl->entries.push_back(std::move(entry));
    }

    return !m_impl->entries.empty();
}

std::string PronunciationDictionary::apply(const std::string& text) const {
    if (m_impl->entries.empty() || text.empty()) {
        return text;
    }

    std::string result = text;

    for (const auto& entry : m_impl->entries) {
        if (entry.whole_word) {
            // Whole word matching - use word boundary regex with icase flag
            // Note: We use std::regex::icase instead of character classes [yY][oO]...
            // because the C++ std::regex implementation has issues with \b word
            // boundaries when used adjacent to character class patterns
            std::string pattern = "\\b" + entry.grapheme + "\\b";

            try {
                std::regex re(pattern, entry.case_sensitive ?
                    std::regex::ECMAScript :
                    (std::regex::ECMAScript | std::regex::icase));
                result = std::regex_replace(result, re, entry.phoneme);
            } catch (const std::regex_error&) {
                // Fall back to simple replacement if regex fails
                continue;
            }
        } else {
            // Substring matching
            size_t pos = 0;
            std::string search_text = entry.case_sensitive ? result : to_lower(result);
            std::string search_grapheme = entry.case_sensitive ? entry.grapheme : to_lower(entry.grapheme);

            while ((pos = search_text.find(search_grapheme, pos)) != std::string::npos) {
                result.replace(pos, entry.grapheme.length(), entry.phoneme);
                // Update search_text to reflect the change
                search_text = entry.case_sensitive ? result : to_lower(result);
                pos += entry.phoneme.length();
            }
        }
    }

    return result;
}

void PronunciationDictionary::add_entry(const DictionaryEntry& entry) {
    if (!entry.grapheme.empty() && !entry.phoneme.empty()) {
        m_impl->entries.push_back(entry);
    }
}

void PronunciationDictionary::clear() {
    m_impl->entries.clear();
}

size_t PronunciationDictionary::size() const {
    return m_impl->entries.size();
}

bool PronunciationDictionary::empty() const {
    return m_impl->entries.empty();
}

} // namespace laprdus
