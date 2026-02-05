/*
 * test_cli.cpp - Unit tests for LaprdusTTS command-line interface
 *
 * These tests verify the CLI argument parsing and synthesis functionality.
 * Uses Catch2 testing framework for clear, readable tests.
 *
 * Build: g++ -std=c++17 -I../../include test_cli.cpp -o test_cli -llaprdus -lpthread
 * Run: ./test_cli
 */

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include <cstdlib>
#include <cstdio>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <memory>

/* LaprdusTTS C API */
#include <laprdus/laprdus_api.h>

/* Path to data directory (set by test runner or default) */
static const char* DATA_DIR = "/usr/share/laprdus";

/* Get CLI path from environment or default */
static std::string get_cli_path() {
    const char* env = std::getenv("LAPRDUS_CLI");
    return env ? env : "laprdus";
}

/* Get data directory from environment or default */
static std::string get_data_dir() {
    const char* env = std::getenv("LAPRDUS_DATA");
    return env ? env : DATA_DIR;
}

/* Helper to run CLI command and capture output */
std::pair<int, std::string> run_cli(const std::string& args) {
    std::string cmd = get_cli_path() + " -D " + get_data_dir() + " " + args + " 2>&1";
    std::array<char, 128> buffer;
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

/* Helper to check if a file exists */
bool file_exists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

/* Helper to get file size */
size_t file_size(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    return f.tellg();
}

// =============================================================================
// CLI Argument Parsing Tests
// =============================================================================

TEST_CASE("CLI displays help with -h", "[cli][help]") {
    auto [exit_code, output] = run_cli("-h");

    REQUIRE(exit_code == 0);
    REQUIRE(output.find("LaprdusTTS") != std::string::npos);
    REQUIRE(output.find("Usage") != std::string::npos);
    REQUIRE(output.find("--voice") != std::string::npos);
    REQUIRE(output.find("--speech-rate") != std::string::npos);
}

TEST_CASE("CLI displays help with --help", "[cli][help]") {
    auto [exit_code, output] = run_cli("--help");

    REQUIRE(exit_code == 0);
    REQUIRE(output.find("LaprdusTTS") != std::string::npos);
}

TEST_CASE("CLI lists voices with -l", "[cli][voices]") {
    auto [exit_code, output] = run_cli("-l");

    REQUIRE(exit_code == 0);
    REQUIRE(output.find("josip") != std::string::npos);
    REQUIRE(output.find("vlado") != std::string::npos);
    REQUIRE(output.find("hr-HR") != std::string::npos);
    REQUIRE(output.find("sr-RS") != std::string::npos);
}

TEST_CASE("CLI fails gracefully without text", "[cli][error]") {
    auto [exit_code, output] = run_cli("");

    REQUIRE(exit_code != 0);
    REQUIRE(output.find("No text") != std::string::npos);
}

TEST_CASE("CLI rejects invalid voice", "[cli][error]") {
    auto [exit_code, output] = run_cli("-v invalid_voice \"Test\"");

    REQUIRE(exit_code != 0);
    REQUIRE(output.find("Failed") != std::string::npos);
}

// =============================================================================
// CLI Output Tests
// =============================================================================

TEST_CASE("CLI generates WAV file with -o", "[cli][output]") {
    const char* test_file = "/tmp/laprdus_test_output.wav";

    // Remove old file if exists
    std::remove(test_file);

    auto [exit_code, output] = run_cli("-o " + std::string(test_file) + " \"Test\"");

    REQUIRE(exit_code == 0);
    REQUIRE(file_exists(test_file));

    // Check WAV file has content
    size_t size = file_size(test_file);
    REQUIRE(size > 44); // WAV header is 44 bytes minimum

    // Check WAV header
    std::ifstream wav(test_file, std::ios::binary);
    char header[4];
    wav.read(header, 4);
    REQUIRE(std::string(header, 4) == "RIFF");

    // Cleanup
    std::remove(test_file);
}

TEST_CASE("CLI reads from input file with -i", "[cli][input]") {
    const char* input_file = "/tmp/laprdus_test_input.txt";
    const char* output_file = "/tmp/laprdus_test_output2.wav";

    // Create input file
    {
        std::ofstream f(input_file);
        f << "Dobar dan!";
    }

    // Remove old output if exists
    std::remove(output_file);

    auto [exit_code, output] = run_cli("-i " + std::string(input_file) +
                                       " -o " + std::string(output_file));

    REQUIRE(exit_code == 0);
    REQUIRE(file_exists(output_file));
    REQUIRE(file_size(output_file) > 44);

    // Cleanup
    std::remove(input_file);
    std::remove(output_file);
}

// =============================================================================
// CLI Parameter Tests
// =============================================================================

TEST_CASE("CLI accepts valid speech rate", "[cli][params]") {
    const char* output_file = "/tmp/laprdus_test_rate.wav";
    std::remove(output_file);

    SECTION("Rate 0.5 (slow)") {
        auto [exit_code, output] = run_cli("-r 0.5 -o " + std::string(output_file) + " \"Test\"");
        REQUIRE(exit_code == 0);
    }

    SECTION("Rate 1.0 (normal)") {
        auto [exit_code, output] = run_cli("-r 1.0 -o " + std::string(output_file) + " \"Test\"");
        REQUIRE(exit_code == 0);
    }

    SECTION("Rate 2.0 (fast)") {
        auto [exit_code, output] = run_cli("-r 2.0 -o " + std::string(output_file) + " \"Test\"");
        REQUIRE(exit_code == 0);
    }

    std::remove(output_file);
}

TEST_CASE("CLI accepts valid pitch", "[cli][params]") {
    const char* output_file = "/tmp/laprdus_test_pitch.wav";
    std::remove(output_file);

    SECTION("Pitch 0.5 (low)") {
        auto [exit_code, output] = run_cli("-p 0.5 -o " + std::string(output_file) + " \"Test\"");
        REQUIRE(exit_code == 0);
    }

    SECTION("Pitch 2.0 (high)") {
        auto [exit_code, output] = run_cli("-p 2.0 -o " + std::string(output_file) + " \"Test\"");
        REQUIRE(exit_code == 0);
    }

    std::remove(output_file);
}

TEST_CASE("CLI accepts valid volume", "[cli][params]") {
    const char* output_file = "/tmp/laprdus_test_volume.wav";
    std::remove(output_file);

    SECTION("Volume 0.0 (silent)") {
        auto [exit_code, output] = run_cli("-V 0.0 -o " + std::string(output_file) + " \"Test\"");
        REQUIRE(exit_code == 0);
    }

    SECTION("Volume 1.0 (full)") {
        auto [exit_code, output] = run_cli("-V 1.0 -o " + std::string(output_file) + " \"Test\"");
        REQUIRE(exit_code == 0);
    }

    std::remove(output_file);
}

TEST_CASE("CLI accepts number mode flag", "[cli][params]") {
    const char* output_file = "/tmp/laprdus_test_digits.wav";
    std::remove(output_file);

    auto [exit_code, output] = run_cli("-d -o " + std::string(output_file) + " \"123\"");
    REQUIRE(exit_code == 0);
    REQUIRE(file_exists(output_file));

    std::remove(output_file);
}

TEST_CASE("CLI accepts pause settings", "[cli][params]") {
    const char* output_file = "/tmp/laprdus_test_pauses.wav";
    std::remove(output_file);

    auto [exit_code, output] = run_cli(
        "-c 100 -e 200 -x 150 -q 120 -n 300 "
        "-o " + std::string(output_file) + " \"Test, test. Test! Test? Test\\nTest\"");

    REQUIRE(exit_code == 0);
    REQUIRE(file_exists(output_file));

    std::remove(output_file);
}

// =============================================================================
// CLI Voice Selection Tests
// =============================================================================

TEST_CASE("CLI synthesizes with different voices", "[cli][voices]") {
    const char* output_file = "/tmp/laprdus_test_voice.wav";

    std::vector<std::string> voices = {"josip", "vlado", "detence", "baba", "djedo"};

    for (const auto& voice : voices) {
        DYNAMIC_SECTION(("Voice: " + voice).c_str()) {
            std::remove(output_file);

            auto [exit_code, output] = run_cli(
                "-v " + voice + " -o " + std::string(output_file) + " \"Test\"");

            REQUIRE(exit_code == 0);
            REQUIRE(file_exists(output_file));
            REQUIRE(file_size(output_file) > 44);
        }
    }

    std::remove(output_file);
}

// =============================================================================
// C API Unit Tests
// =============================================================================

TEST_CASE("C API creates engine", "[api]") {
    LaprdusHandle engine = laprdus_create();
    REQUIRE(engine != nullptr);
    laprdus_destroy(engine);
}

TEST_CASE("C API reports version", "[api]") {
    const char* version = laprdus_get_version();
    REQUIRE(version != nullptr);
    REQUIRE(strlen(version) > 0);
}

TEST_CASE("C API lists voices", "[api]") {
    uint32_t count = laprdus_get_voice_count();
    REQUIRE(count >= 5); // At least 5 voices

    LaprdusVoiceInfo info;
    REQUIRE(laprdus_get_voice_info(0, &info) == LAPRDUS_OK);
    REQUIRE(info.id != nullptr);
    REQUIRE(info.display_name != nullptr);
    REQUIRE(info.language_code != nullptr);
}

TEST_CASE("C API initializes with voice", "[api]") {
    LaprdusHandle engine = laprdus_create();
    REQUIRE(engine != nullptr);

    SECTION("Croatian voice (josip)") {
        LaprdusError err = laprdus_set_voice(engine, "josip", get_data_dir().c_str());
        REQUIRE(err == LAPRDUS_OK);
        REQUIRE(laprdus_is_initialized(engine) != 0);
    }

    SECTION("Serbian voice (vlado)") {
        LaprdusError err = laprdus_set_voice(engine, "vlado", get_data_dir().c_str());
        REQUIRE(err == LAPRDUS_OK);
        REQUIRE(laprdus_is_initialized(engine) != 0);
    }

    laprdus_destroy(engine);
}

TEST_CASE("C API synthesizes text", "[api]") {
    LaprdusHandle engine = laprdus_create();
    REQUIRE(engine != nullptr);

    LaprdusError err = laprdus_set_voice(engine, "josip", get_data_dir().c_str());
    REQUIRE(err == LAPRDUS_OK);

    int16_t* samples = nullptr;
    LaprdusAudioFormat format;

    int32_t num_samples = laprdus_synthesize(engine, "Dobar dan!", &samples, &format);

    REQUIRE(num_samples > 0);
    REQUIRE(samples != nullptr);
    REQUIRE(format.sample_rate == 22050);
    REQUIRE(format.bits_per_sample == 16);
    REQUIRE(format.channels == 1);

    laprdus_free_buffer(samples);
    laprdus_destroy(engine);
}

TEST_CASE("C API sets parameters", "[api]") {
    LaprdusHandle engine = laprdus_create();
    REQUIRE(engine != nullptr);

    laprdus_set_voice(engine, "josip", get_data_dir().c_str());

    SECTION("Speed") {
        REQUIRE(laprdus_set_speed(engine, 0.5f) == LAPRDUS_OK);
        REQUIRE(laprdus_set_speed(engine, 1.0f) == LAPRDUS_OK);
        REQUIRE(laprdus_set_speed(engine, 2.0f) == LAPRDUS_OK);
    }

    SECTION("Pitch") {
        REQUIRE(laprdus_set_user_pitch(engine, 0.5f) == LAPRDUS_OK);
        REQUIRE(laprdus_set_user_pitch(engine, 1.0f) == LAPRDUS_OK);
        REQUIRE(laprdus_set_user_pitch(engine, 2.0f) == LAPRDUS_OK);
    }

    SECTION("Volume") {
        REQUIRE(laprdus_set_volume(engine, 0.0f) == LAPRDUS_OK);
        REQUIRE(laprdus_set_volume(engine, 0.5f) == LAPRDUS_OK);
        REQUIRE(laprdus_set_volume(engine, 1.0f) == LAPRDUS_OK);
    }

    SECTION("Pauses") {
        REQUIRE(laprdus_set_sentence_pause(engine, 100) == LAPRDUS_OK);
        REQUIRE(laprdus_set_comma_pause(engine, 50) == LAPRDUS_OK);
        REQUIRE(laprdus_set_newline_pause(engine, 200) == LAPRDUS_OK);
    }

    SECTION("Number mode") {
        REQUIRE(laprdus_set_number_mode(engine, LAPRDUS_NUMBER_MODE_DIGIT) == LAPRDUS_OK);
        REQUIRE(laprdus_get_number_mode(engine) == LAPRDUS_NUMBER_MODE_DIGIT);

        REQUIRE(laprdus_set_number_mode(engine, LAPRDUS_NUMBER_MODE_WHOLE) == LAPRDUS_OK);
        REQUIRE(laprdus_get_number_mode(engine) == LAPRDUS_NUMBER_MODE_WHOLE);
    }

    laprdus_destroy(engine);
}

TEST_CASE("C API handles errors gracefully", "[api][error]") {
    SECTION("NULL handle") {
        REQUIRE(laprdus_set_speed(nullptr, 1.0f) == LAPRDUS_ERROR_INVALID_HANDLE);
        REQUIRE(laprdus_synthesize(nullptr, "test", nullptr, nullptr) < 0);
    }

    SECTION("Uninitialized engine") {
        LaprdusHandle engine = laprdus_create();

        int16_t* samples = nullptr;
        LaprdusAudioFormat format;
        int32_t result = laprdus_synthesize(engine, "test", &samples, &format);

        REQUIRE(result < 0);

        laprdus_destroy(engine);
    }

    SECTION("Invalid voice") {
        LaprdusHandle engine = laprdus_create();

        LaprdusError err = laprdus_set_voice(engine, "nonexistent", DATA_DIR);
        REQUIRE(err != LAPRDUS_OK);

        laprdus_destroy(engine);
    }
}
