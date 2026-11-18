//! What the analysis makes of the recordings of a recorded voice: per
//! recording its length, the sounding part, the onset (the burst of a stop),
//! the strong part, its pitch and how many of its marks are voiced, and its
//! level every 5 ms in dB under its loudest 5 ms.
//!
//! ```text
//!     cargo run --release --example recorded_units -- data/hr/Josip
//! ```

use laprdus::concatenative::unit_bank::UnitBank;
use laprdus::concatenative::voice_data::{Sound, VoiceData};
use std::path::Path;

fn main() {
    let dir = std::env::args().nth(1).unwrap_or_else(|| "data/hr/Josip".into());
    let data = VoiceData::from_directory(Path::new(&dir)).expect("a folder of recordings");
    let bank = UnitBank::build(&data, 1.0).expect("recordings");
    let ms = |samples: i32| samples as f32 * 1000.0 / data.sample_rate() as f32;
    println!("sound      length  lead onset trail  strong        f0  voiced  level every 5 ms (dB under the loudest)");
    for sound in Sound::ALL {
        let unit = bank.unit(sound);
        if !unit.loaded || unit.sounding_length() == 0 {
            continue;
        }
        let voiced = unit.voiced.iter().filter(|&&v| v).count();
        let blocks: Vec<f32> = unit
            .samples
            .chunks(110)
            .map(|block| 10.0 * (block.iter().map(|x| x * x).sum::<f32>() / block.len() as f32 + 1e-10).log10())
            .collect();
        let top = blocks.iter().copied().fold(f32::MIN, f32::max);
        let strip: Vec<String> = blocks.iter().map(|db| format!("{:.0}", (top - db).min(99.0))).collect();
        println!(
            "{:10} {:6.0} {:5.0} {:5.0} {:5.0}  {:4.0}-{:<4.0} {:7.1}  {:2}/{:<3}  {}",
            sound.file_name(),
            ms(unit.length()),
            ms(unit.lead),
            ms(unit.onset),
            ms(unit.trail),
            ms(unit.strong_begin),
            ms(unit.strong_end),
            unit.f0,
            voiced,
            unit.marks.len(),
            strip.join(" ")
        );
    }
}
