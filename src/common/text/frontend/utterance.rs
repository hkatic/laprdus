//! From the analysed words of a clause to the utterance the synthesizers
//! take: the clause type, and the words flattened into one string of phones.

use super::Word;
use super::letters::{consonant_phone, eq, is_foreign_letter, is_vowel_letter};
use crate::formant::phones::{Ph, Phone};
use crate::prosody::{ClauseKind, Utterance};
use crate::text::Punctuation;

/// The clause type from the punctuation and the words, then the words
/// flattened into one phone string with word, syllable and stress
/// information: shared by every language. `li_particle` is the Croatian
/// question particle li, whose host carries the peak of a yes/no question.
/// Returns, for each word, whether it is a clitic that leans on its
/// neighbour.
pub fn finish_utterance(
    words: &mut [Word],
    punct: Punctuation,
    is_question_word: impl Fn(&[char]) -> bool,
    li_particle: bool,
    utt: &mut Utterance,
) -> Vec<bool> {
    // ---- Clause type ----
    utt.kind = match punct {
        Punctuation::Question => {
            // A question word among the first two words that are not clitics
            let mut checked = 0;
            let mut wh = false;
            for word in words.iter() {
                if checked >= 2 {
                    break;
                }
                if is_question_word(&word.w) {
                    wh = true;
                    break;
                }
                checked += usize::from(!word.clitic);
            }
            if wh { ClauseKind::WhQuestion } else { ClauseKind::YesNoQuestion }
        }
        Punctuation::Exclamation => ClauseKind::Exclamation,
        Punctuation::Comma | Punctuation::Semicolon | Punctuation::Colon => ClauseKind::Continuation,
        _ => ClauseKind::Statement,
    };
    if utt.kind == ClauseKind::YesNoQuestion
        && li_particle
        && let Some(i) = (1..words.len()).find(|&i| eq(&words[i].w, "li") && !words[i - 1].nuclei.is_empty())
    {
        // "Znaš li...", "Je li...", "Da li...": the peak sits on the
        // word the particle leans on.
        let host = &mut words[i - 1];
        host.clitic = false;
        host.prominence = host.prominence.max(2);
        utt.focus_word = i32::try_from(i - 1).unwrap_or(-1);
    }

    // ---- Flatten into one phone string ----
    // A joined word counts as part of the one before it.
    let mut group: Vec<usize> = Vec::with_capacity(words.len());
    let mut group_syllables = vec![0usize; words.len()];
    for (wi, word) in words.iter().enumerate() {
        let of = if wi > 0 && word.joined { group[wi - 1] } else { wi };
        group.push(of);
        group_syllables[of] += word.nuclei.len();
    }

    let mut word_is_clitic = Vec::with_capacity(words.len());
    let mut syllable = 0usize;
    for (wi, word) in words.iter().enumerate() {
        word_is_clitic.push(word.clitic && !word.joined);
        let n = word.nuclei.len();
        let mut current = syllable; // consonants before the first vowel
        for (pi, &phone) in word.phones.iter().enumerate() {
            let mut phone = phone;
            // Wraps past 65,535 words, as the syllable does past 32,767
            phone.word = group[wi] as u16;
            phone.word_start = pi == 0 && !word.joined;
            // A short pause before the word, when something was said before it
            phone.break_before = pi == 0 && word.break_before && !word.joined && !utt.phones.is_empty();
            phone.item_break = phone.break_before && word.item_break;
            phone.prominence = word.prominence;
            phone.word_syllables = group_syllables[group[wi]].min(255) as u8;

            if phone.nucleus {
                let k = word.nuclei.iter().position(|&at| at == pi).unwrap_or(n);
                current = syllable + k;
                if !word.clitic && word.stress == Some(k) {
                    phone.stressed = true;
                    phone.accent = word.accent;
                }
            } else if !phone.nucleus_tail
                && let Some(k) = word.nuclei.iter().position(|&at| at > pi)
            {
                // Onset consonants belong to the following vowel.
                current = syllable + k;
            }
            phone.syllable = current as i16;
            // The conjunctions i and a and the prepositions u and o are a
            // syllable each: the synthesizers give them their full vowel.
            phone.vowel_word =
                word.phones.len() == 1 && phone.nucleus && phone.ph.is_vowel() && !phone.stressed && !word.joined;
            utt.phones.push(phone);
        }
        syllable += n;
    }
    utt.syllable_count = i32::try_from(syllable).unwrap_or(i32::MAX);

    // Stress and length of a syllabic r belong to all its parts (three in
    // Croatian, Serbian and Bosnian: vocoid, tap, vocoid; two in Slovenian).
    let mut nucleus: Option<Phone> = None;
    for phone in &mut utt.phones {
        match nucleus {
            Some(head) if phone.nucleus_tail => {
                phone.stressed = head.stressed;
                phone.is_long = head.is_long;
            }
            _ => nucleus = phone.nucleus.then_some(*phone),
        }
    }
    word_is_clitic
}

/// A clause needs at least one accent, and a lone function word ("Ne.",
/// "Da.", "Ovo:") is the whole message.
pub fn ensure_accent(words: &mut [Word]) {
    let accented = |word: &Word| !word.clitic && !word.nuclei.is_empty();
    if !words.iter().any(accented)
        && let Some(last) = words.iter_mut().rev().find(|word| !word.nuclei.is_empty())
    {
        last.clitic = false;
        last.prominence = 2;
    }
    let mut full = words.iter_mut().filter(|word| accented(word));
    if let (Some(only), None) = (full.next(), full.next()) {
        only.prominence = 2;
    }
}

/// The sound of one letter alone, for spelling by sounds: a vowel as a
/// stressed syllable (`vowel` says which), a consonant followed by a short
/// neutral vowel ("bə", "sə"). An unknown letter and the foreign letters,
/// which are spelled by their names, give no phones.
///
/// A consonant alone is spoken as the consonant followed by a short neutral
/// vowel ("bə", "sə", "mə"), the way the letters are sounded out when
/// children learn to read and the way synthesizer B's Croatian voice spells
/// them: a held fricative or a bare burst with nothing after it is hard to
/// tell from the next letter, and a stop cannot be heard at all without a
/// release.
pub fn letter_sound(letter: &[char], vowel: impl Fn(char) -> Ph) -> Utterance {
    let mut utt = Utterance { isolated_sound: true, syllable_count: 1, ..Utterance::default() };
    let (first, is_vowel) = match *letter {
        ['l', 'j'] => (Ph::Lj, false),
        ['n', 'j'] => (Ph::Nj, false),
        ['d', 'ž'] => (Ph::Dzh, false),
        [c] if is_foreign_letter(letter) || (!is_vowel_letter(c) && consonant_phone(c) == Ph::Sil) => return utt,
        [c] if is_vowel_letter(c) => (vowel(c), true),
        [c] => (consonant_phone(c), false),
        _ => return utt,
    };
    let phone = |ph: Ph, word_start: bool| Phone {
        ph,
        word_start,
        prominence: 2,
        word: 0,
        syllable: 0,
        word_syllables: 1,
        // The syllable: the vowel letter itself, or the release vowel
        nucleus: ph == Ph::Schwa || is_vowel,
        stressed: ph == Ph::Schwa || is_vowel,
        ..Phone::default()
    };
    utt.phones.push(phone(first, true));
    if !is_vowel {
        utt.phones.push(phone(Ph::Schwa, false));
    }
    utt
}
