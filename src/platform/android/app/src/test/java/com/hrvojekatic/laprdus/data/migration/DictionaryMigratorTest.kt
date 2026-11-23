package com.hrvojekatic.laprdus.data.migration

import com.hrvojekatic.laprdus.data.AtomicFiles
import com.hrvojekatic.laprdus.data.storage.StorageLogger
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import kotlinx.coroutines.awaitAll
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.test.runTest
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Assume.assumeFalse
import org.junit.Assume.assumeTrue
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File
import java.io.IOException
import java.util.UUID

/**
 * Unit tests for [DictionaryMigrator]: the move of the user dictionary files
 * from the legacy credential-encrypted files directory into the
 * device-protected files directory.
 *
 * Plain JVM: the migrator has no android.* references. Both directories are
 * TemporaryFolder subfolders, the unlock gate is a lambda, the retry clock is
 * a fake, and mid-migration crashes are simulated with [OneShotCrashInjector].
 */
class DictionaryMigratorTest {

    @get:Rule
    val tempFolder = TemporaryFolder()

    private lateinit var legacyDir: File
    private lateinit var targetDir: File

    /** Fake monotonic clock (nanoseconds) behind the retry rate limit. */
    private var now = 0L

    @Before
    fun setup() {
        legacyDir = tempFolder.newFolder("legacy")
        targetDir = tempFolder.newFolder("target")
    }

    // ==========================================================================
    // Nothing To Migrate
    // ==========================================================================

    @Test
    fun `missing legacy directory returns NotNeeded`() = runTest {
        val migrator = newMigrator(legacy = { File(tempFolder.root, "does-not-exist") })

        assertEquals(MigrationResult.NotNeeded, migrator.migrateIfNeeded())
        assertTrue(migrator.isDone)
        assertEquals(emptyList<String>(), namesIn(targetDir))
    }

    @Test
    fun `legacy directory without dictionary files returns NotNeeded`() = runTest {
        File(legacyDir, "something_else.txt").writeText("keep me")
        val migrator = newMigrator()

        assertEquals(MigrationResult.NotNeeded, migrator.migrateIfNeeded())
        assertTrue(migrator.isDone)
        assertEquals("keep me", File(legacyDir, "something_else.txt").readText())
        assertEquals(emptyList<String>(), namesIn(targetDir))
    }

    @Test
    fun `same legacy and target directory is a no-op that deletes nothing`() = runTest {
        File(targetDir, "user.json").writeBytes(LEGACY_CONTENT.getValue("user.json"))
        val migrator = newMigrator(legacy = { targetDir })

        assertEquals(MigrationResult.NotNeeded, migrator.migrateIfNeeded())
        assertTrue(migrator.isDone)
        assertArrayEquals(LEGACY_CONTENT.getValue("user.json"), File(targetDir, "user.json").readBytes())
        assertEquals(listOf("user.json"), namesIn(targetDir))
    }

    @Test
    fun `same directory reached through a different path is also a no-op`() = runTest {
        File(targetDir, "user.json").writeBytes(LEGACY_CONTENT.getValue("user.json"))
        // "<root>/target/../target" canonicalizes to targetDir.
        val roundabout = File(targetDir, "..${File.separator}${targetDir.name}")
        val migrator = newMigrator(legacy = { roundabout })

        assertEquals(MigrationResult.NotNeeded, migrator.migrateIfNeeded())
        assertTrue(migrator.isDone)
        assertEquals(listOf("user.json"), namesIn(targetDir))
    }

    // ==========================================================================
    // Unlock Gate
    // ==========================================================================

    @Test
    fun `locked user returns SkippedLocked and never resolves the legacy directory`() = runTest {
        seedLegacy()
        var unlocked = false
        var legacyResolved = 0
        val migrator = newMigrator(
            legacy = { legacyResolved++; legacyDir },
            isUnlocked = { unlocked }
        )

        assertEquals(MigrationResult.SkippedLocked, migrator.migrateIfNeeded())
        assertEquals(MigrationResult.SkippedLocked, migrator.migrateIfNeeded())
        assertEquals("legacy path must not be resolved while locked", 0, legacyResolved)
        assertFalse(migrator.isDone)
        assertLegacyIntact()
        assertEquals(emptyList<String>(), namesIn(targetDir))

        // ACTION_USER_UNLOCKED: the same instance migrates without a rearm,
        // because a skipped attempt records no failure.
        unlocked = true
        assertEquals(MigrationResult.Migrated(3), migrator.migrateIfNeeded())
        assertEquals(1, legacyResolved)
        assertTrue(migrator.isDone)
        assertConverged()
    }

