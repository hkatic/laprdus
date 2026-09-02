package com.hrvojekatic.laprdus.data

import androidx.datastore.core.CorruptionException
import androidx.datastore.core.DataStore
import androidx.datastore.core.handlers.ReplaceFileCorruptionHandler
import androidx.datastore.preferences.core.PreferenceDataStoreFactory
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.PreferencesFileSerializer
import androidx.datastore.preferences.core.booleanPreferencesKey
import androidx.datastore.preferences.core.emptyPreferences
import androidx.datastore.preferences.core.floatPreferencesKey
import androidx.datastore.preferences.core.intPreferencesKey
import androidx.datastore.preferences.core.mutablePreferencesOf
import androidx.datastore.preferences.core.stringPreferencesKey
import com.hrvojekatic.laprdus.data.migration.DirectBootMigrationException
import com.hrvojekatic.laprdus.data.migration.LegacyMigrator
import com.hrvojekatic.laprdus.data.migration.MigrationCrashPoint
import com.hrvojekatic.laprdus.data.migration.MigrationResult
import com.hrvojekatic.laprdus.data.migration.OneShotCrashInjector
import com.hrvojekatic.laprdus.data.migration.SettingsMigrator
import com.hrvojekatic.laprdus.data.migration.SimulatedMigrationCrashException
import com.hrvojekatic.laprdus.data.storage.StorageLogger
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.Job
import kotlinx.coroutines.cancel
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.test.TestScope
import kotlinx.coroutines.test.UnconfinedTestDispatcher
import kotlinx.coroutines.test.runTest
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File
import java.io.IOException

/**
 * Unit tests for SettingsRepository.
 * Tests cover all settings including new force toggles and restore default methods,
 * the advanced settings, and the legacy-storage migration gate (values migrated on
 * first read, re-arming after unlock, contained migration failures, corruption handling).
 * Uses a fresh DataStore for each test to ensure isolation.
 */
@OptIn(ExperimentalCoroutinesApi::class)
class SettingsRepositoryTest {

    @get:Rule
    val tempFolder = TemporaryFolder()

    private val testDispatcher = UnconfinedTestDispatcher()
    private val testScope = TestScope(testDispatcher + Job())

    private lateinit var testDataStore: DataStore<Preferences>
    private lateinit var repository: SettingsRepository

    @Before
    fun setup() {
        // Create a fresh DataStore for each test using temp folder
        testDataStore = PreferenceDataStoreFactory.create(
            scope = testScope,
            produceFile = { tempFolder.newFile("test_settings.preferences_pb") }
        )
        repository = SettingsRepository(testDataStore)
    }

    @After
    fun cleanup() {
        // Cancel the test scope to clean up the DataStore
        testScope.cancel()
    }

    // ==========================================================================
    // Voice Settings Tests
    // ==========================================================================

    @Test
    fun `defaultVoice returns josip when not set`() = runTest {
        val defaultVoice = repository.defaultVoice.first()
        assertEquals(SettingsRepository.DEFAULT_VOICE, defaultVoice)
    }

    @Test
    fun `setDefaultVoice persists voice id`() = runTest {
        repository.setDefaultVoice("vlado")
        val voice = repository.defaultVoice.first()
        assertEquals("vlado", voice)
    }

    // ==========================================================================
    // Speed Settings Tests
    // ==========================================================================

    @Test
    fun `speed returns default 1_0 when not set`() = runTest {
        val speed = repository.speed.first()
        assertEquals(SettingsRepository.DEFAULT_SPEED, speed)
    }

    @Test
    fun `setSpeed clamps value below 0_5 to 0_5`() = runTest {
        repository.setSpeed(0.1f)
        val speed = repository.speed.first()
        assertEquals(0.5f, speed)
    }

    @Test
    fun `setSpeed clamps value above 2_0 to 2_0`() = runTest {
        repository.setSpeed(5.0f)
        val speed = repository.speed.first()
        assertEquals(2.0f, speed)
    }

    @Test
    fun `setSpeed persists valid values`() = runTest {
        repository.setSpeed(1.5f)
        val speed = repository.speed.first()
        assertEquals(1.5f, speed)
    }

    // ==========================================================================
    // Pitch Settings Tests
    // ==========================================================================

    @Test
    fun `pitch returns default 1_0 when not set`() = runTest {
        val pitch = repository.pitch.first()
        assertEquals(SettingsRepository.DEFAULT_PITCH, pitch)
    }

