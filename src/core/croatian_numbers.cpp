// -*- coding: utf-8 -*-
// croatian_numbers.cpp - Croatian number to words implementation
// Ported from Python numbers.py

#include "croatian_numbers.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace laprdus {

// =============================================================================
// Single Digit to Word (0-9)
// =============================================================================

// The name of a digit read by itself. Slovenian: SP 2001, § 11 and SSKJ² give dve
// as the name of the number 2 (ena in ena je dve), also in a phone number
// (Jezikovna svetovalnica, "Uporaba dva in dve", 2014).
std::string_view CroatianNumbers::digit_name(char digit) {
    if (slovenian() && digit == '2') return "dve";
    return digit_to_word(digit);
}

std::string_view CroatianNumbers::digit_to_word(char digit) {
    if (slovenian()) {
        // The word of a digit inside a longer number: dva tisoč, sto dva. The
        // number 2 itself is dve (digit_name(), slovenian_agree()). en/ena and
        // dva/dve agree with a scale word in process_number_groups, and the
        // compounds have dva- (dvaindvajset)
        switch (digit) {
            case '0': return u8"nič";
            case '1': return "ena";
            case '2': return "dva";
            case '3': return "tri";
            case '4': return u8"štiri";
            case '5': return "pet";
            case '6': return u8"šest";
            case '7': return "sedem";
            case '8': return "osem";
            case '9': return "devet";
            default: return "";
        }
    }
    switch (digit) {
        case '0': return "nula";
        case '1': return "jedan";
        case '2': return "dva";
        case '3': return "tri";
        case '4': return u8"četiri";
        case '5': return "pet";
        case '6': return u8"šest";
        case '7': return "sedam";
        case '8': return "osam";
        case '9': return "devet";
        default: return "";
    }
}

// =============================================================================
// Tens to Words (10, 20, 30, ..., 90)
// =============================================================================

std::string CroatianNumbers::tens_to_words(char tens_digit) {
    if (slovenian()) {
        switch (tens_digit) {
            case '1': return "deset";
            case '2': return "dvajset";
            case '3': return "trideset";
            case '4': return u8"štirideset";
            case '5': return "petdeset";
            case '6': return u8"šestdeset";
            case '7': return "sedemdeset";
            case '8': return "osemdeset";
            case '9': return "devetdeset";
            default: return "";
        }
    }
    switch (tens_digit) {
        case '1': return "deset";
        case '2': return "dvadeset";
        case '3': return "trideset";
        case '4': return u8"četrdeset";
        case '5': return "pedeset";
        case '6': return u8"šezdeset";
        case '7': return "sedamdeset";
        case '8': return "osamdeset";
        case '9': return "devedeset";
        default: return "";
    }
}

// =============================================================================
// Teens to Words (11-19)
// =============================================================================

std::string CroatianNumbers::teens_to_words(char ones_digit) {
    if (slovenian()) {
        switch (ones_digit) {
            case '1': return "enajst";
            case '2': return "dvanajst";
            case '3': return "trinajst";
            case '4': return u8"štirinajst";
            case '5': return "petnajst";
            case '6': return u8"šestnajst";
            case '7': return "sedemnajst";
            case '8': return "osemnajst";
            case '9': return "devetnajst";
            default: return "";
        }
    }
    switch (ones_digit) {
        case '1': return "jedanaest";
        case '2': return "dvanaest";
        case '3': return "trinaest";
        case '4': return u8"četrnaest";
        case '5': return "petnaest";
        case '6': return u8"šesnaest";
        case '7': return "sedamnaest";
        case '8': return "osamnaest";
        case '9': return "devetnaest";
        default: return "";
    }
}

// =============================================================================
// Two Digit Number to Words (10-99)
// =============================================================================

std::string CroatianNumbers::two_digit_to_words(std::string_view two_digits) {
    if (two_digits.size() != 2) return "";

    char tens = two_digits[0];
    char ones = two_digits[1];

    // Handle pure tens (10, 20, 30, etc.)
    if (ones == '0') {
        if (tens >= '1' && tens <= '9') {
            return tens_to_words(tens);
        }
        return "";
    }

    // Handle teens (11-19)
    if (tens == '1') {
        return teens_to_words(ones);
    }

    // Handle 01-09 (leading zero)
    if (tens == '0') {
        return std::string(digit_to_word(ones));
    }

    // Handle 21-99 (excluding teens). Slovenian puts the ones first and
    // writes the whole as one word: enaindvajset, štiriinštirideset.
    std::string result = tens_to_words(tens);
    if (result.empty()) return result;
    if (slovenian()) {
        return std::string(slovenian_compound_unit(ones)) + "in" + result;
    }
    result += " ";
    result += digit_to_word(ones);
    return result;
}

// =============================================================================
// Hundreds Word (100, 200, 300, etc.)
// =============================================================================

std::string CroatianNumbers::hundreds_word(char digit) {
    if (slovenian()) {
        switch (digit) {
            case '1': return "sto";
            case '2': return "dvesto";
            case '3': return "tristo";
            case '4': return u8"štiristo";
            case '5': return "petsto";
            case '6': return u8"šeststo";
            case '7': return "sedemsto";
            case '8': return "osemsto";
            case '9': return "devetsto";
            default: return "";
        }
    }
    switch (digit) {
        case '1': return "sto";
        case '2': return m_dialect == Dialect::Serbian ? "dvesta" : "dvjesto";
        case '3': return m_dialect == Dialect::Serbian ? "trista" : "tristo";
        case '4': return u8"četiristo";
        case '5': return "petsto";
        case '6': return u8"šesto";
        case '7': return "sedamsto";
        case '8': return "osamsto";
        case '9': return "devetsto";
        default: return "";
    }
}

// =============================================================================
// Three Digit Number to Words (100-999)
// =============================================================================

std::string CroatianNumbers::three_digit_to_words(std::string_view three_digits) {
    if (three_digits.size() != 3) return "";

    char hundreds_digit = three_digits[0];
    std::string_view last_two = three_digits.substr(1, 2);

    std::string result;

    // Add hundreds part if non-zero
    if (hundreds_digit >= '1' && hundreds_digit <= '9') {
        result = hundreds_word(hundreds_digit);
    }

    // Add tens and ones
    std::string two_digit_part = two_digit_to_words(last_two);
    if (!two_digit_part.empty()) {
        if (!result.empty()) {
            result += " ";
        }
        result += two_digit_part;
    }

    return result;
}

// =============================================================================
// Group to Words (1-3 digit group)
// =============================================================================

std::string CroatianNumbers::group_to_words(std::string_view group) {
    if (group.empty()) return "";

    switch (group.size()) {
        case 1:
            return std::string(digit_to_word(group[0]));
        case 2:
            return two_digit_to_words(group);
        case 3:
            return three_digit_to_words(group);
        default:
            return "";
    }
}

// =============================================================================
// Thousand Variants (tisuću, tisuće, tisuća)
// =============================================================================

std::string_view CroatianNumbers::get_thousand_variant(char last_digit) {
    // Slovenian tisoč does not change: tisoč, dva tisoč, pet tisoč
    if (slovenian()) return u8"tisoč";
    if (m_dialect != Dialect::Croatian) {
        switch (last_digit) {
            case '1':
                return "hiljadu";
            case '2':
            case '3':
            case '4':
                return "hiljade";
            default:
                return "hiljada";
        }
    }

    switch (last_digit) {
        case '1':
            return u8"tisuću";      // Nominative singular
        case '2':
        case '3':
        case '4':
            return u8"tisuće";      // Nominative plural for 2-4
        default:
            return u8"tisuća";      // Genitive plural for 0, 5-9
    }
}

// =============================================================================
// Million Variants (milijun, milijuna)
// =============================================================================

std::string CroatianNumbers::get_million_variant(std::string_view prefix, char last_digit) {
    std::string result(prefix);
    if (slovenian()) {
        // en milijon, dva milijona (dual), trije/štirje milijoni, pet
        // milijonov; last_digit '0' is the genitive plural of 5-9, 11-19
        // and every compound (enaindvajset milijonov)
        switch (last_digit) {
            case '1': result += "lijon"; break;
            case '2': result += "lijona"; break;
            case '3':
            case '4': result += "lijoni"; break;
            default: result += "lijonov"; break;
        }
        return result;
    }
    const bool eastern = m_dialect != Dialect::Croatian;
    switch (last_digit) {
        case '1':
            result += eastern ? "lion" : "lijun";
            break;
        default:
            result += eastern ? "liona" : "lijuna";
            break;
    }
    return result;
}

// =============================================================================
// Milliard Variants (milijarda, milijarde, milijardi)
// =============================================================================

std::string CroatianNumbers::get_milliard_variant(std::string_view prefix, char last_digit) {
    std::string result(prefix);
    if (slovenian()) {
        // ena milijarda, dve milijardi (dual), tri/štiri milijarde, pet milijard
        switch (last_digit) {
            case '1': result += "lijarda"; break;
            case '2': result += "lijardi"; break;
            case '3':
            case '4': result += "lijarde"; break;
            default: result += "lijard"; break;
        }
        return result;
    }
    switch (last_digit) {
        case '1':
            result += "lijarda";
            break;
        case '2':
        case '3':
        case '4':
            result += "lijarde";
            break;
        default:
            result += "lijardi";
            break;
    }
    return result;
}

// =============================================================================
// Large Number Suffix by Group Index
// =============================================================================

std::string CroatianNumbers::get_large_number_suffix(int group_index, char last_digit) {
    // group_index: 0 = thousands, 1 = millions, 2 = milliards, etc.
    switch (group_index) {
        case 0:
            return std::string(get_thousand_variant(last_digit));
        case 1:
            return get_million_variant("mi", last_digit);
        case 2:
            return get_milliard_variant("mi", last_digit);
        case 3:
            return get_million_variant("bi", last_digit);
        case 4:
            return get_milliard_variant("bi", last_digit);
        case 5:
            return get_million_variant("tri", last_digit);
        case 6:
            return get_milliard_variant("tri", last_digit);
        case 7:
            return get_million_variant("kvadri", last_digit);
        case 8:
            return get_milliard_variant("kvadri", last_digit);
        case 9:
            return get_million_variant("kvinti", last_digit);
        case 10:
            return get_milliard_variant("kvinti", last_digit);
        case 11:
            return get_million_variant("seksti", last_digit);
        case 12:
            return get_million_variant("septi", last_digit);
        case 13:
            return get_million_variant("okti", last_digit);
        case 14:
            return get_million_variant("noni", last_digit);
        case 15:
            return get_million_variant("deci", last_digit);
        case 16:
            return get_million_variant("undeci", last_digit);
        case 17:
            return get_million_variant("duodeci", last_digit);
        case 18:
            return get_million_variant("centi", last_digit);
        default:
            return "";
    }
}

// =============================================================================
// Gender of the Scale Words
// =============================================================================

bool CroatianNumbers::is_feminine_scale(int group_index) {
    // tisuća/hiljada and the words in -ilijarda are feminine, the words in
    // -ilijun/-ilion masculine (see get_large_number_suffix). Slovenian
    // tisoč is masculine (en tisoč, dva tisoč).
    if (group_index == 0) return !slovenian();
    return group_index >= 2 && group_index <= 10 && group_index % 2 == 0;
}

std::string_view CroatianNumbers::zero_word() const {
    return slovenian() ? std::string_view(u8"nič") : std::string_view("nula");
}

// The unit of a Slovenian compound numeral: enaindvajset, dvaindvajset,
// triindvajset, štiriindvajset ... (Slovenski pravopis 2001, § 536)
std::string_view CroatianNumbers::slovenian_compound_unit(char digit) {
    switch (digit) {
        case '1': return "ena";
        case '2': return "dva";
        case '3': return "tri";
        case '4': return u8"štiri";
        case '5': return "pet";
        case '6': return u8"šest";
        case '7': return "sedem";
        case '8': return "osem";
        case '9': return "devet";
        default: return "";
    }
}

namespace {

bool sl_ends(std::string_view w, std::string_view tail) {
    return w.size() >= tail.size() && w.compare(w.size() - tail.size(), tail.size(), tail) == 0;
}
bool sl_starts(std::string_view w, std::string_view head) {
    return w.size() >= head.size() && w.compare(0, head.size(), head) == 0;
}

// The stem of a Slovenian ordinal from the last word of the cardinal (SP
// 2001: pŕvi, drúgi, trétji -a -e, četŕti, pêti, šêsti, sêdmi, ôsmi, devêti,
// desêti, enájsti, dvájseti, ênaindvájseti, stôti, dvéstoti, tisóči -a -e,
// milijónti). Only the last word of a longer number changes: sto prvi,
// tisoč devetsto enaindevetdeseti, dva tisoči.
std::string sl_ordinal_stem(std::string_view token) {
    if (token == "ena") return "prv";
    if (token == "dva") return "drug";
    if (token == "tri") return "tretj";
    if (token == u8"štiri") return u8"četrt";
    if (token == "sedem") return "sedm";
    if (token == "osem") return "osm";
    if (sl_ends(token, "sto")) return std::string(token) + "t";
    if (sl_starts(token, "milijon")) return "milijont";
    if (sl_starts(token, "milijard")) return "milijardt";
    return std::string(token);      // pet, šest, devet, deset, the teens, the tens, the compounds, tisoč
}

} // namespace

// A Slovenian ordinal with the adjectival ending the sentence asks for:
// "ega" of a date (sedmega oktobra, enaintridesetega), "o" of prvo mesto,
// "em" of na prvem mestu, "ih" of v devetdesetih letih. The soft stems
// (tretj-, tisoč-) take -e for the -o of the neuter (tretje mesto, SP 2001
// trétji -a -e; tisóči -a -e).
std::string CroatianNumbers::slovenian_ordinal(std::string_view number, std::string_view ending) {
    number = remove_leading_zeros(number);
    if (number.empty()) return "";
    std::string words = number_to_words(number);
    size_t token = words.rfind(' ');
    token = token == std::string::npos ? 0 : token + 1;
    std::string stem = sl_ordinal_stem(std::string_view(words).substr(token));
    std::string end(ending);
    if (end == "o") {
        const char last = stem.back();
        if (last == 'j' || last == 'c' || sl_ends(stem, u8"č") || sl_ends(stem, u8"š") || sl_ends(stem, u8"ž")) end = "e";
    }
    words.replace(token, std::string::npos, stem + end);
    return words;
}

// A Slovenian decimal: the whole part, "cela" agreeing with it (ena cela,
// dve celi, tri cele, štiri cele, pet celih, nič celih, enaindvajset celih:
// SSKJ "ena cela, pet stotink"; the compounds like every number from five
// up), then the digits after the comma as one number, leading zeros each
// as nič (nič celih nič ena: SSKJ)
std::string CroatianNumbers::slovenian_decimal(std::string_view whole, std::string_view fraction) {
    std::string words = number_to_words(whole);
    std::string_view clean = remove_leading_zeros(whole);
    const bool ones_alone = clean.size() == 1 || clean[clean.size() - 2] == '0';
    const char last = clean.back();
    feminine_two(words);        // dve celi
    if (ones_alone && last == '1') words += " cela";
    else if (ones_alone && last == '2') words += " celi";
    else if (ones_alone && (last == '3' || last == '4')) words += " cele";
    else words += " celih";
    words += ' ';
    std::string decimals = digit_group_to_words(fraction);
    feminine_two(decimals);     // tri cele dve: dve desetinki
    words += decimals;
    return words;
}

// A Slovenian number that ends in a 2 of its own (dva, sto dva, dva tisoč
// dva; not dvaindvajset, dvanajst) in the feminine.
void CroatianNumbers::feminine_two(std::string& words) {
    const size_t n = words.size();
    if (n >= 3 && words.compare(n - 3, 3, "dva") == 0 && (n == 3 || words[n - 4] == ' ')) {
        words[n - 1] = 'e';
    }
}

// =============================================================================
// Remove Leading Zeros
// =============================================================================

std::string_view CroatianNumbers::remove_leading_zeros(std::string_view number) {
    size_t pos = 0;
    while (pos < number.size() - 1 && number[pos] == '0') {
        pos++;
    }
    return number.substr(pos);
}

// =============================================================================
// Validate Number String
// =============================================================================

bool CroatianNumbers::is_valid_number(std::string_view str) {
    if (str.empty()) return false;
    for (char c : str) {
        if (c < '0' || c > '9') return false;
    }
    return true;
}

// =============================================================================
// Process Number into Groups and Convert
// =============================================================================

std::string CroatianNumbers::process_number_groups(std::string_view number) {
    if (!is_valid_number(number)) return "";

    // Remove leading zeros
    number = remove_leading_zeros(number);

    // Handle zero
    if (number == "0") {
        return std::string(zero_word());
    }

    size_t length = number.size();
    int num_groups = static_cast<int>((length + 2) / 3);  // Ceiling division

    std::string result;

    for (int group_num = 0; group_num < num_groups; group_num++) {
        // Calculate group bounds
        size_t group_end = length - (static_cast<size_t>(num_groups) - 1 - group_num) * 3;
        size_t group_start;
        size_t group_len;

        if (group_num == 0) {
            // First group may have 1, 2, or 3 digits
            group_start = 0;
            group_len = length - (static_cast<size_t>(num_groups) - 1) * 3;
        } else {
            group_start = group_end - 3;
            group_len = 3;
        }

        std::string_view group_str = number.substr(group_start, group_len);
        std::string_view group_clean = remove_leading_zeros(group_str);

        // Skip zero groups (but not the last one if it's the only group)
        if (group_clean == "0" || group_clean.empty()) {
            continue;
        }

        int groups_from_end = num_groups - 1 - group_num;
        char last_digit = group_clean.back();
        // 11-19 have no "one" or "two" of their own: jedanaest tisuća
        bool is_teen = group_clean.size() >= 2 && group_clean[group_clean.size() - 2] == '1';
        // In Slovenian the ones are a word of their own only after a zero
        // tens digit (sto en milijon, tisoč dva milijona); from 21 up they
        // are one word with the tens (enaindvajset), which takes the
        // genitive plural like every compound: enaindvajset milijonov.
        const bool ones_alone = group_clean.size() == 1 || group_clean[group_clean.size() - 2] == '0';

        // Special case: group is exactly "1" for thousands and higher
        // In Croatian, you say "tisuću" not "jedan tisuću" for 1000
        bool is_one = (group_clean == "1");

        // Convert group to words (unless it's "1" for thousands+)
        if (!is_one || groups_from_end == 0) {
            if (!result.empty()) {
                result += " ";
            }
            std::string words = group_to_words(group_clean);
            if (slovenian()) {
                // One to four agree with the scale word: en milijon, dva
                // milijona, trije milijoni, štirje milijoni; ena milijarda,
                // dve milijardi, tri milijarde, štiri milijarde; en tisoč,
                // dva tisoč, tri tisoč, štiri tisoč
                if (groups_from_end > 0 && ones_alone && last_digit >= '1' && last_digit <= '4') {
                    const bool feminine = is_feminine_scale(groups_from_end - 1);
                    std::string_view counting = digit_to_word(last_digit);
                    words.resize(words.size() - counting.size());
                    const bool thousand = groups_from_end == 1;
                    switch (last_digit) {
                        case '1': words += feminine ? "ena" : "en"; break;
                        case '2': words += feminine ? "dve" : "dva"; break;
                        case '3': words += (feminine || thousand) ? "tri" : "trije"; break;
                        default: words += (feminine || thousand) ? u8"štiri" : u8"štirje"; break;
                    }
                }
            } else if (groups_from_end > 0 && is_feminine_scale(groups_from_end - 1) && !is_teen &&
                       (last_digit == '1' || last_digit == '2')) {
                // The numbers one and two agree with a feminine scale word:
                // dvije tisuće, dve hiljade, dvadeset jedna tisuća, dvije milijarde
                std::string_view masculine = digit_to_word(last_digit);
                words.resize(words.size() - masculine.size());
                words += last_digit == '1' ? "jedna"
                       : (m_dialect == Dialect::Serbian ? "dve" : "dvije");
            }
            result += words;
        }

        // Add scale word (thousand, million, etc.)
        if (groups_from_end > 0) {
            // Determine which digit to use for plural form
            // 11-19 take the genitive plural like 5-9 (dvanaest tisuća).
            // A group ending in 1 takes the singular: "dvadeset jedan
            // milijun", "dvadeset jedna milijarda". For thousands that is
            // the nominative "tisuća", spelled like the genitive plural
            // ('1' gives the accusative "tisuću" used for 1000 itself).
            char plural_digit = last_digit;
            if (is_teen || (!is_one && last_digit == '1' && groups_from_end == 1)) {
                plural_digit = '0';
            }
            if (slovenian() && !ones_alone) {
                plural_digit = '0';     // enaindvajset milijonov, dvainštirideset milijard
            }

            std::string suffix = get_large_number_suffix(groups_from_end - 1, plural_digit);
            if (!suffix.empty()) {
                if (!result.empty()) {
                    result += " ";
                }
                result += suffix;
            }
        }
    }

    return result;
}

