//! Cascade/parallel formant synthesizer (Klatt-style): turns a stream of
//! control frames into audio samples.
//!
//! Voiced sound and aspiration go through a cascade of resonators (nasal
//! pole/zero pair followed by eight formants). Frication and plosive bursts
//! go through parallel band-pass resonators: three that follow F2-F4, so a
//! burst carries the same formant movement as the vowel it releases into, and
//! three at fixed frequencies for sibilant noise. The voicing source is a
//! KLGLOTT88-style glottal pulse (flow derivative) with a band-limited closing
//! edge, spectral tilt, jitter, shimmer and flutter.
//!
//! The arithmetic keeps the precision of every intermediate value (`f32`
//! parameters, `f64` state) so that the same text gives the same samples as
//! the reference recordings in `tests/golden`.

use crate::SAMPLE_RATE;
use crate::math::{pow2, pow10};
use std::f64::consts::PI;

/// One control frame every 44 samples (~2 ms at 22050 Hz). Parameters are
/// interpolated between frames, so tracks only need to be this fine.
pub const FRAME_SAMPLES: usize = 44;
pub const FRAME_MS: f32 = 1000.0 * FRAME_SAMPLES as f32 / SAMPLE_RATE as f32;

/// Fixed-frequency frication peaks (sibilants).
pub const NOISE_PEAKS: usize = 3;
/// Frication shaped by F2, F3, F4 (bursts, /h/).
pub const PARALLEL_FORMANTS: usize = 3;
/// F1..F4 move, the rest are fixed per voice.
pub const CASCADE_VARIABLE: usize = 4;
/// F5..F8.
pub const CASCADE_FIXED: usize = 4;

const T: f64 = 1.0 / SAMPLE_RATE as f64;
const MAX_FREQ: f64 = SAMPLE_RATE as f64 * 0.46;

// Diplophonia at full roughness: the odd periods come this much sooner (as a
// share of the period) and this much weaker than the even ones. (0.05 and 0.6
// until a listener found 75 and 100 too faint on a phone in November 2026.)
const ROUGHNESS_PERIOD_ALTERNATION: f64 = 0.10;
const ROUGHNESS_AMPLITUDE_DIP: f64 = 0.8;

// The breathiness setting (VoiceQuality::whisper above 50, tension below).
// At full whisper the glottis stays open for most of the cycle, the source is
// steeper by WHISPER_TILT_DB at 3 kHz, the voicing is gone (it falls with
// whisper², so 75 is a breathy voice and not yet a whisper) and breath noise
// of WHISPER_BREATH times the voicing amplitude goes through the cascade in
// its place, unmodulated by the glottal cycle. At full tension the glottis
// closes for longer (TENSE_OQ) and the source is flatter by TENSE_TILT_DB, a
// pressed voice whose H1-H2 is lower (Klatt & Klatt 1990). Measured values in
// "Breathiness, from a tense voice to a whisper" in docs/formant.md.
const WHISPER_OQ: f64 = 0.85;
const WHISPER_TILT_DB: f64 = 10.0;
const WHISPER_BREATH: f64 = 0.6;
const WHISPER_B1_HZ: f64 = 150.0;
const TENSE_OQ: f64 = 0.30; // 0.40 and 4 dB until November 2026 (inaudible)
const TENSE_TILT_DB: f64 = 8.0;
const FRICATION_GAIN: f64 = 3.0;

/// Coefficients are refreshed twice per frame.
const SUBFRAME: usize = FRAME_SAMPLES / 2;

const RNG_SEED: u32 = 0x2545_F491;

// Frication noise is band-limited before it reaches the parallel bank: two
// second-order sections (fourth-order Butterworth) at 8 kHz, for every voice;
// the ceiling below is the voiced path's alone. See "The hiss where the ear
// is sharpest" in docs/formant.md.
const FRICATION_CUTOFF: f64 = 8000.0;
const FRICATION_LP_Q: [f64; 2] = [0.541_196_10, 1.306_562_96];

// The voiced path of the singing presets gets a gentle second-order roll-off;
// the speaking voices run through the 6th-order ceiling instead.
const VOICE_CUTOFF: f64 = 6000.0;
const VOICE_LP_Q: f64 = 0.65;

// The speaking voices' ceiling (VoiceQuality::ceiling_hz): a 6th-order
// Butterworth on the voiced path. At 5300 Hz it is -0.2 dB at 4.45 kHz, -3 at
// 5.3, -15 at 6.3 and -44 at 8 kHz.
// Written to eight places, as the reference recordings were made.
#[allow(clippy::approx_constant)]
const SPEECH_CEILING_Q: [f64; 3] = [0.517_638_09, 0.707_106_78, 1.931_851_65];

/// Fixed frication peaks from here up have zeros at 0 Hz and Nyquist.
const NOISE_ZERO_MIN_HZ: f64 = 1000.0;

