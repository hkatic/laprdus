// -*- coding: utf-8 -*-
// Direct front-end tests for compound stress and word-boundary metadata.
// Build instructions are in CLAUDE.md and docs/formant.md.

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include "formant/formant_frontend.hpp"
#include <algorithm>
#include <vector>

using namespace laprdus;
using namespace laprdus::formant;

namespace {

Utterance read(const Frontend& frontend, const std::u32string& text) {
    return frontend.process(text, Punctuation::PERIOD);
}

std::vector<int> stressed_syllables(const Utterance& utterance) {
    std::vector<int> result;
    for (const auto& phone : utterance.phones) {
        if (phone.nucleus && phone.stressed) result.push_back(phone.syllable);
    }
    return result;
}

std::vector<Phone> nuclei(const Utterance& utterance) {
    std::vector<Phone> result;
    for (const auto& phone : utterance.phones) {
        if (phone.nucleus) result.push_back(phone);
    }
    return result;
}

void require_one_word(const Utterance& utterance) {
    REQUIRE(!utterance.phones.empty());
    REQUIRE(utterance.phones.front().word_start);
    REQUIRE(std::count_if(utterance.phones.begin(), utterance.phones.end(),
                         [](const Phone& p) { return p.word_start; }) == 1);
    for (const auto& phone : utterance.phones) {
        REQUIRE(phone.word == 0);
        REQUIRE(phone.word_syllables == utterance.syllable_count);
        REQUIRE(phone.ph != Ph::SIL);
    }
}

} // namespace

TEST_CASE("Pacijentima through dusa retain sourced accents and inflections", "[frontend][accents][health-oct11]") {
    struct Case { const char32_t* text; int syllables; int stress; Accent tone; bool long_vowel; };
    const auto F = Accent::Falling;
    const auto R = Accent::Rising;
    const Case cases[] = {
        {U"pacijentima", 5, 1, R, false}, {U"pacijent", 3, 1, R, false},
        {U"pacijenta", 4, 1, R, false}, {U"pacijenata", 5, 1, R, false},
        {U"veseli", 3, 0, F, false}, {U"veseo", 3, 0, F, false},
        {U"vesela", 3, 0, F, false}, {U"veseloga", 4, 0, F, false},
        {U"veselima", 4, 0, F, false}, {U"veseliji", 4, 1, R, false},
        {U"veseliti", 4, 1, R, false}, {U"veselimo", 4, 1, R, false},
        {U"zaštita", 3, 0, R, false}, {U"zaštite", 3, 0, R, false},
        {U"zaštitom", 3, 0, R, false}, {U"zaštitama", 4, 0, R, false},
        {U"posjekotina", 5, 2, R, false}, {U"posjekotine", 5, 2, R, false},
        {U"posjekotinom", 5, 2, R, false}, {U"posjekotinama", 6, 2, R, false},
        {U"nepravilnosti", 5, 2, R, true}, {U"nepravilnost", 4, 2, R, true},
        {U"nepravilnošću", 5, 2, R, true}, {U"nepravilnostima", 6, 2, R, true},
        {U"nježno", 2, 0, F, false}, {U"nježan", 2, 0, F, false},
        {U"nježna", 2, 0, R, false}, {U"nježni", 2, 0, F, false},
        {U"nježniji", 3, 0, R, false},
        {U"poliklinika", 5, 2, R, false}, {U"poliklinike", 5, 2, R, false},
        {U"poliklinici", 5, 2, R, false}, {U"poliklinikama", 6, 2, R, false},
        {U"cedevita", 4, 2, R, false}, {U"Cedevita", 4, 2, R, false},
        {U"Cedevite", 4, 2, R, false}, {U"Cedevitom", 4, 2, R, false},
        {U"dostojanstvo", 4, 2, R, false}, {U"dostojanstva", 4, 2, R, false},
        {U"dostojanstvom", 4, 2, R, false}, {U"dostojanstvima", 5, 2, R, false},
        {U"gladan", 2, 0, F, true}, {U"gladna", 2, 0, R, true},
        {U"gladno", 2, 0, F, true}, {U"gladni", 2, 0, F, true},
        {U"gladnijima", 4, 0, R, false},
        {U"glad", 1, 0, F, true}, {U"gladi", 2, 0, F, true},
        {U"glađu", 2, 0, F, true},
        {U"dana", 2, 0, F, true}, {U"dan", 1, 0, F, true},
        {U"danu", 2, 0, F, true}, {U"danima", 3, 0, F, true},
        {U"biće", 2, 0, R, true}, {U"bića", 2, 0, R, true},
        {U"bićima", 3, 0, R, true},
        {U"dijeta", 3, 1, R, true}, {U"dijeti", 3, 1, R, true},
        {U"dijetom", 3, 1, R, true}, {U"dijetama", 4, 1, R, true},
        {U"dijete", 2, 0, R, true}, // preserve the default child reading
        {U"duša", 2, 0, R, true}, {U"duši", 2, 0, F, true},
        {U"dušu", 2, 0, F, true}, {U"dušo", 2, 0, F, true},
        {U"duše", 2, 0, F, true}, {U"dušom", 2, 0, R, true},
        {U"dušama", 3, 0, R, true},
        // User-confirmed female name and middle syllable; length remains
        // provisional. The short rising rendering is an explicit policy.
        {U"Dženana", 3, 1, R, false}, {U"Dženane", 3, 1, R, false},
        {U"Dženani", 3, 1, R, false}, {U"Dženanu", 3, 1, R, false},
        {U"Dženanom", 3, 1, R, false},
    };
    const Frontend frontend(VoiceLanguage::Croatian);
    for (size_t i = 0; i < std::size(cases); ++i) {
        CAPTURE(i);
        const auto& c = cases[i];
        const auto utterance = read(frontend, c.text);
        REQUIRE(utterance.syllable_count == c.syllables);
        const auto vowels = nuclei(utterance);
        REQUIRE(std::count_if(vowels.begin(), vowels.end(), [](const Phone& p) { return p.stressed; }) == 1);
        REQUIRE(vowels.at(c.stress).stressed);
        REQUIRE(vowels.at(c.stress).accent == c.tone);
        REQUIRE(vowels.at(c.stress).is_long == c.long_vowel);
        for (const auto& p : vowels) if (!p.stressed) REQUIRE(!p.is_long);
    }
}

TEST_CASE("Health list homographs use bounded cues and preserve overrides", "[frontend][accents][health-oct11]") {
    struct Case { const char32_t* text; int word; int stress; Accent tone; bool long_vowel; };
    const auto F = Accent::Falling;
    const auto R = Accent::Rising;
    const Case cases[] = {
        {U"veseli ljudi", 0, 0, F, false}, {U"Oni su veseli", 2, 0, F, false},
        {U"te vesele žene", 1, 0, F, false}, {U"s veselim ljudima", 1, 0, F, false},
        {U"to me veseli", 2, 1, R, false}, {U"Veseli me", 0, 1, R, false},
        {U"veselim se", 0, 1, R, false}, {U"vesele se", 0, 1, R, false},
        {U"svakog dana", 1, 0, F, true}, {U"dva dana", 1, 0, F, true},
        {U"tri dana", 1, 0, F, true}, {U"pet dana", 1, 0, R, true},
        {U"nekoliko dana", 1, 0, R, true}, {U"tjedan dana", 1, 0, R, true},
        {U"pet dugih dana", 2, 0, R, true}, {U"od dana", 1, 0, F, true},
        {U"o danu", 1, 0, R, true}, {U"o tom danu", 2, 0, R, true},
        {U"prema danu", 1, 0, F, true},
        {U"bez duše", 1, 0, R, true}, {U"iz dubine duše", 2, 0, R, true},
        {U"mirne duše", 1, 0, R, true}, {U"dvije duše", 1, 0, F, true},
        {U"mrtve duše", 1, 0, F, true}, {U"na duši", 1, 0, F, true},
        {U"više zaštita", 1, 0, F, false}, {U"ta zaštita", 1, 0, R, false},
        {U"pet dȃna", 1, 0, F, true}, {U"o dȃnu", 1, 0, F, true},
        {U"to me vȅseli", 2, 0, F, false}, {U"vesèli ljudi", 0, 1, R, false},
        {U"bez dȗše", 1, 0, F, true}, {U"više zàštita", 1, 0, R, false},
        {U"Dženána", 0, 1, R, true},
    };
    const Frontend frontend(VoiceLanguage::Croatian);
    for (size_t i = 0; i < std::size(cases); ++i) {
        CAPTURE(i);
        const auto& c = cases[i];
        std::vector<Phone> vowels;
        for (const auto& p : read(frontend, c.text).phones)
            if (p.word == c.word && p.nucleus) vowels.push_back(p);
        REQUIRE(vowels.at(c.stress).stressed);
        REQUIRE(vowels.at(c.stress).accent == c.tone);
        REQUIRE(vowels.at(c.stress).is_long == c.long_vowel);
    }
    Frontend custom(VoiceLanguage::Croatian);
    custom.set_user_lexicon(UserLexicon::parse(
        R"({"entries":[{"word":"v^eseli"},{"word":"d^a:na"},{"word":"d^a:nu"},{"word":"d^u:še"},{"word":"z/aštita"},{"word":"džen/a:na"}]})"));
    REQUIRE(nuclei(read(custom, U"veseli se"))[0].stressed);
    REQUIRE(nuclei(read(custom, U"pet dana"))[1].accent == F);
    REQUIRE(nuclei(read(custom, U"o danu"))[1].accent == F);
    REQUIRE(nuclei(read(custom, U"bez duše"))[1].accent == F);
    REQUIRE(nuclei(read(custom, U"više zaštita"))[2].accent == R);
    REQUIRE(nuclei(read(custom, U"Dženana"))[1].is_long);
    // The name table remains capitalization-sensitive and Croatian-only.
    REQUIRE(nuclei(read(frontend, U"dženana"))[0].stressed);
    REQUIRE(nuclei(read(frontend, U"DŽENANA"))[0].stressed);
    REQUIRE(nuclei(read(Frontend(VoiceLanguage::Bosnian), U"Dženana"))[0].stressed);
}

