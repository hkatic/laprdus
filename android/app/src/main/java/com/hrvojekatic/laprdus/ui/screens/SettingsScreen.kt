package com.hrvojekatic.laprdus.ui.screens

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
import androidx.compose.foundation.clickable
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.selection.toggleable
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.ExposedDropdownMenuAnchorType
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Slider
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.heading
import androidx.compose.ui.semantics.isTraversalGroup
import androidx.compose.ui.semantics.LiveRegionMode
import androidx.compose.ui.semantics.liveRegion
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import android.content.Intent
import androidx.compose.material.icons.filled.ChevronRight
import com.hrvojekatic.laprdus.AboutActivity
import com.hrvojekatic.laprdus.DictionaryActivity
import com.hrvojekatic.laprdus.R
import com.hrvojekatic.laprdus.tts.VoiceInfo
import com.hrvojekatic.laprdus.viewmodel.SettingsUiState
import androidx.compose.ui.platform.LocalContext
import kotlin.math.roundToInt

/**
 * Settings screen for the Laprdus TTS application.
 * Provides UI for configuring voice, rate, pitch, volume, and advanced settings.
 *
 * Accessibility: Uses merged semantics so TalkBack announces each setting as a single
 * item with name, value, control type, and description in one swipe.
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SettingsScreen(
    uiState: SettingsUiState,
    onNavigateBack: () -> Unit,
    onVoiceSelected: (String) -> Unit,
    onSpeedChange: (Float) -> Unit,
    onPitchChange: (Float) -> Unit,
    onVolumeChange: (Float) -> Unit,
    onForceSpeedChange: (Boolean) -> Unit,
    onForcePitchChange: (Boolean) -> Unit,
    onForceVolumeChange: (Boolean) -> Unit,
    onForceLanguageChange: (Boolean) -> Unit,
    onRestoreDefaultSpeed: () -> Unit,
    onRestoreDefaultPitch: () -> Unit,
    onRestoreDefaultVolume: () -> Unit,
    // Advanced settings
    onEmojiEnabledChange: (Boolean) -> Unit,
    onInflectionLevelChange: (Float) -> Unit,
    onAccelerationChange: (Float) -> Unit,
    onSentencePauseChange: (Int) -> Unit,
    onCommaPauseChange: (Int) -> Unit,
    onNewlinePauseChange: (Int) -> Unit,
    onNumberModeChange: (Int) -> Unit,
    onSpellingSpeedChange: (Int) -> Unit,
    onLetterSoundsChange: (Boolean) -> Unit,
    // Dictionary settings
    onUserDictionariesEnabledChange: (Boolean) -> Unit
) {
    val context = LocalContext.current
    val backButtonDescription = stringResource(R.string.cd_back_button)

    Scaffold(
        topBar = {
            TopAppBar(
                title = {
                    Text(
                        text = stringResource(R.string.settings_title),
                        modifier = Modifier.semantics { heading() }
                    )
                },
                navigationIcon = {
                    IconButton(
                        onClick = onNavigateBack,
                        modifier = Modifier.semantics {
                            contentDescription = backButtonDescription
                        }
                    ) {
                        Icon(
                            imageVector = Icons.AutoMirrored.Filled.ArrowBack,
                            contentDescription = null
                        )
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = MaterialTheme.colorScheme.primaryContainer,
                    titleContentColor = MaterialTheme.colorScheme.onPrimaryContainer
                )
            )
        }
    ) { paddingValues ->
        if (uiState.isLoading) {
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
            LazyColumn(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(paddingValues)
                    .padding(horizontal = 16.dp),
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                // Storage problem reported by the repository (e.g. settings from an
                // older version could not be moved to device-protected storage)
                uiState.storageError?.let { errorMessage ->
                    item {
                        Text(
                            text = errorMessage,
                            color = MaterialTheme.colorScheme.error,
                            style = MaterialTheme.typography.bodyMedium,
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(top = 8.dp)
                                .semantics { liveRegion = LiveRegionMode.Polite }
                        )
                    }
                }

                // Voice Category
                item {
                    Spacer(modifier = Modifier.height(8.dp))
                    SettingsCategoryHeader(title = stringResource(R.string.category_voice))
                }

                // Voice Selector
                item {
                    VoiceSettingItem(
                        voices = uiState.availableVoices,
                        selectedVoiceId = uiState.selectedVoiceId,
                        onVoiceSelected = onVoiceSelected
                    )
                }

                // Each slider is followed by its restore button and, where there
                // is one, the switch that forces the value on every app: speed and
                // acceleration, then pitch and inflection, then volume.
                // Speech Rate Slider (0.5x-2.0x; the formant voices reach 0.25x-4.0x)
                item {
                    SliderSettingItem(
                        title = stringResource(R.string.setting_speech_rate),
                        value = uiState.speed,
                        valueRange = uiState.speedPitchRange,
                        onValueChange = onSpeedChange,
                        valueLabel = formatMultiplier(uiState.speed),
                        description = stringResource(R.string.cd_slider_adjust)
                    )
                }

                item {
                    RestoreDefaultButton(
                        text = stringResource(R.string.restore_default_speed),
                        description = stringResource(R.string.cd_restore_speed_desc),
                        onClick = onRestoreDefaultSpeed
                    )
                }

                // Force Speed Toggle
                item {
                    SwitchSettingItem(
                        title = stringResource(R.string.setting_force_speed_title),
                        subtitle = stringResource(R.string.setting_force_speed_subtitle),
                        checked = uiState.forceSpeed,
                        onCheckedChange = onForceSpeedChange
                    )
                }

                item {
                    // The rate the top of the rate range (2.0x) reaches
                    // with this acceleration, in words per minute.
                    val topWpm = (uiState.nominalWpm * 2.0f * uiState.acceleration).roundToInt()
                    SliderSettingItem(
                        title = stringResource(R.string.setting_acceleration),
                        value = uiState.acceleration,
                        valueRange = 0.5f..3.0f,
                        onValueChange = onAccelerationChange,
                        valueLabel = stringResource(R.string.acceleration_value, uiState.acceleration, topWpm),
                        description = stringResource(R.string.setting_acceleration_desc)
                    )
                }

                item {
                    RestoreDefaultButton(
                        text = stringResource(R.string.restore_default_acceleration),
                        description = stringResource(R.string.cd_restore_acceleration_desc),
                        onClick = { onAccelerationChange(1.0f) }
                    )
                }

                // Speech Pitch Slider
                item {
                    SliderSettingItem(
                        title = stringResource(R.string.setting_speech_pitch),
                        value = uiState.pitch,
                        valueRange = uiState.speedPitchRange,
                        onValueChange = onPitchChange,
                        valueLabel = formatMultiplier(uiState.pitch),
                        description = stringResource(R.string.cd_slider_adjust)
                    )
                }

                item {
                    RestoreDefaultButton(
                        text = stringResource(R.string.restore_default_pitch),
                        description = stringResource(R.string.cd_restore_pitch_desc),
                        onClick = onRestoreDefaultPitch
                    )
                }

                // Force Pitch Toggle
                item {
                    SwitchSettingItem(
                        title = stringResource(R.string.setting_force_pitch_title),
                        subtitle = stringResource(R.string.setting_force_pitch_subtitle),
                        checked = uiState.forcePitch,
                        onCheckedChange = onForcePitchChange
                    )
                }

                // Inflection level (every voice)
                item {
                    SliderSettingItem(
                        title = stringResource(R.string.setting_inflection_level),
                        value = uiState.inflectionLevel,
                        valueRange = 0.0f..1.0f,
                        onValueChange = onInflectionLevelChange,
                        valueLabel = "${(uiState.inflectionLevel * 100).roundToInt()}%",
                        description = stringResource(R.string.setting_inflection_level_desc)
                    )
                }

                item {
                    RestoreDefaultButton(
                        text = stringResource(R.string.restore_default_inflection_level),
                        description = stringResource(R.string.cd_restore_inflection_level_desc),
                        onClick = { onInflectionLevelChange(0.5f) }
                    )
                }

                // Speech Volume Slider
                item {
                    SliderSettingItem(
                        title = stringResource(R.string.setting_speech_volume),
                        value = uiState.volume,
                        valueRange = 0.0f..1.0f,
                        onValueChange = onVolumeChange,
                        valueLabel = "${(uiState.volume * 100).toInt()}%",
                        description = stringResource(R.string.cd_slider_adjust)
                    )
                }

                item {
                    RestoreDefaultButton(
                        text = stringResource(R.string.restore_default_volume),
                        description = stringResource(R.string.cd_restore_volume_desc),
                        onClick = onRestoreDefaultVolume
                    )
                }

                // Force Volume Toggle
                item {
                    SwitchSettingItem(
                        title = stringResource(R.string.setting_force_volume_title),
                        subtitle = stringResource(R.string.setting_force_volume_subtitle),
                        checked = uiState.forceVolume,
                        onCheckedChange = onForceVolumeChange
                    )
                }

                // Advanced Category
                item {
                    Spacer(modifier = Modifier.height(16.dp))
                    SettingsCategoryHeader(title = stringResource(R.string.category_advanced))
                }

                // Force Language Toggle
                item {
                    SwitchSettingItem(
                        title = stringResource(R.string.setting_force_language_title),
                        subtitle = stringResource(R.string.setting_force_language_subtitle),
                        checked = uiState.forceLanguage,
                        onCheckedChange = onForceLanguageChange
                    )
                }

                // Emoji Enable Toggle
                item {
                    SwitchSettingItem(
                        title = stringResource(R.string.setting_emoji_enabled_title),
                        subtitle = stringResource(R.string.setting_emoji_enabled_subtitle),
                        checked = uiState.emojiEnabled,
                        onCheckedChange = onEmojiEnabledChange
                    )
                }

                // Number Mode Toggle
                item {
                    SwitchSettingItem(
                        title = stringResource(R.string.setting_number_mode_title),
                        subtitle = stringResource(R.string.setting_number_mode_subtitle),
                        checked = uiState.numberMode == 1,
                        onCheckedChange = { checked ->
                            onNumberModeChange(if (checked) 1 else 0)
                        }
                    )
                }

                // Spelling: letter names or letter sounds
                item {
                    SwitchSettingItem(
                        title = stringResource(R.string.setting_spelling_mode),
                        subtitle = stringResource(R.string.setting_spelling_mode_subtitle),
                        checked = uiState.letterSounds,
                        onCheckedChange = onLetterSoundsChange
                    )
                }

                // Pause Settings Category
                item {
                    Spacer(modifier = Modifier.height(16.dp))
                    SettingsCategoryHeader(title = stringResource(R.string.category_pause_settings))
                }

                // Sentence Pause Slider
                item {
                    SliderSettingItem(
                        title = stringResource(R.string.setting_sentence_pause),
                        value = uiState.sentencePause.toFloat(),
                        valueRange = 0f..2000f,
                        onValueChange = { onSentencePauseChange(it.toInt()) },
                        valueLabel = "${uiState.sentencePause} ms",
                        description = stringResource(R.string.setting_sentence_pause_desc)
                    )
                }

                // Comma Pause Slider
                item {
                    SliderSettingItem(
                        title = stringResource(R.string.setting_comma_pause),
                        value = uiState.commaPause.toFloat(),
                        valueRange = 0f..2000f,
                        onValueChange = { onCommaPauseChange(it.toInt()) },
                        valueLabel = "${uiState.commaPause} ms",
                        description = stringResource(R.string.setting_comma_pause_desc)
                    )
                }

                // Newline Pause Slider
                item {
                    SliderSettingItem(
                        title = stringResource(R.string.setting_newline_pause),
                        value = uiState.newlinePause.toFloat(),
                        valueRange = 0f..2000f,
                        onValueChange = { onNewlinePauseChange(it.toInt()) },
                        valueLabel = "${uiState.newlinePause} ms",
                        description = stringResource(R.string.setting_newline_pause_desc)
                    )
                }

                // Spelling Speed Slider (percent of the speech rate)
                item {
                    SliderSettingItem(
                        title = stringResource(R.string.setting_spelling_speed),
                        value = uiState.spellingSpeed.toFloat(),
                        valueRange = 0f..100f,
                        onValueChange = { onSpellingSpeedChange(it.roundToInt()) },
                        valueLabel = "${uiState.spellingSpeed}%",
                        description = stringResource(R.string.setting_spelling_speed_desc)
                    )
                }

                item {
                    RestoreDefaultButton(
                        text = stringResource(R.string.restore_default_spelling_speed),
                        description = stringResource(R.string.cd_restore_spelling_speed_desc),
                        onClick = { onSpellingSpeedChange(50) }
                    )
                }

                // Dictionary Category
                item {
                    Spacer(modifier = Modifier.height(16.dp))
                    SettingsCategoryHeader(title = stringResource(R.string.category_dictionaries))
                }

                // User Dictionaries Enabled Toggle
                item {
                    SwitchSettingItem(
                        title = stringResource(R.string.setting_user_dict_enabled_title),
                        subtitle = stringResource(R.string.setting_user_dict_enabled_subtitle),
                        checked = uiState.userDictionariesEnabled,
                        onCheckedChange = onUserDictionariesEnabledChange
                    )
                }

                // Manage Dictionaries Button
                item {
                    NavigationSettingItem(
                        title = stringResource(R.string.setting_manage_dictionaries),
                        subtitle = stringResource(R.string.setting_manage_dictionaries_desc),
                        onClick = {
                            context.startActivity(Intent(context, DictionaryActivity::class.java))
                        }
                    )
                }

                // About Category
                item {
                    Spacer(modifier = Modifier.height(16.dp))
                    SettingsCategoryHeader(title = stringResource(R.string.category_about))
                }

                // About Laprdus Button
                item {
                    NavigationSettingItem(
                        title = stringResource(R.string.setting_about),
                        subtitle = stringResource(R.string.setting_about_desc),
                        onClick = {
                            context.startActivity(Intent(context, AboutActivity::class.java))
                        }
                    )
                }

                // Bottom padding
                item {
                    Spacer(modifier = Modifier.height(24.dp))
                }
            }
        }
    }
}

/**
 * Category header for settings sections.
 * Marked as heading for TalkBack navigation with swipe up/down to headings.
 */
