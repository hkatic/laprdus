package com.hrvojekatic.laprdus.viewmodel

import android.content.Context
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.hrvojekatic.laprdus.data.SettingsRepository
import com.hrvojekatic.laprdus.tts.LaprdusTTS
import com.hrvojekatic.laprdus.tts.VoiceInfo
import dagger.hilt.android.lifecycle.HiltViewModel
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import javax.inject.Inject

/**
 * UI state for the Settings screen.
 */
data class SettingsUiState(
    val isLoading: Boolean = true,
    val availableVoices: List<VoiceInfo> = emptyList(),
    val selectedVoiceId: String = SettingsRepository.DEFAULT_VOICE,
    val speed: Float = SettingsRepository.DEFAULT_SPEED,
    val pitch: Float = SettingsRepository.DEFAULT_PITCH,
    val volume: Float = SettingsRepository.DEFAULT_VOLUME,
    val forceSpeed: Boolean = SettingsRepository.DEFAULT_FORCE_SPEED,
    val forcePitch: Boolean = SettingsRepository.DEFAULT_FORCE_PITCH,
    val forceVolume: Boolean = SettingsRepository.DEFAULT_FORCE_VOLUME,
    val forceLanguage: Boolean = SettingsRepository.DEFAULT_FORCE_LANGUAGE,
    // Advanced settings
    val emojiEnabled: Boolean = SettingsRepository.DEFAULT_EMOJI_ENABLED,
    val inflectionEnabled: Boolean = SettingsRepository.DEFAULT_INFLECTION_ENABLED,
    val sentencePause: Int = SettingsRepository.DEFAULT_SENTENCE_PAUSE,
    val commaPause: Int = SettingsRepository.DEFAULT_COMMA_PAUSE,
    val newlinePause: Int = SettingsRepository.DEFAULT_NEWLINE_PAUSE,
    val numberMode: Int = SettingsRepository.DEFAULT_NUMBER_MODE,
    // Dictionary settings
    val userDictionariesEnabled: Boolean = SettingsRepository.DEFAULT_USER_DICTIONARIES_ENABLED,
    val error: String? = null
)

/**
 * ViewModel for the Settings screen.
 * Manages settings state and persistence for voice, rate, pitch, volume, and force toggles.
 */
