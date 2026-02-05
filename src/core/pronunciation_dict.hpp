#pragma once
/**
 * @file pronunciation_dict.hpp
 * @brief Pronunciation dictionary for text-to-speech preprocessing
 *
 * This module provides word-level pronunciation correction before phoneme mapping.
 * It supports both an internal (built-in) dictionary and a user-defined dictionary.
 */

#include <string>
#include <vector>
#include <memory>

namespace laprdus {

/**
 * @brief Single entry in the pronunciation dictionary
 */
struct DictionaryEntry {
    std::string grapheme;        ///< The written form to match
    std::string phoneme;         ///< The replacement text (pronunciation)
    bool case_sensitive = false; ///< Whether matching is case-sensitive
    bool whole_word = true;      ///< Whether to match whole words only

    DictionaryEntry() = default;
    DictionaryEntry(const std::string& g, const std::string& p,
                    bool cs = false, bool ww = true)
        : grapheme(g), phoneme(p), case_sensitive(cs), whole_word(ww) {}
};

/**
 * @brief Pronunciation dictionary for custom word pronunciations
 *
 * The dictionary applies text replacements before phoneme mapping,
 * allowing custom pronunciations for abbreviations, acronyms, and
 * words that need special handling.
 *
 * Example:
 *   "ZG" -> "Ze Ge" (Zagreb airport code)
 *   "HR" -> "Ha Er" (Croatia country code)
 */
class PronunciationDictionary {
public:
    PronunciationDictionary();
    ~PronunciationDictionary();

    // Disable copy, allow move
    PronunciationDictionary(const PronunciationDictionary&) = delete;
    PronunciationDictionary& operator=(const PronunciationDictionary&) = delete;
    PronunciationDictionary(PronunciationDictionary&&) noexcept;
    PronunciationDictionary& operator=(PronunciationDictionary&&) noexcept;

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
     * @brief Apply all dictionary replacements to input text
     * @param text The input text to process
     * @return Text with all matching entries replaced
     */
    std::string apply(const std::string& text) const;

    /**
     * @brief Add a single entry to the dictionary
     * @param entry The dictionary entry to add
     */
    void add_entry(const DictionaryEntry& entry);

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
