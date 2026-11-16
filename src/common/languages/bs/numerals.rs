//! Bosnian numerals in a sentence: the number words, the agreement of one
//! and two with the noun, ordinals in the noun's case, dates, ranges and the
//! nouns of the unit signs (prvo mjesto, na prvom mjestu, sedmog oktobra,
//! jedna kuća, dvije kuće, jedan euro, dva eura, pet stepeni Celzijusa,
//! hiljadu tristo sedamnaest).

use crate::text::numbers::cardinal::{Numerals, ScaleAgreement};
use crate::text::numbers::scan::{
    Link, Out, Text, Unit, UnitAt, is_letter_byte, is_word_byte, join_parts, last_digit, last_token, latin, latin_word,
    part_break, read_glued_minus, say, small_value, starts_clause, strip_zeros,
};
use crate::text::numbers::{Grammar, NumberReader};

/// The Bosnian number words: 1 takes the singular of a scale word (jedan
/// milion, hiljadu), 2 to 4 its special plural (dva miliona, dvije hiljade),
/// 5 and above, 0 and 11 to 19 the genitive plural (pet hiljada, pet
/// miliona); one and two agree with the feminine scale words (dvije hiljade,
/// dvadeset jedna hiljada, dvije milijarde). Bosnian has hiljada, milion,
/// dvjesto.
static NUMERALS: Numerals = Numerals {
    digits: ["nula", "jedan", "dva", "tri", "četiri", "pet", "šest", "sedam", "osam", "devet"],
    two_alone: "dva",
    teens: [
        "",
        "jedanaest",
        "dvanaest",
        "trinaest",
        "četrnaest",
        "petnaest",
        "šesnaest",
        "sedamnaest",
        "osamnaest",
        "devetnaest",
    ],
    tens: [
        "",
        "deset",
        "dvadeset",
        "trideset",
        "četrdeset",
        "pedeset",
        "šezdeset",
        "sedamdeset",
        "osamdeset",
        "devedeset",
    ],
    hundreds: ["", "sto", "dvjesto", "tristo", "četiristo", "petsto", "šesto", "sedamsto", "osamsto", "devetsto"],
    // The accusative for 1000 itself, the nominative plural for 2-4, the
    // genitive plural for 0 and 5-9
    thousand: ["hiljadu", "hiljade", "hiljade", "hiljada"],
    million: ["lion", "liona", "liona", "liona"],
    milliard: ["lijarda", "lijarde", "lijarde", "lijardi"],
    feminine_thousand: true,
    ones_first: false,
    scale_agreement: ScaleAgreement::OneAndTwo { one: "jedna", two: "dvije" },
};

/// The nouns of %, €, $ and °.
static UNITS: [Unit; 4] = [
    Unit::counted("%", "posto", "posto", "posto", "posto"),
    Unit::counted("€", "euro", "eura", "eura", "eura"),
    Unit::counted("$", "dolar", "dolara", "dolara", "dolara"),
    Unit::counted("°", "stepen", "stepena", "stepeni", "stepena"),
];

/// The scale of °C, in the genitive.
const CELSIUS: &str = " Celzijusa";

/// Two before a feminine noun.
const TWO_FEMININE: &str = "dvije";

/// Four in a compound: četvorosobni.
const FOUR_COMPOUND: &str = "četvoro";

// The prepositions by the case they govern
#[rustfmt::skip]
const WITH_GENITIVE: &[&str] = &[
    "od", "do", "iz", "bez", "zbog", "oko", "kod", "pored", "blizu", "između", "izmedju", "poslije", "posle",
    "nakon", "prije", "pre", "osim", "radi", "tijekom", "tokom", "protiv", "ispod", "iznad", "ispred", "iza",
    "preko", "kraj", "pokraj", "umjesto", "umesto", "mimo", "duž", "putem", "pomoću", "unutar",
    "izvan", "van", "nasuprot", "usred", "sred", "s", "sa",
];
const WITH_LOCATIVE: &[&str] = &["u", "na", "o", "pri", "po"];
const WITH_DATIVE: &[&str] = &["prema", "k", "ka", "usprkos", "unatoč"];
const WITH_INSTRUMENTAL: &[&str] = &["s", "sa", "pred", "pod", "nad", "za", "među", "medju"];

/// The words after which a number counts nothing, and the commonest verbs.
#[rustfmt::skip]
const NOT_COUNTED: &[&str] = &[
    "i", "ili", "te", "pa", "a", "ali", "je", "su", "sam", "si", "smo", "ste", "bi", "će", "ću", "ćeš",
    "ćemo", "ćete", "ni", "ne", "da", "li", "se", "ga", "mu", "joj", "im", "ih", "nas", "vas", "od", "do",
    "u", "na", "za", "s", "sa", "o", "po", "pri", "kod", "iz", "bez", "pod", "nad", "pred", "među", "kroz",
    "prema", "plus", "minus", "puta", "posto", "odsto", "više", "manje", "oko", "otprilike", "samo", "već",
    "još", "opet", "ujutro", "navečer", "uvečer", "uveče", "popodne", "poslijepodne", "poslepodne",
    "danas", "sutra", "jučer", "juče", "sada", "sad", "tada", "onda", "ovdje", "ovde", "tamo", "tu", "gdje",
    "gde", "kako", "kada", "kad", "zašto", "jer", "ako", "dok", "nego", "kao", "čak", "isto", "tako",
    "također", "takođe", "zato", "dakle", "naime", "možda", "valjda", "sigurno", "vrlo", "jako", "puno",
    "mnogo", "malo", "dosta", "previše", "premalo", "bar", "barem", "čim", "tek", "upravo", "uopće",
    "uopšte", "zatim", "potom", "ipak", "inače", "dalje", "ponovo", "ponovno", "jedan", "dva", "tri",
    "četiri", "pet", "šest", "sedam", "osam", "devet", "deset", "sto", "tisuću", "hiljadu", "milijun",
    "milion", "ima", "nema", "biti", "bio", "bila", "bilo", "bili", "bile", "jest", "nije", "nisu",
    "jesu", "jesam", "nisam", "budu", "bude", "mogu", "može", "treba", "mora", "moraju", "hoće",
    "jedno", "jedna", "dvije", "dve", "oba", "obje", "obe", "svaki", "svaka", "svako", "svi", "sve", "sva",
];

