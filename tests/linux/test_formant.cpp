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

// Fundamental frequency of one window, by autocorrelation.
double pitch_at(const std::vector<int16_t>& audio, size_t start, size_t window) {
    double best_corr = 0.0;
    size_t best_lag = 0;
    for (size_t lag = 22050 / 400; lag <= 22050 / 50 && lag < window; ++lag) {
        double corr = 0.0;
        for (size_t i = 0; i + lag < window; ++i) {
            corr += static_cast<double>(audio[start + i]) * audio[start + i + lag];
        }
        corr /= static_cast<double>(window - lag);
        if (corr > best_corr) {
            best_corr = corr;
            best_lag = lag;
        }
    }
    return best_lag ? 22050.0 / static_cast<double>(best_lag) : 0.0;
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
