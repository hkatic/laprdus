//! What the audio tests share: an engine with one voice loaded, and the
//! measurements their thresholds were set on.
//!
//! The measurements are the algorithms of the test programs the thresholds
//! were tuned with (the same windows, hops and arithmetic): change one and
//! the ranges in the tests no longer mean what their comments say.

#![allow(dead_code)]

use laprdus::engine::{Engine, Error};
use laprdus::language::Language;
use laprdus::params::{NumberMode, SpellingMode, VOLUME_BOOST_TREBLE_DB, VOLUME_BOOST_TREBLE_HZ};
use laprdus::voice;
use std::ops::{Deref, DerefMut};
use std::path::{Path, PathBuf};

/// A directory that does not exist: formant voices must not look for files.
pub const NO_DATA: &str = "/nonexistent/laprdus-data";

pub const FORMANT_VOICES: [&str; 3] = ["zvonko", "stojan", "mirsad"];

/// The short pause the number converter writes where the colon of a clock
/// time and the periods of a numeric date stood, and before a cell's address
/// (U+FDD0 as a word of its own): the words a number is expected to be read
/// as have it too.
pub const BRK: &str = " \u{FDD0} ";
/// The pause between two items, a cell's contents and its address.
pub const IBRK: &str = " \u{FDD2} ";

// Pi as the measurements were written, so that the ported arithmetic is theirs.
#[allow(clippy::approx_constant)]
const PI: f64 = 3.14159265358979;
const FS: f64 = 22050.0;

/// Prints a measured value behind a threshold when `LAPRDUS_TEST_INFO` is set.
#[macro_export]
macro_rules! info {
    ($($arg:tt)*) => {
        if std::env::var_os("LAPRDUS_TEST_INFO").is_some() {
            eprintln!("  info: {}", format_args!($($arg)*));
        }
    };
}

/// The rows of a table that fail, so that one run names them all.
#[derive(Default)]
pub struct Failures(Vec<String>);

impl Failures {
    pub fn new() -> Self {
        Self::default()
    }

    /// Notes a row whose check failed; `what` describes it.
    pub fn check(&mut self, passed: bool, what: impl FnOnce() -> String) {
        if !passed {
            self.0.push(what());
        }
    }

    /// Fails the test when a row failed, naming every one.
    #[track_caller]
    pub fn finish(self) {
        assert!(self.0.is_empty(), "{} failed:\n  {}", self.0.len(), self.0.join("\n  "));
    }
}

/// The root of the repository.
pub fn repo() -> &'static Path {
    Path::new(env!("CARGO_MANIFEST_DIR"))
}

/// A bundled dictionary, as `cargo xtask data` stages it in `data/dictionary`.
pub fn dictionary(name: &str) -> PathBuf {
    let path = repo().join("data/dictionary").join(name);
    assert!(path.is_file(), "{} is missing: run `cargo xtask data` first", path.display());
    path
}

/// The directory of the packed recordings (`Josip.bin`, `Vlado.bin`):
/// `LAPRDUS_DATA` when set, else `data/voices` of the repository.
pub fn data_dir() -> PathBuf {
    let dir = std::env::var_os("LAPRDUS_DATA")
        .filter(|dir| !dir.is_empty())
        .map_or_else(|| repo().join("data/voices"), PathBuf::from);
    for file in ["Josip.bin", "Vlado.bin"] {
        assert!(
            dir.join(file).is_file(),
            "{} is missing: run `cargo xtask data` first, or point LAPRDUS_DATA at the packed voices",
            dir.join(file).display()
        );
    }
    dir
}

/// An engine as the tests drive it: the setters a host calls one by one.
pub struct Speaker(pub Engine);

impl Deref for Speaker {
    type Target = Engine;

    fn deref(&self) -> &Engine {
        &self.0
    }
}

impl DerefMut for Speaker {
    fn deref_mut(&mut self) -> &mut Engine {
        &mut self.0
    }
}

impl Speaker {
    /// An engine without a voice.
    pub fn empty() -> Self {
        Self(Engine::new())
    }

    /// An engine with a voice loaded: a formant voice from a directory that
    /// does not exist, a recorded one from [`data_dir`].
    pub fn new(voice: &str) -> Self {
        let mut speaker = Self::empty();
        speaker.set_voice(voice).unwrap_or_else(|error| panic!("{voice}: {error}"));
        speaker
    }

    /// Switches the voice; ids of earlier versions are accepted.
    pub fn set_voice(&mut self, id: &str) -> Result<(), Error> {
        let recorded = voice::find(id).is_some_and(|voice| !voice.is_formant());
        let directory = if recorded { data_dir() } else { PathBuf::from(NO_DATA) };
        self.0.set_voice(id, &directory)
    }