// =============================================================================
// Number to Words (main function for single number)
// =============================================================================

std::string CroatianNumbers::number_to_words(std::string_view number_str) {
    return process_number_groups(number_str);
}

// =============================================================================
// Convert Numbers in Text (main entry point)
// =============================================================================

namespace {

bool is_digit(char c) { return c >= '0' && c <= '9'; }

// A period or comma glued between two digits: "3.14", "3,14", "192.168.1.1".
bool is_separator_between_digits(const std::string& text, size_t i) {
    return i > 0 && i + 1 < text.size() && (text[i] == '.' || text[i] == ',') &&
           is_digit(text[i - 1]) && is_digit(text[i + 1]);
}

size_t digits_end(const std::string& text, size_t i) {
    while (i < text.size() && is_digit(text[i])) ++i;
    return i;
}

// A letter or digit, as far as the number converter needs to know: any
// non-ASCII byte counts as part of a word.
bool is_word_byte(char c) {
    unsigned char b = static_cast<unsigned char>(c);
    return b >= 0x80 || std::isalnum(b);
}

// The code point at position i of the UTF-8 text (0 at the end or at an
// invalid byte)
char32_t code_point_at(const std::string& text, size_t i) {
    if (i >= text.size()) return 0;
    unsigned char b = static_cast<unsigned char>(text[i]);
    size_t len = b < 0x80 ? 1 : (b & 0xE0) == 0xC0 ? 2 : (b & 0xF0) == 0xE0 ? 3 : (b & 0xF8) == 0xF0 ? 4 : 0;
    if (len == 0 || i + len > text.size()) return 0;
    char32_t c = len == 1 ? b : b & (0x7F >> len);
    for (size_t k = 1; k < len; ++k) {
        unsigned char cont = static_cast<unsigned char>(text[i + k]);
        if ((cont & 0xC0) != 0x80) return 0;
        c = (c << 6) | (cont & 0x3F);
    }
    return c;
}

// A lowercase letter of the Latin (ASCII, Latin-1, Latin Extended-A) or
// Cyrillic alphabet
bool is_lowercase_letter(char32_t c) {
    if (c >= U'a' && c <= U'z') return true;
    if (c >= 0xDF && c <= 0xFF) return c != 0xF7;                   // ß à-ÿ
    if (c >= 0x100 && c <= 0x137) return (c & 1) != 0;               // ā ă ą ć č đ ... ķ
    if (c == 0x138) return true;                                     // ĸ
    if (c >= 0x139 && c <= 0x148) return (c & 1) == 0;               // ĺ ļ ľ ŀ ł ń ņ ň
    if (c >= 0x149 && c <= 0x177) return (c & 1) != 0;               // ŉ ŋ ō ... š ... ŷ
    if (c == 0x17A || c == 0x17C || c == 0x17E || c == 0x17F) return true;  // ź ż ž ſ
    if (c >= 0x430 && c <= 0x45F) return true;                       // а-я ѐ-џ
    return false;
}

// Value of a group of at most 4 digits
int group_value(std::string_view group) {
    int value = 0;
    for (char c : group) value = value * 10 + (c - '0');
    return value;
}

// A clock time at position i: h or hh, a colon, exactly two minute digits,
// optionally a colon and two second digits, and no digit after that
// ("12:30", "9:05", "12:30:45"). Fills the groups and returns the end.
size_t parse_time(const std::string& text, size_t i, std::vector<std::string_view>& groups) {
    groups.clear();
    size_t hours_end = digits_end(text, i);
    size_t hours_len = hours_end - i;
    if (hours_len < 1 || hours_len > 2) return 0;
    if (group_value(std::string_view(text.data() + i, hours_len)) > 23) return 0;
    size_t pos = hours_end;
    for (int part = 0; part < 2; ++part) {
        if (pos >= text.size() || text[pos] != ':') break;
        size_t start = pos + 1;
        size_t end = digits_end(text, start);
        if (end - start != 2) return part == 0 ? 0 : pos;
        if (group_value(std::string_view(text.data() + start, 2)) > 59) return part == 0 ? 0 : pos;
        if (part == 0) groups.push_back(std::string_view(text.data() + i, hours_len));
        groups.push_back(std::string_view(text.data() + start, 2));
        pos = end;
    }
    return groups.empty() ? 0 : pos;
}

// A date at position i: day 1-31, a period, month 1-12, a period, a year
// of two or four digits, and no digit after that ("7.10.2026", "07.10.26").
// One space may follow each period, as the orthography writes it
// ("7. 10. 2026."). A period after the year is left in the text: it ends
// the sentence or the clause as usual.
size_t parse_date(const std::string& text, size_t i, std::vector<std::string_view>& groups) {
    groups.clear();
    size_t pos = i;
    for (int part = 0; part < 3; ++part) {
        size_t end = digits_end(text, pos);
        std::string_view group(text.data() + pos, end - pos);
        int value = group_value(group);
        if (part == 0 && (group.size() > 2 || value < 1 || value > 31)) return 0;
        if (part == 1 && (group.size() > 2 || value < 1 || value > 12)) return 0;
        if (part == 2 && group.size() != 2 && group.size() != 4) return 0;
        groups.push_back(group);
        pos = end;
        if (part < 2) {
            if (pos >= text.size() || text[pos] != '.') return 0;
            ++pos;
            if (pos < text.size() && text[pos] == ' ') ++pos;
            if (pos >= text.size() || !is_digit(text[pos])) return 0;
        }
    }
    return pos;
}

} // namespace

// A number of up to four digits without a leading zero, standing at the
// start of a word and followed by a period and a lowercase word, is an
// ordinal ("7. listopada", "u 19. stoljeću", "1990. godine"). Its words
// would need the case of the noun (sedmog listopada, devetnaestom
// stoljeću), which the converter cannot know, so it is read as the
// cardinal (sedam listopada). The period is silent and does not end the
// clause. Before an uppercase word the period ends the sentence as usual
// ("Ima ih 7. Sutra ..."); eSpeak draws the same line. Returns the position
// after the period, 0 when the number is not such an ordinal.
size_t CroatianNumbers::silent_dot_end(const std::string& text, size_t start, size_t end) {
    if (end - start > 4 || text[start] == '0') return 0;
    if (start > 0 && is_word_byte(text[start - 1])) return 0;
    if (end >= text.size() || text[end] != '.') return 0;
    size_t next = end + 1;
    while (next < text.size() && (text[next] == ' ' || text[next] == '\t')) ++next;
    return is_lowercase_letter(code_point_at(text, next)) ? end + 1 : 0;
}

// "12:30" -> dvanaest trideset, "9:05" -> devet nula pet
std::string CroatianNumbers::time_to_words(const std::vector<std::string_view>& groups) {
    std::string words;
    for (size_t k = 0; k < groups.size(); ++k) {
        if (k > 0) words += ' ';
        std::string part = k == 0 ? number_to_words(groups[k]) : digit_group_to_words(groups[k]);
        if (slovenian()) feminine_two(part);    // the hour and the minute: dve trideset
        words += part;
    }
    return words;
}

// "7.10.2026" -> sedam deset dvije tisuće dvadeset šest. The day and month
// are cardinals for the same reason as "7. listopada" above. Slovenian
// reads a date in the genitive, which does not depend on the sentence:
// sedmega desetega dva tisoč šestindvajset (Jezikovna svetovalnica, ZRC
// SAZU: "šestega julija dva tisoč deset").
std::string CroatianNumbers::date_to_words(const std::vector<std::string_view>& groups) {
    if (slovenian()) {
        return slovenian_ordinal(groups[0], "ega") + ' ' + slovenian_ordinal(groups[1], "ega") +
               ' ' + number_to_words(groups[2]);
    }
    return hr_ordinal(groups[0], "og") + ' ' + hr_ordinal(groups[1], "og") + ' ' + hr_ordinal(groups[2], "e");
}

// A time or date at position i, read without the separators; returns the
// end of what was consumed, 0 when there is none.
size_t CroatianNumbers::convert_time_or_date(const std::string& text, size_t i, std::string& result) {
    std::vector<std::string_view> groups;
    if (size_t end = parse_time(text, i, groups)) {
        result += time_to_words(groups);
        return end;
    }
    if (size_t end = parse_date(text, i, groups)) {
        result += date_to_words(groups);
        return end;
    }
    return 0;
}

// Words for a group of digits: every leading zero is "nula", the rest is
// one number ("007" -> "nula nula sedam", "000" -> "nula nula nula").
std::string CroatianNumbers::digit_group_to_words(std::string_view group) {
    if (group.size() == 1) return std::string(digit_name(group[0]));    // 3.2: tri pika dve
    std::string words;
    while (group.size() > 1 && group[0] == '0') {
        words += zero_word();
        words += ' ';
        group.remove_prefix(1);
    }
    words += number_to_words(group);
    return words;
}

// The separator itself, as the front end will read it. A period between
// digits becomes the one dot leader (U+2024) glued to the digits on both
// sides: InflectionProcessor does not know it, so it stays inside the
// clause, and the front end names it in the voice's language (točka /
// tačka). A period itself would be the end of a clause there, as every
// period in the text that is not a decimal point or part of a file name
// now is. The decimal comma is "zarez" in every language, so the word is
// written here; a comma in the text would otherwise be a clause
// break. Slovenian reads the decimal comma as "vejica" (and the front end
// names the dot leader "pika").
std::string CroatianNumbers::separator_words(char separator) const {
    if (separator == '.') return "\xE2\x80\xA4";
    return slovenian() ? " vejica " : " zarez ";
}

namespace {

bool is_letter_byte(char c) {
    unsigned char u = static_cast<unsigned char>(c);
    return std::isalpha(u) || u >= 0x80;
}

// A number glued to letters ("u20:03", "5kg", "COVID19") is set off from
// them, or the letters would be read as part of the number word
// ("udvadeset", stressed on the u). When the number is a time, a date or
// a decimal (compound), a period that joins a word to the letter run
// ("listopada.u20:03", the way a screen reader glues the parts of a
// label) is a word break, not a period to read by name; before a plain
// number it stays, for the file extensions "pjesma.mp3", "film.mp4".
void set_off_from_letters(std::string& result, bool compound) {
    if (result.empty() || !is_letter_byte(result.back())) return;
    size_t k = result.size();
    while (k > 0 && is_letter_byte(result[k - 1])) --k;
    if (compound && k >= 2 && result[k - 1] == '.' && is_letter_byte(result[k - 2])) {
        result[k - 1] = ' ';
    }
    result += ' ';
}

// A comma between two single digits of a spaced-out digit string is a
// grouping mark, not a decimal comma: screen readers write a phone number
// digit by digit, with a comma where a group ends ("+ 1 2 3,5 6,7 8 9,8 7 6").
// Such a string is a run of single digits, each separated from the next by
// one space or a glued comma, with at least six digits and at least two of
// them touching no comma, so that a list of decimals ("4,5 3,5 5,0") stays
// a list of decimals.
bool is_grouping_comma(const std::string& text, size_t i) {
    if (!is_separator_between_digits(text, i) || text[i] != ',') return false;
    auto single = [&](size_t d) {
        return (d == 0 || !is_digit(text[d - 1])) && (d + 1 >= text.size() || !is_digit(text[d + 1]));
    };
    auto linked = [&](size_t d) {   // the digit at d is followed by one at d + 2
        return d + 2 < text.size() && (text[d + 1] == ' ' || text[d + 1] == ',') &&
               is_digit(text[d + 2]) && single(d + 2);
    };
    if (!single(i - 1) || !single(i + 1)) return false;

    size_t first = i - 1;
    while (first >= 2 && is_digit(text[first - 2]) && single(first - 2) && linked(first - 2)) {
        first -= 2;
    }
    size_t digits = 0, lone = 0;
    for (size_t d = first;; d += 2) {
        ++digits;
        bool comma = (d > first && text[d - 1] == ',') || (d + 1 < text.size() && text[d + 1] == ',');
        if (!comma) ++lone;
        if (!linked(d)) break;
    }
    return digits >= 6 && lone >= 2;
}

} // namespace

