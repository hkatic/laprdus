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
#include "spelling_checks.hpp"

TEST_CASE("Recorded Slovenian spelling uses Slovenian names", "[concat][spelling]") {
    const char* data = std::getenv("LAPRDUS_DATA");
    if (!data || !*data) return;
    LaprdusHandle engine = laprdus_create();
    REQUIRE(laprdus_set_voice(engine, "josip", data) == LAPRDUS_OK);
    check_slovenian_spelling(engine);
    laprdus_destroy(engine);
}

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
    const char* voices[] = {"josip", "vlado", "detence", "baba", "djedo"};
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

TEST_CASE("A word of one vowel is a syllable of its own", "[concat][rate]") {
    // i, a, u, o keep a full short vowel and are set off from a vowel
    // next to them by the word gap ("ja i ona" was "jajona").
    const char* dir = data_dir();
    if (!dir) return;
    Engine engine;
    auto ms = [](const std::vector<int16_t>& audio) {
        return static_cast<double>(audio.size()) * 1000.0 / 22050.0;
    };
    for (const char* voice : {"josip", "vlado"}) {
        REQUIRE(laprdus_set_voice(engine.handle, voice, dir) == LAPRDUS_OK);
        INFO(voice);
        REQUIRE(ms(speak(engine.handle, "Ja i ona.")) - ms(speak(engine.handle, "Ja ona.")) >= 90.0);
        REQUIRE(ms(speak(engine.handle, "Idemo u Vukovar.")) -
                ms(speak(engine.handle, "Idemo Vukovar.")) >= 85.0);
    }
}

TEST_CASE("User pitch moves the pitch and keeps the duration", "[concat][pitch]") {
    const char* dir = data_dir();
    if (!dir) return;
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "josip", dir) == LAPRDUS_OK);
    REQUIRE(laprdus_set_inflection_level(engine.handle, 0.0f) == LAPRDUS_OK);
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
    REQUIRE(laprdus_set_inflection_level(engine.handle, 0.0f) == LAPRDUS_OK);
    const char* text = "Mama ima lala.";
    REQUIRE(laprdus_set_voice(engine.handle, "josip", dir) == LAPRDUS_OK);
    double josip = median_pitch(speak(engine.handle, text));
    REQUIRE(laprdus_set_voice(engine.handle, "detence", dir) == LAPRDUS_OK);
    double detence = median_pitch(speak(engine.handle, text));
    REQUIRE(laprdus_set_voice(engine.handle, "vlado", dir) == LAPRDUS_OK);
    double vlado = median_pitch(speak(engine.handle, text));
    REQUIRE(laprdus_set_voice(engine.handle, "djedo", dir) == LAPRDUS_OK);
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

    // At inflection level 0 both end at the voice's own pitch
    REQUIRE(laprdus_set_inflection_level(engine.handle, 0.0f) == LAPRDUS_OK);
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

TEST_CASE("A period inside a word or a number is read by name", "[concat][text]") {
    // The recorded voices share the front end, so "datoteka.txt" is spoken
    // as "datoteka točka txt" (tačka for Vlado) and "3.14" as a decimal.
    const char* dir = data_dir();
    if (!dir) return;
    Engine engine;
    auto same = [&](const char* a, const char* b) { return speak(engine.handle, a) == speak(engine.handle, b); };

    REQUIRE(laprdus_set_voice(engine.handle, "josip", dir) == LAPRDUS_OK);
    REQUIRE(same("datoteka.txt", "datoteka to\xC4\x8Dka txt"));
    REQUIRE(same("3.14", "tri to\xC4\x8Dka \xC4\x8D" "etrnaest"));
    REQUIRE_FALSE(same("Kraj. Novi", "Kraj Novi"));

    REQUIRE(laprdus_set_voice(engine.handle, "vlado", dir) == LAPRDUS_OK);
    REQUIRE(same("datoteka.txt", "datoteka ta\xC4\x8Dka txt"));
    REQUIRE(same("3,14", "tri zarez \xC4\x8D" "etrnaest"));
}