TEST_CASE("Croatian tiflo compounds have two stresses within one word", "[frontend][tiflo]") {
    // The user explicitly requested independent accents for both components,
    // including established compounds that dictionaries accent as one unit.
    struct Case { const char32_t* text; int syllables; int second_stress; };
    const Case cases[] = {
        {U"tiflonova", 4, 2},
        {U"TifloNova", 4, 2},
        {U"TIFLONOVA", 4, 2},
        {U"Tiflonove", 4, 2},
        {U"tiflonovom", 4, 2},
        {U"tiflotehnika", 5, 2},
        {U"tiflotehnici", 5, 2},
        {U"tiflotehnikama", 6, 2},
        {U"tiflolog", 3, 2},
        {U"tiflologija", 5, 2},
        {U"tiflopedagogija", 7, 4},
        // A productive component can carry its syllable on r rather than a vowel.
        {U"tifloprst", 3, 2},
    };
    const Frontend frontend(VoiceLanguage::Croatian);
    for (const auto& c : cases) {
        const auto utterance = read(frontend, c.text);
        REQUIRE(utterance.syllable_count == c.syllables);
        REQUIRE((stressed_syllables(utterance) == std::vector<int>{0, c.second_stress}));
        require_one_word(utterance);
        const auto vowels = nuclei(utterance);
        REQUIRE(vowels[0].accent == Accent::Falling);
        REQUIRE(!vowels[0].is_long);
        REQUIRE(!vowels[1].stressed);
    }
}

TEST_CASE("Tiflo components retain their independent syllables and prosody", "[frontend][tiflo]") {
    const Frontend frontend(VoiceLanguage::Croatian);
    struct Case { const char32_t* compound; const char32_t* tail; };
    const Case cases[] = {
        {U"tiflonova", U"nova"},
        {U"tiflotehnika", U"tehnika"},
        {U"tiflotehnikama", U"tehnikama"},
        {U"tiflologija", U"logija"},
        {U"tiflopedagogija", U"pedagogija"},
        {U"tifloprst", U"prst"},
    };
    const auto prefix = nuclei(read(frontend, U"tiflo"));
    REQUIRE(prefix.size() == 2);
    for (const auto& c : cases) {
        const auto compound = nuclei(read(frontend, c.compound));
        auto separate = prefix;
        const auto tail = nuclei(read(frontend, c.tail));
        separate.insert(separate.end(), tail.begin(), tail.end());
        REQUIRE(compound.size() == separate.size());
        for (size_t i = 0; i < compound.size(); ++i) {
            REQUIRE(compound[i].ph == separate[i].ph);
            REQUIRE(compound[i].stressed == separate[i].stressed);
            REQUIRE(compound[i].accent == separate[i].accent);
            REQUIRE(compound[i].is_long == separate[i].is_long);
        }
    }
    const auto technical = nuclei(read(frontend, U"tiflotehnika"));
    REQUIRE(technical[2].accent == Accent::Rising); // dictionary tèhnika
    REQUIRE(!technical[2].is_long);
    // Splitting the first spelling must not merge its following real word.
    const auto phrase = read(frontend, U"TifloNova radi");
    REQUIRE(std::count_if(phrase.phones.begin(), phrase.phones.end(),
                         [](const Phone& p) { return p.word_start; }) == 2);
    REQUIRE(phrase.phones.front().word != phrase.phones.back().word);
}

TEST_CASE("Written tiflo accents override component stress", "[frontend][tiflo]") {
    Frontend frontend(VoiceLanguage::Croatian);
    const auto written = read(frontend, U"tiflónova");
    REQUIRE((stressed_syllables(written) == std::vector<int>{1}));
    require_one_word(written);
    REQUIRE(nuclei(written)[1].accent == Accent::Rising);
    REQUIRE(nuclei(written)[1].is_long);
    const auto written_tail = read(frontend, U"tiflonóva");
    REQUIRE((stressed_syllables(written_tail) == std::vector<int>{2}));
    REQUIRE(nuclei(written_tail)[2].is_long);
    // Written accents retain priority even over a user entry for the word.
    frontend.set_user_lexicon(UserLexicon::parse(
        R"({"entries":[{"word":"tiflon/o:va"}]})"));
    REQUIRE((stressed_syllables(read(frontend, U"tiflónova")) == std::vector<int>{1}));
}

TEST_CASE("User tiflo words stems and verbs override splitting", "[frontend][tiflo]") {
    struct Case { const char* json; const char32_t* text; int stress; bool verb; };
    const Case cases[] = {
        {R"({"entries":[{"word":"tifl/o:nova"}]})", U"TifloNova", 1, false},
        {R"({"entries":[{"word":"tifl/o:nov*"}]})", U"tiflonovom", 1, false},
        {R"({"entries":[{"verb":"tiflon/ov=ia"}]})", U"tiflonoviti", 2, true},
        {R"({"entries":[{"verb":"tiflon/ov=ia"}]})", U"TifloNoviti", 2, true},
    };
    for (const auto& c : cases) {
        Frontend frontend(VoiceLanguage::Croatian);
        UserLexicon::Report report;
        const auto lexicon = UserLexicon::parse(c.json, &report);
        REQUIRE(report.rejected == 0);
        REQUIRE((c.verb ? report.verbs : report.words) == 1);
        frontend.set_user_lexicon(lexicon);
        const auto utterance = read(frontend, c.text);
        REQUIRE((stressed_syllables(utterance) == std::vector<int>{c.stress}));
        require_one_word(utterance);
    }
    // A foreign-language override must not suppress the Croatian preference.
    Frontend frontend(VoiceLanguage::Croatian);
    frontend.set_user_lexicon(UserLexicon::parse(
        R"({"entries":[{"word":"tifl/o:nova","language":"sr"}]})"));
    REQUIRE((stressed_syllables(read(frontend, U"tiflonova")) == std::vector<int>{0, 2}));
    frontend.set_user_lexicon(nullptr);
    REQUIRE((stressed_syllables(read(frontend, U"tiflonova")) == std::vector<int>{0, 2}));
}

TEST_CASE("The tiflo pronunciation preference stays Croatian", "[frontend][tiflo]") {
    for (const auto language : {VoiceLanguage::Serbian, VoiceLanguage::Bosnian}) {
        const Frontend frontend(language);
        const auto brand = read(frontend, U"tiflonova");
        REQUIRE((stressed_syllables(brand) == std::vector<int>{1}));
        REQUIRE(nuclei(brand)[1].is_long);
        require_one_word(brand);
        const auto term = read(frontend, U"tiflologija");
        REQUIRE((stressed_syllables(term) == std::vector<int>{2}));
        require_one_word(term);
    }
    const Frontend frontend(VoiceLanguage::Croatian);
    const auto prefix = read(frontend, U"tiflo");
    REQUIRE((stressed_syllables(prefix) == std::vector<int>{0}));
    REQUIRE(prefix.syllable_count == 2);
    require_one_word(prefix);
}

