//! One clause of Serbian text to phones: the lexical
//! stress of every word, the clitics, the accents that depend on the words
//! around a word, and the sound changes between words.

use super::english::{english_entry, english_rule, english_spelling, english_word};
use super::frontend::Frontend;
use super::lexicon::Entries;
use super::phonemize::{phonemize, syllable_count};
use super::stress::{StressResult, StressRules};
use super::words::{COUNTING_NUMBERS, ENCLITICS, FUNCTION_WORDS, NE_SHIFT_VERBS, PROCLITICS, QUESTION_WORDS, Words};
use crate::formant::phones::{Accent, Ph, Phone};
use crate::prosody::Utterance;
use crate::text::frontend::Word;
use crate::text::frontend::letters::{
    MARK_FALL, MARK_LONG, MARK_RISE, MARK_STRESS, ends_with_any, eq, is_one_of, is_vowel_letter,
};
use crate::text::frontend::notation::LexEntry;
use crate::text::frontend::sounds::{is_postalveolar, voiced_of, voiceless_of};
use crate::text::frontend::tokenizer::{expand_words, join_camel, rewrite_foreign, tokenize};
use crate::text::frontend::utterance::{ensure_accent, finish_utterance};
use crate::text::{Punctuation, cyrillic};

/// The longest stem of `stems` a word begins with: the word less up to
/// three letters, and at least two letters.
fn longest_stem<'a>(stems: &'a Entries, w: &[char]) -> Option<&'a LexEntry> {
    let shortest = if w.len() > 3 { w.len() - 3 } else { 1 }.max(2);
    (shortest..=w.len()).rev().find_map(|len| stems.get(&w[..len]))
}

impl Frontend {
    fn user_stem_matches(&self, w: &[char]) -> bool {
        longest_stem(&self.user_stems, w).is_some()
    }
}

/// Puts the stress a rule found on the word.
fn apply(word: &mut Word, r: StressResult) {
    word.stress = usize::try_from(r.nucleus).ok();
    word.accent = r.accent;
    if r.is_long {
        word.set_long(usize::try_from(r.nucleus).unwrap_or(usize::MAX), true);
    }
    if let Ok(after) = usize::try_from(r.long_after) {
        word.set_long(after, true);
    }
}

/// "ti" and "mi" are the dative clitics (Kažem ti, Daj mi) or the stressed
/// pronouns tȋ, mȋ. A clitic cannot follow a lone conjunction (A ti?, I ti i
/// ja, Ni mi ne znamo), and a verb of the second person singular does not
/// take the dative ti (that is sebi, si), so next to one the word is its
/// subject: Kako si ti?, Jesi li ti to rekao?, Što ti radiš?; mi the same
/// next to the first person plural (Što mi radimo?, Jesmo li mi?).
fn stressed_pronoun(words: &[Vec<char>], i: usize) -> bool {
    let w = &words[i];
    let you = eq(w, "ti");
    if !you && !eq(w, "mi") {
        return false;
    }
    if i > 0 && is_one_of(&words[i - 1], &["a", "i", "ni"]) {
        return true;
    }
    let agrees = |v: &[char]| {
        let long = v.iter().filter(|&&c| is_vowel_letter(c)).count() >= 2;
        if you {
            // radiš, kažeš, imaš; not naš, vaš, baš (one syllable)
            is_one_of(v, &["si", "jesi", "nisi", "ćeš", "nećeš", "hoćeš", "znaš", "daš"])
                || (long && ends_with_any(v, &["aš", "eš", "iš"]))
        } else {
            // radimo, idemo, znamo; not the adverbs samo, tamo, mimo
            is_one_of(v, &["smo", "jesmo", "nismo", "ćemo", "nećemo", "hoćemo", "bismo"])
                || (long
                    && ends_with_any(v, &["amo", "emo", "imo"])
                    && !is_one_of(
                        v,
                        &["samo", "tamo", "ovamo", "onamo", "kamo", "nekamo", "nikamo", "svakamo", "mimo"],
                    ))
        }
    };
    if i > 0 {
        // Jesi li ti
        let before = if eq(&words[i - 1], "li") && i > 1 { i - 2 } else { i - 1 };
        if agrees(&words[before]) {
            return true;
        }
    }
    words.get(i + 1).is_some_and(|next| agrees(next))
}

