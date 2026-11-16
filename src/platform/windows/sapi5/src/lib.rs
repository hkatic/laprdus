//! The SAPI5 driver: `laprd64.dll` and `laprd32.dll`.
//!
//! SAPI creates the engine object for a voice token and calls `Speak` with
//! the text as a list of fragments. The installer registers four tokens, one
//! per language; the token sets the language, and the voice is the one of the
//! user's `settings.json` (`voice.default`, empty = Zvonko), which the
//! Configurator edits. A changed `settings.json` is picked up at the next
//! `Speak`, without a restart of the application.
//!
//! The library also exports the C API (`laprdus_*`): the NVDA add-on loads it
//! for that.
#![cfg(windows)]

// Linked for its exported functions.
use laprdus_capi as _;

use laprdus::config::{UserConfig, UserSettings};
use laprdus::engine::Engine;
use laprdus::language::Language;
use laprdus::params::VoiceParams;
use laprdus::voice;
use laprdus::{SAMPLE_RATE, silence};
use std::ffi::c_void;
use std::path::PathBuf;
use std::sync::{Mutex, PoisonError};
use std::time::SystemTime;
use windows::Win32::Foundation::{
    CLASS_E_CLASSNOTAVAILABLE, CLASS_E_NOAGGREGATION, E_FAIL, E_INVALIDARG, E_POINTER, HMODULE, S_FALSE, S_OK,
};
use windows::Win32::Media::Audio::{WAVE_FORMAT_PCM, WAVEFORMATEX};
use windows::Win32::Media::Speech::{
    ISpObjectToken, ISpObjectWithToken, ISpObjectWithToken_Impl, ISpTTSEngine, ISpTTSEngine_Impl, ISpTTSEngineSite,
    SPVA_Bookmark, SPVA_Silence, SPVA_SpellOut, SPVES_ABORT, SPVTEXTFRAG,
};
use windows::Win32::System::Com::{CoTaskMemAlloc, CoTaskMemFree, IClassFactory, IClassFactory_Impl};
use windows::Win32::System::LibraryLoader::{
    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, GetModuleFileNameW,
    GetModuleHandleExW,
};
use windows_core::{BOOL, GUID, HRESULT, IUnknown, Interface, PCWSTR, PWSTR, Ref, Result, implement, w};

/// `{D4B5A4E1-8B5C-4C5D-9E6F-1A2B3C4D5E6F}`: the class the voice tokens name.
const CLSID_LAPRDUS: GUID = GUID::from_u128(0xD4B5A4E1_8B5C_4C5D_9E6F_1A2B3C4D5E6F);
const CLSID_KEY: &str = r"CLSID\{D4B5A4E1-8B5C-4C5D-9E6F-1A2B3C4D5E6F}";
const PROG_ID: &str = "Laprdus.TTSEngine.1";

/// `SPDFID_WaveFormatEx`: the output is described by a `WAVEFORMATEX`.
const SPDFID_WAVE_FORMAT_EX: GUID = GUID::from_u128(0xC31ADBAE_527F_4FF5_A230_F62BB61FF70C);

/// Audio goes to the site in pieces of this many samples, so a stop is seen
/// between two of them.
const CHUNK_SAMPLES: usize = 4096;

/// The path of this library.
fn module_path() -> Option<PathBuf> {
    static ANCHOR: u8 = 0;
    let mut module = HMODULE::default();
    let flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
    let mut path = [0u16; 1024];
    // SAFETY: the address is one inside this module, and the buffer is as
    // long as the call is told.
    let length = unsafe {
        GetModuleHandleExW(flags, PCWSTR((&raw const ANCHOR).cast()), &mut module).ok()?;
        GetModuleFileNameW(Some(module), &mut path) as usize
    };
    (length > 0).then(|| String::from_utf16_lossy(&path[..length]).into())
}

/// A string value of a token or one of its keys, when it has one.
fn string_value(value: Result<PWSTR>) -> Option<String> {
    let value = value.ok().filter(|value| !value.is_null())?;
    // SAFETY: SAPI returned a NUL-terminated string allocated for the caller
    // to free.
    unsafe {
        let text = value.to_string().ok();
        CoTaskMemFree(Some(value.as_ptr().cast()));
        text
    }
}

