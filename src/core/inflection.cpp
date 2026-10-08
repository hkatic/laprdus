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
           c == U'\v' || c == 0x85 || c == 0xA0 || (c >= 0x2000 && c <= 0x200B) ||
           c == 0x2028 || c == 0x2029 || c == 0x3000;
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

// A line break, in any of its encodings.
bool is_line_break(char32_t c) {
    return c == U'\n' || c == U'\r' || c == U'\v' || c == U'\f' || c == 0x85 ||
           c == 0x2028 || c == 0x2029;
}

bool is_alnum(char32_t c) {
    return (c >= U'0' && c <= U'9') || (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z') ||
           c >= 0x80;
}

bool is_upper(char32_t c) {
    return (c >= U'A' && c <= U'Z') || c == 0x10C || c == 0x106 || c == 0x110 || c == 0x160 ||
           c == 0x17D || (c >= 0x400 && c <= 0x42F);
}

// A single period glued to the next character stays in the clause, to be
// read by name, only after a single letter (the dot of an abbreviation,
// "s.a.r.s.", "U.S.A.", which the front end silences and spells) and
// before a file extension or domain label: one to five letters or digits
// in one case, with a letter among them, and nothing of a word after them
// ("datoteka.txt", "pjesma.mp3", "www.index.hr", "README.TXT"). Any other
// glued period ends the clause like one before a space; a screen reader
// glues the items of a label that way ("Preslušano.Nestajuća poruka",
// "1.Prvi"). A decimal point never gets here: the number converter writes
// it as another character.
bool period_stays_in_word(const std::u32string& text, size_t i) {
    if (i + 1 >= text.size()) return false;
    if (i > 0 && is_alnum(text[i - 1]) && (i == 1 || !is_alnum(text[i - 2]))) return true;
    size_t end = i + 1;
    bool letter = false, lower = false, upper = false;
    while (end < text.size() && is_alnum(text[end])) {
        char32_t c = text[end];
        if (c < U'0' || c > U'9') {
            letter = true;
            if (is_upper(c)) upper = true; else lower = true;
        }
        ++end;
    }
    size_t length = end - i - 1;
    return letter && length >= 1 && length <= 5 && !(lower && upper);
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
        // A line break ends the clause, with the newline pause: the lines of
        // a post, a list or a label are phrases of their own ("Javno" /
        // "S ponosom predstavljam"), and the next line starts a sentence of
        // its own. A run of breaks and spaces (a blank line, "\r\n") is one
        // boundary; after a mark that ended the clause it adds nothing.
        if (is_line_break(utf32[i])) {
            size_t run_end = i + 1;
            while (run_end < utf32.size() && is_space(utf32[run_end])) ++run_end;
            bool blank = true;
            for (size_t k = segment_start; k < i; ++k) {
                if (!is_space(utf32[k])) blank = false;
            }
            if (!blank) {
                current.text = utf32.substr(segment_start, i - segment_start);
                current.trailing_punct = Punctuation::NEWLINE;
                current.inflection = punct_to_inflection(Punctuation::NEWLINE);
                current.is_end_of_sentence = false;
                segments.push_back(std::move(current));
                current = TextSegment{};
            }
            segment_start = run_end;
            i = run_end - 1;
            continue;
        }

        Punctuation punct = PhonemeMapper::detect_punctuation(utf32[i]);
        if (punct == Punctuation::NONE) {
            continue;
        }

        // A run of marks ("...", "?!", ".)") is one boundary. Like eSpeak,
        // a mark ends the clause only when whitespace, a bracket or quote,
        // or the end of the text follows it. A mark glued to the next
        // character is part of a word ("12:30", "Hej!ti") and stays in the
        // clause text, where the front end reads it by name; a glued period
        // does so only in a file name or after an abbreviated letter
        // (period_stays_in_word). An ellipsis character always ends the
        // clause.
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
        const bool lone_period = run_end == i + 1 && utf32[i] == U'.';
        const bool ends_clause = ellipsis || run_end >= utf32.size() ||
                                 is_space(utf32[run_end]) ||
                                 is_bracket_or_quote(utf32[run_end]) ||
                                 (lone_period && !period_stays_in_word(utf32, i));
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
