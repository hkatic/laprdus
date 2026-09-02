package com.hrvojekatic.laprdus.data.storage

/**
 * Minimal logging abstraction for the storage and migration layer.
 *
 * The migration classes must not reference android.* so they can run in plain
 * JVM unit tests; the Android-backed implementation is wired in by
 * [LaprdusStorage].
 */
interface StorageLogger {
    fun info(message: String)
    fun warn(message: String, error: Throwable? = null)
    fun error(message: String, error: Throwable? = null)

    /** Logger that discards everything (default for tests). */
    object None : StorageLogger {
        override fun info(message: String) = Unit
        override fun warn(message: String, error: Throwable?) = Unit
        override fun error(message: String, error: Throwable?) = Unit
    }
}
