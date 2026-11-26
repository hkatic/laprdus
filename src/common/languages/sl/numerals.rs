//! Slovenian numerals in a sentence: the agreement of a number with what it
//! counts, ordinals, dates, the hours, ranges and the nouns of the unit signs.
//!
//! Sources: Slovenski pravopis 2001 and SSKJ² (Fran), Pravopis 8.0, the
//! Jezikovna svetovalnica; the word lists are the commonest of Sloleks 3.1,
//! and the rules were measured on the corpus SUK 1.1.

use super::nouns::{NounA, is_neuter_noun_o, noun_a_kind};
use crate::text::numbers::NumberReader;
use crate::text::numbers::scan::{
    Link, Next, Out, Prev, Text, Unit, UnitAt, is_letter_byte, is_word_byte, join_parts, last_digit, last_token,
    ones_alone, part_break, read_glued_minus, say, small_value, starts_clause, strip_zeros,
};

/// The case a number with a unit's sign is in, by the preposition before it
/// (do petih odstotkov, pri eni stopinji, z dvema evroma, k petim evrom) or
/// its place as an object (za tri evre).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(crate) enum Case {
    Nominative,
    Accusative,
    Genitive,
    Dative,
    Locative,
    Instrumental,
}

// The prepositions that govern a case (SSKJ², Slovenski pravopis 2001),
// shared by the agreement of 1 to 4, of 5 and above, and of the ordinals
const WITH_GENITIVE: &[&str] = &[
    "od", "do", "iz", "brez", "zaradi", "okoli", "okrog", "blizu", "izmed", "izpod", "izza", "razen", "sredi", "glede",
    "mimo", "poleg", "znotraj", "zunaj", "starosti",
];
const WITH_LOCATIVE: &[&str] = &["v", "na", "o", "ob", "pri", "po"];
const WITH_INSTRUMENTAL: &[&str] = &["z", "s", "pred", "med", "nad", "pod", "za"];
const WITH_DATIVE: &[&str] = &["k", "h", "proti", "kljub"];

/// The words after which a number counts nothing (2 plus 2, 2 je sodo), and
/// the commonest verbs.
#[rustfmt::skip]
const NOT_COUNTED: &[&str] = &[
    // Function words and adverbs
    "tudi", "pri", "ali", "vse", "zaradi", "proti", "glede", "niti", "vsi", "okoli", "lani",
    "šele", "nikoli", "hkrati", "skozi", "zlasti", "drugače", "bolje", "pozneje", "kasneje",
    "vendarle", "kajti", "sredi", "kje", "radi", "nekateri", "kateri", "katere", "kakor", "meni",
    "plus", "minus", "deljeno",     // 2 plus 2
    "sebi", "sebe", "mene", "nje", "tri", "štiri", "dve", "obe", "dva", "ena", "bodisi", "jutri", "potem",
    "kot", "ker", "pod", "nad", "pred", "med", "brez", "sem", "smo", "bom", "nam", "vam", "jih", "jim",
    "tem", "teh", "kar", "kaj", "kdo", "kako", "ter", "bodo", "krat", "lahko", "tako", "zato", "samo",
    "zelo", "malo", "veliko", "dobro", "hitro", "vedno", "nato", "danes", "tisoč",
    // The commonest verbs
    "bili", "bile", "biti", "boste", "ste", "imeli", "imate", "ima", "imajo", "morali", "začeli", "dobili",
    "gre", "pravi", "more", "kaže", "zdi", "želi", "prišli", "pride", "deluje", "piše",
    "živi", "začne", "govori", "potrebuje", "zgodi", "daje", "sme", "pove", "postane", "vidi",
    "sodi", "pričakuje", "vsebuje", "drži", "dobi", "pomeni",
];

/// The feminine i-stems, the commonest of Sloleks (without -ost, -tev),
/// singular and plural: their consonant and their -i look masculine.
#[rustfmt::skip]
const FEMININE_I: &[&str] = &[
    "stvar", "oblast", "žival", "pesem", "napoved", "misel", "jed", "bolezen", "luč", "kazen", "snov",
    "stran", "pot", "vez", "reč", "skrb", "moč", "cev", "sled", "smet", "zamisel", "raven", "smuč",
    "strast", "laž", "vas", "plesen", "pripoved", "prepoved", "kokoš", "uš", "plat", "dlan", "plast",
    "smer", "korist", "odpoved", "klet", "smrt", "noč", "plavut", "kopel", "vajet", "klop", "izpoved",
    "vest", "obrt", "peč", "vrzel", "bojazen", "miš", "rast", "past", "pošast", "četrt", "zapoved",
    "obrv", "ljubezen", "pomoč", "pamet", "sol", "vrv", "zver", "gos", "utež", "vzmet", "dlesen",
    "pešpot", "piščal", "zvrst", "perut", "čeljust", "kap", "pest", "prikazen", "gred", "nit", "pomlad",
    "predjed", "basen", "kad", "jesen", "posest", "čast", "zmrzal", "prst", "povest", "zavest", "mast",
    "podstran", "kost",
    "stvari", "oblasti", "živali", "pesmi", "napovedi", "misli", "jedi", "bolezni", "luči", "kazni",
    "snovi", "strani", "poti", "vezi", "reči", "skrbi", "moči", "cevi", "sledi", "smeti", "zamisli",
    "ravni", "smuči", "strasti", "laži", "vasi", "plesni", "pripovedi", "prepovedi", "kokoši", "uši",
    "plati", "dlani", "plasti", "smeri", "koristi", "odpovedi", "kleti", "smrti", "noči", "plavuti",
    "kopeli", "vajeti", "klopi", "izpovedi", "vesti", "obrti", "peči", "vrzeli", "bojazni", "miši",
    "rasti", "pasti", "pošasti", "četrti", "zapovedi", "obrvi", "ljubezni", "pomoči", "pameti", "soli",
    "vrvi", "zveri", "gosi", "uteži", "vzmeti", "dlesni", "pešpoti", "piščali", "zvrsti", "peruti",
    "čeljusti", "kapi", "pesti", "prikazni", "gredi", "niti", "pomladi", "predjedi", "basni", "kadi",
    "jeseni", "posesti", "časti", "zmrzali", "prsti", "povesti", "zavesti", "masti", "podstrani", "kosti",
    "oči", "prsi", "sani", "jasli",
];

// Units written as abbreviations, by the gender of their noun: kilogram,
// meter, liter, megabajt, herc, vat are masculine, minuta, ura, sekunda,
// tona feminine (SSKJ²). Compared in small letters (MB, kW).
#[rustfmt::skip]
const MASCULINE_UNITS: &[&str] = &[
    "m", "cm", "mm", "dm", "km", "kg", "g", "dag", "mg", "l", "dl", "cl", "ml", "hl", "kb", "mb", "gb", "tb", "pb",
    "hz", "khz", "mhz", "ghz", "w", "kw", "mw", "kwh", "mwh", "bar", "mbar", "px", "dpi", "fps", "bit", "bajt",
    "tisoč",
];
const FEMININE_UNITS: &[&str] = &["h", "min", "t", "s", "sek", "ms"];

const MONTHS_GENITIVE: [&str; 12] = [
    "januarja",
    "februarja",
    "marca",
    "aprila",
    "maja",
    "junija",
    "julija",
    "avgusta",
    "septembra",
    "oktobra",
    "novembra",
    "decembra",
];

/// The hours after ob, in the locative plural (SP 2001 pét: "priti ob petih
/// |ob 5. uri; ob 5h; ob 5.00; ob 17. uri; ob 17h; ob 17.00|"; SSKJ² "vstajati
/// ob petih zjutraj"), when no form of ura follows: before one the hour is
/// the ordinal (`hour_ordinal_ending()`).
#[rustfmt::skip]
const HOURS_LOCATIVE: [&str; 25] = [
    "", "enih", "dveh", "treh", "štirih", "petih", "šestih", "sedmih", "osmih", "devetih", "desetih",
    "enajstih", "dvanajstih", "trinajstih", "štirinajstih", "petnajstih", "šestnajstih", "sedemnajstih",
    "osemnajstih", "devetnajstih", "dvajsetih", "enaindvajsetih", "dvaindvajsetih", "triindvajsetih",
    "štiriindvajsetih",
];

fn hour_locative(value: i64) -> &'static str {
    usize::try_from(value).ok().and_then(|hour| HOURS_LOCATIVE.get(hour)).copied().unwrap_or("")
}

/// The singular forms of ura a clock time can be written before (ob 10:00
/// uri, do 17:00 ure, pred 10:00 uro).
const URA_FORMS: &[&str] = &["ura", "ure", "uri", "uro"];

/// The end of a whole hour written as a clock time at `pos` (10:00, 17.00,
/// 24:00: one or two digits of 1 to 24, a colon or a period and 00), or
/// nothing.
fn whole_hour_end(t: Text, pos: usize) -> Option<usize> {
    let end = t.digits_end(pos);
    let value = small_value(t.str(pos, end));
    let stop = end + 3;
    let whole = (1..=2).contains(&(end - pos))
        && (1..=24).contains(&value)
        && matches!(t.at(end), b':' | b'.')
        && t.has(end + 1, "00")
        && !t.is_digit(stop)
        && t.at(stop) != b':'
        && !(t.at(stop) == b'.' && t.is_digit(stop + 1));
    whole.then_some(stop)
}

/// The form of ura written right after the clock time ending at `after`,
/// with its end: ob 10:00 uri, do 17:00 ure.
fn ura_after<'a>(t: Text<'a>, after: usize) -> Option<Next<'a>> {
    let next = t.next_word(after);
    (next.begin > after && next.lowercase && URA_FORMS.contains(&next.word)).then_some(next)
}

/// The ordinal's ending of a whole hour ending at `after` (after the word
/// `prev`) when a form of ura follows: ob 10:00 uri is ob deseti uri, do
/// 17:00 ure do sedemnajste ure, pred 10:00 uro pred deseto uro (a Slovenian
/// listener's report of 2026-11-24: ob desetih is the hour without uri, the
/// ordinal with it; SSKJ² úra "sonce vzide ob peti uri [5. uri]", "vstajati
/// pred šesto (uro)"; the Jezikovna svetovalnica, "Pomen nadpisanega simbola
/// h", 2021: "Dobiva se ob osmi uri – Dobiva se ob 8. uri"). An hour joined
/// by a dash, do, in, ali, oziroma or ter to another whole hour before ura
/// takes the other's ending: od 8:00 do 16:00 ure is od osme do šestnajste
/// ure, ob 10:00 in 11:00 uri ob deseti in enajsti uri (SSKJ² "popoldne bom
/// doma med tretjo in četrto (uro)").
fn hour_ordinal_ending(t: Text, prev: &str, after: usize) -> Option<&'static str> {
    let ending = |prev: &str, ura: &Next| ordinal_ending(prev, ura.word, lowercase_word(&t.next_word(ura.end)));
    if let Some(ura) = ura_after(t, after) {
        return Some(ending(prev, &ura));
    }
    let (link, second) = if dash_at(t, after) {
        ("", after + 3)
    } else {
        let word = t.next_word(after);
        if !(word.begin > after && word.lowercase && ["do", "in", "ali", "oziroma", "ter"].contains(&word.word)) {
            return None;
        }
        (word.word, t.skip_spaces(word.end))
    };
    if !t.is_digit(second) {
        return None;
    }
    let second_end = whole_hour_end(t, second)?;
    ura_after(t, second_end).map(|ura| ending(link, &ura))
}

