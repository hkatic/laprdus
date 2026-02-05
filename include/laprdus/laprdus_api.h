// -*- coding: utf-8 -*-
// laprdus_api.h - Public C API for LaprdusTTS
// Croatian Text-to-Speech Engine

#ifndef LAPRDUS_API_H
#define LAPRDUS_API_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Platform-specific export macros
// =============================================================================

#if defined(_WIN32) || defined(_WIN64)
    #ifdef LAPRDUS_EXPORTS
        #define LAPRDUS_API __declspec(dllexport)
    #else
        #define LAPRDUS_API __declspec(dllimport)
    #endif
    #define LAPRDUS_CALL __stdcall
#else
    #define LAPRDUS_API __attribute__((visibility("default")))
    #define LAPRDUS_CALL
#endif

// =============================================================================
// Opaque handle type
// =============================================================================

typedef struct LaprdusEngine* LaprdusHandle;
typedef struct LaprdusStream* LaprdusStreamHandle;

// =============================================================================
// Error codes
// =============================================================================

typedef enum LaprdusError {
    LAPRDUS_OK = 0,
    LAPRDUS_ERROR_INVALID_HANDLE = -1,
    LAPRDUS_ERROR_NOT_INITIALIZED = -2,
    LAPRDUS_ERROR_INVALID_PATH = -3,
    LAPRDUS_ERROR_LOAD_FAILED = -4,
    LAPRDUS_ERROR_SYNTHESIS_FAILED = -5,
    LAPRDUS_ERROR_OUT_OF_MEMORY = -6,
    LAPRDUS_ERROR_CANCELLED = -7,
    LAPRDUS_ERROR_INVALID_PARAMETER = -8,
    LAPRDUS_ERROR_DECRYPTION_FAILED = -9,
    LAPRDUS_ERROR_FILE_NOT_FOUND = -10,
    LAPRDUS_ERROR_INVALID_FORMAT = -11
} LaprdusError;

// =============================================================================
// Audio format info
// =============================================================================

typedef struct LaprdusAudioFormat {
    uint32_t sample_rate;       // Samples per second (typically 22050)
    uint16_t bits_per_sample;   // Bits per sample (typically 16)
    uint16_t channels;          // Number of channels (typically 1 = mono)
} LaprdusAudioFormat;

// =============================================================================
// Voice parameters
// =============================================================================

typedef struct LaprdusVoiceParams {
    float speed;    // Speech rate: 0.5 - 2.0, default 1.0
    float pitch;    // Base pitch: 0.5 - 2.0, default 1.0
    float volume;   // Volume: 0.0 - 1.0, default 1.0
} LaprdusVoiceParams;

// =============================================================================
// Voice information
// =============================================================================

typedef struct LaprdusVoiceInfo {
    const char* id;              // Internal ID: "josip", "vlado", etc.
    const char* display_name;    // Display name: "Laprdus Josip (Croatian)"
    const char* language_code;   // Language code: "hr-HR" or "sr-RS"
    uint16_t language_lcid;      // Windows LCID: 0x041A or 0x081A
    const char* gender;          // "Male" or "Female"
    const char* age;             // "Child", "Adult", or "Senior"
    float base_pitch;            // Base pitch multiplier (1.0 for normal)
    const char* base_voice_id;   // Base voice ID for derived voices (NULL if physical)
    const char* data_filename;   // Phoneme data file (NULL for derived voices)
} LaprdusVoiceInfo;

// =============================================================================
// Lifecycle Functions
// =============================================================================

/**
 * Create a new TTS engine instance.
 * @return Handle to the engine, or NULL on failure.
 */
LAPRDUS_API LaprdusHandle LAPRDUS_CALL laprdus_create(void);

/**
 * Destroy an engine instance and free all resources.
 * @param handle Engine handle to destroy.
 */
LAPRDUS_API void LAPRDUS_CALL laprdus_destroy(LaprdusHandle handle);

