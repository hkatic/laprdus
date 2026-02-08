// -*- coding: utf-8 -*-
// user_config.cpp - User configuration management implementation

#include "user_config.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <pwd.h>
#include <unistd.h>
#endif

namespace laprdus {

// =============================================================================
// UserSettings Implementation
// =============================================================================

VoiceParams UserSettings::to_voice_params() const {
    VoiceParams params;
    params.speed = speed;
    params.pitch = 1.0f;  // Voice character pitch is set by voice definition
    params.user_pitch = user_pitch;
    params.volume = volume;
    params.inflection_enabled = inflection_enabled;
    params.emoji_enabled = emoji_enabled;
    params.number_mode = number_mode;
    params.pause_settings = get_pause_settings();
    params.clamp();
    return params;
}

void UserSettings::from_voice_params(const VoiceParams& params) {
    speed = params.speed;
    user_pitch = params.user_pitch;
    volume = params.volume;
    inflection_enabled = params.inflection_enabled;
    emoji_enabled = params.emoji_enabled;
    number_mode = params.number_mode;
    apply_pause_settings(params.pause_settings);
}

void UserSettings::apply_pause_settings(const PauseSettings& pause) {
    sentence_pause_ms = pause.sentence_pause_ms;
    comma_pause_ms = pause.comma_pause_ms;
    newline_pause_ms = pause.newline_pause_ms;
    spelling_pause_ms = pause.spelling_pause_ms;
}

PauseSettings UserSettings::get_pause_settings() const {
    PauseSettings pause;
    pause.sentence_pause_ms = sentence_pause_ms;
    pause.comma_pause_ms = comma_pause_ms;
    pause.newline_pause_ms = newline_pause_ms;
    pause.spelling_pause_ms = spelling_pause_ms;
    return pause;
}

// =============================================================================
// JSON Parsing Helpers (same pattern as pronunciation_dict.cpp)
// =============================================================================

namespace {

// Extract string value from JSON
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
    while (end < json.length()) {
        if (json[end] == '\\' && end + 1 < json.length()) {
            end += 2;
        } else if (json[end] == '"') {
            break;
        } else {
            end++;
        }
    }
    if (end >= json.length()) return "";

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

// Extract boolean value from JSON
bool extract_bool_value(const std::string& json, const std::string& key, bool default_value) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return default_value;

    pos = json.find(':', pos + search.length());
    if (pos == std::string::npos) return default_value;

    pos = json.find_first_not_of(" \t\n\r", pos + 1);
    if (pos == std::string::npos) return default_value;

    if (json.compare(pos, 4, "true") == 0) return true;
    if (json.compare(pos, 5, "false") == 0) return false;

    return default_value;
}

// Extract numeric value from JSON (supports integers and floats)
double extract_number_value(const std::string& json, const std::string& key, double default_value) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return default_value;

    pos = json.find(':', pos + search.length());
    if (pos == std::string::npos) return default_value;

    pos = json.find_first_not_of(" \t\n\r", pos + 1);
    if (pos == std::string::npos) return default_value;

    // Find end of number
    size_t end = pos;
    bool has_decimal = false;
    bool has_digit = false;
    if (json[end] == '-' || json[end] == '+') end++;
    while (end < json.length()) {
        char c = json[end];
        if (c >= '0' && c <= '9') {
            has_digit = true;
            end++;
        } else if (c == '.' && !has_decimal) {
            has_decimal = true;
            end++;
        } else {
            break;
        }
    }

    if (!has_digit) return default_value;

    try {
        return std::stod(json.substr(pos, end - pos));
    } catch (...) {
        return default_value;
    }
}

// Extract integer value from JSON
int extract_int_value(const std::string& json, const std::string& key, int default_value) {
    return static_cast<int>(extract_number_value(json, key, static_cast<double>(default_value)));
}

