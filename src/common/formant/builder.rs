//! The clause builder: phones to control frames.
//!
//! Durations, acoustic segments (closures, releases, the contacts of r),
//! formant targets with locus-based transitions, the control tracks and the
//! pitch contour of one clause. Every constant here was set against a
//! measurement or a listening test; docs/formant.md records them.

use super::klatt::{FRAME_MS, Frame, NOISE_PEAKS};
use super::phones::{NoiseSpec, Ph, PhClass, PhDef, Phone, needs_vowel_separation};
use super::singing::Note;
use super::voice::FormantVoice;
use crate::math::pow2f;
use crate::params::{INFLECTION_LEVEL_DEFAULT, VoiceParams};
use crate::prosody::intonation::{Intonation, Syllable};
use crate::prosody::{ClauseKind, MINOR_BREAK_STRETCH, Utterance};

const SCHWA_F: [f32; 3] = [500.0, 1380.0, 2600.0];

/// F4's bandwidth in the cascade for the speaking voices (the singing presets
/// keep 180): with 180 Hz the /a/'s 3-4 kHz stood 3-5 dB above synthesizer A's
/// after the source and F5 had been matched (November 2026).
const SPEAKING_F4_BANDWIDTH: f32 = 220.0;

/// Silent frames before the first sound. The friction track is box-smoothed
/// with a radius of 3 frames, so the lead must be one frame longer than that
/// for a clause that opens with s, š, f or h to rise from zero: with 3 lead
/// frames the first output sample of "sapa" jumped to 4 % of full scale, a
/// tick at the start of every such utterance (November 2026).
const LEAD_FRAMES: i32 = 4;

/// How long a noisy segment's filter bank stays in place after the segment
/// ends: the smoothing radius of the friction track plus one frame for the
/// bank to ring out.
const NOISE_HOLD_FRAMES: i32 = 4;

/// An affricate alone holds its friction for long enough to be told from a
/// stop. The consonant keeps its own level: synthesizer B's sit at 20-45% of
/// the vowel that follows, and so do these.
const ISOLATED_AFFRICATE_MS: f32 = 110.0;

/// A word that is one vowel (the conjunctions i and a, the prepositions u and
/// o) is a syllable of its own, not a clitic vowel: as long as the stressed
/// vowel of a function word and a little more, at full level, and set off
/// from a vowel or j of the word next to it by a short dip of the voice.
/// Shortened, weakened and gliding like the other clitics it was lost between
/// its neighbours ("ja i ona" as "jajona"). See "Words of one vowel" in
/// docs/formant.md.
const VOWEL_WORD_LENGTH: f32 = 1.30;

/// The stressed long vowel of a function word (tȋ, mȋ, vȋ, jȃ, ȏn, svȋ, vȇć),
/// relative to the inherent duration; a content word's has 1.90.
const WEAK_LONG_VOWEL: f32 = 1.70;
// The one-vowel-word cue at every vowel-to-vowel word boundary. The dip
// shrinks with the rate like the sounds around it.
const VOWEL_BOUNDARY_DIP: f32 = 0.15; // voicing at the join
const VOWEL_BOUNDARY_DIP_MS: f32 = 18.0; // at rate 1
const VOWEL_BOUNDARY_DIP_MIN_MS: f32 = 8.0;

// The closure of a stop or affricate at the start of a clause: silence kept
// short; a voice bar with F1 damped (the source is raised to keep its level),
// duller and no longer than synthesizer A's.
const INITIAL_CLOSURE_MS: f32 = 14.0;
const INITIAL_VOICE_BAR_MS: f32 = 60.0;
const INITIAL_VOICE_BAR_AV: f32 = 1.0; // 1.20 until the low shelf of November 2026 lifted every bar 5-6 dB
/// The voice bar of b, d, g between vowels (d's own is `D_BAR_AV`). 0.95 until
/// November 2026, when the low shelf on the voicing lifted the bars by 5-6 dB.
const VOICE_BAR_AV: f32 = 0.70;
const INITIAL_VOICE_BAR_TILT: f32 = 30.0;
const INITIAL_VOICE_BAR_B1: f32 = 200.0;

// The labiodental v: no longer than this at the start of a clause, with
// little friction right after a stop or affricate, and partly devoiced after
// a voiceless fricative and before a pause.
const INITIAL_V_MS: f32 = 60.0;
const V_AFTER_STOP_AF: f32 = 0.3;
const V_DEVOICED_AV: f32 = 0.5;
const V_DEVOICED_AF: f32 = 1.5;

// d, a little harder than b and g: a weaker voice bar that fades just before
// the release (to 40% over 15 ms), a stronger burst with less voicing under
// it, the vowel opening as fast as after p and b, and at the start of a
// clause a short bar and a long release (the recorded speaker's d: the bar
// 10-20 dB under the vowel and fading out, the release 16-20 ms of noise at
// the start). With the loud bar and short burst of b, d was heard as l or n
// ("ada" as "ala", the spelled "de" as "le"). See "v, l, nj and d" in
// docs/formant.md.
const D_BAR_AV: f32 = 0.45;
const D_INITIAL_BAR_MS: f32 = 40.0;
const D_BAR_FADE_MS: f32 = 15.0;
const D_BAR_FADE: f32 = 0.60;
const D_BURST: f32 = 1.5; // t's af fell from 0.50 to 0.27 for synthesizer A's burst; d keeps most of its release
// The velar burst by the vowel after it (synthesizer A: k -15.6 dB re the
// vowel before a, -20.5 before i and u): louder before a central vowel,
// weaker before a front one.
const VELAR_CENTRAL_BURST: f32 = 1.6;
const VELAR_FRONT_BURST: f32 = 0.8;

/// The first 6 ms of an affricate's friction, over its body: the transient of
/// the stop release. It was 1.5 (1.25 for c) until November 2026, the
/// strongest friction in the voice after the k burst; on a phone at full
/// volume it was the spike listeners heard in č, ć, c.
const AFFRICATE_ATTACK: f32 = 1.15;
const D_RELEASE_AV: f32 = 0.55;
const D_RELEASE_MS: f32 = 8.0;
const D_INITIAL_RELEASE_MS: f32 = 16.0;
const D_INITIAL_DECAY_MS: f32 = 9.0;
const D_F1_SNAP: f32 = 0.72;

/// A clause-final l keeps a higher F1 (the recorded speaker's 450 Hz,
/// synthesizer B's 390-450): with the low F1 of an l between vowels it was
/// heard as a nasal at the end ("sol" as "son", the spelled "el" as "em"),
/// and no vowel follows that the deep dip would set it off from.
const FINAL_L_F1: f32 = 420.0;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum SegKind {
    Plain,
    /// Stop or affricate occlusion.
    Closure,
    /// Burst (stops) or friction (affricates).
    Release,
}

#[derive(Clone, Copy, Debug)]
struct Seg {
    ph: Ph,
    kind: SegKind,
    /// Index of the phone it belongs to.
    phone: i32,
    dur: f32,
    start: f32,

    /// Vowel, glide, liquid, nasal.
    has_target: bool,
    /// Keeps its own formants up to the edge.
    abrupt: bool,
    voiced: bool,
    rank: i32,
    tr_out: f32,

    tgt: [f32; 3],
    anchor: [f32; 3],
    coart: [f32; 3],
    own: [f32; 3],
    lb: [f32; 3],
    rb: [f32; 3],
    tl: [f32; 3],
    tr: [f32; 3],
    f4: f32,
    bw: [f32; 3],

    av: f32,
    ah: f32,
    /// Smoothed friction.
    af: f32,
    /// Unsmoothed friction (sharp onset).
    burst: f32,
    /// ms; 0 = sustained.
    burst_decay: f32,
    /// Slower second decay of a release into a pause: its share of the burst ...
    burst_tail: f32,
    /// ... and its time constant (ms).
    tail_decay: f32,
    tilt: f32,
    nasal: f32,
    /// Semitones (intrinsic pitch, obstruent dip).
    f0_shift: f32,
    noise: Option<NoiseSpec>,
    /// Leaves its left boundary slowly (the vowel after nj).
    slow_onset: bool,
}

/// The notes one syllable is sung on, and the rest after them.
#[derive(Clone, Debug)]
struct SungSyllable {
    /// The first note and the tied ones after it.
    notes: Vec<Note>,
    /// Total of the notes.
    note_ms: f32,
    rest_ms: f32,
    /// Start of the syllable in the clause (ms).
    t0: f32,
    /// End of its last sound (its rest begins here).
    sound_end: f32,
    /// First of the clause or after a rest: a fresh attack.
    after_rest: bool,
}

/// Where in its notes a frame of a syllable is.
struct NotePos {
    midi: i32,
    /// ms since the syllable started (vibrato and attack run through a melisma).
    t_syl: f32,
    /// ms until the syllable's sound ends.
    remaining: f32,
    in_rest: bool,
}

/// Box average over `radius` frames each way.
fn smooth(track: &mut [f32], radius: usize) {
    if radius == 0 || track.len() < 2 {
        return;
    }
    let n = track.len();
    let mut prefix = vec![0f32; n + 1];
    for i in 0..n {
        prefix[i + 1] = prefix[i] + track[i];
    }
    for (i, value) in track.iter_mut().enumerate() {
        let lo = i.saturating_sub(radius);
        let hi = (i + radius).min(n - 1);
        *value = (prefix[hi + 1] - prefix[lo]) / (hi - lo + 1) as f32;
    }
}

