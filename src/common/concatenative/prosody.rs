//! Clause layout for the recorded voices: which recording speaks each phone,
//! for how long, with what gaps and joins, and the pitch to render it at.
//!
//! The durations follow the rules of the formant voices; the melody is the
//! shared intonation model.

use super::psola::{PitchContour, Segment};
use super::unit_bank::UnitBank;
use super::voice_data::Sound;
use crate::formant::phones::{Ph, PhClass, Phone, needs_vowel_separation};
use crate::math::pow2f;
use crate::params::{
    CONCAT_SPEED_MAX, CONCAT_SPEED_MIN, CONCAT_USER_PITCH_MAX, CONCAT_USER_PITCH_MIN, INFLECTION_LEVEL_DEFAULT,
    VoiceParams,
};
use crate::prosody::intonation::{Intonation, Syllable};
use crate::prosody::{ClauseKind, MINOR_BREAK_STRETCH, Utterance};

/// Duration factor of a word that is one vowel (i, a, u, o), as in the
/// formant voices: a syllable of its own, not a shortened clitic vowel.
const VOWEL_WORD_LENGTH: f32 = 1.30;

// Gap between two words (ms at rate 1.0); clitics join their host without one.
const WORD_GAP_MS: f32 = 22.0;
const WORD_GAP_MIN_MS: f32 = 5.0;

/// The burst of a stop is played whole even when the rate leaves less time.
const BURST_MIN_MS: f32 = 20.0;

/// The closure a stop or affricate keeps before its release whatever the
/// rate (ms at rate 1.0, and the floor at fast rates): without one, a release
/// that fills its slot follows the vowel with no stop to be heard. Above 1.4x
/// only Josip's k had a closure left, and Vlado's p, k and affricates, whose
/// releases are longer than their slots, had none at any rate (2026-11-17).
const CLOSURE_MS: f32 = 18.0;
const CLOSURE_MIN_MS: f32 = 8.0;

// A trill recording: the duration of /r/ between vowels.
const TRILL_MS: f32 = 50.0;
const TRILL_MIN_MS: f32 = 22.0;

/// Grid of the pitch contour.
const CONTOUR_STEP_MS: f32 = 1.0;

/// The laid-out clause: segments to render and the pitch to render them at.
#[derive(Clone, Debug, Default)]
pub struct ClausePlan<'a> {
    pub segments: Vec<Segment<'a>>,
    pub contour: PitchContour,
    pub total_samples: f32,
}

/// The recording that speaks a phone of the front end; `None` for a phone
/// without one (the schwa of a syllabic consonant).
pub fn unit_for(ph: Ph) -> Option<Sound> {
    Some(match ph {
        Ph::Sil => Sound::Silence,
        Ph::A => Sound::A,
        Ph::I => Sound::I,
        Ph::U => Sound::U,
        Ph::Schwa => return None,
        // Slovenian: the open vowels on the recorded e and o, the schwa on a
        // short e, the [w] on the u.
        Ph::E | Ph::Eh | Ph::Ee | Ph::Ax => Sound::E,
        Ph::O | Ph::Oh | Ph::Oo => Sound::O,
        Ph::W => Sound::U,
        Ph::J => Sound::J,
        Ph::V => Sound::V,
        Ph::L => Sound::L,
        Ph::Lj => Sound::Lj,
        Ph::M => Sound::M,
        Ph::N | Ph::Ng => Sound::N,
        Ph::Nj => Sound::Nj,
        Ph::R => Sound::R,
        Ph::P => Sound::P,
        Ph::B => Sound::B,
        Ph::T => Sound::T,
        Ph::D => Sound::D,
        Ph::K => Sound::K,
        Ph::G => Sound::G,
        Ph::C | Ph::Dz => Sound::C,
        Ph::Ch => Sound::Ch,
        Ph::Tj => Sound::Tj,
        Ph::Dzh | Ph::Dj => Sound::Dj,
        Ph::F => Sound::F,
        Ph::S => Sound::S,
        Ph::Z => Sound::Z,
        Ph::Sh | Ph::Sj => Sound::Sh,
        Ph::Zh | Ph::Zj => Sound::Zh,
        Ph::H | Ph::Gh => Sound::H,
    })
}