    @Test
    fun `setPitch clamps value below 0_5 to 0_5`() = runTest {
        repository.setPitch(0.1f)
        val pitch = repository.pitch.first()
        assertEquals(0.5f, pitch)
    }

    @Test
    fun `setPitch clamps value above 2_0 to 2_0`() = runTest {
        repository.setPitch(5.0f)
        val pitch = repository.pitch.first()
        assertEquals(2.0f, pitch)
    }

    @Test
    fun `setPitch persists valid values`() = runTest {
        repository.setPitch(1.3f)
        val pitch = repository.pitch.first()
        assertEquals(1.3f, pitch)
    }

    // ==========================================================================
    // Volume Settings Tests
    // ==========================================================================

    @Test
    fun `volume returns default 1_0 when not set`() = runTest {
        val volume = repository.volume.first()
        assertEquals(SettingsRepository.DEFAULT_VOLUME, volume)
    }

    @Test
    fun `setVolume clamps value below 0_0 to 0_0`() = runTest {
        repository.setVolume(-1.0f)
        val volume = repository.volume.first()
        assertEquals(0.0f, volume)
    }

    @Test
    fun `setVolume clamps value above 1_0 to 1_0`() = runTest {
        repository.setVolume(2.0f)
        val volume = repository.volume.first()
        assertEquals(1.0f, volume)
    }

    @Test
    fun `setVolume persists valid values`() = runTest {
        repository.setVolume(0.7f)
        val volume = repository.volume.first()
        assertEquals(0.7f, volume)
    }

    // ==========================================================================
    // Force Speed Settings Tests
    // ==========================================================================

    @Test
    fun `forceSpeed returns false when not set`() = runTest {
        val forceSpeed = repository.forceSpeed.first()
        assertFalse(forceSpeed)
    }

    @Test
    fun `setForceSpeed persists true value`() = runTest {
        repository.setForceSpeed(true)
        val forceSpeed = repository.forceSpeed.first()
        assertTrue(forceSpeed)
    }

    @Test
    fun `setForceSpeed persists false value`() = runTest {
        repository.setForceSpeed(true)
        repository.setForceSpeed(false)
        val forceSpeed = repository.forceSpeed.first()
        assertFalse(forceSpeed)
    }

    // ==========================================================================
    // Force Pitch Settings Tests
    // ==========================================================================

    @Test
    fun `forcePitch returns false when not set`() = runTest {
        val forcePitch = repository.forcePitch.first()
        assertFalse(forcePitch)
    }

    @Test
    fun `setForcePitch persists value`() = runTest {
        repository.setForcePitch(true)
        val forcePitch = repository.forcePitch.first()
        assertTrue(forcePitch)
    }

    // ==========================================================================
    // Force Volume Settings Tests
    // ==========================================================================

    @Test
    fun `forceVolume returns false when not set`() = runTest {
        val forceVolume = repository.forceVolume.first()
        assertFalse(forceVolume)
    }

    @Test
    fun `setForceVolume persists value`() = runTest {
        repository.setForceVolume(true)
        val forceVolume = repository.forceVolume.first()
        assertTrue(forceVolume)
    }

    // ==========================================================================
    // Force Language Settings Tests
    // ==========================================================================

    @Test
    fun `forceLanguage returns false when not set`() = runTest {
        val forceLanguage = repository.forceLanguage.first()
        assertFalse(forceLanguage)
    }

    @Test
    fun `setForceLanguage persists value`() = runTest {
        repository.setForceLanguage(true)
        val forceLanguage = repository.forceLanguage.first()
        assertTrue(forceLanguage)
    }

    // ==========================================================================
    // Restore Default Methods Tests
    // ==========================================================================

    @Test
    fun `restoreDefaultSpeed restores speed to 1_0`() = runTest {
        repository.setSpeed(1.8f)
        repository.restoreDefaultSpeed()
        val speed = repository.speed.first()
        assertEquals(SettingsRepository.DEFAULT_SPEED, speed)
    }

    @Test
    fun `restoreDefaultPitch restores pitch to 1_0`() = runTest {
        repository.setPitch(0.6f)
        repository.restoreDefaultPitch()
        val pitch = repository.pitch.first()
        assertEquals(SettingsRepository.DEFAULT_PITCH, pitch)
    }

