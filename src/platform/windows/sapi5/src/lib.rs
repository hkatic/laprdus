//! The SAPI5 driver: `laprd64.dll` and `laprd32.dll`.
//!
//! SAPI creates the engine object for a voice token and calls `Speak` with
//! the text as a list of fragments. The installer registers four tokens, one
//! per language; the token sets the language, and the voice is the one of the
//! user's `settings.json` (`voice.default`, empty = Zvonko), which the
//! Configurator edits. The settings and the user's dictionaries are applied
//! by `Engine::apply_user_config()`, as on the other platforms; a changed
//! `settings.json`, `user.json`, `spelling.json`, `emoji.json` or
//! `accents.json` is picked up at the next `Speak`, without a restart of the
//! application.
//!
//! The rate, volume and pitch SAPI's XML asks for (`<rate>`, `<volume>`,
//! `<pitch>`, as NVDA writes them around a capital letter) are applied per
//! fragment, on top of the voice's rate and volume.
//!
//! The library also exports the C API (`laprdus_*`): the NVDA add-on loads it
//! for that.
#![cfg(windows)]
// The linker would have the COM entry points exported PRIVATE, which only
// keeps them out of an import library nobody uses.
#![allow(linker_messages)]

// Linked for its exported functions.
use laprdus_capi as _;
// The build for Windows 7, 8 and 8.1 answers the loader's request for
// combase.dll with ole32.dll: see src/platform/windows/legacy.
#[cfg(target_vendor = "win7")]
use laprdus_win7 as _;

use laprdus::config::{UserConfig, UserSettings};
use laprdus::engine::{Engine, Error};
use laprdus::language::Language;
use laprdus::voice;
use laprdus::{SAMPLE_RATE, silence};
use std::cell::Cell;
use std::ffi::c_void;
use std::ops::ControlFlow;
use std::path::PathBuf;
use std::sync::{Mutex, PoisonError};
use std::time::SystemTime;
use windows::Win32::Foundation::{
    CLASS_E_CLASSNOTAVAILABLE, CLASS_E_NOAGGREGATION, E_FAIL, E_INVALIDARG, E_POINTER, HMODULE, LPARAM, S_FALSE, S_OK,
    WPARAM,
};
use windows::Win32::Media::Audio::{WAVE_FORMAT_PCM, WAVEFORMATEX};
use windows::Win32::Media::Speech::{
    ISpObjectToken, ISpObjectWithToken, ISpObjectWithToken_Impl, ISpTTSEngine, ISpTTSEngine_Impl, ISpTTSEngineSite,
    SPEI_TTS_BOOKMARK, SPET_LPARAM_IS_STRING, SPEVENT, SPVA_Bookmark, SPVA_Silence, SPVA_SpellOut, SPVES_ABORT,
    SPVTEXTFRAG,
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

/// The prosody of a fragment, as SAPI's XML set it (SAPI SDK, `SPVSTATE`):
/// `<rate>` adds `rate` (-10 to 10) to the voice's rate, `<volume>` is
/// `volume` per cent (0-100, 100 without a tag) of the voice's volume, and
/// `<pitch>` moves the pitch by `pitch` (-10 to 10 nominally; NVDA writes up
/// to 25), a semitone per two.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
struct Prosody {
    rate: i32,
    volume: u32,
    pitch: i32,
}

impl Prosody {
    fn of(fragment: &SPVTEXTFRAG) -> Self {
        let state = &fragment.State;
        Self { rate: state.RateAdj, volume: state.Volume.min(100), pitch: state.PitchAdj.MiddleAdj }
    }
}

/// The voice's rate and volume, which a fragment's prosody is relative to.
#[derive(Clone, Copy)]
struct Base {
    rate: i32,
    volume: Option<u16>,
}

/// One stretch of what SAPI asked for.
enum Run {
    Text {
        text: String,
        spell: bool,
        prosody: Prosody,
    },
    Silence(u32),
    /// A bookmark's text, NUL-terminated.
    Bookmark(Vec<u16>),
}

const BLANKS: [char; 4] = [' ', '\t', '\r', '\n'];

/// The fragments as runs. Consecutive fragments of the same kind and
/// prosody stay one run, so a sentence that SAPI cut at a tag is still
/// synthesized as one sentence; a fragment of blanks alone joins the run
/// before it whatever its prosody, and never decides one. A change of
/// prosody (NVDA's raised pitch for a capital letter, a `<rate>` stretch)
/// begins a run of its own. Text is trimmed (SAPI often adds trailing spaces
/// and line breaks) and runs with nothing left are dropped. A bookmark is a
/// run of its own and ends the text before it, so that its event is sent
/// where it stands in the audio.
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
        } else if action == SPVA_Bookmark {
            let mut mark = match frag.pTextStart.is_null() {
                true => Vec::new(),
                false => unsafe { std::slice::from_raw_parts(frag.pTextStart.0, frag.ulTextLen as usize) }.to_vec(),
            };
            mark.push(0);
            runs.push(Run::Bookmark(mark));
        } else if !frag.pTextStart.is_null() && frag.ulTextLen > 0 {
            // Only a spell-out fragment is spelled; pronunciation hints and
            // unknown actions are spoken as text.
            let spell = action == SPVA_SpellOut;
            let prosody = Prosody::of(frag);
            let piece = String::from_utf16_lossy(unsafe {
                std::slice::from_raw_parts(frag.pTextStart.0, frag.ulTextLen as usize)
            });
            let blank = piece.trim_matches(BLANKS).is_empty();
            let joined = match runs.last_mut() {
                Some(Run::Text { text, spell: spelled, prosody: last }) if *spelled == spell => {
                    if *last != prosody && !blank && text.trim_matches(BLANKS).is_empty() {
                        // Blanks so far: the text decides the run's prosody
                        *last = prosody;
                    }
                    let joins = *last == prosody || blank;
                    if joins {
                        text.push_str(&piece);
                    }
                    joins
                }
                _ => false,
            };
            if !joined {
                runs.push(Run::Text { text: piece, spell, prosody });
            }
        }
        fragment = frag.pNext;
    }
    runs.into_iter()
        .filter_map(|run| match run {
            Run::Text { text, spell, prosody } => {
                let text = text.trim_matches(BLANKS);
                (!text.is_empty()).then(|| Run::Text { text: text.to_owned(), spell, prosody })
            }
            other => Some(other),
        })
        .collect()
}

