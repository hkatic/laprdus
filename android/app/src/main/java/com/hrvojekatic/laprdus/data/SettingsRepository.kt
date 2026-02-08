package com.hrvojekatic.laprdus.data

import android.content.Context
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.booleanPreferencesKey
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.floatPreferencesKey
import androidx.datastore.preferences.core.intPreferencesKey
import androidx.datastore.preferences.core.stringPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map

/**
 * Extension property to create DataStore
 */
private val Context.dataStore: DataStore<Preferences> by preferencesDataStore(name = "laprdus_settings")

/**
 * Repository for persisting TTS settings using Jetpack DataStore.
 * Stores user preferences for voice, speed, pitch, volume, and force settings.
 *
 * For testing, use the constructor that accepts a DataStore directly.
 */
class SettingsRepository internal constructor(private val dataStore: DataStore<Preferences>) {

    /**
     * Primary constructor for production use - uses Context's DataStore
     */
    constructor(context: Context) : this(context.dataStore)

    companion object {
        // Preference keys
        private val KEY_DEFAULT_VOICE = stringPreferencesKey("default_voice")
        private val KEY_SPEED = floatPreferencesKey("speed")
        private val KEY_PITCH = floatPreferencesKey("pitch")
        private val KEY_VOLUME = floatPreferencesKey("volume")

        // Force settings keys
        private val KEY_FORCE_SPEED = booleanPreferencesKey("force_speed")
        private val KEY_FORCE_PITCH = booleanPreferencesKey("force_pitch")
        private val KEY_FORCE_VOLUME = booleanPreferencesKey("force_volume")
        private val KEY_FORCE_LANGUAGE = booleanPreferencesKey("force_language")

        // Advanced settings keys
        private val KEY_EMOJI_ENABLED = booleanPreferencesKey("emoji_enabled")
        private val KEY_INFLECTION_ENABLED = booleanPreferencesKey("inflection_enabled")
        private val KEY_SENTENCE_PAUSE = intPreferencesKey("sentence_pause")
        private val KEY_COMMA_PAUSE = intPreferencesKey("comma_pause")
        private val KEY_NEWLINE_PAUSE = intPreferencesKey("newline_pause")
        private val KEY_NUMBER_MODE = intPreferencesKey("number_mode")

        // Default TTS dialog preference
        private val KEY_DONT_ASK_DEFAULT_TTS = booleanPreferencesKey("dont_ask_default_tts")

        // User dictionaries setting
        private val KEY_USER_DICTIONARIES_ENABLED = booleanPreferencesKey("user_dictionaries_enabled")

        // Default values
        const val DEFAULT_VOICE = "josip"
        const val DEFAULT_SPEED = 1.0f
        const val DEFAULT_PITCH = 1.0f
        const val DEFAULT_VOLUME = 1.0f
        const val DEFAULT_FORCE_SPEED = false
        const val DEFAULT_FORCE_PITCH = false
        const val DEFAULT_FORCE_VOLUME = false
        const val DEFAULT_FORCE_LANGUAGE = false

        // Advanced settings defaults
        const val DEFAULT_EMOJI_ENABLED = false
        const val DEFAULT_INFLECTION_ENABLED = true
        const val DEFAULT_SENTENCE_PAUSE = 100
        const val DEFAULT_COMMA_PAUSE = 100
        const val DEFAULT_NEWLINE_PAUSE = 100
        const val DEFAULT_NUMBER_MODE = 0  // Whole numbers

        // Default TTS dialog defaults
        const val DEFAULT_DONT_ASK_DEFAULT_TTS = false

        // User dictionaries defaults
        const val DEFAULT_USER_DICTIONARIES_ENABLED = true
    }

    // ==========================================================================
    // Voice Settings
    // ==========================================================================

    /**
     * Flow of the default voice ID
     */
    val defaultVoice: Flow<String> = dataStore.data
        .map { preferences ->
            preferences[KEY_DEFAULT_VOICE] ?: DEFAULT_VOICE
        }

    /**
     * Set the default voice
     * @param voiceId Voice ID: "josip", "vlado", "detence", "baba", or "djed"
     */
    suspend fun setDefaultVoice(voiceId: String) {
        dataStore.edit { preferences ->
            preferences[KEY_DEFAULT_VOICE] = voiceId
        }
    }

    // ==========================================================================
    // Speed Settings
    // ==========================================================================

    /**
     * Flow of the speech speed setting (0.5 - 2.0)
     */
    val speed: Flow<Float> = dataStore.data
        .map { preferences ->
            preferences[KEY_SPEED] ?: DEFAULT_SPEED
        }

