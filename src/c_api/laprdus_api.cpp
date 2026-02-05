// -*- coding: utf-8 -*-
// laprdus_api.cpp - C API implementation

#include "laprdus/laprdus_api.h"
#include "../core/tts_engine.hpp"
#include "../core/voice_registry.hpp"
#include "../core/user_config.hpp"
#include <cstring>
#include <new>
#include <mutex>
#include <memory>
#include <string>

// =============================================================================
// Internal Structures
// =============================================================================

struct LaprdusEngine {
    laprdus::TTSEngine engine;
    std::string last_error;
    std::string current_voice_id;     // Currently active voice ID
    std::string data_directory;       // Directory containing voice .bin files
    float voice_base_pitch = 1.0f;    // Base pitch of current voice
    std::mutex mutex;  // For thread-safe error message access

    LaprdusEngine() = default;
};

struct LaprdusStream {
    laprdus::AudioBuffer audio;
    size_t read_position = 0;
    size_t total_samples = 0;
    bool complete = false;

    LaprdusStream() = default;
};

// =============================================================================
// Helper Functions
// =============================================================================

static void set_error(LaprdusEngine* engine, const char* msg) {
    if (engine) {
        std::lock_guard<std::mutex> lock(engine->mutex);
        engine->last_error = msg ? msg : "";
    }
}

static void set_error(LaprdusEngine* engine, const std::string& msg) {
    if (engine) {
        std::lock_guard<std::mutex> lock(engine->mutex);
        engine->last_error = msg;
    }
}

// =============================================================================
// Lifecycle Functions
// =============================================================================

extern "C" {

LAPRDUS_API LaprdusHandle LAPRDUS_CALL laprdus_create(void) {
    try {
        return new LaprdusEngine();
    } catch (const std::bad_alloc&) {
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

LAPRDUS_API void LAPRDUS_CALL laprdus_destroy(LaprdusHandle handle) {
    if (handle) {
        delete handle;
    }
}

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_init_from_file(
    LaprdusHandle handle,
    const char* phoneme_data_path,
    const uint8_t* decryption_key,
    size_t key_size) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    if (!phoneme_data_path) {
        set_error(handle, "Phoneme data path is NULL");
        return LAPRDUS_ERROR_INVALID_PATH;
    }

    laprdus::span<const uint8_t> key;
    if (decryption_key && key_size > 0) {
        key = laprdus::span<const uint8_t>(decryption_key, key_size);
    }

    if (!handle->engine.initialize(phoneme_data_path, key)) {
        set_error(handle, "Failed to load phoneme data from file");
        return LAPRDUS_ERROR_LOAD_FAILED;
    }

    return LAPRDUS_OK;
}

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_init_from_memory(
    LaprdusHandle handle,
    const uint8_t* data,
    size_t data_size,
    const uint8_t* decryption_key,
    size_t key_size) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    if (!data || data_size == 0) {
        set_error(handle, "Invalid data pointer or size");
        return LAPRDUS_ERROR_INVALID_PARAMETER;
    }

    laprdus::span<const uint8_t> key;
    if (decryption_key && key_size > 0) {
        key = laprdus::span<const uint8_t>(decryption_key, key_size);
    }

    if (!handle->engine.initialize_from_memory(data, data_size, key)) {
        set_error(handle, "Failed to load phoneme data from memory");
        return LAPRDUS_ERROR_LOAD_FAILED;
    }

    return LAPRDUS_OK;
}

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_init_from_directory(
    LaprdusHandle handle,
    const char* phoneme_dir) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    if (!phoneme_dir) {
        set_error(handle, "Phoneme directory is NULL");
        return LAPRDUS_ERROR_INVALID_PATH;
    }

    if (!handle->engine.initialize(phoneme_dir)) {
        set_error(handle, "Failed to load phoneme data from directory");
        return LAPRDUS_ERROR_LOAD_FAILED;
    }

    return LAPRDUS_OK;
}

LAPRDUS_API int LAPRDUS_CALL laprdus_is_initialized(LaprdusHandle handle) {
    if (!handle) {
        return 0;
    }
    return handle->engine.is_initialized() ? 1 : 0;
}

