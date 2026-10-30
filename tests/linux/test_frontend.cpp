// -*- coding: utf-8 -*-
// Direct front-end tests for compound stress and word-boundary metadata.
// Build instructions are in CLAUDE.md and docs/formant.md.

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include "formant/formant_frontend.hpp"
#include "core/phoneme_mapper.hpp"
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

TEST_CASE("Kolica to Dragojevic keep sourced accents in their ordinary forms", "[frontend][accents][oct14]") {
    struct Paradigm {
        std::vector<const char32_t*> forms;
        int stress;
        Accent tone;
        bool long_vowel;
    };
    const auto R = Accent::Rising;
    const auto F = Accent::Falling;
    const Paradigm paradigms[] = {
        {{U"kolica", U"kolicima", U"Kolica", U"KOLICA"}, 1, R, true},
        {{U"dubinsku", U"dubinski", U"dubinska", U"dubinsko", U"dubinskoga",
          U"dubinskoj", U"dubinskih", U"dubinskima"}, 0, R, false},
        {{U"pogledati", U"pogledat", U"pogledam", U"pogledamo", U"pogledaju",
          U"pogledaj", U"pogledajte", U"pogledao", U"pogledala", U"pogledali",
          U"pogledah", U"pogledavši", U"pogledan"}, 0, R, false},
        {{U"doći", U"dođem", U"dođeš", U"dođe", U"dođemo", U"dođete", U"dođu"}, 0, F, true},
        {{U"dođi", U"dođimo", U"dođite", U"dođoh", U"dođosmo", U"dođoste", U"dođoše"}, 0, R, true},
        {{U"došao", U"došla", U"došlo", U"došli", U"došle", U"došavši"}, 0, R, false},
        {{U"tiskara", U"tiskare", U"tiskari", U"tiskaru", U"tiskarom", U"tiskarama"}, 0, F, false},
        {{U"podizanje", U"podizanja", U"podizanju", U"podizanjem", U"podizanjima",
          U"podizati", U"podižem", U"podižu", U"podiži", U"podizao", U"podizan"}, 0, R, false},
        {{U"element", U"elementa", U"elementu", U"elementom", U"elementi",
          U"elemenata", U"elementima"}, 1, R, false},
        {{U"stanje", U"stanja", U"stanju", U"stanjem", U"stanjima"}, 0, R, true},
        {{U"raspoloživ", U"raspoloživa", U"raspoloživo", U"raspoloživu",
          U"raspoloživi", U"raspoloživoga", U"raspoloživima"}, 2, R, false},
        {{U"raspoloživiji", U"raspoloživijima"}, 3, R, false},
        {{U"Oliver", U"Olivera", U"Oliveru", U"Oliverom"}, 0, R, false},
        {{U"Dragojević", U"Dragojevića", U"Dragojeviću", U"Dragojevićem"}, 0, F, false},
    };
    Frontend frontend(VoiceLanguage::Croatian);
    for (const auto& paradigm : paradigms) {
        for (const auto* text : paradigm.forms) {
            DYNAMIC_SECTION(PhonemeMapper::utf32_to_utf8(text)) {
                const auto utterance = read(frontend, text);
                REQUIRE((stressed_syllables(utterance) == std::vector<int>{paradigm.stress}));
                const auto vowels = nuclei(utterance);
                REQUIRE(vowels[paradigm.stress].accent == paradigm.tone);
                REQUIRE(vowels[paradigm.stress].is_long == paradigm.long_vowel);
                for (size_t i = 0; i < vowels.size(); ++i)
                    if (static_cast<int>(i) != paradigm.stress) REQUIRE(!vowels[i].is_long);
                require_one_word(utterance);
            }
        }
    }
    // A name must work both at clause head and after another word.
    for (const auto* text : {U"Oliver Dragojević", U"Olivera Dragojevića"}) {
        const auto utterance = read(frontend, text);
        const auto vowels = nuclei(utterance);
        const int surname = std::u32string(text) == U"Oliver Dragojević" ? 3 : 4;
        REQUIRE((stressed_syllables(utterance) == std::vector<int>{0, surname}));
        REQUIRE(vowels[0].accent == R);
        REQUIRE(vowels[surname].accent == F);
    }
    // Exact noun/adjective forms leave derivatives to their own rules.
    REQUIRE((stressed_syllables(read(frontend, U"elementaran")) == std::vector<int>{3}));
    REQUIRE((stressed_syllables(read(frontend, U"raspoložiti")) == std::vector<int>{2}));
    // Ambiguous aspects and surname variants remain explicitly selectable.
    REQUIRE((stressed_syllables(read(frontend, U"poglédati")) == std::vector<int>{1}));
    REQUIRE(nuclei(read(frontend, U"poglédati"))[1].is_long);
    REQUIRE((stressed_syllables(read(frontend, U"Dragójević")) == std::vector<int>{1}));
    frontend.set_user_lexicon(UserLexicon::parse(
        R"({"entries":[{"word":"drag/o:jević"},{"verb":"pogl/e:d=a"}]})"));
    REQUIRE((stressed_syllables(read(frontend, U"Dragojević")) == std::vector<int>{1}));
    REQUIRE((stressed_syllables(read(frontend, U"pogledati")) == std::vector<int>{1}));
}

TEST_CASE("Collected Croatian words retain sourced accents through inflection", "[frontend][accents][collected-words]") {
    struct Paradigm {
        std::vector<const char32_t*> forms;
        int stress;
        Accent tone;
        bool long_vowel;
    };
    const auto R = Accent::Rising;
    const auto F = Accent::Falling;
    const Paradigm paradigms[] = {
        {{U"ispuna", U"ispune", U"ispunu", U"ispuno", U"ispunom", U"ispunama",
          U"Ispuna", U"ISPUNA"}, 0, F, false},
        {{U"hrvat", U"Hrvat", U"HRVAT"}, 0, R, false},
        {{U"hrvata", U"Hrvata", U"HRVATA", U"Hrvatu", U"Hrvatom", U"Hrvati",
          U"Hrvate", U"Hrvatima"}, 1, R, true},
        {{U"vatrogasac", U"vatrogasca", U"vatrogascu", U"vatrogascem", U"vatrogasci",
          U"vatrogasce", U"vatrogascima", U"Vatrogasci", U"VATROGASCI"}, 2, R, true},
        {{U"vatrogasaca"}, 1, R, false},
        {{U"vatrogašče"}, 0, F, false},
        {{U"muškarac", U"muškarca", U"muškarcu", U"muškarcem", U"muškarci",
          U"muškarce", U"muškarcima", U"Muškarci", U"MUŠKARCI"}, 1, R, true},
        {{U"muškaraca"}, 0, R, false},
        {{U"muškarče"}, 0, F, false},
        {{U"policajac", U"policajca", U"policajcu", U"policajcem", U"policajci",
          U"policajce", U"policajcima", U"Policajci", U"POLICAJCI"}, 2, R, true},
        {{U"policajaca"}, 1, R, false},
        {{U"policajče"}, 0, F, false},
        {{U"nisam", U"nisi", U"nije", U"nismo", U"niste", U"nisu", U"Nisi", U"NISI"}, 0, R, true},
        // Listener's long i; case forms are an extension of that preference.
        {{U"kafanica", U"kafanice", U"kafanici", U"kafanicu", U"kafanico",
          U"kafanicom", U"kafanicama", U"Kafanica", U"KAFANICA"}, 2, R, true},
        {{U"laganica", U"laganice", U"laganici", U"laganicu", U"laganico",
          U"laganicom", U"laganicama", U"Laganica", U"LAGANICA"}, 2, R, true},
        // The noun must not swallow its verb's imperative or longer forms.
        {{U"ispuni", U"ispuniti", U"ispunim", U"ispunio", U"ispunila"}, 0, R, false},
        // Written accents still override dictionary and preferred readings.
        {{U"ispúna"}, 1, R, true},
        {{U"nȉsi"}, 0, F, false},
        {{U"kȁfanica", U"lȁganica"}, 0, F, false},
    };
    Frontend frontend(VoiceLanguage::Croatian);
    for (size_t i = 0; i < std::size(paradigms); ++i) {
        const auto& p = paradigms[i];
        for (size_t j = 0; j < p.forms.size(); ++j) {
            CAPTURE(i);
            CAPTURE(j);
            const auto utterance = read(frontend, p.forms[j]);
            require_one_word(utterance);
            REQUIRE(stressed_syllables(utterance) == std::vector<int>{p.stress});
            const auto vowels = nuclei(utterance);
            REQUIRE(vowels.at(p.stress).accent == p.tone);
            REQUIRE(vowels.at(p.stress).is_long == p.long_vowel);
            for (const auto& v : vowels) if (!v.stressed) REQUIRE_FALSE(v.is_long);
        }
    }
}

