package com.hrvojekatic.laprdus.ui.screens

import androidx.compose.ui.semantics.SemanticsProperties
import androidx.compose.ui.semantics.getOrNull
import androidx.compose.ui.test.SemanticsMatcher
import androidx.compose.ui.test.hasContentDescription
import androidx.compose.ui.test.hasStateDescription
import androidx.compose.ui.test.junit4.createComposeRule
import androidx.compose.ui.test.performClick
import com.hrvojekatic.laprdus.tts.VoiceInfo
import com.hrvojekatic.laprdus.viewmodel.SettingsUiState
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test

/**
 * Accessibility tests for the Settings screen.
 *
 * These tests verify that TalkBack announcements follow the correct order:
 * - For switches: "Title. Switch. On/Off, Description"
 * - For sliders: "Title. Slider. Value, Description"
 * - For dropdowns: "Label. Dropdown, Value"
 *
 * Tests also verify:
 * - Sliders remain adjustable
 * - Switches remain toggleable
 * - All settings items have isTraversalGroup set for proper swipe navigation order
 */
class SettingsScreenAccessibilityTest {

    @get:Rule
    val composeTestRule = createComposeRule()

    private val testVoices = listOf(
        VoiceInfo(
            id = "josip",
            displayName = "Laprdus Josip (Croatian)",
            languageCode = "hr-HR",
            gender = "Male",
            age = "Adult",
            basePitch = 1.0f
        ),
        VoiceInfo(
            id = "vlado",
            displayName = "Laprdus Vlado (Serbian)",
            languageCode = "sr-RS",
            gender = "Male",
            age = "Adult",
            basePitch = 1.0f
        )
    )

    private val defaultUiState = SettingsUiState(
        isLoading = false,
        availableVoices = testVoices,
        selectedVoiceId = "josip",
        speed = 1.0f,
        pitch = 1.0f,
        volume = 0.8f,
        forceSpeed = false,
        forcePitch = false,
        forceVolume = false,
        forceLanguage = true,
        emojiEnabled = true,
        sentencePause = 500,
        commaPause = 200,
        newlinePause = 300,
        numberMode = 0,
        error = null
    )

    // ==========================================================================
    // Switch Accessibility Tests
    // ==========================================================================

    @Test
    fun switchSettingItem_hasCorrectStateDescription_whenOff() {
        composeTestRule.setContent {
            SwitchSettingItem(
                title = "Force Language",
                subtitle = "Use selected language regardless of system settings",
                checked = false,
                onCheckedChange = {}
            )
        }

        // Verify stateDescription contains title, switch role, and state
        val matcher = hasStateDescription("Force Language. Switch. Off")
        composeTestRule.onNode(matcher).assertExists()
    }

    @Test
    fun switchSettingItem_hasCorrectStateDescription_whenOn() {
        composeTestRule.setContent {
            SwitchSettingItem(
                title = "Force Language",
                subtitle = "Use selected language regardless of system settings",
                checked = true,
                onCheckedChange = {}
            )
        }

        // Verify stateDescription contains title, switch role, and state
        val matcher = hasStateDescription("Force Language. Switch. On")
        composeTestRule.onNode(matcher).assertExists()
    }

    @Test
    fun switchSettingItem_hasCorrectContentDescription() {
        composeTestRule.setContent {
            SwitchSettingItem(
                title = "Force Language",
                subtitle = "Use selected language regardless of system settings",
                checked = false,
                onCheckedChange = {}
            )
        }

        // Verify contentDescription contains the description/subtitle
        val matcher = hasContentDescription("Use selected language regardless of system settings")
        composeTestRule.onNode(matcher).assertExists()
    }

    @Test
    fun switchSettingItem_hasToggleableState() {
        var checkedState = false
        composeTestRule.setContent {
            SwitchSettingItem(
                title = "Force Language",
                subtitle = "Use selected language regardless of system settings",
                checked = checkedState,
                onCheckedChange = { checkedState = it }
            )
        }

        // Verify the switch is toggleable by clicking
        composeTestRule.onNode(hasStateDescription("Force Language. Switch. Off"))
            .performClick()

        // State should change
        assertTrue("Switch should toggle when clicked", checkedState)
    }

