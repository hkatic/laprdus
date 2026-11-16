//! Telephone numbers: the country code, the area or network code and the
//! subscriber number in groups, with the comma pause between them. The
//! groups bypass grammatical agreement in both reading modes.

use super::NumberReader;
use super::scan::{Out, Text, is_word_byte};
use crate::language::Language;

/// ITU E.164: country codes are prefix-free, 1--3 digits; the international
/// prefix is not part of the 15-digit limit. Code assignments and the regional
/// formatting exceptions below were checked against Google's libphonenumber
/// resources/PhoneNumberMetadata.xml (2026-11-09). This is a reading heuristic,
/// not a validator of assigned subscriber numbers.
#[rustfmt::skip]
const COUNTRY_CODES: &[&str] = &[
    "1", "7", "20", "27", "30", "31", "32", "33", "34", "36", "39", "40", "41", "43", "44", "45",
    "46", "47", "48", "49", "51", "52", "53", "54", "55", "56", "57", "58", "60", "61", "62", "63",
    "64", "65", "66", "81", "82", "84", "86", "90", "91", "92", "93", "94", "95", "98",
    "211", "212", "213", "216", "218", "220", "221", "222", "223", "224", "225", "226", "227",
    "228", "229", "230", "231", "232", "233", "234", "235", "236", "237", "238", "239", "240",
    "241", "242", "243", "244", "245", "246", "247", "248", "249", "250", "251", "252", "253",
    "254", "255", "256", "257", "258", "260", "261", "262", "263", "264", "265", "266", "267",
    "268", "269", "290", "291", "297", "298", "299", "350", "351", "352", "353", "354", "355",
    "356", "357", "358", "359", "370", "371", "372", "373", "374", "375", "376", "377", "378",
    "380", "381", "382", "383", "385", "386", "387", "389", "420", "421", "423", "500", "501",
    "502", "503", "504", "505", "506", "507", "508", "509", "590", "591", "592", "593", "594",
    "595", "596", "597", "598", "599", "670", "672", "673", "674", "675", "676", "677", "678",
    "679", "680", "681", "682", "683", "685", "686", "687", "688", "689", "690", "691", "692",
    "800", "808", "850", "852", "853", "855", "856", "870", "878", "880", "881", "882", "883",
    "886", "888", "960", "961", "962", "963", "964", "965", "966", "967", "968", "970", "971",
    "972", "973", "974", "975", "976", "977", "979", "992", "993", "994", "995", "996", "998",
];

/// The length of the country code a number starts with, 0 for none.
fn country_length(digits: &str) -> usize {
    (1..=3.min(digits.len().saturating_sub(1))).find(|&n| COUNTRY_CODES.contains(&&digits[..n])).unwrap_or(0)
}

/// The country code of a domestic number written in the language.
fn home_country(language: Language) -> &'static str {
    match language {
        Language::Croatian => "385",
        Language::Serbian => "381",
        Language::Bosnian => "387",
        Language::Slovenian => "386",
    }
}

/// Known regional destination codes take precedence over length guesses for
/// compact numbers. Written code boundaries elsewhere are retained (Germany,
/// for example, has 2--5 digit area codes). Italy's initial zero is significant.
fn area_length(country: &str, national: &str) -> usize {
    let digits = national.as_bytes();
    let Some(&first) = digits.first() else { return 0 };
    let prefix = |n: usize| &national[..n.min(national.len())];
    match country {
        "1" | "7" => return 3,
        "385" => {
            return match first {
                b'1' => 1,
                b'8' => 3,
                _ => 2,
            };
        }
        "387" => return 2,
        "381" => return if first >= b'7' || ["230", "280", "290", "390"].contains(&prefix(3)) { 3 } else { 2 },
        "386" => {
            const MOBILE: &[&str] = &["30", "31", "40", "41", "43", "51", "64", "65", "68", "69", "70", "71"];
            let prefix = prefix(2);
            if prefix == "59" || (first == b'8' && prefix != "80" && prefix != "89") {
                return 3;
            }
            if MOBILE.contains(&prefix) || first == b'8' || first == b'9' {
                return 2;
            }
            if (b'1'..=b'7').contains(&first) {
                return 1;
            }
        }
        "39" if first == b'0' => {
            if matches!(digits.get(1), Some(b'2' | b'6')) {
                return 2;
            }
            if let Some(third) = digits.get(2) {
                return if b"0159".contains(third) { 3 } else { 4 };
            }
        }
        "44" if first == b'2' => return 2, // London etc.: eight subscriber digits
        "44" if first == b'7' => return 4, // mobile: six subscriber digits
        // No geographic area code in these international service numbers.
        "800" | "808" | "870" | "878" | "881" | "882" | "883" | "888" | "979" => return 0,
        _ => {}
    }
    // The requested common 2/3-digit code + 6/7-digit subscriber pattern.
    // Nine digits are ambiguous: default to 2+7 unless written otherwise.
    match national.len() {
        8 | 9 => 2,
        10 => 3,
        _ => 0,
    }
}

