//! Croatian formant voices: Zvonko and his singing presets.

use crate::formant::klatt::SourceKind;
use crate::formant::singing::{SingingStyle, Song};
use crate::formant::voice::FormantVoice;

/// Zvonko: mid-pitched, clear.
pub static ZVONKO: FormantVoice = FormantVoice {
    id: "zvonko",
    base_f0: 112.0,
    formant_scale: 1.00,
    open_quotient: 0.60,
    tilt: 0.0,
    breathiness: 0.020,
    pitch_range: 1.00,
    tempo: 1.00,
    postaccent_length: 1.10,
    h_strength: 0.90,
    hard_palatal_shift: 1.03,
    soft_palatal_shift: 0.97,
    hard_affricate_shift: 0.93,
    singing: None,
};

/// "Vila Velebita" ("Oj ti vilo, vilo Velebita"; words and tune anonymous,
/// first sung 1882, printed 1893). D major, 2/4, brisk, and without a rest
/// from the first note to the last: one verse and the refrain, lines of 10,
/// 7, 10, 6 (+ a melisma), 7, 7, 12, 7 and 7 syllables. Taken from a
/// public-domain two-voice MIDI transcription.
pub static SONG_VILA: Song = Song {
    title: "Vila Velebita",
    notation: "D4/8. E4/16 F#4/4 F#4/4 F#4/8 F#4/8 E4/8 F#4/8 A4/4 G4/4 \
               G4/8 G4/8. F#4/16 E4/4 A4/4 A4/4 F#4/4. \
               D4/8. E4/16 F#4/4 F#4/4 F#4/8 F#4/8 E4/8 F#4/8 A4/4 G4/4. \
               G4/8. F#4/16 E4/4 A4/4 A4/8 ~G4/8 F#4/4 \
               F#4/8 G4/8 A4/8 B4/4. B4/8 B4/4 A4/4 \
               F#4/8 G4/8 A4/8 B4/4. B4/8 B4/4 A4/8. \
               F#4/8. G4/16 A4/4 F#4/8. G4/16 A4/4 F#4/8. G4/16 A4/4 F#4/8. G4/16 A4/4 \
               B4/8. B4/16 B4/4 D5/4 C#5/8. B4/16 A4/4 \
               A4/8 B4/8 A4/8 G4/4 E4/4 A4/4 F#4/4.",
    bpm: 113.0,
};

/// The bećarac, the traditional two-line (ten syllables a line) humorous song
/// of Slavonia, Baranja and Srijem, on UNESCO's list since 2011. The tune is
/// the traditional one as transcribed for the couplet "U mog strica osam
/// kobasica, sedam prži, osmu strina drži" (2/4, quarter = 75, D minor with
/// its raised seventh): the first line to bars 1-3, the second to bars 4-6,
/// with the melismas on "mog", "sam", "ba", "ca", "dam", "smu", "stri", "dr"
/// as sung. Any other couplet typed in lands on the same two lines.
pub static SONG_BECARAC: Song = Song {
    title: "Bećarac",
    notation: "F4/8 F4/16 ~E4/16 F4/8 C#4/8 \
               F4/8 F4/16 ~G4/16 F4/16 E4/16 ~C#4/16 B3/16 \
               C#4/8. ~B3/16 ~G3/8 \
               D4/8 D4/16 ~B3/16 C#4/8 C#4/8 \
               B3/8 B3/16 ~C#4/16 E4/16 ~C#4/16 B3/8 \
               D4/8. ~C#4/16 B3/8",
    bpm: 75.0,
};

// Singing presets: the speaking voice with an instrument in place of the
// larynx. Vibrato depth is the value at inflection level 0.5; the inflection
// slider scales it and 0 switches it off.

/// A pipe organ.
static STYLE_ORGULJE: SingingStyle = SingingStyle {
    song: &SONG_VILA,
    source: SourceKind::Organ,
    chorus: 0.0,
    chorus_cents: 8.0,
    sub_octave: 0.5,
    reverb: 0.35,
    brightness: 0.15,
    breathiness: 0.0,
    flutter: 0.0,
    jitter: 0.0,
    shimmer: 0.0,
    open_quotient: 0.60,
    tilt: 0.0,
    vibrato_hz: 0.0,
    vibrato_cents: 0.0,
    vibrato_delay_ms: 0.0,
    attack_ms: 25.0,
    release_ms: 30.0,
    decay_ms: 0.0,
    retrigger_beats: 0.0,
    portamento_ms: 8.0,
    transpose: -12.0,
    gain: 0.55,
    f5: 0.0,
    b5: 0.0,
    dry: false,
};

/// A klapa choir.
static STYLE_KLAPA: SingingStyle = SingingStyle {
    song: &SONG_VILA,
    source: SourceKind::Glottal,
    chorus: 0.7,
    chorus_cents: 10.0,
    sub_octave: 0.6,
    reverb: 0.20,
    brightness: 0.60,
    breathiness: 0.03,
    flutter: 0.3,
    jitter: 0.004,
    shimmer: 0.03,
    open_quotient: 0.62,
    tilt: 1.0,
    vibrato_hz: 5.5,
    vibrato_cents: 30.0,
    vibrato_delay_ms: 350.0,
    attack_ms: 60.0,
    release_ms: 80.0,
    decay_ms: 0.0,
    retrigger_beats: 0.0,
    portamento_ms: 40.0,
    transpose: -12.0,
    gain: 0.55,
    f5: 0.0,
    b5: 0.0,
    dry: false,
};

static STYLE_PJEVAC: SingingStyle = SingingStyle::dry(&SONG_VILA);

/// The bećarac is belted out straight, with hardly any vibrato and crisp
/// steps between the notes.
static STYLE_BECARAC: SingingStyle = SingingStyle {
    vibrato_hz: 6.0,
    vibrato_cents: 20.0,
    vibrato_delay_ms: 250.0,
    attack_ms: 30.0,
    release_ms: 40.0,
    portamento_ms: 30.0,
    gain: 0.95,
    ..SingingStyle::dry(&SONG_BECARAC)
};

pub static ORGULJAS: FormantVoice = ZVONKO.preset("orguljas", &STYLE_ORGULJE);
pub static DALMACIJA_KLAPA: FormantVoice = ZVONKO.preset("dalmacija_klapa", &STYLE_KLAPA);
pub static LIKA_VELEBIT: FormantVoice = ZVONKO.preset("lika_velebit", &STYLE_PJEVAC);
pub static BECARAC: FormantVoice = ZVONKO.preset("becarac", &STYLE_BECARAC);

/// Every Croatian formant voice.
pub static VOICES: [&FormantVoice; 5] = [&ZVONKO, &ORGULJAS, &DALMACIJA_KLAPA, &LIKA_VELEBIT, &BECARAC];

/// The voices as hosts list them. An instrument preset is named after the
/// player and the song, a voice singing as itself after the region of its
/// song.
pub mod voices {
    use crate::language::Language;
    use crate::voice::Voice;

    /// The default voice.
    pub static ZVONKO: Voice = Voice::formant(&super::ZVONKO, "Zvonko", Language::Croatian);
    pub static ORGULJAS: Voice = ZVONKO.preset(&super::ORGULJAS, "Orguljaš - Vila Velebita");
    pub static DALMACIJA_KLAPA: Voice = ZVONKO.preset(&super::DALMACIJA_KLAPA, "Dalmacija Klapa");
    pub static LIKA_VELEBIT: Voice = ZVONKO.preset(&super::LIKA_VELEBIT, "Lika Velebit");
    pub static BECARAC: Voice = ZVONKO.preset(&super::BECARAC, "Bećarac");
}