fn should_stop(site: &ISpTTSEngineSite) -> bool {
    // SAFETY: a plain call on a valid interface.
    unsafe { site.GetActions() & SPVES_ABORT.0 as u32 != 0 }
}

/// The number a bookmark's text begins with, as C's `_wtol` reads it: what a
/// host gets as the bookmark's id.
fn bookmark_id(mark: &[u16]) -> i32 {
    let text = String::from_utf16_lossy(mark);
    let text = text.trim_start();
    let digits = text.strip_prefix(['-', '+']).unwrap_or(text);
    let end = digits.find(|c: char| !c.is_ascii_digit()).unwrap_or(digits.len());
    let value = digits[..end].parse::<i64>().map_or(0, |value| value.min(i64::from(i32::MAX)) as i32);
    if text.starts_with('-') { -value } else { value }
}

/// Tells SAPI that the bookmark was reached after `offset` bytes of this
/// call's audio. A screen reader waits for this event before it sends its
/// next utterance (NVDA 2023.3, the last for Windows 7, has nothing else to
/// go by: without it only the first utterance of several was spoken).
fn bookmark_reached(site: &ISpTTSEngineSite, mark: &[u16], offset: u64) {
    let event = SPEVENT {
        // The event's id in the low 16 bits, the kind of `lParam` above them.
        _bitfield: SPEI_TTS_BOOKMARK.0 | (SPET_LPARAM_IS_STRING.0 << 16),
        ulStreamNum: 0,
        ullAudioStreamOffset: offset,
        wParam: WPARAM(bookmark_id(mark) as isize as usize),
        lParam: LPARAM(mark.as_ptr() as isize),
    };
    // SAFETY: one event, whose string is NUL-terminated and outlives the
    // call; SAPI copies it. An event SAPI does not take costs no speech.
    let _ = unsafe { site.AddEvents(&event, 1) };
}

/// The engine of one voice token with the user's settings.
struct Voice {
    engine: Engine,
    settings: UserSettings,
    /// The user's configuration directory ...
    config: UserConfig,
    /// ... and when its files were last changed, as the engine read them.
    times: [Option<SystemTime>; 5],
}

impl Voice {
    /// The engine for a token: the token's language, the voice of the token
    /// or of the user's settings, the bundled dictionaries from the install
    /// directory and the user's settings and dictionaries on top.
    fn new(token: &ISpObjectToken) -> Option<Self> {
        let mut config = UserConfig::new();
        let loaded = config.load_settings();
        let times = config.modified_times();
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
        // The settings with their pauses, and the user's dictionaries, as
        // every platform applies them
        if loaded {
            engine.apply_user_config(&config);
        }
        Some(Self { engine, settings: if loaded { settings } else { UserSettings::default() }, config, times })
    }

    /// Whether the user changed the settings or a dictionary since the
    /// engine read them.
    fn outdated(&self) -> bool {
        self.config.modified_times() != self.times
    }

    /// The voice's rate and volume for this call.
    fn base(site: &ISpTTSEngineSite) -> Base {
        // SAFETY: plain calls on a valid interface.
        unsafe { Base { rate: site.GetRate().unwrap_or(0), volume: site.GetVolume().ok() } }
    }

