//! The C API of the engine, declared in `include/laprdus/laprdus_api.h`.
//!
//! Every function checks its pointers, catches a panic and turns it into the
//! function's error value, so nothing the engine does can take a host (a
//! screen reader) down with it. Pointers given to these functions must be
//! null or valid as the header says; a handle must not be used from two
//! threads at once, except by `laprdus_cancel`, which is there to be called
//! from another thread while the handle's thread synthesizes.

use laprdus::config::UserConfig;
use laprdus::engine::{CancelHandle, Engine, Error};
use laprdus::language::Language;
use laprdus::params::{
    ACCELERATION_DEFAULT, INFLECTION_LEVEL_DEFAULT, NumberMode, SPELLING_SPEED_DEFAULT, SPELLING_SPEED_MAX,
    SPELLING_SPEED_MIN, SpellingMode, VOICE_QUALITY_DEFAULT, VOICE_QUALITY_MAX, VOICE_QUALITY_MIN, VoiceParams,
};
use laprdus::text::frontend::UserLexicon;
use laprdus::voice::{self, VOICES, Voice};
use std::cell::RefCell;
use std::collections::HashMap;
use std::ffi::{CStr, CString, c_char, c_int, c_void};
use std::ops::ControlFlow;
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::path::PathBuf;
use std::sync::{LazyLock, Mutex, PoisonError};

pub type LaprdusError = i32;
pub const LAPRDUS_OK: LaprdusError = 0;
pub const LAPRDUS_ERROR_INVALID_HANDLE: LaprdusError = -1;
pub const LAPRDUS_ERROR_NOT_INITIALIZED: LaprdusError = -2;
pub const LAPRDUS_ERROR_INVALID_PATH: LaprdusError = -3;
pub const LAPRDUS_ERROR_LOAD_FAILED: LaprdusError = -4;
pub const LAPRDUS_ERROR_SYNTHESIS_FAILED: LaprdusError = -5;
pub const LAPRDUS_ERROR_OUT_OF_MEMORY: LaprdusError = -6;
pub const LAPRDUS_ERROR_CANCELLED: LaprdusError = -7;
pub const LAPRDUS_ERROR_INVALID_PARAMETER: LaprdusError = -8;
pub const LAPRDUS_ERROR_DECRYPTION_FAILED: LaprdusError = -9;
pub const LAPRDUS_ERROR_FILE_NOT_FOUND: LaprdusError = -10;
pub const LAPRDUS_ERROR_INVALID_FORMAT: LaprdusError = -11;

/// `LaprdusSpellingMode`: 0 letter names, 1 letter sounds.
pub const LAPRDUS_SPELLING_LETTER_SOUNDS: c_int = 1;
/// `LaprdusNumberMode`: 0 whole numbers, 1 digit by digit.
pub const LAPRDUS_NUMBER_MODE_DIGIT: c_int = 1;

#[repr(C)]
pub struct LaprdusAudioFormat {
    pub sample_rate: u32,
    pub bits_per_sample: u16,
    pub channels: u16,
}

#[repr(C)]
pub struct LaprdusVoiceParams {
    pub speed: f32,
    pub pitch: f32,
    pub volume: f32,
}

#[repr(C)]
pub struct LaprdusVoiceInfo {
    pub id: *const c_char,
    pub display_name: *const c_char,
    pub language_code: *const c_char,
    pub language_lcid: u16,
    pub gender: *const c_char,
    pub age: *const c_char,
    pub base_pitch: f32,
    pub base_voice_id: *const c_char,
    pub data_filename: *const c_char,
}

#[repr(C)]
pub struct LaprdusLanguageInfo {
    pub code: *const c_char,
    pub short_code: *const c_char,
    pub lcid: u16,
    pub english_name: *const c_char,
    pub native_name: *const c_char,
}

/// An engine and the message of its last error.
pub struct LaprdusEngine {
    engine: Engine,
    last_error: String,
}

/// The audio of one synthesis, read in pieces.
pub struct LaprdusStream {
    audio: Vec<i16>,
    position: usize,
    complete: bool,
}

pub type LaprdusHandle = *mut LaprdusEngine;
pub type LaprdusStreamHandle = *mut LaprdusStream;

/// Receives the audio of `laprdus_synthesize_streaming`; a non-zero result
/// stops the synthesis.
pub type LaprdusAudioCallback =
    Option<unsafe extern "system" fn(samples: *const i16, count: usize, user_data: *mut c_void) -> c_int>;

/// The cancel handles of the engines alive, by the address of their handle.
/// `laprdus_cancel` is called while another thread is inside a synthesis with
/// the engine borrowed, so it must not touch the engine itself: it finds the
/// engine's cancel handle here.
static CANCEL_HANDLES: LazyLock<Mutex<HashMap<usize, CancelHandle>>> = LazyLock::new(Mutex::default);

fn cancel_handles() -> std::sync::MutexGuard<'static, HashMap<usize, CancelHandle>> {
    CANCEL_HANDLES.lock().unwrap_or_else(PoisonError::into_inner)
}

// -----------------------------------------------------------------------------
// The boundary: pointers in, pointers out
// -----------------------------------------------------------------------------

/// Defines an exported function whose body runs inside `catch_unwind`; a
/// panic gives the value after `=`.
macro_rules! api {
    ($(fn $name:ident($($arg:ident: $ty:ty),* $(,)?) $(-> $ret:ty)? = $on_panic:expr => $body:block)*) => {$(
        #[unsafe(no_mangle)]
        #[allow(unused_unsafe, clippy::not_unsafe_ptr_arg_deref)]
        pub unsafe extern "system" fn $name($($arg: $ty),*) $(-> $ret)? {
            // SAFETY: the pointers are null or valid, as the header requires
            // of the caller; every one is checked for null before use.
            catch_unwind(AssertUnwindSafe(|| unsafe { $body })).unwrap_or($on_panic)
        }
    )*};
}

