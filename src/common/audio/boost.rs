//! Louder output for both kinds of voices.
//!
//! The speech of the formant voices peaks close to full scale while its median
//! level sits near -20 dBFS, and the recorded voices already touch the
//! ceiling, so a plain gain cannot make either louder without clipping. The
//! volume boost is a gain of `VOLUME_BOOST_GAIN` (+14 dB) followed by a peak
//! limiter whose gain moves only between syllables, never inside a glottal
//! cycle:
//!
//! 1. every sample asks for the gain that would put it at the ceiling (1 when
//!    it is already below);
//! 2. the envelope is the minimum of those over a window that reaches
//!    `LOOKAHEAD_SECONDS` ahead and `HOLD_SECONDS` back, so the gain is down
//!    before a peak arrives and stays down for longer than a pitch period
//!    after it (20 ms covers every period down to 50 Hz);
//! 3. the attack is a box average of the envelope over the lookahead: a linear
//!    ramp down to the peak, and every term of the average already includes
//!    the peak's own wanted gain, so no peak passes the ceiling;
//! 4. the release follows upward moves with a one-pole of `RELEASE_SECONDS`;
//!    downward moves are taken at once;
//! 5. a tanh soft clipper above the knee stays as a safety net; with the
//!    ceiling under the knee it is idle;
//! 6. the gain may rise at most `max_lift_db` above the smallest gain of the
//!    last `LIFT_WINDOW_SECONDS` (the loudest recent syllable), so the quiet
//!    consonants between loud vowels keep their distance from them;
//! 7. before all that, in boost mode only, a second-order high shelf takes the
//!    treble down from `VOLUME_BOOST_TREBLE_HZ` up, where a phone's speaker
//!    has its own peak, and the hiss limiter holds the hissing consonants
//!    under the vowels.
//!
//! The boost sits before the volume (host or user) is applied, so a host at
//! half volume with the boost on is still louder than without it. Measurements
//! in "Hiss, crackle and the boost" in docs/formant.md and "Volume boost" in
//! docs/dev.md.

// `!(x > 0.0)` is meant: a NaN counts as not positive.
#![allow(clippy::neg_cmp_op_on_partial_ord)]

use crate::math::{pow10, pow10f};
use crate::params::{
    VOLUME_BOOST_CEILING, VOLUME_BOOST_GAIN, VOLUME_BOOST_HISS_DB, VOLUME_BOOST_HISS_HZ, VOLUME_BOOST_HISS_MAX_CUT_DB,
    VOLUME_BOOST_HISS_SPLIT_HZ, VOLUME_BOOST_KNEE, VOLUME_BOOST_MAX_LIFT_DB, VOLUME_BOOST_TREBLE_DB,
    VOLUME_BOOST_TREBLE_HZ,
};
use std::collections::VecDeque;
use std::ops::Range;

/// The gain ramps down over this time before a peak ...
const LOOKAHEAD_SECONDS: f32 = 0.005;
/// ... stays down at least this long after it ...
const HOLD_SECONDS: f32 = 0.020;
/// ... then recovers with this time constant.
const RELEASE_SECONDS: f32 = 0.050;
/// The loudest syllable this far back caps the lift.
const LIFT_WINDOW_SECONDS: f32 = 0.6;