TEST_CASE("Collected Croatian words follow checked dictionary accents", "[frontend][accents][collected]") {
    // Školski rječnik/HJP headwords and paradigms, not suffix predictions.
    // Source disagreements, homographs and the listener-preferred evènata
    // pronunciation are documented in docs/formant.md.
    struct Case { const char32_t* text; int syllables; int stress; Accent tone; bool long_vowel; };
    const auto R = Accent::Rising;
    const auto F = Accent::Falling;
    const Case cases[] = {
        {U"slavlje", 2, 0, F, true}, {U"festivala", 4, 2, R, true},
        {U"evenata", 4, 1, R, false}, {U"Evenata", 4, 1, R, false},
        {U"razloga", 3, 0, R, true}, {U"zahvalnost", 3, 1, R, true},
        {U"nasmiješeno", 4, 0, R, false}, {U"putovanja", 4, 2, R, true},
        {U"krafna", 2, 0, R, true}, {U"glas", 1, 0, F, true},
        {U"izvela", 3, 0, R, false}, {U"odvela", 3, 0, R, false},
        {U"strpljenje", 3, 1, R, true}, {U"zavela", 3, 0, R, false},
        {U"dovela", 3, 0, R, false}, {U"pažnja", 2, 0, R, false},
        {U"hrabrost", 2, 0, R, true}, {U"samopouzdanje", 6, 4, R, true},
        {U"samodopadan", 5, 2, R, false}, {U"samodopadnost", 5, 2, R, false},
        {U"nabava", 3, 0, R, false}, {U"konobar", 3, 0, R, false},
        {U"namigne", 3, 0, R, false}, {U"ukraden", 3, 1, R, false},
        {U"obijene", 4, 1, R, false}, {U"ubijene", 4, 1, R, false},
        {U"razbijene", 4, 1, R, false},
        {U"počinitelji", 5, 1, R, false}, {U"Špansko", 2, 0, R, true},
        {U"špansko", 2, 0, R, true}, {U"društvo", 2, 0, R, false},
        {U"rečeno", 3, 0, R, false}, {U"problema", 3, 1, R, true},
        {U"travnjaku", 3, 0, F, true}, {U"terenu", 3, 1, R, true},
        {U"trener", 2, 0, R, true}, {U"smisao", 3, 0, F, true},
        {U"stanka", 2, 0, F, true}, {U"stvari", 2, 0, F, true},
        {U"neću", 2, 0, R, true}, {U"prilika", 3, 0, R, true},
        {U"komponenti", 4, 1, R, false}, {U"kolačić", 3, 1, R, false},
        {U"kolačići", 4, 1, R, false}, {U"rep", 1, 0, F, true},
        {U"krokodil", 3, 1, R, false}, {U"zec", 1, 0, F, true},
        {U"vuk", 1, 0, F, true}, {U"žirafa", 3, 1, R, false},
        {U"pantera", 3, 1, R, true},
        // Paradigm contrasts: do not extend one form's accent blindly.
        {U"festival", 3, 1, R, false}, {U"festivalima", 5, 2, R, true},
        {U"problem", 2, 0, R, false}, {U"problemi", 3, 1, R, true},
        {U"teren", 2, 0, R, false}, {U"tereni", 3, 1, R, true},
        {U"glasovi", 3, 0, F, false}, {U"repovi", 3, 0, F, false},
        {U"zečevi", 3, 0, F, false}, {U"vukovi", 3, 0, F, false},
        {U"razlozima", 4, 0, R, true}, {U"travnjacima", 4, 0, F, true},
        {U"strpljenjem", 3, 1, R, true}, {U"zahvalnošću", 4, 1, R, true},
        {U"samodopadnima", 6, 2, R, false}, {U"počiniteljima", 6, 1, R, false},
        {U"namignuti", 4, 0, R, false}, {U"namignuo", 4, 0, R, false},
        {U"namigni", 3, 0, R, false}, {U"namignemo", 4, 0, R, false},
        {U"izveli", 3, 0, R, false}, {U"odveo", 3, 0, R, false},
        {U"zavelo", 3, 0, R, false}, {U"dovele", 3, 0, R, false},
        {U"izveden", 3, 1, R, false}, {U"odvede", 3, 1, R, false},
        {U"zavedi", 3, 1, R, false}, {U"dovedena", 4, 1, R, false},
        {U"ukradenima", 5, 1, R, false}, {U"obijenima", 5, 1, R, false},
        {U"ubijenog", 4, 1, R, false}, {U"rečenima", 4, 0, R, false},
        {U"razbijen", 3, 1, R, false}, {U"razbijeno", 4, 1, R, false},
        {U"razbijenima", 5, 1, R, false},
        {U"Španskoga", 3, 0, R, true}, {U"Španskom", 2, 0, R, true},
        {U"stanci", 2, 0, F, true}, {U"prilici", 3, 0, R, true},
        {U"smisla", 2, 0, F, true}, {U"smislovi", 3, 0, R, false},
        {U"stvarima", 3, 0, R, true}, {U"nećeš", 2, 0, F, true},
        {U"nećemo", 3, 0, F, true}, {U"komponenta", 4, 2, R, false},
        {U"komponenata", 5, 1, R, false}, {U"kolačićima", 5, 1, R, false},
        {U"krokodila", 4, 1, R, false}, {U"panterama", 4, 1, R, true},
    };
    const Frontend frontend(VoiceLanguage::Croatian);
    for (size_t i = 0; i < std::size(cases); ++i) {
        CAPTURE(i);
        const auto& c = cases[i];
        const auto utterance = read(frontend, c.text);
        REQUIRE(utterance.syllable_count == c.syllables);
        REQUIRE((stressed_syllables(utterance) == std::vector<int>{c.stress}));
        const auto vowels = nuclei(utterance);
        REQUIRE(vowels[c.stress].accent == c.tone);
        REQUIRE(vowels[c.stress].is_long == c.long_vowel);
        require_one_word(utterance);
    }
    const auto events = nuclei(read(frontend, U"evenata"));
    REQUIRE(std::none_of(events.begin(), events.end(), [](const Phone& p) { return p.is_long; }));
}

