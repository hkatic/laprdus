//! Recorded (concatenative) voice tests: Josip, Vlado and the voices derived
//! from them.
//!
//! Every test needs the packed recordings: `cargo xtask data` writes
//! `data/voices/Josip.bin` and `Vlado.bin`, and `LAPRDUS_DATA` names another
//! directory. Without them the tests fail and say so.
//!
//! The tests render a lot of audio; run them optimized:
//!
//! ```text
//! cargo test --release --test recorded
//! ```
//!
//! `LAPRDUS_TEST_INFO=1` prints the measured values behind the thresholds.

#[macro_use]
mod support;

use laprdus::params::{NumberMode, SpellingMode};
use support::*;

const RECORDED_VOICES: [&str; 5] = ["josip", "vlado", "detence", "baba", "djedo"];

/// Pitch (Hz) of a 30 ms stretch starting at a sample, by normalised
/// autocorrelation; 0 if it is not periodic enough.
fn pitch_at(audio: &[i16], start: usize) -> f64 {
    const FS: usize = 22050;
    const WINDOW: usize = 661;
    const LAG_MIN: usize = FS / 400;
    const LAG_MAX: usize = FS / 60;
    if start + WINDOW + LAG_MAX > audio.len() {
        return 0.0;
    }
    let correlation = |lag: usize| {
        let (mut num, mut e1, mut e2) = (0.0, 0.0, 0.0);
        for i in 0..WINDOW {
            let a = f64::from(audio[start + i]);
            let b = f64::from(audio[start + i + lag]);
            num += a * b;
            e1 += a * a;
            e2 += b * b;
        }
        num / (e1 * e2 + 1e-9).sqrt()
    };
    let mut best = 0.0;
    let mut best_lag = 0;
    for lag in LAG_MIN..=LAG_MAX {
        let r = correlation(lag);
        if r > best {
            best = r;
            best_lag = lag;
        }
    }
    if best < 0.7 || best_lag == 0 {
        return 0.0;
    }
    // Take the shortest lag that correlates nearly as well (octave errors)
    for div in [3, 2] {
        let lag = best_lag / div;
        if lag < LAG_MIN {
            continue;
        }
        if correlation(lag) >= 0.9 * best {
            return FS as f64 / lag as f64;
        }
    }
    FS as f64 / best_lag as f64
}

/// Median pitch over the audio (frames every 10 ms that are periodic)
fn median_pitch(audio: &[i16]) -> f64 {
    let mut values: Vec<f64> = (0..)
        .map(|k| k * 220)
        .take_while(|start| start + 1200 < audio.len())
        .map(|start| pitch_at(audio, start))
        .filter(|&p| p > 0.0)
        .collect();
    if values.is_empty() {
        return 0.0;
    }
    values.sort_by(f64::total_cmp);
    values[values.len() / 2]
}

/// Median pitch of the last audible 120 ms
fn final_pitch(audio: &[i16]) -> f64 {
    let floor = (peak(audio) / 50).max(1);
    let mut end = audio.len();
    while end > 0 && i32::from(audio[end - 1]).abs() < floor {
        end -= 1;
    }
    if end < 3000 {
        return 0.0;
    }
    median_pitch(&audio[end - 2900..end])
}

/// The median level of the vowel frames (12 ms, within 30 dB of the loudest,
/// their band under 3 kHz ten times the band above it), in dB.
fn vowel_level_db(x: &[i16]) -> f64 {
    const N: usize = 265;
    let (h, l) = (butterworth_3k(x, true), butterworth_3k(x, false));
    let energy = |frame: &[i16]| frame.iter().map(|&v| f64::from(v) * f64::from(v)).sum::<f64>();
    let band = |frame: &[f64]| frame.iter().map(|v| v * v).sum::<f64>();
    let top = x.chunks_exact(N).map(energy).fold(0.0, f64::max);
    let mut levels = Vec::new();
    for (k, frame) in x.chunks_exact(N).enumerate() {
        let e = energy(frame);
        let eh = band(&h[k * N..(k + 1) * N]);
        let el = band(&l[k * N..(k + 1) * N]);
        if e > top * 1e-3 && el > 10.0 * eh {
            levels.push(10.0 * (e / N as f64 + 1e-9).log10());
        }
    }
    levels.sort_by(f64::total_cmp);
    if levels.is_empty() { -200.0 } else { levels[levels.len() / 2] }
}