// The hiss limiter: the band powers are followed over these times, the vowel
// reference is the loudest low band this far either side of a sample, and the
// cut acts only while the hiss band stands at least HISS_GATE_LOW_DB re the
// low band of the same moment (fully from HISS_GATE_HIGH_DB), which no vowel
// reaches (their band above 3 kHz lies 15-25 dB under the rest) and every
// sibilant passes.
const HISS_TAU_SECONDS: f32 = 0.002;
const HISS_LOW_TAU_SECONDS: f32 = 0.005;
const HISS_VOWEL_WINDOW_SECONDS: f32 = 0.25;
/// Two glottal periods: the vowel, not its pulses.
const HISS_VOWEL_AVERAGE_SECONDS: f32 = 0.020;
const HISS_GATE_LOW_DB: f32 = -12.0;
const HISS_GATE_HIGH_DB: f32 = -3.0;
const HISS_LOOKAHEAD_SECONDS: f32 = 0.002;
const HISS_HOLD_SECONDS: f32 = 0.005;
const HISS_RELEASE_SECONDS: f32 = 0.015;
/// The release outside the spans the hold is confined to (see
/// [`limit_hiss_in`]): the gain is back at 1 this long after the cut at the
/// latest.
const HISS_SPAN_RELEASE_SECONDS: f32 = 0.0015;

/// π as the reference recordings were made with it.
#[allow(clippy::approx_constant)]
const PI: f64 = 3.14159265358979;

/// A second-order section of the Audio EQ Cookbook.
#[derive(Clone, Copy)]
struct Section {
    b: [f64; 3],
    a: [f64; 2],
    x: [f64; 2],
    y: [f64; 2],
}

impl Section {
    fn new(b: [f64; 3], a: [f64; 2]) -> Self {
        Self { b, a, x: [0.0; 2], y: [0.0; 2] }
    }

    /// Butterworth low-pass or high-pass (Q = 1/√2).
    fn butterworth(freq: f32, fs: f32, highpass: bool) -> Self {
        let w0 = 2.0 * PI * f64::from(freq) / f64::from(fs);
        let cw = w0.cos();
        let alpha = w0.sin() / 2.0 * 2f64.sqrt();
        let a0 = 1.0 + alpha;
        let (b0, b1) =
            if highpass { ((1.0 + cw) / 2.0 / a0, -(1.0 + cw) / a0) } else { ((1.0 - cw) / 2.0 / a0, (1.0 - cw) / a0) };
        Self::new([b0, b1, b0], [-2.0 * cw / a0, (1.0 - alpha) / a0])
    }

    /// The cookbook's high shelf with the slope 1: `db` at the top, half of it
    /// at `freq`.
    fn high_shelf(freq: f32, fs: f32, db: f32) -> Self {
        let a = pow10(f64::from(db) / 40.0);
        let w0 = 2.0 * PI * f64::from(freq) / f64::from(fs);
        let cw = w0.cos();
        let sa = 2.0 * a.sqrt() * w0.sin() / 2.0 * 2f64.sqrt();
        let a0 = (a + 1.0) - (a - 1.0) * cw + sa;
        Self::new(
            [
                a * ((a + 1.0) + (a - 1.0) * cw + sa) / a0,
                -2.0 * a * ((a - 1.0) + (a + 1.0) * cw) / a0,
                a * ((a + 1.0) + (a - 1.0) * cw - sa) / a0,
            ],
            [2.0 * ((a - 1.0) - (a + 1.0) * cw) / a0, ((a + 1.0) - (a - 1.0) * cw - sa) / a0],
        )
    }

    fn tick(&mut self, x: f64) -> f64 {
        let y = self.b[0] * x + self.b[1] * self.x[0] + self.b[2] * self.x[1]
            - self.a[0] * self.y[0]
            - self.a[1] * self.y[1];
        self.x = [x, self.x[0]];
        self.y = [y, self.y[0]];
        y
    }

    /// Called while the input is exact silence: once the section has rung
    /// down to nothing (500 dB under full scale, far below what a sample can
    /// hold), it is cleared. Left alone it does not arrive at zero: it stalls
    /// at the smallest numbers a double can hold, on which arithmetic is many
    /// times slower, for as long as the silence lasts.
    fn settle(&mut self) {
        const NOTHING: f64 = 1e-25;
        if self.x.iter().chain(&self.y).all(|v| v.abs() < NOTHING) {
            (self.x, self.y) = ([0.0; 2], [0.0; 2]);
        }
    }
}

