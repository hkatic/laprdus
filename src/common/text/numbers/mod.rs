//! Numbers to words, and the text normalization around numbers: ordinals,
//! dates, clock times, telephone numbers, currency amounts, the addresses of
//! spreadsheet cells and the agreement of a number with its noun.
//!
//! The language chooses the number words ([`cardinal`]) and the grammar: the
//! Slovenian one is in `languages::sl::numerals`, the Croatian, Serbian and
//! Bosnian one in `languages::hr::numerals`. What is written the same way in
//! every language is here.

mod cardinal;
mod cells;
mod currency;
mod phone;
pub(crate) mod scan;
#[cfg(test)]
mod tests;

use crate::language::Language;
use crate::languages::{hr, sl};
use cardinal::Numerals;
use scan::{Out, Text, Unit, is_letter_byte, is_word_byte, join_parts};

/// Reads the numbers of a text in one language.
#[derive(Clone, Copy)]
pub struct NumberReader {
    language: Language,
    numerals: &'static Numerals,
}

impl NumberReader {
    pub fn new(language: Language) -> Self {
        NumberReader { language, numerals: Numerals::of(language) }
    }

    pub fn language(&self) -> Language {
        self.language
    }

    fn slovenian(&self) -> bool {
        self.language == Language::Slovenian
    }