    // ==========================================================================
    // Successful Migration
    // ==========================================================================

    @Test
    fun `unlocked user copies every file byte for byte and deletes the legacy copies`() = runTest {
        seedLegacy()
        val migrator = newMigrator()

        assertEquals(MigrationResult.Migrated(3), migrator.migrateIfNeeded())
        assertTrue(migrator.isDone)
        assertConverged()
        assertEquals(FILE_NAMES.sorted(), namesIn(targetDir))
        assertEquals(emptyList<String>(), namesIn(legacyDir))

        // Idempotent: the done flag makes later calls free.
        assertEquals(MigrationResult.NotNeeded, migrator.migrateIfNeeded())
    }

    @Test
    fun `partial legacy set migrates only the files that exist`() = runTest {
        seedLegacy(names = listOf("spelling.json"))
        val migrator = newMigrator()

        assertEquals(MigrationResult.Migrated(1), migrator.migrateIfNeeded())
        assertTrue(migrator.isDone)
        assertConverged(names = listOf("spelling.json"))
        assertEquals(listOf("spelling.json"), namesIn(targetDir))
        assertEquals(emptyList<String>(), namesIn(legacyDir))
    }

    @Test
    fun `existing target wins and only the legacy copy is deleted`() = runTest {
        val targetContent = """{"version":"1.0","entries":[{"grapheme":"B","phoneme":"be"}]}"""
            .toByteArray(Charsets.UTF_8)
        File(targetDir, "user.json").writeBytes(targetContent)
        seedLegacy(names = listOf("user.json", "spelling.json"))
        val migrator = newMigrator()

        // itemCount counts copies only: spelling.json was copied, user.json was not.
        assertEquals(MigrationResult.Migrated(1), migrator.migrateIfNeeded())
        assertTrue(migrator.isDone)
        assertArrayEquals(targetContent, File(targetDir, "user.json").readBytes())
        assertFalse(File(legacyDir, "user.json").exists())
        assertConverged(names = listOf("spelling.json"))
        assertEquals(emptyList<String>(), namesIn(legacyDir))
    }

    @Test
    fun `stale temp files for dictionary names are removed but unrelated files stay`() = runTest {
        val staleMigration = File(
            targetDir,
            "user.json.${DictionaryMigrator.MIGRATION_TEMP_TAG}-${UUID.randomUUID()}${AtomicFiles.TEMP_SUFFIX}"
        )
        staleMigration.writeText("half-written by an earlier migration run")
        val inFlightRepository = File(targetDir, "user.json.${UUID.randomUUID()}${AtomicFiles.TEMP_SUFFIX}")
        inFlightRepository.writeText("being written by DictionaryRepository right now")
        val unrelated = File(targetDir, "unrelated.tmp")
        unrelated.writeText("keep")
        seedLegacy(names = listOf("user.json"))

        val result = newMigrator().migrateIfNeeded()

        assertEquals(MigrationResult.Migrated(1), result)
        assertFalse("the migrator's own stale temp file must be removed", staleMigration.exists())
        assertTrue("another writer's in-flight temp file must be left alone", inFlightRepository.exists())
        assertTrue("unrelated file must stay", unrelated.exists())
        assertArrayEquals(LEGACY_CONTENT.getValue("user.json"), File(targetDir, "user.json").readBytes())
    }

    @Test
    fun `rearm lets the same instance migrate files that appeared later`() = runTest {
        seedLegacy(names = listOf("user.json"))
        val migrator = newMigrator()
        assertEquals(MigrationResult.Migrated(1), migrator.migrateIfNeeded())

        seedLegacy(names = listOf("spelling.json"))
        assertEquals("done flag short-circuits", MigrationResult.NotNeeded, migrator.migrateIfNeeded())
        assertTrue(File(legacyDir, "spelling.json").isFile)

        migrator.rearm()
        assertFalse(migrator.isDone)
        assertEquals(MigrationResult.Migrated(1), migrator.migrateIfNeeded())
        assertTrue(migrator.isDone)
        assertConverged(names = listOf("user.json", "spelling.json"))
    }

