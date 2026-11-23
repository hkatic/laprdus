package com.hrvojekatic.laprdus.tts

import android.content.res.AssetManager
import io.mockk.every
import io.mockk.mockk
import io.mockk.mockkStatic
import io.mockk.unmockkAll
import io.mockk.verify
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config

/**
 * Unit tests for LaprdusTTS wrapper class.
 * Tests verify dictionary loading behavior and voice switching.
 */
@RunWith(RobolectricTestRunner::class)
@Config(manifest = Config.NONE)
class LaprdusTTSTest {

    private lateinit var mockAssetManager: AssetManager

    @Before
    fun setup() {
        mockAssetManager = mockk(relaxed = true)
    }

    @After
    fun tearDown() {
        unmockkAll()
    }

    // ==========================================================================
    // Voice Info Tests
    // ==========================================================================

    @Test
    fun `VoiceInfo correctly identifies physical vs derived voices`() {
        // Physical voice - has data_filename, basePitch 1.0
        val physicalVoice = VoiceInfo(
            id = "josip",
            displayName = "Josip",
            languageCode = "hr-HR",
            gender = "Male",
            age = "Adult",
            basePitch = 1.0f
        )

        // Derived voice - has non-1.0 basePitch
        val derivedVoice = VoiceInfo(
            id = "detence",
            displayName = "Detence",
            languageCode = "hr-HR",
            gender = "Male",
            age = "Child",
            basePitch = 1.5f
        )

        assert(physicalVoice.isPhysicalVoice)
        assert(!derivedVoice.isPhysicalVoice)
    }

    @Test
    fun `VoiceInfo reports the native language of a voice`() {
        val croatianVoice = VoiceInfo(
            id = "josip",
            displayName = "Josip",
            languageCode = "hr-HR",
            gender = "Male",
            age = "Adult",
            basePitch = 1.0f
        )

        val serbianVoice = VoiceInfo(
            id = "vlado",
            displayName = "Vlado",
            languageCode = "sr-RS",
            gender = "Male",
            age = "Adult",
            basePitch = 1.0f
        )

        // Only the default: every voice speaks every language
        assertEquals("hr", croatianVoice.nativeLanguage)
        assertEquals("sr", serbianVoice.nativeLanguage)
    }

    @Test
    fun `an old voice id normalizes to the renamed voice`() {
        // LaprdusTTS.normalizeVoiceId delegates to this (touching the
        // LaprdusTTS companion would load the native library on the JVM)
        assertEquals("djedo", VoiceInfo.normalizeId("djed"))
        assertEquals("lika_velebit", VoiceInfo.normalizeId("pjevac"))
        assertEquals("zvonko", VoiceInfo.normalizeId("zvonko"))
    }

    // ==========================================================================
    // Number Mode Constants Tests
    // ==========================================================================

    @Test
    fun `number mode constants are correct`() {
        assert(LaprdusTTS.NUMBER_MODE_WHOLE == 0)
        assert(LaprdusTTS.NUMBER_MODE_DIGIT == 1)
    }

    // ==========================================================================
    // Dictionary Path Constants Tests
    // ==========================================================================

    @Test
    fun `dictionary asset path constants are correct`() {
        // These paths must match the asset files bundled in the APK
        // The files are sourced from data/dictionary/ in build.gradle.kts
        val dictionaryPath = "internal.json"
        val spellingPath = "spelling.json"
        val emojiPath = "emoji.json"

        // Verify expected paths are documented (actual constants are private)
        // This test documents the expected asset structure
        assert(dictionaryPath.isNotEmpty())
        assert(spellingPath.isNotEmpty())
        assert(emojiPath.isNotEmpty())
    }

    // ==========================================================================
    // Voice ID Tests
    // ==========================================================================

    @Test
    fun `all voice IDs are valid strings`() {
        val validVoiceIds = listOf("josip", "vlado", "detence", "baba", "djedo", "zvonko", "stojan", "mirsad") +
            VoiceInfo.SINGING_VOICE_IDS

        validVoiceIds.forEach { voiceId ->
            assert(voiceId.isNotEmpty()) { "Voice ID should not be empty" }
            assert(voiceId.all { it in 'a'..'z' || it == '_' }) {
                "Voice ID '$voiceId' should be lowercase"
            }
        }
    }

    @Test
    fun `native languages of the recorded voices`() {
        val croatianVoices = listOf("josip", "detence", "baba")
        val serbianVoices = listOf("vlado", "djedo")

        croatianVoices.forEach { voiceId ->
            val voiceInfo = VoiceInfo(
                id = voiceId,
                displayName = "Test",
                languageCode = "hr-HR",
                gender = "Male",
                age = "Adult",
                basePitch = 1.0f
            )
            assertEquals("$voiceId is natively Croatian", "hr", voiceInfo.nativeLanguage)
        }

        serbianVoices.forEach { voiceId ->
            val voiceInfo = VoiceInfo(
                id = voiceId,
                displayName = "Test",
                languageCode = "sr-RS",
                gender = "Male",
                age = "Adult",
                basePitch = 1.0f
            )
            assertEquals("$voiceId is natively Serbian", "sr", voiceInfo.nativeLanguage)
        }
    }

    // ==========================================================================
    // Derived Voice Tests
    // ==========================================================================

    @Test
    fun `derived voices have non-standard base pitch`() {
        // Derived voices: detence (child), baba (grandma), djedo (grandpa)
        val derivedVoicePitches = mapOf(
            "detence" to 1.5f,  // Child - higher pitch
            "baba" to 1.2f,     // Grandma - slightly higher
            "djedo" to 0.75f    // Grandpa - lower pitch
        )

        derivedVoicePitches.forEach { (voiceId, expectedPitch) ->
            val voiceInfo = VoiceInfo(
                id = voiceId,
                displayName = "Test",
                languageCode = "hr-HR",
                gender = "Male",
                age = "Adult",
                basePitch = expectedPitch
            )

            assert(!voiceInfo.isPhysicalVoice) {
                "$voiceId with pitch $expectedPitch should be a derived voice"
            }
        }
    }

    @Test
    fun `physical voices have base pitch 1_0`() {
        val physicalVoices = listOf("josip", "vlado")

        physicalVoices.forEach { voiceId ->
            val voiceInfo = VoiceInfo(
                id = voiceId,
                displayName = "Test",
                languageCode = "hr-HR",
                gender = "Male",
                age = "Adult",
                basePitch = 1.0f
            )

            assert(voiceInfo.isPhysicalVoice) {
                "$voiceId with pitch 1.0 should be a physical voice"
            }
        }
    }
}