// =============================================================================
// Voice Configuration
// =============================================================================

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_voice_params(
    LaprdusHandle handle,
    const LaprdusVoiceParams* params) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    if (!params) {
        set_error(handle, "Voice params is NULL");
        return LAPRDUS_ERROR_INVALID_PARAMETER;
    }

    laprdus::VoiceParams vp;
    vp.speed = params->speed;
    vp.pitch = params->pitch;
    vp.volume = params->volume;

    handle->engine.set_voice_params(vp);
    return LAPRDUS_OK;
}

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_get_voice_params(
    LaprdusHandle handle,
    LaprdusVoiceParams* out_params) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    if (!out_params) {
        set_error(handle, "Output params is NULL");
        return LAPRDUS_ERROR_INVALID_PARAMETER;
    }

    laprdus::VoiceParams vp = handle->engine.voice_params();
    out_params->speed = vp.speed;
    out_params->pitch = vp.pitch;
    out_params->volume = vp.volume;

    return LAPRDUS_OK;
}

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_speed(
    LaprdusHandle handle,
    float speed) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    laprdus::VoiceParams vp = handle->engine.voice_params();
    vp.speed = speed;
    handle->engine.set_voice_params(vp);
    return LAPRDUS_OK;
}

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_pitch(
    LaprdusHandle handle,
    float pitch) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    // Apply effective pitch: voice base pitch * user pitch
    // This allows derived voices (child, grandma, grandpa) to have their
    // own base pitch while still allowing user adjustment
    // Note: This shifts formants (chipmunk effect) - for voice character changes
    float effective_pitch = handle->voice_base_pitch * pitch;

    // Clamp to valid range (wider range for better compatibility)
    effective_pitch = std::clamp(effective_pitch, 0.25f, 4.0f);

    laprdus::VoiceParams vp = handle->engine.voice_params();
    vp.pitch = effective_pitch;
    handle->engine.set_voice_params(vp);
    return LAPRDUS_OK;
}

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_user_pitch(
    LaprdusHandle handle,
    float pitch) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    // User pitch preference - formant-preserving pitch shift
    // This does NOT shift formants - voice character stays the same
    // Use this for user-controlled pitch adjustment (NVDA/SAPI5 slider)
    pitch = std::clamp(pitch, 0.5f, 2.0f);

    laprdus::VoiceParams vp = handle->engine.voice_params();
    vp.user_pitch = pitch;
    handle->engine.set_voice_params(vp);
    return LAPRDUS_OK;
}

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_volume(
    LaprdusHandle handle,
    float volume) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    laprdus::VoiceParams vp = handle->engine.voice_params();
    vp.volume = volume;
    handle->engine.set_voice_params(vp);
    return LAPRDUS_OK;
}

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_inflection_enabled(
    LaprdusHandle handle,
    int enabled) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    laprdus::VoiceParams vp = handle->engine.voice_params();
    vp.inflection_enabled = (enabled != 0);
    handle->engine.set_voice_params(vp);
    return LAPRDUS_OK;
}

// =============================================================================
// Synthesis Functions
// =============================================================================

LAPRDUS_API int32_t LAPRDUS_CALL laprdus_synthesize(
    LaprdusHandle handle,
    const char* text,
    int16_t** out_samples,
    LaprdusAudioFormat* out_format) {

    if (!handle) {
        return static_cast<int32_t>(LAPRDUS_ERROR_INVALID_HANDLE);
    }

    if (!handle->engine.is_initialized()) {
        set_error(handle, "Engine not initialized");
        return static_cast<int32_t>(LAPRDUS_ERROR_NOT_INITIALIZED);
    }

    if (!text) {
        set_error(handle, "Text is NULL");
        return static_cast<int32_t>(LAPRDUS_ERROR_INVALID_PARAMETER);
    }

    if (!out_samples) {
        set_error(handle, "Output samples pointer is NULL");
        return static_cast<int32_t>(LAPRDUS_ERROR_INVALID_PARAMETER);
    }

    // Synthesize
    laprdus::SynthesisResult result = handle->engine.synthesize(text);

    if (!result.success) {
        set_error(handle, result.error_message);
        return static_cast<int32_t>(LAPRDUS_ERROR_SYNTHESIS_FAILED);
    }

    // Allocate output buffer
    size_t num_samples = result.audio.samples.size();
    if (num_samples == 0) {
        *out_samples = nullptr;
        if (out_format) {
            out_format->sample_rate = laprdus::SAMPLE_RATE;
            out_format->bits_per_sample = laprdus::BITS_PER_SAMPLE;
            out_format->channels = laprdus::NUM_CHANNELS;
        }
        return 0;
    }

    // Check for integer overflow in size calculation
    if (num_samples > SIZE_MAX / sizeof(int16_t)) {
        set_error(handle, "Audio data too large");
        return static_cast<int32_t>(LAPRDUS_ERROR_OUT_OF_MEMORY);
    }

    int16_t* buffer = static_cast<int16_t*>(
        malloc(num_samples * sizeof(int16_t)));

    if (!buffer) {
        set_error(handle, "Out of memory");
        return static_cast<int32_t>(LAPRDUS_ERROR_OUT_OF_MEMORY);
    }

    // Copy samples
    memcpy(buffer, result.audio.samples.data(), num_samples * sizeof(int16_t));

    *out_samples = buffer;

    if (out_format) {
        out_format->sample_rate = result.audio.sample_rate;
        out_format->bits_per_sample = result.audio.bits_per_sample;
        out_format->channels = result.audio.channels;
    }

    return static_cast<int32_t>(num_samples);
}