TEST_CASE("Collected accents survive phrases, homographs and user overrides", "[frontend][accents][collected-words]") {
    struct Case { const char32_t* text; int word; int stress; Accent tone; bool long_vowel; };
    const auto R = Accent::Rising;
    const auto F = Accent::Falling;
    const Case cases[] = {
        {U"zubna ispuna", 1, 0, F, false}, {U"u ispuni", 1, 0, F, false},
        {U"nema ispune", 1, 0, F, false}, {U"pet ispuna", 1, 0, F, false},
        {U"Ispuni obrazac", 0, 0, R, false}, {U"Molim ispuni obrazac", 1, 0, R, false},
        {U"jednog Hrvata", 1, 1, R, true}, {U"pet Hrvata", 1, 1, R, true},
        {U"hrabri vatrogasci", 1, 2, R, true}, {U"pet vatrogasaca", 1, 1, R, false},
        {U"ovi muškarci", 1, 1, R, true}, {U"pet muškaraca", 1, 0, R, false},
        {U"ovi policajci", 1, 2, R, true}, {U"pet policajaca", 1, 1, R, false},
        {U"ti nisi sam", 1, 0, R, true}, {U"zašto nisi došao", 1, 0, R, true},
        {U"kafanica laganica", 0, 2, R, true}, {U"kafanica laganica", 1, 2, R, true},
        {U"u kafanici", 1, 2, R, true}, {U"s laganicom", 1, 2, R, true},
    };
    Frontend frontend(VoiceLanguage::Croatian);
    for (size_t i = 0; i < std::size(cases); ++i) {
        CAPTURE(i);
        const auto& c = cases[i];
        std::vector<Phone> vowels;
        for (const auto& p : read(frontend, c.text).phones)
            if (p.nucleus && p.word == c.word) vowels.push_back(p);
        REQUIRE(std::count_if(vowels.begin(), vowels.end(),
                             [](const Phone& p) { return p.stressed; }) == 1);
        REQUIRE(vowels.at(c.stress).stressed);
        REQUIRE(vowels.at(c.stress).accent == c.tone);
        REQUIRE(vowels.at(c.stress).is_long == c.long_vowel);
    }
    // These shared verb forms must not fall through to the long -un rule
    // when the Croatian noun adds an imperative homograph.
    for (auto language : {VoiceLanguage::Serbian, VoiceLanguage::Bosnian}) {
        Frontend other(language);
        REQUIRE(stressed_syllables(read(other, U"oni ispune obrazac"))[1] == 2);
        REQUIRE(nuclei(read(other, U"ispuni"))[0].accent == R);
    }
    frontend.set_user_lexicon(UserLexicon::parse(
        R"({"entries":[{"word":"k^afanic*"},{"word":"n^isi"},{"word":"/ispuni"}]})"));
    REQUIRE(stressed_syllables(read(frontend, U"kafanica")) == std::vector<int>{0});
    REQUIRE_FALSE(nuclei(read(frontend, U"nisi"))[0].is_long);
    REQUIRE(nuclei(read(frontend, U"u ispuni"))[1].accent == R);
    REQUIRE(stressed_syllables(read(frontend, U"kafaníca")) == std::vector<int>{2});
}

TEST_CASE("Drinks and cevap keep sourced Croatian accents through inflection", "[frontend][accents][drinks-cevap]") {
    struct Paradigm {
        std::vector<const char32_t*> forms;
        int stress;
        Accent tone;
        bool long_vowel;
    };
    const auto R = Accent::Rising;
    const auto F = Accent::Falling;
    const Paradigm paradigms[] = {
        {{U"pivo", U"piva", U"pivu", U"pivom", U"pivima", U"pive", U"pivi", U"pivama",
          U"Pivo", U"PIVA"}, 0, F, true},
        {{U"vino", U"vina", U"vinu", U"vinom", U"vinima", U"Vino", U"VINA"}, 0, R, true},
        {{U"sok", U"Sok", U"SOK"}, 0, F, true},
        {{U"soka", U"soku", U"sokom", U"soče", U"sokovi", U"sokova", U"sokove",
          U"sokovima"}, 0, F, false},
        {{U"pelinkovac", U"pelinkovca", U"pelinkovcu", U"pelinkovcem", U"pelinkovci",
          U"pelinkovce", U"pelinkovcima", U"pelinkovaca", U"pelinkovče",
          U"Pelinkovac", U"PELINKOVAC"}, 0, R, false},
        {{U"medica", U"medice", U"medici", U"medicu", U"medico", U"medicom", U"medicama",
          U"Medica", U"MEDICA"}, 0, R, false},
        {{U"ćevap", U"Ćevap", U"ĆEVAP"}, 0, R, false},
        {{U"ćevapa", U"ćevapu", U"ćevapom", U"ćevapi", U"ćevape", U"ćevapima",
          U"Ćevapi", U"ĆEVAPI"}, 1, R, true},
        {{U"ćevapčić", U"ćevapčića", U"ćevapčiću", U"ćevapčićem", U"ćevapčići",
          U"ćevapčiće", U"ćevapčićima", U"Ćevapčići", U"ĆEVAPČIĆI"}, 1, R, false},
        // Nearby words must not inherit these noun accents.
        {{U"medicina", U"medicine"}, 2, R, true},
        {{U"medicinska"}, 1, R, false},
        {{U"sokak"}, 0, R, false},
        {{U"sokaka", U"sokacima"}, 1, R, true},
        // Explicit accents still take priority.
        {{U"pìvo", U"vìno", U"sòkovi"}, 0, R, false},
        {{U"ćȅvap"}, 0, F, false},
    };
    Frontend frontend(VoiceLanguage::Croatian);
    for (size_t i = 0; i < std::size(paradigms); ++i) {
        const auto& paradigm = paradigms[i];
        for (size_t j = 0; j < paradigm.forms.size(); ++j) {
            CAPTURE(i);
            CAPTURE(j);
            const auto utterance = read(frontend, paradigm.forms[j]);
            require_one_word(utterance);
            REQUIRE(stressed_syllables(utterance) == std::vector<int>{paradigm.stress});
            const auto vowels = nuclei(utterance);
            REQUIRE(vowels.at(paradigm.stress).accent == paradigm.tone);
            REQUIRE(vowels.at(paradigm.stress).is_long == paradigm.long_vowel);
            for (const auto& phone : vowels) if (!phone.stressed) REQUIRE_FALSE(phone.is_long);
        }
    }
}

TEST_CASE("Drink and food accents survive phrases and respect user overrides", "[frontend][accents][drinks-cevap]") {
    struct Case { const char32_t* text; int word; int stress; Accent tone; bool long_vowel; };
    const auto R = Accent::Rising;
    const auto F = Accent::Falling;
    const Case cases[] = {
        {U"čaša piva", 1, 0, F, true}, {U"dvije pive", 1, 0, F, true},
        {U"čaša vina", 1, 0, R, true}, {U"tri vina", 1, 0, R, true},
        {U"pijem sok", 1, 0, F, true}, {U"čaša soka", 1, 0, F, false},
        {U"pet sokova", 1, 0, F, false}, {U"o sokovima", 1, 0, F, false},
        {U"čaša pelinkovca", 1, 0, R, false}, {U"s medicom", 1, 0, R, false},
        {U"jedan ćevap", 1, 0, R, false}, {U"pet ćevapa", 1, 1, R, true},
        {U"s ćevapima", 1, 1, R, true}, {U"pet ćevapčića", 1, 1, R, false},
    };
    Frontend frontend(VoiceLanguage::Croatian);
    for (size_t i = 0; i < std::size(cases); ++i) {
        CAPTURE(i);
        const auto& c = cases[i];
        std::vector<Phone> vowels;
        for (const auto& p : read(frontend, c.text).phones)
            if (p.nucleus && p.word == c.word) vowels.push_back(p);
        REQUIRE(std::count_if(vowels.begin(), vowels.end(),
                             [](const Phone& p) { return p.stressed; }) == 1);
        REQUIRE(vowels.at(c.stress).stressed);
        REQUIRE(vowels.at(c.stress).accent == c.tone);
        REQUIRE(vowels.at(c.stress).is_long == c.long_vowel);
    }
    frontend.set_user_lexicon(UserLexicon::parse(
        R"({"entries":[{"word":"p/iv*"},{"word":"ć^evap"}]})"));
    REQUIRE(nuclei(read(frontend, U"piva"))[0].accent == R);
    REQUIRE_FALSE(nuclei(read(frontend, U"piva"))[0].is_long);
    REQUIRE(nuclei(read(frontend, U"ćevap"))[0].accent == F);
    REQUIRE(nuclei(read(frontend, U"pȋva"))[0].accent == F);
    REQUIRE(nuclei(read(frontend, U"pȋva"))[0].is_long);
}