    @Test
    fun `restoreDefaultVolume restores volume to 1_0`() = runTest {
        repository.setVolume(0.3f)
        repository.restoreDefaultVolume()
        val volume = repository.volume.first()
        assertEquals(SettingsRepository.DEFAULT_VOLUME, volume)
    }

    // ==========================================================================
    // Combined Settings Tests
    // ==========================================================================

    @Test
    fun `allSettings returns correct combined values`() = runTest {
        repository.setDefaultVoice("detence")
        repository.setSpeed(1.5f)
        repository.setPitch(0.8f)
        repository.setVolume(0.9f)
        repository.setForceSpeed(true)
        repository.setForcePitch(false)
        repository.setForceVolume(true)
        repository.setForceLanguage(true)

        val settings = repository.allSettings.first()

        assertEquals("detence", settings.defaultVoice)
        assertEquals(1.5f, settings.speed)
        assertEquals(0.8f, settings.pitch)
        assertEquals(0.9f, settings.volume)
        assertTrue(settings.forceSpeed)
        assertFalse(settings.forcePitch)
        assertTrue(settings.forceVolume)
        assertTrue(settings.forceLanguage)
    }

    @Test
    fun `allSettings returns defaults when nothing is set`() = runTest {
        val settings = repository.allSettings.first()

        assertEquals(SettingsRepository.DEFAULT_VOICE, settings.defaultVoice)
        assertEquals(SettingsRepository.DEFAULT_SPEED, settings.speed)
        assertEquals(SettingsRepository.DEFAULT_PITCH, settings.pitch)
        assertEquals(SettingsRepository.DEFAULT_VOLUME, settings.volume)
        assertEquals(SettingsRepository.DEFAULT_FORCE_SPEED, settings.forceSpeed)
        assertEquals(SettingsRepository.DEFAULT_FORCE_PITCH, settings.forcePitch)
        assertEquals(SettingsRepository.DEFAULT_FORCE_VOLUME, settings.forceVolume)
        assertEquals(SettingsRepository.DEFAULT_FORCE_LANGUAGE, settings.forceLanguage)
    }

    // ==========================================================================
    // Inflection Enabled Settings Tests
    // ==========================================================================

    @Test
    fun `inflectionEnabled returns true when not set`() = runTest {
        val inflection = repository.inflectionEnabled.first()
        assertTrue(inflection)  // Default is true
    }

    @Test
    fun `setInflectionEnabled persists false value`() = runTest {
        repository.setInflectionEnabled(false)
        val inflection = repository.inflectionEnabled.first()
        assertFalse(inflection)
    }

    @Test
    fun `setInflectionEnabled persists true value after toggling`() = runTest {
        repository.setInflectionEnabled(false)
        repository.setInflectionEnabled(true)
        val inflection = repository.inflectionEnabled.first()
        assertTrue(inflection)
    }

    // ==========================================================================
    // Don't Ask Default TTS Settings Tests
    // ==========================================================================

    @Test
    fun `dontAskDefaultTts returns false when not set`() = runTest {
        val dontAsk = repository.dontAskDefaultTts.first()
        assertFalse(dontAsk)
    }

    @Test
    fun `setDontAskDefaultTts persists true value`() = runTest {
        repository.setDontAskDefaultTts(true)
        val dontAsk = repository.dontAskDefaultTts.first()
        assertTrue(dontAsk)
    }

    @Test
    fun `setDontAskDefaultTts persists false value`() = runTest {
        repository.setDontAskDefaultTts(true)
        repository.setDontAskDefaultTts(false)
        val dontAsk = repository.dontAskDefaultTts.first()
        assertFalse(dontAsk)
    }

    // ==========================================================================
    // Emoji Settings Tests
    // ==========================================================================

    @Test
    fun `emojiEnabled returns false when not set`() = runTest {
        val emoji = repository.emojiEnabled.first()
        assertFalse(emoji)  // Default is false
    }

    @Test
    fun `setEmojiEnabled persists true value`() = runTest {
        repository.setEmojiEnabled(true)
        val emoji = repository.emojiEnabled.first()
        assertTrue(emoji)
    }

    @Test
    fun `setEmojiEnabled persists false value after toggling`() = runTest {
        repository.setEmojiEnabled(true)
        repository.setEmojiEnabled(false)
        val emoji = repository.emojiEnabled.first()
        assertFalse(emoji)
    }

    // ==========================================================================
    // Pause Settings Tests
    // ==========================================================================