/// For every `j`, the index of the extreme of `values` over `j - back ..=
/// j + ahead` (a monotonic deque); `better(a, b)` says that `a` displaces an
/// earlier `b`.
fn sliding_extreme(values: &[f32], ahead: usize, back: usize, better: impl Fn(f32, f32) -> bool) -> Vec<usize> {
    let n = values.len();
    let mut window: VecDeque<usize> = VecDeque::new();
    let mut out = Vec::with_capacity(n);
    for i in 0..n + ahead {
        if i < n {
            while window.back().is_some_and(|&last| better(values[i], values[last])) {
                window.pop_back();
            }
            window.push_back(i);
        }
        if i < ahead {
            continue;
        }
        let j = i - ahead;
        while window.front().is_some_and(|&first| first + back < j) {
            window.pop_front();
        }
        // The window always holds the sample itself or one after it.
        out.push(window.front().copied().unwrap_or(j));
    }
    out
}

/// Holds the hiss band of `wave` under `hiss_db` re the loudest nearby vowel
/// while it dominates the sound (see `VOLUME_BOOST_HISS_DB`, and
/// `RECORDED_HISS_DB` for the plain output of the recorded voices). The band
/// is the signal minus its low-passed part, and a sample whose gain is 1 is
/// left as it is.
pub fn limit_hiss(wave: &mut [f32], fs: f32, hiss_db: f32) {
    limit_hiss_in(wave, fs, hiss_db, None);
}