/// The minutes after an hour read as the ordinal before ura, in the
/// nominative whatever the preposition: ob 10:30 uri is ob deseti uri in
/// trideset minut, ob 11:02 uri ob enajsti uri in dve minuti (SP 2001 úra
/// "začetek predstave ob dvajseti uri in dvajset minut ‹20.20›"; the Jezikovna
/// svetovalnica, "Ujemanje: ure in minute v priredno zloženi besedni zvezi",
/// 2021: "ob 11.02 ... ob enajsti uri / enajstih in dve minuti", "ob 21. uri
/// in 15 minut"; "Zapis nepolne oz. necele ure", 2016: ob 8.30 "ob osmi uri in
/// trideset minut").
fn minutes_words(r: &NumberReader, minutes: &str) -> String {
    let clean = strip_zeros(minutes);
    let mut words = r.number_to_words(clean);
    feminine_two(&mut words);
    words += match last_digit(clean) {
        _ if !ones_alone(clean) => " minut",
        b'1' => " minuta",
        b'2' => " minuti",
        b'3' | b'4' => " minute",
        _ => " minut",
    };
    words
}

/// A clock time at `i` (hours ending at `end`, `minutes` two digits, the time
/// ending at `after`) before a form of ura: a whole hour is the ordinal
/// (deseti, with the ura left in the text), any other the ordinal, the ura
/// and the minutes (deseti uri in trideset minut, the ura consumed; not in a
/// range, whose other side is read as a time). Writes the words and returns
/// the end of what it read.
fn hour_before_ura(
    r: &NumberReader,
    t: Text,
    (i, end, after): (usize, usize, usize),
    minutes: &str,
    prev: &str,
    in_range: bool,
    out: &mut Out,
) -> Option<usize> {
    let digits = t.str(i, end);
    if !(1..=24).contains(&small_value(digits)) {
        return None;
    }
    if minutes == "00" {
        let ending = hour_ordinal_ending(t, prev, after)?;
        out.push_str(&ordinal(r, digits, ending));
        return Some(after);
    }
    let ura = ura_after(t, after).filter(|_| !in_range && small_value(digits) < 24)?;
    let ending = ordinal_ending(prev, ura.word, lowercase_word(&t.next_word(ura.end)));
    out.push_str(&format!("{} {} in {}", ordinal(r, digits, ending), ura.word, minutes_words(r, minutes)));
    Some(ura.end)
}

/// How a clock time is read after a preposition when no form of ura follows
/// it (a Slovenian listener's answers of 2026-11-24: ob 8:30 is ob osmih
/// trideset, do 17:00 do sedemnajste, pred 10:00 pred deseto).
#[derive(Clone, Copy, PartialEq, Eq)]
enum HourAfter {
    /// The cardinal in the locative after ob (ob desetih, and with minutes ob
    /// osmih trideset: the Jezikovna svetovalnica, "Zapis nepolne oz. necele
    /// ure", 2016: ob 8.30 "manj formalno pa 'ob osmih trideset'").
    Ob,
    /// The cardinal in the genitive after okoli and okrog, and on both sides
    /// of a range with minutes (od osmih trideset do šestnajstih)
    /// (SP 2001 okóli "priti ~ desetih"; SSKJ² okóli "pride okoli desetih,
    /// desete (ure)", okróg "pride okrog enajstih, enajste (ure)"). The forms
    /// are those of the locative: desetih, dveh, enih.
    About,
    /// The feminine ordinal of the left-out ura after do, as after od (do
    /// sedemnajste), but minutes under ten as the clock reads them: do 17:05
    /// do sedemnajste nič pet (the listener's second answers of 2026-11-24,
    /// question 6; od, pred, po and med keep "in pet minut" until the
    /// listener is asked about them, the maintainer's decision).
    Until,
    /// The feminine ordinal of the left-out ura with this ending: "e" after od (od osme), "O" after pred and med
    /// (pred deseto, med osmo in deseto), "i" after po (po deseti). SSKJ²
    /// úra "vstajati pred šesto (uro); med tretjo in četrto (uro)", drúgi
    /// "upam, da pride po drugi (uri)"; SP 2001 the nouns of the hours,
    /// deséta "poklicati po ~i |po 10. uri|", ôsma "priti pred ~o |pred 8.
    /// uro|", dvájseta "čakati do ~e |do 20. ure|", trinájsta "delati do ~e
    /// |do 13. ure|"; the Jezikovna svetovalnica, "Kako je prav: med enajsto
    /// in »prvo« ali »eno« uro?" (Dobrovoljc 2017: "deseta ura > deseta ...
    /// posamostaljene glavne števnike le ob predlogu ob") and "Pomen
    /// nadpisanega simbola h" (Dobrovoljc 2021: "Kosili smo med petnajsto in
    /// šestnajsto, pred devetnajsto pa smo odšli v mesto").
    Ordinal(&'static str),
}

/// The reading of a clock time after the word `prev` with no ura after it.
/// Not after proti or za, for which no dictionary gives the hour.
fn hour_after(prev: &str) -> Option<HourAfter> {
    match prev {
        "ob" => Some(HourAfter::Ob),
        "okoli" | "okrog" => Some(HourAfter::About),
        "do" => Some(HourAfter::Until),
        "od" => Some(HourAfter::Ordinal("e")),
        "pred" | "med" => Some(HourAfter::Ordinal("O")),
        "po" => Some(HourAfter::Ordinal("i")),
        _ => None,
    }
}

/// A clock time at `pos` without seconds: one or two digits of the hours, a
/// colon or a period, two of the minutes (24 only with 00), and no more
/// digits, colon or decimal after it (ob 8:30, do 17.00, 24:00; not 8:30:15,
/// 8.30.5, 8.00,5). The end, the hours and the minutes.
fn clock_time_at<'a>(t: Text<'a>, pos: usize) -> Option<(usize, &'a str, &'a str)> {
    let end = t.digits_end(pos);
    if !(1..=2).contains(&(end - pos)) || !matches!(t.at(end), b':' | b'.') || t.digits_end(end + 1) != end + 3 {
        return None;
    }
    let (hours, minutes, stop) = (t.str(pos, end), t.str(end + 1, end + 3), end + 3);
    let (hh, mm) = (small_value(hours), small_value(minutes));
    let whole = (1..=24).contains(&hh)
        && mm <= 59
        && (hh < 24 || mm == 0)
        && t.at(stop) != b':'
        && !(matches!(t.at(stop), b'.' | b',') && t.is_digit(stop + 1));
    whole.then_some((stop, hours, minutes))
}

/// Whether a clock time ends at `pos` (8:30 or 16.00 before " do"), so that
/// the time after the word that follows is the other side of a range.
fn time_ends_at(t: Text, pos: usize) -> bool {
    pos >= 4
        && t.is_digit(pos - 1)
        && t.is_digit(pos - 2)
        && matches!(t.at(pos - 3), b':' | b'.')
        && t.is_digit(pos - 4)
}

/// The minutes after an hour said without ura: under ten with in and their
/// noun (ob 8:05 ob osmih in pet minut, ob 12:02 ob dvanajstih in dve minuti;
/// a Slovenian listener's answer of 2026-11-24, second round; the Jezikovna
/// svetovalnica, "Zapis nepolne oz. necele ure", 2016: "ob osmih in trideset
/// minut"), from ten on, and after do (`clock_minutes`) also under ten, as the
/// clock reads them, after the short pause of a clock time (ob osmih trideset,
/// do sedemnajste trideset, do sedemnajste nič pet: the listener's second
/// answers, question 6).
fn hour_with_minutes(r: &NumberReader, hour: String, minutes: &str, clock_minutes: bool) -> String {
    if minutes == "00" {
        return hour;
    }
    if small_value(minutes) < 10 && !clock_minutes {
        return format!("{hour} in {}", minutes_words(r, minutes));
    }
    let mut part = r.digit_group_to_words(minutes);
    feminine_two(&mut part);
    join_parts([hour, part])
}

/// The words of a clock time read as `reading` says, the minutes by
/// `hour_with_minutes()`: ob 8:30 ob osmih trideset, okoli 10:30 okoli
/// desetih trideset, do 17:30 do sedemnajste trideset, pred 10:30 pred deseto
/// trideset (the listener's answer for do 17:30; the other prepositions by
/// the same pattern), do 17:05 do sedemnajste nič pet. One o'clock as an
/// ordinal is the feminine one, ene, eni, eno (Dobrovoljc 2017: "med (pol)
/// eno in drugo; med dvanajsto in (pol) eno", prva "neprimeren"; SSKJ² úra
/// "čas od polnoči do ene").
fn hour_words(r: &NumberReader, reading: HourAfter, hours: &str, minutes: &str) -> String {
    let ordinal_ending = match reading {
        HourAfter::Until => Some("e"),
        HourAfter::Ordinal(ending) => Some(ending),
        HourAfter::Ob | HourAfter::About => None,
    };
    let hour = match ordinal_ending {
        None => hour_locative(small_value(hours)).to_owned(),
        Some(ending) => match (small_value(hours), ending) {
            (1, "e") => "ene".to_owned(),
            (1, "i") => "eni".to_owned(),
            (1, _) => "eno".to_owned(),
            _ => ordinal(r, hours, ending),
        },
    };
    hour_with_minutes(r, hour, minutes, reading == HourAfter::Until)
}