    // ==========================================================================
    // Crash Simulation (process dies mid-migration, then retries)
    // ==========================================================================

    @Test
    fun `crash before copy converges on the same instance and after a restart`() = runTest {
        assertCrashConverges(MigrationCrashPoint.DICT_BEFORE_COPY) { legacy, target ->
            assertEquals("nothing was copied yet", emptyList<String>(), namesIn(target))
            assertLegacyIntact(legacy)
        }
    }

    @Test
    fun `crash after temp write leaves the temp file behind and converges`() = runTest {
        assertCrashConverges(MigrationCrashPoint.DICT_AFTER_TMP_WRITE) { legacy, target ->
            val temps = tempFilesIn(target)
            assertEquals("exactly one temp file is left behind", 1, temps.size)
            assertTrue(AtomicFiles.isTempFileFor(temps[0], "user.json"))
            assertArrayEquals(LEGACY_CONTENT.getValue("user.json"), temps[0].readBytes())
            assertFalse("rename must not have happened", File(target, "user.json").exists())
            assertLegacyIntact(legacy)
        }
    }

    @Test
    fun `crash after rename keeps the copied target and converges`() = runTest {
        assertCrashConverges(MigrationCrashPoint.DICT_AFTER_RENAME) { legacy, target ->
            assertArrayEquals(LEGACY_CONTENT.getValue("user.json"), File(target, "user.json").readBytes())
            assertEquals(listOf("user.json"), namesIn(target))
            // The legacy copy was not deleted yet; the retry treats the target as the winner.
            assertLegacyIntact(legacy)
        }
    }

    @Test
    fun `crash after legacy delete continues with the remaining files and converges`() = runTest {
        assertCrashConverges(MigrationCrashPoint.DICT_AFTER_DELETE_LEGACY) { legacy, target ->
            assertArrayEquals(LEGACY_CONTENT.getValue("user.json"), File(target, "user.json").readBytes())
            assertEquals(listOf("user.json"), namesIn(target))
            assertFalse(File(legacy, "user.json").exists())
            assertLegacyIntact(legacy, names = listOf("spelling.json", "emoji.json"))
        }
    }

    // ==========================================================================
    // Failures: Unwritable Target, Undeletable Legacy, Rate Limit
    // ==========================================================================

    @Test
    fun `unwritable target directory throws DirectBootMigrationException and keeps the legacy file`() {
        seedLegacy(names = listOf("user.json"))
        assumeTrue("directory permissions are not enforced here", targetDir.setWritable(false, false))
        assumeFalse("running as root; directory permissions are ignored", targetDir.canWrite())
        val migrator = newMigrator()

        try {
            runTest {
                val error = assertSuspendThrows(DirectBootMigrationException::class.java) {
                    migrator.migrateIfNeeded()
                }
                assertNotNull("wraps the underlying I/O error", error.cause)
                assertTrue("cause is ${error.cause}", error.cause is IOException)
                assertFalse(migrator.isDone)
                assertLegacyIntact(names = listOf("user.json"))
                assertEquals(emptyList<String>(), namesIn(targetDir))

                // The failure is recorded: an immediate retry is rate-limited.
                val limited = migrator.migrateIfNeeded()
                assertTrue("expected RetryLater but was $limited", limited is MigrationResult.RetryLater)
                assertLegacyIntact(names = listOf("user.json"))
            }
        } finally {
            targetDir.setWritable(true)
        }

        runTest {
            // Storage is writable again and the retry interval has elapsed.
            now += LegacyMigrator.DEFAULT_RETRY_INTERVAL_NANOS
            assertEquals(MigrationResult.Migrated(1), migrator.migrateIfNeeded())
            assertTrue(migrator.isDone)
            assertConverged(names = listOf("user.json"))
        }
    }

