// -*- coding: utf-8 -*-
/**
 * Formant voice tests (Zvonko, Stojan, Mirsad)
 * Uses the bundled minimal Catch-style header and the public C API only.
 *
 * Build: g++ -std=c++17 -I../../include test_formant.cpp -o test_formant -llaprdus -lpthread
 * Run: ./test_formant
 *
 * Formant voices are synthesized by rule, so none of these tests need voice
 * data. Only "switches between formant and concatenative voices" reads
 * LAPRDUS_DATA and is skipped when Josip.bin is not there.
 */

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include <laprdus/laprdus_api.h>

namespace {

// A directory that does not exist: formant voices must not look for files.
const char* const NO_DATA = "/nonexistent/laprdus-data";

const char* const FORMANT_VOICES[] = {"zvonko", "stojan", "mirsad"};

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

// True when a is within the given fraction of b.
bool close_to(double a, double b, double fraction) {
    return std::abs(a - b) <= std::abs(b) * fraction;
}

int peak(const std::vector<int16_t>& audio) {
    int max = 0;
    for (int16_t s : audio) max = std::max(max, std::abs(static_cast<int>(s)));
    return max;
}

// Silence inside the audio, between its first and last audible sample, in
// steps of 2 ms. Audible means above 0.5% of the peak.
struct Gaps {
    double longest_ms = 0.0;        // longest silent stretch
    double after_last_ms = 0.0;     // audible time after the last one of 20 ms or more
};

Gaps gaps(const std::vector<int16_t>& audio) {
    const size_t step = 44;
    const int floor = std::max(1, peak(audio) / 200);
    std::vector<bool> audible;
    for (size_t start = 0; start + step <= audio.size(); start += step) {
        int max = 0;
        for (size_t i = 0; i < step; ++i) {
            max = std::max(max, std::abs(static_cast<int>(audio[start + i])));
        }
        audible.push_back(max > floor);
    }
    size_t first = 0;
    size_t last = audible.size();
    while (first < last && !audible[first]) ++first;
    while (last > first && !audible[last - 1]) --last;

    Gaps result;
    size_t silent = 0;
    size_t after = 0;
    for (size_t i = first; i < last; ++i) {
        if (!audible[i]) {
            ++silent;
            continue;
        }
        if (silent >= 10) after = 0;
        result.longest_ms = std::max(result.longest_ms, static_cast<double>(silent) * 2.0);
        silent = 0;
        ++after;
    }
    result.after_last_ms = static_cast<double>(after) * 2.0;
    return result;
}

// Fundamental frequency of the loudest ~93 ms within [from, to) (fractions
// of the audio), by autocorrelation.
double pitch_hz(const std::vector<int16_t>& audio, double from = 0.0, double to = 1.0) {
    const size_t window = 2048;
    const size_t begin = static_cast<size_t>(static_cast<double>(audio.size()) * from);
    const size_t end = static_cast<size_t>(static_cast<double>(audio.size()) * to);
    if (end < begin + window) return 0.0;
    size_t best_start = begin;
    double best_energy = 0.0;
    for (size_t start = begin; start + window <= end; start += 256) {
        double energy = 0.0;
        for (size_t i = 0; i < window; ++i) {
            energy += static_cast<double>(audio[start + i]) * audio[start + i];
        }
        if (energy > best_energy) {
            best_energy = energy;
            best_start = start;
        }
    }
    double best_corr = 0.0;
    size_t best_lag = 0;
    for (size_t lag = 22050 / 400; lag <= 22050 / 50; ++lag) {
        double corr = 0.0;
        for (size_t i = 0; i + lag < window; ++i) {
            corr += static_cast<double>(audio[best_start + i]) * audio[best_start + i + lag];
        }
        corr /= static_cast<double>(window - lag);
        if (corr > best_corr) {
            best_corr = corr;
            best_lag = lag;
        }
    }
    return best_lag ? 22050.0 / static_cast<double>(best_lag) : 0.0;
}

} // namespace

