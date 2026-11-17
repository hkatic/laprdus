//! The contract of the C API (`include/laprdus/laprdus_api.h`): handles,
//! null arguments, error codes, the ownership of buffers, the info
//! structures, the stream functions and the strings it hands out.
//!
//! What the engine does with text is tested through the safe API in the
//! root package (`tests/formant.rs`, `tests/recorded.rs`); here are the cases
//! of the earlier test programs that are about the boundary to C itself.
//!
//! Run with `cargo test --release -p laprdus-capi --test c_api`. The cases
//! with a recorded voice need the packed voice files: `LAPRDUS_DATA` names
//! their directory, else `data/voices` of the repository, which
//! `cargo xtask data` fills. Nothing here plays sound.

use laprdus::voice::VOICES;
use laprdus_capi::*;
use std::ffi::{CStr, CString, c_char, c_int, c_void};
use std::path::{Path, PathBuf};
use std::ptr::{null, null_mut};

/// A directory that does not exist: formant voices must not look for files.
const NO_DATA: &str = "/nonexistent/laprdus-data";

/// `LaprdusSpellingMode` and `LaprdusNumberMode` of the header.
const LAPRDUS_SPELLING_LETTER_NAMES: c_int = 0;
const LAPRDUS_NUMBER_MODE_WHOLE: c_int = 0;

/// The directory of `Josip.bin` and `Vlado.bin`.
fn data_dir() -> String {
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
    dir.to_string_lossy().into_owned()
}

fn c(text: &str) -> CString {
    CString::new(text).expect("text without a NUL")
}

/// A string the API handed out.
fn text(pointer: *const c_char) -> String {
    assert!(!pointer.is_null(), "a string, not NULL");
    // SAFETY: the API returns NUL-terminated strings that outlive the call.
    unsafe { CStr::from_ptr(pointer) }.to_str().expect("UTF-8").to_owned()
}

/// A string of an info structure that may be NULL.
fn optional(pointer: *const c_char) -> Option<String> {
    (!pointer.is_null()).then(|| text(pointer))
}

/// True when a is within the absolute tolerance the earlier tests used.
fn approx(a: f32, b: f32) -> bool {
    (f64::from(a) - f64::from(b)).abs() < 0.00001
}

/// An engine, destroyed when it goes out of scope.
struct Engine(LaprdusHandle);

impl Drop for Engine {
    fn drop(&mut self) {
        // SAFETY: the handle came from laprdus_create and is not used again.
        unsafe { laprdus_destroy(self.0) }
    }
}

impl Engine {
    fn new() -> Self {
        // SAFETY: no arguments.
        let handle = unsafe { laprdus_create() };
        assert!(!handle.is_null());
        Self(handle)
    }

    /// An engine with a formant voice.
    fn formant(voice: &str) -> Self {
        let engine = Self::new();
        assert_eq!(engine.set_voice(voice, NO_DATA), LAPRDUS_OK, "{voice}");
        engine
    }

    fn set_voice(&self, voice: &str, directory: &str) -> LaprdusError {
        // SAFETY: a live handle and two C strings.
        unsafe { laprdus_set_voice(self.0, c(voice).as_ptr(), c(directory).as_ptr()) }
    }

    fn voice(&self) -> String {
        // SAFETY: a live handle.
        text(unsafe { laprdus_get_current_voice(self.0) })
    }

    /// `None` is the NULL tag.
    fn set_language(&self, tag: Option<&str>) -> LaprdusError {
        let tag = tag.map(c);
        // SAFETY: a live handle and a C string or NULL.
        unsafe { laprdus_set_language(self.0, tag.as_ref().map_or(null(), |tag| tag.as_ptr())) }
    }

    fn language(&self) -> String {
        // SAFETY: a live handle.
        text(unsafe { laprdus_get_language(self.0) })
    }

    /// Takes over the audio of a synthesis call: checks the format and frees
    /// the buffer.
    fn take(count: i32, samples: *mut i16, format: &LaprdusAudioFormat) -> Vec<i16> {
        let mut audio = Vec::new();
        if count > 0 && !samples.is_null() {
            assert_eq!((format.sample_rate, format.bits_per_sample, format.channels), (22050, 16, 1));
            // SAFETY: the call returned `count` samples at `samples`.
            audio.extend_from_slice(unsafe { std::slice::from_raw_parts(samples, count as usize) });
        }
        // SAFETY: the buffer came from the API (or is NULL) and is freed once.
        unsafe { laprdus_free_buffer(samples) };
        audio
    }

    /// The audio of a text; empty when nothing was synthesized.
    fn speak(&self, text: &str) -> Vec<i16> {
        let mut samples = null_mut();
        let mut format = LaprdusAudioFormat { sample_rate: 0, bits_per_sample: 0, channels: 0 };
        // SAFETY: a live handle, a C string and two valid out pointers.
        let count = unsafe { laprdus_synthesize(self.0, c(text).as_ptr(), &mut samples, &mut format) };
        Self::take(count, samples, &format)
    }

    /// The audio of a text spelled; with `final_pause` false the pause after
    /// the last character is left out. Returns the count the call returned,
    /// the audio and the length of the final pause in ms.
    fn spell(&self, text: &str, final_pause: bool) -> (i32, Vec<i16>, u32) {
        let mut samples = null_mut();
        let mut format = LaprdusAudioFormat { sample_rate: 0, bits_per_sample: 0, channels: 0 };
        let mut pause_ms = 999;
        // SAFETY: a live handle, a C string and three valid out pointers.
        let count = unsafe {
            laprdus_synthesize_spelled_ex(
                self.0,
                c(text).as_ptr(),
                c_int::from(final_pause),
                &mut pause_ms,
                &mut samples,
                &mut format,
            )
        };
        (count, Self::take(count, samples, &format), pause_ms)
    }

    fn load_accent_lexicon_from_memory(&self, json: &str) -> LaprdusError {
        // SAFETY: a live handle and a C string (length 0: NUL-terminated).
        unsafe { laprdus_load_accent_lexicon_from_memory(self.0, c(json).as_ptr(), 0) }
    }

    fn load_accent_lexicon(&self, path: &str) -> LaprdusError {
        // SAFETY: a live handle and a C string.
        unsafe { laprdus_load_accent_lexicon(self.0, c(path).as_ptr()) }
    }

    fn accent_lexicon_report(&self) -> String {
        // SAFETY: a live handle.
        text(unsafe { laprdus_get_accent_lexicon_report(self.0) })
    }