@HiltViewModel
class SettingsViewModel @Inject constructor(
    @ApplicationContext private val context: Context,
    private val settings: SettingsRepository,
    private val tts: LaprdusTTS
) : ViewModel() {

    private val _uiState = MutableStateFlow(SettingsUiState())
    val uiState: StateFlow<SettingsUiState> = _uiState.asStateFlow()

    init {
        loadSettings()
    }

    /**
     * Load all settings from the repository and initialize available voices.
     */
    private fun loadSettings() {
        viewModelScope.launch {
            try {
                // Load available voices
                val voices = tts.getAllVoices()

                // Collect all settings from repository
                settings.allSettings.collect { allSettings ->
                    _uiState.update { state ->
                        state.copy(
                            isLoading = false,
                            availableVoices = voices,
                            selectedVoiceId = allSettings.defaultVoice,
                            speed = allSettings.speed,
                            pitch = allSettings.pitch,
                            volume = allSettings.volume,
                            forceSpeed = allSettings.forceSpeed,
                            forcePitch = allSettings.forcePitch,
                            forceVolume = allSettings.forceVolume,
                            forceLanguage = allSettings.forceLanguage,
                            emojiEnabled = allSettings.emojiEnabled,
                            inflectionEnabled = allSettings.inflectionEnabled,
                            sentencePause = allSettings.sentencePause,
                            commaPause = allSettings.commaPause,
                            newlinePause = allSettings.newlinePause,
                            numberMode = allSettings.numberMode,
                            userDictionariesEnabled = allSettings.userDictionariesEnabled,
                            error = null
                        )
                    }
                }
            } catch (e: Exception) {
                _uiState.update {
                    it.copy(
                        isLoading = false,
                        error = e.message ?: "Failed to load settings"
                    )
                }
            }
        }
    }

    // ==========================================================================
    // Voice Settings
    // ==========================================================================

    /**
     * Select a voice and persist it.
     * @param voiceId The ID of the voice to select
     */
    fun selectVoice(voiceId: String) {
        viewModelScope.launch {
            try {
                // Set the voice in the TTS engine
                val success = tts.setVoice(voiceId, context.assets)
                if (success) {
                    // Persist the selection
                    settings.setDefaultVoice(voiceId)
                    _uiState.update { it.copy(selectedVoiceId = voiceId, error = null) }
                } else {
                    _uiState.update { it.copy(error = "Failed to set voice: $voiceId") }
                }
            } catch (e: Exception) {
                _uiState.update { it.copy(error = e.message ?: "Failed to set voice") }
            }
        }
    }

    // ==========================================================================
    // Speed Settings
    // ==========================================================================

    /**
     * Set the speech speed and persist it.
     * @param speed Speed factor (0.5 - 2.0)
     */
    fun setSpeed(speed: Float) {
        val clampedSpeed = speed.coerceIn(0.5f, 2.0f)
        viewModelScope.launch {
            tts.speed = clampedSpeed
            settings.setSpeed(clampedSpeed)
            _uiState.update { it.copy(speed = clampedSpeed) }
        }
    }

    /**
     * Set whether to force Laprdus speed settings over application settings.
     * @param enabled True to force Laprdus speed settings
     */
    fun setForceSpeed(enabled: Boolean) {
        viewModelScope.launch {
            settings.setForceSpeed(enabled)
            _uiState.update { it.copy(forceSpeed = enabled) }
        }
    }

    /**
     * Restore speed to default value (1.0).
     */
    fun restoreDefaultSpeed() {
        viewModelScope.launch {
            settings.restoreDefaultSpeed()
            tts.speed = SettingsRepository.DEFAULT_SPEED
            _uiState.update { it.copy(speed = SettingsRepository.DEFAULT_SPEED) }
        }
    }

    // ==========================================================================
    // Pitch Settings
    // ==========================================================================

    /**
     * Set the pitch and persist it.
     * @param pitch Pitch factor (0.5 - 2.0)
     */
    fun setPitch(pitch: Float) {
        val clampedPitch = pitch.coerceIn(0.5f, 2.0f)
        viewModelScope.launch {
            tts.pitch = clampedPitch
            settings.setPitch(clampedPitch)
            _uiState.update { it.copy(pitch = clampedPitch) }
        }
    }

    /**
     * Set whether to force Laprdus pitch settings over application settings.
     * @param enabled True to force Laprdus pitch settings
     */
    fun setForcePitch(enabled: Boolean) {
        viewModelScope.launch {
            settings.setForcePitch(enabled)
            _uiState.update { it.copy(forcePitch = enabled) }
        }
    }

    /**
     * Restore pitch to default value (1.0).
     */
    fun restoreDefaultPitch() {
        viewModelScope.launch {
            settings.restoreDefaultPitch()
            tts.pitch = SettingsRepository.DEFAULT_PITCH
            _uiState.update { it.copy(pitch = SettingsRepository.DEFAULT_PITCH) }
        }
    }

    // ==========================================================================
    // Volume Settings
    // ==========================================================================

    /**
     * Set the volume and persist it.
     * @param volume Volume level (0.0 - 1.0)
     */
    fun setVolume(volume: Float) {
        val clampedVolume = volume.coerceIn(0.0f, 1.0f)
        viewModelScope.launch {
            tts.volume = clampedVolume
            settings.setVolume(clampedVolume)
            _uiState.update { it.copy(volume = clampedVolume) }
        }
    }

    /**
     * Set whether to force Laprdus volume settings over multimedia volume.
     * @param enabled True to force Laprdus volume settings
     */
    fun setForceVolume(enabled: Boolean) {
        viewModelScope.launch {
            settings.setForceVolume(enabled)
            _uiState.update { it.copy(forceVolume = enabled) }
        }
    }

    /**
     * Restore volume to default value (1.0).
     */
    fun restoreDefaultVolume() {
        viewModelScope.launch {
            settings.restoreDefaultVolume()
            tts.volume = SettingsRepository.DEFAULT_VOLUME
            _uiState.update { it.copy(volume = SettingsRepository.DEFAULT_VOLUME) }
        }
    }

    // ==========================================================================
    // Force Language Settings
    // ==========================================================================

    /**
     * Set whether to force the selected language regardless of system settings.
     * @param enabled True to force the selected language
     */
    fun setForceLanguage(enabled: Boolean) {
        viewModelScope.launch {
            settings.setForceLanguage(enabled)
            _uiState.update { it.copy(forceLanguage = enabled) }
        }
    }

    // ==========================================================================
    // Emoji Settings
    // ==========================================================================

    /**
     * Set whether emoji processing is enabled.
     * @param enabled True to enable emoji to text conversion
     */
    fun setEmojiEnabled(enabled: Boolean) {
        viewModelScope.launch {
            tts.emojiEnabled = enabled
            settings.setEmojiEnabled(enabled)
            _uiState.update { it.copy(emojiEnabled = enabled) }
        }
    }

    // ==========================================================================
    // Inflection Settings
    // ==========================================================================

    /**
     * Set whether voice inflection is enabled.
     * @param enabled True to enable pitch variation for questions, exclamations, and pauses
     */
    fun setInflectionEnabled(enabled: Boolean) {
        viewModelScope.launch {
            tts.inflectionEnabled = enabled
            settings.setInflectionEnabled(enabled)
            _uiState.update { it.copy(inflectionEnabled = enabled) }
        }
    }

    // ==========================================================================
    // Pause Settings
    // ==========================================================================

    /**
     * Set the sentence pause duration.
     * @param pauseMs Pause duration in milliseconds (0-2000)
     */
    fun setSentencePause(pauseMs: Int) {
        val clamped = pauseMs.coerceIn(0, 2000)
        viewModelScope.launch {
            tts.sentencePause = clamped
            settings.setSentencePause(clamped)
            _uiState.update { it.copy(sentencePause = clamped) }
        }
    }

    /**
     * Set the comma pause duration.
     * @param pauseMs Pause duration in milliseconds (0-2000)
     */
    fun setCommaPause(pauseMs: Int) {
        val clamped = pauseMs.coerceIn(0, 2000)
        viewModelScope.launch {
            tts.commaPause = clamped
            settings.setCommaPause(clamped)
            _uiState.update { it.copy(commaPause = clamped) }
        }
    }

    /**
     * Set the newline pause duration.
     * @param pauseMs Pause duration in milliseconds (0-2000)
     */
    fun setNewlinePause(pauseMs: Int) {
        val clamped = pauseMs.coerceIn(0, 2000)
        viewModelScope.launch {
            tts.newlinePause = clamped
            settings.setNewlinePause(clamped)
            _uiState.update { it.copy(newlinePause = clamped) }
        }
    }

    // ==========================================================================
    // Number Mode Settings
    // ==========================================================================

    /**
     * Set the number processing mode.
     * @param mode 0 for whole numbers, 1 for digit by digit
     */
    fun setNumberMode(mode: Int) {
        val clamped = mode.coerceIn(0, 1)
        viewModelScope.launch {
            tts.numberMode = clamped
            settings.setNumberMode(clamped)
            _uiState.update { it.copy(numberMode = clamped) }
        }
    }

    // ==========================================================================
    // User Dictionary Settings
    // ==========================================================================

    /**
     * Set whether user dictionaries are enabled.
     * @param enabled True to apply user dictionaries during synthesis
     */
    fun setUserDictionariesEnabled(enabled: Boolean) {
        viewModelScope.launch {
            settings.setUserDictionariesEnabled(enabled)
            _uiState.update { it.copy(userDictionariesEnabled = enabled) }
        }
    }

    // ==========================================================================
    // Error Handling
    // ==========================================================================

    /**
     * Clear the current error message.
     */
    fun clearError() {
        _uiState.update { it.copy(error = null) }
    }
}
