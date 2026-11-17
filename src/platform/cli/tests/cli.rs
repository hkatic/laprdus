//! The command-line program: its options, its listings and the WAV files it
//! writes.
//!
//! Run with `cargo test --release -p laprdus-cli --test cli` (the recorded
//! voices render slowly unoptimized). The recorded voices need the packed
//! voice files: `LAPRDUS_DATA` names their directory, else `data/voices` of
//! the repository, which `cargo xtask data` fills.
//!
//! Nothing here plays sound: every run that reaches synthesis writes a WAV
//! file (`-o`) into a temporary directory, and the standard input is closed.
//! The program reads the user's `settings.json` and dictionaries, and writes
//! a default `settings.json` when there is none; the tests point it at a
//! configuration directory of their own, so the user's is neither read nor
//! touched.

use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};

/// The directory of `Josip.bin` and `Vlado.bin`.
fn data_dir() -> PathBuf {
    let dir = std::env::var_os("LAPRDUS_DATA")
        .filter(|dir| !dir.is_empty())
        .map_or_else(|| Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../data/voices"), PathBuf::from);
    for file in ["Josip.bin", "Vlado.bin"] {
        assert!(
            dir.join(file).is_file(),
            "{} is missing: run `cargo xtask data` first, or point LAPRDUS_DATA at the packed voices",
            dir.join(file).display()
        );
    }
    dir
}

/// A directory of one test's files, removed when the test ends.
struct Scratch(PathBuf);

impl Scratch {
    fn new(test: &str) -> Self {
        let dir = std::env::temp_dir().join(format!("laprdus_cli_test_{}_{test}", std::process::id()));
        let _ = std::fs::remove_dir_all(&dir);
        std::fs::create_dir_all(dir.join("config")).expect("a temporary directory");
        Self(dir)
    }

    /// The path of a file in the directory; an old one of that name is removed.
    fn file(&self, name: &str) -> PathBuf {
        let path = self.0.join(name);
        let _ = std::fs::remove_file(&path);
        path
    }

    /// Runs the program with `-D <data directory>` before the arguments and
    /// returns its exit code and what it printed, standard output and
    /// standard error together.
    fn run<S: AsRef<std::ffi::OsStr>>(&self, arguments: &[S]) -> (i32, String) {
        let output = Command::new(env!("CARGO_BIN_EXE_laprdus"))
            .arg("-D")
            .arg(data_dir())
            .args(arguments)
            // A configuration directory of the test's own.
            .env("APPDATA", self.0.join("config"))
            .env("XDG_CONFIG_HOME", self.0.join("config"))
            .stdin(Stdio::null())
            .stderr(Stdio::piped())
            .stdout(Stdio::piped())
            .output()
            .expect("the laprdus program runs");
        let text = String::from_utf8_lossy(&output.stdout).into_owned() + &String::from_utf8_lossy(&output.stderr);
        (output.status.code().unwrap_or(-1), text)
    }

    /// Runs the program writing a WAV file: `<arguments> -o <file> <text>`.
    fn write(&self, arguments: &[&str], file: &Path, text: &str) -> (i32, String) {
        let mut all: Vec<&std::ffi::OsStr> = arguments.iter().map(AsRef::as_ref).collect();
        all.extend([std::ffi::OsStr::new("-o"), file.as_os_str(), std::ffi::OsStr::new(text)]);
        self.run(&all)
    }
}

impl Drop for Scratch {
    fn drop(&mut self) {
        let _ = std::fs::remove_dir_all(&self.0);
    }
}

fn file_size(path: &Path) -> u64 {
    std::fs::metadata(path).unwrap_or_else(|error| panic!("{}: {error}", path.display())).len()
}

// =============================================================================
// Argument parsing
// =============================================================================

/// CLI displays help with -h
#[test]
fn cli_displays_help_with_h() {
    let scratch = Scratch::new("help_h");
    let (exit_code, output) = scratch.run(&["-h"]);

    assert_eq!(exit_code, 0, "{output}");
    for expected in ["LaprdusTTS", "Usage", "--voice", "--language", "--speech-rate"] {
        assert!(output.contains(expected), "{expected:?} is missing in:\n{output}");
    }
    // The voices are listed by their current ids, the old ones are gone
    for expected in ["djedo", "dalmacija_klapa", "sarajevo_bembasa"] {
        assert!(output.contains(expected), "{expected:?} is missing in:\n{output}");
    }
    assert!(!output.contains(" djed,"), "{output}");
    assert!(!output.contains("pjevac"), "{output}");
}

/// CLI displays help with --help
#[test]
fn cli_displays_help_with_help() {
    let scratch = Scratch::new("help_long");
    let (exit_code, output) = scratch.run(&["--help"]);

    assert_eq!(exit_code, 0, "{output}");
    assert!(output.contains("LaprdusTTS"), "{output}");
}

/// CLI lists voices with -l
#[test]
fn cli_lists_voices_with_l() {
    let scratch = Scratch::new("list");
    let (exit_code, output) = scratch.run(&["-l"]);

    assert_eq!(exit_code, 0, "{output}");
    // Every voice by id with its native language, in registry order
    const IDS: [&str; 18] = [
        "josip",
        "vlado",
        "detence",
        "baba",
        "djedo",
        "zvonko",
        "stojan",
        "mirsad",
        "orguljas",
        "dalmacija_klapa",
        "trubac",
        "harmonikas",
        "sevdalija",
        "sazlija",
        "lika_velebit",
        "solunac",
        "sarajevo_bembasa",
        "becarac",
    ];
    let mut previous = 0;
    for id in IDS {
        let at = output.find(&format!("\n{id} ")).unwrap_or_else(|| panic!("{id} is not listed in:\n{output}"));
        assert!(at > previous, "{id} is out of order in:\n{output}");
        previous = at;
    }
    for code in ["hr-HR", "sr-RS", "bs-BA"] {
        assert!(output.contains(code), "{code} is missing in:\n{output}");
    }
    // No voice is natively Slovenian: the language is chosen with -L
    // Display names from the registry, UTF-8
    assert!(output.contains("Đedo"), "{output}");
    assert!(output.contains("Dalmacija Klapa"), "{output}");
    // ... and the note that the language is chosen apart from the voice
    assert!(output.contains("Every voice speaks every language"), "{output}");
    assert!(output.contains("-L hr|sr|bs|sl"), "{output}");
    // --list is the same listing; -L is the language now
    let (list_code, list_output) = scratch.run(&["--list"]);
    assert_eq!(list_code, 0, "{list_output}");
    assert_eq!(list_output, output);
}

/// CLI chooses the language with -L
#[test]
fn cli_chooses_the_language_with_l() {
    let scratch = Scratch::new("language");
    let output_file = scratch.file("language.wav");

    // Every language with the default voice
    for language in ["hr", "sr", "bs", "sl", "sr-RS", "HR_HR", "bos", "slv", "sl-SI"] {
        let _ = std::fs::remove_file(&output_file);
        let (exit_code, output) = scratch.write(&["-L", language], &output_file, "1000");
        assert_eq!(exit_code, 0, "{language}: {output}");
        assert!(output_file.is_file(), "{language}: no file written");
    }

    // The language changes the words, the voice stays
    let (hr_code, hr_output) = scratch.write(&["-v", "vlado", "-L", "hr"], &output_file, "1000");
    assert_eq!(hr_code, 0, "{hr_output}");
    let croatian = file_size(&output_file);
    let (sr_code, sr_output) = scratch.write(&["-v", "vlado", "-L", "sr"], &output_file, "1000");
    assert_eq!(sr_code, 0, "{sr_output}");
    let serbian = file_size(&output_file);
    let (native_code, native_output) = scratch.write(&["-v", "vlado"], &output_file, "1000");
    assert_eq!(native_code, 0, "{native_output}");
    // tisuću and hiljadu differ; Vlado's native language is Serbian
    assert_ne!(croatian, serbian);
    assert_eq!(file_size(&output_file), serbian);
    let (sl_code, sl_output) = scratch.write(&["-v", "vlado", "-L", "sl"], &output_file, "1000");
    assert_eq!(sl_code, 0, "{sl_output}");
    assert_ne!(file_size(&output_file), serbian, "Slovenian 1000 is tisoč"); // tisoč

    // Verbose output names the language in use
    let (exit_code, output) = scratch.write(&["-w", "-v", "josip", "-L", "bs"], &output_file, "Test");
    assert_eq!(exit_code, 0, "{output}");
    assert!(output.contains("Language: bs-BA"), "{output}");
    let (native_code, native_output) = scratch.write(&["-w", "-v", "josip"], &output_file, "Test");
    assert_eq!(native_code, 0, "{native_output}");
    assert!(native_output.contains("Language: hr-HR"), "{native_output}");

    // An unknown language is refused
    let (exit_code, output) = scratch.write(&["-L", "de"], &output_file, "Test");
    assert_ne!(exit_code, 0, "{output}");
    assert!(output.contains("Unknown language"), "{output}");
}

/// CLI fails gracefully without text
#[test]
fn cli_fails_gracefully_without_text() {
    // Nothing on the command line and nothing on the (closed) standard
    // input: the program stops before it loads a voice.
    let scratch = Scratch::new("no_text");
    let (exit_code, output) = scratch.run::<&str>(&[]);

    assert_ne!(exit_code, 0, "{output}");
    assert!(output.contains("No text"), "{output}");
}

/// CLI rejects invalid voice
#[test]
fn cli_rejects_invalid_voice() {
    // The original gave no output file: the voice is refused before anything
    // is synthesized. The file is named here all the same, so that a program
    // that wrongly went on would write it and not play.
    let scratch = Scratch::new("invalid_voice");
    let output_file = scratch.file("invalid_voice.wav");
    let (exit_code, output) = scratch.write(&["-v", "invalid_voice"], &output_file, "Test");

    assert_ne!(exit_code, 0, "{output}");
    assert!(output.contains("Failed"), "{output}");
    assert!(!output_file.exists(), "nothing is written for a voice that does not exist");
}

// =============================================================================
// Output
// =============================================================================

/// CLI generates WAV file with -o
#[test]
fn cli_generates_wav_file_with_o() {
    let scratch = Scratch::new("output");
    let test_file = scratch.file("output.wav");

    let (exit_code, output) = scratch.write(&[], &test_file, "Test");

    assert_eq!(exit_code, 0, "{output}");
    assert!(test_file.is_file());

    // Check WAV file has content
    assert!(file_size(&test_file) > 44); // WAV header is 44 bytes minimum

    // Check WAV header
    let wav = std::fs::read(&test_file).expect("the WAV file");
    assert_eq!(&wav[..4], b"RIFF");
}

/// CLI reads from input file with -i
#[test]
fn cli_reads_from_input_file_with_i() {
    let scratch = Scratch::new("input");
    let input_file = scratch.file("input.txt");
    let output_file = scratch.file("output2.wav");
    std::fs::write(&input_file, "Dobar dan!").expect("the input file");

    let (exit_code, output) = scratch.run(&[
        std::ffi::OsStr::new("-i"),
        input_file.as_os_str(),
        std::ffi::OsStr::new("-o"),
        output_file.as_os_str(),
    ]);

    assert_eq!(exit_code, 0, "{output}");
    assert!(output_file.is_file());
    assert!(file_size(&output_file) > 44);
}

// =============================================================================
// Parameters
// =============================================================================

/// CLI accepts valid speech rate
#[test]
fn cli_accepts_valid_speech_rate() {
    let scratch = Scratch::new("rate");
    let output_file = scratch.file("rate.wav");
    // Slow, normal, fast
    for rate in ["0.5", "1.0", "2.0"] {
        let (exit_code, output) = scratch.write(&["-r", rate], &output_file, "Test");
        assert_eq!(exit_code, 0, "rate {rate}: {output}");
    }
}

/// CLI accepts valid pitch
#[test]
fn cli_accepts_valid_pitch() {
    let scratch = Scratch::new("pitch");
    let output_file = scratch.file("pitch.wav");
    // Low, high
    for pitch in ["0.5", "2.0"] {
        let (exit_code, output) = scratch.write(&["-p", pitch], &output_file, "Test");
        assert_eq!(exit_code, 0, "pitch {pitch}: {output}");
    }
}

/// CLI accepts valid volume
#[test]
fn cli_accepts_valid_volume() {
    let scratch = Scratch::new("volume");
    let output_file = scratch.file("volume.wav");
    // Silent, full
    for volume in ["0.0", "1.0"] {
        let (exit_code, output) = scratch.write(&["-V", volume], &output_file, "Test");
        assert_eq!(exit_code, 0, "volume {volume}: {output}");
    }
}

/// CLI accepts number mode flag
#[test]
fn cli_accepts_number_mode_flag() {
    let scratch = Scratch::new("digits");
    let output_file = scratch.file("digits.wav");

    let (exit_code, output) = scratch.write(&["-d"], &output_file, "123");
    assert_eq!(exit_code, 0, "{output}");
    assert!(output_file.is_file());
}

/// CLI accepts pause settings
#[test]
fn cli_accepts_pause_settings() {
    let scratch = Scratch::new("pauses");
    let output_file = scratch.file("pauses.wav");

    // The \n is a backslash and an n, as the shell handed it over.
    let (exit_code, output) = scratch.write(
        &["-c", "100", "-e", "200", "-x", "150", "-q", "120", "-n", "300"],
        &output_file,
        "Test, test. Test! Test? Test\\nTest",
    );

    assert_eq!(exit_code, 0, "{output}");
    assert!(output_file.is_file());
}

// =============================================================================
// Voice selection
// =============================================================================

/// CLI synthesizes with different voices
#[test]
fn cli_synthesizes_with_different_voices() {
    let scratch = Scratch::new("voices");
    // The old id djed still loads Đedo
    for voice in ["josip", "vlado", "detence", "baba", "djedo", "djed"] {
        let output_file = scratch.file("voice.wav");

        let (exit_code, output) = scratch.write(&["-v", voice], &output_file, "Test");

        assert_eq!(exit_code, 0, "{voice}: {output}");
        assert!(output_file.is_file(), "{voice}");
        assert!(file_size(&output_file) > 44, "{voice}");
    }
}