LAPRDUS_API int32_t LAPRDUS_CALL laprdus_synthesize_to_buffer(
    LaprdusHandle handle,
    const char* text,
    int16_t* buffer,
    size_t buffer_size,
    LaprdusAudioFormat* out_format) {

    if (!handle) {
        return static_cast<int32_t>(LAPRDUS_ERROR_INVALID_HANDLE);
    }

    if (!handle->engine.is_initialized()) {
        set_error(handle, "Engine not initialized");
        return static_cast<int32_t>(LAPRDUS_ERROR_NOT_INITIALIZED);
    }

    if (!text) {
        set_error(handle, "Text is NULL");
        return static_cast<int32_t>(LAPRDUS_ERROR_INVALID_PARAMETER);
    }

    // Synthesize
    laprdus::SynthesisResult result = handle->engine.synthesize(text);

    if (!result.success) {
        set_error(handle, result.error_message);
        return static_cast<int32_t>(LAPRDUS_ERROR_SYNTHESIS_FAILED);
    }

    // Set format info
    if (out_format) {
        out_format->sample_rate = result.audio.sample_rate;
        out_format->bits_per_sample = result.audio.bits_per_sample;
        out_format->channels = result.audio.channels;
    }

    size_t num_samples = result.audio.samples.size();

    // If buffer is NULL, just return required size
    if (!buffer) {
        return static_cast<int32_t>(num_samples);
    }

    // Check if buffer is large enough
    if (buffer_size < num_samples) {
        // Return required size (positive) to indicate buffer too small
        return static_cast<int32_t>(num_samples);
    }

    // Copy samples to buffer
    if (num_samples > 0) {
        memcpy(buffer, result.audio.samples.data(), num_samples * sizeof(int16_t));
    }

    return static_cast<int32_t>(num_samples);
}

LAPRDUS_API void LAPRDUS_CALL laprdus_free_buffer(int16_t* buffer) {
    if (buffer) {
        free(buffer);
    }
}

LAPRDUS_API void LAPRDUS_CALL laprdus_cancel(LaprdusHandle handle) {
    // TODO: Implement cancellation support
    // For now, this is a no-op as synthesis is synchronous
    (void)handle;
}

// =============================================================================
// Streaming Synthesis
// =============================================================================

LAPRDUS_API LaprdusStreamHandle LAPRDUS_CALL laprdus_stream_begin(
    LaprdusHandle handle,
    const char* text) {

    if (!handle || !handle->engine.is_initialized()) {
        return nullptr;
    }

    if (!text) {
        return nullptr;
    }

    try {
        // Create stream object
        auto* stream = new LaprdusStream();

        // Synthesize entire audio upfront
        // (Real streaming would synthesize incrementally)
        laprdus::SynthesisResult result = handle->engine.synthesize(text);

        if (!result.success) {
            delete stream;
            set_error(handle, result.error_message);
            return nullptr;
        }

        stream->audio = std::move(result.audio);
        stream->total_samples = stream->audio.samples.size();
        stream->read_position = 0;
        stream->complete = false;

        return stream;

    } catch (const std::bad_alloc&) {
        set_error(handle, "Out of memory");
        return nullptr;
    }
}

