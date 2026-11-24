//! Direct number-normalization regressions.

use super::NumberReader;
use crate::language::Language::{self, *};
use crate::text::marks::{ITEM_BREAK_MARK, LETTER_CODE_MARK, MINOR_BREAK_MARK};

/// The converter's marks: the short pause as a word of its own ...
fn brk() -> String {
    format!(" {MINOR_BREAK_MARK} ")
}
/// ... the longer pause between two items, a cell's contents and its address ...
fn item() -> String {
    format!(" {ITEM_BREAK_MARK} ")
}
/// ... and the mark after the letters of a designation.
const CODE: char = LETTER_CODE_MARK;

fn numbers(language: Language, text: &str) -> String {
    NumberReader::new(language).convert_numbers(text)
}

fn digits(language: Language, text: &str) -> String {
    NumberReader::new(language).convert_digits(text)
}

/// The parts of a time or date with the short pause between them.
fn parts(parts: &[&str]) -> String {
    parts.join(&brk())
}

/// Expected group boundaries are independent of the parser; spell their
/// digits through the public digit-name API so the same matrix covers all
/// four languages.
fn phone_words(n: &NumberReader, groups: &str, by_digit: bool) -> String {
    let (sign, groups) = groups.strip_prefix("+ ").map_or(("", groups), |groups| ("+ ", groups));
    let groups: Vec<String> = groups
        .split('|')
        .map(|group| {
            if by_digit {
                group.bytes().map(|digit| n.digit_name(digit)).collect::<Vec<_>>().join(" ")
            } else {
                let zeros = group.bytes().take_while(|&b| b == b'0').count().min(group.len() - 1);
                let rest = &group[zeros..];
                let last =
                    if rest.len() == 1 { n.digit_name(rest.as_bytes()[0]).to_owned() } else { n.number_to_words(rest) };
                format!("{} ", n.digit_name(b'0')).repeat(zeros) + &last
            }
        })
        .collect();
    format!("{sign}{}", groups.join(", "))
}

#[test]
fn telephone_groups_honor_the_number_mode_in_every_language() {
    let cases = [
        ("+38641453375", "+ 386|41|453|375"),
        ("0038641453375", "00386|41|453|375"),
        ("00 386 41 453 375", "00386|41|453|375"),
        ("+386 41/453-375", "+ 386|41|453|375"),
        ("+\u{a0}386 41 453375", "+ 386|41|453|375"),
        ("+ 386 / 41 - 453375", "+ 386|41|453|375"),
        ("+385911234567", "+ 385|91|123|4567"),
        ("00385911234567", "00385|91|123|4567"),
        ("+385 91 123 4567", "+ 385|91|123|4567"),
        ("+385/91/1234/567", "+ 385|91|123|4567"),
        ("+385-91-1234567", "+ 385|91|123|4567"),
        ("+385\u{a0}91\u{202f}123\u{2009}4567", "+ 385|91|123|4567"),
        ("+385 91 0000001", "+ 385|91|000|0001"),
        ("+385 1 2345672", "+ 385|1|234|5672"),
        ("+38512345672", "+ 385|1|234|5672"),
        ("+38612345678", "+ 386|1|234|5678"),
        ("+38733123456", "+ 387|33|123|456"),
        ("+381111234567", "+ 381|11|123|4567"),
        ("+381230123456", "+ 381|230|123|456"),
        ("+381 230 1234567", "+ 381|230|123|4567"),
        ("+49123123456", "+ 49|12|312|3456"),   // ambiguous compact 9 digits: 2+7
        ("+49 123 123456", "+ 49|123|123|456"), // written code takes precedence
        ("+49 123 1234567", "+ 49|123|123|4567"),
        ("+49 12345 123456", "+ 49|12345|123|456"),
        ("+12025550123", "+ 1|202|555|0123"),
        ("0012025550123", "001|202|555|0123"),
        ("+74951234567", "+ 7|495|123|4567"),
        ("+442079460018", "+ 44|20|7946|0018"),
        ("+393331234567", "+ 39|333|123|4567"),
        ("+390212345678", "+ 39|02|1234|5678"),
        ("+385 (0)91 1234567", "+ 385|091|123|4567"),
        ("+385 (91) 123-4567", "+ 385|91|123|4567"),
        ("+3858001234", "+ 385|800|1234"),
        ("+80012345678", "+ 800|1234|5678"),
    ];
    for language in Language::ALL {
        let n = NumberReader::new(language);
        for by_digit in [false, true] {
            let convert = |text: &str| if by_digit { n.convert_digits(text) } else { n.convert_numbers(text) };
            for (text, groups) in cases {
                let expected = phone_words(&n, groups, by_digit);
                assert_eq!(convert(text), expected, "{text} mode={by_digit} {language:?}");
                assert_eq!(convert(&format!("Telefon: {text}!")), format!("Telefon: {expected}!"));
                // Neither a preceding preposition nor a following noun may
                // decline the final digit or change it to a feminine form.
                assert_eq!(convert(&format!("do {text} ure")), format!("do {expected} ure"));
            }
            for phone in ["031123456", "031/123-456", "031 123 456", "(031) 123456"] {
                // Written domestic area codes are independent of the language.
                assert_eq!(convert(phone), phone_words(&n, "031|123|456", by_digit));
            }
        }
    }
    assert_eq!(digits(Slovenian, "+386 1 2222222"), "+ tri osem šest, ena, dve dve dve, dve dve dve dve");
    assert_eq!(digits(Slovenian, "031123456"), "nič tri ena, ena dve tri, štiri pet šest");
    assert_eq!(
        numbers(Croatian, "+385 91 1234567"),
        "+ tristo osamdeset pet, devedeset jedan, sto dvadeset tri, četiri tisuće petsto šezdeset sedam"
    );
}

/// A listener's report of November 2026: NVDA at the punctuation level "none"
/// takes the plus sign away before the synthesizer sees the text (and says
/// "plus" for it at the higher levels), so "+385 91 123 4567" had no pauses
/// and "+385911234567" was one long number.
#[test]
fn telephone_numbers_whose_plus_sign_a_screen_reader_took() {
    let cases = [
        ("385 91 123 4567", "385|91|123|4567"),
        ("385911234567", "385|91|123|4567"),
        ("385 91 1234567", "385|91|123|4567"),
        ("385/91/123-4567", "385|91|123|4567"),
        ("385 98 123 456", "385|98|123|456"),
        ("385 1 2345 672", "385|1|234|5672"),
        ("38512345672", "385|1|234|5672"),
        ("385 21 123 456", "385|21|123|456"),
        ("385 (0)91 1234567", "385|091|123|4567"),
        ("38641453375", "386|41|453|375"),
        ("386 41 453 375", "386|41|453|375"),
        ("386 1 234 5678", "386|1|234|5678"),
        ("387 33 123 456", "387|33|123|456"),
        ("387 61 123 456", "387|61|123|456"),
        ("381 11 123 4567", "381|11|123|4567"),
        ("381 64 123 4567", "381|64|123|4567"),
    ];
    for language in Language::ALL {
        let n = NumberReader::new(language);
        for by_digit in [false, true] {
            let convert = |text: &str| if by_digit { n.convert_digits(text) } else { n.convert_numbers(text) };
            for (text, groups) in cases {
                let expected = phone_words(&n, groups, by_digit);
                assert_eq!(convert(text), expected, "{text} mode={by_digit} {language:?}");
                // What NVDA sends: a space where the sign was, two after the item
                assert_eq!(convert(&format!(" {text}  ")), format!(" {expected}  "));
                assert_eq!(convert(&format!("Telefon: {text}!")), format!("Telefon: {expected}!"));
                assert_eq!(convert(&format!("Nazovi {text} sutra")), format!("Nazovi {expected} sutra"));
                // The sign read out as a word, in either script
                assert_eq!(convert(&format!("plus {text}")), format!("plus {expected}"));
                assert_eq!(convert(&format!("Plus  {text}")), format!("Plus  {expected}"));
                assert_eq!(convert(&format!("плус {text}")), format!("плус {expected}"));
            }
            // After the word any country's number is one, as after the sign
            assert_eq!(
                convert("plus 44 20 7946 0018"),
                format!("plus {}", phone_words(&n, "44|20|7946|0018", by_digit))
            );
            assert_eq!(convert("plus 12025550123"), format!("plus {}", phone_words(&n, "1|202|555|0123", by_digit)));
        }
    }
}

