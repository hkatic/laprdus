//! The marks the engine puts at the end of a Slovenian clause, since the
//! front end sees one clause at a time and some words are told by what the
//! sentence goes on with. Private use characters, consumed by the Slovenian
//! front end, never spoken.

use crate::text::frontend::letters::{is_base_letter, is_one_of};

/// The mark the engine puts at the end of a clause whose comma is followed
/// by a clause [`opens_complement_clause`] says yes to: a verb of saying or
/// thinking is told by what follows it. Meni, da je prav. Ni obvestil, da
/// pride.
pub const BEFORE_COMPLEMENT_MARK: char = '\u{E000}';

/// The same for a clause before a comma or a semicolon in a sentence that
/// ends in a question mark, which only its last clause shows: Lahko delo
/// opravi vsak, ki hoče?
pub const IN_QUESTION_MARK: char = '\u{E001}';

/// The same for a sentence that ends in an exclamation mark: Veseli se,
/// dragi prijatelj! (the imperative vesêli; Veseli se novega dneva. is the
/// present)
pub const IN_EXCLAMATION_MARK: char = '\u{E002}';

/// The mark of a clause that is one capitalized word before a comma or after
/// one: an address (Pero, kje si? Pero, pridi sem. Hvala, Pero.), so the
/// word is read by the tables of names as inside a clause, where the name
/// Pêro wins over the pen peró (a listener's answer of 2026-11-03: the name
/// whatever ends the sentence). Pero je na mizi. stays the pen.
pub const VOCATIVE_MARK: char = '\u{E003}';

/// Whether a clause begins with da, naj, kdaj, kako ...
pub fn opens_complement_clause(text: &[char]) -> bool {
    const OPENERS: &[&str] = &["da", "naj", "kdaj", "kako", "kje", "zakaj", "ali", "kaj", "kdo", "koliko", "kam", "če"];
    // The first word of the clause, in small letters
    let lower = |c: char| match c {
        'Č' => 'č',
        'Š' => 'š',
        'Ž' => 'ž',
        _ => c.to_ascii_lowercase(),
    };
    let letters = text.iter().map(|&c| lower(c)).skip_while(|&c| !is_base_letter(c));
    let first: Vec<char> = letters.take_while(|&c| is_base_letter(c)).collect();
    is_one_of(&first, OPENERS)
}

/// Whether a clause is one capitalized word (Pero, Janez, Mama), the shape
/// of an address before a comma; for [`VOCATIVE_MARK`], with the sentence's
/// end.
pub fn is_address(text: &[char]) -> bool {
    // One word of letters, capitalized and not all in capitals, with the
    // engine's marks of the clause after it
    let letters: Vec<char> = text.iter().copied().filter(|c| !('\u{E000}'..='\u{E00F}').contains(c)).collect();
    let mut words = letters.split(|&c| c == ' ' || c == '\t').filter(|word| !word.is_empty());
    let (Some([first, rest @ ..]), None) = (words.next(), words.next()) else { return false };
    (first.is_ascii_uppercase() || matches!(first, 'Č' | 'Ć' | 'Š' | 'Ž' | 'Đ'))
        && !rest.is_empty()
        && rest.iter().all(|&c| is_base_letter(c))
}

/// Marks the clauses of a text for what the front end, which sees one clause
/// at a time, cannot see: a verb of saying or thinking by the clause after it
/// (Meni, da je prav. Ni obvestil, da pride.), lahko by the question mark at
/// the end of the sentence (Lahko delo opravi vsak, ki hoče?), the imperative
/// vesêli se by its exclamation mark (Veseli se, dragi prijatelj!), an
/// address by the comma next to it (Pero, kje si? Hvala, Pero.).
pub fn mark_clauses(clauses: &mut [crate::text::clauses::Clause]) {
    use crate::text::Punctuation::{Comma, Exclamation, Question, Semicolon};
    let (mut in_question, mut in_exclamation) = (false, false);
    for i in (0..clauses.len()).rev() {
        let punct = clauses[i].punct;
        let continues = matches!(punct, Comma | Semicolon);
        match punct {
            Question => (in_question, in_exclamation) = (true, false),
            Exclamation => (in_question, in_exclamation) = (false, true),
            _ if !continues => (in_question, in_exclamation) = (false, false),
            _ => {}
        }
        let complement = punct == Comma && clauses.get(i + 1).is_some_and(|next| opens_complement_clause(&next.text));
        // An address: one capitalized word before a comma or after one.
        let address = is_address(&clauses[i].text)
            && ((punct == Comma && i + 1 < clauses.len()) || (i > 0 && clauses[i - 1].punct == Comma));
        let marks = [
            (continues && in_question, IN_QUESTION_MARK),
            (continues && in_exclamation, IN_EXCLAMATION_MARK),
            (complement, BEFORE_COMPLEMENT_MARK),
            (address, VOCATIVE_MARK),
        ];
        clauses[i].text.extend(marks.into_iter().filter(|(set, _)| *set).map(|(_, mark)| mark));
    }
}