    /// The id of the voice in use.
    pub fn voice_id(&self) -> &'static str {
        self.0.voice().map_or("", |voice| voice.id)
    }

    /// Chooses the language by a tag, as a host does; an empty tag goes back
    /// to the voice's native language. False (and nothing changed) for a tag
    /// of no language.
    pub fn set_language(&mut self, tag: &str) -> bool {
        if tag.is_empty() {
            self.0.set_language(None);
            return true;
        }
        let language = Language::parse(tag);
        if language.is_some() {
            self.0.set_language(language);
        }
        language.is_some()
    }

    /// The code of the language spoken: "hr-HR".
    pub fn language_code(&self) -> &'static str {
        self.0.language().code()
    }

    /// The audio of a text; empty when nothing was synthesized.
    pub fn say(&mut self, text: &str) -> Vec<i16> {
        self.0.synthesize(text).unwrap_or_default()
    }

    /// The audio of a text read character by character, the pause after the
    /// last character included; empty when nothing was synthesized.
    pub fn spell(&mut self, text: &str) -> Vec<i16> {
        self.0.synthesize_spelled(text, true).map(|(audio, _)| audio).unwrap_or_default()
    }

    pub fn set_speed(&mut self, speed: f32) {
        self.0.update_params(|p| p.speed = speed);
    }

    /// The voice character pitch, on top of the voice's base pitch.
    pub fn set_pitch(&mut self, pitch: f32) {
        self.0.set_character_pitch(pitch);
    }

    pub fn set_user_pitch(&mut self, pitch: f32) {
        self.0.update_params(|p| p.user_pitch = pitch);
    }

    pub fn set_volume(&mut self, volume: f32) {
        self.0.update_params(|p| p.volume = volume);
    }

    pub fn set_volume_boost(&mut self, enabled: bool) {
        self.0.update_params(|p| p.volume_boost = enabled);
    }

    pub fn set_inflection_level(&mut self, level: f32) {
        self.0.update_params(|p| p.inflection_level = level);
    }

    pub fn set_acceleration(&mut self, acceleration: f32) {
        self.0.update_params(|p| p.acceleration = acceleration);
    }

    pub fn set_head_size(&mut self, value: i32) {
        self.0.update_params(|p| p.head_size = value);
    }

    pub fn set_breathiness(&mut self, value: i32) {
        self.0.update_params(|p| p.breathiness = value);
    }

    pub fn set_roughness(&mut self, value: i32) {
        self.0.update_params(|p| p.roughness = value);
    }

    pub fn set_brightness(&mut self, value: i32) {
        self.0.update_params(|p| p.brightness = value);
    }

    pub fn set_spelling_mode(&mut self, mode: SpellingMode) {
        self.0.update_params(|p| p.spelling_mode = mode);
    }

    pub fn set_spelling_speed(&mut self, percent: i32) {
        self.0.update_params(|p| p.spelling_speed = percent);
    }

    pub fn set_number_mode(&mut self, mode: NumberMode) {
        self.0.update_params(|p| p.number_mode = mode);
    }

    pub fn set_emoji_enabled(&mut self, enabled: bool) {
        self.0.update_params(|p| p.emoji_enabled = enabled);
    }

    pub fn set_sentence_pause(&mut self, pause_ms: u32) {
        self.0.update_pauses(|pauses| pauses.sentence_pause_ms = pause_ms);
    }

    pub fn set_comma_pause(&mut self, pause_ms: u32) {
        self.0.update_pauses(|pauses| pauses.comma_pause_ms = pause_ms);
    }

    pub fn set_newline_pause(&mut self, pause_ms: u32) {
        self.0.update_pauses(|pauses| pauses.newline_pause_ms = pause_ms);
    }
}

// -----------------------------------------------------------------------------
// Levels
// -----------------------------------------------------------------------------

pub fn rms(audio: &[i16]) -> f64 {
    if audio.is_empty() {
        return 0.0;
    }
    let sum: f64 = audio.iter().map(|&s| f64::from(s) * f64::from(s)).sum();
    (sum / audio.len() as f64).sqrt()
}

/// True when a is within the given fraction of b.
pub fn close_to(a: f64, b: f64, fraction: f64) -> bool {
    (a - b).abs() <= b.abs() * fraction
}

pub fn peak(audio: &[i16]) -> i32 {
    audio.iter().map(|&s| i32::from(s).abs()).max().unwrap_or(0)
}

/// The treble shelf of the volume boost (the Audio EQ Cookbook high shelf,
/// `VOLUME_BOOST_TREBLE_DB` from `VOLUME_BOOST_TREBLE_HZ` up), applied to
/// plain samples so that the boosted ones can be compared with a scaled copy
/// of what the limiter was given.
pub fn treble_shelf(plain: &[i16]) -> Vec<i16> {
    let a = 10f64.powf(f64::from(VOLUME_BOOST_TREBLE_DB) / 40.0);
    let w0 = 2.0 * PI * f64::from(VOLUME_BOOST_TREBLE_HZ) / FS;
    let cw = w0.cos();
    let sa = 2.0 * a.sqrt() * w0.sin() / 2.0 * 2f64.sqrt();
    let a0 = (a + 1.0) - (a - 1.0) * cw + sa;
    let b0 = a * ((a + 1.0) + (a - 1.0) * cw + sa) / a0;
    let b1 = -2.0 * a * ((a - 1.0) + (a + 1.0) * cw) / a0;
    let b2 = a * ((a + 1.0) + (a - 1.0) * cw - sa) / a0;
    let a1 = 2.0 * ((a - 1.0) - (a + 1.0) * cw) / a0;
    let a2 = ((a + 1.0) - (a - 1.0) * cw - sa) / a0;
    let (mut x1, mut x2, mut y1, mut y2) = (0.0, 0.0, 0.0, 0.0);
    plain
        .iter()
        .map(|&sample| {
            let x = f64::from(sample);
            let y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            (x2, x1, y2, y1) = (x1, x, y1, y);
            y.clamp(-32768.0, 32767.0).round() as i16
        })
        .collect()
}

/// How far `processed` departs from a scaled copy of `plain`, in dB: per
/// 10 ms frame the best gain is fitted and the energy of what is left is
/// taken against the fitted copy; the median over the loudest fifth of the
/// frames. A gain that moves only between syllables leaves under -40 dB,
/// clipping of the vowel peaks about -18 dB.
pub fn residual_db(plain: &[i16], processed: &[i16]) -> f64 {
    const FRAME: usize = 220;
    let mut frames: Vec<(f64, f64)> = Vec::new(); // (plain energy, residual)
    let mut start = 0;
    while start + FRAME <= plain.len().min(processed.len()) {
        let (mut pp, mut bp) = (0.0, 0.0);
        for i in start..start + FRAME {
            pp += f64::from(plain[i]) * f64::from(plain[i]);
            bp += f64::from(processed[i]) * f64::from(plain[i]);
        }
        if pp > 0.0 {
            let g = bp / pp;
            let (mut res, mut fit) = (0.0, 0.0);
            for i in start..start + FRAME {
                let f = g * f64::from(plain[i]);
                res += (f64::from(processed[i]) - f) * (f64::from(processed[i]) - f);
                fit += f * f;
            }
            frames.push((pp, 10.0 * (res / (fit + 1e-9) + 1e-12).log10()));
        }
        start += FRAME;
    }
    if frames.is_empty() {
        return 0.0;
    }
    frames.sort_by(|a, b| b.0.total_cmp(&a.0));
    frames.truncate((frames.len() / 5).max(1));
    let mut residuals: Vec<f64> = frames.iter().map(|frame| frame.1).collect();
    residuals.sort_by(f64::total_cmp);
    residuals[residuals.len() / 2]
}

