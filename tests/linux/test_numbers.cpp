// Direct number-normalization regressions; no audio data required.
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "core/croatian_numbers.hpp"
#include <iostream>
using namespace laprdus;

// Expected group boundaries are independent of the parser; spell their digits
// through the public digit-name API so the same matrix covers all four languages.
static std::string phone_words(CroatianNumbers& n, std::string_view groups, bool by_digit) {
    std::string out;
    size_t i = 0;
    if (groups.substr(0, 2) == "+ ") { out = "+ "; i = 2; }
    while (i < groups.size()) {
        size_t end = groups.find('|', i);
        if (end == std::string_view::npos) end = groups.size();
        if (by_digit) {
            for (size_t k = i; k < end; ++k) {
                if (k > i) out += ' ';
                out += n.digit_to_croatian_word(groups[k]);
            }
        } else {
            while (i + 1 < end && groups[i] == '0') {
                out += n.digit_to_croatian_word('0') + ' ';
                ++i;
            }
            out += end - i == 1 ? n.digit_to_croatian_word(groups[i]) : n.number_to_words(groups.substr(i, end - i));
        }
        i = end + 1;
        if (i < groups.size()) out += ", ";
    }
    return out;
}

TEST_CASE("Telephone groups honor the number mode in every language", "[numbers][phone]") {
    const std::pair<const char*, const char*> cases[] = {
        {"+38641453375", "+ 386|41|4533|75"},
        {"0038641453375", "00386|41|4533|75"},
        {"00 386 41 453 375", "00386|41|4533|75"},
        {"+386 41/453-375", "+ 386|41|4533|75"},
        {"+\u00a0386 41 453375", "+ 386|41|4533|75"},
        {"+ 386 / 41 - 453375", "+ 386|41|4533|75"},
        {"+385911234567", "+ 385|91|1234|567"},
        {"00385911234567", "00385|91|1234|567"},
        {"+385 91 123 4567", "+ 385|91|1234|567"},
        {"+385/91/1234/567", "+ 385|91|1234|567"},
        {"+385-91-1234567", "+ 385|91|1234|567"},
        {"+385\u00a091\u202f123\u20094567", "+ 385|91|1234|567"},
        {"+385 91 0000001", "+ 385|91|0000|001"},
        {"+385 1 2345672", "+ 385|1|2345|672"},
        {"+38512345672", "+ 385|1|2345|672"},
        {"+38612345678", "+ 386|1|2345|678"},
        {"+38733123456", "+ 387|33|1234|56"},
        {"+381111234567", "+ 381|11|1234|567"},
        {"+381230123456", "+ 381|230|1234|56"},
        {"+381 230 1234567", "+ 381|230|1234|567"},
        {"+49123123456", "+ 49|12|3123|456"}, // ambiguous compact 9 digits: 2+7
        {"+49 123 123456", "+ 49|123|1234|56"}, // written code takes precedence
        {"+49 123 1234567", "+ 49|123|1234|567"},
        {"+49 12345 123456", "+ 49|12345|1234|56"},
        {"+12025550123", "+ 1|202|5550|123"},
        {"0012025550123", "001|202|5550|123"},
        {"+74951234567", "+ 7|495|1234|567"},
        {"+442079460018", "+ 44|20|7946|0018"},
        {"+393331234567", "+ 39|333|1234|567"},
        {"+390212345678", "+ 39|02|1234|5678"},
        {"+385 (0)91 1234567", "+ 385|091|1234|567"},
        {"+385 (91) 123-4567", "+ 385|91|1234|567"},
        {"+3858001234", "+ 385|800|1234"},
        {"+80012345678", "+ 800|1234|5678"},
    };
    CroatianNumbers n;
    for (auto dialect : {CroatianNumbers::Dialect::Croatian, CroatianNumbers::Dialect::Serbian,
                         CroatianNumbers::Dialect::Bosnian, CroatianNumbers::Dialect::Slovenian}) {
        n.set_dialect(dialect);
        for (bool by_digit : {false, true}) {
            auto convert = [&](const std::string& text) {
                return by_digit ? n.convert_digits_in_text(text) : n.convert_numbers_in_text(text);
            };
            for (auto c : cases) {
                INFO(c.first << " mode=" << by_digit << " language=" << int(dialect));
                const std::string expected = phone_words(n, c.second, by_digit);
                if (convert(c.first) != expected) std::cerr << c.first << " mode=" << by_digit << " dialect=" << int(dialect) << " -> " << convert(c.first) << " expected " << expected << "\n";
                REQUIRE(convert(c.first) == expected);
                REQUIRE(convert(std::string("Telefon: ") + c.first + "!") == "Telefon: " + expected + "!");
                // Neither a preceding preposition nor a following noun may
                // decline the final digit or change it to a feminine form.
                REQUIRE(convert(std::string("do ") + c.first + " ure") == "do " + expected + " ure");
            }
            for (const char* phone : {"031123456", "031/123-456", "031 123 456", "(031) 123456"}) {
                // Written domestic area codes are independent of the language.
                REQUIRE(convert(phone) == phone_words(n, "031|1234|56", by_digit));
            }
        }
    }
    n.set_dialect(CroatianNumbers::Dialect::Slovenian);
    REQUIRE(n.convert_digits_in_text("+386 1 2222222") ==
            "+ tri osem šest, ena, dve dve dve dve, dve dve dve");
    REQUIRE(n.convert_digits_in_text("031123456") == "nič tri ena, ena dve tri štiri, pet šest");
    n.set_dialect(CroatianNumbers::Dialect::Croatian);
    REQUIRE(n.convert_numbers_in_text("+385 91 1234567") ==
            "+ tristo osamdeset pet, devedeset jedan, tisuću dvjesto trideset četiri, petsto šezdeset sedam");
}