/// The phones and the lexical stress of one word. `first` says the word
/// opens the clause, `at_head` that only words a command may follow stand
/// before it.
fn analyze(frontend: &Frontend, word: &mut Word, first: bool, at_head: bool) {
    let lexicon = frontend.lexicon;
    let user_verbs = frontend.user_verbs();
    phonemize(word);
    let n = word.nuclei.len();
    if n == 0 {
        word.clitic = true;
        word.prominence = 0;
        return;
    }

    let nucleus_letters = word.nucleus_letters();
    let nucleus_from_letter = |letter: usize| nucleus_letters.iter().position(|&at| at >= letter).unwrap_or(n - 1);
    // A word spelled the English way, unless the English table has it
    // as a native word (Emilia, Julia: the names Emilija, Julija)
    let english_shape =
        !word.letter_name && english_spelling(&word.spelled) && english_entry(&word.spelled) != Some(None);

    // 1. Accent marks written in the text
    for i in 0..word.marks.len() {
        let mark = word.marks[i];
        if mark == 0 {
            continue;
        }
        let k = nucleus_from_letter(i);
        if mark & MARK_LONG != 0 {
            word.set_long(k, true);
        }
        if mark & MARK_STRESS != 0 {
            word.stress = Some(k);
            word.explicit_stress = true;
            word.accent = if mark & MARK_FALL != 0 {
                Accent::Falling
            } else if mark & MARK_RISE != 0 {
                Accent::Rising
            } else {
                Accent::None
            };
        }
    }

    // A command at the head of its clause is the verb even where the
    // built-in lexicon has the noun of the same spelling ("Napomeni mu",
    // but "u napomeni"); the rules below find it. The user's entries
    // still win.
    let command_at_head = word.stress.is_none()
        && at_head
        && StressRules::new(word, true, user_verbs).verb_form().nucleus >= 0
        && StressRules::new(word, false, user_verbs).verb_form().nucleus < 0;

    // 2. Lexicon: exact form, then the longest stem; the user's entries
    // before the built-in ones at every step. A word of one syllable
    // spelled with a foreign letter is not the native word it is
    // rewritten to ("Wi" of Wi-Fi is not the pronoun vȋ).
    if word.stress.is_none() && !(word.foreign && n == 1) {
        // User stems and verbs must also precede built-in exact forms:
        // adding a dictionary correction must not disable an existing
        // personal accent (for example a povucite verb override).
        let mut entry: Option<LexEntry> =
            frontend.user_exact.get(&word.w).or_else(|| longest_stem(&frontend.user_stems, &word.w)).copied();
        word.user_accent = entry.is_some();
        if entry.is_none()
            && let Some(verbs) = user_verbs
        {
            let mut r = StressResult::default();
            if StressRules::new(word, at_head, user_verbs).verb_form_in(verbs, &mut r) {
                apply(word, r);
                word.user_accent = true;
            }
        }
        let user_stem = frontend.user_stem_matches(&word.w);
        // Place names and personal names (the tables of towns, villages,
        // regions, names and surnames) only for a word written with a
        // capital letter, so that a village or a name spelled like a
        // common word does not change that word (Púla, pȕla; Mȋrna,
        // mìrna); not in text written all in capitals, where a capital
        // says nothing (ZATVORI PROZOR). The first word of a clause has
        // its capital anyway: there a name spelled like a common word
        // (Bȃr, bar) and a word of the built-in lexicon keep the common
        // reading. A stem of the user's comes before the tables.
        if entry.is_none() && !word.user_accent && !command_at_head && word.capitalized && !word.all_caps && !user_stem
        {
            let clause_start = first || word.after_opening;
            if !(clause_start && (lexicon.exact.contains_key(&word.w) || lexicon.proper_mid.contains(&word.w))) {
                entry = lexicon.proper.get(&word.w).copied();
            }
        }
        if entry.is_none() && !word.user_accent && !command_at_head {
            entry = lexicon.exact.get(&word.w).copied();
        }
        // English words (see "English words"): after the native
        // lexicon, before its stems, which would take English words
        // for native ones with an ending; a stem of the user's wins.
        if entry.is_none()
            && !word.user_accent
            && !word.letter_name
            && !user_stem
            && let Some(letter) = english_word(&word.spelled, english_shape)
        {
            // The nucleus of a letter of the spelling as written (x, q, w
            // and y rewritten after it)
            let rewritten = word.origin.iter().take_while(|&&origin| origin < letter).count();
            word.stress = Some(nucleus_from_letter(rewritten));
            word.english = true;
        }
        if entry.is_none() && !word.user_accent && !word.english {
            let shortest = if word.w.len() > 3 { word.w.len() - 3 } else { 1 }.max(2);
            for len in (shortest..=word.w.len()).rev() {
                let prefix = &word.w[..len];
                entry = frontend.user_stems.get(prefix).copied();
                word.user_accent |= entry.is_some();
                if entry.is_none() && !command_at_head {
                    entry = lexicon.stems.get(prefix).copied();
                }
                if entry.is_some() {
                    break;
                }
            }
        }
        if let Some(entry) = entry {
            if let Some(letter) = entry.stress_letter {
                word.stress = Some(nucleus_from_letter(usize::from(letter)));
                word.accent = entry.accent;
            }
            for (k, &letter) in nucleus_letters.iter().enumerate() {
                if letter < 32 && entry.long_letters & (1 << letter) != 0 {
                    word.set_long(k, true);
                }
            }
        }
    }

    // 3. Suffix rules, 4. default: first syllable; an English word by
    // the English rules
    if word.stress.is_none() && english_shape {
        let native = StressRules::new(word, at_head, user_verbs).derivation();
        if native.nucleus >= 0 {
            apply(word, native);
        } else {
            // The letter of the spelling as written each nucleus comes from
            let spelled: Vec<usize> =
                nucleus_letters.iter().filter_map(|&letter| word.origin.get(letter).copied()).collect();
            word.stress = Some(english_rule(&word.spelled, &spelled));
            word.english = true;
        }
    }
    if word.stress.is_none() {
        let rules = StressRules::new(word, at_head, user_verbs);
        let mut r = rules.strong();
        if r.nucleus < 0 {
            r = rules.weak();
        }
        if r.nucleus >= 0 {
            apply(word, r);
        } else {
            word.stress = Some(0);
        }
    }

    // Tone: only falling accents on monosyllables, only rising accents
    // on non-initial syllables. An English word has a plain stress,
    // its pitch peak inside the stressed vowel, wherever it is.
    if n == 1 {
        word.accent = Accent::Falling;
    } else if word.english {
        word.accent = Accent::Neutral;
    } else if word.stress.is_some_and(|stress| stress > 0) {
        word.accent = Accent::Rising;
    } else if word.accent == Accent::None {
        word.accent = Accent::Neutral;
    }
}