fn ms(audio: &[i16]) -> f64 {
    audio.len() as f64 * 1000.0 / 22050.0
}

fn dbfs(level: f64) -> f64 {
    20.0 * (level / 32768.0).log10()
}

#[test]
fn recorded_slovenian_spelling_uses_slovenian_names() {
    let mut josip = Speaker::new("josip");
    check_slovenian_spelling(&mut josip);
}

#[test]
fn recorded_voices_speak() {
    let mut engine = Speaker::empty();
    for voice in RECORDED_VOICES {
        engine.set_voice(voice).unwrap_or_else(|error| panic!("{voice}: {error}"));
        let audio = engine.say("Dobar dan, kako ste?");
        assert!(audio.len() > 22050 / 2, "{voice}");
        assert!(rms(&audio) > 500.0, "{voice}");
        assert!(peak(&audio) <= 32767, "{voice}");
    }
}

#[test]
fn recorded_voice_synthesis_is_deterministic() {
    let mut josip = Speaker::new("josip");
    let a = josip.say("Ponovljena rečenica.");
    let b = josip.say("Ponovljena rečenica.");
    assert!(a == b);
}

#[test]
fn the_output_starts_and_ends_quietly() {
    let mut josip = Speaker::new("josip");
    let audio = josip.say("ao");
    assert!(audio.len() > 1000);
    let limit = peak(&audio) / 10;
    for i in 0..22 {
        assert!(i32::from(audio[i]).abs() < limit, "sample {i}");
        assert!(i32::from(audio[audio.len() - 1 - i]).abs() < limit, "sample {i} from the end");
    }
}

#[test]
fn rate_changes_the_duration_not_the_pitch() {
    let mut vlado = Speaker::new("vlado");
    let text = "Ovo je rečenica za mjerenje brzine govora.";
    vlado.set_speed(1.0);
    let normal = vlado.say(text);
    let f0_normal = median_pitch(&normal);
    vlado.set_speed(2.0);
    let fast = vlado.say(text);
    vlado.set_speed(0.5);
    let slow = vlado.say(text);

    // Pauses and the minimum durations of consonants do not scale fully
    let fast_ratio = fast.len() as f64 / normal.len() as f64;
    let slow_ratio = slow.len() as f64 / normal.len() as f64;
    info!("fast {fast_ratio:.3}, slow {slow_ratio:.3}, pitch {f0_normal:.1} Hz");
    assert!(fast_ratio > 0.45);
    assert!(fast_ratio < 0.70);
    assert!(slow_ratio > 1.6);
    assert!(slow_ratio < 2.2);

    assert!(f0_normal > 70.0);
    assert!(close_to(median_pitch(&fast), f0_normal, 0.12));
    assert!(close_to(median_pitch(&slow), f0_normal, 0.12));
}

#[test]
fn a_word_of_one_vowel_is_a_syllable_of_its_own() {
    // i, a, u, o keep a full short vowel and are set off from a vowel
    // next to them by the word gap ("ja i ona" was "jajona").
    let mut engine = Speaker::empty();
    for voice in ["josip", "vlado"] {
        engine.set_voice(voice).unwrap_or_else(|error| panic!("{voice}: {error}"));
        assert!(ms(&engine.say("Ja i ona.")) - ms(&engine.say("Ja ona.")) >= 90.0, "{voice}");
        assert!(ms(&engine.say("Idemo u Vukovar.")) - ms(&engine.say("Idemo Vukovar.")) >= 85.0, "{voice}");
    }
}

#[test]
fn user_pitch_moves_the_pitch_and_keeps_the_duration() {
    let mut josip = Speaker::new("josip");
    josip.set_inflection_level(0.0);
    let text = "Mama ima lala.";
    let normal = josip.say(text);
    josip.set_user_pitch(1.5);
    let high = josip.say(text);
    josip.set_user_pitch(0.7);
    let low = josip.say(text);

    let f0 = median_pitch(&normal);
    info!("pitch {f0:.1} Hz, high {:.1}, low {:.1}", median_pitch(&high), median_pitch(&low));
    assert!(f0 > 100.0);
    assert!(f0 < 180.0);
    assert!(close_to(median_pitch(&high), f0 * 1.5, 0.10));
    assert!(close_to(median_pitch(&low), f0 * 0.7, 0.10));
    assert!(close_to(high.len() as f64, normal.len() as f64, 0.03));
    assert!(close_to(low.len() as f64, normal.len() as f64, 0.03));
}

