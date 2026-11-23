package com.hrvojekatic.laprdus.data.migration

import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.MutablePreferences
import androidx.datastore.preferences.core.PreferenceDataStoreFactory
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.PreferencesFileSerializer
import androidx.datastore.preferences.core.booleanPreferencesKey
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.floatPreferencesKey
import androidx.datastore.preferences.core.intPreferencesKey
import androidx.datastore.preferences.core.mutablePreferencesOf
import androidx.datastore.preferences.core.stringPreferencesKey
import com.hrvojekatic.laprdus.data.storage.StorageLogger
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.Job
import kotlinx.coroutines.async
import kotlinx.coroutines.awaitAll
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.cancel
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.job
import kotlinx.coroutines.test.runTest
import org.junit.After
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertSame
import org.junit.Assert.assertTrue
import org.junit.Assume.assumeFalse
import org.junit.Assume.assumeTrue
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File
import java.io.IOException
import java.util.concurrent.atomic.AtomicInteger

/**
 * Unit tests for [SettingsMigrator]: the move of the legacy credential-encrypted
 * settings DataStore file into the device-protected settings DataStore.
 *
 * Plain JVM: the migrator has no android.* references. The legacy file is
 * seeded with the public [PreferencesFileSerializer] (no DataStore is ever
 * opened on it), the target is a real DataStore over a temp file with the
 * same harness as SettingsRepositoryTest, the unlock gate is a lambda, the
 * retry clock is a fake, and mid-migration crashes are simulated with
 * [OneShotCrashInjector].
 */
@OptIn(ExperimentalCoroutinesApi::class)
class SettingsMigratorTest {

    @get:Rule
    val tempFolder = TemporaryFolder()

    /**
     * On-disk layout of one simulated app data directory:
     * `<name>-ce/datastore/laprdus_settings.preferences_pb` is the legacy
     * (credential-encrypted) DataStore file, `<name>-de/laprdus_settings.preferences_pb`
     * the device-protected target.
     */
    private inner class Layout(name: String) {
        val ceDir: File = tempFolder.newFolder("$name-ce")
        val legacyDir: File = File(ceDir, "datastore")
        val legacyFile: File = File(legacyDir, SETTINGS_FILE_NAME)
        val deDir: File = tempFolder.newFolder("$name-de")
        val targetFile: File = File(deDir, SETTINGS_FILE_NAME)
    }

    /**
     * One DataStore instance over [file] with its own scope, mirroring the
     * production wiring. DataStore refuses two live instances on one file, so
     * [close] cancels the scope (which closes the file connection) before a
     * "restarted process" opens the same file again.
     */
    private inner class OpenStore(val file: File) {
        val scope = CoroutineScope(Dispatchers.IO + Job())
        val store: DataStore<Preferences> = PreferenceDataStoreFactory.create(
            scope = scope,
            produceFile = { file }
        )

        suspend fun close() {
            scope.cancel()
            scope.coroutineContext.job.join()
        }
    }

    private lateinit var layout: Layout

    /** Fake monotonic clock (nanoseconds) behind the retry rate limit. */
    private var now = 0L

    private val openStores = mutableListOf<OpenStore>()

    @Before
    fun setup() {
        layout = Layout("main")
        now = 0L
    }

    @After
    fun cleanup() {
        openStores.forEach { it.scope.cancel() }
        openStores.clear()
    }

    // ==========================================================================
    // Nothing To Migrate
    // ==========================================================================

    @Test
    fun `missing legacy file returns NotNeeded and marks the migration done`() = runTest {
        val store = openStore()
        var legacyResolved = 0
        val migrator = newMigrator(store.store, legacy = { legacyResolved++; layout.legacyFile })

        assertEquals(MigrationResult.NotNeeded, migrator.migrateIfNeeded())

        assertTrue(migrator.isDone)
        assertEquals("the legacy path is resolved once to check for the file", 1, legacyResolved)
        assertTargetUntouched(store.store)
        assertEquals(MigrationResult.NotNeeded, migrator.migrateIfNeeded())
        assertEquals("the done flag short-circuits later calls", 1, legacyResolved)
    }