/// Exponential smoothing forward and backward (zero lag).
fn smooth_both_ways(st: &mut [f32], a: f32) {
    for j in 1..st.len() {
        st[j] = a * st[j - 1] + (1.0 - a) * st[j];
    }
    for j in (0..st.len().saturating_sub(1)).rev() {
        st[j] = a * st[j + 1] + (1.0 - a) * st[j];
    }
}

fn frame_of(ms: f32) -> i32 {
    (ms / FRAME_MS).round() as i32
}

/// The song a singing preset is in: its notes and the next one to sing.
pub struct Melody<'a> {
    pub notes: &'a [Note],
    pub cursor: &'a mut usize,
}

/// Builds the control frames of one clause.
pub struct ClauseBuilder<'a> {
    voice: &'a FormantVoice,
    params: &'a VoiceParams,
    utt: &'a Utterance,
    ph: &'a [Phone],
    melody: Option<Melody<'a>>,
    singing: bool,
    tempo: f32,
    speed: f32,
    total_ms: f32,
    lead: i32,
    sung: Vec<SungSyllable>,

    dur: Vec<f32>,
    segs: Vec<Seg>,
    seg_of_frame: Vec<i32>,
    frames: Vec<Frame>,
}

impl<'a> ClauseBuilder<'a> {
    pub fn new(
        voice: &'a FormantVoice,
        params: &'a VoiceParams,
        utt: &'a Utterance,
        melody: Option<Melody<'a>>,
    ) -> Self {
        let melody = melody.filter(|melody| voice.singing.is_some() && !melody.notes.is_empty());
        let singing = melody.is_some();
        // When singing the rate sets the tempo. Consonants follow it only
        // part of the way: they are spoken, not sung.
        let tempo = if singing { params.effective_speed() } else { 1.0 };
        let speed = if singing { tempo.clamp(0.6, 2.0) } else { params.effective_speed() * voice.tempo };
        Self {
            voice,
            params,
            utt,
            ph: &utt.phones,
            melody,
            singing,
            tempo,
            speed,
            total_ms: 0.0,
            lead: 0,
            sung: Vec::new(),
            dur: Vec::new(),
            segs: Vec::new(),
            seg_of_frame: Vec::new(),
            frames: Vec::new(),
        }
    }

    pub fn build(mut self) -> Vec<Frame> {
        if self.singing {
            self.plan_notes();
        }
        self.assign_durations();
        self.expand_segments();
        self.compute_formants();
        self.build_tracks();
        self.build_pitch();
        // Asked once, not for every clause spoken.
        static TRACE: std::sync::OnceLock<bool> = std::sync::OnceLock::new();
        if *TRACE.get_or_init(|| std::env::var_os("LAPRDUS_TRACE_SEGMENTS").is_some()) {
            self.trace_segments();
        }
        self.frames
    }

    /// Debug aid: the segment table on stderr, with the time of each segment
    /// in the rendered clause (the lead frames included), so that a click or a
    /// spectral event measured in the output can be put to its sound.
    fn trace_segments(&self) {
        let lead_ms = self.lead as f32 * FRAME_MS;
        eprintln!("segments (ms from the clause start; lead {lead_ms:.0} ms):");
        eprintln!("  start   dur  phone   kind     av    af  burst    ah  tilt  noise f/bw/a + bypass");
        for seg in &self.segs {
            let kind = format!("{:?}", seg.kind).to_lowercase();
            let phone = format!("{:?}", seg.ph).to_uppercase();
            eprint!(
                "  {:5.0} {:5.0}  {phone:<6}  {kind:<7} {:5.2} {:5.2}  {:5.2} {:5.2} {:5.1}",
                lead_ms + seg.start,
                seg.dur,
                seg.av,
                seg.af,
                seg.burst,
                seg.ah,
                seg.tilt
            );
            if let Some(noise) = seg.noise {
                for k in 0..NOISE_PEAKS {
                    eprint!("  {:.0}/{:.0}/{:.2}", noise.f[k], noise.b[k], noise.a[k]);
                }
                eprint!(" +{:.2} fa {:.2}/{:.2}/{:.2}", noise.bypass, noise.fa[0], noise.fa[1], noise.fa[2]);
            }
            eprintln!();
        }
    }

