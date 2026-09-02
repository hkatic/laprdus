package com.hrvojekatic.laprdus.data

import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.content.pm.ResolveInfo
import android.os.Build
import android.speech.tts.TextToSpeech
import android.util.Log
import androidx.core.os.UserManagerCompat
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import com.hrvojekatic.laprdus.data.storage.LaprdusStorage
import com.hrvojekatic.laprdus.service.LaprdusTTSService
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Instrumented checks of the Direct Boot plumbing on a real device: the
 * device-protected (DE) storage context, the directories [LaprdusStorage]
 * resolves, the unlock state, and the manifest flag that keeps the TTS
 * service visible to the framework before the user's first unlock.
 *
 * Read-only on purpose: it never opens the production settings DataStore and
 * never deletes anything in the app's storage, so it is safe to run while the
 * TTS service is alive in the same process. Instrumented tests can only run on
 * an unlocked device (the test APK itself lives in credential-encrypted
 * storage), so the actual pre-unlock behaviour is verified manually.
 */
@RunWith(AndroidJUnit4::class)
class DeviceProtectedStorageInstrumentedTest {

    private val context: Context = InstrumentationRegistry.getInstrumentation().targetContext
    private val deContext: Context = context.createDeviceProtectedStorageContext()

    // ==========================================================================
    // Device-protected storage context
    // ==========================================================================

    @Test
    fun deviceProtectedContextReportsDeviceProtectedStorage() {
        assertTrue(
            "createDeviceProtectedStorageContext() must yield a device-protected context",
            deContext.isDeviceProtectedStorage
        )
        assertFalse(
            "the regular app context must be credential-encrypted",
            context.isDeviceProtectedStorage
        )
    }

    @Test
    fun deviceProtectedFilesDirIsSeparateFromCredentialEncryptedFilesDir() {
        val deFiles = deContext.filesDir
        val ceFiles = context.filesDir
        assertNotNull("DE filesDir must resolve", deFiles)
        assertNotNull("CE filesDir must resolve", ceFiles)

        assertNotEquals(
            "DE and CE files directories must differ",
            ceFiles.canonicalPath,
            deFiles.canonicalPath
        )
        assertTrue(
            "DE files dir should live under user_de, was ${deFiles.path}",
            deFiles.path.contains("user_de")
        )
        assertTrue("DE files dir must exist after resolving it", deFiles.isDirectory)
    }

    // ==========================================================================
    // LaprdusStorage path resolution
    // ==========================================================================

    @Test
    fun dictionaryDirIsTheDeviceProtectedFilesDir() {
        assertEquals(
            deContext.filesDir.canonicalPath,
            LaprdusStorage.dictionaryDir(context).canonicalPath
        )
    }

    @Test
    fun legacyDictionaryDirIsTheCredentialEncryptedFilesDir() {
        assertEquals(
            context.filesDir.canonicalPath,
            LaprdusStorage.legacyDictionaryDir(context).canonicalPath
        )
        assertNotEquals(
            "legacy and current dictionary dirs must not collide",
            LaprdusStorage.dictionaryDir(context).canonicalPath,
            LaprdusStorage.legacyDictionaryDir(context).canonicalPath
        )
    }

    @Test
    fun legacySettingsFileAndCrashMarkerResolveToTheExpectedStorage() {
        // Legacy DataStore file: <CE files>/datastore/laprdus_settings.preferences_pb
        val legacy = LaprdusStorage.legacySettingsFile(context)
        assertEquals(LaprdusStorage.SETTINGS_FILE_NAME, legacy.name)
        assertEquals("datastore", legacy.parentFile!!.name)
        assertEquals(
            context.filesDir.canonicalPath,
            legacy.parentFile!!.parentFile!!.canonicalPath
        )

        // Engine crash marker: <DE files>/engine_crash_marker
        val marker = LaprdusStorage.engineCrashMarkerFile(context)
        assertEquals(LaprdusStorage.ENGINE_CRASH_MARKER_FILE_NAME, marker.name)
        assertEquals(deContext.filesDir.canonicalPath, marker.parentFile!!.canonicalPath)
    }

    // ==========================================================================
    // Unlock state
    // ==========================================================================

    @Test
    fun userIsUnlockedWhileInstrumentedTestsRun() {
        assertTrue(
            "instrumented tests can only run after the user unlocked",
            UserManagerCompat.isUserUnlocked(context)
        )
        assertTrue(
            "LaprdusStorage.isUserUnlocked must agree with UserManagerCompat",
            LaprdusStorage.isUserUnlocked(context)
        )
    }

    // ==========================================================================
    // Manifest: the TTS service is direct-boot aware
    // ==========================================================================

    @Test
    fun ttsServiceIsDirectBootAware() {
        // MATCH_DIRECT_BOOT_AWARE without MATCH_DIRECT_BOOT_UNAWARE returns only
        // direct-boot-aware components regardless of the current user state,
        // i.e. exactly the engines the framework can see before the first unlock.
        val aware = queryTtsEngines(PackageManager.MATCH_DIRECT_BOOT_AWARE)
        val awarePackages = aware.map { it.serviceInfo.packageName }.distinct()

        // Diagnostic for the device. Note: on Android 11+ package visibility
        // filtering hides engines of packages this app cannot see, so the list
        // may be shorter than what the TTS settings screen shows.
        Log.i(
            TAG,
            "Direct-boot-aware TTS engines visible to ${context.packageName}: " +
                awarePackages.joinToString(", ").ifEmpty { "(none)" }
        )

        val laprdus = aware.firstOrNull { it.serviceInfo.packageName == context.packageName }
        assertNotNull(
            "Laprdus must be returned by MATCH_DIRECT_BOOT_AWARE; found $awarePackages",
            laprdus
        )
        assertTrue(
            "LaprdusTTSService must be declared android:directBootAware=\"true\"",
            laprdus!!.serviceInfo.directBootAware
        )
        assertEquals(LaprdusTTSService::class.java.name, laprdus.serviceInfo.name)
    }

    @Test
    fun ttsServiceIsListedForRegularQueriesAsWell() {
        // Default flags match runnable components for the current (unlocked)
        // user state; the service must not have disappeared from normal lookups.
        val all = queryTtsEngines(0)
        val allPackages = all.map { "${it.serviceInfo.packageName}(directBootAware=${it.serviceInfo.directBootAware})" }
        Log.i(TAG, "All TTS engines visible to ${context.packageName}: $allPackages")

        assertTrue(
            "Laprdus must be a resolvable TTS engine; found $allPackages",
            all.any { it.serviceInfo.packageName == context.packageName }
        )
    }

    private fun queryTtsEngines(flags: Int): List<ResolveInfo> {
        val intent = Intent(TextToSpeech.Engine.INTENT_ACTION_TTS_SERVICE)
        val pm = context.packageManager
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            pm.queryIntentServices(intent, PackageManager.ResolveInfoFlags.of(flags.toLong()))
        } else {
            @Suppress("DEPRECATION")
            pm.queryIntentServices(intent, flags)
        }
    }

    private companion object {
        const val TAG = "DirectBootTest"
    }
}