#[test]
fn telephone_recognition_leaves_other_numeric_formats_alone() {
    let n = NumberReader::new(Croatian);
    assert_eq!(n.convert_numbers("+12"), "+dvanaest");
    assert_eq!(n.convert_numbers("+123456"), format!("+{}", n.number_to_words("123456")));
    assert_eq!(n.convert_numbers("+1234567890123456"), format!("+{}", n.number_to_words("1234567890123456")));
    // A long number that is no national number of the four countries stays a number
    assert_eq!(n.convert_numbers("38561453375"), n.number_to_words("38561453375"));
    assert_eq!(n.convert_numbers("49123123456"), n.number_to_words("49123123456"));
    for text in ["+1234567,5", "+385911234567.5", "ID+385911234567", "+385911234567kg", "+99912345678"] {
        assert!(!n.convert_numbers(text).contains(", "), "{text}");
    }
    assert_eq!(n.convert_numbers("1 000 000"), "milijun");
    assert_eq!(n.convert_numbers("12:30"), parts(&["dvanaest", "trideset"]));
    assert_eq!(n.convert_numbers("7.10.2026"), parts(&["sedmog", "desetog", "dvije tisuće dvadeset šeste"]));
    assert_eq!(n.convert_numbers("112"), "sto dvanaest");
    assert_eq!(n.convert_digits("112"), "jedan jedan dva");
    assert_eq!(
        n.convert_digits("+ 1 2 3,5 6,7 8 9,8 7 6"),
        "+ jedan dva tri, pet šest, sedam osam devet, osam sedam šest"
    );
    // Without the plus sign (see the test above) nothing else becomes a phone number
    for text in [
        "385",
        "385 kuna",
        "Imam 385 911 kuna",
        "385 911 234",
        "385 912 345 678",
        "3859112345",
        "38591123456789",
        "12 385 91 123 4567",
        "2 plus 385 91 1234567",
        "3,385 91 123 4567",
        "44 20 7946 0018",
        "1 202 555 0123",
        "38 591 123 4567",
        "385 61 234 567",
        "386 81 234 567",
        "387 21 234 567",
        "381 41 234 567",
    ] {
        assert!(!n.convert_numbers(text).contains(", "), "{text}");
        assert!(!n.convert_digits(text).contains(", "), "{text}");
    }
    // Newlines terminate phone candidates; multiple numbers remain separate.
    assert_eq!(
        n.convert_digits("+385911234567\n+38641453375"),
        format!("{}\n{}", phone_words(&n, "+ 385|91|123|4567", true), phone_words(&n, "+ 386|41|453|375", true))
    );
}

#[test]
fn slovenian_ordinal_noun_agreement() {
    for noun in ["dete", "letališče", "ognjišče", "igrišče", "središče", "nadstropje", "ime", "dekle"] {
        assert_eq!(numbers(Slovenian, &format!("1 {noun} ni 1. {noun}")), format!("eno {noun} ni prvo {noun}"));
        assert_eq!(numbers(Slovenian, &format!("3. {noun}")), format!("tretje {noun}"));
    }
    for (text, expected) in [
        ("1. ura", "prva ura"),
        ("do 1. ure", "do prve ure"),
        ("k 1. uri", "k prvi uri"),
        ("za 1. uro", "za prvo uro"),
        ("ob 1. uri", "ob prvi uri"),
        ("s 1. uro", "s prvo uro"),
        ("do 1. hiše", "do prve hiše"),
        ("s 3. hišo", "s tretjo hišo"),
        ("k 1. hišam", "k prvim hišam"),
        ("s 1. hišami", "s prvimi hišami"),
        ("v 1. hišah", "v prvih hišah"),
        ("s 1. hišama", "s prvima hišama"),
        ("do 1. letališča", "do prvega letališča"),
        ("k 1. letališču", "k prvemu letališču"),
        ("na 1. letališču", "na prvem letališču"),
        ("pred 1. letališčem", "pred prvim letališčem"),
    ] {
        assert_eq!(numbers(Slovenian, text), expected, "{text}");
    }
}

#[test]
fn croatian_serbian_bosnian_ordinal_cases() {
    for language in [Croatian, Serbian, Bosnian] {
        let year = match language {
            Croatian => "dvije tisuće dvadeset",
            Serbian => "dve hiljade dvadeset",
            _ => "dvije hiljade dvadeset",
        };
        for (text, expected) in [
            ("1. godina", "prva godina"),
            ("do 1. godine", "do prve godine"),
            ("prema 1. godini", "prema prvoj godini"),
            ("u 1. godinu", "u prvu godinu"),
            ("u 1. godini", "u prvoj godini"),
            ("s 1. godinom", "s prvom godinom"),
            ("1. ženo", "prva ženo"),
            ("1. škole", "prve škole"),
            ("s 3. školom", "s trećom školom"),
            ("u 1. sobu", "u prvu sobu"),
            ("na 1. nagradu", "na prvu nagradu"),
            ("s 1. prijateljem", "s prvim prijateljem"),
            ("do 1. razreda", "do prvog razreda"),
            ("u 1. razredu", "u prvom razredu"),
            ("1. godine", "prve godine"),
            ("u 1. godinama", "u prvim godinama"),
            ("2026. godine", "YEAR šeste godine"),
            ("2026. godina", "YEAR šesta godina"),
            ("u 2026. godini", "u YEAR šestoj godini"),
        ] {
            assert_eq!(numbers(language, text), expected.replace("YEAR", year), "{text} {language:?}");
        }
        let minute = if language == Serbian { "s prvim minutom" } else { "s prvom minutom" };
        assert_eq!(numbers(language, "s 1. minutom"), minute);
        assert_eq!(numbers(language, "у 1. години"), "у prvoj години");
    }
}