    /// True when a text and its accented spelling sound the same.
    fn same(&self, plain: &str, accented: &str) -> bool {
        self.speak(plain) == self.speak(accented)
    }
}

fn voice_info(index: u32) -> Result<LaprdusVoiceInfo, LaprdusError> {
    // SAFETY: the structure is pointers, integers and a float: all zero is a
    // valid value, and the call fills it.
    let mut info: LaprdusVoiceInfo = unsafe { std::mem::zeroed() };
    // SAFETY: a valid out pointer.
    match unsafe { laprdus_get_voice_info(index, &mut info) } {
        LAPRDUS_OK => Ok(info),
        error => Err(error),
    }
}

/// `None` is the NULL id.
fn voice_info_by_id(id: Option<&str>) -> Result<LaprdusVoiceInfo, LaprdusError> {
    let id = id.map(c);
    // SAFETY: as in `voice_info`.
    let mut info: LaprdusVoiceInfo = unsafe { std::mem::zeroed() };
    // SAFETY: a C string or NULL and a valid out pointer.
    match unsafe { laprdus_get_voice_info_by_id(id.as_ref().map_or(null(), |id| id.as_ptr()), &mut info) } {
        LAPRDUS_OK => Ok(info),
        error => Err(error),
    }
}

fn default_voice() -> String {
    // SAFETY: no arguments.
    text(unsafe { laprdus_get_default_voice() })
}

// =============================================================================
// The C API cases of the command-line test program
// =============================================================================

/// C API creates engine
#[test]
fn c_api_creates_engine() {
    // SAFETY: no arguments; the handle is destroyed once.
    unsafe {
        let engine = laprdus_create();
        assert!(!engine.is_null());
        laprdus_destroy(engine);
    }
}

/// C API reports version
#[test]
fn c_api_reports_version() {
    // SAFETY: no arguments.
    let version = unsafe { laprdus_get_version() };
    assert!(!version.is_null());
    assert!(!text(version).is_empty());
}

/// C API lists voices
#[test]
fn c_api_lists_voices() {
    // SAFETY: no arguments.
    let count = unsafe { laprdus_get_voice_count() };
    assert_eq!(count, 18); // 5 recorded, 3 formant, 10 singing

    let info = voice_info(0).expect("the first voice");
    assert!(!info.id.is_null());
    assert!(!info.display_name.is_null());
    assert!(!info.language_code.is_null());

    assert_eq!(default_voice(), "zvonko");
    // SAFETY: no arguments.
    assert_eq!(unsafe { laprdus_get_language_count() }, 4);
}

/// C API initializes with voice
#[test]
fn c_api_initializes_with_voice() {
    let data = data_dir();
    // Croatian voice (josip), Serbian voice (vlado)
    for voice in ["josip", "vlado"] {
        let engine = Engine::new();
        assert_eq!(engine.set_voice(voice, &data), LAPRDUS_OK, "{voice}");
        // SAFETY: a live handle.
        assert_ne!(unsafe { laprdus_is_initialized(engine.0) }, 0, "{voice}");
    }

    // Old id (djed) loads the renamed voice
    let engine = Engine::new();
    assert_eq!(engine.set_voice("djed", &data), LAPRDUS_OK);
    assert_eq!(engine.voice(), "djedo");

    // A voice speaks the language asked for
    let engine = Engine::new();
    assert_eq!(engine.set_voice("josip", &data), LAPRDUS_OK);
    assert_eq!(engine.language(), "hr-HR");
    assert_eq!(engine.set_language(Some("sr")), LAPRDUS_OK);
    assert_eq!(engine.language(), "sr-RS");
    assert_eq!(engine.set_language(Some("xx")), LAPRDUS_ERROR_INVALID_PARAMETER);
    assert_eq!(engine.language(), "sr-RS");
}

/// C API synthesizes text
#[test]
fn c_api_synthesizes_text() {
    let engine = Engine::new();
    assert_eq!(engine.set_voice("josip", &data_dir()), LAPRDUS_OK);

    let mut samples = null_mut();
    let mut format = LaprdusAudioFormat { sample_rate: 0, bits_per_sample: 0, channels: 0 };
    // SAFETY: a live handle, a C string and two valid out pointers; the
    // buffer is freed once.
    unsafe {
        let num_samples = laprdus_synthesize(engine.0, c"Dobar dan!".as_ptr(), &mut samples, &mut format);

        assert!(num_samples > 0);
        assert!(!samples.is_null());
        assert_eq!(format.sample_rate, 22050);
        assert_eq!(format.bits_per_sample, 16);
        assert_eq!(format.channels, 1);

        laprdus_free_buffer(samples);
    }
}

/// C API sets parameters
#[test]
fn c_api_sets_parameters() {
    let engine = Engine::new();
    engine.set_voice("josip", &data_dir());

    // SAFETY: a live handle and plain values throughout.
    unsafe {
        // Speed
        for speed in [0.5, 1.0, 2.0] {
            assert_eq!(laprdus_set_speed(engine.0, speed), LAPRDUS_OK);
        }
        // Pitch
        for pitch in [0.5, 1.0, 2.0] {
            assert_eq!(laprdus_set_user_pitch(engine.0, pitch), LAPRDUS_OK);
        }
        // Volume
        for volume in [0.0, 0.5, 1.0] {
            assert_eq!(laprdus_set_volume(engine.0, volume), LAPRDUS_OK);
        }
        // Pauses
        assert_eq!(laprdus_set_sentence_pause(engine.0, 100), LAPRDUS_OK);
        assert_eq!(laprdus_set_comma_pause(engine.0, 50), LAPRDUS_OK);
        assert_eq!(laprdus_set_newline_pause(engine.0, 200), LAPRDUS_OK);
        // Number mode
        assert_eq!(laprdus_set_number_mode(engine.0, LAPRDUS_NUMBER_MODE_DIGIT), LAPRDUS_OK);
        assert_eq!(laprdus_get_number_mode(engine.0), LAPRDUS_NUMBER_MODE_DIGIT);

        assert_eq!(laprdus_set_number_mode(engine.0, LAPRDUS_NUMBER_MODE_WHOLE), LAPRDUS_OK);
        assert_eq!(laprdus_get_number_mode(engine.0), LAPRDUS_NUMBER_MODE_WHOLE);
    }
}

