//! From the text of a clause to its words: the tokenizer, the names of
//! symbols, the reading of abbreviations and lone letters, and the words
//! written with inner capitals.

use super::letters::{
    accented, chars, combining_mark, consonant_phone, ends_with, ends_with_any, eq, foreign, has_inner_syllabic_r,
    has_vowel, is_base_letter, is_one_of, is_vowel_letter, to_lower,
};
use super::{LanguageWords, MAX_WORD_LETTERS, Word};
use crate::formant::phones::Ph;
use crate::text::marks::{ITEM_BREAK_MARK, LETTER_CODE_MARK, MINOR_BREAK_MARK};

/// Two-letter words written vowel + consonant that are real words, so an
/// all-caps "ON" or "IZ" is not spelled out like "IT" or "UK": the list the
/// C++ engine had for every language, for a language without one of its own
/// ([`LanguageWords::is_short_word`]).
const SHORT_VC_WORDS: &[&str] =
    &["on", "od", "uz", "iz", "ih", "im", "um", "ah", "oh", "eh", "uh", "al", "il", "ej", "aj", "oj", "uf", "as", "ar"];

pub fn is_short_vc_word(w: &[char]) -> bool {
    is_one_of(w, SHORT_VC_WORDS)
}

/// The words of a name ("duplo ve"), each as a word of its own.
pub fn split_words(text: &str, out: &mut Vec<Word>) {
    out.extend(text.split(' ').filter(|word| !word.is_empty()).map(|word| Word::new(&chars(word))));
}

/// What the tokenizer keeps between the characters of a clause.
struct Tokenizer {
    words: Vec<Word>,
    current: Word,
    upper_count: usize,
    after_abbreviation_dot: bool,
    opening: bool,
    pending_break: bool,
    pending_item: bool,
}

impl Tokenizer {
    fn flush(&mut self) {
        let mut word = std::mem::replace(&mut self.current, Word::empty());
        if !word.w.is_empty() {
            word.all_caps = self.upper_count == word.w.len();
            word.after_opening = std::mem::take(&mut self.opening);
            word.break_before = self.pending_break;
            word.item_break = self.pending_break && self.pending_item;
            self.pending_break = false;
            self.pending_item = false;
            if self.after_abbreviation_dot && word.w.len() == 1 {
                word.spell = true;
            }
            self.words.push(word);
        }
        self.upper_count = 0;
        self.after_abbreviation_dot = false;
    }

    fn push(&mut self, letter: char, mark: u8, upper: bool) {
        self.current.w.push(letter);
        self.current.marks.push(mark);
        self.upper_count += usize::from(upper);
    }
}

