// -*- coding: utf-8 -*-
// jni_bridge.cpp - Android JNI bridge for LaprdusTTS

#ifdef ANDROID

#include <jni.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <string>
#include <memory>
#include <mutex>

#include "../../core/tts_engine.hpp"
#include "../../core/voice_registry.hpp"

#define LOG_TAG "LaprdusTTS"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

// Global engine instance with mutex for thread safety
std::unique_ptr<laprdus::TTSEngine> g_engine;
std::mutex g_engine_mutex;

// Current voice data directory
std::string g_data_directory;

// Store the current voice's base_pitch to preserve it across parameter changes
// This ensures derived voices (detence=1.5, baba=1.2, djedo=0.75) maintain their
// character pitch even when Android TTS requests pitch changes
float g_voice_base_pitch = 1.0f;

// Convert Java string to std::string
std::string jstringToString(JNIEnv* env, jstring jstr) {
    if (!jstr) return {};

    const char* chars = env->GetStringUTFChars(jstr, nullptr);
    if (!chars) return {};

    std::string result(chars);
    env->ReleaseStringUTFChars(jstr, chars);
    return result;
}

// Helper to throw Java exception
void throwException(JNIEnv* env, const char* className, const char* message) {
    jclass exClass = env->FindClass(className);
    if (exClass) {
        env->ThrowNew(exClass, message);
    }
}

// Create VoiceInfo Java object from VoiceDefinition
jobject createVoiceInfoObject(JNIEnv* env, const laprdus::VoiceDefinition* voice) {
    if (!voice) return nullptr;

    jclass voiceInfoClass = env->FindClass("com/hrvojekatic/laprdus/tts/VoiceInfo");
    if (!voiceInfoClass) {
        LOGE("Failed to find VoiceInfo class");
        return nullptr;
    }

    jmethodID constructor = env->GetMethodID(voiceInfoClass, "<init>",
        "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;F)V");
    if (!constructor) {
        LOGE("Failed to find VoiceInfo constructor");
        return nullptr;
    }

    jstring id = env->NewStringUTF(voice->id);
    jstring displayName = env->NewStringUTF(voice->display_name);
    jstring languageCode = env->NewStringUTF(laprdus::voice_language_code(voice->language));
    jstring gender = env->NewStringUTF(laprdus::voice_gender_string(voice->gender));
    jstring age = env->NewStringUTF(laprdus::voice_age_string(voice->age));
    jfloat basePitch = voice->base_pitch;

    jobject voiceInfo = env->NewObject(voiceInfoClass, constructor,
        id, displayName, languageCode, gender, age, basePitch);

    env->DeleteLocalRef(id);
    env->DeleteLocalRef(displayName);
    env->DeleteLocalRef(languageCode);
    env->DeleteLocalRef(gender);
    env->DeleteLocalRef(age);

    return voiceInfo;
}

} // anonymous namespace

