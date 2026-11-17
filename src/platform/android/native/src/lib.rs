//! The native half of `com.hrvojekatic.laprdus.tts.LaprdusTTS`.
//!
//! The app and its TTS service each own an engine: `nativeCreate` returns a
//! handle, every other call takes it, and there is no global engine. A handle
//! is a number that names an engine in a table, never a pointer, so a stale
//! or wrong handle finds nothing instead of touching memory. Voice data and
//! dictionaries are read from the APK's assets through the `AssetManager`. A
//! panic is caught and gives the call's failure value: nothing here may take
//! the process of a screen reader's voice down.

#![allow(non_snake_case)]

use engine::engine::Engine;
use engine::language::Language;
use engine::params::{NumberMode, PauseSettings, SpellingMode, VoiceParams};
use engine::text::frontend::UserLexicon;
use engine::voice::{self, VOICES};
use jni::JNIEnv;
use jni::objects::{JByteArray, JObject, JString, JValue};
use jni::sys::{jboolean, jfloat, jint, jlong, jobject, jshortArray, jstring};
use std::collections::HashMap;
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::sync::atomic::{AtomicI64, Ordering};
use std::sync::{Arc, LazyLock, Mutex, PoisonError};

const TAG: &str = "LaprdusTTS";

type Shared = Arc<Mutex<Engine>>;

static ENGINES: LazyLock<Mutex<HashMap<jlong, Shared>>> = LazyLock::new(Mutex::default);
static NEXT_HANDLE: AtomicI64 = AtomicI64::new(1);

fn engine_of(handle: jlong) -> Option<Shared> {
    ENGINES.lock().unwrap_or_else(PoisonError::into_inner).get(&handle).cloned()
}

/// Runs `call` on the engine behind a handle; `failure` for an unknown
/// handle or a panic.
fn with<T>(handle: jlong, failure: T, call: impl FnOnce(&mut Engine) -> T) -> T {
    let Some(engine) = engine_of(handle) else { return failure };
    let result = catch_unwind(AssertUnwindSafe(|| call(&mut engine.lock().unwrap_or_else(PoisonError::into_inner))));
    result.unwrap_or(failure)
}

/// As [`with`], for an engine that has a voice.
fn with_voice<T>(handle: jlong, failure: T, call: impl FnOnce(&mut Engine) -> T) -> T {
    with(handle, None, |engine| engine.is_initialized().then(|| call(engine))).unwrap_or(failure)
}

fn change(handle: jlong, change: impl FnOnce(&mut VoiceParams)) {
    with(handle, (), |engine| engine.update_params(change));
}

fn pauses(handle: jlong, change: impl FnOnce(&mut PauseSettings)) {
    with(handle, (), |engine| engine.update_pauses(change));
}

fn text(env: &mut JNIEnv, string: &JString) -> String {
    if string.is_null() { String::new() } else { env.get_string(string).map(Into::into).unwrap_or_default() }
}

fn java_string(env: &mut JNIEnv, text: &str) -> jstring {
    env.new_string(text).map_or(std::ptr::null_mut(), JString::into_raw)
}

fn java_samples(env: &mut JNIEnv, samples: &[i16]) -> jshortArray {
    let array = env.new_short_array(samples.len() as i32).and_then(|array| {
        env.set_short_array_region(&array, 0, samples)?;
        Ok(array)
    });
    array.map_or(std::ptr::null_mut(), |array| array.into_raw())
}

fn log_error(env: &mut JNIEnv, message: &str) {
    let (Ok(tag), Ok(message)) = (env.new_string(TAG), env.new_string(message)) else { return };
    let arguments = [JValue::Object(&tag), JValue::Object(&message)];
    // Logging must never be the reason a call fails.
    if env.call_static_method("android/util/Log", "e", "(Ljava/lang/String;Ljava/lang/String;)I", &arguments).is_err() {
        let _ = env.exception_clear();
    }
}