namespace {

// The prepositions that govern a case (SSKJ², Slovenski pravopis 2001),
// shared by the agreement of 1 to 4, of 5 and above, and of the ordinals
const std::unordered_set<std::string_view> SL_WITH_GENITIVE = {
    "od", "do", "iz", "brez", "zaradi", "okoli", "okrog", "blizu", "izmed", "izpod", "izza", "razen",
    "sredi", "glede", "mimo", "poleg", "znotraj", "zunaj",
    };
const std::unordered_set<std::string_view> SL_WITH_LOCATIVE = {"v", "na", "o", "ob", "pri", "po"};
const std::unordered_set<std::string_view> SL_WITH_INSTRUMENTAL = {"z", "s", "pred", "med", "nad", "pod", "za"};
const std::unordered_set<std::string_view> SL_WITH_DATIVE = {"k", "h", "proti", "kljub"};

// The words after which a number counts nothing (2 plus 2, 2 je sodo), and
// the commonest verbs
const std::unordered_set<std::string_view> SL_NOT_COUNTED = {
    // Function words and adverbs
    "tudi", "pri", "ali", "vse", "zaradi", "proti", "glede", "niti", "vsi", "okoli", "lani",
    u8"šele", "nikoli", "hkrati", "skozi", "zlasti", u8"drugače", "bolje", "pozneje", "kasneje",
    "vendarle", "kajti", "sredi", "kje", "radi", "nekateri", "kateri", "katere", "kakor", "meni",
    "plus", "minus", "deljeno",     // 2 plus 2
    "sebi", "sebe", "mene", "nje", "tri", u8"štiri", "dve", "obe", "dva", "ena", "bodisi", "jutri", "potem",
    "kot", "ker", "pod", "nad", "pred", "med", "brez", "sem", "smo", "bom", "nam", "vam", "jih", "jim",
    "tem", "teh", "kar", "kaj", "kdo", "kako", "ter", "bodo", "krat", "lahko", "tako", "zato", "samo",
    "zelo", "malo", "veliko", "dobro", "hitro", "vedno", "nato", "danes", u8"tisoč",
    // The commonest verbs
    "bili", "bile", "biti", "boste", "ste", "imeli", "imate", "ima", "imajo", "morali", u8"začeli", "dobili",
    "gre", "pravi", "more", u8"kaže", "zdi", u8"želi", u8"prišli", "pride", "deluje", u8"piše",
    u8"živi", u8"začne", "govori", "potrebuje", "zgodi", "daje", "sme", "pove", "postane", "vidi",
    "sodi", u8"pričakuje", "vsebuje", u8"drži", "dobi", "pomeni",
    };

// The feminine i-stems, the commonest of Sloleks (without -ost, -tev),
// singular and plural: their consonant and their -i look masculine
const std::unordered_set<std::string_view> SL_FEMININE_I = {
    "stvar", "oblast", u8"žival", "pesem", "napoved", "misel", "jed", "bolezen", u8"luč", "kazen", "snov",
    "stran", "pot", "vez", u8"reč", "skrb", u8"moč", "cev", "sled", "smet", "zamisel", "raven", u8"smuč",
    "strast", u8"laž", "vas", "plesen", "pripoved", "prepoved", u8"kokoš", u8"uš", "plat", "dlan", "plast",
    "smer", "korist", "odpoved", "klet", "smrt", u8"noč", "plavut", "kopel", "vajet", "klop", "izpoved",
    "vest", "obrt", u8"peč", "vrzel", "bojazen", u8"miš", "rast", "past", u8"pošast", u8"četrt", "zapoved",
    "obrv", "ljubezen", u8"pomoč", "pamet", "sol", "vrv", "zver", "gos", u8"utež", "vzmet", "dlesen",
    u8"pešpot", u8"piščal", "zvrst", "perut", u8"čeljust", "kap", "pest", "prikazen", "gred", "nit", "pomlad",
    "predjed", "basen", "kad", "jesen", "posest", u8"čast", "zmrzal", "prst", "povest", "zavest", "mast",
    "podstran", "kost",
    "stvari", "oblasti", u8"živali", "pesmi", "napovedi", "misli", "jedi", "bolezni", u8"luči", "kazni",
    "snovi", "strani", "poti", "vezi", u8"reči", "skrbi", u8"moči", "cevi", "sledi", "smeti", "zamisli",
    "ravni", u8"smuči", "strasti", u8"laži", "vasi", "plesni", "pripovedi", "prepovedi", u8"kokoši", u8"uši",
    "plati", "dlani", "plasti", "smeri", "koristi", "odpovedi", "kleti", "smrti", u8"noči", "plavuti",
    "kopeli", "vajeti", "klopi", "izpovedi", "vesti", "obrti", u8"peči", "vrzeli", "bojazni", u8"miši",
    "rasti", "pasti", u8"pošasti", u8"četrti", "zapovedi", "obrvi", "ljubezni", u8"pomoči", "pameti", "soli",
    "vrvi", "zveri", "gosi", u8"uteži", "vzmeti", "dlesni", u8"pešpoti", u8"piščali", "zvrsti", "peruti",
    u8"čeljusti", "kapi", "pesti", "prikazni", "gredi", "niti", "pomladi", "predjedi", "basni", "kadi",
    "jeseni", "posesti", u8"časti", "zmrzali", "prsti", "povesti", "zavesti", "masti", "podstrani", "kosti",
    u8"oči", "prsi", "sani", "jasli",
    };

// Units written as abbreviations, by the gender of their noun: kilogram,
// meter, liter, megabajt, herc, vat are masculine, minuta, ura, sekunda,
// tona feminine (SSKJ²). Compared in small letters (MB, kW).
const std::unordered_set<std::string_view> SL_MASCULINE_UNITS = {
    "m", "cm", "mm", "dm", "km", "kg", "g", "dag", "mg", "l", "dl", "cl", "ml", "hl", "kb", "mb", "gb", "tb", "pb",
    "hz", "khz", "mhz", "ghz", "w", "kw", "mw", "kwh", "mwh", "bar", "mbar", "px", "dpi", "fps", "bit", "bajt",
    u8"tisoč",
};
const std::unordered_set<std::string_view> SL_FEMININE_UNITS = {"h", "min", "t", "s", "sek", "ms"};

const char* const SL_MONTHS_GENITIVE[] = {
    "januarja", "februarja", "marca", "aprila", "maja", "junija", "julija", "avgusta", "septembra", "oktobra",
    "novembra", "decembra",
};

// The hours after ob, in the locative plural (SP 2001 pét: "priti ob petih
// |ob 5. uri; ob 5h; ob 5.00; ob 17. uri; ob 17h; ob 17.00|"; SSKJ² "vstajati
// ob petih zjutraj")
const char* const SL_HOURS_LOCATIVE[] = {
    "", "enih", "dveh", "treh", u8"štirih", "petih", u8"šestih", "sedmih", "osmih", "devetih", "desetih",
    "enajstih", "dvanajstih", "trinajstih", u8"štirinajstih", "petnajstih", u8"šestnajstih", "sedemnajstih",
    "osemnajstih", "devetnajstih", "dvajsetih", "enaindvajsetih", "dvaindvajsetih", "triindvajsetih",
    u8"štiriindvajsetih",
};

bool sl_is_month_genitive(std::string_view w) {
    for (const char* m : SL_MONTHS_GENITIVE) {
        if (w == m) return true;
    }
    return false;
}

bool sl_feminine_i(std::string_view next) {
    return SL_FEMININE_I.count(next) != 0 || (sl_ends(next, "ost") && next.size() > 5) ||
           (sl_ends(next, "osti") && next.size() > 6) || sl_ends(next, "tev");
}

// A cardinal of five and above as its last word: pet ... deset, the teens,
// the tens and the compounds (enaindvajset); not sto, tisoč, milijon
bool sl_five_plus(std::string_view w) {
    static const std::unordered_set<std::string_view> base = {"pet", u8"šest", "sedem", "osem", "devet", "deset"};
    return base.count(w) != 0 || sl_ends(w, "najst") || sl_ends(w, "deset") || sl_ends(w, "dvajset");
}

std::string sl_ascii_lower(std::string_view w) {
    std::string out(w);
    for (char& c : out) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return out;
}

// Lowercases an initial capital (ASCII or Č Ć Đ Š Ž); true when there was one
bool sl_lowercase_first(std::string& w) {
    if (w.empty()) return false;
    const unsigned char c0 = static_cast<unsigned char>(w[0]);
    const unsigned char c1 = w.size() > 1 ? static_cast<unsigned char>(w[1]) : 0;
    if (c0 >= 'A' && c0 <= 'Z') {
        w[0] = static_cast<char>(c0 - 'A' + 'a');
        return true;
    }
    if ((c0 == 0xC4 && (c1 == 0x8C || c1 == 0x86 || c1 == 0x90)) || (c0 == 0xC5 && (c1 == 0xA0 || c1 == 0xBD))) {
        w[1] = static_cast<char>(c1 + 1);     // the small letter is the next code point
        return true;
    }
    return false;
}

// Spaces between a number and what belongs to it: space, tab, no-break
// space, narrow no-break space, thin space
size_t sl_skip_spaces(const std::string& text, size_t pos) {
    for (;;) {
        if (pos < text.size() && (text[pos] == ' ' || text[pos] == '\t')) { ++pos; continue; }
        if (pos + 1 < text.size() && static_cast<unsigned char>(text[pos]) == 0xC2 &&
            static_cast<unsigned char>(text[pos + 1]) == 0xA0) { pos += 2; continue; }
        if (pos + 2 < text.size() && static_cast<unsigned char>(text[pos]) == 0xE2 &&
            static_cast<unsigned char>(text[pos + 1]) == 0x80 &&
            (static_cast<unsigned char>(text[pos + 2]) == 0xAF || static_cast<unsigned char>(text[pos + 2]) == 0x89)) {
            pos += 3;
            continue;
        }
        return pos;
    }
}

// The word before position start, with only spaces between it and the
// number, its first letter lowercased; `ahead` is the end of what stands
// before that word
struct SlPrev {
    std::string word;
    bool capital = false;
    size_t begin = 0;
    size_t end = 0;
    size_t ahead = 0;
};

SlPrev sl_prev_word(const std::string& text, size_t start) {
    SlPrev p;
    size_t before = start;
    while (before > 0 && (text[before - 1] == ' ' || text[before - 1] == '\t')) --before;
    size_t begin = before;
    while (begin > 0 && is_word_byte(text[begin - 1])) --begin;
    p.word = text.substr(begin, before - begin);
    p.begin = begin;
    p.end = before;
    p.ahead = begin;
    while (p.ahead > 0 && (text[p.ahead - 1] == ' ' || text[p.ahead - 1] == '\t')) --p.ahead;
    p.capital = sl_lowercase_first(p.word);
    return p;
}

// The word after position pos (spaces skipped), as written
struct SlNext {
    std::string word;
    size_t begin = 0;
    size_t end = 0;
    bool lowercase = false;
};

SlNext sl_next_word(const std::string& text, size_t pos) {
    SlNext n;
    n.begin = sl_skip_spaces(text, pos);
    n.end = n.begin;
    while (n.end < text.size() && is_word_byte(text[n.end])) ++n.end;
    n.word = text.substr(n.begin, n.end - n.begin);
    n.lowercase = is_lowercase_letter(code_point_at(text, n.begin));
    return n;
}

// The ending of an ordinal before the word `next`, after the word `prev`
// (both in small letters): the adjectival endings as the noun's case,
// gender and number show them. "" keeps the cardinal (nothing counted:
// 2. je, 2. deluje).
std::string_view sl_ordinal_ending(const std::string& prev, std::string_view next) {
    if (sl_is_month_genitive(next)) return "ega";                   // sedmega oktobra
    if (next.size() < 3) return "";
    if (SL_NOT_COUNTED.count(next) || sl_ends(next, "uje") || sl_ends(next, "jte")) return "";
    const bool genitive = SL_WITH_GENITIVE.count(prev) != 0 || prev == "z" || prev == "s";
    const bool dative = SL_WITH_DATIVE.count(prev) != 0;
    const bool instrumental = SL_WITH_INSTRUMENTAL.count(prev) != 0;
    if (sl_ends(next, "ega")) return "ega";
    if (sl_ends(next, "emu")) return "emu";
    if (sl_ends(next, "ima") || sl_ends(next, "ema") || sl_ends(next, "oma")) return "ima";
    if (sl_ends(next, "imi") || sl_ends(next, "ami")) return "imi";                        // s prvimi koraki
    if (sl_ends(next, "ih") || sl_ends(next, "ah") || sl_ends(next, "eh") || sl_ends(next, "ov") || sl_ends(next, "ev")) {
        return "ih";                                                                        // v devetdesetih letih
    }
    if (sl_ends(next, "om") || (sl_ends(next, "em") && instrumental)) return "im";         // pred prvim letom
    if (next == "let" || next == "ur" || next == "dni") return "ih";                        // konec 80. let
    // After a word that is not a preposition, a copula, a conjunction or a
    // particle the phrase in -a is a genitive attribute (odstavka 3. člena,
    // začetek 2. tisočletja, pari 3. kola); at the head of the clause or after
    // je, so, in, pa ... the feminine nominative (1. nagrada, 2. liga, 80.
    // leta, 100. obletnica)
    static const std::unordered_set<std::string_view> before_subject = {
        "je", "so", "bo", "bodo", "bil", "bila", "bilo", "bili", "ni", "in", "ali", "pa", "ter", "tudi", u8"še", "kot",
        "to", "ta", "le", "samo", "kar", "sicer", "torej", "npr", "oziroma", "zdaj", "danes", u8"že", u8"šele",
    };
    switch (next.back()) {
        case 'a':
            if (genitive) return "ega";                                             // do tretjega razreda
            return prev.empty() || before_subject.count(prev) ? "a" : "ega";        // prva stran; odstavka tretjega člena
        case 'e': return (sl_ends(next, "je") && !sl_ends(next, "ije")) ? "o" : "e";    // tretje nadstropje; prve knjige
        case 'o': return "o";                                       // prvo mesto
        case 'u': return dative ? "emu" : "em";                     // k prvemu členu; na prvem mestu
        case 'i': return "i";                                       // na prvi strani, ob peti uri, prvi koraki
        default: return sl_feminine_i(next) ? "a" : "i";            // prva stran; tretji člen, sedmi oktober
    }
}

// The stem of a multiplicative (enkrat, dvakrat, trikrat, štirikrat,
// petkrat, stokrat: SP 2001 ênkrat, dvákrat) and of a compound with a
// number (enosobno, dvoletni, triletna, štiriletni, petdnevni,
// desetletnica, dvajsetletnik, stoletnica: SP dvóléten, desétlétnica),
// from the digits; a longer number is written as one word (stopetdeset-)
std::string sl_number_stem(std::string_view digits, std::string words, bool multiplicative) {
    if (digits == "1") return multiplicative ? "en" : "eno";
    if (digits == "2") return multiplicative ? "dva" : "dvo";
    words.erase(std::remove(words.begin(), words.end(), ' '), words.end());
    return words;
}

} // namespace

// A Slovenian 1, 2, 3 or 4 at the end of the number in [start, end) of the
// text agrees with what it counts (dvá dvé dvé, dvéh, dvéma; tríje trí, tréh,
// trém, trémi; štírje štíri, štírih, štírim, štírimi; èn êna êno: SSKJ²,
// Slovenski pravopis 2001). Asked for by a
// Slovenian listener (2 koša, T2, en koš, trije koši, z dvema žličkama). The
// ending of the next word and the preposition before the number decide; there
// is no parser here, so a word that could be something else leaves the plain
// form (ena, dve, tri, štiri), the names of the numbers:
//   - The number 2 counting nothing is dve. The same listener asked for it where
//     it stands alone; SP 2001, § 11, SSKJ² (ena in ena je dve) and the Jezikovna
//     svetovalnica ("Uporaba dva in dve", 2014) give the rest: 1, 2, 3, 0 1 2,
//     2 plus 2, 2 je sodo, pritisni 2, stran 2, točka 2, slika 2. A feminine or a
//     neuter noun before it asks for dve anyway.
//   - dva stays in what is the name of a thing: glued to letters or after a
//     hyphen or a slash (T2, T-2, PS/2, A-2 liga, 2kg, 2-krat), after another
//     capitalized word (Pentium 2, Voyager 2), after a masculine noun (člen 2,
//     korak 2, monitor 2), before a capital or a sign of a unit (2 %, 2 €).
//   - -ama, -oma, -ema: the dual's instrumental and dative (z dvema žličkama).
//     -ami, -imi: the plural's (s tremi žlicami). -ah, -eh, -ih, -ov, -ev: the
//     locative and genitive (v dveh urah, v treh dneh, dveh mesecev). A word
//     with no ending after a preposition with the genitive (do treh let); -i
//     after z, s, pred, med (pred dvema leti, s tremi otroki); -am, -em after
//     k, h, proti (k trem osebam).
//   - 1 after a preposition: enega, ene (do enega leta, od ene ure), enem, eni
//     (v enem dnevu, v eni uri), enim (pred enim letom), enemu, eni.
//   - Otherwise the nominative and the accusative. 1: -a ena; -o, -e eno (not
//     evro, avto); a consonant or -i en (en koš, en glavni), but ena for the
//     feminine i-stems (ena stvar, ena možnost, ena odločitev). 2: -i, -e dve
//     (dve uri, dve veliki hiši), else dva. 3, 4: -i and the masculines in -je
//     trije, štirje (trije koši, trije ljudje), but tri, štiri for the feminine
//     i-stems (tri stvari) and dni, leti, else tri, štiri (tri koše, tri hiše).
// The hour is feminine (SSKJ² "ura je dve"). On the 209 occurrences of 1 to 4
// before a noun or an adjective in the corpus SUK 1.1 this gives the form the
// noun's gender, number and case ask for in 185, the plain forms in 95; of the
// 906 before anything else it changes 6.
namespace {

bool ends_in_vowel(const std::string& word) {
    const char c = word.back();
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u';
}

} // namespace

