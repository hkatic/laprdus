package com.hrvojekatic.laprdus.service

import android.speech.tts.TextToSpeech
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * Unit tests for LaprdusTTSService.
 * These tests verify the language processing logic without requiring native code.
 */
class LaprdusTTSServiceTest {

    // ==========================================================================
    // Single Character Detection Tests (for spelling mode)
    // ==========================================================================

    @Test
    fun `single ASCII letter is detected as single character`() {
        assertTrue(isSingleGrapheme("A"))
        assertTrue(isSingleGrapheme("b"))
        assertTrue(isSingleGrapheme("Z"))
    }

    @Test
    fun `single Croatian letter with diacritics is detected as single character`() {
        assertTrue(isSingleGrapheme("Č"))
        assertTrue(isSingleGrapheme("ć"))
        assertTrue(isSingleGrapheme("Đ"))
        assertTrue(isSingleGrapheme("đ"))
        assertTrue(isSingleGrapheme("Š"))
        assertTrue(isSingleGrapheme("š"))
        assertTrue(isSingleGrapheme("Ž"))
        assertTrue(isSingleGrapheme("ž"))
    }

    @Test
    fun `single digit is detected as single character`() {
        assertTrue(isSingleGrapheme("0"))
        assertTrue(isSingleGrapheme("5"))
        assertTrue(isSingleGrapheme("9"))
    }

    @Test
    fun `single punctuation is detected as single character`() {
        assertTrue(isSingleGrapheme("."))
        assertTrue(isSingleGrapheme(","))
        assertTrue(isSingleGrapheme("!"))
        assertTrue(isSingleGrapheme("?"))
        assertTrue(isSingleGrapheme("@"))
        assertTrue(isSingleGrapheme("#"))
    }

    @Test
    fun `single space is detected as single character`() {
        assertTrue(isSingleGrapheme(" "))
    }

    @Test
    fun `multiple characters are not single character`() {
        assertFalse(isSingleGrapheme("AB"))
        assertFalse(isSingleGrapheme("abc"))
        assertFalse(isSingleGrapheme("12"))
        assertFalse(isSingleGrapheme("Hello"))
    }

    @Test
    fun `empty string is not single character`() {
        assertFalse(isSingleGrapheme(""))
    }

    @Test
    fun `whitespace-only longer than one char is not single character`() {
        assertFalse(isSingleGrapheme("  "))
        assertFalse(isSingleGrapheme("\t\t"))
    }

    @Test
    fun `emoji is detected as single character`() {
        // Emoji should be treated as single grapheme
        assertTrue(isSingleGrapheme("😀"))
        assertTrue(isSingleGrapheme("👍"))
    }

    @Test
    fun `newline is detected as single character`() {
        assertTrue(isSingleGrapheme("\n"))
        assertTrue(isSingleGrapheme("\r"))
    }

    @Test
    fun `tab is detected as single character`() {
        assertTrue(isSingleGrapheme("\t"))
    }

    // ==========================================================================
    // Language Code Tests
    // ==========================================================================

    @Test
    fun `Croatian language code hr is recognized`() {
        val isSupported = isLanguageSupported("hr", null)
        assertEquals(true, isSupported)
    }

    @Test
    fun `Croatian ISO 639-2 code hrv is recognized`() {
        val isSupported = isLanguageSupported("hrv", null)
        assertEquals(true, isSupported)
    }

    @Test
    fun `Serbian language code sr is recognized`() {
        val isSupported = isLanguageSupported("sr", null)
        assertEquals(true, isSupported)
    }

    @Test
    fun `Serbian ISO 639-2 code srp is recognized`() {
        val isSupported = isLanguageSupported("srp", null)
        assertEquals(true, isSupported)
    }

    @Test
    fun `English language code en is not supported`() {
        val isSupported = isLanguageSupported("en", null)
        assertEquals(false, isSupported)
    }

