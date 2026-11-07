package com.hrvojekatic.laprdus.ui.screens

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.calculateEndPadding
import androidx.compose.foundation.layout.calculateStartPadding
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.ExperimentalComposeUiApi
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.platform.LocalLayoutDirection
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.LiveRegionMode
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.heading
import androidx.compose.ui.semantics.liveRegion
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.testTagsAsResourceId
import androidx.compose.ui.unit.dp
import com.hrvojekatic.laprdus.R
import com.hrvojekatic.laprdus.data.SettingsRepository
import com.hrvojekatic.laprdus.tts.Language
import com.hrvojekatic.laprdus.tts.VoiceInfo
import com.hrvojekatic.laprdus.ui.components.ChoiceDialog
import com.hrvojekatic.laprdus.ui.components.ChoiceOption
import com.hrvojekatic.laprdus.ui.components.ClickableRow
import com.hrvojekatic.laprdus.ui.components.ExpandRow
import com.hrvojekatic.laprdus.ui.components.SettingsSection
import com.hrvojekatic.laprdus.ui.components.SliderRow
import com.hrvojekatic.laprdus.ui.components.SwitchRow
import com.hrvojekatic.laprdus.ui.settings.SettingFormat
import com.hrvojekatic.laprdus.viewmodel.SettingsActions
import com.hrvojekatic.laprdus.viewmodel.SettingsUiState
import kotlin.math.roundToInt

private val FractionRange = 0f..1f
private val PercentRange = 0f..100f
private val PauseRange = 0f..2000f

/** Test tags, exposed as resource ids for UiAutomator and the macrobenchmarks. */
object SettingsTags {
    const val COLUMN = "settings_column"
    const val VOICE = "settings_voice"
    const val LANGUAGE = "settings_language"
    const val MORE_VOICE = "settings_more_voice"
}

/**
 * The Settings screen, after Android's settings guidelines: sections with
 * headings TalkBack can jump between, grouped rows, and the settings few
 * people change ("More voice settings") collapsed until asked for.
 *
 * 1. Voice: voice, language, rate, pitch, volume; more voice settings
 *    (acceleration, inflection, volume boost, head size, breathiness,
 *    roughness, brightness).
 * 2. Apps and screen readers: the four force switches.
 * 3. Reading: phonetic spelling, spelling speed, digits, emoji.
 * 4. Reading pauses. 5. Dictionaries. 6. About.
 *
 * @param systemLanguage the system language when Laprdus speaks it, else
 *   null: spoken unless the language is forced
 */
