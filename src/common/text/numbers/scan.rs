//! Reading the text around a number: bytes, words, clock times and dates.
//!
//! The converter looks at the text byte by byte: digits, the separators
//! between them and the punctuation around them are ASCII, and every byte of
//! a longer character counts as part of a word. A position past the end reads
//! as 0, so no index is ever out of bounds.

use crate::text::marks::{ITEM_BREAK_MARK, MINOR_BREAK_MARK};

/// A letter or digit, as far as the number converter needs to know: any
/// non-ASCII byte counts as part of a word.
pub(crate) fn is_word_byte(b: u8) -> bool {
    b >= 0x80 || b.is_ascii_alphanumeric()
}

pub(crate) fn is_letter_byte(b: u8) -> bool {
    b >= 0x80 || b.is_ascii_alphabetic()
}

/// A lowercase letter of the Latin (ASCII, Latin-1, Latin Extended-A) or
/// Cyrillic alphabet.
pub(crate) fn is_lowercase_letter(c: u32) -> bool {
    match c {
        0x61..=0x7A => true,
        0xDF..=0xFF => c != 0xF7,                      // ß à-ÿ
        0x100..=0x137 | 0x149..=0x177 => c & 1 != 0,   // ā ă ą ć č đ ... ķ, ŉ ŋ ō ... š ... ŷ
        0x138 | 0x17A | 0x17C | 0x17E | 0x17F => true, // ĸ ź ż ž ſ
        0x139..=0x148 => c & 1 == 0,                   // ĺ ļ ľ ŀ ł ń ņ ň
        0x430..=0x45F => true,                         // а-я ѐ-џ
        _ => false,
    }
}

/// The head of a clause, as the byte before a word shows it: a capitalized
/// word after anything else is a name inside a sentence.
pub(crate) fn starts_clause(head: u8) -> bool {
    matches!(head, b'.' | b'!' | b'?' | b':' | b';' | b'\n' | b',' | b'(')
}

/// The start of the last word of a number's words.
pub(crate) fn last_token(words: &str) -> usize {
    words.rfind(' ').map_or(0, |space| space + 1)
}

/// Replaces the last word of a number's words, which starts at `token`.
pub(crate) fn say(words: &mut String, token: usize, form: &str) {
    words.truncate(token);
    words.push_str(form);
}

/// A number without its leading zeros; the last digit stays.
pub(crate) fn strip_zeros(number: &str) -> &str {
    let zeros = number.bytes().take_while(|&b| b == b'0').count();
    &number[zeros.min(number.len().saturating_sub(1))..]
}

/// Whether the ones of a number are a word of their own: one digit, or a zero
/// before the last.
pub(crate) fn ones_alone(clean: &str) -> bool {
    clean.len() == 1 || clean.as_bytes().iter().rev().nth(1) == Some(&b'0')
}

/// The last digit of a number (0 for an empty one).
pub(crate) fn last_digit(clean: &str) -> u8 {
    clean.as_bytes().last().copied().unwrap_or(0)
}

/// The value of a few digits; a long number saturates.
pub(crate) fn group_value(group: &str) -> u32 {
    group.bytes().fold(0u32, |value, b| value.saturating_mul(10).saturating_add(u32::from(b.wrapping_sub(b'0'))))
}

/// The value of a number of up to nine digits, -1 for a longer one.
pub(crate) fn small_value(digits: &str) -> i64 {
    if digits.len() > 9 { -1 } else { i64::from(group_value(digits)) }
}

/// The short pause between the parts of a clock time or a numeric date and
/// before the address of a spreadsheet cell (`text::marks`), as a word of its
/// own. Without it the hours ran into the minutes and the day into the month:
/// "dvadeset tri pedeset devet" is 23:59 or 20 3 50 9 (a listener's report of
/// November 2026).
pub(crate) fn part_break() -> String {
    format!(" {MINOR_BREAK_MARK} ")
}

/// The pause between two items said in one breath, a cell's contents and its
/// address: the whole comma pause (`ITEM_BREAK_MARK`).
pub(crate) fn item_break() -> String {
    format!(" {ITEM_BREAK_MARK} ")
}

