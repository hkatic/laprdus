package com.hrvojekatic.laprdus.tts

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * The rate and pitch of a request from another app (TalkBack) scale the
 * user's own Laprdus setting, unless the setting is forced; the voice ids
 * of earlier versions map to the renamed voices.
 */
class VoiceInfoTest {

    @Test
    fun `the app's rate scales the Laprdus rate`() {
        // TalkBack at its normal rate: the Laprdus rate alone
        assertEquals(1.5f, VoiceInfo.requestValue(1.5f, 1.0f, false, "zvonko"), 1e-6f)
        // TalkBack twice as fast: twice the Laprdus rate
        assertEquals(3.0f, VoiceInfo.requestValue(1.5f, 2.0f, false, "zvonko"), 1e-6f)
        // The Laprdus rate at its default leaves the app's rate as it was
        assertEquals(0.75f, VoiceInfo.requestValue(1.0f, 0.75f, false, "zvonko"), 1e-6f)
    }

    @Test
    fun `a forced setting ignores the app`() {
        assertEquals(1.5f, VoiceInfo.requestValue(1.5f, 2.0f, true, "zvonko"), 1e-6f)
        assertEquals(0.5f, VoiceInfo.requestValue(0.5f, 3.0f, true, "josip"), 1e-6f)
    }

    @Test
    fun `the product stays in the voice's range`() {
        assertEquals(4.0f, VoiceInfo.requestValue(3.0f, 2.0f, false, "zvonko"), 1e-6f)
        assertEquals(2.0f, VoiceInfo.requestValue(1.5f, 2.0f, false, "josip"), 1e-6f)
        assertEquals(0.5f, VoiceInfo.requestValue(0.5f, 0.5f, false, "vlado"), 1e-6f)
        assertEquals(0.25f, VoiceInfo.requestValue(0.25f, 0.1f, false, "stojan"), 1e-6f)
    }

    @Test
    fun `a broken request falls back to the normal rate`() {
        assertEquals(1.0f, VoiceInfo.requestValue(1.0f, Float.NaN, false, "zvonko"), 1e-6f)
    }

    // ==========================================================================
    // Voice ids
    // ==========================================================================

    @Test
    fun `old voice ids normalize to the renamed voices`() {
        assertEquals("djedo", VoiceInfo.normalizeId("djed"))
        assertEquals("detence", VoiceInfo.normalizeId("dijete"))
        assertEquals("baba", VoiceInfo.normalizeId("baka"))
        assertEquals("dalmacija_klapa", VoiceInfo.normalizeId("klapa"))
        assertEquals("lika_velebit", VoiceInfo.normalizeId("pjevac"))
        assertEquals("solunac", VoiceInfo.normalizeId("pevac"))
        assertEquals("sarajevo_bembasa", VoiceInfo.normalizeId("solist"))
    }

    @Test
    fun `new voice ids and unknown ids normalize to themselves`() {
        for (id in listOf("josip", "vlado", "detence", "baba", "djedo", "zvonko", "stojan", "mirsad") +
            VoiceInfo.SINGING_VOICE_IDS
        ) {
            assertEquals(id, VoiceInfo.normalizeId(id))
        }
        assertEquals("nobody", VoiceInfo.normalizeId("nobody"))
        // Normalization is idempotent
        assertEquals(VoiceInfo.normalizeId("djed"), VoiceInfo.normalizeId(VoiceInfo.normalizeId("djed")))
    }

    @Test
    fun `formant voices are known by their new ids and their old ones`() {
        for (id in listOf("zvonko", "stojan", "mirsad", "orguljas", "dalmacija_klapa", "trubac",
            "harmonikas", "sevdalija", "sazlija", "lika_velebit", "solunac", "sarajevo_bembasa", "becarac")
        ) {
            assertTrue(id, VoiceInfo.isFormantVoice(id))
            assertEquals(VoiceInfo.FORMANT_RANGE, VoiceInfo.rangeFor(id))
        }
        for (old in listOf("klapa", "pjevac", "pevac", "solist")) {
            assertTrue(old, VoiceInfo.isFormantVoice(old))
        }
        for (id in listOf("josip", "vlado", "detence", "baba", "djedo", "djed", "dijete", "baka", null)) {
            assertFalse(id ?: "null", VoiceInfo.isFormantVoice(id))
            assertEquals(VoiceInfo.RECORDED_RANGE, VoiceInfo.rangeFor(id))
        }
    }

    @Test
    fun `the kind of a voice comes from the registry fields`() {
        val zvonko = VoiceInfo("zvonko", "Zvonko", "hr-HR", "Male", "Adult", 1.0f, null, isFormant = true)
        val klapa = VoiceInfo("dalmacija_klapa", "Dalmacija Klapa", "hr-HR", "Male", "Adult", 1.0f, "zvonko", isFormant = true)
        val djedo = VoiceInfo("djedo", "Đedo", "sr-RS", "Male", "Senior", 0.75f, "vlado", isFormant = false)
        val josip = VoiceInfo("josip", "Josip", "hr-HR", "Male", "Adult", 1.0f)

        assertTrue(zvonko.isFormantVoice)
        assertFalse(zvonko.isSingingVoice)
        assertTrue(klapa.isFormantVoice)
        assertTrue(klapa.isSingingVoice)
        assertFalse(djedo.isFormantVoice)
        assertFalse(djedo.isSingingVoice)
        assertFalse(djedo.isPhysicalVoice)
        assertFalse(josip.isFormantVoice)
        assertTrue(josip.isPhysicalVoice)
    }

    @Test
    fun `the native language is only the voice's default`() {
        assertEquals("hr", VoiceInfo("josip", "Josip", "hr-HR", "Male", "Adult", 1.0f).nativeLanguage)
        assertEquals("sr", VoiceInfo("vlado", "Vlado", "sr-RS", "Male", "Adult", 1.0f).nativeLanguage)
        assertEquals("bs", VoiceInfo("mirsad", "Mirsad", "bs-BA", "Male", "Adult", 1.0f, null, true).nativeLanguage)
    }
}
