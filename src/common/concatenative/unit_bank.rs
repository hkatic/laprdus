//! Analysed recordings of a concatenative voice.
//!
//! The recorded voices (Josip, Vlado) are one recording per sound. Before
//! they can be joined, stretched or sung at another pitch, each recording is
//! analysed once, when the voice is loaded:
//!
//! - the DC offset is removed and the raw edges of the file are faded, so a
//!   recording that was cut in the middle of a period cannot click;
//! - a pitch track (normalised autocorrelation, with the voice's median pitch
//!   as a prior against octave errors) decides which parts are voiced;
//! - voiced parts get one pitch mark per period, at the main peak of the
//!   low-passed waveform (the glottal closure); unvoiced parts get marks
//!   every few milliseconds;
//! - the sounding part of the recording (lead and trail silence) is found,
//!   and the sharpest rise in level, which in a stop is the burst that must
//!   survive whatever the rate does to the rest.
//!
//! The methods are the classic ones of pitch-synchronous synthesis (Moulines
//! & Charpentier 1990). The PSOLA renderer works from these marks only.

use super::voice_data::{Sound, VoiceData};
use crate::SAMPLE_RATE;
use std::f32::consts::PI;

/// Spacing of the marks in unvoiced sound (samples at 22050 Hz, 5 ms).
pub const UNVOICED_HOP: i32 = 110;

/// Both ends of every recording are faded over this many samples (2 ms).
pub const EDGE_FADE: i32 = 44;

// Pitch search range of the recorded voices.
const F0_MIN: f32 = 60.0;
const F0_MAX: f32 = 400.0;

// Pitch track.
const TRACK_HOP: i32 = UNVOICED_HOP; // 5 ms
const CORR_LENGTH: i32 = 331; // 15 ms of correlation
const VOICED_CORRELATION: f32 = 0.60; // normalised autocorrelation of voiced sound
const VOICED_LEVEL: f32 = 0.05; // of the loudest frame

/// The sounding part: -34 dB below the loudest block.
const SOUNDING_LEVEL: f32 = 0.02;

/// One analysed recording.
#[derive(Clone, Debug, Default)]
pub struct Unit {
    /// -1..1, DC-free, edges faded, formant-warped.
    pub samples: Vec<f32>,
    /// Analysis marks, ascending.
    pub marks: Vec<i32>,
    /// Local period at each mark (samples).
    pub period: Vec<i32>,
    /// Whether the mark lies in voiced sound.
    pub voiced: Vec<bool>,
    /// First sounding sample.
    pub lead: i32,
    /// One past the last sounding sample.
    pub trail: i32,
    /// Sharpest rise in level: the burst of a stop.
    pub onset: i32,
    /// Median pitch of the voiced marks (Hz), 0 if unvoiced.
    pub f0: f32,
    /// Level of the sounding part.
    pub rms: f32,
    pub loaded: bool,
}

impl Unit {
    pub fn length(&self) -> i32 {
        self.samples.len() as i32
    }

    pub fn has_voicing(&self) -> bool {
        self.f0 > 0.0
    }

    pub fn sounding_length(&self) -> i32 {
        self.trail - self.lead
    }
}

fn median(mut v: Vec<f32>) -> f32 {
    if v.is_empty() {
        return 0.0;
    }
    v.sort_by(f32::total_cmp);
    let n = v.len();
    if n % 2 == 1 { v[n / 2] } else { 0.5 * (v[n / 2 - 1] + v[n / 2]) }
}

/// Sum of squares of a stretch, in double precision.
fn energy(x: &[f32]) -> f64 {
    x.iter().fold(0.0, |e, &v| e + f64::from(v) * f64::from(v))
}

/// Resamples by a factor (`output[n] = x(n * ratio)`) with a windowed sinc,
/// band-limited to the narrower of the two Nyquist frequencies.
fn resample(x: &[f32], ratio: f32) -> Vec<f32> {
    const HALF_TAPS: i32 = 16;
    let n_in = x.len() as i32;
    if n_in == 0 || (ratio - 1.0).abs() < 1e-4 {
        return x.to_vec();
    }
    let n_out = (((n_in - 1) as f32 / ratio).floor() as i32 + 1).max(1);
    let cutoff = 0.5 * f32::min(1.0, 1.0 / ratio); // cycles per input sample
    (0..n_out)
        .map(|n| {
            let pos = n as f32 * ratio;
            let centre = pos.floor() as i32;
            let (mut sum, mut norm) = (0f32, 0f32);
            for k in centre - HALF_TAPS + 1..=centre + HALF_TAPS {
                let d = pos - k as f32;
                let arg = 2.0 * PI * cutoff * d;
                let sinc = if arg.abs() < 1e-6 { 1.0 } else { arg.sin() / arg };
                // Hann window over the taps
                let w = 0.5 * (1.0 + (PI * d / HALF_TAPS as f32).cos());
                let h = 2.0 * cutoff * sinc * w;
                norm += h;
                if (0..n_in).contains(&k) {
                    sum += h * x[k as usize];
                }
            }
            if norm > 1e-6 { sum / norm } else { 0.0 }
        })
        .collect()
}

