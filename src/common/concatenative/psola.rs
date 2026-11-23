//! TD-PSOLA rendering of planned segments: time-domain pitch-synchronous
//! overlap-add over the pitch marks of the recordings.
//!
//! Joins between two voiced recordings are a crossfade of whole periods (no
//! fixed crossfade, no truncation); rate repeats or skips periods; pitch is
//! the spacing of the output marks alone.

use super::unit_bank::{EDGE_FADE, UNVOICED_HOP, Unit};
use std::f32::consts::PI;
use std::sync::OnceLock;

/// Span of the period crossfade at a join between two voiced recordings.
const JOIN_MS: f32 = 14.0;

/// Longest window the renderer places (two periods at the lowest pitch).
const MAX_HALF_WINDOW: i32 = 400;

// Pitch limits of the output marks.
const OUT_F0_MIN: f32 = 40.0;
const OUT_F0_MAX: f32 = 600.0;

/// One stretch of the output: a recording played over a time span, or silence.
#[derive(Clone, Copy, Debug)]
pub struct Segment<'a> {
    /// `None`: silence.
    pub unit: Option<&'a Unit>,
    /// Output start (samples).
    pub start: f32,
    /// Output length (samples).
    pub length: f32,
    /// Source range used, `src_begin..src_end`.
    pub src_begin: i32,
    pub src_end: i32,
    /// False: natural rate, cut off at the end of the span.
    pub stretch: bool,
    /// Crossfade periods with the previous segment's recording ...
    pub join_prev: bool,
    /// ... and with the next one's.
    pub join_next: bool,
    /// Level of the segment.
    pub gain: f32,
    /// Long friction (s, z, š, ž, c, č, ć, đ, f): what the hiss hold acts on.
    pub hiss: bool,
}

impl Default for Segment<'_> {
    fn default() -> Self {
        Self {
            unit: None,
            start: 0.0,
            length: 0.0,
            src_begin: 0,
            src_end: 0,
            stretch: true,
            join_prev: false,
            join_next: false,
            gain: 1.0,
            hiss: false,
        }
    }
}

impl Segment<'_> {
    /// Output time -> source sample (extrapolates past the segment's span).
    fn source_pos(&self, t: f32) -> f32 {
        if self.stretch {
            let x = if self.length > 0.0 { (t - self.start) / self.length } else { 0.0 };
            self.src_begin as f32 + x * (self.src_end - self.src_begin) as f32
        } else {
            self.src_begin as f32 + (t - self.start)
        }
    }

    /// Index of the analysis mark of `u` nearest a source position, within
    /// the segment's source range; `None` if the recording has no marks.
    fn nearest_mark(&self, u: &Unit, pos: f32) -> Option<usize> {
        if u.marks.is_empty() {
            return None;
        }
        let pos = pos.max(self.src_begin as f32).min(self.src_begin.max(self.src_end - 1) as f32);
        let distance = |c: usize| (u.marks[c] as f32 - pos).abs();
        let mut k = u.marks.partition_point(|&mark| mark < pos.round() as i32);
        // Only marks inside the segment's source range.
        let in_range = |&c: &usize| {
            u.marks.get(c).is_some_and(|&m| m >= self.src_begin - u.period[c] && m < self.src_end + u.period[c])
        };
        let (before, after) = (k.checked_sub(1).filter(in_range), Some(k).filter(in_range));
        match (before, after) {
            (Some(before), Some(after)) => {
                return Some(if distance(after) < distance(before) { after } else { before });
            }
            (Some(c), None) | (None, Some(c)) => return Some(c),
            (None, None) => {}
        }
        // Nothing within range: the nearest mark anyway.
        k = k.min(u.marks.len() - 1);
        if k > 0 && distance(k - 1) < distance(k) {
            k -= 1;
        }
        Some(k)
    }
}

/// Pitch contour of a clause: one value per millisecond of output, in Hz.
/// Values are interpolated between the grid points.
#[derive(Clone, Debug, Default)]
pub struct PitchContour {
    /// `hz[k]` at k ms.
    pub hz: Vec<f32>,
}

