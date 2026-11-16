//! Letter-to-sound rules of Serbian: the digraphs, the
//! "ije" of the long jat, the syllabic r.

use crate::formant::phones::Ph;
use crate::text::frontend::Word;
use crate::text::frontend::letters::{
    consonant_phone, eq, find, has_at, has_vowel, is_consonant_letter, is_one_of, is_syllabic_r, is_vowel_letter,
    starts_with, tail, vowel_phone,
};
use crate::text::frontend::tokenizer::{phonemize_letter_sound, rewrite_foreign};

/// d + ž across a prefix boundary are two sounds (nadživjeti, podžupan).
fn splits_dz(w: &[char], i: usize) -> bool {
    const PREFIXES: [&str; 4] = ["nad", "pod", "od", "pred"];
    const ROOTS: [&str; 5] = ["živ", "žnj", "žup", "žeć", "žet"];
    PREFIXES.iter().any(|prefix| i + 1 == prefix.len() && starts_with(w, prefix))
        && ROOTS.iter().any(|root| has_at(w, i + 1, root))
}

/// n + j that are not the letter nj (injekcija, konjunktiv): the pattern and
/// where its n stands.
fn splits_nj(w: &[char], i: usize) -> bool {
    const PATTERNS: [(&str, usize); 6] =
        [("injekc", 1), ("injic", 1), ("konjug", 2), ("konjunk", 2), ("vanjezi", 2), ("tanjug", 2)];
    PATTERNS.iter().any(|&(pattern, n_offset)| i >= n_offset && has_at(w, i - n_offset, pattern))
}

/// "ije" as the long reflex of jat is one syllable, [i̯eː] (lijep, mlijeko,
/// vrijeme). It stays two syllables at the end of a word (nije, prije), in
/// verb and comparative endings (pijem, starijeg) and in loans (klijent).
fn is_ije_diphthong(w: &[char], i: usize) -> bool {
    if i == 0 || !is_consonant_letter(w[i - 1]) {
        return false;
    }
    let rest = tail(w, i + 3);
    let Some(&after) = rest.first() else { return false };
    if is_vowel_letter(after) || is_one_of(rest, &["m", "š", "mo"]) {
        return false;
    }
    if eq(rest, "te") {
        return is_one_of(w, &["dijete", "svijete", "cvijete"]);
    }
    if is_one_of(rest, &["g", "ga", "mu", "h"]) && has_vowel(&w[..i]) {
        return false;
    }
    if starts_with(rest, "nt") {
        return false;
    }
    // The inserted a of G plural pacìjenātā removes the -nt- cue above,
    // but i-je remains two syllables just as in pacijent and pacijentima.
    if eq(w, "pacijenata") {
        return false;
    }
    // The loans in -ier (ka-ri-jé-ra, pre-mì-jer, ba-ri-jé-ra, ho-te-lì-jer,
    // in-te-rì-jer, ho-te-li-jer-ski) and the Greek hiero- (hi-je-ràr-hi-ja):
    // "ije" before r is jat only in zvijer, pomijerati and after the
    // prefixes prije-, poslije- (prijeratni)
    if after == 'r'
        && find(w, "zvijer").is_none()
        && !starts_with(w, "pomijer")
        && !starts_with(w, "prije")
        && !starts_with(w, "poslije")
    {
        return false;
    }
    if starts_with(w, "higijen") || starts_with(w, "hijen") {
        return false;
    }
    // The Hebrew names in -el (Dà-ni-jel, Gà-bri-jel), not jat
    if starts_with(w, "danijel") || starts_with(w, "gabrijel") {
        return false;
    }
    // Passive -bi-jen belongs to biti 'hit', not to the long reflex of
    // jat: obìjen/ubìjen and their inflections have separate i and e.
    // Restrict this to verbal prefixes; bijel/bijedan retain their jat.
    if w[i - 1] == 'b' && after == 'n' {
        let prefix = &w[..i - 1];
        let prefix = if starts_with(prefix, "ne") { &prefix[2..] } else { prefix };
        if is_one_of(prefix, &["", "o", "u", "do", "iz", "na", "od", "po", "pre", "pri", "pro", "raz", "s", "za"]) {
            return false;
        }
    }
    // The passive participles of viti with a prefix (razvìjen, sàvijen,
    // zàvijen, ùvijen, prìvijen): vi-jen, not jat
    if w[i - 1] == 'v'
        && after == 'n'
        && is_one_of(&w[..i - 1], &["raz", "sa", "za", "u", "pre", "iz", "po", "od", "oba", "pod", "nad", "pri"])
    {
        return false;
    }
    if starts_with(w, "dijet")
        && !eq(w, "dijete")
        && w.len() <= 8
        && is_one_of(rest, &["ta", "ti", "tu", "tom", "tama"])
    {
        return false;
    }
    true
}

/// The phones of a word, with the letter each comes from and the syllable
/// nuclei.
pub fn phonemize(word: &mut Word) {
    if word.letter_sound {
        return phonemize_letter_sound(word);
    }
    rewrite_foreign(word);
    let w = std::mem::take(&mut word.w);
    let mut i = 0;
    while i < w.len() {
        let c = w[i];
        let next = w.get(i + 1).copied().unwrap_or('\0');
        if c == 'd' && next == 'ž' && !splits_dz(&w, i) {
            word.add_phone(Ph::Dzh, i, false);
            i += 1;
        } else if c == 'l' && next == 'j' {
            word.add_phone(Ph::Lj, i, false);
            i += 1;
        } else if c == 'n' && next == 'j' && !splits_nj(&w, i) {
            word.add_phone(Ph::Nj, i, false);
            i += 1;
        } else if c == 'i' && next == 'j' && w.get(i + 2) == Some(&'e') && is_ije_diphthong(&w, i) {
            word.add_phone(Ph::J, i, false);
            if let Some(glide) = word.phones.last_mut() {
                glide.short_glide = true;
            }
            word.add_phone(Ph::E, i + 2, true);
            if let Some(vowel) = word.phones.last_mut() {
                vowel.is_long = true;
            }
            // Accent marks may sit on either vowel letter of the digraph.
            if let Some(mark) = word.marks.get_mut(i).map(std::mem::take)
                && let Some(to) = word.marks.get_mut(i + 2)
            {
                *to |= mark;
            }
            i += 2;
        } else if is_vowel_letter(c) {
            word.add_phone(vowel_phone(c), i, true);
        } else if c == 'r' && is_syllabic_r(&w, i) {
            word.add_phone(Ph::Schwa, i, true);
            for ph in [Ph::R, Ph::Schwa] {
                word.add_phone(ph, i, false);
                if let Some(part) = word.phones.last_mut() {
                    part.nucleus_tail = true;
                }
            }
        } else if (c == 'l' || c == 'n')
            && i + 1 == w.len()
            && i > 0
            && is_consonant_letter(w[i - 1])
            && !matches!(w[i - 1], 'r' | 'l' | 'j')
        {
            // Syllabic l/n at the end of loans: bicikl, ansambl, njutn.
            word.add_phone(Ph::Schwa, i, true);
            word.add_phone(consonant_phone(c), i, false);
        } else {
            let ph = consonant_phone(c);
            if ph != Ph::Sil {
                word.add_phone(ph, i, false);
            }
        }
        i += 1;
    }
    word.w = w;
}

/// The number of syllables the rules give a spelling.
pub fn syllable_count(w: &[char]) -> usize {
    let mut probe = Word::new(w);
    phonemize(&mut probe);
    probe.nuclei.len()
}