/// The bytes of an asset, read through `AssetManager.open()`.
fn asset(env: &mut JNIEnv, manager: &JObject, path: &str) -> Option<Vec<u8>> {
    let read = |env: &mut JNIEnv| -> jni::errors::Result<Vec<u8>> {
        let name = env.new_string(path)?;
        let stream = env
            .call_method(manager, "open", "(Ljava/lang/String;)Ljava/io/InputStream;", &[JValue::Object(&name)])?
            .l()?;
        let buffer: JByteArray = env.new_byte_array(64 * 1024)?;
        let mut bytes = Vec::new();
        loop {
            let count = env.call_method(&stream, "read", "([B)I", &[JValue::Object(&buffer)])?.i()?;
            if count < 0 {
                break;
            }
            let mut chunk = vec![0i8; count as usize];
            env.get_byte_array_region(&buffer, 0, &mut chunk)?;
            bytes.extend(chunk.into_iter().map(|byte| byte as u8));
        }
        env.call_method(&stream, "close", "()V", &[])?;
        Ok(bytes)
    };
    match read(env) {
        Ok(bytes) if !bytes.is_empty() => Some(bytes),
        _ => {
            // A missing asset is an answer, not a pending Java exception.
            let _ = env.exception_clear();
            None
        }
    }
}

fn asset_text(env: &mut JNIEnv, manager: &JObject, path: &JString) -> Option<String> {
    let path = text(env, path);
    asset(env, manager, &path).map(|bytes| String::from_utf8_lossy(&bytes).into_owned())
}