LAPRDUS_API int32_t LAPRDUS_CALL laprdus_stream_read(
    LaprdusStreamHandle stream,
    int16_t* buffer,
    size_t max_samples) {

    if (!stream || !buffer || max_samples == 0) {
        return static_cast<int32_t>(LAPRDUS_ERROR_INVALID_PARAMETER);
    }

    if (stream->complete) {
        return 0;  // No more data
    }

    // Calculate how many samples to read
    size_t remaining = stream->total_samples - stream->read_position;
    size_t to_read = std::min(max_samples, remaining);

    if (to_read == 0) {
        stream->complete = true;
        return 0;
    }

    // Copy samples
    memcpy(buffer,
           stream->audio.samples.data() + stream->read_position,
           to_read * sizeof(int16_t));

    stream->read_position += to_read;

    if (stream->read_position >= stream->total_samples) {
        stream->complete = true;
    }

    return static_cast<int32_t>(to_read);
}

LAPRDUS_API float LAPRDUS_CALL laprdus_stream_progress(LaprdusStreamHandle stream) {
    if (!stream || stream->total_samples == 0) {
        return 1.0f;
    }

    return static_cast<float>(stream->read_position) /
           static_cast<float>(stream->total_samples);
}

LAPRDUS_API int LAPRDUS_CALL laprdus_stream_is_complete(LaprdusStreamHandle stream) {
    if (!stream) {
        return 1;
    }
    return stream->complete ? 1 : 0;
}

LAPRDUS_API void LAPRDUS_CALL laprdus_stream_destroy(LaprdusStreamHandle stream) {
    if (stream) {
        delete stream;
    }
}

// =============================================================================
// Utility Functions
// =============================================================================

LAPRDUS_API const char* LAPRDUS_CALL laprdus_get_error_message(LaprdusHandle handle) {
    if (!handle) {
        return "Invalid handle";
    }

    // Use thread-local storage to safely return a copy of the error message.
    // This prevents TOCTOU race conditions where another thread could modify
    // last_error after the lock is released but before the caller uses the string.
    thread_local std::string error_copy;
    {
        std::lock_guard<std::mutex> lock(handle->mutex);
        error_copy = handle->last_error;
    }
    return error_copy.c_str();
}

LAPRDUS_API const char* LAPRDUS_CALL laprdus_get_version(void) {
    return laprdus::TTSEngine::version();
}

LAPRDUS_API void LAPRDUS_CALL laprdus_get_default_format(LaprdusAudioFormat* out_format) {
    if (out_format) {
        out_format->sample_rate = laprdus::SAMPLE_RATE;
        out_format->bits_per_sample = laprdus::BITS_PER_SAMPLE;
        out_format->channels = laprdus::NUM_CHANNELS;
    }
}

LAPRDUS_API const char* LAPRDUS_CALL laprdus_error_to_string(LaprdusError error) {
    switch (error) {
        case LAPRDUS_OK:
            return "OK";
        case LAPRDUS_ERROR_INVALID_HANDLE:
            return "Invalid handle";
        case LAPRDUS_ERROR_NOT_INITIALIZED:
            return "Engine not initialized";
        case LAPRDUS_ERROR_INVALID_PATH:
            return "Invalid path";
        case LAPRDUS_ERROR_LOAD_FAILED:
            return "Failed to load data";
        case LAPRDUS_ERROR_SYNTHESIS_FAILED:
            return "Synthesis failed";
        case LAPRDUS_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case LAPRDUS_ERROR_CANCELLED:
            return "Operation cancelled";
        case LAPRDUS_ERROR_INVALID_PARAMETER:
            return "Invalid parameter";
        case LAPRDUS_ERROR_DECRYPTION_FAILED:
            return "Decryption failed";
        case LAPRDUS_ERROR_FILE_NOT_FOUND:
            return "File not found";
        case LAPRDUS_ERROR_INVALID_FORMAT:
            return "Invalid file format";
        default:
            return "Unknown error";
    }
}

// =============================================================================
// Voice Selection Functions
// =============================================================================

// Helper to populate LaprdusVoiceInfo from VoiceDefinition
static void populate_voice_info(const laprdus::VoiceDefinition* def, LaprdusVoiceInfo* out) {
    out->id = def->id;
    out->display_name = def->display_name;
    out->language_code = laprdus::voice_language_code(def->language);
    out->language_lcid = laprdus::voice_language_lcid(def->language);
    out->gender = laprdus::voice_gender_string(def->gender);
    out->age = laprdus::voice_age_string(def->age);
    out->base_pitch = def->base_pitch;
    out->base_voice_id = def->base_voice_id;
    out->data_filename = laprdus::VoiceRegistry::get_data_filename(def);
}