/// Parts of a time or date joined by the short pause.
pub(crate) fn join_parts(parts: impl IntoIterator<Item = String>) -> String {
    parts.into_iter().collect::<Vec<_>>().join(&part_break())
}

/// What joins the two numbers of a range.
pub(crate) enum Link {
    Dash,
    Hyphen,
    /// A conjunction or preposition: do, in, i, ili ...
    Word(String),
}

impl Link {
    pub(crate) fn is_mark(&self) -> bool {
        !matches!(self, Link::Word(_))
    }

    /// The words between the two numbers: "do" for a dash or a hyphen.
    pub(crate) fn join(&self) -> String {
        match self {
            Link::Word(word) => format!(" {word} "),
            _ => " do ".to_owned(),
        }
    }
}

/// The nouns of a unit's sign, in the forms a number asks for. The cases
/// after a preposition are Slovenian (do petih odstotkov, pri eni stopinji, z
/// dvema evroma, k petim evrom); the other languages leave them empty.
pub(crate) struct Unit {
    pub symbol: &'static str,
    pub feminine: bool,
    pub singular: &'static str,
    pub dual: &'static str,
    /// Nominative: trije evri.
    pub plural: &'static str,
    /// tri evre (za 3 €, stane 3 €).
    pub accusative_plural: &'static str,
    pub genitive_plural: &'static str,
    pub genitive_singular: &'static str,
    /// eno stopinjo (the singular for the masculines).
    pub accusative_singular: &'static str,
    /// enemu odstotku, eni stopinji (also the locative).
    pub dative_singular: &'static str,
    /// enim odstotkom, eno stopinjo.
    pub instrumental_singular: &'static str,
    /// dvema odstotkoma, stopinjama (also the instrumental).
    pub dual_dative: &'static str,
    /// trem odstotkom, stopinjam.
    pub plural_dative: &'static str,
    /// treh odstotkih, stopinjah.
    pub plural_locative: &'static str,
    /// tremi odstotki, stopinjami.
    pub plural_instrumental: &'static str,
}

impl Unit {
    /// A masculine unit of a language that declines only by the number:
    /// singular, the form after 2 to 4, the genitive plural and the genitive
    /// singular.
    pub(crate) const fn counted(
        symbol: &'static str,
        singular: &'static str,
        few: &'static str,
        genitive_plural: &'static str,
        genitive_singular: &'static str,
    ) -> Self {
        Unit {
            symbol,
            feminine: false,
            singular,
            dual: few,
            plural: few,
            accusative_plural: few,
            genitive_plural,
            genitive_singular,
            accusative_singular: "",
            dative_singular: "",
            instrumental_singular: "",
            dual_dative: "",
            plural_dative: "",
            plural_locative: "",
            plural_instrumental: "",
        }
    }
}

/// A unit's sign found in the text: its nouns, the end of the sign and the
/// scale's word (" Celzija", or nothing).
pub(crate) type UnitAt = (&'static Unit, usize, &'static str);

/// The word before a number, with only spaces between it and the number, its
/// first letter lowercased; `ahead` is the end of what stands before that
/// word.
pub(crate) struct Prev {
    pub word: String,
    pub capital: bool,
    pub begin: usize,
    pub end: usize,
    pub ahead: usize,
}

/// The word after a position (spaces skipped), as written.
pub(crate) struct Next<'a> {
    pub word: &'a str,
    pub begin: usize,
    pub end: usize,
    pub lowercase: bool,
}

/// Lowercases an initial capital (ASCII or Č Ć Đ Š Ž); true when there was one.
pub(crate) fn lowercase_first(word: &str) -> (String, bool) {
    let mut chars = word.chars();
    let lower = match chars.next() {
        Some(c) if c.is_ascii_uppercase() => Some(c.to_ascii_lowercase()),
        // The small letter is the next code point
        Some(c @ ('Č' | 'Ć' | 'Đ' | 'Š' | 'Ž')) => char::from_u32(c as u32 + 1),
        _ => None,
    };
    match lower {
        Some(lower) => (std::iter::once(lower).chain(chars).collect(), true),
        None => (word.to_owned(), false),
    }
}