/// Bandwidths of the parallel formants F2-F4 (wider than in the cascade).
const PARALLEL_BW: [f64; PARALLEL_FORMANTS] = [190.0, 260.0, 360.0];

fn lerp(a: impl Into<f64>, b: impl Into<f64>, t: f64) -> f64 {
    let a = a.into();
    a + (b.into() - a) * t
}

/// Control frame: everything the synthesizer needs for one instant.
/// Amplitudes are linear (0 = off, 1 = nominal full level).
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Frame {
    /// Fundamental frequency (Hz).
    pub f0: f32,
    /// Voicing amplitude.
    pub av: f32,
    /// Aspiration amplitude (noise through the cascade).
    pub ah: f32,
    /// Frication amplitude (noise through the parallel bank).
    pub af: f32,
    pub f: [f32; CASCADE_VARIABLE],
    pub b: [f32; CASCADE_VARIABLE],
    /// Nasal coupling 0..1 (separates the zero from the pole).
    pub nasal: f32,
    /// Extra source spectral tilt (dB down at 3 kHz).
    pub tilt: f32,
    /// Glottal open quotient.
    pub oq: f32,
    /// Frication share at F2, F3, F4.
    pub pa: [f32; PARALLEL_FORMANTS],
    /// Fixed frication peaks.
    pub np_f: [f32; NOISE_PEAKS],
    pub np_b: [f32; NOISE_PEAKS],
    pub np_a: [f32; NOISE_PEAKS],
    /// Flat (unfiltered) frication share.
    pub bypass: f32,
}

impl Default for Frame {
    fn default() -> Self {
        Self {
            f0: 110.0,
            av: 0.0,
            ah: 0.0,
            af: 0.0,
            f: [500.0, 1500.0, 2500.0, 3500.0],
            b: [60.0, 90.0, 150.0, 250.0],
            nasal: 0.0,
            tilt: 0.0,
            oq: 0.6,
            pa: [0.0; PARALLEL_FORMANTS],
            np_f: [2500.0, 4000.0, 6000.0],
            np_b: [500.0, 800.0, 1200.0],
            np_a: [0.0; NOISE_PEAKS],
            bypass: 0.0,
        }
    }
}

/// What the voiced sound source is. The speaking voices use the glottal
/// pulse; the singing presets replace it with an instrument. Every source
/// goes through the same cascade vocal tract, so the words stay audible.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum SourceKind {
    /// KLGLOTT88 pulse: the speaking voice.
    #[default]
    Glottal,
    /// Pipe organ: a steady stack of partials over a 16' sub-octave.
    Organ,
    /// Bowed strings: sawtooth.
    Strings,
    /// Sawtooth that saturates as it gets louder (lip buzz).
    Brass,
    /// Accordion: sawtooth plus a second reed a few cents sharp.
    Reed,
}

/// Per-voice constants of the vocal tract and voice source.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct VoiceQuality {
    /// F5 sits where synthesizer A's does (a fixed resonance near 3.9 kHz in
    /// every vowel), the rest are spread up to the voiced-path cut-off.
    pub upper_f: [f32; CASCADE_FIXED],
    /// F5 is widened for speech in `quality_for()`.
    pub upper_b: [f32; CASCADE_FIXED],
    pub nasal_pole: f32,
    /// Zero position at full coupling (Hz).
    pub nasal_zero: f32,
    /// Source high-frequency emphasis (0..0.9); 0.55 since the match to the
    /// reference balance. See "synthesizer A's balance" in docs/formant.md.
    pub brightness: f32,
    /// The reference balance: a low shelf of `bass_db` on the voicing from
    /// `bass_hz` down (0 = none).
    pub bass_db: f32,
    pub bass_hz: f32,
    /// A 6th-order ceiling on the voiced path (0 = the voice low-pass
    /// instead, which the singing presets keep).
    pub ceiling_hz: f32,
    /// Glottal noise mixed into the voicing.
    pub breathiness: f32,
    /// Slow quasi-random F0 drift (0..1).
    pub flutter: f32,
    /// Period-to-period F0 perturbation.
    pub jitter: f32,
    /// Period-to-period amplitude perturbation.
    pub shimmer: f32,
    /// Diplophonia 0..1 (the roughness setting above 50): every second period
    /// shorter and weaker, a subharmonic an octave under the voice.
    pub roughness: f32,
    /// The breathiness setting above 50, 0..1: 1 is a whisper.
    pub whisper: f32,
    /// The breathiness setting below 50, 0..1: a pressed, clear voice.
    pub tension: f32,
    // Singing presets. chorus adds two copies of the source detuned by
    // chorus_cents up and down (a section of players, a klapa); sub_octave
    // adds one an octave down (the bass singer, the 16' organ stop); reverb
    // is the wet level of a small hall.
    pub source: SourceKind,
    pub chorus: f32,
    pub chorus_cents: f32,
    pub sub_octave: f32,
    pub reverb: f32,
}