/// Loads a dictionary asset into an engine that has a voice.
fn load_asset(
    mut env: JNIEnv,
    handle: jlong,
    manager: JObject,
    path: JString,
    load: impl FnOnce(&mut Engine, &str) -> bool,
) -> jboolean {
    let Some(json) = asset_text(&mut env, &manager, &path) else { return 0 };
    jboolean::from(with_voice(handle, false, |engine| load(engine, &json)))
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeGetVersion(
    mut env: JNIEnv,
    _class: JObject,
) -> jstring {
    java_string(&mut env, Engine::version())
}

/// The reason the first rejected entry of an accent lexicon was rejected, or
/// null when every entry is accepted.
#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeCheckAccentLexicon(
    mut env: JNIEnv,
    _class: JObject,
    json: JString,
) -> jstring {
    let json = text(&mut env, &json);
    match catch_unwind(|| UserLexicon::parse(&json).1) {
        Ok(report) if report.rejected > 0 => java_string(&mut env, &report.first_error),
        _ => std::ptr::null_mut(),
    }
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeCreate(_env: JNIEnv, _this: JObject) -> jlong {
    let handle = NEXT_HANDLE.fetch_add(1, Ordering::Relaxed);
    ENGINES.lock().unwrap_or_else(PoisonError::into_inner).insert(handle, Shared::default());
    handle
}

/// Back to an engine without a voice; the handle stays valid.
#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeShutdown(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
) {
    with(handle, (), |engine| *engine = Engine::new());
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeIsInitialized(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
) -> jboolean {
    jboolean::from(with(handle, false, |engine| engine.is_initialized()))
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSynthesize(
    mut env: JNIEnv,
    _this: JObject,
    handle: jlong,
    text_in: JString,
) -> jshortArray {
    let text = text(&mut env, &text_in);
    let failure = || Err(("java/lang/RuntimeException", "Synthesis failed".to_owned()));
    let audio = with(handle, failure(), |engine| {
        if !engine.is_initialized() {
            return Err(("java/lang/IllegalStateException", "Engine not initialized".to_owned()));
        }
        engine.synthesize(&text).map_err(|error| ("java/lang/RuntimeException", error.to_string()))
    });
    match audio {
        Ok(audio) => java_samples(&mut env, &audio),
        Err((class, message)) => {
            log_error(&mut env, &message);
            let _ = env.throw_new(class, message);
            std::ptr::null_mut()
        }
    }
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSynthesizeSpelled(
    mut env: JNIEnv,
    _this: JObject,
    handle: jlong,
    text_in: JString,
) -> jshortArray {
    let text = text(&mut env, &text_in);
    match with_voice(handle, None, |engine| engine.synthesize_spelled(&text, true).ok()) {
        Some((audio, _)) => java_samples(&mut env, &audio),
        None => std::ptr::null_mut(),
    }
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetSpeed(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
    speed: jfloat,
) {
    change(handle, |params| params.speed = speed);
}

/// The voice character pitch as it is, in place of the voice's base pitch.
#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetPitch(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
    pitch: jfloat,
) {
    change(handle, |params| params.pitch = pitch);
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetUserPitch(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
    pitch: jfloat,
) {
    change(handle, |params| params.user_pitch = pitch);
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetVolume(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
    volume: jfloat,
) {
    change(handle, |params| params.volume = volume);
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetVolumeBoost(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
    enabled: jboolean,
) {
    change(handle, |params| params.volume_boost = enabled != 0);
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetInflectionLevel(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
    level: jfloat,
) {
    change(handle, |params| params.inflection_level = level);
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetAcceleration(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
    acceleration: jfloat,
) {
    change(handle, |params| params.acceleration = acceleration);
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetHeadSize(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
    value: jint,
) {
    change(handle, |params| params.head_size = value);
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetBreathiness(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
    value: jint,
) {
    change(handle, |params| params.breathiness = value);
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetRoughness(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
    value: jint,
) {
    change(handle, |params| params.roughness = value);
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetBrightness(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
    value: jint,
) {
    change(handle, |params| params.brightness = value);
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeGetNominalWpm(
    mut env: JNIEnv,
    _this: JObject,
    voice_id: JString,
) -> jfloat {
    voice::find(&text(&mut env, &voice_id)).map_or(0.0, |voice| voice.nominal_wpm())
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeGetSampleRate(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
) -> jint {
    with(handle, 0, |engine| engine.sample_rate() as jint)
}

/// Synthesis is synchronous: there is nothing to cancel.
#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeCancel(_env: JNIEnv, _this: JObject) {}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeGetVoiceCount(
    _env: JNIEnv,
    _this: JObject,
) -> jint {
    VOICES.len() as jint
}

/// A `VoiceInfo(id, displayName, languageCode, gender, age, basePitch,
/// baseVoiceId, isFormant)`, or null for an index past the list.
#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeGetVoiceInfo(
    mut env: JNIEnv,
    _this: JObject,
    index: jint,
) -> jobject {
    let Some(voice) = usize::try_from(index).ok().and_then(|index| VOICES.get(index)) else {
        return std::ptr::null_mut();
    };
    let build = |env: &mut JNIEnv| -> jni::errors::Result<jobject> {
        fn string<'a>(env: &mut JNIEnv<'a>, text: &str) -> jni::errors::Result<JObject<'a>> {
            env.new_string(text).map(JObject::from)
        }
        let id = string(env, voice.id)?;
        let name = string(env, voice.display_name)?;
        let language = string(env, voice.language.code())?;
        let gender = string(env, voice.gender.as_str())?;
        let age = string(env, voice.age.as_str())?;
        let base = match voice.base_voice_id {
            Some(base) => string(env, base)?,
            None => JObject::null(),
        };
        let arguments = [
            JValue::Object(&id),
            JValue::Object(&name),
            JValue::Object(&language),
            JValue::Object(&gender),
            JValue::Object(&age),
            JValue::Float(voice.base_pitch),
            JValue::Object(&base),
            JValue::Bool(jboolean::from(voice.is_formant())),
        ];
        let signature = "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;FLjava/lang/String;Z)V";
        Ok(env.new_object("com/hrvojekatic/laprdus/tts/VoiceInfo", signature, &arguments)?.into_raw())
    };
    build(&mut env).unwrap_or(std::ptr::null_mut())
}

/// Switches the voice; a recorded voice is read from the asset
/// `voices/<file>`. Returns the voice's canonical id, or null when it could
/// not be set.
#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetVoice(
    mut env: JNIEnv,
    _this: JObject,
    handle: jlong,
    voice_id: JString,
    asset_manager: JObject,
) -> jstring {
    let id = text(&mut env, &voice_id);
    let Some(voice) = voice::find(&id) else {
        log_error(&mut env, &format!("Voice not found: {id}"));
        return std::ptr::null_mut();
    };
    // The asset is read before the engine is locked: reading calls into Java.
    let data = voice.data_file().and_then(|file| asset(&mut env, &asset_manager, &format!("voices/{file}")));
    match with(handle, None, |engine| Some(engine.set_voice_from(voice.id, |_| data))) {
        Some(Ok(())) => java_string(&mut env, voice.id),
        Some(Err(error)) => {
            log_error(&mut env, &error.to_string());
            std::ptr::null_mut()
        }
        None => std::ptr::null_mut(),
    }
}

/// Chooses the language; null or empty goes back to the voice's own. False
/// for a language the engine does not speak.
#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetLanguage(
    mut env: JNIEnv,
    _this: JObject,
    handle: jlong,
    tag: JString,
) -> jboolean {
    let tag = text(&mut env, &tag);
    let language = if tag.is_empty() { None } else { Language::parse(&tag) };
    if language.is_none() && !tag.is_empty() {
        log_error(&mut env, &format!("Unknown language: {tag}"));
        return 0;
    }
    jboolean::from(with(handle, false, |engine| {
        engine.set_language(language);
        true
    }))
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeGetLanguage(
    mut env: JNIEnv,
    _this: JObject,
    handle: jlong,
) -> jstring {
    let language = with(handle, Language::default(), |engine| engine.language());
    java_string(&mut env, language.code())
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeLoadDictionaryFromAssets(
    env: JNIEnv,
    _this: JObject,
    handle: jlong,
    asset_manager: JObject,
    asset_path: JString,
) -> jboolean {
    load_asset(env, handle, asset_manager, asset_path, |engine, json| engine.dictionary().load(json))
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeLoadSpellingDictionaryFromAssets(
    env: JNIEnv,
    _this: JObject,
    handle: jlong,
    asset_manager: JObject,
    asset_path: JString,
) -> jboolean {
    load_asset(env, handle, asset_manager, asset_path, |engine, json| engine.load_spelling_dictionary(json))
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeLoadEmojiDictionaryFromAssets(
    env: JNIEnv,
    _this: JObject,
    handle: jlong,
    asset_manager: JObject,
    asset_path: JString,
) -> jboolean {
    load_asset(env, handle, asset_manager, asset_path, |engine, json| engine.load_emoji_dictionary(json))
}

/// The emoji names of one language, used while it is spoken.
#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeLoadLanguageEmojiDictionaryFromAssets(
    mut env: JNIEnv,
    _this: JObject,
    handle: jlong,
    asset_manager: JObject,
    language_tag: JString,
    asset_path: JString,
) -> jboolean {
    let Some(language) = Language::parse(&text(&mut env, &language_tag)) else { return 0 };
    load_asset(env, handle, asset_manager, asset_path, |engine, json| {
        engine.load_language_emoji_dictionary(language, json)
    })
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeAddPronunciation(
    mut env: JNIEnv,
    _this: JObject,
    handle: jlong,
    grapheme: JString,
    phoneme: JString,
    case_sensitive: jboolean,
    whole_word: jboolean,
) {
    let (grapheme, phoneme) = (text(&mut env, &grapheme), text(&mut env, &phoneme));
    with_voice(handle, (), |engine| {
        engine.add_pronunciation(&grapheme, &phoneme, case_sensitive != 0, whole_word != 0)
    });
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeAddSpellingEntry(
    mut env: JNIEnv,
    _this: JObject,
    handle: jlong,
    character: JString,
    pronunciation: JString,
) {
    let (character, pronunciation) = (text(&mut env, &character), text(&mut env, &pronunciation));
    with_voice(handle, (), |engine| engine.add_spelling_entry(&character, &pronunciation));
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeAddEmojiEntry(
    mut env: JNIEnv,
    _this: JObject,
    handle: jlong,
    emoji: JString,
    spoken: JString,
) {
    let (emoji, spoken) = (text(&mut env, &emoji), text(&mut env, &spoken));
    with_voice(handle, (), |engine| engine.add_emoji_entry(&emoji, &spoken));
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeLoadAccentLexicon(
    mut env: JNIEnv,
    _this: JObject,
    handle: jlong,
    json: JString,
) -> jboolean {
    let json = text(&mut env, &json);
    jboolean::from(with_voice(handle, false, |engine| engine.load_accent_lexicon(&json)))
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeClearAccentLexicon(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
) {
    with(handle, (), |engine| engine.clear_accent_lexicon());
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetEmojiEnabled(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
    enabled: jboolean,
) {
    change(handle, |params| params.emoji_enabled = enabled != 0);
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeIsEmojiEnabled(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
) -> jboolean {
    jboolean::from(with(handle, false, |engine| engine.params().emoji_enabled))
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetSentencePause(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
    pause_ms: jint,
) {
    pauses(handle, |pauses| pauses.sentence_pause_ms = pause_ms.max(0) as u32);
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetCommaPause(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
    pause_ms: jint,
) {
    pauses(handle, |pauses| pauses.comma_pause_ms = pause_ms.max(0) as u32);
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetNewlinePause(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
    pause_ms: jint,
) {
    pauses(handle, |pauses| pauses.newline_pause_ms = pause_ms.max(0) as u32);
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetSpellingPause(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
    pause_ms: jint,
) {
    with(handle, (), |engine| engine.set_spelling_pause(pause_ms.max(0) as u32));
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeGetSentencePause(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
) -> jint {
    with(handle, 100, |engine| engine.params().pause_settings.sentence_pause_ms as jint)
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeGetCommaPause(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
) -> jint {
    with(handle, 100, |engine| engine.params().pause_settings.comma_pause_ms as jint)
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeGetNewlinePause(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
) -> jint {
    with(handle, 100, |engine| engine.params().pause_settings.newline_pause_ms as jint)
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeGetSpellingPause(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
) -> jint {
    with(handle, 200, |engine| engine.params().pause_settings.spelling_pause_ms as jint)
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetSpellingSpeed(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
    percent: jint,
) {
    change(handle, |params| params.spelling_speed = percent);
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeGetSpellingSpeed(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
) -> jint {
    with(handle, 50, |engine| engine.params().spelling_speed)
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetLetterSounds(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
    enabled: jboolean,
) {
    change(handle, |params| {
        params.spelling_mode = if enabled != 0 { SpellingMode::LetterSounds } else { SpellingMode::LetterNames }
    });
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeGetLetterSounds(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
) -> jboolean {
    jboolean::from(with(handle, true, |engine| engine.params().spelling_mode == SpellingMode::LetterSounds))
}

/// 0 whole numbers, 1 digit by digit.
#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetNumberMode(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
    mode: jint,
) {
    change(handle, |params| {
        params.number_mode = if mode == 1 { NumberMode::DigitByDigit } else { NumberMode::WholeNumbers }
    });
}

#[unsafe(no_mangle)]
pub extern "system" fn Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeGetNumberMode(
    _env: JNIEnv,
    _this: JObject,
    handle: jlong,
) -> jint {
    with(handle, 0, |engine| jint::from(engine.params().number_mode == NumberMode::DigitByDigit))
}