/** "1.5x", or "0.25x" where one decimal would round the value away. */
private fun formatMultiplier(value: Float): String {
    val hundredths = (value * 100).roundToInt()
    return if (hundredths % 10 == 0) "%.1fx".format(value) else "%.2fx".format(value)
}

@Composable
fun SettingsCategoryHeader(title: String) {
    Text(
        text = title,
        style = MaterialTheme.typography.titleMedium,
        fontWeight = FontWeight.Bold,
        color = MaterialTheme.colorScheme.primary,
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 8.dp)
            .semantics { heading() }
    )
}

/**
 * Voice selector dropdown for settings.
 *
 * Accessibility for TalkBack:
 * - Same pattern as SwitchSettingItem: Row wrapper with clickable + stateDescription
 * - Uses isTraversalGroup to ensure proper navigation order
 * - Announcement: "Label. Dropdown. Selected value"
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun VoiceSettingItem(
    voices: List<VoiceInfo>,
    selectedVoiceId: String,
    onVoiceSelected: (String) -> Unit
) {
    var expanded by remember { mutableStateOf(false) }
    val selectedVoice = voices.find { it.id == selectedVoiceId }
    val voiceSelectorLabel = stringResource(R.string.voice_label)
    val selectVoiceText = stringResource(R.string.select_voice)
    val selectedVoiceName = selectedVoice?.localizedDisplayName ?: selectVoiceText
    val dropdownDesc = stringResource(R.string.cd_voice_dropdown_desc)

    // Row wrapper with Role.DropdownList for native TalkBack role announcement
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 8.dp)
            .semantics(mergeDescendants = true) {
                isTraversalGroup = true
                contentDescription = voiceSelectorLabel
                stateDescription = selectedVoiceName
            }
            .clickable(
                role = Role.DropdownList,
                onClickLabel = dropdownDesc
            ) { expanded = true }
    ) {
        ExposedDropdownMenuBox(
            expanded = expanded,
            onExpandedChange = { expanded = it },
            modifier = Modifier.clearAndSetSemantics { }
        ) {
            OutlinedTextField(
                value = selectedVoiceName,
                onValueChange = {},
                readOnly = true,
                label = {
                    Text(
                        text = voiceSelectorLabel,
                        modifier = Modifier.clearAndSetSemantics { }
                    )
                },
                trailingIcon = {
                    ExposedDropdownMenuDefaults.TrailingIcon(expanded = expanded)
                },
                colors = ExposedDropdownMenuDefaults.outlinedTextFieldColors(),
                modifier = Modifier
                    .fillMaxWidth()
                    .menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
                    .clearAndSetSemantics { }
            )

            ExposedDropdownMenu(
                expanded = expanded,
                onDismissRequest = { expanded = false }
            ) {
                voices.forEach { voice ->
                    // Capture composable values before entering non-composable lambda
                    val voiceDescription = getVoiceDescription(voice)
                    val itemContentDescription = "${voice.localizedDisplayName}, $voiceDescription"

                    DropdownMenuItem(
                        text = {
                            Column {
                                Text(
                                    text = voice.localizedDisplayName,
                                    style = MaterialTheme.typography.bodyLarge
                                )
                                Text(
                                    text = voiceDescription,
                                    style = MaterialTheme.typography.bodySmall,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant
                                )
                            }
                        },
                        onClick = {
                            onVoiceSelected(voice.id)
                            expanded = false
                        },
                        modifier = Modifier.semantics {
                            contentDescription = itemContentDescription
                        }
                    )
                }
            }
        }
    }
}

/**
 * Get a user-friendly description for a voice.
 */
