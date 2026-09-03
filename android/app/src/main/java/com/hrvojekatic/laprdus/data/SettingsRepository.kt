package com.hrvojekatic.laprdus.data

import android.content.Context
import androidx.annotation.VisibleForTesting
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.booleanPreferencesKey
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.floatPreferencesKey
import androidx.datastore.preferences.core.intPreferencesKey
import androidx.datastore.preferences.core.stringPreferencesKey
import com.hrvojekatic.laprdus.data.migration.MigrationResult
import com.hrvojekatic.laprdus.data.migration.LegacyMigrator
import com.hrvojekatic.laprdus.data.migration.SettingsMigrator
import com.hrvojekatic.laprdus.data.migration.SimulatedMigrationCrashException
import com.hrvojekatic.laprdus.data.storage.LaprdusStorage
import com.hrvojekatic.laprdus.data.storage.StorageLogger
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.emitAll
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.withContext

/**
 * Repository for persisting TTS settings using Jetpack DataStore.
 * Stores user preferences for voice, speed, pitch, volume, and force settings.
 *
 * The production store lives in device-protected storage (see
 * [LaprdusStorage]) so the TTS service can read it on the lock screen before
 * the first unlock. Settings written by older versions into
 * credential-encrypted storage are migrated by [SettingsMigrator] on first
 * access after the user unlocks; migration problems are contained here and
 * reported through [storageError], never thrown at callers.
 *
 * For testing, use the constructor that accepts a DataStore directly.
 */
