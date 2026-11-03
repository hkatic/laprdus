// -*- coding: utf-8 -*-
// formant_slovenian.cpp - Slovenian text front end: letter-to-sound rules,
// stress and the pronunciation lexicon
//
// Sources (see "Slovenian" in docs/formant.md): Slovenski pravopis 2001
// (Pravila, "Glasoslovje": the pronunciation of l and v, the schwa, voicing
// assimilation and final devoicing), Toporišič's Slovenska slovnica (stress,
// the clitics), SSKJ and the Slovenski pravopis on Fran.si for the accents
// of the words in the lexicon, and the Sloleks lexicon (CJVT, CLARIN.SI) for
// the generated table in formant_slovenian.inc.

#include "formant_slovenian.hpp"
#include "../core/phoneme_mapper.hpp"
#include <algorithm>
#include <cstring>
#include <initializer_list>
#include <unordered_set>

namespace laprdus {
namespace formant {
namespace slovenian {

namespace {

constexpr char32_t C_CARON = U'č';
constexpr char32_t C_ACUTE = U'ć';
constexpr char32_t D_STROKE = U'đ';
constexpr char32_t S_CARON = U'š';
constexpr char32_t Z_CARON = U'ž';
constexpr char32_t SCHWA_LETTER = 0x259;     // ə
constexpr char32_t COMBINING_GRAVE = 0x300;
constexpr char32_t COMBINING_ACUTE = 0x301;
constexpr char32_t COMBINING_CIRCUMFLEX = 0x302;
constexpr char32_t COMBINING_MACRON = 0x304;

// Per-letter flags of a word being analysed
constexpr uint8_t F_STRESS = MARK_STRESS;   // 1
constexpr uint8_t F_LONG = MARK_LONG;       // 2
constexpr uint8_t F_CLOSE = 16;             // e, o: close [e], [o] (acute)
constexpr uint8_t F_OPEN = 32;              // e, o: open, said explicitly (circumflex, grave)
constexpr uint8_t F_SCHWA = 64;             // e: the schwa
constexpr uint8_t F_W = 128;                // l: [w] (notation w); a notation l follows the rules
constexpr uint8_t F_L = 4;                  // l: [l] (notation ł), whatever the rules would say

bool is_vowel_letter(char32_t c) {
    return c == U'a' || c == U'e' || c == U'i' || c == U'o' || c == U'u';
}

bool is_base_letter(char32_t c) {
    return (c >= U'a' && c <= U'z') || c == C_CARON || c == C_ACUTE ||
           c == D_STROKE || c == S_CARON || c == Z_CARON;
}

bool is_sonorant_letter(char32_t c) {
    return c == U'l' || c == U'r' || c == U'm' || c == U'n' || c == U'j' || c == U'v';
}

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

// A surname in -ović, -ević (-ovič, -evič, -ovic, -evic) in any of its
// forms (Petrović, Petrovića, Petrovićem, Petrovićeva): where the suffix
// begins, or npos. tools/formant/slovenian_words.py, vic_surname(),
// mirrors it.
size_t vic_surname(const std::u32string& w) {
    static const char32_t* const ENDINGS[] = {
        U"evimi", U"evega", U"evemu", U"evem", U"evih", U"evim", U"ema", U"ama", U"eva", U"evo",
        U"evi", U"eve", U"em", U"om", U"ev", U"ih", U"a", U"u", U"i", U"e", U""};
    for (const char32_t* ending : ENDINGS) {
        const std::u32string e(ending);
        if (!ends_with(w, e)) continue;
        const size_t core = w.size() - e.size();
        if (core < 5) continue;
        const char32_t last = w[core - 1];
        if ((last == C_ACUTE || last == C_CARON || last == U'c') && w[core - 2] == U'i' &&
            w[core - 3] == U'v' && (w[core - 4] == U'o' || w[core - 4] == U'e')) {
            return core - 4;
        }
    }
    return std::u32string::npos;
}

// A word read by the rules of names: written with a capital, or with a
// letter of Croatian, Serbian and Bosnian names (ć, đ) that no Slovenian
// common word has
bool has_name_letter(const std::u32string& w) {
    return w.find(C_ACUTE) != std::u32string::npos || w.find(D_STROKE) != std::u32string::npos;
}

// An ending of the declension of nouns and adjectives or of a possessive
// adjective, also after one consonant: the j or t that names in -r and -e
// take (Cánkarja, Cánkarjev; Jóžeta, Jóžetov) and the consonant after a
// fleeting e (Koróšca of Koróš*ec, Júpitrov, Štúkljem): a name inside a
// clause takes a stem of the lexicon only with one of these, not another
// word's beginning (Ferdolenc: férd* of Ferdo + olenc; Pirnatčič).
// tools/formant/slovenian_words.py, declension_ending(), mirrors it.
bool declension_ending(const std::u32string& e);

// A name as base + an ending of the declension of nouns and adjectives or
// of a possessive adjective, the base ending with `suffix` (Albína: albin
// + a; Dolénca: dolenc + a; Gorénjskega: gorenjsk + ega; Kováčičeva:
// kovačič + eva): the base's length, or 0. Used by the rules of names
// only (default_stress()); the generator does not simulate them.
size_t name_base(const std::u32string& w, const char32_t* suffix) {
    static const char32_t* const ENDINGS[] = {
        U"evimi", U"ovimi", U"inimi", U"evega", U"ovega", U"inega", U"evemu", U"ovemu", U"inemu",
        U"evem", U"ovem", U"inem", U"evih", U"ovih", U"inih", U"evim", U"ovim", U"inim",
        U"eva", U"evo", U"evi", U"eve", U"ova", U"ovo", U"ovi", U"ove", U"ina", U"ino", U"ini", U"ine",
        U"ega", U"emu", U"imi", U"ima",
        U"ama", U"ami", U"ema", U"oma", U"ah", U"am", U"em", U"om", U"ih", U"im", U"ov", U"ev", U"in",
        U"a", U"e", U"i", U"o", U"u", U""};
    const std::u32string suf(suffix);
    for (const char32_t* ending : ENDINGS) {
        const std::u32string e(ending);
        if (!ends_with(w, e) || w.size() < e.size() + suf.size()) continue;
        const size_t base = w.size() - e.size();
        if (w.compare(base - suf.size(), suf.size(), suf) == 0) return base;
    }
    return 0;
}

bool declension_ending(const std::u32string& e) {
    static const std::unordered_set<std::u32string> ENDINGS = [] {
        std::unordered_set<std::u32string> set;
        for (const char32_t* ending : {
                 U"evimi", U"ovimi", U"inimi", U"evega", U"ovega", U"inega", U"evemu", U"ovemu", U"inemu",
                 U"evem", U"ovem", U"inem", U"evih", U"ovih", U"inih", U"evim", U"ovim", U"inim",
                 U"eva", U"evo", U"evi", U"eve", U"ova", U"ovo", U"ovi", U"ove", U"ina", U"ino", U"ini",
                 U"ine", U"ega", U"emu", U"imi", U"ima", U"ama", U"ami", U"ema", U"oma", U"ah", U"am",
                 U"em", U"om", U"ih", U"im", U"ov", U"ev", U"in", U"ijo", U"jo", U"a", U"e", U"i", U"o",
                 U"u", U""}) {
            set.insert(ending);
        }
        return set;
    }();
    if (ENDINGS.count(e)) return true;
    const size_t consonants = starts_with(e, U"lj") || starts_with(e, U"nj") ? 2 : 1;
    return e.size() > consonants && !is_vowel_letter(e[0]) && ENDINGS.count(e.substr(consonants));
}

using WordSet = std::unordered_set<std::u32string>;

// =============================================================================
// Names
// =============================================================================

struct LetterName {
    const char32_t* letter;
    const char32_t* name;
};

// Slovenska abeceda (Slovenski pravopis 2001, § 7-9, "Imena črk"): the
// vowels á é í ó ú, the consonants bé cé čé dé ef gé há jé ká èl èm èn pé èr
// ès èš té vé zé žé (the rules also allow the sound with a schwa, bə, cə,
// which is the spelling by sounds, and so dvojni və for w: expand_words() in
// formant_frontend.cpp). The letters of other alphabets: kú, dvojni vé, íks,
// ípsilon; ć and đ are mehki čé and mehki džé, dž is trdi
// džé. The pronunciation of the names is in the lexicon.
const LetterName LETTER_NAMES[] = {
    {U"a", U"a"},   {U"b", U"be"},  {U"c", U"ce"},  {U"č", U"če"},  {U"d", U"de"},
    {U"e", U"e"},   {U"f", U"ef"},  {U"g", U"ge"},  {U"h", U"ha"},  {U"i", U"i"},
    {U"j", U"je"},  {U"k", U"ka"},  {U"l", U"el"},  {U"m", U"em"},  {U"n", U"en"},
    {U"o", U"o"},   {U"p", U"pe"},  {U"r", U"er"},  {U"s", U"es"},  {U"š", U"eš"},
    {U"t", U"te"},  {U"u", U"u"},   {U"v", U"ve"},  {U"z", U"ze"},  {U"ž", U"že"},
    {U"q", U"ku"},  {U"w", U"dvojni ve"}, {U"x", U"iks"}, {U"y", U"ipsilon"},
    {U"ć", U"mehki če"}, {U"đ", U"mehki dže"},
    {U"lj", U"el je"}, {U"nj", U"en je"}, {U"dž", U"trdi dže"},
};

struct SymbolName {
    char32_t symbol;
    const char32_t* name;
};

// Symbols read by name where they stand inside a clause (see SYMBOLS in
// formant_frontend.cpp): the period that is not a clause end, the colon of
// a clock time, and the signs that follow a number.
const SymbolName SYMBOLS[] = {
    {U'.', U"pika"},
    {0x2024, U"pika"},      // the decimal point, from the number converter
    {U':', U"dvopičje"},
    {U'!', U"klicaj"},
    {U'%', U"odstotkov"},
    {U'&', U"in"},
    {U'+', U"plus"},
    {U'=', U"enako"},
    {U'@', U"afna"},
    {0x20AC, U"evrov"},
    {U'$', U"dolarjev"},
    {0xB0, U"stopinj"},
};

struct SpelledName {
    const char* character;
    const char* name;
};

// The names of digits, punctuation and symbols when spelled (the Croatian
// ones are in data/dictionary/spelling.json). Slovenian usage: Slovenski
// pravopis 2001, "Ločila"; the names of the computer symbols as the
// Slovenian localizations of NVDA and Windows give them.
const SpelledName SPELLED[] = {
    {"0", u8"nič"}, {"1", "ena"}, {"2", "dve"}, {"3", "tri"}, {"4", u8"štiri"},
    {"5", "pet"}, {"6", u8"šest"}, {"7", "sedem"}, {"8", "osem"}, {"9", "devet"},
    {" ", "presledek"},
    {".", "pika"}, {",", "vejica"}, {"!", "klicaj"}, {"?", u8"vprašaj"},
    {";", u8"podpičje"}, {":", u8"dvopičje"}, {"-", "vezaj"}, {"_", u8"podčrtaj"},
    {"'", u8"opuščaj"}, {"\"", "narekovaj"},
    {"(", "oklepaj"}, {")", "zaklepaj"},
    {"[", "oglati oklepaj"}, {"]", "oglati zaklepaj"},
    {"{", "zaviti oklepaj"}, {"}", "zaviti zaklepaj"},
    {"/", u8"poševnica"}, {"\\", u8"leva poševnica"},
    {"@", "afna"}, {"#", "lojtra"}, {"$", "dolar"}, {"%", "odstotek"},
    {"^", u8"strešica"}, {"&", "in"}, {"*", "zvezdica"}, {"+", "plus"},
    {"=", u8"enačaj"}, {"<", u8"manjše"}, {">", u8"večje"}, {"|", u8"navpičnica"},
    {"~", "tilda"}, {"`", "krativec"},
    {u8"€", "evro"}, {u8"£", "funt"}, {u8"°", "stopinja"}, {u8"§", "paragraf"},
    {u8"–", u8"pomišljaj"}, {u8"—", u8"dolgi pomišljaj"}, {u8"…", u8"tripičje"},
    {u8"„", "narekovaj"}, {u8"“", "narekovaj"}, {u8"”", "narekovaj"},
    {u8"«", "narekovaj"}, {u8"»", "narekovaj"}, {u8"‚", u8"opuščaj"}, {u8"‘", u8"opuščaj"},
    {u8"’", u8"opuščaj"}, {u8"©", "avtorske pravice"}, {u8"®", u8"registrirana znamka"},
    {u8"™", "blagovna znamka"}, {u8"·", "pika na sredini"}, {u8"•", "krogec"},
    {u8"×", "krat"}, {u8"÷", "deljeno"}, {u8"±", "plus minus"}, {u8"‰", "promil"},
    {u8"¼", u8"četrt"}, {u8"½", "pol"}, {u8"¾", "tri četrt"},
};

// =============================================================================
// Word classes (Toporišič, Slovenska slovnica: naslonke; Slovenski pravopis)
// =============================================================================

// Proclitics: prepositions and conjunctions without a stress of their own.
const WordSet& proclitics() {
    static const WordSet set = {
        U"v", U"na", U"za", U"pri", U"po", U"s", U"z", U"k", U"h", U"o", U"ob",
        U"od", U"do", U"iz", U"brez", U"čez", U"med", U"nad", U"pod", U"pred",
        U"skozi", U"proti", U"zoper", U"u",
        U"in", U"pa", U"ter", U"ali", U"da", U"ki", U"ko", U"ker", U"če", U"a",
        U"ne", U"naj", U"saj", U"le", U"še", U"že", U"ni", U"niti",
    };
    return set;
}

// Enclitics: the forms of biti, the future auxiliary, the conditional
// particle and the short pronouns.
const WordSet& enclitics() {
    static const WordSet set = {
        U"sem", U"si", U"je", U"sva", U"sta", U"smo", U"ste", U"so",
        U"bom", U"boš", U"bo", U"bova", U"bosta", U"bomo", U"boste", U"bodo",
        U"bi",
        U"me", U"te", U"se", U"ga", U"jo", U"ju", U"jih", U"mi", U"ti", U"mu",
        U"ji", U"jim", U"nam", U"vam", U"nas", U"vas", U"nju", U"nam",
    };
    return set;
}

// Stressed, but weaker than content words. Not bila, bilo, bili, bile:
// the participle of biti is a verb with an accent of its own on the
// last syllable (bilà biló bilí bilè), which as a function word's small
// movement was lost after the higher bi and heard on the first syllable.
const WordSet& function_words() {
    static const WordSet set = {
        U"jaz", U"ti", U"on", U"ona", U"ono", U"mi", U"vi", U"oni", U"one",
        U"midva", U"vidva", U"onadva", U"ta", U"to", U"tisti", U"tista", U"tisto",
        U"oni", U"ta", U"tega", U"temu", U"tem", U"teh", U"tej", U"tisto",
        U"moj", U"moja", U"moje", U"tvoj", U"tvoja", U"tvoje", U"njegov",
        U"njegova", U"njen", U"njena", U"naš", U"naša", U"vaš", U"vaša",
        U"svoj", U"svoja", U"svoje", U"njihov", U"njihova",
        U"vse", U"vsi", U"ves", U"vsa", U"vsak", U"vsaka", U"vsako",
        U"nekaj", U"nekdo", U"nič", U"nihče", U"nekateri", U"neki", U"neka", U"neko",
        U"tudi", U"samo", U"le", U"zelo", U"prav", U"tako", U"tam", U"tu", U"tukaj",
        U"zdaj", U"sedaj", U"potem", U"nato", U"zato", U"torej", U"pa", U"sicer",
        U"ampak", U"vendar", U"toda", U"kajti", U"zakaj",
        U"bil", U"biti", U"bodi", U"bodite",
        U"nisem", U"nisi", U"ni", U"nismo", U"niste", U"niso", U"nisva", U"nista",
        U"bova", U"bosta", U"ima", U"imam", U"imaš", U"imamo", U"imate", U"imajo",
        U"nima", U"nimam", U"lahko", U"mora", U"moram", U"treba", U"more", U"morem",
        U"mene", U"tebe", U"sebe", U"njega", U"nje", U"njih", U"njo", U"njim",
        U"njemu", U"njej", U"nama", U"vama", U"naju", U"vaju",
        U"okoli", U"okrog", U"poleg", U"zraven", U"mimo", U"blizu", U"zaradi",
        U"glede", U"namesto", U"razen", U"kljub", U"preko", U"prek", U"izza",
        U"izpod", U"iznad", U"izpred", U"ampak", U"kot", U"kakor", U"kadar", U"dokler",
        U"preden", U"potem", U"medtem", U"čeprav", U"četudi", U"sicer", U"zato",
        U"ja", U"da", U"ne", U"seveda", U"morda", U"mogoče", U"verjetno",
    };
    return set;
}

// Words that open a wh-question.
const WordSet& question_words() {
    static const WordSet set = {
        U"kdo", U"kaj", U"kje", U"kam", U"kod", U"kdaj", U"kako", U"zakaj",
        U"koliko", U"kateri", U"katera", U"katero", U"katerega", U"kateremu",
        U"katerem", U"katerih", U"katerim", U"katere", U"čigav", U"čigava",
        U"čigavo", U"kakšen", U"kakšna", U"kakšno", U"kakšni", U"kakšne",
        U"kolikšen", U"kolikšna", U"odkod", U"odkje", U"doklej", U"komu",
        U"koga", U"kom", U"čemu", U"česa", U"čim", U"kolikokrat", U"kolikšen",
    };
    return set;
}

// =============================================================================
// The built-in lexicon
// =============================================================================

// Hand-written entries. They win over the generated table, which since
// 2026-10-30 holds every Sloleks lemma with hand-checked accents and is
// verified form by form, so only two kinds are left: words Sloleks does
// not list, and readings SSKJ² or the Slovenski pravopis 2001 (Fran.si)
// give otherwise than Sloleks's first accentuation, or that a listener
// asked for. A hand-written stem catches the words derived from it too
// (gôr* read goreti as gôreti), so whole forms are preferred; the
// readings that differ from Sloleks for every form of a word belong in
// CORRECTIONS of tools/formant/slovenian_words.py, which writes them into
// the table. Check a new entry against Fran and the table first
// (2026-10-29: 281 of 1,605 entries written from memory contradicted
// Sloleks, and the dictionaries sided with Sloleks every time).
const char* const HAND_ENTRIES[] = {
    // ---- The commoner word where the corpus counts of a form mislead: áfna
    // the noun, also the name of @ (SSKJ², SP 2001, eSSKJ; the verb afnati's
    // afná was taken for it, clause_reading() has the verb after se), rôka
    // the hand (SSKJ², SP; the genitive róka of rók "deadline" is commoner in
    // the news corpus, clause_reading() has it after a preposition or a
    // genitive, the name Róka is in the tables of names), pòdmení the submenu
    // (SSKJ² mení; the locative podméni of podména "hypothesis" is commoner in
    // the corpus, not on a screen) ----
    u8"áfna", u8"rôka", u8"pòdmení",
    // ---- Numbers Sloleks does not list ----
    u8"pétdeset", u8"šéstdeset", u8"devétdeset",
    u8"trilijárd|a|i|e|o", u8"kvadrilijón|a|i|ov", u8"kvintilijón|a|i|ov",
    // ---- s'əm: the stressed sem. The participle of biti is the table's,
    // bilà biló bilí bilè (SSKJ² bíl bilà biló in bilò bilì in bilí bilè,
    // "stil. bíla"; SP 2001 bíl -à -ó in -ò, bilí in -ì -è); until
    // 2026-10-30 the stylistic bíla bílo bíli bíle were listed here ----
    u8"s'əm",
    // ---- Where Sloleks's first reading is not the dictionaries' first or
    // the commoner word: otròk (SSKJ² nom. otròk, gen. pl. otrók), dní
    // (SSKJ² rod. mn. dní), gôzda (gòzd gôzda tudi gózd), glasú (glás -ú
    // tudi -a), naglàs the accent (the adverb naglás "aloud" is commoner in
    // the corpus), ôno ône (the personal pronoun, óno ône the
    // demonstrative), dà (the particle). (Sobóta, here until 2026-10-30
    // against the town Múrska Sóbota, is the table's: a name no longer
    // wins over a common word there, and a capital reads the name from
    // the tables of names, name_reading().) ----
    u8"otròk", u8"dní", u8"gôzda", u8"glasú", u8"naglàs",
    u8"ôno", u8"ône", u8"dà",
    // ---- Verbs: the first variant of SSKJ² where Sloleks has the second
    // (písati in pisáti, potrdíti in potŕditi, shraníti in shrániti), the
    // present where Sloleks lists another word of the same spelling
    // (naredíjo, stojíjo, živíjo "they live"; žívijo the greeting is in
    // HEAD_READINGS), jé (jésti jém), tŕdi (SP: trdíti in tŕditi -im,
    // tŕdi), kúpite, vŕnite (SP: kúpi -te, vŕni -te), povéj (SP povèj/
    // povéj), razúmel (SSKJ² razúmel tudi razumèl in razumél) ----
    u8"písati", u8"potrdíti", u8"shraníti", u8"shraníla", u8"shraníli",
    u8"naredíjo", u8"stojíjo", u8"živíjo", u8"jé", u8"tŕdi", u8"kúpite", u8"vŕnite",
    u8"povéj", u8"razúmel", u8"gréjo", u8"dájo", u8"dobíjo", u8"jéjo", u8"učíjo",
    // ---- Words of computers and phones, and names ----
    u8"ínternet", u8"ínternet*", u8"menu=mení",      // SSKJ² mení tudi menu [mení]
    u8"žívjo", u8"zarádi", u8"médtem", u8"anglésk*",
    u8"laprdus=láprdus*",
};

// Spellings whose reading depends on where they stand: at the head of a
// clause (or after a conjunction or an adverb that opens a request) they
// are the greeting, the imperative of a button label or the name of a
// menu, elsewhere the present tense of the verb or another word spelled
// the same (SSKJ, SP 2001: naredíti -ím, imperative narêdi; govoríti -ím,
// govôri; dobíti -ím, dôbi; zapustíti -ím, zapústi; storíti -ím, stôri;
// delíti -ím, dêli; uredíti -ím, urêdi; ponovíti -ím, ponôvi; obnovíti
// -ím, obnôvi; naložíti -ím, nalôži; osvežíti -ím, osvéžil; žívijo the
// greeting, živíjo "they live"). The head reading is given here; the
// other comes from the lexicon. Meni has a rule of its own
// (clause_reading()).
struct HeadReading {
    const char32_t* spelling;
    const char32_t* head;
    bool speech;        // a verb of saying: the present before a clause of da
};

const HeadReading HEAD_READINGS[] = {
    {U"živijo", U"žívijo", false},
    {U"živio", U"žívio", false},
    {U"naredi", U"narêdi", false},
    {U"govori", U"govôri", true},
    {U"odgovori", U"odgovôri", true},
    {U"dobi", U"dôbi", false},
    {U"zapusti", U"zapústi", false},
    {U"spusti", U"spústi", false},
    {U"stori", U"stôri", false},
    {U"prosi", U"prôsi", true},
    {U"deli", U"dêli", false},          // the Share button; déli "parts" elsewhere
    {U"uredi", U"urêdi", false},
    {U"ponovi", U"ponôvi", false},
    {U"obnovi", U"obnôvi", false},
    {U"naloži", U"nalôži", false},
    {U"osveži", U"osvéži", false},
};

// The flags a notation letter carries, and the plain letter it stands for.
// Returns false for a character that is not part of the notation.
bool notation_letter(char32_t c, char32_t& plain, uint8_t& flags) {
    flags = 0;
    switch (c) {
        case 0xE1: plain = U'a'; flags = F_STRESS | F_LONG; return true;             // á
        case 0xE0: plain = U'a'; flags = F_STRESS; return true;                      // à
        case 0xE9: plain = U'e'; flags = F_STRESS | F_LONG | F_CLOSE; return true;   // é
        case 0xEA: plain = U'e'; flags = F_STRESS | F_LONG | F_OPEN; return true;    // ê
        case 0xE8: plain = U'e'; flags = F_STRESS | F_OPEN; return true;             // è
        case 0xED: plain = U'i'; flags = F_STRESS | F_LONG; return true;             // í
        case 0xEC: plain = U'i'; flags = F_STRESS; return true;                      // ì
        case 0xF3: plain = U'o'; flags = F_STRESS | F_LONG | F_CLOSE; return true;   // ó
        case 0xF4: plain = U'o'; flags = F_STRESS | F_LONG | F_OPEN; return true;    // ô
        case 0xF2: plain = U'o'; flags = F_STRESS | F_OPEN; return true;             // ò
        case 0xFA: plain = U'u'; flags = F_STRESS | F_LONG; return true;             // ú
        case 0xF9: plain = U'u'; flags = F_STRESS; return true;                      // ù
        case 0x155: plain = U'r'; flags = F_STRESS | F_LONG; return true;            // ŕ
        case SCHWA_LETTER: plain = U'e'; flags = F_SCHWA; return true;               // ə
        case U'w': plain = U'l'; flags = F_W; return true;
        case 0x142: plain = U'l'; flags = F_L; return true;                          // ł
        default:
            if (is_base_letter(c)) { plain = c; return true; }
            return false;
    }
}

// Reads a pronunciation in the notation into plain letters and flags.
// Accepts the ASCII marks too (' before the stressed vowel, : after a long
// one) and the combining accents. Returns false when something in it is
// not notation.
bool read_notation(const std::u32string& pron, std::u32string& plain, std::vector<uint8_t>& flags,
                   std::string* error) {
    plain.clear();
    flags.clear();
    bool stress_pending = false;
    for (size_t i = 0; i < pron.size(); ++i) {
        char32_t c = pron[i];
        if (c == U'\'') { stress_pending = true; continue; }
        if (c == U':') {
            if (plain.empty()) { if (error) *error = "length mark before any letter"; return false; }
            flags.back() |= F_LONG;
            continue;
        }
        if (c == COMBINING_GRAVE || c == COMBINING_ACUTE || c == COMBINING_CIRCUMFLEX ||
            c == COMBINING_MACRON) {
            if (plain.empty()) { if (error) *error = "accent before any letter"; return false; }
            uint8_t& f = flags.back();
            if (c == COMBINING_MACRON) {
                f |= F_LONG;
            } else {
                f |= F_STRESS;
                if (c != COMBINING_GRAVE) f |= F_LONG;
                if (plain.back() == U'e' || plain.back() == U'o') {
                    f |= c == COMBINING_ACUTE ? F_CLOSE : F_OPEN;
                }
            }
            continue;
        }
        char32_t letter;
        uint8_t f;
        if (!notation_letter(c, letter, f)) {
            if (error) *error = "not a letter of the notation";
            return false;
        }
        if (stress_pending) {
            if (!is_vowel_letter(letter) && letter != U'r') {
                if (error) *error = "stress mark before a consonant";
                return false;
            }
            f |= F_STRESS;
            stress_pending = false;
        }
        plain.push_back(letter);
        flags.push_back(f);
    }
    if (stress_pending) { if (error) *error = "stress mark at the end"; return false; }
    // A stressed short e or o is open; a stressed long one without a
    // quality mark (the ASCII form "'e:") is open too: the close vowel is
    // always written é, ó.
    for (size_t i = 0; i < plain.size(); ++i) {
        if ((plain[i] == U'e' || plain[i] == U'o') && (flags[i] & F_STRESS) &&
            !(flags[i] & (F_CLOSE | F_OPEN | F_SCHWA))) {
            flags[i] |= F_OPEN;
        }
    }
    return true;
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

// r is a syllable of its own ([ər]) between consonants, at the head of a
// word before a consonant and at the end after one: prst, vrt, trg, rdeč,
// rjav, Trst, četrtek.
bool is_syllabic_r(const std::u32string& w, size_t i) {
    char32_t prev = i > 0 ? w[i - 1] : 0;
    char32_t next = i + 1 < w.size() ? w[i + 1] : 0;
    if (is_vowel_letter(prev) || is_vowel_letter(next)) return false;
    if (prev == U'r' || next == U'r') return false;
    return true;
}

// Where an l before a consonant is [w] (Slovenski pravopis 2001, § 650 b
// and c): in the roots with -ol- between consonants that the rules list
// (dolg, golčati, golk, golt, jabolko, kolcati se, kolč, kolk, kolnem, molk,
// molčati, molsti, polh, poln, polt, polzeti, polž, solza, tolči, tolst,
// tolšča, volk, volna, žolč, žolna, žolt), and in the -lc-, -lk-, -lsk-,
// -lstv- of agent nouns derived from verbs (bralec, bralca, bralka, bralski,
// bralstvo; igralec, gledalka, nosilec). Everywhere else an l before a
// consonant is [l] (kultura, film, Alpe, polje, delno, alge: Čibej 2023
// found [l] in 71% of the words of the Sloleks lexicon). A lexicon entry
// settles a word either way.
bool l_vocalizes_before_consonant(const std::u32string& w, size_t i) {
    static const char32_t* const OL_ROOTS[] = {
        U"dolg", U"golč", U"golk", U"golt", U"jabolk", U"kolc", U"kolč", U"kolk", U"koln",
        U"molk", U"molč", U"mols", U"polh", U"poln", U"polt", U"polz", U"polž", U"solz",
        U"tolč", U"tolst", U"tolšč", U"volk", U"voln", U"žolč", U"žoln", U"žolt",
    };
    if (i >= 2 && w[i - 1] == U'o') {
        for (const char32_t* root : OL_ROOTS) {
            std::u32string r(root);
            size_t l = r.find(U'l');
            if (i >= l && w.compare(i - l, r.size(), r) == 0) return true;
        }
    }
    // -alec, -alca, -alka, -alski, -alstvo, -ilec, -ilka ...: a vowel, the l,
    // then c, k or s + k/t, with at least one syllable before the vowel
    if (i >= 2 && (w[i - 1] == U'a' || w[i - 1] == U'i' || w[i - 1] == U'e') && i + 1 < w.size()) {
        char32_t c = w[i + 1];
        bool agent = c == U'c' || c == U'k' ||
                     (c == U's' && i + 2 < w.size() && (w[i + 2] == U'k' || w[i + 2] == U't'));
        if (agent) {
            for (size_t k = 0; k + 1 < i; ++k) {
                if (is_vowel_letter(w[k])) return true;
            }
        }
    }
    return false;
}

// Nouns and adjectives whose final l after a vowel is [w] (Slovenski
// pravopis 2001, § 650 č and d): stol, žival, sol, bel, gol, fižol, kol,
// vol, drhal, piščal, zibel, cel, debel, gnil, vesel, zrel, pol, žal, tal;
// the participles and the words with a schwa before the l are rules.
bool l_final_w_word(const std::u32string& w) {
    static const WordSet words = {
        U"stol", U"žival", U"sol", U"bel", U"gol", U"fižol", U"kol", U"vol", U"drhal",
        U"piščal", U"zibel", U"cel", U"debel", U"gnil", U"vesel", U"zrel", U"pol", U"žal",
        U"tal", U"dol", U"spol", U"stol", U"pepel", U"kotel", U"kozel", U"orel", U"osel",
        U"misel", U"topel", U"mrzel", U"posel", U"kašelj",
    };
    return words.count(w) != 0;
}

// Words that keep the [l] although the rule says [w], beside the table
// SLOVENIAN_FINAL_L: dél (SSKJ: dél -a [tudi deu̯], the [l] first), the loans
// šál, bál, gél, mól, and frequent foreign words without an accent in Sloleks,
// which a stem or the rule would otherwise read (Paul, email, cocktail).
bool l_final_l_word(const std::u32string& w) {
    static const WordSet words = {U"del", U"šal", U"bal", U"gel", U"mol", U"paul", U"opel", U"posedel",
                                  U"musical", U"email", U"mail", U"hotmail", U"cocktail"};
    return words.count(w) != 0;
}

struct Analysis {
    std::u32string plain;           // letters
    std::vector<uint8_t> flags;     // per letter
    size_t notation_letters = 0;    // letters that came from the lexicon's notation (the rest by rule)
    bool letter_name = false;
    bool unknown_name = false;      // written with a capital and in no lexicon
};

void add_phone(WordPhones& out, Ph ph, int letter, bool nucleus) {
    Phone phone;
    phone.ph = ph;
    phone.nucleus = nucleus;
    if (nucleus) out.nuclei.push_back(static_cast<int>(out.phones.size()));
    out.phones.push_back(phone);
    out.letter.push_back(letter);
}

// The phones of the letters. Vowel quality and length follow the flags
// where they say so, the rules of unstressed vowels otherwise (every
// unstressed e and o is open); the stressed vowel's quality is settled
// after the stress is known (finish_word).
void letters_to_phones(const Analysis& a, WordPhones& out, std::vector<bool>& prothetic) {
    const std::u32string& w = a.plain;
    const size_t n = w.size();
    prothetic.clear();
    for (size_t i = 0; i < n; ++i) {
        char32_t c = w[i];
        uint8_t f = a.flags[i];
        char32_t next = i + 1 < n ? w[i + 1] : 0;
        const int li = static_cast<int>(i);

        if (c == U'd' && next == Z_CARON) {
            add_phone(out, Ph::DZH, li, false);
            ++i;
            continue;
        }
        if ((c == U'l' || c == U'n') && next == U'j') {
            // lj, nj are the two sounds l + j, n + j before a vowel
            // (Ljubljana, polje, konja) and a plain l, n before a consonant
            // and at the end of a word (konj, Kranj, cilj, manjši, poljski):
            // Slovenski pravopis 2001, § 664-666
            char32_t after = i + 2 < n ? w[i + 2] : 0;
            add_phone(out, c == U'l' ? Ph::L : Ph::N, li, false);
            if (is_vowel_letter(after)) add_phone(out, Ph::J, li + 1, false);
            ++i;
            continue;
        }
        if (is_vowel_letter(c)) {
            Ph ph;
            if (c == U'a') ph = Ph::A;
            else if (c == U'i') ph = Ph::I;
            else if (c == U'u') ph = Ph::U;
            else if (c == U'e') ph = (f & F_SCHWA) ? Ph::AX : (f & F_CLOSE) ? Ph::EE : Ph::EH;
            else ph = (f & F_CLOSE) ? Ph::OO : Ph::OH;
            add_phone(out, ph, li, true);
            out.phones.back().is_long = (f & F_LONG) != 0 && ph != Ph::AX;
            prothetic.push_back(false);
            // i before another vowel in the words of Greek and Latin origin
            // is [ij]: pacient, socialen, radio, material (§ 701)
            if (c == U'i' && (next == U'a' || next == U'e' || next == U'o' || next == U'u') &&
                i + 1 < a.notation_letters + (a.notation_letters == 0 ? n : 0)) {
                add_phone(out, Ph::J, li, false);
            }
            continue;
        }
        if (c == U'r' && is_syllabic_r(w, i)) {
            // [ər]: the schwa carries the syllable, the r follows it
            add_phone(out, Ph::AX, li, true);
            out.phones.back().is_long = false;
            prothetic.push_back(false);
            add_phone(out, Ph::R, li, false);
            out.phones.back().nucleus_tail = true;
            continue;
        }
        if (c == U'v') {
            // Slovenski pravopis 2001, § 642-645: [v] only before a vowel of
            // the same word (vino, vrt [vərt]); after a vowel, before a
            // consonant or at the end, the glide [u̯] (siv, avto, prav);
            // at the head of a word before a consonant [w] (vlak, vrana,
            // vzeti), which may also be the vowel [u] (vsak [usak]): the
            // vowel is used before an obstruent, where a glide alone is hard
            // to hear, and never takes the stress.
            if (is_vowel_letter(next) || (next == U'r' && i + 1 < n && is_syllabic_r(w, i + 1))) {
                add_phone(out, Ph::V, li, false);
            } else if (i == 0 && next != 0 && !is_sonorant_letter(next)) {
                add_phone(out, Ph::U, li, true);
                out.phones.back().is_long = false;
                prothetic.push_back(true);
            } else {
                add_phone(out, Ph::W, li, false);
            }
            continue;
        }
        if (c == U'l') {
            if (f & F_W) {
                add_phone(out, Ph::W, li, false);
            } else if (is_vowel_letter(next) || next == U'j') {
                add_phone(out, Ph::L, li, false);
            } else if (next == 0) {
                // At the end of a word: settled in finish_word, once the
                // stress is known (bil, delal: [w]; hotel: [l])
                add_phone(out, Ph::L, li, false);
            } else if (l_vocalizes_before_consonant(w, i)) {
                add_phone(out, Ph::W, li, false);
            } else {
                add_phone(out, Ph::L, li, false);
            }
            continue;
        }
        Ph ph = consonant_phone(c);
        if (ph != Ph::SIL) add_phone(out, ph, li, false);
    }
}

// Does a stem of the ones (ena ... devet) stand before "in" + a tens word?
// Compound numerals: enaindvajset, štiriinštirideset.
bool split_compound_numeral_impl(const std::u32string& w, std::u32string& ones, std::u32string& tens) {
    static const char32_t* const ONES[] = {
        U"ena", U"dva", U"tri", U"štiri", U"pet", U"šest", U"sedem", U"osem", U"devet",
    };
    static const char32_t* const TENS[] = {
        U"dvajset", U"trideset", U"štirideset", U"petdeset", U"šestdeset", U"sedemdeset",
        U"osemdeset", U"devetdeset",
    };
    for (const char32_t* o : ONES) {
        std::u32string head = std::u32string(o) + U"in";
        if (!starts_with(w, head)) continue;
        std::u32string rest = w.substr(head.size());
        for (const char32_t* t : TENS) {
            if (starts_with(rest, t)) {
                ones = o;
                tens = rest;
                return true;
            }
        }
    }
    return false;
}

} // namespace

// =============================================================================
// Names
// =============================================================================

const char32_t* letter_name(const std::u32string& letter) {
    for (const LetterName& n : LETTER_NAMES) {
        if (letter == n.letter) return n.name;
    }
    return U"";
}

const char32_t* symbol_name(char32_t c) {
    for (const SymbolName& s : SYMBOLS) {
        if (s.symbol == c) return s.name;
    }
    return nullptr;
}

const char* spelled_name(const std::string& character) {
    for (const SpelledName& s : SPELLED) {
        if (character == s.character) return s.name;
    }
    return nullptr;
}

bool split_compound_numeral(const std::u32string& w, std::u32string& ones, std::u32string& tens) {
    return split_compound_numeral_impl(w, ones, tens);
}

bool is_proclitic(const std::u32string& w) { return proclitics().count(w) != 0; }
bool is_enclitic(const std::u32string& w) { return enclitics().count(w) != 0; }
bool is_function_word(const std::u32string& w) { return function_words().count(w) != 0; }
bool is_question_word(const std::u32string& w) { return question_words().count(w) != 0; }
const std::unordered_set<std::u32string>& question_word_set() { return question_words(); }

// =============================================================================
// Lexicon
// =============================================================================

namespace {
bool find_in_table(const Lexicon::Table& table, const std::u32string& key, std::string& entry);
}

bool Lexicon::parse(const std::u32string& marked, std::u32string& key, std::u32string& pron,
                    bool& stem, std::string* error) {
    key.clear();
    pron.clear();
    stem = false;
    if (marked.empty()) { if (error) *error = "empty entry"; return false; }
    std::u32string text = marked;
    if (!text.empty() && text.back() == U'*') {
        stem = true;
        text.pop_back();
    }
    size_t eq = text.find(U'=');
    std::u32string given_key;
    if (eq != std::u32string::npos) {
        // The spelling may carry the dictionary's accents too (rékel=rékəw)
        std::u32string spelling;
        for (char32_t c : text.substr(0, eq)) {
            char32_t plain;
            uint8_t flags;
            if (c == U' ' || c == COMBINING_GRAVE || c == COMBINING_ACUTE ||
                c == COMBINING_CIRCUMFLEX || c == COMBINING_MACRON) continue;
            if (!notation_letter(c, plain, flags) || !is_base_letter(plain)) {
                if (error) *error = "the spelling has a letter that is not one";
                return false;
            }
            spelling.push_back(plain);
        }
        given_key = spelling;
        text = text.substr(eq + 1);
    }
    std::u32string plain;
    std::vector<uint8_t> flags;
    if (text.empty()) { if (error) *error = "empty pronunciation"; return false; }
    for (char32_t c : text) {
        if (c == U' ') { if (error) *error = "space in an entry"; return false; }
    }
    if (!read_notation(text, plain, flags, error)) return false;
    // A stem needs no stress (its ending may carry it in a form list); a
    // whole word does, unless it has no vowel at all.
    bool stressed = false, vowel = false;
    for (size_t i = 0; i < plain.size(); ++i) {
        if (flags[i] & F_STRESS) stressed = true;
        if (is_vowel_letter(plain[i]) || plain[i] == U'r') vowel = true;
    }
    if (!stem && vowel && !stressed) {
        if (error) *error = "no stressed vowel";
        return false;
    }
    pron = text;
    key = given_key.empty() ? plain : given_key;
    return true;
}

bool Lexicon::add(const std::u32string& marked) {
    // A form list: prefix|ending|ending; marks may sit in either part
    size_t bar = marked.find(U'|');
    if (bar == std::u32string::npos) {
        std::u32string key, pron;
        bool stem = false;
        if (!parse(marked, key, pron, stem, nullptr)) return false;
        (stem ? m_stems : m_exact)[key] = pron;
        return true;
    }
    const std::u32string prefix = marked.substr(0, bar);
    bool ok = true;
    while (bar != std::u32string::npos) {
        size_t next = marked.find(U'|', bar + 1);
        size_t length = (next == std::u32string::npos ? marked.size() : next) - bar - 1;
        std::u32string form = prefix + marked.substr(bar + 1, length);
        std::u32string key, pron;
        bool stem = false;
        if (parse(form, key, pron, stem, nullptr)) {
            (stem ? m_stems : m_exact)[key] = pron;
        } else {
            ok = false;
        }
        bar = next;
    }
    return ok;
}

void Lexicon::clear() {
    m_exact.clear();
    m_stems.clear();
}

bool Lexicon::find_exact(const std::u32string& word, std::u32string& pron) const {
    auto it = m_exact.find(word);
    if (it != m_exact.end()) {
        pron = it->second;
        return true;
    }
    std::string entry;
    if (find_in_table(m_form_table, word, entry)) {
        pron = PhonemeMapper::utf8_to_utf32(entry);
        return true;
    }
    return false;
}

bool Lexicon::find_stem(const std::u32string& word, size_t& stem_length, std::u32string& pron,
                        bool (*accept)(const std::u32string& ending)) const {
    if (m_stems.empty() && !m_stem_table.count) return false;
    const size_t min_len = word.size() > MAX_ENDING ? word.size() - MAX_ENDING : 1;
    std::string entry;
    for (size_t len = word.size(); len >= min_len && len >= 2; --len) {
        if (accept && !accept(word.substr(len))) continue;
        const std::u32string stem = word.substr(0, len);
        auto it = m_stems.find(stem);
        if (it != m_stems.end()) {
            stem_length = len;
            pron = it->second;
            return true;
        }
        if (find_in_table(m_stem_table, stem, entry)) {
            stem_length = len;
            pron = PhonemeMapper::utf8_to_utf32(entry);
            return true;
        }
    }
    return false;
}

namespace {

struct VerbNoun {
    const char* verb;           // the verb's reading
    const char* nominal;        // the noun's, adjective's or adverb's
    bool participle;            // the verb's reading is the l-participle
    int noun_percent;           // the share of a genitive plural in -l, or -1 (none)
};

#include "formant_slovenian.inc"

// The generated tables are sorted by the spelling their entries are for
// and searched in place: the spelling of an entry is read off the UTF-8
// notation while comparing, so no copy of the table is built at start.
// Returns <0, 0, >0 as the key compares with the entry's spelling.
int compare_key(const std::u32string& key, const char* entry) {
    size_t i = 0;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(entry);
    while (*p) {
        // Decode one UTF-8 code point
        char32_t c;
        if (*p < 0x80) { c = *p++; }
        else if ((*p & 0xE0) == 0xC0) { c = ((*p & 0x1F) << 6) | (p[1] & 0x3F); p += 2; }
        else if ((*p & 0xF0) == 0xE0) { c = ((*p & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); p += 3; }
        else { c = ((*p & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F); p += 4; }
        if (c == U'*' || c == COMBINING_GRAVE || c == U'\'' || c == U':') continue;
        char32_t plain;
        uint8_t flags;
        if (!notation_letter(c, plain, flags)) plain = c;
        if (i >= key.size()) return -1;             // the entry is longer
        if (key[i] != plain) return key[i] < plain ? -1 : 1;
        ++i;
    }
    return i == key.size() ? 0 : 1;                 // the key is longer
}

// Reads the entry of a front-coded block that starts at p into `entry`,
// which holds the entry before it: a leading byte 1 + n keeps its first n
// bytes, and the bytes that follow are appended. Returns where the next
// entry starts: a byte of 1 to 64 opens one (the notation's bytes are 0x61
// and above), 0 ends the block.
const char* read_entry(const char* p, std::string& entry) {
    const unsigned char* u = reinterpret_cast<const unsigned char*>(p);
    if (*u >= 1 && *u <= 64) {
        entry.resize(*u - 1u);
        ++u;
    } else {
        entry.clear();
    }
    while (*u > 64) entry.push_back(static_cast<char>(*u++));
    return reinterpret_cast<const char*>(u);
}

bool find_in_table(const Lexicon::Table& table, const std::u32string& key, std::string& entry) {
    if (!table.count) return false;
    // The last block whose first entry is not after the key
    const size_t blocks = (table.count + table.block - 1) / table.block;
    size_t lo = 0, hi = blocks;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        read_entry(table.blocks[mid], entry);
        if (compare_key(key, entry.c_str()) < 0) hi = mid;
        else lo = mid + 1;
    }
    if (lo == 0) return false;
    const char* p = table.blocks[lo - 1];
    while (*p) {
        p = read_entry(p, entry);
        int c = compare_key(key, entry.c_str());
        if (c == 0) return true;
        if (c < 0) return false;
    }
    return false;
}

} // namespace

const Lexicon& builtin_lexicon() {
    static const Lexicon lexicon = [] {
        Lexicon lex;
        Lexicon::Table forms, stems;
        forms.blocks = SLOVENIAN_FORMS;
        forms.count = SLOVENIAN_FORM_COUNT;
        forms.block = SLOVENIAN_BLOCK;
        stems.blocks = SLOVENIAN_STEMS;
        stems.count = SLOVENIAN_STEM_COUNT;
        stems.block = SLOVENIAN_BLOCK;
        lex.set_tables(forms, stems);
        for (const char* entry : HAND_ENTRIES) {
            lex.add(PhonemeMapper::utf8_to_utf32(entry));
        }
        return lex;
    }();
    return lexicon;
}

// A lexicon of one generated table of whole forms (the names, the commands)
static Lexicon table_of(const char* const* blocks, size_t count) {
    Lexicon lex;
    Lexicon::Table forms;
    forms.blocks = blocks;
    forms.count = count;
    forms.block = SLOVENIAN_BLOCK;
    lex.set_tables(forms, Lexicon::Table());
    return lex;
}

const char32_t* head_reading(const std::u32string& w, bool* speech) {
    if (speech) *speech = false;
    for (const HeadReading& r : HEAD_READINGS) {
        if (w == r.spelling) {
            if (speech) *speech = r.speech;
            return r.head;
        }
    }
    // The commands of software (slovenian_commands.tsv): the imperatives of
    // Sloleks where they retract the stress of the present (SP 2001 gradíti
    // grádi, namestíti namésti, vršíti vŕši; a listener's report of
    // 2026-11-03: Posodobi, Nadgradi, Namesti, Izvrši)
    static const Lexicon commands = table_of(SLOVENIAN_HEAD_IMPERATIVES, SLOVENIAN_HEAD_IMPERATIVES_COUNT);
    static thread_local std::u32string pron;
    return commands.find_exact(w, pron) ? pron.c_str() : nullptr;
}

bool name_reading(const std::u32string& w, bool clause_head, std::u32string& pron) {
    static const Lexicon names = table_of(SLOVENIAN_NAMES, SLOVENIAN_NAMES_COUNT);
    static const Lexicon inside = table_of(SLOVENIAN_NAMES_INSIDE, SLOVENIAN_NAMES_INSIDE_COUNT);
    if (names.find_exact(w, pron)) return true;
    return !clause_head && inside.find_exact(w, pron);
}

bool verb_noun_readings(const std::u32string& w, VerbNounReadings& out) {
    size_t lo = 0, hi = sizeof(SLOVENIAN_VERB_NOUN) / sizeof(SLOVENIAN_VERB_NOUN[0]);
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        int c = compare_key(w, SLOVENIAN_VERB_NOUN[mid].verb);
        if (c == 0) {
            out.verb = PhonemeMapper::utf8_to_utf32(SLOVENIAN_VERB_NOUN[mid].verb);
            out.nominal = PhonemeMapper::utf8_to_utf32(SLOVENIAN_VERB_NOUN[mid].nominal);
            out.participle = SLOVENIAN_VERB_NOUN[mid].participle;
            out.noun_percent = SLOVENIAN_VERB_NOUN[mid].noun_percent;
            return true;
        }
        if (c < 0) hi = mid;
        else lo = mid + 1;
    }
    return false;
}

// The readings were chosen on the tagged corpus SUK 1.1 (CLARIN.SI,
// 11356/1959; 6,863 tokens of these spellings): the more frequent reading
// alone is right for 93.3 %, these cues for 94.8 %. Cues that look
// plausible did worse than nothing over all the spellings and were left
// out: a clause-final word is as often a noun (pred leti, čez dve leti) as
// a verb, se earlier in the clause belongs to another verb as often as to
// this one, and a determiner or a copula before it says little.
namespace {

// The prepositions that decide a reading after them.
const WordSet& prepositions() {
    static const WordSet set = {
        U"v", U"na", U"z", U"s", U"k", U"h", U"o", U"pri", U"po", U"od", U"do", U"iz",
        U"za", U"pred", U"nad", U"pod", U"med", U"brez", U"ob", U"proti", U"skozi",
        U"čez", U"zaradi", U"okoli", U"okrog", U"blizu", U"glede", U"poleg", U"razen",
        U"sredi", U"mimo", U"preko", U"prek", U"kljub", U"zoper", U"izza", U"izpod",
        U"namesto", U"vzdolž", U"znotraj", U"zunaj", U"nasproti", U"izmed",
    };
    return set;
}

} // namespace

// meni is three words: mení the menu (SSKJ² "mení tudi menu [mení]", SP
// 2001 mení -ja), mêni the dative and locative of jàz (SSKJ², SP: mêne,
// mêni), méni the present of méniti "think" (SP: meníti in méniti -im
// tudi méniti -im). Alone in its clause it is the name of a menu, a
// button or a screen ("Meni", "Meni, gumb"); in a sentence it is one of
// the other two. In SUK 1.1 the verb is 200 of 242 occurrences and the
// pronoun 37: the pronoun is every one after a preposition (k meni, v
// meni, o meni: 17 of 17) and before se or je (meni se zdi, meni je
// všeč); the lexicon's méni stays elsewhere (minister meni, kaj meni).
//
// lepo is lepó at the end of its clause and lépo before another word, as
// a listener asked on 2026-10-30 (Hvala lepo. To je lepo. Lepo vreme.
// Lepo pozdravljeni.), and so are the other spellings Sloleks reads as an
// adverb on -ó and an adjective on its stem (težkó/têžko, slabó/slábo:
// the listener said yes to them the same day). The dictionaries have -ó
// for the adverb and the predicative (SP 2001: lepó nač. prisl., lepó
// povdk.) and the stem for the adjective (SP: lép -a -o tudi -ó), so
// before a word the adverb (lepo pozdravljeni, močno dežuje, težko je)
// follows the listener, not them: in SUK 1.1 that is 361 of the 579
// occurrences of these spellings, and the adjective before a word, which
// the lexicon read as the adverb (težko delo, slabo vreme), 158. Not
// lahko: the modal lahkó "can" stands before its verb (Lahko pridem).
// Not nelepo: Sloleks has no reading of its adverb. The readings are
// Sloleks's (the adverb's -ó, the adjective's first stem variant).
namespace {

struct EndReading {
    const char32_t* spelling;
    const char32_t* at_end;         // the adverb
    const char32_t* before_word;    // the adjective
};
const EndReading END_READINGS[] = {
    {U"lepo", U"lepó", U"lépo"}, {U"težko", U"težkó", U"têžko"},
    {U"močno", U"močnó", U"môčno"}, {U"slabo", U"slabó", U"slábo"},
    {U"hudo", U"hudó", U"húdo"}, {U"tesno", U"tesnó", U"têsno"},
    {U"ostro", U"ostró", U"óstro"}, {U"trdo", U"trdó", U"tŕdo"},
    {U"toplo", U"topló", U"tôplo"}, {U"grdo", U"grdó", U"gŕdo"},
    {U"grenko", U"grenkó", U"grênko"}, {U"tenko", U"tenkó", U"tènko"},
    {U"premočno", U"premočnó", U"premôčno"}, {U"preslabo", U"preslabó", U"preslábo"},
    {U"predrago", U"predragó", U"predrágo"}, {U"prelepo", U"prelepó", U"prelépo"},
    {U"pretežko", U"pretežkó", U"pretêžko"}, {U"prehudo", U"prehudó", U"prehúdo"},
    {U"prelahko", U"prelahkó", U"preláhko"}, {U"pretesno", U"pretesnó", U"pretêsno"},
    {U"pretoplo", U"pretopló", U"pretôplo"}, {U"premrzlo", U"premrzló", U"premŕzlo"},
    {U"pretrdo", U"pretrdó", U"pretŕdo"}, {U"preostro", U"preostró", U"preóstro"},
    {U"pretenko", U"pretenkó", U"pretènko"}, {U"pregrdo", U"pregrdó", U"pregŕdo"},
};

} // namespace

namespace {

bool word_ends_with(const std::u32string& s, const char32_t* tail) {
    const std::u32string t(tail);
    return s.size() >= t.size() && s.compare(s.size() - t.size(), t.size(), t) == 0;
}

const WordSet& short_pronouns() {
    static const WordSet set = {U"mi", U"ti", U"mu", U"ji", U"nam", U"vam", U"jim", U"me", U"te", U"ga",
                                U"jo", U"nas", U"vas", U"jih", U"se", U"si", U"ju", U"naju", U"vaju"};
    return set;
}

// A verb in the first or second person, by its ending (poznam, vidiš, kupite)
bool addressed_verb(const std::u32string& w) {
    static const WordSet not_verbs = {
        U"potem", U"zatem", U"nekam", U"drugam", U"nikamor", U"vsem", U"njim", U"njem", U"mojim", U"tvojim",
        U"svojim", U"našim", U"vašim", U"mojem", U"tvojem", U"svojem", U"našem", U"vašem",
    };
    if (w.size() < 4 || not_verbs.count(w)) return false;
    for (const char32_t* tail : {U"im", U"am", U"em", U"iš", U"aš", U"eš", U"imo", U"amo", U"emo", U"ite",
                                 U"ate", U"ete"}) {
        if (word_ends_with(w, tail)) return true;
    }
    return false;
}

// lahko is the modal and the adverb, lahkó (SP 2001), in 2,703 of its 2,718
// occurrences in SUK 1.1, and láhko, the adjective lahek (SP "láhek -hka -o";
// a Slovenian listener's Jesti moraš lahko hrano, Prosim daj mi lahko nalogo,
// On nosi lahko torbo) in the others, so the adjective needs evidence. A noun
// right after it does not decide by itself: the modal stands before its object
// in 63 of those (Lahko kuhinjo prepleskamo, Preboliš lahko bolezen), and a
// present of a perfective verb, which has no other way to be a present, comes
// with the modal after it (Izberete lahko nalogo, Kupi lahko hrano). What does:
// lahko noč (SSKJ² "kot pozdrav"); a preposition before it (z lahko nogo, na
// lahko nafto; SP na láhko tudi na lahkó) unless an adverb is modifying an
// adjective in -ljiv (na lahko obranljivem); a verb of obligation, which the
// modal never follows, before a word that is no infinitive (moraš lahko hrano,
// not mora lahko dostopati); and a noun things are called light by (SSKJ²
// under láhek, the Collocations Dictionary of Modern Slovene at CJVT, the
// corpus's lahko nafto), alone or after an adjective of its own (lahko poletno
// obleko, lahko in hladno obleko), ending the clause or before a preposition or
// a conjunction (a verb after it would make lahko the modal), with a verb before
// lahko that carries such things in the present (nosim, pijem, kuham), or none
// that would make it the modal. A question that opens with lahko and such a noun
// asks whether something can be done, a statement says something of the light
// thing, as a native listener answered in November 2026 (Lahko delo opravi vsak?
// lahkó, Lahko delo opravi vsak. láhko); the only such sentence of SUK 1.1 is a
// question. A verb in another person than the third has a subject of its own,
// and is the modal in a statement too (Lahko hrano kupite v trgovini).
bool lahko_is_adjective(const std::vector<std::u32string>& words, const std::vector<bool>& capitalized,
                        size_t i, bool question) {
    static const WordSet nouns = {
        U"aktovko", U"artilerijo", U"atletiko", U"baterijo", U"besedilo", U"blago", U"bluzo", U"bolezen",
        U"branje", U"cev", U"copato", U"delo", U"desko", U"dieto", U"dihanje", U"dostopnost", U"figuro",
        U"folijo", U"gimnastiko", U"glasbo", U"gorivo", U"gripo", U"haljo", U"harmoniko", U"hojo", U"hrano",
        U"hruško", U"igro", U"industrijo", U"izbiro", U"jakno", U"jed", U"jedilo", U"jopico", U"jopo", U"juho",
        U"kamero", U"kapico", U"kapo", U"kategorijo", U"kitaro", U"klobaso", U"knjigo", U"kolesce",
        U"kolo", U"komedijo", U"konfekcijo", U"konstrukcijo", U"kosilo", U"kost", U"kovino", U"kremo", U"krilce",
        U"krilo", U"križarko", U"krmo", U"kroglo", U"kuhinjo", U"letalo", U"letev", U"letvo", U"literaturo",
        U"litino", U"majico", U"malico", U"nafto", U"nalogo", U"nesrečo", U"nogavico", U"nogo", U"oblačilo",
        U"obleko", U"obliko", U"obutev", U"obuvalo", U"odejo", U"odločitev", U"ogrodje", U"olje", U"omako",
        U"opeko", U"operacijo", U"opravilo", U"opremo", U"orodje", U"orožje", U"palico", U"pasto", U"pecivo",
        U"peno", U"perilo", U"pero", U"pijačo", U"pito", U"pivo", U"pižamo", U"platišče", U"platno", U"ploščo",
        U"podvozje", U"pohištvo", U"pohodnico", U"pokrivalo", U"posodo", U"pot", U"poškodbo", U"prehrano",
        U"preizkušnjo", U"preskušnjo", U"prevleko", U"prst", U"puško", U"raketo", U"rekreacijo", U"rezino",
        U"rokavico", U"roko", U"salamo", U"sandalo", U"sladico", U"slovo", U"službo", U"smrt", U"snov", U"solato",
        U"spanje", U"srajco", U"srce", U"steklo", U"strojnico", U"suknjo", U"tarčo", U"tekmo", U"tekočino",
        U"telesce", U"telo", U"telovadbo", U"tipkovnico", U"tiskalnico", U"tkanino", U"tkivo", U"torbo", U"torto",
        U"tuniko", U"turo", U"usnje", U"vajo", U"vetrovko", U"večerjo", U"vino", U"vozilo", U"vožnjo",
        U"vprašanje", U"vrečko", U"vrečo", U"vzmet", U"zaveso", U"zaščito", U"zemljo", U"zlitino", U"zmago",
        U"čelado", U"čokolado", U"čtivo", U"škatlo", U"železo", U"žganje", U"življenje", U"žogo",
    };
    static const WordSet obligation = {
        U"mora", U"moram", U"moraš", U"morava", U"morata", U"moramo", U"morate", U"morajo",
        U"moral", U"morala", U"moralo", U"morali", U"morale", U"sme", U"smem", U"smeš", U"smemo",
        U"smete", U"smejo", U"hoče", U"hočem", U"hočeš", U"hočemo", U"hočete", U"hočejo",
    };
    static const WordSet modal_before = {
        U"bi", U"da", U"če", U"ki", U"ali", U"a", U"kako", U"kje", U"kaj", U"kdaj", U"zakaj", U"kdo",
        U"bo", U"bom", U"boš", U"bova", U"bosta", U"bomo", U"boste", U"bodo", U"sem", U"so", U"sta",
        U"smo", U"ste", U"vsak", U"vsakdo", U"vsi", U"nihče",
    };
    // Verbs that carry, eat, wear or look for such a thing in the present
    static const WordSet carrying = {
        U"nosim", U"nosiš", U"nosi", U"nosiva", U"nosita", U"nosimo", U"nosijo",
        U"jem", U"ješ", U"jemo", U"jedo", U"jejo",
        U"pijem", U"piješ", U"pije", U"pijemo", U"pijejo",
        U"oblačim", U"oblačiš", U"oblači", U"oblačimo", U"oblačijo",
        U"kuham", U"kuhaš", U"kuha", U"kuhamo", U"kuhajo",
        U"pripravljam", U"pripravljaš", U"pripravlja", U"pripravljamo", U"pripravljajo",
        U"iščem", U"iščeš", U"išče", U"iščemo", U"iščejo",
        U"maram", U"maraš", U"mara", U"maramo", U"marajo",
        U"ljubim", U"ljubiš", U"ljubi", U"ljubimo", U"ljubijo",
        U"želim", U"želiš", U"želi", U"želimo", U"želijo",
        U"potrebujem", U"potrebuješ", U"potrebuje", U"potrebujemo", U"potrebujejo",
        U"rabim", U"rabiš", U"rabi", U"rabimo", U"rabijo",
        U"kupujem", U"kupuješ", U"kupuje", U"kupujemo", U"kupujejo",
        U"prodajam", U"prodajaš", U"prodaja", U"prodajamo", U"prodajajo",
        U"opravljam", U"opravljaš", U"opravlja", U"opravljamo", U"opravljajo",
        U"prenašam", U"prenašaš", U"prenaša", U"prenašamo", U"prenašajo",
        U"držim", U"držiš", U"drži", U"držimo", U"držijo",
        U"vlečem", U"vlečeš", U"vleče", U"vlečemo", U"vlečejo",
        U"uživam", U"uživaš", U"uživa", U"uživamo", U"uživajo",
        U"berem", U"bereš", U"bere", U"beremo", U"berejo",
        U"poslušam", U"poslušaš", U"posluša", U"poslušamo", U"poslušajo",
        U"igram", U"igraš", U"igra", U"igramo", U"igrajo",
        U"izdelujem", U"izdeluješ", U"izdeluje", U"izdelujemo", U"izdelujejo",
        U"proizvajam", U"proizvajaš", U"proizvaja", U"proizvajamo", U"proizvajajo",
        U"dobivam", U"dobivaš", U"dobiva", U"dobivamo", U"dobivajo",
        U"imam", U"imaš", U"ima", U"imava", U"imata", U"imamo", U"imate", U"imajo",
    };
    // Perfective presents: with lahko after them they are its verb
    static const WordSet perfective = {
        U"izbere", U"dobi", U"kupi", U"najde", U"vzame", U"uporabi", U"naroči", U"opravi", U"prebere", U"vidi",
        U"odda", U"pridobi", U"spremeni", U"prenese", U"pošlje", U"ponudi", U"pripravi", U"skuha", U"sestavi",
        U"zamenja", U"doda", U"prevzame", U"preboli", U"obleče", U"sname", U"poje", U"popije", U"ustvari",
    };
    // Adverbs an adjective's -o is spelled like, which modify the modal's verb
    static const WordSet not_adjectives = {
        U"tako", U"nekako", U"zato", U"samo", U"kako", U"zelo", U"malo", U"veliko", U"hitro", U"lepo", U"dobro",
        U"slabo", U"vedno", U"skoraj", U"celo", U"nato", U"tudi", U"prej", U"ravno", U"kmalu", U"mirno", U"pravo",
        U"neko", U"eno", U"vsako", U"katero", U"kakšno", U"takšno", U"tako", U"tisto", U"tole", U"ono", U"svojo",
        U"mojo", U"tvojo", U"našo", U"vašo", U"njeno", U"drugo", U"isto", U"marsikatero", U"nekako",
    };
    static const WordSet joining = {U"in", U"ter", U"ali", U"pa", U"a", U"ampak", U"oziroma"};
    // A degree goes with the adjective, not the modal (zelo lahko torbo imam)
    static const WordSet degrees = {U"zelo", U"precej", U"dokaj", U"razmeroma", U"sorazmerno", U"dovolj",
                                    U"izredno", U"nadvse", U"preveč", U"res", U"čisto", U"povsem"};

    const size_t n = words.size();
    const std::u32string* prev = i > 0 ? &words[i - 1] : nullptr;
    // na lahko, the adverbial phrase, ends a clause too (jemati službo na lahko)
    if (prev && *prev == U"na" && !(capitalized[i - 1] && i > 1)) {
        if (i + 1 == n) return true;
    }
    if (i + 1 == n) return false;
    const std::u32string& next = words[i + 1];
    // lahko prebavljiva hrana, the adverb; but z lahko prebavljivostjo
    if (next.find(U"ljiv") != std::u32string::npos && next.find(U"ljivost") == std::u32string::npos) return false;
    if (next == U"noč") return true;
    // Z, S with a capital inside a clause are letters (intervalna opredelitev
    // Z lahko povzroča, model S lahko ponudi)
    if (prev && prepositions().count(*prev) && !(capitalized[i - 1] && i > 1)) return true;
    size_t k = i;
    while (k > 0 && short_pronouns().count(words[k - 1])) --k;
    const std::u32string* before = k > 0 ? &words[k - 1] : nullptr;
    if (before && obligation.count(*before) && !word_ends_with(next, U"ti") && !word_ends_with(next, U"či")) {
        return true;
    }

    // The noun: right after it, after an adjective, or after a coordinated one
    size_t noun = i + 1;
    if (!nouns.count(next)) {
        auto adjective_o = [&](size_t a) {
            return a < n && words[a].size() >= 3 && words[a].back() == U'o' && !not_adjectives.count(words[a]);
        };
        if (adjective_o(i + 1) && i + 2 < n && nouns.count(words[i + 2])) {
            noun = i + 2;
        } else if (next == U"in" && adjective_o(i + 2) && i + 3 < n && nouns.count(words[i + 3])) {
            noun = i + 3;
        } else {
            return false;
        }
    }
    if (before && degrees.count(*before)) return true;
    const bool at_head = !before && k == i;     // Lahko delo, not Se lahko delo
    // What follows it: a verb there makes lahko the modal, but not a copula,
    // whose subject the noun is (Lahko vino je najboljše)
    bool copula = false;
    if (noun + 1 < n) {
        const std::u32string& after = words[noun + 1];
        copula = k == i && (after == U"je" || after == U"ni" || after == U"bo");
        if (!prepositions().count(after) && !joining.count(after) && !copula) {
            // A sentence that opens with it, not a question: Lahko delo opravi
            // vsak. Lahko torbo nosi. But Lahko hrano kupite, Lahko hrano kupijo.
            if (!at_head || !capitalized[i] || question) return false;
            if (word_ends_with(after, U"ajo") || word_ends_with(after, U"ejo") || word_ends_with(after, U"ijo")) {
                return false;
            }
            for (size_t m = noun + 1; m < n; ++m) {
                if (addressed_verb(words[m])) return false;
            }
            return true;
        }
    }
    if (!before) return at_head && (!question || copula);   // Lahko delo. Lahko delo?
    if (modal_before.count(*before)) return false;
    if (carrying.count(*before)) return true;
    // An imperative in -jte (Dajte mi lahko nalogo)
    if (word_ends_with(*before, U"jte")) return true;
    if (perfective.count(*before)) return false;
    // A verb in the present before it (Kupim lahko hrano, Vidite lahko delo)
    for (const char32_t* tail : {U"m", U"š", U"mo", U"te", U"ta", U"va", U"jo"}) {
        if (word_ends_with(*before, tail)) return false;
    }
    return true;
}

// roka is the hand's nominative, rôka, or the genitive of rók "deadline",
// róka (SP 2001 rók -a), or the genitive and the accusative of the name Rok,
// Róka (SP Rók -a). In SUK 1.1 the 28 genitives have a preposition (do roka,
// brez roka), a genitive in -ega (vojaškega roka) or a noun of time limits
// before them; the 16 hands have none of these, and a hand after a preposition
// would be roke or roko.
bool roka_is_deadline(const std::vector<std::u32string>& words, size_t i) {
    if (i == 0) return false;
    const std::u32string& prev = words[i - 1];
    static const WordSet limits = {
        U"podaljšanje", U"podaljšanja", U"podaljšanju", U"podaljšanjem", U"iztek", U"izteka", U"izteku",
        U"iztekom", U"potek", U"poteka", U"poteku", U"potekom", U"služenje", U"služenja", U"služenju",
        U"služenjem", U"določitev", U"določitve", U"predpisuje", U"upoštevanje", U"upoštevanja",
        U"spoštovanje", U"spoštovanja", U"zamuda", U"zamude", U"zamudo", U"zamik", U"zamika", U"zamiku",
        U"prestavitev", U"prestavitve", U"podaljševanje", U"podaljševanja", U"prekoračitev", U"prekoračitve",
        U"odsluženje", U"odsluženja", U"pretek", U"preteka", U"preteku", U"štetje", U"štetja", U"zaključek",
        U"zaključka", U"dolžina", U"dolžine", U"sprememba", U"spremembe", U"konec", U"konca", U"začetek",
        U"začetka",
    };
    return prepositions().count(prev) != 0 || word_ends_with(prev, U"ega") || limits.count(prev) != 0;
}

// The first words of a clause that are no hand: a noun after a negated verb of
// being is in the genitive, and that genitive can only be the name's or the
// deadline's (Roka že ni, Roka ni več, Roka ni bilo na sestanku, Roka ne bo; a
// listener's examples, November 2026), but a hand is in the nominative with an
// adjective or a participle (Roka ni zlomljena, Roka ni več bolna, Roka ne bo
// bolela). A verb in the first or second person or in the plural has another
// subject, so roka is its object (Roka nisem videl, Roka poznam, Roka so
// našli), and so it is before a surname (Roka Žarnića).
bool roka_is_absent_or_object(const std::vector<std::u32string>& words, const std::vector<bool>& capitalized,
                              size_t i) {
    static const WordSet openers = {U"in", U"pa", U"ampak", U"a", U"toda", U"ker", U"da", U"če", U"ko",
                                    U"zakaj", U"kje", U"ali", U"še", U"tudi", U"vendar"};
    static const WordSet adverbs = {
        U"že", U"še", U"tudi", U"pa", U"sploh", U"danes", U"včeraj", U"jutri", U"zdaj", U"sedaj", U"trenutno",
        U"nikjer", U"nikoli", U"nikdar", U"več", U"tu", U"tukaj", U"tam", U"doma", U"notri", U"zunaj", U"tja",
        U"dolgo", U"spet", U"vedno", U"res", U"žal", U"sedaj", U"več",
    };
    static const WordSet places = {
        U"službi", U"šoli", U"pisarni", U"sestanku", U"delu", U"poti", U"treningu", U"tekmi", U"vaji",
        U"predavanju", U"zabavi", U"nas", U"vas", U"njih", U"meni", U"tebi",
    };
    static const WordSet other_subject = {
        U"sem", U"sva", U"sta", U"smo", U"ste", U"so", U"nisem", U"nisi", U"nisva", U"nista", U"nismo", U"niste",
        U"niso", U"bom", U"boš", U"bova", U"bosta", U"bomo", U"boste", U"bodo",
    };
    static const WordSet imperatives = {
        U"pokliči", U"pokličite", U"vprašaj", U"vprašajte", U"počakaj", U"počakajte", U"pozdravi", U"pozdravite",
        U"poišči", U"poiščite", U"obvesti", U"obvestite", U"kliči", U"kličite",
    };
    if (i > 0 && !openers.count(words[i - 1])) return false;
    const size_t n = words.size();
    size_t k = i + 1;
    while (k < n && adverbs.count(words[k])) ++k;
    if (k >= n) return false;
    if (words[k] == U"ni") {
        ++k;
        if (k < n && words[k] == U"bilo") return true;
        while (k < n && adverbs.count(words[k])) ++k;
        if (k == n) return true;
        return k + 2 == n && (words[k] == U"v" || words[k] == U"na" || words[k] == U"pri") && places.count(words[k + 1]);
    }
    if (words[k] == U"ne") {
        ++k;
        if (k < n && words[k] == U"bo") {
            ++k;
            while (k < n && adverbs.count(words[k])) ++k;
            return k == n;
        }
        return k < n && addressed_verb(words[k]);
    }
    if (other_subject.count(words[k]) || imperatives.count(words[k]) || addressed_verb(words[k])) return true;
    return k == i + 1 && capitalized[i] && capitalized[k];
}

// roki and roke are the hand's (v roki, obe roki, dvigni roke) unless a word
// around them says they are deadlines, in the plural (SP rók -a; SUK 1.1 has
// 12 of them against 75 hands): an adjective that goes with deadlines (the
// Collocations Dictionary of Modern Slovene at CJVT: plačilni roki, izpitne
// roke), the instrumental (z roki, prijavnimi roki: a hand's is roko, rokama),
// the plural verb (da so roki, roki so kratki) or what a deadline is for (roki
// za prijavo, roki plačila).
const char32_t* roki_roke_reading(const std::vector<std::u32string>& words, size_t i) {
    static const WordSet adjectives_i = {
        U"plačilni", U"časovni", U"dobavni", U"izvedbeni", U"odpovedni", U"izpitni", U"prijavni", U"razpisni",
        U"pritožbeni", U"zastaralni", U"zakonski", U"zakoniti", U"pogodbeni", U"predvideni", U"dogovorjeni",
        U"določeni", U"predpisani", U"skrajni", U"prestopni", U"garancijski", U"setveni", U"vpisni", U"kratki",
        U"daljši", U"krajši", U"zimski", U"jesenski", U"spomladanski", U"novi", U"vsi", U"ti", U"taki", U"taksni",
        U"predvidljivi",
    };
    static const WordSet adjectives_e = {
        U"plačilne", U"časovne", U"dobavne", U"izvedbene", U"odpovedne", U"izpitne", U"prijavne", U"razpisne",
        U"pritožbene", U"zastaralne", U"zakonske", U"zakonite", U"pogodbene", U"predvidene", U"dogovorjene",
        U"predpisane", U"skrajne", U"prestopne", U"garancijske", U"setvene", U"vpisne", U"zimske", U"jesenske",
        U"spomladanske", U"predvidljive",
    };
    static const WordSet for_what = {
        U"plačila", U"dobave", U"trajanja", U"vračanja", U"izvedbe", U"oddaje", U"prijave", U"odplačila",
    };
    const std::u32string* prev = i > 0 ? &words[i - 1] : nullptr;
    const std::u32string* next = i + 1 < words.size() ? &words[i + 1] : nullptr;
    if (words[i] == U"roke") {
        return prev && adjectives_e.count(*prev) ? U"róke" : nullptr;
    }
    if (prev) {
        if (*prev == U"z" || *prev == U"s") return U"róki";
        if (prepositions().count(*prev) || *prev == U"dve" || *prev == U"obe") return nullptr;
        if (adjectives_i.count(*prev) || word_ends_with(*prev, U"nimi") || word_ends_with(*prev, U"skimi")) return U"róki";
        if (*prev == U"so" || *prev == U"bodo" || *prev == U"niso") return U"róki";
    }
    if (next && (*next == U"so" || *next == U"niso" || *next == U"bodo" || for_what.count(*next))) return U"róki";
    if (next && *next == U"za" && !prev) return U"róki";
    return nullptr;
}

// The endings of the masculine definite adjectives that hardly anything else has
bool is_adjective_ending(const std::u32string& w) {
    return (w.size() >= 5 && word_ends_with(w, U"ni")) || word_ends_with(w, U"ski") || word_ends_with(w, U"ški") ||
           word_ends_with(w, U"čki") || word_ends_with(w, U"nji") || word_ends_with(w, U"oči") ||
           word_ends_with(w, U"eči");
}

// The other masculine definite adjectives of Sloleks (the generated table)
bool is_definite_adjective(const std::u32string& w) {
    size_t lo = 0, hi = sizeof(SLOVENIAN_DEFINITE_ADJECTIVES) / sizeof(SLOVENIAN_DEFINITE_ADJECTIVES[0]);
    while (lo < hi) {
        const size_t mid = lo + (hi - lo) / 2;
        const int c = compare_key(w, SLOVENIAN_DEFINITE_ADJECTIVES[mid]);
        if (c == 0) return true;
        if (c < 0) hi = mid;
        else lo = mid + 1;
    }
    return false;
}

// meni after a preposition: the pronoun's form after those with the dative
// and the locative (k meni, proti meni, o meni, pri meni, ob meni, po meni,
// v meni, na meni), but after those with the accusative or the instrumental
// the pronoun is mene or mano (pod mano, za mano, skozi mene), so meni is
// the menu (pod meni, a Slovenian listener's example; SP 2001 mení -ja).
// After v and na it is the menu's accusative when something moves there
// (pojdi v meni, kliknite na meni, nazaj v meni); in SUK 1.1 the other 17
// are the pronoun. Before those the menu follows an adjective or a possessive
// (glavni meni, vaš meni, padajoči meni: the Collocations Dictionary of Modern
// Slovene at CJVT) or a verb that takes a menu (odpri meni), and comes before
// its name, itself with a capital (Meni Start) or after v or na (v meni
// Dokumenti), and inside a clause with a capital it is a name (Gumb Meni). A
// small meni before a capital is the verb of the news (meni Kučan, tako meni
// Križanič: 30 occurrences in SUK 1.1), and so is meni after a surname that
// looks like an adjective (Franco Frattini meni). The pronoun comes after a
// word that stresses it (tudi meni, samo meni, ti meni), after an auxiliary or a
// participle, since the verb's present cannot follow those (je meni rekel,
// ponudili so meni), and before what is said of whom (meni ne, meni je všeč,
// meni se zdi); but what opens, closes and is empty is the menu (Meni se
// odpre, Meni je prazen). An adjective before it, whatever its meaning (zaprti
// meni nedavno, a listener's example; odprti meni, najboljši meni), says the
// menu by itself: the verb's subject is not an adjective and a pronoun has no
// use for one. Those the ending does not show are in the table of Sloleks
// (is_definite_adjective()). Of the 237 tokens of the verb and the pronoun in
// SUK 1.1 none stands after one of these.
const char32_t* meni_reading(const std::vector<std::u32string>& words, const std::vector<bool>& capitalized,
                             size_t i) {
    static const WordSet accusative_only = {U"pod", U"nad", U"pred", U"med", U"za", U"čez", U"skozi"};
    static const WordSet towards = {
        U"pojdi", U"pojdite", U"pojdimo", U"pojdiva", U"pojdem", U"pojdeš", U"gre", U"greš", U"grem",
        U"gremo", U"greste", U"gredo", U"gresta", U"odidi", U"odidite", U"vstopi", U"vstopite",
        U"vstopimo", U"vstop", U"stopi", U"stopite", U"preidi", U"preidite", U"preide", U"prehod",
        U"vrni", U"vrnite", U"vrnimo", U"vrnitev", U"nazaj", U"skoči", U"skočite", U"premakni",
        U"premaknite", U"povleci", U"povlecite", U"dodaj", U"dodajte", U"dodajanje", U"pripni",
        U"pripnite", U"vstavi", U"vstavite", U"klikni", U"kliknite", U"klik", U"tapni", U"tapnite",
        U"pritisni", U"pritisnite", U"dotakni", U"dotaknite",
    };
    // What a noun's nominative or accusative is qualified by, without the
    // adjective's endings below
    static const WordSet qualifying = {
        U"vaš", U"tvoj", U"moj", U"naš", U"svoj", U"njihov", U"njegov", U"njen", U"novi", U"levi", U"hitri",
        U"stari", U"ta", U"tale", U"tisti", U"isti", U"ves", U"cel", U"celoten", U"poseben", U"dober", U"okusen",
        U"bogat", U"pester", U"kratek", U"velik", U"majhen", U"lep", U"nov", U"star", U"lahek", U"prvi", U"drugi",
        U"tretji", U"četrti", U"vsak", U"neki", U"nek", U"kakšen", U"cenejši", U"dražji", U"večji", U"manjši",
        U"skriti",      // the adjective's form is also the infinitive, which is ten times as frequent
    };
    // The state of a menu, right before or after it: Zaprt meni, Meni zaprt
    static const WordSet menu_states = {
        U"odprt", U"zaprt", U"skrit", U"prikazan", U"izbran", U"prazen", U"razširjen", U"strnjen",
        U"onemogočen", U"omogočen", U"aktiven", U"neaktiven", U"naložen",
    };
    // Nouns it is the name or the kind of (Gumb meni, Tipka meni)
    static const WordSet naming = {
        U"gumb", U"gumba", U"ikona", U"ikone", U"ikono", U"tipka", U"tipke", U"tipko", U"zavihek", U"zavihka",
        U"postavka", U"možnost", U"ukaz", U"element", U"vrstica", U"izbira",
    };
    static const WordSet takes_menu = {
        U"odpri", U"odprite", U"odpre", U"odprl", U"odprla", U"zapri", U"zaprite", U"zapre", U"zaprl", U"zaprla",
        U"prikaži", U"prikažite", U"pokaži", U"pokažite", U"izberi", U"izberite", U"izbral", U"izbrala",
        U"skrij", U"skrijte", U"poišči", U"poiščite", U"razširi", U"razširite", U"strni", U"strnite",
        U"aktiviraj", U"aktivirajte", U"uporabi", U"uporabite", U"klikni", U"kliknite", U"tapni", U"tapnite",
        U"sestavi", U"sestavite", U"sestavil", U"sestavila", U"pripravi", U"pripravite", U"pripravil",
        U"pripravila", U"ima", U"imajo", U"ponuja", U"ponujajo", U"uredi", U"uredite", U"nastavi", U"nastavite",
        U"spremeni", U"spremenite", U"prilagodi", U"prilagodite", U"preglej", U"preglejte", U"najdi", U"najdite",
        // The verbs the Collocations Dictionary lists with a menu as the object
        U"odpira", U"odpirajo", U"odpiraj", U"odpirajte", U"izbira", U"izbirajo", U"izbiraj", U"izbirajte",
        U"sestavlja", U"sestavljajo", U"sestavljaj", U"sestavljajte", U"uporablja", U"uporabljajo", U"uporabljaj",
        U"uporabljajte", U"upravlja", U"upravljajo", U"upravljaj", U"upravljajte", U"menjaj", U"menjajte",
        U"oblikuj", U"oblikujte", U"ustvari", U"ustvarite", U"ustvaril", U"ustvarila", U"izdelaj", U"izdelajte",
        U"izdelal", U"izdelala", U"vsebuje", U"vsebujejo", U"vključuje", U"vključujejo",
    };
    // A word before it that stresses the pronoun, or a verb with a dative
    static const WordSet before_pronoun = {
        U"tudi", U"samo", U"le", U"prav", U"celo", U"vsaj", U"predvsem", U"zlasti", U"kar", U"ti", U"tebi",
        U"njemu", U"njej", U"nam", U"vam", U"niti", U"sem", U"si", U"je", U"bi", U"smo", U"ste", U"so", U"sva",
        U"sta", U"bom", U"boš", U"bo", U"bomo", U"boste", U"bodo", U"bova", U"bosta", U"nisem", U"nisi", U"ni",
        U"nismo", U"niste", U"niso", U"daj", U"dajte", U"reci", U"recite", U"povej", U"povejte", U"pomagaj",
        U"pomagajte", U"piši", U"pišite", U"zaupaj", U"verjemi", U"oprosti", U"vrni", U"vrnite", U"prinesi",
        U"prinesite", U"pošlji", U"pošljite", U"hvala", U"gre", U"gredo", U"edino",
    };
    static const WordSet not_participles = {U"general", U"admiral", U"kardinal", U"portal", U"tribunal", U"kanal",
                                            U"kapital", U"šola", U"sila", U"skupščina"};
    // After it: said of whom. The verb's present does not take these.
    static const WordSet after_pronoun = {
        U"ne", U"že", U"nič", U"osebno", U"doma", U"všeč", U"znano", U"znana", U"znan", U"zdi", U"zdijo", U"ugaja",
        U"diši", U"paše", U"manjka", U"preostane", U"pripada", U"dovolj", U"mar", U"žal", U"dolgčas", U"vseeno",
        U"hladno", U"vroče", U"toplo", U"slabo", U"dobro", U"lepo", U"jasno", U"nerazumljivo", U"se", U"je", U"ni",
    };
    // A predicate anywhere after a pronoun at the head (Meni za to ni mar)
    static const WordSet dative_predicate = {U"mar", U"všeč", U"žal", U"zdi", U"zdijo", U"dolgčas", U"vseeno",
                                             U"ugaja", U"manjka", U"preostane"};
    // What a menu does, after meni se
    static const WordSet menu_verbs_se = {
        U"odpre", U"odpira", U"zapre", U"zapira", U"prikaže", U"prikazuje", U"pokaže", U"pokazuje", U"razširi",
        U"skrije", U"pojavi", U"naloži", U"nalaga", U"strne", U"spremeni", U"premakne", U"premika", U"osveži",
    };
    // What a menu is, after meni je, ni, bo
    static const WordSet menu_predicates = {
        U"odprt", U"zaprt", U"prazen", U"skrit", U"viden", U"prikazan", U"izbran", U"onemogočen", U"omogočen",
        U"aktiven", U"neaktiven", U"razširjen", U"strnjen", U"naložen", U"poln", U"dolg", U"pregleden",
        U"nepregleden", U"zapleten", U"sestavljen", U"dostopen", U"nedostopen",
    };
    // What a menu does, after it
    static const WordSet menu_verbs = {
        U"ima", U"vsebuje", U"omogoča", U"prikazuje", U"ponuja", U"vključuje", U"stane", U"sestavljajo", U"odpre",
        U"zapre", U"izgine",
    };
    // A genitive or a name of what the menu is of
    static const WordSet menu_of = {
        U"aplikacij", U"aplikacije", U"možnosti", U"nastavitev", U"orodij", U"orodja", U"jedi", U"dneva",
        U"restavracije", U"palete", U"telefona", U"gumba", U"datoteke", U"pomoči", U"povezav", U"gostilne",
        U"vrstica", U"vrstice",
    };
    static const WordSet menu_for = {
        U"kosilo", U"večerjo", U"zajtrk", U"otroke", U"goste", U"vegetarijance", U"krmarjenje", U"navigacijo",
        U"dostopnost", U"pomoč", U"izbiro", U"pokušino", U"cel", U"ves",
    };
    const size_t n = words.size();
    if (n == 1) return U"mení";
    const std::u32string* prev = i > 0 ? &words[i - 1] : nullptr;
    const std::u32string* next = i + 1 < n ? &words[i + 1] : nullptr;
    const bool before_capital = next && capitalized[i + 1];
    if (prev) {
        if (accusative_only.count(*prev)) return U"mení";
        if (*prev == U"v" || *prev == U"na") {
            if (before_capital) return U"mení";      // v meni Dokumenti
            for (size_t k = 0; k + 1 < i; ++k) {
                if (towards.count(words[k])) return U"mení";
            }
            return U"mêni";
        }
        if (prepositions().count(*prev)) return U"mêni";
        if (before_capital && capitalized[i]) return U"mení";   // To je Meni Start
        if (qualifying.count(*prev) || naming.count(*prev) || takes_menu.count(*prev) || menu_states.count(*prev)) {
            return U"mení";
        }
        const bool a_name = capitalized[i - 1] && i > 1;
        if (!a_name && (is_adjective_ending(*prev) || is_definite_adjective(*prev))) {
            return U"mení";     // glavni meni, stranski meni, zadnji meni, padajoči meni, zaprti meni
        }
        if (capitalized[i]) return U"mení";     // Gumb Meni
        if (before_pronoun.count(*prev)) return U"mêni";
        // ponudili meni, je rekla meni: a participle, not a name or a noun
        if (!capitalized[i - 1] && prev->size() >= 4 && !not_participles.count(*prev) &&
            (word_ends_with(*prev, U"li") || word_ends_with(*prev, U"le") || word_ends_with(*prev, U"la") ||
             word_ends_with(*prev, U"al") || word_ends_with(*prev, U"il") || word_ends_with(*prev, U"el"))) {
            return U"mêni";
        }
    } else if (before_capital && capitalized[i]) {
        return U"mení";     // Meni Start, Meni Datoteka
    }
    if (next) {
        if (menu_states.count(*next)) return U"mení";       // Meni zaprt
        if (*next == U"se" && i + 2 < n && menu_verbs_se.count(words[i + 2])) return U"mení";
        if ((*next == U"je" || *next == U"ni" || *next == U"bo") && i + 2 < n && menu_predicates.count(words[i + 2])) {
            return U"mení";
        }
        if (!prev) {
            if (menu_verbs.count(*next) || menu_of.count(*next)) return U"mení";
            if (*next == U"za" && i + 2 < n && menu_for.count(words[i + 2])) return U"mení";
            if (*next == U"pa") return U"mêni";
            for (size_t k = i + 1; k < n; ++k) {
                if (dative_predicate.count(words[k])) return U"mêni";
            }
        }
        if (after_pronoun.count(*next)) return U"mêni";
    }
    return nullptr;
}

} // namespace

const char32_t* clause_reading(const std::vector<std::u32string>& words, const std::vector<bool>& capitalized,
                               size_t i, bool before_complement, bool question, bool exclamation) {
    const std::u32string& w = words[i];
    // Veseli se! Veseli se novega dneva! The imperative vesêli (SP 2001
    // veselíti -ím, vesêli -íte) at the head of a clause before se or si in
    // a sentence that ends in an exclamation mark, the cue a listener named
    // (2026-11-03); without it the shape is the present veselí (Veseli se
    // novega dneva. "he looks forward to the new day"), and before an object
    // clitic the verb of pleasing (Veseli me, da ...), context_reading()
    if (w == U"veseli" && i == 0 && exclamation && words.size() > 1 && (words[1] == U"se" || words[1] == U"si")) {
        return U"vesêli";
    }
    if (!w.empty() && w.back() == U'o') {
        for (const EndReading& r : END_READINGS) {
            if (w == r.spelling) return i + 1 == words.size() ? r.at_end : r.before_word;
        }
        if (w == U"lahko" && lahko_is_adjective(words, capitalized, i, question)) return U"láhko";
        return nullptr;
    }
    if (w == U"meni") {
        const char32_t* reading = meni_reading(words, capitalized, i);
        // Meni, da je prav. Ta meni, da je prav. Tudi meni, da je prav. The
        // clause after it is what a verb of thinking takes; but a purpose
        // clause follows a menu (Odprite meni, da izberete), and a preposition
        // has the pronoun (k meni, da).
        if (before_complement && i + 1 == words.size() && (i == 0 || !prepositions().count(words[i - 1])) &&
            (!reading || i == 0 || (i == 1 && words[0] == U"ta") || std::u32string(reading) != U"mení")) {
            return U"méni";
        }
        return reading;
    }
    if (w == U"roka") {
        return roka_is_deadline(words, i) || roka_is_absent_or_object(words, capitalized, i) ? U"róka" : nullptr;
    }
    if (w == U"roki" || w == U"roke") return roki_roke_reading(words, i);
    if (w == U"afna") {
        const bool se = (i > 0 && words[i - 1] == U"se") || (i + 1 < words.size() && words[i + 1] == U"se");
        return se ? U"afná" : nullptr;
    }
    return nullptr;
}

namespace {

// The first word of a clause, in small letters
std::u32string first_word(const std::u32string& text) {
    std::u32string word;
    for (char32_t c : text) {
        if (c >= U'A' && c <= U'Z') c += 32;
        else if (c == 0x10C || c == 0x160 || c == 0x17D) c += 1;    // Č Š Ž
        if (!is_base_letter(c)) {
            if (!word.empty()) break;
            continue;
        }
        word.push_back(c);
    }
    return word;
}

}  // namespace

bool opens_complement_clause(const std::u32string& text) {
    static const WordSet openers = {U"da", U"naj", U"kdaj", U"kako", U"kje", U"zakaj", U"ali", U"kaj",
                                    U"kdo", U"koliko", U"kam", U"če"};
    return openers.count(first_word(text)) != 0;
}

bool is_address(const std::u32string& text) {
    // One word of letters, capitalized and not all in capitals, with the
    // engine's marks of the clause after it
    bool upper = false, lower = false;
    size_t words = 0;
    bool in_word = false;
    for (char32_t c : text) {
        if (c >= 0xE000 && c <= 0xE00F) continue;
        if (c == U' ' || c == U'\t') {
            in_word = false;
            continue;
        }
        if (!in_word) {
            ++words;
            in_word = true;
            if (!(c >= U'A' && c <= U'Z') && c != 0x10C && c != 0x106 && c != 0x160 && c != 0x17D &&
                c != 0x110) {
                return false;       // the first letter is a capital
            }
            upper = true;
            continue;
        }
        if (c >= U'A' && c <= U'Z') return false;      // all in capitals (an abbreviation)
        if (!is_base_letter(c) && c != 0x107 && c != 0x111) return false;
        lower = true;
    }
    return words == 1 && upper && lower;
}

bool clause_reading_is_content(const std::u32string& w, const char32_t* reading) {
    return reading && w == U"lahko" && std::u32string(reading) == U"láhko";
}

bool is_given_name(const std::u32string& w) {
    size_t lo = 0, hi = sizeof(SLOVENIAN_GIVEN_NAMES) / sizeof(SLOVENIAN_GIVEN_NAMES[0]);
    while (lo < hi) {
        const size_t mid = lo + (hi - lo) / 2;
        const int c = compare_key(w, SLOVENIAN_GIVEN_NAMES[mid]);
        if (c == 0) return true;
        if (c < 0) hi = mid;
        else lo = mid + 1;
    }
    return false;
}

bool surname_reading(const std::u32string& w, const std::u32string& previous, bool previous_capitalized,
                     std::u32string& pron) {
    // The names in -e with another accent in Sloleks, hand-accented: surnames
    // and given names that may follow a given name
    static const WordSet not_final = {
        U"lapajne", U"barle", U"strmole", U"kobe", U"habe", U"maze", U"mate", U"kette", U"mengele",
        U"verne", U"inge", U"arne", U"stipe", U"felipe", U"dante", U"monte", U"ante", U"palme",
        U"pavle", U"pepe", U"polde", U"nande", U"nace", U"rade", U"dane", U"dare", U"bine", U"brane",
        U"cene", U"france", U"frane", U"jože", U"jure", U"lojze", U"stane", U"tine", U"tone", U"zvone",
        U"žare", U"čare", U"čile",
    };
    if (!previous_capitalized || w.size() < 3 || w.back() != U'e' || is_vowel_letter(w[w.size() - 2])) return false;
    if (not_final.count(w)) return false;
    size_t vowels = 0;
    for (size_t k = 0; k + 1 < w.size(); ++k) {
        const char32_t c = w[k];
        if (c == U'q' || c == U'w' || c == U'x' || c == U'y') return false;
        if (is_vowel_letter(c)) {
            ++vowels;
            // au stays (Gaube); the other pairs are not Slovenian (Moore, Pierce)
            if (k + 1 < w.size() && is_vowel_letter(w[k + 1]) && !(c == U'a' && w[k + 1] == U'u')) return false;
        }
        if (w[k + 1] == c) return false;        // Hesse, Allende
        if (c == U'h' && k > 0 && std::u32string(U"ctspg").find(w[k - 1]) != std::u32string::npos) return false;
        if (c == U'k' && k > 0 && w[k - 1] == U'c') return false;
    }
    if (vowels == 0) return false;

    const bool named = is_given_name(previous);
    if (!named) return false;
    pron = w.substr(0, w.size() - 1);
    pron += U'è';
    return true;
}

namespace {

// A spelling in -l that is a noun's genitive plural (obvestíl [l], vozíl,
// sporočíl) and a verb's l-participle (obvéstil [u̯], vôzil, sporóčil), and
// noun_percent is how often it is the noun in Gigafida (Sloleks's form
// frequencies): 91 % of vozil, 30 % of obvestil, 3 % of pojasnil. A participle
// needs an auxiliary, a noun does not: that is the main cue. Of the 2,309 tokens
// of such spellings in SUK 1.1 the cues below read 2,245 right, against 2,208
// without them. (A listener's Ni obvestil, On me ni obvestil: November 2026.)
VerbOrNoun participle_or_genitive(const std::vector<std::u32string>& words, size_t i, int noun_percent,
                                  bool before_complement) {
    static const WordSet auxiliaries = {
        U"sem", U"si", U"je", U"bi", U"ni", U"nisem", U"nisi", U"bom", U"boš", U"bo", U"smo", U"ste", U"so",
        U"sva", U"sta", U"nismo", U"niste", U"niso", U"nisva", U"nista", U"bomo", U"boste", U"bodo", U"bova",
        U"bosta",
    };
    static const WordSet clitics = {
        U"me", U"te", U"ga", U"jo", U"nas", U"vas", U"jih", U"ju", U"naju", U"vaju", U"mi", U"ti", U"mu",
        U"ji", U"nam", U"vam", U"jim", U"se", U"si",
    };
    // What counts things, in the genitive plural after it (pet obvestil)
    static const WordSet quantities = {
        U"dve", U"dva", U"tri", U"štiri", U"pet", U"šest", U"sedem", U"osem", U"devet", U"deset", U"enajst",
        U"dvanajst", U"trinajst", U"štirinajst", U"petnajst", U"šestnajst", U"sedemnajst", U"osemnajst",
        U"devetnajst", U"dvajset", U"trideset", U"štirideset", U"petdeset", U"šestdeset", U"sedemdeset",
        U"osemdeset", U"devetdeset", U"sto", U"dvesto", U"tristo", U"štiristo", U"petsto", U"tisoč",
        U"milijon", U"milijonov", U"veliko", U"več", U"mnogo", U"nekaj", U"dosti", U"manj", U"koliko",
        U"toliko", U"preveč", U"premalo", U"največ", U"najmanj", U"oba", U"obe", U"vseh", U"teh", U"nobenih",
        U"tistih", U"nekaterih",
    };
    static const WordSet lacking = {
        U"nimam", U"nimaš", U"nima", U"nimava", U"nimata", U"nimamo", U"nimate", U"nimajo",
    };
    // A noun with its genitive after it (seznam obvestil, pošiljanje sporočil)
    static const WordSet head_nouns = {
        U"seznam", U"seznama", U"število", U"števila", U"vrsta", U"vrste", U"vrst", U"pregled", U"pregleda",
        U"prikaz", U"prikaza", U"pošiljanje", U"pošiljanja", U"zgodovina", U"zgodovine", U"center", U"centra",
        U"nastavitev", U"nastavitve", U"urejanje", U"upravljanje", U"uporaba", U"uporabe", U"prodaja",
        U"prodaje", U"nakup", U"nakupa", U"proizvodnja", U"proizvodnje", U"izdelava", U"izdelave", U"zaloga",
        U"zaloge", U"kup", U"kupa", U"niz", U"niza", U"večina", U"večine", U"polovica", U"polovice",
        U"tretjina", U"odstotek", U"odstotkov", U"par", U"para", U"skupina", U"skupine", U"sistem", U"sistema",
        U"branje", U"prejemanje", U"brisanje", U"izbris", U"izvoz", U"uvoz", U"rezervacija", U"pravilnik",
    };
    // With these before ni it says something is not there, not who did not
    // do something (Trenutno ni obvestil, Janez ni obvestil)
    static const WordSet existential = {
        U"trenutno", U"zdaj", U"sedaj", U"danes", U"tu", U"tukaj", U"tam", U"še", U"že", U"žal", U"sploh",
        U"nikjer", U"več", U"nikoli", U"včeraj", U"jutri",
    };
    static const WordSet coordinators = {U"in", U"ter", U"ali", U"pa", U"oziroma", U"nato", U"potem"};
    auto ends_with = [](const std::u32string& s, const char32_t* tail) {
        const std::u32string t(tail);
        return s.size() >= t.size() && s.compare(s.size() - t.size(), t.size(), t) == 0;
    };
    auto looks_like_participle = [&](const std::u32string& s) {
        return s.size() > 3 && s.back() == U'l' &&
               (s[s.size() - 2] == U'a' || s[s.size() - 2] == U'e' || s[s.size() - 2] == U'i' ||
                s[s.size() - 2] == U'u');
    };
    const size_t n = words.size();
    // Alone, the commoner reading: a genitive plural is no heading (učil, poročil)
    if (n == 1) return noun_percent >= 50 ? VerbOrNoun::Nominal : VerbOrNoun::Verb;
    const std::u32string* prev = i > 0 ? &words[i - 1] : nullptr;
    const std::u32string* next = i + 1 < n ? &words[i + 1] : nullptr;
    if (prev) {
        if (prepositions().count(*prev)) return VerbOrNoun::Nominal;
        if (prev->size() > 3 && ends_with(*prev, U"ih") && *prev != U"njih") {
            return noun_percent >= 2 ? VerbOrNoun::Nominal : VerbOrNoun::Verb;
        }
        if (lacking.count(*prev)) return VerbOrNoun::Nominal;
        if (noun_percent >= 3 && (quantities.count(*prev) || head_nouns.count(*prev))) return VerbOrNoun::Nominal;
    }
    // What a verb of saying takes: Ni obvestil, da pride. Ni sporočil, kdaj.
    if (!next && before_complement && noun_percent < 90) return VerbOrNoun::Verb;
    // At the head of the clause: Obvestil me je. Obvestil je policijo. But
    // Vozil je bilo malo.
    if (i == 0) {
        size_t k = 1;
        while (k < n && clitics.count(words[k])) ++k;
        if (k < n && auxiliaries.count(words[k])) {
            if (noun_percent >= 5 && k == 1 && k + 1 < n && words[k] == U"je" && words[k + 1] == U"bilo") {
                return VerbOrNoun::Nominal;
            }
            return VerbOrNoun::Verb;
        }
    }
    size_t aux = n;
    for (size_t k = 0; k < i; ++k) {
        if (auxiliaries.count(words[k])) aux = k;
    }
    if (aux == n) return noun_percent >= 5 ? VerbOrNoun::Nominal : VerbOrNoun::Verb;
    bool only_clitics = true;
    for (size_t k = aux + 1; k < i; ++k) {
        if (!clitics.count(words[k])) only_clitics = false;
    }
    if (only_clitics) {
        // Ni obvestil. Trenutno ni obvestil. Ni vozil na cesti. But Ni me
        // obvestil, On ni obvestil, Ni obvestil policije.
        if (words[aux] == U"ni" && aux + 1 == i && noun_percent >= 20 &&
            (aux == 0 || (aux == 1 && existential.count(words[0]))) &&
            (!next || prepositions().count(*next))) {
            return VerbOrNoun::Nominal;
        }
        return VerbOrNoun::Verb;
    }
    // se je poročil: the reflexive makes it the verb
    for (size_t k = 0; k < i; ++k) {
        if (words[k] == U"se") return VerbOrNoun::Verb;
    }
    if (noun_percent >= 5) {
        // Another participle has the auxiliary (Nisem dobil obvestil), unless
        // this one is coordinated with it (je prišel in obvestil)
        for (size_t k = aux + 1; k < i; ++k) {
            if (!looks_like_participle(words[k])) continue;
            bool coordinated = false;
            for (size_t c = k + 1; c < i; ++c) {
                if (coordinators.count(words[c])) coordinated = true;
            }
            if (!coordinated) return VerbOrNoun::Nominal;
        }
    }
    return noun_percent >= 40 ? VerbOrNoun::Nominal : VerbOrNoun::Verb;
}

} // namespace

VerbOrNoun context_reading(const std::vector<std::u32string>& words, size_t i, bool participle, int noun_percent,
                           bool before_complement) {
    static const WordSet biti = {
        U"sem", U"si", U"je", U"sva", U"sta", U"smo", U"ste", U"so", U"nisem", U"nisi",
        U"ni", U"nisva", U"nista", U"nismo", U"niste", U"niso", U"bi", U"bom", U"boš",
        U"bo", U"bova", U"bosta", U"bomo", U"boste", U"bodo", U"bil", U"bila", U"bilo",
        U"bili", U"bile", U"biti",
    };
    static const WordSet object_clitics = {
        U"me", U"te", U"ga", U"jo", U"nas", U"vas", U"jih", U"ju", U"naju", U"vaju",
        U"mi", U"ti", U"mu", U"ji", U"nam", U"vam", U"jim",
    };
    static const WordSet counts = {
        U"dve", U"dva", U"obe", U"oba", U"tri", U"štiri", U"pet", U"šest", U"sedem",
        U"osem", U"devet", U"deset", U"nekaj", U"več", U"mnogo", U"veliko",
    };
    const std::u32string& w = words[i];
    const std::u32string* prev = i > 0 ? &words[i - 1] : nullptr;
    const bool last = i + 1 == words.size();
    auto ends_with = [](const std::u32string& s, const char32_t* tail) {
        const std::u32string t(tail);
        return s.size() >= t.size() && s.compare(s.size() - t.size(), t.size(), t) == 0;
    };

    // A verb's present opens the clause before its object: Veseli me, da sva se
    // spoznala. Žalosti me, da nisi prišel. Skrbi me. (SSKJ² veseli nas, da delo
    // napreduje; SP ~i me, da vas vidim.) All 20 occurrences of a spelling of this
    // table at the head of a clause before an object clitic in SUK 1.1 are verbs;
    // a noun there has a form of biti after the clitic (Pomoči mi ni).
    // Also after a conjunction (rojen sem bil leta 2026 in veseli me, da ...:
    // the sample text; a listener's report of 2026-11-03)
    static const WordSet conjunctions = {U"in", U"pa", U"ter", U"ki", U"ampak", U"a", U"toda"};
    if ((i == 0 || conjunctions.count(words[i - 1])) && !participle && !last &&
        object_clitics.count(words[i + 1]) && !(i + 2 < words.size() && biti.count(words[i + 2]))) {
        return VerbOrNoun::Verb;
    }

    // veseli: veselí (se) "looks forward, pleases", veséli "cheerful". The
    // adjective follows a form of biti (smo veseli, so bili zelo veseli);
    // the verb has its se, an object (nas veseli), ne, or ends the clause
    // (On se zelo veseli. To me veseli.) The imperative vesêli se! is read
    // by clause_reading(), which sees the exclamation mark.
    if (w == U"veseli") {
        for (size_t k = i >= 3 ? i - 3 : 0; k < i; ++k) {
            if (biti.count(words[k])) return VerbOrNoun::Nominal;
        }
        // An object clitic anywhere before it, not only right before it: Me
        // zelo veseli vaš obisk, To me srčno veseli (a listener's report of
        // 2026-11-03: "me veseli", "me zelo veseli", "me srčno veseli", "in me
        // veseli" are all the verb); with a form of biti before it the
        // adjective took the clause above (Otroci so ga veseli)
        for (size_t k = 0; k < i; ++k) {
            if (words[k] == U"se" || words[k] == U"si") return VerbOrNoun::Verb;
            if (words[k] != U"ti" && words[k] != U"te" && object_clitics.count(words[k])) return VerbOrNoun::Verb;
        }
        if ((prev && (object_clitics.count(*prev) || *prev == U"ne")) || last) return VerbOrNoun::Verb;
        // An object clitic right after it anywhere in the clause (in veseli
        // me, da ...), not ti, which is also the demonstrative (veseli ti
        // ljudje)
        if (!last && words[i + 1] != U"ti" && object_clitics.count(words[i + 1])) return VerbOrNoun::Verb;
        return VerbOrNoun::Lexicon;
    }
    // gori: gorí "burns" (Peč gori vsak dan), but gôri the locative and the
    // dative of gôra after a preposition (na gori) or an adjective (v Kranjski
    // Gori, Črni gori; the 24 occurrences of the noun in SUK 1.1 that have a
    // word before them) and the adverb "up there" after tam (SSKJ² gôri).
    // The lexicon's commoner gôri would read the listener's Na gori gori ogenj
    // twice as the noun.
    if (w == U"gori") {
        if (!prev) return VerbOrNoun::Verb;     // Gori! Gori luč.
        static const WordSet up = {U"tam", U"tu", U"tukaj", U"tja", U"dve", U"obe", U"tej", U"neki",
                                   U"visoko", U"daleč"};
        static const WordSet not_adjective = {U"gori", U"tudi", U"vsi", U"oni", U"ali", U"kateri", U"nekateri",
                                              U"zaradi", U"hkrati", U"nikoli", U"lani", U"ponoči", U"sredi",
                                              U"okoli", U"zdi", U"vidi", U"pravi", U"meni", U"tebi", U"sebi"};
        if (prepositions().count(*prev) || up.count(*prev)) return VerbOrNoun::Nominal;
        if (prev->size() >= 4 && prev->back() == U'i' && !not_adjective.count(*prev)) return VerbOrNoun::Nominal;
        return VerbOrNoun::Verb;
    }
    // leti: léti is the dual and the instrumental of léto (dve leti, pred
    // leti, z leti) or the plural of lèt "flight" (leti so odpovedani),
    // always with a number or a preposition close before it or a form of
    // biti after it; letí "flies" elsewhere (ptič leti, letalo leti nad
    // mestom). The noun is 98 % of the corpus, and every one of its
    // occurrences there has such a word next to it.
    if (w == U"leti") {
        for (size_t k = i >= 3 ? i - 3 : 0; k < i; ++k) {
            const std::u32string& x = words[k];
            if (prepositions().count(x) || counts.count(x) || ends_with(x, U"imi") || ends_with(x, U"ema")) {
                return VerbOrNoun::Nominal;
            }
        }
        if (!last && biti.count(words[i + 1])) return VerbOrNoun::Nominal;
        return VerbOrNoun::Verb;
    }

    if (participle && noun_percent >= 0) return participle_or_genitive(words, i, noun_percent, before_complement);
    // A noun in the nominative or the accusative that a participle is spelled
    // like (predál and predáw, hotél and hôtel, lokál, pokál): in a clause of
    // several words, with no auxiliary in it, it is no participle (Odpri predal)
    if (participle && words.size() > 1) {
        bool auxiliary = false;
        for (const std::u32string& x : words) {
            if (biti.count(x)) auxiliary = true;
        }
        if (!auxiliary) return VerbOrNoun::Nominal;
    }

    if (prev) {
        if (prepositions().count(*prev) || *prev == U"dve" || *prev == U"dva" || *prev == U"obe" ||
            *prev == U"oba") {
            return VerbOrNoun::Nominal;
        }
        // brez novih vozil, teh sporočil: a genitive plural after an adjective
        if (participle && prev->size() > 3 && ends_with(*prev, U"ih")) return VerbOrNoun::Nominal;
        if (*prev == U"ne") return VerbOrNoun::Verb;
        // To me žalosti. Ga skrbi. An object clitic before a present (not ti and
        // te, also the demonstratives: ti pogoji, te dobe); after an auxiliary
        // only a participle could follow, so this is another word (so nas levi in
        // desni, so ga poleti). si is the reflexive here: si ga deli, si ga lasti.
        if (!participle && *prev != U"ti" && *prev != U"te" && object_clitics.count(*prev) &&
            !(i >= 2 && words[i - 2] != U"si" && biti.count(words[i - 2]))) {
            return VerbOrNoun::Verb;
        }
        // je vozil, sem pojasnila, so sporočili: the auxiliary right before
        // a participle that agrees with it (not -lo: to je besedilo, pravilo)
        if (participle) {
            static const WordSet singular = {U"sem", U"si", U"je", U"bi", U"ni", U"nisem", U"nisi"};
            static const WordSet plural = {U"smo", U"ste", U"so", U"sva", U"sta", U"nismo",
                                           U"niste", U"niso", U"nisva", U"nista"};
            if (singular.count(*prev) && (ends_with(w, U"l") || ends_with(w, U"la"))) return VerbOrNoun::Verb;
            if (plural.count(*prev) && (ends_with(w, U"li") || ends_with(w, U"le"))) return VerbOrNoun::Verb;
        }
    }
    return VerbOrNoun::Lexicon;
}

void Lexicon::set_tables(const Table& forms, const Table& stems) {
    m_form_table = forms;
    m_stem_table = stems;
}

// =============================================================================
// Stress and the sounds that depend on it
// =============================================================================

namespace {

// Where the stress falls when no dictionary says: the suffix rules of the
// Slovenian word classes whose accent is regular, then the second-last
// syllable, the commonest place in the language. Returns the index into
// `nuclei` (the prothetic u of vsak, vzeti excluded).
int default_stress(const std::u32string& w, const WordPhones& wp, const std::vector<bool>& prothetic,
                   Capital capital) {
    const bool foreign_letter = has_name_letter(w);
    const bool surname = capital != Capital::None || foreign_letter;
    const bool name = capital == Capital::Head || capital == Capital::Inside || foreign_letter;
    std::vector<int> candidates;     // nucleus indexes that may carry the stress
    for (size_t k = 0; k < wp.nuclei.size(); ++k) {
        if (!prothetic[k]) candidates.push_back(static_cast<int>(k));
    }
    if (candidates.empty()) return -1;
    if (candidates.size() == 1) return candidates[0];
    auto letter_of = [&](int candidate) {
        return static_cast<size_t>(wp.letter[static_cast<size_t>(wp.nuclei[static_cast<size_t>(candidate)])]);
    };

    // Surnames in -ović, -ević: the syllable before the suffix, which is
    // the first syllable of the three-syllable ones (Pétrović, Pópovič,
    // Túrković, Srébrović) and the second of four (Stojánović, Kováčević,
    // Milóšević), as in the hand-accented names of Sloleks (63 of 65 with
    // three syllables, 69 of 72 with four, 12 of 13 with five); before
    // -ijević the syllable before that (Antónijević, Dimítrijević); an i
    // after a vowel stands for j (Mihailović as Mihájlović); without a
    // vowel before the suffix, its o (Kôvič, Nóvič). The lexicon's
    // hand-accented forms still win, the stems of common words do not
    // (phonemize()).
    if (surname) {
        size_t end = vic_surname(w);
        if (end != std::u32string::npos) {
            if (end >= 3 && w[end - 2] == U'i' && w[end - 1] == U'j') end -= 2;
            int before = -1;
            for (size_t c = 0; c < candidates.size(); ++c) {
                if (letter_of(candidates[c]) < end) before = static_cast<int>(c);
            }
            if (before < 0) return candidates[0];
            const size_t letter = letter_of(candidates[static_cast<size_t>(before)]);
            if (before > 0 && w[letter] == U'i' && letter > 0 && is_vowel_letter(w[letter - 1])) --before;
            return candidates[static_cast<size_t>(before)];
        }
    }

    // The other names the lexicon does not know, in any case form: the
    // patterns of the hand-accented names of Sloleks that the rules of
    // common words miss (on its 5,000 names written as one word they
    // raise the rules from 74 to 80 %; the counts are of names right by
    // the pattern against those right by the common rules)
    if (name) {
        auto nuclei_before = [&](size_t letter) {      // candidate positions before a letter
            std::vector<int> out;
            for (size_t c = 0; c < candidates.size(); ++c) {
                if (letter_of(candidates[c]) < letter) out.push_back(static_cast<int>(c));
            }
            return out;
        };
        auto at_letter = [&](size_t letter) {
            for (size_t c = 0; c < candidates.size(); ++c) {
                if (letter_of(candidates[c]) == letter) return candidates[c];
            }
            return -1;
        };
        size_t b;
        // -ič, -ić of three syllables and more (not -ović, above): before
        // -čič after a consonant the second-last syllable (Zupánčič,
        // Miklávčič, Gregórčič), otherwise the third-last (Gólobič,
        // Tómažič, Míhelič; Kóstelić, Níkolić, Káradžić): 117 and 76
        // against 64 and 67
        if ((b = name_base(w, U"ič")) != 0 || (b = name_base(w, U"ić")) != 0) {
            const std::vector<int> base = nuclei_before(b);
            if (base.size() >= 3) {
                const bool cic = w[b - 1] == C_CARON && b >= 4 && w[b - 3] == C_CARON &&
                                 !is_vowel_letter(w[b - 4]);
                return candidates[static_cast<size_t>(base[base.size() - (cic ? 2 : 3)])];
            }
        }
        // -ovec, -evec, and the oblique cases -ovca, -evcu ...: the syllable
        // before (Ájdovec, Búkovec, Gábrovca): 34 against 6, the oblique
        // forms of the names of Sloleks 226 against 100 (Debévca of the
        // contracted Debévc is the rest)
        if (ends_with(w, U"ovec") || ends_with(w, U"evec")) {
            const std::vector<int> base = nuclei_before(w.size() - 4);
            if (base.size() >= 1 && nuclei_before(w.size()).size() >= 3) {
                return candidates[static_cast<size_t>(base.back())];
            }
        } else if (((b = name_base(w, U"ovc")) != 0 || (b = name_base(w, U"evc")) != 0) && b < w.size()) {
            const std::vector<int> base = nuclei_before(b - 3);
            if (!base.empty()) return candidates[static_cast<size_t>(base.back())];
        }
        // -ec without its e after n, v, j, l, r, m: the last syllable
        // (Dolénc, Bohínc, Debévc, Dobrájc): 39 against 13
        if ((b = name_base(w, U"c")) != 0 && b >= 3 && is_vowel_letter(w[b - 3]) &&
            std::u32string(U"nvjlrm").find(w[b - 2]) != std::u32string::npos && nuclei_before(b).size() >= 2) {
            const int k = at_letter(b - 3);
            if (k >= 0) return k;
        }
        // Slavic names in -mir, -bor, -slav, -dar, -gor after a vowel: the
        // first syllable (Bógomir, Dálibor, Ljúbomir, Bránimir): 23 against 1
        for (const char32_t* suffix : {U"mir", U"bor", U"slav", U"dar", U"gor"}) {
            if ((b = name_base(w, suffix)) != 0) {
                const size_t len = std::char_traits<char32_t>::length(suffix);
                if (b > len && is_vowel_letter(w[b - len - 1]) && nuclei_before(b).size() >= 3) {
                    return candidates[0];
                }
            }
        }
        // -iel, -ael: the first syllable (Gábriel, Dániel, Ízrael)
        if (ends_with(w, U"iel") || ends_with(w, U"ael")) return candidates[0];
        // -in: its i (Albín, Avguštín, Albína, Martína): 56 against 21
        if ((b = name_base(w, U"in")) != 0 && nuclei_before(b).size() >= 2) {
            const int k = at_letter(b - 2);
            if (k >= 0) return k;
        }
        // -ak of three syllables and more: its a (Cerkvenják, Gorenják): 18
        // against 0
        if ((b = name_base(w, U"ak")) != 0 && nuclei_before(b).size() >= 3) {
            const int k = at_letter(b - 2);
            if (k >= 0) return k;
        }
        // -ij, -ija: the syllable before (Júrij, Antónij, Albánija,
        // Bolgárija, Ávstrija): 18 against 3, 85 against 30. A feminine
        // only inside a clause: at its head the word may be a common noun
        // the lexicon lacks, stressed on the i (biologíja)
        if ((b = name_base(w, U"ij")) != 0 && (b == w.size() || capital == Capital::Inside)) {
            const std::vector<int> base = nuclei_before(b - 2);
            if (!base.empty() && nuclei_before(b).size() >= 2) return candidates[static_cast<size_t>(base.back())];
        }
        // -aj, -ej, -oj, -uj: that vowel (Andréj, Jernéj, Cigój): 29
        // against 14; -al, -il, -ul of two syllables: its vowel (Abdúl,
        // Kanál, Ciríl): 19 against 3 (-el and -ol are as often on the
        // first syllable: Pável, Kárel, Bártol)
        if ((b = name_base(w, U"j")) != 0 && b >= 2 && w[b - 2] != U'i' && is_vowel_letter(w[b - 2]) &&
            nuclei_before(b).size() >= 2) {
            const int k = at_letter(b - 2);
            if (k >= 0) return k;
        }
        if ((b = name_base(w, U"l")) != 0 && b >= 2 &&
            (w[b - 2] == U'a' || w[b - 2] == U'i' || w[b - 2] == U'u') && nuclei_before(b).size() == 2) {
            const int k = at_letter(b - 2);
            if (k >= 0) return k;
        }
    }

    auto nucleus_of_letter = [&](size_t letter) {
        for (size_t k = 0; k < wp.nuclei.size(); ++k) {
            if (wp.letter[static_cast<size_t>(wp.nuclei[k])] >= static_cast<int>(letter)) {
                return static_cast<int>(k);
            }
        }
        return static_cast<int>(wp.nuclei.size()) - 1;
    };
    auto last_vowel_before = [&](size_t end) -> int {
        for (size_t i = end; i-- > 0;) {
            if (is_vowel_letter(w[i])) return static_cast<int>(i);
        }
        return -1;
    };

    const size_t syllables = candidates.size();
    auto penult = [&] { return candidates[syllables - 2]; };
    auto antepenult = [&] { return candidates[syllables >= 3 ? syllables - 3 : 0]; };
    auto final = [&] { return candidates.back(); };
    auto before = [&](size_t suffix_letters) {       // the syllable before a suffix
        size_t start = w.size() - suffix_letters;
        int lv = last_vowel_before(start);
        return lv < 0 ? penult() : nucleus_of_letter(static_cast<size_t>(lv));
    };

    // The rules below follow the counts over the manually accented lemmas
    // of the Sloleks lexicon and § 897-901 of the Pravopis: the suffixes
    // that always carry the stress, then the syllable before a suffix
    // where that is the rule, then the second-last syllable.

    // -írati, -íran, -íranje and the rest of the paradigm (študírati,
    // organizírati): the i before r, always
    {
        size_t pos = w.rfind(U"ir");
        if (pos != std::u32string::npos && pos >= 3 && pos + 2 < w.size() &&
            (w[pos + 2] == U'a' || w[pos + 2] == U'e' || w[pos + 2] == U'u' || w[pos + 2] == U'o' ||
             w[pos + 2] == U'j')) {
            return nucleus_of_letter(pos);
        }
    }
    // -ováti, -eváti and their nouns -ovánje, -evánje (kupováti, potováti,
    // črkovánje, pričakovánje, izpolnjeváti, poučevánje): the a of the
    // suffix; -ávati, -ávanje (predávati, spoznávati) and the -évati whose
    // e belongs to the stem, after a non-palatal consonant (zahtévati,
    // ogrévati, dozorévati): the first vowel
    for (const char32_t* suffix : {U"ovati", U"ovanje", U"ovanja", U"ovanju", U"ovanjem",
                                   U"avati", U"avanje", U"avanja", U"avanju", U"avanjem",
                                   U"evati", U"evanje", U"evanja", U"evanju", U"evanjem"}) {
        std::u32string suf(suffix);
        if (ends_with(w, suf) && w.size() >= suf.size() + 2) {
            const size_t start = w.size() - suf.size();
            bool on_a = suf[0] == U'o';
            if (suf[0] == U'e') {
                const char32_t before = w[start - 1];
                on_a = before == C_CARON || before == S_CARON || before == Z_CARON ||
                       before == U'j' || before == U'c';
            }
            return nucleus_of_letter(start + (on_a ? 2 : 0));
        }
    }
    // -éti infinitives (živéti, želéti, hotéti, razuméti): the e
    if (ends_with(w, U"eti") && w.size() >= 5 && !ends_with(w, U"deti")) {
        return nucleus_of_letter(w.size() - 3);
    }
    // -niti infinitives (pritísniti, klíkniti, dotakníti is the exception):
    // the syllable before -niti
    if (ends_with(w, U"niti") && w.size() >= 6) return before(4);
    // -ati, -anje: a verb of three syllables on the a (igráti, iskáti,
    // končáti, vprašánje), a longer one on the stem (poslúšati, pomágati,
    // prihájati, poslúšanje)
    if ((ends_with(w, U"ati") || ends_with_any(w, {U"anje", U"anja", U"anju", U"anjem", U"anjih"})) &&
        syllables >= 4) {
        return antepenult();
    }
    // Comparatives and superlatives: -ejši keeps the adjective's stress
    // before it (abótnejši, zanimívejši), the others are stressed on the
    // syllable before -ši (lépši, bóljši, najlépši: the naj- is a second,
    // weaker accent)
    if (ends_with_any(w, {U"ejši", U"ejša", U"ejše", U"ejšo", U"ejšu", U"ejših", U"ejšim", U"ejšimi",
                          U"ejšega", U"ejšemu", U"ejšem"})) {
        size_t pos = w.rfind(U"ejš");
        int lv = last_vowel_before(pos);
        if (lv >= 0) return nucleus_of_letter(static_cast<size_t>(lv));
    }
    if (ends_with_any(w, {U"ši", U"ša", U"še", U"šo", U"šu", U"ših", U"šim", U"šimi", U"šega",
                          U"šemu", U"šem"}) && syllables >= 2) {
        size_t pos = w.rfind(S_CARON);
        int lv = last_vowel_before(pos);
        if (lv >= 0) return nucleus_of_letter(static_cast<size_t>(lv));
    }
    // nàjbolj, nàjmanj, nàjveč, nàjprej
    if (starts_with(w, U"naj") && ends_with_any(w, {U"bolj", U"manj", U"več", U"prej", U"raje"})) return 0;
    // Nouns in -cija, -zija, -sija (informácija, televízija, diskúsija):
    // the syllable before; the other nouns in -íja (policíja, kemíja,
    // biologíja, Srbíja) on the i
    if (ends_with_any(w, {U"ija", U"ije", U"iji", U"ijo", U"ij", U"ijah", U"ijam", U"ijami"}) &&
        w.size() >= 5) {
        size_t pos = w.rfind(U"ij");
        if (pos != std::u32string::npos && pos >= 2) {
            char32_t before_ij = w[pos - 1];
            if (before_ij == U'c' || before_ij == U'z' || before_ij == U's') return before(w.size() - pos + 1);
            return nucleus_of_letter(pos);
        }
    }
    // Adjectives in -ljív, -ív (berljív, zanimív) and nouns in -íst, -áč,
    // -ár (turíst, kováč, abecedár) have the stress at the end
    if (w.size() >= 5 && ends_with_any(w, {U"ljiv", U"iv", U"ist", U"ač"})) return final();
    // -ščina on the syllable before (angléščina); -ína, -íšče, -ílo,
    // -ízem, -átor, -úra, -ácija, -étje, -álen on the suffix vowel
    if (ends_with_any(w, {U"ščina", U"ščine", U"ščini", U"ščino"})) return before(5);
    struct Suffix { const char32_t* text; size_t vowel; };
    static const Suffix ON_SUFFIX[] = {
        {U"ina", 0}, {U"ine", 0}, {U"ini", 0}, {U"ino", 0}, {U"inah", 0}, {U"inam", 0},
        {U"išče", 0}, {U"išča", 0}, {U"išču", 0}, {U"iščem", 0}, {U"išč", 0},
        {U"ilo", 0}, {U"ila", 0}, {U"ilu", 0}, {U"ilom", 0},
        {U"izem", 0}, {U"izma", 0}, {U"izmu", 0}, {U"izmom", 0}, {U"izmi", 0},
        {U"ator", 0}, {U"atorja", 0}, {U"atorju", 0}, {U"atorjem", 0}, {U"atorji", 0}, {U"atorjev", 0},
        {U"ura", 0}, {U"ure", 0}, {U"uri", 0}, {U"uro", 0}, {U"urah", 0}, {U"uram", 0},
        {U"etje", 0}, {U"etja", 0}, {U"etju", 0}, {U"etjem", 0},
        {U"alen", 0}, {U"alna", 0}, {U"alno", 0}, {U"alni", 0}, {U"alne", 0}, {U"alnega", 0},
        {U"alnemu", 0}, {U"alnem", 0}, {U"alnih", 0}, {U"alnim", 0}, {U"alnimi", 0},
        {U"ota", 0}, {U"ote", 0}, {U"oti", 0}, {U"oto", 0},
    };
    for (const Suffix& suf : ON_SUFFIX) {
        std::u32string suffix(suf.text);
        if (!ends_with(w, suffix) || w.size() < suffix.size() + 2) continue;
        size_t start = w.size() - suffix.size();
        if (last_vowel_before(start) < 0) continue;      // a stem without a vowel
        return nucleus_of_letter(start + suf.vowel);
    }
    // -ica (déklica, knjížnica, učíteljica), -ičen (ístičen) and the
    // other suffixes of two syllables: the syllable before them
    if (ends_with_any(w, {U"ica", U"ice", U"ici", U"ico", U"icah", U"icam", U"icami",
                          U"ičen", U"ična", U"ično", U"ični", U"ične", U"ičnega", U"ičnemu",
                          U"ičnem", U"ičnih", U"ičnim"}) && syllables >= 3) {
        return antepenult();
    }
    // Default: the second-last syllable
    return candidates[candidates.size() - 2];
}

// Settles what depends on the stress: the length and quality of the
// stressed vowel when no mark gave them, the schwa of an unstressed final
// -er, -el, -en, -ek, -ec, -ev, and the l at the end of a word.
// The forms of SLOVENIAN_FINAL_L (see finish_word())
bool final_l_form(const std::u32string& w) {
    static const WordSet forms = [] {
        WordSet set;
        for (const char* form : SLOVENIAN_FINAL_L) set.insert(PhonemeMapper::utf8_to_utf32(form));
        return set;
    }();
    return forms.count(w) != 0;
}

void finish_word(const Analysis& a, WordPhones& wp) {
    const std::u32string& w = a.plain;
    const size_t n = w.size();
    if (wp.stress < 0 || wp.nuclei.empty()) return;
    const size_t stressed_index = static_cast<size_t>(wp.nuclei[static_cast<size_t>(wp.stress)]);
    Phone& stressed = wp.phones[stressed_index];
    const size_t stressed_letter = static_cast<size_t>(wp.letter[stressed_index]);
    const uint8_t sflags = a.flags[stressed_letter];

    // Length: a stressed vowel is long unless the lexicon says short; the
    // schwa is always short
    if (!(sflags & (F_STRESS))) {
        // Stress by rule: long, and a close e or o (the commoner quality of
        // the long stressed e and o, and that of every loanword: § 222)
        if (stressed.ph != Ph::AX) stressed.is_long = true;
        if (stressed.ph == Ph::EH && !(sflags & (F_OPEN | F_SCHWA))) stressed.ph = Ph::EE;
        if (stressed.ph == Ph::OH && !(sflags & F_OPEN)) stressed.ph = Ph::OO;
    }
    if (stressed.ph == Ph::AX) stressed.is_long = false;

    // Letter names (bé, cé; ef, èl, ès; á é í ó ú): a long close vowel
    // when the name ends in it, short and open before a consonant (§ 7)
    if (a.letter_name && !(sflags & F_STRESS)) {
        const bool final_vowel = stressed_letter + 1 == n;
        stressed.is_long = final_vowel;
        if (stressed.ph == Ph::EH || stressed.ph == Ph::EE) stressed.ph = final_vowel ? Ph::EE : Ph::EH;
        if (stressed.ph == Ph::OH || stressed.ph == Ph::OO) stressed.ph = final_vowel ? Ph::OO : Ph::OH;
    }

    // The schwa of an unstressed final syllable -eC after a consonant:
    // veter, misel, lačen, petek, konec, cerkev (Slovenski pravopis: the
    // "polglasnik" written e). Not where the lexicon spelled the vowel.
    if (n >= 3 && !wp.nuclei.empty()) {
        const size_t last_nucleus = static_cast<size_t>(wp.nuclei.back());
        const size_t letter = static_cast<size_t>(wp.letter[last_nucleus]);
        if (letter >= a.notation_letters && letter + 2 == n && w[letter] == U'e' &&
            last_nucleus != stressed_index &&
            !is_vowel_letter(w[letter - 1]) && w[letter - 1] != U'j') {
            // -ev only after k and t (cerkev, bukev, molitev): the genitive
            // plural -ev (delavcev, stricev) keeps its e; -em only after z
            // and t (absolutizem, algoritem): the instrumental -em
            // (delavcem, možem) keeps its e
            char32_t c = w[letter + 1];
            char32_t before = w[letter - 1];
            if (c == U'r' || c == U'l' || c == U'n' || c == U'k' || c == U'c' ||
                (c == U'v' && (before == U'k' || before == U't')) ||
                (c == U'm' && (before == U'z' || before == U't'))) {
                wp.phones[last_nucleus].ph = Ph::AX;
                wp.phones[last_nucleus].is_long = false;
            }
        }
    }

    // l at the end of a word after a vowel (§ 650): [w] in the participles
    // (bil, dal, delal, videl, nosil), after a schwa (rekel, misel, topel)
    // and in the nouns and adjectives of the list (stol, sol, bel, vesel);
    // [l] otherwise, as in the loanwords (hotél, kanál, avtomobíl, apríl,
    // modél). A word with no stress on the last syllable, or of one syllable,
    // looks like a participle: those that are not, whatever the vowel, are in
    // SLOVENIAN_FINAL_L, from the transcriptions of Sloleks without the words
    // SSKJ² and SP mark [-u̯] (ángel, módul, stíl, kábel, ansámbel, síl, žíl,
    // škátel), and so is a capitalized word in no lexicon, nearly always a
    // foreign name (Michael, Paul, Excel). A notation l follows the rule too
    // (the lexicon writes w only where the rule would say l).
    if (n >= 2 && w.back() == U'l' && !wp.phones.empty() && wp.phones.back().ph == Ph::L &&
        is_vowel_letter(w[n - 2])) {
        const bool final_stress = static_cast<size_t>(wp.stress + 1) == wp.nuclei.size();
        const bool monosyllable = wp.nuclei.size() == 1;
        const bool schwa = wp.phones[static_cast<size_t>(wp.nuclei.back())].ph == Ph::AX;
        const bool participle_shape = w[n - 2] != U'o';
        const bool listed_w = l_final_w_word(w);
        const bool keeps_l = (a.flags.back() & F_L) != 0 ||
                             (!listed_w && (a.unknown_name || l_final_l_word(w) || final_l_form(w)));
        if (!keeps_l && (schwa || listed_w || (participle_shape && (monosyllable || !final_stress)))) {
            wp.phones.back().ph = Ph::W;
        }
    }
}

// The letters and flags of a word from a lexicon pronunciation, plus the
// ending read by rule after a stem.
bool analysis_from_pron(const std::u32string& pron, const std::u32string& ending, Analysis& a) {
    std::u32string plain;
    std::vector<uint8_t> flags;
    if (!read_notation(pron, plain, flags, nullptr)) return false;
    a.plain = plain;
    a.flags = flags;
    a.notation_letters = plain.size();
    for (char32_t c : ending) {
        a.plain.push_back(c);
        a.flags.push_back(0);
    }
    return true;
}

} // namespace

// A stem of the lexicon followed by a derivational suffix that begins with
// an l before a consonant: the agent nouns and instruments in -lec, -lka,
// -lnik, -lnica, -lski, -lstvo and the nouns in -lišče. The stem's accent
// does not hold there: the stress moves to the vowel before the l
// (zagánjati but zaganjálnik, napájati / napajálnik, bráti / brálec,
// gledáti / gledálec, poslúšati / poslušálec, učíti / učílnica, spáti /
// spálnica), or to the í of -lišče (glédati / gledalíšče). SSKJ gives every
// one of these with that accent.
void stress_derived_l_suffix(const std::u32string& ending, Analysis& a) {
    if (ending.size() < 2 || ending[0] != U'l') return;
    const size_t stem = a.notation_letters;
    if (stem == 0 || !is_vowel_letter(a.plain[stem - 1])) return;
    size_t target;
    if (starts_with(ending, U"lišč")) {     // gledalíšče, igríšče
        target = stem + 1;
    } else if (ending[1] == U'c' || ending[1] == U'k' || ending[1] == U'n' || ending[1] == U's' ||
               ending[1] == U't') {
        target = stem - 1;
    } else {
        return;
    }
    for (uint8_t& f : a.flags) f &= static_cast<uint8_t>(~(F_STRESS | F_LONG | F_CLOSE | F_OPEN));
    a.flags[target] |= F_STRESS | F_LONG;
    if (a.plain[target] == U'e' || a.plain[target] == U'o') a.flags[target] |= F_CLOSE;
}

void phonemize(const std::u32string& word, const std::vector<uint8_t>& marks,
               const Lexicon* user, bool letter_name, WordPhones& out,
               const std::u32string* pron_override, Capital capital) {
    out = WordPhones();
    const bool surname = capital != Capital::None || has_name_letter(word);
    Analysis a;
    a.letter_name = letter_name;
    std::vector<bool> prothetic;

    // 1. Accent marks written in the text. Slovenian dictionaries' marks:
    // acute = long (close e, o), circumflex = long open e, o, grave = short.
    bool marked = std::any_of(marks.begin(), marks.end(), [](uint8_t m) { return m != 0; });
    if (marked) {
        a.plain = word;
        a.flags.assign(word.size(), 0);
        for (size_t i = 0; i < word.size() && i < marks.size(); ++i) {
            uint8_t m = marks[i];
            if (!m) continue;
            uint8_t f = 0;
            if (m & MARK_STRESS) f |= F_STRESS;
            if (m & MARK_LONG) f |= F_LONG;
            if (word[i] == U'e' || word[i] == U'o') {
                if ((m & MARK_RISE) && (m & MARK_LONG)) f |= F_CLOSE;    // é ó
                else if (m & MARK_STRESS) f |= F_OPEN;                   // ê ô è ò
            }
            a.flags[i] = f;
        }
        a.notation_letters = 0;
        letters_to_phones(a, out, prothetic);
        // A compound with two accents (àbiótski, nàjlépši) has its main
        // stress on the last of them
        for (size_t k = 0; k < out.nuclei.size(); ++k) {
            if (a.flags[static_cast<size_t>(out.letter[static_cast<size_t>(out.nuclei[k])])] & F_STRESS) {
                out.stress = static_cast<int>(k);
            }
        }
        if (out.stress < 0) out.stress = default_stress(a.plain, out, prothetic, capital);
        out.from_lexicon = true;
        finish_word(a, out);
    } else {
        // 2. The lexicon: the user's exact forms and stems, then the
        // built-in ones; a compound numeral from its parts. The name of a
        // letter is read by the rule (finish_word), not as the word it is
        // spelled like (je, en, es).
        std::u32string pron;
        bool found = false;
        size_t stem_length = word.size();
        if (user && !letter_name) {
            found = user->find_exact(word, pron) || user->find_stem(word, stem_length, pron);
        }
        if (!found && pron_override && !letter_name) {
            pron = *pron_override;
            found = true;
            stem_length = word.size();
        }
        if (!found && (!letter_name || word.size() > 2)) {
            // A surname in -ović is read by its rule (default_stress()),
            // not as a common word's stem followed by an ending (petr*,
            // turk*): only a whole form of the lexicon decides it. A name
            // inside a clause takes a stem only with an ending of the
            // declension (Kovačiča: kováčič* + a), not another word's
            // beginning (Ferdolenc: férd* of Ferdo + olenc)
            stem_length = word.size();
            found = builtin_lexicon().find_exact(word, pron) ||
                    (!(surname && vic_surname(word) != std::u32string::npos) &&
                     builtin_lexicon().find_stem(word, stem_length, pron,
                                                 capital == Capital::Inside || has_name_letter(word)
                                                     ? declension_ending : nullptr));
            if (!found) stem_length = word.size();
        }
        if (!found && !letter_name) {
            std::u32string ones, tens, tens_pron;
            if (split_compound_numeral_impl(word, ones, tens) && builtin_lexicon().find_exact(tens, tens_pron)) {
                pron = ones + U"in" + tens_pron;
                found = true;
                stem_length = word.size();
            }
        }
        if (found && analysis_from_pron(pron, word.substr(stem_length), a)) {
            out.from_lexicon = true;
            if (stem_length < word.size()) stress_derived_l_suffix(word.substr(stem_length), a);
            // A stem and the ending -l alone is the l-participle: [w], also
            // after a stem that ends in its stressed vowel (želé*, sledí*,
            // učí*, predá*: Slovenski pravopis 2001, § 650 and the
            // transcriptions of Sloleks). finish_word() keeps [l] after a
            // final stress for the loanwords, whose stem has the l (kanál*,
            // hotél*); without this the participles were read like them.
            // A participle read by the stem of its other forms (sledíl*) is
            // listed whole with its w by the generator.
            if (stem_length + 1 == word.size() && word.back() == U'l' && word.size() >= 2 &&
                is_vowel_letter(word[word.size() - 2]) && word[word.size() - 2] != U'o' &&
                a.flags.size() == word.size()) {
                a.flags.back() |= F_W;
            }
        } else {
            a.plain = word;
            a.flags.assign(word.size(), 0);
            a.notation_letters = 0;
            a.unknown_name = capital != Capital::None;
        }
        letters_to_phones(a, out, prothetic);
        for (size_t k = 0; k < out.nuclei.size(); ++k) {
            size_t letter = static_cast<size_t>(out.letter[static_cast<size_t>(out.nuclei[k])]);
            if (letter < a.notation_letters && (a.flags[letter] & F_STRESS)) {
                out.stress = static_cast<int>(k);    // the last accent of a compound is the main one
            }
        }
        if (out.stress < 0) {
            out.stress = default_stress(a.plain, out, prothetic, capital);
            if (!found) out.from_lexicon = false;
        }
        finish_word(a, out);
    }

    // The analysis's letters may differ from the word's (ə, w are the
    // word's e, l): the letter indexes refer to the spelling as given and
    // the plain letters match it one to one, so nothing to map.
    // The stressed flag itself is set by the Frontend, which knows whether
    // the word is a clitic
    for (Phone& p : out.phones) p.accent = Accent::None;
}

// =============================================================================
// Sandhi
// =============================================================================

namespace {

Ph voiced_of(Ph ph) {
    switch (ph) {
        case Ph::P: return Ph::B;
        case Ph::T: return Ph::D;
        case Ph::K: return Ph::G;
        case Ph::S: return Ph::Z;
        case Ph::SH: return Ph::ZH;
        case Ph::F: return Ph::F;       // no voiced partner in Slovenian (v is a sonorant)
        case Ph::C: return Ph::DZ;
        case Ph::CH: return Ph::DZH;
        case Ph::TJ: return Ph::DJ;
        case Ph::H: return Ph::GH;
        case Ph::SJ: return Ph::ZJ;
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
        case Ph::DZ: return Ph::C;
        case Ph::DZH: return Ph::CH;
        case Ph::DJ: return Ph::TJ;
        case Ph::GH: return Ph::H;
        case Ph::ZJ: return Ph::SJ;
        default: return ph;
    }
}

} // namespace

void assimilate(std::vector<Phone>& ph, const std::vector<bool>& word_is_clitic) {
    auto bound = [&](size_t i) {
        if (ph[i].word == ph[i + 1].word) return true;
        return word_is_clitic[ph[i].word] || word_is_clitic[ph[i + 1].word];
    };

    // Final devoicing: an obstruent at the end of a word, before a pause or
    // before a word that begins with anything but a voiced obstruent, is
    // voiceless (grad [grat], mož [moš], grozd [grost]); before a voiced
    // obstruent of the next word it stays voiced, and so do the obstruents
    // before it (Slovenski pravopis 2001, § 650-653). The clitic
    // prepositions (v, z, k, od, pod) take the voicing of the word they
    // lean on below.
    for (size_t i = 0; i < ph.size(); ++i) {
        const bool last_in_word = i + 1 == ph.size() || ph[i + 1].word != ph[i].word;
        if (!last_in_word || !is_obstruent(ph[i].ph) || !ph_def(ph[i].ph).voiced) continue;
        const bool before_voiced_obstruent = i + 1 < ph.size() && is_obstruent(ph[i + 1].ph) &&
                                             ph_def(ph[i + 1].ph).voiced;
        if (before_voiced_obstruent) continue;
        // A preposition is spoken as part of the word after it: od orača,
        // iz okna, ob materi keep the d, z, b (§ 678)
        if (i + 1 < ph.size() && word_is_clitic[ph[i].word] && !word_is_clitic[ph[i + 1].word] &&
            !is_obstruent(ph[i + 1].ph)) {
            continue;
        }
        for (size_t k = i + 1; k-- > 0;) {
            if (!is_obstruent(ph[k].ph) || ph[k].word != ph[i].word) break;
            ph[k].ph = voiceless_of(ph[k].ph);
        }
    }

    for (size_t i = ph.size(); i-- > 1;) {
        Phone& left = ph[i - 1];
        const Phone& right = ph[i];
        if (!bound(i - 1)) continue;

        // Regressive voicing assimilation between obstruents (odkod
        // [otkot], s bratom [zbratom], k gradu written h gradu)
        if (is_obstruent(left.ph) && is_obstruent(right.ph)) {
            left.ph = ph_def(right.ph).voiced ? voiced_of(left.ph) : voiceless_of(left.ph);
        }
        // s, z before š, ž, č, dž are š, ž (s časom [ščasom], iz šole
        // [išole]: § 696)
        if ((left.ph == Ph::S || left.ph == Ph::Z) &&
            (right.ph == Ph::SH || right.ph == Ph::ZH || right.ph == Ph::CH || right.ph == Ph::DZH)) {
            left.ph = left.ph == Ph::S ? Ph::SH : Ph::ZH;
        }
        // Nasal place: n before k, g, h is velar (Anka, Angela, Anhovo: § 638)
        if (left.ph == Ph::N && (right.ph == Ph::K || right.ph == Ph::G || right.ph == Ph::H ||
                                 right.ph == Ph::GH)) {
            left.ph = Ph::NG;
        }
    }

    // Two identical consonants across a clitic boundary merge (iz sela, od
    // toga, s sestro)
    for (size_t i = 0; i + 1 < ph.size();) {
        if (ph[i].ph == ph[i + 1].ph && is_obstruent(ph[i].ph) &&
            ph[i].word != ph[i + 1].word && bound(i)) {
            ph[i + 1].word_start = ph[i].word_start || ph[i + 1].word_start;
            ph.erase(ph.begin() + static_cast<std::ptrdiff_t>(i));
        } else {
            ++i;
        }
    }
}

} // namespace slovenian
} // namespace formant
} // namespace laprdus