@Composable
private fun getVoiceDescription(voice: VoiceInfo): String {
    val language = when {
        voice.isCroatian -> stringResource(R.string.language_croatian)
        voice.isBosnian -> stringResource(R.string.language_bosnian)
        else -> stringResource(R.string.language_serbian)
    }
    val ageDesc = when (voice.age) {
        "Child" -> stringResource(R.string.age_child)
        "Adult" -> stringResource(R.string.age_adult)
        "Senior" -> stringResource(R.string.age_senior)
        else -> voice.age
    }
    val genderDesc = if (voice.gender == "Male") {
        stringResource(R.string.gender_male)
    } else {
        stringResource(R.string.gender_female)
    }
    return "$language - $genderDesc, $ageDesc"
}

/**
 * Slider setting item for rate, pitch, and volume.
 *
 * Accessibility for TalkBack:
 * - Visual text row is hidden; Slider keeps native SeekBar accessibility
 * - mergeDescendants on Slider merges internal thumb/track into one node
 * - contentDescription provides the label, stateDescription the formatted value
 * - Announcement: "Title, Slider, Value, Use volume keys to adjust"
 */
@Composable
fun SliderSettingItem(
    title: String,
    value: Float,
    valueRange: ClosedFloatingPointRange<Float>,
    onValueChange: (Float) -> Unit,
    valueLabel: String,
    description: String = ""
) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 8.dp)
    ) {
        // Visual text row - hidden from TalkBack
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .clearAndSetSemantics {},
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                text = title,
                style = MaterialTheme.typography.bodyLarge
            )
            Text(
                text = valueLabel,
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.primary
            )
        }
        // Slider keeps its native SeekBar accessibility (className, role, range, setProgress).
        // mergeDescendants merges internal thumb/track into one node.
        // contentDescription provides the label, stateDescription overrides the default
        // percentage with our formatted value (e.g. "1.0x", "500 ms").
        Slider(
            value = value,
            onValueChange = onValueChange,
            valueRange = valueRange,
            modifier = Modifier.semantics(mergeDescendants = true) {
                isTraversalGroup = true
                contentDescription = title
                stateDescription = valueLabel
            }
        )
    }
}