/// A clock time at `i` after a preposition of time, with no ura after it or
/// after a time joined to it (those are `hour_before_ura()`'s): ob 8:30 ob
/// osmih trideset, okoli 10:00 okoli desetih, do 17:00 do sedemnajste, pred
/// 10:00 pred deseto, do 17:30 do sedemnajste trideset (a Slovenian
/// listener's answers of 2026-11-24, both rounds; see `HourAfter`). In a
/// range by a dash or do after od both hours are the cardinal in -ih, and
/// after ob where either side has minutes (od 8:30 do 16:00 od osmih
/// trideset do šestnajstih: the listener's answer, "točno je od osmih
/// trideset do šestnajstih"; od 8:00 do 16:30 od osmih do šestnajstih
/// trideset, and od 8:00 do 16:00, od 8:00–16:00 od osmih do šestnajstih:
/// the listener's second answers, questions 3 and 4; SSKJ² ósem "dela od
/// osmih do štirih"). A second whole hour after another preposition and a
/// whole hour is read as after do (ob 8:00 do 9:00 ob osmih do devete, a
/// question for the listener's next round). A time joined by in,
/// ali, oziroma or ter is read as the first (med 8:00 in 10:00 med osmo in
/// deseto, ob 10:00 in 11:00 ob desetih in enajstih). Nothing before a
/// unit's sign, where a clock time stands before `prev` (8:30 do 16:00: the
/// right side of a range), or where a period, a possible month and another
/// period make it a date after any word but ob (do 5.11.). Writes the words
/// and returns the end of what it read.
fn hour_after_preposition(r: &NumberReader, t: Text, i: usize, prev: &Prev, out: &mut Out) -> Option<usize> {
    let reading = hour_after(&prev.word)?;
    let (after, hours, minutes) = clock_time_at(t, i)?;
    if time_ends_at(t, prev.ahead)
        || ura_after(t, after).is_some()
        || hour_ordinal_ending(t, &prev.word, after).is_some()
    {
        return None;
    }
    // do 5.11., od 1.12. do 5.12.: the day and the month
    let date_like = |end: usize, minutes: &str| {
        t.at(end - 3) == b'.' && t.at(end) == b'.' && (1..=12).contains(&small_value(minutes))
    };
    if reading != HourAfter::Ob && date_like(after, minutes) {
        return None;
    }
    let mut words = hour_words(r, reading, hours, minutes);
    let mut stop = after;
    let link = if dash_at(t, after) {
        Some(("do", after + 3))
    } else {
        let word = t.next_word(after);
        (word.begin > after && word.lowercase && ["do", "in", "ali", "oziroma", "ter"].contains(&word.word))
            .then(|| (word.word, t.skip_spaces(word.end)))
    };
    if let Some((link, second)) = link
        && let Some((second_end, second_hours, second_minutes)) = clock_time_at(t, second)
    {
        if ura_after(t, second_end).is_some() || date_like(second_end, second_minutes) {
            return None;
        }
        let whole_hours = minutes == "00" && second_minutes == "00";
        let second_reading = if link != "do" {
            reading
        } else if prev.word == "od" || (prev.word == "ob" && !whole_hours) {
            // A range after od, and one with minutes after ob: both hours in -ih
            words = hour_words(r, HourAfter::About, hours, minutes);
            HourAfter::About
        } else if whole_hours {
            HourAfter::Ordinal("e")
        } else {
            return None;
        };
        words = format!("{words} {link} {}", hour_words(r, second_reading, second_hours, second_minutes));
        stop = second_end;
    }
    if unit_at(t, stop).is_some() {
        return None;
    }
    out.push_str(&words);
    Some(stop)
}

/// The nouns of the unit signs, in the forms a number asks for (SP 2001
/// odstótek -tka m "pet ~ov ‹5 %›", évro -a m, dólar -ja m, stopínja -e ž
/// "pet stopinj Celzija ‹5 °C›", Célzij -a "37 stopinj ~a").
pub(crate) static UNITS: [Unit; 4] = [
    Unit {
        symbol: "%",
        feminine: false,
        singular: "odstotek",
        dual: "odstotka",
        plural: "odstotki",
        accusative_plural: "odstotke",
        genitive_plural: "odstotkov",
        genitive_singular: "odstotka",
        accusative_singular: "odstotek",
        dative_singular: "odstotku",
        instrumental_singular: "odstotkom",
        dual_dative: "odstotkoma",
        plural_dative: "odstotkom",
        plural_locative: "odstotkih",
        plural_instrumental: "odstotki",
    },
    Unit {
        symbol: "€",
        feminine: false,
        singular: "evro",
        dual: "evra",
        plural: "evri",
        accusative_plural: "evre",
        genitive_plural: "evrov",
        genitive_singular: "evra",
        accusative_singular: "evro",
        dative_singular: "evru",
        instrumental_singular: "evrom",
        dual_dative: "evroma",
        plural_dative: "evrom",
        plural_locative: "evrih",
        plural_instrumental: "evri",
    },
    Unit {
        symbol: "$",
        feminine: false,
        singular: "dolar",
        dual: "dolarja",
        plural: "dolarji",
        accusative_plural: "dolarje",
        genitive_plural: "dolarjev",
        genitive_singular: "dolarja",
        accusative_singular: "dolar",
        dative_singular: "dolarju",
        instrumental_singular: "dolarjem",
        dual_dative: "dolarjema",
        plural_dative: "dolarjem",
        plural_locative: "dolarjih",
        plural_instrumental: "dolarji",
    },
    Unit {
        symbol: "°",
        feminine: true,
        singular: "stopinja",
        dual: "stopinji",
        plural: "stopinje",
        accusative_plural: "stopinje",
        genitive_plural: "stopinj",
        genitive_singular: "stopinje",
        accusative_singular: "stopinjo",
        dative_singular: "stopinji",
        instrumental_singular: "stopinjo",
        dual_dative: "stopinjama",
        plural_dative: "stopinjam",
        plural_locative: "stopinjah",
        plural_instrumental: "stopinjami",
    },
];

/// A unit's sign at or after `pos` (spaces allowed): %, €, $, °, °C, °F, not
/// glued to a letter.
pub(crate) fn unit_at(t: Text, pos: usize) -> Option<UnitAt> {
    t.unit_at(pos, &UNITS, " Celzija")
}

fn is_month_genitive(word: &str) -> bool {
    MONTHS_GENITIVE.contains(&word)
}

fn is_feminine_i(next: &str) -> bool {
    FEMININE_I.contains(&next)
        || (next.ends_with("ost") && next.len() > 5)
        || (next.ends_with("osti") && next.len() > 6)
        || next.ends_with("tev")
}

/// A cardinal of five and above as its last word: pet ... deset, the teens,
/// the tens and the compounds (enaindvajset); not sto, tisoč, milijon.
fn is_five_plus(word: &str) -> bool {
    ["pet", "šest", "sedem", "osem", "devet", "deset"].contains(&word)
        || ["najst", "deset", "dvajset"].iter().any(|tail| word.ends_with(tail))
}

/// The stem the oblique endings of five and above are added to: petih,
/// sedmih, osmih.
fn oblique_stem(word: &str) -> &str {
    match word {
        "sedem" => "sedm",
        "osem" => "osm",
        _ => word,
    }
}

/// The digit a number's last word is, when it is a word of its own: 1 to 4.
fn small_digit(word: &str) -> usize {
    ["ena", "dva", "tri", "štiri"].iter().position(|&digit| digit == word).map_or(0, |at| at + 1)
}

/// The word, when it starts with a lowercase letter.
fn lowercase_word<'a>(next: &Next<'a>) -> &'a str {
    if next.lowercase { next.word } else { "" }
}

/// The stem of a Slovenian ordinal from the last word of the cardinal (SP
/// 2001: pŕvi, drúgi, trétji -a -e, četŕti, pêti, šêsti, sêdmi, ôsmi, devêti,
/// desêti, enájsti, dvájseti, ênaindvájseti, stôti, dvéstoti, tisóči -a -e,
/// milijónti). Only the last word of a longer number changes: sto prvi,
/// tisoč devetsto enaindevetdeseti, dva tisoči.
fn ordinal_stem(token: &str) -> String {
    match token {
        "ena" => "prv".to_owned(),
        "dva" => "drug".to_owned(),
        "tri" => "tretj".to_owned(),
        "štiri" => "četrt".to_owned(),
        "sedem" => "sedm".to_owned(),
        "osem" => "osm".to_owned(),
        _ if token.ends_with("sto") => format!("{token}t"),
        _ if token.starts_with("milijon") => "milijont".to_owned(),
        _ if token.starts_with("milijard") => "milijardt".to_owned(),
        // pet, šest, devet, deset, the teens, the tens, the compounds, tisoč
        _ => token.to_owned(),
    }
}

/// A Slovenian ordinal with the adjectival ending the sentence asks for:
/// "ega" of a date (sedmega oktobra, enaintridesetega), "o" of prvo mesto,
/// "em" of na prvem mestu, "ih" of v devetdesetih letih. The soft stems
/// (tretj-, tisoč-) take -e for the -o of the neuter (tretje mesto, SP 2001
/// trétji -a -e; tisóči -a -e) and keep it for the feminine accusative and
/// instrumental, written "O" (tretjo nagrado, s tretjo nagrado).
pub(crate) fn ordinal(r: &NumberReader, number: &str, ending: &str) -> String {
    if number.is_empty() {
        return String::new();
    }
    let mut words = r.number_to_words(strip_zeros(number));
    let token = last_token(&words);
    let stem = ordinal_stem(&words[token..]);
    let ending = match ending {
        // The feminine accusative and instrumental, which the soft stems keep (tretjo nagrado)
        "O" => "o",
        "o" if ["j", "c", "č", "š", "ž"].iter().any(|soft| stem.ends_with(soft)) => "e",
        "Om" => "om",
        _ => ending,
    };
    say(&mut words, token, &(stem + ending));
    words
}

/// A Slovenian decimal: the whole part, "cela" agreeing with it (ena cela,
/// dve celi, tri cele, štiri cele, pet celih, nič celih, enaindvajset celih:
/// SSKJ "ena cela, pet stotink"; the compounds like every number from five
/// up), then the digits after the comma as one number, leading zeros each
/// as nič (nič celih nič ena: SSKJ).
pub(crate) fn decimal(r: &NumberReader, whole: &str, fraction: &str) -> String {
    let mut words = r.number_to_words(whole);
    let clean = strip_zeros(whole);
    feminine_two(&mut words); // dve celi
    words += match last_digit(clean) {
        _ if !ones_alone(clean) => " celih",
        b'1' => " cela",
        b'2' => " celi",
        b'3' | b'4' => " cele",
        _ => " celih",
    };
    words.push(' ');
    let mut decimals = r.digit_group_to_words(fraction);
    feminine_two(&mut decimals); // tri cele dve: dve desetinki
    words + &decimals
}

/// A Slovenian number that ends in a 2 of its own (dva, sto dva, dva tisoč
/// dva; not dvaindvajset, dvanajst) in the feminine.
pub(crate) fn feminine_two(words: &mut String) {
    if words == "dva" || words.ends_with(" dva") {
        words.pop();
        words.push('e');
    }
}

/// Whether the noun phrase in -o after an ordinal or a numeral is neuter
/// (tretje mesto) or the feminine accusative or instrumental (tretjo
/// nagrado, s tretjo nagrado; a listener's report of 2026-11-03): the
/// generated table of the neuter nouns in -o (`nouns`), the feminine being
/// the commoner and the default. When the word after it is in -o too the
/// first may be an adjective, and the second decides (tretjo zlato medaljo,
/// tretje veliko mesto). Only tretji shows the difference; prvo, drugo, peto
/// are the same in both genders.
fn is_neuter_o(next: &str, after_next: &str) -> bool {
    if after_next.len() >= 3 && after_next.ends_with('o') && !after_next.as_bytes()[0].is_ascii_digit() {
        return is_neuter_noun_o(after_next);
    }
    // An adjective and a neuter noun in -je or -šče (tretje politično
    // zatočišče, tretje veliko nadstropje); not a verb (prejme, deluje) nor
    // a feminine genitive in -ije (tretjo nagrado akcije)
    if after_next.len() >= 5
        && ((after_next.ends_with("je") && !after_next.ends_with("uje") && !after_next.ends_with("ije"))
            || after_next.ends_with("šče"))
    {
        return true;
    }
    is_neuter_noun_o(next)
}