#[test]
fn derived_voices_differ_from_their_base_voice_in_pitch() {
    let mut engine = Speaker::empty();
    engine.set_inflection_level(0.0);
    let text = "Mama ima lala.";
    let mut pitch_of = |voice: &str| {
        engine.set_voice(voice).unwrap_or_else(|error| panic!("{voice}: {error}"));
        median_pitch(&engine.say(text))
    };
    let josip = pitch_of("josip");
    let detence = pitch_of("detence");
    let vlado = pitch_of("vlado");
    let djedo = pitch_of("djedo");
    info!("josip {josip:.1} Hz, detence {detence:.1}, vlado {vlado:.1}, djedo {djedo:.1}");
    assert!(josip > 0.0);
    assert!(vlado > 0.0);
    assert!(close_to(detence, josip * 1.5, 0.12));
    assert!(close_to(djedo, vlado * 0.75, 0.12));
}

#[test]
fn a_question_ends_higher_than_a_statement() {
    let mut josip = Speaker::new("josip");
    let statement = final_pitch(&josip.say("Ona ima novi auto."));
    let question = final_pitch(&josip.say("Ona ima novi auto?"));
    info!("statement {statement:.1} Hz, question {question:.1} Hz");
    assert!(statement > 0.0);
    assert!(question > statement * 1.15);

    // At inflection level 0 both end at the voice's own pitch
    josip.set_inflection_level(0.0);
    let flat_statement = final_pitch(&josip.say("Ona ima novi auto."));
    let flat_question = final_pitch(&josip.say("Ona ima novi auto?"));
    assert!(close_to(flat_statement, flat_question, 0.05));
}

/// Streaming gives the same audio as one buffer. (The stream handles of the
/// C API are tested with the C API.)
#[test]
fn streaming_gives_the_same_audio_as_one_buffer() {
    let mut josip = Speaker::new("josip");
    let text = "Prva rečenica. Druga, s zarezom, rečenica!";
    let whole = josip.say(text);
    assert!(!whole.is_empty());

    // In chunks of about 1000 samples, and every clause whole
    for chunk_ms in [1000 * 1000 / 22050, 0] {
        let mut streamed = Vec::new();
        let mut chunks = 0;
        josip
            .synthesize_streaming(text, chunk_ms, |chunk| {
                assert!(!chunk.is_empty());
                streamed.extend_from_slice(chunk);
                chunks += 1;
            })
            .expect("streaming synthesis");
        assert!(chunks > 1, "chunks of {chunk_ms} ms");
        assert!(streamed == whole, "chunks of {chunk_ms} ms");
    }
}

#[test]
fn a_period_inside_a_word_or_a_number_is_read_by_name() {
    // The recorded voices share the front end, so "datoteka.txt" is spoken
    // as "datoteka točka txt" (tačka for Vlado) and "3.14" as a decimal.
    let mut engine = Speaker::new("josip");
    assert!(engine.say("datoteka.txt") == engine.say("datoteka točka txt"));
    assert!(engine.say("3.14") == engine.say("tri točka četrnaest"));
    assert!(engine.say("Kraj. Novi") != engine.say("Kraj Novi"));

    engine.set_voice("vlado").expect("vlado");
    assert!(engine.say("datoteka.txt") == engine.say("datoteka tačka txt"));
    assert!(engine.say("3,14") == engine.say("tri zarez četrnaest"));
}

