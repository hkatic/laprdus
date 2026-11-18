//! Formant voice tests (Zvonko, Stojan, Mirsad and the singing presets):
//! the registry, determinism, languages, numerals, accents, rate, pitch, voice
//! quality, the reference balance, pauses, spelling, streaming and singing.
//!
//! Formant voices are synthesized by rule and need no data. The few cases
//! that also speak with a recorded voice need the packed recordings:
//! `cargo xtask data` writes `data/voices/Josip.bin` and `Vlado.bin`
//! (`LAPRDUS_DATA` names another directory), and those cases fail with a
//! clear message without them. The bundled dictionaries are read from
//! `data/dictionary`, staged by the same step.
//!
//! These tests render a lot of audio: run them optimized,
//!
//! ```text
//! cargo test --release --test formant
//! ```
//!
//! `LAPRDUS_TEST_INFO=1` (with `-- --nocapture`) prints the measured values
//! behind the thresholds.

#[macro_use]
mod support;

use laprdus::config::UserSettings;
use laprdus::engine::Error;
use laprdus::language::Language;
use laprdus::params::{NumberMode, SpellingMode, VoiceParams};
use laprdus::text::frontend::UserLexicon;
use laprdus::voice::{self, VOICES, Voice};
use std::ops::ControlFlow;
use std::path::{Path, PathBuf};
use std::sync::Arc;
use std::sync::atomic::{AtomicBool, Ordering};
use support::*;

// =============================================================================
// Registry, synthesis, languages and numerals
// =============================================================================

/// Notes the rows whose first text is not read as the second one: the audio
/// of both must be the same.
fn check_read_as(speaker: &mut Speaker, failures: &mut Failures, context: &str, rows: &[(&str, &str)]) {
    for &(text, words) in rows {
        info!("{text}");
        let same = speaker.say(text) == speaker.say(words);
        failures.check(same, || format!("{context}: {text:?} is not read as {words:?}"));
    }
}

// =============================================================================
// Registry
// =============================================================================

#[test]
fn formant_voices_are_registered() {
    for (id, language, lcid) in [("zvonko", "hr-HR", 0x041A), ("stojan", "sr-RS", 0x081A), ("mirsad", "bs-BA", 0x141A)]
    {
        let voice = voice::find(id).unwrap_or_else(|| panic!("{id} is not registered"));
        assert_eq!(voice.id, id);
        assert_eq!(voice.language.code(), language);
        assert_eq!(voice.language.lcid(), lcid);
        assert_eq!(voice.gender.as_str(), "Male");
        assert_eq!(voice.age.as_str(), "Adult");
        assert_eq!(voice.base_pitch, 1.0);
        assert_eq!(voice.base_voice_id, None);
        assert_eq!(voice.data_file(), None); // nothing to load
    }
}

#[test]
fn existing_voices_keep_their_place_in_the_registry() {
    // Platform code and saved settings rely on the first five entries; the
    // order of the rest is part of the 2.0 contract too.
    const ORDER: [(&str, &str, &str); 18] = [
        ("josip", "Josip", "hr-HR"),
        ("vlado", "Vlado", "sr-RS"),
        ("detence", "Detence", "hr-HR"),
        ("baba", "Baba", "hr-HR"),
        ("djedo", "Đedo", "sr-RS"),
        ("zvonko", "Zvonko", "hr-HR"),
        ("stojan", "Stojan", "sr-RS"),
        ("mirsad", "Mirsad", "bs-BA"),
        ("orguljas", "Orguljaš - Vila Velebita", "hr-HR"),
        ("dalmacija_klapa", "Dalmacija Klapa", "hr-HR"),
        ("trubac", "Trubač - Kreće se lađa francuska", "sr-RS"),
        ("harmonikas", "Harmonikaš - Kreće se lađa francuska", "sr-RS"),
        ("sevdalija", "Sevdalija - Kad ja pođoh na Bembašu", "bs-BA"),
        ("sazlija", "Sazlija - Kad ja pođoh na Bembašu", "bs-BA"),
        ("lika_velebit", "Lika Velebit", "hr-HR"),
        ("solunac", "Solunac", "sr-RS"),
        ("sarajevo_bembasa", "Sarajevo Bembaša", "bs-BA"),
        ("becarac", "Bećarac", "hr-HR"),
    ];
    assert_eq!(VOICES.len(), 18);
    for (voice, (id, name, language)) in VOICES.iter().zip(ORDER) {
        assert_eq!(voice.id, id);
        assert_eq!(voice.display_name, name, "{id}");
        assert_eq!(voice.language.code(), language, "{id}");
        // Ids are plain ASCII; names carry no "Laprdus" prefix or language suffix
        assert!(voice.id.chars().all(|c| c.is_ascii_lowercase() || c == '_'), "{id}");
        assert!(!voice.display_name.contains("Laprdus"), "{id}");
        assert!(!voice.display_name.contains('('), "{id}");
    }

    assert_eq!(voice::find("josip").and_then(|josip| josip.data_file()), Some("Josip.bin"));
}

#[test]
fn old_voice_ids_load_the_renamed_voices() {
    const ALIASES: [(&str, &str); 7] = [
        ("djed", "djedo"),
        ("dijete", "detence"),
        ("baka", "baba"),
        ("klapa", "dalmacija_klapa"),
        ("pjevac", "lika_velebit"),
        ("pevac", "solunac"),
        ("solist", "sarajevo_bembasa"),
    ];
    for (old_id, id) in ALIASES {
        let voice = voice::find(old_id).unwrap_or_else(|| panic!("{old_id} is not known"));
        assert_eq!(voice.id, id, "{old_id}");
        // The renamed voice loads under the old id and reports the new one
        // (a recorded one from its data, a formant one without any)
        let mut speaker = Speaker::empty();
        let directory = if voice.data_file().is_some() { data_dir() } else { NO_DATA.into() };
        assert!(speaker.0.set_voice(old_id, &directory).is_ok(), "{old_id}");
        assert_eq!(speaker.voice_id(), id, "{old_id}");
    }

    let djedo = voice::find("djed").expect("the old id of Đedo");
    assert_eq!(djedo.id, "djedo");
    assert_eq!(djedo.display_name, "Đedo");
    assert_eq!(djedo.base_voice_id, Some("vlado"));
    assert_eq!(djedo.gender.as_str(), "Male");
    assert_eq!(djedo.age.as_str(), "Senior");
    assert!(voice::find("no_such_voice").is_none());
}

#[test]
fn the_four_languages_are_listed_and_zvonko_is_the_default_voice() {
    assert_eq!(voice::DEFAULT_VOICE_ID, "zvonko");
    let zvonko = voice::find(voice::DEFAULT_VOICE_ID).expect("the default voice");
    assert_eq!(voice::default_voice(), zvonko);
    assert_eq!(zvonko.data_file(), None);

    assert_eq!(Language::ALL.len(), 4);
    const EXPECTED: [(&str, &str, u16, &str, &str); 4] = [
        ("hr-HR", "hr", 0x041A, "Croatian", "Hrvatski"),
        ("sr-RS", "sr", 0x081A, "Serbian", "Srpski"),
        ("bs-BA", "bs", 0x141A, "Bosnian", "Bosanski"),
        ("sl-SI", "sl", 0x0424, "Slovenian", "Slovenščina"),
    ];
    for (language, (code, short_code, lcid, english, native)) in Language::ALL.into_iter().zip(EXPECTED) {
        assert_eq!(language.code(), code);
        assert_eq!(language.short_code(), short_code, "{code}");
        assert_eq!(language.lcid(), lcid, "{code}");
        assert_eq!(language.english_name(), english, "{code}");
        assert_eq!(language.native_name(), native, "{code}");
    }

    // Every voice's native language is one of the four
    for voice in VOICES {
        assert!(["hr-HR", "sr-RS", "bs-BA", "sl-SI"].contains(&voice.language.code()), "{}", voice.id);
    }
}

// =============================================================================
// Synthesis
// =============================================================================

#[test]
fn formant_voices_speak_without_any_voice_data() {
    for voice in FORMANT_VOICES {
        let mut speaker = Speaker::empty();
        assert!(speaker.0.set_voice(voice, Path::new(NO_DATA)).is_ok(), "{voice}");
        assert!(speaker.is_initialized(), "{voice}");
        assert_eq!(speaker.voice_id(), voice);

        let audio = speaker.say("Dobar dan, kako ste?");
        let seconds = audio.len() as f64 / 22050.0;

        // Seven syllables plus two pauses: clearly more than a blip, clearly
        // less than slow-motion speech.
        assert!(seconds > 0.8, "{voice}: {seconds} s");
        assert!(seconds < 3.0, "{voice}: {seconds} s");
        assert!(rms(&audio) > 800.0, "{voice}"); // audible
        assert!(peak(&audio) < 32000, "{voice}"); // not clipped
    }
}

#[test]
fn formant_synthesis_is_deterministic() {
    let mut speaker = Speaker::new("zvonko");
    let first = speaker.say("Sunce sja.");
    let second = speaker.say("Sunce sja.");
    assert!(!first.is_empty());
    assert!(first == second);
}

#[test]
fn the_three_formant_voices_are_different_speakers() {
    let pitch = FORMANT_VOICES.map(|voice| {
        let mut speaker = Speaker::new(voice);
        speaker.set_inflection_level(0.0);
        let pitch = pitch_hz(&speaker.say("aaa"));
        info!("{voice}: {pitch:.1} Hz");
        assert!(pitch > 70.0, "{voice}: {pitch} Hz");
        assert!(pitch < 180.0, "{voice}: {pitch} Hz");
        pitch
    });
    // Stojan is the lowest voice, Mirsad the highest.
    assert!(pitch[1] < pitch[0] - 5.0, "{pitch:?}");
    assert!(pitch[2] > pitch[0] + 5.0, "{pitch:?}");
}

#[test]
fn formant_voices_handle_edge_case_input() {
    let mut speaker = Speaker::new("zvonko");

    // Nothing speakable: no audio, no crash.
    assert!(speaker.say("").is_empty());
    assert!(speaker.say("...").is_empty());
    assert!(speaker.say("   ").is_empty());

    // Still produces speech.
    assert!(!speaker.say("a").is_empty());
    assert!(!speaker.say("s").is_empty()); // letter name
    assert!(!speaker.say("HTML").is_empty()); // spelled abbreviation
    assert!(!speaker.say("prst").is_empty()); // syllabic r
    assert!(!speaker.say("123").is_empty()); // number expansion
    assert!(!speaker.say("Žèna").is_empty()); // accent mark in text
    assert!(!speaker.say("Hello world, xylophone & quartz!").is_empty());

    // A long text must not grow unbounded or fail.
    let long_text = "Ovo je duga rečenica broj jedan. ".repeat(200);
    let audio = speaker.say(&long_text);
    assert!(audio.len() > 22050 * 60);
}

#[test]
fn cyrillic_and_latin_text_sound_the_same() {
    let mut speaker = Speaker::new("stojan");
    let latin = speaker.say("Dobar dan");
    let cyrillic = speaker.say("Добар дан");
    assert!(!latin.is_empty());
    assert!(latin == cyrillic);
}

#[test]
fn number_words_follow_the_voice_language() {
    // Croatian "tisuću" and Serbian/Bosnian "hiljadu" are different words,
    // so the audio for "1000" must equal the audio for the spelled-out word.
    // One and two agree with the feminine "tisuća"/"hiljada" (dvije tisuće,
    // dve hiljade, dvadeset jedna tisuća), 11-19 take the genitive plural.
    const CASES: [(&str, &str, &str); 14] = [
        ("zvonko", "1000", "tisuću"),
        ("stojan", "1000", "hiljadu"),
        ("mirsad", "1000", "hiljadu"),
        ("zvonko", "2000", "dvije tisuće"),
        ("stojan", "2000", "dve hiljade"),
        ("mirsad", "2000", "dvije hiljade"),
        ("zvonko", "22000", "dvadeset dvije tisuće"),
        ("stojan", "22000", "dvadeset dve hiljade"),
        ("mirsad", "22000", "dvadeset dvije hiljade"),
        ("zvonko", "220000", "dvjesto dvadeset tisuća"),
        ("zvonko", "21000", "dvadeset jedna tisuća"),
        ("zvonko", "12000", "dvanaest tisuća"),
        ("zvonko", "2000000", "dva milijuna"),
        ("zvonko", "2000000000", "dvije milijarde"),
    ];
    for (voice, digits, words) in CASES {
        let mut speaker = Speaker::new(voice);
        let spoken = speaker.say(digits);
        assert!(!spoken.is_empty(), "{voice}: {digits}");
        assert!(spoken == speaker.say(words), "{voice}: {digits} is not read as {words:?}");
    }
}

#[test]
fn the_language_is_chosen_apart_from_the_voice() {
    // Every voice speaks every language: the language picks the number words
    // (and the rest of the front end) while the voice stays.
    let mut speaker = Speaker::new("zvonko");
    assert_eq!(speaker.language_code(), "hr-HR");
    let croatian = speaker.say("1000");
    assert!(!croatian.is_empty());
    assert!(croatian == speaker.say("tisuću"));

    // Serbian from Zvonko
    assert!(speaker.set_language("sr"));
    assert_eq!(speaker.language_code(), "sr-RS");
    assert_eq!(speaker.voice_id(), "zvonko");
    assert!(speaker.say("1000") == speaker.say("hiljadu"));
    assert!(speaker.say("2000") == speaker.say("dve hiljade"));
    assert!(speaker.say("1000") != croatian);

    // Slovenian from Zvonko: tisoč, which does not change (dva tisoč)
    assert!(speaker.set_language("sl"));
    assert_eq!(speaker.language_code(), "sl-SI");
    assert_eq!(speaker.voice_id(), "zvonko");
    assert!(speaker.say("1000") == speaker.say("tisoč"), "Slovenian: 1000 is not read as tisoč");
    assert!(speaker.say("2000") == speaker.say("dva tisoč"), "Slovenian: 2000 is not read as dva tisoč");
    assert!(speaker.say("1000") != croatian);

    // Bosnian from Zvonko
    assert!(speaker.set_language("bs"));
    assert_eq!(speaker.language_code(), "bs-BA");
    assert!(speaker.say("2000") == speaker.say("dvije hiljade"));
    assert!(speaker.say("1000") == speaker.say("hiljadu"));

    // The same words as the native speaker of the language says them
    let zvonko_bosnian = speaker.say("2000");
    assert!(speaker.set_language("hr"));
    assert!(speaker.say("1000") == croatian);

    // Tags in other spellings; an unknown one is refused and changes nothing
    for tag in ["HR", "hr-HR", "hr_HR", "hrv", "Croatian", "hrvatski"] {
        assert!(speaker.set_language("sr"), "{tag}");
        assert!(speaker.set_language(tag), "{tag}");
        assert_eq!(speaker.language_code(), "hr-HR", "{tag}");
    }
    for tag in ["SR", "sr-RS", "sr_RS", "srp", "sr-Latn-RS", "Serbian", "srpski"] {
        assert!(speaker.set_language(tag), "{tag}");
        assert_eq!(speaker.language_code(), "sr-RS", "{tag}");
    }
    for tag in ["BS", "bs-BA", "bs_BA", "bos", "Bosnian", "bosanski"] {
        assert!(speaker.set_language(tag), "{tag}");
        assert_eq!(speaker.language_code(), "bs-BA", "{tag}");
    }
    for tag in ["sl", "sl-SI", "SL_si", "slv", "Slovenian", "Slovene", "slovenščina"] {
        assert!(speaker.set_language(tag), "{tag}");
        assert_eq!(speaker.language_code(), "sl-SI", "{tag}");
    }
    assert!(speaker.set_language("bs"));
    assert!(!speaker.set_language("de"));
    assert!(!speaker.set_language("english"));
    assert_eq!(speaker.language_code(), "bs-BA");
    assert!(speaker.say("2000") == zvonko_bosnian);
}

#[test]
fn a_chosen_language_stays_across_voice_changes_until_it_is_cleared() {
    let mut speaker = Speaker::new("zvonko");
    assert!(speaker.set_language("sr"));

    // Mirsad, whose own language is Bosnian, keeps speaking Serbian
    assert!(speaker.set_voice("mirsad").is_ok());
    assert_eq!(speaker.voice_id(), "mirsad");
    assert_eq!(speaker.language_code(), "sr-RS");
    assert!(speaker.say("2000") == speaker.say("dve hiljade"));
    assert!(speaker.set_voice("stojan").is_ok());
    assert_eq!(speaker.language_code(), "sr-RS");
    assert!(speaker.set_voice("mirsad").is_ok());
    assert_eq!(speaker.language_code(), "sr-RS");

    // No language returns to the voice's native language, now and after
    // later changes
    speaker.0.set_language(None);
    assert_eq!(speaker.language_code(), "bs-BA");
    assert!(speaker.say("2000") == speaker.say("dvije hiljade"));
    assert!(speaker.set_voice("zvonko").is_ok());
    assert_eq!(speaker.language_code(), "hr-HR");
    assert!(speaker.say("2000") == speaker.say("dvije tisuće"));
    assert!(speaker.set_voice("stojan").is_ok());
    assert_eq!(speaker.language_code(), "sr-RS");

    // An empty tag means the same, as a host passes it
    assert!(speaker.set_language("bs"));
    assert!(speaker.set_language(""));
    assert_eq!(speaker.language_code(), "sr-RS");

    // A language chosen before the first voice applies to it
    let mut fresh = Speaker::empty();
    assert!(fresh.set_language("bs"));
    assert!(fresh.set_voice("zvonko").is_ok());
    assert_eq!(fresh.language_code(), "bs-BA");
    assert!(fresh.say("2000") == fresh.say("dvije hiljade"));

    // The singing presets follow the language too
    assert!(fresh.set_voice("dalmacija_klapa").is_ok());
    assert_eq!(fresh.language_code(), "bs-BA");
    fresh.0.set_language(None);
    assert_eq!(fresh.language_code(), "hr-HR");
}

#[test]
fn slovenian_spelling_uses_slovenian_names_and_preserves_user_overrides() {
    check_slovenian_spelling(&mut Speaker::new("zvonko"));
}

#[test]
fn slovenian_reads_a_word_by_the_clause_after_the_comma() {
    // Meni, da je prav: the clause after the comma makes meni the verb, which
    // the front end cannot see in one clause; alone it is the menu. For every
    // voice, the recorded ones too.
    //
    // Each row: a text, the same text with the accent written, and whether
    // the two must sound the same.
    const READINGS: &[(&str, &str, bool)] = &[
        ("Meni, da je prav.", "Méni, da je prav.", true),
        ("Meni, gumb.", "Méni, gumb.", false),
        ("Ni obvestil, da pride.", "Ni obvéstil, da pride.", true),
        ("Ni obvestil, gumb.", "Ni obvéstil, gumb.", false),
        // Pero, kje si? Pero, pridi sem! The address is the name Pêro (first
        // syllable), the pen peró elsewhere at the head; the imperative
        // vesêli se by the exclamation mark of the sentence
        ("Pero, kje si?", "Pêro, kje si?", true),
        ("Pero, pridi sem!", "Pêro, pridi sem!", true),
        ("Pero, pridi sem.", "Pêro, pridi sem.", true),
        ("Pero je na mizi.", "Peró je na mizi.", true),
        ("Pero je na mizi.", "Pêro je na mizi.", false),
        ("Veseli se, Pero!", "Vesêli se, Pêro!", true),
        ("Veseli se, Pero.", "Vesêli se, Pêro.", false),
        // Samo, pridi sem. Hvala, Samo. The address is the name Sámo (SP 2001
        // Sámo -a, oseb. i.; a listener's report of 2026-11-10); without the
        // comma the head of the clause stays samó, "only"
        ("Samo, pridi sem.", "Sámo, pridi sem.", true),
        ("Samo, kje si?", "Sámo, kje si?", true),
        ("Hvala, Samo.", "Hvala, Sámo.", true),
        ("Samo pridi sem.", "Sámo pridi sem.", false),
        // Samo je prišel. The name as the subject of a man's perfect at the
        // head of a sentence; On je samo prišel. is "only"
        ("Samo je prišel.", "Sámo je prišel.", true),
        ("On je samo prišel.", "On je sámo prišel.", false),
        // glasu: the genitive glasú, the dative and locative glásu (SSKJ²,
        // SP 2001 glás -ú, dalje -u -- -u -om; a listener's report of
        // 2026-11-10), and the same for the nouns SP declines so
        ("Nisem imel glasu.", "Nisem imel glasú.", true),
        ("V glasu je hripavost.", "V glásu je hripavost.", true),
        ("V glasu je hripavost.", "V glasú je hripavost.", false),
        ("Glasu ne morem preveriti.", "Glasú ne morem preveriti.", true),
        ("V tvojem glasu je slišati toplino.", "V tvojem glásu je slišati toplino.", true),
        ("Barva glasu.", "Barva glasú.", true),
        ("Pusti me pri miru.", "Pusti me pri míru.", true),
        ("Brez strahu.", "Brez strahú.", true),
        // A listener's report of 2026-11-17: mêja and mejá, vasí and vási, the
        // imperative and the present of izgovoriti, govoriti, sprostiti, the
        // day a sentence names, pêtega
        ("Levo od tebe je meja.", "Levo od tebe je mêja.", true),
        ("Levo od tebe je meja.", "Levo od tebe je mejá.", false),
        ("Igre brez meja.", "Igre brez mejá.", true),
        ("On je iz vasi.", "On je iz vasí.", true),
        ("V tej vasi je veliko hiš.", "V tej vási je veliko hiš.", true),
        ("V tej vasi je veliko hiš.", "V tej vasí je veliko hiš.", false),
        ("Na drugi strani.", "Na drugi stráni.", true),
        ("Z druge strani.", "Z druge straní.", true),
        ("Izgovori prosim pravilno!", "Izgovôri prosim pravilno!", true),
        ("On to pogosto izgovori.", "On to pogosto izgovorí.", true),
        ("On to pogosto izgovori.", "On to pogosto izgôvori.", false),
        ("Govori razločno!", "Govôri razločno!", true),
        ("Ona govori razločno.", "Ona govorí razločno.", true),
        ("Sprosti prostor na telefonu!", "Sprôsti prostor na telefonu!", true),
        ("Glasba me sprosti.", "Glasba me sprostí.", true),
        ("Ta vas je lepa.", "Ta vás je lepa.", true),
        ("Grem v vas.", "Grem v vás.", true),
        ("Zaupam v vas.", "Zaupam v vás.", false),
        ("Pogovori se z njim.", "Pogovôri se z njim.", true),
        // prosim between commas is a clause of its own
        ("Položi, prosim, telefon.", "Polôži, prosim, telefon.", true),
        ("Prosim, položi telefon.", "Prosim, polôži telefon.", true),
        ("Položi telefon.", "Polôži telefon.", false),
        // Lahko by the question mark, which may be clauses away
        ("Lahko delo opravi vsak.", "Láhko delo opravi vsak.", true),
        ("Lahko delo opravi vsak?", "Láhko delo opravi vsak?", false),
        ("Lahko delo opravi vsak, ki hoče.", "Láhko delo opravi vsak, ki hoče.", true),
        ("Lahko delo opravi vsak, ki hoče?", "Láhko delo opravi vsak, ki hoče?", false),
    ];
    let mut failures = Failures::new();
    for voice in ["zvonko", "josip"] {
        let mut speaker = Speaker::new(voice);
        assert!(speaker.set_language("sl"));
        for &(text, marked, same) in READINGS {
            let equal = speaker.say(text) == speaker.say(marked);
            failures.check(equal == same, || {
                format!("{voice}: {text:?} {} sound like {marked:?}", if same { "does not" } else { "must not" })
            });
        }
    }
    failures.finish();
}

#[test]
fn slovenian_is_spoken_by_every_voice() {
    // No voice has Slovenian as its native language: every one of them
    // speaks it when asked, formant and recorded alike, with a front end of
    // its own, so the same sentence does not sound as it does in Croatian.
    let text = "Dober dan, kako ste? Bil sem v Ljubljani.";
    let mut zvonko = Speaker::new("zvonko");
    let croatian = zvonko.say(text);
    assert!(!croatian.is_empty());
    assert!(zvonko.set_language("sl"));
    assert_eq!(zvonko.language_code(), "sl-SI");
    let slovenian = zvonko.say(text);
    assert!(!slovenian.is_empty());
    assert!(slovenian != croatian, "Slovenian sounds like Croatian");
    // Deterministic, like the other languages
    assert!(zvonko.say(text) == slovenian);

    let mut failures = Failures::new();
    // Numbers in Slovenian words: the ones before the tens (written as one
    // word, read with an accent on each part), the dual of milijon, the
    // "cela" of a decimal
    const NUMBERS: &[(&str, &str)] = &[
        ("21", "enaindvajset"),
        ("55", "petinpetdeset"),
        ("2000000", "dva milijona"),
        ("3,14", "tri cele štirinajst"),
    ];
    check_read_as(&mut zvonko, &mut failures, "numbers", NUMBERS);
    // 1 to 4 agree with what they count, in gender, number and case, and are
    // ena, dve, tri, stiri where they count nothing (a listener's reports; SP 2001,
    // SSKJ2 and the Jezikovna svetovalnica have dve as the name of the number)
    const TWOS: &[(&str, &str)] = &[
        // 2
        ("2 koša", "dva koša"),
        ("2 načina", "dva načina"),
        ("2 velika koša", "dva velika koša"),
        ("T2", "T dva"),
        ("T-2", "T-dva"),
        ("PS/2", "PS/dva"),
        ("Pentium 2", "Pentium dva"),
        ("To je Pentium 2", "To je Pentium dva"),
        ("člen 2", "člen dva"),
        ("monitor 2", "monitor dva"),
        ("2 kg", "dva kg"),
        ("2kg", "dva kg"),
        ("2-krat", "dvakrat"),
        ("2 %", "dva odstotka"),
        ("2 Slovenca", "dva Slovenca"),
        // The number 2 itself, counting nothing
        ("2", "dve"),
        ("2.", "dve."),
        ("1, 2, 3", "ena, dve, tri"),
        ("0 1 2 3", "nič ena dve tri"),
        ("02", "nič dve"),
        ("2 plus 2", "dve plus dve"),
        ("2 + 2", "dve + dve"),
        ("2 je sodo", "dve je sodo"),
        ("pritisni 2", "pritisni dve"),
        ("stran 2", "stran dve"),
        ("Stran 2", "Stran dve"),
        ("slika 2", "slika dve"),
        ("poglavje 2", "poglavje dve"),
        ("3.2", "tri.dve"),
        ("2 dni", "dva dni"),
        ("102 stola", "sto dva stola"),
        ("2002", "dva tisoč dva"),
        ("2 uri", "dve uri"),
        ("2 mesti", "dve mesti"),
        ("2 veliki hiši", "dve veliki hiši"),
        ("102 leti", "sto dve leti"),
        ("1.002 knjigi", "tisoč dve knjigi"),
        ("ura je 2", "ura je dve"),
        ("2:30", "dve \u{FDD0} trideset"),
        ("2,5", "dve celi pet"),
        ("2 milijardi", "dve milijardi"),
        ("2 milijona", "dva milijona"),
        ("z 2 žličkama", "z dvema žličkama"),
        ("pred 2 leti", "pred dvema leti"),
        ("v 2 urah", "v dveh urah"),
        ("do 2 let", "do dveh let"),
        ("od 2 ur", "od dveh ur"),
        ("2 min", "dve min"),
        // 1
        ("1 koš", "en koš"),
        ("101 koš", "sto en koš"),
        ("1 knjiga", "ena knjiga"),
        ("1 knjigo", "eno knjigo"),
        ("1 leto", "eno leto"),
        ("1 evro", "en evro"),
        ("1 stvar", "ena stvar"),
        ("1 možnost", "ena možnost"),
        ("1 kg", "en kg"),
        ("do 1 leta", "do enega leta"),
        ("od 1 ure", "od ene ure"),
        ("v 1 uri", "v eni uri"),
        ("v 1 dnevu", "v enem dnevu"),
        ("pred 1 letom", "pred enim letom"),
        ("1", "ena"),
        ("T1", "T ena"),
        ("1 od 5", "ena od pet"),
        ("1 in 1", "ena in ena"),
        ("stran 1 od 3", "stran ena od tri"),
        // 3, 4
        ("3 koši", "trije koši"),
        ("4 koši", "štirje koši"),
        ("103 koši", "sto trije koši"),
        ("3 ljudje", "trije ljudje"),
        ("3 koše", "tri koše"),
        ("3 hiše", "tri hiše"),
        ("4 mesta", "štiri mesta"),
        ("3 stvari", "tri stvari"),
        ("4 možnosti", "štiri možnosti"),
        ("3 dni", "tri dni"),
        ("s 3 žlicami", "s tremi žlicami"),
        ("s 4 žlicami", "s štirimi žlicami"),
        ("pred 3 leti", "pred tremi leti"),
        ("v 3 dneh", "v treh dneh"),
        ("v 4 letih", "v štirih letih"),
        ("k 3 osebam", "k trem osebam"),
        ("3 tisoč", "tri tisoč"),
        ("slika 3", "slika tri"),
    ];
    check_read_as(&mut zvonko, &mut failures, "1 to 4", TWOS);
    // The same, since the listener's report of 2026-11-03 on the numerals:
    // ordinals in the case the noun and the preposition show, 5 and above
    // in the oblique cases, the nouns of the unit signs, a minus, thousands
    // with spaces, ranges, 2-krat and 2-letni as one word, the hours after
    // ob (SP 2001 pét, stó, tísoč, pŕvi, trétji, dvákrat, dvóléten,
    // odstótek, évro, stopínja; Pravopis 8.0 § 26)
    const NUMERALS: &[(&str, &str)] = &[
        ("1. mesto", "prvo mesto"),
        ("2. mesto", "drugo mesto"),
        ("3. mesto", "tretje mesto"),
        ("na 1. mestu", "na prvem mestu"),
        ("v 2. nadstropju", "v drugem nadstropju"),
        ("3. člen", "tretji člen"),
        ("1. stran", "prva stran"),
        ("21. stoletje", "enaindvajseto stoletje"),
        ("v 21. stoletju", "v enaindvajsetem stoletju"),
        ("v 90. letih", "v devetdesetih letih"),
        ("80. leta", "osemdeseta leta"),
        ("100. obletnica", "stota obletnica"),
        ("7. oktober 2026", "sedmi oktober dva tisoč šestindvajset"),
        ("7. oktobra", "sedmega oktobra"),
        ("15. do 20. marca", "petnajstega do dvajsetega marca"),
        ("7. 10.", "sedmega \u{FDD0} desetega."),
        ("7.10.", "sedmega \u{FDD0} desetega."),
        // The day a sentence names is in the nominative (a listener's report
        // of 2026-11-17); petega has the open e of pêti
        ("Danes je 16.4.2026.", "Danes je šestnajsti \u{FDD0} četrti \u{FDD0} dva tisoč šestindvajset."),
        ("Danes je 16. 4.", "Danes je šestnajsti \u{FDD0} četrti."),
        ("Danes smo 16.4.", "Danes smo šestnajstega \u{FDD0} četrtega."),
        ("do 5.11.", "do pêtega \u{FDD0} enajstega."),
        ("15.–20. 3.", "petnajstega do dvajsetega \u{FDD0} tretjega."),
        ("3.–4. mesto", "tretje do četrto mesto"),
        ("do 3. razreda", "do tretjega razreda"),
        ("k 1. členu", "k prvemu členu"),
        ("ob 5. uri", "ob peti uri"),
        ("2. svetovna vojna", "druga svetovna vojna"),
        ("1.", "ena."),
        ("v 5 urah", "v petih urah"),
        ("s 5 prsti", "s petimi prsti"),
        ("pred 5 leti", "pred petimi leti"),
        ("pred 21 leti", "pred enaindvajsetimi leti"),
        ("po 5 letih", "po petih letih"),
        ("v 12 urah", "v dvanajstih urah"),
        ("k 5 osebam", "k petim osebam"),
        ("do 5 let", "do petih let"),
        ("pred 7 leti", "pred sedmimi leti"),
        ("v 100 letih", "v sto letih"),
        ("5 novih hiš", "pet novih hiš"),
        ("1 %", "en odstotek"),
        ("2 %", "dva odstotka"),
        ("3 %", "trije odstotki"),
        ("4 %", "štirje odstotki"),
        ("5 %", "pet odstotkov"),
        ("21 %", "enaindvajset odstotkov"),
        ("101 %", "sto en odstotek"),
        ("0 %", "nič odstotkov"),
        ("za 3 %", "za tri odstotke"),
        ("1 €", "en evro"),
        ("2 €", "dva evra"),
        ("3 €", "trije evri"),
        ("5 €", "pet evrov"),
        ("stane 3 €", "stane tri evre"),
        ("1 °C", "ena stopinja Celzija"),
        ("2 °C", "dve stopinji Celzija"),
        ("5 °C", "pet stopinj Celzija"),
        ("1,5 %", "ena cela pet odstotka"),
        ("2,50 €", "dve celi petdeset evra"),
        ("1.000 €", "tisoč evrov"),
        ("1 min", "ena min"),
        ("1 MB", "en MB"),
        ("2 h", "dve h"),
        ("1 tisoč", "en tisoč"),
        ("2 tisoč", "dva tisoč"),
        ("3 tisoč", "tri tisoč"),
        ("-5", "minus pet"),
        ("-5 °C", "minus pet stopinj Celzija"),
        ("–10 °C", "minus deset stopinj Celzija"),
        ("T-2", "T-dva"),
        ("1 000", "tisoč"),
        ("1 000 000", "milijon"),
        ("1\u{A0}000", "tisoč"),
        ("2 000 ljudi", "dva tisoč ljudi"),
        ("1 2 3", "ena dve tri"),
        ("1991–2000", "tisoč devetsto enaindevetdeset do dva tisoč"),
        ("1991-2000", "tisoč devetsto enaindevetdeset do dva tisoč"),
        ("2–3 dni", "dva do tri dni"),
        ("pred 2–3 leti", "pred dvema do tremi leti"),
        ("10–15 %", "deset do petnajst odstotkov"),
        ("2x", "dvakrat"),
        ("3x", "trikrat"),
        ("1-krat", "enkrat"),
        ("21-krat", "enaindvajsetkrat"),
        ("2-letni", "dvoletni"),
        ("3-letna", "triletna"),
        ("10-letnica", "desetletnica"),
        ("20-letnik", "dvajsetletnik"),
        ("5-dnevni", "petdnevni"),
        ("1-sobno", "enosobno"),
        ("2-sobno", "dvosobno"),
        ("ob 5", "ob petih"),
        ("ob 5h", "ob petih"),
        ("ob 8h", "ob osmih"),
        ("ob 5.00", "ob petih"),
        ("ob 17:00", "ob sedemnajstih"),
        ("ob 13", "ob trinajstih"),
        ("ob 8.30", "ob osem \u{FDD0} trideset"),
        ("ob 20.15", "ob dvajset \u{FDD0} petnajst"),
        ("od 8.30 do 16.00", "od osem \u{FDD0} trideset do šestnajst \u{FDD0} nič nič"),
        ("8.30–16.00", "osem \u{FDD0} trideset do šestnajst \u{FDD0} nič nič"),
        ("24:00", "štiriindvajset \u{FDD0} nič nič"),
        ("Ura je 8.30", "Ura je osem \u{FDD0} trideset"),
        ("Ura je 5", "Ura je pet"),
        ("ob 5 urah", "ob petih urah"),
        ("3.14", "tri pika štirinajst"),
        // The listener's answers of 2026-11-03: the feminine accusative and
        // instrumental of tretji by the table of neuter nouns in -o, 1 before
        // a masculine genitive or animate accusative in -a, the unit's noun
        // in the case of the preposition, ranges written with do and in
        ("za 3. nagrado", "za tretjo nagrado"),
        ("s 3. nagrado", "s tretjo nagrado"),
        ("3. nagrada", "tretja nagrada"),
        ("3. svetovno vojno", "tretjo svetovno vojno"),
        ("3. zlato medaljo", "tretjo zlato medaljo"),
        ("3. politično zatočišče", "tretje politično zatočišče"),
        ("videl sem 1 otroka", "videl sem enega otroka"),
        ("imam 1 brata", "imam enega brata"),
        ("videl sem 3. otroka", "videl sem tretjega otroka"),
        ("1 miza", "ena miza"),
        ("1 kolega", "en kolega"),
        ("1958. leta", "tisoč devetsto oseminpetdesetega leta"),
        ("do 5 %", "do petih odstotkov"),
        ("do 1 %", "do enega odstotka"),
        ("do 2 %", "do dveh odstotkov"),
        ("do 3 €", "do treh evrov"),
        ("pri 20 °C", "pri dvajsetih stopinjah Celzija"),
        ("pri 1 °C", "pri eni stopinji Celzija"),
        ("Pri 0 °C", "Pri nič stopinj Celzija"),
        ("s 5 €", "s petimi evri"),
        ("z 1 €", "z enim evrom"),
        ("z 2 €", "z dvema evroma"),
        ("s 3 €", "s tremi evri"),
        ("k 5 €", "k petim evrom"),
        ("nad 5 %", "nad petimi odstotki"),
        ("v 30 % primerov", "v tridesetih odstotkih primerov"),
        ("za 1 °C", "za eno stopinjo Celzija"),
        ("do 21 %", "do enaindvajsetih odstotkov"),
        ("do 100 %", "do sto odstotkov"),
        ("od 5 do 10 %", "od petih do desetih odstotkov"),
        ("5 do 10 %", "pet do deset odstotkov"),
        ("med 5 in 10 °C", "med petimi in desetimi stopinjami Celzija"),
        ("od 1 do 2 %", "od enega do dveh odstotkov"),
        ("od 2 do 3 ur", "od dveh do treh ur"),
        ("od 5 do 10 let", "od petih do desetih let"),
        ("od 1 do 5 ur", "od ene do petih ur"),
        ("za 2 do 5 kilogramov", "za dva do pet kilogramov"),
        ("2 do 3 ure", "dve do tri ure"),
        ("starosti 2 do 3 let", "starosti dveh do treh let"),
        ("ocene od 1 do 5", "ocene od ena do pet"),
        ("od 2006 do 1. aprila 2007", "od dva tisoč šest do prvega aprila dva tisoč sedem"),
    ];
    check_read_as(&mut zvonko, &mut failures, "numerals", NUMERALS);
    // The other formant voices and the singing presets speak it too
    for voice in ["stojan", "mirsad", "lika_velebit"] {
        assert!(zvonko.set_voice(voice).is_ok(), "{voice}");
        assert_eq!(zvonko.language_code(), "sl-SI", "{voice}");
        assert!(!zvonko.say(text).is_empty(), "{voice}");
    }

    // A recorded voice
    let mut josip = Speaker::new("josip");
    let josip_croatian = josip.say(text);
    assert!(!josip_croatian.is_empty());
    assert!(josip.set_language("sl"));
    assert_eq!(josip.language_code(), "sl-SI");
    let josip_slovenian = josip.say(text);
    assert!(!josip_slovenian.is_empty());
    failures.check(josip_slovenian != josip_croatian, || "josip: Slovenian sounds like Croatian".to_owned());
    check_read_as(&mut josip, &mut failures, "josip", &[("1000", "tisoč")]);
    failures.finish();
}

