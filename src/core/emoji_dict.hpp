// -*- coding: utf-8 -*-
// emoji_dict.hpp - Emoji to text dictionary for TTS
// Converts emoji characters to their Croatian/Serbian text representations

#ifndef LAPRDUS_EMOJI_DICT_HPP
#define LAPRDUS_EMOJI_DICT_HPP

#include <string>
#include <memory>

namespace laprdus {

/**
 * EmojiDictionary - Converts emoji characters to spoken text.
 *
 * Features:
 * - Maps Unicode emoji codepoints to Croatian text
 * - Supports Emoji 15.1 standard (3,782+ emojis)
 * - Case-insensitive UTF-8 handling
 * - Configurable (can be enabled/disabled)
 * - Disabled by default on all platforms
 *
 * Usage:
 *   EmojiDictionary dict;
 *   dict.load_from_file("emoji.json");  // or load_from_memory
 *   std::string text = dict.replace_emojis("Hello 😀 world");
 *   // text = "Hello nasmijano lice world"
 */
class EmojiDictionary {
public:
    EmojiDictionary();
    ~EmojiDictionary();

    // Non-copyable
    EmojiDictionary(const EmojiDictionary&) = delete;
    EmojiDictionary& operator=(const EmojiDictionary&) = delete;

    // Moveable
    EmojiDictionary(EmojiDictionary&&) noexcept;
    EmojiDictionary& operator=(EmojiDictionary&&) noexcept;

    /**
     * Load emoji dictionary from JSON file (replaces existing entries).
     * @param path Path to emoji dictionary JSON file.
     * @return true on success.
     */
    bool load_from_file(const std::string& path);

    /**
     * Load emoji dictionary from memory (replaces existing entries).
     * @param json_content JSON content string.
     * @param length Length of content (0 for null-terminated).
     * @return true on success.
     */
    bool load_from_memory(const char* json_content, size_t length = 0);

    /**
     * Append entries from a JSON file (keeps existing entries).
     * @param path Path to emoji dictionary JSON file.
     * @return true on success.
     */
    bool append_from_file(const std::string& path);

    /**
     * Append entries from memory (keeps existing entries).
     * @param json_content JSON content string.
     * @param length Length of content (0 for null-terminated).
     * @return true on success.
     */
    bool append_from_memory(const char* json_content, size_t length = 0);

    /**
     * Replace all emojis in text with their text representations.
     * @param text Input text possibly containing emojis.
     * @return Text with emojis replaced by words.
     */
    std::string replace_emojis(const std::string& text) const;

    /**
     * Add a single emoji entry.
     * @param emoji UTF-8 emoji character(s).
     * @param text Spoken text representation.
     */
    void add_entry(const std::string& emoji, const std::string& text);

    /**
     * Clear all entries.
     */
    void clear();

    /**
     * Get number of loaded entries.
     * @return Number of emoji entries.
     */
    size_t size() const;

    /**
     * Check if dictionary is empty.
     * @return true if no entries loaded.
     */
    bool empty() const;

    /**
     * Enable or disable emoji processing.
     * @param enabled true to enable, false to disable.
     */
    void set_enabled(bool enabled);

    /**
     * Check if emoji processing is enabled.
     * @return true if enabled.
     */
    bool is_enabled() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace laprdus

#endif // LAPRDUS_EMOJI_DICT_HPP
