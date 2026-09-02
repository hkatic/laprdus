package com.hrvojekatic.laprdus.data.storage

import android.util.Log

/** [StorageLogger] backed by android.util.Log. */
class AndroidStorageLogger(private val tag: String) : StorageLogger {
    override fun info(message: String) {
        Log.i(tag, message)
    }

    override fun warn(message: String, error: Throwable?) {
        if (error != null) Log.w(tag, message, error) else Log.w(tag, message)
    }

    override fun error(message: String, error: Throwable?) {
        if (error != null) Log.e(tag, message, error) else Log.e(tag, message)
    }
}
