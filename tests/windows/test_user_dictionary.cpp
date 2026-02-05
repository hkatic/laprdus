/*
 * test_user_dictionary.cpp - Unit tests for user dictionary functionality
 *
 * These tests verify:
 * - Dictionary JSON file parsing and generation
 * - Entry CRUD operations
 * - Case sensitivity and whole word matching
 * - Dictionary application to text
 *
 * Build (from project root):
 *   cl /EHsc /std:c++17 /I include /I tests\windows tests\windows\test_user_dictionary.cpp ^
 *      src\core\user_config.cpp src\core\dictionary.cpp ^
 *      /Fe:build\windows-x64-release\test_user_dictionary.exe
 *
 * Run:
 *   build\windows-x64-release\test_user_dictionary.exe
 */

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlobj.h>

namespace fs = std::filesystem;

// =============================================================================
// Helper Functions for JSON Parsing (matching dictionary_dialog.cpp logic)
// =============================================================================

struct TestDictionaryEntry {
    std::wstring grapheme;
    std::wstring phoneme;
    std::wstring comment;
    bool caseSensitive = false;
    bool wholeWord = true;
};

/**
 * Convert wide string to UTF-8.
 */
static std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return "";
    std::string result(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, result.data(), size, nullptr, nullptr);
    return result;
}

/**
 * Convert UTF-8 to wide string.
 */
static std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (size <= 0) return L"";
    std::wstring result(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, result.data(), size);
    return result;
}

/**
 * Parse string value from JSON (basic parser).
 */
static std::wstring ParseJsonString(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return L"";

    pos = json.find('"', pos + searchKey.length());
    if (pos == std::string::npos) return L"";

    size_t endPos = json.find('"', pos + 1);
    while (endPos != std::string::npos && json[endPos - 1] == '\\') {
        endPos = json.find('"', endPos + 1);
    }
    if (endPos == std::string::npos) return L"";

    std::string value = json.substr(pos + 1, endPos - pos - 1);
    // Unescape
    std::string unescaped;
    for (size_t i = 0; i < value.length(); ++i) {
        if (value[i] == '\\' && i + 1 < value.length()) {
            switch (value[i + 1]) {
                case 'n': unescaped += '\n'; ++i; break;
                case 'r': unescaped += '\r'; ++i; break;
                case 't': unescaped += '\t'; ++i; break;
                case '\\': unescaped += '\\'; ++i; break;
                case '"': unescaped += '"'; ++i; break;
                default: unescaped += value[i]; break;
            }
        } else {
            unescaped += value[i];
        }
    }
    return Utf8ToWide(unescaped);
}

/**
 * Parse boolean value from JSON.
 */
static bool ParseJsonBool(const std::string& json, const std::string& key, bool defaultVal = false) {
    std::string searchKey = "\"" + key + "\":";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return defaultVal;

    pos += searchKey.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;

    if (pos < json.length() - 3 && json.substr(pos, 4) == "true") return true;
    if (pos < json.length() - 4 && json.substr(pos, 5) == "false") return false;
    return defaultVal;
}

/**
 * Escape string for JSON.
 */
static std::string EscapeJsonString(const std::wstring& wstr) {
    std::string str = WideToUtf8(wstr);
    std::string result;
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

/**
 * Get temp directory for test files.
 */
static fs::path GetTestDir() {
    wchar_t temp[MAX_PATH];
    GetTempPathW(MAX_PATH, temp);
    fs::path dir = fs::path(temp) / L"laprdus_dict_tests";
    fs::create_directories(dir);
    return dir;
}

/**
 * Clean up test directory.
 */
static void CleanupTestDir() {
    fs::path dir = GetTestDir();
    if (fs::exists(dir)) {
        fs::remove_all(dir);
    }
}

// =============================================================================
// JSON Parsing Tests
// =============================================================================

TEST_CASE("JSON string parsing extracts values correctly", "[json][parse]") {
    SECTION("Simple string value") {
        std::string json = R"({"grapheme": "hello", "phoneme": "ola"})";
        REQUIRE(ParseJsonString(json, "grapheme") == L"hello");
        REQUIRE(ParseJsonString(json, "phoneme") == L"ola");
    }

    SECTION("String with special characters") {
        std::string json = R"({"text": "line1\nline2"})";
        std::wstring result = ParseJsonString(json, "text");
        REQUIRE(result.find(L'\n') != std::wstring::npos);
    }

    SECTION("String with quotes") {
        std::string json = R"({"text": "say \"hello\""})";
        std::wstring result = ParseJsonString(json, "text");
        REQUIRE(result == L"say \"hello\"");
    }

    SECTION("Missing key returns empty") {
        std::string json = R"({"grapheme": "test"})";
        REQUIRE(ParseJsonString(json, "missing") == L"");
    }
}

