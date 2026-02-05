package com.hrvojekatic.laprdus.audio

import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import android.util.Log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.withContext
import kotlin.coroutines.coroutineContext

/**
 * AudioTrack-based player for 16-bit PCM audio.
 * Optimized for TTS output at 22050Hz mono.
 *
 * @param sampleRate Audio sample rate in Hz (default 22050)
 */
class AudioPlayer(
    private val sampleRate: Int = 22050
) {
    companion object {
        private const val TAG = "AudioPlayer"
    }

    private var audioTrack: AudioTrack? = null

    private val bufferSize = AudioTrack.getMinBufferSize(
        sampleRate,
        AudioFormat.CHANNEL_OUT_MONO,
        AudioFormat.ENCODING_PCM_16BIT
    ).coerceAtLeast(4096)

    /**
     * Initialize the audio track for playback.
     * Call this before playing any audio.
     */
    fun initialize() {
        if (audioTrack != null) {
            Log.d(TAG, "AudioTrack already initialized")
            return
        }

        val attributes = AudioAttributes.Builder()
            .setUsage(AudioAttributes.USAGE_ASSISTANCE_ACCESSIBILITY)
            .setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
            .build()

        val format = AudioFormat.Builder()
            .setSampleRate(sampleRate)
            .setChannelMask(AudioFormat.CHANNEL_OUT_MONO)
            .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
            .build()

        audioTrack = AudioTrack(
            attributes,
            format,
            bufferSize,
            AudioTrack.MODE_STREAM,
            AudioManager.AUDIO_SESSION_ID_GENERATE
        )

        Log.d(TAG, "AudioTrack initialized: sampleRate=$sampleRate, bufferSize=$bufferSize")
    }

    /**
     * Play audio samples asynchronously.
     * This suspending function will block until playback completes or is cancelled.
     *
     * @param samples 16-bit PCM audio samples
     */
    suspend fun play(samples: ShortArray) = withContext(Dispatchers.IO) {
        val track = audioTrack
        if (track == null) {
            Log.e(TAG, "AudioTrack not initialized")
            return@withContext
        }

        if (track.state != AudioTrack.STATE_INITIALIZED) {
            Log.e(TAG, "AudioTrack not in initialized state")
            return@withContext
        }

        if (samples.isEmpty()) {
            Log.d(TAG, "No samples to play")
            return@withContext
        }

        Log.d(TAG, "Playing ${samples.size} samples")

        try {
            track.play()

            // Write samples in chunks for responsive cancellation
            val chunkSize = bufferSize / 2
            var offset = 0

            while (offset < samples.size && coroutineContext.isActive) {
                val count = minOf(chunkSize, samples.size - offset)
                val written = track.write(samples, offset, count)

                if (written < 0) {
                    Log.e(TAG, "AudioTrack write error: $written")
                    break
                }

                offset += written
            }

            // Wait for playback to complete
            if (coroutineContext.isActive) {
                while (track.playbackHeadPosition < samples.size && coroutineContext.isActive) {
                    delay(10)
                }
            }

        } finally {
            track.stop()
            track.flush()
            Log.d(TAG, "Playback complete")
        }
    }

    /**
     * Stop playback immediately
     */
    fun stop() {
        audioTrack?.let { track ->
            if (track.playState == AudioTrack.PLAYSTATE_PLAYING) {
                track.stop()
                track.flush()
                Log.d(TAG, "Playback stopped")
            }
        }
    }

    /**
     * Release audio resources.
     * Call this when done with the player.
     */
    fun release() {
        audioTrack?.release()
        audioTrack = null
        Log.d(TAG, "AudioTrack released")
    }

    /**
     * Check if audio is currently playing
     */
    val isPlaying: Boolean
        get() = audioTrack?.playState == AudioTrack.PLAYSTATE_PLAYING
}