TEST_CASE("Vowels at word boundaries get a separation cue", "[frontend][prosody]") {
    Frontend frontend(VoiceLanguage::Croatian);
    auto boundaries = [&](const std::u32string& text) {
        const auto utterance = read(frontend, text);
        size_t count = 0;
        for (size_t i = 1; i < utterance.phones.size(); ++i) {
            if (needs_vowel_separation(utterance.phones[i - 1], utterance.phones[i])) ++count;
        }
        return count;
    };
    REQUIRE(boundaries(U"video uradak") == 1);
    REQUIRE(boundaries(U"moju ulicu") == 1);
    REQUIRE(boundaries(U"vidio ih") == 1);  // also before an unstressed clitic
    REQUIRE(boundaries(U"ja i ona") == 2);
    REQUIRE(boundaries(U"njoj i njenom") == 1);  // existing /j/ cue
    for (char32_t left : std::u32string(U"aeiou")) {
        for (char32_t right : std::u32string(U"aeiou")) {
            REQUIRE(boundaries(std::u32string(U"man") + left + U" " + right + U"lana") == 1);
            REQUIRE(boundaries(std::u32string(U"man") + left + right + U"lana") == 0);
        }
    }
    REQUIRE(boundaries(U"video") == 0);
    REQUIRE(boundaries(U"moju kuću") == 0);
    REQUIRE(boundaries(U"dobar uradak") == 0);
    REQUIRE(boundaries(U"moja jabuka") == 0);
    REQUIRE(boundaries(U"tifloasistent") == 0);  // joined compound components
    REQUIRE(boundaries(U"krv ulazi") == 0);     // syllabic r is not vowel hiatus
}

TEST_CASE("Svetac and hope verbs retain sourced accents across their paradigms", "[frontend][accents][svetac-nadati]") {
    struct Case { const char32_t* text; int stress; Accent tone; bool long_vowel; };
    const auto R = Accent::Rising;
    const auto F = Accent::Falling;
    const Case cases[] = {
        {U"svetac", 0, R, true}, {U"Svetac", 0, R, true}, {U"SVETAC", 0, R, true},
        {U"sveci", 0, R, true}, {U"sveca", 0, R, true}, {U"svecu", 0, R, true},
        {U"svecem", 0, R, true}, {U"svece", 0, R, true}, {U"svecima", 0, R, true},
        {U"svetci", 0, R, true}, {U"svetca", 0, R, true}, {U"svetcu", 0, R, true},
        {U"svetcem", 0, R, true}, {U"svetce", 0, R, true}, {U"svetcima", 0, R, true},
        {U"sveče", 0, F, true}, {U"svetče", 0, F, true}, {U"svetaca", 0, F, true},
        {U"nadati", 0, R, true}, {U"Nadati", 0, R, true}, {U"NADATI", 0, R, true},
        {U"nadat", 0, R, true}, {U"nadao", 0, R, true}, {U"nadala", 0, R, true},
        {U"nadalo", 0, R, true}, {U"nadali", 0, R, true}, {U"nadale", 0, R, true},
        {U"nadaju", 0, R, true}, {U"nadajući", 0, R, true}, {U"nadah", 0, R, true},
        {U"nadam", 0, F, true}, {U"nadaš", 0, F, true}, {U"nadamo", 0, F, true},
        {U"nadate", 0, F, true}, {U"nadaj", 0, F, true}, {U"nadajmo", 0, F, true},
        {U"nadajte", 0, F, true}, {U"nada", 0, R, true}, {U"nade", 0, R, true},
        {U"nadi", 0, R, true}, {U"nadu", 0, R, true}, {U"nadom", 0, R, true},
        {U"nadama", 0, R, true},
        {U"ponadati", 1, R, true}, {U"Ponadati", 1, R, true},
        {U"ponadat", 1, R, true}, {U"ponadao", 1, R, true}, {U"ponadala", 1, R, true},
        {U"ponadalo", 1, R, true}, {U"ponadali", 1, R, true}, {U"ponadale", 1, R, true},
        {U"PONADALI", 1, R, true}, {U"ponadaju", 1, R, true}, {U"ponadavši", 1, R, true},
        {U"ponadah", 1, R, true}, {U"ponadasmo", 1, R, true}, {U"ponadaste", 1, R, true},
        {U"ponadam", 0, R, false}, {U"ponadaš", 0, R, false}, {U"ponada", 0, R, false},
        {U"ponadamo", 0, R, false}, {U"ponadate", 0, R, false},
        {U"ponadaj", 0, R, false}, {U"ponadajmo", 0, R, false}, {U"ponadajte", 0, R, false},
        {U"svȅtac", 0, F, false}, {U"nàdati", 0, R, false}, {U"pònadali", 0, R, false},
    };
    Frontend frontend(VoiceLanguage::Croatian);
    for (size_t i = 0; i < std::size(cases); ++i) {
        CAPTURE(i);
        const auto& c = cases[i];
        const auto utterance = read(frontend, c.text);
        require_one_word(utterance);
        REQUIRE(stressed_syllables(utterance) == std::vector<int>{c.stress});
        const auto vowels = nuclei(utterance);
        REQUIRE(vowels.at(c.stress).accent == c.tone);
        REQUIRE(vowels.at(c.stress).is_long == c.long_vowel);
        for (const auto& p : vowels) if (!p.stressed) REQUIRE_FALSE(p.is_long);
    }
}

TEST_CASE("Nada distinguishes the noun from the reflexive verb and respects overrides", "[frontend][accents][svetac-nadati]") {
    Frontend frontend(VoiceLanguage::Croatian);
    auto word_accent = [](const Frontend& reader, const char32_t* text, int word) {
        for (const auto& p : read(reader, text).phones)
            if (p.word == word && p.stressed) return p.accent;
        return Accent::None;
    };
    REQUIRE(word_accent(frontend, U"nada se", 0) == Accent::Falling);
    REQUIRE(word_accent(frontend, U"Nada se uspjehu", 0) == Accent::Falling);
    REQUIRE(word_accent(frontend, U"on se nada", 2) == Accent::Falling);
    REQUIRE(word_accent(frontend, U"moja nada", 1) == Accent::Rising);
    REQUIRE(word_accent(frontend, U"Nada se nadala", 0) == Accent::Rising);
    REQUIRE(word_accent(frontend, U"nada se pojavila", 0) == Accent::Rising);
    REQUIRE(word_accent(frontend, U"náda se", 0) == Accent::Rising);
    frontend.set_user_lexicon(UserLexicon::parse(
        R"({"entries":[{"word":"n/a:da"},{"word":"sv^etac"},{"verb":"p^onad=a"}]})"));
    REQUIRE(word_accent(frontend, U"nada se", 0) == Accent::Rising);
    REQUIRE_FALSE(nuclei(read(frontend, U"svetac"))[0].is_long);
    REQUIRE(stressed_syllables(read(frontend, U"ponadali")) == std::vector<int>{0});
}

