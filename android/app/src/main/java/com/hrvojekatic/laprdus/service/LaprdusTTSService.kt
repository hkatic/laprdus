package com.hrvojekatic.laprdus.service

import android.app.Service
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.media.AudioFormat
import android.speech.tts.SynthesisCallback
import android.speech.tts.SynthesisRequest
import android.speech.tts.TextToSpeech
import android.speech.tts.TextToSpeechService
import android.speech.tts.Voice
import android.util.Log
import androidx.core.content.ContextCompat
import com.hrvojekatic.laprdus.BuildConfig
import com.hrvojekatic.laprdus.data.DictionaryRepository
import com.hrvojekatic.laprdus.data.SettingsRepository
import com.hrvojekatic.laprdus.data.migration.DictionaryMigrator
import com.hrvojekatic.laprdus.data.migration.MigrationResult
import com.hrvojekatic.laprdus.data.migration.SimulatedMigrationCrashException
import com.hrvojekatic.laprdus.data.storage.LaprdusStorage
import com.hrvojekatic.laprdus.tts.LaprdusTTS
import com.hrvojekatic.laprdus.tts.VoiceInfo
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.async
import kotlinx.coroutines.cancel
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.drop
import kotlinx.coroutines.flow.retryWhen
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withTimeoutOrNull
import java.io.File
import java.text.BreakIterator
import java.util.Locale

/**
 * Android TextToSpeechService implementation for system-wide TTS.
 * Supports Croatian (hr-HR) and Serbian (sr-RS) languages.
 *
 * This service allows other apps to use Laprdus as their TTS engine.
 *
 * Direct Boot: the service is declared `directBootAware`, so screen readers
 * can use it on the lock screen after a restart, before the user's first
 * unlock. All state it needs (settings DataStore, user dictionaries) lives in
 * device-protected storage via [LaprdusStorage]; voice data and bundled
 * dictionaries come from APK assets. Data written by older versions into
 * credential-encrypted storage is migrated once the user unlocks.
 *
 * Failure policy: storage and migration problems are never fatal, the engine
 * keeps speaking with defaults. Only "no voice can be loaded at all" is
 * surfaced as [LaprdusEngineUnavailableException] from the synthesis path so
 * the system can fall back to another engine (see [EngineRuntime]).
 */
class LaprdusTTSService : TextToSpeechService() {

    companion object {
        private const val TAG = "LaprdusTTSService"
        private const val FALLBACK_VOICE = EngineRuntime.DEFAULT_FALLBACK_VOICE
        private const val STARTUP_READ_TIMEOUT_MS = 2_000L
        private const val SETTINGS_RETRY_DELAY_MS = 5_000L
        private const val UNLOCK_RETRY_DELAY_MS = 30_000L
        private const val MAX_UNLOCK_RETRIES = 5
    }

    /**
     * Debug-only logging. Utterance text passes through here — including
     * everything a screen reader speaks on the lock screen — and R8 keeps
     * android.util.Log calls in release builds, so these are compiled out
     * instead. The message is a lambda so it is not even built in release.
     */
    private inline fun logDebug(message: () -> String) {
        if (BuildConfig.DEBUG) Log.d(TAG, message())
    }

    @Volatile
    private var tts: LaprdusTTS? = null
    @Volatile
    private var currentVoiceId: String = FALLBACK_VOICE

    // Assigned in onCreate: a Service has no base context in its constructor,
    // so these must not be property initializers.
    private lateinit var settingsRepo: SettingsRepository
    private lateinit var dictionaryMigrator: DictionaryMigrator
    private lateinit var dictionaryDir: File
    private lateinit var engineRuntime: EngineRuntime

    // Cached settings (avoids blocking on every synthesis)
    @Volatile
    private var cachedSettings: SettingsRepository.TTSSettings? = null
    @Volatile
    private var unlockHandled = false
    private var unlockReceiver: BroadcastReceiver? = null
    private val settingsScope = CoroutineScope(Dispatchers.IO + SupervisorJob())

