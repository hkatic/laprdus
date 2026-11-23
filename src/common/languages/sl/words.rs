//! Slovenian letter, symbol and character names and the classes of its
//! function words (Toporišič, Slovenska slovnica: naslonke; Slovenski
//! pravopis).

use crate::text::frontend::letters::{eq, is_one_of};

/// Slovenska abeceda (Slovenski pravopis 2001, § 7-9, "Imena črk"): the
/// vowels á é í ó ú, the consonants bé cé čé dé ef gé há jé ká èl èm èn pé èr
/// ès èš té vé zé žé (the rules also allow the sound with a schwa, bə, cə,
/// which is the spelling by sounds, and so dvojni və for w: `expand_words()`
/// of the shared tokenizer). The letters of other alphabets: kú, dvojni vé,
/// íks, ípsilon; ć and đ are mehki čé and mehki džé, dž is trdi džé. The
/// pronunciation of the names is in the lexicon.
const LETTER_NAMES: &[(&str, &str)] = &[
    ("a", "a"),
    ("b", "be"),
    ("c", "ce"),
    ("č", "če"),
    ("d", "de"),
    ("e", "e"),
    ("f", "ef"),
    ("g", "ge"),
    ("h", "ha"),
    ("i", "i"),
    ("j", "je"),
    ("k", "ka"),
    ("l", "el"),
    ("m", "em"),
    ("n", "en"),
    ("o", "o"),
    ("p", "pe"),
    ("r", "er"),
    ("s", "es"),
    ("š", "eš"),
    ("t", "te"),
    ("u", "u"),
    ("v", "ve"),
    ("z", "ze"),
    ("ž", "že"),
    ("q", "ku"),
    ("w", "dvojni ve"),
    ("x", "iks"),
    ("y", "ipsilon"),
    ("ć", "mehki če"),
    ("đ", "mehki dže"),
    ("lj", "el je"),
    ("nj", "en je"),
    ("dž", "trdi dže"),
];

/// Symbols read by name where they stand inside a clause: the period that is
/// not a clause end, the colon of a clock time, and the signs that follow a
/// number.
const SYMBOLS: &[(char, &str)] = &[
    ('.', "pika"),
    ('\u{2024}', "pika"), // the decimal point, from the number converter
    (':', "dvopičje"),
    ('!', "klicaj"),
    ('%', "odstotkov"),
    ('&', "in"),
    ('+', "plus"),
    ('=', "enako"),
    ('@', "afna"),
    ('€', "evrov"),
    ('$', "dolarjev"),
    ('°', "stopinj"),
];

/// The names of digits, punctuation and symbols when spelled (the Croatian
/// ones are in `data/dictionary/spelling.json`). Slovenian usage: Slovenski
/// pravopis 2001, "Ločila"; the names of the computer symbols as the
/// Slovenian localizations of NVDA and Windows give them.
const SPELLED: &[(&str, &str)] = &[
    ("0", "nič"),
    ("1", "ena"),
    ("2", "dve"),
    ("3", "tri"),
    ("4", "štiri"),
    ("5", "pet"),
    ("6", "šest"),
    ("7", "sedem"),
    ("8", "osem"),
    ("9", "devet"),
    (" ", "presledek"),
    (".", "pika"),
    (",", "vejica"),
    ("!", "klicaj"),
    ("?", "vprašaj"),
    (";", "podpičje"),
    (":", "dvopičje"),
    ("-", "vezaj"),
    ("_", "podčrtaj"),
    ("'", "opuščaj"),
    ("\"", "narekovaj"),
    ("(", "oklepaj"),
    (")", "zaklepaj"),
    ("[", "oglati oklepaj"),
    ("]", "oglati zaklepaj"),
    ("{", "zaviti oklepaj"),
    ("}", "zaviti zaklepaj"),
    ("/", "poševnica"),
    ("\\", "leva poševnica"),
    ("@", "afna"),
    ("#", "lojtra"),
    ("$", "dolar"),
    ("%", "odstotek"),
    ("^", "strešica"),
    ("&", "in"),
    ("*", "zvezdica"),
    ("+", "plus"),
    ("=", "enačaj"),
    ("<", "manjše"),
    (">", "večje"),
    ("|", "navpičnica"),
    ("~", "tilda"),
    ("`", "krativec"),
    ("€", "evro"),
    ("£", "funt"),
    ("°", "stopinja"),
    ("§", "paragraf"),
    ("–", "pomišljaj"),
    ("—", "dolgi pomišljaj"),
    ("…", "tripičje"),
    ("„", "narekovaj"),
    ("“", "narekovaj"),
    ("”", "narekovaj"),
    ("«", "narekovaj"),
    ("»", "narekovaj"),
    ("‚", "opuščaj"),
    ("‘", "opuščaj"),
    ("’", "opuščaj"),
    ("©", "avtorske pravice"),
    ("®", "registrirana znamka"),
    ("™", "blagovna znamka"),
    ("·", "pika na sredini"),
    ("•", "krogec"),
    ("×", "krat"),
    ("÷", "deljeno"),
    ("±", "plus minus"),
    ("‰", "promil"),
    ("¼", "četrt"),
    ("½", "pol"),
    ("¾", "tri četrt"),
];