TEST_CASE("Collected nouns preserve dictionary tone length and inflection", "[frontend][accents][collected]") {
    // Linguistic expectations from ŠR/HJP, including contrasts the generic
    // suffix rules cannot predict. Source disagreements: docs/formant.md.
    struct Case { const char32_t* text; int syllables; int stress; Accent tone; bool long_vowel; };
    const auto R = Accent::Rising;
    const auto F = Accent::Falling;
    const Case cases[] = {
        {U"komarac", 3, 1, R, true}, {U"komarca", 3, 1, R, true},
        {U"komarcima", 4, 1, R, true}, {U"komaraca", 4, 0, R, false},
        {U"komarče", 3, 0, F, false},
        {U"žalac", 2, 0, F, false}, {U"žalca", 2, 0, R, true},
        {U"žalcima", 3, 0, R, true}, {U"žalaca", 3, 0, F, false},
        {U"samac", 2, 0, R, true}, {U"samcem", 2, 0, R, true},
        {U"samci", 2, 0, R, true}, {U"samaca", 3, 0, F, true},
        {U"šarac", 2, 0, R, true}, {U"šarca", 2, 0, R, true},
        {U"šarcima", 3, 0, R, true}, {U"šaraca", 3, 0, F, true},
        {U"sokak", 2, 0, R, false}, {U"sokaka", 3, 1, R, true},
        {U"sokaci", 3, 1, R, true}, {U"sokacima", 4, 1, R, true},
        {U"planinarenje", 5, 2, R, true}, {U"planinarenjima", 6, 2, R, true},
        {U"rimac", 2, 0, R, true}, {U"Rimac", 2, 0, R, true},
        {U"rimca", 2, 0, R, true}, {U"Rimcem", 2, 0, R, true},
        {U"Bare", 2, 0, R, true},
        {U"općenito", 4, 1, R, false}, {U"općenitima", 5, 1, R, false},
        {U"medijateka", 5, 3, R, true}, {U"medijateci", 5, 3, R, true},
        {U"medijatekama", 6, 3, R, true},
        {U"rotor", 2, 0, F, true}, {U"rotorima", 4, 0, F, true},
        {U"aktivnost", 3, 1, R, true}, {U"aktivnošću", 4, 1, R, true},
        {U"aktivnostima", 5, 1, R, true},
        {U"element", 3, 1, R, false}, {U"elemenata", 5, 1, R, false},
        {U"snimka", 2, 0, F, false}, {U"snimci", 2, 0, F, false},
        {U"snimkama", 3, 0, F, false}, {U"snimaka", 3, 0, F, false},
        {U"dodira", 3, 0, F, false}, {U"dodirima", 4, 0, F, false},
        {U"naredba", 3, 0, R, true}, {U"naredaba", 4, 0, R, true},
        {U"zvuk", 1, 0, F, true}, {U"zvuka", 2, 0, F, true},
        {U"zvukovi", 3, 0, F, false}, {U"zvuci", 2, 0, F, true},
        {U"glavni", 2, 0, R, false}, {U"glavnoga", 3, 0, R, false},
    };
    const Frontend frontend(VoiceLanguage::Croatian);
    for (size_t i = 0; i < std::size(cases); ++i) {
        CAPTURE(i);
        const auto& c = cases[i];
        const auto utterance = read(frontend, c.text);
        REQUIRE(utterance.syllable_count == c.syllables);
        REQUIRE((stressed_syllables(utterance) == std::vector<int>{c.stress}));
        const auto vowels = nuclei(utterance);
        REQUIRE(vowels[c.stress].accent == c.tone);
        REQUIRE(vowels[c.stress].is_long == c.long_vowel);
        require_one_word(utterance);
    }
    // Capitalization selects the name. Keep the ordinary lowercase and
    // all-capitals analyses separate, and do not capture related verbs.
    const auto name = nuclei(read(frontend, U"Goran Bare"));
    REQUIRE(name[2].stressed);
    REQUIRE(name[2].accent == R);
    REQUIRE(name[2].is_long);
    REQUIRE(!nuclei(read(frontend, U"bare"))[0].is_long);
    REQUIRE(!nuclei(read(frontend, U"BARE"))[0].is_long);
    REQUIRE((stressed_syllables(read(frontend, U"dodirati")) == std::vector<int>{1}));
    REQUIRE((stressed_syllables(read(frontend, U"narediti")) == std::vector<int>{1}));
    REQUIRE((stressed_syllables(read(frontend, U"elementaran")) == std::vector<int>{3}));
}

TEST_CASE("Case context selects Croatian accent homographs", "[frontend][accents][collected]") {
    struct Case { const char32_t* text; int stress; Accent tone; bool long_vowel; };
    const auto R = Accent::Rising;
    const auto F = Accent::Falling;
    const Case cases[] = {
        {U"o glasu", 0, R, true}, {U"na dobrom glasu", 0, R, true},
        {U"prema glasu", 0, F, true}, {U"po repu", 0, R, true},
        {U"prema repu", 0, F, true}, {U"pet stvari", 0, R, true},
        {U"mnogo stvari", 0, R, true}, {U"dvije stvari", 0, F, true},
        {U"bez stvari", 0, F, true}, {U"o važnoj stvari", 0, R, true},
        {U"gledam na stvari", 0, F, true}, {U"o komponenti", 2, R, false},
        {U"prema novoj komponenti", 2, R, false}, {U"pet komponenti", 1, R, false},
        {U"mnogo novih komponenti", 1, R, false},
        {U"pet žirafa", 1, R, true}, {U"nekoliko pažnja", 0, R, true},
        {U"nekoliko nabava", 0, F, false}, {U"pet društva", 0, R, true},
        {U"pet vuka", 0, R, true}, {U"pet komponenta", 1, R, false},
        {U"o zvuku", 0, R, true}, {U"o novom zvuku", 0, R, true},
        {U"prema zvuku", 0, F, true}, {U"pet zvuka", 0, R, true},
        {U"dvije snimke", 0, F, false}, {U"pet snimka", 0, F, true},
        // Explicit accents win over case cues.
        {U"o glȃsu", 0, F, true}, {U"pet stvȃri", 0, F, true},
        {U"o kompònenti", 1, R, false}, {U"pet žiràfa", 1, R, false},
        {U"o zvȗku", 0, F, true}, {U"pet snȉmka", 0, F, false},
    };
    Frontend frontend(VoiceLanguage::Croatian);
    for (size_t i = 0; i < std::size(cases); ++i) {
        CAPTURE(i);
        const auto& c = cases[i];
        const auto utterance = read(frontend, c.text);
        const int last_word = utterance.phones.back().word;
        std::vector<Phone> vowels;
        for (const auto& phone : utterance.phones)
            if (phone.word == last_word && phone.nucleus) vowels.push_back(phone);
        REQUIRE(std::count_if(vowels.begin(), vowels.end(), [](const Phone& p) { return p.stressed; }) == 1);
        REQUIRE(vowels[c.stress].stressed);
        REQUIRE(vowels[c.stress].accent == c.tone);
        REQUIRE(vowels[c.stress].is_long == c.long_vowel);
    }
    frontend.set_user_lexicon(UserLexicon::parse(
        R"({"entries":[{"word":"gl^a:su"},{"word":"komp/onenti"},{"word":"žir/afa"}]})"));
    const auto voice = nuclei(read(frontend, U"o glasu"));
    REQUIRE(voice[1].accent == F);
    const auto component = nuclei(read(frontend, U"o komponenti"));
    REQUIRE(component[2].stressed);
    const auto giraffes = nuclei(read(frontend, U"pet žirafa"));
    REQUIRE(!giraffes[2].is_long);
}

TEST_CASE("Croatian siguran and direktno keep their selected adjective accents", "[frontend][accents]") {
    // ŠR/HJP: sȉgūran and its positive forms, but sigùrnijī and sigúrnōst.
    // Croatian's lexicon omits post-accent length; stressed length is
    // checked independently so a broad sigur- stem cannot hide the contrast.
    struct Case { const char32_t* text; int stress; Accent tone; bool long_vowel; };
    const auto F = Accent::Falling;
    const auto R = Accent::Rising;
    const Case cases[] = {
        {U"siguran", 0, F, false}, {U"Siguran", 0, F, false},
        {U"SIGURAN", 0, F, false}, {U"sigurna", 0, F, false},
        {U"sigurno", 0, F, false}, {U"sigurni", 0, F, false},
        {U"sigurne", 0, F, false}, {U"sigurnu", 0, F, false},
        {U"sigurnog", 0, F, false}, {U"sigurnoga", 0, F, false},
        {U"sigurnome", 0, F, false}, {U"sigurnomu", 0, F, false},
        {U"sigurnoj", 0, F, false}, {U"sigurnih", 0, F, false},
        {U"sigurnim", 0, F, false}, {U"sigurnima", 0, F, false},
        {U"sigurniji", 1, R, false}, {U"sigurnije", 1, R, false},
        {U"sigurnijima", 1, R, false},
        {U"sigurnost", 1, R, true}, {U"sigurnošću", 1, R, true},
        {U"sigurnostima", 1, R, true},
        // User's HJP-listed dirèktno variant, including positive adjective
        // forms; the comparative already has the same short rising e.
        {U"direktno", 1, R, false}, {U"Direktno", 1, R, false},
        {U"DIREKTNO", 1, R, false}, {U"direktan", 1, R, false},
        {U"direktna", 1, R, false}, {U"direktni", 1, R, false},
        {U"direktne", 1, R, false}, {U"direktnu", 1, R, false},
        {U"direktnog", 1, R, false}, {U"direktnoga", 1, R, false},
        {U"direktnom", 1, R, false}, {U"direktnome", 1, R, false},
        {U"direktnomu", 1, R, false}, {U"direktnoj", 1, R, false},
        {U"direktnih", 1, R, false}, {U"direktnim", 1, R, false},
        {U"direktnima", 1, R, false}, {U"direktniji", 1, R, false},
        {U"direktnije", 1, R, false},
    };
    Frontend frontend(VoiceLanguage::Croatian);
    for (size_t i = 0; i < std::size(cases); ++i) {
        CAPTURE(i);
        const auto& c = cases[i];
        const auto utterance = read(frontend, c.text);
        REQUIRE((stressed_syllables(utterance) == std::vector<int>{c.stress}));
        const auto vowel = nuclei(utterance)[c.stress];
        REQUIRE(vowel.accent == c.tone);
        REQUIRE(vowel.is_long == c.long_vowel);
    }
    REQUIRE(nuclei(read(frontend, U"síguran"))[0].accent == R);
    REQUIRE(nuclei(read(frontend, U"síguran"))[0].is_long);
    REQUIRE((stressed_syllables(read(frontend, U"dìrektno")) == std::vector<int>{0}));
    frontend.set_user_lexicon(UserLexicon::parse(R"({"entries":[{"word":"sig/u:ran"}]})"));
    const auto custom = nuclei(read(frontend, U"siguran"));
    REQUIRE(custom[1].stressed);
    REQUIRE(custom[1].accent == R);
    REQUIRE(custom[1].is_long);
    frontend.set_user_lexicon(UserLexicon::parse(R"({"entries":[{"word":"d/irektno"}]})"));
    REQUIRE((stressed_syllables(read(frontend, U"direktno")) == std::vector<int>{0}));
    for (const auto language : {VoiceLanguage::Serbian, VoiceLanguage::Bosnian}) {
        const Frontend other(language);
        REQUIRE((stressed_syllables(read(other, U"direktno")) == std::vector<int>{0}));
    }
}

