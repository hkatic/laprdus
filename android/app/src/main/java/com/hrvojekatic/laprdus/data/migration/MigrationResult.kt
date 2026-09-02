package com.hrvojekatic.laprdus.data.migration

/** Outcome of one [LegacyMigrator.migrateIfNeeded] call. */
sealed class MigrationResult {
    /** Nothing to migrate (no legacy data, or already migrated). */
    object NotNeeded : MigrationResult()

    /** The user is still locked; credential-encrypted storage is not readable yet. */
    object SkippedLocked : MigrationResult()

    /** Legacy data exists but could not be read right now; retried later. */
    data class RetryLater(val cause: Throwable) : MigrationResult()

    /** Migration completed; [itemCount] keys or files were copied. */
    data class Migrated(val itemCount: Int) : MigrationResult()

    /** The legacy data was corrupt and has been set aside; defaults are used. */
    object QuarantinedCorrupt : MigrationResult()

    override fun toString(): String = when (this) {
        NotNeeded -> "NotNeeded"
        SkippedLocked -> "SkippedLocked"
        is RetryLater -> "RetryLater(${cause.javaClass.simpleName}: ${cause.message})"
        is Migrated -> "Migrated($itemCount)"
        QuarantinedCorrupt -> "QuarantinedCorrupt"
    }
}