void CroatianNumbers::slovenian_agree(const std::string& text, size_t start, size_t end, std::string& words) {
    // The number must end in a word of its own for 1 to 4: dvesto tri, not
    // trinajst or triindvajset
    size_t token = words.rfind(' ');
    token = token == std::string::npos ? 0 : token + 1;
    const std::string_view last = std::string_view(words).substr(token);
    int digit = 0;
    if (last == "ena") digit = 1;
    else if (last == "dva") digit = 2;
    else if (last == "tri") digit = 3;
    else if (last == u8"štiri") digit = 4;
    // 5 and above have their own oblique forms (petih, petimi), below
    const bool five_plus = digit == 0 && sl_five_plus(last);
    if (digit == 0 && !five_plus) return;
    auto say = [&](std::string_view form) { words.replace(token, std::string::npos, form); };
    // Only the number 2 by itself, not sto dva or dva tisoč dva
    auto name_of_number = [&] {
        if (digit == 2 && token == 0) say("dve");
    };

    if (start > 0 && (is_letter_byte(text[start - 1]) || text[start - 1] == '-')) return;

    // The word before the number, with only spaces between, in small letters
    size_t before = start;
    while (before > 0 && (text[before - 1] == ' ' || text[before - 1] == '\t')) --before;
    // T- 2, PS/2, PS/ 2: part of a name written with a hyphen or a slash
    if (before >= 2 && (text[before - 1] == '-' || text[before - 1] == '/') && is_letter_byte(text[before - 2])) return;
    size_t begin = before;
    while (begin > 0 && is_word_byte(text[begin - 1])) --begin;
    std::string prev = text.substr(begin, before - begin);
    size_t ahead = begin;       // the end of what stands before that word
    while (ahead > 0 && (text[ahead - 1] == ' ' || text[ahead - 1] == '\t')) --ahead;
    bool prev_capital = false;
    if (!prev.empty()) {
        const unsigned char c0 = static_cast<unsigned char>(prev[0]);
        const unsigned char c1 = prev.size() > 1 ? static_cast<unsigned char>(prev[1]) : 0;
        if (c0 >= 'A' && c0 <= 'Z') {
            prev_capital = true;
            prev[0] = static_cast<char>(c0 - 'A' + 'a');
        } else if ((c0 == 0xC4 && (c1 == 0x8C || c1 == 0x86 || c1 == 0x90)) || (c0 == 0xC5 && (c1 == 0xA0 || c1 == 0xBD))) {
            prev_capital = true;        // Č Ć Đ Š Ž: the small letter is the next code point
            prev[1] = static_cast<char>(c1 + 1);
        }
    }
    if (prev_capital) {
        // A capital inside a sentence is a name, and the number a part of it
        const char head = ahead > 0 ? text[ahead - 1] : '.';
        if (head != '.' && head != '!' && head != '?' && head != ':' && head != ';' && head != '\n' &&
            head != ',' && head != '(') {
            return;
        }
    }
    if (digit == 2 && prev == "je" && ahead >= 3 && text[ahead - 1] == 'a' && text[ahead - 2] == 'r' &&
        (text[ahead - 3] == 'u' || text[ahead - 3] == 'U') && (ahead == 3 || !is_word_byte(text[ahead - 4]))) {
        say("dve");     // ura je dve
        return;
    }
    static const std::unordered_set<std::string_view> labels = {
        "leta", "leto", "letu", "stran", "strani", "slika", "sliki", "sliko", "tabela", "tabeli", "tabelo",
        "skupina", "skupini", "skupino", u8"člen", u8"členu", u8"točka", u8"točki",
        u8"točko", "odstavek", "odstavku", u8"različica", u8"različici", u8"različico", "verzija", "verziji",
        "kanal", "kanalu", u8"številka", u8"številki", u8"št", "opomba", "opombi", "priloga", "prilogi",
        "faza", "fazi", "korak", "koraku", "element", u8"vrstica", u8"vrstici", "stolpec", "stolpcu",
        "zavihek", "zavihku", "gumb", "gumbu", "datoteka", "datoteki", "mapa", "mapi",
        "stopnja", "stopnji", "nivo", "del", "delu", "tier", "ulica", "ulici", "cesta", "cesti",
    };
    // What is named by the number's name: feminine labels, and the verbs of
    // choosing a number
    static const std::unordered_set<std::string_view> names_the_number = {
        "stran", "strani", "slika", "sliki", "sliko", "tabela", "tabeli", "tabelo", "skupina", "skupini", "skupino",
        u8"točka", u8"točki", u8"točko", u8"različica", u8"različici", u8"različico", "verzija", "verziji",
        u8"številka", u8"številki", u8"številko", u8"št", "opomba", "opombi", "priloga", "prilogi", "faza", "fazi",
        u8"vrstica", u8"vrstici", "datoteka", "datoteki", "mapa", "mapi", "stopnja", "stopnji", "ulica", "ulici",
        "cesta", "cesti", "tipka", "tipko", u8"možnost", "poglavje", "poglavju", "okno", "oknu", "pritisni", "pritisnite", "izberi", "izberite", "vnesi",
        "vnesite", "vtipkaj", "vtipkajte", "tapni", "tapnite", "klikni", "kliknite",
    };
    if (names_the_number.count(prev)) { name_of_number(); return; }
    if (labels.count(prev)) return;
    const auto& not_counted = SL_NOT_COUNTED;
    // A masculine noun before it, by its final consonant: monitor 2, disk 2,
    // uporabnik 2, as the labels above (not plus 2, kot 2, pod 2)
    const bool after_masculine = prev.size() >= 3 && !is_digit(prev.back()) && !ends_in_vowel(prev) &&
                                 !not_counted.count(prev);

    // The word after the number, in small letters
    size_t word = end;
    while (word < text.size() && (text[word] == ' ' || text[word] == '\t')) ++word;
    // Nothing after it, or a mark glued to it (2. 2, 2)); a letter glued to it is
    // a unit or a compound (2kg, 2-krat, 2-letni), and a capitalized word before
    // it at the head of a clause a name like the one inside (Pentium 2)
    if (word == end || word >= text.size()) {
        const bool glued_letter = word == end && word < text.size() && is_letter_byte(text[word]);
        const bool glued_compound = word == end && word + 1 < text.size() && text[word] == '-' &&
                                    is_letter_byte(text[word + 1]);
        const bool unit_sign = word == end && word < text.size() && (text[word] == '%' || text[word] == '$');
        if (!glued_letter && !glued_compound && !unit_sign && !prev_capital && !after_masculine) name_of_number();
        return;
    }
    const unsigned char first = static_cast<unsigned char>(text[word]);
    const unsigned char second = word + 1 < text.size() ? static_cast<unsigned char>(text[word + 1]) : 0;
    const bool small_letter = (first >= 'a' && first <= 'z') ||
                              (first == 0xC4 && (second == 0x8D || second == 0x87 || second == 0x91)) ||   // č ć đ
                              (first == 0xC5 && (second == 0xA1 || second == 0xBE));                       // š ž
    size_t stop = word;
    while (stop < text.size() && is_word_byte(text[stop])) {
        if (is_digit(text[stop])) {
            stop = word;
            break;
        }
        ++stop;
    }
    const std::string_view next(text.data() + word, stop - word);
    auto ends = [&](std::string_view tail) {
        return next.size() >= tail.size() && next.compare(next.size() - tail.size(), tail.size(), tail) == 0;
    };

    // Units are not expanded, but 1 and 2 agree with the unit's noun: en kg
    // (kilogram), ena min (minuta), dve min, en MB (megabajt), en tisoč; 3
    // and 4 keep the plain form (tri kg). The capitals of MB, GB, kW count.
    const std::string unit = sl_ascii_lower(next);
    if (!next.empty() && (SL_MASCULINE_UNITS.count(unit) || SL_FEMININE_UNITS.count(unit))) {
        const bool fem = SL_FEMININE_UNITS.count(unit) != 0;
        if (digit == 1) say(fem ? "ena" : "en");
        else if (digit == 2) say(fem ? "dve" : "dva");
        return;
    }
    if (!small_letter) {
        // Another number, or a sign of arithmetic: 0 1 2 3, 2 + 2. A capital is
        // a noun of either gender (2 Slovenca), and %, $, € and ° are masculine.
        if (is_digit(text[word]) || first == '+' || first == '=' || first == '-' || first == '*' || first == '/') {
            name_of_number();
        }
        return;
    }
    if (stop == word) return;       // digits inside the word
    // je, se, ni, ki, mi: no noun; ur: od dveh ur
    if (next.size() < 3 && next != "ur") { name_of_number(); return; }
    // a verb's -uje, the imperative's -jte: 2 deluje, 2 zamešajte
    if (ends("uje") || ends("jte")) { name_of_number(); return; }

    if (not_counted.count(next)) { name_of_number(); return; }

    const auto& with_genitive = SL_WITH_GENITIVE;
    const auto& with_locative = SL_WITH_LOCATIVE;
    const auto& with_instrumental = SL_WITH_INSTRUMENTAL;
    const auto& with_dative = SL_WITH_DATIVE;
    const bool genitive = with_genitive.count(prev) != 0;
    const bool locative = with_locative.count(prev) != 0;
    const bool instrumental = with_instrumental.count(prev) != 0;
    const bool dative = with_dative.count(prev) != 0;
    const char tail = next.back();
    const bool no_ending = tail != 'a' && tail != 'e' && tail != 'i' && tail != 'o' && tail != 'u';

    // The feminine i-stems, the commonest of Sloleks (without -ost, -tev),
    // singular and plural: their consonant and their -i look masculine
    const auto& feminine = SL_FEMININE_I;
    const bool feminine_i = feminine.count(next) != 0 || (ends("ost") && next.size() > 5) ||
                            (ends("osti") && next.size() > 6) || ends("tev");

    if (five_plus) {
        // 5 and above decline in the oblique cases (SP 2001 pét pêtih, petim,
        // petimi: "v petih urah", "priti s petimi sošolci"; sedmih, osmimi;
        // the compounds on their whole: pred enaindvajsetimi leti), while
        // sto and tisoč stay (stó stôtih tudi neskl., tísoč -- tudi -ih). In
        // the nominative and the accusative the noun phrase is in the genitive
        // plural anyway (pet novih hiš), so -ih and -ov say nothing there:
        // only a preposition, or the -ah, -eh, -ami, -imi no other case has.
        const std::string base = last == "sedem" ? "sedm" : last == "osem" ? "osm" : std::string(last);
        auto decline = [&](std::string_view ending) { words.replace(token, std::string::npos, base + std::string(ending)); };
        if (ends("ami") || ends("imi")) decline("imi");                                   // s petimi prsti
        else if (ends("ah") || ends("eh")) decline("ih");                                  // v petih urah, v petih dneh
        else if (locative && ends("ih")) decline("ih");                                    // po petih letih
        else if (dative && (ends("am") || ends("em"))) decline("im");                      // k petim osebam
        else if ((prev == "z" || prev == "s" || prev == "pred" || prev == "med") && tail == 'i') decline("imi");   // pred petimi leti
        else if (genitive && (no_ending || next == "ljudi")) decline("ih");               // do petih let, od petih ur
        return;
    }

    if (digit == 1) {
        if (genitive && tail == 'a') { say("enega"); return; }      // do enega leta
        if (genitive && tail == 'e') { say("ene"); return; }        // od ene ure
        if (locative && tail == 'u') { say("enem"); return; }       // v enem dnevu
        if (locative && tail == 'i') { say("eni"); return; }        // v eni uri, na eni strani
        if (instrumental && (ends("om") || ends("em"))) { say("enim"); return; }    // pred enim letom
        if (dative && tail == 'u') { say("enemu"); return; }
        if (dative && tail == 'i') { say("eni"); return; }
        static const std::unordered_set<std::string_view> masculine_o = {
            "evro", "avto", "euro", "kino", "radio", "video", "studio", "biro", "metro", "foto",
        };
        if (tail == 'a' || tail == 'u') return;                     // ena knjiga
        if (tail == 'o') { say(masculine_o.count(next) ? "en" : "eno"); return; }
        if (tail == 'e') { say("eno"); return; }                    // eno polje, eno vprašanje
        if (tail == 'i' || !feminine_i) say("en");                  // en koš, en glavni; ena stvar
        return;
    }

    const char* const INSTRUMENTAL[] = {"dvema", "tremi", u8"štirimi"};
    const char* const GENITIVE[] = {"dveh", "treh", u8"štirih"};
    const char* const DATIVE[] = {"dvema", "trem", u8"štirim"};
    const char* const MASCULINE[] = {"dva", "trije", u8"štirje"};
    const size_t n = static_cast<size_t>(digit - 2);

    if (digit == 2 && (ends("ama") || ends("oma") || ends("ema"))) { say("dvema"); return; }
    if (ends("ami") || ends("imi")) { say(INSTRUMENTAL[n]); return; }
    if (ends("ah") || ends("eh") || ends("ih") || ends("ov") || ends("ev")) { say(GENITIVE[n]); return; }
    if (dative && digit > 2 && (ends("am") || ends("em"))) { say(DATIVE[n]); return; }
    if ((prev == "z" || prev == "s" || prev == "pred" || prev == "med") && tail == 'i') { say(INSTRUMENTAL[n]); return; }
    if (genitive && (no_ending || next == "ljudi")) { say(GENITIVE[n]); return; }

    if (digit == 2) {
        static const std::unordered_set<std::string_view> masculine_forms = {
            "dni", "dneve", "dnevi", "ljudje", "ljudi", "otroke", "otroci", u8"možje", u8"moški",
            "fantje", "bratje", "gostje", u8"starši", "tedne", "tedni", "mesece", "meseci",
        };
        if ((tail == 'i' || tail == 'e') && !masculine_forms.count(next)) say("dve");
        return;
    }
    // 3, 4
    static const std::unordered_set<std::string_view> masculine_je = {
        "ljudje", "gostje", "fantje", "kmetje", u8"možje", u8"študentje", "sosedje", "lasje", "vodje",
        "bratje", "gospodje", u8"očetje", "zobje", u8"škofje", "grofje", "agentje", "dedje",
    };
    if (masculine_je.count(next)) { say(MASCULINE[n]); return; }
    if (tail == 'i' && !feminine_i && next != "dni" && next != "leti" && next != "uri" && next != "ljudi") {
        say(MASCULINE[n]);
    }
}


// The nouns of the unit signs, in the forms a number asks for (SP 2001
// odstótek -tka m "pet ~ov ‹5 %›", évro -a m, dólar -ja m, stopínja -e ž
// "pet stopinj Celzija ‹5 °C›", Célzij -a "37 stopinj ~a")
struct SlUnit {
    const char* symbol;
    bool feminine;
    const char* singular;
    const char* dual;
    const char* plural;             // nominative: trije evri
    const char* accusative_plural;  // tri evre (za 3 €, stane 3 €)
    const char* genitive_plural;
    const char* genitive_singular;
};

namespace {

const SlUnit SL_UNITS[] = {
    {"%", false, "odstotek", "odstotka", "odstotki", "odstotke", "odstotkov", "odstotka"},
    {"\xE2\x82\xAC", false, "evro", "evra", "evri", "evre", "evrov", "evra"},
    {"$", false, "dolar", "dolarja", "dolarji", "dolarje", "dolarjev", "dolarja"},
    {"\xC2\xB0", true, "stopinja", "stopinji", "stopinje", "stopinje", "stopinj", "stopinje"},
};

// A unit's sign at or after pos (spaces allowed): %, €, $, °, °C, °F, not
// glued to a letter. Sets the end of the sign and the scale's word.
const SlUnit* sl_unit_at(const std::string& text, size_t pos, size_t& end, std::string_view& scale) {
    pos = sl_skip_spaces(text, pos);
    if (pos >= text.size()) return nullptr;
    for (const SlUnit& u : SL_UNITS) {
        const size_t n = std::strlen(u.symbol);
        if (text.compare(pos, n, u.symbol) != 0) continue;
        end = pos + n;
        scale = "";
        if (u.feminine && end < text.size() && (end + 1 >= text.size() || !is_word_byte(text[end + 1]))) {
            if (text[end] == 'C' || text[end] == 'c') { scale = " Celzija"; ++end; }
            else if (text[end] == 'F' || text[end] == 'f') { scale = " Fahrenheita"; ++end; }
        }
        if (end < text.size() && is_letter_byte(text[end])) return nullptr;
        return &u;
    }
    return nullptr;
}

// Whether a number with a unit is an object: after za, na, po, čez, skozi
// (za 3 €, po 3 € kos) or after a verb's l-participle or stane, znaša
// (plačal 3 €, stane 4 €, zvišali za 3 %); the subject otherwise (3 € je
// malo, 3 % ljudi). Only 3 and 4 show the difference (tri evre, trije evri).
bool sl_accusative_context(const std::string& text, const SlPrev& prev_word) {
    std::string prev = prev_word.word;
    static const std::unordered_set<std::string_view> clitics = {
        "sem", "si", "je", "sva", "sta", "smo", "ste", "so", "bo", "bom", u8"boš", "bova", "bosta", "bomo", "boste",
        "bodo", "bi", "ga", "jo", "mi", "ti", "mu", "ji", "nam", "vam", "jim", "jih", "se",
    };
    if (clitics.count(prev)) prev = sl_prev_word(text, prev_word.begin).word;      // plačal sem 4 €
    static const std::unordered_set<std::string_view> before_object = {
        "za", "na", "po", u8"čez", "skozi", "stane", "stanejo", u8"znaša", u8"znašajo", "velja", "veljajo",
        "dobi", "dobite", "dobim", "dobimo", "prejme", "prejmete", "prejmem", u8"plača", u8"plačate", u8"plačam",
        u8"plačamo", u8"plačajo", u8"zviša", u8"zniža", u8"zvišajo", u8"znižajo", "doda", "dodajte", "dodaj",
        "odbije", "odbijte", "prihrani", "prihranite", "prihranimo",
    };
    if (before_object.count(prev)) return true;
    return prev.size() >= 4 && (sl_ends(prev, "al") || sl_ends(prev, "il") || sl_ends(prev, "el") || sl_ends(prev, "ala") ||
                                sl_ends(prev, "ila") || sl_ends(prev, "ali") || sl_ends(prev, "ili") || sl_ends(prev, "alo") ||
                                sl_ends(prev, "ilo") || sl_ends(prev, "ale") || sl_ends(prev, "ile"));
}

long sl_value(std::string_view digits) {
    if (digits.size() > 9) return -1;
    long v = 0;
    for (char c : digits) v = v * 10 + (c - '0');
    return v;
}

bool sl_punct_or_end(const std::string& text, size_t pos) {
    if (pos >= text.size()) return true;
    const char c = text[pos];
    return c == '.' || c == ',' || c == ';' || c == '!' || c == '?' || c == ':' || c == ')' || c == '\n' ||
           c == '"' || c == '\'' || static_cast<unsigned char>(c) >= 0x80;
}

} // namespace

// A number with a unit's sign: the noun in the form the number asks for,
// the number agreeing with it (en odstotek, dva odstotka, trije odstotki,
// štirje evri, pet odstotkov, enaindvajset odstotkov, sto en evro, ena
// stopinja, dve stopinji, tri stopinje, pet stopinj Celzija, nič odstotkov);
// `accusative`: tri evre, štiri odstotke, where the number is an object
std::string CroatianNumbers::slovenian_count(std::string_view digits, const SlUnit& unit, std::string_view scale,
                                             bool accusative) {
    std::string words = number_to_words(digits);
    const std::string_view clean = remove_leading_zeros(digits);
    const bool ones_alone = clean.size() == 1 || clean[clean.size() - 2] == '0';
    const char last = clean.back();
    size_t token = words.rfind(' ');
    token = token == std::string::npos ? 0 : token + 1;
    const char* form = unit.genitive_plural;
    if (ones_alone && last == '1') {
        words.replace(token, std::string::npos, unit.feminine ? "ena" : "en");
        form = unit.singular;
    } else if (ones_alone && last == '2') {
        words.replace(token, std::string::npos, unit.feminine ? "dve" : "dva");
        form = unit.dual;
    } else if (ones_alone && (last == '3' || last == '4')) {
        // The object of a verb or of za, na, po: tri evre (plačal 3 €, za
        // 3 %); the subject: trije evri
        if (!unit.feminine && !accusative) words.replace(token, std::string::npos, last == '3' ? "trije" : u8"štirje");
        form = accusative ? unit.accusative_plural : unit.plural;
    }
    words += ' ';
    words += form;
    words += scale;
    return words;
}

// A Slovenian ordinal before a lowercase word: "1. mesto" prvo mesto, "na
// 1. mestu" na prvem mestu, "v 21. stoletju" v enaindvajsetem stoletju,
// "7. oktober" sedmi oktober, "7. oktobra" sedmega oktobra, "v 90. letih"
// v devetdesetih letih, "1. stran" prva stran, "3. člen" tretji člen. The
// ending comes from the noun's ending and the preposition before the number
// (sl_ordinal_ending()); "" where nothing is counted, and the cardinal
// stays. [start, end) are the digits, `after` the position after the period.
std::string CroatianNumbers::slovenian_ordinal_before(const std::string& text, size_t start, size_t end, size_t after) {
    const std::string_view number(text.data() + start, end - start);
    if (remove_leading_zeros(number) == "0") return "";
    const SlNext next = sl_next_word(text, after);
    if (next.word.empty()) return "";
    for (char c : next.word) {
        if (is_digit(c)) return "";
    }
    const SlPrev prev = sl_prev_word(text, start);
    const std::string_view ending = sl_ordinal_ending(prev.word, next.word);
    if (ending.empty()) return "";
    return slovenian_ordinal(number, ending);
}

