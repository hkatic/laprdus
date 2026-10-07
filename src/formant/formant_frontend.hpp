// -*- coding: utf-8 -*-
// formant_frontend.hpp - Text to phone conversion for the formant voices
// Letter-to-sound rules, stress placement and clitic handling for
// Croatian, Serbian and Bosnian

#ifndef LAPRDUS_FORMANT_FRONTEND_HPP
#define LAPRDUS_FORMANT_FRONTEND_HPP

#include "laprdus/types.hpp"
#include "formant_phonemes.hpp"
#include <memory>
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

/** One lexicon entry: where the stress is and which vowels are long. */
struct LexEntry {
    int8_t stress_letter = -1;  // letter index the stress mark precedes
    Accent accent = Accent::None;
    uint32_t long_letters = 0;  // bit per letter index carrying length
};

struct VerbRoot;    // a verb's root or stem with its conjugation (formant_frontend.cpp)

/**
 * UserLexicon - accent entries supplied by the user.
 *
 * The entries use the notation of formant_lexicon.cpp and come from a JSON
 * file next to the user dictionaries:
 *
 *   { "version": "1.0", "entries": [
 *       { "word": "kontr'o:l*", "comment": "kontróla, kontróle" },
 *       { "word": "sign'a:l|a|u|om|e|i|ima" },
 *       { "verb": "ur'e:d=i<p", "comment": "uréditi; see VERBS" },
 *       { "verb": "dijel:i", "language": "hr" }
 *   ] }
 *
 * "word" is a word, a stem ("*") or a paradigm ("|"); "verb" a whole stem
 * ("st'e:m=classes") or a root with a long ije ("root:classes[:prefixes]");
 * "language" ("hr", "sr", "bs") limits an entry to one voice language.
 * Every entry is checked when the file is parsed, and a malformed one is
 * skipped and counted, so a typo can neither crash the engine nor silently
 * produce a word without stress. The lexicon is parsed once; synthesis
 * only looks entries up in hash maps and scans the user's verbs, so there
 * is no cost per utterance beyond that of the built-in tables.
 */
class UserLexicon {
public:
    struct Report {
        size_t words = 0;           // word forms accepted
        size_t verbs = 0;           // verb entries accepted
        size_t rejected = 0;        // entries skipped
        std::string first_error;    // what was wrong with the first one
    };

    // Limits that keep a huge file from slowing every word down.
    static constexpr size_t MAX_WORDS = 20000;
    static constexpr size_t MAX_VERBS = 2000;

    /** Parse the JSON content. Never fails: bad entries are counted. */
    static std::shared_ptr<const UserLexicon> parse(const std::string& json, Report* report = nullptr);

    ~UserLexicon();

    bool empty() const { return m_words.empty() && m_verbs.empty(); }

private:
    friend class Frontend;
    UserLexicon();

    struct WordEntry {
        std::u32string plain;
        LexEntry entry;
        bool stem = false;
        uint8_t languages = 0;      // bit per VoiceLanguage, 0 = all
    };
    struct VerbEntry;

    std::vector<WordEntry> m_words;
    std::vector<VerbEntry> m_verbs;
};

/**
 * Frontend - converts one clause of text into phones.
 *
 * - Serbian Cyrillic is transliterated, digraphs (lj, nj, dž) and the
 *   ijekavian "ije" diphthong are resolved, syllabic r is detected
 * - stress comes from explicit accent marks in the text, the user's lexicon,
 *   the built-in lexicon, suffix rules, or the Neo-Štokavian default (first
 *   syllable)
 * - proclitics and enclitics lose their stress and join their host word
 * - voicing and place assimilation apply inside words and clitic groups
 */
class Frontend {
public:
    explicit Frontend(VoiceLanguage language);

    Utterance process(const std::u32string& text, Punctuation punct) const;

    /**
     * Use the user's accent entries for this voice's language. The user's
     * exact forms and stems are consulted before the built-in ones, and the
     * user's verbs before the built-in verb tables. Pass nullptr to remove
     * them. Costs one copy of the matching entries, nothing per utterance.
     */
    void set_user_lexicon(const std::shared_ptr<const UserLexicon>& lexicon);

private:
    void add_entries(const char* const* entries, size_t count);
    void add_entry(const std::u32string& marked);

    VoiceLanguage m_language;
    std::unordered_map<std::u32string, LexEntry> m_exact;
    std::unordered_map<std::u32string, LexEntry> m_stems;
    std::unordered_map<std::u32string, LexEntry> m_user_exact;
    std::unordered_map<std::u32string, LexEntry> m_user_stems;
    std::shared_ptr<const std::vector<VerbRoot>> m_user_verbs;
};

// Built-in accent lexicon (formant_lexicon.cpp). Entries are UTF-8 words with
// marks: ' before the stressed vowel (^ = falling, / = rising), : after a long
// vowel, * at the end for a stem that also matches inflected forms,
// stem|ending|ending for a list of exact forms.
const char* const* lexicon_common(size_t& count);
const char* const* lexicon_croatian(size_t& count);
const char* const* lexicon_serbian(size_t& count);
const char* const* lexicon_bosnian(size_t& count);

// Roots of verbs with the long "ije", as "root:classes[:prefixes]" (see the
// table in formant_lexicon.cpp).
const char* const* lexicon_ije_verbs(size_t& count);
// Whole stems of other verbs, as "st'e:m=classes" (same file).
const char* const* lexicon_verbs(size_t& count);

} // namespace formant
} // namespace laprdus

#endif // LAPRDUS_FORMANT_FRONTEND_HPP
