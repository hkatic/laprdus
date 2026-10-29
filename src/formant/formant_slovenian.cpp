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
// which is the spelling by sounds). The letters of other alphabets: kú,
// dvojni vé, íks, ípsilon; ć and đ are mehki čé and mehki džé, dž is trdi
// džé. The pronunciation of the names is in HAND_ENTRIES.
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
    {"0", u8"nič"}, {"1", "ena"}, {"2", "dva"}, {"3", "tri"}, {"4", u8"štiri"},
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

// Stressed, but weaker than content words.
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
        U"bil", U"bila", U"bilo", U"bili", U"bile", U"biti", U"bodi", U"bodite",
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

// Hand-written entries: numbers, the days, months and the words every
// sentence has, with the accents of SSKJ and the Slovenski pravopis
// (Fran.si). These win over the generated table below.
const char* const HAND_ENTRIES[] = {
    // ---- Numbers (SSKJ and SP 2001: nìč, èn êna êno, dvá, dvé, tríje, trí,
    // štírje, štíri, pét, šést, sédem [sédəm], ósem [ósəm], devét, desét,
    // enájst, dvánajst ... devétnajst (SSKJ² lists the first syllable
    // first, "dvánajst in dvanájst", SP 2001 only -nájst; the first
    // syllable is what the Slovenian listener expected, and it is where
    // Sloleks has the ordinals, dvánajsti: stems, so every form agrees),
    // dvájset, trídeset, štírideset, pétdeset, šéstdeset,
    // sédemdeset, ósemdeset, devétdeset, stó, dvésto, trísto, štíristo,
    // pétsto, šéststo, sédemsto, ósemsto, devétsto, tísoč, milijón,
    // milijárda; the compounds ênaindvájset, pétindvájset have an accent on
    // each part: the front end reads them as three words) ----
    u8"nìč", u8"èn", u8"êna", u8"êno", u8"dvá", u8"dvé", u8"trí", u8"tríje", u8"štíri",
    u8"štírje", u8"pét", u8"šést", u8"sédəm", u8"ósəm", u8"devét", u8"desét",
    u8"enájst*", u8"dvánajst*", u8"trínajst*", u8"štírinajst*", u8"pétnajst*", u8"šéstnajst*",
    u8"sédəmnajst*", u8"ósəmnajst*", u8"devétnajst*",
    u8"dvájset", u8"trídeset", u8"štírideset", u8"pétdeset", u8"šéstdeset",
    u8"sédəmdeset", u8"ósəmdeset", u8"devétdeset",
    u8"stó", u8"dvésto", u8"trísto", u8"štíristo", u8"pétsto", u8"šéststo",
    u8"sédəmsto", u8"ósəmsto", u8"devétsto", u8"tísoč",
    u8"milijón|a|i|ov|u|e|om|ih|om|oma|e", u8"milijárd|a|i|e|o|am|ah|ami",
    u8"bilijón|a|i|ov|u|e|om|ih", u8"bilijárd|a|i|e|o", u8"trilijón|a|i|ov|u|e|om|ih",
    u8"trilijárd|a|i|e|o", u8"kvadrilijón|a|i|ov", u8"kvintilijón|a|i|ov",
    // ordinals: pŕvi, drúgi, trétji, četŕti, péti, šésti, sédmi, ósmi, devéti, deséti
    u8"pŕv*", u8"drúg*", u8"trétj*", u8"četŕt*", u8"pêt|i|a|o|ega|emu|em|im|ih|imi",
    u8"šêst|i|a|o|ega|emu|em|im|ih|imi", u8"sêdm*", u8"ósm*", u8"devêt|i|a|o|ega|emu|em|im|ih|imi",
    u8"desêt|i|a|o|ega|emu|em|im|ih|imi", u8"stót*", u8"tísoč|i|a|o|ega|emu|em",
    u8"obá", u8"obé", u8"dvóje", u8"tróje", u8"pôl", u8"polovíc*", u8"četŕt*",
    u8"vêjica", u8"píka", u8"céla", u8"cél*", u8"odstótk*", u8"odstótek", u8"évr*", u8"évro",
    u8"dólar|ja|ji|jev|jem|jih|", u8"stopínj*", u8"stopínja",
    // ---- Days, months, time ----
    u8"ponedéljək", u8"ponedéljk*", u8"tôrək", u8"tórk*", u8"sréda", u8"sréd*",
    u8"četŕtək", u8"četŕtk*", u8"pétək", u8"pétk*", u8"sobóta", u8"sobót*",
    u8"nedélja", u8"nedélj*",
    u8"jánuar*", u8"fébruar*", u8"márəc", u8"márc*", u8"apríl*", u8"máj", u8"mája", u8"máju",
    u8"júnij*", u8"júlij*", u8"avgúst*", u8"septémbər", u8"septémbr*", u8"októbər",
    u8"októbr*", u8"novémbər", u8"novémbr*", u8"decémbər", u8"decémbr*",
    u8"dán", u8"dnéva", u8"dnévi", u8"dní", u8"dnévu", u8"dnévom", u8"dnéve", u8"dnévih",
    u8"úra", u8"úre", u8"úri", u8"úro", u8"úr", u8"úrah", u8"úram", u8"minúta", u8"minút*",
    u8"sekúnda", u8"sekúnd*", u8"tédən", u8"tédn*", u8"mésec", u8"mésəc", u8"mésec*",
    u8"léto", u8"léta", u8"létu", u8"létom", u8"lét", u8"létih", u8"létom", u8"létoma",
    u8"dánəs", u8"včéraj", u8"jútri", u8"zjútraj", u8"zvečér", u8"opôldne",
    u8"dopôldne", u8"popôldne", u8"ponôči", u8"zdàj", u8"sedàj", u8"potém", u8"nató",
    u8"kmálu", u8"pôzno", u8"zgódaj", u8"védno", u8"nikóli", u8"vsákokrat",
    // ---- Greetings and the most frequent words ----
    u8"dóbər", u8"dôbro", u8"dôbra", u8"dôbri", u8"dôbre", u8"dôbrega", u8"dôbremu",
    u8"dóbər", u8"jútro", u8"večér", u8"nóč", u8"žívjo", u8"zdrávo", u8"adíjo",
    u8"nasvídenje", u8"hvála", u8"prósim", u8"oprostíte", u8"oprôsti", u8"izvólite",
    u8"dobrodôšli", u8"dobrodôšla", u8"dobrodôšəl", u8"prijétno", u8"prijétən", u8"já", u8"dà", u8"nè", u8"sevéda", u8"mordà", u8"mogóče", u8"verjétno",
    u8"člôvek", u8"človéka", u8"človéku", u8"človékom", u8"ljudjé", u8"ljudí", u8"ljudém",
    u8"ljudmí", u8"ljudéh", u8"otròk", u8"otrôka", u8"otrôci",
    u8"žénska", u8"žénsk*", u8"móški", u8"môšk*", u8"déklica", u8"déklic*", u8"fànt",
    u8"fánt*", u8"máma", u8"mám*", u8"ôče", u8"očéta", u8"očétu", u8"očétom", u8"očéti",
    u8"bràt", u8"bráta", u8"brátje", u8"séstra", u8"séstr*", u8"sín", u8"sinôvi",
    u8"hčí", u8"hčére", u8"hčéri", u8"družína", u8"družín*", u8"prijátelj*", u8"prijáteljica",
    u8"híša", u8"híš*", u8"dóm", u8"domá", u8"dómu", u8"mésto", u8"mésta", u8"méstu",
    u8"méstom", u8"mêsti", u8"mést", u8"vàs", u8"vasí", u8"dežêla", u8"dežêl*",
    u8"držáva", u8"držáv*", u8"svét", u8"svetá", u8"svétu", u8"svétom", u8"zêmlja",
    u8"zêmlj*", u8"vôda", u8"vôd*", u8"zràk", u8"zráka", u8"sónce", u8"sónc*", u8"nebó", u8"lúna", u8"zvézda", u8"zvézd*", u8"gôra", u8"gôr*", u8"mórje",
    u8"mórj*", u8"réka", u8"rék*", u8"gózd", u8"gôzda", u8"césta", u8"cést*",
    u8"délo", u8"déla", u8"délu", u8"délom", u8"dél", u8"délati", u8"délam", u8"délaš",
    u8"déla", u8"délamo", u8"délate", u8"délajo", u8"délaw", u8"délala", u8"délali",
    u8"délalo", u8"délale", u8"délaj", u8"délajte", u8"šóla", u8"šól*", u8"knjíga",
    u8"knjíg*", u8"béseda", u8"beséd*", u8"jêzik", u8"jezík*", u8"stávek", u8"stávk*",
    u8"imé", u8"iména", u8"iménu", u8"iménom", u8"iména", u8"imén",
    u8"čàs", u8"čása", u8"čásu", u8"čásom", u8"čási", u8"čásov", u8"ljubézen", u8"ljubézn*",
    u8"življênje", u8"življênj*", u8"smŕt", u8"smŕti", u8"zdrávje", u8"zdrávj*",
    u8"denár", u8"denárja", u8"denárju", u8"denárjem", u8"céna", u8"cén*", u8"trgovína",
    u8"trgovín*", u8"ávto", u8"ávta", u8"ávtu", u8"ávtom", u8"ávti", u8"ávtov",
    u8"avtomobíl", u8"avtomobíl*", u8"vlák", u8"vláka", u8"vláku", u8"letálo", u8"letál*", u8"telefón", u8"telefón*", u8"računálnik",
    u8"računálnik*", u8"ínternet", u8"ínternet*", u8"sporočílo", u8"sporočíl*", u8"gúmb*", u8"ôkno", u8"ôkn*", u8"okênce", u8"menú", u8"menúj*", u8"səznàm",
    u8"səznám*", u8"datotéka", u8"datoték*", u8"mápa", u8"máp*",
    u8"besedílo", u8"besedíl*", u8"strán", u8"stráni", u8"strán*", u8"povezáva", u8"povezáv*",
    u8"nastavítve", u8"nastavítev", u8"nastavíte", u8"nastavítv*", u8"glás", u8"glasú",
    u8"glásu", u8"glásovi", u8"hitrôsti", u8"víšina", u8"višín*",
    u8"glásnost", u8"glasnôsti", u8"jákost", u8"jakôsti", u8"premòr", u8"premôr*",
    u8"slovár", u8"slovárj*", u8"izgovorjáva", u8"izgovorjáv*", u8"naglàs", u8"naglása", u8"naglás*", u8"čŕk|a|e|i|o||am|ah|ami", u8"črkováti", u8"črkúj*", u8"črkovánj*", u8"črkován*",
    u8"ločílo", u8"ločíl*", u8"števílka", u8"števílk*", u8"števílo", u8"števíl*",
    u8"velíko", u8"vêlik", u8"májhən", u8"májhna", u8"májhno", u8"májhni",
    u8"májhne", u8"májhnega", u8"májhnemu", u8"máli", u8"mála", u8"málo", u8"nòv", u8"nôva",
    u8"nôvo", u8"nôvi", u8"nôve", u8"nôvega", u8"nôvemu", u8"nôvih", u8"nôvim", u8"stàr",
    u8"stára", u8"stáro", u8"stári", u8"stáre", u8"stárega", u8"stáremu", u8"mlád",
    u8"mláda", u8"mládo", u8"mládi", u8"mláde", u8"mládega", u8"lép", u8"lépa", u8"lépo",
    u8"lépi", u8"lépe", u8"lépega", u8"lépemu", u8"gŕd", u8"gŕda", u8"gŕdo", u8"gŕdi",
    u8"dôwg", u8"dôwga", u8"dôwgo", u8"dôwgi", u8"dôwge", u8"krátək", u8"krátka", u8"krátko",
    u8"krátki", u8"krátke", u8"visòk", u8"visôka", u8"visôko", u8"visôki", u8"nízək",
    u8"nízka", u8"nízko", u8"nízki", u8"tôpel=tôpəw", u8"tôpla", u8"tôplo", u8"hláden=hládən",
    u8"hládna", u8"hládno", u8"vróč", u8"vróča", u8"vróče", u8"mŕzel=mŕzəw", u8"mŕzla",
    u8"mŕzlo", u8"hítər", u8"hítra", u8"hítro", u8"hítri", u8"hítre", u8"počásən", u8"počásna",
    u8"počásno", u8"počási", u8"glásen=glásən", u8"glásno", u8"glásna", u8"tíh", u8"tího",
    u8"tíha", u8"rdèč", u8"rdéča", u8"rdéče", u8"rdéči", u8"módər", u8"módra", u8"módro",
    u8"zelèn", u8"zelêna", u8"zelêno", u8"zelêni", u8"rumèn", u8"rumêna", u8"rumêno",
    u8"čŕn", u8"čŕna", u8"čŕno", u8"čŕni", u8"bél", u8"béla", u8"bélo", u8"béli", u8"béle",
    u8"sív", u8"síva", u8"sívo", u8"rjàv", u8"rjáva", u8"rjávo",
    // ---- Verbs: biti, imeti, iti, the modals and the commonest verbs ----
    u8"bíti", u8"s'əm", u8"sì", u8"jè", u8"svà", u8"stà", u8"smò", u8"stè", u8"sò",
    u8"nísəm", u8"nísi", u8"ní", u8"nísva", u8"nísta", u8"nísmo", u8"níste", u8"níso",
    u8"bíw", u8"bíla", u8"bílo", u8"bíli", u8"bíle", u8"bodì", u8"bodíte", u8"bóm",
    u8"bóš", u8"bó", u8"bóva", u8"bósta", u8"bómo", u8"bóste", u8"bódo", u8"bí",
    u8"iméti", u8"imám", u8"imáš", u8"imá", u8"imáva", u8"imáta", u8"imámo", u8"imáte",
    u8"imájo", u8"iméw", u8"iméla", u8"imélo", u8"iméli", u8"iméle", u8"imêj", u8"imêjte",
    u8"nímam", u8"nímaš", u8"níma", u8"nímava", u8"nímata", u8"nímamo", u8"nímate", u8"nímajo",
    u8"íti", u8"grém", u8"gréš", u8"gré", u8"gréva", u8"grésta", u8"grémo", u8"gréste",
    u8"gréjo", u8"gredò", u8"šèw", u8"šlà", u8"šlò", u8"šlì", u8"šlè", u8"pójdi",
    u8"pójdite", u8"pójdimo", u8"príti", u8"prídem", u8"prídeš", u8"príde", u8"prídemo",
    u8"prídete", u8"prídejo", u8"prišèw", u8"prišlà", u8"prišlò", u8"prišlì", u8"prídi",
    u8"prídite", u8"hotéti", u8"hóčem", u8"hóčeš", u8"hóče", u8"hóčemo", u8"hóčete",
    u8"hóčejo", u8"hôtel=hôtəw", u8"hotéla", u8"hotélo", u8"hotéli", u8"nóčem", u8"nóčeš",
    u8"nóče", u8"nóčemo", u8"nóčete", u8"nóčejo", u8"môči", u8"mórem", u8"móreš",
    u8"móre", u8"móremo", u8"mórete", u8"mórejo", u8"mógel=mógəw", u8"môgla",
    u8"môglo", u8"môgli", u8"lahkó", u8"mórati", u8"móram",
    u8"móraš", u8"móra", u8"móramo", u8"mórate", u8"mórajo", u8"móral=móraw", u8"mórala",
    u8"mórali", u8"sméti", u8"smém", u8"sméš", u8"smé", u8"smémo", u8"sméte", u8"smêjo",
    u8"védeti", u8"vém", u8"véš", u8"vé", u8"véva", u8"vésta", u8"vémo", u8"véste",
    u8"vêjo", u8"vedó", u8"védel=védəw", u8"védela", u8"védeli", u8"znáti", u8"znám",
    u8"znáš", u8"zná", u8"známo", u8"znáte", u8"znájo", u8"znàw", u8"znála", u8"ználi",
    u8"rêči", u8"rêčem", u8"rêčeš", u8"rêče", u8"rêčemo", u8"rêčete", u8"rêčejo",
    u8"rékel=rékəw", u8"rêkla", u8"rêklo", u8"rêkli", u8"rêci", u8"recíte",
    u8"govoríti", u8"govorím", u8"govoríš", u8"govorí", u8"govorímo", u8"govoríte",
    u8"govoríjo", u8"govóril=govóriw", u8"govoríla", u8"govoríli",
    u8"povédati", u8"povém", u8"povéš", u8"pové", u8"povémo", u8"povéste", u8"povéjo",
    u8"povédal=povédaw", u8"povédala", u8"povédali", u8"povéj", u8"povéjte",
    u8"vídeti", u8"vídim", u8"vídiš", u8"vídi", u8"vídimo", u8"vídite", u8"vídijo",
    u8"vídel=vídəw", u8"vídela", u8"vídeli", u8"slíšati", u8"slíšim", u8"slíšiš", u8"slíši",
    u8"slíšimo", u8"slíšite", u8"slíšijo", u8"slíšal=slíšaw", u8"slíšala", u8"slíšali",
    u8"poslúšati", u8"poslúšam", u8"poslúšaš", u8"poslúša", u8"poslúšamo", u8"poslúšate",
    u8"poslúšajo", u8"poslúšal=poslúšaw", u8"poslúšala", u8"poslúšali", u8"poslúšaj",
    u8"poslúšajte", u8"glédati", u8"glédam", u8"glédaš", u8"gléda", u8"glédamo",
    u8"glédate", u8"glédajo", u8"glédal=glédaw", u8"glédala", u8"glédali", u8"glêj",
    u8"glêjte", u8"bráti", u8"bêrem", u8"bêreš", u8"bêre", u8"bêremo", u8"bêrete",
    u8"bêrejo", u8"brál=bráw", u8"brála", u8"bráli", u8"bêri", u8"beríte", u8"písati",
    u8"píšem", u8"píšeš", u8"píše", u8"píšemo", u8"píšete", u8"píšejo", u8"písal=písaw",
    u8"písala", u8"písali", u8"píši", u8"píšite", u8"mísliti", u8"míslim", u8"mísliš",
    u8"mísli", u8"míslimo", u8"míslite", u8"míslijo", u8"míslil=mísliw", u8"míslila",
    u8"míslili", u8"razuméti", u8"razúmem", u8"razúmeš", u8"razúme", u8"razúmemo",
    u8"razúmete", u8"razúmejo", u8"razúmel=razúməw", u8"razuméla", u8"razuméli",
    u8"délati", u8"naredíti", u8"naredím", u8"naredíš", u8"naredímo",
    u8"naredíte", u8"naredíjo", u8"narédil=narédiw", u8"naredíla", u8"naredíli",
    u8"dáti", u8"dám", u8"dáš", u8"dá", u8"dámo", u8"dáste", u8"dájo", u8"dál=dáw",
    u8"dála", u8"dáli", u8"dàj", u8"dájte", u8"vzéti", u8"vzámem", u8"vzámeš", u8"vzáme",
    u8"vzámemo", u8"vzámete", u8"vzámejo", u8"vzél=vzéw", u8"vzéla", u8"vzéli", u8"vzêmi",
    u8"vzemíte", u8"dobíti", u8"dobím", u8"dobíš", u8"dobí", u8"dobímo", u8"dobíte",
    u8"dobíjo", u8"dobíw", u8"dobíla", u8"dobíli", u8"najti=nájti", u8"nájdem",
    u8"nájdeš", u8"nájde", u8"nájdemo", u8"nájdete", u8"nájdejo", u8"nášel=nášəw",
    u8"nášla", u8"nášli", u8"iskáti", u8"íščem", u8"íščeš", u8"íšče", u8"íščemo", u8"íščete",
    u8"íščejo", u8"iskàw", u8"iskála", u8"iskáli", u8"íšči", u8"íščite",
    u8"odpréti", u8"odprèm", u8"odprèš", u8"odprè", u8"odprêmo", u8"odprête", u8"odprêjo",
    u8"odpŕl=odpŕw", u8"odpŕla", u8"odpŕli", u8"odprì", u8"odpríte", u8"zapréti",
    u8"zaprèm", u8"zaprèš", u8"zaprè", u8"zaprêmo", u8"zaprête", u8"zaprêjo",
    u8"zapŕl=zapŕw", u8"zapŕla", u8"zapŕli", u8"zaprì", u8"zapríte", u8"izbráti",
    u8"izbêrem", u8"izbêreš", u8"izbêre", u8"izbêremo", u8"izbêrete", u8"izbêrejo",
    u8"izbrál=izbráw", u8"izbrála", u8"izbráli", u8"izbêri", u8"izberíte", u8"pritísniti",
    u8"pritísnem", u8"pritísneš", u8"pritísne", u8"pritísnemo", u8"pritísnete",
    u8"pritísnejo", u8"pritísnil=pritísniw", u8"pritísnila", u8"pritísnili", u8"pritísni",
    u8"pritísnite", u8"klíkniti", u8"klíknem", u8"klíkneš", u8"klíkne", u8"klíkni",
    u8"klíknite", u8"vnêsti", u8"vnêsem", u8"vnêseš", u8"vnêse", u8"vnêsemo", u8"vnêsete",
    u8"vnêsejo", u8"vnésel=vnésəw", u8"vnêsla", u8"vnêsli", u8"vnêsi", u8"vnesíte",
    u8"shraníti", u8"shránim", u8"shrániš", u8"shráni", u8"shránimo", u8"shránite",
    u8"shránijo", u8"shránil=shrániw", u8"shraníla", u8"shraníli", u8"izbrísati",
    u8"izbríšem", u8"izbríšeš", u8"izbríše", u8"izbríši", u8"izbríšite", u8"nadaljeváti",
    u8"nadaljújem", u8"nadaljúješ", u8"nadaljúje", u8"nadaljújemo", u8"nadaljújete",
    u8"nadaljújejo", u8"nadaljúj", u8"nadaljújte", u8"ustáviti", u8"ustávim", u8"ustáviš",
    u8"ustávi", u8"ustávite", u8"počákati", u8"počákam", u8"počákaj", u8"počákajte",
    u8"potrdíti", u8"potŕdim", u8"potŕdiš", u8"potŕdi", u8"potŕdite", u8"preklícati",
    u8"preklíčem", u8"preklíči", u8"preklíčite", u8"zapustíti", u8"zapustím",
    u8"zapustíte", u8"vrníti", u8"vŕnem", u8"vŕneš", u8"vŕne", u8"vŕni", u8"vŕnite",
    u8"státi", u8"stojím", u8"stojíš", u8"stojí", u8"stojímo", u8"stojíte", u8"stojíjo",
    u8"sedéti", u8"sedím", u8"sedíš", u8"sedí", u8"sedímo", u8"sedíte", u8"sedíjo",
    u8"ležáti", u8"ležím", u8"ležíš", u8"leží", u8"spáti", u8"spím", u8"spíš", u8"spí",
    u8"spímo", u8"spíte", u8"spíjo", u8"jésti", u8"jém", u8"jéš", u8"jé", u8"jémo",
    u8"jéste", u8"jéjo", u8"jédel=jédəw", u8"jédla", u8"jédli", u8"píti", u8"píjem",
    u8"píješ", u8"píje", u8"píjemo", u8"píjete", u8"píjejo", u8"píl=píw", u8"píla", u8"píli",
    u8"ljúbiti", u8"ljúbim", u8"ljúbiš", u8"ljúbi", u8"ljúbimo", u8"ljúbite", u8"ljúbijo",
    u8"ràd", u8"ráda", u8"rádo", u8"rádi", u8"ráde", u8"živím", u8"živíš", u8"živí",
    u8"živímo", u8"živíte", u8"živíjo", u8"živéti", u8"žível=žívəw", u8"živéla", u8"živéli",
    u8"iméti", u8"posláti", u8"póšljem", u8"póšlješ", u8"póšlje", u8"póšljemo", u8"póšljete",
    u8"póšljejo", u8"poslàw", u8"poslála", u8"posláli", u8"pôšlji", u8"pošljíte",
    u8"kupíti", u8"kúpim", u8"kúpiš", u8"kúpi", u8"kúpimo", u8"kúpite", u8"kúpijo",
    u8"kúpil=kúpiw", u8"kupíla", u8"kupíli", u8"prodáti", u8"prodám", u8"prodáš", u8"prodá",
    u8"plačáti", u8"pláčam", u8"pláčaš", u8"pláča", u8"pláčamo", u8"pláčate", u8"pláčajo",
    u8"pláčal=pláčaw", u8"plačála", u8"plačáli", u8"igráti", u8"igrám", u8"igráš", u8"igrá",
    u8"igrámo", u8"igráte", u8"igrájo", u8"igràw", u8"igrála", u8"igráli",
    u8"pomágati", u8"pomágam", u8"pomágaš", u8"pomága", u8"pomágamo", u8"pomágate",
    u8"pomágajo", u8"pomágal=pomágaw", u8"pomágala", u8"pomágali", u8"pomágaj", u8"pomágajte",
    u8"učíti", u8"učím", u8"učíš", u8"učí", u8"učímo", u8"učíte", u8"učíjo", u8"učíl=učíw",
    u8"učíla", u8"učíli", u8"hodíti", u8"hódim", u8"hódiš", u8"hódi", u8"hódimo", u8"hódite",
    u8"hódijo", u8"hôdiw", u8"hodíla", u8"hodíli", u8"vozíti", u8"vózim", u8"vóziš",
    u8"vózi", u8"vózimo", u8"vózite", u8"vózijo", u8"vôziw", u8"vozíla", u8"vozíli",
    u8"čákati", u8"čákam", u8"čákaš", u8"čáka", u8"čákamo", u8"čákate", u8"čákajo",
    u8"čákal=čákaw", u8"čákala", u8"čákali", u8"čákaj", u8"čákajte", u8"prosíti", u8"prósim",
    u8"prósiš", u8"prósi", u8"prósimo", u8"prósite", u8"prósijo", u8"prôsiw",
    u8"prosíla", u8"prosíli", u8"vprášati", u8"vprášam", u8"vprášaš", u8"vpráša", u8"vprášamo",
    u8"vprášate", u8"vprášajo", u8"vprášal=vprášaw", u8"vprašála", u8"vprašáli", u8"vprášaj",
    u8"odgovoríti", u8"odgovorím", u8"odgovoríš", u8"odgovorí", u8"odgovorímo", u8"odgovoríte",
    u8"odgovoríjo", u8"odgovóril=odgovóriw", u8"odgovoríla", u8"odgovoríli",
    u8"odgôvor", u8"odgôvor*", u8"vprašánje", u8"vprašánj*",
    // ---- Adverbs, pronouns and particles ----
    u8"jàz", u8"tí", u8"òn", u8"ôna", u8"ôno", u8"mí", u8"ví", u8"ôni", u8"ône",
    u8"mídva", u8"vídva", u8"ônadva", u8"tá", u8"tó", u8"tísti", u8"tísta", u8"tísto",
    u8"téga", u8"tému", u8"tém", u8"téh", u8"têj", u8"tó", u8"mój", u8"môja", u8"môje",
    u8"tvój", u8"tvôja", u8"tvôje", u8"njegòv", u8"njegôva", u8"njegôvo", u8"njén",
    u8"njéna", u8"njéno", u8"nàš", u8"náša", u8"náše", u8"vàš", u8"váša", u8"váše",
    u8"svój", u8"svôja", u8"svôje", u8"njíhov", u8"njíhova", u8"njíhovo",
    u8"vsè", u8"vsì", u8"vès", u8"vsà", u8"vsák", u8"vsáka", u8"vsáko", u8"vsákdo", u8"nékaj", u8"nekdó", u8"nihčè", u8"nekatéri", u8"nekatére", u8"néki",
    u8"néka", u8"néko", u8"túdi", u8"sámo", u8"lè", u8"zeló", u8"pràv", u8"takó", u8"tàm",
    u8"tù", u8"túkaj", u8"túdi", u8"potém", u8"zató", u8"tórej", u8"sícer", u8"àmpak",
    u8"vèndar", u8"tóda", u8"kájti", u8"zakáj", u8"kdó", u8"káj", u8"kjé", u8"kám", u8"kód",
    u8"kdáj", u8"kakó", u8"kóliko", u8"katéri", u8"katéra", u8"katéro", u8"katérega",
    u8"katéremu", u8"katérem", u8"katérih", u8"katérim", u8"katére", u8"čigáv", u8"kákšen",
    u8"kákšna", u8"kákšno", u8"kákšni", u8"kákšne", u8"odkód", u8"kómu", u8"kóga", u8"čému",
    u8"čésa", u8"čím", u8"mêne", u8"têbe", u8"sêbe", u8"njêga", u8"njé", u8"njìh", u8"njó",
    u8"njím", u8"njêmu", u8"njêj", u8"náma", u8"váma", u8"náju", u8"váju",
    u8"okóli", u8"okróg", u8"póleg", u8"zráven", u8"mímo", u8"blízu", u8"zaràdi", u8"zarádi",
    u8"gledé", u8"namésto", u8"rázən", u8"kljúb", u8"préko", u8"prék", u8"kót", u8"kàkor",
    u8"kàdar", u8"doklèr", u8"préden", u8"médtem", u8"čepràv", u8"četúdi", u8"sevéda",
    u8"zdàj", u8"tedàj", u8"takràt", u8"nekóč", u8"spét", u8"šè", u8"žé",
    u8"tàm", u8"tjà", u8"gôr", u8"dôl", u8"nótri", u8"zúnaj", u8"znótraj",
    u8"spôdaj", u8"zgôraj", u8"lévo", u8"désno", u8"naprêj", u8"nazáj", u8"navzgòr",
    u8"navzdòl", u8"dôbro", u8"slábo", u8"hítro", u8"počási", u8"glásno", u8"tího",
    u8"dáleč", u8"blízu", u8"vèč", u8"mànj", u8"nájveč", u8"nájmanj", u8"bòlj", u8"nájbolj",
    u8"zeló", u8"precèj", u8"malce=málce", u8"málo", u8"velíko", u8"dôsti", u8"dovòlj",
    u8"prevèč", u8"premálo", u8"skôraj", u8"čísto", u8"popôwnoma", u8"načín*",
    u8"primér", u8"primér*", u8"ozíroma", u8"ítak", u8"pàč",
    u8"sploh=splòh", u8"résno", u8"resníca", u8"resníc*", u8"sevéda", u8"točno=tóčno",
    u8"tóčka", u8"tóčk*", u8"kônəc", u8"kônca", u8"kôncu", u8"kôncem", u8"začétek", u8"začétk*",
    u8"srédina", u8"sredín*", u8"stvár", u8"stvarí", u8"stvár*", u8"réč",
    u8"ljudjé",
    // ---- The names of letters of more than one syllable (the names of one
    // syllable, bé, èl, are read by the rule in finish_word: they would
    // collide with the words je, en, es) ----
    u8"dvójni", u8"íks", u8"ípsilon", u8"mêhki", u8"tŕdi",
    // ---- Place names and nationalities ----
    u8"slovénija", u8"slovénij*", u8"slovénski", u8"slovénsk*", u8"slovénec", u8"slovénc*",
    u8"slovénka", u8"slovénk*", u8"slovénščina", u8"slovénščin*", u8"ljubljána", u8"ljublján*",
    u8"máribor", u8"máribor*", u8"célje", u8"cêlj*", u8"kránj", u8"kránj*", u8"kóper",
    u8"kópər", u8"kópr*", u8"velénje", u8"velênj*", u8"ptúj",
    u8"ptúj*", u8"hrvaška=hrváška", u8"hrvášk*", u8"hrvàt", u8"hrvát*", u8"hrvática",
    u8"hrvatíc*", u8"hrváščina", u8"srbíja", u8"sŕbij*", u8"sŕbski", u8"sŕbsk*", u8"sŕb",
    u8"sŕb*", u8"bósna", u8"bósn*", u8"bosánski", u8"bosánsk*", u8"ávstrija", u8"ávstrij*",
    u8"itálija", u8"itálij*", u8"nemčíja", u8"némčij*", u8"némški", u8"némšk*",
    u8"angléški", u8"anglésk*", u8"angléščina", u8"angléščin*", u8"evrópa", u8"evróp*",
    u8"evrópski", u8"evrópsk*", u8"madžárska", u8"madžársk*", u8"amérika", u8"amêrik*",
    u8"ameríški", u8"amêrišk*", u8"zágreb", u8"zágreb*",
    u8"sarájevo", u8"sárajev*", u8"dúnaj", u8"dúnaj*", u8"tŕst", u8"tŕst*",
    u8"laprdus=láprdus*",
    // ---- Added after the first Slovenian user reports (2026-10-29): seznàm with
    // its schwa, the participles of (po)čistiti, the touch verb tapniti, the
    // dual of narediti; the readings that depend on the place in the clause
    // (žívijo, narêdi) are in HEAD_READINGS ----
    u8"naredíva", u8"naredíta", u8"počíšč*", u8"očíšč*", u8"číšč*", u8"tápn*",
    u8"zaganjálnik*", u8"napajálnik*", u8"êden", u8"ênkrat", u8"domôv",
};