    override fun onCreate() {
        // Order matters: super.onCreate() synchronously calls onLoadLanguage(),
        // which loads the voice AND the user dictionary, so everything that
        // load depends on (settings, storage objects, pending migrations) must
        // be in place first. System.loadLibrary() runs in the LaprdusTTS
        // companion object init (class loading) and needs no Context.
        // The service has an engine of its own; the app's screens use another.
        val engine = LaprdusTTS.service
        tts = engine

        val app = applicationContext
        settingsRepo = SettingsRepository.getInstance(app)
        dictionaryMigrator = LaprdusStorage.dictionaryMigrator(app)
        dictionaryDir = LaprdusStorage.dictionaryDir(app)
        engineRuntime = EngineRuntime(
            engine = ServiceSpeechEngine(),
            crashMarkerFile = LaprdusStorage.engineCrashMarkerFile(app),
            logger = LaprdusStorage.logger(TAG)
        )

        // Register first, then check: a broadcast between the two cannot be missed.
        registerUnlockReceiver()
        val startupSettings = readSettingsBlocking()
        cachedSettings = startupSettings
        currentVoiceId = startupSettings.defaultVoice
        // Every voice the engine loads gets the user dictionaries from here.
        engine.userDictionaryDir = dictionaryDir
        applyEngineSettings(startupSettings)

        val unlocked = LaprdusStorage.isUserUnlocked(app)
        Log.i(TAG, "Service created (userUnlocked=$unlocked)")

        // Keep settings current; the collector also re-applies values once a
        // pending migration completes. A failing store is retried, never abandoned.
        settingsScope.launch {
            settingsRepo.allSettings
                .retryWhen { e, _ ->
                    if (e is SimulatedMigrationCrashException) {
                        false
                    } else {
                        Log.e(TAG, "Settings flow failed; retrying in $SETTINGS_RETRY_DELAY_MS ms", e)
                        delay(SETTINGS_RETRY_DELAY_MS)
                        true
                    }
                }
                .collect { settings ->
                    val previous = cachedSettings
                    cachedSettings = settings
                    applyEngineSettings(settings)
                    followSettingsChanges(previous, settings)
                }
        }

        // A dictionary edited in the app is heard on the next utterance.
        settingsScope.launch {
            DictionaryRepository.changes.drop(1).collect {
                logDebug { "User dictionaries changed; reloading them" }
                tts?.reloadDictionaries(assets)
            }
        }

        super.onCreate()
        logDebug { "Service created" }
        initializeEngine()

        // The post-unlock migration (and the reload it may trigger) runs only
        // after the initial engine configuration above, so the two cannot interleave.
        if (unlocked) {
            onUserUnlocked()
        }
    }

    /**
     * Handle service restart after process kill.
     * Returns START_STICKY to ensure service is restarted if killed.
     */
    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        logDebug { "onStartCommand called, intent: $intent, flags: $flags" }

        // Ensure engine is initialized (handles process restart case)
        val engine = tts
        if (engine == null || !engine.isInitialized()) {
            logDebug { "Engine not initialized, reinitializing..." }
            initializeEngine()
        }

        // Call super to let TextToSpeechService handle standard behavior
        val result = super.onStartCommand(intent, flags, startId)

