// test_ui_language.cpp - Which language the Laprdus Configurator reads under
// which Windows display language or language tag.
//
// Build and run from the project root, in a Visual Studio developer prompt:
//   cl /nologo /EHsc /std:c++17 /I src\platform\windows\config tests\windows\test_ui_language.cpp ^
//      /Fe:build\windows-x64-release\test_ui_language.exe /Fo:build\windows-x64-release\ user32.lib
//   build\windows-x64-release\test_ui_language.exe

#include "ui_language.hpp"
#include <cstdio>
#include <cwchar>

using namespace laprdus;

static int failures = 0;

static void check(const char* name, const UiLanguage& got, LANGID strings, const wchar_t* speech) {
    const bool ok = got.strings == strings && std::wcscmp(got.speech, speech) == 0;
    std::printf("%s %s\n", ok ? "ok  " : "FAIL", name);
    if (!ok) ++failures;
}

int main() {
#define WINDOWS(id, strings, speech) check("Windows " #id, UiLanguageFromWindows(id), strings, speech)
    WINDOWS(0x041A, UI_CROATIAN, L"hr");   // hr-HR
    WINDOWS(0x101A, UI_CROATIAN, L"hr");   // hr-BA
    WINDOWS(0x241A, UI_SERBIAN, L"sr");    // sr-Latn-RS, Windows's Serbian language pack
    WINDOWS(0x281A, UI_SERBIAN, L"sr");    // sr-Cyrl-RS
    WINDOWS(0x081A, UI_SERBIAN, L"sr");    // sr-Latn-CS
    WINDOWS(0x0C1A, UI_SERBIAN, L"sr");    // sr-Cyrl-CS
    WINDOWS(0x181A, UI_SERBIAN, L"sr");    // sr-Latn-BA
    WINDOWS(0x1C1A, UI_SERBIAN, L"sr");    // sr-Cyrl-BA
    WINDOWS(0x2C1A, UI_SERBIAN, L"sr");    // sr-Latn-ME
    WINDOWS(0x301A, UI_SERBIAN, L"sr");    // sr-Cyrl-ME
    WINDOWS(0x141A, UI_CROATIAN, L"bs");   // bs-Latn-BA
    WINDOWS(0x201A, UI_CROATIAN, L"bs");   // bs-Cyrl-BA
    WINDOWS(0x0424, UI_SLOVENIAN, L"sl");  // sl-SI
    WINDOWS(0x0409, UI_ENGLISH, L"hr");    // en-US
    WINDOWS(0x0809, UI_ENGLISH, L"hr");    // en-GB
    WINDOWS(0x0407, UI_ENGLISH, L"hr");    // de-DE
    WINDOWS(0x1000, UI_ENGLISH, L"hr");    // a language without an LCID
#undef WINDOWS

#define TAG(tag, strings, speech) check("tag " #tag, UiLanguageFromTag(tag), strings, speech)
    TAG(L"hr", UI_CROATIAN, L"hr");
    TAG(L"HR_hr", UI_CROATIAN, L"hr");
    TAG(L"sr", UI_SERBIAN, L"sr");
    TAG(L"sr_Latn", UI_SERBIAN, L"sr");
    TAG(L"sr-Cyrl-RS", UI_SERBIAN, L"sr");
    TAG(L"bs", UI_CROATIAN, L"bs");
    TAG(L"sl_SI", UI_SLOVENIAN, L"sl");
    TAG(L"en", UI_ENGLISH, L"hr");
    TAG(L"pt_BR", UI_ENGLISH, L"hr");
    TAG(L"srp", UI_ENGLISH, L"hr");   // NVDA and BCP 47 use the two-letter codes
    TAG(L"", UI_ENGLISH, L"hr");
    TAG(L"sr --other", UI_SERBIAN, L"sr");
    TAG(L"verylongtag", UI_ENGLISH, L"hr");
#undef TAG

    std::printf("%d failed\n", failures);
    return failures ? 1 : 0;
}
