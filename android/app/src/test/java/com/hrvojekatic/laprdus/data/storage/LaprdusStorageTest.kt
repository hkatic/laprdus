package com.hrvojekatic.laprdus.data.storage

import android.content.Context
import androidx.datastore.preferences.core.PreferencesFileSerializer
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.floatPreferencesKey
import androidx.datastore.preferences.core.mutablePreferencesOf
import androidx.datastore.preferences.core.stringPreferencesKey
import com.hrvojekatic.laprdus.data.DictionaryEntry
import com.hrvojekatic.laprdus.data.DictionaryJson
import com.hrvojekatic.laprdus.data.DictionaryRepository
import com.hrvojekatic.laprdus.data.DictionaryType
import com.hrvojekatic.laprdus.data.SettingsRepository
import com.hrvojekatic.laprdus.data.migration.MigrationResult
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotEquals
import org.junit.Assert.assertNotSame
import org.junit.Assert.assertNull
import org.junit.Assert.assertSame
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.RuntimeEnvironment
import org.robolectric.annotation.Config
import java.io.File

/**
 * Robolectric tests for [LaprdusStorage]: singleton identity, the split
 * between device-protected and credential-encrypted paths, and the production
 * wiring of the migrators and repositories over those paths.
 *
 * Robolectric gives the device-protected context its own data directory, so
 * these tests exercise the real `createDeviceProtectedStorageContext()` code
 * path; its UserManager reports the user as unlocked.
 */
@RunWith(RobolectricTestRunner::class)
@Config(sdk = [36], manifest = Config.NONE)
class LaprdusStorageTest {

    companion object {
        // Same names and types as the private keys in SettingsRepository.
        private val KEY_DEFAULT_VOICE = stringPreferencesKey("default_voice")
        private val KEY_SPEED = floatPreferencesKey("speed")
    }

    private lateinit var context: Context

    @Before
    fun setup() {
        context = RuntimeEnvironment.getApplication()
        LaprdusStorage.resetForTesting()
        SettingsRepository.resetInstanceForTesting()
    }

    @After
    fun tearDown() {
        LaprdusStorage.resetForTesting()
        SettingsRepository.resetInstanceForTesting()
    }

    private fun deviceProtectedSettingsFile(): File = File(
        LaprdusStorage.deviceProtectedContext(context).filesDir,
        LaprdusStorage.LEGACY_SETTINGS_RELATIVE_PATH
    )

    // ==========================================================================
    // Device-Protected vs Credential-Encrypted Paths
    // ==========================================================================

    @Test
    fun `deviceProtectedContext points at device-protected storage`() {
        val deviceContext = LaprdusStorage.deviceProtectedContext(context)

        assertTrue(deviceContext.isDeviceProtectedStorage)
        assertFalse(context.isDeviceProtectedStorage)
        assertSame(deviceContext, LaprdusStorage.deviceProtectedContext(context))
    }

    @Test
    fun `dictionaryDir is the device-protected files directory`() {
        val dictionaryDir = LaprdusStorage.dictionaryDir(context)

        assertEquals(
            LaprdusStorage.deviceProtectedContext(context).filesDir.canonicalFile,
            dictionaryDir.canonicalFile
        )
        assertTrue(dictionaryDir.isDirectory)
    }

    @Test
    fun `dictionaryDir differs from legacyDictionaryDir`() {
        val dictionaryDir = LaprdusStorage.dictionaryDir(context)
        val legacyDir = LaprdusStorage.legacyDictionaryDir(context)

        assertNotEquals(legacyDir.canonicalFile, dictionaryDir.canonicalFile)
        assertEquals(context.filesDir.canonicalFile, legacyDir.canonicalFile)
        assertFalse(legacyDir.canonicalFile.startsWith(dictionaryDir.canonicalFile))
        assertFalse(dictionaryDir.canonicalFile.startsWith(legacyDir.canonicalFile))
    }

