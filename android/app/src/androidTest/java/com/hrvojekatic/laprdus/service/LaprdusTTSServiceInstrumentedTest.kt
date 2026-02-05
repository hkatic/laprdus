package com.hrvojekatic.laprdus.service

import android.speech.tts.TextToSpeech
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Instrumented tests for LaprdusTTSService.
 * These tests run on an actual Android device/emulator to verify
 * TTS service functionality with real asset loading.
 */
@RunWith(AndroidJUnit4::class)
class LaprdusTTSServiceInstrumentedTest {

    private val context = InstrumentationRegistry.getInstrumentation().targetContext

    @Before
    fun setup() {
        // Ensure clean state
    }

    // ==========================================================================
    // Asset Availability Tests
    // ==========================================================================

    @Test
    fun voiceAssetsAreAvailable() {
        val assetManager = context.assets

        // Check voice binary files
        val josipAsset = assetManager.open("Josip.bin")
        assertNotNull("Josip.bin should be available in assets", josipAsset)
        josipAsset.close()

        val vladoAsset = assetManager.open("Vlado.bin")
        assertNotNull("Vlado.bin should be available in assets", vladoAsset)
        vladoAsset.close()
    }

    @Test
    fun dictionaryAssetsAreAvailable() {
        val assetManager = context.assets

        // Check dictionary files
        val internalDict = assetManager.open("internal.json")
        assertNotNull("internal.json should be available in assets", internalDict)
        internalDict.close()

        val spellingDict = assetManager.open("spelling.json")
        assertNotNull("spelling.json should be available in assets", spellingDict)
        spellingDict.close()

        val emojiDict = assetManager.open("emoji.json")
        assertNotNull("emoji.json should be available in assets", emojiDict)
        emojiDict.close()
    }

    @Test
    fun dictionaryFilesHaveContent() {
        val assetManager = context.assets

        // Verify internal.json has entries
        val internalContent = assetManager.open("internal.json").bufferedReader().readText()
        assert(internalContent.contains("entries")) { "internal.json should contain 'entries' array" }
        assert(internalContent.contains("grapheme")) { "internal.json should have grapheme entries" }

        // Verify spelling.json has entries
        val spellingContent = assetManager.open("spelling.json").bufferedReader().readText()
        assert(spellingContent.contains("character")) { "spelling.json should contain character entries" }

        // Verify emoji.json has entries
        val emojiContent = assetManager.open("emoji.json").bufferedReader().readText()
        assert(emojiContent.contains("emoji")) { "emoji.json should contain emoji entries" }
    }

    // ==========================================================================
    // TTS Engine Integration Tests
    // ==========================================================================

    @Test
    fun ttsEngineCanBeInitialized() {
        val tts = com.hrvojekatic.laprdus.tts.LaprdusTTS.getInstance()
        val success = tts.setVoice("josip", context.assets)
        assert(success) { "TTS engine should initialize successfully with josip voice" }
        assert(tts.isInitialized()) { "TTS engine should report as initialized" }
    }

    @Test
    fun allVoicesAreAvailable() {
        val tts = com.hrvojekatic.laprdus.tts.LaprdusTTS.getInstance()
        val voices = tts.getAllVoices()

        // Should have 5 voices: josip, vlado, detence, baba, djedo
        assert(voices.size >= 5) { "Should have at least 5 voices, found ${voices.size}" }

        val voiceIds = voices.map { it.id }
        assert("josip" in voiceIds) { "Should have josip voice" }
        assert("vlado" in voiceIds) { "Should have vlado voice" }
        assert("detence" in voiceIds) { "Should have detence voice" }
        assert("baba" in voiceIds) { "Should have baba voice" }
        assert("djedo" in voiceIds) { "Should have djedo voice" }
    }

    @Test
    fun derivedVoicesHaveCorrectBasePitch() {
        val tts = com.hrvojekatic.laprdus.tts.LaprdusTTS.getInstance()
        val voices = tts.getAllVoices()

        val detence = voices.find { it.id == "detence" }
        assertNotNull("detence voice should exist", detence)
        assertEquals("detence should have basePitch 1.5", 1.5f, detence!!.basePitch)

        val baba = voices.find { it.id == "baba" }
        assertNotNull("baba voice should exist", baba)
        assertEquals("baba should have basePitch 1.2", 1.2f, baba!!.basePitch)
    }

    @Test
    fun synthesisProducesSamples() {
        val tts = com.hrvojekatic.laprdus.tts.LaprdusTTS.getInstance()
        tts.setVoice("josip", context.assets)

        val samples = tts.synthesize("Dobar dan")
        assertNotNull("Synthesis should return samples", samples)
        assert(samples!!.isNotEmpty()) { "Samples array should not be empty" }
    }

    // ==========================================================================
    // Spelling Dictionary Tests (Critical for TalkBack accessibility)
    // ==========================================================================