/// The language of a token: its `Language` value as a tag (the tokens the
/// installer writes), else the language id among its attributes.
fn token_language(token: &ISpObjectToken) -> Language {
    // SAFETY: plain calls on a valid interface.
    let (tag, lcid) = unsafe {
        let attributes = token.OpenKey(w!("Attributes")).ok();
        (
            string_value(token.GetStringValue(w!("Language"))),
            attributes.and_then(|key| string_value(key.GetStringValue(w!("Language")))),
        )
    };
    let by_lcid = || {
        let lcid = u32::from_str_radix(lcid.as_deref()?.trim(), 16).ok()? & 0xFFFF;
        Language::ALL.into_iter().find(|language| u32::from(language.lcid()) == lcid)
    };
    match tag {
        Some(tag) => Language::parse(&tag).unwrap_or_default(),
        None => by_lcid().unwrap_or_default(),
    }
}

/// One stretch of what SAPI asked for.
enum Run {
    Text { text: String, spell: bool },
    Silence(u32),
}

/// The fragments as runs. Consecutive fragments of the same kind stay one
/// run, so a sentence that SAPI cut at a tag is still synthesized as one
/// sentence; text is trimmed (SAPI often adds trailing spaces and line
/// breaks) and runs with nothing left are dropped.
///
/// # Safety
/// `fragment` must be null or the head of a valid fragment list.
unsafe fn runs_of(mut fragment: *const SPVTEXTFRAG) -> Vec<Run> {
    let mut runs: Vec<Run> = Vec::new();
    // SAFETY: the caller's list is valid, and each text has the length given.
    while let Some(frag) = unsafe { fragment.as_ref() } {
        let action = frag.State.eAction;
        if action == SPVA_Silence {
            if frag.State.SilenceMSecs > 0 {
                runs.push(Run::Silence(frag.State.SilenceMSecs));
            }
        } else if action != SPVA_Bookmark && !frag.pTextStart.is_null() && frag.ulTextLen > 0 {
            // Only a spell-out fragment is spelled; pronunciation hints and
            // unknown actions are spoken as text.
            let spell = action == SPVA_SpellOut;
            let piece = String::from_utf16_lossy(unsafe {
                std::slice::from_raw_parts(frag.pTextStart.0, frag.ulTextLen as usize)
            });
            match runs.last_mut() {
                Some(Run::Text { text, spell: spelled }) if *spelled == spell => text.push_str(&piece),
                _ => runs.push(Run::Text { text: piece, spell }),
            }
        }
        fragment = frag.pNext;
    }
    runs.into_iter()
        .filter_map(|run| match run {
            Run::Text { text, spell } => {
                let text = text.trim_matches([' ', '\t', '\r', '\n']);
                (!text.is_empty()).then(|| Run::Text { text: text.to_owned(), spell })
            }
            silence => Some(silence),
        })
        .collect()
}

fn should_stop(site: &ISpTTSEngineSite) -> bool {
    // SAFETY: a plain call on a valid interface.
    unsafe { site.GetActions() & SPVES_ABORT.0 as u32 != 0 }
}

/// The engine of one voice token with the user's settings.
struct Voice {
    engine: Engine,
    settings: UserSettings,
    settings_path: Option<PathBuf>,
    settings_time: Option<SystemTime>,
}

fn modified(path: Option<&PathBuf>) -> Option<SystemTime> {
    std::fs::metadata(path?).and_then(|file| file.modified()).ok()
}

