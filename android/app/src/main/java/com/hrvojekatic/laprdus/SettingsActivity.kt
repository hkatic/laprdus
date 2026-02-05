package com.hrvojekatic.laprdus

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.hilt.navigation.compose.hiltViewModel
import com.hrvojekatic.laprdus.ui.screens.SettingsScreen
import com.hrvojekatic.laprdus.ui.theme.LaprdusTheme
import com.hrvojekatic.laprdus.viewmodel.SettingsViewModel
import dagger.hilt.android.AndroidEntryPoint

/**
 * Settings activity for the Laprdus TTS application.
 * This activity is launched from:
 * - The main app when user taps "Laprdus Settings" button
 * - Android TTS settings when user taps the settings icon for Laprdus engine
 *
 * When launched from Android TTS settings, pressing Back will return
 * to the TTS settings screen.
 */
@AndroidEntryPoint
class SettingsActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        setContent {
            LaprdusTheme {
                val viewModel: SettingsViewModel = hiltViewModel()
                val uiState by viewModel.uiState.collectAsState()

                SettingsScreen(
                    uiState = uiState,
                    onNavigateBack = { finish() },
                    onVoiceSelected = viewModel::selectVoice,
                    onSpeedChange = viewModel::setSpeed,
                    onPitchChange = viewModel::setPitch,
                    onVolumeChange = viewModel::setVolume,
                    onForceSpeedChange = viewModel::setForceSpeed,
                    onForcePitchChange = viewModel::setForcePitch,
                    onForceVolumeChange = viewModel::setForceVolume,
                    onForceLanguageChange = viewModel::setForceLanguage,
                    onRestoreDefaultSpeed = viewModel::restoreDefaultSpeed,
                    onRestoreDefaultPitch = viewModel::restoreDefaultPitch,
                    onRestoreDefaultVolume = viewModel::restoreDefaultVolume,
                    // Advanced settings
                    onEmojiEnabledChange = viewModel::setEmojiEnabled,
                    onInflectionEnabledChange = viewModel::setInflectionEnabled,
                    onSentencePauseChange = viewModel::setSentencePause,
                    onCommaPauseChange = viewModel::setCommaPause,
                    onNewlinePauseChange = viewModel::setNewlinePause,
                    onNumberModeChange = viewModel::setNumberMode,
                    // Dictionary settings
                    onUserDictionariesEnabledChange = viewModel::setUserDictionariesEnabled
                )
            }
        }
    }
}