/// Silence inside the audio, between its first and last audible sample, in
/// steps of 2 ms. Audible means above 0.5% of the peak.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Gaps {
    /// Longest silent stretch.
    pub longest_ms: f64,
    /// Audible time after the last one of 20 ms or more.
    pub after_last_ms: f64,
}

pub fn gaps(audio: &[i16]) -> Gaps {
    const STEP: usize = 44;
    let floor = (peak(audio) / 200).max(1);
    let audible: Vec<bool> = audio.chunks_exact(STEP).map(|step| peak(step) > floor).collect();
    let mut first = 0;
    let mut last = audible.len();
    while first < last && !audible[first] {
        first += 1;
    }
    while last > first && !audible[last - 1] {
        last -= 1;
    }

    let mut result = Gaps::default();
    let (mut silent, mut after) = (0usize, 0usize);
    for &heard in &audible[first..last] {
        if !heard {
            silent += 1;
            continue;
        }
        if silent >= 10 {
            after = 0;
        }
        result.longest_ms = result.longest_ms.max(silent as f64 * 2.0);
        silent = 0;
        after += 1;
    }
    result.after_last_ms = after as f64 * 2.0;
    result
}

// -----------------------------------------------------------------------------
// Spectral measures for the balance tests (the reference balance, November
// 2026; the ranges in the tests were set on values measured this way)
// -----------------------------------------------------------------------------

/// Average power spectrum (513 bins of 21.5 Hz) of the 1024-sample Hann
/// frames, hop 512, that lie within 35 dB of the loudest frame.
pub fn power_spectrum(audio: &[i16]) -> Vec<f64> {
    const N: usize = 1024;
    let mut energy = Vec::new();
    let mut s = 0;
    while s + N <= audio.len() {
        let e: f64 = audio[s..s + N].iter().map(|&x| f64::from(x) * f64::from(x)).sum();
        energy.push(e / N as f64);
        s += N / 2;
    }
    let mut spectrum = vec![0.0; N / 2 + 1];
    if energy.is_empty() {
        return spectrum;
    }
    let top = energy.iter().copied().fold(f64::MIN, f64::max);
    let mut frames = 0;
    let mut buf = vec![(0.0f64, 0.0f64); N];
    for (f, &frame_energy) in energy.iter().enumerate() {
        if frame_energy <= top * 10f64.powf(-3.5) {
            continue;
        }
        let s = f * (N / 2);
        for (i, slot) in buf.iter_mut().enumerate() {
            let w = 0.5 - 0.5 * (2.0 * PI * i as f64 / (N - 1) as f64).cos();
            *slot = (f64::from(audio[s + i]) * w, 0.0);
        }
        // iterative radix-2 FFT
        let mut j = 0;
        for i in 1..N {
            let mut bit = N >> 1;
            while j & bit != 0 {
                j ^= bit;
                bit >>= 1;
            }
            j ^= bit;
            if i < j {
                buf.swap(i, j);
            }
        }
        let mut len = 2;
        while len <= N {
            let ang = -2.0 * PI / len as f64;
            let wl = (ang.cos(), ang.sin());
            for i in (0..N).step_by(len) {
                let mut w = (1.0, 0.0);
                for k in 0..len / 2 {
                    let u = buf[i + k];
                    let b = buf[i + k + len / 2];
                    let v = (b.0 * w.0 - b.1 * w.1, b.0 * w.1 + b.1 * w.0);
                    buf[i + k] = (u.0 + v.0, u.1 + v.1);
                    buf[i + k + len / 2] = (u.0 - v.0, u.1 - v.1);
                    w = (w.0 * wl.0 - w.1 * wl.1, w.0 * wl.1 + w.1 * wl.0);
                }
            }
            len <<= 1;
        }
        for (bin, value) in spectrum.iter_mut().zip(&buf) {
            *bin += value.0 * value.0 + value.1 * value.1;
        }
        frames += 1;
    }
    for value in &mut spectrum {
        *value /= f64::from(frames.max(1));
    }
    spectrum
}

/// Energy of the spectrum between lo and hi Hz, in dB.
pub fn band_db(spectrum: &[f64], lo: f64, hi: f64) -> f64 {
    let mut e = 0.0;
    for (i, &value) in spectrum.iter().enumerate() {
        let f = i as f64 * FS / 1024.0;
        if f >= lo && f < hi {
            e += value;
        }
    }
    10.0 * (e + 1e-9).log10()
}

pub fn zero_crossing_rate(x: &[i16]) -> f64 {
    let crossings = x.windows(2).filter(|pair| (pair[1] >= 0) != (pair[0] >= 0)).count();
    if x.len() > 1 { crossings as f64 / (x.len() - 1) as f64 } else { 0.0 }
}

/// Mean energy of `n` samples from `s`; what lies past the end counts as
/// silence.
pub fn frame_energy(audio: &[i16], s: usize, n: usize) -> f64 {
    let e: f64 = audio.iter().skip(s).take(n).map(|&x| f64::from(x) * f64::from(x)).sum();
    e / n as f64
}

pub fn median_of(mut v: Vec<f64>) -> f64 {
    if v.is_empty() {
        return 0.0;
    }
    v.sort_by(f64::total_cmp);
    v[v.len() / 2]
}

/// The starts of the whole frames of `n` samples, `hop` apart.
fn frame_starts(len: usize, n: usize, hop: usize) -> impl Iterator<Item = usize> {
    (0..).map(move |k| k * hop).take_while(move |s| s + n <= len)
}

/// The median level of the friction frames (12 ms, zero-crossing rate above
/// 0.22) against the median of the voiced ones (rate under 0.08), in dB.
pub fn friction_minus_voice_db(audio: &[i16]) -> f64 {
    const N: usize = 265; // 12 ms
    const HOP: usize = 88; // 4 ms
    let top = frame_starts(audio.len(), N, HOP).map(|s| frame_energy(audio, s, N)).fold(0.0, f64::max);
    let (mut friction, mut voiced) = (Vec::new(), Vec::new());
    for s in frame_starts(audio.len(), N, HOP) {
        let e = frame_energy(audio, s, N);
        if e < top * 1e-4 {
            continue;
        }
        let z = zero_crossing_rate(&audio[s..s + N]);
        if z > 0.22 {
            friction.push(10.0 * e.log10());
        } else if z < 0.08 && e > top * 1e-3 {
            voiced.push(10.0 * e.log10());
        }
    }
    median_of(friction) - median_of(voiced)
}