impl PitchContour {
    pub fn at(&self, t_samples: f32, sample_rate: u32) -> f32 {
        let (Some(&first), Some(&last)) = (self.hz.first(), self.hz.last()) else { return 120.0 };
        let ms = t_samples * 1000.0 / sample_rate as f32;
        if ms <= 0.0 {
            return first;
        }
        let k = ms as usize;
        if k + 1 >= self.hz.len() {
            return last;
        }
        self.hz[k] + (self.hz[k + 1] - self.hz[k]) * (ms - k as f32)
    }
}

fn half_window(u: &Unit, k: usize) -> i32 {
    u.period[k].clamp(8, MAX_HALF_WINDOW)
}

/// A mark whose whole window lies inside the recording, clear of its faded
/// ends. The windows of the first and last marks of a short recording reach
/// past it: the sound then faded out within 2 ms (Josip's dž, -2 dB at its
/// peak, into silence before the vowel) or came in as fast. A mark further in
/// takes their place, one of the same kind if there is one near.
fn fitting_mark(u: &Unit, k: usize) -> usize {
    let n = u.length();
    let fits = |c: usize| {
        let (m, half) = (u.marks[c], half_window(u, c));
        (m - half - EDGE_FADE).min(n - EDGE_FADE - (m + half)) >= 0
    };
    if fits(k) {
        return k;
    }
    let inwards: Vec<usize> =
        if u.marks[k] > n / 2 { (0..k).rev().take(4).collect() } else { (k + 1..u.marks.len()).take(4).collect() };
    [true, false]
        .into_iter()
        .find_map(|same| inwards.iter().copied().find(|&c| (!same || u.voiced[c] == u.voiced[k]) && fits(c)))
        .unwrap_or(k)
}

/// One half of a Hann window of `length` samples (1 to `MAX_HALF_WINDOW`):
/// the weight of the sample `j` away from the centre, before it (`rising`)
/// or from it on. Every output sample lies under two windows, and each
/// weight was a cosine computed for it: a fifth of a recorded voice's time. A
/// half is computed once, the first time a period of its length is placed,
/// by the expression the renderer used, so the weights are the same.
fn window_half(length: i32, rising: bool) -> &'static [f32] {
    const LENGTHS: usize = MAX_HALF_WINDOW as usize + 1;
    static HALVES: [[OnceLock<Box<[f32]>>; 2]; LENGTHS] = [const { [const { OnceLock::new() }; 2] }; LENGTHS];
    HALVES[length as usize][usize::from(rising)].get_or_init(|| {
        let scale = PI / length as f32;
        (0..length).map(|j| 0.5 * (1.0 + (scale * (if rising { -j } else { j }) as f32).cos())).collect()
    })
}

/// One Hann-windowed period of a recording, centred on its mark `k`, added at
/// the output sample `centre`. Each half of the window is shortened where it
/// would reach past the recording, or before the output sample `earliest`, so
/// it still rises from zero: the first window of a clause was centred half a
/// period of another mark into the clause and opened on a third of a period's
/// peak (Vlado's u: -3197 as the very first sample).
fn add_window(out: &mut [f32], u: &Unit, k: usize, centre: i32, gain: f32, reversed: bool, earliest: i32) {
    let mark = u.marks[k];
    let half = half_window(u, k);
    let n = u.length();
    // The recording before and after the mark, in output order.
    let (before, after) = if reversed { (n - 1 - mark, mark) } else { (mark, n - 1 - mark) };
    let left = half.min(before + 1).min(centre - earliest + 1).max(1);
    let right = half.min(after + 1).max(1);
    let (rising, falling) = (window_half(left, true), window_half(right, false));
    for i in -left + 1..right {
        let Some(o) = usize::try_from(centre + i).ok().filter(|&o| o < out.len()) else { continue };
        let s = mark + if reversed { -i } else { i };
        let w = if i < 0 { rising[(-i) as usize] } else { falling[i as usize] };
        out[o] += gain * w * u.samples[s as usize];
    }
}