class SettingsRepository internal constructor(
    private val dataStore: DataStore<Preferences>,
    private val migrator: LegacyMigrator? = null,
    private val logger: StorageLogger = StorageLogger.None,
) {

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

        @Volatile
        private var instance: SettingsRepository? = null

        /**
         * The process-wide repository over the device-protected settings store.
         * Shared by the UI (via Hilt) and the TTS service, so there is exactly
         * one DataStore instance and one migrator per process.
         */
        fun getInstance(context: Context): SettingsRepository {
            instance?.let { return it }
            synchronized(this) {
                instance?.let { return it }
                return SettingsRepository(
                    dataStore = LaprdusStorage.settingsDataStore(context),
                    migrator = LaprdusStorage.settingsMigrator(context),
                    logger = LaprdusStorage.logger("SettingsRepository")
                ).also { instance = it }
            }
        }

        @VisibleForTesting
        fun resetInstanceForTesting() {
            synchronized(this) { instance = null }
        }
    }

    // ==========================================================================
    // Migration and error reporting
    // ==========================================================================

    private val _storageError = MutableStateFlow<String?>(null)

    /** Human-readable description of the last storage/migration failure, or null. */
    val storageError: StateFlow<String?> = _storageError.asStateFlow()

    /** Result of the most recent migration attempt (diagnostics and tests). */
    @Volatile
    var lastMigrationResult: MigrationResult? = null
        private set

    /**
     * Runs the legacy-storage migration if it is still pending, on the IO
     * dispatcher. Never throws (except for simulated crashes in debug/test
     * builds): failures are logged, published to [storageError], reported as
     * [MigrationResult.RetryLater], and retried on a later call.
     */
    internal suspend fun ensureMigrated(): MigrationResult? {
        val migrator = migrator ?: return null
        return withContext(Dispatchers.IO) {
            try {
                val result = migrator.migrateIfNeeded()
                when (result) {
                    is MigrationResult.Migrated -> {
                        if (result.itemCount > 0) {
                            logger.info("Migrated ${result.itemCount} settings key(s) to device-protected storage")
                        }
                        _storageError.value = null
                    }
                    is MigrationResult.RetryLater -> {
                        logger.warn("Settings migration postponed: ${result.cause.message}")
                    }
                    MigrationResult.QuarantinedCorrupt -> {
                        logger.warn("Legacy settings were corrupt and have been set aside; defaults are in use")
                        _storageError.value = null
                    }
                    MigrationResult.NotNeeded, MigrationResult.SkippedLocked -> Unit
                }
                lastMigrationResult = result
                result
            } catch (e: SimulatedMigrationCrashException) {
                throw e
            } catch (e: CancellationException) {
                throw e
            } catch (e: Exception) {
                logger.error("Settings migration failed; continuing with current settings", e)
                _storageError.value = e.message ?: e.javaClass.simpleName
                MigrationResult.RetryLater(e).also { lastMigrationResult = it }
            }
        }
    }

    /** Re-enables the migration after the user unlocked the device. */
    fun onUserUnlocked() {
        migrator?.rearm()
    }

    /**
     * Current settings without waiting for a pending migration. Used for the
     * service's bounded startup read; the regular flows deliver migrated values
     * as soon as the migration completes.
     */
    suspend fun readSettingsNow(): TTSSettings = dataStore.data.first().toSettings()

    /** All reads go through the migration gate so callers never see pre-migration defaults. */
    private val prefs: Flow<Preferences> = flow {
        ensureMigrated()
        emitAll(dataStore.data)
    }

    // ==========================================================================
    // Voice Settings
    // ==========================================================================

    /**
     * Flow of the default voice ID
     */
    val defaultVoice: Flow<String> = prefs
        .map { preferences ->
            preferences[KEY_DEFAULT_VOICE] ?: DEFAULT_VOICE
        }

    /**
     * Set the default voice
     * @param voiceId Voice ID: "josip", "vlado", "detence", "baba", or "djed"
     */
    suspend fun setDefaultVoice(voiceId: String) {
        ensureMigrated()
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
    val speed: Flow<Float> = prefs
        .map { preferences ->
            preferences[KEY_SPEED] ?: DEFAULT_SPEED
        }

    /**
     * Set the speech speed
     * @param speed Speed factor (0.5 - 2.0)
     */
    suspend fun setSpeed(speed: Float) {
        ensureMigrated()
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
    val pitch: Flow<Float> = prefs
        .map { preferences ->
            preferences[KEY_PITCH] ?: DEFAULT_PITCH
        }

    /**
     * Set the pitch
     * @param pitch Pitch factor (0.5 - 2.0)
     */
    suspend fun setPitch(pitch: Float) {
        ensureMigrated()
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
    val volume: Flow<Float> = prefs
        .map { preferences ->
            preferences[KEY_VOLUME] ?: DEFAULT_VOLUME
        }

    /**
     * Set the volume
     * @param volume Volume level (0.0 - 1.0)
     */
    suspend fun setVolume(volume: Float) {
        ensureMigrated()
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
    val forceSpeed: Flow<Boolean> = prefs
        .map { preferences ->
            preferences[KEY_FORCE_SPEED] ?: DEFAULT_FORCE_SPEED
        }

    /**
     * Set whether to force Laprdus speed settings over application settings.
     * @param enabled True to force Laprdus speed settings
     */
    suspend fun setForceSpeed(enabled: Boolean) {
        ensureMigrated()
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
    val forcePitch: Flow<Boolean> = prefs
        .map { preferences ->
            preferences[KEY_FORCE_PITCH] ?: DEFAULT_FORCE_PITCH
        }

    /**
     * Set whether to force Laprdus pitch settings over application settings.
     * @param enabled True to force Laprdus pitch settings
     */
    suspend fun setForcePitch(enabled: Boolean) {
        ensureMigrated()
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
    val forceVolume: Flow<Boolean> = prefs
        .map { preferences ->
            preferences[KEY_FORCE_VOLUME] ?: DEFAULT_FORCE_VOLUME
        }

    /**
     * Set whether to force Laprdus volume settings over multimedia volume.
     * @param enabled True to force Laprdus volume settings
     */
    suspend fun setForceVolume(enabled: Boolean) {
        ensureMigrated()
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
    val forceLanguage: Flow<Boolean> = prefs
        .map { preferences ->
            preferences[KEY_FORCE_LANGUAGE] ?: DEFAULT_FORCE_LANGUAGE
        }

    /**
     * Set whether to force the selected language regardless of system settings.
     * @param enabled True to force the selected language
     */
    suspend fun setForceLanguage(enabled: Boolean) {
        ensureMigrated()
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
    val emojiEnabled: Flow<Boolean> = prefs
        .map { preferences ->
            preferences[KEY_EMOJI_ENABLED] ?: DEFAULT_EMOJI_ENABLED
        }

    /**
     * Set whether emoji processing is enabled.
     * @param enabled True to enable emoji to text conversion
     */
    suspend fun setEmojiEnabled(enabled: Boolean) {
        ensureMigrated()
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
    val inflectionEnabled: Flow<Boolean> = prefs
        .map { preferences ->
            preferences[KEY_INFLECTION_ENABLED] ?: DEFAULT_INFLECTION_ENABLED
        }

    /**
     * Set whether voice inflection is enabled.
     * @param enabled True to enable pitch variation for questions, exclamations, and pauses
     */
    suspend fun setInflectionEnabled(enabled: Boolean) {
        ensureMigrated()
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
    val sentencePause: Flow<Int> = prefs
        .map { preferences ->
            preferences[KEY_SENTENCE_PAUSE] ?: DEFAULT_SENTENCE_PAUSE
        }

    /**
     * Set the sentence pause duration.
     * @param pauseMs Pause duration in milliseconds (0-2000)
     */
    suspend fun setSentencePause(pauseMs: Int) {
        ensureMigrated()
        dataStore.edit { preferences ->
            preferences[KEY_SENTENCE_PAUSE] = pauseMs.coerceIn(0, 2000)
        }
    }

    /**
     * Flow of the comma pause setting (milliseconds).
     */
    val commaPause: Flow<Int> = prefs
        .map { preferences ->
            preferences[KEY_COMMA_PAUSE] ?: DEFAULT_COMMA_PAUSE
        }

    /**
     * Set the comma pause duration.
     * @param pauseMs Pause duration in milliseconds (0-2000)
     */
    suspend fun setCommaPause(pauseMs: Int) {
        ensureMigrated()
        dataStore.edit { preferences ->
            preferences[KEY_COMMA_PAUSE] = pauseMs.coerceIn(0, 2000)
        }
    }

    /**
     * Flow of the newline pause setting (milliseconds).
     */
    val newlinePause: Flow<Int> = prefs
        .map { preferences ->
            preferences[KEY_NEWLINE_PAUSE] ?: DEFAULT_NEWLINE_PAUSE
        }

    /**
     * Set the newline pause duration.
     * @param pauseMs Pause duration in milliseconds (0-2000)
     */
    suspend fun setNewlinePause(pauseMs: Int) {
        ensureMigrated()
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
    val numberMode: Flow<Int> = prefs
        .map { preferences ->
            preferences[KEY_NUMBER_MODE] ?: DEFAULT_NUMBER_MODE
        }

    /**
     * Set the number processing mode.
     * @param mode 0 for whole numbers, 1 for digit by digit
     */
    suspend fun setNumberMode(mode: Int) {
        ensureMigrated()
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
    val dontAskDefaultTts: Flow<Boolean> = prefs
        .map { preferences ->
            preferences[KEY_DONT_ASK_DEFAULT_TTS] ?: DEFAULT_DONT_ASK_DEFAULT_TTS
        }

    /**
     * Set whether to suppress the default TTS dialog.
     * @param enabled True to never show the dialog again
     */
    suspend fun setDontAskDefaultTts(enabled: Boolean) {
        ensureMigrated()
        dataStore.edit { preferences ->
            preferences[KEY_DONT_ASK_DEFAULT_TTS] = enabled
        }
    }

    // ==========================================================================
    // User Dictionaries Settings
    // ==========================================================================

    /**
     * Flow of the user dictionaries enabled setting.
     * When enabled, the user pronunciation dictionary (user.json) is applied during synthesis.
     * Enabled by default.
     */
    val userDictionariesEnabled: Flow<Boolean> = prefs
        .map { preferences ->
            preferences[KEY_USER_DICTIONARIES_ENABLED] ?: DEFAULT_USER_DICTIONARIES_ENABLED
        }

    /**
     * Set whether user dictionaries are enabled.
     * @param enabled True to apply user dictionaries during synthesis
     */
    suspend fun setUserDictionariesEnabled(enabled: Boolean) {
        ensureMigrated()
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
        ensureMigrated()
        dataStore.edit { preferences ->
            preferences[KEY_SPEED] = DEFAULT_SPEED
        }
    }

    /**
     * Restore pitch to default value (1.0)
     */
    suspend fun restoreDefaultPitch() {
        ensureMigrated()
        dataStore.edit { preferences ->
            preferences[KEY_PITCH] = DEFAULT_PITCH
        }
    }

    /**
     * Restore volume to default value (1.0)
     */
    suspend fun restoreDefaultVolume() {
        ensureMigrated()
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
    val allSettings: Flow<TTSSettings> = prefs.map { preferences -> preferences.toSettings() }

    private fun Preferences.toSettings(): TTSSettings = TTSSettings(
        defaultVoice = this[KEY_DEFAULT_VOICE] ?: DEFAULT_VOICE,
        speed = this[KEY_SPEED] ?: DEFAULT_SPEED,
        pitch = this[KEY_PITCH] ?: DEFAULT_PITCH,
        volume = this[KEY_VOLUME] ?: DEFAULT_VOLUME,
        forceSpeed = this[KEY_FORCE_SPEED] ?: DEFAULT_FORCE_SPEED,
        forcePitch = this[KEY_FORCE_PITCH] ?: DEFAULT_FORCE_PITCH,
        forceVolume = this[KEY_FORCE_VOLUME] ?: DEFAULT_FORCE_VOLUME,
        forceLanguage = this[KEY_FORCE_LANGUAGE] ?: DEFAULT_FORCE_LANGUAGE,
        emojiEnabled = this[KEY_EMOJI_ENABLED] ?: DEFAULT_EMOJI_ENABLED,
        inflectionEnabled = this[KEY_INFLECTION_ENABLED] ?: DEFAULT_INFLECTION_ENABLED,
        sentencePause = this[KEY_SENTENCE_PAUSE] ?: DEFAULT_SENTENCE_PAUSE,
        commaPause = this[KEY_COMMA_PAUSE] ?: DEFAULT_COMMA_PAUSE,
        newlinePause = this[KEY_NEWLINE_PAUSE] ?: DEFAULT_NEWLINE_PAUSE,
        numberMode = this[KEY_NUMBER_MODE] ?: DEFAULT_NUMBER_MODE,
        userDictionariesEnabled = this[KEY_USER_DICTIONARIES_ENABLED] ?: DEFAULT_USER_DICTIONARIES_ENABLED
    )
}
