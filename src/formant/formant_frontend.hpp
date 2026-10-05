// -*- coding: utf-8 -*-
// formant_frontend.hpp - Text to phone conversion for the formant voices
// Letter-to-sound rules, stress placement and clitic handling for
// Croatian, Serbian and Bosnian

#ifndef LAPRDUS_FORMANT_FRONTEND_HPP
#define LAPRDUS_FORMANT_FRONTEND_HPP

#include "laprdus/types.hpp"
#include "formant_phonemes.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace laprdus {
namespace formant {

enum class ClauseKind : uint8_t {
    Statement,          // falling
    Continuation,       // comma, semicolon, colon: suspended rise
    YesNoQuestion,      // rise on the focused word
    WhQuestion,         // high start on the question word, then falling
    Exclamation         // statement with a wider pitch range
};

struct Utterance {
    std::vector<Phone> phones;
    ClauseKind kind = ClauseKind::Statement;
    int syllable_count = 0;
    int focus_word = -1;        // word carrying the question peak (-1: last accented)
};

/**
 * Frontend - converts one clause of text into phones.
 *
 * - Serbian Cyrillic is transliterated, digraphs (lj, nj, dž) and the
 *   ijekavian "ije" diphthong are resolved, syllabic r is detected
 * - stress comes from explicit accent marks in the text, a built-in lexicon,
 *   suffix rules, or the Neo-Štokavian default (first syllable)
 * - proclitics and enclitics lose their stress and join their host word
 * - voicing and place assimilation apply inside words and clitic groups
 */
class Frontend {
public:
    explicit Frontend(VoiceLanguage language);

    Utterance process(const std::u32string& text, Punctuation punct) const;

    struct LexEntry {
        int8_t stress_letter = -1;  // letter index the stress mark precedes
        Accent accent = Accent::None;
        uint32_t long_letters = 0;  // bit per letter index carrying length
    };

private:
    void add_entries(const char* const* entries, size_t count);

    VoiceLanguage m_language;
    std::unordered_map<std::u32string, LexEntry> m_exact;
    std::unordered_map<std::u32string, LexEntry> m_stems;
};

// Built-in accent lexicon (formant_lexicon.cpp). Entries are UTF-8 words with
// marks: ' before the stressed vowel (^ = falling, / = rising), : after a long
// vowel, * at the end for a stem that also matches inflected forms.
const char* const* lexicon_common(size_t& count);
const char* const* lexicon_croatian(size_t& count);
const char* const* lexicon_serbian(size_t& count);
const char* const* lexicon_bosnian(size_t& count);

} // namespace formant
} // namespace laprdus

#endif // LAPRDUS_FORMANT_FRONTEND_HPP
