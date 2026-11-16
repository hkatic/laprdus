//! The engine: text in, 16-bit mono audio at 22050 Hz out.
//!
//! 1. The text is prepared: emoji and pronunciation dictionaries, numbers to
//!    words.
//! 2. It is split into clauses at punctuation and line breaks.
//! 3. The front end of the language spoken turns each clause into phones.
//! 4. The voice renders them: a recorded voice by TD-PSOLA, a formant voice
//!    by rule. Rate, pitch, volume and intonation are applied at the source
//!    by both.

use crate::concatenative::synthesizer::ConcatSynthesizer;
use crate::concatenative::voice_data::{VoiceData, VoiceDataError};
use crate::config::UserConfig;
use crate::dictionary::emoji::{EmojiDictionary, replace_emojis};
use crate::dictionary::pronunciation::{DictionaryEntry, PronunciationDictionary};
use crate::dictionary::spelling::SpellingDictionary;
use crate::formant::synthesizer::FormantSynthesizer;
use crate::language::Language;
use crate::languages::{self, sl};
use crate::params::{CONCAT_SPEED_MAX, NumberMode, PauseSettings, SpellingMode, VoiceParams, spelling_rate_factor};
use crate::prosody::{item_break_pause_ms, minor_break_pause_ms};
use crate::text::Punctuation;
use crate::text::clauses::{Clause, split_clauses};
use crate::text::frontend::{UserLexicon, is_foreign_letter, is_letter_name, letter_name, spelling_letter};
use crate::text::numbers::NumberReader;
use crate::voice::{self, Synthesis, Voice};
use crate::{SAMPLE_RATE, silence};
use std::collections::HashMap;
use std::fmt;
use std::path::{Path, PathBuf};
use std::sync::Arc;

/// Why the engine could not do what was asked.
#[derive(Debug)]
pub enum Error {
    /// No voice is loaded.
    NotInitialized,
    /// No voice has this id.
    VoiceNotFound(String),
    /// The recordings of a voice could not be loaded from this path.
    VoiceData(PathBuf, VoiceDataError),
    /// Nothing could be synthesized.
    Synthesis(String),
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::NotInitialized => f.write_str("Engine not initialized"),
            Self::VoiceNotFound(id) => write!(f, "Voice not found: {id}"),
            Self::VoiceData(path, error) => write!(f, "Failed to load phoneme data: {} ({error})", path.display()),
            Self::Synthesis(message) => f.write_str(message),
        }
    }
}

impl std::error::Error for Error {}

/// The synthesizer of the voice in use.
enum Backend {
    None,
    Recorded(Box<ConcatSynthesizer>),
    Formant(Box<FormantSynthesizer>),
}

/// Receives the audio of a streaming synthesis, clause by clause.
struct Sink<'a> {
    callback: &'a mut dyn FnMut(&[i16]),
    /// Samples per chunk; 0 hands every clause over whole.
    chunk: usize,
}

/// A text-to-speech engine with one voice loaded.
pub struct Engine {
    backend: Backend,
    /// The voice in use, when it was chosen by id.
    voice: Option<&'static Voice>,
    /// The language spoken now ...
    language: Language,
    /// ... and the one the host asked for, which outlives a change of voice.
    chosen_language: Option<Language>,
    frontend: languages::Frontend,
    numbers: NumberReader,
    data_directory: PathBuf,
    /// Base pitch of the voice in use (a child, a grandmother) ...
    voice_base_pitch: f32,
    /// ... and the character pitch the host set on top of it.
    character_pitch: f32,
    params: VoiceParams,
    /// The pauses after clauses. They follow the pause setters, not
    /// `set_params`, whose pauses reach the spelling pause alone.
    clause_pauses: PauseSettings,