    @Test
    fun `legacySettingsFile is the datastore file under the credential-encrypted filesDir`() {
        val legacyFile = LaprdusStorage.legacySettingsFile(context)

        assertEquals(
            File(File(context.filesDir, "datastore"), "laprdus_settings.preferences_pb"),
            legacyFile
        )
        assertTrue(
            legacyFile.path.replace(File.separatorChar, '/')
                .endsWith("datastore/laprdus_settings.preferences_pb")
        )
        assertEquals(
            context.filesDir.canonicalFile,
            legacyFile.parentFile!!.parentFile!!.canonicalFile
        )
        assertNotEquals(
            LaprdusStorage.dictionaryDir(context).canonicalFile,
            legacyFile.parentFile!!.parentFile!!.canonicalFile
        )
    }

    @Test
    fun `engineCrashMarkerFile lives in the device-protected dictionary directory`() {
        assertEquals(
            File(LaprdusStorage.dictionaryDir(context), LaprdusStorage.ENGINE_CRASH_MARKER_FILE_NAME),
            LaprdusStorage.engineCrashMarkerFile(context)
        )
        assertEquals("engine_crash_marker", LaprdusStorage.engineCrashMarkerFile(context).name)
    }

    @Test
    fun `isUserUnlocked is true under Robolectric`() {
        assertTrue(LaprdusStorage.isUserUnlocked(context))
    }

    // ==========================================================================
    // Singleton Identity
    // ==========================================================================

    @Test
    fun `settingsDataStore returns the same instance on repeated calls`() {
        val first = LaprdusStorage.settingsDataStore(context)
        val second = LaprdusStorage.settingsDataStore(context)

        assertSame(first, second)
        assertSame(first, LaprdusStorage.settingsDataStore(context.applicationContext))
    }

    @Test
    fun `settingsMigrator returns the same instance on repeated calls`() {
        assertSame(LaprdusStorage.settingsMigrator(context), LaprdusStorage.settingsMigrator(context))
    }

    @Test
    fun `dictionaryMigrator returns the same instance on repeated calls`() {
        assertSame(LaprdusStorage.dictionaryMigrator(context), LaprdusStorage.dictionaryMigrator(context))
    }

    @Test
    fun `SettingsRepository getInstance returns the same instance twice`() {
        val first = SettingsRepository.getInstance(context)
        val second = SettingsRepository.getInstance(context)

        assertSame(first, second)
    }

    @Test
    fun `resetForTesting makes settingsDataStore return a new instance`() {
        val store = LaprdusStorage.settingsDataStore(context)
        val settingsMigrator = LaprdusStorage.settingsMigrator(context)
        val dictionaryMigrator = LaprdusStorage.dictionaryMigrator(context)
        val deviceContext = LaprdusStorage.deviceProtectedContext(context)

        LaprdusStorage.resetForTesting()

        assertNotSame(store, LaprdusStorage.settingsDataStore(context))
        assertNotSame(settingsMigrator, LaprdusStorage.settingsMigrator(context))
        assertNotSame(dictionaryMigrator, LaprdusStorage.dictionaryMigrator(context))
        assertNotSame(deviceContext, LaprdusStorage.deviceProtectedContext(context))
    }

    @Test
    fun `resetInstanceForTesting makes SettingsRepository getInstance return a new instance`() {
        val first = SettingsRepository.getInstance(context)

        SettingsRepository.resetInstanceForTesting()

        assertNotSame(first, SettingsRepository.getInstance(context))
    }

    // ==========================================================================
    // Production Wiring
    // ==========================================================================

    @Test
    fun `migrators report NotNeeded on a fresh install`() = runBlocking {
        assertEquals(MigrationResult.NotNeeded, LaprdusStorage.settingsMigrator(context).migrateIfNeeded())
        assertEquals(MigrationResult.NotNeeded, LaprdusStorage.dictionaryMigrator(context).migrateIfNeeded())
        assertTrue(LaprdusStorage.settingsMigrator(context).isDone)
        assertTrue(LaprdusStorage.dictionaryMigrator(context).isDone)
    }