    @Test
    fun `legacy path that is a directory is treated as absent`() = runTest {
        layout.legacyFile.mkdirs()
        val store = openStore()
        val migrator = newMigrator(store.store)

        assertEquals(MigrationResult.NotNeeded, migrator.migrateIfNeeded())

        assertTrue(migrator.isDone)
        assertTrue("nothing may be deleted", layout.legacyFile.isDirectory)
        assertTargetUntouched(store.store)
    }

    // ==========================================================================
    // Unlock Gate
    // ==========================================================================

    @Test
    fun `locked user returns SkippedLocked without resolving the legacy path or touching the target`() = runTest {
        seedLegacy()
        val store = openStore()
        var unlocked = false
        var legacyResolved = 0
        val migrator = newMigrator(
            store.store,
            legacy = {
                // Resolving a credential-encrypted path while locked is itself a
                // StrictMode violation, so the lambda must never run here.
                assertTrue("legacy path must not be resolved while locked", unlocked)
                legacyResolved++
                layout.legacyFile
            },
            isUnlocked = { unlocked }
        )

        assertEquals(MigrationResult.SkippedLocked, migrator.migrateIfNeeded())
        assertEquals(MigrationResult.SkippedLocked, migrator.migrateIfNeeded())

        assertEquals(0, legacyResolved)
        assertFalse(migrator.isDone)
        assertTargetUntouched(store.store)
        assertEquals("legacy file must be intact", FULL_LEGACY.asMap(), readLegacy().asMap())

        // ACTION_USER_UNLOCKED: the same instance migrates without a rearm,
        // because a skipped attempt records no failure.
        unlocked = true
        assertEquals(MigrationResult.Migrated(FULL_KEY_COUNT), migrator.migrateIfNeeded())
        assertEquals(1, legacyResolved)
        assertTrue(migrator.isDone)
        assertConverged(store.store)
    }

    // ==========================================================================
    // Successful Migration
    // ==========================================================================

    @Test
    fun `unlocked user copies all 16 keys and removes the legacy file with its empty datastore directory`() = runTest {
        assertEquals("the seed must cover every preference key", 16, FULL_KEY_COUNT)
        seedLegacy()
        val store = openStore()
        val migrator = newMigrator(store.store)

        assertEquals(MigrationResult.Migrated(16), migrator.migrateIfNeeded())

        assertTrue(migrator.isDone)
        assertConverged(store.store)
        // Types survive the round trip, not only the values.
        val prefs = store.store.data.first()
        assertEquals("vlado", prefs[KEY_DEFAULT_VOICE])
        assertEquals(1.5f, prefs[KEY_SPEED])
        assertEquals(250, prefs[KEY_SENTENCE_PAUSE])
        assertEquals(true, prefs[KEY_FORCE_SPEED])
        assertEquals(false, prefs[KEY_INFLECTION_ENABLED])
        assertEquals(MigrationResult.NotNeeded, migrator.migrateIfNeeded())
    }

    @Test
    fun `non-empty legacy datastore directory is kept`() = runTest {
        seedLegacy()
        val sibling = File(layout.legacyDir, "other.preferences_pb")
        sibling.writeText("belongs to someone else")
        val store = openStore()
        val migrator = newMigrator(store.store)

        assertEquals(MigrationResult.Migrated(FULL_KEY_COUNT), migrator.migrateIfNeeded())

        assertFalse(layout.legacyFile.exists())
        assertTrue("directory with other files must survive", layout.legacyDir.isDirectory)
        assertEquals("belongs to someone else", sibling.readText())
        assertEquals(FULL_LEGACY.asMap(), store.store.data.first().asMap())
        assertNoTempFiles()
    }

    @Test
    fun `existing target keys win and legacy values only fill the gaps`() = runTest {
        val store = openStore()
        store.store.edit { it[KEY_SPEED] = 1.5f }
        seedLegacy(mutablePreferencesOf(KEY_SPEED to 0.7f, KEY_DEFAULT_VOICE to "vlado").toPreferences())
        val migrator = newMigrator(store.store)

        // itemCount counts copied keys only: default_voice was copied, speed was not.
        assertEquals(MigrationResult.Migrated(1), migrator.migrateIfNeeded())

        assertTrue(migrator.isDone)
        val expected = mutablePreferencesOf(KEY_SPEED to 1.5f, KEY_DEFAULT_VOICE to "vlado").toPreferences()
        assertConverged(store.store, expected = expected)
    }

