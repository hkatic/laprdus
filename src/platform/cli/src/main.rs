//! `laprdus`: speak text or write it to a WAV file from the command line.

use laprdus::config::{UserConfig, UserSettings};
use laprdus::engine::Engine;
use laprdus::language::Language;
use laprdus::params::{NumberMode, SpellingMode, VoiceParams};
use laprdus::voice::{DEFAULT_VOICE_ID, VOICES};
use std::io::{IsTerminal, Read, Write};
use std::path::{Path, PathBuf};
use std::process::ExitCode;

/// Where the packages install the voices and dictionaries on Unix.
const DATA_DIRECTORIES: [&str; 2] = ["/usr/share/laprdus", "/usr/local/share/laprdus"];

/// What the command line asks for; the user's settings give the defaults.
struct Options {
    text: String,
    voice: String,
    /// hr, sr, bs or sl; `None`: the voice's native language.
    language: Option<String>,
    params: VoiceParams,
    spell: bool,
    output_file: Option<PathBuf>,
    input_file: Option<String>,
    data_dir: Option<PathBuf>,
    verbose: bool,
}

/// What the program is to do.
enum Command {
    Speak(Box<Options>),
    Help,
    ListVoices,
}

fn directory_of_program() -> PathBuf {
    std::env::current_exe().ok().and_then(|exe| exe.parent().map(Path::to_owned)).unwrap_or_default()
}

/// The voice data directory when `-D` does not name one: `voices` next to
/// the program (Windows), `../share/laprdus` (a tarball), or where the
/// packages install it.
fn default_data_dir() -> PathBuf {
    let program = directory_of_program();
    let candidates = [program.join("voices"), program.join("../share/laprdus"), program.clone()];
    candidates
        .into_iter()
        .chain(DATA_DIRECTORIES.map(PathBuf::from))
        .find(|dir| dir.join("Josip.bin").is_file())
        .unwrap_or_else(|| if cfg!(windows) { program.join("voices") } else { DATA_DIRECTORIES[0].into() })
}

/// A bundled dictionary: in the data directory (Unix) or in `dictionaries`
/// next to the program (Windows), under either of its names.
fn bundled_dictionary(data_dir: &Path, names: &[&str]) -> Option<PathBuf> {
    let directories = [data_dir.to_owned(), directory_of_program().join("dictionaries")];
    directories.iter().flat_map(|dir| names.iter().map(|name| dir.join(name))).find(|path| path.is_file())
}

