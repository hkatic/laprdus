//! Serbian and Macedonian Cyrillic to Latin.

/// The lowercase letters and what they are in Latin script; the Macedonian
/// ѓ, ќ and ѕ are read as đ, ć and dz.
const LETTERS: [(char, &str); 33] = [
    ('а', "a"),
    ('б', "b"),
    ('в', "v"),
    ('г', "g"),
    ('д', "d"),
    ('ђ', "đ"),
    ('е', "e"),
    ('ж', "ž"),
    ('з', "z"),
    ('и', "i"),
    ('ј', "j"),
    ('к', "k"),
    ('л', "l"),
    ('љ', "lj"),
    ('м', "m"),
    ('н', "n"),
    ('њ', "nj"),
    ('о', "o"),
    ('п', "p"),
    ('р', "r"),
    ('с', "s"),
    ('т', "t"),
    ('ћ', "ć"),
    ('у', "u"),
    ('ф', "f"),
    ('х', "h"),
    ('ц', "c"),
    ('ч', "č"),
    ('џ', "dž"),
    ('ш', "š"),
    ('ѓ', "đ"),
    ('ќ', "ć"),
    ('ѕ', "dz"),
];

/// True for a character of the Cyrillic blocks (U+0400 to U+052F).
pub fn is_cyrillic(c: char) -> bool {
    ('\u{400}'..='\u{52F}').contains(&c)
}

/// Appends the Latin form of a character: a capital keeps its capital (Љ is
/// Lj, Џ is Dž), and anything that is not a known Cyrillic letter passes
/// through.
pub fn push_latin(c: char, out: &mut Vec<char>) {
    let latin = |c: char| LETTERS.iter().find(|&&(cyrillic, _)| cyrillic == c).map(|&(_, latin)| latin);
    if !is_cyrillic(c) {
        out.push(c);
    } else if let Some(latin) = latin(c) {
        out.extend(latin.chars());
    } else if let Some(latin) = c.to_lowercase().next().filter(|&lower| lower != c).and_then(latin) {
        let mut letters = latin.chars();
        out.extend(letters.next().into_iter().flat_map(char::to_uppercase).chain(letters));
    } else {
        out.push(c);
    }
}

/// Text with its Cyrillic letters in Latin script.
pub fn to_latin(text: &[char]) -> Vec<char> {
    let mut out = Vec::with_capacity(text.len() * 2);
    for &c in text {
        push_latin(c, &mut out);
    }
    out
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn transliterates() {
        let latin = |text: &str| to_latin(&text.chars().collect::<Vec<_>>()).into_iter().collect::<String>();
        assert_eq!(latin("Добар дан, Љубице и Џоне!"), "Dobar dan, Ljubice i Džone!");
        assert_eq!(latin("ђак Ћира Њ Ѕ ѓ Ќ щ abc"), "đak Ćira Nj Dz đ Ć щ abc");
    }
}