impl Default for VoiceQuality {
    fn default() -> Self {
        Self {
            upper_f: [3950.0, 4900.0, 6000.0, 7200.0],
            upper_b: [500.0, 420.0, 700.0, 900.0],
            nasal_pole: 270.0,
            nasal_zero: 450.0,
            brightness: 0.55,
            bass_db: 7.0,
            bass_hz: 230.0,
            ceiling_hz: 5300.0,
            breathiness: 0.02,
            flutter: 0.2,
            jitter: 0.004,
            shimmer: 0.03,
            roughness: 0.0,
            whisper: 0.0,
            tension: 0.0,
            source: SourceKind::Glottal,
            chorus: 0.0,
            chorus_cents: 8.0,
            sub_octave: 0.0,
            reverb: 0.0,
        }
    }
}

/// Pole radius and angle of a second-order section at `freq` with `bw`.
fn pole(freq: f64, bw: f64) -> (f64, f64) {
    ((-PI * bw.max(20.0) * T).exp(), 2.0 * PI * freq.clamp(50.0, MAX_FREQ) * T)
}

/// Two-pole resonator.
#[derive(Clone, Copy, Debug)]
struct Resonator {
    a: f64,
    b: f64,
    c: f64,
    y1: f64,
    y2: f64,
}

impl Default for Resonator {
    fn default() -> Self {
        Self { a: 1.0, b: 0.0, c: 0.0, y1: 0.0, y2: 0.0 }
    }
}

impl Resonator {
    /// Unity gain at DC.
    fn set(&mut self, freq: impl Into<f64>, bw: impl Into<f64>) {
        let (r, theta) = pole(freq.into(), bw.into());
        self.c = -r * r;
        self.b = 2.0 * r * theta.cos();
        self.a = 1.0 - self.b - self.c;
    }

    /// Unity gain at the resonance peak.
    fn set_bandpass(&mut self, freq: f64, bw: f64) {
        let (r, theta) = pole(freq, bw);
        self.c = -r * r;
        self.b = 2.0 * r * theta.cos();
        self.a = (1.0 - r) * (1.0 - 2.0 * r * (2.0 * theta).cos() + r * r).sqrt();
    }

    fn tick(&mut self, x: f64) -> f64 {
        let y = self.a * x + self.b * self.y1 + self.c * self.y2;
        self.y2 = self.y1;
        self.y1 = y;
        y
    }

    fn clear(&mut self) {
        (self.y1, self.y2) = (0.0, 0.0);
    }
}

/// A fixed frication peak. Above 1 kHz it is a true band-pass, with zeros at
/// 0 Hz and at the Nyquist frequency: an all-pole resonator at 4.5 kHz passes
/// low frequencies only 17 dB below its peak, and the c of "ce" carried a
/// rumble 15 dB under its hiss. The low labial peak keeps its all-pole skirt.
#[derive(Clone, Copy, Debug)]
struct NoisePeak {
    a: f64,
    z: f64,
    b: f64,
    c: f64,
    x1: f64,
    x2: f64,
    y1: f64,
    y2: f64,
}

impl Default for NoisePeak {
    fn default() -> Self {
        Self { a: 1.0, z: 0.0, b: 0.0, c: 0.0, x1: 0.0, x2: 0.0, y1: 0.0, y2: 0.0 }
    }
}

impl NoisePeak {
    fn set(&mut self, freq: f64, bw: f64) {
        let (r, theta) = pole(freq, bw);
        self.c = -r * r;
        self.b = 2.0 * r * theta.cos();
        // Unity gain at the resonance peak, with or without the zeros.
        let peak = (1.0 - r) * (1.0 - 2.0 * r * (2.0 * theta).cos() + r * r).sqrt();
        if freq.clamp(50.0, MAX_FREQ) >= NOISE_ZERO_MIN_HZ {
            self.z = 1.0;
            self.a = peak / (2.0 * theta.sin());
        } else {
            self.z = 0.0;
            self.a = peak;
        }
    }

    fn tick(&mut self, x: f64) -> f64 {
        let y = self.a * (x - self.z * self.x2) + self.b * self.y1 + self.c * self.y2;
        self.x2 = self.x1;
        self.x1 = x;
        self.y2 = self.y1;
        self.y1 = y;
        y
    }

    fn clear(&mut self) {
        (self.x1, self.x2, self.y1, self.y2) = (0.0, 0.0, 0.0, 0.0);
    }
}

/// Two-zero antiresonator (the nasal zero).
#[derive(Clone, Copy, Debug)]
struct AntiResonator {
    a: f64,
    b: f64,
    c: f64,
    x1: f64,
    x2: f64,
}

impl Default for AntiResonator {
    fn default() -> Self {
        Self { a: 1.0, b: 0.0, c: 0.0, x1: 0.0, x2: 0.0 }
    }
}

impl AntiResonator {
    fn set(&mut self, freq: f64, bw: f64) {
        let (r, theta) = pole(freq, bw);
        let rc = -r * r;
        let rb = 2.0 * r * theta.cos();
        let ra = 1.0 - rb - rc;
        self.a = 1.0 / ra;
        self.b = -rb / ra;
        self.c = -rc / ra;
    }