// =============================================================================
// Registry
// =============================================================================

TEST_CASE("Formant voices are registered", "[formant][registry]") {
    struct Expected { const char* id; const char* language; uint16_t lcid; };
    const Expected expected[] = {
        {"zvonko", "hr-HR", 0x041A},
        {"stojan", "sr-RS", 0x081A},
        {"mirsad", "bs-BA", 0x141A},
    };

    for (const auto& e : expected) {
        LaprdusVoiceInfo info;
        REQUIRE(laprdus_get_voice_info_by_id(e.id, &info) == LAPRDUS_OK);
        REQUIRE(std::string(info.id) == e.id);
        REQUIRE(std::string(info.language_code) == e.language);
        REQUIRE(info.language_lcid == e.lcid);
        REQUIRE(std::string(info.gender) == "Male");
        REQUIRE(std::string(info.age) == "Adult");
        REQUIRE(info.base_pitch == 1.0f);
        REQUIRE(info.base_voice_id == nullptr);
        REQUIRE(info.data_filename == nullptr);     // nothing to load
    }
}

TEST_CASE("Existing voices keep their place in the registry", "[formant][registry]") {
    // Platform code and saved settings rely on the first five entries.
    const char* const order[] = {"josip", "vlado", "detence", "baba", "djed",
                                 "zvonko", "stojan", "mirsad"};
    REQUIRE(laprdus_get_voice_count() == 8);
    for (uint32_t i = 0; i < 8; ++i) {
        LaprdusVoiceInfo info;
        REQUIRE(laprdus_get_voice_info(i, &info) == LAPRDUS_OK);
        REQUIRE(std::string(info.id) == order[i]);
    }

    LaprdusVoiceInfo josip;
    REQUIRE(laprdus_get_voice_info_by_id("josip", &josip) == LAPRDUS_OK);
    REQUIRE(std::string(josip.data_filename) == "Josip.bin");
}

// =============================================================================
// Synthesis
// =============================================================================

TEST_CASE("Formant voices speak without any voice data", "[formant][synthesis]") {
    for (const char* voice : FORMANT_VOICES) {
        Engine engine;
        REQUIRE(laprdus_set_voice(engine.handle, voice, NO_DATA) == LAPRDUS_OK);
        REQUIRE(laprdus_is_initialized(engine.handle) != 0);
        REQUIRE(std::string(laprdus_get_current_voice(engine.handle)) == voice);

        std::vector<int16_t> audio = speak(engine.handle, "Dobar dan, kako ste?");
        double seconds = static_cast<double>(audio.size()) / 22050.0;

        // Seven syllables plus two pauses: clearly more than a blip, clearly
        // less than slow-motion speech.
        REQUIRE(seconds > 0.8);
        REQUIRE(seconds < 3.0);
        REQUIRE(rms(audio) > 800.0);        // audible
        REQUIRE(peak(audio) < 32000);       // not clipped
    }
}

TEST_CASE("Formant synthesis is deterministic", "[formant][synthesis]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    std::vector<int16_t> first = speak(engine.handle, "Sunce sja.");
    std::vector<int16_t> second = speak(engine.handle, "Sunce sja.");
    REQUIRE(!first.empty());
    REQUIRE(first == second);
}

TEST_CASE("The three formant voices are different speakers", "[formant][synthesis]") {
    double pitch[3];
    for (int i = 0; i < 3; ++i) {
        Engine engine;
        REQUIRE(laprdus_set_voice(engine.handle, FORMANT_VOICES[i], NO_DATA) == LAPRDUS_OK);
        REQUIRE(laprdus_set_inflection_enabled(engine.handle, 0) == LAPRDUS_OK);
        pitch[i] = pitch_hz(speak(engine.handle, "aaa"));
        REQUIRE(pitch[i] > 70.0);
        REQUIRE(pitch[i] < 180.0);
    }
    // Stojan is the lowest voice, Mirsad the highest.
    REQUIRE(pitch[1] < pitch[0] - 5.0);
    REQUIRE(pitch[2] > pitch[0] + 5.0);
}

