package com.hrvojekatic.laprdus.baselineprofile

import android.os.Bundle
import android.speech.tts.TextToSpeech
import android.speech.tts.UtteranceProgressListener
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit

/**
 * Speaks through the Laprdus engine from another process while no Laprdus
 * activity is visible, the way TalkBack does.
 *
 * On Android 17 the audio framework mutes playback of a background process
 * that has no visible activity and no foreground service ("background audio
 * hardening"), and the engine's process plays speak() audio itself. Run it
 * with `adb shell cmd audio set-enable-hardening throw` on an Android 17
 * device and check `adb shell dumpsys audio | grep AudioHardening` for the
 * engine's package: a muted engine fails the utterance or logs a hardening
 * entry.
 */
@RunWith(AndroidJUnit4::class)
class BackgroundSpeechTest {

    @Test
    fun speaksWithoutAVisibleActivity() {
        val context = InstrumentationRegistry.getInstrumentation().context
        val ready = CountDownLatch(1)
        var status = TextToSpeech.ERROR
        val tts = TextToSpeech(context, { status = it; ready.countDown() }, targetPackage)
        try {
            assertTrue("engine did not connect", ready.await(30, TimeUnit.SECONDS))
            assertEquals(TextToSpeech.SUCCESS, status)

            val done = CountDownLatch(1)
            var error: Int? = null
            var started = false
            tts.setOnUtteranceProgressListener(object : UtteranceProgressListener() {
                override fun onStart(utteranceId: String?) { started = true }
                override fun onDone(utteranceId: String?) { done.countDown() }
                @Deprecated("Deprecated in Java")
                override fun onError(utteranceId: String?) { error = TextToSpeech.ERROR; done.countDown() }
                override fun onError(utteranceId: String?, errorCode: Int) { error = errorCode; done.countDown() }
            })
            val result = tts.speak("Dobar dan. Laprdus govori iz pozadine.", TextToSpeech.QUEUE_FLUSH, Bundle(), "bg")
            assertEquals(TextToSpeech.SUCCESS, result)
            assertTrue("utterance did not finish", done.await(30, TimeUnit.SECONDS))
            assertTrue("utterance did not start", started)
            assertEquals("utterance failed", null, error)
        } finally {
            tts.shutdown()
        }
    }
}