// Spellings whose reading depends on where they stand: at the head of a
// clause (or after a conjunction or an adverb that opens a request) they
// are the greeting, the imperative of a button label or the name of a
// menu, elsewhere the present tense of the verb or another word spelled
// the same (SSKJ, SP 2001: naredíti -ím, imperative narêdi; govoríti -ím,
// govôri; dobíti -ím, dôbi; zapustíti -ím, zapústi; storíti -ím, stôri;
// delíti -ím, dêli; uredíti -ím, urêdi; ponovíti -ím, ponôvi; obnovíti
// -ím, obnôvi; naložíti -ím, nalôži; osvežíti -ím, osvéžil; mení the
// menu, méni the dative; žívijo the greeting, živíjo "they live"). The
// head reading is given here; the other comes from the lexicon.
struct HeadReading {
    const char32_t* spelling;
    const char32_t* head;
};

const HeadReading HEAD_READINGS[] = {
    {U"živijo", U"žívijo"},
    {U"živio", U"žívio"},
    {U"naredi", U"narêdi"},
    {U"govori", U"govôri"},
    {U"odgovori", U"odgovôri"},
    {U"dobi", U"dôbi"},
    {U"zapusti", U"zapústi"},
    {U"spusti", U"spústi"},
    {U"stori", U"stôri"},
    {U"prosi", U"prôsi"},
    {U"deli", U"dêli"},          // the Share button; déli "parts" elsewhere
    {U"uredi", U"urêdi"},
    {U"ponovi", U"ponôvi"},
    {U"obnovi", U"obnôvi"},
    {U"naloži", U"nalôži"},
    {U"osveži", U"osvéži"},
    {U"meni", U"mení"},          // the Menu of an app; méni "to me" elsewhere
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

// Words of one syllable in -l that keep the [l] although the rule for
// monosyllables says [w]: dél (SSKJ: dél -a [tudi deu̯], the [l] first),
// and the loans šál, bál, gél, mól.
bool l_final_l_word(const std::u32string& w) {
    static const WordSet words = {U"del", U"šal", U"bal", U"gel", U"mol"};
    return words.count(w) != 0;
}

struct Analysis {
    std::u32string plain;           // letters
    std::vector<uint8_t> flags;     // per letter
    size_t notation_letters = 0;    // letters that came from the lexicon's notation (the rest by rule)
    bool letter_name = false;
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

const char32_t* head_reading(const std::u32string& w) {
    for (const HeadReading& r : HEAD_READINGS) {
        if (w == r.spelling) return r.head;
    }
    return nullptr;
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
const char* find_in_table(const char* const* table, size_t count, const std::u32string& key);
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
    if (m_forms) {
        if (const char* entry = find_in_table(m_forms, m_form_count, word)) {
            pron = PhonemeMapper::utf8_to_utf32(entry);
            return true;
        }
    }
    return false;
}

bool Lexicon::find_stem(const std::u32string& word, size_t& stem_length, std::u32string& pron) const {
    if (m_stems.empty() && !m_stem_table) return false;
    const size_t min_len = word.size() > MAX_ENDING ? word.size() - MAX_ENDING : 1;
    for (size_t len = word.size(); len >= min_len && len >= 2; --len) {
        const std::u32string stem = word.substr(0, len);
        auto it = m_stems.find(stem);
        if (it != m_stems.end()) {
            stem_length = len;
            pron = it->second;
            return true;
        }
        if (m_stem_table) {
            if (const char* entry = find_in_table(m_stem_table, m_stem_count, stem)) {
                stem_length = len;
                pron = PhonemeMapper::utf8_to_utf32(entry);
                pron.pop_back();     // the '*'
                return true;
            }
        }
    }
    return false;
}

namespace {

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

const char* find_in_table(const char* const* table, size_t count, const std::u32string& key) {
    size_t lo = 0, hi = count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        int c = compare_key(key, table[mid]);
        if (c == 0) return table[mid];
        if (c < 0) hi = mid;
        else lo = mid + 1;
    }
    return nullptr;
}

} // namespace

const Lexicon& builtin_lexicon() {
    static const Lexicon lexicon = [] {
        Lexicon lex;
        lex.set_tables(SLOVENIAN_FORMS, sizeof(SLOVENIAN_FORMS) / sizeof(SLOVENIAN_FORMS[0]),
                       SLOVENIAN_STEMS, sizeof(SLOVENIAN_STEMS) / sizeof(SLOVENIAN_STEMS[0]));
        for (const char* entry : HAND_ENTRIES) {
            lex.add(PhonemeMapper::utf8_to_utf32(entry));
        }
        return lex;
    }();
    return lexicon;
}

void Lexicon::set_tables(const char* const* forms, size_t form_count,
                         const char* const* stems, size_t stem_count) {
    m_forms = forms;
    m_form_count = form_count;
    m_stem_table = stems;
    m_stem_count = stem_count;
}

// =============================================================================
// Stress and the sounds that depend on it
// =============================================================================

namespace {

// Where the stress falls when no dictionary says: the suffix rules of the
// Slovenian word classes whose accent is regular, then the second-last
// syllable, the commonest place in the language. Returns the index into
// `nuclei` (the prothetic u of vsak, vzeti excluded).
int default_stress(const std::u32string& w, const WordPhones& wp, const std::vector<bool>& prothetic) {
    std::vector<int> candidates;     // nucleus indexes that may carry the stress
    for (size_t k = 0; k < wp.nuclei.size(); ++k) {
        if (!prothetic[k]) candidates.push_back(static_cast<int>(k));
    }
    if (candidates.empty()) return -1;
    if (candidates.size() == 1) return candidates[0];

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
    // [l] otherwise, as in the loanwords with the stress on the last
    // syllable (hotél, kanál, avtomobíl, apríl, modél).
    // A notation l follows the rule too (the lexicon writes w only where
    // the rule would say l).
    if (n >= 2 && w.back() == U'l' && !wp.phones.empty() && wp.phones.back().ph == Ph::L &&
        is_vowel_letter(w[n - 2])) {
        const bool final_stress = static_cast<size_t>(wp.stress + 1) == wp.nuclei.size();
        const bool monosyllable = wp.nuclei.size() == 1;
        const bool schwa = wp.phones[static_cast<size_t>(wp.nuclei.back())].ph == Ph::AX;
        const bool participle_shape = w[n - 2] != U'o';
        if (schwa || l_final_w_word(w) ||
            (participle_shape && (monosyllable || !final_stress) && !l_final_l_word(w))) {
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
    if (starts_with(ending, U"išč")) {
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
               const std::u32string* pron_override) {
    out = WordPhones();
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
        if (out.stress < 0) out.stress = default_stress(a.plain, out, prothetic);
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
            stem_length = word.size();
            found = builtin_lexicon().find_exact(word, pron) ||
                    builtin_lexicon().find_stem(word, stem_length, pron);
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
        } else {
            a.plain = word;
            a.flags.assign(word.size(), 0);
            a.notation_letters = 0;
        }
        letters_to_phones(a, out, prothetic);
        for (size_t k = 0; k < out.nuclei.size(); ++k) {
            size_t letter = static_cast<size_t>(out.letter[static_cast<size_t>(out.nuclei[k])]);
            if (letter < a.notation_letters && (a.flags[letter] & F_STRESS)) {
                out.stress = static_cast<int>(k);    // the last accent of a compound is the main one
            }
        }
        if (out.stress < 0) {
            out.stress = default_stress(a.plain, out, prothetic);
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