TEST_CASE("Number words follow the chosen language for recorded voices", "[concat][language]") {
    // Josip and Vlado speak every language too: the language chosen with
    // laprdus_set_language() picks the number words (tisuću / hiljadu),
    // stays across voice changes, and NULL returns to the voice's own.
    const char* dir = data_dir();
    if (!dir) SKIP("Set LAPRDUS_DATA to test the recorded voices' languages");
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "josip", dir) == LAPRDUS_OK);
    REQUIRE(std::string(laprdus_get_language(engine.handle)) == "hr-HR");
    std::vector<int16_t> croatian = speak(engine.handle, "1000");
    REQUIRE(!croatian.empty());
    REQUIRE(croatian == speak(engine.handle, "tisu\xC4\x87u"));

    REQUIRE(laprdus_set_language(engine.handle, "sr") == LAPRDUS_OK);
    REQUIRE(std::string(laprdus_get_language(engine.handle)) == "sr-RS");
    REQUIRE(std::string(laprdus_get_current_voice(engine.handle)) == "josip");
    REQUIRE(speak(engine.handle, "1000") == speak(engine.handle, "hiljadu"));
    REQUIRE(speak(engine.handle, "2000") == speak(engine.handle, "dve hiljade"));
    REQUIRE(speak(engine.handle, "1000") != croatian);

    REQUIRE(laprdus_set_language(engine.handle, "bs") == LAPRDUS_OK);
    REQUIRE(std::string(laprdus_get_language(engine.handle)) == "bs-BA");
    REQUIRE(speak(engine.handle, "2000") == speak(engine.handle, "dvije hiljade"));

    // The chosen language stays when the voice changes ...
    REQUIRE(laprdus_set_voice(engine.handle, "vlado", dir) == LAPRDUS_OK);
    REQUIRE(std::string(laprdus_get_language(engine.handle)) == "bs-BA");
    // ... and Vlado speaking Croatian says tisuću
    REQUIRE(laprdus_set_language(engine.handle, "hr-HR") == LAPRDUS_OK);
    REQUIRE(std::string(laprdus_get_language(engine.handle)) == "hr-HR");
    std::vector<int16_t> vlado_croatian = speak(engine.handle, "1000");
    REQUIRE(vlado_croatian == speak(engine.handle, "tisu\xC4\x87u"));
    REQUIRE(vlado_croatian != speak(engine.handle, "hiljadu"));

    // NULL: back to Vlado's native Serbian, also after later voice changes
    REQUIRE(laprdus_set_language(engine.handle, nullptr) == LAPRDUS_OK);
    REQUIRE(std::string(laprdus_get_language(engine.handle)) == "sr-RS");
    REQUIRE(speak(engine.handle, "1000") == speak(engine.handle, "hiljadu"));
    REQUIRE(laprdus_set_voice(engine.handle, "detence", dir) == LAPRDUS_OK);
    REQUIRE(std::string(laprdus_get_language(engine.handle)) == "hr-HR");
    REQUIRE(speak(engine.handle, "1000") == speak(engine.handle, "tisu\xC4\x87u"));
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

TEST_CASE("Spelling speed adds silence without slowing recorded speech", "[concat][spelling]") {
    const char* dir = data_dir();
    if (!dir) SKIP("Set LAPRDUS_DATA to test recorded spelling pauses");
    for (const char* voice : {"josip", "vlado"}) {
        INFO(voice);
        Engine engine;
        REQUIRE(laprdus_set_voice(engine.handle, voice, dir) == LAPRDUS_OK);
        check_spelling_pauses(engine.handle);
    }
}

