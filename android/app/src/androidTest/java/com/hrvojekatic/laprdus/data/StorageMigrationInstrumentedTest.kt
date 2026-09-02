package com.hrvojekatic.laprdus.data

import android.content.Context
import androidx.core.os.UserManagerCompat
import androidx.datastore.core.DataStore
import androidx.datastore.core.deviceProtectedDataStoreFile
import androidx.datastore.preferences.core.PreferenceDataStoreFactory
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.PreferencesFileSerializer
import androidx.datastore.preferences.core.booleanPreferencesKey
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.floatPreferencesKey
import androidx.datastore.preferences.core.intPreferencesKey
import androidx.datastore.preferences.core.mutablePreferencesOf
import androidx.datastore.preferences.core.stringPreferencesKey
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import com.hrvojekatic.laprdus.data.migration.DictionaryMigrator
import com.hrvojekatic.laprdus.data.migration.MigrationCrashPoint
import com.hrvojekatic.laprdus.data.migration.MigrationFaultInjector
import com.hrvojekatic.laprdus.data.migration.MigrationResult
import com.hrvojekatic.laprdus.data.migration.OneShotCrashInjector
import com.hrvojekatic.laprdus.data.migration.SettingsMigrator
import com.hrvojekatic.laprdus.data.migration.SimulatedMigrationCrashException
import com.hrvojekatic.laprdus.data.storage.LaprdusStorage
import com.hrvojekatic.laprdus.data.storage.StorageLogger
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancelAndJoin
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File
import java.util.UUID

/**
 * Instrumented tests for the credential-encrypted (CE) to device-protected
 * (DE) storage migration on a real device: [SettingsMigrator] and
 * [DictionaryMigrator] run against the real CE and DE file systems, and the
 * repositories load what was migrated.
 *
 * Hermetic by construction: every test gets its own subdirectory under the
 * real CE files dir (`filesDir/migration_test/<uuid>`) and under the real DE
 * files dir (`createDeviceProtectedStorageContext().filesDir/migration_test/<uuid>`),
 * both deleted afterwards. The production settings DataStore file and the
 * production dictionary files are never opened, read or written; the settings
 * DataStore used here is a separate instance over a file inside the test DE
 * directory. Legacy files are seeded with [PreferencesFileSerializer] so no
 * DataStore is ever opened on a legacy path.
 *
 * Instrumented tests only run on an unlocked device, so the migrators see the
 * real unlock state through [UserManagerCompat]; the pre-unlock behaviour is
 * verified manually.
 */
@RunWith(AndroidJUnit4::class)
class StorageMigrationInstrumentedTest {

    private val context: Context = InstrumentationRegistry.getInstrumentation().targetContext
    private val deContext: Context = context.createDeviceProtectedStorageContext()

    /** Per-test legacy directory in credential-encrypted storage. */
    private lateinit var legacyDir: File

    /** Per-test target directory in device-protected storage. */
    private lateinit var targetDir: File

    /** Fake monotonic clock (nanoseconds) behind the migrators' retry rate limit. */
    private var now = 0L

    private var dataStoreScope: CoroutineScope? = null
    private var dataStore: DataStore<Preferences>? = null

    @Before
    fun setup() {
        now = 0L
        val id = UUID.randomUUID().toString()
        legacyDir = File(context.filesDir, "$TEST_ROOT/$id")
        targetDir = File(deContext.filesDir, "$TEST_ROOT/$id")
        assertTrue("could not create CE test dir $legacyDir", legacyDir.mkdirs())
        assertTrue("could not create DE test dir $targetDir", targetDir.mkdirs())
    }

    @After
    fun cleanup() {
        closeTestDataStore()
        legacyDir.deleteRecursively()
        targetDir.deleteRecursively()
        // Remove the shared parent directories when no other test left anything behind.
        File(context.filesDir, TEST_ROOT).delete()
        File(deContext.filesDir, TEST_ROOT).delete()
    }

    // ==========================================================================
    // Environment
    // ==========================================================================

