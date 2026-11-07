package com.hrvojekatic.laprdus.service

import android.os.SystemClock
import android.util.Log
import com.hrvojekatic.laprdus.BuildConfig
import java.io.File

/**
 * What clients, TalkBack above all, ask the service to speak, with how long
 * the engine took. Debug builds only: it is how the instrumented tests hear
 * exactly what TalkBack says for every control, in which order, and how fast
 * speech starts after a focus change.
 *
 * Release builds record nothing: every method returns at once when
 * [BuildConfig.DEBUG] is false, so R8 removes the bodies and no utterance
 * text is kept or logged. Debug builds also write each entry to logcat
 * (tag `LaprdusSpeech`) while the flag file `files/debug/log_speech` exists in
 * device-protected storage, for sessions driven by hand:
 * `adb shell run-as com.hrvojekatic.laprdus touch /data/user_de/0/com.hrvojekatic.laprdus/files/debug/log_speech`.
 */
object SpeechRequestLog {

    const val LOG_TAG = "LaprdusSpeech"
    private const val CAPACITY = 512

    /** One request, as the service received and rendered it. */
    data class Entry(
        /** The text the client asked for. */
        val text: String,
        /** The language the engine spoke ("hr", "sl", ...). */
        val language: String,
        /** Whether it was read character by character. */
        val spelled: Boolean,
        /** [SystemClock.elapsedRealtimeNanos] when the request arrived. */
        val receivedAtNanos: Long,
        /** Time the engine spent synthesizing, in nanoseconds. */
        val synthesisNanos: Long,
        /** From arrival to the first audio handed to the framework, in nanoseconds. */
        val firstAudioNanos: Long,
        /** Whether the client stopped it before all audio was handed over. */
        val interrupted: Boolean,
        /** Length of the speech, ms. */
        val audioMillis: Long = 0,
    ) {
        /** When the speech has been heard to its end, [SystemClock.elapsedRealtimeNanos]. */
        val endNanos: Long get() = receivedAtNanos + firstAudioNanos + audioMillis * 1_000_000
    }

    /** A stop request (TalkBack interrupting speech), [SystemClock.elapsedRealtimeNanos]. */
    data class Stop(val atNanos: Long)

    private val lock = Any()
    private val entries = ArrayDeque<Entry>()
    private val stops = ArrayDeque<Stop>()

    @Volatile
    private var logFile: File? = null

    /** Where the logcat flag file lives; set by the service. */
    fun setLogFlagFile(file: File) {
        if (!BuildConfig.DEBUG) return
        logFile = file
    }

    fun now(): Long = SystemClock.elapsedRealtimeNanos()

    /**
     * When the request being synthesized now arrived, or 0. An entry is
     * recorded when its audio has been handed over, seconds after it arrived
     * for a long text; this tells a waiting test that speech is under way.
     */
    @Volatile
    var inFlightSinceNanos: Long = 0L
        private set

    fun begin(receivedAtNanos: Long) {
        if (!BuildConfig.DEBUG) return
        inFlightSinceNanos = receivedAtNanos
    }

    /** The request has been rendered, recorded or not (an error). */
    fun end() {
        if (!BuildConfig.DEBUG) return
        inFlightSinceNanos = 0L
    }

    fun record(entry: Entry) {
        if (!BuildConfig.DEBUG) return
        inFlightSinceNanos = 0L
        synchronized(lock) {
            if (entries.size == CAPACITY) entries.removeFirst()
            entries.addLast(entry)
        }
        if (logFile?.exists() == true) {
            Log.i(
                LOG_TAG,
                "[${entry.language}${if (entry.spelled) ", spelled" else ""}] " +
                    "first audio ${entry.firstAudioNanos / 1_000_000} ms, " +
                    "synthesis ${entry.synthesisNanos / 1_000_000} ms" +
                    (if (entry.interrupted) ", interrupted" else "") +
                    ": ${entry.text}"
            )
        }
    }

    fun recordStop() {
        if (!BuildConfig.DEBUG) return
        val stop = Stop(now())
        synchronized(lock) {
            if (stops.size == CAPACITY) stops.removeFirst()
            stops.addLast(stop)
        }
        if (logFile?.exists() == true) Log.i(LOG_TAG, "stop")
    }

    /** Every entry recorded so far, oldest first. */
    fun entries(): List<Entry> = synchronized(lock) { entries.toList() }

    /** Entries that arrived at or after [sinceNanos]. */
    fun entriesSince(sinceNanos: Long): List<Entry> =
        synchronized(lock) { entries.filter { it.receivedAtNanos >= sinceNanos } }

    fun stopsSince(sinceNanos: Long): List<Stop> =
        synchronized(lock) { stops.filter { it.atNanos >= sinceNanos } }

    fun clear() {
        synchronized(lock) {
            entries.clear()
            stops.clear()
        }
    }
}