/**
 * Initialize the engine from a packed phoneme data file.
 * @param handle Engine handle.
 * @param phoneme_data_path Path to the phonemes.bin file.
 * @param decryption_key Optional decryption key (NULL if not encrypted).
 * @param key_size Size of decryption key in bytes (0 if not encrypted).
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_init_from_file(
    LaprdusHandle handle,
    const char* phoneme_data_path,
    const uint8_t* decryption_key,
    size_t key_size
);

/**
 * Initialize the engine from a memory buffer (for embedded resources).
 * @param handle Engine handle.
 * @param data Pointer to packed phoneme data.
 * @param data_size Size of data in bytes.
 * @param decryption_key Optional decryption key (NULL if not encrypted).
 * @param key_size Size of decryption key in bytes (0 if not encrypted).
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_init_from_memory(
    LaprdusHandle handle,
    const uint8_t* data,
    size_t data_size,
    const uint8_t* decryption_key,
    size_t key_size
);

/**
 * Initialize from individual WAV files in a directory (development mode).
 * @param handle Engine handle.
 * @param phoneme_dir Path to directory containing PHONEME_*.wav files.
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_init_from_directory(
    LaprdusHandle handle,
    const char* phoneme_dir
);

/**
 * Check if the engine is initialized and ready for synthesis.
 * @param handle Engine handle.
 * @return Non-zero if initialized, zero otherwise.
 */
LAPRDUS_API int LAPRDUS_CALL laprdus_is_initialized(LaprdusHandle handle);

// =============================================================================
// Voice Configuration
// =============================================================================

/**
 * Set voice parameters.
 * @param handle Engine handle.
 * @param params Pointer to voice parameters structure.
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_voice_params(
    LaprdusHandle handle,
    const LaprdusVoiceParams* params
);

/**
 * Get current voice parameters.
 * @param handle Engine handle.
 * @param out_params Pointer to structure to receive parameters.
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_get_voice_params(
    LaprdusHandle handle,
    LaprdusVoiceParams* out_params
);

/**
 * Set speech speed.
 * @param handle Engine handle.
 * @param speed Speed factor (0.5 - 2.0, default 1.0).
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_speed(
    LaprdusHandle handle,
    float speed
);

/**
 * Set voice character pitch (shifts formants).
 * Use this for derived voices (child, grandma, grandpa).
 * Note: This causes chipmunk effect - changes voice character.
 * For user pitch preference, use laprdus_set_user_pitch instead.
 * @param handle Engine handle.
 * @param pitch Pitch factor (0.25 - 4.0, default 1.0).
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_pitch(
    LaprdusHandle handle,
    float pitch
);

/**
 * Set user pitch preference (preserves formants).
 * Use this for user-controlled pitch adjustment.
 * Note: This does NOT cause chipmunk effect - preserves voice character.
 * For voice character changes, use laprdus_set_pitch instead.
 * @param handle Engine handle.
 * @param pitch Pitch factor (0.5 - 2.0, default 1.0).
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_user_pitch(
    LaprdusHandle handle,
    float pitch
);

/**
 * Set volume.
 * @param handle Engine handle.
 * @param volume Volume level (0.0 - 1.0, default 1.0).
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_volume(
    LaprdusHandle handle,
    float volume
);

/**
 * Enable or disable punctuation-based voice inflection.
 * @param handle Engine handle.
 * @param enabled Non-zero to enable, zero to disable.
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_inflection_enabled(
    LaprdusHandle handle,
    int enabled
);

// =============================================================================
// Voice Selection
// =============================================================================

/**
 * Get the number of available voices.
 * @return Number of voices.
 */
LAPRDUS_API uint32_t LAPRDUS_CALL laprdus_get_voice_count(void);

/**
 * Get voice information by index.
 * @param index Voice index (0 to count-1).
 * @param out_info Pointer to receive voice information.
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_get_voice_info(
    uint32_t index,
    LaprdusVoiceInfo* out_info
);

/**
 * Get voice information by ID.
 * @param voice_id Voice ID string (e.g., "josip").
 * @param out_info Pointer to receive voice information.
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_get_voice_info_by_id(
    const char* voice_id,
    LaprdusVoiceInfo* out_info
);

/**
 * Set the active voice for synthesis.
 * For physical voices, this loads the voice's phoneme data.
 * For derived voices, this loads the base voice's data and applies pitch offset.
 * @param handle Engine handle.
 * @param voice_id Voice ID to activate.
 * @param data_directory Directory containing voice .bin files.
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_voice(
    LaprdusHandle handle,
    const char* voice_id,
    const char* data_directory
);

/**
 * Get the currently active voice ID.
 * @param handle Engine handle.
 * @return Voice ID string, or NULL if no voice is set.
 */
LAPRDUS_API const char* LAPRDUS_CALL laprdus_get_current_voice(LaprdusHandle handle);

// =============================================================================
// Synthesis Functions
// =============================================================================

/**
 * Synthesize text to audio.
 * @param handle Engine handle.
 * @param text UTF-8 encoded text to synthesize.
 * @param out_samples Pointer to receive allocated audio sample buffer.
 *                    Caller must free with laprdus_free_buffer().
 * @param out_format Pointer to receive audio format information.
 * @return Number of samples on success, negative error code on failure.
 */