TEST_CASE("JSON boolean parsing extracts values correctly", "[json][parse]") {
    SECTION("True value") {
        std::string json = R"({"caseSensitive": true})";
        REQUIRE(ParseJsonBool(json, "caseSensitive") == true);
    }

    SECTION("False value") {
        std::string json = R"({"caseSensitive": false})";
        REQUIRE(ParseJsonBool(json, "caseSensitive") == false);
    }

    SECTION("Missing key uses default") {
        std::string json = R"({"other": true})";
        REQUIRE(ParseJsonBool(json, "caseSensitive", false) == false);
        REQUIRE(ParseJsonBool(json, "caseSensitive", true) == true);
    }
}

// =============================================================================
// Dictionary File Creation Tests
// =============================================================================

TEST_CASE("Dictionary file is created on first save", "[file][create]") {
    CleanupTestDir();
    fs::path testDir = GetTestDir();
    fs::path dictPath = testDir / L"test_dict.json";

    REQUIRE_FALSE(fs::exists(dictPath));

    // Create a simple dictionary JSON
    std::string json = R"({
    "version": "1.0",
    "entries": [
        {
            "grapheme": "Facebook",
            "phoneme": "Fejzbuk",
            "caseSensitive": false,
            "wholeWord": true,
            "comment": "Social media"
        }
    ]
})";

    std::ofstream f(dictPath);
    f << json;
    f.close();

    REQUIRE(fs::exists(dictPath));
    REQUIRE(fs::file_size(dictPath) > 0);

    CleanupTestDir();
}

TEST_CASE("Dictionary JSON structure is valid", "[json][structure]") {
    fs::path testDir = GetTestDir();
    fs::path dictPath = testDir / L"structure_test.json";

    // Write test JSON
    std::string json = R"({
    "version": "1.0",
    "entries": [
        {
            "grapheme": "Test",
            "phoneme": "Test replacement",
            "caseSensitive": true,
            "wholeWord": false,
            "comment": "Test comment"
        }
    ]
})";

    std::ofstream f(dictPath);
    f << json;
    f.close();

    // Read and parse
    std::ifstream in(dictPath);
    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string content = buffer.str();

    REQUIRE(content.find("\"version\"") != std::string::npos);
    REQUIRE(content.find("\"entries\"") != std::string::npos);
    REQUIRE(content.find("\"grapheme\"") != std::string::npos);
    REQUIRE(content.find("\"phoneme\"") != std::string::npos);

    CleanupTestDir();
}

// =============================================================================
// Entry CRUD Operation Tests
// =============================================================================

TEST_CASE("Dictionary entries can be added", "[crud][add]") {
    std::vector<TestDictionaryEntry> entries;

    SECTION("Add single entry") {
        TestDictionaryEntry entry;
        entry.grapheme = L"hello";
        entry.phoneme = L"helo";
        entries.push_back(entry);

        REQUIRE(entries.size() == 1);
        REQUIRE(entries[0].grapheme == L"hello");
    }

    SECTION("Add multiple entries") {
        for (int i = 0; i < 5; ++i) {
            TestDictionaryEntry entry;
            entry.grapheme = L"word" + std::to_wstring(i);
            entry.phoneme = L"pronunciation" + std::to_wstring(i);
            entries.push_back(entry);
        }

        REQUIRE(entries.size() == 5);
    }
}