// The Slovenian constructs at position i (a digit) that the shared loop does
// not know. Writes the words into `result` and returns the end of what it
// consumed, or 0 to leave the number to the shared paths. `in_range`: the
// right side of a range, whose context the left side has already read.
size_t CroatianNumbers::slovenian_number_at(const std::string& text, size_t i, std::string& result, bool in_range) {
    const size_t length = text.size();
    const size_t end = digits_end(text, i);
    const std::string_view digits(text.data() + i, end - i);
    const bool leading_zero = text[i] == '0' && end - i > 1;
    const long value = sl_value(digits);
    const SlPrev prev = sl_prev_word(text, i);
    std::vector<std::string_view> groups;

    // A minus glued to the number (Pravopis 8.0, § 26: –10 °C; SP 2001 mínus
    // "temperatura ~ deset stopinj Celzija"): the hyphen-minus, the minus
    // sign or the en dash, after a space or at the start, so that T-2, A-2
    // and 1991-2000 keep their hyphen
    if (!in_range && i > 0) {
        size_t sign = 0;
        if (text[i - 1] == '-') {
            sign = 1;
        } else if (i >= 3 && static_cast<unsigned char>(text[i - 3]) == 0xE2 &&
                   ((static_cast<unsigned char>(text[i - 2]) == 0x88 && static_cast<unsigned char>(text[i - 1]) == 0x92) ||
                    (static_cast<unsigned char>(text[i - 2]) == 0x80 && static_cast<unsigned char>(text[i - 1]) == 0x93))) {
            sign = 3;
        }
        if (sign > 0) {
            const size_t at = i - sign;
            const unsigned char b = at > 0 ? static_cast<unsigned char>(text[at - 1]) : ' ';
            if (b == ' ' || b == '\t' || b == '\n' || b == '(' || b == '[' || b == '"' || b == '\'' || b == ':' || b == 0xA0 ||
                b == 0x9E || b == 0x9C) {       // the last bytes of „ and “
                size_t r = result.size();
                if (sign == 3 && r > 0 && result[r - 1] == ' ') --r;       // set_off_from_letters() set the dash off
                if (r >= sign && result.compare(r - sign, sign, text, at, sign) == 0) {
                    result.erase(r - sign);
                    if (!result.empty() && is_word_byte(result.back())) result += ' ';
                    result += "minus ";
                }
            }
        }
    }
    if (leading_zero) return 0;

    // A range of two times: 8.30–16.00, 8:30–16:00
    if (!in_range) {
        size_t left_stop = 0;
        if (end + 2 < length && (text[end] == '.' || text[end] == ':') && digits_end(text, end + 1) == end + 3) {
            left_stop = end + 3;
        }
        if (left_stop > 0 && left_stop + 2 < length && static_cast<unsigned char>(text[left_stop]) == 0xE2 &&
            static_cast<unsigned char>(text[left_stop + 1]) == 0x80 &&
            (static_cast<unsigned char>(text[left_stop + 2]) == 0x93 || static_cast<unsigned char>(text[left_stop + 2]) == 0x94) &&
            left_stop + 3 < length && is_digit(text[left_stop + 3])) {
            std::string left_words, right_words;
            const size_t l = slovenian_number_at(text, i, left_words, true);
            const size_t r = l == left_stop ? slovenian_number_at(text, left_stop + 3, right_words, true) : 0;
            if (l == left_stop && r > 0) {
                result += left_words + " do " + right_words;
                return r;
            }
        }
    }

    // A full date is read by the shared path
    if (parse_date(text, i, groups)) return 0;

    // A clock time with a colon: ob 17:00 is ob sedemnajstih; 24:00 is a
    // time too (parse_time() stops at 23)
    if (size_t t_end = parse_time(text, i, groups)) {
        if (prev.word == "ob" && groups.size() == 2 && groups[1] == "00" && value >= 1) {
            result += SL_HOURS_LOCATIVE[value];
            return digits_end(text, end + 1);
        }
        if (in_range) {
            result += time_to_words(groups);
            return t_end;
        }
        return 0;
    }
    if (digits == "24" && text.compare(end, 3, ":00") == 0 && (end + 3 >= length || (!is_digit(text[end + 3]) && text[end + 3] != ':'))) {
        result += prev.word == "ob" ? u8"štiriindvajsetih" : u8"štiriindvajset nič nič";
        return end + 3;
    }

    // A clock time written with a period, the Slovenian way (SP 2001: ob
    // 8.30, ob 17.00): h or hh, a period, two minute digits, in the context
    // of a time: after ob, od, do, med, okoli, okrog, or before h, ure, uri,
    // ur, zjutraj, zvečer, dopoldne, popoldne, ponoči. Without it 3.14 and
    // 3.2 stay what they were. A period after it with a possible month
    // inside (7.10.) is the date, unless ob stands before it.
    static const std::unordered_set<std::string_view> before_time = {
        "ob", "od", "do", "med", "okoli", "okrog", u8"približno", u8"točno", "ure", "uri", "ura",
    };
    static const std::unordered_set<std::string_view> after_time = {
        "h", "ure", "uri", "ur", "zjutraj", u8"zvečer", "dopoldne", "popoldne", u8"ponoči", "opoldne",
    };
    if (end - i <= 2 && value >= 0 && value <= 24 && end + 2 < length && text[end] == '.' &&
        digits_end(text, end + 1) == end + 3) {
        const std::string_view minutes(text.data() + end + 1, 2);
        const long mm = sl_value(minutes);
        const size_t after = end + 3;
        const bool glued_more = after < length && ((text[after] == '.' || text[after] == ',') && after + 1 < length &&
                                                   is_digit(text[after + 1]));
        const bool colon_after = after < length && text[after] == ':';
        const SlNext next = sl_next_word(text, after);
        const bool date_like = after < length && text[after] == '.' && prev.word != "ob" && mm >= 1 && mm <= 12 &&
                               value >= 1 && value <= 31;
        const bool after_ura = (prev.word == "je" || prev.word == "bo") && prev.ahead >= 3 &&
                               sl_ascii_lower(std::string_view(text).substr(prev.ahead - 3, 3)) == "ura" &&
                               (prev.ahead == 3 || !is_word_byte(text[prev.ahead - 4]));     // ura je 8.30
        const bool context = in_range || before_time.count(prev.word) != 0 || after_ura ||
                             (next.lowercase && after_time.count(next.word) != 0 && next.begin <= after + 1);
        if (mm <= 59 && (value < 24 || mm == 0) && !glued_more && !colon_after && !date_like && context) {
            if (prev.word == "ob" && mm == 0 && value >= 1) {
                result += SL_HOURS_LOCATIVE[value];
            } else {
                groups = {digits, minutes};
                result += time_to_words(groups);
            }
            return after;
        }
    }

    // A date without a year: 7. 10., 7.10. (sedmega desetega); not before a
    // year (the shared path), not in an enumeration (1. 2. in 3. mesto)
    auto short_date = [&](size_t pos, std::string& words, size_t& stop) -> bool {
        const size_t d_end = digits_end(text, pos);
        if (d_end == pos || d_end - pos > 2 || text[pos] == '0' || d_end >= length || text[d_end] != '.') return false;
        const long day = sl_value(std::string_view(text.data() + pos, d_end - pos));
        if (day < 1 || day > 31) return false;
        size_t m = d_end + 1;
        if (m < length && text[m] == ' ') ++m;
        const size_t m_end = digits_end(text, m);
        if (m_end == m || m_end - m > 2 || m_end >= length || text[m_end] != '.') return false;
        const long month = sl_value(std::string_view(text.data() + m, m_end - m));
        if (month < 1 || month > 12) return false;
        size_t y = m_end + 1;
        if (y < length && text[y] == ' ') ++y;
        if (y < length && is_digit(text[y])) return false;           // a year follows: the full date
        const SlNext next = sl_next_word(text, m_end + 1);
        static const std::unordered_set<std::string_view> joining = {"in", "ali", "do", "ter", "oziroma", "pa"};
        if (next.lowercase && joining.count(next.word)) return false;
        words = slovenian_ordinal(std::string_view(text.data() + pos, d_end - pos), "ega") + ' ' +
                slovenian_ordinal(std::string_view(text.data() + m, m_end - m), "ega");
        // The final period is silent before a lowercase word, as the period
        // of "7. oktobra" is; before anything else it ends the sentence
        if (next.lowercase && next.begin > m_end + 1) {
            stop = m_end + 1;
        } else if (next.lowercase) {
            stop = m_end + 1;
            words += ' ';
        } else {
            stop = m_end;
        }
        return true;
    };

    // A range: 1991–2000, 1991-2000, 2–3 dni, 15.–20. 3., 3.–4. mesto, 15. do
    // 20. marca, 8.30–16.00: "do" between the two, both read alike
    if (!in_range && !leading_zero) {
        size_t left_end = end;
        bool dotted = false;
        if (left_end < length && text[left_end] == '.' && !(left_end + 1 < length && is_digit(text[left_end + 1]))) {
            dotted = true;
            ++left_end;
        }
        size_t c = left_end;
        std::string connector;
        if (c + 2 < length && static_cast<unsigned char>(text[c]) == 0xE2 && static_cast<unsigned char>(text[c + 1]) == 0x80 &&
            (static_cast<unsigned char>(text[c + 2]) == 0x93 || static_cast<unsigned char>(text[c + 2]) == 0x94)) {
            connector = "dash";
            c += 3;
        } else if (c < length && text[c] == '-' && !dotted) {
            connector = "hyphen";
            c += 1;
        } else if (dotted) {
            const SlNext word = sl_next_word(text, c);
            static const std::unordered_set<std::string_view> joining = {"do", "in", "ali", "ter", "oziroma"};
            if (word.begin > c && word.lowercase && joining.count(word.word)) {
                connector = word.word;
                c = sl_skip_spaces(text, word.end);
            }
        }
        if (!connector.empty() && c < length && is_digit(text[c]) && text[c] != '0') {
            const size_t r_end = digits_end(text, c);
            const std::string_view right(text.data() + c, r_end - c);
            const std::string join = connector == "dash" || connector == "hyphen" ? " do " : " " + connector + " ";
            std::string words;
            size_t stop = 0;
            // The right side a date: 15.–20. 3. 2026, 15. do 20. marca
            if (parse_date(text, c, groups)) {
                result += slovenian_ordinal(digits, "ega") + join + date_to_words(groups);
                return digits_end(text, c + 0) == c ? 0 : parse_date(text, c, groups);
            }
            if (short_date(c, words, stop)) {
                result += slovenian_ordinal(digits, "ega") + join + words;
                return stop;
            }
            if (r_end < length && text[r_end] == '.') {
                // The right side an ordinal before a lowercase word (3.–4.
                // mesto); silent_dot_end() would stop at the dash before it
                const SlNext noun = sl_next_word(text, r_end + 1);
                if (r_end - c <= 4 && noun.lowercase && !(r_end + 1 < length && is_digit(text[r_end + 1]))) {
                    const size_t after = r_end + 1;
                    if (sl_is_month_genitive(noun.word)) {
                        result += slovenian_ordinal(digits, "ega") + join + slovenian_ordinal(right, "ega");
                    } else {
                        const std::string_view ending = sl_ordinal_ending(prev.word, noun.word);
                        if (ending.empty()) {
                            result += number_to_words(digits) + join + number_to_words(right);
                        } else {
                            result += slovenian_ordinal(digits, ending) + join + slovenian_ordinal(right, ending);
                        }
                    }
                    if (text[after] != ' ' && text[after] != '\t') result += ' ';
                    return after;
                }
                if (dotted) return 0;
            } else if (dotted) {
                return 0;
            }
            if (!dotted) {
                // Both times: 8.30–16.00
                std::string right_words;
                if (end + 2 < length && text[end] == '.' && digits_end(text, end + 1) == end + 3) {
                    if (size_t t = slovenian_number_at(text, c, right_words, true)) {
                        std::string left_words;
                        if (size_t l = slovenian_number_at(text, i, left_words, true); l == left_end || l == end + 3) {
                            result += left_words + join + right_words;
                            return t;
                        }
                    }
                }
                // A hyphen joins a range only between small numbers or two
                // years (2-3 dni, 10-15 %, str. 98-102, 1991-2000); 716-844
                // is a telephone number
                if (connector == "hyphen" &&
                    (sl_value(right) <= value || r_end - c > 4 || !(end - i <= 2 || (end - i == 4 && r_end - c == 4)))) {
                    return 0;
                }
                if (text[end] == '.' || text[end] == ',') return 0;
                if (is_separator_between_digits(text, r_end)) return 0;
                std::string left_words = number_to_words(digits);
                size_t unit_end = 0;
                std::string_view scale;
                if (const SlUnit* unit = sl_unit_at(text, r_end, unit_end, scale)) {
                    result += left_words + join + slovenian_count(right, *unit, scale, sl_accusative_context(text, prev));
                    return unit_end;
                }
                std::string right_words2 = number_to_words(right);
                slovenian_agree(text, i, r_end, left_words);
                slovenian_agree(text, i, r_end, right_words2);
                result += left_words + join + right_words2;
                return r_end;
            }
        }
    }

    {
        std::string words;
        size_t stop = 0;
        if (!in_range && short_date(i, words, stop)) {
            // Not after another "N." (1. 2. 3.), which would be a list
            if (!(prev.word.empty() && prev.ahead >= 2 && text[prev.ahead - 1] == '.' && is_digit(text[prev.ahead - 2]))) {
                result += words;
                return stop;
            }
        }
    }

    // Thousands separated by spaces (1 000, 1 000 000, also no-break spaces):
    // one number when the first group has 1 to 3 digits and every other one
    // exactly 3, not after a digit or a plus (+386 41 123 456), not before
    // more digits; a phone number with its leading zero (031 123 456) and
    // single digits (1 2 3) are left as they are
    if (end - i <= 3) {
        size_t pos = end;
        std::string joined(digits);
        int group_count = 0;
        while (group_count < 4) {
            const size_t next = sl_skip_spaces(text, pos);
            if (next == pos || digits_end(text, next) != next + 3) break;
            joined.append(text, next, 3);
            pos = next + 3;
            ++group_count;
        }
        // Not after a digit, a plus, a bracket or a slash (+386 41 123 456,
        // (031) 731 940, 01/234 567), not after the word telefon or številka,
        // and not after a name inside a sentence (Pentium 2 266 MHz)
        size_t before_pos = prev.word.empty() ? prev.end : prev.begin;
        while (before_pos > 0 && (text[before_pos - 1] == ' ' || text[before_pos - 1] == '\t')) --before_pos;
        const char before = before_pos > 0 ? text[before_pos - 1] : ' ';
        static const std::unordered_set<std::string_view> telephone = {
            "telefon", "telefonu", "telefona", "tel", "telefonska", "telefonski", "faks", "faksu", "gsm", "mobitel",
            "mobitelu", u8"številka", u8"številki", u8"številko", u8"št", u8"račun", u8"računu", "trr", "iban",
            u8"pošta", u8"poštna", "sklic", "klic", u8"pokličite", u8"pokliči",
        };
        const bool phone = telephone.count(prev.word) != 0 ||
                           (!prev.word.empty() && (is_digit(before) || is_digit(prev.word.back()))) ||
                           (prev.word.empty() && (is_digit(before) || before == '+' || before == ')' || before == '/'));
        const char head = prev.ahead > 0 ? text[prev.ahead - 1] : '.';
        const bool after_name = prev.capital && head != '.' && head != '!' && head != '?' && head != ':' && head != ';' &&
                                head != '\n' && head != ',' && head != '(';
        if (group_count > 0 && !phone && !after_name && !is_separator_between_digits(text, pos) &&
            !(pos < length && (is_letter_byte(text[pos]) || text[pos] == ':'))) {
            size_t unit_end = 0;
            std::string_view scale;
            if (const SlUnit* unit = sl_unit_at(text, pos, unit_end, scale)) {
                result += slovenian_count(joined, *unit, scale, sl_accusative_context(text, prev));
                return unit_end;
            }
            std::string words = number_to_words(joined);
            slovenian_agree(text, i, pos, words);
            result += words;
            return pos;
        }
    }

    // Thousands separated by periods (132.457, the shared Slovenian path)
    // before a unit's sign: 1.000 € is tisoč evrov, 2.500 € dva tisoč petsto
    // evrov
    if (end - i <= 3 && end < length && text[end] == '.' && digits_end(text, end + 1) == end + 4) {
        size_t pos = end;
        std::string joined(digits);
        while (pos < length && text[pos] == '.' && digits_end(text, pos + 1) == pos + 4) {
            joined.append(text, pos + 1, 3);
            pos += 4;
        }
        size_t unit_end = 0;
        std::string_view scale;
        if (!is_separator_between_digits(text, pos)) {
            if (const SlUnit* unit = sl_unit_at(text, pos, unit_end, scale)) {
                result += slovenian_count(joined, *unit, scale, sl_accusative_context(text, prev));
                return unit_end;
            }
        }
        return 0;
    }

    // A number compounded with a word by a hyphen or glued to x: 2-krat
    // dvakrat, 2x dvakrat, 21-krat enaindvajsetkrat, 2-letni dvoletni,
    // 3-letna triletna, 10-letnica desetletnica, 5-dnevni petdnevni, 1-sobno
    // enosobno (SP 2001 dvákrat, dvóléten, desétlétnica), one word so the
    // lexicon's accent applies; not after a letter (A-2), not before a
    // capital (3-D)
    if (end - i <= 4 && (i == 0 || !is_word_byte(text[i - 1])) && end < length) {
        const std::string cardinal = number_to_words(digits);
        if (text[end] == '-' && end + 1 < length && is_lowercase_letter(code_point_at(text, end + 1))) {
            size_t stop = end + 1;
            while (stop < length && is_word_byte(text[stop]) && !is_digit(text[stop])) ++stop;
            const std::string_view word(text.data() + end + 1, stop - end - 1);
            if (word.compare(0, 4, "krat") == 0) {
                result += sl_number_stem(digits, cardinal, true);
                result += word;
                return stop;
            }
            if (word.size() >= 4 && !(stop < length && is_digit(text[stop]))) {
                result += sl_number_stem(digits, cardinal, false);
                result += word;
                return stop;
            }
        }
        const bool times_x = text[end] == 'x' && (end + 1 >= length || !is_letter_byte(text[end + 1]));
        const bool times_sign = end + 1 < length && static_cast<unsigned char>(text[end]) == 0xC3 &&
                                static_cast<unsigned char>(text[end + 1]) == 0x97 &&
                                (end + 2 >= length || !is_letter_byte(text[end + 2]));
        if (times_x || times_sign) {
            result += sl_number_stem(digits, cardinal, true);
            result += "krat";
            return end + (times_x ? 1 : 2);
        }
    }

    // The hour after ob: ob 5, ob 5h, ob 13 → ob petih, ob trinajstih, when
    // nothing counted follows (SP 2001 pét: priti ob petih |ob 5h|); ob 5
    // urah and ob 5. uri are read by the agreement and the ordinal
    if (prev.word == "ob" && end - i <= 2 && value >= 1 && value <= 24) {
        size_t pos = end;
        bool hour = false;
        if (pos < length && text[pos] == 'h' && (pos + 1 >= length || !is_word_byte(text[pos + 1]))) {
            hour = true;
            ++pos;
        } else if (pos >= length || !is_word_byte(text[pos])) {
            const size_t w = sl_skip_spaces(text, pos);
            if (w < length && text[w] == 'h' && (w + 1 >= length || !is_word_byte(text[w + 1]))) {
                hour = true;
                pos = w + 1;
            } else if (!(pos < length && (text[pos] == '.' || text[pos] == ',' || text[pos] == ':'))) {
                static const std::unordered_set<std::string_view> after_hour = {
                    "zjutraj", u8"zvečer", "dopoldne", "popoldne", u8"ponoči", "opoldne", "je", "bo", "bomo", "boste",
                    "bodo", "se", "sem", "smo", "ste", "pa", "in", "ali", "ter", "do", "na", "v", "pri", "ob",
                };
                const SlNext next = sl_next_word(text, pos);
                hour = sl_punct_or_end(text, w) || (!next.word.empty() && !next.lowercase && !is_digit(text[next.begin])) ||
                       (next.lowercase && after_hour.count(next.word) != 0);
            }
        }
        if (hour) {
            result += SL_HOURS_LOCATIVE[value];
            return pos;
        }
    }

    // A decimal with a unit's sign: 1,5 % ena cela pet odstotka, 2,50 € dve
    // celi petdeset evra (the genitive singular after a decimal)
    if (end < length && text[end] == ',' && is_separator_between_digits(text, end)) {
        const size_t f_end = digits_end(text, end + 1);
        if (!is_separator_between_digits(text, f_end)) {
            size_t unit_end = 0;
            std::string_view scale;
            if (const SlUnit* unit = sl_unit_at(text, f_end, unit_end, scale)) {
                result += slovenian_decimal(digits, std::string_view(text.data() + end + 1, f_end - end - 1));
                result += ' ';
                result += unit->genitive_singular;
                result += scale;
                return unit_end;
            }
        }
    }

    // A whole number with a unit's sign: 1 % en odstotek, 2 € dva evra, 5 °C
    // pet stopinj Celzija
    if (!is_separator_between_digits(text, end)) {
        size_t unit_end = 0;
        std::string_view scale;
        if (const SlUnit* unit = sl_unit_at(text, end, unit_end, scale)) {
            result += slovenian_count(digits, *unit, scale, sl_accusative_context(text, prev));
            return unit_end;
        }
    }
    return 0;
}


