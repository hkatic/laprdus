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

double energy_at(const std::vector<int16_t>& audio, size_t start, size_t window) {
    double energy = 0.0;
    for (size_t i = 0; i < window; ++i) {
        energy += static_cast<double>(audio[start + i]) * audio[start + i];
    }
    return energy;
}

// Fundamental frequency of one window, by normalized autocorrelation. Takes
// the shortest period that correlates nearly as well as the best one (a
// local maximum within 10% of it), so a moving pitch, or a window that
// takes in the voice bar of a b, is not mistaken for its octave below.
double pitch_at(const std::vector<int16_t>& audio, size_t start, size_t window) {
    const size_t min_lag = 22050 / 400;
    const size_t max_lag = std::min<size_t>(22050 / 50, window - 1);
    std::vector<double> corr(max_lag + 2, 0.0);
    double best = 0.0;
    for (size_t lag = min_lag - 1; lag <= max_lag + 1 && lag < window; ++lag) {
        double sum = 0.0, left = 0.0, right = 0.0;
        for (size_t i = 0; i + lag < window; ++i) {
            double a = audio[start + i];
            double b = audio[start + i + lag];
            sum += a * b;
            left += a * a;
            right += b * b;
        }
        corr[lag] = left > 0.0 && right > 0.0 ? sum / std::sqrt(left * right) : 0.0;
        if (lag >= min_lag && lag <= max_lag) best = std::max(best, corr[lag]);
    }
    if (best <= 0.0) return 0.0;
    for (size_t lag = min_lag; lag <= max_lag; ++lag) {
        if (corr[lag] >= 0.9 * best && corr[lag] >= corr[lag - 1] && corr[lag] >= corr[lag + 1]) {
            return 22050.0 / static_cast<double>(lag);
        }
    }
    return 0.0;
}

// Fundamental frequency of the loudest ~93 ms within [from, to) (fractions
// of the audio).
double pitch_hz(const std::vector<int16_t>& audio, double from = 0.0, double to = 1.0) {
    const size_t window = 2048;
    const size_t begin = static_cast<size_t>(static_cast<double>(audio.size()) * from);
    const size_t end = static_cast<size_t>(static_cast<double>(audio.size()) * to);
    if (end < begin + window) return 0.0;
    size_t best_start = begin;
    double best_energy = 0.0;
    for (size_t start = begin; start + window <= end; start += 256) {
        double energy = energy_at(audio, start, window);
        if (energy > best_energy) {
            best_energy = energy;
            best_start = start;
        }
    }
    return pitch_at(audio, best_start, window);
}

// Where the pitch is highest, in ms from the start: the highest of the
// 40 ms windows (10 ms apart) loud and periodic enough to be a vowel.
double pitch_peak_ms(const std::vector<int16_t>& audio) {
    const size_t window = 882;
    const size_t hop = 220;
    std::vector<double> energy;
    double loudest = 0.0;
    for (size_t start = 0; start + window <= audio.size(); start += hop) {
        energy.push_back(energy_at(audio, start, window));
        loudest = std::max(loudest, energy.back());
    }
    double highest = 0.0;
    double at = -1.0;
    for (size_t k = 0; k < energy.size(); ++k) {
        if (energy[k] < 0.2 * loudest) continue;
        const double f0 = pitch_at(audio, k * hop, window);
        if (f0 <= 0.0) continue;
        // A consonant's noise has no period to repeat.
        const size_t lag = static_cast<size_t>(22050.0 / f0 + 0.5);
        double sum = 0.0, left = 0.0, right = 0.0;
        for (size_t i = k * hop; i + lag < k * hop + window; ++i) {
            sum += static_cast<double>(audio[i]) * audio[i + lag];
            left += static_cast<double>(audio[i]) * audio[i];
            right += static_cast<double>(audio[i + lag]) * audio[i + lag];
        }
        if (sum < 0.5 * std::sqrt(left * right)) continue;
        if (f0 > highest) {
            highest = f0;
            at = static_cast<double>(k * hop + window / 2) * 1000.0 / 22050.0;
        }
    }
    return at;
}

// Fundamental frequency where the voice ends: the last ~35 ms that are
// clearly periodic and not yet faded out. Takes the shortest period that
// correlates nearly as well as the best one, so a moving pitch is not
// mistaken for its octave below. The search stays in the last stretch of
// sound, where the most periodic window is taken if none is periodic
// enough. With a 46 ms window a short final vowel falling fast at 2x fell
// under the threshold, and the search went on into the words before it
// (Mirsad's "Tko je to." read 133 Hz where the voice ends at 110 Hz).
double final_pitch_hz(const std::vector<int16_t>& audio) {
    const size_t window = 768;
    const size_t min_lag = 22050 / 300;
    const size_t max_lag = 22050 / 60;
    if (audio.size() < window) return 0.0;
    double loudest = 0.0;
    for (size_t start = 0; start + window <= audio.size(); start += 128) {
        loudest = std::max(loudest, energy_at(audio, start, window));
    }

    std::vector<double> corr(max_lag + 2, 0.0);
    bool in_sound = false;
    double fallback_hz = 0.0;
    double fallback_corr = 0.0;
    for (size_t start = audio.size() - window; start >= 128; start -= 128) {
        if (energy_at(audio, start, window) < 0.03 * loudest) {
            if (in_sound) break;
            continue;
        }
        in_sound = true;
        double best = 0.0;
        for (size_t lag = min_lag - 1; lag <= max_lag + 1; ++lag) {
            double sum = 0.0;
            double left = 0.0;
            double right = 0.0;
            for (size_t i = 0; i + lag < window; ++i) {
                double a = audio[start + i];
                double b = audio[start + i + lag];
                sum += a * b;
                left += a * a;
                right += b * b;
            }
            corr[lag] = left > 0.0 && right > 0.0 ? sum / std::sqrt(left * right) : 0.0;
            if (lag >= min_lag && lag <= max_lag) best = std::max(best, corr[lag]);
        }
        double hz = 0.0;
        for (size_t lag = min_lag; lag <= max_lag; ++lag) {
            if (corr[lag] >= 0.9 * best && corr[lag] >= corr[lag - 1] &&
                corr[lag] >= corr[lag + 1]) {
                hz = 22050.0 / static_cast<double>(lag);
                break;
            }
        }
        if (best >= 0.7) return hz;
        if (best > fallback_corr) {
            fallback_corr = best;
            fallback_hz = hz;
        }
    }
    return fallback_hz;
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
                                 "zvonko", "stojan", "mirsad",
                                 "orguljas", "klapa", "trubac", "harmonikas",
                                 "sevdalija", "sazlija", "pjevac", "pevac", "solist",
                                 "becarac"};
    REQUIRE(laprdus_get_voice_count() == 18);
    for (uint32_t i = 0; i < 18; ++i) {
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
    // One and two agree with the feminine "tisuća"/"hiljada" (dvije tisuće,
    // dve hiljade, dvadeset jedna tisuća), 11-19 take the genitive plural.
    struct Case { const char* voice; const char* digits; const char* words; };
    const Case cases[] = {
        {"zvonko", "1000", "tisu\xC4\x87u"},
        {"stojan", "1000", "hiljadu"},
        {"mirsad", "1000", "hiljadu"},
        {"zvonko", "2000", "dvije tisu\xC4\x87" "e"},
        {"stojan", "2000", "dve hiljade"},
        {"mirsad", "2000", "dvije hiljade"},
        {"zvonko", "22000", "dvadeset dvije tisu\xC4\x87" "e"},
        {"stojan", "22000", "dvadeset dve hiljade"},
        {"mirsad", "22000", "dvadeset dvije hiljade"},
        {"zvonko", "220000", "dvjesto dvadeset tisu\xC4\x87" "a"},
        {"zvonko", "21000", "dvadeset jedna tisu\xC4\x87" "a"},
        {"zvonko", "12000", "dvanaest tisu\xC4\x87" "a"},
        {"zvonko", "2000000", "dva milijuna"},
        {"zvonko", "2000000000", "dvije milijarde"},
    };
    for (const auto& c : cases) {
        Engine engine;
        REQUIRE(laprdus_set_voice(engine.handle, c.voice, NO_DATA) == LAPRDUS_OK);
        std::vector<int16_t> digits = speak(engine.handle, c.digits);
        std::vector<int16_t> words = speak(engine.handle, c.words);
        REQUIRE(!digits.empty());
        REQUIRE(digits == words);
    }
}