/// The words of a clause, with the symbols that are read by name wherever
/// they stand as the words of their names.
pub fn tokenize(text: &[char], language: &dyn LanguageWords) -> Vec<Word> {
    let mut t = Tokenizer {
        words: Vec::new(),
        current: Word::empty(),
        upper_count: 0,
        after_abbreviation_dot: false,
        opening: false,
        pending_break: false,
        pending_item: false,
    };

    let letter_at = |pos: Option<usize>, want_upper: bool| {
        pos.and_then(|pos| text.get(pos)).is_some_and(|&raw| {
            let lower = to_lower(raw);
            is_base_letter(lower) && (lower != raw) == want_upper
        })
    };
    let letters_from = |pos: usize| text[pos..].iter().take_while(|&&c| is_base_letter(to_lower(c))).count();
    // Names written with a capital inside, after small letters, are made
    // of words, and each part is read as a word of its own: Eleven Labs,
    // Chat GPT, Power Point. Only after two small letters, so not after a
    // single one (iPhone, eUprava, BiH, PhD, McDonald, and the Lj, Nj, Dž
    // of Serbian Cyrillic capitals: ЉУБАВЉУ is LjUBAVLjU), and not before a
    // single capital at the end. Capitals followed by small letters stay
    // one word (PDFom, SMSati, KONZUMklik). join_camel() puts back together
    // the words the lexicons know and those made of one-syllable parts
    // (TikTok, TalkBack).
    let camel_break = |letters: usize, pos: usize| {
        letters >= 2
            && letter_at(pos.checked_sub(1), false)
            && letter_at(pos.checked_sub(2), false)
            && letters_from(pos) >= 2
    };

    for (pos, &raw) in text.iter().enumerate() {
        let c = to_lower(raw);
        let upper = c != raw;

        if t.current.w.len() >= MAX_WORD_LETTERS {
            t.flush();
        }

        // The number converter's marks: a short pause before the next word,
        // and the letters of a designation such as A17
        if raw == MINOR_BREAK_MARK || raw == ITEM_BREAK_MARK {
            t.flush();
            t.pending_break = true;
            t.pending_item |= raw == ITEM_BREAK_MARK;
            continue;
        }
        if raw == LETTER_CODE_MARK {
            t.current.code = !t.current.w.is_empty() && t.current.w.len() <= 2;
            t.flush();
            continue;
        }

        if is_base_letter(c) {
            if upper && camel_break(t.current.w.len(), pos) {
                t.current.camel = true;
                t.flush();
                t.current.camel = true;
                t.current.camel_join = true;
            }
            t.current.capitalized |= upper && t.current.w.is_empty();
            t.push(c, 0, upper);
            continue;
        }

        let mark = combining_mark(c);
        if mark != 0 {
            if let Some(last) = t.current.marks.last_mut() {
                *last |= mark;
            }
            continue;
        }

        if let Some((base, mark)) = accented(c) {
            t.current.capitalized |= upper && t.current.w.is_empty();
            t.push(base, mark, upper);
            continue;
        }
        if let Some(replacement) = foreign(c) {
            t.current.capitalized |= upper && t.current.w.is_empty();
            for letter in replacement.chars() {
                t.push(letter, 0, upper);
            }
            continue;
        }

        // Apostrophes join (elided vowels: "al'", "k'o"); everything else splits.
        if c == '\'' || c == '\u{2019}' {
            continue;
        }

        // A period after a single letter is the dot of an abbreviation
        // ("s.a.r.s.", "U.S.A.", "t.j."): it is silent, and the letter
        // before it and a single letter after it are read by name, as
        // synthesizer B reads them. (The dot after the last letter is
        // a clause end and never reaches the front end.)
        if c == '.' && t.current.w.len() == 1 {
            t.current.spell = true;
            t.flush();
            t.after_abbreviation_dot = true;
            continue;
        }

        if matches!(c, '-' | '\u{2010}' | '\u{2011}') {
            t.current.before_hyphen = true;
        }
        t.flush();
        // Quotes and brackets open a sentence of their own (Rekla je „S tobom
        // nikad“); spaces keep that, every other character ends it.
        if matches!(c, '"' | '(' | '[' | '{' | '„' | '“' | '‚' | '‘' | '«' | '»') {
            t.opening = true;
        } else if c != ' ' {
            t.opening = false;
        }
        // The colon of a web address ("https://", "ftp://") is silent, like
        // the slashes after it.
        if c == ':' && text.get(pos + 1) == Some(&'/') && text.get(pos + 2) == Some(&'/') {
            continue;
        }
        if let Some(name) = language.symbol_name(c) {
            split_words(name, &mut t.words);
        }
    }
    t.flush();
    t.words
}

/// Abbreviations without a vowel and short all-caps tokens are read letter by
/// letter ("HR" -> "ha er", "USB" -> "u es be"). A word whose only syllable
/// would be an r at its end is an abbreviation too ("dr", "mr", "str", "npr",
/// "hr"): a syllabic r stands inside a word (prst, vrt, krv) or at its head
/// (rt, rđa), never alone at the end after consonants.
pub fn needs_spelling(word: &Word, language: &dyn LanguageWords) -> bool {
    let w = &word.w[..];
    let vowel = has_vowel(w);
    if !vowel && !has_inner_syllabic_r(w) {
        return true;
    }
    // RTV, RTL, RTS: capitals without a vowel that open with r and two more
    // consonants are an acronym; a word's syllabic r at the head has one
    // consonant after it (rt, rž)
    if !vowel && word.all_caps && w.len() >= 3 && w[0] == 'r' {
        return true;
    }
    if !word.all_caps || w.len() > 5 {
        return false;
    }
    let v = |i: usize| is_vowel_letter(w[i]);
    match w.len() {
        1 => !v(0),
        // CV; VC that is no word; VV, CC
        2 => (v(0) || !v(1)) && !(v(0) && !v(1) && language.is_short_word(w)),
        // TRG, KRV; CVC, CVV; VCV; CCV are words, VCC and VVC are not
        3 => vowel && !(!v(0) && v(1)) && !(v(0) && !v(1) && v(2)) && !(!v(0) && !v(1) && v(2)),
        // NVDA, HDMI: three consonants that cannot open a word. Native
        // clusters end in a sonorant (STRAH, SKLOP, STVAR) and stay words.
        _ => !v(0) && !v(1) && !v(2) && !matches!(w[2], 'r' | 'l' | 'v' | 'j'),
    }
}