/// The Serbian Cyrillic letters, small and capital, in Latin script.
const CYRILLIC: [(char, char, &str); 30] = [
    ('а', 'А', "a"),
    ('б', 'Б', "b"),
    ('в', 'В', "v"),
    ('г', 'Г', "g"),
    ('д', 'Д', "d"),
    ('ђ', 'Ђ', "đ"),
    ('е', 'Е', "e"),
    ('ж', 'Ж', "ž"),
    ('з', 'З', "z"),
    ('и', 'И', "i"),
    ('ј', 'Ј', "j"),
    ('к', 'К', "k"),
    ('л', 'Л', "l"),
    ('љ', 'Љ', "lj"),
    ('м', 'М', "m"),
    ('н', 'Н', "n"),
    ('њ', 'Њ', "nj"),
    ('о', 'О', "o"),
    ('п', 'П', "p"),
    ('р', 'Р', "r"),
    ('с', 'С', "s"),
    ('т', 'Т', "t"),
    ('ћ', 'Ћ', "ć"),
    ('у', 'У', "u"),
    ('ф', 'Ф', "f"),
    ('х', 'Х', "h"),
    ('ц', 'Ц', "c"),
    ('ч', 'Ч', "č"),
    ('џ', 'Џ', "dž"),
    ('ш', 'Ш', "š"),
];

/// A word of the text in small Latin letters: Serbian Cyrillic transliterated
/// (the converter writes Latin words, which the front end of every language
/// reads in either script), an initial capital lowered; and whether there
/// was one.
pub(crate) fn latin_word(word: &str) -> (String, bool) {
    let mut out = String::with_capacity(word.len());
    let mut capital = false;
    for (k, c) in word.chars().enumerate() {
        match CYRILLIC.iter().find(|&&(small, large, _)| c == small || c == large) {
            Some(&(_, large, latin)) => {
                capital |= k == 0 && c == large;
                out += latin;
            }
            None => out.push(c),
        }
    }
    let (out, lowered) = lowercase_first(&out);
    (out, capital || lowered)
}

/// [`latin_word`] without the capital.
pub(crate) fn latin(word: &str) -> String {
    latin_word(word).0
}

/// A day and a month without a year: the ends of the day and of the month
/// and the start of the month.
pub(crate) struct DayMonth {
    pub day_end: usize,
    pub month: usize,
    pub month_end: usize,
}

/// The text a number stands in.
#[derive(Clone, Copy)]
pub(crate) struct Text<'a>(pub &'a str);

impl<'a> Text<'a> {
    pub(crate) fn len(self) -> usize {
        self.0.len()
    }

    /// The byte at `i`, 0 past the end.
    pub(crate) fn at(self, i: usize) -> u8 {
        self.0.as_bytes().get(i).copied().unwrap_or(0)
    }

