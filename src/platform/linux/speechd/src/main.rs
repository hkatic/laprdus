//! `sd_laprdus`: the Speech Dispatcher output module.
//!
//! Speech Dispatcher starts the module as a process and talks to it over its
//! standard input and output in the module protocol (the one its own
//! `module_main.c` implements for modules written in C): `INIT`, `SET`,
//! `SPEAK`, `STOP`, `LIST VOICES` ... in, numbered replies and events out.
//! The audio is sent back to the server, which plays it.
//!
//! Every voice is listed for every language; `SET language` switches the
//! language and keeps the voice. SSIP rate and pitch are relative (0 is
//! normal), so they scale the rate and pitch of the user's `settings.json`.

use laprdus::engine::Engine;
use laprdus::language::Language;
use laprdus::voice::{self, DEFAULT_VOICE_ID, VOICES, Voice};
use std::io::{BufRead, Write};
use std::path::{Path, PathBuf};
use std::process::ExitCode;
use std::sync::mpsc::{Receiver, channel};

/// Where the packages install the voices and dictionaries.
const DATA_DIRECTORIES: [&str; 2] = ["/usr/share/laprdus", "/usr/local/share/laprdus"];

/// Bytes of audio sent to the server at a time; a `STOP` is looked for
/// between two chunks.
const MAX_CHUNK_BYTES: usize = 10_000;

const BAD_SYNTAX: &str = "302 ERROR BAD SYNTAX";
const BAD_PARAMETER: &str = "303 ERROR INVALID PARAMETER OR VALUE";

/// What a message is to the server.
#[derive(Clone, Copy, PartialEq, Eq)]
enum MessageType {
    Text,
    SoundIcon,
    Char,
    Key,
}

/// The speed or pitch factor of an SSIP rate or pitch (-100..100): 0.5 at
/// -100, 1.0 at 0, 2.0 at 100.
fn relative_factor(value: i32) -> f32 {
    1.0 + value as f32 / if value <= 0 { 200.0 } else { 100.0 }
}

/// The volume of an SSIP volume (-100..100): 0.0 to 1.0.
fn volume_of(volume: i32) -> f32 {
    (volume + 100) as f32 / 200.0
}

/// Text without its markup: Speech Dispatcher wraps text in `<speak>`.
fn strip_ssml_tags(text: &str) -> String {
    let mut in_tag = false;
    text.chars()
        .filter(|&c| {
            let tag = in_tag || c == '<';
            in_tag = tag && c != '>';
            !tag
        })
        .collect()
}

/// A name in lower case with č, ć, đ, š, ž as c, c, dj, s, z: the way voice
/// ids are written.
fn ascii_fold(name: &str) -> String {
    name.chars()
        .flat_map(|c| match c {
            'Č' | 'č' | 'Ć' | 'ć' => vec!['c'],
            'Đ' | 'đ' => vec!['d', 'j'],
            'Š' | 'š' => vec!['s'],
            'Ž' | 'ž' => vec!['z'],
            _ => vec![c.to_ascii_lowercase()],
        })
        .collect()
}

/// The voice a client names: by id, an id of an earlier version, its display
/// name, or either without diacritics and in any case ("Djed", "Solist",
/// "Đedo", "ZVONKO").
fn find_voice(name: &str) -> Option<&'static Voice> {
    let wanted = ascii_fold(name);
    voice::find(name).or_else(|| voice::find(&wanted)).or_else(|| {
        VOICES.iter().copied().find(|voice| {
            voice.id.eq_ignore_ascii_case(name)
                || voice.display_name.to_lowercase() == name.to_lowercase()
                || ascii_fold(voice.display_name) == wanted
        })
    })
}

/// The voice of an SSIP voice type: the same for every language.
fn symbolic_voice(voice_type: &str) -> Option<&'static str> {
    match voice_type.to_ascii_lowercase().as_str() {
        "male1" => Some(DEFAULT_VOICE_ID),
        "male2" => Some("josip"),
        "male3" => Some("djedo"),
        "female1" | "female3" => Some("baba"),
        "child_male" | "child_female" => Some("detence"),
        _ => None,
    }
}