/// The ordinal's ending before a regular feminine a-stem: lexical identity
/// avoids treating e.g. ure as neuter (the broad -re guess). Singular oblique
/// forms and plural forms are shared by many everyday/UI nouns. Source: SP
/// 2001 paradigms (Fran).
fn feminine_ordinal_ending(word: &str, genitive: bool) -> &'static str {
    #[rustfmt::skip]
    const STEMS: &[&str] = &[
        "ur", "minut", "sekund", "knjig", "sob", "oseb", "nagrad", "držav", "hiš", "žen",
        "šol", "stavb", "ulic", "slik", "datotek", "map", "aplikacij", "različic", "točk",
        "skupin", "lig", "sezon", "vrst", "občin", "naprav", "nalog", "besed", "strank",
    ];
    for n in 1..=3.min(word.len().saturating_sub(1)) {
        let Some((stem, end)) = word.split_at_checked(word.len() - n) else { continue };
        if !STEMS.contains(&stem) {
            continue;
        }
        match end {
            "a" => return if genitive { "ih" } else { "a" },
            "e" => return "e",
            "i" => return if genitive { "ih" } else { "i" },
            "o" => return "O",
            "ah" => return "ih",
            "am" => return "im",
            "ami" => return "imi",
            "ama" => return "ima",
            _ => {}
        }
    }
    ""
}

/// The ending of an ordinal before the word `next`, after the word `prev`
/// (both in small letters; `after_next` the lowercase word after `next`, or
/// empty): the adjectival endings as the noun's case, gender and number show
/// them. "" keeps the cardinal (nothing counted: 2. je, 2. deluje).
fn ordinal_ending(prev: &str, next: &str, after_next: &str) -> &'static str {
    // After a word that is not a preposition, a copula, a conjunction or a
    // particle the phrase in -a is a genitive attribute (odstavka 3. člena,
    // začetek 2. tisočletja, pari 3. kola); at the head of the clause or after
    // je, so, in, pa ... the feminine nominative (1. nagrada, 2. liga, 80.
    // leta, 100. obletnica)
    #[rustfmt::skip]
    const BEFORE_SUBJECT: &[&str] = &[
        "je", "so", "bo", "bodo", "bil", "bila", "bilo", "bili", "ni", "in", "ali", "pa", "ter", "tudi", "še", "kot",
        "to", "ta", "le", "samo", "kar", "sicer", "torej", "npr", "oziroma", "zdaj", "danes", "že", "šele",
    ];
    if is_month_genitive(next) {
        return "ega"; // sedmega oktobra
    }
    if next.len() < 3 || NOT_COUNTED.contains(&next) || next.ends_with("uje") || next.ends_with("jte") {
        return "";
    }
    let ends = |tails: &[&str]| tails.iter().any(|tail| next.ends_with(tail));
    let genitive = WITH_GENITIVE.contains(&prev) || prev == "z" || prev == "s";
    let dative = WITH_DATIVE.contains(&prev);
    let instrumental = WITH_INSTRUMENTAL.contains(&prev);
    let feminine = feminine_ordinal_ending(next, genitive);
    if !feminine.is_empty() {
        return feminine;
    }
    // The endings that show the case by themselves: s prvimi koraki, v
    // devetdesetih letih
    const BY_ENDING: [(&[&str], &str); 5] = [
        (&["ega"], "ega"),
        (&["emu"], "emu"),
        (&["ima", "ema", "oma"], "ima"),
        (&["imi", "ami"], "imi"),
        (&["ih", "ah", "eh", "ov", "ev"], "ih"),
    ];
    if let Some(&(_, ending)) = BY_ENDING.iter().find(|(tails, _)| ends(tails)) {
        return ending;
    }
    if ends(&["om"]) || (ends(&["em"]) && instrumental) {
        return "im"; // pred prvim letom
    }
    if ["let", "ur", "dni"].contains(&next) {
        return "ih"; // konec 80. let
    }
    match last_digit(next) {
        // do tretjega razreda
        b'a' if genitive => "ega",
        // A masculine noun's genitive or animate accusative, or its
        // nominative in -a (the generated table): videl sem tretjega otroka,
        // prvi kolega (leta is the decades' nominative plural too, 80. leta;
        // a year's "1958. leta" is made the genitive by ordinal_before())
        b'a' => match if next == "leta" { None } else { noun_a_kind(next) } {
            Some(NounA::Genitive) => "ega",
            Some(NounA::Masculine) => "i",
            // prva stran; odstavka tretjega člena
            None if prev.is_empty() || BEFORE_SUBJECT.contains(&prev) => "a",
            None => "ega",
        },
        // tretje nadstropje; prve knjige
        b'e' if ["dete", "ime", "dekle"].contains(&next) || ends(&["šče"]) || (ends(&["je"]) && !ends(&["ije"])) => {
            "o"
        }
        b'e' => "e",
        b'o' if is_neuter_o(next, after_next) => "o", // prvo mesto, tretje mesto
        b'o' => "O",                                  // tretjo nagrado
        b'u' if dative => "emu",                      // k prvemu členu
        b'u' => "em",                                 // na prvem mestu
        b'i' => "i",                                  // na prvi strani, ob peti uri, prvi koraki
        _ if is_feminine_i(next) => "a",              // prva stran
        _ => "i",                                     // tretji člen, sedmi oktober
    }
}

/// The stem of a multiplicative (enkrat, dvakrat, trikrat, štirikrat,
/// petkrat, stokrat: SP 2001 ênkrat, dvákrat) and of a compound with a
/// number (enosobno, dvoletni, triletna, štiriletni, petdnevni,
/// desetletnica, dvajsetletnik, stoletnica: SP dvóléten, desétlétnica),
/// from the digits; a longer number is written as one word (stopetdeset-).
fn number_stem(digits: &str, words: &str, multiplicative: bool) -> String {
    match digits {
        "1" => if multiplicative { "en" } else { "eno" }.to_owned(),
        "2" => if multiplicative { "dva" } else { "dvo" }.to_owned(),
        _ => words.replace(' ', ""),
    }
}

