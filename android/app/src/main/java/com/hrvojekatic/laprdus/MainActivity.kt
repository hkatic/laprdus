package com.hrvojekatic.laprdus

import android.content.Intent
import android.os.Bundle
import android.provider.Settings
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.compose.LocalLifecycleOwner
import androidx.lifecycle.repeatOnLifecycle
import com.hrvojekatic.laprdus.ui.screens.MainScreen
import com.hrvojekatic.laprdus.ui.theme.LaprdusTheme
import com.hrvojekatic.laprdus.viewmodel.TTSViewModel
import dagger.hilt.android.AndroidEntryPoint

/**
 * Main activity for the Laprdus TTS application.
 * Provides a UI for testing the TTS engine with text input and playback controls.
 * Settings are now in a separate SettingsActivity.
 */
@AndroidEntryPoint
class MainActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        setContent {
            LaprdusTheme {
                val viewModel: TTSViewModel = hiltViewModel()
                val uiState by viewModel.uiState.collectAsState()
                val lifecycleOwner = LocalLifecycleOwner.current

                // Check if Laprdus is the default TTS engine on resume
                LaunchedEffect(lifecycleOwner) {
                    lifecycleOwner.repeatOnLifecycle(Lifecycle.State.RESUMED) {
                        viewModel.checkDefaultTtsEngine()
                    }
                }

                MainScreen(
                    uiState = uiState,
                    onTextChange = viewModel::updateInputText,
                    onSpeak = viewModel::speak,
                    onStop = viewModel::stop,
                    onOpenSettings = { openLaprdusSettings() },
                    onOpenTTSSettings = { openSystemTTSSettings() },
                    onDontAskDefaultTtsChange = viewModel::toggleDontAskDefaultTts,
                    onDismissDefaultTtsDialog = viewModel::dismissDefaultTtsDialog,
                    onConfirmSetDefaultTts = viewModel::confirmSetDefaultTts
                )
            }
        }
    }

    /**
     * Open the Laprdus settings activity for voice, rate, pitch, and volume configuration.
     */
    private fun openLaprdusSettings() {
        val intent = Intent(this, SettingsActivity::class.java)
        startActivity(intent)
    }

    /**
     * Open the system TTS settings where users can select Laprdus as their preferred engine.
     */
    private fun openSystemTTSSettings() {
        try {
            // Try to open TTS settings directly
            val intent = Intent().apply {
                action = "com.android.settings.TTS_SETTINGS"
            }
            startActivity(intent)
        } catch (e: Exception) {
            // Fallback to accessibility settings
            try {
                val intent = Intent(Settings.ACTION_ACCESSIBILITY_SETTINGS)
                startActivity(intent)
            } catch (e2: Exception) {
                // Final fallback to main settings
                val intent = Intent(Settings.ACTION_SETTINGS)
                startActivity(intent)
            }
        }
    }
}