/// C API handles errors gracefully
#[test]
fn c_api_handles_errors_gracefully() {
    // NULL handle
    // SAFETY: NULL is what these calls are asked to refuse.
    unsafe {
        assert_eq!(laprdus_set_speed(null_mut(), 1.0), LAPRDUS_ERROR_INVALID_HANDLE);
        assert!(laprdus_synthesize(null_mut(), c"test".as_ptr(), null_mut(), null_mut()) < 0);
    }

    // Uninitialized engine
    let engine = Engine::new();
    let mut samples = null_mut();
    let mut format = LaprdusAudioFormat { sample_rate: 0, bits_per_sample: 0, channels: 0 };
    // SAFETY: a live handle, a C string and two valid out pointers.
    let result = unsafe { laprdus_synthesize(engine.0, c"test".as_ptr(), &mut samples, &mut format) };
    assert!(result < 0);

    // Invalid voice
    let engine = Engine::new();
    assert_ne!(engine.set_voice("nonexistent", "/usr/share/laprdus"), LAPRDUS_OK);
}

// =============================================================================
// The registry as the info structures give it
// =============================================================================

/// Formant voices are registered
#[test]
fn formant_voices_are_registered() {
    for (id, language, lcid) in [("zvonko", "hr-HR", 0x041A), ("stojan", "sr-RS", 0x081A), ("mirsad", "bs-BA", 0x141A)]
    {
        let info = voice_info_by_id(Some(id)).expect(id);
        assert_eq!(text(info.id), id);
        assert_eq!(text(info.language_code), language);
        assert_eq!(info.language_lcid, lcid);
        assert_eq!(text(info.gender), "Male");
        assert_eq!(text(info.age), "Adult");
        assert_eq!(info.base_pitch, 1.0);
        assert!(info.base_voice_id.is_null());
        assert!(info.data_filename.is_null()); // nothing to load
    }
}

/// Existing voices keep their place in the registry
#[test]
fn existing_voices_keep_their_place_in_the_registry() {
    // Platform code and saved settings rely on the first five entries; the
    // order of the rest is part of the 2.0 contract too.
    const ORDER: [(&str, &str, &str); 18] = [
        ("josip", "Josip", "hr-HR"),
        ("vlado", "Vlado", "sr-RS"),
        ("detence", "Detence", "hr-HR"),
        ("baba", "Baba", "hr-HR"),
        ("djedo", "Đedo", "sr-RS"),
        ("zvonko", "Zvonko", "hr-HR"),
        ("stojan", "Stojan", "sr-RS"),
        ("mirsad", "Mirsad", "bs-BA"),
        ("orguljas", "Orguljaš - Vila Velebita", "hr-HR"),
        ("dalmacija_klapa", "Dalmacija Klapa", "hr-HR"),
        ("trubac", "Trubač - Kreće se lađa francuska", "sr-RS"),
        ("harmonikas", "Harmonikaš - Kreće se lađa francuska", "sr-RS"),
        ("sevdalija", "Sevdalija - Kad ja pođoh na Bembašu", "bs-BA"),
        ("sazlija", "Sazlija - Kad ja pođoh na Bembašu", "bs-BA"),
        ("lika_velebit", "Lika Velebit", "hr-HR"),
        ("solunac", "Solunac", "sr-RS"),
        ("sarajevo_bembasa", "Sarajevo Bembaša", "bs-BA"),
        ("becarac", "Bećarac", "hr-HR"),
    ];
    // SAFETY: no arguments.
    assert_eq!(unsafe { laprdus_get_voice_count() }, 18);
    for (i, (id, name, language)) in ORDER.into_iter().enumerate() {
        let info = voice_info(i as u32).expect(id);
        assert_eq!(text(info.id), id);
        assert_eq!(text(info.display_name), name, "{id}");
        assert_eq!(text(info.language_code), language, "{id}");
        // Ids are plain ASCII; names carry no "Laprdus" prefix or language suffix
        assert!(text(info.id).chars().all(|c| c.is_ascii_lowercase() || c == '_'), "{id}");
        assert!(!text(info.display_name).contains("Laprdus"), "{id}");
        assert!(!text(info.display_name).contains('('), "{id}");
    }

    let josip = voice_info_by_id(Some("josip")).expect("josip");
    assert_eq!(text(josip.data_filename), "Josip.bin");
}

/// Every field of every info structure is the registry's, by index and by id.
#[test]
fn voice_info_structures_agree_with_the_registry() {
    for (i, voice) in VOICES.iter().enumerate() {
        for info in [voice_info(i as u32), voice_info_by_id(Some(voice.id))] {
            let info = info.expect(voice.id);
            assert_eq!(text(info.id), voice.id);
            assert_eq!(text(info.display_name), voice.display_name, "{}", voice.id);
            assert_eq!(text(info.language_code), voice.language.code(), "{}", voice.id);
            assert_eq!(info.language_lcid, voice.language.lcid(), "{}", voice.id);
            assert_eq!(text(info.gender), voice.gender.as_str(), "{}", voice.id);
            assert_eq!(text(info.age), voice.age.as_str(), "{}", voice.id);
            assert_eq!(info.base_pitch, voice.base_pitch, "{}", voice.id);
            assert_eq!(optional(info.base_voice_id).as_deref(), voice.base_voice_id, "{}", voice.id);
            assert_eq!(optional(info.data_filename).as_deref(), voice.data_file(), "{}", voice.id);
        }
    }
    assert_eq!(voice_info(VOICES.len() as u32).err(), Some(LAPRDUS_ERROR_INVALID_PARAMETER));
    // SAFETY: a NULL out pointer is what the calls are asked to refuse.
    unsafe {
        assert_eq!(laprdus_get_voice_info(0, null_mut()), LAPRDUS_ERROR_INVALID_PARAMETER);
        assert_eq!(laprdus_get_voice_info_by_id(c"zvonko".as_ptr(), null_mut()), LAPRDUS_ERROR_INVALID_PARAMETER);
    }
}

/// Old voice ids load the renamed voices
#[test]
fn old_voice_ids_load_the_renamed_voices() {
    const ALIASES: [(&str, &str); 7] = [
        ("djed", "djedo"),
        ("dijete", "detence"),
        ("baka", "baba"),
        ("klapa", "dalmacija_klapa"),
        ("pjevac", "lika_velebit"),
        ("pevac", "solunac"),
        ("solist", "sarajevo_bembasa"),
    ];
    let data = data_dir();
    for (old_id, id) in ALIASES {
        let info = voice_info_by_id(Some(old_id)).expect(old_id);
        assert_eq!(text(info.id), id, "{old_id}");
        // The renamed voice loads under the old id and reports the new one
        let engine = Engine::new();
        let directory = if info.data_filename.is_null() { NO_DATA } else { &data };
        assert_eq!(engine.set_voice(old_id, directory), LAPRDUS_OK, "{old_id}");
        assert_eq!(engine.voice(), id, "{old_id}");
    }

    let djedo = voice_info_by_id(Some("djed")).expect("djed");
    assert_eq!(text(djedo.id), "djedo");
    assert_eq!(text(djedo.display_name), "Đedo");
    assert_eq!(text(djedo.base_voice_id), "vlado");
    assert_eq!(text(djedo.gender), "Male");
    assert_eq!(text(djedo.age), "Senior");
    assert!(voice_info_by_id(Some("no_such_voice")).is_err());
    assert!(voice_info_by_id(None).is_err());
}

