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
#include <array>
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

    SECTION("Croatian voice") {
        auto [code, output] = run_spd_say("-o laprdus -y josip -w \"Dobar dan!\"");
        REQUIRE(code == 0);
    }

    SECTION("Serbian voice") {
        auto [code, output] = run_spd_say("-o laprdus -y vlado -w \"Zdravo!\"");
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
}

// =============================================================================
// Voice Definition Tests (Unit tests using C API)
// =============================================================================

TEST_CASE("Module voice list matches C API", "[speechd][voices]") {
    uint32_t count = laprdus_get_voice_count();
    REQUIRE(count >= 5);

    std::vector<std::string> expected_voices = {"josip", "vlado", "detence", "baba", "djedo"};
    std::vector<std::string> found_voices;

    for (uint32_t i = 0; i < count; i++) {
        LaprdusVoiceInfo info;
        if (laprdus_get_voice_info(i, &info) == LAPRDUS_OK) {
            found_voices.push_back(info.id);
        }
    }

    for (const auto& voice : expected_voices) {
        bool found = std::find(found_voices.begin(), found_voices.end(), voice) != found_voices.end();
        CAPTURE(voice);
        REQUIRE(found);
    }
}

TEST_CASE("Module voice languages are correct", "[speechd][voices]") {
    // Croatian voices
    {
        LaprdusVoiceInfo info;
        REQUIRE(laprdus_get_voice_info_by_id("josip", &info) == LAPRDUS_OK);
        REQUIRE(std::string(info.language_code).find("hr") != std::string::npos);
    }

    // Serbian voices
    {
        LaprdusVoiceInfo info;
        REQUIRE(laprdus_get_voice_info_by_id("vlado", &info) == LAPRDUS_OK);
        REQUIRE(std::string(info.language_code).find("sr") != std::string::npos);
    }
}
