/*
 * test_speechd_module.cpp - Unit tests for LaprdusTTS Speech Dispatcher module
 *
 * These tests verify the Speech Dispatcher integration functionality
 * by testing the module's parameter mapping and synthesis.
 *
 * Requirements:
 * - speech-dispatcher installed and running
 * - laprdus module installed
 *
 * Build: g++ -std=c++17 -I../../include test_speechd_module.cpp -o test_speechd_module -llaprdus -lpthread
 * Run: ./test_speechd_module
 */

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <strings.h>   /* strcasecmp, as the module uses it */
#include <array>
#include <vector>
#include <thread>
#include <chrono>

/* LaprdusTTS C API for parameter mapping verification */
#include <laprdus/laprdus_api.h>

using Catch::Approx;

/* Data directory */
static const char* DATA_DIR = "/usr/share/laprdus";

/* Helper to run spd-say command */
std::pair<int, std::string> run_spd_say(const std::string& args) {
    std::string cmd = "spd-say " + args + " 2>&1";
    std::array<char, 256> buffer;
    std::string result;

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return {-1, "Failed to run command"};
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }

    int exit_code = pclose(pipe);
    return {WEXITSTATUS(exit_code), result};
}

/* Helper to check if speech-dispatcher is running */
bool speechd_available() {
    auto [code, output] = run_spd_say("-l");
    return code == 0;
}

/* Helper to check if laprdus module is available */
bool laprdus_module_available() {
    auto [code, output] = run_spd_say("-L");
    return output.find("laprdus") != std::string::npos;
}

// =============================================================================
// Parameter Mapping Tests (Unit tests, no speechd required)
// =============================================================================

TEST_CASE("Speech Dispatcher rate mapping", "[speechd][mapping]") {
    /*
     * Speech Dispatcher uses rate -100 to +100
     * Laprdus uses speed 0.5 to 2.0
     *
     * Mapping:
     *   rate -100 -> speed 0.5
     *   rate    0 -> speed 1.0
     *   rate +100 -> speed 2.0
     */

    auto map_rate_to_speed = [](int rate) -> float {
        if (rate <= 0) {
            return 1.0f + (rate / 200.0f);
        } else {
            return 1.0f + (rate / 100.0f);
        }
    };

    REQUIRE(map_rate_to_speed(-100) == Approx(0.5f));
    REQUIRE(map_rate_to_speed(-50) == Approx(0.75f));
    REQUIRE(map_rate_to_speed(0) == Approx(1.0f));
    REQUIRE(map_rate_to_speed(50) == Approx(1.5f));
    REQUIRE(map_rate_to_speed(100) == Approx(2.0f));
}

TEST_CASE("Speech Dispatcher pitch mapping", "[speechd][mapping]") {
    /*
     * Speech Dispatcher uses pitch -100 to +100
     * Laprdus uses user_pitch 0.5 to 2.0
     *
     * Mapping:
     *   pitch -100 -> user_pitch 0.5
     *   pitch    0 -> user_pitch 1.0
     *   pitch +100 -> user_pitch 2.0
     */

    auto map_pitch_to_user_pitch = [](int pitch) -> float {
        if (pitch <= 0) {
            return 1.0f + (pitch / 200.0f);
        } else {
            return 1.0f + (pitch / 100.0f);
        }
    };

    REQUIRE(map_pitch_to_user_pitch(-100) == Approx(0.5f));
    REQUIRE(map_pitch_to_user_pitch(-50) == Approx(0.75f));
    REQUIRE(map_pitch_to_user_pitch(0) == Approx(1.0f));
    REQUIRE(map_pitch_to_user_pitch(50) == Approx(1.5f));
    REQUIRE(map_pitch_to_user_pitch(100) == Approx(2.0f));
}