TEST_CASE("Letter sounds work with a recorded voice", "[concat][spelling]") {
    const char* dir = data_dir();
    if (!dir) return;
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "josip", dir) == LAPRDUS_OK);
    REQUIRE(laprdus_set_spelling_pause(engine.handle, 0) == LAPRDUS_OK);

    auto spell = [&](const char* text) {
        int16_t* samples = nullptr;
        LaprdusAudioFormat format;
        int32_t count = laprdus_synthesize_spelled(engine.handle, text, &samples, &format);
        std::vector<int16_t> audio;
        if (count > 0 && samples) audio.assign(samples, samples + count);
        laprdus_free_buffer(samples);
        return audio;
    };

    // Names at the speech rate are the names spoken (sounds are the default)
    REQUIRE(laprdus_set_spelling_mode(engine.handle, LAPRDUS_SPELLING_LETTER_NAMES) == LAPRDUS_OK);
    REQUIRE(laprdus_set_spelling_speed(engine.handle, 100) == LAPRDUS_OK);
    std::vector<int16_t> name = spell("b");
    REQUIRE(name == speak(engine.handle, "be"));
    REQUIRE(spell("j") == speak(engine.handle, "je"));

    // Sounds: every letter gives audible sound, different from its name
    REQUIRE(laprdus_set_spelling_mode(engine.handle, LAPRDUS_SPELLING_LETTER_SOUNDS) == LAPRDUS_OK);
    REQUIRE(laprdus_set_spelling_speed(engine.handle, 100) == LAPRDUS_OK);
    REQUIRE(spell("b") != name);
    const char* letters[] = {
        "a", "b", "c", "\xC4\x8D", "\xC4\x87", "d", "\xC4\x91", "e", "f", "g", "h", "i",
        "j", "k", "l", "m", "n", "o", "p", "r", "s", "\xC5\xA1", "t", "u", "v", "z",
        "\xC5\xBE", "\xC7\x89", "\xD1\x9F",
    };
    for (const char* letter : letters) {
        INFO(letter);
        std::vector<int16_t> audio = spell(letter);
        REQUIRE(audio.size() > 22050 / 25);
        REQUIRE(rms(audio) > 150.0);
    }

    // The foreign letters keep their names (iks, ipsilon)
    REQUIRE(spell("x") == speak(engine.handle, "iks"));
    REQUIRE(spell("Y") == speak(engine.handle, "ipsilon"));

    // A screen reader's "veliko N" spells the letter by its sound too
    std::vector<int16_t> capital = speak(engine.handle, "veliko");
    std::vector<int16_t> sound = spell("N");
    capital.insert(capital.end(), sound.begin(), sound.end());
    REQUIRE(speak(engine.handle, "veliko N") == capital);
    REQUIRE(speak(engine.handle, "veliko N") != speak(engine.handle, "veliko en"));

    // A letter alone in running text ("Obrisano B") is its sound too, an
    // audible syllable; a preposition stays one
    std::vector<int16_t> deleted = speak(engine.handle, "Obrisano B");
    REQUIRE(deleted != speak(engine.handle, "Obrisano be"));
    REQUIRE(deleted.size() > speak(engine.handle, "Obrisano").size() + 22050 / 10);
    std::vector<int16_t> with = speak(engine.handle, "s njom");

    // In name mode it is the name
    REQUIRE(laprdus_set_spelling_mode(engine.handle, LAPRDUS_SPELLING_LETTER_NAMES) == LAPRDUS_OK);
    REQUIRE(speak(engine.handle, "Obrisano B") == speak(engine.handle, "Obrisano be"));
    REQUIRE(speak(engine.handle, "s njom") == with);

    // Vlado spells with Serbian names
    REQUIRE(laprdus_set_voice(engine.handle, "vlado", dir) == LAPRDUS_OK);
    REQUIRE(spell("\xC5\xA1") == speak(engine.handle, "\xC5\xA1" "a"));
}

