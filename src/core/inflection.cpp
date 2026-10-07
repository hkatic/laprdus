// -*- coding: utf-8 -*-
// inflection.cpp - Clause segmentation and pause settings

#include "inflection.hpp"
#include "phoneme_mapper.hpp"
#include <cmath>
#include <algorithm>

namespace laprdus {

// =============================================================================
// Constructor
// =============================================================================

InflectionProcessor::InflectionProcessor() = default;

// =============================================================================
// Pause Settings
// =============================================================================

void InflectionProcessor::set_pause_settings(const PauseSettings& settings) {
    m_pause_settings = settings;
    m_pause_settings.clamp();
}

PauseSettings InflectionProcessor::pause_settings() const {
    return m_pause_settings;
}

// =============================================================================
// Analyze Text for Inflection Points
// =============================================================================

namespace {

// Whitespace in the sense of "what follows a clause-ending mark".
bool is_space(char32_t c) {
    return c == U' ' || c == U'\t' || c == U'\n' || c == U'\r' || c == U'\f' ||
           c == U'\v' || c == 0xA0 || (c >= 0x2000 && c <= 0x200B) || c == 0x3000;
}

// Brackets and quotation marks: a mark followed by one of these still ends
// the clause ("(Dobro.)", "\"Idemo!\"").
bool is_bracket_or_quote(char32_t c) {
    switch (c) {
        case U'(': case U')': case U'[': case U']': case U'{': case U'}':
        case U'"': case U'\'': case U'<': case U'>':
        case 0xAB: case 0xBB:                   // « »
        case 0x2018: case 0x2019: case 0x201A:  // ‘ ’ ‚
        case 0x201C: case 0x201D: case 0x201E:  // “ ” „
        case 0x2039: case 0x203A:               // ‹ ›
            return true;
        default:
            return false;
    }
}

} // namespace

std::vector<TextSegment> InflectionProcessor::analyze_text(const std::string& text) {
    std::vector<TextSegment> segments;

    // Convert to UTF-32 for proper character handling
    std::u32string utf32 = PhonemeMapper::utf8_to_utf32(text);

    if (utf32.empty()) {
        return segments;
    }

    TextSegment current;
    size_t segment_start = 0;

    for (size_t i = 0; i < utf32.size(); ++i) {
        Punctuation punct = PhonemeMapper::detect_punctuation(utf32[i]);
        if (punct == Punctuation::NONE) {
            continue;
        }

        // A run of marks ("...", "?!", ".)") is one boundary. Like eSpeak,
        // a mark ends the clause only when whitespace, a bracket or quote,
        // or the end of the text follows it. A mark glued to the next
        // character is part of a word ("datoteka.txt", "3.14", "12:30",
        // "www.index.hr") and stays in the clause text, where the front end
        // reads it by name. An ellipsis character always ends the clause.
        size_t run_end = i;
        size_t dots = 0;
        bool question = false, exclamation = false, ellipsis = false;
        while (run_end < utf32.size()) {
            Punctuation p = PhonemeMapper::detect_punctuation(utf32[run_end]);
            if (p == Punctuation::NONE) break;
            if (utf32[run_end] == U'.') ++dots;
            if (p == Punctuation::QUESTION) question = true;
            if (p == Punctuation::EXCLAMATION) exclamation = true;
            if (p == Punctuation::ELLIPSIS) ellipsis = true;
            ++run_end;
        }
        const bool ends_clause = ellipsis || run_end >= utf32.size() ||
                                 is_space(utf32[run_end]) ||
                                 is_bracket_or_quote(utf32[run_end]);
        if (!ends_clause) {
            i = run_end - 1;
            continue;
        }

        if (ellipsis || dots >= 3) punct = Punctuation::ELLIPSIS;
        else if (question) punct = Punctuation::QUESTION;
        else if (exclamation) punct = Punctuation::EXCLAMATION;

        // End current segment at this punctuation
        if (i > segment_start) {
            current.text = utf32.substr(segment_start, i - segment_start);
        }

        current.trailing_punct = punct;
        current.inflection = punct_to_inflection(punct);

        // Check if this ends a sentence
        current.is_end_of_sentence = (punct == Punctuation::PERIOD ||
                                       punct == Punctuation::QUESTION ||
                                       punct == Punctuation::EXCLAMATION);

        if (!current.text.empty()) {
            segments.push_back(std::move(current));
        }

        // Start new segment after the whole run
        current = TextSegment{};
        segment_start = run_end;
        i = run_end - 1;
    }

    // Handle remaining text (no trailing punctuation)
    if (segment_start < utf32.size()) {
        current.text = utf32.substr(segment_start);
        current.trailing_punct = Punctuation::NONE;
        current.inflection = InflectionType::NEUTRAL;
        current.is_end_of_sentence = false;

        if (!current.text.empty()) {
            segments.push_back(std::move(current));
        }
    }

    return segments;
}

// =============================================================================
// Get Pause Duration for Punctuation (instance method with custom settings)
// =============================================================================

uint32_t InflectionProcessor::get_pause_duration(Punctuation punct) const {
    switch (punct) {
        case Punctuation::COMMA:
            return m_pause_settings.comma_pause_ms;
        case Punctuation::PERIOD:
        case Punctuation::QUESTION:
        case Punctuation::EXCLAMATION:
            return m_pause_settings.sentence_pause_ms;
        case Punctuation::SEMICOLON:
        case Punctuation::COLON:
            return m_pause_settings.comma_pause_ms;
        case Punctuation::ELLIPSIS:
            return m_pause_settings.sentence_pause_ms;
        case Punctuation::NEWLINE:
            return m_pause_settings.newline_pause_ms;
        default:
            return 0;
    }
}

// =============================================================================
// Get Default Pause Duration for Punctuation (static method)
// =============================================================================

uint32_t InflectionProcessor::get_default_pause_duration(Punctuation punct) {
    switch (punct) {
        case Punctuation::COMMA:
            return 100;  // 100ms default
        case Punctuation::PERIOD:
        case Punctuation::QUESTION:
        case Punctuation::EXCLAMATION:
            return 100;  // 100ms default
        case Punctuation::SEMICOLON:
        case Punctuation::COLON:
            return 100;  // 100ms default
        case Punctuation::ELLIPSIS:
            return 100;  // 100ms default
        case Punctuation::NEWLINE:
            return 100;  // 100ms default
        default:
            return 0;
    }
}

// =============================================================================
// Convert Punctuation to Inflection Type
// =============================================================================

InflectionType InflectionProcessor::punct_to_inflection(Punctuation punct) {
    switch (punct) {
        case Punctuation::COMMA:
            return InflectionType::COMMA_CONTINUATION;
        case Punctuation::PERIOD:
            return InflectionType::PERIOD_FINALITY;
        case Punctuation::QUESTION:
            return InflectionType::QUESTION_RISING;
        case Punctuation::EXCLAMATION:
            return InflectionType::EXCLAMATION_EMPHATIC;
        case Punctuation::SEMICOLON:
            return InflectionType::COMMA_CONTINUATION;  // Similar to comma
        case Punctuation::COLON:
            return InflectionType::NEUTRAL;
        case Punctuation::ELLIPSIS:
            return InflectionType::PERIOD_FINALITY;  // Trailing off
        default:
            return InflectionType::NEUTRAL;
    }
}

} // namespace laprdus