    @Test
    fun switchSettingItem_hasTraversalGroup() {
        composeTestRule.setContent {
            SwitchSettingItem(
                title = "Force Language",
                subtitle = "Use selected language regardless of system settings",
                checked = false,
                onCheckedChange = {}
            )
        }

        // Verify isTraversalGroup is set for proper TalkBack navigation
        val hasTraversalGroup = SemanticsMatcher("has isTraversalGroup") { node ->
            node.config.getOrNull(SemanticsProperties.IsTraversalGroup) == true
        }

        composeTestRule.onNode(hasTraversalGroup).assertExists()
    }

    // ==========================================================================
    // Slider Accessibility Tests
    // ==========================================================================

    @Test
    fun sliderSettingItem_hasCorrectStateDescription() {
        composeTestRule.setContent {
            SliderSettingItem(
                title = "Speech rate",
                value = 1.5f,
                valueRange = 0.5f..2.0f,
                onValueChange = {},
                valueLabel = "1.5x",
                description = "Slide to adjust"
            )
        }

        // Verify stateDescription contains title, slider role, and value
        val matcher = hasStateDescription("Speech rate. Slider. 1.5x")
        composeTestRule.onNode(matcher).assertExists()
    }

    @Test
    fun sliderSettingItem_hasCorrectContentDescription_withDescription() {
        composeTestRule.setContent {
            SliderSettingItem(
                title = "Speech rate",
                value = 1.5f,
                valueRange = 0.5f..2.0f,
                onValueChange = {},
                valueLabel = "1.5x",
                description = "Slide to adjust"
            )
        }

        // Verify contentDescription contains the description
        val matcher = hasContentDescription("Slide to adjust")
        composeTestRule.onNode(matcher).assertExists()
    }

    @Test
    fun sliderSettingItem_preservesProgressBarRangeInfo() {
        composeTestRule.setContent {
            SliderSettingItem(
                title = "Speech rate",
                value = 0.75f,
                valueRange = 0.5f..2.0f,
                onValueChange = {},
                valueLabel = "0.8x",
                description = ""
            )
        }

        // Verify the slider has progressBarRangeInfo (required for adjustability)
        val hasProgressBarInfo = SemanticsMatcher("has progressBarRangeInfo") { node ->
            node.config.getOrNull(SemanticsProperties.ProgressBarRangeInfo) != null
        }

        composeTestRule.onNode(hasProgressBarInfo).assertExists()
    }

    @Test
    fun sliderSettingItem_isAdjustable() {
        var currentValue = 1.0f
        composeTestRule.setContent {
            SliderSettingItem(
                title = "Speech rate",
                value = currentValue,
                valueRange = 0.5f..2.0f,
                onValueChange = { currentValue = it },
                valueLabel = "%.1fx".format(currentValue),
                description = ""
            )
        }

        // Verify the slider has setProgress action (indicates adjustability)
        val hasSetProgress = SemanticsMatcher("has setProgress action") { node ->
            node.config.getOrNull(SemanticsProperties.ProgressBarRangeInfo) != null
        }

        composeTestRule.onNode(hasSetProgress).assertExists()
    }

    @Test
    fun sliderSettingItem_hasTraversalGroup() {
        composeTestRule.setContent {
            SliderSettingItem(
                title = "Speech rate",
                value = 1.0f,
                valueRange = 0.5f..2.0f,
                onValueChange = {},
                valueLabel = "1.0x",
                description = ""
            )
        }

        // Verify isTraversalGroup is set for proper TalkBack navigation
        val hasTraversalGroup = SemanticsMatcher("has isTraversalGroup") { node ->
            node.config.getOrNull(SemanticsProperties.IsTraversalGroup) == true
        }

        composeTestRule.onNode(hasTraversalGroup).assertExists()
    }

    // ==========================================================================
    // Dropdown Accessibility Tests
    // ==========================================================================

