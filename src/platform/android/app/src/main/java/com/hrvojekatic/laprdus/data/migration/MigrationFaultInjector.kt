package com.hrvojekatic.laprdus.data.migration

import java.io.File
import java.io.IOException

/** Points in the legacy-to-device-protected migrations where a crash can be simulated. */
enum class MigrationCrashPoint {
    SETTINGS_BEFORE_READ_LEGACY,
    SETTINGS_AFTER_READ_LEGACY,
    SETTINGS_AFTER_COMMIT,
    SETTINGS_AFTER_DELETE_LEGACY,
    DICT_BEFORE_COPY,
    DICT_AFTER_TMP_WRITE,
    DICT_AFTER_RENAME,
    DICT_AFTER_DELETE_LEGACY,
}

/**
 * Hook invoked by the migrators between their steps. Implementations may throw
 * [SimulatedMigrationCrashException] to emulate the process dying at that point.
 */
fun interface MigrationFaultInjector {
    fun crashIfArmed(point: MigrationCrashPoint)

    companion object {
        /** Production default: never crashes. */
        val NONE: MigrationFaultInjector = MigrationFaultInjector { }
    }
}

/**
 * Crashes exactly once at [point], then disarms itself. Used by unit and
 * instrumented tests to verify that an interrupted migration converges on the
 * next attempt.
 */
class OneShotCrashInjector(private val point: MigrationCrashPoint) : MigrationFaultInjector {
    @Volatile
    var fired: Boolean = false
        private set

    override fun crashIfArmed(point: MigrationCrashPoint) {
        if (point == this.point && !fired) {
            fired = true
            throw SimulatedMigrationCrashException(point)
        }
    }
}

/**
 * Debug-build injector driven by a control file in device-protected storage.
 * Writing a [MigrationCrashPoint] name into the file (for example with
 * `adb shell run-as`) makes the next migration throw at that point, which
 * kills the process for real. The file is deleted before throwing so a single
 * arming produces exactly one crash.
 */
class DebugFileCrashInjector(private val controlFile: File) : MigrationFaultInjector {
    override fun crashIfArmed(point: MigrationCrashPoint) {
        val armed = try {
            if (controlFile.isFile) controlFile.readText().trim() else return
        } catch (_: IOException) {
            return
        }
        if (armed.equals(point.name, ignoreCase = true)) {
            controlFile.delete()
            throw SimulatedMigrationCrashException(point)
        }
    }
}