TEST_CASE("Ostarjeti forms keep initial rising stress and a short a", "[frontend][accents][ostarjeti]") {
    Frontend frontend(VoiceLanguage::Croatian);
    // ŠR explicitly lists òstarīm, òstarē, òstari, òstarjeh, òstario,
    // òstarjela/òstarjelo/òstarjeli. Check the remaining regular forms too.
    const char32_t* const forms[] = {
             U"ostare", U"Ostare", U"OSTARE", U"ostarim", U"ostariš", U"ostari",
             U"ostarimo", U"ostarite", U"ostarjeti", U"ostarjet", U"ostariti",
             U"ostarit", U"ostario", U"ostarjela", U"ostarjelo", U"ostarjeli",
             U"ostarjele", U"ostarila", U"ostarilo", U"ostarili", U"ostarile",
             U"ostarjeh", U"ostarje", U"ostarjesmo", U"ostarjeste", U"ostarješe",
             U"ostarjevši", U"ostarih", U"ostarismo", U"ostariste", U"ostariše",
             U"ostarivši"};
    for (size_t i = 0; i < std::size(forms); ++i) {
        const auto utterance = read(frontend, forms[i]);
        CAPTURE(i);
        require_one_word(utterance);
        REQUIRE(stressed_syllables(utterance) == std::vector<int>{0});
        const auto vowels = nuclei(utterance);
        REQUIRE(vowels.size() >= 3);
        REQUIRE(vowels[0].accent == Accent::Rising);
        REQUIRE_FALSE(vowels[0].is_long);
        REQUIRE(vowels[1].ph == Ph::A);
        REQUIRE_FALSE(vowels[1].stressed);
        REQUIRE_FALSE(vowels[1].is_long);
    }
    // The correction also applies away from clause head and keeps explicit
    // user-supplied accents authoritative.
    const auto phrase = nuclei(read(frontend, U"oni ostare"));
    REQUIRE(phrase[2].stressed);
    REQUIRE(phrase[2].accent == Accent::Rising);
    REQUIRE_FALSE(phrase[3].stressed);
    REQUIRE_FALSE(phrase[3].is_long);
    const auto explicit_accent = nuclei(read(frontend, U"ostáre"));
    REQUIRE(explicit_accent[1].stressed);
    REQUIRE(explicit_accent[1].is_long);
}