LAPRDUS_API uint32_t LAPRDUS_CALL laprdus_get_voice_count(void) {
    return static_cast<uint32_t>(laprdus::VoiceRegistry::voice_count());
}

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_get_voice_info(
    uint32_t index,
    LaprdusVoiceInfo* out_info) {

    if (!out_info) {
        return LAPRDUS_ERROR_INVALID_PARAMETER;
    }

    const laprdus::VoiceDefinition* def = laprdus::VoiceRegistry::get_by_index(index);
    if (!def) {
        return LAPRDUS_ERROR_INVALID_PARAMETER;
    }

    populate_voice_info(def, out_info);
    return LAPRDUS_OK;
}

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_get_voice_info_by_id(
    const char* voice_id,
    LaprdusVoiceInfo* out_info) {

    if (!voice_id || !out_info) {
        return LAPRDUS_ERROR_INVALID_PARAMETER;
    }

    const laprdus::VoiceDefinition* def = laprdus::VoiceRegistry::find_by_id(voice_id);
    if (!def) {
        return LAPRDUS_ERROR_INVALID_PARAMETER;
    }

    populate_voice_info(def, out_info);
    return LAPRDUS_OK;
}

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_voice(
    LaprdusHandle handle,
    const char* voice_id,
    const char* data_directory) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    if (!voice_id) {
        set_error(handle, "Voice ID is NULL");
        return LAPRDUS_ERROR_INVALID_PARAMETER;
    }

    if (!data_directory) {
        set_error(handle, "Data directory is NULL");
        return LAPRDUS_ERROR_INVALID_PATH;
    }

    // Find the requested voice
    const laprdus::VoiceDefinition* voice = laprdus::VoiceRegistry::find_by_id(voice_id);
    if (!voice) {
        set_error(handle, "Voice not found: " + std::string(voice_id));
        return LAPRDUS_ERROR_INVALID_PARAMETER;
    }

    // Get the physical voice (same as voice if it's physical, or base voice if derived)
    const laprdus::VoiceDefinition* physical = laprdus::VoiceRegistry::get_physical_voice(voice);
    if (!physical) {
        set_error(handle, "Failed to resolve physical voice");
        return LAPRDUS_ERROR_INVALID_PARAMETER;
    }

    // Get the data filename
    const char* data_filename = laprdus::VoiceRegistry::get_data_filename(voice);
    if (!data_filename) {
        set_error(handle, "Failed to get data filename for voice");
        return LAPRDUS_ERROR_INVALID_PARAMETER;
    }

    // Check if we need to reload phoneme data
    // Only reload if the physical voice changed or engine is not initialized
    const laprdus::VoiceDefinition* current_voice = nullptr;
    if (!handle->current_voice_id.empty()) {
        current_voice = laprdus::VoiceRegistry::find_by_id(handle->current_voice_id.c_str());
    }

    const laprdus::VoiceDefinition* current_physical = current_voice ?
        laprdus::VoiceRegistry::get_physical_voice(current_voice) : nullptr;

    bool need_reload = !handle->engine.is_initialized() ||
                       current_physical != physical ||
                       handle->data_directory != data_directory;

    if (need_reload) {
        // Build full path to phoneme data file
        std::string full_path = std::string(data_directory);
        if (!full_path.empty() && full_path.back() != '/' && full_path.back() != '\\') {
            full_path += '/';
        }
        full_path += data_filename;

        // Initialize engine with new phoneme data
        if (!handle->engine.initialize(full_path)) {
            set_error(handle, "Failed to load phoneme data: " + full_path);
            return LAPRDUS_ERROR_LOAD_FAILED;
        }

        handle->data_directory = data_directory;
    }

    // Store current voice and apply base pitch
    handle->current_voice_id = voice_id;
    handle->voice_base_pitch = voice->base_pitch;

    // Apply the voice's base pitch to the engine
    laprdus::VoiceParams params = handle->engine.voice_params();
    // The effective pitch will be base_pitch * user_pitch
    // We store base_pitch separately and apply it in synthesis
    handle->engine.set_voice_params(params);

    return LAPRDUS_OK;
}

LAPRDUS_API const char* LAPRDUS_CALL laprdus_get_current_voice(LaprdusHandle handle) {
    if (!handle) {
        return nullptr;
    }

    if (handle->current_voice_id.empty()) {
        return nullptr;
    }

    return handle->current_voice_id.c_str();
}