/// A number written without its plus sign: a screen reader takes the sign away
/// before the synthesizer sees the text (NVDA at the punctuation level "none"
/// sends "+385 91 123 4567" as " 385 91 123 4567", at "some" and above as
/// "plus 385 ..."; a listener's report of November 2026). Without the sign
/// only the country codes of the four languages are read as a telephone
/// number, and only when what follows the code is a national number of that
/// country by its first digit and its length (the trunk zero left out):
///   385  1 + 7 digits (Zagreb), 2x-5x and 9x + 6 or 7
///   386  1-7 + 7 digits (geographic 1-5 and 7, mobile 30-71)
///   387  3x-5x + 6 digits, 6x + 6 or 7
///   381  1x-3x and 6x + 6 or 7 digits
/// so a price, a year or a quantity that begins with 385 stays a number.
fn bare_international_ok(country: &str, national: &str) -> bool {
    let Some(&first) = national.as_bytes().first() else { return false };
    let n = national.len();
    let eight_or_nine = n == 8 || n == 9;
    match country {
        "385" if first == b'1' => n == 8,
        "385" => matches!(first, b'2'..=b'5' | b'9') && eight_or_nine,
        "386" => (b'1'..=b'7').contains(&first) && n == 8,
        "387" if first == b'6' => eight_or_nine,
        "387" => (b'3'..=b'5').contains(&first) && n == 8,
        "381" => matches!(first, b'1'..=b'3' | b'6') && eight_or_nine,
        _ => false,
    }
}

/// The word a screen reader says for the plus sign, right before `pos` (spaces
/// skipped): plus, Plus, PLUS, and the Cyrillic плус. `Some(true)` after a
/// number: "2 plus 385" is a sum.
fn plus_word_before(t: Text, pos: usize) -> Option<bool> {
    let pos = t.blanks_begin(pos);
    let start = if pos >= 4 && t.bytes(pos - 4, pos).eq_ignore_ascii_case(b"plus") {
        pos - 4
    } else if ["плус", "Плус", "ПЛУС"].iter().any(|word| t.has_before(pos, word)) {
        pos - 8
    } else {
        return None;
    };
    if start > 0 && (is_word_byte(t.at(start - 1)) || t.at(start - 1) == b'_') {
        return None;
    }
    let before = t.blanks_begin(start);
    Some(before > 0 && t.is_digit(before - 1))
}