/// [`limit_hiss`] confined to `spans` (sample ranges in ascending order: the
/// sibilants and f as the planner of a recorded voice laid them out), when
/// there are any. Found by the sound alone, the hold also took 6 dB off the
/// burst of t, all there is of a t, and, its release being 15 ms, 2 to 8 dB
/// off the band above 2 kHz of the first 10 to 25 ms of the vowel after every
/// sound it held, where the vowel's F2 and F3 say which consonant it was
/// (2026-11-17).
pub fn limit_hiss_in(wave: &mut [f32], fs: f32, hiss_db: f32, spans: Option<&[Range<usize>]>) {
    let n = wave.len();
    if n == 0 {
        return;
    }
    let mut split = [Section::butterworth(VOLUME_BOOST_HISS_SPLIT_HZ, fs, false); 2];
    let mut det_lp = Section::butterworth(VOLUME_BOOST_HISS_HZ, fs, false);
    let mut det_hp = Section::butterworth(VOLUME_BOOST_HISS_HZ, fs, true);
    let (mut low, mut hi_pow, mut low_pow, mut low_sq) = (vec![0f32; n], vec![0f32; n], vec![0f32; n], vec![0f32; n]);
    let k_hi = 1.0 - (-1.0 / f64::from(HISS_TAU_SECONDS * fs)).exp();
    let k_lo = 1.0 - (-1.0 / f64::from(HISS_LOW_TAU_SECONDS * fs)).exp();
    let (mut ph, mut pl) = (0f64, 0f64);
    for i in 0..n {
        let x = f64::from(wave[i]);
        if x == 0.0 {
            split.iter_mut().chain([&mut det_lp, &mut det_hp]).for_each(Section::settle);
        }
        let once = split[0].tick(x);
        low[i] = split[1].tick(once) as f32;
        let h = det_hp.tick(x);
        let l = det_lp.tick(x);
        ph += (h * h - ph) * k_hi;
        pl += (l * l - pl) * k_lo;
        hi_pow[i] = ph as f32;
        low_pow[i] = pl as f32;
        low_sq[i] = (l * l) as f32;
    }

    // The vowel reference: the low band's power averaged over two glottal
    // periods (a shorter follower rides on the pulses and overstates the vowel
    // by a few dB), the loudest within the window either side.
    let avg_len = ((HISS_VOWEL_AVERAGE_SECONDS * fs) as usize).max(1);
    let mut average = vec![0f32; n];
    let mut sum = 0f64;
    let lead = avg_len / 2;
    for i in 0..n + lead {
        if i < n {
            sum += f64::from(low_sq[i]);
        }
        if i >= avg_len && i - avg_len < n {
            sum -= f64::from(low_sq[i - avg_len]);
        }
        if i < lead {
            continue;
        }
        let from = if i >= avg_len { i - avg_len + 1 } else { 0 };
        let to = i.min(n - 1);
        average[i - lead] = (sum.max(0.0) / (to - from + 1) as f64) as f32;
    }
    let half = (HISS_VOWEL_WINDOW_SECONDS * fs) as usize;
    let vowel = sliding_extreme(&average, half, half, |new, old| old <= new);

    // The gain the hiss band wants, sample by sample.
    let target = pow10f(hiss_db / 10.0); // in power
    let floor_gain = pow10f(-VOLUME_BOOST_HISS_MAX_CUT_DB / 20.0);
    // A band 3 dB and more under the gate has no share: known from the
    // ratio, without its logarithm (most samples of a vowel).
    let surely_under_gate = 0.5 * pow10f(HISS_GATE_LOW_DB / 10.0);
    let mut cut = false;
    let inside = spans.map(|spans| {
        let mut inside = vec![false; n];
        for span in spans {
            inside[span.start.min(n)..span.end.min(n)].fill(true);
        }
        inside
    });
    let want: Vec<f32> = (0..n)
        .map(|i| {
            if inside.as_ref().is_some_and(|inside| !inside[i]) {
                return 1.0;
            }
            let vowel = average[vowel[i]];
            if !(hi_pow[i] > 0.0) || !(vowel > 0.0) {
                return 1.0;
            }
            let dominance = hi_pow[i] / (low_pow[i] + 1e-20);
            if dominance < surely_under_gate {
                return 1.0;
            }
            cut = true;
            let dominance_db = 10.0 * dominance.log10();
            let share = ((dominance_db - HISS_GATE_LOW_DB) / (HISS_GATE_HIGH_DB - HISS_GATE_LOW_DB)).clamp(0.0, 1.0);
            if share <= 0.0 {
                return 1.0;
            }
            let level = f32::min(1.0, (vowel * target / hi_pow[i]).sqrt());
            1.0 - share * (1.0 - floor_gain.max(level))
        })
        .collect();
    // No hiss anywhere: the gain below would be 1 throughout and no sample
    // would be touched.
    if !cut || want.iter().all(|&gain| gain == 1.0) {
        return;
    }

    // Down before the hiss arrives and held a little after it, then released.
    let ahead = (HISS_LOOKAHEAD_SECONDS * fs) as usize;
    let hold = (HISS_HOLD_SECONDS * fs) as usize;
    let release = (-1.0 / (HISS_RELEASE_SECONDS * fs)).exp();
    // Outside the spans the gain rises in a straight line and arrives.
    let span_step = 1.0 / (HISS_SPAN_RELEASE_SECONDS * fs);
    let mut g = 1f32;
    for (j, lowest) in sliding_extreme(&want, ahead, hold, |new, old| old >= new).into_iter().enumerate() {
        // Past the end of a span the hold lets go at once.
        let outside = inside.as_ref().is_some_and(|inside| !inside[j]);
        let t = if outside && want[lowest] < 1.0 && lowest < j { 1.0 } else { want[lowest] };
        g = if t < g {
            t
        } else if outside {
            (g + span_step).min(t)
        } else {
            g + (t - g) * (1.0 - release)
        };
        // Untouched where nothing is cut: low + (wave - low) is not the sample
        // bit for bit, and the plain output of the recorded voices goes
        // through here too.
        if g < 1.0 {
            wave[j] = low[j] + g * (wave[j] - low[j]);
        }
    }
}