// =============================================================================
// Croatian, Serbian and Bosnian: the numerals' grammar
// =============================================================================

namespace {

// A word of the text in small Latin letters: Serbian Cyrillic transliterated
// (the converter writes Latin words, which the front end reads in either
// script), an initial capital lowered. `capital` says whether there was one.
std::string hr_latin(const std::string& word, bool* capital) {
    static const struct { const char* cyr; const char* lat; } CYR[] = {
        {u8"а", "a"}, {u8"б", "b"}, {u8"в", "v"}, {u8"г", "g"}, {u8"д", "d"}, {u8"ђ", u8"đ"}, {u8"е", "e"},
        {u8"ж", u8"ž"}, {u8"з", "z"}, {u8"и", "i"}, {u8"ј", "j"}, {u8"к", "k"}, {u8"л", "l"}, {u8"љ", "lj"},
        {u8"м", "m"}, {u8"н", "n"}, {u8"њ", "nj"}, {u8"о", "o"}, {u8"п", "p"}, {u8"р", "r"}, {u8"с", "s"},
        {u8"т", "t"}, {u8"ћ", u8"ć"}, {u8"у", "u"}, {u8"ф", "f"}, {u8"х", "h"}, {u8"ц", "c"}, {u8"ч", u8"č"},
        {u8"џ", u8"dž"}, {u8"ш", u8"š"},
        {u8"А", "a"}, {u8"Б", "b"}, {u8"В", "v"}, {u8"Г", "g"}, {u8"Д", "d"}, {u8"Ђ", u8"đ"}, {u8"Е", "e"},
        {u8"Ж", u8"ž"}, {u8"З", "z"}, {u8"И", "i"}, {u8"Ј", "j"}, {u8"К", "k"}, {u8"Л", "l"}, {u8"Љ", "lj"},
        {u8"М", "m"}, {u8"Н", "n"}, {u8"Њ", "nj"}, {u8"О", "o"}, {u8"П", "p"}, {u8"Р", "r"}, {u8"С", "s"},
        {u8"Т", "t"}, {u8"Ћ", u8"ć"}, {u8"У", "u"}, {u8"Ф", "f"}, {u8"Х", "h"}, {u8"Ц", "c"}, {u8"Ч", u8"č"},
        {u8"Џ", u8"dž"}, {u8"Ш", u8"š"},
    };
    std::string out;
    bool cap = false;
    bool first = true;
    for (size_t k = 0; k < word.size();) {
        const unsigned char b = static_cast<unsigned char>(word[k]);
        if (b == 0xD0 || b == 0xD1) {
            bool done = false;
            for (size_t n = 0; n < sizeof(CYR) / sizeof(CYR[0]); ++n) {
                if (word.compare(k, 2, CYR[n].cyr) == 0) {
                    if (first && n >= 30) cap = true;
                    out += CYR[n].lat;
                    k += 2;
                    done = true;
                    break;
                }
            }
            if (!done) { out += word[k]; ++k; }
        } else {
            out += word[k];
            ++k;
        }
        first = false;
    }
    if (sl_lowercase_first(out)) cap = true;
    if (capital) *capital = cap;
    return out;
}

// The prepositions by the case they govern (hr/sr/bs)
const std::unordered_set<std::string_view> HR_WITH_GENITIVE = {
    "od", "do", "iz", "bez", "zbog", "oko", "kod", "pored", "blizu", u8"između", "izmedju", "poslije", "posle",
    "nakon", "prije", "pre", "osim", "radi", "tijekom", "tokom", "protiv", "ispod", "iznad", "ispred", "iza",
    "preko", "kraj", "pokraj", "umjesto", "umesto", "mimo", u8"duž", "putem", "pomoću", u8"pomoću", "unutar",
    "izvan", "van", "nasuprot", "usred", "sred", "s", "sa",
};
const std::unordered_set<std::string_view> HR_WITH_LOCATIVE = {"u", "na", "o", "pri", "po"};
const std::unordered_set<std::string_view> HR_WITH_DATIVE = {"prema", "k", "ka", u8"usprkos", "unatoč", u8"unatoč"};
const std::unordered_set<std::string_view> HR_WITH_INSTRUMENTAL = {"s", "sa", "pred", "pod", "nad", "za", u8"među", "medju"};

// The words after which a number counts nothing, and the commonest verbs
const std::unordered_set<std::string_view> HR_NOT_COUNTED = {
    "i", "ili", "te", "pa", "a", "ali", "je", "su", "sam", "si", "smo", "ste", "bi", u8"će", u8"ću", u8"ćeš",
    u8"ćemo", u8"ćete", "ni", "ne", "da", "li", "se", "ga", "mu", "joj", "im", "ih", "nas", "vas", "od", "do",
    "u", "na", "za", "s", "sa", "o", "po", "pri", "kod", "iz", "bez", "pod", "nad", "pred", u8"među", "kroz",
    "prema", "plus", "minus", "puta", "posto", "odsto", u8"više", "manje", "oko", "otprilike", "samo", u8"već",
    u8"još", "opet", "ujutro", u8"navečer", u8"uvečer", u8"uveče", "popodne", "poslijepodne", "poslepodne",
    "danas", "sutra", u8"jučer", u8"juče", "sada", "sad", "tada", "onda", "ovdje", "ovde", "tamo", "tu", "gdje",
    "gde", "kako", "kada", "kad", u8"zašto", "jer", "ako", "dok", "nego", "kao", u8"čak", "isto", "tako",
    u8"također", u8"takođe", "zato", "dakle", "naime", u8"možda", "valjda", "sigurno", "vrlo", "jako", "puno",
    "mnogo", "malo", "dosta", u8"previše", "premalo", "bar", "barem", u8"čim", "tek", "upravo", u8"uopće",
    u8"uopšte", "zatim", "potom", "ipak", u8"inače", "dalje", "ponovo", "ponovno", "jedan", "dva", "tri",
    u8"četiri", "pet", u8"šest", "sedam", "osam", "devet", "deset", "sto", u8"tisuću", "hiljadu", "milijun",
    "milion", "ima", "nema", "biti", "bio", "bila", "bilo", "bili", "bile", "jest", "nije", "nisu",
    "jesu", "jesam", "nisam", "budu", "bude", "mogu", u8"može", "treba", "mora", "moraju", "hoće", u8"hoće",
    "jedno", "jedna", "dvije", "dve", "oba", "obje", "obe", "svaki", "svaka", "svako", "svi", "sve", "sva",
};
// The labels a number names (stranica 2, slika 2): the plain form
const std::unordered_set<std::string_view> HR_LABELS = {
    "stranica", "stranici", "strana", "strani", "slika", "slici", "sliku", "tablica", "tablici", "tabela",
    "tabeli", "grupa", "grupi", "skupina", "skupini", u8"članak", u8"članku", u8"član", u8"članu", u8"točka",
    u8"točki", u8"tačka", u8"tački", "stavak", "stavku", "stav", "stavu", "verzija", "verziji", "inačica",
    u8"inačici", "kanal", "kanalu", "broj", "broju", "napomena", "napomeni", "prilog", "prilogu", "faza",
    "fazi", "korak", "koraku", "element", "red", "redu", "redak", "retku", "stupac", "stupcu", "kolona",
    "koloni", "kartica", "kartici", "gumb", "gumbu", "dugme", "dugmetu", "datoteka", "datoteci", "mapa", "mapi",
    "razina", "razini", "nivo", "nivou", "dio", "deo", "dijelu", "delu", "ulica", "ulici", "cesta", "cesti",
    "linija", "liniji", "autobus", "tramvaj", "soba", "sobi", "kat", "katu", "sprat", "spratu", "poglavlje",
    "poglavlju", "lekcija", "lekciji", "zadatak", "zadatku", "pitanje", "pitanju", "odgovor", "model", "tip",
    "tipka", "tipku", "taster", "pritisni", "pritisnite", "odaberi", "odaberite", "izaberi", "izaberite",
    "unesi", "unesite", "upiši", u8"upišite", "klikni", "kliknite", "dodirni", "dodirnite",
};
// The feminine nouns in a consonant (i-stems), the commonest, singular and
// the forms in -i; and every -ost, -est, -ad
const std::unordered_set<std::string_view> HR_FEMININE_I = {
    "stvar", u8"riječ", u8"reč", u8"noć", "kost", "sol", "so", "krv", "ljubav", "smrt", u8"moć", u8"pomoć",
    "obitelj", "misao", "pamet", u8"kćer", u8"kći", "vlast", "oblast", u8"čast", "strast", "mast", "zob",
    u8"peć", "vijest", "vest", "savjest", "savest", "svijest", "svest", "propast", "korist", u8"pošast",
    "cijev", "cev", u8"ćud", u8"čeljust", "glad", u8"žeđ", "ravan", u8"raž", "nit", "zvijer", "zver", u8"laž",
    u8"čađ", "kap", "jesen", u8"večer", "ovlast", "narav", "nakit", "pogibelj", "vrst", u8"četvrt", "svrha",
    "stvari", u8"riječi", u8"reči", u8"noći", "kosti", "soli", "krvi", "ljubavi", "smrti", u8"moći", u8"pomoći",
    "obitelji", "misli", "pameti", u8"kćeri", "vlasti", "oblasti", u8"časti", "strasti", "masti", u8"peći",
    "vijesti", "vesti", "savjesti", "savesti", "svijesti", "svesti", "propasti", "koristi", "cijevi", "cevi",
    u8"ćudi", u8"čeljusti", "gladi", u8"žeđi", "ravni", u8"raži", "niti", "zvijeri", "zveri", u8"laži", "kapi",
    "jeseni", u8"večeri", "ovlasti", "naravi", u8"četvrti", "djeca", "deca", "braća", "braca", "gospoda",
};
bool hr_feminine_i(std::string_view next) {
    return HR_FEMININE_I.count(next) != 0 || (sl_ends(next, "ost") && next.size() > 4) ||
           (sl_ends(next, "osti") && next.size() > 5) || (sl_ends(next, "est") && next.size() > 4) ||
           (sl_ends(next, "ad") && next.size() > 4 && (next == u8"čeljad" || next == "telad" || next == u8"momčad" ||
                                                       next == u8"unučad" || next == "burad" || next == "pilad"));
}
// Masculine nouns in -o and the neuter endings in -e
const std::unordered_set<std::string_view> HR_MASCULINE_O = {
    "auto", "radio", "kino", "euro", "evro", "video", "foto", "metro", "studio", "biro", "kimono", "torzo",
    "dinamo", "tango", "logo", "saldo", "konto", "tempo", "solo", "libreto", "kasino", "kazino",
};
bool hr_neuter_e(std::string_view next) {
    if (sl_ends(next, "ije")) return false;
    return sl_ends(next, "je") || sl_ends(next, "lje") || sl_ends(next, "nje") || sl_ends(next, "me") ||
           sl_ends(next, "te") || sl_ends(next, "re") || sl_ends(next, u8"đe");
}
// The units written as abbreviations, by the gender of their noun
const std::unordered_set<std::string_view> HR_MASCULINE_UNITS = {
    "m", "cm", "mm", "dm", "km", "kg", "g", "dag", "dkg", "mg", "l", "dl", "cl", "ml", "hl", "kb", "mb", "gb",
    "tb", "pb", "hz", "khz", "mhz", "ghz", "w", "kw", "mw", "kwh", "mwh", "bar", "mbar", "px", "dpi", "fps",
    "bit", "bajt", "h", "eur", "usd", "din", "dinar", "cent", "lipa", u8"tisuću", "hiljadu",
};
const std::unordered_set<std::string_view> HR_FEMININE_UNITS = {"min", "t", "s", "sek", "ms", "kn", "km"};

const char* const HR_MONTHS_HR[] = {
    u8"siječnja", u8"veljače", u8"ožujka", "travnja", "svibnja", "lipnja", "srpnja", "kolovoza", "rujna",
    "listopada", "studenoga", "studenog", "prosinca",
};
const char* const HR_MONTHS_SR[] = {
    "januara", "februara", "marta", "aprila", "maja", "juna", "jula", "avgusta", "augusta", "septembra",
    "oktobra", "novembra", "decembra",
};
bool hr_month_genitive(std::string_view w) {
    for (const char* m : HR_MONTHS_HR) if (w == m) return true;
    for (const char* m : HR_MONTHS_SR) if (w == m) return true;
    return false;
}

// The stem of an ordinal from the last word of the cardinal (prvi, drugi,
// treći, četvrti, peti, šesti, sedmi, osmi, deveti, deseti, jedanaesti,
// dvadeseti, dvadeset prvi, stoti, dvjestoti, tisućiti / hiljaditi,
// milijunti / milioniti)
std::string hr_ordinal_stem(std::string_view token) {
    if (token == "jedan" || token == "jedna" || token == "jedno") return "prv";
    if (token == "dva" || token == "dvije" || token == "dve") return "drug";
    if (token == "tri") return u8"treć";
    if (token == u8"četiri") return u8"četvrt";
    if (token == "sedam") return "sedm";
    if (token == "osam") return "osm";
    if (sl_ends(token, "sto")) return std::string(token) + "t";
    if (sl_ends(token, "sta")) return std::string(token.substr(0, token.size() - 1)) + "ot";      // dvesta: dvestot
    if (sl_starts(token, u8"tisuć")) return u8"tisućit";
    if (sl_starts(token, "hiljad")) return "hiljadit";
    if (sl_starts(token, "milijun")) return "milijunt";
    if (sl_starts(token, "milion")) return "miliont";
    if (sl_starts(token, "milijard")) return "milijardit";
    return std::string(token);      // pet, šest, devet, deset, the teens, the tens
}
bool hr_soft_stem(const std::string& stem) {
    return sl_ends(stem, u8"ć") || sl_ends(stem, u8"č") || sl_ends(stem, u8"š") || sl_ends(stem, u8"ž") ||
           sl_ends(stem, "j") || sl_ends(stem, u8"đ");
}

// The ending of an ordinal before the word `next`, after the word `prev`:
// "" keeps the cardinal
std::string_view hr_ordinal_ending(const std::string& prev, std::string_view next) {
    if (hr_month_genitive(next)) return "og";                             // sedmog listopada
    if (next.size() < 3) return "";
    if (HR_NOT_COUNTED.count(next) || sl_ends(next, "ti")) return "";
    const bool genitive = HR_WITH_GENITIVE.count(prev) != 0;
    const bool locative = HR_WITH_LOCATIVE.count(prev) != 0 || HR_WITH_DATIVE.count(prev) != 0;
    const bool instrumental = HR_WITH_INSTRUMENTAL.count(prev) != 0;
    static const std::unordered_set<std::string_view> before_subject = {
        "je", "su", "bio", "bila", "bilo", "bili", "bit", "biti", "nije", "nisu", "i", "ili", "pa", "te", "a",
        "ali", u8"također", u8"takođe", u8"još", "kao", "to", "ta", "taj", "ovo", "ova", "ovaj", "samo", "zatim",
        "potom", "onda", "sada", "sad", "danas", "npr", "odnosno", "dakle", "naime", u8"već",
    };
    if (sl_ends(next, "og") || sl_ends(next, "eg") || sl_ends(next, "oga") || sl_ends(next, "ega")) return "og";
    if (sl_ends(next, "im") || sl_ends(next, "ima") || sl_ends(next, "ama")) return "im";       // u devedesetim godinama
    if (sl_ends(next, "ih")) return "ih";
    if (sl_ends(next, "oj") || sl_ends(next, "ioj")) return "oj";
    if (sl_ends(next, "om") || sl_ends(next, "em")) return instrumental ? "im" : (sl_ends(next, "om") ? "om" : "em");
    if (next == "godina" && !prev.empty() && !before_subject.count(prev) && !genitive) return "ih";   // konac 80. godina
    switch (next.back()) {
        case 'a':
            if (genitive) return "og";                                                        // do trećeg razreda
            if (locative || instrumental) return "a";
            return prev.empty() || before_subject.count(prev) ? "a" : "og";                   // prva nagrada; odlomak trećeg članka
        case 'e': return genitive ? "e" : (hr_neuter_e(next) ? "o" : "e");                    // prvo dijete; druge godine
        case 'o': return HR_MASCULINE_O.count(next) ? "i" : "o";                              // prvo mjesto; prvi auto
        case 'u': return locative ? "om" : "u";                                               // na prvom mjestu; prvu nagradu
        case 'i': return locative ? "oj" : "i";                                               // u prvoj godini; prvi koraci
        default: return hr_feminine_i(next) ? "a" : "i";                                      // prva stvar; treći članak
    }
}

const SlUnit HR_UNITS_HR[] = {
    {"%", false, "posto", "posto", "posto", "posto", "posto", "posto"},
    {"\xE2\x82\xAC", false, "euro", "eura", "eura", "eura", "eura", "eura"},
    {"$", false, "dolar", "dolara", "dolara", "dolara", "dolara", "dolara"},
    {"\xC2\xB0", false, "stupanj", "stupnja", "stupnja", "stupnja", "stupnjeva", "stupnja"},
};
const SlUnit HR_UNITS_SR[] = {
    {"%", false, "posto", "posto", "posto", "posto", "posto", "posto"},
    {"\xE2\x82\xAC", false, "evro", "evra", "evra", "evra", "evra", "evra"},
    {"$", false, "dolar", "dolara", "dolara", "dolara", "dolara", "dolara"},
    {"\xC2\xB0", false, "stepen", "stepena", "stepena", "stepena", "stepeni", "stepena"},
};
const SlUnit HR_UNITS_BS[] = {
    {"%", false, "posto", "posto", "posto", "posto", "posto", "posto"},
    {"\xE2\x82\xAC", false, "euro", "eura", "eura", "eura", "eura", "eura"},
    {"$", false, "dolar", "dolara", "dolara", "dolara", "dolara", "dolara"},
    {"\xC2\xB0", false, "stepen", "stepena", "stepena", "stepena", "stepeni", "stepena"},
};

const SlUnit* hr_unit_at(const std::string& text, size_t pos, size_t& end, std::string_view& scale, const SlUnit* table,
                         bool croatian) {
    pos = sl_skip_spaces(text, pos);
    if (pos >= text.size()) return nullptr;
    for (size_t k = 0; k < 4; ++k) {
        const SlUnit& u = table[k];
        const size_t n = std::strlen(u.symbol);
        if (text.compare(pos, n, u.symbol) != 0) continue;
        end = pos + n;
        scale = "";
        if (k == 3 && end < text.size() && (end + 1 >= text.size() || !is_word_byte(text[end + 1]))) {
            if (text[end] == 'C' || text[end] == 'c') { scale = croatian ? " Celzija" : " Celzijusa"; ++end; }
            else if (text[end] == 'F' || text[end] == 'f') { scale = " Fahrenheita"; ++end; }
        }
        if (end < text.size() && is_letter_byte(text[end])) return nullptr;
        return &u;
    }
    return nullptr;
}

// The combining stem of a compound with a number (jednosobni, dvogodišnji,
// trogodišnja, četverosobni / četvorosobni, petodnevni, desetogodišnjica,
// dvadesetogodišnjak, stogodišnjica) from the cardinal's words
std::string hr_compound_stem(std::string words, bool croatian) {
    size_t token = words.rfind(' ');
    token = token == std::string::npos ? 0 : token + 1;
    const std::string last = words.substr(token);
    std::string stem;
    if (last == "jedan") stem = "jedno";
    else if (last == "dva") stem = "dvo";
    else if (last == "tri") stem = "tro";
    else if (last == u8"četiri") stem = croatian ? u8"četvero" : u8"četvoro";
    else if (last == "sedam") stem = "sedmo";
    else if (last == "osam") stem = "osmo";
    else if (sl_ends(last, "sto") || sl_ends(last, "sta")) stem = last;
    else stem = last + "o";
    words.replace(token, std::string::npos, stem);
    words.erase(std::remove(words.begin(), words.end(), ' '), words.end());
    return words;
}

} // namespace

