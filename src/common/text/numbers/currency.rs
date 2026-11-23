//! Currency amounts: grouping, signs and affixes on either side of the
//! number, read as words before the punctuation is segmented.

use super::NumberReader;
use super::scan::{Out, Text, is_word_byte};

/// A currency, by its place in the language's unit table.
#[derive(Clone, Copy, PartialEq, Eq)]
enum Currency {
    Euro = 1,
    Dollar = 2,
}

/// The ways a currency is written: its sign or its code.
const TOKENS: [(&str, Currency, bool); 4] = [
    ("€", Currency::Euro, false),
    ("$", Currency::Dollar, false),
    ("EUR", Currency::Euro, true),
    ("USD", Currency::Dollar, true),
];

/// Whether an amount may start at `pos`: not inside a word, an identifier or
/// another number.
fn is_amount_boundary(t: Text, pos: usize) -> bool {
    if pos == 0 || t.spaces_begin(pos) != pos {
        return true;
    }
    let c = t.at(pos - 1);
    !is_word_byte(c) && !matches!(c, b'_' | b'/' | b'-' | b'+' | b'.' | b',')
}

/// The start of a minus that ends at `end` (hyphen, minus sign or en dash),
/// `end` when there is none.
fn minus_before(t: Text, end: usize) -> usize {
    if end > 0 && t.at(end - 1) == b'-' {
        end - 1
    } else if t.has_before(end, "\u{2212}") || t.has_before(end, "\u{2013}") {
        end - 3
    } else {
        end
    }
}

/// The end of a minus that starts at `pos` (hyphen or minus sign), `pos`
/// when there is none.
fn minus_end(t: Text, pos: usize) -> usize {
    if t.at(pos) == b'-' {
        pos + 1
    } else if t.has(pos, "\u{2212}") {
        pos + 3
    } else {
        pos
    }
}

/// Digit mode has no grammatical hook to expand a unary minus. Make it a
/// word before punctuation is segmented, preserving T-2, ranges and phones.
pub(super) fn read_digit_mode_minus(t: Text, i: usize, out: &mut Out) {
    let end = t.spaces_begin(i);
    let start = minus_before(t, end);
    if start != end && is_amount_boundary(t, start) && out.take_back(t.bytes(start, i)) {
        out.push_str("minus ");
    }
}

/// The currency written at `pos`, and its end.
fn currency_at(t: Text, pos: usize) -> Option<(Currency, usize)> {
    TOKENS.iter().find_map(|&(text, currency, code)| {
        let stop = pos + text.len();
        (t.has(pos, text) && !(code && is_word_byte(t.at(stop)))).then_some((currency, stop))
    })
}

/// The digit groups of an amount and what separates them: '.', ',', ' ' or
/// an apostrophe (straight or curly).
#[derive(Default)]
struct Amount<'a> {
    groups: Vec<&'a str>,
    separators: Vec<u8>,
    end: usize,
}

fn scan_amount(t: Text, i: usize) -> Amount {
    let mut a = Amount::default();
    let mut pos = i;
    loop {
        let stop = t.digits_end(pos);
        a.groups.push(t.str(pos, stop));
        a.end = stop;
        if a.groups.len() > 105 {
            return Amount::default(); // the engine's centillion limit
        }
        let (separator, next) = match t.at(stop) {
            separator @ (b'.' | b',' | b'\'') => (separator, stop + 1),
            _ if t.has(stop, "’") => (b'\'', stop + 3),
            _ => (b' ', t.skip_spaces(stop)),
        };
        if next == stop || !t.is_digit(next) {
            break;
        }
        // Whitespace can separate unrelated values. Only exact groups of
        // three are candidates; newlines never join an amount.
        if matches!(separator, b' ' | b'\'') && (a.groups[0].len() > 3 || t.digits_end(next) - next != 3) {
            break;
        }
        a.separators.push(separator);
        pos = next;
    }
    a
}