/// The largest magnitude between samples `i` and `i + 1` as a resampler will
/// reconstruct it: an 8-tap Lanczos interpolator (a = 4) at a quarter, a half
/// and three quarters of the way. A cubic through four samples missed 0.3 dB
/// of Josip's peaks, which then went over -1 dBTP.
fn inter_sample_peak(x: &[f32], i: usize) -> f32 {
    const W: [[f32; 8]; 3] = [
        [-0.0150542, 0.0554490, -0.1523039, 0.8933886, 0.2826839, -0.0916606, 0.0314677, -0.0039706],
        [-0.0126302, 0.0597641, -0.1660114, 0.6188774, 0.6188774, -0.1660114, 0.0597641, -0.0126302],
        [-0.0039706, 0.0314677, -0.0916606, 0.2826839, 0.8933886, -0.1523039, 0.0554490, -0.0150542],
    ];
    let taps: [f32; 8] = std::array::from_fn(|k| x[(i + k).saturating_sub(3).min(x.len() - 1)]);
    W.iter().fold(x[i].abs(), |peak, w| {
        let v = w.iter().zip(&taps).fold(0f32, |v, (w, tap)| v + w * tap);
        peak.max(v.abs())
    })
}

/// How [`boost_volume`] drives a clause.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Boost {
    pub gain: f32,
    /// Ceiling on the true peak.
    pub ceiling: f32,
    /// Soft clipper above this (0 or ≥ 1: none).
    pub knee: f32,
    /// Cap on the lift of the quiet stretches (0: none).
    pub max_lift_db: f32,
    /// High shelf (0: none).
    pub treble_db: f32,
    /// Hiss limiter (0: none).
    pub hiss_db: f32,
}

/// [`boost_volume`] with the hiss limiter confined to `hiss_spans` (see
/// [`limit_hiss_in`]).
pub fn boost_volume_in(wave: &mut [f32], sample_rate: u32, boost: &Boost, hiss_spans: Option<&[Range<usize>]>) {
    boost_stages(wave, sample_rate, boost, hiss_spans);
}

impl Default for Boost {
    /// The volume boost.
    fn default() -> Self {
        Self {
            gain: VOLUME_BOOST_GAIN,
            ceiling: VOLUME_BOOST_CEILING,
            knee: VOLUME_BOOST_KNEE,
            max_lift_db: VOLUME_BOOST_MAX_LIFT_DB,
            treble_db: VOLUME_BOOST_TREBLE_DB,
            hiss_db: VOLUME_BOOST_HISS_DB,
        }
    }
}

impl Boost {
    /// The limiter alone at unity gain, for the plain output: its ceiling at
    /// the renderer's soft knee.
    pub fn limiter(knee: f32) -> Self {
        Self { gain: 1.0, ceiling: knee, knee: 0.0, max_lift_db: 0.0, treble_db: 0.0, hiss_db: 0.0 }
    }
}

/// Multiplies `wave` by the gain and holds the peaks of the result under the
/// ceiling with a lookahead peak limiter whose gain never moves inside a
/// glottal cycle; what is still above the knee is rounded off below full scale
/// with a soft clipper. In place; deterministic. The limiter starts at unity
/// gain, so a quiet clause is the plain product.
pub fn boost_volume(wave: &mut [f32], sample_rate: u32, boost: &Boost) {
    boost_stages(wave, sample_rate, boost, None);
}