/// The friction of a vowel-consonant-vowel item: the median level of its
/// friction frames (6 ms, hop 2 ms) against the loudest frame, in dB.
pub fn vcv_friction_db(audio: &[i16]) -> f64 {
    const N: usize = 132;
    const HOP: usize = 44;
    let top = frame_starts(audio.len(), N, HOP).map(|s| frame_energy(audio, s, N)).fold(0.0, f64::max);
    let mut levels = Vec::new();
    for s in frame_starts(audio.len(), N, HOP) {
        let e = frame_energy(audio, s, N);
        if e > top * 1e-4 && zero_crossing_rate(&audio[s..s + N]) > 0.22 {
            levels.push(10.0 * (e / top).log10());
        }
    }
    median_of(levels)
}

/// The spectrum of the friction of a vowel-consonant-vowel item: the mean
/// power per bin (129 bins of 86 Hz) of its 256-sample Hann frames, hop 64,
/// that are friction (zero-crossing rate above 0.22, within 40 dB of the
/// loudest frame).
pub fn friction_spectrum(audio: &[i16]) -> Vec<f64> {
    const N: usize = 256;
    const HOP: usize = 64;
    let mut spectrum = vec![0.0; N / 2 + 1];
    let top = frame_starts(audio.len(), N, HOP).map(|s| frame_energy(audio, s, N)).fold(0.0, f64::max);
    let mut frames = 0;
    for s in frame_starts(audio.len(), N, HOP) {
        if frame_energy(audio, s, N) < top * 1e-4 {
            continue;
        }
        if zero_crossing_rate(&audio[s..s + N]) <= 0.22 {
            continue;
        }
        for (k, bin) in spectrum.iter_mut().enumerate() {
            let (mut re, mut im) = (0.0, 0.0);
            for i in 0..N {
                let w = 0.5 - 0.5 * (2.0 * PI * i as f64 / (N - 1) as f64).cos();
                let a = 2.0 * PI * k as f64 * i as f64 / N as f64;
                re += f64::from(audio[s + i]) * w * a.cos();
                im -= f64::from(audio[s + i]) * w * a.sin();
            }
            *bin += re * re + im * im;
        }
        frames += 1;
    }
    for value in &mut spectrum {
        *value /= f64::from(frames.max(1));
    }
    spectrum
}

/// How long the hiss of an item is heard, in ms: the time its sound above
/// 3 kHz (8 ms frames, hop 2 ms) stands within 20 dB of the loudest frame of
/// the whole sound.
pub fn hiss_ms(audio: &[i16]) -> f64 {
    const N: usize = 176;
    const HOP: usize = 44;
    let high = butterworth_3k(audio, true);
    let top = frame_starts(audio.len(), N, HOP).map(|s| frame_energy(audio, s, N)).fold(0.0, f64::max);
    let heard = frame_starts(high.len(), N, HOP)
        .filter(|&s| high[s..s + N].iter().map(|x| x * x).sum::<f64>() / N as f64 > top / 100.0)
        .count();
    heard as f64 * 2.0
}

/// The hiss of the consonant of a vowel-consonant-vowel item, voiced or not:
/// the mean power above 2 kHz of the frames between the two vowels against
/// the loudest frame's whole power, in dB. The frames are 512-sample Hann
/// frames, hop 64, and the consonant is the longest run of them whose
/// 600-1400 Hz band, where an a has its F1 and F2 and no consonant its murmur
/// or noise, is 10 dB or more under its loudest, with a vowel on either side.
/// A voiced fricative has too few zero crossings for `vcv_friction_db()`: its
/// murmur carries the wave.
pub fn vcv_hiss_db(audio: &[i16]) -> f64 {
    const N: usize = 512;
    const HOP: usize = 64;
    let mut frames = Vec::new(); // (whole, vowel band, above 2 kHz)
    for s in frame_starts(audio.len(), N, HOP) {
        let (mut whole, mut vowel, mut hiss) = (0.0, 0.0, 0.0);
        for k in 1..=N / 2 {
            let (mut re, mut im) = (0.0, 0.0);
            for i in 0..N {
                let w = 0.5 - 0.5 * (2.0 * PI * i as f64 / (N - 1) as f64).cos();
                let a = 2.0 * PI * k as f64 * i as f64 / N as f64;
                re += f64::from(audio[s + i]) * w * a.cos();
                im -= f64::from(audio[s + i]) * w * a.sin();
            }
            let power = re * re + im * im;
            let f = k as f64 * FS / N as f64;
            whole += power;
            if (600.0..1400.0).contains(&f) {
                vowel += power;
            } else if f >= 2000.0 {
                hiss += power;
            }
        }
        frames.push((whole, vowel, hiss));
    }
    let top = frames.iter().map(|f| f.0).fold(0.0, f64::max);
    let floor = frames.iter().map(|f| f.1).fold(0.0, f64::max) / 10.0;
    let first = frames.iter().position(|f| f.1 > floor);
    let last = frames.iter().rposition(|f| f.1 > floor);
    let (Some(first), Some(last)) = (first, last) else { return f64::NAN };
    // the longest run under the floor between the two vowels
    let (mut best, mut run) = (0..0, first..first);
    for (i, frame) in frames.iter().enumerate().take(last + 1).skip(first) {
        if frame.1 < floor {
            if run.end != i {
                run = i..i;
            }
            run.end = i + 1;
            if run.len() > best.len() {
                best = run.clone();
            }
        }
    }
    if best.is_empty() {
        return f64::NAN;
    }
    let hiss: f64 = frames[best.clone()].iter().map(|f| f.2).sum();
    10.0 * (hiss / best.len() as f64 / top).log10()
}

