// -*- coding: utf-8 -*-
// test_features.cpp - Unit tests for Laprdus TTS features
// Tests Cyrillic conversion, emoji replacement, and dictionary lookups

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include "core/phoneme_mapper.hpp"
#include "core/emoji_dict.hpp"
#include "core/pronunciation_dict.hpp"

using namespace laprdus;

// =============================================================================
// Test Utilities
// =============================================================================

#define TEST(name) \
    std::cout << "Testing " << name << "... "; \
    bool test_passed = true;

#define ASSERT_EQ(actual, expected) \
    if ((actual) != (expected)) { \
        std::cerr << "\nFailed: " << #actual << " != " << #expected << std::endl; \
        std::cerr << "  Actual: " << (actual) << std::endl; \
        std::cerr << "  Expected: " << (expected) << std::endl; \
        test_passed = false; \
    }

#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        std::cerr << "\nFailed: " << #condition << " is false" << std::endl; \
        test_passed = false; \
    }

#define END_TEST() \
    if (test_passed) { \
        std::cout << "PASSED" << std::endl; \
    } else { \
        std::cout << "FAILED" << std::endl; \
        all_passed = false; \
    }

// =============================================================================
// Cyrillic Conversion Tests
// =============================================================================

void test_cyrillic_basic() {
    TEST("Cyrillic basic conversion");

    // Test Serbian Cyrillic to Latin conversion
    std::u32string cyrillic = U"Добар дан";  // "Dobar dan" in Cyrillic
    std::u32string latin = cyrillic::to_latin(cyrillic);

    ASSERT_EQ(latin, U"Dobar dan");
    END_TEST();
}

void test_cyrillic_special_chars() {
    TEST("Cyrillic special characters");

    // Test Serbian-specific letters
    // љ -> lj, њ -> nj, ћ -> ć, ђ -> đ, џ -> dž
    std::u32string cyrillic = U"љубав";  // "ljubav" (love)
    std::u32string latin = cyrillic::to_latin(cyrillic);

    // Check that љ was converted to "lj"
    ASSERT_TRUE(latin.find(U'l') != std::u32string::npos);
    ASSERT_TRUE(latin.find(U'j') != std::u32string::npos);

    END_TEST();
}

void test_cyrillic_uppercase() {
    TEST("Cyrillic uppercase conversion");

    std::u32string cyrillic = U"СРБИЈА";  // "SRBIJA" in uppercase Cyrillic
    std::u32string latin = cyrillic::to_latin(cyrillic);

    // First character should be uppercase 'S'
    ASSERT_EQ(latin[0], U'S');

    END_TEST();
}

void test_cyrillic_mixed() {
    TEST("Cyrillic mixed with Latin");

    std::u32string mixed = U"Hello Свет World";  // Mixed Latin and Cyrillic
    std::u32string result = cyrillic::to_latin(mixed);

    // Latin parts should be unchanged
    ASSERT_TRUE(result.find(U"Hello") != std::u32string::npos);
    ASSERT_TRUE(result.find(U"World") != std::u32string::npos);

    // Cyrillic part should be converted
    ASSERT_TRUE(result.find(U"Svet") != std::u32string::npos);

    END_TEST();
}

void test_cyrillic_is_cyrillic() {
    TEST("is_cyrillic function");

    // Cyrillic characters should return true
    ASSERT_TRUE(cyrillic::is_cyrillic(U'\u0410'));  // А (Cyrillic A)
    ASSERT_TRUE(cyrillic::is_cyrillic(U'\u0430'));  // а (Cyrillic a)
    ASSERT_TRUE(cyrillic::is_cyrillic(U'\u0459'));  // љ (Cyrillic Lje)

    // Latin characters should return false
    ASSERT_TRUE(!cyrillic::is_cyrillic(U'A'));
    ASSERT_TRUE(!cyrillic::is_cyrillic(U'a'));
    ASSERT_TRUE(!cyrillic::is_cyrillic(U'1'));

    END_TEST();
}