// =============================================================================
// Dictionary Functions
// =============================================================================

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_load_dictionary(
    LaprdusHandle handle,
    const char* dictionary_path) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    if (!dictionary_path) {
        set_error(handle, "Dictionary path is NULL");
        return LAPRDUS_ERROR_INVALID_PATH;
    }

    if (!handle->engine.load_dictionary(dictionary_path)) {
        set_error(handle, "Failed to load dictionary");
        return LAPRDUS_ERROR_LOAD_FAILED;
    }

    return LAPRDUS_OK;
}

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_load_dictionary_from_memory(
    LaprdusHandle handle,
    const char* json_content,
    size_t length) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    if (!json_content) {
        set_error(handle, "Dictionary content is NULL");
        return LAPRDUS_ERROR_INVALID_PARAMETER;
    }

    if (!handle->engine.load_dictionary_from_memory(json_content, length)) {
        set_error(handle, "Failed to parse dictionary content");
        return LAPRDUS_ERROR_LOAD_FAILED;
    }

    return LAPRDUS_OK;
}

LAPRDUS_API void LAPRDUS_CALL laprdus_clear_dictionary(LaprdusHandle handle) {
    if (handle) {
        handle->engine.clear_dictionary();
    }
}

// =============================================================================
// Spelling Dictionary Functions
// =============================================================================

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_load_spelling_dictionary(
    LaprdusHandle handle,
    const char* dictionary_path) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    if (!dictionary_path) {
        set_error(handle, "Spelling dictionary path is NULL");
        return LAPRDUS_ERROR_INVALID_PATH;
    }

    if (!handle->engine.load_spelling_dictionary(dictionary_path)) {
        set_error(handle, "Failed to load spelling dictionary");
        return LAPRDUS_ERROR_LOAD_FAILED;
    }

    return LAPRDUS_OK;
}

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_load_spelling_dictionary_from_memory(
    LaprdusHandle handle,
    const char* json_content,
    size_t length) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    if (!json_content) {
        set_error(handle, "Spelling dictionary content is NULL");
        return LAPRDUS_ERROR_INVALID_PARAMETER;
    }

    if (!handle->engine.load_spelling_dictionary_from_memory(json_content, length)) {
        set_error(handle, "Failed to parse spelling dictionary content");
        return LAPRDUS_ERROR_LOAD_FAILED;
    }

    return LAPRDUS_OK;
}

LAPRDUS_API void LAPRDUS_CALL laprdus_clear_spelling_dictionary(LaprdusHandle handle) {
    if (handle) {
        handle->engine.clear_spelling_dictionary();
    }
}

LAPRDUS_API int32_t LAPRDUS_CALL laprdus_synthesize_spelled(
    LaprdusHandle handle,
    const char* text,
    int16_t** out_samples,
    LaprdusAudioFormat* out_format) {

    if (!handle) {
        return static_cast<int32_t>(LAPRDUS_ERROR_INVALID_HANDLE);
    }

    if (!handle->engine.is_initialized()) {
        set_error(handle, "Engine not initialized");
        return static_cast<int32_t>(LAPRDUS_ERROR_NOT_INITIALIZED);
    }

    if (!text) {
        set_error(handle, "Text is NULL");
        return static_cast<int32_t>(LAPRDUS_ERROR_INVALID_PARAMETER);
    }

    if (!out_samples) {
        set_error(handle, "Output samples pointer is NULL");
        return static_cast<int32_t>(LAPRDUS_ERROR_INVALID_PARAMETER);
    }

    // Synthesize in spelling mode
    laprdus::SynthesisResult result = handle->engine.synthesize_spelled(text);

    if (!result.success) {
        set_error(handle, result.error_message);
        return static_cast<int32_t>(LAPRDUS_ERROR_SYNTHESIS_FAILED);
    }

    // Allocate output buffer
    size_t num_samples = result.audio.samples.size();
    if (num_samples == 0) {
        *out_samples = nullptr;
        if (out_format) {
            out_format->sample_rate = laprdus::SAMPLE_RATE;
            out_format->bits_per_sample = laprdus::BITS_PER_SAMPLE;
            out_format->channels = laprdus::NUM_CHANNELS;
        }
        return 0;
    }

    // Check for integer overflow in size calculation
    if (num_samples > SIZE_MAX / sizeof(int16_t)) {
        set_error(handle, "Audio data too large");
        return static_cast<int32_t>(LAPRDUS_ERROR_OUT_OF_MEMORY);
    }

    int16_t* buffer = static_cast<int16_t*>(
        malloc(num_samples * sizeof(int16_t)));

    if (!buffer) {
        set_error(handle, "Out of memory");
        return static_cast<int32_t>(LAPRDUS_ERROR_OUT_OF_MEMORY);
    }

    // Copy samples
    memcpy(buffer, result.audio.samples.data(), num_samples * sizeof(int16_t));

    *out_samples = buffer;

    if (out_format) {
        out_format->sample_rate = result.audio.sample_rate;
        out_format->bits_per_sample = result.audio.bits_per_sample;
        out_format->channels = result.audio.channels;
    }

    return static_cast<int32_t>(num_samples);
}