/// The share of a friction spectrum's energy above 2 kHz that lies between lo
/// and hi Hz.
pub fn friction_share(spectrum: &[f64], lo: f64, hi: f64) -> f64 {
    let (mut part, mut total) = (0.0, 0.0);
    for (k, &value) in spectrum.iter().enumerate() {
        let f = k as f64 * FS / 256.0;
        if f < 2000.0 {
            continue;
        }
        total += value;
        if f >= lo && f < hi {
            part += value;
        }
    }
    if total > 0.0 { part / total } else { 0.0 }
}

/// The frequency under which half of a friction spectrum's energy above
/// 2 kHz lies.
pub fn friction_middle_hz(spectrum: &[f64]) -> f64 {
    let hz = |k: usize| k as f64 * FS / 256.0;
    let total: f64 = spectrum.iter().enumerate().filter(|&(k, _)| hz(k) >= 2000.0).map(|(_, &value)| value).sum();
    let mut sum = 0.0;
    for (k, &value) in spectrum.iter().enumerate() {
        if hz(k) < 2000.0 {
            continue;
        }
        sum += value;
        if sum >= total * 0.5 {
            return hz(k);
        }
    }
    0.0
}

/// The quietest 12 ms of the middle third of an item (the h of "aha") against
/// the loudest frame, in dB.
pub fn vcv_h_db(audio: &[i16]) -> f64 {
    const N: usize = 265;
    const HOP: usize = 44;
    let levels: Vec<f64> = frame_starts(audio.len(), N, HOP).map(|s| frame_energy(audio, s, N)).collect();
    let top = levels.iter().copied().fold(f64::MIN, f64::max);
    let lowest = levels[levels.len() / 3..2 * levels.len() / 3].iter().copied().fold(top, f64::min);
    10.0 * ((lowest + 1e-9) / top).log10()
}

/// The burst of a stop in a vowel-consonant-vowel item: the loudest 6 ms
/// within 10 ms after the first silent gap (12 ms or more under -30 dB),
/// against the loudest frame of the item, in dB; a longer window would reach
/// the vowel's onset. NaN without a gap.
pub fn vcv_burst_db(audio: &[i16]) -> f64 {
    const N: usize = 132;
    const HOP: usize = 44;
    let levels: Vec<f64> = frame_starts(audio.len(), N, HOP).map(|s| frame_energy(audio, s, N)).collect();
    let top = levels.iter().copied().fold(f64::MIN, f64::max);
    let mut i = 0;
    while i < levels.len() && levels[i] < top * 1e-2 {
        i += 1;
    }
    let mut j = i;
    while j < levels.len() {
        if levels[j] < top * 1e-3 {
            let mut k = j;
            while k < levels.len() && levels[k] < top * 1e-3 {
                k += 1;
            }
            if k - j >= 6 {
                let burst = levels.iter().skip(k).take(3).copied().fold(0.0, f64::max);
                return 10.0 * ((burst + 1e-9) / top).log10();
            }
            j = k;
        } else {
            j += 1;
        }
    }
    f64::NAN
}

// -----------------------------------------------------------------------------
// Pitch
// -----------------------------------------------------------------------------

pub fn energy_at(audio: &[i16], start: usize, window: usize) -> f64 {
    audio[start..start + window].iter().map(|&x| f64::from(x) * f64::from(x)).sum()
}

/// The normalized autocorrelation of `window` samples from `start` at a lag.
fn correlation(audio: &[i16], start: usize, window: usize, lag: usize) -> f64 {
    let (mut sum, mut left, mut right) = (0.0, 0.0, 0.0);
    let mut i = 0;
    while i + lag < window {
        let a = f64::from(audio[start + i]);
        let b = f64::from(audio[start + i + lag]);
        sum += a * b;
        left += a * a;
        right += b * b;
        i += 1;
    }
    if left > 0.0 && right > 0.0 { sum / (left * right).sqrt() } else { 0.0 }
}

/// Fundamental frequency of one window, by normalized autocorrelation. Takes
/// the shortest period that correlates nearly as well as the best one (a
/// local maximum within 10% of it), so a moving pitch, or a window that
/// takes in the voice bar of a b, is not mistaken for its octave below.
pub fn pitch_at(audio: &[i16], start: usize, window: usize) -> f64 {
    let min_lag = 22050 / 400;
    let max_lag = (22050 / 50).min(window - 1);
    let mut corr = vec![0.0; max_lag + 2];
    let mut best = 0.0f64;
    let mut lag = min_lag - 1;
    while lag <= max_lag + 1 && lag < window {
        corr[lag] = correlation(audio, start, window, lag);
        if (min_lag..=max_lag).contains(&lag) {
            best = best.max(corr[lag]);
        }
        lag += 1;
    }
    if best <= 0.0 {
        return 0.0;
    }
    for lag in min_lag..=max_lag {
        if corr[lag] >= 0.9 * best && corr[lag] >= corr[lag - 1] && corr[lag] >= corr[lag + 1] {
            return FS / lag as f64;
        }
    }
    0.0
}

/// The window starts (2048 samples, hop 256) within [from, to), fractions of
/// the audio; none when the stretch is shorter than a window.
fn pitch_windows(len: usize, from: f64, to: f64) -> impl Iterator<Item = usize> {
    const WINDOW: usize = 2048;
    let begin = (len as f64 * from) as usize;
    let end = (len as f64 * to) as usize;
    (0..).map(move |k| begin + k * 256).take_while(move |start| start + WINDOW <= end)
}

/// Fundamental frequency of the loudest ~93 ms of the audio.
pub fn pitch_hz(audio: &[i16]) -> f64 {
    pitch_hz_in(audio, 0.0, 1.0)
}

/// Fundamental frequency of the loudest ~93 ms within [from, to) (fractions
/// of the audio).
pub fn pitch_hz_in(audio: &[i16], from: f64, to: f64) -> f64 {
    const WINDOW: usize = 2048;
    let mut windows = pitch_windows(audio.len(), from, to).peekable();
    let Some(&begin) = windows.peek() else { return 0.0 };
    let mut best_start = begin;
    let mut best_energy = 0.0;
    for start in windows {
        let energy = energy_at(audio, start, WINDOW);
        if energy > best_energy {
            best_energy = energy;
            best_start = start;
        }
    }
    pitch_at(audio, best_start, WINDOW)
}

