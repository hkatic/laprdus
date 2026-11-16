//! Designations written like the address of a spreadsheet cell (A17, F5,
//! AB12): their letters are marked as letters, and an address that follows a
//! cell's contents is set off from them by a pause.

use super::scan::{Out, Text, is_word_byte, item_break, latin_word};
use crate::text::marks::LETTER_CODE_MARK;

/// The signs a cell address may touch in a formula, a range or a list
/// (=A1+B2, A1:B5, (A17), "A17"). A hyphen, a period, a slash and an underscore
/// make a longer designation, which is left alone (R2-D2, A4-format, PS/2).
fn is_formula_sign(c: u8) -> bool {
    b"()[]{}\"'=+*<>|^,;:!?".contains(&c)
}

/// The start of the one or two capital letters of the English alphabet that
/// end at `end`, when they begin a word of their own or touch a sign of a
/// formula.
fn letters_begin(t: Text, end: usize) -> Option<usize> {
    let mut begin = end;
    while begin > 0 && t.at(begin - 1).is_ascii_uppercase() {
        begin -= 1;
    }
    if begin == end || end - begin > 2 {
        return None;
    }
    if begin > 0 && t.spaces_begin(begin) == begin {
        let before = t.at(begin - 1);
        if before != b'\n' && before != b'\r' && !is_formula_sign(before) {
            return None;
        }
    }
    Some(begin)
}

/// A designation written like a spreadsheet cell's address: one or two
/// capital letters of the English alphabet glued to a number of up to seven
/// digits (A17, B2, AB120; F5, A4, S22, U21, PS5, H2O), not the continuation
/// of a longer one (R2-D2's D2, PS/2). `digits` is the first digit; returns
/// the first letter, and whether the designation stands as a word of its
/// own, as a cell's address does: A4-format and I2C are designations but no
/// addresses. A clock time or a decimal after capital letters is neither
/// (U20:03, V1.5). Three letters are left out: Excel's columns reach XFD, but
/// in a text such a word is far more often a name with its own reading
/// (TOP10, WIN11, HRT1).
fn code_begin(t: Text, digits: usize) -> Option<(usize, bool)> {
    let begin = letters_begin(t, digits)?;
    let end = t.digits_end(digits);
    if t.at(digits) == b'0' || end - digits > 7 || (matches!(t.at(end), b'.' | b',' | b':') && t.is_digit(end + 1)) {
        return None;
    }
    let alone = end >= t.len()
        || t.skip_spaces(end) != end
        || matches!(t.at(end), b'\n' | b'\r' | b'.')
        || is_formula_sign(t.at(end));
    Some((begin, alone))
}

/// A cell's address as NVDA says it in Excel: the letters and the number as
/// two words, "A 3" (the driver's log of November 2026: the items 'Marijan  '
/// and 'A 3  '; with the glued form alone the listener heard no change, the A
/// stayed the conjunction a). Written so, the letters are far more often a
/// word (U 5 sati, A 3 su došla, NA 5 mjesta), so this form is an address only
/// as an item of its own: one space between the letters and the number, and
/// after the number the end of the text, of the line or of a screen reader's
/// item (two spaces or more). `digits` is the first digit; returns the first
/// letter.
fn spaced_code_begin(t: Text, digits: usize) -> Option<usize> {
    if digits < 2 || t.at(digits - 1) != b' ' {
        return None;
    }
    let begin = letters_begin(t, digits - 1)?;
    // In a text of capitals the letters are a word: DOLAZIM U 5
    let word_end = t.spaces_begin(begin);
    let mut word = word_end;
    while word > 0 && t.at(word - 1).is_ascii_uppercase() {
        word -= 1;
    }
    if word_end - word >= 3 && (word == 0 || !is_word_byte(t.at(word - 1))) {
        return None;
    }
    let end = t.digits_end(digits);
    if t.at(digits) == b'0' || end - digits > 7 {
        return None;
    }
    let item_end = end >= t.len()
        || matches!(t.at(end), b'\n' | b'\r')
        || (t.at(end) == b' ' && (end + 1 >= t.len() || t.at(end + 1) == b' '));
    item_end.then_some(begin)
}

/// The words after which the designation belongs to the phrase and no pause
/// stands before it (u A1, od A1 do B5, A1 i B2, v A1): the prepositions and
/// conjunctions of the four languages.
fn leans_on_next(word: &str) -> bool {
    #[rustfmt::skip]
    const WORDS: &[&str] = &[
        "u", "na", "o", "po", "pri", "iz", "od", "do", "s", "sa", "k", "ka", "za", "uz", "kod", "bez", "kroz",
        "pred", "nad", "pod", "oko", "prema", "preko", "između", "i", "ili", "a", "te", "pa", "ni", "niti",
        "ali", "da", "kao", "nego",
        "v", "z", "h", "ob", "in", "ter", "kot", "med", "čez", "skozi", "proti",     // Slovenian
    ];
    WORDS.contains(&word)
}

/// True when what stands before `pos`, with spaces between, is a word or a
/// number that does not lean on what follows: the place of a short pause
/// before a cell's address. Nothing before it, the end of a clause or a sign
/// of a formula needs none.
fn follows_an_item(t: Text, pos: usize) -> bool {
    let before = t.spaces_begin(pos);
    if before == pos || before == 0 || b"\0\n\r.,;:!?-/=+*<>|^([{".contains(&t.at(before - 1)) {
        return false;
    }
    !leans_on_next(&latin_word(&t.prev_word(before).word).0.to_ascii_lowercase())
}

/// A designation at the number that starts at `digits` (see `code_begin()`):
/// its letters, already in the output, get the mark that makes them letters
/// (A is not the conjunction a, S not the preposition, NA not the word), and
/// one that stands alone, as a cell's address does, has a pause before it (the
/// whole comma pause: half of it was not heard through a screen reader) when
/// the cell's contents or any other item came first: "Marjan A17" was
/// heard as "Marijana 17" (a listener's report of November 2026). The number
/// itself is read by the caller as before. A number right after an address
/// gets the pause too: "A20 5" is the cell A20 with a 5 in it, not A25.
pub(super) fn mark_cell_address(t: Text, digits: usize, out: &mut Out) {
    // "A 3", the address as two words: the space goes with the letters
    let (begin, alone, gap) = match (code_begin(t, digits), spaced_code_begin(t, digits)) {
        (Some((begin, alone)), _) => (begin, alone, 0),
        (None, Some(begin)) => (begin, true, 1),
        (None, None) => {
            // The contents after the address: A20 5
            let before = t.spaces_begin(digits);
            if before == digits || before == 0 || !t.is_digit(before - 1) {
                return;
            }
            let mut number = before;
            while number > 0 && t.is_digit(number - 1) {
                number -= 1;
            }
            if matches!(code_begin(t, number), Some((_, true))) {
                out.trim_blanks();
                out.push_str(&item_break());
            }
            return;
        }
    };
    let letters = t.bytes(begin, digits - gap);
    let written = out.len().checked_sub(letters.len() + gap).filter(|&at| out.0[at..].starts_with(letters));
    let Some(written) = written else { return };
    out.0.truncate(written);
    if alone && follows_an_item(t, begin) {
        out.trim_blanks();
        out.push_str(&item_break());
    }
    out.0.extend_from_slice(letters);
    out.push_str(LETTER_CODE_MARK.encode_utf8(&mut [0; 4]));
    out.push(b' ');
}