#[test]
fn currency_formats_normalize_grouping_signs_and_affixes() {
    let amount = "dvije tisuće sto pedeset zarez nula nula";
    let million = "milijun dvjesto trideset četiri tisuće petsto šezdeset sedam zarez osamdeset devet";
    for (text, expected) in [
        ("2.150,00", "AMOUNT"),
        ("Stanje 2.150,00", "Stanje AMOUNT"),
        ("Račun 2.150,00", "Račun AMOUNT"),
        ("Saldo:2.150,00", "Saldo:AMOUNT"),
        ("-2.150,00", "minus AMOUNT"),
        ("−2.150,00", "minus AMOUNT"),
        ("- 2.150,00", "minus AMOUNT"),
        ("Stanje:-2.150,00", "Stanje:minus AMOUNT"),
        ("2 150,00", "AMOUNT"),
        ("2\u{a0}150,00", "AMOUNT"),
        ("2\u{202f}150,00", "AMOUNT"),
        ("2\u{2009}150,00", "AMOUNT"),
        ("2.150,00 €", "AMOUNT eura"),
        ("2.150,00€", "AMOUNT eura"),
        ("€2.150,00", "AMOUNT eura"),
        ("EUR 2.150,00", "AMOUNT eura"),
        ("2.150,00 EUR", "AMOUNT eura"),
        ("€ 2150.00", "AMOUNT eura"),
        ("2,150.00 €", "AMOUNT eura"),
        ("2'150.00 EUR", "AMOUNT eura"),
        ("2’150.00 EUR", "AMOUNT eura"),
        ("$2,150.00", "AMOUNT dolara"),
        ("$2.150,00", "AMOUNT dolara"),
        ("USD2,150.00", "AMOUNT dolara"),
        ("2,150.00USD", "AMOUNT dolara"),
        ("-$2,150.00", "minus AMOUNT dolara"),
        ("$-2,150.00", "minus AMOUNT dolara"),
        ("$ − 2,150.00", "minus AMOUNT dolara"),
        ("−2.150,00 €", "minus AMOUNT eura"),
        ("2.150,00€-", "minus AMOUNT eura"),
        ("$2,150.00-", "minus AMOUNT dolara"),
        ("($2,150.00)", "minus AMOUNT dolara"),
        ("$(2,150.00)", "minus AMOUNT dolara"),
        ("(2.150,00 €)", "minus AMOUNT eura"),
        ("+2.150,00 €", "plus AMOUNT eura"),
        ("2.150,00 €!", "AMOUNT eura!"),
        ("2.150,00- €", "minus AMOUNT eura"),
        ("2.150,00 € −", "minus AMOUNT eura"),
        ("$2,150", "dvije tisuće sto pedeset dolara"),
        ("€2.150", "dvije tisuće sto pedeset eura"),
        ("€2,150", "dva zarez jedan pet nula eura"),
        ("$1", "jedan dolar"),
        ("1€", "jedan euro"),
        ("21 €", "dvadeset jedan euro"),
        ("$0.00", "nula zarez nula nula dolara"),
        ("-€0,01", "minus nula zarez nula jedan eura"),
        ("$12.05", "dvanaest zarez nula pet dolara"),
        ("€1.234.567,89", "MILLION eura"),
        ("$1,234,567.89", "MILLION dolara"),
    ] {
        let expected = expected.replace("AMOUNT", amount).replace("MILLION", million);
        assert_eq!(numbers(Croatian, text), expected, "{text}");
    }
    assert_eq!(numbers(Croatian, "-2.500,00"), "minus dvije tisuće petsto zarez nula nula");
    assert_eq!(numbers(Croatian, "Stanje 2.150"), "Stanje dvije tisuće sto pedeset");
    assert_eq!(digits(Croatian, "-2.500,00 €"), "minus dva pet nula nula zarez nula nula eura");
    assert_eq!(digits(Croatian, "$2,150.00"), "dva jedan pet nula zarez nula nula dolara");
    assert_eq!(digits(Croatian, "− 2500"), "minus dva pet nula nula");
}

#[test]
fn currency_reading_keeps_the_language_and_ordinary_numeric_meanings() {
    assert_eq!(numbers(Serbian, "-2.150,00 €"), "minus dve hiljade sto pedeset zarez nula nula evra");
    assert_eq!(numbers(Bosnian, "$2,150.00"), "dvije hiljade sto pedeset zarez nula nula dolara");
    assert_eq!(numbers(Slovenian, "2.150,00 €"), "dva tisoč sto petdeset celih nič nič evra");
    assert_eq!(numbers(Slovenian, "do €1"), "do enega evra");
    assert_eq!(numbers(Slovenian, "s 3 €"), "s tremi evri");
    assert_eq!(digits(Slovenian, "€2,00"), "dve vejica nič nič evra");
    let n = NumberReader::new(Croatian);
    assert_eq!(n.convert_numbers("3.14"), "tri\u{2024}četrnaest");
    assert_eq!(n.convert_numbers("7.10.2026"), parts(&["sedmog", "desetog", "dvije tisuće dvadeset šeste"]));
    assert_eq!(n.convert_numbers("12:30"), parts(&["dvanaest", "trideset"]));
    assert_eq!(
        n.convert_numbers("192.168.1.1"),
        "sto devedeset dva\u{2024}sto šezdeset osam\u{2024}jedan\u{2024}jedan"
    );
    assert_eq!(n.convert_numbers("T-2"), "T-dva");
    assert_eq!(n.convert_numbers("1€2€"), "jedan euro dva eura");
    assert!(!n.convert_numbers("1991-2000").contains("minus"));
    assert_eq!(n.convert_digits("091/123-4567"), phone_words(&n, "091|123|4567", true));
    assert!(!n.convert_numbers("(2.150,00)").contains("minus"));
    assert!(!n.convert_numbers("$2.00-3.00").contains("minus"));
    assert_eq!(
        n.convert_numbers("2.150,00\n3.150,00"),
        "dvije tisuće sto pedeset zarez nula nula\ntri tisuće sto pedeset zarez nula nula"
    );
}

/// A listener's report of November 2026: a screen reader says a cell's contents
/// and then its address, and "Marjan A17" was heard as "Marijana 17"; the hours
/// ran into the minutes and the day into the month. The converter leaves a
/// short pause (`MINOR_BREAK_MARK`) between them and marks the letters of the
/// address as letters (`LETTER_CODE_MARK`).
#[test]
fn clock_times_and_numeric_dates_have_a_short_pause_between_their_parts() {
    let n = NumberReader::new(Croatian);
    assert_eq!(n.convert_numbers("12:30:45"), parts(&["dvanaest", "trideset", "četrdeset pet"]));
    assert_eq!(n.convert_numbers("9:05"), parts(&["devet", "nula pet"]));
    assert_eq!(n.convert_numbers("23:59"), parts(&["dvadeset tri", "pedeset devet"]));
    assert_eq!(n.convert_numbers("24:00"), parts(&["dvadeset četiri", "nula nula"]));
    assert_eq!(
        n.convert_numbers("Sastanak je u 12:30 u uredu."),
        parts(&["Sastanak je u dvanaest", "trideset u uredu."])
    );
    assert_eq!(n.convert_numbers("u 8.30"), parts(&["u osam", "trideset"]));
    assert_eq!(n.convert_numbers("od 8:30 do 16:00"), parts(&["od osam", "trideset do šesnaest", "nula nula"]));
    // Written together or with spaces, a date is the same date
    let date = parts(&["desetog", "jedanaestog", "dvije tisuće dvadeset šeste."]);
    assert_eq!(n.convert_numbers("10.11.2026."), date);
    assert_eq!(n.convert_numbers("10. 11. 2026."), date);
    assert_eq!(
        n.convert_numbers("Danas je 10.11.2026."),
        parts(&["Danas je deseti", "jedanaesti", "dvije tisuće dvadeset šeste."])
    );
    assert_eq!(n.convert_numbers("5.3."), parts(&["petog", "trećeg."]));
    assert_eq!(n.convert_numbers("5. 3."), parts(&["petog", "trećeg."]));
    // The digit mode reads a time and a date as the whole-number mode does
    assert_eq!(n.convert_digits("12:30"), parts(&["dvanaest", "trideset"]));
    assert_eq!(n.convert_digits("10.11.2026."), date);
    // What is no time and no date has no pause
    for text in ["3.14", "192.168.1.1", "7. listopada 2026.", "1.000.000"] {
        assert!(!n.convert_numbers(text).contains(MINOR_BREAK_MARK), "{text}");
    }

    assert_eq!(numbers(Serbian, "10.11.2026."), parts(&["desetog", "jedanaestog", "dve hiljade dvadeset šeste."]));
    assert_eq!(numbers(Slovenian, "12:30"), parts(&["dvanajst", "trideset"]));
    assert_eq!(numbers(Slovenian, "ob 17:00"), "ob sedemnajstih");
    assert_eq!(numbers(Slovenian, "10.11.2026"), parts(&["desetega", "enajstega", "dva tisoč šestindvajset"]));
    assert_eq!(numbers(Slovenian, "7. 10."), parts(&["sedmega", "desetega."]));
}