    @Test
    fun `sentencePause returns default 100 when not set`() = runTest {
        val pause = repository.sentencePause.first()
        assertEquals(SettingsRepository.DEFAULT_SENTENCE_PAUSE, pause)
    }

    @Test
    fun `setSentencePause persists valid values`() = runTest {
        repository.setSentencePause(350)
        val pause = repository.sentencePause.first()
        assertEquals(350, pause)
    }

    @Test
    fun `setSentencePause clamps negative values to 0`() = runTest {
        repository.setSentencePause(-50)
        val pause = repository.sentencePause.first()
        assertEquals(0, pause)
    }

    @Test
    fun `setSentencePause clamps values above 2000 to 2000`() = runTest {
        repository.setSentencePause(5000)
        val pause = repository.sentencePause.first()
        assertEquals(2000, pause)
    }

    @Test
    fun `commaPause returns default 100 when not set`() = runTest {
        val pause = repository.commaPause.first()
        assertEquals(SettingsRepository.DEFAULT_COMMA_PAUSE, pause)
    }

    @Test
    fun `setCommaPause persists valid values`() = runTest {
        repository.setCommaPause(75)
        val pause = repository.commaPause.first()
        assertEquals(75, pause)
    }

    @Test
    fun `setCommaPause clamps values to the 0 to 2000 range`() = runTest {
        repository.setCommaPause(-1)
        assertEquals(0, repository.commaPause.first())

        repository.setCommaPause(2001)
        assertEquals(2000, repository.commaPause.first())

        repository.setCommaPause(2000)
        assertEquals(2000, repository.commaPause.first())
    }

    @Test
    fun `newlinePause returns default 100 when not set`() = runTest {
        val pause = repository.newlinePause.first()
        assertEquals(SettingsRepository.DEFAULT_NEWLINE_PAUSE, pause)
    }

    @Test
    fun `setNewlinePause persists valid values`() = runTest {
        repository.setNewlinePause(600)
        val pause = repository.newlinePause.first()
        assertEquals(600, pause)
    }

    @Test
    fun `setNewlinePause clamps values to the 0 to 2000 range`() = runTest {
        repository.setNewlinePause(Int.MIN_VALUE)
        assertEquals(0, repository.newlinePause.first())

        repository.setNewlinePause(Int.MAX_VALUE)
        assertEquals(2000, repository.newlinePause.first())

        repository.setNewlinePause(0)
        assertEquals(0, repository.newlinePause.first())
    }

    // ==========================================================================
    // Number Mode Settings Tests
    // ==========================================================================

    @Test
    fun `numberMode returns whole numbers mode when not set`() = runTest {
        val mode = repository.numberMode.first()
        assertEquals(SettingsRepository.DEFAULT_NUMBER_MODE, mode)
        assertEquals(0, mode)
    }

    @Test
    fun `setNumberMode persists digit by digit mode`() = runTest {
        repository.setNumberMode(1)
        val mode = repository.numberMode.first()
        assertEquals(1, mode)
    }

    @Test
    fun `setNumberMode persists whole numbers mode after toggling`() = runTest {
        repository.setNumberMode(1)
        repository.setNumberMode(0)
        val mode = repository.numberMode.first()
        assertEquals(0, mode)
    }

    @Test
    fun `setNumberMode clamps values to the 0 to 1 range`() = runTest {
        repository.setNumberMode(-3)
        assertEquals(0, repository.numberMode.first())

        repository.setNumberMode(7)
        assertEquals(1, repository.numberMode.first())
    }

    // ==========================================================================
    // User Dictionaries Settings Tests
    // ==========================================================================

    @Test
    fun `userDictionariesEnabled returns true when not set`() = runTest {
        val enabled = repository.userDictionariesEnabled.first()
        assertTrue(enabled)  // Default is true
    }

    @Test
    fun `setUserDictionariesEnabled persists false value`() = runTest {
        repository.setUserDictionariesEnabled(false)
        val enabled = repository.userDictionariesEnabled.first()
        assertFalse(enabled)
    }

    @Test
    fun `setUserDictionariesEnabled persists true value after toggling`() = runTest {
        repository.setUserDictionariesEnabled(false)
        repository.setUserDictionariesEnabled(true)
        val enabled = repository.userDictionariesEnabled.first()
        assertTrue(enabled)
    }

