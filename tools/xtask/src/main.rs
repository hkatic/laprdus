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
//!
//! `windows` builds the Windows programs for x64 and x86 and stages them
//! under the names the installer and the NVDA add-on take them by:
//! `dist/windows-x64/` with `laprd64.dll` (the SAPI5 driver, which also
//! exports the C API), `laprdus.exe` and `laprdgui.exe`, and
//! `dist/windows-x86/` with `laprd32.dll` and the same two programs.
//!
//! `build`, `install` and `dist` (Linux and macOS) are described in
//! `install.rs`.

mod install;

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

/// Packages built for Windows, with the file Cargo writes and its staged name
/// (`{bits}` is 64 or 32).
const WINDOWS: [(&str, &str, &str); 3] = [
    ("laprdus-sapi5", "laprdus_sapi5.dll", "laprd{bits}.dll"),
    ("laprdus-cli", "laprdus.exe", "laprdus.exe"),
    ("laprdus-configurator", "laprdgui.exe", "laprdgui.exe"),
];

fn windows() -> Outcome {
    let root = root();
    for (triple, folder, bits) in
        [("x86_64-pc-windows-msvc", "windows-x64", "64"), ("i686-pc-windows-msvc", "windows-x86", "32")]
    {
        let mut cargo = std::process::Command::new(env::var("CARGO").unwrap_or_else(|_| "cargo".into()));
        cargo.current_dir(&root).args(["build", "--release", "--target", triple]);
        cargo.args(WINDOWS.iter().flat_map(|(package, ..)| ["-p", package]));
        if !cargo.status()?.success() {
            return Err(format!("the build for {triple} failed").into());
        }
        let staged = root.join("dist").join(folder);
        fs::create_dir_all(&staged)?;
        for (_, built, name) in WINDOWS {
            let name = name.replace("{bits}", bits);
            fs::copy(root.join("target").join(triple).join("release").join(built), staged.join(&name))?;
            println!("dist/{folder}/{name}");
        }
    }
    Ok(())
}

const USAGE: &str = "usage: cargo xtask <step> [options]

  data      pack the recordings and stage the dictionaries (data/voices,
            data/dictionary)
              --key HEX          key the voice files are packed with
                                 (or LAPRDUS_PHONEME_KEY)
  windows   build the SAPI5 driver, the CLI and the configurator for x64 and
            x86 into dist/windows-x64 and dist/windows-x86
  build     Linux, macOS: build what install copies (the CLI, the C API
            library, on Linux the Speech Dispatcher module; runs data first)
  install   Linux, macOS: build and install the CLI, the C API library and
            its header, the voice data and dictionaries, and on Linux the
            Speech Dispatcher module with its configuration (runs data first)
              --destdir DIR      staging root (a package's directory)
              --prefix DIR       default /usr/local
              --libdir DIR       the library's directory, default PREFIX/lib
              --moduledir DIR    Speech Dispatcher's modules, default
                                 PREFIX/lib/speech-dispatcher-modules
  dist      Linux, macOS: the release tarball with its install script, in
            dist/<os>-<arch>/
  build, install and dist also take
              --universal        macOS: Apple Silicon and Intel in one binary
              --codesign-identity ID
                                 macOS: sign the CLI and the library
                                 (or LAPRDUS_CODESIGN_IDENTITY; - is ad hoc)
              --locked, --offline, --frozen
                                 handed on to Cargo
              --key HEX          as for data
";

/// The value after an option, when the option is there.
fn value<'a>(arguments: &'a [String], option: &str) -> Result<Option<&'a str>, Box<dyn Error>> {
    match arguments.iter().position(|argument| argument == option) {
        Some(at) => Ok(Some(arguments.get(at + 1).ok_or_else(|| format!("{option} needs a value"))?)),
        None => Ok(None),
    }
}

fn main() -> Outcome {
    let arguments: Vec<String> = env::args().skip(1).collect();
    let key = match value(&arguments, "--key")? {
        Some(key) => key_bytes(key)?,
        None => key_bytes(&env::var("LAPRDUS_PHONEME_KEY").unwrap_or_default())?,
    };
    let folder = |option| value(&arguments, option).map(|folder| folder.map(PathBuf::from));
    let has = |flag: &str| arguments.iter().any(|argument| argument == flag);
    let options = || -> Result<install::Options, Box<dyn Error>> {
        Ok(install::Options {
            destdir: folder("--destdir")?,
            prefix: folder("--prefix")?,
            libdir: folder("--libdir")?,
            moduledir: folder("--moduledir")?,
            universal: has("--universal"),
            identity: match value(&arguments, "--codesign-identity")? {
                Some(identity) => Some(identity.to_owned()),
                None => env::var("LAPRDUS_CODESIGN_IDENTITY").ok().filter(|identity| !identity.is_empty()),
            },
            cargo_flags: ["--locked", "--offline", "--frozen"]
                .into_iter()
                .filter(|flag| has(flag))
                .map(String::from)
                .collect(),
        })
    };
    match arguments.first().map(String::as_str) {
        Some("data" | "voice-data") => data(&key),
        Some("windows") => windows(),
        Some("build") => {
            let options = options()?;
            data(&key)?;
            install::build_only(&root(), &options)
        }
        Some("install") => {
            let options = options()?;
            data(&key)?;
            install::install(&root(), &options)
        }
        Some("dist") => {
            let options = options()?;
            data(&key)?;
            install::dist(&root(), &options)
        }
        _ => {
            eprint!("{USAGE}");
            std::process::exit(2);
        }
    }
}
