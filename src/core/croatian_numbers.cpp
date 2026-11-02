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

std::string_view CroatianNumbers::digit_to_word(char digit) {
    if (slovenian()) {
        // The words of the digits (Slovenski pravopis 2001, § 11: nič, ena,
        // dve, tri, štiri ...). A 2 with nothing to agree with is dva, as a
        // Slovenian listener asked on 2026-11-02 (T2, stran 2, 2 plus 2);
        // CroatianNumbers::slovenian_agree() and the callers that
        // count something feminine make it dve. en/ena and dva/dve agree
        // with a scale word in process_number_groups, and the compounds
        // have dva- (dvaindvajset)
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

// The genitive of a Slovenian ordinal of 1 to 99, as a date is read
// (sedmega oktobra, enaintridesetega): the cardinal's stem and -ega, with
// the stems prv-, drug-, tretj-, četrt-, sedm-, osm- and stot- of their own
std::string CroatianNumbers::slovenian_ordinal_genitive(std::string_view number) {
    number = remove_leading_zeros(number);
    if (number.empty() || number.size() > 2) return "";
    if (number.size() == 1) {
        switch (number[0]) {
            case '1': return "prvega";
            case '2': return "drugega";
            case '3': return "tretjega";
            case '4': return u8"četrtega";
            case '7': return "sedmega";
            case '8': return "osmega";
            default: break;
        }
        return std::string(digit_to_word(number[0])) + "ega";
    }
    if (number[1] == '0') return tens_to_words(number[0]) + "ega";
    if (number[0] == '1') return teens_to_words(number[1]) + "ega";
    return std::string(slovenian_compound_unit(number[1])) + "in" + tens_to_words(number[0]) + "ega";
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
        return slovenian_ordinal_genitive(groups[0]) + ' ' + slovenian_ordinal_genitive(groups[1]) +
               ' ' + number_to_words(groups[2]);
    }
    return number_to_words(groups[0]) + ' ' + number_to_words(groups[1]) + ' ' +
           number_to_words(groups[2]);
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

// A Slovenian 1, 2, 3 or 4 at the end of the number in [start, end) of the
// text agrees with what it counts (dvá dvé dvé, dvéh, dvéma; tríje trí, tréh,
// trém, trémi; štírje štíri, štírih, štírim, štírimi; èn êna êno: SSKJ²,
// Slovenski pravopis 2001). Asked for by a
// Slovenian listener (2 koša, T2, en koš, trije koši, z dvema žličkama). The
// ending of the next word and the preposition before the number decide; there
// is no parser here, so a word that could be something else leaves the plain
// form (ena, dva, tri, štiri):
//   - A number that names something rather than counts it: glued to letters
//     or after a hyphen (T2, A-1 liga), after a capitalized word inside a
//     sentence (Pentium 2, Voyager 1), after a word that takes a number as a
//     label (stran 2, slika 1, leta 3), before a capital, a sign, a function
//     word or a common verb.
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
    if (digit == 0) return;
    auto say = [&](std::string_view form) { words.replace(token, std::string::npos, form); };

    if (start > 0 && (is_letter_byte(text[start - 1]) || text[start - 1] == '-')) return;

    // The word before the number, with only spaces between, in small letters
    size_t before = start;
    while (before > 0 && (text[before - 1] == ' ' || text[before - 1] == '\t')) --before;
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
        "skupina", "skupini", "skupino", "poglavje", "poglavju", u8"člen", u8"členu", u8"točka", u8"točki",
        u8"točko", "odstavek", "odstavku", u8"različica", u8"različici", u8"različico", "verzija", "verziji",
        "kanal", "kanalu", u8"številka", u8"številki", u8"št", "opomba", "opombi", "priloga", "prilogi",
        "faza", "fazi", "korak", "koraku", "element", u8"vrstica", u8"vrstici", "stolpec", "stolpcu",
        "zavihek", "zavihku", "okno", "oknu", "gumb", "gumbu", "datoteka", "datoteki", "mapa", "mapi",
        "stopnja", "stopnji", "nivo", "del", "delu", "tier", "ulica", "ulici", "cesta", "cesti",
    };
    if (labels.count(prev)) return;

    // The word after the number, in small letters
    size_t word = end;
    while (word < text.size() && (text[word] == ' ' || text[word] == '\t')) ++word;
    if (word == end || word >= text.size()) return;
    const unsigned char first = static_cast<unsigned char>(text[word]);
    const unsigned char second = word + 1 < text.size() ? static_cast<unsigned char>(text[word + 1]) : 0;
    const bool small_letter = (first >= 'a' && first <= 'z') ||
                              (first == 0xC4 && (second == 0x8D || second == 0x87 || second == 0x91)) ||   // č ć đ
                              (first == 0xC5 && (second == 0xA1 || second == 0xBE));                       // š ž
    if (!small_letter) return;
    size_t stop = word;
    while (stop < text.size() && is_word_byte(text[stop])) {
        if (is_digit(text[stop])) return;
        ++stop;
    }
    const std::string_view next(text.data() + word, stop - word);
    auto ends = [&](std::string_view tail) {
        return next.size() >= tail.size() && next.compare(next.size() - tail.size(), tail.size(), tail) == 0;
    };

    // Units are not expanded: en kg, dve min
    static const std::unordered_set<std::string_view> masculine_units = {
        "m", "cm", "mm", "dm", "km", "kg", "g", "dag", "mg", "l", "dl", "cl", "ml", "hl", "kb", "mb", "gb", "tb",
    };
    static const std::unordered_set<std::string_view> feminine_units = {"h", "min", "t"};
    if (digit == 1 && masculine_units.count(next)) { say("en"); return; }
    if (digit == 2 && feminine_units.count(next)) { say("dve"); return; }
    if (next.size() < 3 && next != "ur") return;    // je, se, ni, ki, mi: no noun; ur: od dveh ur
    if (ends("uje") || ends("jte")) return;     // a verb's -uje, the imperative's -jte: 2 deluje, 2 zamešajte

    static const std::unordered_set<std::string_view> not_counted = {
        // Function words and adverbs
        "tudi", "pri", "ali", "vse", "zaradi", "proti", "glede", "niti", "vsi", "okoli", "lani",
        u8"šele", "nikoli", "hkrati", "skozi", "zlasti", u8"drugače", "bolje", "pozneje", "kasneje",
        "vendarle", "kajti", "sredi", "kje", "radi", "nekateri", "kateri", "katere", "kakor", "meni",
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
    if (not_counted.count(next)) return;

    static const std::unordered_set<std::string_view> with_genitive = {
        "od", "do", "iz", "brez", "zaradi", "okoli", "okrog", "blizu", "izmed", "izpod", "izza", "razen",
        "sredi", "glede", "mimo", "poleg", "znotraj", "zunaj",
    };
    static const std::unordered_set<std::string_view> with_locative = {"v", "na", "o", "ob", "pri", "po"};
    static const std::unordered_set<std::string_view> with_instrumental = {"z", "s", "pred", "med", "nad", "pod", "za"};
    static const std::unordered_set<std::string_view> with_dative = {"k", "h", "proti", "kljub"};
    const bool genitive = with_genitive.count(prev) != 0;
    const bool locative = with_locative.count(prev) != 0;
    const bool instrumental = with_instrumental.count(prev) != 0;
    const bool dative = with_dative.count(prev) != 0;
    const char tail = next.back();
    const bool no_ending = tail != 'a' && tail != 'e' && tail != 'i' && tail != 'o' && tail != 'u';

    // The feminine i-stems, the commonest of Sloleks (without -ost, -tev),
    // singular and plural: their consonant and their -i look masculine
    static const std::unordered_set<std::string_view> feminine = {
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
    const bool feminine_i = feminine.count(next) != 0 || (ends("ost") && next.size() > 5) ||
                            (ends("osti") && next.size() > 6) || ends("tev");

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
        // "7. oktobra" is the date's genitive ordinal, sedmega oktobra.
        if (size_t end = silent_dot_end(text, num_start, i)) {
            std::string_view number(text.data() + num_start, i - num_start);
            std::string ordinal;
            if (slovenian() && number.size() <= 2) {
                static const char* const MONTHS[] = {
                    "januarja", "februarja", "marca", "aprila", "maja", "junija", "julija",
                    "avgusta", "septembra", "oktobra", "novembra", "decembra",
                };
                size_t word = end;
                while (word < length && (text[word] == ' ' || text[word] == '\t')) ++word;
                for (const char* month : MONTHS) {
                    size_t n = std::strlen(month);
                    if (text.compare(word, n, month) == 0 && (word + n >= length || !is_word_byte(text[word + n]))) {
                        ordinal = slovenian_ordinal_genitive(number);
                        break;
                    }
                }
            }
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
                    result += number_to_words(rest);
                } else {
                    for (char d : rest) {
                        if (!first) result += ' ';
                        result += digit_to_word(d);
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
    return std::string(digit_to_word(digit));
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
            result += digit_to_word(text[i]);
            first_digit = false;
            i++;
        }
    }

    return result;
}

} // namespace laprdus
