package com.hrvojekatic.laprdus.ui.screens

import androidx.activity.ComponentActivity
import androidx.annotation.StringRes
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.SemanticsActions
import androidx.compose.ui.semantics.SemanticsNode
import androidx.compose.ui.semantics.SemanticsProperties
import androidx.compose.ui.semantics.getOrNull
import androidx.compose.ui.state.ToggleableState
import androidx.compose.ui.test.SemanticsMatcher
import androidx.compose.ui.test.assert
import androidx.compose.ui.test.assertCountEquals
import androidx.compose.ui.test.assertIsEnabled
import androidx.compose.ui.test.assertIsNotEnabled
import androidx.compose.ui.test.assertIsNotSelected
import androidx.compose.ui.test.assertIsSelected
import androidx.compose.ui.test.hasContentDescription
import androidx.compose.ui.test.hasText
import androidx.compose.ui.test.isDialog
import androidx.compose.ui.test.isHeading
import androidx.compose.ui.test.isSelectable
import androidx.compose.ui.test.junit4.accessibility.enableAccessibilityChecks
import androidx.compose.ui.test.junit4.v2.createAndroidComposeRule
import androidx.compose.ui.test.onRoot
import androidx.compose.ui.test.performClick
import androidx.compose.ui.test.performScrollTo
import androidx.compose.ui.test.performSemanticsAction
import androidx.compose.ui.test.tryPerformAccessibilityChecks
import androidx.test.ext.junit.runners.AndroidJUnit4
import com.hrvojekatic.laprdus.R
import com.hrvojekatic.laprdus.tts.VoiceInfo
import com.hrvojekatic.laprdus.ui.settings.SettingFormat
import com.hrvojekatic.laprdus.ui.theme.LaprdusTheme
import com.hrvojekatic.laprdus.viewmodel.SettingsActions
import com.hrvojekatic.laprdus.viewmodel.SettingsUiState
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

/**
 * What the Settings screen exposes to TalkBack: one node per setting whose
 * texts read in the order they are shown (title, then summary), the right
 * role and state, headings for the sections, the restore-default action of
 * each slider, and the collapsed "More voice settings".
 *
 * The strings come from the app's resources, so the test passes in any
 * device language. [com.hrvojekatic.laprdus.ui.TalkBackSpeechTest] checks
 * what TalkBack actually says on a phone.
 */
@RunWith(AndroidJUnit4::class)
class SettingsScreenAccessibilityTest {

    @get:Rule
    val rule = createAndroidComposeRule<ComponentActivity>()

    private fun s(@StringRes id: Int, vararg args: Any): String =
        if (args.isEmpty()) rule.activity.getString(id) else rule.activity.getString(id, *args)

    private val locale get() = rule.activity.resources.configuration.locales[0]

    private val voices = listOf(
        VoiceInfo("zvonko", "Zvonko", "hr-HR", "Male", "Adult", 1.0f, isFormant = true),
        VoiceInfo("josip", "Josip", "hr-HR", "Male", "Adult", 1.0f),
        VoiceInfo("vlado", "Vlado", "sr-RS", "Male", "Adult", 1.0f),
    )

    private val state = SettingsUiState(
        isLoading = false,
        availableVoices = voices,
        selectedVoiceId = "zvonko",
        language = "hr",
        speed = 1.5f,
        letterSounds = true,
    )

    /** Records what the screen asked for. */
    private class RecordingActions : SettingsActions {
        val calls = mutableListOf<String>()
        override fun selectVoice(voiceId: String) { calls += "voice $voiceId" }
        override fun selectLanguage(language: String) { calls += "language $language" }
        override fun setSpeed(speed: Float) { calls += "speed $speed" }
        override fun setPitch(pitch: Float) { calls += "pitch $pitch" }
        override fun setVolume(volume: Float) { calls += "volume $volume" }
        override fun setVolumeBoost(enabled: Boolean) { calls += "boost $enabled" }
        override fun setForceSpeed(enabled: Boolean) { calls += "forceSpeed $enabled" }
        override fun setForcePitch(enabled: Boolean) { calls += "forcePitch $enabled" }
        override fun setForceVolume(enabled: Boolean) { calls += "forceVolume $enabled" }
        override fun setForceLanguage(enabled: Boolean) { calls += "forceLanguage $enabled" }
        override fun setEmojiEnabled(enabled: Boolean) { calls += "emoji $enabled" }
        override fun setInflectionLevel(level: Float) { calls += "inflection $level" }
        override fun setAcceleration(acceleration: Float) { calls += "acceleration $acceleration" }
        override fun setHeadSize(value: Int) { calls += "headSize $value" }
        override fun setBreathiness(value: Int) { calls += "breathiness $value" }
        override fun setRoughness(value: Int) { calls += "roughness $value" }
        override fun setBrightness(value: Int) { calls += "brightness $value" }
        override fun setSentencePause(pauseMs: Int) { calls += "sentencePause $pauseMs" }
        override fun setCommaPause(pauseMs: Int) { calls += "commaPause $pauseMs" }
        override fun setNewlinePause(pauseMs: Int) { calls += "newlinePause $pauseMs" }
        override fun setNumberMode(mode: Int) { calls += "numberMode $mode" }
        override fun setSpellingSpeed(percent: Int) { calls += "spellingSpeed $percent" }
        override fun setLetterSounds(enabled: Boolean) { calls += "letterSounds $enabled" }
        override fun setUserDictionariesEnabled(enabled: Boolean) { calls += "userDictionaries $enabled" }
        override fun clearError() { calls += "clearError" }
    }