/// A chunk of samples as the server takes it: a header, the bytes with line
/// feeds and the escape byte escaped, and the closing line.
fn audio_message(samples: &[i16], sample_rate: u32) -> Vec<u8> {
    const ESCAPE: u8 = 0x7d;
    const INVERT: u8 = 1 << 5;
    let mut message = format!(
        "705-bits=16\n705-num_channels=1\n705-sample_rate={sample_rate}\n705-num_samples={}\n705-big_endian={}\n705-AUDIO\0",
        samples.len(),
        i32::from(cfg!(target_endian = "big")),
    )
    .into_bytes();
    for byte in samples.iter().flat_map(|sample| sample.to_ne_bytes()) {
        if byte == ESCAPE || byte == b'\n' {
            message.extend([ESCAPE, byte ^ INVERT]);
        } else {
            message.push(byte);
        }
    }
    message.extend(b"\n705 AUDIO\n");
    message
}

fn number(value: &str) -> i32 {
    value.trim().parse::<i32>().unwrap_or(0).clamp(-100, 100)
}

struct Module {
    engine: Engine,
    /// The lines the server sends, read by a thread of their own so that a
    /// `STOP` can be seen while audio is going out.
    lines: Receiver<String>,
    data_dir: PathBuf,
    // What the server asked for (-100..100 each) ...
    rate: i32,
    pitch: i32,
    pitch_range: i32,
    volume: i32,
    // ... relative to the user's settings.
    configured_speed: f32,
    configured_pitch: f32,
    configured_inflection: f32,
    spelling_mode: bool,
    debug: bool,
    speaking: bool,
    /// A `STOP` or `PAUSE` came while speaking.
    stop_requested: bool,
    /// A `STOP` or `PAUSE` came: no more audio of this message.
    should_stop: bool,
    quit: bool,
}

/// The server closed its end.
struct BrokenPipe;

impl Module {
    fn send(&self, reply: &str) {
        self.send_bytes(format!("{reply}\n").as_bytes());
    }

    fn send_bytes(&self, bytes: &[u8]) {
        let mut out = std::io::stdout().lock();
        // A server that has gone is noticed when its input ends.
        let _ = out.write_all(bytes).and_then(|()| out.flush());
    }

    fn log(&self, message: impl FnOnce() -> String) {
        if self.debug {
            eprintln!("[laprdus] {}", message());
        }
    }

    /// The next line from the server; without `block`, only one that has
    /// already come.
    fn read_line(&self, block: bool) -> Option<String> {
        if block { self.lines.recv().ok() } else { self.lines.try_recv().ok() }
    }

    /// The lines of a block up to the line of a single period.
    fn read_block(&self) -> Option<Vec<String>> {
        let mut block = Vec::new();
        loop {
            match self.read_line(true)? {
                line if line == ".\n" => return Some(block),
                line => block.push(line),
            }
        }
    }

    fn set_voice(&mut self, name: &str) -> bool {
        let Some(voice) = find_voice(name) else {
            eprintln!("[laprdus ERROR] Voice not found: {name}");
            return false;
        };
        match self.engine.set_voice(voice.id, &self.data_dir) {
            Ok(()) => {
                self.log(|| format!("Set voice: {} (language {})", voice.id, self.engine.language().code()));
                true
            }
            Err(error) => {
                eprintln!("[laprdus ERROR] Failed to set voice: {} ({error})", voice.id);
                false
            }
        }
    }

    fn apply_parameters(&mut self) {
        let speed = self.configured_speed * relative_factor(self.rate);
        let user_pitch = self.configured_pitch * relative_factor(self.pitch);
        let volume = volume_of(self.volume);
        // The pitch range scales the configured inflection level: 0 keeps
        // it, 100 doubles it, -100 is a monotone.
        let inflection = (self.configured_inflection * (1.0 + self.pitch_range as f32 / 100.0)).clamp(0.0, 1.0);
        self.engine.update_params(|params| {
            params.speed = speed;
            params.user_pitch = user_pitch;
            params.volume = volume;
            params.inflection_level = inflection;
        });
    }

