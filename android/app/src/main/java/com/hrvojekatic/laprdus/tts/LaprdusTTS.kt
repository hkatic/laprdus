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

        const val DEFAULT_INFLECTION_LEVEL = 0.5f
        const val DEFAULT_ACCELERATION = 1.0f
        val ACCELERATION_RANGE = 0.5f..3.0f

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

    private external fun nativeShutdown()
    private external fun nativeIsInitialized(): Boolean
    private external fun nativeSynthesize(text: String): ShortArray?
    private external fun nativeSetSpeed(speed: Float)
    private external fun nativeSetPitch(pitch: Float)
    private external fun nativeSetUserPitch(pitch: Float)
    private external fun nativeSetVolume(volume: Float)
    private external fun nativeSetInflectionEnabled(enabled: Boolean)
    private external fun nativeSetInflectionLevel(level: Float)
    private external fun nativeSetAcceleration(acceleration: Float)
    private external fun nativeGetNominalWpm(voiceId: String): Float
    private external fun nativeGetSampleRate(): Int
    private external fun nativeCancel()
    private external fun nativeGetVoiceCount(): Int
    private external fun nativeGetVoiceInfo(index: Int): VoiceInfo?
    private external fun nativeSetVoice(voiceId: String, assetManager: AssetManager): Boolean
    private external fun nativeLoadDictionaryFromAssets(assetManager: AssetManager, assetPath: String): Boolean
    private external fun nativeAddPronunciation(grapheme: String, phoneme: String, caseSensitive: Boolean, wholeWord: Boolean)
    private external fun nativeAddSpellingEntry(character: String, pronunciation: String)
    private external fun nativeAddEmojiEntry(emoji: String, text: String)
    private external fun nativeLoadAccentLexicon(json: String): Boolean
    private external fun nativeClearAccentLexicon()
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

    // Spelling speed and mode methods
    private external fun nativeSetSpellingSpeed(percent: Int)
    private external fun nativeGetSpellingSpeed(): Int
    private external fun nativeSetLetterSounds(enabled: Boolean)
    private external fun nativeGetLetterSounds(): Boolean

    // Number mode methods
    private external fun nativeSetNumberMode(mode: Int)
    private external fun nativeGetNumberMode(): Int

    // ==========================================================================
    // Public Kotlin API
    // ==========================================================================

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
     * Speech rate/speed (0.25 - 4.0, default 1.0).
     * The recorded voices narrow it to 0.5 - 2.0 and time-stretch with Sonic;
     * the formant voices apply it at the source and multiply it by
     * [acceleration].
     */
    var speed: Float = 1.0f
        set(value) {
            field = value.coerceIn(VoiceInfo.FORMANT_RANGE)
            nativeSetSpeed(field)
        }

    /**
     * User pitch preference (0.25 - 4.0, default 1.0).
     * The recorded voices narrow it to 0.5 - 2.0 and use a formant-preserving
     * pitch shift that keeps the voice character.
     */
    var pitch: Float = 1.0f
        set(value) {
            field = value.coerceIn(VoiceInfo.FORMANT_RANGE)
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
     * Inflection level of the formant voices (0.0 - 1.0, default 0.5):
     * 0.0 is a monotone, 0.5 the measured pitch movements, 1.0 twice those.
     * The recorded voices ignore it.
     */
    var inflectionLevel: Float = DEFAULT_INFLECTION_LEVEL
        set(value) {
            field = value.coerceIn(0.0f, 1.0f)
            nativeSetInflectionLevel(field)
        }

    /**
     * Acceleration of the formant voices (0.5 - 3.0, default 1.0): a
     * multiplier on [speed], so the top of the rate slider reaches a higher
     * (or lower) rate. The recorded voices ignore it.
     */
    var acceleration: Float = DEFAULT_ACCELERATION
        set(value) {
            field = value.coerceIn(ACCELERATION_RANGE)
            nativeSetAcceleration(field)
        }

    /**
     * Words per minute a voice speaks at speed 1.0 and acceleration 1.0
     * (0 for an unknown voice).
     */
    fun getNominalWpm(voiceId: String): Float = nativeGetNominalWpm(voiceId)

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
     * @param voiceId Voice ID: "zvonko", "stojan", "mirsad", "josip", "vlado", "detence", "baba" or "djed"
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
     * Add a single entry to the spelling dictionary, replacing the bundled
     * name of that character if there is one.
     *
     * @param character The character to match
     * @param pronunciation How the character is named when spelling
     */
    fun addSpellingEntry(character: String, pronunciation: String) {
        nativeAddSpellingEntry(character, pronunciation)
    }

    /**
     * Add a single entry to the emoji dictionary, replacing the bundled
     * description of that emoji if there is one.
     *
     * @param emoji The emoji to match
     * @param text The text spoken for it
     */
    fun addEmojiEntry(emoji: String, text: String) {
        nativeAddEmojiEntry(emoji, text)
    }

    /**
     * Load the user's accent lexicon (the content of accents.json) for the
     * formant voices, replacing the current one. It is kept across voice
     * changes. Malformed entries are skipped and logged by the engine.
     *
     * @param json The lexicon file's content
     * @return true if at least one entry was accepted
     */
    fun loadAccentLexicon(json: String): Boolean = nativeLoadAccentLexicon(json)

    /** Remove the user's accent lexicon. */
    fun clearAccentLexicon() {
        nativeClearAccentLexicon()
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
        // Only an empty string is skipped: whitespace has spelling entries (" " -> "razmak")
        if (text.isEmpty()) {
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

    /**
     * Speed of spelled characters as a percentage of the speech rate:
     * 100 is the speech rate itself, 50 (the default) a little over half
     * of it, 0 a third.
     * Range: 0-100, default 50
     */
    var spellingSpeed: Int
        get() = nativeGetSpellingSpeed()
        set(value) {
            nativeSetSpellingSpeed(value.coerceIn(0, 100))
        }

    /**
     * How letters are read when spelling: false reads letter names
     * ("be", "ce", "de"; the default), true the sound of each letter.
     */
    var letterSounds: Boolean
        get() = nativeGetLetterSounds()
        set(value) {
            nativeSetLetterSounds(value)
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