    @Test
    fun `allSettings includes advanced and dictionary settings`() = runTest {
        repository.setEmojiEnabled(true)
        repository.setInflectionEnabled(false)
        repository.setSentencePause(250)
        repository.setCommaPause(150)
        repository.setNewlinePause(300)
        repository.setNumberMode(1)
        repository.setUserDictionariesEnabled(false)

        val settings = repository.allSettings.first()

        assertTrue(settings.emojiEnabled)
        assertFalse(settings.inflectionEnabled)
        assertEquals(250, settings.sentencePause)
        assertEquals(150, settings.commaPause)
        assertEquals(300, settings.newlinePause)
        assertEquals(1, settings.numberMode)
        assertFalse(settings.userDictionariesEnabled)
    }

    @Test
    fun `allSettings advanced values default when nothing is set`() = runTest {
        val settings = repository.allSettings.first()

        assertEquals(SettingsRepository.DEFAULT_EMOJI_ENABLED, settings.emojiEnabled)
        assertEquals(SettingsRepository.DEFAULT_INFLECTION_ENABLED, settings.inflectionEnabled)
        assertEquals(SettingsRepository.DEFAULT_SENTENCE_PAUSE, settings.sentencePause)
        assertEquals(SettingsRepository.DEFAULT_COMMA_PAUSE, settings.commaPause)
        assertEquals(SettingsRepository.DEFAULT_NEWLINE_PAUSE, settings.newlinePause)
        assertEquals(SettingsRepository.DEFAULT_NUMBER_MODE, settings.numberMode)
        assertEquals(SettingsRepository.DEFAULT_USER_DICTIONARIES_ENABLED, settings.userDictionariesEnabled)
        assertEquals(SettingsRepository.TTSSettings(), settings)
    }

    // ==========================================================================
    // readSettingsNow Tests
    // ==========================================================================

    @Test
    fun `readSettingsNow returns the current values without a migrator`() = runTest {
        repository.setDefaultVoice("baba")
        repository.setSpeed(1.2f)
        repository.setForceVolume(true)
        repository.setCommaPause(400)

        val settings = repository.readSettingsNow()

        assertEquals("baba", settings.defaultVoice)
        assertEquals(1.2f, settings.speed)
        assertTrue(settings.forceVolume)
        assertEquals(400, settings.commaPause)
        assertEquals(repository.allSettings.first(), settings)
        assertNull(repository.lastMigrationResult)
        assertNull(repository.storageError.value)
    }

    @Test
    fun `readSettingsNow returns defaults on an empty store`() = runTest {
        assertEquals(SettingsRepository.TTSSettings(), repository.readSettingsNow())
    }

    @Test
    fun `readSettingsNow does not run the migration`() = runTest {
        val legacy = legacyFile()
        seedLegacy(LEGACY_PREFS, legacy)
        val migrated = SettingsRepository(testDataStore, migratorOver(legacy))

        val beforeMigration = migrated.readSettingsNow()

        assertEquals("the bounded startup read must not wait for the migration", SettingsRepository.TTSSettings(), beforeMigration)
        assertNull(migrated.lastMigrationResult)
        assertTrue("legacy file must be untouched", legacy.isFile)

        // The regular flows still migrate on their first collection.
        assertEquals("vlado", migrated.allSettings.first().defaultVoice)
        assertEquals("vlado", migrated.readSettingsNow().defaultVoice)
        assertFalse(legacy.exists())
    }

    // ==========================================================================
    // Legacy Storage Migration Tests
    // ==========================================================================

    @Test
    fun `legacy values are visible on the first read through the flows`() = runTest {
        val legacy = legacyFile()
        seedLegacy(LEGACY_PREFS, legacy)
        val migrated = SettingsRepository(testDataStore, migratorOver(legacy))

        val settings = migrated.allSettings.first()

        assertEquals("vlado", settings.defaultVoice)
        assertEquals(1.5f, settings.speed)
        assertTrue(settings.forceSpeed)
        assertEquals(250, settings.sentencePause)
        assertEquals(1, settings.numberMode)
        assertEquals(MigrationResult.Migrated(LEGACY_PREFS.asMap().size), migrated.lastMigrationResult)
        assertNull(migrated.storageError.value)
        assertFalse("legacy file must be gone after migration", legacy.exists())

        // Every individual flow reads the same migrated store.
        assertEquals("vlado", migrated.defaultVoice.first())
        assertEquals(1.5f, migrated.speed.first())
        assertTrue(migrated.forceSpeed.first())
        assertEquals(250, migrated.sentencePause.first())
        assertEquals(1, migrated.numberMode.first())
    }

