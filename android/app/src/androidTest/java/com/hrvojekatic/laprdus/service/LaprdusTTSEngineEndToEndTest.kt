package com.hrvojekatic.laprdus.service

import android.content.Context
import android.os.Bundle
import android.speech.tts.TextToSpeech
import android.speech.tts.UtteranceProgressListener
import android.util.Log
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File
import java.util.Locale
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicInteger
import java.util.concurrent.atomic.AtomicReference

/**
 * End-to-end check of the Laprdus TTS engine through the public
 * [TextToSpeech] client API, exactly as TalkBack or any other app uses it:
 * bind to the engine by package name, wait for `onInit`, select Croatian,
 * and synthesize an utterance into a WAV file.
 *
 * Uses only public API and no DataStore or repository state, so it can run
 * next to the storage tests without interfering with them. The
 * instrumentation runs inside the app's own process, so the client binds to
 * [LaprdusTTSService] in-process and package visibility is not a concern.
 * Precondition: no other client should be driving the engine while this
 * runs (TalkBack is normally idle while the instrumentation has focus), and
 * each test selects its own language/voice before asserting.
 */
@RunWith(AndroidJUnit4::class)
class LaprdusTTSEngineEndToEndTest {

    private val context: Context = InstrumentationRegistry.getInstrumentation().targetContext

    private var tts: TextToSpeech? = null
    private lateinit var outputFile: File

    @Before
    fun setup() {
        outputFile = File(context.cacheDir, OUTPUT_FILE_NAME)
        outputFile.delete()
    }

    @After
    fun cleanup() {
        tts?.shutdown()
        tts = null
        outputFile.delete()
    }

    // ==========================================================================
    // Engine discovery and initialization
    // ==========================================================================

    @Test
    fun engineInitializesSuccessfully() {
        val engine = connectEngine()

        val engines = engine.engines.orEmpty().map { it.name }
        Log.i(TAG, "Installed TTS engines: $engines (default: ${engine.defaultEngine})")
        assertTrue(
            "the TextToSpeech client must list ${context.packageName} among installed engines; found $engines",
            engines.contains(context.packageName)
        )
    }

    @Test
    fun engineExposesAllFiveVoices() {
        val engine = connectEngine()
        assertTrue(engine.setLanguage(CROATIAN) >= 0)

        val voices = engine.voices
        assertNotNull("getVoices() must not fail for the connected engine", voices)
        val names = voices!!.map { it.name }
        for (expected in listOf("josip", "vlado", "detence", "baba", "djed")) {
            assertTrue("voice '$expected' missing from $names", names.contains(expected))
        }
    }

    // ==========================================================================
    // Language selection
    // ==========================================================================

    @Test
    fun croatianAndSerbianAreAvailable() {
        val engine = connectEngine()

        val croatian = engine.setLanguage(CROATIAN)
        assertTrue("setLanguage(hr-HR) must not report missing data or unsupported; got $croatian", croatian >= 0)

        val serbian = engine.isLanguageAvailable(SERBIAN)
        assertTrue("isLanguageAvailable(sr-RS) must be non-negative; got $serbian", serbian >= 0)
    }

    // ==========================================================================
    // Synthesis
    // ==========================================================================

    @Test
    fun synthesizeToFileProducesWavAudio() {
        val engine = connectEngine()
        assertTrue(engine.setLanguage(CROATIAN) >= 0)

        val done = CountDownLatch(1)
        val outcome = AtomicReference<String>("pending")
        engine.setOnUtteranceProgressListener(object : UtteranceProgressListener() {
            override fun onStart(utteranceId: String?) = Unit

            override fun onDone(utteranceId: String?) {
                if (utteranceId == UTTERANCE_ID) {
                    outcome.set("done")
                    done.countDown()
                }
            }

            @Deprecated("Deprecated in Java")
            override fun onError(utteranceId: String?) {
                if (utteranceId == UTTERANCE_ID) {
                    outcome.set("error")
                    done.countDown()
                }
            }

            override fun onError(utteranceId: String?, errorCode: Int) {
                if (utteranceId == UTTERANCE_ID) {
                    outcome.set("error($errorCode)")
                    done.countDown()
                }
            }
        })

        val queued = engine.synthesizeToFile("Dobar dan", Bundle(), outputFile, UTTERANCE_ID)
        assertEquals("synthesizeToFile must be accepted", TextToSpeech.SUCCESS, queued)

        assertTrue(
            "synthesis did not finish within $SYNTHESIS_TIMEOUT_SECONDS s",
            done.await(SYNTHESIS_TIMEOUT_SECONDS, TimeUnit.SECONDS)
        )
        assertEquals("utterance must complete without error", "done", outcome.get())

        assertTrue("output file must exist", outputFile.isFile)
        val size = outputFile.length()
        Log.i(TAG, "Synthesized ${outputFile.name}: $size bytes")
        assertTrue("output file should contain audio, was $size bytes", size > MIN_WAV_BYTES)

        // The framework writes a RIFF/WAVE header in front of the PCM data.
        val header = outputFile.inputStream().use { it.readNBytesCompat(12) }
        assertEquals("RIFF", String(header, 0, 4, Charsets.US_ASCII))
        assertEquals("WAVE", String(header, 8, 4, Charsets.US_ASCII))
    }

    // ==========================================================================
    // Helpers
    // ==========================================================================

    /** Binds to the Laprdus engine and waits for a successful onInit. */
    private fun connectEngine(): TextToSpeech {
        val initialized = CountDownLatch(1)
        val status = AtomicInteger(Int.MIN_VALUE)
        val engine = TextToSpeech(
            context,
            TextToSpeech.OnInitListener { result ->
                status.set(result)
                initialized.countDown()
            },
            context.packageName
        )
        tts = engine

        assertTrue(
            "onInit was not delivered within $INIT_TIMEOUT_SECONDS s",
            initialized.await(INIT_TIMEOUT_SECONDS, TimeUnit.SECONDS)
        )
        assertEquals("engine must initialize with SUCCESS", TextToSpeech.SUCCESS, status.get())
        return engine
    }

    /** Reads up to [count] bytes; InputStream.readNBytes needs API 33. */
    private fun java.io.InputStream.readNBytesCompat(count: Int): ByteArray {
        val buffer = ByteArray(count)
        var total = 0
        while (total < count) {
            val read = read(buffer, total, count - total)
            if (read < 0) break
            total += read
        }
        return buffer.copyOf(total)
    }

    private companion object {
        const val TAG = "LaprdusE2ETest"
        const val OUTPUT_FILE_NAME = "e2e.wav"
        const val UTTERANCE_ID = "e2e"
        const val INIT_TIMEOUT_SECONDS = 30L
        const val SYNTHESIS_TIMEOUT_SECONDS = 30L
        const val MIN_WAV_BYTES = 1000L
        val CROATIAN: Locale = Locale("hr", "HR")
        val SERBIAN: Locale = Locale("sr", "RS")
    }
}