    fn tick(&mut self, x: f64) -> f64 {
        let y = self.a * x + self.b * self.x1 + self.c * self.x2;
        self.x2 = self.x1;
        self.x1 = x;
        y
    }

    fn clear(&mut self) {
        (self.x1, self.x2) = (0.0, 0.0);
    }
}

/// A general second-order section.
#[derive(Clone, Copy, Debug)]
struct Biquad {
    b0: f64,
    b1: f64,
    b2: f64,
    a1: f64,
    a2: f64,
    x1: f64,
    x2: f64,
    y1: f64,
    y2: f64,
}

impl Default for Biquad {
    fn default() -> Self {
        Self { b0: 1.0, b1: 0.0, b2: 0.0, a1: 0.0, a2: 0.0, x1: 0.0, x2: 0.0, y1: 0.0, y2: 0.0 }
    }
}

impl Biquad {
    /// The Audio EQ Cookbook's low shelf with the slope 1: `db` from `freq`
    /// down, half of it at `freq`, nothing an octave and a half above.
    fn set_low_shelf(&mut self, freq: f64, db: f64) {
        let a = pow10(db / 40.0);
        let w0 = 2.0 * PI * freq * T;
        let cw = w0.cos();
        let sa = 2.0 * a.sqrt() * w0.sin() / 2.0 * 2f64.sqrt();
        let a0 = (a + 1.0) + (a - 1.0) * cw + sa;
        self.b0 = a * ((a + 1.0) - (a - 1.0) * cw + sa) / a0;
        self.b1 = 2.0 * a * ((a - 1.0) - (a + 1.0) * cw) / a0;
        self.b2 = a * ((a + 1.0) - (a - 1.0) * cw - sa) / a0;
        self.a1 = -2.0 * ((a - 1.0) + (a + 1.0) * cw) / a0;
        self.a2 = ((a + 1.0) + (a - 1.0) * cw - sa) / a0;
    }

    fn tick(&mut self, x: f64) -> f64 {
        let y = self.b0 * x + self.b1 * self.x1 + self.b2 * self.x2 - self.a1 * self.y1 - self.a2 * self.y2;
        self.shift(x, y)
    }

    fn shift(&mut self, x: f64, y: f64) -> f64 {
        self.x2 = self.x1;
        self.x1 = x;
        self.y2 = self.y1;
        self.y1 = y;
        y
    }

    fn clear(&mut self) {
        (self.x1, self.x2, self.y1, self.y2) = (0.0, 0.0, 0.0, 0.0);
    }
}

/// Second-order low-pass (`b2 == b0`, summed before the multiplication).
#[derive(Clone, Copy, Debug, Default)]
struct LowPass(Biquad);

impl LowPass {
    fn set(&mut self, freq: f64, q: f64) {
        let w = 2.0 * PI * freq * T;
        let alpha = w.sin() / (2.0 * q);
        let cw = w.cos();
        let a0 = 1.0 + alpha;
        let s = &mut self.0;
        s.b0 = (1.0 - cw) * 0.5 / a0;
        s.b1 = (1.0 - cw) / a0;
        s.a1 = -2.0 * cw / a0;
        s.a2 = (1.0 - alpha) / a0;
    }

    fn tick(&mut self, x: f64) -> f64 {
        let s = &mut self.0;
        let y = s.b0 * (x + s.x2) + s.b1 * s.x1 - s.a1 * s.y1 - s.a2 * s.y2;
        s.shift(x, y)
    }

    fn clear(&mut self) {
        self.0.clear();
    }
}

/// Schroeder reverberator: four parallel combs into two allpasses.
#[derive(Clone, Debug)]
struct Reverb {
    comb: [Vec<f32>; 4],
    allpass: [Vec<f32>; 2],
    comb_pos: [usize; 4],
    allpass_pos: [usize; 2],
    comb_state: [f64; 4],
}

impl Default for Reverb {
    fn default() -> Self {
        // Freeverb's tuning, halved for 22050 Hz.
        Self {
            comb: [558, 594, 639, 678].map(|n| vec![0.0; n]),
            allpass: [113, 278].map(|n| vec![0.0; n]),
            comb_pos: [0; 4],
            allpass_pos: [0; 2],
            comb_state: [0.0; 4],
        }
    }
}

impl Reverb {
    fn clear(&mut self) {
        *self = Self::default();
    }

    fn tick(&mut self, x: f64) -> f64 {
        const FEEDBACK: f64 = 0.82;
        const DAMP: f64 = 0.3;
        const ALLPASS_G: f64 = 0.5;
        let mut out = 0.0;
        for ((line, pos), state) in self.comb.iter_mut().zip(&mut self.comb_pos).zip(&mut self.comb_state) {
            let y = f64::from(line[*pos]);
            *state = y * (1.0 - DAMP) + *state * DAMP;
            line[*pos] = (x + *state * FEEDBACK) as f32;
            *pos = (*pos + 1) % line.len();
            out += y;
        }
        out *= 0.25;
        for (line, pos) in self.allpass.iter_mut().zip(&mut self.allpass_pos) {
            let y = f64::from(line[*pos]);
            line[*pos] = (out + y * ALLPASS_G) as f32;
            out = y - out;
            *pos = (*pos + 1) % line.len();
        }
        out
    }
}