/// Proclitics: prepositions and conjunctions without a stress of their own.
const PROCLITICS: &[&str] = &[
    "v", "na", "za", "pri", "po", "s", "z", "k", "h", "o", "ob", "od", "do", "iz", "brez", "čez", "med", "nad", "pod",
    "pred", "skozi", "proti", "zoper", "u", "in", "pa", "ter", "ali", "da", "ki", "ko", "ker", "če", "a", "ne", "naj",
    "saj", "le", "še", "že", "ni", "niti",
];

/// Enclitics: the forms of biti, the future auxiliary, the conditional
/// particle and the short pronouns.
const ENCLITICS: &[&str] = &[
    "sem", "si", "je", "sva", "sta", "smo", "ste", "so", "bom", "boš", "bo", "bova", "bosta", "bomo", "boste", "bodo",
    "bi", "me", "te", "se", "ga", "jo", "ju", "jih", "mi", "ti", "mu", "ji", "jim", "nam", "vam", "nas", "vas", "nju",
];

/// Stressed, but weaker than content words. Not bila, bilo, bili, bile: the
/// participle of biti is a verb with an accent of its own on the last
/// syllable (bilà biló bilí bilè), which as a function word's small movement
/// was lost after the higher bi and heard on the first syllable.
const FUNCTION_WORDS: &[&str] = &[
    "jaz", "ti", "on", "ona", "ono", "mi", "vi", "oni", "one", "midva", "vidva", "onadva", "ta", "to", "tisti",
    "tista", "tisto", "tega", "temu", "tem", "teh", "tej", "moj", "moja", "moje", "tvoj", "tvoja", "tvoje", "njegov",
    "njegova", "njen", "njena", "naš", "naša", "vaš", "vaša", "svoj", "svoja", "svoje", "njihov", "njihova", "vse",
    "vsi", "ves", "vsa", "vsak", "vsaka", "vsako", "nekaj", "nekdo", "nič", "nihče", "nekateri", "neki", "neka",
    "neko", "tudi", "samo", "le", "zelo", "prav", "tako", "tam", "tu", "tukaj", "zdaj", "sedaj", "potem", "nato",
    "zato", "torej", "pa", "sicer", "ampak", "vendar", "toda", "kajti", "zakaj", "bil", "biti", "bodi", "bodite",
    "nisem", "nisi", "ni", "nismo", "niste", "niso", "nisva", "nista", "bova", "bosta", "ima", "imam", "imaš", "imamo",
    "imate", "imajo", "nima", "nimam", "lahko", "mora", "moram", "treba", "more", "morem", "mene", "tebe", "sebe",
    "njega", "nje", "njih", "njo", "njim", "njemu", "njej", "nama", "vama", "naju", "vaju", "okoli", "okrog", "poleg",
    "zraven", "mimo", "blizu", "zaradi", "glede", "namesto", "razen", "kljub", "preko", "prek", "izza", "izpod",
    "iznad", "izpred", "kot", "kakor", "kadar", "dokler", "preden", "medtem", "čeprav", "četudi", "ja", "da", "ne",
    "seveda", "morda", "mogoče", "verjetno",
];

/// Words that open a wh-question.
#[rustfmt::skip] // rows of words
const QUESTION_WORDS: &[&str] = &[
    "kdo", "kaj", "kje", "kam", "kod", "kdaj", "kako", "zakaj", "koliko", "kateri", "katera", "katero", "katerega",
    "kateremu", "katerem", "katerih", "katerim", "katere", "čigav", "čigava", "čigavo", "kakšen", "kakšna", "kakšno",
    "kakšni", "kakšne", "kolikšen", "kolikšna", "odkod", "odkje", "doklej", "komu", "koga", "kom", "čemu", "česa",
    "čim", "kolikokrat",
];

/// The name of a letter in Slovenian ("b" -> "be", "š" -> "eš", "w" ->
/// "dvojni ve"; Slovenski pravopis § 7); empty for anything else. The letter
/// is as `spelling_letter()` returns it.
pub fn letter_name(letter: &[char]) -> &'static str {
    LETTER_NAMES.iter().find(|&&(l, _)| eq(letter, l)).map_or("", |&(_, name)| name)
}

/// The name of a symbol read wherever it stands inside a clause (".", ":",
/// "%": pika, dvopičje, odstotkov); `None` otherwise.
pub fn symbol_name(c: char) -> Option<&'static str> {
    SYMBOLS.iter().find(|&&(symbol, _)| symbol == c).map(|&(_, name)| name)
}

/// The name of a digit, a punctuation mark or a symbol when spelled ("1" ->
/// "ena", "." -> "pika", "@" -> "afna"); `None` for a character without one.
pub fn spelled_name(character: &str) -> Option<&'static str> {
    SPELLED.iter().find(|&&(c, _)| c == character).map(|&(_, name)| name)
}

/// A word that leans on the next one: the monosyllabic prepositions and
/// conjunctions, ne, naj.
pub fn is_proclitic(w: &[char]) -> bool {
    is_one_of(w, PROCLITICS)
}

/// A word that leans on the one before it: the forms of biti and the future,
/// bi, the short pronouns.
pub fn is_enclitic(w: &[char]) -> bool {
    is_one_of(w, ENCLITICS)
}

/// A word stressed more weakly than a content word.
pub fn is_function_word(w: &[char]) -> bool {
    is_one_of(w, FUNCTION_WORDS)
}

/// A word that opens a wh-question (kdo, kaj, kje, kako, zakaj, kateri, ...).
pub fn is_question_word(w: &[char]) -> bool {
    is_one_of(w, QUESTION_WORDS)
}