        // Return START_STICKY so service is restarted if killed
        return Service.START_STICKY
    }

    /**
     * Bounded read of the device-protected settings store for startup.
     * Never throws and never waits longer than [STARTUP_READ_TIMEOUT_MS]: the
     * read runs on [settingsScope] and is abandoned (not cancelled) on timeout;
     * the settings collector corrects the cached value later if needed.
     */
    private fun readSettingsBlocking(): SettingsRepository.TTSSettings {
        return try {
            val pending = settingsScope.async { settingsRepo.readSettingsNow() }
            runBlocking { withTimeoutOrNull(STARTUP_READ_TIMEOUT_MS) { pending.await() } }
                ?: SettingsRepository.TTSSettings().also {
                    Log.w(TAG, "Settings read timed out; using defaults until the store responds")
                }
        } catch (e: Exception) {
            Log.e(TAG, "Could not read settings; using defaults", e)
            SettingsRepository.TTSSettings()
        }
    }

    /**
     * Initialize the TTS engine with the saved voice and user dictionaries.
     * Called from onCreate and onStartCommand to handle process restart.
     * Never throws: storage problems degrade to defaults.
     */
    private fun initializeEngine() {
        if (tts == null) {
            tts = LaprdusTTS.service
        }

        val settings = cachedSettings ?: readSettingsBlocking().also { cachedSettings = it }
        currentVoiceId = settings.defaultVoice

        // Initialize with saved voice using setVoice
        // This ensures proper loading, pitch settings, and user dictionaries
        try {
            val success = setVoiceAndLoadUserDictionaries(currentVoiceId)
            if (success) {
                logDebug { "Engine initialized with $currentVoiceId voice" }
            } else {
                Log.e(TAG, "Failed to initialize engine with $currentVoiceId voice")
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to initialize engine", e)
        }
    }

    /** Applies the advanced settings to the native engine. */
    private fun applyEngineSettings(settings: SettingsRepository.TTSSettings) {
        val engine = tts ?: return
        try {
            engine.applySettings(settings)
        } catch (e: Exception) {
            Log.e(TAG, "Failed to apply engine settings", e)
        }
    }

    /**
     * The app has an engine of its own, so what the user changes there
     * reaches the service through the saved settings: a newly chosen voice
     * is loaded at once (TalkBack speaks with it from the next utterance, as
     * the user expects after picking it), and switching the user
     * dictionaries on or off reloads the dictionaries.
     */
    private fun followSettingsChanges(
        previous: SettingsRepository.TTSSettings?,
        settings: SettingsRepository.TTSSettings
    ) {
        if (previous == null) return
        try {
            if (settings.defaultVoice != previous.defaultVoice) {
                logDebug { "Saved voice changed to ${settings.defaultVoice}; loading it" }
                if (setVoiceAndLoadUserDictionaries(settings.defaultVoice)) {
                    currentVoiceId = settings.defaultVoice
                } else {
                    Log.e(TAG, "Failed to load the newly chosen voice ${settings.defaultVoice}")
                }
            } else if (settings.userDictionariesEnabled != previous.userDictionariesEnabled) {
                tts?.reloadDictionaries(assets)
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to follow a settings change", e)
        }
    }

    // ==========================================================================
    // Direct Boot: unlock handling and legacy-storage migration
    // ==========================================================================

    private fun registerUnlockReceiver() {
        val receiver = object : BroadcastReceiver() {
            override fun onReceive(context: Context?, intent: Intent?) {
                if (intent?.action == Intent.ACTION_USER_UNLOCKED) {
                    Log.i(TAG, "User unlocked; credential-encrypted storage is now available")
                    onUserUnlocked()
                }
            }
        }
        unlockReceiver = receiver
        ContextCompat.registerReceiver(
            this,
            receiver,
            IntentFilter(Intent.ACTION_USER_UNLOCKED),
            ContextCompat.RECEIVER_NOT_EXPORTED
        )
    }

    private fun unregisterUnlockReceiver() {
        val receiver = unlockReceiver ?: return
        unlockReceiver = null
        try {
            unregisterReceiver(receiver)
        } catch (_: IllegalArgumentException) {
            // Already unregistered
        }
    }

    /**
     * Runs once per process after the user is unlocked (immediately when the
     * service starts unlocked, otherwise on ACTION_USER_UNLOCKED).
     */
    private fun onUserUnlocked() {
        if (unlockHandled) return
        unlockHandled = true
        unregisterUnlockReceiver()
        settingsScope.launch { runUnlockPath() }
    }

    /**
     * Migrates legacy credential-encrypted data (settings and dictionaries)
     * into device-protected storage and reloads the user dictionaries when
     * they changed. Idempotent; retried a few times when storage is not ready.
     */
    private suspend fun runUnlockPath(attempt: Int = 0) {
        settingsRepo.onUserUnlocked()
        val settingsResult = settingsRepo.ensureMigrated()
        val dictionaryResult = migrateDictionaries()
        Log.i(TAG, "Post-unlock migration: settings=$settingsResult, dictionaries=$dictionaryResult")

        // Apply migrated settings right away, including the saved default voice,
        // instead of waiting for the collector.
        var reloadVoiceId = currentVoiceId
        if (settingsResult is MigrationResult.Migrated && settingsResult.itemCount > 0) {
            val migrated = try {
                settingsRepo.readSettingsNow()
            } catch (e: CancellationException) {
                throw e
            } catch (e: Exception) {
                Log.e(TAG, "Could not re-read settings after migration", e)
                null
            }
            if (migrated != null) {
                cachedSettings = migrated
                applyEngineSettings(migrated)
                reloadVoiceId = migrated.defaultVoice
            }
        }

        val dictionariesChanged =
            dictionaryResult is MigrationResult.Migrated && dictionaryResult.itemCount > 0
        val voiceChanged = reloadVoiceId != currentVoiceId
        if (dictionariesChanged || voiceChanged) {
            if (setVoiceAndLoadUserDictionaries(reloadVoiceId)) {
                currentVoiceId = reloadVoiceId
            } else {
                Log.e(TAG, "Failed to reload voice $reloadVoiceId after unlock")
            }
        }

        val retry = settingsResult is MigrationResult.RetryLater ||
            dictionaryResult is MigrationResult.RetryLater
        if (retry && attempt < MAX_UNLOCK_RETRIES) {
            delay(UNLOCK_RETRY_DELAY_MS)
            runUnlockPath(attempt + 1)
        }
    }

    private suspend fun migrateDictionaries(): MigrationResult {
        return try {
            // No rearm(): a migration already completed in this process (e.g. by
            // the dictionary screen) does not need to run again.
            dictionaryMigrator.migrateIfNeeded()
        } catch (e: SimulatedMigrationCrashException) {
            throw e
        } catch (e: CancellationException) {
            throw e
        } catch (e: Exception) {
            Log.e(TAG, "Dictionary migration failed; user dictionaries stay unavailable until retried", e)
            MigrationResult.RetryLater(e)
        }
    }

    // ==========================================================================
    // Voice and dictionary loading
    // ==========================================================================

    /**
     * Set voice and reload user dictionaries.
     * The engine loads the bundled dictionaries and then the user's entries
     * (from [LaprdusTTS.userDictionaryDir], set in onCreate) as one step.
     */
    private fun setVoiceAndLoadUserDictionaries(voiceId: String): Boolean {
        val engine = tts ?: return false
        return engine.setVoice(voiceId, assets)
    }

    /** Adapter that lets [EngineRuntime] drive the native engine. */
    private inner class ServiceSpeechEngine : SpeechEngine {
        override fun setVoice(voiceId: String): Boolean {
            if (tts == null) {
                tts = LaprdusTTS.service
            }
            return try {
                setVoiceAndLoadUserDictionaries(voiceId)
            } catch (e: Exception) {
                Log.e(TAG, "Exception loading voice $voiceId", e)
                false
            }
        }
    }

    override fun onDestroy() {
        logDebug { "Service destroyed" }
        unregisterUnlockReceiver()
        settingsScope.cancel()
        // The service's engine stays loaded for the next start of the service
        // in this process; the app's screens have an engine of their own.
        tts = null
        super.onDestroy()
    }

    /**
     * Called when the user removes the app from recent tasks.
     * We do NOT stop the service - it should continue running for TTS.
     * The android:stopWithTask="false" in manifest also helps with this.
     */
    override fun onTaskRemoved(rootIntent: Intent?) {
        logDebug { "Task removed, service continues running" }
        // Do NOT call super.onTaskRemoved() or stopSelf()
        // The TTS service should continue running independently of the app task

        // Ensure engine is still initialized
        if (tts == null || tts?.isInitialized() != true) {
            logDebug { "Reinitializing engine after task removal" }
            initializeEngine()
        }
    }

    /**
     * Check if a language is supported.
     * Supports Croatian (hr), Serbian (sr) and Bosnian (bs).
     *
     * The Android framework passes ISO3 codes on this boundary ("hrv"/"HRV",
     * "srp"/"SRB", "bos"/"BIH"), while apps may pass ISO2 ("hr"/"HR"), so both
     * are accepted.
     *
     * For any other language this deliberately still reports LANG_AVAILABLE
     * instead of LANG_NOT_SUPPORTED: TTS settings and screen readers probe
     * with the device locale, and on a negative answer some of them (e.g.
     * Honor MagicOS settings) disable the engine entirely even though the
     * user explicitly selected it. eSpeak NG and RhVoice apply the same
     * "never silent" fallback - synthesis proceeds with the default voice.
     */
    override fun onIsLanguageAvailable(lang: String, country: String?, variant: String?): Int {
        val normalizedLang = lang.lowercase()
        val normalizedCountry = country?.lowercase() ?: ""

        return when (normalizedLang) {
            "hr", "hrv" -> {
                if (normalizedCountry == "hr" || normalizedCountry == "hrv") {
                    TextToSpeech.LANG_COUNTRY_AVAILABLE
                } else {
                    TextToSpeech.LANG_AVAILABLE
                }
            }
            "sr", "srp" -> {
                if (normalizedCountry == "rs" || normalizedCountry == "srb") {
                    TextToSpeech.LANG_COUNTRY_AVAILABLE
                } else {
                    TextToSpeech.LANG_AVAILABLE
                }
            }
            "bs", "bos" -> {
                if (normalizedCountry == "ba" || normalizedCountry == "bih") {
                    TextToSpeech.LANG_COUNTRY_AVAILABLE
                } else {
                    TextToSpeech.LANG_AVAILABLE
                }
            }
            else -> TextToSpeech.LANG_AVAILABLE
        }
    }

    /**
     * Get the current language configuration.
     * The framework expects ISO3 language and country codes here.
     */
    override fun onGetLanguage(): Array<String> {
        return when (languageOfVoice(currentVoiceId)) {
            "sr" -> arrayOf("srp", "SRB", "")
            "bs" -> arrayOf("bos", "BIH", "")
            else -> arrayOf("hrv", "HRV", "")
        }
    }

    /** Two-letter language of a voice. */
    private fun languageOfVoice(voiceId: String): String = when (voiceId) {
        "vlado", "djed", "stojan", "trubac", "harmonikas", "pevac" -> "sr"
        "mirsad", "sevdalija", "sazlija", "solist" -> "bs"
        else -> "hr"
    }

    /** Two-letter code of a language Laprdus speaks, null for any other. */
    private fun spokenLanguage(lang: String): String? = when (lang.lowercase()) {
        "hr", "hrv" -> "hr"
        "sr", "srp" -> "sr"
        "bs", "bos" -> "bs"
        else -> null
    }

    /** Default voice of a language: its formant voice. */
    private fun defaultVoiceFor(language: String?): String = when (language) {
        "sr" -> "stojan"
        "bs" -> "mirsad"
        else -> "zvonko"
    }

    /**
     * Load the specified language.
     * Unknown languages fall back to the current voice instead of failing,
     * matching the availability contract of onIsLanguageAvailable.
     */
    override fun onLoadLanguage(lang: String, country: String?, variant: String?): Int {
        val available = onIsLanguageAvailable(lang, country, variant)

        // A voice that already speaks the requested language is kept, so the
        // voice the user chose is not replaced by the language's default.
        val language = spokenLanguage(lang)
        val voiceId = if (language == null || language == languageOfVoice(currentVoiceId)) {
            currentVoiceId
        } else {
            defaultVoiceFor(language)
        }

        return try {
            val success = setVoiceAndLoadUserDictionaries(voiceId)
            if (success) {
                currentVoiceId = voiceId
                logDebug { "Loaded language: $lang with voice: $voiceId" }
                available
            } else {
                Log.e(TAG, "Failed to load voice $voiceId for language $lang")
                TextToSpeech.LANG_NOT_SUPPORTED
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to load language", e)
            TextToSpeech.LANG_NOT_SUPPORTED
        }
    }

    /**
     * Get all available voices.
     */
    override fun onGetVoices(): List<Voice> {
        val voices = mutableListOf<Voice>()
        val engine = tts ?: return voices

        val allVoices = engine.getAllVoices()
        for (info in allVoices) {
            val locale = when (info.languageCode) {
                "hr-HR" -> Locale.forLanguageTag("hr-HR")
                "sr-RS" -> Locale.forLanguageTag("sr-RS")
                "bs-BA" -> Locale.forLanguageTag("bs-BA")
                else -> Locale.forLanguageTag("hr-HR")
            }

            // Determine quality based on voice type
            val quality = if (info.isPhysicalVoice) {
                Voice.QUALITY_HIGH
            } else {
                Voice.QUALITY_NORMAL
            }

            voices.add(
                Voice(
                    info.id,
                    locale,
                    quality,
                    Voice.LATENCY_NORMAL,
                    false,
                    emptySet()
                )
            )
        }

        logDebug { "Returning ${voices.size} voices" }
        return voices
    }

    /**
     * Check if a voice name is valid.
     */
    override fun onIsValidVoiceName(voiceName: String?): Int {
        if (voiceName == null) return TextToSpeech.ERROR

        val engine = tts ?: return TextToSpeech.ERROR
        val allVoices = engine.getAllVoices()

        return if (allVoices.any { it.id == voiceName }) {
            TextToSpeech.SUCCESS
        } else {
            TextToSpeech.ERROR
        }
    }

    /**
     * Load a specific voice.
     */
    override fun onLoadVoice(voiceName: String?): Int {
        if (voiceName == null) return TextToSpeech.ERROR

        val engine = tts ?: return TextToSpeech.ERROR

        return try {
            val success = setVoiceAndLoadUserDictionaries(voiceName)
            if (success) {
                currentVoiceId = voiceName
                logDebug { "Loaded voice: $voiceName" }
                TextToSpeech.SUCCESS
            } else {
                Log.e(TAG, "Failed to load voice: $voiceName")
                TextToSpeech.ERROR
            }
        } catch (e: Exception) {
            Log.e(TAG, "Exception loading voice", e)
            TextToSpeech.ERROR
        }
    }

    /**
     * Get the default voice name for a language.
     */
    override fun onGetDefaultVoiceNameFor(
        lang: String,
        country: String?,
        variant: String?
    ): String {
        return defaultVoiceFor(spokenLanguage(lang))
    }

    /**
     * Determines if the input text is a single grapheme (user-perceived character).
     * This is used to detect when TalkBack or keyboard input sends a single character
     * that should be spelled out using the spelling dictionary.
     *
     * Uses Java's BreakIterator for proper Unicode grapheme cluster detection.
     * This handles:
     * - Simple ASCII characters (A-Z, 0-9)
     * - Croatian characters with diacritics (Č, Ć, Đ, Š, Ž)
     * - Emoji (including compound emoji like 👨‍👩‍👧)
     * - Combining characters (e.g., e + combining acute = é)
     *
     * @param text The text to check
     * @return true if the text contains exactly one grapheme cluster
     */
    private fun isSingleGrapheme(text: String): Boolean {
        if (text.isEmpty()) return false

        val iterator = BreakIterator.getCharacterInstance()
        iterator.setText(text)

        // Move to first boundary (should be at start)
        iterator.first()
        // Move to next boundary
        val end = iterator.next()

        // If we're at the end of text after one grapheme, it's a single grapheme
        return end == text.length
    }

    /**
     * Synthesize text to speech.
     * Respects force settings from SettingsRepository when enabled.
     *
     * When a single character is detected (common when TalkBack navigates character-by-character
     * or when typing on the keyboard), uses spelled synthesis mode which pronounces the character
     * by its name (e.g., "A" -> "A", "Č" -> "Če", "." -> "točka").
     */
    override fun onSynthesizeText(request: SynthesisRequest, callback: SynthesisCallback) {
        var engine = tts

        // Attempt recovery if the engine is not ready (handles process restart).
        // Deliberately NOT wrapped in try/catch: when no voice can be loaded at
        // all, EngineRuntime throws LaprdusEngineUnavailableException so the
        // process dies and the TTS framework reports the failure to the client.
        if (engine == null || !engine.isInitialized()) {
            Log.w(TAG, "Engine not initialized, attempting recovery...")
            when (val ready = engineRuntime.ensureReady(currentVoiceId)) {
                is EngineRuntime.ReadyResult.Ready -> {
                    currentVoiceId = ready.voiceId
                    engine = tts
                }
                EngineRuntime.ReadyResult.Unavailable -> {
                    callback.error()
                    return
                }
            }
        }

        if (engine == null || !engine.isInitialized()) {
            Log.e(TAG, "Engine not initialized after recovery attempt")
            callback.error()
            return
        }

        // Get text to synthesize
        val text = request.charSequenceText?.toString() ?: ""
        if (text.isEmpty()) {
            callback.done()
            return
        }

        logDebug { "Synthesizing: ${text.take(50)}..." }

        try {
            // Use cached settings (non-blocking) - falls back to defaults if not yet loaded
            val settings = cachedSettings ?: SettingsRepository.TTSSettings()

            // Nothing may change the voice, rate or pitch between setting them
            // and synthesizing (a voice chosen in the app or a dictionary
            // edited there is loaded from another thread).
            val useSpelledMode = isSingleGrapheme(text)
            val samples = engine.exclusive {
                // Apply force language - use saved voice regardless of request
                if (settings.forceLanguage) {
                    val savedVoice = settings.defaultVoice
                    if (savedVoice != currentVoiceId) {
                        logDebug { "Using forced language voice: $savedVoice" }
                        if (setVoiceAndLoadUserDictionaries(savedVoice)) {
                            currentVoiceId = savedVoice
                        } else {
                            Log.e(TAG, "Failed to switch to forced voice: $savedVoice")
                        }
                    }
                }

                // Rate and pitch: Android asks relative to normal (100 = 1.0),
                // and the Laprdus settings are the normal, so TalkBack's rate
                // scales the Laprdus rate; a forced setting ignores the app.
                // The acceleration multiplies the rate on top of that.
                engine.speed = VoiceInfo.requestValue(
                    settings.speed, request.speechRate / 100f, settings.forceSpeed, currentVoiceId
                )
                engine.pitch = VoiceInfo.requestValue(
                    settings.pitch, request.pitch / 100f, settings.forcePitch, currentVoiceId
                )
                logDebug { "Rate ${engine.speed}, pitch ${engine.pitch} (forced: ${settings.forceSpeed}, ${settings.forcePitch})" }

                // Apply volume - use Laprdus settings if force is enabled, reset to 1.0 if not
                engine.volume = if (settings.forceVolume) settings.volume else 1.0f

                // Synthesize - use spelled mode for single characters (TalkBack accessibility)
                if (useSpelledMode) {
                    logDebug { "Using spelled synthesis for single character: '$text'" }
                    engine.synthesizeSpelled(text)
                } else {
                    engine.synthesize(text)
                }
            }

            if (samples == null || samples.isEmpty()) {
                Log.e(TAG, "Synthesis returned no samples")
                callback.error()
                return
            }

            logDebug { "Synthesized ${samples.size} samples (spelled=$useSpelledMode)" }

            // Start audio output
            val result = callback.start(
                engine.sampleRate,
                AudioFormat.ENCODING_PCM_16BIT,
                1 // mono
            )

            if (result != TextToSpeech.SUCCESS) {
                Log.e(TAG, "Callback start failed: $result")
                callback.error()
                return
            }

            // Convert shorts to bytes (little-endian)
            val bytes = ByteArray(samples.size * 2)
            for (i in samples.indices) {
                val sample = samples[i].toInt()
                bytes[i * 2] = (sample and 0xFF).toByte()
                bytes[i * 2 + 1] = (sample shr 8 and 0xFF).toByte()
            }

            // Write audio in chunks
            val chunkSize = 4096
            var offset = 0
            while (offset < bytes.size) {
                val count = minOf(chunkSize, bytes.size - offset)
                val writeResult = callback.audioAvailable(bytes, offset, count)
                if (writeResult != TextToSpeech.SUCCESS) {
                    Log.w(TAG, "audioAvailable returned: $writeResult")
                    break
                }
                offset += count
            }

            callback.done()
            engineRuntime.onSynthesisSucceeded()
            logDebug { "Synthesis complete" }

        } catch (e: Exception) {
            Log.e(TAG, "Exception during synthesis", e)
            callback.error()
        }
    }

    /**
     * Stop any ongoing synthesis.
     */
    override fun onStop() {
        logDebug { "Stop requested" }
        tts?.cancel()
    }
}
