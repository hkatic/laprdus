//! Clause synthesis with a recorded voice.
//!
//! The recordings are analysed when the synthesizer is created (pitch marks,
//! voicing, sounding part) and again whenever the voice character pitch or
//! the head size changes, because those warp the spectrum of the recordings.

use super::prosody::plan_clause;
use super::psola::render;
use super::unit_bank::UnitBank;
use super::voice_data::VoiceData;
use crate::audio::boost::{Boost, boost_volume, limit_hiss};
use crate::audio::brightness::tilt_brightness;
use crate::params::{RECORDED_HISS_DB, VoiceParams, head_size_factor};
use crate::prosody::{Utterance, split_at_breaks};
use crate::text::{Frontend, Punctuation};
use crate::{SAMPLE_RATE, silence};

/// Output above this level is bent back softly instead of clipped.
const LIMIT_KNEE: f32 = 0.90;

/// `limited` output (the volume boost's) is already held under full scale and
/// is not bent again: a second knee would only add distortion.
fn to_sample(v: f32, limited: bool) -> i16 {
    let mut a = v.abs();
    if !limited && a > LIMIT_KNEE {
        a = LIMIT_KNEE + (1.0 - LIMIT_KNEE) * ((a - LIMIT_KNEE) / (1.0 - LIMIT_KNEE)).tanh();
    }
    (a.copysign(v) * 32767.0).clamp(-32768.0, 32767.0).round() as i16
}

/// Spectrum warp applied to the recordings for a voice character pitch. A
/// child's vocal tract is shorter than an adult's by less than its pitch is
/// higher: the formants move by the square root of the pitch factor (1.5 ->
/// 1.22, 0.75 -> 0.87).
pub fn formant_warp_for_pitch(pitch: f32) -> f32 {
    if pitch > 0.0 { pitch.sqrt() } else { 1.0 }
}

/// Synthesizes clauses with one recorded voice.
#[derive(Clone, Debug)]
pub struct ConcatSynthesizer {
    data: VoiceData,
    params: VoiceParams,
    bank: Option<UnitBank>,
    /// Warp the bank was built with (0: not built).
    bank_warp: f32,
    natural_f0: f32,
}

impl ConcatSynthesizer {
    /// Creates the synthesizer and analyses the recordings.
    pub fn new(data: VoiceData) -> Self {
        let mut synth = Self { data, params: VoiceParams::default(), bank: None, bank_warp: 0.0, natural_f0: 0.0 };
        synth.ensure_bank();
        synth
    }

    pub fn data(&self) -> &VoiceData {
        &self.data
    }

    pub fn voice_params(&self) -> &VoiceParams {
        &self.params
    }

    pub fn set_voice_params(&mut self, params: &VoiceParams) {
        self.params = *params;
        self.params.clamp();
        self.ensure_bank();
    }

    /// Pitch of the voice as recorded (Hz), 0 if no recordings are loaded.
    pub fn natural_f0(&self) -> f32 {
        self.natural_f0
    }

    /// The head size setting lengthens or shortens the vocal tract on top of
    /// the character pitch's warp; the bank is rebuilt only when the warp
    /// moves (a settings change, not every clause).
    fn ensure_bank(&mut self) {
        let warp = formant_warp_for_pitch(self.params.pitch) * head_size_factor(self.params.head_size);
        if self.bank_warp > 0.0 && (warp - self.bank_warp).abs() < 1e-3 {
            return;
        }
        self.bank = UnitBank::build(&self.data, warp);
        self.bank_warp = warp;
        // The bank's pitch is that of the warped recordings; the voice's own
        // is the unwarped one, and the character pitch is applied by the
        // planner.
        self.natural_f0 = self.bank.as_ref().map_or(120.0, UnitBank::base_f0) / warp;
    }

