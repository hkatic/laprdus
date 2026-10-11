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
