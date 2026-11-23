//! The speaker of a formant voice, and what the user's voice quality settings
//! make of it.

use super::klatt::VoiceQuality;
use super::singing::SingingStyle;
use crate::params::{
    BREATHINESS_TENSE_BASS_CUT, BREATHINESS_TENSE_EMPHASIS, VOICE_QUALITY_DEFAULT, VoiceParams,
    breathiness_noise_scale, breathiness_tension, breathiness_whisper, brightness_emphasis, head_size_factor,
    roughness_diplophonia, voice_quality_scale,
};

/// F5's bandwidth for the speaking voices: 500 Hz left the vowels' 3-4 kHz
/// 3-5 dB above synthesizer A's (November 2026); the singing presets keep 500.
const SPEAKING_F5_BANDWIDTH: f32 = 650.0;

/// Speaker definition of a formant voice. No data files are involved: the
/// voice is fully described by these parameters and the phone tables.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct FormantVoice {
    pub id: &'static str,
    /// Mid-range speaking pitch (Hz).
    pub base_f0: f32,
    /// Vocal tract length (1.0 = the reference speaker).
    pub formant_scale: f32,
    /// Glottal open quotient (higher = softer).
    pub open_quotient: f32,
    /// Source spectral tilt (dB at 3 kHz).
    pub tilt: f32,
    pub breathiness: f32,
    /// Size of accent and boundary movements.
    pub pitch_range: f32,
    /// The speaker's own rate relative to the reference.
    pub tempo: f32,
    /// Duration factor of unstressed long vowels.
    pub postaccent_length: f32,
    /// Friction of /h/.
    pub h_strength: f32,
    /// Noise frequency factor of č, dž, š, ž.
    pub hard_palatal_shift: f32,
    /// Noise frequency factor of ć, đ.
    pub soft_palatal_shift: f32,
    /// Further factor of č, dž alone: how hard they are.
    pub hard_affricate_shift: f32,
    /// Set on the singing presets only.
    pub singing: Option<&'static SingingStyle>,
}

impl FormantVoice {
    /// A singing preset of this voice: its vocal tract with an instrument in
    /// place of the larynx, or (a dry style) the voice itself singing.
    pub const fn preset(self, id: &'static str, style: &'static SingingStyle) -> Self {
        let mut voice = Self { id, singing: Some(style), ..self };
        if !style.dry {
            voice.open_quotient = style.open_quotient;
            voice.tilt = style.tilt;
            voice.breathiness = style.breathiness;
        }
        voice
    }

    /// The voice as the user's voice quality settings shape it: head size
    /// scales the vocal tract (every formant target goes through
    /// `formant_scale`), and breathiness below 50 takes the voice's own breath
    /// noise away (above 50 the whisper of [`quality`](Self::quality) adds
    /// breath). The speaker's own values are the setting 50.
    pub fn with_quality(&self, params: &VoiceParams) -> Self {
        let mut voice = *self;
        voice.formant_scale *= head_size_factor(params.head_size);
        voice.breathiness *= breathiness_noise_scale(params.breathiness);
        // Roughness below 50 also takes the breath noise away, the last
        // aperiodic part of the voicing: at 0 every glottal pulse is alike.
        if params.roughness < VOICE_QUALITY_DEFAULT {
            voice.breathiness *= voice_quality_scale(params.roughness);
        }
        voice
    }

    /// The synthesizer's source and fixed resonators for the voice, with the
    /// roughness (jitter and shimmer) and brightness (source emphasis)
    /// settings applied around the voice's or the singing style's own values.
    pub fn quality(&self, params: &VoiceParams) -> VoiceQuality {
        let mut quality = VoiceQuality::default();
        for f in &mut quality.upper_f {
            *f *= self.formant_scale;
        }
        quality.breathiness = self.breathiness;
        match self.singing {
            None => quality.upper_b[0] = SPEAKING_F5_BANDWIDTH,
            Some(style) => {
                quality.source = style.source;
                quality.chorus = style.chorus;
                quality.chorus_cents = style.chorus_cents;
                quality.sub_octave = style.sub_octave;
                quality.reverb = style.reverb;
                quality.brightness = style.brightness;
                quality.flutter = style.flutter;
                quality.jitter = style.jitter;
                quality.shimmer = style.shimmer;
                // The presets keep their source and their top: no low shelf,
                // and the voice low-pass instead of the speaking ceiling.
                quality.bass_db = 0.0;
                quality.ceiling_hz = 0.0;
                if style.f5 > 0.0 {
                    // The singer's formant: F5 pulled down towards F4 and
                    // narrowed (synthesizer C's f5/b5 trick), a ring the
                    // speaking voice has not.
                    quality.upper_f[0] = style.f5 * self.formant_scale;
                    quality.upper_b[0] = style.b5;
                }
            }
        }
        // Roughness: the voice's own jitter and shimmer from none to eight
        // times, and above 50 diplophonia on top.
        let diplophonia = roughness_diplophonia(params.roughness);
        let rough = voice_quality_scale(params.roughness) * (1.0 + 0.6 * diplophonia);
        quality.jitter *= rough;
        quality.shimmer *= rough;
        quality.roughness = diplophonia;
        // Below 50 the slow pitch drift goes too, so 0 is a perfectly steady
        // voice (the jitter and shimmer alone were too small to hear go).
        if params.roughness < VOICE_QUALITY_DEFAULT {
            quality.flutter *= voice_quality_scale(params.roughness);
        }
        // Breathiness: a whisper above 50, a tense voice below.
        quality.whisper = breathiness_whisper(params.breathiness);
        quality.tension = breathiness_tension(params.breathiness);
        quality.brightness = brightness_emphasis(params.brightness, quality.brightness);
        if quality.tension > 0.0 {
            // A pressed voice has stronger upper harmonics and a weak fundamental.
            if quality.brightness < BREATHINESS_TENSE_EMPHASIS {
                quality.brightness += quality.tension * (BREATHINESS_TENSE_EMPHASIS - quality.brightness);
            }
            quality.bass_db *= 1.0 - BREATHINESS_TENSE_BASS_CUT * quality.tension;
        }
        quality
    }
}
