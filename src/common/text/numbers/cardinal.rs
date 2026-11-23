//! Cardinal numbers in words, up to centillions (10^303).
//!
//! The words are the language's ([`Numerals`], from its folder); how they
//! are put together is the same for all:
//! - the groups of three digits, each with its scale word (thousand,
//!   million, milliard ...)
//! - the scale word in the form its count asks for: after 1, after 2,
//!   after 3 and 4, and after the rest (5 and above, 0, 11-19)
//! - the last word of a group agreeing with its scale word, by the
//!   language's [`ScaleAgreement`]
//! - the ones after the tens (dvadeset jedan) or before them as one word
//!   (enaindvajset)

use super::NumberReader;
use super::scan::{last_digit, ones_alone, strip_zeros};

/// The number words of a language. The scale words are listed by what they
/// count: one, two, three or four, and the rest (five and above, the teens,
/// zero).
pub(crate) struct Numerals {
    /// The word of a digit inside a longer number.
    pub(crate) digits: [&'static str; 10],
    /// The name of the digit 2 read by itself.
    pub(crate) two_alone: &'static str,
    pub(crate) teens: [&'static str; 10],
    pub(crate) tens: [&'static str; 10],
    pub(crate) hundreds: [&'static str; 10],
    pub(crate) thousand: [&'static str; 4],
    /// The endings of milijun, bilijun ... after "mi", "bi" ...
    pub(crate) million: [&'static str; 4],
    /// The endings of milijarda, bilijarda ...
    pub(crate) milliard: [&'static str; 4],
    /// tisuća and hiljada are feminine, tisoč masculine (en tisoč, dva tisoč).
    pub(crate) feminine_thousand: bool,
    /// The ones stand before the tens, as one word: enaindvajset.
    pub(crate) ones_first: bool,
    pub(crate) scale_agreement: ScaleAgreement,
}

/// How the last word of a group agrees with the scale word after it.
pub(crate) enum ScaleAgreement {
    /// One and two agree with a feminine scale word, except in the teens:
    /// dvije tisuće, dve hiljade, dvadeset jedna tisuća, dvije milijarde.
    OneAndTwo { one: &'static str, two: &'static str },
    /// One to four agree with every scale word where they are a word of
    /// their own, and a group whose ones are part of a compound takes the
    /// genitive plural (enaindvajset milijonov). The forms of 1, 2, 3 and 4,
    /// each before the thousand, before a word in -ilijon and before a word
    /// in -ilijarda: en tisoč, dva milijona, trije milijoni, štiri milijarde.
    UpToFour([[&'static str; 3]; 4]),
}

/// The scale words above the thousand: the prefix, and whether the word is
/// one in -ilijarda (feminine) or in -ilijun.
const SCALES: [(&str, bool); 18] = [
    ("mi", false),
    ("mi", true),
    ("bi", false),
    ("bi", true),
    ("tri", false),
    ("tri", true),
    ("kvadri", false),
    ("kvadri", true),
    ("kvinti", false),
    ("kvinti", true),
    ("seksti", false),
    ("septi", false),
    ("okti", false),
    ("noni", false),
    ("deci", false),
    ("undeci", false),
    ("duodeci", false),
    ("centi", false),
];

fn index(digit: u8) -> Option<usize> {
    digit.is_ascii_digit().then(|| usize::from(digit - b'0'))
}

impl Numerals {
    fn word(table: &[&'static str; 10], digit: u8) -> &'static str {
        index(digit).map_or("", |digit| table[digit])
    }

    /// 10-99 and 01-09.
    fn two_digits(&self, tens: u8, ones: u8) -> String {
        match (tens, ones) {
            (_, b'0') => Self::word(&self.tens, tens).to_owned(),
            (b'1', _) => Self::word(&self.teens, ones).to_owned(),
            (b'0', _) => Self::word(&self.digits, ones).to_owned(),
            _ => {
                let (tens, ones) = (Self::word(&self.tens, tens), Self::word(&self.digits, ones));
                match (tens.is_empty(), self.ones_first) {
                    (true, _) => String::new(),
                    (_, true) => format!("{ones}in{tens}"),
                    (_, false) => format!("{tens} {ones}"),
                }
            }
        }
    }

    /// A group of one to three digits.
    fn group(&self, group: &[u8]) -> String {
        match *group {
            [ones] => Self::word(&self.digits, ones).to_owned(),
            [tens, ones] => self.two_digits(tens, ones),
            [hundreds, tens, ones] => {
                let hundreds = Self::word(&self.hundreds, hundreds);
                let rest = self.two_digits(tens, ones);
                let space = if hundreds.is_empty() || rest.is_empty() { "" } else { " " };
                format!("{hundreds}{space}{rest}")
            }
            _ => String::new(),
        }
    }

    /// The scale word of a group: 0 the thousands, 1 the millions, 2 the
    /// milliards ...; `count` is the digit that decides its form.
    fn scale(&self, scale: usize, count: u8) -> String {
        let form = match count {
            b'1' => 0,
            b'2' => 1,
            b'3' | b'4' => 2,
            _ => 3,
        };
        match scale.checked_sub(1).map(|above| SCALES.get(above)) {
            None => self.thousand[form].to_owned(),
            Some(Some(&(prefix, milliard))) => {
                format!("{prefix}{}", if milliard { self.milliard[form] } else { self.million[form] })
            }
            Some(None) => String::new(),
        }
    }

    /// tisuća/hiljada and the words in -ilijarda are feminine, the words in
    /// -ilijun/-ilion masculine.
    fn is_feminine_scale(&self, scale: usize) -> bool {
        match scale.checked_sub(1) {
            None => self.feminine_thousand,
            Some(above) => SCALES.get(above).is_some_and(|&(_, milliard)| milliard),
        }
    }
}

impl NumberReader {
    pub(crate) fn zero_word(&self) -> &'static str {
        self.numerals.digits[0]
    }

    /// The name of a digit read by itself ("" for anything else).
    pub fn digit_name(&self, digit: u8) -> &'static str {
        if digit == b'2' { self.numerals.two_alone } else { Numerals::word(&self.numerals.digits, digit) }
    }

    /// A number written with digits alone, in words; empty for anything else.
    pub fn number_to_words(&self, number: &str) -> String {
        if number.is_empty() || !number.bytes().all(|b| b.is_ascii_digit()) {
            return String::new();
        }
        let numerals = self.numerals;
        let compounds = matches!(numerals.scale_agreement, ScaleAgreement::UpToFour(_));
        let number = strip_zeros(number);
        if number == "0" {
            return self.zero_word().to_owned();
        }

        let group_count = number.len().div_ceil(3);
        // The first group may have 1, 2, or 3 digits
        let first_len = number.len() - (group_count - 1) * 3;
        let mut result = String::new();
        let mut start = 0;
        for group_num in 0..group_count {
            let len = if group_num == 0 { first_len } else { 3 };
            let group = strip_zeros(&number[start..start + len]);
            start += len;
            if group == "0" {
                continue;
            }

            let groups_from_end = group_count - 1 - group_num;
            let last = last_digit(group);
            // 11-19 have no "one" or "two" of their own: jedanaest tisuća
            let is_teen = group.as_bytes().iter().rev().nth(1) == Some(&b'1');
            // Where the ones stand first they are a word of their own only after a zero
            // tens digit (sto en milijon, tisoč dva milijona); from 21 up they
            // are one word with the tens (enaindvajset), which takes the
            // genitive plural like every compound: enaindvajset milijonov.
            let ones_alone = ones_alone(group);
            // The group is exactly "1" for thousands and higher: "tisuću",
            // not "jedan tisuću", for 1000
            let is_one = group == "1";

            if !is_one || groups_from_end == 0 {
                if !result.is_empty() {
                    result.push(' ');
                }
                let mut words = numerals.group(group.as_bytes());
                let counting = Numerals::word(&numerals.digits, last);
                let feminine = groups_from_end > 0 && numerals.is_feminine_scale(groups_from_end - 1);
                let agreeing = match numerals.scale_agreement {
                    // One to four agree with the scale word: en milijon, dva
                    // milijona, trije milijoni, štirje milijoni; ena
                    // milijarda, dve milijardi, tri milijarde, štiri
                    // milijarde; en tisoč, dva tisoč, tri tisoč, štiri tisoč
                    ScaleAgreement::UpToFour(forms) => {
                        let scale = match groups_from_end {
                            1 => 0,
                            _ if feminine => 2,
                            _ => 1,
                        };
                        match last {
                            _ if groups_from_end == 0 || !ones_alone => None,
                            b'1'..=b'4' => Some(forms[usize::from(last - b'1')][scale]),
                            _ => None,
                        }
                    }
                    // The numbers one and two agree with a feminine scale
                    // word: dvije tisuće, dve hiljade, dvadeset jedna tisuća,
                    // dvije milijarde
                    ScaleAgreement::OneAndTwo { one, two } => match last {
                        _ if !feminine || is_teen => None,
                        b'1' => Some(one),
                        b'2' => Some(two),
                        _ => None,
                    },
                };
                if let Some(agreeing) = agreeing {
                    words.truncate(words.strip_suffix(counting).map_or(words.len(), str::len));
                    words += agreeing;
                }
                result += &words;
            }

            if groups_from_end > 0 {
                // 11-19 take the genitive plural like 5-9 (dvanaest tisuća).
                // A group ending in 1 takes the singular: "dvadeset jedan
                // milijun", "dvadeset jedna milijarda". For thousands that is
                // the nominative "tisuća", spelled like the genitive plural
                // ('1' gives the accusative "tisuću" used for 1000 itself).
                // Where the ones stand first: enaindvajset milijonov,
                // dvainštirideset milijard.
                let genitive_plural =
                    is_teen || (!is_one && last == b'1' && groups_from_end == 1) || (compounds && !ones_alone);
                let suffix = numerals.scale(groups_from_end - 1, if genitive_plural { b'0' } else { last });
                if !suffix.is_empty() {
                    if !result.is_empty() {
                        result.push(' ');
                    }
                    result += &suffix;
                }
            }
        }
        result
    }
}