/// The four languages are listed and Zvonko is the default voice
#[test]
fn the_four_languages_are_listed_and_zvonko_is_the_default_voice() {
    assert_eq!(default_voice(), "zvonko");
    let zvonko = voice_info_by_id(Some(&default_voice())).expect("the default voice");
    assert!(zvonko.data_filename.is_null());

    // SAFETY: no arguments.
    assert_eq!(unsafe { laprdus_get_language_count() }, 4);
    const EXPECTED: [(&str, &str, u16, &str, &str); 4] = [
        ("hr-HR", "hr", 0x041A, "Croatian", "Hrvatski"),
        ("sr-RS", "sr", 0x081A, "Serbian", "Srpski"),
        ("bs-BA", "bs", 0x141A, "Bosnian", "Bosanski"),
        ("sl-SI", "sl", 0x0424, "Slovenian", "Slovenščina"),
    ];
    // SAFETY: all zero is a valid value of the structure (pointers and an integer).
    let mut info: LaprdusLanguageInfo = unsafe { std::mem::zeroed() };
    for (i, (code, short_code, lcid, english, native)) in EXPECTED.into_iter().enumerate() {
        // SAFETY: a valid out pointer.
        assert_eq!(unsafe { laprdus_get_language_info(i as u32, &mut info) }, LAPRDUS_OK, "{code}");
        assert_eq!(text(info.code), code);
        assert_eq!(text(info.short_code), short_code, "{code}");
        assert_eq!(info.lcid, lcid, "{code}");
        assert_eq!(text(info.english_name), english, "{code}");
        assert_eq!(text(info.native_name), native, "{code}");
    }
    // SAFETY: an index past the end and a NULL out pointer are what the call
    // is asked to refuse.
    unsafe {
        assert_eq!(laprdus_get_language_info(4, &mut info), LAPRDUS_ERROR_INVALID_PARAMETER);
        assert_eq!(laprdus_get_language_info(0, null_mut()), LAPRDUS_ERROR_INVALID_PARAMETER);
    }

    // Every voice's native language is one of the four
    // SAFETY: no arguments.
    for i in 0..unsafe { laprdus_get_voice_count() } {
        let code = text(voice_info(i).expect("a voice").language_code);
        assert!(["hr-HR", "sr-RS", "bs-BA", "sl-SI"].contains(&code.as_str()), "{code}");
    }
}

// =============================================================================
// Languages by tag
// =============================================================================

/// The language is chosen apart from the voice: the tags a host may pass
#[test]
fn the_language_is_chosen_by_a_tag() {
    let engine = Engine::formant("zvonko");
    assert_eq!(engine.language(), "hr-HR");
    assert_eq!(engine.set_language(Some("bs")), LAPRDUS_OK);
    let zvonko_bosnian = engine.speak("2000");
    assert!(!zvonko_bosnian.is_empty());

    // Tags in other spellings; an unknown one is refused and changes nothing
    for tag in ["HR", "hr-HR", "hr_HR", "hrv", "Croatian", "hrvatski"] {
        assert_eq!(engine.set_language(Some("sr")), LAPRDUS_OK, "{tag}");
        assert_eq!(engine.set_language(Some(tag)), LAPRDUS_OK, "{tag}");
        assert_eq!(engine.language(), "hr-HR", "{tag}");
    }
    for (tags, code) in [
        (&["SR", "sr-RS", "sr_RS", "srp", "sr-Latn-RS", "Serbian", "srpski"][..], "sr-RS"),
        (&["BS", "bs-BA", "bs_BA", "bos", "Bosnian", "bosanski"][..], "bs-BA"),
        (&["sl", "sl-SI", "SL_si", "slv", "Slovenian", "Slovene", "slovenščina"][..], "sl-SI"),
    ] {
        for tag in tags {
            assert_eq!(engine.set_language(Some(tag)), LAPRDUS_OK, "{tag}");
            assert_eq!(engine.language(), code, "{tag}");
        }
    }
    assert_eq!(engine.set_language(Some("bs")), LAPRDUS_OK);
    assert_eq!(engine.set_language(Some("de")), LAPRDUS_ERROR_INVALID_PARAMETER);
    assert_eq!(engine.set_language(Some("english")), LAPRDUS_ERROR_INVALID_PARAMETER);
    assert_eq!(engine.language(), "bs-BA");
    assert!(engine.speak("2000") == zvonko_bosnian);
    // SAFETY: a NULL handle is what the calls are asked to refuse.
    unsafe {
        assert_eq!(laprdus_set_language(null_mut(), c"hr".as_ptr()), LAPRDUS_ERROR_INVALID_HANDLE);
        assert!(laprdus_get_language(null_mut()).is_null());
    }
}

/// A chosen language stays across voice changes until it is cleared: NULL
/// and the empty tag
#[test]
fn a_null_or_empty_tag_returns_to_the_native_language() {
    let engine = Engine::formant("zvonko");
    assert_eq!(engine.set_language(Some("sr")), LAPRDUS_OK);

    // Mirsad, whose own language is Bosnian, keeps speaking Serbian
    assert_eq!(engine.set_voice("mirsad", NO_DATA), LAPRDUS_OK);
    assert_eq!(engine.voice(), "mirsad");
    assert_eq!(engine.language(), "sr-RS");

    // NULL returns to the voice's native language, now and after later changes
    assert_eq!(engine.set_language(None), LAPRDUS_OK);
    assert_eq!(engine.language(), "bs-BA");
    assert_eq!(engine.set_voice("zvonko", NO_DATA), LAPRDUS_OK);
    assert_eq!(engine.language(), "hr-HR");
    assert_eq!(engine.set_voice("stojan", NO_DATA), LAPRDUS_OK);
    assert_eq!(engine.language(), "sr-RS");

    // An empty tag means the same as NULL
    assert_eq!(engine.set_language(Some("bs")), LAPRDUS_OK);
    assert_eq!(engine.set_language(Some("")), LAPRDUS_OK);
    assert_eq!(engine.language(), "sr-RS");

    // A language chosen before the first voice applies to it
    let fresh = Engine::new();
    assert_eq!(fresh.set_language(Some("bs")), LAPRDUS_OK);
    assert_eq!(fresh.set_voice("zvonko", NO_DATA), LAPRDUS_OK);
    assert_eq!(fresh.language(), "bs-BA");

    // The singing presets follow the language too
    assert_eq!(fresh.set_voice("dalmacija_klapa", NO_DATA), LAPRDUS_OK);
    assert_eq!(fresh.language(), "bs-BA");
    assert_eq!(fresh.set_language(None), LAPRDUS_OK);
    assert_eq!(fresh.language(), "hr-HR");
}