LAPRDUS_API int32_t LAPRDUS_CALL laprdus_synthesize(
    LaprdusHandle handle,
    const char* text,
    int16_t** out_samples,
    LaprdusAudioFormat* out_format
);

/**
 * Synthesize text to a caller-provided buffer.
 * @param handle Engine handle.
 * @param text UTF-8 encoded text to synthesize.
 * @param buffer Pre-allocated buffer for audio samples.
 * @param buffer_size Size of buffer in samples.
 * @param out_format Pointer to receive audio format information.
 * @return Number of samples written on success, negative error code on failure.
 *         If buffer is too small, returns required size as positive number
 *         and sets *out_format but doesn't write audio.
 */
LAPRDUS_API int32_t LAPRDUS_CALL laprdus_synthesize_to_buffer(
    LaprdusHandle handle,
    const char* text,
    int16_t* buffer,
    size_t buffer_size,
    LaprdusAudioFormat* out_format
);

/**
 * Free a buffer allocated by laprdus_synthesize().
 * @param buffer Buffer to free.
 */
LAPRDUS_API void LAPRDUS_CALL laprdus_free_buffer(int16_t* buffer);

/**
 * Cancel any ongoing synthesis operation.
 * @param handle Engine handle.
 */
LAPRDUS_API void LAPRDUS_CALL laprdus_cancel(LaprdusHandle handle);

// =============================================================================
// Streaming Synthesis
// =============================================================================

/**
 * Begin streaming synthesis of text.
 * @param handle Engine handle.
 * @param text UTF-8 encoded text to synthesize.
 * @return Stream handle, or NULL on failure.
 */
LAPRDUS_API LaprdusStreamHandle LAPRDUS_CALL laprdus_stream_begin(
    LaprdusHandle handle,
    const char* text
);

/**
 * Read the next chunk of audio from a stream.
 * @param stream Stream handle.
 * @param buffer Buffer to receive audio samples.
 * @param max_samples Maximum number of samples to read.
 * @return Number of samples read, 0 if complete, negative on error.
 */
LAPRDUS_API int32_t LAPRDUS_CALL laprdus_stream_read(
    LaprdusStreamHandle stream,
    int16_t* buffer,
    size_t max_samples
);

/**
 * Get the progress of a streaming synthesis (0.0 to 1.0).
 * @param stream Stream handle.
 * @return Progress as a float between 0.0 and 1.0.
 */
LAPRDUS_API float LAPRDUS_CALL laprdus_stream_progress(LaprdusStreamHandle stream);

/**
 * Check if a stream has completed.
 * @param stream Stream handle.
 * @return Non-zero if complete, zero otherwise.
 */
LAPRDUS_API int LAPRDUS_CALL laprdus_stream_is_complete(LaprdusStreamHandle stream);

/**
 * Destroy a stream handle.
 * @param stream Stream handle to destroy.
 */
LAPRDUS_API void LAPRDUS_CALL laprdus_stream_destroy(LaprdusStreamHandle stream);

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * Get the last error message for an engine.
 * @param handle Engine handle.
 * @return Error message string (do not free).
 */
LAPRDUS_API const char* LAPRDUS_CALL laprdus_get_error_message(LaprdusHandle handle);

/**
 * Get the library version string.
 * @return Version string in format "major.minor.patch".
 */
LAPRDUS_API const char* LAPRDUS_CALL laprdus_get_version(void);

/**
 * Get the default audio format.
 * @param out_format Pointer to receive format information.
 */
LAPRDUS_API void LAPRDUS_CALL laprdus_get_default_format(LaprdusAudioFormat* out_format);

/**
 * Convert an error code to a human-readable string.
 * @param error Error code.
 * @return String description of the error.
 */
LAPRDUS_API const char* LAPRDUS_CALL laprdus_error_to_string(LaprdusError error);

// =============================================================================
// Dictionary Functions
// =============================================================================

/**
 * Load a pronunciation dictionary from a JSON file.
 * Dictionary entries replace text before phoneme mapping.
 * @param handle Engine handle.
 * @param dictionary_path Path to the dictionary JSON file.
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_load_dictionary(
    LaprdusHandle handle,
    const char* dictionary_path
);

/**
 * Load a pronunciation dictionary from memory.
 * @param handle Engine handle.
 * @param json_content JSON content as a string.
 * @param length Length of the content (0 for null-terminated).
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_load_dictionary_from_memory(
    LaprdusHandle handle,
    const char* json_content,
    size_t length
);

/**
 * Append pronunciation dictionary entries from a JSON file.
 * Keeps existing entries and adds new ones from the file.
 * @param handle Engine handle.
 * @param dictionary_path Path to the dictionary JSON file.
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_append_dictionary(
    LaprdusHandle handle,
    const char* dictionary_path
);

/**
 * Clear all entries from the pronunciation dictionary.
 * @param handle Engine handle.
 */