/// A telephone number at `i`: `+` / `00` international numbers and domestic
/// leading-zero numbers, written with compact digits, spaces, slashes, dashes
/// and parenthesized codes. Writes its groups and returns its end.
pub(super) fn convert(r: &NumberReader, t: Text, i: usize, out: &mut Out, by_digit: bool) -> Option<usize> {
    let mut head = t.spaces_begin(i);
    let plus = head > 0 && t.at(head - 1) == b'+';
    head = if plus { head - 1 } else { i };
    let outer_bracket = !plus && i > 0 && t.at(i - 1) == b'(';
    if outer_bracket {
        head = i - 1;
    }
    if head > 0 && (is_word_byte(t.at(head - 1)) || matches!(t.at(head - 1), b'+' | b'-' | b'/' | b'_')) {
        return None;
    }
    let plus_word = if !plus && !outer_bracket && head == i && head > 0 { plus_word_before(t, i) } else { None };
    if plus_word == Some(true) {
        return None; // a sum
    }
    let plus_word = plus_word.is_some();
    let double_zero = !plus && !plus_word && t.has(i, "00");
    // Neither a sign nor a leading zero: a number whose plus sign the screen
    // reader took (see bare_international_ok()), never right after a number
    let bare = !plus && !plus_word && !double_zero && t.at(i) != b'0';
    if bare {
        let before = t.blanks_begin(i);
        if before > 0 && (t.is_digit(before - 1) || matches!(t.at(before - 1), b'.' | b',')) {
            return None;
        }
    }
    let international = plus || plus_word || double_zero || bare;

    let mut digits = String::new();
    let mut boundaries = Vec::new();
    let (mut pos, mut end) = (i, i);
    let mut bracket = outer_bracket;
    while t.is_digit(pos) {
        let stop = t.digits_end(pos);
        if digits.len() + stop - pos > if double_zero { 17 } else { 15 } {
            return None;
        }
        digits += t.str(pos, stop);
        end = stop;
        if bracket {
            if t.at(end) != b')' {
                return None;
            }
            end += 1;
            bracket = false;
        }
        let mut next = t.skip_spaces(end);
        if matches!(t.at(next), b'/' | b'-') {
            next = t.skip_spaces(next + 1);
        } else if t.has(next, "\u{2011}") || t.has(next, "\u{2013}") {
            next = t.skip_spaces(next + 3);
        }
        if t.at(next) == b'(' {
            bracket = true;
            next += 1;
        }
        if (next == end && end == stop) || !t.is_digit(next) {
            break;
        }
        boundaries.push(digits.len());
        pos = next;
    }
    if t.skip_spaces(end) == end
        && (is_word_byte(t.at(end)) || matches!(t.at(end), b'_' | b':') || t.is_separator_between_digits(end))
    {
        return None;
    }

    let digit_at = |k: usize| digits.as_bytes().get(k).copied().unwrap_or(0);
    let from = |k: usize| digits.get(k..).unwrap_or("");
    let mut country_end = 0;
    let country;
    if international {
        let prefix = if double_zero { 2 } else { 0 };
        if digits.len() < prefix + 7 || digits.len() > prefix + 15 {
            return None;
        }
        let count = country_length(from(prefix));
        if count == 0 {
            return None;
        }
        country = &digits[prefix..prefix + count];
        country_end = prefix + count;
        if bare {
            // Written in groups, the code is the first group, and groups of
            // three throughout are the thousands of one number (385 912 345 678)
            if boundaries.first().is_some_and(|&first| first != country_end) {
                return None;
            }
            let threes = boundaries.len() >= 2
                && digits.len() % 3 == 0
                && boundaries.iter().enumerate().all(|(b, &boundary)| boundary == 3 * (b + 1));
            if threes && boundaries.len() == digits.len() / 3 - 1 {
                return None;
            }
            let zero = usize::from(digit_at(country_end) == b'0');
            if !bare_international_ok(country, from(country_end + zero)) {
                return None;
            }
        }
    } else {
        if !(8..=11).contains(&digits.len()) {
            return None;
        }
        country = home_country(r.language());
    }

    // Keep a written domestic/optional trunk zero with the area code. Never
    // discard a digit, particularly the significant zero of +39 (Italy).
    let trunk = usize::from(digit_at(country_end) == b'0' && country != "39");
    let written = boundaries
        .iter()
        .find(|&&b| b > country_end + trunk && b <= country_end + trunk + 5 && digits.len() - b >= 4)
        .map(|&b| b - country_end);
    let area = written.unwrap_or_else(|| trunk + area_length(country, from(country_end + trunk)));
    let mut subscriber = country_end + area;
    if subscriber > digits.len() || digits.len() - subscriber < 4 {
        return None;
    }
    if !international && (!(2..=4).contains(&area) || digits.len() - subscriber > 7) {
        return None;
    }

    if outer_bracket && out.last() == b'(' {
        out.0.pop();
    }
    if plus {
        out.0.truncate(out.0.iter().rposition(|&b| b == b'+').map_or(0, |at| at + 1));
        out.push(b' ');
    }
    let group = |first: usize, stop: usize| {
        let group = &digits[first..stop];
        if by_digit {
            group.bytes().map(|digit| r.digit_name(digit)).collect::<Vec<_>>().join(" ")
        } else {
            // No agreement: surrounding words must never inflect an
            // identifier, including its last one or two.
            let zeros = group.bytes().take_while(|&b| b == b'0').count().min(group.len().saturating_sub(1));
            let rest = &group[zeros..];
            let last =
                if let &[digit] = rest.as_bytes() { r.digit_name(digit).to_owned() } else { r.number_to_words(rest) };
            format!("{}{last}", format!("{} ", r.zero_word()).repeat(zeros))
        }
    };
    if country_end > 0 {
        out.push_str(&group(0, country_end));
        out.push_str(", ");
    }
    if area > 0 {
        out.push_str(&group(country_end, subscriber));
        out.push_str(", ");
    }
    // Short comma pauses inside the subscriber number, as it is written and
    // said: three digits first, the rest after them (123 456, 123 4567), up
    // to four digits whole, eight as 4+4, and longer service numbers in
    // threes with the last group of two to four. Existing comma-pause
    // settings apply.
    while subscriber < digits.len() {
        let left = digits.len() - subscriber;
        let stop = subscriber
            + match left {
                0..=4 => left,
                8 => 4,
                _ => 3,
            };
        out.push_str(&group(subscriber, stop));
        subscriber = stop;
        if subscriber < digits.len() {
            out.push_str(", ");
        }
    }
    Some(end)
}