// =============================================================================
// Emoji Dictionary Functions
// =============================================================================

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_load_emoji_dictionary(
    LaprdusHandle handle,
    const char* dictionary_path) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    if (!dictionary_path) {
        set_error(handle, "Emoji dictionary path is NULL");
        return LAPRDUS_ERROR_INVALID_PATH;
    }

    if (!handle->engine.load_emoji_dictionary(dictionary_path)) {
        set_error(handle, "Failed to load emoji dictionary");
        return LAPRDUS_ERROR_LOAD_FAILED;
    }

    return LAPRDUS_OK;
}

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_load_emoji_dictionary_from_memory(
    LaprdusHandle handle,
    const char* json_content,
    size_t length) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    if (!json_content) {
        set_error(handle, "Emoji dictionary content is NULL");
        return LAPRDUS_ERROR_INVALID_PARAMETER;
    }

    if (!handle->engine.load_emoji_dictionary_from_memory(json_content, length)) {
        set_error(handle, "Failed to parse emoji dictionary content");
        return LAPRDUS_ERROR_LOAD_FAILED;
    }

    return LAPRDUS_OK;
}

LAPRDUS_API void LAPRDUS_CALL laprdus_clear_emoji_dictionary(LaprdusHandle handle) {
    if (handle) {
        handle->engine.clear_emoji_dictionary();
    }
}

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_emoji_enabled(
    LaprdusHandle handle,
    int enabled) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    handle->engine.set_emoji_enabled(enabled != 0);
    return LAPRDUS_OK;
}

LAPRDUS_API int LAPRDUS_CALL laprdus_is_emoji_enabled(LaprdusHandle handle) {
    if (!handle) {
        return 0;
    }
    return handle->engine.is_emoji_enabled() ? 1 : 0;
}

// =============================================================================
// Pause Settings Functions
// =============================================================================

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_sentence_pause(
    LaprdusHandle handle,
    uint32_t pause_ms) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    handle->engine.set_sentence_pause(pause_ms);
    return LAPRDUS_OK;
}

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_comma_pause(
    LaprdusHandle handle,
    uint32_t pause_ms) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    handle->engine.set_comma_pause(pause_ms);
    return LAPRDUS_OK;
}

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_newline_pause(
    LaprdusHandle handle,
    uint32_t pause_ms) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    handle->engine.set_newline_pause(pause_ms);
    return LAPRDUS_OK;
}

LAPRDUS_API uint32_t LAPRDUS_CALL laprdus_get_sentence_pause(LaprdusHandle handle) {
    if (!handle) {
        return 100;  // Default
    }
    return handle->engine.pause_settings().sentence_pause_ms;
}

LAPRDUS_API uint32_t LAPRDUS_CALL laprdus_get_comma_pause(LaprdusHandle handle) {
    if (!handle) {
        return 100;  // Default
    }
    return handle->engine.pause_settings().comma_pause_ms;
}

LAPRDUS_API uint32_t LAPRDUS_CALL laprdus_get_newline_pause(LaprdusHandle handle) {
    if (!handle) {
        return 100;  // Default
    }
    return handle->engine.pause_settings().newline_pause_ms;
}

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_spelling_pause(
    LaprdusHandle handle,
    uint32_t pause_ms) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    handle->engine.set_spelling_pause(pause_ms);
    return LAPRDUS_OK;
}

LAPRDUS_API uint32_t LAPRDUS_CALL laprdus_get_spelling_pause(LaprdusHandle handle) {
    if (!handle) {
        return 200;  // Default
    }
    return handle->engine.spelling_pause();
}