/// Lays out one clause. `natural_f0` is the pitch of the voice as recorded
/// (Hz), before the voice character and user pitch.
pub fn plan_clause<'a>(
    utt: &Utterance,
    bank: &'a UnitBank,
    params: &VoiceParams,
    natural_f0: f32,
    sample_rate: u32,
) -> ClausePlan<'a> {
    let ph = &utt.phones;
    let samples_per_ms = sample_rate as f32 / 1000.0;

    let speed = params.effective_speed().clamp(CONCAT_SPEED_MIN, CONCAT_SPEED_MAX);
    let rate = |d: f32, min_d: f32| {
        let floor_d = if speed > 1.0 { min_d / speed.sqrt() } else { min_d };
        (d / speed).max(floor_d)
    };
    let phone_at = |i: usize| ph.get(i);

    // ---------------------------------------------------------------------
    // Durations (ms): the rules of the formant voices
    // ---------------------------------------------------------------------
    let last_syl = utt.syllable_count - 1;
    let final_stretch = if utt.minor_break {
        MINOR_BREAK_STRETCH
    } else if matches!(utt.kind, ClauseKind::Statement | ClauseKind::Exclamation) {
        1.40
    } else {
        1.30
    };
    let last_nucleus = ph.iter().rposition(|p| p.nucleus);

    let dur: Vec<f32> = ph
        .iter()
        .enumerate()
        .map(|(i, p)| {
            let def = p.ph.def();
            let prev = i.checked_sub(1).and_then(phone_at);
            let next = phone_at(i + 1);
            let next2 = phone_at(i + 2);
            let final_syl = i32::from(p.syllable) == last_syl;
            let mut min_d = def.min_dur;

            if p.ph == Ph::Schwa {
                // No recording: the syllabic consonant carries the syllable.
                // A consonant letter's sound alone ends in one, the vowel the
                // consonant is released into ("bə", "sə"; see below).
                return if utt.isolated_sound && p.nucleus { rate(120.0, 40.0) } else { 0.0 };
            }
            let mut d = if p.ph == Ph::R {
                // The recording is a trill; a syllabic r a little longer.
                min_d = TRILL_MIN_MS;
                let mut d = TRILL_MS * if p.nucleus_tail { 1.3 } else { 1.0 };
                if p.nucleus_tail && p.stressed {
                    d *= 1.2;
                }
                if p.nucleus_tail && final_syl {
                    d *= 1.2;
                }
                d
            } else if p.nucleus {
                let mut factor: f32;
                if p.stressed {
                    // As in the formant voices: a function word's long vowel
                    // keeps most of its length (tȋ), and only a long vowel is
                    // lengthened in a word of one syllable (sȁd stays short).
                    factor = if p.is_long { 1.90 } else { 1.35 };
                    if p.prominence < 2 {
                        factor = if p.is_long { 1.70 } else { 1.15 };
                    }
                    if p.word_syllables == 1 && p.is_long {
                        factor *= 1.12;
                    }
                } else if p.vowel_word {
                    factor = VOWEL_WORD_LENGTH;
                } else {
                    factor = if p.is_long { 1.25 } else { 1.0 };
                    if p.prominence == 0 {
                        factor *= 0.90;
                    }
                }
                if p.word_syllables >= 6 {
                    factor *= 0.88;
                } else if p.word_syllables >= 4 {
                    factor *= 0.93;
                }
                if next.is_some_and(|n| n.word == p.word && n.ph.is_obstruent() && !n.ph.def().voiced) {
                    factor *= 0.93;
                }
                if !p.vowel_word && (next.is_some_and(|n| n.ph.is_vowel()) || prev.is_some_and(|n| n.ph.is_vowel())) {
                    factor *= 0.90;
                }
                // A word of one vowel is an open syllable: the consonants
                // after it belong to the next word.
                if !p.vowel_word
                    && next.is_some_and(|n| n.ph.is_consonant())
                    && next2.is_none_or(|n| n.ph.is_consonant())
                {
                    factor *= 0.93; // closed syllable
                }
                if final_syl {
                    // Half the final lengthening for a short stressed vowel
                    // except in a question, as in the formant voices: sȁd
                    // stays short next to sȃd.
                    let statement = !matches!(utt.kind, ClauseKind::YesNoQuestion | ClauseKind::WhQuestion);
                    factor *= if p.stressed && !p.is_long && statement && p.prominence == 2 {
                        1.0 + (final_stretch - 1.0) * 0.5
                    } else {
                        final_stretch
                    };
                } else if i32::from(p.syllable) == last_syl - 1 {
                    factor *= 1.08;
                }
                def.dur * factor
            } else {
                let mut factor: f32 = 0.92;
                let prev_c = prev.is_some_and(|n| n.ph.is_consonant());
                let next_c = next.is_some_and(|n| n.ph.is_consonant());
                if prev_c && next_c {
                    factor *= 0.80;
                } else if prev_c || next_c {
                    factor *= 0.88;
                }
                if p.word_start {
                    factor *= 1.05;
                }
                if next.is_some_and(|n| n.nucleus && n.stressed) {
                    factor *= 1.08;
                }
                if last_nucleus.is_none_or(|last| i > last) {
                    factor *= 1.20;
                }
                if prev.is_some_and(|n| n.ph == p.ph) {
                    factor *= 0.60;
                }
                if p.short_glide { 34.0 } else { def.dur * factor }
            };
            if utt.isolated_sound {
                // A letter's sound alone (spelling by sounds): a vowel letter
                // as a stressed syllable; a consonant letter as the consonant
                // (fricatives held a little, stops and affricates with their
                // whole recording) and the release vowel after it, as
                // synthesizer B sounds the letters out.
                d = match def.cls {
                    PhClass::Vowel => 190.0,
                    PhClass::Fricative => 100.0,
                    PhClass::Nasal | PhClass::Liquid | PhClass::Glide => 85.0,
                    PhClass::Tap => TRILL_MS * 1.2,
                    PhClass::Stop | PhClass::Affricate => def.dur * 1.3,
                    PhClass::Silence => d,
                };
            }
            rate(d, min_d)
        })
        .collect();

    // ---------------------------------------------------------------------
    // Segments
    // ---------------------------------------------------------------------
    let mut segments: Vec<Segment<'a>> = Vec::new();
    let mut syls = vec![Syllable::default(); utt.syllable_count.max(0) as usize];
    let mut cursor = 0f32; // samples

    fn add_silence(segments: &mut Vec<Segment<'_>>, cursor: &mut f32, samples: f32) {
        if samples > 0.0 {
            segments.push(Segment { start: *cursor, length: samples, ..Segment::default() });
            *cursor += samples;
        }
    }
    let mut note_syllable = |p: &Phone, t0: f32, t1: f32| {
        if !(p.nucleus || p.nucleus_tail) {
            return;
        }
        let Some(syl) = usize::try_from(p.syllable).ok().and_then(|s| syls.get_mut(s)) else { return };
        if syl.t0 < 0.0 || t0 < syl.t0 {
            syl.t0 = t0;
        }
        syl.t1 = syl.t1.max(t1);
        syl.word = i32::from(p.word);
        syl.prominence = p.prominence;
        if p.nucleus && p.stressed {
            syl.stressed = true;
            syl.accent = p.accent;
        }
    };

    let mut prev_is_unit = false;
    // The word gap just written, which is a part of the closure of a stop
    // that begins the word.
    let mut gap_before = 0f32;
    for (i, p) in ph.iter().enumerate() {
        let d_ms = dur[i];

        // A short gap before a word that is not a clitic, and at every
        // vowel-to-vowel word boundary, including clitics ("vidio ih").
        // One-vowel words also keep their separation next to /j/.
        let hiatus = i > 0 && needs_vowel_separation(&ph[i - 1], p);
        if i > 0 && p.word_start && (p.prominence >= 1 || hiatus) && prev_is_unit {
            gap_before = rate(WORD_GAP_MS, WORD_GAP_MIN_MS) * samples_per_ms;
            add_silence(&mut segments, &mut cursor, gap_before);
            prev_is_unit = false;
        }

        // The release vowel of a letter's sound: the "e" recording, a little
        // softer than a full vowel.
        let release_vowel = p.ph == Ph::Schwa && d_ms > 0.0;
        let sound = if release_vowel { Some(Sound::E) } else { unit_for(p.ph) };
        let unit = sound
            .filter(|&sound| sound != Sound::Silence)
            .map(|sound| bank.unit(sound))
            .filter(|unit| unit.loaded && unit.sounding_length() > 0);
        let t0_ms = cursor / samples_per_ms;

        let Some(unit) = unit else {
            if d_ms > 0.0 {
                add_silence(&mut segments, &mut cursor, d_ms * samples_per_ms);
                prev_is_unit = false;
                gap_before = 0.0;
            }
            note_syllable(p, t0_ms, cursor / samples_per_ms);
            continue;
        };

        let slot = d_ms * samples_per_ms;
        let mut s = Segment { unit: Some(unit), src_begin: unit.lead, src_end: unit.trail, ..Segment::default() };
        // The long friction the hiss hold acts on: the sibilants, voiced and
        // voiceless (in the recordings z, ž and đ hiss as loudly as s, š and
        // ć), and f, as loud above 2 kHz as they are. Not the burst of t,
        // which is short and all there is of a t.
        s.hiss = matches!(
            sound,
            Some(Sound::S | Sound::Z | Sound::Sh | Sound::Zh | Sound::C | Sound::Ch | Sound::Tj | Sound::Dj | Sound::F)
        );
        let class = p.ph.class();
        if matches!(class, PhClass::Stop | PhClass::Affricate) {
            // The recording is closure (silent or murmured), burst and tail.
            // The release has the slot less a closure's time, the time left
            // goes to the closure, first from the recording's own, then as
            // silence. A stop is never stretched: a release longer than its
            // room loses its tail, never its burst. The friction of an
            // affricate is the sound itself: it is played faster instead,
            // from its start to its end.
            let release_len = (unit.trail - unit.onset) as f32;
            let reserve = (rate(CLOSURE_MS, CLOSURE_MIN_MS) * samples_per_ms).min(0.4 * slot);
            let room = (slot - reserve).max(release_len.min(BURST_MIN_MS * samples_per_ms));
            let release = release_len.min(room);
            let pre_len = (unit.onset - unit.lead) as f32;
            let pre = (slot - release).max(0.0).min(pre_len);
            // A stop that begins a word has the word gap as a part of its
            // closure, and the rest of it after the gap.
            let closure = slot - release - pre - if prev_is_unit { 0.0 } else { gap_before };
            if closure > 0.0 && (prev_is_unit || gap_before > 0.0) {
                add_silence(&mut segments, &mut cursor, closure);
                prev_is_unit = false;
            }
            s.src_begin = unit.onset - pre as i32;
            if class == PhClass::Affricate && release < release_len {
                s.src_end = unit.trail;
                s.stretch = true;
                s.length = pre + release;
            } else {
                s.src_end = unit.onset + release as i32;
                s.stretch = false;
                s.length = (s.src_end - s.src_begin) as f32;
            }
        } else {
            s.stretch = true;
            s.length = slot;
            if release_vowel {
                s.gain = 0.8;
            } else if p.short_glide {
                // The glide of "ije" has 34 ms, and the recording of j is a
                // hum for 38 ms and the glide for its last 20: played whole,
                // the hum had the larger share. It gives up the quiet
                // build-up before its strong part first (2026-11-17). Only
                // here: for every consonant short of time a recognizer heard
                // l, r and v right a little more often at 1.5x and clearly
                // less often at 2x (l in 9 of 38 items against 14).
                let sounding = (unit.trail - unit.lead) as f32;
                let strong = (unit.strong_end - unit.strong_begin) as f32;
                let mut cut = sounding - slot.max(strong).min(sounding);
                let before = cut.min((unit.strong_begin - unit.lead) as f32);
                s.src_begin += before as i32;
                cut -= before;
                s.src_end -= cut.min((unit.trail - unit.strong_end) as f32) as i32;
            }
        }
        s.start = cursor;
        if prev_is_unit
            && unit.has_voicing()
            && let Some(before) =
                segments.last_mut().filter(|before| before.unit.is_some_and(|unit| unit.has_voicing()))
        {
            before.join_next = true;
            s.join_prev = true;
        }
        cursor += s.length;
        segments.push(s);
        prev_is_unit = true;
        gap_before = 0.0;
        note_syllable(p, t0_ms, cursor / samples_per_ms);
    }

    // ---------------------------------------------------------------------
    // Pitch contour
    // ---------------------------------------------------------------------
    let total_ms = cursor / samples_per_ms;
    let intonation = Intonation::new(utt, &syls, total_ms, 1.0);

    let points = (total_ms / CONTOUR_STEP_MS).ceil() as usize + 2;
    let mut st: Vec<f32> = (0..points).map(|k| intonation.at((k as f32 + 0.5) * CONTOUR_STEP_MS)).collect();

    // Pitch starts higher after a voiceless consonant and settles quickly
    // (the inflection level scales this away with the rest of the contour).
    let voiced = |seg: &Segment<'_>| seg.unit.map(|unit| unit.has_voicing());
    for pair in segments.windows(2) {
        if voiced(&pair[1]) != Some(true) || voiced(&pair[0]) != Some(false) {
            continue;
        }
        let k0 = (pair[1].start / samples_per_ms / CONTOUR_STEP_MS) as usize;
        for (step, st) in st.iter_mut().skip(k0).take(150).enumerate() {
            *st += 0.9 * (-(step as f32 * CONTOUR_STEP_MS) / 18.0).exp();
        }
    }

    // The larynx cannot follow corners: smooth forwards and backwards.
    let a = (-CONTOUR_STEP_MS / 16.0).exp();
    for k in 1..points {
        st[k] = a * st[k - 1] + (1.0 - a) * st[k];
    }
    for k in (0..points - 1).rev() {
        st[k] = a * st[k + 1] + (1.0 - a) * st[k];
    }

    let level = params.inflection_level.clamp(0.0, 1.0) / INFLECTION_LEVEL_DEFAULT;
    let user_pitch = params.user_pitch.clamp(CONCAT_USER_PITCH_MIN, CONCAT_USER_PITCH_MAX);
    let base = natural_f0 * params.pitch * user_pitch;
    let hz = st.iter().map(|st| (base * pow2f(level * st / 12.0)).clamp(45.0, 480.0)).collect();
    ClausePlan { segments, contour: PitchContour { hz }, total_samples: cursor }
}