/// Second-order low-pass, run forwards and backwards (zero phase).
fn lowpass_zero_phase(x: &[f32], cutoff_hz: f32, fs: f32) -> Vec<f32> {
    let w0 = 2.0 * PI * cutoff_hz / fs;
    let (cw, sw) = (w0.cos(), w0.sin());
    // Q to four places, as the reference recordings were made.
    #[allow(clippy::approx_constant)]
    let alpha = sw / (2.0 * 0.7071);
    let a0 = 1.0 + alpha;
    let b0 = (1.0 - cw) / 2.0 / a0;
    let b1 = (1.0 - cw) / a0;
    let b2 = b0;
    let a1 = -2.0 * cw / a0;
    let a2 = (1.0 - alpha) / a0;

    let run = |v: &mut [f32]| {
        let (mut x1, mut x2, mut y1, mut y2) = (0f32, 0f32, 0f32, 0f32);
        for s in v {
            let x0 = *s;
            let y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            (x2, x1, y2, y1) = (x1, x0, y1, y0);
            *s = y0;
        }
    };
    let mut y = x.to_vec();
    run(&mut y);
    y.reverse();
    run(&mut y);
    y.reverse();
    y
}

#[derive(Clone, Copy, Default)]
struct Frame {
    centre: i32,
    /// Best period (samples), 0 if none.
    lag: i32,
    rms: f32,
    voiced: bool,
}

/// Normalised cross-correlation of the signal around `centre` at one lag.
fn ncc_at(x: &[f32], centre: i32, lag: i32) -> f32 {
    let at = |i: i32| usize::try_from(i).ok().and_then(|i| x.get(i)).map_or(0.0, |&v| f64::from(v));
    let start = centre - CORR_LENGTH / 2;
    let (mut num, mut e1, mut e2) = (0f64, 0f64, 0f64);
    for a in start..start + CORR_LENGTH {
        let (xa, xb) = (at(a), at(a + lag));
        num += xa * xb;
        e1 += xa * xa;
        e2 += xb * xb;
    }
    let den = (e1 * e2).sqrt();
    if den > 1e-12 { (num / den) as f32 } else { 0.0 }
}

/// The pitch search range (periods in samples).
fn lag_range(fs: f32) -> (i32, i32) {
    (((fs / F0_MAX).floor() as i32).max(2), (fs / F0_MIN).ceil() as i32)
}