/// The prepositions "s" and "k" are single consonants that lean on the next
/// word ("s njom", "k meni"); alone they are just letters. A capital says
/// nothing at the head of a sentence ("S tobom sam htio sve", "K meni") or in
/// all-caps text ("ALI SAM S NJOM"), so the letter is a preposition there too;
/// a capital anywhere else is a letter ("Mercedes S klasa", "pritisnite S za
/// spremanje"), unless an instrumental follows it ("Javno S ponosom", where
/// a host joined two lines with a space): no letter S stands before one.
/// Never before a hyphen ("S-klasa", "k-pop"), a single letter ("S i M") or a
/// word no preposition is followed by ("S je slovo").
fn leaning_preposition(tokens: &[Word], t: usize, language: &dyn LanguageWords) -> bool {
    let token = &tokens[t];
    // S22, K5
    let (true, false, false, Some(next)) =
        (language.is_consonant_preposition(&token.w), token.before_hyphen, token.code, tokens.get(t + 1))
    else {
        return false;
    };
    // s ne malo truda
    let enclitic = language.is_enclitic(&next.w) && !language.is_pronoun_after_preposition(&next.w);
    if next.w.len() == 1 || enclitic || (language.is_proclitic(&next.w) && !eq(&next.w, "ne")) {
        return false;
    }
    if !token.all_caps || t == 0 || token.after_opening {
        return true;
    }
    let caps_word = |w: &Word| w.all_caps && w.w.len() > 1 && !needs_spelling(w, language);
    if eq(&token.w, "s") && !next.all_caps && ends_with_any(&next.w, &["om", "em", "im", "ima", "ama", "lju", "šću"])
    {
        return true; // Javno S ponosom
    }
    caps_word(next) || (next.all_caps && caps_word(&tokens[t - 1])) // S USB KABELOM
}

/// A consonant letter standing alone as a word (b, s, њ): what a screen
/// reader reports as a character ("veliko slovo B", "Obrisano B", "B
/// deleted"). Not one before a hyphen, which is part of a compound
/// (S-klasa, k-pop), and not q, x, y, named in both spelling modes, or w,
/// whose name is two words (expand_words()).
fn lone_consonant_letter(token: &Word) -> bool {
    !token.before_hyphen
        && (is_one_of(&token.w, &["lj", "nj", "dž"]) || (token.w.len() == 1 && consonant_phone(token.w[0]) != Ph::Sil))
}

/// The letters read one by one, each with its accent, in the prefix of a web
/// address in any case: a listener's report of November 2026, "ha te te pe es"
/// for https, "ef te pe" for ftp and "ve ve ve" for www, in every language; the
/// usual reading of www names the v, not the w. `None` for any other word.
fn address_letters(w: &[char]) -> Option<Vec<char>> {
    if eq(w, "www") {
        Some(chars("vvv"))
    } else {
        is_one_of(w, &["http", "https", "ftp", "ftps", "sftp"]).then(|| w.to_vec())
    }
}

/// Appends the names of `letters`, one after another, as words that are
/// letter names; `mark` sets what else they are.
fn push_letter_names(
    letters: &[char],
    language: &dyn LanguageWords,
    words: &mut Vec<Word>,
    mark: impl Fn(&mut Word),
) -> usize {
    let start = words.len();
    for letter in letters {
        split_words(language.letter_name(std::slice::from_ref(letter)), words);
    }
    for word in &mut words[start..] {
        word.letter_name = true;
        mark(word);
    }
    start
}