/// Dates the converter misread until November 2026, found while the pauses
/// were added: the period after a year ended the sentence in the middle of
/// one, leading zeros and ranges of dates were read digit by digit or with the
/// period by name.
#[test]
fn numeric_dates_inside_a_sentence_with_leading_zeros_and_in_ranges() {
    let n = NumberReader::new(Croatian);
    let y2026 = "dvije tisuće dvadeset šeste";
    let date = |end: &str| parts(&["desetog", "jedanaestog", &format!("{y2026}{end}")]);
    // The period after the year is the ordinal's before a lowercase word
    assert_eq!(n.convert_numbers("10.11.2026. godine"), date(" godine"));
    assert_eq!(n.convert_numbers("10. 11. 2026. godine"), date(" godine"));
    assert_eq!(
        n.convert_numbers("Rođen je 10.11.1985. u Zagrebu."),
        parts(&["Rođen je desetog", "jedanaestog", "tisuću devetsto osamdeset pete u Zagrebu."])
    );
    // (glued to the word, as a screen reader glues the parts of a label: set off twice, like "7.listopada")
    assert_eq!(n.convert_numbers("10.11.2026.godine"), date("  godine"));
    assert_eq!(
        n.convert_numbers("od 1.12.2026. do 5.12.2026."),
        parts(&["od prvog", "dvanaestog", &format!("{y2026} do petog"), "dvanaestog", &format!("{y2026}.")])
    );
    // ... and the sentence's before a capital, a number or the end
    assert_eq!(
        n.convert_numbers("Došao je 10.11.2026. Zatim je otišao."),
        parts(&["Došao je desetog", "jedanaestog", &format!("{y2026}. Zatim je otišao.")])
    );
    assert_eq!(
        n.convert_numbers("10.11.2026. 12:30"),
        parts(&["desetog", "jedanaestog", &format!("{y2026}. dvanaest"), "trideset"])
    );
    // Leading zeros
    assert_eq!(n.convert_numbers("05.03."), parts(&["petog", "trećeg."]));
    assert_eq!(n.convert_numbers("05. 03."), parts(&["petog", "trećeg."]));
    assert_eq!(n.convert_numbers("07.10.2026."), parts(&["sedmog", "desetog", &format!("{y2026}.")]));
    assert_eq!(n.convert_numbers("Danas je 07.10.2026."), parts(&["Danas je sedmi", "deseti", &format!("{y2026}.")]));
    assert_eq!(n.convert_numbers("u 08.30"), parts(&["u osam", "trideset"]));
    assert_eq!(n.convert_numbers("007"), "nula nula sedam");
    assert_eq!(n.convert_numbers("verzija 05.03"), "verzija nula pet\u{2024}nula tri");
    assert_eq!(n.convert_numbers("05.000"), "nula pet\u{2024}nula nula nula");
    // Ranges of dates; an enumeration of ordinals stays one
    let range = parts(&["od prvog", "dvanaestog do petog", "dvanaestog."]);
    assert_eq!(n.convert_numbers("od 1.12. do 5.12."), range);
    assert_eq!(n.convert_numbers("od 1. 12. do 5. 12."), range);
    assert_eq!(n.convert_numbers("od 01.12. do 05.12."), range);
    assert_eq!(n.convert_numbers("7. 10. i 8. 10."), parts(&["sedmog", "desetog i osmog", "desetog."]));
    assert_eq!(n.convert_numbers("1. 2. i 3. mjesto"), "jedan. drugo i treće mjesto");
    assert_eq!(n.convert_numbers("u 7. i 8. razredu"), "u sedmom i osmom razredu");
    // A hyphen after an ordinal's period stands for the dash
    assert_eq!(n.convert_numbers("10.-15.11.2026."), n.convert_numbers("10.\u{2013}15.11.2026."));
    assert_eq!(
        n.convert_numbers("10.-15.11.2026."),
        parts(&["desetog do petnaestog", "jedanaestog", &format!("{y2026}.")])
    );
    assert_eq!(n.convert_numbers("3.-4. mjesto"), "treće do četvrto mjesto");
    assert_eq!(n.convert_numbers("1991.-2000."), n.convert_numbers("1991.\u{2013}2000."));
    assert_eq!(n.convert_numbers("2-3 dana"), "dva do tri dana");
    assert_eq!(n.convert_numbers("T-2"), "T-dva");

    assert_eq!(numbers(Slovenian, "05.03."), parts(&["petega", "tretjega."]));
    let range = parts(&["od prvega", "dvanajstega do petega", "dvanajstega."]);
    assert_eq!(numbers(Slovenian, "od 1.12. do 5.12."), range);
    assert_eq!(numbers(Slovenian, "od 1. 12. do 5. 12."), range);
    assert_eq!(numbers(Slovenian, "1. 2. in 3. mesto"), "ena. drugo in tretje mesto");
    // Danes je 16. 4. 2026 names the day, the nominative; Danes smo 16. 4.
    // 2026 and a date something happens on, the genitive (a listener's report
    // of 2026-11-17), written together or apart
    let named = parts(&["Danes je šestnajsti", "četrti", "dva tisoč šestindvajset."]);
    assert_eq!(numbers(Slovenian, "Danes je 16.4.2026."), named);
    assert_eq!(numbers(Slovenian, "Danes je 16. 4. 2026."), named);
    assert_eq!(numbers(Slovenian, "Danes je 16.4."), parts(&["Danes je šestnajsti", "četrti."]));
    assert_eq!(numbers(Slovenian, "Danes je 05.03."), parts(&["Danes je peti", "tretji."]));
    assert_eq!(numbers(Slovenian, "Jutri bo 17. 4."), parts(&["Jutri bo sedemnajsti", "četrti."]));
    assert_eq!(numbers(Slovenian, "Včeraj je bil 15.4."), parts(&["Včeraj je bil petnajsti", "četrti."]));
    assert_eq!(
        numbers(Slovenian, "Danes smo 16.4.2026."),
        parts(&["Danes smo šestnajstega", "četrtega", "dva tisoč šestindvajset."])
    );
    assert_eq!(
        numbers(Slovenian, "Rodil se je 16.4.2026."),
        parts(&["Rodil se je šestnajstega", "četrtega", "dva tisoč šestindvajset."])
    );
    assert_eq!(
        numbers(Slovenian, "od 2.11.2026 do 5.11.2026."),
        parts(&[
            "od drugega",
            "enajstega",
            "dva tisoč šestindvajset do petega",
            "enajstega",
            "dva tisoč šestindvajset."
        ])
    );
    // Slovenian writes no period after the year; one that is there ends the sentence
    assert_eq!(
        numbers(Slovenian, "7. 10. 2026. leta"),
        parts(&["sedmega", "desetega", "dva tisoč šestindvajset. leta"])
    );
}

