package com.hrvojekatic.laprdus.service

import android.speech.tts.TextToSpeech
import com.hrvojekatic.laprdus.tts.Language
import com.hrvojekatic.laprdus.tts.VoiceInfo
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * Unit tests for LaprdusTTSService.
 * These tests verify the language and voice selection logic without
 * requiring native code.
 */
class LaprdusTTSServiceTest {

    /** The registry's 18 voices, as the engine reports them. */
    private val VOICES: List<VoiceInfo> = listOf(
        VoiceInfo("josip", "Josip", "hr-HR", "Male", "Adult", 1.0f, null, false),
        VoiceInfo("vlado", "Vlado", "sr-RS", "Male", "Adult", 1.0f, null, false),
        VoiceInfo("detence", "Detence", "hr-HR", "Male", "Child", 1.5f, "josip", false),
        VoiceInfo("baba", "Baba", "hr-HR", "Female", "Senior", 1.2f, "josip", false),
        VoiceInfo("djedo", "\u0110edo", "sr-RS", "Male", "Senior", 0.75f, "vlado", false),
        VoiceInfo("zvonko", "Zvonko", "hr-HR", "Male", "Adult", 1.0f, null, true),
        VoiceInfo("stojan", "Stojan", "sr-RS", "Male", "Adult", 1.0f, null, true),
        VoiceInfo("mirsad", "Mirsad", "bs-BA", "Male", "Adult", 1.0f, null, true),
        VoiceInfo("orguljas", "Orgulja\u0161 - Vila Velebita", "hr-HR", "Male", "Adult", 1.0f, "zvonko", true),
        VoiceInfo("dalmacija_klapa", "Dalmacija Klapa", "hr-HR", "Male", "Adult", 1.0f, "zvonko", true),
        VoiceInfo("trubac", "Truba\u010d - Kre\u0107e se la\u0111a francuska", "sr-RS", "Male", "Adult", 1.0f, "stojan", true),
        VoiceInfo("harmonikas", "Harmonika\u0161 - Kre\u0107e se la\u0111a francuska", "sr-RS", "Male", "Adult", 1.0f, "stojan", true),
        VoiceInfo("sevdalija", "Sevdalija - Kad ja po\u0111oh na Bemba\u0161u", "bs-BA", "Male", "Adult", 1.0f, "mirsad", true),
        VoiceInfo("sazlija", "Sazlija - Kad ja po\u0111oh na Bemba\u0161u", "bs-BA", "Male", "Adult", 1.0f, "mirsad", true),
        VoiceInfo("lika_velebit", "Lika Velebit", "hr-HR", "Male", "Adult", 1.0f, "zvonko", true),
        VoiceInfo("solunac", "Solunac", "sr-RS", "Male", "Adult", 1.0f, "stojan", true),
        VoiceInfo("sarajevo_bembasa", "Sarajevo Bemba\u0161a", "bs-BA", "Male", "Adult", 1.0f, "mirsad", true),
        VoiceInfo("becarac", "Be\u0107arac", "hr-HR", "Male", "Adult", 1.0f, "zvonko", true)
    )

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
        assertEquals("hr", Language.fromTag("hr"))
    }

    @Test
    fun `Croatian ISO 639-2 code hrv is recognized`() {
        assertEquals("hr", Language.fromTag("hrv"))
    }

    @Test
    fun `Serbian language code sr is recognized`() {
        assertEquals("sr", Language.fromTag("sr"))
    }

    @Test
    fun `Serbian ISO 639-2 code srp is recognized`() {
        assertEquals("sr", Language.fromTag("srp"))
    }

    @Test
    fun `Bosnian codes bs and bos are recognized`() {
        assertEquals("bs", Language.fromTag("bs"))
        assertEquals("bs", Language.fromTag("bos"))
    }

    @Test
    fun `English language code en is not a native language`() {
        assertNull(Language.fromTag("en"))
    }

    @Test
    fun `unknown language still reports availability so engine is never skipped`() {
        // TTS settings and screen readers probe with the device locale;
        // LANG_NOT_SUPPORTED would make them disable the engine entirely
        val level = getLanguageSupportLevel("en", "US")
        assertEquals(TextToSpeech.LANG_AVAILABLE, level)
    }

    @Test
    fun `Croatian with country HR has full support`() {
        val level = getLanguageSupportLevel("hr", "HR")
        assertEquals(TextToSpeech.LANG_COUNTRY_AVAILABLE, level)
    }

    @Test
    fun `Croatian with ISO3 country HRV has full support`() {
        // The Android framework passes ISO3 codes to the engine boundary
        val level = getLanguageSupportLevel("hrv", "HRV")
        assertEquals(TextToSpeech.LANG_COUNTRY_AVAILABLE, level)
    }

    @Test
    fun `Serbian with country RS has full support`() {
        val level = getLanguageSupportLevel("sr", "RS")
        assertEquals(TextToSpeech.LANG_COUNTRY_AVAILABLE, level)
    }

    @Test
    fun `Serbian with ISO3 country SRB has full support`() {
        val level = getLanguageSupportLevel("srp", "SRB")
        assertEquals(TextToSpeech.LANG_COUNTRY_AVAILABLE, level)
    }

    @Test
    fun `Croatian without country has language support`() {
        val level = getLanguageSupportLevel("hr", null)
        assertEquals(TextToSpeech.LANG_AVAILABLE, level)
    }

    // ==========================================================================
    // System voice names: <id>-<language>
    // ==========================================================================

    @Test
    fun `there is one system voice per voice and language`() {
        val names = VoiceName.all(VOICES).map { it.name }
        assertEquals(VOICES.size * 3, names.size)
        assertEquals(54, names.size)
        assertTrue("zvonko-hr" in names)
        assertTrue("zvonko-sr" in names)
        assertTrue("zvonko-bs" in names)
        assertTrue("djedo-sr" in names)
        assertTrue("lika_velebit-bs" in names)
        assertEquals("no duplicates", names.size, names.toSet().size)
    }

    @Test
    fun `a voice name is parsed into voice and language`() {
        assertEquals(VoiceName("zvonko", "sr"), VoiceName.parse("zvonko-sr", VOICES))
        assertEquals(VoiceName("djedo", "sr"), VoiceName.parse("djedo-sr", VOICES))
        assertEquals(VoiceName("lika_velebit", "bs"), VoiceName.parse("lika_velebit-bs", VOICES))
        assertEquals(VoiceName("josip", "bs"), VoiceName.parse("josip-bs", VOICES))
        assertEquals("zvonko-sr", VoiceName("zvonko", "sr").name)
    }

    @Test
    fun `a bare voice id means the voice in its native language`() {
        assertEquals(VoiceName("zvonko", "hr"), VoiceName.parse("zvonko", VOICES))
        assertEquals(VoiceName("vlado", "sr"), VoiceName.parse("vlado", VOICES))
        assertEquals(VoiceName("mirsad", "bs"), VoiceName.parse("mirsad", VOICES))
    }

    @Test
    fun `the voice ids of earlier versions are accepted`() {
        assertEquals(VoiceName("djedo", "sr"), VoiceName.parse("djed", VOICES))
        assertEquals(VoiceName("djedo", "hr"), VoiceName.parse("djed-hr", VOICES))
        assertEquals(VoiceName("lika_velebit", "hr"), VoiceName.parse("pjevac", VOICES))
        assertEquals(VoiceName("sarajevo_bembasa", "sr"), VoiceName.parse("solist-sr", VOICES))
    }

    @Test
    fun `names that are not a Laprdus voice are rejected`() {
        assertNull(VoiceName.parse(null, VOICES))
        assertNull(VoiceName.parse("", VOICES))
        assertNull(VoiceName.parse("nobody", VOICES))
        assertNull(VoiceName.parse("nobody-hr", VOICES))
        assertNull(VoiceName.parse("zvonko-en", VOICES))
        assertNull(VoiceName.parse("-hr", VOICES))
    }

    // ==========================================================================
    // Which voice and language a request gets
    // ==========================================================================

    @Test
    fun `default voice for every language is the configured voice`() {
        val selection = SpeechSelection("zvonko")
        assertEquals("zvonko-hr", selection.defaultVoiceNameFor("hr"))
        assertEquals("zvonko-sr", selection.defaultVoiceNameFor("srp"))
        assertEquals("zvonko-bs", selection.defaultVoiceNameFor("bos"))
        // An unspoken language gets the voice in Croatian
        assertEquals("zvonko-hr", selection.defaultVoiceNameFor("en"))

        selection.setConfiguredVoice("josip")
        assertEquals("josip-sr", selection.defaultVoiceNameFor("sr"))
    }

    @Test
    fun `loading a language keeps the configured voice`() {
        val selection = SpeechSelection("josip", "hr")

        assertEquals("sr", selection.requestLanguage("srp"))
        assertEquals("josip", selection.voiceId)
        assertEquals("sr", selection.requestedLanguage)

        assertEquals("bs", selection.requestLanguage("bos"))
        assertEquals("josip", selection.voiceId)
        assertEquals("bs", selection.requestedLanguage)
    }

    @Test
    fun `loading an unspoken language keeps the current language`() {
        val selection = SpeechSelection("zvonko", "sr")
        assertNull(selection.requestLanguage("eng"))
        assertEquals("sr", selection.requestedLanguage)
        assertEquals("zvonko", selection.voiceId)
    }

    @Test
    fun `loading a voice name takes its language and keeps the configured voice`() {
        val selection = SpeechSelection("josip", "hr")

        assertTrue(selection.requestVoiceName("zvonko-sr", VOICES))
        assertEquals("josip", selection.voiceId)
        assertEquals("sr", selection.requestedLanguage)

        assertFalse(selection.requestVoiceName("nobody-sr", VOICES))
        assertEquals("sr", selection.requestedLanguage)
    }

    @Test
    fun `a request speaks its own locale`() {
        val selection = SpeechSelection("zvonko", "hr")
        assertEquals("sr", selection.languageFor("srp", forceLanguage = false, forcedLanguage = "bs"))
        assertEquals("bs", selection.languageFor("bos", forceLanguage = false, forcedLanguage = "hr"))
        // A request without a spoken locale gets the last requested language
        selection.requestLanguage("sr")
        assertEquals("sr", selection.languageFor("eng", forceLanguage = false, forcedLanguage = "bs"))
        assertEquals("sr", selection.languageFor(null, forceLanguage = false, forcedLanguage = "bs"))
    }

    @Test
    fun `force language speaks the Laprdus setting whatever the app asks`() {
        val selection = SpeechSelection("zvonko", "hr")
        assertEquals("bs", selection.languageFor("srp", forceLanguage = true, forcedLanguage = "bs"))
        assertEquals("bs", selection.languageFor("hrv", forceLanguage = true, forcedLanguage = "bs"))
        assertEquals("bs", selection.currentLanguage(forceLanguage = true, forcedLanguage = "bs"))
        assertEquals("hr", selection.currentLanguage(forceLanguage = false, forcedLanguage = "bs"))
        // A broken setting falls back to Croatian rather than to the app's language
        assertEquals("hr", selection.languageFor("srp", forceLanguage = true, forcedLanguage = "xx"))
    }

    @Test
    fun `the configured voice is stored by its canonical id`() {
        val selection = SpeechSelection("djed")
        assertEquals("djedo", selection.voiceId)
        selection.setConfiguredVoice("pevac")
        assertEquals("solunac", selection.voiceId)
    }

    // ==========================================================================
    // Voice ID Tests
    // ==========================================================================

    @Test
    fun `all voice IDs are lowercase ASCII`() {
        VOICES.forEach { voice ->
            assertEquals(voice.id, voice.id.lowercase())
            assertTrue(voice.id, voice.id.all { it in 'a'..'z' || it == '_' })
        }
    }

    // ==========================================================================
    // Helper functions (mirroring service logic for testing)
    // ==========================================================================

    private fun getLanguageSupportLevel(lang: String, country: String?): Int {
        val normalizedLang = lang.lowercase()
        val normalizedCountry = country?.lowercase() ?: ""
        return when (normalizedLang) {
            "hr", "hrv" -> {
                if (normalizedCountry == "hr" || normalizedCountry == "hrv") {
                    TextToSpeech.LANG_COUNTRY_AVAILABLE
                } else {
                    TextToSpeech.LANG_AVAILABLE
                }
            }
            "sr", "srp" -> {
                if (normalizedCountry == "rs" || normalizedCountry == "srb") {
                    TextToSpeech.LANG_COUNTRY_AVAILABLE
                } else {
                    TextToSpeech.LANG_AVAILABLE
                }
            }
            else -> TextToSpeech.LANG_AVAILABLE
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