extern "C" {

// =============================================================================
// Native Methods - Package: com.hrvojekatic.laprdus.tts
// =============================================================================

JNIEXPORT jboolean JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeInit(
    JNIEnv* env,
    jobject thiz,
    jstring phonemeDataPath) {

    (void)thiz;
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    LOGI("Initializing LaprdusTTS native engine");

    try {
        std::string path = jstringToString(env, phonemeDataPath);
        if (path.empty()) {
            LOGE("Invalid phoneme data path");
            return JNI_FALSE;
        }

        g_engine = std::make_unique<laprdus::TTSEngine>();

        if (!g_engine->initialize(path)) {
            LOGE("Failed to initialize engine from path: %s", path.c_str());
            g_engine.reset();
            return JNI_FALSE;
        }

        // Store data directory
        size_t lastSlash = path.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            g_data_directory = path.substr(0, lastSlash);
        }

        LOGI("Engine initialized successfully");
        return JNI_TRUE;

    } catch (const std::exception& e) {
        LOGE("Exception during init: %s", e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jboolean JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeInitFromAssets(
    JNIEnv* env,
    jobject thiz,
    jobject assetManager,
    jstring assetPath) {

    (void)thiz;
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    LOGI("Initializing LaprdusTTS from assets");

    try {
        // Get asset manager
        AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
        if (!mgr) {
            LOGE("Failed to get asset manager");
            return JNI_FALSE;
        }

        std::string path = jstringToString(env, assetPath);
        if (path.empty()) {
            LOGE("Invalid asset path");
            return JNI_FALSE;
        }

        // Open asset
        AAsset* asset = AAssetManager_open(mgr, path.c_str(), AASSET_MODE_BUFFER);
        if (!asset) {
            LOGE("Failed to open asset: %s", path.c_str());
            return JNI_FALSE;
        }

        // Get data
        size_t size = AAsset_getLength(asset);
        const uint8_t* data = static_cast<const uint8_t*>(AAsset_getBuffer(asset));

        if (!data || size == 0) {
            AAsset_close(asset);
            LOGE("Asset is empty");
            return JNI_FALSE;
        }

        // Initialize engine
        g_engine = std::make_unique<laprdus::TTSEngine>();

        bool success = g_engine->initialize_from_memory(data, size, {});

        AAsset_close(asset);

        if (!success) {
            LOGE("Failed to initialize engine from asset");
            g_engine.reset();
            return JNI_FALSE;
        }

        LOGI("Engine initialized from assets successfully");
        return JNI_TRUE;

    } catch (const std::exception& e) {
        LOGE("Exception during asset init: %s", e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT void JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeShutdown(
    JNIEnv* env,
    jobject thiz) {

    (void)env;
    (void)thiz;
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    LOGI("Shutting down LaprdusTTS native engine");
    g_engine.reset();
}

JNIEXPORT jboolean JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeIsInitialized(
    JNIEnv* env,
    jobject thiz) {

    (void)env;
    (void)thiz;
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    return (g_engine && g_engine->is_initialized()) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jshortArray JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSynthesize(
    JNIEnv* env,
    jobject thiz,
    jstring text) {

    (void)thiz;
    std::lock_guard<std::mutex> lock(g_engine_mutex);

    if (!g_engine || !g_engine->is_initialized()) {
        throwException(env, "java/lang/IllegalStateException", "Engine not initialized");
        return nullptr;
    }

    std::string utf8Text = jstringToString(env, text);
    if (utf8Text.empty()) {
        // Return empty array for empty text
        return env->NewShortArray(0);
    }

    try {
        laprdus::SynthesisResult result = g_engine->synthesize(utf8Text);

        if (!result.success) {
            LOGE("Synthesis failed: %s", result.error_message.c_str());
            throwException(env, "java/lang/RuntimeException", result.error_message.c_str());
            return nullptr;
        }

        // Create Java short array
        size_t numSamples = result.audio.samples.size();
        jshortArray jsamples = env->NewShortArray(static_cast<jsize>(numSamples));

        if (!jsamples) {
            throwException(env, "java/lang/OutOfMemoryError", "Failed to allocate audio buffer");
            return nullptr;
        }

        // Copy samples to Java array (only if we have samples to avoid null pointer)
        if (numSamples > 0) {
            env->SetShortArrayRegion(jsamples, 0, static_cast<jsize>(numSamples),
                                    reinterpret_cast<const jshort*>(result.audio.samples.data()));
        }

        return jsamples;

    } catch (const std::exception& e) {
        LOGE("Exception during synthesis: %s", e.what());
        throwException(env, "java/lang/RuntimeException", e.what());
        return nullptr;
    }
}

JNIEXPORT void JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetSpeed(
    JNIEnv* env,
    jobject thiz,
    jfloat speed) {

    (void)env;
    (void)thiz;
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (!g_engine) return;

    laprdus::VoiceParams params = g_engine->voice_params();
    params.speed = speed;
    g_engine->set_voice_params(params);
}

JNIEXPORT void JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetPitch(
    JNIEnv* env,
    jobject thiz,
    jfloat pitch) {

    (void)env;
    (void)thiz;
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (!g_engine) return;

    laprdus::VoiceParams params = g_engine->voice_params();
    params.pitch = pitch;
    g_engine->set_voice_params(params);
}

JNIEXPORT void JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetUserPitch(
    JNIEnv* env,
    jobject thiz,
    jfloat pitch) {

    (void)env;
    (void)thiz;
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (!g_engine) return;

    laprdus::VoiceParams params = g_engine->voice_params();
    params.user_pitch = pitch;
    g_engine->set_voice_params(params);
}

JNIEXPORT void JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetVolume(
    JNIEnv* env,
    jobject thiz,
    jfloat volume) {

    (void)env;
    (void)thiz;
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (!g_engine) return;

    laprdus::VoiceParams params = g_engine->voice_params();
    params.volume = volume;
    g_engine->set_voice_params(params);
}

JNIEXPORT void JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetInflectionEnabled(
    JNIEnv* env,
    jobject thiz,
    jboolean enabled) {

    (void)env;
    (void)thiz;
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (!g_engine) return;

    laprdus::VoiceParams params = g_engine->voice_params();
    params.inflection_enabled = enabled;
    g_engine->set_voice_params(params);
}

JNIEXPORT jint JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeGetSampleRate(
    JNIEnv* env,
    jobject thiz) {

    (void)env;
    (void)thiz;
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (!g_engine) return 0;
    return static_cast<jint>(g_engine->sample_rate());
}

JNIEXPORT jstring JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeGetVersion(
    JNIEnv* env,
    jclass clazz) {

    (void)clazz;
    return env->NewStringUTF(laprdus::TTSEngine::version());
}

JNIEXPORT void JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeCancel(
    JNIEnv* env,
    jobject thiz) {

    // Note: TTSEngine doesn't have a cancel method yet.
    // This is a placeholder for future implementation.
    // For now, synthesis is synchronous and cannot be cancelled mid-operation.
    (void)env;
    (void)thiz;
    LOGI("Cancel requested (no-op)");
}

// =============================================================================
// Voice Selection Methods
// =============================================================================

JNIEXPORT jint JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeGetVoiceCount(
    JNIEnv* env,
    jobject thiz) {

    (void)env;
    (void)thiz;
    return static_cast<jint>(laprdus::VoiceRegistry::voice_count());
}

JNIEXPORT jobject JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeGetVoiceInfo(
    JNIEnv* env,
    jobject thiz,
    jint index) {

    (void)thiz;
    const laprdus::VoiceDefinition* voice = laprdus::VoiceRegistry::get_by_index(
        static_cast<size_t>(index));

    return createVoiceInfoObject(env, voice);
}

JNIEXPORT jboolean JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetVoice(
    JNIEnv* env,
    jobject thiz,
    jstring voiceId,
    jobject assetManager) {

    (void)thiz;
    std::lock_guard<std::mutex> lock(g_engine_mutex);

    std::string id = jstringToString(env, voiceId);
    if (id.empty()) {
        LOGE("Invalid voice ID");
        return JNI_FALSE;
    }

    // Find voice definition
    const laprdus::VoiceDefinition* voice = laprdus::VoiceRegistry::find_by_id(id.c_str());
    if (!voice) {
        LOGE("Voice not found: %s", id.c_str());
        return JNI_FALSE;
    }

    // Get the physical voice (for derived voices)
    const laprdus::VoiceDefinition* physicalVoice = laprdus::VoiceRegistry::get_physical_voice(voice);
    if (!physicalVoice) {
        LOGE("Failed to get physical voice for: %s", id.c_str());
        return JNI_FALSE;
    }

    // Get asset manager
    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
    if (!mgr) {
        LOGE("Failed to get asset manager");
        return JNI_FALSE;
    }

    // Get data filename from voice registry
    const char* dataFilename = laprdus::VoiceRegistry::get_data_filename(voice);
    if (!dataFilename) {
        LOGE("No data filename for voice: %s", id.c_str());
        return JNI_FALSE;
    }

    // Build full asset path with voices/ subdirectory prefix
    std::string assetPath = std::string("voices/") + dataFilename;

    // Open asset
    AAsset* asset = AAssetManager_open(mgr, assetPath.c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
        LOGE("Failed to open voice asset: %s", assetPath.c_str());
        return JNI_FALSE;
    }

    // Get data
    size_t size = AAsset_getLength(asset);
    const uint8_t* data = static_cast<const uint8_t*>(AAsset_getBuffer(asset));

    if (!data || size == 0) {
        AAsset_close(asset);
        LOGE("Voice asset is empty: %s", dataFilename);
        return JNI_FALSE;
    }

    // Initialize or reinitialize engine with new voice data
    if (!g_engine) {
        g_engine = std::make_unique<laprdus::TTSEngine>();
    }

    bool success = g_engine->initialize_from_memory(data, size, {});
    AAsset_close(asset);

    if (!success) {
        LOGE("Failed to load voice data: %s", dataFilename);
        return JNI_FALSE;
    }

    // Store and apply voice's base pitch for derived voices
    // This base_pitch defines the voice character (e.g., detence=1.5 for child voice)
    // and must be preserved even when Android TTS changes other pitch settings
    g_voice_base_pitch = voice->base_pitch;

    laprdus::VoiceParams params = g_engine->voice_params();
    params.pitch = g_voice_base_pitch;
    g_engine->set_voice_params(params);

    if (voice->base_pitch != 1.0f) {
        LOGI("Applied base pitch %.2f for derived voice: %s", voice->base_pitch, id.c_str());
    }

    LOGI("Voice set successfully: %s", id.c_str());
    return JNI_TRUE;
}

// =============================================================================
// Dictionary Methods
// =============================================================================

JNIEXPORT jboolean JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeLoadDictionaryFromAssets(
    JNIEnv* env,
    jobject thiz,
    jobject assetManager,
    jstring assetPath) {

    (void)thiz;
    std::lock_guard<std::mutex> lock(g_engine_mutex);

    if (!g_engine || !g_engine->is_initialized()) {
        LOGE("Cannot load dictionary - engine not initialized");
        return JNI_FALSE;
    }

    // Get asset manager
    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
    if (!mgr) {
        LOGE("Failed to get asset manager for dictionary");
        return JNI_FALSE;
    }

    std::string path = jstringToString(env, assetPath);
    if (path.empty()) {
        LOGE("Invalid dictionary asset path");
        return JNI_FALSE;
    }

    // Open asset
    AAsset* asset = AAssetManager_open(mgr, path.c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
        LOGI("Dictionary asset not found: %s (this is optional)", path.c_str());
        return JNI_FALSE;
    }

    // Get data
    size_t size = AAsset_getLength(asset);
    const char* data = static_cast<const char*>(AAsset_getBuffer(asset));

    if (!data || size == 0) {
        AAsset_close(asset);
        LOGE("Dictionary asset is empty");
        return JNI_FALSE;
    }

    // Load dictionary
    bool success = g_engine->load_dictionary_from_memory(data, size);
    AAsset_close(asset);

    if (success) {
        LOGI("Dictionary loaded from: %s", path.c_str());
    } else {
        LOGE("Failed to parse dictionary: %s", path.c_str());
    }

    return success ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeAddPronunciation(
    JNIEnv* env,
    jobject thiz,
    jstring grapheme,
    jstring phoneme,
    jboolean caseSensitive,
    jboolean wholeWord) {

    (void)thiz;
    std::lock_guard<std::mutex> lock(g_engine_mutex);

    if (!g_engine || !g_engine->is_initialized()) {
        LOGE("Cannot add pronunciation - engine not initialized");
        return;
    }

    std::string g = jstringToString(env, grapheme);
    std::string p = jstringToString(env, phoneme);

    if (g.empty() || p.empty()) {
        return;
    }

    g_engine->add_pronunciation(g, p, caseSensitive, wholeWord);
}

// =============================================================================
// Spelling Dictionary Methods
// =============================================================================

JNIEXPORT jboolean JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeLoadSpellingDictionaryFromAssets(
    JNIEnv* env,
    jobject thiz,
    jobject assetManager,
    jstring assetPath) {

    (void)thiz;

    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (!g_engine || !g_engine->is_initialized()) {
        LOGE("Engine not initialized - cannot load spelling dictionary");
        return JNI_FALSE;
    }

    // Get asset manager
    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
    if (!mgr) {
        LOGE("Failed to get AAssetManager");
        return JNI_FALSE;
    }

    // Get path string
    const char* pathChars = env->GetStringUTFChars(assetPath, nullptr);
    if (!pathChars) {
        LOGE("Failed to get spelling dictionary path string");
        return JNI_FALSE;
    }
    std::string path(pathChars);
    env->ReleaseStringUTFChars(assetPath, pathChars);

    // Open asset
    AAsset* asset = AAssetManager_open(mgr, path.c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
        LOGI("Spelling dictionary asset not found: %s (this is optional)", path.c_str());
        return JNI_FALSE;
    }

    // Get data
    size_t size = AAsset_getLength(asset);
    const char* data = static_cast<const char*>(AAsset_getBuffer(asset));

    if (!data || size == 0) {
        AAsset_close(asset);
        LOGE("Spelling dictionary asset is empty");
        return JNI_FALSE;
    }

    // Load spelling dictionary
    bool success = g_engine->load_spelling_dictionary_from_memory(data, size);
    AAsset_close(asset);

    if (success) {
        LOGI("Spelling dictionary loaded from: %s", path.c_str());
    } else {
        LOGE("Failed to parse spelling dictionary: %s", path.c_str());
    }

    return success ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jshortArray JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSynthesizeSpelled(
    JNIEnv* env,
    jobject thiz,
    jstring text) {

    (void)thiz;

    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (!g_engine || !g_engine->is_initialized()) {
        LOGE("Engine not initialized for spelled synthesis");
        return nullptr;
    }

    // Get text string
    const char* textChars = env->GetStringUTFChars(text, nullptr);
    if (!textChars) {
        LOGE("Failed to get text string for spelled synthesis");
        return nullptr;
    }
    std::string utf8_text(textChars);
    env->ReleaseStringUTFChars(text, textChars);

    if (utf8_text.empty()) {
        return env->NewShortArray(0);
    }

    // Synthesize in spelled mode
    laprdus::SynthesisResult result = g_engine->synthesize_spelled(utf8_text);
    if (!result.success) {
        LOGE("Spelled synthesis failed: %s", result.error_message.c_str());
        return nullptr;
    }

    // Convert to Java short array
    size_t numSamples = result.audio.samples.size();
    if (numSamples == 0) {
        return env->NewShortArray(0);
    }

    jshortArray output = env->NewShortArray(static_cast<jsize>(numSamples));
    if (!output) {
        LOGE("Failed to allocate output array for spelled synthesis");
        return nullptr;
    }

    env->SetShortArrayRegion(output, 0, static_cast<jsize>(numSamples),
                             result.audio.samples.data());

    return output;
}

// =============================================================================
// Emoji Dictionary Methods
// =============================================================================

JNIEXPORT jboolean JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeLoadEmojiDictionaryFromAssets(
    JNIEnv* env,
    jobject thiz,
    jobject assetManager,
    jstring assetPath) {

    (void)thiz;

    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (!g_engine || !g_engine->is_initialized()) {
        LOGE("Engine not initialized - cannot load emoji dictionary");
        return JNI_FALSE;
    }

    // Get asset manager
    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
    if (!mgr) {
        LOGE("Failed to get AAssetManager");
        return JNI_FALSE;
    }

    // Get path string
    std::string path = jstringToString(env, assetPath);
    if (path.empty()) {
        LOGE("Invalid emoji dictionary path");
        return JNI_FALSE;
    }

    // Open asset
    AAsset* asset = AAssetManager_open(mgr, path.c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
        LOGI("Emoji dictionary asset not found: %s (this is optional)", path.c_str());
        return JNI_FALSE;
    }

    // Get data
    size_t size = AAsset_getLength(asset);
    const char* data = static_cast<const char*>(AAsset_getBuffer(asset));

    if (!data || size == 0) {
        AAsset_close(asset);
        LOGE("Emoji dictionary asset is empty");
        return JNI_FALSE;
    }

    // Load emoji dictionary
    bool success = g_engine->load_emoji_dictionary_from_memory(data, size);
    AAsset_close(asset);

    if (success) {
        LOGI("Emoji dictionary loaded from: %s", path.c_str());
    } else {
        LOGE("Failed to parse emoji dictionary: %s", path.c_str());
    }

    return success ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetEmojiEnabled(
    JNIEnv* env,
    jobject thiz,
    jboolean enabled) {

    (void)env;
    (void)thiz;

    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (!g_engine) return;

    g_engine->set_emoji_enabled(enabled);
}

JNIEXPORT jboolean JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeIsEmojiEnabled(
    JNIEnv* env,
    jobject thiz) {

    (void)env;
    (void)thiz;

    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (!g_engine) return JNI_FALSE;

    return g_engine->is_emoji_enabled() ? JNI_TRUE : JNI_FALSE;
}

// =============================================================================
// Pause Settings Methods
// =============================================================================

JNIEXPORT void JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetSentencePause(
    JNIEnv* env,
    jobject thiz,
    jint pauseMs) {

    (void)env;
    (void)thiz;

    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (!g_engine) return;

    g_engine->set_sentence_pause(static_cast<uint32_t>(pauseMs));
}

JNIEXPORT void JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetCommaPause(
    JNIEnv* env,
    jobject thiz,
    jint pauseMs) {

    (void)env;
    (void)thiz;

    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (!g_engine) return;

    g_engine->set_comma_pause(static_cast<uint32_t>(pauseMs));
}

JNIEXPORT void JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetNewlinePause(
    JNIEnv* env,
    jobject thiz,
    jint pauseMs) {

    (void)env;
    (void)thiz;

    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (!g_engine) return;

    g_engine->set_newline_pause(static_cast<uint32_t>(pauseMs));
}

JNIEXPORT jint JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeGetSentencePause(
    JNIEnv* env,
    jobject thiz) {

    (void)env;
    (void)thiz;

    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (!g_engine) return 100;

    return static_cast<jint>(g_engine->pause_settings().sentence_pause_ms);
}

JNIEXPORT jint JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeGetCommaPause(
    JNIEnv* env,
    jobject thiz) {

    (void)env;
    (void)thiz;

    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (!g_engine) return 100;

    return static_cast<jint>(g_engine->pause_settings().comma_pause_ms);
}

JNIEXPORT jint JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeGetNewlinePause(
    JNIEnv* env,
    jobject thiz) {

    (void)env;
    (void)thiz;

    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (!g_engine) return 100;

    return static_cast<jint>(g_engine->pause_settings().newline_pause_ms);
}

JNIEXPORT void JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetSpellingPause(
    JNIEnv* env,
    jobject thiz,
    jint pauseMs) {

    (void)env;
    (void)thiz;

    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (!g_engine) return;

    g_engine->set_spelling_pause(static_cast<uint32_t>(pauseMs));
}

JNIEXPORT jint JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeGetSpellingPause(
    JNIEnv* env,
    jobject thiz) {

    (void)env;
    (void)thiz;

    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (!g_engine) return 200;

    return static_cast<jint>(g_engine->spelling_pause());
}

// =============================================================================
// Number Mode Methods
// =============================================================================

JNIEXPORT void JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeSetNumberMode(
    JNIEnv* env,
    jobject thiz,
    jint mode) {

    (void)env;
    (void)thiz;

    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (!g_engine) return;

    laprdus::NumberMode numberMode;
    switch (mode) {
        case 1:
            numberMode = laprdus::NumberMode::DigitByDigit;
            break;
        case 0:
        default:
            numberMode = laprdus::NumberMode::WholeNumbers;
            break;
    }
    g_engine->set_number_mode(numberMode);
}

JNIEXPORT jint JNICALL
Java_com_hrvojekatic_laprdus_tts_LaprdusTTS_nativeGetNumberMode(
    JNIEnv* env,
    jobject thiz) {

    (void)env;
    (void)thiz;

    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (!g_engine) return 0;

    laprdus::NumberMode mode = g_engine->number_mode();
    switch (mode) {
        case laprdus::NumberMode::DigitByDigit:
            return 1;
        case laprdus::NumberMode::WholeNumbers:
        default:
            return 0;
    }
}

// JNI_OnLoad - Called when native library is loaded
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    (void)vm;
    (void)reserved;
    LOGI("LaprdusTTS native library loaded");
    return JNI_VERSION_1_6;
}

// JNI_OnUnload - Called when native library is unloaded
JNIEXPORT void JNI_OnUnload(JavaVM* vm, void* reserved) {
    (void)vm;
    (void)reserved;
    LOGI("LaprdusTTS native library unloading");
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    g_engine.reset();
}

} // extern "C"

#endif // ANDROID
