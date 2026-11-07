package com.hrvojekatic.laprdus.viewmodel

import android.content.Context
import androidx.compose.runtime.Immutable
import androidx.compose.runtime.Stable
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.hrvojekatic.laprdus.R
import com.hrvojekatic.laprdus.data.SettingsRepository
import com.hrvojekatic.laprdus.tts.Language
import com.hrvojekatic.laprdus.tts.LaprdusTTS
import com.hrvojekatic.laprdus.tts.VoiceInfo
import dagger.hilt.android.lifecycle.HiltViewModel
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.distinctUntilChanged
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import javax.inject.Inject

/**
 * UI state for the Settings screen.
 */
@Immutable
data class SettingsUiState(
    val isLoading: Boolean = true,
    val availableVoices: List<VoiceInfo> = emptyList(),
    val selectedVoiceId: String = SettingsRepository.DEFAULT_VOICE,
    /** The chosen language ("hr", "sr", "bs" or "sl"); every voice speaks every language. */
    val language: String = SettingsRepository.DEFAULT_LANGUAGE,
    val speed: Float = SettingsRepository.DEFAULT_SPEED,
    val pitch: Float = SettingsRepository.DEFAULT_PITCH,
    val volume: Float = SettingsRepository.DEFAULT_VOLUME,
    /** Louder speech through a peak limiter. */
    val volumeBoost: Boolean = SettingsRepository.DEFAULT_VOLUME_BOOST,
    val forceSpeed: Boolean = SettingsRepository.DEFAULT_FORCE_SPEED,
    val forcePitch: Boolean = SettingsRepository.DEFAULT_FORCE_PITCH,
    val forceVolume: Boolean = SettingsRepository.DEFAULT_FORCE_VOLUME,
    val forceLanguage: Boolean = SettingsRepository.DEFAULT_FORCE_LANGUAGE,
    // Advanced settings
    val emojiEnabled: Boolean = SettingsRepository.DEFAULT_EMOJI_ENABLED,
    val inflectionLevel: Float = SettingsRepository.DEFAULT_INFLECTION_LEVEL,
    val acceleration: Float = SettingsRepository.DEFAULT_ACCELERATION,
    /** Voice quality, 0-100 each, 50 = the voice as it is. */
    val headSize: Int = SettingsRepository.DEFAULT_HEAD_SIZE,
    val breathiness: Int = SettingsRepository.DEFAULT_BREATHINESS,
    val roughness: Int = SettingsRepository.DEFAULT_ROUGHNESS,
    val brightness: Int = SettingsRepository.DEFAULT_BRIGHTNESS,
    /** Words per minute of the selected voice at speed 1.0 and acceleration 1.0. */
    val nominalWpm: Float = 0f,
    val sentencePause: Int = SettingsRepository.DEFAULT_SENTENCE_PAUSE,
    val commaPause: Int = SettingsRepository.DEFAULT_COMMA_PAUSE,
    val newlinePause: Int = SettingsRepository.DEFAULT_NEWLINE_PAUSE,
    val numberMode: Int = SettingsRepository.DEFAULT_NUMBER_MODE,
    val spellingSpeed: Int = SettingsRepository.DEFAULT_SPELLING_SPEED,
    /** True when letters are spelled by their sounds (the default) instead of their names. */
    val letterSounds: Boolean = SettingsRepository.DEFAULT_SPELLING_MODE == SettingsRepository.SPELLING_MODE_SOUNDS,
    // Dictionary settings
    val userDictionariesEnabled: Boolean = SettingsRepository.DEFAULT_USER_DICTIONARIES_ENABLED,
    val error: String? = null,
    /** Persistent notice that saved settings could not be migrated (null when fine). */
    val storageError: String? = null
) {
    /** Whether the selected voice is a formant voice (Zvonko, Stojan, Mirsad). */
    val isFormantVoice: Boolean
        get() = VoiceInfo.isFormantVoice(selectedVoiceId)

    /** Speed and pitch range of the selected voice. */
    val speedPitchRange: ClosedFloatingPointRange<Float>
        get() = VoiceInfo.rangeFor(selectedVoiceId)
}