    /// The nouns of the unit signs in the language: %, €, $, °.
    fn units(&self) -> &'static [Unit; 4] {
        if self.slovenian() { &sl::numerals::UNITS } else { &hr::numerals::dialect(self.language).units }
    }

    /// The language's own constructs at the number at `i`.
    fn number_at(&self, t: Text, i: usize, out: &mut Out) -> Option<usize> {
        if self.slovenian() {
            sl::numerals::number_at(self, t, i, out, false)
        } else {
            hr::numerals::number_at(self, t, i, out, false)
        }
    }

    /// The last word of a number's words agrees with what the number in
    /// [start, end) counts.
    fn agree(&self, t: Text, start: usize, end: usize, words: &mut String) {
        if self.slovenian() {
            sl::numerals::agree(t, start, end, words);
        } else {
            hr::numerals::agree(self, t, start, end, words);
        }
    }

    /// "12:30" -> dvanaest trideset, "9:05" -> devet nula pet, with a short
    /// pause where the colon stood, so that the hours, minutes and seconds
    /// are told apart.
    pub(crate) fn time_to_words(&self, groups: &[&str]) -> String {
        join_parts(groups.iter().enumerate().map(|(k, group)| {
            let mut part = if k == 0 { self.number_to_words(group) } else { self.digit_group_to_words(group) };
            if self.slovenian() {
                sl::numerals::feminine_two(&mut part); // the hour and the minute: dve trideset
            }
            part
        }))
    }

    /// "7.10.2026" -> sedmog desetog dvije tisuće dvadeset šeste. Slovenian
    /// reads a date in the genitive, which does not depend on the sentence:
    /// sedmega desetega dva tisoč šestindvajset (Jezikovna svetovalnica, ZRC
    /// SAZU: "šestega julija dva tisoč deset").
    pub(crate) fn date_to_words(&self, groups: &[&str; 3]) -> String {
        if self.slovenian() {
            join_parts([
                sl::numerals::ordinal(self, groups[0], "ega"),
                sl::numerals::ordinal(self, groups[1], "ega"),
                self.number_to_words(groups[2]),
            ])
        } else {
            hr::numerals::date_to_words(self, groups, "og")
        }
    }

    /// A time or date at position `i`, read without the separators; the end
    /// of what was consumed.
    fn convert_time_or_date(&self, t: Text, i: usize, out: &mut Out) -> Option<usize> {
        if let Some((end, groups)) = t.parse_time(i) {
            out.push_str(&self.time_to_words(&groups));
            return Some(end);
        }
        let (mut end, groups) = t.parse_date(i)?;
        let mut words = self.date_to_words(&groups);
        if !self.slovenian() {
            end = t.date_period_end(end, &mut words);
        }
        out.push_str(&words);
        Some(end)
    }

    /// Words for a group of digits: every leading zero is "nula", the rest is
    /// one number ("007" -> "nula nula sedam", "000" -> "nula nula nula").
    pub(crate) fn digit_group_to_words(&self, group: &str) -> String {
        if let &[digit] = group.as_bytes() {
            return self.digit_name(digit).to_owned(); // 3.2: tri pika dve
        }
        let zeros = group.bytes().take_while(|&b| b == b'0').count().min(group.len().saturating_sub(1));
        format!("{}{}", format!("{} ", self.zero_word()).repeat(zeros), self.number_to_words(&group[zeros..]))
    }

    /// The digits after a decimal comma: up to two as one number, more one by
    /// one ("3,14" -> četrnaest, "3,14159" -> jedan četiri jedan pet devet),
    /// leading zeros each as "nula".
    pub(crate) fn decimal_fraction(&self, fraction: &str) -> String {
        let zeros = fraction.bytes().take_while(|&b| b == b'0').count().min(fraction.len().saturating_sub(1));
        let rest = &fraction[zeros..];
        let mut words: Vec<String> = vec![self.zero_word().to_owned(); zeros];
        match rest.as_bytes() {
            &[digit] => words.push(self.digit_name(digit).to_owned()), // 3.2, 3.02
            [_, _] => words.push(self.number_to_words(rest)),
            digits => words.extend(digits.iter().map(|&digit| self.digit_name(digit).to_owned())),
        }
        words.join(" ")
    }

    /// The digits of a number, each by its name.
    fn digit_names(&self, digits: &str) -> String {
        digits.bytes().map(|digit| self.digit_name(digit)).collect::<Vec<_>>().join(" ")
    }

    /// The separator itself, as the front end will read it. A period between
    /// digits becomes the one dot leader (U+2024) glued to the digits on both
    /// sides: the clause splitter does not know it, so it stays inside the
    /// clause, and the front end names it in the voice's language (točka /
    /// tačka). A period itself would be the end of a clause there, as every
    /// period in the text that is not a decimal point or part of a file name
    /// now is. The decimal comma is "zarez" in every language, so the word is
    /// written here; a comma in the text would otherwise be a clause
    /// break. Slovenian reads the decimal comma as "vejica" (and the front end
    /// names the dot leader "pika").
    pub(crate) fn separator_words(&self, separator: u8) -> &'static str {
        match separator {
            b'.' => "\u{2024}",
            _ if self.slovenian() => " vejica ",
            _ => " zarez ",
        }
    }

    /// Convert all numbers in text to words (whole number mode): "123" ->
    /// "sto dvadeset tri".
    pub fn convert_numbers(&self, text: &str) -> String {
        let t = Text(text);
        let length = t.len();
        let mut out = Out(Vec::with_capacity(length * 2));
        let mut i = 0;
        let mut after_number = false;

        while i < length {
            // Non-digit text is appended unchanged, set off from a number it
            // is glued to ("20:03h", "2x")
            let text_start = i;
            while i < length && !t.is_digit(i) {
                i += 1;
            }
            if i > text_start {
                if after_number && is_letter_byte(t.at(text_start)) {
                    out.push(b' ');
                }
                out.0.extend_from_slice(t.bytes(text_start, i));
            }
            if i >= length {
                break;
            }

            cells::mark_cell_address(t, i, &mut out);
            if let Some(end) = currency::convert(self, t, i, &mut out, false) {
                i = end;
                after_number = true;
                continue;
            }
            set_off_from_letters(&mut out, is_compound(t, i));
            after_number = true;

            if let Some(end) = phone::convert(self, t, i, &mut out, false) {
                i = end;
                continue;
            }

            // What a language reads in its own way: a minus, the hours after
            // ob, a range, a date without a year, thousands with spaces,
            // 2-krat and 2-letni, a unit's noun
            if let Some(end) = self.number_at(t, i, &mut out) {
                i = end;
                continue;
            }

            // A clock time or a date is read as such, without the separators:
            // the colon of "12:30" and the periods of "7.10.2026." are silent.
            if let Some(end) = self.convert_time_or_date(t, i, &mut out) {
                i = end;
                continue;
            }

            // Slovenian writes a period between the groups of three digits of
            // a large number (132.457: sto dvaintrideset tisoč štiristo
            // sedeminpetdeset; Slovenski pravopis 2001, § 253), so such a
            // number is read whole. A period before fewer digits (3.14) is the
            // decimal point, read by name, as below.
            if self.slovenian() && t.at(i) != b'0' {
                let first_end = t.digits_end(i);
                let mut end = first_end;
                while end + 3 < length && t.at(end) == b'.' && t.digits_end(end + 1) == end + 4 {
                    end += 4;
                }
                if end > first_end && first_end - i <= 3 && !t.is_separator_between_digits(end) && t.at(end) != b':' {
                    let digits: String = t.str(i, end).chars().filter(char::is_ascii_digit).collect();
                    let mut words = self.number_to_words(&digits);
                    sl::numerals::agree(t, i, end, &mut words);
                    out.push_str(&words);
                    i = end;
                    continue;
                }
            }

            // Each leading zero becomes "nula ", set off from a preceding
            // letter but glued to a separator ("25:00" must stay one token)
            while t.at(i) == b'0' && t.is_digit(i + 1) {
                out.set_off();
                out.push_str(self.zero_word());
                out.push(b' ');
                i += 1;
            }

            let num_start = i;
            i = t.digits_end(i);
            let number = t.str(num_start, i);

            // "7. listopada", "1990. godine": the period is silent, and the
            // ordinal is read in the case the noun and the preposition show
            // (sedmog listopada, prvo mjesto, na prvem mestu)
            if let Some(end) = silent_dot_end(t, num_start, i) {
                let ordinal = if self.slovenian() {
                    sl::numerals::ordinal_before(self, t, num_start, i, end)
                } else {
                    hr::numerals::ordinal_before(self, t, num_start, i, end)
                };
                out.push_str(&if ordinal.is_empty() { self.number_to_words(number) } else { ordinal });
                if !matches!(t.at(end), b' ' | b'\t') {
                    out.push(b' '); // "7.listopada"
                }
                i = end;
                continue;
            }

            let mut words = self.number_to_words(number);
            if !words.is_empty() {
                self.agree(t, num_start, i, &mut words);
                out.push_str(&words);
            }

            // Digits after a period or comma glued to the number. A period is
            // read by name and every group of digits as a whole number, as
            // synthesizer B reads it ("1.317" -> jedan.tristo sedamnaest,
            // "3.14" -> tri.četrnaest, "192.168.1.1" and "2.0.1" group by
            // group), leading zeros each as "nula" ("3.05" -> tri.nula pet).
            // One comma is the decimal comma (synthesizer B's rule for
            // Croatian): up to two digits after it are read as one number,
            // more are read one by one ("3,14" -> tri zarez četrnaest,
            // "3,14159" -> tri zarez jedan četiri jedan pet devet); with two
            // or more commas every group is a whole number. In a phone number
            // spelled out digit by digit the comma is a pause between the
            // groups.
            if is_grouping_comma(t, i) {
                out.push_str(", ");
                i += 1;
                continue;
            }
            let mut separators = 0;
            let mut k = i;
            while t.is_separator_between_digits(k) {
                separators += 1;
                k = t.digits_end(k + 1);
            }
            while t.is_separator_between_digits(i) {
                let separator = t.at(i);
                let group_end = t.digits_end(i + 1);
                let group = t.str(i + 1, group_end);
                if self.slovenian() && separator == b',' && separators == 1 && i > num_start {
                    // tri cele štirinajst: the whole part was already written
                    let whole = t.str(num_start, i);
                    out.0.truncate(out.len().saturating_sub(self.number_to_words(whole).len()));
                    out.push_str(&sl::numerals::decimal(self, whole, group));
                } else {
                    out.push_str(self.separator_words(separator));
                    if separator == b'.' || separators >= 2 {
                        out.push_str(&self.digit_group_to_words(group));
                    } else {
                        out.push_str(&self.decimal_fraction(group));
                    }
                }
                i = group_end;
            }
        }
        out.into_string()
    }

    /// Convert all numbers in text to words digit by digit: "123" -> "jedan
    /// dva tri".
    pub fn convert_digits(&self, text: &str) -> String {
        let t = Text(text);
        let length = t.len();
        let mut out = Out(Vec::with_capacity(length * 4));
        let mut i = 0;

        while i < length {
            let text_start = i;
            while i < length && !t.is_digit(i) {
                i += 1;
            }
            out.0.extend_from_slice(t.bytes(text_start, i));
            if i >= length {
                break;
            }

            cells::mark_cell_address(t, i, &mut out);
            if let Some(end) = currency::convert(self, t, i, &mut out, true) {
                i = end;
                continue;
            }
            if let Some(end) = phone::convert(self, t, i, &mut out, true) {
                i = end;
                continue;
            }
            currency::read_digit_mode_minus(t, i, &mut out);

            // Digits glued to letters are set off from them as in the
            // whole-number mode ("u20:03", "5kg", "COVID19"), or the first
            // digit's name ran into the word before it ("Ajedan sedam" for A17)
            set_off_from_letters(&mut out, is_compound(t, i));

            // A time or a date is not a string of digits: read it as in the
            // whole-number mode
            if let Some(end) = self.convert_time_or_date(t, i, &mut out) {
                i = end;
                continue;
            }

            // Each digit by its name; a period or comma glued between digits
            // is read by name ("3.14" -> tri.jedan četiri), except the comma
            // between the groups of a phone number
            let mut first_digit = true;
            while t.is_digit(i) || t.is_separator_between_digits(i) {
                if t.is_digit(i) {
                    if !first_digit {
                        out.push(b' ');
                    }
                    out.push_str(self.digit_name(t.at(i)));
                    first_digit = false;
                } else {
                    out.push_str(if is_grouping_comma(t, i) { ", " } else { self.separator_words(t.at(i)) });
                    first_digit = true; // glued to the period, or after the spaces of " zarez "
                }
                i += 1;
            }
        }
        out.into_string()
    }
}