    @Test
    fun userIsUnlockedWhileTheseTestsRun() {
        assertTrue(
            "the migrators only run after the user unlocked; instrumented tests need that state",
            UserManagerCompat.isUserUnlocked(context)
        )
    }

    @Test
    fun testDirectoriesLiveOnDifferentStorageAndAwayFromProductionFiles() {
        assertFalse(context.isDeviceProtectedStorage)
        assertTrue(deContext.isDeviceProtectedStorage)
        assertNotEquals(legacyDir.canonicalPath, targetDir.canonicalPath)
        assertTrue(
            "DE test dir should live under user_de, was ${targetDir.path}",
            targetDir.path.contains("user_de")
        )
        assertFalse(
            "CE test dir must not live under user_de, was ${legacyDir.canonicalPath}",
            legacyDir.canonicalPath.contains("user_de")
        )

        // The files this class touches are never the production ones.
        val productionLegacy = LaprdusStorage.legacySettingsFile(context)
        val productionCurrent = context.deviceProtectedDataStoreFile(LaprdusStorage.SETTINGS_FILE_NAME)
        assertNotEquals(productionLegacy.canonicalPath, legacySettingsFile().canonicalPath)
        assertNotEquals(productionCurrent.canonicalPath, targetSettingsFile().canonicalPath)
        assertFalse(legacySettingsFile().canonicalPath.startsWith(productionLegacy.parentFile!!.canonicalPath))
        assertFalse(targetSettingsFile().canonicalPath.startsWith(productionCurrent.parentFile!!.canonicalPath))
        for (name in LaprdusStorage.DICTIONARY_FILE_NAMES) {
            assertNotEquals(File(context.filesDir, name).canonicalPath, File(legacyDir, name).canonicalPath)
            assertNotEquals(File(deContext.filesDir, name).canonicalPath, File(targetDir, name).canonicalPath)
        }
    }

    // ==========================================================================
    // Settings: CE DataStore file -> DE DataStore
    // ==========================================================================

    @Test
    fun settingsMigrateFromCredentialEncryptedToDeviceProtectedStorage() {
        val legacy = seedLegacySettings()
        assertTrue(legacy.isFile)
        val store = openTestDataStore()
        val migrator = newSettingsMigrator(store)

        val result = runBlocking { migrator.migrateIfNeeded() }

        assertEquals(MigrationResult.Migrated(LEGACY_SETTINGS.asMap().size), result)
        assertTrue(migrator.isDone)
        assertAllSettingsMigrated(store)
        assertFalse("legacy settings file must be deleted", legacy.exists())
        assertFalse(
            "empty legacy datastore directory should be removed",
            File(legacyDir, "datastore").exists()
        )
        assertTrue("DE settings file must exist on disk", targetSettingsFile().isFile)
        assertEquals(emptyList<String>(), tempFileNamesIn(targetSettingsFile().parentFile!!))

        // Idempotent: the done flag makes later calls free.
        assertEquals(MigrationResult.NotNeeded, runBlocking { migrator.migrateIfNeeded() })
    }

    @Test
    fun settingsAlreadyInDeviceProtectedStoreWinOverLegacyValues() {
        val legacy = seedLegacySettings()
        val store = openTestDataStore()
        runBlocking {
            store.edit { prefs ->
                prefs[KEY_DEFAULT_VOICE] = "detence"
                prefs[KEY_SPEED] = 2.0f
            }
        }
        val migrator = newSettingsMigrator(store)

        val result = runBlocking { migrator.migrateIfNeeded() }

        // itemCount counts copied keys only: the two DE keys were kept.
        assertEquals(MigrationResult.Migrated(LEGACY_SETTINGS.asMap().size - 2), result)
        val merged = runBlocking { store.data.first() }
        assertEquals("detence", merged[KEY_DEFAULT_VOICE])
        assertEquals(2.0f, merged[KEY_SPEED])
        val expected = LEGACY_SETTINGS.toMutablePreferences().apply {
            this[KEY_DEFAULT_VOICE] = "detence"
            this[KEY_SPEED] = 2.0f
        }
        assertEquals(expected.asMap(), merged.asMap())
        assertFalse("legacy settings file must be deleted", legacy.exists())
    }