#[test]
fn a_cells_address_is_set_off_from_the_cells_contents() {
    let n = NumberReader::new(Croatian);
    let item = item();
    assert_eq!(n.convert_numbers("Marjan A17"), format!("Marjan{item}A{CODE} sedamnaest"));
    assert_eq!(n.convert_numbers("Marjan   A17  "), format!("Marjan{item}A{CODE} sedamnaest  "));
    assert_eq!(n.convert_numbers("Ukupno B2"), format!("Ukupno{item}B{CODE} dva"));
    assert_eq!(n.convert_numbers("1250 C3"), format!("tisuću dvjesto pedeset{item}C{CODE} tri"));
    assert_eq!(n.convert_numbers("Marjan AB12"), format!("Marjan{item}AB{CODE} dvanaest"));
    assert_eq!(n.convert_numbers("\"Marjan\" A17"), format!("\"Marjan\"{item}A{CODE} sedamnaest"));
    // The address first: no pause before a word, one before a number (A20 with a 5 in it is not A25)
    assert_eq!(n.convert_numbers("A17 Marjan"), format!("A{CODE} sedamnaest Marjan"));
    assert_eq!(n.convert_numbers("A20 5"), format!("A{CODE} dvadeset{item}pet"));
    assert_eq!(n.convert_numbers("A1 B2"), format!("A{CODE} jedan{item}B{CODE} dva"));
    // No pause at the head of a clause, after a preposition or conjunction, in a range or a formula
    assert_eq!(n.convert_numbers("A17"), format!("A{CODE} sedamnaest"));
    assert_eq!(n.convert_numbers("Marjan, A17"), format!("Marjan, A{CODE} sedamnaest"));
    assert!(!n.convert_numbers("10.11.2026. A17").contains(&format!("{item}A")));
    assert_eq!(n.convert_numbers("od A1 do B5"), format!("od A{CODE} jedan do B{CODE} pet"));
    assert_eq!(n.convert_numbers("u A1 i B2"), format!("u A{CODE} jedan i B{CODE} dva"));
    assert_eq!(n.convert_numbers("У A1"), format!("У A{CODE} jedan"));
    assert_eq!(n.convert_numbers("A1:B5"), format!("A{CODE} jedan:B{CODE} pet"));
    assert_eq!(n.convert_numbers("=A1+B2"), format!("=A{CODE} jedan+B{CODE} dva"));
    // A designation inside a longer one keeps its letters as letters but has no pause
    assert_eq!(n.convert_numbers("papir A4-format"), format!("papir A{CODE} četiri-format"));
    assert_eq!(n.convert_numbers("voda H2O"), format!("voda H{CODE} dva O"));
    // Not designations: small letters, three letters, the second half of a longer
    // name, a time or a decimal after the letters, a leading zero
    for text in ["u20:03", "5kg", "TOP10", "HRT1", "R2-D2", "PS/2", "U20:03", "V1.5", "A01", "Marjan a17"] {
        let marks = n.convert_numbers(text).matches(CODE).count();
        assert_eq!(marks, usize::from(text == "R2-D2"), "{text}"); // R2 is one; -D2 is not
    }
    assert_eq!(n.convert_numbers("Gledam utrku F1 na HRT2"), format!("Gledam utrku{item}F{CODE} jedan na HRT dva"));

    // NVDA in Excel says the address as two words, each item followed by two
    // spaces and joined by the driver with a third (the driver's log,
    // November 2026: 'Marijan  ' and 'A 3  '): the same address
    assert_eq!(n.convert_numbers("Marijan   A 3  "), format!("Marijan{item}A{CODE} tri  "));
    assert_eq!(n.convert_numbers("Ivan   A 4  "), format!("Ivan{item}A{CODE} četiri  "));
    assert_eq!(n.convert_numbers("A 5  "), format!("A{CODE} pet  "));
    assert_eq!(n.convert_numbers("A 5"), format!("A{CODE} pet"));
    assert_eq!(n.convert_numbers("1250   C 3  "), format!("tisuću dvjesto pedeset{item}C{CODE} tri  "));
    assert_eq!(n.convert_numbers("Marijan   AB 12  "), format!("Marijan{item}AB{CODE} dvanaest  "));
    assert_eq!(n.convert_numbers("Marijan   A 3  odabrano  "), format!("Marijan{item}A{CODE} tri  odabrano  "));
    assert_eq!(
        n.convert_numbers("Marijan A 3\nIvan A 4"),
        format!("Marijan{item}A{CODE} tri\nIvan{item}A{CODE} četiri")
    );
    assert_eq!(n.convert_digits("Marijan   A 17  "), format!("Marijan{item}A{CODE} jedan sedam  "));
    // Written apart inside a sentence the letters are the words they spell
    for text in [
        "U 5 sati dolazim",
        "A 3 su došla",
        "Dolazim u 5",
        "Idem na 5 dana",
        "NA 5 mjesta",
        "U 5.",
        "U 5, a ne u 6",
        "DOLAZIM U 5",
        "A  5",
        "A 05",
        "ABC 5",
        "a 5  ",
        "U 5 sati  ",
    ] {
        assert!(!n.convert_numbers(text).contains(CODE), "{text}");
        assert!(!n.convert_numbers(text).contains(&item), "{text}");
        assert!(!n.convert_digits(text).contains(CODE), "{text}");
    }

    // The digit mode: the same marks, and the digits set off from the letters
    assert_eq!(n.convert_digits("Marjan A17"), format!("Marjan{item}A{CODE} jedan sedam"));
    assert_eq!(n.convert_digits("COVID19"), "COVID jedan devet");
    assert_eq!(n.convert_digits("u20:03"), parts(&["u dvadeset", "nula tri"]));

    assert_eq!(numbers(Slovenian, "Marjan A2"), format!("Marjan{item}A{CODE} dva"));
    assert_eq!(numbers(Slovenian, "v A1"), format!("v A{CODE} ena"));
}

/// Arbitrary text never panics: positions past the end, characters cut by
/// the separators the converter looks for, signs with nothing around them.
#[test]
fn any_text_is_safe() {
    let pieces = [
        "",
        "0",
        "1",
        "2",
        "5",
        "7",
        "12",
        "24",
        "007",
        "2026",
        "385911234567",
        ".",
        ",",
        ":",
        "-",
        "–",
        "−",
        "+",
        "(",
        ")",
        "/",
        " ",
        "\u{a0}",
        "\u{2009}",
        "\n",
        "€",
        "$",
        "%",
        "°",
        "°C",
        "EUR",
        "x",
        "×",
        "h",
        "A",
        "AB",
        "š",
        "Ž",
        "ob",
        "do",
        "od",
        "je",
        "u",
        "s",
        "plus",
        "плус",
        "ura",
        "leta",
        "godine",
        "krat",
        "letni",
        "’",
        "'",
        "\0",
        "\u{FDD0}",
        "😀",
    ];
    for language in Language::ALL {
        let n = NumberReader::new(language);
        for (a, b) in pieces.iter().flat_map(|a| pieces.iter().map(move |b| (a, b))) {
            for c in ["", "5", ". ", " leta", "€", "-3 dni", ".10. in"] {
                let text = format!("{a}{b}{c}");
                n.convert_numbers(&text);
                n.convert_digits(&text);
                n.convert_numbers(&format!("{a} {b}{c} 3. 4."));
            }
        }
        let long = "9".repeat(400);
        assert!(!n.convert_numbers(&long).is_empty());
        n.convert_numbers(&format!("{long}.{long},{long} 1.{long}.2026 {long}:30"));
    }
}