    @Test
    fun `legacy identical to the target copies nothing but still cleans up`() = runTest {
        val store = openStore()
        store.store.edit { prefs -> FULL_LEGACY.asMap().forEach { (key, value) -> prefs.setUnchecked(key, value) } }
        seedLegacy()
        val migrator = newMigrator(store.store)

        assertEquals(MigrationResult.Migrated(0), migrator.migrateIfNeeded())

        assertTrue(migrator.isDone)
        assertConverged(store.store)
    }

    @Test
    fun `second call on the same instance is a no-op that does not touch the files`() = runTest {
        seedLegacy()
        val store = openStore()
        var legacyResolved = 0
        val migrator = newMigrator(store.store, legacy = { legacyResolved++; layout.legacyFile })
        assertEquals(MigrationResult.Migrated(FULL_KEY_COUNT), migrator.migrateIfNeeded())
        assertEquals(1, legacyResolved)

        // A legacy file that reappears (restored backup) is ignored until a rearm.
        seedLegacy(mutablePreferencesOf(KEY_SPEED to 0.6f).toPreferences())
        val bytesBefore = layout.legacyFile.readBytes()

        assertEquals(MigrationResult.NotNeeded, migrator.migrateIfNeeded())

        assertEquals("the done flag short-circuits before the legacy path is resolved", 1, legacyResolved)
        assertArrayEquals("legacy file must be untouched", bytesBefore, layout.legacyFile.readBytes())
        assertEquals(FULL_LEGACY.asMap(), store.store.data.first().asMap())
    }

    @Test
    fun `rearm resets the done flag so the same instance migrates again`() = runTest {
        seedLegacy(mutablePreferencesOf(KEY_DEFAULT_VOICE to "vlado", KEY_SPEED to 1.5f).toPreferences())
        val store = openStore()
        val migrator = newMigrator(store.store)
        assertEquals(MigrationResult.Migrated(2), migrator.migrateIfNeeded())
        assertTrue(migrator.isDone)

        seedLegacy(mutablePreferencesOf(KEY_SPEED to 0.6f, KEY_PITCH to 0.8f).toPreferences())
        assertEquals(MigrationResult.NotNeeded, migrator.migrateIfNeeded())
        assertTrue(layout.legacyFile.isFile)

        migrator.rearm()

        assertFalse(migrator.isDone)
        // pitch is new and gets copied; speed already exists in the target and wins.
        assertEquals(MigrationResult.Migrated(1), migrator.migrateIfNeeded())
        assertTrue(migrator.isDone)
        val expected = mutablePreferencesOf(
            KEY_DEFAULT_VOICE to "vlado",
            KEY_SPEED to 1.5f,
            KEY_PITCH to 0.8f
        ).toPreferences()
        assertConverged(store.store, expected = expected)
    }

    // ==========================================================================
    // Corrupt Legacy File
    // ==========================================================================

    @Test
    fun `corrupt legacy file is quarantined and the target is left untouched`() = runTest {
        layout.legacyDir.mkdirs()
        layout.legacyFile.writeBytes(CORRUPT_BYTES)
        val store = openStore()
        val migrator = newMigrator(store.store)

        assertEquals(MigrationResult.QuarantinedCorrupt, migrator.migrateIfNeeded())

        assertTrue(migrator.isDone)
        val quarantined = File(layout.legacyDir, SETTINGS_FILE_NAME + SettingsMigrator.CORRUPT_SUFFIX)
        assertTrue("corrupt file must be renamed, not deleted", quarantined.isFile)
        assertArrayEquals(CORRUPT_BYTES, quarantined.readBytes())
        assertFalse(layout.legacyFile.exists())
        assertTargetUntouched(store.store)
        assertNoTempFiles()
        assertEquals(MigrationResult.NotNeeded, migrator.migrateIfNeeded())
    }

    @Test
    fun `quarantine replaces an older corrupt copy`() = runTest {
        layout.legacyDir.mkdirs()
        val quarantined = File(layout.legacyDir, SETTINGS_FILE_NAME + SettingsMigrator.CORRUPT_SUFFIX)
        quarantined.writeText("older quarantine")
        layout.legacyFile.writeBytes(CORRUPT_BYTES)
        val store = openStore()
        val migrator = newMigrator(store.store)

        assertEquals(MigrationResult.QuarantinedCorrupt, migrator.migrateIfNeeded())

        assertArrayEquals(CORRUPT_BYTES, quarantined.readBytes())
        assertFalse(layout.legacyFile.exists())
        assertEquals(listOf(quarantined.name), layout.legacyDir.list().orEmpty().toList())
    }