/// Tokens to words: the letters of abbreviations and designations by their
/// names, a lone letter by its name or its sound. With `letter_sounds` a lone
/// consonant letter is spoken by its sound ("bə"), as when it is spelled in
/// that mode, instead of by its name ("be").
pub fn expand_words(tokens: Vec<Word>, language: &dyn LanguageWords, letter_sounds: bool) -> Vec<Word> {
    // Decided before the loop below moves the tokens away.
    let leaning: Vec<bool> = (0..tokens.len()).map(|t| leaning_preposition(&tokens, t, language)).collect();

    let mut words: Vec<Word> = Vec::new();
    for (mut token, leaning) in tokens.into_iter().zip(leaning) {
        // The short pause before a token stands before the first word made of it
        let made_from = words.len();
        let (pause, item) = (token.break_before, token.item_break);
        expand_token(&mut token, leaning, language, letter_sounds, &mut words);
        if pause && let Some(first) = words.get_mut(made_from) {
            first.break_before = true;
            first.item_break = item;
        }
    }
    words
}

fn expand_token(
    token: &mut Word,
    leaning: bool,
    language: &dyn LanguageWords,
    letter_sounds: bool,
    words: &mut Vec<Word>,
) {
    let keep = |token: &mut Word, words: &mut Vec<Word>| words.push(std::mem::take(token));
    let as_sound = |token: &mut Word| {
        token.letter_sound = true;
        token.letter_name = true;
    };
    let begin_word = |words: &mut Vec<Word>, start: usize| {
        if let Some(first) = words.get_mut(start) {
            first.word_start = true;
        }
    };

    if token.spell {
        push_letter_names(&token.w[..token.w.len().min(1)], language, words, |_| ());
        return;
    }
    // Abbreviations are not expanded (dr., npr., km): they are read as
    // written, by the rules for any other word.
    if token.is_marked() {
        return keep(token, words);
    }
    if let Some(letters) = address_letters(&token.w) {
        push_letter_names(&letters, language, words, |word| word.own_accent = true);
        return;
    }
    // w alone is two words, "dvojni ve", "duplo ve"; by sounds the second
    // is the sound of the v: "dvojni və", "duplo və" (the same report)
    if eq(&token.w, "w") && !token.before_hyphen {
        let first = push_letter_names(&token.w, language, words, |word| word.own_accent = true);
        if letter_sounds && words.len() > first {
            let mut sound = Word::new(&['v']);
            as_sound(&mut sound);
            sound.own_accent = true;
            words.pop();
            words.push(sound);
        }
        return;
    }
    // The letters of a designation glued to a number (A17, S22, NA5:
    // LETTER_CODE_MARK) are letters whatever word they are spelled
    // like: one letter by the spelling mode, as any lone letter, two
    // by their names, as an abbreviation. Without this the A of A17
    // was the conjunction a and leaned on the number ("Marjan A17" was
    // heard as "Marijana 17"), U21 began with the preposition u and
    // S22, at the head of a clause, with the preposition s.
    if token.code && !eq(&token.w, "w") {
        if token.w.len() == 1 && letter_sounds && consonant_phone(token.w[0]) != Ph::Sil {
            as_sound(token);
            return keep(token, words);
        }
        let start = push_letter_names(&token.w, language, words, |_| ());
        return begin_word(words, start);
    }
    if letter_sounds && !leaning && lone_consonant_letter(token) {
        // A full word of its own, never a clitic (see letter_name)
        as_sound(token);
        return keep(token, words);
    }
    if token.w.len() == 2 && lone_consonant_letter(token) {
        // A digraph alone is one letter (Њ, nj): "nje", not "en je"
        let start = words.len();
        split_words(language.letter_name(&token.w), words);
        for word in &mut words[start..] {
            word.letter_name = true;
        }
        return begin_word(words, start);
    }
    if !leaning && needs_spelling(token, language) {
        let start = push_letter_names(&token.w, language, words, |_| ());
        return begin_word(words, start);
    }
    keep(token, words);
}