#[test]
fn croatian_serbian_and_bosnian_numerals_agree_with_their_nouns() {
    // The numerals' grammar of 2026-11-03 for the three languages: jedan,
    // jedna, jedno and dva, dvije (dve) agree with the noun, 1 declines after
    // a preposition, the ordinals take the case the noun and the preposition
    // show, the unit signs are nouns, a period or a space between groups of
    // three digits is the thousands separator (Hrvatski pravopis), a minus is
    // minus, a range reads do, 2-godišnji is one word, 8.30 after u is a time
    let mut speaker = Speaker::new("zvonko");
    let mut failures = Failures::new();
    const CROATIAN: &[(&str, &str)] = &[
        ("1 kuća", "jedna kuća"),
        ("2 kuće", "dvije kuće"),
        ("1 selo", "jedno selo"),
        ("1 dijete", "jedno dijete"),
        ("1 stol", "jedan stol"),
        ("2 stola", "dva stola"),
        ("1 stvar", "jedna stvar"),
        ("2 noći", "dvije noći"),
        ("1 auto", "jedan auto"),
        ("21 kuća", "dvadeset jedna kuća"),
        ("102 kuće", "sto dvije kuće"),
        ("s 1 prijateljem", "s jednim prijateljem"),
        ("u 1 satu", "u jednom satu"),
        ("u 1 godini", "u jednoj godini"),
        ("do 1 godine", "do jedne godine"),
        ("od 1 sata", "od jednog sata"),
        ("za 1 kunu", "za jednu kunu"),
        ("u 1 ujutro", "u jedan ujutro"),
        ("1 i 2", "jedan i dva"),
        ("stranica 2", "stranica dva"),
        ("T2", "T dva"),
        ("1 €", "jedan euro"),
        ("2 €", "dva eura"),
        ("5 €", "pet eura"),
        ("21 €", "dvadeset jedan euro"),
        ("1,5 €", "jedan zarez pet eura"),
        ("1 $", "jedan dolar"),
        ("1 %", "jedan posto"),
        ("1 °C", "jedan stupanj Celzija"),
        ("2 °C", "dva stupnja Celzija"),
        ("5 °C", "pet stupnjeva Celzija"),
        ("1 min", "jedna min"),
        ("2 min", "dvije min"),
        ("1 kg", "jedan kg"),
        ("-5", "minus pet"),
        ("–10 °C", "minus deset stupnjeva Celzija"),
        ("T-2", "T-dva"),
        ("1 000", "tisuću"),
        ("1.000", "tisuću"),
        ("1.000.000", "milijun"),
        ("1.317", "tisuću tristo sedamnaest"),
        ("1.000,50", "tisuću zarez pedeset"),
        ("1.000 €", "tisuću eura"),
        ("3.14", "tri točka četrnaest"),
        ("1 2 3", "jedan dva tri"),
        ("1. mjesto", "prvo mjesto"),
        ("2. mjesto", "drugo mjesto"),
        ("3. mjesto", "treće mjesto"),
        ("na 1. mjestu", "na prvom mjestu"),
        ("u 2. razredu", "u drugom razredu"),
        ("u 21. stoljeću", "u dvadeset prvom stoljeću"),
        ("3. članak", "treći članak"),
        ("1. stranica", "prva stranica"),
        ("7. listopad", "sedmi listopad"),
        ("7. listopada", "sedmog listopada"),
        ("7. 10.", "sedmog \u{FDD0} desetog."),
        ("7.10.2026.", "sedmog \u{FDD0} desetog \u{FDD0} dvije tisuće dvadeset šeste."),
        ("Danas je 7. 10. 2026.", "Danas je sedmi \u{FDD0} deseti \u{FDD0} dvije tisuće dvadeset šeste."),
        ("1990. godine", "tisuću devetsto devedesete godine"),
        ("u 1990. godini", "u tisuću devetsto devedesetoj godini"),
        ("1991.–2000.", "tisuću devetsto devedeset prve do dvije tisućite."),
        ("15.–20. ožujka", "petnaestog do dvadesetog ožujka"),
        ("3.–4. mjesto", "treće do četvrto mjesto"),
        ("u 7. i 8. razredu", "u sedmom i osmom razredu"),
        ("100. obljetnica", "stota obljetnica"),
        ("u 90. godinama", "u devedesetim godinama"),
        ("konac 80. godina", "konac osamdesetih godina"),
        ("do 3. razreda", "do trećeg razreda"),
        ("osvojio je 1. nagradu", "osvojio je prvu nagradu"),
        ("To je 1. liga", "To je prva liga"),
        ("početak 2. svjetskog rata", "početak drugog svjetskog rata"),
        ("1.", "jedan."),
        ("u 8.30", "u osam \u{FDD0} trideset"),
        ("od 8.30 do 16.00", "od osam \u{FDD0} trideset do šesnaest \u{FDD0} nula nula"),
        ("24:00", "dvadeset četiri \u{FDD0} nula nula"),
        ("Sada je 8.30", "Sada je osam \u{FDD0} trideset"),
        ("2x", "dva puta"),
        ("1x", "jedanput"),
        ("2-godišnji", "dvogodišnji"),
        ("3-godišnja", "trogodišnja"),
        ("10-godišnjica", "desetogodišnjica"),
        ("4-sobni", "četverosobni"),
        ("1-sobni", "jednosobni"),
        ("57-godišnji", "pedesetsedmogodišnji"),
        ("2-3 dana", "dva do tri dana"),
        ("10-15 %", "deset do petnaest posto"),
        ("5-3", "pet-tri"),
    ];
    check_read_as(&mut speaker, &mut failures, "Croatian", CROATIAN);
    assert!(speaker.set_language("sr"));
    const SERBIAN: &[(&str, &str)] = &[
        ("2 kuće", "dve kuće"),
        ("1 žena", "jedna žena"),
        ("u 1 satu", "u jednom satu"),
        ("1 €", "jedan evro"),
        ("2 €", "dva evra"),
        ("5 °C", "pet stepeni Celzijusa"),
        ("1.000", "hiljadu"),
        ("4-sobni", "četvorosobni"),
        ("1. mesto", "prvo mesto"),
        ("u 21. veku", "u dvadeset prvom veku"),
        ("7. oktobra", "sedmog oktobra"),
        ("7. 10. 2026.", "sedmog \u{FDD0} desetog \u{FDD0} dve hiljade dvadeset šeste."),
        ("2 куће", "dve kuće"),
        ("у 1 сату", "u jednom satu"),
        ("на 1. месту", "na prvom mestu"),
        ("7. октобра", "sedmog oktobra"),
        ("у 21. веку", "u dvadeset prvom veku"),
    ];
    check_read_as(&mut speaker, &mut failures, "Serbian", SERBIAN);
    assert!(speaker.set_language("bs"));
    const BOSNIAN: &[(&str, &str)] = &[
        ("2 kuće", "dvije kuće"),
        ("1 €", "jedan euro"),
        ("5 °C", "pet stepeni Celzijusa"),
        ("1.000", "hiljadu"),
        ("4-sobni", "četvorosobni"),
        ("7. 10. 2026.", "sedmog \u{FDD0} desetog \u{FDD0} dvije hiljade dvadeset šeste."),
    ];
    check_read_as(&mut speaker, &mut failures, "Bosnian", BOSNIAN);
    failures.finish();
}

// =============================================================================
// Accents: the lexicon and its paradigms
// =============================================================================

/// A word and the same word with its accent written out.
type Accented = (&'static str, &'static str);

/// Every plain text must be spoken, and sound like its accented spelling.
#[track_caller]
fn expect_accents(speaker: &mut Speaker, cases: &[Accented]) {
    let mut failures = Failures::new();
    for &(plain, accented) in cases {
        let audio = speaker.say(plain);
        failures.check(!audio.is_empty(), || format!("{plain}: no audio"));
        failures.check(audio == speaker.say(accented), || format!("{plain} does not sound like {accented}"));
    }
    failures.finish();
}

/// Every plain text must sound like its accented spelling.
#[track_caller]
fn expect_same(speaker: &mut Speaker, cases: &[Accented]) {
    let mut failures = Failures::new();
    for &(plain, accented) in cases {
        failures
            .check(speaker.say(plain) == speaker.say(accented), || format!("{plain} does not sound like {accented}"));
    }
    failures.finish();
}

/// No plain text may sound like the accented spelling next to it.
#[track_caller]
fn expect_different(speaker: &mut Speaker, cases: &[Accented]) {
    let mut failures = Failures::new();
    for &(plain, accented) in cases {
        failures.check(speaker.say(plain) != speaker.say(accented), || format!("{plain} sounds like {accented}"));
    }
    failures.finish();
}

#[test]
fn lexicon_accents_cover_the_whole_paradigm() {
    // A word from the lexicon must sound like the same word with its accent
    // written out, in the oblique cases as well as in the dictionary form.
    let cases: &[Accented] = &[
        ("razlikujemo", "rázlikujemo"),
        ("svetac", "svétac"),
        ("sveci", "svéci"),
        ("svetaca", "svȇtaca"),
        ("nadati", "nádati"),
        ("nadam se", "nȃdam se"),
        ("nadaju se", "nádaju se"),
        ("nadali", "nádali"),
        ("ponadati", "ponádati"),
        ("ponadali", "ponádali"),
        ("ponadam se", "pònadam se"),
        ("ponadaju se", "ponádaju se"),
        ("apsurdni", "àpsurdni"),
        ("društvo", "drúštvo"),
        ("višednevni", "vȉšednevni"),
        ("višednevnog", "vȉšednevnog"),
        ("pojedini", "pȍjedini"),
        ("pojedinima", "pȍjedinima"),
        ("ostare", "òstare"),
        ("ostarim", "òstarim"),
        ("ostarjeti", "òstarjeti"),
        ("ostario", "òstario"),
        ("ostarjela", "òstarjela"),
        ("obavijest", "ȍbavijest"),
        ("obavijesti", "ȍbavijesti"),
        ("obavijestima", "ȍbavijestima"),
        ("obavijestiti", "obavijéstiti"),
        ("obavijestio", "obavijéstio"),
        ("obavijestim", "obavijéstīm"),
        ("dodatno", "dȍdatno"),
        ("dodatnih", "dȍdatnih"),
        ("mogućnost", "mogúćnōst"),
        ("mogućnosti", "mogúćnosti"),
        ("mogućnostima", "mogúćnostima"),
        ("kapacitet", "kapacitét"),
        ("kapaciteta", "kapacitéta"),
        ("kapacitetima", "kapacitétima"),
        ("kontrolni", "kòntrolni"),
        ("kontrolnog", "kòntrolnog"),
        ("podatak", "podátak"),
        ("podaci", "podáci"),
        ("podacima", "podácima"),
        ("podatke", "podátke"),
        ("signal", "sìgnal"),
        ("signala", "signála"),
        ("signalu", "signálu"),
        ("signalom", "signálom"),
        ("signale", "signále"),
        ("pozadina", "pòzadina"),
        ("pozadinu", "pòzadinu"),
        ("privatnost", "privátnōst"),
        ("privatnosti", "privátnosti"),
        ("sigurnost", "sigúrnōst"),
        ("sigurnošću", "sigúrnošću"),
        ("novčanik", "novčànīk"),
        ("novčanici", "novčaníci"),
        ("novčanika", "novčaníka"),
        ("proslijediti", "proslijéditi"),
        ("proslijedi", "proslijédi"),
        ("proslijedio", "proslijédio"),
        ("proslijedim", "pròslijedim"),
        ("proslijedite", "proslijédite"),
        ("proslijeđeno", "pròslijeđeno"),
        ("prosljeđujem", "prosljèđujem"),
        ("prosljeđivati", "prosljeđívati"),
        ("podijeli", "podijéli"),
        ("podijelio", "podijélio"),
        ("podijelim", "pòdijelim"),
        ("podijeljeno", "pòdijeljeno"),
        ("raspodijeli", "raspodijéli"),
        ("promijeni", "promijéni"),
        ("zalijepi", "zalijépi"),
        ("primijeni", "primijéni"),
        ("zamijenjen", "zamijénjen"),
        ("upotrijebi", "upotrijébi"),
        ("primijetio", "primijétio"),
        ("pomiješaj", "pomijéšaj"),
        ("zahtijeva", "zahtijéva"),
        ("razumijevanje", "razumijévanje"),
        ("polijeće", "polijéće"),
        ("Zvonko", "Zvónko"),
        ("Zvonka", "Zvónka"),
        ("Zvonkov", "Zvónkov"),
        ("Vlado", "Vládo"),
        ("uredi", "urédi"),
        ("uredio", "urédio"),
        ("uredim", "urédim"),
        ("uređeno", "uréđeno"),
        ("otvori", "otvòri"),
        ("otvorim", "otvòrim"),
        ("otvoren", "otvòren"),
        ("zatvori", "zatvòri"),
        ("isključi", "iskljúči"),
        ("isključeno", "ìsključeno"),
        ("potvrdi", "potvŕdi"),
        ("potvrđeno", "potvŕđeno"),
        ("objavi", "objávi"),
        ("prijavi", "prijávi"),
        ("pročitaj", "pročìtaj"),
        ("pročitao", "pročìtao"),
        ("pokreni", "pokréni"),
        ("pokrenuo", "pokrénuo"),
        ("prikaži", "prikáži"),
        ("prikazan", "prikázan"),
        ("napiši", "napíši"),
        ("odaberi", "odabèri"),
        ("pošalji", "pošàlji"),
        ("preuzmi", "preùzmi"),
        ("unesi", "unèsi"),
        ("prevedi", "prevèdi"),
        ("zadrži", "zadr\u{300}ži"),
        ("omogući", "omogúći"),
        ("završi", "zavŕši"),
        ("ne zaboravi", "ne zabòravi"),
        ("potvrdi lozinku", "potvŕdi lozinku"),
        ("molim potvrdi", "molim potvŕdi"),
        ("uključuje", "ukljùčuje"),
        ("uključujem", "ukljùčujem"),
        ("uključujući", "ukljùčujući"),
        ("prikazuje", "prikàzuje"),
        ("potvrđuje", "potvr\u{300}đuje"),
        ("uređuju", "urèđuju"),
        ("omogućuje", "omogùćuje"),
        ("kupujem", "kùpujem"),
        ("napreduje", "nàpreduje"),
        ("sudjeluju", "sùdjeluju"),
        ("označava", "označáva"),
        ("označavam", "označávam"),
        ("označavaju", "označávaju"),
        ("rješava", "rješáva"),
        ("obavještavam", "obavještávam"),
        ("uživa", "užíva"),
        ("pokrivaju", "pokrívaju"),
        ("oluje", "olúje"),
        ("telefon", "telèfon"),
        ("telefona", "telefóna"),
        ("telefonom", "telefónom"),
        ("telefonima", "telefónima"),
        ("telefonski", "telèfonski"),
        ("ne znam", "nèznam"),
        ("ne znamo", "nèznamo"),
        ("ne znate", "nèznate"),
        ("ne znaju", "nèznaju"),
        ("ja ne znam", "ja nèznam"),
        ("rođenje", "rođénje"),
        ("rođenja", "rođénja"),
        ("rođenju", "rođénju"),
        ("rođenjem", "rođénjem"),
        ("rođenjima", "rođénjima"),
        ("rješenje", "rješénje"),
        ("sniženje", "snižénje"),
        ("mikrofon", "mȉkrofon"),
        ("mikrofona", "mȉkrofona"),
        ("mikrofonom", "mȉkrofonom"),
        ("mikrofonima", "mȉkrofonima"),
        ("poruka", "pȍruka"),
        ("poruke", "pȍruke"),
        ("poruci", "pȍruci"),
        ("porukom", "pȍrukom"),
        ("porukama", "pȍrukama"),
        ("preporuka", "prȅporuka"),
        ("Tijana", "Tìjana"),
        ("Tijanom", "Tìjanom"),
        ("Tijanama", "Tìjanama"),
        ("Zvonimir", "Zvȍnimir"),
        ("Zvonimira", "Zvȍnimira"),
        ("Zvonimirom", "Zvȍnimirom"),
        ("emotikon", "emòtikon"),
        ("emotikona", "emòtikona"),
        ("emotikonima", "emòtikonima"),
        ("nepročitan", "nepròčitan"),
        ("nepročitane", "nepròčitane"),
        ("nepročitanih", "nepròčitanih"),
        ("nepreslušan", "neprèslušan"),
        ("nepreslušane", "neprèslušane"),
        ("nepregledan", "nȅpregledan"),
        ("nepregledanih", "nȅpregledanih"),
        ("nepregledno", "nȅpregledno"),
        ("zaustavljanje", "zaùstavljanje"),
        ("zaustavljanja", "zaùstavljanja"),
        ("zaustavljati", "zaùstavljati"),
        ("nepoznata", "nȅpoznata"),
        ("nepotpisana", "nepòtpīsana"),
        ("neispunjen", "neìspunjen"),
        ("Nataša", "Nàtaša"),
        ("Natašom", "Nàtašom"),
        ("sad", "sȁd"),
        ("sada", "sȁda"),
        ("Novi Sad", "Novi Sȃd"),
        ("Novog Sada", "Novog Sáda"),
        ("dvadeset", "dvádeset"),
        ("dvadeseti", "dvádeseti"),
        ("dvadeset jedan", "dvádeset jedan"),
        ("trideset", "trîdeset"),
        ("pedeset", "pedèset"),
        ("pedeseti", "pedèseti"),
        ("nula", "nȕla"),
        ("nule", "nȕle"),
        ("nulti", "nȕlti"),
        ("četiristo", "čȅtiristo"),
        ("tisuća", "tìsuća"),
        ("tisuću", "tìsuću"),
        ("tisuće", "tìsuće"),
        ("tisućiti", "tìsućiti"),
        ("dvije tisuće", "dvije tìsuće"),
        ("dvadeset jedna tisuća", "dvadeset jedna tìsuća"),
        ("pet tisuća", "pet tȉsūćā"),
        ("dvadeset tisuća", "dvadeset tȉsūćā"),
        ("sto tisuća", "sto tȉsūćā"),
        ("milijarda", "milìjarda"),
        ("milijarde", "milìjarde"),
        ("milijardi", "milìjardi"),
        ("milijunti", "milìjunti"),
        ("peti", "pȇti"),
        ("peta", "pȇta"),
        ("petog", "pȇtog"),
        ("šesti", "šȇsti"),
        ("sedmi", "sȇdmi"),
        ("sedmog", "sȇdmog"),
        ("osmi", "ȏsmi"),
        ("osma", "ȏsma"),
        ("deveti", "dèveti"),
        ("devetoga", "dèvetoga"),
        ("deseti", "dèseti"),
        ("desetog", "dèsetog"),
        ("stoti", "stȏti"),
        ("dvjestoti", "dvjȅstoti"),
        ("dvoje", "dvȍje"),
        ("troje", "trȍje"),
        ("oba", "ȍba"),
        ("obje", "ȍbje"),
        ("dvojica", "dvòjica"),
        ("četvorica", "četvòrica"),
        ("inače", "ȉnače"),
        ("uostalom", "uòstalom"),
        ("ionako", "ionàko"),
        ("slobodno", "slȍbodno"),
        ("slobodan", "slȍbodan"),
        ("slobodnih", "slȍbodnih"),
        ("slobodnima", "slȍbodnima"),
        ("sloboda", "slobòda"),
        ("Slobodana", "Slobòdana"),
        ("snaga", "snága"),
        ("snazi", "snázi"),
        ("snagama", "snágama"),
        ("snažan", "snážan"),
        ("snažno", "snážno"),
        ("legalan", "lègalan"),
        ("legalno", "lègalno"),
        ("legalnih", "lègalnih"),
        ("legalnost", "legálnōst"),
        ("ilegalan", "ȉlegalan"),
        ("ilegalno", "ȉlegalno"),
        ("jednostavan", "jȅdnostavan"),
        ("jednostavno", "jȅdnostavno"),
        ("jednostavnije", "jednostàvnije"),
        ("jednostavnost", "jednostávnōst"),
        ("Instagram", "Ȉnstagram"),
        ("Instagramu", "Ȉnstagramu"),
        ("ubrzanje", "ubrzánje"),
        ("ubrzanja", "ubrzánja"),
        ("ubrzanjem", "ubrzánjem"),
        ("ubrzano", "ȕbrzano"),
        ("podesivo", "podèsivo"),
        ("podesivih", "podèsivih"),
        ("nepodesiv", "nepodèsiv"),
        ("podesivost", "podesívost"),
        ("monoton", "mȍnoton"),
        ("monotono", "mȍnotono"),
        ("monotonih", "mȍnotonih"),
        ("monotonija", "monotònija"),
        ("pauza", "pàuza"),
        ("pauze", "pàuze"),
        ("pauzama", "pàuzama"),
        ("sauna", "sȁuna"),
        ("fauna", "fàuna"),
        ("korisnički", "kòrisnički"),
        ("korisničkih", "kòrisničkih"),
        ("korisničkim", "kòrisničkim"),
        ("korisnik", "kòrisnik"),
        ("korisnici", "kòrisnici"),
        ("koristan", "kȍristan"),
        ("korisno", "kȍrisno"),
        ("korist", "kȍrist"),
        ("redak", "rédak"),
        ("retka", "rétka"),
        ("redka", "rédka"),
        ("retku", "rétku"),
        ("retkom", "rétkom"),
        ("recima", "récima"),
        ("redaka", "rȇdaka"),
        ("rođendan", "rȍđendan"),
        ("rođendana", "rȍđendana"),
        ("rođendanima", "rȍđendanima"),
        ("zauzet", "zȁuzet"),
        ("zauzeta", "zȁuzeta"),
        ("zauzeto", "zȁuzeto"),
        ("zauzetog", "zȁuzetog"),
        ("zauzetost", "zȁuzetost"),
        ("zauzeo", "zȁuzeo"),
        ("zauzela", "zȁuzela"),
        ("zauzeti", "zaùzeti"),
        ("zauzeće", "zauzéće"),
        // The list of October 2026 (docs/formant.md, "Stress")
        ("potpuno", "pòtpuno"),
        ("maslinovu", "mȁslinovu"),
        ("mobitel", "mòbitel"),
        ("mobitela", "mòbitela"),
        ("oaza", "oáza"),
        ("raketa", "rakèta"),
        ("kralj", "krȃlj"),
        ("kralja", "králja"),
        ("kraljevi", "králjevi"),
        ("uistinu", "ùistinu"),
        ("higijena", "higijéna"),
        ("javno", "jȃvno"),
        ("srednja", "srȅdnja"),
        ("ekipa", "ekípa"),
        ("inspirirana", "inspìrirana"),
        ("inspirirati", "inspirírati"),
        ("uključeno", "ùključeno"),
        ("uključen", "ùključen"),
        ("uključim", "ùključim"),
        ("uključi", "ukljúči"),
        ("željezničar", "žèljezničar"),
        ("streljaštvo", "streljáštvo"),
        ("direktno", "dirèktno"), // listener-selected HJP variant
        ("direktan", "dirèktan"),
        ("direktnima", "dirèktnima"),
        ("siguran", "sȉguran"), // Croatian omits unstressed length
        ("sigurno", "sȉgurno"),
        ("sigurnima", "sȉgurnima"),
        // Pacijentima–duša: verify the accents survive synthesis too.
        ("pacijentima", "pacìjentima"),
        ("pacijenata", "pacìjenata"),
        ("veseli ljudi", "vȅseli ljudi"),
        ("to me veseli", "to me vesèli"),
        ("zaštita", "zàštita"),
        ("posjekotina", "posjekòtina"),
        ("nepravilnosti", "nepravílnosti"),
        ("nježno", "njȅžno"),
        ("poliklinika", "poliklìnika"),
        ("cedevita", "cedevìta"), // tone inferred; source marks short i
        ("dostojanstvo", "dostojànstvo"),
        ("gladan", "glȃdan"),
        ("glad", "glȃd"),
        ("dana", "dȃna"),
        ("pet dana", "pet dána"),
        ("biće", "bíće"),
        ("dijeta", "dijéta"),
        ("duša", "dúša"),
        ("bez duše", "bez dúše"),
        ("Dženana", "Dženàna"), // user stress; provisional short vowel
        ("najdraži", "nȃjdraži"),
        ("međunarodni", "međunárodni"),
        ("održivi", "odr\u{300}živi"),
        ("kampanji", "kampànji"),
        ("zajednički", "zȁjednički"),
        ("obrazovanje", "òbrazovanje"),
        ("vid", "vȋd"),
        ("vidu", "vídu"),
        ("iskustvo", "iskústvo"),
        ("iskustava", "ìskustava"),
        ("alat", "àlat"),
        ("alati", "aláti"),
        ("model", "mòdel"),
        ("modeli", "modèli"),
        ("razvijaju", "razvíjaju"),
        ("razvijen", "razvìjen"),
        ("raspoređeni", "raspòređeni"),
        ("rasporediti", "rasporéditi"),
        ("programersko", "progràmersko"),
        ("ekran", "èkran"),
        ("ekrana", "ekrána"),
        ("zaslon", "záslon"),
        ("osnažiti", "osnážiti"),
        ("potaknuti", "potàknuti"),
        ("potakni", "potàkni"),
        ("potaknem", "pòtaknem"),
        ("suradnja", "surádnja"),
        ("saradnja", "saràdnja"),
        ("istovremeno", "istòvremeno"),
        ("znanje", "znánje"),
        ("moderator", "modèrator"),
        ("moderatorica", "modèratorica"),
        ("sudjelujete", "sùdjelujete"),
        ("učestvujete", "ùčestvujete"),
        ("sudjelovati", "sùdjelovati"),
        ("ja", "jȃ"),
        ("ti", "tȋ"),
        ("on", "ȏn"),
        ("mi", "mȋ"),
        ("vi", "vȋ"),
        ("oni", "òni"),
        ("njih", "njȋh"),
        ("enida", "enída"), // listener's long stressed i
        ("brkljača", "bȑkljača"),
        ("ivanščica", "ivànščica"),
        ("teodora", "teodóra"),
        ("crnogorac", "crnògorac"),
        ("crnogorci", "crnogórci"),
        ("milosava", "mȉlosava"),
        ("markovina", "márkovina"),
        ("prugovečki", "prȗgovečki"),
        ("danijel", "dànijel"),
        ("daniel", "dàniel"),
        ("danijela", "dànijela"),
        ("rotić", "rȍtić"),
        ("tihić", "tȉhić"),
        ("letić", "lȅtić"),
        ("matić", "mátić"),
        ("jusić", "jȕsić"),
        ("mato", "máto"),
        ("maja", "mája"),
        ("anita", "ànita"),
        ("sandra", "sȃndra"),
        ("siniša", "sìniša"),
        ("mihajlo", "mìhajlo"),
        ("naida", "nàida"),
        ("vladić", "vládić"),
        ("eva", "ȇva"),
        ("adrijana", "adrijàna"),
        ("aleksandar", "aleksándar"),
        ("proteza", "protéza"),
        ("protezu", "protézu"),
        ("protezama", "protézama"),
        ("protetičar", "protètičar"),
        ("zubna", "zȗbna"),
        ("očna", "òčna"),
        ("javnost", "jávnost"),
        ("javnosti", "jávnosti"),
        ("potpunosti", "pòtpunosti"),
        ("iskustveni", "iskùstveni"),
        ("najbolji", "nȃjbolji"),
        ("najbolje", "nȃjbolje"),
        ("najveći", "nȃjveći"),
        ("najmanje", "nȃjmanje"),
        ("najviše", "nȃjviše"),
        ("najjači", "nȃjjači"),
        ("najbrži", "nȃjbrži"),
        ("najljepši", "nȃjljepši"),
        ("najčešće", "nȃjčešće"),
        ("najprije", "nȃjprije"),
        ("najposlije", "nȃjposlije"),
        ("najgori", "nȃjgori"),
        ("najnoviji", "najnòviji"),
        ("najvažniji", "najvàžniji"),
        ("najstariji", "najstàriji"),
        ("najjednostavniji", "najjednostàvniji"),
        ("odabir", "ȍdabir"),
        ("odabiru", "ȍdabiru"),
        ("odabrano", "ȍdabrano"),
        ("odabrao", "ȍdabrao"),
        ("odaberem", "odàberem"),
        ("odaberi", "odabèri"),
        ("odabirati", "odàbirati"),
        ("odabirem", "odàbirem"),
        ("asistenta", "asìstenta"),
        ("asistenata", "asìstenata"),
        ("učenik", "ȕčenik"),
        ("učenika", "ȕčenika"),
        ("učenici", "ȕčenici"),
        ("izgled", "ȉzgled"),
        ("izgleda", "ȉzgleda"),
        ("izgledam", "ìzgledam"),
        ("izgledaju", "izglédaju"),
        ("komentar", "komèntar"),
        ("komentara", "komentára"),
        ("prekini", "prèkini"),
        ("prekinuo", "prèkinuo"),
        ("prekinut", "prèkinut"),
        ("prekinem", "prèkinem"),
        ("život", "žìvot"),
        ("života", "živòta"),
        ("živote", "živòte"),
        ("životinja", "živòtinja"),
        ("spava", "spȃva"),
        ("spavati", "spávati"),
        ("spavao", "spávao"),
        ("nema", "nȇma"),
        ("nemam", "nȇmam"),
        ("nemaju", "némaju"),
        ("lice", "líce"),
        ("lica", "líca"),
        ("kredit", "krèdit"),
        ("kredita", "kredíta"),
        ("stavka", "stȃvka"),
        ("stavke", "stȃvke"),
        ("krećete", "krȇćete"),
        ("kreće", "krȇće"),
        ("kreći", "kréći"),
        ("kretanje", "krétanje"),
        ("kreni", "kréni"),
        ("krenem", "krȇnem"),
        ("krenuo", "krénuo"),
        ("prikvači", "prìkvači"),
        ("prikvačen", "prìkvačen"),
        ("otkvači", "òtkvači"),
        ("dokumentarac", "dokumentárac"),
        ("dokumentarca", "dokumentárca"),
        ("vlast", "vlȃst"),
        ("vlasti", "vlȃsti"),
        ("na vlasti", "na vlásti"),
        ("novac", "nòvac"),
        ("novcem", "nóvcem"),
        ("novci", "nȏvci"),
        ("sustav", "sústav"),
        ("sustava", "sústava"),
        ("radnja", "rádnja"),
        ("radnje", "rádnje"),
        ("dar", "dȃr"),
        ("darovi", "dȁrovi"),
        ("o daru", "o dáru"),
        ("važi", "vȃži"),
        ("važiti", "vážiti"),
        ("važeći", "vážeći"),
    ];
    let mut speaker = Speaker::new("zvonko");
    expect_accents(&mut speaker, cases);

    // The noun "obavijesti" and the verb's infinitive differ in stress.
    assert!(speaker.say("obavijesti") != speaker.say("obavijestiti"));

    // The short vowel of sȁd stays clearly shorter than the long one of
    // sȃd at the end of a clause (the only difference between the two).
    {
        let kratko = speaker.say("sad");
        let dugo = speaker.say("sȃd");
        info!("sad {} samples, sȃd {} samples", kratko.len(), dugo.len());
        assert!(dugo.len() > kratko.len() + 22050 * 80 / 1000);
    }

    // Verbs with a long "ije" after a prefix are found by their root. Nouns,
    // adjectives and other verbs that only look like them keep their accent.
    let not_these: &[Accented] = &[
        ("rođendan", "rođéndan"),
        ("učenje", "učénje"),
        ("država", "držáva"),
        ("zabava", "zabáva"),
        ("predstava", "predstáva"),
        ("osjetljiva", "osjetljíva"),
        ("dodaje", "dodáje"),
        ("povijesti", "povijésti"),
        ("prelijepi", "prelijépi"),
        ("donijeli", "donijéli"),
        ("zapovijedi", "zapovijédi"),
    ];
    expect_different(&mut speaker, not_these);

    // "Potvrdi" opens a clause as a command; after a preposition or another
    // word it is the noun, which has priority ("u potvrdi", "svi uredi").
    let nouns: &[Accented] = &[
        ("u potvrdi", "u potvŕdi"),
        ("hvala na objavi", "hvala na objávi"),
        ("svi uredi", "svi urédi"),
        ("nova oprema", "nova opréma"),
    ];
    expect_different(&mut speaker, nouns);

    // Mirsad follows the dictionaries' shifts throughout; Croatian also
    // shifts podijeliti and proslijediti, while keeping the root accent in
    // the other verbs here.
    let bosnian: &[Accented] = &[
        ("podijelim", "pòdijelim"),
        ("podijeljen", "pòdijeljen"),
        ("podijeli", "podijéli"),
        ("podijelio", "podijélio"),
        ("pomiješam", "pòmiješam"),
        ("upotrijebim", "upòtrijebim"),
        ("uredi", "urédi"),
        ("uredim", "ùrēdim"),
        ("uređen", "ùrēđen"),
        ("otvorim", "òtvorim"),
        ("otvorio", "otvòrio"),
        ("pročitan", "pròčitan"),
        ("pokrenut", "pòkrēnut"),
        ("prikažem", "prìkāžem"),
        ("prikaži", "prikáži"),
        ("uključuje", "ukljùčuje"),
        ("označava", "oznàčāva"),
        ("uživam", "ùžīvam"),
        ("rješava", "rjèšāva"),
        ("ne znam", "nèznām"),
        ("ne znamo", "nèznāmo"),
    ];
    speaker.set_voice("mirsad").expect("mirsad");
    expect_same(&mut speaker, bosnian);

    // The common ekavian verbs are listed stem by stem.
    let ekavian: &[Accented] = &[
        ("podelio", "podélio"),
        ("podelim", "pòdēlim"),
        ("zalepi", "zalépi"),
        ("pomešao", "poméšao"),
        ("prosledi", "proslédi"),
    ];
    speaker.set_voice("stojan").expect("stojan");
    expect_same(&mut speaker, ekavian);
}

