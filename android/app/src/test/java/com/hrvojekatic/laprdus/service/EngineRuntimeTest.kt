package com.hrvojekatic.laprdus.service

import com.hrvojekatic.laprdus.data.storage.StorageLogger
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertThrows
import org.junit.Assert.assertTrue
import org.junit.Assume.assumeTrue
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File
import java.io.IOException

/**
 * Unit tests for [EngineRuntime]: voice fallback, the fatal-failure decision
 * and the crash circuit breaker persisted in the marker file.
 *
 * Plain JVM: the native engine is a recording fake, the clock is a variable
 * and the crash marker lives in a temp folder, so every test is deterministic.
 */
class EngineRuntimeTest {

    @get:Rule
    val tempFolder = TemporaryFolder()

    /** Fake native engine: records every setVoice call and loads only the configured ids. */
    private class FakeSpeechEngine(vararg loadable: String) : SpeechEngine {
        val loadableVoices: MutableSet<String> = loadable.toMutableSet()
        val calls: MutableList<String> = mutableListOf()

        override fun setVoice(voiceId: String): Boolean {
            calls += voiceId
            return voiceId in loadableVoices
        }
    }

    /** Fake epoch-millis clock read by the runtime. */
    private var now: Long = START_TIME

    /** Marker path inside a fresh temp directory; the file itself does not exist yet. */
    private lateinit var markerFile: File

    @Before
    fun setup() {
        now = START_TIME
        markerFile = File(tempFolder.newFolder("engine"), "engine_crash_marker")
        assertFalse("marker must not exist before the test", markerFile.exists())
    }

    private fun runtime(
        engine: SpeechEngine,
        marker: File? = markerFile,
        maxCrashes: Int = EngineRuntime.DEFAULT_MAX_CRASHES,
        windowMillis: Long = EngineRuntime.DEFAULT_WINDOW_MILLIS,
        fallbackVoiceId: String = EngineRuntime.DEFAULT_FALLBACK_VOICE,
    ): EngineRuntime = EngineRuntime(
        engine = engine,
        crashMarkerFile = marker,
        logger = StorageLogger.None,
        clock = { now },
        maxCrashesPerWindow = maxCrashes,
        windowMillis = windowMillis,
        fallbackVoiceId = fallbackVoiceId,
    )

    private fun markerTimestamps(file: File = markerFile): List<Long> =
        if (file.isFile) {
            file.readLines().filter { it.isNotBlank() }.map { it.trim().toLong() }
        } else {
            emptyList()
        }

    private fun assertFatal(runtime: EngineRuntime, voiceId: String): LaprdusEngineUnavailableException =
        assertThrows(LaprdusEngineUnavailableException::class.java) { runtime.ensureReady(voiceId) }

    // ==========================================================================
    // Voice loading and fallback
    // ==========================================================================

    @Test
    fun `requested voice loads without fallback`() {
        val engine = FakeSpeechEngine("josip", "vlado")
        val runtime = runtime(engine)

        val result = runtime.ensureReady("vlado")

        assertEquals(EngineRuntime.ReadyResult.Ready("vlado", usedFallback = false), result)
        assertEquals(listOf("vlado"), engine.calls)
        assertFalse("a successful load must not create the crash marker", markerFile.exists())
    }

    @Test
    fun `requested voice fails and josip fallback loads`() {
        val engine = FakeSpeechEngine("josip")
        val runtime = runtime(engine)

        val result = runtime.ensureReady("vlado")

        assertEquals(EngineRuntime.ReadyResult.Ready("josip", usedFallback = true), result)
        assertEquals(listOf("vlado", "josip"), engine.calls)
        assertFalse("falling back is not fatal and must not touch the marker", markerFile.exists())
    }

    @Test
    fun `requesting the fallback voice itself tries it only once`() {
        val engine = FakeSpeechEngine()
        val runtime = runtime(engine)

        assertFatal(runtime, EngineRuntime.DEFAULT_FALLBACK_VOICE)

        assertEquals(listOf("josip"), engine.calls)
    }

    @Test
    fun `custom fallback voice id is honoured`() {
        val engine = FakeSpeechEngine("vlado")
        val runtime = runtime(engine, fallbackVoiceId = "vlado")

        val result = runtime.ensureReady("baba")

        assertEquals(EngineRuntime.ReadyResult.Ready("vlado", usedFallback = true), result)
        assertEquals(listOf("baba", "vlado"), engine.calls)
    }