/// The highest pitch of the windows (2048 samples, hop 256) that carry at
/// least a tenth of the loudest window's energy: the peak of a melody,
/// wherever the loudest window falls (with the low shelf of "The reference
/// balance" the loudest window of a short word is its onset).
pub fn peak_pitch_hz(audio: &[i16]) -> f64 {
    peak_pitch_hz_in(audio, 0.0, 1.0)
}

/// [`peak_pitch_hz`] of the windows between `from` and `to` (fractions of
/// the audio).
pub fn peak_pitch_hz_in(audio: &[i16], from: f64, to: f64) -> f64 {
    const WINDOW: usize = 2048;
    let top = pitch_windows(audio.len(), from, to).map(|start| energy_at(audio, start, WINDOW)).fold(0.0, f64::max);
    let mut best = 0.0f64;
    for start in pitch_windows(audio.len(), from, to) {
        if energy_at(audio, start, WINDOW) > 0.1 * top {
            best = best.max(pitch_at(audio, start, WINDOW));
        }
    }
    best
}

/// Where the pitch is highest, in ms from the start: the highest of the
/// 40 ms windows (10 ms apart) loud and periodic enough to be a vowel; -1
/// when there is none.
pub fn pitch_peak_ms(audio: &[i16]) -> f64 {
    const WINDOW: usize = 882;
    const HOP: usize = 220;
    let energy: Vec<f64> = frame_starts(audio.len(), WINDOW, HOP).map(|s| energy_at(audio, s, WINDOW)).collect();
    let loudest = energy.iter().copied().fold(0.0, f64::max);
    let mut highest = 0.0;
    let mut at = -1.0;
    for (k, &e) in energy.iter().enumerate() {
        if e < 0.2 * loudest {
            continue;
        }
        let f0 = pitch_at(audio, k * HOP, WINDOW);
        if f0 <= 0.0 {
            continue;
        }
        // A consonant's noise has no period to repeat.
        let lag = (FS / f0 + 0.5) as usize;
        let (mut sum, mut left, mut right) = (0.0, 0.0, 0.0);
        let mut i = k * HOP;
        while i + lag < k * HOP + WINDOW {
            sum += f64::from(audio[i]) * f64::from(audio[i + lag]);
            left += f64::from(audio[i]) * f64::from(audio[i]);
            right += f64::from(audio[i + lag]) * f64::from(audio[i + lag]);
            i += 1;
        }
        if sum < 0.5 * (left * right).sqrt() {
            continue;
        }
        if f0 > highest {
            highest = f0;
            at = (k * HOP + WINDOW / 2) as f64 * 1000.0 / FS;
        }
    }
    at
}

/// Fundamental frequency where the voice ends: the last ~35 ms that are
/// clearly periodic and not yet faded out. Takes the shortest period that
/// correlates nearly as well as the best one, so a moving pitch is not
/// mistaken for its octave below. The search stays in the last stretch of
/// sound, where the most periodic window is taken if none is periodic
/// enough. With a 46 ms window a short final vowel falling fast at 2x fell
/// under the threshold, and the search went on into the words before it
/// (Mirsad's "Tko je to." read 133 Hz where the voice ends at 110 Hz).
pub fn final_pitch_hz(audio: &[i16]) -> f64 {
    const WINDOW: usize = 768;
    let min_lag = 22050 / 300;
    let max_lag = 22050 / 60;
    if audio.len() < WINDOW {
        return 0.0;
    }
    let loudest = frame_starts(audio.len(), WINDOW, 128).map(|s| energy_at(audio, s, WINDOW)).fold(0.0, f64::max);

    let mut corr = vec![0.0; max_lag + 2];
    let mut in_sound = false;
    let mut fallback_hz = 0.0;
    let mut fallback_corr = 0.0;
    let mut start = audio.len() - WINDOW;
    while start >= 128 {
        if energy_at(audio, start, WINDOW) < 0.03 * loudest {
            if in_sound {
                break;
            }
            start -= 128;
            continue;
        }
        in_sound = true;
        let mut best = 0.0f64;
        for (lag, value) in corr.iter_mut().enumerate().skip(min_lag - 1) {
            *value = correlation(audio, start, WINDOW, lag);
            if (min_lag..=max_lag).contains(&lag) {
                best = best.max(*value);
            }
        }
        let mut hz = 0.0;
        for lag in min_lag..=max_lag {
            if corr[lag] >= 0.9 * best && corr[lag] >= corr[lag - 1] && corr[lag] >= corr[lag + 1] {
                hz = FS / lag as f64;
                break;
            }
        }
        if best >= 0.7 {
            return hz;
        }
        if best > fallback_corr {
            fallback_corr = best;
            fallback_hz = hz;
        }
        start -= 128;
    }
    fallback_hz
}

/// Pitch of the loudest window in each quarter of the audio, to see how much
/// the voice moves along a clause.
pub fn pitch_spread_hz(audio: &[i16]) -> f64 {
    let (mut lo, mut hi) = (1e9f64, 0.0f64);
    for q in 0..4 {
        let hz = pitch_hz_in(audio, f64::from(q) * 0.25, f64::from(q + 1) * 0.25);
        if hz <= 0.0 {
            continue;
        }
        lo = lo.min(hz);
        hi = hi.max(hz);
    }
    if hi > lo { hi - lo } else { 0.0 }
}

// -----------------------------------------------------------------------------
// Hiss and true peak (the hiss limiter of boost mode and the brightness tilt,
// November 2026)
// -----------------------------------------------------------------------------

/// One second-order Butterworth section at 3 kHz (bilinear, as the Audio EQ
/// Cookbook gives it), high- or low-pass.
pub fn butterworth_3k(x: &[i16], highpass: bool) -> Vec<f64> {
    let w0 = 2.0 * PI * 3000.0 / FS;
    let cw = w0.cos();
    let alpha = w0.sin() / 2.0 * 2f64.sqrt();
    let a0 = 1.0 + alpha;
    let b0 = (if highpass { 1.0 + cw } else { 1.0 - cw }) / 2.0 / a0;
    let b1 = (if highpass { -(1.0 + cw) } else { 1.0 - cw }) / a0;
    let a1 = -2.0 * cw / a0;
    let a2 = (1.0 - alpha) / a0;
    let (mut x1, mut x2, mut y1, mut y2) = (0.0, 0.0, 0.0, 0.0);
    x.iter()
        .map(|&sample| {
            let sample = f64::from(sample);
            let v = b0 * sample + b1 * x1 + b0 * x2 - a1 * y1 - a2 * y2;
            (x2, x1, y2, y1) = (x1, sample, y1, v);
            v
        })
        .collect()
}