@OptIn(ExperimentalMaterial3Api::class, ExperimentalComposeUiApi::class)
@Composable
fun SettingsScreen(
    uiState: SettingsUiState,
    systemLanguage: String?,
    actions: SettingsActions,
    onNavigateBack: () -> Unit,
    onOpenDictionaries: () -> Unit,
    onOpenAbout: () -> Unit,
    initiallyExpanded: Boolean = false,
) {
    val snackbarHostState = remember { SnackbarHostState() }

    // A failed voice load and the like: announced (Snackbar is a live region).
    LaunchedEffect(uiState.error) {
        val message = uiState.error ?: return@LaunchedEffect
        snackbarHostState.showSnackbar(message)
        actions.clearError()
    }

    Scaffold(
        modifier = Modifier
            .fillMaxSize()
            .semantics { testTagsAsResourceId = true },
        topBar = {
            // A plain bar: a collapsing one draws its title twice and moves
            // while TalkBack speaks, which cut announcements short.
            TopAppBar(
                title = {
                    Text(
                        text = stringResource(R.string.settings_title),
                        modifier = Modifier.semantics { heading() },
                    )
                },
                navigationIcon = {
                    IconButton(onClick = onNavigateBack) {
                        Icon(
                            painter = painterResource(R.drawable.ic_arrow_back),
                            contentDescription = stringResource(R.string.cd_back_button),
                        )
                    }
                },
            )
        },
        snackbarHost = { SnackbarHost(snackbarHostState) },
    ) { padding ->
        if (uiState.isLoading) {
            val loading = stringResource(R.string.loading)
            Box(Modifier.fillMaxSize().padding(padding), contentAlignment = Alignment.Center) {
                CircularProgressIndicator(Modifier.semantics { contentDescription = loading })
            }
            return@Scaffold
        }

        val layoutDirection = LocalLayoutDirection.current
        Box(
            modifier = Modifier
                .fillMaxSize()
                .padding(
                    top = padding.calculateTopPadding(),
                    start = padding.calculateStartPadding(layoutDirection),
                    end = padding.calculateEndPadding(layoutDirection),
                ),
            contentAlignment = Alignment.TopCenter,
        ) {
            Column(
                modifier = Modifier
                    // Settings guidelines: a readable width on large screens.
                    .widthIn(max = 840.dp)
                    .fillMaxWidth()
                    .verticalScroll(rememberScrollState())
                    .testTag(SettingsTags.COLUMN)
                    .padding(horizontal = 16.dp)
                    // Scroll under the navigation bar, but let the last row clear it.
                    .padding(bottom = padding.calculateBottomPadding() + 24.dp),
            ) {
                uiState.storageError?.let { message ->
                    Text(
                        text = message,
                        color = MaterialTheme.colorScheme.error,
                        style = MaterialTheme.typography.bodyMedium,
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(top = 8.dp)
                            .semantics { liveRegion = LiveRegionMode.Polite },
                    )
                }
                VoiceSection(uiState, systemLanguage, actions, initiallyExpanded)
                AppsSection(uiState, actions)
                ReadingSection(uiState, actions)
                PausesSection(uiState, actions)
                SettingsSection(stringResource(R.string.category_dictionaries)) {
                    SwitchRow(
                        title = stringResource(R.string.setting_user_dict_enabled_title),
                        summary = stringResource(R.string.setting_user_dict_enabled_subtitle),
                        checked = uiState.userDictionariesEnabled,
                        onCheckedChange = actions::setUserDictionariesEnabled,
                    )
                    ClickableRow(
                        title = stringResource(R.string.setting_manage_dictionaries),
                        summary = stringResource(R.string.setting_manage_dictionaries_desc),
                        onClick = onOpenDictionaries,
                        showChevron = true,
                    )
                }
                SettingsSection(stringResource(R.string.category_about)) {
                    ClickableRow(
                        title = stringResource(R.string.setting_about),
                        summary = stringResource(R.string.setting_about_desc),
                        onClick = onOpenAbout,
                        showChevron = true,
                    )
                }
            }
        }
    }
}