TEST_CASE("Collected household words retain dictionary accents across forms", "[frontend][accents][household]") {
    // Školski rječnik principal forms, with HJP for bȃjan/bȃjno.
    // See docs/formant.md for source disagreements and ambiguous spellings.
    struct Case { const char32_t* text; int stress; Accent tone; bool long_vowel; };
    const auto R = Accent::Rising;
    const auto F = Accent::Falling;
    const Case cases[] = {
        {U"kontejnera", 1, R, false}, {U"kontejner", 1, R, false},
        {U"kontejneru", 1, R, false}, {U"kontejnerom", 1, R, false},
        {U"kontejneri", 1, R, false}, {U"kontejnere", 1, R, false},
        {U"kontejnerima", 1, R, false},
        {U"komoda", 1, R, false}, {U"komode", 1, R, false},
        {U"komodi", 1, R, false}, {U"komodu", 1, R, false},
        {U"komodom", 1, R, false}, {U"komodama", 1, R, false},
        {U"stan", 0, F, true}, {U"stana", 0, F, true},
        {U"stanu", 0, F, true}, {U"stanom", 0, F, true},
        {U"stanovi", 0, F, false}, {U"stanova", 0, F, false},
        {U"stanove", 0, F, false}, {U"stanovima", 0, F, false},
        {U"ispuste", 0, R, false}, {U"ispust", 0, R, false},
        {U"ispusta", 0, R, false}, {U"ispustom", 0, R, false},
        {U"ispustima", 0, R, false}, {U"ispustim", 0, R, false},
        {U"ispustiš", 0, R, false}, {U"ispustimo", 0, R, false},
        {U"ispustiti", 1, R, false}, {U"ispustio", 1, R, false},
        {U"ispustila", 1, R, false}, {U"ispusti", 1, R, false},
        {U"ispušten", 0, R, false}, {U"ispuštena", 0, R, false},
        {U"sastav", 0, R, true}, {U"sastava", 0, R, true},
        {U"sastavu", 0, R, true}, {U"sastavom", 0, R, true},
        {U"sastave", 0, R, true}, {U"sastavima", 0, R, true},
        {U"rastava", 0, R, false}, {U"rastave", 0, R, false},
        {U"rastavi", 0, R, false}, {U"rastavom", 0, R, false},
        {U"rastavama", 0, R, false},
        {U"tajna", 0, F, true}, {U"tajnama", 0, F, true},
        {U"tajnoga", 0, F, true}, {U"tajnomu", 0, F, true},
        {U"sjajna", 0, R, true}, {U"sjajan", 0, R, true},
        {U"sjajno", 0, R, true}, {U"sjajni", 0, F, true},
        {U"sjajnoga", 0, F, true}, {U"sjajnome", 0, F, true},
        {U"sjajniji", 0, R, false}, {U"sjajnijima", 0, R, false},
        {U"bajna", 0, F, true}, {U"bajan", 0, F, true},
        {U"bajno", 0, F, true}, {U"bajnoga", 0, F, true},
        {U"bajnima", 0, F, true},
        {U"trajna", 0, R, true}, {U"trajan", 0, F, true},
        {U"trajno", 0, F, true}, {U"trajni", 0, F, true},
        {U"trajnoga", 0, F, true}, {U"trajnome", 0, F, true},
        {U"trajniji", 0, R, false}, {U"trajnijima", 0, R, false},
        {U"jajna", 0, F, true}, {U"jajni", 0, F, true},
        {U"jajnoga", 0, F, true}, {U"jajnima", 0, F, true},
        {U"jaje", 0, R, true}, {U"jaja", 0, R, true},
        {U"jaju", 0, R, true}, {U"jajem", 0, R, true},
        {U"jajima", 0, R, true}, {U"jajeta", 0, R, false},
        {U"jajetu", 0, R, false}, {U"jajetom", 0, R, false},
        {U"Kontejnera", 1, R, false}, {U"KOMODA", 1, R, false},
        {U"Sastav", 0, R, true}, {U"TRAJNA", 0, R, true},
    };
    const Frontend frontend(VoiceLanguage::Croatian);
    for (size_t i = 0; i < std::size(cases); ++i) {
        CAPTURE(i);
        const auto& c = cases[i];
        const auto utterance = read(frontend, c.text);
        REQUIRE((stressed_syllables(utterance) == std::vector<int>{c.stress}));
        const auto vowel = nuclei(utterance)[c.stress];
        REQUIRE(vowel.accent == c.tone);
        REQUIRE(vowel.is_long == c.long_vowel);
        require_one_word(utterance);
    }
}

TEST_CASE("Household noun and adjective homographs keep context and overrides", "[frontend][accents][household]") {
    struct Case { const char32_t* text; int stress; Accent tone; bool long_vowel; };
    const auto R = Accent::Rising;
    const auto F = Accent::Falling;
    const Case cases[] = {
        {U"u stanu", 0, R, true}, {U"u novom stanu", 0, R, true},
        {U"prema stanu", 0, F, true},
        {U"pet komoda", 1, R, true}, {U"mnogo komoda", 1, R, true},
        {U"nova komoda", 1, R, false}, {U"pet rastava", 0, F, false},
        {U"jedna rastava", 0, R, false}, {U"glazbeni sastavi", 0, R, true},
        {U"Sastavi", 0, R, false}, {U"sastaviti", 0, R, false},
        {U"sastavio", 0, R, false},
        {U"oni ispuste", 0, R, false}, {U"kroz ispuste", 0, R, false},
        {U"u stȃnu", 0, F, true}, {U"pet komòda", 1, R, false},
        {U"pet ràstava", 0, R, false},
        {U"sjȃjna", 0, F, true}, {U"trȃjna", 0, F, true},
        {U"komóda", 1, R, true}, {U"rȃstava", 0, F, true},
    };
    Frontend frontend(VoiceLanguage::Croatian);
    for (size_t i = 0; i < std::size(cases); ++i) {
        CAPTURE(i);
        const auto& c = cases[i];
        const auto utterance = read(frontend, c.text);
        const int last_word = utterance.phones.back().word;
        std::vector<Phone> vowels;
        for (const auto& phone : utterance.phones)
            if (phone.word == last_word && phone.nucleus) vowels.push_back(phone);
        REQUIRE(vowels[c.stress].stressed);
        REQUIRE(vowels[c.stress].accent == c.tone);
        REQUIRE(vowels[c.stress].is_long == c.long_vowel);
    }
    // The noun's rare vocative stȃne must not lengthen the verb stȁne.
    REQUIRE(!nuclei(read(frontend, U"stane"))[0].is_long);
    frontend.set_user_lexicon(UserLexicon::parse(
        R"({"entries":[{"word":"st^a:nu"},{"word":"kom/oda"},{"word":"r/a:stava"}]})"));
    REQUIRE(nuclei(read(frontend, U"u stanu"))[1].accent == F);
    REQUIRE(!nuclei(read(frontend, U"pet komoda"))[2].is_long);
    REQUIRE(nuclei(read(frontend, U"pet rastava"))[1].is_long);
}