    // ==========================================================================
    // Unreadable Legacy File: RetryLater And Rate Limit
    // ==========================================================================

    @Test
    fun `unreadable legacy file returns RetryLater and leaves the migration pending`() = runTest {
        val store = openStore()
        var legacyResolved = 0
        val migrator = newMigrator(store.store, legacy = { legacyResolved++; directoryPosingAsFile() })

        val result = migrator.migrateIfNeeded()

        assertTrue("expected RetryLater but was $result", result is MigrationResult.RetryLater)
        val cause = (result as MigrationResult.RetryLater).cause
        assertTrue("cause is $cause", cause is IOException)
        assertFalse(migrator.isDone)
        assertEquals(1, legacyResolved)
        assertTargetUntouched(store.store)
    }

    @Test
    fun `retry is rate-limited until the interval elapsed`() = runTest {
        seedLegacy()
        val store = openStore()
        var legacyResolved = 0
        var legacy: File = directoryPosingAsFile()
        val migrator = newMigrator(store.store, legacy = { legacyResolved++; legacy })

        val first = migrator.migrateIfNeeded()
        assertTrue("expected RetryLater but was $first", first is MigrationResult.RetryLater)
        assertEquals(1, legacyResolved)

        // Storage is fine again, but the retry window has not passed: no new attempt.
        legacy = layout.legacyFile
        now += LegacyMigrator.DEFAULT_RETRY_INTERVAL_NANOS - 1
        val limited = migrator.migrateIfNeeded()
        assertTrue("expected RetryLater but was $limited", limited is MigrationResult.RetryLater)
        assertSame(
            "the recorded failure is reported while rate-limited",
            (first as MigrationResult.RetryLater).cause,
            (limited as MigrationResult.RetryLater).cause
        )
        assertEquals("no attempt while rate-limited", 1, legacyResolved)
        assertFalse(migrator.isDone)
        assertTrue(layout.legacyFile.isFile)

        // Interval elapsed: the next call attempts again and succeeds.
        now += 1
        assertEquals(MigrationResult.Migrated(FULL_KEY_COUNT), migrator.migrateIfNeeded())
        assertEquals(2, legacyResolved)
        assertTrue(migrator.isDone)
        assertConverged(store.store)
    }

    @Test
    fun `rearm clears the rate limit so the next call retries immediately`() = runTest {
        seedLegacy()
        val store = openStore()
        var legacyResolved = 0
        var legacy: File = directoryPosingAsFile()
        val migrator = newMigrator(store.store, legacy = { legacyResolved++; legacy })
        assertTrue(migrator.migrateIfNeeded() is MigrationResult.RetryLater)
        legacy = layout.legacyFile
        assertTrue("still rate-limited", migrator.migrateIfNeeded() is MigrationResult.RetryLater)
        assertEquals(1, legacyResolved)

        migrator.rearm()

        assertFalse(migrator.isDone)
        assertEquals(MigrationResult.Migrated(FULL_KEY_COUNT), migrator.migrateIfNeeded())
        assertEquals(2, legacyResolved)
        assertTrue(migrator.isDone)
        assertConverged(store.store)
    }

    // ==========================================================================
    // Crash Simulation (process dies mid-migration, then retries)
    // ==========================================================================

    @Test
    fun `crash before reading the legacy file converges on the same instance and after a restart`() = runTest {
        assertCrashConverges(
            MigrationCrashPoint.SETTINGS_BEFORE_READ_LEGACY,
            expectedRecovery = MigrationResult.Migrated(FULL_KEY_COUNT)
        ) { crashed, store ->
            assertTargetUntouched(store, crashed)
            assertEquals(FULL_LEGACY.asMap(), readLegacy(crashed.legacyFile).asMap())
        }
    }

