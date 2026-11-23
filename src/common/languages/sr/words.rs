//! The function words of Serbian, the names of the symbols and of the
//! letters.

use crate::text::frontend::LanguageWords;
use crate::text::frontend::letters::{eq, is_one_of};

/// Words that lean on the next one.
pub const PROCLITICS: &[&str] = &[
    "u", "na", "o", "po", "za", "iz", "od", "do", "sa", "s", "k", "ka", "uz", "niz", "bez", "kod", "pod", "nad",
    "pred", "pri", "kroz", "i", "a", "ni", "da", "pa", "te", "no", "ne", "al",
];

/// Words that lean on the one before them.
pub const ENCLITICS: &[&str] = &[
    "sam", "si", "je", "smo", "ste", "su", "ću", "ćeš", "će", "ćemo", "ćete", "bih", "bi", "bismo", "biste", "me",
    "te", "se", "ga", "ju", "ih", "mi", "ti", "mu", "joj", "im", "nam", "vam", "li",
];

/// Cardinal numbers after which a thousand or million word stands in the
/// genitive plural (pet tisuća, dvadeset hiljada, sto milijuna).
pub const COUNTING_NUMBERS: &[&str] = &[
    "pet",
    "šest",
    "sedam",
    "osam",
    "devet",
    "deset",
    "jedanaest",
    "dvanaest",
    "trinaest",
    "četrnaest",
    "petnaest",
    "šesnaest",
    "sedamnaest",
    "osamnaest",
    "devetnaest",
    "dvadeset",
    "trideset",
    "četrdeset",
    "pedeset",
    "šezdeset",
    "sedamdeset",
    "osamdeset",
    "devedeset",
    "sto",
    "stotinu",
    "dvjesto",
    "dvjesta",
    "dvesta",
    "tristo",
    "trista",
    "četiristo",
    "petsto",
    "šeststo",
    "sedamsto",
    "osamsto",
    "devetsto",
];

/// Stressed, but weaker than content words.
pub const FUNCTION_WORDS: &[&str] = &[
    "ali", "ili", "ako", "kad", "kada", "dok", "jer", "nego", "kao", "koji", "koja", "koje", "kojeg", "kojem", "kojoj",
    "koju", "ovaj", "ova", "ovo", "taj", "ta", "to", "onaj", "ona", "ono", "on", "oni", "one", "ja", "vi", "moj",
    "moja", "moje", "tvoj", "tvoja", "tvoje", "njegov", "njen", "naš", "vaš", "svoj", "svoja", "svoje", "sve", "svi",
    "još", "već", "samo", "bio", "bila", "bilo", "bili", "biti", "jesam", "jesi", "jest", "jeste", "jesmo", "jesu",
    "nije", "nisam", "nisi", "nismo", "niste", "nisu", "neće", "neću", "hoću", "hoće", "može", "mogu", "treba", "ima",
    "nema", "zato", "tako", "tu", "oko", "preko", "prema", "poslije", "posle", "prije", "pre", "nakon", "između",
    "među", "protiv", "zbog", "radi", "osim", "iznad", "ispod", "ispred", "iza", "pokraj", "pored", "blizu", "mene",
    "tebe", "sebe", "njega", "nje", "njih", "nas", "vas", "što", "šta", "neki", "neka", "neko", "vrlo", "jako", "tež",
];

/// Words that open a wh-question.
pub const QUESTION_WORDS: &[&str] = &[
    "tko", "ko", "što", "šta", "gdje", "gde", "kada", "kad", "kako", "zašto", "koliko", "koji", "koja", "koje",
    "kojeg", "kojem", "koju", "čiji", "čija", "čije", "kamo", "kuda", "odakle", "otkud", "čime", "čega", "čemu",
    "kome", "koga", "kim", "kakav", "kakva", "kakvo",
];

/// Verb forms that hand their accent to a preceding "ne" (nè znām, nè mogu).
/// Croatian does this with monosyllables only.
pub const NE_SHIFT_VERBS: &[&str] = &[
    "mogu", "može", "možeš", "znamo", "znate", "znaju", "želim", "želi", "volim", "voli", "vidim", "vidi", "radim",
    "radi", "treba", "smijem", "smije", "smem", "sme", "dajem", "daje", "kažem", "kaže", "mislim", "misli", "idem",
    "ide", "čujem", "čuje", "piše", "čita", "pada", "boli", "valja", "mari", "brini", "diraj", "pitaj",
];