TEST_CASE("Collected Croatian accents retain quantity tone and inflection", "[frontend][accents][collected-oct12]") {
    // Školski rječnik principal forms; source choices and inferred
    // inflections are documented in docs/formant.md (12 October 2026).
    struct Case { const char32_t* text; int syllables; int stress; Accent tone; bool long_vowel; };
    const auto F = Accent::Falling;
    const auto R = Accent::Rising;
    const Case cases[] = {
        {U"razlikujemo", 5, 0, R, true}, {U"Razlikujemo", 5, 0, R, true},
        {U"razlikujem", 4, 0, R, true}, {U"razlikuješ", 4, 0, R, true},
        {U"razlikuje", 4, 0, R, true}, {U"razlikujete", 5, 0, R, true},
        {U"razlikuju", 4, 0, R, true}, {U"razlikujući", 5, 0, R, true},
        {U"razlikuj", 3, 0, R, true}, {U"razlikujmo", 4, 0, R, true},
        {U"razlikujte", 4, 0, R, true}, {U"razlikovati", 5, 0, R, true},
        {U"razlikovao", 5, 0, R, true}, {U"razlikovala", 5, 0, R, true},
        {U"razlikovali", 5, 0, R, true}, {U"razlikovan", 4, 0, R, true},
        {U"razlikovanima", 6, 0, R, true}, {U"razlikovanje", 5, 0, R, true},
        {U"apsurdni", 3, 0, R, false}, {U"Apsurdni", 3, 0, R, false},
        {U"apsurdan", 3, 0, R, false}, {U"apsurdna", 3, 0, R, false},
        {U"apsurdno", 3, 0, R, false}, {U"apsurdne", 3, 0, R, false},
        {U"apsurdnu", 3, 0, R, false}, {U"apsurdnog", 3, 0, R, false},
        {U"apsurdnoga", 4, 0, R, false}, {U"apsurdnoj", 3, 0, R, false},
        {U"apsurdnome", 4, 0, R, false}, {U"apsurdnima", 4, 0, R, false},
        {U"apsurdniji", 4, 1, R, false}, {U"apsurdnijima", 5, 1, R, false},
        {U"najapsurdniji", 5, 2, R, false},
        {U"društvo", 2, 0, R, true}, {U"Društvo", 2, 0, R, true},
        {U"društva", 2, 0, R, true}, {U"društvu", 2, 0, R, true},
        {U"društvom", 2, 0, R, true}, {U"društvima", 3, 0, R, true},
        {U"društava", 3, 0, F, true}, {U"drùštvo", 2, 0, R, false},
        {U"VIŠEDNEVNI", 4, 0, F, false}, {U"višèdnevni", 4, 1, R, false},
        {U"višemjesečni", 5, 1, R, false}, {U"petodnevni", 4, 1, R, false},
        {U"višednevnog", 4, 0, F, false}, {U"Višednevnog", 4, 0, F, false},
        {U"višednevni", 4, 0, F, false}, {U"višednevna", 4, 0, F, false},
        {U"višednevno", 4, 0, F, false}, {U"višednevnoga", 5, 0, F, false},
        {U"višednevnoj", 4, 0, F, false}, {U"višednevnima", 5, 0, F, false},
        {U"sposobnosti", 4, 1, R, true}, {U"Sposobnosti", 4, 1, R, true},
        {U"sposobnost", 3, 1, R, true}, {U"sposobnošću", 4, 1, R, true},
        {U"sposobnostima", 5, 1, R, true},
        // Listener's initial stress; written accents retain the ŠR/HJP form.
        {U"pojedine", 4, 0, F, false}, {U"Pojedine", 4, 0, F, false},
        {U"pojedini", 4, 0, F, false}, {U"pojedina", 4, 0, F, false},
        {U"pojedino", 4, 0, F, false}, {U"pojedinu", 4, 0, F, false},
        {U"pojedinoga", 5, 0, F, false}, {U"pojedinomu", 5, 0, F, false},
        {U"pojedinoj", 4, 0, F, false}, {U"pojedinih", 4, 0, F, false},
        {U"pojedinima", 5, 0, F, false}, {U"POJEDINI", 4, 0, F, false},
        {U"pojèdini", 4, 1, R, false},
        {U"pojedinac", 4, 2, R, false}, {U"pojedinca", 4, 2, R, false},
        {U"vještinu", 3, 1, R, false}, {U"Vještinu", 3, 1, R, false},
        {U"vještina", 3, 1, R, false}, {U"vještine", 3, 1, R, false},
        {U"vještini", 3, 1, R, false}, {U"vještinom", 3, 1, R, false},
        {U"vještinama", 4, 1, R, false},
        {U"prvenstveno", 4, 1, R, false}, {U"Prvenstveno", 4, 1, R, false},
        {U"jugoslavija", 5, 1, R, false}, {U"Jugoslavija", 5, 1, R, false},
        {U"JUGOSLAVIJA", 5, 1, R, false}, {U"Jugoslavije", 5, 1, R, false},
        {U"Jugoslaviji", 5, 1, R, false}, {U"Jugoslaviju", 5, 1, R, false},
        {U"Jugoslavijo", 5, 1, R, false}, {U"Jugoslavijom", 5, 1, R, false},
        {U"jugoslavijom", 5, 1, R, false},
        {U"žal", 1, 0, F, true}, {U"Žal", 1, 0, F, true},
        {U"ŽAL", 1, 0, F, true},
        {U"žala", 2, 0, F, true}, {U"žalu", 2, 0, F, true},
        {U"žalom", 2, 0, F, true}, // default regret reading
        {U"žála", 2, 0, R, true}, {U"žálu", 2, 0, R, true},
        {U"žálom", 2, 0, R, true}, // explicitly supplied shore reading
        {U"žalovi", 3, 0, R, true}, {U"žalova", 3, 0, R, true},
        {U"žalove", 3, 0, R, true}, {U"žalovima", 4, 0, R, true},
        {U"ulaganje", 4, 1, R, true}, {U"Ulaganje", 4, 1, R, true},
        {U"ulaganja", 4, 1, R, true}, {U"ulaganju", 4, 1, R, true},
        {U"ulaganjem", 4, 1, R, true}, {U"ulaganjima", 5, 1, R, true},
        {U"vrag", 1, 0, F, true}, {U"Vrag", 1, 0, F, true},
        {U"vraga", 2, 0, F, true}, {U"vragu", 2, 0, F, true},
        {U"vraže", 2, 0, F, true}, {U"vragom", 2, 0, F, true},
        {U"vragovi", 3, 0, F, false}, {U"vragova", 3, 0, F, false},
        {U"vragove", 3, 0, F, false}, {U"vragovima", 4, 0, F, false},
        {U"vrazi", 2, 0, F, true},
        {U"mrak", 1, 0, F, true}, {U"Mrak", 1, 0, F, true},
        {U"mraka", 2, 0, F, true}, {U"mraku", 2, 0, F, true},
        {U"mrakom", 2, 0, F, true},
        {U"mrakovi", 3, 0, F, false}, {U"mrakova", 3, 0, F, false},
        {U"mrakove", 3, 0, F, false}, {U"mrakovima", 4, 0, F, false},
        {U"mraci", 2, 0, F, true},
        {U"zrak", 1, 0, F, true}, {U"Zrak", 1, 0, F, true},
        {U"zraka", 2, 0, F, true}, {U"zraku", 2, 0, F, true},
        {U"zrakom", 2, 0, F, true},
        {U"zrake", 2, 0, F, false}, {U"zraci", 2, 0, F, false},
        {U"zrako", 2, 0, F, false}, {U"zrakama", 3, 0, F, false},
        {U"rastegnuti", 4, 1, R, true}, {U"rastegni", 3, 1, R, true},
        {U"rastegnimo", 4, 1, R, true}, {U"rastegnite", 4, 1, R, true},
        {U"rastegnuo", 4, 1, R, true}, {U"rastegnula", 4, 1, R, true},
        {U"rastegnuli", 4, 1, R, true}, {U"rastegnuh", 3, 1, R, true},
        {U"rastegnuvši", 4, 1, R, true},
        {U"rastegnem", 3, 0, R, false}, {U"rastegneš", 3, 0, R, false},
        {U"rastegne", 3, 0, R, false}, {U"rastegnemo", 4, 0, R, false},
        {U"rastegnete", 4, 0, R, false}, {U"rastegnu", 3, 0, R, false},
        {U"rastegnut", 3, 0, R, false}, {U"rastegnuta", 4, 0, R, false},
        {U"rastegnutima", 5, 0, R, false},
        {U"protegnuti", 4, 1, R, true}, {U"protegni", 3, 1, R, true},
        {U"protegnimo", 4, 1, R, true}, {U"protegnite", 4, 1, R, true},
        {U"protegnuo", 4, 1, R, true}, {U"protegnula", 4, 1, R, true},
        {U"protegnuli", 4, 1, R, true}, {U"protegnuh", 3, 1, R, true},
        {U"protegnuvši", 4, 1, R, true},
        {U"protegnem", 3, 0, R, false}, {U"protegneš", 3, 0, R, false},
        {U"protegne", 3, 0, R, false}, {U"protegnemo", 4, 0, R, false},
        {U"protegnete", 4, 0, R, false}, {U"protegnu", 3, 0, R, false},
        {U"protegnut", 3, 0, R, false}, {U"protegnuta", 4, 0, R, false},
        {U"protegnutima", 5, 0, R, false},
        {U"pošiljatelj", 4, 1, R, false}, {U"Pošiljatelj", 4, 1, R, false},
        {U"pošiljatelja", 5, 1, R, false}, {U"pošiljatelju", 5, 1, R, false},
        {U"pošiljateljem", 5, 1, R, false}, {U"pošiljatelji", 5, 1, R, false},
        {U"pošiljatelje", 5, 1, R, false}, {U"pošiljateljima", 6, 1, R, false},
        {U"zvučna", 2, 0, R, true}, {U"zvučan", 2, 0, F, true},
        {U"zvučno", 2, 0, F, true}, {U"zvučni", 2, 0, F, true},
        {U"zvučnoga", 3, 0, F, true}, {U"zvučnima", 3, 0, F, true},
        {U"zvučniji", 3, 0, R, false}, {U"zvučnijima", 4, 0, R, false},
        {U"zvȗčna", 2, 0, F, true}, // explicitly requested definite form
        {U"anketa", 3, 1, R, false}, {U"Anketa", 3, 1, R, false},
        {U"ankete", 3, 1, R, false}, {U"anketi", 3, 1, R, false},
        {U"anketu", 3, 1, R, false}, {U"anketom", 3, 1, R, false},
        {U"anketama", 4, 1, R, false},
        {U"zategnuti", 4, 1, R, true}, {U"zategni", 3, 1, R, true},
        {U"zategnite", 4, 1, R, true}, {U"zategnimo", 4, 1, R, true},
        {U"zategnuo", 4, 1, R, true}, {U"zategnula", 4, 1, R, true},
        {U"zategnuli", 4, 1, R, true}, {U"zategnuh", 3, 1, R, true},
        {U"zategnuvši", 4, 1, R, true},
        {U"zategnem", 3, 0, R, false}, {U"zategneš", 3, 0, R, false},
        {U"zategne", 3, 0, R, false}, {U"zategnemo", 4, 0, R, false},
        {U"zategnete", 4, 0, R, false}, {U"zategnu", 3, 0, R, false},
        {U"zategnut", 3, 0, R, false}, {U"zategnuta", 4, 0, R, false},
        {U"zategnutim", 4, 0, R, false}, {U"zategnutima", 5, 0, R, false},
        {U"zategao", 4, 0, R, false}, {U"zategla", 3, 0, R, false},
        {U"zategli", 3, 0, R, false},
        {U"kriva", 2, 0, R, true}, {U"kriv", 1, 0, F, true},
        {U"krivo", 2, 0, F, true}, {U"krivi", 2, 0, F, true},
        {U"krivoga", 3, 0, F, true}, {U"krivima", 3, 0, F, true},
        {U"krivlji", 2, 0, F, false}, {U"krivljima", 3, 0, F, false},
        {U"krȋva", 2, 0, F, true},
        {U"Drina", 2, 0, R, true}, {U"Drine", 2, 0, R, true},
        {U"Drini", 2, 0, R, true}, {U"Drinu", 2, 0, R, true},
        {U"Drinom", 2, 0, R, true},
        {U"kriminalac", 4, 2, R, true}, {U"kriminalca", 4, 2, R, true},
        {U"kriminalcu", 4, 2, R, true}, {U"kriminalcem", 4, 2, R, true},
        {U"kriminalci", 4, 2, R, true}, {U"kriminalce", 4, 2, R, true},
        {U"kriminalcima", 5, 2, R, true}, {U"kriminalče", 4, 0, F, false},
        {U"kriminalaca", 5, 1, R, false},
        {U"dosije", 3, 1, R, false}, {U"Dosije", 3, 1, R, false},
        {U"dosijea", 4, 2, R, false}, {U"dosijeu", 4, 2, R, false},
        {U"dosijeom", 4, 2, R, false}, {U"dosijei", 4, 2, R, false},
        {U"dosijee", 4, 2, R, false}, {U"dosijeima", 5, 2, R, false},
        {U"očajnički", 4, 0, F, false}, {U"očajnička", 4, 0, F, false},
        {U"očajničkog", 4, 0, F, false}, {U"očajničkima", 5, 0, F, false},
        {U"ispraviti", 4, 0, R, false}, {U"ispravim", 3, 0, R, false},
        {U"isprave", 3, 0, R, false}, {U"ispravite", 4, 0, R, false},
        {U"ispravio", 4, 0, R, false}, {U"ispravila", 4, 0, R, false},
        {U"ispravljen", 3, 0, R, false}, {U"ispravljenima", 5, 0, R, false},
        {U"budućnost", 3, 1, R, true}, {U"budućnosti", 4, 1, R, true},
        {U"budućnošću", 4, 1, R, true}, {U"budućnostima", 5, 1, R, true},
        {U"razočaranja", 5, 3, R, true}, {U"razočaranje", 5, 3, R, true},
        {U"razočaranju", 5, 3, R, true}, {U"razočaranjem", 5, 3, R, true},
        {U"razočaranjima", 6, 3, R, true},
        {U"pomislio", 4, 0, R, false}, {U"pomislila", 4, 0, R, false},
        {U"pomislili", 4, 0, R, false}, {U"pomisliti", 4, 0, R, false},
        {U"pomislim", 3, 0, R, false}, {U"pomislite", 4, 0, R, false},
        {U"postoji", 3, 0, R, true}, {U"postojati", 4, 0, R, true},
        {U"postojim", 3, 0, R, true}, {U"postojimo", 4, 0, R, true},
        {U"postojite", 4, 0, R, true}, {U"postoje", 3, 0, R, true},
        {U"postoj", 2, 0, R, true}, {U"postojte", 3, 0, R, true},
        {U"postojao", 4, 0, R, true}, {U"postojali", 4, 0, R, true},
        {U"postojeći", 4, 0, R, true}, {U"postojanje", 4, 0, R, true},
        {U"viri", 2, 0, R, true}, {U"viriti", 3, 0, R, true},
        {U"virim", 2, 0, R, true}, {U"vire", 2, 0, R, true},
        {U"virite", 3, 0, R, true}, {U"virio", 3, 0, R, true},
        {U"vireći", 3, 0, R, true}, {U"virenje", 3, 0, R, true},
        {U"vir", 1, 0, F, true}, {U"vira", 2, 0, F, true},
        {U"viru", 2, 0, F, true}, {U"virom", 2, 0, F, true},
        {U"virovi", 3, 0, F, false}, {U"virovima", 4, 0, F, false},
        {U"sunce", 2, 0, F, true}, {U"Sunce", 2, 0, F, true},
        {U"sunca", 2, 0, F, true}, {U"suncu", 2, 0, F, true},
        {U"suncem", 2, 0, F, true}, {U"suncima", 3, 0, F, true},
        {U"sunaca", 3, 0, F, false},
        {U"noć", 1, 0, F, true}, {U"noći", 2, 0, F, false},
        {U"noću", 2, 0, F, false}, {U"noćima", 3, 0, R, false},
        {U"trud", 1, 0, F, true}, {U"truda", 2, 0, R, true},
        {U"trudu", 2, 0, R, true}, {U"trudom", 2, 0, R, true},
        {U"trudovi", 3, 0, R, true}, {U"trudova", 3, 0, R, true},
        {U"trudovima", 4, 0, R, true}, {U"trudima", 3, 0, R, true},
        {U"povucite", 4, 1, R, true}, {U"povuci", 3, 1, R, true},
        {U"povucimo", 4, 1, R, true}, {U"povući", 3, 0, R, false},
        {U"povukao", 4, 0, R, false}, {U"povukli", 3, 0, R, false},
        {U"povukavši", 4, 0, R, false}, {U"povučem", 3, 1, R, true},
        {U"povučete", 4, 1, R, true}, {U"povuku", 3, 1, R, true},
        {U"povukoh", 3, 1, R, true}, {U"povučen", 3, 1, R, false},
        {U"povučena", 4, 1, R, false}, {U"povučenima", 5, 1, R, false},
    };
    const Frontend frontend(VoiceLanguage::Croatian);
    for (size_t i = 0; i < std::size(cases); ++i) {
        DYNAMIC_SECTION("dictionary form " + std::to_string(i)) {
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
    // Noun corrections must not lengthen homographic verbal forms.
    REQUIRE_FALSE(nuclei(read(frontend, U"žali"))[0].is_long);
    REQUIRE_FALSE(nuclei(read(frontend, U"žale"))[0].is_long);
}

TEST_CASE("Vir and noc case cues preserve length and user accents", "[frontend][accents][collected-oct12]") {
    struct Case { const char32_t* text; int word; Accent tone; bool long_vowel; };
    const auto F = Accent::Falling;
    const auto R = Accent::Rising;
    const Case cases[] = {
        {U"u viru", 1, R, true}, {U"o dubokom viru", 2, R, true},
        {U"prema viru", 1, F, true}, {U"iz vira", 1, F, true},
        {U"dvije noći", 1, F, false}, {U"tijekom noći", 1, F, false},
        {U"pet noći", 1, R, false}, {U"nekoliko noći", 1, R, false},
        {U"pet dugih noći", 2, R, false}, {U"u noći", 1, R, false},
        {U"o toj dugoj noći", 3, R, false}, {U"prema noći", 1, F, false},
        {U"u vȋru", 1, F, true}, {U"pet nȍći", 1, F, false},
        {U"o nȏći", 1, F, true},
    };
    const Frontend frontend(VoiceLanguage::Croatian);
    for (size_t i = 0; i < std::size(cases); ++i) {
        DYNAMIC_SECTION("phrase " + std::to_string(i)) {
            const auto& c = cases[i];
            std::vector<Phone> vowels;
            for (const auto& p : read(frontend, c.text).phones)
                if (p.word == c.word && p.nucleus) vowels.push_back(p);
            REQUIRE(vowels.at(0).stressed);
            REQUIRE(vowels.at(0).accent == c.tone);
            REQUIRE(vowels.at(0).is_long == c.long_vowel);
        }
    }
    Frontend custom(VoiceLanguage::Croatian);
    custom.set_user_lexicon(UserLexicon::parse(
        R"({"entries":[{"word":"v^i:ru"},{"word":"n^o:ći"},{"word":"p^ostoji"},{"verb":"pov/uc=e"}]})"));
    REQUIRE(nuclei(read(custom, U"u viru"))[1].accent == F);
    REQUIRE(nuclei(read(custom, U"pet noći"))[1].accent == F);
    REQUIRE(nuclei(read(custom, U"pet noći"))[1].is_long);
    REQUIRE(nuclei(read(custom, U"postoji"))[0].accent == F);
    REQUIRE_FALSE(nuclei(read(custom, U"povucite"))[1].is_long);
    REQUIRE(nuclei(read(frontend, U"pòvucite"))[0].stressed);
    Frontend stem(VoiceLanguage::Croatian);
    stem.set_user_lexicon(UserLexicon::parse(
        R"({"entries":[{"word":"n^o:ć*"},{"word":"p^ovuc*"}]})"));
    REQUIRE(nuclei(read(stem, U"pet noći"))[1].accent == F);
    REQUIRE(nuclei(read(stem, U"pet noći"))[1].is_long);
    REQUIRE(nuclei(read(stem, U"povucite"))[0].stressed);
    REQUIRE(nuclei(read(stem, U"povúcite"))[1].stressed);
    // Within user entries, exact words beat stems, which beat verbs.
    stem.set_user_lexicon(UserLexicon::parse(
        R"({"entries":[{"word":"pov/u:ci"},{"word":"p^ovuc*"},{"verb":"pov/uc=e"}]})"));
    REQUIRE(nuclei(read(stem, U"povuci"))[1].is_long);
    REQUIRE(nuclei(read(stem, U"povucite"))[0].accent == F);
    // Existing shared nȏć remains long in the other voices as well.
    for (auto language : {VoiceLanguage::Serbian, VoiceLanguage::Bosnian}) {
        REQUIRE(nuclei(read(Frontend(language), U"noć"))[0].is_long);
    }
}

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
        {U"špansko", 2, 0, R, true}, {U"društvo", 2, 0, R, true},
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
        {U"o vragu", 0, R, true}, {U"o crnom vragu", 0, R, true},
        {U"k vragu", 0, F, true}, {U"tri vraga", 0, F, true},
        {U"pet vraga", 0, R, true}, {U"pet crnih vraga", 0, R, true},
        {U"u mraku", 0, R, true}, {U"po potpunom mraku", 0, R, true},
        {U"prema mraku", 0, F, true}, {U"iz mraka", 0, F, true},
        {U"na zraku", 0, R, true}, {U"na svježem jutarnjem zraku", 0, R, true},
        {U"prema zraku", 0, F, true}, {U"bez zraka", 0, F, true},
        {U"nema zraka", 0, F, true}, {U"svježim zrakom", 0, F, true},
        {U"jedna zraka", 0, F, false}, {U"sunčeva zraka", 0, F, false},
        {U"ovu zraku", 0, F, false}, {U"na lasersku zraku", 0, F, false},
        {U"tom zrakom", 0, F, false}, {U"laserskom zrakom", 0, F, false},
        {U"u toj zraci", 0, F, false}, {U"pet sunčevih zraka", 0, F, true},
        {U"o vrȃgu", 0, F, true}, {U"pet vrȃga", 0, F, true},
        {U"u mrȁku", 0, F, false}, {U"na zrȃku", 0, F, true},
        {U"jedna zrȃka", 0, F, true},
        {U"nova anketa", 1, R, false}, {U"dvije ankete", 1, R, false},
        {U"pet anketa", 1, R, true}, {U"nekoliko anketa", 1, R, true},
        {U"pet novih anketa", 1, R, true}, {U"pet ankèta", 1, R, false},
        {U"nova vještina", 1, R, false}, {U"dvije vještine", 1, R, false},
        {U"pet vještina", 1, R, true}, {U"nekoliko vještina", 1, R, true},
        {U"mnogo novih vještina", 1, R, true}, {U"pet vještìna", 1, R, false},
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
        R"({"entries":[{"word":"gl^a:su"},{"word":"komp/onenti"},{"word":"žir/afa"},{"word":"ank/eta"},{"word":"vješt/ina"},{"word":"vr^agu"},{"word":"zr^a:ka"},{"word":"zr^a:ku"}]})"));
    REQUIRE(nuclei(read(frontend, U"o vragu"))[1].accent == F);
    REQUIRE(!nuclei(read(frontend, U"o vragu"))[1].is_long);
    REQUIRE(nuclei(read(frontend, U"jedna zraka"))[2].is_long);
    REQUIRE(nuclei(read(frontend, U"na zraku"))[1].accent == F);
    REQUIRE(!nuclei(read(frontend, U"pet anketa"))[2].is_long);
    REQUIRE(!nuclei(read(frontend, U"pet vještina"))[2].is_long);
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