/// The labels a number names (stranica 2, slika 2): the plain form.
#[rustfmt::skip]
const LABELS: &[&str] = &[
    "stranica", "stranici", "strana", "strani", "slika", "slici", "sliku", "tablica", "tablici", "tabela",
    "tabeli", "grupa", "grupi", "skupina", "skupini", "članak", "članku", "član", "članu", "točka",
    "točki", "tačka", "tački", "stavak", "stavku", "stav", "stavu", "verzija", "verziji", "inačica",
    "inačici", "kanal", "kanalu", "broj", "broju", "napomena", "napomeni", "prilog", "prilogu", "faza",
    "fazi", "korak", "koraku", "element", "red", "redu", "redak", "retku", "stupac", "stupcu", "kolona",
    "koloni", "kartica", "kartici", "gumb", "gumbu", "dugme", "dugmetu", "datoteka", "datoteci", "mapa", "mapi",
    "razina", "razini", "nivo", "nivou", "dio", "deo", "dijelu", "delu", "ulica", "ulici", "cesta", "cesti",
    "linija", "liniji", "autobus", "tramvaj", "soba", "sobi", "kat", "katu", "sprat", "spratu", "poglavlje",
    "poglavlju", "lekcija", "lekciji", "zadatak", "zadatku", "pitanje", "pitanju", "odgovor", "model", "tip",
    "tipka", "tipku", "taster", "pritisni", "pritisnite", "odaberi", "odaberite", "izaberi", "izaberite",
    "unesi", "unesite", "upiši", "upišite", "klikni", "kliknite", "dodirni", "dodirnite",
];

/// The feminine nouns in a consonant (i-stems), the commonest, singular and
/// the forms in -i; and every -ost, -est, -ad.
#[rustfmt::skip]
const FEMININE_I: &[&str] = &[
    "stvar", "riječ", "reč", "noć", "kost", "sol", "so", "krv", "ljubav", "smrt", "moć", "pomoć",
    "obitelj", "misao", "pamet", "kćer", "kći", "vlast", "oblast", "čast", "strast", "mast", "zob",
    "peć", "vijest", "vest", "savjest", "savest", "svijest", "svest", "propast", "korist", "pošast",
    "cijev", "cev", "ćud", "čeljust", "glad", "žeđ", "ravan", "raž", "nit", "zvijer", "zver", "laž",
    "čađ", "kap", "jesen", "večer", "ovlast", "narav", "nakit", "pogibelj", "vrst", "četvrt", "svrha",
    "stvari", "riječi", "reči", "noći", "kosti", "soli", "krvi", "ljubavi", "smrti", "moći", "pomoći",
    "obitelji", "misli", "pameti", "kćeri", "vlasti", "oblasti", "časti", "strasti", "masti", "peći",
    "vijesti", "vesti", "savjesti", "savesti", "svijesti", "svesti", "propasti", "koristi", "cijevi", "cevi",
    "ćudi", "čeljusti", "gladi", "žeđi", "ravni", "raži", "niti", "zvijeri", "zveri", "laži", "kapi",
    "jeseni", "večeri", "ovlasti", "naravi", "četvrti", "djeca", "deca", "braća", "braca", "gospoda",
];

fn is_feminine_i(next: &str) -> bool {
    FEMININE_I.contains(&next)
        || (next.ends_with("ost") && next.len() > 4)
        || (next.ends_with("osti") && next.len() > 5)
        || (next.ends_with("est") && next.len() > 4)
        || ["čeljad", "telad", "momčad", "unučad", "burad", "pilad"].contains(&next)
}

/// Masculine nouns in -o.
#[rustfmt::skip]
const MASCULINE_O: &[&str] = &[
    "auto", "radio", "kino", "euro", "evro", "video", "foto", "metro", "studio", "biro", "kimono", "torzo",
    "dinamo", "tango", "logo", "saldo", "konto", "tempo", "solo", "libreto", "kasino", "kazino",
];

/// The neuter endings in -e.
fn is_neuter_e(next: &str) -> bool {
    !next.ends_with("ije") && ["je", "lje", "nje", "me", "te", "re", "đe"].iter().any(|tail| next.ends_with(tail))
}

// The units written as abbreviations, by the gender of their noun
#[rustfmt::skip]
const MASCULINE_UNITS: &[&str] = &[
    "m", "cm", "mm", "dm", "km", "kg", "g", "dag", "dkg", "mg", "l", "dl", "cl", "ml", "hl", "kb", "mb", "gb",
    "tb", "pb", "hz", "khz", "mhz", "ghz", "w", "kw", "mw", "kwh", "mwh", "bar", "mbar", "px", "dpi", "fps",
    "bit", "bajt", "h", "eur", "usd", "din", "dinar", "cent", "lipa", "tisuću", "hiljadu",
];
const FEMININE_UNITS: &[&str] = &["min", "t", "s", "sek", "ms", "kn", "km"];