#[test]
fn croatian_ideja_besplatan_and_podijelim_follow_dictionary_accents() {
    // Mrežnik: idéja (the same accent throughout its cases), bèsplatan.
    // HJP: podijéliti, present pòdijēlīm, passive pòdijēljen.
    // Zvonko keeps its usual omission of post-accent length; the long ije
    // is already supplied by phonemization, including after the accent.
    let cases: &[Accented] = &[
        ("ideja", "idéja"),
        ("ideje", "idéje"),
        ("ideji", "idéji"),
        ("ideju", "idéju"),
        ("idejo", "idéjo"),
        ("idejom", "idéjom"),
        ("idejama", "idéjama"),
        ("besplatan", "bèsplatan"),
        ("besplatna", "bèsplatna"),
        ("besplatno", "bèsplatno"),
        ("besplatni", "bèsplatni"),
        ("besplatne", "bèsplatne"),
        ("besplatnu", "bèsplatnu"),
        ("besplatnog", "bèsplatnog"),
        ("besplatnoga", "bèsplatnoga"),
        ("besplatnom", "bèsplatnom"),
        ("besplatnome", "bèsplatnome"),
        ("besplatnomu", "bèsplatnomu"),
        ("besplatnoj", "bèsplatnoj"),
        ("besplatnih", "bèsplatnih"),
        ("besplatnim", "bèsplatnim"),
        ("besplatnima", "bèsplatnima"),
        ("podijelim", "pòdijelim"),
        ("podijeliš", "pòdijeliš"),
        ("podijelimo", "pòdijelimo"),
        ("podijele", "pòdijele"),
        ("podijeljen", "pòdijeljen"),
        ("podijeljena", "pòdijeljena"),
        ("podijeljeno", "pòdijeljeno"),
        ("podijeljeni", "pòdijeljeni"),
        ("podijeljenih", "pòdijeljenih"),
        ("podijeljenima", "pòdijeljenima"),
        ("nepodijeljen", "nepòdijeljen"),
        ("podijeliti", "podijéliti"),
        ("podijelio", "podijélio"),
        ("podijelila", "podijélila"),
        // Ambiguous -i/-ite forms retain the existing imperative reading.
        ("podijeli", "podijéli"),
        ("podijelite", "podijélite"),
        // Other prefixes must not inherit the podijeliti override.
        ("raspodijelim", "raspodijélim"),
        ("dodijelim", "dodijélim"),
        ("To je ideja", "To je idéja"),
        ("Ulaz je besplatan", "Ulaz je bèsplatan"),
        ("Želim da podijelim ideju", "Želim da pòdijelim idéju"),
    ];
    let mut speaker = Speaker::new("zvonko");
    expect_accents(&mut speaker, cases);
    assert!(speaker.say("ideja") != speaker.say("idèja"));
    assert!(speaker.say("besplatan") != speaker.say("bȅsplatan"));
    assert!(speaker.say("podijelim") != speaker.say("podijélim"));
}

#[test]
fn croatian_proslijediti_shifts_its_present_and_passive_accents() {
    // HJP proslijéditi, pròslijēdīm, pròslijēđen. Croatian omits
    // unstressed length except the ije supplied by phonemization.
    let cases: &[Accented] = &[
        ("proslijediti", "proslijéditi"),
        ("proslijedit ću", "proslijédit ću"),
        ("proslijedim", "pròslijedim"),
        ("proslijediš", "pròslijediš"),
        ("proslijedimo", "pròslijedimo"),
        ("proslijede", "pròslijede"),
        // Ambiguous -i/-ite forms retain the existing imperative reading.
        ("proslijedi", "proslijédi"),
        ("proslijedite", "proslijédite"),
        ("proslijedio", "proslijédio"),
        ("proslijedila", "proslijédila"),
        ("proslijedilo", "proslijédilo"),
        ("proslijedili", "proslijédili"),
        ("proslijedile", "proslijédile"),
        ("proslijedivši", "proslijédivši"),
        ("proslijeđen", "pròslijeđen"),
        ("proslijeđena", "pròslijeđena"),
        ("proslijeđeno", "pròslijeđeno"),
        ("proslijeđeni", "pròslijeđeni"),
        ("proslijeđene", "pròslijeđene"),
        ("proslijeđenu", "pròslijeđenu"),
        ("proslijeđenog", "pròslijeđenog"),
        ("proslijeđenoga", "pròslijeđenoga"),
        ("proslijeđenom", "pròslijeđenom"),
        ("proslijeđenome", "pròslijeđenome"),
        ("proslijeđenomu", "pròslijeđenomu"),
        ("proslijeđenoj", "pròslijeđenoj"),
        ("proslijeđenih", "pròslijeđenih"),
        ("proslijeđenim", "pròslijeđenim"),
        ("proslijeđenima", "pròslijeđenima"),
        ("neproslijeđen", "nepròslijeđen"),
        ("neproslijeđena", "nepròslijeđena"),
        ("Poruka je proslijeđena", "Poruka je pròslijeđena"),
        ("Želim da proslijedim poruku", "Želim da pròslijedim poruku"),
        ("Proslijedi poruku", "Proslijédi poruku"),
        // Other prefixes and the imperfective retain their existing rules.
        ("naslijedim", "naslijédim"),
        ("naslijeđen", "naslijéđen"),
        ("prosljeđivati", "prosljeđívati"),
        ("prosljeđujem", "prosljèđujem"),
    ];
    let mut speaker = Speaker::new("zvonko");
    expect_accents(&mut speaker, cases);
    assert!(speaker.say("proslijediti") != speaker.say("pròslijediti"));
    assert!(speaker.say("proslijedim") != speaker.say("proslijédim"));
    assert!(speaker.say("proslijeđen") != speaker.say("proslijéđen"));
}

#[test]
fn croatian_sucelje_and_otici_keep_their_lexical_accents() {
    // HJP: súčēlje; HJP / Školski rječnik: òtīći, òtišao, òtišla.
    // Croatian omits unstressed length, but the stressed u stays long.
    let cases: &[Accented] = &[
        ("sučelje", "súčelje"),
        ("sučelja", "súčelja"),
        ("sučelju", "súčelju"),
        ("sučeljem", "súčeljem"),
        ("sučeljima", "súčeljima"),
        ("otići", "òtići"),
        ("otišao", "òtišao"),
        ("otišla", "òtišla"),
        ("otišlo", "òtišlo"),
        ("otišli", "òtišli"),
        ("otišle", "òtišle"),
        ("otišavši", "òtišavši"),
        ("Korisničko sučelje", "Korisničko súčelje"),
        ("Otići ću kući", "Òtići ću kući"),
        ("Otišao je kući", "Òtišao je kući"),
        ("Otišla je kući", "Òtišla je kući"),
        ("Oni su otišli", "Oni su òtišli"),
    ];
    let mut speaker = Speaker::new("zvonko");
    expect_accents(&mut speaker, cases);
    assert!(speaker.say("sučelje") != speaker.say("sùčelje"));
    assert!(speaker.say("sučelje") != speaker.say("sȗčelje"));
    assert!(speaker.say("otići") != speaker.say("otíći"));
    assert!(speaker.say("otišao") != speaker.say("otìšao"));
    assert!(speaker.say("otišla") != speaker.say("ȍtišla"));
}

#[test]
fn croatian_sat_and_rad_distinguish_case_accents() {
    // Školski rječnik / HJP noun paradigms, including D vs L sg and
    // N/V vs G pl sati. Stressed length stays; unstressed length is omitted.
    let cases: &[Accented] = &[
        ("sat", "sȃt"),
        ("sata", "sȃta"),
        ("satu", "sȃtu"),
        ("sate", "sȃte"),
        ("satom", "sȃtom"),
        ("sati", "sȃti"),
        ("satima", "sátima"),
        ("satovi", "sȁtovi"),
        ("satova", "sȁtova"),
        ("satove", "sȁtove"),
        ("satovima", "sȁtovima"),
        ("rad", "rȃd"),
        ("rada", "rȃda"),
        ("radu", "rȃdu"),
        ("rade", "rȃde"),
        ("radom", "rȃdom"),
        ("radovi", "rȁdovi"),
        ("radova", "rȁdova"),
        ("radove", "rȁdove"),
        ("radovima", "rȁdovima"),
        ("Dva sata", "Dva sȃta"),
        ("Pet sati", "Pet sáti"),
        ("Dvanaest sati", "Dvanaest sáti"),
        ("25 sati", "25 sáti"),
        ("Nekoliko sati", "Nekoliko sáti"),
        ("Do kasnih sati", "Do kasnih sáti"),
        ("Pet radnih sati", "Pet radnih sáti"),
        ("Koliko je sati", "Koliko je sáti"),
        ("Sati prolaze", "Sȃti prolaze"),
        ("To su dugi sati", "To su dugi sȃti"),
        ("Na satu", "Na sátu"),
        ("Na tom satu", "Na tom sátu"),
        ("Prema satu", "Prema sȃtu"),
        ("Na radu", "Na rádu"),
        ("O radu", "O rádu"),
        ("Pri radu", "Pri rádu"),
        ("U radu", "U rádu"),
        ("Po radu", "Po rádu"),
        ("O svom novom radu", "O svom novom rádu"),
        ("O našem radu", "O našem rádu"),
        ("Pristup radu", "Pristup rȃdu"),
        ("Prema dobrom radu", "Prema dobrom rȃdu"),
        ("Na poslu se posvetio radu", "Na poslu se posvetio rȃdu"),
        ("Nema rada", "Nema rȃda"),
        ("Oni rade", "Oni rȃde"),
        // Exact paradigms must not catch the verb or unrelated sat- words.
        ("raditi", "ráditi"),
        ("radila", "rádila"),
        ("satelit", "satèlit"),
    ];
    let mut speaker = Speaker::new("zvonko");
    expect_accents(&mut speaker, cases);
    assert!(speaker.say("sati") != speaker.say("sáti"));
    assert!(speaker.say("pet sati") != speaker.say("pet sȃti"));
    assert!(speaker.say("o radu") != speaker.say("o rȃdu"));
    assert!(speaker.say("prema radu") != speaker.say("prema rádu"));
    assert!(speaker.say("satovi") != speaker.say("sȃtovi"));

    // User entries override these contextual guesses; written accents
    // still take precedence over a user entry.
    let json = r#"{"entries":[{"word":"s^a:ti"},{"word":"r^a:du"},{"word":"s^a:tu"}]}"#;
    assert!(speaker.load_accent_lexicon(json));
    assert!(speaker.say("pet sati") == speaker.say("pet sȃti"));
    assert!(speaker.say("o radu") == speaker.say("o rȃdu"));
    assert!(speaker.say("na satu") == speaker.say("na sȃtu"));
    assert!(speaker.say("o radu") != speaker.say("o rádu"));
    speaker.clear_accent_lexicon();
    assert!(speaker.say("o radu") == speaker.say("o rádu"));
}