TEST_CASE("Head size and brightness shape the recorded voices", "[concat][params]") {
    // Head size lengthens or shortens the vocal tract through the bank warp,
    // so the formants move and the pitch stays; brightness tilts the top
    // around 1.5 kHz. 50 is the recording as it is.
    const char* dir = data_dir();
    if (!dir) return;
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "josip", dir) == LAPRDUS_OK);
    REQUIRE(laprdus_get_head_size(engine.handle) == 50);
    REQUIRE(laprdus_get_brightness(engine.handle) == 50);
    std::vector<int16_t> plain = speak(engine.handle, "Pozivam Marka na mobitel.");

    // The share of the energy above about 1.5 kHz (a one-pole high-pass).
    auto top_share = [](const std::vector<int16_t>& audio) {
        double low = 0.0, top = 0.0, total = 0.0;
        const double coef = std::exp(-2.0 * 3.14159265 * 1500.0 / 22050.0);
        for (int16_t s : audio) {
            low = coef * low + (1.0 - coef) * s;
            const double hp = s - low;
            top += hp * hp;
            total += double(s) * s;
        }
        return total > 0.0 ? top / total : 0.0;
    };

    REQUIRE(laprdus_set_head_size(engine.handle, 100) == LAPRDUS_OK);
    std::vector<int16_t> big = speak(engine.handle, "Pozivam Marka na mobitel.");
    // The bank is resampled, so the length moves by a few periods' worth;
    // the pitch does not (the formants do: see "Voice quality settings" in
    // docs/formant.md for the measured spectra).
    REQUIRE(std::abs(double(big.size()) - double(plain.size())) < 0.02 * double(plain.size()));
    REQUIRE(big != plain);
    const double plain_pitch = median_pitch(plain);
    REQUIRE(std::abs(median_pitch(big) - plain_pitch) < 0.03 * plain_pitch);

    REQUIRE(laprdus_set_head_size(engine.handle, 50) == LAPRDUS_OK);
    REQUIRE(speak(engine.handle, "Pozivam Marka na mobitel.") == plain);

    REQUIRE(laprdus_set_brightness(engine.handle, 0) == LAPRDUS_OK);
    std::vector<int16_t> dull = speak(engine.handle, "Pozivam Marka na mobitel.");
    REQUIRE(dull.size() == plain.size());
    REQUIRE(top_share(dull) < top_share(plain) * 0.7);
    REQUIRE(laprdus_set_brightness(engine.handle, 100) == LAPRDUS_OK);
    REQUIRE(top_share(speak(engine.handle, "Pozivam Marka na mobitel.")) > top_share(plain));
    REQUIRE(laprdus_set_brightness(engine.handle, 50) == LAPRDUS_OK);
    REQUIRE(speak(engine.handle, "Pozivam Marka na mobitel.") == plain);
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