/// Pitch track of one recording. With a prior, the search is narrowed to
/// about an octave around it and a candidate near the prior wins over a
/// slightly better one far from it (octave errors).
fn pitch_track(x: &[f32], fs: f32, f0_prior: f32) -> Vec<Frame> {
    let n = x.len() as i32;
    let (mut lag_min, mut lag_max) = lag_range(fs);
    let mut prior_lag = 0;
    if f0_prior > 0.0 {
        prior_lag = (fs / f0_prior).round() as i32;
        lag_min = lag_min.max((prior_lag as f32 / 1.6) as i32);
        lag_max = lag_max.min((prior_lag as f32 * 1.6) as i32);
    }

    let mut loudest = 0f32;
    let mut frames: Vec<Frame> = (TRACK_HOP / 2..n)
        .step_by(TRACK_HOP as usize)
        .map(|centre| {
            let window = &x[(centre - TRACK_HOP / 2).max(0) as usize..(centre + TRACK_HOP / 2).min(n) as usize];
            let rms = if window.is_empty() { 0.0 } else { (energy(window) / window.len() as f64).sqrt() as f32 };
            loudest = loudest.max(rms);
            Frame { centre, rms, ..Frame::default() }
        })
        .collect();

    for f in &mut frames {
        if f.rms < VOICED_LEVEL * loudest || f.rms < 1e-4 {
            continue;
        }
        let mut corr = vec![-1f32; lag_max as usize + 1];
        let mut best = 0usize;
        for lag in lag_min..=lag_max {
            let lag = lag as usize;
            corr[lag] = ncc_at(x, f.centre, lag as i32);
            if best == 0 || corr[lag] > corr[best] {
                best = lag;
            }
        }
        if best == 0 {
            continue;
        }
        let mut best_corr = corr[best];
        if prior_lag > 0 {
            // Prefer the local maximum nearest the prior if it is nearly as good.
            let mut near = 0i32;
            for lag in lag_min + 1..lag_max {
                let c0 = corr[lag as usize];
                if c0 < corr[lag as usize - 1] || c0 < corr[lag as usize + 1] || c0 < 0.85 * best_corr {
                    continue;
                }
                if near == 0 || (lag - prior_lag).abs() < (near - prior_lag).abs() {
                    near = lag;
                }
            }
            if near > 0 {
                best = near as usize;
                best_corr = corr[best];
            }
        } else {
            // Without a prior, an octave-down error is the usual one: take
            // the half period if it correlates nearly as well.
            let half = best / 2;
            if half as i32 >= lag_min && corr[half] >= 0.90 * best_corr {
                best = half;
                best_corr = corr[best];
            }
        }
        f.lag = best as i32;
        // A period far from the voice's own is a burst or noise that happens
        // to correlate, not voicing.
        let off_pitch =
            prior_lag > 0 && ((best as f32) < prior_lag as f32 / 1.45 || best as f32 > prior_lag as f32 * 1.45);
        f.voiced = best_corr >= VOICED_CORRELATION && !off_pitch;
    }

    // A single voiced frame between unvoiced ones, or a single unvoiced frame
    // inside a voiced run, is a decision error.
    for i in 1..frames.len().saturating_sub(1) {
        if frames[i].voiced && !frames[i - 1].voiced && !frames[i + 1].voiced {
            frames[i].voiced = false;
        }
    }
    for i in 1..frames.len().saturating_sub(1) {
        if !frames[i].voiced && frames[i - 1].voiced && frames[i + 1].voiced && frames[i].lag > 0 {
            frames[i].voiced = true;
        }
    }
    frames
}

/// Period at a sample position, from the nearest voiced frame.
fn period_at(frames: &[Frame], pos: i32, fallback: i32) -> i32 {
    let mut best: Option<(i32, i32)> = None;
    for frame in frames.iter().filter(|frame| frame.voiced && frame.lag > 0) {
        let d = (frame.centre - pos).abs();
        if best.is_none_or(|(best_d, _)| d < best_d) {
            best = Some((d, frame.lag));
        }
    }
    best.map_or(fallback, |(_, lag)| lag)
}

