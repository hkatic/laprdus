//! The texts of the string tables and the numbers written into them.

use crate::ids::{IDS_DECIMAL_SEPARATOR, IDS_VALUE_PERCENT};
use crate::win32::{self, Window};

/// A number for [`printf`].
#[derive(Clone, Copy, Debug)]
pub enum Number {
    Int(i32),
    Float(f32),
}

/// Sets the texts of a dialog's controls from the string table: pairs of
/// control id and string id. A missing string leaves the template's text.
pub fn set_texts(dialog: Window, texts: &[(u16, u16)]) {
    for &(control, string) in texts {
        let text = win32::string(string);
        if !text.is_empty() {
            dialog.item(control).set_text(&text);
        }
    }
}

/// The format strings of the string tables: `%d`, `%.1f` and `%%`, each
/// conversion taking the next number.
pub fn printf(format: &str, numbers: &[Number]) -> String {
    let mut out = String::new();
    let mut numbers = numbers.iter();
    let mut chars = format.chars().peekable();
    while let Some(c) = chars.next() {
        if c != '%' {
            out.push(c);
            continue;
        }
        let mut precision = None;
        if chars.next_if_eq(&'.').is_some() {
            let mut digits = 0;
            while let Some(digit) = chars.next_if(char::is_ascii_digit) {
                digits = digits * 10 + digit as usize - '0' as usize;
            }
            precision = Some(digits.min(9));
        }
        match chars.next() {
            Some('d' | 'f') => match numbers.next() {
                Some(Number::Int(value)) => out += &value.to_string(),
                Some(Number::Float(value)) => out += &format!("{value:.*}", precision.unwrap_or(6)),
                None => {}
            },
            Some('%') => out.push('%'),
            Some(other) => out.extend(['%', other]),
            None => out.push('%'),
        }
    }
    out
}

/// The first "%s" of a resource string replaced by a text; the text appended
/// when there is none.
pub fn insert(format: &str, argument: &str) -> String {
    if format.contains("%s") { format.replacen("%s", argument, 1) } else { format.to_owned() + argument }
}

/// The first decimal point as the language writes it ("1,5x"), when the
/// separator is one character.
pub fn with_separator(text: &str, separator: &str) -> String {
    if separator.chars().count() == 1 { text.replacen('.', separator, 1) } else { text.to_owned() }
}

/// A multiplier given in percent: "1.0x", and "0.25x" where one decimal is
/// not enough.
pub fn times(percent: i32) -> String {
    let percent = percent.max(0);
    if percent % 10 == 0 {
        format!("{}.{}x", percent / 100, percent % 100 / 10)
    } else {
        format!("{}.{:02}x", percent / 100, percent % 100)
    }
}

/// A text with the decimal separator of the UI language.
pub fn localized(text: &str) -> String {
    with_separator(text, &win32::string(IDS_DECIMAL_SEPARATOR))
}

/// "50%" or "50 %", as the UI language writes it.
pub fn percent(value: i32) -> String {
    let format = win32::string(IDS_VALUE_PERCENT);
    printf(if format.is_empty() { "%d%%" } else { &format }, &[Number::Int(value)])
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_formats_of_the_string_tables() {
        assert_eq!(printf("%d%%", &[Number::Int(50)]), "50%");
        assert_eq!(printf("%d %%", &[Number::Int(50)]), "50 %");
        assert_eq!(printf("%d ms", &[Number::Int(100)]), "100 ms");
        assert_eq!(
            printf("%.1fx (do %d riječi/min)", &[Number::Float(1.5), Number::Int(525)]),
            "1.5x (do 525 riječi/min)"
        );
        assert_eq!(printf("%.1fx", &[Number::Float(0.75)]), "0.8x");
        assert_eq!(printf("100%", &[]), "100%");
        assert_eq!(printf("%d and %d", &[Number::Int(1)]), "1 and ");
    }

    #[test]
    fn a_reason_goes_where_the_string_asks() {
        assert_eq!(insert("Unos nije ispravan: %s", "nema naglaska"), "Unos nije ispravan: nema naglaska");
        assert_eq!(insert("Invalid: ", "why"), "Invalid: why");
        assert_eq!(insert("%s (%s)", "100%d"), "100%d (%s)");
    }

    #[test]
    fn multipliers_have_the_decimals_they_need() {
        assert_eq!(times(100), "1.0x");
        assert_eq!(times(150), "1.5x");
        assert_eq!(times(25), "0.25x");
        assert_eq!(times(105), "1.05x");
        assert_eq!(times(400), "4.0x");
        assert_eq!(with_separator(&times(150), ","), "1,5x");
        assert_eq!(with_separator(&times(150), "."), "1.5x");
        assert_eq!(with_separator("1.5x (up to 525 WPM)", ","), "1,5x (up to 525 WPM)");
        assert_eq!(with_separator("1.5x", ""), "1.5x");
    }
}