LAPRDUS_API void LAPRDUS_CALL laprdus_clear_dictionary(LaprdusHandle handle);

// =============================================================================
// Spelling Dictionary Functions (for character-by-character pronunciation)
// =============================================================================

/**
 * Load a spelling dictionary from a JSON file.
 * The spelling dictionary maps individual characters to their spoken names.
 * Used for screen reader spelling mode (e.g., "B" -> "Be", "Č" -> "Če").
 * @param handle Engine handle.
 * @param dictionary_path Path to the spelling dictionary JSON file.
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_load_spelling_dictionary(
    LaprdusHandle handle,
    const char* dictionary_path
);

/**
 * Load a spelling dictionary from memory.
 * @param handle Engine handle.
 * @param json_content JSON content as a string.
 * @param length Length of the content (0 for null-terminated).
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_load_spelling_dictionary_from_memory(
    LaprdusHandle handle,
    const char* json_content,
    size_t length
);

/**
 * Append spelling dictionary entries from a JSON file.
 * Keeps existing entries and adds new ones from the file.
 * @param handle Engine handle.
 * @param dictionary_path Path to the spelling dictionary JSON file.
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_append_spelling_dictionary(
    LaprdusHandle handle,
    const char* dictionary_path
);

/**
 * Clear all entries from the spelling dictionary.
 * @param handle Engine handle.
 */
LAPRDUS_API void LAPRDUS_CALL laprdus_clear_spelling_dictionary(LaprdusHandle handle);

/**
 * Synthesize text in spelling mode (character by character).
 * Each character is converted to its pronunciation using the spelling
 * dictionary, then synthesized with a small pause between characters.
 * @param handle Engine handle.
 * @param text UTF-8 encoded text to spell.
 * @param out_samples Pointer to receive allocated audio sample buffer.
 *                    Caller must free with laprdus_free_buffer().
 * @param out_format Pointer to receive audio format information.
 * @return Number of samples on success, negative error code on failure.
 */
LAPRDUS_API int32_t LAPRDUS_CALL laprdus_synthesize_spelled(
    LaprdusHandle handle,
    const char* text,
    int16_t** out_samples,
    LaprdusAudioFormat* out_format
);

// =============================================================================
// Emoji Dictionary Functions
// =============================================================================

/**
 * Load an emoji dictionary from a JSON file.
 * The emoji dictionary maps emoji characters to their spoken text.
 * @param handle Engine handle.
 * @param dictionary_path Path to the emoji dictionary JSON file.
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_load_emoji_dictionary(
    LaprdusHandle handle,
    const char* dictionary_path
);

/**
 * Load an emoji dictionary from memory.
 * @param handle Engine handle.
 * @param json_content JSON content as a string.
 * @param length Length of the content (0 for null-terminated).
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_load_emoji_dictionary_from_memory(
    LaprdusHandle handle,
    const char* json_content,
    size_t length
);

/**
 * Append emoji dictionary entries from a JSON file.
 * Keeps existing entries and adds new ones from the file.
 * @param handle Engine handle.
 * @param dictionary_path Path to the emoji dictionary JSON file.
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_append_emoji_dictionary(
    LaprdusHandle handle,
    const char* dictionary_path
);

/**
 * Clear all entries from the emoji dictionary.
 * @param handle Engine handle.
 */
LAPRDUS_API void LAPRDUS_CALL laprdus_clear_emoji_dictionary(LaprdusHandle handle);

/**
 * Enable or disable emoji processing.
 * When enabled, emojis are converted to their text representations.
 * Disabled by default on all platforms.
 * @param handle Engine handle.
 * @param enabled Non-zero to enable, zero to disable.
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_emoji_enabled(
    LaprdusHandle handle,
    int enabled
);

/**
 * Check if emoji processing is enabled.
 * @param handle Engine handle.
 * @return Non-zero if enabled, zero if disabled.
 */
LAPRDUS_API int LAPRDUS_CALL laprdus_is_emoji_enabled(LaprdusHandle handle);

// =============================================================================
// Pause Settings
// =============================================================================