@Composable
private fun VoiceSection(
    uiState: SettingsUiState,
    systemLanguage: String?,
    actions: SettingsActions,
    initiallyExpanded: Boolean,
) {
    val locale = LocalConfiguration.current.locales[0]
    var voiceDialog by remember { mutableStateOf(false) }
    var languageDialog by remember { mutableStateOf(false) }
    var moreExpanded by rememberSaveable { mutableStateOf(initiallyExpanded) }

    val selectedVoice = uiState.availableVoices.find { it.id == VoiceInfo.normalizeId(uiState.selectedVoiceId) }
    // With "Force language" off, the system language is spoken when Laprdus
    // speaks it: the row shows that language, disabled, and says why.
    val followedLanguage = if (uiState.forceLanguage) null else systemLanguage
    val shownLanguage = languageName(followedLanguage ?: uiState.language)
    val multiplier = remember(locale) { { value: Float -> SettingFormat.multiplier(value, locale) } }

    SettingsSection(stringResource(R.string.category_voice)) {
        ClickableRow(
            title = stringResource(R.string.voice_label),
            summary = selectedVoice?.displayName ?: stringResource(R.string.select_voice),
            onClick = { voiceDialog = true },
            modifier = Modifier.testTag(SettingsTags.VOICE),
        )
        ClickableRow(
            title = stringResource(R.string.language_label),
            summary = if (followedLanguage != null) {
                stringResource(R.string.language_follows_system, shownLanguage)
            } else {
                shownLanguage
            },
            enabled = followedLanguage == null,
            onClick = { languageDialog = true },
            modifier = Modifier.testTag(SettingsTags.LANGUAGE),
        )
        SliderRow(
            title = stringResource(R.string.setting_speech_rate),
            value = uiState.speed,
            onValueChange = actions::setSpeed,
            valueRange = uiState.speedPitchRange,
            step = SettingFormat.MULTIPLIER_STEP,
            valueText = multiplier,
            defaultValue = SettingsRepository.DEFAULT_SPEED,
        )
        SliderRow(
            title = stringResource(R.string.setting_speech_pitch),
            value = uiState.pitch,
            onValueChange = actions::setPitch,
            valueRange = uiState.speedPitchRange,
            step = SettingFormat.MULTIPLIER_STEP,
            valueText = multiplier,
            defaultValue = SettingsRepository.DEFAULT_PITCH,
        )
        val percentText = percentFormatter()
        SliderRow(
            title = stringResource(R.string.setting_speech_volume),
            value = uiState.volume,
            onValueChange = actions::setVolume,
            valueRange = FractionRange,
            step = SettingFormat.FRACTION_STEP,
            valueText = { percentText(SettingFormat.fractionToPercent(it)) },
            defaultValue = SettingsRepository.DEFAULT_VOLUME,
        )

        val hidden = listOf(
            stringResource(R.string.setting_acceleration),
            stringResource(R.string.setting_inflection_level),
            stringResource(R.string.setting_volume_boost_title),
            stringResource(R.string.setting_head_size),
            stringResource(R.string.setting_breathiness),
            stringResource(R.string.setting_roughness),
            stringResource(R.string.setting_brightness),
        )
        ExpandRow(
            title = stringResource(R.string.setting_more_voice),
            expanded = moreExpanded,
            onExpandedChange = { moreExpanded = it },
            collapsedSummary = hidden.joinToString(", "),
            modifier = Modifier.testTag(SettingsTags.MORE_VOICE),
        )
        if (moreExpanded) {
            val wpmText = { value: Float ->
                SettingFormat.topWordsPerMinute(
                    nominalWpm = uiState.nominalWpm,
                    speed = uiState.speed,
                    acceleration = value,
                    forceSpeed = uiState.forceSpeed,
                    speedRange = uiState.speedPitchRange,
                    formantVoice = uiState.isFormantVoice,
                )
            }
            val accelerationSpoken = accelerationFormatter(multiplier, wpmText)
            SliderRow(
                title = stringResource(R.string.setting_acceleration),
                summary = stringResource(R.string.setting_acceleration_desc),
                value = uiState.acceleration,
                onValueChange = actions::setAcceleration,
                valueRange = SettingsRepository.ACCELERATION_RANGE,
                step = SettingFormat.ACCELERATION_STEP,
                valueText = multiplier,
                spokenValue = accelerationSpoken,
                note = accelerationSpoken(uiState.acceleration),
                defaultValue = SettingsRepository.DEFAULT_ACCELERATION,
            )
            SliderRow(
                title = stringResource(R.string.setting_inflection_level),
                summary = stringResource(R.string.setting_inflection_level_desc),
                value = uiState.inflectionLevel,
                onValueChange = actions::setInflectionLevel,
                valueRange = FractionRange,
                step = SettingFormat.FRACTION_STEP,
                valueText = { percentText(SettingFormat.fractionToPercent(it)) },
                defaultValue = SettingsRepository.DEFAULT_INFLECTION_LEVEL,
            )
            SwitchRow(
                title = stringResource(R.string.setting_volume_boost_title),
                summary = stringResource(R.string.setting_volume_boost_subtitle),
                checked = uiState.volumeBoost,
                onCheckedChange = actions::setVolumeBoost,
            )
            VoiceQualitySlider(
                R.string.setting_head_size, R.string.setting_head_size_desc,
                uiState.headSize, SettingsRepository.DEFAULT_HEAD_SIZE, actions::setHeadSize, percentText,
            )
            VoiceQualitySlider(
                R.string.setting_breathiness, R.string.setting_breathiness_desc,
                uiState.breathiness, SettingsRepository.DEFAULT_BREATHINESS, actions::setBreathiness, percentText,
            )
            VoiceQualitySlider(
                R.string.setting_roughness, R.string.setting_roughness_desc,
                uiState.roughness, SettingsRepository.DEFAULT_ROUGHNESS, actions::setRoughness, percentText,
            )
            VoiceQualitySlider(
                R.string.setting_brightness, R.string.setting_brightness_desc,
                uiState.brightness, SettingsRepository.DEFAULT_BRIGHTNESS, actions::setBrightness, percentText,
            )
        }
    }

    if (voiceDialog) {
        val options = uiState.availableVoices.map { voice ->
            ChoiceOption(voice.id, voice.displayName, voiceDescription(voice))
        }
        ChoiceDialog(
            title = stringResource(R.string.voice_label),
            options = options,
            selectedId = selectedVoice?.id,
            onSelect = { id ->
                voiceDialog = false
                actions.selectVoice(id)
            },
            onDismiss = { voiceDialog = false },
        )
    }
    if (languageDialog) {
        val options = Language.ALL.map { ChoiceOption(it, languageName(it)) }
        ChoiceDialog(
            title = stringResource(R.string.language_label),
            options = options,
            selectedId = uiState.language,
            onSelect = { code ->
                languageDialog = false
                actions.selectLanguage(code)
            },
            onDismiss = { languageDialog = false },
        )
    }
}