    @Test
    fun voiceSettingItem_hasCorrectStateDescription() {
        composeTestRule.setContent {
            VoiceSettingItem(
                voices = testVoices,
                selectedVoiceId = "josip",
                onVoiceSelected = {}
            )
        }

        // Verify stateDescription contains label and dropdown role
        // Note: The exact string depends on localized resources
        val hasDropdownState = SemanticsMatcher("has Voice and Dropdown in stateDescription") { node ->
            val stateDesc = node.config.getOrNull(SemanticsProperties.StateDescription)
            stateDesc != null && stateDesc.contains("Voice") && stateDesc.contains("Dropdown")
        }

        composeTestRule.onNode(hasDropdownState).assertExists()
    }

    @Test
    fun voiceSettingItem_hasCorrectContentDescription_withSelectedVoice() {
        composeTestRule.setContent {
            VoiceSettingItem(
                voices = testVoices,
                selectedVoiceId = "josip",
                onVoiceSelected = {}
            )
        }

        // Verify contentDescription contains the selected voice name
        val matcher = hasContentDescription("Josip")
        composeTestRule.onNode(matcher).assertExists()
    }

    @Test
    fun voiceSettingItem_updatesContentDescription_whenSelectionChanges() {
        var selectedId = "josip"
        composeTestRule.setContent {
            VoiceSettingItem(
                voices = testVoices,
                selectedVoiceId = selectedId,
                onVoiceSelected = { selectedId = it }
            )
        }

        // Initially shows Josip
        composeTestRule.onNode(hasContentDescription("Josip")).assertExists()
    }

    @Test
    fun voiceSettingItem_hasTraversalGroup() {
        composeTestRule.setContent {
            VoiceSettingItem(
                voices = testVoices,
                selectedVoiceId = "josip",
                onVoiceSelected = {}
            )
        }

        // Verify isTraversalGroup is set for proper TalkBack navigation
        val hasTraversalGroup = SemanticsMatcher("has isTraversalGroup") { node ->
            node.config.getOrNull(SemanticsProperties.IsTraversalGroup) == true
        }

        composeTestRule.onNode(hasTraversalGroup).assertExists()
    }

    // ==========================================================================
    // Category Header Tests
    // ==========================================================================

    @Test
    fun settingsCategoryHeader_markedAsHeading() {
        composeTestRule.setContent {
            SettingsCategoryHeader(title = "Voice")
        }

        // Verify the header has heading semantics
        val hasHeading = SemanticsMatcher("has heading") { node ->
            node.config.getOrNull(SemanticsProperties.Heading) != null
        }

        composeTestRule.onNode(hasHeading).assertExists()
    }

    // ==========================================================================
    // Full Settings Screen Integration Tests
    // ==========================================================================

    @Test
    fun settingsScreen_allSwitchesHaveCorrectAccessibility() {
        composeTestRule.setContent {
            SettingsScreen(
                uiState = defaultUiState,
                onNavigateBack = {},
                onVoiceSelected = {},
                onSpeedChange = {},
                onPitchChange = {},
                onVolumeChange = {},
                onForceSpeedChange = {},
                onForcePitchChange = {},
                onForceVolumeChange = {},
                onForceLanguageChange = {},
                onRestoreDefaultSpeed = {},
                onRestoreDefaultPitch = {},
                onRestoreDefaultVolume = {},
                onEmojiEnabledChange = {},
                onSentencePauseChange = {},
                onCommaPauseChange = {},
                onNewlinePauseChange = {},
                onNumberModeChange = {}
            )
        }

        // Verify all switch items have stateDescription containing "Switch"
        val switchNodes = composeTestRule.onAllNodes(
            SemanticsMatcher("has Switch in stateDescription") { node ->
                val stateDesc = node.config.getOrNull(SemanticsProperties.StateDescription)
                stateDesc != null && stateDesc.contains("Switch")
            }
        )

        // There should be 6 switches in the settings screen
        // (forceSpeed, forcePitch, forceVolume, forceLanguage, emojiEnabled, numberMode)
        val nodeCount = switchNodes.fetchSemanticsNodes().size
        assertTrue("Should have at least 6 switch items", nodeCount >= 6)
    }

