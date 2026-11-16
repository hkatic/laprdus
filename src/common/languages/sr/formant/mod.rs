//! Serbian formant voices: Stojan and his singing presets.

use crate::formant::klatt::SourceKind;
use crate::formant::singing::{SingingStyle, Song};
use crate::formant::voice::FormantVoice;

/// Stojan: lower and darker, slightly brisker.
pub static STOJAN: FormantVoice = FormantVoice {
    id: "stojan",
    base_f0: 98.0,
    formant_scale: 0.965,
    open_quotient: 0.56,
    tilt: 1.5,
    breathiness: 0.015,
    pitch_range: 0.95,
    tempo: 1.04,
    postaccent_length: 1.20,
    h_strength: 0.90,
    hard_palatal_shift: 0.94,
    soft_palatal_shift: 1.00,
    hard_affricate_shift: 1.00,
    singing: None,
};

/// "Kreće se lađa francuska" (Branislav Milosavljević, d. 1944; the song of
/// the Salonika front). A minor, 3/4, eight syllables a line, every line in
/// the same rhythm. The first line is taken from a printed score; the other
/// three follow the song's harmony (F-C-G7-C, C-G7-C-Dm, Am-E7-Am) and want
/// checking against a recording.
pub static SONG_LADJA: Song = Song {
    title: "Kreće se lađa francuska",
    notation: "E4/4 E4/4 E4/4 C5/2 C5/4 B4/2 B4/4 A4/2. \
               A4/4 A4/4 A4/4 E5/2 E5/4 D5/2 D5/4 C5/2. \
               C5/4 C5/4 C5/4 D5/2 D5/4 E5/2 E5/4 D5/2. \
               C5/4 C5/4 C5/4 B4/2 B4/4 G#4/2 G#4/4 A4/2.",
    bpm: 100.0,
};

/// A trumpet.
static STYLE_TRUBA: SingingStyle = SingingStyle {
    song: &SONG_LADJA,
    source: SourceKind::Brass,
    chorus: 0.3,
    chorus_cents: 6.0,
    sub_octave: 0.0,
    reverb: 0.10,
    brightness: 0.50,
    breathiness: 0.0,
    flutter: 0.1,
    jitter: 0.001,
    shimmer: 0.01,
    open_quotient: 0.60,
    tilt: 0.0,
    vibrato_hz: 6.0,
    vibrato_cents: 25.0,
    vibrato_delay_ms: 400.0,
    attack_ms: 45.0,
    release_ms: 40.0,
    decay_ms: 0.0,
    retrigger_beats: 0.0,
    portamento_ms: 25.0,
    transpose: -5.0,
    gain: 0.60,
    f5: 0.0,
    b5: 0.0,
    dry: false,
};

/// An accordion.
static STYLE_HARMONIKA: SingingStyle = SingingStyle {
    song: &SONG_LADJA,
    source: SourceKind::Reed,
    chorus: 0.9,
    chorus_cents: 14.0,
    sub_octave: 0.0,
    reverb: 0.05,
    brightness: 0.30,
    breathiness: 0.0,
    flutter: 0.0,
    jitter: 0.0,
    shimmer: 0.0,
    open_quotient: 0.60,
    tilt: 0.0,
    vibrato_hz: 0.0,
    vibrato_cents: 0.0,
    vibrato_delay_ms: 0.0,
    attack_ms: 15.0,
    release_ms: 25.0,
    decay_ms: 0.0,
    retrigger_beats: 0.0,
    portamento_ms: 6.0,
    transpose: -7.0,
    gain: 0.70,
    f5: 0.0,
    b5: 0.0,
    dry: false,
};

static STYLE_PEVAC: SingingStyle = SingingStyle::dry(&SONG_LADJA);

pub static TRUBAC: FormantVoice = STOJAN.preset("trubac", &STYLE_TRUBA);
pub static HARMONIKAS: FormantVoice = STOJAN.preset("harmonikas", &STYLE_HARMONIKA);
pub static SOLUNAC: FormantVoice = STOJAN.preset("solunac", &STYLE_PEVAC);

/// Every Serbian formant voice.
pub static VOICES: [&FormantVoice; 4] = [&STOJAN, &TRUBAC, &HARMONIKAS, &SOLUNAC];

/// The voices as hosts list them.
pub mod voices {
    use crate::language::Language;
    use crate::voice::Voice;

    pub static STOJAN: Voice = Voice::formant(&super::STOJAN, "Stojan", Language::Serbian);
    pub static TRUBAC: Voice = STOJAN.preset(&super::TRUBAC, "Trubač - Kreće se lađa francuska");
    pub static HARMONIKAS: Voice = STOJAN.preset(&super::HARMONIKAS, "Harmonikaš - Kreće se lađa francuska");
    pub static SOLUNAC: Voice = STOJAN.preset(&super::SOLUNAC, "Solunac");
}
