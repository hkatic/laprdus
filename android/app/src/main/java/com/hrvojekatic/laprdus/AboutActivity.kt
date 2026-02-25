package com.hrvojekatic.laprdus

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import com.hrvojekatic.laprdus.ui.screens.AboutScreen
import com.hrvojekatic.laprdus.ui.theme.LaprdusTheme
import dagger.hilt.android.AndroidEntryPoint

/**
 * About activity for the Laprdus TTS application.
 * Shows application information, legal links, and support contact.
 */
@AndroidEntryPoint
class AboutActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        setContent {
            LaprdusTheme {
                AboutScreen(
                    onNavigateBack = { finish() }
                )
            }
        }
    }
}