TEST_CASE("Telephone recognition leaves other numeric formats alone", "[numbers][phone]") {
    CroatianNumbers n;
    REQUIRE(n.convert_numbers_in_text("+12") == "+dvanaest");
    REQUIRE(n.convert_numbers_in_text("+123456") == "+" + n.number_to_words("123456"));
    REQUIRE(n.convert_numbers_in_text("+1234567890123456") == "+" + n.number_to_words("1234567890123456"));
    REQUIRE(n.convert_numbers_in_text("38641453375") == n.number_to_words("38641453375"));
    REQUIRE(n.convert_numbers_in_text("+1234567,5").find(", ") == std::string::npos);
    REQUIRE(n.convert_numbers_in_text("+385911234567.5").find(", ") == std::string::npos);
    REQUIRE(n.convert_numbers_in_text("ID+385911234567").find(", ") == std::string::npos);
    REQUIRE(n.convert_numbers_in_text("+385911234567kg").find(", ") == std::string::npos);
    REQUIRE(n.convert_numbers_in_text("+99912345678").find(", ") == std::string::npos);
    REQUIRE(n.convert_numbers_in_text("1 000 000") == "milijun");
    REQUIRE(n.convert_numbers_in_text("12:30") == "dvanaest trideset");
    REQUIRE(n.convert_numbers_in_text("7.10.2026") == "sedmog desetog dvije tisuće dvadeset šeste");
    REQUIRE(n.convert_numbers_in_text("112") == "sto dvanaest");
    REQUIRE(n.convert_digits_in_text("112") == "jedan jedan dva");
    REQUIRE(n.convert_digits_in_text("+ 1 2 3,5 6,7 8 9,8 7 6") ==
            "+ jedan dva tri, pet šest, sedam osam devet, osam sedam šest");
    // Newlines terminate phone candidates; multiple numbers remain separate.
    REQUIRE(n.convert_digits_in_text("+385911234567\n+38641453375") ==
            phone_words(n, "+ 385|91|1234|567", true) + "\n" + phone_words(n, "+ 386|41|4533|75", true));
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

TEST_CASE("Currency formats normalize grouping signs and affixes", "[numbers][currency]") {
    CroatianNumbers n;
    const std::string amount = "dvije tisuće sto pedeset zarez nula nula";
    const std::pair<const char*, const char*> cases[] = {
        {"2.150,00", "AMOUNT"}, {"Stanje 2.150,00", "Stanje AMOUNT"},
        {"Račun 2.150,00", "Račun AMOUNT"}, {"Saldo:2.150,00", "Saldo:AMOUNT"},
        {"-2.150,00", "minus AMOUNT"}, {"−2.150,00", "minus AMOUNT"},
        {"- 2.150,00", "minus AMOUNT"}, {"Stanje:-2.150,00", "Stanje:minus AMOUNT"},
        {"2 150,00", "AMOUNT"}, {"2\u00a0150,00", "AMOUNT"},
        {"2\u202f150,00", "AMOUNT"}, {"2\u2009150,00", "AMOUNT"},
        {"2.150,00 €", "AMOUNT eura"}, {"2.150,00€", "AMOUNT eura"},
        {"€2.150,00", "AMOUNT eura"}, {"EUR 2.150,00", "AMOUNT eura"},
        {"2.150,00 EUR", "AMOUNT eura"}, {"€ 2150.00", "AMOUNT eura"},
        {"2,150.00 €", "AMOUNT eura"}, {"2'150.00 EUR", "AMOUNT eura"},
        {"2’150.00 EUR", "AMOUNT eura"}, {"$2,150.00", "AMOUNT dolara"},
        {"$2.150,00", "AMOUNT dolara"}, {"USD2,150.00", "AMOUNT dolara"},
        {"2,150.00USD", "AMOUNT dolara"}, {"-$2,150.00", "minus AMOUNT dolara"},
        {"$-2,150.00", "minus AMOUNT dolara"}, {"$ − 2,150.00", "minus AMOUNT dolara"},
        {"−2.150,00 €", "minus AMOUNT eura"}, {"2.150,00€-", "minus AMOUNT eura"},
        {"$2,150.00-", "minus AMOUNT dolara"}, {"($2,150.00)", "minus AMOUNT dolara"},
        {"$(2,150.00)", "minus AMOUNT dolara"}, {"(2.150,00 €)", "minus AMOUNT eura"},
        {"+2.150,00 €", "plus AMOUNT eura"}, {"2.150,00 €!", "AMOUNT eura!"},
        {"2.150,00- €", "minus AMOUNT eura"},
        {"2.150,00 € −", "minus AMOUNT eura"},
        {"$2,150", "dvije tisuće sto pedeset dolara"},
        {"€2.150", "dvije tisuće sto pedeset eura"},
        {"€2,150", "dva zarez jedan pet nula eura"},
        {"$1", "jedan dolar"}, {"1€", "jedan euro"}, {"21 €", "dvadeset jedan euro"},
        {"$0.00", "nula zarez nula nula dolara"}, {"-€0,01", "minus nula zarez nula jedan eura"},
        {"$12.05", "dvanaest zarez nula pet dolara"},
        {"€1.234.567,89", "milijun dvjesto trideset četiri tisuće petsto šezdeset sedam zarez osamdeset devet eura"},
        {"$1,234,567.89", "milijun dvjesto trideset četiri tisuće petsto šezdeset sedam zarez osamdeset devet dolara"},
    };
    for (auto c : cases) {
        std::string expected = c.second;
        const size_t at = expected.find("AMOUNT");
        if (at != std::string::npos) expected.replace(at, 6, amount);
        const auto got = n.convert_numbers_in_text(c.first);
        if (got != expected) std::cerr << c.first << " -> " << got << " expected " << expected << '\n';
        REQUIRE(got == expected);
    }
    REQUIRE(n.convert_numbers_in_text("-2.500,00") == "minus dvije tisuće petsto zarez nula nula");
    REQUIRE(n.convert_numbers_in_text("Stanje 2.150") == "Stanje dvije tisuće sto pedeset");
    REQUIRE(n.convert_digits_in_text("-2.500,00 €") == "minus dva pet nula nula zarez nula nula eura");
    REQUIRE(n.convert_digits_in_text("$2,150.00") == "dva jedan pet nula zarez nula nula dolara");
    REQUIRE(n.convert_digits_in_text("− 2500") == "minus dva pet nula nula");
}

TEST_CASE("Currency reading keeps the language and ordinary numeric meanings", "[numbers][currency]") {
    CroatianNumbers n;
    n.set_dialect(CroatianNumbers::Dialect::Serbian);
    REQUIRE(n.convert_numbers_in_text("-2.150,00 €") == "minus dve hiljade sto pedeset zarez nula nula evra");
    n.set_dialect(CroatianNumbers::Dialect::Bosnian);
    REQUIRE(n.convert_numbers_in_text("$2,150.00") == "dvije hiljade sto pedeset zarez nula nula dolara");
    n.set_dialect(CroatianNumbers::Dialect::Slovenian);
    REQUIRE(n.convert_numbers_in_text("2.150,00 €") == "dva tisoč sto petdeset celih nič nič evra");
    REQUIRE(n.convert_numbers_in_text("do €1") == "do enega evra");
    REQUIRE(n.convert_numbers_in_text("s 3 €") == "s tremi evri");
    REQUIRE(n.convert_digits_in_text("€2,00") == "dve vejica nič nič evra");
    n.set_dialect(CroatianNumbers::Dialect::Croatian);
    REQUIRE(n.convert_numbers_in_text("3.14") == "tri\u2024četrnaest");
    REQUIRE(n.convert_numbers_in_text("7.10.2026") == "sedmog desetog dvije tisuće dvadeset šeste");
    REQUIRE(n.convert_numbers_in_text("12:30") == "dvanaest trideset");
    REQUIRE(n.convert_numbers_in_text("192.168.1.1") == "sto devedeset dva\u2024sto šezdeset osam\u2024jedan\u2024jedan");
    REQUIRE(n.convert_numbers_in_text("T-2") == "T-dva");
    REQUIRE(n.convert_numbers_in_text("1€2€") == "jedan euro dva eura");
    REQUIRE(n.convert_numbers_in_text("1991-2000").find("minus") == std::string::npos);
    REQUIRE(n.convert_digits_in_text("091/123-4567") == phone_words(n, "091|1234|567", true));
    REQUIRE(n.convert_numbers_in_text("(2.150,00)").find("minus") == std::string::npos);
    REQUIRE(n.convert_numbers_in_text("$2.00-3.00").find("minus") == std::string::npos);
    REQUIRE(n.convert_numbers_in_text("2.150,00\n3.150,00") ==
            "dvije tisuće sto pedeset zarez nula nula\ntri tisuće sto pedeset zarez nula nula");
}