    /// The text between two positions; empty when they are no boundaries of
    /// characters (the converter cuts only at ASCII bytes, so they are).
    pub(crate) fn str(self, from: usize, to: usize) -> &'a str {
        self.0.get(from..to).unwrap_or("")
    }

    /// The bytes between two positions, as far as the text reaches.
    pub(crate) fn bytes(self, from: usize, to: usize) -> &'a [u8] {
        let bytes = self.0.as_bytes();
        let to = to.min(bytes.len());
        &bytes[from.min(to)..to]
    }

    /// Whether `literal` stands at `pos`.
    pub(crate) fn has(self, pos: usize, literal: &str) -> bool {
        self.0.as_bytes().get(pos..).is_some_and(|rest| rest.starts_with(literal.as_bytes()))
    }

    /// Whether `literal` ends at `end`.
    pub(crate) fn has_before(self, end: usize, literal: &str) -> bool {
        end >= literal.len() && self.has(end - literal.len(), literal)
    }

    pub(crate) fn is_digit(self, i: usize) -> bool {
        self.at(i).is_ascii_digit()
    }

    pub(crate) fn digits_end(self, mut i: usize) -> usize {
        while self.is_digit(i) {
            i += 1;
        }
        i
    }

    /// A period or comma glued between two digits: "3.14", "3,14", "192.168.1.1".
    pub(crate) fn is_separator_between_digits(self, i: usize) -> bool {
        i > 0 && matches!(self.at(i), b'.' | b',') && self.is_digit(i - 1) && self.is_digit(i + 1)
    }

    /// Whether a space or a tab stands before `pos`.
    fn blank_before(self, pos: usize) -> bool {
        pos > 0 && matches!(self.at(pos - 1), b' ' | b'\t')
    }

    /// Spaces between a number and what belongs to it: space, tab, no-break
    /// space, narrow no-break space, thin space.
    pub(crate) fn skip_spaces(self, mut pos: usize) -> usize {
        loop {
            if matches!(self.at(pos), b' ' | b'\t') {
                pos += 1;
            } else if self.has(pos, "\u{A0}") {
                pos += 2;
            } else if self.has(pos, "\u{202F}") || self.has(pos, "\u{2009}") {
                pos += 3;
            } else {
                return pos;
            }
        }
    }

    /// The start of the spaces that end at `pos`: space, tab, no-break space,
    /// thin and narrow no-break space.
    pub(crate) fn spaces_begin(self, mut pos: usize) -> usize {
        loop {
            if self.blank_before(pos) {
                pos -= 1;
            } else if self.has_before(pos, "\u{A0}") {
                pos -= 2;
            } else if self.has_before(pos, "\u{2009}") || self.has_before(pos, "\u{202F}") {
                pos -= 3;
            } else {
                return pos;
            }
        }
    }

    /// The start of the plain spaces and tabs that end at `pos`.
    pub(crate) fn blanks_begin(self, mut pos: usize) -> usize {
        while self.blank_before(pos) {
            pos -= 1;
        }
        pos
    }

    /// The code point at position `i` (0 at the end or at an invalid byte).
    pub(crate) fn code_point_at(self, i: usize) -> u32 {
        let b = self.at(i);
        let len = match b {
            _ if i >= self.len() => return 0,
            0..0x80 => return u32::from(b),
            _ if b & 0xE0 == 0xC0 => 2,
            _ if b & 0xF0 == 0xE0 => 3,
            _ if b & 0xF8 == 0xF0 => 4,
            _ => return 0,
        };
        if i + len > self.len() {
            return 0;
        }
        let mut c = u32::from(b & (0x7F >> len));
        for k in 1..len {
            let cont = self.at(i + k);
            if cont & 0xC0 != 0x80 {
                return 0;
            }
            c = c << 6 | u32::from(cont & 0x3F);
        }
        c
    }

    /// Whether a lowercase letter stands at `i`.
    pub(crate) fn lowercase_at(self, i: usize) -> bool {
        is_lowercase_letter(self.code_point_at(i))
    }

    pub(crate) fn prev_word(self, start: usize) -> Prev {
        let end = self.blanks_begin(start.min(self.len()));
        let mut begin = end;
        while begin > 0 && is_word_byte(self.at(begin - 1)) {
            begin -= 1;
        }
        let (word, capital) = lowercase_first(self.str(begin, end));
        Prev { word, capital, begin, end, ahead: self.blanks_begin(begin) }
    }

    pub(crate) fn next_word(self, pos: usize) -> Next<'a> {
        let begin = self.skip_spaces(pos);
        let mut end = begin;
        while end < self.len() && is_word_byte(self.at(end)) {
            end += 1;
        }
        Next { word: self.str(begin, end), begin, end, lowercase: self.lowercase_at(begin) }
    }

    /// A clock time at position `i`: h or hh, a colon, exactly two minute
    /// digits, optionally a colon and two second digits, and no digit after
    /// that ("12:30", "9:05", "12:30:45"). The end and the groups.
    pub(crate) fn parse_time(self, i: usize) -> Option<(usize, Vec<&'a str>)> {
        let hours_end = self.digits_end(i);
        let hours = self.str(i, hours_end);
        if !(1..=2).contains(&hours.len()) || group_value(hours) > 23 {
            return None;
        }
        let mut groups = Vec::with_capacity(3);
        let mut pos = hours_end;
        for part in 0..2 {
            if self.at(pos) != b':' {
                break;
            }
            let start = pos + 1;
            let end = self.digits_end(start);
            if end - start != 2 || group_value(self.str(start, end)) > 59 {
                break;
            }
            if part == 0 {
                groups.push(hours);
            }
            groups.push(self.str(start, end));
            pos = end;
        }
        (!groups.is_empty()).then_some((pos, groups))
    }

    /// A date at position `i`: day 1-31, a period, month 1-12, a period, a
    /// year of two or four digits, and no digit after that ("7.10.2026",
    /// "07.10.26"). One space may follow each period, as the orthography
    /// writes it ("7. 10. 2026."). A period after the year is left in the
    /// text: it ends the sentence or the clause as usual. The other written
    /// forms of a date (slashes, hyphens, the year first, a Roman month,
    /// typeset spaces) arrive here in this one (`dates::normalize()`).
    pub(crate) fn parse_date(self, i: usize) -> Option<(usize, [&'a str; 3])> {
        let mut groups = [""; 3];
        let mut pos = i;
        for (part, slot) in groups.iter_mut().enumerate() {
            let end = self.digits_end(pos);
            let group = self.str(pos, end);
            let value = group_value(group);
            let fits = match part {
                0 => group.len() <= 2 && (1..=31).contains(&value),
                1 => group.len() <= 2 && (1..=12).contains(&value),
                _ => group.len() == 2 || group.len() == 4,
            };
            if !fits {
                return None;
            }
            *slot = group;
            pos = end;
            if part < 2 {
                if self.at(pos) != b'.' {
                    return None;
                }
                pos += 1;
                if self.at(pos) == b' ' {
                    pos += 1;
                }
                if !self.is_digit(pos) {
                    return None;
                }
            }
        }
        // More groups after the year: an address or a version, not a date
        // (10.10.10.10, 1.12.10.3)
        if self.is_separator_between_digits(pos) {
            return None;
        }
        Some((pos, groups))
    }

    /// A day and a month without a year at `pos`: 7.10., 7. 10., 05.03.;
    /// nothing when the numbers are no day and month or a year follows (the
    /// full date).
    pub(crate) fn parse_day_month(self, pos: usize) -> Option<DayMonth> {
        let day_end = self.digits_end(pos);
        let day = self.str(pos, day_end);
        if !(1..=2).contains(&day.len()) || self.at(day_end) != b'.' || !(1..=31).contains(&group_value(day)) {
            return None;
        }
        let month = day_end + 1 + usize::from(self.at(day_end + 1) == b' ');
        let month_end = self.digits_end(month);
        let digits = self.str(month, month_end);
        if !(1..=2).contains(&digits.len()) || self.at(month_end) != b'.' || !(1..=12).contains(&group_value(digits)) {
            return None;
        }
        let year = month_end + 1 + usize::from(self.at(month_end + 1) == b' ');
        (!self.is_digit(year)).then_some(DayMonth { day_end, month, month_end })
    }

    /// A date, with or without a year, at `pos`.
    pub(crate) fn date_at(self, pos: usize) -> bool {
        self.is_digit(pos) && (self.parse_date(pos).is_some() || self.parse_day_month(pos).is_some())
    }

    /// The period after the year of a date is the ordinal's, as the
    /// orthography writes it (10. 11. 2026.), and before a lowercase word it
    /// ends no sentence: "10.11.2026. godine", "od 1.12.2026. do 5.12.2026.",
    /// "Rođen je 10.11.1985. u Zagrebu" fell to the bottom of the voice and
    /// paused after the year. The same line as for "7. listopada"
    /// (`silent_dot_end()`). Returns the end of what the date takes, the
    /// period included when it is silent.
    pub(crate) fn date_period_end(self, end: usize, words: &mut String) -> usize {
        if self.at(end) != b'.' {
            return end;
        }
        let mut next = end + 1;
        while matches!(self.at(next), b' ' | b'\t') {
            next += 1;
        }
        if !self.lowercase_at(next) {
            return end;
        }
        if next == end + 1 {
            words.push(' ');
        }
        end + 1
    }

    /// A unit's sign at or after `pos` (spaces allowed): %, €, $, °, °C, °F,
    /// not glued to a letter. `celsius` is the scale's word in the language.
    pub(crate) fn unit_at(self, pos: usize, units: &'static [Unit; 4], celsius: &'static str) -> Option<UnitAt> {
        let pos = self.skip_spaces(pos);
        let (k, unit) = units.iter().enumerate().find(|(_, unit)| self.has(pos, unit.symbol))?;
        let mut end = pos + unit.symbol.len();
        let mut scale = "";
        // The degree's scale: °C, °F
        if k == 3 && end < self.len() && !is_word_byte(self.at(end + 1)) {
            scale = match self.at(end) {
                b'C' | b'c' => celsius,
                b'F' | b'f' => " Fahrenheita",
                _ => "",
            };
            end += usize::from(!scale.is_empty());
        }
        (!is_letter_byte(self.at(end))).then_some((unit, end, scale))
    }
}