fn help(program: &str) -> String {
    format!(
        "LaprdusTTS - Croatian/Serbian/Bosnian/Slovenian Text-to-Speech Engine
Version {version}

Usage:
  {program} [OPTIONS] \"Text to speak\"
  {program} [OPTIONS] -i input.txt
  echo \"Text\" | {program} [OPTIONS]

Options:
  -v, --voice ID             Select voice (default: {DEFAULT_VOICE_ID})
                             Available: zvonko, stojan, mirsad, josip,
                             vlado, detence, baba, djedo, and the singing
                             presets listed below
  -L, --language CODE        Language to speak: hr (Croatian), sr (Serbian),
                             bs (Bosnian) or sl (Slovenian); hr-HR and the
                             like are accepted.
                             Every voice speaks every language; without -L
                             the voice speaks its native language
  -r, --speech-rate RATE     Speech rate 0.5-2.0 (default: 1.0);
                             formant voices accept 0.25-4.0
  -p, --speech-pitch PITCH   Speech pitch 0.5-2.0 (default: 1.0);
                             formant voices accept 0.25-4.0
  -V, --speech-volume VOL    Volume 0.0-1.0 (default: 1.0)
  -B, --volume-boost         Louder speech (peak limiter), for noisy places
  -I, --inflection PERCENT   Inflection 0-100 (default: 50; 0 is a monotone)
  -a, --acceleration FACTOR  Rate multiplier 0.5-3.0 (default: 1.0)
      --head-size PERCENT    Voice quality: vocal tract length 0-100
                             (default: 50 = the voice as it is; 0 40% longer,
                             formants x0.7; 100 30% shorter, x1.4)
      --breathiness PERCENT  Breath 0-100: 0 a tense voice, 100 a whisper
                             (default: 50 = the voice as it is; formant voices)
      --roughness PERCENT    Roughness 0-100: 0 perfectly steady, above 50 a rasp
                             (default: 50; formant voices)
      --brightness PERCENT   Tone 0-100 (default: 50; 0 dull, with softer s and
                             sh, 100 sharp)
  -d, --numbers-digits       Speak numbers as digits (jedan-dva-tri)
  -s, --spell                Spell the text character by character
  -S, --spelling-speed PERCENT
                             Spelling cadence 0-100 (default: 50); lower
                             adds longer pauses, speech rate unchanged
  -m, --spelling-mode MODE   How letters are spelled: sounds (b, c, d;
                             default) or names (be, ce, de)
  -c, --comma-pauses MS      Pause duration for commas (default: 100)
  -e, --period-pauses MS     Pause duration for periods (default: 100)
  -x, --exclamationmark-pauses MS
                             Accepted; exclamation marks use the period pause
  -q, --questionmark-pauses MS
                             Accepted; question marks use the period pause
  -n, --newline-pauses MS    Pause duration for newlines (default: 100)
  -o, --output-file FILE     Output to WAV file instead of speakers
  -i, --input-file FILE      Read text from file (- for stdin)
  -D, --data-dir DIR         Voice data directory (default: {data_dir})
  -l, --list-voices          List available voices
  -w, --verbose              Enable verbose output
  -h, --help                 Show this help message

Examples:
  {program} \"Dobar dan!\"
  {program} -v vlado -L sr -r 1.5 \"Zdravo svete!\"
  {program} -v zvonko -L bs \"Dobar dan, kako ste?\"
  {program} -L sl \"Dober dan, kako ste?\"
  {program} -i document.txt -o speech.wav
  echo \"Jedan, dva, tri\" | {program}

Voices (every voice speaks every language; the language named is
the one it speaks without -L):
  zvonko   - Zvonko, Croatian male adult (formant synthesis, default)
  stojan   - Stojan, Serbian male adult (formant synthesis)
  mirsad   - Mirsad, Bosnian male adult (formant synthesis)
  josip    - Josip, Croatian male adult (recorded)
  vlado    - Vlado, Serbian male adult (recorded)
  detence  - Detence, Croatian child
  baba     - Baba, Croatian female senior
  djedo    - Đedo, Serbian male senior

Singing presets (formant voices that sing the text to a folk song;
-r sets the tempo, -p transposes, -I sets the vibrato):
  orguljas         - Orguljaš - Vila Velebita (Zvonko at the pipe organ)
  dalmacija_klapa  - Dalmacija Klapa (Zvonko as a whole klapa)
  trubac           - Trubač - Kreće se lađa francuska
                     (Stojan on the trumpet)
  harmonikas       - Harmonikaš - Kreće se lađa francuska
                     (Stojan on the accordion)
  sevdalija        - Sevdalija - Kad ja pođoh na Bembašu
                     (Mirsad singing sevdah)
  sazlija          - Sazlija - Kad ja pođoh na Bembašu
                     (Mirsad on the saz)
  lika_velebit     - Lika Velebit (Zvonko singing as himself)
  solunac          - Solunac (Stojan singing as himself)
  sarajevo_bembasa - Sarajevo Bembaša (Mirsad singing as himself)
  becarac          - Bećarac (Zvonko singing a bećarac)
",
        version = Engine::version(),
        data_dir = default_data_dir().display(),
    )
}

fn list_voices() -> String {
    let mut list = String::from(
        "Available voices:\n\n\
         ID                Native lang  Gender    Age       Name\n\
         ----------------  -----------  --------  --------  -------------------------\n",
    );
    for voice in VOICES {
        list += &format!(
            "{:<16}  {:<11}  {:<8}  {:<8}  {}\n",
            voice.id,
            voice.language.code(),
            voice.gender.as_str(),
            voice.age.as_str(),
            voice.display_name
        );
    }
    list + "\nEvery voice speaks every language: choose it with -L hr|sr|bs|sl.\n"
}

/// The options with their short letters; `true`: takes a value.
const OPTIONS: [(&str, char, bool); 28] = [
    ("voice", 'v', true),
    ("language", 'L', true),
    ("speech-rate", 'r', true),
    ("speech-pitch", 'p', true),
    ("speech-volume", 'V', true),
    ("volume-boost", 'B', false),
    ("inflection", 'I', true),
    ("acceleration", 'a', true),
    ("head-size", '1', true),
    ("breathiness", '2', true),
    ("roughness", '3', true),
    ("brightness", '4', true),
    ("numbers-digits", 'd', false),
    ("spell", 's', false),
    ("spelling-speed", 'S', true),
    ("spelling-mode", 'm', true),
    ("comma-pauses", 'c', true),
    ("period-pauses", 'e', true),
    ("exclamationmark-pauses", 'x', true),
    ("questionmark-pauses", 'q', true),
    ("newline-pauses", 'n', true),
    ("output-file", 'o', true),
    ("input-file", 'i', true),
    ("data-dir", 'D', true),
    ("help", 'h', false),
    ("list-voices", 'l', false),
    ("list", 'l', false),
    ("verbose", 'w', false),
];

