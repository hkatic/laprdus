package com.hrvojekatic.laprdus.tts

import android.content.res.AssetManager
import android.util.Log
import com.hrvojekatic.laprdus.data.SettingsRepository
import java.io.File

/**
 * JNI wrapper for the native LaprdusTTS engine.
 *
 * There are two engines, one per client: [service] for the speech service
 * (TalkBack and every other app) and [app] for the app's own screens. Each
 * has its own voice, rate, pitch, volume and dictionaries, so nothing one
 * of them sets ever reaches the other. Both are thread-safe.
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

        /** The engine of the speech service: TalkBack and every other app. */
        val service: LaprdusTTS by lazy { LaprdusTTS() }

        /**
         * The engine of the app's own screens (the preview and the settings).
         * What the app sets here never changes what TalkBack says, and
         * TalkBack's requests never change the preview.
         */
        val app: LaprdusTTS by lazy { LaprdusTTS() }

        /**
         * Get the library version
         */
        fun getVersion(): String = nativeGetVersion()
    }

    // ==========================================================================
    // Native method declarations
    // ==========================================================================

    private external fun nativeCreate(): Long
    private external fun nativeShutdown(handle: Long)
    private external fun nativeIsInitialized(handle: Long): Boolean
    private external fun nativeSynthesize(handle: Long, text: String): ShortArray?
    private external fun nativeSetSpeed(handle: Long, speed: Float)
    private external fun nativeSetPitch(handle: Long, pitch: Float)
    private external fun nativeSetUserPitch(handle: Long, pitch: Float)
    private external fun nativeSetVolume(handle: Long, volume: Float)
    private external fun nativeSetInflectionEnabled(handle: Long, enabled: Boolean)
    private external fun nativeSetInflectionLevel(handle: Long, level: Float)
    private external fun nativeSetAcceleration(handle: Long, acceleration: Float)
    private external fun nativeGetNominalWpm(voiceId: String): Float
    private external fun nativeGetSampleRate(handle: Long): Int
    private external fun nativeCancel()
    private external fun nativeGetVoiceCount(): Int
    private external fun nativeGetVoiceInfo(index: Int): VoiceInfo?
    private external fun nativeSetVoice(handle: Long, voiceId: String, assetManager: AssetManager): Boolean
    private external fun nativeLoadDictionaryFromAssets(handle: Long, assetManager: AssetManager, assetPath: String): Boolean
    private external fun nativeAddPronunciation(handle: Long, grapheme: String, phoneme: String, caseSensitive: Boolean, wholeWord: Boolean)
    private external fun nativeAddSpellingEntry(handle: Long, character: String, pronunciation: String)
    private external fun nativeAddEmojiEntry(handle: Long, emoji: String, text: String)
    private external fun nativeLoadAccentLexicon(handle: Long, json: String): Boolean
    private external fun nativeClearAccentLexicon(handle: Long)
    private external fun nativeLoadSpellingDictionaryFromAssets(handle: Long, assetManager: AssetManager, assetPath: String): Boolean
    private external fun nativeSynthesizeSpelled(handle: Long, text: String): ShortArray?

    // Emoji dictionary methods
    private external fun nativeLoadEmojiDictionaryFromAssets(handle: Long, assetManager: AssetManager, assetPath: String): Boolean
    private external fun nativeSetEmojiEnabled(handle: Long, enabled: Boolean)
    private external fun nativeIsEmojiEnabled(handle: Long): Boolean

    // Pause settings methods
    private external fun nativeSetSentencePause(handle: Long, pauseMs: Int)
    private external fun nativeSetCommaPause(handle: Long, pauseMs: Int)
    private external fun nativeSetNewlinePause(handle: Long, pauseMs: Int)
    private external fun nativeSetSpellingPause(handle: Long, pauseMs: Int)
    private external fun nativeGetSentencePause(handle: Long): Int
    private external fun nativeGetCommaPause(handle: Long): Int
    private external fun nativeGetNewlinePause(handle: Long): Int
    private external fun nativeGetSpellingPause(handle: Long): Int

    // Spelling speed and mode methods
    private external fun nativeSetSpellingSpeed(handle: Long, percent: Int)
    private external fun nativeGetSpellingSpeed(handle: Long): Int
    private external fun nativeSetLetterSounds(handle: Long, enabled: Boolean)
    private external fun nativeGetLetterSounds(handle: Long): Boolean

    // Number mode methods
    private external fun nativeSetNumberMode(handle: Long, mode: Int)
    private external fun nativeGetNumberMode(handle: Long): Int

    // ==========================================================================
    // Public Kotlin API
    // ==========================================================================

    /** This object's native engine; it lives as long as the process. */
    private val handle: Long = nativeCreate()

    private val clientLock = Any()

    /**
     * Runs [block] with this engine to itself: no other thread can change
     * its voice, rate, pitch, volume or dictionaries in between, so "set the
     * request's rate and pitch, then synthesize" happens as one step.
     * [setVoice] and [reloadDictionaries] take it too. [cancel] does not wait
     * for it.
     */
    fun <T> exclusive(block: () -> T): T = synchronized(clientLock) { block() }

    /**
     * The folder with the user's own dictionaries, set by this engine's
     * client. [setVoice] and [reloadDictionaries] load them on top of the
     * bundled dictionaries; null loads none.
     */
    @Volatile
    var userDictionaryDir: File? = null

    /** Whether the user's own dictionaries are used (the settings switch). */
    @Volatile
    var userDictionariesEnabled: Boolean = true

    /** The voice this engine has loaded, or null before the first [setVoice]. */
    @Volatile
    var currentVoiceId: String? = null
        private set

    /**
     * Shutdown the TTS engine and release resources
     */
    fun shutdown() {
        Log.d(TAG, "Shutting down")
        exclusive {
            nativeShutdown(handle)
            currentVoiceId = null
        }
    }

    /**
     * Check if the engine is initialized and ready for synthesis
     */
    fun isInitialized(): Boolean = nativeIsInitialized(handle)

    /**
     * Synthesize text to audio samples
     * @param text UTF-8 text to synthesize (Croatian or Serbian)
     * @return Array of 16-bit PCM samples at 22050 Hz, or null on error
     */
    fun synthesize(text: String): ShortArray? {
        if (text.isBlank()) {
            return ShortArray(0)
        }
        return nativeSynthesize(handle, text)
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
            nativeSetSpeed(handle, field)
        }

    /**
     * User pitch preference (0.25 - 4.0, default 1.0).
     * The recorded voices narrow it to 0.5 - 2.0 and use a formant-preserving
     * pitch shift that keeps the voice character.
     */
    var pitch: Float = 1.0f
        set(value) {
            field = value.coerceIn(VoiceInfo.FORMANT_RANGE)
            nativeSetUserPitch(handle, field)
        }

    /**
     * Volume level (0.0 - 1.0, default 1.0)
     */
    var volume: Float = 1.0f
        set(value) {
            field = value.coerceIn(0.0f, 1.0f)
            nativeSetVolume(handle, field)
        }

    /**
     * Enable/disable punctuation-based inflection
     * When enabled, applies pitch contours based on punctuation marks
     */
    var inflectionEnabled: Boolean = true
        set(value) {
            field = value
            nativeSetInflectionEnabled(handle, value)
        }

    /**
     * Inflection level of the formant voices (0.0 - 1.0, default 0.5):
     * 0.0 is a monotone, 0.5 the measured pitch movements, 1.0 twice those.
     * The recorded voices ignore it.
     */
    var inflectionLevel: Float = DEFAULT_INFLECTION_LEVEL
        set(value) {
            field = value.coerceIn(0.0f, 1.0f)
            nativeSetInflectionLevel(handle, field)
        }

    /**
     * Acceleration of the formant voices (0.5 - 3.0, default 1.0): a
     * multiplier on [speed], so the top of the rate slider reaches a higher
     * (or lower) rate. The recorded voices ignore it.
     */
    var acceleration: Float = DEFAULT_ACCELERATION
        set(value) {
            field = value.coerceIn(ACCELERATION_RANGE)
            nativeSetAcceleration(handle, field)
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
        get() = if (isInitialized()) nativeGetSampleRate(handle) else 22050

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
    fun setVoice(voiceId: String, assetManager: AssetManager): Boolean = exclusive {
        Log.d(TAG, "Setting voice: $voiceId")
        val success = nativeSetVoice(handle, voiceId, assetManager)
        if (success) {
            currentVoiceId = voiceId
            loadAllDictionaries(assetManager)
        }
        success
    }

    /**
     * Reload the bundled dictionaries and the user's on top of them, so an
     * edited user dictionary, or switching the user dictionaries on or off,
     * takes effect without reloading the voice.
     *
     * @return false when no voice is loaded yet (the next [setVoice] loads them)
     */
    fun reloadDictionaries(assetManager: AssetManager): Boolean = exclusive {
        if (!isInitialized()) return@exclusive false
        loadAllDictionaries(assetManager)
        true
    }

    /**
     * The bundled dictionaries, which replace every entry the engine had,
     * then the user's entries on top of them.
     */
    private fun loadAllDictionaries(assetManager: AssetManager) {
        // Pronunciation dictionary
        loadDictionary(assetManager)
        // Spelling dictionary for character-by-character pronunciation
        loadSpellingDictionary(assetManager)
        // Emoji dictionary (used only when emojiEnabled is true)
        loadEmojiDictionary(assetManager)

        val dir = userDictionaryDir ?: return
        if (userDictionariesEnabled) {
            UserDictionaries.load(this, dir)
        } else {
            clearAccentLexicon()
        }
    }

    /**
     * Applies the saved settings that every utterance shares: everything but
     * the voice, the rate, the pitch and the volume, which each client sets
     * itself. The user dictionaries switch takes effect on the next
     * [setVoice] or [reloadDictionaries].
     */
    fun applySettings(settings: SettingsRepository.TTSSettings) {
        emojiEnabled = settings.emojiEnabled
        inflectionEnabled = settings.inflectionEnabled
        inflectionLevel = settings.inflectionLevel
        acceleration = settings.acceleration
        sentencePause = settings.sentencePause
        commaPause = settings.commaPause
        newlinePause = settings.newlinePause
        numberMode = settings.numberMode
        spellingSpeed = settings.spellingSpeed
        letterSounds = settings.spellingMode == SettingsRepository.SPELLING_MODE_SOUNDS
        userDictionariesEnabled = settings.userDictionariesEnabled
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
        val result = nativeLoadDictionaryFromAssets(handle, assetManager, DICTIONARY_ASSET_PATH)
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
        nativeAddPronunciation(handle, grapheme, phoneme, caseSensitive, wholeWord)
    }

    /**
     * Add a single entry to the spelling dictionary, replacing the bundled
     * name of that character if there is one.
     *
     * @param character The character to match
     * @param pronunciation How the character is named when spelling
     */
    fun addSpellingEntry(character: String, pronunciation: String) {
        nativeAddSpellingEntry(handle, character, pronunciation)
    }

    /**
     * Add a single entry to the emoji dictionary, replacing the bundled
     * description of that emoji if there is one.
     *
     * @param emoji The emoji to match
     * @param text The text spoken for it
     */
    fun addEmojiEntry(emoji: String, text: String) {
        nativeAddEmojiEntry(handle, emoji, text)
    }

    /**
     * Load the user's accent lexicon (the content of accents.json) for the
     * formant voices, replacing the current one. It is kept across voice
     * changes. Malformed entries are skipped and logged by the engine.
     *
     * @param json The lexicon file's content
     * @return true if at least one entry was accepted
     */
    fun loadAccentLexicon(json: String): Boolean = nativeLoadAccentLexicon(handle, json)

    /** Remove the user's accent lexicon. */
    fun clearAccentLexicon() {
        nativeClearAccentLexicon(handle)
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
        val result = nativeLoadSpellingDictionaryFromAssets(handle, assetManager, SPELLING_DICTIONARY_ASSET_PATH)
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
        return nativeSynthesizeSpelled(handle, text)
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
        val result = nativeLoadEmojiDictionaryFromAssets(handle, assetManager, EMOJI_DICTIONARY_ASSET_PATH)
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
            nativeSetEmojiEnabled(handle, value)
        }
        get() = nativeIsEmojiEnabled(handle)

    // ==========================================================================
    // Pause Settings
    // ==========================================================================

    /**
     * Pause duration after sentence-ending punctuation (. ! ?) in milliseconds
     * Range: 0-2000, default 100
     */
    var sentencePause: Int
        get() = nativeGetSentencePause(handle)
        set(value) {
            nativeSetSentencePause(handle, value.coerceIn(0, 2000))
        }

    /**
     * Pause duration after commas in milliseconds
     * Range: 0-2000, default 100
     */
    var commaPause: Int
        get() = nativeGetCommaPause(handle)
        set(value) {
            nativeSetCommaPause(handle, value.coerceIn(0, 2000))
        }

    /**
     * Pause duration for newlines in milliseconds
     * Range: 0-2000, default 100
     */
    var newlinePause: Int
        get() = nativeGetNewlinePause(handle)
        set(value) {
            nativeSetNewlinePause(handle, value.coerceIn(0, 2000))
        }

    /**
     * Pause duration between spelled characters in milliseconds
     * Range: 0-2000, default 200
     */
    var spellingPause: Int
        get() = nativeGetSpellingPause(handle)
        set(value) {
            nativeSetSpellingPause(handle, value.coerceIn(0, 2000))
        }

    /**
     * Speed of spelled characters as a percentage of the speech rate:
     * 100 is the speech rate itself, 50 (the default) a little over half
     * of it, 0 a third.
     * Range: 0-100, default 50
     */
    var spellingSpeed: Int
        get() = nativeGetSpellingSpeed(handle)
        set(value) {
            nativeSetSpellingSpeed(handle, value.coerceIn(0, 100))
        }

    /**
     * How letters are read when spelling: false reads letter names
     * ("be", "ce", "de"; the default), true the sound of each letter.
     */
    var letterSounds: Boolean
        get() = nativeGetLetterSounds(handle)
        set(value) {
            nativeSetLetterSounds(handle, value)
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
        get() = nativeGetNumberMode(handle)
        set(value) {
            nativeSetNumberMode(handle, value.coerceIn(NUMBER_MODE_WHOLE, NUMBER_MODE_DIGIT))
        }
}