/// KLGLOTT88 glottal flow derivative over one period (phase 0..1) with the
/// closing edge (a step of +1 at `ph == oq`) band-limited over one sample.
fn glottal_pulse(ph: f64, oq: f64, dt: f64) -> f64 {
    let mut g = 0.0;
    if ph < oq {
        let x = ph / oq;
        g = 2.0 * x - 3.0 * x * x;
    }
    let d = ph - oq;
    if d >= 0.0 && d < dt {
        let u = 1.0 - d / dt;
        g -= 0.5 * u * u;
    } else if d < 0.0 && d > -dt {
        let u = 1.0 + d / dt;
        g += 0.5 * u * u;
    }
    g
}

/// Sawtooth with the discontinuity smoothed by a polynomial (PolyBLEP), so it
/// does not alias at the high notes.
fn sawtooth(ph: f64, dt: f64) -> f64 {
    let mut y = 2.0 * ph - 1.0;
    if ph < dt {
        let x = ph / dt;
        y -= x + x - x * x - 1.0;
    } else if ph > 1.0 - dt {
        let x = (ph - 1.0) / dt;
        y -= x * x + x + x + 1.0;
    }
    y
}

fn advance(phase: &mut f64, dt: f64) {
    *phase += dt;
    if *phase >= 1.0 {
        *phase -= 1.0;
    }
}

/// Cascade/parallel formant synthesizer.
#[derive(Clone, Debug)]
pub struct KlattSynth {
    quality: VoiceQuality,
    prev: Option<Frame>,

    cascade: [Resonator; CASCADE_VARIABLE],
    upper: [Resonator; CASCADE_FIXED],
    nasal_pole: Resonator,
    nasal_zero: AntiResonator,
    noise_bank: [NoisePeak; NOISE_PEAKS],
    parallel: [Resonator; PARALLEL_FORMANTS],
    fric_lp: [LowPass; 2],
    voice_lp: LowPass,
    /// The low shelf on the voicing (`VoiceQuality::bass_db`).
    bass: Option<Biquad>,
    /// The 6th-order ceiling (`VoiceQuality::ceiling_hz`).
    ceiling: Option<[LowPass; 3]>,

    // Voice source state. The extra oscillators carry the chorus copies
    // (up, down) and the sub-octave.
    phase: f64,
    phase_up: f64,
    phase_down: f64,
    phase_sub: f64,
    reverb: Reverb,
    period_jitter: f64,
    period_shimmer: f64,
    period_oq: f64,
    /// Parity of the glottal period (diplophonia).
    period_odd: bool,
    tilt_state: f64,
    tilt_coef: f64,
    bright_x1: f64,
    time: f64,

    // Output conditioning.
    dc_x1: f64,
    dc_y1: f64,

    rng: u32,
}

impl Default for KlattSynth {
    fn default() -> Self {
        Self::new(&VoiceQuality::default())
    }
}

impl KlattSynth {
    pub fn new(quality: &VoiceQuality) -> Self {
        let mut synth = Self {
            quality: *quality,
            prev: None,
            cascade: Default::default(),
            upper: Default::default(),
            nasal_pole: Default::default(),
            nasal_zero: Default::default(),
            noise_bank: Default::default(),
            parallel: Default::default(),
            fric_lp: Default::default(),
            voice_lp: Default::default(),
            bass: None,
            ceiling: None,
            phase: 0.0,
            phase_up: 0.0,
            phase_down: 0.0,
            phase_sub: 0.0,
            reverb: Reverb::default(),
            period_jitter: 1.0,
            period_shimmer: 1.0,
            period_oq: 0.6,
            period_odd: false,
            tilt_state: 0.0,
            tilt_coef: 0.0,
            bright_x1: 0.0,
            time: 0.0,
            dc_x1: 0.0,
            dc_y1: 0.0,
            rng: 0,
        };
        synth.set_quality(quality);
        synth
    }