    @Test
    fun `undeletable legacy file yields RetryLater and converges once it can be deleted`() {
        seedLegacy(names = listOf("user.json"))
        assumeTrue("directory permissions are not enforced here", legacyDir.setWritable(false, false))
        assumeFalse("running as root; directory permissions are ignored", legacyDir.canWrite())
        val migrator = newMigrator()

        try {
            runTest {
                val result = migrator.migrateIfNeeded()
                assertTrue("expected RetryLater but was $result", result is MigrationResult.RetryLater)
                assertNotNull((result as MigrationResult.RetryLater).cause)
                assertFalse(migrator.isDone)
                // The copy itself succeeded; only the legacy cleanup is pending.
                assertArrayEquals(LEGACY_CONTENT.getValue("user.json"), File(targetDir, "user.json").readBytes())
                assertEquals(emptyList<File>(), tempFilesIn(targetDir))
                assertLegacyIntact(names = listOf("user.json"))
            }
        } finally {
            legacyDir.setWritable(true)
        }

        runTest {
            // Rate limit: an immediate retry is refused even though the delete would now work.
            val limited = migrator.migrateIfNeeded()
            assertTrue("expected RetryLater but was $limited", limited is MigrationResult.RetryLater)
            assertFalse(migrator.isDone)
            assertTrue(File(legacyDir, "user.json").isFile)

            now += LegacyMigrator.DEFAULT_RETRY_INTERVAL_NANOS
            // The target already exists, so nothing is copied; the legacy file is finally removed.
            assertEquals(MigrationResult.Migrated(0), migrator.migrateIfNeeded())
            assertTrue(migrator.isDone)
            assertConverged(names = listOf("user.json"))
        }
    }

    // ==========================================================================
    // Concurrency
    // ==========================================================================

    @Test
    fun `concurrent calls on one instance migrate exactly once`() = runTest {
        seedLegacy()
        val migrator = newMigrator()

        val results = coroutineScope {
            List(8) { async(Dispatchers.Default) { migrator.migrateIfNeeded() } }.awaitAll()
        }

        assertEquals("exactly one Migrated result in $results", 1, results.count { it is MigrationResult.Migrated })
        assertEquals(MigrationResult.Migrated(3), results.first { it is MigrationResult.Migrated })
        assertEquals(7, results.count { it == MigrationResult.NotNeeded })
        assertTrue(migrator.isDone)
        assertConverged()
        assertEquals(FILE_NAMES.sorted(), namesIn(targetDir))
    }

    // ==========================================================================
    // Helpers
    // ==========================================================================

    private fun newMigrator(
        legacy: () -> File = { legacyDir },
        target: File = targetDir,
        isUnlocked: () -> Boolean = { true },
        faultInjector: MigrationFaultInjector = MigrationFaultInjector.NONE,
    ): DictionaryMigrator = DictionaryMigrator(
        legacyDir = legacy,
        targetDir = target,
        fileNames = FILE_NAMES,
        isUserUnlocked = isUnlocked,
        faultInjector = faultInjector,
        logger = StorageLogger.None,
        clock = { now }
    )

    private fun seedLegacy(dir: File = legacyDir, names: Collection<String> = FILE_NAMES) {
        for (name in names) {
            File(dir, name).writeBytes(LEGACY_CONTENT.getValue(name))
        }
    }

    private fun namesIn(dir: File): List<String> = dir.listFiles().orEmpty().map { it.name }.sorted()

    private fun tempFilesIn(dir: File): List<File> =
        dir.listFiles().orEmpty().filter { it.name.endsWith(AtomicFiles.TEMP_SUFFIX) }

    private fun assertLegacyIntact(dir: File = legacyDir, names: Collection<String> = FILE_NAMES) {
        for (name in names) {
            assertArrayEquals("legacy $name", LEGACY_CONTENT.getValue(name), File(dir, name).readBytes())
        }
    }

    /**
     * Every file in [names] has the legacy content in [target], is gone from
     * [legacy], and no temp file is left in [target].
     */
    private fun assertConverged(
        legacy: File = legacyDir,
        target: File = targetDir,
        names: Collection<String> = FILE_NAMES,
    ) {
        for (name in names) {
            assertArrayEquals("target $name", LEGACY_CONTENT.getValue(name), File(target, name).readBytes())
            assertFalse("legacy $name must be deleted", File(legacy, name).exists())
        }
        assertEquals("no temp files may remain", emptyList<File>(), tempFilesIn(target))
    }