/// (Option letter, value) pairs and the words of the text.
type Arguments<'a> = (Vec<(char, String)>, Vec<&'a str>);

/// The command line as (option letter, value) pairs and the words of the
/// text: options may be bundled (`-dB`), take their value attached or as the
/// next argument, and stand before or after the text; `--` ends them.
fn split_arguments(arguments: &[String]) -> Result<Arguments<'_>, String> {
    let (mut options, mut words) = (Vec::new(), Vec::new());
    let mut rest = arguments.iter();
    while let Some(argument) = rest.next() {
        let mut value_for = |letter: char, attached: Option<&str>, name: &str| match attached {
            Some(value) => Ok((letter, value.to_owned())),
            None => rest.next().map(|value| (letter, value.clone())).ok_or(format!("option {name} needs a value")),
        };
        if argument == "--" {
            words.extend(rest.by_ref().map(String::as_str));
        } else if let Some(long) = argument.strip_prefix("--") {
            let (name, attached) = long.split_once('=').map_or((long, None), |(name, value)| (name, Some(value)));
            let &(_, letter, takes_value) =
                OPTIONS.iter().find(|(known, ..)| *known == name).ok_or(format!("unknown option --{name}"))?;
            options.push(if takes_value { value_for(letter, attached, argument)? } else { (letter, String::new()) });
        } else if let Some(letters) = argument.strip_prefix('-').filter(|letters| !letters.is_empty()) {
            for (at, letter) in letters.char_indices() {
                let &(_, _, takes_value) = OPTIONS
                    .iter()
                    .find(|&&(_, known, _)| known == letter && !letter.is_ascii_digit())
                    .ok_or(format!("unknown option -{letter}"))?;
                if takes_value {
                    let attached = Some(&letters[at + letter.len_utf8()..]).filter(|value| !value.is_empty());
                    options.push(value_for(letter, attached, &format!("-{letter}"))?);
                    break;
                }
                options.push((letter, String::new()));
            }
        } else {
            words.push(argument);
        }
    }
    Ok((options, words))
}

fn number<T: std::str::FromStr>(value: &str, letter: char) -> Result<T, String> {
    value.trim().parse().map_err(|_| format!("option -{letter}: '{value}' is not a number"))
}

fn parse(arguments: &[String], settings: Option<&UserSettings>) -> Result<Command, String> {
    let mut options = Options {
        text: String::new(),
        voice: settings
            .map(|settings| settings.default_voice.clone())
            .filter(|voice| !voice.is_empty())
            .unwrap_or(DEFAULT_VOICE_ID.into()),
        language: None,
        params: settings.map_or_else(VoiceParams::default, UserSettings::to_voice_params),
        spell: false,
        output_file: None,
        input_file: None,
        data_dir: None,
        verbose: false,
    };
    let (given, words) = split_arguments(arguments)?;
    for (letter, value) in given {
        let params = &mut options.params;
        // The engine narrows the rate and pitch for the recorded voices.
        match letter {
            'v' => options.voice = value,
            'L' => options.language = Some(value),
            'r' => params.speed = number::<f32>(&value, letter)?.clamp(0.25, 4.0),
            'p' => params.user_pitch = number::<f32>(&value, letter)?.clamp(0.25, 4.0),
            'V' => params.volume = number::<f32>(&value, letter)?.clamp(0.0, 1.0),
            'B' => params.volume_boost = true,
            'I' => params.inflection_level = (number::<f32>(&value, letter)? / 100.0).clamp(0.0, 1.0),
            'a' => params.acceleration = number::<f32>(&value, letter)?.clamp(0.5, 3.0),
            '1' => params.head_size = number::<i32>(&value, letter)?.clamp(0, 100),
            '2' => params.breathiness = number::<i32>(&value, letter)?.clamp(0, 100),
            '3' => params.roughness = number::<i32>(&value, letter)?.clamp(0, 100),
            '4' => params.brightness = number::<i32>(&value, letter)?.clamp(0, 100),
            'd' => params.number_mode = NumberMode::DigitByDigit,
            's' => options.spell = true,
            'S' => params.spelling_speed = number::<i32>(&value, letter)?.clamp(0, 100),
            'm' => {
                params.spelling_mode = match value.as_str() {
                    "sounds" => SpellingMode::LetterSounds,
                    "names" => SpellingMode::LetterNames,
                    _ => return Err(format!("spelling mode must be 'sounds' or 'names', not '{value}'")),
                }
            }
            'c' => params.pause_settings.comma_pause_ms = number(&value, letter)?,
            'e' => params.pause_settings.sentence_pause_ms = number(&value, letter)?,
            // Exclamation and question marks use the period pause.
            'x' | 'q' => drop(number::<u32>(&value, letter)?),
            'n' => params.pause_settings.newline_pause_ms = number(&value, letter)?,
            'o' => options.output_file = Some(value.into()),
            'i' => options.input_file = Some(value),
            'D' => options.data_dir = Some(value.into()),
            'h' => return Ok(Command::Help),
            'l' => return Ok(Command::ListVoices),
            'w' => options.verbose = true,
            _ => return Err(format!("unknown option -{letter}")),
        }
    }
    options.text = words.join(" ");
    Ok(Command::Speak(Box::new(options)))
}