@Composable
private fun AppsSection(uiState: SettingsUiState, actions: SettingsActions) {
    SettingsSection(stringResource(R.string.category_apps)) {
        SwitchRow(
            title = stringResource(R.string.setting_force_language_title),
            summary = stringResource(R.string.setting_force_language_subtitle),
            checked = uiState.forceLanguage,
            onCheckedChange = actions::setForceLanguage,
        )
        SwitchRow(
            title = stringResource(R.string.setting_force_speed_title),
            summary = stringResource(R.string.setting_force_speed_subtitle),
            checked = uiState.forceSpeed,
            onCheckedChange = actions::setForceSpeed,
        )
        SwitchRow(
            title = stringResource(R.string.setting_force_pitch_title),
            summary = stringResource(R.string.setting_force_pitch_subtitle),
            checked = uiState.forcePitch,
            onCheckedChange = actions::setForcePitch,
        )
        SwitchRow(
            title = stringResource(R.string.setting_force_volume_title),
            summary = stringResource(R.string.setting_force_volume_subtitle),
            checked = uiState.forceVolume,
            onCheckedChange = actions::setForceVolume,
        )
    }
}

@Composable
private fun ReadingSection(uiState: SettingsUiState, actions: SettingsActions) {
    val percentText = percentFormatter()
    SettingsSection(stringResource(R.string.category_reading)) {
        SwitchRow(
            // The title says what "on" does ("Sricanje" suggested the letter
            // names); no example letters, which Laprdus reads by this switch.
            title = stringResource(R.string.setting_spelling_mode),
            summary = stringResource(R.string.setting_spelling_mode_subtitle),
            checked = uiState.letterSounds,
            onCheckedChange = actions::setLetterSounds,
        )
        SliderRow(
            title = stringResource(R.string.setting_spelling_speed),
            summary = stringResource(R.string.setting_spelling_speed_desc),
            value = uiState.spellingSpeed.toFloat(),
            onValueChange = { actions.setSpellingSpeed(it.roundToInt()) },
            valueRange = PercentRange,
            step = SettingFormat.PERCENT_STEP,
            valueText = { percentText(it.roundToInt()) },
            defaultValue = SettingsRepository.DEFAULT_SPELLING_SPEED.toFloat(),
        )
        SwitchRow(
            title = stringResource(R.string.setting_number_mode_title),
            summary = stringResource(R.string.setting_number_mode_subtitle),
            checked = uiState.numberMode == 1,
            onCheckedChange = { actions.setNumberMode(if (it) 1 else 0) },
        )
        SwitchRow(
            title = stringResource(R.string.setting_emoji_enabled_title),
            summary = stringResource(R.string.setting_emoji_enabled_subtitle),
            checked = uiState.emojiEnabled,
            onCheckedChange = actions::setEmojiEnabled,
        )
    }
}

