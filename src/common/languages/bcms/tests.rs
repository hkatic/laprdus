//! Direct front-end tests for accents, compound stress and word-boundary
//! metadata of Croatian, Serbian and Bosnian.

use std::sync::Arc;

use super::Frontend;
use crate::formant::phones::Accent::{self, Falling as F, Rising as R};
use crate::formant::phones::{Ph, Phone, needs_vowel_separation};
use crate::language::Language;
use crate::prosody::{ClauseKind, Utterance, split_at_breaks};
use crate::text::frontend::UserLexicon;
use crate::text::{Frontend as _, Punctuation};

fn chars(text: &str) -> Vec<char> {
    text.chars().collect()
}

/// A clause read with letter names, the default.
fn read(frontend: &Frontend, text: &str) -> Utterance {
    frontend.process(&chars(text), Punctuation::Period, false)
}

/// The user's accent lexicon from its JSON.
fn user(json: &str) -> Option<Arc<UserLexicon>> {
    Some(UserLexicon::parse(json).0)
}

fn stressed_syllables(utterance: &Utterance) -> Vec<usize> {
    let stressed = utterance.phones.iter().filter(|phone| phone.nucleus && phone.stressed);
    stressed.filter_map(|phone| usize::try_from(phone.syllable).ok()).collect()
}

fn nuclei(utterance: &Utterance) -> Vec<Phone> {
    utterance.phones.iter().filter(|phone| phone.nucleus).copied().collect()
}

/// The nuclei of one word of the clause.
fn word_nuclei(utterance: &Utterance, word: u16) -> Vec<Phone> {
    utterance.phones.iter().filter(|phone| phone.nucleus && phone.word == word).copied().collect()
}

fn stressed_count(vowels: &[Phone]) -> usize {
    vowels.iter().filter(|phone| phone.stressed).count()
}

fn word_starts(utterance: &Utterance) -> usize {
    utterance.phones.iter().filter(|phone| phone.word_start).count()
}

fn last_word(utterance: &Utterance) -> u16 {
    utterance.phones.last().expect("the clause has phones").word
}

fn require_one_word(utterance: &Utterance, context: &str) {
    assert!(!utterance.phones.is_empty(), "{context}");
    assert!(utterance.phones[0].word_start, "{context}");
    assert_eq!(word_starts(utterance), 1, "{context}");
    for phone in &utterance.phones {
        assert_eq!(phone.word, 0, "{context}");
        assert_eq!(i32::from(phone.word_syllables), utterance.syllable_count, "{context}");
        assert_ne!(phone.ph, Ph::Sil, "{context}");
    }
}

/// The phones of a clause as text: words apart, ' before a stressed nucleus,
/// : after a long one.
fn phones_of(utterance: &Utterance) -> String {
    const NAMES: [&str; Ph::COUNT] = [
        "_", "a", "e", "i", "o", "u", "@", "E", "O", "é", "ó", "ə", "w", "j", "v", "l", "lj", "m", "n", "nj", "ŋ", "r",
        "p", "b", "t", "d", "k", "g", "c", "dz", "č", "dž", "ć", "đ", "f", "s", "z", "š", "ž", "ś", "ź", "h", "ɣ",
    ];
    let mut out = String::new();
    let mut word = None;
    for phone in &utterance.phones {
        if word != Some(phone.word) {
            if word.is_some() {
                out.push(' ');
            }
            word = Some(phone.word);
        }
        if phone.nucleus && phone.stressed {
            out.push('\'');
        }
        out.push_str(NAMES[phone.ph as usize]);
        if phone.nucleus && phone.is_long {
            out.push(':');
        }
    }
    out
}

