package com.hrvojekatic.laprdus.service

import android.app.Service
import android.content.Intent
import android.media.AudioFormat
import android.os.Build
import android.speech.tts.SynthesisCallback
import android.speech.tts.SynthesisRequest
import android.speech.tts.TextToSpeech
import android.speech.tts.TextToSpeechService
import android.speech.tts.Voice
import android.util.Log
import com.hrvojekatic.laprdus.data.SettingsRepository
import com.hrvojekatic.laprdus.tts.LaprdusTTS
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.runBlocking
import org.json.JSONObject
import java.io.File
import java.text.BreakIterator
import java.util.Locale

/**
 * Android TextToSpeechService implementation for system-wide TTS.
 * Supports Croatian (hr-HR) and Serbian (sr-RS) languages.
 *
 * This service allows other apps to use Laprdus as their TTS engine.
 */
class LaprdusTTSService : TextToSpeechService() {

    companion object {
        private const val TAG = "LaprdusTTSService"
    }

    private var tts: LaprdusTTS? = null
    private var currentVoiceId: String = "josip"

    // Settings repository and cached settings (avoids blocking on every synthesis)
    private lateinit var settingsRepo: SettingsRepository
    private var cachedSettings: SettingsRepository.TTSSettings? = null
    private val settingsScope = CoroutineScope(Dispatchers.IO + SupervisorJob())

    override fun onCreate() {
        super.onCreate()
        Log.d(TAG, "Service created")
        initializeEngine()
    }

    /**
     * Handle service restart after process kill.
     * Returns START_STICKY to ensure service is restarted if killed.
     */
    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        Log.d(TAG, "onStartCommand called, intent: $intent, flags: $flags")

        // Ensure engine is initialized (handles process restart case)
        if (tts == null || !tts!!.isInitialized()) {
            Log.d(TAG, "Engine not initialized, reinitializing...")
            initializeEngine()
        }

        // Call super to let TextToSpeechService handle standard behavior
        val result = super.onStartCommand(intent, flags, startId)

