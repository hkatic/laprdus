package com.hrvojekatic.laprdus.ui.screens

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.selection.toggleable
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.filled.Stop
import androidx.compose.material.icons.outlined.RecordVoiceOver
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Checkbox
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.LiveRegionMode
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.heading
import androidx.compose.ui.semantics.liveRegion
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.isTraversalGroup
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import com.hrvojekatic.laprdus.R
import com.hrvojekatic.laprdus.viewmodel.TTSUiState

/**
 * Main screen for the Laprdus TTS application.
 * Provides UI for text input and playback controls.
 * Voice selection and other settings are now in SettingsActivity.
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MainScreen(
    uiState: TTSUiState,
    onTextChange: (String) -> Unit,
    onSpeak: () -> Unit,
    onStop: () -> Unit,
    onOpenSettings: () -> Unit,
    onOpenTTSSettings: () -> Unit,
    onDontAskDefaultTtsChange: (Boolean) -> Unit = {},
    onDismissDefaultTtsDialog: () -> Unit = {},
    onConfirmSetDefaultTts: () -> Unit = {}
) {
    // Default TTS Engine Dialog
    if (uiState.showDefaultTtsDialog) {
        DefaultTtsDialog(
            dontAskAgainChecked = uiState.dontAskDefaultTtsChecked,
            onDontAskAgainChange = onDontAskDefaultTtsChange,
            onDismiss = onDismissDefaultTtsDialog,
            onOpenTtsSettings = {
                onConfirmSetDefaultTts()
                onOpenTTSSettings()
            }
        )
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = {
                    Text(
                        text = stringResource(R.string.app_name),
                        maxLines = 1,
                        overflow = TextOverflow.Ellipsis,
                        modifier = Modifier.semantics { heading() }
                    )
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = MaterialTheme.colorScheme.primaryContainer,
                    titleContentColor = MaterialTheme.colorScheme.onPrimaryContainer
                )
            )
        }
    ) { paddingValues ->
        if (uiState.isLoading) {
            // Loading state
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(paddingValues),
                contentAlignment = Alignment.Center
            ) {
                Column(
                    horizontalAlignment = Alignment.CenterHorizontally,
                    verticalArrangement = Arrangement.spacedBy(16.dp)
                ) {
                    CircularProgressIndicator()
                    Text(
                        text = stringResource(R.string.loading),
                        style = MaterialTheme.typography.bodyLarge
                    )
                }
            }
        } else {
            // Main content
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(paddingValues)
                    .padding(16.dp),
                verticalArrangement = Arrangement.spacedBy(16.dp)
            ) {
                // Text input area
                val textInputDescription = stringResource(R.string.cd_text_input)
                OutlinedTextField(
                    value = uiState.inputText,
                    onValueChange = onTextChange,
                    label = { Text(stringResource(R.string.text_input_label)) },
                    placeholder = { Text(stringResource(R.string.text_input_placeholder)) },
                    modifier = Modifier
                        .fillMaxWidth()
                        .weight(1f)
                        .semantics {
                            contentDescription = textInputDescription
                        },
                    enabled = !uiState.isPlaying,
                    minLines = 5
                )

                // Button section - three buttons stacked vertically
                Column(
                    verticalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    // Play/Stop button
                    if (uiState.isPlaying) {
                        val stopText = stringResource(R.string.stop_button)
                        val stopDesc = stringResource(R.string.cd_stop_desc)
                        AccessibleButton(
                            text = stopText,
                            description = stopDesc,
                            onClick = onStop,
                            containerColor = MaterialTheme.colorScheme.error,
                            contentColor = MaterialTheme.colorScheme.onError,
                            icon = Icons.Default.Stop
                        )
                    } else {
                        val speakText = stringResource(R.string.speak_button)
                        val speakDesc = stringResource(R.string.cd_speak_desc)
                        val isEnabled = uiState.isInitialized && uiState.inputText.isNotBlank()
                        AccessibleButton(
                            text = speakText,
                            description = speakDesc,
                            onClick = onSpeak,
                            enabled = isEnabled,
                            icon = Icons.Default.PlayArrow
                        )
                    }

                    // Laprdus Settings button
                    val laprdusSettingsText = stringResource(R.string.button_laprdus_settings)
                    val laprdusSettingsDesc = stringResource(R.string.cd_laprdus_settings_desc)
                    AccessibleOutlinedButton(
                        text = laprdusSettingsText,
                        description = laprdusSettingsDesc,
                        onClick = onOpenSettings,
                        icon = Icons.Default.Settings
                    )

                    // Android TTS Settings button
                    val ttsSettingsText = stringResource(R.string.button_android_tts_settings)
                    val ttsSettingsDesc = stringResource(R.string.cd_android_tts_settings_desc)
                    AccessibleOutlinedButton(
                        text = ttsSettingsText,
                        description = ttsSettingsDesc,
                        onClick = onOpenTTSSettings,
                        icon = Icons.Outlined.RecordVoiceOver
                    )
                }

                // Error display - uses liveRegion to announce errors to TalkBack
                uiState.error?.let { error ->
                    val errorDescription = stringResource(R.string.error_prefix, error)
                    Text(
                        text = error,
                        color = MaterialTheme.colorScheme.error,
                        style = MaterialTheme.typography.bodyMedium,
                        modifier = Modifier
                            .fillMaxWidth()
                            .semantics {
                                contentDescription = errorDescription
                                liveRegion = LiveRegionMode.Assertive
                            }
                    )
                }
            }
        }
    }
}

/**
 * Dialog prompting user to set Laprdus as default TTS engine.
 *
 * Accessibility:
 * - Dialog is automatically announced by TalkBack when it appears
 * - Uses AlertDialog which handles focus trapping
 * - Title uses heading semantics for TalkBack navigation
 * - Checkbox uses merged semantics with Role.Checkbox for proper announcement
 * - All text uses stringResource for localization
 */