    /// One setting of a `SET` block; false for a value that cannot be used.
    fn set(&mut self, variable: &str, value: &str) -> bool {
        self.log(|| format!("SET {variable} = {value}"));
        let unset = value == "NULL" || value.is_empty();
        match variable {
            // A voice type (MALE1, FEMALE1 ...) or a voice by name.
            "voice" => return self.set_voice(symbolic_voice(value).unwrap_or(value)),
            // Orca sends the voice's name as it was listed; NULL keeps the voice.
            "synthesis_voice" if !unset => return self.set_voice(value),
            // Every voice speaks every language: the language is switched
            // and the voice kept; one that is not spoken changes nothing.
            "language" if !unset => {
                if let Some(language) = Language::parse(value) {
                    self.engine.set_language(Some(language));
                }
            }
            "rate" => self.rate = number(value),
            "pitch" => self.pitch = number(value),
            "pitch_range" => self.pitch_range = number(value),
            "volume" => self.volume = number(value),
            "spelling_mode" => self.spelling_mode = value == "on",
            _ => return true,
        }
        if matches!(variable, "rate" | "pitch" | "pitch_range" | "volume") {
            self.apply_parameters();
        }
        true
    }

    /// A block of `variable=value` lines; the reply is the last error, or
    /// nothing when every line was taken.
    fn settings(&mut self, receiving: &str, mut set: impl FnMut(&mut Self, &str, &str) -> bool) -> Option<bool> {
        self.send(receiving);
        let mut error = None;
        for line in self.read_block()? {
            match line.trim_end_matches('\n').split_once('=').filter(|(_, value)| !value.is_empty()) {
                Some((variable, value)) if set(self, variable, value) => {}
                Some(_) => error = Some(BAD_PARAMETER),
                None => error = Some(BAD_SYNTAX),
            }
        }
        if let Some(error) = error {
            self.send(error);
        }
        Some(error.is_none())
    }

    fn list_voices(&self, line: &str) {
        let mut requested = line.split([' ', '\n']).filter(|word| !word.is_empty()).skip(2);
        let (language, variant) = (requested.next(), requested.next());
        let mut list = String::new();
        for voice in VOICES {
            for spoken in Language::ALL {
                let code = spoken.code();
                let wanted = language.is_none_or(|language| {
                    language.eq_ignore_ascii_case(code) || language.eq_ignore_ascii_case(spoken.short_code())
                }) && variant.is_none_or(|variant| variant.eq_ignore_ascii_case(voice.id));
                if wanted {
                    list += &format!("200-{}\t{code}\t{}\n", voice.display_name, voice.id);
                }
            }
        }
        list += if list.is_empty() { "304 CANT LIST VOICES\n" } else { "200 OK VOICE LIST SENT\n" };
        self.send_bytes(list.as_bytes());
    }

    fn debug_command(&mut self, line: &str) {
        let mut words = line.split([' ', '\n']).filter(|word| !word.is_empty()).skip(1);
        match (words.next(), words.next()) {
            (Some("ON"), Some(_file)) => self.debug = true,
            (Some("OFF"), _) => self.debug = false,
            _ => return self.send(BAD_SYNTAX),
        }
        self.send(if self.debug { "200 OK DEBUGGING ON" } else { "200 OK DEBUGGING OFF" });
    }

    fn stop(&mut self, pause: bool) {
        self.should_stop = true;
        if self.speaking {
            self.stop_requested = true;
            if pause {
                self.send("704 PAUSE");
            }
        }
    }

    /// A message from the server: its text block, then the speech.
    fn speak_command(&mut self, message_type: MessageType) {
        self.send("202 OK RECEIVING MESSAGE");
        let Some(lines) = self.read_block() else { return };
        // A line that begins with a period has one more in front of it.
        let mut text: String = lines.iter().map(|line| line.strip_prefix('.').unwrap_or(line)).collect();
        text.pop(); // the last line feed
        if text.is_empty() {
            return self.send("301 ERROR CANT SPEAK");
        }
        if message_type != MessageType::Text && lines.len() > 1 {
            return self.send("305 DATA MORE THAN ONE LINE");
        }
        if matches!(message_type, MessageType::Key | MessageType::Char) && text == "space" {
            text = " ".into();
        }
        self.should_stop = false;
        self.speak(&text, message_type);
    }