/// A Slovenian 1, 2, 3 or 4 at the end of the number in [start, end) of the
/// text agrees with what it counts (dvá dvé dvé, dvéh, dvéma; tríje trí, tréh,
/// trém, trémi; štírje štíri, štírih, štírim, štírimi; èn êna êno: SSKJ²,
/// Slovenski pravopis 2001). Asked for by a
/// Slovenian listener (2 koša, T2, en koš, trije koši, z dvema žličkama). The
/// ending of the next word and the preposition before the number decide; there
/// is no parser here, so a word that could be something else leaves the plain
/// form (ena, dve, tri, štiri), the names of the numbers:
///   - The number 2 counting nothing is dve. The same listener asked for it where
///     it stands alone; SP 2001, § 11, SSKJ² (ena in ena je dve) and the Jezikovna
///     svetovalnica ("Uporaba dva in dve", 2014) give the rest: 1, 2, 3, 0 1 2,
///     2 plus 2, 2 je sodo, pritisni 2, stran 2, točka 2, slika 2. A feminine or a
///     neuter noun before it asks for dve anyway.
///   - dva stays in what is the name of a thing: glued to letters or after a
///     hyphen or a slash (T2, T-2, PS/2, A-2 liga, 2kg, 2-krat), after another
///     capitalized word (Pentium 2, Voyager 2), after a masculine noun (člen 2,
///     korak 2, monitor 2), before a capital or a sign of a unit (2 %, 2 €).
///   - -ama, -oma, -ema: the dual's instrumental and dative (z dvema žličkama).
///     -ami, -imi: the plural's (s tremi žlicami). -ah, -eh, -ih, -ov, -ev: the
///     locative and genitive (v dveh urah, v treh dneh, dveh mesecev). A word
///     with no ending after a preposition with the genitive (do treh let); -i
///     after z, s, pred, med (pred dvema leti, s tremi otroki); -am, -em after
///     k, h, proti (k trem osebam).
///   - 1 after a preposition: enega, ene (do enega leta, od ene ure), enem, eni
///     (v enem dnevu, v eni uri), enim (pred enim letom), enemu, eni.
///   - Otherwise the nominative and the accusative. 1: -a ena; -o, -e eno (not
///     evro, avto); a consonant or -i en (en koš, en glavni), but ena for the
///     feminine i-stems (ena stvar, ena možnost, ena odločitev). 2: -i, -e dve
///     (dve uri, dve veliki hiši), else dva. 3, 4: -i and the masculines in -je
///     trije, štirje (trije koši, trije ljudje), but tri, štiri for the feminine
///     i-stems (tri stvari) and dni, leti, else tri, štiri (tri koše, tri hiše).
///
/// The hour is feminine (SSKJ² "ura je dve"). On the 209 occurrences of 1 to 4
/// before a noun or an adjective in the corpus SUK 1.1 this gives the form the
/// noun's gender, number and case ask for in 185, the plain forms in 95; of the
/// 906 before anything else it changes 6.
fn agreeing_form(t: Text, start: usize, end: usize, last: &str, alone: bool) -> Option<String> {
    #[rustfmt::skip]
    const LABELS: &[&str] = &[
        "leta", "leto", "letu", "stran", "strani", "slika", "sliki", "sliko", "tabela", "tabeli", "tabelo",
        "skupina", "skupini", "skupino", "člen", "členu", "točka", "točki",
        "točko", "odstavek", "odstavku", "različica", "različici", "različico", "verzija", "verziji",
        "kanal", "kanalu", "številka", "številki", "št", "opomba", "opombi", "priloga", "prilogi",
        "faza", "fazi", "korak", "koraku", "element", "vrstica", "vrstici", "stolpec", "stolpcu",
        "zavihek", "zavihku", "gumb", "gumbu", "datoteka", "datoteki", "mapa", "mapi",
        "stopnja", "stopnji", "nivo", "del", "delu", "tier", "ulica", "ulici", "cesta", "cesti",
    ];
    // What is named by the number's name: feminine labels, and the verbs of
    // choosing a number
    #[rustfmt::skip]
    const NAMES_THE_NUMBER: &[&str] = &[
        "stran", "strani", "slika", "sliki", "sliko", "tabela", "tabeli", "tabelo", "skupina", "skupini", "skupino",
        "točka", "točki", "točko", "različica", "različici", "različico", "verzija", "verziji",
        "številka", "številki", "številko", "št", "opomba", "opombi", "priloga", "prilogi", "faza", "fazi",
        "vrstica", "vrstici", "datoteka", "datoteki", "mapa", "mapi", "stopnja", "stopnji", "ulica", "ulici",
        "cesta", "cesti", "tipka", "tipko", "možnost", "poglavje", "poglavju", "okno", "oknu", "pritisni",
        "pritisnite", "izberi", "izberite", "vnesi", "vnesite", "vtipkaj", "vtipkajte", "tapni", "tapnite", "klikni",
        "kliknite",
    ];
    const MASCULINE_O: &[&str] = &["evro", "avto", "euro", "kino", "radio", "video", "studio", "biro", "metro", "foto"];
    #[rustfmt::skip]
    const MASCULINE_FORMS: &[&str] = &[
        "dni", "dneve", "dnevi", "ljudje", "ljudi", "otroke", "otroci", "možje", "moški",
        "fantje", "bratje", "gostje", "starši", "tedne", "tedni", "mesece", "meseci",
    ];
    #[rustfmt::skip]
    const MASCULINE_JE: &[&str] = &[
        "ljudje", "gostje", "fantje", "kmetje", "možje", "študentje", "sosedje", "lasje", "vodje",
        "bratje", "gospodje", "očetje", "zobje", "škofje", "grofje", "agentje", "dedje",
    ];
    const INSTRUMENTAL: [&str; 3] = ["dvema", "tremi", "štirimi"];
    const GENITIVE: [&str; 3] = ["dveh", "treh", "štirih"];
    const DATIVE: [&str; 3] = ["dvema", "trem", "štirim"];
    const MASCULINE: [&str; 3] = ["dva", "trije", "štirje"];

    // 5 and above have their own oblique forms (petih, petimi), below
    let digit = small_digit(last);
    let five_plus = digit == 0 && is_five_plus(last);
    if digit == 0 && !five_plus {
        return None;
    }
    // Only the number 2 by itself, not sto dva or dva tisoč dva
    let name_of_number = (digit == 2 && alone).then(|| "dve".to_owned());

    if start > 0 && (is_letter_byte(t.at(start - 1)) || t.at(start - 1) == b'-') {
        return None;
    }

    // The word before the number, with only spaces between, in small letters
    let Prev { word: prev, capital: prev_capital, end: before, ahead, .. } = t.prev_word(start);
    let prev = prev.as_str();
    // T- 2, PS/2, PS/ 2: part of a name written with a hyphen or a slash
    if before >= 2 && matches!(t.at(before - 1), b'-' | b'/') && is_letter_byte(t.at(before - 2)) {
        return None;
    }
    // A capital inside a sentence is a name, and the number a part of it
    if prev_capital && !starts_clause(if ahead > 0 { t.at(ahead - 1) } else { b'.' }) {
        return None;
    }
    if digit == 2
        && prev == "je"
        && ahead >= 3
        && t.bytes(ahead - 2, ahead) == b"ra"
        && matches!(t.at(ahead - 3), b'u' | b'U')
        && (ahead == 3 || !is_word_byte(t.at(ahead - 4)))
    {
        return Some("dve".into()); // ura je dve
    }
    if NAMES_THE_NUMBER.contains(&prev) {
        return name_of_number;
    }
    if LABELS.contains(&prev) {
        return None;
    }
    // A masculine noun before it, by its final consonant: monitor 2, disk 2,
    // uporabnik 2, as the labels above (not plus 2, kot 2, pod 2)
    let after_masculine = prev.len() >= 3
        && !matches!(last_digit(prev), b'0'..=b'9' | b'a' | b'e' | b'i' | b'o' | b'u')
        && !NOT_COUNTED.contains(&prev);

    // The word after the number, in small letters
    let mut word = end;
    while matches!(t.at(word), b' ' | b'\t') {
        word += 1;
    }
    // Nothing after it, or a mark glued to it (2. 2, 2)); a letter glued to it is
    // a unit or a compound (2kg, 2-krat, 2-letni), and a capitalized word before
    // it at the head of a clause a name like the one inside (Pentium 2)
    if word == end || word >= t.len() {
        let glued = word == end
            && (is_letter_byte(t.at(word))
                || (t.at(word) == b'-' && is_letter_byte(t.at(word + 1)))
                || matches!(t.at(word), b'%' | b'$'));
        return name_of_number.filter(|_| !glued && !prev_capital && !after_masculine);
    }
    let first = t.at(word);
    let small_letter = first.is_ascii_lowercase() || ["č", "ć", "đ", "š", "ž"].iter().any(|letter| t.has(word, letter));
    let mut stop = word;
    while stop < t.len() && is_word_byte(t.at(stop)) {
        if t.is_digit(stop) {
            stop = word;
            break;
        }
        stop += 1;
    }
    let next = t.str(word, stop);
    let ends = |tails: &[&str]| tails.iter().any(|tail| next.ends_with(tail));

    // Units are not expanded, but 1 and 2 agree with the unit's noun: en kg
    // (kilogram), ena min (minuta), dve min, en MB (megabajt), en tisoč; 3
    // and 4 keep the plain form (tri kg). The capitals of MB, GB, kW count.
    let unit = next.to_ascii_lowercase();
    let feminine_unit = FEMININE_UNITS.contains(&unit.as_str());
    if !next.is_empty() && (feminine_unit || MASCULINE_UNITS.contains(&unit.as_str())) {
        return match digit {
            1 => Some(if feminine_unit { "ena" } else { "en" }.into()),
            2 => Some(if feminine_unit { "dve" } else { "dva" }.into()),
            _ => None,
        };
    }
    if !small_letter {
        // Another number, or a sign of arithmetic: 0 1 2 3, 2 + 2. A capital is
        // a noun of either gender (2 Slovenca), and %, $, € and ° are masculine.
        return name_of_number.filter(|_| matches!(first, b'0'..=b'9' | b'+' | b'=' | b'-' | b'*' | b'/'));
    }
    if stop == word {
        return None; // digits inside the word
    }
    // je, se, ni, ki, mi: no noun; ur: od dveh ur.
    // A verb's -uje, the imperative's -jte: 2 deluje, 2 zamešajte
    if (next.len() < 3 && next != "ur") || ends(&["uje", "jte"]) || NOT_COUNTED.contains(&next) {
        return name_of_number;
    }

    let genitive = WITH_GENITIVE.contains(&prev);
    let locative = WITH_LOCATIVE.contains(&prev);
    let instrumental = WITH_INSTRUMENTAL.contains(&prev);
    let dative = WITH_DATIVE.contains(&prev);
    let before_instrumental_i = ["z", "s", "pred", "med"].contains(&prev);
    let tail = last_digit(next);
    let no_ending = !matches!(tail, b'a' | b'e' | b'i' | b'o' | b'u');
    let feminine_i = is_feminine_i(next);

    if five_plus {
        // 5 and above decline in the oblique cases (SP 2001 pét pêtih, petim,
        // petimi: "v petih urah", "priti s petimi sošolci"; sedmih, osmimi;
        // the compounds on their whole: pred enaindvajsetimi leti), while
        // sto and tisoč stay (stó stôtih tudi neskl., tísoč -- tudi -ih). In
        // the nominative and the accusative the noun phrase is in the genitive
        // plural anyway (pet novih hiš), so -ih and -ov say nothing there:
        // only a preposition, or the -ah, -eh, -ami, -imi no other case has.
        let ending = if ends(&["ami", "imi"]) {
            "imi" // s petimi prsti
        } else if ends(&["ah", "eh"]) || (locative && ends(&["ih"])) {
            "ih" // v petih urah, v petih dneh; po petih letih
        } else if dative && ends(&["am", "em"]) {
            "im" // k petim osebam
        } else if before_instrumental_i && tail == b'i' {
            "imi" // pred petimi leti
        } else if genitive && (no_ending || next == "ljudi") {
            "ih" // do petih let, od petih ur
        } else {
            return None;
        };
        return Some(format!("{}{ending}", oblique_stem(last)));
    }

    if digit == 1 {
        let form = match tail {
            b'a' if genitive => "enega",                        // do enega leta
            b'e' if genitive => "ene",                          // od ene ure
            b'u' if locative => "enem",                         // v enem dnevu
            b'i' if locative => "eni",                          // v eni uri, na eni strani
            _ if instrumental && ends(&["om", "em"]) => "enim", // pred enim letom
            b'u' if dative => "enemu",
            b'i' if dative => "eni",
            // A masculine noun's animate accusative or a masculine or neuter
            // noun's genitive (videl sem enega otroka, the listener's example
            // of 2026-11-03), or a masculine nominative in -a (en kolega):
            // the generated table, for the number 1 alone (a year before a
            // verb, 2001 pooblastila, is not counting); ena knjiga otherwise
            b'a' => match if alone { noun_a_kind(next) } else { None } {
                Some(NounA::Genitive) => "enega",
                Some(NounA::Masculine) => "en",
                None => return None,
            },
            b'u' => return None,
            b'o' if MASCULINE_O.contains(&next) => "en",
            b'o' | b'e' => "eno", // eno polje, eno vprašanje
            // en koš, en glavni; ena stvar
            _ if tail == b'i' || !feminine_i => "en",
            _ => return None,
        };
        return Some(form.into());
    }

    let n = digit - 2;
    if digit == 2 && ends(&["ama", "oma", "ema"]) {
        return Some("dvema".into());
    }
    if ends(&["ami", "imi"]) {
        return Some(INSTRUMENTAL[n].into());
    }
    if ends(&["ah", "eh", "ih", "ov", "ev"]) {
        return Some(GENITIVE[n].into());
    }
    if dative && digit > 2 && ends(&["am", "em"]) {
        return Some(DATIVE[n].into());
    }
    if before_instrumental_i && tail == b'i' {
        return Some(INSTRUMENTAL[n].into());
    }
    if genitive && (no_ending || next == "ljudi") {
        return Some(GENITIVE[n].into());
    }

    if digit == 2 {
        return (matches!(tail, b'i' | b'e') && !MASCULINE_FORMS.contains(&next)).then(|| "dve".into());
    }
    // 3, 4
    (MASCULINE_JE.contains(&next) || (tail == b'i' && !feminine_i && !["dni", "leti", "uri", "ljudi"].contains(&next)))
        .then(|| MASCULINE[n].into())
}

/// Makes the last word of a number's words agree with what the number in
/// [start, end) of the text counts (see `agreeing_form()`).
pub(crate) fn agree(t: Text, start: usize, end: usize, words: &mut String) {
    // The number must end in a word of its own for 1 to 4: dvesto tri, not
    // trinajst or triindvajset
    let token = last_token(words);
    if let Some(form) = agreeing_form(t, start, end, &words[token..], token == 0) {
        say(words, token, &form);
    }
}