/// "boli", "bole" are the cases of bȏl (the lexicon), and the present of
/// bòljeti (it hurts: bòlī, bòlē) where the words around make the verb
/// plain: a pronoun clitic right after it or before it, with at most
/// two adverbs between ("Boli me glava", "Boli li te?", "Što te boli?",
/// "Glava me jako boli"), "je" after it at the head of the clause ("Boli
/// je glava"), and before it negation, an adverb, a conjunction, a
/// stressed pronoun or a part of the body ("Ne boli", "To jako boli",
/// "Gdje boli?", "ako boli", "Mene boli glava", "Srce boli"). Never
/// within two words after a preposition ("bez boli", "zbog jake boli ga
/// je ...", "protiv te boli").
fn boli_is_verb(spelling: &[Vec<char>], i: usize) -> bool {
    const CLITICS: &[&str] = &["me", "te", "ga", "ju", "nas", "vas", "ih", "mi", "ti", "mu", "joj", "nam", "vam", "im"];
    const ADVERBS: &[&str] = &[
        "ne", "jako", "tako", "još", "stvarno", "užasno", "strašno", "baš", "uvijek", "uvek", "opet", "stalno", "isto",
        "sad", "sada", "često", "i", "dalje", "nimalo", "uopće", "uopšte",
    ];
    const CUES: &[&str] = &[
        "gdje",
        "gde",
        "kako",
        "što",
        "šta",
        "ako",
        "kad",
        "kada",
        "dok",
        "jer",
        "čim",
        "mene",
        "tebe",
        "njega",
        "nju",
        "njih",
        "glava",
        "srce",
        "noga",
        "ruka",
        "zub",
        "grlo",
        "trbuh",
        "želudac",
        "stomak",
        "uho",
        "duša",
        "koljeno",
        "rame",
        "vrat",
        "leđa",
        "noge",
        "ruke",
        "zubi",
        "oči",
        "uši",
        "koljena",
        "ramena",
        "prsa",
        "prst",
        "prsti",
        "kičma",
        "kralježnica",
        "bubreg",
        "bubrezi",
        "mišići",
        "zglobovi",
    ];
    const PREPOSITIONS: &[&str] = &[
        "u", "na", "o", "po", "pri", "bez", "od", "do", "zbog", "iz", "s", "sa", "za", "nakon", "protiv", "kroz",
        "pod", "nad", "pred", "među", "između", "uz", "prema", "usprkos", "unatoč", "poput",
    ];
    let count = spelling.len();
    let before = |back: usize| &spelling[i - back][..];
    if (1..=i.min(2)).any(|back| is_one_of(before(back), PREPOSITIONS)) {
        return false;
    }
    let after = if spelling.get(i + 1).is_some_and(|next| eq(next, "li")) { i + 2 } else { i + 1 };
    if spelling.get(after).is_some_and(|next| is_one_of(next, CLITICS))
        || (i == 0 && count > 1 && eq(&spelling[1], "je"))
    {
        return true;
    }
    for back in 1..=i.min(3) {
        // A clitic cannot open its clause: "Te boli" is the demonstrative
        if back < i && is_one_of(before(back), CLITICS) {
            return true;
        }
        if !is_one_of(before(back), ADVERBS) {
            break;
        }
    }
    i > 0 && (is_one_of(before(1), CUES) || (is_one_of(before(1), ADVERBS) && !eq(before(1), "i")))
}

