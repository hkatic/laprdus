//! Voice parameters: rate, pitch, volume, pauses, spelling and the four voice
//! quality settings, with their limits and the curves that turn a 0-100
//! setting into what the synthesizers use.

// Limits of the user-controlled speech rate and pitch. The recorded
// (concatenative) voices are rendered by PSOLA from their pitch marks and keep
// the narrower ranges (beyond them the periods thin out or pile up); the
// formant voices apply rate and pitch at the source and accept the wider ones.
// `VoiceParams::clamp()` enforces the wide limits; the concatenative planner
// narrows them again for itself.
pub const CONCAT_SPEED_MIN: f32 = 0.5;
pub const CONCAT_SPEED_MAX: f32 = 4.0; // 4.0x for NVDA rate boost
pub const CONCAT_USER_PITCH_MIN: f32 = 0.5;
pub const CONCAT_USER_PITCH_MAX: f32 = 2.0;
pub const FORMANT_SPEED_MIN: f32 = 0.25;
pub const FORMANT_SPEED_MAX: f32 = 4.0; // before acceleration
pub const FORMANT_USER_PITCH_MIN: f32 = 0.25;
pub const FORMANT_USER_PITCH_MAX: f32 = 4.0;

/// Acceleration multiplies the speech rate of every voice, so the same rate
/// slider reaches a higher (or lower) top speed. 1.0 leaves the rate alone.
pub const ACCELERATION_MIN: f32 = 0.5;
pub const ACCELERATION_MAX: f32 = 3.0;
pub const ACCELERATION_DEFAULT: f32 = 1.0;
/// Fastest rate the formant synthesizer renders (speed × acceleration); the
/// recorded voices stop at [`CONCAT_SPEED_MAX`].
pub const FORMANT_EFFECTIVE_SPEED_MAX: f32 = 8.0;

/// Words per minute of the formant voices at speed 1.0 and acceleration 1.0,
/// measured on running Croatian text with Zvonko (the other voices differ by
/// their tempo). Used to show rate limits in words per minute.
pub const FORMANT_NOMINAL_WPM: f32 = 175.0;

/// Inflection level of every voice: 0.0 is a monotone, 0.5 the measured pitch
/// movements, 1.0 twice those movements.
pub const INFLECTION_LEVEL_DEFAULT: f32 = 0.5;

/// How numbers are read.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum NumberMode {
    /// "123" → "sto dvadeset tri".
    #[default]
    WholeNumbers,
    /// "123" → "jedan dva tri".
    DigitByDigit,
}

/// Pause lengths in milliseconds.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct PauseSettings {
    /// After sentence-ending punctuation (. ! ?).
    pub sentence_pause_ms: u32,
    pub comma_pause_ms: u32,
    pub newline_pause_ms: u32,
    /// Between spelled characters.
    pub spelling_pause_ms: u32,
}

impl Default for PauseSettings {
    fn default() -> Self {
        Self { sentence_pause_ms: 100, comma_pause_ms: 100, newline_pause_ms: 100, spelling_pause_ms: 200 }
    }
}

impl PauseSettings {
    pub const MAX_MS: u32 = 2000;

    pub fn clamp(&mut self) {
        for pause in [
            &mut self.sentence_pause_ms,
            &mut self.comma_pause_ms,
            &mut self.newline_pause_ms,
            &mut self.spelling_pause_ms,
        ] {
            *pause = (*pause).min(Self::MAX_MS);
        }
    }
}

/// How a letter is read when text is spelled (screen reader character
/// navigation, typing echo, SAPI5 spell-out).
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum SpellingMode {
    /// "b" → "be", "h" → "ha", "j" → "je".
    LetterNames,
    /// "b" → the sound [b], "f" → [f], "h" → [x]; q, w, x, y by name.
    #[default]
    LetterSounds,
}

