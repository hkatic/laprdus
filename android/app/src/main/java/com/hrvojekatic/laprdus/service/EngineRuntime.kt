package com.hrvojekatic.laprdus.service

import com.hrvojekatic.laprdus.data.storage.StorageLogger
import java.io.File
import java.io.IOException

/** The subset of the native engine the runtime policy needs. */
interface SpeechEngine {
    /** Loads [voiceId] (and its dictionaries). Returns false if the voice cannot be loaded. */
    fun setVoice(voiceId: String): Boolean
}

/**
 * Thrown from the synthesis path when the native engine cannot load any voice.
 * Deliberately left uncaught: the service process dies, the TTS framework
 * reports ERROR to the client on its next `speak()`, and screen readers such
 * as TalkBack count the failure and switch to another engine if one exists.
 */
class LaprdusEngineUnavailableException(message: String, cause: Throwable? = null) :
    RuntimeException(message, cause)

/**
 * Decides how the TTS service reacts when the engine is not ready.
 *
 * Storage problems are never fatal (synthesis needs no storage: voices and
 * bundled dictionaries come from APK assets). The only fatal condition is the
 * native engine refusing to load both the requested voice and the fallback
 * voice. Even that is bounded by a circuit breaker persisted in
 * [crashMarkerFile]: at most [maxCrashesPerWindow] deliberate crashes per
 * [windowMillis] (TalkBack's own failover threshold), after which the service
 * degrades to per-utterance errors instead of crash-looping.
 *
 * Contains no android.* references so it can be unit-tested on the JVM.
 */
class EngineRuntime(
    private val engine: SpeechEngine,
    private val crashMarkerFile: File?,
    private val logger: StorageLogger = StorageLogger.None,
    private val clock: () -> Long = System::currentTimeMillis,
    private val maxCrashesPerWindow: Int = DEFAULT_MAX_CRASHES,
    private val windowMillis: Long = DEFAULT_WINDOW_MILLIS,
    private val fallbackVoiceId: String = DEFAULT_FALLBACK_VOICE,
) {
    sealed class ReadyResult {
        data class Ready(val voiceId: String, val usedFallback: Boolean) : ReadyResult()
        object Unavailable : ReadyResult()
    }

    /**
     * Makes sure a voice is loaded, trying [requestedVoiceId] then the fallback.
     *
     * @throws LaprdusEngineUnavailableException when no voice can be loaded and
     *   the circuit breaker still allows a deliberate crash.
     * @return [ReadyResult.Unavailable] when no voice can be loaded and the
     *   breaker is open (caller should report a per-utterance error).
     */
    fun ensureReady(requestedVoiceId: String): ReadyResult {
        if (engine.setVoice(requestedVoiceId)) {
            return ReadyResult.Ready(requestedVoiceId, usedFallback = false)
        }
        if (requestedVoiceId != fallbackVoiceId && engine.setVoice(fallbackVoiceId)) {
            logger.warn("Voice '$requestedVoiceId' failed to load; using fallback '$fallbackVoiceId'")
            return ReadyResult.Ready(fallbackVoiceId, usedFallback = true)
        }

        val attempt = recordFatalAttempt()
        if (attempt <= maxCrashesPerWindow) {
            logger.error(
                "Engine cannot load '$requestedVoiceId' or '$fallbackVoiceId'; " +
                    "terminating so the system can fall back ($attempt/$maxCrashesPerWindow)"
            )
            throw LaprdusEngineUnavailableException(
                "Laprdus cannot load voice '$requestedVoiceId' or fallback voice '$fallbackVoiceId'"
            )
        }
        logger.error(
            "Engine cannot load any voice and the crash limit is reached; " +
                "reporting per-utterance errors instead"
        )
        return ReadyResult.Unavailable
    }

    /** Clears the crash breaker after speech worked. */
    fun onSynthesisSucceeded() {
        val file = crashMarkerFile ?: return
        if (file.exists() && !file.delete()) {
            logger.warn("Could not clear engine crash marker ${file.path}")
        }
    }

    /**
     * Records a fatal attempt and returns how many happened inside the window,
     * including this one. Returns [Int.MAX_VALUE] if the marker cannot be
     * persisted, so an unbounded crash loop is impossible.
     */
    internal fun recordFatalAttempt(): Int {
        val file = crashMarkerFile ?: return 1
        val now = clock()
        return try {
            val recent = if (file.isFile) {
                file.readLines()
                    .mapNotNull { it.trim().toLongOrNull() }
                    .filter { (now - it) in 0..windowMillis }
            } else {
                emptyList()
            }
            val updated = recent + now
            file.parentFile?.mkdirs()
            file.writeText(updated.joinToString("\n"))
            updated.size
        } catch (e: IOException) {
            logger.error("Cannot persist engine crash marker; degrading instead of crashing", e)
            Int.MAX_VALUE
        }
    }

    companion object {
        const val DEFAULT_MAX_CRASHES = 3
        const val DEFAULT_WINDOW_MILLIS: Long = 10L * 60L * 1000L
        const val DEFAULT_FALLBACK_VOICE = "josip"
    }
}