// =============================================================================
// Number Mode Functions
// =============================================================================

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_number_mode(
    LaprdusHandle handle,
    LaprdusNumberMode mode) {

    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    laprdus::NumberMode internal_mode;
    switch (mode) {
        case LAPRDUS_NUMBER_MODE_DIGIT:
            internal_mode = laprdus::NumberMode::DigitByDigit;
            break;
        case LAPRDUS_NUMBER_MODE_WHOLE:
        default:
            internal_mode = laprdus::NumberMode::WholeNumbers;
            break;
    }

    handle->engine.set_number_mode(internal_mode);
    return LAPRDUS_OK;
}

LAPRDUS_API LaprdusNumberMode LAPRDUS_CALL laprdus_get_number_mode(LaprdusHandle handle) {
    if (!handle) {
        return LAPRDUS_NUMBER_MODE_WHOLE;
    }

    laprdus::NumberMode mode = handle->engine.number_mode();
    switch (mode) {
        case laprdus::NumberMode::DigitByDigit:
            return LAPRDUS_NUMBER_MODE_DIGIT;
        case laprdus::NumberMode::WholeNumbers:
        default:
            return LAPRDUS_NUMBER_MODE_WHOLE;
    }
}

// =============================================================================
// User Configuration Functions
// =============================================================================

LAPRDUS_API size_t LAPRDUS_CALL laprdus_get_config_directory(
    char* buffer,
    size_t buffer_size) {

    std::string config_dir = laprdus::UserConfig::get_config_directory();

    if (config_dir.empty()) {
        return 0;
    }

    size_t len = config_dir.length();

    if (buffer && buffer_size > 0) {
        size_t copy_len = std::min(len, buffer_size - 1);
        memcpy(buffer, config_dir.c_str(), copy_len);
        buffer[copy_len] = '\0';
    }

    return len;
}

LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_load_user_config(LaprdusHandle handle) {
    if (!handle) {
        return LAPRDUS_ERROR_INVALID_HANDLE;
    }

    laprdus::UserConfig config;
    if (!config.load_settings()) {
        set_error(handle, "Failed to load user configuration");
        return LAPRDUS_ERROR_LOAD_FAILED;
    }

    // Apply user settings to engine
    const laprdus::UserSettings& settings = config.settings();

    laprdus::VoiceParams params = handle->engine.voice_params();
    params.speed = settings.speed;
    params.user_pitch = settings.user_pitch;
    params.volume = settings.volume;
    params.inflection_enabled = settings.inflection_enabled;
    params.emoji_enabled = settings.emoji_enabled;
    params.number_mode = settings.number_mode;
    params.pause_settings = settings.get_pause_settings();
    params.clamp();
    handle->engine.set_voice_params(params);

    // Load user dictionaries if they exist and are enabled
    if (settings.user_dictionaries_enabled) {
        if (config.user_dictionary_exists("user.json")) {
            std::string path = config.get_user_dictionary_path();
            handle->engine.load_dictionary(path);
        }

        if (config.user_dictionary_exists("spelling.json")) {
            std::string path = config.get_user_spelling_dictionary_path();
            handle->engine.load_spelling_dictionary(path);
        }

        if (config.user_dictionary_exists("emoji.json")) {
            std::string path = config.get_user_emoji_dictionary_path();
            handle->engine.load_emoji_dictionary(path);
        }
    }

    return LAPRDUS_OK;
}

LAPRDUS_API int LAPRDUS_CALL laprdus_user_dictionary_exists(const char* filename) {
    if (!filename) {
        return 0;
    }

    laprdus::UserConfig config;
    return config.user_dictionary_exists(filename) ? 1 : 0;
}

LAPRDUS_API size_t LAPRDUS_CALL laprdus_get_user_dictionary_path(
    const char* filename,
    char* buffer,
    size_t buffer_size) {

    if (!filename) {
        return 0;
    }

    laprdus::UserConfig config;
    std::string path = config.get_config_file_path(filename);

    if (path.empty()) {
        return 0;
    }

    size_t len = path.length();

    if (buffer && buffer_size > 0) {
        size_t copy_len = std::min(len, buffer_size - 1);
        memcpy(buffer, path.c_str(), copy_len);
        buffer[copy_len] = '\0';
    }

    return len;
}

} // extern "C"
