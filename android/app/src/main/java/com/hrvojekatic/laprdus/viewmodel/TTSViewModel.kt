package com.hrvojekatic.laprdus.viewmodel

import android.content.Context
import android.provider.Settings
import android.util.Log
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.hrvojekatic.laprdus.BuildConfig
import com.hrvojekatic.laprdus.audio.AudioPlayer
import com.hrvojekatic.laprdus.data.DictionaryRepository
import com.hrvojekatic.laprdus.data.SettingsRepository
import com.hrvojekatic.laprdus.tts.LaprdusTTS
import com.hrvojekatic.laprdus.tts.VoiceInfo
import dagger.hilt.android.lifecycle.HiltViewModel
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.drop
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import com.hrvojekatic.laprdus.R
import javax.inject.Inject

/**
 * UI state for the TTS screen
 */
data class TTSUiState(
    val isInitialized: Boolean = false,
    val isPlaying: Boolean = false,
    val inputText: String = "Dobar dan. Ja sam Laprdus, rođen sam 2026. godine, i drago mi je da se možemo upoznati! 😁\nKako si ti? ❤\n",
    val selectedVoiceId: String = SettingsRepository.DEFAULT_VOICE,
    val availableVoices: List<VoiceInfo> = emptyList(),
    val speed: Float = 1.0f,
    val pitch: Float = 1.0f,
    val volume: Float = 1.0f,
    val error: String? = null,
    val isLoading: Boolean = true,
    // Default TTS dialog state
    val showDefaultTtsDialog: Boolean = false,
    val dontAskDefaultTtsChecked: Boolean = false
)

/**
 * ViewModel for the TTS main screen.
 * Manages TTS engine lifecycle, audio playback, and settings.
 */