impl Voice {
    /// The engine for a token: the token's language, the voice of the token
    /// or of the user's settings, the bundled dictionaries from the install
    /// directory and the user's own on top.
    fn new(token: &ISpObjectToken) -> Option<Self> {
        let mut config = UserConfig::new();
        let settings_path = config.path(UserConfig::SETTINGS);
        let settings_time = modified(settings_path.as_ref());
        let loaded = config.load_settings();
        let settings = config.settings.clone();

        // SAFETY: a plain call on a valid interface.
        let voice_id = string_value(unsafe { token.GetStringValue(w!("VoiceId")) })
            .or_else(|| Some(settings.default_voice.clone()).filter(|id| loaded && !id.is_empty()));
        let voice = voice_id.and_then(|id| voice::find(&id)).unwrap_or_else(voice::default_voice);

        let install_dir = module_path()?.parent()?.to_owned();
        let mut engine = Engine::new();
        engine.set_language(Some(token_language(token)));
        engine.set_voice(voice.id, &install_dir.join("voices")).ok()?;

        let dictionaries = install_dir.join("dictionaries");
        engine.dictionary().load_file(&dictionaries.join("dictionary.json"));
        engine.load_spelling_dictionary_file(&dictionaries.join("spelling.json"));
        engine.load_emoji_dictionary_file(&dictionaries.join("emoji.json"));
        if loaded {
            let pitch = engine.params().pitch;
            engine.set_params(&VoiceParams { pitch, ..settings.to_voice_params() });
            if settings.user_dictionaries_enabled {
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
                if let Some(path) = user_file(UserConfig::ACCENTS) {
                    engine.load_accent_lexicon_file(&path);
                }
            }
        }
        Some(Self {
            engine,
            settings: if loaded { settings } else { UserSettings::default() },
            settings_path,
            settings_time,
        })
    }

    /// The rate, pitch and volume of this call. The user's rate and pitch
    /// are the voice's normal and SAPI's are relative to it, so they
    /// multiply; the force switches mean "ignore SAPI".
    fn apply_request(&mut self, site: &ISpTTSEngineSite, first: Option<&SPVTEXTFRAG>) {
        let settings = &self.settings;
        let (mut speed, mut user_pitch, mut volume) = (settings.speed, settings.user_pitch, settings.volume);
        // SAFETY: plain calls on a valid interface.
        let (rate, site_volume) = unsafe { (site.GetRate().unwrap_or(0), site.GetVolume().ok()) };
        if !settings.force_speed && rate != 0 {
            speed *= 2f32.powf(rate as f32 / 10.0).clamp(0.5, 2.0);
        }
        if !settings.force_volume {
            volume = site_volume.map_or(1.0, |volume| (f32::from(volume) / 100.0).clamp(0.0, 1.0));
        }
        if let Some(first) = first.filter(|_| !settings.force_pitch) {
            user_pitch *= 2f32.powf(first.State.PitchAdj.MiddleAdj as f32 / 24.0).clamp(0.5, 2.0);
        }
        self.engine.update_params(|params| {
            params.speed = speed;
            params.user_pitch = user_pitch;
            params.volume = volume;
        });
    }

    /// The audio of the runs; `None` when none of them could be spoken.
    fn synthesize(&mut self, runs: &[Run], site: &ISpTTSEngineSite) -> Option<Vec<i16>> {
        let spelling_pause = self.engine.params().pause_settings.spelling_pause_ms;
        let text_runs = runs.iter().filter(|run| matches!(run, Run::Text { .. })).count();
        let mut audio = Vec::new();
        let (mut succeeded, mut failed) = (false, false);
        // True while the audio so far ends in a pause (or there is none yet).
        let mut separated = true;
        let mut previous_spelled = false;
        for run in runs {
            if should_stop(site) {
                break;
            }
            let (text, spell) = match run {
                Run::Silence(ms) => {
                    audio.extend(silence(*ms));
                    separated = true;
                    continue;
                }
                Run::Text { text, spell } => (text, *spell),
            };
            // Many SAPI5 applications spell by sending single characters as
            // ordinary text, so an utterance that is one character is
            // spelled too.
            let spell = spell || (text_runs == 1 && text.chars().count() == 1);
            let spoken = if spell {
                self.engine.synthesize_spelled(text, true).map(|(audio, _)| audio)
            } else {
                self.engine.synthesize(text)
            };
            // One run that cannot be spoken must not silence the others.
            let Ok(spoken) = spoken else {
                failed = true;
                continue;
            };
            succeeded = true;
            // Spelled characters are kept apart by the spelling pause; so is
            // the spoken text next to them.
            if !separated && spell != previous_spelled {
                audio.extend(silence(spelling_pause));
            }
            audio.extend(spoken);
            // The engine ends every spelled run with the spelling pause.
            separated = spell;
            previous_spelled = spell;
        }
        (succeeded || !failed).then_some(audio)
    }
}