/// The converted text. Bytes, because the converter takes back what it wrote
/// when a later number shows what it was (a minus, a currency's sign, the
/// letters of a cell's address).
#[derive(Default)]
pub(crate) struct Out(pub Vec<u8>);

impl Out {
    pub(crate) fn push_str(&mut self, text: &str) {
        self.0.extend_from_slice(text.as_bytes());
    }

    pub(crate) fn push(&mut self, byte: u8) {
        self.0.push(byte);
    }

    pub(crate) fn len(&self) -> usize {
        self.0.len()
    }

    /// The last byte, 0 when there is none.
    pub(crate) fn last(&self) -> u8 {
        self.0.last().copied().unwrap_or(0)
    }

    /// Takes `literal` back when the output ends in it.
    pub(crate) fn take_back(&mut self, literal: &[u8]) -> bool {
        let ends = self.0.ends_with(literal);
        if ends {
            self.0.truncate(self.0.len() - literal.len());
        }
        ends
    }

    /// A space after a letter or digit, before a word that replaces a sign.
    pub(crate) fn set_off(&mut self) {
        if is_word_byte(self.last()) {
            self.push(b' ');
        }
    }

    pub(crate) fn trim_blanks(&mut self) {
        while matches!(self.0.last(), Some(b' ' | b'\t')) {
            self.0.pop();
        }
    }