/// Whether a number with a unit is an object: after za, na, po, čez, skozi
/// (za 3 €, po 3 € kos) or after a verb's l-participle or stane, znaša
/// (plačal 3 €, stane 4 €, zvišali za 3 %); the subject otherwise (3 € je
/// malo, 3 % ljudi). Only 3 and 4 show the difference (tri evre, trije evri).
fn is_accusative_context(t: Text, prev_word: &Prev) -> bool {
    #[rustfmt::skip]
    const CLITICS: &[&str] = &[
        "sem", "si", "je", "sva", "sta", "smo", "ste", "so", "bo", "bom", "boš", "bova", "bosta", "bomo", "boste",
        "bodo", "bi", "ga", "jo", "mi", "ti", "mu", "ji", "nam", "vam", "jim", "jih", "se",
    ];
    #[rustfmt::skip]
    const BEFORE_OBJECT: &[&str] = &[
        "za", "na", "po", "čez", "skozi", "stane", "stanejo", "znaša", "znašajo", "velja", "veljajo",
        "dobi", "dobite", "dobim", "dobimo", "prejme", "prejmete", "prejmem", "plača", "plačate", "plačam",
        "plačamo", "plačajo", "zviša", "zniža", "zvišajo", "znižajo", "doda", "dodajte", "dodaj",
        "odbije", "odbijte", "prihrani", "prihranite", "prihranimo",
    ];
    const PARTICIPLE: &[&str] = &["al", "il", "el", "ala", "ila", "ali", "ili", "alo", "ilo", "ale", "ile"];
    let before_clitic;
    let mut prev = prev_word.word.as_str();
    if CLITICS.contains(&prev) {
        before_clitic = t.prev_word(prev_word.begin).word; // plačal sem 4 €
        prev = &before_clitic;
    }
    BEFORE_OBJECT.contains(&prev) || (prev.len() >= 4 && PARTICIPLE.iter().any(|tail| prev.ends_with(tail)))
}

/// The case a number with a unit's sign is in, by the word before it: od,
/// do, brez ... the genitive (do petih odstotkov), k, h, proti the dative,
/// pri, ob, o, v the locative (pri dvajsetih stopinjah Celzija), z, s, pred,
/// med, nad, pod the instrumental (s petimi evri), za, na, po and the verbs
/// of `is_accusative_context()` the accusative (za tri evre), the nominative
/// otherwise. A listener asked for the cases on 2026-11-03 ("do 5 %" do
/// petih odstotkov). For a range (od 5 do 10 %) `prev` is the word before
/// the first number.
pub(crate) fn unit_case(t: Text, prev: &Prev) -> Case {
    let word = prev.word.as_str();
    if WITH_GENITIVE.contains(&word) {
        Case::Genitive
    } else if WITH_DATIVE.contains(&word) {
        Case::Dative
    } else if ["pri", "ob", "o", "v"].contains(&word) {
        Case::Locative
    } else if word != "za" && WITH_INSTRUMENTAL.contains(&word) {
        Case::Instrumental
    } else if is_accusative_context(t, prev) {
        Case::Accusative
    } else {
        Case::Nominative
    }
}

fn is_punct_or_end(t: Text, pos: usize) -> bool {
    pos >= t.len()
        || matches!(t.at(pos), b'.' | b',' | b';' | b'!' | b'?' | b':' | b')' | b'\n' | b'"' | b'\'' | 0x80..)
}

/// A number with a unit's sign: the noun in the form the number asks for,
/// the number agreeing with it (en odstotek, dva odstotka, trije odstotki,
/// štirje evri, pet odstotkov, enaindvajset odstotkov, sto en evro, ena
/// stopinja, dve stopinji, tri stopinje, pet stopinj Celzija, nič odstotkov),
/// both in the case the sentence asks for (`unit_case()`): the accusative
/// of an object (tri evre, štiri odstotke, eno stopinjo), the genitive (do
/// enega odstotka, do dveh, treh, petih odstotkov), the dative (k enemu evru,
/// k dvema evroma, k trem, petim evrom), the locative (pri eni stopinji, pri
/// dveh, treh, dvajsetih stopinjah) and the instrumental (z enim evrom, z
/// dvema evroma, s tremi, petimi evri; SP 2001 èn ênega ênemu, dvá dvéh dvéma,
/// tríje tréh trém trémi, pét pêtih pêtim pêtimi, odstótek -tka, évro -a,
/// stopínja -e). nič keeps the genitive plural in every case (SP nìč "pri
/// nič stopinj Celzija se led taja").
/// `number_only`: the words of the number alone, for the first number of a
/// range (od petih do desetih odstotkov).
pub(crate) fn count(r: &NumberReader, digits: &str, unit: &Unit, scale: &str, case: Case, number_only: bool) -> String {
    use Case::*;
    let mut words = r.number_to_words(digits);
    let clean = strip_zeros(digits);
    let fem = unit.feminine;
    let token = last_token(&words);
    let last = if ones_alone(clean) { last_digit(clean) } else { 0 };
    let (number, form): (Option<&str>, &str) = if clean == "0" {
        // nič with the genitive plural in every case (SP 2001 nìč "pri nič
        // stopinj Celzija se led taja")
        (None, unit.genitive_plural)
    } else if last == b'1' {
        match case {
            Nominative => (Some(if fem { "ena" } else { "en" }), unit.singular),
            Accusative => {
                (Some(if fem { "eno" } else { "en" }), if fem { unit.accusative_singular } else { unit.singular })
            }
            Genitive => (Some(if fem { "ene" } else { "enega" }), unit.genitive_singular),
            Dative => (Some(if fem { "eni" } else { "enemu" }), unit.dative_singular),
            Locative => (Some(if fem { "eni" } else { "enem" }), unit.dative_singular),
            Instrumental => (Some(if fem { "eno" } else { "enim" }), unit.instrumental_singular),
        }
    } else if last == b'2' {
        match case {
            Nominative | Accusative => (Some(if fem { "dve" } else { "dva" }), unit.dual),
            Genitive => (Some("dveh"), unit.genitive_plural),
            Locative => (Some("dveh"), unit.plural_locative),
            Dative | Instrumental => (Some("dvema"), unit.dual_dative),
        }
    } else if last == b'3' || last == b'4' {
        let three = last == b'3';
        match case {
            // The subject: trije evri; the object of a verb or of za, na,
            // po: tri evre
            Nominative => ((!fem).then_some(if three { "trije" } else { "štirje" }), unit.plural),
            Accusative => (None, unit.accusative_plural),
            Genitive => (Some(if three { "treh" } else { "štirih" }), unit.genitive_plural),
            Dative => (Some(if three { "trem" } else { "štirim" }), unit.plural_dative),
            Locative => (Some(if three { "treh" } else { "štirih" }), unit.plural_locative),
            Instrumental => (Some(if three { "tremi" } else { "štirimi" }), unit.plural_instrumental),
        }
    } else {
        // 5 and above, the teens, the tens, the compounds (SP 2001 pét pêtih
        // pêtim pêtimi, the compounds on their whole), nič, sto, tisoč and
        // the milijon words, which stay (stó stôtih tudi neskl.)
        let (ending, form) = match case {
            Genitive => ("ih", unit.genitive_plural),
            Locative => ("ih", unit.plural_locative),
            Dative => ("im", unit.plural_dative),
            Instrumental => ("imi", unit.plural_instrumental),
            _ => ("", unit.genitive_plural),
        };
        let last_word = &words[token..];
        if !ending.is_empty() && is_five_plus(last_word) {
            let declined = format!("{}{ending}", oblique_stem(last_word));
            say(&mut words, token, &declined);
        }
        (None, form)
    };
    if let Some(number) = number {
        say(&mut words, token, number);
    }
    if number_only { words } else { format!("{words} {form}{scale}") }
}

/// The first number of a range (od 5 do 10 let, 2 do 3 ure, 1 do 5
/// kilogramov, med 4 in 8 milijonov): the second agrees with the noun
/// (`agree()`), and the first takes the second's case: the same
/// oblique case (od petih do desetih let, starosti dveh do treh let, med
/// dvema in tremi leti, od enega do petih kilogramov), or the plain form when
/// the second is in the nominative or the accusative, because the noun's
/// ending then belongs to the second number (dve do tri ure, ena do dva
/// litra, štiri in osem milijonov; dva and en before a masculine genitive
/// plural in -ov, -ev: dva do pet kilogramov). `right_words` are the second
/// number's words, `prev` the word before the first, `noun` the lowercase
/// word after the second.
fn range_left(r: &NumberReader, digits: &str, right_words: &str, prev: &str, noun: &str) -> String {
    const GENITIVE: [&str; 5] = ["", "", "dveh", "treh", "štirih"];
    const DATIVE: [&str; 5] = ["", "", "dvema", "trem", "štirim"];
    const INSTRUMENTAL: [&str; 5] = ["", "", "dvema", "tremi", "štirimi"];
    let mut words = r.number_to_words(digits);
    let token = last_token(&words);
    let last = words[token..].to_owned();
    let right = &right_words[last_token(right_words)..];
    let digit = small_digit(&last);
    let five_plus = digit == 0 && is_five_plus(&last);

    let case = if right.ends_with("mi") || (right == "dvema" && !WITH_DATIVE.contains(&prev)) {
        Case::Instrumental // tremi, štirimi, petimi
    } else if right.ends_with("ih") || right.ends_with("eh") {
        if WITH_LOCATIVE.contains(&prev) { Case::Locative } else { Case::Genitive }
    } else if right == "dvema" || (right.ends_with("im") && right != "enim") {
        Case::Dative
    } else {
        Case::Nominative
    };
    // The noun's gender for 1 and 2: masculine and neuter before a genitive
    // plural in -ov, -ev, let, dni, mest and a masculine unit's abbreviation
    // (km, kg), feminine otherwise (ur, hiš, minut)
    let masculine = noun.ends_with("ov")
        || noun.ends_with("ev")
        || ["let", "dni", "mest"].contains(&noun)
        || MASCULINE_UNITS.contains(&noun.to_ascii_lowercase().as_str());
    let one = |masculine_form: &'static str, feminine_form: &'static str| {
        if masculine { masculine_form } else { feminine_form }
    };
    let form = match case {
        Case::Nominative if digit == 1 && masculine => "en".to_owned(),
        Case::Nominative if digit == 2 && !masculine && token == 0 => "dve".to_owned(),
        Case::Nominative => return words,
        Case::Instrumental if five_plus => format!("{}imi", oblique_stem(&last)),
        Case::Dative if five_plus => format!("{}im", oblique_stem(&last)),
        _ if five_plus => format!("{}ih", oblique_stem(&last)),
        // sto, tisoč, the milijon words stay
        _ if digit == 0 => return words,
        Case::Genitive => if digit == 1 { one("enega", "ene") } else { GENITIVE[digit] }.to_owned(),
        Case::Locative => if digit == 1 { one("enem", "eni") } else { GENITIVE[digit] }.to_owned(),
        Case::Dative => if digit == 1 { one("enemu", "eni") } else { DATIVE[digit] }.to_owned(),
        Case::Instrumental => if digit == 1 { one("enim", "eno") } else { INSTRUMENTAL[digit] }.to_owned(),
        Case::Accusative => return words,
    };
    say(&mut words, token, &form);
    words
}

