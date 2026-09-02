package com.hrvojekatic.laprdus.data.storage

import android.content.Context
import androidx.annotation.VisibleForTesting
import androidx.core.os.UserManagerCompat
import androidx.datastore.core.DataStore
import androidx.datastore.core.deviceProtectedDataStoreFile
import androidx.datastore.core.handlers.ReplaceFileCorruptionHandler
import androidx.datastore.preferences.core.PreferenceDataStoreFactory
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.emptyPreferences
import com.hrvojekatic.laprdus.BuildConfig
import com.hrvojekatic.laprdus.data.migration.DebugFileCrashInjector
import com.hrvojekatic.laprdus.data.migration.DictionaryMigrator
import com.hrvojekatic.laprdus.data.migration.MigrationFaultInjector
import com.hrvojekatic.laprdus.data.migration.SettingsMigrator
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancelAndJoin
import kotlinx.coroutines.runBlocking
import java.io.File

/**
 * Process-wide owner of Laprdus' persistent storage.
 *
 * All mutable app data (settings DataStore and user dictionaries) lives in
 * device-protected storage so the TTS service can use it on the lock screen
 * before the user's first unlock (Direct Boot). Data written by versions that
 * used credential-encrypted storage is migrated by [SettingsMigrator] and
 * [DictionaryMigrator], which are single instances shared by the UI and the
 * service so their locking is effective.
 *
 * The settings DataStore is a single instance per process: DataStore throws
 * if two instances are opened on one file.
 */
object LaprdusStorage {
    const val SETTINGS_FILE_NAME = "laprdus_settings.preferences_pb"
    const val LEGACY_SETTINGS_RELATIVE_PATH = "datastore/$SETTINGS_FILE_NAME"
    const val DEBUG_CRASH_POINT_RELATIVE_PATH = "debug/crash_point"
    const val ENGINE_CRASH_MARKER_FILE_NAME = "engine_crash_marker"
    val DICTIONARY_FILE_NAMES: List<String> = listOf("user.json", "spelling.json", "emoji.json")

    private val lock = Any()

    @Volatile
    private var deviceContext: Context? = null

    @Volatile
    private var dataStore: DataStore<Preferences>? = null
    private var dataStoreScope: CoroutineScope? = null

    @Volatile
    private var settingsMigrator: SettingsMigrator? = null

    @Volatile
    private var dictionaryMigrator: DictionaryMigrator? = null

    /** Context whose storage APIs point at device-protected storage. */
    fun deviceProtectedContext(context: Context): Context {
        deviceContext?.let { return it }
        synchronized(lock) {
            deviceContext?.let { return it }
            return context.applicationContext.createDeviceProtectedStorageContext().also {
                deviceContext = it
            }
        }
    }

    /** True once the user has entered their credentials (always true without a lock screen). */
    fun isUserUnlocked(context: Context): Boolean = UserManagerCompat.isUserUnlocked(context)

    /** Directory holding the user dictionaries (device-protected). */
    fun dictionaryDir(context: Context): File = deviceProtectedContext(context).filesDir

    /** Legacy (credential-encrypted) dictionary directory; only touch after unlock. */
    fun legacyDictionaryDir(context: Context): File = context.applicationContext.filesDir

    /** Legacy (credential-encrypted) settings DataStore file; only touch after unlock. */
    fun legacySettingsFile(context: Context): File =
        File(context.applicationContext.filesDir, LEGACY_SETTINGS_RELATIVE_PATH)

    /** Device-protected file used by the engine crash circuit breaker. */
    fun engineCrashMarkerFile(context: Context): File =
        File(dictionaryDir(context), ENGINE_CRASH_MARKER_FILE_NAME)

    fun logger(tag: String): StorageLogger = AndroidStorageLogger(tag)

    /** The process-wide settings DataStore in device-protected storage. */
    fun settingsDataStore(context: Context): DataStore<Preferences> {
        dataStore?.let { return it }
        synchronized(lock) {
            dataStore?.let { return it }
            val app = context.applicationContext
            val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
            val log = logger("LaprdusStorage")
            val store = PreferenceDataStoreFactory.create(
                corruptionHandler = ReplaceFileCorruptionHandler { e ->
                    log.error("Settings store is corrupt; replacing it with defaults", e)
                    emptyPreferences()
                },
                scope = scope,
                produceFile = { app.deviceProtectedDataStoreFile(SETTINGS_FILE_NAME) }
            )
            dataStoreScope = scope
            dataStore = store
            return store
        }
    }

    fun settingsMigrator(context: Context): SettingsMigrator {
        settingsMigrator?.let { return it }
        synchronized(lock) {
            settingsMigrator?.let { return it }
            val app = context.applicationContext
            return SettingsMigrator(
                legacyFile = { legacySettingsFile(app) },
                target = settingsDataStore(app),
                isUserUnlocked = { isUserUnlocked(app) },
                faultInjector = faultInjector(app),
                logger = logger("SettingsMigrator")
            ).also { settingsMigrator = it }
        }
    }

    fun dictionaryMigrator(context: Context): DictionaryMigrator {
        dictionaryMigrator?.let { return it }
        synchronized(lock) {
            dictionaryMigrator?.let { return it }
            val app = context.applicationContext
            return DictionaryMigrator(
                legacyDir = { legacyDictionaryDir(app) },
                targetDir = dictionaryDir(app),
                fileNames = DICTIONARY_FILE_NAMES,
                isUserUnlocked = { isUserUnlocked(app) },
                faultInjector = faultInjector(app),
                logger = logger("DictionaryMigrator")
            ).also { dictionaryMigrator = it }
        }
    }

    /**
     * Debug builds can force a real process crash at a migration step by
     * writing the step name into the control file (see [DebugFileCrashInjector]).
     * Release builds never reference the injector, so R8 removes it.
     */
    private fun faultInjector(app: Context): MigrationFaultInjector =
        if (BuildConfig.DEBUG) {
            DebugFileCrashInjector(File(dictionaryDir(app), DEBUG_CRASH_POINT_RELATIVE_PATH))
        } else {
            MigrationFaultInjector.NONE
        }

    /** Drops all singletons (cancelling the DataStore scope). Tests only. */
    @VisibleForTesting
    fun resetForTesting() {
        synchronized(lock) {
            dataStoreScope?.let { scope ->
                runBlocking { scope.coroutineContext[Job]?.cancelAndJoin() }
            }
            dataStoreScope = null
            dataStore = null
            settingsMigrator = null
            dictionaryMigrator = null
            deviceContext = null
        }
    }
}
