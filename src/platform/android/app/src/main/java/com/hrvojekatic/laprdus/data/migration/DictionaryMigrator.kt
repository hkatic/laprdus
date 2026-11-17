package com.hrvojekatic.laprdus.data.migration

import com.hrvojekatic.laprdus.data.AtomicFiles
import com.hrvojekatic.laprdus.data.storage.StorageLogger
import java.io.File
import java.io.IOException

/**
 * Moves the user dictionary files from the legacy credential-encrypted files
 * directory into the device-protected files directory.
 *
 * Per file: if the target already exists the target wins and the legacy copy
 * is deleted; otherwise the legacy file is copied with a unique temp name,
 * synced, renamed into place, and then the legacy file is deleted. Every step
 * is idempotent, so a crash at any [MigrationCrashPoint] converges on the next
 * run. Stale temp files from an earlier crash are removed first.
 *
 * @param legacyDir evaluated only after the unlock gate (see [SettingsMigrator]).
 */
class DictionaryMigrator(
    private val legacyDir: () -> File,
    private val targetDir: File,
    private val fileNames: List<String>,
    isUserUnlocked: () -> Boolean,
    private val faultInjector: MigrationFaultInjector = MigrationFaultInjector.NONE,
    logger: StorageLogger = StorageLogger.None,
    clock: () -> Long = System::nanoTime,
    retryIntervalNanos: Long = DEFAULT_RETRY_INTERVAL_NANOS,
) : LegacyMigrator(isUserUnlocked, logger, clock, retryIntervalNanos) {

    override suspend fun performMigration(): MigrationResult {
        val legacy = legacyDir()
        if (sameDirectory(legacy, targetDir)) return MigrationResult.NotNeeded
        if (!legacy.isDirectory) return MigrationResult.NotNeeded

        // Only this migrator's own leftovers: the repository writes untagged temp
        // files into the same directory and may be mid-write right now.
        AtomicFiles.deleteStaleTempFiles(targetDir, fileNames, MIGRATION_TEMP_TAG)

        var processed = 0
        var copied = 0
        var pending: Throwable? = null
        for (name in fileNames) {
            val source = File(legacy, name)
            if (!source.isFile) continue
            processed++
            val target = File(targetDir, name)

            if (!target.exists()) {
                faultInjector.crashIfArmed(MigrationCrashPoint.DICT_BEFORE_COPY)
                val bytes = try {
                    source.readBytes()
                } catch (e: IOException) {
                    logger.warn("Legacy dictionary $name could not be read yet; will retry", e)
                    pending = e
                    continue
                }
                try {
                    AtomicFiles.writeBytesAtomically(target, bytes, MIGRATION_TEMP_TAG) {
                        faultInjector.crashIfArmed(MigrationCrashPoint.DICT_AFTER_TMP_WRITE)
                    }
                } catch (e: IOException) {
                    throw DirectBootMigrationException(
                        "Could not write dictionary $name to device-protected storage", e
                    )
                }
                copied++
                faultInjector.crashIfArmed(MigrationCrashPoint.DICT_AFTER_RENAME)
            }

            if (!source.delete() && source.exists()) {
                logger.warn("Could not delete legacy dictionary ${source.path}; will retry")
                pending = IOException("Cannot delete ${source.path}")
                continue
            }
            faultInjector.crashIfArmed(MigrationCrashPoint.DICT_AFTER_DELETE_LEGACY)
        }

        pending?.let { return MigrationResult.RetryLater(it) }
        if (processed == 0) return MigrationResult.NotNeeded
        logger.info("Migrated $copied dictionary file(s) from legacy storage ($processed processed)")
        return MigrationResult.Migrated(copied)
    }

    companion object {
        /** Tag embedded in this migrator's temp file names (see [AtomicFiles]). */
        const val MIGRATION_TEMP_TAG = "migration"
    }

    private fun sameDirectory(a: File, b: File): Boolean {
        val ca = try { a.canonicalFile } catch (_: IOException) { a.absoluteFile }
        val cb = try { b.canonicalFile } catch (_: IOException) { b.absoluteFile }
        return ca == cb
    }
}