/// Foreign letters: x = ks, q = k (qu = kv), w = v, y = i or j.
pub fn rewrite_foreign(word: &mut Word) {
    let spelled = std::mem::take(&mut word.w);
    let old_marks = std::mem::take(&mut word.marks);
    let mut origin = Vec::with_capacity(spelled.len());
    let mut i = 0;
    while i < spelled.len() {
        let c = spelled[i];
        let mark = old_marks.get(i).copied().unwrap_or(0);
        let vowel_at = |at: Option<usize>| at.and_then(|at| spelled.get(at)).is_some_and(|&c| is_vowel_letter(c));
        let mut put = |letter: char, mark: u8, from: usize| {
            word.w.push(letter);
            word.marks.push(mark);
            origin.push(from);
        };
        match c {
            'x' => {
                put('k', 0, i);
                put('s', 0, i);
            }
            'q' => {
                put('k', 0, i);
                if spelled.get(i + 1) == Some(&'u') && vowel_at(Some(i + 2)) {
                    put('v', 0, i + 1);
                    i += 1;
                }
            }
            'w' => put('v', 0, i),
            'y' => {
                let glide = vowel_at(i.checked_sub(1)) || vowel_at(Some(i + 1));
                put(if glide { 'j' } else { 'i' }, if glide { 0 } else { mark }, i);
            }
            _ => put(c, mark, i),
        }
        i += 1;
    }
    word.foreign = word.w != spelled;
    word.spelled = spelled;
    word.origin = origin;
}

/// A lone consonant letter in letter-sound mode: the consonant and a
/// stressed neutral vowel, "bə", the syllable a letter's sound alone has
/// when it is spelled. The full schwa vowel, which both kinds of voices
/// render inside running speech (the recorded voices by their e).
pub fn phonemize_letter_sound(word: &mut Word) {
    let consonant = match word.w.as_slice() {
        ['l', 'j'] => Ph::Lj,
        ['n', 'j'] => Ph::Nj,
        ['d', 'ž'] => Ph::Dzh,
        [c, ..] => consonant_phone(*c),
        [] => Ph::Sil,
    };
    word.add_phone(consonant, 0, false);
    word.add_phone(Ph::Ax, word.w.len().saturating_sub(1), true);
    word.stress = Some(0);
}

/// Joins the parts of a word with inner capitals back into one word where
/// `known` knows the whole, or where every part is one syllable (counted by
/// `syllables`), the last perhaps with a case ending: TalkBack, TalkBacku,
/// GitHub, PayPal are compounds stressed on their first part, not two words.
pub fn join_camel(
    tokens: Vec<Word>,
    syllables: impl Fn(&[char]) -> usize,
    known: impl Fn(&[char]) -> bool,
) -> Vec<Word> {
    let one_syllable = |part: &[char], last: bool| {
        syllables(part) == 1
            || (last
                && ["a", "u", "e", "i", "om", "em", "ov", "ova"].iter().any(|ending| {
                    let length = ending.chars().count();
                    part.len() > length + 1 && ends_with(part, ending) && syllables(&part[..part.len() - length]) == 1
                }))
    };
    let mut out: Vec<Word> = Vec::with_capacity(tokens.len());
    let mut tokens = tokens.into_iter().peekable();
    while let Some(first) = tokens.next() {
        let mut parts = vec![first];
        while let Some(part) = tokens.next_if(|next| next.camel_join) {
            parts.push(part);
        }
        if parts.len() > 1 {
            let joined: Vec<char> = parts.iter().flat_map(|part| part.w.iter().copied()).collect();
            let last = parts.len() - 1;
            let short_parts = parts.iter().enumerate().all(|(i, part)| one_syllable(&part.w, i == last));
            if short_parts || known(&joined) {
                let mut parts = parts.into_iter();
                let mut word = parts.next().unwrap_or_default();
                for part in parts {
                    word.w.extend(part.w);
                    word.marks.extend(part.marks);
                    word.all_caps &= part.all_caps;
                    word.before_hyphen = part.before_hyphen;
                }
                word.camel = false;
                out.push(word);
                continue;
            }
        }
        out.extend(parts);
    }
    out
}
