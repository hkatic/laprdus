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
        char32_t ch = utf32[i];
        Punctuation punct = PhonemeMapper::detect_punctuation(ch);

        if (punct != Punctuation::NONE) {
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

            // Start new segment
            current = TextSegment{};
            segment_start = i + 1;
        }
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