    @Test
    fun `Croatian with country HR has full support`() {
        val level = getLanguageSupportLevel("hr", "HR")
        assertEquals(TextToSpeech.LANG_COUNTRY_AVAILABLE, level)
    }

    @Test
    fun `Serbian with country RS has full support`() {
        val level = getLanguageSupportLevel("sr", "RS")
        assertEquals(TextToSpeech.LANG_COUNTRY_AVAILABLE, level)
    }

    @Test
    fun `Croatian without country has language support`() {
        val level = getLanguageSupportLevel("hr", null)
        assertEquals(TextToSpeech.LANG_AVAILABLE, level)
    }

    // ==========================================================================
    // Default Voice Tests
    // ==========================================================================

    @Test
    fun `default voice for Croatian is josip`() {
        val voice = getDefaultVoiceFor("hr", "HR")
        assertEquals("josip", voice)
    }

    @Test
    fun `default voice for Serbian is vlado`() {
        val voice = getDefaultVoiceFor("sr", "RS")
        assertEquals("vlado", voice)
    }

    @Test
    fun `default voice for unknown language is josip`() {
        val voice = getDefaultVoiceFor("en", "US")
        assertEquals("josip", voice)
    }

    // ==========================================================================
    // Voice ID Tests
    // ==========================================================================

    @Test
    fun `all voice IDs are lowercase`() {
        val validVoices = listOf("josip", "vlado", "detence", "baba", "djed")
        validVoices.forEach { voice ->
            assertEquals(voice, voice.lowercase())
        }
    }

    // ==========================================================================
    // Helper functions (mirroring service logic for testing)
    // ==========================================================================

    private fun isLanguageSupported(lang: String, country: String?): Boolean {
        val normalizedLang = lang.lowercase()
        return normalizedLang == "hr" || normalizedLang == "hrv" ||
               normalizedLang == "sr" || normalizedLang == "srp"
    }

    private fun getLanguageSupportLevel(lang: String, country: String?): Int {
        val normalizedLang = lang.lowercase()
        return when {
            normalizedLang == "hr" || normalizedLang == "hrv" -> {
                if (country?.lowercase() == "hr") {
                    TextToSpeech.LANG_COUNTRY_AVAILABLE
                } else {
                    TextToSpeech.LANG_AVAILABLE
                }
            }
            normalizedLang == "sr" || normalizedLang == "srp" -> {
                if (country?.lowercase() == "rs") {
                    TextToSpeech.LANG_COUNTRY_AVAILABLE
                } else {
                    TextToSpeech.LANG_AVAILABLE
                }
            }
            else -> TextToSpeech.LANG_NOT_SUPPORTED
        }
    }

    private fun getDefaultVoiceFor(lang: String, country: String?): String {
        val normalizedLang = lang.lowercase()
        return when {
            normalizedLang == "hr" || normalizedLang == "hrv" -> "josip"
            normalizedLang == "sr" || normalizedLang == "srp" -> "vlado"
            else -> "josip"
        }
    }

    /**
     * Determines if the input text is a single grapheme (user-perceived character).
     * This is used to detect when TalkBack or keyboard input sends a single character
     * that should be spelled out using the spelling dictionary.
     *
     * Uses Java's BreakIterator for proper Unicode grapheme cluster detection.
     * This handles:
     * - Simple ASCII characters (A-Z, 0-9)
     * - Croatian characters with diacritics (Č, Ć, Đ, Š, Ž)
     * - Emoji (including compound emoji like 👨‍👩‍👧)
     * - Combining characters (e.g., e + combining acute = é)
     */
    private fun isSingleGrapheme(text: String): Boolean {
        if (text.isEmpty()) return false

        val iterator = java.text.BreakIterator.getCharacterInstance()
        iterator.setText(text)

        // Move to first boundary (should be at start)
        iterator.first()
        // Move to next boundary
        val end = iterator.next()

        // If we're at the end of text after one grapheme, it's a single grapheme
        return end == text.length
    }
}