// =============================================================================
// Emoji Dictionary Tests
// =============================================================================

void test_emoji_basic() {
    TEST("Emoji basic replacement");

    EmojiDictionary dict;
    dict.add_entry("\xF0\x9F\x98\x80", "nasmijano lice");  // 😀
    dict.set_enabled(true);

    std::string result = dict.replace_emojis("Hello \xF0\x9F\x98\x80 World");
    ASSERT_TRUE(result.find("nasmijano lice") != std::string::npos);

    END_TEST();
}

void test_emoji_variation_selector() {
    TEST("Emoji variation selector handling");

    EmojiDictionary dict;
    // Add red heart with variation selector (❤️ = U+2764 U+FE0F)
    dict.add_entry("\xE2\x9D\xA4\xEF\xB8\x8F", "crveno srce");
    dict.set_enabled(true);

    // Test with variation selector
    std::string result1 = dict.replace_emojis("I love you \xE2\x9D\xA4\xEF\xB8\x8F");
    ASSERT_TRUE(result1.find("crveno srce") != std::string::npos);

    // Test without variation selector (❤ = U+2764 only)
    std::string result2 = dict.replace_emojis("I love you \xE2\x9D\xA4");
    ASSERT_TRUE(result2.find("crveno srce") != std::string::npos);

    END_TEST();
}

void test_emoji_disabled() {
    TEST("Emoji disabled state");

    EmojiDictionary dict;
    dict.add_entry("\xF0\x9F\x98\x80", "nasmijano lice");
    dict.set_enabled(false);  // Disabled

    std::string input = "Hello \xF0\x9F\x98\x80 World";
    std::string result = dict.replace_emojis(input);

    // Should return unchanged when disabled
    ASSERT_EQ(result, input);

    END_TEST();
}

// =============================================================================
// Phoneme Mapper Integration Tests
// =============================================================================

void test_phoneme_mapper_cyrillic_integration() {
    TEST("PhonemeMapper Cyrillic integration");

    PhonemeMapper mapper;

    // Map Cyrillic text - should convert to Latin first, then map phonemes
    std::vector<PhonemeToken> tokens = mapper.map_text("Добар дан");

    // Should produce phonemes for "Dobar dan"
    ASSERT_TRUE(tokens.size() > 0);

    // First phoneme should be 'D'
    ASSERT_EQ(tokens[0].phoneme, Phoneme::D);

    END_TEST();
}

void test_phoneme_mapper_serbian_digraphs() {
    TEST("PhonemeMapper Serbian Cyrillic digraphs");

    PhonemeMapper mapper;

    // Test љ (lj) - should produce LJ phoneme after conversion
    std::vector<PhonemeToken> tokens = mapper.map_text("\xD1\x99");  // љ in UTF-8

    ASSERT_TRUE(tokens.size() > 0);
    ASSERT_EQ(tokens[0].phoneme, Phoneme::LJ);

    END_TEST();
}

// =============================================================================
// Main
// =============================================================================

int main() {
    bool all_passed = true;

    std::cout << "=== Laprdus TTS Feature Tests ===" << std::endl;
    std::cout << std::endl;

    std::cout << "--- Cyrillic Conversion Tests ---" << std::endl;
    test_cyrillic_basic();
    test_cyrillic_special_chars();
    test_cyrillic_uppercase();
    test_cyrillic_mixed();
    test_cyrillic_is_cyrillic();

    std::cout << std::endl;
    std::cout << "--- Emoji Dictionary Tests ---" << std::endl;
    test_emoji_basic();
    test_emoji_variation_selector();
    test_emoji_disabled();

    std::cout << std::endl;
    std::cout << "--- PhonemeMapper Integration Tests ---" << std::endl;
    test_phoneme_mapper_cyrillic_integration();
    test_phoneme_mapper_serbian_digraphs();

    std::cout << std::endl;
    std::cout << "==================================" << std::endl;
    if (all_passed) {
        std::cout << "All tests PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << "Some tests FAILED!" << std::endl;
        return 1;
    }
}
