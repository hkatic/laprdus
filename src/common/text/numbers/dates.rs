//! The written forms of a numeric date, brought to the one the languages
//! read: day, period, month, period, year (`Text::parse_date()`).
//!
//! A date is written in many ways (a listener's report of 2026-11-18, and
//! the forms the lists of national date formats give: slashes, hyphens, the
//! year first as in ISO 8601, the month as a Roman numeral, typeset spaces):
//!
//! | written | read as |
//! |---|---|
//! | `18/11/2026`, `18/11/26`, `18-11-2026` | `18.11.2026`, `18.11.26` |
//! | `2026-11-18`, `2026/11/18`, `2026.11.18` | `18.11.2026` |
//! | `2026-11-18T10:30` | `18.11.2026 10:30` |
//! | `11/18/2026` (no month 18: month first) | `18.11.2026` |
//! | `18.XI.2026.`, `18. XI. 2026.`, `18/XI/2026` | `18.11.2026.` |
//! | `18.` + no-break or thin space + `11.` ... | `18. 11. ...` |
//!
//! Rewriting the text before the numbers are read gives every form the same
//! words, cases and pauses as the dotted one in every language, with the
//! nominative after "Danas je" and the silent period after the year, and
//! keeps a slashed or hyphenated date from the telephone numbers and the
//! ranges (`05/03/1985` was a telephone number, `18-11-2026` "osamnaest
//! jedanaest do dvije tisuće ...").
//!
//! What is left alone: a day and month without a year written with slashes
//! or hyphens (`1/2`, `24/7`, `10-15`), a two-digit year with hyphens
//! (`10-12-14`), a number glued to more groups (`1/2/2026/3`, `2026-11-18-4`)
//! or to a letter, and a number after *telefon*, *tel*, *mob*, *faks*.

use std::borrow::Cow;

/// The text with its dates in the dotted form; the text itself when it has
/// none written otherwise.
pub(crate) fn normalize(text: &str) -> Cow<'_, str> {
    let bytes = text.as_bytes();
    let mut out: Option<String> = None;
    let mut copied = 0;
    let mut i = 0;
    while i < bytes.len() {
        if !bytes[i].is_ascii_digit() {
            i += 1;
            continue;
        }
        let run_end = digits_end(bytes, i);
        if starts_a_number(text, i)
            && let Some((end, date)) = written_date(text, i)
        {
            let out = out.get_or_insert_with(|| String::with_capacity(text.len()));
            out.push_str(&text[copied..i]);
            out.push_str(&date);
            copied = end;
            i = end;
        } else {
            i = run_end;
        }
    }
    match out {
        Some(mut out) => {
            out.push_str(&text[copied..]);
            Cow::Owned(out)
        }
        None => Cow::Borrowed(text),
    }
}

fn digits_end(bytes: &[u8], mut i: usize) -> usize {
    while bytes.get(i).is_some_and(u8::is_ascii_digit) {
        i += 1;
    }
    i
}

/// Whether the digits at `i` begin a number of their own: not glued to a
/// letter, a digit or a separator of another number before them, and not a
/// telephone number named as one.
fn starts_a_number(text: &str, i: usize) -> bool {
    let before = &text[..i];
    if before.chars().next_back().is_some_and(|c| c.is_alphanumeric() || "./-+,_\u{2011}".contains(c)) {
        return false;
    }
    let word: String = before
        .trim_end_matches([' ', ':', '.', '\t'])
        .chars()
        .rev()
        .take_while(|c| c.is_alphabetic())
        .collect::<Vec<_>>()
        .into_iter()
        .rev()
        .flat_map(char::to_lowercase)
        .collect();
    const TELEPHONE: &[&str] = &[
        "telefon",
        "telefona",
        "telefonu",
        "tel",
        "mob",
        "mobitel",
        "mobitela",
        "faks",
        "fax",
        "gsm",
        "телефон",
        "тел",
        "моб",
        "факс",
    ];
    !TELEPHONE.contains(&word.as_str())
}

/// A group of digits at `i`: its end and its value.
fn group(bytes: &[u8], i: usize, lengths: std::ops::RangeInclusive<usize>) -> Option<(usize, u32)> {
    let end = digits_end(bytes, i);
    if !lengths.contains(&(end - i)) {
        return None;
    }
    let value = bytes[i..end].iter().fold(0u32, |value, b| value * 10 + u32::from(b - b'0'));
    Some((end, value))
}

/// A separator of a date written with slashes or hyphens at `i`: its
/// character and its end. The non-breaking hyphen (U+2011) is a hyphen.
fn separator(text: &str, i: usize) -> Option<(char, usize)> {
    match text[i..].chars().next()? {
        '/' => Some(('/', i + 1)),
        '-' => Some(('-', i + 1)),
        '\u{2011}' => Some(('-', i + 3)),
        _ => None,
    }
}

/// One space as a typesetter writes it after an ordinal's period: no-break
/// space, narrow no-break space, thin space. Its end.
fn typeset_space(text: &str, i: usize) -> Option<usize> {
    let c = text[i..].chars().next()?;
    matches!(c, '\u{A0}' | '\u{202F}' | '\u{2009}').then(|| i + c.len_utf8())
}

/// The month a Roman numeral at `i` stands for (I to XII, capitals), and its
/// end.
fn roman_month(text: &str, i: usize) -> Option<(usize, u32)> {
    const ROMAN: [&str; 12] = ["XII", "VIII", "III", "VII", "XI", "IX", "IV", "VI", "II", "X", "V", "I"];
    const VALUE: [u32; 12] = [12, 8, 3, 7, 11, 9, 4, 6, 2, 10, 5, 1];
    let rest = &text[i..];
    let letters = rest.bytes().take_while(|b| matches!(b, b'I' | b'V' | b'X')).count();
    let k = ROMAN.iter().position(|roman| roman.len() == letters && rest.starts_with(roman))?;
    Some((i + letters, VALUE[k]))
}

