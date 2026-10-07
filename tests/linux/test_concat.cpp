// -*- coding: utf-8 -*-
/**
 * Recorded (concatenative) voice tests: Josip, Vlado and the derived voices.
 * Uses the bundled minimal Catch-style header and the public C API only.
 *
 * Build: g++ -std=c++17 -I../../include test_concat.cpp -o test_concat -llaprdus -lpthread
 * Run: LAPRDUS_DATA=<dir with Josip.bin and Vlado.bin> ./test_concat
 *
 * Every test needs the voice data and is skipped without LAPRDUS_DATA.
 */

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <laprdus/laprdus_api.h>

namespace {

const char* data_dir() {
    const char* dir = std::getenv("LAPRDUS_DATA");
    return dir && *dir ? dir : nullptr;
}

struct Engine {
    LaprdusHandle handle = laprdus_create();
    ~Engine() { laprdus_destroy(handle); }
};

std::vector<int16_t> speak(LaprdusHandle engine, const char* text) {
    int16_t* samples = nullptr;
    LaprdusAudioFormat format;
    int32_t count = laprdus_synthesize(engine, text, &samples, &format);
    std::vector<int16_t> audio;
    if (count > 0 && samples) {
        REQUIRE(format.sample_rate == 22050);
        REQUIRE(format.bits_per_sample == 16);
        REQUIRE(format.channels == 1);
        audio.assign(samples, samples + count);
    }
    laprdus_free_buffer(samples);
    return audio;
}

double rms(const std::vector<int16_t>& audio) {
    if (audio.empty()) return 0.0;
    double sum = 0.0;
    for (int16_t s : audio) sum += static_cast<double>(s) * s;
    return std::sqrt(sum / static_cast<double>(audio.size()));
}

int peak(const std::vector<int16_t>& audio) {
    int max = 0;
    for (int16_t s : audio) max = std::max(max, std::abs(static_cast<int>(s)));
    return max;
}

bool close_to(double a, double b, double fraction) {
    return std::abs(a - b) <= std::abs(b) * fraction;
}

// Pitch (Hz) of a 30 ms stretch starting at a sample, by normalised
// autocorrelation; 0 if it is not periodic enough.
double pitch_at(const std::vector<int16_t>& audio, size_t start) {
    const int fs = 22050;
    const int window = 661;
    const int lag_min = fs / 400;
    const int lag_max = fs / 60;
    if (start + static_cast<size_t>(window + lag_max) > audio.size()) return 0.0;
    double best = 0.0;
    int best_lag = 0;
    for (int lag = lag_min; lag <= lag_max; ++lag) {
        double num = 0.0, e1 = 0.0, e2 = 0.0;
        for (int i = 0; i < window; ++i) {
            double a = audio[start + static_cast<size_t>(i)];
            double b = audio[start + static_cast<size_t>(i + lag)];
            num += a * b;
            e1 += a * a;
            e2 += b * b;
        }
        double r = num / std::sqrt(e1 * e2 + 1e-9);
        if (r > best) { best = r; best_lag = lag; }
    }
    if (best < 0.7 || best_lag == 0) return 0.0;
    // Take the shortest lag that correlates nearly as well (octave errors)
    for (int div = 3; div >= 2; --div) {
        int lag = best_lag / div;
        if (lag < lag_min) continue;
        double num = 0.0, e1 = 0.0, e2 = 0.0;
        for (int i = 0; i < window; ++i) {
            double a = audio[start + static_cast<size_t>(i)];
            double b = audio[start + static_cast<size_t>(i + lag)];
            num += a * b; e1 += a * a; e2 += b * b;
        }
        if (num / std::sqrt(e1 * e2 + 1e-9) >= 0.9 * best) return static_cast<double>(fs) / lag;
    }
    return static_cast<double>(fs) / best_lag;
}

// Median pitch over the audio (frames every 10 ms that are periodic)
double median_pitch(const std::vector<int16_t>& audio) {
    std::vector<double> values;
    for (size_t start = 0; start + 1200 < audio.size(); start += 220) {
        double p = pitch_at(audio, start);
        if (p > 0.0) values.push_back(p);
    }
    if (values.empty()) return 0.0;
    std::sort(values.begin(), values.end());
    return values[values.size() / 2];
}

// Median pitch of the last audible 120 ms
double final_pitch(const std::vector<int16_t>& audio) {
    const int floor = std::max(1, peak(audio) / 50);
    size_t end = audio.size();
    while (end > 0 && std::abs(static_cast<int>(audio[end - 1])) < floor) --end;
    if (end < 3000) return 0.0;
    std::vector<int16_t> tail(audio.begin() + static_cast<std::ptrdiff_t>(end - 2900), audio.begin() + static_cast<std::ptrdiff_t>(end));
    return median_pitch(tail);
}

} // namespace

