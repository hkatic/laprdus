// -*- coding: utf-8 -*-
// formant_slovenian.hpp - Slovenian text front end: letter-to-sound rules,
// stress, the built-in pronunciation lexicon, letter and symbol names
//
// Slovenian differs from Croatian, Serbian and Bosnian in its sounds (eight
// vowels: the open and close e and o and the schwa; l and v spoken as [w]
// before a consonant and at the end of a word; final devoicing), in its
// stress (dynamic, long or short, no tones, on any syllable) and in its
// function words, so it has rules and a lexicon of its own here. The
// tokenizer, the clitic grouping, the syllable count and the flattening into
// one phone string are shared with the other languages (formant_frontend.cpp).

#ifndef LAPRDUS_FORMANT_SLOVENIAN_HPP
#define LAPRDUS_FORMANT_SLOVENIAN_HPP

#include "formant_phonemes.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace laprdus {
namespace formant {

// Accent marks found in the text (and in lexicon entries), one byte per
// letter. Shared by every language's letter-to-sound code.
constexpr uint8_t MARK_STRESS = 1;  // the letter carries the stress
constexpr uint8_t MARK_LONG = 2;    // the vowel is long
constexpr uint8_t MARK_FALL = 4;    // falling accent (Slovenian: an open long e/o, ê ô)
constexpr uint8_t MARK_RISE = 8;    // rising accent (Slovenian: acute = close long e/o, grave = short)

namespace slovenian {

/**
 * The name of a letter in Slovenian ("b" -> "be", "š" -> "eš", "w" ->
 * "dvojni ve"); empty for anything else. The letter is as
 * spelling_letter() returns it.
 */
const char32_t* letter_name(const std::u32string& letter);

/** The name of a symbol read wherever it stands inside a clause (".", ":", "%"); nullptr otherwise. */
const char32_t* symbol_name(char32_t c);

/**
 * The name of a digit, a punctuation mark or a symbol when spelled
 * ("1" -> "ena", "." -> "pika", "@" -> "afna"); nullptr for a character
 * without one. UTF-8 in and out.
 */
const char* spelled_name(const std::string& character);

// ---- Word classes ----
bool is_proclitic(const std::u32string& w);
bool is_enclitic(const std::u32string& w);
bool is_function_word(const std::u32string& w);
bool is_question_word(const std::u32string& w);
/** The words that open a wh-question (kdo, kaj, kje, kako, zakaj, kateri, ...). */
const std::unordered_set<std::u32string>& question_word_set();

/**
 * The pronunciation lexicon. An entry is the word spelled as it is
 * pronounced, in the notation of Slovenian dictionaries (SSKJ, Slovenski
 * pravopis):
 *
 *   á í ú ŕ   long stressed vowel (ŕ: the syllabic r)
 *   é ó       long stressed close e, o            (mésto, góra)
 *   ê ô       long stressed open e, o             (člôvek, ôkno)
 *   à è ì ò ù short stressed vowel (e, o open)    (bràt, pès, otròk)
 *   ə         the schwa written e                 (vétər, pəs)
 *   ə̀         stressed schwa (ə + U+0300)         (pə̀s)
 *   w         an l pronounced [w]                 (bíw, délaw)
 *   l         an l read by the rules (before a vowel [l]; at the end and
 *             before a consonant [w] in the participles, after a schwa and
 *             in the words the Pravopis lists, [l] otherwise)
 *   v         v: [v] before a vowel, [w] elsewhere, by the rule
 *   '  :      ASCII alternatives: ' before the stressed vowel, : after a
 *             long one ("m'é:sto" = mésto, "p'əs" = pəs, "'r:" = ŕ)
 *   *         at the end: a stem; forms with up to five more letters are
 *             the stem plus the ending read by the rules of unstressed
 *             endings (d'é:la* covers délal, délala, délajo)
 *   |         prefix|ending|ending: one exact form per ending, marks in
 *             either part (zač|éti|éla|nèm; an empty ending is the bare prefix)
 *   =         spelling=pronunciation, when the pronunciation cannot be
 *             turned back into the spelling (vse=wsè is not needed: v is by
 *             rule; hotel=hotél is)
 *
 * The spelling the entry is for is the entry without its marks (ə -> e,
 * w -> l). Keys are lowercase; the lexicon does not know capitals.
 */
class Lexicon {
public:
    /** Parse one entry; false, with a reason, for a malformed one. */
    static bool parse(const std::u32string& marked, std::u32string& key, std::u32string& pron,
                      bool& stem, std::string* error);

    /** Add one entry (a word, a stem or a form list); false if malformed. */
    bool add(const std::u32string& marked);
    void clear();
    bool empty() const { return m_exact.empty() && m_stems.empty(); }

    /** The pronunciation of an exact form; false when there is none. */
    bool find_exact(const std::u32string& word, std::u32string& pron) const;
    /** The longest stem the word starts with (ending of up to five letters); false when there is none. */
    bool find_stem(const std::u32string& word, size_t& stem_length, std::u32string& pron) const;

    static constexpr size_t MAX_ENDING = 5;

    /**
     * Generated tables searched in place (formant_slovenian.inc): entries
     * in the notation, sorted by the spelling they are for. The hand-added
     * entries win over them.
     */
    void set_tables(const char* const* forms, size_t form_count,
                    const char* const* stems, size_t stem_count);

private:
    std::unordered_map<std::u32string, std::u32string> m_exact;
    std::unordered_map<std::u32string, std::u32string> m_stems;
    const char* const* m_forms = nullptr;
    size_t m_form_count = 0;
    const char* const* m_stem_table = nullptr;
    size_t m_stem_count = 0;
};

/** The built-in lexicon (formant_slovenian.inc plus the hand-written entries). */
const Lexicon& builtin_lexicon();

/**
 * One word turned into phones. `phones` carry ph, nucleus, is_long and
 * nucleus_tail; `stress` is the index into `nuclei` of the stressed
 * syllable (-1 only for a word without a vowel).
 */
struct WordPhones {
    std::vector<Phone> phones;
    std::vector<int> letter;        // letter index of each phone
    std::vector<int> nuclei;        // phone index of each syllable nucleus
    int stress = -1;
    bool from_lexicon = false;      // the lexicon (or the text's marks) gave the stress
};

/**
 * Letters to phones for one word: the marks written in the text, then the
 * user's lexicon, then the built-in one, then the rules. `user` may be
 * nullptr. `letter_name` marks the name of a letter ("be", "eš"), which
 * the rules read with a long close vowel.
 */
void phonemize(const std::u32string& word, const std::vector<uint8_t>& marks,
               const Lexicon* user, bool letter_name, WordPhones& out);

/**
 * Sound changes across a clause: regressive voicing assimilation, final
 * devoicing, n before k/g, the merging of identical consonants over a
 * clitic boundary. `bound(i)` says whether phones i and i + 1 are spoken
 * without a word break; `clitic` whether the word is a clitic.
 */
void assimilate(std::vector<Phone>& phones, const std::vector<bool>& word_is_clitic);

/**
 * The accent lexicon the user keeps in accents.json: entries with
 * "language": "sl" (and those without a language that parse in this
 * notation) are collected here by UserLexicon::parse.
 */
} // namespace slovenian
} // namespace formant
} // namespace laprdus

#endif // LAPRDUS_FORMANT_SLOVENIAN_HPP