namespace {

// Hiss and true peak (the hiss limiter of boost mode and the brightness tilt,
// November 2026).

// One second-order Butterworth section at 3 kHz (bilinear, as the Audio EQ
// Cookbook gives it), high- or low-pass.
std::vector<double> butterworth_3k(const std::vector<int16_t>& x, bool highpass) {
    const double w0 = 2.0 * 3.14159265358979 * 3000.0 / 22050.0;
    const double cw = std::cos(w0), alpha = std::sin(w0) / 2.0 * std::sqrt(2.0), a0 = 1.0 + alpha;
    const double b0 = (highpass ? (1.0 + cw) : (1.0 - cw)) / 2.0 / a0;
    const double b1 = (highpass ? -(1.0 + cw) : (1.0 - cw)) / a0;
    const double a1 = -2.0 * cw / a0, a2 = (1.0 - alpha) / a0;
    std::vector<double> y(x.size());
    double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;
    for (size_t i = 0; i < x.size(); ++i) {
        const double v = b0 * x[i] + b1 * x1 + b0 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = x[i]; y2 = y1; y1 = v;
        y[i] = v;
    }
    return y;
}

// The hiss of two renderings of the same item, measured on the 12 ms frames
// where the first one's band above 3 kHz outweighs the rest (a sibilant, not
// a vowel's own top): the loudest such frame's band above 3 kHz, each re its
// own loudest frame, in dB.
std::pair<double, double> hiss_pair_db(const std::vector<int16_t>& plain, const std::vector<int16_t>& other) {
    const std::vector<double> ph = butterworth_3k(plain, true), pl = butterworth_3k(plain, false);
    const std::vector<double> oh = butterworth_3k(other, true);
    const size_t n = 265, hop = 88, len = std::min(plain.size(), other.size());
    double top_p = 1e-9, top_o = 1e-9, hiss_p = 1e-9, hiss_o = 1e-9;
    for (size_t s = 0; s + n <= len; s += hop) {
        double ep = 0.0, eo = 0.0, hp = 0.0, lp = 0.0, ho = 0.0;
        for (size_t i = s; i < s + n; ++i) {
            ep += double(plain[i]) * plain[i];
            eo += double(other[i]) * other[i];
            hp += ph[i] * ph[i];
            lp += pl[i] * pl[i];
            ho += oh[i] * oh[i];
        }
        top_p = std::max(top_p, ep);
        top_o = std::max(top_o, eo);
        if (hp > lp) {
            hiss_p = std::max(hiss_p, hp);
            hiss_o = std::max(hiss_o, ho);
        }
    }
    return {10.0 * std::log10(hiss_p / top_p), 10.0 * std::log10(hiss_o / top_o)};
}

// The true peak: the largest magnitude of the signal 8 times oversampled by a
// 64-tap Hann-windowed sinc (what a phone's resampler reconstructs).
double true_peak(const std::vector<int16_t>& x) {
    const int taps = 32;
    double best = 0.0;
    for (int16_t v : x) best = std::max(best, std::abs(double(v)));
    for (int f = 1; f < 8; ++f) {
        const double frac = f / 8.0;
        double w[2 * taps];
        for (int k = -taps + 1; k <= taps; ++k) {
            const double t = k - frac;
            const double sinc = std::abs(t) < 1e-12 ? 1.0 : std::sin(3.14159265358979 * t) / (3.14159265358979 * t);
            const double hann = 0.5 + 0.5 * std::cos(3.14159265358979 * t / taps);
            w[k + taps - 1] = sinc * hann;
        }
        for (size_t i = taps; i + taps < x.size(); ++i) {
            double v = 0.0;
            for (int k = -taps + 1; k <= taps; ++k) v += w[k + taps - 1] * x[i + static_cast<size_t>(k)];
            best = std::max(best, std::abs(v));
        }
    }
    return best;
}

// The median level of the vowel frames (12 ms, within 30 dB of the loudest,
// their band under 3 kHz ten times the band above it), in dB.
double vowel_level_db(const std::vector<int16_t>& x) {
    const std::vector<double> h = butterworth_3k(x, true), l = butterworth_3k(x, false);
    const size_t n = 265;
    double top = 0.0;
    for (size_t s = 0; s + n <= x.size(); s += n) {
        double e = 0.0;
        for (size_t i = s; i < s + n; ++i) e += double(x[i]) * x[i];
        top = std::max(top, e);
    }
    std::vector<double> levels;
    for (size_t s = 0; s + n <= x.size(); s += n) {
        double e = 0.0, eh = 0.0, el = 0.0;
        for (size_t i = s; i < s + n; ++i) {
            e += double(x[i]) * x[i];
            eh += h[i] * h[i];
            el += l[i] * l[i];
        }
        if (e > top * 1e-3 && el > 10.0 * eh) levels.push_back(10.0 * std::log10(e / n + 1e-9));
    }
    std::sort(levels.begin(), levels.end());
    return levels.empty() ? -200.0 : levels[levels.size() / 2];
}

// What of `full` is not a scaled copy of `half`: the residual after the best
// gain over every 2 ms (a limiter's gain is constant there, a clipper's
// waveshaping is not), re `full`, in dB.
double waveshaping_db(const std::vector<int16_t>& full, const std::vector<int16_t>& half) {
    const size_t n = std::min(full.size(), half.size()), m = 44;
    double res = 0.0, total = 0.0;
    for (size_t s = 0; s + m <= n; s += m) {
        double xy = 0.0, xx = 0.0;
        for (size_t i = s; i < s + m; ++i) {
            xy += double(half[i]) * full[i];
            xx += double(half[i]) * half[i];
        }
        const double g = xx > 0.0 ? xy / xx : 0.0;
        for (size_t i = s; i < s + m; ++i) {
            const double r = full[i] - g * half[i];
            res += r * r;
            total += double(full[i]) * full[i];
        }
    }
    return 10.0 * std::log10(res / (total + 1e-9) + 1e-15);
}

} // namespace