    @Test
    fun `successful load leaves an existing marker untouched`() {
        markerFile.writeText("$START_TIME")
        val runtime = runtime(FakeSpeechEngine("josip"))

        runtime.ensureReady("josip")

        assertEquals(listOf(START_TIME), markerTimestamps())
    }

    // ==========================================================================
    // Fatal path
    // ==========================================================================

    @Test
    fun `both voices fail throws and records one timestamp`() {
        val engine = FakeSpeechEngine()
        val runtime = runtime(engine)

        val error = assertFatal(runtime, "vlado")

        assertEquals(listOf("vlado", "josip"), engine.calls)
        assertEquals(listOf(START_TIME), markerTimestamps())
        assertTrue("message should name the requested voice", error.message!!.contains("vlado"))
        assertTrue("message should name the fallback voice", error.message!!.contains("josip"))
    }

    @Test
    fun `marker parent directory is created on demand`() {
        val marker = File(tempFolder.root, "nested/deeper/engine_crash_marker")
        val runtime = runtime(FakeSpeechEngine(), marker = marker)

        assertFatal(runtime, "vlado")

        assertTrue("marker should be written even if its directory did not exist", marker.isFile)
        assertEquals(listOf(START_TIME), markerTimestamps(marker))
    }

    // ==========================================================================
    // Circuit breaker
    // ==========================================================================

    @Test
    fun `breaker allows exactly DEFAULT_MAX_CRASHES throws then degrades`() {
        val engine = FakeSpeechEngine()
        val runtime = runtime(engine)

        repeat(EngineRuntime.DEFAULT_MAX_CRASHES) {
            assertFatal(runtime, "vlado")
            now += 1_000
        }
        val result = runtime.ensureReady("vlado")

        assertEquals(EngineRuntime.ReadyResult.Unavailable, result)
        assertEquals(
            "every attempt, including the degraded one, is recorded",
            EngineRuntime.DEFAULT_MAX_CRASHES + 1,
            markerTimestamps().size
        )
        // The degraded attempt still tried both voices before giving up.
        assertEquals((EngineRuntime.DEFAULT_MAX_CRASHES + 1) * 2, engine.calls.size)
    }

    @Test
    fun `breaker keeps degrading while the window is open`() {
        val runtime = runtime(FakeSpeechEngine())
        repeat(EngineRuntime.DEFAULT_MAX_CRASHES) { assertFatal(runtime, "vlado") }

        repeat(3) {
            now += 60_000
            assertEquals(EngineRuntime.ReadyResult.Unavailable, runtime.ensureReady("vlado"))
        }
    }

    @Test
    fun `advancing the clock beyond the window resets the breaker`() {
        val runtime = runtime(FakeSpeechEngine())
        repeat(EngineRuntime.DEFAULT_MAX_CRASHES) { assertFatal(runtime, "vlado") }
        assertEquals(EngineRuntime.ReadyResult.Unavailable, runtime.ensureReady("vlado"))

        now = START_TIME + EngineRuntime.DEFAULT_WINDOW_MILLIS + 1

        assertFatal(runtime, "vlado")
        assertEquals("expired timestamps are pruned from the marker", listOf(now), markerTimestamps())
    }

    @Test
    fun `timestamp exactly at the window edge still counts`() {
        markerFile.writeText(List(EngineRuntime.DEFAULT_MAX_CRASHES) { START_TIME }.joinToString("\n"))
        val runtime = runtime(FakeSpeechEngine())

        now = START_TIME + EngineRuntime.DEFAULT_WINDOW_MILLIS

        assertEquals(EngineRuntime.ReadyResult.Unavailable, runtime.ensureReady("vlado"))
    }

    @Test
    fun `custom maxCrashesPerWindow is honoured`() {
        val runtime = runtime(FakeSpeechEngine(), maxCrashes = 1)

        assertFatal(runtime, "vlado")
        assertEquals(EngineRuntime.ReadyResult.Unavailable, runtime.ensureReady("vlado"))
    }