    fn speak(&mut self, text: &str, message_type: MessageType) {
        self.stop_requested = false;
        self.speaking = true;
        self.send("200 OK SPEAKING");
        self.apply_parameters();
        let stopped = !self.speak_text(&strip_ssml_tags(text), message_type);
        self.speaking = false;
        self.send(if stopped { "703 STOP" } else { "702 END" });
    }

    /// Synthesizes the text and sends its audio; false when nothing was
    /// spoken to the end.
    fn speak_text(&mut self, text: &str, message_type: MessageType) -> bool {
        if text.is_empty() {
            return false;
        }
        // Characters and keys are spelled, and everything in spelling mode.
        let spelled = self.spelling_mode || matches!(message_type, MessageType::Char | MessageType::Key);
        let audio = if spelled {
            self.engine.synthesize_spelled(text, true).map(|(audio, _)| audio)
        } else {
            self.engine.synthesize(text)
        };
        let audio = match audio {
            Ok(audio) if !audio.is_empty() => audio,
            Ok(_) => return false,
            Err(error) => {
                eprintln!("[laprdus ERROR] Synthesis failed: {error}");
                return false;
            }
        };
        self.send("701 BEGIN");

        // A stop may have come while the text was being synthesized, and may
        // come between any two chunks.
        let _ = self.process(false);
        let sample_rate = self.engine.sample_rate();
        for chunk in audio.chunks(MAX_CHUNK_BYTES / size_of::<i16>()) {
            if self.stop_requested || self.should_stop || self.quit {
                break;
            }
            self.send_bytes(&audio_message(chunk, sample_rate));
            let _ = self.process(false);
        }
        !self.stop_requested
    }

    /// Carries out the server's commands: with `block` until it says `QUIT`
    /// or closes its end, without it only one that has already come.
    fn process(&mut self, block: bool) -> Result<(), BrokenPipe> {
        while !self.quit {
            let Some(line) = self.read_line(block) else { return if block { Err(BrokenPipe) } else { Ok(()) } };
            match line.as_str() {
                "SPEAK\n" => self.speak_command(MessageType::Text),
                "SOUND_ICON\n" => self.speak_command(MessageType::SoundIcon),
                "CHAR\n" => self.speak_command(MessageType::Char),
                "KEY\n" => self.speak_command(MessageType::Key),
                "STOP\n" => self.stop(false),
                "PAUSE\n" => self.stop(true),
                "SET\n" => {
                    if self.settings("203 OK RECEIVING SETTINGS", Self::set) == Some(true) {
                        self.send("203 OK SETTINGS RECEIVED");
                    }
                }
                // The server plays the audio: its settings need nothing here.
                "AUDIO\n" => {
                    if self.settings("207 OK RECEIVING AUDIO SETTINGS", |_, _, _| true) == Some(true) {
                        self.send("203 OK AUDIO INITIALIZED");
                    }
                }
                "LOGLEVEL\n" => {
                    let set = |module: &mut Self, variable: &str, value: &str| {
                        if variable == "log_level" {
                            module.debug = value.trim().parse::<i32>().is_ok_and(|level| level >= 3);
                        }
                        true
                    };
                    if self.settings("207 OK RECEIVING LOGLEVEL SETTINGS", set) == Some(true) {
                        self.send("203 OK LOGLEVEL SET");
                    }
                }
                "QUIT\n" => {
                    self.stop(false);
                    self.quit = true;
                    self.send("210 OK QUIT");
                }
                line if line.starts_with("LIST VOICES") => self.list_voices(line),
                line if line.starts_with("DEBUG") => self.debug_command(line),
                _ => self.send("300 ERR UNKNOWN COMMAND"),
            }
            if !block {
                break;
            }
        }
        Ok(())
    }
}

/// The lines of standard input, each with its line feed, as they come.
fn lines_of_stdin() -> Receiver<String> {
    let (sender, receiver) = channel();
    std::thread::spawn(move || {
        let mut input = std::io::stdin().lock();
        loop {
            let mut line = Vec::new();
            match input.read_until(b'\n', &mut line) {
                Ok(n) if n > 0 && sender.send(String::from_utf8_lossy(&line).into_owned()).is_ok() => {}
                _ => break,
            }
        }
    });
    receiver
}