/// Whether nothing that belongs to the number follows at `end`: no digit or
/// letter, and no separator with a digit after it (`1/2/2026/3`, a version
/// `2026.11.18.4`).
fn ends_a_number(text: &str, end: usize) -> bool {
    let mut rest = text[end..].chars();
    match rest.next() {
        None => true,
        Some(c) if c.is_alphanumeric() || c == '_' => false,
        Some('/' | '-' | '.' | ':' | ',' | '\u{2011}') => !rest.next().is_some_and(|c| c.is_ascii_digit()),
        Some(_) => true,
    }
}

fn dotted(day: u32, month: u32, year: &str) -> String {
    format!("{day}.{month}.{year}")
}

/// A date at `i` written otherwise than with periods and Arabic numerals:
/// the end of what it takes and its dotted form.
fn written_date(text: &str, i: usize) -> Option<(usize, String)> {
    let bytes = text.as_bytes();
    let first_end = digits_end(bytes, i);

    // The year first: 2026-11-18, 2026/11/18, 2026.11.18, with the time of
    // ISO 8601 after a T
    if first_end - i == 4 {
        let (_, year) = group(bytes, i, 4..=4)?;
        let (sep, month_at) = match bytes.get(first_end) {
            Some(b'.') => ('.', first_end + 1),
            _ => separator(text, first_end)?,
        };
        let (month_end, month) = group(bytes, month_at, 2..=2)?;
        let day_at = match (sep, bytes.get(month_end)) {
            ('.', Some(b'.')) => month_end + 1,
            ('.', _) => return None,
            _ => separator(text, month_end).filter(|&(second, _)| second == sep)?.1,
        };
        let (day_end, day) = group(bytes, day_at, 2..=2)?;
        if !(1000..=2999).contains(&year) || !(1..=12).contains(&month) || !(1..=31).contains(&day) {
            return None;
        }
        let mut date = dotted(day, month, &text[i..first_end]);
        // 2026-11-18T10:30: the T is a space
        if bytes.get(day_end) == Some(&b'T')
            && group(bytes, day_end + 1, 2..=2).is_some_and(|(hours_end, _)| bytes.get(hours_end) == Some(&b':'))
        {
            date.push(' ');
            return Some((day_end + 1, date));
        }
        return ends_a_number(text, day_end).then_some((day_end, date));
    }

    let (day_end, first) = group(bytes, i, 1..=2)?;

    // Slashes or hyphens: 18/11/2026, 18-11-2026, 18/11/26, the month first
    // where the second number is no month (11/18/2026), a Roman month
    if let Some((sep, second_at)) = separator(text, day_end) {
        let (second_end, second, roman) = match roman_month(text, second_at) {
            Some((end, month)) => (end, month, true),
            None => {
                let (end, value) = group(bytes, second_at, 1..=2)?;
                (end, value, false)
            }
        };
        let year_at = separator(text, second_end).filter(|&(other, _)| other == sep)?.1;
        let year_end = digits_end(bytes, year_at);
        let year_ok = match year_end - year_at {
            4 => bytes[year_at] != b'0',
            2 => sep == '/',
            _ => false,
        };
        if !year_ok || !ends_a_number(text, year_end) {
            return None;
        }
        let (day, month) = match (first, second) {
            (1..=31, 1..=12) => (first, second),
            (1..=12, 13..=31) if sep == '/' && !roman => (second, first),
            _ => return None,
        };
        return Some((year_end, dotted(day, month, &text[year_at..year_end])));
    }

    // Periods from here on
    if bytes.get(day_end) != Some(&b'.') || !(1..=31).contains(&first) {
        return None;
    }
    let after_day = day_end + 1;
    // One space of any kind after a period
    let space = |at: usize| match bytes.get(at) {
        Some(b' ') => (at + 1, false),
        _ => typeset_space(text, at).map_or((at, false), |end| (end, true)),
    };
    let (month_at, typeset_first) = space(after_day);

    // A Roman month: 18.XI.2026., 18. XI. 2026.
    if let Some((roman_end, month)) = roman_month(text, month_at) {
        if bytes.get(roman_end) != Some(&b'.') {
            return None;
        }
        let (year_at, _) = space(roman_end + 1);
        let year_end = digits_end(bytes, year_at);
        if !matches!(year_end - year_at, 2 | 4) || !ends_a_number(text, year_end) {
            return None;
        }
        return Some((year_end, dotted(first, month, &text[year_at..year_end])));
    }

    // Typeset spaces: 18.&nbsp;11.&nbsp;2026. becomes 18. 11. 2026.
    let (month_end, month) = group(bytes, month_at, 1..=2)?;
    if !(1..=12).contains(&month) || bytes.get(month_end) != Some(&b'.') {
        return None;
    }
    let (year_at, typeset_second) = space(month_end + 1);
    if !typeset_first && !typeset_second {
        return None;
    }
    let mut date = format!("{}. {}.", &text[i..day_end], &text[month_at..month_end]);
    let year_end = digits_end(bytes, year_at);
    if year_end > year_at {
        date.push(' ');
        date.push_str(&text[year_at..year_end]);
        return Some((year_end, date));
    }
    // A day and a month alone: only the first space was typeset
    typeset_first.then_some((month_end + 1, date))
}
