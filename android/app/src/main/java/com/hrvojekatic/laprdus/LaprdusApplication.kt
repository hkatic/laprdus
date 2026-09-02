package com.hrvojekatic.laprdus

import android.app.Application
import android.content.Context
import android.content.pm.ApplicationInfo
import android.os.Build
import android.os.StrictMode
import dagger.hilt.android.HiltAndroidApp

/**
 * Application class for Laprdus TTS.
 * Annotated with @HiltAndroidApp to enable Hilt dependency injection.
 *
 * Because the TTS service is direct-boot aware, this class also runs before
 * the user's first unlock. It must therefore never touch credential-encrypted
 * storage eagerly; Hilt singletons are created lazily, which keeps that true.
 */
@HiltAndroidApp
class LaprdusApplication : Application() {

    override fun attachBaseContext(base: Context) {
        installDebugStrictMode(base)
        super.attachBaseContext(base)
    }

    /**
     * In debuggable builds, log any access to credential-encrypted storage
     * while the user is locked. Such access silently sees an empty directory,
     * which is the classic Direct Boot bug; StrictMode makes it visible in
     * logcat during lock-screen testing.
     */
    private fun installDebugStrictMode(base: Context) {
        val debuggable = (base.applicationInfo.flags and ApplicationInfo.FLAG_DEBUGGABLE) != 0
        if (!debuggable || Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) return
        StrictMode.setVmPolicy(
            StrictMode.VmPolicy.Builder(StrictMode.getVmPolicy())
                .detectCredentialProtectedWhileLocked()
                .penaltyLog()
                .build()
        )
    }
}
