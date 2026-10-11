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