TEST_CASE("Presvlacenje keeps the root accent across noun cases", "[frontend][accents]") {
    // ŠR presvláčēnje, G presvláčēnja: long rising a, not generic -énje.
    // Croatian output omits post-accent length, so the following e stays short.
    const Frontend frontend(VoiceLanguage::Croatian);
    for (const auto* text : {U"presvlačenje", U"presvlačenja", U"presvlačenju",
                            U"presvlačenjem", U"presvlačenjima",
                            U"Presvlačenje", U"PRESVLAČENJE"}) {
        const auto utterance = read(frontend, text);
        REQUIRE((stressed_syllables(utterance) == std::vector<int>{1}));
        const auto vowels = nuclei(utterance);
        REQUIRE(vowels[1].accent == Accent::Rising);
        REQUIRE(vowels[1].is_long);
        REQUIRE(!vowels[2].is_long);
    }
}

TEST_CASE("Croatian ivac nouns keep their dictionary paradigms", "[frontend][accents][ivac]") {
    // ŠR where available, HJP otherwise. The genitive plural is not
    // uniformly stressed like the lemma; -ljivac/-njivac are separate.
    struct Family {
        const char32_t* stem;
        int stress;
        bool long_vowel;
        int plural_stress;
        Accent plural_tone;
        bool plural_long;
    };
    const auto R = Accent::Rising;
    const auto F = Accent::Falling;
    const Family families[] = {
        {U"pozitiv", 2, true, 1, R, false},
        {U"negativ", 2, true, 1, R, false},
        {U"depresiv", 2, true, 1, R, false},
        {U"emotiv", 2, true, 1, R, false},
        {U"agresiv", 2, true, 1, R, false},
        {U"kreativ", 2, true, 1, R, false},
        {U"primitiv", 2, true, 1, R, false},
        {U"operativ", 3, true, 2, R, false},
        {U"konzervativ", 3, true, 2, R, false},
        {U"rekreativ", 3, true, 2, R, false},
        {U"naiv", 1, true, 0, R, false},
        {U"kriv", 0, true, 0, F, true},
        {U"siv", 0, true, 0, F, true},
        {U"živ", 0, true, 0, F, true},
        {U"sumnjiv", 0, false, 0, R, false},
        {U"plašljiv", 0, false, 0, R, false},
        {U"šutljiv", 0, false, 0, R, false},
        {U"stidljiv", 0, false, 0, R, false},
        {U"sramežljiv", 1, false, 1, R, false},
        {U"zavidljiv", 1, false, 1, R, false},
    };
    const Frontend frontend(VoiceLanguage::Croatian);
    for (const auto& family : families) {
        for (const auto* ending : {U"ac", U"ca", U"cu", U"cem", U"ci", U"ce", U"cima", U"aca"}) {
            const auto text = std::u32string(family.stem) + ending;
            const bool genitive_plural = std::u32string(ending) == U"aca";
            const int stress = genitive_plural ? family.plural_stress : family.stress;
            const auto utterance = read(frontend, text);
            REQUIRE((stressed_syllables(utterance) == std::vector<int>{stress}));
            const auto vowel = nuclei(utterance)[stress];
            REQUIRE(vowel.accent == (genitive_plural ? family.plural_tone : R));
            REQUIRE(vowel.is_long == (genitive_plural ? family.plural_long : family.long_vowel));
            require_one_word(utterance);
        }
    }
    // Only vocatives explicitly accented by ŠR/HJP are prescribed here.
    struct Case { const char32_t* text; int stress; Accent tone; bool long_vowel; };
    const Case cases[] = {
        {U"pozitivče", 2, R, true}, {U"depresivče", 0, F, false},
        {U"kreativče", 1, R, false}, {U"primitivče", 0, F, false},
        {U"konzervativče", 0, F, false}, {U"naivče", 0, F, false},
        {U"krivče", 0, F, true}, {U"sivče", 0, R, true},
        {U"sumnjivče", 0, R, false}, {U"plašljivče", 0, R, false},
        {U"šutljivče", 0, R, false}, {U"stidljivče", 0, R, false},
        {U"sramežljivče", 1, R, false},
        {U"Pozitivac", 2, R, true}, {U"NEGATIVAC", 2, R, true},
        {U"Depresivaca", 1, R, false}, {U"EMOTIVACA", 1, R, false},
    };
    for (const auto& c : cases) {
        const auto utterance = read(frontend, c.text);
        REQUIRE((stressed_syllables(utterance) == std::vector<int>{c.stress}));
        const auto vowel = nuclei(utterance)[c.stress];
        REQUIRE(vowel.accent == c.tone);
        REQUIRE(vowel.is_long == c.long_vowel);
    }
}

TEST_CASE("Ivac noun accents respect written and user overrides", "[frontend][accents][ivac]") {
    Frontend frontend(VoiceLanguage::Croatian);
    auto vowels = nuclei(read(frontend, U"pȍzitivac"));
    REQUIRE(vowels[0].stressed);
    REQUIRE(vowels[0].accent == Accent::Falling);
    REQUIRE(!vowels[0].is_long);
    vowels = nuclei(read(frontend, U"žìvac"));
    REQUIRE(vowels[0].accent == Accent::Rising);
    REQUIRE(!vowels[0].is_long);
    frontend.set_user_lexicon(UserLexicon::parse(
        R"({"entries":[{"word":"poz/itivac"},{"word":"emot/i:vaca"}]})"));
    REQUIRE((stressed_syllables(read(frontend, U"pozitivac")) == std::vector<int>{1}));
    vowels = nuclei(read(frontend, U"emotivaca"));
    REQUIRE(vowels[2].stressed);
    REQUIRE(vowels[2].accent == Accent::Rising);
    REQUIRE(vowels[2].is_long);
}

TEST_CASE("Biti participles keep hiatus without breaking the jat reflex", "[frontend][accents][collected]") {
    for (const auto language : {VoiceLanguage::Croatian, VoiceLanguage::Serbian, VoiceLanguage::Bosnian}) {
        const Frontend frontend(language);
        for (const auto* text : {U"obijene", U"ubijene", U"razbijene", U"dobijene", U"neubijene"}) {
            const auto vowels = nuclei(read(frontend, text));
            REQUIRE(vowels.size() == (text == std::u32string(U"neubijene") ? 5 : 4));
        }
        for (const auto* text : {U"bijel", U"bijedan", U"pobijeljen", U"obaviješten"}) {
            const auto vowels = nuclei(read(frontend, text));
            REQUIRE(std::any_of(vowels.begin(), vowels.end(), [](const Phone& p) { return p.is_long; }));
        }
    }
    const Frontend frontend(VoiceLanguage::Croatian);
    const auto vowels = nuclei(read(frontend, U"obijene"));
    REQUIRE(vowels[1].ph == Ph::I);
    REQUIRE(vowels[2].ph == Ph::E);
    REQUIRE(!vowels[1].is_long);
    REQUIRE(!vowels[2].is_long);
}