    @Test
    fun `crash after reading the legacy file converges on the same instance and after a restart`() = runTest {
        assertCrashConverges(
            MigrationCrashPoint.SETTINGS_AFTER_READ_LEGACY,
            expectedRecovery = MigrationResult.Migrated(FULL_KEY_COUNT)
        ) { crashed, store ->
            assertTargetUntouched(store, crashed)
            assertEquals(FULL_LEGACY.asMap(), readLegacy(crashed.legacyFile).asMap())
        }
    }

    @Test
    fun `crash after the commit keeps the copied keys and converges without copying again`() = runTest {
        assertCrashConverges(
            MigrationCrashPoint.SETTINGS_AFTER_COMMIT,
            expectedRecovery = MigrationResult.Migrated(0)
        ) { crashed, store ->
            assertEquals("the commit is atomic and complete", FULL_LEGACY.asMap(), store.data.first().asMap())
            assertTrue(crashed.targetFile.isFile)
            // The legacy file was not deleted yet; the retry treats the target as the winner.
            assertEquals(FULL_LEGACY.asMap(), readLegacy(crashed.legacyFile).asMap())
        }
    }

    @Test
    fun `crash after deleting the legacy file converges as NotNeeded`() = runTest {
        assertCrashConverges(
            MigrationCrashPoint.SETTINGS_AFTER_DELETE_LEGACY,
            expectedRecovery = MigrationResult.NotNeeded
        ) { crashed, store ->
            assertEquals(FULL_LEGACY.asMap(), store.data.first().asMap())
            assertFalse(crashed.legacyFile.exists())
            assertFalse("empty legacy datastore directory is already gone", crashed.legacyDir.exists())
        }
    }

    // ==========================================================================
    // Unwritable Target
    // ==========================================================================

    @Test
    fun `unwritable target throws DirectBootMigrationException and keeps the legacy file`() {
        assumeFalse("running as root; directory permissions are ignored", System.getProperty("user.name") == "root")
        runTest { seedLegacy() }
        val store = openStore()
        assumeTrue("directory permissions are not enforced here", layout.deDir.setWritable(false, false))
        assumeFalse("directory is still writable; permissions are not enforced", layout.deDir.canWrite())
        val migrator = newMigrator(store.store)

        try {
            runTest {
                val error = assertSuspendThrows(DirectBootMigrationException::class.java) {
                    migrator.migrateIfNeeded()
                }
                assertNotNull("wraps the underlying I/O error", error.cause)
                assertTrue("cause is ${error.cause}", error.cause is IOException)
                assertFalse(migrator.isDone)
                assertEquals("legacy file must be intact", FULL_LEGACY.asMap(), readLegacy().asMap())
                assertFalse("nothing may be written to the target", layout.targetFile.exists())

                // The failure is recorded: an immediate retry is rate-limited.
                val limited = migrator.migrateIfNeeded()
                assertTrue("expected RetryLater but was $limited", limited is MigrationResult.RetryLater)
                assertSame(error, (limited as MigrationResult.RetryLater).cause)
                assertTrue(layout.legacyFile.isFile)
            }
        } finally {
            layout.deDir.setWritable(true)
        }

        runTest {
            // Storage is writable again and the retry interval has elapsed.
            now += LegacyMigrator.DEFAULT_RETRY_INTERVAL_NANOS
            assertEquals(MigrationResult.Migrated(FULL_KEY_COUNT), migrator.migrateIfNeeded())
            assertTrue(migrator.isDone)
            assertConverged(store.store)
        }
    }

    // ==========================================================================
    // Concurrency
    // ==========================================================================

    @Test
    fun `concurrent calls on one instance migrate exactly once`() = runTest {
        seedLegacy()
        val store = openStore()
        val legacyResolved = AtomicInteger(0)
        val migrator = newMigrator(store.store, legacy = { legacyResolved.incrementAndGet(); layout.legacyFile })

        val results = coroutineScope {
            List(8) { async(Dispatchers.Default) { migrator.migrateIfNeeded() } }.awaitAll()
        }

        assertEquals("exactly one Migrated result in $results", 1, results.count { it is MigrationResult.Migrated })
        assertEquals(MigrationResult.Migrated(FULL_KEY_COUNT), results.first { it is MigrationResult.Migrated })
        assertEquals(7, results.count { it == MigrationResult.NotNeeded })
        assertEquals("the legacy file is read exactly once", 1, legacyResolved.get())
        assertTrue(migrator.isDone)
        assertConverged(store.store)
    }