// =============================================================================
// Parameters without a handle, and from the user's settings
// =============================================================================

/// What the getters answer without a handle, and the clamps of the setters
#[test]
fn getters_answer_the_defaults_without_a_handle() {
    let engine = Engine::formant("zvonko");
    // SAFETY: a live or a NULL handle and plain values.
    unsafe {
        assert_eq!(laprdus_get_head_size(engine.0), 50);
        assert_eq!(laprdus_get_breathiness(engine.0), 50);
        assert_eq!(laprdus_get_roughness(engine.0), 50);
        assert_eq!(laprdus_get_brightness(engine.0), 50);
        assert_eq!(laprdus_get_head_size(null_mut()), 50);

        // Out-of-range values are clamped; no handle reads as the normal rate.
        assert_eq!(laprdus_set_speed(engine.0, 9.0), LAPRDUS_OK);
        assert!(approx(laprdus_get_speed(engine.0), 4.0));
        assert!(approx(laprdus_get_speed(null_mut()), 1.0));
        assert!(approx(laprdus_get_user_pitch(null_mut()), 1.0));
    }
}

/// Nominal words per minute are known for every voice
#[test]
fn nominal_words_per_minute_are_known_for_every_voice() {
    // SAFETY: a C string or NULL.
    let wpm = |id: &CStr| unsafe { laprdus_get_nominal_wpm(id.as_ptr()) };
    assert!(approx(wpm(c"zvonko"), 175.0));
    // SAFETY: NULL stands for the default voice.
    assert!(approx(unsafe { laprdus_get_nominal_wpm(null()) }, 175.0));
    assert!(wpm(c"stojan") > wpm(c"mirsad"));
    assert!(wpm(c"josip") > 0.0);
    assert!(wpm(c"djedo") > 0.0);
    assert_eq!(wpm(c"djed"), wpm(c"djedo")); // old id
    assert_eq!(wpm(c"nobody"), 0.0);
}

/// A settings.json of version 1.0 keeps its meaning
#[test]
fn a_settings_json_of_version_1_0_keeps_its_meaning() {
    // Version 1.0 ignored an unforced speed and pitch and had an inflection
    // switch that no settings screen shows any more; reading such a file
    // must not change the rate or leave the voice monotone for good.
    let dir = std::env::temp_dir().join(format!("laprdus_test_settings_{}", std::process::id()));
    let laprdus_dir = dir.join("Laprdus");
    std::fs::create_dir_all(&laprdus_dir).expect("a temporary configuration directory");
    // The variable the configuration directory is found by.
    let variable = if cfg!(windows) { "APPDATA" } else { "XDG_CONFIG_HOME" };
    let saved = std::env::var_os(variable);
    // SAFETY: this is the only test of this program that reads the user's
    // configuration, so no other thread reads or writes this variable while
    // it is changed here.
    unsafe { std::env::set_var(variable, &dir) };

    let load = |json: &str| {
        std::fs::write(laprdus_dir.join("settings.json"), json).expect("settings.json");
        let engine = Engine::formant("zvonko");
        // SAFETY: a live handle.
        assert_eq!(unsafe { laprdus_load_user_config(engine.0) }, LAPRDUS_OK);
        engine
    };
    let values = |engine: &Engine| {
        // SAFETY: a live handle.
        unsafe {
            [laprdus_get_speed(engine.0), laprdus_get_user_pitch(engine.0), laprdus_get_inflection_level(engine.0)]
        }
    };

    let v1 = values(&load(
        r#"{ "version": "1.0",
        "speech": { "speed": 1.6, "pitch": 1.3, "volume": 1, "inflection": false },
        "force": { "speed": false, "pitch": false, "volume": false } }"#,
    ));
    assert!(approx(v1[0], 1.0));
    assert!(approx(v1[1], 1.0));
    // The 1.0 switch controlled another effect; it does not set the level
    assert!(approx(v1[2], 0.5));

    let forced = values(&load(
        r#"{ "version": "1.0",
        "speech": { "speed": 1.6, "pitch": 1.3, "volume": 1, "inflection": true },
        "force": { "speed": true, "pitch": true, "volume": false } }"#,
    ));
    assert!(approx(forced[0], 1.6));
    assert!(approx(forced[1], 1.3));
    assert!(approx(forced[2], 0.5));

    let v2 = values(&load(
        r#"{ "version": "2.0",
        "speech": { "speed": 1.6, "pitch": 1.3, "volume": 1,
                    "inflection_level": 0.7, "acceleration": 1 },
        "force": { "speed": false, "pitch": false, "volume": false } }"#,
    ));
    assert!(approx(v2[0], 1.6));
    assert!(approx(v2[1], 1.3));
    assert!(approx(v2[2], 0.7));

    // The voice quality keys (November 2026): read as integers, clamped to
    // 0-100, 50 when a file lacks them (v2 above has none).
    let engine = load(
        r#"{ "version": "2.0",
            "speech": { "speed": 1, "pitch": 1, "volume": 1, "inflection_level": 0.5,
                        "acceleration": 1, "head_size": 20, "breathiness": 140,
                        "roughness": 0, "brightness": 70.6 } }"#,
    );
    // SAFETY: a live handle.
    unsafe {
        assert_eq!(laprdus_get_head_size(engine.0), 20);
        assert_eq!(laprdus_get_breathiness(engine.0), 100);
        assert_eq!(laprdus_get_roughness(engine.0), 0);
        assert_eq!(laprdus_get_brightness(engine.0), 71);
    }

    // SAFETY: as above.
    unsafe {
        match saved {
            Some(value) => std::env::set_var(variable, value),
            None => std::env::remove_var(variable),
        }
    }
    let _ = std::fs::remove_dir_all(&dir);
}

// =============================================================================
// Spelling
// =============================================================================