/// Kolica to Dragojevic keep sourced accents in their ordinary forms
#[test]
fn kolica_to_dragojevic_keep_sourced_accents_in_their_ordinary_forms() {
    #[rustfmt::skip]
    let paradigms: &[(&[&str], usize, Accent, bool)] = &[
        (&["kolica", "kolicima", "Kolica", "KOLICA"], 1, R, true),
        (&["dubinsku", "dubinski", "dubinska", "dubinsko", "dubinskoga",
          "dubinskoj", "dubinskih", "dubinskima"], 0, R, false),
        (&["pogledati", "pogledat", "pogledam", "pogledamo", "pogledaju",
          "pogledaj", "pogledajte", "pogledao", "pogledala", "pogledali",
          "pogledah", "pogledavši", "pogledan"], 0, R, false),
        (&["doći", "dođem", "dođeš", "dođe", "dođemo", "dođete", "dođu"], 0, F, true),
        (&["dođi", "dođimo", "dođite", "dođoh", "dođosmo", "dođoste", "dođoše"], 0, R, true),
        (&["došao", "došla", "došlo", "došli", "došle", "došavši"], 0, R, false),
        (&["tiskara", "tiskare", "tiskari", "tiskaru", "tiskarom", "tiskarama"], 0, F, false),
        (&["podizanje", "podizanja", "podizanju", "podizanjem", "podizanjima",
          "podizati", "podižem", "podižu", "podiži", "podizao", "podizan"], 0, R, false),
        (&["element", "elementa", "elementu", "elementom", "elementi",
          "elemenata", "elementima"], 1, R, false),
        (&["stanje", "stanja", "stanju", "stanjem", "stanjima"], 0, R, true),
        (&["raspoloživ", "raspoloživa", "raspoloživo", "raspoloživu",
          "raspoloživi", "raspoloživoga", "raspoloživima"], 2, R, false),
        (&["raspoloživiji", "raspoloživijima"], 3, R, false),
        (&["Oliver", "Olivera", "Oliveru", "Oliverom"], 0, R, false),
        (&["Dragojević", "Dragojevića", "Dragojeviću", "Dragojevićem"], 0, F, false),
    ];
    let mut frontend = Frontend::new(Language::Croatian);
    for &(forms, stress, tone, long_vowel) in paradigms {
        for &text in forms {
            let utterance = read(&frontend, text);
            assert_eq!(stressed_syllables(&utterance), [stress], "{text}");
            let vowels = nuclei(&utterance);
            assert_eq!(vowels[stress].accent, tone, "{text}");
            assert_eq!(vowels[stress].is_long, long_vowel, "{text}");
            for (i, vowel) in vowels.iter().enumerate() {
                assert!(i == stress || !vowel.is_long, "{text}");
            }
            require_one_word(&utterance, text);
        }
    }
    // A name must work both at clause head and after another word.
    for (text, surname) in [("Oliver Dragojević", 3), ("Olivera Dragojevića", 4)] {
        let utterance = read(&frontend, text);
        let vowels = nuclei(&utterance);
        assert_eq!(stressed_syllables(&utterance), [0, surname], "{text}");
        assert_eq!(vowels[0].accent, R, "{text}");
        assert_eq!(vowels[surname].accent, F, "{text}");
    }
    // Exact noun/adjective forms leave derivatives to their own rules.
    assert_eq!(stressed_syllables(&read(&frontend, "elementaran")), [3]);
    assert_eq!(stressed_syllables(&read(&frontend, "raspoložiti")), [2]);
    // Ambiguous aspects and surname variants remain explicitly selectable.
    assert_eq!(stressed_syllables(&read(&frontend, "poglédati")), [1]);
    assert!(nuclei(&read(&frontend, "poglédati"))[1].is_long);
    assert_eq!(stressed_syllables(&read(&frontend, "Dragójević")), [1]);
    frontend.set_user_lexicon(user(r#"{"entries":[{"word":"drag/o:jević"},{"verb":"pogl/e:d=a"}]}"#));
    assert_eq!(stressed_syllables(&read(&frontend, "Dragojević")), [1]);
    assert_eq!(stressed_syllables(&read(&frontend, "pogledati")), [1]);
}

/// Collected Croatian words retain sourced accents through inflection
#[test]
fn collected_croatian_words_retain_sourced_accents_through_inflection() {
    #[rustfmt::skip]
    let paradigms: &[(&[&str], usize, Accent, bool)] = &[
        (&["ispuna", "ispune", "ispunu", "ispuno", "ispunom", "ispunama",
          "Ispuna", "ISPUNA"], 0, F, false),
        (&["hrvat", "Hrvat", "HRVAT"], 0, R, false),
        (&["hrvata", "Hrvata", "HRVATA", "Hrvatu", "Hrvatom", "Hrvati",
          "Hrvate", "Hrvatima"], 1, R, true),
        (&["vatrogasac", "vatrogasca", "vatrogascu", "vatrogascem", "vatrogasci",
          "vatrogasce", "vatrogascima", "Vatrogasci", "VATROGASCI"], 2, R, true),
        (&["vatrogasaca"], 1, R, false),
        (&["vatrogašče"], 0, F, false),
        (&["muškarac", "muškarca", "muškarcu", "muškarcem", "muškarci",
          "muškarce", "muškarcima", "Muškarci", "MUŠKARCI"], 1, R, true),
        (&["muškaraca"], 0, R, false),
        (&["muškarče"], 0, F, false),
        (&["policajac", "policajca", "policajcu", "policajcem", "policajci",
          "policajce", "policajcima", "Policajci", "POLICAJCI"], 2, R, true),
        (&["policajaca"], 1, R, false),
        (&["policajče"], 0, F, false),
        (&["nisam", "nisi", "nije", "nismo", "niste", "nisu", "Nisi", "NISI"], 0, R, true),
        // Listener's long i; case forms are an extension of that preference.
        (&["kafanica", "kafanice", "kafanici", "kafanicu", "kafanico",
          "kafanicom", "kafanicama", "Kafanica", "KAFANICA"], 2, R, true),
        (&["laganica", "laganice", "laganici", "laganicu", "laganico",
          "laganicom", "laganicama", "Laganica", "LAGANICA"], 2, R, true),
        // The noun must not swallow its verb's imperative or longer forms.
        (&["ispuni", "ispuniti", "ispunim", "ispunio", "ispunila"], 0, R, false),
        // Written accents still override dictionary and preferred readings.
        (&["ispúna"], 1, R, true),
        (&["nȉsi"], 0, F, false),
        (&["kȁfanica", "lȁganica"], 0, F, false),
    ];
    let frontend = Frontend::new(Language::Croatian);
    for &(forms, stress, tone, long_vowel) in paradigms {
        for &text in forms {
            let utterance = read(&frontend, text);
            require_one_word(&utterance, text);
            assert_eq!(stressed_syllables(&utterance), [stress], "{text}");
            let vowels = nuclei(&utterance);
            assert_eq!(vowels[stress].accent, tone, "{text}");
            assert_eq!(vowels[stress].is_long, long_vowel, "{text}");
            for vowel in &vowels {
                assert!(vowel.stressed || !vowel.is_long, "{text}");
            }
        }
    }
}

/// Collected accents survive phrases, homographs and user overrides
#[test]
fn collected_accents_survive_phrases_homographs_and_user_overrides() {
    #[rustfmt::skip]
    let cases: &[(&str, u16, usize, Accent, bool)] = &[
        ("zubna ispuna", 1, 0, F, false), ("u ispuni", 1, 0, F, false),
        ("nema ispune", 1, 0, F, false), ("pet ispuna", 1, 0, F, false),
        ("Ispuni obrazac", 0, 0, R, false), ("Molim ispuni obrazac", 1, 0, R, false),
        ("jednog Hrvata", 1, 1, R, true), ("pet Hrvata", 1, 1, R, true),
        ("hrabri vatrogasci", 1, 2, R, true), ("pet vatrogasaca", 1, 1, R, false),
        ("ovi muškarci", 1, 1, R, true), ("pet muškaraca", 1, 0, R, false),
        ("ovi policajci", 1, 2, R, true), ("pet policajaca", 1, 1, R, false),
        ("ti nisi sam", 1, 0, R, true), ("zašto nisi došao", 1, 0, R, true),
        ("kafanica laganica", 0, 2, R, true), ("kafanica laganica", 1, 2, R, true),
        ("u kafanici", 1, 2, R, true), ("s laganicom", 1, 2, R, true),
    ];
    let mut frontend = Frontend::new(Language::Croatian);
    for &(text, word, stress, tone, long_vowel) in cases {
        let vowels = word_nuclei(&read(&frontend, text), word);
        assert_eq!(stressed_count(&vowels), 1, "{text}");
        assert!(vowels[stress].stressed, "{text}");
        assert_eq!(vowels[stress].accent, tone, "{text}");
        assert_eq!(vowels[stress].is_long, long_vowel, "{text}");
    }
    // These shared verb forms must not fall through to the long -un rule
    // when the Croatian noun adds an imperative homograph.
    for language in [Language::Serbian, Language::Bosnian] {
        let other = Frontend::new(language);
        assert_eq!(stressed_syllables(&read(&other, "oni ispune obrazac"))[1], 2);
        assert_eq!(nuclei(&read(&other, "ispuni"))[0].accent, R);
    }
    frontend.set_user_lexicon(user(r#"{"entries":[{"word":"k^afanic*"},{"word":"n^isi"},{"word":"/ispuni"}]}"#));
    assert_eq!(stressed_syllables(&read(&frontend, "kafanica")), [0]);
    assert!(!nuclei(&read(&frontend, "nisi"))[0].is_long);
    assert_eq!(nuclei(&read(&frontend, "u ispuni"))[1].accent, R);
    assert_eq!(stressed_syllables(&read(&frontend, "kafaníca")), [2]);
}

/// Drinks and cevap keep sourced Croatian accents through inflection
#[test]
fn drinks_and_cevap_keep_sourced_croatian_accents_through_inflection() {
    #[rustfmt::skip]
    let paradigms: &[(&[&str], usize, Accent, bool)] = &[
        (&["pivo", "piva", "pivu", "pivom", "pivima", "pive", "pivi", "pivama",
          "Pivo", "PIVA"], 0, F, true),
        (&["vino", "vina", "vinu", "vinom", "vinima", "Vino", "VINA"], 0, R, true),
        (&["sok", "Sok", "SOK"], 0, F, true),
        (&["soka", "soku", "sokom", "soče", "sokovi", "sokova", "sokove",
          "sokovima"], 0, F, false),
        (&["pelinkovac", "pelinkovca", "pelinkovcu", "pelinkovcem", "pelinkovci",
          "pelinkovce", "pelinkovcima", "pelinkovaca", "pelinkovče",
          "Pelinkovac", "PELINKOVAC"], 0, R, false),
        (&["medica", "medice", "medici", "medicu", "medico", "medicom", "medicama",
          "Medica", "MEDICA"], 0, R, false),
        (&["ćevap", "Ćevap", "ĆEVAP"], 0, R, false),
        (&["ćevapa", "ćevapu", "ćevapom", "ćevapi", "ćevape", "ćevapima",
          "Ćevapi", "ĆEVAPI"], 1, R, true),
        (&["ćevapčić", "ćevapčića", "ćevapčiću", "ćevapčićem", "ćevapčići",
          "ćevapčiće", "ćevapčićima", "Ćevapčići", "ĆEVAPČIĆI"], 1, R, false),
        // Nearby words must not inherit these noun accents.
        (&["medicina", "medicine"], 2, R, true),
        (&["medicinska"], 1, R, false),
        (&["sokak"], 0, R, false),
        (&["sokaka", "sokacima"], 1, R, true),
        // Explicit accents still take priority.
        (&["pìvo", "vìno", "sòkovi"], 0, R, false),
        (&["ćȅvap"], 0, F, false),
    ];
    let frontend = Frontend::new(Language::Croatian);
    for &(forms, stress, tone, long_vowel) in paradigms {
        for &text in forms {
            let utterance = read(&frontend, text);
            require_one_word(&utterance, text);
            assert_eq!(stressed_syllables(&utterance), [stress], "{text}");
            let vowels = nuclei(&utterance);
            assert_eq!(vowels[stress].accent, tone, "{text}");
            assert_eq!(vowels[stress].is_long, long_vowel, "{text}");
            for vowel in &vowels {
                assert!(vowel.stressed || !vowel.is_long, "{text}");
            }
        }
    }
}

/// Drink and food accents survive phrases and respect user overrides
#[test]
fn drink_and_food_accents_survive_phrases_and_respect_user_overrides() {
    #[rustfmt::skip]
    let cases: &[(&str, u16, usize, Accent, bool)] = &[
        ("čaša piva", 1, 0, F, true), ("dvije pive", 1, 0, F, true),
        ("čaša vina", 1, 0, R, true), ("tri vina", 1, 0, R, true),
        ("pijem sok", 1, 0, F, true), ("čaša soka", 1, 0, F, false),
        ("pet sokova", 1, 0, F, false), ("o sokovima", 1, 0, F, false),
        ("čaša pelinkovca", 1, 0, R, false), ("s medicom", 1, 0, R, false),
        ("jedan ćevap", 1, 0, R, false), ("pet ćevapa", 1, 1, R, true),
        ("s ćevapima", 1, 1, R, true), ("pet ćevapčića", 1, 1, R, false),
    ];
    let mut frontend = Frontend::new(Language::Croatian);
    for &(text, word, stress, tone, long_vowel) in cases {
        let vowels = word_nuclei(&read(&frontend, text), word);
        assert_eq!(stressed_count(&vowels), 1, "{text}");
        assert!(vowels[stress].stressed, "{text}");
        assert_eq!(vowels[stress].accent, tone, "{text}");
        assert_eq!(vowels[stress].is_long, long_vowel, "{text}");
    }
    frontend.set_user_lexicon(user(r#"{"entries":[{"word":"p/iv*"},{"word":"ć^evap"}]}"#));
    assert_eq!(nuclei(&read(&frontend, "piva"))[0].accent, R);
    assert!(!nuclei(&read(&frontend, "piva"))[0].is_long);
    assert_eq!(nuclei(&read(&frontend, "ćevap"))[0].accent, F);
    assert_eq!(nuclei(&read(&frontend, "pȋva"))[0].accent, F);
    assert!(nuclei(&read(&frontend, "pȋva"))[0].is_long);
}

/// Vowels at word boundaries get a separation cue
#[test]
fn vowels_at_word_boundaries_get_a_separation_cue() {
    let frontend = Frontend::new(Language::Croatian);
    let boundaries = |text: &str| {
        let utterance = read(&frontend, text);
        utterance.phones.windows(2).filter(|pair| needs_vowel_separation(&pair[0], &pair[1])).count()
    };
    assert_eq!(boundaries("video uradak"), 1);
    assert_eq!(boundaries("moju ulicu"), 1);
    assert_eq!(boundaries("vidio ih"), 1); // also before an unstressed clitic
    assert_eq!(boundaries("ja i ona"), 2);
    assert_eq!(boundaries("njoj i njenom"), 1); // existing /j/ cue
    for left in "aeiou".chars() {
        for right in "aeiou".chars() {
            assert_eq!(boundaries(&format!("man{left} {right}lana")), 1, "{left} {right}");
            assert_eq!(boundaries(&format!("man{left}{right}lana")), 0, "{left}{right}");
        }
    }
    assert_eq!(boundaries("video"), 0);
    assert_eq!(boundaries("moju kuću"), 0);
    assert_eq!(boundaries("dobar uradak"), 0);
    assert_eq!(boundaries("moja jabuka"), 0);
    assert_eq!(boundaries("tifloasistent"), 0); // joined compound components
    assert_eq!(boundaries("krv ulazi"), 0); // syllabic r is not vowel hiatus
}

/// Svetac and hope verbs retain sourced accents across their paradigms
#[test]
fn svetac_and_hope_verbs_retain_sourced_accents_across_their_paradigms() {
    #[rustfmt::skip]
    let cases: &[(&str, usize, Accent, bool)] = &[
        ("svetac", 0, R, true), ("Svetac", 0, R, true), ("SVETAC", 0, R, true),
        ("sveci", 0, R, true), ("sveca", 0, R, true), ("svecu", 0, R, true),
        ("svecem", 0, R, true), ("svece", 0, R, true), ("svecima", 0, R, true),
        ("svetci", 0, R, true), ("svetca", 0, R, true), ("svetcu", 0, R, true),
        ("svetcem", 0, R, true), ("svetce", 0, R, true), ("svetcima", 0, R, true),
        ("sveče", 0, F, true), ("svetče", 0, F, true), ("svetaca", 0, F, true),
        ("nadati", 0, R, true), ("Nadati", 0, R, true), ("NADATI", 0, R, true),
        ("nadat", 0, R, true), ("nadao", 0, R, true), ("nadala", 0, R, true),
        ("nadalo", 0, R, true), ("nadali", 0, R, true), ("nadale", 0, R, true),
        ("nadaju", 0, R, true), ("nadajući", 0, R, true), ("nadah", 0, R, true),
        ("nadam", 0, F, true), ("nadaš", 0, F, true), ("nadamo", 0, F, true),
        ("nadate", 0, F, true), ("nadaj", 0, F, true), ("nadajmo", 0, F, true),
        ("nadajte", 0, F, true), ("nada", 0, R, true), ("nade", 0, R, true),
        ("nadi", 0, R, true), ("nadu", 0, R, true), ("nadom", 0, R, true),
        ("nadama", 0, R, true),
        ("ponadati", 1, R, true), ("Ponadati", 1, R, true),
        ("ponadat", 1, R, true), ("ponadao", 1, R, true), ("ponadala", 1, R, true),
        ("ponadalo", 1, R, true), ("ponadali", 1, R, true), ("ponadale", 1, R, true),
        ("PONADALI", 1, R, true), ("ponadaju", 1, R, true), ("ponadavši", 1, R, true),
        ("ponadah", 1, R, true), ("ponadasmo", 1, R, true), ("ponadaste", 1, R, true),
        ("ponadam", 0, R, false), ("ponadaš", 0, R, false), ("ponada", 0, R, false),
        ("ponadamo", 0, R, false), ("ponadate", 0, R, false),
        ("ponadaj", 0, R, false), ("ponadajmo", 0, R, false), ("ponadajte", 0, R, false),
        ("svȅtac", 0, F, false), ("nàdati", 0, R, false), ("pònadali", 0, R, false),
    ];
    let frontend = Frontend::new(Language::Croatian);
    for &(text, stress, tone, long_vowel) in cases {
        let utterance = read(&frontend, text);
        require_one_word(&utterance, text);
        assert_eq!(stressed_syllables(&utterance), [stress], "{text}");
        let vowels = nuclei(&utterance);
        assert_eq!(vowels[stress].accent, tone, "{text}");
        assert_eq!(vowels[stress].is_long, long_vowel, "{text}");
        for vowel in &vowels {
            assert!(vowel.stressed || !vowel.is_long, "{text}");
        }
    }
}

/// Nada distinguishes the noun from the reflexive verb and respects overrides
#[test]
fn nada_distinguishes_the_noun_from_the_reflexive_verb_and_respects_overrides() {
    let mut frontend = Frontend::new(Language::Croatian);
    let word_accent = |reader: &Frontend, text: &str, word: u16| {
        let utterance = read(reader, text);
        utterance.phones.iter().find(|p| p.word == word && p.stressed).map_or(Accent::None, |p| p.accent)
    };
    assert_eq!(word_accent(&frontend, "nada se", 0), F);
    assert_eq!(word_accent(&frontend, "Nada se uspjehu", 0), F);
    assert_eq!(word_accent(&frontend, "on se nada", 2), F);
    assert_eq!(word_accent(&frontend, "moja nada", 1), R);
    assert_eq!(word_accent(&frontend, "Nada se nadala", 0), R);
    assert_eq!(word_accent(&frontend, "nada se pojavila", 0), R);
    assert_eq!(word_accent(&frontend, "náda se", 0), R);
    frontend.set_user_lexicon(user(r#"{"entries":[{"word":"n/a:da"},{"word":"sv^etac"},{"verb":"p^onad=a"}]}"#));
    assert_eq!(word_accent(&frontend, "nada se", 0), R);
    assert!(!nuclei(&read(&frontend, "svetac"))[0].is_long);
    assert_eq!(stressed_syllables(&read(&frontend, "ponadali")), [0]);
}

/// Ostarjeti forms keep initial rising stress and a short a
#[test]
fn ostarjeti_forms_keep_initial_rising_stress_and_a_short_a() {
    let frontend = Frontend::new(Language::Croatian);
    // ŠR explicitly lists òstarīm, òstarē, òstari, òstarjeh, òstario,
    // òstarjela/òstarjelo/òstarjeli. Check the remaining regular forms too.
    let forms: &[&str] = &[
        "ostare",
        "Ostare",
        "OSTARE",
        "ostarim",
        "ostariš",
        "ostari",
        "ostarimo",
        "ostarite",
        "ostarjeti",
        "ostarjet",
        "ostariti",
        "ostarit",
        "ostario",
        "ostarjela",
        "ostarjelo",
        "ostarjeli",
        "ostarjele",
        "ostarila",
        "ostarilo",
        "ostarili",
        "ostarile",
        "ostarjeh",
        "ostarje",
        "ostarjesmo",
        "ostarjeste",
        "ostarješe",
        "ostarjevši",
        "ostarih",
        "ostarismo",
        "ostariste",
        "ostariše",
        "ostarivši",
    ];
    for &text in forms {
        let utterance = read(&frontend, text);
        require_one_word(&utterance, text);
        assert_eq!(stressed_syllables(&utterance), [0], "{text}");
        let vowels = nuclei(&utterance);
        assert!(vowels.len() >= 3, "{text}");
        assert_eq!(vowels[0].accent, R, "{text}");
        assert!(!vowels[0].is_long, "{text}");
        assert_eq!(vowels[1].ph, Ph::A, "{text}");
        assert!(!vowels[1].stressed, "{text}");
        assert!(!vowels[1].is_long, "{text}");
    }
    // The correction also applies away from clause head and keeps explicit
    // user-supplied accents authoritative.
    let phrase = nuclei(&read(&frontend, "oni ostare"));
    assert!(phrase[2].stressed);
    assert_eq!(phrase[2].accent, R);
    assert!(!phrase[3].stressed);
    assert!(!phrase[3].is_long);
    let explicit_accent = nuclei(&read(&frontend, "ostáre"));
    assert!(explicit_accent[1].stressed);
    assert!(explicit_accent[1].is_long);
}

/// Collected Croatian accents retain quantity tone and inflection
#[test]
fn collected_croatian_accents_retain_quantity_tone_and_inflection() {
    // Školski rječnik principal forms; source choices and inferred
    // inflections are documented in docs/formant.md (12 October 2026).
    #[rustfmt::skip]
    let cases: &[(&str, i32, usize, Accent, bool)] = &[
        ("razlikujemo", 5, 0, R, true), ("Razlikujemo", 5, 0, R, true),
        ("razlikujem", 4, 0, R, true), ("razlikuješ", 4, 0, R, true),
        ("razlikuje", 4, 0, R, true), ("razlikujete", 5, 0, R, true),
        ("razlikuju", 4, 0, R, true), ("razlikujući", 5, 0, R, true),
        ("razlikuj", 3, 0, R, true), ("razlikujmo", 4, 0, R, true),
        ("razlikujte", 4, 0, R, true), ("razlikovati", 5, 0, R, true),
        ("razlikovao", 5, 0, R, true), ("razlikovala", 5, 0, R, true),
        ("razlikovali", 5, 0, R, true), ("razlikovan", 4, 0, R, true),
        ("razlikovanima", 6, 0, R, true), ("razlikovanje", 5, 0, R, true),
        ("apsurdni", 3, 0, R, false), ("Apsurdni", 3, 0, R, false),
        ("apsurdan", 3, 0, R, false), ("apsurdna", 3, 0, R, false),
        ("apsurdno", 3, 0, R, false), ("apsurdne", 3, 0, R, false),
        ("apsurdnu", 3, 0, R, false), ("apsurdnog", 3, 0, R, false),
        ("apsurdnoga", 4, 0, R, false), ("apsurdnoj", 3, 0, R, false),
        ("apsurdnome", 4, 0, R, false), ("apsurdnima", 4, 0, R, false),
        ("apsurdniji", 4, 1, R, false), ("apsurdnijima", 5, 1, R, false),
        ("najapsurdniji", 5, 2, R, false),
        ("društvo", 2, 0, R, true), ("Društvo", 2, 0, R, true),
        ("društva", 2, 0, R, true), ("društvu", 2, 0, R, true),
        ("društvom", 2, 0, R, true), ("društvima", 3, 0, R, true),
        ("društava", 3, 0, F, true), ("drùštvo", 2, 0, R, false),
        ("VIŠEDNEVNI", 4, 0, F, false), ("višèdnevni", 4, 1, R, false),
        ("višemjesečni", 5, 1, R, false), ("petodnevni", 4, 1, R, false),
        ("višednevnog", 4, 0, F, false), ("Višednevnog", 4, 0, F, false),
        ("višednevni", 4, 0, F, false), ("višednevna", 4, 0, F, false),
        ("višednevno", 4, 0, F, false), ("višednevnoga", 5, 0, F, false),
        ("višednevnoj", 4, 0, F, false), ("višednevnima", 5, 0, F, false),
        ("sposobnosti", 4, 1, R, true), ("Sposobnosti", 4, 1, R, true),
        ("sposobnost", 3, 1, R, true), ("sposobnošću", 4, 1, R, true),
        ("sposobnostima", 5, 1, R, true),
        // Listener's initial stress; written accents retain the ŠR/HJP form.
        ("pojedine", 4, 0, F, false), ("Pojedine", 4, 0, F, false),
        ("pojedini", 4, 0, F, false), ("pojedina", 4, 0, F, false),
        ("pojedino", 4, 0, F, false), ("pojedinu", 4, 0, F, false),
        ("pojedinoga", 5, 0, F, false), ("pojedinomu", 5, 0, F, false),
        ("pojedinoj", 4, 0, F, false), ("pojedinih", 4, 0, F, false),
        ("pojedinima", 5, 0, F, false), ("POJEDINI", 4, 0, F, false),
        ("pojèdini", 4, 1, R, false),
        ("pojedinac", 4, 2, R, false), ("pojedinca", 4, 2, R, false),
        ("vještinu", 3, 1, R, false), ("Vještinu", 3, 1, R, false),
        ("vještina", 3, 1, R, false), ("vještine", 3, 1, R, false),
        ("vještini", 3, 1, R, false), ("vještinom", 3, 1, R, false),
        ("vještinama", 4, 1, R, false),
        ("prvenstveno", 4, 1, R, false), ("Prvenstveno", 4, 1, R, false),
        ("jugoslavija", 5, 1, R, false), ("Jugoslavija", 5, 1, R, false),
        ("JUGOSLAVIJA", 5, 1, R, false), ("Jugoslavije", 5, 1, R, false),
        ("Jugoslaviji", 5, 1, R, false), ("Jugoslaviju", 5, 1, R, false),
        ("Jugoslavijo", 5, 1, R, false), ("Jugoslavijom", 5, 1, R, false),
        ("jugoslavijom", 5, 1, R, false),
        ("žal", 1, 0, F, true), ("Žal", 1, 0, F, true),
        ("ŽAL", 1, 0, F, true),
        ("žala", 2, 0, F, true), ("žalu", 2, 0, F, true),
        ("žalom", 2, 0, F, true), // default regret reading
        ("žála", 2, 0, R, true), ("žálu", 2, 0, R, true),
        ("žálom", 2, 0, R, true), // explicitly supplied shore reading
        ("žalovi", 3, 0, R, true), ("žalova", 3, 0, R, true),
        ("žalove", 3, 0, R, true), ("žalovima", 4, 0, R, true),
        ("ulaganje", 4, 1, R, true), ("Ulaganje", 4, 1, R, true),
        ("ulaganja", 4, 1, R, true), ("ulaganju", 4, 1, R, true),
        ("ulaganjem", 4, 1, R, true), ("ulaganjima", 5, 1, R, true),
        ("vrag", 1, 0, F, true), ("Vrag", 1, 0, F, true),
        ("vraga", 2, 0, F, true), ("vragu", 2, 0, F, true),
        ("vraže", 2, 0, F, true), ("vragom", 2, 0, F, true),
        ("vragovi", 3, 0, F, false), ("vragova", 3, 0, F, false),
        ("vragove", 3, 0, F, false), ("vragovima", 4, 0, F, false),
        ("vrazi", 2, 0, F, true),
        ("mrak", 1, 0, F, true), ("Mrak", 1, 0, F, true),
        ("mraka", 2, 0, F, true), ("mraku", 2, 0, F, true),
        ("mrakom", 2, 0, F, true),
        ("mrakovi", 3, 0, F, false), ("mrakova", 3, 0, F, false),
        ("mrakove", 3, 0, F, false), ("mrakovima", 4, 0, F, false),
        ("mraci", 2, 0, F, true),
        ("zrak", 1, 0, F, true), ("Zrak", 1, 0, F, true),
        ("zraka", 2, 0, F, true), ("zraku", 2, 0, F, true),
        ("zrakom", 2, 0, F, true),
        ("zrake", 2, 0, F, false), ("zraci", 2, 0, F, false),
        ("zrako", 2, 0, F, false), ("zrakama", 3, 0, F, false),
        ("rastegnuti", 4, 1, R, true), ("rastegni", 3, 1, R, true),
        ("rastegnimo", 4, 1, R, true), ("rastegnite", 4, 1, R, true),
        ("rastegnuo", 4, 1, R, true), ("rastegnula", 4, 1, R, true),
        ("rastegnuli", 4, 1, R, true), ("rastegnuh", 3, 1, R, true),
        ("rastegnuvši", 4, 1, R, true),
        ("rastegnem", 3, 0, R, false), ("rastegneš", 3, 0, R, false),
        ("rastegne", 3, 0, R, false), ("rastegnemo", 4, 0, R, false),
        ("rastegnete", 4, 0, R, false), ("rastegnu", 3, 0, R, false),
        ("rastegnut", 3, 0, R, false), ("rastegnuta", 4, 0, R, false),
        ("rastegnutima", 5, 0, R, false),
        ("protegnuti", 4, 1, R, true), ("protegni", 3, 1, R, true),
        ("protegnimo", 4, 1, R, true), ("protegnite", 4, 1, R, true),
        ("protegnuo", 4, 1, R, true), ("protegnula", 4, 1, R, true),
        ("protegnuli", 4, 1, R, true), ("protegnuh", 3, 1, R, true),
        ("protegnuvši", 4, 1, R, true),
        ("protegnem", 3, 0, R, false), ("protegneš", 3, 0, R, false),
        ("protegne", 3, 0, R, false), ("protegnemo", 4, 0, R, false),
        ("protegnete", 4, 0, R, false), ("protegnu", 3, 0, R, false),
        ("protegnut", 3, 0, R, false), ("protegnuta", 4, 0, R, false),
        ("protegnutima", 5, 0, R, false),
        ("pošiljatelj", 4, 1, R, false), ("Pošiljatelj", 4, 1, R, false),
        ("pošiljatelja", 5, 1, R, false), ("pošiljatelju", 5, 1, R, false),
        ("pošiljateljem", 5, 1, R, false), ("pošiljatelji", 5, 1, R, false),
        ("pošiljatelje", 5, 1, R, false), ("pošiljateljima", 6, 1, R, false),
        ("zvučna", 2, 0, R, true), ("zvučan", 2, 0, F, true),
        ("zvučno", 2, 0, F, true), ("zvučni", 2, 0, F, true),
        ("zvučnoga", 3, 0, F, true), ("zvučnima", 3, 0, F, true),
        ("zvučniji", 3, 0, R, false), ("zvučnijima", 4, 0, R, false),
        ("zvȗčna", 2, 0, F, true), // explicitly requested definite form
        ("anketa", 3, 1, R, false), ("Anketa", 3, 1, R, false),
        ("ankete", 3, 1, R, false), ("anketi", 3, 1, R, false),
        ("anketu", 3, 1, R, false), ("anketom", 3, 1, R, false),
        ("anketama", 4, 1, R, false),
        ("zategnuti", 4, 1, R, true), ("zategni", 3, 1, R, true),
        ("zategnite", 4, 1, R, true), ("zategnimo", 4, 1, R, true),
        ("zategnuo", 4, 1, R, true), ("zategnula", 4, 1, R, true),
        ("zategnuli", 4, 1, R, true), ("zategnuh", 3, 1, R, true),
        ("zategnuvši", 4, 1, R, true),
        ("zategnem", 3, 0, R, false), ("zategneš", 3, 0, R, false),
        ("zategne", 3, 0, R, false), ("zategnemo", 4, 0, R, false),
        ("zategnete", 4, 0, R, false), ("zategnu", 3, 0, R, false),
        ("zategnut", 3, 0, R, false), ("zategnuta", 4, 0, R, false),
        ("zategnutim", 4, 0, R, false), ("zategnutima", 5, 0, R, false),
        ("zategao", 4, 0, R, false), ("zategla", 3, 0, R, false),
        ("zategli", 3, 0, R, false),
        ("kriva", 2, 0, R, true), ("kriv", 1, 0, F, true),
        ("krivo", 2, 0, F, true), ("krivi", 2, 0, F, true),
        ("krivoga", 3, 0, F, true), ("krivima", 3, 0, F, true),
        ("krivlji", 2, 0, F, false), ("krivljima", 3, 0, F, false),
        ("krȋva", 2, 0, F, true),
        ("Drina", 2, 0, R, true), ("Drine", 2, 0, R, true),
        ("Drini", 2, 0, R, true), ("Drinu", 2, 0, R, true),
        ("Drinom", 2, 0, R, true),
        ("kriminalac", 4, 2, R, true), ("kriminalca", 4, 2, R, true),
        ("kriminalcu", 4, 2, R, true), ("kriminalcem", 4, 2, R, true),
        ("kriminalci", 4, 2, R, true), ("kriminalce", 4, 2, R, true),
        ("kriminalcima", 5, 2, R, true), ("kriminalče", 4, 0, F, false),
        ("kriminalaca", 5, 1, R, false),
        ("dosije", 3, 1, R, false), ("Dosije", 3, 1, R, false),
        ("dosijea", 4, 2, R, false), ("dosijeu", 4, 2, R, false),
        ("dosijeom", 4, 2, R, false), ("dosijei", 4, 2, R, false),
        ("dosijee", 4, 2, R, false), ("dosijeima", 5, 2, R, false),
        ("očajnički", 4, 0, F, false), ("očajnička", 4, 0, F, false),
        ("očajničkog", 4, 0, F, false), ("očajničkima", 5, 0, F, false),
        ("ispraviti", 4, 0, R, false), ("ispravim", 3, 0, R, false),
        ("isprave", 3, 0, R, false), ("ispravite", 4, 0, R, false),
        ("ispravio", 4, 0, R, false), ("ispravila", 4, 0, R, false),
        ("ispravljen", 3, 0, R, false), ("ispravljenima", 5, 0, R, false),
        ("budućnost", 3, 1, R, true), ("budućnosti", 4, 1, R, true),
        ("budućnošću", 4, 1, R, true), ("budućnostima", 5, 1, R, true),
        ("razočaranja", 5, 3, R, true), ("razočaranje", 5, 3, R, true),
        ("razočaranju", 5, 3, R, true), ("razočaranjem", 5, 3, R, true),
        ("razočaranjima", 6, 3, R, true),
        ("pomislio", 4, 0, R, false), ("pomislila", 4, 0, R, false),
        ("pomislili", 4, 0, R, false), ("pomisliti", 4, 0, R, false),
        ("pomislim", 3, 0, R, false), ("pomislite", 4, 0, R, false),
        ("postoji", 3, 0, R, true), ("postojati", 4, 0, R, true),
        ("postojim", 3, 0, R, true), ("postojimo", 4, 0, R, true),
        ("postojite", 4, 0, R, true), ("postoje", 3, 0, R, true),
        ("postoj", 2, 0, R, true), ("postojte", 3, 0, R, true),
        ("postojao", 4, 0, R, true), ("postojali", 4, 0, R, true),
        ("postojeći", 4, 0, R, true), ("postojanje", 4, 0, R, true),
        ("viri", 2, 0, R, true), ("viriti", 3, 0, R, true),
        ("virim", 2, 0, R, true), ("vire", 2, 0, R, true),
        ("virite", 3, 0, R, true), ("virio", 3, 0, R, true),
        ("vireći", 3, 0, R, true), ("virenje", 3, 0, R, true),
        ("vir", 1, 0, F, true), ("vira", 2, 0, F, true),
        ("viru", 2, 0, F, true), ("virom", 2, 0, F, true),
        ("virovi", 3, 0, F, false), ("virovima", 4, 0, F, false),
        ("sunce", 2, 0, F, true), ("Sunce", 2, 0, F, true),
        ("sunca", 2, 0, F, true), ("suncu", 2, 0, F, true),
        ("suncem", 2, 0, F, true), ("suncima", 3, 0, F, true),
        ("sunaca", 3, 0, F, false),
        ("noć", 1, 0, F, true), ("noći", 2, 0, F, false),
        ("noću", 2, 0, F, false), ("noćima", 3, 0, R, false),
        ("trud", 1, 0, F, true), ("truda", 2, 0, R, true),
        ("trudu", 2, 0, R, true), ("trudom", 2, 0, R, true),
        ("trudovi", 3, 0, R, true), ("trudova", 3, 0, R, true),
        ("trudovima", 4, 0, R, true), ("trudima", 3, 0, R, true),
        ("povucite", 4, 1, R, true), ("povuci", 3, 1, R, true),
        ("povucimo", 4, 1, R, true), ("povući", 3, 0, R, false),
        ("povukao", 4, 0, R, false), ("povukli", 3, 0, R, false),
        ("povukavši", 4, 0, R, false), ("povučem", 3, 1, R, true),
        ("povučete", 4, 1, R, true), ("povuku", 3, 1, R, true),
        ("povukoh", 3, 1, R, true), ("povučen", 3, 1, R, false),
        ("povučena", 4, 1, R, false), ("povučenima", 5, 1, R, false),
    ];
    let frontend = Frontend::new(Language::Croatian);
    for &(text, syllables, stress, tone, long_vowel) in cases {
        let utterance = read(&frontend, text);
        assert_eq!(utterance.syllable_count, syllables, "{text}");
        let vowels = nuclei(&utterance);
        assert_eq!(stressed_count(&vowels), 1, "{text}");
        assert!(vowels[stress].stressed, "{text}");
        assert_eq!(vowels[stress].accent, tone, "{text}");
        assert_eq!(vowels[stress].is_long, long_vowel, "{text}");
        for vowel in &vowels {
            assert!(vowel.stressed || !vowel.is_long, "{text}");
        }
    }
    // Noun corrections must not lengthen homographic verbal forms.
    assert!(!nuclei(&read(&frontend, "žali"))[0].is_long);
    assert!(!nuclei(&read(&frontend, "žale"))[0].is_long);
}

/// Vir and noc case cues preserve length and user accents
#[test]
fn vir_and_noc_case_cues_preserve_length_and_user_accents() {
    #[rustfmt::skip]
    let cases: &[(&str, u16, Accent, bool)] = &[
        ("u viru", 1, R, true), ("o dubokom viru", 2, R, true),
        ("prema viru", 1, F, true), ("iz vira", 1, F, true),
        ("dvije noći", 1, F, false), ("tijekom noći", 1, F, false),
        ("pet noći", 1, R, false), ("nekoliko noći", 1, R, false),
        ("pet dugih noći", 2, R, false), ("u noći", 1, R, false),
        ("o toj dugoj noći", 3, R, false), ("prema noći", 1, F, false),
        ("u vȋru", 1, F, true), ("pet nȍći", 1, F, false),
        ("o nȏći", 1, F, true),
    ];
    let frontend = Frontend::new(Language::Croatian);
    for &(text, word, tone, long_vowel) in cases {
        let vowels = word_nuclei(&read(&frontend, text), word);
        assert!(vowels[0].stressed, "{text}");
        assert_eq!(vowels[0].accent, tone, "{text}");
        assert_eq!(vowels[0].is_long, long_vowel, "{text}");
    }
    let mut custom = Frontend::new(Language::Croatian);
    custom.set_user_lexicon(user(
        r#"{"entries":[{"word":"v^i:ru"},{"word":"n^o:ći"},{"word":"p^ostoji"},{"verb":"pov/uc=e"}]}"#,
    ));
    assert_eq!(nuclei(&read(&custom, "u viru"))[1].accent, F);
    assert_eq!(nuclei(&read(&custom, "pet noći"))[1].accent, F);
    assert!(nuclei(&read(&custom, "pet noći"))[1].is_long);
    assert_eq!(nuclei(&read(&custom, "postoji"))[0].accent, F);
    assert!(!nuclei(&read(&custom, "povucite"))[1].is_long);
    assert!(nuclei(&read(&frontend, "pòvucite"))[0].stressed);
    let mut stem = Frontend::new(Language::Croatian);
    stem.set_user_lexicon(user(r#"{"entries":[{"word":"n^o:ć*"},{"word":"p^ovuc*"}]}"#));
    assert_eq!(nuclei(&read(&stem, "pet noći"))[1].accent, F);
    assert!(nuclei(&read(&stem, "pet noći"))[1].is_long);
    assert!(nuclei(&read(&stem, "povucite"))[0].stressed);
    assert!(nuclei(&read(&stem, "povúcite"))[1].stressed);
    // Within user entries, exact words beat stems, which beat verbs.
    stem.set_user_lexicon(user(r#"{"entries":[{"word":"pov/u:ci"},{"word":"p^ovuc*"},{"verb":"pov/uc=e"}]}"#));
    assert!(nuclei(&read(&stem, "povuci"))[1].is_long);
    assert_eq!(nuclei(&read(&stem, "povucite"))[0].accent, F);
    // Existing shared nȏć remains long in the other voices as well.
    for language in [Language::Serbian, Language::Bosnian] {
        assert!(nuclei(&read(&Frontend::new(language), "noć"))[0].is_long);
    }
}

/// Pacijentima through dusa retain sourced accents and inflections
#[test]
fn pacijentima_through_dusa_retain_sourced_accents_and_inflections() {
    #[rustfmt::skip]
    let cases: &[(&str, i32, usize, Accent, bool)] = &[
        ("pacijentima", 5, 1, R, false), ("pacijent", 3, 1, R, false),
        ("pacijenta", 4, 1, R, false), ("pacijenata", 5, 1, R, false),
        ("veseli", 3, 0, F, false), ("veseo", 3, 0, F, false),
        ("vesela", 3, 0, F, false), ("veseloga", 4, 0, F, false),
        ("veselima", 4, 0, F, false), ("veseliji", 4, 1, R, false),
        ("veseliti", 4, 1, R, false), ("veselimo", 4, 1, R, false),
        ("zaštita", 3, 0, R, false), ("zaštite", 3, 0, R, false),
        ("zaštitom", 3, 0, R, false), ("zaštitama", 4, 0, R, false),
        ("posjekotina", 5, 2, R, false), ("posjekotine", 5, 2, R, false),
        ("posjekotinom", 5, 2, R, false), ("posjekotinama", 6, 2, R, false),
        ("nepravilnosti", 5, 2, R, true), ("nepravilnost", 4, 2, R, true),
        ("nepravilnošću", 5, 2, R, true), ("nepravilnostima", 6, 2, R, true),
        ("nježno", 2, 0, F, false), ("nježan", 2, 0, F, false),
        ("nježna", 2, 0, R, false), ("nježni", 2, 0, F, false),
        ("nježniji", 3, 0, R, false),
        ("poliklinika", 5, 2, R, false), ("poliklinike", 5, 2, R, false),
        ("poliklinici", 5, 2, R, false), ("poliklinikama", 6, 2, R, false),
        ("cedevita", 4, 2, R, false), ("Cedevita", 4, 2, R, false),
        ("Cedevite", 4, 2, R, false), ("Cedevitom", 4, 2, R, false),
        ("dostojanstvo", 4, 2, R, false), ("dostojanstva", 4, 2, R, false),
        ("dostojanstvom", 4, 2, R, false), ("dostojanstvima", 5, 2, R, false),
        ("gladan", 2, 0, F, true), ("gladna", 2, 0, R, true),
        ("gladno", 2, 0, F, true), ("gladni", 2, 0, F, true),
        ("gladnijima", 4, 0, R, false),
        ("glad", 1, 0, F, true), ("gladi", 2, 0, F, true),
        ("glađu", 2, 0, F, true),
        ("dana", 2, 0, F, true), ("dan", 1, 0, F, true),
        ("danu", 2, 0, F, true), ("danima", 3, 0, F, true),
        ("biće", 2, 0, R, true), ("bića", 2, 0, R, true),
        ("bićima", 3, 0, R, true),
        ("dijeta", 3, 1, R, true), ("dijeti", 3, 1, R, true),
        ("dijetom", 3, 1, R, true), ("dijetama", 4, 1, R, true),
        ("dijete", 2, 0, R, true), // preserve the default child reading
        ("duša", 2, 0, R, true), ("duši", 2, 0, F, true),
        ("dušu", 2, 0, F, true), ("dušo", 2, 0, F, true),
        ("duše", 2, 0, F, true), ("dušom", 2, 0, R, true),
        ("dušama", 3, 0, R, true),
        // User-confirmed female name and middle syllable; length remains
        // provisional. The short rising rendering is an explicit policy.
        ("Dženana", 3, 1, R, false), ("Dženane", 3, 1, R, false),
        ("Dženani", 3, 1, R, false), ("Dženanu", 3, 1, R, false),
        ("Dženanom", 3, 1, R, false),
    ];
    let frontend = Frontend::new(Language::Croatian);
    for &(text, syllables, stress, tone, long_vowel) in cases {
        let utterance = read(&frontend, text);
        assert_eq!(utterance.syllable_count, syllables, "{text}");
        let vowels = nuclei(&utterance);
        assert_eq!(stressed_count(&vowels), 1, "{text}");
        assert!(vowels[stress].stressed, "{text}");
        assert_eq!(vowels[stress].accent, tone, "{text}");
        assert_eq!(vowels[stress].is_long, long_vowel, "{text}");
        for vowel in &vowels {
            assert!(vowel.stressed || !vowel.is_long, "{text}");
        }
    }
}

/// Health list homographs use bounded cues and preserve overrides
#[test]
fn health_list_homographs_use_bounded_cues_and_preserve_overrides() {
    #[rustfmt::skip]
    let cases: &[(&str, u16, usize, Accent, bool)] = &[
        ("veseli ljudi", 0, 0, F, false), ("Oni su veseli", 2, 0, F, false),
        ("te vesele žene", 1, 0, F, false), ("s veselim ljudima", 1, 0, F, false),
        ("to me veseli", 2, 1, R, false), ("Veseli me", 0, 1, R, false),
        ("veselim se", 0, 1, R, false), ("vesele se", 0, 1, R, false),
        ("svakog dana", 1, 0, F, true), ("dva dana", 1, 0, F, true),
        ("tri dana", 1, 0, F, true), ("pet dana", 1, 0, R, true),
        ("nekoliko dana", 1, 0, R, true), ("tjedan dana", 1, 0, R, true),
        ("pet dugih dana", 2, 0, R, true), ("od dana", 1, 0, F, true),
        ("o danu", 1, 0, R, true), ("o tom danu", 2, 0, R, true),
        ("prema danu", 1, 0, F, true),
        ("bez duše", 1, 0, R, true), ("iz dubine duše", 2, 0, R, true),
        ("mirne duše", 1, 0, R, true), ("dvije duše", 1, 0, F, true),
        ("mrtve duše", 1, 0, F, true), ("na duši", 1, 0, F, true),
        ("više zaštita", 1, 0, F, false), ("ta zaštita", 1, 0, R, false),
        ("pet dȃna", 1, 0, F, true), ("o dȃnu", 1, 0, F, true),
        ("to me vȅseli", 2, 0, F, false), ("vesèli ljudi", 0, 1, R, false),
        ("bez dȗše", 1, 0, F, true), ("više zàštita", 1, 0, R, false),
        ("Dženána", 0, 1, R, true),
    ];
    let frontend = Frontend::new(Language::Croatian);
    for &(text, word, stress, tone, long_vowel) in cases {
        let vowels = word_nuclei(&read(&frontend, text), word);
        assert!(vowels[stress].stressed, "{text}");
        assert_eq!(vowels[stress].accent, tone, "{text}");
        assert_eq!(vowels[stress].is_long, long_vowel, "{text}");
    }
    let mut custom = Frontend::new(Language::Croatian);
    custom.set_user_lexicon(user(r#"{"entries":[{"word":"v^eseli"},{"word":"d^a:na"},{"word":"d^a:nu"},{"word":"d^u:še"},{"word":"z/aštita"},{"word":"džen/a:na"}]}"#));
    assert!(nuclei(&read(&custom, "veseli se"))[0].stressed);
    assert_eq!(nuclei(&read(&custom, "pet dana"))[1].accent, F);
    assert_eq!(nuclei(&read(&custom, "o danu"))[1].accent, F);
    assert_eq!(nuclei(&read(&custom, "bez duše"))[1].accent, F);
    assert_eq!(nuclei(&read(&custom, "više zaštita"))[2].accent, R);
    assert!(nuclei(&read(&custom, "Dženana"))[1].is_long);
    // The name table remains capitalization-sensitive and Croatian-only.
    assert!(nuclei(&read(&frontend, "dženana"))[0].stressed);
    assert!(nuclei(&read(&frontend, "DŽENANA"))[0].stressed);
    assert!(nuclei(&read(&Frontend::new(Language::Bosnian), "Dženana"))[0].stressed);
}

/// Croatian tiflo compounds have two stresses within one word
#[test]
fn croatian_tiflo_compounds_have_two_stresses_within_one_word() {
    // The user explicitly requested independent accents for both components,
    // including established compounds that dictionaries accent as one unit.
    #[rustfmt::skip]
    let cases: &[(&str, i32, usize)] = &[
        ("tiflonova", 4, 2),
        ("TifloNova", 4, 2),
        ("TIFLONOVA", 4, 2),
        ("Tiflonove", 4, 2),
        ("tiflonovom", 4, 2),
        ("tiflotehnika", 5, 2),
        ("tiflotehnici", 5, 2),
        ("tiflotehnikama", 6, 2),
        ("tiflolog", 3, 2),
        ("tiflologija", 5, 2),
        ("tiflopedagogija", 7, 4),
        // A productive component can carry its syllable on r rather than a vowel.
        ("tifloprst", 3, 2),
    ];
    let frontend = Frontend::new(Language::Croatian);
    for &(text, syllables, second_stress) in cases {
        let utterance = read(&frontend, text);
        assert_eq!(utterance.syllable_count, syllables, "{text}");
        assert_eq!(stressed_syllables(&utterance), [0, second_stress], "{text}");
        require_one_word(&utterance, text);
        let vowels = nuclei(&utterance);
        assert_eq!(vowels[0].accent, F, "{text}");
        assert!(!vowels[0].is_long, "{text}");
        assert!(!vowels[1].stressed, "{text}");
    }
}

/// Tiflo components retain their independent syllables and prosody
#[test]
fn tiflo_components_retain_their_independent_syllables_and_prosody() {
    let frontend = Frontend::new(Language::Croatian);
    #[rustfmt::skip]
    let cases: &[(&str, &str)] = &[
        ("tiflonova", "nova"),
        ("tiflotehnika", "tehnika"),
        ("tiflotehnikama", "tehnikama"),
        ("tiflologija", "logija"),
        ("tiflopedagogija", "pedagogija"),
        ("tifloprst", "prst"),
    ];
    let prefix = nuclei(&read(&frontend, "tiflo"));
    assert_eq!(prefix.len(), 2);
    for &(compound_text, tail_text) in cases {
        let compound = nuclei(&read(&frontend, compound_text));
        let mut separate = prefix.clone();
        separate.extend(nuclei(&read(&frontend, tail_text)));
        assert_eq!(compound.len(), separate.len(), "{compound_text}");
        for (joined, apart) in compound.iter().zip(&separate) {
            assert_eq!(joined.ph, apart.ph, "{compound_text}");
            assert_eq!(joined.stressed, apart.stressed, "{compound_text}");
            assert_eq!(joined.accent, apart.accent, "{compound_text}");
            assert_eq!(joined.is_long, apart.is_long, "{compound_text}");
        }
    }
    let technical = nuclei(&read(&frontend, "tiflotehnika"));
    assert_eq!(technical[2].accent, R); // dictionary tèhnika
    assert!(!technical[2].is_long);
    // Splitting the first spelling must not merge its following real word.
    let phrase = read(&frontend, "TifloNova radi");
    assert_eq!(word_starts(&phrase), 2);
    assert_ne!(phrase.phones[0].word, last_word(&phrase));
}

/// Written tiflo accents override component stress
#[test]
fn written_tiflo_accents_override_component_stress() {
    let mut frontend = Frontend::new(Language::Croatian);
    let written = read(&frontend, "tiflónova");
    assert_eq!(stressed_syllables(&written), [1]);
    require_one_word(&written, "tiflónova");
    assert_eq!(nuclei(&written)[1].accent, R);
    assert!(nuclei(&written)[1].is_long);
    let written_tail = read(&frontend, "tiflonóva");
    assert_eq!(stressed_syllables(&written_tail), [2]);
    assert!(nuclei(&written_tail)[2].is_long);
    // Written accents retain priority even over a user entry for the word.
    frontend.set_user_lexicon(user(r#"{"entries":[{"word":"tiflon/o:va"}]}"#));
    assert_eq!(stressed_syllables(&read(&frontend, "tiflónova")), [1]);
}

/// User tiflo words stems and verbs override splitting
#[test]
fn user_tiflo_words_stems_and_verbs_override_splitting() {
    #[rustfmt::skip]
    let cases: &[(&str, &str, usize, bool)] = &[
        (r#"{"entries":[{"word":"tifl/o:nova"}]}"#, "TifloNova", 1, false),
        (r#"{"entries":[{"word":"tifl/o:nov*"}]}"#, "tiflonovom", 1, false),
        (r#"{"entries":[{"verb":"tiflon/ov=ia"}]}"#, "tiflonoviti", 2, true),
        (r#"{"entries":[{"verb":"tiflon/ov=ia"}]}"#, "TifloNoviti", 2, true),
    ];
    for &(json, text, stress, verb) in cases {
        let mut frontend = Frontend::new(Language::Croatian);
        let (lexicon, report) = UserLexicon::parse(json);
        assert_eq!(report.rejected, 0, "{json}");
        assert_eq!(if verb { report.verbs } else { report.words }, 1, "{json}");
        frontend.set_user_lexicon(Some(lexicon));
        let utterance = read(&frontend, text);
        assert_eq!(stressed_syllables(&utterance), [stress], "{text}");
        require_one_word(&utterance, text);
    }
    // A foreign-language override must not suppress the Croatian preference.
    let mut frontend = Frontend::new(Language::Croatian);
    frontend.set_user_lexicon(user(r#"{"entries":[{"word":"tifl/o:nova","language":"sr"}]}"#));
    assert_eq!(stressed_syllables(&read(&frontend, "tiflonova")), [0, 2]);
    frontend.set_user_lexicon(None);
    assert_eq!(stressed_syllables(&read(&frontend, "tiflonova")), [0, 2]);
}

/// The tiflo pronunciation preference stays Croatian
#[test]
fn the_tiflo_pronunciation_preference_stays_croatian() {
    for language in [Language::Serbian, Language::Bosnian] {
        let frontend = Frontend::new(language);
        let brand = read(&frontend, "tiflonova");
        assert_eq!(stressed_syllables(&brand), [1]);
        assert!(nuclei(&brand)[1].is_long);
        require_one_word(&brand, "tiflonova");
        let term = read(&frontend, "tiflologija");
        assert_eq!(stressed_syllables(&term), [2]);
        require_one_word(&term, "tiflologija");
    }
    let frontend = Frontend::new(Language::Croatian);
    let prefix = read(&frontend, "tiflo");
    assert_eq!(stressed_syllables(&prefix), [0]);
    assert_eq!(prefix.syllable_count, 2);
    require_one_word(&prefix, "tiflo");
}

/// Collected Croatian words follow checked dictionary accents
#[test]
fn collected_croatian_words_follow_checked_dictionary_accents() {
    // Školski rječnik/HJP headwords and paradigms, not suffix predictions.
    // Source disagreements, homographs and the listener-preferred evènata
    // pronunciation are documented in docs/formant.md.
    #[rustfmt::skip]
    let cases: &[(&str, i32, usize, Accent, bool)] = &[
        ("slavlje", 2, 0, F, true), ("festivala", 4, 2, R, true),
        ("evenata", 4, 1, R, false), ("Evenata", 4, 1, R, false),
        ("razloga", 3, 0, R, true), ("zahvalnost", 3, 1, R, true),
        ("nasmiješeno", 4, 0, R, false), ("putovanja", 4, 2, R, true),
        ("krafna", 2, 0, R, true), ("glas", 1, 0, F, true),
        ("izvela", 3, 0, R, false), ("odvela", 3, 0, R, false),
        ("strpljenje", 3, 1, R, true), ("zavela", 3, 0, R, false),
        ("dovela", 3, 0, R, false), ("pažnja", 2, 0, R, false),
        ("hrabrost", 2, 0, R, true), ("samopouzdanje", 6, 4, R, true),
        ("samodopadan", 5, 2, R, false), ("samodopadnost", 5, 2, R, false),
        ("nabava", 3, 0, R, false), ("konobar", 3, 0, R, false),
        ("namigne", 3, 0, R, false), ("ukraden", 3, 1, R, false),
        ("obijene", 4, 1, R, false), ("ubijene", 4, 1, R, false),
        ("razbijene", 4, 1, R, false),
        ("počinitelji", 5, 1, R, false), ("Špansko", 2, 0, R, true),
        ("špansko", 2, 0, R, true), ("društvo", 2, 0, R, true),
        ("rečeno", 3, 0, R, false), ("problema", 3, 1, R, true),
        ("travnjaku", 3, 0, F, true), ("terenu", 3, 1, R, true),
        ("trener", 2, 0, R, true), ("smisao", 3, 0, F, true),
        ("stanka", 2, 0, F, true), ("stvari", 2, 0, F, true),
        ("neću", 2, 0, R, true), ("prilika", 3, 0, R, true),
        ("komponenti", 4, 1, R, false), ("kolačić", 3, 1, R, false),
        ("kolačići", 4, 1, R, false), ("rep", 1, 0, F, true),
        ("krokodil", 3, 1, R, false), ("zec", 1, 0, F, true),
        ("vuk", 1, 0, F, true), ("žirafa", 3, 1, R, false),
        ("pantera", 3, 1, R, true),
        // Paradigm contrasts: do not extend one form's accent blindly.
        ("festival", 3, 1, R, false), ("festivalima", 5, 2, R, true),
        ("problem", 2, 0, R, false), ("problemi", 3, 1, R, true),
        ("teren", 2, 0, R, false), ("tereni", 3, 1, R, true),
        ("glasovi", 3, 0, F, false), ("repovi", 3, 0, F, false),
        ("zečevi", 3, 0, F, false), ("vukovi", 3, 0, F, false),
        ("razlozima", 4, 0, R, true), ("travnjacima", 4, 0, F, true),
        ("strpljenjem", 3, 1, R, true), ("zahvalnošću", 4, 1, R, true),
        ("samodopadnima", 6, 2, R, false), ("počiniteljima", 6, 1, R, false),
        ("namignuti", 4, 0, R, false), ("namignuo", 4, 0, R, false),
        ("namigni", 3, 0, R, false), ("namignemo", 4, 0, R, false),
        ("izveli", 3, 0, R, false), ("odveo", 3, 0, R, false),
        ("zavelo", 3, 0, R, false), ("dovele", 3, 0, R, false),
        ("izveden", 3, 1, R, false), ("odvede", 3, 1, R, false),
        ("zavedi", 3, 1, R, false), ("dovedena", 4, 1, R, false),
        ("ukradenima", 5, 1, R, false), ("obijenima", 5, 1, R, false),
        ("ubijenog", 4, 1, R, false), ("rečenima", 4, 0, R, false),
        ("razbijen", 3, 1, R, false), ("razbijeno", 4, 1, R, false),
        ("razbijenima", 5, 1, R, false),
        ("Španskoga", 3, 0, R, true), ("Španskom", 2, 0, R, true),
        ("stanci", 2, 0, F, true), ("prilici", 3, 0, R, true),
        ("smisla", 2, 0, F, true), ("smislovi", 3, 0, R, false),
        ("stvarima", 3, 0, R, true), ("nećeš", 2, 0, F, true),
        ("nećemo", 3, 0, F, true), ("komponenta", 4, 2, R, false),
        ("komponenata", 5, 1, R, false), ("kolačićima", 5, 1, R, false),
        ("krokodila", 4, 1, R, false), ("panterama", 4, 1, R, true),
    ];
    let frontend = Frontend::new(Language::Croatian);
    for &(text, syllables, stress, tone, long_vowel) in cases {
        let utterance = read(&frontend, text);
        assert_eq!(utterance.syllable_count, syllables, "{text}");
        assert_eq!(stressed_syllables(&utterance), [stress], "{text}");
        let vowels = nuclei(&utterance);
        assert_eq!(vowels[stress].accent, tone, "{text}");
        assert_eq!(vowels[stress].is_long, long_vowel, "{text}");
        require_one_word(&utterance, text);
    }
    let events = nuclei(&read(&frontend, "evenata"));
    assert!(events.iter().all(|vowel| !vowel.is_long));
}

/// Collected nouns preserve dictionary tone length and inflection
#[test]
fn collected_nouns_preserve_dictionary_tone_length_and_inflection() {
    // Linguistic expectations from ŠR/HJP, including contrasts the generic
    // suffix rules cannot predict. Source disagreements: docs/formant.md.
    #[rustfmt::skip]
    let cases: &[(&str, i32, usize, Accent, bool)] = &[
        ("komarac", 3, 1, R, true), ("komarca", 3, 1, R, true),
        ("komarcima", 4, 1, R, true), ("komaraca", 4, 0, R, false),
        ("komarče", 3, 0, F, false),
        ("žalac", 2, 0, F, false), ("žalca", 2, 0, R, true),
        ("žalcima", 3, 0, R, true), ("žalaca", 3, 0, F, false),
        ("samac", 2, 0, R, true), ("samcem", 2, 0, R, true),
        ("samci", 2, 0, R, true), ("samaca", 3, 0, F, true),
        ("šarac", 2, 0, R, true), ("šarca", 2, 0, R, true),
        ("šarcima", 3, 0, R, true), ("šaraca", 3, 0, F, true),
        ("sokak", 2, 0, R, false), ("sokaka", 3, 1, R, true),
        ("sokaci", 3, 1, R, true), ("sokacima", 4, 1, R, true),
        ("planinarenje", 5, 2, R, true), ("planinarenjima", 6, 2, R, true),
        ("rimac", 2, 0, R, true), ("Rimac", 2, 0, R, true),
        ("rimca", 2, 0, R, true), ("Rimcem", 2, 0, R, true),
        ("Bare", 2, 0, R, true),
        ("općenito", 4, 1, R, false), ("općenitima", 5, 1, R, false),
        ("medijateka", 5, 3, R, true), ("medijateci", 5, 3, R, true),
        ("medijatekama", 6, 3, R, true),
        ("rotor", 2, 0, F, true), ("rotorima", 4, 0, F, true),
        ("aktivnost", 3, 1, R, true), ("aktivnošću", 4, 1, R, true),
        ("aktivnostima", 5, 1, R, true),
        ("element", 3, 1, R, false), ("elemenata", 5, 1, R, false),
        ("snimka", 2, 0, F, false), ("snimci", 2, 0, F, false),
        ("snimkama", 3, 0, F, false), ("snimaka", 3, 0, F, false),
        ("dodira", 3, 0, F, false), ("dodirima", 4, 0, F, false),
        ("naredba", 3, 0, R, true), ("naredaba", 4, 0, R, true),
        ("zvuk", 1, 0, F, true), ("zvuka", 2, 0, F, true),
        ("zvukovi", 3, 0, F, false), ("zvuci", 2, 0, F, true),
        ("glavni", 2, 0, R, false), ("glavnoga", 3, 0, R, false),
    ];
    let frontend = Frontend::new(Language::Croatian);
    for &(text, syllables, stress, tone, long_vowel) in cases {
        let utterance = read(&frontend, text);
        assert_eq!(utterance.syllable_count, syllables, "{text}");
        assert_eq!(stressed_syllables(&utterance), [stress], "{text}");
        let vowels = nuclei(&utterance);
        assert_eq!(vowels[stress].accent, tone, "{text}");
        assert_eq!(vowels[stress].is_long, long_vowel, "{text}");
        require_one_word(&utterance, text);
    }
    // Capitalization selects the name. Keep the ordinary lowercase and
    // all-capitals analyses separate, and do not capture related verbs.
    let name = nuclei(&read(&frontend, "Goran Bare"));
    assert!(name[2].stressed);
    assert_eq!(name[2].accent, R);
    assert!(name[2].is_long);
    assert!(!nuclei(&read(&frontend, "bare"))[0].is_long);
    assert!(!nuclei(&read(&frontend, "BARE"))[0].is_long);
    assert_eq!(stressed_syllables(&read(&frontend, "dodirati")), [1]);
    assert_eq!(stressed_syllables(&read(&frontend, "narediti")), [1]);
    assert_eq!(stressed_syllables(&read(&frontend, "elementaran")), [3]);
}

/// Case context selects Croatian accent homographs
#[test]
fn case_context_selects_croatian_accent_homographs() {
    #[rustfmt::skip]
    let cases: &[(&str, usize, Accent, bool)] = &[
        ("o vragu", 0, R, true), ("o crnom vragu", 0, R, true),
        ("k vragu", 0, F, true), ("tri vraga", 0, F, true),
        ("pet vraga", 0, R, true), ("pet crnih vraga", 0, R, true),
        ("u mraku", 0, R, true), ("po potpunom mraku", 0, R, true),
        ("prema mraku", 0, F, true), ("iz mraka", 0, F, true),
        ("na zraku", 0, R, true), ("na svježem jutarnjem zraku", 0, R, true),
        ("prema zraku", 0, F, true), ("bez zraka", 0, F, true),
        ("nema zraka", 0, F, true), ("svježim zrakom", 0, F, true),
        ("jedna zraka", 0, F, false), ("sunčeva zraka", 0, F, false),
        ("ovu zraku", 0, F, false), ("na lasersku zraku", 0, F, false),
        ("tom zrakom", 0, F, false), ("laserskom zrakom", 0, F, false),
        ("u toj zraci", 0, F, false), ("pet sunčevih zraka", 0, F, true),
        ("o vrȃgu", 0, F, true), ("pet vrȃga", 0, F, true),
        ("u mrȁku", 0, F, false), ("na zrȃku", 0, F, true),
        ("jedna zrȃka", 0, F, true),
        ("nova anketa", 1, R, false), ("dvije ankete", 1, R, false),
        ("pet anketa", 1, R, true), ("nekoliko anketa", 1, R, true),
        ("pet novih anketa", 1, R, true), ("pet ankèta", 1, R, false),
        ("nova vještina", 1, R, false), ("dvije vještine", 1, R, false),
        ("pet vještina", 1, R, true), ("nekoliko vještina", 1, R, true),
        ("mnogo novih vještina", 1, R, true), ("pet vještìna", 1, R, false),
        ("o glasu", 0, R, true), ("na dobrom glasu", 0, R, true),
        ("prema glasu", 0, F, true), ("po repu", 0, R, true),
        ("prema repu", 0, F, true), ("pet stvari", 0, R, true),
        ("mnogo stvari", 0, R, true), ("dvije stvari", 0, F, true),
        ("bez stvari", 0, F, true), ("o važnoj stvari", 0, R, true),
        ("gledam na stvari", 0, F, true), ("o komponenti", 2, R, false),
        ("prema novoj komponenti", 2, R, false), ("pet komponenti", 1, R, false),
        ("mnogo novih komponenti", 1, R, false),
        ("pet žirafa", 1, R, true), ("nekoliko pažnja", 0, R, true),
        ("nekoliko nabava", 0, F, false), ("pet društva", 0, R, true),
        ("pet vuka", 0, R, true), ("pet komponenta", 1, R, false),
        ("o zvuku", 0, R, true), ("o novom zvuku", 0, R, true),
        ("prema zvuku", 0, F, true), ("pet zvuka", 0, R, true),
        ("dvije snimke", 0, F, false), ("pet snimka", 0, F, true),
        // Explicit accents win over case cues.
        ("o glȃsu", 0, F, true), ("pet stvȃri", 0, F, true),
        ("o kompònenti", 1, R, false), ("pet žiràfa", 1, R, false),
        ("o zvȗku", 0, F, true), ("pet snȉmka", 0, F, false),
    ];
    let mut frontend = Frontend::new(Language::Croatian);
    for &(text, stress, tone, long_vowel) in cases {
        let utterance = read(&frontend, text);
        let vowels = word_nuclei(&utterance, last_word(&utterance));
        assert_eq!(stressed_count(&vowels), 1, "{text}");
        assert!(vowels[stress].stressed, "{text}");
        assert_eq!(vowels[stress].accent, tone, "{text}");
        assert_eq!(vowels[stress].is_long, long_vowel, "{text}");
    }
    frontend.set_user_lexicon(user(r#"{"entries":[{"word":"gl^a:su"},{"word":"komp/onenti"},{"word":"žir/afa"},{"word":"ank/eta"},{"word":"vješt/ina"},{"word":"vr^agu"},{"word":"zr^a:ka"},{"word":"zr^a:ku"}]}"#));
    assert_eq!(nuclei(&read(&frontend, "o vragu"))[1].accent, F);
    assert!(!nuclei(&read(&frontend, "o vragu"))[1].is_long);
    assert!(nuclei(&read(&frontend, "jedna zraka"))[2].is_long);
    assert_eq!(nuclei(&read(&frontend, "na zraku"))[1].accent, F);
    assert!(!nuclei(&read(&frontend, "pet anketa"))[2].is_long);
    assert!(!nuclei(&read(&frontend, "pet vještina"))[2].is_long);
    let voice = nuclei(&read(&frontend, "o glasu"));
    assert_eq!(voice[1].accent, F);
    let component = nuclei(&read(&frontend, "o komponenti"));
    assert!(component[2].stressed);
    let giraffes = nuclei(&read(&frontend, "pet žirafa"));
    assert!(!giraffes[2].is_long);
}

/// Croatian siguran and direktno keep their selected adjective accents
#[test]
fn croatian_siguran_and_direktno_keep_their_selected_adjective_accents() {
    // ŠR/HJP: sȉgūran and its positive forms, but sigùrnijī and sigúrnōst.
    // Croatian's lexicon omits post-accent length; stressed length is
    // checked independently so a broad sigur- stem cannot hide the contrast.
    #[rustfmt::skip]
    let cases: &[(&str, usize, Accent, bool)] = &[
        ("siguran", 0, F, false), ("Siguran", 0, F, false),
        ("SIGURAN", 0, F, false), ("sigurna", 0, F, false),
        ("sigurno", 0, F, false), ("sigurni", 0, F, false),
        ("sigurne", 0, F, false), ("sigurnu", 0, F, false),
        ("sigurnog", 0, F, false), ("sigurnoga", 0, F, false),
        ("sigurnome", 0, F, false), ("sigurnomu", 0, F, false),
        ("sigurnoj", 0, F, false), ("sigurnih", 0, F, false),
        ("sigurnim", 0, F, false), ("sigurnima", 0, F, false),
        ("sigurniji", 1, R, false), ("sigurnije", 1, R, false),
        ("sigurnijima", 1, R, false),
        ("sigurnost", 1, R, true), ("sigurnošću", 1, R, true),
        ("sigurnostima", 1, R, true),
        // User's HJP-listed dirèktno variant, including positive adjective
        // forms; the comparative already has the same short rising e.
        ("direktno", 1, R, false), ("Direktno", 1, R, false),
        ("DIREKTNO", 1, R, false), ("direktan", 1, R, false),
        ("direktna", 1, R, false), ("direktni", 1, R, false),
        ("direktne", 1, R, false), ("direktnu", 1, R, false),
        ("direktnog", 1, R, false), ("direktnoga", 1, R, false),
        ("direktnom", 1, R, false), ("direktnome", 1, R, false),
        ("direktnomu", 1, R, false), ("direktnoj", 1, R, false),
        ("direktnih", 1, R, false), ("direktnim", 1, R, false),
        ("direktnima", 1, R, false), ("direktniji", 1, R, false),
        ("direktnije", 1, R, false),
    ];
    let mut frontend = Frontend::new(Language::Croatian);
    for &(text, stress, tone, long_vowel) in cases {
        let utterance = read(&frontend, text);
        assert_eq!(stressed_syllables(&utterance), [stress], "{text}");
        let vowel = nuclei(&utterance)[stress];
        assert_eq!(vowel.accent, tone, "{text}");
        assert_eq!(vowel.is_long, long_vowel, "{text}");
    }
    assert_eq!(nuclei(&read(&frontend, "síguran"))[0].accent, R);
    assert!(nuclei(&read(&frontend, "síguran"))[0].is_long);
    assert_eq!(stressed_syllables(&read(&frontend, "dìrektno")), [0]);
    frontend.set_user_lexicon(user(r#"{"entries":[{"word":"sig/u:ran"}]}"#));
    let custom = nuclei(&read(&frontend, "siguran"));
    assert!(custom[1].stressed);
    assert_eq!(custom[1].accent, R);
    assert!(custom[1].is_long);
    frontend.set_user_lexicon(user(r#"{"entries":[{"word":"d/irektno"}]}"#));
    assert_eq!(stressed_syllables(&read(&frontend, "direktno")), [0]);
    for language in [Language::Serbian, Language::Bosnian] {
        let other = Frontend::new(language);
        assert_eq!(stressed_syllables(&read(&other, "direktno")), [0]);
    }
}

/// Collected household words retain dictionary accents across forms
#[test]
fn collected_household_words_retain_dictionary_accents_across_forms() {
    // Školski rječnik principal forms, with HJP for bȃjan/bȃjno.
    // See docs/formant.md for source disagreements and ambiguous spellings.
    #[rustfmt::skip]
    let cases: &[(&str, usize, Accent, bool)] = &[
        ("kontejnera", 1, R, false), ("kontejner", 1, R, false),
        ("kontejneru", 1, R, false), ("kontejnerom", 1, R, false),
        ("kontejneri", 1, R, false), ("kontejnere", 1, R, false),
        ("kontejnerima", 1, R, false),
        ("komoda", 1, R, false), ("komode", 1, R, false),
        ("komodi", 1, R, false), ("komodu", 1, R, false),
        ("komodom", 1, R, false), ("komodama", 1, R, false),
        ("stan", 0, F, true), ("stana", 0, F, true),
        ("stanu", 0, F, true), ("stanom", 0, F, true),
        ("stanovi", 0, F, false), ("stanova", 0, F, false),
        ("stanove", 0, F, false), ("stanovima", 0, F, false),
        ("ispuste", 0, R, false), ("ispust", 0, R, false),
        ("ispusta", 0, R, false), ("ispustom", 0, R, false),
        ("ispustima", 0, R, false), ("ispustim", 0, R, false),
        ("ispustiš", 0, R, false), ("ispustimo", 0, R, false),
        ("ispustiti", 1, R, false), ("ispustio", 1, R, false),
        ("ispustila", 1, R, false), ("ispusti", 1, R, false),
        ("ispušten", 0, R, false), ("ispuštena", 0, R, false),
        ("sastav", 0, R, true), ("sastava", 0, R, true),
        ("sastavu", 0, R, true), ("sastavom", 0, R, true),
        ("sastave", 0, R, true), ("sastavima", 0, R, true),
        ("rastava", 0, R, false), ("rastave", 0, R, false),
        ("rastavi", 0, R, false), ("rastavom", 0, R, false),
        ("rastavama", 0, R, false),
        ("tajna", 0, F, true), ("tajnama", 0, F, true),
        ("tajnoga", 0, F, true), ("tajnomu", 0, F, true),
        ("sjajna", 0, R, true), ("sjajan", 0, R, true),
        ("sjajno", 0, R, true), ("sjajni", 0, F, true),
        ("sjajnoga", 0, F, true), ("sjajnome", 0, F, true),
        ("sjajniji", 0, R, false), ("sjajnijima", 0, R, false),
        ("bajna", 0, F, true), ("bajan", 0, F, true),
        ("bajno", 0, F, true), ("bajnoga", 0, F, true),
        ("bajnima", 0, F, true),
        ("trajna", 0, R, true), ("trajan", 0, F, true),
        ("trajno", 0, F, true), ("trajni", 0, F, true),
        ("trajnoga", 0, F, true), ("trajnome", 0, F, true),
        ("trajniji", 0, R, false), ("trajnijima", 0, R, false),
        ("jajna", 0, F, true), ("jajni", 0, F, true),
        ("jajnoga", 0, F, true), ("jajnima", 0, F, true),
        ("jaje", 0, R, true), ("jaja", 0, R, true),
        ("jaju", 0, R, true), ("jajem", 0, R, true),
        ("jajima", 0, R, true), ("jajeta", 0, R, false),
        ("jajetu", 0, R, false), ("jajetom", 0, R, false),
        ("Kontejnera", 1, R, false), ("KOMODA", 1, R, false),
        ("Sastav", 0, R, true), ("TRAJNA", 0, R, true),
    ];
    let frontend = Frontend::new(Language::Croatian);
    for &(text, stress, tone, long_vowel) in cases {
        let utterance = read(&frontend, text);
        assert_eq!(stressed_syllables(&utterance), [stress], "{text}");
        let vowel = nuclei(&utterance)[stress];
        assert_eq!(vowel.accent, tone, "{text}");
        assert_eq!(vowel.is_long, long_vowel, "{text}");
        require_one_word(&utterance, text);
    }
}

/// Household noun and adjective homographs keep context and overrides
#[test]
fn household_noun_and_adjective_homographs_keep_context_and_overrides() {
    #[rustfmt::skip]
    let cases: &[(&str, usize, Accent, bool)] = &[
        ("u stanu", 0, R, true), ("u novom stanu", 0, R, true),
        ("prema stanu", 0, F, true),
        ("pet komoda", 1, R, true), ("mnogo komoda", 1, R, true),
        ("nova komoda", 1, R, false), ("pet rastava", 0, F, false),
        ("jedna rastava", 0, R, false), ("glazbeni sastavi", 0, R, true),
        ("Sastavi", 0, R, false), ("sastaviti", 0, R, false),
        ("sastavio", 0, R, false),
        ("oni ispuste", 0, R, false), ("kroz ispuste", 0, R, false),
        ("u stȃnu", 0, F, true), ("pet komòda", 1, R, false),
        ("pet ràstava", 0, R, false),
        ("sjȃjna", 0, F, true), ("trȃjna", 0, F, true),
        ("komóda", 1, R, true), ("rȃstava", 0, F, true),
    ];
    let mut frontend = Frontend::new(Language::Croatian);
    for &(text, stress, tone, long_vowel) in cases {
        let utterance = read(&frontend, text);
        let vowels = word_nuclei(&utterance, last_word(&utterance));
        assert!(vowels[stress].stressed, "{text}");
        assert_eq!(vowels[stress].accent, tone, "{text}");
        assert_eq!(vowels[stress].is_long, long_vowel, "{text}");
    }
    // The noun's rare vocative stȃne must not lengthen the verb stȁne.
    assert!(!nuclei(&read(&frontend, "stane"))[0].is_long);
    frontend.set_user_lexicon(user(r#"{"entries":[{"word":"st^a:nu"},{"word":"kom/oda"},{"word":"r/a:stava"}]}"#));
    assert_eq!(nuclei(&read(&frontend, "u stanu"))[1].accent, F);
    assert!(!nuclei(&read(&frontend, "pet komoda"))[2].is_long);
    assert!(nuclei(&read(&frontend, "pet rastava"))[1].is_long);
}

/// Presvlacenje keeps the root accent across noun cases
#[test]
fn presvlacenje_keeps_the_root_accent_across_noun_cases() {
    // ŠR presvláčēnje, G presvláčēnja: long rising a, not generic -énje.
    // Croatian output omits post-accent length, so the following e stays short.
    let frontend = Frontend::new(Language::Croatian);
    let forms: &[&str] = &[
        "presvlačenje",
        "presvlačenja",
        "presvlačenju",
        "presvlačenjem",
        "presvlačenjima",
        "Presvlačenje",
        "PRESVLAČENJE",
    ];
    for &text in forms {
        let utterance = read(&frontend, text);
        assert_eq!(stressed_syllables(&utterance), [1], "{text}");
        let vowels = nuclei(&utterance);
        assert_eq!(vowels[1].accent, R, "{text}");
        assert!(vowels[1].is_long, "{text}");
        assert!(!vowels[2].is_long, "{text}");
    }
}

/// Croatian ivac nouns keep their dictionary paradigms
#[test]
fn croatian_ivac_nouns_keep_their_dictionary_paradigms() {
    // ŠR where available, HJP otherwise. The genitive plural is not
    // uniformly stressed like the lemma; -ljivac/-njivac are separate.
    // The stem, its stress and length, and those of the genitive plural
    // with its tone.
    #[rustfmt::skip]
    let families: &[(&str, usize, bool, usize, Accent, bool)] = &[
        ("pozitiv", 2, true, 1, R, false),
        ("negativ", 2, true, 1, R, false),
        ("depresiv", 2, true, 1, R, false),
        ("emotiv", 2, true, 1, R, false),
        ("agresiv", 2, true, 1, R, false),
        ("kreativ", 2, true, 1, R, false),
        ("primitiv", 2, true, 1, R, false),
        ("operativ", 3, true, 2, R, false),
        ("konzervativ", 3, true, 2, R, false),
        ("rekreativ", 3, true, 2, R, false),
        ("naiv", 1, true, 0, R, false),
        ("kriv", 0, true, 0, F, true),
        ("siv", 0, true, 0, F, true),
        ("živ", 0, true, 0, F, true),
        ("sumnjiv", 0, false, 0, R, false),
        ("plašljiv", 0, false, 0, R, false),
        ("šutljiv", 0, false, 0, R, false),
        ("stidljiv", 0, false, 0, R, false),
        ("sramežljiv", 1, false, 1, R, false),
        ("zavidljiv", 1, false, 1, R, false),
    ];
    let frontend = Frontend::new(Language::Croatian);
    for &(stem, stress, long_vowel, plural_stress, plural_tone, plural_long) in families {
        for ending in ["ac", "ca", "cu", "cem", "ci", "ce", "cima", "aca"] {
            let text = format!("{stem}{ending}");
            let genitive_plural = ending == "aca";
            let stress = if genitive_plural { plural_stress } else { stress };
            let utterance = read(&frontend, &text);
            assert_eq!(stressed_syllables(&utterance), [stress], "{text}");
            let vowel = nuclei(&utterance)[stress];
            assert_eq!(vowel.accent, if genitive_plural { plural_tone } else { R }, "{text}");
            assert_eq!(vowel.is_long, if genitive_plural { plural_long } else { long_vowel }, "{text}");
            require_one_word(&utterance, &text);
        }
    }
    // Only vocatives explicitly accented by ŠR/HJP are prescribed here.
    #[rustfmt::skip]
    let cases: &[(&str, usize, Accent, bool)] = &[
        ("pozitivče", 2, R, true), ("depresivče", 0, F, false),
        ("kreativče", 1, R, false), ("primitivče", 0, F, false),
        ("konzervativče", 0, F, false), ("naivče", 0, F, false),
        ("krivče", 0, F, true), ("sivče", 0, R, true),
        ("sumnjivče", 0, R, false), ("plašljivče", 0, R, false),
        ("šutljivče", 0, R, false), ("stidljivče", 0, R, false),
        ("sramežljivče", 1, R, false),
        ("Pozitivac", 2, R, true), ("NEGATIVAC", 2, R, true),
        ("Depresivaca", 1, R, false), ("EMOTIVACA", 1, R, false),
    ];
    for &(text, stress, tone, long_vowel) in cases {
        let utterance = read(&frontend, text);
        assert_eq!(stressed_syllables(&utterance), [stress], "{text}");
        let vowel = nuclei(&utterance)[stress];
        assert_eq!(vowel.accent, tone, "{text}");
        assert_eq!(vowel.is_long, long_vowel, "{text}");
    }
}

/// Ivac noun accents respect written and user overrides
#[test]
fn ivac_noun_accents_respect_written_and_user_overrides() {
    let mut frontend = Frontend::new(Language::Croatian);
    let mut vowels = nuclei(&read(&frontend, "pȍzitivac"));
    assert!(vowels[0].stressed);
    assert_eq!(vowels[0].accent, F);
    assert!(!vowels[0].is_long);
    vowels = nuclei(&read(&frontend, "žìvac"));
    assert_eq!(vowels[0].accent, R);
    assert!(!vowels[0].is_long);
    frontend.set_user_lexicon(user(r#"{"entries":[{"word":"poz/itivac"},{"word":"emot/i:vaca"}]}"#));
    assert_eq!(stressed_syllables(&read(&frontend, "pozitivac")), [1]);
    vowels = nuclei(&read(&frontend, "emotivaca"));
    assert!(vowels[2].stressed);
    assert_eq!(vowels[2].accent, R);
    assert!(vowels[2].is_long);
}

/// Biti participles keep hiatus without breaking the jat reflex
#[test]
fn biti_participles_keep_hiatus_without_breaking_the_jat_reflex() {
    for language in [Language::Croatian, Language::Serbian, Language::Bosnian] {
        let frontend = Frontend::new(language);
        for text in ["obijene", "ubijene", "razbijene", "dobijene", "neubijene"] {
            let vowels = nuclei(&read(&frontend, text));
            assert_eq!(vowels.len(), if text == "neubijene" { 5 } else { 4 }, "{text}");
        }
        for text in ["bijel", "bijedan", "pobijeljen", "obaviješten"] {
            let vowels = nuclei(&read(&frontend, text));
            assert!(vowels.iter().any(|vowel| vowel.is_long), "{text}");
        }
    }
    let frontend = Frontend::new(Language::Croatian);
    let vowels = nuclei(&read(&frontend, "obijene"));
    assert_eq!(vowels[1].ph, Ph::I);
    assert_eq!(vowels[2].ph, Ph::E);
    assert!(!vowels[1].is_long);
    assert!(!vowels[2].is_long);
}

/// Testament to nogomet families follow the checked Croatian paradigms
#[test]
fn testament_to_nogomet_families_follow_the_checked_croatian_paradigms() {
    // Školski rječnik principal forms; HJP for pediker/pedikerski and
    // nogometaški. ŠR is preferred for bògatstvo and medìcīnskī.
    // The Croatian voice omits unstressed lengths, including G plural.
    #[rustfmt::skip]
    let families: &[(&[&str], usize, Accent, bool)] = &[
        (&["testament", "testamenta", "testamentu", "testamentom",
          "testamenti", "testamente", "testamentima", "testamenata"], 1, R, false),
        (&["bogatstvo", "bogatstva", "bogatstvu", "bogatstvom",
          "bogatstvima", "bogatstava"], 0, R, false),
        (&["sažetak", "sažetka", "sažetku", "sažetkom", "sažetke",
          "sažeci", "sažecima", "sažetci", "sažetcima"], 1, R, true),
        (&["sažetaka"], 0, R, false),
        (&["salon"], 0, R, false),
        (&["salona", "salonu", "salonom", "saloni", "salone", "salonima"], 1, R, true),
        (&["salonski", "salonska", "salonskoj", "salonskima"], 0, R, false),
        (&["pediker"], 1, R, false),
        (&["pedikera", "pedikeru", "pedikerom", "pedikeri", "pedikere", "pedikerima"], 2, R, true),
        (&["pedikerski", "pedikerska", "pedikersko", "pedikerske",
          "pedikersku", "pedikerskog", "pedikerskoga", "pedikerskom",
          "pedikerskome", "pedikerskomu", "pedikerskoj", "pedikerskih",
          "pedikerskim", "pedikerskima"], 1, R, false),
        (&["poklanjati", "poklanjam", "poklanjaš", "poklanja", "poklanjamo",
          "poklanjate", "poklanjaju", "poklanjaj", "poklanjajte",
          "poklanjao", "poklanjala", "poklanjali", "poklanjan",
          "poklanjana", "poklanjanje", "poklanjanja", "poklanjajući"], 0, R, false),
        (&["obzir", "obzira", "obziru", "obzirom", "obziri", "obzire", "obzirima"], 0, F, false),
        (&["medicinski", "medicinska", "medicinsko", "medicinske", "medicinsku",
          "medicinskog", "medicinskoga", "medicinskom", "medicinskome",
          "medicinskomu", "medicinskoj", "medicinskih", "medicinskim", "medicinskima"], 1, R, false),
        // The adjective must not change the noun medicína.
        (&["medicina", "medicine", "medicinu", "medicinom"], 2, R, true),
        (&["nogomet", "nogometa", "nogometu", "nogometom", "nogomete",
          "nogometi", "nogometima", "Nogomet", "NOGOMET"], 0, F, false),
        (&["nogometni", "nogometna", "nogometno", "nogometne", "nogometnu",
          "nogometnog", "nogometnoga", "nogometnom", "nogometnome",
          "nogometnomu", "nogometnoj", "nogometnih", "nogometnim", "nogometnima"], 0, F, false),
        // Related player nouns and their adjective have different accents.
        (&["nogometaš"], 2, R, false),
        (&["nogometaša", "nogometašu", "nogometašem", "nogometaši",
          "nogometaše", "nogometašima"], 3, R, true),
        (&["nogometaški", "nogometaška", "nogometaškoj", "nogometaškima"], 2, R, false),
        (&["nogometašica", "nogometašice", "nogometašici", "nogometašicu",
          "nogometašicom", "nogometašicama"], 3, R, false),
    ];
    let frontend = Frontend::new(Language::Croatian);
    for &(words, stress, tone, long_vowel) in families {
        for &text in words {
            let vowels = nuclei(&read(&frontend, text));
            assert_eq!(stressed_count(&vowels), 1, "{text}");
            assert!(vowels[stress].stressed, "{text}");
            assert_eq!(vowels[stress].accent, tone, "{text}");
            assert_eq!(vowels[stress].is_long, long_vowel, "{text}");
            for vowel in &vowels {
                assert!(vowel.stressed || !vowel.is_long, "{text}");
            }
        }
    }
}

/// Testament to nogomet accents survive context and allow overrides
#[test]
fn testament_to_nogomet_accents_survive_context_and_allow_overrides() {
    let mut frontend = Frontend::new(Language::Croatian);
    // In ordinary phrases, compare each target's nuclei with its isolated form.
    #[rustfmt::skip]
    let cases: &[(&str, &str, u16)] = &[
        ("s obzirom na", "obzirom", 1),
        ("ja poklanjam knjigu", "poklanjam", 1),
        ("medicinska sestra", "medicinska", 0),
        ("nogometna lopta", "nogometna", 0),
        ("iz salona", "salona", 1),
        ("nekoliko sažetaka", "sažetaka", 1),
    ];
    for &(phrase, word, index) in cases {
        let in_phrase = word_nuclei(&read(&frontend, phrase), index);
        let alone = nuclei(&read(&frontend, word));
        assert_eq!(in_phrase.len(), alone.len(), "{phrase}");
        for (inside, single) in in_phrase.iter().zip(&alone) {
            assert_eq!(inside.stressed, single.stressed, "{phrase}");
            assert_eq!(inside.accent, single.accent, "{phrase}");
            assert_eq!(inside.is_long, single.is_long, "{phrase}");
        }
    }
    // HJP's alternatives and the vocatives can still be requested explicitly.
    assert!(nuclei(&read(&frontend, "bogàtstvo"))[1].stressed);
    assert_eq!(nuclei(&read(&frontend, "mȅdicinska"))[0].accent, F);
    assert_eq!(nuclei(&read(&frontend, "pȅdikeru"))[0].accent, F);
    assert_eq!(nuclei(&read(&frontend, "nȍgometašu"))[0].accent, F);
    frontend
        .set_user_lexicon(user(r#"{"entries":[{"word":"bog/atstvo"},{"word":"m^edicinska"},{"verb":"pokl/a:nj=a"}]}"#));
    assert!(nuclei(&read(&frontend, "bogatstvo"))[1].stressed);
    assert_eq!(nuclei(&read(&frontend, "medicinska"))[0].accent, F);
    assert!(nuclei(&read(&frontend, "poklanjam"))[1].is_long);
    assert!(nuclei(&read(&frontend, "pòklanjam"))[0].stressed);
}

/// Collected Croatian accents preserve stress tone and vowel quantity
#[test]
fn collected_croatian_accents_preserve_stress_tone_and_vowel_quantity() {
    // Independently checked against Školski rječnik/HJP principal forms.
    // mnom is the user's short-vowel preference; godišnjakinja is the
    // bare age component, by analogy with attested petogodišnjakinja.
    #[rustfmt::skip]
    let cases: &[(&str, usize, Accent, bool)] = &[
        ("stanovnik", 1, R, false), ("stanovnika", 2, R, true),
        ("stanovniku", 2, R, true), ("stanovnici", 2, R, true),
        ("stanovnicima", 2, R, true), ("stanovniče", 0, F, false),
        ("stanovnica", 1, R, false), ("stanovnicama", 1, R, false),
        ("stanovništvo", 2, R, true),
        ("stanar", 0, R, false), ("stanara", 1, R, true),
        ("stanaru", 1, R, true), ("stanare", 1, R, true),
        ("stanarima", 1, R, true),
        ("pretpostave", 1, R, false), ("pretpostaviti", 1, R, false),
        ("pretpostavim", 1, R, false), ("pretpostavio", 1, R, false),
        ("pretpostavljena", 1, R, false),
        ("tu", 0, F, true), ("često", 0, F, true), ("češće", 0, F, false),
        ("magazin", 0, F, false), ("magazini", 0, F, false),
        ("magazinima", 0, F, false), ("Magazin", 0, F, false),
        ("nebesa", 1, R, false), ("nebesima", 1, R, false),
        ("događaju", 1, R, true), ("događati", 1, R, true),
        ("događao", 1, R, true), ("događa", 0, R, false),
        ("događaj", 0, F, false), ("događaji", 0, F, false),
        ("pad", 0, F, true), ("padu", 0, F, true),
        ("padovi", 0, F, false), ("padovima", 0, F, false),
        ("pada", 0, F, false), ("padala", 0, F, false),
        ("osjećam", 0, R, false), ("osjećaju", 0, R, false),
        ("osjećala", 0, R, false), ("osjećaj", 0, F, false),
        ("ostavio", 0, R, false), ("ostaviti", 0, R, false),
        ("ostavila", 0, R, false), ("ostavljen", 0, R, false),
        ("mnom", 0, F, false),
        ("radnik", 0, F, true), ("radnika", 0, F, true),
        ("radnici", 0, F, true), ("radnicima", 0, F, true),
        ("radniče", 0, F, true), ("radnica", 0, F, true),
        ("radnicama", 0, F, true),
        ("dokument", 1, R, false), ("dokumenti", 1, R, false),
        ("dokumentima", 1, R, false), ("dokumenata", 1, R, false),
        ("dokumentarac", 3, R, true),
        ("pokušala", 0, R, false), ("pokušati", 0, R, false),
        ("pokušao", 0, R, false), ("pokušaju", 0, R, false),
        ("zavod", 0, R, true), ("zavodima", 0, R, true),
        ("imovina", 1, R, false), ("imovinom", 1, R, false),
        ("imovinama", 1, R, false),
        ("susretao", 0, R, false), ("susretati", 0, R, false),
        ("susretala", 0, R, false), ("susrećem", 0, R, false),
        ("mirovina", 1, R, false), ("mirovini", 1, R, false),
        ("mirovinama", 1, R, false),
        ("ministarstvo", 2, R, true), ("ministarstva", 2, R, true),
        ("ministarstvima", 2, R, true), ("ministarstava", 1, R, false),
        ("navod", 0, R, true), ("navoda", 0, R, true),
        ("navodima", 0, R, true),
        ("godišnjak", 0, R, false), ("godišnjaka", 0, R, false),
        ("godišnjacima", 0, R, false), ("godišnjakinja", 2, R, false),
        ("godišnjakinjama", 2, R, false),
        ("petogodišnjak", 2, R, false), ("petogodišnjakinja", 4, R, false),
        ("dvadesetogodišnjakinjama", 6, R, false),
        ("stablo", 0, R, false), ("stabla", 0, R, false),
        ("stablu", 0, R, false), ("stablom", 0, R, false),
        ("stablima", 0, R, false), ("stabala", 1, R, true),
        ("zelenilo", 1, R, false), ("zelenila", 1, R, false),
        ("zelenilu", 1, R, false), ("zelenilom", 1, R, false),
        ("zelenilima", 1, R, false),
        ("tobogan", 0, F, false), ("tobogana", 0, F, false),
        ("toboganu", 0, F, false), ("toboganom", 0, F, false),
        ("tobogani", 0, F, false), ("tobogane", 0, F, false),
        ("toboganima", 0, F, false), ("Tobogan", 0, F, false),
    ];
    let frontend = Frontend::new(Language::Croatian);
    for &(text, stress, tone, long_vowel) in cases {
        let vowels = nuclei(&read(&frontend, text));
        assert_eq!(stressed_count(&vowels), 1, "{text}");
        assert!(vowels[stress].stressed, "{text}");
        assert_eq!(vowels[stress].accent, tone, "{text}");
        assert_eq!(vowels[stress].is_long, long_vowel, "{text}");
        for vowel in &vowels {
            assert!(vowel.stressed || !vowel.is_long, "{text}");
        }
    }
}

/// Collected homographs use nearby grammatical cues
#[test]
fn collected_homographs_use_nearby_grammatical_cues() {
    #[rustfmt::skip]
    let cases: &[(&str, u16, usize, Accent, bool)] = &[
        ("Tu sam", 0, 0, F, true), ("sa mnom", 1, 0, F, false),
        ("Događaju se promjene", 0, 1, R, true),
        ("Promjene se često događaju", 3, 1, R, true),
        ("Promjene se uglavnom događaju", 3, 1, R, true),
        ("o događaju", 1, 0, F, false),
        ("o tom novom događaju se govori", 3, 0, F, false),
        ("prema događaju", 1, 0, F, false),
        ("tom događaju", 1, 0, F, false),
        ("Oni osjećaju bol", 1, 0, R, false),
        ("Potom osjećaju bol", 1, 0, R, false),
        ("o neobičnom osjećaju", 2, 0, F, false),
        ("Kiša pada", 1, 0, F, false),
        ("nakon pada", 1, 0, F, true),
        ("zbog naglog pada", 2, 0, F, true),
        ("o padu", 1, 0, R, true),
        ("o naglom padu", 2, 0, R, true),
        ("u nebesa", 1, 1, R, false),
        ("s nebesa", 1, 1, R, true),
        ("iz visokih nebesa", 2, 1, R, true),
        ("zgrada ministarstva", 1, 2, R, true),
        ("dva ministarstva", 1, 2, R, true),
        ("pet ministarstva", 1, 1, R, false),
        ("nekoliko ministarstava", 1, 1, R, false),
        ("drugih ministarstva", 1, 1, R, false),
        ("citirani navodi", 1, 0, R, true),
        ("grana stabla", 1, 0, R, false),
        ("dva stabla", 1, 0, R, false),
        ("pet stabla", 1, 0, R, true),
        ("nekoliko stabala", 1, 1, R, true),
        ("visokih stabla", 1, 0, R, true),
        ("pet stàbla", 1, 0, R, false),
        ("Stáblo", 0, 0, R, true),
    ];
    let frontend = Frontend::new(Language::Croatian);
    for &(text, word, stress, tone, long_vowel) in cases {
        let vowels = word_nuclei(&read(&frontend, text), word);
        assert_eq!(stressed_count(&vowels), 1, "{text}");
        assert!(vowels[stress].stressed, "{text}");
        assert_eq!(vowels[stress].accent, tone, "{text}");
        assert_eq!(vowels[stress].is_long, long_vowel, "{text}");
    }
}

/// Collected accent corrections respect explicit and user choices
#[test]
fn collected_accent_corrections_respect_explicit_and_user_choices() {
    let mut frontend = Frontend::new(Language::Croatian);
    assert!(nuclei(&read(&frontend, "mnȏm"))[0].is_long);
    assert!(!nuclei(&read(&frontend, "tȕ"))[0].is_long);
    assert!(nuclei(&read(&frontend, "o dogáđaju"))[2].is_long);
    assert!(!nuclei(&read(&frontend, "s nebèsa"))[1].is_long);
    assert!(nuclei(&read(&frontend, "pet ministárstva"))[3].is_long);
    assert!(!nuclei(&read(&frontend, "nakon pȁda"))[2].is_long);
    assert_eq!(nuclei(&read(&frontend, "o pȃdu"))[1].accent, F);
    frontend.set_user_lexicon(user(r#"{"entries":[{"word":"mn^o:m"},{"word":"dog/a:đaju"},{"word":"neb/esa"},{"word":"minist/a:rstva"},{"word":"p^ada"},{"word":"st/abla"}]}"#));
    assert!(nuclei(&read(&frontend, "sa mnom"))[1].is_long);
    assert!(nuclei(&read(&frontend, "o događaju"))[2].is_long);
    assert!(!nuclei(&read(&frontend, "s nebesa"))[1].is_long);
    assert!(nuclei(&read(&frontend, "pet ministarstva"))[3].is_long);
    assert!(!nuclei(&read(&frontend, "nakon pada"))[2].is_long);
    assert!(!nuclei(&read(&frontend, "pet stabla"))[1].is_long);
    // The requested short mnom applies only to Croatian.
    assert!(nuclei(&read(&Frontend::new(Language::Serbian), "mnom"))[0].is_long);
    assert!(nuclei(&read(&Frontend::new(Language::Bosnian), "mnom"))[0].is_long);
}

/// A clause read with letter sounds, as text.
fn sounds(frontend: &Frontend, text: &str) -> String {
    phones_of(&frontend.process(&chars(text), Punctuation::Period, true))
}

/// A clause read with letter names, as text.
fn names(frontend: &Frontend, text: &str) -> String {
    phones_of(&read(frontend, text))
}

/// A lone consonant letter is its sound in letter-sound mode
#[test]
fn a_lone_consonant_letter_is_its_sound_in_letter_sound_mode() {
    let croatian = Frontend::new(Language::Croatian);
    // The consonant and a stressed schwa, a full word of its own
    assert_eq!(sounds(&croatian, "Obrisano B"), "'obrisano b'ə");
    assert_eq!(sounds(&croatian, "B obrisano"), "b'ə 'obrisano");
    assert_eq!(sounds(&croatian, "veliko slovo Č"), "v'eliko sl'ovo č'ə");
    assert_eq!(sounds(&croatian, "Obrisano s"), "'obrisano s'ə");
    assert_eq!(sounds(&croatian, "Obrisano nj"), "'obrisano nj'ə");
    // Names in name mode; a digraph alone is one letter
    assert_eq!(names(&croatian, "Obrisano B"), "'obrisano b'e");
    assert_eq!(names(&croatian, "Obrisano nj"), "'obrisano 'enj");
    // Prepositions, abbreviations, compounds and the foreign letters are
    // read as in name mode
    for text in ["s njom", "S tobom", "k meni", "USB", "s.a.r.s.", "S-klasa", "x", "Obrisano a"] {
        assert_eq!(sounds(&croatian, text), names(&croatian, text), "{text}");
    }

    let serbian = Frontend::new(Language::Serbian);
    assert_eq!(sounds(&serbian, "Обрисано Њ"), "'obrisano nj'ə");
    assert_eq!(names(&serbian, "Обрисано Њ"), "'obrisano nj'e:");
}

/// The letters of a designation are letters, whatever word they spell
///
/// A listener's report of November 2026: "Marjan A17", a cell's contents and
/// its address as a screen reader says them, was heard as "Marijana 17", and
/// the hours of a clock time ran into its minutes. The number converter leaves
/// marks in the text (`crate::text::marks`): U+FDD1 after the letters of a
/// designation, U+FDD0 where a short pause belongs.
#[test]
fn the_letters_of_a_designation_are_letters_whatever_word_they_spell() {
    let hr = Frontend::new(Language::Croatian);
    // Without the mark the A is the conjunction a and leans on the number
    assert_eq!(sounds(&hr, "Marjan A sedamnaest"), "m'arjan a sed'amnaest");
    assert_eq!(sounds(&hr, "Marjan A\u{FDD1} sedamnaest"), "m'arjan 'a sed'amnaest");
    // The vowels that are words of their own: a, i, o, u
    assert_eq!(sounds(&hr, "Ćelija I\u{FDD1} pet"), "ć'elija 'i p'e:t");
    assert_eq!(sounds(&hr, "U\u{FDD1} dvadeset jedan"), "'u dv'a:deset j'edan");
    assert_eq!(sounds(&hr, "O\u{FDD1} dva"), "'o dv'a:");
    // s and k at the head of a clause are prepositions (the s voiced by the
    // d after it); in a designation, letters
    assert_eq!(sounds(&hr, "S dvadeset dva"), "z dv'a:deset dv'a:");
    assert_eq!(sounds(&hr, "S\u{FDD1} dvadeset dva"), "s'ə dv'a:deset dv'a:");
    assert_eq!(sounds(&hr, "K\u{FDD1} pet"), "k'ə p'e:t");
    // One letter follows the spelling mode, like any lone letter; q, x and y are named in both
    assert_eq!(sounds(&hr, "B\u{FDD1} dva"), "b'ə dv'a:");
    assert_eq!(names(&hr, "B\u{FDD1} dva"), "b'e dv'a:");
    assert_eq!(names(&hr, "S\u{FDD1} dvadeset dva"), "'es dv'a:deset dv'a:");
    assert_eq!(sounds(&hr, "X\u{FDD1} pet"), "'iks p'e:t");
    assert_eq!(sounds(&hr, "W\u{FDD1} pet"), "d'uplo v'ə p'e:t");
    // Two letters are spelled by their names, also where they are a word
    assert_eq!(sounds(&hr, "NA pet"), "na p'e:t");
    assert_eq!(sounds(&hr, "NA\u{FDD1} pet"), "'en 'a p'e:t");
    assert_eq!(sounds(&hr, "DO\u{FDD1} pet"), "d'e 'o p'e:t");
    assert_eq!(sounds(&hr, "AB\u{FDD1} dvanaest"), "'a b'e dv'a:naest");
    assert_eq!(sounds(&hr, "NJ\u{FDD1} pet"), "'en j'e p'e:t");
    // A longer word before the mark is left to its own reading
    assert_eq!(sounds(&hr, "TOP\u{FDD1} deset"), sounds(&hr, "TOP deset"));
    // The marks are noncharacters. A private use character, which an icon
    // font can put into a button's label, is no mark: it separates words as
    // any unknown sign does
    assert_eq!(sounds(&hr, "NA\u{E011} pet"), sounds(&hr, "NA pet"));
    assert_eq!(sounds(&hr, "dvanaest \u{E010} trideset"), sounds(&hr, "dvanaest trideset"));
    let private_use = hr.process(&chars("dvanaest \u{E010} trideset"), Punctuation::Period, true);
    assert_eq!(split_at_breaks(&private_use).len(), 1);

    let sr = Frontend::new(Language::Serbian);
    assert_eq!(sounds(&sr, "Marjan A\u{FDD1} sedamnaest"), "m'arjan 'a sed'amnae:st");
}

/// A short pause inside a clause splits it into parts
#[test]
fn a_short_pause_inside_a_clause_splits_it_into_parts() {
    let hr = Frontend::new(Language::Croatian);
    let breaks = |utt: &Utterance| -> Vec<u16> {
        utt.phones.iter().filter(|phone| phone.break_before).map(|phone| phone.word).collect()
    };
    let process = |text: &str, punct| hr.process(&chars(text), punct, true);

    // The mark is no word: the phones are those of the text without it
    let plain = process("Sastanak je u dvanaest trideset u uredu", Punctuation::Period);
    let mut marked = process("Sastanak je u dvanaest \u{FDD0} trideset u uredu", Punctuation::Period);
    assert_eq!(phones_of(&marked), phones_of(&plain));
    assert!(breaks(&plain).is_empty());
    assert_eq!(breaks(&marked), [4]);
    assert_eq!(split_at_breaks(&plain).len(), 1);

    // The parts: a continuation that only suspends the melody, then the rest
    // with the clause's own kind; words and syllables counted from each start
    marked.sentence_initial = true;
    let parts = split_at_breaks(&marked);
    assert_eq!(parts.len(), 2);
    assert_eq!(phones_of(&parts[0]), "s'astanak je u dv'a:naest");
    assert_eq!(phones_of(&parts[1]), "tr'i:deset u 'uredu");
    assert_eq!(parts[0].kind, ClauseKind::Continuation);
    assert!(parts[0].minor_break);
    assert!(!parts[0].continued);
    assert!(parts[0].sentence_initial);
    assert_eq!(parts[1].kind, ClauseKind::Statement);
    assert!(!parts[1].minor_break);
    assert!(parts[1].continued);
    assert!(!parts[1].sentence_initial);
    assert_eq!(parts[0].syllable_count + parts[1].syllable_count, marked.syllable_count);
    for part in &parts {
        assert_eq!(part.phones[0].word, 0);
        assert!(part.phones[0].word_start);
        let mut last = -1;
        for phone in &part.phones {
            assert!(!phone.break_before);
            assert!(phone.syllable >= 0);
            assert!(i32::from(phone.syllable) < part.syllable_count);
            assert!(phone.syllable >= last);
            last = phone.syllable;
        }
    }

    // The longer pause between two items (a cell's contents and its address)
    // splits the clause the same way and says which pause it is
    let items = process("dvanaest \u{FDD0} trideset \u{FDD2} A\u{FDD1} sedamnaest", Punctuation::Period);
    assert_eq!(breaks(&items), [1, 2]);
    let cell_parts = split_at_breaks(&items);
    assert_eq!(cell_parts.len(), 3);
    assert!(cell_parts[0].minor_break);
    assert!(!cell_parts[0].item_break);
    assert!(cell_parts[1].minor_break);
    assert!(cell_parts[1].item_break);
    assert!(!cell_parts[2].minor_break);
    assert!(!cell_parts[2].item_break);
    for part in &cell_parts {
        assert!(part.phones.iter().all(|phone| !phone.item_break));
    }
    assert!(parts.iter().all(|part| !part.item_break));

    // Three parts of a date; a question keeps its kind and its rise in the last part
    let date = process("desetog \u{FDD0} jedanaestog \u{FDD0} dvije tisuće dvadeset šeste", Punctuation::Question);
    let date_parts = split_at_breaks(&date);
    assert_eq!(date_parts.len(), 3);
    assert_eq!(date_parts[0].kind, ClauseKind::Continuation);
    assert_eq!(date_parts[1].kind, ClauseKind::Continuation);
    assert!(date_parts[1].continued);
    assert_eq!(date_parts[2].kind, ClauseKind::YesNoQuestion);
    // The word a question's peak sits on stays in its part: "Je li u dvanaest | trideset?"
    let li = process("Je li u dvanaest \u{FDD0} trideset", Punctuation::Question);
    assert_eq!(li.focus_word, 0);
    let li_parts = split_at_breaks(&li);
    assert_eq!(li_parts.len(), 2);
    assert_eq!(li_parts[0].focus_word, 0);
    assert_eq!(li_parts[1].focus_word, -1);

    // A pause with nothing before it, or nothing after it, is none
    assert!(breaks(&process("\u{FDD0} trideset", Punctuation::Period)).is_empty());
    assert!(breaks(&process("dvanaest \u{FDD0}", Punctuation::Period)).is_empty());
    // The pause stands before the first word made of what follows it: a
    // spelled designation
    let cell = process("Marjan \u{FDD0} AB\u{FDD1} dvanaest", Punctuation::Period);
    assert_eq!(phones_of(&cell), "m'arjan 'a b'e dv'a:naest");
    assert_eq!(breaks(&cell), [1]);
}