    @Test
    fun settingsMigrationWithoutLegacyFileIsNotNeeded() {
        val store = openTestDataStore()
        val migrator = newSettingsMigrator(store)

        assertEquals(MigrationResult.NotNeeded, runBlocking { migrator.migrateIfNeeded() })

        assertTrue(migrator.isDone)
        assertEquals(0, runBlocking { store.data.first() }.asMap().size)
    }

    @Test
    fun settingsCrashBeforeReadLegacyConverges() {
        assertSettingsCrashConverges(MigrationCrashPoint.SETTINGS_BEFORE_READ_LEGACY)
    }

    @Test
    fun settingsCrashAfterReadLegacyConverges() {
        assertSettingsCrashConverges(MigrationCrashPoint.SETTINGS_AFTER_READ_LEGACY)
    }

    @Test
    fun settingsCrashAfterCommitConverges() {
        assertSettingsCrashConverges(MigrationCrashPoint.SETTINGS_AFTER_COMMIT)
    }

    @Test
    fun settingsCrashAfterDeleteLegacyConverges() {
        assertSettingsCrashConverges(MigrationCrashPoint.SETTINGS_AFTER_DELETE_LEGACY)
    }

    // ==========================================================================
    // Dictionaries: CE files dir -> DE files dir
    // ==========================================================================

    @Test
    fun userDictionaryMigratesFromCredentialEncryptedToDeviceProtectedStorage() {
        val legacyFile = File(legacyDir, USER_DICTIONARY)
        legacyFile.writeBytes(LEGACY_DICTIONARIES.getValue(USER_DICTIONARY))
        val migrator = newDictionaryMigrator()

        val result = runBlocking { migrator.migrateIfNeeded() }

        assertEquals(MigrationResult.Migrated(1), result)
        assertTrue(migrator.isDone)
        assertArrayEquals(
            "migrated bytes must match the legacy file exactly",
            LEGACY_DICTIONARIES.getValue(USER_DICTIONARY),
            File(targetDir, USER_DICTIONARY).readBytes()
        )
        assertFalse("legacy user.json must be deleted", legacyFile.exists())
        assertEquals(emptyList<String>(), tempFileNamesIn(targetDir))
        assertEquals(listOf(USER_DICTIONARY), fileNamesIn(targetDir))
        assertEquals(MigrationResult.NotNeeded, runBlocking { migrator.migrateIfNeeded() })
    }

    @Test
    fun allDictionaryFilesMigrateAndExistingTargetCopiesWin() {
        seedLegacyDictionaries()
        val targetContent = """{"version":"1.0","entries":[{"grapheme":"DE","phoneme":"de-e"}]}"""
            .toByteArray(Charsets.UTF_8)
        File(targetDir, USER_DICTIONARY).writeBytes(targetContent)
        val migrator = newDictionaryMigrator()

        val result = runBlocking { migrator.migrateIfNeeded() }

        // user.json already existed in DE and was kept; the other two were copied.
        assertEquals(MigrationResult.Migrated(LaprdusStorage.DICTIONARY_FILE_NAMES.size - 1), result)
        assertArrayEquals(targetContent, File(targetDir, USER_DICTIONARY).readBytes())
        for (name in LaprdusStorage.DICTIONARY_FILE_NAMES - USER_DICTIONARY) {
            assertArrayEquals("target $name", LEGACY_DICTIONARIES.getValue(name), File(targetDir, name).readBytes())
        }
        assertEquals("every legacy copy is deleted", emptyList<String>(), fileNamesIn(legacyDir))
        assertEquals(LaprdusStorage.DICTIONARY_FILE_NAMES.sorted(), fileNamesIn(targetDir))
        assertEquals(emptyList<String>(), tempFileNamesIn(targetDir))
    }

    @Test
    fun dictionaryCrashBeforeCopyConverges() {
        assertDictionaryCrashConverges(MigrationCrashPoint.DICT_BEFORE_COPY)
    }

    @Test
    fun dictionaryCrashAfterTmpWriteConverges() {
        assertDictionaryCrashConverges(MigrationCrashPoint.DICT_AFTER_TMP_WRITE)
    }

