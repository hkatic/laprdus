//! `cargo xtask <step>`: the build steps Cargo does not do by itself.
//!
//! `data` turns the per-language sources in `data/<language>/` into the
//! files every platform ships:
//!
//! - `data/voices/<Voice>.bin`, one packed file per folder of recordings
//!   (`data/hr/Josip/*.wav` becomes `Josip.bin`);
//! - `data/dictionary/`: `internal.json` (the pronunciation dictionary),
//!   `spelling.json` and `emoji.json` of Croatian, the base language whose
//!   dictionaries serve Serbian and Bosnian too, and `emoji_<code>.json` of
//!   each other language that has emoji names of its own.
//!
//! Both folders are build products and are not checked in.

use laprdus::concatenative::voice_data::pack;
use laprdus::language::Language;
use std::error::Error;
use std::path::{Path, PathBuf};
use std::{env, fs};

type Outcome = Result<(), Box<dyn Error>>;

/// The language whose dictionaries are the bundled ones.
const BASE: Language = Language::Croatian;

fn root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).ancestors().nth(2).expect("tools/xtask is two levels down").to_owned()
}

/// A key of hexadecimal digits as bytes; the empty key leaves the file plain.
fn key_bytes(hex: &str) -> Result<Vec<u8>, Box<dyn Error>> {
    if hex.len() % 2 != 0 || !hex.is_ascii() {
        return Err("the key must be an even number of hexadecimal digits".into());
    }
    (0..hex.len()).step_by(2).map(|at| Ok(u8::from_str_radix(&hex[at..at + 2], 16)?)).collect()
}

fn has_recordings(folder: &Path) -> bool {
    fs::read_dir(folder).is_ok_and(|files| {
        files
            .flatten()
            .any(|file| file.path().extension().is_some_and(|extension| extension.eq_ignore_ascii_case("wav")))
    })
}

fn data(key: &[u8]) -> Outcome {
    let data = root().join("data");
    let (voices, dictionaries) = (data.join("voices"), data.join("dictionary"));
    fs::create_dir_all(&voices)?;
    fs::create_dir_all(&dictionaries)?;
    for language in Language::ALL {
        let source = data.join(language.short_code());
        let Ok(entries) = fs::read_dir(&source) else { continue };
        let mut folders: Vec<_> =
            entries.flatten().map(|entry| entry.path()).filter(|path| has_recordings(path)).collect();
        folders.sort();
        for folder in folders {
            let name = folder.file_name().unwrap_or_default().to_string_lossy().into_owned();
            let (file, missing) = pack(&folder, key);
            if !missing.is_empty() {
                return Err(
                    format!("{}: missing or unreadable recordings: {}", folder.display(), missing.join(", ")).into()
                );
            }
            fs::write(voices.join(format!("{name}.bin")), &file)?;
            println!("data/voices/{name}.bin: {} bytes", file.len());
        }
        let copies: &[(&str, String)] = if language == BASE {
            &[
                ("dictionary.json", "internal.json".into()),
                ("spelling.json", "spelling.json".into()),
                ("emoji.json", "emoji.json".into()),
            ]
        } else {
            &[("emoji.json", format!("emoji_{}.json", language.short_code()))]
        };
        for (from, to) in copies {
            let from = source.join(from);
            if from.is_file() {
                fs::copy(&from, dictionaries.join(to))?;
                println!("data/dictionary/{to}");
            } else if language == BASE {
                return Err(format!("{} is missing", from.display()).into());
            }
        }
    }
    Ok(())
}

fn main() -> Outcome {
    let arguments: Vec<String> = env::args().skip(1).collect();
    let key = match arguments.iter().position(|argument| argument == "--key") {
        Some(at) => key_bytes(arguments.get(at + 1).ok_or("--key needs a value")?)?,
        None => key_bytes(&env::var("LAPRDUS_PHONEME_KEY").unwrap_or_default())?,
    };
    match arguments.first().map(String::as_str) {
        Some("data" | "voice-data") => data(&key),
        _ => {
            eprintln!(
                "usage: cargo xtask data [--key HEX]\n\n  data   pack the recordings and stage the dictionaries (data/voices, data/dictionary)"
            );
            std::process::exit(2);
        }
    }
}