    /// Synthesizes one clause: `text` without its punctuation, which is
    /// `punct` and chooses the melody. See
    /// [`FormantSynthesizer::synthesize_clause`](crate::formant::synthesizer::FormantSynthesizer::synthesize_clause)
    /// for the other arguments. No pause is appended.
    #[allow(clippy::too_many_arguments)]
    pub fn synthesize_clause(
        &mut self,
        frontend: &dyn Frontend,
        text: &[char],
        punct: Punctuation,
        sentence_initial: bool,
        letter_sounds: bool,
        minor_pause_ms: u32,
        item_pause_ms: u32,
    ) -> Vec<i16> {
        if text.is_empty() {
            return Vec::new();
        }
        self.ensure_bank();
        let mut utt = frontend.process(text, punct, letter_sounds);
        utt.sentence_initial = sentence_initial;
        // The short pauses inside the clause (a clock time, a numeric date, a
        // cell's address), as in the formant voices: the parts one after
        // another with a silence between them.
        let parts = split_at_breaks(&utt);
        let mut audio = Vec::new();
        for (k, part) in parts.iter().enumerate() {
            let samples = self.render_utterance(part);
            if samples.is_empty() {
                continue;
            }
            audio.extend(samples);
            if k + 1 < parts.len() {
                audio.extend(silence(if part.item_break { item_pause_ms } else { minor_pause_ms }));
            }
        }
        audio
    }

    /// The sound of one letter alone, for spelling by sounds; empty for an
    /// unknown letter.
    pub fn synthesize_letter_sound(&mut self, frontend: &dyn Frontend, letter: &[char]) -> Vec<i16> {
        if letter.is_empty() {
            return Vec::new();
        }
        self.ensure_bank();
        self.render_utterance(&frontend.letter_sound(letter))
    }

    /// Renders one utterance.
    pub fn render_utterance(&self, utt: &Utterance) -> Vec<i16> {
        let Some(bank) = self.bank.as_ref().filter(|_| !utt.phones.is_empty()) else { return Vec::new() };
        let params = &self.params;
        let fs = if self.data.sample_rate() > 0 { self.data.sample_rate() } else { SAMPLE_RATE };
        let plan = plan_clause(utt, bank, params, self.natural_f0, fs);
        if plan.segments.is_empty() {
            return Vec::new();
        }

        let mut wave = render(&plan.segments, &plan.contour, fs, plan.total_samples);

        // Cut the tail back to the clause plus whatever the last window left.
        let end = plan.total_samples.ceil() as usize;
        let sounding = wave.iter().rposition(|sample| sample.abs() >= 1e-4).map_or(0, |last| last + 1);
        wave.truncate(end.max(sounding));

        tilt_brightness(&mut wave, fs, params.brightness);

        // The volume boost raises the level before the volume and holds the
        // peaks under its ceiling. The plain output goes through the same
        // limiter at unity gain after the volume, with its ceiling at the
        // soft knee: the recordings touch full scale, and at full volume
        // Vlado and Đedo drove the knee so hard that above 5 kHz half of
        // Đedo's sound was its distortion (November 2026). The limiter's gain
        // moves only between syllables, so nothing is waveshaped, and an
        // utterance that stays under the knee, between the samples too, is
        // bit for bit what it was (every lower volume included).
        let boosted = params.volume_boost;
        if boosted {
            boost_volume(&mut wave, fs, &Boost::default());
        } else if RECORDED_HISS_DB < 0.0 {
            // The hissing consonants of the recordings stand level with the
            // vowels: the plain output holds them where a commercial recorded
            // voice has them; boost mode has its own, lower hold.
            limit_hiss(&mut wave, fs as f32, RECORDED_HISS_DB);
        }
        wave.iter_mut().for_each(|x| *x *= params.volume);
        if !boosted {
            boost_volume(&mut wave, fs, &Boost::limiter(LIMIT_KNEE));
        }
        wave.into_iter().map(|x| to_sample(x, boosted)).collect()
    }
}
