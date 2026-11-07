package com.hrvojekatic.laprdus.ui.settings

import com.hrvojekatic.laprdus.tts.VoiceInfo
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import java.util.Locale

class SettingFormatTest {

    @Test
    fun steps_countThePositionsBetweenTheEnds() {
        // 0.25-4.0 in steps of 0.05: 75 intervals, 74 positions between the ends.
        assertEquals(74, SettingFormat.steps(VoiceInfo.FORMANT_RANGE, SettingFormat.MULTIPLIER_STEP))
        assertEquals(29, SettingFormat.steps(VoiceInfo.RECORDED_RANGE, SettingFormat.MULTIPLIER_STEP))
        assertEquals(19, SettingFormat.steps(0f..100f, SettingFormat.PERCENT_STEP))
        assertEquals(19, SettingFormat.steps(0f..1f, SettingFormat.FRACTION_STEP))
        assertEquals(39, SettingFormat.steps(0f..2000f, SettingFormat.PAUSE_STEP))
        assertEquals(24, SettingFormat.steps(0.5f..3.0f, SettingFormat.ACCELERATION_STEP))
    }

    @Test
    fun snap_landsOnAStep_withoutFloatNoise() {
        assertEquals(1.05f, SettingFormat.snap(1.0612f, VoiceInfo.FORMANT_RANGE, 0.05f))
        assertEquals(1.0f, SettingFormat.snap(1.024f, VoiceInfo.FORMANT_RANGE, 0.05f))
        assertEquals(0.25f, SettingFormat.snap(0.1f, VoiceInfo.FORMANT_RANGE, 0.05f))
        assertEquals(4.0f, SettingFormat.snap(9f, VoiceInfo.FORMANT_RANGE, 0.05f))
        assertEquals(150f, SettingFormat.snap(163f, 0f..2000f, 50f))
        assertEquals(55f, SettingFormat.snap(53f, 0f..100f, 5f))
    }

    @Test
    fun multiplier_followsTheLanguage() {
        assertEquals("1.0x", SettingFormat.multiplier(1.0f, Locale.ENGLISH))
        assertEquals("1,0x", SettingFormat.multiplier(1.0f, Locale.forLanguageTag("hr")))
        assertEquals("1,5x", SettingFormat.multiplier(1.5f, Locale.forLanguageTag("sl")))
        assertEquals("1,05x", SettingFormat.multiplier(1.05f, Locale.forLanguageTag("sr")))
        assertEquals("0.25x", SettingFormat.multiplier(0.25f, Locale.ENGLISH))
    }

    @Test
    fun sameStep_toleratesFloatError() {
        assertTrue(SettingFormat.sameStep(1.0f, 1.0000001f, 0.05f))
        assertFalse(SettingFormat.sameStep(1.0f, 1.05f, 0.05f))
    }

    @Test
    fun topWordsPerMinute_scalesTheTopOfTheAppsRange() {
        // The app's 2.0x scales the Laprdus rate; acceleration multiplies it.
        assertEquals(700, SettingFormat.topWordsPerMinute(175f, 1.0f, 2.0f, false, VoiceInfo.FORMANT_RANGE, true))
        // Forced: the Laprdus rate alone.
        assertEquals(175, SettingFormat.topWordsPerMinute(175f, 1.0f, 1.0f, true, VoiceInfo.FORMANT_RANGE, true))
        // Never above what the voice renders (8x formant, 4x recorded).
        assertEquals(1400, SettingFormat.topWordsPerMinute(175f, 4.0f, 3.0f, false, VoiceInfo.FORMANT_RANGE, true))
        assertEquals(700, SettingFormat.topWordsPerMinute(175f, 2.0f, 3.0f, false, VoiceInfo.RECORDED_RANGE, false))
    }

    @Test
    fun fractionToPercent_rounds() {
        assertEquals(50, SettingFormat.fractionToPercent(0.5f))
        assertEquals(83, SettingFormat.fractionToPercent(0.8299f))
    }
}
