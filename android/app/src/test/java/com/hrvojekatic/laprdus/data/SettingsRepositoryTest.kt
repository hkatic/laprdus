package com.hrvojekatic.laprdus.data

import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.PreferenceDataStoreFactory
import androidx.datastore.preferences.core.Preferences
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
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder

/**
 * Unit tests for SettingsRepository.
 * Tests cover all settings including new force toggles and restore default methods.
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
}