// Escape a string for JSON output
std::string escape_json_string(const std::string& str) {
    std::string result;
    result.reserve(str.length() + 10);
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

// Generate settings.json content from UserSettings
std::string generate_settings_json(const UserSettings& settings) {
    std::ostringstream json;
    json << "{\n";
    json << "    \"version\": \"1.0\",\n";
    json << "    \"description\": \"LaprdusTTS user settings\",\n";
    json << "\n";
    json << "    \"_help\": {\n";
    json << "        \"voice.default\": \"Default voice ID: josip, vlado, detence, baba, djed (empty = auto)\",\n";
    json << "        \"speech.speed\": \"Speech rate from 0.5 (slow) to 2.0 (fast), default 1.0\",\n";
    json << "        \"speech.pitch\": \"Voice pitch from 0.5 (low) to 2.0 (high), default 1.0\",\n";
    json << "        \"speech.volume\": \"Volume from 0.0 (silent) to 1.0 (full), default 1.0\",\n";
    json << "        \"speech.inflection\": \"Enable natural pitch variation based on punctuation (true/false)\",\n";
    json << "        \"speech.emoji\": \"Convert emoji to spoken text descriptions (true/false)\",\n";
    json << "        \"numbers.mode\": \"Number reading: 'words' (twenty-three) or 'digits' (two-three)\",\n";
    json << "        \"pauses.sentence\": \"Pause after sentences (. ! ?) in milliseconds, 0-2000\",\n";
    json << "        \"pauses.comma\": \"Pause after commas in milliseconds, 0-2000\",\n";
    json << "        \"pauses.newline\": \"Pause at newlines in milliseconds, 0-2000\",\n";
    json << "        \"pauses.spelling\": \"Pause between spelled characters in milliseconds, 0-2000\",\n";
    json << "        \"force.speed\": \"Use Laprdus speed setting instead of system/SAPI5 (true/false)\",\n";
    json << "        \"force.pitch\": \"Use Laprdus pitch setting instead of system/SAPI5 (true/false)\",\n";
    json << "        \"force.volume\": \"Use Laprdus volume setting instead of system/SAPI5 (true/false)\",\n";
    json << "        \"dictionaries.user_enabled\": \"Apply user dictionaries (user.json, spelling.json, emoji.json) during synthesis (true/false)\"\n";
    json << "    },\n";
    json << "\n";
    json << "    \"voice\": {\n";
    json << "        \"default\": \"" << escape_json_string(settings.default_voice) << "\"\n";
    json << "    },\n";
    json << "\n";
    json << "    \"speech\": {\n";
    json << "        \"speed\": " << settings.speed << ",\n";
    json << "        \"pitch\": " << settings.user_pitch << ",\n";
    json << "        \"volume\": " << settings.volume << ",\n";
    json << "        \"inflection\": " << (settings.inflection_enabled ? "true" : "false") << ",\n";
    json << "        \"emoji\": " << (settings.emoji_enabled ? "true" : "false") << "\n";
    json << "    },\n";
    json << "\n";
    json << "    \"numbers\": {\n";
    json << "        \"mode\": \"" << (settings.number_mode == NumberMode::DigitByDigit ? "digits" : "words") << "\"\n";
    json << "    },\n";
    json << "\n";
    json << "    \"pauses\": {\n";
    json << "        \"sentence\": " << settings.sentence_pause_ms << ",\n";
    json << "        \"comma\": " << settings.comma_pause_ms << ",\n";
    json << "        \"newline\": " << settings.newline_pause_ms << ",\n";
    json << "        \"spelling\": " << settings.spelling_pause_ms << "\n";
    json << "    },\n";
    json << "\n";
    json << "    \"force\": {\n";
    json << "        \"speed\": " << (settings.force_speed ? "true" : "false") << ",\n";
    json << "        \"pitch\": " << (settings.force_pitch ? "true" : "false") << ",\n";
    json << "        \"volume\": " << (settings.force_volume ? "true" : "false") << "\n";
    json << "    },\n";
    json << "\n";
    json << "    \"dictionaries\": {\n";
    json << "        \"user_enabled\": " << (settings.user_dictionaries_enabled ? "true" : "false") << "\n";
    json << "    }\n";
    json << "}\n";
    return json.str();
}

// Parse settings.json content into UserSettings
bool parse_settings_json(const std::string& json, UserSettings& settings) {
    // Check for valid JSON (basic check - must start with {)
    size_t pos = json.find_first_not_of(" \t\n\r");
    if (pos == std::string::npos || json[pos] != '{') {
        return false;
    }

    // Extract voice settings
    settings.default_voice = extract_string_value(json, "default");

    // Extract speech settings
    settings.speed = static_cast<float>(extract_number_value(json, "speed", 1.0));
    settings.user_pitch = static_cast<float>(extract_number_value(json, "pitch", 1.0));
    settings.volume = static_cast<float>(extract_number_value(json, "volume", 1.0));
    settings.inflection_enabled = extract_bool_value(json, "inflection", true);
    settings.emoji_enabled = extract_bool_value(json, "emoji", false);

    // Extract number mode
    std::string number_mode_str = extract_string_value(json, "mode");
    if (number_mode_str == "digits") {
        settings.number_mode = NumberMode::DigitByDigit;
    } else {
        settings.number_mode = NumberMode::WholeNumbers;
    }

    // Extract pause settings
    settings.sentence_pause_ms = static_cast<uint32_t>(extract_int_value(json, "sentence", 100));
    settings.comma_pause_ms = static_cast<uint32_t>(extract_int_value(json, "comma", 100));
    settings.newline_pause_ms = static_cast<uint32_t>(extract_int_value(json, "newline", 100));
    settings.spelling_pause_ms = static_cast<uint32_t>(extract_int_value(json, "spelling", 200));

    // Clamp values to valid ranges
    settings.speed = std::clamp(settings.speed, 0.5f, 4.0f);  // 4.0x max for NVDA rate boost
    settings.user_pitch = std::clamp(settings.user_pitch, 0.5f, 2.0f);
    settings.volume = std::clamp(settings.volume, 0.0f, 1.0f);
    settings.sentence_pause_ms = std::clamp(settings.sentence_pause_ms, 0u, 2000u);
    settings.comma_pause_ms = std::clamp(settings.comma_pause_ms, 0u, 2000u);
    settings.newline_pause_ms = std::clamp(settings.newline_pause_ms, 0u, 2000u);
    settings.spelling_pause_ms = std::clamp(settings.spelling_pause_ms, 0u, 2000u);

    // Extract force settings
    // Look for the "force" section and parse within it
    size_t force_pos = json.find("\"force\"");
    if (force_pos != std::string::npos) {
        // Find the opening brace of the force section
        size_t force_start = json.find('{', force_pos);
        if (force_start != std::string::npos) {
            size_t force_end = json.find('}', force_start);
            if (force_end != std::string::npos) {
                std::string force_section = json.substr(force_start, force_end - force_start + 1);
                settings.force_speed = extract_bool_value(force_section, "speed", false);
                settings.force_pitch = extract_bool_value(force_section, "pitch", false);
                settings.force_volume = extract_bool_value(force_section, "volume", false);
            }
        }
    }

    // Extract dictionaries settings
    size_t dict_pos = json.find("\"dictionaries\"");
    if (dict_pos != std::string::npos) {
        size_t dict_start = json.find('{', dict_pos);
        if (dict_start != std::string::npos) {
            size_t dict_end = json.find('}', dict_start);
            if (dict_end != std::string::npos) {
                std::string dict_section = json.substr(dict_start, dict_end - dict_start + 1);
                settings.user_dictionaries_enabled = extract_bool_value(dict_section, "user_enabled", true);
            }
        }
    }

    return true;
}

} // anonymous namespace