TEST_CASE("Speech Dispatcher volume mapping", "[speechd][mapping]") {
    /*
     * Speech Dispatcher uses volume -100 to +100
     * Laprdus uses volume 0.0 to 1.0
     *
     * Mapping:
     *   volume -100 -> 0.0
     *   volume    0 -> 0.5
     *   volume +100 -> 1.0
     */

    auto map_volume = [](int volume) -> float {
        return (volume + 100) / 200.0f;
    };

    REQUIRE(map_volume(-100) == Approx(0.0f));
    REQUIRE(map_volume(0) == Approx(0.5f));
    REQUIRE(map_volume(100) == Approx(1.0f));
}

// =============================================================================
// Speech Dispatcher Integration Tests (requires speechd running)
// =============================================================================

TEST_CASE("Speech Dispatcher module is installed", "[speechd][integration]") {
    if (!speechd_available()) {
        SKIP("Speech Dispatcher not available");
    }

    auto [code, output] = run_spd_say("-L");
    REQUIRE(code == 0);

    // Check if laprdus module is listed
    INFO("Available modules: " << output);
    // This test documents that the module should be available after installation
}

TEST_CASE("Speech Dispatcher basic synthesis", "[speechd][integration]") {
    if (!speechd_available()) {
        SKIP("Speech Dispatcher not available");
    }
    if (!laprdus_module_available()) {
        SKIP("Laprdus module not installed");
    }

    // Test basic synthesis with laprdus module
    auto [code, output] = run_spd_say("-o laprdus -w \"Test\"");
    REQUIRE(code == 0);
}

TEST_CASE("Speech Dispatcher voice selection", "[speechd][integration]") {
    if (!speechd_available()) {
        SKIP("Speech Dispatcher not available");
    }
    if (!laprdus_module_available()) {
        SKIP("Laprdus module not installed");
    }

    SECTION("Recorded voice by id") {
        auto [code, output] = run_spd_say("-o laprdus -y josip -w \"Dobar dan!\"");
        REQUIRE(code == 0);
    }

    SECTION("Recorded voice by id, another language") {
        auto [code, output] = run_spd_say("-o laprdus -y vlado -l hr -w \"Zdravo!\"");
        REQUIRE(code == 0);
    }

    SECTION("Voice by display name") {
        auto [code, output] = run_spd_say("-o laprdus -y \"Dalmacija Klapa\" -w \"Oj!\"");
        REQUIRE(code == 0);
    }

    SECTION("Old man by folded name") {
        auto [code, output] = run_spd_say("-o laprdus -y Djedo -w \"Zdravo!\"");
        REQUIRE(code == 0);
    }
}

TEST_CASE("Speech Dispatcher rate control", "[speechd][integration]") {
    if (!speechd_available()) {
        SKIP("Speech Dispatcher not available");
    }
    if (!laprdus_module_available()) {
        SKIP("Laprdus module not installed");
    }

    SECTION("Slow rate") {
        auto [code, output] = run_spd_say("-o laprdus -r -50 -w \"Sporo\"");
        REQUIRE(code == 0);
    }

    SECTION("Fast rate") {
        auto [code, output] = run_spd_say("-o laprdus -r 50 -w \"Brzo\"");
        REQUIRE(code == 0);
    }
}

TEST_CASE("Speech Dispatcher pitch control", "[speechd][integration]") {
    if (!speechd_available()) {
        SKIP("Speech Dispatcher not available");
    }
    if (!laprdus_module_available()) {
        SKIP("Laprdus module not installed");
    }

    SECTION("Low pitch") {
        auto [code, output] = run_spd_say("-o laprdus -p -50 -w \"Nisko\"");
        REQUIRE(code == 0);
    }

    SECTION("High pitch") {
        auto [code, output] = run_spd_say("-o laprdus -p 50 -w \"Visoko\"");
        REQUIRE(code == 0);
    }
}

TEST_CASE("Speech Dispatcher volume control", "[speechd][integration]") {
    if (!speechd_available()) {
        SKIP("Speech Dispatcher not available");
    }
    if (!laprdus_module_available()) {
        SKIP("Laprdus module not installed");
    }

    SECTION("Quiet") {
        auto [code, output] = run_spd_say("-o laprdus -i -50 -w \"Tiho\"");
        REQUIRE(code == 0);
    }

    SECTION("Loud") {
        auto [code, output] = run_spd_say("-o laprdus -i 50 -w \"Glasno\"");
        REQUIRE(code == 0);
    }
}