    /// Sets the voice's constants. The filter state is kept; call
    /// [`reset`](Self::reset) at the start of an utterance.
    pub fn set_quality(&mut self, quality: &VoiceQuality) {
        self.quality = *quality;
        let q = &self.quality;
        for (resonator, (&f, &b)) in self.upper.iter_mut().zip(q.upper_f.iter().zip(&q.upper_b)) {
            resonator.set(f, b);
        }
        self.nasal_pole.set(q.nasal_pole, 90.0);
        self.nasal_zero.set(q.nasal_pole.into(), 90.0);
        for (lp, &lp_q) in self.fric_lp.iter_mut().zip(&FRICATION_LP_Q) {
            lp.set(FRICATION_CUTOFF, lp_q);
        }
        self.voice_lp.set(VOICE_CUTOFF, VOICE_LP_Q);
        // The sections keep their state across a change of quality, as the
        // filters above do.
        self.bass = (q.bass_db > 0.05 && q.bass_hz > 0.0).then(|| {
            let mut shelf = self.bass.unwrap_or_default();
            shelf.set_low_shelf(q.bass_hz.into(), q.bass_db.into());
            shelf
        });
        self.ceiling = (q.ceiling_hz > 0.0).then(|| {
            let mut sections = self.ceiling.unwrap_or_default();
            for (section, &section_q) in sections.iter_mut().zip(&SPEECH_CEILING_Q) {
                section.set(q.ceiling_hz.into(), section_q);
            }
            sections
        });
    }

    /// Resets all filter and oscillator state (start of an utterance).
    pub fn reset(&mut self) {
        self.cascade.iter_mut().for_each(Resonator::clear);
        self.upper.iter_mut().for_each(Resonator::clear);
        self.noise_bank.iter_mut().for_each(NoisePeak::clear);
        self.parallel.iter_mut().for_each(Resonator::clear);
        self.fric_lp.iter_mut().for_each(LowPass::clear);
        self.voice_lp.clear();
        self.bass.iter_mut().for_each(Biquad::clear);
        self.ceiling.iter_mut().flatten().for_each(LowPass::clear);
        self.nasal_pole.clear();
        self.nasal_zero.clear();
        (self.phase, self.phase_up, self.phase_down, self.phase_sub) = (0.0, 0.0, 0.0, 0.0);
        self.reverb.clear();
        self.period_jitter = 1.0;
        self.period_shimmer = 1.0;
        self.period_odd = false;
        self.tilt_state = 0.0;
        self.bright_x1 = 0.0;
        (self.dc_x1, self.dc_y1) = (0.0, 0.0);
        self.prev = None;
        // Same text, same audio: restart the noise and flutter sequences too.
        self.time = 0.0;
        self.rng = RNG_SEED;
    }

    /// Sum of four uniform values: cheap, close enough to Gaussian.
    fn noise(&mut self) -> f64 {
        let mut sum = 0.0;
        for _ in 0..4 {
            self.rng = self.rng.wrapping_mul(1_664_525).wrapping_add(1_013_904_223);
            sum += f64::from(self.rng >> 8) * (1.0 / 8_388_608.0) - 1.0;
        }
        sum * 0.5
    }

    fn update_coefficients(&mut self, fr: &Frame) {
        let q = self.quality;
        for (i, resonator) in self.cascade.iter_mut().enumerate() {
            resonator.set(fr.f[i], fr.b[i]);
        }
        // An open glottis couples the trachea to the tract: F1's bandwidth
        // widens in a breathy voice and a whisper, which also keeps the noise
        // from ringing in a narrow F1.
        if q.whisper > 0.0 {
            self.cascade[0].set(fr.f[0], f64::from(fr.b[0]) + f64::from(q.whisper) * WHISPER_B1_HZ);
        }

        let zero =
            f64::from(q.nasal_pole) + f64::from(fr.nasal).clamp(0.0, 1.0) * f64::from(q.nasal_zero - q.nasal_pole);
        self.nasal_zero.set(zero, 90.0);

        for (i, peak) in self.noise_bank.iter_mut().enumerate() {
            peak.set(fr.np_f[i].into(), fr.np_b[i].into());
        }
        for (i, resonator) in self.parallel.iter_mut().enumerate() {
            resonator.set_bandpass(fr.f[i + 1].into(), PARALLEL_BW[i]);
        }

        // One-pole low-pass giving `tilt` dB of attenuation at 3 kHz.
        let mut tilt = f64::from(fr.tilt);
        if q.whisper > 0.0 {
            tilt += f64::from(q.whisper) * WHISPER_TILT_DB;
        }
        if q.tension > 0.0 {
            tilt -= f64::from(q.tension) * TENSE_TILT_DB;
        }
        let tilt = tilt.clamp(0.0, 40.0);
        self.tilt_coef = if tilt < 0.05 {
            0.0
        } else {
            let g2 = pow10(-tilt / 10.0);
            let cw = (2.0 * PI * 3000.0 * T).cos();
            let p = 1.0 - g2 * cw;
            let q = 1.0 - g2;
            (p - (p * p - q * q).max(0.0).sqrt()) / q
        };
    }