/// Every written form of 18 November 2026 that is read as the dotted one.
const DATE_FORMS: [&str; 15] = [
    "18.11.2026",
    "18. 11. 2026",
    "18/11/2026",
    "18-11-2026",
    "18\u{2011}11\u{2011}2026",
    "2026-11-18",
    "2026/11/18",
    "2026.11.18",
    "11/18/2026",
    "18.XI.2026",
    "18. XI. 2026",
    "18/XI/2026",
    "18-XI-2026",
    "18.\u{A0}11.\u{A0}2026",
    "18.\u{202F}11.\u{2009}2026",
];

/// A date in every written form is read as the dotted one (a listener's
/// report of 2026-11-18): slashes, hyphens, the year first as in ISO 8601,
/// the month first where the second number is no month, a Roman month,
/// typeset spaces after the periods; and what only looks like one is left.
/// In all four languages alike (Slovenian, Serbian and Bosnian were checked
/// for six of the forms until 2026-11-23).
#[test]
fn a_date_is_read_in_every_written_form() {
    let n = NumberReader::new(Croatian);
    let date = parts(&["osamnaestog", "jedanaestog", "dvije tisuće dvadeset šeste"]);
    for text in DATE_FORMS {
        assert_eq!(n.convert_numbers(text), date, "{text}");
        assert_eq!(n.convert_digits(text), date, "{text}");
        // The period after the year, the sentence around the date
        assert_eq!(n.convert_numbers(&format!("{text}.")), format!("{date}."), "{text}.");
        assert_eq!(
            n.convert_numbers(&format!("Danas je {text}.")),
            parts(&["Danas je osamnaesti", "jedanaesti", "dvije tisuće dvadeset šeste."]),
            "{text}"
        );
        assert_eq!(n.convert_numbers(&format!("Rođen je {text} u Zagrebu")), format!("Rođen je {date} u Zagrebu"));
    }
    // Leading zeros, which the telephone numbers took, and a two-digit year
    let born = parts(&["petog", "trećeg", "tisuću devetsto osamdeset pete"]);
    for text in ["05.03.1985", "05/03/1985", "05-03-1985", "1985-03-05", "5/3/1985", "05.III.1985"] {
        assert_eq!(n.convert_numbers(text), born, "{text}");
    }
    assert_eq!(n.convert_numbers("18/11/26"), parts(&["osamnaestog", "jedanaestog", "dvadeset šeste"]));
    assert_eq!(n.convert_numbers("18. XI. 26."), parts(&["osamnaestog", "jedanaestog", "dvadeset šeste."]));
    // The time of ISO 8601, and a time after a date
    assert_eq!(n.convert_numbers("2026-11-18T10:30"), format!("{date} {}", parts(&["deset", "trideset"])),);
    assert_eq!(n.convert_numbers("18/11/2026 10:30"), format!("{date} {}", parts(&["deset", "trideset"])));
    // A day and a month with typeset spaces
    assert_eq!(n.convert_numbers("5.\u{A0}3."), parts(&["petog", "trećeg."]));

    // The other languages, every form
    let slovenian = parts(&["osemnajstega", "enajstega", "dva tisoč šestindvajset"]);
    for (language, date) in [
        (Serbian, parts(&["osamnaestog", "jedanaestog", "dve hiljade dvadeset šeste"])),
        (Bosnian, parts(&["osamnaestog", "jedanaestog", "dvije hiljade dvadeset šeste"])),
        (Slovenian, slovenian.clone()),
    ] {
        let today = if language == Slovenian { "Danes je" } else { "Danas je" };
        let dotted_sentence = numbers(language, &format!("{today} 18.11.2026."));
        for text in DATE_FORMS {
            assert_eq!(numbers(language, text), date, "{language:?}: {text}");
            assert_eq!(digits(language, text), date, "{language:?}: {text}");
            assert_eq!(numbers(language, &format!("{text}.")), format!("{date}."), "{language:?}: {text}.");
            assert_eq!(numbers(language, &format!("{today} {text}.")), dotted_sentence, "{language:?}: {text}");
        }
    }
    for text in DATE_FORMS {
        assert_eq!(
            numbers(Slovenian, &format!("Danes je {text}")),
            parts(&["Danes je osemnajsti", "enajsti", "dva tisoč šestindvajset"]),
            "{text}"
        );
        // The sentence of the report of 2026-11-23
        assert_eq!(
            numbers(Slovenian, &format!("Rojen je {text} v Ljubljani.")),
            format!("Rojen je {slovenian} v Ljubljani."),
            "{text}"
        );
    }
    // Slovenian: leading zeros, a two-digit year, the time of ISO 8601 and
    // a day and a month with typeset spaces are read as their dotted forms
    let dotted = |text| numbers(Slovenian, text);
    assert!(dotted("5.3.1985").contains(MINOR_BREAK_MARK));
    for text in ["05.03.1985", "05/03/1985", "05-03-1985", "1985-03-05", "5/3/1985", "05.III.1985"] {
        assert_eq!(dotted(text), dotted("5.3.1985"), "{text}");
    }
    assert_eq!(dotted("18/11/26"), dotted("18.11.26"));
    assert_eq!(dotted("18. XI. 26"), dotted("18. 11. 26"));
    assert_eq!(dotted("2026-11-18T10:30"), format!("{slovenian} {}", dotted("10:30")));
    assert_eq!(dotted("5.\u{A0}3."), dotted("5. 3."));
    // Slovenian: the day a sentence names, with a leading zero too
    assert_eq!(
        numbers(Slovenian, "Danes je 07.10.2026"),
        parts(&["Danes je sedmi", "deseti", "dva tisoč šestindvajset"])
    );
    assert_eq!(
        numbers(Slovenian, "Rodil se je 07.10.2026"),
        parts(&["Rodil se je sedmega", "desetega", "dva tisoč šestindvajset"])
    );

    // The year after a month's name is the ordinal of "godine"
    assert_eq!(n.convert_numbers("7. listopada 2026."), "sedmog listopada dvije tisuće dvadeset šeste.");
    assert_eq!(n.convert_numbers("7. listopada 2026"), "sedmog listopada dvije tisuće dvadeset šeste");
    assert_eq!(n.convert_numbers("7. listopada 2026. godine"), "sedmog listopada dvije tisuće dvadeset šeste godine");
    assert_eq!(
        n.convert_numbers("Rođen je 7. listopada 1985. u Zagrebu"),
        "Rođen je sedmog listopada tisuću devetsto osamdeset pete u Zagrebu"
    );
    assert_eq!(numbers(Serbian, "18. novembra 2026."), "osamnaestog novembra dve hiljade dvadeset šeste.");
    assert_eq!(numbers(Bosnian, "18. novembra 2026."), "osamnaestog novembra dvije hiljade dvadeset šeste.");
    assert_eq!(numbers(Slovenian, "18. novembra 2026"), "osemnajstega novembra dva tisoč šestindvajset");

    // No dates: fractions, ranges, scores, versions, addresses, telephone
    // numbers, numbers glued to more
    for text in [
        "1/2",
        "24/7",
        "50/50",
        "PS/2",
        "10-15",
        "2-3 dana",
        "716-844",
        "10-12-14",
        "1/2/2026/3",
        "2026-11-18-4",
        "32/11/2026",
        "18/13/2026",
        "13/13/2026",
        "2026-13-18",
        "18/11/202",
        "3.14",
        "192.168.1.1",
        "10.10.10.10",
        "verzija 1.12.10.3",
        "1.000.000",
        "A18/11/2026",
        "18/11/2026a",
        "1. I. Kant",
        "5. V.",
    ] {
        assert!(!n.convert_numbers(text).contains(MINOR_BREAK_MARK), "{text}: {}", n.convert_numbers(text));
    }
    assert_eq!(n.convert_numbers("tel. 05-03-1985"), n.convert_numbers("tel. 05 03 1985"));
    assert!(!numbers(Slovenian, "10.10.10.10").contains(MINOR_BREAK_MARK));
}