TEST_CASE("Formant voices handle edge-case input", "[formant][synthesis]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);

    // Nothing speakable: no audio, no crash.
    REQUIRE(speak(engine.handle, "").empty());
    REQUIRE(speak(engine.handle, "...").empty());
    REQUIRE(speak(engine.handle, "   ").empty());

    // Still produces speech.
    REQUIRE(!speak(engine.handle, "a").empty());
    REQUIRE(!speak(engine.handle, "s").empty());            // letter name
    REQUIRE(!speak(engine.handle, "HTML").empty());         // spelled abbreviation
    REQUIRE(!speak(engine.handle, "prst").empty());         // syllabic r
    REQUIRE(!speak(engine.handle, "123").empty());          // number expansion
    REQUIRE(!speak(engine.handle, "\xC5\xBD\xC3\xA8na").empty());   // accent mark in text
    REQUIRE(!speak(engine.handle, "Hello world, xylophone & quartz!").empty());

    // A long text must not grow unbounded or fail.
    std::string long_text;
    for (int i = 0; i < 200; ++i) long_text += "Ovo je duga re\xC4\x8D" "enica broj jedan. ";
    std::vector<int16_t> audio = speak(engine.handle, long_text.c_str());
    REQUIRE(audio.size() > 22050u * 60u);
}

TEST_CASE("Cyrillic and Latin text sound the same", "[formant][text]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "stojan", NO_DATA) == LAPRDUS_OK);
    std::vector<int16_t> latin = speak(engine.handle, "Dobar dan");
    // "Добар дан"
    std::vector<int16_t> cyrillic = speak(engine.handle,
        "\xD0\x94\xD0\xBE\xD0\xB1\xD0\xB0\xD1\x80 \xD0\xB4\xD0\xB0\xD0\xBD");
    REQUIRE(!latin.empty());
    REQUIRE(latin == cyrillic);
}

TEST_CASE("Number words follow the voice language", "[formant][text]") {
    // Croatian "tisuću" and Serbian/Bosnian "hiljadu" are different words,
    // so the audio for "1000" must equal the audio for the spelled-out word.
    struct Case { const char* voice; const char* words; };
    const Case cases[] = {
        {"zvonko", "tisu\xC4\x87u"},
        {"stojan", "hiljadu"},
        {"mirsad", "hiljadu"},
    };
    for (const auto& c : cases) {
        Engine engine;
        REQUIRE(laprdus_set_voice(engine.handle, c.voice, NO_DATA) == LAPRDUS_OK);
        std::vector<int16_t> digits = speak(engine.handle, "1000");
        std::vector<int16_t> words = speak(engine.handle, c.words);
        REQUIRE(!digits.empty());
        REQUIRE(digits == words);
    }
}

// =============================================================================
// Parameters
// =============================================================================

TEST_CASE("Speech rate changes duration, not pitch", "[formant][params]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    REQUIRE(laprdus_set_inflection_enabled(engine.handle, 0) == LAPRDUS_OK);

    const char* text = "mama ima malu nanu";
    std::vector<int16_t> normal = speak(engine.handle, text);

    REQUIRE(laprdus_set_speed(engine.handle, 2.0f) == LAPRDUS_OK);
    std::vector<int16_t> fast = speak(engine.handle, text);

    REQUIRE(laprdus_set_speed(engine.handle, 0.5f) == LAPRDUS_OK);
    std::vector<int16_t> slow = speak(engine.handle, text);

    REQUIRE(fast.size() < normal.size() * 0.7);
    REQUIRE(slow.size() > normal.size() * 1.6);
    REQUIRE(close_to(pitch_hz(fast), pitch_hz(normal), 0.06));
    REQUIRE(close_to(pitch_hz(slow), pitch_hz(normal), 0.06));
}