    @Test
    fun `settingsDataStore persists into the device-protected datastore directory`() {
        runBlocking {
            LaprdusStorage.settingsDataStore(context).edit { it[KEY_SPEED] = 1.25f }
        }

        val deviceFile = deviceProtectedSettingsFile()
        assertTrue("expected ${deviceFile.path} to exist", deviceFile.isFile)
        assertFalse(LaprdusStorage.legacySettingsFile(context).exists())
        val readBack = runBlocking { LaprdusStorage.settingsDataStore(context).data.first() }
        assertEquals(1.25f, readBack[KEY_SPEED]!!, 0f)
    }

    @Test
    fun `DictionaryRepository create writes user json under dictionaryDir`() {
        val repository = DictionaryRepository.create(context)

        runBlocking {
            assertTrue(repository.loadDictionary(DictionaryType.MAIN).isSuccess)
            assertTrue(
                repository.saveEntry(DictionaryEntry(grapheme = "Facebook", phoneme = "Fejzbuk")).isSuccess
            )
        }

        val userFile = File(LaprdusStorage.dictionaryDir(context), DictionaryType.MAIN.fileName)
        assertTrue("expected ${userFile.path} to exist", userFile.isFile)
        assertEquals(
            listOf("Facebook"),
            DictionaryJson.parse(userFile.readText(Charsets.UTF_8)).map { it.grapheme }
        )
        assertFalse(File(LaprdusStorage.legacyDictionaryDir(context), DictionaryType.MAIN.fileName).exists())
        assertNull(repository.storageError.value)
        assertEquals(MigrationResult.NotNeeded, repository.lastMigrationResult)
    }

    @Test
    fun `dictionaryMigrator moves a legacy dictionary into device-protected storage`() {
        val legacyFile = File(LaprdusStorage.legacyDictionaryDir(context), "user.json")
        legacyFile.writeText(
            DictionaryJson.generate(listOf(DictionaryEntry(grapheme = "stari", phoneme = "novi"))),
            Charsets.UTF_8
        )

        val result = runBlocking { LaprdusStorage.dictionaryMigrator(context).migrateIfNeeded() }

        assertEquals(MigrationResult.Migrated(1), result)
        assertFalse(legacyFile.exists())
        val migrated = File(LaprdusStorage.dictionaryDir(context), "user.json")
        assertTrue(migrated.isFile)
        assertEquals(listOf("stari"), DictionaryJson.parse(migrated.readText(Charsets.UTF_8)).map { it.grapheme })

        // The repository built by create() sees the migrated entries.
        val loaded = runBlocking { DictionaryRepository.create(context).loadDictionary(DictionaryType.MAIN) }
        assertEquals(listOf("stari"), loaded.getOrThrow().map { it.grapheme })
    }

    @Test
    fun `settingsMigrator moves legacy settings into the device-protected DataStore`() {
        val legacyFile = LaprdusStorage.legacySettingsFile(context)
        legacyFile.parentFile!!.mkdirs()
        runBlocking {
            val legacyPrefs = mutablePreferencesOf()
            legacyPrefs[KEY_DEFAULT_VOICE] = "vlado"
            legacyPrefs[KEY_SPEED] = 1.5f
            legacyFile.outputStream().use { PreferencesFileSerializer.writeTo(legacyPrefs, it) }
        }
        assertTrue(legacyFile.isFile)

        val repository = SettingsRepository.getInstance(context)
        val settings = runBlocking { repository.allSettings.first() }

        assertEquals("vlado", settings.defaultVoice)
        assertEquals(1.5f, settings.speed, 0f)
        assertEquals(MigrationResult.Migrated(2), repository.lastMigrationResult)
        assertNull(repository.storageError.value)
        assertFalse(legacyFile.exists())
        assertTrue(deviceProtectedSettingsFile().isFile)
        assertEquals("vlado", runBlocking { repository.defaultVoice.first() })
    }
}