// =============================================================================
// Slovenian
// =============================================================================

namespace {

std::string phones_of(const Utterance& utterance) {
    static const char* const NAMES[] = {
        "_", "a", "e", "i", "o", "u", "@", "E", "O", "é", "ó", "ə", "w", "j", "v", "l", "lj",
        "m", "n", "nj", "ŋ", "r", "p", "b", "t", "d", "k", "g", "c", "dz", "č", "dž", "ć", "đ",
        "f", "s", "z", "š", "ž", "ś", "ź", "h", "ɣ"};
    std::string out;
    int word = -1;
    for (const auto& phone : utterance.phones) {
        if (phone.word != word) {
            if (word >= 0) out += " ";
            word = phone.word;
        }
        if (phone.nucleus && phone.stressed) out += "'";
        out += NAMES[static_cast<size_t>(phone.ph)];
        if (phone.nucleus && phone.is_long) out += ":";
    }
    return out;
}

} // namespace

TEST_CASE("Slovenian letters become the sounds of the Pravopis", "[frontend][slovenian]") {
    Frontend frontend(VoiceLanguage::Slovenian);
    struct Case { const char32_t* text; const char* phones; };
    const Case cases[] = {
        // Open and close e, o, the schwa and the final devoicing, from the lexicon
        {U"mesto", "m'é:stO"},
        {U"človek", "čl'O:vEk"},
        {U"človeka", "člOv'é:ka"},
        {U"otrok", "Otr'Ok"},
        {U"otroka", "Otr'O:ka"},
        {U"veter", "v'é:tər"},
        {U"vetra", "v'é:tra"},
        {U"pes", "p'əs"},
        {U"grad", "gr'a:t"},
        // l: [w] in participles, after a schwa and in the listed words, [l] otherwise
        {U"bil", "b'i:w"},
        {U"delal", "d'é:law"},
        {U"delala", "d'é:lala"},
        {U"rekel", "r'é:kəw"},
        {U"stol", "st'Ow"},
        {U"volk", "v'O:wk"},
        {U"kultura", "kult'u:ra"},
        {U"film", "f'i:lm"},
        {U"polje", "p'ó:ljE"},
        {U"konj", "k'On"},
        // v: [v] before a vowel, [w] before a consonant and at the end, [u] at the head
        {U"voda", "v'O:da"},
        {U"nov", "n'Ow"},                 // nòv nôva: short
        {U"avto", "'a:wtO"},
        {U"vlak", "wl'a:k"},
        {U"vsak", "us'a:k"},
        {U"vrt", "v'ərt"},
        // Syllabic r, i before a vowel
        {U"prst", "p'ərst"},
        {U"Trst", "t'ərst"},
        {U"pacient", "pacij'Ent"},
        // Compound numerals (an accent on the ones and on the tens, the
        // "in" a clitic, no boundary: SP 2001 ênaindvájset) and the words
        // of the number converter
        {U"enaindvajset", "'E:naindv'a:jsEt"},
        {U"petinpetdeset", "p'é:tinp'é:ddEsEt"},
        {U"tisoč", "t'i:sOč"},
        {U"milijonov", "milij'ó:nOw"},
    };
    for (const Case& c : cases) {
        DYNAMIC_SECTION(PhonemeMapper::utf32_to_utf8(c.text)) {
            REQUIRE(phones_of(read(frontend, c.text)) == c.phones);
        }
    }
}