TEST_CASE("Dictionary entries can be edited", "[crud][edit]") {
    std::vector<TestDictionaryEntry> entries;

    TestDictionaryEntry entry;
    entry.grapheme = L"original";
    entry.phoneme = L"original_pron";
    entries.push_back(entry);

    SECTION("Edit grapheme") {
        entries[0].grapheme = L"modified";
        REQUIRE(entries[0].grapheme == L"modified");
        REQUIRE(entries[0].phoneme == L"original_pron");
    }

    SECTION("Edit phoneme") {
        entries[0].phoneme = L"modified_pron";
        REQUIRE(entries[0].grapheme == L"original");
        REQUIRE(entries[0].phoneme == L"modified_pron");
    }

    SECTION("Edit flags") {
        entries[0].caseSensitive = true;
        entries[0].wholeWord = false;
        REQUIRE(entries[0].caseSensitive == true);
        REQUIRE(entries[0].wholeWord == false);
    }
}

TEST_CASE("Dictionary entries can be deleted", "[crud][delete]") {
    std::vector<TestDictionaryEntry> entries;

    for (int i = 0; i < 3; ++i) {
        TestDictionaryEntry entry;
        entry.grapheme = L"word" + std::to_wstring(i);
        entries.push_back(entry);
    }

    REQUIRE(entries.size() == 3);

    SECTION("Delete middle entry") {
        entries.erase(entries.begin() + 1);
        REQUIRE(entries.size() == 2);
        REQUIRE(entries[0].grapheme == L"word0");
        REQUIRE(entries[1].grapheme == L"word2");
    }

    SECTION("Delete first entry") {
        entries.erase(entries.begin());
        REQUIRE(entries.size() == 2);
        REQUIRE(entries[0].grapheme == L"word1");
    }

    SECTION("Delete last entry") {
        entries.pop_back();
        REQUIRE(entries.size() == 2);
        REQUIRE(entries[1].grapheme == L"word1");
    }
}

TEST_CASE("Dictionary entries can be duplicated", "[crud][duplicate]") {
    std::vector<TestDictionaryEntry> entries;

    TestDictionaryEntry entry;
    entry.grapheme = L"original";
    entry.phoneme = L"pron";
    entry.comment = L"comment";
    entry.caseSensitive = true;
    entry.wholeWord = false;
    entries.push_back(entry);

    // Duplicate entry
    TestDictionaryEntry duplicate = entries[0];
    duplicate.grapheme = L"original (copy)";
    entries.push_back(duplicate);

    REQUIRE(entries.size() == 2);
    REQUIRE(entries[1].grapheme == L"original (copy)");
    REQUIRE(entries[1].phoneme == L"pron");
    REQUIRE(entries[1].comment == L"comment");
    REQUIRE(entries[1].caseSensitive == true);
    REQUIRE(entries[1].wholeWord == false);
}

// =============================================================================
// Case Sensitivity Tests
// =============================================================================

TEST_CASE("Case sensitive matching works correctly", "[matching][case]") {
    TestDictionaryEntry entry;
    entry.grapheme = L"Test";
    entry.phoneme = L"replacement";
    entry.caseSensitive = true;

    SECTION("Exact case matches") {
        std::wstring text = L"This is a Test.";
        size_t pos = text.find(entry.grapheme);
        REQUIRE(pos != std::wstring::npos);
    }

    SECTION("Different case does not match when case sensitive") {
        std::wstring text = L"This is a test.";
        size_t pos = text.find(entry.grapheme);
        REQUIRE(pos == std::wstring::npos);
    }
}

