//! The Slovenian clause: the shared tokenizer and clause structure, with the
//! letter-to-sound rules, the lexicon, the clitics and the sound changes of
//! this module. Every word has a plain (neutral) accent: Slovenian stress is
//! dynamic, with long and short stressed vowels and no tones.

use super::lexicon::{Lexicon, builtin_lexicon};
use super::marks::{BEFORE_COMPLEMENT_MARK, IN_EXCLAMATION_MARK, IN_QUESTION_MARK, REQUEST_MARK, VOCATIVE_MARK};
use super::phonemize::{Capital, assimilate, phonemize};
use super::readings::{self, Clause, VerbOrNoun};
use super::words;
use crate::formant::phones::{Accent, Ph, Phone};
use crate::prosody::Utterance;
use crate::text::frontend::letters::{MARK_STRESS, chars, ends_with, eq, is_one_of, vowel_phone};
use crate::text::frontend::tokenizer::{expand_words, join_camel, phonemize_letter_sound, rewrite_foreign, tokenize};
use crate::text::frontend::utterance::{self, ensure_accent, finish_utterance};
use crate::text::frontend::{LanguageWords, Word};
use crate::text::{Punctuation, cyrillic};

/// What the shared tokenizer asks of Slovenian.
pub struct Slovenian;

impl LanguageWords for Slovenian {
    fn symbol_name(&self, c: char) -> Option<&'static str> {
        words::symbol_name(c)
    }

    fn letter_name(&self, letter: &[char]) -> &'static str {
        words::letter_name(letter)
    }

    fn is_proclitic(&self, w: &[char]) -> bool {
        words::is_proclitic(w)
    }

    fn is_enclitic(&self, w: &[char]) -> bool {
        words::is_enclitic(w)
    }

    /// Slovenian has v, s, z, k and h (Pravopis: v šoli, s hiše, z oreha,
    /// k bratu, h godu).
    fn is_consonant_preposition(&self, w: &[char]) -> bool {
        is_one_of(w, &["v", "s", "z", "k", "h"])
    }

    /// Zaupam v vas. Pridi k nam. V nas je upanje. The pronouns nas, vas,
    /// nam, vam after a preposition are the full forms (SP 2001 vàs, nàs),
    /// spelled like the clitics; the preposition was read as a letter's name
    /// before them (v vas was "vé vas").
    fn is_pronoun_after_preposition(&self, w: &[char]) -> bool {
        is_one_of(w, &["nas", "vas", "nam", "vam"])
    }

    /// The list is the one Croatian, Serbian and Bosnian use: the C++ engine
    /// had one for all four languages.
    fn is_short_word(&self, w: &[char]) -> bool {
        crate::text::frontend::tokenizer::is_short_vc_word(w)
    }
}

/// An unstressed word: short, and its e and o open (od [ɔt], je [jɛ]).
fn reduce(phones: &mut [Phone]) {
    for phone in phones {
        phone.is_long = false;
        phone.ph = match phone.ph {
            Ph::Ee => Ph::Eh,
            Ph::Oo => Ph::Oh,
            ph => ph,
        };
    }
}

/// The sound of one letter alone, for spelling by sounds. The Slovenian
/// letters e and o are named é and ó, with the close vowel (Slovenski
/// pravopis 2001, § 7).
pub fn letter_sound(letter: &[char]) -> Utterance {
    utterance::letter_sound(letter, |c| match c {
        'e' => Ph::Ee,
        'o' => Ph::Oo,
        _ => vowel_phone(c),
    })
}