@Composable
private fun PausesSection(uiState: SettingsUiState, actions: SettingsActions) {
    val msText = millisecondsFormatter()
    SettingsSection(stringResource(R.string.category_pause_settings)) {
        PauseSlider(
            R.string.setting_sentence_pause, R.string.setting_sentence_pause_desc,
            uiState.sentencePause, SettingsRepository.DEFAULT_SENTENCE_PAUSE, actions::setSentencePause, msText,
        )
        PauseSlider(
            R.string.setting_comma_pause, R.string.setting_comma_pause_desc,
            uiState.commaPause, SettingsRepository.DEFAULT_COMMA_PAUSE, actions::setCommaPause, msText,
        )
        PauseSlider(
            R.string.setting_newline_pause, R.string.setting_newline_pause_desc,
            uiState.newlinePause, SettingsRepository.DEFAULT_NEWLINE_PAUSE, actions::setNewlinePause, msText,
        )
    }
}

@Composable
private fun VoiceQualitySlider(
    title: Int,
    summary: Int,
    value: Int,
    default: Int,
    onChange: (Int) -> Unit,
    percentText: (Int) -> String,
) {
    SliderRow(
        title = stringResource(title),
        summary = stringResource(summary),
        value = value.toFloat(),
        onValueChange = { onChange(it.roundToInt()) },
        valueRange = PercentRange,
        step = SettingFormat.PERCENT_STEP,
        valueText = { percentText(it.roundToInt()) },
        defaultValue = default.toFloat(),
    )
}

@Composable
private fun PauseSlider(
    title: Int,
    summary: Int,
    value: Int,
    default: Int,
    onChange: (Int) -> Unit,
    msText: (Int) -> String,
) {
    SliderRow(
        title = stringResource(title),
        summary = stringResource(summary),
        value = value.toFloat(),
        onValueChange = { onChange(it.roundToInt()) },
        valueRange = PauseRange,
        step = SettingFormat.PAUSE_STEP,
        valueText = { msText(it.roundToInt()) },
        defaultValue = default.toFloat(),
    )
}

/** "50 %" in Croatian and Slovenian, "50%" in English and Serbian (CLDR). */
@Composable
private fun percentFormatter(): (Int) -> String {
    val format = stringResource(R.string.format_percent)
    val locale = LocalConfiguration.current.locales[0]
    return remember(format, locale) { { value: Int -> String.format(locale, format, value) } }
}

@Composable
private fun millisecondsFormatter(): (Int) -> String {
    val format = stringResource(R.string.format_milliseconds)
    val locale = LocalConfiguration.current.locales[0]
    return remember(format, locale) { { value: Int -> String.format(locale, format, value) } }
}

/** The acceleration with the top rate it gives: "1,5x (do 525 riječi/min)". */
@Composable
private fun accelerationFormatter(
    multiplier: (Float) -> String,
    topWpm: (Float) -> Int,
): (Float) -> String {
    val format = stringResource(R.string.acceleration_value)
    val locale = LocalConfiguration.current.locales[0]
    return { value: Float -> String.format(locale, format, multiplier(value), topWpm(value)) }
}

/**
 * A voice's gender, age and kind. No language: every voice speaks every
 * language.
 */
@Composable
private fun voiceDescription(voice: VoiceInfo): String {
    val age = when (voice.age) {
        "Child" -> stringResource(R.string.age_child)
        "Adult" -> stringResource(R.string.age_adult)
        "Senior" -> stringResource(R.string.age_senior)
        else -> voice.age
    }
    val gender = if (voice.gender == "Male") {
        stringResource(R.string.gender_male)
    } else {
        stringResource(R.string.gender_female)
    }
    val kind = when {
        voice.isSingingVoice -> stringResource(R.string.voice_kind_singing)
        voice.isFormantVoice -> stringResource(R.string.voice_kind_formant)
        else -> stringResource(R.string.voice_kind_recorded)
    }
    return "$gender, $age, $kind"
}

/** The localized name of a language code ("hr", "sr", "bs", "sl"). */
@Composable
private fun languageName(language: String): String = when (language) {
    Language.SERBIAN -> stringResource(R.string.language_serbian)
    Language.BOSNIAN -> stringResource(R.string.language_bosnian)
    Language.SLOVENIAN -> stringResource(R.string.language_slovenian)
    else -> stringResource(R.string.language_croatian)
}
