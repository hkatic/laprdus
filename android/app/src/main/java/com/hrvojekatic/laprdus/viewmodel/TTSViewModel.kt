package com.hrvojekatic.laprdus.viewmodel

import android.content.Context
import android.provider.Settings
import android.util.Log
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.hrvojekatic.laprdus.audio.AudioPlayer
import com.hrvojekatic.laprdus.data.SettingsRepository
import com.hrvojekatic.laprdus.tts.LaprdusTTS
import com.hrvojekatic.laprdus.tts.VoiceInfo
import dagger.hilt.android.lifecycle.HiltViewModel
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.Job
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import javax.inject.Inject

/**
 * UI state for the TTS screen
 */
data class TTSUiState(
    val isInitialized: Boolean = false,
    val isPlaying: Boolean = false,
    val inputText: String = "Dobar dan. Ja sam Laprdus, rođen sam 2026. godine, i drago mi je da se možemo upoznati! 😁\nKako si ti? ❤\n",
    val selectedVoiceId: String = "josip",
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
    @ApplicationContext private val context: Context,
    private val tts: LaprdusTTS,
    private val audioPlayer: AudioPlayer,
    private val settings: SettingsRepository
) : ViewModel() {

    companion object {
        private const val TAG = "TTSViewModel"
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
    }

    /**
     * Observe settings changes from SettingsActivity.
     * This ensures the main screen reflects changes made in settings.
     */
    private suspend fun observeSettingsChanges() {
        settings.allSettings.collect { savedSettings ->
            // Update TTS engine with new settings
            tts.speed = savedSettings.speed
            tts.pitch = savedSettings.pitch
            tts.volume = savedSettings.volume

            // Apply advanced settings
            tts.emojiEnabled = savedSettings.emojiEnabled
            tts.inflectionEnabled = savedSettings.inflectionEnabled
            tts.sentencePause = savedSettings.sentencePause
            tts.commaPause = savedSettings.commaPause
            tts.newlinePause = savedSettings.newlinePause
            tts.numberMode = savedSettings.numberMode

            // If voice changed, reload it
            if (_uiState.value.selectedVoiceId != savedSettings.defaultVoice &&
                _uiState.value.isInitialized) {
                tts.setVoice(savedSettings.defaultVoice, context.assets)
            }

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
                Log.w(TAG, "Saved voice '$defaultVoiceId' not found, falling back to josip")
                defaultVoiceId = "josip"
            }

            // Initialize audio player
            audioPlayer.initialize()

            // Initialize TTS with default voice using setVoice()
            // This ensures proper loading of voice data AND dictionaries
            Log.d(TAG, "Initializing with voice: $defaultVoiceId")
            val success = tts.setVoice(defaultVoiceId, context.assets)

            if (success) {
                // Apply saved settings
                tts.speed = savedSettings.speed
                tts.pitch = savedSettings.pitch
                tts.volume = savedSettings.volume

                // Apply advanced settings
                tts.emojiEnabled = savedSettings.emojiEnabled
                tts.inflectionEnabled = savedSettings.inflectionEnabled
                tts.sentencePause = savedSettings.sentencePause
                tts.commaPause = savedSettings.commaPause
                tts.newlinePause = savedSettings.newlinePause
                tts.numberMode = savedSettings.numberMode

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
                        error = "Greška pri pokretanju TTS motora. Provjerite da su glasovne datoteke instalirane."
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
                    error = "Greška: ${e.message}"
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
                val appContext = context

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
                    _uiState.update { it.copy(error = "Greška pri odabiru glasa") }
                }
            } catch (e: Exception) {
                Log.e(TAG, "Error selecting voice", e)
                _uiState.update { it.copy(error = "Greška: ${e.message}") }
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
                Log.d(TAG, "Synthesizing: $text")
                val samples = tts.synthesize(text)

                if (samples != null && samples.isNotEmpty()) {
                    Log.d(TAG, "Playing ${samples.size} samples")
                    audioPlayer.play(samples)
                } else {
                    Log.w(TAG, "Synthesis returned empty or null samples")
                }
            } catch (e: Exception) {
                Log.e(TAG, "Error during synthesis/playback", e)
                _uiState.update { it.copy(error = "Greška pri sintezi: ${e.message}") }
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
        audioPlayer.release()
        tts.shutdown()
    }
}