/// Spelling mode works with formant voices
#[test]
fn spelling_mode_works_with_formant_voices() {
    let engine = Engine::formant("zvonko");

    let mut samples = null_mut();
    let mut format = LaprdusAudioFormat { sample_rate: 0, bits_per_sample: 0, channels: 0 };
    // SAFETY: a live handle, a C string and two valid out pointers; the
    // buffer is freed once.
    unsafe {
        let count = laprdus_synthesize_spelled(engine.0, c"abc".as_ptr(), &mut samples, &mut format);
        assert!(count > 0);
        assert!(!samples.is_null());
        laprdus_free_buffer(samples);
    }
}

/// A host may leave the final silence of spelled text out and get its length
#[test]
fn spelled_text_may_leave_its_final_pause_to_the_host() {
    let engine = Engine::formant("zvonko");
    // SAFETY: a live handle and plain values.
    unsafe {
        assert_eq!(laprdus_add_spelling_entry(engine.0, c"*".as_ptr(), c"moja zvjezdica".as_ptr()), LAPRDUS_OK);
        assert_eq!(laprdus_set_spelling_pause(engine.0, 0), LAPRDUS_OK);
    }
    for mode in [LAPRDUS_SPELLING_LETTER_NAMES, LAPRDUS_SPELLING_LETTER_SOUNDS] {
        // SAFETY: a live handle and plain values.
        unsafe { assert_eq!(laprdus_set_spelling_mode(engine.0, mode), LAPRDUS_OK) };
        for character in ["b", "Č", "7", "*"] {
            // SAFETY: as above.
            unsafe { assert_eq!(laprdus_set_spelling_speed(engine.0, 100), LAPRDUS_OK) };
            let (count, full, _) = engine.spell(character, true);
            assert!(count > 0, "mode {mode}, character {character}");
            for speed in [50, 20, 0] {
                let context = format!("mode {mode}, character {character}, spelling speed {speed}");
                // SAFETY: as above.
                unsafe { assert_eq!(laprdus_set_spelling_speed(engine.0, speed), LAPRDUS_OK) };
                let (spaced_count, spaced, closed_pause_ms) = engine.spell(character, true);
                assert!(spaced.len() > full.len(), "{context}");
                let added = (spaced.len() - full.len()) as f64;

                let (count, open, final_pause_ms) = engine.spell(character, false);
                assert!(count > 0, "{context}");
                assert!(open == full, "{context}");
                assert_eq!(final_pause_ms, (added * 1000.0 / 22050.0).round() as u32, "{context}");
                assert_eq!(spaced_count as usize, spaced.len(), "{context}");
                assert_eq!(closed_pause_ms, final_pause_ms, "{context}");

                // The length of the pause is optional
                let mut samples = null_mut();
                let mut format = LaprdusAudioFormat { sample_rate: 0, bits_per_sample: 0, channels: 0 };
                // SAFETY: a live handle, a C string, a NULL the call accepts
                // and two valid out pointers; the buffer is freed once.
                unsafe {
                    let count = laprdus_synthesize_spelled_ex(
                        engine.0,
                        c(character).as_ptr(),
                        0,
                        null_mut(),
                        &mut samples,
                        &mut format,
                    );
                    assert_eq!(count as usize, full.len(), "{context}");
                    laprdus_free_buffer(samples);
                }
            }
        }
    }
}

// =============================================================================
// Streams
// =============================================================================

/// Reads a stream to its end in pieces of 1024 samples, as the formant test
/// program did.
fn read_stream(engine: &Engine, text: &str) -> Vec<i16> {
    // SAFETY: a live handle and a C string; the stream is read into a buffer
    // of the size given and destroyed once.
    unsafe {
        let stream = laprdus_stream_begin(engine.0, c(text).as_ptr());
        assert!(!stream.is_null());
        let mut streamed = Vec::new();
        let mut buffer = [0i16; 1024];
        let mut guard = 0;
        while guard < 100_000 && laprdus_stream_is_complete(stream) == 0 {
            let got = laprdus_stream_read(stream, buffer.as_mut_ptr(), 1024);
            if got <= 0 {
                break;
            }
            streamed.extend_from_slice(&buffer[..got as usize]);
            guard += 1;
        }
        laprdus_stream_destroy(stream);
        streamed
    }
}

/// Streaming API works with formant voices
#[test]
fn streaming_api_works_with_formant_voices() {
    let engine = Engine::formant("stojan");

    let whole = engine.speak("Dobar dan. Kako ste?");
    assert!(!whole.is_empty());
    assert!(read_stream(&engine, "Dobar dan. Kako ste?") == whole);
}

/// A capital letter announced by a screen reader is spelled: the streaming
/// API gives the same samples
#[test]
fn a_capital_letter_announcement_streams_the_same_samples() {
    let engine = Engine::formant("zvonko");
    // SAFETY: a live handle and a plain value.
    unsafe { assert_eq!(laprdus_set_spelling_mode(engine.0, LAPRDUS_SPELLING_LETTER_SOUNDS), LAPRDUS_OK) };
    let streamed = read_stream(&engine, "veliko N");
    assert!(!streamed.is_empty());
    assert!(streamed == engine.speak("veliko N"));
}

/// What a callback of `laprdus_synthesize_streaming` is given, and when it
/// asks to stop.
struct Heard {
    audio: Vec<i16>,
    calls: usize,
    stop_after: usize,
}

unsafe extern "system" fn hear(samples: *const i16, count: usize, user_data: *mut c_void) -> c_int {
    // SAFETY: `user_data` is the `Heard` the test passed, and the samples are
    // valid during the call.
    let heard = unsafe { &mut *user_data.cast::<Heard>() };
    heard.audio.extend_from_slice(unsafe { std::slice::from_raw_parts(samples, count) });
    heard.calls += 1;
    c_int::from(heard.calls >= heard.stop_after)
}