/// Two-letter words written vowel + consonant that are real words, so an
/// all-caps "ON" or "IZ" is not spelled out like "IT" or "UK".
const SHORT_VC_WORDS: &[&str] =
    &["on", "od", "uz", "iz", "ih", "im", "um", "ah", "oh", "eh", "uh", "al", "il", "ej", "aj", "oj", "uf", "as", "ar"];

pub fn is_short_vc_word(w: &[char]) -> bool {
    is_one_of(w, SHORT_VC_WORDS)
}

/// Symbols that are read by name wherever they stand: the symbol and its
/// Serbian name. The marks are here for the cases where the clause
/// splitter leaves them inside a clause: glued to the next character, as in
/// "datoteka.txt", "3.14", "12:30" (synthesizer B reads them the same way; a
/// comma, semicolon or question mark inside a word is silent there and here).
const SYMBOLS: [(char, &str); 12] = [
    ('.', "tačka"),
    // The decimal point, from the number converter
    ('\u{2024}', "tačka"),
    (':', "dvotačka"),
    ('!', "uzvičnik"),
    ('%', "posto"),
    ('&', "i"),
    ('+', "plus"),
    ('=', "jednako"),
    ('@', "et"),
    ('€', "evra"),
    ('$', "dolara"),
    ('°', "stepeni"),
];

/// Names of the letters, by decision 41 of the Board for the Standardization
/// of the Serbian Language (the same names for Cyrillic and Latin): a, be,
/// ce, če, će, de, dže, đe, e, ef, ge, ha, i, je, ka, el, lje, em, en, nje,
/// o, pe, er, es, ša, te, u, ve, ze, že; lje, nje and ša where Croatian and
/// Bosnian have elj, enj and eš. The foreign letters: ku, duplo ve, iks,
/// ipsilon (the reading of w by sounds, duplo və, is the tokenizer's).
const LETTER_NAMES: [(&str, &str); 34] = [
    ("a", "a"),
    ("b", "be"),
    ("c", "ce"),
    ("č", "če"),
    ("ć", "će"),
    ("d", "de"),
    ("dž", "dže"),
    ("đ", "đe"),
    ("e", "e"),
    ("f", "ef"),
    ("g", "ge"),
    ("h", "ha"),
    ("i", "i"),
    ("j", "je"),
    ("k", "ka"),
    ("l", "el"),
    ("lj", "lje"),
    ("m", "em"),
    ("n", "en"),
    ("nj", "nje"),
    ("o", "o"),
    ("p", "pe"),
    ("r", "er"),
    ("s", "es"),
    ("š", "ša"),
    ("t", "te"),
    ("u", "u"),
    ("v", "ve"),
    ("z", "ze"),
    ("ž", "že"),
    ("q", "ku"),
    ("w", "duplo ve"),
    ("x", "iks"),
    ("y", "ipsilon"),
];

/// The name of a letter; empty for anything that is not a letter.
pub fn letter_name(letter: &[char]) -> &'static str {
    LETTER_NAMES.iter().find(|(known, _)| eq(letter, known)).map_or("", |&(_, name)| name)
}

/// What the shared tokenizer asks of Serbian.
pub struct Words;

impl LanguageWords for Words {
    fn symbol_name(&self, c: char) -> Option<&'static str> {
        SYMBOLS.iter().find(|&&(symbol, _)| symbol == c).map(|&(_, name)| name)
    }

    fn letter_name(&self, letter: &[char]) -> &'static str {
        letter_name(letter)
    }

    fn is_proclitic(&self, w: &[char]) -> bool {
        is_one_of(w, PROCLITICS)
    }

    fn is_enclitic(&self, w: &[char]) -> bool {
        is_one_of(w, ENCLITICS)
    }

    fn is_consonant_preposition(&self, w: &[char]) -> bool {
        is_one_of(w, &["s", "k"])
    }

    fn is_short_word(&self, w: &[char]) -> bool {
        is_short_vc_word(w)
    }
}