/// The months in the genitive, by their Croatian names ...
#[rustfmt::skip]
const MONTHS_CROATIAN: &[&str] = &[
    "siječnja", "veljače", "ožujka", "travnja", "svibnja", "lipnja", "srpnja", "kolovoza", "rujna",
    "listopada", "studenoga", "studenog", "prosinca",
];
/// ... and by the international ones of Serbian and Bosnian.
#[rustfmt::skip]
const MONTHS_INTERNATIONAL: &[&str] = &[
    "januara", "februara", "marta", "aprila", "maja", "juna", "jula", "avgusta", "augusta", "septembra",
    "oktobra", "novembra", "decembra",
];

fn is_month_genitive(word: &str) -> bool {
    MONTHS_CROATIAN.contains(&word) || MONTHS_INTERNATIONAL.contains(&word)
}

/// The stem of an ordinal from the last word of the cardinal (prvi, drugi,
/// treći, četvrti, peti, šesti, sedmi, osmi, deveti, deseti, jedanaesti,
/// dvadeseti, dvadeset prvi, stoti, dvjestoti, tisućiti / hiljaditi,
/// milijunti / milioniti).
fn ordinal_stem(token: &str) -> String {
    match token {
        "jedan" | "jedna" | "jedno" => "prv".to_owned(),
        "dva" | "dvije" | "dve" => "drug".to_owned(),
        "tri" => "treć".to_owned(),
        "četiri" => "četvrt".to_owned(),
        "sedam" => "sedm".to_owned(),
        "osam" => "osm".to_owned(),
        _ if token.ends_with("sto") => format!("{token}t"),
        // dvesta: dvestot
        _ if token.ends_with("sta") => format!("{}ot", &token[..token.len() - 1]),
        _ if token.starts_with("tisuć") => "tisućit".to_owned(),
        _ if token.starts_with("hiljad") => "hiljadit".to_owned(),
        _ if token.starts_with("milijun") => "milijunt".to_owned(),
        _ if token.starts_with("milion") => "miliont".to_owned(),
        _ if token.starts_with("milijard") => "milijardit".to_owned(),
        // pet, šest, devet, deset, the teens, the tens
        _ => token.to_owned(),
    }
}

/// The ordinal's ending before a regular feminine a-stem: lexical identity
/// avoids treating godinom as masculine. Singular oblique forms and plural
/// forms are shared by many everyday/UI nouns. Source: IHJJ Hrvatska školska
/// gramatika, Brojevi.
fn feminine_ordinal_ending(word: &str, genitive: bool, instrumental: bool) -> &'static str {
    #[rustfmt::skip]
    const STEMS: &[&str] = &[
        "godin", "minut", "sekund", "ur", "knjig", "sob", "osob", "stranic", "nagrad",
        "drzav", "držav", "kuć", "žen", "škol", "zgrad", "ulic", "slik", "poruk",
        "datotek", "map", "aplikacij", "verzij", "točk", "tačk", "grup", "skupin", "lig",
        "sezon", "vrst", "smjen", "smen", "knjiz", "ruc", "noz", "općin", "opštin", "opcin", "opstin",
    ];
    for n in 1..=3.min(word.len().saturating_sub(1)) {
        let Some((stem, end)) = word.split_at_checked(word.len() - n) else { continue };
        if !STEMS.contains(&stem) {
            continue;
        }
        match end {
            "a" => return if genitive { "ih" } else { "a" },
            "e" => return "e",
            "i" if genitive => return "ih",
            "i" => return if instrumental { "im" } else { "oj" },
            "u" => return "u",
            "om" => return "Om", // feminine, including trećom
            "o" => return "a",   // vocative: prva ženo
            "ama" => return "im",
            _ => {}
        }
    }
    ""
}

/// The ending of an ordinal before the word `next`, after the word `prev`:
/// "" keeps the cardinal.
fn ordinal_ending(prev: &str, next: &str) -> &'static str {
    #[rustfmt::skip]
    const BEFORE_SUBJECT: &[&str] = &[
        "je", "su", "bio", "bila", "bilo", "bili", "bit", "biti", "nije", "nisu", "i", "ili", "pa", "te", "a",
        "ali", "također", "takođe", "još", "kao", "to", "ta", "taj", "ovo", "ova", "ovaj", "samo", "zatim",
        "potom", "onda", "sada", "sad", "danas", "npr", "odnosno", "dakle", "naime", "već",
    ];
    if is_month_genitive(next) {
        return "og"; // sedmog listopada
    }
    if next.len() < 3 || NOT_COUNTED.contains(&next) || next.ends_with("ti") {
        return "";
    }
    let ends = |tails: &[&str]| tails.iter().any(|tail| next.ends_with(tail));
    let genitive = WITH_GENITIVE.contains(&prev);
    let locative = WITH_LOCATIVE.contains(&prev) || WITH_DATIVE.contains(&prev);
    let instrumental = WITH_INSTRUMENTAL.contains(&prev);
    let after_subject = prev.is_empty() || BEFORE_SUBJECT.contains(&prev);
    // godina alone can also be genitive plural (konac 80. godina).
    let years = next == "godina" && !after_subject;
    if !years {
        let feminine = feminine_ordinal_ending(next, genitive, instrumental);
        if !feminine.is_empty() {
            return feminine;
        }
    }
    // The endings that show the case by themselves: u devedesetim godinama
    const BY_ENDING: [(&[&str], &str); 4] =
        [(&["og", "eg", "oga", "ega"], "og"), (&["im", "ima", "ama"], "im"), (&["ih"], "ih"), (&["oj", "ioj"], "oj")];
    if let Some(&(_, ending)) = BY_ENDING.iter().find(|(tails, _)| ends(tails)) {
        return ending;
    }
    if ends(&["om", "em"]) {
        return if instrumental {
            "im"
        } else if ends(&["om"]) {
            "om"
        } else {
            "em"
        };
    }
    if years && !genitive {
        return "ih"; // konac 80. godina
    }
    let either = |yes: bool, a: &'static str, b: &'static str| if yes { a } else { b };
    match last_digit(next) {
        b'a' if genitive => "og", // do trećeg razreda
        b'a' if locative || instrumental => "a",
        b'a' => either(after_subject, "a", "og"), // prva nagrada; odlomak trećeg članka
        b'e' => either(!genitive && is_neuter_e(next), "o", "e"), // prvo dijete; druge godine
        b'o' => either(MASCULINE_O.contains(&next), "i", "o"), // prvo mjesto; prvi auto
        b'u' => either(locative, "om", "u"),      // na prvom mjestu; prvu nagradu
        b'i' => either(locative, "oj", "i"),      // u prvoj godini; prvi koraci
        _ => either(is_feminine_i(next), "a", "i"), // prva stvar; treći članak
    }
}