    @Test
    fun `values written before the migration win over legacy values`() = runTest {
        val legacy = legacyFile()
        seedLegacy(LEGACY_PREFS, legacy)
        // Written through a repository without a migrator, i.e. already in the target store.
        repository.setSpeed(0.8f)
        val migrated = SettingsRepository(testDataStore, migratorOver(legacy))

        val settings = migrated.allSettings.first()

        assertEquals(0.8f, settings.speed)
        assertEquals("vlado", settings.defaultVoice)
        assertEquals(MigrationResult.Migrated(LEGACY_PREFS.asMap().size - 1), migrated.lastMigrationResult)
        assertFalse(legacy.exists())
    }

    @Test
    fun `setters run the migration before writing`() = runTest {
        val legacy = legacyFile()
        seedLegacy(LEGACY_PREFS, legacy)
        val migrated = SettingsRepository(testDataStore, migratorOver(legacy))

        migrated.setPitch(0.6f)

        assertEquals(MigrationResult.Migrated(LEGACY_PREFS.asMap().size), migrated.lastMigrationResult)
        assertFalse(legacy.exists())
        val settings = migrated.allSettings.first()
        assertEquals(0.6f, settings.pitch)
        assertEquals("vlado", settings.defaultVoice)
        assertEquals(1.5f, settings.speed)
    }

    @Test
    fun `onUserUnlocked re-arms the migration after a locked start`() = runTest {
        val legacy = legacyFile()
        seedLegacy(LEGACY_PREFS, legacy)
        var unlocked = false
        val migrated = SettingsRepository(testDataStore, migratorOver(legacy) { unlocked })

        val locked = migrated.allSettings.first()

        assertEquals(SettingsRepository.DEFAULT_VOICE, locked.defaultVoice)
        assertEquals(SettingsRepository.DEFAULT_SPEED, locked.speed)
        assertEquals(SettingsRepository.TTSSettings(), locked)
        assertEquals(MigrationResult.SkippedLocked, migrated.lastMigrationResult)
        assertNull("a locked start is not an error", migrated.storageError.value)
        assertTrue("legacy data must survive a locked start", legacy.isFile)

        unlocked = true
        migrated.onUserUnlocked()

        val afterUnlock = migrated.allSettings.first()
        assertEquals("vlado", afterUnlock.defaultVoice)
        assertEquals(1.5f, afterUnlock.speed)
        assertTrue(afterUnlock.forceSpeed)
        assertEquals(MigrationResult.Migrated(LEGACY_PREFS.asMap().size), migrated.lastMigrationResult)
        assertNull(migrated.storageError.value)
        assertFalse(legacy.exists())
    }

    @Test
    fun `onUserUnlocked without a migrator is a no-op`() = runTest {
        repository.setDefaultVoice("djed")

        repository.onUserUnlocked()

        assertEquals("djed", repository.defaultVoice.first())
        assertNull(repository.lastMigrationResult)
    }

    @Test
    fun `onUserUnlocked after a completed migration does not migrate a reappearing legacy file twice`() = runTest {
        val legacy = legacyFile()
        seedLegacy(LEGACY_PREFS, legacy)
        val migrated = SettingsRepository(testDataStore, migratorOver(legacy))
        assertEquals("vlado", migrated.allSettings.first().defaultVoice)
        migrated.setDefaultVoice("baba")

        // A restored backup brings the legacy file back; the re-armed migration
        // must not overwrite what the user changed since (target wins).
        seedLegacy(LEGACY_PREFS, legacy)
        migrated.onUserUnlocked()

        val settings = migrated.allSettings.first()
        assertEquals("baba", settings.defaultVoice)
        assertEquals(1.5f, settings.speed)
        assertEquals(MigrationResult.Migrated(0), migrated.lastMigrationResult)
        assertFalse(legacy.exists())
    }

    // ==========================================================================
    // Migration Failure Containment Tests
    // ==========================================================================

