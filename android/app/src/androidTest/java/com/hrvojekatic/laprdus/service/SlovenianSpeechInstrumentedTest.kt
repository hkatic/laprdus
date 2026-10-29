package com.hrvojekatic.laprdus.service

import android.os.Bundle
import android.speech.tts.TextToSpeech
import android.speech.tts.UtteranceProgressListener
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import com.hrvojekatic.laprdus.data.SettingsRepository
import com.hrvojekatic.laprdus.tts.LaprdusTTS
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Assert.*
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.Locale
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicInteger

/** Exercises the same client/service/JNI path as keyboard echo and TalkBack.
 * Temporary preferences are restored, and user dictionaries are never edited.
 */
@RunWith(AndroidJUnit4::class)
class SlovenianSpeechInstrumentedTest {
    private val context = InstrumentationRegistry.getInstrumentation().targetContext
    private val settings = SettingsRepository.getInstance(context)
    private lateinit var saved: SettingsRepository.TTSSettings
    private var client: TextToSpeech? = null
    private val output by lazy { File(context.cacheDir, "slovenian-regression.wav") }

    @Before
    fun setup() = runBlocking {
        saved = settings.readSettingsNow()
        settings.setDefaultVoice("zvonko")
        settings.setUserDictionariesEnabled(false)
        settings.setForceLanguage(false)
        val ready = CountDownLatch(1)
        val result = AtomicInteger(Int.MIN_VALUE)
        client = TextToSpeech(context, { status ->
            result.set(status)
            ready.countDown()
        }, context.packageName)
        assertTrue(ready.await(30, TimeUnit.SECONDS))
        assertEquals(TextToSpeech.SUCCESS, result.get())
    }

    @After
    fun cleanup() = runBlocking {
        client?.shutdown()
        output.delete()
        if (::saved.isInitialized) {
            settings.setDefaultVoice(saved.defaultVoice)
            settings.setLanguage(saved.language)
            settings.setForceLanguage(saved.forceLanguage)
            settings.setUserDictionariesEnabled(saved.userDictionariesEnabled)
        }
    }

    @Test
    fun selectingSlovenianChangesSpeechAndKeyboardEcho() {
        val tts = client!!
        assertTrue(tts.setLanguage(Locale("hr", "HR")) >= 0)
        awaitLanguage("hr")
        val croatian = render("Dober dan, kako ste? Bil sem v Ljubljani.")

        assertTrue(tts.setLanguage(Locale("sl", "SI")) >= 0)
        assertEquals("zvonko-sl", tts.voice.name)
        awaitLanguage("sl")
        assertFalse(croatian.contentEquals(render("Dober dan, kako ste? Bil sem v Ljubljani.")))
        checkSlovenianCharacters()
    }

    @Test
    fun forcedSlovenianOverridesCroatianAndTurningItOffRestoresTheRequest() = runBlocking {
        // The client keeps asking for Croatian, as on a Croatian phone.
        assertTrue(client!!.setLanguage(Locale("hr", "HR")) >= 0)
        awaitLanguage("hr")
        settings.setLanguage("sl")
        settings.setForceLanguage(true)
        awaitLanguage("sl")
        checkSlovenianCharacters()

        settings.setForceLanguage(false)
        awaitLanguage("hr")
        assertArrayEquals(render("točka"), render("."))
        assertEquals("sl", settings.readSettingsNow().language)
    }

    private fun checkSlovenianCharacters() {
        for ((character, name) in listOf("." to "pika", "," to "vejica", "@" to "afna",
                "7" to "sedem", "8" to "osem", " " to "presledek", "w" to "dvojni ve")) {
            assertArrayEquals("keyboard echo of '$character' must say '$name'", render(name), render(character))
        }
    }

    private fun awaitLanguage(language: String) {
        val deadline = System.nanoTime() + TimeUnit.SECONDS.toNanos(10)
        do {
            render("Dober dan.")
            if (LaprdusTTS.service.spokenLanguage == language &&
                LaprdusTTS.service.currentVoiceId == "zvonko" &&
                !LaprdusTTS.service.userDictionariesEnabled) return
            Thread.sleep(25)
        } while (System.nanoTime() < deadline)
        fail("service did not apply language $language and the test settings")
    }

    /** Read the PCM data, ignoring only final silence (spelling cadence). */
    private fun render(text: String): ByteArray {
        val done = CountDownLatch(1)
        val result = AtomicInteger(Int.MIN_VALUE)
        client!!.setOnUtteranceProgressListener(object : UtteranceProgressListener() {
            override fun onStart(id: String?) = Unit
            override fun onDone(id: String?) { result.set(TextToSpeech.SUCCESS); done.countDown() }
            @Deprecated("Deprecated in Java")
            override fun onError(id: String?) { result.set(TextToSpeech.ERROR); done.countDown() }
        })
        output.delete()
        assertEquals(TextToSpeech.SUCCESS, client!!.synthesizeToFile(text, Bundle(), output, "sl-test"))
        assertTrue("synthesis timed out", done.await(30, TimeUnit.SECONDS))
        assertEquals(TextToSpeech.SUCCESS, result.get())
        val wav = output.readBytes()
        var offset = 12
        while (offset + 8 <= wav.size) {
            val size = ByteBuffer.wrap(wav, offset + 4, 4).order(ByteOrder.LITTLE_ENDIAN).int
            if (String(wav, offset, 4, Charsets.US_ASCII) == "data") {
                val start = offset + 8
                var end = start + size
                assertTrue(size > 0 && end <= wav.size)
                while (end >= start + 2 && wav[end - 1] == 0.toByte() && wav[end - 2] == 0.toByte()) end -= 2
                assertTrue("speech must contain non-silent audio", end > start)
                return wav.copyOfRange(start, end)
            }
            offset += 8 + size + (size % 2)
        }
        error("WAV contains no PCM data")
    }
}