    /// The rate, pitch and volume of a run. The user's rate and pitch are
    /// the voice's normal and SAPI's are relative to it, so they multiply;
    /// the XML's rate adds to the voice's, its volume is a share of the
    /// voice's; the force switches mean "ignore SAPI".
    fn apply_prosody(&mut self, base: Base, prosody: Prosody) {
        let settings = &self.settings;
        let (mut speed, mut user_pitch, mut volume) = (settings.speed, settings.user_pitch, settings.volume);
        let rate = base.rate.saturating_add(prosody.rate).clamp(-10, 10);
        if !settings.force_speed && rate != 0 {
            speed *= 2f32.powf(rate as f32 / 10.0).clamp(0.5, 2.0);
        }
        if !settings.force_volume {
            let voice = base.volume.map_or(1.0, |volume| (f32::from(volume) / 100.0).clamp(0.0, 1.0));
            // Exactly the voice's volume without a <volume> tag
            volume = voice * (prosody.volume as f32 / 100.0);
        }
        if !settings.force_pitch {
            user_pitch *= 2f32.powf(prosody.pitch as f32 / 24.0).clamp(0.5, 2.0);
        }
        self.engine.update_params(|params| {
            params.speed = speed;
            params.user_pitch = user_pitch;
            params.volume = volume;
        });
    }

    /// Speaks the runs to the site, each clause as soon as it is rendered,
    /// so the first sound of a long text does not wait for the rest, and
    /// stops when SAPI asks (`SPVES_ABORT`): the synthesis under way is cut
    /// short, where it used to finish the whole text before `Speak` returned
    /// and the next utterance could start. Each text run gets its own rate,
    /// volume and pitch before it is synthesized. A bookmark is reported with
    /// the number of bytes written before it. An error when none of the runs
    /// could be spoken, or when the site refused the audio.
    fn speak(&mut self, runs: &[Run], site: &ISpTTSEngineSite) -> Result<()> {
        let base = Self::base(site);
        // The prosody the engine has now, once a run has set it
        let mut applied = None;
        let spelling_pause = self.engine.params().pause_settings.spelling_pause_ms;
        let text_runs = runs.iter().filter(|run| matches!(run, Run::Text { .. })).count();
        let (mut succeeded, mut failed) = (false, false);
        // True while the audio so far ends in a pause (or there is none yet).
        let mut separated = true;
        let mut previous_spelled = false;
        // Why the site took no more audio, when it refused some.
        let mut refused = None;
        // The bytes of audio the site has taken.
        let written = Cell::new(0u64);
        let mut write = |samples: &[i16]| {
            for chunk in samples.chunks(CHUNK_SAMPLES) {
                if should_stop(site) {
                    return ControlFlow::Break(());
                }
                // SAFETY: the pointer and the byte count describe `chunk`.
                if let Err(error) = unsafe { site.Write(chunk.as_ptr().cast(), size_of_val(chunk) as u32) } {
                    refused = Some(error);
                    return ControlFlow::Break(());
                }
                written.set(written.get() + size_of_val(chunk) as u64);
            }
            ControlFlow::Continue(())
        };
        for run in runs {
            if should_stop(site) {
                break;
            }
            let (text, spell) = match run {
                Run::Silence(ms) => {
                    if write(&silence(*ms).collect::<Vec<_>>()).is_break() {
                        break;
                    }
                    separated = true;
                    continue;
                }
                Run::Bookmark(mark) => {
                    bookmark_reached(site, mark, written.get());
                    continue;
                }
                Run::Text { text, spell, prosody } => {
                    if applied != Some(*prosody) {
                        self.apply_prosody(base, *prosody);
                        applied = Some(*prosody);
                    }
                    (text, *spell)
                }
            };
            // Many SAPI5 applications spell by sending single characters as
            // ordinary text, so an utterance that is one character is
            // spelled too.
            let spell = spell || (text_runs == 1 && text.chars().count() == 1);
            // Spelled characters are kept apart by the spelling pause; so is
            // the spoken text next to them. The pause goes out with the
            // run's first sound: a run that cannot be spoken gets none.
            let mut pause_owed = !separated && spell != previous_spelled;
            let mut speak = |audio: &[i16]| {
                if std::mem::take(&mut pause_owed) {
                    write(&silence(spelling_pause).collect::<Vec<_>>())?;
                }
                write(audio)
            };
            let spoken = if spell {
                self.engine.synthesize_spelled(text, true).map(|(audio, _)| speak(&audio).is_break())
            } else {
                self.engine.synthesize_streaming(text, 0, &mut speak).map(|()| false)
            };
            match spoken {
                // SAPI asked to stop, or the site refused the audio.
                Ok(true) | Err(Error::Cancelled) => break,
                Ok(false) => succeeded = true,
                // One run that cannot be spoken must not silence the others.
                Err(_) => {
                    failed = true;
                    continue;
                }
            }
            // The engine ends every spelled run with the spelling pause.
            separated = spell;
            previous_spelled = spell;
        }
        match refused {
            Some(error) => Err(error),
            None if succeeded || !failed => Ok(()),
            None => Err(E_FAIL.into()),
        }
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
        // A changed settings.json or user dictionary: the engine is made
        // again; the old one stays when that fails.
        if let Some(current) = voice.as_ref()
            && current.outdated()
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
        let runs = unsafe { runs_of(fragments) };
        voice.speak(&runs, site)
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