// =============================================================================
// UserConfig Implementation
// =============================================================================

struct UserConfig::Impl {
    std::string config_dir;
    UserSettings settings;
};

UserConfig::UserConfig()
    : m_impl(std::make_unique<Impl>()) {
    m_impl->config_dir = get_config_directory();
}

UserConfig::~UserConfig() = default;

UserConfig::UserConfig(UserConfig&&) noexcept = default;
UserConfig& UserConfig::operator=(UserConfig&&) noexcept = default;

std::string UserConfig::get_config_directory() {
    std::string config_dir;

#ifdef _WIN32
    // Windows: Use %APPDATA%\Laprdus
    // First try SHGetFolderPathW for compatibility
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
        // Convert wide string to UTF-8
        int size = WideCharToMultiByte(CP_UTF8, 0, path, -1, nullptr, 0, nullptr, nullptr);
        if (size > 0) {
            std::string utf8_path(size - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, path, -1, utf8_path.data(), size, nullptr, nullptr);
            config_dir = utf8_path + "\\Laprdus";
        }
    }

    // Fallback to %APPDATA% environment variable
    if (config_dir.empty()) {
        const char* appdata = std::getenv("APPDATA");
        if (appdata) {
            config_dir = std::string(appdata) + "\\Laprdus";
        }
    }