/// A Slovenian ordinal before a lowercase word: "1. mesto" prvo mesto,
/// "na 1. mestu" na prvem mestu, "v 21. stoletju" v enaindvajsetem stoletju,
/// "7. oktober" sedmi oktober, "7. oktobra" sedmega oktobra, "v 90. letih"
/// v devetdesetih letih, "1. stran" prva stran, "3. člen" tretji člen. The
/// ending comes from the noun's ending and the preposition before the number
/// (`ordinal_ending()`); "" where nothing is counted, and the cardinal
/// stays. [start, end) are the digits, `after` the position after the period.
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
    let prev = t.prev_word(start);
    let following = t.next_word(next.end);
    let mut ending = ordinal_ending(&prev.word, next.word, lowercase_word(&following));
    if ending.is_empty() {
        return String::new();
    }
    // A year with leta is the genitive of time (1958. leta debitiral: tisoč
    // devetsto oseminpetdesetega leta), a decade the plural (80. leta)
    if ending == "a" && next.word == "leta" && end - start == 4 {
        ending = "ega";
    }
    ordinal(r, number, ending)
}

/// A date without a year at `pos`: 7. 10., 7.10., 05.03. (sedmega desetega);
/// not before a year (the shared path), not in an enumeration (1. 2. in 3.
/// mesto), unless another date follows the conjunction (od 1. 12. do 5.
/// 12., 7. 10. in 8. 10.) or the day and the month are written together
/// (od 1.12. do 5.12.). The words and the end of what they take.
/// `ending`: "ega" for the date something happens on, "i" for the day a
/// sentence names (`names_the_day()`).
fn short_date(r: &NumberReader, t: Text, pos: usize, ending: &str) -> Option<(String, usize)> {
    let date = t.parse_day_month(pos)?;
    let next = t.next_word(date.month_end + 1);
    if next.lowercase
        && ["in", "ali", "do", "ter", "oziroma", "pa"].contains(&next.word)
        && date.month != date.day_end + 1
        && !t.date_at(t.skip_spaces(next.end))
    {
        return None;
    }
    let mut words = join_parts([
        ordinal(r, t.str(pos, date.day_end), ending),
        ordinal(r, t.str(date.month, date.month_end), ending),
    ]);
    // The final period is silent before a lowercase word, as the period
    // of "7. oktobra" is; before anything else it ends the sentence
    if !next.lowercase {
        return Some((words, date.month_end));
    }
    if next.begin <= date.month_end + 1 {
        words.push(' ');
    }
    Some((words, date.month_end + 1))
}

/// Danes je 16. 4. 2026 names the day: the nominative, šestnajsti četrti (a
/// listener's report of 2026-11-17, who also said that Danes smo 16. 4. 2026
/// is right in the genitive, šestnajstega četrtega; SP 2001 has both
/// patterns in its entries, pêti: "~ega januarja <5.>" for the date of an
/// event). As in Croatian, only after the words that name a day and the verb
/// "is": Rodil se je 16. 4. says when, the genitive.
fn names_the_day(t: Text, prev: &Prev) -> bool {
    const DAYS: &[&str] = &["danes", "jutri", "včeraj", "pojutrišnjem", "predvčerajšnjim", "datum", "to", "dan"];
    let before = t.prev_word(prev.begin);
    match prev.word.as_str() {
        "je" | "bo" => DAYS.contains(&before.word.as_str()),
        // Včeraj je bil 15. 4.
        "bil" => before.word == "je" && DAYS.contains(&t.prev_word(before.begin).word.as_str()),
        _ => false,
    }
}

/// Whether an en dash or an em dash stands at `pos`.
fn dash_at(t: Text, pos: usize) -> bool {
    t.has(pos, "\u{2013}") || t.has(pos, "\u{2014}")
}