TEST_CASE("Pitch settings change pitch, not duration", "[formant][params]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    REQUIRE(laprdus_set_inflection_enabled(engine.handle, 0) == LAPRDUS_OK);

    std::vector<int16_t> normal = speak(engine.handle, "aaa");
    double base = pitch_hz(normal);

    REQUIRE(laprdus_set_user_pitch(engine.handle, 1.5f) == LAPRDUS_OK);
    std::vector<int16_t> high = speak(engine.handle, "aaa");
    REQUIRE(close_to(pitch_hz(high), base * 1.5, 0.06));
    REQUIRE(high.size() == normal.size());

    REQUIRE(laprdus_set_user_pitch(engine.handle, 1.0f) == LAPRDUS_OK);
    REQUIRE(laprdus_set_pitch(engine.handle, 0.75f) == LAPRDUS_OK);
    std::vector<int16_t> low = speak(engine.handle, "aaa");
    REQUIRE(close_to(pitch_hz(low), base * 0.75, 0.06));
    REQUIRE(low.size() == normal.size());
}

TEST_CASE("Volume scales the output level", "[formant][params]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "mirsad", NO_DATA) == LAPRDUS_OK);

    std::vector<int16_t> full = speak(engine.handle, "Dobar dan");
    REQUIRE(laprdus_set_volume(engine.handle, 0.5f) == LAPRDUS_OK);
    std::vector<int16_t> half = speak(engine.handle, "Dobar dan");
    REQUIRE(laprdus_set_volume(engine.handle, 0.0f) == LAPRDUS_OK);
    std::vector<int16_t> silent = speak(engine.handle, "Dobar dan");

    REQUIRE(half.size() == full.size());
    REQUIRE(close_to(rms(half), rms(full) * 0.5, 0.1));
    REQUIRE(peak(silent) == 0);
}

TEST_CASE("Punctuation selects intonation", "[formant][prosody]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    // No trailing pause, so the end of the audio is the last syllable.
    REQUIRE(laprdus_set_sentence_pause(engine.handle, 0) == LAPRDUS_OK);

    // A statement ends low, a yes/no question ends high.
    const char* statement = "Ona ima novu lampu.";
    const char* question = "Ona ima novu lampu?";
    double statement_end = pitch_hz(speak(engine.handle, statement), 0.82, 1.0);
    double question_end = pitch_hz(speak(engine.handle, question), 0.82, 1.0);
    REQUIRE(statement_end > 50.0);
    REQUIRE(question_end > statement_end * 1.25);

    // With inflection off both stay on the same flat pitch.
    REQUIRE(laprdus_set_inflection_enabled(engine.handle, 0) == LAPRDUS_OK);
    statement_end = pitch_hz(speak(engine.handle, statement), 0.82, 1.0);
    question_end = pitch_hz(speak(engine.handle, question), 0.82, 1.0);
    REQUIRE(close_to(question_end, statement_end, 0.05));
}

// =============================================================================
// Stops
// =============================================================================

TEST_CASE("A stop after a fricative keeps its silent gap at high rates", "[formant][stops]") {
    // Without the gap "st", "sk", "št", "šk" are heard as the bare fricative.
    for (const char* voice : FORMANT_VOICES) {
        Engine engine;
        REQUIRE(laprdus_set_voice(engine.handle, voice, NO_DATA) == LAPRDUS_OK);
        REQUIRE(laprdus_set_speed(engine.handle, 2.0f) == LAPRDUS_OK);

        REQUIRE(gaps(speak(engine.handle, "asta")).longest_ms >= 14.0);
        REQUIRE(gaps(speak(engine.handle, "aska")).longest_ms >= 14.0);
        REQUIRE(gaps(speak(engine.handle, "a\xC5\xA1ta")).longest_ms >= 14.0);
        REQUIRE(gaps(speak(engine.handle, "a\xC5\xA1ka")).longest_ms >= 14.0);
        REQUIRE(gaps(speak(engine.handle, "asa")).longest_ms < 6.0);
    }
}