TEST_CASE("Lexicon accents cover the whole paradigm", "[formant][text]") {
    // A word from the lexicon must sound like the same word with its accent
    // written out, in the oblique cases as well as in the dictionary form.
    struct Case { const char* plain; const char* accented; };
    const Case cases[] = {
        {"obavijest", "\xC8\x8D" "bavijest"},   // ȍbavijest
        {"obavijesti", "\xC8\x8D" "bavijesti"},   // ȍbavijesti
        {"obavijestima", "\xC8\x8D" "bavijestima"},   // ȍbavijestima
        {"obavijestiti", "obavij\xC3\xA9stiti"},   // obavijéstiti
        {"obavijestio", "obavij\xC3\xA9stio"},   // obavijéstio
        {"obavijestim", "obavij\xC3\xA9st\xC4\xABm"},   // obavijéstīm
        {"dodatno", "d\xC8\x8D" "datno"},   // dȍdatno
        {"dodatnih", "d\xC8\x8D" "datnih"},   // dȍdatnih
        {"mogu\xC4\x87nost", "mog\xC3\xBA\xC4\x87n\xC5\x8Dst"},   // mogúćnōst
        {"mogu\xC4\x87nosti", "mog\xC3\xBA\xC4\x87nosti"},   // mogúćnosti
        {"mogu\xC4\x87nostima", "mog\xC3\xBA\xC4\x87nostima"},   // mogúćnostima
        {"kapacitet", "kapacit\xC3\xA9t"},   // kapacitét
        {"kapaciteta", "kapacit\xC3\xA9ta"},   // kapacitéta
        {"kapacitetima", "kapacit\xC3\xA9tima"},   // kapacitétima
        {"kontrolni", "k\xC3\xB2ntrolni"},   // kòntrolni
        {"kontrolnog", "k\xC3\xB2ntrolnog"},   // kòntrolnog
        {"podatak", "pod\xC3\xA1tak"},   // podátak
        {"podaci", "pod\xC3\xA1" "ci"},   // podáci
        {"podacima", "pod\xC3\xA1" "cima"},   // podácima
        {"podatke", "pod\xC3\xA1tke"},   // podátke
        {"signal", "s\xC3\xACgnal"},   // sìgnal
        {"signala", "sign\xC3\xA1la"},   // signála
        {"signalu", "sign\xC3\xA1lu"},   // signálu
        {"signalom", "sign\xC3\xA1lom"},   // signálom
        {"signale", "sign\xC3\xA1le"},   // signále
        {"pozadina", "p\xC3\xB2zadina"},   // pòzadina
        {"pozadinu", "p\xC3\xB2zadinu"},   // pòzadinu
        {"privatnost", "priv\xC3\xA1tn\xC5\x8Dst"},   // privátnōst
        {"privatnosti", "priv\xC3\xA1tnosti"},   // privátnosti
        {"sigurnost", "sig\xC3\xBArn\xC5\x8Dst"},   // sigúrnōst
        {"sigurno\xC5\xA1\xC4\x87u", "sig\xC3\xBArno\xC5\xA1\xC4\x87u"},   // sigúrnošću
        {"nov\xC4\x8D" "anik", "nov\xC4\x8D\xC3\xA0n\xC4\xABk"},   // novčànīk
        {"nov\xC4\x8D" "anici", "nov\xC4\x8D" "an\xC3\xAD" "ci"},   // novčaníci
        {"nov\xC4\x8D" "anika", "nov\xC4\x8D" "an\xC3\xADka"},   // novčaníka
        {"proslijediti", "proslij\xC3\xA9" "diti"},   // proslijéditi
        {"proslijedi", "proslij\xC3\xA9" "di"},   // proslijédi
        {"proslijedio", "proslij\xC3\xA9" "dio"},   // proslijédio
        {"proslijedim", "pr\xC3\xB2slijedim"},   // pròslijedim
        {"proslijedite", "proslij\xC3\xA9" "dite"},   // proslijédite
        {"proslije\xC4\x91" "eno", "pr\xC3\xB2slije\xC4\x91" "eno"},   // pròslijeđeno
        {"proslje\xC4\x91ujem", "proslj\xC3\xA8\xC4\x91ujem"},   // prosljèđujem
        {"proslje\xC4\x91ivati", "proslje\xC4\x91\xC3\xADvati"},   // prosljeđívati
        {"podijeli", "podij\xC3\xA9li"},   // podijéli
        {"podijelio", "podij\xC3\xA9lio"},   // podijélio
        {"podijelim", "p\xC3\xB2" "dijelim"},   // pòdijelim
        {"podijeljeno", "p\xC3\xB2" "dijeljeno"},   // pòdijeljeno
        {"raspodijeli", "raspodij\xC3\xA9li"},   // raspodijéli
        {"promijeni", "promij\xC3\xA9ni"},   // promijéni
        {"zalijepi", "zalij\xC3\xA9pi"},   // zalijépi
        {"primijeni", "primij\xC3\xA9ni"},   // primijéni
        {"zamijenjen", "zamij\xC3\xA9njen"},   // zamijénjen
        {"upotrijebi", "upotrij\xC3\xA9" "bi"},   // upotrijébi
        {"primijetio", "primij\xC3\xA9tio"},   // primijétio
        {"pomije\xC5\xA1" "aj", "pomij\xC3\xA9\xC5\xA1" "aj"},   // pomijéšaj
        {"zahtijeva", "zahtij\xC3\xA9va"},   // zahtijéva
        {"razumijevanje", "razumij\xC3\xA9vanje"},   // razumijévanje
        {"polije\xC4\x87" "e", "polij\xC3\xA9\xC4\x87" "e"},   // polijéće
        {"Zvonko", "Zv\xC3\xB3nko"},   // Zvónko
        {"Zvonka", "Zv\xC3\xB3nka"},   // Zvónka
        {"Zvonkov", "Zv\xC3\xB3nkov"},   // Zvónkov
        {"Vlado", "Vl\xC3\xA1" "do"},   // Vládo
        {"uredi", "ur\xC3\xA9" "di"},   // urédi
        {"uredio", "ur\xC3\xA9" "dio"},   // urédio
        {"uredim", "ur\xC3\xA9" "dim"},   // urédim
        {"ure\xC4\x91" "eno", "ur\xC3\xA9\xC4\x91" "eno"},   // uréđeno
        {"otvori", "otv\xC3\xB2ri"},   // otvòri
        {"otvorim", "otv\xC3\xB2rim"},   // otvòrim
        {"otvoren", "otv\xC3\xB2ren"},   // otvòren
        {"zatvori", "zatv\xC3\xB2ri"},   // zatvòri
        {"isklju\xC4\x8Di", "isklj\xC3\xBA\xC4\x8Di"},   // iskljúči
        {"isklju\xC4\x8D" "eno", "\xC3\xACsklju\xC4\x8D" "eno"},   // ìsključeno
        {"potvrdi", "potv\xC5\x95" "di"},   // potvŕdi
        {"potvr\xC4\x91" "eno", "potv\xC5\x95\xC4\x91" "eno"},   // potvŕđeno
        {"objavi", "obj\xC3\xA1vi"},   // objávi
        {"prijavi", "prij\xC3\xA1vi"},   // prijávi
        {"pro\xC4\x8Ditaj", "pro\xC4\x8D\xC3\xACtaj"},   // pročìtaj
        {"pro\xC4\x8Ditao", "pro\xC4\x8D\xC3\xACtao"},   // pročìtao
        {"pokreni", "pokr\xC3\xA9ni"},   // pokréni
        {"pokrenuo", "pokr\xC3\xA9nuo"},   // pokrénuo
        {"prika\xC5\xBEi", "prik\xC3\xA1\xC5\xBEi"},   // prikáži
        {"prikazan", "prik\xC3\xA1zan"},   // prikázan
        {"napi\xC5\xA1i", "nap\xC3\xAD\xC5\xA1i"},   // napíši
        {"odaberi", "odab\xC3\xA8ri"},   // odabèri
        {"po\xC5\xA1" "alji", "po\xC5\xA1\xC3\xA0lji"},   // pošàlji
        {"preuzmi", "pre\xC3\xB9zmi"},   // preùzmi
        {"unesi", "un\xC3\xA8si"},   // unèsi
        {"prevedi", "prev\xC3\xA8" "di"},   // prevèdi
        {"zadr\xC5\xBEi", "zadr\xCC\x80\xC5\xBEi"},   // zadr̀ži
        {"omogu\xC4\x87i", "omog\xC3\xBA\xC4\x87i"},   // omogúći
        {"zavr\xC5\xA1i", "zav\xC5\x95\xC5\xA1i"},   // zavŕši
        {"ne zaboravi", "ne zab\xC3\xB2ravi"},   // ne zabòravi
        {"potvrdi lozinku", "potv\xC5\x95" "di lozinku"},   // potvŕdi lozinku
        {"molim potvrdi", "molim potv\xC5\x95" "di"},   // molim potvŕdi
        {"uklju\xC4\x8Duje", "uklj\xC3\xB9\xC4\x8Duje"},   // ukljùčuje
        {"uklju\xC4\x8Dujem", "uklj\xC3\xB9\xC4\x8Dujem"},   // ukljùčujem
        {"uklju\xC4\x8Duju\xC4\x87i", "uklj\xC3\xB9\xC4\x8Duju\xC4\x87i"},   // ukljùčujući
        {"prikazuje", "prik\xC3\xA0zuje"},   // prikàzuje
        {"potvr\xC4\x91uje", "potvr\xCC\x80\xC4\x91uje"},   // potvr̀đuje
        {"ure\xC4\x91uju", "ur\xC3\xA8\xC4\x91uju"},   // urèđuju
        {"omogu\xC4\x87uje", "omog\xC3\xB9\xC4\x87uje"},   // omogùćuje
        {"kupujem", "k\xC3\xB9pujem"},   // kùpujem
        {"napreduje", "n\xC3\xA0preduje"},   // nàpreduje
        {"sudjeluju", "s\xC3\xB9" "djeluju"},   // sùdjeluju
        {"ozna\xC4\x8D" "ava", "ozna\xC4\x8D\xC3\xA1va"},   // označáva
        {"ozna\xC4\x8D" "avam", "ozna\xC4\x8D\xC3\xA1vam"},   // označávam
        {"ozna\xC4\x8D" "avaju", "ozna\xC4\x8D\xC3\xA1vaju"},   // označávaju
        {"rje\xC5\xA1" "ava", "rje\xC5\xA1\xC3\xA1va"},   // rješáva
        {"obavje\xC5\xA1tavam", "obavje\xC5\xA1t\xC3\xA1vam"},   // obavještávam
        {"u\xC5\xBEiva", "u\xC5\xBE\xC3\xADva"},   // užíva
        {"pokrivaju", "pokr\xC3\xADvaju"},   // pokrívaju
        {"oluje", "ol\xC3\xBAje"},   // olúje
        {"telefon", "tel\xC3\xA8" "fon"},   // telèfon
        {"telefona", "telef\xC3\xB3na"},   // telefóna
        {"telefonom", "telef\xC3\xB3nom"},   // telefónom
        {"telefonima", "telef\xC3\xB3nima"},   // telefónima
        {"telefonski", "tel\xC3\xA8" "fonski"},   // telèfonski
        {"ne znam", "n\xC3\xA8znam"},   // nèznam
        {"ne znamo", "n\xC3\xA8znamo"},   // nèznamo
        {"ne znate", "n\xC3\xA8znate"},   // nèznate
        {"ne znaju", "n\xC3\xA8znaju"},   // nèznaju
        {"ja ne znam", "ja n\xC3\xA8znam"},   // ja nèznam
        {"ro\xC4\x91" "enje", "ro\xC4\x91\xC3\xA9nje"},   // rođénje
        {"ro\xC4\x91" "enja", "ro\xC4\x91\xC3\xA9nja"},   // rođénja
        {"ro\xC4\x91" "enju", "ro\xC4\x91\xC3\xA9nju"},   // rođénju
        {"ro\xC4\x91" "enjem", "ro\xC4\x91\xC3\xA9njem"},   // rođénjem
        {"ro\xC4\x91" "enjima", "ro\xC4\x91\xC3\xA9njima"},   // rođénjima
        {"rje\xC5\xA1" "enje", "rje\xC5\xA1\xC3\xA9nje"},   // rješénje
        {"sni\xC5\xBE" "enje", "sni\xC5\xBE\xC3\xA9nje"},   // snižénje
        {"mikrofon", "m\xC8\x89krofon"},   // mȉkrofon
        {"mikrofona", "m\xC8\x89krofona"},   // mȉkrofona
        {"mikrofonom", "m\xC8\x89krofonom"},   // mȉkrofonom
        {"mikrofonima", "m\xC8\x89krofonima"},   // mȉkrofonima
        {"poruka", "p\xC8\x8Druka"},   // pȍruka
        {"poruke", "p\xC8\x8Druke"},   // pȍruke
        {"poruci", "p\xC8\x8Druci"},   // pȍruci
        {"porukom", "p\xC8\x8Drukom"},   // pȍrukom
        {"porukama", "p\xC8\x8Drukama"},   // pȍrukama
        {"preporuka", "pr\xC8\x85poruka"},   // prȅporuka
        {"Tijana", "T\xC3\xACjana"},   // Tìjana
        {"Tijanom", "T\xC3\xACjanom"},   // Tìjanom
        {"Tijanama", "T\xC3\xACjanama"},   // Tìjanama
        {"Zvonimir", "Zv\xC8\x8Dnimir"},   // Zvȍnimir
        {"Zvonimira", "Zv\xC8\x8Dnimira"},   // Zvȍnimira
        {"Zvonimirom", "Zv\xC8\x8Dnimirom"},   // Zvȍnimirom
        {"emotikon", "em\xC3\xB2tikon"},   // emòtikon
        {"emotikona", "em\xC3\xB2tikona"},   // emòtikona
        {"emotikonima", "em\xC3\xB2tikonima"},   // emòtikonima
        {"nepro\xC4\x8Ditan", "nepr\xC3\xB2\xC4\x8Ditan"},   // nepròčitan
        {"nepro\xC4\x8Ditane", "nepr\xC3\xB2\xC4\x8Ditane"},   // nepròčitane
        {"nepro\xC4\x8Ditanih", "nepr\xC3\xB2\xC4\x8Ditanih"},   // nepròčitanih
        {"nepreslu\xC5\xA1" "an", "nepr\xC3\xA8slu\xC5\xA1" "an"},   // neprèslušan
        {"nepreslu\xC5\xA1" "ane", "nepr\xC3\xA8slu\xC5\xA1" "ane"},   // neprèslušane
        {"nepregledan", "n\xC8\x85pregledan"},   // nȅpregledan
        {"nepregledanih", "n\xC8\x85pregledanih"},   // nȅpregledanih
        {"nepregledno", "n\xC8\x85pregledno"},   // nȅpregledno
        {"zaustavljanje", "za\xC3\xB9stavljanje"},   // zaùstavljanje
        {"zaustavljanja", "za\xC3\xB9stavljanja"},   // zaùstavljanja
        {"zaustavljati", "za\xC3\xB9stavljati"},   // zaùstavljati
        {"nepoznata", "n\xC8\x85poznata"},   // nȅpoznata
        {"nepotpisana", "nep\xC3\xB2tp\xC4\xABsana"},   // nepòtpīsana
        {"neispunjen", "ne\xC3\xACspunjen"},   // neìspunjen
        {"Nata\xC5\xA1" "a", "N\xC3\xA0ta\xC5\xA1" "a"},   // Nàtaša
        {"Nata\xC5\xA1" "om", "N\xC3\xA0ta\xC5\xA1" "om"},   // Nàtašom
        {"sad", "s\xC8\x81" "d"},   // sȁd
        {"sada", "s\xC8\x81" "da"},   // sȁda
        {"Novi Sad", "Novi S\xC8\x83" "d"},   // Novi Sȃd
        {"Novog Sada", "Novog S\xC3\xA1" "da"},   // Novog Sáda
        {"dvadeset", "dv\xC3\xA1" "deset"},   // dvádeset
        {"dvadeseti", "dv\xC3\xA1" "deseti"},   // dvádeseti
        {"dvadeset jedan", "dv\xC3\xA1" "deset jedan"},   // dvádeset jedan
        {"trideset", "tr\xC3\xAE" "deset"},   // trîdeset
        {"pedeset", "ped\xC3\xA8set"},   // pedèset
        {"pedeseti", "ped\xC3\xA8seti"},   // pedèseti
        {"nula", "n\xC8\x95la"},   // nȕla
        {"nule", "n\xC8\x95le"},   // nȕle
        {"nulti", "n\xC8\x95lti"},   // nȕlti
        {"\xC4\x8D" "etiristo", "\xC4\x8D\xC8\x85tiristo"},   // čȅtiristo
        {"tisu\xC4\x87" "a", "t\xC3\xACsu\xC4\x87" "a"},   // tìsuća
        {"tisu\xC4\x87u", "t\xC3\xACsu\xC4\x87u"},   // tìsuću
        {"tisu\xC4\x87" "e", "t\xC3\xACsu\xC4\x87" "e"},   // tìsuće
        {"tisu\xC4\x87iti", "t\xC3\xACsu\xC4\x87iti"},   // tìsućiti
        {"dvije tisu\xC4\x87" "e", "dvije t\xC3\xACsu\xC4\x87" "e"},   // dvije tìsuće
        {"dvadeset jedna tisu\xC4\x87" "a", "dvadeset jedna t\xC3\xACsu\xC4\x87" "a"},   // dvadeset jedna tìsuća
        {"pet tisu\xC4\x87" "a", "pet t\xC8\x89s\xC5\xAB\xC4\x87\xC4\x81"},   // pet tȉsūćā
        {"dvadeset tisu\xC4\x87" "a", "dvadeset t\xC8\x89s\xC5\xAB\xC4\x87\xC4\x81"},   // dvadeset tȉsūćā
        {"sto tisu\xC4\x87" "a", "sto t\xC8\x89s\xC5\xAB\xC4\x87\xC4\x81"},   // sto tȉsūćā
        {"milijarda", "mil\xC3\xACjarda"},   // milìjarda
        {"milijarde", "mil\xC3\xACjarde"},   // milìjarde
        {"milijardi", "mil\xC3\xACjardi"},   // milìjardi
        {"milijunti", "mil\xC3\xACjunti"},   // milìjunti
        {"peti", "p\xC8\x87ti"},   // pȇti
        {"peta", "p\xC8\x87ta"},   // pȇta
        {"petog", "p\xC8\x87tog"},   // pȇtog
        {"\xC5\xA1" "esti", "\xC5\xA1\xC8\x87sti"},   // šȇsti
        {"sedmi", "s\xC8\x87" "dmi"},   // sȇdmi
        {"sedmog", "s\xC8\x87" "dmog"},   // sȇdmog
        {"osmi", "\xC8\x8Fsmi"},   // ȏsmi
        {"osma", "\xC8\x8Fsma"},   // ȏsma
        {"deveti", "d\xC3\xA8veti"},   // dèveti
        {"devetoga", "d\xC3\xA8vetoga"},   // dèvetoga
        {"deseti", "d\xC3\xA8seti"},   // dèseti
        {"desetog", "d\xC3\xA8setog"},   // dèsetog
        {"stoti", "st\xC8\x8Fti"},   // stȏti
        {"dvjestoti", "dvj\xC8\x85stoti"},   // dvjȅstoti
        {"dvoje", "dv\xC8\x8Dje"},   // dvȍje
        {"troje", "tr\xC8\x8Dje"},   // trȍje
        {"oba", "\xC8\x8D" "ba"},   // ȍba
        {"obje", "\xC8\x8D" "bje"},   // ȍbje
        {"dvojica", "dv\xC3\xB2jica"},   // dvòjica
        {"\xC4\x8D" "etvorica", "\xC4\x8D" "etv\xC3\xB2rica"},   // četvòrica
        {"ina\xC4\x8D" "e", "\xC8\x89na\xC4\x8D" "e"},   // ȉnače
        {"uostalom", "u\xC3\xB2stalom"},   // uòstalom
        {"ionako", "ion\xC3\xA0ko"},   // ionàko
        {"slobodno", "sl\xC8\x8D" "bodno"},   // slȍbodno
        {"slobodan", "sl\xC8\x8D" "bodan"},   // slȍbodan
        {"slobodnih", "sl\xC8\x8D" "bodnih"},   // slȍbodnih
        {"slobodnima", "sl\xC8\x8D" "bodnima"},   // slȍbodnima
        {"sloboda", "slob\xC3\xB2" "da"},   // slobòda
        {"Slobodana", "Slob\xC3\xB2" "dana"},   // Slobòdana
        {"snaga", "sn\xC3\xA1ga"},   // snága
        {"snazi", "sn\xC3\xA1zi"},   // snázi
        {"snagama", "sn\xC3\xA1gama"},   // snágama
        {"sna\xC5\xBE" "an", "sn\xC3\xA1\xC5\xBE" "an"},   // snážan
        {"sna\xC5\xBE" "no", "sn\xC3\xA1\xC5\xBE" "no"},   // snážno
        {"legalan", "l\xC3\xA8galan"},   // lègalan
        {"legalno", "l\xC3\xA8galno"},   // lègalno
        {"legalnih", "l\xC3\xA8galnih"},   // lègalnih
        {"legalnost", "leg\xC3\xA1ln\xC5\x8Dst"},   // legálnōst
        {"ilegalan", "\xC8\x89legalan"},   // ȉlegalan
        {"ilegalno", "\xC8\x89legalno"},   // ȉlegalno
        {"jednostavan", "j\xC8\x85" "dnostavan"},   // jȅdnostavan
        {"jednostavno", "j\xC8\x85" "dnostavno"},   // jȅdnostavno
        {"jednostavnije", "jednost\xC3\xA0vnije"},   // jednostàvnije
        {"jednostavnost", "jednost\xC3\xA1vn\xC5\x8Dst"},   // jednostávnōst
        {"Instagram", "\xC8\x88nstagram"},   // Ȉnstagram
        {"Instagramu", "\xC8\x88nstagramu"},   // Ȉnstagramu
        {"ubrzanje", "ubrz\xC3\xA1nje"},   // ubrzánje
        {"ubrzanja", "ubrz\xC3\xA1nja"},   // ubrzánja
        {"ubrzanjem", "ubrz\xC3\xA1njem"},   // ubrzánjem
        {"ubrzano", "\xC8\x95" "brzano"},   // ȕbrzano
        {"podesivo", "pod\xC3\xA8sivo"},   // podèsivo
        {"podesivih", "pod\xC3\xA8sivih"},   // podèsivih
        {"nepodesiv", "nepod\xC3\xA8siv"},   // nepodèsiv
        {"podesivost", "podes\xC3\xADvost"},   // podesívost
        {"monoton", "m\xC8\x8Dnoton"},   // mȍnoton
        {"monotono", "m\xC8\x8Dnotono"},   // mȍnotono
        {"monotonih", "m\xC8\x8Dnotonih"},   // mȍnotonih
        {"monotonija", "monot\xC3\xB2nija"},   // monotònija
        {"pauza", "p\xC3\xA0uza"},   // pàuza
        {"pauze", "p\xC3\xA0uze"},   // pàuze
        {"pauzama", "p\xC3\xA0uzama"},   // pàuzama
        {"sauna", "s\xC8\x81una"},   // sȁuna
        {"fauna", "f\xC3\xA0una"},   // fàuna
        {"korisni\xC4\x8Dki", "k\xC3\xB2risni\xC4\x8Dki"},   // kòrisnički
        {"korisni\xC4\x8Dkih", "k\xC3\xB2risni\xC4\x8Dkih"},   // kòrisničkih
        {"korisni\xC4\x8Dkim", "k\xC3\xB2risni\xC4\x8Dkim"},   // kòrisničkim
        {"korisnik", "k\xC3\xB2risnik"},   // kòrisnik
        {"korisnici", "k\xC3\xB2risnici"},   // kòrisnici
        {"koristan", "k\xC8\x8Dristan"},   // kȍristan
        {"korisno", "k\xC8\x8Drisno"},   // kȍrisno
        {"korist", "k\xC8\x8Drist"},   // kȍrist
        {"redak", "r\xC3\xA9" "dak"},   // rédak
        {"retka", "r\xC3\xA9tka"},   // rétka
        {"redka", "r\xC3\xA9" "dka"},   // rédka
        {"retku", "r\xC3\xA9tku"},   // rétku
        {"retkom", "r\xC3\xA9tkom"},   // rétkom
        {"recima", "r\xC3\xA9" "cima"},   // récima
        {"redaka", "r\xC8\x87" "daka"},   // rȇdaka
        {"ro\xC4\x91" "endan", "r\xC8\x8D\xC4\x91" "endan"},   // rȍđendan
        {"ro\xC4\x91" "endana", "r\xC8\x8D\xC4\x91" "endana"},   // rȍđendana
        {"ro\xC4\x91" "endanima", "r\xC8\x8D\xC4\x91" "endanima"},   // rȍđendanima
        {"zauzet", "z\xC8\x81uzet"},   // zȁuzet
        {"zauzeta", "z\xC8\x81uzeta"},   // zȁuzeta
        {"zauzeto", "z\xC8\x81uzeto"},   // zȁuzeto
        {"zauzetog", "z\xC8\x81uzetog"},   // zȁuzetog
        {"zauzetost", "z\xC8\x81uzetost"},   // zȁuzetost
        {"zauzeo", "z\xC8\x81uzeo"},   // zȁuzeo
        {"zauzela", "z\xC8\x81uzela"},   // zȁuzela
        {"zauzeti", "za\xC3\xB9zeti"},   // zaùzeti
        {"zauze\xC4\x87" "e", "zauz\xC3\xA9\xC4\x87" "e"},   // zauzéće
        // The list of October 2026 (docs/formant.md, "Stress")
        {"potpuno", "p\xC3\xB2tpuno"},   // pòtpuno
        {"maslinovu", "m\xC8\x81slinovu"},   // mȁslinovu
        {"mobitel", "m\xC3\xB2" "bitel"},   // mòbitel
        {"mobitela", "m\xC3\xB2" "bitela"},   // mòbitela
        {"oaza", "o\xC3\xA1za"},   // oáza
        {"raketa", "rak\xC3\xA8ta"},   // rakèta
        {"kralj", "kr\xC8\x83lj"},   // krȃlj
        {"kralja", "kr\xC3\xA1lja"},   // králja
        {"kraljevi", "kr\xC3\xA1ljevi"},   // králjevi
        {"uistinu", "\xC3\xB9istinu"},   // ùistinu
        {"higijena", "higij\xC3\xA9na"},   // higijéna
        {"javno", "j\xC8\x83vno"},   // jȃvno
        {"srednja", "sr\xC8\x85" "dnja"},   // srȅdnja
        {"ekipa", "ek\xC3\xADpa"},   // ekípa
        {"inspirirana", "insp\xC3\xACrirana"},   // inspìrirana
        {"inspirirati", "inspir\xC3\xADrati"},   // inspirírati
        {"uklju\xC4\x8D" "eno", "\xC3\xB9klju\xC4\x8D" "eno"},   // ùključeno
        {"uklju\xC4\x8D" "en", "\xC3\xB9klju\xC4\x8D" "en"},   // ùključen
        {"uklju\xC4\x8Dim", "\xC3\xB9klju\xC4\x8Dim"},   // ùključim
        {"uklju\xC4\x8Di", "uklj\xC3\xBA\xC4\x8Di"},   // ukljúči
        {"\xC5\xBE" "eljezni\xC4\x8D" "ar", "\xC5\xBE\xC3\xA8ljezni\xC4\x8D" "ar"},   // žèljezničar
        {"strelja\xC5\xA1tvo", "strelj\xC3\xA1\xC5\xA1tvo"},   // streljáštvo
        {"direktno", "d\xC3\xACrektno"},   // dìrektno
        {"najdra\xC5\xBEi", "n\xC8\x83jdra\xC5\xBEi"},   // nȃjdraži
        {"me\xC4\x91unarodni", "me\xC4\x91un\xC3\xA1rodni"},   // međunárodni
        {"odr\xC5\xBEivi", "odr\xCC\x80\xC5\xBEivi"},   // odr̀živi
        {"kampanji", "kamp\xC3\xA0nji"},   // kampànji
        {"zajedni\xC4\x8Dki", "z\xC8\x81jedni\xC4\x8Dki"},   // zȁjednički
        {"obrazovanje", "\xC3\xB2" "brazovanje"},   // òbrazovanje
        {"vid", "v\xC8\x8B" "d"},   // vȋd
        {"vidu", "v\xC3\xAD" "du"},   // vídu
        {"iskustvo", "isk\xC3\xBAstvo"},   // iskústvo
        {"iskustava", "\xC3\xACskustava"},   // ìskustava
        {"alat", "\xC3\xA0lat"},   // àlat
        {"alati", "al\xC3\xA1ti"},   // aláti
        {"model", "m\xC3\xB2" "del"},   // mòdel
        {"modeli", "mod\xC3\xA8li"},   // modèli
        {"razvijaju", "razv\xC3\xADjaju"},   // razvíjaju
        {"razvijen", "razv\xC3\xACjen"},   // razvìjen
        {"raspore\xC4\x91" "eni", "rasp\xC3\xB2re\xC4\x91" "eni"},   // raspòređeni
        {"rasporediti", "raspor\xC3\xA9" "diti"},   // rasporéditi
        {"programersko", "progr\xC3\xA0mersko"},   // progràmersko
        {"ekran", "\xC3\xA8kran"},   // èkran
        {"ekrana", "ekr\xC3\xA1na"},   // ekrána
        {"zaslon", "z\xC3\xA1slon"},   // záslon
        {"osna\xC5\xBEiti", "osn\xC3\xA1\xC5\xBEiti"},   // osnážiti
        {"potaknuti", "pot\xC3\xA0knuti"},   // potàknuti
        {"potakni", "pot\xC3\xA0kni"},   // potàkni
        {"potaknem", "p\xC3\xB2taknem"},   // pòtaknem
        {"suradnja", "sur\xC3\xA1" "dnja"},   // surádnja
        {"saradnja", "sar\xC3\xA0" "dnja"},   // saràdnja
        {"istovremeno", "ist\xC3\xB2vremeno"},   // istòvremeno
        {"znanje", "zn\xC3\xA1nje"},   // znánje
        {"moderator", "mod\xC3\xA8rator"},   // modèrator
        {"moderatorica", "mod\xC3\xA8ratorica"},   // modèratorica
        {"sudjelujete", "s\xC3\xB9" "djelujete"},   // sùdjelujete
        {"u\xC4\x8D" "estvujete", "\xC3\xB9\xC4\x8D" "estvujete"},   // ùčestvujete
        {"sudjelovati", "s\xC3\xB9" "djelovati"},   // sùdjelovati
        {"ja", "j\xC8\x83"},   // jȃ
        {"ti", "t\xC8\x8B"},   // tȋ
        {"on", "\xC8\x8Fn"},   // ȏn
        {"mi", "m\xC8\x8B"},   // mȋ
        {"vi", "v\xC8\x8B"},   // vȋ
        {"oni", "\xC3\xB2ni"},   // òni
        {"njih", "nj\xC8\x8Bh"},   // njȋh
        {u8"enida", u8"enída"},   // listener's long stressed i
        {"brklja\xC4\x8D" "a", "b\xC8\x91klja\xC4\x8D" "a"},   // bȑkljača
        {"ivan\xC5\xA1\xC4\x8Dica", "iv\xC3\xA0n\xC5\xA1\xC4\x8Dica"},   // ivànščica
        {"teodora", "teod\xC3\xB3ra"},   // teodóra
        {"crnogorac", "crn\xC3\xB2gorac"},   // crnògorac
        {"crnogorci", "crnog\xC3\xB3rci"},   // crnogórci
        {"milosava", "m\xC8\x89losava"},   // mȉlosava
        {"markovina", "m\xC3\xA1rkovina"},   // márkovina
        {"prugove\xC4\x8Dki", "pr\xC8\x97gove\xC4\x8Dki"},   // prȗgovečki
        {"danijel", "d\xC3\xA0nijel"},   // dànijel
        {"daniel", "d\xC3\xA0niel"},   // dàniel
        {"danijela", "d\xC3\xA0nijela"},   // dànijela
        {"roti\xC4\x87", "r\xC8\x8Dti\xC4\x87"},   // rȍtić
        {"tihi\xC4\x87", "t\xC8\x89hi\xC4\x87"},   // tȉhić
        {"leti\xC4\x87", "l\xC8\x85ti\xC4\x87"},   // lȅtić
        {"mati\xC4\x87", "m\xC3\xA1ti\xC4\x87"},   // mátić
        {"jusi\xC4\x87", "j\xC8\x95si\xC4\x87"},   // jȕsić
        {"mato", "m\xC3\xA1to"},   // máto
        {"maja", "m\xC3\xA1ja"},   // mája
        {"anita", "\xC3\xA0nita"},   // ànita
        {"sandra", "s\xC8\x83ndra"},   // sȃndra
        {"sini\xC5\xA1" "a", "s\xC3\xACni\xC5\xA1" "a"},   // sìniša
        {"mihajlo", "m\xC3\xAChajlo"},   // mìhajlo
        {"naida", "n\xC3\xA0ida"},   // nàida
        {"vladi\xC4\x87", "vl\xC3\xA1" "di\xC4\x87"},   // vládić
        {"eva", "\xC8\x87va"},   // ȇva
        {"adrijana", "adrij\xC3\xA0na"},   // adrijàna
        {"aleksandar", "aleks\xC3\xA1ndar"},   // aleksándar
        {"proteza", "prot\xC3\xA9za"},   // protéza
        {"protezu", "prot\xC3\xA9zu"},   // protézu
        {"protezama", "prot\xC3\xA9zama"},   // protézama
        {"proteti\xC4\x8D" "ar", "prot\xC3\xA8ti\xC4\x8D" "ar"},   // protètičar
        {"zubna", "z\xC8\x97" "bna"},   // zȗbna
        {"o\xC4\x8Dna", "\xC3\xB2\xC4\x8Dna"},   // òčna
        {"javnost", "j\xC3\xA1vnost"},   // jávnost
        {"javnosti", "j\xC3\xA1vnosti"},   // jávnosti
        {"potpunosti", "p\xC3\xB2tpunosti"},   // pòtpunosti
        {"iskustveni", "isk\xC3\xB9stveni"},   // iskùstveni
        {"najbolji", "n\xC8\x83jbolji"},   // nȃjbolji
        {"najbolje", "n\xC8\x83jbolje"},   // nȃjbolje
        {"najve\xC4\x87i", "n\xC8\x83jve\xC4\x87i"},   // nȃjveći
        {"najmanje", "n\xC8\x83jmanje"},   // nȃjmanje
        {"najvi\xC5\xA1" "e", "n\xC8\x83jvi\xC5\xA1" "e"},   // nȃjviše
        {"najja\xC4\x8Di", "n\xC8\x83jja\xC4\x8Di"},   // nȃjjači
        {"najbr\xC5\xBEi", "n\xC8\x83jbr\xC5\xBEi"},   // nȃjbrži
        {"najljep\xC5\xA1i", "n\xC8\x83jljep\xC5\xA1i"},   // nȃjljepši
        {"naj\xC4\x8D" "e\xC5\xA1\xC4\x87" "e", "n\xC8\x83j\xC4\x8D" "e\xC5\xA1\xC4\x87" "e"},   // nȃjčešće
        {"najprije", "n\xC8\x83jprije"},   // nȃjprije
        {"najposlije", "n\xC8\x83jposlije"},   // nȃjposlije
        {"najgori", "n\xC8\x83jgori"},   // nȃjgori
        {"najnoviji", "najn\xC3\xB2viji"},   // najnòviji
        {"najva\xC5\xBEniji", "najv\xC3\xA0\xC5\xBEniji"},   // najvàžniji
        {"najstariji", "najst\xC3\xA0riji"},   // najstàriji
        {"najjednostavniji", "najjednost\xC3\xA0vniji"},   // najjednostàvniji
        {"odabir", "\xC8\x8D" "dabir"},   // ȍdabir
        {"odabiru", "\xC8\x8D" "dabiru"},   // ȍdabiru
        {"odabrano", "\xC8\x8D" "dabrano"},   // ȍdabrano
        {"odabrao", "\xC8\x8D" "dabrao"},   // ȍdabrao
        {"odaberem", "od\xC3\xA0" "berem"},   // odàberem
        {"odaberi", "odab\xC3\xA8ri"},   // odabèri
        {"odabirati", "od\xC3\xA0" "birati"},   // odàbirati
        {"odabirem", "od\xC3\xA0" "birem"},   // odàbirem
        {"asistenta", "as\xC3\xACstenta"},   // asìstenta
        {"asistenata", "as\xC3\xACstenata"},   // asìstenata
        {"u\xC4\x8D" "enik", "\xC8\x95\xC4\x8D" "enik"},   // ȕčenik
        {"u\xC4\x8D" "enika", "\xC8\x95\xC4\x8D" "enika"},   // ȕčenika
        {"u\xC4\x8D" "enici", "\xC8\x95\xC4\x8D" "enici"},   // ȕčenici
        {"izgled", "\xC8\x89zgled"},   // ȉzgled
        {"izgleda", "\xC8\x89zgleda"},   // ȉzgleda
        {"izgledam", "\xC3\xACzgledam"},   // ìzgledam
        {"izgledaju", "izgl\xC3\xA9" "daju"},   // izglédaju
        {"komentar", "kom\xC3\xA8ntar"},   // komèntar
        {"komentara", "koment\xC3\xA1ra"},   // komentára
        {"prekini", "pr\xC3\xA8kini"},   // prèkini
        {"prekinuo", "pr\xC3\xA8kinuo"},   // prèkinuo
        {"prekinut", "pr\xC3\xA8kinut"},   // prèkinut
        {"prekinem", "pr\xC3\xA8kinem"},   // prèkinem
        {"\xC5\xBEivot", "\xC5\xBE\xC3\xACvot"},   // žìvot
        {"\xC5\xBEivota", "\xC5\xBEiv\xC3\xB2ta"},   // živòta
        {"\xC5\xBEivote", "\xC5\xBEiv\xC3\xB2te"},   // živòte
        {"\xC5\xBEivotinja", "\xC5\xBEiv\xC3\xB2tinja"},   // živòtinja
        {"spava", "sp\xC8\x83va"},   // spȃva
        {"spavati", "sp\xC3\xA1vati"},   // spávati
        {"spavao", "sp\xC3\xA1vao"},   // spávao
        {"nema", "n\xC8\x87ma"},   // nȇma
        {"nemam", "n\xC8\x87mam"},   // nȇmam
        {"nemaju", "n\xC3\xA9maju"},   // némaju
        {"lice", "l\xC3\xAD" "ce"},   // líce
        {"lica", "l\xC3\xAD" "ca"},   // líca
        {"kredit", "kr\xC3\xA8" "dit"},   // krèdit
        {"kredita", "kred\xC3\xADta"},   // kredíta
        {"stavka", "st\xC8\x83vka"},   // stȃvka
        {"stavke", "st\xC8\x83vke"},   // stȃvke
        {"kre\xC4\x87" "ete", "kr\xC8\x87\xC4\x87" "ete"},   // krȇćete
        {"kre\xC4\x87" "e", "kr\xC8\x87\xC4\x87" "e"},   // krȇće
        {"kre\xC4\x87i", "kr\xC3\xA9\xC4\x87i"},   // kréći
        {"kretanje", "kr\xC3\xA9tanje"},   // krétanje
        {"kreni", "kr\xC3\xA9ni"},   // kréni
        {"krenem", "kr\xC8\x87nem"},   // krȇnem
        {"krenuo", "kr\xC3\xA9nuo"},   // krénuo
        {"prikva\xC4\x8Di", "pr\xC3\xACkva\xC4\x8Di"},   // prìkvači
        {"prikva\xC4\x8D" "en", "pr\xC3\xACkva\xC4\x8D" "en"},   // prìkvačen
        {"otkva\xC4\x8Di", "\xC3\xB2tkva\xC4\x8Di"},   // òtkvači
        {"dokumentarac", "dokument\xC3\xA1rac"},   // dokumentárac
        {"dokumentarca", "dokument\xC3\xA1rca"},   // dokumentárca
        {"vlast", "vl\xC8\x83st"},   // vlȃst
        {"vlasti", "vl\xC8\x83sti"},   // vlȃsti
        {"na vlasti", "na vl\xC3\xA1sti"},   // na vlásti
        {"novac", "n\xC3\xB2vac"},   // nòvac
        {"novcem", "n\xC3\xB3vcem"},   // nóvcem
        {"novci", "n\xC8\x8Fvci"},   // nȏvci
        {"sustav", "s\xC3\xBAstav"},   // sústav
        {"sustava", "s\xC3\xBAstava"},   // sústava
        {"radnja", "r\xC3\xA1" "dnja"},   // rádnja
        {"radnje", "r\xC3\xA1" "dnje"},   // rádnje
        {"dar", "d\xC8\x83r"},   // dȃr
        {"darovi", "d\xC8\x81rovi"},   // dȁrovi
        {"o daru", "o d\xC3\xA1ru"},   // o dáru
        {"va\xC5\xBEi", "v\xC8\x83\xC5\xBEi"},   // vȃži
        {"va\xC5\xBEiti", "v\xC3\xA1\xC5\xBEiti"},   // vážiti
        {"va\xC5\xBE" "e\xC4\x87i", "v\xC3\xA1\xC5\xBE" "e\xC4\x87i"},   // vážeći
    };
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    for (const auto& c : cases) {
        std::vector<int16_t> plain = speak(engine.handle, c.plain);
        std::vector<int16_t> accented = speak(engine.handle, c.accented);
        INFO(c.plain);
        REQUIRE(!plain.empty());
        REQUIRE(plain == accented);
    }

    // The noun "obavijesti" and the verb's infinitive differ in stress.
    REQUIRE(speak(engine.handle, "obavijesti") != speak(engine.handle, "obavijestiti"));

    // The short vowel of sȁd stays clearly shorter than the long one of
    // sȃd at the end of a clause (the only difference between the two).
    {
        const std::vector<int16_t> kratko = speak(engine.handle, "sad");
        const std::vector<int16_t> dugo = speak(engine.handle, "s\xC8\x83" "d");
        REQUIRE(dugo.size() > kratko.size() + 22050 * 80 / 1000);
    }

    // Verbs with a long "ije" after a prefix are found by their root. Nouns,
    // adjectives and other verbs that only look like them keep their accent.
    const Case not_these[] = {
        {"ro\xC4\x91" "endan", "ro\xC4\x91\xC3\xA9ndan"},   // rođéndan
        {"u\xC4\x8D" "enje", "u\xC4\x8D\xC3\xA9nje"},   // učénje
        {"dr\xC5\xBE" "ava", "dr\xC5\xBE\xC3\xA1va"},   // držáva
        {"zabava", "zab\xC3\xA1va"},   // zabáva
        {"predstava", "predst\xC3\xA1va"},   // predstáva
        {"osjetljiva", "osjetlj\xC3\xADva"},   // osjetljíva
        {"dodaje", "dod\xC3\xA1je"},   // dodáje
        {"povijesti", "povij\xC3\xA9sti"},   // povijésti
        {"prelijepi", "prelij\xC3\xA9pi"},   // prelijépi
        {"donijeli", "donij\xC3\xA9li"},   // donijéli
        {"zapovijedi", "zapovij\xC3\xA9" "di"},   // zapovijédi
    };
    for (const auto& c : not_these) {
        INFO(c.plain);
        REQUIRE(speak(engine.handle, c.plain) != speak(engine.handle, c.accented));
    }

    // "Potvrdi" opens a clause as a command; after a preposition or another
    // word it is the noun, which has priority ("u potvrdi", "svi uredi").
    const Case nouns[] = {
        {"u potvrdi", "u potv\xC5\x95" "di"},   // u potvŕdi
        {"hvala na objavi", "hvala na obj\xC3\xA1vi"},   // hvala na objávi
        {"svi uredi", "svi ur\xC3\xA9" "di"},   // svi urédi
        {"nova oprema", "nova opr\xC3\xA9ma"},   // nova opréma
    };
    for (const auto& c : nouns) {
        INFO(c.plain);
        REQUIRE(speak(engine.handle, c.plain) != speak(engine.handle, c.accented));
    }

    // Mirsad follows the dictionaries' shifts throughout; Croatian also
    // shifts podijeliti and proslijediti, while keeping the root accent in
    // the other verbs here.
    const Case bosnian[] = {
        {"podijelim", "p\xC3\xB2" "dijelim"},   // pòdijelim
        {"podijeljen", "p\xC3\xB2" "dijeljen"},   // pòdijeljen
        {"podijeli", "podij\xC3\xA9li"},   // podijéli
        {"podijelio", "podij\xC3\xA9lio"},   // podijélio
        {"pomije\xC5\xA1" "am", "p\xC3\xB2mije\xC5\xA1" "am"},   // pòmiješam
        {"upotrijebim", "up\xC3\xB2trijebim"},   // upòtrijebim
        {"uredi", "ur\xC3\xA9" "di"},   // urédi
        {"uredim", "\xC3\xB9r\xC4\x93" "dim"},   // ùrēdim
        {"ure\xC4\x91" "en", "\xC3\xB9r\xC4\x93\xC4\x91" "en"},   // ùrēđen
        {"otvorim", "\xC3\xB2tvorim"},   // òtvorim
        {"otvorio", "otv\xC3\xB2rio"},   // otvòrio
        {"pro\xC4\x8Ditan", "pr\xC3\xB2\xC4\x8Ditan"},   // pròčitan
        {"pokrenut", "p\xC3\xB2kr\xC4\x93nut"},   // pòkrēnut
        {"prika\xC5\xBE" "em", "pr\xC3\xACk\xC4\x81\xC5\xBE" "em"},   // prìkāžem
        {"prika\xC5\xBEi", "prik\xC3\xA1\xC5\xBEi"},   // prikáži
        {"uklju\xC4\x8Duje", "uklj\xC3\xB9\xC4\x8Duje"},   // ukljùčuje
        {"ozna\xC4\x8D" "ava", "ozn\xC3\xA0\xC4\x8D\xC4\x81va"},   // oznàčāva
        {"u\xC5\xBEivam", "\xC3\xB9\xC5\xBE\xC4\xABvam"},   // ùžīvam
        {"rje\xC5\xA1" "ava", "rj\xC3\xA8\xC5\xA1\xC4\x81va"},   // rjèšāva
        {"ne znam", "n\xC3\xA8zn\xC4\x81m"},   // nèznām
        {"ne znamo", "n\xC3\xA8zn\xC4\x81mo"},   // nèznāmo
    };
    REQUIRE(laprdus_set_voice(engine.handle, "mirsad", NO_DATA) == LAPRDUS_OK);
    for (const auto& c : bosnian) {
        INFO(c.plain);
        REQUIRE(speak(engine.handle, c.plain) == speak(engine.handle, c.accented));
    }

    // The common ekavian verbs are listed stem by stem.
    const Case ekavian[] = {
        {"podelio", "pod\xC3\xA9lio"},   // podélio
        {"podelim", "p\xC3\xB2" "d\xC4\x93lim"},   // pòdēlim
        {"zalepi", "zal\xC3\xA9pi"},   // zalépi
        {"pome\xC5\xA1" "ao", "pom\xC3\xA9\xC5\xA1" "ao"},   // poméšao
        {"prosledi", "prosl\xC3\xA9" "di"},   // proslédi
    };
    REQUIRE(laprdus_set_voice(engine.handle, "stojan", NO_DATA) == LAPRDUS_OK);
    for (const auto& c : ekavian) {
        INFO(c.plain);
        REQUIRE(speak(engine.handle, c.plain) == speak(engine.handle, c.accented));
    }
}