    /// One sample of the voiced source and whether the glottis is open.
    fn source_sample(&mut self, f0: f64, oq_now: f64, av: f64) -> (f64, bool) {
        let q = self.quality;
        let dt = f0 * T;
        let oq = self.period_oq;
        let up = pow2(f64::from(q.chorus_cents) / 1200.0);
        let down = 1.0 / up;
        let (chorus, sub_octave) = (f64::from(q.chorus), f64::from(q.sub_octave));

        let g = match q.source {
            SourceKind::Glottal => {
                let mut g = glottal_pulse(self.phase, oq, dt);
                if q.chorus > 0.0 {
                    g += chorus
                        * (glottal_pulse(self.phase_up, oq, dt * up) + glottal_pulse(self.phase_down, oq, dt * down));
                }
                if q.sub_octave > 0.0 {
                    g += sub_octave * glottal_pulse(self.phase_sub, oq, dt * 0.5);
                }
                g
            }
            SourceKind::Organ => {
                // Stops over the 16' phase: 16', 8', 4', 2 2/3', 2', 1 3/5',
                // 1 1/3', 1'. As in a classic desktop organ voice the
                // fundamental is weak and the energy sits on the fourth to
                // eighth harmonics, where the upper stops of a principal
                // chorus put it.
                const HARMONIC: [f64; 8] = [1.0, 2.0, 4.0, 6.0, 8.0, 10.0, 12.0, 16.0];
                const LEVEL: [f64; 8] = [0.0, 0.45, 0.90, 0.70, 0.80, 0.40, 0.35, 0.22];
                let mut g = 0.0;
                for k in 0..8 {
                    let level = if k == 0 { sub_octave } else { LEVEL[k] };
                    if level > 0.0 {
                        g += level * (2.0 * PI * HARMONIC[k] * self.phase_sub).sin();
                    }
                }
                g * 0.30
            }
            SourceKind::Strings | SourceKind::Brass | SourceKind::Reed => {
                let mut g = sawtooth(self.phase, dt);
                if q.chorus > 0.0 {
                    g += chorus * sawtooth(self.phase_up, dt * up);
                    // An accordion's second reed is only tuned sharp.
                    if q.source != SourceKind::Reed {
                        g += chorus * sawtooth(self.phase_down, dt * down);
                    }
                }
                if q.sub_octave > 0.0 {
                    g += sub_octave * sawtooth(self.phase_sub, dt * 0.5);
                }
                if q.source == SourceKind::Brass {
                    // The lips saturate: the louder the note, the brighter.
                    let drive = 1.0 + 4.0 * av.clamp(0.0, 1.0);
                    g = (drive * g).tanh() / drive.tanh();
                }
                g * 0.45
            }
        };

        let open_phase = self.phase < oq;

        advance(&mut self.phase_up, dt * up);
        advance(&mut self.phase_down, dt * down);
        advance(&mut self.phase_sub, dt * 0.5);
        self.phase += dt;
        if self.phase >= 1.0 {
            self.phase -= 1.0;
            self.period_odd = !self.period_odd;
            self.period_jitter = 1.0 + f64::from(q.jitter) * self.noise() * 2.0;
            self.period_shimmer = 1.0 + f64::from(q.shimmer) * self.noise() * 2.0;
            if q.roughness > 0.0 && self.period_odd {
                self.period_jitter *= 1.0 + ROUGHNESS_PERIOD_ALTERNATION * f64::from(q.roughness);
                self.period_shimmer *= 1.0 - ROUGHNESS_AMPLITUDE_DIP * f64::from(q.roughness);
            }
            let mut oq_next = oq_now;
            if q.whisper > 0.0 {
                oq_next += f64::from(q.whisper) * (WHISPER_OQ - oq_next);
            }
            if q.tension > 0.0 {
                oq_next += f64::from(q.tension) * (TENSE_OQ - oq_next);
            }
            self.period_oq = oq_next.clamp(0.3, 0.9);
        }
        (g, open_phase)
    }

