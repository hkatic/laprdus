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
import com.hrvojekatic.laprdus.data.DictionaryJson
import com.hrvojekatic.laprdus.data.DictionaryType
import com.hrvojekatic.laprdus.data.SettingsRepository
import com.hrvojekatic.laprdus.data.migration.DictionaryMigrator
import com.hrvojekatic.laprdus.data.migration.MigrationResult
import com.hrvojekatic.laprdus.data.migration.SimulatedMigrationCrashException
import com.hrvojekatic.laprdus.data.storage.LaprdusStorage
import com.hrvojekatic.laprdus.tts.LaprdusTTS
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.async
import kotlinx.coroutines.cancel
import kotlinx.coroutines.delay
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
    private var userDictionariesPendingReload = false
    @Volatile
    private var unlockHandled = false
    private var unlockReceiver: BroadcastReceiver? = null
    private val settingsScope = CoroutineScope(Dispatchers.IO + SupervisorJob())

    /** Serializes voice + dictionary (re)loading across the main, binder, IO and synthesis threads. */
    private val engineLock = Any()

    override fun onCreate() {
        // Order matters: super.onCreate() synchronously calls onLoadLanguage(),
        // which loads the voice AND the user dictionary, so everything that
        // load depends on (settings, storage objects, pending migrations) must
        // be in place first. System.loadLibrary() runs in the LaprdusTTS
        // companion object init (class loading) and needs no Context.
        tts = LaprdusTTS.getInstance()

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
        cachedSettings = readSettingsBlocking()
        currentVoiceId = cachedSettings?.defaultVoice ?: FALLBACK_VOICE

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
                    cachedSettings = settings
                    applyEngineSettings(settings)
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
            tts = LaprdusTTS.getInstance()
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
            engine.emojiEnabled = settings.emojiEnabled
            engine.inflectionEnabled = settings.inflectionEnabled
            engine.sentencePause = settings.sentencePause
            engine.commaPause = settings.commaPause
            engine.newlinePause = settings.newlinePause
            engine.numberMode = settings.numberMode
        } catch (e: Exception) {
            Log.e(TAG, "Failed to apply engine settings", e)
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
        if (dictionariesChanged || voiceChanged || userDictionariesPendingReload) {
            userDictionariesPendingReload = false
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
     * Use this instead of calling tts.setVoice() directly to ensure
     * user dictionary entries are always loaded after the bundled dictionary.
     */
    private fun setVoiceAndLoadUserDictionaries(voiceId: String): Boolean {
        val engine = tts ?: return false
        // setVoice replaces the native engine and reloads the bundled dictionaries
        // before the user entries are appended; that sequence must not interleave
        // with the same sequence on another thread.
        synchronized(engineLock) {
            val success = engine.setVoice(voiceId, assets)
            if (success) {
                loadUserDictionaries()
            }
            return success
        }
    }

    /**
     * Load user dictionary entries from the device-protected user.json into the
     * native engine. Entries are appended to the already-loaded bundled
     * dictionary using addPronunciation(), which does NOT clear existing entries.
     * Respects the userDictionariesEnabled setting; fails closed (defers the
     * load) while the settings are not known yet.
     */
    private fun loadUserDictionaries() {
        val settings = cachedSettings
        if (settings == null || !::dictionaryDir.isInitialized) {
            userDictionariesPendingReload = true
            logDebug { "Settings not loaded yet; deferring user dictionaries" }
            return
        }
        if (!settings.userDictionariesEnabled) {
            logDebug { "User dictionaries disabled, skipping" }
            return
        }

        val engine = tts ?: return

        val userDictFile = File(dictionaryDir, DictionaryType.MAIN.fileName)
        if (!userDictFile.isFile) {
            logDebug { "No user dictionary file found" }
            return
        }

        try {
            val entries = DictionaryJson.parse(userDictFile.readText(Charsets.UTF_8))
            var count = 0
            for (entry in entries) {
                if (entry.phoneme.isNotEmpty()) {
                    engine.addPronunciation(entry.grapheme, entry.phoneme, entry.caseSensitive, entry.wholeWord)
                    count++
                }
            }
            Log.i(TAG, "Loaded $count user dictionary entries")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to load user dictionary: ${e.message}")
        }
    }

    /** Adapter that lets [EngineRuntime] drive the native engine. */
    private inner class ServiceSpeechEngine : SpeechEngine {
        override fun setVoice(voiceId: String): Boolean {
            if (tts == null) {
                tts = LaprdusTTS.getInstance()
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
        // Do NOT call tts?.shutdown() — LaprdusTTS is a shared singleton.
        // shutdown() destroys the native engine (g_engine.reset()), which breaks
        // TTSViewModel and any other consumer sharing the same instance.
        // Same pattern as TTSViewModel.onCleared() (commit 827f0a1).
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
     * Supports Croatian (hr) and Serbian (sr).
     *
     * The Android framework passes ISO3 codes on this boundary ("hrv"/"HRV",
     * "srp"/"SRB"), while apps may pass ISO2 ("hr"/"HR"), so both are accepted.
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
            else -> TextToSpeech.LANG_AVAILABLE
        }
    }

    /**
     * Get the current language configuration.
     * The framework expects ISO3 language and country codes here.
     */
    override fun onGetLanguage(): Array<String> {
        return when {
            currentVoiceId in listOf("vlado", "djed") -> arrayOf("srp", "SRB", "")
            else -> arrayOf("hrv", "HRV", "")
        }
    }

    /**
     * Load the specified language.
     * Unknown languages fall back to the current voice instead of failing,
     * matching the availability contract of onIsLanguageAvailable.
     */
    override fun onLoadLanguage(lang: String, country: String?, variant: String?): Int {
        val available = onIsLanguageAvailable(lang, country, variant)

        // Select appropriate default voice
        val normalizedLang = lang.lowercase()
        val voiceId = when {
            normalizedLang == "hr" || normalizedLang == "hrv" -> "josip"
            normalizedLang == "sr" || normalizedLang == "srp" -> "vlado"
            else -> currentVoiceId
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
        val normalizedLang = lang.lowercase()
        return when {
            normalizedLang == "hr" || normalizedLang == "hrv" -> "josip"
            normalizedLang == "sr" || normalizedLang == "srp" -> "vlado"
            else -> "josip"
        }
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
            val settings = cachedSettings

            // Apply speech rate - use Laprdus settings if force is enabled
            val speechRate = if (settings?.forceSpeed == true) {
                logDebug { "Using forced Laprdus speed: ${settings.speed}" }
                settings.speed
            } else {
                // Android uses 100 as normal = 1.0
                (request.speechRate / 100f).coerceIn(0.5f, 2.0f)
            }
            engine.speed = speechRate

            // Apply pitch - use Laprdus settings if force is enabled
            val pitch = if (settings?.forcePitch == true) {
                logDebug { "Using forced Laprdus pitch: ${settings.pitch}" }
                settings.pitch
            } else {
                // Android uses 100 as normal = 1.0
                (request.pitch / 100f).coerceIn(0.5f, 2.0f)
            }
            engine.pitch = pitch

            // Apply volume - use Laprdus settings if force is enabled, reset to 1.0 if not
            if (settings?.forceVolume == true) {
                logDebug { "Using forced Laprdus volume: ${settings.volume}" }
                engine.volume = settings.volume
            } else {
                engine.volume = 1.0f
            }

            // Apply force language - use saved voice regardless of request
            if (settings?.forceLanguage == true) {
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

            // Synthesize - use spelled mode for single characters (TalkBack accessibility)
            val useSpelledMode = isSingleGrapheme(text)
            val samples = if (useSpelledMode) {
                logDebug { "Using spelled synthesis for single character: '$text'" }
                engine.synthesizeSpelled(text)
            } else {
                engine.synthesize(text)
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