    private val actions = RecordingActions()

    private fun show(
        uiState: SettingsUiState = state,
        systemLanguage: String? = null,
        expanded: Boolean = false,
    ) {
        rule.setContent {
            LaprdusTheme {
                SettingsScreen(
                    uiState = uiState,
                    systemLanguage = systemLanguage,
                    actions = actions,
                    onNavigateBack = {},
                    onOpenDictionaries = {},
                    onOpenAbout = {},
                    initiallyExpanded = expanded,
                )
            }
        }
    }

    private fun texts(node: SemanticsNode): List<String> =
        node.config.getOrNull(SemanticsProperties.Text)?.map { it.text }.orEmpty()

    private fun hasTexts(vararg expected: String) =
        SemanticsMatcher("texts ${expected.toList()}") { texts(it) == expected.toList() }

    private fun hasRole(role: Role) =
        SemanticsMatcher("role $role") { it.config.getOrNull(SemanticsProperties.Role) == role }

    private fun hasState(state: String) =
        SemanticsMatcher("state $state") { it.config.getOrNull(SemanticsProperties.StateDescription) == state }

    private fun hasNoContentDescription() =
        SemanticsMatcher("no content description") { it.config.getOrNull(SemanticsProperties.ContentDescription) == null }

    /** A node TalkBack stops on because it can be activated or adjusted. */
    private val isControl = SemanticsMatcher("control") {
        it.config.contains(SemanticsActions.OnClick) || it.config.contains(SemanticsActions.SetProgress)
    }

    // ==========================================================================
    // Switch rows
    // ==========================================================================

    @Test
    fun switchRow_readsTitleThenSummary_asOneSwitch() {
        show()
        val title = s(R.string.setting_spelling_mode)
        val summary = s(R.string.setting_spelling_mode_subtitle)
        rule.onNode(hasText(title) and hasText(summary))
            .performScrollTo()
            .assert(hasTexts(title, summary))
            .assert(hasRole(Role.Switch))
            .assert(hasNoContentDescription())
            .assert(SemanticsMatcher.expectValue(SemanticsProperties.ToggleableState, ToggleableState.On))
            .performClick()
        assertEquals(listOf("letterSounds false"), actions.calls)
    }

    @Test
    fun spellingSwitch_hasTheShortName() {
        show()
        // Slova kao glasovi / Črke kot glasovi / Letters as sounds: the title
        // says what "on" does, in a few words.
        assertTrue(s(R.string.setting_spelling_mode).split(' ').size <= 3)
        // A short summary without example letters: Laprdus reads a lone "b"
        // by this very switch, so an example could never be trusted.
        val summary = s(R.string.setting_spelling_mode_subtitle)
        assertTrue(summary, summary.length <= 40)
        // No list of letters such as "b, c, d" (a lone letter before a comma).
        assertTrue(summary, !Regex("""(^|[\s,])\p{L},""").containsMatchIn(summary))
    }

    @Test
    fun forceSwitches_areInTheAppsSection() {
        show()
        val apps = s(R.string.category_apps)
        rule.onNode(hasText(apps) and isHeading()).performScrollTo()
        for (id in listOf(
            R.string.setting_force_language_title,
            R.string.setting_force_speed_title,
            R.string.setting_force_pitch_title,
            R.string.setting_force_volume_title,
        )) {
            rule.onNode(hasText(s(id)) and hasRole(Role.Switch)).performScrollTo()
                .assert(SemanticsMatcher.expectValue(SemanticsProperties.ToggleableState, ToggleableState.Off))
        }
    }