/// A static string as a C string that lives as long as the program.
fn c_str(text: &'static str) -> *const c_char {
    static STRINGS: LazyLock<Mutex<HashMap<&'static str, CString>>> = LazyLock::new(Mutex::default);
    let mut strings = STRINGS.lock().unwrap_or_else(PoisonError::into_inner);
    strings.entry(text).or_insert_with(|| CString::new(text).unwrap_or_default()).as_ptr()
}

/// A changing string as a C string that lives until the thread asks for the
/// next one of its kind, so a caller can use it without racing another
/// thread's call.
fn held(slot: &'static std::thread::LocalKey<RefCell<CString>>, text: &str) -> *const c_char {
    slot.with_borrow_mut(|held| {
        *held = CString::new(text.replace('\0', " ")).unwrap_or_default();
        held.as_ptr()
    })
}

unsafe fn string(text: *const c_char) -> Option<String> {
    (!text.is_null()).then(|| unsafe { CStr::from_ptr(text) }.to_string_lossy().into_owned())
}

unsafe fn path(text: *const c_char) -> Option<PathBuf> {
    unsafe { string(text) }.map(PathBuf::from)
}

/// JSON given with its length, or as a C string when the length is 0.
unsafe fn json(content: *const c_char, length: usize) -> Option<String> {
    match length {
        _ if content.is_null() => None,
        0 => unsafe { string(content) },
        _ => Some(String::from_utf8_lossy(unsafe { std::slice::from_raw_parts(content.cast(), length) }).into_owned()),
    }
}

unsafe fn bytes<'a>(data: *const u8, size: usize) -> &'a [u8] {
    if data.is_null() || size == 0 { &[] } else { unsafe { std::slice::from_raw_parts(data, size) } }
}

unsafe fn write<T>(out: *mut T, value: T) {
    if !out.is_null() {
        unsafe { out.write(value) };
    }
}

/// Copies text into a caller's buffer with a terminating NUL, cut to fit at a
/// character boundary. Returns the length of the whole text.
unsafe fn copy_out(text: &str, buffer: *mut c_char, buffer_size: usize) -> usize {
    if !buffer.is_null() && buffer_size > 0 {
        let mut n = text.len().min(buffer_size - 1);
        while !text.is_char_boundary(n) {
            n -= 1;
        }
        unsafe {
            std::ptr::copy_nonoverlapping(text.as_ptr(), buffer.cast(), n);
            buffer.add(n).write(0);
        }
    }
    text.len()
}

fn format() -> LaprdusAudioFormat {
    LaprdusAudioFormat { sample_rate: laprdus::SAMPLE_RATE, bits_per_sample: 16, channels: 1 }
}

// Audio handed to the caller is freed by `laprdus_free_buffer`, which gets
// the pointer alone: the length is kept in front of the samples.
const LENGTH_SLOTS: usize = size_of::<u64>() / size_of::<i16>();

fn export(samples: &[i16]) -> *mut i16 {
    let length = (samples.len() as u64).to_ne_bytes();
    let mut buffer = Vec::with_capacity(samples.len() + LENGTH_SLOTS);
    buffer.extend(length.chunks_exact(2).map(|pair| i16::from_ne_bytes([pair[0], pair[1]])));
    buffer.extend_from_slice(samples);
    let whole = Box::into_raw(buffer.into_boxed_slice()).cast::<i16>();
    // SAFETY: the allocation is longer than the length slots.
    unsafe { whole.add(LENGTH_SLOTS) }
}

unsafe fn release(buffer: *mut i16) {
    if buffer.is_null() {
        return;
    }
    // SAFETY: `buffer` came from `export`, which put the length in front.
    unsafe {
        let whole = buffer.sub(LENGTH_SLOTS);
        let length = whole.cast::<u64>().read_unaligned() as usize;
        drop(Box::from_raw(std::ptr::slice_from_raw_parts_mut(whole, length + LENGTH_SLOTS)));
    }
}

impl LaprdusEngine {
    fn fail(&mut self, code: LaprdusError, message: impl Into<String>) -> LaprdusError {
        self.last_error = message.into();
        code
    }

    /// The code of a synthesis that did not finish, with its message.
    fn failed(&mut self, error: &Error) -> LaprdusError {
        let code =
            if matches!(error, Error::Cancelled) { LAPRDUS_ERROR_CANCELLED } else { LAPRDUS_ERROR_SYNTHESIS_FAILED };
        self.fail(code, error.to_string())
    }

    /// `LAPRDUS_OK` when `done`, else the error with its message.
    fn check(&mut self, done: bool, code: LaprdusError, message: &str) -> LaprdusError {
        if done { LAPRDUS_OK } else { self.fail(code, message) }
    }

    fn change(&mut self, change: impl FnOnce(&mut VoiceParams)) -> LaprdusError {
        self.engine.update_params(change);
        LAPRDUS_OK
    }

    /// Hands synthesized audio to the caller: the number of samples, or the
    /// error code as a negative number.
    unsafe fn deliver(
        &mut self,
        audio: Result<Vec<i16>, Error>,
        out_samples: *mut *mut i16,
        out_format: *mut LaprdusAudioFormat,
    ) -> i32 {
        match audio {
            Err(error) => self.failed(&error),
            Ok(audio) if audio.len() > i32::MAX as usize => {
                self.fail(LAPRDUS_ERROR_OUT_OF_MEMORY, "Audio data too large")
            }
            Ok(audio) => unsafe {
                write(out_samples, if audio.is_empty() { std::ptr::null_mut() } else { export(&audio) });
                write(out_format, format());
                audio.len() as i32
            },
        }
    }