    // ==========================================================================
    // Helpers
    // ==========================================================================

    private fun openStore(file: File = layout.targetFile): OpenStore =
        OpenStore(file).also { openStores += it }

    private fun newMigrator(
        target: DataStore<Preferences>,
        legacy: () -> File = { layout.legacyFile },
        isUnlocked: () -> Boolean = { true },
        faultInjector: MigrationFaultInjector = MigrationFaultInjector.NONE,
    ): SettingsMigrator = SettingsMigrator(
        legacyFile = legacy,
        target = target,
        isUserUnlocked = isUnlocked,
        faultInjector = faultInjector,
        logger = StorageLogger.None,
        clock = { now }
    )

    /** Writes [prefs] as a legacy DataStore file without opening a DataStore on it. */
    private suspend fun seedLegacy(prefs: Preferences = FULL_LEGACY, file: File = layout.legacyFile) {
        file.parentFile!!.mkdirs()
        file.outputStream().use { PreferencesFileSerializer.writeTo(prefs, it) }
        assertTrue("seeding must produce a regular file", file.isFile)
    }

    private suspend fun readLegacy(file: File = layout.legacyFile): Preferences =
        file.inputStream().use { PreferencesFileSerializer.readFrom(it) }

    /**
     * A directory that claims to be a regular file, so the migrator passes its
     * `isFile` gate and then fails to open it for reading with an [IOException]
     * (the JVM refuses to open a directory as a stream). Deterministic on every
     * platform and unaffected by running as root, unlike permission bits.
     */
    private fun directoryPosingAsFile(): File = object : File(tempFolder.newFolder().path) {
        override fun isFile(): Boolean = true
    }

    private fun tempFilesUnder(root: File): List<String> =
        root.walkTopDown().filter { it.isFile && it.name.endsWith(".tmp") }.map { it.path }.sorted().toList()

    /** Neither the migrator nor DataStore may leave a temp file behind anywhere. */
    private fun assertNoTempFiles() {
        assertEquals("no temp files may remain", emptyList<String>(), tempFilesUnder(tempFolder.root))
    }

    private suspend fun assertTargetUntouched(store: DataStore<Preferences>, at: Layout = layout) {
        assertEquals("target store must be empty", emptyMap<Preferences.Key<*>, Any>(), store.data.first().asMap())
        assertFalse("nothing may be written to the target file", at.targetFile.exists())
        assertNoTempFiles()
    }

    /**
     * The target holds exactly [expected], the legacy file and its now-empty
     * `datastore` directory are gone, and no temp file is left anywhere.
     */
    private suspend fun assertConverged(
        store: DataStore<Preferences>,
        at: Layout = layout,
        expected: Preferences = FULL_LEGACY,
    ) {
        assertEquals(expected.asMap(), store.data.first().asMap())
        assertTrue("target file must exist", at.targetFile.isFile)
        assertFalse("legacy file must be deleted", at.legacyFile.exists())
        assertFalse("empty legacy datastore directory must be removed", at.legacyDir.exists())
        assertTrue("the legacy files directory itself stays", at.ceDir.isDirectory)
        assertNoTempFiles()
    }

    /**
     * Seeds all keys, crashes once at [point], lets [inspectAfterCrash] verify
     * the on-disk state a real crash would leave, then checks that
     * (a) a second call on the same instance and
     * (b) a fresh migrator over a fresh DataStore on the same files
     *     (simulated process restart)
     * both converge on the fully migrated state with [expectedRecovery].
     */
    private suspend fun assertCrashConverges(
        point: MigrationCrashPoint,
        expectedRecovery: MigrationResult,
        inspectAfterCrash: suspend (crashed: Layout, store: DataStore<Preferences>) -> Unit,
    ) {
        run {
            val at = Layout("crash-same")
            val store = openStore(at.targetFile)
            val crashed = crashOnce(point, at, store.store)
            inspectAfterCrash(at, store.store)
            assertRecovers(crashed, at, store.store, expectedRecovery)
        }
        run {
            val at = Layout("crash-restart")
            val first = openStore(at.targetFile)
            crashOnce(point, at, first.store)
            inspectAfterCrash(at, first.store)
            first.close()
            val restarted = openStore(at.targetFile)
            val fresh = newMigrator(restarted.store, legacy = { at.legacyFile })
            assertRecovers(fresh, at, restarted.store, expectedRecovery)
        }
    }