    // ==========================================================================
    // Slider rows
    // ==========================================================================

    @Test
    fun sliderRow_labelIsTheTitle_stateIsTheValue() {
        show()
        val title = s(R.string.setting_speech_rate)
        val node = rule.onNode(hasContentDescription(title) and hasState(SettingFormat.multiplier(1.5f, locale)))
            .performScrollTo()
            .fetchSemanticsNode()
        val range = node.config[SemanticsProperties.ProgressBarRangeInfo]
        assertEquals(1.5f, range.current, 0.001f)
        assertEquals(VoiceInfo.FORMANT_RANGE, range.range)
        // One TalkBack swipe moves 0.05x.
        assertEquals(SettingFormat.steps(VoiceInfo.FORMANT_RANGE, SettingFormat.MULTIPLIER_STEP), range.steps)
    }

    @Test
    fun sliderRow_restoresTheDefault_throughItsAction() {
        show()
        val title = s(R.string.setting_speech_rate)
        val restore = s(R.string.action_restore_default, SettingFormat.multiplier(1.0f, locale))
        val node = rule.onNode(hasContentDescription(title)).performScrollTo().fetchSemanticsNode()
        val action = node.config[SemanticsActions.CustomActions].single { it.label == restore }
        action.action()
        assertEquals(listOf("speed 1.0"), actions.calls)
    }

    @Test
    fun sliderAtDefault_offersNoRestore() {
        show()
        val node = rule.onNode(hasContentDescription(s(R.string.setting_speech_pitch)))
            .performScrollTo().fetchSemanticsNode()
        assertNull(node.config.getOrNull(SemanticsActions.CustomActions))
    }

    @Test
    fun sliderWithSummary_readsTitleThenSummary() {
        show()
        val label = s(R.string.setting_spelling_speed) + ", " + s(R.string.setting_spelling_speed_desc)
        rule.onNode(hasContentDescription(label)).performScrollTo()
            .assert(hasState(String.format(locale, s(R.string.format_percent), 50)))
    }

    /**
     * What TalkBack receives, not only Compose's semantics: Compose moves a
     * node's label onto a child node when the node has children, and
     * TalkBack does not read the children of a slider. Every slider must
     * reach the platform with its own label and no children.
     */
    @Test
    fun everySlider_reachesTalkBackWithItsName() {
        show(expanded = true)
        val sliders = rule.onAllNodes(SemanticsMatcher.keyIsDefined(SemanticsActions.SetProgress))
            .fetchSemanticsNodes()
        // Rate, pitch, volume, acceleration, inflection, the four voice
        // quality settings, spelling speed, three pauses.
        assertEquals(13, sliders.size)
        for (node in sliders) {
            val label = node.config[SemanticsProperties.ContentDescription].first()
            // Compose reports only nodes on screen.
            rule.onNode(SemanticsMatcher("id ${node.id}") { it.id == node.id }).performScrollTo()
            rule.runOnUiThread {
                val provider = composeView(rule.activity.window.decorView)!!.accessibilityNodeProvider!!
                val info = provider.createAccessibilityNodeInfo(node.id)
                    ?: throw AssertionError("no accessibility node for slider $label")
                assertEquals(label, info.contentDescription?.toString())
                assertEquals("android.widget.SeekBar", info.className?.toString())
                assertEquals("slider $label has children", 0, info.childCount)
            }
        }
    }

    private fun composeView(view: android.view.View): android.view.View? {
        if (view.javaClass.name.endsWith("AndroidComposeView")) return view
        if (view is android.view.ViewGroup) {
            for (i in 0 until view.childCount) composeView(view.getChildAt(i))?.let { return it }
        }
        return null
    }

    @Test
    fun tellingTalkBackTheValue_setsItOnce() {
        show()
        rule.onNode(hasContentDescription(s(R.string.setting_speech_rate)))
            .performScrollTo()
            .performSemanticsAction(SemanticsActions.SetProgress) { it(1.0f) }
        assertEquals(listOf("speed 1.0"), actions.calls)
    }

    // ==========================================================================
    // More voice settings
    // ==========================================================================