    /**
     * Set the speech speed
     * @param speed Speed factor (0.5 - 2.0)
     */
    suspend fun setSpeed(speed: Float) {
        dataStore.edit { preferences ->
            preferences[KEY_SPEED] = speed.coerceIn(0.5f, 2.0f)
        }
    }

    // ==========================================================================
    // Pitch Settings
    // ==========================================================================

    /**
     * Flow of the pitch setting (0.5 - 2.0)
     */
    val pitch: Flow<Float> = dataStore.data
        .map { preferences ->
            preferences[KEY_PITCH] ?: DEFAULT_PITCH
        }

    /**
     * Set the pitch
     * @param pitch Pitch factor (0.5 - 2.0)
     */
    suspend fun setPitch(pitch: Float) {
        dataStore.edit { preferences ->
            preferences[KEY_PITCH] = pitch.coerceIn(0.5f, 2.0f)
        }
    }

    // ==========================================================================
    // Volume Settings
    // ==========================================================================

    /**
     * Flow of the volume setting (0.0 - 1.0)
     */
    val volume: Flow<Float> = dataStore.data
        .map { preferences ->
            preferences[KEY_VOLUME] ?: DEFAULT_VOLUME
        }

    /**
     * Set the volume
     * @param volume Volume level (0.0 - 1.0)
     */
    suspend fun setVolume(volume: Float) {
        dataStore.edit { preferences ->
            preferences[KEY_VOLUME] = volume.coerceIn(0.0f, 1.0f)
        }
    }

    // ==========================================================================
    // Force Speed Settings
    // ==========================================================================

    /**
     * Flow of the force speed setting.
     * When enabled, Laprdus speed settings override application-provided speed.
     */
    val forceSpeed: Flow<Boolean> = dataStore.data
        .map { preferences ->
            preferences[KEY_FORCE_SPEED] ?: DEFAULT_FORCE_SPEED
        }

    /**
     * Set whether to force Laprdus speed settings over application settings.
     * @param enabled True to force Laprdus speed settings
     */
    suspend fun setForceSpeed(enabled: Boolean) {
        dataStore.edit { preferences ->
            preferences[KEY_FORCE_SPEED] = enabled
        }
    }

    // ==========================================================================
    // Force Pitch Settings
    // ==========================================================================

    /**
     * Flow of the force pitch setting.
     * When enabled, Laprdus pitch settings override application-provided pitch.
     */
    val forcePitch: Flow<Boolean> = dataStore.data
        .map { preferences ->
            preferences[KEY_FORCE_PITCH] ?: DEFAULT_FORCE_PITCH
        }

    /**
     * Set whether to force Laprdus pitch settings over application settings.
     * @param enabled True to force Laprdus pitch settings
     */
    suspend fun setForcePitch(enabled: Boolean) {
        dataStore.edit { preferences ->
            preferences[KEY_FORCE_PITCH] = enabled
        }
    }

    // ==========================================================================
    // Force Volume Settings
    // ==========================================================================

    /**
     * Flow of the force volume setting.
     * When enabled, Laprdus volume settings override multimedia volume.
     */
    val forceVolume: Flow<Boolean> = dataStore.data
        .map { preferences ->
            preferences[KEY_FORCE_VOLUME] ?: DEFAULT_FORCE_VOLUME
        }

    /**
     * Set whether to force Laprdus volume settings over multimedia volume.
     * @param enabled True to force Laprdus volume settings
     */
    suspend fun setForceVolume(enabled: Boolean) {
        dataStore.edit { preferences ->
            preferences[KEY_FORCE_VOLUME] = enabled
        }
    }

    // ==========================================================================
    // Force Language Settings
    // ==========================================================================

    /**
     * Flow of the force language setting.
     * When enabled, the selected language is used regardless of system settings.
     */
    val forceLanguage: Flow<Boolean> = dataStore.data
        .map { preferences ->
            preferences[KEY_FORCE_LANGUAGE] ?: DEFAULT_FORCE_LANGUAGE
        }

    /**
     * Set whether to force the selected language regardless of system settings.
     * @param enabled True to force the selected language
     */
    suspend fun setForceLanguage(enabled: Boolean) {
        dataStore.edit { preferences ->
            preferences[KEY_FORCE_LANGUAGE] = enabled
        }
    }

    // ==========================================================================
    // Emoji Settings
    // ==========================================================================

    /**
     * Flow of the emoji enabled setting.
     * When enabled, emojis are converted to their text representations.
     * Disabled by default.
     */
    val emojiEnabled: Flow<Boolean> = dataStore.data
        .map { preferences ->
            preferences[KEY_EMOJI_ENABLED] ?: DEFAULT_EMOJI_ENABLED
        }