/// The callback synthesis hands over the same audio clause by clause, and
/// stops when the callback or laprdus_cancel says so
#[test]
fn the_callback_synthesis_streams_and_can_be_stopped() {
    let engine = Engine::formant("zvonko");
    let text = "Prva rečenica. Druga rečenica. Treća rečenica.";
    let whole = engine.speak(text);
    assert!(!whole.is_empty());
    let stream = |heard: &mut Heard, text: &str, chunk_ms: u32| {
        // SAFETY: a live handle, a C string, and a callback with its data,
        // which outlives the call.
        unsafe {
            laprdus_synthesize_streaming(
                engine.0,
                c(text).as_ptr(),
                chunk_ms,
                Some(hear),
                std::ptr::from_mut(heard).cast(),
            )
        }
    };

    for chunk_ms in [0, 46] {
        let mut heard = Heard { audio: Vec::new(), calls: 0, stop_after: usize::MAX };
        assert_eq!(stream(&mut heard, text, chunk_ms), whole.len() as i32);
        assert!(heard.audio == whole, "chunks of {chunk_ms} ms");
        assert!(heard.calls >= 3);
    }

    // The callback stops it after the first clause
    let mut heard = Heard { audio: Vec::new(), calls: 0, stop_after: 1 };
    assert_eq!(stream(&mut heard, text, 0), LAPRDUS_ERROR_CANCELLED);
    assert_eq!(heard.calls, 1);
    assert!(whole.starts_with(&heard.audio) && heard.audio.len() < whole.len());

    // No callback, no handle
    // SAFETY: a NULL callback and a NULL handle are refused.
    unsafe {
        let text = c(text);
        assert_eq!(
            laprdus_synthesize_streaming(engine.0, text.as_ptr(), 0, None, null_mut()),
            LAPRDUS_ERROR_INVALID_PARAMETER
        );
        assert_eq!(
            laprdus_synthesize_streaming(null_mut(), text.as_ptr(), 0, Some(hear), null_mut()),
            LAPRDUS_ERROR_INVALID_HANDLE
        );
        laprdus_cancel(null_mut());
    }

    // laprdus_cancel from another thread, in the middle of a long text
    let long = "Ovo je jedna duga rečenica bez kraja i konca koja se ponavlja ".repeat(300);
    let finished = std::sync::Arc::new(std::sync::atomic::AtomicBool::new(false));
    let stopper = {
        let (handle, finished) = (engine.0 as usize, finished.clone());
        std::thread::spawn(move || {
            while !finished.load(std::sync::atomic::Ordering::Relaxed) {
                // SAFETY: the handle is alive until the thread is joined;
                // laprdus_cancel may be called from another thread.
                unsafe { laprdus_cancel(handle as LaprdusHandle) };
                std::thread::sleep(std::time::Duration::from_millis(1));
            }
        })
    };
    let mut samples = null_mut();
    let mut format = LaprdusAudioFormat { sample_rate: 0, bits_per_sample: 0, channels: 0 };
    // SAFETY: a live handle, a C string and two valid out pointers.
    let count = unsafe { laprdus_synthesize(engine.0, c(&long).as_ptr(), &mut samples, &mut format) };
    finished.store(true, std::sync::atomic::Ordering::Relaxed);
    stopper.join().expect("the stopping thread");
    assert_eq!(count, LAPRDUS_ERROR_CANCELLED);
    assert!(samples.is_null());

    // A stop before a synthesis stops nothing, and the next one is whole
    // SAFETY: a live handle.
    unsafe { laprdus_cancel(engine.0) };
    assert!(engine.speak(text) == whole);
}

/// Streaming gives the same audio as one buffer
#[test]
fn streaming_gives_the_same_audio_as_one_buffer() {
    let engine = Engine::new();
    assert_eq!(engine.set_voice("josip", &data_dir()), LAPRDUS_OK);
    let text = "Prva rečenica. Druga, s zarezom, rečenica!";
    let whole = engine.speak(text);
    assert!(!whole.is_empty());

    let mut streamed = Vec::new();
    // SAFETY: a live handle and a C string; the stream is read into a buffer
    // of the size given and destroyed once.
    unsafe {
        let stream = laprdus_stream_begin(engine.0, c(text).as_ptr());
        assert!(!stream.is_null());
        let mut chunk = [0i16; 1000];
        loop {
            let got = laprdus_stream_read(stream, chunk.as_mut_ptr(), 1000);
            assert!(got >= 0);
            if got == 0 {
                break;
            }
            streamed.extend_from_slice(&chunk[..got as usize]);
        }
        assert_ne!(laprdus_stream_is_complete(stream), 0);
        laprdus_stream_destroy(stream);
    }
    assert!(streamed == whole);
}

// =============================================================================
// Voices
// =============================================================================

/// Switches between formant and concatenative voices
#[test]
fn switches_between_formant_and_concatenative_voices() {
    let data = data_dir();
    let engine = Engine::new();
    assert_eq!(engine.set_voice("josip", &data), LAPRDUS_OK);
    let josip = engine.speak("Dobar dan");
    assert!(!josip.is_empty());

    assert_eq!(engine.set_voice("zvonko", &data), LAPRDUS_OK);
    let zvonko = engine.speak("Dobar dan");
    assert!(!zvonko.is_empty());
    assert!(zvonko != josip);

    // Back again: the recorded voice must be fully restored.
    assert_eq!(engine.set_voice("josip", &data), LAPRDUS_OK);
    assert!(engine.speak("Dobar dan") == josip);

    // A missing data file fails for recorded voices only.
    assert_ne!(engine.set_voice("vlado", NO_DATA), LAPRDUS_OK);
}

/// Unknown voice is rejected
#[test]
fn unknown_voice_is_rejected() {
    let engine = Engine::new();
    assert_ne!(engine.set_voice("zvonimir", NO_DATA), LAPRDUS_OK);
    // SAFETY: a live handle.
    assert_eq!(unsafe { laprdus_is_initialized(engine.0) }, 0);
}

// =============================================================================
// The accent lexicon
// =============================================================================

/// Checks accent lexicon JSON (`None`: NULL) with a message buffer of `size`
/// bytes; returns the number of rejected entries and the message.
fn check_accent_lexicon(json: Option<&str>, size: usize) -> (c_int, String) {
    let json = json.map(c);
    // Filled, so that a message that is not terminated shows.
    let mut message = [b'#' as c_char; 256];
    // SAFETY: a C string or NULL, and a buffer at least `size` bytes long.
    let rejected = unsafe {
        laprdus_check_accent_lexicon(json.as_ref().map_or(null(), |json| json.as_ptr()), 0, message.as_mut_ptr(), size)
    };
    message[255] = 0;
    (rejected, text(message.as_ptr()))
}