#[test]
fn accent_lexicon_entries_can_be_checked_without_an_engine() {
    // What the dictionary editors call before saving an entry: the same
    // parser as the load, with the reason of the first rejection (empty
    // when nothing was rejected, as the C API hands it out).
    let check = |json: &str| {
        let (_, report) = UserLexicon::parse(json);
        let message = if report.rejected > 0 { report.first_error } else { String::new() };
        (report.rejected, message)
    };
    let (rejected, message) = check(r#"{"entries":[{"word":"kontr'o:l*"}]}"#);
    assert_eq!(rejected, 0);
    assert!(message.is_empty(), "{message}");
    assert_eq!(check(r#"{"entries":[{"verb":"ur'e:d=i<p"},{"verb":"dijel:i","language":"hr"}]}"#).0, 0);
    assert_eq!(check(r#"{"entries":[{"word":"sign'a:l|a|u|om|e|i|ima"}]}"#).0, 0);

    // No stress mark at all
    let (rejected, message) = check(r#"{"entries":[{"word":"kontrola"}]}"#);
    assert_eq!(rejected, 1);
    assert!(message.starts_with("kontrola"), "{message}");
    // Unknown language, two bad entries counted
    let (rejected, message) = check(r#"{"entries":[{"word":"k'ontrola","language":"de"},{"verb":"uredi"}]}"#);
    assert_eq!(rejected, 2);
    assert!(message.contains("language"), "{message}");
    // Neither word nor verb
    assert_eq!(check(r#"{"entries":[{"comment":"x"}]}"#).0, 1);
}

#[test]
fn croatian_pobjednik_and_dvadeset_families_follow_dictionary_accents() {
    // HJP pòbjednīk/pòbjedničkī; Mrežnik dvádesēt/dvádesētī/dvadesétak;
    // Školski rječnik dvádesetero. Croatian omits unstressed lengths.
    let cases: &[Accented] = &[
        ("pobjednik", "pòbjednik"),
        ("pobjednika", "pòbjednika"),
        ("pobjedniku", "pòbjedniku"),
        ("pobjednikom", "pòbjednikom"),
        ("pobjedniče", "pòbjedniče"),
        ("pobjednici", "pòbjednici"),
        ("pobjednicima", "pòbjednicima"),
        ("pobjednike", "pòbjednike"),
        ("pobjednička", "pòbjednička"),
        ("pobjednički", "pòbjednički"),
        ("pobjedničko", "pòbjedničko"),
        ("pobjedničke", "pòbjedničke"),
        ("pobjedničku", "pòbjedničku"),
        ("pobjedničkog", "pòbjedničkog"),
        ("pobjedničkoga", "pòbjedničkoga"),
        ("pobjedničkom", "pòbjedničkom"),
        ("pobjedničkome", "pòbjedničkome"),
        ("pobjedničkomu", "pòbjedničkomu"),
        ("pobjedničkoj", "pòbjedničkoj"),
        ("pobjedničkih", "pòbjedničkih"),
        ("pobjedničkim", "pòbjedničkim"),
        ("pobjedničkima", "pòbjedničkima"),
        ("dvadeset", "dvádeset"),
        ("dvadeseti", "dvádeseti"),
        ("dvadeseta", "dvádeseta"),
        ("dvadeseto", "dvádeseto"),
        ("dvadesete", "dvádesete"),
        ("dvadesetu", "dvádesetu"),
        ("dvadesetog", "dvádesetog"),
        ("dvadesetoga", "dvádesetoga"),
        ("dvadesetom", "dvádesetom"),
        ("dvadesetome", "dvádesetome"),
        ("dvadesetomu", "dvádesetomu"),
        ("dvadesetoj", "dvádesetoj"),
        ("dvadesetih", "dvádesetih"),
        ("dvadesetim", "dvádesetim"),
        ("dvadesetima", "dvádesetima"),
        ("dvadesetero", "dvádesetero"),
        ("dvadesetak", "dvadesétak"),
        ("Pobjednička ekipa", "Pòbjednička ekipa"),
        ("On je pobjednik", "On je pòbjednik"),
        ("Čestitam pobjednicima", "Čestitam pòbjednicima"),
        ("Dvadeset pobjednika", "Dvádeset pòbjednika"),
        ("20", "dvádeset"),
        ("21", "dvádeset jedan"),
        ("120", "sto dvádeset"),
        ("Dvadesetak pobjednika", "Dvadesétak pòbjednika"),
    ];
    let mut speaker = Speaker::new("zvonko");
    expect_accents(&mut speaker, cases);
    assert!(speaker.say("pobjednik") != speaker.say("pobjédnik"));
    assert!(speaker.say("pobjednička") != speaker.say("pobjèdnička"));
    assert!(speaker.say("pobjednička") != speaker.say("pȍbjednička"));
    assert!(speaker.say("dvadeset") != speaker.say("dvȃdeset"));
    assert!(speaker.say("dvadesetak") != speaker.say("dvádesetak"));
}

#[test]
fn the_tens_keep_their_long_first_vowel() {
    // The long first vowel prevents the earlier clipped "dva deset".
    // Croatian twenty now has dictionary rising tone; the other voices
    // and thirty retain their earlier falling-accent rendering.
    let mut speaker = Speaker::empty();
    for voice in FORMANT_VOICES {
        speaker.set_voice(voice).expect(voice);
        let plain = speaker.say("dvadeset");
        let short_falling = speaker.say("dvȁdeset");
        let long_rising = speaker.say("dvádeset");
        info!("{voice}: dvadeset {} samples, dvȁdeset {} samples", plain.len(), short_falling.len());
        assert!(plain.len() > short_falling.len() + 22050 / 40, "{voice}"); // 25 ms or more
        assert!(plain != short_falling, "{voice}");
        if voice == "zvonko" {
            assert!(plain == long_rising, "{voice}");
        } else {
            assert!(plain != long_rising, "{voice}");
            assert!(plain == speaker.say("dvȃdeset"), "{voice}");
        }
        let thirty = speaker.say("trideset");
        assert!(thirty.len() > speaker.say("trȉdeset").len() + 22050 / 40, "{voice}");
    }
}

#[test]
fn collected_croatian_accents_retain_tone_and_inflection() {
    // Školski rječnik and HJP, checked 2026-10-08; see docs/formant.md.
    // Audio must equal explicitly accented input, including related forms
    // whose stress differs. Croatian's existing unstressed-length policy
    // is retained. "tekučina" in the report is the typo "tekućina".
    let cases: &[Accented] = &[
        ("dolar", "dȍlar"),
        ("dolara", "dȍlara"),
        ("dolari", "dȍlari"),
        ("dolarima", "dȍlarima"),
        ("euro", "ȅuro"),
        ("eura", "ȅura"),
        ("eurima", "ȅurima"),
        ("naravno", "náravno"),
        ("naravan", "náravan"),
        ("naravnoga", "náravnoga"),
        ("priča", "prȋča"),
        ("priču", "prȋču"),
        ("pričama", "prȋčama"),
        ("pričati", "príčati"),
        ("pričao", "príčao"),
        ("pričam", "prȋčam"),
        ("pričaj", "prȋčaj"),
        ("pričaju", "príčaju"),
        ("pričan", "prȋčan"),
        ("djelomično", "djȅlomično"),
        ("djelomičan", "djȅlomičan"),
        ("djelomičnoga", "djȅlomičnoga"),
        ("činjenica", "čȉnjenica"),
        ("činjenicama", "čȉnjenicama"),
        ("nemoguće", "nȅmoguće"),
        ("nemogućega", "nȅmogućega"),
        ("nemogućnost", "nemogúćnōst"),
        ("popodne", "popódne"),
        ("popodneva", "popódneva"),
        ("popodnevima", "popódnevima"),
        ("popodnevni", "popódnevni"),
        ("proslaviti", "pròslaviti"),
        ("proslavimo", "pròslavimo"),
        ("proslavite", "pròslavite"),
        ("proslavio", "pròslavio"),
        ("proslavljen", "pròslavljen"),
        ("proslavljenima", "pròslavljenima"),
        ("proslava", "prȍslava"),
        ("na proslavi", "na prȍslavi"),
        ("europska", "èuropska"),
        ("europskoga", "èuropskoga"),
        ("europskima", "èuropskima"),
        ("Europa", "Európa"),
        ("Europe", "Európe"),
        ("osiguranja", "osiguránja"),
        ("osiguranje", "osiguránje"),
        ("osiguranjima", "osiguránjima"),
        ("veselimo", "vesèlimo"),
        ("veseliti", "vesèliti"),
        // Bare veseli now defaults to the user-confirmed adjective; make
        // the older verb checks unambiguous with the reflexive clitic.
        ("veseli se", "vesèli se"),
        ("veselim se", "vesèlim se"),
        ("veselio", "vesèlio"),
        ("veselje", "vesélje"),
        ("automobil", "automòbil"),
        ("automobila", "automobíla"),
        ("automobilima", "automobílima"),
        ("automobilski", "automòbilski"),
        ("automobilskoga", "automòbilskoga"),
        ("tekućina", "tekùćina"),
        ("tekućine", "tekùćine"),
        ("tekućinama", "tekùćinama"),
        ("kafić", "kàfić"),
        ("kafića", "kafíća"),
        ("kafići", "kafíći"),
        ("kafićima", "kafíćima"),
        ("pomoćnik", "pomòćnik"),
        ("pomoćnika", "pomoćníka"),
        ("pomoćnici", "pomoćníci"),
        ("pomoćnicima", "pomoćnícima"),
        ("pomoćniče", "pȍmoćniče"),
        ("pomoćnica", "pomòćnica"),
        ("pomoćnicama", "pomòćnicama"),
        ("pomoćnički", "pomòćnički"),
        ("tamo", "tȁmo"),
        ("prognoza", "prognóza"),
        ("prognozama", "prognózama"),
        ("kabanica", "kabànica"),
        ("kabanicama", "kabànicama"),
        ("kišobran", "kȉšobran"),
        ("kišobrana", "kȉšobrana"),
        ("kišobranu", "kȉšobranu"),
        ("kišobrane", "kȉšobrane"),
        ("kišobranom", "kȉšobranom"),
        ("kišobrani", "kȉšobrani"),
        ("kišobranima", "kȉšobranima"),
        ("ponesi kišobran", "ponesi kȉšobran"),
        ("zaštićen", "zàštićen"),
        ("zaštićena", "zàštićena"),
        ("zaštićeno", "zàštićeno"),
        ("zaštićeni", "zàštićeni"),
        ("zaštićene", "zàštićene"),
        ("zaštićenu", "zàštićenu"),
        ("zaštićenoga", "zàštićenoga"),
        ("zaštićenome", "zàštićenome"),
        ("zaštićenomu", "zàštićenomu"),
        ("zaštićenoj", "zàštićenoj"),
        ("zaštićenih", "zàštićenih"),
        ("zaštićenim", "zàštićenim"),
        ("zaštićenima", "zàštićenima"),
        ("zaštitim", "zàštitim"),
        ("zaštitiš", "zàštitiš"),
        ("zaštitimo", "zàštitimo"),
        ("zaštititi", "zaštítiti"),
        ("zaštitio", "zaštítio"),
        ("zaštiti", "zaštíti"),
        ("zaštitite", "zaštítite"),
        ("nezaštićen", "nezàštīćen"),
        ("nezaštićenima", "nezàštīćenima"),
        ("zaštićenost", "zàštićenost"),
        ("zaštićenošću", "zàštićenošću"),
        ("dokument je zaštićen", "dokument je zàštićen"),
        ("slojevita", "slojèvita"),
        ("slojevit", "slojèvit"),
        ("slojevitima", "slojèvitima"),
        ("slojevitost", "slojèvitost"),
        ("slojevitošću", "slojèvitošću"),
        ("pet dolara", "pet dȍlara"),
        ("dva eura", "dva ȅura"),
        ("veselimo se", "vesèlimo se"),
        ("to je nemoguće", "to je nȅmoguće"),
        ("europska osiguranja", "èuropska osiguránja"),
        ("naši pomoćnici", "naši pomoćníci"),
    ];
    let mut speaker = Speaker::new("zvonko");
    expect_accents(&mut speaker, cases);
}

#[test]
fn kompliciran_keeps_its_adjective_accent_throughout_declension() {
    // Školski rječnik: komplìcīrān, feminine/neuter komplìcīrāna/o,
    // but comparative komplicirànijī. HJP confirms the adverb and noun
    // komplìcīrano, komplìcīranōst. Croatian omits unstressed lengths.
    let cases: &[Accented] = &[
        ("kompliciran", "komplìciran"),
        ("komplicirana", "komplìcirana"),
        ("komplicirano", "komplìcirano"),
        ("komplicirani", "komplìcirani"),
        ("komplicirane", "komplìcirane"),
        ("kompliciranu", "komplìciranu"),
        ("kompliciranog", "komplìciranog"),
        ("kompliciranoga", "komplìciranoga"),
        ("kompliciranom", "komplìciranom"),
        ("kompliciranome", "komplìciranome"),
        ("kompliciranomu", "komplìciranomu"),
        ("kompliciranoj", "komplìciranoj"),
        ("kompliciranih", "komplìciranih"),
        ("kompliciranim", "komplìciranim"),
        ("kompliciranima", "komplìciranima"),
        ("kompliciranost", "komplìciranost"),
        ("kompliciranosti", "komplìciranosti"),
        ("kompliciranostima", "komplìciranostima"),
        ("kompliciranošću", "komplìciranošću"),
        // The comparative and infinitive must retain their later accents.
        ("kompliciraniji", "kompliciràniji"),
        ("kompliciranija", "komplicirànija"),
        ("kompliciranije", "komplicirànije"),
        ("kompliciranijima", "komplicirànijima"),
        ("najkompliciraniji", "najkompliciràniji"),
        ("komplicirati", "komplicírati"),
        ("komplicirao", "komplicírao"),
        ("kompliciranje", "komplicíranje"),
    ];
    let mut speaker = Speaker::new("zvonko");
    expect_accents(&mut speaker, cases);
    assert!(speaker.say("kompliciran") != speaker.say("komplicíran"));
    assert!(speaker.say("komplicirano") != speaker.say("komplícirano"));
}

#[test]
fn october_11_collected_croatian_words_keep_their_verified_accents() {
    // Školski rječnik and HJP, checked 2026-10-11; source differences and
    // inflectional evidence are documented in docs/formant.md. As elsewhere,
    // Croatian omits unstressed lengths while preserving stressed quantity.
    let cases: &[Accented] = &[
        ("pomagalima", "pomágalima"),
        ("pomagalo", "pomágalo"),
        ("nastradalim", "nastrádalim"),
        ("nastradaloj", "nastrádaloj"),
        ("nastradalima", "nastrádalima"),
        ("nastradala", "nastrádala"),
        ("nastradamo", "nàstradamo"),
        ("nastradaju", "nastrádaju"),
        ("nastradaj", "nàstradaj"),
        ("kontakata", "kòntakata"),
        ("kontakt", "kòntakt"),
        ("kontaktima", "kòntaktima"),
        ("riječanka", "rijéčanka"),
        ("Riječanka", "Rijéčanka"),
        ("riječanin", "rijéčanin"),
        ("Riječanin", "Rijéčanin"),
        // Inflections longer than the old three-letter stem allowance.
        ("Riječanina", "Rijéčanina"),
        ("Riječanima", "Rijéčanima"),
        ("Riječankama", "Rijéčankama"),
        ("prostire", "pròstire"),
        ("prostirati", "pròstirati"),
        ("prostiremo", "pròstiremo"),
        ("pronađe", "prònađe"),
        ("pronaći", "prònaći"),
        ("pronađem", "prònađem"),
        ("pronašli", "prònašli"),
        ("pronađi", "pronáđi"),
        ("pribadača", "pribàdača"),
        ("pribadače", "pribàdače"),
        ("odgovornostima", "odgovórnostima"),
        ("odgovornost", "odgovórnost"),
        ("odgovornošću", "odgovórnošću"),
        ("logopedi", "logopédi"),
        ("logoped", "logòped"),
        ("logopeda", "logopéda"),
        ("logopedima", "logopédima"),
        ("logopede", "logopéde"),
        ("logopedija", "logopèdija"),
        ("nas", "nȁs"),
        ("vas", "vȁs"),
        ("tumačenje", "tumáčenje"),
        ("tumačenja", "tumáčenja"),
        ("pedijatri", "pedìjatri"),
        ("pedijatar", "pedìjatar"),
        ("pedijatrima", "pedìjatrima"),
        ("pedijatara", "pedìjatara"),
        ("pedijatrija", "pedijàtrija"),
        ("svijetloplavo", "svijetloplávo"),
        ("svijetloplava", "svijetlopláva"),
        ("svijetloplav", "svijétloplav"),
        ("svijetloplavi", "svijétloplavi"),
        ("stručnjacima", "strȕčnjacima"),
        ("stručnjak", "strȕčnjak"),
        ("stručnjaka", "strȕčnjaka"),
    ];
    let mut speaker = Speaker::new("zvonko");
    expect_accents(&mut speaker, cases);
    // Written long pronouns remain available despite the requested short
    // default; their difference must survive synthesis, not just lookup.
    assert!(speaker.say("nas").len() < speaker.say("nȃs").len());
    assert!(speaker.say("vas").len() < speaker.say("vȃs").len());
    assert!(speaker.say("Riječanka") != speaker.say("Riječànka"));
    assert!(speaker.say("logopedi") != speaker.say("logòpedi"));
    // A plural object and the less common singular vocative share spelling.
    assert!(speaker.say("logopede") != speaker.say("lȍgopede"));
}

// =============================================================================
// Accents: names, listener lists, dictionaries, English words, word classes
// =============================================================================

/// One row of an accent table: the plain spelling gives audio, and the same
/// audio as the spelling with the accent written out.
fn same_as_accented(speaker: &mut Speaker, failures: &mut Failures, plain: &str, accented: &str) {
    let voice = speaker.voice_id();
    let audio = speaker.say(plain);
    failures.check(!audio.is_empty(), || format!("{voice}: no audio for {plain}"));
    failures.check(audio == speaker.say(accented), || format!("{voice}: {plain} is not {accented}"));
}

/// A table of (voice, plain, accented) rows, read by one engine that changes
/// its voice from row to row. Returns the engine for what the test says next.
#[track_caller]
fn check_accents_by_voice(cases: &[(&str, &str, &str)]) -> Speaker {
    let mut speaker = Speaker::empty();
    let mut failures = Failures::new();
    for &(voice, plain, accented) in cases {
        info!("{voice} {plain}");
        speaker.set_voice(voice).expect(voice);
        same_as_accented(&mut speaker, &mut failures, plain, accented);
    }
    failures.finish();
    speaker
}

/// A bundled dictionary, or the file an environment variable names instead.
fn dictionary_or(variable: &str, name: &str) -> PathBuf {
    std::env::var_os(variable).map_or_else(|| dictionary(name), PathBuf::from)
}

#[test]
fn personal_names_preserve_verified_stress_and_length() {
    // Listener corrections: Enída, Ìsaković, Greblìčki and Spàsojević.
    // HJP confirms long initial e: Knȇzović (falling), Knéžević (rising).
    // Exercise the shared lexicon in every language, including Cyrillic.
    const CASES: &[(&str, &str)] = &[
        ("Enida", "Enída"),
        ("enida", "enída"),
        ("ENIDA", "ENÍDA"),
        ("Enide", "Eníde"),
        ("Enidi", "Enídi"),
        ("Enidu", "Enídu"),
        ("Enidom", "Enídom"),
        ("Enidama", "Enídama"),
        ("Enidin", "Enídin"),
        ("Enidina", "Enídina"),
        ("Enidino", "Enídino"),
        ("Enidinoga", "Enídinoga"),
        ("Enidinim", "Enídinim"),
        ("Enidinima", "Enídinima"),
        ("Isaković", "Ìsaković"),
        ("isaković", "ìsaković"),
        ("ISAKOVIĆ", "ÌSAKOVIĆ"),
        ("Isakovića", "Ìsakovića"),
        ("Isakoviću", "Ìsakoviću"),
        ("Isakovićem", "Ìsakovićem"),
        ("Isakovići", "Ìsakovići"),
        ("Isakoviće", "Ìsakoviće"),
        ("Isakovićima", "Ìsakovićima"),
        ("Isakovićev", "Ìsakovićev"),
        ("Isakovićeva", "Ìsakovićeva"),
        ("Isakovićevo", "Ìsakovićevo"),
        ("Isakovićevoga", "Ìsakovićevoga"),
        ("Isakovićevom", "Ìsakovićevom"),
        ("Isakovićevim", "Ìsakovićevim"),
        ("Isakovićevima", "Ìsakovićevima"),
        ("Енида", "Enída"),
        ("Енидиним", "Enídinim"),
        ("Исаковић", "Ìsaković"),
        ("Исаковићевима", "Ìsakovićevima"),
        ("Enida Isaković", "Enída Ìsaković"),
        ("razgovaram s Enidom", "razgovaram s Enídom"),
        ("knjiga gospodina Isakovića", "knjiga gospodina Ìsakovića"),
        ("Greblički", "Greblìčki"),
        ("greblički", "greblìčki"),
        ("GREBLIČKI", "GREBLÌČKI"),
        ("Greblička", "Greblìčka"),
        ("Grebličke", "Greblìčke"),
        ("Grebličku", "Greblìčku"),
        ("Grebličkog", "Greblìčkog"),
        ("Grebličkoga", "Greblìčkoga"),
        ("Grebličkom", "Greblìčkom"),
        ("Grebličkome", "Greblìčkome"),
        ("Grebličkomu", "Greblìčkomu"),
        ("Grebličkoj", "Greblìčkoj"),
        ("Grebličkih", "Greblìčkih"),
        ("Grebličkim", "Greblìčkim"),
        ("Grebličkima", "Greblìčkima"),
        ("Греблички", "Greblìčki"),
        ("Гребличким", "Greblìčkim"),
        ("razgovaram s Grebličkim", "razgovaram s Greblìčkim"),
        ("Spasojević", "Spàsojević"),
        ("spasojević", "spàsojević"),
        ("SPASOJEVIĆ", "SPÀSOJEVIĆ"),
        ("Spasojevića", "Spàsojevića"),
        ("Spasojeviću", "Spàsojeviću"),
        ("Spasojevićem", "Spàsojevićem"),
        ("Spasojevići", "Spàsojevići"),
        ("Spasojeviće", "Spàsojeviće"),
        ("Spasojevićima", "Spàsojevićima"),
        ("Spasojevićev", "Spàsojevićev"),
        ("Spasojevićeva", "Spàsojevićeva"),
        ("Spasojevićevo", "Spàsojevićevo"),
        ("Spasojevićevoga", "Spàsojevićevoga"),
        ("Spasojevićevim", "Spàsojevićevim"),
        ("Spasojevićevima", "Spàsojevićevima"),
        ("Спасојевић", "Spàsojević"),
        ("Спасојевићевима", "Spàsojevićevima"),
        ("razgovaram sa Spasojevićem", "razgovaram sa Spàsojevićem"),
        ("Knezović", "Knȇzović"),
        ("knezović", "knȇzović"),
        ("KNEZOVIĆ", "KNȆZOVIĆ"),
        ("Knezovića", "Knȇzovića"),
        ("Knezoviću", "Knȇzoviću"),
        ("Knezovićem", "Knȇzovićem"),
        ("Knezovići", "Knȇzovići"),
        ("Knezoviće", "Knȇzoviće"),
        ("Knezovićima", "Knȇzovićima"),
        ("Knezovićev", "Knȇzovićev"),
        ("Knezovićeva", "Knȇzovićeva"),
        ("Knezovićevo", "Knȇzovićevo"),
        ("Knezovićevoga", "Knȇzovićevoga"),
        ("Knezovićevom", "Knȇzovićevom"),
        ("Knezovićevim", "Knȇzovićevim"),
        ("Knezovićevima", "Knȇzovićevima"),
        ("Кнезовић", "Knȇzović"),
        ("Кнезовићевима", "Knȇzovićevima"),
        ("Knežević", "Knéžević"),
        ("knežević", "knéžević"),
        ("KNEŽEVIĆ", "KNÉŽEVIĆ"),
        ("Kneževića", "Knéževića"),
        ("Kneževiću", "Knéževiću"),
        ("Kneževićem", "Knéževićem"),
        ("Kneževići", "Knéževići"),
        ("Kneževiće", "Knéževiće"),
        ("Kneževićima", "Knéževićima"),
        ("Kneževićev", "Knéževićev"),
        ("Kneževićeva", "Knéževićeva"),
        ("Kneževićevo", "Knéževićevo"),
        ("Kneževićevoga", "Knéževićevoga"),
        ("Kneževićevom", "Knéževićevom"),
        ("Kneževićevim", "Knéževićevim"),
        ("Kneževićevima", "Knéževićevima"),
        ("Кнежевић", "Knéžević"),
        ("Кнежевићевима", "Knéževićevima"),
        ("razgovaram s Knezovićem i Kneževićem", "razgovaram s Knȇzovićem i Knéževićem"),
    ];
    let mut speaker = Speaker::empty();
    for voice in FORMANT_VOICES {
        info!("{voice}");
        speaker.set_voice(voice).expect(voice);
        let mut failures = Failures::new();
        for &(plain, accented) in CASES {
            info!("{plain}");
            same_as_accented(&mut speaker, &mut failures, plain, accented);
        }
        failures.finish();
        // Check length separately from placement: Enida keeps long i;
        // Isakovic has initial stress and short a; Spasojevic has initial stress and short unstressed o.
        assert!(speaker.say("Enida").len() > speaker.say("Enìda").len(), "{voice}");
        assert!(speaker.say("Isaković").len() < speaker.say("Ìsāković").len(), "{voice}");
        assert!(speaker.say("Isaković") != speaker.say("Isàković"), "{voice}");
        assert!(speaker.say("Spasojević").len() < speaker.say("Spàsōjević").len(), "{voice}");
        assert!(speaker.say("Spasojević") != speaker.say("Spasòjević"), "{voice}");
        assert!(speaker.say("Spasojević") != speaker.say("Spasójević"), "{voice}");
        // Both surnames keep long e, with their distinct falling/rising tones.
        assert!(speaker.say("Knezović").len() > speaker.say("Knȅzović").len(), "{voice}");
        assert!(speaker.say("Knežević").len() > speaker.say("Knèžević").len(), "{voice}");
        assert!(speaker.say("Knezović") != speaker.say("Knézović"), "{voice}");
        assert!(speaker.say("Knežević") != speaker.say("Knȇžević"), "{voice}");
    }
}

#[test]
fn battery_and_reading_list_accents_follow_the_croatian_dictionaries() {
    // Školski rječnik / Mrežnik, checked 2026-10-08. In particular,
    // batèrija differs from HJP, and prȍčitan differs from pročìtati.
    // Croatian omits the dictionaries' unstressed lengths, as elsewhere.
    const CASES: &[(&str, &str)] = &[
        ("baterija", "batèrija"),
        ("baterije", "batèrije"),
        ("bateriji", "batèriji"),
        ("bateriju", "batèriju"),
        ("baterijom", "batèrijom"),
        ("baterijama", "batèrijama"),
        ("baterijski", "batèrijski"),
        ("baterijskoga", "batèrijskoga"),
        ("baterijskim", "batèrijskim"),
        ("baterija je prazna", "batèrija je prazna"),
        ("poslušati", "pòslušati"),
        ("poslušat ću", "pòslušat ću"),
        ("poslušam", "pòslušam"),
        ("poslušaš", "pòslušaš"),
        ("posluša", "pòsluša"),
        ("poslušamo", "pòslušamo"),
        ("poslušate", "pòslušate"),
        ("poslušaju", "pòslušaju"),
        ("poslušaj", "pòslušaj"),
        ("poslušajmo", "pòslušajmo"),
        ("poslušajte", "pòslušajte"),
        ("poslušao", "pòslušao"),
        ("poslušala", "pòslušala"),
        ("poslušali", "pòslušali"),
        ("poslušavši", "pòslušavši"),
        ("poslušan", "pòslušan"),
        ("poslušanima", "pòslušanima"),
        ("želim poslušati poruku", "želim pòslušati poruku"),
        ("pročitati", "pročìtati"),
        ("pročitat ću", "pročìtat ću"),
        ("pročitam", "pročìtam"),
        ("pročitaš", "pročìtaš"),
        ("pročita", "pročìta"),
        ("pročitamo", "pročìtamo"),
        ("pročitate", "pročìtate"),
        ("pročitaju", "pročìtaju"),
        ("pročitaj", "pročìtaj"),
        ("pročitajmo", "pročìtajmo"),
        ("pročitajte", "pročìtajte"),
        ("pročitao", "pročìtao"),
        ("pročitala", "pročìtala"),
        ("pročitali", "pročìtali"),
        ("pročitavši", "pročìtavši"),
        ("pročitan", "prȍčitan"),
        ("pročitana", "prȍčitana"),
        ("pročitano", "prȍčitano"),
        ("pročitani", "prȍčitani"),
        ("pročitane", "prȍčitane"),
        ("pročitanu", "prȍčitanu"),
        ("pročitanoga", "prȍčitanoga"),
        ("pročitanome", "prȍčitanome"),
        ("pročitanomu", "prȍčitanomu"),
        ("pročitanoj", "prȍčitanoj"),
        ("pročitanih", "prȍčitanih"),
        ("pročitanima", "prȍčitanima"),
        ("nepročitan", "nepròčitan"),
        ("nepročitanima", "nepròčitanima"),
        ("poruka je pročitana", "poruka je prȍčitana"),
    ];
    let mut speaker = Speaker::new("zvonko");
    let mut failures = Failures::new();
    for &(plain, accented) in CASES {
        info!("{plain}");
        same_as_accented(&mut speaker, &mut failures, plain, accented);
    }
    failures.finish();
}

#[test]
fn the_third_listener_list_follows_the_dictionaries_in_every_voice() {
    // prijateljstvo, nevidljiv, potencijal, snalaženje, simulacijski,
    // doživjeti, poštovanje, interesantno with their families and classes
    // (HJP, Školski rječnik, Mrežnik, Rečnik Matice srpske, Alić), checked
    // 2026-10-09; see docs/formant.md. Croatian omits unstressed length.
    const CASES: &[(&str, &str, &str)] = &[
        ("zvonko", "prijateljstvo", "prijatèljstvo"),
        ("zvonko", "prijateljstva", "prijatèljstva"),
        ("zvonko", "prijateljstvom", "prijatèljstvom"),
        ("zvonko", "prijateljstava", "prijatèljstava"),
        ("zvonko", "prijateljstvima", "prijatèljstvima"),
        ("zvonko", "neprijateljstvo", "neprijatèljstvo"),
        ("zvonko", "neprijateljstava", "neprijàteljstava"),
        ("zvonko", "roditeljstvo", "roditèljstvo"),
        ("zvonko", "prijatelj", "prȉjatelj"),
        ("zvonko", "prijatelja", "prȉjatelja"),
        ("zvonko", "prijateljima", "prȉjateljima"),
        ("zvonko", "neprijatelj", "nèprijatelj"),
        ("zvonko", "prijateljica", "prijatèljica"),
        ("zvonko", "prijateljicama", "prijatèljicama"),
        ("zvonko", "prijateljski", "prijatèljski"),
        ("zvonko", "prijateljskoga", "prijatèljskoga"),
        ("zvonko", "sprijateljiti", "sprijatèljiti"),
        ("zvonko", "sprijateljio", "sprijatèljio"),
        ("zvonko", "nevidljiv", "nevìdljiv"),
        ("zvonko", "nevidljiva", "nevìdljiva"),
        ("zvonko", "nevidljivoga", "nevìdljivoga"),
        ("zvonko", "nevidljivima", "nevìdljivima"),
        ("zvonko", "nevidljivo", "nevìdljivo"),
        ("zvonko", "nevidljivost", "nevìdljivost"),
        ("zvonko", "nevidljivošću", "nevìdljivošću"),
        ("zvonko", "nevidljiviji", "nevidljìviji"),
        ("zvonko", "vidljiv", "vìdljiv"),
        ("zvonko", "razumljivo", "razùmljivo"),
        ("zvonko", "prihvatljiv", "prihvàtljiv"),
        ("zvonko", "potencijal", "potencìjal"),
        ("zvonko", "potencijala", "potencijála"),
        ("zvonko", "potencijalu", "potencijálu"),
        ("zvonko", "potencijali", "potencijáli"),
        ("zvonko", "potencijalima", "potencijálima"),
        ("zvonko", "materijala", "materijála"),
        ("zvonko", "potencijalan", "pȍtencijalan"),
        ("zvonko", "potencijalno", "pȍtencijalno"),
        ("zvonko", "potencijalnih", "pȍtencijalnih"),
        ("zvonko", "potencijalnost", "potencijálnōst"),
        ("zvonko", "snalaženje", "snàlaženje"),
        ("zvonko", "snalaženja", "snàlaženja"),
        ("zvonko", "snalaženjem", "snàlaženjem"),
        ("zvonko", "nesnalaženje", "nesnàlaženje"),
        ("zvonko", "snalaziti", "snàlaziti"),
        ("zvonko", "snalazim", "snàlazim"),
        ("zvonko", "snalazio", "snàlazio"),
        ("zvonko", "snalazeći", "snàlazeći"),
        ("zvonko", "dolaziti", "dòlaziti"),
        ("zvonko", "dolaženje", "dòlaženje"),
        ("zvonko", "pronalaziti", "pronàlaziti"),
        ("zvonko", "pronalazi", "pronàlazi"),
        ("zvonko", "snaći", "snȃći"),
        ("zvonko", "snađem", "snȃđem"),
        ("zvonko", "snađi", "snáđi"),
        ("zvonko", "snašao", "snàšao"),
        ("zvonko", "snalažljiv", "snalàžljiv"),
        ("zvonko", "snalažljivost", "snalàžljivost"),
        ("zvonko", "simulacijski", "simulácijski"),
        ("zvonko", "simulacijskoga", "simulácijskoga"),
        ("zvonko", "simulacijskima", "simulácijskima"),
        ("zvonko", "simulacija", "simulácija"),
        ("zvonko", "komunikacijski", "komunikácijski"),
        ("zvonko", "policijski", "polìcijski"),
        ("zvonko", "simulator", "simùlator"),
        ("zvonko", "simulatora", "simùlatora"),
        ("zvonko", "doživjeti", "dožívjeti"),
        ("zvonko", "doživjet ću", "dožívjet ću"),
        ("zvonko", "doživio", "dožívio"),
        ("zvonko", "doživjela", "dožívjela"),
        ("zvonko", "doživjeli", "dožívjeli"),
        ("zvonko", "doživjevši", "dožívjevši"),
        ("zvonko", "doživi", "dožívi"),
        ("zvonko", "doživite", "dožívite"),
        ("zvonko", "Doživite nešto novo", "Dožívite nešto novo"),
        ("zvonko", "doživim", "dožívim"),
        ("zvonko", "dožive", "dožíve"),
        ("zvonko", "doživljen", "dòživljen"),
        ("zvonko", "doživljeno", "dòživljeno"),
        ("zvonko", "doživljaj", "dȍživljaj"),
        ("zvonko", "doživljajima", "dȍživljajima"),
        ("zvonko", "preživjeti", "prežívjeti"),
        ("zvonko", "preživio", "prežívio"),
        ("zvonko", "preživljen", "prèživljen"),
        ("zvonko", "živjeti", "žívjeti"),
        ("zvonko", "živio", "žívio"),
        ("zvonko", "poštovanje", "poštovánje"),
        ("zvonko", "poštovanja", "poštovánja"),
        ("zvonko", "poštovanjem", "poštovánjem"),
        ("zvonko", "nepoštovanje", "nepoštovánje"),
        ("zvonko", "Poštovanje!", "Poštovánje!"),
        ("zvonko", "poštovati", "poštòvati"),
        ("zvonko", "poštovao", "poštòvao"),
        ("zvonko", "poštujem", "pòštujem"),
        ("zvonko", "poštovan", "pȍštovan"),
        ("zvonko", "poštovani", "pȍštovani"),
        ("zvonko", "Poštovani korisnici", "Pȍštovani korisnici"),
        ("zvonko", "poštovatelj", "poštòvatelj"),
        ("zvonko", "interesantno", "interesàntno"),
        ("zvonko", "interesantan", "interesàntan"),
        ("zvonko", "interesantna", "interesàntna"),
        ("zvonko", "interesantnoga", "interesàntnoga"),
        ("zvonko", "interesantniji", "interesàntniji"),
        ("zvonko", "interesantnost", "interesàntnost"),
        ("zvonko", "elegantno", "elegàntno"),
        ("zvonko", "kompetentni", "kompetèntni"),
        ("zvonko", "formantni", "fòrmantni"),
        ("zvonko", "interes", "ȉnteres"),
        ("zvonko", "interesima", "ȉnteresima"),
        ("stojan", "prijateljstvo", "prijatéljstvo"),
        ("stojan", "nevidljiv", "nevìdljiv"),
        ("stojan", "potencijala", "potencijála"),
        ("stojan", "potencijalno", "pȍtencijālno"),
        ("stojan", "snalaženje", "snàlažēnje"),
        ("stojan", "snaći", "snáći"),
        ("stojan", "simulacijski", "simulácījski"),
        ("stojan", "doživeti", "dožíveti"),
        ("stojan", "doživeo", "dožíveo"),
        ("stojan", "doživim", "dožívīm"),
        ("stojan", "doživljaj", "dȍživljāj"),
        ("stojan", "doživljavaju", "doživljávaju"),
        ("stojan", "poštovanje", "poštovánje"),
        ("stojan", "interesantno", "interesàntno"),
        ("stojan", "interesovati", "ȉnteresovati"),
        ("stojan", "interesujem", "ȉnteresujēm"),
        ("stojan", "otvaraju", "otváraju"),
        ("stojan", "poštovaću", "poštòvaću"),
        // Words around these rules that keep their own accent (review of
        // 2026-10-09): the stems of the lexicon win over the new rules
        ("zvonko", "austrijskog", "àustrijskog"),
        ("zvonko", "australijski", "aùstralijski"),
        ("zvonko", "cementni", "cèmentni"),
        ("zvonko", "momentna", "mòmentna"),
        ("zvonko", "prijateljuju", "prijatèljuju"),
        ("zvonko", "preživljenje", "preživljénje"),
        ("zvonko", "preživjelih", "prežívjelih"),
        ("zvonko", "poštovateljica", "poštovatèljica"),
        ("zvonko", "snalazi", "snàlazi"),
        ("zvonko", "vjerovanje", "vjȅrovanje"),
        ("zvonko", "ljetovati", "ljȅtovati"),
        ("stojan", "verovaćemo", "vȅrovaćemo"),
        ("stojan", "interesovaćeš", "ȉnteresovaćeš"),
        ("mirsad", "prijatelj", "prìjatelj"),
        ("mirsad", "prijateljstvo", "prijatèljstvo"),
        ("mirsad", "doživjeti", "dožívjeti"),
        ("mirsad", "doživim", "dožívīm"),
        ("mirsad", "snalaženje", "snàlažēnje"),
        ("mirsad", "poštovanje", "poštovánje"),
    ];
    let mut speaker = check_accents_by_voice(CASES);
    // The old readings are gone
    speaker.set_voice("zvonko").expect("zvonko");
    assert!(speaker.say("snalaženje") != speaker.say("snalažénje"));
    assert!(speaker.say("poštovanje") != speaker.say("poštòvanje"));
    assert!(speaker.say("doživjeti") != speaker.say("dožìvjeti"));
    assert!(speaker.say("poštovati") != speaker.say("pȍštovati"));
}

#[test]
fn the_fourth_listener_list_follows_the_dictionaries_in_every_voice() {
    // Egipat, proizvodnja, vodič, napomene with their families (HJP, Školski
    // rječnik, Wiktionary, Rečnik Matice srpske), checked 2026-10-09; see
    // docs/formant.md. Croatian omits unstressed length.
    const CASES: &[(&str, &str, &str)] = &[
        ("zvonko", "Egipat", "Ègipat"),
        ("zvonko", "Egipta", "Ègipta"),
        ("zvonko", "Egiptu", "Ègiptu"),
        ("zvonko", "Egiptom", "Ègiptom"),
        ("zvonko", "egipatski", "ègipatski"),
        ("zvonko", "egipatskoga", "ègipatskoga"),
        ("zvonko", "Egipćanin", "Ègipćanin"),
        ("zvonko", "Egipćani", "Ègipćani"),
        ("zvonko", "Egipćanima", "Ègipćanima"),
        ("zvonko", "Egipćanka", "Ègipćanka"),
        ("zvonko", "proizvodnja", "proizvòdnja"),
        ("zvonko", "proizvodnje", "proizvòdnje"),
        ("zvonko", "proizvodnji", "proizvòdnji"),
        ("zvonko", "proizvodnju", "proizvòdnju"),
        ("zvonko", "proizvodnjom", "proizvòdnjom"),
        ("zvonko", "proizvodnjama", "proizvòdnjama"),
        ("zvonko", "proizvod", "proìzvod"),
        ("zvonko", "proizvoda", "proìzvoda"),
        ("zvonko", "proizvodi", "proìzvodi"),
        ("zvonko", "Proizvodi", "Proìzvodi"),
        ("zvonko", "proizvodima", "proìzvodima"),
        ("zvonko", "proizvesti", "proìzvesti"),
        ("zvonko", "proizveo", "proìzveo"),
        ("zvonko", "proizvedem", "proizvèdem"),
        ("zvonko", "proizveden", "proizvèden"),
        ("zvonko", "proizvedeno", "proizvèdeno"),
        ("zvonko", "proizvoditi", "proizvòditi"),
        ("zvonko", "proizvodio", "proizvòdio"),
        ("zvonko", "proizvodim", "proizvòdim"),
        ("zvonko", "proizvođenje", "proìzvođenje"),
        ("zvonko", "proizvođača", "proizvođáča"),
        ("zvonko", "vodič", "vòdič"),
        ("zvonko", "vodiča", "vodíča"),
        ("zvonko", "vodiču", "vodíču"),
        ("zvonko", "vodičem", "vodíčem"),
        ("zvonko", "vodiči", "vodíči"),
        ("zvonko", "vodiče", "vodíče"),
        ("zvonko", "vodičima", "vodíčima"),
        ("zvonko", "poluvodič", "poluvòdič"),
        ("zvonko", "poluvodiča", "poluvodíča"),
        ("zvonko", "napomena", "nȁpomena"),
        ("zvonko", "napomene", "nȁpomene"),
        ("zvonko", "Dodaj napomene", "Dodaj nȁpomene"),
        ("zvonko", "u napomeni", "u nȁpomeni"),
        ("zvonko", "napomenu", "nȁpomenu"),
        ("zvonko", "napomenom", "nȁpomenom"),
        ("zvonko", "napomenama", "nȁpomenama"),
        ("zvonko", "kad napomene", "kad nȁpomene"),
        ("zvonko", "Napomeni mu", "Napoméni mu"),
        ("zvonko", "napomenuti", "napoménuti"),
        ("zvonko", "napomenuo", "napoménuo"),
        ("zvonko", "napominjati", "napòminjati"),
        ("zvonko", "napominjem", "napòminjem"),
        ("zvonko", "napominjao", "napòminjao"),
        ("stojan", "Egipat", "Ègipat"),
        ("stojan", "Egipćanka", "Ègipćānka"),
        ("stojan", "proizvodnja", "proizvòdnja"),
        ("stojan", "proizvod", "proìzvod"),
        ("stojan", "proizvedeno", "proizvèdeno"),
        ("stojan", "proizvodim", "proìzvodim"),
        ("stojan", "vodič", "vòdīč"),
        ("stojan", "vodiča", "vodíča"),
        ("stojan", "poluvodič", "poluvòdīč"),
        ("stojan", "napomena", "nȁpomena"),
        ("stojan", "napomene", "nȁpomene"),
        ("stojan", "napomenuti", "napoménuti"),
        ("stojan", "napominjem", "napòminjem"),
        ("mirsad", "Egipat", "Ègipat"),
        ("mirsad", "proizvodnja", "proizvòdnja"),
        ("mirsad", "vodič", "vòdīč"),
        ("mirsad", "vodiču", "vodíču"),
        ("mirsad", "napomene", "nȁpomene"),
    ];
    let mut speaker = check_accents_by_voice(CASES);
    // The old readings are gone
    speaker.set_voice("zvonko").expect("zvonko");
    assert!(speaker.say("Egipat") != speaker.say("Egìpat"));
    assert!(speaker.say("napomene") != speaker.say("napoméne"));
}

#[test]
fn the_sixth_listener_list_follows_the_dictionaries_in_every_voice() {
    // karijera ... ruža with their families (Školski rječnik, HJP, Hrvatski
    // mrežni rječnik, Wiktionary, Rečnik Matice srpske, Alić), checked
    // 2026-10-09; see docs/formant.md. Croatian omits unstressed length.
    const CASES: &[(&str, &str, &str)] = &[
        // The loans in -ijer: four syllables, not the jat of rijeka
        ("zvonko", "karijera", "karijéra"),
        ("zvonko", "karijerom", "karijérom"),
        ("zvonko", "premijer", "premìjer"),
        ("zvonko", "premijera", "premijéra"),
        ("zvonko", "premijeri", "premijéri"),
        ("zvonko", "barijera", "barijéra"),
        ("zvonko", "hotelijer", "hotelìjer"),
        ("zvonko", "interijer", "interìjer"),
        ("zvonko", "hijerarhija", "hijeràrhija"),
        ("zvonko", "umjetnik", "ùmjetnik"),
        ("zvonko", "umjetnici", "ùmjetnici"),
        ("zvonko", "umjetnost", "ùmjetnost"),
        ("zvonko", "umjetnošću", "ùmjetnošću"),
        ("zvonko", "umjetnički", "ùmjetnički"),
        ("zvonko", "umjetničkoga", "ùmjetničkoga"),
        ("zvonko", "album", "àlbum"),
        ("zvonko", "albuma", "albúma"),
        ("zvonko", "albumima", "albúmima"),
        ("zvonko", "jedinstven", "jedìnstven"),
        ("zvonko", "jedinstvenoga", "jedìnstvenoga"),
        ("zvonko", "jedinstvo", "jedínstvo"),
        ("zvonko", "jedinstveniji", "jedinstvèniji"),
        ("zvonko", "kompromis", "kompròmis"),
        ("zvonko", "kompromisima", "kompròmisima"),
        ("zvonko", "kompromisan", "kompròmisan"),
        ("zvonko", "kompromisnoga", "kompròmisnoga"),
        ("zvonko", "beskompromisan", "bèskompromisan"),
        ("zvonko", "beskompromisna", "bèskompromisna"),
        ("zvonko", "zanima", "zànima"),
        ("zvonko", "zanimam", "zànimam"),
        ("zvonko", "zanimaju", "zanímaju"),
        ("zvonko", "zanimati", "zanímati"),
        ("zvonko", "zanimalo", "zanímalo"),
        ("zvonko", "zanimanje", "zanímanje"),
        ("zvonko", "plaćenik", "plȃćenik"),
        ("zvonko", "plaćenici", "plȃćenici"),
        ("zvonko", "plačenik", "plȃčenik"),
        ("zvonko", "povijest", "pȍvijest"),
        ("zvonko", "povijesni", "pȍvijesni"),
        ("zvonko", "ostavština", "ostávština"),
        ("zvonko", "ostavštinom", "ostávštinom"),
        ("zvonko", "odijelo", "odijélo"),
        ("zvonko", "odijelima", "odijélima"),
        ("zvonko", "odjelo", "odjélo"),
        ("zvonko", "obilježila", "obìlježila"),
        ("zvonko", "gubitak", "gubítak"),
        ("zvonko", "gubitka", "gubítka"),
        ("zvonko", "gubici", "gubíci"),
        ("zvonko", "gubicima", "gubícima"),
        ("zvonko", "gubitaka", "gùbitaka"),
        ("zvonko", "dobitak", "dobítak"),
        ("zvonko", "užitak", "užítak"),
        ("zvonko", "imutak", "imútak"),
        ("zvonko", "razvitak", "razvítak"),
        ("zvonko", "debitant", "debìtant"),
        ("zvonko", "debitantica", "debìtantica"),
        ("zvonko", "debitantski", "debìtantski"),
        ("zvonko", "debitantskoga", "debìtantskoga"),
        ("zvonko", "neukrotiv", "neukròtiv"),
        ("zvonko", "neukrotivoga", "neukròtivoga"),
        ("zvonko", "ljubav", "ljúbav"),
        ("zvonko", "ljubavlju", "ljúbavlju"),
        ("zvonko", "ljubavni", "ljúbavni"),
        ("zvonko", "debil", "dèbil"),
        ("zvonko", "debila", "debíla"),
        ("zvonko", "izdanje", "izdánje"),
        ("zvonko", "izdanjima", "izdánjima"),
        ("zvonko", "nezaobilazan", "nezaobìlazan"),
        ("zvonko", "nezaobilaznoga", "nezaobìlaznoga"),
        ("zvonko", "zaobilaznica", "zaobìlaznica"),
        ("zvonko", "boja", "bòja"),
        ("zvonko", "bojama", "bòjama"),
        ("zvonko", "teško", "tȇško"),
        ("zvonko", "teški", "tȇški"),
        ("zvonko", "teška", "téška"),
        ("zvonko", "težak", "téžak"),
        ("zvonko", "teže", "tȅže"),
        ("zvonko", "Ljetni odmori", "Ljetni òdmori"),
        ("zvonko", "Odmori se", "Odmòri se"),
        ("zvonko", "odmor", "òdmor"),
        ("zvonko", "odmoriti", "odmòriti"),
        ("zvonko", "odmorim", "òdmorim"),
        ("zvonko", "odmoren", "òdmoren"),
        ("zvonko", "bol", "bȏl"),
        ("zvonko", "bola", "bȏla"),
        ("zvonko", "Bez boli", "Bez bȏli"),
        ("zvonko", "u boli", "u bóli"),
        ("zvonko", "o bolu", "o bólu"),
        ("zvonko", "bolovi", "bȍlovi"),
        ("zvonko", "Boli me glava", "Bòli me glava"),
        ("zvonko", "Glava me jako boli", "Glava me jako bòli"),
        ("zvonko", "Ne boli", "Ne bòli"),
        ("zvonko", "Gdje boli", "Gdje bòli"),
        ("zvonko", "Mene boli glava", "Mene bòli glava"),
        ("zvonko", "Glava me i dalje boli", "Glava me i dalje bòli"),
        ("zvonko", "Zbog jake boli ga je odvezla", "Zbog jake bȏli ga je odvezla"),
        ("zvonko", "Lijek mu ublažava boli", "Lijek mu ublažava bȏli"),
        ("zvonko", "Te boli su prošle", "Te bȏli su prošle"),
        ("zvonko", "Žali se na boli", "Žali se na bȏli"),
        ("zvonko", "hijeroglif", "hijeròglif"),
        ("zvonko", "arhijerej", "arhijèrej"),
        ("zvonko", "zaobilaznicom", "zaobìlaznicom"),
        ("zvonko", "bolno", "bȏlno"),
        ("zvonko", "bolna", "bólna"),
        ("zvonko", "neprikosnoven", "neprikosnòven"),
        ("zvonko", "neprekosnoven", "neprekosnòven"),
        ("zvonko", "krhotina", "kr̀hotina"),
        ("zvonko", "krhotinama", "kr̀hotinama"),
        ("zvonko", "tuga", "túga"),
        ("zvonko", "tuzi", "túzi"),
        ("zvonko", "tužno", "túžno"),
        ("stojan", "tužno", "tȗžno"),
        ("zvonko", "tužan", "túžan"),
        ("zvonko", "tužna", "túžna"),
        ("zvonko", "tužni", "tȗžni"),
        ("zvonko", "tužne", "tȗžne"),
        ("zvonko", "tužniji", "tùžniji"),
        ("zvonko", "Bila je duga noć", "Bila je dùga noć"),
        ("zvonko", "važno", "vȃžno"),
        ("zvonko", "važan", "vážan"),
        ("zvonko", "važnost", "vážnost"),
        ("zvonko", "važniji", "vàžniji"),
        ("zvonko", "lažno", "lȁžno"),
        ("zvonko", "lažna", "làžna"),
        ("zvonko", "laž", "lȃž"),
        ("zvonko", "laži", "lȁži"),
        ("zvonko", "lažima", "làžima"),
        ("zvonko", "ruža", "rúža"),
        ("zvonko", "ružama", "rúžama"),
        ("stojan", "karijera", "karijéra"),
        ("stojan", "premijer", "premìjēr"),
        ("stojan", "umetnik", "ùmetnīk"),
        ("stojan", "umetnost", "ùmetnōst"),
        ("stojan", "album", "àlbūm"),
        ("stojan", "beskompromisan", "bȅskompromisan"),
        ("stojan", "zanima", "zànīma"),
        ("stojan", "plaćenik", "plȃćenīk"),
        ("stojan", "odelo", "odélo"),
        ("stojan", "obeležen", "obèležen"),
        ("stojan", "gubitaka", "gùbītākā"),
        ("stojan", "debil", "dèbīl"),
        ("stojan", "ljubavnik", "ljúbāvnīk"),
        ("stojan", "nezaobilazan", "nȅzaobilāzan"),
        ("stojan", "tugom", "túgōm"),
        ("stojan", "tužan", "tȗžan"),
        ("zvonko", "zvijer", "zvȋjer"),
        ("mirsad", "umjetnik", "ùmjetnīk"),
        ("mirsad", "gubitaka", "gubítākā"),
        ("mirsad", "ljubav", "ljúbav"),
        ("mirsad", "izdanje", "izdánje"),
        ("mirsad", "bojom", "bòjōm"),
    ];
    let mut speaker = check_accents_by_voice(CASES);
    // The old readings are gone
    speaker.set_voice("zvonko").expect("zvonko");
    assert!(speaker.say("zanima") != speaker.say("zaníma"));
    assert!(speaker.say("krhotina") != speaker.say("krhotína"));
    assert!(speaker.say("bolnica") != speaker.say("bȏlnica"));
    // odjela, odjelu are the cases of òdjel, not of odijelo
    assert!(speaker.say("odjela") != speaker.say("odjéla"));
    // Jat before r stays one syllable: zvijer, prijeratni
    assert!(speaker.say("zvijer") == speaker.say("zvȋjer"));
}

#[test]
fn the_seventh_and_eighth_listener_lists_follow_the_dictionaries_in_every_voice() {
    // nikad, nikada, poboljšanja, reproducirano with their families (HJP,
    // Školski rječnik, Rečnik Matice srpske), checked 2026-10-09
    const CASES: &[(&str, &str, &str)] = &[
        ("zvonko", "nikad", "nȉkad"),
        ("zvonko", "nikada", "nȉkada"),
        ("zvonko", "ikada", "ȉkada"),
        ("zvonko", "nekada", "nȅkada"),
        ("zvonko", "katkada", "kȁtkada"),
        ("zvonko", "otkada", "òtkada"),
        ("zvonko", "ponekad", "pònekad"),
        ("zvonko", "kadikada", "kadìkada"),
        ("zvonko", "poboljšanje", "poboljšánje"),
        ("zvonko", "poboljšanja", "poboljšánja"),
        ("zvonko", "poboljšanjima", "poboljšánjima"),
        ("zvonko", "poboljšati", "pobòljšati"),
        ("zvonko", "obećanje", "obećánje"),
        ("zvonko", "povećanje", "povećánje"),
        ("zvonko", "pojačanje", "pojačánje"),
        ("zvonko", "pogoršanje", "pogoršánje"),
        ("zvonko", "olakšanje", "olakšánje"),
        ("zvonko", "reproducirano", "reprodùcirano"),
        ("zvonko", "reproducira", "reprodùcira"),
        ("zvonko", "reproduciram", "reprodùciram"),
        ("zvonko", "reproducirati", "reproducírati"),
        ("zvonko", "reproduciraju", "reproducíraju"),
        ("zvonko", "reproducirao", "reproducírao"),
        ("zvonko", "reproduciranje", "reproducíranje"),
        ("zvonko", "reproduktivan", "rȅproduktivan"),
        ("stojan", "nikada", "nȉkada"),
        ("stojan", "poboljšanje", "poboljšánje"),
        ("stojan", "reproducirano", "reprodùcīrano"),
        ("mirsad", "nikad", "nȉkad"),
        ("mirsad", "obećanja", "obećánja"),
        // The eighth list (2026-10-09)
        ("zvonko", "karta", "kȃrta"),
        ("zvonko", "karte", "kȃrte"),
        ("zvonko", "kartama", "kȃrtama"),
        ("zvonko", "karata", "kȁrata"),
        ("zvonko", "kartica", "kàrtica"),
        ("zvonko", "zdravlje", "zdrȃvlje"),
        ("zvonko", "zdravljem", "zdrȃvljem"),
        ("zvonko", "zdravstvo", "zdràvstvo"),
        ("zvonko", "zdravstvenoga", "zdràvstvenoga"),
        ("stojan", "karata", "kȁrātā"),
        ("mirsad", "zdravlje", "zdrȃvlje"),
        ("zvonko", "privitak", "privítak"),
        ("zvonko", "privitku", "privítku"),
        ("zvonko", "privici", "privíci"),
        ("zvonko", "privicima", "privícima"),
        ("zvonko", "privitci", "privítci"),
        ("zvonko", "privitaka", "prìvitaka"),
        ("zvonko", "privijen", "prìvijen"),
        ("stojan", "privitaka", "prìvītākā"),
        ("mirsad", "privitaka", "privítākā"),
        // The ninth list (2026-10-09)
        ("zvonko", "različit", "rázličit"),
        ("zvonko", "različitoga", "rázličitoga"),
        ("zvonko", "različitost", "rázličitost"),
        ("zvonko", "različitosti", "rázličitosti"),
        ("zvonko", "različitiji", "različìtiji"),
        ("zvonko", "strana", "strána"),
        ("zvonko", "S druge strane", "S druge stráne"),
        ("zvonko", "stranama", "stránama"),
        ("zvonko", "stranu", "strȃnu"),
        ("zvonko", "Na lijevoj strani", "Na lijevoj stráni"),
        ("zvonko", "Strani jezik", "Strȃni jezik"),
        ("zvonko", "dokumenti", "dokùmenti"),
        ("zvonko", "dokumenata", "dokùmenata"),
        ("zvonko", "Danijela", "Dànijela"),
        ("stojan", "različitost", "rázličitōst"),
        ("stojan", "dokumenata", "dokùmenātā"),
        // vijest (2026-10-09)
        ("zvonko", "vijest", "vijȇst"),
        ("zvonko", "vijesti", "vijȇsti"),
        ("zvonko", "vijestima", "vijéstima"),
        ("zvonko", "viješću", "vijȇšću"),
        ("zvonko", "o toj vijesti", "o toj vijésti"),
        ("zvonko", "na vijesti", "na vijȇsti"),
        ("stojan", "vesti", "vȇsti"),
        ("stojan", "vestima", "véstima"),
        // razmak, razmaknica, razmaknuti; ipsilon (2026-10-09)
        ("zvonko", "razmak", "rázmak"),
        ("zvonko", "razmaka", "rázmaka"),
        ("zvonko", "razmaci", "rázmaci"),
        ("zvonko", "razmacima", "rázmacima"),
        ("zvonko", "razmaknica", "rázmaknica"),
        ("zvonko", "Pritisni razmaknicu", "Pritisni rázmaknicu"),
        ("zvonko", "razmaknuti", "razmàknuti"),
        ("zvonko", "razmaknem", "ràzmaknem"),
        ("zvonko", "Razmakni stolove", "Razmàkni stolove"),
        ("zvonko", "razmaknuo", "razmàknuo"),
        ("zvonko", "razmaknut", "ràzmaknut"),
        ("zvonko", "razmicati", "ràzmicati"),
        ("zvonko", "razmičem", "ràzmičem"),
        ("zvonko", "ipsilon", "ȉpsilon"),
        ("zvonko", "epsilon", "èpsilon"),
        ("stojan", "razmaka", "rázmākā"),
        ("stojan", "ipsilon", "ȉpsilōn"),
        // carstvo, kraljevstvo (2026-10-09)
        ("zvonko", "carstvo", "cȃrstvo"),
        ("zvonko", "carstvima", "cȃrstvima"),
        ("zvonko", "carstava", "cȃrstava"),
        ("zvonko", "carski", "cȃrski"),
        ("zvonko", "kraljevstvo", "králjevstvo"),
        ("zvonko", "kraljevstvom", "králjevstvom"),
        ("zvonko", "kraljevstava", "králjevstava"),
        ("stojan", "carstava", "cȃrstāvā"),
        ("mirsad", "kraljevstava", "králjevstāvā"),
        // ćar, čar, čarati; pritisnuti, ujediniti (2026-10-09)
        ("zvonko", "ćar", "ćȃr"),
        ("zvonko", "čar", "čȃr"),
        ("zvonko", "čari", "čȃri"),
        ("zvonko", "čarima", "čárima"),
        ("zvonko", "u čari", "u čári"),
        ("zvonko", "čarati", "čárati"),
        ("zvonko", "čaram", "čȃram"),
        ("zvonko", "čaraj", "čȃraj"),
        ("zvonko", "čaraju", "čáraju"),
        ("zvonko", "čarao", "čárao"),
        ("zvonko", "čaroban", "čȁroban"),
        ("zvonko", "očarati", "očárati"),
        ("zvonko", "očaram", "òčaram"),
        ("zvonko", "očaran", "òčaran"),
        ("zvonko", "pritisnuti", "prìtisnuti"),
        ("zvonko", "pritisnuo", "prìtisnuo"),
        ("zvonko", "Pritisnite tipku", "Prìtisnite tipku"),
        ("zvonko", "pritisak", "prìtisak"),
        ("zvonko", "pritisci", "prìtisci"),
        ("zvonko", "pritiskati", "pritískati"),
        ("zvonko", "pritiskam", "prìtiskam"),
        ("zvonko", "pritišćem", "prìtišćem"),
        ("zvonko", "ujediniti", "ujedíniti"),
        ("zvonko", "ujedinio", "ujedínio"),
        ("zvonko", "ujedinim", "ujèdinim"),
        ("zvonko", "Ujedinjeno Kraljevstvo", "Ujèdinjeno Králjevstvo"),
        ("stojan", "čaram", "čȃrām"),
        ("stojan", "ujedinjen", "ujèdīnjen"),
        ("zvonko", "minimalac", "minimálac"),
        ("zvonko", "minimalca", "minimálca"),
        ("zvonko", "na minimalcu", "na minimálcu"),
        ("zvonko", "minimalci", "minimálci"),
        ("zvonko", "minimalaca", "minimálaca"),
        ("zvonko", "minimalan", "mȉnimalan"),
        ("zvonko", "minimalno", "mȉnimalno"),
        ("zvonko", "minimalnost", "minimálnost"),
        ("zvonko", "firma", "fȋrma"),
        ("zvonko", "firmi", "fȋrmi"),
        ("zvonko", "firmama", "fȋrmama"),
        ("stojan", "firme", "fȋrme"),
        ("stojan", "minimalan", "mȉnimālan"),
        // The eleventh list (2026-10-10)
        ("zvonko", "trenutak", "trenútak"),
        ("zvonko", "trenuci", "trenúci"),
        ("zvonko", "trenutaka", "trènutaka"),
        ("zvonko", "podataka", "pòdataka"),
        ("stojan", "trenutaka", "trènūtākā"),
        ("mirsad", "trenutaka", "trenútākā"),
        ("mirsad", "podataka", "podátākā"),
        ("zvonko", "sitnica", "sitnìca"),
        ("zvonko", "sitnice", "sitnìce"),
        ("zvonko", "sitnicama", "sitnìcama"),
        ("stojan", "sitnicu", "sitnìcu"),
        ("zvonko", "nezapaženo", "nezàpaženo"),
        ("zvonko", "nezapažen", "nezàpažen"),
        ("zvonko", "nezapaženog", "nezàpaženog"),
        ("zvonko", "zapažen", "zàpažen"),
        ("zvonko", "zapaziti", "zàpaziti"),
        ("zvonko", "zapazio", "zàpazio"),
        ("zvonko", "zapazim", "zàpazim"),
        ("zvonko", "zapažati", "zapážati"),
        ("zvonko", "zapažam", "zàpažam"),
        ("zvonko", "zapažanje", "zapážanje"),
        ("zvonko", "opaziti", "òpaziti"),
        ("zvonko", "opažen", "òpažen"),
        ("stojan", "zapažam", "zàpāžam"),
        ("zvonko", "zanat", "zànat"),
        ("zvonko", "zanata", "zanáta"),
        ("zvonko", "zanatima", "zanátima"),
        ("zvonko", "zanatski", "zànatski"),
        ("stojan", "zanat", "zànāt"),
        ("zvonko", "pismo", "písmo"),
        ("zvonko", "pisma", "písma"),
        ("zvonko", "pismima", "písmima"),
        ("zvonko", "pisama", "pȋsama"),
        ("stojan", "pisama", "pȋsāmā"),
        ("zvonko", "star", "stȁr"),
        ("zvonko", "stari", "stȃri"),
        ("zvonko", "stara", "stȃra"),
        ("zvonko", "starog", "stȃrog"),
        ("zvonko", "starima", "stȃrima"),
        ("zvonko", "mlad", "mlȃd"),
        ("zvonko", "mladi", "mlȃdi"),
        ("zvonko", "mlada", "mláda"),
        ("zvonko", "mladog", "mlȃdog"),
        ("zvonko", "detalj", "dètalj"),
        ("zvonko", "detalja", "detálja"),
        ("zvonko", "detalji", "detálji"),
        ("zvonko", "detaljima", "detáljima"),
        ("zvonko", "detaljan", "dètaljan"),
        ("zvonko", "detaljna", "dètaljna"),
        ("zvonko", "detaljno", "dȅtaljno"),
        ("stojan", "detalj", "dètālj"),
    ];
    let mut speaker = check_accents_by_voice(CASES);
    // The old readings are gone, and the loans in -ada keep theirs
    speaker.set_voice("zvonko").expect("zvonko");
    assert!(speaker.say("nikada") != speaker.say("nikáda"));
    assert!(speaker.say("blokada") == speaker.say("blokáda"));
    assert!(speaker.say("barikada") == speaker.say("barikáda"));
}

/// Age and time compounds and čestitka follow the dictionaries
#[test]
fn age_and_time_compounds_and_cestitka_follow_the_dictionaries() {
    // The compounds of -godišnji, -godišnjak, -godišnjica, -godište,
    // -mjesečni, -dnevni, -tjedni, -nedeljni and -ljetan
    // (StressRules::time_compound), gòdišnjāk, čèstitka and čestítati (HJP,
    // Školski rječnik, Rečnik Matice srpske), checked 2026-10-09; see
    // docs/formant.md. Croatian omits unstressed length.
    const CASES: &[(&str, &str, &str)] = &[
        ("zvonko", "petogodišnji", "petogòdišnji"),
        ("zvonko", "petogodišnja", "petogòdišnja"),
        ("zvonko", "petogodišnjeg", "petogòdišnjeg"),
        ("zvonko", "šestogodišnjoj", "šestogòdišnjoj"),
        ("zvonko", "sedmogodišnji", "sedmogòdišnji"),
        ("zvonko", "osmogodišnjem", "osmogòdišnjem"),
        ("zvonko", "desetogodišnje", "desetogòdišnje"),
        ("zvonko", "stogodišnji", "stogòdišnji"),
        ("zvonko", "dvadesetogodišnjak", "dvadesetogòdišnjak"),
        ("zvonko", "dvadesetpetogodišnji", "dvadesetpetogòdišnji"),
        ("zvonko", "dugogodišnji", "dugogòdišnji"),
        ("zvonko", "novogodišnja", "novogòdišnja"),
        ("zvonko", "stogodišnjica", "stogòdišnjica"),
        ("zvonko", "desetogodišnjicu", "desetogòdišnjicu"),
        ("zvonko", "petogodišnjak", "petogòdišnjak"),
        ("zvonko", "petogodišnjaka", "petogòdišnjaka"),
        ("zvonko", "petogodišnjakinja", "petogodišnjàkinja"),
        ("zvonko", "polugodište", "polugòdište"),
        ("zvonko", "polugodišta", "polugòdišta"),
        ("zvonko", "šestomjesečna", "šestòmjesečna"),
        ("zvonko", "devetomjesečni", "devetòmjesečni"),
        ("zvonko", "višemjesečnog", "višèmjesečnog"),
        ("zvonko", "tromjesečni", "tròmjesečni"),
        ("zvonko", "petodnevni", "petòdnevni"),
        ("zvonko", "jednodnevnog", "jednòdnevnog"),
        ("zvonko", "trotjedni", "tròtjedni"),
        ("zvonko", "maloljetan", "malòljetan"),
        ("zvonko", "maloljetna", "malòljetna"),
        ("zvonko", "maloljetnik", "malòljetnik"),
        ("zvonko", "maloljetnici", "malòljetnici"),
        ("zvonko", "punoljetnost", "punòljetnost"),
        ("zvonko", "stoljetni", "stòljetni"),
        ("zvonko", "godišnjak", "gòdišnjak"),
        ("zvonko", "godišnjaka", "gòdišnjaka"),
        ("zvonko", "čestitka", "čèstitka"),
        ("zvonko", "čestitke", "čèstitke"),
        ("zvonko", "čestitku", "čèstitku"),
        ("zvonko", "čestitaka", "čèstitaka"),
        ("zvonko", "čestitati", "čestítati"),
        ("zvonko", "čestitam", "čèstitam"),
        ("zvonko", "čestitao", "čestítao"),
        ("zvonko", "čestitaju", "čestítaju"),
        ("zvonko", "čestitaj", "čèstitaj"),
        ("stojan", "petogodišnji", "petogòdišnji"),
        ("stojan", "stogodišnjica", "stogòdišnjica"),
        ("stojan", "šestomesečna", "šestomèsečna"),
        ("stojan", "devetomesečni", "devetomèsečni"),
        ("stojan", "petodnevni", "petòdnēvni"),
        ("stojan", "dvonedeljni", "dvonèdēljni"),
        ("stojan", "maloletan", "malòletan"),
        ("stojan", "punoletnost", "punòletnost"),
        ("stojan", "polugodište", "polugòdīšte"),
        ("stojan", "godišnjak", "gòdišnjāk"),
        ("stojan", "čestitka", "čèstitka"),
        ("stojan", "čestitam", "čèstītam"),
        ("stojan", "čestitati", "čestítati"),
        ("mirsad", "šestomjesečna", "šestomjèsečna"),
        ("mirsad", "petogodišnji", "petogòdišnji"),
        ("mirsad", "maloljetnik", "malòljetnik"),
        ("mirsad", "čestitke", "čèstitke"),
    ];
    let mut speaker = check_accents_by_voice(CASES);
    // The old readings are gone, and a word that only looks like such a
    // compound keeps its own accent
    speaker.set_voice("zvonko").expect("zvonko");
    assert!(speaker.say("petogodišnji") != speaker.say("pètogodišnji"));
    assert!(speaker.say("čestitka") != speaker.say("čestítka"));
    assert!(speaker.say("poletan") != speaker.say("pòletan")); // pȍlētan
}

#[test]
fn place_names_follow_the_dictionaries_in_their_case_forms() {
    // The place-name table (formant_proper_names.inc, from tools/formant/places.tsv;
    // HJP, Wiktionary, Vuk's Srpski rječnik), checked 2026-10-09; see "Place
    // names" in docs/formant.md. Croatian omits unstressed length.
    const CASES: &[(&str, &str, &str)] = &[
        ("zvonko", "Idem u Pulu.", "Idem u Púlu."),
        ("zvonko", "Pula je lijepa.", "Púla je lijepa."),
        ("zvonko", "Knin", "Knȋn"),
        ("zvonko", "iz Knina", "iz Knína"),
        ("zvonko", "u Kninu", "u Knínu"),
        ("zvonko", "u Sisku", "u Sísku"),
        ("zvonko", "Vinkovci", "Vȋnkovci"),
        ("zvonko", "u Vinkovcima", "u Vȋnkovcima"),
        ("zvonko", "Slavonija", "Slàvonija"),
        ("zvonko", "u Dalmaciji", "u Dàlmaciji"),
        ("zvonko", "putujem u Liku", "putujem u Líku"),
        ("zvonko", "u Bihaću", "u Biháću"),
        ("zvonko", "Bjelovar", "Bjȅlovar"),
        ("zvonko", "Beograd", "Bȅograd"),
        ("zvonko", "u Beogradu", "u Bȅogradu"),
        ("zvonko", "u Baru", "u Báru"),
        ("zvonko", "u Slavonskom Brodu", "u Slàvonskom Brȏdu"),
        ("zvonko", "iz Solina", "iz Solína"),
        ("zvonko", "na Lošinju", "na Lošínju"),
        ("zvonko", "u Rovinju", "u Rovínju"),
        ("zvonko", "u Visokom", "u Vìsokom"),
        ("zvonko", "Ilidža", "Ilìdža"),
        ("zvonko", "u Zvorniku", "u Zvorníku"),
        ("zvonko", "Idem u Niš.", "Idem u Nȋš."),
        ("zvonko", "iz Niša", "iz Níša"),
        ("stojan", "Beograd", "Beògrad"),
        ("stojan", "u Beogradu", "u Beògradu"),
        ("stojan", "Valjevo", "Vȃljevo"),
        ("stojan", "Čačak", "Čáčak"),
        ("stojan", "Zaječar", "Zȁječār"),
        ("stojan", "u Kninu", "u Knínu"),
        ("mirsad", "Ilidža", "Ilìdža"),
        ("mirsad", "u Zvorniku", "u Zvorníku"),
        ("mirsad", "u Visokom", "u Vìsokom"),
    ];
    let mut speaker = check_accents_by_voice(CASES);
    // Only a word written with a capital letter is a place; a place spelled
    // like a frequent word is not used for the first word of a sentence
    speaker.set_voice("zvonko").expect("zvonko");
    assert!(speaker.say("Pula") != speaker.say("pula"));
    assert!(speaker.say("Bar ću doći.") == speaker.say("bar ću doći."));
    assert!(speaker.say("Visoko je.") == speaker.say("visoko je."));
    assert!(speaker.say("Ruda je skupa.") == speaker.say("ruda je skupa."));
    // Text in capitals says nothing about places (ZATVORI PROZOR is not the
    // town Prózor)
    assert!(speaker.say("ZATVORI PROZOR") == speaker.say("zatvori prozor"));
    assert!(speaker.say("u Prozoru") != speaker.say("u prozoru"));
    // The user's stems come before the table
    assert!(speaker.load_accent_lexicon(r#"{ "entries": [ { "word": "R/ovinj*" } ] }"#));
    assert!(speaker.say("u Rovinju") == speaker.say("u Ròvinju"));
}

#[test]
fn personal_names_follow_the_dictionaries_in_their_case_forms() {
    // The table of given names and surnames (formant_proper_names.inc, from
    // tools/formant/persons.tsv; HJP, Wiktionary, Vuk's Srpski rječnik) and
    // the rule for Slavic names in -mir, -dar, -zar, checked 2026-10-09; see
    // "Personal names" in docs/formant.md. Croatian omits unstressed length.
    const CASES: &[(&str, &str, &str)] = &[
        ("zvonko", "rekao je Ante", "rekao je Ánte"),
        ("zvonko", "s Antom", "s Ántom"),
        ("zvonko", "Darko i Zdenko", "Dárko i Zdénko"),
        ("zvonko", "kod Darka", "kod Dárka"),
        ("zvonko", "pjesma Ive Andrića", "pjesma Ive Ándrića"),
        ("zvonko", "s Ivanom Jurićem", "s Ivanom Júrićem"),
        ("zvonko", "o Anti Starčeviću", "o Ánti Stárčeviću"),
        ("zvonko", "Miška Kranjca", "Miška Kránjca"),
        ("zvonko", "o Ignaciju", "o Ìgnaciju"),
        ("zvonko", "kod Mate", "kod Máte"),
        ("zvonko", "s Matom", "s Mátom"),
        ("zvonko", "pozdravi Tamaru", "pozdravi Tàmaru"),
        ("zvonko", "rekao je Luka", "rekao je Lȗka"),
        ("zvonko", "s Markom", "s Mȃrkom"),
        ("zvonko", "Radomir", "Rȁdomir"),
        ("zvonko", "o Radomiru", "o Rȁdomiru"),
        ("zvonko", "Vladimir", "Vlàdimir"),
        ("zvonko", "Božidar", "Bòžidar"),
        ("zvonko", "Dragan Ranković", "Dragan Ránković"),
        ("zvonko", "s Idom", "s Ídom"),
        ("stojan", "Božidar", "Bȍžidār"),
        ("stojan", "Đorđe", "Đȏrđe"),
        ("stojan", "Radomir", "Rȁdomīr"),
        ("stojan", "rekao je Ante", "rekao je Ánte"),
    ];
    let mut speaker = check_accents_by_voice(CASES);
    // Words spelled like a form of a name keep their own reading at the
    // start of a sentence (ide, idi of Ìda; ali; niko) and in lowercase text
    speaker.set_voice("zvonko").expect("zvonko");
    assert!(speaker.say("Ide kući.") == speaker.say("ide kući."));
    assert!(speaker.say("Ali ja ne znam.") == speaker.say("ali ja ne znam."));
    assert!(speaker.say("Niko nije došao.") == speaker.say("niko nije došao."));
    assert!(speaker.say("Luka je velika.") == speaker.say("luka je velika."));
    assert!(speaker.say("Vera u Boga.") == speaker.say("vera u Boga."));
    // ... and a rare name spelled like a common word is not in the table
    // (Kolega, Era, Lasta), nor the forms of Pètar taken by Pétra
    assert!(speaker.say("Dragi Kolega") == speaker.say("Dragi kolega"));
    assert!(speaker.say("Era interneta") == speaker.say("era interneta"));
    assert!(speaker.say("Svetog Petra") == speaker.say("svetog petra"));
    assert!(speaker.say("u Mali Lošinj") == speaker.say("u mali Lošinj"));
    // The rule for -mir is for names only (gospòdār, uznemíri)
    assert!(speaker.say("Tihomira") != speaker.say("Tihomíra"));
    assert!(speaker.say("gospodar") == speaker.say("gospòdar"));
    assert!(speaker.say("Deprimira me.") == speaker.say("deprimira me."));
}

/// Slovenian names its emoji from emoji_sl.json
#[test]
fn slovenian_names_its_emoji_from_emoji_sl_json() {
    // The bundled emoji.json, or the file LAPRDUS_EMOJI names; the Slovenian
    // file is found next to it.
    let path = dictionary_or("LAPRDUS_EMOJI", "emoji.json");
    let (mut speaker, mut bare) = (Speaker::empty(), Speaker::empty());
    assert!(speaker.load_emoji_dictionary_file(&path));
    speaker.set_emoji_enabled(true);
    // Croatian: the bundled names
    assert!(speaker.say("\u{1F600}") == bare.say("nasmijano lice"));
    // Slovenian: the names of the Unicode CLDR for sl
    assert!(speaker.set_language("sl"));
    assert!(bare.set_language("sl"));
    assert!(speaker.say("\u{1F600}") == bare.say("režeči obraz"));
    assert!(speaker.say("Živijo \u{1F44B}") == bare.say("Živijo mahajoča dlan"));
    // The user's entry wins in every language, and a bundled reload drops it
    let user_file = std::env::temp_dir().join(format!("laprdus_emoji_user_test_{}.json", std::process::id()));
    std::fs::write(&user_file, "{ \"entries\": [ { \"grapheme\": \"\u{1F600}\", \"phoneme\": \"nasmeh\" } ] }")
        .expect("the user's emoji file");
    let user = std::fs::read_to_string(&user_file);
    let _ = std::fs::remove_file(&user_file);
    assert!(speaker.append_emoji_dictionary(&user.expect("the user's emoji file")));
    assert!(speaker.say("\u{1F600}") == bare.say("nasmeh"));
    assert!(speaker.set_language("hr"));
    assert!(bare.set_language("hr"));
    assert!(speaker.say("\u{1F600}") == bare.say("nasmeh"));
    assert!(speaker.load_emoji_dictionary_file(&path));
    assert!(speaker.say("\u{1F600}") == bare.say("nasmijano lice"));
    assert!(speaker.set_language("sl"));
    assert!(bare.set_language("sl"));
    assert!(speaker.say("\u{1F600}") == bare.say("režeči obraz"));
}

/// Bundled OK and NLO replacements are case insensitive whole words
#[test]
fn bundled_ok_and_nlo_replacements_are_case_insensitive_whole_words() {
    let (mut speaker, mut bare) = (Speaker::empty(), Speaker::empty());
    assert!(speaker.dictionary().load_file(&dictionary_or("LAPRDUS_DICTIONARY", "internal.json")));
    for voice in FORMANT_VOICES {
        info!("{voice}");
        speaker.set_voice(voice).expect(voice);
        bare.set_voice(voice).expect(voice);
        for text in ["OK", "ok", "Ok"] {
            assert!(speaker.say(text) == bare.say("Okej"), "{voice}: {text}");
        }
        for text in ["nlo", "NLO", "Nlo"] {
            assert!(speaker.say(text) == bare.say("En El O"), "{voice}: {text}");
        }
        assert!(speaker.say("OK, nlo!") == bare.say("Okej, En El O!"), "{voice}");
        for text in ["okolina", "sok", "poklon", "unload"] {
            assert!(speaker.say(text) == bare.say(text), "{voice}: {text}");
        }
    }
}

#[test]
fn foreign_app_names_keep_their_pronunciation_and_stress_in_case_forms() {
    // Exercise the shipped replacements as well as the accent lexicon: the
    // bundled internal.json, or the file LAPRDUS_DICTIONARY names.
    let mut speaker = Speaker::empty();
    assert!(speaker.dictionary().load_file(&dictionary_or("LAPRDUS_DICTIONARY", "internal.json")));
    const CASES: &[(&str, &str)] = &[
        ("messenger", "Mȅsindžer"),
        ("Messenger", "Mȅsindžer"),
        ("MESSENGER", "Mȅsindžer"),
        ("Messengera", "Mȅsindžera"),
        ("Messengeru", "Mȅsindžeru"),
        ("Messengerom", "Mȅsindžerom"),
        ("poruka na Messengeru", "poruka na Mȅsindžeru"),
        ("tiktok", "Tȉktok"),
        ("TikTok", "Tȉktok"),
        ("TIKTOK", "Tȉktok"),
        ("TikToka", "Tȉktoka"),
        ("TikToku", "Tȉktoku"),
        ("TikTokom", "Tȉktokom"),
        ("video na TikToku", "video na Tȉktoku"),
        ("wintalker", "Vintòker"),
        ("WinTalker", "Vintòker"),
        ("WINTALKER", "Vintòker"),
        ("WinTalkera", "Vintòkera"),
        ("WinTalkeru", "Vintòkeru"),
        ("WinTalkerom", "Vintòkerom"),
        ("govori WinTalkerom", "govori Vintòkerom"),
    ];
    for voice in FORMANT_VOICES {
        info!("{voice}");
        speaker.set_voice(voice).expect(voice);
        let mut failures = Failures::new();
        for &(plain, accented) in CASES {
            info!("{plain}");
            same_as_accented(&mut speaker, &mut failures, plain, accented);
        }
        failures.finish();
        // Whole-word suffix patterns must not rewrite longer unrelated words.
        for text in ["messengerica", "wintalkerica"] {
            let mut bare = Speaker::new(voice);
            assert!(speaker.say(text) == bare.say(text), "{voice}: {text}");
        }
    }
}

#[test]
fn english_words_are_stressed_where_english_stresses_them() {
    // A word with a capital after small letters is read as its parts,
    // unless a lexicon knows the whole word (TikTok) or every part is one
    // syllable (TalkBack); a single small letter before the capital (also
    // the Lj, Nj, Dž of Serbian Cyrillic capitals), a single capital at
    // the end, and capitals followed by small letters keep the word whole.
    const SAME: &[(&str, &str)] = &[
        ("ElevenLabs", "Eleven Labs"),
        ("TalkBack", "talkback"),
        ("TalkBacku", "talkbacku"),
        ("ChatGPT", "Chat GPT"),
        ("iPhone", "iphone"),
        ("BiH", "bih"),
        ("PDFom", "pdfom"),
        ("SMSati", "smsati"),
        ("KONZUMklik", "konzumklik"),
        ("McDonald", "Mcdonald"),
        ("TikTok", "tiktok"),
        ("TikToka", "tiktoka"),
        ("ЉУБАВЉУ", "љубављу"),
        ("КЊИГА", "књига"),
        ("ПРИЈАТЕЉИМА", "пријатељима"),
        // A native word made from an English stem keeps its native suffix
        ("chatirati", "chatírati"),
        // Case does not matter, and neither does a Croatian case ending on
        // a word spelled the English way
        ("Croatian", "croatian"),
        ("CROATIAN", "croatian"),
    ];
    // Native words keep their native accents, also those spelled like an
    // English word with an ending (operate, distribute); the Serbian and
    // Bosnian voices keep the length after the accent.
    const NATIVE: &[(&str, &str)] = &[("telefon", "telèfon"), ("telefona", "telefóna")];
    const NATIVE_HR: &[(&str, &str)] = &[("operater", "operàter"), ("distributer", "distribùter")];
    const NATIVE_SR: &[(&str, &str)] = &[("operater", "operàtēr"), ("distributer", "distribùtēr")];
    for voice in FORMANT_VOICES {
        info!("{voice}");
        let mut speaker = Speaker::new(voice);
        let mut failures = Failures::new();
        for &(a, b) in SAME {
            info!("{a}");
            same_as_accented(&mut speaker, &mut failures, a, b);
        }
        for &(a, b) in NATIVE.iter().chain(if voice == "zvonko" { NATIVE_HR } else { NATIVE_SR }) {
            info!("{a}");
            failures.check(speaker.say(a) == speaker.say(b), || format!("{voice}: {a} is not {b}"));
        }
        failures.finish();

        // The stress is where English has it: Croàtian peaks long before
        // the native rules' Croatìan, Ábleton on its first syllable, not on
        // the loan rule's Ablèton, Elèven not on the first. It is a plain
        // stress, not the rising accent a native word would have there.
        let croatian_audio = speaker.say("Croatian");
        let croatian = pitch_peak_ms(&croatian_audio);
        assert!(croatian > 0.0, "{voice}");
        assert!(croatian + 80.0 < pitch_peak_ms(&speaker.say("Croatìan")), "{voice}");
        assert!(croatian_audio != speaker.say("Croàtian"), "{voice}");
        assert!(pitch_peak_ms(&speaker.say("Ableton")) + 80.0 < pitch_peak_ms(&speaker.say("Ablèton")), "{voice}");
        assert!(pitch_peak_ms(&speaker.say("Eleven")) > pitch_peak_ms(&speaker.say("Èleven")) + 50.0, "{voice}");
        // The English first syllable is the plain stress of an unknown word
        // stressed there by the user's accent lexicon.
        let mut marked = Speaker::new(voice);
        assert!(marked.load_accent_lexicon(r#"{ "entries": [ { "word": "'ableton" } ] }"#), "{voice}");
        assert!(speaker.say("Ableton") == marked.say("Ableton"), "{voice}");
    }
}

#[test]
fn word_class_rules_place_the_accent_of_loans_derived_nouns_and_prefixed_verbs() {
    // Words outside the lexicon: the rules for nouns with a long last stem
    // syllable, prefixed verbs, verbs in -ovati, nouns in -ica and -anin and
    // adjectives in -izan/-ozan (StressRules::strong).
    const CASES: &[(&str, &str)] = &[
        // nouns with a long last stem syllable: case forms
        ("zemljaka", "zemljáka"),
        ("čarobnjaka", "čarobnjáka"),
        ("čarobnjacima", "čarobnjácima"),
        ("programera", "programéra"),
        ("inženjera", "inženjéra"),
        ("rezultata", "rezultáta"),
        ("rezultatima", "rezultátima"),
        ("kapetana", "kapetána"),
        ("balkona", "balkóna"),
        ("sezone", "sezóne"),
        ("broj računa", "broj račúna"),
        ("računa", "račúna"), // račúna (the verb carries the noun-twin flag)
        ("čuvara", "čuvára"),
        ("novinari", "novinári"),
        ("prodavača", "prodaváča"),
        ("igrača", "igráča"),
        ("košarkaša", "košarkáša"),
        ("čokolade", "čokoláde"),
        ("piramida", "piramída"),
        ("analiza", "analíza"),
        ("kanalom", "kanálom"),
        ("materijalu", "materijálu"),
        ("motiva", "motíva"),
        ("Francuza", "Francúza"),
        ("zaposlenika", "zaposleníka"),
        ("povjerenici", "povjereníci"),
        ("zarobljenika", "zarobljeníka"),
        ("dobitka", "dobítka"),
        ("Amerikanca", "Amerikánca"),
        ("Amerikanac", "Amerikánac"),
        ("čuvarov", "čuvárov"),
        // the nominative: one syllable back, Zvonko without the length
        ("čarobnjak", "čaròbnjak"),
        ("inženjer", "inžènjer"),
        ("rezultat", "rezùltat"),
        ("prodavač", "prodàvač"),
        ("vitamin", "vitàmin"),
        ("festival", "festìval"),
        ("zarobljenik", "zaròbljenik"),
        // prefixed verbs follow the infinitive
        ("pogledala", "pògledala"), // ŠR/HJP perfective, lexical exception
        ("pogledamo", "pògledamo"),
        ("napravio", "napràvio"),
        ("dočekali", "dočèkali"),
        ("iskusio", "iskùsio"),
        ("zahvalismo", "zahválismo"), // zahválismo (VERBS: zahváliti)
        ("zatražismo", "zatrážismo"),
        ("pohvalismo", "pohválismo"),
        ("ispitasmo", "ispìtasmo"),
        ("pokušavši", "pòkušavši"), // pòkušavši (ŠR/HJP pòkušati)
        ("poželjeti", "požèljeti"),
        // verbs in -ovati
        ("putovao", "putòvao"),
        ("kupovala", "kupòvala"),
        ("vjerovao", "vjȅrovao"),
        // nouns in -ica, inhabitants, adjectives
        ("čuvarica", "čuvàrica"),
        ("voditeljica", "voditèljica"),
        ("državljanin", "držàvljanin"),
        ("državljanina", "držàvljanina"),
        ("precizan", "precìzan"),
        ("nervozna", "nervózna"),
        // the -ina rule and its exceptions
        ("balerina", "balerìna"),
        ("kućetina", "kućètina"),
        ("mješavina", "mješàvina"),
        ("planina", "planìna"),
        ("trećina", "trećìna"),
        // the words the rules would otherwise take: lexicon
        ("zakona", "zákona"),
        ("spomenika", "spȍmenika"),
        ("vojnika", "vojníka"),
        ("milijuna", "milijúna"),
    ];
    let mut speaker = Speaker::new("zvonko");
    let mut failures = Failures::new();
    for &(plain, accented) in CASES {
        info!("{plain}");
        same_as_accented(&mut speaker, &mut failures, plain, accented);
    }

    // Native words of the same shape keep the first syllable: the rules
    // must not move their accent.
    const NOT_THESE: &[(&str, &str)] = &[
        ("jezera", "jezéra"),
        ("večera", "večéra"),
        ("servera", "servéra"),
        ("predmeta", "predméta"),
        ("poslana", "poslána"),
        ("poznata", "poznáta"),
        ("imate", "imáte"),
        ("gledati", "gledáti"),
        ("gledala", "gledála"),
        ("radnika", "radníka"),
        ("korisnika", "korisníka"),
        ("osjetljiva", "osjetljíva"),
        ("naziva", "nazíva"),
        ("Ivanin", "Ivànin"),
        ("mamina", "mamìna"),
        ("lozinke", "lozínke"),
        ("zadruzi", "zadrúzi"),
        ("poseban", "posèban"),
        ("dosadan", "dosàdan"),
        ("nagrada", "nagráda"),
        ("svijeta", "svijéta"),
        ("događaju", "dogàđaju"),
        ("učenik", "učènik"),       // učènik (ȕčenīk)
        ("učenika", "učeníka"),     // učeníka (ȕčenīka)
        ("poznanici", "poznaníci"), // poznaníci (pòznanīci)
        ("udara", "udára"),         // udára (ùdara, a verb in VERBS)
        ("ispune", "ispúne"),       // ispúne (ìspune)
        ("umire", "umíre"),         // umíre (ùmire)
    ];
    for &(plain, accented) in NOT_THESE {
        info!("{plain}");
        failures.check(speaker.say(plain) != speaker.say(accented), || format!("zvonko: {plain} is {accented}"));
    }

    // Stojan keeps the length the dictionaries write after the retracted
    // accent of the nominative.
    const SERBIAN: &[(&str, &str)] = &[
        ("čarobnjak", "čaròbnjāk"),
        ("prodavač", "prodàvāč"),
        ("zarobljenik", "zaròbljenīk"),
        ("prodavača", "prodaváča"),
    ];
    speaker.set_voice("stojan").expect("stojan");
    for &(plain, accented) in SERBIAN {
        info!("{plain}");
        failures.check(speaker.say(plain) == speaker.say(accented), || format!("stojan: {plain} is not {accented}"));
    }
    failures.finish();
}

// =============================================================================
// Voice parameters, voice quality, volume, currency, telephone numbers, short pauses
// =============================================================================

/// A value within 1e-5 of what is expected.
fn approx(value: f32, expected: f32) -> bool {
    (f64::from(value) - f64::from(expected)).abs() < 0.00001
}

// =============================================================================
// Parameters
// =============================================================================

#[test]
fn speech_rate_changes_duration_not_pitch() {
    let mut engine = Speaker::new("zvonko");
    engine.set_inflection_level(0.0);

    let text = "mama ima malu nanu";
    let normal = engine.say(text);

    engine.set_speed(2.0);
    let fast = engine.say(text);

    engine.set_speed(0.5);
    let slow = engine.say(text);

    assert!((fast.len() as f64) < normal.len() as f64 * 0.7);
    assert!(slow.len() as f64 > normal.len() as f64 * 1.6);
    assert!(close_to(pitch_hz(&fast), pitch_hz(&normal), 0.06));
    assert!(close_to(pitch_hz(&slow), pitch_hz(&normal), 0.06));
}

#[test]
fn pitch_settings_change_pitch_not_duration() {
    let mut engine = Speaker::new("zvonko");
    engine.set_inflection_level(0.0);

    let normal = engine.say("aaa");
    let base = pitch_hz(&normal);

    engine.set_user_pitch(1.5);
    let high = engine.say("aaa");
    assert!(close_to(pitch_hz(&high), base * 1.5, 0.06));
    assert_eq!(high.len(), normal.len());

    engine.set_user_pitch(1.0);
    engine.set_pitch(0.75);
    let low = engine.say("aaa");
    assert!(close_to(pitch_hz(&low), base * 0.75, 0.06));
    assert_eq!(low.len(), normal.len());
}

#[test]
fn inflection_level_scales_the_pitch_movements() {
    let mut engine = Speaker::new("zvonko");
    assert!(approx(engine.params().inflection_level, 0.5));

    let text = "Marija danas putuje u Zagreb?";
    let normal = engine.say(text);

    // The default level is the measured contour: setting it explicitly
    // changes nothing.
    engine.set_inflection_level(0.5);
    assert!(engine.say(text) == normal);

    engine.set_inflection_level(0.0);
    let flat = engine.say(text);
    assert_eq!(flat.len(), normal.len());
    assert!(pitch_spread_hz(&flat) < 4.0);

    engine.set_inflection_level(1.0);
    let wide = engine.say(text);
    assert_eq!(wide.len(), normal.len());
    assert!(pitch_spread_hz(&wide) > pitch_spread_hz(&normal) * 1.3);
    assert!(pitch_spread_hz(&normal) > pitch_spread_hz(&flat) + 10.0);

    // Out-of-range values are clamped, not rejected.
    engine.set_inflection_level(7.0);
    assert!(approx(engine.params().inflection_level, 1.0));
}

#[test]
fn acceleration_multiplies_the_speech_rate() {
    let mut engine = Speaker::new("stojan");
    assert!(approx(engine.params().acceleration, 1.0));

    let text = "Mama ima malu nanu.";
    let normal = engine.say(text);

    engine.set_speed(2.0);
    let fast = engine.say(text);

    // Speed 1.0 at acceleration 2.0 is speed 2.0: the same samples.
    engine.set_speed(1.0);
    engine.set_acceleration(2.0);
    assert!(engine.say(text) == fast);

    // The host's top rate goes past the plain 4.0x with acceleration.
    engine.set_speed(4.0);
    let fastest = engine.say(text);
    engine.set_acceleration(1.0);
    let plain = engine.say(text);
    assert!((fastest.len() as f64) < plain.len() as f64 * 0.8);

    // And below the normal rate with a low acceleration.
    engine.set_speed(1.0);
    engine.set_acceleration(0.5);
    let slow = engine.say(text);
    assert!(slow.len() as f64 > normal.len() as f64 * 1.6);

    engine.set_acceleration(100.0);
    assert!(approx(engine.params().acceleration, 3.0));
}

#[test]
fn the_volume_boost_makes_the_speech_louder_without_clipping() {
    // The formant voices peak near full scale with a median level near
    // -20 dBFS; the boost drives them through a peak limiter whose gain
    // moves only between syllables, so the level comes up (2.3 times the
    // RMS here) while the peaks stay under the ceiling and the waveform of
    // a syllable stays a scaled copy of the plain one: no clipping. (The
    // boost of 2.0 rounded every vowel peak off with a soft clipper and
    // left a residual of -18 dB, which phone users heard as distortion.)
    // In boost mode the treble is shelved down first (VOLUME_BOOST_TREBLE_DB
    // from VOLUME_BOOST_TREBLE_HZ up), so the copy is of the shelved speech.
    // The volume still scales the boosted output, so a host's volume
    // control keeps working.
    let mut engine = Speaker::new("zvonko");
    assert!(!engine.params().volume_boost);

    let text = "Pozivam Marka na mobitel. Uključi zvučnik!";
    let plain = engine.say(text);

    engine.set_volume_boost(true);
    assert!(engine.params().volume_boost);
    let boosted = engine.say(text);
    assert_eq!(boosted.len(), plain.len());
    assert!(rms(&boosted) > rms(&plain) * 1.5);
    assert!(peak(&boosted) < 32000); // held under the ceiling
    assert!(f64::from(peak(&boosted)) > 32000.0 * 0.8); // and close to it
    assert!(residual_db(&treble_shelf(&plain), &boosted) < -40.0); // a gain, not a waveshaper
    assert!(residual_db(&plain, &boosted) > -36.0); // the shelf is there
    assert!(engine.say(text) == boosted); // deterministic

    // Half volume with the boost is about the plain level: the boost is
    // applied before the volume.
    engine.set_volume(0.5);
    let half = engine.say(text);
    assert!(rms(&half) < rms(&boosted) * 0.6);
    assert!(rms(&half) > rms(&plain) * 0.7);

    // Off again: the plain samples.
    engine.set_volume(1.0);
    engine.set_volume_boost(false);
    assert!(engine.say(text) == plain);

    // The singing presets and the other formant voices take it too.
    for voice in ["stojan", "mirsad", "lika_velebit"] {
        engine.set_voice(voice).expect(voice);
        assert!(!engine.params().volume_boost, "{voice}");
        let quiet = engine.say(text);
        engine.set_volume_boost(true);
        let loud = engine.say(text);
        assert!(rms(&loud) > rms(&quiet) * 1.2, "{voice}");
        assert!(peak(&loud) < 32000, "{voice}");
        engine.set_volume_boost(false);
    }

    // The hissing consonants are held under the vowels in boost mode
    // (VOLUME_BOOST_HISS_DB; a listener heard them pierce at full phone
    // volume), and nothing goes over full scale between the samples, which a
    // phone's resampler would clip. Measured: s, c and ć 4-8 dB further
    // under the vowel than without the boost; the true peak -0.7 dBTP.
    for voice in FORMANT_VOICES {
        info!("{voice}");
        engine.set_voice(voice).expect(voice);
        for item in ["asa", "aca", "aća"] {
            info!("{item}");
            engine.set_volume_boost(false);
            let quiet = engine.say(item);
            engine.set_volume_boost(true);
            let loud = engine.say(item);
            let (plain_db, boosted_db) = hiss_pair_db(&quiet, &loud);
            info!("hiss re the vowel: plain {plain_db} dB, boosted {boosted_db} dB");
            assert!(boosted_db < plain_db - 3.0, "{voice}, {item}: plain {plain_db} dB, boosted {boosted_db} dB");
        }
        let sentence = engine.say(text);
        let true_peak = true_peak(&sentence);
        info!("true peak {} dBFS", 20.0 * (true_peak / 32768.0).log10());
        assert!(true_peak < 32768.0 * 10f64.powf(-0.2 / 20.0), "{voice}: true peak {true_peak}");
        engine.set_volume_boost(false);
    }
}

/// The share of the energy above about 1.5 kHz (a one-pole high-pass).
fn top_share(audio: &[i16]) -> f64 {
    let (mut low, mut top, mut total) = (0.0, 0.0, 0.0);
    // Pi as the measurement was written.
    #[allow(clippy::approx_constant)]
    let coef = (-2.0 * 3.14159265 * 1500.0 / 22050.0f64).exp();
    for &s in audio {
        let s = f64::from(s);
        low = coef * low + (1.0 - coef) * s;
        let hp = s - low;
        top += hp * hp;
        total += s * s;
    }
    if total > 0.0 { top / total } else { 0.0 }
}

#[test]
fn voice_quality_settings_shape_the_voice_and_50_leaves_it_alone() {
    // Head size, breathiness, roughness and brightness, 0-100 each, after
    // synthesizer A's voice parameters: 50 is the speaker's own value, so the
    // defaults change nothing; the ends change the sound but never the
    // length; out-of-range values are clamped.
    let mut engine = Speaker::new("zvonko");
    assert_eq!(engine.params().head_size, 50);
    assert_eq!(engine.params().breathiness, 50);
    assert_eq!(engine.params().roughness, 50);
    assert_eq!(engine.params().brightness, 50);

    let text = "Pozivam Marka na mobitel.";
    let normal = engine.say(text);
    engine.set_head_size(50);
    engine.set_breathiness(50);
    engine.set_roughness(50);
    engine.set_brightness(50);
    assert!(engine.say(text) == normal);

    // Head size: a long tract (0) lowers every formant, a short one (100)
    // raises them (the higher value is the higher voice, at the
    // maintainer's word); the pitch and the length stay.
    engine.set_head_size(100);
    let small = engine.say(text);
    engine.set_head_size(0);
    let big = engine.say(text);
    assert_eq!(small.len(), normal.len());
    assert_eq!(big.len(), normal.len());
    assert!(top_share(&small) > top_share(&normal));
    assert!(top_share(&big) < top_share(&normal));
    assert!(close_to(pitch_hz_in(&big, 0.3, 0.5), pitch_hz_in(&normal, 0.3, 0.5), 0.03));
    engine.set_head_size(140); // clamped
    assert_eq!(engine.params().head_size, 100);
    engine.set_head_size(-3);
    assert_eq!(engine.params().head_size, 0);
    engine.set_head_size(50);

    // Brightness: the tone, 0 dull, 100 sharp.
    engine.set_brightness(0);
    let dull = engine.say(text);
    engine.set_brightness(100);
    let sharp = engine.say(text);
    assert_eq!(dull.len(), normal.len());
    assert!(top_share(&dull) < top_share(&normal));
    assert!(top_share(&sharp) > top_share(&normal));
    // Brightness reaches the hiss too (until November 2026 it shaped the
    // voicing alone and 0 left the s as sharp as before).
    // Measured: the s of asa 8 dB further under the vowel at 0 than at 50; at
    // 100 the vowels and the hiss grow sharper together (top_share above).
    {
        engine.set_brightness(50);
        let s50 = engine.say("asa");
        engine.set_brightness(0);
        let s0 = engine.say("asa");
        let (at_50, at_0) = hiss_pair_db(&s50, &s0);
        info!("hiss of asa re the vowel: 0 {at_0}, 50 {at_50}");
        assert!(at_0 < at_50 - 5.0, "hiss of asa re the vowel: 0 {at_0}, 50 {at_50}");
    }
    engine.set_brightness(50);

    // The lower halves are heard too (a listener heard breathiness and
    // roughness change only above 50, November 2026): breathiness 0 is a
    // pressed voice, its top three times the share of 50's (open quotient
    // 0.30, source emphasis 0.88, the bass shelf mostly gone). The sharpest
    // settings raise the peaks, and the plain output's limiter holds them
    // under the soft knee instead of letting it bend them (a waveshaping
    // residual of -16 dB at breathiness 0 with brightness 100 before).
    engine.set_breathiness(0);
    let pressed = engine.say(text);
    assert_eq!(pressed.len(), normal.len());
    assert!(top_share(&pressed) > top_share(&normal) * 2.0);
    engine.set_brightness(100);
    let hottest = engine.say(text);
    assert!(peak(&hottest) <= (0.8f32 * 32000.0f32) as i32 + 1);
    engine.set_volume(0.5);
    let hottest_half = engine.say(text);
    let waveshaping = waveshaping_db(&hottest, &hottest_half);
    info!("waveshaping {waveshaping} dB");
    assert!(waveshaping < -35.0, "waveshaping {waveshaping} dB");
    engine.set_volume(1.0);
    engine.set_brightness(50);
    engine.set_breathiness(50);
    assert!(engine.say(text) == normal);

    // Breathiness and roughness change the voice and nothing else (what
    // breathiness does is measured in "Breathiness runs from a tense voice
    // to a whisper").
    engine.set_breathiness(100);
    let breathy = engine.say(text);
    assert_eq!(breathy.len(), normal.len());
    assert!(breathy != normal);
    engine.set_breathiness(0);
    let tense = engine.say(text);
    assert_eq!(tense.len(), normal.len());
    assert!(tense != normal);
    engine.set_breathiness(50);
    // Roughness above 50 adds diplophonia (every second period shorter
    // and weaker), so 75 and 100 are different voices, not more of the
    // same noise.
    engine.set_roughness(100);
    let rough = engine.say(text);
    assert_eq!(rough.len(), normal.len());
    assert!(rough != normal);
    engine.set_roughness(75);
    let rougher = engine.say(text);
    assert_eq!(rougher.len(), normal.len());
    assert!(rougher != normal);
    assert!(rougher != rough);
    engine.set_roughness(50);
    assert!(engine.say(text) == normal);

    // The other formant voices and a singing preset take them too.
    for voice in ["stojan", "mirsad", "lika_velebit"] {
        engine.set_voice(voice).expect(voice);
        assert_eq!(engine.params().head_size, 50, "{voice}");
        let own = engine.say(text);
        engine.set_head_size(0);
        assert!(engine.say(text) != own, "{voice}");
        engine.set_head_size(50);
    }
}

#[test]
fn roughness_is_a_rasp_above_50_and_a_steady_voice_at_0() {
    // A sustained a at half rate and a flat pitch. Above 50 diplophonia puts
    // a series at odd multiples of F0/2 between the harmonics; it is
    // measured in 0.5-3 kHz, which a phone's speaker plays. Measured on
    // Zvonko's paragraph (November 2026): the series 9 dB under the
    // harmonics at 50, 0.7 dB under them at 75 and 0.4 dB over them at 100
    // (3.2 and 1.6 dB under with the weaker diplophonia before). At 0 the
    // slow drift of the pitch goes too: a perfectly steady voice.
    let mut engine = Speaker::new("zvonko");
    engine.set_speed(0.5);
    engine.set_inflection_level(0.0);
    let mut at = |roughness: i32| {
        engine.set_roughness(roughness);
        engine.say("aaaaaa")
    };
    let steady = at(0);
    let normal = at(50);
    let rasp = at(75);
    let hoarse = at(100);
    assert_eq!(steady.len(), normal.len());
    assert!(steady != normal);
    let f0 = pitch_hz(&normal);
    assert!(f0 > 80.0);
    let subharmonics_db = |audio: &[i16]| {
        let (mut harm, mut sub) = (0.0, 0.0);
        let mut k = 1.0;
        while k * f0 <= 3000.0 {
            if k * f0 >= 500.0 {
                harm += 10f64.powf(component_db(audio, k * f0) / 10.0);
            }
            if (k + 0.5) * f0 >= 500.0 && (k + 0.5) * f0 <= 3000.0 {
                sub += 10f64.powf(component_db(audio, (k + 0.5) * f0) / 10.0);
            }
            k += 1.0;
        }
        10.0 * (sub / harm).log10()
    };
    let (s50, s75, s100) = (subharmonics_db(&normal), subharmonics_db(&rasp), subharmonics_db(&hoarse));
    info!("F0/2 series re the harmonics in 0.5-3 kHz: 50 {s50}, 75 {s75}, 100 {s100}");
    assert!(s75 > s50 + 6.0, "50 {s50}, 75 {s75}");
    assert!(s100 > s50 + 6.0, "50 {s50}, 100 {s100}");
    assert!(periodicity(&steady) >= periodicity(&normal));
}

#[test]
fn breathiness_runs_from_a_tense_voice_to_a_whisper() {
    // A sustained a at half rate and a flat pitch, as spectrum.py --floor
    // measures it (docs/formant.md, "Breathiness, from a tense voice to a
    // whisper"). Measured for Zvonko: periodicity 0.987 at 0, 0.974 at 50,
    // 0.685 at 75, 0.340 at 100; H1-H2 -1.0 dB at 0, 4.3 at 50; the
    // whispered a 1 dB under the voiced one. Until 2026-11-07 breathiness
    // only scaled a breath noise too faint to hear, and 0 and 100 sounded
    // like 50.
    for voice in FORMANT_VOICES {
        info!("{voice}");
        let mut engine = Speaker::new(voice);
        engine.set_speed(0.5);
        engine.set_inflection_level(0.0);
        let mut at = |breathiness: i32| {
            engine.set_breathiness(breathiness);
            engine.say("aaaaaa")
        };
        let tense = at(0);
        let normal = at(50);
        let breathy = at(75);
        let whisper = at(100);
        assert_eq!(tense.len(), normal.len(), "{voice}");
        assert_eq!(breathy.len(), normal.len(), "{voice}");
        assert_eq!(whisper.len(), normal.len(), "{voice}");

        // Voiced at 0, 50 and (less steadily) 75; no voice at all at 100.
        let p50 = periodicity(&normal);
        let (p0, p75, p100) = (periodicity(&tense), periodicity(&breathy), periodicity(&whisper));
        let measured = format!("{voice}: periodicity 0 {p0}, 50 {p50}, 75 {p75}, 100 {p100}");
        info!("{measured}");
        assert!(p50 > 0.9, "{measured}");
        assert!(p0 > 0.9, "{measured}");
        assert!(p75 < p50 - 0.15, "{measured}");
        assert!(p75 > 0.5, "{measured}");
        assert!(p100 < 0.5, "{measured}");

        // The whisper keeps the loudness of the vowel within a few dB.
        let level_db = 20.0 * (rms(&whisper) / rms(&normal)).log10();
        info!("whisper re voice {level_db} dB");
        assert!(level_db > -8.0, "{voice}: whisper re voice {level_db} dB");
        assert!(level_db < 3.0, "{voice}: whisper re voice {level_db} dB");

        // Tense: the first harmonic sinks towards the second (a pressed
        // voice); breathy: it rises above it.
        let f0 = pitch_hz(&normal);
        assert!(f0 > 80.0, "{voice}");
        let h1_h2 = |audio: &[i16]| component_db(audio, f0) - component_db(audio, 2.0 * f0);
        let (h0, h50, h75) = (h1_h2(&tense), h1_h2(&normal), h1_h2(&breathy));
        info!("H1-H2 0 {h0}, 50 {h50}, 75 {h75}");
        assert!(h0 < h50 - 3.0, "{voice}: H1-H2 0 {h0}, 50 {h50}");
        assert!(h75 > h50 + 2.0, "{voice}: H1-H2 50 {h50}, 75 {h75}");
    }
}

#[test]
fn a_clause_that_opens_with_a_fricative_rises_from_silence() {
    // Until November 2026 the friction track's smoothing reached into the
    // three lead frames, and the first output sample of "sapa" jumped to
    // 4 % of full scale: a tick at the start of every utterance that began
    // with s, š, f or h. The lead is now one frame longer than the smoothing
    // radius: the first 2 ms are digital silence and the friction ramps up
    // from there (1.5 % of full scale in the next 2 ms for "sapa").
    let mut engine = Speaker::new("zvonko");
    for text in ["sapa", "škola", "sve", "hvala", "fotografija", "zvono"] {
        info!("{text}");
        let audio = engine.say(text);
        assert!(audio.len() > 88, "{text}");
        let first = peak(&audio[..44]);
        let second = peak(&audio[44..88]);
        assert_eq!(first, 0, "{text}");
        assert!(f64::from(second) < 32768.0 * 0.03, "{text}: {second}");
    }
}

#[test]
fn the_speech_rate_still_changes_the_rate_under_an_acceleration() {
    // Hosts scale the user's own rate and pitch, which they read back with
    // the getters after loading the user's configuration; the acceleration
    // multiplies the rate without taking its place.
    let mut engine = Speaker::new("zvonko");
    assert!(approx(engine.params().speed, 1.0));
    assert!(approx(engine.params().user_pitch, 1.0));
    engine.set_acceleration(1.5);

    let text = "Mama ima malu nanu.";
    let normal = engine.say(text);
    engine.set_speed(2.0);
    assert!(approx(engine.params().speed, 2.0));
    let fast = engine.say(text);
    assert!((fast.len() as f64) < normal.len() as f64 * 0.75);

    engine.set_user_pitch(1.25);
    assert!(approx(engine.params().user_pitch, 1.25));

    // Out-of-range values are clamped.
    engine.set_speed(9.0);
    assert!(approx(engine.params().speed, 4.0));
}

#[test]
fn formant_voices_accept_the_wider_rate_and_pitch_ranges() {
    let mut engine = Speaker::new("zvonko");
    engine.set_inflection_level(0.0);

    let text = "mama ima malu nanu";
    engine.set_speed(0.5);
    let slow = engine.say(text);
    engine.set_speed(0.25);
    let slower = engine.say(text);
    assert!(slower.len() as f64 > slow.len() as f64 * 1.7);

    engine.set_speed(1.0);
    let normal = engine.say("aaa");
    let base = pitch_hz(&normal);
    engine.set_user_pitch(3.0);
    assert!(close_to(pitch_hz(&engine.say("aaa")), base * 3.0, 0.06));
    // Below 0.5 the voice reaches its 45 Hz floor, which the pitch detector
    // of these tests cannot follow; the audio must still come out whole.
    engine.set_user_pitch(0.25);
    let lowest = engine.say("aaa");
    assert_eq!(lowest.len(), normal.len());
    assert!(rms(&lowest) > rms(&normal) * 0.3);
}

/// A settings.json of version 1.0 keeps its meaning
///
/// The settings are applied as `Engine::load_user_config` applies them, from
/// the text of the file; the C API tests read the same files from a
/// configuration directory.
#[test]
fn a_settings_json_of_version_1_0_keeps_its_meaning() {
    // Version 1.0 ignored an unforced speed and pitch and had an inflection
    // switch that no settings screen shows any more; reading such a file
    // must not change the rate or leave the voice monotone for good.
    let load = |json: &str| {
        let mut engine = Speaker::new("zvonko");
        let settings = UserSettings::parse(json).unwrap_or_default();
        let mut params = VoiceParams { pitch: engine.params().pitch, ..settings.to_voice_params() };
        params.clamp();
        engine.set_params(&params);
        *engine.params()
    };

    let v1 = load(
        r#"{ "version": "1.0",
        "speech": { "speed": 1.6, "pitch": 1.3, "volume": 1, "inflection": false },
        "force": { "speed": false, "pitch": false, "volume": false } }"#,
    );
    assert!(approx(v1.speed, 1.0));
    assert!(approx(v1.user_pitch, 1.0));
    // The 1.0 switch controlled another effect; it does not set the level
    assert!(approx(v1.inflection_level, 0.5));

    let forced = load(
        r#"{ "version": "1.0",
        "speech": { "speed": 1.6, "pitch": 1.3, "volume": 1, "inflection": true },
        "force": { "speed": true, "pitch": true, "volume": false } }"#,
    );
    assert!(approx(forced.speed, 1.6));
    assert!(approx(forced.user_pitch, 1.3));
    assert!(approx(forced.inflection_level, 0.5));

    let v2 = load(
        r#"{ "version": "2.0",
        "speech": { "speed": 1.6, "pitch": 1.3, "volume": 1,
                    "inflection_level": 0.7, "acceleration": 1 },
        "force": { "speed": false, "pitch": false, "volume": false } }"#,
    );
    assert!(approx(v2.speed, 1.6));
    assert!(approx(v2.user_pitch, 1.3));
    assert!(approx(v2.inflection_level, 0.7));

    // The voice quality keys (November 2026): read as integers, clamped to
    // 0-100, 50 when a file lacks them (v2 above has none).
    let quality = load(
        r#"{ "version": "2.0",
            "speech": { "speed": 1, "pitch": 1, "volume": 1, "inflection_level": 0.5,
                        "acceleration": 1, "head_size": 20, "breathiness": 140,
                        "roughness": 0, "brightness": 70.6 } }"#,
    );
    assert_eq!(quality.head_size, 20);
    assert_eq!(quality.breathiness, 100);
    assert_eq!(quality.roughness, 0);
    assert_eq!(quality.brightness, 71);
}

#[test]
fn nominal_words_per_minute_are_known_for_every_voice() {
    let wpm = |id: &str| voice::find(id).map_or(0.0, Voice::nominal_wpm);
    assert!(approx(wpm("zvonko"), 175.0));
    assert!(approx(voice::default_voice().nominal_wpm(), 175.0));
    assert!(wpm("stojan") > wpm("mirsad"));
    assert!(wpm("josip") > 0.0);
    assert!(wpm("djedo") > 0.0);
    assert_eq!(wpm("djed"), wpm("djedo")); // old id
    assert_eq!(wpm("nobody"), 0.0);
}

#[test]
fn volume_scales_the_output_level() {
    let mut engine = Speaker::new("mirsad");

    let full = engine.say("Dobar dan");
    engine.set_volume(0.5);
    let half = engine.say("Dobar dan");
    engine.set_volume(0.0);
    let silent = engine.say("Dobar dan");

    assert_eq!(half.len(), full.len());
    assert!(close_to(rms(&half), rms(&full) * 0.5, 0.1));
    assert_eq!(peak(&silent), 0);
}

// =============================================================================
// Text that reaches the audio: currency, telephone numbers, short pauses
// =============================================================================

#[test]
fn currency_amounts_speak_their_value_minus_and_unit_before_punctuation() {
    let mut engine = Speaker::new("zvonko");
    // Turning punctuation pauses off cannot remove semantic signs or units.
    engine.set_comma_pause(0);
    engine.set_sentence_pause(0);
    let expected = engine.say("minus dvije tisuće petsto zarez nula nula eura");
    assert!(!expected.is_empty());
    assert!(engine.say("-2.500,00 €") == expected);
    assert!(engine.say("€\u{2212}2,500.00") == expected);
    assert!(engine.say("(2.500,00 EUR)") == expected);
    assert!(engine.say("Stanje 2.150,00") == engine.say("Stanje dvije tisuće sto pedeset zarez nula nula"));
    assert!(engine.say("$2,150.00") == engine.say("dvije tisuće sto pedeset zarez nula nula dolara"));
    engine.set_number_mode(NumberMode::DigitByDigit);
    assert!(engine.say("-$2,150.00") == engine.say("minus dva jedan pet nula zarez nula nula dolara"));
}

#[test]
fn telephone_grouping_reaches_the_audio_and_honors_comma_pauses() {
    let mut engine = Speaker::new("zvonko");
    engine.set_number_mode(NumberMode::DigitByDigit);
    let words = "plus tri osam pet, devet jedan, jedan dva tri, četiri pet šest sedam";
    let spoken = engine.say("+385911234567");
    assert!(!spoken.is_empty());
    assert!(spoken == engine.say(words));
    assert!(spoken == engine.say("+385 91/123-4567"));
    engine.set_comma_pause(0);
    let without = engine.say("+385911234567");
    engine.set_comma_pause(100);
    let with = engine.say("+385911234567");
    // Three group boundaries, each adding exactly 100 ms at 22050 Hz.
    assert_eq!(with.len(), without.len() + 3 * 2205);
    engine.set_number_mode(NumberMode::WholeNumbers);
    assert!(
        engine.say("+385911234567")
            == engine.say(
                "plus tristo osamdeset pet, devedeset jedan, sto dvadeset tri, četiri tisuće petsto šezdeset sedam"
            )
    );
    // The plus sign taken by a screen reader (NVDA at the punctuation level
    // "none" sends a space for it, at the higher levels the word)
    let bare = engine.say(" 385 91 123 4567  ");
    assert!(
        bare == engine
            .say("tristo osamdeset pet, devedeset jedan, sto dvadeset tri, četiri tisuće petsto šezdeset sedam")
    );
    assert!(bare == engine.say(" 385911234567  "));
    assert!(engine.say("plus 385 91 1234567  ") == engine.say("+385911234567"));
}

/// The longest silence inside the audio: its first sample and the one after
/// its last.
fn longest_gap(audio: &[i16]) -> (usize, usize) {
    let floor = (peak(audio) / 200).max(1);
    let (mut best_begin, mut best_end, mut begin) = (0, 0, 0);
    let (mut silent, mut started) = (false, false);
    for (i, &sample) in audio.iter().enumerate() {
        let quiet = i32::from(sample).abs() <= floor;
        if !quiet {
            started = true;
        }
        if quiet && !silent {
            begin = i;
        }
        if !quiet && silent && started && begin > 0 && i - begin > best_end - best_begin {
            best_begin = begin;
            best_end = i;
        }
        silent = quiet;
    }
    (best_begin, best_end)
}

fn gap_ms(audio: &[i16]) -> f64 {
    let (begin, end) = longest_gap(audio);
    (end - begin) as f64 * 1000.0 / 22050.0
}

/// Clock times, dates and cell addresses have short pauses
///
/// A listener's report of November 2026: a screen reader says a spreadsheet
/// cell's contents and then its address, and "Marjan A17" was heard as
/// "Marijana 17"; the hours of a clock time ran into the minutes, the day of a
/// date into the month. A short pause stands between them (the number
/// converter's short pause mark, rendered part by part): half of the comma
/// pause, less at a fast rate (the whole of it at first and 65 % next, which
/// the listener found too long both times), and the letters of the address
/// are letters.
#[test]
fn clock_times_dates_and_cell_addresses_have_short_pauses() {
    let mut engine = Speaker::new("zvonko");
    engine.set_sentence_pause(0);
    // The rows that fail are named together at the end, so that the sections
    // after a failing one still run.
    let mut failures = Failures::new();

    // A cell's address after the cell's contents
    {
        let cell = engine.say("Marjan A17");
        assert!(!cell.is_empty());
        // The pause and the letter: the marks typed into the words give the same samples
        assert!(cell == engine.say(&format!("Marjan{IBRK}A\u{FDD1} sedamnaest")));
        // NVDA ends every item with two spaces, and the driver joins them with a third
        assert!(cell == engine.say("Marjan   A17  "));
        // NVDA in Excel says the address as two words ('Marijan  ', 'A 3  ' in
        // the driver's log): the same letter and the same pause
        assert!(engine.say("Marijan   A 3  ") == engine.say("Marijan A3"));
        assert!(engine.say("Marjan   A 17  ") == cell);
        assert!(engine.say("A 5  ") == engine.say("A5"));
        // but not inside a sentence, where the letter is a word
        assert!(engine.say("U 5 sati dolazim") == engine.say("u pet sati dolazim"));
        // Before, the A was the conjunction a and leaned on the number
        assert!(cell != engine.say("Marjan A sedamnaest"));
        // The silence itself: the whole comma pause (half of it, 54 ms at a
        // screen reader's settings, was not heard as a pause through NVDA in
        // Excel; "Marijana 17" has a stop's closure of 56 ms)
        assert!(gap_ms(&cell) >= 100.0);
        info!("silence in Marjan A17 {} ms, in Marijana 17 {} ms", gap_ms(&cell), gap_ms(&engine.say("Marijana 17")));
        engine.set_comma_pause(300);
        assert!(gap_ms(&engine.say("Marjan A17")) >= 300.0);
        assert!(gap_ms(&engine.say("Marijana 17")) < 80.0);
        // The same with a consonant letter and with a number as the contents
        assert!(gap_ms(&engine.say("Ukupno B2")) >= 300.0);
        engine.set_comma_pause(100);
        assert!(engine.say("1250 C3") == engine.say(&format!("tisuću dvjesto pedeset{IBRK}C\u{FDD1} tri")));
        // The address alone, after a comma or after a preposition has no pause of its own
        assert!(engine.say("A17") == engine.say("A\u{FDD1} sedamnaest"));
        assert!(engine.say("u A17") == engine.say("u A\u{FDD1} sedamnaest"));
        // A designation at the head of a clause is not the preposition s
        assert!(engine.say("S22 je nov") != engine.say("S dvadeset dva je nov"));
    }

    // The pause is half the comma pause and shrinks with the rate
    {
        // 50 ms at the default 100, 150 ms at 300; before a cell's address
        // (`items`) the whole comma pause
        let samples = |comma_ms: u32, pauses: usize, items: usize| {
            pauses * (22050 * (comma_ms / 2) / 1000) as usize + items * (22050 * comma_ms / 1000) as usize
        };
        // (text, short pauses, pauses between items)
        let cases = [
            ("Marjan A17", 0, 1),
            ("12:30", 1, 0),
            ("12:30:45", 2, 0),
            ("10.11.2026.", 2, 0),
            ("10. 11. 2026.", 2, 0),
            ("Sastanak je u 12:30 u uredu.", 1, 0),
            ("A1 B2 C3", 0, 2),
            ("12:30 A17", 1, 1),
            ("Dobar dan svima", 0, 0),
            ("3.14", 0, 0),
        ];
        for (text, pauses, items) in cases {
            info!("{text}");
            engine.set_comma_pause(0);
            let without = engine.say(text);
            engine.set_comma_pause(100);
            let with = engine.say(text);
            assert_eq!(with.len(), without.len() + samples(100, pauses, items), "{text}");
            engine.set_comma_pause(300);
            assert_eq!(engine.say(text).len(), without.len() + samples(300, pauses, items), "{text}");
        }
        engine.set_comma_pause(100);
        // A date is the same written together and with spaces
        assert!(engine.say("10.11.2026.") == engine.say("10. 11. 2026."));
        // A comma between the same words pauses for longer
        assert!(engine.say("dvanaest, trideset").len() > engine.say("12:30").len());
        // Faster speech has a shorter pause, by the square root of the rate:
        // 50 ms at 1x, 35 ms at 2x, 25 ms at 4x; slower speech keeps the 50
        for (speed, pause_ms) in [(2.0f32, 35usize), (4.0, 25), (0.5, 50)] {
            info!("rate {speed}");
            engine.set_speed(speed);
            engine.set_comma_pause(0);
            let without = engine.say("u 12:30");
            let cell = engine.say("Marjan A17");
            engine.set_comma_pause(100);
            assert_eq!(engine.say("u 12:30").len(), without.len() + 22050 * pause_ms / 1000, "rate {speed}");
            // the pause before a cell's address is the comma pause at every rate
            assert_eq!(engine.say("Marjan A17").len(), cell.len() + 2205, "rate {speed}");
            // while the comma pause itself is the user's at every rate
            assert_eq!(
                engine.say("Marjan, Ana").len(),
                engine.say("Marjan,").len() + engine.say("Ana").len(),
                "rate {speed}"
            );
        }
        engine.set_speed(1.0);
    }

    // The melody goes on over the pause
    {
        // The end of a sentence falls to the bottom of the voice; before the
        // short pause the voice stays up, as before a comma, with less of a
        // comma's movement (measured for Zvonko in "Imam novi stan": the
        // sentence ends 6 semitones under the middle of the voice, the part
        // before the pause 1 under it, the clause before a comma on it)
        engine.set_comma_pause(300);
        let mut end_of_first_part = |text: &str| {
            let audio = engine.say(text);
            let (begin, end) = longest_gap(&audio);
            assert!(end - begin >= 3307, "{text}"); // 150 ms, the short pause at a comma pause of 300
            let end = begin as f64 / audio.len() as f64;
            pitch_hz_in(&audio, (end - 0.08).max(0.0), end)
        };
        let minor = end_of_first_part(&format!("Imam novi stan{BRK}i auto"));
        let comma = end_of_first_part("Imam novi stan, i auto");
        let statement = pitch_hz_in(&engine.say("Imam novi stan"), 0.88, 1.0);
        let measured = format!(
            "pitch at the end of a sentence {statement} Hz, before the pause {minor} Hz, before a comma {comma} Hz"
        );
        info!("{measured}");
        assert!(statement > 50.0, "{measured}");
        assert!(minor > statement * 1.19, "{measured}"); // three semitones
        assert!(minor <= comma * 1.03, "{measured}");
        engine.set_comma_pause(100);
        // and the time is not the two words with a comma between them
        assert!(engine.say("12:30") != engine.say("dvanaest, trideset"));
    }

    // Every language and both number modes
    {
        engine.set_number_mode(NumberMode::DigitByDigit);
        assert!(engine.say("Marjan A17") == engine.say(&format!("Marjan{IBRK}A\u{FDD1} jedan sedam")));
        assert!(engine.say("12:30") == engine.say(&format!("dvanaest{BRK}trideset")));
        engine.set_number_mode(NumberMode::WholeNumbers);
        engine.set_comma_pause(300);
        for language in ["sr", "bs", "sl"] {
            info!("{language}");
            assert!(engine.set_language(language));
            let cell = gap_ms(&engine.say("Marjan A17"));
            failures.check(cell >= 300.0, || format!("{language}: silence in Marjan A17 {cell} ms, under 300"));
            let time = gap_ms(&engine.say("12:30"));
            failures.check(time >= 150.0, || format!("{language}: silence in 12:30 {time} ms, under 150"));
            let same = engine.say("10.11.2026.") == engine.say("10. 11. 2026.");
            failures.check(same, || format!("{language}: 10.11.2026. is not 10. 11. 2026."));
        }
        engine.set_comma_pause(100);
        assert!(engine.set_language("sl"));
        // v is a preposition in Slovenian, [u] before a word; in an address it is the letter
        let same = engine.say("Marjan V5") == engine.say(&format!("Marjan{IBRK}V\u{FDD1} pet"));
        failures.check(same, || "sl: Marjan V5 is not Marjan, the letter V, pet".to_owned());
        let differs = engine.say("V5 je prazna") != engine.say("V pet je prazna");
        failures.check(differs, || "sl: V5 je prazna is V pet je prazna".to_owned());
        let same = engine.say("12:21") == engine.say(&format!("dvanajst{BRK}enaindvajset"));
        failures.check(same, || "sl: 12:21 is not dvanajst, enaindvajset".to_owned());
        assert!(engine.set_language("hr"));
    }

    // A singing preset keeps to its song
    {
        engine.set_voice("dalmacija_klapa").expect("dalmacija_klapa");
        assert!(engine.say("12:30") == engine.say("dvanaest trideset"));
        engine.set_voice("zvonko").expect("zvonko");
    }

    failures.finish();
}

// =============================================================================
// Prosody, balance, spelling, streaming, voices, singing presets, the user's accent lexicon
// =============================================================================

/// The length of audio in milliseconds.
fn ms(audio: &[i16]) -> f64 {
    audio.len() as f64 * 1000.0 / 22050.0
}

// =============================================================================
// Prosody
// =============================================================================

#[test]
fn punctuation_selects_intonation() {
    let mut zvonko = Speaker::new("zvonko");
    // No trailing pause, so the end of the audio is the last syllable.
    zvonko.set_sentence_pause(0);

    // A statement ends low, a yes/no question ends high.
    let statement = "Ona ima novu lampu.";
    let question = "Ona ima novu lampu?";
    let statement_end = pitch_hz_in(&zvonko.say(statement), 0.82, 1.0);
    let question_end = pitch_hz_in(&zvonko.say(question), 0.82, 1.0);
    assert!(statement_end > 50.0, "{statement_end}");
    assert!(question_end > statement_end * 1.25, "{question_end} against {statement_end}");

    // At inflection level 0 both stay on the same flat pitch.
    zvonko.set_inflection_level(0.0);
    let statement_end = pitch_hz_in(&zvonko.say(statement), 0.82, 1.0);
    let question_end = pitch_hz_in(&zvonko.say(question), 0.82, 1.0);
    assert!(close_to(question_end, statement_end, 0.05), "{question_end} against {statement_end}");
}

#[test]
fn a_comma_ends_between_a_statement_and_a_question() {
    // A comma gets a small fall after the last accent and a slight rise on
    // the last syllable (docs/formant.md, "The melody of read Croatian"):
    // it ends well above the same clause as a statement, but clearly below
    // it as a question, which a rise of four semitones from the level did
    // not.
    for voice in FORMANT_VOICES {
        let mut speaker = Speaker::new(voice);
        speaker.set_sentence_pause(0);
        speaker.set_comma_pause(0);
        for speed in [1.0f32, 2.0] {
            speaker.set_speed(speed);
            for text in ["Ona ima novu lampu", "Vlada je danas usvojila prijedlog novog zakona"] {
                let statement = final_pitch_hz(&speaker.say(&format!("{text}.")));
                let comma = final_pitch_hz(&speaker.say(&format!("{text},")));
                let question = final_pitch_hz(&speaker.say(&format!("{text}?")));
                let context =
                    format!("{voice} {speed} {text}: statement {statement}, comma {comma}, question {question}");
                info!("{context}");
                assert!(statement > 50.0, "{context}");
                assert!(comma > statement * 1.19, "{context}"); // three semitones
                assert!(comma * 1.12 < question, "{context}"); // two semitones
            }
        }
    }
}

/// A mark glued to a word is read by name, not as a clause end
#[test]
fn a_mark_glued_to_a_word_is_read_by_name_not_as_a_clause_end() {
    // synthesizer B's rule: a period, comma, colon or other mark ends the clause
    // only when whitespace, a bracket or quote, or the end of the text
    // follows. Inside a word or a number it is spoken (točka / tačka,
    // dvotočka, zarez); a comma or question mark inside a word is silent.
    // Synthesis is deterministic, so the audio must equal the spelled-out
    // text sample for sample.
    //
    // A row is two texts and whether they must sound the same. The sections
    // share one engine, in this order. \u{FDD0} between spaces is the short
    // pause inside a clock time or a date (`BRK`).
    type Rows<'a> = &'a [(&'a str, &'a str, bool)];
    let mut speaker = Speaker::new("zvonko");
    let mut failures = Failures::new();
    let mut check = |speaker: &mut Speaker, section: &str, rows: Rows| {
        for &(a, b, same) in rows {
            let equal = speaker.say(a) == speaker.say(b);
            failures.check(equal == same, || {
                format!("{section}: {a:?} {} {b:?}", if same { "does not sound like" } else { "sounds like" })
            });
        }
    };

    check(
        &mut speaker,
        "Period inside a word",
        &[
            ("datoteka.txt", "datoteka točka txt", true),
            ("www.index.hr", "www točka index točka hr", true),
            ("pjesma.mp3", "pjesma točka mp3", true),
            ("README.TXT", "README točka TXT", true),
            // Anywhere else a glued period ends the clause (a screen reader
            // glues the items of a label with it)
            ("Preslušano.Nestajuća poruka", "Preslušano. Nestajuća poruka", true),
            ("Preslušano.Nestajuća", "Preslušano točka Nestajuća", false),
            ("kraj.Novi", "kraj. Novi", true),
            ("1.Prvi", "jedan. Prvi", true),
            ("datoteka.tekstualna", "datoteka. tekstualna", true),
            (".txt", "točka txt", true),
            ("datoteka.txt", "datoteka txt", false),
        ],
    );
    check(
        &mut speaker,
        "Decimal point and comma",
        &[
            // After a period every group of digits is a whole number, as synthesizer B
            // reads it; after the decimal comma up to two digits are a number
            // and more are read one by one.
            ("3.14", "tri točka četrnaest", true),
            // A number glued to letters is set off from them, and the period
            // that joins a word to the letters before it is silent (a screen
            // reader's label "7.listopada.u20:03")
            ("u20:03", "u dvadeset \u{FDD0} nula tri", true),
            ("7.listopada.u20:03", "sedmog listopada u dvadeset \u{FDD0} nula tri", true),
            ("od20do30", "od dvadeset do trideset", true),
            ("5kg", "pet kg", true),
            ("COVID19", "COVID devetnaest", true),
            ("20:03h", "dvadeset \u{FDD0} nula tri h", true),
            ("3,14", "tri zarez četrnaest", true),
            ("0.05", "nula točka nula pet", true),
            // A period between groups of three digits is the thousands separator
            // (Hrvatski pravopis), since 2026-11-03; it used to be read by name
            ("1.317", "tisuću tristo sedamnaest", true),
            ("3.14159", "tri točka četrnaest tisuća sto pedeset devet", true),
            ("3,14159", "tri zarez jedan četiri jedan pet devet", true),
            ("1.000", "tisuću", true),
        ],
    );
    check(
        &mut speaker,
        "The commas of a phone number spelled digit by digit are pauses",
        &[
            // Screen readers write a phone number one digit at a time, with a
            // comma glued between the groups; a decimal comma stays one
            ("+ 1 2 3,5 6,7 8 9,8 7 6", "+ 1 2 3, 5 6, 7 8 9, 8 7 6", true),
            ("0 9 1,2 3 4,5 6 7 8", "nula devet jedan, dva tri četiri, pet šest sedam osam", true),
            (
                "+ 1 2 3,5 6,7 8 9,8 7 6",
                "plus jedan dva tri zarez pet šest zarez sedam osam devet zarez osam sedam šest",
                false,
            ),
            ("4,5 3,5 5,0", "četiri zarez pet tri zarez pet pet zarez nula", true),
            ("Ocjena 4,5.", "Ocjena četiri zarez pet.", true),
        ],
    );
    check(
        &mut speaker,
        "A period after a single letter is an abbreviation dot: silent, the letters by name",
        &[
            // (Letter names are full words, so "a" and "u" spelled from an
            // abbreviation differ from the typed conjunction and preposition;
            // the comparisons use letters whose names are not clitics.)
            ("p.s.", "pe es.", true),
            ("s.r.s.", "es er es.", true),
            ("b.b.", "be be.", true),
            ("s.a.r.s.", "s točka a točka er točka es.", false),
            ("U.S.A.", "u točka es točka a.", false),
        ],
    );
    check(
        &mut speaker,
        "Abbreviations are read as written, not expanded",
        &[
            // A word whose only syllable would be a final r is spelled
            ("dr. Ivić", "de er. Ivić", true),
            ("npr. ovo", "en pe er. ovo", true),
            ("mr. Ana", "em er. Ana", true),
            ("5 mm", "pet em em", true),
            ("dr. Ivić", "doktor. Ivić", false),
            ("5 km", "pet kilometara", false),
            // A syllabic r inside or at the head of a word is still a syllable
            ("rt", "er te", false),
            ("krv", "ka er ve", false),
        ],
    );
    // At the head of a sentence and in all-caps text a capital S or K
    // is the preposition, as a lowercase one is anywhere. The letter
    // is compared with its name, so in name mode (in sound mode it is
    // its sound: "A lone letter in running text follows the spelling mode")
    speaker.set_spelling_mode(SpellingMode::LetterNames);
    check(
        &mut speaker,
        "The prepositions s and k are words wherever a capital says nothing",
        &[
            (
                "S tobom sam htio sve, ali sam s njom dobio puno više.",
                "s tobom sam htio sve, ali sam s njom dobio puno više.",
                true,
            ),
            ("K meni.", "k meni.", true),
            ("(S tobom)", "(s tobom)", true),
            ("С тобом.", "s tobom.", true),
            ("ALI SAM S NJOM.", "ali sam s njom.", true),
            ("SPOJITE S USB KABELOM.", "spojite s USB kabelom.", true),
            ("S tobom.", "es tobom.", false),
            // Elsewhere a capital, a letter alone, before a hyphen, a single
            // letter or a word no preposition takes is the letter's name
            ("Mercedes S klasa.", "Mercedes es klasa.", true),
            ("Pritisnite S za spremanje.", "Pritisnite es za spremanje.", true),
            ("S je slovo.", "es je slovo.", true),
            ("slovo s je", "slovo es je", true),
            ("S-klasa", "es klasa", true),
            ("S i M.", "es i em.", true),
            ("S", "es", true),
            ("K", "ka", true),
            // A line starts a sentence of its own: the lines of a post as
            // VoiceOver reads it ("Javno" / "S ponosom predstavljam")
            ("prije 18 h, Javno\nS ponosom predstavljam.", "prije 18 h, Javno\ns ponosom predstavljam.", true),
            ("Javno\r\n\r\nK meni.", "Javno\r\n\r\nk meni.", true),
            // Before an instrumental a capital S is the preposition anywhere,
            // for a host that joins the lines with a space
            ("Javno S ponosom predstavljam.", "Javno s ponosom predstavljam.", true),
            ("Razgovarao sam S Ivanom.", "Razgovarao sam s Ivanom.", true),
            ("Došla je S radošću.", "Došla je s radošću.", true),
            ("Vozim Mercedes S klasu.", "Vozim Mercedes es klasu.", true),
            ("Mercedes S-klasom.", "Mercedes es klasom.", true),
        ],
    );

    // A line break ends the clause with the newline pause
    speaker.set_newline_pause(0);
    let short_pause = speaker.say("Prvi red\nDrugi red");
    speaker.set_newline_pause(500);
    let long_pause = speaker.say("Prvi red\nDrugi red");
    let added_ms = (long_pause.len() as f64 - short_pause.len() as f64) * 1000.0 / 22050.0;
    assert!(added_ms > 490.0, "{added_ms}");
    assert!(added_ms < 510.0, "{added_ms}");
    check(
        &mut speaker,
        "A line break ends the clause with the newline pause",
        &[
            // A blank line or CR LF is one break; after a full stop it adds nothing
            ("Prvi red\r\n\r\nDrugi red", "Prvi red\nDrugi red", true),
            ("Prvi red.\nDrugi red.", "Prvi red. Drugi red.", true),
            ("Prvi red\nDrugi red", "Prvi red Drugi red", false),
        ],
    );
    check(
        &mut speaker,
        "A number with a period before a lowercase word is an ordinal in the noun's case",
        &[
            // Since 2026-11-03 the ordinal takes the case the noun's ending and
            // the preposition show; the period is silent and does not end the
            // clause. Before an uppercase word it ends the sentence as before.
            ("7. listopada 2026.", "sedmog listopada dvije tisuće dvadeset šest.", true),
            ("u 19. stoljeću", "u devetnaestom stoljeću", true),
            ("1990. godine", "tisuću devetsto devedesete godine", true),
            ("2000. godine", "dvije tisućite godine", true),
            ("100. put", "stoti put", true),
            ("21. put", "dvadeset prvi put", true),
            ("7.listopada", "sedmog listopada", true),
            ("7. Listopad.", "sedam. Listopad.", true),
            ("Ima ih 7. Sutra", "Ima ih sedam. Sutra", true),
            ("0. element", "nula. element", true),
            ("12345. dan", "dvanaest tisuća tristo četrdeset pet. dan", true),
        ],
    );
    check(
        &mut speaker,
        "Dotted identifiers are read group by group",
        &[
            ("192.168.1.1", "sto devedeset dva točka sto šezdeset osam točka jedan točka jedan", true),
            ("2.0.1", "dva točka nula točka jedan", true),
        ],
    );
    speaker.set_number_mode(NumberMode::DigitByDigit);
    check(
        &mut speaker,
        "Digit mode keeps the separator",
        &[
            ("3.14", "tri točka jedan četiri", true),
            ("3,14", "tri zarez jedan četiri", true),
            ("12:30", "dvanaest \u{FDD0} trideset", true),
            ("0 9 1,2 3 4,5 6 7 8", "0 9 1, 2 3 4, 5 6 7 8", true),
            ("7.10.2026", "sedmog \u{FDD0} desetog \u{FDD0} dvije tisuće dvadeset šeste", true),
        ],
    );
    // The sections share one engine
    speaker.set_number_mode(NumberMode::WholeNumbers);
    check(
        &mut speaker,
        "Clock times and dates are read without the separators",
        &[
            // A date is read as it is said since 2026-11-03: the day and the
            // month as genitive ordinals, the year as the ordinal of "godine"
            ("12:30", "dvanaest \u{FDD0} trideset", true),
            ("9:05", "devet \u{FDD0} nula pet", true),
            ("12:30:45", "dvanaest \u{FDD0} trideset \u{FDD0} četrdeset pet", true),
            ("u 12:30 sati", "u dvanaest \u{FDD0} trideset sati", true),
            ("7.10.2026.", "sedmog \u{FDD0} desetog \u{FDD0} dvije tisuće dvadeset šeste.", true),
            ("7. 10. 2026.", "sedmog \u{FDD0} desetog \u{FDD0} dvije tisuće dvadeset šeste.", true),
            ("07.10.26", "sedmog \u{FDD0} desetog \u{FDD0} dvadeset šeste", true),
            ("31.12.1999", "trideset prvog \u{FDD0} dvanaestog \u{FDD0} tisuću devetsto devedeset devete", true),
            // Not a time or a date: the marks are read
            ("3:1", "tri dvotočka jedan", true),
            ("25:00", "dvadeset pet dvotočka nula nula", true),
            ("7.13.2026", "sedam točka trinaest točka dvije tisuće dvadeset šest", true),
        ],
    );
    check(
        &mut speaker,
        "Colon and exclamation mark inside a word, silent comma and question mark",
        &[
            ("a:b", "a dvotočka b", true),
            // The colon of a web address is silent, like its slashes
            ("https://www.index.hr", "https www.index.hr", true),
            ("http://example.com", "http example.com", true),
            ("https://www.index.hr", "https dvotočka www.index.hr", false),
            ("Hej!ti", "Hej uskličnik ti", true),
            ("a,b", "a b", true),
            ("a?b", "a b", true),
        ],
    );
    check(
        &mut speaker,
        "A mark before a space, bracket, quote or the end still ends the clause",
        &[
            ("Kraj. Novi", "Kraj Novi", false),
            ("Stvarno?!", "Stvarno?", true),
            ("Čekaj...", "Čekaj…", true),
            ("(Da.) Ne", "(Da. Ne", true),
            ("\"Da.\" Ne", "Da. Ne", true),
        ],
    );
    speaker.set_voice("stojan").expect("stojan");
    check(
        &mut speaker,
        "Serbian and Bosnian voices say tačka (stojan)",
        &[
            ("datoteka.txt", "datoteka tačka txt", true),
            ("3.14", "tri tačka četrnaest", true),
            ("a:b", "a dvotačka b", true),
            ("7.10.2026", "sedmog \u{FDD0} desetog \u{FDD0} dve hiljade dvadeset šeste", true),
            ("2000. godine", "dve hiljadite godine", true),
            ("p.s.", "pe es.", true),
        ],
    );
    speaker.set_voice("mirsad").expect("mirsad");
    check(&mut speaker, "Serbian and Bosnian voices say tačka (mirsad)", &[("Hej!ti", "Hej uzvičnik ti", true)]);
    failures.finish();
}

#[test]
fn short_questions_and_exclamations_are_audible() {
    // In a clause of a few words the end is all there is to hear the
    // punctuation by. Every question ends with a rise, also after a question
    // word ("Kako si ti?") and inside a single syllable ("Ti?").
    const CASES: [(&str, &str); 6] = [
        ("Kako si ti.", "Kako si ti?"),
        ("A ti.", "A ti?"),
        ("Ti.", "Ti?"),
        ("Zašto.", "Zašto?"),
        ("Tko je to.", "Tko je to?"),
        ("Jesi li dobro.", "Jesi li dobro?"),
    ];
    for voice in FORMANT_VOICES {
        let mut speaker = Speaker::new(voice);
        speaker.set_sentence_pause(0);
        for speed in [1.0f32, 2.0] {
            speaker.set_speed(speed);
            for (statement, question) in CASES {
                let statement_end = final_pitch_hz(&speaker.say(statement));
                let question_end = final_pitch_hz(&speaker.say(question));
                let context = format!("{voice} {speed} {question}: {question_end} against {statement_end}");
                info!("{context}");
                assert!(statement_end > 50.0, "{context}");
                assert!(question_end > statement_end * 1.19, "{context}"); // three semitones
            }
        }

        // A one-syllable exclamation starts higher than the statement.
        speaker.set_speed(1.0);
        let statement_peak = peak_pitch_hz_in(&speaker.say("Ne."), 0.0, 0.7);
        let exclamation_peak = peak_pitch_hz_in(&speaker.say("Ne!"), 0.0, 0.7);
        // measured 1.23 (150 against 122 Hz)
        assert!(exclamation_peak > statement_peak * 1.09, "{voice}: {exclamation_peak} against {statement_peak}");
    }
}

#[test]
fn short_sad_and_the_long_pronoun_ti_keep_their_lengths() {
    // The adverb sȁd was heard as long before a comma and in a question
    // (the 12% of a one-syllable word and the full final lengthening), and
    // the pronoun tȋ as short: read as the dative clitic after "a", "i" and
    // next to a verb of the second person, and at the head of a clause cut
    // to 99 ms as a function word, shorter than the short o of dobȁr.
    // Checked 2026-10-10.
    for voice in FORMANT_VOICES {
        let mut speaker = Speaker::new(voice);
        info!("{voice}");
        let mut longer = |a: &str, b: &str| ms(&speaker.say(a)) - ms(&speaker.say(b));
        assert!(longer("Dobro, sȃd,", "Dobro, sad,") >= 70.0, "{voice}");
        assert!(longer("Što sȃd?", "Što sad?") >= 60.0, "{voice}");
        // tȋ against the clitic mu in the same place; the clitic ti stays
        // as short as mu
        assert!(longer("Kako si ti?", "Kako si mu?") >= 75.0, "{voice}");
        assert!(longer("Što ti radiš?", "Što mu radiš?") >= 70.0, "{voice}");
        assert!(longer("Kažem ti.", "Kažem mu.") <= 45.0, "{voice}");
        assert!(longer("Ti si dobar.", "Tȉ si dobar.") >= 25.0, "{voice}");
        assert!(speaker.say("A ti?") == speaker.say("A tȋ?"), "{voice}");
        assert!(speaker.say("I mi?") == speaker.say("I mȋ?"), "{voice}");
    }
}

#[test]
fn a_word_of_one_vowel_is_a_syllable_of_its_own() {
    // The conjunctions i and a and the prepositions u and o used to be
    // shortened as clitics and once more next to a vowel, and were lost
    // between their neighbours ("ja i ona" as "jajona"). They now add a
    // full short vowel to the clause.
    for voice in FORMANT_VOICES {
        let mut speaker = Speaker::new(voice);
        info!("{voice}");
        assert!(ms(&speaker.say("Ja i ona.")) - ms(&speaker.say("Ja ona.")) >= 70.0, "{voice}");
        assert!(ms(&speaker.say("Idemo u Vukovar.")) - ms(&speaker.say("Idemo Vukovar.")) >= 65.0, "{voice}");
    }
}

#[test]
fn vowels_across_words_have_a_brief_dip_in_the_rendered_voice() {
    // Sustained /a/ on both sides exposes a missing boundary even when
    // the vowel colour does not change, as with the /u u/ in "moju ulicu".
    // Compare each short window with its neighbours, away from the onset
    // and final fade. This checks the PCM output, not just phone metadata.
    // At four times the rate the dip is about one glottal period wide, so
    // a 10 ms window measures its weakest period against the neighbours and
    // the value moves with where the pulses and the jitter happen to fall
    // (0.63-0.75 across the three voices in November 2026, 0.67-0.69 before
    // the lead frames grew by one); without a join the same measure reads
    // 1.0 there, so the limit at that rate is wide.
    fn dip(audio: &[i16]) -> f64 {
        const WINDOW: usize = 220; // 10 ms, covering at least one pitch period
        const OFFSET: usize = 550; // 25 ms to either side
        let threshold = peak(audio) / 30;
        let mut last = audio.len();
        while last > 0 && i32::from(audio[last - 1]).abs() <= threshold {
            last -= 1;
        }
        assert!(last > 2 * OFFSET + WINDOW);
        let mut lowest = 1.0f64;
        let mut i = OFFSET.max(last / 4);
        while i < (last * 3 / 4).min(last - OFFSET - WINDOW) {
            let neighbours = energy_at(audio, i - OFFSET, WINDOW).min(energy_at(audio, i + OFFSET, WINDOW));
            if neighbours > 0.0 {
                lowest = lowest.min((energy_at(audio, i, WINDOW) / neighbours).sqrt());
            }
            i += 44;
        }
        lowest
    }
    for voice in FORMANT_VOICES {
        let mut speaker = Speaker::new(voice);
        for speed in [1.0f32, 1.7, 4.0] {
            speaker.set_speed(speed);
            let (joined, held) = (dip(&speaker.say("ȃa ȁa")), dip(&speaker.say("ȃaaa")));
            info!("{voice} {speed}: {joined} across words, {held} inside one");
            assert!(joined < if speed < 4.0 { 0.65 } else { 0.80 }, "{voice} {speed}: {joined}");
            assert!(held > 0.70, "{voice} {speed}: {held}");
        }
    }
}

// =============================================================================
// Stops
// =============================================================================

#[test]
fn a_stop_after_a_fricative_keeps_its_silent_gap_at_high_rates() {
    // Without the gap "st", "sk", "št", "šk" are heard as the bare fricative.
    for voice in FORMANT_VOICES {
        let mut speaker = Speaker::new(voice);
        speaker.set_speed(2.0);

        for item in ["asta", "aska", "ašta", "aška"] {
            assert!(gaps(&speaker.say(item)).longest_ms >= 14.0, "{voice} {item}");
        }
        assert!(gaps(&speaker.say("asa")).longest_ms < 6.0, "{voice} asa");
    }
}

#[test]
fn the_speaking_voices_have_the_reference_balance() {
    // Measured on synthesizer A (synthesizer A, English, NVDA's defaults) in November
    // 2026 and matched: the low shelf on the voicing (lows against the
    // 450-1400 Hz mids +5.0 dB there; the voices had -0.3), the presence
    // region (2.5-5 kHz against the mids -10.5 there; -11.7 before, with a
    // different shape), and the 5.3 kHz ceiling (nothing above 6.3 kHz: -69
    // dB of the total there, -42 before). See "The reference balance" in
    // docs/formant.md; the Python of the same measures is testmetrics.py of
    // the round. Stojan and Mirsad share the tables and sit within 2 dB.
    // The ceiling is the voiced path's (the friction reaches 8 kHz since the
    // s was heard as a slight š), so the top is measured on a sentence of
    // vowels, nasals, l and j.
    let text = "Pozivam Marka na mobitel. Uključi zvučnik!";
    let sonorants = "Mama i njena mala Mila mole.";
    for voice in FORMANT_VOICES {
        let mut speaker = Speaker::new(voice);
        let spectrum = power_spectrum(&speaker.say(text));
        let mids = band_db(&spectrum, 450.0, 1400.0);
        let lows = band_db(&spectrum, 80.0, 300.0) - mids;
        let presence = band_db(&spectrum, 2500.0, 5000.0) - mids;
        let voiced = power_spectrum(&speaker.say(sonorants));
        let top = band_db(&voiced, 6300.0, 11025.0) - band_db(&voiced, 0.0, 11025.0);
        let context = format!("{voice}: lows {lows} presence {presence} top {top}");
        info!("{context}");
        assert!(lows > 2.0, "{context}"); // measured 4.6 / 5.1 / 3.6
        assert!(lows < 7.0, "{context}");
        assert!(presence > -16.0, "{context}"); // measured -12.4 / -13.6 / -13.3
        assert!(presence < -9.0, "{context}");
        assert!(top < -50.0, "{context}"); // measured -62
    }
}

#[test]
fn friction_sits_about_ten_decibels_under_the_voice() {
    // The friction frames of running speech (zero-crossing rate above 0.22)
    // against the voiced ones: synthesizer A -15.7 dB on an English paragraph,
    // the voices -8.9 before the round and -10.1 / -10.0 / -10.6 after it
    // (their sibilants are at synthesizer A's levels; what is left is Croatian's
    // share of fricatives and the quieter voiced frames of the voices).
    let text = "Sestra šalje pismo s fotografijom, hvala Tihomire. Čitaj šest stranica, zatim stani.";
    for voice in FORMANT_VOICES {
        let mut speaker = Speaker::new(voice);
        let d = friction_minus_voice_db(&speaker.say(text));
        let context = format!("{voice}: friction - voice {d} dB");
        info!("{context}");
        assert!(d > -13.0, "{context}");
        assert!(d < -7.0, "{context}");
    }
}

/// f, h, t, p, k and the sibilants sit at the reference levels
#[test]
fn f_h_t_p_k_and_the_sibilants_sit_at_the_reference_levels() {
    // Vowel-consonant-vowel items of Zvonko against the same items of
    // synthesizer A (the numbers in the comments), with the measures
    // above: the friction's median, the quietest 12 ms for h, the burst's
    // loudest 6 ms for the stops, all re the loudest frame of the item.
    let mut zvonko = Speaker::new("zvonko");
    const FRICTION: [(&str, f64, f64); 5] = [
        ("asa", -19.0, -13.0), // synthesizer A -17.9
        ("aša", -22.0, -16.0), // aša: -19.7
        ("aca", -20.5, -14.0), // c
        ("ača", -19.5, -13.0), // ača
        ("afa", -31.0, -25.0), // -29.9 (f is kept a little stronger for Croatian)
    ];
    for (text, lo, hi) in FRICTION {
        let level = vcv_friction_db(&zvonko.say(text));
        let context = format!("{text}: friction {level} dB");
        info!("{context}");
        assert!(level > lo, "{context}");
        assert!(level < hi, "{context}");
    }
    let h = vcv_h_db(&zvonko.say("aha"));
    info!("aha: h {h} dB");
    assert!(h > -25.0, "aha: h {h} dB"); // synthesizer A's h: 20-22 dB under the vowel
    assert!(h < -18.0, "aha: h {h} dB");
    const BURSTS: [(&str, f64, f64); 5] = [
        ("ata", -26.0, -19.0), // synthesizer A -22.2
        ("apa", -27.0, -20.0), // -22.7
        ("aka", -19.0, -12.0), // -11.7: the loudest before a
        ("aki", -22.0, -15.0), // -16.7
        ("aku", -24.0, -15.0), // -17.6
    ];
    for (text, lo, hi) in BURSTS {
        let level = vcv_burst_db(&zvonko.say(text));
        let context = format!("{text}: burst {level} dB");
        info!("{context}");
        assert!(!level.is_nan(), "{context}"); // a gap was found
        assert!(level > lo, "{context}");
        assert!(level < hi, "{context}");
    }
}

#[test]
fn the_singing_presets_keep_their_top() {
    // The 5.3 kHz ceiling and the low shelf are the speaking voices' alone:
    // a preset keeps the gentle 6 kHz low-pass on its voiced path, so its
    // vowels have a top the spoken ones have not (the presets' vowels were
    // checked byte by byte against the build before the round). The
    // friction has the same 8 kHz low-pass for every voice.
    let mut speaker = Speaker::new("zvonko");
    let spoken = power_spectrum(&speaker.say("mama"));
    speaker.set_voice("dalmacija_klapa").expect("dalmacija_klapa");
    let sung = power_spectrum(&speaker.say("mama"));
    let spoken_top = band_db(&spoken, 6300.0, 11025.0) - band_db(&spoken, 0.0, 11025.0);
    let sung_top = band_db(&sung, 6300.0, 11025.0) - band_db(&sung, 0.0, 11025.0);
    let context = format!("top above 6.3 kHz: spoken {spoken_top} sung {sung_top}");
    info!("{context}");
    assert!(sung_top > spoken_top + 5.0, "{context}"); // measured -80 against -89 dB of the total
}

/// s and c are not heard as š and č
#[test]
fn s_and_c_are_not_heard_as_sh_and_ch() {
    // Matched to the centroids of a 10-11 kHz synthesizer under the 5.3 kHz
    // ceiling, s, z and c had four fifths of their friction between 3.8 and
    // 5.0 kHz (the middle at 4.5 kHz, 15 % under 4 kHz, nothing above 6),
    // the band the ear is most sensitive in, and listeners heard a slight š,
    // ž and č (November 2026). The friction of a recorded Croatian s has its
    // middle at 5.2 kHz and reaches 8.2 (2 % under 4 kHz, 40 % above 6), a š
    // 3.0 and 5.3. Measured now: s 5.8 kHz, 1 % of it under 4 kHz and 42 %
    // above 6; š 3.3 kHz with 15 % above 4.5 kHz, as recorded.
    for voice in FORMANT_VOICES {
        let mut speaker = Speaker::new(voice);
        for item in ["asa", "aca"] {
            let friction = friction_spectrum(&speaker.say(item));
            let middle = friction_middle_hz(&friction);
            let low = friction_share(&friction, 2000.0, 4000.0);
            let high = friction_share(&friction, 6000.0, 11025.0);
            let context = format!("{voice} {item}: middle {middle} Hz, under 4 kHz {low}, above 6 kHz {high}");
            info!("{context}");
            assert!(middle > 5000.0, "{context}");
            assert!(middle < 6500.0, "{context}");
            assert!(low < 0.05, "{context}");
            assert!(high > 0.15, "{context}");
            assert!(high < 0.60, "{context}");
        }
        for item in ["aša", "ača"] {
            let friction = friction_spectrum(&speaker.say(item));
            let middle = friction_middle_hz(&friction);
            let context = format!("{voice} {item}: middle {middle} Hz");
            info!("{context}");
            assert!(middle > 2600.0, "{context}");
            assert!(middle < 3900.0, "{context}");
            // most of it under 4 kHz, and a tail above it instead of a wall
            assert!(friction_share(&friction, 2000.0, 4000.0) > 0.6, "{context}");
            assert!(friction_share(&friction, 4500.0, 11025.0) > 0.03, "{context}");
        }
    }
}

/// z and ž are not heard as v
#[test]
fn z_and_zh_are_not_heard_as_v() {
    // A listener's report of 2026-11-17: z and ž sounded more like v. Their
    // friction had been 0.40 and 0.50 of the friction of s and š since the
    // first version, and every later round took the shared friction down:
    // above 2 kHz z and ž lay 25.5 and 25.8 dB under the loudest frame and v
    // 26.2, under the same murmur. The recorded speaker's z and ž hiss as his
    // s and š do. Now z is 3 dB and ž 2 dB under the voiceless partner
    // (-19.7 and -21.0 dB) and 5 to 6 dB over v; s, š and v are the samples
    // they were.
    for voice in FORMANT_VOICES {
        let mut speaker = Speaker::new(voice);
        let mut hiss = |text: &str| vcv_hiss_db(&speaker.say(text));
        let (z, zh, v, s, sh) = (hiss("aza"), hiss("aža"), hiss("ava"), hiss("asa"), hiss("aša"));
        let context = format!("{voice}: hiss above 2 kHz z {z} ž {zh} v {v} s {s} š {sh} dB");
        info!("{context}");
        assert!(z > v + 3.5, "{context}");
        assert!(zh > v + 2.5, "{context}");
        // voiced: under the voiceless partner, and not far under
        assert!(z < s - 1.5 && z > s - 7.0, "{context}");
        assert!(zh < sh - 1.0 && zh > sh - 6.0, "{context}");
    }
}

#[test]
fn a_voiceless_stop_before_a_pause_is_released_audibly() {
    // The release is all that is heard of a final stop: it has to outlast
    // the bare burst (about 16 ms).
    for voice in FORMANT_VOICES {
        let mut speaker = Speaker::new(voice);
        for word in ["pat", "pak", "most"] {
            assert!(gaps(&speaker.say(word)).after_last_ms >= 30.0, "{voice} {word}");
        }
    }
}

// =============================================================================
// Spelling
// =============================================================================

#[test]
fn spelling_mode_works_with_formant_voices() {
    let mut zvonko = Speaker::new("zvonko");
    let (audio, _) = zvonko.synthesize_spelled("abc", true).expect("spelled audio");
    assert!(!audio.is_empty());
}

#[test]
fn letters_are_spelled_by_their_names_in_the_language_of_the_voice() {
    let mut speaker = Speaker::new("zvonko");
    speaker.set_spelling_mode(SpellingMode::LetterNames);
    speaker.set_spelling_speed(100);
    speaker.set_spelling_pause(0);

    // A spelled letter is its name spoken: "b" is "be", "j" is "je" (not "jot"),
    // "h" is "ha", and an uppercase letter the same as a lowercase one
    assert!(speaker.spell("b") == speaker.say("be"));
    assert!(speaker.spell("j") == speaker.say("je"));
    assert!(speaker.spell("h") == speaker.say("ha"));
    assert!(speaker.spell("B") == speaker.spell("b"));
    assert!(speaker.spell("š") == speaker.say("eš")); // š: eš
    // Y is ȉpsilōn (HJP), not ipsìlon as the rule for loans in -on had it
    assert!(speaker.spell("Y") == speaker.say("ȉpsilon"));
    assert!(speaker.spell("y") != speaker.say("ipsìlon"));

    // Serbian: lje, nje, ša; Cyrillic letters the same as Latin ones
    speaker.set_voice("stojan").expect("stojan");
    assert!(speaker.spell("š") == speaker.say("ša")); // š: ša
    assert!(speaker.spell("љ") == speaker.say("lje"));
    assert!(speaker.spell("б") == speaker.spell("b"));
    assert!(speaker.spell("Ј") == speaker.say("je")); // the Cyrillic Ј
    assert!(speaker.spell("y") == speaker.say("ȉpsilōn"));

    // Bosnian follows the Croatian names
    speaker.set_voice("mirsad").expect("mirsad");
    assert!(speaker.spell("š") == speaker.say("eš"));
    assert!(speaker.spell("l") == speaker.say("el"));
}

#[test]
fn letters_are_spelled_by_their_names_in_the_chosen_language() {
    // The letter names follow the language set, not the voice: Zvonko
    // speaking Serbian says ša and lje, Stojan speaking Croatian eš and el.
    let mut speaker = Speaker::new("zvonko");
    speaker.set_spelling_mode(SpellingMode::LetterNames);
    speaker.set_spelling_speed(100);
    speaker.set_spelling_pause(0);
    assert!(speaker.spell("š") == speaker.say("eš"));

    assert!(speaker.set_language("sr"));
    assert!(speaker.spell("š") == speaker.say("ša")); // š: ša
    assert!(speaker.spell("љ") == speaker.say("lje"));
    assert!(speaker.spell("ǌ") == speaker.say("nje")); // the ligature ǌ
    assert!(speaker.spell("b") == speaker.say("be"));

    // Back to the voice's own language
    assert!(speaker.set_language(""));
    assert!(speaker.spell("š") == speaker.say("eš"));

    // Stojan speaking Croatian or Bosnian uses the Croatian names
    speaker.set_voice("stojan").expect("stojan");
    assert!(speaker.spell("š") == speaker.say("ša"));
    assert!(speaker.set_language("hr"));
    assert!(speaker.spell("š") == speaker.say("eš"));
    assert!(speaker.spell("l") == speaker.say("el"));
    assert!(speaker.set_language("bs"));
    assert!(speaker.spell("š") == speaker.say("eš"));
    // The language survives the voice change
    speaker.set_voice("mirsad").expect("mirsad");
    assert!(speaker.set_language("sr"));
    speaker.set_voice("zvonko").expect("zvonko");
    assert!(speaker.spell("š") == speaker.say("ša"));
}

#[test]
fn letters_are_spelled_by_their_sounds_when_asked() {
    let mut speaker = Speaker::new("zvonko");
    assert_eq!(speaker.params().spelling_mode, SpellingMode::LetterSounds); // the default
    speaker.set_spelling_mode(SpellingMode::LetterNames);
    speaker.set_spelling_pause(0);

    let name = speaker.spell("f");
    speaker.set_spelling_mode(SpellingMode::LetterSounds);
    assert_eq!(speaker.params().spelling_mode, SpellingMode::LetterSounds);
    let sound = speaker.spell("f");

    // The sound is not the name, and it is audible
    assert!(sound != name);
    assert!(!sound.is_empty());
    assert!(rms(&sound) > 300.0);

    // Every letter of the alphabet, the digraph ligatures and the Cyrillic
    // letters have a sound
    const LETTERS: [&str; 37] = [
        "a", "b", "c", "č", "ć", "d", "đ", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "r", "s", "š",
        "t", "u", "v", "z", "ž", "ǉ", "ǌ", "ǆ", "б", "љ", "џ", "ћ", "Ђ", "Z", "Š",
    ];
    for letter in LETTERS {
        let audio = speaker.spell(letter);
        assert!(audio.len() > 22050 / 20, "{letter}"); // at least 50 ms
        assert!(rms(&audio) > 150.0, "{letter}");
    }

    // Digits, punctuation and symbols keep their names, and so do the
    // foreign letters (ku, iks, ipsilon) in every language
    for voice in ["zvonko", "stojan", "mirsad"] {
        speaker.set_voice(voice).expect(voice);
        for character in ["7", "q", "x", "y", "Q", "X", "Y"] {
            speaker.set_spelling_mode(SpellingMode::LetterNames);
            let by_name = speaker.spell(character);
            speaker.set_spelling_mode(SpellingMode::LetterSounds);
            assert!(speaker.spell(character) == by_name, "{voice} {character}");
        }
    }
    speaker.set_voice("zvonko").expect("zvonko");

    // w: duplo ve, dvojni ve by names, and by sounds the v is a sound (duplo v,
    // dvojni v), spelled as in running text (a listener's report)
    let mut failures = Failures::new();
    speaker.set_spelling_speed(100);
    for language in ["hr", "sr", "bs", "sl"] {
        assert!(speaker.set_language(language));
        speaker.set_spelling_mode(SpellingMode::LetterNames);
        let by_name = speaker.spell("w");
        failures.check(speaker.spell("W") == by_name, || format!("{language}: W is not w by names"));
        failures.check(by_name == speaker.say("w"), || format!("{language}: w by names is not w in running text"));
        speaker.set_spelling_mode(SpellingMode::LetterSounds);
        let by_sound = speaker.spell("w");
        failures.check(by_sound != by_name, || format!("{language}: w by sounds is w by names"));
        failures.check(by_sound == speaker.say("w"), || format!("{language}: w by sounds is not w in running text"));
    }
    assert!(speaker.set_language("sl"));
    failures.check(speaker.spell("w") == speaker.say("dvojni v"), || "sl: w is not dvojni v".to_owned());
    assert!(speaker.set_language("hr"));
    failures.check(speaker.spell("w") == speaker.say("duplo v"), || "hr: w is not duplo v".to_owned());

    // A spelled or typed 2 in Slovenian is the name of the number
    assert!(speaker.set_language("sl"));
    failures.check(speaker.spell("2") == speaker.say("dve"), || "sl: 2 is not dve".to_owned());
    assert!(speaker.set_language("hr"));

    // Spelling a word gives the sounds with the spelling pause between them
    speaker.set_spelling_pause(100);
    let word = speaker.spell("sat");
    failures.check(word.len() > speaker.spell("s").len() + speaker.spell("a").len(), || {
        "hr: sat is not longer than s and a".to_owned()
    });
    failures.finish();
}

#[test]
fn spelling_speed_adds_silence_without_slowing_formant_speech() {
    for voice in FORMANT_VOICES {
        info!("{voice}");
        check_spelling_pauses(&mut Speaker::new(voice));
    }
}

#[test]
fn spelling_speed_defaults_and_limits() {
    let mut speaker = Speaker::new("zvonko");
    speaker.set_spelling_pause(0);
    assert_eq!(speaker.params().spelling_speed, 50);

    // Out-of-range values are clamped
    speaker.set_spelling_speed(250);
    assert_eq!(speaker.params().spelling_speed, 100);
    speaker.set_spelling_speed(-5);
    assert_eq!(speaker.params().spelling_speed, 0);
}

/// The user's spelling entries win, except a letter name in sound mode
#[test]
fn the_users_spelling_entries_win_except_a_letter_name_in_sound_mode() {
    const BUNDLED: &str = r#"{ "entries": [ { "character": "*", "pronunciation": "zvjezdica" } ] }"#;
    let mut speaker = Speaker::new("zvonko");
    // The first block checks the names; sounds are the default since 2026-10-18
    speaker.set_spelling_mode(SpellingMode::LetterNames);
    speaker.set_spelling_speed(100);
    speaker.set_spelling_pause(0);

    // The bundled dictionary first, the user's entries on top of it
    assert!(speaker.load_spelling_dictionary(BUNDLED));
    speaker.add_spelling_entry("b", "bum");
    speaker.add_spelling_entry("j", "Jot");
    speaker.add_spelling_entry("*", "zvijezda");

    // Letter names: the user's entries are spoken as written
    assert!(speaker.spell("b") == speaker.say("bum"));
    assert!(speaker.spell("j") == speaker.say("Jot"));
    assert!(speaker.spell("*") == speaker.say("zvijezda"));
    assert!(speaker.spell("c") == speaker.say("ce"));

    // Letter sounds: "bum" is not a letter's name and still wins, "Jot" is
    // one and gives way to the sound; a symbol keeps the user's entry
    speaker.set_spelling_mode(SpellingMode::LetterSounds);
    assert!(speaker.spell("b") == speaker.say("bum"));
    assert!(speaker.spell("j") != speaker.say("Jot"));
    assert!(speaker.spell("j") != speaker.say("je"));
    assert!(speaker.spell("*") == speaker.say("zvijezda"));

    // A foreign letter is named in both modes, by the user's entry if any
    speaker.add_spelling_entry("w", "dvostruko ve");
    assert!(speaker.spell("w") == speaker.say("dvostruko ve"));
    assert!(speaker.spell("x") == speaker.say("iks"));
    speaker.set_spelling_mode(SpellingMode::LetterNames);
    assert!(speaker.spell("w") == speaker.say("dvostruko ve"));

    // An entry that merely names w gives way to the sound, as for any letter
    speaker.add_spelling_entry("w", "duplo ve");
    assert!(speaker.spell("w") == speaker.say("duplo ve"));
    speaker.set_spelling_mode(SpellingMode::LetterSounds);
    assert!(speaker.spell("w") == speaker.say("duplo v"));
    speaker.set_spelling_mode(SpellingMode::LetterNames);

    // Loading the bundled dictionary again drops the user's entries
    assert!(speaker.load_spelling_dictionary(BUNDLED));
    assert!(speaker.spell("*") == speaker.say("zvjezdica"));
}

#[test]
fn a_capital_letter_announced_by_a_screen_reader_is_spelled() {
    // NVDA "veliko N", TalkBack "veliko slovo N" and "велико Н", VoiceOver
    // "veliko početno slovo N", in English "cap N" and "capital N": the
    // words are spoken, the letter is spelled in the spelling mode and at
    // the spelling speed like any spelled character
    fn joined(speaker: &mut Speaker, words: &str, letter: &str) -> Vec<i16> {
        let mut audio = speaker.say(words);
        audio.extend(speaker.spell(letter));
        audio
    }
    // voice, text, words, letter
    const CASES: [(&str, &str, &str, &str); 10] = [
        ("zvonko", "veliko N", "veliko", "N"),
        ("zvonko", "Veliko slovo N", "Veliko slovo", "N"),
        ("zvonko", "veliko početno slovo N", "veliko početno slovo", "N"),
        ("zvonko", "cap N", "cap", "N"),
        ("zvonko", "capital N", "capital", "N"),
        ("zvonko", "veliko Č", "veliko", "Č"),
        ("zvonko", "veliko N ", "veliko", "N"),
        ("stojan", "велико Н", "велико", "Н"),
        ("stojan", "veliko N", "veliko", "N"),
        ("mirsad", "Veliko N", "Veliko", "N"),
    ];
    let mut speaker = Speaker::empty();
    for mode in [SpellingMode::LetterSounds, SpellingMode::LetterNames] {
        speaker.set_spelling_mode(mode);
        for (voice, text, words, letter) in CASES {
            speaker.set_voice(voice).expect(voice);
            assert!(speaker.say(text) == joined(&mut speaker, words, letter), "{voice} {text:?} mode {mode:?}");
        }
    }

    // In sound mode that is the sound, not the name the text used to give
    speaker.set_voice("zvonko").expect("zvonko");
    speaker.set_spelling_mode(SpellingMode::LetterSounds);
    assert!(speaker.say("veliko N") != joined(&mut speaker, "veliko", "en"));
    assert!(speaker.say("veliko N") != speaker.say("veliko en"));

    // Running text with a letter in it is not an announcement; a lone
    // letter in it follows the spelling mode (see the next test), and text
    // without one does not depend on the mode
    for text in ["veliko more", "veliko NN", "veliko 7"] {
        speaker.set_spelling_mode(SpellingMode::LetterSounds);
        let by_sound = speaker.say(text);
        speaker.set_spelling_mode(SpellingMode::LetterNames);
        assert!(speaker.say(text) == by_sound, "{text}");
    }

    // The streaming API gives the same samples
    speaker.set_spelling_mode(SpellingMode::LetterSounds);
    let mut streamed = Vec::new();
    speaker
        .synthesize_streaming("veliko N", 46, |chunk| {
            streamed.extend_from_slice(chunk);
            std::ops::ControlFlow::Continue(())
        })
        .expect("streamed audio");
    assert!(streamed == speaker.say("veliko N"));
}

#[test]
fn a_lone_letter_in_running_text_follows_the_spelling_mode() {
    // A screen reader reports a character inside a message ("Obrisano B",
    // "B obrisano", "Odabrano B", "veliko slovo B" split by the host): in
    // sound mode the letter is its sound, as when it is spelled, in name
    // mode its name. A preposition stays one, and the letters of an
    // abbreviation are named in both modes.
    fn same(speaker: &mut Speaker, a: &str, b: &str) -> bool {
        speaker.say(a) == speaker.say(b)
    }
    // voice, text, the text with the letter named
    const CASES: [(&str, &str, &str); 7] = [
        ("zvonko", "Obrisano B", "Obrisano be"),
        ("zvonko", "B obrisano", "be obrisano"),
        ("zvonko", "Plan B je bolji", "Plan be je bolji"),
        ("zvonko", "Obrisano s", "Obrisano es"),
        ("zvonko", "Obrisano Č", "Obrisano če"),
        ("stojan", "Обрисано Б", "Obrisano be"),
        ("mirsad", "Izbrisano m", "Izbrisano em"),
    ];
    let mut speaker = Speaker::empty();
    for (voice, text, named) in CASES {
        speaker.set_voice(voice).expect(voice);
        speaker.set_spelling_mode(SpellingMode::LetterNames);
        assert!(same(&mut speaker, text, named), "{voice} {text}");
        speaker.set_spelling_mode(SpellingMode::LetterSounds);
        assert!(!same(&mut speaker, text, named), "{voice} {text}");
    }
    // Cyrillic and Latin alike; a digraph alone is one letter in both modes
    speaker.set_voice("stojan").expect("stojan");
    assert!(same(&mut speaker, "Обрисано Б", "Obrisano B"));
    assert!(same(&mut speaker, "Обрисано Њ", "Obrisano Nj"));
    let nj_sound = speaker.say("Обрисано Њ");
    speaker.set_spelling_mode(SpellingMode::LetterNames);
    assert!(same(&mut speaker, "Обрисано Њ", "Obrisano Nj"));
    assert!(speaker.say("Обрисано Њ") != nj_sound);
    assert!(!same(&mut speaker, "Обрисано Њ", "Obrisano en je"));

    // Slovenian, its prepositions included
    let mut failures = Failures::new();
    speaker.set_voice("zvonko").expect("zvonko");
    assert!(speaker.set_language("sl"));
    speaker.set_spelling_mode(SpellingMode::LetterNames);
    let sl_named = speaker.say("Izbrisano v");
    speaker.set_spelling_mode(SpellingMode::LetterSounds);
    failures.check(speaker.say("Izbrisano v") != sl_named, || "sl: Izbrisano v is the same in both modes".to_owned());
    assert!(speaker.set_language(""));

    // Not a lone consonant letter: the same in both modes
    for text in
        ["s njom", "S tobom sam htio", "k meni", "USB kabel", "s.a.r.s.", "S-klasa", "x os", "Obrisano a", "veliko NN"]
    {
        speaker.set_spelling_mode(SpellingMode::LetterSounds);
        let by_sound = speaker.say(text);
        speaker.set_spelling_mode(SpellingMode::LetterNames);
        failures.check(speaker.say(text) == by_sound, || format!("{text:?} depends on the spelling mode"));
    }
    failures.finish();
}

/// Streaming API works with formant voices
/// A stop reaches a synthesis under way: by the callback of a streaming
/// synthesis, and by the engine's cancel handle from another thread, which is
/// how a screen reader's "stop speaking" arrives. The synthesis after it is
/// whole.
#[test]
fn a_synthesis_under_way_can_be_stopped_and_the_next_one_is_whole() {
    let mut speaker = Speaker::new("zvonko");
    let text = "Prva rečenica. Druga rečenica. Treća rečenica.";
    let whole = speaker.say(text);
    assert!(!whole.is_empty());

    // The callback stops it after the first clause
    let mut heard = Vec::new();
    let stopped = speaker.synthesize_streaming(text, 0, |chunk| {
        heard.extend_from_slice(chunk);
        ControlFlow::Break(())
    });
    assert!(matches!(stopped, Err(Error::Cancelled)));
    assert!(!heard.is_empty() && heard.len() < whole.len());
    assert!(whole.starts_with(&heard));

    // The handle stops it: nothing after the clause that was being handed over
    let cancel = speaker.cancel_handle();
    let mut clauses = 0;
    let stopped = speaker.synthesize_streaming(text, 0, |_| {
        clauses += 1;
        cancel.cancel();
        ControlFlow::Continue(())
    });
    assert!(matches!(stopped, Err(Error::Cancelled)));
    assert_eq!(clauses, 1);

    // A stop that came before a synthesis began stops nothing
    cancel.cancel();
    assert!(speaker.say(text) == whole);
    cancel.cancel();
    let spelled = speaker.synthesize_spelled("abc", true).expect("spelled audio").0;
    assert!(!spelled.is_empty());

    // From another thread, in the middle of a long text and of a spelled one
    let long = "Ovo je jedna duga rečenica bez kraja i konca koja se ponavlja ".repeat(300);
    for spell in [false, true] {
        let finished = Arc::new(AtomicBool::new(false));
        let stopper = {
            let (cancel, finished) = (cancel.clone(), finished.clone());
            std::thread::spawn(move || {
                while !finished.load(Ordering::Relaxed) {
                    cancel.cancel();
                    std::thread::sleep(std::time::Duration::from_millis(1));
                }
            })
        };
        let stopped =
            if spell { speaker.synthesize_spelled(&long, true).map(drop) } else { speaker.synthesize(&long).map(drop) };
        finished.store(true, Ordering::Relaxed);
        stopper.join().expect("the stopping thread");
        assert!(matches!(stopped, Err(Error::Cancelled)), "spelled: {spell}");
    }
    assert!(speaker.say(text) == whole);
}

#[test]
fn streaming_works_with_formant_voices() {
    let mut stojan = Speaker::new("stojan");
    let text = "Dobar dan. Kako ste?";
    let whole = stojan.say(text);
    assert!(!whole.is_empty());

    // Clause by clause, and in pieces of 46 ms (1014 samples; the C API's
    // stream is read in pieces of 1024)
    for chunk_ms in [0, 46] {
        let mut streamed = Vec::new();
        let mut longest = 0;
        stojan
            .synthesize_streaming(text, chunk_ms, |chunk| {
                longest = longest.max(chunk.len());
                streamed.extend_from_slice(chunk);
                ControlFlow::Continue(())
            })
            .expect("streamed audio");
        assert!(streamed == whole, "chunks of {chunk_ms} ms");
        if chunk_ms > 0 {
            assert!(longest <= 22050 * chunk_ms as usize / 1000, "chunks of {chunk_ms} ms: {longest} samples");
        }
    }
}

// =============================================================================
// Voice switching
// =============================================================================

#[test]
fn switches_between_formant_voices() {
    let mut speaker = Speaker::new("zvonko");
    let zvonko = speaker.say("Dobar dan");

    speaker.set_voice("mirsad").expect("mirsad");
    let mirsad = speaker.say("Dobar dan");
    assert!(zvonko != mirsad);

    speaker.set_voice("zvonko").expect("zvonko");
    assert!(speaker.say("Dobar dan") == zvonko);
}

#[test]
fn switches_between_formant_and_concatenative_voices() {
    let data_dir = data_dir();
    let mut speaker = Speaker::empty();
    speaker.0.set_voice("josip", &data_dir).expect("josip");
    let josip = speaker.say("Dobar dan");
    assert!(!josip.is_empty());

    speaker.0.set_voice("zvonko", &data_dir).expect("zvonko");
    let zvonko = speaker.say("Dobar dan");
    assert!(!zvonko.is_empty());
    assert!(zvonko != josip);

    // Back again: the recorded voice must be fully restored.
    speaker.0.set_voice("josip", &data_dir).expect("josip");
    assert!(speaker.say("Dobar dan") == josip);

    // A missing data file fails for recorded voices only.
    assert!(speaker.0.set_voice("vlado", Path::new(NO_DATA)).is_err());
}

#[test]
fn unknown_voice_is_rejected() {
    let mut speaker = Speaker::empty();
    assert!(speaker.set_voice("zvonimir").is_err());
    assert!(!speaker.is_initialized());
}

// =============================================================================
// Singing presets
// =============================================================================

/// id, base voice, language
const SINGING_PRESETS: [(&str, &str, &str); 10] = [
    ("orguljas", "zvonko", "hr-HR"),
    ("dalmacija_klapa", "zvonko", "hr-HR"),
    ("trubac", "stojan", "sr-RS"),
    ("harmonikas", "stojan", "sr-RS"),
    ("sevdalija", "mirsad", "bs-BA"),
    ("sazlija", "mirsad", "bs-BA"),
    ("lika_velebit", "zvonko", "hr-HR"),
    ("solunac", "stojan", "sr-RS"),
    ("sarajevo_bembasa", "mirsad", "bs-BA"),
    ("becarac", "zvonko", "hr-HR"),
];

/// Singing presets are formant voices of their base voice's language
#[test]
fn singing_presets_are_formant_voices_of_their_base_voices_language() {
    for (id, base, language) in SINGING_PRESETS {
        let preset = voice::find(id).unwrap_or_else(|| panic!("{id} is not registered"));
        assert_eq!(preset.language.code(), language, "{id}");
        assert_eq!(preset.base_voice_id, Some(base), "{id}");
        assert_eq!(preset.data_file(), None, "{id}");
        assert_eq!(preset.base_pitch, 1.0, "{id}");
        assert_eq!(Some(preset.nominal_wpm()), voice::find(base).map(|voice| voice.nominal_wpm()), "{id}");

        // No data directory needed, and the text is rendered.
        let sung = Speaker::new(id).say("Dobar dan, kako ste?");
        assert!(sung.len() > 22050, "{id}");
        assert!(peak(&sung) > 3000, "{id}");
    }
}

#[test]
fn singing_is_deterministic_and_starts_the_song_over_with_every_utterance() {
    let mut klapa = Speaker::new("dalmacija_klapa");
    let first = klapa.say("La la la la la la");
    let again = klapa.say("La la la la la la");
    assert!(first == again);

    // The clauses of one utterance continue the song: sung as two clauses
    // the second "la la la" sits on later notes than the first.
    let two_clauses = klapa.say("La la la, la la la");
    let one_clause = klapa.say("La la la");
    assert!(two_clauses.len() > one_clause.len());
}

#[test]
fn singing_presets_differ_from_each_other_and_from_the_speaking_voice() {
    let mut speaker = Speaker::new("zvonko");
    let spoken = speaker.say("Dobar dan");
    speaker.set_voice("orguljas").expect("orguljas");
    let organ = speaker.say("Dobar dan");
    speaker.set_voice("dalmacija_klapa").expect("dalmacija_klapa");
    let klapa = speaker.say("Dobar dan");

    // Notes last longer than spoken syllables.
    assert!(organ.len() > spoken.len());
    assert!(organ != klapa);

    // And back to speech.
    speaker.set_voice("zvonko").expect("zvonko");
    assert!(speaker.say("Dobar dan") == spoken);
}

#[test]
fn rate_sets_the_tempo_and_pitch_transposes_the_song() {
    let mut accordion = Speaker::new("harmonikas");
    let normal = accordion.say("Ma ma ma ma");
    accordion.set_speed(2.0);
    let fast = accordion.say("Ma ma ma ma");
    assert!((fast.len() as f64) < normal.len() as f64 * 0.65);
    assert!(fast.len() as f64 > normal.len() as f64 * 0.4);
    accordion.set_speed(1.0);

    // The accordion has no vibrato, so a held note's pitch is steady and
    // the user pitch moves it by an octave exactly.
    let high = accordion.say("Ma");
    accordion.set_user_pitch(0.5);
    let low = accordion.say("Ma");
    // (The autocorrelation helper is confused by the two beating reeds, so
    // the octave itself is checked with tools/formant/pitch.py, not here.)
    assert_eq!(low.len(), high.len());
    assert!(low != high);
}

#[test]
fn inflection_level_sets_the_vibrato_of_a_sung_voice() {
    let mut singer = Speaker::new("sevdalija");
    let with = singer.say("Aaa");
    singer.set_inflection_level(0.0);
    let without = singer.say("Aaa");
    assert_eq!(with.len(), without.len());
    assert!(with != without);
}

/// Dry presets sing at the voice's own pitch, legato
#[test]
fn dry_presets_sing_at_the_voices_own_pitch_legato() {
    let mut singer = Speaker::new("lika_velebit");
    // Zvonko speaks around 112 Hz; his song must stay within an octave of it.
    let sung = singer.say("Oj ti vilo, vilo Velebita");
    let f = pitch_hz_in(&sung, 0.2, 0.8);
    assert!(f > 70.0, "{f}");
    assert!(f < 230.0, "{f}");
    // Syllables that follow each other are joined: no silent gap between
    // the vowels of "vi-lo vi-lo".
    let legato = singer.say("Ma ma ma ma");
    assert!(gaps(&legato).longest_ms < 30.0);
}

#[test]
fn singing_presets_spell_as_well() {
    let mut trumpet = Speaker::new("trubac");
    let (audio, _) = trumpet.synthesize_spelled("abc", true).expect("spelled audio");
    assert!(!audio.is_empty());
}

// =============================================================================
// User accent lexicon
// =============================================================================

/// The user's accent lexicon moves the stress of words and verbs
#[test]
fn the_users_accent_lexicon_moves_the_stress_of_words_and_verbs() {
    // Words the built-in lexicon does not know get the first syllable.
    let mut speaker = Speaker::new("zvonko");
    assert!(speaker.say("balkon") != speaker.say("balkón"));
    assert!(speaker.say("balzama") != speaker.say("balzáma"));
    assert!(speaker.say("zatrubim") != speaker.say("zatrùbim"));

    let json = concat!(
        "{ \"version\": \"1.0\", \"entries\": [\n",
        "  { \"word\": \"balk'o:n*\", \"comment\": \"balk\\u00f3n, balk\\u00f3na\" },\n",
        "  { \"word\": \"balz'a:m|a|u|om\" },\n",
        "  { \"word\": \"kami'o:n*\", \"language\": \"sr\" },\n",
        "  { \"verb\": \"zatr'ub=i\" },\n",
        "  { \"word\": \"kabina\" },\n",
        "  { \"verb\": \"xyz\" }\n",
        "] }"
    );
    assert!(speaker.load_accent_lexicon(json));
    let report = speaker.accent_lexicon_report().to_owned();
    info!("{report}");
    // Two stems and three paradigm forms, one verb; the two malformed
    // entries are reported.
    assert!(report.starts_with("5 words, 1 verbs; 2 rejected (first: kabina: no stress or length mark)"), "{report}");

    // A stem covers the inflected forms, a paradigm only the forms listed.
    assert!(speaker.say("balkon") == speaker.say("balkón"));
    assert!(speaker.say("balkona") == speaker.say("balkóna"));
    assert!(speaker.say("balzama") == speaker.say("balzáma"));
    assert!(speaker.say("balzamom") == speaker.say("balzámom"));
    assert!(speaker.say("balzam") != speaker.say("balzám"));
    // A verb stem reaches every form the built-in verbs do.
    assert!(speaker.say("zatrubiti") == speaker.say("zatrùbiti"));
    assert!(speaker.say("zatrubim") == speaker.say("zatrùbim"));
    assert!(speaker.say("zatrubio") == speaker.say("zatrùbio"));
    // An entry for Serbian only leaves Zvonko alone.
    assert!(speaker.say("kamion") != speaker.say("kamión"));
    // Built-in entries, suffix rules and accent marks in the text still apply.
    assert!(speaker.say("kontrola") == speaker.say("kontróla"));
    assert!(speaker.say("organizacija") == speaker.say("organizácija"));
    assert!(speaker.say("bàlkon") != speaker.say("balkón"));

    // The lexicon survives a voice change, and the Serbian voice gets its entry.
    speaker.set_voice("stojan").expect("stojan");
    assert!(speaker.say("balkon") == speaker.say("balkón"));
    assert!(speaker.say("kamion") == speaker.say("kamión"));
    speaker.set_voice("zvonko").expect("zvonko");
    assert!(speaker.say("balkon") == speaker.say("balkón"));

    // Clearing restores the defaults; so does a file with nothing usable.
    speaker.clear_accent_lexicon();
    assert!(speaker.say("balkon") != speaker.say("balkón"));
    assert!(speaker.accent_lexicon_report().is_empty());
    assert!(speaker.load_accent_lexicon(json));
    assert!(speaker.say("balkon") == speaker.say("balkón"));
    assert!(!speaker.load_accent_lexicon(r#"{ "entries": [ { "word": "" } ] }"#));
    assert!(speaker.say("balkon") != speaker.say("balkón"));
    assert!(!speaker.load_accent_lexicon("not json at all"));
    assert!(!speaker.load_accent_lexicon_file(Path::new("/nonexistent/accents.json")));
}

/// The user's accent lexicon is read from a file and reports bad entries
#[test]
fn the_users_accent_lexicon_is_read_from_a_file_and_reports_bad_entries() {
    let path = std::env::temp_dir().join(format!("laprdus_test_accents_{}.json", std::process::id()));
    std::fs::write(
        &path,
        concat!(
            "{ \"entries\": [ { \"word\": \"balk'o:n*\" }, { \"word\": \"bal'k'on\" }, ",
            "{ \"word\": \"balkon:\" }, { \"verb\": \"zam'ol=q\" }, ",
            "{ \"word\": \"x'y\", \"language\": \"de\" } ] }"
        ),
    )
    .expect("a file in the temporary directory");
    let mut mirsad = Speaker::new("mirsad");
    let loaded = mirsad.load_accent_lexicon_file(&path);
    let _ = std::fs::remove_file(&path);
    assert!(loaded);
    let report = mirsad.accent_lexicon_report().to_owned();
    info!("{report}");
    assert!(report.starts_with("1 words, 0 verbs; 4 rejected (first: bal'k'on: two stress marks)"), "{report}");
    assert!(mirsad.say("balkon") == mirsad.say("balkón"));
}