fn main() -> ExitCode {
    // The configuration file Speech Dispatcher names holds only the voice
    // table the server itself reads.
    let lines = lines_of_stdin();
    if lines.recv().ok().as_deref() != Some("INIT\n") {
        eprintln!("ERROR: Server did not start with INIT");
        return ExitCode::from(3);
    }

    let data_dir = DATA_DIRECTORIES
        .map(Path::new)
        .into_iter()
        .find(|dir| dir.join("Josip.bin").is_file())
        .unwrap_or(Path::new(DATA_DIRECTORIES[0]));
    let mut module = Module {
        engine: Engine::new(),
        lines,
        data_dir: data_dir.to_owned(),
        rate: 0,
        pitch: 0,
        pitch_range: 0,
        volume: 0,
        configured_speed: 1.0,
        configured_pitch: 1.0,
        configured_inflection: 0.5,
        spelling_mode: false,
        debug: false,
        speaking: false,
        stop_requested: false,
        should_stop: false,
        quit: false,
    };
    if !module.set_voice(DEFAULT_VOICE_ID) {
        module.send("399-Laprdus could not load its default voice\n399 ERR CANT INIT MODULE");
        return ExitCode::FAILURE;
    }
    module.engine.dictionary().load_file(&data_dir.join("internal.json"));
    module.engine.load_spelling_dictionary_file(&data_dir.join("spelling.json"));
    // The user's settings from ~/.config/Laprdus: rate, pitch, pauses,
    // dictionaries. The server's rate and pitch are relative to them.
    module.engine.load_user_config();
    let params = module.engine.params();
    (module.configured_speed, module.configured_pitch, module.configured_inflection) =
        (params.speed, params.user_pitch, params.inflection_level);
    module.send("299-Laprdus initialized\n299 OK LOADED SUCCESSFULLY");

    match module.process(true) {
        Ok(()) => ExitCode::SUCCESS,
        Err(BrokenPipe) => {
            eprintln!("[laprdus ERROR] Broken pipe, exiting...");
            module.send("399 ERR MODULE CLOSED");
            ExitCode::from(255)
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn rate_pitch_and_volume_mapping() {
        for (value, factor) in [(-100, 0.5), (-50, 0.75), (0, 1.0), (50, 1.5), (100, 2.0)] {
            assert_eq!(relative_factor(value), factor);
        }
        for (volume, level) in [(-100, 0.0), (0, 0.5), (100, 1.0)] {
            assert_eq!(volume_of(volume), level);
        }
        assert_eq!((number("250"), number("-250"), number("x"), number(" 7\n")), (100, -100, 0, 7));
    }

    #[test]
    fn voices_by_id_old_id_name_and_folded_name() {
        for (name, id) in [
            ("zvonko", "zvonko"),
            ("ZVONKO", "zvonko"),
            ("Zvonko", "zvonko"),
            ("djed", "djedo"),
            ("Djed", "djedo"),
            ("Đedo", "djedo"),
            ("đedo", "djedo"),
            ("Solist", "sarajevo_bembasa"),
            ("Dalmacija Klapa", "dalmacija_klapa"),
            ("Bećarac", "becarac"),
            ("Becarac", "becarac"),
        ] {
            assert_eq!(find_voice(name).map(|voice| voice.id), Some(id), "{name}");
        }
        assert!(find_voice("nobody").is_none() && find_voice("").is_none());
        assert_eq!(
            (symbolic_voice("MALE1"), symbolic_voice("child_female"), symbolic_voice("x")),
            (Some("zvonko"), Some("detence"), None)
        );
    }

    #[test]
    fn markup_and_audio() {
        assert_eq!(strip_ssml_tags("<speak>Dobar <mark name=\"1\"/>dan > noć</speak>"), "Dobar dan > noć");
        let message = audio_message(&[0x0a7d, 1], 22050);
        let (header, rest) = message.split_at(message.iter().position(|&byte| byte == 0).unwrap() + 1);
        assert!(header.starts_with(b"705-bits=16\n705-num_channels=1\n705-sample_rate=22050\n705-num_samples=2\n"));
        assert_eq!(
            rest,
            [0x7d, 0x5d, 0x7d, 0x2a, 1, 0, b'\n', b'7', b'0', b'5', b' ', b'A', b'U', b'D', b'I', b'O', b'\n']
        );
    }
}