// Spelling speed controls the silence after each character, never its speech
// rate. 100 adds no extra silence; 50 (the default) adds about 0.41 times the
// character's duration; 20 about 0.74 times; 0 adds its whole duration, plus
// the spelling pause.
pub const SPELLING_SPEED_MIN: i32 = 0;
pub const SPELLING_SPEED_MAX: i32 = 100;
pub const SPELLING_SPEED_DEFAULT: i32 = 50;

/// Spelling cadence factor, used for pauses only, never to scale speech:
/// 2^((s − 100) / 100), 0.5 at 0, 0.57 at 20, 0.71 at 50, 1.0 at 100. The
/// silence added is duration × (1/factor − 1).
pub fn spelling_rate_factor(spelling_speed: i32) -> f32 {
    let s = spelling_speed.clamp(SPELLING_SPEED_MIN, SPELLING_SPEED_MAX);
    crate::math::pow2f((s as f32 - 100.0) / 100.0)
}

// Volume boost (`VoiceParams::volume_boost`): louder speech for a phone's
// earpiece in traffic. `audio::volume_boost` drives the clause with
// VOLUME_BOOST_GAIN (+14 dB) and holds the peaks under VOLUME_BOOST_CEILING
// with a lookahead peak limiter whose gain moves only between syllables, so
// nothing is waveshaped. The soft clipper above VOLUME_BOOST_KNEE is a safety
// net that the ceiling keeps idle.
pub const VOLUME_BOOST_GAIN: f32 = 5.0;
/// On the true peak (the limiter also looks between the samples): the phone
/// resamples 22.05 to 48 kHz, and 0.944 keeps every voice under full scale
/// between the samples.
pub const VOLUME_BOOST_CEILING: f32 = 0.944;
pub const VOLUME_BOOST_KNEE: f32 = 0.9;
/// In boost mode the gain may rise at most this far above the gain of the
/// loudest syllable of the last 0.6 s, so the consonants keep their distance
/// from the vowels (6 for a day in November 2026: the bursts still came 4 dB
/// closer to the vowels).
pub const VOLUME_BOOST_MAX_LIFT_DB: f32 = 2.0;
pub const VOLUME_BOOST_TREBLE_DB: f32 = -3.0;
pub const VOLUME_BOOST_TREBLE_HZ: f32 = 4000.0;
// The hiss limiter of boost mode: while the band above VOLUME_BOOST_HISS_HZ
// dominates the sound (a sibilant, not a vowel), the part above
// VOLUME_BOOST_HISS_SPLIT_HZ is held VOLUME_BOOST_HISS_DB under the loudest
// nearby vowel, cut by at most VOLUME_BOOST_HISS_MAX_CUT_DB.
pub const VOLUME_BOOST_HISS_DB: f32 = -22.0;
pub const VOLUME_BOOST_HISS_HZ: f32 = 3000.0;
pub const VOLUME_BOOST_HISS_SPLIT_HZ: f32 = 2000.0;
pub const VOLUME_BOOST_HISS_MAX_CUT_DB: f32 = 24.0;
/// The same limiter on the plain output of the recorded voices, with a mild
/// hold: in the recordings s, c, č, ć and the release of t stand level with
/// the vowels ("The hiss where the ear is sharpest" in docs/formant.md).
/// 0 turns it off.
pub const RECORDED_HISS_DB: f32 = -10.0;