#[test]
fn number_words_follow_the_chosen_language_for_recorded_voices() {
    // Josip and Vlado speak every language too: the chosen language picks
    // the number words (tisuću / hiljadu), stays across voice changes, and
    // clearing it returns to the voice's own.
    let mut engine = Speaker::new("josip");
    assert_eq!(engine.language_code(), "hr-HR");
    let croatian = engine.say("1000");
    assert!(!croatian.is_empty());
    assert!(croatian == engine.say("tisuću"));

    assert!(engine.set_language("sr"));
    assert_eq!(engine.language_code(), "sr-RS");
    assert_eq!(engine.voice_id(), "josip");
    assert!(engine.say("1000") == engine.say("hiljadu"));
    assert!(engine.say("2000") == engine.say("dve hiljade"));
    assert!(engine.say("1000") != croatian);

    assert!(engine.set_language("bs"));
    assert_eq!(engine.language_code(), "bs-BA");
    assert!(engine.say("2000") == engine.say("dvije hiljade"));

    // The chosen language stays when the voice changes ...
    engine.set_voice("vlado").expect("vlado");
    assert_eq!(engine.language_code(), "bs-BA");
    // ... and Vlado speaking Croatian says tisuću
    assert!(engine.set_language("hr-HR"));
    assert_eq!(engine.language_code(), "hr-HR");
    let vlado_croatian = engine.say("1000");
    assert!(vlado_croatian == engine.say("tisuću"));
    assert!(vlado_croatian != engine.say("hiljadu"));

    // No language: back to Vlado's native Serbian, also after later voice changes
    engine.0.set_language(None);
    assert_eq!(engine.language_code(), "sr-RS");
    assert!(engine.say("1000") == engine.say("hiljadu"));
    engine.set_voice("detence").expect("detence");
    assert_eq!(engine.language_code(), "hr-HR");
    assert!(engine.say("1000") == engine.say("tisuću"));
}

#[test]
fn spelling_works_with_a_recorded_voice() {
    let mut vlado = Speaker::new("vlado");
    let audio = vlado.spell("ab");
    assert!(audio.len() > 2000);
    assert!(rms(&audio) > 300.0);
}

#[test]
fn spelling_speed_adds_silence_without_slowing_recorded_speech() {
    for voice in ["josip", "vlado"] {
        info!("{voice}");
        let mut engine = Speaker::new(voice);
        check_spelling_pauses(&mut engine);
    }
}

#[test]
fn letter_sounds_work_with_a_recorded_voice() {
    let mut engine = Speaker::new("josip");
    engine.set_spelling_pause(0);

    // Names at the speech rate are the names spoken (sounds are the default)
    engine.set_spelling_mode(SpellingMode::LetterNames);
    engine.set_spelling_speed(100);
    let name = engine.spell("b");
    assert!(name == engine.say("be"));
    assert!(engine.spell("j") == engine.say("je"));

    // Sounds: every letter gives audible sound, different from its name
    engine.set_spelling_mode(SpellingMode::LetterSounds);
    engine.set_spelling_speed(100);
    assert!(engine.spell("b") != name);
    for letter in [
        "a", "b", "c", "č", "ć", "d", "đ", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "r", "s", "š",
        "t", "u", "v", "z", "ž", "\u{01C9}", "џ",
    ] {
        let audio = engine.spell(letter);
        assert!(audio.len() > 22050 / 25, "{letter}");
        assert!(rms(&audio) > 150.0, "{letter}");
    }

    // The foreign letters keep their names (iks, ipsilon)
    assert!(engine.spell("x") == engine.say("iks"));
    assert!(engine.spell("Y") == engine.say("ipsilon"));

    // A screen reader's "veliko N" spells the letter by its sound too
    let mut capital = engine.say("veliko");
    capital.extend(engine.spell("N"));
    assert!(engine.say("veliko N") == capital);
    assert!(engine.say("veliko N") != engine.say("veliko en"));

    // A letter alone in running text ("Obrisano B") is its sound too, an
    // audible syllable; a preposition stays one
    let deleted = engine.say("Obrisano B");
    assert!(deleted != engine.say("Obrisano be"));
    assert!(deleted.len() > engine.say("Obrisano").len() + 22050 / 10);
    let with = engine.say("s njom");

    // In name mode it is the name
    engine.set_spelling_mode(SpellingMode::LetterNames);
    assert!(engine.say("Obrisano B") == engine.say("Obrisano be"));
    assert!(engine.say("s njom") == with);

    // Vlado spells with Serbian names
    engine.set_voice("vlado").expect("vlado");
    assert!(engine.spell("š") == engine.say("ša"));
}