    @Test
    fun dictionaryCrashAfterRenameConverges() {
        assertDictionaryCrashConverges(MigrationCrashPoint.DICT_AFTER_RENAME)
    }

    @Test
    fun dictionaryCrashAfterDeleteLegacyConverges() {
        assertDictionaryCrashConverges(MigrationCrashPoint.DICT_AFTER_DELETE_LEGACY)
    }

    // ==========================================================================
    // Repositories over the migrated data
    // ==========================================================================

    @Test
    fun dictionaryRepositoryLoadsEntriesMigratedFromLegacyStorage() {
        val legacyEntries = listOf(
            DictionaryEntry(grapheme = "Facebook", phoneme = "Fejzbuk"),
            DictionaryEntry(
                grapheme = "Đakovo",
                phoneme = "Đakovo grad",
                caseSensitive = true,
                wholeWord = false,
                comment = "with diacritics"
            )
        )
        File(legacyDir, USER_DICTIONARY).writeText(DictionaryJson.generate(legacyEntries), Charsets.UTF_8)
        val repository = DictionaryRepository(
            dictionaryDir = targetDir,
            migrator = newDictionaryMigrator(),
            logger = StorageLogger.None
        )

        val loaded = runBlocking { repository.loadDictionary(DictionaryType.MAIN) }.getOrThrow()

        assertEquals(legacyEntries.withoutIds(), loaded.withoutIds())
        assertEquals(legacyEntries.withoutIds(), runBlocking { repository.entries.first() }.withoutIds())
        assertEquals(MigrationResult.Migrated(1), repository.lastMigrationResult)
        assertNull(repository.storageError.value)
        assertFalse("legacy user.json must be deleted", File(legacyDir, USER_DICTIONARY).exists())
        assertTrue(File(targetDir, USER_DICTIONARY).isFile)

        // Later edits go to the DE file atomically and report no further migration.
        val added = DictionaryEntry(grapheme = "WhatsApp", phoneme = "Vocap")
        assertTrue(runBlocking { repository.saveEntry(added) }.isSuccess)
        assertEquals(MigrationResult.NotNeeded, repository.lastMigrationResult)
        val onDisk = DictionaryJson.parse(File(targetDir, USER_DICTIONARY).readText(Charsets.UTF_8))
        assertEquals((legacyEntries + added).withoutIds(), onDisk.withoutIds())
        assertEquals(emptyList<String>(), tempFileNamesIn(targetDir))
        assertEquals(emptyList<String>(), fileNamesIn(legacyDir))
    }

    @Test
    fun settingsRepositoryDeliversLegacyValuesThroughTheMigrationGate() {
        val legacy = seedLegacySettings()
        val store = openTestDataStore()
        val repository = SettingsRepository(
            dataStore = store,
            migrator = newSettingsMigrator(store),
            logger = StorageLogger.None
        )

        // The bounded startup read bypasses the gate and sees the empty DE store.
        val beforeMigration = runBlocking { repository.readSettingsNow() }
        assertEquals(SettingsRepository.TTSSettings(), beforeMigration)

        // Every regular read runs the migration first.
        val settings = runBlocking { repository.allSettings.first() }
        // lastMigrationResult reflects the most recent attempt, so check it before any further read
        assertEquals(MigrationResult.Migrated(LEGACY_SETTINGS.asMap().size), repository.lastMigrationResult)

        assertEquals(LEGACY_SETTINGS[KEY_DEFAULT_VOICE], settings.defaultVoice)
        assertEquals(LEGACY_SETTINGS[KEY_SPEED], settings.speed)
        assertEquals(LEGACY_SETTINGS[KEY_PITCH], settings.pitch)
        assertEquals(LEGACY_SETTINGS[KEY_VOLUME], settings.volume)
        assertEquals(LEGACY_SETTINGS[KEY_FORCE_SPEED], settings.forceSpeed)
        assertEquals(LEGACY_SETTINGS[KEY_FORCE_PITCH], settings.forcePitch)
        assertEquals(LEGACY_SETTINGS[KEY_FORCE_VOLUME], settings.forceVolume)
        assertEquals(LEGACY_SETTINGS[KEY_FORCE_LANGUAGE], settings.forceLanguage)
        assertEquals(LEGACY_SETTINGS[KEY_EMOJI_ENABLED], settings.emojiEnabled)
        assertEquals(LEGACY_SETTINGS[KEY_INFLECTION_ENABLED], settings.inflectionEnabled)
        assertEquals(LEGACY_SETTINGS[KEY_SENTENCE_PAUSE], settings.sentencePause)
        assertEquals(LEGACY_SETTINGS[KEY_COMMA_PAUSE], settings.commaPause)
        assertEquals(LEGACY_SETTINGS[KEY_NEWLINE_PAUSE], settings.newlinePause)
        assertEquals(LEGACY_SETTINGS[KEY_NUMBER_MODE], settings.numberMode)
        assertEquals(LEGACY_SETTINGS[KEY_USER_DICTIONARIES_ENABLED], settings.userDictionariesEnabled)
        assertEquals(LEGACY_SETTINGS[KEY_DONT_ASK_DEFAULT_TTS], runBlocking { repository.dontAskDefaultTts.first() })
        assertNull(repository.storageError.value)
        assertFalse("legacy settings file must be deleted", legacy.exists())
        assertEquals(settings, runBlocking { repository.readSettingsNow() })
    }