@HiltViewModel
class TTSViewModel @Inject constructor(
    @param:ApplicationContext private val context: Context,
    private val tts: LaprdusTTS,
    private val audioPlayer: AudioPlayer,
    private val settings: SettingsRepository
) : ViewModel() {

    companion object {
        private const val TAG = "TTSViewModel"
    }

    /**
     * Debug-only logging: the message below carries the text the user asked to
     * have spoken, and R8 keeps android.util.Log calls in release builds.
     */
    private inline fun logDebug(message: () -> String) {
        if (BuildConfig.DEBUG) Log.d(TAG, message())
    }

    private val _uiState = MutableStateFlow(TTSUiState())
    val uiState: StateFlow<TTSUiState> = _uiState.asStateFlow()

    private var playbackJob: Job? = null

    // Flag to ensure we only check default TTS once per app session
    // This prevents the dialog from showing when returning from Settings or on config changes
    private var hasCheckedDefaultTtsThisSession = false

    init {
        viewModelScope.launch {
            initializeEngine()
        }
        // Observe settings changes from SettingsActivity
        viewModelScope.launch {
            observeSettingsChanges()
        }
        // A dictionary entry edited in the app is heard in the next preview
        viewModelScope.launch {
            DictionaryRepository.changes.drop(1).collect {
                withContext(Dispatchers.Default) { tts.reloadDictionaries(context.assets) }
            }
        }
    }

    /**
     * Observe settings changes from SettingsActivity.
     * This ensures the main screen reflects changes made in settings.
     */
    private suspend fun observeSettingsChanges() {
        var previous: SettingsRepository.TTSSettings? = null
        settings.allSettings.collect { savedSettings ->
            // Update TTS engine with new settings
            tts.speed = savedSettings.speed
            tts.pitch = savedSettings.pitch
            tts.volume = savedSettings.volume

            // Apply advanced settings
            tts.applySettings(savedSettings)

            // If voice changed, reload it (unless the settings screen already
            // loaded it into this engine)
            if (_uiState.value.isInitialized && tts.currentVoiceId != savedSettings.defaultVoice) {
                val success = withContext(Dispatchers.Default) {
                    tts.setVoice(savedSettings.defaultVoice, context.assets)
                }
                if (!success) {
                    Log.e(TAG, "Failed to switch voice in settings observer")
                    _uiState.update { it.copy(isInitialized = false, error = context.getString(R.string.error_voice_failed)) }
                }
            } else if (previous?.let { it.userDictionariesEnabled != savedSettings.userDictionariesEnabled } == true) {
                withContext(Dispatchers.Default) { tts.reloadDictionaries(context.assets) }
            }
            previous = savedSettings

            // Update UI state
            _uiState.update {
                it.copy(
                    selectedVoiceId = savedSettings.defaultVoice,
                    speed = savedSettings.speed,
                    pitch = savedSettings.pitch,
                    volume = savedSettings.volume
                )
            }
        }
    }

    /**
     * Initialize the TTS engine and load settings
     */
    private suspend fun initializeEngine() {
        Log.d(TAG, "Initializing TTS engine")

        try {
            // Load saved settings first
            val savedSettings = settings.allSettings.first()
            var defaultVoiceId = savedSettings.defaultVoice

            // Get all available voices from static registry (doesn't depend on initialization)
            val voices = tts.getAllVoices()
            Log.d(TAG, "Loaded ${voices.size} voices from registry")

            // Validate the saved voice ID
            if (voices.none { it.id == defaultVoiceId }) {
                Log.w(TAG, "Saved voice '$defaultVoiceId' not found, falling back to the default")
                defaultVoiceId = SettingsRepository.DEFAULT_VOICE
            }

            // Initialize audio player
            audioPlayer.initialize()

            // The advanced settings first: the user dictionaries switch
            // decides what setVoice() loads on top of the bundled dictionaries.
            tts.applySettings(savedSettings)

            // Initialize TTS with default voice using setVoice()
            // This ensures proper loading of voice data AND dictionaries
            Log.d(TAG, "Initializing with voice: $defaultVoiceId")
            val success = withContext(Dispatchers.Default) {
                tts.setVoice(defaultVoiceId, context.assets)
            }

            if (success) {
                // Apply saved settings
                tts.speed = savedSettings.speed
                tts.pitch = savedSettings.pitch
                tts.volume = savedSettings.volume

                _uiState.update {
                    it.copy(
                        isInitialized = true,
                        isLoading = false,
                        availableVoices = voices,
                        selectedVoiceId = defaultVoiceId,
                        speed = savedSettings.speed,
                        pitch = savedSettings.pitch,
                        volume = savedSettings.volume
                    )
                }

                Log.d(TAG, "Engine initialized with ${voices.size} voices, default voice: $defaultVoiceId")

                // Check default TTS engine after initialization completes
                checkDefaultTtsEngine()
            } else {
                // Engine initialization failed, but still show voices list for selection
                _uiState.update {
                    it.copy(
                        isInitialized = false,
                        isLoading = false,
                        availableVoices = voices,
                        selectedVoiceId = defaultVoiceId,
                        speed = savedSettings.speed,
                        pitch = savedSettings.pitch,
                        volume = savedSettings.volume,
                        error = context.getString(R.string.error_init_failed)
                    )
                }
                Log.e(TAG, "Failed to initialize TTS engine with voice: $defaultVoiceId")

                // Check default TTS engine even if initialization failed
                checkDefaultTtsEngine()
            }
        } catch (e: Exception) {
            Log.e(TAG, "Exception during initialization", e)
            // Even on exception, try to load voices
            val voices = try { tts.getAllVoices() } catch (_: Exception) { emptyList() }
            _uiState.update {
                it.copy(
                    isLoading = false,
                    availableVoices = voices,
                    error = context.getString(R.string.error_prefix, e.message ?: "")
                )
            }

            // Check default TTS engine even on exception
            checkDefaultTtsEngine()
        }
    }

    /**
     * Update the input text
     */
    fun updateInputText(text: String) {
        _uiState.update { it.copy(inputText = text) }
    }

    /**
     * Select a voice
     */
    fun selectVoice(voiceId: String) {
        viewModelScope.launch {
            try {
                // Set the voice (this loads the correct .bin and applies pitch)
                val success = tts.setVoice(voiceId, context.assets)

                if (success) {
                    _uiState.update {
                        it.copy(
                            selectedVoiceId = voiceId,
                            isInitialized = true,
                            error = null
                        )
                    }
                    settings.setDefaultVoice(voiceId)
                    Log.d(TAG, "Voice selected: $voiceId")
                } else {
                    _uiState.update { it.copy(error = context.getString(R.string.error_voice_failed)) }
                }
            } catch (e: Exception) {
                Log.e(TAG, "Error selecting voice", e)
                _uiState.update { it.copy(error = context.getString(R.string.error_prefix, e.message ?: "")) }
            }
        }
    }

    /**
     * Set the speech speed
     */
    fun setSpeed(speed: Float) {
        val clampedSpeed = speed.coerceIn(0.5f, 2.0f)
        tts.speed = clampedSpeed
        _uiState.update { it.copy(speed = clampedSpeed) }
        viewModelScope.launch {
            settings.setSpeed(clampedSpeed)
        }
    }

    /**
     * Set the pitch
     */
    fun setPitch(pitch: Float) {
        val clampedPitch = pitch.coerceIn(0.5f, 2.0f)
        tts.pitch = clampedPitch
        _uiState.update { it.copy(pitch = clampedPitch) }
        viewModelScope.launch {
            settings.setPitch(clampedPitch)
        }
    }

    /**
     * Set the volume
     */
    fun setVolume(volume: Float) {
        val clampedVolume = volume.coerceIn(0.0f, 1.0f)
        tts.volume = clampedVolume
        _uiState.update { it.copy(volume = clampedVolume) }
        viewModelScope.launch {
            settings.setVolume(clampedVolume)
        }
    }

    /**
     * Speak the current input text
     */
    fun speak() {
        val text = _uiState.value.inputText
        if (text.isBlank()) {
            return
        }

        // Cancel any ongoing playback
        stop()

        playbackJob = viewModelScope.launch {
            _uiState.update { it.copy(isPlaying = true, error = null) }

            try {
                // The engine has no voice when loading it failed before: try again.
                if (!tts.isInitialized()) {
                    Log.w(TAG, "Engine has no voice, reinitializing...")
                    val voiceId = _uiState.value.selectedVoiceId
                    val success = withContext(Dispatchers.Default) {
                        tts.setVoice(voiceId, context.assets)
                    }
                    if (!success) {
                        _uiState.update { it.copy(error = context.getString(R.string.error_init_failed)) }
                        return@launch
                    }
                }

                logDebug { "Synthesizing: $text" }
                // The app's screens share this engine (the settings sliders set
                // it as they move): the preview speaks with the saved settings.
                val state = _uiState.value
                val samples = withContext(Dispatchers.Default) {
                    tts.exclusive {
                        tts.speed = state.speed
                        tts.pitch = state.pitch
                        tts.volume = state.volume
                        tts.synthesize(text)
                    }
                }

                if (samples != null && samples.isNotEmpty()) {
                    Log.d(TAG, "Playing ${samples.size} samples")
                    audioPlayer.play(samples)
                } else {
                    Log.w(TAG, "Synthesis returned empty or null samples")
                }
            } catch (e: kotlinx.coroutines.CancellationException) {
                throw e
            } catch (e: Exception) {
                Log.e(TAG, "Error during synthesis/playback", e)
                _uiState.update { it.copy(error = context.getString(R.string.error_synthesis_failed)) }
            } finally {
                _uiState.update { it.copy(isPlaying = false) }
            }
        }
    }

    /**
     * Stop playback
     */
    fun stop() {
        playbackJob?.cancel()
        playbackJob = null
        tts.cancel()
        audioPlayer.stop()
        _uiState.update { it.copy(isPlaying = false) }
    }

    /**
     * Clear any error message
     */
    fun clearError() {
        _uiState.update { it.copy(error = null) }
    }

    // ==========================================================================
    // Default TTS Engine Dialog
    // ==========================================================================

    /**
     * Check if Laprdus is the default TTS engine and show dialog if not.
     * Only runs once per app session to avoid showing dialog when returning
     * from Settings, on orientation change, or when resuming from background.
     */
    fun checkDefaultTtsEngine() {
        // Only check once per app session
        if (hasCheckedDefaultTtsThisSession) {
            return
        }

        // Don't check while still loading
        if (_uiState.value.isLoading) {
            return
        }

        // Mark as checked for this session
        hasCheckedDefaultTtsThisSession = true

        viewModelScope.launch {
            val dontAsk = settings.dontAskDefaultTts.first()
            if (dontAsk) {
                return@launch
            }

            val defaultEngine = Settings.Secure.getString(
                context.contentResolver,
                Settings.Secure.TTS_DEFAULT_SYNTH
            )
            val isLaprdusDefault = defaultEngine == context.packageName

            if (!isLaprdusDefault) {
                _uiState.update { it.copy(showDefaultTtsDialog = true) }
            }
        }
    }

    /**
     * Toggle the "don't ask again" checkbox in the dialog.
     */
    fun toggleDontAskDefaultTts(checked: Boolean) {
        _uiState.update { it.copy(dontAskDefaultTtsChecked = checked) }
    }

    /**
     * Dismiss the default TTS dialog.
     * If "don't ask again" is checked, persist the preference.
     */
    fun dismissDefaultTtsDialog() {
        viewModelScope.launch {
            if (_uiState.value.dontAskDefaultTtsChecked) {
                settings.setDontAskDefaultTts(true)
            }
            _uiState.update {
                it.copy(
                    showDefaultTtsDialog = false,
                    dontAskDefaultTtsChecked = false
                )
            }
        }
    }

    /**
     * Called when user confirms to set Laprdus as default TTS.
     * Persists "don't ask again" preference if checked, then dismisses dialog.
     * The caller should open TTS settings after this.
     */
    fun confirmSetDefaultTts() {
        viewModelScope.launch {
            if (_uiState.value.dontAskDefaultTtsChecked) {
                settings.setDontAskDefaultTts(true)
            }
            _uiState.update {
                it.copy(
                    showDefaultTtsDialog = false,
                    dontAskDefaultTtsChecked = false
                )
            }
        }
    }

    override fun onCleared() {
        super.onCleared()
        Log.d(TAG, "ViewModel cleared, releasing resources")
        playbackJob?.cancel()
        tts.cancel()
        audioPlayer.stop()
        audioPlayer.release()
        // Do NOT call tts.shutdown() here: the app's engine outlives this
        // ViewModel and the settings screen uses it too. The speech service
        // has an engine of its own.
    }
}