TEST_CASE("Croatian ideja besplatan and podijelim follow dictionary accents", "[formant][text]") {
    // Mrežnik: idéja (the same accent throughout its cases), bèsplatan.
    // HJP: podijéliti, present pòdijēlīm, passive pòdijēljen.
    // Zvonko keeps its usual omission of post-accent length; the long ije
    // is already supplied by phonemization, including after the accent.
    struct Case { const char* plain; const char* accented; };
    const Case cases[] = {
        {u8"ideja", u8"idéja"},
        {u8"ideje", u8"idéje"},
        {u8"ideji", u8"idéji"},
        {u8"ideju", u8"idéju"},
        {u8"idejo", u8"idéjo"},
        {u8"idejom", u8"idéjom"},
        {u8"idejama", u8"idéjama"},
        {u8"besplatan", u8"bèsplatan"},
        {u8"besplatna", u8"bèsplatna"},
        {u8"besplatno", u8"bèsplatno"},
        {u8"besplatni", u8"bèsplatni"},
        {u8"besplatne", u8"bèsplatne"},
        {u8"besplatnu", u8"bèsplatnu"},
        {u8"besplatnog", u8"bèsplatnog"},
        {u8"besplatnoga", u8"bèsplatnoga"},
        {u8"besplatnom", u8"bèsplatnom"},
        {u8"besplatnome", u8"bèsplatnome"},
        {u8"besplatnomu", u8"bèsplatnomu"},
        {u8"besplatnoj", u8"bèsplatnoj"},
        {u8"besplatnih", u8"bèsplatnih"},
        {u8"besplatnim", u8"bèsplatnim"},
        {u8"besplatnima", u8"bèsplatnima"},
        {u8"podijelim", u8"pòdijelim"},
        {u8"podijeliš", u8"pòdijeliš"},
        {u8"podijelimo", u8"pòdijelimo"},
        {u8"podijele", u8"pòdijele"},
        {u8"podijeljen", u8"pòdijeljen"},
        {u8"podijeljena", u8"pòdijeljena"},
        {u8"podijeljeno", u8"pòdijeljeno"},
        {u8"podijeljeni", u8"pòdijeljeni"},
        {u8"podijeljenih", u8"pòdijeljenih"},
        {u8"podijeljenima", u8"pòdijeljenima"},
        {u8"nepodijeljen", u8"nepòdijeljen"},
        {u8"podijeliti", u8"podijéliti"},
        {u8"podijelio", u8"podijélio"},
        {u8"podijelila", u8"podijélila"},
        // Ambiguous -i/-ite forms retain the existing imperative reading.
        {u8"podijeli", u8"podijéli"},
        {u8"podijelite", u8"podijélite"},
        // Other prefixes must not inherit the podijeliti override.
        {u8"raspodijelim", u8"raspodijélim"},
        {u8"dodijelim", u8"dodijélim"},
        {u8"To je ideja", u8"To je idéja"},
        {u8"Ulaz je besplatan", u8"Ulaz je bèsplatan"},
        {u8"Želim da podijelim ideju", u8"Želim da pòdijelim idéju"},
    };
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    for (const auto& c : cases) {
        INFO(c.plain);
        const auto plain = speak(engine.handle, c.plain);
        REQUIRE(!plain.empty());
        REQUIRE(plain == speak(engine.handle, c.accented));
    }
    REQUIRE(speak(engine.handle, u8"ideja") != speak(engine.handle, u8"idèja"));
    REQUIRE(speak(engine.handle, u8"besplatan") != speak(engine.handle, u8"bȅsplatan"));
    REQUIRE(speak(engine.handle, u8"podijelim") != speak(engine.handle, u8"podijélim"));
}

TEST_CASE("Croatian proslijediti shifts its present and passive accents", "[formant][text]") {
    // HJP proslijéditi, pròslijēdīm, pròslijēđen. Croatian omits
    // unstressed length except the ije supplied by phonemization.
    struct Case { const char* plain; const char* accented; };
    const Case cases[] = {
        {u8"proslijediti", u8"proslijéditi"},
        {u8"proslijedit ću", u8"proslijédit ću"},
        {u8"proslijedim", u8"pròslijedim"},
        {u8"proslijediš", u8"pròslijediš"},
        {u8"proslijedimo", u8"pròslijedimo"},
        {u8"proslijede", u8"pròslijede"},
        // Ambiguous -i/-ite forms retain the existing imperative reading.
        {u8"proslijedi", u8"proslijédi"},
        {u8"proslijedite", u8"proslijédite"},
        {u8"proslijedio", u8"proslijédio"},
        {u8"proslijedila", u8"proslijédila"},
        {u8"proslijedilo", u8"proslijédilo"},
        {u8"proslijedili", u8"proslijédili"},
        {u8"proslijedile", u8"proslijédile"},
        {u8"proslijedivši", u8"proslijédivši"},
        {u8"proslijeđen", u8"pròslijeđen"},
        {u8"proslijeđena", u8"pròslijeđena"},
        {u8"proslijeđeno", u8"pròslijeđeno"},
        {u8"proslijeđeni", u8"pròslijeđeni"},
        {u8"proslijeđene", u8"pròslijeđene"},
        {u8"proslijeđenu", u8"pròslijeđenu"},
        {u8"proslijeđenog", u8"pròslijeđenog"},
        {u8"proslijeđenoga", u8"pròslijeđenoga"},
        {u8"proslijeđenom", u8"pròslijeđenom"},
        {u8"proslijeđenome", u8"pròslijeđenome"},
        {u8"proslijeđenomu", u8"pròslijeđenomu"},
        {u8"proslijeđenoj", u8"pròslijeđenoj"},
        {u8"proslijeđenih", u8"pròslijeđenih"},
        {u8"proslijeđenim", u8"pròslijeđenim"},
        {u8"proslijeđenima", u8"pròslijeđenima"},
        {u8"neproslijeđen", u8"nepròslijeđen"},
        {u8"neproslijeđena", u8"nepròslijeđena"},
        {u8"Poruka je proslijeđena", u8"Poruka je pròslijeđena"},
        {u8"Želim da proslijedim poruku", u8"Želim da pròslijedim poruku"},
        {u8"Proslijedi poruku", u8"Proslijédi poruku"},
        // Other prefixes and the imperfective retain their existing rules.
        {u8"naslijedim", u8"naslijédim"},
        {u8"naslijeđen", u8"naslijéđen"},
        {u8"prosljeđivati", u8"prosljeđívati"},
        {u8"prosljeđujem", u8"prosljèđujem"},
    };
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    for (const auto& c : cases) {
        INFO(c.plain);
        const auto plain = speak(engine.handle, c.plain);
        REQUIRE(!plain.empty());
        REQUIRE(plain == speak(engine.handle, c.accented));
    }
    REQUIRE(speak(engine.handle, u8"proslijediti") != speak(engine.handle, u8"pròslijediti"));
    REQUIRE(speak(engine.handle, u8"proslijedim") != speak(engine.handle, u8"proslijédim"));
    REQUIRE(speak(engine.handle, u8"proslijeđen") != speak(engine.handle, u8"proslijéđen"));
}

TEST_CASE("Croatian sucelje and otici keep their lexical accents", "[formant][text]") {
    // HJP: súčēlje; HJP / Školski rječnik: òtīći, òtišao, òtišla.
    // Croatian omits unstressed length, but the stressed u stays long.
    struct Case { const char* plain; const char* accented; };
    const Case cases[] = {
        {u8"sučelje", u8"súčelje"},
        {u8"sučelja", u8"súčelja"},
        {u8"sučelju", u8"súčelju"},
        {u8"sučeljem", u8"súčeljem"},
        {u8"sučeljima", u8"súčeljima"},
        {u8"otići", u8"òtići"},
        {u8"otišao", u8"òtišao"},
        {u8"otišla", u8"òtišla"},
        {u8"otišlo", u8"òtišlo"},
        {u8"otišli", u8"òtišli"},
        {u8"otišle", u8"òtišle"},
        {u8"otišavši", u8"òtišavši"},
        {u8"Korisničko sučelje", u8"Korisničko súčelje"},
        {u8"Otići ću kući", u8"Òtići ću kući"},
        {u8"Otišao je kući", u8"Òtišao je kući"},
        {u8"Otišla je kući", u8"Òtišla je kući"},
        {u8"Oni su otišli", u8"Oni su òtišli"},
    };
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    for (const auto& c : cases) {
        INFO(c.plain);
        const auto plain = speak(engine.handle, c.plain);
        REQUIRE(!plain.empty());
        REQUIRE(plain == speak(engine.handle, c.accented));
    }
    REQUIRE(speak(engine.handle, u8"sučelje") != speak(engine.handle, u8"sùčelje"));
    REQUIRE(speak(engine.handle, u8"sučelje") != speak(engine.handle, u8"sȗčelje"));
    REQUIRE(speak(engine.handle, u8"otići") != speak(engine.handle, u8"otíći"));
    REQUIRE(speak(engine.handle, u8"otišao") != speak(engine.handle, u8"otìšao"));
    REQUIRE(speak(engine.handle, u8"otišla") != speak(engine.handle, u8"ȍtišla"));
}

TEST_CASE("Croatian sat and rad distinguish case accents", "[formant][text]") {
    // Školski rječnik / HJP noun paradigms, including D vs L sg and
    // N/V vs G pl sati. Stressed length stays; unstressed length is omitted.
    struct Case { const char* plain; const char* accented; };
    const Case cases[] = {
        {u8"sat", u8"sȃt"},
        {u8"sata", u8"sȃta"},
        {u8"satu", u8"sȃtu"},
        {u8"sate", u8"sȃte"},
        {u8"satom", u8"sȃtom"},
        {u8"sati", u8"sȃti"},
        {u8"satima", u8"sátima"},
        {u8"satovi", u8"sȁtovi"},
        {u8"satova", u8"sȁtova"},
        {u8"satove", u8"sȁtove"},
        {u8"satovima", u8"sȁtovima"},
        {u8"rad", u8"rȃd"},
        {u8"rada", u8"rȃda"},
        {u8"radu", u8"rȃdu"},
        {u8"rade", u8"rȃde"},
        {u8"radom", u8"rȃdom"},
        {u8"radovi", u8"rȁdovi"},
        {u8"radova", u8"rȁdova"},
        {u8"radove", u8"rȁdove"},
        {u8"radovima", u8"rȁdovima"},
        {u8"Dva sata", u8"Dva sȃta"},
        {u8"Pet sati", u8"Pet sáti"},
        {u8"Dvanaest sati", u8"Dvanaest sáti"},
        {u8"25 sati", u8"25 sáti"},
        {u8"Nekoliko sati", u8"Nekoliko sáti"},
        {u8"Do kasnih sati", u8"Do kasnih sáti"},
        {u8"Pet radnih sati", u8"Pet radnih sáti"},
        {u8"Koliko je sati", u8"Koliko je sáti"},
        {u8"Sati prolaze", u8"Sȃti prolaze"},
        {u8"To su dugi sati", u8"To su dugi sȃti"},
        {u8"Na satu", u8"Na sátu"},
        {u8"Na tom satu", u8"Na tom sátu"},
        {u8"Prema satu", u8"Prema sȃtu"},
        {u8"Na radu", u8"Na rádu"},
        {u8"O radu", u8"O rádu"},
        {u8"Pri radu", u8"Pri rádu"},
        {u8"U radu", u8"U rádu"},
        {u8"Po radu", u8"Po rádu"},
        {u8"O svom novom radu", u8"O svom novom rádu"},
        {u8"O našem radu", u8"O našem rádu"},
        {u8"Pristup radu", u8"Pristup rȃdu"},
        {u8"Prema dobrom radu", u8"Prema dobrom rȃdu"},
        {u8"Na poslu se posvetio radu", u8"Na poslu se posvetio rȃdu"},
        {u8"Nema rada", u8"Nema rȃda"},
        {u8"Oni rade", u8"Oni rȃde"},
        // Exact paradigms must not catch the verb or unrelated sat- words.
        {u8"raditi", u8"ráditi"},
        {u8"radila", u8"rádila"},
        {u8"satelit", u8"satèlit"},
    };
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    for (const auto& c : cases) {
        INFO(c.plain);
        const auto plain = speak(engine.handle, c.plain);
        REQUIRE(!plain.empty());
        REQUIRE(plain == speak(engine.handle, c.accented));
    }
    REQUIRE(speak(engine.handle, u8"sati") != speak(engine.handle, u8"sáti"));
    REQUIRE(speak(engine.handle, u8"pet sati") != speak(engine.handle, u8"pet sȃti"));
    REQUIRE(speak(engine.handle, u8"o radu") != speak(engine.handle, u8"o rȃdu"));
    REQUIRE(speak(engine.handle, u8"prema radu") != speak(engine.handle, u8"prema rádu"));
    REQUIRE(speak(engine.handle, u8"satovi") != speak(engine.handle, u8"sȃtovi"));

    // User entries override these contextual guesses; written accents
    // still take precedence over a user entry.
    const char* json = R"({"entries":[{"word":"s^a:ti"},{"word":"r^a:du"},{"word":"s^a:tu"}]})";
    REQUIRE(laprdus_load_accent_lexicon_from_memory(engine.handle, json, 0) == LAPRDUS_OK);
    REQUIRE(speak(engine.handle, u8"pet sati") == speak(engine.handle, u8"pet sȃti"));
    REQUIRE(speak(engine.handle, u8"o radu") == speak(engine.handle, u8"o rȃdu"));
    REQUIRE(speak(engine.handle, u8"na satu") == speak(engine.handle, u8"na sȃtu"));
    REQUIRE(speak(engine.handle, u8"o radu") != speak(engine.handle, u8"o rádu"));
    laprdus_clear_accent_lexicon(engine.handle);
    REQUIRE(speak(engine.handle, u8"o radu") == speak(engine.handle, u8"o rádu"));
}

TEST_CASE("Croatian pobjednik and dvadeset families follow dictionary accents", "[formant][text]") {
    // HJP pòbjednīk/pòbjedničkī; Mrežnik dvádesēt/dvádesētī/dvadesétak;
    // Školski rječnik dvádesetero. Croatian omits unstressed lengths.
    struct Case { const char* plain; const char* accented; };
    const Case cases[] = {
        {u8"pobjednik", u8"pòbjednik"},
        {u8"pobjednika", u8"pòbjednika"},
        {u8"pobjedniku", u8"pòbjedniku"},
        {u8"pobjednikom", u8"pòbjednikom"},
        {u8"pobjedniče", u8"pòbjedniče"},
        {u8"pobjednici", u8"pòbjednici"},
        {u8"pobjednicima", u8"pòbjednicima"},
        {u8"pobjednike", u8"pòbjednike"},
        {u8"pobjednička", u8"pòbjednička"},
        {u8"pobjednički", u8"pòbjednički"},
        {u8"pobjedničko", u8"pòbjedničko"},
        {u8"pobjedničke", u8"pòbjedničke"},
        {u8"pobjedničku", u8"pòbjedničku"},
        {u8"pobjedničkog", u8"pòbjedničkog"},
        {u8"pobjedničkoga", u8"pòbjedničkoga"},
        {u8"pobjedničkom", u8"pòbjedničkom"},
        {u8"pobjedničkome", u8"pòbjedničkome"},
        {u8"pobjedničkomu", u8"pòbjedničkomu"},
        {u8"pobjedničkoj", u8"pòbjedničkoj"},
        {u8"pobjedničkih", u8"pòbjedničkih"},
        {u8"pobjedničkim", u8"pòbjedničkim"},
        {u8"pobjedničkima", u8"pòbjedničkima"},
        {u8"dvadeset", u8"dvádeset"},
        {u8"dvadeseti", u8"dvádeseti"},
        {u8"dvadeseta", u8"dvádeseta"},
        {u8"dvadeseto", u8"dvádeseto"},
        {u8"dvadesete", u8"dvádesete"},
        {u8"dvadesetu", u8"dvádesetu"},
        {u8"dvadesetog", u8"dvádesetog"},
        {u8"dvadesetoga", u8"dvádesetoga"},
        {u8"dvadesetom", u8"dvádesetom"},
        {u8"dvadesetome", u8"dvádesetome"},
        {u8"dvadesetomu", u8"dvádesetomu"},
        {u8"dvadesetoj", u8"dvádesetoj"},
        {u8"dvadesetih", u8"dvádesetih"},
        {u8"dvadesetim", u8"dvádesetim"},
        {u8"dvadesetima", u8"dvádesetima"},
        {u8"dvadesetero", u8"dvádesetero"},
        {u8"dvadesetak", u8"dvadesétak"},
        {u8"Pobjednička ekipa", u8"Pòbjednička ekipa"},
        {u8"On je pobjednik", u8"On je pòbjednik"},
        {u8"Čestitam pobjednicima", u8"Čestitam pòbjednicima"},
        {u8"Dvadeset pobjednika", u8"Dvádeset pòbjednika"},
        {u8"20", u8"dvádeset"},
        {u8"21", u8"dvádeset jedan"},
        {u8"120", u8"sto dvádeset"},
        {u8"Dvadesetak pobjednika", u8"Dvadesétak pòbjednika"},
    };
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    for (const auto& c : cases) {
        INFO(c.plain);
        const auto plain = speak(engine.handle, c.plain);
        REQUIRE(!plain.empty());
        REQUIRE(plain == speak(engine.handle, c.accented));
    }
    REQUIRE(speak(engine.handle, u8"pobjednik") != speak(engine.handle, u8"pobjédnik"));
    REQUIRE(speak(engine.handle, u8"pobjednička") != speak(engine.handle, u8"pobjèdnička"));
    REQUIRE(speak(engine.handle, u8"pobjednička") != speak(engine.handle, u8"pȍbjednička"));
    REQUIRE(speak(engine.handle, u8"dvadeset") != speak(engine.handle, u8"dvȃdeset"));
    REQUIRE(speak(engine.handle, u8"dvadesetak") != speak(engine.handle, u8"dvádesetak"));
}

TEST_CASE("The tens keep their long first vowel", "[formant][text]") {
    // The long first vowel prevents the earlier clipped "dva deset".
    // Croatian twenty now has dictionary rising tone; the other voices
    // and thirty retain their earlier falling-accent rendering.
    Engine engine;
    const char* voices[] = {"zvonko", "stojan", "mirsad"};
    for (const char* voice : voices) {
        REQUIRE(laprdus_set_voice(engine.handle, voice, NO_DATA) == LAPRDUS_OK);
        INFO(voice);
        std::vector<int16_t> plain = speak(engine.handle, "dvadeset");
        std::vector<int16_t> short_falling = speak(engine.handle, "dv\xC8\x81" "deset");   // dvȁdeset
        std::vector<int16_t> long_rising = speak(engine.handle, "dv\xC3\xA1" "deset");   // dvádeset
        REQUIRE(plain.size() > short_falling.size() + 22050 / 40);   // 25 ms or more
        REQUIRE(plain != short_falling);
        if (std::strcmp(voice, "zvonko") == 0) {
            REQUIRE(plain == long_rising);
        } else {
            REQUIRE(plain != long_rising);
            REQUIRE(plain == speak(engine.handle, u8"dvȃdeset"));
        }
        std::vector<int16_t> thirty = speak(engine.handle, "trideset");
        REQUIRE(thirty.size() > speak(engine.handle, "tr\xC8\x89" "deset").size() + 22050 / 40);   // trȉdeset
    }
}

TEST_CASE("Collected Croatian accents retain tone and inflection", "[formant][text]") {
    // Školski rječnik and HJP, checked 2026-10-08; see docs/formant.md.
    // Audio must equal explicitly accented input, including related forms
    // whose stress differs. Croatian's existing unstressed-length policy
    // is retained. "tekučina" in the report is the typo "tekućina".
    struct Case { const char* plain; const char* accented; };
    const Case cases[] = {
        {u8"dolar", u8"dȍlar"},
        {u8"dolara", u8"dȍlara"},
        {u8"dolari", u8"dȍlari"},
        {u8"dolarima", u8"dȍlarima"},
        {u8"euro", u8"ȅuro"},
        {u8"eura", u8"ȅura"},
        {u8"eurima", u8"ȅurima"},
        {u8"naravno", u8"náravno"},
        {u8"naravan", u8"náravan"},
        {u8"naravnoga", u8"náravnoga"},
        {u8"priča", u8"prȋča"},
        {u8"priču", u8"prȋču"},
        {u8"pričama", u8"prȋčama"},
        {u8"pričati", u8"príčati"},
        {u8"pričao", u8"príčao"},
        {u8"pričam", u8"prȋčam"},
        {u8"pričaj", u8"prȋčaj"},
        {u8"pričaju", u8"príčaju"},
        {u8"pričan", u8"prȋčan"},
        {u8"djelomično", u8"djȅlomično"},
        {u8"djelomičan", u8"djȅlomičan"},
        {u8"djelomičnoga", u8"djȅlomičnoga"},
        {u8"činjenica", u8"čȉnjenica"},
        {u8"činjenicama", u8"čȉnjenicama"},
        {u8"nemoguće", u8"nȅmoguće"},
        {u8"nemogućega", u8"nȅmogućega"},
        {u8"nemogućnost", u8"nemogúćnōst"},
        {u8"popodne", u8"popódne"},
        {u8"popodneva", u8"popódneva"},
        {u8"popodnevima", u8"popódnevima"},
        {u8"popodnevni", u8"popódnevni"},
        {u8"proslaviti", u8"pròslaviti"},
        {u8"proslavimo", u8"pròslavimo"},
        {u8"proslavite", u8"pròslavite"},
        {u8"proslavio", u8"pròslavio"},
        {u8"proslavljen", u8"pròslavljen"},
        {u8"proslavljenima", u8"pròslavljenima"},
        {u8"proslava", u8"prȍslava"},
        {u8"na proslavi", u8"na prȍslavi"},
        {u8"europska", u8"èuropska"},
        {u8"europskoga", u8"èuropskoga"},
        {u8"europskima", u8"èuropskima"},
        {u8"Europa", u8"Európa"},
        {u8"Europe", u8"Európe"},
        {u8"osiguranja", u8"osiguránja"},
        {u8"osiguranje", u8"osiguránje"},
        {u8"osiguranjima", u8"osiguránjima"},
        {u8"veselimo", u8"vesèlimo"},
        {u8"veseliti", u8"vesèliti"},
        {u8"veseli", u8"vesèli"},
        {u8"veselim", u8"vesèlim"},
        {u8"veselio", u8"vesèlio"},
        {u8"veselje", u8"vesélje"},
        {u8"automobil", u8"automòbil"},
        {u8"automobila", u8"automobíla"},
        {u8"automobilima", u8"automobílima"},
        {u8"automobilski", u8"automòbilski"},
        {u8"automobilskoga", u8"automòbilskoga"},
        {u8"tekućina", u8"tekùćina"},
        {u8"tekućine", u8"tekùćine"},
        {u8"tekućinama", u8"tekùćinama"},
        {u8"kafić", u8"kàfić"},
        {u8"kafića", u8"kafíća"},
        {u8"kafići", u8"kafíći"},
        {u8"kafićima", u8"kafíćima"},
        {u8"pomoćnik", u8"pomòćnik"},
        {u8"pomoćnika", u8"pomoćníka"},
        {u8"pomoćnici", u8"pomoćníci"},
        {u8"pomoćnicima", u8"pomoćnícima"},
        {u8"pomoćniče", u8"pȍmoćniče"},
        {u8"pomoćnica", u8"pomòćnica"},
        {u8"pomoćnicama", u8"pomòćnicama"},
        {u8"pomoćnički", u8"pomòćnički"},
        {u8"tamo", u8"tȁmo"},
        {u8"prognoza", u8"prognóza"},
        {u8"prognozama", u8"prognózama"},
        {u8"kabanica", u8"kabànica"},
        {u8"kabanicama", u8"kabànicama"},
        {u8"kišobran", u8"kȉšobran"},
        {u8"kišobrana", u8"kȉšobrana"},
        {u8"kišobranu", u8"kȉšobranu"},
        {u8"kišobrane", u8"kȉšobrane"},
        {u8"kišobranom", u8"kȉšobranom"},
        {u8"kišobrani", u8"kȉšobrani"},
        {u8"kišobranima", u8"kȉšobranima"},
        {u8"ponesi kišobran", u8"ponesi kȉšobran"},
        {u8"zaštićen", u8"zàštićen"},
        {u8"zaštićena", u8"zàštićena"},
        {u8"zaštićeno", u8"zàštićeno"},
        {u8"zaštićeni", u8"zàštićeni"},
        {u8"zaštićene", u8"zàštićene"},
        {u8"zaštićenu", u8"zàštićenu"},
        {u8"zaštićenoga", u8"zàštićenoga"},
        {u8"zaštićenome", u8"zàštićenome"},
        {u8"zaštićenomu", u8"zàštićenomu"},
        {u8"zaštićenoj", u8"zàštićenoj"},
        {u8"zaštićenih", u8"zàštićenih"},
        {u8"zaštićenim", u8"zàštićenim"},
        {u8"zaštićenima", u8"zàštićenima"},
        {u8"zaštitim", u8"zàštitim"},
        {u8"zaštitiš", u8"zàštitiš"},
        {u8"zaštitimo", u8"zàštitimo"},
        {u8"zaštititi", u8"zaštítiti"},
        {u8"zaštitio", u8"zaštítio"},
        {u8"zaštiti", u8"zaštíti"},
        {u8"zaštitite", u8"zaštítite"},
        {u8"nezaštićen", u8"nezàštīćen"},
        {u8"nezaštićenima", u8"nezàštīćenima"},
        {u8"zaštićenost", u8"zàštićenost"},
        {u8"zaštićenošću", u8"zàštićenošću"},
        {u8"dokument je zaštićen", u8"dokument je zàštićen"},
        {u8"slojevita", u8"slojèvita"},
        {u8"slojevit", u8"slojèvit"},
        {u8"slojevitima", u8"slojèvitima"},
        {u8"slojevitost", u8"slojèvitost"},
        {u8"slojevitošću", u8"slojèvitošću"},
        {u8"pet dolara", u8"pet dȍlara"},
        {u8"dva eura", u8"dva ȅura"},
        {u8"veselimo se", u8"vesèlimo se"},
        {u8"to je nemoguće", u8"to je nȅmoguće"},
        {u8"europska osiguranja", u8"èuropska osiguránja"},
        {u8"naši pomoćnici", u8"naši pomoćníci"},
    };
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    for (const auto& c : cases) {
        INFO(c.plain);
        const auto plain = speak(engine.handle, c.plain);
        REQUIRE(!plain.empty());
        REQUIRE(plain == speak(engine.handle, c.accented));
    }
}

