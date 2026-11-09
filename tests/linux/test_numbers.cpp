// Direct number-normalization regressions; no audio data required.
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "core/croatian_numbers.hpp"
#include <iostream>
using namespace laprdus;

TEST_CASE("International telephone digits in every language", "[numbers]") {
    CroatianNumbers n;
    for (auto dialect : {CroatianNumbers::Dialect::Croatian, CroatianNumbers::Dialect::Serbian,
                         CroatianNumbers::Dialect::Bosnian, CroatianNumbers::Dialect::Slovenian}) {
        n.set_dialect(dialect);
        std::string expected = "+ ";
        for (char digit : std::string("38641453375")) {
            if (expected.size() > 2) expected += ' ';
            expected += n.number_to_words(std::string(1, digit));
        }
        REQUIRE(n.convert_numbers_in_text("+38641453375") == expected);
        REQUIRE(n.convert_numbers_in_text("Telefon: +38641453375!") == "Telefon: " + expected + "!");
        REQUIRE(n.convert_numbers_in_text("+12") == "+" + n.number_to_words("12"));
        REQUIRE(n.convert_numbers_in_text("+123456") == "+" + n.number_to_words("123456"));
        REQUIRE(n.convert_numbers_in_text("+1234567890123456") == "+" + n.number_to_words("1234567890123456"));
        REQUIRE(n.convert_numbers_in_text("+1234567,5").find("+ jedan dva") == std::string::npos);
        REQUIRE(n.convert_numbers_in_text("38641453375") == n.number_to_words("38641453375"));
    }
}
TEST_CASE("Slovenian ordinal noun agreement", "[numbers]") {
    CroatianNumbers n;
    n.set_dialect(CroatianNumbers::Dialect::Slovenian);
    for (const auto& noun : {"dete", "letališče", "ognjišče", "igrišče", "središče", "nadstropje", "ime", "dekle"}) {
        REQUIRE(n.convert_numbers_in_text(std::string("1 ") + noun + " ni 1. " + noun) ==
                std::string("eno ") + noun + " ni prvo " + noun);
        REQUIRE(n.convert_numbers_in_text(std::string("3. ") + noun) == std::string("tretje ") + noun);
    }
    const std::pair<const char*, const char*> cases[] = {
        {"1. ura", "prva ura"}, {"do 1. ure", "do prve ure"},
        {"k 1. uri", "k prvi uri"}, {"za 1. uro", "za prvo uro"},
        {"ob 1. uri", "ob prvi uri"}, {"s 1. uro", "s prvo uro"},
        {"do 1. hiše", "do prve hiše"}, {"s 3. hišo", "s tretjo hišo"},
        {"k 1. hišam", "k prvim hišam"}, {"s 1. hišami", "s prvimi hišami"},
        {"v 1. hišah", "v prvih hišah"}, {"s 1. hišama", "s prvima hišama"},
        {"do 1. letališča", "do prvega letališča"},
        {"k 1. letališču", "k prvemu letališču"},
        {"na 1. letališču", "na prvem letališču"},
        {"pred 1. letališčem", "pred prvim letališčem"},
    };
    for (auto c : cases) { INFO(c.first); REQUIRE(n.convert_numbers_in_text(c.first) == c.second); }
}
TEST_CASE("Croatian Serbian Bosnian ordinal cases", "[numbers]") {
    CroatianNumbers n;
    for (auto dialect : {CroatianNumbers::Dialect::Croatian, CroatianNumbers::Dialect::Serbian,
                         CroatianNumbers::Dialect::Bosnian}) {
        n.set_dialect(dialect);
        const std::pair<const char*, const char*> cases[] = {
            {"1. godina", "prva godina"}, {"do 1. godine", "do prve godine"},
            {"prema 1. godini", "prema prvoj godini"}, {"u 1. godinu", "u prvu godinu"},
            {"u 1. godini", "u prvoj godini"}, {"s 1. godinom", "s prvom godinom"},
            {"1. ženo", "prva ženo"}, {"1. škole", "prve škole"}, {"s 3. školom", "s trećom školom"},
            {"u 1. sobu", "u prvu sobu"}, {"na 1. nagradu", "na prvu nagradu"},
            {"s 1. prijateljem", "s prvim prijateljem"},
            {"do 1. razreda", "do prvog razreda"}, {"u 1. razredu", "u prvom razredu"},
            {"1. godine", "prve godine"}, {"u 1. godinama", "u prvim godinama"},
            {"2026. godine", "YEAR šeste godine"}, {"2026. godina", "YEAR šesta godina"},
            {"u 2026. godini", "u YEAR šestoj godini"},
        };
        for (auto c : cases) {
            INFO(c.first);
            std::string expected = c.second;
            auto pos = expected.find("YEAR");
            if (pos != std::string::npos) expected.replace(pos, 4,
                dialect == CroatianNumbers::Dialect::Croatian ? "dvije tisuće dvadeset" :
                dialect == CroatianNumbers::Dialect::Serbian ? "dve hiljade dvadeset" : "dvije hiljade dvadeset");
            if (n.convert_numbers_in_text(c.first) != expected) std::cerr << c.first << " -> " << n.convert_numbers_in_text(c.first) << " expected " << expected << "\n";
            REQUIRE(n.convert_numbers_in_text(c.first) == expected);
        }
        REQUIRE(n.convert_numbers_in_text("s 1. minutom") ==
                (dialect == CroatianNumbers::Dialect::Serbian ? "s prvim minutom" : "s prvom minutom"));
        REQUIRE(n.convert_numbers_in_text("у 1. години") == "у prvoj години");
    }
}