/// Analyses one recording. `formant_warp` scales its whole spectrum (1.0 = as
/// recorded); `f0_prior` is the expected pitch of the voice (Hz), 0 if unknown.
pub fn analyse_unit(samples: &[i16], sample_rate: u32, formant_warp: f32, f0_prior: f32) -> Unit {
    let mut unit = Unit::default();
    let fs = sample_rate as f32;
    if samples.is_empty() {
        return unit;
    }

    // Float, DC removed.
    let mean = (samples.iter().fold(0f64, |sum, &s| sum + f64::from(s)) / samples.len() as f64) as f32;
    let mut x: Vec<f32> = samples.iter().map(|&s| (f32::from(s) - mean) / 32768.0).collect();

    // Formant warp: the whole spectrum scaled by the factor.
    if (formant_warp - 1.0).abs() > 1e-3 {
        x = resample(&x, formant_warp);
    }
    let n = x.len() as i32;

    // Edge fades: a file cut inside a period must not click.
    let fade = EDGE_FADE.min(n / 2);
    for i in 0..fade {
        let w = 0.5 * (1.0 - (PI * (i as f32 + 0.5) / fade as f32).cos());
        x[i as usize] *= w;
        x[(n - 1 - i) as usize] *= w;
    }

    // Blocks of 2 ms every millisecond.
    const BLOCK: i32 = 44;
    const HOP: i32 = 22;

    // Sounding part: blocks above -34 dB of the loudest block.
    let env: Vec<f32> = (0..n)
        .step_by(HOP as usize)
        .map(|s| {
            let block = &x[s as usize..(s + BLOCK).min(n) as usize];
            if block.is_empty() { 0.0 } else { (energy(block) / block.len() as f64).sqrt() as f32 }
        })
        .collect();
    let peak = env.iter().fold(0f32, |peak, &r| peak.max(r));
    let threshold = (peak * SOUNDING_LEVEL).max(1e-4);
    let sounding = |&r: &f32| r >= threshold;
    if let (Some(first), Some(last)) = (env.iter().position(sounding), env.iter().rposition(sounding)) {
        unit.lead = first as i32 * HOP;
        unit.trail = n.min(last as i32 * HOP + BLOCK);
    }

    unit.samples = x;
    unit.loaded = true;
    if unit.trail <= unit.lead {
        // Silence (the pause recording).
        (unit.lead, unit.trail) = (0, 0);
        return unit;
    }
    let x = &unit.samples;

    // Sharpest rise in level: the burst of a stop (or affricate). The rise is
    // measured over 3 ms, at least 10 dB and landing within 12 dB of the
    // loudest block.
    unit.onset = unit.lead;
    let env_db: Vec<f32> = (unit.lead..=unit.trail - BLOCK)
        .step_by(HOP as usize)
        .map(|s| 10.0 * ((energy(&x[s as usize..(s + BLOCK) as usize]) / f64::from(BLOCK)) as f32 + 1e-10).log10())
        .collect();
    let peak_db = env_db.iter().fold(-120f32, |peak, &db| peak.max(db));
    let mut best_rise = 10f32;
    for b in 1..env_db.len().saturating_sub(3) {
        let rise = env_db[b + 3] - env_db[b - 1];
        if rise > best_rise && env_db[b + 3] >= peak_db - 12.0 {
            best_rise = rise;
            unit.onset = unit.lead + b as i32 * HOP;
        }
    }

    // Level of the sounding part.
    unit.rms = (energy(&x[unit.lead as usize..unit.trail as usize]) / f64::from(unit.trail - unit.lead)).sqrt() as f32;

    // Pitch track and voicing.
    let frames = pitch_track(x, fs, f0_prior);
    let (lag_min, lag_max) = lag_range(fs);

    // Voiced runs of frames -> sample regions [begin, end).
    let regions: Vec<(i32, i32)> = frames
        .chunk_by(|a, b| a.voiced == b.voiced)
        .filter(|run| run[0].voiced)
        .map(|run| {
            (unit.lead.max(run[0].centre - TRACK_HOP / 2), unit.trail.min(run[run.len() - 1].centre + TRACK_HOP / 2))
        })
        .filter(|(begin, end)| end > begin)
        .collect();

    // Pitch marks: peaks of the low-passed waveform, one per period.
    let mut marks: Vec<(i32, bool)> = Vec::new();
    if !regions.is_empty() {
        // Low-passed to the first harmonics, the waveform has one peak per
        // period; with more of the first formant in it, it can have two.
        let cutoff = if f0_prior > 0.0 { (2.5 * f0_prior).clamp(250.0, 600.0) } else { 400.0 };
        let lp = lowpass_zero_phase(x, cutoff, fs);

        // Polarity: the side with the larger extrema in voiced sound.
        let (mut pos_sum, mut neg_sum) = (0f32, 0f32);
        for &(begin, end) in &regions {
            for &v in &lp[begin as usize..end as usize] {
                if v > 0.0 {
                    pos_sum += v * v;
                } else {
                    neg_sum += v * v;
                }
            }
        }
        let polarity = if pos_sum >= neg_sum { 1.0 } else { -1.0 };
        let value = |i: i32| polarity * lp[i as usize];
        // The first of the highest peaks in lo..=hi, unless `start` is as high.
        let peak_in = |start: i32, lo: i32, hi: i32| {
            (lo..=hi).fold(start, |best, i| if value(i) > value(best) { i } else { best })
        };
        let period = |m: i32| period_at(&frames, m, lag_min).clamp(lag_min, lag_max) as f32;

        for &(begin, end) in &regions {
            // Strongest peak in the region.
            let start = peak_in(begin, begin, end - 1);
            let mut region_marks = vec![start];

            // Forwards: the best peak about one period on.
            let mut m = start;
            loop {
                let t = period(m);
                let lo = m + (0.7 * t) as i32;
                if lo >= end {
                    break;
                }
                m = peak_in(lo, lo, (m + (1.3 * t) as i32).min(end - 1));
                region_marks.push(m);
            }
            // Backwards.
            m = start;
            loop {
                let t = period(m);
                let hi = m - (0.7 * t) as i32;
                if hi < begin {
                    break;
                }
                m = peak_in(hi, (m - (1.3 * t) as i32).max(begin), hi);
                region_marks.push(m);
            }
            marks.extend(region_marks.into_iter().map(|mark| (mark, true)));
        }
    }

    // Unvoiced marks at a fixed spacing wherever the sounding part is not voiced.
    let mut cursor = unit.lead;
    let mut gaps = Vec::new();
    for &(begin, end) in &regions {
        if begin > cursor {
            gaps.push((cursor, begin));
        }
        cursor = cursor.max(end);
    }
    if unit.trail > cursor {
        gaps.push((cursor, unit.trail));
    }
    for (begin, end) in gaps {
        marks.extend((begin + UNVOICED_HOP / 2..end).step_by(UNVOICED_HOP as usize).map(|m| (m, false)));
        // A gap shorter than half a hop still needs a mark, or the sound in
        // it would be lost.
        if end - begin > 0 && end - begin < UNVOICED_HOP / 2 + 1 {
            marks.push(((begin + end) / 2, false));
        }
    }

    // In order, one mark per position.
    marks.sort_by_key(|&(mark, _)| mark);
    marks.dedup_by_key(|&mut (mark, _)| mark);
    (unit.marks, unit.voiced) = marks.into_iter().unzip();

    // Local period of every mark.
    let count = unit.marks.len();
    unit.period = vec![UNVOICED_HOP; count];
    let mut periods = Vec::new();
    for k in 0..count {
        if !unit.voiced[k] {
            continue;
        }
        let prev_v = k > 0 && unit.voiced[k - 1];
        let next_v = k + 1 < count && unit.voiced[k + 1];
        let t = match (prev_v, next_v) {
            (true, true) => (unit.marks[k + 1] - unit.marks[k - 1]) / 2,
            (false, true) => unit.marks[k + 1] - unit.marks[k],
            (true, false) => unit.marks[k] - unit.marks[k - 1],
            (false, false) => period_at(&frames, unit.marks[k], lag_min),
        };
        unit.period[k] = t.clamp(lag_min, lag_max);
        periods.push(unit.period[k] as f32);
    }
    unit.f0 = if periods.len() >= 2 { fs / median(periods) } else { 0.0 };
    unit
}