    /**
     * Seeds all three files, crashes once at [point], lets [inspectAfterCrash]
     * verify the on-disk state a real crash would leave, then checks that
     * (a) a second call on the same instance and
     * (b) a fresh migrator over the same directories (simulated process restart)
     * both converge on the fully migrated state.
     */
    private suspend fun assertCrashConverges(
        point: MigrationCrashPoint,
        inspectAfterCrash: (legacy: File, target: File) -> Unit,
    ) {
        run {
            val legacy = tempFolder.newFolder("crash-same-legacy")
            val target = tempFolder.newFolder("crash-same-target")
            val crashed = crashOnce(point, legacy, target)
            inspectAfterCrash(legacy, target)
            assertRecovers(crashed, legacy, target)
        }
        run {
            val legacy = tempFolder.newFolder("crash-restart-legacy")
            val target = tempFolder.newFolder("crash-restart-target")
            crashOnce(point, legacy, target)
            inspectAfterCrash(legacy, target)
            val fresh = newMigrator(legacy = { legacy }, target = target)
            assertRecovers(fresh, legacy, target)
        }
    }

    private suspend fun crashOnce(point: MigrationCrashPoint, legacy: File, target: File): DictionaryMigrator {
        seedLegacy(legacy)
        val injector = OneShotCrashInjector(point)
        val migrator = newMigrator(legacy = { legacy }, target = target, faultInjector = injector)

        val crash = assertSuspendThrows(SimulatedMigrationCrashException::class.java) {
            migrator.migrateIfNeeded()
        }
        assertEquals(point, crash.point)
        assertTrue(injector.fired)
        assertFalse("a crash must not mark the migration done", migrator.isDone)
        return migrator
    }

    private suspend fun assertRecovers(migrator: DictionaryMigrator, legacy: File, target: File) {
        // Files already renamed into place before the crash are not copied again.
        val alreadyPresent = FILE_NAMES.count { File(target, it).isFile }

        val result = migrator.migrateIfNeeded()

        assertEquals(MigrationResult.Migrated(FILE_NAMES.size - alreadyPresent), result)
        assertTrue(migrator.isDone)
        assertConverged(legacy, target)
        assertEquals(FILE_NAMES.sorted(), namesIn(target))
        assertEquals(emptyList<String>(), namesIn(legacy))
        assertEquals(MigrationResult.NotNeeded, migrator.migrateIfNeeded())
    }

    /** assertThrows for suspending code: returns the thrown [type] or fails. */
    private suspend fun <T : Throwable> assertSuspendThrows(type: Class<T>, block: suspend () -> Unit): T {
        val thrown = try {
            block()
            null
        } catch (e: Throwable) {
            e
        }
        assertNotNull("expected ${type.simpleName} to be thrown", thrown)
        assertTrue("expected ${type.simpleName} but got $thrown", type.isInstance(thrown))
        return type.cast(thrown)
    }

    companion object {
        private val FILE_NAMES = listOf("user.json", "spelling.json", "emoji.json")

        /** Non-ASCII payloads so a byte-for-byte comparison also covers encoding. */
        private val LEGACY_CONTENT: Map<String, ByteArray> = mapOf(
            "user.json" to """{"version":"1.0","entries":[{"grapheme":"Đakovo","phoneme":"Đakovo","caseSensitive":true,"wholeWord":true,"comment":"čćžšđ ČĆŽŠĐ"}]}"""
                .toByteArray(Charsets.UTF_8),
            "spelling.json" to """{"version":"1.0","entries":[{"grapheme":"Č","phoneme":"Če"},{"grapheme":"Ž","phoneme":"Že"}]}"""
                .toByteArray(Charsets.UTF_8),
            "emoji.json" to """{"version":"1.0","entries":[{"grapheme":"😀","phoneme":"nasmijano lice"},{"grapheme":"🇭🇷","phoneme":"hrvatska zastava"}]}"""
                .toByteArray(Charsets.UTF_8)
        )
    }
}