/**
 * Switch setting item for toggles.
 *
 * Accessibility for TalkBack:
 * - Uses toggleable(role = Role.Switch) for native role announcement
 * - Uses isTraversalGroup to ensure proper navigation order
 * - TalkBack announces: "Title. Subtitle, Switch, On/Off, Double-tap to toggle"
 */
@Composable
fun SwitchSettingItem(
    title: String,
    subtitle: String,
    checked: Boolean,
    onCheckedChange: (Boolean) -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 12.dp)
            .semantics(mergeDescendants = true) {
                isTraversalGroup = true
                contentDescription = "$title. $subtitle"
            }
            .toggleable(
                value = checked,
                onValueChange = onCheckedChange,
                role = Role.Switch
            ),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = title,
                style = MaterialTheme.typography.bodyLarge,
                modifier = Modifier.clearAndSetSemantics { }
            )
            Text(
                text = subtitle,
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
                modifier = Modifier.clearAndSetSemantics { }
            )
        }
        Spacer(modifier = Modifier.width(16.dp))
        Switch(
            checked = checked,
            onCheckedChange = null, // Handled by parent Row's clickable
            modifier = Modifier.clearAndSetSemantics { }
        )
    }
}

/**
 * Button to restore default values.
 *
 * Accessibility for TalkBack:
 * - Uses Role.Button for native role announcement
 * - Uses isTraversalGroup to ensure proper navigation order
 * - TalkBack: "Text, Button, Double-tap to Description"
 */