/** What the Settings screen can change; [SettingsViewModel] does it. */
@Stable
interface SettingsActions {
    fun selectVoice(voiceId: String)
    fun selectLanguage(language: String)
    fun setSpeed(speed: Float)
    fun setPitch(pitch: Float)
    fun setVolume(volume: Float)
    fun setVolumeBoost(enabled: Boolean)
    fun setForceSpeed(enabled: Boolean)
    fun setForcePitch(enabled: Boolean)
    fun setForceVolume(enabled: Boolean)
    fun setForceLanguage(enabled: Boolean)
    fun setEmojiEnabled(enabled: Boolean)
    fun setInflectionLevel(level: Float)
    fun setAcceleration(acceleration: Float)
    fun setHeadSize(value: Int)
    fun setBreathiness(value: Int)
    fun setRoughness(value: Int)
    fun setBrightness(value: Int)
    fun setSentencePause(pauseMs: Int)
    fun setCommaPause(pauseMs: Int)
    fun setNewlinePause(pauseMs: Int)
    fun setNumberMode(mode: Int)
    fun setSpellingSpeed(percent: Int)
    fun setLetterSounds(enabled: Boolean)
    fun setUserDictionariesEnabled(enabled: Boolean)
    fun clearError()
}

/**
 * ViewModel for the Settings screen.
 *
 * A change shows at once (the state is updated before it is saved) and is
 * saved in order by one writer off the main thread. The screen's engine
 * calls are left to those who speak: the main screen's engine and the
 * speech service follow the saved settings, so a change here costs one
 * DataStore write, not a round of engine calls on the main thread.
 */