/// A unit's sign at or after `pos` (spaces allowed), with its noun.
fn unit_at(t: Text, pos: usize) -> Option<UnitAt> {
    t.unit_at(pos, &UNITS, CELSIUS)
}

/// The combining stem of a compound with a number (jednosobni, dvogodišnji,
/// trogodišnja, četvorosobni, petodnevni, desetogodišnjica,
/// dvadesetogodišnjak, stogodišnjica) from the cardinal's words.
fn compound_stem(cardinal: &str) -> String {
    let token = last_token(cardinal);
    let last = &cardinal[token..];
    let stem = match last {
        "jedan" => "jedno".to_owned(),
        "dva" => "dvo".to_owned(),
        "tri" => "tro".to_owned(),
        "četiri" => FOUR_COMPOUND.to_owned(),
        "sedam" => "sedmo".to_owned(),
        "osam" => "osmo".to_owned(),
        _ if last.ends_with("sto") || last.ends_with("sta") => last.to_owned(),
        _ => format!("{last}o"),
    };
    format!("{}{stem}", &cardinal[..token]).replace(' ', "")
}

/// An ordinal with the adjectival ending the sentence asks for: prvi, prva,
/// prvo, prvog, prvom, prvim, prvih, prve, prvu, prvoj; treći, treće, trećeg,
/// trećem; dvadeset prvi (only the last word changes), stoti, tisućita,
/// dvije tisuće dvadeset šeste.
pub(crate) fn ordinal(r: &NumberReader, number: &str, ending: &str) -> String {
    if number.is_empty() {
        return String::new();
    }
    let mut words = r.number_to_words(strip_zeros(number));
    let token = last_token(&words);
    let stem = ordinal_stem(&words[token..]);
    let soft = ["ć", "č", "š", "ž", "j", "đ"].iter().any(|soft| stem.ends_with(soft));
    let ending = match ending {
        "o" if soft => "e",
        "og" if soft => "eg",
        "om" if soft => "em",
        "Om" => "om",
        _ => ending,
    };
    say(&mut words, token, &(stem + ending));
    words
}

/// An ordinal before a lowercase word, in the case the noun's ending and the
/// preposition before the number show; empty where nothing is counted, and
/// the cardinal stays. [start, end) are the digits, `after` the position
/// after the period.
pub(crate) fn ordinal_before(r: &NumberReader, t: Text, start: usize, end: usize, after: usize) -> String {
    let number = t.str(start, end);
    let next = t.next_word(after);
    if number.is_empty()
        || strip_zeros(number) == "0"
        || next.word.is_empty()
        || next.word.bytes().any(|b| b.is_ascii_digit())
    {
        return String::new();
    }
    let ending = ordinal_ending(&latin(&t.prev_word(start).word), &latin(next.word));
    if ending.is_empty() { String::new() } else { ordinal(r, number, ending) }
}

/// A number with a unit's sign: jedan euro, dva eura, pet eura, dvadeset jedan
/// euro, dvadeset dva eura, jedan stupanj, tri stupnja, pet stupnjeva Celzija,
/// jedan posto; the decimal takes the genitive singular.
pub(crate) fn count(r: &NumberReader, digits: &str, unit: &Unit, scale: &str) -> String {
    let clean = strip_zeros(digits).as_bytes();
    let teen = clean.iter().rev().nth(1) == Some(&b'1');
    let form = match clean.last() {
        Some(b'1') if !teen => unit.singular,
        Some(b'2'..=b'4') if !teen => unit.dual,
        _ => unit.genitive_plural,
    };
    format!("{} {form}{scale}", r.number_to_words(digits))
}

/// A decimal as the shared path reads it: "jedan zarez pet", up to two digits
/// after the comma as one number, more one by one, leading zeros as nula.
pub(crate) fn decimal(r: &NumberReader, whole: &str, fraction: &str) -> String {
    r.number_to_words(whole) + r.separator_words(b',') + &r.decimal_fraction(fraction)
}

/// jedan, jedna, jedno; dva, dvije (dve): the last word of a number agrees
/// with the noun after it in gender, and 1 in case after a preposition (jedna
/// kuća, jedno selo, jedan stol, jedna stvar; dvije kuće, dva stola, dvije
/// noći; s jednim prijateljem, u jednom satu, u jednoj godini, do jedne godine,
/// za jednu kunu; dvadeset jedna kuća, sto dvije kuće). Nothing counted keeps
/// jedan, dva: alone, in a list, before a sign, after a label (stranica 2),
/// glued to letters or a hyphen (T2, A-2), after a name (Pentium 2).
pub(crate) fn agree(t: Text, start: usize, end: usize, words: &mut String) {
    let token = last_token(words);
    let digit = match &words[token..] {
        "jedan" => 1,
        "dva" => 2,
        _ => return,
    };
    if let Some(form) = agreeing_form(t, start, end, digit) {
        say(words, token, form);
    }
}