@Composable
fun RestoreDefaultButton(
    text: String,
    description: String,
    onClick: () -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 4.dp)
            .semantics(mergeDescendants = true) {
                isTraversalGroup = true
                contentDescription = text
            }
            .clickable(
                role = Role.Button,
                onClickLabel = description,
                onClick = onClick
            ),
        horizontalArrangement = Arrangement.Center
    ) {
        // Visual button appearance without its own semantics
        Button(
            onClick = onClick,
            modifier = Modifier
                .fillMaxWidth()
                .clearAndSetSemantics { },
            colors = ButtonDefaults.buttonColors(
                containerColor = MaterialTheme.colorScheme.secondaryContainer,
                contentColor = MaterialTheme.colorScheme.onSecondaryContainer
            )
        ) {
            Text(text = text)
        }
    }
}

/**
 * Navigation setting item that opens another screen.
 *
 * Accessibility for TalkBack:
 * - Uses Role.Button for native role announcement
 * - Uses isTraversalGroup to ensure proper navigation order
 * - TalkBack: "Title, Button, Double-tap to Subtitle"
 */
@Composable
fun NavigationSettingItem(
    title: String,
    subtitle: String,
    onClick: () -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 12.dp)
            .semantics(mergeDescendants = true) {
                isTraversalGroup = true
                contentDescription = title
                stateDescription = subtitle
            }
            .clickable(
                role = Role.Button,
                onClickLabel = subtitle,
                onClick = onClick
            ),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = title,
                style = MaterialTheme.typography.bodyLarge,
                modifier = Modifier.clearAndSetSemantics { }
            )
            Text(
                text = subtitle,
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
                modifier = Modifier.clearAndSetSemantics { }
            )
        }
        Icon(
            imageVector = Icons.Default.ChevronRight,
            contentDescription = null,
            tint = MaterialTheme.colorScheme.onSurfaceVariant,
            modifier = Modifier.clearAndSetSemantics { }
        )
    }
}