TEST_CASE("Personal names preserve verified stress and length", "[formant][text]") {
    // Listener corrections: Enída, Ìsaković, Greblìčki and Spàsojević.
    // HJP confirms long initial e: Knȇzović (falling), Knéžević (rising).
    // Exercise the shared lexicon in every language, including Cyrillic.
    struct Case { const char* plain; const char* accented; };
    const Case cases[] = {
        {u8"Enida", u8"Enída"},
        {u8"enida", u8"enída"},
        {u8"ENIDA", u8"ENÍDA"},
        {u8"Enide", u8"Eníde"},
        {u8"Enidi", u8"Enídi"},
        {u8"Enidu", u8"Enídu"},
        {u8"Enidom", u8"Enídom"},
        {u8"Enidama", u8"Enídama"},
        {u8"Enidin", u8"Enídin"},
        {u8"Enidina", u8"Enídina"},
        {u8"Enidino", u8"Enídino"},
        {u8"Enidinoga", u8"Enídinoga"},
        {u8"Enidinim", u8"Enídinim"},
        {u8"Enidinima", u8"Enídinima"},
        {u8"Isaković", u8"Ìsaković"},
        {u8"isaković", u8"ìsaković"},
        {u8"ISAKOVIĆ", u8"ÌSAKOVIĆ"},
        {u8"Isakovića", u8"Ìsakovića"},
        {u8"Isakoviću", u8"Ìsakoviću"},
        {u8"Isakovićem", u8"Ìsakovićem"},
        {u8"Isakovići", u8"Ìsakovići"},
        {u8"Isakoviće", u8"Ìsakoviće"},
        {u8"Isakovićima", u8"Ìsakovićima"},
        {u8"Isakovićev", u8"Ìsakovićev"},
        {u8"Isakovićeva", u8"Ìsakovićeva"},
        {u8"Isakovićevo", u8"Ìsakovićevo"},
        {u8"Isakovićevoga", u8"Ìsakovićevoga"},
        {u8"Isakovićevom", u8"Ìsakovićevom"},
        {u8"Isakovićevim", u8"Ìsakovićevim"},
        {u8"Isakovićevima", u8"Ìsakovićevima"},
        {u8"Енида", u8"Enída"},
        {u8"Енидиним", u8"Enídinim"},
        {u8"Исаковић", u8"Ìsaković"},
        {u8"Исаковићевима", u8"Ìsakovićevima"},
        {u8"Enida Isaković", u8"Enída Ìsaković"},
        {u8"razgovaram s Enidom", u8"razgovaram s Enídom"},
        {u8"knjiga gospodina Isakovića", u8"knjiga gospodina Ìsakovića"},
        {u8"Greblički", u8"Greblìčki"},
        {u8"greblički", u8"greblìčki"},
        {u8"GREBLIČKI", u8"GREBLÌČKI"},
        {u8"Greblička", u8"Greblìčka"},
        {u8"Grebličke", u8"Greblìčke"},
        {u8"Grebličku", u8"Greblìčku"},
        {u8"Grebličkog", u8"Greblìčkog"},
        {u8"Grebličkoga", u8"Greblìčkoga"},
        {u8"Grebličkom", u8"Greblìčkom"},
        {u8"Grebličkome", u8"Greblìčkome"},
        {u8"Grebličkomu", u8"Greblìčkomu"},
        {u8"Grebličkoj", u8"Greblìčkoj"},
        {u8"Grebličkih", u8"Greblìčkih"},
        {u8"Grebličkim", u8"Greblìčkim"},
        {u8"Grebličkima", u8"Greblìčkima"},
        {u8"Греблички", u8"Greblìčki"},
        {u8"Гребличким", u8"Greblìčkim"},
        {u8"razgovaram s Grebličkim", u8"razgovaram s Greblìčkim"},
        {u8"Spasojević", u8"Spàsojević"},
        {u8"spasojević", u8"spàsojević"},
        {u8"SPASOJEVIĆ", u8"SPÀSOJEVIĆ"},
        {u8"Spasojevića", u8"Spàsojevića"},
        {u8"Spasojeviću", u8"Spàsojeviću"},
        {u8"Spasojevićem", u8"Spàsojevićem"},
        {u8"Spasojevići", u8"Spàsojevići"},
        {u8"Spasojeviće", u8"Spàsojeviće"},
        {u8"Spasojevićima", u8"Spàsojevićima"},
        {u8"Spasojevićev", u8"Spàsojevićev"},
        {u8"Spasojevićeva", u8"Spàsojevićeva"},
        {u8"Spasojevićevo", u8"Spàsojevićevo"},
        {u8"Spasojevićevoga", u8"Spàsojevićevoga"},
        {u8"Spasojevićevim", u8"Spàsojevićevim"},
        {u8"Spasojevićevima", u8"Spàsojevićevima"},
        {u8"Спасојевић", u8"Spàsojević"},
        {u8"Спасојевићевима", u8"Spàsojevićevima"},
        {u8"razgovaram sa Spasojevićem", u8"razgovaram sa Spàsojevićem"},
        {u8"Knezović", u8"Knȇzović"},
        {u8"knezović", u8"knȇzović"},
        {u8"KNEZOVIĆ", u8"KNȆZOVIĆ"},
        {u8"Knezovića", u8"Knȇzovića"},
        {u8"Knezoviću", u8"Knȇzoviću"},
        {u8"Knezovićem", u8"Knȇzovićem"},
        {u8"Knezovići", u8"Knȇzovići"},
        {u8"Knezoviće", u8"Knȇzoviće"},
        {u8"Knezovićima", u8"Knȇzovićima"},
        {u8"Knezovićev", u8"Knȇzovićev"},
        {u8"Knezovićeva", u8"Knȇzovićeva"},
        {u8"Knezovićevo", u8"Knȇzovićevo"},
        {u8"Knezovićevoga", u8"Knȇzovićevoga"},
        {u8"Knezovićevom", u8"Knȇzovićevom"},
        {u8"Knezovićevim", u8"Knȇzovićevim"},
        {u8"Knezovićevima", u8"Knȇzovićevima"},
        {u8"Кнезовић", u8"Knȇzović"},
        {u8"Кнезовићевима", u8"Knȇzovićevima"},
        {u8"Knežević", u8"Knéžević"},
        {u8"knežević", u8"knéžević"},
        {u8"KNEŽEVIĆ", u8"KNÉŽEVIĆ"},
        {u8"Kneževića", u8"Knéževića"},
        {u8"Kneževiću", u8"Knéževiću"},
        {u8"Kneževićem", u8"Knéževićem"},
        {u8"Kneževići", u8"Knéževići"},
        {u8"Kneževiće", u8"Knéževiće"},
        {u8"Kneževićima", u8"Knéževićima"},
        {u8"Kneževićev", u8"Knéževićev"},
        {u8"Kneževićeva", u8"Knéževićeva"},
        {u8"Kneževićevo", u8"Knéževićevo"},
        {u8"Kneževićevoga", u8"Knéževićevoga"},
        {u8"Kneževićevom", u8"Knéževićevom"},
        {u8"Kneževićevim", u8"Knéževićevim"},
        {u8"Kneževićevima", u8"Knéževićevima"},
        {u8"Кнежевић", u8"Knéžević"},
        {u8"Кнежевићевима", u8"Knéževićevima"},
        {u8"razgovaram s Knezovićem i Kneževićem", u8"razgovaram s Knȇzovićem i Knéževićem"},
    };
    Engine engine;
    for (const char* voice : FORMANT_VOICES) {
        INFO(voice);
        REQUIRE(laprdus_set_voice(engine.handle, voice, NO_DATA) == LAPRDUS_OK);
        for (const auto& c : cases) {
            INFO(c.plain);
            const auto plain = speak(engine.handle, c.plain);
            REQUIRE(!plain.empty());
            REQUIRE(plain == speak(engine.handle, c.accented));
        }
        // Check length separately from placement: Enida keeps long i;
        // Isakovic has initial stress and short a; Spasojevic has initial stress and short unstressed o.
        REQUIRE(speak(engine.handle, u8"Enida").size() >
                speak(engine.handle, u8"Enìda").size());
        REQUIRE(speak(engine.handle, u8"Isaković").size() <
                speak(engine.handle, u8"Ìsāković").size());
        REQUIRE(speak(engine.handle, u8"Isaković") !=
                speak(engine.handle, u8"Isàković"));
        REQUIRE(speak(engine.handle, u8"Spasojević").size() <
                speak(engine.handle, u8"Spàsōjević").size());
        REQUIRE(speak(engine.handle, u8"Spasojević") !=
                speak(engine.handle, u8"Spasòjević"));
        REQUIRE(speak(engine.handle, u8"Spasojević") !=
                speak(engine.handle, u8"Spasójević"));
        // Both surnames keep long e, with their distinct falling/rising tones.
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

TEST_CASE("Battery and reading-list accents follow the Croatian dictionaries", "[formant][text]") {
    // Školski rječnik / Mrežnik, checked 2026-10-08. In particular,
    // batèrija differs from HJP, and prȍčitan differs from pročìtati.
    // Croatian omits the dictionaries' unstressed lengths, as elsewhere.
    struct Case { const char* plain; const char* accented; };
    const Case cases[] = {
        {u8"baterija", u8"batèrija"},
        {u8"baterije", u8"batèrije"},
        {u8"bateriji", u8"batèriji"},
        {u8"bateriju", u8"batèriju"},
        {u8"baterijom", u8"batèrijom"},
        {u8"baterijama", u8"batèrijama"},
        {u8"baterijski", u8"batèrijski"},
        {u8"baterijskoga", u8"batèrijskoga"},
        {u8"baterijskim", u8"batèrijskim"},
        {u8"baterija je prazna", u8"batèrija je prazna"},
        {u8"poslušati", u8"pòslušati"},
        {u8"poslušat ću", u8"pòslušat ću"},
        {u8"poslušam", u8"pòslušam"},
        {u8"poslušaš", u8"pòslušaš"},
        {u8"posluša", u8"pòsluša"},
        {u8"poslušamo", u8"pòslušamo"},
        {u8"poslušate", u8"pòslušate"},
        {u8"poslušaju", u8"pòslušaju"},
        {u8"poslušaj", u8"pòslušaj"},
        {u8"poslušajmo", u8"pòslušajmo"},
        {u8"poslušajte", u8"pòslušajte"},
        {u8"poslušao", u8"pòslušao"},
        {u8"poslušala", u8"pòslušala"},
        {u8"poslušali", u8"pòslušali"},
        {u8"poslušavši", u8"pòslušavši"},
        {u8"poslušan", u8"pòslušan"},
        {u8"poslušanima", u8"pòslušanima"},
        {u8"želim poslušati poruku", u8"želim pòslušati poruku"},
        {u8"pročitati", u8"pročìtati"},
        {u8"pročitat ću", u8"pročìtat ću"},
        {u8"pročitam", u8"pročìtam"},
        {u8"pročitaš", u8"pročìtaš"},
        {u8"pročita", u8"pročìta"},
        {u8"pročitamo", u8"pročìtamo"},
        {u8"pročitate", u8"pročìtate"},
        {u8"pročitaju", u8"pročìtaju"},
        {u8"pročitaj", u8"pročìtaj"},
        {u8"pročitajmo", u8"pročìtajmo"},
        {u8"pročitajte", u8"pročìtajte"},
        {u8"pročitao", u8"pročìtao"},
        {u8"pročitala", u8"pročìtala"},
        {u8"pročitali", u8"pročìtali"},
        {u8"pročitavši", u8"pročìtavši"},
        {u8"pročitan", u8"prȍčitan"},
        {u8"pročitana", u8"prȍčitana"},
        {u8"pročitano", u8"prȍčitano"},
        {u8"pročitani", u8"prȍčitani"},
        {u8"pročitane", u8"prȍčitane"},
        {u8"pročitanu", u8"prȍčitanu"},
        {u8"pročitanoga", u8"prȍčitanoga"},
        {u8"pročitanome", u8"prȍčitanome"},
        {u8"pročitanomu", u8"prȍčitanomu"},
        {u8"pročitanoj", u8"prȍčitanoj"},
        {u8"pročitanih", u8"prȍčitanih"},
        {u8"pročitanima", u8"prȍčitanima"},
        {u8"nepročitan", u8"nepròčitan"},
        {u8"nepročitanima", u8"nepròčitanima"},
        {u8"poruka je pročitana", u8"poruka je prȍčitana"},
    };
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    for (const auto& c : cases) {
        INFO(c.plain);
        const auto plain = speak(engine.handle, c.plain);
        REQUIRE(!plain.empty());
        REQUIRE(plain == speak(engine.handle, c.accented));
    }
}

TEST_CASE("The third listener list follows the dictionaries in every voice", "[formant][text]") {
    // prijateljstvo, nevidljiv, potencijal, snalaženje, simulacijski,
    // doživjeti, poštovanje, interesantno with their families and classes
    // (HJP, Školski rječnik, Mrežnik, Rečnik Matice srpske, Alić), checked
    // 2026-10-09; see docs/formant.md. Croatian omits unstressed length.
    struct Case { const char* voice; const char* plain; const char* accented; };
    const Case cases[] = {
        {"zvonko", u8"prijateljstvo", u8"prijatèljstvo"},
        {"zvonko", u8"prijateljstva", u8"prijatèljstva"},
        {"zvonko", u8"prijateljstvom", u8"prijatèljstvom"},
        {"zvonko", u8"prijateljstava", u8"prijatèljstava"},
        {"zvonko", u8"prijateljstvima", u8"prijatèljstvima"},
        {"zvonko", u8"neprijateljstvo", u8"neprijatèljstvo"},
        {"zvonko", u8"neprijateljstava", u8"neprijàteljstava"},
        {"zvonko", u8"roditeljstvo", u8"roditèljstvo"},
        {"zvonko", u8"prijatelj", u8"prȉjatelj"},
        {"zvonko", u8"prijatelja", u8"prȉjatelja"},
        {"zvonko", u8"prijateljima", u8"prȉjateljima"},
        {"zvonko", u8"neprijatelj", u8"nèprijatelj"},
        {"zvonko", u8"prijateljica", u8"prijatèljica"},
        {"zvonko", u8"prijateljicama", u8"prijatèljicama"},
        {"zvonko", u8"prijateljski", u8"prijatèljski"},
        {"zvonko", u8"prijateljskoga", u8"prijatèljskoga"},
        {"zvonko", u8"sprijateljiti", u8"sprijatèljiti"},
        {"zvonko", u8"sprijateljio", u8"sprijatèljio"},
        {"zvonko", u8"nevidljiv", u8"nevìdljiv"},
        {"zvonko", u8"nevidljiva", u8"nevìdljiva"},
        {"zvonko", u8"nevidljivoga", u8"nevìdljivoga"},
        {"zvonko", u8"nevidljivima", u8"nevìdljivima"},
        {"zvonko", u8"nevidljivo", u8"nevìdljivo"},
        {"zvonko", u8"nevidljivost", u8"nevìdljivost"},
        {"zvonko", u8"nevidljivošću", u8"nevìdljivošću"},
        {"zvonko", u8"nevidljiviji", u8"nevidljìviji"},
        {"zvonko", u8"vidljiv", u8"vìdljiv"},
        {"zvonko", u8"razumljivo", u8"razùmljivo"},
        {"zvonko", u8"prihvatljiv", u8"prihvàtljiv"},
        {"zvonko", u8"potencijal", u8"potencìjal"},
        {"zvonko", u8"potencijala", u8"potencijála"},
        {"zvonko", u8"potencijalu", u8"potencijálu"},
        {"zvonko", u8"potencijali", u8"potencijáli"},
        {"zvonko", u8"potencijalima", u8"potencijálima"},
        {"zvonko", u8"materijala", u8"materijála"},
        {"zvonko", u8"potencijalan", u8"pȍtencijalan"},
        {"zvonko", u8"potencijalno", u8"pȍtencijalno"},
        {"zvonko", u8"potencijalnih", u8"pȍtencijalnih"},
        {"zvonko", u8"potencijalnost", u8"potencijálnōst"},
        {"zvonko", u8"snalaženje", u8"snàlaženje"},
        {"zvonko", u8"snalaženja", u8"snàlaženja"},
        {"zvonko", u8"snalaženjem", u8"snàlaženjem"},
        {"zvonko", u8"nesnalaženje", u8"nesnàlaženje"},
        {"zvonko", u8"snalaziti", u8"snàlaziti"},
        {"zvonko", u8"snalazim", u8"snàlazim"},
        {"zvonko", u8"snalazio", u8"snàlazio"},
        {"zvonko", u8"snalazeći", u8"snàlazeći"},
        {"zvonko", u8"dolaziti", u8"dòlaziti"},
        {"zvonko", u8"dolaženje", u8"dòlaženje"},
        {"zvonko", u8"pronalaziti", u8"pronàlaziti"},
        {"zvonko", u8"pronalazi", u8"pronàlazi"},
        {"zvonko", u8"snaći", u8"snȃći"},
        {"zvonko", u8"snađem", u8"snȃđem"},
        {"zvonko", u8"snađi", u8"snáđi"},
        {"zvonko", u8"snašao", u8"snàšao"},
        {"zvonko", u8"snalažljiv", u8"snalàžljiv"},
        {"zvonko", u8"snalažljivost", u8"snalàžljivost"},
        {"zvonko", u8"simulacijski", u8"simulácijski"},
        {"zvonko", u8"simulacijskoga", u8"simulácijskoga"},
        {"zvonko", u8"simulacijskima", u8"simulácijskima"},
        {"zvonko", u8"simulacija", u8"simulácija"},
        {"zvonko", u8"komunikacijski", u8"komunikácijski"},
        {"zvonko", u8"policijski", u8"polìcijski"},
        {"zvonko", u8"simulator", u8"simùlator"},
        {"zvonko", u8"simulatora", u8"simùlatora"},
        {"zvonko", u8"doživjeti", u8"dožívjeti"},
        {"zvonko", u8"doživjet ću", u8"dožívjet ću"},
        {"zvonko", u8"doživio", u8"dožívio"},
        {"zvonko", u8"doživjela", u8"dožívjela"},
        {"zvonko", u8"doživjeli", u8"dožívjeli"},
        {"zvonko", u8"doživjevši", u8"dožívjevši"},
        {"zvonko", u8"doživi", u8"dožívi"},
        {"zvonko", u8"doživite", u8"dožívite"},
        {"zvonko", u8"Doživite nešto novo", u8"Dožívite nešto novo"},
        {"zvonko", u8"doživim", u8"dožívim"},
        {"zvonko", u8"dožive", u8"dožíve"},
        {"zvonko", u8"doživljen", u8"dòživljen"},
        {"zvonko", u8"doživljeno", u8"dòživljeno"},
        {"zvonko", u8"doživljaj", u8"dȍživljaj"},
        {"zvonko", u8"doživljajima", u8"dȍživljajima"},
        {"zvonko", u8"preživjeti", u8"prežívjeti"},
        {"zvonko", u8"preživio", u8"prežívio"},
        {"zvonko", u8"preživljen", u8"prèživljen"},
        {"zvonko", u8"živjeti", u8"žívjeti"},
        {"zvonko", u8"živio", u8"žívio"},
        {"zvonko", u8"poštovanje", u8"poštovánje"},
        {"zvonko", u8"poštovanja", u8"poštovánja"},
        {"zvonko", u8"poštovanjem", u8"poštovánjem"},
        {"zvonko", u8"nepoštovanje", u8"nepoštovánje"},
        {"zvonko", u8"Poštovanje!", u8"Poštovánje!"},
        {"zvonko", u8"poštovati", u8"poštòvati"},
        {"zvonko", u8"poštovao", u8"poštòvao"},
        {"zvonko", u8"poštujem", u8"pòštujem"},
        {"zvonko", u8"poštovan", u8"pȍštovan"},
        {"zvonko", u8"poštovani", u8"pȍštovani"},
        {"zvonko", u8"Poštovani korisnici", u8"Pȍštovani korisnici"},
        {"zvonko", u8"poštovatelj", u8"poštòvatelj"},
        {"zvonko", u8"interesantno", u8"interesàntno"},
        {"zvonko", u8"interesantan", u8"interesàntan"},
        {"zvonko", u8"interesantna", u8"interesàntna"},
        {"zvonko", u8"interesantnoga", u8"interesàntnoga"},
        {"zvonko", u8"interesantniji", u8"interesàntniji"},
        {"zvonko", u8"interesantnost", u8"interesàntnost"},
        {"zvonko", u8"elegantno", u8"elegàntno"},
        {"zvonko", u8"kompetentni", u8"kompetèntni"},
        {"zvonko", u8"formantni", u8"fòrmantni"},
        {"zvonko", u8"interes", u8"ȉnteres"},
        {"zvonko", u8"interesima", u8"ȉnteresima"},
        {"stojan", u8"prijateljstvo", u8"prijatéljstvo"},
        {"stojan", u8"nevidljiv", u8"nevìdljiv"},
        {"stojan", u8"potencijala", u8"potencijála"},
        {"stojan", u8"potencijalno", u8"pȍtencijālno"},
        {"stojan", u8"snalaženje", u8"snàlažēnje"},
        {"stojan", u8"snaći", u8"snáći"},
        {"stojan", u8"simulacijski", u8"simulácījski"},
        {"stojan", u8"doživeti", u8"dožíveti"},
        {"stojan", u8"doživeo", u8"dožíveo"},
        {"stojan", u8"doživim", u8"dožívīm"},
        {"stojan", u8"doživljaj", u8"dȍživljāj"},
        {"stojan", u8"doživljavaju", u8"doživljávaju"},
        {"stojan", u8"poštovanje", u8"poštovánje"},
        {"stojan", u8"interesantno", u8"interesàntno"},
        {"stojan", u8"interesovati", u8"ȉnteresovati"},
        {"stojan", u8"interesujem", u8"ȉnteresujēm"},
        {"stojan", u8"otvaraju", u8"otváraju"},
        {"stojan", u8"poštovaću", u8"poštòvaću"},
        // Words around these rules that keep their own accent (review of
        // 2026-10-09): the stems of the lexicon win over the new rules
        {"zvonko", u8"austrijskog", u8"àustrijskog"},
        {"zvonko", u8"australijski", u8"aùstralijski"},
        {"zvonko", u8"cementni", u8"cèmentni"},
        {"zvonko", u8"momentna", u8"mòmentna"},
        {"zvonko", u8"prijateljuju", u8"prijatèljuju"},
        {"zvonko", u8"preživljenje", u8"preživljénje"},
        {"zvonko", u8"preživjelih", u8"prežívjelih"},
        {"zvonko", u8"poštovateljica", u8"poštovatèljica"},
        {"zvonko", u8"snalazi", u8"snàlazi"},
        {"zvonko", u8"vjerovanje", u8"vjȅrovanje"},
        {"zvonko", u8"ljetovati", u8"ljȅtovati"},
        {"stojan", u8"verovaćemo", u8"vȅrovaćemo"},
        {"stojan", u8"interesovaćeš", u8"ȉnteresovaćeš"},
        {"mirsad", u8"prijatelj", u8"prìjatelj"},
        {"mirsad", u8"prijateljstvo", u8"prijatèljstvo"},
        {"mirsad", u8"doživjeti", u8"dožívjeti"},
        {"mirsad", u8"doživim", u8"dožívīm"},
        {"mirsad", u8"snalaženje", u8"snàlažēnje"},
        {"mirsad", u8"poštovanje", u8"poštovánje"},
    };
    Engine engine;
    for (const auto& c : cases) {
        INFO(c.voice << " " << c.plain);
        REQUIRE(laprdus_set_voice(engine.handle, c.voice, NO_DATA) == LAPRDUS_OK);
        const auto plain = speak(engine.handle, c.plain);
        REQUIRE(!plain.empty());
        REQUIRE(plain == speak(engine.handle, c.accented));
    }
    // The old readings are gone
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    REQUIRE(speak(engine.handle, u8"snalaženje") != speak(engine.handle, u8"snalažénje"));
    REQUIRE(speak(engine.handle, u8"poštovanje") != speak(engine.handle, u8"poštòvanje"));
    REQUIRE(speak(engine.handle, u8"doživjeti") != speak(engine.handle, u8"dožìvjeti"));
    REQUIRE(speak(engine.handle, u8"poštovati") != speak(engine.handle, u8"pȍštovati"));
}

TEST_CASE("The fourth listener list follows the dictionaries in every voice", "[formant][text]") {
    // Egipat, proizvodnja, vodič, napomene with their families (HJP, Školski
    // rječnik, Wiktionary, Rečnik Matice srpske), checked 2026-10-09; see
    // docs/formant.md. Croatian omits unstressed length.
    struct Case { const char* voice; const char* plain; const char* accented; };
    const Case cases[] = {
        {"zvonko", u8"Egipat", u8"Ègipat"},
        {"zvonko", u8"Egipta", u8"Ègipta"},
        {"zvonko", u8"Egiptu", u8"Ègiptu"},
        {"zvonko", u8"Egiptom", u8"Ègiptom"},
        {"zvonko", u8"egipatski", u8"ègipatski"},
        {"zvonko", u8"egipatskoga", u8"ègipatskoga"},
        {"zvonko", u8"Egipćanin", u8"Ègipćanin"},
        {"zvonko", u8"Egipćani", u8"Ègipćani"},
        {"zvonko", u8"Egipćanima", u8"Ègipćanima"},
        {"zvonko", u8"Egipćanka", u8"Ègipćanka"},
        {"zvonko", u8"proizvodnja", u8"proizvòdnja"},
        {"zvonko", u8"proizvodnje", u8"proizvòdnje"},
        {"zvonko", u8"proizvodnji", u8"proizvòdnji"},
        {"zvonko", u8"proizvodnju", u8"proizvòdnju"},
        {"zvonko", u8"proizvodnjom", u8"proizvòdnjom"},
        {"zvonko", u8"proizvodnjama", u8"proizvòdnjama"},
        {"zvonko", u8"proizvod", u8"proìzvod"},
        {"zvonko", u8"proizvoda", u8"proìzvoda"},
        {"zvonko", u8"proizvodi", u8"proìzvodi"},
        {"zvonko", u8"Proizvodi", u8"Proìzvodi"},
        {"zvonko", u8"proizvodima", u8"proìzvodima"},
        {"zvonko", u8"proizvesti", u8"proìzvesti"},
        {"zvonko", u8"proizveo", u8"proìzveo"},
        {"zvonko", u8"proizvedem", u8"proizvèdem"},
        {"zvonko", u8"proizveden", u8"proizvèden"},
        {"zvonko", u8"proizvedeno", u8"proizvèdeno"},
        {"zvonko", u8"proizvoditi", u8"proizvòditi"},
        {"zvonko", u8"proizvodio", u8"proizvòdio"},
        {"zvonko", u8"proizvodim", u8"proizvòdim"},
        {"zvonko", u8"proizvođenje", u8"proìzvođenje"},
        {"zvonko", u8"proizvođača", u8"proizvođáča"},
        {"zvonko", u8"vodič", u8"vòdič"},
        {"zvonko", u8"vodiča", u8"vodíča"},
        {"zvonko", u8"vodiču", u8"vodíču"},
        {"zvonko", u8"vodičem", u8"vodíčem"},
        {"zvonko", u8"vodiči", u8"vodíči"},
        {"zvonko", u8"vodiče", u8"vodíče"},
        {"zvonko", u8"vodičima", u8"vodíčima"},
        {"zvonko", u8"poluvodič", u8"poluvòdič"},
        {"zvonko", u8"poluvodiča", u8"poluvodíča"},
        {"zvonko", u8"napomena", u8"nȁpomena"},
        {"zvonko", u8"napomene", u8"nȁpomene"},
        {"zvonko", u8"Dodaj napomene", u8"Dodaj nȁpomene"},
        {"zvonko", u8"u napomeni", u8"u nȁpomeni"},
        {"zvonko", u8"napomenu", u8"nȁpomenu"},
        {"zvonko", u8"napomenom", u8"nȁpomenom"},
        {"zvonko", u8"napomenama", u8"nȁpomenama"},
        {"zvonko", u8"kad napomene", u8"kad nȁpomene"},
        {"zvonko", u8"Napomeni mu", u8"Napoméni mu"},
        {"zvonko", u8"napomenuti", u8"napoménuti"},
        {"zvonko", u8"napomenuo", u8"napoménuo"},
        {"zvonko", u8"napominjati", u8"napòminjati"},
        {"zvonko", u8"napominjem", u8"napòminjem"},
        {"zvonko", u8"napominjao", u8"napòminjao"},
        {"stojan", u8"Egipat", u8"Ègipat"},
        {"stojan", u8"Egipćanka", u8"Ègipćānka"},
        {"stojan", u8"proizvodnja", u8"proizvòdnja"},
        {"stojan", u8"proizvod", u8"proìzvod"},
        {"stojan", u8"proizvedeno", u8"proizvèdeno"},
        {"stojan", u8"proizvodim", u8"proìzvodim"},
        {"stojan", u8"vodič", u8"vòdīč"},
        {"stojan", u8"vodiča", u8"vodíča"},
        {"stojan", u8"poluvodič", u8"poluvòdīč"},
        {"stojan", u8"napomena", u8"nȁpomena"},
        {"stojan", u8"napomene", u8"nȁpomene"},
        {"stojan", u8"napomenuti", u8"napoménuti"},
        {"stojan", u8"napominjem", u8"napòminjem"},
        {"mirsad", u8"Egipat", u8"Ègipat"},
        {"mirsad", u8"proizvodnja", u8"proizvòdnja"},
        {"mirsad", u8"vodič", u8"vòdīč"},
        {"mirsad", u8"vodiču", u8"vodíču"},
        {"mirsad", u8"napomene", u8"nȁpomene"},
    };
    Engine engine;
    for (const auto& c : cases) {
        INFO(c.voice << " " << c.plain);
        REQUIRE(laprdus_set_voice(engine.handle, c.voice, NO_DATA) == LAPRDUS_OK);
        const auto plain = speak(engine.handle, c.plain);
        REQUIRE(!plain.empty());
        REQUIRE(plain == speak(engine.handle, c.accented));
    }
    // The old readings are gone
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    REQUIRE(speak(engine.handle, u8"Egipat") != speak(engine.handle, u8"Egìpat"));
    REQUIRE(speak(engine.handle, u8"napomene") != speak(engine.handle, u8"napoméne"));
}

TEST_CASE("The sixth listener list follows the dictionaries in every voice", "[formant][text]") {
    // karijera ... ruža with their families (Školski rječnik, HJP, Hrvatski
    // mrežni rječnik, Wiktionary, Rečnik Matice srpske, Alić), checked
    // 2026-10-09; see docs/formant.md. Croatian omits unstressed length.
    struct Case { const char* voice; const char* plain; const char* accented; };
    const Case cases[] = {
        // The loans in -ijer: four syllables, not the jat of rijeka
        {"zvonko", u8"karijera", u8"karijéra"},
        {"zvonko", u8"karijerom", u8"karijérom"},
        {"zvonko", u8"premijer", u8"premìjer"},
        {"zvonko", u8"premijera", u8"premijéra"},
        {"zvonko", u8"premijeri", u8"premijéri"},
        {"zvonko", u8"barijera", u8"barijéra"},
        {"zvonko", u8"hotelijer", u8"hotelìjer"},
        {"zvonko", u8"interijer", u8"interìjer"},
        {"zvonko", u8"hijerarhija", u8"hijeràrhija"},
        {"zvonko", u8"umjetnik", u8"ùmjetnik"},
        {"zvonko", u8"umjetnici", u8"ùmjetnici"},
        {"zvonko", u8"umjetnost", u8"ùmjetnost"},
        {"zvonko", u8"umjetnošću", u8"ùmjetnošću"},
        {"zvonko", u8"umjetnički", u8"ùmjetnički"},
        {"zvonko", u8"umjetničkoga", u8"ùmjetničkoga"},
        {"zvonko", u8"album", u8"àlbum"},
        {"zvonko", u8"albuma", u8"albúma"},
        {"zvonko", u8"albumima", u8"albúmima"},
        {"zvonko", u8"jedinstven", u8"jedìnstven"},
        {"zvonko", u8"jedinstvenoga", u8"jedìnstvenoga"},
        {"zvonko", u8"jedinstvo", u8"jedínstvo"},
        {"zvonko", u8"jedinstveniji", u8"jedinstvèniji"},
        {"zvonko", u8"kompromis", u8"kompròmis"},
        {"zvonko", u8"kompromisima", u8"kompròmisima"},
        {"zvonko", u8"kompromisan", u8"kompròmisan"},
        {"zvonko", u8"kompromisnoga", u8"kompròmisnoga"},
        {"zvonko", u8"beskompromisan", u8"bèskompromisan"},
        {"zvonko", u8"beskompromisna", u8"bèskompromisna"},
        {"zvonko", u8"zanima", u8"zànima"},
        {"zvonko", u8"zanimam", u8"zànimam"},
        {"zvonko", u8"zanimaju", u8"zanímaju"},
        {"zvonko", u8"zanimati", u8"zanímati"},
        {"zvonko", u8"zanimalo", u8"zanímalo"},
        {"zvonko", u8"zanimanje", u8"zanímanje"},
        {"zvonko", u8"plaćenik", u8"plȃćenik"},
        {"zvonko", u8"plaćenici", u8"plȃćenici"},
        {"zvonko", u8"plačenik", u8"plȃčenik"},
        {"zvonko", u8"povijest", u8"pȍvijest"},
        {"zvonko", u8"povijesni", u8"pȍvijesni"},
        {"zvonko", u8"ostavština", u8"ostávština"},
        {"zvonko", u8"ostavštinom", u8"ostávštinom"},
        {"zvonko", u8"odijelo", u8"odijélo"},
        {"zvonko", u8"odijelima", u8"odijélima"},
        {"zvonko", u8"odjelo", u8"odjélo"},
        {"zvonko", u8"obilježila", u8"obìlježila"},
        {"zvonko", u8"gubitak", u8"gubítak"},
        {"zvonko", u8"gubitka", u8"gubítka"},
        {"zvonko", u8"gubici", u8"gubíci"},
        {"zvonko", u8"gubicima", u8"gubícima"},
        {"zvonko", u8"gubitaka", u8"gùbitaka"},
        {"zvonko", u8"dobitak", u8"dobítak"},
        {"zvonko", u8"užitak", u8"užítak"},
        {"zvonko", u8"imutak", u8"imútak"},
        {"zvonko", u8"razvitak", u8"razvítak"},
        {"zvonko", u8"debitant", u8"debìtant"},
        {"zvonko", u8"debitantica", u8"debìtantica"},
        {"zvonko", u8"debitantski", u8"debìtantski"},
        {"zvonko", u8"debitantskoga", u8"debìtantskoga"},
        {"zvonko", u8"neukrotiv", u8"neukròtiv"},
        {"zvonko", u8"neukrotivoga", u8"neukròtivoga"},
        {"zvonko", u8"ljubav", u8"ljúbav"},
        {"zvonko", u8"ljubavlju", u8"ljúbavlju"},
        {"zvonko", u8"ljubavni", u8"ljúbavni"},
        {"zvonko", u8"debil", u8"dèbil"},
        {"zvonko", u8"debila", u8"debíla"},
        {"zvonko", u8"izdanje", u8"izdánje"},
        {"zvonko", u8"izdanjima", u8"izdánjima"},
        {"zvonko", u8"nezaobilazan", u8"nezaobìlazan"},
        {"zvonko", u8"nezaobilaznoga", u8"nezaobìlaznoga"},
        {"zvonko", u8"zaobilaznica", u8"zaobìlaznica"},
        {"zvonko", u8"boja", u8"bòja"},
        {"zvonko", u8"bojama", u8"bòjama"},
        {"zvonko", u8"teško", u8"tȇško"},
        {"zvonko", u8"teški", u8"tȇški"},
        {"zvonko", u8"teška", u8"téška"},
        {"zvonko", u8"težak", u8"téžak"},
        {"zvonko", u8"teže", u8"tȅže"},
        {"zvonko", u8"Ljetni odmori", u8"Ljetni òdmori"},
        {"zvonko", u8"Odmori se", u8"Odmòri se"},
        {"zvonko", u8"odmor", u8"òdmor"},
        {"zvonko", u8"odmoriti", u8"odmòriti"},
        {"zvonko", u8"odmorim", u8"òdmorim"},
        {"zvonko", u8"odmoren", u8"òdmoren"},
        {"zvonko", u8"bol", u8"bȏl"},
        {"zvonko", u8"bola", u8"bȏla"},
        {"zvonko", u8"Bez boli", u8"Bez bȏli"},
        {"zvonko", u8"u boli", u8"u bóli"},
        {"zvonko", u8"o bolu", u8"o bólu"},
        {"zvonko", u8"bolovi", u8"bȍlovi"},
        {"zvonko", u8"Boli me glava", u8"Bòli me glava"},
        {"zvonko", u8"Glava me jako boli", u8"Glava me jako bòli"},
        {"zvonko", u8"Ne boli", u8"Ne bòli"},
        {"zvonko", u8"Gdje boli", u8"Gdje bòli"},
        {"zvonko", u8"Mene boli glava", u8"Mene bòli glava"},
        {"zvonko", u8"Glava me i dalje boli", u8"Glava me i dalje bòli"},
        {"zvonko", u8"Zbog jake boli ga je odvezla", u8"Zbog jake bȏli ga je odvezla"},
        {"zvonko", u8"Lijek mu ublažava boli", u8"Lijek mu ublažava bȏli"},
        {"zvonko", u8"Te boli su prošle", u8"Te bȏli su prošle"},
        {"zvonko", u8"Žali se na boli", u8"Žali se na bȏli"},
        {"zvonko", u8"hijeroglif", u8"hijeròglif"},
        {"zvonko", u8"arhijerej", u8"arhijèrej"},
        {"zvonko", u8"zaobilaznicom", u8"zaobìlaznicom"},
        {"zvonko", u8"bolno", u8"bȏlno"},
        {"zvonko", u8"bolna", u8"bólna"},
        {"zvonko", u8"neprikosnoven", u8"neprikosnòven"},
        {"zvonko", u8"neprekosnoven", u8"neprekosnòven"},
        {"zvonko", u8"krhotina", "kr\xCC\x80hotina"},
        {"zvonko", u8"krhotinama", "kr\xCC\x80hotinama"},
        {"zvonko", u8"tuga", u8"túga"},
        {"zvonko", u8"tuzi", u8"túzi"},
        {"zvonko", u8"tužno", u8"tȗžno"},
        {"zvonko", u8"tužan", u8"túžan"},
        {"zvonko", u8"tužna", u8"túžna"},
        {"zvonko", u8"tužni", u8"tȗžni"},
        {"zvonko", u8"tužne", u8"tȗžne"},
        {"zvonko", u8"tužniji", u8"tùžniji"},
        {"zvonko", u8"Bila je duga noć", u8"Bila je dùga noć"},
        {"zvonko", u8"važno", u8"vȃžno"},
        {"zvonko", u8"važan", u8"vážan"},
        {"zvonko", u8"važnost", u8"vážnost"},
        {"zvonko", u8"važniji", u8"vàžniji"},
        {"zvonko", u8"lažno", u8"lȁžno"},
        {"zvonko", u8"lažna", u8"làžna"},
        {"zvonko", u8"laž", u8"lȃž"},
        {"zvonko", u8"laži", u8"lȁži"},
        {"zvonko", u8"lažima", u8"làžima"},
        {"zvonko", u8"ruža", u8"rúža"},
        {"zvonko", u8"ružama", u8"rúžama"},
        {"stojan", u8"karijera", u8"karijéra"},
        {"stojan", u8"premijer", u8"premìjēr"},
        {"stojan", u8"umetnik", u8"ùmetnīk"},
        {"stojan", u8"umetnost", u8"ùmetnōst"},
        {"stojan", u8"album", u8"àlbūm"},
        {"stojan", u8"beskompromisan", u8"bȅskompromisan"},
        {"stojan", u8"zanima", u8"zànīma"},
        {"stojan", u8"plaćenik", u8"plȃćenīk"},
        {"stojan", u8"odelo", u8"odélo"},
        {"stojan", u8"obeležen", u8"obèležen"},
        {"stojan", u8"gubitaka", u8"gùbītākā"},
        {"stojan", u8"debil", u8"dèbīl"},
        {"stojan", u8"ljubavnik", u8"ljúbāvnīk"},
        {"stojan", u8"nezaobilazan", u8"nȅzaobilāzan"},
        {"stojan", u8"tugom", u8"túgōm"},
        {"stojan", u8"tužan", u8"tȗžan"},
        {"zvonko", u8"zvijer", u8"zvȋjer"},
        {"mirsad", u8"umjetnik", u8"ùmjetnīk"},
        {"mirsad", u8"gubitaka", u8"gubítākā"},
        {"mirsad", u8"ljubav", u8"ljúbav"},
        {"mirsad", u8"izdanje", u8"izdánje"},
        {"mirsad", u8"bojom", u8"bòjōm"},
    };
    Engine engine;
    for (const auto& c : cases) {
        INFO(c.voice << " " << c.plain);
        REQUIRE(laprdus_set_voice(engine.handle, c.voice, NO_DATA) == LAPRDUS_OK);
        const auto plain = speak(engine.handle, c.plain);
        REQUIRE(!plain.empty());
        REQUIRE(plain == speak(engine.handle, c.accented));
    }
    // The old readings are gone
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    REQUIRE(speak(engine.handle, u8"zanima") != speak(engine.handle, u8"zaníma"));
    REQUIRE(speak(engine.handle, u8"krhotina") != speak(engine.handle, u8"krhotína"));
    REQUIRE(speak(engine.handle, u8"bolnica") != speak(engine.handle, u8"bȏlnica"));
    // odjela, odjelu are the cases of òdjel, not of odijelo
    REQUIRE(speak(engine.handle, u8"odjela") != speak(engine.handle, u8"odjéla"));
    // Jat before r stays one syllable: zvijer, prijeratni
    REQUIRE(speak(engine.handle, u8"zvijer") == speak(engine.handle, u8"zvȋjer"));
}

TEST_CASE("The seventh and eighth listener lists follow the dictionaries in every voice", "[formant][text]") {
    // nikad, nikada, poboljšanja, reproducirano with their families (HJP,
    // Školski rječnik, Rečnik Matice srpske), checked 2026-10-09
    struct Case { const char* voice; const char* plain; const char* accented; };
    const Case cases[] = {
        {"zvonko", u8"nikad", u8"nȉkad"},
        {"zvonko", u8"nikada", u8"nȉkada"},
        {"zvonko", u8"ikada", u8"ȉkada"},
        {"zvonko", u8"nekada", u8"nȅkada"},
        {"zvonko", u8"katkada", u8"kȁtkada"},
        {"zvonko", u8"otkada", u8"òtkada"},
        {"zvonko", u8"ponekad", u8"pònekad"},
        {"zvonko", u8"kadikada", u8"kadìkada"},
        {"zvonko", u8"poboljšanje", u8"poboljšánje"},
        {"zvonko", u8"poboljšanja", u8"poboljšánja"},
        {"zvonko", u8"poboljšanjima", u8"poboljšánjima"},
        {"zvonko", u8"poboljšati", u8"pobòljšati"},
        {"zvonko", u8"obećanje", u8"obećánje"},
        {"zvonko", u8"povećanje", u8"povećánje"},
        {"zvonko", u8"pojačanje", u8"pojačánje"},
        {"zvonko", u8"pogoršanje", u8"pogoršánje"},
        {"zvonko", u8"olakšanje", u8"olakšánje"},
        {"zvonko", u8"reproducirano", u8"reprodùcirano"},
        {"zvonko", u8"reproducira", u8"reprodùcira"},
        {"zvonko", u8"reproduciram", u8"reprodùciram"},
        {"zvonko", u8"reproducirati", u8"reproducírati"},
        {"zvonko", u8"reproduciraju", u8"reproducíraju"},
        {"zvonko", u8"reproducirao", u8"reproducírao"},
        {"zvonko", u8"reproduciranje", u8"reproducíranje"},
        {"zvonko", u8"reproduktivan", u8"rȅproduktivan"},
        {"stojan", u8"nikada", u8"nȉkada"},
        {"stojan", u8"poboljšanje", u8"poboljšánje"},
        {"stojan", u8"reproducirano", u8"reprodùcīrano"},
        {"mirsad", u8"nikad", u8"nȉkad"},
        {"mirsad", u8"obećanja", u8"obećánja"},
        // The eighth list (2026-10-09)
        {"zvonko", u8"karta", u8"kȃrta"},
        {"zvonko", u8"karte", u8"kȃrte"},
        {"zvonko", u8"kartama", u8"kȃrtama"},
        {"zvonko", u8"karata", u8"kȁrata"},
        {"zvonko", u8"kartica", u8"kàrtica"},
        {"zvonko", u8"zdravlje", u8"zdrȃvlje"},
        {"zvonko", u8"zdravljem", u8"zdrȃvljem"},
        {"zvonko", u8"zdravstvo", u8"zdràvstvo"},
        {"zvonko", u8"zdravstvenoga", u8"zdràvstvenoga"},
        {"stojan", u8"karata", u8"kȁrātā"},
        {"mirsad", u8"zdravlje", u8"zdrȃvlje"},
        {"zvonko", u8"privitak", u8"privítak"},
        {"zvonko", u8"privitku", u8"privítku"},
        {"zvonko", u8"privici", u8"privíci"},
        {"zvonko", u8"privicima", u8"privícima"},
        {"zvonko", u8"privitci", u8"privítci"},
        {"zvonko", u8"privitaka", u8"prìvitaka"},
        {"zvonko", u8"privijen", u8"prìvijen"},
        {"stojan", u8"privitaka", u8"prìvītākā"},
        {"mirsad", u8"privitaka", u8"privítākā"},
        // The ninth list (2026-10-09)
        {"zvonko", u8"različit", u8"rázličit"},
        {"zvonko", u8"različitoga", u8"rázličitoga"},
        {"zvonko", u8"različitost", u8"rázličitost"},
        {"zvonko", u8"različitosti", u8"rázličitosti"},
        {"zvonko", u8"različitiji", u8"različìtiji"},
        {"zvonko", u8"strana", u8"strána"},
        {"zvonko", u8"S druge strane", u8"S druge stráne"},
        {"zvonko", u8"stranama", u8"stránama"},
        {"zvonko", u8"stranu", u8"strȃnu"},
        {"zvonko", u8"Na lijevoj strani", u8"Na lijevoj stráni"},
        {"zvonko", u8"Strani jezik", u8"Strȃni jezik"},
        {"zvonko", u8"dokumenti", u8"dokùmenti"},
        {"zvonko", u8"dokumenata", u8"dokùmenata"},
        {"zvonko", u8"Danijela", u8"Dànijela"},
        {"stojan", u8"različitost", u8"rázličitōst"},
        {"stojan", u8"dokumenata", u8"dokùmenātā"},
        // vijest (2026-10-09)
        {"zvonko", u8"vijest", u8"vijȇst"},
        {"zvonko", u8"vijesti", u8"vijȇsti"},
        {"zvonko", u8"vijestima", u8"vijéstima"},
        {"zvonko", u8"viješću", u8"vijȇšću"},
        {"zvonko", u8"o toj vijesti", u8"o toj vijésti"},
        {"zvonko", u8"na vijesti", u8"na vijȇsti"},
        {"stojan", u8"vesti", u8"vȇsti"},
        {"stojan", u8"vestima", u8"véstima"},
        // razmak, razmaknica, razmaknuti; ipsilon (2026-10-09)
        {"zvonko", u8"razmak", u8"rázmak"},
        {"zvonko", u8"razmaka", u8"rázmaka"},
        {"zvonko", u8"razmaci", u8"rázmaci"},
        {"zvonko", u8"razmacima", u8"rázmacima"},
        {"zvonko", u8"razmaknica", u8"rázmaknica"},
        {"zvonko", u8"Pritisni razmaknicu", u8"Pritisni rázmaknicu"},
        {"zvonko", u8"razmaknuti", u8"razmàknuti"},
        {"zvonko", u8"razmaknem", u8"ràzmaknem"},
        {"zvonko", u8"Razmakni stolove", u8"Razmàkni stolove"},
        {"zvonko", u8"razmaknuo", u8"razmàknuo"},
        {"zvonko", u8"razmaknut", u8"ràzmaknut"},
        {"zvonko", u8"razmicati", u8"ràzmicati"},
        {"zvonko", u8"razmičem", u8"ràzmičem"},
        {"zvonko", u8"ipsilon", u8"ȉpsilon"},
        {"zvonko", u8"epsilon", u8"èpsilon"},
        {"stojan", u8"razmaka", u8"rázmākā"},
        {"stojan", u8"ipsilon", u8"ȉpsilōn"},
        // carstvo, kraljevstvo (2026-10-09)
        {"zvonko", u8"carstvo", u8"cȃrstvo"},
        {"zvonko", u8"carstvima", u8"cȃrstvima"},
        {"zvonko", u8"carstava", u8"cȃrstava"},
        {"zvonko", u8"carski", u8"cȃrski"},
        {"zvonko", u8"kraljevstvo", u8"králjevstvo"},
        {"zvonko", u8"kraljevstvom", u8"králjevstvom"},
        {"zvonko", u8"kraljevstava", u8"králjevstava"},
        {"stojan", u8"carstava", u8"cȃrstāvā"},
        {"mirsad", u8"kraljevstava", u8"králjevstāvā"},
    };
    Engine engine;
    for (const auto& c : cases) {
        INFO(c.voice << " " << c.plain);
        REQUIRE(laprdus_set_voice(engine.handle, c.voice, NO_DATA) == LAPRDUS_OK);
        const auto plain = speak(engine.handle, c.plain);
        REQUIRE(!plain.empty());
        REQUIRE(plain == speak(engine.handle, c.accented));
    }
    // The old readings are gone, and the loans in -ada keep theirs
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    REQUIRE(speak(engine.handle, u8"nikada") != speak(engine.handle, u8"nikáda"));
    REQUIRE(speak(engine.handle, u8"blokada") == speak(engine.handle, u8"blokáda"));
    REQUIRE(speak(engine.handle, u8"barikada") == speak(engine.handle, u8"barikáda"));
}

TEST_CASE("Age and time compounds and čestitka follow the dictionaries", "[formant][text]") {
    // The compounds of -godišnji, -godišnjak, -godišnjica, -godište,
    // -mjesečni, -dnevni, -tjedni, -nedeljni and -ljetan
    // (StressRules::time_compound), gòdišnjāk, čèstitka and čestítati (HJP,
    // Školski rječnik, Rečnik Matice srpske), checked 2026-10-09; see
    // docs/formant.md. Croatian omits unstressed length.
    struct Case { const char* voice; const char* plain; const char* accented; };
    const Case cases[] = {
        {"zvonko", u8"petogodišnji", u8"petogòdišnji"},
        {"zvonko", u8"petogodišnja", u8"petogòdišnja"},
        {"zvonko", u8"petogodišnjeg", u8"petogòdišnjeg"},
        {"zvonko", u8"šestogodišnjoj", u8"šestogòdišnjoj"},
        {"zvonko", u8"sedmogodišnji", u8"sedmogòdišnji"},
        {"zvonko", u8"osmogodišnjem", u8"osmogòdišnjem"},
        {"zvonko", u8"desetogodišnje", u8"desetogòdišnje"},
        {"zvonko", u8"stogodišnji", u8"stogòdišnji"},
        {"zvonko", u8"dvadesetogodišnjak", u8"dvadesetogòdišnjak"},
        {"zvonko", u8"dvadesetpetogodišnji", u8"dvadesetpetogòdišnji"},
        {"zvonko", u8"dugogodišnji", u8"dugogòdišnji"},
        {"zvonko", u8"novogodišnja", u8"novogòdišnja"},
        {"zvonko", u8"stogodišnjica", u8"stogòdišnjica"},
        {"zvonko", u8"desetogodišnjicu", u8"desetogòdišnjicu"},
        {"zvonko", u8"petogodišnjak", u8"petogòdišnjak"},
        {"zvonko", u8"petogodišnjaka", u8"petogòdišnjaka"},
        {"zvonko", u8"petogodišnjakinja", u8"petogodišnjàkinja"},
        {"zvonko", u8"polugodište", u8"polugòdište"},
        {"zvonko", u8"polugodišta", u8"polugòdišta"},
        {"zvonko", u8"šestomjesečna", u8"šestòmjesečna"},
        {"zvonko", u8"devetomjesečni", u8"devetòmjesečni"},
        {"zvonko", u8"višemjesečnog", u8"višèmjesečnog"},
        {"zvonko", u8"tromjesečni", u8"tròmjesečni"},
        {"zvonko", u8"petodnevni", u8"petòdnevni"},
        {"zvonko", u8"jednodnevnog", u8"jednòdnevnog"},
        {"zvonko", u8"trotjedni", u8"tròtjedni"},
        {"zvonko", u8"maloljetan", u8"malòljetan"},
        {"zvonko", u8"maloljetna", u8"malòljetna"},
        {"zvonko", u8"maloljetnik", u8"malòljetnik"},
        {"zvonko", u8"maloljetnici", u8"malòljetnici"},
        {"zvonko", u8"punoljetnost", u8"punòljetnost"},
        {"zvonko", u8"stoljetni", u8"stòljetni"},
        {"zvonko", u8"godišnjak", u8"gòdišnjak"},
        {"zvonko", u8"godišnjaka", u8"gòdišnjaka"},
        {"zvonko", u8"čestitka", u8"čèstitka"},
        {"zvonko", u8"čestitke", u8"čèstitke"},
        {"zvonko", u8"čestitku", u8"čèstitku"},
        {"zvonko", u8"čestitaka", u8"čèstitaka"},
        {"zvonko", u8"čestitati", u8"čestítati"},
        {"zvonko", u8"čestitam", u8"čèstitam"},
        {"zvonko", u8"čestitao", u8"čestítao"},
        {"zvonko", u8"čestitaju", u8"čestítaju"},
        {"zvonko", u8"čestitaj", u8"čèstitaj"},
        {"stojan", u8"petogodišnji", u8"petogòdišnji"},
        {"stojan", u8"stogodišnjica", u8"stogòdišnjica"},
        {"stojan", u8"šestomesečna", u8"šestomèsečna"},
        {"stojan", u8"devetomesečni", u8"devetomèsečni"},
        {"stojan", u8"petodnevni", u8"petòdnēvni"},
        {"stojan", u8"dvonedeljni", u8"dvonèdēljni"},
        {"stojan", u8"maloletan", u8"malòletan"},
        {"stojan", u8"punoletnost", u8"punòletnost"},
        {"stojan", u8"polugodište", u8"polugòdīšte"},
        {"stojan", u8"godišnjak", u8"gòdišnjāk"},
        {"stojan", u8"čestitka", u8"čèstitka"},
        {"stojan", u8"čestitam", u8"čèstītam"},
        {"stojan", u8"čestitati", u8"čestítati"},
        {"mirsad", u8"šestomjesečna", u8"šestomjèsečna"},
        {"mirsad", u8"petogodišnji", u8"petogòdišnji"},
        {"mirsad", u8"maloljetnik", u8"malòljetnik"},
        {"mirsad", u8"čestitke", u8"čèstitke"},
    };
    Engine engine;
    for (const auto& c : cases) {
        INFO(c.voice << " " << c.plain);
        REQUIRE(laprdus_set_voice(engine.handle, c.voice, NO_DATA) == LAPRDUS_OK);
        const auto plain = speak(engine.handle, c.plain);
        REQUIRE(!plain.empty());
        REQUIRE(plain == speak(engine.handle, c.accented));
    }
    // The old readings are gone, and a word that only looks like such a
    // compound keeps its own accent
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    REQUIRE(speak(engine.handle, u8"petogodišnji") != speak(engine.handle, u8"pètogodišnji"));
    REQUIRE(speak(engine.handle, u8"čestitka") != speak(engine.handle, u8"čestítka"));
    REQUIRE(speak(engine.handle, u8"poletan") != speak(engine.handle, u8"pòletan"));   // pȍlētan
}

TEST_CASE("Place names follow the dictionaries in their case forms", "[formant][text]") {
    // The place-name table (formant_proper_names.inc, from tools/formant/places.tsv;
    // HJP, Wiktionary, Vuk's Srpski rječnik), checked 2026-10-09; see "Place
    // names" in docs/formant.md. Croatian omits unstressed length.
    struct Case { const char* voice; const char* plain; const char* accented; };
    const Case cases[] = {
        {"zvonko", u8"Idem u Pulu.", u8"Idem u Púlu."},
        {"zvonko", u8"Pula je lijepa.", u8"Púla je lijepa."},
        {"zvonko", u8"Knin", u8"Knȋn"},
        {"zvonko", u8"iz Knina", u8"iz Knína"},
        {"zvonko", u8"u Kninu", u8"u Knínu"},
        {"zvonko", u8"u Sisku", u8"u Sísku"},
        {"zvonko", u8"Vinkovci", u8"Vȋnkovci"},
        {"zvonko", u8"u Vinkovcima", u8"u Vȋnkovcima"},
        {"zvonko", u8"Slavonija", u8"Slàvonija"},
        {"zvonko", u8"u Dalmaciji", u8"u Dàlmaciji"},
        {"zvonko", u8"putujem u Liku", u8"putujem u Líku"},
        {"zvonko", u8"u Bihaću", u8"u Biháću"},
        {"zvonko", u8"Bjelovar", u8"Bjȅlovar"},
        {"zvonko", u8"Beograd", u8"Bȅograd"},
        {"zvonko", u8"u Beogradu", u8"u Bȅogradu"},
        {"zvonko", u8"u Baru", u8"u Báru"},
        {"zvonko", u8"u Slavonskom Brodu", u8"u Slàvonskom Brȏdu"},
        {"zvonko", u8"iz Solina", u8"iz Solína"},
        {"zvonko", u8"na Lošinju", u8"na Lošínju"},
        {"zvonko", u8"u Rovinju", u8"u Rovínju"},
        {"zvonko", u8"u Visokom", u8"u Vìsokom"},
        {"zvonko", u8"Ilidža", u8"Ilìdža"},
        {"zvonko", u8"u Zvorniku", u8"u Zvorníku"},
        {"zvonko", u8"Idem u Niš.", u8"Idem u Nȋš."},
        {"zvonko", u8"iz Niša", u8"iz Níša"},
        {"stojan", u8"Beograd", u8"Beògrad"},
        {"stojan", u8"u Beogradu", u8"u Beògradu"},
        {"stojan", u8"Valjevo", u8"Vȃljevo"},
        {"stojan", u8"Čačak", u8"Čáčak"},
        {"stojan", u8"Zaječar", u8"Zȁječār"},
        {"stojan", u8"u Kninu", u8"u Knínu"},
        {"mirsad", u8"Ilidža", u8"Ilìdža"},
        {"mirsad", u8"u Zvorniku", u8"u Zvorníku"},
        {"mirsad", u8"u Visokom", u8"u Vìsokom"},
    };
    Engine engine;
    for (const auto& c : cases) {
        INFO(c.voice << " " << c.plain);
        REQUIRE(laprdus_set_voice(engine.handle, c.voice, NO_DATA) == LAPRDUS_OK);
        const auto plain = speak(engine.handle, c.plain);
        REQUIRE(!plain.empty());
        REQUIRE(plain == speak(engine.handle, c.accented));
    }
    // Only a word written with a capital letter is a place; a place spelled
    // like a frequent word is not used for the first word of a sentence
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    REQUIRE(speak(engine.handle, u8"Pula") != speak(engine.handle, u8"pula"));
    REQUIRE(speak(engine.handle, u8"Bar ću doći.") == speak(engine.handle, u8"bar ću doći."));
    REQUIRE(speak(engine.handle, u8"Visoko je.") == speak(engine.handle, u8"visoko je."));
    REQUIRE(speak(engine.handle, u8"Ruda je skupa.") == speak(engine.handle, u8"ruda je skupa."));
    // Text in capitals says nothing about places (ZATVORI PROZOR is not the
    // town Prózor)
    REQUIRE(speak(engine.handle, u8"ZATVORI PROZOR") == speak(engine.handle, u8"zatvori prozor"));
    REQUIRE(speak(engine.handle, u8"u Prozoru") != speak(engine.handle, u8"u prozoru"));
    // The user's stems come before the table
    const char* json = u8"{ \"entries\": [ { \"word\": \"R/ovinj*\" } ] }";
    REQUIRE(laprdus_load_accent_lexicon_from_memory(engine.handle, json, 0) == LAPRDUS_OK);
    REQUIRE(speak(engine.handle, u8"u Rovinju") == speak(engine.handle, u8"u Ròvinju"));
}

TEST_CASE("Personal names follow the dictionaries in their case forms", "[formant][text]") {
    // The table of given names and surnames (formant_proper_names.inc, from
    // tools/formant/persons.tsv; HJP, Wiktionary, Vuk's Srpski rječnik) and
    // the rule for Slavic names in -mir, -dar, -zar, checked 2026-10-09; see
    // "Personal names" in docs/formant.md. Croatian omits unstressed length.
    struct Case { const char* voice; const char* plain; const char* accented; };
    const Case cases[] = {
        {"zvonko", u8"rekao je Ante", u8"rekao je Ánte"},
        {"zvonko", u8"s Antom", u8"s Ántom"},
        {"zvonko", u8"Darko i Zdenko", u8"Dárko i Zdénko"},
        {"zvonko", u8"kod Darka", u8"kod Dárka"},
        {"zvonko", u8"pjesma Ive Andrića", u8"pjesma Ive Ándrića"},
        {"zvonko", u8"s Ivanom Jurićem", u8"s Ivanom Júrićem"},
        {"zvonko", u8"o Anti Starčeviću", u8"o Ánti Stárčeviću"},
        {"zvonko", u8"Miška Kranjca", u8"Miška Kránjca"},
        {"zvonko", u8"o Ignaciju", u8"o Ìgnaciju"},
        {"zvonko", u8"kod Mate", u8"kod Máte"},
        {"zvonko", u8"s Matom", u8"s Mátom"},
        {"zvonko", u8"pozdravi Tamaru", u8"pozdravi Tàmaru"},
        {"zvonko", u8"rekao je Luka", u8"rekao je Lȗka"},
        {"zvonko", u8"s Markom", u8"s Mȃrkom"},
        {"zvonko", u8"Radomir", u8"Rȁdomir"},
        {"zvonko", u8"o Radomiru", u8"o Rȁdomiru"},
        {"zvonko", u8"Vladimir", u8"Vlàdimir"},
        {"zvonko", u8"Božidar", u8"Bòžidar"},
        {"zvonko", u8"Dragan Ranković", u8"Dragan Ránković"},
        {"zvonko", u8"s Idom", u8"s Ídom"},
        {"stojan", u8"Božidar", u8"Bȍžidār"},
        {"stojan", u8"Đorđe", u8"Đȏrđe"},
        {"stojan", u8"Radomir", u8"Rȁdomīr"},
        {"stojan", u8"rekao je Ante", u8"rekao je Ánte"},
    };
    Engine engine;
    for (const auto& c : cases) {
        INFO(c.voice << " " << c.plain);
        REQUIRE(laprdus_set_voice(engine.handle, c.voice, NO_DATA) == LAPRDUS_OK);
        const auto plain = speak(engine.handle, c.plain);
        REQUIRE(!plain.empty());
        REQUIRE(plain == speak(engine.handle, c.accented));
    }
    // Words spelled like a form of a name keep their own reading at the
    // start of a sentence (ide, idi of Ìda; ali; niko) and in lowercase text
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    REQUIRE(speak(engine.handle, u8"Ide kući.") == speak(engine.handle, u8"ide kući."));
    REQUIRE(speak(engine.handle, u8"Ali ja ne znam.") == speak(engine.handle, u8"ali ja ne znam."));
    REQUIRE(speak(engine.handle, u8"Niko nije došao.") == speak(engine.handle, u8"niko nije došao."));
    REQUIRE(speak(engine.handle, u8"Luka je velika.") == speak(engine.handle, u8"luka je velika."));
    REQUIRE(speak(engine.handle, u8"Vera u Boga.") == speak(engine.handle, u8"vera u Boga."));
    // ... and a rare name spelled like a common word is not in the table
    // (Kolega, Era, Lasta), nor the forms of Pètar taken by Pétra
    REQUIRE(speak(engine.handle, u8"Dragi Kolega") == speak(engine.handle, u8"Dragi kolega"));
    REQUIRE(speak(engine.handle, u8"Era interneta") == speak(engine.handle, u8"era interneta"));
    REQUIRE(speak(engine.handle, u8"Svetog Petra") == speak(engine.handle, u8"svetog petra"));
    REQUIRE(speak(engine.handle, u8"u Mali Lošinj") == speak(engine.handle, u8"u mali Lošinj"));
    // The rule for -mir is for names only (gospòdār, uznemíri)
    REQUIRE(speak(engine.handle, u8"Tihomira") != speak(engine.handle, u8"Tihomíra"));
    REQUIRE(speak(engine.handle, u8"gospodar") == speak(engine.handle, u8"gospòdar"));
    REQUIRE(speak(engine.handle, u8"Deprimira me.") == speak(engine.handle, u8"deprimira me."));
}

TEST_CASE("Foreign app names keep their pronunciation and stress in case forms", "[formant][text][dictionary]") {
    // Exercise the shipped replacements as well as the accent lexicon.
    // Run from the repository root, or set LAPRDUS_DICTIONARY to internal.json.
    const char* dictionary = std::getenv("LAPRDUS_DICTIONARY");
    Engine engine;
    REQUIRE(laprdus_load_dictionary(engine.handle,
        dictionary ? dictionary : "data/dictionary/internal.json") == LAPRDUS_OK);
    struct Case { const char* plain; const char* accented; };
    const Case cases[] = {
        {"messenger", u8"Mȅsindžer"},
        {"Messenger", u8"Mȅsindžer"},
        {"MESSENGER", u8"Mȅsindžer"},
        {"Messengera", u8"Mȅsindžera"},
        {"Messengeru", u8"Mȅsindžeru"},
        {"Messengerom", u8"Mȅsindžerom"},
        {u8"poruka na Messengeru", u8"poruka na Mȅsindžeru"},
        {"tiktok", u8"Tȉktok"},
        {"TikTok", u8"Tȉktok"},
        {"TIKTOK", u8"Tȉktok"},
        {"TikToka", u8"Tȉktoka"},
        {"TikToku", u8"Tȉktoku"},
        {"TikTokom", u8"Tȉktokom"},
        {u8"video na TikToku", u8"video na Tȉktoku"},
        {"wintalker", u8"Vintòker"},
        {"WinTalker", u8"Vintòker"},
        {"WINTALKER", u8"Vintòker"},
        {"WinTalkera", u8"Vintòkera"},
        {"WinTalkeru", u8"Vintòkeru"},
        {"WinTalkerom", u8"Vintòkerom"},
        {u8"govori WinTalkerom", u8"govori Vintòkerom"},
    };
    for (const char* voice : FORMANT_VOICES) {
        INFO(voice);
        REQUIRE(laprdus_set_voice(engine.handle, voice, NO_DATA) == LAPRDUS_OK);
        for (const auto& c : cases) {
            INFO(c.plain);
            const auto plain = speak(engine.handle, c.plain);
            REQUIRE(!plain.empty());
            REQUIRE(plain == speak(engine.handle, c.accented));
        }
        // Whole-word suffix patterns must not rewrite longer unrelated words.
        for (const char* text : {"messengerica", "wintalkerica"}) {
            Engine bare;
            REQUIRE(laprdus_set_voice(bare.handle, voice, NO_DATA) == LAPRDUS_OK);
            REQUIRE(speak(engine.handle, text) == speak(bare.handle, text));
        }
    }
}

TEST_CASE("English words are stressed where English stresses them", "[formant][text][english]") {
    struct Case { const char* a; const char* b; };
    // A word with a capital after small letters is read as its parts,
    // unless a lexicon knows the whole word (TikTok) or every part is one
    // syllable (TalkBack); a single small letter before the capital (also
    // the Lj, Nj, Dž of Serbian Cyrillic capitals), a single capital at
    // the end, and capitals followed by small letters keep the word whole.
    const Case same[] = {
        {"ElevenLabs", "Eleven Labs"},
        {"TalkBack", "talkback"},
        {"TalkBacku", "talkbacku"},
        {"ChatGPT", "Chat GPT"},
        {"iPhone", "iphone"},
        {"BiH", "bih"},
        {"PDFom", "pdfom"},
        {"SMSati", "smsati"},
        {"KONZUMklik", "konzumklik"},
        {"McDonald", "Mcdonald"},
        {"TikTok", "tiktok"},
        {"TikToka", "tiktoka"},
        {u8"ЉУБАВЉУ", u8"љубављу"},
        {u8"КЊИГА", u8"књига"},
        {u8"ПРИЈАТЕЉИМА", u8"пријатељима"},
        // A native word made from an English stem keeps its native suffix
        {"chatirati", u8"chatírati"},
        // Case does not matter, and neither does a Croatian case ending on
        // a word spelled the English way
        {"Croatian", "croatian"},
        {"CROATIAN", "croatian"},
    };
    // Native words keep their native accents, also those spelled like an
    // English word with an ending (operate, distribute); the Serbian and
    // Bosnian voices keep the length after the accent.
    const Case native[] = {
        {"telefon", u8"telèfon"},
        {"telefona", u8"telefóna"},
    };
    const Case native_hr[] = {
        {"operater", u8"operàter"},
        {"distributer", u8"distribùter"},
    };
    const Case native_sr[] = {
        {"operater", u8"operàtēr"},
        {"distributer", u8"distribùtēr"},
    };
    for (const char* voice : FORMANT_VOICES) {
        INFO(voice);
        Engine engine;
        REQUIRE(laprdus_set_voice(engine.handle, voice, NO_DATA) == LAPRDUS_OK);
        for (const auto& c : same) {
            INFO(c.a);
            const auto a = speak(engine.handle, c.a);
            REQUIRE(!a.empty());
            REQUIRE(a == speak(engine.handle, c.b));
        }
        for (const auto& c : native) {
            INFO(c.a);
            REQUIRE(speak(engine.handle, c.a) == speak(engine.handle, c.b));
        }
        for (const auto& c : std::strcmp(voice, "zvonko") == 0 ? native_hr : native_sr) {
            INFO(c.a);
            REQUIRE(speak(engine.handle, c.a) == speak(engine.handle, c.b));
        }

        // The stress is where English has it: Croàtian peaks long before
        // the native rules' Croatìan, Ábleton on its first syllable, not on
        // the loan rule's Ablèton, Elèven not on the first. It is a plain
        // stress, not the rising accent a native word would have there.
        const auto croatian_audio = speak(engine.handle, "Croatian");
        const double croatian = pitch_peak_ms(croatian_audio);
        REQUIRE(croatian > 0.0);
        REQUIRE(croatian + 80.0 < pitch_peak_ms(speak(engine.handle, u8"Croatìan")));
        REQUIRE(croatian_audio != speak(engine.handle, u8"Croàtian"));
        REQUIRE(pitch_peak_ms(speak(engine.handle, "Ableton")) + 80.0 <
                pitch_peak_ms(speak(engine.handle, u8"Ablèton")));
        REQUIRE(pitch_peak_ms(speak(engine.handle, "Eleven")) >
                pitch_peak_ms(speak(engine.handle, u8"Èleven")) + 50.0);
        // The English first syllable is the plain stress of an unknown word
        // stressed there by the user's accent lexicon.
        Engine marked;
        REQUIRE(laprdus_set_voice(marked.handle, voice, NO_DATA) == LAPRDUS_OK);
        REQUIRE(laprdus_load_accent_lexicon_from_memory(marked.handle,
            "{ \"entries\": [ { \"word\": \"'ableton\" } ] }", 0) == LAPRDUS_OK);
        REQUIRE(speak(engine.handle, "Ableton") == speak(marked.handle, "Ableton"));
    }
}

TEST_CASE("Word-class rules place the accent of loans, derived nouns and prefixed verbs", "[formant][text]") {
    // Words outside the lexicon: the rules for nouns with a long last stem
    // syllable, prefixed verbs, verbs in -ovati, nouns in -ica and -anin and
    // adjectives in -izan/-ozan (StressRules::strong).
    struct Case { const char* plain; const char* accented; };
    const Case cases[] = {
        // nouns with a long last stem syllable: case forms
        {"zemljaka", "zemlj\xC3\xA1ka"},   // zemljáka
        {"\xC4\x8D" "arobnjaka", "\xC4\x8D" "arobnj\xC3\xA1ka"},   // čarobnjáka
        {"\xC4\x8D" "arobnjacima", "\xC4\x8D" "arobnj\xC3\xA1" "cima"},   // čarobnjácima
        {"programera", "program\xC3\xA9ra"},   // programéra
        {"in\xC5\xBE" "enjera", "in\xC5\xBE" "enj\xC3\xA9ra"},   // inženjéra
        {"rezultata", "rezult\xC3\xA1ta"},   // rezultáta
        {"rezultatima", "rezult\xC3\xA1tima"},   // rezultátima
        {"kapetana", "kapet\xC3\xA1na"},   // kapetána
        {"balkona", "balk\xC3\xB3na"},   // balkóna
        {"sezone", "sez\xC3\xB3ne"},   // sezóne
        {"broj ra\xC4\x8Duna", "broj ra\xC4\x8D\xC3\xBAna"},   // broj račúna
        {"ra\xC4\x8Duna", "ra\xC4\x8D\xC3\xBAna"},   // račúna (the verb carries the noun-twin flag)
        {"\xC4\x8Duvara", "\xC4\x8Duv\xC3\xA1ra"},   // čuvára
        {"novinari", "novin\xC3\xA1ri"},   // novinári
        {"prodava\xC4\x8D" "a", "prodav\xC3\xA1\xC4\x8D" "a"},   // prodaváča
        {"igra\xC4\x8D" "a", "igr\xC3\xA1\xC4\x8D" "a"},   // igráča
        {"ko\xC5\xA1" "arka\xC5\xA1" "a", "ko\xC5\xA1" "ark\xC3\xA1\xC5\xA1" "a"},   // košarkáša
        {"\xC4\x8Dokolade", "\xC4\x8Dokol\xC3\xA1" "de"},   // čokoláde
        {"piramida", "piram\xC3\xAD" "da"},   // piramída
        {"analiza", "anal\xC3\xADza"},   // analíza
        {"kanalom", "kan\xC3\xA1lom"},   // kanálom
        {"materijalu", "materij\xC3\xA1lu"},   // materijálu
        {"motiva", "mot\xC3\xADva"},   // motíva
        {"Francuza", "Franc\xC3\xBAza"},   // Francúza
        {"zaposlenika", "zaposlen\xC3\xADka"},   // zaposleníka
        {"povjerenici", "povjeren\xC3\xAD" "ci"},   // povjereníci
        {"zarobljenika", "zarobljen\xC3\xADka"},   // zarobljeníka
        {"dobitka", "dob\xC3\xADtka"},   // dobítka
        {"Amerikanca", "Amerik\xC3\xA1nca"},   // Amerikánca
        {"Amerikanac", "Amerik\xC3\xA1nac"},   // Amerikánac
        {"\xC4\x8Duvarov", "\xC4\x8Duv\xC3\xA1rov"},   // čuvárov
        // the nominative: one syllable back, Zvonko without the length
        {"\xC4\x8D" "arobnjak", "\xC4\x8D" "ar\xC3\xB2" "bnjak"},   // čaròbnjak
        {"in\xC5\xBE" "enjer", "in\xC5\xBE\xC3\xA8njer"},   // inžènjer
        {"rezultat", "rez\xC3\xB9ltat"},   // rezùltat
        {"prodava\xC4\x8D", "prod\xC3\xA0va\xC4\x8D"},   // prodàvač
        {"vitamin", "vit\xC3\xA0min"},   // vitàmin
        {"festival", "fest\xC3\xACval"},   // festìval
        {"zarobljenik", "zar\xC3\xB2" "bljenik"},   // zaròbljenik
        // prefixed verbs follow the infinitive
        {"pogledala", "pogl\xC3\xA8" "dala"},   // poglèdala
        {"pogledamo", "pogl\xC3\xA8" "damo"},   // poglèdamo
        {"napravio", "napr\xC3\xA0vio"},   // napràvio
        {"do\xC4\x8D" "ekali", "do\xC4\x8D\xC3\xA8kali"},   // dočèkali
        {"iskusio", "isk\xC3\xB9sio"},   // iskùsio
        {"zahvalismo", "zahv\xC3\xA1lismo"},   // zahválismo (VERBS: zahváliti)
        {"zatra\xC5\xBEismo", "zatr\xC3\xA1\xC5\xBEismo"},   // zatrážismo
        {"pohvalismo", "pohv\xC3\xA1lismo"},   // pohválismo
        {"ispitasmo", "isp\xC3\xACtasmo"},   // ispìtasmo
        {"poku\xC5\xA1" "av\xC5\xA1i", "pok\xC3\xB9\xC5\xA1" "av\xC5\xA1i"},   // pokùšavši
        {"po\xC5\xBE" "eljeti", "po\xC5\xBE\xC3\xA8ljeti"},   // požèljeti
        // verbs in -ovati
        {"putovao", "put\xC3\xB2vao"},   // putòvao
        {"kupovala", "kup\xC3\xB2vala"},   // kupòvala
        {"vjerovao", "vj\xC8\x85rovao"},   // vjȅrovao
        // nouns in -ica, inhabitants, adjectives
        {"\xC4\x8Duvarica", "\xC4\x8Duv\xC3\xA0rica"},   // čuvàrica
        {"voditeljica", "vodit\xC3\xA8ljica"},   // voditèljica
        {"dr\xC5\xBE" "avljanin", "dr\xC5\xBE\xC3\xA0vljanin"},   // držàvljanin
        {"dr\xC5\xBE" "avljanina", "dr\xC5\xBE\xC3\xA0vljanina"},   // držàvljanina
        {"precizan", "prec\xC3\xACzan"},   // precìzan
        {"nervozna", "nerv\xC3\xB3zna"},   // nervózna
        // the -ina rule and its exceptions
        {"balerina", "baler\xC3\xACna"},   // balerìna
        {"ku\xC4\x87" "etina", "ku\xC4\x87\xC3\xA8tina"},   // kućètina
        {"mje\xC5\xA1" "avina", "mje\xC5\xA1\xC3\xA0vina"},   // mješàvina
        {"planina", "plan\xC3\xACna"},   // planìna
        {"tre\xC4\x87ina", "tre\xC4\x87\xC3\xACna"},   // trećìna
        // the words the rules would otherwise take: lexicon
        {"zakona", "z\xC3\xA1kona"},   // zákona
        {"spomenika", "sp\xC8\x8Dmenika"},   // spȍmenika
        {"vojnika", "vojn\xC3\xADka"},   // vojníka
        {"milijuna", "milij\xC3\xBAna"},   // milijúna
    };
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    for (const auto& c : cases) {
        std::vector<int16_t> plain = speak(engine.handle, c.plain);
        std::vector<int16_t> accented = speak(engine.handle, c.accented);
        INFO(c.plain);
        REQUIRE(!plain.empty());
        REQUIRE(plain == accented);
    }

    // Native words of the same shape keep the first syllable: the rules
    // must not move their accent.
    const Case not_these[] = {
        {"jezera", "jez\xC3\xA9ra"},   // jezéra
        {"ve\xC4\x8D" "era", "ve\xC4\x8D\xC3\xA9ra"},   // večéra
        {"servera", "serv\xC3\xA9ra"},   // servéra
        {"predmeta", "predm\xC3\xA9ta"},   // predméta
        {"poslana", "posl\xC3\xA1na"},   // poslána
        {"poznata", "pozn\xC3\xA1ta"},   // poznáta
        {"imate", "im\xC3\xA1te"},   // imáte
        {"gledati", "gled\xC3\xA1ti"},   // gledáti
        {"gledala", "gled\xC3\xA1la"},   // gledála
        {"radnika", "radn\xC3\xADka"},   // radníka
        {"korisnika", "korisn\xC3\xADka"},   // korisníka
        {"osjetljiva", "osjetlj\xC3\xADva"},   // osjetljíva
        {"naziva", "naz\xC3\xADva"},   // nazíva
        {"Ivanin", "Iv\xC3\xA0nin"},   // Ivànin
        {"mamina", "mam\xC3\xACna"},   // mamìna
        {"lozinke", "loz\xC3\xADnke"},   // lozínke
        {"zadruzi", "zadr\xC3\xBAzi"},   // zadrúzi
        {"poseban", "pos\xC3\xA8" "ban"},   // posèban
        {"dosadan", "dos\xC3\xA0" "dan"},   // dosàdan
        {"nagrada", "nagr\xC3\xA1" "da"},   // nagráda
        {"svijeta", "svij\xC3\xA9ta"},   // svijéta
        {"doga\xC4\x91" "aju", "dog\xC3\xA0\xC4\x91" "aju"},   // dogàđaju
        {"u\xC4\x8D" "enik", "u\xC4\x8D\xC3\xA8nik"},   // učènik (ȕčenīk)
        {"u\xC4\x8D" "enika", "u\xC4\x8D" "en\xC3\xADka"},   // učeníka (ȕčenīka)
        {"poznanici", "poznan\xC3\xAD" "ci"},   // poznaníci (pòznanīci)
        {"udara", "ud\xC3\xA1ra"},   // udára (ùdara, a verb in VERBS)
        {"ispune", "isp\xC3\xBAne"},   // ispúne (ìspune)
        {"umire", "um\xC3\xADre"},   // umíre (ùmire)
    };
    for (const auto& c : not_these) {
        INFO(c.plain);
        REQUIRE(speak(engine.handle, c.plain) != speak(engine.handle, c.accented));
    }

    // Stojan keeps the length the dictionaries write after the retracted
    // accent of the nominative.
    const Case serbian[] = {
        {"\xC4\x8D" "arobnjak", "\xC4\x8D" "ar\xC3\xB2" "bnj\xC4\x81k"},   // čaròbnjāk
        {"prodava\xC4\x8D", "prod\xC3\xA0v\xC4\x81\xC4\x8D"},   // prodàvāč
        {"zarobljenik", "zar\xC3\xB2" "bljen\xC4\xABk"},   // zaròbljenīk
        {"prodava\xC4\x8D" "a", "prodav\xC3\xA1\xC4\x8D" "a"},   // prodaváča
    };
    REQUIRE(laprdus_set_voice(engine.handle, "stojan", NO_DATA) == LAPRDUS_OK);
    for (const auto& c : serbian) {
        INFO(c.plain);
        REQUIRE(speak(engine.handle, c.plain) == speak(engine.handle, c.accented));
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

// Pitch of the loudest window in each quarter of the audio, to see how much
// the voice moves along a clause.
double pitch_spread_hz(const std::vector<int16_t>& audio) {
    double lo = 1e9, hi = 0.0;
    for (int q = 0; q < 4; ++q) {
        double hz = pitch_hz(audio, q * 0.25, (q + 1) * 0.25);
        if (hz <= 0.0) continue;
        lo = std::min(lo, hz);
        hi = std::max(hi, hz);
    }
    return hi > lo ? hi - lo : 0.0;
}

TEST_CASE("Inflection level scales the pitch movements", "[formant][params]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    REQUIRE(laprdus_get_inflection_level(engine.handle) == Catch::Approx(0.5f));

    const char* text = "Marija danas putuje u Zagreb?";
    std::vector<int16_t> normal = speak(engine.handle, text);

    // The default level is the measured contour: setting it explicitly
    // changes nothing.
    REQUIRE(laprdus_set_inflection_level(engine.handle, 0.5f) == LAPRDUS_OK);
    REQUIRE(speak(engine.handle, text) == normal);

    REQUIRE(laprdus_set_inflection_level(engine.handle, 0.0f) == LAPRDUS_OK);
    std::vector<int16_t> flat = speak(engine.handle, text);
    REQUIRE(flat.size() == normal.size());
    REQUIRE(pitch_spread_hz(flat) < 4.0);

    REQUIRE(laprdus_set_inflection_level(engine.handle, 1.0f) == LAPRDUS_OK);
    std::vector<int16_t> wide = speak(engine.handle, text);
    REQUIRE(wide.size() == normal.size());
    REQUIRE(pitch_spread_hz(wide) > pitch_spread_hz(normal) * 1.3);
    REQUIRE(pitch_spread_hz(normal) > pitch_spread_hz(flat) + 10.0);

    // Out-of-range values are clamped, not rejected.
    REQUIRE(laprdus_set_inflection_level(engine.handle, 7.0f) == LAPRDUS_OK);
    REQUIRE(laprdus_get_inflection_level(engine.handle) == Catch::Approx(1.0f));
}

TEST_CASE("Acceleration multiplies the speech rate", "[formant][params]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "stojan", NO_DATA) == LAPRDUS_OK);
    REQUIRE(laprdus_get_acceleration(engine.handle) == Catch::Approx(1.0f));

    const char* text = "Mama ima malu nanu.";
    std::vector<int16_t> normal = speak(engine.handle, text);

    REQUIRE(laprdus_set_speed(engine.handle, 2.0f) == LAPRDUS_OK);
    std::vector<int16_t> fast = speak(engine.handle, text);

    // Speed 1.0 at acceleration 2.0 is speed 2.0: the same samples.
    REQUIRE(laprdus_set_speed(engine.handle, 1.0f) == LAPRDUS_OK);
    REQUIRE(laprdus_set_acceleration(engine.handle, 2.0f) == LAPRDUS_OK);
    REQUIRE(speak(engine.handle, text) == fast);

    // The host's top rate goes past the plain 4.0x with acceleration.
    REQUIRE(laprdus_set_speed(engine.handle, 4.0f) == LAPRDUS_OK);
    std::vector<int16_t> fastest = speak(engine.handle, text);
    REQUIRE(laprdus_set_acceleration(engine.handle, 1.0f) == LAPRDUS_OK);
    std::vector<int16_t> plain = speak(engine.handle, text);
    REQUIRE(fastest.size() < plain.size() * 0.8);

    // And below the normal rate with a low acceleration.
    REQUIRE(laprdus_set_speed(engine.handle, 1.0f) == LAPRDUS_OK);
    REQUIRE(laprdus_set_acceleration(engine.handle, 0.5f) == LAPRDUS_OK);
    std::vector<int16_t> slow = speak(engine.handle, text);
    REQUIRE(slow.size() > normal.size() * 1.6);

    REQUIRE(laprdus_set_acceleration(engine.handle, 100.0f) == LAPRDUS_OK);
    REQUIRE(laprdus_get_acceleration(engine.handle) == Catch::Approx(3.0f));
}

TEST_CASE("Formant voices accept the wider rate and pitch ranges", "[formant][params]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    REQUIRE(laprdus_set_inflection_enabled(engine.handle, 0) == LAPRDUS_OK);

    const char* text = "mama ima malu nanu";
    REQUIRE(laprdus_set_speed(engine.handle, 0.5f) == LAPRDUS_OK);
    std::vector<int16_t> slow = speak(engine.handle, text);
    REQUIRE(laprdus_set_speed(engine.handle, 0.25f) == LAPRDUS_OK);
    std::vector<int16_t> slower = speak(engine.handle, text);
    REQUIRE(slower.size() > slow.size() * 1.7);

    REQUIRE(laprdus_set_speed(engine.handle, 1.0f) == LAPRDUS_OK);
    std::vector<int16_t> normal = speak(engine.handle, "aaa");
    double base = pitch_hz(normal);
    REQUIRE(laprdus_set_user_pitch(engine.handle, 3.0f) == LAPRDUS_OK);
    REQUIRE(close_to(pitch_hz(speak(engine.handle, "aaa")), base * 3.0, 0.06));
    // Below 0.5 the voice reaches its 45 Hz floor, which the pitch detector
    // of these tests cannot follow; the audio must still come out whole.
    REQUIRE(laprdus_set_user_pitch(engine.handle, 0.25f) == LAPRDUS_OK);
    std::vector<int16_t> lowest = speak(engine.handle, "aaa");
    REQUIRE(lowest.size() == normal.size());
    REQUIRE(rms(lowest) > rms(normal) * 0.3);
}

TEST_CASE("Nominal words per minute are known for every voice", "[formant][params]") {
    REQUIRE(laprdus_get_nominal_wpm("zvonko") == Catch::Approx(175.0f));
    REQUIRE(laprdus_get_nominal_wpm(nullptr) == Catch::Approx(175.0f));
    REQUIRE(laprdus_get_nominal_wpm("stojan") > laprdus_get_nominal_wpm("mirsad"));
    REQUIRE(laprdus_get_nominal_wpm("josip") > 0.0f);
    REQUIRE(laprdus_get_nominal_wpm("djed") > 0.0f);
    REQUIRE(laprdus_get_nominal_wpm("nobody") == 0.0f);
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

TEST_CASE("A comma ends between a statement and a question", "[formant][prosody]") {
    // A comma gets a small fall after the last accent and a slight rise on
    // the last syllable (docs/formant.md, "The melody of read Croatian"):
    // it ends well above the same clause as a statement, but clearly below
    // it as a question, which a rise of four semitones from the level did
    // not.
    for (const char* voice : FORMANT_VOICES) {
        Engine engine;
        REQUIRE(laprdus_set_voice(engine.handle, voice, NO_DATA) == LAPRDUS_OK);
        REQUIRE(laprdus_set_sentence_pause(engine.handle, 0) == LAPRDUS_OK);
        REQUIRE(laprdus_set_comma_pause(engine.handle, 0) == LAPRDUS_OK);
        for (float speed : {1.0f, 2.0f}) {
            REQUIRE(laprdus_set_speed(engine.handle, speed) == LAPRDUS_OK);
            for (const char* text : {"Ona ima novu lampu", "Vlada je danas usvojila prijedlog novog zakona"}) {
                const double statement = final_pitch_hz(speak(engine.handle, (std::string(text) + ".").c_str()));
                const double comma = final_pitch_hz(speak(engine.handle, (std::string(text) + ",").c_str()));
                const double question = final_pitch_hz(speak(engine.handle, (std::string(text) + "?").c_str()));
                INFO(voice << " " << speed << " " << text);
                REQUIRE(statement > 50.0);
                REQUIRE(comma > statement * 1.19);      // three semitones
                REQUIRE(comma * 1.12 < question);       // two semitones
            }
        }
    }
}

TEST_CASE("A mark glued to a word is read by name, not as a clause end", "[formant][text]") {
    // eSpeak's rule: a period, comma, colon or other mark ends the clause
    // only when whitespace, a bracket or quote, or the end of the text
    // follows. Inside a word or a number it is spoken (točka / tačka,
    // dvotočka, zarez); a comma or question mark inside a word is silent.
    // Synthesis is deterministic, so the audio must equal the spelled-out
    // text sample for sample.
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    auto same = [&](const char* a, const char* b) { return speak(engine.handle, a) == speak(engine.handle, b); };

    SECTION("Period inside a word") {
        REQUIRE(same("datoteka.txt", "datoteka to\xC4\x8Dka txt"));
        REQUIRE(same("www.index.hr", "www to\xC4\x8Dka index to\xC4\x8Dka hr"));
        REQUIRE(same("pjesma.mp3", "pjesma to\xC4\x8Dka mp3"));
        REQUIRE(same("README.TXT", "README to\xC4\x8Dka TXT"));
        // Anywhere else a glued period ends the clause (a screen reader
        // glues the items of a label with it)
        REQUIRE(same("Preslu\xC5\xA1" "ano.Nestaju\xC4\x87" "a poruka", "Preslu\xC5\xA1" "ano. Nestaju\xC4\x87" "a poruka"));
        REQUIRE_FALSE(same("Preslu\xC5\xA1" "ano.Nestaju\xC4\x87" "a", "Preslu\xC5\xA1" "ano to\xC4\x8Dka Nestaju\xC4\x87" "a"));
        REQUIRE(same("kraj.Novi", "kraj. Novi"));
        REQUIRE(same("1.Prvi", "jedan. Prvi"));
        REQUIRE(same("datoteka.tekstualna", "datoteka. tekstualna"));
        REQUIRE(same(".txt", "to\xC4\x8Dka txt"));
        REQUIRE_FALSE(same("datoteka.txt", "datoteka txt"));
    }
    SECTION("Decimal point and comma") {
        // After a period every group of digits is a whole number, as eSpeak
        // reads it; after the decimal comma up to two digits are a number
        // and more are read one by one.
        REQUIRE(same("3.14", "tri to\xC4\x8Dka \xC4\x8D" "etrnaest"));
        // A number glued to letters is set off from them, and the period
        // that joins a word to the letters before it is silent (a screen
        // reader's label "7.listopada.u20:03")
        REQUIRE(same("u20:03", "u dvadeset nula tri"));
        REQUIRE(same("7.listopada.u20:03", "sedam listopada u dvadeset nula tri"));
        REQUIRE(same("od20do30", "od dvadeset do trideset"));
        REQUIRE(same("5kg", "pet kg"));
        REQUIRE(same("COVID19", "COVID devetnaest"));
        REQUIRE(same("20:03h", "dvadeset nula tri h"));
        REQUIRE(same("3,14", "tri zarez \xC4\x8D" "etrnaest"));
        REQUIRE(same("0.05", "nula to\xC4\x8Dka nula pet"));
        REQUIRE(same("1.317", "jedan to\xC4\x8Dka tristo sedamnaest"));
        REQUIRE(same("3.14159", "tri to\xC4\x8Dka \xC4\x8D" "etrnaest tisu\xC4\x87" "a sto pedeset devet"));
        REQUIRE(same("3,14159", "tri zarez jedan \xC4\x8D" "etiri jedan pet devet"));
        REQUIRE(same("1.000", "jedan to\xC4\x8Dka nula nula nula"));
    }
    SECTION("The commas of a phone number spelled digit by digit are pauses") {
        // Screen readers write a phone number one digit at a time, with a
        // comma glued between the groups; a decimal comma stays one
        REQUIRE(same("+ 1 2 3,5 6,7 8 9,8 7 6", "+ 1 2 3, 5 6, 7 8 9, 8 7 6"));
        REQUIRE(same("0 9 1,2 3 4,5 6 7 8", "nula devet jedan, dva tri \xC4\x8D" "etiri, pet \xC5\xA1" "est sedam osam"));
        REQUIRE_FALSE(same("+ 1 2 3,5 6,7 8 9,8 7 6", "plus jedan dva tri zarez pet \xC5\xA1" "est zarez sedam osam devet zarez osam sedam \xC5\xA1" "est"));
        REQUIRE(same("4,5 3,5 5,0", "\xC4\x8D" "etiri zarez pet tri zarez pet pet zarez nula"));
        REQUIRE(same("Ocjena 4,5.", "Ocjena \xC4\x8D" "etiri zarez pet."));
    }
    SECTION("A period after a single letter is an abbreviation dot: silent, the letters by name") {
        // (Letter names are full words, so "a" and "u" spelled from an
        // abbreviation differ from the typed conjunction and preposition;
        // the comparisons use letters whose names are not clitics.)
        REQUIRE(same("p.s.", "pe es."));
        REQUIRE(same("s.r.s.", "es er es."));
        REQUIRE(same("b.b.", "be be."));
        REQUIRE_FALSE(same("s.a.r.s.", "s to\xC4\x8Dka a to\xC4\x8Dka er to\xC4\x8Dka es."));
        REQUIRE_FALSE(same("U.S.A.", "u to\xC4\x8Dka es to\xC4\x8Dka a."));
    }
    SECTION("Abbreviations are read as written, not expanded") {
        // A word whose only syllable would be a final r is spelled
        REQUIRE(same("dr. Ivi\xC4\x87", "de er. Ivi\xC4\x87"));
        REQUIRE(same("npr. ovo", "en pe er. ovo"));
        REQUIRE(same("mr. Ana", "em er. Ana"));
        REQUIRE(same("5 mm", "pet em em"));
        REQUIRE_FALSE(same("dr. Ivi\xC4\x87", "doktor. Ivi\xC4\x87"));
        REQUIRE_FALSE(same("5 km", "pet kilometara"));
        // A syllabic r inside or at the head of a word is still a syllable
        REQUIRE_FALSE(same("rt", "er te"));
        REQUIRE_FALSE(same("krv", "ka er ve"));
    }
    SECTION("The prepositions s and k are words wherever a capital says nothing") {
        // At the head of a sentence and in all-caps text a capital S or K
        // is the preposition, as a lowercase one is anywhere
        REQUIRE(same("S tobom sam htio sve, ali sam s njom dobio puno vi\xC5\xA1" "e.",
                     "s tobom sam htio sve, ali sam s njom dobio puno vi\xC5\xA1" "e."));
        REQUIRE(same("K meni.", "k meni."));
        REQUIRE(same("(S tobom)", "(s tobom)"));
        REQUIRE(same("\xD0\xA1 \xD1\x82\xD0\xBE\xD0\xB1\xD0\xBE\xD0\xBC.", "s tobom."));   // С тобом
        REQUIRE(same("ALI SAM S NJOM.", "ali sam s njom."));
        REQUIRE(same("SPOJITE S USB KABELOM.", "spojite s USB kabelom."));
        REQUIRE_FALSE(same("S tobom.", "es tobom."));
        // Elsewhere a capital, a letter alone, before a hyphen, a single
        // letter or a word no preposition takes is the letter's name
        REQUIRE(same("Mercedes S klasa.", "Mercedes es klasa."));
        REQUIRE(same("Pritisnite S za spremanje.", "Pritisnite es za spremanje."));
        REQUIRE(same("S je slovo.", "es je slovo."));
        REQUIRE(same("slovo s je", "slovo es je"));
        REQUIRE(same("S-klasa", "es klasa"));
        REQUIRE(same("S i M.", "es i em."));
        REQUIRE(same("S", "es"));
        REQUIRE(same("K", "ka"));
        // A line starts a sentence of its own: the lines of a post as
        // VoiceOver reads it ("Javno" / "S ponosom predstavljam")
        REQUIRE(same("prije 18 h, Javno\nS ponosom predstavljam.",
                     "prije 18 h, Javno\ns ponosom predstavljam."));
        REQUIRE(same("Javno\r\n\r\nK meni.", "Javno\r\n\r\nk meni."));
        // Before an instrumental a capital S is the preposition anywhere,
        // for a host that joins the lines with a space
        REQUIRE(same("Javno S ponosom predstavljam.", "Javno s ponosom predstavljam."));
        REQUIRE(same("Razgovarao sam S Ivanom.", "Razgovarao sam s Ivanom."));
        REQUIRE(same("Do\xC5\xA1la je S rado\xC5\xA1\xC4\x87u.", "Do\xC5\xA1la je s rado\xC5\xA1\xC4\x87u."));
        REQUIRE(same("Vozim Mercedes S klasu.", "Vozim Mercedes es klasu."));
        REQUIRE(same("Mercedes S-klasom.", "Mercedes es klasom."));
    }
    SECTION("A line break ends the clause with the newline pause") {
        REQUIRE(laprdus_set_newline_pause(engine.handle, 0) == LAPRDUS_OK);
        std::vector<int16_t> short_pause = speak(engine.handle, "Prvi red\nDrugi red");
        REQUIRE(laprdus_set_newline_pause(engine.handle, 500) == LAPRDUS_OK);
        std::vector<int16_t> long_pause = speak(engine.handle, "Prvi red\nDrugi red");
        const double added_ms = (static_cast<double>(long_pause.size()) -
                                 static_cast<double>(short_pause.size())) * 1000.0 / 22050.0;
        REQUIRE(added_ms > 490.0);
        REQUIRE(added_ms < 510.0);
        // A blank line or CR LF is one break; after a full stop it adds nothing
        REQUIRE(same("Prvi red\r\n\r\nDrugi red", "Prvi red\nDrugi red"));
        REQUIRE(same("Prvi red.\nDrugi red.", "Prvi red. Drugi red."));
        REQUIRE_FALSE(same("Prvi red\nDrugi red", "Prvi red Drugi red"));
    }
    SECTION("A number with a period before a lowercase word is read as a cardinal") {
        // An ordinal would need the case of the noun, so the number is read
        // as written; the period is silent and does not end the clause.
        // Before an uppercase word it ends the sentence as before.
        REQUIRE(same("7. listopada 2026.", "sedam listopada dvije tisu\xC4\x87" "e dvadeset \xC5\xA1" "est."));
        REQUIRE(same("u 19. stolje\xC4\x87u", "u devetnaest stolje\xC4\x87u"));
        REQUIRE(same("1990. godine", "tisu\xC4\x87u devetsto devedeset godine"));
        REQUIRE(same("2000. godine", "dvije tisu\xC4\x87" "e godine"));
        REQUIRE(same("100. put", "sto put"));
        REQUIRE(same("21. put", "dvadeset jedan put"));
        REQUIRE(same("7.listopada", "sedam listopada"));
        REQUIRE(same("7. Listopad.", "sedam. Listopad."));
        REQUIRE(same("Ima ih 7. Sutra", "Ima ih sedam. Sutra"));
        REQUIRE(same("0. element", "nula. element"));
        REQUIRE(same("12345. dan", "dvanaest tisu\xC4\x87" "a tristo \xC4\x8D" "etrdeset pet. dan"));
    }
    SECTION("Dotted identifiers are read group by group") {
        REQUIRE(same("192.168.1.1", "sto devedeset dva to\xC4\x8Dka sto \xC5\xA1" "ezdeset osam to\xC4\x8Dka jedan to\xC4\x8Dka jedan"));
        REQUIRE(same("2.0.1", "dva to\xC4\x8Dka nula to\xC4\x8Dka jedan"));
    }
    SECTION("Digit mode keeps the separator") {
        REQUIRE(laprdus_set_number_mode(engine.handle, LAPRDUS_NUMBER_MODE_DIGIT) == LAPRDUS_OK);
        REQUIRE(same("3.14", "tri to\xC4\x8Dka jedan \xC4\x8D" "etiri"));
        REQUIRE(same("3,14", "tri zarez jedan \xC4\x8D" "etiri"));
        REQUIRE(same("12:30", "dvanaest trideset"));
        REQUIRE(same("0 9 1,2 3 4,5 6 7 8", "0 9 1, 2 3 4, 5 6 7 8"));
        REQUIRE(same("7.10.2026", "sedam deset dvije tisu\xC4\x87" "e dvadeset \xC5\xA1" "est"));
        // The sections share one engine in this harness
        REQUIRE(laprdus_set_number_mode(engine.handle, LAPRDUS_NUMBER_MODE_WHOLE) == LAPRDUS_OK);
    }
    SECTION("Clock times and dates are read without the separators") {
        REQUIRE(same("12:30", "dvanaest trideset"));
        REQUIRE(same("9:05", "devet nula pet"));
        REQUIRE(same("12:30:45", "dvanaest trideset \xC4\x8D" "etrdeset pet"));
        REQUIRE(same("u 12:30 sati", "u dvanaest trideset sati"));
        REQUIRE(same("7.10.2026.", "sedam deset dvije tisu\xC4\x87" "e dvadeset \xC5\xA1" "est."));
        REQUIRE(same("7. 10. 2026.", "sedam deset dvije tisu\xC4\x87" "e dvadeset \xC5\xA1" "est."));
        REQUIRE(same("07.10.26", "sedam deset dvadeset \xC5\xA1" "est"));
        REQUIRE(same("31.12.1999", "trideset jedan dvanaest tisu\xC4\x87u devetsto devedeset devet"));
        // Not a time or a date: the marks are read
        REQUIRE(same("3:1", "tri dvoto\xC4\x8Dka jedan"));
        REQUIRE(same("25:00", "dvadeset pet dvoto\xC4\x8Dka nula nula"));
        REQUIRE(same("7.13.2026", "sedam to\xC4\x8Dka trinaest to\xC4\x8Dka dvije tisu\xC4\x87" "e dvadeset \xC5\xA1" "est"));
    }
    SECTION("Colon and exclamation mark inside a word, silent comma and question mark") {
        REQUIRE(same("a:b", "a dvoto\xC4\x8Dka be"));
        // The colon of a web address is silent, like its slashes
        REQUIRE(same("https://www.index.hr", "https www.index.hr"));
        REQUIRE(same("http://example.com", "http example.com"));
        REQUIRE_FALSE(same("https://www.index.hr", "https dvoto\xC4\x8Dka www.index.hr"));
        REQUIRE(same("Hej!ti", "Hej uskli\xC4\x8Dnik ti"));
        REQUIRE(same("a,b", "a b"));
        REQUIRE(same("a?b", "a b"));
    }
    SECTION("A mark before a space, bracket, quote or the end still ends the clause") {
        REQUIRE_FALSE(same("Kraj. Novi", "Kraj Novi"));
        REQUIRE(same("Stvarno?!", "Stvarno?"));
        REQUIRE(same("\xC4\x8C" "ekaj...", "\xC4\x8C" "ekaj\xE2\x80\xA6"));
        REQUIRE(same("(Da.) Ne", "(Da. Ne"));
        REQUIRE(same("\"Da.\" Ne", "Da. Ne"));
    }
    SECTION("Serbian and Bosnian voices say tačka") {
        REQUIRE(laprdus_set_voice(engine.handle, "stojan", NO_DATA) == LAPRDUS_OK);
        REQUIRE(same("datoteka.txt", "datoteka ta\xC4\x8Dka txt"));
        REQUIRE(same("3.14", "tri ta\xC4\x8Dka \xC4\x8D" "etrnaest"));
        REQUIRE(same("a:b", "a dvota\xC4\x8Dka be"));
        REQUIRE(same("7.10.2026", "sedam deset dve hiljade dvadeset \xC5\xA1" "est"));
        REQUIRE(same("2000. godine", "dve hiljade godine"));
        REQUIRE(same("p.s.", "pe es."));
        REQUIRE(laprdus_set_voice(engine.handle, "mirsad", NO_DATA) == LAPRDUS_OK);
        REQUIRE(same("Hej!ti", "Hej uzvi\xC4\x8Dnik ti"));
    }
}

TEST_CASE("Short questions and exclamations are audible", "[formant][prosody]") {
    // In a clause of a few words the end is all there is to hear the
    // punctuation by. Every question ends with a rise, also after a question
    // word ("Kako si ti?") and inside a single syllable ("Ti?").
    struct Case { const char* statement; const char* question; };
    const Case cases[] = {
        {"Kako si ti.", "Kako si ti?"},
        {"A ti.", "A ti?"},
        {"Ti.", "Ti?"},
        {"Za\xC5\xA1to.", "Za\xC5\xA1to?"},                       // Zašto
        {"Tko je to.", "Tko je to?"},
        {"Jesi li dobro.", "Jesi li dobro?"},
    };
    for (const char* voice : FORMANT_VOICES) {
        Engine engine;
        REQUIRE(laprdus_set_voice(engine.handle, voice, NO_DATA) == LAPRDUS_OK);
        REQUIRE(laprdus_set_sentence_pause(engine.handle, 0) == LAPRDUS_OK);
        for (float speed : {1.0f, 2.0f}) {
            REQUIRE(laprdus_set_speed(engine.handle, speed) == LAPRDUS_OK);
            for (const auto& c : cases) {
                double statement_end = final_pitch_hz(speak(engine.handle, c.statement));
                double question_end = final_pitch_hz(speak(engine.handle, c.question));
                INFO(voice << " " << speed << " " << c.question);
                REQUIRE(statement_end > 50.0);
                REQUIRE(question_end > statement_end * 1.19);   // three semitones
            }
        }

        // A one-syllable exclamation starts higher than the statement.
        REQUIRE(laprdus_set_speed(engine.handle, 1.0f) == LAPRDUS_OK);
        double statement_peak = pitch_hz(speak(engine.handle, "Ne."), 0.0, 0.7);
        double exclamation_peak = pitch_hz(speak(engine.handle, "Ne!"), 0.0, 0.7);
        REQUIRE(exclamation_peak > statement_peak * 1.09);
    }
}

TEST_CASE("A word of one vowel is a syllable of its own", "[formant][prosody]") {
    // The conjunctions i and a and the prepositions u and o used to be
    // shortened as clitics and once more next to a vowel, and were lost
    // between their neighbours ("ja i ona" as "jajona"). They now add a
    // full short vowel to the clause.
    auto ms = [](const std::vector<int16_t>& audio) {
        return static_cast<double>(audio.size()) * 1000.0 / 22050.0;
    };
    for (const char* voice : FORMANT_VOICES) {
        Engine engine;
        REQUIRE(laprdus_set_voice(engine.handle, voice, NO_DATA) == LAPRDUS_OK);
        INFO(voice);
        REQUIRE(ms(speak(engine.handle, "Ja i ona.")) - ms(speak(engine.handle, "Ja ona.")) >= 70.0);
        REQUIRE(ms(speak(engine.handle, "Idemo u Vukovar.")) -
                ms(speak(engine.handle, "Idemo Vukovar.")) >= 65.0);
    }
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

namespace {

std::vector<int16_t> spell(LaprdusHandle engine, const char* text) {
    int16_t* samples = nullptr;
    LaprdusAudioFormat format;
    int32_t count = laprdus_synthesize_spelled(engine, text, &samples, &format);
    std::vector<int16_t> audio;
    if (count > 0 && samples) audio.assign(samples, samples + count);
    laprdus_free_buffer(samples);
    return audio;
}

} // namespace

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

TEST_CASE("Letters are spelled by their names in the language of the voice", "[formant][spelling]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    REQUIRE(laprdus_set_spelling_speed(engine.handle, 100) == LAPRDUS_OK);
    REQUIRE(laprdus_set_spelling_pause(engine.handle, 0) == LAPRDUS_OK);

    // A spelled letter is its name spoken: "b" is "be", "j" is "je" (not "jot"),
    // "h" is "ha", and an uppercase letter the same as a lowercase one
    REQUIRE(spell(engine.handle, "b") == speak(engine.handle, "be"));
    REQUIRE(spell(engine.handle, "j") == speak(engine.handle, "je"));
    REQUIRE(spell(engine.handle, "h") == speak(engine.handle, "ha"));
    REQUIRE(spell(engine.handle, "B") == spell(engine.handle, "b"));
    REQUIRE(spell(engine.handle, "\xC5\xA1") == speak(engine.handle, "e\xC5\xA1"));     // š: eš
    // Y is ȉpsilōn (HJP), not ipsìlon as the rule for loans in -on had it
    REQUIRE(spell(engine.handle, "Y") == speak(engine.handle, u8"ȉpsilon"));
    REQUIRE(spell(engine.handle, "y") != speak(engine.handle, u8"ipsìlon"));

    // Serbian: lje, nje, ša; Cyrillic letters the same as Latin ones
    REQUIRE(laprdus_set_voice(engine.handle, "stojan", NO_DATA) == LAPRDUS_OK);
    REQUIRE(spell(engine.handle, "\xC5\xA1") == speak(engine.handle, "\xC5\xA1" "a"));  // š: ša
    REQUIRE(spell(engine.handle, "\xD1\x99") == speak(engine.handle, "lje"));          // љ
    REQUIRE(spell(engine.handle, "\xD0\xB1") == spell(engine.handle, "b"));             // б
    REQUIRE(spell(engine.handle, "\xD0\x88") == speak(engine.handle, "je"));           // Ј
    REQUIRE(spell(engine.handle, "y") == speak(engine.handle, u8"ȉpsilōn"));

    // Bosnian follows the Croatian names
    REQUIRE(laprdus_set_voice(engine.handle, "mirsad", NO_DATA) == LAPRDUS_OK);
    REQUIRE(spell(engine.handle, "\xC5\xA1") == speak(engine.handle, "e\xC5\xA1"));
    REQUIRE(spell(engine.handle, "l") == speak(engine.handle, "el"));
}

TEST_CASE("Letters are spelled by their sounds when asked", "[formant][spelling]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    REQUIRE(laprdus_get_spelling_mode(engine.handle) == LAPRDUS_SPELLING_LETTER_NAMES);
    REQUIRE(laprdus_set_spelling_pause(engine.handle, 0) == LAPRDUS_OK);

    std::vector<int16_t> name = spell(engine.handle, "f");
    REQUIRE(laprdus_set_spelling_mode(engine.handle, LAPRDUS_SPELLING_LETTER_SOUNDS) == LAPRDUS_OK);
    REQUIRE(laprdus_get_spelling_mode(engine.handle) == LAPRDUS_SPELLING_LETTER_SOUNDS);
    std::vector<int16_t> sound = spell(engine.handle, "f");

    // The sound is not the name, and it is audible
    REQUIRE(sound != name);
    REQUIRE(!sound.empty());
    REQUIRE(rms(sound) > 300.0);

    // Every letter of the alphabet, the digraph ligatures and the Cyrillic
    // letters have a sound
    const char* letters[] = {
        "a", "b", "c", "\xC4\x8D", "\xC4\x87", "d", "\xC4\x91", "e", "f", "g", "h", "i",
        "j", "k", "l", "m", "n", "o", "p", "r", "s", "\xC5\xA1", "t", "u", "v", "z",
        "\xC5\xBE", "\xC7\x89", "\xC7\x8C", "\xC7\x86",
        "\xD0\xB1", "\xD1\x99", "\xD1\x9F", "\xD1\x9B", "\xD0\x82", "Z", "\xC5\xA0",
    };
    for (const char* letter : letters) {
        INFO(letter);
        std::vector<int16_t> audio = spell(engine.handle, letter);
        REQUIRE(audio.size() > 22050 / 20);     // at least 50 ms
        REQUIRE(rms(audio) > 150.0);
    }

    // Digits, punctuation and symbols keep their names, and so do the
    // foreign letters (ku, duplo ve, iks, ipsilon) in every language
    const char* named[] = { "7", "q", "w", "x", "y", "Q", "W", "X", "Y" };
    for (const char* voice : { "zvonko", "stojan", "mirsad" }) {
        REQUIRE(laprdus_set_voice(engine.handle, voice, NO_DATA) == LAPRDUS_OK);
        for (const char* character : named) {
            INFO(voice << " " << character);
            REQUIRE(laprdus_set_spelling_mode(engine.handle, LAPRDUS_SPELLING_LETTER_NAMES) == LAPRDUS_OK);
            std::vector<int16_t> by_name = spell(engine.handle, character);
            REQUIRE(laprdus_set_spelling_mode(engine.handle, LAPRDUS_SPELLING_LETTER_SOUNDS) == LAPRDUS_OK);
            REQUIRE(spell(engine.handle, character) == by_name);
        }
    }
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);

    // Spelling a word gives the sounds with the spelling pause between them
    REQUIRE(laprdus_set_spelling_pause(engine.handle, 100) == LAPRDUS_OK);
    std::vector<int16_t> word = spell(engine.handle, "sat");
    REQUIRE(word.size() > spell(engine.handle, "s").size() + spell(engine.handle, "a").size());
}

TEST_CASE("Spelling speed slows the spelled characters", "[formant][spelling]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    REQUIRE(laprdus_set_spelling_pause(engine.handle, 0) == LAPRDUS_OK);
    REQUIRE(laprdus_get_spelling_speed(engine.handle) == 50);

    // 100% is the speech rate itself
    REQUIRE(laprdus_set_spelling_speed(engine.handle, 100) == LAPRDUS_OK);
    std::vector<int16_t> full = spell(engine.handle, "b");
    REQUIRE(full == speak(engine.handle, "be"));

    REQUIRE(laprdus_set_spelling_speed(engine.handle, 50) == LAPRDUS_OK);
    std::vector<int16_t> half = spell(engine.handle, "b");
    REQUIRE(laprdus_set_spelling_speed(engine.handle, 0) == LAPRDUS_OK);
    std::vector<int16_t> slow = spell(engine.handle, "b");
    REQUIRE(half.size() > full.size() * 1.3);
    REQUIRE(slow.size() > half.size() * 1.3);

    // The speech rate is unchanged afterwards
    REQUIRE(speak(engine.handle, "be") == full);

    // Out-of-range values are clamped
    REQUIRE(laprdus_set_spelling_speed(engine.handle, 250) == LAPRDUS_OK);
    REQUIRE(laprdus_get_spelling_speed(engine.handle) == 100);
    REQUIRE(laprdus_set_spelling_speed(engine.handle, -5) == LAPRDUS_OK);
    REQUIRE(laprdus_get_spelling_speed(engine.handle) == 0);
}

TEST_CASE("The user's spelling entries win, except a letter name in sound mode", "[formant][spelling]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    REQUIRE(laprdus_set_spelling_speed(engine.handle, 100) == LAPRDUS_OK);
    REQUIRE(laprdus_set_spelling_pause(engine.handle, 0) == LAPRDUS_OK);

    // The bundled dictionary first, the user's entries on top of it
    REQUIRE(laprdus_load_spelling_dictionary_from_memory(engine.handle,
        "{ \"entries\": [ { \"character\": \"*\", \"pronunciation\": \"zvjezdica\" } ] }", 0) == LAPRDUS_OK);
    REQUIRE(laprdus_add_spelling_entry(engine.handle, "b", "bum") == LAPRDUS_OK);
    REQUIRE(laprdus_add_spelling_entry(engine.handle, "j", "Jot") == LAPRDUS_OK);
    REQUIRE(laprdus_add_spelling_entry(engine.handle, "*", "zvijezda") == LAPRDUS_OK);

    // Letter names: the user's entries are spoken as written
    REQUIRE(spell(engine.handle, "b") == speak(engine.handle, "bum"));
    REQUIRE(spell(engine.handle, "j") == speak(engine.handle, "Jot"));
    REQUIRE(spell(engine.handle, "*") == speak(engine.handle, "zvijezda"));
    REQUIRE(spell(engine.handle, "c") == speak(engine.handle, "ce"));

    // Letter sounds: "bum" is not a letter's name and still wins, "Jot" is
    // one and gives way to the sound; a symbol keeps the user's entry
    REQUIRE(laprdus_set_spelling_mode(engine.handle, LAPRDUS_SPELLING_LETTER_SOUNDS) == LAPRDUS_OK);
    REQUIRE(spell(engine.handle, "b") == speak(engine.handle, "bum"));
    REQUIRE(spell(engine.handle, "j") != speak(engine.handle, "Jot"));
    REQUIRE(spell(engine.handle, "j") != speak(engine.handle, "je"));
    REQUIRE(spell(engine.handle, "*") == speak(engine.handle, "zvijezda"));

    // A foreign letter is named in both modes, by the user's entry if any
    REQUIRE(laprdus_add_spelling_entry(engine.handle, "w", "dvostruko ve") == LAPRDUS_OK);
    REQUIRE(spell(engine.handle, "w") == speak(engine.handle, "dvostruko ve"));
    REQUIRE(spell(engine.handle, "x") == speak(engine.handle, "iks"));
    REQUIRE(laprdus_set_spelling_mode(engine.handle, LAPRDUS_SPELLING_LETTER_NAMES) == LAPRDUS_OK);
    REQUIRE(spell(engine.handle, "w") == speak(engine.handle, "dvostruko ve"));

    // Loading the bundled dictionary again drops the user's entries
    REQUIRE(laprdus_load_spelling_dictionary_from_memory(engine.handle,
        "{ \"entries\": [ { \"character\": \"*\", \"pronunciation\": \"zvjezdica\" } ] }", 0) == LAPRDUS_OK);
    REQUIRE(spell(engine.handle, "*") == speak(engine.handle, "zvjezdica"));
}

TEST_CASE("A capital letter announced by a screen reader is spelled", "[formant][spelling]") {
    // NVDA "veliko N", TalkBack "veliko slovo N" and "велико Н", VoiceOver
    // "veliko početno slovo N", in English "cap N" and "capital N": the
    // words are spoken, the letter is spelled in the spelling mode and at
    // the spelling speed like any spelled character
    Engine engine;
    auto joined = [&](const char* words, const char* letter) {
        std::vector<int16_t> audio = speak(engine.handle, words);
        std::vector<int16_t> spelled = spell(engine.handle, letter);
        audio.insert(audio.end(), spelled.begin(), spelled.end());
        return audio;
    };
    struct Case { const char* voice; const char* text; const char* words; const char* letter; };
    const Case cases[] = {
        {"zvonko", "veliko N", "veliko", "N"},
        {"zvonko", "Veliko slovo N", "Veliko slovo", "N"},
        {"zvonko", u8"veliko početno slovo N", u8"veliko početno slovo", "N"},
        {"zvonko", "cap N", "cap", "N"},
        {"zvonko", "capital N", "capital", "N"},
        {"zvonko", u8"veliko Č", "veliko", u8"Č"},
        {"zvonko", "veliko N ", "veliko", "N"},
        {"stojan", u8"велико Н", u8"велико", u8"Н"},
        {"stojan", "veliko N", "veliko", "N"},
        {"mirsad", "Veliko N", "Veliko", "N"},
    };
    for (auto mode : {LAPRDUS_SPELLING_LETTER_SOUNDS, LAPRDUS_SPELLING_LETTER_NAMES}) {
        REQUIRE(laprdus_set_spelling_mode(engine.handle, mode) == LAPRDUS_OK);
        for (const auto& c : cases) {
            INFO(c.voice << " " << c.text << " mode " << mode);
            REQUIRE(laprdus_set_voice(engine.handle, c.voice, NO_DATA) == LAPRDUS_OK);
            REQUIRE(speak(engine.handle, c.text) == joined(c.words, c.letter));
        }
    }

    // In sound mode that is the sound, not the name the text used to give
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    REQUIRE(laprdus_set_spelling_mode(engine.handle, LAPRDUS_SPELLING_LETTER_SOUNDS) == LAPRDUS_OK);
    REQUIRE(speak(engine.handle, "veliko N") != joined("veliko", "en"));
    REQUIRE(speak(engine.handle, "veliko N") != speak(engine.handle, "veliko en"));

    // Running text with a letter in it is not an announcement and does not
    // depend on the spelling mode
    const char* texts[] = { "Plan B", "veliko N je slovo", "veliko more", "slovo N",
                            "veliko NN", "veliko 7" };
    for (const char* text : texts) {
        INFO(text);
        REQUIRE(laprdus_set_spelling_mode(engine.handle, LAPRDUS_SPELLING_LETTER_SOUNDS) == LAPRDUS_OK);
        const auto by_sound = speak(engine.handle, text);
        REQUIRE(laprdus_set_spelling_mode(engine.handle, LAPRDUS_SPELLING_LETTER_NAMES) == LAPRDUS_OK);
        REQUIRE(speak(engine.handle, text) == by_sound);
    }

    // The streaming API gives the same samples
    REQUIRE(laprdus_set_spelling_mode(engine.handle, LAPRDUS_SPELLING_LETTER_SOUNDS) == LAPRDUS_OK);
    LaprdusStreamHandle stream = laprdus_stream_begin(engine.handle, "veliko N");
    REQUIRE(stream != nullptr);
    std::vector<int16_t> streamed;
    int16_t buffer[1024];
    for (int guard = 0; guard < 100000 && !laprdus_stream_is_complete(stream); ++guard) {
        int32_t got = laprdus_stream_read(stream, buffer, 1024);
        if (got <= 0) break;
        streamed.insert(streamed.end(), buffer, buffer + got);
    }
    laprdus_stream_destroy(stream);
    REQUIRE(streamed == speak(engine.handle, "veliko N"));
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

// =============================================================================
// Singing presets
// =============================================================================

namespace {

struct SingingPreset { const char* id; const char* base; const char* language; };
const SingingPreset SINGING_PRESETS[] = {
    {"orguljas", "zvonko", "hr-HR"},
    {"klapa", "zvonko", "hr-HR"},
    {"trubac", "stojan", "sr-RS"},
    {"harmonikas", "stojan", "sr-RS"},
    {"sevdalija", "mirsad", "bs-BA"},
    {"sazlija", "mirsad", "bs-BA"},
    {"pjevac", "zvonko", "hr-HR"},
    {"pevac", "stojan", "sr-RS"},
    {"solist", "mirsad", "bs-BA"},
    {"becarac", "zvonko", "hr-HR"},
};

} // namespace

TEST_CASE("Singing presets are formant voices of their base voice's language", "[formant][singing]") {
    for (const auto& preset : SINGING_PRESETS) {
        INFO(preset.id);
        LaprdusVoiceInfo info;
        REQUIRE(laprdus_get_voice_info_by_id(preset.id, &info) == LAPRDUS_OK);
        REQUIRE(std::string(info.language_code) == preset.language);
        REQUIRE(std::string(info.base_voice_id) == preset.base);
        REQUIRE(info.data_filename == nullptr);
        REQUIRE(info.base_pitch == 1.0f);
        REQUIRE(laprdus_get_nominal_wpm(preset.id) == laprdus_get_nominal_wpm(preset.base));

        // No data directory needed, and the text is rendered.
        Engine engine;
        REQUIRE(laprdus_set_voice(engine.handle, preset.id, NO_DATA) == LAPRDUS_OK);
        std::vector<int16_t> sung = speak(engine.handle, "Dobar dan, kako ste?");
        REQUIRE(sung.size() > 22050);
        REQUIRE(peak(sung) > 3000);
    }
}

TEST_CASE("Singing is deterministic and starts the song over with every utterance", "[formant][singing]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "klapa", NO_DATA) == LAPRDUS_OK);
    std::vector<int16_t> first = speak(engine.handle, "La la la la la la");
    std::vector<int16_t> again = speak(engine.handle, "La la la la la la");
    REQUIRE(first == again);

    // The clauses of one utterance continue the song: sung as two clauses
    // the second "la la la" sits on later notes than the first.
    std::vector<int16_t> two_clauses = speak(engine.handle, "La la la, la la la");
    std::vector<int16_t> one_clause = speak(engine.handle, "La la la");
    REQUIRE(two_clauses.size() > one_clause.size());
}

TEST_CASE("Singing presets differ from each other and from the speaking voice", "[formant][singing]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    std::vector<int16_t> spoken = speak(engine.handle, "Dobar dan");
    REQUIRE(laprdus_set_voice(engine.handle, "orguljas", NO_DATA) == LAPRDUS_OK);
    std::vector<int16_t> organ = speak(engine.handle, "Dobar dan");
    REQUIRE(laprdus_set_voice(engine.handle, "klapa", NO_DATA) == LAPRDUS_OK);
    std::vector<int16_t> klapa = speak(engine.handle, "Dobar dan");

    // Notes last longer than spoken syllables.
    REQUIRE(organ.size() > spoken.size());
    REQUIRE(organ != klapa);

    // And back to speech.
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    REQUIRE(speak(engine.handle, "Dobar dan") == spoken);
}

TEST_CASE("Rate sets the tempo and pitch transposes the song", "[formant][singing]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "harmonikas", NO_DATA) == LAPRDUS_OK);
    std::vector<int16_t> normal = speak(engine.handle, "Ma ma ma ma");
    REQUIRE(laprdus_set_speed(engine.handle, 2.0f) == LAPRDUS_OK);
    std::vector<int16_t> fast = speak(engine.handle, "Ma ma ma ma");
    REQUIRE(fast.size() < normal.size() * 0.65);
    REQUIRE(fast.size() > normal.size() * 0.4);
    REQUIRE(laprdus_set_speed(engine.handle, 1.0f) == LAPRDUS_OK);

    // The accordion has no vibrato, so a held note's pitch is steady and
    // the user pitch moves it by an octave exactly.
    std::vector<int16_t> high = speak(engine.handle, "Ma");
    REQUIRE(laprdus_set_user_pitch(engine.handle, 0.5f) == LAPRDUS_OK);
    std::vector<int16_t> low = speak(engine.handle, "Ma");
    // (The autocorrelation helper is confused by the two beating reeds, so
    // the octave itself is checked with tools/formant/pitch.py, not here.)
    REQUIRE(low.size() == high.size());
    REQUIRE(low != high);
}

TEST_CASE("Inflection level sets the vibrato of a sung voice", "[formant][singing]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "sevdalija", NO_DATA) == LAPRDUS_OK);
    std::vector<int16_t> with = speak(engine.handle, "Aaa");
    REQUIRE(laprdus_set_inflection_level(engine.handle, 0.0f) == LAPRDUS_OK);
    std::vector<int16_t> without = speak(engine.handle, "Aaa");
    REQUIRE(with.size() == without.size());
    REQUIRE(with != without);
}

TEST_CASE("Dry presets sing at the voice's own pitch, legato", "[formant][singing]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "pjevac", NO_DATA) == LAPRDUS_OK);
    // Zvonko speaks around 112 Hz; his song must stay within an octave of it.
    std::vector<int16_t> sung = speak(engine.handle, "Oj ti vilo, vilo Velebita");
    double f = pitch_hz(sung, 0.2, 0.8);
    REQUIRE(f > 70.0);
    REQUIRE(f < 230.0);
    // Syllables that follow each other are joined: no silent gap between
    // the vowels of "vi-lo vi-lo".
    std::vector<int16_t> legato = speak(engine.handle, "Ma ma ma ma");
    REQUIRE(gaps(legato).longest_ms < 30.0);
}

TEST_CASE("Singing presets spell as well", "[formant][singing]") {
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "trubac", NO_DATA) == LAPRDUS_OK);
    int16_t* samples = nullptr;
    LaprdusAudioFormat format;
    int32_t count = laprdus_synthesize_spelled(engine.handle, "abc", &samples, &format);
    REQUIRE(count > 0);
    laprdus_free_buffer(samples);
}

// =============================================================================
// User accent lexicon
// =============================================================================

TEST_CASE("The user's accent lexicon moves the stress of words and verbs", "[formant][lexicon]") {
    // Words the built-in lexicon does not know get the first syllable.
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    REQUIRE(speak(engine.handle, "balkon") != speak(engine.handle, "balk\xC3\xB3n"));       // balkón
    REQUIRE(speak(engine.handle, "balzama") != speak(engine.handle, "balz\xC3\xA1ma"));     // balzáma
    REQUIRE(speak(engine.handle, "zatrubim") != speak(engine.handle, "zatr\xC3\xB9" "bim"));  // zatrùbim

    const char* json =
        "{ \"version\": \"1.0\", \"entries\": [\n"
        "  { \"word\": \"balk'o:n*\", \"comment\": \"balk\\u00f3n, balk\\u00f3na\" },\n"
        "  { \"word\": \"balz'a:m|a|u|om\" },\n"
        "  { \"word\": \"kami'o:n*\", \"language\": \"sr\" },\n"
        "  { \"verb\": \"zatr'ub=i\" },\n"
        "  { \"word\": \"kabina\" },\n"
        "  { \"verb\": \"xyz\" }\n"
        "] }";
    REQUIRE(laprdus_load_accent_lexicon_from_memory(engine.handle, json, 0) == LAPRDUS_OK);
    std::string report = laprdus_get_accent_lexicon_report(engine.handle);
    INFO(report);
    // Two stems and three paradigm forms, one verb; the two malformed
    // entries are reported.
    REQUIRE(report.find("5 words, 1 verbs; 2 rejected (first: kabina: no stress or length mark)") == 0);

    // A stem covers the inflected forms, a paradigm only the forms listed.
    REQUIRE(speak(engine.handle, "balkon") == speak(engine.handle, "balk\xC3\xB3n"));
    REQUIRE(speak(engine.handle, "balkona") == speak(engine.handle, "balk\xC3\xB3na"));
    REQUIRE(speak(engine.handle, "balzama") == speak(engine.handle, "balz\xC3\xA1ma"));
    REQUIRE(speak(engine.handle, "balzamom") == speak(engine.handle, "balz\xC3\xA1mom"));
    REQUIRE(speak(engine.handle, "balzam") != speak(engine.handle, "balz\xC3\xA1m"));
    // A verb stem reaches every form the built-in verbs do.
    REQUIRE(speak(engine.handle, "zatrubiti") == speak(engine.handle, "zatr\xC3\xB9" "biti"));
    REQUIRE(speak(engine.handle, "zatrubim") == speak(engine.handle, "zatr\xC3\xB9" "bim"));
    REQUIRE(speak(engine.handle, "zatrubio") == speak(engine.handle, "zatr\xC3\xB9" "bio"));
    // An entry for Serbian only leaves Zvonko alone.
    REQUIRE(speak(engine.handle, "kamion") != speak(engine.handle, "kami\xC3\xB3n"));       // kamión
    // Built-in entries, suffix rules and accent marks in the text still apply.
    REQUIRE(speak(engine.handle, "kontrola") == speak(engine.handle, "kontr\xC3\xB3la"));
    REQUIRE(speak(engine.handle, "organizacija") == speak(engine.handle, "organiz\xC3\xA1" "cija"));
    REQUIRE(speak(engine.handle, "b\xC3\xA0lkon") != speak(engine.handle, "balk\xC3\xB3n"));

    // The lexicon survives a voice change, and the Serbian voice gets its entry.
    REQUIRE(laprdus_set_voice(engine.handle, "stojan", NO_DATA) == LAPRDUS_OK);
    REQUIRE(speak(engine.handle, "balkon") == speak(engine.handle, "balk\xC3\xB3n"));
    REQUIRE(speak(engine.handle, "kamion") == speak(engine.handle, "kami\xC3\xB3n"));
    REQUIRE(laprdus_set_voice(engine.handle, "zvonko", NO_DATA) == LAPRDUS_OK);
    REQUIRE(speak(engine.handle, "balkon") == speak(engine.handle, "balk\xC3\xB3n"));

    // Clearing restores the defaults; so does a file with nothing usable.
    laprdus_clear_accent_lexicon(engine.handle);
    REQUIRE(speak(engine.handle, "balkon") != speak(engine.handle, "balk\xC3\xB3n"));
    REQUIRE(std::string(laprdus_get_accent_lexicon_report(engine.handle)).empty());
    REQUIRE(laprdus_load_accent_lexicon_from_memory(engine.handle, json, 0) == LAPRDUS_OK);
    REQUIRE(speak(engine.handle, "balkon") == speak(engine.handle, "balk\xC3\xB3n"));
    REQUIRE(laprdus_load_accent_lexicon_from_memory(engine.handle, "{ \"entries\": [ { \"word\": \"\" } ] }", 0)
            == LAPRDUS_ERROR_LOAD_FAILED);
    REQUIRE(speak(engine.handle, "balkon") != speak(engine.handle, "balk\xC3\xB3n"));
    REQUIRE(laprdus_load_accent_lexicon_from_memory(engine.handle, "not json at all", 0)
            == LAPRDUS_ERROR_LOAD_FAILED);
    REQUIRE(laprdus_load_accent_lexicon(engine.handle, "/nonexistent/accents.json")
            == LAPRDUS_ERROR_LOAD_FAILED);
}

TEST_CASE("The user's accent lexicon is read from a file and reports bad entries", "[formant][lexicon]") {
    const char* tmp = std::getenv("TMPDIR");
    std::string path = std::string(tmp ? tmp : "/tmp") + "/laprdus_test_accents.json";
    {
        std::ofstream out(path, std::ios::binary);
        out << "{ \"entries\": [ { \"word\": \"balk'o:n*\" }, { \"word\": \"bal'k'on\" }, "
               "{ \"word\": \"balkon:\" }, { \"verb\": \"zam'ol=q\" }, "
               "{ \"word\": \"x'y\", \"language\": \"de\" } ] }";
    }
    Engine engine;
    REQUIRE(laprdus_set_voice(engine.handle, "mirsad", NO_DATA) == LAPRDUS_OK);
    REQUIRE(laprdus_load_accent_lexicon(engine.handle, path.c_str()) == LAPRDUS_OK);
    std::string report = laprdus_get_accent_lexicon_report(engine.handle);
    INFO(report);
    REQUIRE(report.find("1 words, 0 verbs; 4 rejected (first: bal'k'on: two stress marks)") == 0);
    REQUIRE(speak(engine.handle, "balkon") == speak(engine.handle, "balk\xC3\xB3n"));
    std::remove(path.c_str());
}
