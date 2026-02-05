// -*- coding: utf-8 -*-
// croatian_numbers.cpp - Croatian number to words implementation
// Ported from Python numbers.py

#include "croatian_numbers.hpp"
#include <algorithm>
#include <cctype>

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
        case '2': return "dvjesto";
        case '3': return "tristo";
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
    switch (last_digit) {
        case '1':
            result += "lijun";
            break;
        default:
            result += "lijuna";
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

        // Special case: group is exactly "1" for thousands and higher
        // In Croatian, you say "tisuću" not "jedan tisuću" for 1000
        bool is_one = (group_clean == "1");

        // Convert group to words (unless it's "1" for thousands+)
        if (!is_one || groups_from_end == 0) {
            if (!result.empty()) {
                result += " ";
            }
            result += group_to_words(group_clean);
        }

        // Add scale word (thousand, million, etc.)
        if (groups_from_end > 0) {
            // Determine which digit to use for plural form
            // Special case: if group ends in 1 but isn't exactly "1",
            // use '0' to get plural form (e.g., "21 tisuća" not "21 tisuću")
            char plural_digit = last_digit;
            if (!is_one && last_digit == '1') {
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

std::string CroatianNumbers::convert_numbers_in_text(const std::string& text) {
    std::string result;
    result.reserve(text.size() * 2);  // Estimate - numbers expand to words

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

        // Handle leading zeros specially
        // Each leading zero becomes " nula "
        while (i < length && text[i] == '0') {
            // Check if this is a leading zero (more digits follow)
            if (i + 1 < length && text[i + 1] >= '0' && text[i + 1] <= '9') {
                result += " nula ";
                i++;
            } else {
                // This is the last digit (either standalone 0 or end of number)
                break;
            }
        }

        // Find end of number sequence
        size_t num_start = i;
        while (i < length && text[i] >= '0' && text[i] <= '9') {
            i++;
        }

        // Convert number to words
        if (i > num_start) {
            std::string_view num_str(text.data() + num_start, i - num_start);
            std::string words = number_to_words(num_str);
            if (!words.empty()) {
                result += words;
            }
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

        // Convert each digit to its word form
        bool first_digit = true;
        while (i < length && text[i] >= '0' && text[i] <= '9') {
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