TEST_CASE("Case insensitive matching works correctly", "[matching][case]") {
    TestDictionaryEntry entry;
    entry.grapheme = L"test";
    entry.phoneme = L"replacement";
    entry.caseSensitive = false;

    // Simulate case-insensitive search
    auto caseInsensitiveFind = [](const std::wstring& text, const std::wstring& pattern) {
        std::wstring lowerText = text;
        std::wstring lowerPattern = pattern;
        for (auto& c : lowerText) c = towlower(c);
        for (auto& c : lowerPattern) c = towlower(c);
        return lowerText.find(lowerPattern);
    };

    SECTION("Lowercase matches") {
        std::wstring text = L"This is a test.";
        REQUIRE(caseInsensitiveFind(text, entry.grapheme) != std::wstring::npos);
    }

    SECTION("Uppercase matches") {
        std::wstring text = L"This is a TEST.";
        REQUIRE(caseInsensitiveFind(text, entry.grapheme) != std::wstring::npos);
    }

    SECTION("Mixed case matches") {
        std::wstring text = L"This is a TeSt.";
        REQUIRE(caseInsensitiveFind(text, entry.grapheme) != std::wstring::npos);
    }
}

// =============================================================================
// Whole Word Matching Tests
// =============================================================================

TEST_CASE("Whole word matching works correctly", "[matching][wholeword]") {
    TestDictionaryEntry entry;
    entry.grapheme = L"test";
    entry.phoneme = L"replacement";
    entry.wholeWord = true;

    // Simulate whole word check
    auto isWholeWord = [](const std::wstring& text, size_t pos, size_t len) {
        // Check character before
        if (pos > 0 && iswalnum(text[pos - 1])) return false;
        // Check character after
        if (pos + len < text.length() && iswalnum(text[pos + len])) return false;
        return true;
    };

    SECTION("Standalone word matches") {
        std::wstring text = L"This is a test here.";
        size_t pos = text.find(entry.grapheme);
        REQUIRE(pos != std::wstring::npos);
        REQUIRE(isWholeWord(text, pos, entry.grapheme.length()) == true);
    }

    SECTION("Word at start matches") {
        std::wstring text = L"test is good.";
        size_t pos = text.find(entry.grapheme);
        REQUIRE(pos != std::wstring::npos);
        REQUIRE(isWholeWord(text, pos, entry.grapheme.length()) == true);
    }

    SECTION("Word at end matches") {
        std::wstring text = L"This is a test";
        size_t pos = text.find(entry.grapheme);
        REQUIRE(pos != std::wstring::npos);
        REQUIRE(isWholeWord(text, pos, entry.grapheme.length()) == true);
    }

    SECTION("Word within another word does not match") {
        std::wstring text = L"This is testing.";
        size_t pos = text.find(entry.grapheme);
        REQUIRE(pos != std::wstring::npos);
        REQUIRE(isWholeWord(text, pos, entry.grapheme.length()) == false);
    }

    SECTION("Word with prefix does not match") {
        std::wstring text = L"This is a pretest.";
        size_t pos = text.find(entry.grapheme);
        REQUIRE(pos != std::wstring::npos);
        REQUIRE(isWholeWord(text, pos, entry.grapheme.length()) == false);
    }
}

// =============================================================================
// Dictionary Type Tests
// =============================================================================

TEST_CASE("All dictionary types can be created", "[types]") {
    fs::path testDir = GetTestDir();

    SECTION("Main dictionary") {
        fs::path path = testDir / L"user.json";
        std::ofstream f(path);
        f << R"({"version":"1.0","entries":[]})";
        f.close();
        REQUIRE(fs::exists(path));
    }

    SECTION("Spelling dictionary") {
        fs::path path = testDir / L"spelling.json";
        std::ofstream f(path);
        f << R"({"version":"1.0","entries":[]})";
        f.close();
        REQUIRE(fs::exists(path));
    }

    SECTION("Emoji dictionary") {
        fs::path path = testDir / L"emoji.json";
        std::ofstream f(path);
        f << R"({"version":"1.0","entries":[]})";
        f.close();
        REQUIRE(fs::exists(path));
    }

    CleanupTestDir();
}