/// A currency amount at `i`, or a grouped decimal that can be nothing else
/// ("2.150,00"). Writes its words and returns its end.
///
/// CLDR UTS #35, Numbers: decimal/group symbols are locale-specific;
/// currency and sign affixes may occur on either side, and accounting
/// negatives use parentheses. Recognize structure, then speak semantic
/// words so punctuation settings cannot silence the sign or currency.
/// <https://unicode.org/reports/tr35/tr35-numbers.html#Number_Format_Patterns>
pub(super) fn convert(r: &NumberReader, t: Text, i: usize, out: &mut Out, by_digit: bool) -> Option<usize> {
    let mut currency = None;
    let mut start = i;
    let (mut negative, mut positive, mut bracket) = (false, false, false);
    for _ in 0..3 {
        let end = t.spaces_begin(start);
        let signed = negative || positive;
        if !signed && minus_before(t, end) != end {
            start = minus_before(t, end);
            negative = true;
        } else if !signed && end > 0 && t.at(end - 1) == b'+' {
            start = end - 1;
            positive = true;
        } else if !bracket && end > 0 && t.at(end - 1) == b'(' {
            start = end - 1;
            bracket = true;
        } else {
            let token = TOKENS.iter().filter(|_| currency.is_none()).find(|&&(text, _, code)| {
                t.has_before(end, text) && (!code || is_amount_boundary(t, end - text.len()))
            });
            let Some(&(text, found, _)) = token else { break };
            currency = Some(found);
            start = end - text.len();
        }
    }
    if !is_amount_boundary(t, start) {
        return None;
    }
    let a = scan_amount(t, i);
    let last_group = *a.groups.last()?;
    let mut end = a.end;
    if currency.is_none() {
        let affix = t.skip_spaces(end);
        let mut after = currency_at(t, affix);
        // Some exports put the sign between the amount and its currency:
        // 2.150,00- € (CLDR negative suffix patterns).
        if after.is_none() && !negative && !positive {
            let sign_end = minus_end(t, affix);
            if sign_end > affix {
                after = currency_at(t, t.skip_spaces(sign_end));
                negative = after.is_some();
            }
        }
        if let Some((found, stop)) = after {
            currency = Some(found);
            end = stop;
        }
    }
    // A trailing minus is financial notation only with an explicit currency,
    // and at the end of the token; a dash followed by another value stays a range.
    if currency.is_some() && !negative && !positive {
        let sign_start = t.skip_spaces(end);
        let stop = minus_end(t, sign_start);
        let next = t.skip_spaces(stop);
        if stop > sign_start && (next == t.len() || (!is_word_byte(t.at(next)) && t.at(next) != b'$')) {
            negative = true;
            end = stop;
        }
    }
    if bracket {
        let close = t.skip_spaces(end);
        // Bare parenthesized numbers are not accounting balances.
        if currency.is_none() || t.at(close) != b')' {
            return None;
        }
        end = close + 1;
        negative = true;
    }
    if t.skip_spaces(end) == end
        && (is_word_byte(t.at(end)) || matches!(t.at(end), b'_' | b':') || t.is_separator_between_digits(end))
    {
        return None;
    }

    let (first_separator, last_separator) =
        (a.separators.first().copied().unwrap_or(0), a.separators.last().copied().unwrap_or(0));
    let mut decimal = false;
    if matches!(last_separator, b'.' | b',') {
        let repeated = a.separators.iter().position(|&s| s == last_separator) != Some(a.separators.len() - 1);
        if !repeated {
            decimal = a.separators.len() > 1 || last_separator == b',' || last_group.len() != 3;
            // A single separator with three following digits is ambiguous.
            // Keep local dot grouping; a dollar amount also accepts the
            // common US comma grouping ($2,150). €2,150 stays decimal.
            if a.separators.len() == 1
                && last_separator == b','
                && last_group.len() == 3
                && currency == Some(Currency::Dollar)
            {
                decimal = false;
            }
        }
    }
    let integer_groups = a.groups.len() - usize::from(decimal);
    if integer_groups > 1
        && (a.groups[0].len() > 3
            || (1..integer_groups).any(|k| a.groups[k].len() != 3 || a.separators[k - 1] != first_separator))
    {
        return None;
    }
    // Without a currency marker only an unambiguous grouped decimal is
    // monetary here. Leave times, dates, identifiers and ordinary number
    // grammar to their existing paths, including spaced phone digits.
    if currency.is_none()
        && (!decimal
            || integer_groups < 2
            || !matches!(last_separator, b',' | b'.')
            || t.unit_at(end, r.units(), "").is_some())
    {
        return None;
    }
    if decimal && integer_groups > 1 && last_separator == first_separator {
        return None;
    }

    let whole = a.groups[..integer_groups].concat();
    let unit = &r.units()[currency.map_or(0, |currency| currency as usize)];
    let counted = if currency.is_none() || decimal {
        String::new()
    } else {
        r.grammar.currency_amount(r, t, start, &whole, unit)
    };
    let mut words = if by_digit {
        let fraction =
            if decimal { r.separator_words(b',').to_owned() + &r.digit_names(last_group) } else { String::new() };
        r.digit_names(&whole) + &fraction
    } else if !decimal {
        counted.clone()
    } else {
        r.grammar.decimal(r, &whole, last_group)
    };
    if currency.is_some() && (by_digit || decimal) {
        words.push(' ');
        words += if decimal { unit.genitive_singular } else { &counted[counted.rfind(' ').map_or(0, |at| at + 1)..] };
    }
    // A preceding affix may already belong to an earlier number (1€2€).
    // Only replace literal input still present at the end of the output.
    if !out.take_back(t.bytes(start, i)) {
        return None;
    }
    out.set_off();
    if negative {
        out.push_str("minus ");
    } else if positive {
        out.push_str("plus ");
    }
    out.push_str(&words);
    Some(end)
}
