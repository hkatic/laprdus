package com.hrvojekatic.laprdus.tts

import android.content.res.AssetManager
import android.util.Log

/**
 * JNI wrapper for the native LaprdusTTS engine.
 * Thread-safe singleton with lazy initialization.
 *
 * Audio output format: 16-bit PCM, 22050 Hz, mono
 */
class LaprdusTTS private constructor() {

    companion object {
        private const val TAG = "LaprdusTTS"
        private const val DICTIONARY_ASSET_PATH = "dictionaries/internal.json"
        private const val SPELLING_DICTIONARY_ASSET_PATH = "dictionaries/spelling.json"
        private const val EMOJI_DICTIONARY_ASSET_PATH = "dictionaries/emoji.json"

        // Number mode constants (matches C++ enum)
        const val NUMBER_MODE_WHOLE = 0
        const val NUMBER_MODE_DIGIT = 1

        init {
            try {
                System.loadLibrary("laprdus")
                Log.i(TAG, "Native library loaded successfully")
            } catch (e: UnsatisfiedLinkError) {
                Log.e(TAG, "Failed to load native library: ${e.message}")
                throw e
            }
        }

        /**
         * Get the library version string from native code
         */
        @JvmStatic
        external fun nativeGetVersion(): String

        @Volatile
        private var instance: LaprdusTTS? = null

        /**
         * Get the singleton instance of the TTS engine
         */
        fun getInstance(): LaprdusTTS {
            return instance ?: synchronized(this) {
                instance ?: LaprdusTTS().also { instance = it }
            }
        }

        /**
         * Get the library version
         */
        fun getVersion(): String = nativeGetVersion()
    }

    // ==========================================================================
    // Native method declarations
    // ==========================================================================

    private external fun nativeInit(phonemeDataPath: String): Boolean
    private external fun nativeInitFromAssets(assetManager: AssetManager, assetPath: String): Boolean
    private external fun nativeShutdown()
    private external fun nativeIsInitialized(): Boolean
    private external fun nativeSynthesize(text: String): ShortArray?
    private external fun nativeSetSpeed(speed: Float)
    private external fun nativeSetPitch(pitch: Float)
    private external fun nativeSetUserPitch(pitch: Float)
    private external fun nativeSetVolume(volume: Float)
    private external fun nativeSetInflectionEnabled(enabled: Boolean)
    private external fun nativeGetSampleRate(): Int
    private external fun nativeCancel()
    private external fun nativeGetVoiceCount(): Int
    private external fun nativeGetVoiceInfo(index: Int): VoiceInfo?
    private external fun nativeSetVoice(voiceId: String, assetManager: AssetManager): Boolean
    private external fun nativeLoadDictionaryFromAssets(assetManager: AssetManager, assetPath: String): Boolean
    private external fun nativeAddPronunciation(grapheme: String, phoneme: String, caseSensitive: Boolean, wholeWord: Boolean)
    private external fun nativeLoadSpellingDictionaryFromAssets(assetManager: AssetManager, assetPath: String): Boolean
    private external fun nativeSynthesizeSpelled(text: String): ShortArray?

    // Emoji dictionary methods
    private external fun nativeLoadEmojiDictionaryFromAssets(assetManager: AssetManager, assetPath: String): Boolean
    private external fun nativeSetEmojiEnabled(enabled: Boolean)
    private external fun nativeIsEmojiEnabled(): Boolean

    // Pause settings methods
    private external fun nativeSetSentencePause(pauseMs: Int)
    private external fun nativeSetCommaPause(pauseMs: Int)
    private external fun nativeSetNewlinePause(pauseMs: Int)
    private external fun nativeSetSpellingPause(pauseMs: Int)
    private external fun nativeGetSentencePause(): Int
    private external fun nativeGetCommaPause(): Int
    private external fun nativeGetNewlinePause(): Int
    private external fun nativeGetSpellingPause(): Int

    // Number mode methods
    private external fun nativeSetNumberMode(mode: Int)
    private external fun nativeGetNumberMode(): Int

    // ==========================================================================
    // Public Kotlin API
    // ==========================================================================

    /**
     * Initialize the TTS engine from a file path
     * @param phonemeDataPath Path to the .bin voice data file
     * @return true if initialization succeeded
     */
    fun initFromFile(phonemeDataPath: String): Boolean {
        Log.d(TAG, "Initializing from file: $phonemeDataPath")
        return nativeInit(phonemeDataPath)
    }

    /**
     * Initialize the TTS engine from APK assets
     * @param assetManager Asset manager from context
     * @param voiceAssetPath Path to voice .bin file within assets (e.g., "Josip.bin")
     * @return true if initialization succeeded
     */
    fun initFromAssets(assetManager: AssetManager, voiceAssetPath: String): Boolean {
        Log.d(TAG, "Initializing from assets: $voiceAssetPath")
        return nativeInitFromAssets(assetManager, voiceAssetPath)
    }

