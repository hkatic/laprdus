package com.hrvojekatic.laprdus.data.migration

/** Base class for storage-layer failures. */
open class LaprdusStorageException(message: String, cause: Throwable? = null) :
    RuntimeException(message, cause)

/**
 * Thrown when legacy (credential-encrypted) data could not be written into
 * device-protected storage. Callers treat this as non-fatal: the migration is
 * retried later and the app keeps running with the settings it has.
 */
class DirectBootMigrationException(message: String, cause: Throwable? = null) :
    LaprdusStorageException(message, cause)

/**
 * Thrown by a [MigrationFaultInjector] to simulate a process crash at a
 * specific point of a migration. Never caught by the storage layer.
 */
class SimulatedMigrationCrashException(val point: MigrationCrashPoint) :
    LaprdusStorageException("Simulated crash at migration point $point")