// An ordinal with the adjectival ending the sentence asks for: prvi, prva,
// prvo, prvog, prvom, prvim, prvih, prve, prvu, prvoj; treći, treće, trećeg,
// trećem; dvadeset prvi (only the last word changes), stoti, tisućita,
// dvije tisuće dvadeset šeste
std::string CroatianNumbers::hr_ordinal(std::string_view number, std::string_view ending) {
    number = remove_leading_zeros(number);
    if (number.empty()) return "";
    std::string words = number_to_words(number);
    size_t token = words.rfind(' ');
    token = token == std::string::npos ? 0 : token + 1;
    std::string stem = hr_ordinal_stem(std::string_view(words).substr(token));
    std::string end(ending);
    if (hr_soft_stem(stem)) {
        if (end == "o") end = "e";
        else if (end == "og") end = "eg";
        else if (end == "om") end = "em";
    }
    words.replace(token, std::string::npos, stem + end);
    return words;
}

std::string CroatianNumbers::hr_ordinal_before(const std::string& text, size_t start, size_t end, size_t after) {
    const std::string_view number(text.data() + start, end - start);
    if (remove_leading_zeros(number) == "0") return "";
    const SlNext next_raw = sl_next_word(text, after);
    if (next_raw.word.empty()) return "";
    for (char c : next_raw.word) {
        if (is_digit(c)) return "";
    }
    const std::string next = hr_latin(next_raw.word, nullptr);
    const SlPrev prev_raw = sl_prev_word(text, start);
    const std::string prev = hr_latin(prev_raw.word, nullptr);
    const std::string_view ending = hr_ordinal_ending(prev, next);
    if (ending.empty()) return "";
    return hr_ordinal(number, ending);
}

// A number with a unit's sign: jedan euro, dva eura, pet eura, dvadeset jedan
// euro, dvadeset dva eura, jedan stupanj, tri stupnja, pet stupnjeva Celzija,
// jedan posto; the decimal takes the genitive singular
std::string CroatianNumbers::hr_count(std::string_view digits, const SlUnit& unit, std::string_view scale) {
    std::string words = number_to_words(digits);
    const std::string_view clean = remove_leading_zeros(digits);
    const char last = clean.back();
    const bool teen = clean.size() >= 2 && clean[clean.size() - 2] == '1';
    const char* form = unit.genitive_plural;
    if (!teen && last == '1') form = unit.singular;
    else if (!teen && last >= '2' && last <= '4') form = unit.dual;
    words += ' ';
    words += form;
    words += scale;
    return words;
}

// A decimal as the shared path reads it: "jedan zarez pet", up to two digits
// after the comma as one number, more one by one, leading zeros as nula
std::string CroatianNumbers::hr_decimal(std::string_view whole, std::string_view fraction) {
    std::string words = number_to_words(whole);
    words += separator_words(',');
    std::string_view rest = fraction;
    bool first = true;
    while (rest.size() > 1 && rest[0] == '0') {
        if (!first) words += ' ';
        words += zero_word();
        rest.remove_prefix(1);
        first = false;
    }
    if (rest.size() <= 2) {
        if (!first) words += ' ';
        words += rest.size() == 1 ? std::string(digit_name(rest[0])) : number_to_words(rest);
    } else {
        for (char d : rest) {
            if (!first) words += ' ';
            words += digit_name(d);
            first = false;
        }
    }
    return words;
}

// jedan, jedna, jedno; dva, dvije (dve): the last word of a number agrees
// with the noun after it in gender, and 1 in case after a preposition (jedna
// kuća, jedno selo, jedan stol, jedna stvar; dvije kuće, dva stola, dvije
// noći; s jednim prijateljem, u jednom satu, u jednoj godini, do jedne godine,
// za jednu kunu; dvadeset jedna kuća, sto dvije kuće). Nothing counted keeps
// jedan, dva: alone, in a list, before a sign, after a label (stranica 2),
// glued to letters or a hyphen (T2, A-2), after a name (Pentium 2).
void CroatianNumbers::hr_agree(const std::string& text, size_t start, size_t end, std::string& words) {
    size_t token = words.rfind(' ');
    token = token == std::string::npos ? 0 : token + 1;
    const std::string_view last = std::string_view(words).substr(token);
    int digit = 0;
    if (last == "jedan") digit = 1;
    else if (last == "dva") digit = 2;
    if (digit == 0) return;
    auto say = [&](std::string_view form) { words.replace(token, std::string::npos, form); };
    const bool ekavian = m_dialect == Dialect::Serbian;
    const std::string_view two_f = ekavian ? "dve" : "dvije";

    if (start > 0 && (is_letter_byte(text[start - 1]) || text[start - 1] == '-')) return;
    const SlPrev prev_raw = sl_prev_word(text, start);
    if (prev_raw.end >= 2 && (text[prev_raw.end - 1] == '-' || text[prev_raw.end - 1] == '/') &&
        is_letter_byte(text[prev_raw.end - 2])) {
        return;
    }
    bool prev_capital = false;
    const std::string prev = hr_latin(prev_raw.word, &prev_capital);
    if (prev_capital) {
        const char head = prev_raw.ahead > 0 ? text[prev_raw.ahead - 1] : '.';
        if (head != '.' && head != '!' && head != '?' && head != ':' && head != ';' && head != '\n' && head != ',' &&
            head != '(') {
            return;
        }
    }
    if (HR_LABELS.count(prev)) return;

    const SlNext next_raw = sl_next_word(text, end);
    if (next_raw.word.empty() || next_raw.begin == end) return;        // nothing, a mark or a glued letter
    for (char c : next_raw.word) {
        if (is_digit(c)) return;
    }
    const std::string next = hr_latin(next_raw.word, nullptr);
    if (!next_raw.lowercase && !hr_latin(next_raw.word, nullptr).empty()) {
        // A capital: a name of either gender (2 Hrvata), unless a unit (MB)
        const std::string unit = sl_ascii_lower(next_raw.word);
        if (!HR_MASCULINE_UNITS.count(unit) && !HR_FEMININE_UNITS.count(unit)) return;
    }
    auto ends = [&](std::string_view tail) { return sl_ends(next, tail); };

    const std::string unit = sl_ascii_lower(next);
    if (HR_MASCULINE_UNITS.count(unit)) return;                                   // jedan kg, dva h
    if (HR_FEMININE_UNITS.count(unit)) { say(digit == 1 ? "jedna" : two_f); return; }   // jedna min, dvije kn
    if (next.size() < 3) return;
    if (HR_NOT_COUNTED.count(next) || ends("ti")) return;

    const bool genitive = HR_WITH_GENITIVE.count(prev) != 0 && !HR_WITH_INSTRUMENTAL.count(prev);
    const bool locative = HR_WITH_LOCATIVE.count(prev) != 0;
    const bool dative = HR_WITH_DATIVE.count(prev) != 0;
    const bool instrumental = HR_WITH_INSTRUMENTAL.count(prev) != 0;
    const char tail = next.back();

    if (digit == 1) {
        if ((genitive || instrumental) && tail == 'a' && !ends("ima")) { say("jednog"); return; }   // do jednog sata, s jednog mjesta
        if (genitive && tail == 'e') { say("jedne"); return; }                                       // od jedne godine
        if ((locative || dative) && tail == 'u') { say("jednom"); return; }                          // u jednom satu
        if ((locative || dative) && tail == 'i') { say("jednoj"); return; }                          // u jednoj godini
        if (instrumental && (ends("om") || ends("em"))) { say("jednim"); return; }                   // s jednim prijateljem
        if (ends("ima")) { say("jednim"); return; }
        if (tail == 'a') { say("jedna"); return; }                                                   // jedna kuća
        if (tail == 'o') { if (!HR_MASCULINE_O.count(next)) say("jedno"); return; }                  // jedno selo; jedan auto
        if (tail == 'e') { say(hr_neuter_e(next) ? "jedno" : "jedne"); return; }                     // jedno dijete; jedne godine
        if (tail == 'u') { say("jednu"); return; }                                                   // jednu kunu
        if (tail == 'i') { if (hr_feminine_i(next)) say("jedne"); return; }                          // jedne noći
        if (hr_feminine_i(next)) say("jedna");                                                       // jedna stvar
        return;
    }
    // 2: feminine before -e and the feminine i-stems (dvije kuće, dvije noći),
    // dva otherwise (dva stola, dva sela, dva dana)
    if (tail == 'e' || (tail == 'i' && hr_feminine_i(next))) say(two_f);
}