TEST_CASE("Testament to nogomet families follow the checked Croatian paradigms", "[frontend][accents][testament-nogomet]") {
    // Školski rječnik principal forms; HJP for pediker/pedikerski and
    // nogometaški. ŠR is preferred for bògatstvo and medìcīnskī.
    // The Croatian voice omits unstressed lengths, including G plural.
    struct Family {
        std::vector<const char32_t*> words;
        int stress;
        Accent tone;
        bool long_vowel;
    };
    const auto F = Accent::Falling;
    const auto R = Accent::Rising;
    const Family families[] = {
        {{U"testament", U"testamenta", U"testamentu", U"testamentom",
          U"testamenti", U"testamente", U"testamentima", U"testamenata"}, 1, R, false},
        {{U"bogatstvo", U"bogatstva", U"bogatstvu", U"bogatstvom",
          U"bogatstvima", U"bogatstava"}, 0, R, false},
        {{U"sažetak", U"sažetka", U"sažetku", U"sažetkom", U"sažetke",
          U"sažeci", U"sažecima", U"sažetci", U"sažetcima"}, 1, R, true},
        {{U"sažetaka"}, 0, R, false},
        {{U"salon"}, 0, R, false},
        {{U"salona", U"salonu", U"salonom", U"saloni", U"salone", U"salonima"}, 1, R, true},
        {{U"salonski", U"salonska", U"salonskoj", U"salonskima"}, 0, R, false},
        {{U"pediker"}, 1, R, false},
        {{U"pedikera", U"pedikeru", U"pedikerom", U"pedikeri", U"pedikere", U"pedikerima"}, 2, R, true},
        {{U"pedikerski", U"pedikerska", U"pedikersko", U"pedikerske",
          U"pedikersku", U"pedikerskog", U"pedikerskoga", U"pedikerskom",
          U"pedikerskome", U"pedikerskomu", U"pedikerskoj", U"pedikerskih",
          U"pedikerskim", U"pedikerskima"}, 1, R, false},
        {{U"poklanjati", U"poklanjam", U"poklanjaš", U"poklanja", U"poklanjamo",
          U"poklanjate", U"poklanjaju", U"poklanjaj", U"poklanjajte",
          U"poklanjao", U"poklanjala", U"poklanjali", U"poklanjan",
          U"poklanjana", U"poklanjanje", U"poklanjanja", U"poklanjajući"}, 0, R, false},
        {{U"obzir", U"obzira", U"obziru", U"obzirom", U"obziri", U"obzire", U"obzirima"}, 0, F, false},
        {{U"medicinski", U"medicinska", U"medicinsko", U"medicinske", U"medicinsku",
          U"medicinskog", U"medicinskoga", U"medicinskom", U"medicinskome",
          U"medicinskomu", U"medicinskoj", U"medicinskih", U"medicinskim", U"medicinskima"}, 1, R, false},
        // The adjective must not change the noun medicína.
        {{U"medicina", U"medicine", U"medicinu", U"medicinom"}, 2, R, true},
        {{U"nogomet", U"nogometa", U"nogometu", U"nogometom", U"nogomete",
          U"nogometi", U"nogometima", U"Nogomet", U"NOGOMET"}, 0, F, false},
        {{U"nogometni", U"nogometna", U"nogometno", U"nogometne", U"nogometnu",
          U"nogometnog", U"nogometnoga", U"nogometnom", U"nogometnome",
          U"nogometnomu", U"nogometnoj", U"nogometnih", U"nogometnim", U"nogometnima"}, 0, F, false},
        // Related player nouns and their adjective have different accents.
        {{U"nogometaš"}, 2, R, false},
        {{U"nogometaša", U"nogometašu", U"nogometašem", U"nogometaši",
          U"nogometaše", U"nogometašima"}, 3, R, true},
        {{U"nogometaški", U"nogometaška", U"nogometaškoj", U"nogometaškima"}, 2, R, false},
        {{U"nogometašica", U"nogometašice", U"nogometašici", U"nogometašicu",
          U"nogometašicom", U"nogometašicama"}, 3, R, false},
    };
    const Frontend frontend(VoiceLanguage::Croatian);
    for (size_t i = 0; i < std::size(families); ++i) {
        const auto& family = families[i];
        for (size_t j = 0; j < family.words.size(); ++j) {
            CAPTURE(i);
            CAPTURE(j);
            const auto vowels = nuclei(read(frontend, family.words[j]));
            CHECK(std::count_if(vowels.begin(), vowels.end(), [](const Phone& p) { return p.stressed; }) == 1);
            CHECK(vowels.at(family.stress).stressed);
            CHECK(vowels.at(family.stress).accent == family.tone);
            CHECK(vowels.at(family.stress).is_long == family.long_vowel);
            for (const auto& p : vowels) if (!p.stressed) CHECK(!p.is_long);
        }
    }
}

TEST_CASE("Testament to nogomet accents survive context and allow overrides", "[frontend][accents][testament-nogomet]") {
    Frontend frontend(VoiceLanguage::Croatian);
    // In ordinary phrases, compare each target's nuclei with its isolated form.
    struct Case { const char32_t* phrase; const char32_t* word; int index; };
    const Case cases[] = {
        {U"s obzirom na", U"obzirom", 1},
        {U"ja poklanjam knjigu", U"poklanjam", 1},
        {U"medicinska sestra", U"medicinska", 0},
        {U"nogometna lopta", U"nogometna", 0},
        {U"iz salona", U"salona", 1},
        {U"nekoliko sažetaka", U"sažetaka", 1},
    };
    for (const auto& c : cases) {
        auto in_phrase = nuclei(read(frontend, c.phrase));
        in_phrase.erase(std::remove_if(in_phrase.begin(), in_phrase.end(),
                        [&](const Phone& p) { return p.word != c.index; }), in_phrase.end());
        const auto alone = nuclei(read(frontend, c.word));
        REQUIRE(in_phrase.size() == alone.size());
        for (size_t i = 0; i < alone.size(); ++i) {
            CHECK(in_phrase[i].stressed == alone[i].stressed);
            CHECK(in_phrase[i].accent == alone[i].accent);
            CHECK(in_phrase[i].is_long == alone[i].is_long);
        }
    }
    // HJP's alternatives and the vocatives can still be requested explicitly.
    CHECK(nuclei(read(frontend, U"bogàtstvo"))[1].stressed);
    CHECK(nuclei(read(frontend, U"mȅdicinska"))[0].accent == Accent::Falling);
    CHECK(nuclei(read(frontend, U"pȅdikeru"))[0].accent == Accent::Falling);
    CHECK(nuclei(read(frontend, U"nȍgometašu"))[0].accent == Accent::Falling);
    frontend.set_user_lexicon(UserLexicon::parse(
        R"({"entries":[{"word":"bog/atstvo"},{"word":"m^edicinska"},{"verb":"pokl/a:nj=a"}]})"));
    CHECK(nuclei(read(frontend, U"bogatstvo"))[1].stressed);
    CHECK(nuclei(read(frontend, U"medicinska"))[0].accent == Accent::Falling);
    CHECK(nuclei(read(frontend, U"poklanjam"))[1].is_long);
    CHECK(nuclei(read(frontend, U"pòklanjam"))[0].stressed);
}