TEST_CASE("A voiceless stop before a pause is released audibly", "[formant][stops]") {
    // The release is all that is heard of a final stop: it has to outlast
    // the bare burst (about 16 ms).
    for (const char* voice : FORMANT_VOICES) {
        Engine engine;
        REQUIRE(laprdus_set_voice(engine.handle, voice, NO_DATA) == LAPRDUS_OK);

        REQUIRE(gaps(speak(engine.handle, "pat")).after_last_ms >= 30.0);
        REQUIRE(gaps(speak(engine.handle, "pak")).after_last_ms >= 30.0);
        REQUIRE(gaps(speak(engine.handle, "most")).after_last_ms >= 30.0);
    }
}

TEST_CASE("Spelling mode works with formant voices", "[formant][spelling]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);

    int16_t* samples = nullptr;
    LaprdusAudioFormat format;
    int32_t count = laprdus_synthesize_spelled(engine.handle, "abc", &samples, &format);
    REQUIRE(count > 0);
    REQUIRE(samples != nullptr);
    laprdus_free_buffer(samples);
}

TEST_CASE("Streaming API works with formant voices", "[formant][stream]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "stojan", NO_DATA) == LAPRDUS_OK);

    std::vector<int16_t> whole = speak(engine.handle, "Dobar dan. Kako ste?");

    LaprdusStreamHandle stream = laprdus_stream_begin(engine.handle, "Dobar dan. Kako ste?");
    REQUIRE(stream != nullptr);
    std::vector<int16_t> streamed;
    int16_t buffer[1024];
    for (int guard = 0; guard < 100000 && !laprdus_stream_is_complete(stream); ++guard) {
        int32_t got = laprdus_stream_read(stream, buffer, 1024);
        if (got <= 0) break;
        streamed.insert(streamed.end(), buffer, buffer + got);
    }
    laprdus_stream_destroy(stream);

    REQUIRE(streamed == whole);
}

// =============================================================================
// Voice switching
// =============================================================================

TEST_CASE("Switches between formant voices", "[formant][voices]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    std::vector<int16_t> zvonko = speak(engine.handle, "Dobar dan");

    REQUIRE(laprdus_set_voice(engine.handle, "mirsad", NO_DATA) == LAPRDUS_OK);
    std::vector<int16_t> mirsad = speak(engine.handle, "Dobar dan");
    REQUIRE(zvonko != mirsad);

    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    REQUIRE(speak(engine.handle, "Dobar dan") == zvonko);
}

TEST_CASE("Switches between formant and concatenative voices", "[formant][voices]") {
    const char* env = std::getenv("LAPRDUS_DATA");
    std::string data_dir = env ? env : "/usr/share/laprdus";
    if (!std::ifstream(data_dir + "/Josip.bin").good()) {
        SKIP("Josip.bin not found in LAPRDUS_DATA");
    }

    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "josip", data_dir.c_str()) == LAPRDUS_OK);
    std::vector<int16_t> josip = speak(engine.handle, "Dobar dan");
    REQUIRE(!josip.empty());

    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", data_dir.c_str()) == LAPRDUS_OK);
    std::vector<int16_t> zvonko = speak(engine.handle, "Dobar dan");
    REQUIRE(!zvonko.empty());
    REQUIRE(zvonko != josip);

    // Back again: the recorded voice must be fully restored.
    REQUIRE(laprdus_set_voice(engine.handle, "josip", data_dir.c_str()) == LAPRDUS_OK);
    REQUIRE(speak(engine.handle, "Dobar dan") == josip);

    // A missing data file fails for recorded voices only.
    REQUIRE(laprdus_set_voice(engine.handle, "vlado", NO_DATA) != LAPRDUS_OK);
}

TEST_CASE("Unknown voice is rejected", "[formant][voices]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonimir", NO_DATA) != LAPRDUS_OK);
    REQUIRE(laprdus_is_initialized(engine.handle) == 0);
}