/// The word takes the accent of the word after it, which leans on it.
fn take_accent(host: &mut Word, donor: &mut Word) {
    host.clitic = false;
    host.prominence = if donor.prominence > 0 { donor.prominence } else { 2 };
    host.stress = Some(0);
    host.accent = Accent::Rising;
    donor.clitic = true;
    donor.prominence = 0;
}

/// Sound changes inside words and clitic groups.
fn assimilate(ph: &mut Vec<Phone>, word_is_clitic: &[bool]) {
    let clitic = |phone: &Phone| word_is_clitic.get(usize::from(phone.word)).copied().unwrap_or(false);
    // True when two phones are pronounced without a word break
    let bound = |left: &Phone, right: &Phone| left.word == right.word || clitic(left) || clitic(right);

    for i in (1..ph.len()).rev() {
        let right = ph[i];
        let left = &mut ph[i - 1];
        if !bound(left, &right) {
            continue;
        }
        // Regressive voicing assimilation between obstruents
        // (predsjednik -> [pretsjednik], s bratom -> [zbratom]).
        if left.ph.is_obstruent() && right.ph.is_obstruent() {
            left.ph = if right.ph.def().voiced { voiced_of(left.ph) } else { voiceless_of(left.ph) };
        }
        // s, z before š ž č ć dž đ (s čim -> [ščim]); š, ž before ć, đ.
        if is_postalveolar(right.ph) {
            left.ph = match left.ph {
                Ph::S => Ph::Sh,
                Ph::Z => Ph::Zh,
                other => other,
            };
        }
        if matches!(right.ph, Ph::Tj | Ph::Dj) {
            left.ph = match left.ph {
                Ph::Sh => Ph::Sj,
                Ph::Zh => Ph::Zj,
                other => other,
            };
        }
        // Nasal place: n before k, g is velar; n before p, b is m.
        if left.ph == Ph::N {
            if matches!(right.ph, Ph::K | Ph::G) {
                left.ph = Ph::Ng;
            } else if matches!(right.ph, Ph::P | Ph::B) && left.word == right.word {
                left.ph = Ph::M;
            }
        }
    }

    // Two identical consonants across a clitic boundary merge (bez zuba,
    // iz sela, od toga).
    let mut merged: Vec<Phone> = Vec::with_capacity(ph.len());
    for &phone in ph.iter() {
        match merged.last_mut() {
            Some(last)
                if last.ph == phone.ph && phone.ph.is_obstruent() && last.word != phone.word && bound(last, &phone) =>
            {
                *last = Phone { word_start: last.word_start || phone.word_start, ..phone };
            }
            _ => merged.push(phone),
        }
    }
    *ph = merged;
}