    /**
     * Shutdown the TTS engine and release resources
     */
    fun shutdown() {
        Log.d(TAG, "Shutting down")
        nativeShutdown()
    }

    /**
     * Check if the engine is initialized and ready for synthesis
     */
    fun isInitialized(): Boolean = nativeIsInitialized()

    /**
     * Synthesize text to audio samples
     * @param text UTF-8 text to synthesize (Croatian or Serbian)
     * @return Array of 16-bit PCM samples at 22050 Hz, or null on error
     */
    fun synthesize(text: String): ShortArray? {
        if (text.isBlank()) {
            return ShortArray(0)
        }
        return nativeSynthesize(text)
    }

    /**
     * Speech rate/speed (0.5 - 2.0, default 1.0)
     * Uses Sonic time-stretching - changes tempo without changing pitch
     */
    var speed: Float = 1.0f
        set(value) {
            field = value.coerceIn(0.5f, 2.0f)
            nativeSetSpeed(field)
        }

    /**
     * User pitch preference (0.5 - 2.0, default 1.0)
     * Uses formant-preserving pitch shift - keeps voice character
     */
    var pitch: Float = 1.0f
        set(value) {
            field = value.coerceIn(0.5f, 2.0f)
            nativeSetUserPitch(field)
        }

    /**
     * Volume level (0.0 - 1.0, default 1.0)
     */
    var volume: Float = 1.0f
        set(value) {
            field = value.coerceIn(0.0f, 1.0f)
            nativeSetVolume(field)
        }

    /**
     * Enable/disable punctuation-based inflection
     * When enabled, applies pitch contours based on punctuation marks
     */
    var inflectionEnabled: Boolean = true
        set(value) {
            field = value
            nativeSetInflectionEnabled(value)
        }

    /**
     * Get the audio sample rate (always 22050 Hz)
     */
    val sampleRate: Int
        get() = if (isInitialized()) nativeGetSampleRate() else 22050

    /**
     * Cancel any ongoing synthesis operation
     */
    fun cancel() {
        nativeCancel()
    }

    /**
     * Get the number of available voices
     */
    fun getVoiceCount(): Int = nativeGetVoiceCount()

    /**
     * Get voice information by index
     * @param index Voice index (0 to voiceCount-1)
     * @return VoiceInfo or null if index is out of bounds
     */
    fun getVoiceInfo(index: Int): VoiceInfo? = nativeGetVoiceInfo(index)

    /**
     * Get all available voices
     */
    fun getAllVoices(): List<VoiceInfo> {
        return (0 until getVoiceCount()).mapNotNull { getVoiceInfo(it) }
    }

    /**
     * Set the active voice for synthesis
     * For derived voices (child, grandma, grandpa), this also applies the appropriate pitch
     *
     * @param voiceId Voice ID: "josip", "vlado", "detence", "baba", or "djed"
     * @param assetManager Asset manager to load voice data
     * @return true if voice was set successfully
     */
    fun setVoice(voiceId: String, assetManager: AssetManager): Boolean {
        Log.d(TAG, "Setting voice: $voiceId")
        val success = nativeSetVoice(voiceId, assetManager)
        if (success) {
            // Load pronunciation dictionary after voice is set
            loadDictionary(assetManager)
            // Load spelling dictionary for character-by-character pronunciation
            loadSpellingDictionary(assetManager)
            // Load emoji dictionary (will be used only when emojiEnabled is true)
            loadEmojiDictionary(assetManager)
        }
        return success
    }

    /**
     * Load the pronunciation dictionary from assets
     * This is called automatically when setting a voice, but can be called manually if needed.
     *
     * @param assetManager Asset manager to load dictionary data
     * @return true if dictionary was loaded successfully
     */
    fun loadDictionary(assetManager: AssetManager): Boolean {
        Log.d(TAG, "Loading pronunciation dictionary from: $DICTIONARY_ASSET_PATH")
        val result = nativeLoadDictionaryFromAssets(assetManager, DICTIONARY_ASSET_PATH)
        if (result) {
            Log.i(TAG, "Pronunciation dictionary loaded successfully")
        } else {
            Log.e(TAG, "Failed to load pronunciation dictionary from: $DICTIONARY_ASSET_PATH")
        }
        return result
    }