/// The Slovenian constructs at position `i` (a digit) that the shared loop
/// does not know: a minus, a time after "ob", a range, a date without a year,
/// thousands with spaces, a number compounded with a word (2-krat, 2-letni),
/// a unit's noun. Writes the words into `out` and returns the end of what it
/// consumed, or nothing to leave the number to the shared paths. `in_range`:
/// the right side of a range, whose context the left side has already read.
pub(crate) fn number_at(r: &NumberReader, t: Text, i: usize, out: &mut Out, in_range: bool) -> Option<usize> {
    let length = t.len();
    let end = t.digits_end(i);
    let digits = t.str(i, end);
    let leading_zero = t.at(i) == b'0' && end - i > 1;
    let value = small_value(digits);
    let prev = t.prev_word(i);
    let two_minutes_after = |pos: usize| pos + 2 < length && t.digits_end(pos + 1) == pos + 3;
    let unit_words = |out: &mut Out, digits: &str, (unit, unit_end, scale): UnitAt| {
        out.push_str(&count(r, digits, unit, scale, unit_case(t, &prev), false));
        Some(unit_end)
    };

    if !in_range {
        read_glued_minus(t, i, out);
    }

    // A leading zero is read digit by digit by the shared path (007, 01),
    // except in a date without a year written with one: 05.03., 07. 10.,
    // and in a clock time with a colon, which goes on to the clock times
    // below as without the zero (ob 08:00 ob osmih, ob 08:00 uri ob osmi uri)
    let colon_time = end - i == 2 && t.at(end) == b':' && t.parse_time(i).is_some();
    if leading_zero && !colon_time {
        let nominative = !in_range && names_the_day(t, &prev);
        // A full date with a leading zero that names the day: Danes je
        // 07.10.2026 is the nominative, as without the zero
        if nominative
            && end - i == 2
            && let Some((date_end, groups)) = t.parse_date(i)
        {
            out.push_str(&join_parts([
                ordinal(r, groups[0], "i"),
                ordinal(r, groups[1], "i"),
                r.number_to_words(groups[2]),
            ]));
            return Some(date_end);
        }
        let ending = if nominative { "i" } else { "ega" };
        let (words, stop) = short_date(r, t, i, ending).filter(|_| !in_range && end - i == 2)?;
        out.push_str(&words);
        return Some(stop);
    }

    // A clock time after a preposition of time without ura: ob 8:30 ob osmih
    // trideset, do 17:00 do sedemnajste, pred 10:00 pred deseto
    if !in_range && let Some(stop) = hour_after_preposition(r, t, i, &prev, out) {
        return Some(stop);
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

    // A full date is read by the shared path, in the genitive; the day a
    // sentence names here, in the nominative, its year the same cardinal
    let nominative_date = !in_range && names_the_day(t, &prev);
    if let Some((date_end, groups)) = t.parse_date(i) {
        if !nominative_date {
            return None;
        }
        out.push_str(&join_parts([
            ordinal(r, groups[0], "i"),
            ordinal(r, groups[1], "i"),
            r.number_to_words(groups[2]),
        ]));
        return Some(date_end);
    }

    // A clock time with a colon: ob 17:00 is ob sedemnajstih, but ob 17:00
    // uri ob sedemnajsti uri and ob 10:30 uri ob deseti uri in trideset
    // minut; 24:00 is a time too (parse_time() stops at 23)
    if let Some((time_end, groups)) = t.parse_time(i) {
        if groups.len() == 2
            && let Some(stop) = hour_before_ura(r, t, (i, end, end + 3), groups[1], &prev.word, in_range, out)
        {
            return Some(stop);
        }
        if prev.word == "ob" && groups.len() == 2 && groups[1] == "00" && value >= 1 {
            out.push_str(hour_locative(value));
            return Some(t.digits_end(end + 1));
        }
        if in_range {
            out.push_str(&r.time_to_words(&groups));
            return Some(time_end);
        }
        return None;
    }
    if digits == "24" && t.has(end, ":00") && !t.is_digit(end + 3) && t.at(end + 3) != b':' {
        if let Some(stop) = hour_before_ura(r, t, (i, end, end + 3), "00", &prev.word, in_range, out) {
            return Some(stop);
        }
        if prev.word == "ob" {
            out.push_str("štiriindvajsetih");
        } else {
            out.push_str(&format!("štiriindvajset{}nič nič", part_break()));
        }
        return Some(end + 3);
    }

    // A clock time written with a period, the Slovenian way (SP 2001: ob
    // 8.30, ob 17.00): h or hh, a period, two minute digits, in the context
    // of a time: after ob, od, do, med, okoli, okrog, or before h, ura, ure,
    // uri, uro, ur, zjutraj, zvečer, dopoldne, popoldne, ponoči (za 23.35 uro
    // is za triindvajseto uro in petintrideset minut). Without it 3.14 and
    // 3.2 stay what they were. A period after it with a possible month
    // inside (7.10.) is the date, unless ob stands before it.
    const BEFORE_TIME: &[&str] =
        &["ob", "od", "do", "med", "okoli", "okrog", "približno", "točno", "ure", "uri", "ura"];
    const AFTER_TIME: &[&str] =
        &["h", "ura", "ure", "uri", "uro", "ur", "zjutraj", "zvečer", "dopoldne", "popoldne", "ponoči", "opoldne"];
    if end - i <= 2 && (0..=24).contains(&value) && t.at(end) == b'.' && two_minutes_after(end) {
        let minutes = t.str(end + 1, end + 3);
        let mm = small_value(minutes);
        let after = end + 3;
        let glued_more = matches!(t.at(after), b'.' | b',') && t.is_digit(after + 1);
        let colon_after = t.at(after) == b':';
        let next = t.next_word(after);
        let date_like = t.at(after) == b'.' && prev.word != "ob" && (1..=12).contains(&mm) && (1..=31).contains(&value);
        // ura je 8.30
        let after_ura = (prev.word == "je" || prev.word == "bo")
            && prev.ahead >= 3
            && t.bytes(prev.ahead - 3, prev.ahead).eq_ignore_ascii_case(b"ura")
            && (prev.ahead == 3 || !is_word_byte(t.at(prev.ahead - 4)));
        let context = in_range
            || BEFORE_TIME.contains(&prev.word.as_str())
            || after_ura
            || (next.lowercase && AFTER_TIME.contains(&next.word) && next.begin <= after + 1);
        if mm <= 59 && (value < 24 || mm == 0) && !glued_more && !colon_after && !date_like && context {
            if let Some(stop) = hour_before_ura(r, t, (i, end, after), minutes, &prev.word, in_range, out) {
                return Some(stop);
            }
            if prev.word == "ob" && mm == 0 && value >= 1 {
                out.push_str(hour_locative(value));
            } else {
                out.push_str(&r.time_to_words(&[digits, minutes]));
            }
            return Some(after);
        }
    }

    // A range: 1991–2000, 1991-2000, 2–3 dni, 15.–20. 3., 3.–4. mesto, 15. do
    // 20. marca, 8.30–16.00: "do" between the two, both read alike
    if !in_range {
        let dotted = t.at(end) == b'.' && !t.is_digit(end + 1);
        let left_end = end + usize::from(dotted);
        let mut c = left_end;
        let link = if dash_at(t, c) {
            c += 3;
            Some(Link::Dash)
        } else if t.at(c) == b'-' && !dotted {
            c += 1;
            Some(Link::Hyphen)
        } else {
            // 15. do 20. marca, 1. in 2. mesto; without the periods only do,
            // and in after med, so that both numbers take the case of the
            // preposition before the first: od 5 do 10 %, od petih do
            // desetih odstotkov; med 5 in 10 °C, med petimi in desetimi
            // stopinjami; od 2 do 3 ur, od dveh do treh ur
            let word = t.next_word(c);
            let joins = if dotted {
                ["do", "in", "ali", "ter", "oziroma"].contains(&word.word)
            } else {
                word.word == "do" || (word.word == "in" && prev.word == "med")
            };
            (word.begin > c && word.lowercase && joins).then(|| {
                c = t.skip_spaces(word.end);
                Link::Word(word.word.to_owned())
            })
        };
        if let Some(link) = link.filter(|_| t.is_digit(c) && t.at(c) != b'0') {
            let right_end = t.digits_end(c);
            let right = t.str(c, right_end);
            let join = link.join();
            // The right side a date: 15.–20. 3. 2026, 15. do 20. marca
            if let Some((date_end, groups)) = t.parse_date(c) {
                out.push_str(&(ordinal(r, digits, "ega") + &join + &r.date_to_words(&groups)));
                return Some(date_end);
            }
            if let Some((words, stop)) = short_date(r, t, c, "ega") {
                out.push_str(&(ordinal(r, digits, "ega") + &join + &words));
                return Some(stop);
            }
            if t.at(right_end) == b'.' {
                // A year before a date is no range: od 2006 do 1. aprila 2007
                if !dotted && !link.is_mark() {
                    return None;
                }
                // The right side an ordinal before a lowercase word (3.–4.
                // mesto); silent_dot_end() would stop at the dash before it
                let noun = t.next_word(right_end + 1);
                if right_end - c <= 4 && noun.lowercase && !t.is_digit(right_end + 1) {
                    let after = right_end + 1;
                    let ending = if is_month_genitive(noun.word) {
                        "ega"
                    } else {
                        ordinal_ending(&prev.word, noun.word, lowercase_word(&t.next_word(noun.end)))
                    };
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
            }
            if dotted {
                return None;
            }
            // Both times: 8.30–16.00
            if t.at(end) == b'.' && two_minutes_after(end) {
                let (mut left, mut right) = (Out::default(), Out::default());
                if let Some(stop) = number_at(r, t, c, &mut right, true)
                    && matches!(number_at(r, t, i, &mut left, true), Some(l) if l == left_end || l == end + 3)
                {
                    out.0.extend(left.0);
                    out.push_str(&join);
                    out.0.extend(right.0);
                    return Some(stop);
                }
            }
            // A hyphen joins a range only between small numbers or two
            // years (2-3 dni, 10-15 %, str. 98-102, 1991-2000); 716-844
            // is a telephone number
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
            if let Some((unit, unit_end, scale)) = unit_at(t, right_end) {
                // Both numbers in the unit's case: od petih do desetih
                // odstotkov, med petimi in desetimi stopinjami
                let case = unit_case(t, &prev);
                let left_words = count(r, digits, unit, scale, case, true);
                out.push_str(&(left_words + &join + &count(r, right, unit, scale, case, false)));
                return Some(unit_end);
            }
            let mut right_words = r.number_to_words(right);
            agree(t, i, right_end, &mut right_words);
            let noun = t.next_word(right_end);
            let left_words = range_left(r, digits, &right_words, &prev.word, lowercase_word(&noun));
            out.push_str(&(left_words + &join + &right_words));
            return Some(right_end);
        }
    }

    // Not after another "N." (1. 2. 3.), which would be a list
    let in_list = prev.word.is_empty() && prev.ahead >= 2 && t.at(prev.ahead - 1) == b'.' && t.is_digit(prev.ahead - 2);
    if !in_range
        && !in_list
        && let Some((words, stop)) = short_date(r, t, i, if nominative_date { "i" } else { "ega" })
    {
        out.push_str(&words);
        return Some(stop);
    }

    // Thousands separated by spaces (1 000, 1 000 000, also no-break spaces):
    // one number when the first group has 1 to 3 digits and every other one
    // exactly 3, not after a digit or a plus (+386 41 123 456), not before
    // more digits; a phone number with its leading zero (031 123 456) and
    // single digits (1 2 3) are left as they are
    if end - i <= 3 {
        #[rustfmt::skip]
        const TELEPHONE: &[&str] = &[
            "telefon", "telefonu", "telefona", "tel", "telefonska", "telefonski", "faks", "faksu", "gsm", "mobitel",
            "mobitelu", "številka", "številki", "številko", "št", "račun", "računu", "trr", "iban",
            "pošta", "poštna", "sklic", "klic", "pokličite", "pokliči",
        ];
        let mut pos = end;
        let mut joined = digits.to_owned();
        let mut group_count = 0;
        while group_count < 4 {
            let next = t.skip_spaces(pos);
            if next == pos || t.digits_end(next) != next + 3 {
                break;
            }
            joined += t.str(next, next + 3);
            pos = next + 3;
            group_count += 1;
        }
        // Not after a digit, a plus, a bracket or a slash (+386 41 123 456,
        // (031) 731 940, 01/234 567), not after the word telefon or številka,
        // and not after a name inside a sentence (Pentium 2 266 MHz)
        let before_pos = t.blanks_begin(if prev.word.is_empty() { prev.end } else { prev.begin });
        let before = if before_pos > 0 { t.at(before_pos - 1) } else { b' ' };
        let phone = TELEPHONE.contains(&prev.word.as_str())
            || if prev.word.is_empty() {
                before.is_ascii_digit() || matches!(before, b'+' | b')' | b'/')
            } else {
                before.is_ascii_digit() || last_digit(&prev.word).is_ascii_digit()
            };
        let after_name = prev.capital && !starts_clause(if prev.ahead > 0 { t.at(prev.ahead - 1) } else { b'.' });
        // MP3 128 kbps: the number of a name glued to its letters is not the
        // first group of a number (the maintainer's report of 2026-11-25)
        let glued_to_letters = i > 0 && is_letter_byte(t.at(i - 1));
        if group_count > 0
            && !phone
            && !after_name
            && !glued_to_letters
            && !t.is_separator_between_digits(pos)
            && !(is_letter_byte(t.at(pos)) || t.at(pos) == b':')
        {
            if let Some(unit) = unit_at(t, pos) {
                return unit_words(out, &joined, unit);
            }
            let mut words = r.number_to_words(&joined);
            agree(t, i, pos, &mut words);
            out.push_str(&words);
            return Some(pos);
        }
    }

    // Thousands separated by periods (132.457, the shared Slovenian path)
    // before a unit's sign: 1.000 € is tisoč evrov, 2.500 € dva tisoč petsto
    // evrov
    let thousands_after = |pos: usize| t.at(pos) == b'.' && t.digits_end(pos + 1) == pos + 4;
    if end - i <= 3 && thousands_after(end) {
        let mut pos = end;
        let mut joined = digits.to_owned();
        while thousands_after(pos) {
            joined += t.str(pos + 1, pos + 4);
            pos += 4;
        }
        if !t.is_separator_between_digits(pos)
            && let Some(unit) = unit_at(t, pos)
        {
            return unit_words(out, &joined, unit);
        }
        return None;
    }

    // A number compounded with a word by a hyphen or glued to x: 2-krat
    // dvakrat, 2x dvakrat, 21-krat enaindvajsetkrat, 2-letni dvoletni,
    // 3-letna triletna, 10-letnica desetletnica, 5-dnevni petdnevni, 1-sobno
    // enosobno (SP 2001 dvákrat, dvóléten, desétlétnica), one word so the
    // lexicon's accent applies; not after a letter (A-2), not before a
    // capital (3-D)
    if end - i <= 4 && (i == 0 || !is_word_byte(t.at(i - 1))) && end < length {
        let cardinal = r.number_to_words(digits);
        if t.at(end) == b'-' && t.lowercase_at(end + 1) {
            let mut stop = end + 1;
            while stop < length && is_word_byte(t.at(stop)) && !t.is_digit(stop) {
                stop += 1;
            }
            let word = t.str(end + 1, stop);
            let times = word.starts_with("krat");
            if times || (word.len() >= 4 && !t.is_digit(stop)) {
                out.push_str(&number_stem(digits, &cardinal, times));
                out.push_str(word);
                return Some(stop);
            }
        }
        let times_x = t.at(end) == b'x' && !is_letter_byte(t.at(end + 1));
        let times_sign = t.has(end, "×") && !is_letter_byte(t.at(end + 2));
        if times_x || times_sign {
            out.push_str(&number_stem(digits, &cardinal, true));
            out.push_str("krat");
            return Some(end + if times_x { 1 } else { 2 });
        }
    }

    // The hour after ob: ob 5, ob 5h, ob 13 → ob petih, ob trinajstih, when
    // nothing counted follows (SP 2001 pét: priti ob petih |ob 5h|); ob 5
    // urah and ob 5. uri are read by the agreement and the ordinal, and so is
    // ob 5 uri without the period: a singular locative after ob follows no
    // cardinal (ob peti uri, ob deseti minuti; a listener's report of
    // 2026-11-24), while v 1 uri stays v eni uri
    if prev.word == "ob" && end - i <= 2 && (1..=24).contains(&value) {
        let next = t.next_word(end);
        if next.begin > end && next.lowercase && ["uri", "minuti", "sekundi"].contains(&next.word) {
            out.push_str(&ordinal(r, digits, "i"));
            return Some(end);
        }
        #[rustfmt::skip]
        const AFTER_HOUR: &[&str] = &[
            "zjutraj", "zvečer", "dopoldne", "popoldne", "ponoči", "opoldne", "je", "bo", "bomo", "boste",
            "bodo", "se", "sem", "smo", "ste", "pa", "in", "ali", "ter", "do", "na", "v", "pri", "ob",
        ];
        let h_at = |pos: usize| t.at(pos) == b'h' && !is_word_byte(t.at(pos + 1));
        let mut pos = end;
        let mut hour = false;
        if h_at(pos) {
            hour = true;
            pos += 1;
        } else if !is_word_byte(t.at(pos)) {
            let w = t.skip_spaces(pos);
            if h_at(w) {
                hour = true;
                pos = w + 1;
            } else if !matches!(t.at(pos), b'.' | b',' | b':') {
                let next = t.next_word(pos);
                hour = is_punct_or_end(t, w)
                    || (!next.word.is_empty() && !next.lowercase && !t.is_digit(next.begin))
                    || (next.lowercase && AFTER_HOUR.contains(&next.word));
            }
        }
        if hour {
            out.push_str(hour_locative(value));
            return Some(pos);
        }
    }

    // A decimal with a unit's sign: 1,5 % ena cela pet odstotka, 2,50 € dve
    // celi petdeset evra (the genitive singular after a decimal)
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

    // A whole number with a unit's sign: 1 % en odstotek, 2 € dva evra, 5 °C
    // pet stopinj Celzija
    if !t.is_separator_between_digits(end)
        && let Some(unit) = unit_at(t, end)
    {
        return unit_words(out, digits, unit);
    }
    None
}