fn read_stdin() -> String {
    let mut bytes = Vec::new();
    // What could be read is spoken.
    let _ = std::io::stdin().read_to_end(&mut bytes);
    String::from_utf8_lossy(&bytes).into_owned()
}

/// A 16-bit PCM mono WAV file.
fn wav(samples: &[i16], sample_rate: u32) -> Vec<u8> {
    let data_size = (samples.len() * 2) as u32;
    let mut file = Vec::with_capacity(44 + samples.len() * 2);
    file.extend(b"RIFF");
    file.extend((36 + data_size).to_le_bytes());
    file.extend(b"WAVEfmt ");
    file.extend(16u32.to_le_bytes());
    file.extend(1u16.to_le_bytes()); // PCM
    file.extend(1u16.to_le_bytes()); // channels
    file.extend(sample_rate.to_le_bytes());
    file.extend((sample_rate * 2).to_le_bytes()); // bytes per second
    file.extend(2u16.to_le_bytes()); // block align
    file.extend(16u16.to_le_bytes()); // bits per sample
    file.extend(b"data");
    file.extend(data_size.to_le_bytes());
    file.extend(samples.iter().flat_map(|sample| sample.to_le_bytes()));
    file
}

fn play(samples: Vec<i16>, sample_rate: u32) -> Result<(), String> {
    let stream =
        rodio::OutputStreamBuilder::open_default_stream().map_err(|error| format!("no audio device: {error}"))?;
    let sink = rodio::Sink::connect_new(stream.mixer());
    let samples: Vec<f32> = samples.into_iter().map(|sample| f32::from(sample) / 32768.0).collect();
    sink.append(rodio::buffer::SamplesBuffer::new(1, sample_rate, samples));
    sink.sleep_until_end();
    Ok(())
}