// Voice quality (head size, breathiness, roughness, brightness): four
// settings of 0-100 after the voice-parameter model of a classic formant
// synthesizer. 50 is the voice as its tables define it, so the defaults change
// nothing. See "Voice quality settings" in docs/formant.md.
pub const VOICE_QUALITY_MIN: i32 = 0;
pub const VOICE_QUALITY_MAX: i32 = 100;
pub const VOICE_QUALITY_DEFAULT: i32 = 50;
/// Formant factor at 100 (and its inverse at 0): a giant at 0 and a child at
/// 100; 1.25 until a listener found the steps hard to hear (November 2026).
pub const HEAD_SIZE_SPAN: f32 = 1.4;
/// Source emphasis at brightness 100.
pub const BRIGHTNESS_MAX_EMPHASIS: f32 = 0.85;
/// Source emphasis a fully tense voice (breathiness 0) reaches: a pressed
/// voice has stronger upper harmonics.
pub const BREATHINESS_TENSE_EMPHASIS: f32 = 0.88;
/// Share of the low shelf on the voicing a fully tense voice gives up: a
/// pressed voice has a weak fundamental.
pub const BREATHINESS_TENSE_BASS_CUT: f32 = 0.8;
// Brightness tilt: the top above BRIGHTNESS_SHARP_HZ is multiplied by up to
// BRIGHTNESS_SHARP_GAIN at 100; below 50 the pivot of the fold moves down to
// BRIGHTNESS_DULL_HZ at 0.
pub const BRIGHTNESS_SHARP_HZ: f32 = 1500.0;
pub const BRIGHTNESS_SHARP_GAIN: f32 = 3.0;
pub const BRIGHTNESS_DULL_HZ: f32 = 800.0;

fn quality(value: i32) -> i32 {
    value.clamp(VOICE_QUALITY_MIN, VOICE_QUALITY_MAX)
}

/// Factor every formant frequency is multiplied by for a head size setting:
/// 0.71 at 0 (a tract 40 % longer, a deeper voice), 1 at 50, 1.4 at 100 (30 %
/// shorter, a higher voice). The maintainer asked for the higher value to be
/// the higher voice; synthesizer A's head size runs the other way.
pub fn head_size_factor(head_size: i32) -> f32 {
    HEAD_SIZE_SPAN.powf((quality(head_size) as f32 - 50.0) / 50.0)
}

/// Diplophonia share for a roughness setting: 0 up to 50, 1 at 100, and the
/// square root of the distance from 50 in between, so that the rasp is heard
/// from the first steps above 50.
pub fn roughness_diplophonia(roughness: i32) -> f32 {
    let v = quality(roughness);
    if v <= 50 { 0.0 } else { ((v - 50) as f32 / 50.0).sqrt() }
}

/// Whisper share for a breathiness setting: 0 up to 50, 1 at 100.
pub fn breathiness_whisper(breathiness: i32) -> f32 {
    let v = quality(breathiness);
    if v <= 50 { 0.0 } else { (v - 50) as f32 / 50.0 }
}

/// Tension share for a breathiness setting: 0 from 50 up, 1 at 0: a pressed,
/// clear voice.
pub fn breathiness_tension(breathiness: i32) -> f32 {
    let v = quality(breathiness);
    if v >= 50 { 0.0 } else { (50 - v) as f32 / 50.0 }
}

/// Factor the voice's own breath noise is multiplied by for a breathiness
/// setting: 0 at 0, 1 from 50 up (above 50 the whisper adds the breath).
pub fn breathiness_noise_scale(breathiness: i32) -> f32 {
    let v = quality(breathiness);
    if v >= 50 { 1.0 } else { v as f32 / 50.0 }
}

/// Factor a voice's own jitter or shimmer is multiplied by: 0 at 0, 1 at 50,
/// 5 at 100.
pub fn voice_quality_scale(value: i32) -> f32 {
    let v = quality(value);
    if v <= 50 { v as f32 / 50.0 } else { 1.0 + (v - 50) as f32 / 50.0 * 4.0 }
}

/// Source emphasis for a brightness setting around a voice's own `base`.
pub fn brightness_emphasis(brightness: i32, base: f32) -> f32 {
    let v = quality(brightness);
    if v <= 50 { base * v as f32 / 50.0 } else { base + (BRIGHTNESS_MAX_EMPHASIS - base) * (v - 50) as f32 / 50.0 }
}