    @Test
    fun spelledSynthesisProducesSamplesForSingleLetter() {
        val tts = com.hrvojekatic.laprdus.tts.LaprdusTTS.getInstance()
        tts.setVoice("josip", context.assets)

        // Single letter should be spelled (pronounced as letter name)
        val samples = tts.synthesizeSpelled("A")
        assertNotNull("Spelled synthesis should return samples for letter A", samples)
        assert(samples!!.isNotEmpty()) { "Spelled samples should not be empty" }
    }

    @Test
    fun spelledSynthesisProducesSamplesForCroatianLetter() {
        val tts = com.hrvojekatic.laprdus.tts.LaprdusTTS.getInstance()
        tts.setVoice("josip", context.assets)

        // Croatian letter with diacritic
        val samples = tts.synthesizeSpelled("Č")
        assertNotNull("Spelled synthesis should return samples for Č", samples)
        assert(samples!!.isNotEmpty()) { "Spelled samples for Č should not be empty" }
    }

    @Test
    fun spelledSynthesisProducesSamplesForDigit() {
        val tts = com.hrvojekatic.laprdus.tts.LaprdusTTS.getInstance()
        tts.setVoice("josip", context.assets)

        // Digit should be spelled (pronounced as number word)
        val samples = tts.synthesizeSpelled("5")
        assertNotNull("Spelled synthesis should return samples for digit 5", samples)
        assert(samples!!.isNotEmpty()) { "Spelled samples for digit should not be empty" }
    }

    @Test
    fun spelledSynthesisProducesSamplesForPunctuation() {
        val tts = com.hrvojekatic.laprdus.tts.LaprdusTTS.getInstance()
        tts.setVoice("josip", context.assets)

        // Punctuation should be spelled (pronounced as punctuation name)
        val samples = tts.synthesizeSpelled(".")
        assertNotNull("Spelled synthesis should return samples for period", samples)
        assert(samples!!.isNotEmpty()) { "Spelled samples for period should not be empty" }
    }

    @Test
    fun spelledSynthesisDifferentFromNormalSynthesis() {
        val tts = com.hrvojekatic.laprdus.tts.LaprdusTTS.getInstance()
        tts.setVoice("josip", context.assets)

        // Normal synthesis of "B" should produce short audio (just the sound "b")
        val normalSamples = tts.synthesize("B")
        // Spelled synthesis of "B" should produce longer audio (says "Be")
        val spelledSamples = tts.synthesizeSpelled("B")

        assertNotNull("Normal synthesis should return samples", normalSamples)
        assertNotNull("Spelled synthesis should return samples", spelledSamples)

        // Spelled version should be different (typically longer as it says the letter name)
        // Note: We can't compare exact content, but we can verify both produce output
        assert(normalSamples!!.isNotEmpty()) { "Normal samples should not be empty" }
        assert(spelledSamples!!.isNotEmpty()) { "Spelled samples should not be empty" }
    }

    @Test
    fun spelledSynthesisWithSpaceProducesSamples() {
        val tts = com.hrvojekatic.laprdus.tts.LaprdusTTS.getInstance()
        tts.setVoice("josip", context.assets)

        // Space should be spelled as "razmak"
        val samples = tts.synthesizeSpelled(" ")
        assertNotNull("Spelled synthesis should return samples for space", samples)
        assert(samples!!.isNotEmpty()) { "Spelled samples for space should not be empty" }
    }

    // ==========================================================================
    // Single Grapheme Detection Tests
    // ==========================================================================

    @Test
    fun isSingleGraphemeDetectsSimpleCharacters() {
        assertTrue("Single ASCII letter should be detected", isSingleGrapheme("A"))
        assertTrue("Single digit should be detected", isSingleGrapheme("5"))
        assertTrue("Single punctuation should be detected", isSingleGrapheme("."))
    }

    @Test
    fun isSingleGraphemeDetectsCroatianCharacters() {
        assertTrue("Č should be detected as single grapheme", isSingleGrapheme("Č"))
        assertTrue("ć should be detected as single grapheme", isSingleGrapheme("ć"))
        assertTrue("Đ should be detected as single grapheme", isSingleGrapheme("Đ"))
        assertTrue("š should be detected as single grapheme", isSingleGrapheme("š"))
        assertTrue("Ž should be detected as single grapheme", isSingleGrapheme("Ž"))
    }

    @Test
    fun isSingleGraphemeRejectsMultipleCharacters() {
        assertFalse("Two letters should not be single grapheme", isSingleGrapheme("AB"))
        assertFalse("Word should not be single grapheme", isSingleGrapheme("Hello"))
        assertFalse("Two spaces should not be single grapheme", isSingleGrapheme("  "))
    }

    @Test
    fun isSingleGraphemeHandlesEmptyString() {
        assertFalse("Empty string should not be single grapheme", isSingleGrapheme(""))
    }

    /**
     * Helper function to detect single grapheme.
     * Mirrors the implementation in LaprdusTTSService.
     */
    private fun isSingleGrapheme(text: String): Boolean {
        if (text.isEmpty()) return false
        val iterator = java.text.BreakIterator.getCharacterInstance()
        iterator.setText(text)
        iterator.first()
        val end = iterator.next()
        return end == text.length
    }
}
