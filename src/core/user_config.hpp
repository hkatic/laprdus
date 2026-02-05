// -*- coding: utf-8 -*-
// user_config.hpp - User configuration management for LaprdusTTS
// Handles loading/saving user settings and dictionaries from config directories

#ifndef LAPRDUS_USER_CONFIG_HPP
#define LAPRDUS_USER_CONFIG_HPP

#include "laprdus/types.hpp"
#include <string>
#include <memory>

namespace laprdus {

/**
 * User configuration settings structure.
 * These are loaded from settings.json in the user config directory.
 */
struct UserSettings {
    // Voice parameters
    float speed = 1.0f;               // Speech rate (0.5 - 2.0)
    float user_pitch = 1.0f;          // User pitch preference (0.5 - 2.0)
    float volume = 1.0f;              // Volume (0.0 - 1.0)
    bool inflection_enabled = true;   // Enable punctuation inflection
    bool emoji_enabled = false;       // Enable emoji to text conversion

    // Number processing
    NumberMode number_mode = NumberMode::WholeNumbers;

    // Pause settings (in milliseconds)
    uint32_t sentence_pause_ms = 100;   // Pause after . ! ?
    uint32_t comma_pause_ms = 100;      // Pause after commas
    uint32_t newline_pause_ms = 100;    // Pause for newlines
    uint32_t spelling_pause_ms = 200;   // Pause between spelled characters

    // Default voice (for platforms that can set it)
    std::string default_voice;

    // Force settings (override SAPI5/system values with Laprdus values)
    bool force_speed = false;     // Use Laprdus speed instead of SAPI5/system
    bool force_pitch = false;     // Use Laprdus pitch instead of SAPI5/system
    bool force_volume = false;    // Use Laprdus volume instead of SAPI5/system

    // User dictionary settings
    bool user_dictionaries_enabled = true;  // Apply user dictionaries during synthesis

    /**
     * Convert to VoiceParams structure.
     * Note: VoiceParams.pitch (voice character pitch) is NOT included here
     * as it's set by the voice definition, not user preference.
     */
    VoiceParams to_voice_params() const;

    /**
     * Apply VoiceParams to these settings (updates relevant fields).
     */
    void from_voice_params(const VoiceParams& params);

    /**
     * Apply pause settings to these settings.
     */
    void apply_pause_settings(const PauseSettings& pause);

    /**
     * Get pause settings from these settings.
     */
    PauseSettings get_pause_settings() const;
};

/**
 * UserConfig - Manages user configuration files and directories.
 *
 * Platform-specific configuration directories:
 *   - Windows: %APPDATA%\Laprdus (e.g., C:\Users\Name\AppData\Roaming\Laprdus)
 *   - Linux:   ~/.config/Laprdus
 *
 * Configuration files:
 *   - settings.json: Main configuration file with all TTS parameters
 *   - user.json: User pronunciation dictionary (loaded after internal.json)
 *   - spelling.json: User spelling dictionary (overrides/extends built-in)
 *   - emoji.json: User emoji dictionary (overrides/extends built-in)
 *
 * The settings.json file is created with default values if it doesn't exist.
 * Dictionary files are optional and only loaded if they exist.
 */
class UserConfig {
public:
    UserConfig();
    ~UserConfig();

    // Non-copyable, moveable
    UserConfig(const UserConfig&) = delete;
    UserConfig& operator=(const UserConfig&) = delete;
    UserConfig(UserConfig&&) noexcept;
    UserConfig& operator=(UserConfig&&) noexcept;

    /**
     * Get the platform-specific user configuration directory path.
     * Creates the directory if it doesn't exist.
     * @return Path to config directory, or empty string on failure.
     */
    static std::string get_config_directory();

    /**
     * Ensure the user configuration directory exists.
     * Creates it with default settings.json if needed.
     * @return true if directory exists or was created successfully.
     */
    bool ensure_config_directory();

    /**
     * Load settings from the user config directory.
     * If settings.json doesn't exist, creates it with defaults.
     * If settings.json has syntax errors, recreates it with defaults.
     * @return true if settings were loaded successfully.
     */
    bool load_settings();

    /**
     * Save current settings to settings.json.
     * @return true if saved successfully.
     */
    bool save_settings();

    /**
     * Get the current user settings.
     * @return Reference to current settings.
     */
    const UserSettings& settings() const;

    /**
     * Get mutable reference to settings for modification.
     * Call save_settings() after modifications to persist changes.
     * @return Mutable reference to settings.
     */
    UserSettings& settings();

    /**
     * Set all settings.
     * @param new_settings New settings to apply.
     */
    void set_settings(const UserSettings& new_settings);

    /**
     * Get the path to the settings.json file.
     * @return Full path to settings.json.
     */
    std::string get_settings_path() const;

    /**
     * Get the path to the user dictionary file.
     * @return Full path to user.json.
     */
    std::string get_user_dictionary_path() const;

    /**
     * Get the path to the user spelling dictionary file.
     * @return Full path to spelling.json (in user config dir).
     */
    std::string get_user_spelling_dictionary_path() const;

    /**
     * Get the path to the user emoji dictionary file.
     * @return Full path to emoji.json (in user config dir).
     */
    std::string get_user_emoji_dictionary_path() const;

    /**
     * Check if a user dictionary file exists.
     * @param filename Name of the dictionary file (e.g., "user.json").
     * @return true if the file exists in the config directory.
     */
    bool user_dictionary_exists(const std::string& filename) const;

    /**
     * Get the full path to a file in the config directory.
     * @param filename Name of the file.
     * @return Full path to the file.
     */
    std::string get_config_file_path(const std::string& filename) const;

    /**
     * Check if the configuration directory exists.
     * @return true if the directory exists.
     */
    bool config_directory_exists() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace laprdus

#endif // LAPRDUS_USER_CONFIG_HPP