#[implement(ISpTTSEngine, ISpObjectWithToken)]
#[derive(Default)]
struct Driver {
    token: Mutex<Option<ISpObjectToken>>,
    voice: Mutex<Option<Voice>>,
}

impl Driver {
    fn token(&self) -> Option<ISpObjectToken> {
        self.token.lock().unwrap_or_else(PoisonError::into_inner).clone()
    }
}

impl ISpObjectWithToken_Impl for Driver_Impl {
    fn SetObjectToken(&self, token: Ref<ISpObjectToken>) -> Result<()> {
        let token = token.ok().map_err(|_| E_INVALIDARG)?.clone();
        let voice = Voice::new(&token).ok_or(E_FAIL)?;
        *self.token.lock().unwrap_or_else(PoisonError::into_inner) = Some(token);
        *self.voice.lock().unwrap_or_else(PoisonError::into_inner) = Some(voice);
        Ok(())
    }

    fn GetObjectToken(&self) -> Result<ISpObjectToken> {
        self.token().ok_or(E_FAIL.into())
    }
}

impl ISpTTSEngine_Impl for Driver_Impl {
    fn Speak(
        &self,
        _flags: u32,
        _format_id: *const GUID,
        _format: *const WAVEFORMATEX,
        fragments: *const SPVTEXTFRAG,
        site: Ref<ISpTTSEngineSite>,
    ) -> Result<()> {
        let mut voice = self.voice.lock().unwrap_or_else(PoisonError::into_inner);
        // A changed settings.json: the engine is made again; the old one
        // stays when that fails.
        if let Some(current) = voice.as_ref()
            && modified(current.settings_path.as_ref()) != current.settings_time
            && let Some(reloaded) = self.token().and_then(|token| Voice::new(&token))
        {
            *voice = Some(reloaded);
        }
        let voice = voice.as_mut().ok_or(E_FAIL)?;
        let site = site.ok().map_err(|_| E_INVALIDARG)?;
        if fragments.is_null() {
            return Err(E_INVALIDARG.into());
        }

        // SAFETY: SAPI hands a valid fragment list.
        let runs = unsafe {
            voice.apply_request(site, fragments.as_ref());
            runs_of(fragments)
        };
        let audio = voice.synthesize(&runs, site).ok_or(E_FAIL)?;
        for chunk in audio.chunks(CHUNK_SAMPLES) {
            if should_stop(site) {
                break;
            }
            // SAFETY: the pointer and the byte count describe `chunk`.
            unsafe { site.Write(chunk.as_ptr().cast(), size_of_val(chunk) as u32)? };
        }
        Ok(())
    }

    fn GetOutputFormat(
        &self,
        _target_id: *const GUID,
        _target: *const WAVEFORMATEX,
        format_id: *mut GUID,
        format: *mut *mut WAVEFORMATEX,
    ) -> Result<()> {
        if format_id.is_null() || format.is_null() {
            return Err(E_POINTER.into());
        }
        let block_align = 2;
        let description = WAVEFORMATEX {
            wFormatTag: WAVE_FORMAT_PCM as u16,
            nChannels: 1,
            nSamplesPerSec: SAMPLE_RATE,
            nAvgBytesPerSec: SAMPLE_RATE * u32::from(block_align),
            nBlockAlign: block_align,
            wBitsPerSample: 16,
            cbSize: 0,
        };
        // SAFETY: the out pointers were checked, and the allocation has the
        // size of what is written into it; the caller frees it.
        unsafe {
            let allocated = CoTaskMemAlloc(size_of::<WAVEFORMATEX>()).cast::<WAVEFORMATEX>();
            if allocated.is_null() {
                return Err(windows::Win32::Foundation::E_OUTOFMEMORY.into());
            }
            allocated.write_unaligned(description);
            format_id.write(SPDFID_WAVE_FORMAT_EX);
            format.write(allocated);
        }
        Ok(())
    }
}