fn agreeing_form(t: Text, start: usize, end: usize, digit: u8) -> Option<&'static str> {
    if start > 0 && (is_letter_byte(t.at(start - 1)) || t.at(start - 1) == b'-') {
        return None;
    }
    let prev_raw = t.prev_word(start);
    if prev_raw.end >= 2 && matches!(t.at(prev_raw.end - 1), b'-' | b'/') && is_letter_byte(t.at(prev_raw.end - 2)) {
        return None;
    }
    let (prev, prev_capital) = latin_word(&prev_raw.word);
    let prev = prev.as_str();
    if (prev_capital && !starts_clause(if prev_raw.ahead > 0 { t.at(prev_raw.ahead - 1) } else { b'.' }))
        || LABELS.contains(&prev)
    {
        return None;
    }

    let next_raw = t.next_word(end);
    // Nothing, a mark or a glued letter
    if next_raw.word.is_empty() || next_raw.begin == end || next_raw.word.bytes().any(|b| b.is_ascii_digit()) {
        return None;
    }
    let next = latin(next_raw.word);
    let next = next.as_str();
    let is_unit = |unit: &str, units: &[&str]| units.contains(&unit.to_ascii_lowercase().as_str());
    // A capital: a name of either gender (2 Hrvata), unless a unit (MB)
    if !next_raw.lowercase
        && !next.is_empty()
        && !is_unit(next_raw.word, MASCULINE_UNITS)
        && !is_unit(next_raw.word, FEMININE_UNITS)
    {
        return None;
    }
    let ends = |tails: &[&str]| tails.iter().any(|tail| next.ends_with(tail));

    if is_unit(next, MASCULINE_UNITS) {
        return None; // jedan kg, dva h
    }
    if is_unit(next, FEMININE_UNITS) {
        return Some(if digit == 1 { "jedna" } else { TWO_FEMININE }); // jedna min, dvije kn
    }
    if next.len() < 3 || NOT_COUNTED.contains(&next) || ends(&["ti"]) {
        return None;
    }

    let instrumental = WITH_INSTRUMENTAL.contains(&prev);
    let genitive = WITH_GENITIVE.contains(&prev) && !instrumental;
    let locative = WITH_LOCATIVE.contains(&prev) || WITH_DATIVE.contains(&prev);
    let tail = last_digit(next);
    let feminine_i = is_feminine_i(next);

    if digit == 2 {
        // Feminine before -e and the feminine i-stems (dvije kuće, dvije
        // noći), dva otherwise (dva stola, dva sela, dva dana)
        return (tail == b'e' || (tail == b'i' && feminine_i)).then_some(TWO_FEMININE);
    }
    Some(match tail {
        b'a' if (genitive || instrumental) && !ends(&["ima"]) => "jednog", // do jednog sata, s jednog mjesta
        b'e' if genitive => "jedne",                                       // od jedne godine
        b'u' if locative => "jednom",                                      // u jednom satu
        b'i' if locative => "jednoj",                                      // u jednoj godini
        _ if instrumental && ends(&["om", "em"]) => "jednim",              // s jednim prijateljem
        _ if ends(&["ima"]) => "jednim",
        b'a' => "jedna",                                 // jedna kuća
        b'o' if !MASCULINE_O.contains(&next) => "jedno", // jedno selo; jedan auto
        b'e' if is_neuter_e(next) => "jedno",            // jedno dijete
        b'e' => "jedne",                                 // jedne godine
        b'u' => "jednu",                                 // jednu kunu
        b'i' if feminine_i => "jedne",                   // jedne noći
        b'o' | b'i' => return None,
        _ if feminine_i => "jedna", // jedna stvar
        _ => return None,
    })
}

/// A date without a year at `pos`: 7. 10., 7.10., 05.03. (sedmog desetog). Two
/// ordinals written apart before a conjunction are an enumeration (1. 2. i
/// 3. mjesto), unless another date follows it (od 1. 12. do 5. 12., 7. 10.
/// i 8. 10.); written together they are a date (od 1.12. do 5.12.). The words,
/// in the nominative when the date names the day (Danas je sedmi deseti), and
/// the end of what they take.
fn short_date(r: &NumberReader, t: Text, pos: usize, nominative: bool) -> Option<(String, usize)> {
    let date = t.parse_day_month(pos)?;
    let next = t.next_word(date.month_end + 1);
    if next.lowercase
        && ["i", "ili", "do", "te", "odnosno", "pa"].contains(&latin(next.word).as_str())
        && date.month != date.day_end + 1
        && !t.date_at(t.skip_spaces(next.end))
    {
        return None;
    }
    let ending = if nominative { "i" } else { "og" };
    let mut words = join_parts([
        ordinal(r, t.str(pos, date.day_end), ending),
        ordinal(r, t.str(date.month, date.month_end), ending),
    ]);
    if !next.lowercase {
        return Some((words, date.month_end));
    }
    if next.begin <= date.month_end + 1 {
        words.push(' ');
    }
    Some((words, date.month_end + 1))
}

/// A full date: the day and the month as ordinals with `ending`, the year as
/// the feminine ordinal of "godine" (sedmog desetog dvije tisuće dvadeset
/// šeste).
pub(crate) fn date_to_words(r: &NumberReader, groups: &[&str; 3], ending: &str) -> String {
    join_parts([ordinal(r, groups[0], ending), ordinal(r, groups[1], ending), ordinal(r, groups[2], "e")])
}

/// Whether an en dash or an em dash stands at `pos`.
fn dash_at(t: Text, pos: usize) -> bool {
    t.has(pos, "\u{2013}") || t.has(pos, "\u{2014}")
}