/// A period between digits that the NVDA add-on carried past NVDA's
/// Slovenian symbol table as a one dot leader is a period again; a dot
/// leader anywhere else is left (the number converter writes it between
/// words).
#[test]
fn a_one_dot_leader_between_digits_is_a_period() {
    use super::restore_carried_points as restore;
    let c = super::DOT_LEADER;
    for (carried, written) in [
        (format!("18{c}11{c}2026"), "18.11.2026".to_owned()),
        (format!("Rojen je 18{c}11{c}2026 v Ljubljani."), "Rojen je 18.11.2026 v Ljubljani.".to_owned()),
        (format!("3{c}14"), "3.14".to_owned()),
        (format!("192{c}168{c}1{c}1"), "192.168.1.1".to_owned()),
        (format!("ob 8{c}30"), "ob 8.30".to_owned()),
        (format!("1{c}5{c}"), format!("1.5{c}")),
    ] {
        assert_eq!(restore(&carried), written, "{carried}");
    }
    for text in [format!("{c}5"), format!("3{c} 4"), format!("a{c}5"), format!("tri{c}četrnaest"), "3.14".into()] {
        assert_eq!(restore(&text), text);
    }
    assert!(matches!(restore("18.11.2026"), std::borrow::Cow::Borrowed(_)));
}

/// A Slovenian listener's report of 2026-11-24: a whole hour before uri is
/// the ordinal (Sestanek bo ob 10:00 uri: ob deseti uri, not ob desetih uri),
/// without uri the locative of the cardinal (Sestanek bo ob 11:00: ob
/// enajstih). SSKJ² úra "sonce vzide ob peti uri [5. uri] in dvajset minut",
/// "vstajati pred šesto (uro)", "med tretjo in četrto (uro)"; SP 2001
/// sedemnájsti "oditi ob ~i uri", úra "ob dvajseti ~i in dvajset minut
/// ‹20.20›"; the Jezikovna svetovalnica on the minutes, which stay in the
/// nominative (ob enajsti uri in dve minuti).
#[test]
fn slovenian_listeners_report_of_2026_11_24_whole_hours_before_uri() {
    for (text, expected) in [
        // The listener's sentences
        ("Sestanek bo ob 10:00 uri.", "Sestanek bo ob deseti uri."),
        ("Pridi ob 17:00 uri.", "Pridi ob sedemnajsti uri."),
        (
            "Nisi prišel ob 19:00 uri, kot sva bila dogovorjena.",
            "Nisi prišel ob devetnajsti uri, kot sva bila dogovorjena.",
        ),
        ("Sestanek bo ob 11:00.", "Sestanek bo ob enajstih."),
        ("Ne morem priti ob 13:00.", "Ne morem priti ob trinajstih."),
        // The same hour written otherwise
        ("ob 10.00 uri", "ob deseti uri"),
        ("ob 10 uri", "ob deseti uri"),
        ("ob 10 minuti", "ob deseti minuti"),
        ("ob 1:00 uri", "ob prvi uri"),
        ("ob 2:00 uri", "ob drugi uri"),
        ("ob 24:00 uri", "ob štiriindvajseti uri"),
        // A leading zero, as digital clocks write the hour
        ("ob 08:00 uri", "ob osmi uri"),
        ("ob 08:00", "ob osmih"),
        ("ob 05:07 uri", "ob peti uri in sedem minut"),
        ("od 08:00 do 16:00 ure", "od osme do šestnajste ure"),
        // The other cases of ura
        ("do 17:00 ure", "do sedemnajste ure"),
        ("pred 10:00 uro", "pred deseto uro"),
        ("po 10:00 uri", "po deseti uri"),
        ("okoli 10.00 ure", "okoli desete ure"),
        // Two hours joined before ura
        ("od 8:00 do 16:00 ure", "od osme do šestnajste ure"),
        ("od 8.00 do 16.00 ure", "od osme do šestnajste ure"),
        ("8:00–16:00 ure", "osme do šestnajste ure"),
        ("ob 10:00 in 11:00 uri", "ob deseti in enajsti uri"),
        ("med 8:00 in 10:00 uro", "med osmo in deseto uro"),
        // An hour with minutes before ura: the ura before the minutes
        ("ob 10:30 uri", "ob deseti uri in trideset minut"),
        ("ob 11:02 uri", "ob enajsti uri in dve minuti"),
        ("ob 10:01 uri", "ob deseti uri in ena minuta"),
        ("ob 10:03 uri", "ob deseti uri in tri minute"),
        ("ob 10:21 uri", "ob deseti uri in enaindvajset minut"),
        ("Okoli 6.30 ure so odprli.", "Okoli šeste ure in trideset minut so odprli."),
        (
            "Glasovanje je predvideno za 23.35 uro.",
            "Glasovanje je predvideno za triindvajseto uro in petintrideset minut.",
        ),
        // Unchanged
        ("ob 17:00", "ob sedemnajstih"),
        ("ob 5h", "ob petih"),
        ("ob 10. uri", "ob deseti uri"),
        ("ob 5 urah", "ob petih urah"),
        ("v 1 uri", "v eni uri"),
        ("za 3 ure", "za tri ure"),
        ("2 uri", "dve uri"),
        ("007", "nič nič sedem"),
    ] {
        assert_eq!(numbers(Slovenian, text), expected, "{text}");
    }
    let brk = brk();
    assert_eq!(numbers(Slovenian, "ob 8.30"), format!("ob osmih{brk}trideset"));
    assert_eq!(numbers(Slovenian, "od 8.30 do 16.00"), format!("od osmih{brk}trideset do šestnajstih"));
    assert_eq!(numbers(Slovenian, "Ura je 8.30"), format!("Ura je osem{brk}trideset"));
    assert_eq!(numbers(Slovenian, "05.03."), format!("petega{brk}tretjega."));
    assert_eq!(numbers(Slovenian, "08:30–16:00"), format!("osem{brk}trideset do šestnajst{brk}nič nič"));
}