TEST_CASE("The volume boost makes the recorded voices louder without clipping", "[concat][volume]") {
    // The recordings already touch full scale and are compressed, so a
    // limiter that does not clip gains little here: the vowels +1.1 dB for
    // Josip (the boost of 2.0 gave +5 dB by rounding the peaks off, at 10 %
    // distortion). Since November 2026 the hiss limiter takes his s, as loud
    // as his vowels, 18 dB down, so the overall level is no measure of the
    // boost any more: the vowels still rise and nothing clips.
    const char* dir = data_dir();
    if (!dir) return;
    Engine engine;
    for (const char* voice : {"josip", "baba"}) {
        REQUIRE(laprdus_set_voice(engine.handle, voice, dir) == LAPRDUS_OK);
        REQUIRE(laprdus_set_volume_boost(engine.handle, 0) == LAPRDUS_OK);
        std::vector<int16_t> plain = speak(engine.handle, "Pozivam Marka na mobitel.");
        REQUIRE(laprdus_set_volume_boost(engine.handle, 1) == LAPRDUS_OK);
        std::vector<int16_t> boosted = speak(engine.handle, "Pozivam Marka na mobitel.");
        REQUIRE(boosted.size() == plain.size());
        INFO("vowels: plain " << vowel_level_db(plain) << " dB, boosted " << vowel_level_db(boosted) << " dB");
        REQUIRE(vowel_level_db(boosted) > vowel_level_db(plain) + 0.5);
        REQUIRE(peak(boosted) <= 32000);
        REQUIRE(speak(engine.handle, "Pozivam Marka na mobitel.") == boosted);
        // Nothing over full scale between the samples (Josip's boosted speech
        // reached +1.4 dBFS there, November 2026), and the recorded s, whose
        // hiss band is as loud as the vowel in the recording, sits about 18 dB
        // under it (the plain output holds it about 8 dB under).
        INFO("true peak " << 20.0 * std::log10(true_peak(boosted) / 32768.0) << " dBFS");
        REQUIRE(true_peak(boosted) < 32768.0 * std::pow(10.0, -0.2 / 20.0));
        REQUIRE(laprdus_set_volume_boost(engine.handle, 0) == LAPRDUS_OK);
        const std::vector<int16_t> s_plain = speak(engine.handle, "asa");
        REQUIRE(laprdus_set_volume_boost(engine.handle, 1) == LAPRDUS_OK);
        const std::vector<int16_t> s_boosted = speak(engine.handle, "asa");
        const auto hiss = hiss_pair_db(s_plain, s_boosted);
        INFO("hiss of asa re the vowel: plain " << hiss.first << " dB, boosted " << hiss.second << " dB");
        REQUIRE(hiss.second < hiss.first - 5.0);   // the plain s is held too, 10 dB higher
        // The volume scales the boosted output
        REQUIRE(laprdus_set_volume(engine.handle, 0.5f) == LAPRDUS_OK);
        std::vector<int16_t> half = speak(engine.handle, "Pozivam Marka na mobitel.");
        REQUIRE(rms(half) < rms(boosted) * 0.6);
        REQUIRE(laprdus_set_volume(engine.handle, 1.0f) == LAPRDUS_OK);
    }
}

TEST_CASE("The hiss of the recorded voices is held under the vowels", "[concat][volume]") {
    // In the recordings s, c, č and ć stand level with the vowels (Josip's s
    // and ć -0.8 and -0.6 dB re the loudest frame, Vlado's š and č -4.8 and
    // -2.3), and weighted by the ear's sensitivity above them, where the
    // softest of four commercial recorded Croatian voices has them 4-8 dB
    // under. The plain
    // output holds the hiss RECORDED_HISS_DB under the nearby vowel (November
    // 2026), and the result is the same every time.
    const char* dir = data_dir();
    if (!dir) return;
    Engine engine;
    struct Item { const char* voice; const char* text; };
    const Item items[] = {
        {"josip", "asa"}, {"josip", u8"aća"}, {"josip", "aca"}, {"baba", "asa"},
        {"vlado", u8"aša"}, {"vlado", u8"ača"},
    };
    for (const Item& item : items) {
        REQUIRE(laprdus_set_voice(engine.handle, item.voice, dir) == LAPRDUS_OK);
        const std::vector<int16_t> audio = speak(engine.handle, item.text);
        const double hiss = hiss_pair_db(audio, audio).first;
        INFO(item.voice << " " << item.text << ": hiss re the loudest frame " << hiss << " dB");
        REQUIRE(hiss < -6.0);                   // measured -8.6 to -10.7
        REQUIRE(hiss > -16.0);                  // held, not removed
        REQUIRE(speak(engine.handle, item.text) == audio);
    }
}

