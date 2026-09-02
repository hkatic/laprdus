package com.hrvojekatic.laprdus.data.migration

import androidx.datastore.core.CorruptionException
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.MutablePreferences
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.PreferencesFileSerializer
import androidx.datastore.preferences.core.edit
import com.hrvojekatic.laprdus.data.storage.StorageLogger
import java.io.File
import java.io.IOException

/**
 * Moves the legacy credential-encrypted settings DataStore file into the
 * device-protected settings DataStore.
 *
 * Merge rule: a key already present in the device-protected store wins; legacy
 * values only fill in missing keys. That makes every re-run (after a crash
 * between commit and cleanup, or after an old backup is restored) safe.
 *
 * Steps, each separated by a [MigrationCrashPoint]:
 * 1. read the legacy file with the public DataStore serializer (no second
 *    DataStore instance is ever opened on it),
 * 2. one atomic `edit` on the target store,
 * 3. delete the legacy file.
 *
 * @param legacyFile evaluated only after the unlock gate, because resolving a
 *   credential-encrypted path while locked is itself a StrictMode violation.
 */
class SettingsMigrator(
    private val legacyFile: () -> File,
    private val target: DataStore<Preferences>,
    isUserUnlocked: () -> Boolean,
    private val faultInjector: MigrationFaultInjector = MigrationFaultInjector.NONE,
    logger: StorageLogger = StorageLogger.None,
    clock: () -> Long = System::nanoTime,
    retryIntervalNanos: Long = DEFAULT_RETRY_INTERVAL_NANOS,
) : LegacyMigrator(isUserUnlocked, logger, clock, retryIntervalNanos) {

    override suspend fun performMigration(): MigrationResult {
        val legacy = legacyFile()
        if (!legacy.isFile) return MigrationResult.NotNeeded

        faultInjector.crashIfArmed(MigrationCrashPoint.SETTINGS_BEFORE_READ_LEGACY)
        val legacyPrefs: Preferences = try {
            legacy.inputStream().use { PreferencesFileSerializer.readFrom(it) }
        } catch (e: CorruptionException) {
            quarantine(legacy, e)
            return MigrationResult.QuarantinedCorrupt
        } catch (e: IOException) {
            logger.warn("Legacy settings file could not be read yet; will retry", e)
            return MigrationResult.RetryLater(e)
        }
        faultInjector.crashIfArmed(MigrationCrashPoint.SETTINGS_AFTER_READ_LEGACY)

        var copied = 0
        try {
            target.edit { prefs ->
                copied = 0
                for ((key, value) in legacyPrefs.asMap()) {
                    if (!prefs.contains(key)) {
                        prefs.setUnchecked(key, value)
                        copied++
                    }
                }
            }
        } catch (e: IOException) {
            throw DirectBootMigrationException(
                "Could not write migrated settings to device-protected storage", e
            )
        }
        faultInjector.crashIfArmed(MigrationCrashPoint.SETTINGS_AFTER_COMMIT)

        deleteLegacy(legacy)
        faultInjector.crashIfArmed(MigrationCrashPoint.SETTINGS_AFTER_DELETE_LEGACY)

        logger.info("Migrated $copied settings key(s) from legacy storage")
        return MigrationResult.Migrated(copied)
    }

    private fun quarantine(legacy: File, cause: Throwable) {
        val quarantined = File(legacy.parentFile, legacy.name + CORRUPT_SUFFIX)
        quarantined.delete()
        if (legacy.renameTo(quarantined)) {
            logger.warn("Legacy settings file is corrupt; moved to ${quarantined.name}", cause)
        } else {
            legacy.delete()
            logger.warn("Legacy settings file is corrupt and was deleted", cause)
        }
    }

    private fun deleteLegacy(legacy: File) {
        if (!legacy.delete() && legacy.exists()) {
            logger.warn("Could not delete legacy settings file ${legacy.path}; it will be ignored")
            return
        }
        // Remove the now-empty legacy datastore directory (fails silently if not empty).
        legacy.parentFile?.takeIf { it.name == "datastore" }?.delete()
    }

    companion object {
        const val CORRUPT_SUFFIX = ".corrupt"

        @Suppress("UNCHECKED_CAST")
        private fun MutablePreferences.setUnchecked(key: Preferences.Key<*>, value: Any) {
            this[key as Preferences.Key<Any>] = value
        }
    }
}