/// The hiss of two renderings of the same item, measured on the 12 ms frames
/// where the first one's band above 3 kHz outweighs the rest (a sibilant, not
/// a vowel's own top): the loudest such frame's band above 3 kHz, each re its
/// own loudest frame, in dB.
pub fn hiss_pair_db(plain: &[i16], other: &[i16]) -> (f64, f64) {
    const N: usize = 265;
    const HOP: usize = 88;
    let (ph, pl) = (butterworth_3k(plain, true), butterworth_3k(plain, false));
    let oh = butterworth_3k(other, true);
    let len = plain.len().min(other.len());
    let (mut top_p, mut top_o, mut hiss_p, mut hiss_o) = (1e-9f64, 1e-9f64, 1e-9f64, 1e-9f64);
    for s in frame_starts(len, N, HOP) {
        let (mut ep, mut eo, mut hp, mut lp, mut ho) = (0.0, 0.0, 0.0, 0.0, 0.0);
        for i in s..s + N {
            ep += f64::from(plain[i]) * f64::from(plain[i]);
            eo += f64::from(other[i]) * f64::from(other[i]);
            hp += ph[i] * ph[i];
            lp += pl[i] * pl[i];
            ho += oh[i] * oh[i];
        }
        top_p = top_p.max(ep);
        top_o = top_o.max(eo);
        if hp > lp {
            hiss_p = hiss_p.max(hp);
            hiss_o = hiss_o.max(ho);
        }
    }
    (10.0 * (hiss_p / top_p).log10(), 10.0 * (hiss_o / top_o).log10())
}

/// The true peak: the largest magnitude of the signal 8 times oversampled by
/// a 64-tap Hann-windowed sinc (what a phone's resampler reconstructs).
pub fn true_peak(x: &[i16]) -> f64 {
    const TAPS: usize = 32;
    let mut best = x.iter().map(|&v| f64::from(v).abs()).fold(0.0, f64::max);
    for f in 1..8 {
        let frac = f64::from(f) / 8.0;
        // w[j] weighs the sample at offset j + 1 - TAPS.
        let w: [f64; 2 * TAPS] = std::array::from_fn(|j| {
            let t = (j as f64 + 1.0 - TAPS as f64) - frac;
            let sinc = if t.abs() < 1e-12 { 1.0 } else { (PI * t).sin() / (PI * t) };
            let hann = 0.5 + 0.5 * (PI * t / TAPS as f64).cos();
            sinc * hann
        });
        let mut i = TAPS;
        while i + TAPS < x.len() {
            let window = &x[i + 1 - TAPS..=i + TAPS];
            let v: f64 = w.iter().zip(window).map(|(w, &sample)| w * f64::from(sample)).sum();
            best = best.max(v.abs());
            i += 1;
        }
    }
    best
}

/// What of `full` is not a scaled copy of `half`: the residual after the best
/// gain over every 2 ms (a limiter's gain barely moves there, a clipper's
/// waveshaping does not go away), re `full`, in dB.
pub fn waveshaping_db(full: &[i16], half: &[i16]) -> f64 {
    const M: usize = 44;
    let n = full.len().min(half.len());
    let (mut res, mut total) = (0.0, 0.0);
    for s in frame_starts(n, M, M) {
        let (mut xy, mut xx) = (0.0, 0.0);
        for i in s..s + M {
            xy += f64::from(half[i]) * f64::from(full[i]);
            xx += f64::from(half[i]) * f64::from(half[i]);
        }
        let g = if xx > 0.0 { xy / xx } else { 0.0 };
        for i in s..s + M {
            let r = f64::from(full[i]) - g * f64::from(half[i]);
            res += r * r;
            total += f64::from(full[i]) * f64::from(full[i]);
        }
    }
    10.0 * (res / (total + 1e-9) + 1e-15).log10()
}

// -----------------------------------------------------------------------------
// The loudest window: periodicity and single components
// -----------------------------------------------------------------------------

/// The loudest 2048 samples (93 ms) of the audio.
pub fn loudest_window(audio: &[i16]) -> Vec<f64> {
    const WINDOW: usize = 2048;
    let mut best_start = 0;
    let mut best_energy = -1.0;
    for start in frame_starts(audio.len(), WINDOW, 256) {
        let energy = energy_at(audio, start, WINDOW);
        if energy > best_energy {
            best_energy = energy;
            best_start = start;
        }
    }
    audio.iter().skip(best_start).take(WINDOW).map(|&x| f64::from(x)).collect()
}

/// How periodic the loudest window is: the best normalised autocorrelation
/// at a lag of 2.5-20 ms (1 a steady voice, near 0 noise).
pub fn periodicity(audio: &[i16]) -> f64 {
    let mut x = loudest_window(audio);
    let mean = x.iter().sum::<f64>() / x.len() as f64;
    for v in &mut x {
        *v -= mean;
    }
    let mut best = 0.0f64;
    let mut lag = 22050 / 400;
    while lag < 22050 / 50 && lag < x.len() {
        let (mut sum, mut left, mut right) = (0.0, 0.0, 0.0);
        for i in 0..x.len() - lag {
            sum += x[i] * x[i + lag];
            left += x[i] * x[i];
            right += x[i + lag] * x[i + lag];
        }
        if left > 0.0 && right > 0.0 {
            best = best.max(sum / (left * right).sqrt());
        }
        lag += 1;
    }
    best
}

/// The level of the strongest component within 12 Hz of `freq` in the
/// loudest window (Hann window, Goertzel at 1 Hz steps), in dB.
pub fn component_db(audio: &[i16], freq: f64) -> f64 {
    let x = loudest_window(audio);
    let n = x.len() as f64;
    let mut best = 0.0f64;
    let mut f = freq - 12.0;
    while f <= freq + 12.0 {
        let (mut re, mut im) = (0.0, 0.0);
        for (i, &sample) in x.iter().enumerate() {
            let w = 0.5 - 0.5 * (2.0 * PI * i as f64 / (n - 1.0)).cos();
            let phase = 2.0 * PI * f * i as f64 / FS;
            re += w * sample * phase.cos();
            im -= w * sample * phase.sin();
        }
        best = best.max(re * re + im * im);
        f += 1.0;
    }
    10.0 * (best + 1e-12).log10()
}