@Composable
private fun DefaultTtsDialog(
    dontAskAgainChecked: Boolean,
    onDontAskAgainChange: (Boolean) -> Unit,
    onDismiss: () -> Unit,
    onOpenTtsSettings: () -> Unit
) {
    AlertDialog(
        onDismissRequest = onDismiss,
        title = {
            Text(
                text = stringResource(R.string.dialog_default_tts_title),
                modifier = Modifier.semantics { heading() }
            )
        },
        text = {
            Column {
                Text(
                    text = stringResource(R.string.dialog_default_tts_message),
                    modifier = Modifier.semantics {
                        liveRegion = LiveRegionMode.Polite
                    }
                )
                Spacer(modifier = Modifier.height(16.dp))
                // Don't ask again checkbox with merged accessibility semantics
                val checkboxDescription = stringResource(R.string.dialog_dont_ask_again)
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .semantics(mergeDescendants = true) {
                            isTraversalGroup = true
                            contentDescription = checkboxDescription
                        }
                        .toggleable(
                            value = dontAskAgainChecked,
                            onValueChange = onDontAskAgainChange,
                            role = Role.Checkbox
                        ),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Checkbox(
                        checked = dontAskAgainChecked,
                        onCheckedChange = null, // Handled by parent Row's toggleable
                        modifier = Modifier.clearAndSetSemantics { }
                    )
                    Spacer(modifier = Modifier.width(8.dp))
                    Text(
                        text = stringResource(R.string.dialog_dont_ask_again),
                        modifier = Modifier.clearAndSetSemantics { }
                    )
                }
            }
        },
        confirmButton = {
            Button(onClick = onOpenTtsSettings) {
                Text(stringResource(R.string.dialog_open_tts_settings))
            }
        },
        dismissButton = {
            TextButton(onClick = onDismiss) {
                Text(stringResource(R.string.dialog_not_now))
            }
        }
    )
}

/**
 * Accessible button with proper TalkBack announcement.
 * Uses Row wrapper with Role.Button for native role announcement.
 * TalkBack: "Text, Button, Double-tap to Description"
 */
@Composable
private fun AccessibleButton(
    text: String,
    description: String,
    onClick: () -> Unit,
    enabled: Boolean = true,
    containerColor: Color = MaterialTheme.colorScheme.primary,
    contentColor: Color = MaterialTheme.colorScheme.onPrimary,
    icon: ImageVector
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .height(64.dp)
            .semantics(mergeDescendants = true) {
                isTraversalGroup = true
                contentDescription = text
            }
            .clickable(
                enabled = enabled,
                role = Role.Button,
                onClickLabel = description,
                onClick = onClick
            )
    ) {
        Button(
            onClick = onClick,
            enabled = enabled,
            modifier = Modifier
                .fillMaxWidth()
                .height(64.dp)
                .clearAndSetSemantics { },
            colors = ButtonDefaults.buttonColors(
                containerColor = containerColor,
                contentColor = contentColor
            )
        ) {
            Icon(
                imageVector = icon,
                contentDescription = null
            )
            Spacer(Modifier.width(8.dp))
            Text(text)
        }
    }
}

/**
 * Accessible outlined button with proper TalkBack announcement.
 * Uses Row wrapper with Role.Button for native role announcement.
 * TalkBack: "Text, Button, Double-tap to Description"
 */
@Composable
private fun AccessibleOutlinedButton(
    text: String,
    description: String,
    onClick: () -> Unit,
    icon: ImageVector
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .height(64.dp)
            .semantics(mergeDescendants = true) {
                isTraversalGroup = true
                contentDescription = text
            }
            .clickable(
                role = Role.Button,
                onClickLabel = description,
                onClick = onClick
            )
    ) {
        OutlinedButton(
            onClick = onClick,
            modifier = Modifier
                .fillMaxWidth()
                .height(64.dp)
                .clearAndSetSemantics { }
        ) {
            Icon(
                imageVector = icon,
                contentDescription = null
            )
            Spacer(Modifier.width(8.dp))
            Text(text)
        }
    }
}