fn boost_stages(wave: &mut [f32], sample_rate: u32, boost: &Boost, hiss_spans: Option<&[Range<usize>]>) {
    let Boost { gain, ceiling, knee, max_lift_db, treble_db, hiss_db } = *boost;
    if wave.is_empty() || !(gain > 0.0) || !(ceiling > 0.0) {
        return;
    }
    let fs = if sample_rate > 0 { sample_rate as f32 } else { 22050.0 };
    let lookahead = ((LOOKAHEAD_SECONDS * fs) as usize).max(1);
    let hold = (HOLD_SECONDS * fs) as usize;
    let lift_window = (LIFT_WINDOW_SECONDS * fs) as usize;
    let release = (-1.0 / (RELEASE_SECONDS * fs)).exp();
    let max_lift = if max_lift_db > 0.0 { pow10f(max_lift_db / 20.0) } else { 0.0 };
    let n = wave.len();

    if treble_db < 0.0 {
        let mut shelf = Section::high_shelf(VOLUME_BOOST_TREBLE_HZ, fs, treble_db);
        for x in wave.iter_mut() {
            *x = shelf.tick(f64::from(*x)) as f32;
        }
    }

    // The hissing consonants held under the vowels.
    if hiss_db < 0.0 {
        limit_hiss_in(wave, fs, hiss_db, hiss_spans);
    }

    // The gain each sample wants on its own: 1 below the ceiling, less above,
    // with the peak between this sample and the next counted too.
    if gain != 1.0 {
        for x in wave.iter_mut() {
            *x *= gain;
        }
    }
    // The peak between two samples is a weighted sum of eight samples around
    // them, and the weights of no row add up to more than 1.7146 in
    // magnitude: where none of the eight reaches the ceiling divided by
    // that, the peak cannot reach the ceiling, and the sample wants 1
    // without the sum being formed. Most of speech lies under it.
    const WEIGHTS_BOUND: f32 = 1.72;
    let quiet = ceiling / WEIGHTS_BOUND;
    let mut want = vec![1f32; n];
    let mut limited = false;
    // The taps of sample i are the samples i - 3 ..= i + 4: the last loud
    // one seen up to i + 4 is among them when it is not more than 3 back.
    let mut last_loud: Option<usize> = None;
    let mut seen = 0;
    for (i, want) in want.iter_mut().enumerate() {
        while seen < n && seen <= i + 4 {
            if wave[seen].abs() > quiet {
                last_loud = Some(seen);
            }
            seen += 1;
        }
        if last_loud.is_some_and(|loud| loud + 3 >= i) {
            let a = inter_sample_peak(wave, i);
            if a > ceiling {
                *want = ceiling / a;
                limited = true;
            }
        }
    }
    // Nothing reaches the ceiling: the limiter's gain would stay 1 from the
    // first sample to the last and every sample would be multiplied by it,
    // which leaves it as it is. (The soft clipper works on its own.)
    let clip = knee > 0.0 && knee < 1.0;
    if !limited && !clip {
        return;
    }

    // The envelope: the sliding minimum of `want` over the next `lookahead`
    // and the last `hold` samples.
    let lowest = |values: &[f32], ahead, back| -> Vec<f32> {
        sliding_extreme(values, ahead, back, |new, old| old >= new).into_iter().map(|i| values[i]).collect()
    };
    let mut envelope = lowest(&want, lookahead, hold);

    // The lift cap: the envelope may not exceed `max_lift` times its own
    // running minimum over the last `lift_window` samples. Applied to the
    // envelope, before the attack averages it, so the gain still moves only in
    // the attack's ramps and the release's curve.
    if max_lift > 0.0 {
        let floors = lowest(&envelope, 0, lift_window);
        for (value, floor) in envelope.iter_mut().zip(floors) {
            *value = value.min(floor * max_lift);
        }
    }

    // The attack (a box average over the lookahead) and the release.
    let mut sum = 0f64;
    let mut g = 1f32;
    for j in 0..n {
        sum += f64::from(envelope[j]);
        if j >= lookahead {
            sum -= f64::from(envelope[j - lookahead]);
        }
        let target = (sum / (j + 1).min(lookahead) as f64) as f32;
        g = if target < g { target } else { g + (target - g) * (1.0 - release) };
        let mut x = wave[j] * g;
        if clip && x.abs() > knee {
            let over = (x.abs() - knee) / (1.0 - knee);
            x = (knee + (1.0 - knee) * over.tanh()).copysign(x);
        }
        wave[j] = x;
    }
}