    /**
     * Set whether emoji processing is enabled.
     * @param enabled True to enable emoji to text conversion
     */
    suspend fun setEmojiEnabled(enabled: Boolean) {
        dataStore.edit { preferences ->
            preferences[KEY_EMOJI_ENABLED] = enabled
        }
    }

    // ==========================================================================
    // Inflection Settings
    // ==========================================================================

    /**
     * Flow of the inflection enabled setting.
     * When enabled, pitch varies based on punctuation (questions rise, exclamations emphasize).
     * Enabled by default.
     */
    val inflectionEnabled: Flow<Boolean> = dataStore.data
        .map { preferences ->
            preferences[KEY_INFLECTION_ENABLED] ?: DEFAULT_INFLECTION_ENABLED
        }

    /**
     * Set whether voice inflection is enabled.
     * @param enabled True to enable pitch variation for questions, exclamations, and pauses
     */
    suspend fun setInflectionEnabled(enabled: Boolean) {
        dataStore.edit { preferences ->
            preferences[KEY_INFLECTION_ENABLED] = enabled
        }
    }

    // ==========================================================================
    // Pause Settings
    // ==========================================================================

    /**
     * Flow of the sentence pause setting (milliseconds).
     * Pause duration after sentence-ending punctuation (. ! ?).
     */
    val sentencePause: Flow<Int> = dataStore.data
        .map { preferences ->
            preferences[KEY_SENTENCE_PAUSE] ?: DEFAULT_SENTENCE_PAUSE
        }

    /**
     * Set the sentence pause duration.
     * @param pauseMs Pause duration in milliseconds (0-2000)
     */
    suspend fun setSentencePause(pauseMs: Int) {
        dataStore.edit { preferences ->
            preferences[KEY_SENTENCE_PAUSE] = pauseMs.coerceIn(0, 2000)
        }
    }

    /**
     * Flow of the comma pause setting (milliseconds).
     */
    val commaPause: Flow<Int> = dataStore.data
        .map { preferences ->
            preferences[KEY_COMMA_PAUSE] ?: DEFAULT_COMMA_PAUSE
        }

    /**
     * Set the comma pause duration.
     * @param pauseMs Pause duration in milliseconds (0-2000)
     */
    suspend fun setCommaPause(pauseMs: Int) {
        dataStore.edit { preferences ->
            preferences[KEY_COMMA_PAUSE] = pauseMs.coerceIn(0, 2000)
        }
    }

    /**
     * Flow of the newline pause setting (milliseconds).
     */
    val newlinePause: Flow<Int> = dataStore.data
        .map { preferences ->
            preferences[KEY_NEWLINE_PAUSE] ?: DEFAULT_NEWLINE_PAUSE
        }

    /**
     * Set the newline pause duration.
     * @param pauseMs Pause duration in milliseconds (0-2000)
     */
    suspend fun setNewlinePause(pauseMs: Int) {
        dataStore.edit { preferences ->
            preferences[KEY_NEWLINE_PAUSE] = pauseMs.coerceIn(0, 2000)
        }
    }

    // ==========================================================================
    // Number Mode Settings
    // ==========================================================================

    /**
     * Flow of the number processing mode.
     * 0 = Whole numbers (default): "123" -> "sto dvadeset tri"
     * 1 = Digit by digit: "123" -> "jedan dva tri"
     */
    val numberMode: Flow<Int> = dataStore.data
        .map { preferences ->
            preferences[KEY_NUMBER_MODE] ?: DEFAULT_NUMBER_MODE
        }

    /**
     * Set the number processing mode.
     * @param mode 0 for whole numbers, 1 for digit by digit
     */
    suspend fun setNumberMode(mode: Int) {
        dataStore.edit { preferences ->
            preferences[KEY_NUMBER_MODE] = mode.coerceIn(0, 1)
        }
    }

    // ==========================================================================
    // Don't Ask Default TTS Settings
    // ==========================================================================

    /**
     * Flow of the "don't ask about default TTS" setting.
     * When true, the app won't show the default TTS dialog on launch.
     */
    val dontAskDefaultTts: Flow<Boolean> = dataStore.data
        .map { preferences ->
            preferences[KEY_DONT_ASK_DEFAULT_TTS] ?: DEFAULT_DONT_ASK_DEFAULT_TTS
        }

    /**
     * Set whether to suppress the default TTS dialog.
     * @param enabled True to never show the dialog again
     */
    suspend fun setDontAskDefaultTts(enabled: Boolean) {
        dataStore.edit { preferences ->
            preferences[KEY_DONT_ASK_DEFAULT_TTS] = enabled
        }
    }

    // ==========================================================================
    // User Dictionaries Settings
    // ==========================================================================

