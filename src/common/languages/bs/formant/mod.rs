//! Bosnian formant voices: Mirsad and his singing presets.

use crate::formant::klatt::SourceKind;
use crate::formant::singing::{SingingStyle, Song};
use crate::formant::voice::FormantVoice;

/// Mirsad: higher and softer, more melodic, slower.
pub static MIRSAD: FormantVoice = FormantVoice {
    id: "mirsad",
    base_f0: 124.0,
    formant_scale: 1.02,
    open_quotient: 0.64,
    tilt: 0.5,
    breathiness: 0.030,
    pitch_range: 1.15,
    tempo: 0.96,
    postaccent_length: 1.30,
    h_strength: 1.20,
    hard_palatal_shift: 0.96,
    soft_palatal_shift: 1.00,
    hard_affricate_shift: 1.00,
    singing: None,
};

/// "Kad ja pođoh na Bembašu" (traditional sevdalinka, tune printed by Ludvík
/// Kuba in 1906; Sarajevo's unofficial anthem). D minor, slow; lines of 8, 7,
/// 8 and 7 syllables ("bijelo" has two), the tune of the first line repeated
/// for the third. Durations are those of a measured performance and are only
/// approximate: the song is sung rubato. The breaths between the lines are
/// left to the punctuation.
pub static SONG_BEMBASA: Song = Song {
    title: "Kad ja pođoh na Bembašu",
    notation: "A4/4 A4/4 Bb4/8 A4/8 G4/4 F4/8 E4/8 D4/2 \
               G4/4 F4/8 F4/8 E4/4 E4/8 D4/8 D4/2 \
               A4/4 A4/4 Bb4/8 A4/8 G4/4 F4/8 E4/8 D4/2 \
               F4/4 E4/8 E4/8 D4/4 C#4/8 D4/8 D4/2",
    bpm: 66.0,
};

/// A sevdah singer.
static STYLE_SEVDAH: SingingStyle = SingingStyle {
    song: &SONG_BEMBASA,
    source: SourceKind::Glottal,
    chorus: 0.0,
    chorus_cents: 8.0,
    sub_octave: 0.0,
    reverb: 0.15,
    brightness: 0.72,
    breathiness: 0.04,
    flutter: 0.2,
    jitter: 0.004,
    shimmer: 0.03,
    open_quotient: 0.66,
    tilt: 1.5,
    vibrato_hz: 5.5,
    vibrato_cents: 60.0,
    vibrato_delay_ms: 350.0,
    attack_ms: 50.0,
    release_ms: 90.0,
    decay_ms: 0.0,
    retrigger_beats: 0.0,
    portamento_ms: 90.0,
    transpose: -12.0,
    gain: 0.80,
    f5: 3100.0,
    b5: 160.0,
    dry: false,
};

/// A saz.
static STYLE_SAZ: SingingStyle = SingingStyle {
    song: &SONG_BEMBASA,
    source: SourceKind::Strings,
    chorus: 0.5,
    chorus_cents: 5.0,
    sub_octave: 0.3,
    reverb: 0.10,
    brightness: 0.40,
    breathiness: 0.0,
    flutter: 0.0,
    jitter: 0.0,
    shimmer: 0.0,
    open_quotient: 0.60,
    tilt: 0.0,
    vibrato_hz: 0.0,
    vibrato_cents: 0.0,
    vibrato_delay_ms: 0.0,
    attack_ms: 3.0,
    release_ms: 20.0,
    decay_ms: 350.0,
    retrigger_beats: 0.5,
    portamento_ms: 5.0,
    transpose: -5.0,
    gain: 1.10,
    f5: 0.0,
    b5: 0.0,
    dry: false,
};

static STYLE_SOLIST: SingingStyle = SingingStyle::dry(&SONG_BEMBASA);

pub static SEVDALIJA: FormantVoice = MIRSAD.preset("sevdalija", &STYLE_SEVDAH);
pub static SAZLIJA: FormantVoice = MIRSAD.preset("sazlija", &STYLE_SAZ);
pub static SARAJEVO_BEMBASA: FormantVoice = MIRSAD.preset("sarajevo_bembasa", &STYLE_SOLIST);

/// Every Bosnian formant voice.
pub static VOICES: [&FormantVoice; 4] = [&MIRSAD, &SEVDALIJA, &SAZLIJA, &SARAJEVO_BEMBASA];