/// One clause of Slovenian text to phones. `user` has the user's accent
/// entries for Slovenian, which win over the built-in lexicon.
pub fn process(marked_text: &[char], punct: Punctuation, letter_sounds: bool, user: Option<&Lexicon>) -> Utterance {
    let mut utt = Utterance { slovenian: true, ..Utterance::default() };
    // The engine's marks: a clause opening with da, naj, kako ... follows; the
    // sentence ends in a question mark or an exclamation mark; the clause is
    // an address (Pero, kje si?)
    let mut text = marked_text;
    let mut before_complement = false;
    let mut question = punct == Punctuation::Question;
    let mut exclamation = punct == Punctuation::Exclamation;
    let mut vocative = false;
    let mut request = false;
    while let [rest @ .., last] = text {
        match *last {
            REQUEST_MARK => request = true,
            BEFORE_COMPLEMENT_MARK => before_complement = true,
            IN_QUESTION_MARK => question = true,
            IN_EXCLAMATION_MARK => exclamation = true,
            VOCATIVE_MARK => vocative = true,
            _ => break,
        }
        text = rest;
    }
    let user_has = |w: &[char]| user.is_some_and(|user| user.find_exact(w).is_some());
    let tokens = tokenize(&cyrillic::to_latin(text), &Slovenian);
    // The syllables of the parts are counted as the C++ engine counted them
    // for every language, by the letter-to-sound rules of Croatian
    let tokens = join_camel(tokens, crate::text::frontend::syllables::syllable_count, |spelled| {
        let mut probe = Word::new(spelled);
        rewrite_foreign(&mut probe);
        user_has(&probe.w) || builtin_lexicon().find_exact(&probe.w).is_some()
    });
    let expanded = expand_words(tokens, &Slovenian, letter_sounds);
    if expanded.is_empty() {
        return utt;
    }

    // A compound numeral is read as its parts, ena in dvajset, with an
    // accent on the ones and on the tens (SP 2001: ênaindvájset,
    // pétindvájset) and no pause between them; the "in" is a clitic. Read
    // as one word with one accent the ones were swallowed ("enaindvajset"
    // was heard as "dvajset" with a mumble before it). An accent written
    // in the text and an entry of the user's for the whole word win.
    let mut words: Vec<Word> = Vec::with_capacity(expanded.len());
    for word in expanded {
        let parts = (!word.letter_name && !word.is_marked() && !user_has(&word.w))
            .then(|| readings::split_compound_numeral(&word.w))
            .flatten();
        let Some((ones, tens)) = parts else {
            words.push(word);
            continue;
        };
        words.push(Word {
            capitalized: word.capitalized,
            all_caps: word.all_caps,
            break_before: word.break_before, // 12:21, dvanajst | ena in dvajset
            item_break: word.item_break,
            ..Word::new(&ones)
        });
        words.push(Word { joined: true, clitic: true, prominence: 0, ..Word::new(&['i', 'n']) });
        words.push(Word { joined: true, before_hyphen: word.before_hyphen, ..Word::new(&tens) });
    }

    // ---- Per-word analysis: phones and stress ----
    // The first word of the clause, and one with only conjunctions and
    // adverbs that open a request before it, is the greeting or the
    // imperative where the spelling has two readings (Živijo!, Naredi to,
    // Zdaj naredi to, Nemudoma posodobi to aplikacijo!); elsewhere it is the
    // verb's present (oni živijo, program naredi, to naj se takoj izvrši).
    // A spelling that is a verb's form or another word (veselí/veséli,
    // letí/léti) is read by the words around it (context_reading()).
    const BEFORE_IMPERATIVE: &[&str] = &[
        "in", "pa", "ter", "zdaj", "sedaj", "potem", "nato", "prosim", "samo", "le", "raje", "kar", "no", "daj",
        "takoj", "nemudoma", "najprej",
    ];
    // A capitalized dative pronoun opening the sentence says whom the
    // command is for (Meni podari ljubezen, Nam pokaži; a listener's report
    // of 2026-11-03); not the accusatives (Nas deli samo meja is the present
    // déli) and not ti (Ti deli so ...). In SUK 1.1 no such pronoun at a
    // clause head stands before a verb of the class.
    const HEAD_PRONOUNS: &[&str] = &["meni", "tebi", "njemu", "njej", "nam", "vam", "njim", "mi"];
    // The words before the first that is not one of them
    let mut head_words =
        usize::from(words[0].capitalized && !words[0].all_caps && is_one_of(&words[0].w, HEAD_PRONOUNS));
    while words.get(head_words).is_some_and(|word| is_one_of(&word.w, BEFORE_IMPERATIVE)) {
        head_words += 1;
    }
    let count = words.len();
    let mut spellings: Vec<Vec<char>> = Vec::with_capacity(count);
    let mut capitalized = Vec::with_capacity(count);
    // Written all in capitals (JAWS) or a letter of a spelled one (NVDA)
    let mut acronym = Vec::with_capacity(count);
    // Spelled the foreign way (Wayne, Carlyle)
    let mut rewritten = Vec::with_capacity(count);
    for word in &mut words {
        let written = word.w.clone();
        rewrite_foreign(word);
        rewritten.push(word.w != written);
        spellings.push(word.w.clone());
        // In text written all in capitals nothing is a name; a capital
        // letter alone is written so in any text
        capitalized.push(word.capitalized && (!word.all_caps || word.w.len() == 1));
        acronym.push(word.letter_name || (word.all_caps && word.w.len() > 1));
    }
    let clause = Clause {
        words: &spellings,
        capitalized: &capitalized,
        acronym: &acronym,
        before_complement,
        question,
        exclamation,
        request,
    };
    // A function word's spelling read as a content word: sámo, the name or
    // "alone" (Samo mi je povedal: the mi after it is the clitic)
    let mut content_reading = vec![false; count];
    for i in 0..count {
        // The head of a clause, where a capital says nothing of a name, for
        // the tables of names: not an address (Pero, kje si?), and not a
        // given name before another capitalized word, its surname (Pero
        // Novak je prišel); samo is "only" there (Samo Janez je prišel)
        let before_surname =
            words.get(i + 1).is_some_and(|next| capitalized[i + 1] && !next.all_caps && !next.letter_name)
                && !eq(&words[i].w, "samo")
                && readings::is_given_name(&words[i].w);
        let word = &mut words[i];
        word.explicit_stress = word.marks.iter().any(|&mark| mark & MARK_STRESS != 0);
        let at_head = i == 0 || word.after_opening;
        let clause_head = at_head && !vocative && !before_surname;
        let mut reading: Option<Vec<char>> = None;
        if !word.letter_name && !word.joined {
            let head = readings::head_reading(&word.w);
            let in_clause = readings::clause_reading(&clause, i);
            // A name spelled like a common word (Múrska Sóbota, sobóta;
            // Jêlena, jeléna): only with a capital, not in text written all
            // in capitals, and at the head of a clause only when the name
            // is the commoner word (name_reading()), unless the clause is an
            // address (Pero, kje si?) or a capitalized word follows, a
            // surname (Pero Novak je prišel)
            let name = (word.capitalized && !word.all_caps)
                .then(|| readings::name_reading(&word.w, clause_head))
                .flatten()
                .or_else(|| {
                    (capitalized[i] && i > 0 && !rewritten[i] && !rewritten[i - 1])
                        .then(|| readings::surname_reading(&word.w, &spellings[i - 1], capitalized[i - 1]))
                        .flatten()
                });
            // Govori, da bo prišel: the verb of saying before its clause
            // of da is the present
            let command_head = i == 0 || i == head_words;
            let next_is = |list: &[&str]| spellings.get(i + 1).is_some_and(|next| is_one_of(next, list));
            // Not before a plural auxiliary or a genitive plural, where the
            // spelling is a noun's plural (Odgovori so bili taki. Deli so
            // razstavljeni. Izgovori odgovornih ne opravičujejo.), and a
            // verb of saying not before se, the impersonal present (Govori
            // se o tem, da ...; SUK 1.1: 4 of 4), unless the sentence
            // exclaims
            let head = head.filter(|head| {
                command_head
                    && !(head.speech && before_complement && i + 1 == count)
                    && !next_is(&["so", "niso", "bodo"])
                    && !spellings.get(i + 1).is_some_and(|next| next.len() > 4 && ends_with(next, "ih"))
                    && !(head.speech && next_is(&["se"]) && !exclamation)
            });
            // A request: prosim before the verb (a head word already), after
            // it, or alone between commas next to the clause. Any verb is the
            // imperative there (Izgovori prosim pravilno!)
            let asked =
                command_head && (request || next_is(&["prosim"]) || spellings[..i].iter().any(|x| eq(x, "prosim")));
            // Pogovori se z njim. Dogovori se z njo! pogovoriti se and
            // dogovoriti se at the head of a clause, where the spelling is
            // also the plural of pogovor, dogovor: the command, unless a
            // plural verb follows the se, which is then the noun's (Pogovori
            // se bodo nadaljevali. Pogovori se še niso začeli. Pogovori se
            // nadaljujejo.). Only these: of the other verb/noun spellings
            // before se the head of a sentence of SUK 1.1 is the present or
            // another word (Zgodi se, Poleti se sprostim, Govori se o tem)
            let plural_verb = |x: &Vec<char>| {
                is_one_of(x, &["so", "niso", "bodo", "bi", "ne"])
                    || (x.len() > 5 && (ends_with(x, "jo") || ends_with(x, "do")))
            };
            let reflexive = command_head
                && next_is(&["se"])
                && is_one_of(&word.w, &["pogovori", "dogovori"])
                && !spellings.iter().skip(i + 2).take(2).any(plural_verb);
            let commanded =
                if (asked || reflexive) && head.is_none() { readings::imperative_reading(&word.w) } else { None };
            if name.is_some() {
                reading = name;
            } else if let Some(head) = head {
                reading = Some(head.pron);
            } else if commanded.is_some() {
                reading = commanded;
            } else if let Some(in_clause) = in_clause {
                reading = Some(chars(in_clause));
                // The adjective láhko has the full accent of a content word,
                // not the small one of the modal lahkó
                if readings::clause_reading_is_content(&word.w, in_clause) {
                    word.explicit_stress = true;
                    content_reading[i] = true;
                }
            } else if let Some(readings) = readings::verb_noun_readings(&word.w) {
                reading = match readings::context_reading(&clause, i, readings.participle, readings.noun_percent) {
                    VerbOrNoun::Verb => Some(readings.verb),
                    VerbOrNoun::Nominal => Some(readings.nominal),
                    VerbOrNoun::Lexicon => None,
                }
                .filter(|pron| !pron.is_empty());
            }
        }
        word.accent = Accent::Neutral;
        if word.letter_sound {
            phonemize_letter_sound(word);
            continue;
        }
        let capital = match (word.capitalized, word.all_caps) {
            (false, _) => Capital::None,
            (true, true) => Capital::AllCaps,
            (true, false) if at_head => Capital::Head,
            (true, false) => Capital::Inside,
        };
        let phones = phonemize(&word.w, &word.marks, user, word.letter_name, reading.as_deref(), capital);
        word.phones = phones.phones;
        word.letter = phones.letter;
        word.nuclei = phones.nuclei;
        word.stress = phones.stress;
        if word.nuclei.is_empty() {
            word.clitic = true;
            word.prominence = 0;
        }
        if word.clitic {
            // The "in" of a compound numeral: unstressed, short, open
            reduce(&mut word.phones);
        }
    }

    // The preposition v: [w] before a vowel (v igri), the vowel [u] before a
    // consonant (v šoli [u šoli]: Slovenski pravopis 2001, § 643-645),
    // which is a syllable, never stressed
    for i in 0..count.saturating_sub(1) {
        let Some(next) = words[i + 1].phones.first().map(|phone| phone.ph) else { continue };
        let word = &mut words[i];
        if eq(&word.w, "v")
            && !word.letter_name
            && !next.is_vowel()
            && let [only] = word.phones.as_mut_slice()
        {
            only.ph = Ph::U;
            only.nucleus = true;
            word.nuclei = vec![0];
            word.stress = Some(0);
        }
    }

    // An acronym spelled letter by letter has one stress, on its last
    // letter (SP [espé], RTV [ertevé]: Slovenski pravopis 2001, § 1022):
    // the names before it lean on it; letters spoken by their sounds keep
    // a stress each, and so do the letters of a web address's prefix (https). Two
    // abbreviations written apart are two (USB PDF), the letters of s.a.r.s. one.
    let named = |word: &Word| word.letter_name && !word.letter_sound && !word.own_accent;
    let mut i = 0;
    while i < count {
        if !named(&words[i]) {
            i += 1;
            continue;
        }
        let mut end = i + 1;
        while end < count && named(&words[end]) && !words[end].word_start {
            end += 1;
        }
        for k in i..end - 1 {
            if words[k].nuclei.is_empty() {
                continue;
            }
            words[k].clitic = true;
            words[k].prominence = 0;
            for phone in &mut words[k].phones {
                phone.is_long = false;
            }
            words[k + 1].joined = true;
        }
        i = end;
    }

    // ---- Clitics ----
    // v vas, k nam, za vas: the full pronoun after a preposition
    let after_preposition: Vec<bool> = (0..count)
        .map(|k| {
            k > 0 && readings::is_preposition(&words[k - 1].w) && is_one_of(&words[k].w, &["nas", "vas", "nam", "vam"])
        })
        .collect();
    for i in 0..count {
        // ti and mi are the dative clitics (Dam ti, Reci mi) or the pronouns
        // tí, mí: those at the head or the end of a clause, or after a
        // conjunction ("in ti", "pa mi", "tudi ti")
        const BEFORE_PRONOUN: &[&str] = &["in", "a", "pa", "ali", "tudi", "samo", "le", "niti", "ne", "kot", "kakor"];
        let pronoun = is_one_of(&words[i].w, &["ti", "mi"])
            && (i == 0 || i + 1 == count || (is_one_of(&words[i - 1].w, BEFORE_PRONOUN) && !content_reading[i - 1]));
        let word = &mut words[i];
        if word.nuclei.is_empty() || word.explicit_stress || word.letter_name || word.joined {
            continue;
        }
        let proclitic = words::is_proclitic(&word.w);
        let after_preposition = after_preposition[i];
        let enclitic = words::is_enclitic(&word.w) && !pronoun && !after_preposition;
        if (proclitic && i + 1 < count) || (enclitic && i > 0) {
            word.clitic = true;
        } else if proclitic || enclitic || words::is_function_word(&word.w) {
            word.prominence = 1;
        }
        if word.clitic {
            word.prominence = 0;
            reduce(&mut word.phones);
        }
    }

    ensure_accent(&mut words);
    let word_is_clitic = finish_utterance(&mut words, punct, words::is_question_word, false, &mut utt);
    assimilate(&mut utt.phones, &word_is_clitic);
    utt
}