@HiltViewModel
class SettingsViewModel @Inject constructor(
    @param:ApplicationContext private val context: Context,
    private val settings: SettingsRepository,
    private val tts: LaprdusTTS
) : ViewModel(), SettingsActions {

    private val _uiState = MutableStateFlow(SettingsUiState())
    val uiState: StateFlow<SettingsUiState> = _uiState.asStateFlow()

    /**
     * Saves run one after the other, in the order the user made them. The
     * writer is not tied to the ViewModel: a change made just before leaving
     * the screen is still saved.
     */
    private val writes = Channel<suspend (SettingsRepository) -> Unit>(Channel.UNLIMITED)
    private val writerScope = CoroutineScope(SupervisorJob() + Dispatchers.IO)

    init {
        writerScope.launch {
            for (write in writes) {
                try {
                    write(settings)
                } catch (e: CancellationException) {
                    throw e
                } catch (e: Exception) {
                    _uiState.update { it.copy(error = context.getString(R.string.error_prefix, e.message ?: "")) }
                }
            }
        }
        loadSettings()
        observeStorageErrors()
    }

    override fun onCleared() {
        // The writer finishes what is queued, then ends.
        writes.close()
        super.onCleared()
    }

    /** Shows [change] at once and queues the save. */
    private fun save(change: (SettingsUiState) -> SettingsUiState, write: suspend (SettingsRepository) -> Unit) {
        _uiState.update(change)
        writes.trySend(write)
    }

    /**
     * Surface storage/migration problems reported by the repository (for example
     * when settings saved by an older version could not be moved into
     * device-protected storage) so the user learns why defaults are in use.
     */
    private fun observeStorageErrors() {
        viewModelScope.launch {
            settings.storageError.collect { message ->
                _uiState.update {
                    it.copy(
                        storageError = message?.let { context.getString(R.string.error_storage_migration) }
                    )
                }
            }
        }
    }

    /**
     * Load all settings from the repository and the voice list.
     */
    private fun loadSettings() {
        viewModelScope.launch {
            try {
                val voices = withContext(Dispatchers.Default) { tts.getAllVoices() }
                var wpmVoice: String? = null
                var wpm = 0f
                settings.allSettings.distinctUntilChanged().collect { saved ->
                    if (saved.defaultVoice != wpmVoice) {
                        wpm = withContext(Dispatchers.Default) { tts.getNominalWpm(saved.defaultVoice) }
                        wpmVoice = saved.defaultVoice
                    }
                    _uiState.update { state ->
                        state.copy(
                            isLoading = false,
                            availableVoices = voices,
                            selectedVoiceId = saved.defaultVoice,
                            language = saved.language,
                            speed = saved.speed,
                            pitch = saved.pitch,
                            volume = saved.volume,
                            volumeBoost = saved.volumeBoost,
                            forceSpeed = saved.forceSpeed,
                            forcePitch = saved.forcePitch,
                            forceVolume = saved.forceVolume,
                            forceLanguage = saved.forceLanguage,
                            emojiEnabled = saved.emojiEnabled,
                            inflectionLevel = saved.inflectionLevel,
                            acceleration = saved.acceleration,
                            headSize = saved.headSize,
                            breathiness = saved.breathiness,
                            roughness = saved.roughness,
                            brightness = saved.brightness,
                            nominalWpm = wpm,
                            sentencePause = saved.sentencePause,
                            commaPause = saved.commaPause,
                            newlinePause = saved.newlinePause,
                            numberMode = saved.numberMode,
                            spellingSpeed = saved.spellingSpeed,
                            letterSounds = saved.spellingMode == SettingsRepository.SPELLING_MODE_SOUNDS,
                            userDictionariesEnabled = saved.userDictionariesEnabled,
                        )
                    }
                }
            } catch (e: CancellationException) {
                throw e
            } catch (e: Exception) {
                _uiState.update {
                    it.copy(
                        isLoading = false,
                        error = context.getString(R.string.error_prefix, e.message ?: "")
                    )
                }
            }
        }
    }

    // ==========================================================================
    // Voice and language
    // ==========================================================================

    /**
     * Select a voice and persist it. The app's engine loads it first (off the
     * main thread), so a voice that cannot be loaded is not saved; the speech
     * service loads it into its own engine when it sees the saved choice.
     */
    override fun selectVoice(voiceId: String) {
        viewModelScope.launch {
            try {
                val loaded = withContext(Dispatchers.Default) { tts.setVoice(voiceId, context.assets) }
                if (!loaded) {
                    _uiState.update { it.copy(error = context.getString(R.string.error_voice_failed)) }
                    return@launch
                }
                val wpm = withContext(Dispatchers.Default) { tts.getNominalWpm(voiceId) }
                // A recorded voice cannot use the wider rate and pitch of the
                // formant voices: pull the values back into its range.
                val range = VoiceInfo.rangeFor(voiceId)
                val state = _uiState.value
                val speed = state.speed.coerceIn(range)
                val pitch = state.pitch.coerceIn(range)
                save({ it.copy(selectedVoiceId = voiceId, speed = speed, pitch = pitch, nominalWpm = wpm) }) {
                    it.setDefaultVoice(voiceId)
                    if (speed != state.speed) it.setSpeed(speed)
                    if (pitch != state.pitch) it.setPitch(pitch)
                }
            } catch (e: CancellationException) {
                throw e
            } catch (e: Exception) {
                _uiState.update { it.copy(error = context.getString(R.string.error_prefix, e.message ?: "")) }
            }
        }
    }

    /**
     * Choose the language. Every voice speaks every language: the voice
     * stays. It is spoken when "Force language" is on or the system language
     * is not one Laprdus speaks (Language.effective).
     */
    override fun selectLanguage(language: String) {
        if (!Language.isValid(language)) return
        save({ it.copy(language = language) }) { it.setLanguage(language) }
    }

    // ==========================================================================
    // Rate, pitch, volume and the force switches
    // ==========================================================================

    override fun setSpeed(speed: Float) {
        val value = speed.coerceIn(_uiState.value.speedPitchRange)
        save({ it.copy(speed = value) }) { it.setSpeed(value) }
    }

    override fun setPitch(pitch: Float) {
        val value = pitch.coerceIn(_uiState.value.speedPitchRange)
        save({ it.copy(pitch = value) }) { it.setPitch(value) }
    }

    override fun setVolume(volume: Float) {
        val value = volume.coerceIn(0.0f, 1.0f)
        save({ it.copy(volume = value) }) { it.setVolume(value) }
    }

    /** Louder speech through a peak limiter. */
    override fun setVolumeBoost(enabled: Boolean) =
        save({ it.copy(volumeBoost = enabled) }) { it.setVolumeBoost(enabled) }

    override fun setForceSpeed(enabled: Boolean) =
        save({ it.copy(forceSpeed = enabled) }) { it.setForceSpeed(enabled) }

    override fun setForcePitch(enabled: Boolean) =
        save({ it.copy(forcePitch = enabled) }) { it.setForcePitch(enabled) }

    override fun setForceVolume(enabled: Boolean) =
        save({ it.copy(forceVolume = enabled) }) { it.setForceVolume(enabled) }

    /**
     * Whether the chosen language is spoken everywhere, ignoring the system
     * language and the language apps request.
     */
    override fun setForceLanguage(enabled: Boolean) =
        save({ it.copy(forceLanguage = enabled) }) { it.setForceLanguage(enabled) }

    // ==========================================================================
    // More voice settings
    // ==========================================================================

    override fun setEmojiEnabled(enabled: Boolean) =
        save({ it.copy(emojiEnabled = enabled) }) { it.setEmojiEnabled(enabled) }

    /** 0.0 (monotone) to 1.0, 0.5 as measured. */
    override fun setInflectionLevel(level: Float) {
        val value = level.coerceIn(0.0f, 1.0f)
        save({ it.copy(inflectionLevel = value) }) { it.setInflectionLevel(value) }
    }

    /** Rate multiplier, 0.5 - 3.0. */
    override fun setAcceleration(acceleration: Float) {
        val value = acceleration.coerceIn(SettingsRepository.ACCELERATION_RANGE)
        save({ it.copy(acceleration = value) }) { it.setAcceleration(value) }
    }

    override fun setHeadSize(value: Int) {
        val v = value.coerceIn(SettingsRepository.VOICE_QUALITY_RANGE)
        save({ it.copy(headSize = v) }) { it.setHeadSize(v) }
    }

    override fun setBreathiness(value: Int) {
        val v = value.coerceIn(SettingsRepository.VOICE_QUALITY_RANGE)
        save({ it.copy(breathiness = v) }) { it.setBreathiness(v) }
    }

    override fun setRoughness(value: Int) {
        val v = value.coerceIn(SettingsRepository.VOICE_QUALITY_RANGE)
        save({ it.copy(roughness = v) }) { it.setRoughness(v) }
    }

    override fun setBrightness(value: Int) {
        val v = value.coerceIn(SettingsRepository.VOICE_QUALITY_RANGE)
        save({ it.copy(brightness = v) }) { it.setBrightness(v) }
    }

    // ==========================================================================
    // Reading
    // ==========================================================================

    override fun setSentencePause(pauseMs: Int) {
        val v = pauseMs.coerceIn(0, 2000)
        save({ it.copy(sentencePause = v) }) { it.setSentencePause(v) }
    }

    override fun setCommaPause(pauseMs: Int) {
        val v = pauseMs.coerceIn(0, 2000)
        save({ it.copy(commaPause = v) }) { it.setCommaPause(v) }
    }

    override fun setNewlinePause(pauseMs: Int) {
        val v = pauseMs.coerceIn(0, 2000)
        save({ it.copy(newlinePause = v) }) { it.setNewlinePause(v) }
    }

    /** 0 whole numbers, 1 digit by digit. */
    override fun setNumberMode(mode: Int) {
        val v = mode.coerceIn(0, 1)
        save({ it.copy(numberMode = v) }) { it.setNumberMode(v) }
    }

    /** Spelling cadence 0-100; lower values add longer pauses. */
    override fun setSpellingSpeed(percent: Int) {
        val v = percent.coerceIn(SettingsRepository.SPELLING_SPEED_RANGE)
        save({ it.copy(spellingSpeed = v) }) { it.setSpellingSpeed(v) }
    }

    /** Letters by their sounds (true, phonetic spelling) or by their names. */
    override fun setLetterSounds(enabled: Boolean) =
        save({ it.copy(letterSounds = enabled) }) {
            it.setSpellingMode(
                if (enabled) SettingsRepository.SPELLING_MODE_SOUNDS else SettingsRepository.SPELLING_MODE_NAMES
            )
        }

    override fun setUserDictionariesEnabled(enabled: Boolean) =
        save({ it.copy(userDictionariesEnabled = enabled) }) { it.setUserDictionariesEnabled(enabled) }

    override fun clearError() {
        _uiState.update { it.copy(error = null) }
    }
}
