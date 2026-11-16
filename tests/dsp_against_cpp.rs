//! The formant synthesizer against the C++ engine, phone for phone.
//!
//! `tools/golden/dump.cpp` writes, for every text of the golden corpus, the
//! phones the C++ front end made of it and a hash of the audio the C++
//! synthesizer rendered for them. This test renders the same phones and
//! compares. The dump is a local file (`build/golden/frontend_full.tsv`); the
//! test passes without it.

use laprdus::formant::phones::{Accent, Ph, Phone};
use laprdus::formant::synthesizer::FormantSynthesizer;
use laprdus::languages::hr::formant::ZVONKO;
use laprdus::params::VoiceParams;
use laprdus::prosody::{ClauseKind, Utterance, split_at_breaks};
use laprdus::silence;

const PHONES: [Ph; Ph::COUNT] = {
    use Ph::*;
    [
        Sil, A, E, I, O, U, Schwa, Eh, Oh, Ee, Oo, Ax, W, J, V, L, Lj, M, N, Nj, Ng, R, P, B, T, D, K, G, C, Dz, Ch,
        Dzh, Tj, Dj, F, S, Z, Sh, Zh, Sj, Zj, H, Gh,
    ]
};

fn fnv(samples: &[i16]) -> u64 {
    samples
        .iter()
        .flat_map(|sample| sample.to_le_bytes())
        .fold(1469598103934665603, |h, byte| (h ^ u64::from(byte)).wrapping_mul(1099511628211))
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

#[test]
fn formant_synthesizer_matches_the_cpp_engine() {
    let root = std::path::Path::new(env!("CARGO_MANIFEST_DIR"));
    let Ok(dump) = std::fs::read_to_string(root.join("build/golden/frontend_full.tsv")) else { return };
    let corpus = std::fs::read_to_string(root.join("build/golden/corpus.txt")).unwrap();
    let texts: Vec<&str> = corpus.lines().collect();
    let params = VoiceParams::default();
    let mut synth = FormantSynthesizer::new(&ZVONKO);
    let (mut compared, mut wrong) = (0, Vec::new());
    for line in dump.lines() {
        let fields: Vec<&str> = line.split('\t').collect();
        let [language, _, index, flags, phones, count, hash] = fields[..] else { continue };
        let text = texts[index.parse::<usize>().unwrap() - 1];
        // The C++ side spoke longer texts in several breath groups.
        if text.chars().count() >= 600 || text.split_whitespace().count() > 48 {
            continue;
        }
        let parts = split_at_breaks(&utterance(flags, phones));
        let mut audio = Vec::new();
        for (k, part) in parts.iter().enumerate() {
            let before = audio.len();
            synth.render_utterance(part, &params, &mut audio, false);
            if k + 1 < parts.len() && audio.len() > before {
                audio.extend(silence(if part.item_break { 100 } else { 50 }));
            }
        }
        compared += 1;
        if audio.len() != count.parse::<usize>().unwrap() || format!("{:016x}", fnv(&audio)) != hash {
            wrong.push(format!("{language} {index} {text:?}: {} samples, C++ {count}", audio.len()));
        }
    }
    eprintln!("{} of {compared} match", compared - wrong.len());
    assert!(wrong.is_empty(), "{} differ, first:\n{}", wrong.len(), wrong[..wrong.len().min(15)].join("\n"));
}