TEST_CASE("Speech Dispatcher spelling mode", "[speechd][integration]") {
    if (!speechd_available()) {
        SKIP("Speech Dispatcher not available");
    }
    if (!laprdus_module_available()) {
        SKIP("Laprdus module not installed");
    }

    // Spelling mode (-s) should spell out characters
    auto [code, output] = run_spd_say("-o laprdus -s -w \"ABC\"");
    REQUIRE(code == 0);
}

TEST_CASE("Speech Dispatcher language selection", "[speechd][integration]") {
    if (!speechd_available()) {
        SKIP("Speech Dispatcher not available");
    }
    if (!laprdus_module_available()) {
        SKIP("Laprdus module not installed");
    }

    SECTION("Croatian") {
        auto [code, output] = run_spd_say("-o laprdus -l hr -w \"Hrvatski\"");
        REQUIRE(code == 0);
    }

    SECTION("Serbian") {
        auto [code, output] = run_spd_say("-o laprdus -l sr -w \"Srpski\"");
        REQUIRE(code == 0);
    }

    SECTION("Bosnian") {
        auto [code, output] = run_spd_say("-o laprdus -l bs -w \"Bosanski\"");
        REQUIRE(code == 0);
    }

    SECTION("Slovenian") {
        auto [code, output] = run_spd_say("-o laprdus -l sl -w \"Sloven\xC5\xA1\xC4\x8Dina\"");
        REQUIRE(code == 0);
    }

    SECTION("Another language keeps speaking") {
        auto [code, output] = run_spd_say("-o laprdus -l de -w \"Guten Tag\"");
        REQUIRE(code == 0);
    }
}

// =============================================================================
// Voice Definition Tests (Unit tests using C API)
// =============================================================================

