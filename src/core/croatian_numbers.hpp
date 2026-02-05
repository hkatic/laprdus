// -*- coding: utf-8 -*-
// croatian_numbers.hpp - Croatian number to words conversion
// Ported from Python numbers.py

#ifndef LAPRDUS_CROATIAN_NUMBERS_HPP
#define LAPRDUS_CROATIAN_NUMBERS_HPP

#include <string>
#include <string_view>

namespace laprdus {

/**
 * CroatianNumbers - Converts numbers to Croatian words.
 *
 * Features:
 * - Supports numbers from 0 to centillions (10^303)
 * - Proper Croatian grammatical forms for plurals
 * - Handles mixed text with embedded numbers
 * - Preserves non-numeric content
 *
 * Croatian number grammar rules:
 * - 1: singular (jedan, tisuću, milijun)
 * - 2-4: special plural (dva, tisuće, milijuna)
 * - 5+, 0: genitive plural (pet, tisuća, milijuna)
 */
class CroatianNumbers {
public:
    CroatianNumbers() = default;
    ~CroatianNumbers() = default;

    /**
     * Convert all numbers in text to Croatian words (whole number mode).
     * Example: "123" -> "sto dvadeset tri"
     * @param text Input text possibly containing numbers.
     * @return Text with numbers replaced by words.
     */
    std::string convert_numbers_in_text(const std::string& text);

    /**
     * Convert all numbers in text to digit-by-digit Croatian words.
     * Example: "123" -> "jedan dva tri"
     * @param text Input text possibly containing numbers.
     * @return Text with each digit replaced by its word.
     */
    std::string convert_digits_in_text(const std::string& text);

    /**
     * Convert a numeric string to Croatian words.
     * @param number_str String containing only digits.
     * @return Croatian word representation.
     */
    std::string number_to_words(std::string_view number_str);

    /**
     * Convert a single digit to its Croatian word.
     * @param digit The digit character ('0'-'9').
     * @return Croatian word for the digit.
     */
    std::string digit_to_croatian_word(char digit);

private:
    // Basic digit conversions
    std::string_view digit_to_word(char digit);
    std::string tens_to_words(char tens_digit);
    std::string teens_to_words(char ones_digit);
    std::string two_digit_to_words(std::string_view two_digits);
    std::string hundreds_word(char digit);
    std::string three_digit_to_words(std::string_view three_digits);

    // Large number handling
    std::string_view get_thousand_variant(char last_digit);
    std::string get_million_variant(std::string_view prefix, char last_digit);
    std::string get_milliard_variant(std::string_view prefix, char last_digit);
    std::string get_large_number_suffix(int group_index, char last_digit);

    // Group processing
    std::string group_to_words(std::string_view group);
    std::string process_number_groups(std::string_view number);

    // Utility
    std::string_view remove_leading_zeros(std::string_view number);
    bool is_valid_number(std::string_view str);
};

} // namespace laprdus

#endif // LAPRDUS_CROATIAN_NUMBERS_HPP