    @Test
    fun moreVoiceSettings_areCollapsed_untilExpanded() {
        show()
        val more = s(R.string.setting_more_voice)
        val acceleration = s(R.string.setting_acceleration)
        rule.onNode(hasContentDescription(acceleration, substring = true)).assertDoesNotExist()
        // TalkBack says "collapsed" from the expand action; no state of our own.
        rule.onNode(hasText(more))
            .performScrollTo()
            .assert(SemanticsMatcher.keyNotDefined(SemanticsProperties.StateDescription))
            .assert(SemanticsMatcher.keyIsDefined(SemanticsActions.Expand))
            .assert(SemanticsMatcher.keyNotDefined(SemanticsActions.Collapse))
            .performSemanticsAction(SemanticsActions.Expand)
        rule.onNode(hasContentDescription(acceleration, substring = true)).performScrollTo()
        for (id in listOf(
            R.string.setting_inflection_level, R.string.setting_head_size, R.string.setting_breathiness,
            R.string.setting_roughness, R.string.setting_brightness,
        )) {
            rule.onNode(hasContentDescription(s(id), substring = true)).performScrollTo()
        }
        rule.onNode(hasText(s(R.string.setting_volume_boost_title)) and hasRole(Role.Switch)).performScrollTo()
        rule.onNode(hasText(more))
            .assert(SemanticsMatcher.keyIsDefined(SemanticsActions.Collapse))
            .assert(SemanticsMatcher.keyNotDefined(SemanticsActions.Expand))
            .performSemanticsAction(SemanticsActions.Collapse)
        rule.onNode(hasContentDescription(acceleration, substring = true)).assertDoesNotExist()
    }

    @Test
    fun moreVoiceSettings_summaryNamesWhatItHides() {
        show()
        val node = rule.onNode(hasText(s(R.string.setting_more_voice))).fetchSemanticsNode()
        val summary = texts(node).getOrNull(1).orEmpty()
        for (id in listOf(R.string.setting_acceleration, R.string.setting_inflection_level, R.string.setting_brightness)) {
            assertTrue(summary.contains(s(id)))
        }
    }

    // ==========================================================================
    // Voice and language
    // ==========================================================================

    @Test
    fun voiceRow_readsNameThenVoice_andOpensARadioList() {
        show()
        val title = s(R.string.voice_label)
        // Clicked as TalkBack does (an accessibility click, not a touch at
        // coordinates, which can land while the dialog is still being laid out).
        rule.onNode(hasTexts(title, "Zvonko")).assert(isControl).performSemanticsAction(SemanticsActions.OnClick)
        rule.waitUntil(5_000) { rule.onAllNodes(isDialog()).fetchSemanticsNodes().isNotEmpty() }
        rule.onNode(hasText("Zvonko") and isSelectable()).assertIsSelected().assert(hasRole(Role.RadioButton))
        rule.onNode(hasText("Vlado") and isSelectable()).assertIsNotSelected()
            .performSemanticsAction(SemanticsActions.OnClick)
        assertEquals(listOf("voice vlado"), actions.calls)
        rule.onNode(isDialog()).assertDoesNotExist()
    }

    @Test
    fun languageRow_followingTheSystem_isDisabledAndSaysWhy() {
        show(systemLanguage = "sl")
        val follows = s(R.string.language_follows_system, s(R.string.language_slovenian))
        rule.onNode(hasTexts(s(R.string.language_label), follows)).assertIsNotEnabled()
    }

    @Test
    fun languageRow_forced_isEnabled() {
        show(uiState = state.copy(forceLanguage = true), systemLanguage = "sl")
        rule.onNode(hasTexts(s(R.string.language_label), s(R.string.language_croatian))).assertIsEnabled()
    }

    // ==========================================================================
    // The screen as a whole
    // ==========================================================================

    @Test
    fun sections_areHeadings() {
        show()
        for (id in listOf(
            R.string.category_voice, R.string.category_apps, R.string.category_reading,
            R.string.category_pause_settings, R.string.category_dictionaries, R.string.category_about,
        )) {
            rule.onNode(hasText(s(id)) and isHeading()).performScrollTo()
        }
    }

    @Test
    fun everySetting_isOneStop() {
        show()
        // Back; voice, language, rate, pitch, volume, more voice settings;
        // four force switches; spelling, spelling speed, digits, emoji;
        // three pauses; user dictionaries, manage dictionaries; about.
        rule.onAllNodes(isControl).assertCountEquals(21)
    }

    @Test
    fun everySetting_isOneStop_whenExpanded() {
        show(expanded = true)
        rule.onAllNodes(isControl).assertCountEquals(28)
    }

    @Test
    fun accessibilityChecks_pass() {
        rule.enableAccessibilityChecks()
        show(expanded = true)
        rule.onRoot().tryPerformAccessibilityChecks()
    }
}