    /**
     * Flow of the user dictionaries enabled setting.
     * When enabled, user dictionaries (user.json, spelling.json, emoji.json) are applied during synthesis.
     * Enabled by default.
     */
    val userDictionariesEnabled: Flow<Boolean> = dataStore.data
        .map { preferences ->
            preferences[KEY_USER_DICTIONARIES_ENABLED] ?: DEFAULT_USER_DICTIONARIES_ENABLED
        }

    /**
     * Set whether user dictionaries are enabled.
     * @param enabled True to apply user dictionaries during synthesis
     */
    suspend fun setUserDictionariesEnabled(enabled: Boolean) {
        dataStore.edit { preferences ->
            preferences[KEY_USER_DICTIONARIES_ENABLED] = enabled
        }
    }

    // ==========================================================================
    // Restore Default Methods
    // ==========================================================================

    /**
     * Restore speech rate to default value (1.0)
     */
    suspend fun restoreDefaultSpeed() {
        dataStore.edit { preferences ->
            preferences[KEY_SPEED] = DEFAULT_SPEED
        }
    }

    /**
     * Restore pitch to default value (1.0)
     */
    suspend fun restoreDefaultPitch() {
        dataStore.edit { preferences ->
            preferences[KEY_PITCH] = DEFAULT_PITCH
        }
    }

    /**
     * Restore volume to default value (1.0)
     */
    suspend fun restoreDefaultVolume() {
        dataStore.edit { preferences ->
            preferences[KEY_VOLUME] = DEFAULT_VOLUME
        }
    }

    // ==========================================================================
    // Combined Settings
    // ==========================================================================

    /**
     * Data class for all TTS settings
     */
    data class TTSSettings(
        val defaultVoice: String = DEFAULT_VOICE,
        val speed: Float = DEFAULT_SPEED,
        val pitch: Float = DEFAULT_PITCH,
        val volume: Float = DEFAULT_VOLUME,
        val forceSpeed: Boolean = DEFAULT_FORCE_SPEED,
        val forcePitch: Boolean = DEFAULT_FORCE_PITCH,
        val forceVolume: Boolean = DEFAULT_FORCE_VOLUME,
        val forceLanguage: Boolean = DEFAULT_FORCE_LANGUAGE,
        // Advanced settings
        val emojiEnabled: Boolean = DEFAULT_EMOJI_ENABLED,
        val inflectionEnabled: Boolean = DEFAULT_INFLECTION_ENABLED,
        val sentencePause: Int = DEFAULT_SENTENCE_PAUSE,
        val commaPause: Int = DEFAULT_COMMA_PAUSE,
        val newlinePause: Int = DEFAULT_NEWLINE_PAUSE,
        val numberMode: Int = DEFAULT_NUMBER_MODE,
        // Dictionary settings
        val userDictionariesEnabled: Boolean = DEFAULT_USER_DICTIONARIES_ENABLED
    )

    /**
     * Flow of all settings combined
     */
    val allSettings: Flow<TTSSettings> = dataStore.data
        .map { preferences ->
            TTSSettings(
                defaultVoice = preferences[KEY_DEFAULT_VOICE] ?: DEFAULT_VOICE,
                speed = preferences[KEY_SPEED] ?: DEFAULT_SPEED,
                pitch = preferences[KEY_PITCH] ?: DEFAULT_PITCH,
                volume = preferences[KEY_VOLUME] ?: DEFAULT_VOLUME,
                forceSpeed = preferences[KEY_FORCE_SPEED] ?: DEFAULT_FORCE_SPEED,
                forcePitch = preferences[KEY_FORCE_PITCH] ?: DEFAULT_FORCE_PITCH,
                forceVolume = preferences[KEY_FORCE_VOLUME] ?: DEFAULT_FORCE_VOLUME,
                forceLanguage = preferences[KEY_FORCE_LANGUAGE] ?: DEFAULT_FORCE_LANGUAGE,
                emojiEnabled = preferences[KEY_EMOJI_ENABLED] ?: DEFAULT_EMOJI_ENABLED,
                inflectionEnabled = preferences[KEY_INFLECTION_ENABLED] ?: DEFAULT_INFLECTION_ENABLED,
                sentencePause = preferences[KEY_SENTENCE_PAUSE] ?: DEFAULT_SENTENCE_PAUSE,
                commaPause = preferences[KEY_COMMA_PAUSE] ?: DEFAULT_COMMA_PAUSE,
                newlinePause = preferences[KEY_NEWLINE_PAUSE] ?: DEFAULT_NEWLINE_PAUSE,
                numberMode = preferences[KEY_NUMBER_MODE] ?: DEFAULT_NUMBER_MODE,
                userDictionariesEnabled = preferences[KEY_USER_DICTIONARIES_ENABLED] ?: DEFAULT_USER_DICTIONARIES_ENABLED
            )
        }
}