// -----------------------------------------------------------------------------
// Spelling checks shared by the formant and the recorded voices
// -----------------------------------------------------------------------------

/// Text as it is spoken, or spelled with the pause after the last character;
/// both must give audio.
fn render(speaker: &mut Speaker, text: &str, spelled: bool) -> Vec<i16> {
    let audio = if spelled { speaker.spell(text) } else { speaker.say(text) };
    assert!(!audio.is_empty(), "no audio for {text:?} (spelled: {spelled})");
    audio
}

/// A typed character must use the selected language even with the Croatian
/// bundled dictionary loaded. Run this for both synthesis backends.
pub fn check_slovenian_spelling(speaker: &mut Speaker) {
    assert!(speaker.load_spelling_dictionary_file(&dictionary("spelling.json")));
    assert!(speaker.set_language("sl"));
    speaker.set_spelling_pause(0);
    speaker.set_spelling_speed(100);
    for mode in [SpellingMode::LetterNames, SpellingMode::LetterSounds] {
        speaker.set_spelling_mode(mode);
        for (character, name) in [
            ("1", "ena"),
            ("7", "sedem"),
            ("8", "osem"),
            (".", "pika"),
            (",", "vejica"),
            ("@", "afna"),
            (" ", "presledek"),
        ] {
            assert!(render(speaker, character, true) == render(speaker, name, false), "{mode:?}: {character:?}");
        }
        // By names dvojni ve, by sounds the v is a sound: as in running text
        assert!(render(speaker, "w", true) == render(speaker, "w", false), "{mode:?}: w");
    }
    speaker.set_spelling_mode(SpellingMode::LetterSounds);
    assert!(render(speaker, "w", true) == render(speaker, "dvojni v", false));
    assert!(render(speaker, "2", true) == render(speaker, "dve", false));
    speaker.add_spelling_entry("@", "moj znak");
    assert!(render(speaker, "@", true) == render(speaker, "moj znak", false));
    assert!(speaker.set_language("hr"));
    assert!(render(speaker, ".", true) == render(speaker, "točka", false));
    assert!(render(speaker, "7", true) == render(speaker, "sedam", false));
    assert!(render(speaker, "@", true) == render(speaker, "moj znak", false));
}

/// Shared PCM regression checks for formant and recorded voices. In
/// particular, a longer buffer alone must not let slowed-down speech pass as
/// a longer pause.
pub fn check_spelling_pauses(speaker: &mut Speaker) {
    const CHARACTERS: [&str; 4] = ["b", "Č", "7", "*"];
    speaker.add_spelling_entry("*", "moja zvjezdica");
    speaker.set_acceleration(1.7);
    speaker.set_user_pitch(1.2);

    for mode in [SpellingMode::LetterNames, SpellingMode::LetterSounds] {
        speaker.set_spelling_mode(mode);
        for rate in [0.25f32, 1.0, 4.0] {
            speaker.set_speed(rate);
            let normal = render(speaker, "Običan govor.", false);
            for character in CHARACTERS {
                let context = format!("mode {mode:?}, rate {rate}, character {character}");
                speaker.set_spelling_pause(0);
                speaker.set_spelling_speed(100);
                let full = render(speaker, character, true);
                if mode == SpellingMode::LetterNames && character == "b" {
                    assert!(full == render(speaker, "be", false), "{context}");
                }
                // The silence added is the character's duration times
                // 2^((100 - speed) / 100) - 1: 0.41 at 50, 0.74 at 20, 1.0 at 0
                let mut previous_size = full.len();
                for (speed, share) in [(50, 0.4142), (20, 0.7411), (0, 1.0)] {
                    let context = format!("{context}, spelling speed {speed}");
                    speaker.set_spelling_speed(speed);
                    let spaced = render(speaker, character, true);
                    assert!(spaced.len() > previous_size, "{context}");
                    assert!(spaced[..full.len()] == full[..], "{context}");
                    assert!(spaced[full.len()..].iter().all(|&sample| sample == 0), "{context}");
                    let added = (spaced.len() - full.len()) as f64;
                    assert!((added - full.len() as f64 * share).abs() <= 2.0, "{context}");
                    previous_size = spaced.len();

                    // A host may leave the final silence out and get its length
                    let (open, final_pause_ms) = speaker.synthesize_spelled(character, false).expect(&context);
                    assert!(!open.is_empty(), "{context}");
                    assert!(open == full, "{context}");
                    assert_eq!(final_pause_ms, (added * 1000.0 / 22050.0).round() as u32, "{context}");
                    let (closed, closed_pause_ms) = speaker.synthesize_spelled(character, true).expect(&context);
                    assert_eq!(closed.len(), spaced.len(), "{context}");
                    assert_eq!(closed_pause_ms, final_pause_ms, "{context}");
                }
            }
            // The configured pause adds to the speed-dependent silence after
            // every character, including the last. UTF-8 letters count once.
            for speed in [100, 50, 0] {
                let context = format!("mode {mode:?}, rate {rate}, spelling speed {speed}");
                speaker.set_spelling_speed(speed);
                speaker.set_spelling_pause(0);
                let no_base_pause = render(speaker, "bČ7*", true);
                speaker.set_spelling_pause(200);
                let together = render(speaker, "bČ7*", true);
                let separate: Vec<i16> =
                    CHARACTERS.iter().flat_map(|character| render(speaker, character, true)).collect();
                assert!(together == separate, "{context}");
                assert_eq!(together.len(), no_base_pause.len() + 4 * 4410, "{context}");
                let mut capital = render(speaker, "veliko", false);
                capital.extend(render(speaker, "N", true));
                assert!(render(speaker, "veliko N", false) == capital, "{context}");
            }
            assert!(render(speaker, "Običan govor.", false) == normal, "mode {mode:?}, rate {rate}");
        }
    }
}