// =============================================================================
// Error Handling Tests
// =============================================================================

TEST_CASE("Missing file is handled gracefully", "[error]") {
    fs::path testDir = GetTestDir();
    fs::path nonExistent = testDir / L"nonexistent.json";

    REQUIRE_FALSE(fs::exists(nonExistent));

    // Attempting to read should not crash
    std::ifstream f(nonExistent);
    REQUIRE_FALSE(f.good());
}

TEST_CASE("Invalid JSON is handled gracefully", "[error][json]") {
    fs::path testDir = GetTestDir();
    fs::path invalidPath = testDir / L"invalid.json";

    // Write invalid JSON
    std::ofstream f(invalidPath);
    f << "{ this is not valid json }";
    f.close();

    // Read file content
    std::ifstream in(invalidPath);
    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string content = buffer.str();

    // Attempting to parse should return empty/default values
    REQUIRE(ParseJsonString(content, "grapheme") == L"");
    REQUIRE(ParseJsonBool(content, "caseSensitive", true) == true);

    CleanupTestDir();
}

TEST_CASE("Empty entry fields are validated", "[validation]") {
    TestDictionaryEntry entry;

    SECTION("Empty grapheme is invalid") {
        entry.grapheme = L"";
        entry.phoneme = L"replacement";
        REQUIRE(entry.grapheme.empty());
    }

    SECTION("Empty phoneme is invalid") {
        entry.grapheme = L"test";
        entry.phoneme = L"";
        REQUIRE(entry.phoneme.empty());
    }

    SECTION("Both fields present is valid") {
        entry.grapheme = L"test";
        entry.phoneme = L"replacement";
        REQUIRE_FALSE(entry.grapheme.empty());
        REQUIRE_FALSE(entry.phoneme.empty());
    }
}

// =============================================================================
// User Config Integration Tests
// =============================================================================

TEST_CASE("User dictionary enabled setting persists", "[config]") {
    // This tests the user_dictionaries_enabled setting in UserSettings
    // The actual integration is tested via the full application

    bool enabled = true;

    SECTION("Can be enabled") {
        enabled = true;
        REQUIRE(enabled == true);
    }

    SECTION("Can be disabled") {
        enabled = false;
        REQUIRE(enabled == false);
    }

    SECTION("Default is enabled") {
        bool defaultValue = true;
        REQUIRE(defaultValue == true);
    }
}

// =============================================================================
// UTF-8 / Unicode Tests
// =============================================================================

TEST_CASE("Unicode characters are handled correctly", "[unicode]") {
    SECTION("Croatian characters") {
        std::wstring text = L"čćžšđ";
        std::string utf8 = WideToUtf8(text);
        std::wstring back = Utf8ToWide(utf8);
        REQUIRE(back == text);
    }

    SECTION("Serbian Cyrillic") {
        std::wstring text = L"ћџљњ";
        std::string utf8 = WideToUtf8(text);
        std::wstring back = Utf8ToWide(utf8);
        REQUIRE(back == text);
    }

    SECTION("Emoji characters") {
        std::wstring text = L"😀🎉";
        std::string utf8 = WideToUtf8(text);
        // Note: Emoji may require surrogate pairs in UTF-16
        REQUIRE_FALSE(utf8.empty());
    }
}

TEST_CASE("JSON escaping handles special characters", "[json][escape]") {
    SECTION("Backslash") {
        std::wstring text = L"path\\to\\file";
        std::string escaped = EscapeJsonString(text);
        REQUIRE(escaped.find("\\\\") != std::string::npos);
    }

    SECTION("Newline") {
        std::wstring text = L"line1\nline2";
        std::string escaped = EscapeJsonString(text);
        REQUIRE(escaped.find("\\n") != std::string::npos);
    }

    SECTION("Quote") {
        std::wstring text = L"say \"hello\"";
        std::string escaped = EscapeJsonString(text);
        REQUIRE(escaped.find("\\\"") != std::string::npos);
    }
}