    // ==========================================================================
    // Helpers: paths and seeding
    // ==========================================================================

    /** Legacy layout mirrors production: `<CE files>/datastore/<settings file>`, but under the test dir. */
    private fun legacySettingsFile(): File = File(legacyDir, LaprdusStorage.LEGACY_SETTINGS_RELATIVE_PATH)

    /** The test DataStore file lives inside the DE test dir, never at the production path. */
    private fun targetSettingsFile(): File = File(targetDir, "datastore/test_settings.preferences_pb")

    /** Writes [prefs] as a legacy DataStore file without opening a DataStore on it. */
    private fun seedLegacySettings(prefs: Preferences = LEGACY_SETTINGS): File {
        val file = legacySettingsFile()
        assertTrue(file.parentFile!!.isDirectory || file.parentFile!!.mkdirs())
        runBlocking {
            file.outputStream().use { PreferencesFileSerializer.writeTo(prefs, it) }
        }
        return file
    }

    private fun readPreferencesFile(file: File): Preferences = runBlocking {
        file.inputStream().use { PreferencesFileSerializer.readFrom(it) }
    }

    private fun seedLegacyDictionaries(names: Collection<String> = LaprdusStorage.DICTIONARY_FILE_NAMES) {
        for (name in names) {
            File(legacyDir, name).writeBytes(LEGACY_DICTIONARIES.getValue(name))
        }
    }

    private fun fileNamesIn(dir: File): List<String> = dir.listFiles().orEmpty().map { it.name }.sorted()

    private fun tempFileNamesIn(dir: File): List<String> =
        fileNamesIn(dir).filter { it.endsWith(AtomicFiles.TEMP_SUFFIX) }

    /** Ids are regenerated on load, so comparisons ignore them. */
    private fun List<DictionaryEntry>.withoutIds(): List<DictionaryEntry> = map { it.copy(id = "") }

    // ==========================================================================
    // Helpers: DataStore lifecycle and migrators
    // ==========================================================================

    /**
     * Opens the test settings DataStore over [targetSettingsFile]. Exactly one
     * instance may be open per file, so the previous one must be closed first.
     */
    private fun openTestDataStore(): DataStore<Preferences> {
        assertNull("close the previous test DataStore before opening a new one", dataStore)
        val dir = targetSettingsFile().parentFile!!
        assertTrue(dir.isDirectory || dir.mkdirs())
        val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
        val store = PreferenceDataStoreFactory.create(
            scope = scope,
            produceFile = { targetSettingsFile() }
        )
        dataStoreScope = scope
        dataStore = store
        return store
    }

    /**
     * Cancels the DataStore scope and waits for it, which releases the file so
     * a fresh instance can be opened on it (simulated process restart).
     */
    private fun closeTestDataStore() {
        val scope = dataStoreScope ?: return
        dataStoreScope = null
        dataStore = null
        runBlocking { scope.coroutineContext[Job]!!.cancelAndJoin() }
    }