/// Renders the segments of a clause, laid out in time order by the planner,
/// at the pitch of `contour`. `total_samples` is the length of the clause's
/// sound; the output is a little longer, so the last window can fade out.
/// Samples are in -1..1 (louder passages may exceed it; the caller limits).
pub fn render(segments: &[Segment<'_>], contour: &PitchContour, sample_rate: u32, total_samples: f32) -> Vec<f32> {
    let fs = sample_rate as f32;
    let mut out = vec![0f32; (total_samples.ceil() as i32).max(0) as usize + MAX_HALF_WINDOW as usize];

    let join = JOIN_MS * fs / 1000.0;
    let half_join = 0.5 * join;

    // Adds the window of a neighbouring segment at the same output mark.
    fn add_neighbour(out: &mut [f32], nb: &Segment<'_>, t: f32, centre: i32, ts: f32, weight: f32) -> bool {
        let Some(unit) = nb.unit else { return false };
        let Some(k) = nb.nearest_mark(unit, nb.source_pos(t)).filter(|&k| unit.voiced[k]) else { return false };
        let k = fitting_mark(unit, k);
        let gain = weight * (ts / unit.period[k] as f32).clamp(0.5, 1.6);
        add_window(out, unit, k, centre, gain, false, 0);
        true
    }

    let mut t = 0f32; // next output mark (samples)
    for (i, seg) in segments.iter().enumerate() {
        let seg_end = seg.start + seg.length;
        let Some(u) = seg.unit.filter(|_| seg.length > 0.0) else {
            // Silence: the marks start afresh after it.
            t = t.max(seg_end);
            continue;
        };
        // A fresh start (clause start, after a pause): the first window rises
        // from zero at the segment's start instead of opening on the peak of
        // a period.
        let mut earliest = 0;
        if t <= seg.start {
            let half = seg.nearest_mark(u, seg.source_pos(seg.start)).map_or(UNVOICED_HOP, |k0| u.period[k0]) as f32;
            t = seg.start + half.min(0.5 * seg.length);
            earliest = seg.start.ceil() as i32;
        }

        let mut last_k = None;
        let mut placed = 0;
        while t < seg_end || placed == 0 {
            if placed == 0 && t >= seg_end {
                // Too short for a whole period: still one window, in the middle.
                t = seg.start + 0.5 * seg.length;
            }
            let Some(k) = seg.nearest_mark(u, seg.source_pos(t)) else { break };
            let k = fitting_mark(u, k);
            let voiced = u.voiced[k];
            let ta = u.period[k] as f32;
            let ts = if voiced {
                fs / contour.at(t, sample_rate).clamp(OUT_F0_MIN, OUT_F0_MAX)
            } else {
                UNVOICED_HOP as f32
            };
            let centre = t.round() as i32;

            // Period crossfade across a join with a voiced neighbour.
            let mut weight = 1f32;
            if voiced {
                if seg.join_prev && i > 0 && t < seg.start + half_join {
                    let x = ((t - (seg.start - half_join)) / join).clamp(0.0, 1.0);
                    if add_neighbour(&mut out, &segments[i - 1], t, centre, ts, 1.0 - x) {
                        weight = x;
                    }
                } else if seg.join_next && i + 1 < segments.len() && t > seg_end - half_join {
                    let x = ((t - (seg_end - half_join)) / join).clamp(0.0, 1.0);
                    if add_neighbour(&mut out, &segments[i + 1], t, centre, ts, x) {
                        weight = 1.0 - x;
                    }
                }
            }

            let gain = seg.gain * if voiced { weight * (ts / ta).clamp(0.5, 1.6) } else { 1.0 };
            let reversed = !voiced && last_k == Some(k);
            add_window(&mut out, u, k, centre, gain, reversed, if placed == 0 { earliest } else { 0 });
            last_k = Some(k);
            placed += 1;
            t += ts;
        }
    }
    out
}