// =============================================================================

TEST_CASE("Recorded voices speak", "[concat][synthesis]") {
    const char* dir = data_dir();
    if (!dir) return;
    Engine engine;
    const char* voices[] = {"josip", "vlado", "detence", "baba", "djed"};
    for (const char* voice : voices) {
        REQUIRE(laprdus_set_voice(engine.handle, voice, dir) == LAPRDUS_OK);
        std::vector<int16_t> audio = speak(engine.handle, "Dobar dan, kako ste?");
        INFO(voice);
        REQUIRE(audio.size() > 22050 / 2);
        REQUIRE(rms(audio) > 500.0);
        REQUIRE(peak(audio) <= 32767);
    }
}

TEST_CASE("Recorded voice synthesis is deterministic", "[concat][synthesis]") {
    const char* dir = data_dir();
    if (!dir) return;
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "josip", dir) == LAPRDUS_OK);
    std::vector<int16_t> a = speak(engine.handle, "Ponovljena rečenica.");
    std::vector<int16_t> b = speak(engine.handle, "Ponovljena rečenica.");
    REQUIRE(a == b);
}

TEST_CASE("The output starts and ends quietly", "[concat][synthesis]") {
    const char* dir = data_dir();
    if (!dir) return;
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "josip", dir) == LAPRDUS_OK);
    std::vector<int16_t> audio = speak(engine.handle, "ao");
    REQUIRE(audio.size() > 1000);
    const int limit = peak(audio) / 10;
    for (size_t i = 0; i < 22; ++i) {
        REQUIRE(std::abs(static_cast<int>(audio[i])) < limit);
        REQUIRE(std::abs(static_cast<int>(audio[audio.size() - 1 - i])) < limit);
    }
}

TEST_CASE("Rate changes the duration, not the pitch", "[concat][rate]") {
    const char* dir = data_dir();
    if (!dir) return;
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "vlado", dir) == LAPRDUS_OK);
    const char* text = "Ovo je rečenica za mjerenje brzine govora.";
    REQUIRE(laprdus_set_speed(engine.handle, 1.0f) == LAPRDUS_OK);
    std::vector<int16_t> normal = speak(engine.handle, text);
    double f0_normal = median_pitch(normal);
    REQUIRE(laprdus_set_speed(engine.handle, 2.0f) == LAPRDUS_OK);
    std::vector<int16_t> fast = speak(engine.handle, text);
    REQUIRE(laprdus_set_speed(engine.handle, 0.5f) == LAPRDUS_OK);
    std::vector<int16_t> slow = speak(engine.handle, text);

    // Pauses and the minimum durations of consonants do not scale fully
    double fast_ratio = static_cast<double>(fast.size()) / static_cast<double>(normal.size());
    double slow_ratio = static_cast<double>(slow.size()) / static_cast<double>(normal.size());
    REQUIRE(fast_ratio > 0.45);
    REQUIRE(fast_ratio < 0.70);
    REQUIRE(slow_ratio > 1.6);
    REQUIRE(slow_ratio < 2.2);

    REQUIRE(f0_normal > 70.0);
    REQUIRE(close_to(median_pitch(fast), f0_normal, 0.12));
    REQUIRE(close_to(median_pitch(slow), f0_normal, 0.12));
}

TEST_CASE("User pitch moves the pitch and keeps the duration", "[concat][pitch]") {
    const char* dir = data_dir();
    if (!dir) return;
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "josip", dir) == LAPRDUS_OK);
    REQUIRE(laprdus_set_inflection_enabled(engine.handle, 0) == LAPRDUS_OK);
    const char* text = "Mama ima lala.";
    std::vector<int16_t> normal = speak(engine.handle, text);
    REQUIRE(laprdus_set_user_pitch(engine.handle, 1.5f) == LAPRDUS_OK);
    std::vector<int16_t> high = speak(engine.handle, text);
    REQUIRE(laprdus_set_user_pitch(engine.handle, 0.7f) == LAPRDUS_OK);
    std::vector<int16_t> low = speak(engine.handle, text);

    double f0 = median_pitch(normal);
    REQUIRE(f0 > 100.0);
    REQUIRE(f0 < 180.0);
    REQUIRE(close_to(median_pitch(high), f0 * 1.5, 0.10));
    REQUIRE(close_to(median_pitch(low), f0 * 0.7, 0.10));
    REQUIRE(close_to(static_cast<double>(high.size()), static_cast<double>(normal.size()), 0.03));
    REQUIRE(close_to(static_cast<double>(low.size()), static_cast<double>(normal.size()), 0.03));
}

