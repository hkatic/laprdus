package com.hrvojekatic.laprdus.data.migration

import com.hrvojekatic.laprdus.data.storage.StorageLogger
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock

/**
 * Shared state machine for the credential-encrypted → device-protected migrations.
 *
 * - Runs only while the user is unlocked (credential-encrypted storage is
 *   otherwise invisible and would look empty).
 * - Is idempotent and safe to call from several coroutines; a [Mutex]
 *   serializes attempts and a `done` flag makes later calls free.
 * - Failures leave `done` unset; retries are rate-limited to one per
 *   [retryIntervalNanos] so concurrent collectors do not hammer the disk.
 * - [rearm] re-enables the migration after the user unlocks.
 *
 * Contains no android.* references so it can run in plain JVM tests.
 */
abstract class LegacyMigrator(
    private val isUserUnlocked: () -> Boolean,
    protected val logger: StorageLogger,
    private val clock: () -> Long = System::nanoTime,
    private val retryIntervalNanos: Long = DEFAULT_RETRY_INTERVAL_NANOS,
) {
    private val mutex = Mutex()

    @Volatile
    var isDone: Boolean = false
        private set

    @Volatile
    private var lastFailureAt: Long? = null

    @Volatile
    private var lastFailure: Throwable? = null

    suspend fun migrateIfNeeded(): MigrationResult {
        if (isDone) return MigrationResult.NotNeeded
        mutex.withLock {
            if (isDone) return MigrationResult.NotNeeded
            val failedAt = lastFailureAt
            if (failedAt != null && clock() - failedAt < retryIntervalNanos) {
                return MigrationResult.RetryLater(
                    lastFailure ?: IllegalStateException("Migration retry rate-limited")
                )
            }
            if (!isUserUnlocked()) return MigrationResult.SkippedLocked

            val result = try {
                performMigration()
            } catch (e: SimulatedMigrationCrashException) {
                throw e
            } catch (e: DirectBootMigrationException) {
                recordFailure(e)
                throw e
            } catch (e: Throwable) {
                // Unexpected failures are rate-limited like the expected ones so a
                // persistent problem cannot re-run the migration on every subscriber.
                if (e !is CancellationException) recordFailure(e)
                throw e
            }
            when (result) {
                is MigrationResult.RetryLater -> recordFailure(result.cause)
                MigrationResult.NotNeeded,
                is MigrationResult.Migrated,
                MigrationResult.QuarantinedCorrupt -> {
                    isDone = true
                    lastFailureAt = null
                    lastFailure = null
                }
                MigrationResult.SkippedLocked -> Unit
            }
            return result
        }
    }

    /** Allow the migration to run again (after the user unlocked, or for tests). */
    fun rearm() {
        isDone = false
        lastFailureAt = null
        lastFailure = null
    }

    private fun recordFailure(cause: Throwable) {
        lastFailureAt = clock()
        lastFailure = cause
    }

    /**
     * Performs the actual migration. Called with the user unlocked, under the
     * mutex. Must be idempotent: it can run again after a crash at any point.
     */
    protected abstract suspend fun performMigration(): MigrationResult

    companion object {
        const val DEFAULT_RETRY_INTERVAL_NANOS: Long = 30_000_000_000L
    }
}