#else
    // Linux/Unix: Use ~/.config/Laprdus (XDG Base Directory Specification)
    const char* xdg_config = std::getenv("XDG_CONFIG_HOME");
    if (xdg_config && xdg_config[0] != '\0') {
        config_dir = std::string(xdg_config) + "/Laprdus";
    } else {
        // Fallback to ~/.config
        const char* home = std::getenv("HOME");
        if (!home) {
            // Fallback to passwd entry
            struct passwd* pw = getpwuid(getuid());
            if (pw) {
                home = pw->pw_dir;
            }
        }
        if (home) {
            config_dir = std::string(home) + "/.config/Laprdus";
        }
    }
#endif

    return config_dir;
}

bool UserConfig::ensure_config_directory() {
    if (m_impl->config_dir.empty()) {
        return false;
    }

    // Create directory if it doesn't exist
    try {
        std::filesystem::path dir_path(m_impl->config_dir);
        if (!std::filesystem::exists(dir_path)) {
            std::filesystem::create_directories(dir_path);
        }
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }

    return std::filesystem::exists(m_impl->config_dir);
}

bool UserConfig::load_settings() {
    // Ensure config directory exists
    if (!ensure_config_directory()) {
        // If we can't create the directory, use defaults
        return true;
    }

    std::string settings_path = get_settings_path();

    // Check if settings.json exists
    if (!std::filesystem::exists(settings_path)) {
        // Create default settings file
        return save_settings();
    }

    // Read settings file
    std::filesystem::path fspath(settings_path);
    std::ifstream file(fspath);
    if (!file.is_open()) {
        // Can't open file, recreate with defaults
        return save_settings();
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    std::string json = buffer.str();

    // Try to parse settings
    if (!parse_settings_json(json, m_impl->settings)) {
        // Parse failed (syntax error), recreate with defaults
        m_impl->settings = UserSettings();  // Reset to defaults
        return save_settings();
    }

    return true;
}

bool UserConfig::save_settings() {
    if (!ensure_config_directory()) {
        return false;
    }

    std::string settings_path = get_settings_path();
    std::string json = generate_settings_json(m_impl->settings);

    std::filesystem::path fspath(settings_path);
    std::ofstream file(fspath);
    if (!file.is_open()) {
        return false;
    }

    file.write(json.c_str(), static_cast<std::streamsize>(json.size()));
    file.close();

    return true;
}

const UserSettings& UserConfig::settings() const {
    return m_impl->settings;
}

UserSettings& UserConfig::settings() {
    return m_impl->settings;
}

void UserConfig::set_settings(const UserSettings& new_settings) {
    m_impl->settings = new_settings;
}

std::string UserConfig::get_settings_path() const {
    return get_config_file_path("settings.json");
}

std::string UserConfig::get_user_dictionary_path() const {
    return get_config_file_path("user.json");
}

std::string UserConfig::get_user_spelling_dictionary_path() const {
    return get_config_file_path("spelling.json");
}

std::string UserConfig::get_user_emoji_dictionary_path() const {
    return get_config_file_path("emoji.json");
}

bool UserConfig::user_dictionary_exists(const std::string& filename) const {
    std::string path = get_config_file_path(filename);
    return std::filesystem::exists(path);
}

std::string UserConfig::get_config_file_path(const std::string& filename) const {
    if (m_impl->config_dir.empty()) {
        return "";
    }

#ifdef _WIN32
    return m_impl->config_dir + "\\" + filename;
#else
    return m_impl->config_dir + "/" + filename;
#endif
}

bool UserConfig::config_directory_exists() const {
    return !m_impl->config_dir.empty() && std::filesystem::exists(m_impl->config_dir);
}

} // namespace laprdus