    /**
     * Add a single pronunciation entry to the dictionary.
     * This appends to the existing dictionary without clearing it,
     * making it suitable for loading user dictionary entries after
     * the bundled dictionary has been loaded.
     *
     * @param grapheme The text to match
     * @param phoneme The replacement pronunciation
     * @param caseSensitive Whether matching is case-sensitive
     * @param wholeWord Whether to match whole words only
     */
    fun addPronunciation(grapheme: String, phoneme: String, caseSensitive: Boolean = false, wholeWord: Boolean = true) {
        nativeAddPronunciation(grapheme, phoneme, caseSensitive, wholeWord)
    }

    /**
     * Load the spelling dictionary from assets
     * This is called automatically when setting a voice, but can be called manually if needed.
     * The spelling dictionary maps individual characters to their pronunciations for
     * character-by-character spelling mode.
     *
     * @param assetManager Asset manager to load spelling dictionary data
     * @return true if spelling dictionary was loaded successfully
     */
    fun loadSpellingDictionary(assetManager: AssetManager): Boolean {
        Log.d(TAG, "Loading spelling dictionary from: $SPELLING_DICTIONARY_ASSET_PATH")
        val result = nativeLoadSpellingDictionaryFromAssets(assetManager, SPELLING_DICTIONARY_ASSET_PATH)
        if (result) {
            Log.i(TAG, "Spelling dictionary loaded successfully")
        } else {
            Log.e(TAG, "Failed to load spelling dictionary from: $SPELLING_DICTIONARY_ASSET_PATH")
        }
        return result
    }

    /**
     * Synthesize text in spelling mode (character by character)
     * Each character is converted to its pronunciation using the spelling dictionary.
     *
     * @param text UTF-8 text to spell
     * @return Array of 16-bit PCM samples at 22050 Hz, or null on error
     */
    fun synthesizeSpelled(text: String): ShortArray? {
        if (text.isBlank()) {
            return ShortArray(0)
        }
        return nativeSynthesizeSpelled(text)
    }

    // ==========================================================================
    // Emoji Dictionary
    // ==========================================================================

    /**
     * Load the emoji dictionary from assets
     * This is called automatically when setting a voice and emoji is enabled.
     *
     * @param assetManager Asset manager to load emoji dictionary data
     * @return true if emoji dictionary was loaded successfully
     */
    fun loadEmojiDictionary(assetManager: AssetManager): Boolean {
        Log.d(TAG, "Loading emoji dictionary from: $EMOJI_DICTIONARY_ASSET_PATH")
        val result = nativeLoadEmojiDictionaryFromAssets(assetManager, EMOJI_DICTIONARY_ASSET_PATH)
        if (result) {
            Log.i(TAG, "Emoji dictionary loaded successfully")
        } else {
            Log.e(TAG, "Failed to load emoji dictionary from: $EMOJI_DICTIONARY_ASSET_PATH")
        }
        return result
    }

    /**
     * Enable/disable emoji to text conversion
     * When enabled, emojis are converted to their text representations.
     * Disabled by default.
     */
    var emojiEnabled: Boolean
        set(value) {
            nativeSetEmojiEnabled(value)
        }
        get() = nativeIsEmojiEnabled()

    // ==========================================================================
    // Pause Settings
    // ==========================================================================

    /**
     * Pause duration after sentence-ending punctuation (. ! ?) in milliseconds
     * Range: 0-2000, default 100
     */
    var sentencePause: Int
        get() = nativeGetSentencePause()
        set(value) {
            nativeSetSentencePause(value.coerceIn(0, 2000))
        }

    /**
     * Pause duration after commas in milliseconds
     * Range: 0-2000, default 100
     */
    var commaPause: Int
        get() = nativeGetCommaPause()
        set(value) {
            nativeSetCommaPause(value.coerceIn(0, 2000))
        }

    /**
     * Pause duration for newlines in milliseconds
     * Range: 0-2000, default 100
     */
    var newlinePause: Int
        get() = nativeGetNewlinePause()
        set(value) {
            nativeSetNewlinePause(value.coerceIn(0, 2000))
        }

    /**
     * Pause duration between spelled characters in milliseconds
     * Range: 0-2000, default 200
     */
    var spellingPause: Int
        get() = nativeGetSpellingPause()
        set(value) {
            nativeSetSpellingPause(value.coerceIn(0, 2000))
        }

    // ==========================================================================
    // Number Processing Mode
    // ==========================================================================

    /**
     * Number processing mode:
     * - NUMBER_MODE_WHOLE (0): "123" -> "sto dvadeset tri" (default)
     * - NUMBER_MODE_DIGIT (1): "123" -> "jedan dva tri"
     */
    var numberMode: Int
        get() = nativeGetNumberMode()
        set(value) {
            nativeSetNumberMode(value.coerceIn(NUMBER_MODE_WHOLE, NUMBER_MODE_DIGIT))
        }
}