TEST_CASE("The recorded voices do not clip at full volume", "[concat][volume]") {
    // The recordings touch full scale, and at full volume Vlado and Đedo drove
    // the soft knee of the renderer so hard that above 5 kHz half of Đedo's
    // sound was its distortion (November 2026): the waveshaping residual of a
    // paragraph -31 dB for Đedo, -38 for Vlado, -45 for Josip. A limiter at
    // unity gain now holds the peaks under the knee after the volume
    // (-51, -55, -55 dB measured; the true peak -0.6 to -0.9 dBTP), and an
    // utterance at half volume is what it was.
    const char* dir = data_dir();
    if (!dir) return;
    const char* text = "Dobar dan, kako ste? Danas je lijep dan, pa smo otišli u šetnju uz rijeku.";
    Engine engine;
    for (const char* voice : {"djedo", "vlado", "josip"}) {
        INFO(voice);
        REQUIRE(laprdus_set_voice(engine.handle, voice, dir) == LAPRDUS_OK);
        REQUIRE(laprdus_set_volume(engine.handle, 1.0f) == LAPRDUS_OK);
        const std::vector<int16_t> full = speak(engine.handle, text);
        REQUIRE(laprdus_set_volume(engine.handle, 0.5f) == LAPRDUS_OK);
        const std::vector<int16_t> half = speak(engine.handle, text);
        REQUIRE(full.size() == half.size());
        INFO("waveshaping " << waveshaping_db(full, half) << " dB, true peak "
             << 20.0 * std::log10(true_peak(full) / 32768.0) << " dBFS");
        REQUIRE(waveshaping_db(full, half) < -45.0);
        REQUIRE(true_peak(full) < 32768.0 * std::pow(10.0, -0.2 / 20.0));
        REQUIRE(peak(full) <= static_cast<int>(32767 * 0.9f) + 1);   // under the knee
    }
    REQUIRE(laprdus_set_volume(engine.handle, 1.0f) == LAPRDUS_OK);
}

TEST_CASE("Recorded voices share surname stress and vowel length", "[concat][text]") {
    const char* dir = data_dir();
    if (!dir) SKIP("Set LAPRDUS_DATA to test the recorded surname accents");
    struct Case { const char* plain; const char* accented; };
    const Case cases[] = {
        {u8"Isaković", u8"Ìsaković"},
        {u8"Isakovića", u8"Ìsakovića"},
        {u8"Isakovićevima", u8"Ìsakovićevima"},
        {u8"Исаковић", u8"Ìsaković"},
        {u8"Spasojević", u8"Spàsojević"},
        {u8"Spasojevićem", u8"Spàsojevićem"},
        {u8"Spasojevićevima", u8"Spàsojevićevima"},
        {u8"Спасојевић", u8"Spàsojević"},
        {u8"Isaković i Spasojević", u8"Ìsaković i Spàsojević"},
        {u8"Knezović", u8"Knȇzović"},
        {u8"Knezovića", u8"Knȇzovića"},
        {u8"Knezovićevima", u8"Knȇzovićevima"},
        {u8"Кнезовић", u8"Knȇzović"},
        {u8"Knežević", u8"Knéžević"},
        {u8"Kneževićem", u8"Knéževićem"},
        {u8"Kneževićevima", u8"Knéževićevima"},
        {u8"Кнежевић", u8"Knéžević"},
        {u8"Knezović i Knežević", u8"Knȇzović i Knéžević"},
    };
    Engine engine;
    for (const char* voice : {"josip", "vlado", "detence", "baba", "djedo"}) {
        INFO(voice);
        REQUIRE(laprdus_set_voice(engine.handle, voice, dir) == LAPRDUS_OK);
        for (const auto& c : cases) {
            INFO(c.plain);
            const auto plain = speak(engine.handle, c.plain);
            REQUIRE(!plain.empty());
            REQUIRE(plain == speak(engine.handle, c.accented));
        }
        // Initial stress must not return to a; changing only a's length
        // must make the output longer. The same check protects short o.
        REQUIRE(speak(engine.handle, u8"Isaković") !=
                speak(engine.handle, u8"Isàković"));
        REQUIRE(speak(engine.handle, u8"Isaković").size() <
                speak(engine.handle, u8"Ìsāković").size());
        REQUIRE(speak(engine.handle, u8"Spasojević").size() <
                speak(engine.handle, u8"Spàsōjević").size());
        REQUIRE(speak(engine.handle, u8"Spasojević") !=
                speak(engine.handle, u8"Spasòjević"));
        REQUIRE(speak(engine.handle, u8"Spasojević") !=
                speak(engine.handle, u8"Spasójević"));
        REQUIRE(speak(engine.handle, u8"Knezović").size() >
                speak(engine.handle, u8"Knȅzović").size());
        REQUIRE(speak(engine.handle, u8"Knežević").size() >
                speak(engine.handle, u8"Knèžević").size());
        REQUIRE(speak(engine.handle, u8"Knezović") !=
                speak(engine.handle, u8"Knézović"));
        REQUIRE(speak(engine.handle, u8"Knežević") !=
                speak(engine.handle, u8"Knȇžević"));
    }
}