/// Whether the number at `i` is the first part of a time, a date or a
/// decimal: a colon or a separator follows its digits.
fn is_compound(t: Text, i: usize) -> bool {
    let digits = t.digits_end(i);
    t.at(digits) == b':' || t.is_separator_between_digits(digits)
}

/// A number of up to four digits without a leading zero, standing at the
/// start of a word and followed by a period and a lowercase word, is an
/// ordinal ("7. listopada", "u 19. stoljeću", "1990. godine"). The period is
/// silent and does not end the clause. Before an uppercase word the period
/// ends the sentence as usual ("Ima ih 7. Sutra ..."); synthesizer B draws
/// the same line. Returns the position after the period.
fn silent_dot_end(t: Text, start: usize, end: usize) -> Option<usize> {
    if end - start > 4 || t.at(start) == b'0' || (start > 0 && is_word_byte(t.at(start - 1))) || t.at(end) != b'.' {
        return None;
    }
    let mut next = end + 1;
    while matches!(t.at(next), b' ' | b'\t') {
        next += 1;
    }
    t.lowercase_at(next).then_some(end + 1)
}

/// A number glued to letters ("u20:03", "5kg", "COVID19") is set off from
/// them, or the letters would be read as part of the number word
/// ("udvadeset", stressed on the u). When the number is a time, a date or
/// a decimal (compound), a period that joins a word to the letter run
/// ("listopada.u20:03", the way a screen reader glues the parts of a
/// label) is a word break, not a period to read by name; before a plain
/// number it stays, for the file extensions "pjesma.mp3", "film.mp4".
fn set_off_from_letters(out: &mut Out, compound: bool) {
    let letters = out.0.iter().rev().take_while(|&&b| is_letter_byte(b)).count();
    if letters == 0 {
        return;
    }
    let k = out.len() - letters;
    if compound && k >= 2 && out.0[k - 1] == b'.' && is_letter_byte(out.0[k - 2]) {
        out.0[k - 1] = b' ';
    }
    out.push(b' ');
}