#[implement(IClassFactory)]
struct Factory;

impl IClassFactory_Impl for Factory_Impl {
    fn CreateInstance(&self, outer: Ref<IUnknown>, iid: *const GUID, object: *mut *mut c_void) -> Result<()> {
        if object.is_null() || iid.is_null() {
            return Err(E_POINTER.into());
        }
        if outer.is_some() {
            return Err(CLASS_E_NOAGGREGATION.into());
        }
        let driver: IUnknown = Driver::default().into();
        // SAFETY: the out pointers were checked.
        unsafe { driver.query(iid, object).ok() }
    }

    fn LockServer(&self, _lock: BOOL) -> Result<()> {
        Ok(())
    }
}

/// Runs an entry point of the library; a panic is a failure, never an unwind
/// into the host.
fn guarded(entry: impl FnOnce() -> HRESULT) -> HRESULT {
    std::panic::catch_unwind(std::panic::AssertUnwindSafe(entry)).unwrap_or(E_FAIL)
}

/// # Safety
/// COM calls this with valid pointers; null ones are refused.
#[unsafe(no_mangle)]
pub unsafe extern "system" fn DllGetClassObject(
    class_id: *const GUID,
    iid: *const GUID,
    object: *mut *mut c_void,
) -> HRESULT {
    guarded(|| {
        if class_id.is_null() || iid.is_null() || object.is_null() {
            return E_POINTER;
        }
        // SAFETY: the pointers were checked; COM passes valid ones.
        unsafe {
            if *class_id != CLSID_LAPRDUS {
                return CLASS_E_CLASSNOTAVAILABLE;
            }
            let factory: IClassFactory = Factory.into();
            factory.query(iid, object)
        }
    })
}

/// The library stays loaded while its process lives: engines are cheap to
/// keep and a screen reader asks for them again at once.
#[unsafe(no_mangle)]
pub extern "system" fn DllCanUnloadNow() -> HRESULT {
    S_FALSE
}

/// Registers the COM class; the voice tokens are the installer's.
#[unsafe(no_mangle)]
pub extern "system" fn DllRegisterServer() -> HRESULT {
    guarded(|| {
        let register = || -> Result<()> {
            let module = module_path().ok_or(E_FAIL)?;
            let classes = windows_registry::CLASSES_ROOT;
            let class = classes.create(CLSID_KEY)?;
            class.set_string("", "LaprdusTTS")?;
            let server = class.create("InprocServer32")?;
            server.set_string("", module.to_string_lossy())?;
            server.set_string("ThreadingModel", "Both")?;
            class.create("ProgID")?.set_string("", PROG_ID)?;
            class.create("Version")?.set_string("", "1.0")?;
            let prog_id = classes.create(PROG_ID)?;
            prog_id.set_string("", "LaprdusTTS Engine")?;
            prog_id.create("CLSID")?.set_string("", &CLSID_KEY["CLSID\\".len()..])
        };
        register().map_or_else(|error| error.code(), |()| S_OK)
    })
}

#[unsafe(no_mangle)]
pub extern "system" fn DllUnregisterServer() -> HRESULT {
    guarded(|| {
        // Keys that are not there are as good as removed.
        let _ = windows_registry::CLASSES_ROOT.remove_tree(CLSID_KEY);
        let _ = windows_registry::CLASSES_ROOT.remove_tree(PROG_ID);
        S_OK
    })
}

#[unsafe(no_mangle)]
pub extern "system" fn DllInstall(install: BOOL, _command_line: PCWSTR) -> HRESULT {
    if !install.as_bool() {
        return DllUnregisterServer();
    }
    let result = DllRegisterServer();
    if result.is_err() {
        let _ = DllUnregisterServer();
    }
    result
}
