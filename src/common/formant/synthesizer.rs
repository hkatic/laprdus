//! Rule-based formant speech synthesis: one clause at a time.
//!
//! Rate, pitch and volume are applied at the source (segment durations,
//! fundamental frequency, output gain), so there is no time-stretching or
//! pitch-shifting artefact at any setting.

use super::builder::{ClauseBuilder, Melody};
use super::klatt::KlattSynth;
use super::singing::Note;
use super::voice::FormantVoice;
use crate::audio::boost::{Boost, boost_volume};
use crate::audio::brightness::tilt_brightness;
use crate::engine::CancelHandle;
use crate::params::{SpellingMode, VoiceParams};
use crate::prosody::{Utterance, split_at_breaks};
use crate::text::{Frontend, Punctuation, breath_groups};
use crate::{SAMPLE_RATE, silence};

/// Output level, set so the formant voices are about as loud as the recorded
/// ones. Samples below `LIMIT_KNEE` pass unchanged; the few peaks above it
/// are rounded off instead of clipped.
const OUTPUT_GAIN: f32 = 0.28;
const LIMIT_KNEE: f32 = 0.8;

/// Synthesizes clauses with one formant voice.
#[derive(Clone, Debug)]
pub struct FormantSynthesizer {
    voice: FormantVoice,
    synth: KlattSynth,
    /// Singing presets: the song ...
    melody: Vec<Note>,
    /// ... and the next note to sing.
    cursor: usize,
    /// The engine's request to stop: what is rendered so far is returned.
    cancel: CancelHandle,
}

/// Frames rendered between two looks at the request to stop: about a second
/// of sound.
const FRAMES_PER_CANCEL_CHECK: usize = 512;

impl FormantSynthesizer {
    pub fn new(voice: &FormantVoice) -> Self {
        Self {
            voice: *voice,
            synth: KlattSynth::new(&voice.quality(&VoiceParams::default())),
            melody: voice.singing.map_or_else(Vec::new, |style| style.song.notes()),
            cursor: 0,
            cancel: CancelHandle::default(),
        }
    }

    /// The handle whose `cancel()` cuts a synthesis short: the engine's.
    pub fn set_cancel_handle(&mut self, cancel: CancelHandle) {
        self.cancel = cancel;
    }

    pub fn voice(&self) -> &FormantVoice {
        &self.voice
    }

    /// True for the singing presets.
    pub fn is_singing(&self) -> bool {
        self.voice.singing.is_some()
    }

    /// Singing presets: start the song from its first note again. Called at
    /// the start of every utterance, so each one begins the song, and the
    /// clauses of one utterance continue it.
    pub fn rewind_song(&mut self) {
        self.cursor = 0;
    }

    /// Synthesizes a clause: `text` without its final punctuation mark, which
    /// is `punct`. `sentence_initial` is false when the clause goes on a
    /// sentence (the clause before it ended with a comma, semicolon or
    /// colon). `minor_pause_ms` is the length of a short pause inside the
    /// clause (a clock time, a numeric date), `item_pause_ms` that of the
    /// pause between two items (before a cell's address); a singing preset
    /// has neither. Returns 16-bit mono audio at [`SAMPLE_RATE`], without a
    /// trailing pause.
    #[allow(clippy::too_many_arguments)]
    pub fn synthesize_clause(
        &mut self,
        frontend: &dyn Frontend,
        text: &[char],
        punct: Punctuation,
        params: &VoiceParams,
        sentence_initial: bool,
        minor_pause_ms: u32,
        item_pause_ms: u32,
    ) -> Vec<i16> {
        let mut audio = Vec::new();
        for (group, punct, first) in breath_groups(text, punct) {
            if self.cancel.is_cancelled() {
                break;
            }
            let mut utt = frontend.process(group, punct, params.spelling_mode == SpellingMode::LetterSounds);
            utt.sentence_initial = sentence_initial && first;
            // A song has no place for a pause of the text: its rests are the song's.
            if self.is_singing() {
                self.render_utterance(&utt, params, &mut audio, true);
                continue;
            }
            // The short pauses inside the clause: the parts are rendered one
            // after another, so that the sounds before a pause end as they do
            // before any pause, with a silence between them.
            let parts = split_at_breaks(&utt);
            for (k, part) in parts.iter().enumerate() {
                if self.cancel.is_cancelled() {
                    break;
                }
                let before = audio.len();
                self.render_utterance(part, params, &mut audio, false);
                if k + 1 < parts.len() && audio.len() > before {
                    audio.extend(silence(if part.item_break { item_pause_ms } else { minor_pause_ms }));
                }
            }
        }
        audio
    }