    private fun newSettingsMigrator(
        target: DataStore<Preferences>,
        faultInjector: MigrationFaultInjector = MigrationFaultInjector.NONE,
    ): SettingsMigrator = SettingsMigrator(
        legacyFile = { legacySettingsFile() },
        target = target,
        isUserUnlocked = { UserManagerCompat.isUserUnlocked(context) },
        faultInjector = faultInjector,
        logger = StorageLogger.None,
        clock = { now }
    )

    private fun newDictionaryMigrator(
        faultInjector: MigrationFaultInjector = MigrationFaultInjector.NONE,
    ): DictionaryMigrator = DictionaryMigrator(
        legacyDir = { legacyDir },
        targetDir = targetDir,
        fileNames = LaprdusStorage.DICTIONARY_FILE_NAMES,
        isUserUnlocked = { UserManagerCompat.isUserUnlocked(context) },
        faultInjector = faultInjector,
        logger = StorageLogger.None,
        clock = { now }
    )

    // ==========================================================================
    // Helpers: assertions
    // ==========================================================================

    private fun assertAllSettingsMigrated(store: DataStore<Preferences>) {
        val migrated = runBlocking { store.data.first() }
        assertEquals("all legacy keys must be present", LEGACY_SETTINGS.asMap().size, migrated.asMap().size)
        assertEquals(LEGACY_SETTINGS.asMap(), migrated.asMap())
        // Persisted, not only cached: the DE file itself carries the values.
        assertEquals(LEGACY_SETTINGS.asMap(), readPreferencesFile(targetSettingsFile()).asMap())
    }

    /** Runs [block] expecting a [SimulatedMigrationCrashException] at [point]. */
    private fun assertCrashesAt(point: MigrationCrashPoint, block: suspend () -> Unit) {
        val thrown = try {
            runBlocking { block() }
            null
        } catch (e: Throwable) {
            e
        }
        assertNotNull("expected a simulated crash at $point", thrown)
        assertTrue("expected SimulatedMigrationCrashException but got $thrown", thrown is SimulatedMigrationCrashException)
        assertEquals(point, (thrown as SimulatedMigrationCrashException).point)
    }

    /**
     * Seeds the legacy settings, crashes once at [point], verifies the on-disk
     * state a real crash would leave, then simulates a process restart (fresh
     * DataStore over the same DE file, fresh migrator) and checks convergence.
     */
    private fun assertSettingsCrashConverges(point: MigrationCrashPoint) {
        val legacy = seedLegacySettings()
        val firstStore = openTestDataStore()
        val injector = OneShotCrashInjector(point)
        val crashed = newSettingsMigrator(firstStore, injector)

        assertCrashesAt(point) { crashed.migrateIfNeeded() }
        assertTrue(injector.fired)
        assertFalse("a crash must not mark the migration done", crashed.isDone)

        // State a real crash would leave behind.
        when (point) {
            MigrationCrashPoint.SETTINGS_BEFORE_READ_LEGACY,
            MigrationCrashPoint.SETTINGS_AFTER_READ_LEGACY -> {
                assertTrue("legacy must be intact before the commit", legacy.isFile)
                assertEquals(LEGACY_SETTINGS.asMap(), readPreferencesFile(legacy).asMap())
                assertEquals(0, runBlocking { firstStore.data.first() }.asMap().size)
            }
            MigrationCrashPoint.SETTINGS_AFTER_COMMIT -> {
                assertTrue("legacy still present after the commit", legacy.isFile)
                assertEquals(LEGACY_SETTINGS.asMap(), runBlocking { firstStore.data.first() }.asMap())
            }
            MigrationCrashPoint.SETTINGS_AFTER_DELETE_LEGACY -> {
                assertFalse("legacy already deleted", legacy.exists())
                assertEquals(LEGACY_SETTINGS.asMap(), runBlocking { firstStore.data.first() }.asMap())
            }
            else -> throw AssertionError("not a settings crash point: $point")
        }

        // Simulated process restart: the old DataStore is gone, a new one reads the same file.
        closeTestDataStore()
        val secondStore = openTestDataStore()
        val fresh = newSettingsMigrator(secondStore)

        val result = runBlocking { fresh.migrateIfNeeded() }

        val expected = when (point) {
            MigrationCrashPoint.SETTINGS_BEFORE_READ_LEGACY,
            MigrationCrashPoint.SETTINGS_AFTER_READ_LEGACY -> MigrationResult.Migrated(LEGACY_SETTINGS.asMap().size)
            // Every key already in DE wins, so nothing is copied; the legacy file is cleaned up.
            MigrationCrashPoint.SETTINGS_AFTER_COMMIT -> MigrationResult.Migrated(0)
            else -> MigrationResult.NotNeeded
        }
        assertEquals(expected, result)
        assertTrue(fresh.isDone)
        assertAllSettingsMigrated(secondStore)
        assertFalse("legacy settings file must be gone after recovery", legacy.exists())
        assertEquals(emptyList<String>(), tempFileNamesIn(targetSettingsFile().parentFile!!))
        assertEquals(MigrationResult.NotNeeded, runBlocking { fresh.migrateIfNeeded() })
    }