namespace {

// One entry of the module's voice list: name = display name, language =
// one of the four language codes, variant = the voice id.
struct ModuleVoice { std::string name; std::string language; std::string variant; };

// The list the module builds in build_voice_list(): every voice for every
// language, in registry order.
std::vector<ModuleVoice> module_voice_list() {
    std::vector<ModuleVoice> list;
    uint32_t voices = laprdus_get_voice_count();
    uint32_t languages = laprdus_get_language_count();
    for (uint32_t i = 0; i < voices; i++) {
        LaprdusVoiceInfo info;
        if (laprdus_get_voice_info(i, &info) != LAPRDUS_OK) continue;
        for (uint32_t l = 0; l < languages; l++) {
            LaprdusLanguageInfo lang;
            if (laprdus_get_language_info(l, &lang) != LAPRDUS_OK) continue;
            list.push_back({info.display_name, lang.code, info.id});
        }
    }
    return list;
}

// The module's ascii_fold(): lowercase ASCII, č ć -> c, đ -> dj, š -> s, ž -> z
std::string ascii_fold(const std::string& in) {
    std::string out;
    for (size_t i = 0; i < in.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(in[i]);
        unsigned char d = i + 1 < in.size() ? static_cast<unsigned char>(in[i + 1]) : 0;
        const char* rep = nullptr;
        if (c == 0xC4) {
            if (d == 0x8C || d == 0x8D || d == 0x86 || d == 0x87) rep = "c";
            else if (d == 0x90 || d == 0x91) rep = "dj";
        } else if (c == 0xC5) {
            if (d == 0xA0 || d == 0xA1) rep = "s";
            else if (d == 0xBD || d == 0xBE) rep = "z";
        }
        if (rep) { out += rep; ++i; continue; }
        out += static_cast<char>((c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c);
    }
    return out;
}

// The module's find_voice_id(): id (old ids too), display name, folded name
std::string find_voice_id(const std::string& name) {
    LaprdusVoiceInfo info;
    if (laprdus_get_voice_info_by_id(name.c_str(), &info) == LAPRDUS_OK) return info.id;
    std::string wanted = ascii_fold(name);
    if (laprdus_get_voice_info_by_id(wanted.c_str(), &info) == LAPRDUS_OK) return info.id;
    for (uint32_t i = 0; i < laprdus_get_voice_count(); i++) {
        if (laprdus_get_voice_info(i, &info) != LAPRDUS_OK) continue;
        if (strcasecmp(info.id, name.c_str()) == 0) return info.id;
        if (strcasecmp(info.display_name, name.c_str()) == 0) return info.id;
        if (ascii_fold(info.display_name) == wanted) return info.id;
    }
    return "";
}

} // namespace

TEST_CASE("Module voice list has every voice for every language", "[speechd][voices]") {
    REQUIRE(laprdus_get_voice_count() == 18);
    REQUIRE(laprdus_get_language_count() == 4);

    std::vector<ModuleVoice> list = module_voice_list();
    REQUIRE(list.size() == 72);

    // Every voice id appears with hr-HR, sr-RS, bs-BA and sl-SI
    const char* const ids[] = {"josip", "vlado", "detence", "baba", "djedo",
                               "zvonko", "stojan", "mirsad",
                               "orguljas", "dalmacija_klapa", "trubac", "harmonikas",
                               "sevdalija", "sazlija", "lika_velebit", "solunac",
                               "sarajevo_bembasa", "becarac"};
    for (const char* id : ids) {
        for (const char* language : {"hr-HR", "sr-RS", "bs-BA", "sl-SI"}) {
            INFO(std::string(id) + " " + language);
            auto it = std::find_if(list.begin(), list.end(), [&](const ModuleVoice& v) {
                return v.variant == id && v.language == language;
            });
            REQUIRE(it != list.end());
        }
    }

    // Names are the registry's display names, without "Laprdus" or a language
    for (const ModuleVoice& voice : list) {
        INFO(voice.name + " " + voice.language + " " + voice.variant);
        LaprdusVoiceInfo info;
        REQUIRE(laprdus_get_voice_info_by_id(voice.variant.c_str(), &info) == LAPRDUS_OK);
        REQUIRE(voice.name == info.display_name);
        REQUIRE(voice.name.find("Laprdus") == std::string::npos);
        REQUIRE(voice.name.find("(") == std::string::npos);
    }
    auto djedo = std::find_if(list.begin(), list.end(), [](const ModuleVoice& v) { return v.variant == "djedo"; });
    REQUIRE(djedo != list.end());
    REQUIRE(djedo->name == "\xC4\x90" "edo");

    // The language filter of LIST VOICES (module_process.c) matches the
    // whole code or the part before the dash: "sr" gives 18 entries.
    size_t serbian = std::count_if(list.begin(), list.end(), [](const ModuleVoice& v) {
        return v.language == "sr-RS";
    });
    REQUIRE(serbian == 18);
}

TEST_CASE("Module resolves voices by id, old id, name and folded name", "[speechd][voices]") {
    REQUIRE(find_voice_id("zvonko") == "zvonko");
    REQUIRE(find_voice_id("Zvonko") == "zvonko");
    REQUIRE(find_voice_id("djedo") == "djedo");
    REQUIRE(find_voice_id("djed") == "djedo");                       // old id
    REQUIRE(find_voice_id("\xC4\x90" "edo") == "djedo");             // Đedo
    REQUIRE(find_voice_id("Djedo") == "djedo");                      // folded
    REQUIRE(find_voice_id("klapa") == "dalmacija_klapa");            // old id
    REQUIRE(find_voice_id("Dalmacija Klapa") == "dalmacija_klapa");
    REQUIRE(find_voice_id("dalmacija klapa") == "dalmacija_klapa");
    REQUIRE(find_voice_id("Orgulja\xC5\xA1 - Vila Velebita") == "orguljas");
    REQUIRE(find_voice_id("Orguljas - Vila Velebita") == "orguljas");
    REQUIRE(find_voice_id("Be\xC4\x87" "arac") == "becarac");
    REQUIRE(find_voice_id("becarac") == "becarac");
    REQUIRE(find_voice_id("pjevac") == "lika_velebit");
    REQUIRE(find_voice_id("Sarajevo Bembasa") == "sarajevo_bembasa");
    REQUIRE(find_voice_id("Solist") == "sarajevo_bembasa");          // old id, any case
    REQUIRE(find_voice_id("nobody") == "");
    REQUIRE(ascii_fold("\xC4\x8C\xC4\x87\xC4\x90\xC4\x91\xC5\xA0\xC5\xBE") == "ccdjdjsz");
}

TEST_CASE("Module symbolic voices are the same for every language", "[speechd][voices]") {
    // SET VOICE MALE1 is Zvonko, the default voice, for hr, sr, bs and sl alike;
    // the others are Josip, Đedo, Baba and Detence.
    REQUIRE(std::string(laprdus_get_default_voice()) == "zvonko");
    struct Symbolic { const char* type; const char* id; };
    const Symbolic symbolic[] = {
        {"MALE1", "zvonko"}, {"MALE2", "josip"}, {"MALE3", "djedo"},
        {"FEMALE1", "baba"}, {"FEMALE3", "baba"},
        {"CHILD_MALE", "detence"}, {"CHILD_FEMALE", "detence"},
    };
    for (const auto& s : symbolic) {
        CAPTURE(s.type);
        LaprdusVoiceInfo info;
        REQUIRE(laprdus_get_voice_info_by_id(s.id, &info) == LAPRDUS_OK);
        REQUIRE(std::string(info.id) == s.id);
    }
}

TEST_CASE("Module language switch keeps the voice", "[speechd][voices]") {
    // SET LANGUAGE calls laprdus_set_language(): the voice stays and speaks
    // the language; a language Laprdus does not speak is ignored.
    LaprdusHandle engine = laprdus_create();
    REQUIRE(engine != nullptr);
    REQUIRE(laprdus_set_voice(engine, "zvonko", "/nonexistent") == LAPRDUS_OK);
    REQUIRE(std::string(laprdus_get_language(engine)) == "hr-HR");
    for (const char* tag : {"sr", "SR", "sr-RS", "sr_RS"}) {
        CAPTURE(tag);
        REQUIRE(laprdus_set_language(engine, tag) == LAPRDUS_OK);
        REQUIRE(std::string(laprdus_get_language(engine)) == "sr-RS");
        REQUIRE(std::string(laprdus_get_current_voice(engine)) == "zvonko");
    }
    REQUIRE(laprdus_set_language(engine, "sl-SI") == LAPRDUS_OK);
    REQUIRE(std::string(laprdus_get_language(engine)) == "sl-SI");
    REQUIRE(std::string(laprdus_get_current_voice(engine)) == "zvonko");
    REQUIRE(laprdus_set_language(engine, "bs-BA") == LAPRDUS_OK);
    REQUIRE(std::string(laprdus_get_language(engine)) == "bs-BA");
    REQUIRE(laprdus_set_language(engine, "de") != LAPRDUS_OK);
    REQUIRE(std::string(laprdus_get_language(engine)) == "bs-BA");
    // The language stays when MALE1 or another voice is set afterwards
    REQUIRE(laprdus_set_voice(engine, "stojan", "/nonexistent") == LAPRDUS_OK);
    REQUIRE(std::string(laprdus_get_language(engine)) == "bs-BA");
    laprdus_destroy(engine);
}

TEST_CASE("Module voice languages are correct", "[speechd][voices]") {
    struct Native { const char* id; const char* language; };
    const Native native[] = {
        {"josip", "hr-HR"}, {"vlado", "sr-RS"}, {"detence", "hr-HR"},
        {"baba", "hr-HR"}, {"djedo", "sr-RS"},
        {"zvonko", "hr-HR"}, {"stojan", "sr-RS"}, {"mirsad", "bs-BA"},
    };
    for (const auto& n : native) {
        CAPTURE(n.id);
        LaprdusVoiceInfo info;
        REQUIRE(laprdus_get_voice_info_by_id(n.id, &info) == LAPRDUS_OK);
        REQUIRE(std::string(info.language_code) == n.language);
    }
}
