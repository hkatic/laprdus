//! Singing presets: songs, notes and the style a preset sings in.
//!
//! A preset is one of the speaking voices (its vocal tract) with a
//! [`SingingStyle`]: it sings the text to a public-domain folk song, one
//! syllable per note. The songs and styles themselves belong to the languages
//! (`languages/<code>/formant`).

use super::klatt::SourceKind;
use crate::math::pow2f;

/// One note of a song. `midi` 0 is a rest. A tied note continues the syllable
/// of the note before it (a melisma) instead of taking a new one.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Note {
    pub midi: i8,
    pub tie: bool,
    /// Quarter notes.
    pub beats: f32,
}

/// A song the singing presets know. The notation is a space-separated list of
/// notes: pitch (C4, F#3, Bb2, R for a rest), a slash and the note value (1,
/// 2, 4, 8, 16; a trailing dot lengthens it by half, a `t` makes it a
/// triplet), with a tilde in front of a note that is sung on the syllable of
/// the note before it: "G4/4 ~A4/8 R/8". Bar lines (|) are ignored.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Song {
    pub title: &'static str,
    pub notation: &'static str,
    /// Quarter notes per minute at speed 1.0.
    pub bpm: f32,
}

impl Song {
    /// The notes of the song. Unparseable tokens are skipped.
    pub fn notes(&self) -> Vec<Note> {
        self.notation.split_ascii_whitespace().filter_map(parse_note).collect()
    }
}

fn parse_note(token: &str) -> Option<Note> {
    let (tie, rest) = token.strip_prefix('~').map_or((false, token), |rest| (true, rest));
    let mut chars = rest.chars().peekable();
    let semis: Option<i32> = match chars.next()? {
        'C' => Some(0),
        'D' => Some(2),
        'E' => Some(4),
        'F' => Some(5),
        'G' => Some(7),
        'A' => Some(9),
        'B' => Some(11),
        'R' | 'r' => None,
        _ => return None,
    };
    let midi = match semis {
        None => 0,
        Some(semis) => {
            let accidental = match chars.peek() {
                Some('#') => 1,
                Some('b') => -1,
                _ => 0,
            };
            if accidental != 0 {
                chars.next();
            }
            let octave = chars.next()?.to_digit(10)? as i32;
            (12 * (octave + 1) + semis + accidental) as i8
        }
    };
    if chars.next()? != '/' {
        return None;
    }
    let mut value = 0u32;
    while let Some(digit) = chars.peek().and_then(|c| c.to_digit(10)) {
        value = value.checked_mul(10)?.checked_add(digit)?;
        chars.next();
    }
    if value == 0 {
        return None;
    }
    let mut beats = 4.0 / value as f32;
    let mut dot = beats * 0.5;
    while chars.next_if_eq(&'.').is_some() {
        beats += dot;
        dot *= 0.5;
    }
    if chars.next_if_eq(&'t').is_some() {
        beats *= 2.0 / 3.0;
    }
    chars.next().is_none().then_some(Note { midi, tie, beats })
}

/// Frequency of a MIDI note number (69 = A4 = 440 Hz).
pub fn midi_hz(midi: f32) -> f32 {
    440.0 * pow2f((midi - 69.0) / 12.0)
}

/// How a singing preset sings: which song, what the voice source is, and the
/// envelope of every note.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct SingingStyle {
    pub song: &'static Song,
    pub source: SourceKind,
    /// Detuned copies of the source (0..1).
    pub chorus: f32,
    pub chorus_cents: f32,
    /// Copy an octave down (0..1).
    pub sub_octave: f32,
    /// Wet level (0..1).
    pub reverb: f32,
    /// Source high-frequency emphasis (0..0.9).
    pub brightness: f32,
    pub breathiness: f32,
    pub flutter: f32,
    pub jitter: f32,
    pub shimmer: f32,
    pub open_quotient: f32,
    /// dB at 3 kHz.
    pub tilt: f32,
    pub vibrato_hz: f32,
    /// Peak deviation at inflection level 0.5.
    pub vibrato_cents: f32,
    /// Vibrato grows over this time after a note starts.
    pub vibrato_delay_ms: f32,
    /// Rise of every note.
    pub attack_ms: f32,
    /// Fall before a rest.
    pub release_ms: f32,
    /// 0: sustained; else a plucked string's decay.
    pub decay_ms: f32,
    /// Tremolo: the string is plucked again every so often (0: once).
    pub retrigger_beats: f32,
    /// Time constant of the glide between notes.
    pub portamento_ms: f32,
    /// Semitones added to the written notes.
    pub transpose: f32,
    /// Level relative to the speaking voice.
    pub gain: f32,
    /// Singer's formant: F5 (Hz, 0 keeps the voice's own).
    pub f5: f32,
    /// Its bandwidth (Hz).
    pub b5: f32,
    /// The speaking voice itself, unchanged, singing around its own pitch
    /// (the transposition is then chosen so the song's middle note sits a
    /// little above the voice's speaking pitch).
    pub dry: bool,
}

impl SingingStyle {
    /// The speaking voice singing as itself: nothing in place of the larynx,
    /// no hall, a moderate vibrato, the song brought to the voice's own pitch.
    /// Phonation (open quotient, tilt, breathiness) stays the voice's.
    pub const fn dry(song: &'static Song) -> Self {
        Self {
            song,
            source: SourceKind::Glottal,
            chorus: 0.0,
            chorus_cents: 8.0,
            sub_octave: 0.0,
            reverb: 0.0,
            brightness: 0.72,
            breathiness: 0.0,
            flutter: 0.2,
            jitter: 0.004,
            shimmer: 0.03,
            open_quotient: 0.0,
            tilt: 0.0,
            vibrato_hz: 5.5,
            vibrato_cents: 40.0,
            vibrato_delay_ms: 300.0,
            attack_ms: 40.0,
            release_ms: 60.0,
            decay_ms: 0.0,
            retrigger_beats: 0.0,
            portamento_ms: 60.0,
            transpose: 0.0,
            gain: 0.90,
            f5: 0.0,
            b5: 0.0,
            dry: true,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn notation() {
        let song = Song { title: "", notation: "G4/4 ~A4/8. | R/8 Bb2/16 C#5/4t x Q4/4 C4", bpm: 60.0 };
        assert_eq!(
            song.notes(),
            [
                Note { midi: 67, tie: false, beats: 1.0 },
                Note { midi: 69, tie: true, beats: 0.75 },
                Note { midi: 0, tie: false, beats: 0.5 },
                Note { midi: 46, tie: false, beats: 0.25 },
                Note { midi: 73, tie: false, beats: 2.0 / 3.0 },
            ]
        );
    }
}