#[test]
fn head_size_and_brightness_shape_the_recorded_voices() {
    // Head size lengthens or shortens the vocal tract through the bank warp,
    // so the formants move and the pitch stays; brightness tilts the top
    // around 1.5 kHz. 50 is the recording as it is.
    const TEXT: &str = "Pozivam Marka na mobitel.";
    let mut josip = Speaker::new("josip");
    assert_eq!(josip.params().head_size, 50);
    assert_eq!(josip.params().brightness, 50);
    let plain = josip.say(TEXT);

    // The share of the energy above about 1.5 kHz (a one-pole high-pass).
    // The thresholds were set with this value of pi.
    #[allow(clippy::approx_constant)]
    let top_share = |audio: &[i16]| {
        let (mut low, mut top, mut total) = (0.0, 0.0, 0.0);
        let coef = (-2.0 * 3.14159265 * 1500.0 / 22050.0f64).exp();
        for &s in audio {
            let s = f64::from(s);
            low = coef * low + (1.0 - coef) * s;
            let hp = s - low;
            top += hp * hp;
            total += s * s;
        }
        if total > 0.0 { top / total } else { 0.0 }
    };

    josip.set_head_size(100);
    let big = josip.say(TEXT);
    // The bank is resampled, so the length moves by a few periods' worth;
    // the pitch does not (the formants do: see "Voice quality settings" in
    // docs/formant.md for the measured spectra).
    assert!((big.len() as f64 - plain.len() as f64).abs() < 0.02 * plain.len() as f64);
    assert!(big != plain);
    let plain_pitch = median_pitch(&plain);
    info!("pitch: plain {plain_pitch:.1} Hz, head size 100 {:.1} Hz", median_pitch(&big));
    assert!((median_pitch(&big) - plain_pitch).abs() < 0.03 * plain_pitch);

    josip.set_head_size(50);
    assert!(josip.say(TEXT) == plain);

    josip.set_brightness(0);
    let dull = josip.say(TEXT);
    assert_eq!(dull.len(), plain.len());
    info!("top share: plain {:.4}, brightness 0 {:.4}", top_share(&plain), top_share(&dull));
    assert!(top_share(&dull) < top_share(&plain) * 0.7);
    josip.set_brightness(100);
    assert!(top_share(&josip.say(TEXT)) > top_share(&plain));
    josip.set_brightness(50);
    assert!(josip.say(TEXT) == plain);
}

#[test]
fn volume_scales_the_output() {
    let mut josip = Speaker::new("josip");
    let loud = josip.say("Glasno i tiho.");
    josip.set_volume(0.25);
    let quiet = josip.say("Glasno i tiho.");
    assert_eq!(quiet.len(), loud.len());
    assert!(rms(&quiet) < rms(&loud) * 0.4);
}

#[test]
fn the_volume_boost_makes_the_recorded_voices_louder_without_clipping() {
    // The recordings already touch full scale and are compressed, so a
    // limiter that does not clip gains little here: the vowels +1.1 dB for
    // Josip (the boost of 2.0 gave +5 dB by rounding the peaks off, at 10 %
    // distortion). Since November 2026 the hiss limiter takes his s, as loud
    // as his vowels, 18 dB down, so the overall level is no measure of the
    // boost any more: the vowels still rise and nothing clips.
    const TEXT: &str = "Pozivam Marka na mobitel.";
    let mut engine = Speaker::empty();
    for voice in ["josip", "baba"] {
        engine.set_voice(voice).unwrap_or_else(|error| panic!("{voice}: {error}"));
        engine.set_volume_boost(false);
        let plain = engine.say(TEXT);
        engine.set_volume_boost(true);
        let boosted = engine.say(TEXT);
        assert_eq!(boosted.len(), plain.len(), "{voice}");
        info!("{voice} vowels: plain {} dB, boosted {} dB", vowel_level_db(&plain), vowel_level_db(&boosted));
        assert!(vowel_level_db(&boosted) > vowel_level_db(&plain) + 0.5, "{voice}");
        assert!(peak(&boosted) <= 32000, "{voice}");
        assert!(engine.say(TEXT) == boosted, "{voice}");
        // Nothing over full scale between the samples (Josip's boosted speech
        // reached +1.4 dBFS there, November 2026), and the recorded s, whose
        // hiss band is as loud as the vowel in the recording, sits about 18 dB
        // under it (the plain output holds it about 8 dB under).
        info!("{voice} true peak {} dBFS", dbfs(true_peak(&boosted)));
        assert!(true_peak(&boosted) < 32768.0 * 10f64.powf(-0.2 / 20.0), "{voice}");
        engine.set_volume_boost(false);
        let s_plain = engine.say("asa");
        engine.set_volume_boost(true);
        let s_boosted = engine.say("asa");
        let hiss = hiss_pair_db(&s_plain, &s_boosted);
        info!("{voice} hiss of asa re the vowel: plain {} dB, boosted {} dB", hiss.0, hiss.1);
        assert!(hiss.1 < hiss.0 - 5.0, "{voice}"); // the plain s is held too, 10 dB higher
        // The volume scales the boosted output
        engine.set_volume(0.5);
        let half = engine.say(TEXT);
        assert!(rms(&half) < rms(&boosted) * 0.6, "{voice}");
        engine.set_volume(1.0);
    }
}