    /**
     * Seeds all dictionary files, crashes once at [point] (which hits the first
     * file, user.json), verifies the on-disk state, then checks that a fresh
     * migrator (simulated process restart) converges on the migrated state.
     */
    private fun assertDictionaryCrashConverges(point: MigrationCrashPoint) {
        seedLegacyDictionaries()
        val injector = OneShotCrashInjector(point)
        val crashed = newDictionaryMigrator(injector)

        assertCrashesAt(point) { crashed.migrateIfNeeded() }
        assertTrue(injector.fired)
        assertFalse("a crash must not mark the migration done", crashed.isDone)

        val legacyUser = File(legacyDir, USER_DICTIONARY)
        val targetUser = File(targetDir, USER_DICTIONARY)
        when (point) {
            MigrationCrashPoint.DICT_BEFORE_COPY -> {
                assertTrue(legacyUser.isFile)
                assertFalse(targetUser.exists())
                assertEquals(emptyList<String>(), fileNamesIn(targetDir))
            }
            MigrationCrashPoint.DICT_AFTER_TMP_WRITE -> {
                assertTrue(legacyUser.isFile)
                assertFalse("rename must not have happened", targetUser.exists())
                val temps = tempFileNamesIn(targetDir)
                assertEquals("exactly one temp file is left behind", 1, temps.size)
                assertTrue(AtomicFiles.isTempFileFor(File(targetDir, temps.single()), USER_DICTIONARY))
            }
            MigrationCrashPoint.DICT_AFTER_RENAME -> {
                assertTrue("legacy still present after the rename", legacyUser.isFile)
                assertArrayEquals(LEGACY_DICTIONARIES.getValue(USER_DICTIONARY), targetUser.readBytes())
                assertEquals(emptyList<String>(), tempFileNamesIn(targetDir))
            }
            MigrationCrashPoint.DICT_AFTER_DELETE_LEGACY -> {
                assertFalse("legacy already deleted", legacyUser.exists())
                assertArrayEquals(LEGACY_DICTIONARIES.getValue(USER_DICTIONARY), targetUser.readBytes())
            }
            else -> throw AssertionError("not a dictionary crash point: $point")
        }
        // The crash hit the first file; the others were never reached.
        for (name in LaprdusStorage.DICTIONARY_FILE_NAMES - USER_DICTIONARY) {
            assertTrue("$name must still be in legacy", File(legacyDir, name).isFile)
            assertFalse("$name must not be in target yet", File(targetDir, name).exists())
        }

        // Simulated process restart: files already renamed into place are not copied again.
        val alreadyPresent = LaprdusStorage.DICTIONARY_FILE_NAMES.count { File(targetDir, it).isFile }
        val fresh = newDictionaryMigrator()

        val result = runBlocking { fresh.migrateIfNeeded() }

        assertEquals(MigrationResult.Migrated(LaprdusStorage.DICTIONARY_FILE_NAMES.size - alreadyPresent), result)
        assertTrue(fresh.isDone)
        for (name in LaprdusStorage.DICTIONARY_FILE_NAMES) {
            assertArrayEquals("target $name", LEGACY_DICTIONARIES.getValue(name), File(targetDir, name).readBytes())
            assertFalse("legacy $name must be deleted", File(legacyDir, name).exists())
        }
        assertEquals("stale temp files must be cleaned up", emptyList<String>(), tempFileNamesIn(targetDir))
        assertEquals(LaprdusStorage.DICTIONARY_FILE_NAMES.sorted(), fileNamesIn(targetDir))
        assertEquals(emptyList<String>(), fileNamesIn(legacyDir))
        assertEquals(MigrationResult.NotNeeded, runBlocking { fresh.migrateIfNeeded() })
    }