pub(super) fn process(frontend: &Frontend, text: &[char], punct: Punctuation, letter_sounds: bool) -> Utterance {
    let lexicon = frontend.lexicon;
    let mut utt = Utterance::default();
    let tokens = tokenize(&cyrillic::to_latin(text), &Words);
    // A name with inner capitals that a lexicon knows as one word stays one
    // word: TikTok and TikToka (the built-in lexicon's Tȉktok), PowerPoint
    // and JavaScript (the English table), or the user's own entry.
    let tokens = join_camel(tokens, syllable_count, |spelled| {
        let mut probe = Word::new(spelled);
        rewrite_foreign(&mut probe);
        let w = &probe.w[..];
        frontend.user_exact.contains_key(w)
            || lexicon.exact.contains_key(w)
            || frontend.user_stem_matches(w)
            || english_word(spelled, english_spelling(spelled)).is_some()
    });
    let mut words = expand_words(tokens, &Words, letter_sounds);
    if words.is_empty() {
        return utt;
    }

    // ---- Per-word analysis: phones and lexical stress ----
    // A command opens its clause, alone or after a word like these ("Ne
    // zaboravi", "I potvrdi", "Molim potvrdi"); see StressRules::form_of.
    const OPENERS: &[&str] = &[
        "ne", "i", "a", "pa", "te", "ili", "ali", "da", "zatim", "onda", "sada", "sad", "molim", "molimo", "odmah",
        "prvo", "nemoj",
    ];
    let mut opens_clause = true;
    for (i, word) in words.iter_mut().enumerate() {
        let at_head = opens_clause;
        opens_clause &= is_one_of(&word.w, OPENERS);
        analyze(frontend, word, i == 0, at_head);
    }
    let count = words.len();
    let spelling: Vec<Vec<char>> = words.iter().map(|word| word.w.clone()).collect();
    let is = |i: usize, list: &[&str]| is_one_of(&spelling[i], list);

    // ---- Clitics and phrase-level accent shifts ----
    for (i, word) in words.iter_mut().enumerate() {
        // Letter names are always full words ("u" in USB is not the
        // preposition, "te" in TV is not the pronoun). A joined compound
        // component likewise retains its independent accent.
        if word.nuclei.is_empty() || word.explicit_stress || word.letter_name || word.joined {
            continue;
        }
        let proclitic = is(i, PROCLITICS);
        let enclitic = is(i, ENCLITICS);
        if proclitic && i + 1 < count {
            word.clitic = true;
        } else if enclitic && i > 0 && !stressed_pronoun(&spelling, i) {
            // Enclitics cannot open a clause; there the same form is a full
            // word ("Ti si...", "Je li..."). They have no length either: the
            // long vowel of the pronouns tȋ, mȋ is not that of the dative
            // clitics ("Kažem ti", "Daj mi").
            word.clitic = true;
            word.shorten();
        } else if proclitic || enclitic || is(i, FUNCTION_WORDS) {
            word.prominence = 1;
        }
        if word.clitic {
            word.prominence = 0;
        }
    }

    // "Novi Sad": the town has the long falling sȃd (Nȍvī Sȃd, Novog Sáda),
    // the adverb the short sȁd. The adverb is the far commoner word and
    // keeps the lexicon's short vowel on its own; only after "novi" is the
    // town meant.
    for (i, next) in words.iter_mut().enumerate().skip(1) {
        if is(i - 1, &["novi", "novog", "novoga", "novom", "novome", "novim"])
            && is(i, &["sad", "sada", "sadu", "sadom"])
            && !next.explicit_stress
            && !next.nuclei.is_empty()
        {
            next.set_long(0, true);
            next.stress = Some(0);
            next.accent = if next.nuclei.len() == 1 { Accent::Falling } else { Accent::Rising };
        }
    }

    for (i, word) in words.iter_mut().enumerate() {
        if is(i, &["boli", "bole"])
            && !word.explicit_stress
            && !word.user_accent
            && word.nuclei.len() == 2
            && boli_is_verb(&spelling, i)
        {
            word.set_long(0, false);
            word.stress = Some(0);
            word.accent = Accent::Rising;
        }
    }

    // After a number the thousand words stand in the genitive plural, whose
    // accent is not the dictionary form's: pet tȉsūćā, sto hȉljādā (tìsuća,
    // hȉljada), pet milijúnā, pet milijárdī. On their own and after one to
    // four (tisuća, tisuću, dvije tisuće) they keep the lexicon's accent.
    for (i, next) in words.iter_mut().enumerate().skip(1) {
        let syllables = next.nuclei.len();
        if next.explicit_stress || syllables < 2 || !is(i - 1, COUNTING_NUMBERS) {
            continue;
        }
        if is(i, &["tisuća", "hiljada"]) {
            next.stress = Some(0);
            next.accent = Accent::Falling;
            for k in 1..syllables {
                next.set_long(k, true);
            }
        } else if is(i, &["milijuna", "miliona", "milijardi", "bilijuna", "biliona"]) {
            next.set_long(syllables - 1, true);
        }
    }

    for i in 1..count {
        let (before, after) = words.split_at_mut(i);
        let (word, next) = (&mut before[i - 1], &mut after[0]);
        if !word.clitic || next.clitic || next.nuclei.is_empty() || next.explicit_stress || next.letter_name {
            continue;
        }
        if is(i - 1, &["ne"]) {
            // "znati" hands its accent over in every person, as in Croatian:
            // nè znam, nè znamo, nè znaju.
            if (next.nuclei.len() == 1 && next.prominence > 0)
                || is(i, &["znamo", "znate", "znaju", "znajući"])
                || is(i, NE_SHIFT_VERBS)
            {
                // The two are then one word, "neznam", in timing as well.
                take_accent(word, next);
                next.joined = true;
                next.prominence = word.prominence;
            }
        }
    }

    ensure_accent(&mut words);
    let word_is_clitic = finish_utterance(&mut words, punct, |w: &[char]| is_one_of(w, QUESTION_WORDS), true, &mut utt);
    assimilate(&mut utt.phones, &word_is_clitic);
    utt
}
