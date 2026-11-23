//! Timings of the engine, to compare a change with what was there before:
//!
//! ```text
//! cargo xtask data
//! cargo run --release --example bench
//! ```
//!
//! For each of three voices (a formant voice, a recorded one and a derived
//! one): the time to load the voice, to speak a short phrase (what a screen
//! reader sends most), to speak a paragraph and how many times faster than
//! real time that is, and the time until the first clause of the paragraph is
//! handed over by the streaming synthesis. Then the time of a change of voice
//! and of language. The best of several runs is printed: the least disturbed.

use laprdus::engine::Engine;
use laprdus::language::Language;
use std::ops::ControlFlow;
use std::path::Path;
use std::time::{Duration, Instant};

const PHRASE: &str = "Postavke, gumb";
const PARAGRAPH: &str = "Jutros je na zagrebačkom Glavnom kolodvoru vlak iz Splita kasnio četrdeset minuta, \
    a putnici su strpljivo čekali na peronu. Je li itko znao zašto? Nitko nije rekao ništa: razglas je šutio, \
    šalteri su bili zatvoreni, a ploča s polascima pokazivala je samo crtice. Tek oko 8:30 stigla je obavijest \
    da je kvar na pruzi kod Karlovca otklonjen i da će vlak stići za 15 minuta! Ljudi su odahnuli, kupili kavu \
    i novine, pa nastavili čekati, ovaj put malo mirnije.";

fn best(runs: usize, mut run: impl FnMut() -> Duration) -> Duration {
    (0..runs).map(|_| run()).min().unwrap_or_default()
}

fn timed<T>(run: impl FnOnce() -> T) -> (Duration, T) {
    let start = Instant::now();
    let value = run();
    (start.elapsed(), value)
}

fn ms(time: Duration) -> f64 {
    time.as_secs_f64() * 1000.0
}

fn main() {
    let data = std::env::var("LAPRDUS_DATA").unwrap_or_else(|_| "data/voices".into());
    let data = Path::new(&data);
    let dictionary = Path::new("data/dictionary/internal.json");

    for voice in ["zvonko", "josip", "djedo"] {
        let load = best(5, || {
            timed(|| {
                let mut engine = Engine::new();
                engine.set_voice(voice, data).expect("the voice loads (cargo xtask data)");
                engine.dictionary().load_file(dictionary);
                // The first synthesis pays for what is built on first use.
                engine.synthesize("a").expect("audio");
            })
            .0
        });

        let mut engine = Engine::new();
        engine.set_voice(voice, data).expect("the voice loads");
        engine.dictionary().load_file(dictionary);

        let phrase = best(200, || timed(|| engine.synthesize(PHRASE).expect("audio")).0);
        let mut samples = 0;
        let paragraph = best(10, || {
            let (time, audio) = timed(|| engine.synthesize(PARAGRAPH).expect("audio"));
            samples = audio.len();
            time
        });
        let first_clause = best(10, || {
            let start = Instant::now();
            let mut first = None;
            let _ = engine.synthesize_streaming(PARAGRAPH, 0, |_| {
                first = Some(start.elapsed());
                ControlFlow::Break(())
            });
            first.unwrap_or_default()
        });
        let seconds = samples as f64 / f64::from(laprdus::SAMPLE_RATE);
        println!(
            "{voice:8} load {:7.2} ms | phrase {:6.3} ms | paragraph {:7.2} ms for {seconds:5.2} s ({:5.0}x real time) | first clause {:6.2} ms",
            ms(load),
            ms(phrase),
            ms(paragraph),
            seconds / paragraph.as_secs_f64(),
            ms(first_clause),
        );
    }

    // Changes of voice and of language, as a settings screen or a screen
    // reader's language switching makes them, each followed by a phrase.
    let mut engine = Engine::new();
    let switch = |engine: &mut Engine, from: &str, to: &str| {
        best(5, || {
            engine.set_voice(from, data).expect("the voice loads");
            engine.synthesize(PHRASE).expect("audio");
            timed(|| {
                engine.set_voice(to, data).expect("the voice loads");
                engine.synthesize(PHRASE).expect("audio");
            })
            .0
        })
    };
    for (from, to) in [("zvonko", "josip"), ("josip", "zvonko"), ("detence", "baba"), ("josip", "vlado")] {
        println!("voice {from} -> {to}: {:.2} ms", ms(switch(&mut engine, from, to)));
    }
    engine.set_voice("zvonko", data).expect("the voice loads");
    let language = best(20, || {
        engine.set_language(Some(Language::parse("hr").expect("a language")));
        engine.synthesize(PHRASE).expect("audio");
        timed(|| {
            engine.set_language(Some(Language::parse("sr").expect("a language")));
            engine.synthesize(PHRASE).expect("audio");
        })
        .0
    });
    println!("language hr -> sr: {:.2} ms", ms(language));
}