TEST_CASE("Recorded voices pause between telephone groups", "[concat][phone]") {
    if (!data_dir()) return;
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "josip", data_dir()) == LAPRDUS_OK);
    REQUIRE(laprdus_set_number_mode(engine.handle, LAPRDUS_NUMBER_MODE_DIGIT) == LAPRDUS_OK);
    const auto spoken = speak(engine.handle, "00385 91/123-4567");
    REQUIRE(!spoken.empty());
    REQUIRE(spoken == speak(engine.handle,
            "nula nula tri osam pet, devet jedan, jedan dva tri, četiri pet šest sedam"));
    REQUIRE(laprdus_set_comma_pause(engine.handle, 0) == LAPRDUS_OK);
    const auto without = speak(engine.handle, "00385911234567");
    REQUIRE(laprdus_set_comma_pause(engine.handle, 100) == LAPRDUS_OK);
    const auto with = speak(engine.handle, "00385911234567");
    REQUIRE(with.size() == without.size() + 3 * 2205);
    // The plus sign taken by a screen reader: the same groups
    REQUIRE(speak(engine.handle, " 385 91 123 4567  ") ==
            speak(engine.handle, "tri osam pet, devet jedan, jedan dva tri, četiri pet šest sedam"));
}

// The short pauses of a clock time, a numeric date and a cell's address (a
// listener's report of November 2026; see test_formant.cpp): the recorded
// voices render the parts of the clause one after another as well.
TEST_CASE("Recorded voices pause inside a time and before a cell's address", "[concat][breaks]") {
    if (!data_dir()) return;
    Engine engine;
    for (const char* voice : {"josip", "vlado", "detence"}) {
        INFO(voice);
        REQUIRE(laprdus_set_voice(engine.handle, voice, data_dir()) == LAPRDUS_OK);
        const auto cell = speak(engine.handle, "Marjan A17");
        REQUIRE(!cell.empty());
        REQUIRE(cell == speak(engine.handle, "Marjan \xEF\xB7\x92 A\xEF\xB7\x91 sedamnaest"));
        REQUIRE(cell != speak(engine.handle, "Marjan A sedamnaest"));
        // NVDA in Excel: the address as two words, two spaces after every item
        REQUIRE(speak(engine.handle, "Marjan   A 17  ") == cell);
        // Samples of pause at a comma pause of 100 ms: half of it, 1102
        // samples, inside a time or a date, the whole before a cell's address
        struct Case { const char* text; size_t samples; };
        const Case cases[] = {
            {"Marjan A17", 2205}, {"12:30:45", 2 * 1102}, {"10.11.2026.", 2 * 1102}, {"Dobar dan svima", 0},
        };
        for (const Case& c : cases) {
            INFO(c.text);
            REQUIRE(laprdus_set_comma_pause(engine.handle, 0) == LAPRDUS_OK);
            const auto without = speak(engine.handle, c.text);
            REQUIRE(laprdus_set_comma_pause(engine.handle, 100) == LAPRDUS_OK);
            const auto with = speak(engine.handle, c.text);
            REQUIRE(with.size() == without.size() + c.samples);
        }
        REQUIRE(speak(engine.handle, "10.11.2026.") == speak(engine.handle, "10. 11. 2026."));
    }
}

TEST_CASE("Recorded voices speak currency signs and localized amounts", "[concat][currency]") {
    if (!data_dir()) return;
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "josip", data_dir()) == LAPRDUS_OK);
    const auto expected = speak(engine.handle, "minus dvije tisuće petsto zarez nula nula eura");
    REQUIRE(!expected.empty());
    REQUIRE(speak(engine.handle, "−2.500,00 €") == expected);
    REQUIRE(speak(engine.handle, "Stanje $2,150.00") ==
            speak(engine.handle, "Stanje dvije tisuće sto pedeset zarez nula nula dolara"));
}
