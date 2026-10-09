package com.hrvojekatic.laprdus.tts

import org.junit.Assert.assertEquals
import org.junit.Test

/**
 * The rate and pitch of a request from another app (TalkBack) scale the
 * user's own Laprdus setting, unless the setting is forced.
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
}
