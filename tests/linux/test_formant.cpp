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

// Fundamental frequency where the voice ends: the last ~46 ms that are
// clearly periodic and not yet faded out. Takes the shortest period that
// correlates nearly as well as the best one, so a moving pitch is not
// mistaken for its octave below.
double final_pitch_hz(const std::vector<int16_t>& audio) {
    const size_t window = 1024;
    const size_t min_lag = 22050 / 300;
    const size_t max_lag = 22050 / 60;
    if (audio.size() < window) return 0.0;
    double loudest = 0.0;
    for (size_t start = 0; start + window <= audio.size(); start += 128) {
        loudest = std::max(loudest, energy_at(audio, start, window));
    }

    std::vector<double> corr(max_lag + 2, 0.0);
    for (size_t start = audio.size() - window; start >= 128; start -= 128) {
        if (energy_at(audio, start, window) < 0.03 * loudest) continue;
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
        if (best < 0.7) continue;
        for (size_t lag = min_lag; lag <= max_lag; ++lag) {
            if (corr[lag] >= 0.9 * best && corr[lag] >= corr[lag - 1] &&
                corr[lag] >= corr[lag + 1]) {
                return 22050.0 / static_cast<double>(lag);
            }
        }
    }
    return 0.0;
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
        {"proslijedim", "proslij\xC3\xA9" "dim"},   // proslijédim
        {"proslijedite", "proslij\xC3\xA9" "dite"},   // proslijédite
        {"proslije\xC4\x91" "eno", "proslij\xC3\xA9\xC4\x91" "eno"},   // proslijéđeno
        {"proslje\xC4\x91ujem", "proslj\xC3\xA8\xC4\x91ujem"},   // prosljèđujem
        {"proslje\xC4\x91ivati", "proslje\xC4\x91\xC3\xADvati"},   // prosljeđívati
        {"podijeli", "podij\xC3\xA9li"},   // podijéli
        {"podijelio", "podij\xC3\xA9lio"},   // podijélio
        {"podijelim", "podij\xC3\xA9lim"},   // podijélim
        {"podijeljeno", "podij\xC3\xA9ljeno"},   // podijéljeno
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
        {"isklju\xC4\x8D" "eno", "isklj\xC3\xBA\xC4\x8D" "eno"},   // iskljúčeno
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
        {"dvadeset", "dv\xC3\xA2" "deset"},   // dvâdeset
        {"dvadeseti", "dv\xC3\xA2" "deseti"},   // dvâdeseti
        {"dvadeset jedan", "dv\xC3\xA2" "deset jedan"},   // dvâdeset jedan
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
        {"odijela", "odij\xC3\xA9la"},   // odijéla
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

    // Mirsad follows the dictionaries: the present and the passive participle
    // have the accent one syllable earlier.
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

TEST_CASE("The tens keep their long first vowel", "[formant][text]") {
    // dvádeset, trídeset (HJP): the first vowel is long, and the accent
    // peaks inside it (the long falling accent of the lexicon entry). A
    // short first vowel, or the rising accent with its peak on "de", was
    // heard as "dva deset". The vowel's length shows in the duration: the
    // word is longer than with the short falling accent written out, and
    // the reading is neither the short falling nor the long rising one.
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
        REQUIRE(plain != long_rising);
        std::vector<int16_t> thirty = speak(engine.handle, "trideset");
        REQUIRE(thirty.size() > speak(engine.handle, "tr\xC8\x89" "deset").size() + 22050 / 40);   // trȉdeset
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
        {"u\xC4\x8D" "enika", "u\xC4\x8D" "en\xC3\xADka"},   // učeníka
        {"u\xC4\x8D" "enici", "u\xC4\x8D" "en\xC3\xAD" "ci"},   // učeníci
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
        {"u\xC4\x8D" "enik", "u\xC4\x8D\xC3\xA8nik"},   // učènik (ùčenik)
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
        REQUIRE(same("7.listopada.u20:03", "sedmi listopada u dvadeset nula tri"));
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
    }
    SECTION("A number with a period before a lowercase word is an ordinal") {
        // The period is silent and does not end the clause. Before an
        // uppercase word it ends the sentence as before.
        REQUIRE(same("7. listopada 2026.", "sedmi listopada dvije tisu\xC4\x87" "e dvadeset \xC5\xA1" "est."));
        REQUIRE(same("u 19. stolje\xC4\x87u", "u devetnaesti stolje\xC4\x87u"));
        REQUIRE(same("1990. godine", "tisu\xC4\x87u devetsto devedeseti godine"));
        REQUIRE(same("2000. godine", "dvijetisu\xC4\x87iti godine"));
        REQUIRE(same("100. put", "stoti put"));
        REQUIRE(same("21. put", "dvadeset prvi put"));
        REQUIRE(same("7.listopada", "sedmi listopada"));
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
        REQUIRE(same("7.10.2026", "sedmi deseti dvije tisu\xC4\x87" "e dvadeset \xC5\xA1" "est"));
        // The sections share one engine in this harness
        REQUIRE(laprdus_set_number_mode(engine.handle, LAPRDUS_NUMBER_MODE_WHOLE) == LAPRDUS_OK);
    }
    SECTION("Clock times and dates are read without the separators") {
        REQUIRE(same("12:30", "dvanaest trideset"));
        REQUIRE(same("9:05", "devet nula pet"));
        REQUIRE(same("12:30:45", "dvanaest trideset \xC4\x8D" "etrdeset pet"));
        REQUIRE(same("u 12:30 sati", "u dvanaest trideset sati"));
        REQUIRE(same("7.10.2026.", "sedmi deseti dvije tisu\xC4\x87" "e dvadeset \xC5\xA1" "est."));
        REQUIRE(same("7. 10. 2026.", "sedmi deseti dvije tisu\xC4\x87" "e dvadeset \xC5\xA1" "est."));
        REQUIRE(same("07.10.26", "sedmi deseti dvadeset \xC5\xA1" "est"));
        REQUIRE(same("31.12.1999", "trideset prvi dvanaesti tisu\xC4\x87u devetsto devedeset devet"));
        // Not a time or a date: the marks are read
        REQUIRE(same("3:1", "tri dvoto\xC4\x8Dka jedan"));
        REQUIRE(same("25:00", "dvadeset pet dvoto\xC4\x8Dka nula nula"));
        REQUIRE(same("7.13.2026", "sedam to\xC4\x8Dka trinaest to\xC4\x8Dka dvije tisu\xC4\x87" "e dvadeset \xC5\xA1" "est"));
    }
    SECTION("Colon and exclamation mark inside a word, silent comma and question mark") {
        REQUIRE(same("a:b", "a dvoto\xC4\x8Dka be"));
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
        REQUIRE(same("7.10.2026", "sedmi deseti dve hiljade dvadeset \xC5\xA1" "est"));
        REQUIRE(same("2000. godine", "dvehiljaditi godine"));
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

    // Serbian: lje, nje, ša; Cyrillic letters the same as Latin ones
    REQUIRE(laprdus_set_voice(engine.handle, "stojan", NO_DATA) == LAPRDUS_OK);
    REQUIRE(spell(engine.handle, "\xC5\xA1") == speak(engine.handle, "\xC5\xA1" "a"));  // š: ša
    REQUIRE(spell(engine.handle, "\xD1\x99") == speak(engine.handle, "lje"));          // љ
    REQUIRE(spell(engine.handle, "\xD0\xB1") == spell(engine.handle, "b"));             // б
    REQUIRE(spell(engine.handle, "\xD0\x88") == speak(engine.handle, "je"));           // Ј

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

    // Every letter of the alphabet, the foreign letters, the digraph
    // ligatures and the Cyrillic letters have a sound
    const char* letters[] = {
        "a", "b", "c", "\xC4\x8D", "\xC4\x87", "d", "\xC4\x91", "e", "f", "g", "h", "i",
        "j", "k", "l", "m", "n", "o", "p", "r", "s", "\xC5\xA1", "t", "u", "v", "z",
        "\xC5\xBE", "q", "w", "x", "y", "\xC7\x89", "\xC7\x8C", "\xC7\x86",
        "\xD0\xB1", "\xD1\x99", "\xD1\x9F", "\xD1\x9B", "\xD0\x82", "Z", "\xC5\xA0",
    };
    for (const char* letter : letters) {
        INFO(letter);
        std::vector<int16_t> audio = spell(engine.handle, letter);
        REQUIRE(audio.size() > 22050 / 20);     // at least 50 ms
        REQUIRE(rms(audio) > 150.0);
    }

    // Digits, punctuation and symbols keep their names
    REQUIRE(laprdus_set_spelling_mode(engine.handle, LAPRDUS_SPELLING_LETTER_NAMES) == LAPRDUS_OK);
    std::vector<int16_t> digit_name = spell(engine.handle, "7");
    REQUIRE(laprdus_set_spelling_mode(engine.handle, LAPRDUS_SPELLING_LETTER_SOUNDS) == LAPRDUS_OK);
    REQUIRE(spell(engine.handle, "7") == digit_name);

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

    // Loading the bundled dictionary again drops the user's entries
    REQUIRE(laprdus_load_spelling_dictionary_from_memory(engine.handle,
        "{ \"entries\": [ { \"character\": \"*\", \"pronunciation\": \"zvjezdica\" } ] }", 0) == LAPRDUS_OK);
    REQUIRE(spell(engine.handle, "*") == speak(engine.handle, "zvjezdica"));
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