TEST_CASE("Slovenian words reported by the first users (October 2026)", "[frontend][slovenian]") {
    Frontend frontend(VoiceLanguage::Slovenian);
    struct Case { const char32_t* text; const char* phones; };
    const Case cases[] = {
        // SSKJ, SP 2001 on Fran.si: the accents the reports were checked against
        {U"napajalnik", "napaj'a:lnik"},
        {U"zaganjalnik", "zaganj'a:lnik"},       // was zagánjalnik from the verb's stem
        {U"otoplitev", "OtOpl'i:təw"},
        {U"črkovanje", "čərkOv'a:njE"},          // was črkóvanje: the -ovanje rule
        {U"kupovati", "kupOv'a:ti"},
        {U"zahtevati", "zaht'é:vati"},
        {U"nadlegovanje", "nadlEgOv'a:njE"},
        {U"nadležen", "nadl'é:žən"},
        {U"seznam", "səzn'am"},                  // seznàm [səznam], was séznam
        {U"seznama", "səzn'a:ma"},
        {U"način", "nač'i:n"},                   // načín, was náčin
        {U"počiščeno", "pOč'i:ščEnO"},
        {U"ena", "'E:na"},                       // êna, open; was éna
        {U"en", "'En"},
        {U"tapnite", "t'a:pnitE"},
        {U"velika", "vEl'i:ka"},                 // vêlik, velíka
        {U"velik", "v'E:lik"},
        {U"del", "d'é:l"},                       // dél [tudi deu̯]: the [l] first
        {U"dvanajst", "dv'a:najst"},             // SSKJ²: dvánajst in dvanájst, the first
        {U"ob dvanajstih", "Ob dv'a:najstih"},    // and the ordinal with it (Sloleks dvánajsti)
        {U"sedemnajst", "s'é:dəmnajst"},
        {U"bralec", "br'a:ləc"},                 // the stress moves before -lec, -lnik, -lišče
        {U"gledališče", "glEdal'i:ščE"},
        {U"domov", "dOm'O:w"},
    };
    for (const Case& c : cases) {
        DYNAMIC_SECTION(PhonemeMapper::utf32_to_utf8(c.text)) {
            REQUIRE(phones_of(read(frontend, c.text)) == c.phones);
        }
    }
    // A spelling with two readings: the greeting or the imperative at the
    // head of the clause (and after a conjunction or an adverb that opens
    // a request), the verb's present elsewhere
    REQUIRE(phones_of(read(frontend, U"živijo")) == "ž'i:vijO");
    REQUIRE(phones_of(read(frontend, U"Živijo, kako si")) == "ž'i:vijO kak'ó: si");
    REQUIRE(phones_of(read(frontend, U"Oni živijo tukaj")) == "'O:ni živ'i:jO t'u:kaj");
    REQUIRE(phones_of(read(frontend, U"Naredi to")) == "nar'E:di t'ó:");
    REQUIRE(phones_of(read(frontend, U"Zdaj naredi to")) == "zd'aj nar'E:di t'ó:");
    REQUIRE(phones_of(read(frontend, U"Program naredi kopijo")) == "prOgr'a:m narEd'i: k'ó:pijO");
    REQUIRE(phones_of(read(frontend, U"Uredi")) == "ur'E:di");
    // meni alone is the menu (SSKJ² mení); in a sentence the pronoun mêni
    // (after a preposition, before se, je, ni) or the verb méni
    REQUIRE(phones_of(read(frontend, U"Meni")) == "mEn'i:");
    REQUIRE(phones_of(read(frontend, U"Meni je všeč")) == "m'E:ni jE uš'é:č");
    REQUIRE(phones_of(read(frontend, U"Meni se zdi")) == "m'E:ni sE zd'i:");
    REQUIRE(phones_of(read(frontend, U"Pridi k meni")) == "pr'i:di k m'E:ni");
    REQUIRE(phones_of(read(frontend, U"Kaj meni o tem")) == "k'a:j m'é:ni O t'é:m");
    REQUIRE(phones_of(read(frontend, U"Meni gumb")) == "m'é:ni g'u:mp");
    // The participle of biti on its last syllable (SSKJ² bilà biló bilí bilè)
    REQUIRE(phones_of(read(frontend, U"bila je")) == "bil'a jE");
    REQUIRE(phones_of(read(frontend, U"bilo je lepo")) == "bil'ó: jE lEp'ó:");
    REQUIRE(phones_of(read(frontend, U"bili so")) == "bil'i: sO");
    REQUIRE(phones_of(read(frontend, U"bile so")) == "bil'E sO");
    // The number converter's words: the ones keep their accent before the tens
    REQUIRE(phones_of(read(frontend, U"sto enaindvajset")) == "st'ó: 'E:naindv'a:jsEt");
}