    @Test
    fun `a throwing migrator is contained and reported through storageError`() = runTest {
        val migrator = object : LegacyMigrator(isUserUnlocked = { true }, logger = StorageLogger.None) {
            override suspend fun performMigration(): MigrationResult {
                throw DirectBootMigrationException("boom")
            }
        }
        val contained = SettingsRepository(testDataStore, migrator)

        val settings = contained.allSettings.first()

        assertEquals(SettingsRepository.TTSSettings(), settings)
        assertEquals("boom", contained.storageError.value)
        assertTrue(
            "a contained failure is reported as RetryLater",
            contained.lastMigrationResult is MigrationResult.RetryLater
        )
        assertFalse(migrator.isDone)

        // Setters keep working on the current store; the failure is never re-thrown at callers.
        contained.setSpeed(1.5f)
        assertEquals(1.5f, contained.speed.first())
        assertEquals("boom", contained.storageError.value)
        // The failed attempt is rate-limited, so later calls see RetryLater instead of a new throw.
        assertTrue(
            "expected RetryLater but was ${contained.lastMigrationResult}",
            contained.lastMigrationResult is MigrationResult.RetryLater
        )
    }

    @Test
    fun `an unexpected exception from the migrator is contained as well`() = runTest {
        val migrator = object : LegacyMigrator(isUserUnlocked = { true }, logger = StorageLogger.None) {
            override suspend fun performMigration(): MigrationResult {
                throw IOException("disk on fire")
            }
        }
        val contained = SettingsRepository(testDataStore, migrator)

        assertEquals(SettingsRepository.TTSSettings(), contained.allSettings.first())
        assertEquals("disk on fire", contained.storageError.value)
        assertTrue(contained.lastMigrationResult is MigrationResult.RetryLater)
        // The unexpected failure is rate-limited like an expected one: the next
        // read does not re-run the migrator but still reports RetryLater.
        assertEquals(SettingsRepository.TTSSettings(), contained.allSettings.first())
        assertTrue(contained.lastMigrationResult is MigrationResult.RetryLater)
    }

    @Test
    fun `a successful migration after a failure clears storageError`() = runTest {
        var now = 0L
        var fail = true
        val migrator = object : LegacyMigrator(
            isUserUnlocked = { true },
            logger = StorageLogger.None,
            clock = { now }
        ) {
            override suspend fun performMigration(): MigrationResult {
                if (fail) throw DirectBootMigrationException("boom")
                return MigrationResult.Migrated(1)
            }
        }
        val recovering = SettingsRepository(testDataStore, migrator)

        recovering.allSettings.first()
        assertEquals("boom", recovering.storageError.value)

        fail = false
        recovering.allSettings.first()
        assertTrue(recovering.lastMigrationResult is MigrationResult.RetryLater)
        assertEquals("still reported while rate-limited", "boom", recovering.storageError.value)

        now = LegacyMigrator.DEFAULT_RETRY_INTERVAL_NANOS + 1
        recovering.allSettings.first()
        assertEquals(MigrationResult.Migrated(1), recovering.lastMigrationResult)
        assertNull(recovering.storageError.value)
    }

    @Test
    fun `quarantined corrupt legacy data yields defaults without an error`() = runTest {
        val legacy = legacyFile()
        legacy.parentFile!!.mkdirs()
        legacy.writeBytes(CORRUPT_BYTES)
        val migrated = SettingsRepository(testDataStore, migratorOver(legacy))

        val settings = migrated.allSettings.first()

        assertEquals(SettingsRepository.TTSSettings(), settings)
        assertEquals(MigrationResult.QuarantinedCorrupt, migrated.lastMigrationResult)
        assertNull(migrated.storageError.value)
        assertFalse(legacy.exists())
        assertTrue(File(legacy.parentFile, legacy.name + SettingsMigrator.CORRUPT_SUFFIX).isFile)
    }

    @Test
    fun `a simulated crash propagates and the next read converges`() = runTest {
        val legacy = legacyFile()
        seedLegacy(LEGACY_PREFS, legacy)
        val injector = OneShotCrashInjector(MigrationCrashPoint.SETTINGS_AFTER_COMMIT)
        val migrator = SettingsMigrator(
            legacyFile = { legacy },
            target = testDataStore,
            isUserUnlocked = { true },
            faultInjector = injector,
            logger = StorageLogger.None
        )
        val crashing = SettingsRepository(testDataStore, migrator)

        val crash = assertSuspendThrows(SimulatedMigrationCrashException::class.java) {
            crashing.allSettings.first()
        }

        assertEquals(MigrationCrashPoint.SETTINGS_AFTER_COMMIT, crash.point)
        assertNull("a simulated crash is not a storage error", crashing.storageError.value)
        assertNull(crashing.lastMigrationResult)
        assertTrue("the legacy file survives a crash after the commit", legacy.isFile)

        // The next collector (or the restarted process) finishes the job.
        val settings = crashing.allSettings.first()
        assertEquals("vlado", settings.defaultVoice)
        assertEquals(1.5f, settings.speed)
        assertEquals(MigrationResult.Migrated(0), crashing.lastMigrationResult)
        assertFalse(legacy.exists())
    }