/// The Bosnian constructs at position `i` (a digit)
/// that the shared loop does not know: a minus, a date in the case the
/// sentence asks for, a time written with a period, a range, thousands with
/// spaces or periods, a number compounded with a word (2-godišnji), a unit's
/// noun. Writes the words into `out` and returns the end of what it consumed,
/// or nothing to leave the number to the shared paths. `in_range`: the right
/// side of a range, whose context the left side has already read.
pub(crate) fn number_at(r: &NumberReader, t: Text, i: usize, out: &mut Out, in_range: bool) -> Option<usize> {
    let length = t.len();
    let end = t.digits_end(i);
    let digits = t.str(i, end);
    let leading_zero = t.at(i) == b'0' && end - i > 1;
    let value = small_value(digits);
    let prev_raw = t.prev_word(i);
    let (prev, prev_capital) = latin_word(&prev_raw.word);
    let prev = prev.as_str();
    let two_minutes_after = |pos: usize| pos + 2 < length && t.digits_end(pos + 1) == pos + 3;

    if !in_range {
        read_glued_minus(t, i, out);
    }
    // A leading zero is read digit by digit by the shared path (007, 0,5,
    // 01), except in a date or a time written with one: 07.10.2026., 05.03.,
    // u 08.30
    if leading_zero && !(end - i == 2 && t.at(end) == b'.') {
        return None;
    }

    // A range of two times: 8.30–16.00, 8:30–16:00
    if !in_range && matches!(t.at(end), b'.' | b':') && two_minutes_after(end) {
        let left_stop = end + 3;
        if dash_at(t, left_stop) && t.is_digit(left_stop + 3) {
            let (mut left, mut right) = (Out::default(), Out::default());
            if number_at(r, t, i, &mut left, true) == Some(left_stop)
                && let Some(stop) = number_at(r, t, left_stop + 3, &mut right, true)
            {
                out.0.extend(left.0);
                out.push_str(" do ");
                out.0.extend(right.0);
                return Some(stop);
            }
        }
    }

    // A full date: the day and the month as genitive ordinals, the year as
    // the feminine ordinal of "godine" (sedmog desetog dvije tisuće dvadeset
    // šeste); after "je" the nominative (Danas je sedmi deseti)
    // "Danas je 7. 10." names the day, the nominative; "Rođen je 7. 10." says
    // when, the genitive: only after danas je, datum je, to je ...
    const NAMES_THE_DAY: &[&str] =
        &["danas", "sutra", "jučer", "juče", "prekosutra", "prekjučer", "datum", "to", "ovo", "dan", "rok"];
    let nominative_date = prev == "je" && NAMES_THE_DAY.contains(&latin(&t.prev_word(prev_raw.begin).word).as_str());
    if let Some((date_end, groups)) = t.parse_date(i) {
        let mut words = date_to_words(r, &groups, if nominative_date { "i" } else { "og" });
        let date_end = t.date_period_end(date_end, &mut words);
        out.push_str(&words);
        return Some(date_end);
    }

    // A clock time with a colon: the shared path, 24:00 too
    if let Some((time_end, groups)) = t.parse_time(i) {
        if in_range {
            out.push_str(&r.time_to_words(&groups));
            return Some(time_end);
        }
        return None;
    }
    if digits == "24" && t.has(end, ":00") && !t.is_digit(end + 3) && t.at(end + 3) != b':' {
        out.push_str(&format!("dvadeset četiri{}nula nula", part_break()));
        return Some(end + 3);
    }

    // A time written with a period (u 8.30, od 8.30 do 16.00) in the context
    // of a time; 3.14 stays a decimal point read by name, 7.10. a date
    #[rustfmt::skip]
    const BEFORE_TIME: &[&str] = &[
        "u", "od", "do", "oko", "između", "izmedju", "iza", "prije", "pre", "poslije", "posle", "nakon",
        "točno", "tačno", "sati", "sat", "sata",
    ];
    #[rustfmt::skip]
    const AFTER_TIME: &[&str] = &[
        "h", "sati", "sat", "sata", "ujutro", "navečer", "uvečer", "uveče", "popodne", "poslijepodne",
        "poslepodne", "prijepodne", "prepodne",
    ];
    if end - i <= 2 && (0..=24).contains(&value) && t.at(end) == b'.' && two_minutes_after(end) {
        let minutes = t.str(end + 1, end + 3);
        let mm = small_value(minutes);
        let after = end + 3;
        let glued_more = matches!(t.at(after), b'.' | b',') && t.is_digit(after + 1);
        let colon_after = t.at(after) == b':';
        let next_raw = t.next_word(after);
        let date_like = t.at(after) == b'.' && prev != "u" && (1..=12).contains(&mm) && (1..=31).contains(&value);
        let ahead = prev_raw.ahead;
        let after_sada = prev == "je"
            && ahead >= 3
            && (t.bytes(ahead - 3, ahead).eq_ignore_ascii_case(b"sad")
                || (ahead >= 4 && t.bytes(ahead - 4, ahead).eq_ignore_ascii_case(b"sada")));
        let context = in_range
            || BEFORE_TIME.contains(&prev)
            || after_sada
            || (next_raw.lowercase
                && AFTER_TIME.contains(&latin(next_raw.word).as_str())
                && next_raw.begin <= after + 1);
        if mm <= 59 && (value < 24 || mm == 0) && !glued_more && !colon_after && !date_like && context {
            out.push_str(&r.time_to_words(&[digits, minutes]));
            return Some(after);
        }
    }

    // A range: 1991.–2000., od 1991. do 2000., 2–3 dana, 15.–20. ožujka,
    // 3.–4. mjesto, u 7. i 8. razredu, 1991-2000, 10–15 %
    if !in_range {
        let dotted = t.at(end) == b'.' && !t.is_digit(end + 1);
        let mut c = end + usize::from(dotted);
        let sp = t.skip_spaces(c);
        let link = if dash_at(t, sp) && (sp == c || dotted) {
            c = t.skip_spaces(sp + 3);
            Some(Link::Dash)
        } else if t.at(c) == b'-' {
            // 2-3 dana; after an ordinal's period the hyphen stands for the
            // dash: 10.-15.11.2026., 3.-4. mjesto, 1991.-2000.
            c += 1;
            Some(Link::Hyphen)
        } else if dotted {
            let word = t.next_word(c);
            let joining = latin(word.word);
            (word.begin > c && word.lowercase && ["do", "i", "ili", "te", "odnosno"].contains(&joining.as_str())).then(
                || {
                    c = t.skip_spaces(word.end);
                    Link::Word(joining)
                },
            )
        } else {
            None
        };
        if let Some(link) = link.filter(|_| t.is_digit(c) && t.at(c) != b'0') {
            let right_end = t.digits_end(c);
            let right = t.str(c, right_end);
            let join = link.join();
            if dotted {
                if let Some((date_end, groups)) = t.parse_date(c) {
                    let mut date = date_to_words(r, &groups, "og");
                    let date_end = t.date_period_end(date_end, &mut date);
                    out.push_str(&(ordinal(r, digits, "og") + &join + &date));
                    return Some(date_end);
                }
                if let Some((words, stop)) = short_date(r, t, c, false) {
                    out.push_str(&(ordinal(r, digits, "og") + &join + &words));
                    return Some(stop);
                }
                if t.at(right_end) == b'.' && !t.is_digit(right_end + 1) {
                    let noun_raw = t.next_word(right_end + 1);
                    let noun = latin(noun_raw.word);
                    if noun_raw.lowercase && right_end - c <= 4 {
                        let after = right_end + 1;
                        let ending = if is_month_genitive(&noun) { "og" } else { ordinal_ending(prev, &noun) };
                        let (left_words, right_words) = if ending.is_empty() {
                            (r.number_to_words(digits), r.number_to_words(right))
                        } else {
                            (ordinal(r, digits, ending), ordinal(r, right, ending))
                        };
                        out.push_str(&(left_words + &join + &right_words));
                        if !matches!(t.at(after), b' ' | b'\t') {
                            out.push(b' ');
                        }
                        return Some(after);
                    }
                    // Two years, nothing counted after them: od 1991. do 2000.
                    // (tisuću devetsto devedeset prve do dvije tisućite godine)
                    if end - i == 4 && right_end - c == 4 {
                        out.push_str(&(ordinal(r, digits, "e") + &join + &ordinal(r, right, "e")));
                        return Some(right_end); // the period ends the sentence as before
                    }
                }
                return None;
            }
            if matches!(link, Link::Hyphen)
                && (small_value(right) <= value
                    || right_end - c > 4
                    || !(end - i <= 2 || (end - i == 4 && right_end - c == 4)))
            {
                return None;
            }
            if matches!(t.at(end), b'.' | b',') || t.is_separator_between_digits(right_end) {
                return None;
            }
            let mut left_words = r.number_to_words(digits);
            if let Some((unit, unit_end, scale)) = unit_at(t, right_end) {
                out.push_str(&(left_words + &join + &count(r, right, unit, scale)));
                return Some(unit_end);
            }
            let mut right_words = r.number_to_words(right);
            agree(t, i, right_end, &mut left_words);
            agree(t, i, right_end, &mut right_words);
            out.push_str(&(left_words + &join + &right_words));
            return Some(right_end);
        }
    }

    // Not after another "N." (1. 2. 3.), which would be a list
    let in_list =
        prev.is_empty() && prev_raw.ahead >= 2 && t.at(prev_raw.ahead - 1) == b'.' && t.is_digit(prev_raw.ahead - 2);
    if !in_range
        && !in_list
        && let Some((words, stop)) = short_date(r, t, i, nominative_date)
    {
        out.push_str(&words);
        return Some(stop);
    }

    // Thousands separated by spaces or by periods (1 000, 1.000, 1.000.000,
    // 1.000,50; Hrvatski pravopis: the groups of three digits are set off
    // by a period or a space, the decimal mark is the comma): one number.
    // Not after a digit, a plus, a bracket or a slash (+385 91 123 4567),
    // not after telefon or broj, not after a name inside a sentence.
    if end - i <= 3 && !leading_zero {
        #[rustfmt::skip]
        const TELEPHONE: &[&str] = &[
            "telefon", "telefona", "telefonu", "tel", "mob", "mobitel", "mobitela", "faks", "fax", "broj",
            "broju", "brojem", "žiro", "iban", "račun", "računa", "računu", "poštanski", "nazovite",
            "nazovi", "zovite", "zovi", "pozovite", "pozovi", "gsm",
        ];
        for spaced in [true, false] {
            let mut pos = end;
            let mut joined = digits.to_owned();
            let mut group_count = 0;
            while group_count < 4 {
                let next = if spaced { t.skip_spaces(pos) } else { pos + 1 };
                let separated = if spaced { next != pos } else { t.at(pos) == b'.' };
                if !separated || t.digits_end(next) != next + 3 {
                    break;
                }
                joined += t.str(next, next + 3);
                pos = next + 3;
                group_count += 1;
            }
            if group_count == 0 {
                continue;
            }
            let before_pos = t.blanks_begin(if prev.is_empty() { prev_raw.end } else { prev_raw.begin });
            let before = if before_pos > 0 { t.at(before_pos - 1) } else { b' ' };
            let phone = TELEPHONE.contains(&prev)
                || if prev.is_empty() {
                    before.is_ascii_digit() || matches!(before, b'+' | b')' | b'/')
                } else {
                    before.is_ascii_digit() || last_digit(prev).is_ascii_digit()
                };
            let after_name =
                prev_capital && !starts_clause(if prev_raw.ahead > 0 { t.at(prev_raw.ahead - 1) } else { b'.' });
            // These guards protect space-separated identifiers. A dotted
            // thousands group is still a number after a UI label (Stanje,
            // Račun); treating it as a phone made positive balances differ
            // from negative ones, whose minus hid the preceding label.
            if (spaced && (phone || after_name))
                || is_letter_byte(t.at(pos))
                || t.at(pos) == b':'
                || (t.at(pos) == b'.' && t.is_digit(pos + 1))
            {
                continue;
            }
            let (words, stop) = if t.at(pos) == b',' && t.is_digit(pos + 1) {
                // 1.000,50: the decimal comma after the thousands
                let fraction_end = t.digits_end(pos + 1);
                if t.is_separator_between_digits(fraction_end) {
                    continue;
                }
                let words = decimal(r, &joined, t.str(pos + 1, fraction_end));
                match unit_at(t, fraction_end) {
                    Some((unit, unit_end, scale)) => (format!("{words} {}{scale}", unit.genitive_singular), unit_end),
                    None => (words, fraction_end),
                }
            } else if let Some((unit, unit_end, scale)) = unit_at(t, pos) {
                (count(r, &joined, unit, scale), unit_end)
            } else {
                let mut words = r.number_to_words(&joined);
                agree(t, i, pos, &mut words);
                (words, pos)
            };
            out.push_str(&words);
            return Some(stop);
        }
    }

    // A number glued to x or compounded with a word by a hyphen: 2x dva puta,
    // 1x jedanput, 2-godišnji dvogodišnji, 3-godišnja trogodišnja,
    // 10-godišnjica desetogodišnjica, 5-dnevni petodnevni, 1-sobni jednosobni,
    // 4-sobni četverosobni, 20-godišnjak dvadesetogodišnjak
    if end - i <= 4 && (i == 0 || !is_word_byte(t.at(i - 1))) && end < length {
        let cardinal = r.number_to_words(digits);
        let times = if digits == "1" { "jedanput".to_owned() } else { format!("{cardinal} puta") };
        if t.at(end) == b'-' && t.lowercase_at(end + 1) {
            let mut stop = end + 1;
            while stop < length && is_word_byte(t.at(stop)) && !t.is_digit(stop) {
                stop += 1;
            }
            let word = latin(t.str(end + 1, stop));
            if word.starts_with("put") {
                out.push_str(&times);
                return Some(stop);
            }
            if word.len() >= 4 && !t.is_digit(stop) {
                out.push_str(&compound_stem(&cardinal));
                out.push_str(&word);
                return Some(stop);
            }
        }
        let times_x = t.at(end) == b'x' && !is_letter_byte(t.at(end + 1));
        let times_sign = t.has(end, "×") && !is_letter_byte(t.at(end + 2));
        if times_x || times_sign {
            out.push_str(&times);
            return Some(end + if times_x { 1 } else { 2 });
        }
    }

    // A decimal with a unit's sign: 1,5 € jedan zarez pet eura
    if t.at(end) == b',' && t.is_separator_between_digits(end) {
        let fraction_end = t.digits_end(end + 1);
        if !t.is_separator_between_digits(fraction_end)
            && let Some((unit, unit_end, scale)) = unit_at(t, fraction_end)
        {
            let words = decimal(r, digits, t.str(end + 1, fraction_end));
            out.push_str(&format!("{words} {}{scale}", unit.genitive_singular));
            return Some(unit_end);
        }
    }

    // A whole number with a unit's sign: 1 € jedan euro, 2 € dva eura, 5 °C
    // pet stupnjeva Celzija, 1 % jedan posto
    if !t.is_separator_between_digits(end)
        && let Some((unit, unit_end, scale)) = unit_at(t, end)
    {
        out.push_str(&count(r, digits, unit, scale));
        return Some(unit_end);
    }
    None
}