    private suspend fun crashOnce(point: MigrationCrashPoint, at: Layout, store: DataStore<Preferences>): SettingsMigrator {
        seedLegacy(file = at.legacyFile)
        val injector = OneShotCrashInjector(point)
        val migrator = newMigrator(store, legacy = { at.legacyFile }, faultInjector = injector)

        val crash = assertSuspendThrows(SimulatedMigrationCrashException::class.java) {
            migrator.migrateIfNeeded()
        }
        assertEquals(point, crash.point)
        assertTrue(injector.fired)
        assertFalse("a crash must not mark the migration done", migrator.isDone)
        return migrator
    }

    private suspend fun assertRecovers(
        migrator: SettingsMigrator,
        at: Layout,
        store: DataStore<Preferences>,
        expected: MigrationResult,
    ) {
        assertEquals(expected, migrator.migrateIfNeeded())
        assertTrue(migrator.isDone)
        assertConverged(store, at)
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
        /** Same file name as LaprdusStorage.SETTINGS_FILE_NAME (DataStore requires this extension). */
        private const val SETTINGS_FILE_NAME = "laprdus_settings.preferences_pb"

        // Same names and types as the private keys in SettingsRepository.
        private val KEY_DEFAULT_VOICE = stringPreferencesKey("default_voice")
        private val KEY_SPEED = floatPreferencesKey("speed")
        private val KEY_PITCH = floatPreferencesKey("pitch")
        private val KEY_VOLUME = floatPreferencesKey("volume")
        private val KEY_FORCE_SPEED = booleanPreferencesKey("force_speed")
        private val KEY_FORCE_PITCH = booleanPreferencesKey("force_pitch")
        private val KEY_FORCE_VOLUME = booleanPreferencesKey("force_volume")
        private val KEY_FORCE_LANGUAGE = booleanPreferencesKey("force_language")
        private val KEY_EMOJI_ENABLED = booleanPreferencesKey("emoji_enabled")
        private val KEY_INFLECTION_ENABLED = booleanPreferencesKey("inflection_enabled")
        private val KEY_SENTENCE_PAUSE = intPreferencesKey("sentence_pause")
        private val KEY_COMMA_PAUSE = intPreferencesKey("comma_pause")
        private val KEY_NEWLINE_PAUSE = intPreferencesKey("newline_pause")
        private val KEY_NUMBER_MODE = intPreferencesKey("number_mode")
        private val KEY_DONT_ASK_DEFAULT_TTS = booleanPreferencesKey("dont_ask_default_tts")
        private val KEY_USER_DICTIONARIES_ENABLED = booleanPreferencesKey("user_dictionaries_enabled")

        /** Every key with a non-default value, so a missed copy is always visible. */
        private val FULL_LEGACY: Preferences = mutablePreferencesOf(
            KEY_DEFAULT_VOICE to "vlado",
            KEY_SPEED to 1.5f,
            KEY_PITCH to 0.8f,
            KEY_VOLUME to 0.7f,
            KEY_FORCE_SPEED to true,
            KEY_FORCE_PITCH to true,
            KEY_FORCE_VOLUME to true,
            KEY_FORCE_LANGUAGE to true,
            KEY_EMOJI_ENABLED to true,
            KEY_INFLECTION_ENABLED to false,
            KEY_SENTENCE_PAUSE to 250,
            KEY_COMMA_PAUSE to 150,
            KEY_NEWLINE_PAUSE to 300,
            KEY_NUMBER_MODE to 1,
            KEY_DONT_ASK_DEFAULT_TTS to true,
            KEY_USER_DICTIONARIES_ENABLED to false
        ).toPreferences()

        private val FULL_KEY_COUNT: Int = FULL_LEGACY.asMap().size

        /**
         * Protobuf tags with wire type 7, which does not exist: every parser
         * rejects them, unlike random bytes that may decode as unknown fields.
         */
        private val CORRUPT_BYTES = byteArrayOf(0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F)

        @Suppress("UNCHECKED_CAST")
        private fun MutablePreferences.setUnchecked(key: Preferences.Key<*>, value: Any) {
            this[key as Preferences.Key<Any>] = value
        }
    }
}