TEST_CASE("Derived voices differ from their base voice in pitch", "[concat][pitch]") {
    const char* dir = data_dir();
    if (!dir) return;
    Engine engine;
    REQUIRE(laprdus_set_inflection_enabled(engine.handle, 0) == LAPRDUS_OK);
    const char* text = "Mama ima lala.";
    REQUIRE(laprdus_set_voice(engine.handle, "josip", dir) == LAPRDUS_OK);
    double josip = median_pitch(speak(engine.handle, text));
    REQUIRE(laprdus_set_voice(engine.handle, "detence", dir) == LAPRDUS_OK);
    double detence = median_pitch(speak(engine.handle, text));
    REQUIRE(laprdus_set_voice(engine.handle, "vlado", dir) == LAPRDUS_OK);
    double vlado = median_pitch(speak(engine.handle, text));
    REQUIRE(laprdus_set_voice(engine.handle, "djed", dir) == LAPRDUS_OK);
    double djed = median_pitch(speak(engine.handle, text));
    REQUIRE(josip > 0.0);
    REQUIRE(vlado > 0.0);
    REQUIRE(close_to(detence, josip * 1.5, 0.12));
    REQUIRE(close_to(djed, vlado * 0.75, 0.12));
}

TEST_CASE("A question ends higher than a statement", "[concat][intonation]") {
    const char* dir = data_dir();
    if (!dir) return;
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "josip", dir) == LAPRDUS_OK);
    double statement = final_pitch(speak(engine.handle, "Ona ima novi auto."));
    double question = final_pitch(speak(engine.handle, "Ona ima novi auto?"));
    REQUIRE(statement > 0.0);
    REQUIRE(question > statement * 1.15);

    // Without inflection both end at the voice's own pitch
    REQUIRE(laprdus_set_inflection_enabled(engine.handle, 0) == LAPRDUS_OK);
    double flat_statement = final_pitch(speak(engine.handle, "Ona ima novi auto."));
    double flat_question = final_pitch(speak(engine.handle, "Ona ima novi auto?"));
    REQUIRE(close_to(flat_statement, flat_question, 0.05));
}

TEST_CASE("Streaming gives the same audio as one buffer", "[concat][streaming]") {
    const char* dir = data_dir();
    if (!dir) return;
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "josip", dir) == LAPRDUS_OK);
    const char* text = "Prva rečenica. Druga, s zarezom, rečenica!";
    std::vector<int16_t> whole = speak(engine.handle, text);

    std::vector<int16_t> streamed;
    LaprdusStreamHandle stream = laprdus_stream_begin(engine.handle, text);
    REQUIRE(stream != nullptr);
    int16_t chunk[1000];
    while (true) {
        int32_t got = laprdus_stream_read(stream, chunk, 1000);
        REQUIRE(got >= 0);
        if (got == 0) break;
        streamed.insert(streamed.end(), chunk, chunk + got);
    }
    REQUIRE(laprdus_stream_is_complete(stream) != 0);
    laprdus_stream_destroy(stream);
    REQUIRE(streamed == whole);
}

TEST_CASE("Spelling works with a recorded voice", "[concat][spelling]") {
    const char* dir = data_dir();
    if (!dir) return;
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "vlado", dir) == LAPRDUS_OK);
    int16_t* samples = nullptr;
    LaprdusAudioFormat format;
    int32_t count = laprdus_synthesize_spelled(engine.handle, "ab", &samples, &format);
    REQUIRE(count > 2000);
    std::vector<int16_t> audio(samples, samples + count);
    laprdus_free_buffer(samples);
    REQUIRE(rms(audio) > 300.0);
}

TEST_CASE("Volume scales the output", "[concat][volume]") {
    const char* dir = data_dir();
    if (!dir) return;
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "josip", dir) == LAPRDUS_OK);
    std::vector<int16_t> loud = speak(engine.handle, "Glasno i tiho.");
    REQUIRE(laprdus_set_volume(engine.handle, 0.25f) == LAPRDUS_OK);
    std::vector<int16_t> quiet = speak(engine.handle, "Glasno i tiho.");
    REQUIRE(quiet.size() == loud.size());
    REQUIRE(rms(quiet) < rms(loud) * 0.4);
}