/**
 * Set pause duration after sentence-ending punctuation (. ! ?).
 * @param handle Engine handle.
 * @param pause_ms Pause duration in milliseconds (0-2000, default 100).
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_sentence_pause(
    LaprdusHandle handle,
    uint32_t pause_ms
);

/**
 * Set pause duration after commas.
 * @param handle Engine handle.
 * @param pause_ms Pause duration in milliseconds (0-2000, default 100).
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_comma_pause(
    LaprdusHandle handle,
    uint32_t pause_ms
);

/**
 * Set pause duration for newlines.
 * @param handle Engine handle.
 * @param pause_ms Pause duration in milliseconds (0-2000, default 100).
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_newline_pause(
    LaprdusHandle handle,
    uint32_t pause_ms
);

/**
 * Get current sentence pause setting.
 * @param handle Engine handle.
 * @return Pause duration in milliseconds.
 */
LAPRDUS_API uint32_t LAPRDUS_CALL laprdus_get_sentence_pause(LaprdusHandle handle);

/**
 * Get current comma pause setting.
 * @param handle Engine handle.
 * @return Pause duration in milliseconds.
 */
LAPRDUS_API uint32_t LAPRDUS_CALL laprdus_get_comma_pause(LaprdusHandle handle);

/**
 * Get current newline pause setting.
 * @param handle Engine handle.
 * @return Pause duration in milliseconds.
 */
LAPRDUS_API uint32_t LAPRDUS_CALL laprdus_get_newline_pause(LaprdusHandle handle);

/**
 * Set pause duration between spelled characters.
 * @param handle Engine handle.
 * @param pause_ms Pause duration in milliseconds (0-2000, default 200).
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_spelling_pause(
    LaprdusHandle handle,
    uint32_t pause_ms
);

/**
 * Get current spelling pause setting.
 * @param handle Engine handle.
 * @return Pause duration in milliseconds.
 */
LAPRDUS_API uint32_t LAPRDUS_CALL laprdus_get_spelling_pause(LaprdusHandle handle);

// =============================================================================
// Number Processing Mode
// =============================================================================

/**
 * Number processing mode enumeration.
 */
typedef enum LaprdusNumberMode {
    LAPRDUS_NUMBER_MODE_WHOLE = 0,      // "123" -> "sto dvadeset tri" (default)
    LAPRDUS_NUMBER_MODE_DIGIT = 1       // "123" -> "jedan dva tri"
} LaprdusNumberMode;

/**
 * Set number processing mode.
 * @param handle Engine handle.
 * @param mode Number processing mode (LAPRDUS_NUMBER_MODE_WHOLE or LAPRDUS_NUMBER_MODE_DIGIT).
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_set_number_mode(
    LaprdusHandle handle,
    LaprdusNumberMode mode
);

/**
 * Get current number processing mode.
 * @param handle Engine handle.
 * @return Current number mode.
 */
LAPRDUS_API LaprdusNumberMode LAPRDUS_CALL laprdus_get_number_mode(LaprdusHandle handle);

// =============================================================================
// User Configuration Functions
// =============================================================================

/**
 * Get the path to the user configuration directory.
 * - Windows: %APPDATA%\Laprdus
 * - Linux: ~/.config/Laprdus
 * @param buffer Buffer to receive the path.
 * @param buffer_size Size of the buffer.
 * @return Length of the path, or 0 on failure.
 */
LAPRDUS_API size_t LAPRDUS_CALL laprdus_get_config_directory(
    char* buffer,
    size_t buffer_size
);

/**
 * Load user configuration from the user config directory.
 * Creates default settings.json if it doesn't exist.
 * @param handle Engine handle.
 * @return LAPRDUS_OK on success, error code on failure.
 */
LAPRDUS_API LaprdusError LAPRDUS_CALL laprdus_load_user_config(LaprdusHandle handle);

/**
 * Check if a user dictionary file exists.
 * @param filename Name of the dictionary file (e.g., "user.json").
 * @return Non-zero if file exists, zero otherwise.
 */
LAPRDUS_API int LAPRDUS_CALL laprdus_user_dictionary_exists(const char* filename);

/**
 * Get the full path to a user dictionary file.
 * @param filename Name of the dictionary file (e.g., "user.json").
 * @param buffer Buffer to receive the path.
 * @param buffer_size Size of the buffer.
 * @return Length of the path, or 0 on failure.
 */
LAPRDUS_API size_t LAPRDUS_CALL laprdus_get_user_dictionary_path(
    const char* filename,
    char* buffer,
    size_t buffer_size
);

#ifdef __cplusplus
}
#endif

#endif // LAPRDUS_API_H