    @Test
    fun `future timestamps from a clock that went backwards are ignored`() {
        markerFile.writeText(List(EngineRuntime.DEFAULT_MAX_CRASHES) { START_TIME + 5_000 }.joinToString("\n"))
        val runtime = runtime(FakeSpeechEngine())

        assertFatal(runtime, "vlado")

        assertEquals(listOf(START_TIME), markerTimestamps())
    }

    @Test
    fun `non numeric lines in the marker are ignored`() {
        markerFile.writeText("garbage\n\n   \nnot-a-number\n")
        val runtime = runtime(FakeSpeechEngine())

        assertFatal(runtime, "vlado")

        assertEquals(listOf(START_TIME), markerTimestamps())
    }

    @Test
    fun `recordFatalAttempt counts attempts inside the window`() {
        val runtime = runtime(FakeSpeechEngine())

        assertEquals(1, runtime.recordFatalAttempt())
        now += 1_000
        assertEquals(2, runtime.recordFatalAttempt())
        now += 1_000
        assertEquals(3, runtime.recordFatalAttempt())

        now += EngineRuntime.DEFAULT_WINDOW_MILLIS + 1
        assertEquals("window expired: count restarts", 1, runtime.recordFatalAttempt())
        assertEquals(listOf(now), markerTimestamps())
    }

    // ==========================================================================
    // Marker clearing
    // ==========================================================================

    @Test
    fun `onSynthesisSucceeded deletes the marker`() {
        val runtime = runtime(FakeSpeechEngine())
        assertFatal(runtime, "vlado")
        assertTrue(markerFile.isFile)

        runtime.onSynthesisSucceeded()

        assertFalse("marker must be gone after speech worked", markerFile.exists())
    }

    @Test
    fun `onSynthesisSucceeded resets the crash count`() {
        val runtime = runtime(FakeSpeechEngine())
        repeat(EngineRuntime.DEFAULT_MAX_CRASHES) { assertFatal(runtime, "vlado") }

        runtime.onSynthesisSucceeded()

        // Would have been Unavailable without the reset.
        assertFatal(runtime, "vlado")
        assertEquals(listOf(START_TIME), markerTimestamps())
    }

    @Test
    fun `onSynthesisSucceeded without a marker is a no-op`() {
        val runtime = runtime(FakeSpeechEngine("josip"))

        runtime.onSynthesisSucceeded()

        assertFalse(markerFile.exists())
    }

    // ==========================================================================
    // Storage failures never crash-loop
    // ==========================================================================

    @Test
    fun `unwritable marker degrades instead of crashing`() {
        val readOnlyDir = tempFolder.newFolder("readonly")
        readOnlyDir.setWritable(false)
        // Root ignores permission bits and Windows does not enforce a read-only
        // attribute on directories; probe instead of trusting canWrite().
        val probe = File(readOnlyDir, "probe")
        val enforced = try {
            probe.createNewFile()
            false
        } catch (_: IOException) {
            true
        }
        if (!enforced) {
            probe.delete()
            readOnlyDir.setWritable(true)
        }
        assumeTrue("read-only directory is not enforced here (root or Windows?)", enforced)
        val marker = File(readOnlyDir, "engine_crash_marker")
        val engine = FakeSpeechEngine()
        val runtime = runtime(engine, marker = marker)

        try {
            assertEquals(Int.MAX_VALUE, runtime.recordFatalAttempt())
            assertEquals(EngineRuntime.ReadyResult.Unavailable, runtime.ensureReady("vlado"))
            assertFalse("nothing could be written", marker.exists())
            assertEquals(listOf("vlado", "josip"), engine.calls)
        } finally {
            readOnlyDir.setWritable(true)
        }
    }

    @Test
    fun `null marker throws on every attempt`() {
        val engine = FakeSpeechEngine()
        val runtime = runtime(engine, marker = null)

        repeat(EngineRuntime.DEFAULT_MAX_CRASHES + 2) {
            assertFatal(runtime, "vlado")
            now += 1_000
        }

        assertEquals("without a marker every attempt counts as the first", 1, runtime.recordFatalAttempt())
        assertFalse("no marker file may appear anywhere", markerFile.exists())
        runtime.onSynthesisSucceeded() // must not throw
    }

    companion object {
        private const val START_TIME = 1_000_000L
    }
}