/// Everything a host can set about how a voice speaks. Both kinds of voices
/// apply rate, pitch and volume at the source.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct VoiceParams {
    /// Speech rate (0.5 - 4.0; formant voices 0.25 - 4.0).
    pub speed: f32,
    /// Voice character pitch (0.25 - 4.0).
    pub pitch: f32,
    /// User pitch preference (0.5 - 2.0; formant voices 0.25 - 4.0).
    pub user_pitch: f32,
    /// Volume (0.0 - 1.0).
    pub volume: f32,
    /// Louder output: gain and a peak limiter that moves only between syllables.
    pub volume_boost: bool,
    /// Size of the pitch movements (0.0 - 1.0).
    pub inflection_level: f32,
    /// Rate multiplier (0.5 - 3.0).
    pub acceleration: f32,
    /// Emoji to text conversion (off by default).
    pub emoji_enabled: bool,
    pub number_mode: NumberMode,
    pub pause_settings: PauseSettings,
    pub spelling_mode: SpellingMode,
    /// Spelling cadence (0-100); lower adds more silence.
    pub spelling_speed: i32,
    /// Vocal tract length, 0-100.
    pub head_size: i32,
    /// Breath, 0-100: 0 a tense voice, 100 a whisper.
    pub breathiness: i32,
    /// Jitter and shimmer, 0-100.
    pub roughness: i32,
    /// Tone, 0-100.
    pub brightness: i32,
}

impl Default for VoiceParams {
    fn default() -> Self {
        Self {
            speed: 1.0,
            pitch: 1.0,
            user_pitch: 1.0,
            volume: 1.0,
            volume_boost: false,
            inflection_level: INFLECTION_LEVEL_DEFAULT,
            acceleration: ACCELERATION_DEFAULT,
            emoji_enabled: false,
            number_mode: NumberMode::default(),
            pause_settings: PauseSettings::default(),
            spelling_mode: SpellingMode::default(),
            spelling_speed: SPELLING_SPEED_DEFAULT,
            head_size: VOICE_QUALITY_DEFAULT,
            breathiness: VOICE_QUALITY_DEFAULT,
            roughness: VOICE_QUALITY_DEFAULT,
            brightness: VOICE_QUALITY_DEFAULT,
        }
    }
}

impl VoiceParams {
    /// Brings every value into its range. A NaN or infinite rate or pitch
    /// would reach the durations and filters of the formant voices, so those
    /// go back to their defaults.
    pub fn clamp(&mut self) {
        let finite = |value: f32, default: f32, min: f32, max: f32| {
            if value.is_finite() { value.clamp(min, max) } else { default }
        };
        self.speed = finite(self.speed, 1.0, FORMANT_SPEED_MIN, FORMANT_SPEED_MAX);
        self.pitch = finite(self.pitch, 1.0, 0.25, 4.0);
        self.user_pitch = finite(self.user_pitch, 1.0, FORMANT_USER_PITCH_MIN, FORMANT_USER_PITCH_MAX);
        self.volume = finite(self.volume, 1.0, 0.0, 1.0);
        self.inflection_level = finite(self.inflection_level, INFLECTION_LEVEL_DEFAULT, 0.0, 1.0);
        self.acceleration = finite(self.acceleration, ACCELERATION_DEFAULT, ACCELERATION_MIN, ACCELERATION_MAX);
        self.spelling_speed = self.spelling_speed.clamp(SPELLING_SPEED_MIN, SPELLING_SPEED_MAX);
        for value in [&mut self.head_size, &mut self.breathiness, &mut self.roughness, &mut self.brightness] {
            *value = quality(*value);
        }
        self.pause_settings.clamp();
    }

    /// Speech rate the voices actually render: speed times acceleration. The
    /// recorded voices narrow it to their own range ([`CONCAT_SPEED_MAX`]).
    pub fn effective_speed(&self) -> f32 {
        let s = self.speed * self.acceleration;
        if s.is_finite() { s.clamp(FORMANT_SPEED_MIN, FORMANT_EFFECTIVE_SPEED_MAX) } else { 1.0 }
    }
}
