// -*- coding: utf-8 -*-
// croatian_numbers.cpp - Croatian number to words implementation
// Ported from Python numbers.py

#include "croatian_numbers.hpp"
#include <algorithm>
#include <cctype>
#include <string_view>
#include <vector>

namespace laprdus {

// =============================================================================
// Single Digit to Word (0-9)
// =============================================================================

std::string_view CroatianNumbers::digit_to_word(char digit) {
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

    // Handle 21-99 (excluding teens)
    std::string result = tens_to_words(tens);
    if (!result.empty()) {
        result += " ";
        result += digit_to_word(ones);
    }
    return result;
}

// =============================================================================
// Hundreds Word (100, 200, 300, etc.)
// =============================================================================

std::string CroatianNumbers::hundreds_word(char digit) {
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
    // -ilijun/-ilion masculine (see get_large_number_suffix)
    return group_index == 0 ||
           (group_index >= 2 && group_index <= 10 && group_index % 2 == 0);
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
        return "nula";
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

        // Special case: group is exactly "1" for thousands and higher
        // In Croatian, you say "tisuću" not "jedan tisuću" for 1000
        bool is_one = (group_clean == "1");

        // Convert group to words (unless it's "1" for thousands+)
        if (!is_one || groups_from_end == 0) {
            if (!result.empty()) {
                result += " ";
            }
            std::string words = group_to_words(group_clean);
            // The numbers one and two agree with a feminine scale word:
            // dvije tisuće, dve hiljade, dvadeset jedna tisuća, dvije milijarde
            if (groups_from_end > 0 && is_feminine_scale(groups_from_end - 1) && !is_teen &&
                (last_digit == '1' || last_digit == '2')) {
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
// A period after the year is left in the text: it ends the sentence or
// the clause as usual.
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
            if (pos + 1 >= text.size() || text[pos] != '.' || !is_digit(text[pos + 1])) return 0;
            ++pos;
        }
    }
    return pos;
}

} // namespace

// Ordinal of a day or month (1-31), masculine: "7." -> sedmi, "10." -> deseti
std::string CroatianNumbers::ordinal_to_words(int n) {
    static const char* const ONES[] = {
        "", "prvi", "drugi", u8"treći", u8"četvrti", "peti", u8"šesti", "sedmi", "osmi", "deveti",
        "deseti", "jedanaesti", "dvanaesti", "trinaesti", u8"četrnaesti", "petnaesti",
        u8"šesnaesti", "sedamnaesti", "osamnaesti", "devetnaesti", "dvadeseti",
    };
    if (n <= 0) return "";
    if (n <= 20) return ONES[n];
    if (n == 30) return "trideseti";
    std::string words = n < 30 ? "dvadeset " : "trideset ";
    return words + ONES[n % 10];
}

// "12:30" -> dvanaest trideset, "9:05" -> devet nula pet
std::string CroatianNumbers::time_to_words(const std::vector<std::string_view>& groups) {
    std::string words;
    for (size_t k = 0; k < groups.size(); ++k) {
        if (k > 0) words += ' ';
        words += k == 0 ? number_to_words(groups[k]) : digit_group_to_words(groups[k]);
    }
    return words;
}

// "7.10.2026" -> sedmi deseti dvije tisuće dvadeset šest
std::string CroatianNumbers::date_to_words(const std::vector<std::string_view>& groups) {
    return ordinal_to_words(group_value(groups[0])) + ' ' +
           ordinal_to_words(group_value(groups[1])) + ' ' + number_to_words(groups[2]);
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
        words += "nula ";
        group.remove_prefix(1);
    }
    words += number_to_words(group);
    return words;
}

// The separator itself, as the front end will read it. A period stays a
// period glued to the digits on both sides: InflectionProcessor then keeps it
// inside the clause and the front end names it in the voice's language
// (točka / tačka). The decimal comma is "zarez" in every language, so the
// word is written here; a comma in the text would otherwise be a clause
// break.
std::string CroatianNumbers::separator_words(char separator) {
    return separator == '.' ? "." : " zarez ";
}

std::string CroatianNumbers::convert_numbers_in_text(const std::string& text) {
    std::string result;
    result.reserve(text.size() * 2);  // Estimate - numbers expand to words

    size_t i = 0;
    size_t length = text.size();

    while (i < length) {
        // Find start of non-digit text
        size_t text_start = i;
        while (i < length && !is_digit(text[i])) {
            i++;
        }

        // Append non-digit text unchanged
        if (i > text_start) {
            result.append(text, text_start, i - text_start);
        }

        if (i >= length) break;

        // A clock time or a date is read as such, without the separators:
        // the colon of "12:30" is silent and the periods of "7.10.2026."
        // make the day and month ordinals.
        if (size_t end = convert_time_or_date(text, i, result)) {
            i = end;
            continue;
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
                result += "nula ";
                i++;
            } else {
                // This is the last digit (either standalone 0 or end of number)
                break;
            }
        }

        // Find end of number sequence
        size_t num_start = i;
        i = digits_end(text, i);

        // Convert number to words
        if (i > num_start) {
            std::string_view num_str(text.data() + num_start, i - num_start);
            std::string words = number_to_words(num_str);
            if (!words.empty()) {
                result += words;
            }
        }

        // Digits after a period or comma glued to the number. One separator
        // is a decimal mark (eSpeak's rule for Croatian): up to two digits
        // after it are read as one number, more are read one by one, leading
        // zeros each as "nula" ("3.14" -> tri.četrnaest, "3.05" -> tri.nula
        // pet, "3.14159" -> tri.jedan četiri jedan pet devet). Two or more
        // separators make a dotted identifier (version, IP address) and
        // every group is a whole number ("192.168.1.1", "2.0.1").
        if (!is_separator_between_digits(text, i)) continue;

        size_t separators = 0;
        for (size_t k = i; is_separator_between_digits(text, k); k = digits_end(text, k + 1)) {
            ++separators;
        }

        while (is_separator_between_digits(text, i)) {
            result += separator_words(text[i]);
            size_t group_start = i + 1;
            size_t group_end = digits_end(text, group_start);
            std::string_view group(text.data() + group_start, group_end - group_start);
            if (separators >= 2) {
                result += digit_group_to_words(group);
            } else {
                std::string_view rest = group;
                bool first = true;
                while (rest.size() > 1 && rest[0] == '0') {
                    result += first ? "nula" : " nula";
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
        // between digits is read by name ("3.14" -> tri.jedan četiri)
        bool first_digit = true;
        while (i < length && (is_digit(text[i]) || is_separator_between_digits(text, i))) {
            if (!is_digit(text[i])) {
                result += separator_words(text[i]);
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