    // ==========================================================================
    // Corruption Handler Tests
    // ==========================================================================

    @Test
    fun `garbage in the store file yields defaults through the corruption handler`() = runTest {
        val corruptFile = tempFolder.newFile("corrupt_settings.preferences_pb")
        corruptFile.writeBytes(CORRUPT_BYTES)
        val corruptStore = PreferenceDataStoreFactory.create(
            corruptionHandler = ReplaceFileCorruptionHandler { emptyPreferences() },
            scope = testScope,
            produceFile = { corruptFile }
        )
        val recovered = SettingsRepository(corruptStore)

        val settings = recovered.allSettings.first()

        assertEquals(SettingsRepository.TTSSettings(), settings)
        assertEquals(SettingsRepository.TTSSettings(), recovered.readSettingsNow())
        assertNull(recovered.storageError.value)

        // The handler replaced the file, so the store keeps working and persists again.
        recovered.setDefaultVoice("vlado")
        assertEquals("vlado", recovered.defaultVoice.first())
        val onDisk = corruptFile.inputStream().use { PreferencesFileSerializer.readFrom(it) }
        assertEquals("vlado", onDisk[KEY_DEFAULT_VOICE])
    }

    @Test
    fun `garbage in the store file without a corruption handler fails the read`() = runTest {
        val corruptFile = tempFolder.newFile("unhandled_corrupt_settings.preferences_pb")
        corruptFile.writeBytes(CORRUPT_BYTES)
        val corruptStore = PreferenceDataStoreFactory.create(
            scope = testScope,
            produceFile = { corruptFile }
        )
        val unprotected = SettingsRepository(corruptStore)

        // Documents why the production store is created with ReplaceFileCorruptionHandler.
        assertSuspendThrows(CorruptionException::class.java) { unprotected.allSettings.first() }
        assertTrue("an unhandled corruption leaves the file as it was", corruptFile.readBytes().contentEquals(CORRUPT_BYTES))
    }

    // ==========================================================================
    // Migration Test Helpers
    // ==========================================================================

    /** Legacy DataStore file in a simulated credential-encrypted files directory. */
    private fun legacyFile(): File =
        File(tempFolder.root, "ce/datastore/laprdus_settings.preferences_pb")

    /** Writes [prefs] as a legacy DataStore file without opening a DataStore on it. */
    private suspend fun seedLegacy(prefs: Preferences, file: File) {
        file.parentFile!!.mkdirs()
        file.outputStream().use { PreferencesFileSerializer.writeTo(prefs, it) }
        assertTrue("seeding must produce a regular file", file.isFile)
    }

    private fun migratorOver(legacy: File, isUserUnlocked: () -> Boolean = { true }): SettingsMigrator =
        SettingsMigrator(
            legacyFile = { legacy },
            target = testDataStore,
            isUserUnlocked = isUserUnlocked,
            logger = StorageLogger.None
        )

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
        // Same names and types as the private keys in SettingsRepository.
        private val KEY_DEFAULT_VOICE = stringPreferencesKey("default_voice")
        private val KEY_SPEED = floatPreferencesKey("speed")
        private val KEY_FORCE_SPEED = booleanPreferencesKey("force_speed")
        private val KEY_SENTENCE_PAUSE = intPreferencesKey("sentence_pause")
        private val KEY_NUMBER_MODE = intPreferencesKey("number_mode")

        /** Legacy content: one key of every preference type, all non-default. */
        private val LEGACY_PREFS: Preferences = mutablePreferencesOf(
            KEY_DEFAULT_VOICE to "vlado",
            KEY_SPEED to 1.5f,
            KEY_FORCE_SPEED to true,
            KEY_SENTENCE_PAUSE to 250,
            KEY_NUMBER_MODE to 1
        ).toPreferences()

        /**
         * Protobuf tags with wire type 7, which does not exist: every parser
         * rejects them, unlike random bytes that may decode as unknown fields.
         */
        private val CORRUPT_BYTES = byteArrayOf(0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F)
    }
}