    dictionary: PronunciationDictionary,
    /// Bundled: digits, punctuation, symbols.
    spelling: SpellingDictionary,
    /// The user's: wins over everything.
    user_spelling: SpellingDictionary,
    /// Bundled: Croatian names, used for hr, sr and bs.
    emoji: EmojiDictionary,
    /// Bundled, in one language's own words.
    emoji_by_language: HashMap<Language, EmojiDictionary>,
    /// The user's entries, for every language.
    user_emoji: EmojiDictionary,

    /// The user's accent entries, kept across voice and language changes.
    user_lexicon: Option<Arc<UserLexicon>>,
    user_lexicon_report: String,
}

impl Default for Engine {
    fn default() -> Self {
        Self::new()
    }
}

impl Engine {
    pub fn new() -> Self {
        let language = Language::default();
        Self {
            backend: Backend::None,
            voice: None,
            language,
            chosen_language: None,
            frontend: languages::Frontend::new(language),
            numbers: NumberReader::new(language),
            data_directory: PathBuf::new(),
            voice_base_pitch: 1.0,
            character_pitch: 1.0,
            params: VoiceParams::default(),
            clause_pauses: PauseSettings::default(),
            dictionary: PronunciationDictionary::default(),
            spelling: SpellingDictionary::default(),
            user_spelling: SpellingDictionary::default(),
            emoji: EmojiDictionary::default(),
            emoji_by_language: HashMap::new(),
            user_emoji: EmojiDictionary::default(),
            user_lexicon: None,
            user_lexicon_report: String::new(),
        }
    }