/// The Slovenian listener's answers of 2026-11-24 to the questions of the
/// report above: without ura, ob 8:30 is ob osmih trideset (the Jezikovna
/// svetovalnica, 2016: ob 8.30 "manj formalno pa 'ob osmih trideset'"), do
/// 17:00 do sedemnajste and pred 10:00 pred deseto, the feminine ordinal of
/// the left-out ura after the other prepositions (SSKJ² úra "vstajati pred
/// šesto (uro); med tretjo in četrto (uro)"; SP 2001 deséta "poklicati po
/// ~i", ôsma "priti pred ~o", dvájseta "čakati do ~e"; Dobrovoljc 2017 and
/// 2021), one o'clock the feminine ena (Dobrovoljc 2017 "med dvanajsto in
/// (pol) eno"; SSKJ² úra "od polnoči do ene"), and the cardinal after okoli
/// and okrog (SP 2001 okóli "priti ~ desetih"; SSKJ² okrog "pride okrog
/// enajstih, enajste (ure)").
#[test]
fn slovenian_listeners_answers_of_2026_11_24_hours_without_ura() {
    let brk = brk();
    for (text, expected) in [
        // The listener's answers
        ("ob 8:30", format!("ob osmih{brk}trideset")),
        ("do 17:00", "do sedemnajste".to_owned()),
        ("pred 10:00", "pred deseto".to_owned()),
        ("Sestanek bo ob 8:30.", format!("Sestanek bo ob osmih{brk}trideset.")),
        ("Pridi do 17:00, prosim.", "Pridi do sedemnajste, prosim.".to_owned()),
        // ob and minutes, however the time is written
        ("ob 8.30", format!("ob osmih{brk}trideset")),
        ("ob 08:30", format!("ob osmih{brk}trideset")),
        ("Ob 20.15 se začne.", format!("Ob dvajsetih{brk}petnajst se začne.")),
        ("ob 1:30", format!("ob enih{brk}trideset")),
        ("ob 12:02", "ob dvanajstih in dve minuti".to_owned()),
        ("ob 8:05", "ob osmih in pet minut".to_owned()),
        ("ob 8:30 ali 9:30", format!("ob osmih{brk}trideset ali devetih{brk}trideset")),
        // The other prepositions with a whole hour
        ("od 8:00", "od osme".to_owned()),
        ("pred 10.00", "pred deseto".to_owned()),
        ("do 13.00", "do trinajste".to_owned()),
        ("po 10:00", "po deseti".to_owned()),
        ("po 2:00", "po drugi".to_owned()),
        ("do 24:00", "do štiriindvajsete".to_owned()),
        ("okoli 10:00", "okoli desetih".to_owned()),
        ("okrog 11.00", "okrog enajstih".to_owned()),
        // One o'clock
        ("do 1:00", "do ene".to_owned()),
        ("po 1:00", "po eni".to_owned()),
        ("pred 1:00", "pred eno".to_owned()),
        ("med 12:00 in 1:00", "med dvanajsto in eno".to_owned()),
        // Two whole hours
        ("od 8:00 do 16:00", "od osme do šestnajste".to_owned()),
        ("od 8.00 do 16.00", "od osme do šestnajste".to_owned()),
        ("od 8:00–16:00", "od osme do šestnajste".to_owned()),
        ("med 8:00 in 10:00", "med osmo in deseto".to_owned()),
        ("ob 10:00 in 11:00", "ob desetih in enajstih".to_owned()),
        ("do 17:00 in 3 ure", "do sedemnajste in tri ure".to_owned()),
        // Unchanged: with ura, a range without a preposition, other prepositions
        ("ob 10:30 uri", "ob deseti uri in trideset minut".to_owned()),
        ("ob 11:00", "ob enajstih".to_owned()),
        ("do 17:00 ure", "do sedemnajste ure".to_owned()),
        ("od 8:00 do 16:00 ure", "od osme do šestnajste ure".to_owned()),
        ("8:00 do 16:00", format!("osem{brk}nič nič do šestnajst{brk}nič nič")),
        ("8:00–16:00", format!("osem{brk}nič nič do šestnajst{brk}nič nič")),
        ("za 10:00", format!("za deset{brk}nič nič")),
        ("proti 10:00", format!("proti deset{brk}nič nič")),
        ("ob 8:30:15", format!("ob osem{brk}trideset{brk}petnajst")),
        ("ob 0:30", format!("ob nič{brk}trideset")),
        ("do 5.11.", format!("do petega{brk}enajstega.")),
    ] {
        assert_eq!(numbers(Slovenian, text), expected, "{text}");
    }
    // Croatian keeps its own clock
    assert_eq!(numbers(Croatian, "do 17:00"), format!("do sedamnaest{brk}nula nula"));
}

/// The Slovenian listener's answers of 2026-11-24, second round, on a time
/// with minutes and no ura: do 17:30 is do sedemnajste trideset, od 8:30 do
/// 16:00 "točno je od osmih trideset do šestnajstih", ob 8:05 ob osmih in pet
/// minut (the Jezikovna svetovalnica, "Zapis nepolne oz. necele ure", 2016:
/// "ob osmih in trideset minut"), okoli 10:00 okoli desetih stays. The cases
/// the listener was not asked about follow the same patterns: the feminine
/// ordinal and the minutes after do, od, pred, med, po; both hours in -ih in
/// a range with minutes after od or ob; minutes under ten with in and minut.
#[test]
fn slovenian_listeners_answers_of_2026_11_24_hours_with_minutes_without_ura() {
    let brk = brk();
    for (text, expected) in [
        // The listener's answers
        ("do 17:30", format!("do sedemnajste{brk}trideset")),
        ("od 8:30 do 16:00", format!("od osmih{brk}trideset do šestnajstih")),
        ("okoli 10:00", "okoli desetih".to_owned()),
        ("ob 8:05", "ob osmih in pet minut".to_owned()),
        ("Odprto od 8:30 do 16:00.", format!("Odprto od osmih{brk}trideset do šestnajstih.")),
        ("Pridi do 17:30, prosim.", format!("Pridi do sedemnajste{brk}trideset, prosim.")),
        // Minutes under ten
        ("ob 8:01", "ob osmih in ena minuta".to_owned()),
        ("ob 12:02", "ob dvanajstih in dve minuti".to_owned()),
        ("ob 08:03", "ob osmih in tri minute".to_owned()),
        ("ob 8.09", "ob osmih in devet minut".to_owned()),
        ("ob 8:10", format!("ob osmih{brk}deset")),
        ("do 17:05", "do sedemnajste in pet minut".to_owned()),
        // The ordinal and the minutes after the other prepositions
        ("do 17.30", format!("do sedemnajste{brk}trideset")),
        ("od 8:30", format!("od osme{brk}trideset")),
        ("pred 10:30", format!("pred deseto{brk}trideset")),
        ("po 10:15", format!("po deseti{brk}petnajst")),
        ("do 1:30", format!("do ene{brk}trideset")),
        ("med 8:30 in 10:00", format!("med osmo{brk}trideset in deseto")),
        ("med 8:00 in 10:30", format!("med osmo in deseto{brk}trideset")),
        ("okoli 10:30", format!("okoli desetih{brk}trideset")),
        ("okrog 11.45", format!("okrog enajstih{brk}petinštirideset")),
        // A range with minutes on either side
        ("od 8:00 do 16:30", format!("od osmih do šestnajstih{brk}trideset")),
        ("od 8.30 do 16.00", format!("od osmih{brk}trideset do šestnajstih")),
        ("od 8:30–16:00", format!("od osmih{brk}trideset do šestnajstih")),
        ("od 8:30 do 16:45", format!("od osmih{brk}trideset do šestnajstih{brk}petinštirideset")),
        ("od 8:05 do 16:00", "od osmih in pet minut do šestnajstih".to_owned()),
        ("ob 8:30–9:00", format!("ob osmih{brk}trideset do devetih")),
        // Unchanged: whole hours, ura, no preposition, a date
        ("od 8:00 do 16:00", "od osme do šestnajste".to_owned()),
        ("ob 8:30", format!("ob osmih{brk}trideset")),
        ("ob 10:30 uri", "ob deseti uri in trideset minut".to_owned()),
        ("do 17:30 ure", "do sedemnajste ure in trideset minut".to_owned()),
        ("8:30 do 16:00", format!("osem{brk}trideset do šestnajst{brk}nič nič")),
        ("8:30–16:00", format!("osem{brk}trideset do šestnajst{brk}nič nič")),
        ("za 10:30", format!("za deset{brk}trideset")),
        ("do 5.11.", format!("do petega{brk}enajstega.")),
        ("od 1.12. do 5.12.", format!("od prvega{brk}dvanajstega do petega{brk}dvanajstega.")),
        ("do 17:30:15", format!("do sedemnajst{brk}trideset{brk}petnajst")),
    ] {
        assert_eq!(numbers(Slovenian, text), expected, "{text}");
    }
    // Croatian keeps its own clock
    assert_eq!(numbers(Croatian, "do 17:30"), format!("do sedamnaest{brk}trideset"));
}