        // Return START_STICKY so service is restarted if killed
        return Service.START_STICKY
    }

    /**
     * Initialize the TTS engine and load all required data.
     * Called from onCreate and onStartCommand to handle process restart.
     */
    private fun initializeEngine() {
        tts = LaprdusTTS.getInstance()

        // Initialize settings repository and start collecting settings
        if (!::settingsRepo.isInitialized) {
            settingsRepo = SettingsRepository(applicationContext)
            settingsScope.launch {
                settingsRepo.allSettings.collect { settings ->
                    cachedSettings = settings
                    // Apply advanced settings to engine when loaded
                    tts?.let { engine ->
                        engine.emojiEnabled = settings.emojiEnabled
                        engine.sentencePause = settings.sentencePause
                        engine.commaPause = settings.commaPause
                        engine.newlinePause = settings.newlinePause
                        engine.numberMode = settings.numberMode
                    }
                }
            }
        }

        // Load saved voice from settings (blocking on first load for correct initialization)
        val savedVoiceId = runBlocking {
            try {
                settingsRepo.defaultVoice.first()
            } catch (e: Exception) {
                Log.w(TAG, "Could not load saved voice, using default: ${e.message}")
                "josip"
            }
        }
        currentVoiceId = savedVoiceId

        // Initialize with saved voice using setVoice
        // This ensures proper loading, pitch settings, and user dictionaries
        try {
            val success = setVoiceAndLoadUserDictionaries(currentVoiceId)
            if (success) {
                Log.d(TAG, "Engine initialized with $currentVoiceId voice")
            } else {
                Log.e(TAG, "Failed to initialize engine with $currentVoiceId voice")
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to initialize engine", e)
        }
    }

    /**
     * Set voice and reload user dictionaries.
     * Use this instead of calling tts.setVoice() directly to ensure
     * user dictionary entries are always loaded after the bundled dictionary.
     */
    private fun setVoiceAndLoadUserDictionaries(voiceId: String): Boolean {
        val engine = tts ?: return false
        val success = engine.setVoice(voiceId, assets)
        if (success) {
            loadUserDictionaries()
        }
        return success
    }

    /**
     * Load user dictionary entries from filesDir/user.json into the native engine.
     * Entries are appended to the already-loaded bundled dictionary using addPronunciation(),
     * which does NOT clear existing entries.
     * Respects the userDictionariesEnabled setting.
     */
    private fun loadUserDictionaries() {
        val settings = cachedSettings
        if (settings != null && !settings.userDictionariesEnabled) {
            Log.d(TAG, "User dictionaries disabled, skipping")
            return
        }

        val engine = tts ?: return

        val userDictFile = File(filesDir, "user.json")
        if (!userDictFile.exists()) {
            Log.d(TAG, "No user dictionary file found")
            return
        }

        try {
            val json = userDictFile.readText(Charsets.UTF_8)
            val jsonObj = JSONObject(json)
            val entriesArray = jsonObj.optJSONArray("entries") ?: return

            var count = 0
            for (i in 0 until entriesArray.length()) {
                val entryObj = entriesArray.getJSONObject(i)
                val grapheme = entryObj.optString("grapheme", "")
                val phoneme = entryObj.optString("phoneme", "")

                if (grapheme.isNotEmpty() && phoneme.isNotEmpty()) {
                    val caseSensitive = entryObj.optBoolean("caseSensitive", false)
                    val wholeWord = entryObj.optBoolean("wholeWord", true)
                    engine.addPronunciation(grapheme, phoneme, caseSensitive, wholeWord)
                    count++
                }
            }

            Log.i(TAG, "Loaded $count user dictionary entries")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to load user dictionary: ${e.message}")
        }
    }

    override fun onDestroy() {
        Log.d(TAG, "Service destroyed")
        settingsScope.cancel()
        tts?.shutdown()
        tts = null
        super.onDestroy()
    }

    /**
     * Called when the user removes the app from recent tasks.
     * We do NOT stop the service - it should continue running for TTS.
     * The android:stopWithTask="false" in manifest also helps with this.
     */
    override fun onTaskRemoved(rootIntent: Intent?) {
        Log.d(TAG, "Task removed, service continues running")
        // Do NOT call super.onTaskRemoved() or stopSelf()
        // The TTS service should continue running independently of the app task

        // Ensure engine is still initialized
        if (tts == null || tts?.isInitialized() != true) {
            Log.d(TAG, "Reinitializing engine after task removal")
            initializeEngine()
        }
    }

    /**
     * Check if a language is supported.
     * Supports Croatian (hr) and Serbian (sr).
     */
    override fun onIsLanguageAvailable(lang: String, country: String?, variant: String?): Int {
        val normalizedLang = lang.lowercase()

        return when {
            normalizedLang == "hr" || normalizedLang == "hrv" -> {
                if (country?.lowercase() == "hr") {
                    TextToSpeech.LANG_COUNTRY_AVAILABLE
                } else {
                    TextToSpeech.LANG_AVAILABLE
                }
            }
            normalizedLang == "sr" || normalizedLang == "srp" -> {
                if (country?.lowercase() == "rs") {
                    TextToSpeech.LANG_COUNTRY_AVAILABLE
                } else {
                    TextToSpeech.LANG_AVAILABLE
                }
            }
            else -> TextToSpeech.LANG_NOT_SUPPORTED
        }
    }

    /**
     * Get the current language configuration.
     */
    override fun onGetLanguage(): Array<String> {
        return when {
            currentVoiceId in listOf("josip", "detence", "baba") -> arrayOf("hr", "HR", "")
            currentVoiceId in listOf("vlado", "djed") -> arrayOf("sr", "RS", "")
            else -> arrayOf("hr", "HR", "")
        }
    }

    /**
     * Load the specified language.
     */
    override fun onLoadLanguage(lang: String, country: String?, variant: String?): Int {
        val available = onIsLanguageAvailable(lang, country, variant)

        if (available == TextToSpeech.LANG_NOT_SUPPORTED) {
            return available
        }

        // Select appropriate default voice
        val normalizedLang = lang.lowercase()
        val voiceId = when {
            normalizedLang == "hr" || normalizedLang == "hrv" -> "josip"
            normalizedLang == "sr" || normalizedLang == "srp" -> "vlado"
            else -> "josip"
        }

        return try {
            setVoiceAndLoadUserDictionaries(voiceId)
            currentVoiceId = voiceId
            Log.d(TAG, "Loaded language: $lang with voice: $voiceId")
            available
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

        Log.d(TAG, "Returning ${voices.size} voices")
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
                Log.d(TAG, "Loaded voice: $voiceName")
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

        // Attempt to reinitialize if engine is not ready (handles process restart case)
        if (engine == null || !engine.isInitialized()) {
            Log.w(TAG, "Engine not initialized, attempting reinitialization...")
            initializeEngine()
            engine = tts
        }

        if (engine == null || !engine.isInitialized()) {
            Log.e(TAG, "Engine not initialized after reinitialization attempt")
            callback.error()
            return
        }

        // Get text to synthesize
        val text = request.charSequenceText?.toString() ?: ""
        if (text.isEmpty()) {
            callback.done()
            return
        }

        Log.d(TAG, "Synthesizing: ${text.take(50)}...")

        try {
            // Use cached settings (non-blocking) - falls back to defaults if not yet loaded
            val settings = cachedSettings

            // Apply speech rate - use Laprdus settings if force is enabled
            val speechRate = if (settings?.forceSpeed == true) {
                Log.d(TAG, "Using forced Laprdus speed: ${settings.speed}")
                settings.speed
            } else {
                // Android uses 100 as normal = 1.0
                (request.speechRate / 100f).coerceIn(0.5f, 2.0f)
            }
            engine.speed = speechRate

            // Apply pitch - use Laprdus settings if force is enabled
            val pitch = if (settings?.forcePitch == true) {
                Log.d(TAG, "Using forced Laprdus pitch: ${settings.pitch}")
                settings.pitch
            } else {
                // Android uses 100 as normal = 1.0
                (request.pitch / 100f).coerceIn(0.5f, 2.0f)
            }
            engine.pitch = pitch

            // Apply volume - use Laprdus settings if force is enabled
            if (settings?.forceVolume == true) {
                Log.d(TAG, "Using forced Laprdus volume: ${settings.volume}")
                engine.volume = settings.volume
            }

            // Apply force language - use saved voice regardless of request
            if (settings?.forceLanguage == true) {
                val savedVoice = settings.defaultVoice
                if (savedVoice != currentVoiceId) {
                    Log.d(TAG, "Using forced language voice: $savedVoice")
                    setVoiceAndLoadUserDictionaries(savedVoice)
                    currentVoiceId = savedVoice
                }
            }

            // Synthesize - use spelled mode for single characters (TalkBack accessibility)
            val useSpeledMode = isSingleGrapheme(text)
            val samples = if (useSpeledMode) {
                Log.d(TAG, "Using spelled synthesis for single character: '$text'")
                engine.synthesizeSpelled(text)
            } else {
                engine.synthesize(text)
            }

            if (samples == null || samples.isEmpty()) {
                Log.e(TAG, "Synthesis returned no samples")
                callback.error()
                return
            }

            Log.d(TAG, "Synthesized ${samples.size} samples (spelled=$useSpeledMode)")

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
            Log.d(TAG, "Synthesis complete")

        } catch (e: Exception) {
            Log.e(TAG, "Exception during synthesis", e)
            callback.error()
        }
    }

    /**
     * Stop any ongoing synthesis.
     */
    override fun onStop() {
        Log.d(TAG, "Stop requested")
        tts?.cancel()
    }
}