// The Croatian, Serbian and Bosnian constructs at position i (a digit) that
// the shared loop does not know; see slovenian_number_at() for the shape
size_t CroatianNumbers::hr_number_at(const std::string& text, size_t i, std::string& result, bool in_range) {
    const size_t length = text.size();
    const size_t end = digits_end(text, i);
    const std::string_view digits(text.data() + i, end - i);
    const bool leading_zero = text[i] == '0' && end - i > 1;
    const long value = sl_value(digits);
    const bool croatian = m_dialect == Dialect::Croatian;
    const SlUnit* const units = croatian ? HR_UNITS_HR : m_dialect == Dialect::Serbian ? HR_UNITS_SR : HR_UNITS_BS;
    const SlPrev prev_raw = sl_prev_word(text, i);
    bool prev_capital = false;
    const std::string prev = hr_latin(prev_raw.word, &prev_capital);
    std::vector<std::string_view> groups;
    auto unit_at = [&](size_t pos, size_t& u_end, std::string_view& scale) {
        return hr_unit_at(text, pos, u_end, scale, units, croatian);
    };

    // A minus glued to the number (Hrvatski pravopis: −5 °C), after a space,
    // a bracket or at the start, so that T-2 and 1991-2000 keep their hyphen
    if (!in_range && i > 0) {
        size_t sign = 0;
        if (text[i - 1] == '-') {
            sign = 1;
        } else if (i >= 3 && static_cast<unsigned char>(text[i - 3]) == 0xE2 &&
                   ((static_cast<unsigned char>(text[i - 2]) == 0x88 && static_cast<unsigned char>(text[i - 1]) == 0x92) ||
                    (static_cast<unsigned char>(text[i - 2]) == 0x80 && static_cast<unsigned char>(text[i - 1]) == 0x93))) {
            sign = 3;
        }
        if (sign > 0) {
            const size_t at = i - sign;
            const unsigned char b = at > 0 ? static_cast<unsigned char>(text[at - 1]) : ' ';
            if (b == ' ' || b == '\t' || b == '\n' || b == '(' || b == '[' || b == '"' || b == '\'' || b == ':' || b == 0xA0 ||
                b == 0x9E || b == 0x9C) {
                size_t r = result.size();
                if (sign == 3 && r > 0 && result[r - 1] == ' ') --r;
                if (r >= sign && result.compare(r - sign, sign, text, at, sign) == 0) {
                    result.erase(r - sign);
                    if (!result.empty() && is_word_byte(result.back())) result += ' ';
                    result += "minus ";
                }
            }
        }
    }
    if (leading_zero) return 0;

    // A range of two times: 8.30–16.00, 8:30–16:00
    if (!in_range) {
        size_t left_stop = 0;
        if (end + 2 < length && (text[end] == '.' || text[end] == ':') && digits_end(text, end + 1) == end + 3) {
            left_stop = end + 3;
        }
        if (left_stop > 0 && left_stop + 2 < length && static_cast<unsigned char>(text[left_stop]) == 0xE2 &&
            static_cast<unsigned char>(text[left_stop + 1]) == 0x80 &&
            (static_cast<unsigned char>(text[left_stop + 2]) == 0x93 || static_cast<unsigned char>(text[left_stop + 2]) == 0x94) &&
            left_stop + 3 < length && is_digit(text[left_stop + 3])) {
            std::string left_words, right_words;
            const size_t l = hr_number_at(text, i, left_words, true);
            const size_t r = l == left_stop ? hr_number_at(text, left_stop + 3, right_words, true) : 0;
            if (l == left_stop && r > 0) {
                result += left_words + " do " + right_words;
                return r;
            }
        }
    }

    // A full date: the day and the month as genitive ordinals, the year as
    // the feminine ordinal of "godine" (sedmog desetog dvije tisuće dvadeset
    // šeste); after "je" the nominative (Danas je sedmi deseti)
    // "Danas je 7. 10." names the day, the nominative; "Rođen je 7. 10." says
    // when, the genitive: only after danas je, datum je, to je ...
    static const std::unordered_set<std::string_view> names_the_day = {
        "danas", "sutra", u8"jučer", u8"juče", "prekosutra", u8"prekjučer", "datum", "to", "ovo", "dan", "rok",
    };
    const bool nominative_date = prev == "je" && names_the_day.count(hr_latin(sl_prev_word(text, prev_raw.begin).word, nullptr)) != 0;
    if (size_t d_end = parse_date(text, i, groups)) {
        const char* day_month = nominative_date ? "i" : "og";
        result += hr_ordinal(groups[0], day_month) + ' ' + hr_ordinal(groups[1], day_month) + ' ' +
                  hr_ordinal(groups[2], "e");
        return d_end;
    }

    // A clock time with a colon: the shared path, 24:00 too
    if (size_t t_end = parse_time(text, i, groups)) {
        if (in_range) {
            result += time_to_words(groups);
            return t_end;
        }
        return 0;
    }
    if (digits == "24" && text.compare(end, 3, ":00") == 0 && (end + 3 >= length || (!is_digit(text[end + 3]) && text[end + 3] != ':'))) {
        result += u8"dvadeset četiri nula nula";
        return end + 3;
    }

    // A time written with a period (u 8.30, od 8.30 do 16.00) in the context
    // of a time; 3.14 stays a decimal point read by name, 7.10. a date
    static const std::unordered_set<std::string_view> before_time = {
        "u", "od", "do", "oko", u8"između", "izmedju", "iza", "prije", "pre", "poslije", "posle", "nakon",
        u8"točno", u8"tačno", "sati", "sat", "sata",
    };
    static const std::unordered_set<std::string_view> after_time = {
        "h", "sati", "sat", "sata", "ujutro", u8"navečer", u8"uvečer", u8"uveče", "popodne", "poslijepodne",
        "poslepodne", "prijepodne", "prepodne",
    };
    if (end - i <= 2 && value >= 0 && value <= 24 && end + 2 < length && text[end] == '.' &&
        digits_end(text, end + 1) == end + 3) {
        const std::string_view minutes(text.data() + end + 1, 2);
        const long mm = sl_value(minutes);
        const size_t after = end + 3;
        const bool glued_more = after < length && ((text[after] == '.' || text[after] == ',') && after + 1 < length &&
                                                   is_digit(text[after + 1]));
        const bool colon_after = after < length && text[after] == ':';
        const SlNext next_raw = sl_next_word(text, after);
        const std::string next = hr_latin(next_raw.word, nullptr);
        const bool date_like = after < length && text[after] == '.' && prev != "u" && mm >= 1 && mm <= 12 &&
                               value >= 1 && value <= 31;
        const bool after_sada = prev == "je" && prev_raw.ahead >= 3 &&
                                (sl_ascii_lower(std::string_view(text).substr(prev_raw.ahead - 3, 3)) == "sad" ||
                                 (prev_raw.ahead >= 4 && sl_ascii_lower(std::string_view(text).substr(prev_raw.ahead - 4, 4)) == "sada"));
        const bool context = in_range || before_time.count(prev) != 0 || after_sada ||
                             (next_raw.lowercase && after_time.count(next) != 0 && next_raw.begin <= after + 1);
        if (mm <= 59 && (value < 24 || mm == 0) && !glued_more && !colon_after && !date_like && context) {
            groups = {digits, minutes};
            result += time_to_words(groups);
            return after;
        }
    }

    // A date without a year: 7. 10., 7.10. (sedmog desetog)
    auto short_date = [&](size_t pos, std::string& words, size_t& stop) -> bool {
        const size_t d_end = digits_end(text, pos);
        if (d_end == pos || d_end - pos > 2 || text[pos] == '0' || d_end >= length || text[d_end] != '.') return false;
        const long day = sl_value(std::string_view(text.data() + pos, d_end - pos));
        if (day < 1 || day > 31) return false;
        size_t m = d_end + 1;
        if (m < length && text[m] == ' ') ++m;
        const size_t m_end = digits_end(text, m);
        if (m_end == m || m_end - m > 2 || m_end >= length || text[m_end] != '.') return false;
        const long month = sl_value(std::string_view(text.data() + m, m_end - m));
        if (month < 1 || month > 12) return false;
        size_t y = m_end + 1;
        if (y < length && text[y] == ' ') ++y;
        if (y < length && is_digit(text[y])) return false;
        const SlNext next = sl_next_word(text, m_end + 1);
        static const std::unordered_set<std::string_view> joining = {"i", "ili", "do", "te", "odnosno", "pa"};
        if (next.lowercase && joining.count(hr_latin(next.word, nullptr))) return false;
        const char* day_month = nominative_date && pos == i ? "i" : "og";      // Danas je sedmi deseti
        words = hr_ordinal(std::string_view(text.data() + pos, d_end - pos), day_month) + ' ' +
                hr_ordinal(std::string_view(text.data() + m, m_end - m), day_month);
        if (next.lowercase && next.begin > m_end + 1) {
            stop = m_end + 1;
        } else if (next.lowercase) {
            stop = m_end + 1;
            words += ' ';
        } else {
            stop = m_end;
        }
        return true;
    };

    // A range: 1991.–2000., od 1991. do 2000., 2–3 dana, 15.–20. ožujka,
    // 3.–4. mjesto, u 7. i 8. razredu, 1991-2000, 10–15 %
    if (!in_range) {
        size_t left_end = end;
        bool dotted = false;
        if (left_end < length && text[left_end] == '.' && !(left_end + 1 < length && is_digit(text[left_end + 1]))) {
            dotted = true;
            ++left_end;
        }
        size_t c = left_end;
        std::string connector;
        size_t sp = sl_skip_spaces(text, c);
        if (sp + 2 < length && static_cast<unsigned char>(text[sp]) == 0xE2 && static_cast<unsigned char>(text[sp + 1]) == 0x80 &&
            (static_cast<unsigned char>(text[sp + 2]) == 0x93 || static_cast<unsigned char>(text[sp + 2]) == 0x94) &&
            (sp == c || dotted)) {
            connector = "dash";
            c = sl_skip_spaces(text, sp + 3);
        } else if (c < length && text[c] == '-' && !dotted) {
            connector = "hyphen";
            c += 1;
        } else if (dotted) {
            const SlNext word = sl_next_word(text, c);
            static const std::unordered_set<std::string_view> joining = {"do", "i", "ili", "te", "odnosno"};
            const std::string w = hr_latin(word.word, nullptr);
            if (word.begin > c && word.lowercase && joining.count(w)) {
                connector = w;
                c = sl_skip_spaces(text, word.end);
            }
        }
        if (!connector.empty() && c < length && is_digit(text[c]) && text[c] != '0') {
            const size_t r_end = digits_end(text, c);
            const std::string_view right(text.data() + c, r_end - c);
            const std::string join = connector == "dash" || connector == "hyphen" ? " do " : " " + connector + " ";
            std::string words;
            size_t stop = 0;
            if (dotted) {
                if (size_t d = parse_date(text, c, groups)) {
                    result += hr_ordinal(digits, "og") + join + hr_ordinal(groups[0], "og") + ' ' +
                              hr_ordinal(groups[1], "og") + ' ' + hr_ordinal(groups[2], "e");
                    return d;
                }
                if (short_date(c, words, stop)) {
                    result += hr_ordinal(digits, "og") + join + words;
                    return stop;
                }
                if (r_end < length && text[r_end] == '.' && !(r_end + 1 < length && is_digit(text[r_end + 1]))) {
                    const SlNext noun_raw = sl_next_word(text, r_end + 1);
                    const std::string noun = hr_latin(noun_raw.word, nullptr);
                    if (noun_raw.lowercase && r_end - c <= 4) {
                        const size_t after = r_end + 1;
                        const std::string_view ending = hr_month_genitive(noun) ? "og" : hr_ordinal_ending(prev, noun);
                        if (ending.empty()) {
                            result += number_to_words(digits) + join + number_to_words(right);
                        } else {
                            result += hr_ordinal(digits, ending) + join + hr_ordinal(right, ending);
                        }
                        if (text[after] != ' ' && text[after] != '\t') result += ' ';
                        return after;
                    }
                    // Two years, nothing counted after them: od 1991. do 2000.
                    // (tisuću devetsto devedeset prve do dvije tisućite godine)
                    if (end - i == 4 && r_end - c == 4) {
                        result += hr_ordinal(digits, "e") + join + hr_ordinal(right, "e");
                        return r_end;      // the period ends the sentence as before
                    }
                }
                return 0;
            }
            if (connector == "hyphen" &&
                (sl_value(right) <= value || r_end - c > 4 || !(end - i <= 2 || (end - i == 4 && r_end - c == 4)))) {
                return 0;
            }
            if (text[end] == '.' || text[end] == ',') return 0;
            if (is_separator_between_digits(text, r_end)) return 0;
            std::string left_words = number_to_words(digits);
            size_t unit_end = 0;
            std::string_view scale;
            if (const SlUnit* unit = unit_at(r_end, unit_end, scale)) {
                result += left_words + join + hr_count(right, *unit, scale);
                return unit_end;
            }
            std::string right_words = number_to_words(right);
            hr_agree(text, i, r_end, left_words);
            hr_agree(text, i, r_end, right_words);
            result += left_words + join + right_words;
            return r_end;
        }
    }

    {
        std::string words;
        size_t stop = 0;
        if (!in_range && short_date(i, words, stop)) {
            if (!(prev.empty() && prev_raw.ahead >= 2 && text[prev_raw.ahead - 1] == '.' && is_digit(text[prev_raw.ahead - 2]))) {
                result += words;
                return stop;
            }
        }
    }

    // Thousands separated by spaces or by periods (1 000, 1.000, 1.000.000,
    // 1.000,50; Hrvatski pravopis: the groups of three digits are set off
    // by a period or a space, the decimal mark is the comma): one number.
    // Not after a digit, a plus, a bracket or a slash (+385 91 123 4567),
    // not after telefon or broj, not after a name inside a sentence.
    if (end - i <= 3) {
        for (int pass = 0; pass < 2; ++pass) {
            const bool spaced = pass == 0;
            size_t pos = end;
            std::string joined(digits);
            int group_count = 0;
            while (group_count < 4) {
                size_t next = pos;
                if (spaced) {
                    next = sl_skip_spaces(text, pos);
                    if (next == pos) break;
                } else {
                    if (!(pos < length && text[pos] == '.')) break;
                    next = pos + 1;
                }
                if (digits_end(text, next) != next + 3) break;
                joined.append(text, next, 3);
                pos = next + 3;
                ++group_count;
            }
            if (group_count == 0) continue;
            size_t before_pos = prev.empty() ? prev_raw.end : prev_raw.begin;
            while (before_pos > 0 && (text[before_pos - 1] == ' ' || text[before_pos - 1] == '\t')) --before_pos;
            const char before = before_pos > 0 ? text[before_pos - 1] : ' ';
            static const std::unordered_set<std::string_view> telephone = {
                "telefon", "telefona", "telefonu", "tel", "mob", "mobitel", "mobitela", "faks", "fax", "broj",
                "broju", "brojem", u8"žiro", "iban", u8"račun", u8"računa", u8"računu", u8"poštanski", "nazovite",
                "nazovi", "zovite", "zovi", "pozovite", "pozovi", "gsm",
            };
            const bool phone = telephone.count(prev) != 0 || (!prev.empty() && (is_digit(before) || is_digit(prev.back()))) ||
                               (prev.empty() && (is_digit(before) || before == '+' || before == ')' || before == '/'));
            const char head = prev_raw.ahead > 0 ? text[prev_raw.ahead - 1] : '.';
            const bool after_name = prev_capital && head != '.' && head != '!' && head != '?' && head != ':' &&
                                    head != ';' && head != '\n' && head != ',' && head != '(';
            if (phone || after_name) continue;
            if (pos < length && (is_letter_byte(text[pos]) || text[pos] == ':')) continue;
            if (pos < length && text[pos] == '.' && pos + 1 < length && is_digit(text[pos + 1])) continue;
            size_t stop = pos;
            std::string words;
            if (pos < length && text[pos] == ',' && pos + 1 < length && is_digit(text[pos + 1])) {
                // 1.000,50: the decimal comma after the thousands
                const size_t f_end = digits_end(text, pos + 1);
                if (is_separator_between_digits(text, f_end)) continue;
                words = hr_decimal(joined, std::string_view(text.data() + pos + 1, f_end - pos - 1));
                stop = f_end;
                size_t unit_end = 0;
                std::string_view scale;
                if (const SlUnit* unit = unit_at(stop, unit_end, scale)) {
                    words += ' ';
                    words += unit->genitive_singular;
                    words += scale;
                    stop = unit_end;
                }
            } else {
                size_t unit_end = 0;
                std::string_view scale;
                if (const SlUnit* unit = unit_at(pos, unit_end, scale)) {
                    words = hr_count(joined, *unit, scale);
                    stop = unit_end;
                } else {
                    words = number_to_words(joined);
                    hr_agree(text, i, pos, words);
                }
            }
            result += words;
            return stop;
        }
    }

    // A number glued to x or compounded with a word by a hyphen: 2x dva puta,
    // 1x jedanput, 2-godišnji dvogodišnji, 3-godišnja trogodišnja,
    // 10-godišnjica desetogodišnjica, 5-dnevni petodnevni, 1-sobni jednosobni,
    // 4-sobni četverosobni, 20-godišnjak dvadesetogodišnjak
    if (end - i <= 4 && (i == 0 || !is_word_byte(text[i - 1])) && end < length) {
        const std::string cardinal = number_to_words(digits);
        if (text[end] == '-' && end + 1 < length && is_lowercase_letter(code_point_at(text, end + 1))) {
            size_t stop = end + 1;
            while (stop < length && is_word_byte(text[stop]) && !is_digit(text[stop])) ++stop;
            const std::string word = hr_latin(text.substr(end + 1, stop - end - 1), nullptr);
            if (word.size() >= 4 && !(stop < length && is_digit(text[stop])) && word.compare(0, 4, "puta") != 0 &&
                word.compare(0, 3, "put") != 0) {
                result += hr_compound_stem(cardinal, croatian);
                result += word;
                return stop;
            }
            if (word.compare(0, 3, "put") == 0) {
                result += digits == "1" ? "jedanput" : cardinal + " puta";
                return stop;
            }
        }
        const bool times_x = text[end] == 'x' && (end + 1 >= length || !is_letter_byte(text[end + 1]));
        const bool times_sign = end + 1 < length && static_cast<unsigned char>(text[end]) == 0xC3 &&
                                static_cast<unsigned char>(text[end + 1]) == 0x97 &&
                                (end + 2 >= length || !is_letter_byte(text[end + 2]));
        if (times_x || times_sign) {
            result += digits == "1" ? "jedanput" : cardinal + " puta";
            return end + (times_x ? 1 : 2);
        }
    }

    // A decimal with a unit's sign: 1,5 € jedan zarez pet eura
    if (end < length && text[end] == ',' && is_separator_between_digits(text, end)) {
        const size_t f_end = digits_end(text, end + 1);
        if (!is_separator_between_digits(text, f_end)) {
            size_t unit_end = 0;
            std::string_view scale;
            if (const SlUnit* unit = unit_at(f_end, unit_end, scale)) {
                result += hr_decimal(digits, std::string_view(text.data() + end + 1, f_end - end - 1));
                result += ' ';
                result += unit->genitive_singular;
                result += scale;
                return unit_end;
            }
        }
    }

    // A whole number with a unit's sign: 1 € jedan euro, 2 € dva eura, 5 °C
    // pet stupnjeva Celzija, 1 % jedan posto
    if (!is_separator_between_digits(text, end)) {
        size_t unit_end = 0;
        std::string_view scale;
        if (const SlUnit* unit = unit_at(end, unit_end, scale)) {
            result += hr_count(digits, *unit, scale);
            return unit_end;
        }
    }
    return 0;
}

std::string CroatianNumbers::convert_numbers_in_text(const std::string& text) {
    std::string result;
    result.reserve(text.size() * 2);  // Estimate - numbers expand to words

    size_t i = 0;
    size_t length = text.size();
    bool after_number = false;

    while (i < length) {
        // Find start of non-digit text
        size_t text_start = i;
        while (i < length && !is_digit(text[i])) {
            i++;
        }

        // Append non-digit text unchanged, set off from a number it is
        // glued to ("20:03h", "2x")
        if (i > text_start) {
            if (after_number && is_letter_byte(text[text_start])) result += ' ';
            result.append(text, text_start, i - text_start);
        }

        if (i >= length) break;
        {
            size_t digits = digits_end(text, i);
            bool compound = (digits < length && text[digits] == ':') ||
                            is_separator_between_digits(text, digits);
            set_off_from_letters(result, compound);
        }
        after_number = true;

        // What Slovenian reads in its own way: a minus, the hours after ob, a
        // range, a date without a year, thousands with spaces, 2-krat and
        // 2-letni, a unit's noun
        if (size_t end = slovenian() ? slovenian_number_at(text, i, result) : hr_number_at(text, i, result)) {
            i = end;
            continue;
        }

        // A clock time or a date is read as such, without the separators:
        // the colon of "12:30" and the periods of "7.10.2026." are silent.
        if (size_t end = convert_time_or_date(text, i, result)) {
            i = end;
            continue;
        }

        // Slovenian writes a period between the groups of three digits of a
        // large number (132.457: sto dvaintrideset tisoč štiristo
        // sedeminpetdeset; Slovenski pravopis 2001, § 253), so such a
        // number is read whole. A period before fewer digits (3.14) is the
        // decimal point, read by name, as below.
        if (slovenian() && text[i] != '0') {
            size_t first_end = digits_end(text, i);
            size_t end = first_end;
            int groups = 0;
            while (end + 3 < length + 0 && end < length && text[end] == '.' &&
                   digits_end(text, end + 1) == end + 4) {
                end += 4;
                ++groups;
            }
            if (groups > 0 && first_end - i <= 3 && !is_separator_between_digits(text, end) &&
                !(end < length && text[end] == ':')) {
                std::string digits;
                for (size_t k = i; k < end; ++k) {
                    if (is_digit(text[k])) digits += text[k];
                }
                std::string words = number_to_words(digits);
                slovenian_agree(text, i, end, words);
                result += words;
                i = end;
                continue;
            }
        }

        // Handle leading zeros specially
        // Each leading zero becomes "nula ", set off from a preceding letter
        // but glued to a separator ("25:00" must stay one token)
        while (i < length && text[i] == '0') {
            // Check if this is a leading zero (more digits follow)
            if (i + 1 < length && is_digit(text[i + 1])) {
                if (!result.empty()) {
                    unsigned char back = static_cast<unsigned char>(result.back());
                    if (std::isalnum(back) || back >= 0x80) result += ' ';
                }
                result += zero_word();
                result += ' ';
                i++;
            } else {
                // This is the last digit (either standalone 0 or end of number)
                break;
            }
        }

        // Find end of number sequence
        size_t num_start = i;
        i = digits_end(text, i);

        // "7. listopada", "1990. godine": the period is silent. Slovenian
        // reads the ordinal in the case the noun and the preposition show
        // (sedmega oktobra, prvo mesto, na prvem mestu: slovenian_ordinal_before()).
        if (size_t end = silent_dot_end(text, num_start, i)) {
            std::string_view number(text.data() + num_start, i - num_start);
            std::string ordinal;
            ordinal = slovenian() ? slovenian_ordinal_before(text, num_start, i, end)
                                  : hr_ordinal_before(text, num_start, i, end);
            result += ordinal.empty() ? number_to_words(number) : ordinal;
            if (text[end] != ' ' && text[end] != '\t') result += ' ';   // "7.listopada"
            i = end;
            continue;
        }

        // Convert number to words
        if (i > num_start) {
            std::string_view num_str(text.data() + num_start, i - num_start);
            std::string words = number_to_words(num_str);
            if (!words.empty()) {
                if (slovenian()) slovenian_agree(text, num_start, i, words);
                else hr_agree(text, num_start, i, words);
                result += words;
            }
        }

        // Digits after a period or comma glued to the number. A period is
        // read by name and every group of digits as a whole number, as
        // eSpeak reads it ("1.317" -> jedan.tristo sedamnaest, "3.14" ->
        // tri.četrnaest, "192.168.1.1" and "2.0.1" group by group), leading
        // zeros each as "nula" ("3.05" -> tri.nula pet). One comma is the
        // decimal comma (eSpeak's rule for Croatian): up to two digits after
        // it are read as one number, more are read one by one ("3,14" -> tri
        // zarez četrnaest, "3,14159" -> tri zarez jedan četiri jedan pet
        // devet); with two or more commas every group is a whole number.
        // In a phone number spelled out digit by digit the comma is a
        // pause between the groups.
        if (is_grouping_comma(text, i)) {
            result += ", ";
            ++i;
            continue;
        }
        if (!is_separator_between_digits(text, i)) continue;

        size_t separators = 0;
        for (size_t k = i; is_separator_between_digits(text, k); k = digits_end(text, k + 1)) {
            ++separators;
        }

        while (is_separator_between_digits(text, i)) {
            char separator = text[i];
            size_t group_start = i + 1;
            size_t group_end = digits_end(text, group_start);
            std::string_view group(text.data() + group_start, group_end - group_start);
            if (slovenian() && separator == ',' && separators == 1 && i > num_start) {
                // tri cele štirinajst: the whole part was already written
                std::string_view whole(text.data() + num_start, i - num_start);
                std::string words = number_to_words(whole);
                result.resize(result.size() - words.size());
                result += slovenian_decimal(whole, group);
                i = group_end;
                continue;
            }
            result += separator_words(separator);
            if (separator == '.' || separators >= 2) {
                result += digit_group_to_words(group);
            } else {
                std::string_view rest = group;
                bool first = true;
                while (rest.size() > 1 && rest[0] == '0') {
                    if (!first) result += ' ';
                    result += zero_word();
                    rest.remove_prefix(1);
                    first = false;
                }
                if (rest.size() <= 2) {
                    if (!first) result += ' ';
                    if (rest.size() == 1) result += digit_name(rest[0]);   // 3.2, 3.02
                    else result += number_to_words(rest);
                } else {
                    for (char d : rest) {
                        if (!first) result += ' ';
                        result += digit_name(d);
                        first = false;
                    }
                }
            }
            i = group_end;
        }
    }

    return result;
}

// =============================================================================
// Single Digit to Croatian Word (public method)
// =============================================================================

std::string CroatianNumbers::digit_to_croatian_word(char digit) {
    return std::string(digit_name(digit));
}

// =============================================================================
// Convert Digits in Text (digit-by-digit mode)
// =============================================================================

std::string CroatianNumbers::convert_digits_in_text(const std::string& text) {
    std::string result;
    result.reserve(text.size() * 4);  // Estimate - each digit becomes a word

    size_t i = 0;
    size_t length = text.size();

    while (i < length) {
        // Find start of non-digit text
        size_t text_start = i;
        while (i < length && (text[i] < '0' || text[i] > '9')) {
            i++;
        }

        // Append non-digit text unchanged
        if (i > text_start) {
            result.append(text, text_start, i - text_start);
        }

        if (i >= length) break;

        // A time or a date is not a string of digits: read it as in the
        // whole-number mode
        if (size_t end = convert_time_or_date(text, i, result)) {
            i = end;
            continue;
        }

        // Convert each digit to its word form; a period or comma glued
        // between digits is read by name ("3.14" -> tri.jedan četiri),
        // except the comma between the groups of a phone number
        bool first_digit = true;
        while (i < length && (is_digit(text[i]) || is_separator_between_digits(text, i))) {
            if (!is_digit(text[i])) {
                result += is_grouping_comma(text, i) ? ", " : separator_words(text[i]);
                first_digit = true;   // glued to the period, or after the spaces of " zarez "
                i++;
                continue;
            }
            if (!first_digit) {
                result += " ";
            }
            result += digit_name(text[i]);
            first_digit = false;
            i++;
        }
    }

    return result;
}

} // namespace laprdus