    fn phone_at(&self, i: i32) -> Option<&'a Phone> {
        usize::try_from(i).ok().and_then(|i| self.ph.get(i))
    }

    fn rate(&self, d: f32, min_d: f32) -> f32 {
        let floor_d = if self.speed > 1.0 { min_d / self.speed.sqrt() } else { min_d };
        (d / self.speed).max(floor_d)
    }

    /// Formant transitions shorten less than segments do when speech is fast.
    fn transition_rate(&self, t: f32) -> f32 {
        if self.speed > 1.0 { t / self.speed.sqrt() } else { t }
    }

    fn beat_ms(&self) -> f32 {
        let bpm = self.voice.singing.map_or(60.0, |style| style.song.bpm);
        60000.0 / (bpm * self.tempo)
    }

    // -------------------------------------------------------------------------
    // Durations
    // -------------------------------------------------------------------------

    fn assign_durations(&mut self) {
        let utt = self.utt;
        let last_syl = utt.syllable_count - 1;
        let final_stretch = if utt.minor_break {
            MINOR_BREAK_STRETCH
        } else if matches!(utt.kind, ClauseKind::Statement | ClauseKind::Exclamation) {
            1.40
        } else {
            1.30
        };

        // Phones after the last nucleus form the final coda.
        let last_nucleus = self.ph.iter().rposition(|p| p.nucleus).map_or(-1, |i| i as i32);

        self.dur = (0..self.ph.len() as i32)
            .map(|i| {
                let p = &self.ph[i as usize];
                let def = p.ph.def();
                let prev = self.phone_at(i - 1);
                let next = self.phone_at(i + 1);
                let next2 = self.phone_at(i + 2);
                let final_syl = i32::from(p.syllable) == last_syl;

                let mut d = if p.nucleus_tail || p.ph == Ph::Schwa {
                    // Syllabic r is vocoid + tap + vocoid; the epenthetic
                    // schwa of "bicikl" is a short vowel of its own.
                    if p.ph == Ph::R {
                        def.dur
                    } else {
                        let mut d: f32 = if !p.stressed {
                            38.0
                        } else if p.is_long {
                            60.0
                        } else {
                            46.0
                        };
                        if p.nucleus_tail {
                            d *= 0.75;
                        }
                        if final_syl {
                            d *= 1.2;
                        }
                        d
                    }
                } else if p.nucleus {
                    let mut factor: f32;
                    if p.stressed {
                        // A function word is weaker than a content word, but
                        // its long vowel keeps most of its length: at 1.50
                        // the tȋ of "Ti si dobar" (99 ms) was shorter than
                        // the short o of dobȁr and heard as short. A word of
                        // one syllable is longer only if its vowel is: the
                        // short vowel of sȁd, brȁt with the same 12% was
                        // heard as long.
                        factor = if p.is_long { 1.90 } else { 1.35 };
                        if p.prominence < 2 {
                            factor = if p.is_long { WEAK_LONG_VOWEL } else { 1.15 };
                        }
                        if p.word_syllables == 1 && p.is_long {
                            factor *= 1.12;
                        }
                    } else if p.vowel_word {
                        factor = VOWEL_WORD_LENGTH;
                    } else {
                        factor = if p.is_long { self.voice.postaccent_length } else { 1.0 };
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
                    if !p.vowel_word && (next.is_some_and(|n| n.ph.is_vowel()) || prev.is_some_and(|n| n.ph.is_vowel()))
                    {
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
                        // Before a period, exclamation mark or comma a short
                        // stressed vowel takes only half of the final
                        // lengthening, so that a clause-final monosyllable
                        // keeps the short/long contrast (sȁd : sȃd, brȁt :
                        // grȃd); with the full stretch the short one was 70%
                        // of the long one and heard as long ("Dobro, sad,").
                        // A question keeps the full stretch (its rise needs
                        // the time), and so does a final function word (tȍ,
                        // tȉ), whose fall would otherwise not reach its target.
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
                    if i > last_nucleus {
                        factor *= 1.20;
                    }
                    if prev.is_some_and(|n| n.ph == p.ph) {
                        factor *= 0.60;
                    }
                    if p.ph == Ph::R {
                        def.dur
                    } else if p.short_glide {
                        34.0
                    } else {
                        def.dur * factor
                    }
                };

                if utt.isolated_sound {
                    d = isolated_duration(p, def);
                }
                self.rate(d, def.min_dur)
            })
            .collect();

        if self.singing {
            self.fit_durations_to_notes();
        }
    }

    // -------------------------------------------------------------------------
    // Singing: notes
    // -------------------------------------------------------------------------

    /// Takes the next notes of the song for every syllable of the clause: a
    /// note, the notes tied to it, and the rests that follow.
    fn plan_notes(&mut self) {
        let beat_ms = self.beat_ms();
        let Some(Melody { notes: melody, cursor }) = self.melody.as_mut() else { return };
        if !melody.iter().any(|note| note.midi > 0) {
            self.singing = false;
            return;
        }
        let len = melody.len();
        let peek = |cursor: &usize| melody[*cursor % len];
        let next = |cursor: &mut usize| {
            let note = melody[*cursor % len];
            *cursor = (*cursor + 1) % len;
            note
        };

        self.sung = (0..self.utt.syllable_count.max(0))
            .map(|_| {
                // Rests before a syllable's note are dropped (the clause
                // pause stands in for them); rests after it are kept.
                while peek(cursor).midi <= 0 {
                    next(cursor);
                }
                let mut notes = vec![Note { tie: false, ..next(cursor) }];
                while peek(cursor).tie && peek(cursor).midi > 0 {
                    notes.push(next(cursor));
                }
                let mut rest_ms = 0.0;
                while peek(cursor).midi <= 0 && !peek(cursor).tie {
                    rest_ms += next(cursor).beats * beat_ms;
                    // A rest at the very end of the song must not swallow the
                    // one at its start as well.
                    if **cursor == 0 {
                        break;
                    }
                }
                let note_ms = notes.iter().fold(0.0, |ms, note| ms + note.beats * beat_ms);
                SungSyllable { notes, note_ms, rest_ms, t0: 0.0, sound_end: 0.0, after_rest: true }
            })
            .collect();
    }

    /// Every syllable lasts as long as its notes, and its vowel starts on the
    /// beat: the consonants keep their spoken durations, those that open the
    /// next syllable are taken off the end of this one's vowel (or its rest),
    /// as singers do (Sundberg), and the vowel takes what is left, at least a
    /// third of the note.
    fn fit_durations_to_notes(&mut self) {
        let count = self.sung.len();
        let mut nucleus: Vec<Option<usize>> = vec![None; count];
        let (mut onset, mut coda) = (vec![0f32; count], vec![0f32; count]);
        let syllable_of = |p: &Phone| usize::try_from(p.syllable).ok().filter(|&s| s < count);
        for (i, p) in self.ph.iter().enumerate() {
            let Some(s) = syllable_of(p) else { continue };
            if nucleus[s].is_some() {
                coda[s] += self.dur[i];
            } else if p.nucleus {
                nucleus[s] = Some(i);
            } else {
                onset[s] += self.dur[i];
            }
        }

        for s in 0..count {
            let Some(vowel_at) = nucleus[s] else { continue };
            let note = self.sung[s].note_ms;
            let next_onset = if s + 1 < count { onset[s + 1] } else { 0.0 };
            let vowel_min = f32::max(40.0, 0.35 * note);
            let mut taken = coda[s];
            let rest = &mut self.sung[s].rest_ms;
            if *rest > 0.0 {
                // The next syllable's consonants eat into the rest first.
                let from_rest = next_onset.min(*rest);
                *rest -= from_rest;
                taken += next_onset - from_rest;
            } else {
                taken += next_onset;
            }
            let mut vowel = note - taken;
            if vowel < vowel_min && taken > 0.0 {
                let factor = (note - vowel_min).max(0.0) / taken;
                // This syllable's coda and the next one's onset give way.
                for (syllable, onset_part) in [(s, false), (s + 1, true)] {
                    let Some(&Some(centre)) = nucleus.get(syllable) else {
                        if syllable < count {
                            onset[syllable] *= factor;
                        }
                        continue;
                    };
                    for (i, p) in self.ph.iter().enumerate() {
                        if syllable_of(p) == Some(syllable) && i != centre && (i < centre) == onset_part {
                            self.dur[i] *= factor;
                        }
                    }
                    if onset_part {
                        onset[syllable] *= factor;
                    } else {
                        coda[syllable] *= factor;
                    }
                }
                vowel = vowel_min;
            }
            self.dur[vowel_at] = vowel;
        }
    }

    // -------------------------------------------------------------------------
    // Phones -> acoustic segments
    // -------------------------------------------------------------------------

    fn add_seg(&mut self, ph: Ph, kind: SegKind, phone: usize, dur: f32) -> &mut Seg {
        let def = ph.def();
        self.segs.push(Seg {
            ph,
            kind,
            phone: phone as i32,
            dur: dur.max(FRAME_MS),
            start: 0.0,
            has_target: false,
            abrupt: false,
            voiced: def.voiced,
            rank: def.rank,
            tr_out: self.transition_rate(def.tr_out),
            tgt: [0.0; 3],
            anchor: [0.0; 3],
            coart: [1.0; 3],
            own: [0.0; 3],
            lb: [0.0; 3],
            rb: [0.0; 3],
            tl: [0.0; 3],
            tr: [0.0; 3],
            f4: def.f4 * self.voice.formant_scale,
            bw: def.bw,
            av: 0.0,
            ah: 0.0,
            af: 0.0,
            burst: 0.0,
            burst_decay: 0.0,
            burst_tail: 0.0,
            tail_decay: 0.0,
            tilt: self.voice.tilt,
            nasal: 0.0,
            f0_shift: 0.0,
            noise: None,
            slow_onset: false,
        });
        self.segs.last_mut().expect("a segment was just pushed")
    }

    fn noise_for(&self, ph: Ph) -> NoiseSpec {
        let voice = self.voice;
        let mut spec = ph.def().noise;
        let shift = match ph {
            Ph::Ch | Ph::Dzh => voice.hard_palatal_shift * voice.hard_affricate_shift,
            Ph::Sh | Ph::Zh => voice.hard_palatal_shift,
            Ph::Tj | Ph::Dj | Ph::Sj | Ph::Zj => voice.soft_palatal_shift,
            _ => 1.0,
        };
        for f in &mut spec.f {
            *f *= shift * voice.formant_scale;
        }
        spec
    }

    /// The vocalic stretch of an r.
    fn r_vocoid(&mut self, phone: usize, ms: f32, level: f32) -> &mut Seg {
        let dur = self.rate(ms, ms * 0.5);
        let tilt = self.voice.tilt + 2.0;
        let seg = self.add_seg(Ph::R, SegKind::Plain, phone, dur);
        seg.av = level;
        seg.tilt = tilt;
        seg.f0_shift = -0.3;
        seg
    }

    /// A tongue-tip contact of an r.
    fn r_contact(&mut self, phone: usize) {
        let dur = self.rate(18.0, 10.0);
        let tilt = self.voice.tilt + 6.0;
        let seg = self.add_seg(Ph::R, SegKind::Plain, phone, dur);
        seg.av = Ph::R.def().av;
        seg.tilt = tilt;
        seg.f0_shift = -0.3;
    }

    /// The vocalic stretch after a contact, with the faint transient of the
    /// tongue tip leaving the ridge.
    fn r_release(&mut self, phone: usize, ms: f32, level: f32) {
        let noise = self.noise_for(Ph::R);
        let seg = self.r_vocoid(phone, ms, level);
        seg.burst = Ph::R.def().af;
        seg.burst_decay = 3.0;
        seg.noise = Some(noise);
    }

    fn expand_segments(&mut self) {
        let voice = self.voice;
        for (i, p) in self.ph.iter().enumerate() {
            let def = p.ph.def();
            let prev = self.phone_at(i as i32 - 1);
            let next = self.phone_at(i as i32 + 1);
            let d = self.dur[i];
            let class_of = |phone: Option<&Phone>| phone.map_or(PhClass::Silence, |phone| phone.ph.class());
            let next_cls = class_of(next);
            let next_vocalic = next.is_some() && matches!(next_cls, PhClass::Vowel | PhClass::Glide | PhClass::Liquid);

            match def.cls {
                PhClass::Vowel => {
                    let singing = self.singing;
                    let seg = self.add_seg(p.ph, SegKind::Plain, i, d);
                    seg.has_target = true;
                    let mut level: f32 = if p.stressed {
                        1.0
                    } else if p.prominence == 0 {
                        0.80
                    } else {
                        0.86
                    };
                    if p.vowel_word {
                        level = 1.0; // i, a, u, o: a syllable of their own
                    }
                    if p.nucleus_tail {
                        level *= 0.45; // second half of syllabic r
                    }
                    if singing && !p.nucleus_tail {
                        level = 1.0; // every vowel is sung in full
                    }
                    seg.av = def.av * level;
                    seg.tilt = voice.tilt + if p.stressed || p.vowel_word || singing { 0.0 } else { 1.5 };
                    // Intrinsic pitch: high vowels are slightly higher.
                    seg.f0_shift = match p.ph {
                        Ph::I | Ph::U => 0.5,
                        Ph::A => -0.4,
                        _ => 0.0,
                    };
                }

                PhClass::Glide | PhClass::Liquid => {
                    // A clause-initial v is heard on its own, and a slow rate
                    // drew it out (96 ms before the "e" of the spelled "ve"
                    // at the default spelling speed) into a hum like the
                    // murmur of m or n.
                    let len = if p.ph == Ph::V && prev.is_none() { d.min(INITIAL_V_MS) } else { d };
                    let noise = (def.af > 0.0).then(|| self.noise_for(p.ph));
                    let seg = self.add_seg(p.ph, SegKind::Plain, i, len);
                    seg.has_target = true;
                    seg.av = def.av;
                    seg.tilt = voice.tilt + def.tilt;
                    if noise.is_some() {
                        seg.af = def.af;
                        seg.noise = noise;
                    }
                    // Right after a stop or affricate the release already
                    // marks the boundary, and at a fast rate the burst ran
                    // into v's friction as into an affricate ("tvoj" heard as
                    // "zvoj"). After a voiceless fricative and before a pause
                    // v loses part of its voicing, as speakers' v does: with
                    // full voicing "svaki" was heard as "slaki" and a final v
                    // as b or m.
                    if p.ph == Ph::V {
                        if matches!(class_of(prev), PhClass::Stop | PhClass::Affricate) {
                            seg.af *= V_AFTER_STOP_AF;
                        } else if prev.is_some_and(|n| n.ph.is_obstruent() && !n.ph.def().voiced) || next.is_none() {
                            seg.av *= V_DEVOICED_AV;
                            seg.af *= V_DEVOICED_AF;
                        }
                    }
                }

                PhClass::Nasal => {
                    let seg = self.add_seg(p.ph, SegKind::Plain, i, d);
                    seg.has_target = true;
                    seg.abrupt = true;
                    seg.av = def.av;
                    seg.nasal = 1.0;
                    seg.tilt = voice.tilt + 4.0 + def.tilt;
                }

                PhClass::Tap => {
                    // /r/ is a vocalic stretch only a few dB below the vowels,
                    // interrupted by brief tongue-tip contacts of about 15 ms,
                    // each followed by a faint release transient. Between
                    // vowels there is a single contact (a tap); next to a
                    // consonant or a pause two (a short trill, period about
                    // 35 ms). A long weak stretch is heard as /l/ or /d/, a
                    // silent gap as /d/.
                    let prev_vowel = prev.is_some_and(|n| n.ph.is_vowel());
                    let next_vowel = next.is_some_and(|n| n.ph.is_vowel());
                    if p.nucleus_tail {
                        // syllabic r: one contact between its two vocoids
                        self.r_contact(i);
                    } else if prev_vowel && next_vowel && !p.word_start {
                        self.r_vocoid(i, 8.0, 0.72);
                        self.r_contact(i);
                        self.r_release(i, 12.0, 0.72);
                    } else if next_vowel && prev.is_some() && !p.word_start {
                        // after a consonant: a full vocalic onset (the
                        // svarabhakti vowel of "kra"), one contact
                        self.r_vocoid(i, 20.0, 0.74);
                        self.r_contact(i);
                        self.r_release(i, 10.0, 0.72);
                    } else if next_vowel {
                        // at the start of a word: two contacts, or the lone
                        // vocalic onset is heard as /v/ ("rastu")
                        self.r_vocoid(i, if prev_vowel { 6.0 } else { 8.0 }, 0.70);
                        self.r_contact(i);
                        self.r_release(i, 16.0, 0.70);
                        self.r_contact(i);
                        self.r_release(i, 10.0, 0.72);
                    } else {
                        // before a consonant or a pause: short trill
                        self.r_vocoid(i, if prev_vowel { 10.0 } else { 18.0 }, 0.68);
                        self.r_contact(i);
                        self.r_release(i, 18.0, 0.66);
                        self.r_contact(i);
                        let (ms, level) = if next.is_some() { (16.0, 0.62) } else { (26.0, 0.45) };
                        self.r_release(i, ms, level);
                    }
                }

                PhClass::Stop => {
                    let voiced = def.voiced;
                    let weak_release = matches!(next_cls, PhClass::Stop | PhClass::Affricate);
                    let initial = prev.is_none();
                    // BCS voiceless stops are unaspirated: short voice onset
                    // times, longest for the velar.
                    let labial = matches!(p.ph, Ph::P | Ph::B);
                    let mut release = match p.ph {
                        Ph::G => 14.0,
                        Ph::B => 12.0,
                        Ph::D if initial => D_INITIAL_RELEASE_MS,
                        _ if voiced => D_RELEASE_MS,
                        Ph::K => 34.0,
                        Ph::T => 16.0,
                        _ => 14.0,
                    };
                    // Before another stop the release is short but audible
                    // (the recorded speaker's "atka", "akta": 24-40 ms of
                    // noise 16-28 dB below the vowels).
                    if weak_release {
                        release = if voiced { 7.0 } else { 14.0 };
                    }
                    // The labial release follows synthesizer A: the click and
                    // then breath through the opening glottis, 22-28 ms of
                    // broadband noise some 30 dB below the vowel, running
                    // straight into it (16 ms when fast). A version with a
                    // silent gap between burst and voice, as in synthesizer B
                    // and the Klatt voice, was heard as a gap in the word. See
                    // "p and b once more" in docs/formant.md.
                    if !voiced && labial && !weak_release {
                        release = self.rate(24.0, 14.0);
                    }
                    release = release.min(if labial && !weak_release { f32::max(d - 12.0, 10.0) } else { d * 0.45 });
                    let mut closure = f32::max(d - release, 12.0);

                    // After friction the silent gap is the only thing that
                    // separates the stop from the noise before it: when it
                    // shrinks to 10-20 ms, "st", "sk", "št", "šk", "čk" are
                    // heard as the bare fricative. synthesizer A keeps 40 ms
                    // there (28 ms at a 60% faster rate, 12-16 ms at three
                    // times the rate), so the gap has a floor of its own that
                    // gives way to speed only slowly.
                    if !voiced && matches!(class_of(prev), PhClass::Fricative | PhClass::Affricate) {
                        closure = closure.max(self.rate(44.0, 40.0));
                    }

                    // Before a pause there is no vowel to carry the stop, so
                    // the release has to: after the burst the friction at the
                    // place of articulation dies away slowly (over 40-55 ms in
                    // the recordings, falling about 15 dB; 90 ms in
                    // synthesizer A). With only the 16 ms burst, final t and k
                    // were easy to miss. The velar's is weaker and flatter.
                    let prepausal = !voiced && next.is_none();
                    if prepausal {
                        release = self.rate(if p.ph == Ph::K { 64.0 } else { 56.0 }, 36.0);
                    }

                    // At the start of a clause nothing before the closure
                    // shows it. A voiceless one is only silence, and the word
                    // came late: 106 ms before "ce", "ka", "pe" at the default
                    // spelling speed, where synthesizer B, diphone
                    // synthesizers and the Klatt voice start the release at
                    // once. A voiced one is the voice bar alone (see below).
                    if initial {
                        closure = closure.min(if !voiced {
                            INITIAL_CLOSURE_MS
                        } else if p.ph == Ph::D {
                            D_INITIAL_BAR_MS
                        } else {
                            INITIAL_VOICE_BAR_MS
                        });
                    }

                    let noise = Some(self.noise_for(p.ph));
                    let cl = self.add_seg(p.ph, SegKind::Closure, i, closure);
                    if voiced {
                        // Voice bar: low-frequency murmur during the closure,
                        // the loud, steady buzz of synthesizer A and
                        // synthesizer C (6-20 dB below the vowels, nothing
                        // above 1 kHz), which is what makes their b stand out.
                        // synthesizer A's d and g between vowels dip just as
                        // little (7-9 dB in "ada", a recorded speaker 10 dB);
                        // with the weaker bar they had before (21 dB down) the
                        // closure of "dvadeset" was a gap that split the word.
                        cl.av = VOICE_BAR_AV;
                        cl.tilt = 22.0;
                        cl.f0_shift = -0.8;
                        if initial {
                            // A clause-initial bar is heard on its own, and
                            // drawn out by a slow rate (98 ms of "de" at the
                            // default spelling speed) with F1 ringing at its
                            // narrow vowel bandwidth it was a hum like the
                            // murmur of n: spelled d and n were hard to tell
                            // apart. A closed tract damps F1 heavily; with F1
                            // damped as in the closure of p and b the bar is a
                            // plain low hum, kept about as loud as a recorded
                            // Croatian speaker's (11-13 dB under the vowel;
                            // synthesizer A 6 dB, but duller still) and no
                            // longer than synthesizer A's 56-70 ms.
                            cl.av = INITIAL_VOICE_BAR_AV;
                            cl.tilt = INITIAL_VOICE_BAR_TILT;
                            cl.bw[0] = INITIAL_VOICE_BAR_B1;
                        }
                        if p.ph == Ph::D {
                            cl.av *= D_BAR_AV / VOICE_BAR_AV;
                        }
                    }
                    // The first formant is heavily damped once the lips close.
                    // With the vowel's 70 Hz bandwidth kept through the
                    // closure the resonator rang on for 20-25 ms after the
                    // voice stopped (4 dB per 2 ms), and the vowel faded into
                    // the stop; the classic synthesizers cut it off within
                    // 8-16 ms.
                    if labial {
                        cl.bw[0] = 250.0;
                    }
                    cl.noise = noise;

                    let rel = self.add_seg(p.ph, SegKind::Release, i, release);
                    rel.burst = (if p.ph == Ph::D { D_BURST } else { 1.0 })
                        * def.af
                        * (if voiced && !labial { 0.90 } else { 1.0 })
                        * (if !weak_release {
                            1.0
                        } else if voiced {
                            0.4
                        } else {
                            0.7
                        });
                    if prepausal {
                        rel.burst_tail = match p.ph {
                            Ph::K => 0.20,
                            Ph::T => 0.65,
                            _ => 0.50,
                        };
                        rel.tail_decay = if p.ph == Ph::K { 40.0 } else { 25.0 };
                        // Breath through the open glottis goes with it. It is
                        // what a released stop has and an affricate has not:
                        // without it "pet", "brat" leaned towards "peć", "brać".
                        rel.ah = 0.30;
                    }
                    // (A flat 18 ms labial burst as in the classic
                    // synthesizers was tried and cost a recognizer one word in
                    // ten; the burst keeps its quick fall and is 2 dB stronger
                    // instead.)
                    rel.burst_decay = match p.ph {
                        Ph::K | Ph::G => 8.0,
                        Ph::D if initial => D_INITIAL_DECAY_MS,
                        Ph::T | Ph::D => 5.0,
                        _ => 4.0,
                    };
                    rel.noise = noise;
                    if voiced {
                        // Voicing runs through the release at nearly the
                        // vowel's strength and brightness, so burst and vowel
                        // onset are one event, as in the recordings.
                        rel.av = if p.ph == Ph::D { D_RELEASE_AV } else { 0.70 };
                        rel.tilt = voice.tilt + 3.0;
                        rel.f0_shift = -0.8;
                    } else if next_vocalic || next_cls == PhClass::Tap {
                        // /r/ after a stop opens with a vocalic stretch, so
                        // the stop is released into it as into a vowel. k's
                        // breath was 0.45 until November 2026: through the
                        // cascade it spread the release over 0.5-2.5 kHz where
                        // synthesizer A's k is one peak at 1.5-2 kHz. t's
                        // breath was 0.20: with the burst at synthesizer A's
                        // level the release after it was 2-4 dB louder than
                        // synthesizer A's.
                        rel.ah = if p.ph == Ph::P { 0.32 } else { 0.15 };
                    }
                }

                PhClass::Affricate => {
                    let voiced = def.voiced;
                    // Voiceless: closure a little longer than the friction.
                    // Voiced: a short closure, then mostly friction.
                    let mut closure = f32::max(d * if voiced { 0.30 } else { 0.54 }, 12.0);
                    let mut friction = f32::max(d - closure, 15.0);
                    if self.utt.isolated_sound {
                        closure = self.rate(40.0, 20.0);
                        friction = f32::max(d - closure, 15.0);
                    }
                    // At the start of a clause, as for the stops.
                    if prev.is_none() {
                        closure = closure.min(if voiced { INITIAL_VOICE_BAR_MS } else { INITIAL_CLOSURE_MS });
                    }

                    let noise = Some(self.noise_for(p.ph));
                    let cl = self.add_seg(p.ph, SegKind::Closure, i, closure);
                    if voiced {
                        cl.av = 0.60;
                        cl.tilt = 26.0;
                        cl.f0_shift = -0.8;
                    }
                    cl.noise = noise;

                    let rel = self.add_seg(p.ph, SegKind::Release, i, friction);
                    rel.burst = def.af * if voiced { 0.50 } else { 1.0 };
                    rel.noise = noise;
                    if voiced {
                        rel.av = 0.30;
                        rel.tilt = voice.tilt + 10.0;
                        rel.f0_shift = -0.8;
                    }
                }

                PhClass::Fricative => {
                    let noise = Some(self.noise_for(p.ph));
                    let seg = self.add_seg(p.ph, SegKind::Plain, i, d);
                    seg.noise = noise;
                    let strength = if matches!(p.ph, Ph::H | Ph::Gh) { voice.h_strength } else { 1.0 };
                    if def.voiced {
                        seg.av = 0.42;
                        seg.af = def.af * (if p.ph == Ph::Z { 0.40 } else { 0.50 }) * strength;
                        seg.ah = def.ah * 0.45 * strength;
                        seg.tilt = voice.tilt + 8.0;
                        seg.f0_shift = -0.8;
                    } else {
                        seg.af = def.af * strength;
                        seg.ah = def.ah * strength;
                    }
                }

                PhClass::Silence => {}
            }

            // Singing: the rest written after this syllable's notes.
            if self.singing && next.is_none_or(|n| n.syllable != p.syllable) {
                let rest = self.sung_index(p).map_or(0.0, |s| self.sung[s].rest_ms);
                if rest >= FRAME_MS {
                    self.add_seg(Ph::Sil, SegKind::Plain, i, rest).av = 0.0;
                }
            }
        }

        let mut t = 0.0;
        for seg in &mut self.segs {
            seg.start = t;
            t += seg.dur;
        }
        self.total_ms = t;

        if self.singing {
            // Where each syllable starts and where its sound ends, for the
            // notes and their envelopes.
            let mut seen = vec![false; self.sung.len()];
            for seg in &self.segs {
                let Some(s) = self.ph.get(seg.phone as usize).and_then(|p| sung_index(p, self.sung.len())) else {
                    continue;
                };
                let syl = &mut self.sung[s];
                if !std::mem::replace(&mut seen[s], true) {
                    syl.t0 = seg.start;
                }
                if seg.ph != Ph::Sil {
                    syl.sound_end = syl.sound_end.max(seg.start + seg.dur);
                }
            }
            for i in 1..self.sung.len() {
                self.sung[i].after_rest = self.sung[i - 1].rest_ms > 0.0;
            }
        }
    }

    /// The sung syllable a phone belongs to.
    fn sung_index(&self, p: &Phone) -> Option<usize> {
        sung_index(p, self.sung.len())
    }

    // -------------------------------------------------------------------------
    // Formant targets, loci and transitions
    // -------------------------------------------------------------------------

    fn vowel_target(&self, seg: &Seg) -> [f32; 3] {
        let def = seg.ph.def();
        let p = self.phone_at(seg.phone);
        // Unstressed vowels are slightly centralized; BCS has no real vowel
        // reduction, so the effect is kept small.
        let central: f32 = if p.is_some_and(|p| p.stressed || p.vowel_word) || self.singing {
            0.0
        } else if p.is_some_and(|p| p.prominence == 0) {
            0.15
        } else {
            0.11
        };
        std::array::from_fn(|k| (def.f[k] + central * (SCHWA_F[k] - def.f[k])) * self.voice.formant_scale)
    }

    fn vowel_word_seg(&self, seg: &Seg) -> bool {
        self.phone_at(seg.phone).is_some_and(|p| p.vowel_word) && is_full_vowel(seg) && !self.singing
    }

    fn compute_formants(&mut self) {
        let n = self.segs.len();
        let scale = self.voice.formant_scale;
        let schwa = SCHWA_F.map(|f| f * scale);

        // Nearest full vowel for each segment: the following one if there is
        // any, because consonants anticipate the vowel they release into.
        let mut ctx: Vec<Option<usize>> = vec![None; n];
        let mut before: Vec<Option<usize>> = vec![None; n];
        let mut found = None;
        for i in (0..n).rev() {
            if is_full_vowel(&self.segs[i]) {
                found = Some(i);
            }
            ctx[i] = found;
        }
        found = None;
        for i in 0..n {
            before[i] = found;
            if is_full_vowel(&self.segs[i]) {
                found = Some(i);
            }
            ctx[i] = ctx[i].or(found);
        }

        // From the last segment to the first: a stop looks at what the sound
        // after it has already been given.
        for i in (0..n).rev() {
            let mut seg = self.segs[i];
            let def = seg.ph.def();
            let ci = ctx[i];
            let mut cv = match ci {
                Some(ci) if ci != i => self.vowel_target(&self.segs[ci]),
                _ => schwa,
            };

            // A stop is coloured by the sound it is released into. Before r,
            // l, j, v, a nasal or the vowel of syllabic r that is this sound,
            // not the vowel beyond it: coloured by the i of "pri", "prvi", the
            // burst of p sat at 1.7 kHz, where those of t and k are, and was
            // taken for them.
            if def.cls == PhClass::Stop
                && let Some(into) = self.segs[i + 1..].iter().find(|other| other.phone != seg.phone)
                && !is_full_vowel(into)
                && (into.has_target || into.ph.class() == PhClass::Tap)
            {
                cv = into.own;
            }

            if is_full_vowel(&seg) {
                seg.tgt = self.vowel_target(&seg);
                seg.anchor = seg.tgt;
                seg.own = seg.tgt;
                seg.coart = def.coart;
                self.segs[i] = seg;
                continue;
            }

            // Loci; the velar place follows the vowel (front before front
            // vowels, back before back vowels), with F2 and F3 close together.
            // A vowel that lies behind another consonant has no hold on it:
            // the k of "disk" is neutral (as synthesizer A's in "risk",
            // "desk"), not the fronted one of "ki", whose burst is close to a
            // /t/'s.
            let mut loc = def.loc.map(|f| f * scale);
            if matches!(seg.ph, Ph::K | Ph::G | Ph::Ng) {
                if ci.is_some_and(|ci| ci < i && self.segs[ci].phone < seg.phone - 1) {
                    cv = schwa;
                }
                loc[1] = (1500.0 * scale + (cv[1] - 700.0 * scale) * 0.55).clamp(1450.0 * scale, 2350.0 * scale);
                loc[2] = if cv[1] > 1500.0 * scale { loc[1] + 450.0 * scale } else { 2050.0 * scale };
            }

            seg.coart = def.coart;
            if seg.has_target {
                // Sonorants take colour from the vowels on both sides; /l/
                // mostly continues the vowel before it.
                let mix = match before[i] {
                    Some(bi) if ci.is_some_and(|ci| ci > i) => {
                        let pv = self.vowel_target(&self.segs[bi]);
                        let w = if seg.ph == Ph::L { 0.7 } else { 0.5 };
                        std::array::from_fn(|k| w * pv[k] + (1.0 - w) * cv[k])
                    }
                    _ => cv,
                };
                let final_l = seg.ph == Ph::L && self.phone_at(seg.phone + 1).is_none();
                for k in 0..3 {
                    let base = def.f[k] * scale;
                    let colour = if k == 0 { def.tgt_coart * 0.5 } else { def.tgt_coart };
                    seg.tgt[k] = base + colour * (mix[k] - base);
                    if k == 0 && final_l {
                        seg.tgt[0] = FINAL_L_F1 * scale;
                    }
                    if def.cls == PhClass::Nasal {
                        seg.tgt[k] = base;
                        seg.anchor[k] = loc[k];
                        seg.own[k] = loc[k] + def.coart[k] * (cv[k] - loc[k]);
                    } else {
                        seg.anchor[k] = seg.tgt[k];
                        seg.own[k] = seg.tgt[k];
                    }
                }
            } else {
                seg.anchor = loc;
                seg.own = std::array::from_fn(|k| loc[k] + def.coart[k] * (cv[k] - loc[k]));
                // The velar burst follows the vowel: before front vowels it
                // sits where F2 and F3 meet, so F3 carries a share of it and
                // the whole is weaker; before a it is the loudest.
                if let Some(noise) = seg.noise.as_mut().filter(|_| matches!(seg.ph, Ph::K | Ph::G)) {
                    let mut gain = 1.0;
                    if cv[1] > 1500.0 * scale {
                        noise.fa[1] = 0.3;
                        gain = VELAR_FRONT_BURST;
                    } else if cv[1] > 1100.0 * scale {
                        gain = VELAR_CENTRAL_BURST;
                    }
                    for share in &mut noise.fa {
                        *share *= gain;
                    }
                }
            }
            self.segs[i] = seg;
        }

        // The vowel after nj leaves the palatal locus slowly, with F1 low and
        // F2 high for its first milliseconds, as the tongue body comes off the
        // palate (voice D's "nja": F2 near 2.3 kHz, F1 rising over 35 ms).
        // Leaving it fast, as after n, nj was heard as n; a separate j there,
        // with the high F3 of a real j, was heard as j.
        for i in 1..n {
            if self.segs[i - 1].ph == Ph::Nj && self.segs[i].ph.is_vowel() {
                self.segs[i].slow_onset = true;
            }
        }

        // Boundary values and transition times.
        for seg in &mut self.segs {
            seg.lb = seg.own;
            seg.rb = seg.own;
        }
        for i in 0..n.saturating_sub(1) {
            let (mut x, mut y) = (self.segs[i], self.segs[i + 1]);
            for k in 0..3 {
                let speedup = if k == 0 { 0.6 } else { 1.0 };
                if x.rank == y.rank {
                    let v = 0.5 * (x.own[k] + y.own[k]);
                    x.rb[k] = v;
                    y.lb[k] = v;
                    // A word of one vowel holds its target between the two
                    // glides (ja i ona).
                    x.tr[k] = (if self.vowel_word_seg(&x) { 0.25 } else { 0.45 }) * x.dur;
                    y.tl[k] = (if self.vowel_word_seg(&y) { 0.25 } else { 0.45 }) * y.dur;
                    continue;
                }
                let x_dominates = x.rank > y.rank;
                let (dom, other) = if x_dominates { (&x, &y) } else { (&y, &x) };
                let mut v = dom.anchor[k] + dom.coart[k] * (other.own[k] - dom.anchor[k]);
                let mut t_other = dom.tr_out * speedup;
                let t_dom = f32::min(0.45 * dom.dur, 35.0);
                // The tongue tip makes its contact for /l/ quickly and lets go
                // slowly: in the recordings F2 drops into an l within 25 ms
                // and takes 60-80 ms to leave it.
                if !x_dominates && y.ph == Ph::L {
                    t_other *= 0.5;
                }
                // The mouth opens fast when a stop is released: by the end of
                // the burst F1 is more than half way to the next sound (490 Hz
                // at the voice onset of the recorded "pa", "apa" against
                // 330 Hz here), and the rest follows within a few
                // milliseconds. With F1 held low the vowel swelled over
                // 20-30 ms and p, b, d sounded soft.
                if k == 0 && x_dominates && x.kind == SegKind::Release && x.ph.class() == PhClass::Stop {
                    // After p and b the vowel snaps in, as in synthesizer B
                    // and the Klatt voice (full level within 6-8 ms).
                    let labial = matches!(x.ph, Ph::P | Ph::B);
                    let snap = if labial {
                        0.72
                    } else if x.ph == Ph::D {
                        D_F1_SNAP
                    } else {
                        0.55
                    };
                    v = x.anchor[0] + snap * (y.own[0] - x.anchor[0]);
                    t_other = t_other.min(self.transition_rate(if labial { 8.0 } else { 12.0 }));
                }
                // The lips close as fast as they open: F1 reaches the closure
                // of p and b within about 14 ms (synthesizer A 16 ms,
                // synthesizer B 10-20, the Klatt voice within a frame).
                // Gliding down over 36 ms, the vowel faded into the stop
                // instead of being cut off by it. (Applied to every stop it
                // did nothing for p and b and cost a recognizer a few t and k
                // words, so the other stops keep their transitions.)
                if k == 0 && !x_dominates && y.kind == SegKind::Closure && matches!(y.ph, Ph::P | Ph::B) {
                    t_other = t_other.min(self.transition_rate(14.0));
                }
                x.rb[k] = v;
                y.lb[k] = v;
                (x.tr[k], y.tl[k]) = if x_dominates { (t_dom, t_other) } else { (t_other, t_dom) };
            }
            self.segs[i] = x;
            self.segs[i + 1] = y;
        }
        for seg in &mut self.segs {
            for k in 0..3 {
                if seg.abrupt {
                    seg.lb[k] = seg.tgt[k];
                    seg.rb[k] = seg.tgt[k];
                }
                let sum = seg.tl[k] + seg.tr[k];
                let room = seg.dur * 0.95;
                if sum > room && sum > 0.0 {
                    seg.tl[k] *= room / sum;
                    seg.tr[k] *= room / sum;
                }
            }
        }

        // A rest between notes is silence, not a sound to glide towards: the
        // vowel before it holds its colour to the end.
        for i in 0..n {
            if self.segs[i].ph != Ph::Sil {
                continue;
            }
            if i > 0 {
                let x = &mut self.segs[i - 1];
                x.rb = if x.has_target { x.tgt } else { x.own };
                x.tr = [0.0; 3];
                self.segs[i].lb = self.segs[i - 1].rb;
            }
            if i + 1 < n {
                let y = &mut self.segs[i + 1];
                y.lb = if y.has_target { y.tgt } else { y.own };
                y.tl = [0.0; 3];
                self.segs[i].rb = self.segs[i + 1].lb;
            }
        }
    }

    // -------------------------------------------------------------------------
    // Control tracks
    // -------------------------------------------------------------------------

    fn build_tracks(&mut self) {
        let voice = self.voice;
        let utt = self.utt;
        let style = voice.singing.filter(|_| self.singing);
        let lead = LEAD_FRAMES; // frames of silence before speech
        // Let the resonators ring out; a hall needs longer to fall silent,
        // but not so long that the next clause comes in late.
        let trail = if style.is_some_and(|style| style.reverb > 0.0) { frame_of(250.0) } else { 14 };
        let body = frame_of(self.total_ms).max(1);
        let total = lead + body + trail;
        let count = total as usize;

        let mut f = SCHWA_F.map(|schwa| vec![schwa; count]);
        let mut bw = [(); 3].map(|()| vec![100f32; count]);
        let mut f4 = vec![0f32; count];
        let (mut av, mut ah, mut af, mut burst) =
            (vec![0f32; count], vec![0f32; count], vec![0f32; count], vec![0f32; count]);
        let mut tilt = vec![voice.tilt; count];
        let mut nasal = vec![0f32; count];
        let mut level = vec![1f32; count];
        self.seg_of_frame = vec![-1; count];
        let mut noise_seg = vec![-1i32; count];

        // Loudness: stressed syllables stand out a little, the level drifts
        // down along the clause and drops on the last syllable of a statement.
        let last_syl = utt.syllable_count - 1;
        let falling_end = utt.kind == ClauseKind::Statement;

        let nsegs = self.segs.len();
        for (si, seg) in self.segs.iter().enumerate() {
            let j0 = lead + frame_of(seg.start);
            let j1 = (lead + frame_of(seg.start + seg.dur)).min(lead + body);
            let p = self.phone_at(seg.phone);
            let last_seg = si == nsegs - 1;

            for j in j0..j1 {
                let idx = j as usize;
                let t = (((j - lead) as f32 + 0.5) * FRAME_MS - seg.start).clamp(0.0, seg.dur);
                self.seg_of_frame[idx] = si as i32;

                for k in 0..3 {
                    f[k][idx] = formant_at(seg, k, t);
                    bw[k][idx] = seg.bw[k];
                }
                f4[idx] = seg.f4;
                av[idx] = seg.av;
                // The voice bar of b, d, g weakens over the second half of the
                // closure (by 10-15 dB in the recordings, 7 dB here), so the
                // release stands out against it.
                if seg.kind == SegKind::Closure && seg.av > 0.0 && seg.ph.class() == PhClass::Stop && seg.dur > 0.0 {
                    let fade = if seg.ph == Ph::B { 0.35 } else { 0.55 };
                    av[idx] *= 1.0 - fade * (2.0 * t / seg.dur - 1.0).clamp(0.0, 1.0);
                    if seg.ph == Ph::D {
                        // ... and that of d fades again just before the release
                        let pre = self.rate(D_BAR_FADE_MS, D_BAR_FADE_MS * 0.6).min(seg.dur * 0.5);
                        av[idx] *= 1.0 - D_BAR_FADE * ((t - (seg.dur - pre)) / pre).clamp(0.0, 1.0);
                    }
                }
                ah[idx] = seg.ah;
                // A release into a pause dies away; its last few milliseconds
                // close it off.
                let mut tail = 0.0;
                if seg.burst_tail > 0.0 {
                    tail = (-t / seg.tail_decay).exp() * f32::min(1.0, (seg.dur - t) / 8.0);
                    ah[idx] *= tail;
                }
                af[idx] = seg.af;
                tilt[idx] = seg.tilt;
                nasal[idx] = seg.nasal;

                if seg.burst > 0.0 {
                    burst[idx] = if seg.burst_decay > 0.0 {
                        seg.burst * (-t / seg.burst_decay).exp().max(seg.burst_tail * tail)
                    } else {
                        // affricate friction: abrupt start, short fall with
                        // the transient of the stop release in front
                        let fall = f32::min(12.0, seg.dur * 0.3);
                        let remain = seg.dur - t;
                        let attack = if t < 6.0 { AFFRICATE_ATTACK } else { 1.0 };
                        seg.burst * attack * if remain < fall { remain / fall } else { 1.0 }
                    };
                }
                if seg.noise.is_some() {
                    noise_seg[idx] = si as i32;
                }

                let mut gain = 1.0;
                if !self.singing {
                    gain = 1.0 - 0.12 * (seg.start + t) / self.total_ms.max(1.0);
                    if falling_end && utt.syllable_count > 1 && p.is_some_and(|p| i32::from(p.syllable) == last_syl) {
                        gain *= 0.84;
                        tilt[idx] += 3.0;
                    }
                    // A vowel before a pause fades instead of stopping dead.
                    if last_seg && seg.av > 0.0 && seg.dur > 0.0 {
                        let x = t / seg.dur;
                        if x > 0.5 {
                            gain *= 1.0 - 0.75 * (x - 0.5) / 0.5;
                        }
                    }
                }
                level[idx] = gain;
            }
        }

        // Vowels across a word boundary ("video uradak", "moju ulicu"): the
        // voice dips briefly at the join, as at a soft glottal onset. This
        // also keeps one-vowel words distinct next to /j/. No extra time is
        // added, and vowels within a word and sung legato stay joined.
        if !self.singing {
            for pair in self.segs.windows(2) {
                let (x, y) = (&pair[0], &pair[1]);
                if x.kind != SegKind::Plain || y.kind != SegKind::Plain {
                    continue;
                }
                let (Some(px), Some(py)) = (self.phone_at(x.phone), self.phone_at(y.phone)) else { continue };
                if !needs_vowel_separation(px, py) {
                    continue;
                }
                let half = 0.5 * self.rate(VOWEL_BOUNDARY_DIP_MS, VOWEL_BOUNDARY_DIP_MIN_MS);
                let j0 = lead + frame_of(y.start - half);
                let j1 = lead + frame_of(y.start + half);
                for j in j0.max(0)..j1.min(total) {
                    av[j as usize] *= VOWEL_BOUNDARY_DIP;
                }
            }
        }

        // Frames outside any segment keep the nearest segment's vocal tract.
        let (first, last) = (lead as usize, (lead + body - 1) as usize);
        for k in 0..3 {
            let value = f[k][first];
            f[k][..first].fill(value);
            let value = f[k][last];
            f[k][last + 1..].fill(value);
            let value = bw[k][last];
            bw[k][last + 1..].fill(value);
        }
        let value = f4[first];
        f4[..first].fill(value);
        let value = f4[last];
        f4[last + 1..].fill(value);

        for k in 0..3 {
            smooth(&mut f[k], 2);
            smooth(&mut bw[k], 3);
        }
        smooth(&mut f4, 6);
        smooth(&mut av, 2);
        smooth(&mut ah, 3);
        // Friction starts and stops at the edge of its sound (within a few
        // milliseconds), not gradually across it: sharp edges are a large part
        // of what makes consonants easy to tell apart.
        smooth(&mut af, 3);
        // No noise may spill into the closure of a voiceless stop: the
        // smoothing would take 6 ms off each end of a silent gap that is short
        // already.
        let seg_at = |j: usize| usize::try_from(self.seg_of_frame[j]).ok().map(|si| &self.segs[si]);
        let closure_at = |j: usize| seg_at(j).is_some_and(|seg| seg.kind == SegKind::Closure);
        for j in 0..count {
            if seg_at(j).is_some_and(|seg| seg.kind == SegKind::Closure && !seg.voiced) {
                af[j] = 0.0;
                ah[j] = 0.0;
            }
        }
        // The voice bar of b, d, g is heavily muffled. Smoothed like the rest,
        // that muffling reached 8 ms past the release and dulled the very
        // start of the next sound; from the release on, the tilt is smoothed
        // as if the closure had the release's own.
        let mut open_tilt = tilt.clone();
        let mut releases = Vec::new();
        for pair in self.segs.windows(2) {
            let (seg, rel) = (&pair[0], &pair[1]);
            if seg.kind != SegKind::Closure || seg.av <= 0.0 {
                continue;
            }
            let j0 = lead + frame_of(seg.start);
            let j1 = (lead + frame_of(rel.start)).min(total);
            for j in j0..j1 {
                open_tilt[j as usize] = rel.tilt;
            }
            releases.push(j1..(j1 + 4).min(total));
        }
        smooth(&mut tilt, 4);
        smooth(&mut open_tilt, 4);
        for j in releases.into_iter().flatten() {
            tilt[j as usize] = open_tilt[j as usize];
        }
        // Nasality reaches a little into the neighbouring vowels, and no
        // further: spread over 16 ms each way it blurred the edges of m and n,
        // which synthesizer A and synthesizer B switch within a few
        // milliseconds.
        smooth(&mut nasal, 3);
        smooth(&mut level, 4);

        // Frication spectrum. A segment's friction is smoothed past its end
        // (smooth(af, 3) above), so its filter bank must stay in place that
        // long: until November 2026 the frames after a fricative took the next
        // noisy segment's spectrum at once, and the tail of every s, š or f
        // before a labial went through the labial burst's bank (a peak at
        // 350 Hz and a flat share), the voiced tail of z before b likewise.
        // The hold covers frames without a noisy segment of their own and the
        // start of a closure, which injects nothing before its release, but
        // never the two frames before another noisy segment, so that a burst
        // finds its own bank in place.
        {
            let own = noise_seg.clone();
            let noisy_at = |j: i32| j >= 0 && j < total && own[j as usize] >= 0 && !closure_at(j as usize);
            let mut held = -1;
            let mut held_for = 0;
            for j in 0..total {
                if noisy_at(j) {
                    held = own[j as usize];
                    held_for = 0;
                    continue;
                }
                if held < 0 {
                    continue;
                }
                let next_is_other = (1..=2).any(|k| noisy_at(j + k) && own[(j + k) as usize] != held);
                if held_for < NOISE_HOLD_FRAMES && !next_is_other {
                    noise_seg[j as usize] = held;
                    held_for += 1;
                } else {
                    held = -1;
                }
            }
        }
        // Silent frames take the spectrum of the next noisy segment, so the
        // filter bank is already in place at a burst; those after the last
        // one keep its spectrum.
        let mut upcoming = -1;
        for ns in noise_seg.iter_mut().rev() {
            if *ns >= 0 { upcoming = *ns } else { *ns = upcoming }
        }
        let mut previous = -1;
        for ns in &mut noise_seg {
            if *ns >= 0 { previous = *ns } else { *ns = previous }
        }

        // The note envelopes below still count time from frame 0 (`self.lead`
        // is set after them), 8 ms late; kept, since the reference recordings
        // were made that way.
        self.frames = (0..count)
            .map(|idx| {
                let mut fr = Frame::default();
                for k in 0..3 {
                    fr.f[k] = f[k][idx];
                    fr.b[k] = bw[k][idx];
                }
                // Keep formants ordered and apart even in extreme contexts.
                fr.f[1] = fr.f[1].max(fr.f[0] + 200.0);
                fr.f[2] = fr.f[2].max(fr.f[1] + 250.0);
                fr.f[3] = f4[idx].max(fr.f[2] + 300.0);
                fr.b[3] = if self.singing { 180.0 } else { SPEAKING_F4_BANDWIDTH };
                fr.av = av[idx] * level[idx];
                fr.ah = ah[idx] * level[idx];
                fr.af = (af[idx] + burst[idx]) * level[idx];
                fr.tilt = tilt[idx];
                if self.singing {
                    let (env, extra_tilt) = self.note_envelope(idx);
                    fr.av *= env;
                    fr.ah *= env;
                    fr.tilt += extra_tilt;
                }
                fr.nasal = nasal[idx];
                fr.oq = voice.open_quotient;

                if let Some(spec) = usize::try_from(noise_seg[idx]).ok().and_then(|ns| self.segs[ns].noise) {
                    fr.pa = spec.fa;
                    fr.np_f = spec.f;
                    fr.np_b = spec.b;
                    fr.np_a = spec.a;
                    fr.bypass = spec.bypass;
                }
                fr
            })
            .collect();
        self.lead = lead;
    }

    // -------------------------------------------------------------------------
    // Singing: envelope and pitch of the notes
    // -------------------------------------------------------------------------

    /// Time of a frame from the start of the clause (ms).
    fn frame_time(&self, j: usize) -> f32 {
        ((j as i32 - self.lead) as f32 + 0.5) * FRAME_MS
    }

    /// The sung syllable a frame belongs to, `None` outside speech.
    fn syllable_of_frame(&self, j: usize) -> Option<usize> {
        let si = usize::try_from(self.seg_of_frame[j]).ok()?;
        self.sung_index(self.phone_at(self.segs[si].phone)?)
    }

    fn note_at(&self, s: usize, t: f32) -> NotePos {
        let syl = &self.sung[s];
        let t_syl = t - syl.t0;
        let beat_ms = self.beat_ms();
        let mut at = 0.0;
        let mut midi = 60;
        for (i, note) in syl.notes.iter().enumerate() {
            midi = i32::from(note.midi);
            let len = note.beats * beat_ms;
            if t_syl < at + len || i + 1 == syl.notes.len() {
                break;
            }
            at += len;
        }
        NotePos { midi, t_syl, remaining: syl.sound_end - t, in_rest: t >= syl.sound_end }
    }

    /// Attack, release and (for a plucked string) decay of a note, and the
    /// extra tilt of a decaying string. Notes that follow each other without
    /// a rest are joined (legato): the attack is only for the first note
    /// after a rest or a pause, or the voice would dip to nothing between
    /// every two syllables.
    fn note_envelope(&self, j: usize) -> (f32, f32) {
        let (Some(s), Some(style)) = (self.syllable_of_frame(j), self.voice.singing) else { return (1.0, 0.0) };
        let pos = self.note_at(s, self.frame_time(j));
        let syl = &self.sung[s];
        let mut env = style.gain;
        let mut extra_tilt = 0.0;
        if style.attack_ms > 0.0 && syl.after_rest {
            env *= (pos.t_syl / style.attack_ms).clamp(0.0, 1.0);
        }
        let last = s + 1 == self.sung.len();
        if syl.rest_ms > 0.0 || last {
            // Into a rest with the style's release; at the end of the clause
            // a short one, so the last vowel does not stop dead.
            let release = if syl.rest_ms > 0.0 { style.release_ms } else { style.release_ms.min(40.0) };
            if release > 0.0 {
                env *= (pos.remaining / release).clamp(0.0, 1.0);
            }
        }
        if style.decay_ms > 0.0 {
            let mut since_pluck = pos.t_syl;
            if style.retrigger_beats > 0.0 {
                let period = style.retrigger_beats * 60000.0 / (style.song.bpm * self.tempo);
                since_pluck = pos.t_syl % period;
            }
            let decay = (-since_pluck / style.decay_ms).exp();
            env *= decay.max(0.02);
            extra_tilt = 9.0 * (1.0 - decay);
        }
        (env, extra_tilt)
    }

    /// Transposition of a dry preset: the middle of the song's range five
    /// semitones above the voice's speaking pitch (a voice sings a little
    /// higher than it speaks; a song an octave wide then runs from about a
    /// semitone below the speaking pitch to an octave above it), in whole
    /// semitones so the key stays in tune with itself.
    fn natural_transpose(&self) -> f32 {
        let pitches = || self.melody.iter().flat_map(|melody| melody.notes).filter(|note| note.midi > 0);
        let (Some(low), Some(high)) = (pitches().map(|n| n.midi).min(), pitches().map(|n| n.midi).max()) else {
            return 0.0;
        };
        let centre = 0.5 * (i32::from(low) + i32::from(high)) as f32;
        let target = 69.0 + 12.0 * (self.voice.base_f0 / 440.0).log2() + 5.0;
        (target - centre).round()
    }

    fn build_sung_pitch(&mut self) {
        let Some(style) = self.voice.singing else { return };
        let count = self.frames.len();
        let transpose = style.transpose + if style.dry { self.natural_transpose() } else { 0.0 };
        let mut last = 60.0 + transpose - 69.0;
        let mut note_t = vec![0f32; count];
        // Semitones above A4.
        let mut st: Vec<f32> = (0..count)
            .map(|j| {
                if let Some(s) = self.syllable_of_frame(j) {
                    let pos = self.note_at(s, self.frame_time(j));
                    if !pos.in_rest {
                        last = pos.midi as f32 + transpose - 69.0;
                    }
                    note_t[j] = pos.t_syl;
                }
                last
            })
            .collect();

        // Portamento: the glide between notes, zero-lag.
        smooth_both_ways(&mut st, (-FRAME_MS / style.portamento_ms.max(1.0)).exp());

        // Vibrato: the inflection slider sets its depth, 0 switches it off.
        let depth = style.vibrato_cents * (self.params.inflection_level.clamp(0.0, 1.0) / INFLECTION_LEVEL_DEFAULT);
        let base = self.params.pitch * self.params.user_pitch;
        let two_pi = 2.0 * std::f32::consts::PI;
        let mut phase = 0f32;
        for j in 0..count {
            let t = self.frame_time(j);
            let onset =
                if style.vibrato_delay_ms > 0.0 { (note_t[j] / style.vibrato_delay_ms).clamp(0.0, 1.0) } else { 1.0 };
            // A singer's vibrato rate wanders by a few percent; a perfectly
            // even one sounds mechanical (Sundberg).
            let rate = style.vibrato_hz * (1.0 + 0.03 * (two_pi * 0.41 * t / 1000.0).sin());
            phase += two_pi * rate * FRAME_MS / 1000.0;
            let vib = depth * onset * onset * phase.sin();
            let hz = 440.0 * base * pow2f((st[j] + vib / 100.0) / 12.0);
            self.frames[j].f0 = hz.clamp(45.0, 1000.0);
        }
    }

    // -------------------------------------------------------------------------
    // Intonation
    // -------------------------------------------------------------------------

    fn collect_syllables(&self) -> Vec<Syllable> {
        let mut syls = vec![Syllable::default(); self.utt.syllable_count.max(0) as usize];
        for seg in &self.segs {
            let Some(p) = self.phone_at(seg.phone).filter(|p| p.nucleus || p.nucleus_tail) else { continue };
            let Some(syl) = usize::try_from(p.syllable).ok().and_then(|s| syls.get_mut(s)) else { continue };
            if syl.t0 < 0.0 || seg.start < syl.t0 {
                syl.t0 = seg.start;
            }
            syl.t1 = syl.t1.max(seg.start + seg.dur);
            syl.word = i32::from(p.word);
            syl.prominence = p.prominence;
            if p.nucleus && p.stressed {
                syl.stressed = true;
                syl.accent = p.accent;
            }
        }
        syls
    }

    fn build_pitch(&mut self) {
        if self.singing {
            return self.build_sung_pitch();
        }
        let count = self.frames.len();
        let intonation = Intonation::new(self.utt, &self.collect_syllables(), self.total_ms, self.voice.pitch_range);
        // The melody, and the segmental effects on pitch.
        let mut st: Vec<f32> = (0..count)
            .map(|j| {
                let shift = usize::try_from(self.seg_of_frame[j]).map_or(0.0, |si| self.segs[si].f0_shift);
                intonation.at(self.frame_time(j)) + shift
            })
            .collect();

        // Smooth the contour (forward and backward for zero lag): the larynx
        // cannot follow corners.
        smooth_both_ways(&mut st, (-FRAME_MS / 16.0).exp());

        // Pitch starts higher after a voiceless consonant and settles quickly.
        for pair in self.segs.windows(2) {
            let (before, seg) = (&pair[0], &pair[1]);
            if seg.av <= 0.0 || before.voiced || before.av > 0.0 {
                continue;
            }
            let j0 = self.lead + frame_of(seg.start);
            for j in j0..(j0 + 30).min(count as i32) {
                st[j as usize] += 0.9 * (-((j - j0) as f32 * FRAME_MS) / 18.0).exp();
            }
        }

        // Inflection level: 0.5 keeps the movements as measured, 0 flattens
        // the contour to a monotone (segmental effects included), 1 doubles
        // every movement.
        let level = self.params.inflection_level.clamp(0.0, 1.0) / INFLECTION_LEVEL_DEFAULT;
        let base = self.voice.base_f0 * self.params.pitch * self.params.user_pitch;
        for (frame, st) in self.frames.iter_mut().zip(st) {
            frame.f0 = (base * pow2f(level * st / 12.0)).clamp(45.0, 480.0);
        }
    }
}

fn sung_index(p: &Phone, count: usize) -> Option<usize> {
    usize::try_from(p.syllable).ok().filter(|&s| s < count)
}

fn is_full_vowel(seg: &Seg) -> bool {
    seg.kind == SegKind::Plain && seg.ph.is_vowel() && seg.ph != Ph::Schwa
}

/// A letter's sound alone (spelling by sounds): a vowel letter as a stressed
/// syllable; a consonant letter as the consonant and a neutral vowel after
/// it, as synthesizer B's Croatian voice sounds the letters out (measured at
/// its default rate: 70-90 ms of consonant, 110-130 ms of vowel at the level
/// of a full vowel).
fn isolated_duration(p: &Phone, def: &PhDef) -> f32 {
    if p.ph == Ph::Schwa {
        return 120.0; // the release vowel
    }
    match def.cls {
        PhClass::Vowel => 190.0,
        PhClass::Fricative => 90.0,
        PhClass::Nasal | PhClass::Liquid | PhClass::Glide => 80.0,
        PhClass::Stop => def.dur * 1.2,
        PhClass::Affricate => ISOLATED_AFFRICATE_MS,
        PhClass::Tap | PhClass::Silence => def.dur,
    }
}

/// Formant `k` of a segment at `t` ms into it.
fn formant_at(seg: &Seg, k: usize, t: f32) -> f32 {
    if !seg.has_target {
        let x = if seg.dur > 0.0 { t / seg.dur } else { 0.0 };
        return seg.lb[k] + (seg.rb[k] - seg.lb[k]) * x;
    }
    if t < seg.tl[k] {
        if seg.slow_onset {
            // leaving nj: slow at first, then smoothly into the target
            let u = t / seg.tl[k];
            return seg.lb[k] + (seg.tgt[k] - seg.lb[k]) * u * u * (3.0 - 2.0 * u);
        }
        // leaving a consonant: fast at first, settling into the target
        let x = 1.0 - t / seg.tl[k];
        return seg.tgt[k] + (seg.lb[k] - seg.tgt[k]) * x * x;
    }
    let tail = seg.dur - seg.tr[k];
    if t > tail && seg.tr[k] > 0.0 {
        // approaching a consonant: slow at first, fastest at the closure
        let x = (t - tail) / seg.tr[k];
        return seg.tgt[k] + (seg.rb[k] - seg.tgt[k]) * x * x;
    }
    seg.tgt[k]
}