/// Accent lexicon entries can be checked without an engine
#[test]
fn accent_lexicon_entries_can_be_checked_without_an_engine() {
    // What the dictionary editors call before saving an entry: the same
    // parser as the load, with the reason of the first rejection.
    let (rejected, message) = check_accent_lexicon(Some(r#"{"entries":[{"word":"kontr'o:l*"}]}"#), 256);
    assert_eq!(rejected, 0);
    assert!(message.is_empty());
    let (rejected, _) =
        check_accent_lexicon(Some(r#"{"entries":[{"verb":"ur'e:d=i<p"},{"verb":"dijel:i","language":"hr"}]}"#), 256);
    assert_eq!(rejected, 0);
    // SAFETY: a C string and no message buffer, which the call accepts.
    assert_eq!(
        unsafe {
            laprdus_check_accent_lexicon(
                c(r#"{"entries":[{"word":"sign'a:l|a|u|om|e|i|ima"}]}"#).as_ptr(),
                0,
                null_mut(),
                0,
            )
        },
        0
    );

    // No stress mark at all
    let (rejected, message) = check_accent_lexicon(Some(r#"{"entries":[{"word":"kontrola"}]}"#), 256);
    assert_eq!(rejected, 1);
    assert!(message.starts_with("kontrola"), "{message}");
    // Unknown language, two bad entries counted
    let (rejected, message) =
        check_accent_lexicon(Some(r#"{"entries":[{"word":"k'ontrola","language":"de"},{"verb":"uredi"}]}"#), 256);
    assert_eq!(rejected, 2);
    assert!(message.contains("language"), "{message}");
    // Neither word nor verb
    let (rejected, _) = check_accent_lexicon(Some(r#"{"entries":[{"comment":"x"}]}"#), 256);
    assert_eq!(rejected, 1);
    // A buffer too small for the reason is cut, never overrun, and stays a string
    let (rejected, message) = check_accent_lexicon(Some(r#"{"entries":[{"word":"kontrola"}]}"#), 4);
    assert_eq!(rejected, 1);
    assert_eq!(message.len(), 3);
    let (rejected, message) = check_accent_lexicon(None, 256);
    assert_eq!(rejected, -1);
    assert!(message.is_empty());
}

/// The user's accent lexicon moves the stress of words and verbs
#[test]
fn the_users_accent_lexicon_moves_the_stress_of_words_and_verbs() {
    // Words the built-in lexicon does not know get the first syllable.
    let engine = Engine::formant("zvonko");
    assert!(!engine.same("balkon", "balkón"));
    assert!(!engine.same("balzama", "balzáma"));
    assert!(!engine.same("zatrubim", "zatrùbim"));

    let json = concat!(
        "{ \"version\": \"1.0\", \"entries\": [\n",
        "  { \"word\": \"balk'o:n*\", \"comment\": \"balk\\u00f3n, balk\\u00f3na\" },\n",
        "  { \"word\": \"balz'a:m|a|u|om\" },\n",
        "  { \"word\": \"kami'o:n*\", \"language\": \"sr\" },\n",
        "  { \"verb\": \"zatr'ub=i\" },\n",
        "  { \"word\": \"kabina\" },\n",
        "  { \"verb\": \"xyz\" }\n",
        "] }"
    );
    assert_eq!(engine.load_accent_lexicon_from_memory(json), LAPRDUS_OK);
    let report = engine.accent_lexicon_report();
    // Two stems and three paradigm forms, one verb; the two malformed
    // entries are reported.
    assert!(report.starts_with("5 words, 1 verbs; 2 rejected (first: kabina: no stress or length mark)"), "{report}");

    // A stem covers the inflected forms, a paradigm only the forms listed.
    assert!(engine.same("balkon", "balkón"));
    assert!(engine.same("balkona", "balkóna"));
    assert!(engine.same("balzama", "balzáma"));
    assert!(engine.same("balzamom", "balzámom"));
    assert!(!engine.same("balzam", "balzám"));
    // A verb stem reaches every form the built-in verbs do.
    assert!(engine.same("zatrubiti", "zatrùbiti"));
    assert!(engine.same("zatrubim", "zatrùbim"));
    assert!(engine.same("zatrubio", "zatrùbio"));
    // An entry for Serbian only leaves Zvonko alone.
    assert!(!engine.same("kamion", "kamión"));
    // Built-in entries, suffix rules and accent marks in the text still apply.
    assert!(engine.same("kontrola", "kontróla"));
    assert!(engine.same("organizacija", "organizácija"));
    assert!(!engine.same("bàlkon", "balkón"));

    // The lexicon survives a voice change, and the Serbian voice gets its entry.
    assert_eq!(engine.set_voice("stojan", NO_DATA), LAPRDUS_OK);
    assert!(engine.same("balkon", "balkón"));
    assert!(engine.same("kamion", "kamión"));
    assert_eq!(engine.set_voice("zvonko", NO_DATA), LAPRDUS_OK);
    assert!(engine.same("balkon", "balkón"));

    // Clearing restores the defaults; so does a file with nothing usable.
    // SAFETY: a live handle.
    unsafe { laprdus_clear_accent_lexicon(engine.0) };
    assert!(!engine.same("balkon", "balkón"));
    assert!(engine.accent_lexicon_report().is_empty());
    assert_eq!(engine.load_accent_lexicon_from_memory(json), LAPRDUS_OK);
    assert!(engine.same("balkon", "balkón"));
    assert_eq!(
        engine.load_accent_lexicon_from_memory("{ \"entries\": [ { \"word\": \"\" } ] }"),
        LAPRDUS_ERROR_LOAD_FAILED
    );
    assert!(!engine.same("balkon", "balkón"));
    assert_eq!(engine.load_accent_lexicon_from_memory("not json at all"), LAPRDUS_ERROR_LOAD_FAILED);
    assert_eq!(engine.load_accent_lexicon("/nonexistent/accents.json"), LAPRDUS_ERROR_LOAD_FAILED);
}

/// The user's accent lexicon is read from a file and reports bad entries
#[test]
fn the_users_accent_lexicon_is_read_from_a_file_and_reports_bad_entries() {
    let path = std::env::temp_dir().join(format!("laprdus_test_accents_{}.json", std::process::id()));
    std::fs::write(
        &path,
        concat!(
            "{ \"entries\": [ { \"word\": \"balk'o:n*\" }, { \"word\": \"bal'k'on\" }, ",
            "{ \"word\": \"balkon:\" }, { \"verb\": \"zam'ol=q\" }, ",
            "{ \"word\": \"x'y\", \"language\": \"de\" } ] }"
        ),
    )
    .expect("the lexicon file");
    let engine = Engine::formant("mirsad");
    assert_eq!(engine.load_accent_lexicon(&path.to_string_lossy()), LAPRDUS_OK);
    let report = engine.accent_lexicon_report();
    assert!(report.starts_with("1 words, 0 verbs; 4 rejected (first: bal'k'on: two stress marks)"), "{report}");
    assert!(engine.same("balkon", "balkón"));
    let _ = std::fs::remove_file(&path);
}