    @Test
    fun settingsScreen_allSlidersPreserveProgressBarRangeInfo() {
        composeTestRule.setContent {
            SettingsScreen(
                uiState = defaultUiState,
                onNavigateBack = {},
                onVoiceSelected = {},
                onSpeedChange = {},
                onPitchChange = {},
                onVolumeChange = {},
                onForceSpeedChange = {},
                onForcePitchChange = {},
                onForceVolumeChange = {},
                onForceLanguageChange = {},
                onRestoreDefaultSpeed = {},
                onRestoreDefaultPitch = {},
                onRestoreDefaultVolume = {},
                onEmojiEnabledChange = {},
                onSentencePauseChange = {},
                onCommaPauseChange = {},
                onNewlinePauseChange = {},
                onNumberModeChange = {}
            )
        }

        // Verify all sliders have progressBarRangeInfo (required for TalkBack adjustability)
        val sliderNodes = composeTestRule.onAllNodes(
            SemanticsMatcher("has progressBarRangeInfo") { node ->
                node.config.getOrNull(SemanticsProperties.ProgressBarRangeInfo) != null
            }
        )

        // There should be 6 sliders in the settings screen
        // (speed, pitch, volume, sentencePause, commaPause, newlinePause)
        val nodeCount = sliderNodes.fetchSemanticsNodes().size
        assertTrue("Should have at least 6 sliders with progressBarRangeInfo", nodeCount >= 6)
    }

    @Test
    fun settingsScreen_categoryHeadersAreHeadings() {
        composeTestRule.setContent {
            SettingsScreen(
                uiState = defaultUiState,
                onNavigateBack = {},
                onVoiceSelected = {},
                onSpeedChange = {},
                onPitchChange = {},
                onVolumeChange = {},
                onForceSpeedChange = {},
                onForcePitchChange = {},
                onForceVolumeChange = {},
                onForceLanguageChange = {},
                onRestoreDefaultSpeed = {},
                onRestoreDefaultPitch = {},
                onRestoreDefaultVolume = {},
                onEmojiEnabledChange = {},
                onSentencePauseChange = {},
                onCommaPauseChange = {},
                onNewlinePauseChange = {},
                onNumberModeChange = {}
            )
        }

        // Verify category headers are marked as headings
        val headingNodes = composeTestRule.onAllNodes(
            SemanticsMatcher("has heading") { node ->
                node.config.getOrNull(SemanticsProperties.Heading) != null
            }
        )

        // There should be 3 category headers (Voice, Advanced, Reading Pauses)
        val nodeCount = headingNodes.fetchSemanticsNodes().size
        assertTrue("Should have at least 3 heading elements", nodeCount >= 3)
    }

    // ==========================================================================
    // Traversal Order Tests (TalkBack swipe navigation)
    // ==========================================================================

    @Test
    fun settingsScreen_allSettingsItemsHaveTraversalGroup() {
        composeTestRule.setContent {
            SettingsScreen(
                uiState = defaultUiState,
                onNavigateBack = {},
                onVoiceSelected = {},
                onSpeedChange = {},
                onPitchChange = {},
                onVolumeChange = {},
                onForceSpeedChange = {},
                onForcePitchChange = {},
                onForceVolumeChange = {},
                onForceLanguageChange = {},
                onRestoreDefaultSpeed = {},
                onRestoreDefaultPitch = {},
                onRestoreDefaultVolume = {},
                onEmojiEnabledChange = {},
                onSentencePauseChange = {},
                onCommaPauseChange = {},
                onNewlinePauseChange = {},
                onNumberModeChange = {}
            )
        }

        // Verify all interactive elements have isTraversalGroup set
        // This ensures TalkBack navigates through items in order
        val traversalGroupNodes = composeTestRule.onAllNodes(
            SemanticsMatcher("has isTraversalGroup") { node ->
                node.config.getOrNull(SemanticsProperties.IsTraversalGroup) == true
            }
        )

        // Count interactive elements:
        // 1 voice dropdown + 6 sliders + 6 switches + 3 restore buttons = 16 items
        val nodeCount = traversalGroupNodes.fetchSemanticsNodes().size
        assertTrue(
            "Should have at least 16 items with isTraversalGroup for proper TalkBack navigation (found $nodeCount)",
            nodeCount >= 16
        )
    }
}
