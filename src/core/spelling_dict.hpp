#pragma once
/**
 * @file spelling_dict.hpp
 * @brief Spelling dictionary for character-by-character pronunciation
 *
 * This module provides character-level pronunciation for screen reader
 * spelling mode. Unlike the pronunciation dictionary which handles word-level
 * replacements, this dictionary maps individual characters to their
 * spoken names (e.g., "B" -> "Be", "Č" -> "Če").
 */

#include <string>
#include <memory>

namespace laprdus {

/**
 * @brief Spelling dictionary for character pronunciation
 *
 * The spelling dictionary maps individual characters (including multi-byte
 * UTF-8 characters like Croatian Č, Ć, Đ, Š, Ž) to their spoken names.
 * This is used when screen readers spell out text character-by-character.
 *
 * Example mappings:
 *   "B" -> "Be"
 *   "Č" -> "Če"
 *   "1" -> "jedan"
 *   "." -> "točka"
 *
 * Matching is case-insensitive for letters.
 */
class SpellingDictionary {
public:
    SpellingDictionary();
    ~SpellingDictionary();

    // Disable copy, allow move
    SpellingDictionary(const SpellingDictionary&) = delete;
    SpellingDictionary& operator=(const SpellingDictionary&) = delete;
    SpellingDictionary(SpellingDictionary&&) noexcept;
    SpellingDictionary& operator=(SpellingDictionary&&) noexcept;

    /**
     * @brief Load dictionary from a JSON file (replaces existing entries)
     * @param path Path to the JSON dictionary file
     * @return true if loaded successfully, false otherwise
     */
    bool load_from_file(const std::string& path);

    /**
     * @brief Load dictionary from a memory buffer containing JSON (replaces existing entries)
     * @param json_content JSON string content
     * @param length Length of the content (0 for null-terminated)
     * @return true if parsed successfully, false otherwise
     */
    bool load_from_memory(const char* json_content, size_t length = 0);

    /**
     * @brief Append entries from a JSON file (keeps existing entries)
     * @param path Path to the JSON dictionary file
     * @return true if loaded successfully, false otherwise
     */
    bool append_from_file(const std::string& path);

    /**
     * @brief Append entries from a memory buffer (keeps existing entries)
     * @param json_content JSON string content
     * @param length Length of the content (0 for null-terminated)
     * @return true if parsed successfully, false otherwise
     */
    bool append_from_memory(const char* json_content, size_t length = 0);

    /**
     * @brief Get pronunciation for a single character
     * @param character UTF-8 encoded character (may be multi-byte)
     * @return Pronunciation string, or the character itself if not found
     */
    std::string get_pronunciation(const std::string& character) const;

    /**
     * @brief Spell out entire text character by character
     * @param text The input text to spell
     * @return Text with each character converted to its pronunciation,
     *         separated by spaces
     */
    std::string spell_text(const std::string& text) const;

    /**
     * @brief Add a single character pronunciation
     * @param character The character to match (case-insensitive for letters)
     * @param pronunciation The pronunciation to use
     */
    void add_entry(const std::string& character, const std::string& pronunciation);

    /**
     * @brief Clear all entries from the dictionary
     */
    void clear();

    /**
     * @brief Get the number of entries in the dictionary
     * @return Number of entries
     */
    size_t size() const;

    /**
     * @brief Check if the dictionary is empty
     * @return true if no entries, false otherwise
     */
    bool empty() const;

private:
    bool parse_entries(const char* json_content, size_t length);

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace laprdus