fn speak(mut options: Options, config: Option<&UserConfig>) -> Result<(), String> {
    if let Some(input) = &options.input_file {
        options.text = match input.as_str() {
            "-" => read_stdin(),
            file => std::fs::read(file)
                .map(|bytes| String::from_utf8_lossy(&bytes).into_owned())
                .map_err(|_| format!("Cannot open file: {file}"))?,
        };
        if options.text.is_empty() {
            return Err(String::new());
        }
    }
    if options.text.is_empty() && !std::io::stdin().is_terminal() {
        options.text = read_stdin();
    }
    if options.text.is_empty() {
        return Err("No text to speak. Provide text as argument, use -i, or pipe to stdin.\nUse -h for help.".into());
    }

    let params = options.params;
    if options.verbose {
        println!("Voice: {}", options.voice);
        println!("Rate: {}", params.speed);
        println!("Pitch: {}", params.user_pitch);
        println!("Volume: {}{}", params.volume, if params.volume_boost { " (boosted)" } else { "" });
        println!("Inflection: {}%", (params.inflection_level * 100.0).round());
        println!("Acceleration: {}", params.acceleration);
        println!(
            "Voice quality: head size {}, breathiness {}, roughness {}, brightness {}",
            params.head_size, params.breathiness, params.roughness, params.brightness
        );
        println!("Text length: {} characters", options.text.chars().count());
    }

    let data_dir = options.data_dir.clone().unwrap_or_else(default_data_dir);
    let mut engine = Engine::new();
    engine
        .set_voice(&options.voice, &data_dir)
        .map_err(|error| format!("Failed to set voice '{}': {error}", options.voice))?;
    if let Some(tag) = &options.language {
        let language = Language::parse(tag).ok_or(format!("Unknown language '{tag}' (use hr, sr, bs or sl)"))?;
        engine.set_language(Some(language));
    }
    if options.verbose {
        println!("Language: {}", engine.language().code());
    }

    // The bundled dictionaries, then the user's, which extend and override them.
    if let Some(path) = bundled_dictionary(&data_dir, &["internal.json", "dictionary.json"]) {
        engine.dictionary().load_file(&path);
    }
    if let Some(path) = bundled_dictionary(&data_dir, &["spelling.json"]) {
        engine.load_spelling_dictionary_file(&path);
    }
    if let Some(path) = bundled_dictionary(&data_dir, &["emoji.json"]) {
        engine.load_emoji_dictionary_file(&path);
    }
    if let Some(config) = config.filter(|config| config.settings.user_dictionaries_enabled) {
        let user_file = |name| config.path(name).filter(|path| path.is_file());
        let content = |name| user_file(name).and_then(|path| std::fs::read_to_string(path).ok());
        if let Some(path) = user_file(UserConfig::DICTIONARY) {
            engine.dictionary().append_file(&path);
        }
        if let Some(json) = content(UserConfig::SPELLING) {
            engine.append_spelling_dictionary(&json);
        }
        if let Some(json) = content(UserConfig::EMOJI) {
            engine.append_emoji_dictionary(&json);
        }
        // The accent lexicon (word stress, length, tone).
        if let Some(path) = user_file(UserConfig::ACCENTS) {
            engine.load_accent_lexicon_file(&path);
            if options.verbose {
                eprintln!("Accent lexicon: {}", engine.accent_lexicon_report());
            }
        }
    }

    // The voice has its own character pitch; everything else is the user's.
    let pitch = engine.params().pitch;
    engine.set_params(&VoiceParams { pitch, ..params });
    engine.set_pause_settings(&params.pause_settings);

    let audio = if options.spell {
        engine.synthesize_spelled(&options.text, true).map(|(audio, _)| audio)
    } else {
        engine.synthesize(&options.text)
    };
    let audio = audio.ok().filter(|audio| !audio.is_empty()).ok_or("Synthesis failed")?;
    let sample_rate = engine.sample_rate();
    if options.verbose {
        println!("Synthesized {} samples ({sample_rate} Hz, 16 bit, 1 ch)", audio.len());
    }

    match &options.output_file {
        Some(file) => {
            std::fs::write(file, wav(&audio, sample_rate))
                .map_err(|_| format!("Cannot create output file: {}", file.display()))?;
            if options.verbose {
                println!("Wrote {}", file.display());
            }
            Ok(())
        }
        None => play(audio, sample_rate).map_err(|error| format!("{error}. Use -o to write a WAV file.")),
    }
}

fn main() -> ExitCode {
    let mut arguments = std::env::args();
    let program = arguments.next().unwrap_or_else(|| "laprdus".into());
    let arguments: Vec<String> = arguments.collect();

    // The user's configuration gives the defaults; the command line overrides them.
    let mut config = UserConfig::new();
    let config = config.load_settings().then_some(&config);

    let failed = |message: &str| {
        if !message.is_empty() {
            eprintln!("Error: {message}");
        }
        ExitCode::FAILURE
    };
    match parse(&arguments, config.map(|config| &config.settings)) {
        Err(message) => {
            eprintln!("Error: {message}\nUse -h for help.");
            ExitCode::FAILURE
        }
        Ok(Command::Help) => {
            print!("{}", help(&program));
            ExitCode::SUCCESS
        }
        Ok(Command::ListVoices) => {
            print!("{}", list_voices());
            ExitCode::SUCCESS
        }
        Ok(Command::Speak(options)) => match speak(*options, config) {
            Ok(()) => {
                // A closed pipe is not an error worth reporting.
                let _ = std::io::stdout().flush();
                ExitCode::SUCCESS
            }
            Err(message) => failed(&message),
        },
    }
}