TEST_CASE("Collected Croatian accents preserve stress tone and vowel quantity", "[frontend][accents][collected-oct11]") {
    // Independently checked against Školski rječnik/HJP principal forms.
    // mnom is the user's short-vowel preference; godišnjakinja is the
    // bare age component, by analogy with attested petogodišnjakinja.
    struct Case { const char32_t* text; int stress; Accent tone; bool long_vowel; };
    const auto F = Accent::Falling;
    const auto R = Accent::Rising;
    const Case cases[] = {
        {U"stanovnik", 1, R, false}, {U"stanovnika", 2, R, true},
        {U"stanovniku", 2, R, true}, {U"stanovnici", 2, R, true},
        {U"stanovnicima", 2, R, true}, {U"stanovniče", 0, F, false},
        {U"stanovnica", 1, R, false}, {U"stanovnicama", 1, R, false},
        {U"stanovništvo", 2, R, true},
        {U"stanar", 0, R, false}, {U"stanara", 1, R, true},
        {U"stanaru", 1, R, true}, {U"stanare", 1, R, true},
        {U"stanarima", 1, R, true},
        {U"pretpostave", 1, R, false}, {U"pretpostaviti", 1, R, false},
        {U"pretpostavim", 1, R, false}, {U"pretpostavio", 1, R, false},
        {U"pretpostavljena", 1, R, false},
        {U"tu", 0, F, true}, {U"često", 0, F, true}, {U"češće", 0, F, false},
        {U"magazin", 0, F, false}, {U"magazini", 0, F, false},
        {U"magazinima", 0, F, false}, {U"Magazin", 0, F, false},
        {U"nebesa", 1, R, false}, {U"nebesima", 1, R, false},
        {U"događaju", 1, R, true}, {U"događati", 1, R, true},
        {U"događao", 1, R, true}, {U"događa", 0, R, false},
        {U"događaj", 0, F, false}, {U"događaji", 0, F, false},
        {U"pad", 0, F, true}, {U"padu", 0, F, true},
        {U"padovi", 0, F, false}, {U"padovima", 0, F, false},
        {U"pada", 0, F, false}, {U"padala", 0, F, false},
        {U"osjećam", 0, R, false}, {U"osjećaju", 0, R, false},
        {U"osjećala", 0, R, false}, {U"osjećaj", 0, F, false},
        {U"ostavio", 0, R, false}, {U"ostaviti", 0, R, false},
        {U"ostavila", 0, R, false}, {U"ostavljen", 0, R, false},
        {U"mnom", 0, F, false},
        {U"radnik", 0, F, true}, {U"radnika", 0, F, true},
        {U"radnici", 0, F, true}, {U"radnicima", 0, F, true},
        {U"radniče", 0, F, true}, {U"radnica", 0, F, true},
        {U"radnicama", 0, F, true},
        {U"dokument", 1, R, false}, {U"dokumenti", 1, R, false},
        {U"dokumentima", 1, R, false}, {U"dokumenata", 1, R, false},
        {U"dokumentarac", 3, R, true},
        {U"pokušala", 0, R, false}, {U"pokušati", 0, R, false},
        {U"pokušao", 0, R, false}, {U"pokušaju", 0, R, false},
        {U"zavod", 0, R, true}, {U"zavodima", 0, R, true},
        {U"imovina", 1, R, false}, {U"imovinom", 1, R, false},
        {U"imovinama", 1, R, false},
        {U"susretao", 0, R, false}, {U"susretati", 0, R, false},
        {U"susretala", 0, R, false}, {U"susrećem", 0, R, false},
        {U"mirovina", 1, R, false}, {U"mirovini", 1, R, false},
        {U"mirovinama", 1, R, false},
        {U"ministarstvo", 2, R, true}, {U"ministarstva", 2, R, true},
        {U"ministarstvima", 2, R, true}, {U"ministarstava", 1, R, false},
        {U"navod", 0, R, true}, {U"navoda", 0, R, true},
        {U"navodima", 0, R, true},
        {U"godišnjak", 0, R, false}, {U"godišnjaka", 0, R, false},
        {U"godišnjacima", 0, R, false}, {U"godišnjakinja", 2, R, false},
        {U"godišnjakinjama", 2, R, false},
        {U"petogodišnjak", 2, R, false}, {U"petogodišnjakinja", 4, R, false},
        {U"dvadesetogodišnjakinjama", 6, R, false},
        {U"stablo", 0, R, false}, {U"stabla", 0, R, false},
        {U"stablu", 0, R, false}, {U"stablom", 0, R, false},
        {U"stablima", 0, R, false}, {U"stabala", 1, R, true},
        {U"zelenilo", 1, R, false}, {U"zelenila", 1, R, false},
        {U"zelenilu", 1, R, false}, {U"zelenilom", 1, R, false},
        {U"zelenilima", 1, R, false},
        {U"tobogan", 0, F, false}, {U"tobogana", 0, F, false},
        {U"toboganu", 0, F, false}, {U"toboganom", 0, F, false},
        {U"tobogani", 0, F, false}, {U"tobogane", 0, F, false},
        {U"toboganima", 0, F, false}, {U"Tobogan", 0, F, false},
    };
    const Frontend frontend(VoiceLanguage::Croatian);
    for (size_t i = 0; i < std::size(cases); ++i) {
        CAPTURE(i);
        const auto& c = cases[i];
        const auto vowels = nuclei(read(frontend, c.text));
        REQUIRE(std::count_if(vowels.begin(), vowels.end(), [](const Phone& p) { return p.stressed; }) == 1);
        REQUIRE(vowels.at(c.stress).stressed);
        REQUIRE(vowels.at(c.stress).accent == c.tone);
        REQUIRE(vowels.at(c.stress).is_long == c.long_vowel);
        for (const auto& p : vowels) if (!p.stressed) REQUIRE(!p.is_long);
    }
}

TEST_CASE("Collected homographs use nearby grammatical cues", "[frontend][accents][collected-oct11]") {
    struct Case { const char32_t* text; int word; int stress; Accent tone; bool long_vowel; };
    const auto F = Accent::Falling;
    const auto R = Accent::Rising;
    const Case cases[] = {
        {U"Tu sam", 0, 0, F, true}, {U"sa mnom", 1, 0, F, false},
        {U"Događaju se promjene", 0, 1, R, true},
        {U"Promjene se često događaju", 3, 1, R, true},
        {U"Promjene se uglavnom događaju", 3, 1, R, true},
        {U"o događaju", 1, 0, F, false},
        {U"o tom novom događaju se govori", 3, 0, F, false},
        {U"prema događaju", 1, 0, F, false},
        {U"tom događaju", 1, 0, F, false},
        {U"Oni osjećaju bol", 1, 0, R, false},
        {U"Potom osjećaju bol", 1, 0, R, false},
        {U"o neobičnom osjećaju", 2, 0, F, false},
        {U"Kiša pada", 1, 0, F, false},
        {U"nakon pada", 1, 0, F, true},
        {U"zbog naglog pada", 2, 0, F, true},
        {U"o padu", 1, 0, R, true},
        {U"o naglom padu", 2, 0, R, true},
        {U"u nebesa", 1, 1, R, false},
        {U"s nebesa", 1, 1, R, true},
        {U"iz visokih nebesa", 2, 1, R, true},
        {U"zgrada ministarstva", 1, 2, R, true},
        {U"dva ministarstva", 1, 2, R, true},
        {U"pet ministarstva", 1, 1, R, false},
        {U"nekoliko ministarstava", 1, 1, R, false},
        {U"drugih ministarstva", 1, 1, R, false},
        {U"citirani navodi", 1, 0, R, true},
        {U"grana stabla", 1, 0, R, false},
        {U"dva stabla", 1, 0, R, false},
        {U"pet stabla", 1, 0, R, true},
        {U"nekoliko stabala", 1, 1, R, true},
        {U"visokih stabla", 1, 0, R, true},
        {U"pet stàbla", 1, 0, R, false},
        {U"Stáblo", 0, 0, R, true},
    };
    const Frontend frontend(VoiceLanguage::Croatian);
    for (size_t i = 0; i < std::size(cases); ++i) {
        CAPTURE(i);
        const auto& c = cases[i];
        std::vector<Phone> vowels;
        for (const auto& p : read(frontend, c.text).phones)
            if (p.word == c.word && p.nucleus) vowels.push_back(p);
        REQUIRE(std::count_if(vowels.begin(), vowels.end(), [](const Phone& p) { return p.stressed; }) == 1);
        REQUIRE(vowels.at(c.stress).stressed);
        REQUIRE(vowels.at(c.stress).accent == c.tone);
        REQUIRE(vowels.at(c.stress).is_long == c.long_vowel);
    }
}

TEST_CASE("Collected accent corrections respect explicit and user choices", "[frontend][accents][collected-oct11]") {
    Frontend frontend(VoiceLanguage::Croatian);
    REQUIRE(nuclei(read(frontend, U"mnȏm"))[0].is_long);
    REQUIRE(!nuclei(read(frontend, U"tȕ"))[0].is_long);
    REQUIRE(nuclei(read(frontend, U"o dogáđaju"))[2].is_long);
    REQUIRE(!nuclei(read(frontend, U"s nebèsa"))[1].is_long);
    REQUIRE(nuclei(read(frontend, U"pet ministárstva"))[3].is_long);
    REQUIRE(nuclei(read(frontend, U"nakon pȁda"))[2].is_long == false);
    REQUIRE(nuclei(read(frontend, U"o pȃdu"))[1].accent == Accent::Falling);
    frontend.set_user_lexicon(UserLexicon::parse(
        R"({"entries":[{"word":"mn^o:m"},{"word":"dog/a:đaju"},{"word":"neb/esa"},{"word":"minist/a:rstva"},{"word":"p^ada"},{"word":"st/abla"}]})"));
    REQUIRE(nuclei(read(frontend, U"sa mnom"))[1].is_long);
    REQUIRE(nuclei(read(frontend, U"o događaju"))[2].is_long);
    REQUIRE(!nuclei(read(frontend, U"s nebesa"))[1].is_long);
    REQUIRE(nuclei(read(frontend, U"pet ministarstva"))[3].is_long);
    REQUIRE(!nuclei(read(frontend, U"nakon pada"))[2].is_long);
    REQUIRE(!nuclei(read(frontend, U"pet stabla"))[1].is_long);
    // The requested short mnom applies only to Croatian.
    REQUIRE(nuclei(read(Frontend(VoiceLanguage::Serbian), U"mnom"))[0].is_long);
    REQUIRE(nuclei(read(Frontend(VoiceLanguage::Bosnian), U"mnom"))[0].is_long);
}
