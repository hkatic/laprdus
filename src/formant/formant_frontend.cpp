// -*- coding: utf-8 -*-
// formant_frontend.cpp - Text to phone conversion for the formant voices

#include "formant_frontend.hpp"
#include "../core/phoneme_mapper.hpp"
#include <algorithm>
#include <initializer_list>
#include <unordered_set>

namespace laprdus {
namespace formant {

namespace {

// =============================================================================
// Letters
// =============================================================================

constexpr size_t MAX_WORD_LETTERS = 48;

constexpr uint8_t MARK_STRESS = 1;
constexpr uint8_t MARK_LONG = 2;
constexpr uint8_t MARK_FALL = 4;
constexpr uint8_t MARK_RISE = 8;

constexpr char32_t C_CARON = U'č';   // č
constexpr char32_t C_ACUTE = U'ć';   // ć
constexpr char32_t D_STROKE = U'đ';  // đ
constexpr char32_t S_CARON = U'š';   // š
constexpr char32_t Z_CARON = U'ž';   // ž

char32_t to_lower(char32_t c) {
    if (c >= U'A' && c <= U'Z') return c + 32;
    if (c >= 0xC0 && c <= 0xDE && c != 0xD7) return c + 32;
    if (c >= 0x100 && c <= 0x137) return c | 1;
    if (c >= 0x139 && c <= 0x148) return (c & 1) ? c + 1 : c;
    if (c >= 0x14A && c <= 0x177) return c | 1;
    if (c >= 0x179 && c <= 0x17E) return (c & 1) ? c + 1 : c;
    if (c >= 0x200 && c <= 0x217) return c | 1;
    return c;
}

bool is_vowel_letter(char32_t c) {
    return c == U'a' || c == U'e' || c == U'i' || c == U'o' || c == U'u';
}

bool is_base_letter(char32_t c) {
    return (c >= U'a' && c <= U'z') || c == C_CARON || c == C_ACUTE ||
           c == D_STROKE || c == S_CARON || c == Z_CARON;
}

bool is_consonant_letter(char32_t c) {
    return is_base_letter(c) && !is_vowel_letter(c);
}

struct AccentedLetter {
    char32_t ch;
    char32_t base;
    uint8_t mark;
};

// Accent marks as used in dictionaries and grammars: grave = short rising,
// acute = long rising, double grave = short falling, inverted breve or
// circumflex = long falling, macron = unstressed length.
const AccentedLetter ACCENTED[] = {
    {0xE0, U'a', MARK_STRESS | MARK_RISE}, {0xE8, U'e', MARK_STRESS | MARK_RISE},
    {0xEC, U'i', MARK_STRESS | MARK_RISE}, {0xF2, U'o', MARK_STRESS | MARK_RISE},
    {0xF9, U'u', MARK_STRESS | MARK_RISE},
    {0xE1, U'a', MARK_STRESS | MARK_RISE | MARK_LONG},
    {0xE9, U'e', MARK_STRESS | MARK_RISE | MARK_LONG},
    {0xED, U'i', MARK_STRESS | MARK_RISE | MARK_LONG},
    {0xF3, U'o', MARK_STRESS | MARK_RISE | MARK_LONG},
    {0xFA, U'u', MARK_STRESS | MARK_RISE | MARK_LONG},
    {0x155, U'r', MARK_STRESS | MARK_RISE | MARK_LONG},
    {0xE2, U'a', MARK_STRESS | MARK_FALL | MARK_LONG},
    {0xEA, U'e', MARK_STRESS | MARK_FALL | MARK_LONG},
    {0xEE, U'i', MARK_STRESS | MARK_FALL | MARK_LONG},
    {0xF4, U'o', MARK_STRESS | MARK_FALL | MARK_LONG},
    {0xFB, U'u', MARK_STRESS | MARK_FALL | MARK_LONG},
    {0x201, U'a', MARK_STRESS | MARK_FALL}, {0x205, U'e', MARK_STRESS | MARK_FALL},
    {0x209, U'i', MARK_STRESS | MARK_FALL}, {0x20D, U'o', MARK_STRESS | MARK_FALL},
    {0x215, U'u', MARK_STRESS | MARK_FALL}, {0x211, U'r', MARK_STRESS | MARK_FALL},
    {0x203, U'a', MARK_STRESS | MARK_FALL | MARK_LONG},
    {0x207, U'e', MARK_STRESS | MARK_FALL | MARK_LONG},
    {0x20B, U'i', MARK_STRESS | MARK_FALL | MARK_LONG},
    {0x20F, U'o', MARK_STRESS | MARK_FALL | MARK_LONG},
    {0x217, U'u', MARK_STRESS | MARK_FALL | MARK_LONG},
    {0x213, U'r', MARK_STRESS | MARK_FALL | MARK_LONG},
    {0x101, U'a', MARK_LONG}, {0x113, U'e', MARK_LONG}, {0x12B, U'i', MARK_LONG},
    {0x14D, U'o', MARK_LONG}, {0x16B, U'u', MARK_LONG},
};

struct ForeignLetter {
    char32_t ch;
    const char32_t* replacement;
};

const ForeignLetter FOREIGN[] = {
    {0xE4, U"e"}, {0xF6, U"e"}, {0xFC, U"i"}, {0xEB, U"e"}, {0xEF, U"i"},
    {0xF1, U"nj"}, {0xDF, U"s"}, {0xE7, U"s"}, {0x142, U"l"}, {0x144, U"nj"},
    {0x15B, U"s"}, {0x17A, U"z"}, {0x17C, U"ž"}, {0x159, U"rž"},
    {0x11B, U"e"}, {0x16F, U"u"}, {0xFD, U"i"}, {0x148, U"nj"}, {0x165, U"t"},
    {0x10F, U"d"}, {0x13E, U"l"}, {0x151, U"e"}, {0x171, U"i"}, {0xE5, U"o"},
    {0xF8, U"e"}, {0xE6, U"e"}, {0xE3, U"a"}, {0xF5, U"o"}, {0xFF, U"i"},
    {0x1C6, U"dž"}, {0x1C5, U"dž"}, {0x1C4, U"dž"},
    {0x1C9, U"lj"}, {0x1C8, U"lj"}, {0x1C7, U"lj"},
    {0x1CC, U"nj"}, {0x1CB, U"nj"}, {0x1CA, U"nj"},
};

uint8_t combining_mark(char32_t c) {
    switch (c) {
        case 0x300: return MARK_STRESS | MARK_RISE;
        case 0x301: return MARK_STRESS | MARK_RISE | MARK_LONG;
        case 0x302: return MARK_STRESS | MARK_FALL | MARK_LONG;
        case 0x30F: return MARK_STRESS | MARK_FALL;
        case 0x311: return MARK_STRESS | MARK_FALL | MARK_LONG;
        case 0x304: return MARK_LONG;
        default: return 0;
    }
}

// =============================================================================
// Words
// =============================================================================

struct Word {
    std::u32string w;               // plain lowercase letters
    std::vector<uint8_t> marks;     // accent marks per letter
    bool all_caps = false;

    std::vector<Phone> phones;
    std::vector<int> letter;        // letter index of each phone
    std::vector<int> nuclei;        // phone index of each syllable nucleus
    int stress = -1;                // index into nuclei
    Accent accent = Accent::None;
    uint8_t prominence = 2;
    bool clitic = false;
    bool explicit_stress = false;
    bool letter_name = false;       // part of a spelled-out abbreviation
    bool joined = false;            // one word with the previous ("ne znam")