    /// Renders frames and appends the samples to `out`. The output is
    /// nominally within [-1, 1] for normal speech levels.
    pub fn render(&mut self, frames: &[Frame], out: &mut Vec<f32>) {
        out.reserve(frames.len() * FRAME_SAMPLES);
        let q = self.quality;

        for next in frames {
            let prev = *self.prev.get_or_insert_with(|| {
                self.period_oq = next.oq.into();
                *next
            });

            for half in 0..2 {
                // Filter coefficients for this half-frame, taken at its midpoint.
                let tm = ((half * SUBFRAME) as f64 + SUBFRAME as f64 * 0.5) / FRAME_SAMPLES as f64;
                let mid_of = |a: f32, b: f32| lerp(a, b, tm) as f32;
                let mut mid = Frame::default();
                for i in 0..CASCADE_VARIABLE {
                    mid.f[i] = mid_of(prev.f[i], next.f[i]);
                    mid.b[i] = mid_of(prev.b[i], next.b[i]);
                }
                for i in 0..NOISE_PEAKS {
                    mid.np_f[i] = mid_of(prev.np_f[i], next.np_f[i]);
                    mid.np_b[i] = mid_of(prev.np_b[i], next.np_b[i]);
                }
                mid.nasal = mid_of(prev.nasal, next.nasal);
                mid.tilt = mid_of(prev.tilt, next.tilt);
                self.update_coefficients(&mid);

                for s in 0..SUBFRAME {
                    let t = (half * SUBFRAME + s + 1) as f64 / FRAME_SAMPLES as f64;

                    let f0 = lerp(prev.f0, next.f0, t);
                    let av = lerp(prev.av, next.av, t);
                    let ah = lerp(prev.ah, next.ah, t);
                    let af = lerp(prev.af, next.af, t);

                    // Slow pitch drift so sustained voicing never sounds machine-flat.
                    let tt = self.time * T;
                    let flutter = 1.0
                        + f64::from(q.flutter)
                            * 0.012
                            * ((2.0 * PI * 12.7 * tt).sin()
                                + (2.0 * PI * 7.1 * tt).sin()
                                + (2.0 * PI * 4.7 * tt).sin());
                    self.time += 1.0;

                    // ---- Voice source ----
                    let f0_eff = f0.clamp(40.0, 1000.0) * flutter * self.period_jitter;
                    let (g, open_phase) = self.source_sample(f0_eff, lerp(prev.oq, next.oq, t), av);

                    self.tilt_state = (1.0 - self.tilt_coef) * g + self.tilt_coef * self.tilt_state;
                    // The bare pulse falls 6 dB/octave, duller than a real
                    // modal voice above 2 kHz; a first-order emphasis restores
                    // the upper formants that carry consonant transitions.
                    let k = f64::from(q.brightness);
                    let mut bright = (self.tilt_state - k * self.bright_x1) / (1.0 - k);
                    self.bright_x1 = self.tilt_state;
                    // The low shelf lifts the voicing alone: the breath and
                    // the frication stay flat (bassy breath made the stop
                    // releases thumps when a low-pass on the aspiration was
                    // tried).
                    if let Some(shelf) = &mut self.bass {
                        bright = shelf.tick(bright);
                    }
                    let mut voice = bright * av * self.period_shimmer;

                    // ---- Aspiration and breath noise (through the cascade) ----
                    let n1 = self.noise();
                    let mut breath = f64::from(q.breathiness) * av;
                    let mut asp_mod = if av > 0.01 && !open_phase { 0.35 } else { 1.0 };
                    if q.whisper > 0.0 {
                        // The breathiness setting above 50: voicing out, breath in.
                        let w = f64::from(q.whisper);
                        voice *= 1.0 - w * w;
                        breath += w * WHISPER_BREATH * av;
                        if asp_mod < 1.0 {
                            asp_mod += w * (1.0 - asp_mod);
                        }
                    }
                    let source = voice + n1 * (ah + breath) * asp_mod;

                    // ---- Cascade vocal tract ----
                    let mut y = self.nasal_pole.tick(self.nasal_zero.tick(source));
                    for resonator in self.upper.iter_mut().rev().chain(self.cascade.iter_mut().rev()) {
                        y = resonator.tick(y);
                    }
                    // The speaking voices' ceiling is the voiced path's alone
                    // (it was on the whole output until the s was heard as a
                    // slight š: see FRICATION_CUTOFF).
                    y = match &mut self.ceiling {
                        Some(sections) => sections.iter_mut().fold(y, |y, section| section.tick(y)),
                        None => self.voice_lp.tick(y),
                    };

                    // ---- Parallel frication branch ----
                    // The filters are heard to the end of their ringing:
                    // dropped as soon as the noise stopped, the hiss before a
                    // stop ("st", "šk") was cut off from a tenth of its level
                    // to nothing.
                    let mut src = 0.0;
                    if af > 1e-5 || f64::from(prev.af) > 1e-5 || f64::from(next.af) > 1e-5 {
                        let n2 = self.noise();
                        // Voiced fricatives: noise is pulsed by the glottal cycle.
                        let fmod = if av > 0.05 && !open_phase { 0.45 } else { 1.0 };
                        src = n2 * af * fmod * FRICATION_GAIN;
                    }
                    let [first, second] = &mut self.fric_lp;
                    let src = second.tick(first.tick(src));
                    let mut fric = 0.0;
                    let mut sign = 1.0;
                    for (i, resonator) in self.parallel.iter_mut().enumerate() {
                        fric += sign * lerp(prev.pa[i], next.pa[i], t) * resonator.tick(src);
                        sign = -sign;
                    }
                    for (i, peak) in self.noise_bank.iter_mut().enumerate() {
                        fric += sign * lerp(prev.np_a[i], next.np_a[i], t) * peak.tick(src);
                        sign = -sign;
                    }
                    fric += lerp(prev.bypass, next.bypass, t) * src;

                    let mut sample = y + fric;
                    if q.reverb > 0.0 {
                        sample += f64::from(q.reverb) * self.reverb.tick(sample);
                    }

                    // DC blocker (~35 Hz)
                    let hp = sample - self.dc_x1 + 0.99 * self.dc_y1;
                    self.dc_x1 = sample;
                    self.dc_y1 = hp;

                    out.push(hp as f32);
                }
            }

            self.prev = Some(*next);
        }
    }
}