    /// The engine's version.
    pub fn version() -> &'static str {
        env!("CARGO_PKG_VERSION")
    }

    // -------------------------------------------------------------------------
    // Voice and language
    // -------------------------------------------------------------------------

    /// Switches to a voice by its id (ids of earlier versions are accepted).
    /// A recorded voice loads its data file from `data_directory`; a formant
    /// voice needs none. The voice speaks the language the host chose with
    /// [`set_language`](Self::set_language), else its own, and gets its base
    /// pitch, so a derived voice sounds like itself without a pitch call
    /// after every change of voice.
    pub fn set_voice(&mut self, id: &str, data_directory: &Path) -> Result<(), Error> {
        let same_directory = self.data_directory == data_directory;
        self.set_voice_with(id, same_directory, |data_file| {
            let path = data_directory.join(data_file);
            let data = if path.is_file() { VoiceData::from_file(&path, &[]) } else { VoiceData::from_directory(&path) };
            data.map_err(|error| Error::VoiceData(path, error))
        })?;
        self.data_directory = data_directory.to_owned();
        Ok(())
    }

    /// Switches to a voice whose data file, if it has one, `load` reads by
    /// its name: from the assets of an application, say.
    pub fn set_voice_from(&mut self, id: &str, load: impl FnOnce(&str) -> Option<Vec<u8>>) -> Result<(), Error> {
        self.set_voice_with(id, false, |data_file| {
            let error = |error| Error::VoiceData(data_file.into(), error);
            let packed =
                load(data_file).ok_or_else(|| error(VoiceDataError::Io(std::io::ErrorKind::NotFound.into())))?;
            VoiceData::from_packed(&packed, &[]).map_err(error)
        })
    }

    /// `reuse`: recordings already loaded for the same data file may stay.
    fn set_voice_with(
        &mut self,
        id: &str,
        reuse: bool,
        load: impl FnOnce(&'static str) -> Result<VoiceData, Error>,
    ) -> Result<(), Error> {
        let voice = voice::find(id).ok_or_else(|| Error::VoiceNotFound(id.to_owned()))?;
        match voice.synthesis {
            Synthesis::Formant(speaker) => {
                if self.voice != Some(voice) || !self.is_formant() {
                    self.backend = Backend::Formant(Box::new(FormantSynthesizer::new(speaker)));
                }
            }
            Synthesis::Concatenative { data_file, .. } => {
                let loaded = matches!(self.backend, Backend::Recorded(_))
                    && self.voice.is_some_and(|current| current.data_file() == Some(data_file));
                if !(reuse && loaded) {
                    self.use_recordings(load(data_file)?);
                }
            }
        }
        self.voice = Some(voice);
        self.voice_base_pitch = voice.base_pitch;
        self.speak(self.chosen_language.unwrap_or(voice.language));
        self.apply_character_pitch();
        Ok(())
    }

    /// Loads the recordings of a voice from a packed file or a directory of
    /// WAV files, without choosing a voice by id. The voice in use stays when
    /// they cannot be loaded.
    pub fn load_recordings(&mut self, path: &Path, key: &[u8]) -> Result<(), Error> {
        let data = if path.is_file() { VoiceData::from_file(path, key) } else { VoiceData::from_directory(path) };
        self.use_recordings(data.map_err(|error| Error::VoiceData(path.to_owned(), error))?);
        Ok(())
    }

    /// Loads the recordings of a voice from a packed file in memory.
    pub fn load_recordings_from_memory(&mut self, data: &[u8], key: &[u8]) -> Result<(), Error> {
        let data = VoiceData::from_packed(data, key).map_err(|error| Error::VoiceData(PathBuf::new(), error))?;
        self.use_recordings(data);
        Ok(())
    }

    fn use_recordings(&mut self, data: VoiceData) {
        let mut synthesizer = ConcatSynthesizer::new(data);
        synthesizer.set_voice_params(&self.params);
        self.backend = Backend::Recorded(Box::new(synthesizer));
    }

    /// The voice in use, when it was chosen by id.
    pub fn voice(&self) -> Option<&'static Voice> {
        self.voice
    }

    pub fn is_initialized(&self) -> bool {
        !matches!(self.backend, Backend::None)
    }

    pub fn is_formant(&self) -> bool {
        matches!(self.backend, Backend::Formant(_))
    }

    /// Chooses the language every voice speaks from now on; `None` goes back
    /// to the native language of the voice in use (and of later ones).
    pub fn set_language(&mut self, language: Option<Language>) {
        self.chosen_language = language;
        if let Some(language) = language.or(self.voice.map(|voice| voice.language)) {
            self.speak(language);
        }
    }

    /// The language spoken now.
    pub fn language(&self) -> Language {
        self.language
    }

    /// Speaks a language: the text front end is rebuilt with that language's
    /// rules and lexicon (the user's accent entries are kept) and the number
    /// words follow. The voice itself is unchanged.
    fn speak(&mut self, language: Language) {
        if language != self.language {
            self.language = language;
            self.frontend = languages::Frontend::new(language);
            self.frontend.set_user_lexicon(self.user_lexicon.clone());
            self.numbers = NumberReader::new(language);
        }
    }

    // -------------------------------------------------------------------------
    // Voice parameters
    // -------------------------------------------------------------------------

    pub fn params(&self) -> &VoiceParams {
        &self.params
    }

    /// Sets every parameter at once; values are brought into their ranges.
    pub fn set_params(&mut self, params: &VoiceParams) {
        self.params = *params;
        self.params.clamp();
        if let Backend::Recorded(synthesizer) = &mut self.backend {
            synthesizer.set_voice_params(&self.params);
        }
    }

    /// Changes some parameters.
    pub fn update_params(&mut self, change: impl FnOnce(&mut VoiceParams)) {
        let mut params = self.params;
        change(&mut params);
        self.set_params(&params);
    }

    /// Sets the voice character pitch on top of the voice's own base pitch.
    /// On the recorded voices it shifts the formants too.
    pub fn set_character_pitch(&mut self, pitch: f32) {
        self.character_pitch = if pitch.is_finite() { pitch } else { 1.0 };
        self.apply_character_pitch();
    }

    fn apply_character_pitch(&mut self) {
        let pitch = (self.voice_base_pitch * self.character_pitch).clamp(0.25, 4.0);
        self.update_params(|params| params.pitch = pitch);
    }

    /// The pauses after clauses and between spelled characters.
    pub fn set_pause_settings(&mut self, pauses: &PauseSettings) {
        self.params.pause_settings = *pauses;
        self.params.pause_settings.clamp();
        self.clause_pauses = self.params.pause_settings;
    }

    /// Changes some pauses.
    pub fn update_pauses(&mut self, change: impl FnOnce(&mut PauseSettings)) {
        let mut pauses = self.params.pause_settings;
        change(&mut pauses);
        self.set_pause_settings(&pauses);
    }

    /// The pause between spelled characters alone (ms).
    pub fn set_spelling_pause(&mut self, pause_ms: u32) {
        self.params.pause_settings.spelling_pause_ms = pause_ms.min(PauseSettings::MAX_MS);
    }

    /// Sample rate of the audio.
    pub fn sample_rate(&self) -> u32 {
        match &self.backend {
            Backend::Recorded(synthesizer) => synthesizer.data().sample_rate(),
            _ => SAMPLE_RATE,
        }
    }

    /// Bytes of recordings held.
    pub fn memory_usage(&self) -> usize {
        match &self.backend {
            Backend::Recorded(synthesizer) => synthesizer.data().memory_usage(),
            _ => 0,
        }
    }

    // -------------------------------------------------------------------------
    // Synthesis
    // -------------------------------------------------------------------------

    /// Synthesizes text.
    pub fn synthesize(&mut self, text: &str) -> Result<Vec<i16>, Error> {
        self.speak_text(text, None)
    }

    /// Synthesizes text and hands the audio over clause by clause, in chunks
    /// of `chunk_ms` (0: every clause whole).
    pub fn synthesize_streaming(
        &mut self,
        text: &str,
        chunk_ms: u32,
        mut callback: impl FnMut(&[i16]),
    ) -> Result<(), Error> {
        let chunk = (SAMPLE_RATE * chunk_ms / 1000) as usize;
        if let Some((words, letter)) = split_capital_announcement(text) {
            // Short: synthesized whole, then handed over in chunks.
            self.begin_utterance()?;
            let audio = self.synthesize_capital(&words, &letter)?;
            audio.chunks(if chunk > 0 { chunk } else { audio.len().max(1) }).for_each(callback);
            return Ok(());
        }
        self.speak_text(text, Some(Sink { callback: &mut callback, chunk })).map(drop)
    }

    /// A singing preset starts its song over with every utterance.
    fn begin_utterance(&mut self) -> Result<(), Error> {
        match &mut self.backend {
            Backend::None => return Err(Error::NotInitialized),
            Backend::Formant(synthesizer) => synthesizer.rewind_song(),
            Backend::Recorded(_) => {}
        }
        Ok(())
    }

    fn speak_text(&mut self, text: &str, sink: Option<Sink<'_>>) -> Result<Vec<i16>, Error> {
        self.begin_utterance()?;
        if text.is_empty() {
            return Ok(Vec::new()); // valid, just no audio
        }
        if sink.is_none()
            && let Some((words, letter)) = split_capital_announcement(text)
        {
            return self.synthesize_capital(&words, &letter);
        }
        let clauses = self.segment(&self.preprocess(text));
        Ok(self.synthesize_clauses(&clauses, sink))
    }

    /// A capital letter as a screen reader announces it: the words as text,
    /// then the letter spelled, so the spelling mode, spelling speed and the
    /// user's spelling entries apply.
    fn synthesize_capital(&mut self, words: &str, letter: &str) -> Result<Vec<i16>, Error> {
        let mut audio = self.synthesize(words)?;
        audio.extend(self.synthesize_spelled(letter, true)?.0);
        Ok(audio)
    }

    /// Emoji and pronunciation dictionaries, then numbers to words.
    fn preprocess(&self, text: &str) -> String {
        let mut text = text.to_owned();
        // The user's emoji entries, then the dictionary of the language
        // spoken, then the bundled one.
        if self.params.emoji_enabled {
            let of_language = self.emoji_by_language.get(&self.language);
            let layers: Vec<&EmojiDictionary> =
                [Some(&self.user_emoji), of_language, Some(&self.emoji)].into_iter().flatten().collect();
            text = replace_emojis(&text, &layers);
        }
        if !self.dictionary.is_empty() {
            text = self.dictionary.apply(&text);
        }
        match self.params.number_mode {
            NumberMode::WholeNumbers => self.numbers.convert_numbers(&text),
            NumberMode::DigitByDigit => self.numbers.convert_digits(&text),
        }
    }

    /// The clauses of prepared text. Some readings are told by what the
    /// front end, which sees one clause at a time, does not see; a language
    /// may mark its clauses for that.
    fn segment(&self, text: &str) -> Vec<Clause> {
        let mut clauses = split_clauses(&text.chars().collect::<Vec<_>>());
        if self.language == Language::Slovenian {
            sl::marks::mark_clauses(&mut clauses);
        }
        clauses
    }

    fn synthesize_clauses(&mut self, clauses: &[Clause], mut sink: Option<Sink<'_>>) -> Vec<i16> {
        let mut result = Vec::new();
        // A short pause inside a clause (between the hours and minutes of
        // 12:30, the parts of 10.11.2026.) is half of the user's comma pause
        // and shrinks with the speech rate; the melody goes on over it. The
        // recorded voices stop at 4x. The pause between a cell's contents and
        // its address is the whole comma pause at every rate.
        let speed = self.params.effective_speed();
        let rate = if self.is_formant() { speed } else { speed.min(CONCAT_SPEED_MAX) };
        let comma_pause = self.clause_pauses.after(Punctuation::Comma);
        let (minor_pause, item_pause) = (minor_break_pause_ms(comma_pause, rate), item_break_pause_ms(comma_pause));
        let letter_sounds = self.params.spelling_mode == SpellingMode::LetterSounds;

        // A clause after a comma, semicolon or colon goes on the sentence and
        // starts lower than one that opens a sentence.
        let mut sentence_initial = true;
        for clause in clauses.iter().filter(|clause| !clause.text.is_empty()) {
            let mut audio = match &mut self.backend {
                Backend::Formant(synthesizer) => synthesizer.synthesize_clause(
                    &self.frontend,
                    &clause.text,
                    clause.punct,
                    &self.params,
                    sentence_initial,
                    minor_pause,
                    item_pause,
                ),
                Backend::Recorded(synthesizer) => synthesizer.synthesize_clause(
                    &self.frontend,
                    &clause.text,
                    clause.punct,
                    sentence_initial,
                    letter_sounds,
                    minor_pause,
                    item_pause,
                ),
                Backend::None => Vec::new(),
            };
            sentence_initial =
                !matches!(clause.punct, Punctuation::Comma | Punctuation::Semicolon | Punctuation::Colon);
            if audio.is_empty() {
                continue;
            }
            audio.extend(silence(self.clause_pauses.after(clause.punct)));
            match &mut sink {
                Some(sink) => {
                    audio.chunks(if sink.chunk > 0 { sink.chunk } else { audio.len() }).for_each(&mut *sink.callback)
                }
                None => result.append(&mut audio),
            }
        }
        result
    }

    // -------------------------------------------------------------------------
    // Spelling
    // -------------------------------------------------------------------------

    /// Reads text character by character, at the normal speech rate, with a
    /// pause after every character. The spelling speed stretches each
    /// character's time slot by adding silence after it, on top of the
    /// spelling pause. The last character gets the same pause, so a word
    /// spelled in one call has the spacing of individual character calls,
    /// unless `final_pause` is false: then that last pause is left out and
    /// its length (ms) returned, for a host that times the gap itself.
    pub fn synthesize_spelled(&mut self, text: &str, final_pause: bool) -> Result<(Vec<i16>, u32), Error> {
        self.begin_utterance()?;
        let pause_samples = (SAMPLE_RATE * self.params.pause_settings.spelling_pause_ms / 1000) as usize;
        let extra_pause_factor = 1.0 / f64::from(spelling_rate_factor(self.params.spelling_speed)) - 1.0;

        let mut audio = Vec::new();
        let mut final_pause_ms = 0;
        let mut characters = text.chars().peekable();
        let mut buffer = [0; 4];
        while let Some(character) = characters.next() {
            // A character that cannot be synthesized is skipped.
            let Ok(samples) = self.spell_character(character.encode_utf8(&mut buffer)) else { continue };
            let pause = pause_samples + (samples.len() as f64 * extra_pause_factor).round() as usize;
            audio.extend(samples);
            let last = characters.peek().is_none();
            if !last || final_pause {
                audio.resize(audio.len() + pause, 0);
            }
            if last {
                final_pause_ms = ((pause * 1000 + SAMPLE_RATE as usize / 2) / SAMPLE_RATE as usize) as u32;
            }
        }
        if text.is_empty() || !audio.is_empty() {
            Ok((audio, final_pause_ms))
        } else {
            Err(Error::Synthesis(String::new()))
        }
    }

    /// One character: a letter by its sound or its name in the language
    /// spoken, anything else by its spelling entry, or as text when it has
    /// none. The user's spelling entries win, except that in sound mode an
    /// entry that is merely a letter's name gives way to the sound.
    fn spell_character(&mut self, character: &str) -> Result<Vec<i16>, Error> {
        let letter = character.chars().next().map(spelling_letter).unwrap_or_default();
        let user = self.user_spelling.find(character).map(str::to_owned);
        let user_is_a_letter_name =
            || user.as_deref().is_some_and(|user| is_letter_name(&letter, &user.chars().collect::<Vec<_>>()));
        // q, x and y are not letters of the language: named in both modes.
        // Nor is w, but its name ends in another letter, and that follows
        // the mode.
        let sound_mode = self.params.spelling_mode == SpellingMode::LetterSounds;
        let sounds = sound_mode && !is_foreign_letter(&letter);

        let text = if letter.is_empty() {
            // The bundled spelling dictionary has Croatian names; a
            // language's own names come before it, the user's before both.
            user.or_else(|| self.frontend.spelled_name(character).map(str::to_owned))
                .or_else(|| self.spelling.find(character).map(str::to_owned))
                .unwrap_or_else(|| character.to_owned())
        } else if sounds {
            if user.is_none() || user_is_a_letter_name() {
                self.begin_utterance()?;
                return Ok(match &mut self.backend {
                    Backend::Formant(synthesizer) => {
                        synthesizer.synthesize_letter_sound(&self.frontend, &letter, &self.params)
                    }
                    Backend::Recorded(synthesizer) => synthesizer.synthesize_letter_sound(&self.frontend, &letter),
                    Backend::None => Vec::new(),
                });
            }
            user.unwrap_or_default()
        } else if letter == ['w'] && (user.is_none() || (sound_mode && user_is_a_letter_name())) {
            // The front end's reading: dvojni ve by names, dvojni və by sounds.
            "w".to_owned()
        } else {
            user.unwrap_or_else(|| letter_name(&letter, self.language).into_iter().collect())
        };
        self.synthesize(&text)
    }

    // -------------------------------------------------------------------------
    // Dictionaries
    // -------------------------------------------------------------------------

    /// The pronunciation dictionary: word-level replacements.
    pub fn dictionary(&mut self) -> &mut PronunciationDictionary {
        &mut self.dictionary
    }

    pub fn add_pronunciation(&mut self, grapheme: &str, phoneme: &str, case_sensitive: bool, whole_word: bool) {
        self.dictionary.add(DictionaryEntry {
            grapheme: grapheme.to_owned(),
            phoneme: phoneme.to_owned(),
            case_sensitive,
            whole_word,
        });
    }

    /// Replaces the bundled spelling dictionary by a JSON file's content and
    /// clears the user's entries, so "load the bundled one, then add the
    /// user's" keeps working.
    pub fn load_spelling_dictionary(&mut self, json: &str) -> bool {
        self.user_spelling.clear();
        self.spelling.load(json)
    }

    pub fn load_spelling_dictionary_file(&mut self, path: &Path) -> bool {
        self.user_spelling.clear();
        self.spelling.load_file(path)
    }

    /// Adds the user's spelling entries, which win over the bundled ones.
    pub fn append_spelling_dictionary(&mut self, json: &str) -> bool {
        self.user_spelling.append(json)
    }

    pub fn add_spelling_entry(&mut self, character: &str, pronunciation: &str) {
        self.user_spelling.add(character, pronunciation);
    }

    pub fn clear_spelling_dictionary(&mut self) {
        self.spelling.clear();
        self.user_spelling.clear();
    }

    /// Replaces the bundled emoji dictionary (Croatian names, used for hr, sr
    /// and bs) by a JSON file's content; the dictionaries of single languages
    /// and the user's entries are cleared.
    pub fn load_emoji_dictionary(&mut self, json: &str) -> bool {
        self.emoji_by_language.clear();
        self.user_emoji.clear();
        self.emoji.load(json)
    }

    /// Loads the bundled emoji dictionary from a file, and with it the
    /// dictionaries of the other languages that lie next to it:
    /// `emoji_sl.json` next to `emoji.json`.
    pub fn load_emoji_dictionary_file(&mut self, path: &Path) -> bool {
        let loaded = std::fs::read_to_string(path).is_ok_and(|json| self.load_emoji_dictionary(&json));
        if !loaded {
            self.emoji_by_language.clear();
            self.user_emoji.clear();
        }
        let stem = path.file_stem().unwrap_or_default().to_string_lossy();
        let extension =
            path.extension().map(|extension| format!(".{}", extension.to_string_lossy())).unwrap_or_default();
        for language in Language::ALL {
            let of_language = path.with_file_name(format!("{stem}_{}{extension}", language.short_code()));
            if let Ok(json) = std::fs::read_to_string(of_language) {
                self.load_language_emoji_dictionary(language, &json);
            }
        }
        loaded
    }

    /// Loads the emoji names of one language, used while it is spoken.
    pub fn load_language_emoji_dictionary(&mut self, language: Language, json: &str) -> bool {
        let mut dictionary = EmojiDictionary::default();
        let loaded = dictionary.load(json);
        if loaded {
            self.emoji_by_language.insert(language, dictionary);
        }
        loaded
    }

    /// Adds the user's emoji entries, which win in every language.
    pub fn append_emoji_dictionary(&mut self, json: &str) -> bool {
        self.user_emoji.append(json)
    }

    pub fn add_emoji_entry(&mut self, emoji: &str, text: &str) {
        self.user_emoji.add(emoji, text);
    }

    pub fn clear_emoji_dictionary(&mut self) {
        self.emoji.clear();
        self.emoji_by_language.clear();
        self.user_emoji.clear();
    }

    /// Loads the user's accent lexicon (the content of `accents.json`). The
    /// entries are kept across voice and language changes. False when none
    /// was accepted; [`accent_lexicon_report`](Self::accent_lexicon_report)
    /// says what was read.
    pub fn load_accent_lexicon(&mut self, json: &str) -> bool {
        let (lexicon, report) = UserLexicon::parse(json);
        self.user_lexicon_report = format!("{} words, {} verbs", report.words, report.verbs);
        if report.rejected > 0 {
            self.user_lexicon_report += &format!("; {} rejected (first: {})", report.rejected, report.first_error);
        }
        let accepted = !lexicon.is_empty();
        self.user_lexicon = accepted.then_some(lexicon);
        self.frontend.set_user_lexicon(self.user_lexicon.clone());
        accepted
    }

    pub fn load_accent_lexicon_file(&mut self, path: &Path) -> bool {
        match std::fs::read_to_string(path) {
            Ok(json) => self.load_accent_lexicon(&json),
            Err(_) => {
                self.user_lexicon_report = format!("cannot open {}", path.display());
                false
            }
        }
    }

    pub fn clear_accent_lexicon(&mut self) {
        self.user_lexicon = None;
        self.user_lexicon_report.clear();
        self.frontend.set_user_lexicon(None);
    }

    /// What the last accent lexicon load read: "12 words, 3 verbs; 1 rejected
    /// (first: ...)".
    pub fn accent_lexicon_report(&self) -> &str {
        &self.user_lexicon_report
    }

    // -------------------------------------------------------------------------
    // The user's configuration
    // -------------------------------------------------------------------------

    /// Applies the user's `settings.json` and, unless the user switched them
    /// off, the user's dictionaries from the configuration directory. False
    /// when a missing settings file could not be created.
    pub fn load_user_config(&mut self) -> bool {
        let mut config = UserConfig::new();
        if !config.load_settings() {
            return false;
        }
        let settings = &config.settings;
        let mut params = VoiceParams { pitch: self.params.pitch, ..settings.to_voice_params() };
        params.clamp();
        self.set_params(&params);

        let file = |name| config.path(name).filter(|path| path.exists());
        if !settings.user_dictionaries_enabled {
            self.clear_accent_lexicon();
            return true;
        }
        if let Some(path) = file(UserConfig::DICTIONARY) {
            self.dictionary.load_file(&path);
        }
        if let Some(json) = file(UserConfig::SPELLING).and_then(|path| std::fs::read_to_string(path).ok()) {
            self.append_spelling_dictionary(&json);
        }
        if let Some(path) = file(UserConfig::EMOJI) {
            self.load_emoji_dictionary_file(&path);
        }
        match file(UserConfig::ACCENTS) {
            Some(path) => drop(self.load_accent_lexicon_file(&path)),
            None => self.clear_accent_lexicon(),
        }
        true
    }
}