    Word() = default;
    explicit Word(const std::u32string& text) : w(text), marks(text.size(), 0) {}
};

using WordSet = std::unordered_set<std::u32string>;

bool ends_with(const std::u32string& s, const std::u32string& suffix) {
    return s.size() >= suffix.size() &&
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool ends_with_any(const std::u32string& s, std::initializer_list<const char32_t*> suffixes) {
    for (const char32_t* suffix : suffixes) {
        if (ends_with(s, suffix)) return true;
    }
    return false;
}

bool starts_with(const std::u32string& s, const std::u32string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

bool has_vowel(const std::u32string& s) {
    return std::any_of(s.begin(), s.end(), [](char32_t c) {
        return is_vowel_letter(c) || c == U'y';
    });
}

// r carries the syllable when it has no vowel on either side
// (prst, vrt, krv, rt, žanr), and in vowel + rđ (zarđati). Before j it never
// does: rj is the short jat after r (rječnik, rješenje, pogrješka).
bool is_syllabic_r(const std::u32string& w, size_t i) {
    if (w.size() < 2) return false;
    char32_t prev = i > 0 ? w[i - 1] : 0;
    char32_t next = i + 1 < w.size() ? w[i + 1] : 0;
    if (is_vowel_letter(next) || next == U'r' || next == U'j' || prev == U'r') return false;
    if (is_vowel_letter(prev)) return next == D_STROKE;
    return true;
}

bool has_syllabic_r(const std::u32string& w) {
    for (size_t i = 0; i < w.size(); ++i) {
        if (w[i] == U'r' && is_syllabic_r(w, i)) return true;
    }
    return false;
}

// =============================================================================
// Function word classes
// =============================================================================

const WordSet& proclitics() {
    static const WordSet set = {
        U"u", U"na", U"o", U"po", U"za", U"iz", U"od", U"do", U"sa", U"s", U"k",
        U"ka", U"uz", U"niz", U"bez", U"kod", U"pod", U"nad", U"pred", U"pri",
        U"kroz", U"i", U"a", U"ni", U"da", U"pa", U"te", U"no", U"ne", U"al",
    };
    return set;
}

const WordSet& enclitics() {
    static const WordSet set = {
        U"sam", U"si", U"je", U"smo", U"ste", U"su",
        U"ću", U"ćeš", U"će", U"ćemo", U"ćete",
        U"bih", U"bi", U"bismo", U"biste",
        U"me", U"te", U"se", U"ga", U"ju", U"ih", U"mi", U"ti", U"mu", U"joj",
        U"im", U"nam", U"vam", U"li",
    };
    return set;
}

// Stressed, but weaker than content words.
const WordSet& function_words() {
    static const WordSet set = {
        U"ali", U"ili", U"ako", U"kad", U"kada", U"dok", U"jer", U"nego", U"kao",
        U"koji", U"koja", U"koje", U"kojeg", U"kojem", U"kojoj", U"koju",
        U"ovaj", U"ova", U"ovo", U"taj", U"ta", U"to", U"onaj", U"ona", U"ono",
        U"on", U"oni", U"one", U"ja", U"vi", U"moj", U"moja", U"moje", U"tvoj",
        U"tvoja", U"tvoje", U"njegov", U"njen", U"naš", U"vaš",
        U"svoj", U"svoja", U"svoje", U"sve", U"svi", U"još", U"već",
        U"samo", U"bio", U"bila", U"bilo", U"bili", U"biti", U"jesam", U"jesi",
        U"jest", U"jeste", U"jesmo", U"jesu", U"nije", U"nisam", U"nisi",
        U"nismo", U"niste", U"nisu", U"neće", U"neću", U"hoću",
        U"hoće", U"može", U"mogu", U"treba", U"ima", U"nema", U"zato",
        U"tako", U"tu", U"oko", U"preko", U"prema", U"poslije", U"posle",
        U"prije", U"pre", U"nakon", U"između", U"među", U"protiv",
        U"zbog", U"radi", U"osim", U"iznad", U"ispod", U"ispred", U"iza",
        U"pokraj", U"pored", U"blizu", U"mene", U"tebe", U"sebe", U"njega",
        U"nje", U"njih", U"nas", U"vas", U"što", U"šta", U"neki",
        U"neka", U"neko", U"vrlo", U"jako", U"tež",
    };
    return set;
}

const WordSet& question_words() {
    static const WordSet set = {
        U"tko", U"ko", U"što", U"šta", U"gdje", U"gde", U"kada", U"kad",
        U"kako", U"zašto", U"koliko", U"koji", U"koja", U"koje", U"kojeg",
        U"kojem", U"koju", U"čiji", U"čija", U"čije", U"kamo",
        U"kuda", U"odakle", U"otkud", U"čime", U"čega", U"čemu",
        U"kome", U"koga", U"kim", U"kakav", U"kakva", U"kakvo",
    };
    return set;
}

// Verb forms that hand their accent to a preceding "ne" in Serbian and
// Bosnian (nè znām, nè mogu). Croatian does this with monosyllables only.
const WordSet& ne_shift_verbs() {
    static const WordSet set = {
        U"mogu", U"može", U"možeš", U"znamo", U"znate", U"znaju",
        U"želim", U"želi", U"volim", U"voli", U"vidim", U"vidi",
        U"radim", U"radi", U"treba", U"smijem", U"smije", U"smem", U"sme",
        U"dajem", U"daje", U"kažem", U"kaže", U"mislim", U"misli",
        U"idem", U"ide", U"čujem", U"čuje", U"piše", U"čita",
        U"pada", U"boli", U"valja", U"mari", U"brini", U"diraj", U"pitaj",
    };
    return set;
}

// Bosnian keeps the old accent shift onto prepositions most consistently
// (ù grād, nà more, zà mene). Limited to words known to have a falling accent.
const WordSet& proclitic_shift_hosts() {
    static const WordSet set = {
        U"grad", U"kuću", U"vodu", U"more", U"zemlju", U"glavu", U"ruku",
        U"nogu", U"stranu", U"zimu", U"dušu", U"goru", U"polje", U"brdo",
        U"nebo", U"ime", U"sunce", U"srce", U"oko", U"uho", U"dan", U"noć",
        U"put", U"most", U"brod", U"zid", U"rad", U"red", U"led", U"dom", U"rod",
        U"pamet", U"jesen", U"večer", U"kraj", U"vrat", U"zrak", U"zub",
        U"sud", U"sat", U"mene", U"tebe", U"sebe", U"sto", U"stol", U"lice",
    };
    return set;
}

const WordSet& shifting_prepositions() {
    static const WordSet set = {
        U"u", U"na", U"za", U"o", U"po", U"od", U"do", U"iz", U"uz", U"niz",
        U"pod", U"nad", U"pred", U"kroz", U"bez", U"kod", U"pri",
    };
    return set;
}

// Two-letter words written vowel + consonant that are real words, so an
// all-caps "ON" or "IZ" is not spelled out like "IT" or "UK".
const WordSet& short_vc_words() {
    static const WordSet set = {
        U"on", U"od", U"uz", U"iz", U"ih", U"im", U"um", U"ah", U"oh", U"eh",
        U"uh", U"al", U"il", U"ej", U"aj", U"oj", U"uf", U"as", U"ar",
    };
    return set;
}

struct Expansion {
    const char32_t* key;
    const char32_t* croatian;
    const char32_t* eastern;    // Serbian and Bosnian, nullptr = same
};

const Expansion ABBREVIATIONS[] = {
    {U"npr", U"na primjer", U"na primer"},
    {U"tj", U"to jest", nullptr},
    {U"itd", U"i tako dalje", nullptr},
    {U"tzv", U"takozvani", nullptr},
    {U"dr", U"doktor", nullptr},
    {U"mr", U"magistar", nullptr},
    {U"br", U"broj", nullptr},
    {U"str", U"stranica", U"strana"},
    {U"sl", U"slično", nullptr},
    {U"kn", U"kuna", nullptr},
    {U"km", U"kilometara", nullptr},
    {U"cm", U"centimetara", nullptr},
    {U"mm", U"milimetara", nullptr},
    {U"kg", U"kilograma", nullptr},
    {U"ml", U"mililitara", nullptr},
    {U"gđa", U"gospođa", nullptr},
    {U"sv", U"sveti", nullptr},
};

struct SymbolName {
    char32_t symbol;
    const char32_t* croatian;
    const char32_t* eastern;
};

const SymbolName SYMBOLS[] = {
    {U'%', U"posto", nullptr},
    {U'&', U"i", nullptr},
    {U'+', U"plus", nullptr},
    {U'=', U"jednako", nullptr},
    {U'@', U"et", nullptr},
    {0x20AC, U"eura", U"evra"},
    {U'$', U"dolara", nullptr},
    {0xB0, U"stupnjeva", U"stepeni"},
};

const char32_t* letter_name(char32_t c) {
    switch (c) {
        case U'a': return U"a";
        case U'b': return U"be";
        case U'c': return U"ce";
        case C_CARON: return U"če";
        case C_ACUTE: return U"će";
        case U'd': return U"de";
        case D_STROKE: return U"đe";
        case U'e': return U"e";
        case U'f': return U"ef";
        case U'g': return U"ge";
        case U'h': return U"ha";
        case U'i': return U"i";
        case U'j': return U"je";
        case U'k': return U"ka";
        case U'l': return U"el";
        case U'm': return U"em";
        case U'n': return U"en";
        case U'o': return U"o";
        case U'p': return U"pe";
        case U'q': return U"ku";
        case U'r': return U"er";
        case U's': return U"es";
        case S_CARON: return U"eš";
        case U't': return U"te";
        case U'u': return U"u";
        case U'v': return U"ve";
        case U'w': return U"duplo ve";
        case U'x': return U"iks";
        case U'y': return U"ipsilon";
        case U'z': return U"ze";
        case Z_CARON: return U"že";
        default: return U"";
    }
}

// =============================================================================
// Tokenizer
// =============================================================================

void split_words(const std::u32string& text, std::vector<Word>& out) {
    size_t i = 0;
    while (i < text.size()) {
        while (i < text.size() && text[i] == U' ') ++i;
        size_t start = i;
        while (i < text.size() && text[i] != U' ') ++i;
        if (i > start) {
            out.emplace_back(text.substr(start, i - start));
        }
    }
}

std::vector<Word> tokenize(const std::u32string& text, bool eastern) {
    std::vector<Word> words;
    Word current;
    size_t upper_count = 0;

    auto flush = [&]() {
        if (!current.w.empty()) {
            current.all_caps = upper_count == current.w.size();
            words.push_back(std::move(current));
        }
        current = Word();
        upper_count = 0;
    };

    for (char32_t raw : text) {
        char32_t c = to_lower(raw);
        bool upper = c != raw;

        // No real word is this long; splitting keeps one runaway token from
        // becoming a single enormous prosodic word.
        if (current.w.size() >= MAX_WORD_LETTERS) flush();

        if (is_base_letter(c)) {
            current.w.push_back(c);
            current.marks.push_back(0);
            if (upper) ++upper_count;
            continue;
        }

        if (uint8_t mark = combining_mark(c)) {
            if (!current.marks.empty()) current.marks.back() |= mark;
            continue;
        }

        bool handled = false;
        for (const auto& acc : ACCENTED) {
            if (acc.ch == c) {
                current.w.push_back(acc.base);
                current.marks.push_back(acc.mark);
                if (upper) ++upper_count;
                handled = true;
                break;
            }
        }
        if (handled) continue;

        for (const auto& foreign : FOREIGN) {
            if (foreign.ch == c) {
                for (const char32_t* p = foreign.replacement; *p; ++p) {
                    current.w.push_back(*p);
                    current.marks.push_back(0);
                    if (upper) ++upper_count;
                }
                handled = true;
                break;
            }
        }
        if (handled) continue;

        // Apostrophes join (elided vowels: "al'", "k'o"); everything else splits.
        if (c == U'\'' || c == 0x2019) continue;

        flush();
        for (const auto& symbol : SYMBOLS) {
            if (symbol.symbol == c) {
                const char32_t* name = (eastern && symbol.eastern) ? symbol.eastern
                                                                    : symbol.croatian;
                split_words(name, words);
                break;
            }
        }
    }
    flush();
    return words;
}

// Abbreviations without a vowel and short all-caps tokens are read letter by
// letter ("HR" -> "ha er", "USB" -> "u es be").
bool needs_spelling(const Word& word) {
    const std::u32string& w = word.w;
    bool vowel = has_vowel(w);
    if (!vowel && !has_syllabic_r(w)) return true;
    if (!word.all_caps || w.size() > 5) return false;
    if (w.size() == 1) return !is_vowel_letter(w[0]);

    auto v = [&](size_t i) { return is_vowel_letter(w[i]); };
    if (w.size() > 3) {
        // NVDA, HDMI: three consonants that cannot open a word. Native
        // clusters end in a sonorant (STRAH, SKLOP, STVAR) and stay words.
        char32_t third = w[2];
        return !v(0) && !v(1) && !v(2) && third != U'r' && third != U'l' &&
               third != U'v' && third != U'j';
    }
    if (w.size() == 2) {
        if (!v(0) && v(1)) return false;                        // CV
        if (v(0) && !v(1)) return short_vc_words().count(w) == 0;
        return true;                                            // VV, CC
    }
    if (!vowel) return false;                                   // TRG, KRV
    if (!v(0) && v(1)) return false;                            // CVC, CVV
    if (v(0) && !v(1) && v(2)) return false;                    // VCV
    if (!v(0) && !v(1) && v(2)) return false;                   // CCV
    return true;                                                // VCC, VVC
}

std::vector<Word> expand_words(std::vector<Word> tokens, bool eastern) {
    std::vector<Word> words;
    for (size_t t = 0; t < tokens.size(); ++t) {
        Word& token = tokens[t];
        bool marked = std::any_of(token.marks.begin(), token.marks.end(),
                                  [](uint8_t m) { return m != 0; });

        if (!marked) {
            bool expanded = false;
            for (const auto& abbr : ABBREVIATIONS) {
                if (token.w == abbr.key) {
                    split_words((eastern && abbr.eastern) ? abbr.eastern : abbr.croatian,
                                words);
                    expanded = true;
                    break;
                }
            }
            if (expanded) continue;

            // The prepositions "s" and "k" are single consonants that lean on
            // the next word; alone they are just letters.
            bool leaning = (token.w == U"s" || token.w == U"k") &&
                           t + 1 < tokens.size() && !token.all_caps;
            if (!leaning && needs_spelling(token)) {
                for (char32_t c : token.w) {
                    size_t first = words.size();
                    split_words(letter_name(c), words);
                    for (size_t i = first; i < words.size(); ++i) {
                        words[i].letter_name = true;
                    }
                }
                continue;
            }
        }
        words.push_back(std::move(token));
    }
    return words;
}

// =============================================================================
// Letter-to-sound
// =============================================================================

Ph consonant_phone(char32_t c) {
    switch (c) {
        case U'b': return Ph::B;
        case U'c': return Ph::C;
        case C_CARON: return Ph::CH;
        case C_ACUTE: return Ph::TJ;
        case U'd': return Ph::D;
        case D_STROKE: return Ph::DJ;
        case U'f': return Ph::F;
        case U'g': return Ph::G;
        case U'h': return Ph::H;
        case U'j': return Ph::J;
        case U'k': return Ph::K;
        case U'l': return Ph::L;
        case U'm': return Ph::M;
        case U'n': return Ph::N;
        case U'p': return Ph::P;
        case U'r': return Ph::R;
        case U's': return Ph::S;
        case S_CARON: return Ph::SH;
        case U't': return Ph::T;
        case U'v': return Ph::V;
        case U'z': return Ph::Z;
        case Z_CARON: return Ph::ZH;
        default: return Ph::SIL;
    }
}

Ph vowel_phone(char32_t c) {
    switch (c) {
        case U'a': return Ph::A;
        case U'e': return Ph::E;
        case U'i': return Ph::I;
        case U'o': return Ph::O;
        case U'u': return Ph::U;
        default: return Ph::SIL;
    }
}

// Foreign letters: x = ks, q = k (qu = kv), w = v, y = i or j.
void rewrite_foreign(Word& word) {
    std::u32string w;
    std::vector<uint8_t> marks;
    const std::u32string& in = word.w;
    for (size_t i = 0; i < in.size(); ++i) {
        char32_t c = in[i];
        uint8_t m = word.marks[i];
        char32_t prev = i > 0 ? in[i - 1] : 0;
        char32_t next = i + 1 < in.size() ? in[i + 1] : 0;
        if (c == U'x') {
            w += U"ks";
            marks.push_back(0);
            marks.push_back(0);
        } else if (c == U'q') {
            w.push_back(U'k');
            marks.push_back(0);
            if (next == U'u' && i + 2 < in.size() && is_vowel_letter(in[i + 2])) {
                w.push_back(U'v');
                marks.push_back(0);
                ++i;
            }
        } else if (c == U'w') {
            w.push_back(U'v');
            marks.push_back(0);
        } else if (c == U'y') {
            bool glide = is_vowel_letter(prev) || is_vowel_letter(next);
            w.push_back(glide ? U'j' : U'i');
            marks.push_back(glide ? 0 : m);
        } else {
            w.push_back(c);
            marks.push_back(m);
        }
    }
    word.w = std::move(w);
    word.marks = std::move(marks);
}

// d + ž across a prefix boundary are two sounds (nadživjeti, podžupan).
bool splits_dz(const std::u32string& w, size_t i) {
    static const char32_t* const PREFIXES[] = {U"nad", U"pod", U"od", U"pred"};
    static const char32_t* const ROOTS[] = {
        U"živ", U"žnj", U"žup", U"žeć", U"žet",
    };
    for (const char32_t* prefix : PREFIXES) {
        std::u32string p(prefix);
        if (i + 1 != p.size() || !starts_with(w, p)) continue;
        for (const char32_t* root : ROOTS) {
            if (w.compare(i + 1, std::u32string(root).size(), root) == 0) return true;
        }
    }
    return false;
}

// n + j that are not the letter nj (injekcija, konjunktiv).
bool splits_nj(const std::u32string& w, size_t i) {
    struct Pattern { const char32_t* text; size_t n_offset; };
    static const Pattern PATTERNS[] = {
        {U"injekc", 1}, {U"injic", 1}, {U"konjug", 2}, {U"konjunk", 2},
        {U"vanjezi", 2}, {U"tanjug", 2},
    };
    for (const auto& pattern : PATTERNS) {
        std::u32string p(pattern.text);
        if (i >= pattern.n_offset &&
            w.compare(i - pattern.n_offset, p.size(), p) == 0) {
            return true;
        }
    }
    return false;
}

// "ije" as the long reflex of jat is one syllable, [i̯eː] (lijep, mlijeko,
// vrijeme). It stays two syllables at the end of a word (nije, prije), in
// verb and comparative endings (pijem, starijeg) and in loans (klijent).
bool is_ije_diphthong(const std::u32string& w, size_t i) {
    if (i == 0 || !is_consonant_letter(w[i - 1])) return false;
    std::u32string rest = w.substr(i + 3);
    if (rest.empty() || is_vowel_letter(rest[0])) return false;
    if (rest == U"m" || rest == U"š" || rest == U"mo") return false;
    if (rest == U"te") {
        return w == U"dijete" || w == U"svijete" || w == U"cvijete";
    }
    if (rest == U"g" || rest == U"ga" || rest == U"mu" || rest == U"h") {
        if (has_vowel(w.substr(0, i))) return false;
    }
    if (starts_with(rest, U"nt")) return false;
    if (starts_with(w, U"higijen") || starts_with(w, U"hijen")) return false;
    if (starts_with(w, U"dijet") && w != U"dijete" && w.size() <= 8 &&
        (rest == U"ta" || rest == U"ti" || rest == U"tu" || rest == U"tom" ||
         rest == U"tama")) {
        return false;
    }
    return true;
}

void add_phone(Word& word, Ph ph, int letter, bool nucleus) {
    Phone phone;
    phone.ph = ph;
    phone.nucleus = nucleus;
    if (nucleus) {
        word.nuclei.push_back(static_cast<int>(word.phones.size()));
    }
    word.phones.push_back(phone);
    word.letter.push_back(letter);
}

void phonemize(Word& word) {
    rewrite_foreign(word);
    const std::u32string& w = word.w;

    for (size_t i = 0; i < w.size(); ++i) {
        char32_t c = w[i];
        char32_t next = i + 1 < w.size() ? w[i + 1] : 0;
        int li = static_cast<int>(i);

        if (c == U'd' && next == Z_CARON && !splits_dz(w, i)) {
            add_phone(word, Ph::DZH, li, false);
            ++i;
        } else if (c == U'l' && next == U'j') {
            add_phone(word, Ph::LJ, li, false);
            ++i;
        } else if (c == U'n' && next == U'j' && !splits_nj(w, i)) {
            add_phone(word, Ph::NJ, li, false);
            ++i;
        } else if (c == U'i' && next == U'j' && i + 2 < w.size() && w[i + 2] == U'e' &&
                   is_ije_diphthong(w, i)) {
            add_phone(word, Ph::J, li, false);
            word.phones.back().short_glide = true;
            add_phone(word, Ph::E, li + 2, true);
            word.phones.back().is_long = true;
            // Accent marks may sit on either vowel letter of the digraph.
            word.marks[i + 2] |= word.marks[i];
            word.marks[i] = 0;
            i += 2;
        } else if (is_vowel_letter(c)) {
            add_phone(word, vowel_phone(c), li, true);
        } else if (c == U'r' && is_syllabic_r(w, i)) {
            add_phone(word, Ph::SCHWA, li, true);
            add_phone(word, Ph::R, li, false);
            word.phones.back().nucleus_tail = true;
            add_phone(word, Ph::SCHWA, li, false);
            word.phones.back().nucleus_tail = true;
        } else if ((c == U'l' || c == U'n') && i + 1 == w.size() && i > 0 &&
                   is_consonant_letter(w[i - 1]) && w[i - 1] != U'r' &&
                   w[i - 1] != U'l' && w[i - 1] != U'j') {
            // Syllabic l/n at the end of loans: bicikl, ansambl, njutn.
            add_phone(word, Ph::SCHWA, li, true);
            add_phone(word, consonant_phone(c), li, false);
        } else {
            Ph ph = consonant_phone(c);
            if (ph != Ph::SIL) {
                add_phone(word, ph, li, false);
            }
        }
    }
}

// =============================================================================
// Stress rules
// =============================================================================

struct StressResult {
    int nucleus = -1;
    bool is_long = false;
    int long_after = -1;    // an unstressed long syllable (kapacìtēt)
    Accent accent = Accent::None;
};

// =============================================================================
// Verbs with a long root vowel
// =============================================================================

// A verb root from lexicon_ije_verbs() or a whole stem from lexicon_verbs().
struct VerbRoot {
    std::u32string root;
    std::vector<std::u32string> soft;   // before the participle's -en: dijel -> dijelj
    size_t vowel = 0;               // letter of the accented vowel within the root
    bool is_long = false;
    bool whole_stem = false;        // starts the word, takes no further prefix
    bool iti = false;               // urediti, uredim, uredi, uredio, uređen
    bool ati = false;               // pročitati, pročitao, pročitan
    bool ati_present = false;       // ... and pročitam, pročitaj
    bool nuti = false;              // pokrenuti, pokrenem, pokreni, pokrenuo
    bool e_present = false;         // pokažem, pokaži
    bool noun_twin = false;         // "potvrdi", "uredi" are also forms of a noun
    bool present_shifts = false;    // dictionaries: ùrēdīm for uréditi
    uint8_t passive = 0;            // dictionaries: 1 = one syllable back, 2 = first
    std::vector<std::u32string> prefixes;   // empty: any verbal prefixes
};

// The consonant changes before -en: podijeljen, zamijenjen, zalijepljen,
// proslijeđen, primijećen, obaviješten, očišćen, zamišljen, odbačen.
std::vector<std::u32string> soft_roots(const std::u32string& root) {
    struct Change { const char32_t* from; const char32_t* to; const char32_t* also; };
    static const Change CHANGES[] = {
        {U"st", U"št", U"šć"}, {U"sn", U"šnj", nullptr}, {U"zn", U"žnj", nullptr},
        {U"sl", U"šlj", nullptr}, {U"tl", U"tlj", nullptr}, {U"d", U"đ", nullptr},
        {U"t", U"ć", nullptr}, {U"n", U"nj", nullptr}, {U"l", U"lj", nullptr},
        {U"p", U"plj", nullptr}, {U"b", U"blj", nullptr}, {U"v", U"vlj", nullptr},
        {U"m", U"mlj", nullptr}, {U"s", U"š", nullptr}, {U"z", U"ž", nullptr},
        {U"c", U"č", nullptr},
    };
    for (const Change& change : CHANGES) {
        std::u32string from(change.from);
        if (!ends_with(root, from)) continue;
        std::u32string base = root.substr(0, root.size() - from.size());
        std::vector<std::u32string> result = {base + change.to};
        if (change.also) result.push_back(base + change.also);
        return result;
    }
    return {root};
}

// "root:classes[:prefixes]" (roots with a long ije) or "st'e:m=classes"
// (whole stems, accent marked as in the lexicon); see formant_lexicon.cpp.
std::vector<VerbRoot> load_verb_roots() {
    std::vector<VerbRoot> roots;
    for (bool whole : {false, true}) {
        size_t count = 0;
        const char* const* entries = whole ? lexicon_verbs(count) : lexicon_ije_verbs(count);
        for (size_t e = 0; e < count; ++e) {
            std::u32string text = PhonemeMapper::utf8_to_utf32(entries[e]);
            size_t split = text.find(whole ? U'=' : U':');
            if (split == std::u32string::npos) continue;
            std::u32string classes = text.substr(split + 1);

            VerbRoot root;
            root.whole_stem = whole;
            if (whole) {
                bool marked = false;
                for (char32_t c : text.substr(0, split)) {
                    if (c == U'\'') {
                        root.vowel = root.root.size();
                        marked = true;
                    } else if (c == U':') {
                        root.is_long = true;
                    } else {
                        root.root.push_back(c);
                    }
                }
                if (!marked) continue;
            } else {
                root.root = text.substr(0, split);
                size_t ije = root.root.find(U"ije");
                if (ije == std::u32string::npos) continue;
                root.vowel = ije + 2;
                root.is_long = true;
                // These all shift in the dictionaries: pòdijēlīm, pòdijēljen.
                root.present_shifts = true;
                root.passive = 1;
                size_t second = classes.find(U':');
                if (second != std::u32string::npos) {
                    std::vector<Word> prefixes;
                    split_words(classes.substr(second + 1), prefixes);
                    for (const Word& prefix : prefixes) root.prefixes.push_back(prefix.w);
                    classes.resize(second);
                }
            }
            root.soft = soft_roots(root.root);
            for (char32_t c : classes) {
                switch (c) {
                    case U'i': root.iti = true; break;
                    case U'a': root.ati = root.ati_present = true; break;
                    case U't': root.ati = true; break;
                    case U'u': root.nuti = true; break;
                    case U'e': root.e_present = true; break;
                    case U'n': root.noun_twin = true; break;
                    case U'<': root.present_shifts = true; break;
                    case U'p': root.passive = 1; break;
                    case U'P': root.passive = 2; break;
                    default: break;
                }
            }
            roots.push_back(std::move(root));
        }
    }
    return roots;
}

const std::vector<VerbRoot>& verb_roots() {
    static const std::vector<VerbRoot> roots = load_verb_roots();
    return roots;
}

// One or more verbal prefixes: po, ras-po, u-na, is.
bool is_verbal_prefix(const std::u32string& s) {
    static const char32_t* const PREFIXES[] = {
        U"do", U"iz", U"is", U"na", U"nad", U"o", U"ob", U"od", U"po", U"pod", U"pot",
        U"pre", U"pred", U"pri", U"pro", U"raz", U"ras", U"s", U"sa", U"su", U"u",
        U"uz", U"us", U"za", U"obez",
    };
    if (s.empty()) return true;
    for (const char32_t* prefix : PREFIXES) {
        std::u32string p(prefix);
        if (starts_with(s, p) && is_verbal_prefix(s.substr(p.size()))) return true;
    }
    return false;
}

bool is_one_of(const std::u32string& s, std::initializer_list<const char32_t*> list) {
    for (const char32_t* item : list) {
        if (s == item) return true;
    }
    return false;
}

class StressRules {
public:
    StressRules(const Word& word, VoiceLanguage language, bool opens_clause)
        : m_w(word.w), m_language(language), m_opens_clause(opens_clause) {
        for (int index : word.nuclei) {
            m_letters.push_back(word.letter[static_cast<size_t>(index)]);
        }
    }

    // Verbs keep the accent of the infinitive on their root in the other
    // forms too: uréditi, urédi, urédio; otvòriti, otvòri; podijéliti,
    // podijéli; pročìtati, pročìtaj; pokrénuti, pokréni. The verbs come from
    // two tables in formant_lexicon.cpp: roots with a long "ije", which take
    // any prefix, and whole stems.
    //
    // In the present and the passive participle the dictionaries often move
    // the accent back (ùrēdīm, ùrēđen, òtvorīm, pòdijēlīm). Croatian is
    // commonly spoken without that shift, and the Croatian voice keeps the
    // accent on the root in every form; the other two follow the dictionaries.
    StressResult verb_form() const {
        StressResult r;
        for (const VerbRoot& root : verb_roots()) {
            if (root.whole_stem) {
                if (form_of(root, 0, r)) return r;
                continue;
            }
            // The root's "ije" has to be one syllable, and not the first.
            for (int k = 1; k < count(); ++k) {
                size_t e = static_cast<size_t>(m_letters[static_cast<size_t>(k)]);
                if (e <= root.vowel) continue;
                if (m_w.compare(e - 2, 3, U"ije") != 0 || nucleus_at(e - 2) >= 0) continue;
                size_t start = e - root.vowel;
                std::u32string prefix = m_w.substr(0, start);
                bool allowed = root.prefixes.empty()
                    ? is_verbal_prefix(prefix)
                    : std::find(root.prefixes.begin(), root.prefixes.end(), prefix) !=
                          root.prefixes.end();
                if (allowed && form_of(root, start, r)) return r;
            }
        }
        return StressResult{};
    }

    // Suffixes whose accent position is fixed.
    StressResult strong() const {
        StressResult r = verb_form();
        if (r.nucleus >= 0) return r;
        int n = count();

        // Present, imperative and present participle of the verbs in -ivati
        // and -ovati: the accent is on the syllable before -uj (ukljùčujem,
        // prikàzuje, urèđuju, kùpujem, pùtujući). The few verbs in -ovati
        // that keep it on the first syllable (nàpredujem, sùdjelujem) are
        // in the lexicon.
        static const char32_t* const UJ_ENDINGS[] = {
            U"em", U"eš", U"e", U"emo", U"ete", U"u", U"ući", U"", U"mo", U"te",
        };
        if (n >= 3 && before_suffix(U"uj", UJ_ENDINGS, r)) {
            r.accent = Accent::Rising;
            return r;
        }

        // The same forms of the verbs that keep -av- or -iv- (označavam,
        // obavještavaju, uživaš, pokrivaj). The dictionaries have the accent
        // one syllable before the suffix (oznàčāvām, ùžīvām); the Croatian
        // voice keeps it where the infinitive has it (označávam, užívam),
        // like the other verbs (see verb_form). The third person in -ava is
        // spelled like many nouns and adjectives (država, zabava, krvava),
        // so it counts only from four syllables on (označava, održava), and
        // the one in -iva not at all (osjetljiva, perspektiva); short verbs
        // are in the VERBS table (rješava, uživa).
        static const char32_t* const AM_ENDINGS[] = {
            U"am", U"aš", U"amo", U"ate", U"aju", U"aj", U"ajmo", U"ajte",
        };
        static const char32_t* const A_ENDING[] = {U"a"};
        StressResult suffix;
        if ((n >= 3 && (at_suffix(U"av", AM_ENDINGS, 0, suffix) ||
                        at_suffix(U"iv", AM_ENDINGS, 0, suffix))) ||
            (n >= 4 && !ends_with(m_w, U"slava") && at_suffix(U"av", A_ENDING, 0, suffix))) {
            if (m_language == VoiceLanguage::Croatian) {
                r.nucleus = suffix.nucleus;
                r.is_long = true;
            } else {
                r.nucleus = suffix.nucleus - 1;
                r.long_after = suffix.nucleus;
                r.accent = Accent::Rising;
            }
            return r;
        }
        r = StressResult{};

        // Surnames in -ović/-ević of four or more syllables carry a long
        // rising accent on the syllable before the suffix (Jovánović,
        // Kováčević, Stefánović, Milénković, Halílović); the three-syllable
        // ones keep the first (Pètrović, Màrković, Jánković), which is the
        // default. Exceptions (Ìvanović, Jòsipović) are in the lexicon.
        static const char32_t* const IC_CASES[] = {
            U"ić", U"ića", U"iću", U"ićem", U"ići", U"ićima", U"ićev", U"ićeva",
            U"ićevu", U"ićevo", U"ićevi", U"ićeve", U"ićevim", U"ićevih",
        };
        if (n >= 4 && (before_suffix(U"ov", IC_CASES, r) ||
                       before_suffix(U"ev", IC_CASES, r))) {
            r.is_long = true;
            return r;
        }

        // Agent nouns in -ač and their feminines in -ačica are stressed on
        // the syllable before the suffix (prodàvāč, pretražìvāč, navìjāč,
        // pjevàčica); two-syllable ones (kòvāč) are the default anyway.
        static const char32_t* const AC_CASES[] = {
            U"", U"a", U"u", U"em", U"e", U"i", U"ima", U"ica", U"ice", U"ici",
            U"icu", U"icom", U"ico", U"icama",
        };
        if (n >= 3 && before_suffix(U"ač", AC_CASES, r)) return r;

        // Nouns in -ina: abstract nouns, loans and female names are stressed
        // on the i (brzìna, planìna, veličìna, mašína, Katarína); the
        // possessive -ovina/-evina on the syllable before it (dòmovina,
        // králjevina, polòvina). Nouns in -bina go both ways (sudbìna,
        // rȍdbina) and are left to the lexicon, as are the exceptions
        // (gȍdina, ȉstina, cȁrina, svȉnjetina, Vȍjvodina).
        static const char32_t* const A_CASES[] = {
            U"a", U"e", U"i", U"u", U"om", U"ama",
        };
        if (n >= 3 && !ends_with_any(m_w, {U"bina", U"bine", U"bini", U"binu", U"binom", U"binama"})) {
            if (before_suffix(U"ovin", A_CASES, r) || before_suffix(U"evin", A_CASES, r)) {
                return r;
            }
            if (at_suffix(U"in", A_CASES, 0, r)) return r;
        }

        // -irati verbs: telefonírati, kombinírām, kopíran
        static const char32_t* const IR_ENDINGS[] = {
            U"ati", U"am", U"aš", U"a", U"amo", U"ate", U"aju", U"ao", U"ala",
            U"alo", U"ali", U"ale", U"an", U"ana", U"ano", U"ani", U"ane", U"anu",
            U"anje", U"anja", U"anju", U"anjem", U"ajući", U"aj", U"ajte",
        };
        if (n >= 3 && at_suffix(U"ir", IR_ENDINGS, 0, r)) {
            r.is_long = true;
            return r;
        }

        // -ivati / -avati verbs: pokazívati, održávanje
        static const char32_t* const VA_ENDINGS[] = {
            U"ati", U"at", U"ao", U"ala", U"alo", U"ali", U"ale", U"ah", U"aše",
            U"asmo", U"aste", U"ahu", U"anje", U"anja", U"anju", U"anjem", U"ajući",
        };
        if (n >= 4 && (at_suffix(U"iv", VA_ENDINGS, 0, r) ||
                       at_suffix(U"av", VA_ENDINGS, 0, r))) {
            r.is_long = true;
            return r;
        }

        // -ija and its case forms, comparatives in -iji: stress falls on the
        // syllable before (policija, organizácija, Itàlija, pamètnijī)
        static const char32_t* const IJ_ENDINGS[] = {
            U"a", U"e", U"i", U"u", U"o", U"om", U"ama", U"em", U"eg", U"ega",
            U"emu", U"ih", U"im", U"ima", U"oj",
        };
        if (n >= 3 && before_suffix(U"ij", IJ_ENDINGS, r)) {
            size_t li = static_cast<size_t>(m_letters[static_cast<size_t>(r.nucleus)]);
            r.is_long = m_w[li] == U'a' && m_w.compare(li + 1, 3, U"cij") == 0;
            return r;
        }

        static const char32_t* const NOUN_ENDINGS[] = {
            U"", U"a", U"u", U"om", U"e", U"i", U"ima",
        };
        // -izam: realìzam, turìzma
        static const char32_t* const IZAM[] = {
            U"am", U"ma", U"mu", U"mom", U"me", U"mi", U"mima",
        };
        if (n >= 3 && at_suffix(U"iz", IZAM, 0, r)) return r;

        // -itet: the case forms have a long rising e (kapacitéta,
        // identitétu, kvalitéta). In the nominative the dictionaries move
        // the accent one syllable back (kapacìtēt); Croatian is commonly
        // spoken with the e still long and prominent there (kapacitét), and
        // that is what the Croatian voice does.
        if (n >= 3 && at_suffix(U"itet", NOUN_ENDINGS, 2, r)) {
            r.is_long = true;
            if (m_language != VoiceLanguage::Croatian && ends_with(m_w, U"itet")) {
                r.long_after = r.nucleus;
                r.nucleus -= 1;
                r.is_long = false;
            }
            return r;
        }
        // -ator: organizátor
        if (n >= 3 && at_suffix(U"ator", NOUN_ENDINGS, 0, r)) {
            r.is_long = true;
            return r;
        }

        // Loans stressed on the last syllable at source move one syllable
        // back: dokùment, specijàlist, telèfon, kilògram, dirèktor
        static const char32_t* const PENULT[] = {
            U"ent", U"ant", U"ist", U"log", U"graf", U"fon", U"gram", U"skop",
            U"tor",
        };
        if (n >= 3) {
            for (const char32_t* suffix : PENULT) {
                if (before_suffix(suffix, NOUN_ENDINGS, r)) return r;
            }
        }

        // -ura: kultúra, temperatúra
        static const char32_t* const A_ENDINGS[] = {
            U"a", U"e", U"i", U"u", U"om", U"ama",
        };
        if (n >= 3 && at_suffix(U"ur", A_ENDINGS, 0, r)) {
            r.is_long = true;
            return r;
        }

        // -tika: matemàtika, polìtika
        if (n >= 4 && (before_suffix(U"tik", A_ENDINGS, r) ||
                       before_suffix(U"tic", A_ENDINGS, r))) {
            return r;
        }

        // -ičan, -ički: elèktričan, polìtički
        static const char32_t* const IC_ENDINGS[] = {
            U"an", U"na", U"no", U"ni", U"ne", U"nu", U"nog", U"nom", U"nih",
            U"nim", U"noj", U"ki", U"ka", U"ko", U"ke", U"ku", U"kog", U"kom",
            U"kih", U"kim", U"koj", U"koga", U"kome", U"nima", U"kima",
        };
        if (n >= 4 && before_suffix(U"ič", IC_ENDINGS, r)) return r;

        // Verbal nouns: putòvānje, zanímanje; upozorénje, odobrénje
        static const char32_t* const NJ_ENDINGS[] = {
            U"e", U"a", U"u", U"em", U"ima",
        };
        if (n >= 4 && before_suffix(U"anj", NJ_ENDINGS, r)) return r;
        if (n >= 4 && at_suffix(U"enj", NJ_ENDINGS, 0, r)) {
            r.is_long = true;
            return r;
        }

        return StressResult{};
    }

    // Long infinitives are most often stressed on the antepenult
    // (govòriti, zabòraviti, razùmjeti).
    StressResult weak() const {
        StressResult r;
        int n = count();
        if (n >= 4 && m_w.size() >= 3 && ends_with(m_w, U"ti") &&
            is_vowel_letter(m_w[m_w.size() - 3])) {
            r.nucleus = n - 3;
        }
        return r;
    }

private:
    int count() const { return static_cast<int>(m_letters.size()); }

    // Is the word, from `start` on, a form of the verb with this root?
    bool form_of(const VerbRoot& root, size_t start, StressResult& r) const {
        enum class Form { None, Plain, Present, Passive };
        Form form = Form::None;

        if (m_w.compare(start, root.root.size(), root.root) == 0) {
            std::u32string ending = m_w.substr(start + root.root.size());
            if (root.iti) {
                if (is_one_of(ending, {U"iti", U"it", U"imo", U"ite", U"io", U"ila", U"ilo",
                                       U"ili", U"ile", U"ih", U"ismo", U"iste", U"iše",
                                       U"ivši", U"iću", U"ićeš", U"iće", U"ićemo",
                                       U"ićete"})) {
                    // "uredimo" is both; the imperative keeps the accent in
                    // every voice, the present is the commoner reading.
                    form = ending == U"imo" ? Form::Present : Form::Plain;
                } else if (ending == U"i") {
                    // "Potvrdi", "Uredi", "Otvori" at the head of a clause
                    // are commands; elsewhere (u potvrdi, uredi su...) the
                    // noun has priority.
                    if (!root.noun_twin || m_opens_clause) form = Form::Plain;
                } else if (is_one_of(ending, {U"im", U"iš"}) ||
                           (ending == U"e" && !root.noun_twin)) {
                    form = Form::Present;
                }
            }
            if (form == Form::None && root.ati) {
                if (is_one_of(ending, {U"ati", U"at", U"ao", U"ala", U"alo", U"ali", U"ale",
                                       U"ah", U"asmo", U"aste", U"aše", U"avši", U"ajući",
                                       U"anje", U"anja", U"anju", U"anjem", U"anjima",
                                       U"aću", U"aćeš", U"aće", U"aćemo", U"aćete"})) {
                    form = Form::Plain;
                } else if (root.ati_present &&
                           (is_one_of(ending, {U"am", U"aš", U"amo", U"ate", U"aju", U"aj",
                                               U"ajmo", U"ajte"}) ||
                            (ending == U"a" && !root.noun_twin))) {
                    form = Form::Present;
                } else if (is_one_of(ending, {U"an", U"ana", U"ano", U"ani", U"ane", U"anu",
                                              U"anog", U"anoga", U"anom", U"anome",
                                              U"anoj", U"anih", U"anim", U"anima"})) {
                    form = Form::Passive;
                }
            }
            if (form == Form::None && root.nuti) {
                if (is_one_of(ending, {U"nuti", U"nuo", U"nula", U"nulo", U"nuli", U"nule",
                                       U"nuh", U"nusmo", U"nuste", U"nuše", U"nuvši", U"ni",
                                       U"nimo", U"nite", U"nuću", U"nućeš", U"nuće",
                                       U"nućemo", U"nućete"})) {
                    form = Form::Plain;
                } else if (is_one_of(ending, {U"nem", U"neš", U"ne", U"nemo", U"nete",
                                              U"nu"})) {
                    form = Form::Present;
                } else if (is_one_of(ending, {U"nut", U"nuta", U"nuto", U"nute", U"nutu",
                                              U"nutog", U"nutoga", U"nutom", U"nutoj",
                                              U"nutih", U"nutim", U"nutima"})) {
                    form = Form::Passive;
                }
            }
            if (form == Form::None && root.e_present) {
                if (is_one_of(ending, {U"i", U"imo", U"ite", U"en", U"ena", U"eno", U"eni",
                                       U"ene", U"enu", U"enog", U"enoga", U"enom",
                                       U"enoj", U"enih", U"enim", U"enima"})) {
                    form = Form::Plain;
                } else if (is_one_of(ending, {U"em", U"eš", U"e", U"emo", U"ete", U"u",
                                              U"ući"})) {
                    form = Form::Present;
                }
            }
        }
        if (form == Form::None && root.iti) {
            for (const std::u32string& soft : root.soft) {
                if (m_w.compare(start, soft.size(), soft) == 0 &&
                    is_one_of(m_w.substr(start + soft.size()),
                              {U"en", U"ena", U"eno", U"eni", U"ene", U"enu", U"enog",
                               U"enoga", U"enom", U"enome", U"enomu", U"enoj", U"enih",
                               U"enim", U"enima"})) {
                    form = Form::Passive;
                    break;
                }
            }
        }
        if (form == Form::None) return false;

        int k = nucleus_at(start + root.vowel);
        if (k < 0) return false;
        int shifted = k;
        if (m_language != VoiceLanguage::Croatian) {
            if (form == Form::Present && root.present_shifts) shifted = k - 1;
            if (form == Form::Passive && root.passive == 1) shifted = k - 1;
            if (form == Form::Passive && root.passive == 2) shifted = 0;
        }
        if (shifted >= 0 && shifted < k) {
            r.nucleus = shifted;
            if (root.is_long) r.long_after = k;
            r.accent = Accent::Rising;
        } else {
            r.nucleus = k;
            r.is_long = root.is_long;
        }
        return true;
    }

    int nucleus_at(size_t letter) const {
        for (size_t k = 0; k < m_letters.size(); ++k) {
            if (m_letters[k] == static_cast<int>(letter)) return static_cast<int>(k);
        }
        return -1;
    }

    int nucleus_before(size_t letter) const {
        int found = -1;
        for (size_t k = 0; k < m_letters.size(); ++k) {
            if (m_letters[k] < static_cast<int>(letter)) found = static_cast<int>(k);
        }
        return found;
    }

    template <size_t N>
    bool match(const char32_t* suffix, const char32_t* const (&endings)[N],
               size_t& suffix_pos) const {
        std::u32string base(suffix);
        for (const char32_t* ending : endings) {
            std::u32string full = base + ending;
            if (ends_with(m_w, full) && m_w.size() > full.size()) {
                suffix_pos = m_w.size() - full.size();
                return true;
            }
        }
        return false;
    }

    // Stress on the vowel at `offset` inside the suffix.
    template <size_t N>
    bool at_suffix(const char32_t* suffix, const char32_t* const (&endings)[N],
                   size_t offset, StressResult& r) const {
        size_t pos = 0;
        if (!match(suffix, endings, pos)) return false;
        int k = nucleus_at(pos + offset);
        if (k < 0 || nucleus_before(pos) < 0) return false;
        r.nucleus = k;
        return true;
    }

    // Stress on the syllable before the suffix.
    template <size_t N>
    bool before_suffix(const char32_t* suffix, const char32_t* const (&endings)[N],
                       StressResult& r) const {
        size_t pos = 0;
        if (!match(suffix, endings, pos)) return false;
        int k = nucleus_before(pos);
        if (k < 0) return false;
        r.nucleus = k;
        return true;
    }

    const std::u32string& m_w;
    VoiceLanguage m_language;
    bool m_opens_clause;
    std::vector<int> m_letters;
};

// =============================================================================
// Assimilation
// =============================================================================

Ph voiced_of(Ph ph) {
    switch (ph) {
        case Ph::P: return Ph::B;
        case Ph::T: return Ph::D;
        case Ph::K: return Ph::G;
        case Ph::S: return Ph::Z;
        case Ph::SH: return Ph::ZH;
        case Ph::SJ: return Ph::ZJ;
        case Ph::CH: return Ph::DZH;
        case Ph::TJ: return Ph::DJ;
        case Ph::C: return Ph::DZ;
        case Ph::H: return Ph::GH;
        default: return ph;
    }
}

Ph voiceless_of(Ph ph) {
    switch (ph) {
        case Ph::B: return Ph::P;
        case Ph::D: return Ph::T;
        case Ph::G: return Ph::K;
        case Ph::Z: return Ph::S;
        case Ph::ZH: return Ph::SH;
        case Ph::ZJ: return Ph::SJ;
        case Ph::DZH: return Ph::CH;
        case Ph::DJ: return Ph::TJ;
        case Ph::DZ: return Ph::C;
        case Ph::GH: return Ph::H;
        default: return ph;
    }
}

bool is_postalveolar(Ph ph) {
    return ph == Ph::SH || ph == Ph::ZH || ph == Ph::CH || ph == Ph::DZH ||
           ph == Ph::TJ || ph == Ph::DJ;
}

} // namespace

// =============================================================================
// Frontend
// =============================================================================

Frontend::Frontend(VoiceLanguage language) : m_language(language) {
    size_t count = 0;
    const char* const* entries = lexicon_common(count);
    add_entries(entries, count);

    switch (language) {
        case VoiceLanguage::Serbian:
            entries = lexicon_serbian(count);
            break;
        case VoiceLanguage::Bosnian:
            entries = lexicon_bosnian(count);
            break;
        case VoiceLanguage::Croatian:
        default:
            entries = lexicon_croatian(count);
            break;
    }
    add_entries(entries, count);
}

void Frontend::add_entries(const char* const* entries, size_t count) {
    for (size_t e = 0; e < count; ++e) {
        std::u32string marked = PhonemeMapper::utf8_to_utf32(entries[e]);
        size_t bar = marked.find(U'|');
        if (bar == std::u32string::npos) {
            add_entry(marked);
            continue;
        }
        // "stem|ending|ending": one entry per ending.
        const std::u32string stem = marked.substr(0, bar);
        while (bar != std::u32string::npos) {
            size_t next = marked.find(U'|', bar + 1);
            size_t length = next == std::u32string::npos ? next : next - bar - 1;
            add_entry(stem + marked.substr(bar + 1, length));
            bar = next;
        }
    }
}

void Frontend::add_entry(const std::u32string& marked) {
    std::u32string plain;
    LexEntry entry;
    bool stem = false;

    for (char32_t raw : marked) {
        char32_t c = to_lower(raw);
        if (c == U'\'' || c == U'^' || c == U'/') {
            entry.stress_letter = static_cast<int8_t>(plain.size());
            entry.accent = c == U'^' ? Accent::Falling
                         : c == U'/' ? Accent::Rising : Accent::None;
        } else if (c == U':') {
            if (!plain.empty() && plain.size() <= 32) {
                entry.long_letters |= 1u << (plain.size() - 1);
            }
        } else if (c == U'*') {
            stem = true;
        } else {
            plain.push_back(c);
        }
    }

    // Later (language-specific) tables override the common one.
    (stem ? m_stems : m_exact)[plain] = entry;
}

Utterance Frontend::process(const std::u32string& text, Punctuation punct) const {
    Utterance utt;
    const bool eastern = m_language != VoiceLanguage::Croatian;

    std::vector<Word> words =
        expand_words(tokenize(cyrillic::to_latin(text), eastern), eastern);
    if (words.empty()) {
        return utt;
    }

    // ---- Per-word analysis: phones and lexical stress ----
    // A command opens its clause, alone or after a word like these ("Ne
    // zaboravi", "I potvrdi", "Molim potvrdi"); see StressRules::form_of.
    static const WordSet openers = {
        U"ne", U"i", U"a", U"pa", U"te", U"ili", U"ali", U"da", U"zatim", U"onda",
        U"sada", U"sad", U"molim", U"molimo", U"odmah", U"prvo", U"nemoj",
    };
    bool opens_clause = true;
    for (Word& word : words) {
        const bool at_head = opens_clause;
        if (openers.count(word.w) == 0) opens_clause = false;
        phonemize(word);
        const int n = static_cast<int>(word.nuclei.size());
        if (n == 0) {
            word.clitic = true;
            word.prominence = 0;
            continue;
        }

        auto nucleus_from_letter = [&](int letter) {
            for (int k = 0; k < n; ++k) {
                if (word.letter[static_cast<size_t>(word.nuclei[static_cast<size_t>(k)])] >= letter) {
                    return k;
                }
            }
            return n - 1;
        };
        auto set_long = [&](int k) {
            word.phones[static_cast<size_t>(word.nuclei[static_cast<size_t>(k)])].is_long = true;
        };

        // 1. Accent marks written in the text
        for (size_t i = 0; i < word.marks.size(); ++i) {
            uint8_t mark = word.marks[i];
            if (!mark) continue;
            int k = nucleus_from_letter(static_cast<int>(i));
            if (mark & MARK_LONG) set_long(k);
            if (mark & MARK_STRESS) {
                word.stress = k;
                word.explicit_stress = true;
                word.accent = (mark & MARK_FALL) ? Accent::Falling
                            : (mark & MARK_RISE) ? Accent::Rising : Accent::None;
            }
        }

        // 2. Lexicon: exact form, then the longest stem
        if (word.stress < 0) {
            const LexEntry* entry = nullptr;
            auto exact = m_exact.find(word.w);
            if (exact != m_exact.end()) {
                entry = &exact->second;
            } else {
                size_t min_len = word.w.size() > 3 ? word.w.size() - 3 : 1;
                for (size_t len = word.w.size(); len >= min_len && len >= 2; --len) {
                    auto stem = m_stems.find(word.w.substr(0, len));
                    if (stem != m_stems.end()) {
                        entry = &stem->second;
                        break;
                    }
                }
            }
            if (entry) {
                if (entry->stress_letter >= 0) {
                    word.stress = nucleus_from_letter(entry->stress_letter);
                    word.accent = entry->accent;
                }
                for (int k = 0; k < n; ++k) {
                    int letter = word.letter[static_cast<size_t>(word.nuclei[static_cast<size_t>(k)])];
                    if (letter < 32 && (entry->long_letters & (1u << letter))) set_long(k);
                }
            }
        }

        // 3. Suffix rules, 4. default: first syllable
        if (word.stress < 0) {
            StressRules rules(word, m_language, at_head);
            StressResult r = rules.strong();
            if (r.nucleus < 0) r = rules.weak();
            if (r.nucleus >= 0) {
                word.stress = r.nucleus;
                word.accent = r.accent;
                if (r.is_long) set_long(r.nucleus);
                if (r.long_after >= 0) set_long(r.long_after);
            } else {
                word.stress = 0;
            }
        }

        // Tone: only falling accents on monosyllables, only rising accents
        // on non-initial syllables.
        if (n == 1) {
            word.accent = Accent::Falling;
        } else if (word.stress > 0) {
            word.accent = Accent::Rising;
        } else if (word.accent == Accent::None) {
            word.accent = Accent::Neutral;
        }
    }

    // ---- Clitics and phrase-level accent shifts ----
    const size_t count = words.size();
    for (size_t i = 0; i < count; ++i) {
        Word& word = words[i];
        // Letter names are always full words ("u" in USB is not the
        // preposition, "te" in TV is not the pronoun).
        if (word.nuclei.empty() || word.explicit_stress || word.letter_name) continue;

        bool proclitic = proclitics().count(word.w) != 0;
        bool enclitic = enclitics().count(word.w) != 0;

        if (proclitic && i + 1 < count) {
            word.clitic = true;
        } else if (enclitic && i > 0) {
            // Enclitics cannot open a clause; there the same form is a full
            // word ("Ti si...", "Je li...").
            word.clitic = true;
        } else if (proclitic || enclitic || function_words().count(word.w)) {
            word.prominence = 1;
        }
        if (word.clitic) {
            word.prominence = 0;
        }
    }

    auto take_accent = [](Word& host, Word& donor) {
        host.clitic = false;
        host.prominence = donor.prominence > 0 ? donor.prominence : 2;
        host.stress = 0;
        host.accent = Accent::Rising;
        donor.clitic = true;
        donor.prominence = 0;
    };

    for (size_t i = 0; i + 1 < count; ++i) {
        Word& word = words[i];
        Word& next = words[i + 1];
        if (!word.clitic || next.clitic || next.nuclei.empty() || next.explicit_stress ||
            next.letter_name) {
            continue;
        }
        if (word.w == U"ne") {
            // "znati" hands its accent over in every person and in all three
            // languages: nè znam, nè znamo, nè znaju.
            static const WordSet znati = {U"znamo", U"znate", U"znaju", U"znajući"};
            bool shift = next.nuclei.size() == 1 && next.prominence > 0;
            if (znati.count(next.w) ||
                (m_language != VoiceLanguage::Croatian && ne_shift_verbs().count(next.w))) {
                shift = true;
            }
            if (shift) {
                // The two are then one word, "neznam", in timing as well.
                take_accent(word, next);
                next.joined = true;
                next.prominence = word.prominence;
                if (m_language == VoiceLanguage::Croatian) {
                    // The length the dictionaries keep after the accent
                    // (nè znām) is not heard in Croatian as commonly spoken.
                    for (int nucleus : next.nuclei) {
                        next.phones[static_cast<size_t>(nucleus)].is_long = false;
                    }
                }
            }
        } else if (m_language == VoiceLanguage::Bosnian && word.nuclei.size() == 1 &&
                   shifting_prepositions().count(word.w) &&
                   proclitic_shift_hosts().count(next.w)) {
            take_accent(word, next);
        }
    }

    // A clause needs at least one accent.
    bool any_stress = std::any_of(words.begin(), words.end(), [](const Word& w) {
        return !w.clitic && !w.nuclei.empty();
    });
    if (!any_stress) {
        for (size_t i = count; i-- > 0;) {
            if (!words[i].nuclei.empty()) {
                words[i].clitic = false;
                words[i].prominence = 2;
                break;
            }
        }
    }

    // A lone function word ("Ne.", "Da.", "Ovo:") is the whole message.
    Word* only = nullptr;
    size_t accented = 0;
    for (Word& word : words) {
        if (!word.clitic && !word.nuclei.empty()) {
            only = &word;
            ++accented;
        }
    }
    if (accented == 1) {
        only->prominence = 2;
    }

    // ---- Clause type ----
    switch (punct) {
        case Punctuation::QUESTION: {
            utt.kind = ClauseKind::YesNoQuestion;
            size_t checked = 0;
            for (size_t i = 0; i < count && checked < 2; ++i) {
                if (question_words().count(words[i].w)) {
                    utt.kind = ClauseKind::WhQuestion;
                    break;
                }
                if (!words[i].clitic) ++checked;
            }
            if (utt.kind == ClauseKind::YesNoQuestion) {
                // "Znaš li...", "Je li...", "Da li...": the peak sits on the
                // word the particle leans on.
                for (size_t i = 1; i < count; ++i) {
                    if (words[i].w == U"li" && !words[i - 1].nuclei.empty()) {
                        Word& host = words[i - 1];
                        host.clitic = false;
                        if (host.prominence < 2) host.prominence = 2;
                        utt.focus_word = static_cast<int>(i - 1);
                        break;
                    }
                }
            }
            break;
        }
        case Punctuation::EXCLAMATION:
            utt.kind = ClauseKind::Exclamation;
            break;
        case Punctuation::COMMA:
        case Punctuation::SEMICOLON:
        case Punctuation::COLON:
            utt.kind = ClauseKind::Continuation;
            break;
        default:
            utt.kind = ClauseKind::Statement;
            break;
    }

    // ---- Flatten into one phone string ----
    // A joined word counts as part of the one before it.
    std::vector<size_t> group(count);
    std::vector<int> group_syllables(count, 0);
    for (size_t wi = 0; wi < count; ++wi) {
        group[wi] = (wi > 0 && words[wi].joined) ? group[wi - 1] : wi;
        group_syllables[group[wi]] += static_cast<int>(words[wi].nuclei.size());
    }

    std::vector<bool> word_is_clitic;
    int syllable = 0;
    for (size_t wi = 0; wi < count; ++wi) {
        Word& word = words[wi];
        word_is_clitic.push_back(word.clitic && !word.joined);
        const int n = static_cast<int>(word.nuclei.size());

        int current = syllable;     // consonants before the first vowel
        for (size_t pi = 0; pi < word.phones.size(); ++pi) {
            Phone phone = word.phones[pi];
            phone.word = static_cast<uint16_t>(group[wi]);
            phone.word_start = pi == 0 && !word.joined;
            phone.prominence = word.prominence;
            phone.word_syllables =
                static_cast<uint8_t>(std::min(group_syllables[group[wi]], 255));

            if (phone.nucleus) {
                int k = static_cast<int>(
                    std::find(word.nuclei.begin(), word.nuclei.end(),
                              static_cast<int>(pi)) - word.nuclei.begin());
                current = syllable + k;
                if (!word.clitic && k == word.stress) {
                    phone.stressed = true;
                    phone.accent = word.accent;
                }
            } else if (!phone.nucleus_tail && n > 0) {
                // Onset consonants belong to the following vowel.
                for (int k = 0; k < n; ++k) {
                    if (word.nuclei[static_cast<size_t>(k)] > static_cast<int>(pi)) {
                        current = syllable + k;
                        break;
                    }
                }
            }
            phone.syllable = static_cast<int16_t>(current);
            utt.phones.push_back(phone);
        }
        syllable += n;
    }
    utt.syllable_count = syllable;

    // Stress and length of a syllabic r belong to all three of its parts.
    for (size_t i = 0; i + 2 < utt.phones.size(); ++i) {
        if (utt.phones[i].nucleus && utt.phones[i + 1].nucleus_tail) {
            utt.phones[i + 1].stressed = utt.phones[i + 2].stressed = utt.phones[i].stressed;
            utt.phones[i + 1].is_long = utt.phones[i + 2].is_long = utt.phones[i].is_long;
        }
    }

    // ---- Assimilation inside words and clitic groups ----
    std::vector<Phone>& ph = utt.phones;
    auto bound = [&](size_t i) {
        // true when phones i and i+1 are pronounced without a word break
        if (ph[i].word == ph[i + 1].word) return true;
        return word_is_clitic[ph[i].word] || word_is_clitic[ph[i + 1].word];
    };

    for (size_t i = ph.size(); i-- > 1;) {
        Phone& left = ph[i - 1];
        const Phone& right = ph[i];
        if (!bound(i - 1)) continue;

        // Regressive voicing assimilation between obstruents
        // (predsjednik -> [pretsjednik], s bratom -> [zbratom]).
        if (is_obstruent(left.ph) && is_obstruent(right.ph)) {
            left.ph = ph_def(right.ph).voiced ? voiced_of(left.ph) : voiceless_of(left.ph);
        }

        // s, z before š ž č ć dž đ (s čim -> [ščim]); š, ž before ć, đ.
        if ((left.ph == Ph::S || left.ph == Ph::Z) && is_postalveolar(right.ph)) {
            left.ph = left.ph == Ph::S ? Ph::SH : Ph::ZH;
        }
        if ((left.ph == Ph::SH || left.ph == Ph::ZH) &&
            (right.ph == Ph::TJ || right.ph == Ph::DJ)) {
            left.ph = left.ph == Ph::SH ? Ph::SJ : Ph::ZJ;
        }

        // Nasal place: n before k, g is velar; n before p, b is m.
        if (left.ph == Ph::N) {
            if (right.ph == Ph::K || right.ph == Ph::G) {
                left.ph = Ph::NG;
            } else if ((right.ph == Ph::P || right.ph == Ph::B) && left.word == right.word) {
                left.ph = Ph::M;
            }
        }
    }

    // Two identical consonants across a clitic boundary merge (bez zuba,
    // iz sela, od toga).
    for (size_t i = 0; i + 1 < ph.size();) {
        if (ph[i].ph == ph[i + 1].ph && is_obstruent(ph[i].ph) &&
            ph[i].word != ph[i + 1].word && bound(i)) {
            ph[i + 1].word_start = ph[i].word_start || ph[i + 1].word_start;
            ph.erase(ph.begin() + static_cast<std::ptrdiff_t>(i));
        } else {
            ++i;
        }
    }

    return utt;
}

} // namespace formant
} // namespace laprdus
