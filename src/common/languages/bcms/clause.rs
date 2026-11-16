//! One clause of Croatian, Serbian or Bosnian text to phones: the lexical
//! stress of every word, the clitics, the accents that depend on the words
//! around a word, and the sound changes between words.

use super::Frontend;
use super::english::{english_entry, english_rule, english_spelling, english_word};
use super::lexicon::Entries;
use super::phonemize::{phonemize, syllable_count};
use super::stress::{StressResult, StressRules};
use super::words::{
    COUNTING_NUMBERS, ENCLITICS, FUNCTION_WORDS, NE_SHIFT_VERBS, PROCLITIC_SHIFT_HOSTS, PROCLITICS, QUESTION_WORDS,
    SHIFTING_PREPOSITIONS, Words,
};
use crate::formant::phones::{Accent, Ph, Phone};
use crate::language::Language;
use crate::prosody::Utterance;
use crate::text::frontend::Word;
use crate::text::frontend::letters::{
    MARK_FALL, MARK_LONG, MARK_RISE, MARK_STRESS, ends_with, ends_with_any, eq, has_inner_syllabic_r, has_vowel,
    is_one_of, is_vowel_letter, starts_with,
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

/// Listener preference: every Croatian tiflo- compound has an accent on
/// tiflo and one on its second component, including established loans.
/// Analyze the components independently, but join their phones into one
/// word so there is no pause or word-initial lengthening at the boundary.
/// Written accents and the user's whole-word/stem/verb entries still win.
fn split_tiflo(frontend: &Frontend, words: Vec<Word>) -> Vec<Word> {
    let mut compounds = Vec::with_capacity(words.len());
    for mut word in words {
        let tail_text = if starts_with(&word.w, "tiflo") { &word.w[5..] } else { &[] };
        let mut split = !tail_text.is_empty()
            && (has_vowel(tail_text) || has_inner_syllabic_r(tail_text))
            && !word.letter_name
            && !word.is_marked()
            && !frontend.user_exact.contains_key(&word.w)
            && !frontend.user_stem_matches(&word.w);
        if split && let Some(verbs) = frontend.user_verbs() {
            let mut probe = Word::new(&word.w);
            phonemize(&mut probe);
            let rules = StressRules::new(&probe, frontend.language, true, Some(verbs));
            split = !rules.verb_form_in(verbs, &mut StressResult::default());
        }
        if split {
            let tail = Word { joined: true, before_hyphen: word.before_hyphen, ..Word::new(tail_text) };
            word.w.truncate(5);
            word.marks.truncate(5);
            word.before_hyphen = false;
            compounds.extend([word, tail]);
        } else {
            compounds.push(word);
        }
    }
    compounds
}

/// The phones and the lexical stress of one word. `first` says the word
/// opens the clause, `at_head` that only words a command may follow stand
/// before it.
fn analyze(frontend: &Frontend, word: &mut Word, first: bool, at_head: bool) {
    let language = frontend.language;
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
        && StressRules::new(word, language, true, user_verbs).verb_form().nucleus >= 0
        && StressRules::new(word, language, false, user_verbs).verb_form().nucleus < 0;

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
            if StressRules::new(word, language, at_head, user_verbs).verb_form_in(verbs, &mut r) {
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
        let native = StressRules::new(word, language, at_head, user_verbs).derivation();
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
        let rules = StressRules::new(word, language, at_head, user_verbs);
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

/// The word before `i` that governs it, past up to two words `modifier`
/// accepts ("o svom novom radu"): its index plus one, 0 when the modifiers
/// reach the head of the clause.
fn before_modifiers(spelling: &[Vec<char>], i: usize, modifier: impl Fn(&[char]) -> bool) -> usize {
    let mut cue = i;
    while cue > 0 && i - cue < 2 && modifier(&spelling[cue - 1]) {
        cue -= 1;
    }
    cue
}

/// Croatian sȃt/rȃd/dȃr/bȏl/glȃs/rȇp/zvȗk/stȃn keep falling accents in D sg, but have rising
/// L sg sátu/rádu/dáru/bólu, and vlȃst, bȏl (f.), strána, vijȇst, čȃr have
/// vlásti, bóli, stráni, vijésti, čári (Školski rječnik, Mrežnik).
/// Recognize a governing locative preposition, allowing up to two
/// adjacent modifiers ("o svom novom radu", "na državnoj vlasti"). This
/// is a local cue, not a general case parser; other contexts keep the
/// exact entry.
/// N/V pl sȃti likewise differs from G pl sátī ("pet sati"). Croatian
/// omits the unstressed final length, as in the rest of its lexicon.
fn croatian_homographs(frontend: &Frontend, words: &mut [Word], spelling: &[Vec<char>]) {
    const LOCATIVE: &[&str] = &["u", "na", "o", "po", "pri"];
    let count = words.len();
    let user_verbs = frontend.user_verbs();
    let fixed = |word: &Word| word.explicit_stress || word.user_accent || word.letter_name;

    // náda (hope) versus nȃda se (hopes). A nearby reflexive selects
    // the verb, unless it introduces another verb ("Nada se nadala",
    // "nada se pojavila"). Capitalized names within a clause stay names.
    for i in 0..count {
        let word = &words[i];
        if !eq(&word.w, "nada")
            || fixed(word)
            || word.nuclei.len() != 2
            || (i > 0 && word.capitalized && !word.all_caps)
        {
            continue;
        }
        let se = |at: Option<usize>| at.and_then(|at| spelling.get(at)).is_some_and(|w| eq(w, "se"));
        let another_verb = se(Some(i + 1))
            && words.get(i + 2).is_some_and(|next| {
                StressRules::new(next, Language::Croatian, false, user_verbs).verb_form().nucleus >= 0
            });
        if se(i.checked_sub(1)) || (se(Some(i + 1)) && !another_verb) {
            words[i].accent = Accent::Falling;
        }
    }
    // vȅseli/vȅsele/vȅselim can also be forms of vesèliti. Keep the
    // adjective default, but recognize an adjacent reflexive/object
    // clitic ("veseli ljudi", "to me veseli", "veselim se").
    for i in 0..count {
        let word = &mut words[i];
        if fixed(word) || !is_one_of(&word.w, &["veseli", "vesele", "veselim"]) {
            continue;
        }
        let verbal_clitic = |w: &[char]| is_one_of(w, &["se", "me", "te", "ga", "nas", "vas", "ih"]);
        // "te vesele žene" has demonstrative te, not an object clitic.
        if (i > 0 && verbal_clitic(&spelling[i - 1]) && !(eq(&spelling[i - 1], "te") && eq(&word.w, "vesele")))
            || spelling.get(i + 1).is_some_and(|next| verbal_clitic(next))
        {
            word.stress = Some(1);
            word.accent = Accent::Rising;
        }
    }

    const GENITIVE_HOURS: &[&str] = &[
        "nula", "koliko", "nekoliko", "toliko", "puno", "mnogo", "malo", "više", "manje", "dosta", "par", "bez", "do",
        "od", "između", "poslije", "prije", "nakon", "oko", "usred", "tijekom", "broj", "raspored",
    ];
    let plural_genitive_cue = |previous: &[char]| {
        is_one_of(previous, COUNTING_NUMBERS)
            || is_one_of(previous, &["koliko", "nekoliko", "toliko", "puno", "mnogo", "malo", "više", "manje", "dosta"])
            || (previous.len() > 2 && ends_with(previous, "ih"))
    };
    for i in 1..count {
        let word = &mut words[i];
        if fixed(word) || word.nuclei.is_empty() {
            continue;
        }
        let previous = &spelling[i - 1][..];
        let is = |list: &[&str]| is_one_of(&spelling[i], list);
        // The word that governs this one, past up to two modifiers
        let governed = |modifier: &dyn Fn(&[char]) -> bool| {
            let cue = before_modifiers(spelling, i, modifier);
            cue.checked_sub(1).map(|at| &spelling[at][..])
        };
        let shorten_first = |word: &mut Word| word.set_long(0, false);
        // zrȃka (air, G) / zrȁka (ray, N), and their homographic
        // -u/-om forms. Only clear feminine modifiers select ray;
        // arbitrary -a endings would also catch verbs such as nema.
        if is(&["zraka", "zraku", "zrakom"]) {
            const STEMS: [&str; 19] = [
                "jedn",
                "jedin",
                "ov",
                "t",
                "on",
                "nek",
                "svak",
                "nijedn",
                "moj",
                "tvoj",
                "svoj",
                "naš",
                "vaš",
                "sunčev",
                "svjetlosn",
                "lasersk",
                "rendgensk",
                "infracrven",
                "ultraljubičast",
            ];
            let ending = &spelling[i][4..];
            let ray = STEMS.iter().any(|stem| {
                previous.len() == stem.chars().count() + ending.len()
                    && starts_with(previous, stem)
                    && previous.ends_with(ending)
            });
            if ray {
                word.stress = Some(0);
                word.accent = Accent::Falling;
                shorten_first(word);
                continue;
            }
        }
        let mut rising = false;
        if is(&[
            "satu", "radu", "daru", "vlasti", "bolu", "boli", "strani", "vijesti", "čari", "glasu", "repu", "zvuku",
            "stanu", "stvari", "padu", "danu", "viru", "vragu", "mraku", "zraku",
        ]) {
            // "na boli", "na vijesti", "u vijesti" are the accusative
            // (žaliti se na bȏli, reagirati na vijȇsti, ući u vijȇsti)
            rising =
                governed(&|w| ends_with_any(w, &["om", "ome", "omu", "em", "emu", "oj"])).is_some_and(|preposition| {
                    is_one_of(preposition, LOCATIVE)
                        && !(is(&["boli"]) && eq(preposition, "na"))
                        && !(is(&["vijesti", "stvari"]) && is_one_of(preposition, &["na", "u"]))
                });
            // pet/mnogo stvari is G plural stvárī; bez stvari alone
            // is ambiguous with G singular, so do not infer from it.
            if is(&["stvari"]) {
                rising = rising || plural_genitive_cue(previous);
            }
        } else if is(&["noći"]) {
            // ŠR: G sg / N/A pl nȍći, L sg and G pl nòći. Unlike
            // sátu and víru, the rising vowel remains SHORT.
            if governed(&|w| ends_with(w, "oj")).is_some_and(|w| is_one_of(w, LOCATIVE))
                || plural_genitive_cue(previous)
            {
                word.stress = Some(0);
                word.accent = Accent::Rising;
                shorten_first(word);
            }
        } else if is(&["dana"]) {
            // dȃna (one day) versus dánā (several days). Prepositions
            // alone do not disambiguate: "od dana" stays singular.
            rising = plural_genitive_cue(previous) || is_one_of(previous, &["nula", "tjedan", "mjesec", "godinu"]);
        } else if is(&["duše"]) {
            // G singular dúšē versus N/A/V plural dȗše (Mrežnik).
            // Only use a governing genitive preposition, optionally
            // across two modifiers, or the fixed phrase mirne duše.
            const GENITIVE_SOUL: &[&str] =
                &["bez", "do", "od", "iz", "s", "sa", "zbog", "radi", "poput", "unutar", "izvan"];
            rising = governed(&|w| !is_one_of(w, GENITIVE_SOUL) && ends_with(w, "e"))
                .is_some_and(|w| is_one_of(w, GENITIVE_SOUL))
                || eq(previous, "mirne");
        } else if is(&["zaštita"]) && plural_genitive_cue(previous) {
            word.accent = Accent::Falling; // ŠR G plural zȁštītā
        } else if is(&["događaju", "osjećaju"]) {
            // Present dogáđajū / òsjećajū versus D/L of the nouns
            // dȍgađāj / ȍsjećāj. A governing preposition, optionally
            // separated by two agreeing modifiers, selects the noun:
            // "o novom događaju se govori", but "događaju se stvari".
            // An -om ending alone is not enough: adverbs such as
            // "potom" and "uglavnom" also occur before these verbs.
            let noun = is_one_of(
                previous,
                &["tom", "tome", "tomu", "ovom", "ovome", "onom", "onome", "nekom", "svakom", "takvom"],
            ) || governed(&|w| ends_with_any(w, &["om", "ome", "omu", "em", "emu"]))
                .is_some_and(|w| is_one_of(w, LOCATIVE) || is_one_of(w, &["k", "ka", "prema", "unatoč", "usprkos"]));
            if noun {
                word.shorten();
                word.stress = Some(0);
                word.accent = Accent::Falling;
            }
        } else if is(&["pada", "nebesa"]) {
            // G pȃda / nebésā after a genitive preposition, versus
            // the verb pȁdā and N/A nebèsa. Skip agreeing modifiers.
            const GENITIVE: &[&str] = &[
                "bez", "do", "od", "iz", "s", "sa", "zbog", "nakon", "prije", "poslije", "uslijed", "protiv", "tijekom",
            ];
            if governed(&|w| !is_one_of(w, GENITIVE) && ends_with_any(w, &["og", "oga", "eg", "ega", "ih"]))
                .is_some_and(|w| is_one_of(w, GENITIVE))
                && let Some(stress) = word.stress
            {
                word.set_long(stress, true);
            }
        } else if is(&["ministarstva"]) && plural_genitive_cue(previous) {
            // ŠR G pl minìstārstvā, versus G sg / N/A pl ministárstva.
            word.shorten();
            word.stress = Some(1);
            word.accent = Accent::Rising;
        } else if is(&[
            "žirafa",
            "pažnja",
            "nabava",
            "društva",
            "vuka",
            "vraga",
            "komponenta",
            "zvuka",
            "snimka",
            "komoda",
            "rastava",
            "stabla",
            "anketa",
            "vještina",
        ]) && plural_genitive_cue(previous)
        {
            // Homographic G plurals: žiráfā, pážnjā, nȁbāvā,
            // drúštvā, vúkā, vrágā, kompònēntā, zvúkā, snȋmkā, komódā and
            // rȁstāvā, stáblā, ankétā, vještínā (Školski rječnik).
            let stress = usize::from(is(&["žirafa", "komponenta", "komoda", "anketa", "vještina"]));
            word.stress = Some(stress);
            word.accent = if is(&["nabava", "snimka", "rastava"]) { Accent::Falling } else { Accent::Rising };
            word.set_long(stress, !is(&["nabava", "komponenta", "rastava"]));
        } else if is(&["komponenti"]) {
            // The isolated/G plural kompònentī contrasts with singular
            // DL komponènti. A singular modifier or a nearby governing
            // preposition identifies the latter without a global parser.
            if governed(&|w| ends_with(w, "oj"))
                .is_some_and(|w| is_one_of(w, LOCATIVE) || is_one_of(w, &["k", "ka", "prema"]))
                || ends_with(previous, "oj")
            {
                word.stress = Some(2);
                word.accent = Accent::Rising;
            }
        } else if is(&["sati"]) {
            rising = is_one_of(previous, COUNTING_NUMBERS)
                || is_one_of(previous, GENITIVE_HOURS)
                || (previous.len() > 2 && ends_with(previous, "ih"))
                // The frequent question "koliko je sati" has the same G.
                || (i >= 2 && eq(previous, "je") && eq(&spelling[i - 2], "koliko"));
        }
        if rising {
            word.stress = Some(0);
            word.accent = Accent::Rising;
            word.set_long(0, true);
        }
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
    let language = frontend.language;
    let croatian = language == Language::Croatian;
    let lexicon = frontend.lexicon;
    let mut utt = Utterance::default();
    let tokens = tokenize(&cyrillic::to_latin(text), &Words(language));
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
            // TifloNova and tiflonova share the component boundary below.
            || (croatian && starts_with(w, "tiflo"))
            || english_word(spelled, english_spelling(spelled)).is_some()
    });
    let mut words = expand_words(tokens, &Words(language), letter_sounds);
    if croatian {
        words = split_tiflo(frontend, words);
    }
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

    if croatian {
        croatian_homographs(frontend, &mut words, &spelling);
    }

    for i in 1..count {
        let (before, after) = words.split_at_mut(i);
        let (word, next) = (&mut before[i - 1], &mut after[0]);
        if !word.clitic || next.clitic || next.nuclei.is_empty() || next.explicit_stress || next.letter_name {
            continue;
        }
        if is(i - 1, &["ne"]) {
            // "znati" hands its accent over in every person and in all three
            // languages: nè znam, nè znamo, nè znaju.
            if (next.nuclei.len() == 1 && next.prominence > 0)
                || is(i, &["znamo", "znate", "znaju", "znajući"])
                || (!croatian && is(i, NE_SHIFT_VERBS))
            {
                // The two are then one word, "neznam", in timing as well.
                take_accent(word, next);
                next.joined = true;
                next.prominence = word.prominence;
                if croatian {
                    // The length the dictionaries keep after the accent
                    // (nè znām) is not heard in Croatian as commonly spoken.
                    next.shorten();
                }
            }
        } else if language == Language::Bosnian
            && word.nuclei.len() == 1
            && is(i - 1, SHIFTING_PREPOSITIONS)
            && is(i, PROCLITIC_SHIFT_HOSTS)
        {
            take_accent(word, next);
        }
    }

    ensure_accent(&mut words);
    let word_is_clitic = finish_utterance(&mut words, punct, |w: &[char]| is_one_of(w, QUESTION_WORDS), true, &mut utt);
    assimilate(&mut utt.phones, &word_is_clitic);
    utt
}