    pub(crate) fn into_string(self) -> String {
        String::from_utf8(self.0).unwrap_or_else(|error| String::from_utf8_lossy(error.as_bytes()).into_owned())
    }
}

/// A minus glued to the number at `i` becomes the word: the hyphen-minus, the
/// minus sign or the en dash, after a space, a bracket or at the start, so
/// that T-2, A-2 and 1991-2000 keep their hyphen (Pravopis 8.0, § 26: –10 °C;
/// SP 2001 mínus "temperatura ~ deset stopinj Celzija"; Hrvatski pravopis:
/// −5 °C).
pub(crate) fn read_glued_minus(t: Text, i: usize, out: &mut Out) {
    let sign = if i > 0 && t.at(i - 1) == b'-' {
        1
    } else if t.has_before(i, "\u{2212}") || t.has_before(i, "\u{2013}") {
        3
    } else {
        return;
    };
    let at = i - sign;
    let before = if at > 0 { t.at(at - 1) } else { b' ' };
    // 0x9E and 0x9C: the last bytes of „ and “
    if !matches!(before, b' ' | b'\t' | b'\n' | b'(' | b'[' | b'"' | b'\'' | b':' | 0xA0 | 0x9E | 0x9C) {
        return;
    }
    // set_off_from_letters() set the dash off
    let r = out.len() - usize::from(sign == 3 && out.last() == b' ');
    if r >= sign && out.0[r - sign..r] == *t.bytes(at, i) {
        out.0.truncate(r - sign);
        if !out.0.is_empty() {
            out.set_off();
        }
        out.push_str("minus ");
    }
}