/// The analysed recordings of one voice.
///
/// A formant warp other than 1.0 resamples every recording first, scaling its
/// whole spectrum by the factor (1.2 makes the vocal tract sound a fifth
/// shorter); the voice character of the derived voices (child, grandmother)
/// comes from this. The pitch itself is not fixed here: the renderer sets it
/// from the prosody.
#[derive(Clone, Debug)]
pub struct UnitBank {
    units: [Unit; Sound::COUNT],
    warp: f32,
    base_f0: f32,
}

impl UnitBank {
    /// Analyses the recordings with a spectrum scale factor (1.0 = as
    /// recorded). `None` if the voice holds no recording.
    pub fn build(data: &VoiceData, formant_warp: f32) -> Option<Self> {
        const VOWELS: [Sound; 5] = [Sound::A, Sound::E, Sound::I, Sound::O, Sound::U];
        let fs = if data.sample_rate() > 0 { data.sample_rate() } else { SAMPLE_RATE };

        // Pass 1: the vowels, without a prior, give the voice's pitch.
        let prior = median(
            VOWELS
                .iter()
                .map(|&vowel| analyse_unit(data.sound(vowel), fs, formant_warp, 0.0).f0)
                .filter(|&f0| f0 > 0.0)
                .collect(),
        );

        // Pass 2: every recording, with the prior.
        let units = Sound::ALL.map(|sound| analyse_unit(data.sound(sound), fs, formant_warp, prior));
        let vowel_f0 = VOWELS.iter().map(|&vowel| units[vowel as usize].f0).filter(|&f0| f0 > 0.0).collect();
        let base_f0 = match median(vowel_f0) {
            f0 if f0 > 0.0 => f0,
            _ if prior > 0.0 => prior,
            _ => 120.0,
        };
        units.iter().any(|unit| unit.loaded).then_some(Self { units, warp: formant_warp, base_f0 })
    }

    pub fn unit(&self, sound: Sound) -> &Unit {
        &self.units[sound as usize]
    }

    /// Median pitch of the vowels after warping (Hz).
    pub fn base_f0(&self) -> f32 {
        self.base_f0
    }

    pub fn formant_warp(&self) -> f32 {
        self.warp
    }
}
