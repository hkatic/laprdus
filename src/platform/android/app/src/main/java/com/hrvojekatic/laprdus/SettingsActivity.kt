package com.hrvojekatic.laprdus

import android.content.Intent
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.platform.LocalConfiguration
import androidx.hilt.lifecycle.viewmodel.compose.hiltViewModel
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.hrvojekatic.laprdus.tts.Language
import com.hrvojekatic.laprdus.tts.SystemLanguage
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
        enableEdgeToEdge()
        super.onCreate(savedInstanceState)

        setContent {
            LaprdusTheme {
                val viewModel: SettingsViewModel = hiltViewModel()
                val uiState by viewModel.uiState.collectAsStateWithLifecycle()
                // The system language (a system service call): read again only
                // when the configuration changes, not on every recomposition.
                val configuration = LocalConfiguration.current
                val systemLanguage = remember(configuration) {
                    Language.deviceLanguage(SystemLanguage.locale(this@SettingsActivity))
                }

                SettingsScreen(
                    uiState = uiState,
                    systemLanguage = systemLanguage,
                    actions = viewModel,
                    onNavigateBack = { finish() },
                    onOpenDictionaries = {
                        startActivity(Intent(this@SettingsActivity, DictionaryActivity::class.java))
                    },
                    onOpenAbout = {
                        startActivity(Intent(this@SettingsActivity, AboutActivity::class.java))
                    },
                )
            }
        }
    }
}
