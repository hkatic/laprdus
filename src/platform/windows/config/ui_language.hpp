// -*- coding: utf-8 -*-
// ui_language.hpp - The language of the configurator's own text
//
// The string tables of laprdus_config.rc are in Croatian, Serbian, Slovenian
// and English. The language follows Windows's display language; the NVDA
// add-on passes NVDA's language with --language=<tag> instead. A language
// without a table of its own reads English, except Bosnian, which reads
// Croatian: the same script and the same ijekavian forms.

#ifndef LAPRDUS_CONFIG_UI_LANGUAGE_HPP
#define LAPRDUS_CONFIG_UI_LANGUAGE_HPP

#include <windows.h>
#include <cwchar>
#include <cwctype>

namespace laprdus {

constexpr LANGID UI_ENGLISH = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
constexpr LANGID UI_CROATIAN = MAKELANGID(LANG_CROATIAN, SUBLANG_CROATIAN_CROATIA);
constexpr LANGID UI_SERBIAN = MAKELANGID(LANG_SERBIAN, SUBLANG_SERBIAN_SERBIA_LATIN);
constexpr LANGID UI_SLOVENIAN = MAKELANGID(LANG_SLOVENIAN, SUBLANG_SLOVENIAN_SLOVENIA);

struct UiLanguage {
    LANGID strings;         // the string table to read
    const wchar_t* speech;  // the language of the SAPI5 token the Test button speaks through
};

// For a language id of Windows. Croatian, Serbian and Bosnian share one
// primary language id (0x1A), so only their whole ids tell them apart.
inline UiLanguage UiLanguageFromWindows(LANGID id) {
    switch (id) {
        case 0x001A: case 0x041A: case 0x101A:   // hr, hr-HR, hr-BA
            return {UI_CROATIAN, L"hr"};
        case 0x141A: case 0x201A: case 0x641A: case 0x681A: case 0x781A:   // bs
            return {UI_CROATIAN, L"bs"};
        case 0x081A: case 0x0C1A: case 0x181A: case 0x1C1A:   // sr-*-CS, sr-*-BA
        case 0x241A: case 0x281A: case 0x2C1A: case 0x301A:   // sr-*-RS, sr-*-ME
        case 0x6C1A: case 0x701A: case 0x7C1A:                // sr-Cyrl, sr-Latn, sr
            return {UI_SERBIAN, L"sr"};
        case 0x0024: case 0x0424:   // sl, sl-SI
            return {UI_SLOVENIAN, L"sl"};
        default:
            return {UI_ENGLISH, L"hr"};
    }
}

// For a language tag: NVDA's forms ("hr", "sr_Latn", "pt_BR") and BCP 47
// ("sr-Latn-RS"), any case. The text after the tag on a command line ends it.
inline UiLanguage UiLanguageFromTag(const wchar_t* tag) {
    wchar_t primary[4] = {};
    int length = 0;
    for (; tag && std::iswalpha(tag[length]); ++length) {
        if (length == 3) return {UI_ENGLISH, L"hr"};
        primary[length] = static_cast<wchar_t>(std::towlower(tag[length]));
    }
    if (std::wcscmp(primary, L"hr") == 0) return {UI_CROATIAN, L"hr"};
    if (std::wcscmp(primary, L"bs") == 0) return {UI_CROATIAN, L"bs"};
    if (std::wcscmp(primary, L"sr") == 0) return {UI_SERBIAN, L"sr"};
    if (std::wcscmp(primary, L"sl") == 0) return {UI_SLOVENIAN, L"sl"};
    return {UI_ENGLISH, L"hr"};
}

// The language picked at start-up by SelectUiLanguage()
inline UiLanguage g_uiLanguage = {UI_ENGLISH, L"hr"};

// Picks the language from the command line or from Windows and makes the
// resource loader read its string table, whether or not Windows has that
// language installed.
inline void SelectUiLanguage(const wchar_t* commandLine) {
    static const wchar_t OPTION[] = L"--language=";
    const wchar_t* option = commandLine ? std::wcsstr(commandLine, OPTION) : nullptr;
    g_uiLanguage = option
        ? UiLanguageFromTag(option + (sizeof(OPTION) / sizeof(OPTION[0])) - 1)
        : UiLanguageFromWindows(GetUserDefaultUILanguage());
    SetThreadUILanguage(g_uiLanguage.strings);
}

}  // namespace laprdus

#endif  // LAPRDUS_CONFIG_UI_LANGUAGE_HPP
