package com.hrvojekatic.laprdus.tts

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNotSame
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith

/**
 * The app's screens and the speech service have engines of their own, so a
 * voice, rate or dictionary set by one never reaches the other (before, one
 * shared engine let every TalkBack request change the preview's rate and a
 * voice chosen in the app drop TalkBack's user dictionaries).
 */
@RunWith(AndroidJUnit4::class)
class SeparateEnginesInstrumentedTest {

    private val assets = InstrumentationRegistry.getInstrumentation().targetContext.assets

    @Test
    fun appAndServiceHaveTheirOwnEngines() {
        assertNotSame(LaprdusTTS.app, LaprdusTTS.service)
    }

    @Test
    fun aVoiceLoadedByOneEngineDoesNotChangeTheOther() {
        val app = LaprdusTTS.app
        val service = LaprdusTTS.service
        assertTrue(app.setVoice("zvonko", assets))
        assertTrue(service.setVoice("stojan", assets))
        assertEquals("zvonko", app.currentVoiceId)
        assertEquals("stojan", service.currentVoiceId)
    }

    @Test
    fun aRateSetOnOneEngineDoesNotChangeTheOther() {
        val app = LaprdusTTS.app
        val service = LaprdusTTS.service
        assertTrue(app.setVoice("zvonko", assets))
        assertTrue(service.setVoice("zvonko", assets))
        val text = "Dobar dan, ovo je proba brzine govora."

        app.speed = 1.0f
        service.speed = 1.0f
        val normal = app.synthesize(text)
        assertNotNull(normal)

        // TalkBack asks the service for twice the rate; the app's preview
        // still speaks at its own.
        service.speed = 2.0f
        val fast = service.synthesize(text)
        val preview = app.synthesize(text)
        assertNotNull(fast)
        assertNotNull(preview)
        assertTrue("the service speaks faster", fast!!.size < normal!!.size * 0.75)
        assertEquals("the app keeps its rate", normal.size, preview!!.size)
        service.speed = 1.0f
    }

    @Test
    fun settingsSurviveLoadingAVoice() {
        // The service applies its settings before it loads its first voice,
        // so loading a voice must keep them.
        val app = LaprdusTTS.app
        val saved = app.sentencePause
        app.sentencePause = 777
        assertTrue(app.setVoice("zvonko", assets))
        assertEquals(777, app.sentencePause)
        app.sentencePause = saved
    }
}