/// A screen reader announces a capital letter as a word or two and the
/// letter: NVDA "veliko N" (Croatian, Serbian, Bosnian) and "cap N", TalkBack
/// "veliko slovo N" (Croatian), "велико Н" (Serbian), "veliko N" (Serbian
/// Latin, Bosnian) and "capital N", VoiceOver "veliko početno slovo N"
/// (Croatian) and "cap N". They send it as text, not as a spelled character,
/// so without this the letter is read by its name in every spelling mode, and
/// at the speech rate. Returns the words and the letter.
fn split_capital_announcement(text: &str) -> Option<(String, String)> {
    const FIRST: [&str; 3] = ["veliko", "cap", "capital"];
    const MORE: [&str; 3] = ["slovo", "početno", "letter"];
    let text = text.trim_end_matches([' ', '\t', '\n', '\r', '.']);
    let tokens: Vec<&str> = text.split([' ', '\t', '\n', '\r']).filter(|token| !token.is_empty()).collect();
    let (letter, words) = tokens.split_last().filter(|_| (2..=4).contains(&tokens.len()))?;
    let mut letters = letter.chars();
    if letters.next().is_none_or(|c| spelling_letter(c).is_empty()) || letters.next().is_some() {
        return None;
    }
    // The words lowercased, Cyrillic in Latin letters.
    let plain = |word: &str| {
        let letters: Vec<Vec<char>> = word.chars().map(spelling_letter).collect();
        (!letters.iter().any(Vec::is_empty)).then(|| letters.concat().into_iter().collect::<String>())
    };
    let known = words
        .iter()
        .enumerate()
        .all(|(i, word)| plain(word).is_some_and(|word| if i == 0 { FIRST } else { MORE }.contains(&word.as_str())));
    known.then(|| (words.join(" "), (*letter).to_owned()))
}