    /// The sound of one letter alone, for spelling by sounds. Spoken, not
    /// sung, by every preset: the singing presets leave their song alone.
    /// Empty for an unknown letter.
    pub fn synthesize_letter_sound(
        &mut self,
        frontend: &dyn Frontend,
        letter: &[char],
        params: &VoiceParams,
    ) -> Vec<i16> {
        let mut audio = Vec::new();
        self.render_utterance(&frontend.letter_sound(letter), params, &mut audio, false);
        audio
    }

    /// Renders one utterance and appends it to `audio`.
    pub fn render_utterance(&mut self, utt: &Utterance, params: &VoiceParams, audio: &mut Vec<i16>, sing: bool) {
        if utt.phones.is_empty() {
            return;
        }

        // The user's voice quality settings (head size, breathiness,
        // roughness, brightness) shape the voice and the synthesizer per
        // utterance; at their defaults of 50 both are the speaker's own.
        let voice = self.voice.with_quality(params);
        let melody = sing.then_some(Melody { notes: &self.melody, cursor: &mut self.cursor });
        let frames = ClauseBuilder::new(&voice, params, utt, melody).build();
        if frames.is_empty() {
            return;
        }

        let mut pcm = Vec::new();
        self.synth.set_quality(&voice.quality(params));
        self.synth.reset();
        // In slices, which the synthesizer's state carries across: the same
        // samples as in one call, with a look at the request to stop between.
        for slice in frames.chunks(FRAMES_PER_CANCEL_CHECK) {
            if self.cancel.is_cancelled() {
                return;
            }
            self.synth.render(slice, &mut pcm);
        }

        // Brightness reaches the whole output, the frication and the bursts
        // too (the source emphasis shapes the voicing alone). 50 leaves it out.
        tilt_brightness(&mut pcm, SAMPLE_RATE, params.brightness);

        // The volume boost raises the level before the volume and holds the
        // peaks under its ceiling; the soft knee below is for the plain
        // output only, since bending the limited signal a second time would
        // add distortion of its own.
        let volume = params.volume.clamp(0.0, 1.0);
        let gain = if params.volume_boost {
            pcm.iter_mut().for_each(|sample| *sample *= OUTPUT_GAIN);
            boost_volume(&mut pcm, SAMPLE_RATE, &Boost::default());
            volume
        } else {
            OUTPUT_GAIN * volume
        };
        pcm.iter_mut().for_each(|sample| *sample *= gain);
        // The plain output goes through the boost's limiter at unity gain
        // with its ceiling at the soft knee: breathiness 0 and brightness 100
        // raise the peaks, and the knee bent them into distortion (a
        // waveshaping residual of -17 to -21 dB on a paragraph, November
        // 2026). The gain moves only between syllables; output under the knee
        // is unchanged.
        if !params.volume_boost {
            boost_volume(&mut pcm, SAMPLE_RATE, &Boost::limiter(LIMIT_KNEE));
        }
        audio.extend(pcm.into_iter().map(|sample| {
            let mut x = sample;
            if !params.volume_boost && x.abs() > LIMIT_KNEE {
                let over = (x.abs() - LIMIT_KNEE) / (1.0 - LIMIT_KNEE);
                x = (LIMIT_KNEE + (1.0 - LIMIT_KNEE) * over.tanh()).copysign(x);
            }
            (x.clamp(-1.0, 1.0) * 32000.0).round() as i16
        }));
    }
}