    /// The checks every synthesis call makes before it starts.
    unsafe fn text_to_speak(
        &mut self,
        text: *const c_char,
        out_samples: *mut *mut i16,
    ) -> Result<String, LaprdusError> {
        if !self.engine.is_initialized() {
            return Err(self.fail(LAPRDUS_ERROR_NOT_INITIALIZED, "Engine not initialized"));
        }
        let Some(text) = (unsafe { string(text) }) else {
            return Err(self.fail(LAPRDUS_ERROR_INVALID_PARAMETER, "Text is NULL"));
        };
        if out_samples.is_null() {
            return Err(self.fail(LAPRDUS_ERROR_INVALID_PARAMETER, "Output samples pointer is NULL"));
        }
        Ok(text)
    }
}

unsafe fn voice_info(voice: &'static Voice, out: *mut LaprdusVoiceInfo) {
    let optional = |text: Option<&'static str>| text.map_or(std::ptr::null(), c_str);
    let info = LaprdusVoiceInfo {
        id: c_str(voice.id),
        display_name: c_str(voice.display_name),
        language_code: c_str(voice.language.code()),
        language_lcid: voice.language.lcid(),
        gender: c_str(voice.gender.as_str()),
        age: c_str(voice.age.as_str()),
        base_pitch: voice.base_pitch,
        base_voice_id: optional(voice.base_voice_id),
        data_filename: optional(voice.data_file()),
    };
    unsafe { out.write(info) };
}

fn error_text(error: LaprdusError) -> &'static str {
    match error {
        LAPRDUS_OK => "OK",
        LAPRDUS_ERROR_INVALID_HANDLE => "Invalid handle",
        LAPRDUS_ERROR_NOT_INITIALIZED => "Engine not initialized",
        LAPRDUS_ERROR_INVALID_PATH => "Invalid path",
        LAPRDUS_ERROR_LOAD_FAILED => "Failed to load data",
        LAPRDUS_ERROR_SYNTHESIS_FAILED => "Synthesis failed",
        LAPRDUS_ERROR_OUT_OF_MEMORY => "Out of memory",
        LAPRDUS_ERROR_CANCELLED => "Operation cancelled",
        LAPRDUS_ERROR_INVALID_PARAMETER => "Invalid parameter",
        LAPRDUS_ERROR_DECRYPTION_FAILED => "Decryption failed",
        LAPRDUS_ERROR_FILE_NOT_FOUND => "File not found",
        LAPRDUS_ERROR_INVALID_FORMAT => "Invalid file format",
        _ => "Unknown error",
    }
}

thread_local! {
    static ERROR_MESSAGE: RefCell<CString> = RefCell::default();
    static LEXICON_REPORT: RefCell<CString> = RefCell::default();
}

/// Runs `$body` with the engine behind a handle, or returns `$invalid` for a
/// null handle.
macro_rules! with {
    ($handle:expr, $invalid:expr, |$engine:ident| $body:expr) => {
        match $handle.as_mut() {
            Some($engine) => $body,
            None => $invalid,
        }
    };
}

/// A setter of one voice parameter.
macro_rules! set {
    ($handle:expr, |$params:ident| $change:expr) => {
        with!($handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| h.change(|$params| $change))
    };
}

/// A getter of one voice parameter, with its value for a null handle.
macro_rules! get {
    ($handle:expr, $default:expr, |$params:ident| $value:expr) => {
        with!($handle, $default, |h| {
            let $params = h.engine.params();
            $value
        })
    };
}

// -----------------------------------------------------------------------------
// The functions, in the order of the header
// -----------------------------------------------------------------------------