TEST_CASE("Slovenian lexicon of every Sloleks lemma and verb/noun spellings (October 2026)",
          "[frontend][slovenian]") {
    Frontend frontend(VoiceLanguage::Slovenian);
    struct Case { const char32_t* text; const char* phones; };
    const Case cases[] = {
        // Reported: SSKJ² karamból, zobobòl -ôla (lemmas below the table's old
        // frequency threshold, read by the second-last-syllable rule)
        {U"karambol", "karamb'ó:l"},
        {U"zobobol", "zObOb'Ol"},
        // A stem of one word read another: postavít*, of postavítev, gave
        // postavíti; the table is now checked form by form
        {U"postaviti", "pOst'a:viti"},
        {U"preveriti", "prEv'é:riti"},
        {U"nastavitve", "nastav'i:tvE"},
        // A homograph by the frequency of the form, not of the lemma:
        // veselím "I look forward" is eight times the adjective's vesélim
        {U"veselim", "vEsEl'i:m"},
        // Forms with two spellings (mesca, meseca) were dropped
        {U"meseca", "m'é:sEca"},
        {U"mesecih", "m'é:sEcih"},
        // The fleeting e is the schwa (rékel rêkla, začétek začétka), and
        // only then (vídel vídela; črkujem is not črkujmo's)
        {U"začetek", "zač'é:tək"},
        {U"videl", "v'i:dEw"},
        {U"črkujem", "čərk'u:jEm"},
        // Hand entries written from memory that the dictionaries contradict
        {U"beseda", "bEs'é:da"},                 // beséda, not béseda
        {U"višina", "viš'i:na"},
        {U"sestra", "s'E:stra"},                 // sêstra
        {U"septembra", "sEpt'E:mbra"},           // septêmber -bra
    };
    for (const Case& c : cases) {
        DYNAMIC_SECTION(PhonemeMapper::utf32_to_utf8(c.text)) {
            REQUIRE(phones_of(read(frontend, c.text)) == c.phones);
        }
    }
    // veseli: the verb with its se, an object or at the end of the clause,
    // the adjective after a form of biti and before a noun
    REQUIRE(phones_of(read(frontend, U"On se zelo veseli")) == "'On sE zEl'ó: vEsEl'i:");
    REQUIRE(phones_of(read(frontend, U"To me veseli")) == "t'ó: mE vEsEl'i:");
    REQUIRE(phones_of(read(frontend, U"Otroci so veseli")) == "Otr'O:ci sO vEs'é:li");
    REQUIRE(phones_of(read(frontend, U"Veseli ljudje")) == "vEs'é:li ljudj'é:");
    // leti: letí "flies", léti after a number or a preposition
    REQUIRE(phones_of(read(frontend, U"Letalo leti nad mestom")) == "lEt'a:lO lEt'i: nad m'é:stOm");
    REQUIRE(phones_of(read(frontend, U"Ptič leti")) == "pt'ič lEt'i:");
    REQUIRE(phones_of(read(frontend, U"Dve leti")) == "dv'é: l'é:ti");
    REQUIRE(phones_of(read(frontend, U"Pred dvema leti")) == "prE dv'é:ma l'é:ti");
    // A participle after its auxiliary, the noun after a preposition
    REQUIRE(phones_of(read(frontend, U"On je vozil avto")) == "'On jE v'O:ziw 'a:wtO");
    REQUIRE(phones_of(read(frontend, U"Brez novih vozil")) == "brEz n'O:vih vOz'i:l");
}

TEST_CASE("Slovenian clitics, sandhi and clause types", "[frontend][slovenian]") {
    Frontend frontend(VoiceLanguage::Slovenian);
    // Proclitics and enclitics lose their stress and length; v before a
    // consonant is the vowel u; the final d of a preposition stays voiced
    REQUIRE(phones_of(read(frontend, U"Bil sem v šoli")) == "b'i:w səm u š'ó:li");
    REQUIRE(phones_of(read(frontend, U"v Ameriki")) == "w am'E:riki");
    REQUIRE(phones_of(read(frontend, U"od orača")) == "Od Or'a:ča");
    // Regressive voicing assimilation across a clitic boundary and inside a word
    REQUIRE(phones_of(read(frontend, U"k bratu")) == "g br'a:tu");
    REQUIRE(phones_of(read(frontend, U"s časom")) == "š č'a:sOm");
    REQUIRE(phones_of(read(frontend, U"kdo")) == "gd'ó:");
    // An acronym is stressed on its last letter
    REQUIRE(phones_of(read(frontend, U"SP")) == "Esp'é:");
    // Clause types: a question word, a yes/no question, an exclamation
    REQUIRE(frontend.process(U"Kdo je to", Punctuation::QUESTION).kind == ClauseKind::WhQuestion);
    REQUIRE(frontend.process(U"Ali greš domov", Punctuation::QUESTION).kind == ClauseKind::YesNoQuestion);
    REQUIRE(frontend.process(U"Pojdi", Punctuation::EXCLAMATION).kind == ClauseKind::Exclamation);
    // Every Slovenian word has the plain accent, never a tone
    for (const auto& phone : read(frontend, U"Dober dan, kako ste").phones) {
        if (phone.stressed) REQUIRE(phone.accent == Accent::Neutral);
    }
}

TEST_CASE("Slovenian accents written in the text and in the user's lexicon win", "[frontend][slovenian]") {
    Frontend frontend(VoiceLanguage::Slovenian);
    // Dictionary marks typed in the text: acute = long close, circumflex =
    // long open, grave = short
    REQUIRE(phones_of(read(frontend, U"mésto")) == "m'é:stO");
    REQUIRE(phones_of(read(frontend, U"mêsto")) == "m'E:stO");
    REQUIRE(phones_of(read(frontend, U"mèsto")) == "m'EstO");
    REQUIRE(phones_of(read(frontend, U"mestó")) == "mEst'ó:");
    // A word the lexicon does not know: the second-last syllable, long and close
    REQUIRE(phones_of(read(frontend, U"bruhoberko")) == "bruhOb'é:rkO");
    // The user's entries, in the Slovenian notation, before the built-in ones
    frontend.set_user_lexicon(UserLexicon::parse(
        R"({"entries":[{"word":"bruhobêrk*","language":"sl"},{"word":"mestó","language":"sl"},{"word":"p'əs","language":"sl"}]})"));
    REQUIRE(phones_of(read(frontend, U"bruhoberko")) == "bruhOb'E:rkO");
    REQUIRE(phones_of(read(frontend, U"bruhoberkom")) == "bruhOb'E:rkOm");
    REQUIRE(phones_of(read(frontend, U"mesto")) == "mEst'ó:");
    // A malformed entry is counted and skipped
    UserLexicon::Report report;
    UserLexicon::parse(R"({"entries":[{"word":"mesto","language":"sl"},{"verb":"ur'e:d=i","language":"sl"}]})", &report);
    REQUIRE(report.rejected == 2);
    REQUIRE(report.words == 0);
}
