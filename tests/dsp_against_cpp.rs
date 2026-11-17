//! Both synthesizers against the C++ engine, phone for phone.
//!
//! A dump program built from the C++ engine (removed with it) wrote, for every text of the golden corpus, the
//! phones the C++ front end made of it and a hash of the audio the C++
//! synthesizers rendered for them. These tests render the same phones and
//! compare. The dumps are local files (`build/golden/*_full.tsv`); the tests
//! pass without them.

use laprdus::concatenative::synthesizer::ConcatSynthesizer;
use laprdus::concatenative::voice_data::VoiceData;
use laprdus::formant::phones::{Accent, Ph, Phone};
use laprdus::formant::synthesizer::FormantSynthesizer;
use laprdus::languages::hr::formant::ZVONKO;
use laprdus::params::VoiceParams;
use laprdus::prosody::{ClauseKind, Utterance, split_at_breaks};
use laprdus::silence;
use std::collections::HashMap;
use std::path::{Path, PathBuf};

const PHONES: [Ph; Ph::COUNT] = {
    use Ph::*;
    [
        Sil, A, E, I, O, U, Schwa, Eh, Oh, Ee, Oo, Ax, W, J, V, L, Lj, M, N, Nj, Ng, R, P, B, T, D, K, G, C, Dz, Ch,
        Dzh, Tj, Dj, F, S, Z, Sh, Zh, Sj, Zj, H, Gh,
    ]
};

fn golden() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("build/golden")
}

fn fnv(samples: &[i16]) -> String {
    let hash = samples
        .iter()
        .flat_map(|sample| sample.to_le_bytes())
        .fold(1469598103934665603u64, |h, byte| (h ^ u64::from(byte)).wrapping_mul(1099511628211));
    format!("{hash:016x}")
}

fn utterance(flags: &str, phones: &str) -> Utterance {
    let flags: Vec<i32> = flags.split(' ').map(|n| n.parse().unwrap()).collect();
    let phones = phones
        .split_ascii_whitespace()
        .map(|phone| {
            let n: Vec<i32> = phone.split(':').map(|n| n.parse().unwrap()).collect();
            let bit = |i: u32| n[1] >> i & 1 == 1;
            Phone {
                ph: PHONES[n[0] as usize],
                nucleus: bit(0),
                nucleus_tail: bit(1),
                stressed: bit(2),
                is_long: bit(3),
                short_glide: bit(4),
                word_start: bit(5),
                vowel_word: bit(6),
                break_before: bit(7),
                item_break: bit(8),
                accent: [Accent::None, Accent::Neutral, Accent::Falling, Accent::Rising][n[2] as usize],
                prominence: n[3] as u8,
                word: n[4] as u16,
                syllable: n[5] as i16,
                word_syllables: n[6] as u8,
            }
        })
        .collect();
    Utterance {
        phones,
        kind: [
            ClauseKind::Statement,
            ClauseKind::Continuation,
            ClauseKind::YesNoQuestion,
            ClauseKind::WhQuestion,
            ClauseKind::Exclamation,
        ][flags[0] as usize],
        syllable_count: flags[1],
        focus_word: flags[2],
        isolated_sound: flags[3] == 1,
        slovenian: flags[4] == 1,
        ..Utterance::default()
    }
}

/// The parts of a clause between its short pauses, each rendered by `render`,
/// with the pauses of the synthesizers' defaults between them.
fn clause(utt: &Utterance, mut render: impl FnMut(&Utterance) -> Vec<i16>) -> Vec<i16> {
    let parts = split_at_breaks(utt);
    let mut audio = Vec::new();
    for (k, part) in parts.iter().enumerate() {
        let samples = render(part);
        if !samples.is_empty() && k + 1 < parts.len() {
            audio.extend(samples);
            audio.extend(silence(if part.item_break { 100 } else { 50 }));
        } else {
            audio.extend(samples);
        }
    }
    audio
}

/// Fails with the first differences between what `render` makes of the
/// dumped phones of a language and the C++ audio in `expected`.
fn compare(
    dump: &str,
    language: &str,
    expected: impl Fn(&[&str]) -> Option<(String, String)>,
    mut render: impl FnMut(&Utterance) -> Vec<i16>,
) {
    let (mut compared, mut wrong) = (0, Vec::new());
    for line in dump.lines() {
        let fields: Vec<&str> = line.split('\t').collect();
        if fields[0] != language || fields[1] != "1" {
            continue;
        }
        let Some((count, hash)) = expected(&fields) else { continue };
        let audio = clause(&utterance(fields[3], fields[4]), &mut render);
        compared += 1;
        if audio.len().to_string() != count || fnv(&audio) != hash {
            wrong.push(format!("{language} {}: {} samples, C++ {count}", fields[2], audio.len()));
        }
    }
    eprintln!("{language}: {} of {compared} match", compared - wrong.len());
    assert!(wrong.is_empty(), "{} differ, first:\n{}", wrong.len(), wrong[..wrong.len().min(15)].join("\n"));
}

#[test]
fn formant_synthesizer_matches_the_cpp_engine() {
    let Ok(dump) = std::fs::read_to_string(golden().join("frontend_full.tsv")) else { return };
    let params = VoiceParams::default();
    let mut synth = FormantSynthesizer::new(&ZVONKO);
    for language in ["hr", "sr", "bs", "sl"] {
        compare(
            &dump,
            language,
            |fields| Some((fields.get(5)?.to_string(), fields.get(6)?.to_string())),
            |utt| {
                let mut audio = Vec::new();
                synth.render_utterance(utt, &params, &mut audio, false);
                audio
            },
        );
    }
}

#[test]
fn recorded_voices_match_the_cpp_engine() {
    let (Ok(dump), Ok(recorded)) = (
        std::fs::read_to_string(golden().join("frontend_full.tsv")),
        std::fs::read_to_string(golden().join("recorded_full.tsv")),
    ) else {
        return;
    };
    let recorded: HashMap<(&str, &str), (&str, &str)> = recorded
        .lines()
        .filter_map(|line| {
            let fields: Vec<&str> = line.split('\t').collect();
            Some(((*fields.first()?, *fields.get(1)?), (*fields.get(2)?, *fields.get(3)?)))
        })
        .collect();
    let voices = Path::new(env!("CARGO_MANIFEST_DIR")).join("data/voices");
    for (language, file) in [("hr", "Josip.bin"), ("sr", "Vlado.bin")] {
        let synth = ConcatSynthesizer::new(VoiceData::from_file(&voices.join(file), &[]).unwrap());
        compare(
            &dump,
            language,
            |fields| recorded.get(&(language, fields[2])).map(|(count, hash)| (count.to_string(), hash.to_string())),
            |utt| synth.render_utterance(utt),
        );
    }
}