api! {
    fn laprdus_create() -> LaprdusHandle = std::ptr::null_mut() => {
        let engine = Engine::new();
        let cancel = engine.cancel_handle();
        let handle = Box::into_raw(Box::new(LaprdusEngine { engine, last_error: String::new() }));
        cancel_handles().insert(handle as usize, cancel);
        handle
    }

    fn laprdus_destroy(handle: LaprdusHandle) = () => {
        if !handle.is_null() {
            cancel_handles().remove(&(handle as usize));
            drop(Box::from_raw(handle));
        }
    }

    fn laprdus_init_from_file(handle: LaprdusHandle, phoneme_data_path: *const c_char, decryption_key: *const u8, key_size: usize) -> LaprdusError
    = LAPRDUS_ERROR_LOAD_FAILED => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| match path(phoneme_data_path) {
            None => h.fail(LAPRDUS_ERROR_INVALID_PATH, "Phoneme data path is NULL"),
            Some(path) => {
                let loaded = h.engine.load_recordings(&path, bytes(decryption_key, key_size)).is_ok();
                h.check(loaded, LAPRDUS_ERROR_LOAD_FAILED, "Failed to load phoneme data from file")
            }
        })
    }

    fn laprdus_init_from_memory(handle: LaprdusHandle, data: *const u8, data_size: usize, decryption_key: *const u8, key_size: usize) -> LaprdusError
    = LAPRDUS_ERROR_LOAD_FAILED => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| {
            if data.is_null() || data_size == 0 {
                return h.fail(LAPRDUS_ERROR_INVALID_PARAMETER, "Invalid data pointer or size");
            }
            let loaded = h.engine.load_recordings_from_memory(bytes(data, data_size), bytes(decryption_key, key_size)).is_ok();
            h.check(loaded, LAPRDUS_ERROR_LOAD_FAILED, "Failed to load phoneme data from memory")
        })
    }

    fn laprdus_init_from_directory(handle: LaprdusHandle, phoneme_dir: *const c_char) -> LaprdusError = LAPRDUS_ERROR_LOAD_FAILED => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| match path(phoneme_dir) {
            None => h.fail(LAPRDUS_ERROR_INVALID_PATH, "Phoneme directory is NULL"),
            Some(path) => {
                let loaded = h.engine.load_recordings(&path, &[]).is_ok();
                h.check(loaded, LAPRDUS_ERROR_LOAD_FAILED, "Failed to load phoneme data from directory")
            }
        })
    }

    fn laprdus_is_initialized(handle: LaprdusHandle) -> c_int = 0 => {
        with!(handle, 0, |h| c_int::from(h.engine.is_initialized()))
    }

    // Voice parameters

    fn laprdus_set_voice_params(handle: LaprdusHandle, params: *const LaprdusVoiceParams) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| match params.as_ref() {
            None => h.fail(LAPRDUS_ERROR_INVALID_PARAMETER, "Voice params is NULL"),
            // Everything the structure does not carry goes back to its default.
            Some(p) => {
                h.engine.set_params(&VoiceParams { speed: p.speed, pitch: p.pitch, volume: p.volume, ..VoiceParams::default() });
                LAPRDUS_OK
            }
        })
    }

    fn laprdus_get_voice_params(handle: LaprdusHandle, out_params: *mut LaprdusVoiceParams) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| {
            if out_params.is_null() {
                return h.fail(LAPRDUS_ERROR_INVALID_PARAMETER, "Output params is NULL");
            }
            let params = h.engine.params();
            out_params.write(LaprdusVoiceParams { speed: params.speed, pitch: params.pitch, volume: params.volume });
            LAPRDUS_OK
        })
    }

    fn laprdus_set_speed(handle: LaprdusHandle, speed: f32) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        set!(handle, |p| p.speed = speed)
    }

    fn laprdus_get_speed(handle: LaprdusHandle) -> f32 = 1.0 => {
        get!(handle, 1.0, |p| p.speed)
    }

    fn laprdus_set_pitch(handle: LaprdusHandle, pitch: f32) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| {
            h.engine.set_character_pitch(pitch);
            LAPRDUS_OK
        })
    }

    fn laprdus_set_user_pitch(handle: LaprdusHandle, pitch: f32) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        set!(handle, |p| p.user_pitch = pitch)
    }

    fn laprdus_get_user_pitch(handle: LaprdusHandle) -> f32 = 1.0 => {
        get!(handle, 1.0, |p| p.user_pitch)
    }

    fn laprdus_set_volume(handle: LaprdusHandle, volume: f32) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        set!(handle, |p| p.volume = volume)
    }

    fn laprdus_set_volume_boost(handle: LaprdusHandle, enabled: c_int) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        set!(handle, |p| p.volume_boost = enabled != 0)
    }

    fn laprdus_get_volume_boost(handle: LaprdusHandle) -> c_int = 0 => {
        get!(handle, 0, |p| c_int::from(p.volume_boost))
    }

    fn laprdus_set_inflection_level(handle: LaprdusHandle, level: f32) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        set!(handle, |p| p.inflection_level = level)
    }

    fn laprdus_get_inflection_level(handle: LaprdusHandle) -> f32 = INFLECTION_LEVEL_DEFAULT => {
        get!(handle, INFLECTION_LEVEL_DEFAULT, |p| p.inflection_level)
    }

    fn laprdus_set_head_size(handle: LaprdusHandle, value: c_int) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        set!(handle, |p| p.head_size = value.clamp(VOICE_QUALITY_MIN, VOICE_QUALITY_MAX))
    }

    fn laprdus_set_breathiness(handle: LaprdusHandle, value: c_int) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        set!(handle, |p| p.breathiness = value.clamp(VOICE_QUALITY_MIN, VOICE_QUALITY_MAX))
    }

    fn laprdus_set_roughness(handle: LaprdusHandle, value: c_int) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        set!(handle, |p| p.roughness = value.clamp(VOICE_QUALITY_MIN, VOICE_QUALITY_MAX))
    }

    fn laprdus_set_brightness(handle: LaprdusHandle, value: c_int) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        set!(handle, |p| p.brightness = value.clamp(VOICE_QUALITY_MIN, VOICE_QUALITY_MAX))
    }

    fn laprdus_get_head_size(handle: LaprdusHandle) -> c_int = VOICE_QUALITY_DEFAULT => {
        get!(handle, VOICE_QUALITY_DEFAULT, |p| p.head_size)
    }

    fn laprdus_get_breathiness(handle: LaprdusHandle) -> c_int = VOICE_QUALITY_DEFAULT => {
        get!(handle, VOICE_QUALITY_DEFAULT, |p| p.breathiness)
    }

    fn laprdus_get_roughness(handle: LaprdusHandle) -> c_int = VOICE_QUALITY_DEFAULT => {
        get!(handle, VOICE_QUALITY_DEFAULT, |p| p.roughness)
    }

    fn laprdus_get_brightness(handle: LaprdusHandle) -> c_int = VOICE_QUALITY_DEFAULT => {
        get!(handle, VOICE_QUALITY_DEFAULT, |p| p.brightness)
    }

    fn laprdus_set_acceleration(handle: LaprdusHandle, acceleration: f32) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        set!(handle, |p| p.acceleration = acceleration)
    }

    fn laprdus_get_acceleration(handle: LaprdusHandle) -> f32 = ACCELERATION_DEFAULT => {
        get!(handle, ACCELERATION_DEFAULT, |p| p.acceleration)
    }

    fn laprdus_get_nominal_wpm(voice_id: *const c_char) -> f32 = 0.0 => {
        let id = string(voice_id).unwrap_or_else(|| voice::DEFAULT_VOICE_ID.to_owned());
        voice::find(&id).map_or(0.0, Voice::nominal_wpm)
    }

    // Synthesis

    fn laprdus_synthesize(handle: LaprdusHandle, text: *const c_char, out_samples: *mut *mut i16, out_format: *mut LaprdusAudioFormat) -> i32
    = LAPRDUS_ERROR_SYNTHESIS_FAILED => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| match h.text_to_speak(text, out_samples) {
            Err(code) => code,
            Ok(text) => {
                let audio = h.engine.synthesize(&text);
                h.deliver(audio, out_samples, out_format)
            }
        })
    }

    fn laprdus_synthesize_to_buffer(handle: LaprdusHandle, text: *const c_char, buffer: *mut i16, buffer_size: usize, out_format: *mut LaprdusAudioFormat) -> i32
    = LAPRDUS_ERROR_SYNTHESIS_FAILED => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| {
            if !h.engine.is_initialized() {
                return h.fail(LAPRDUS_ERROR_NOT_INITIALIZED, "Engine not initialized");
            }
            let Some(text) = string(text) else { return h.fail(LAPRDUS_ERROR_INVALID_PARAMETER, "Text is NULL") };
            match h.engine.synthesize(&text) {
                Err(error) => h.failed(&error),
                Ok(audio) => {
                    write(out_format, format());
                    // Without a buffer, or with one too small, the size needed.
                    if !buffer.is_null() && buffer_size >= audio.len() {
                        std::ptr::copy_nonoverlapping(audio.as_ptr(), buffer, audio.len());
                    }
                    audio.len().min(i32::MAX as usize) as i32
                }
            }
        })
    }

    fn laprdus_free_buffer(buffer: *mut i16) = () => {
        release(buffer);
    }

    fn laprdus_cancel(handle: LaprdusHandle) = () => {
        // Called from another thread than the one that synthesizes: the
        // engine itself is not touched.
        if let Some(cancel) = cancel_handles().get(&(handle as usize)) {
            cancel.cancel();
        }
    }

    fn laprdus_synthesize_streaming(handle: LaprdusHandle, text: *const c_char, chunk_ms: u32, callback: LaprdusAudioCallback, user_data: *mut c_void) -> i32
    = LAPRDUS_ERROR_SYNTHESIS_FAILED => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| {
            if !h.engine.is_initialized() {
                return h.fail(LAPRDUS_ERROR_NOT_INITIALIZED, "Engine not initialized");
            }
            let Some(text) = string(text) else { return h.fail(LAPRDUS_ERROR_INVALID_PARAMETER, "Text is NULL") };
            let Some(callback) = callback else { return h.fail(LAPRDUS_ERROR_INVALID_PARAMETER, "Callback is NULL") };
            let mut total = 0usize;
            let done = h.engine.synthesize_streaming(&text, chunk_ms, |chunk| {
                total += chunk.len();
                if callback(chunk.as_ptr(), chunk.len(), user_data) == 0 { ControlFlow::Continue(()) } else { ControlFlow::Break(()) }
            });
            match done {
                Ok(()) => total.min(i32::MAX as usize) as i32,
                Err(error) => h.failed(&error),
            }
        })
    }

    fn laprdus_stream_begin(handle: LaprdusHandle, text: *const c_char) -> LaprdusStreamHandle = std::ptr::null_mut() => {
        let (Some(h), Some(text)) = (handle.as_mut(), string(text)) else { return std::ptr::null_mut() };
        if !h.engine.is_initialized() {
            return std::ptr::null_mut();
        }
        // The whole audio is synthesized up front and read in pieces.
        match h.engine.synthesize(&text) {
            Ok(audio) => Box::into_raw(Box::new(LaprdusStream { audio, position: 0, complete: false })),
            Err(error) => {
                h.last_error = error.to_string();
                std::ptr::null_mut()
            }
        }
    }

    fn laprdus_stream_read(stream: LaprdusStreamHandle, buffer: *mut i16, max_samples: usize) -> i32 = LAPRDUS_ERROR_INVALID_PARAMETER => {
        let Some(stream) = stream.as_mut().filter(|_| !buffer.is_null() && max_samples > 0) else {
            return LAPRDUS_ERROR_INVALID_PARAMETER;
        };
        if stream.complete {
            return 0;
        }
        let rest = &stream.audio[stream.position..];
        let count = rest.len().min(max_samples).min(i32::MAX as usize);
        std::ptr::copy_nonoverlapping(rest.as_ptr(), buffer, count);
        stream.position += count;
        stream.complete = stream.position >= stream.audio.len();
        count as i32
    }

    fn laprdus_stream_progress(stream: LaprdusStreamHandle) -> f32 = 1.0 => {
        match stream.as_ref() {
            Some(stream) if !stream.audio.is_empty() => stream.position as f32 / stream.audio.len() as f32,
            _ => 1.0,
        }
    }

    fn laprdus_stream_is_complete(stream: LaprdusStreamHandle) -> c_int = 1 => {
        stream.as_ref().map_or(1, |stream| c_int::from(stream.complete))
    }

    fn laprdus_stream_destroy(stream: LaprdusStreamHandle) = () => {
        if !stream.is_null() {
            drop(Box::from_raw(stream));
        }
    }

    // Utilities

    fn laprdus_get_error_message(handle: LaprdusHandle) -> *const c_char = c"".as_ptr() => {
        with!(handle, c"Invalid handle".as_ptr(), |h| held(&ERROR_MESSAGE, &h.last_error))
    }

    fn laprdus_get_version() -> *const c_char = c"".as_ptr() => {
        c_str(Engine::version())
    }

    fn laprdus_get_default_format(out_format: *mut LaprdusAudioFormat) = () => {
        write(out_format, format());
    }

    fn laprdus_error_to_string(error: LaprdusError) -> *const c_char = c"Unknown error".as_ptr() => {
        c_str(error_text(error))
    }

    // Voices

    fn laprdus_get_voice_count() -> u32 = 0 => {
        VOICES.len() as u32
    }

    fn laprdus_get_voice_info(index: u32, out_info: *mut LaprdusVoiceInfo) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        match VOICES.get(index as usize) {
            Some(voice) if !out_info.is_null() => {
                voice_info(voice, out_info);
                LAPRDUS_OK
            }
            _ => LAPRDUS_ERROR_INVALID_PARAMETER,
        }
    }

    fn laprdus_get_voice_info_by_id(voice_id: *const c_char, out_info: *mut LaprdusVoiceInfo) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        match string(voice_id).and_then(|id| voice::find(&id)) {
            Some(voice) if !out_info.is_null() => {
                voice_info(voice, out_info);
                LAPRDUS_OK
            }
            _ => LAPRDUS_ERROR_INVALID_PARAMETER,
        }
    }

    fn laprdus_set_voice(handle: LaprdusHandle, voice_id: *const c_char, data_directory: *const c_char) -> LaprdusError = LAPRDUS_ERROR_LOAD_FAILED => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| {
            let Some(id) = string(voice_id) else { return h.fail(LAPRDUS_ERROR_INVALID_PARAMETER, "Voice ID is NULL") };
            let Some(directory) = path(data_directory) else { return h.fail(LAPRDUS_ERROR_INVALID_PATH, "Data directory is NULL") };
            match h.engine.set_voice(&id, &directory) {
                Ok(()) => LAPRDUS_OK,
                Err(error @ Error::VoiceNotFound(_)) => h.fail(LAPRDUS_ERROR_INVALID_PARAMETER, error.to_string()),
                Err(error) => h.fail(LAPRDUS_ERROR_LOAD_FAILED, error.to_string()),
            }
        })
    }

    fn laprdus_get_current_voice(handle: LaprdusHandle) -> *const c_char = std::ptr::null() => {
        handle.as_ref().and_then(|h| h.engine.voice()).map_or(std::ptr::null(), |voice| c_str(voice.id))
    }

    fn laprdus_get_default_voice() -> *const c_char = c"zvonko".as_ptr() => {
        c_str(voice::DEFAULT_VOICE_ID)
    }

    // Languages

    fn laprdus_get_language_count() -> u32 = 0 => {
        Language::ALL.len() as u32
    }

    fn laprdus_get_language_info(index: u32, out_info: *mut LaprdusLanguageInfo) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        match Language::ALL.get(index as usize) {
            Some(language) if !out_info.is_null() => {
                out_info.write(LaprdusLanguageInfo {
                    code: c_str(language.code()),
                    short_code: c_str(language.short_code()),
                    lcid: language.lcid(),
                    english_name: c_str(language.english_name()),
                    native_name: c_str(language.native_name()),
                });
                LAPRDUS_OK
            }
            _ => LAPRDUS_ERROR_INVALID_PARAMETER,
        }
    }

    fn laprdus_set_language(handle: LaprdusHandle, language: *const c_char) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| match string(language).filter(|tag| !tag.is_empty()) {
            // Back to the voice's native language.
            None => {
                h.engine.set_language(None);
                LAPRDUS_OK
            }
            Some(tag) => match Language::parse(&tag) {
                Some(language) => {
                    h.engine.set_language(Some(language));
                    LAPRDUS_OK
                }
                None => h.fail(LAPRDUS_ERROR_INVALID_PARAMETER, format!("Unknown language: {tag}")),
            },
        })
    }

    fn laprdus_get_language(handle: LaprdusHandle) -> *const c_char = std::ptr::null() => {
        handle.as_ref().map_or(std::ptr::null(), |h| c_str(h.engine.language().code()))
    }

    // Pronunciation dictionary

    fn laprdus_load_dictionary(handle: LaprdusHandle, dictionary_path: *const c_char) -> LaprdusError = LAPRDUS_ERROR_LOAD_FAILED => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| match path(dictionary_path) {
            None => h.fail(LAPRDUS_ERROR_INVALID_PATH, "Dictionary path is NULL"),
            Some(path) => {
                let loaded = h.engine.dictionary().load_file(&path);
                h.check(loaded, LAPRDUS_ERROR_LOAD_FAILED, "Failed to load dictionary")
            }
        })
    }

    fn laprdus_load_dictionary_from_memory(handle: LaprdusHandle, json_content: *const c_char, length: usize) -> LaprdusError = LAPRDUS_ERROR_LOAD_FAILED => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| match json(json_content, length) {
            None => h.fail(LAPRDUS_ERROR_INVALID_PARAMETER, "Dictionary content is NULL"),
            Some(json) => {
                let loaded = h.engine.dictionary().load(&json);
                h.check(loaded, LAPRDUS_ERROR_LOAD_FAILED, "Failed to parse dictionary content")
            }
        })
    }

    fn laprdus_append_dictionary(handle: LaprdusHandle, dictionary_path: *const c_char) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| match path(dictionary_path) {
            Some(path) if h.engine.dictionary().append_file(&path) => LAPRDUS_OK,
            _ => LAPRDUS_ERROR_INVALID_PARAMETER,
        })
    }

    fn laprdus_clear_dictionary(handle: LaprdusHandle) = () => {
        with!(handle, (), |h| h.engine.dictionary().clear())
    }

    // Spelling

    fn laprdus_load_spelling_dictionary(handle: LaprdusHandle, dictionary_path: *const c_char) -> LaprdusError = LAPRDUS_ERROR_LOAD_FAILED => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| match path(dictionary_path) {
            None => h.fail(LAPRDUS_ERROR_INVALID_PATH, "Spelling dictionary path is NULL"),
            Some(path) => {
                let loaded = h.engine.load_spelling_dictionary_file(&path);
                h.check(loaded, LAPRDUS_ERROR_LOAD_FAILED, "Failed to load spelling dictionary")
            }
        })
    }

    fn laprdus_load_spelling_dictionary_from_memory(handle: LaprdusHandle, json_content: *const c_char, length: usize) -> LaprdusError
    = LAPRDUS_ERROR_LOAD_FAILED => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| match json(json_content, length) {
            None => h.fail(LAPRDUS_ERROR_INVALID_PARAMETER, "Spelling dictionary content is NULL"),
            Some(json) => {
                let loaded = h.engine.load_spelling_dictionary(&json);
                h.check(loaded, LAPRDUS_ERROR_LOAD_FAILED, "Failed to parse spelling dictionary content")
            }
        })
    }

    fn laprdus_append_spelling_dictionary(handle: LaprdusHandle, dictionary_path: *const c_char) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| match path(dictionary_path).and_then(|path| std::fs::read_to_string(path).ok()) {
            Some(json) if h.engine.append_spelling_dictionary(&json) => LAPRDUS_OK,
            _ => LAPRDUS_ERROR_INVALID_PARAMETER,
        })
    }

    fn laprdus_add_spelling_entry(handle: LaprdusHandle, character: *const c_char, pronunciation: *const c_char) -> LaprdusError
    = LAPRDUS_ERROR_INVALID_PARAMETER => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| match (string(character), string(pronunciation)) {
            (Some(character), Some(pronunciation)) if !character.is_empty() && !pronunciation.is_empty() => {
                h.engine.add_spelling_entry(&character, &pronunciation);
                LAPRDUS_OK
            }
            _ => LAPRDUS_ERROR_INVALID_PARAMETER,
        })
    }

    fn laprdus_clear_spelling_dictionary(handle: LaprdusHandle) = () => {
        with!(handle, (), |h| h.engine.clear_spelling_dictionary())
    }

    fn laprdus_synthesize_spelled(handle: LaprdusHandle, text: *const c_char, out_samples: *mut *mut i16, out_format: *mut LaprdusAudioFormat) -> i32
    = LAPRDUS_ERROR_SYNTHESIS_FAILED => {
        laprdus_synthesize_spelled_ex(handle, text, 1, std::ptr::null_mut(), out_samples, out_format)
    }

    fn laprdus_synthesize_spelled_ex(
        handle: LaprdusHandle,
        text: *const c_char,
        final_pause: c_int,
        out_final_pause_ms: *mut u32,
        out_samples: *mut *mut i16,
        out_format: *mut LaprdusAudioFormat,
    ) -> i32
    = LAPRDUS_ERROR_SYNTHESIS_FAILED => {
        write(out_final_pause_ms, 0);
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| match h.text_to_speak(text, out_samples) {
            Err(code) => code,
            Ok(text) => {
                let spelled = h.engine.synthesize_spelled(&text, final_pause != 0);
                let audio = spelled.map(|(audio, pause_ms)| {
                    write(out_final_pause_ms, pause_ms);
                    audio
                });
                h.deliver(audio, out_samples, out_format)
            }
        })
    }

    // Emoji

    fn laprdus_load_emoji_dictionary(handle: LaprdusHandle, dictionary_path: *const c_char) -> LaprdusError = LAPRDUS_ERROR_LOAD_FAILED => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| match path(dictionary_path) {
            None => h.fail(LAPRDUS_ERROR_INVALID_PATH, "Emoji dictionary path is NULL"),
            Some(path) => {
                let loaded = h.engine.load_emoji_dictionary_file(&path);
                h.check(loaded, LAPRDUS_ERROR_LOAD_FAILED, "Failed to load emoji dictionary")
            }
        })
    }

    fn laprdus_load_emoji_dictionary_from_memory(handle: LaprdusHandle, json_content: *const c_char, length: usize) -> LaprdusError
    = LAPRDUS_ERROR_LOAD_FAILED => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| match json(json_content, length) {
            None => h.fail(LAPRDUS_ERROR_INVALID_PARAMETER, "Emoji dictionary content is NULL"),
            Some(json) => {
                let loaded = h.engine.load_emoji_dictionary(&json);
                h.check(loaded, LAPRDUS_ERROR_LOAD_FAILED, "Failed to parse emoji dictionary content")
            }
        })
    }

    fn laprdus_append_emoji_dictionary(handle: LaprdusHandle, dictionary_path: *const c_char) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| match path(dictionary_path).and_then(|path| std::fs::read_to_string(path).ok()) {
            Some(json) if h.engine.append_emoji_dictionary(&json) => LAPRDUS_OK,
            _ => LAPRDUS_ERROR_INVALID_PARAMETER,
        })
    }

    fn laprdus_clear_emoji_dictionary(handle: LaprdusHandle) = () => {
        with!(handle, (), |h| h.engine.clear_emoji_dictionary())
    }

    // Accent lexicon

    fn laprdus_load_accent_lexicon(handle: LaprdusHandle, lexicon_path: *const c_char) -> LaprdusError = LAPRDUS_ERROR_LOAD_FAILED => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| match path(lexicon_path) {
            None => LAPRDUS_ERROR_INVALID_PARAMETER,
            Some(path) if h.engine.load_accent_lexicon_file(&path) => LAPRDUS_OK,
            Some(_) => {
                let report = format!("Accent lexicon: {}", h.engine.accent_lexicon_report());
                h.fail(LAPRDUS_ERROR_LOAD_FAILED, report)
            }
        })
    }

    fn laprdus_load_accent_lexicon_from_memory(handle: LaprdusHandle, json_content: *const c_char, length: usize) -> LaprdusError
    = LAPRDUS_ERROR_LOAD_FAILED => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| match json(json_content, length) {
            None => LAPRDUS_ERROR_INVALID_PARAMETER,
            Some(json) if h.engine.load_accent_lexicon(&json) => LAPRDUS_OK,
            Some(_) => {
                let report = format!("Accent lexicon: {}", h.engine.accent_lexicon_report());
                h.fail(LAPRDUS_ERROR_LOAD_FAILED, report)
            }
        })
    }

    fn laprdus_clear_accent_lexicon(handle: LaprdusHandle) = () => {
        with!(handle, (), |h| h.engine.clear_accent_lexicon())
    }

    fn laprdus_get_accent_lexicon_report(handle: LaprdusHandle) -> *const c_char = c"".as_ptr() => {
        with!(handle, c"".as_ptr(), |h| held(&LEXICON_REPORT, h.engine.accent_lexicon_report()))
    }

    fn laprdus_check_accent_lexicon(json_content: *const c_char, length: usize, message: *mut c_char, message_size: usize) -> c_int = -1 => {
        copy_out("", message, message_size);
        let Some(json) = json(json_content, length) else { return -1 };
        let (_, report) = UserLexicon::parse(&json);
        if report.rejected > 0 {
            copy_out(&report.first_error, message, message_size);
        }
        report.rejected.min(c_int::MAX as usize) as c_int
    }

    fn laprdus_set_emoji_enabled(handle: LaprdusHandle, enabled: c_int) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        set!(handle, |p| p.emoji_enabled = enabled != 0)
    }

    fn laprdus_is_emoji_enabled(handle: LaprdusHandle) -> c_int = 0 => {
        get!(handle, 0, |p| c_int::from(p.emoji_enabled))
    }

    // Pauses, spelling, numbers

    fn laprdus_set_sentence_pause(handle: LaprdusHandle, pause_ms: u32) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| {
            h.engine.update_pauses(|pauses| pauses.sentence_pause_ms = pause_ms);
            LAPRDUS_OK
        })
    }

    fn laprdus_set_comma_pause(handle: LaprdusHandle, pause_ms: u32) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| {
            h.engine.update_pauses(|pauses| pauses.comma_pause_ms = pause_ms);
            LAPRDUS_OK
        })
    }

    fn laprdus_set_newline_pause(handle: LaprdusHandle, pause_ms: u32) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| {
            h.engine.update_pauses(|pauses| pauses.newline_pause_ms = pause_ms);
            LAPRDUS_OK
        })
    }

    fn laprdus_get_sentence_pause(handle: LaprdusHandle) -> u32 = 100 => {
        get!(handle, 100, |p| p.pause_settings.sentence_pause_ms)
    }

    fn laprdus_get_comma_pause(handle: LaprdusHandle) -> u32 = 100 => {
        get!(handle, 100, |p| p.pause_settings.comma_pause_ms)
    }

    fn laprdus_get_newline_pause(handle: LaprdusHandle) -> u32 = 100 => {
        get!(handle, 100, |p| p.pause_settings.newline_pause_ms)
    }

    fn laprdus_set_spelling_pause(handle: LaprdusHandle, pause_ms: u32) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| {
            h.engine.set_spelling_pause(pause_ms);
            LAPRDUS_OK
        })
    }

    fn laprdus_get_spelling_pause(handle: LaprdusHandle) -> u32 = 200 => {
        get!(handle, 200, |p| p.pause_settings.spelling_pause_ms)
    }

    fn laprdus_set_spelling_mode(handle: LaprdusHandle, mode: c_int) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        set!(handle, |p| p.spelling_mode =
            if mode == LAPRDUS_SPELLING_LETTER_SOUNDS { SpellingMode::LetterSounds } else { SpellingMode::LetterNames })
    }

    fn laprdus_get_spelling_mode(handle: LaprdusHandle) -> c_int = 0 => {
        get!(handle, 0, |p| c_int::from(p.spelling_mode == SpellingMode::LetterSounds))
    }

    fn laprdus_set_spelling_speed(handle: LaprdusHandle, percent: c_int) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        set!(handle, |p| p.spelling_speed = percent.clamp(SPELLING_SPEED_MIN, SPELLING_SPEED_MAX))
    }

    fn laprdus_get_spelling_speed(handle: LaprdusHandle) -> c_int = SPELLING_SPEED_DEFAULT => {
        get!(handle, SPELLING_SPEED_DEFAULT, |p| p.spelling_speed)
    }

    fn laprdus_set_number_mode(handle: LaprdusHandle, mode: c_int) -> LaprdusError = LAPRDUS_ERROR_INVALID_PARAMETER => {
        set!(handle, |p| p.number_mode =
            if mode == LAPRDUS_NUMBER_MODE_DIGIT { NumberMode::DigitByDigit } else { NumberMode::WholeNumbers })
    }

    fn laprdus_get_number_mode(handle: LaprdusHandle) -> c_int = 0 => {
        get!(handle, 0, |p| c_int::from(p.number_mode == NumberMode::DigitByDigit))
    }

    // The user's configuration

    fn laprdus_get_config_directory(buffer: *mut c_char, buffer_size: usize) -> usize = 0 => {
        UserConfig::directory().map_or(0, |directory| copy_out(&directory.to_string_lossy(), buffer, buffer_size))
    }

    fn laprdus_load_user_config(handle: LaprdusHandle) -> LaprdusError = LAPRDUS_ERROR_LOAD_FAILED => {
        with!(handle, LAPRDUS_ERROR_INVALID_HANDLE, |h| {
            let loaded = h.engine.load_user_config();
            h.check(loaded, LAPRDUS_ERROR_LOAD_FAILED, "Failed to load user configuration")
        })
    }

    fn laprdus_user_dictionary_exists(filename: *const c_char) -> c_int = 0 => {
        string(filename).map_or(0, |name| c_int::from(UserConfig::new().exists(&name)))
    }

    fn laprdus_get_user_dictionary_path(filename: *const c_char, buffer: *mut c_char, buffer_size: usize) -> usize = 0 => {
        string(filename)
            .and_then(|name| UserConfig::new().path(&name))
            .map_or(0, |path| copy_out(&path.to_string_lossy(), buffer, buffer_size))
    }
}