/// How Bosnian reads numbers, as the number converter asks.
pub(crate) struct Numbers;

impl Grammar for Numbers {
    fn numerals(&self) -> &'static Numerals {
        &NUMERALS
    }

    fn units(&self) -> &'static [Unit; 4] {
        &UNITS
    }

    fn home_country(&self) -> &'static str {
        "387"
    }

    fn decimal_comma(&self) -> &'static str {
        " zarez "
    }

    fn number_at(&self, r: &NumberReader, t: Text, i: usize, out: &mut Out) -> Option<usize> {
        number_at(r, t, i, out, false)
    }

    fn agree(&self, t: Text, start: usize, end: usize, words: &mut String) {
        agree(t, start, end, words);
    }

    fn ordinal_before(&self, r: &NumberReader, t: Text, start: usize, end: usize, after: usize) -> String {
        ordinal_before(r, t, start, end, after)
    }

    fn date_to_words(&self, r: &NumberReader, groups: &[&str; 3]) -> String {
        date_to_words(r, groups, "og")
    }

    /// The period after a date's year is the ordinal's: 10.11.2026. godine.
    fn date_takes_its_period(&self) -> bool {
        true
    }

    fn currency_amount(&self, r: &NumberReader, _t: Text, _start: usize, whole: &str, unit: &Unit) -> String {
        count(r, whole, unit, "")
    }

    fn decimal(&self, r: &NumberReader, whole: &str, fraction: &str) -> String {
        decimal(r, whole, fraction)
    }
}