/// A comma between two single digits of a spaced-out digit string is a
/// grouping mark, not a decimal comma: screen readers write a phone number
/// digit by digit, with a comma where a group ends ("+ 1 2 3,5 6,7 8 9,8 7 6").
/// Such a string is a run of single digits, each separated from the next by
/// one space or a glued comma, with at least six digits and at least two of
/// them touching no comma, so that a list of decimals ("4,5 3,5 5,0") stays
/// a list of decimals.
fn is_grouping_comma(t: Text, i: usize) -> bool {
    if !t.is_separator_between_digits(i) || t.at(i) != b',' {
        return false;
    }
    let single = |d: usize| (d == 0 || !t.is_digit(d - 1)) && !t.is_digit(d + 1);
    // The digit at d is followed by one at d + 2
    let linked = |d: usize| matches!(t.at(d + 1), b' ' | b',') && t.is_digit(d + 2) && single(d + 2);
    if !single(i - 1) || !single(i + 1) {
        return false;
    }

    let mut first = i - 1;
    while first >= 2 && t.is_digit(first - 2) && single(first - 2) && linked(first - 2) {
        first -= 2;
    }
    let (mut digits, mut lone) = (0, 0);
    let mut d = first;
    loop {
        digits += 1;
        if !(d > first && t.at(d - 1) == b',') && t.at(d + 1) != b',' {
            lone += 1;
        }
        if !linked(d) {
            break;
        }
        d += 2;
    }
    digits >= 6 && lone >= 2
}