    private companion object {
        const val TEST_ROOT = "migration_test"
        const val USER_DICTIONARY = "user.json"

        // Same names and types as the private keys in SettingsRepository.
        val KEY_DEFAULT_VOICE = stringPreferencesKey("default_voice")
        val KEY_SPEED = floatPreferencesKey("speed")
        val KEY_PITCH = floatPreferencesKey("pitch")
        val KEY_VOLUME = floatPreferencesKey("volume")
        val KEY_FORCE_SPEED = booleanPreferencesKey("force_speed")
        val KEY_FORCE_PITCH = booleanPreferencesKey("force_pitch")
        val KEY_FORCE_VOLUME = booleanPreferencesKey("force_volume")
        val KEY_FORCE_LANGUAGE = booleanPreferencesKey("force_language")
        val KEY_EMOJI_ENABLED = booleanPreferencesKey("emoji_enabled")
        val KEY_INFLECTION_ENABLED = booleanPreferencesKey("inflection_enabled")
        val KEY_DONT_ASK_DEFAULT_TTS = booleanPreferencesKey("dont_ask_default_tts")
        val KEY_USER_DICTIONARIES_ENABLED = booleanPreferencesKey("user_dictionaries_enabled")
        val KEY_SENTENCE_PAUSE = intPreferencesKey("sentence_pause")
        val KEY_COMMA_PAUSE = intPreferencesKey("comma_pause")
        val KEY_NEWLINE_PAUSE = intPreferencesKey("newline_pause")
        val KEY_NUMBER_MODE = intPreferencesKey("number_mode")

        /** All 16 keys, every value different from its SettingsRepository default. */
        val LEGACY_SETTINGS: Preferences = mutablePreferencesOf(
            KEY_DEFAULT_VOICE to "vlado",
            KEY_SPEED to 1.5f,
            KEY_PITCH to 0.8f,
            KEY_VOLUME to 0.6f,
            KEY_FORCE_SPEED to true,
            KEY_FORCE_PITCH to true,
            KEY_FORCE_VOLUME to true,
            KEY_FORCE_LANGUAGE to true,
            KEY_EMOJI_ENABLED to true,
            KEY_INFLECTION_ENABLED to false,
            KEY_DONT_ASK_DEFAULT_TTS to true,
            KEY_USER_DICTIONARIES_ENABLED to false,
            KEY_SENTENCE_PAUSE to 250,
            KEY_COMMA_PAUSE to 150,
            KEY_NEWLINE_PAUSE to 300,
            KEY_NUMBER_MODE to 1
        )

        /** Distinct, non-ASCII content per dictionary so byte-exact copies are verifiable. */
        val LEGACY_DICTIONARIES: Map<String, ByteArray> = mapOf(
            "user.json" to """{"version":"1.0","entries":[{"grapheme":"Čačak","phoneme":"Ča-čak","caseSensitive":false,"wholeWord":true}]}""",
            "spelling.json" to """{"version":"1.0","entries":[{"character":"Đ","pronunciation":"Đe"}]}""",
            "emoji.json" to """{"version":"1.0","entries":[{"emoji":"😀","text":"nasmijano lice"}]}"""
        ).mapValues { it.value.toByteArray(Charsets.UTF_8) }
    }
}