#[test]
fn the_hiss_of_the_recorded_voices_is_held_under_the_vowels() {
    // In the recordings s, c, č and ć stand level with the vowels (Josip's s
    // and ć -0.8 and -0.6 dB re the loudest frame, Vlado's š and č -4.8 and
    // -2.3), and weighted by the ear's sensitivity above them, where the
    // softest of four commercial recorded Croatian voices has them 4-8 dB
    // under. The plain output holds the hiss RECORDED_HISS_DB under the
    // nearby vowel (November 2026), and the result is the same every time.
    let mut engine = Speaker::empty();
    for (voice, text) in
        [("josip", "asa"), ("josip", "aća"), ("josip", "aca"), ("baba", "asa"), ("vlado", "aša"), ("vlado", "ača")]
    {
        engine.set_voice(voice).unwrap_or_else(|error| panic!("{voice}: {error}"));
        let audio = engine.say(text);
        let hiss = hiss_pair_db(&audio, &audio).0;
        info!("{voice} {text}: hiss re the loudest frame {hiss} dB");
        assert!(hiss < -6.0, "{voice} {text}: {hiss} dB"); // measured -8.6 to -10.7
        assert!(hiss > -16.0, "{voice} {text}: {hiss} dB"); // held, not removed
        assert!(engine.say(text) == audio, "{voice} {text}");
    }
}

#[test]
fn the_recorded_voices_do_not_clip_at_full_volume() {
    // The recordings touch full scale, and at full volume Vlado and Đedo drove
    // the soft knee of the renderer so hard that above 5 kHz half of Đedo's
    // sound was its distortion (November 2026): the waveshaping residual of a
    // paragraph -31 dB for Đedo, -38 for Vlado, -45 for Josip. A limiter at
    // unity gain now holds the peaks under the knee after the volume
    // (-51, -55, -55 dB measured; the true peak -0.6 to -0.9 dBTP), and an
    // utterance at half volume is what it was.
    let text = "Dobar dan, kako ste? Danas je lijep dan, pa smo otišli u šetnju uz rijeku.";
    let mut engine = Speaker::empty();
    for voice in ["djedo", "vlado", "josip"] {
        engine.set_voice(voice).unwrap_or_else(|error| panic!("{voice}: {error}"));
        engine.set_volume(1.0);
        let full = engine.say(text);
        engine.set_volume(0.5);
        let half = engine.say(text);
        assert_eq!(full.len(), half.len(), "{voice}");
        let waveshaping = waveshaping_db(&full, &half);
        info!("{voice}: waveshaping {waveshaping} dB, true peak {} dBFS", dbfs(true_peak(&full)));
        assert!(waveshaping < -45.0, "{voice}: {waveshaping} dB");
        assert!(true_peak(&full) < 32768.0 * 10f64.powf(-0.2 / 20.0), "{voice}");
        assert!(peak(&full) <= (32767.0 * 0.9f32) as i32 + 1, "{voice}"); // under the knee
    }
    engine.set_volume(1.0);
}

#[test]
fn recorded_voices_share_surname_stress_and_vowel_length() {
    const CASES: [(&str, &str); 18] = [
        ("Isaković", "Ìsaković"),
        ("Isakovića", "Ìsakovića"),
        ("Isakovićevima", "Ìsakovićevima"),
        ("Исаковић", "Ìsaković"),
        ("Spasojević", "Spàsojević"),
        ("Spasojevićem", "Spàsojevićem"),
        ("Spasojevićevima", "Spàsojevićevima"),
        ("Спасојевић", "Spàsojević"),
        ("Isaković i Spasojević", "Ìsaković i Spàsojević"),
        ("Knezović", "Knȇzović"),
        ("Knezovića", "Knȇzovića"),
        ("Knezovićevima", "Knȇzovićevima"),
        ("Кнезовић", "Knȇzović"),
        ("Knežević", "Knéžević"),
        ("Kneževićem", "Knéževićem"),
        ("Kneževićevima", "Knéževićevima"),
        ("Кнежевић", "Knéžević"),
        ("Knezović i Knežević", "Knȇzović i Knéžević"),
    ];
    let mut engine = Speaker::empty();
    let mut failures = Failures::new();
    for voice in RECORDED_VOICES {
        engine.set_voice(voice).unwrap_or_else(|error| panic!("{voice}: {error}"));
        for (plain, accented) in CASES {
            let audio = engine.say(plain);
            failures.check(!audio.is_empty(), || format!("{voice}: no audio for {plain}"));
            failures.check(audio == engine.say(accented), || format!("{voice}: {plain} is not {accented}"));
        }
        // Initial stress must not return to a; changing only a's length
        // must make the output longer. The same check protects short o.
        let mut differs = |a: &str, b: &str| {
            failures.check(engine.say(a) != engine.say(b), || format!("{voice}: {a} sounds like {b}"));
        };
        differs("Isaković", "Isàković");
        differs("Spasojević", "Spasòjević");
        differs("Spasojević", "Spasójević");
        differs("Knezović", "Knézović");
        differs("Knežević", "Knȇžević");
        let mut shorter = |a: &str, b: &str| {
            failures
                .check(engine.say(a).len() < engine.say(b).len(), || format!("{voice}: {a} is not shorter than {b}"));
        };
        shorter("Isaković", "Ìsāković");
        shorter("Spasojević", "Spàsōjević");
        shorter("Knȅzović", "Knezović");
        shorter("Knèžević", "Knežević");
    }
    failures.finish();
}

#[test]
fn recorded_voices_pause_between_telephone_groups() {
    let mut josip = Speaker::new("josip");
    josip.set_number_mode(NumberMode::DigitByDigit);
    let spoken = josip.say("00385 91/123-4567");
    assert!(!spoken.is_empty());
    assert!(spoken == josip.say("nula nula tri osam pet, devet jedan, jedan dva tri, četiri pet šest sedam"));
    josip.set_comma_pause(0);
    let without = josip.say("00385911234567");
    josip.set_comma_pause(100);
    let with = josip.say("00385911234567");
    assert_eq!(with.len(), without.len() + 3 * 2205);
    // The plus sign taken by a screen reader: the same groups
    assert!(
        josip.say(" 385 91 123 4567  ") == josip.say("tri osam pet, devet jedan, jedan dva tri, četiri pet šest sedam")
    );
}

/// Recorded voices pause inside a time and before a cell's address.
///
/// The short pauses of a clock time, a numeric date and a cell's address (a
/// listener's report of November 2026; see the formant tests): the recorded
/// voices render the parts of the clause one after another as well.
#[test]
fn recorded_voices_pause_inside_a_time_and_before_a_cells_address() {
    let mut engine = Speaker::empty();
    for voice in ["josip", "vlado", "detence"] {
        engine.set_voice(voice).unwrap_or_else(|error| panic!("{voice}: {error}"));
        let cell = engine.say("Marjan A17");
        assert!(!cell.is_empty(), "{voice}");
        assert!(cell == engine.say("Marjan \u{FDD2} A\u{FDD1} sedamnaest"), "{voice}");
        assert!(cell != engine.say("Marjan A sedamnaest"), "{voice}");
        // NVDA in Excel: the address as two words, two spaces after every item
        assert!(engine.say("Marjan   A 17  ") == cell, "{voice}");
        // Samples of pause at a comma pause of 100 ms: half of it, 1102
        // samples, inside a time or a date, the whole before a cell's address
        for (text, samples) in
            [("Marjan A17", 2205), ("12:30:45", 2 * 1102), ("10.11.2026.", 2 * 1102), ("Dobar dan svima", 0)]
        {
            engine.set_comma_pause(0);
            let without = engine.say(text);
            engine.set_comma_pause(100);
            let with = engine.say(text);
            assert_eq!(with.len(), without.len() + samples, "{voice}: {text}");
        }
        assert!(engine.say("10.11.2026.") == engine.say("10. 11. 2026."), "{voice}");
    }
}

#[test]
fn recorded_voices_speak_currency_signs_and_localized_amounts() {
    let mut josip = Speaker::new("josip");
    let expected = josip.say("minus dvije tisuće petsto zarez nula nula eura");
    assert!(!expected.is_empty());
    assert!(josip.say("−2.500,00 €") == expected);
    assert!(josip.say("Stanje $2,150.00") == josip.say("Stanje dvije tisuće sto pedeset zarez nula nula dolara"));
}
