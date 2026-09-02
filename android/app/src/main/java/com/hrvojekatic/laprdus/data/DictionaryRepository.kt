package com.hrvojekatic.laprdus.data

import android.content.Context
import com.hrvojekatic.laprdus.data.migration.DictionaryMigrator
import com.hrvojekatic.laprdus.data.migration.LegacyMigrator
import com.hrvojekatic.laprdus.data.migration.MigrationResult
import com.hrvojekatic.laprdus.data.migration.SimulatedMigrationCrashException
import com.hrvojekatic.laprdus.data.storage.LaprdusStorage
import com.hrvojekatic.laprdus.data.storage.StorageLogger
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlinx.coroutines.withContext
import java.io.File
import java.util.UUID

/**
 * Types of user dictionaries supported by Laprdus.
 */
enum class DictionaryType(val fileName: String) {
    /** Main pronunciation dictionary (user.json) */
    MAIN("user.json"),
    /** Spelling dictionary for character-by-character reading (spelling.json) */
    SPELLING("spelling.json"),
    /** Emoji dictionary for emoji to text conversion (emoji.json) */
    EMOJI("emoji.json")
}

/**
 * Represents a single entry in a user dictionary.
 */
data class DictionaryEntry(
    /** Unique identifier for this entry */
    val id: String = UUID.randomUUID().toString(),
    /** Original text to match */
    val grapheme: String,
    /** Replacement/pronunciation text */
    val phoneme: String,
    /** Whether matching is case-sensitive */
    val caseSensitive: Boolean = false,
    /** Whether to match whole words only */
    val wholeWord: Boolean = true,
    /** Optional comment explaining this entry */
    val comment: String = ""
)

/**
 * Repository for managing user dictionaries.
 * Handles loading, saving, and CRUD operations on dictionary entries.
 *
 * Dictionary files are stored in the app's device-protected files directory
 * (see [LaprdusStorage]) so the TTS service can apply them on the lock screen.
 * Files written by older versions into credential-encrypted storage are moved
 * by [DictionaryMigrator] before the first load or save; migration problems
 * are logged and reported through [storageError], never thrown.
 *
 * Writes are atomic (temp file + rename) because the TTS service reads
 * `user.json` on its synthesis thread while the UI edits it, and all
 * operations are serialized by a mutex.
 */
class DictionaryRepository internal constructor(
    private val dictionaryDir: File,
    private val migrator: LegacyMigrator? = null,
    private val logger: StorageLogger = StorageLogger.None,
) {
    companion object {
        /** Production repository over device-protected storage with the shared migrator. */
        fun create(context: Context): DictionaryRepository = DictionaryRepository(
            dictionaryDir = LaprdusStorage.dictionaryDir(context),
            migrator = LaprdusStorage.dictionaryMigrator(context),
            logger = LaprdusStorage.logger("DictionaryRepository")
        )
    }

    private val mutex = Mutex()

    private val _entries = MutableStateFlow<List<DictionaryEntry>>(emptyList())

    /** Flow of current dictionary entries */
    val entries: Flow<List<DictionaryEntry>> = _entries.asStateFlow()

    private val _storageError = MutableStateFlow<String?>(null)

    /** Human-readable description of the last storage/migration failure, or null. */
    val storageError: StateFlow<String?> = _storageError.asStateFlow()

    /** Result of the most recent migration attempt (diagnostics and tests). */
    @Volatile
    var lastMigrationResult: MigrationResult? = null
        private set

    @Volatile
    private var currentType: DictionaryType = DictionaryType.MAIN

    /**
     * Get the file for a dictionary type.
     */
    internal fun getDictionaryFile(type: DictionaryType): File = File(dictionaryDir, type.fileName)

    /**
     * Runs the legacy-storage migration if it is still pending. Never throws
     * (except for simulated crashes in debug/test builds).
     */
    internal suspend fun ensureMigrated(): MigrationResult? {
        val migrator = migrator ?: return null
        return try {
            val result = migrator.migrateIfNeeded()
            if (result is MigrationResult.Migrated) _storageError.value = null
            lastMigrationResult = result
            result
        } catch (e: SimulatedMigrationCrashException) {
            throw e
        } catch (e: CancellationException) {
            throw e
        } catch (e: Exception) {
            logger.error("Dictionary migration failed; continuing with current dictionaries", e)
            _storageError.value = e.message ?: e.javaClass.simpleName
            MigrationResult.RetryLater(e).also { lastMigrationResult = it }
        }
    }

    /**
     * Load a dictionary from file.
     * @param type The type of dictionary to load
     * @return Result containing the list of entries or an error
     */
    suspend fun loadDictionary(type: DictionaryType): Result<List<DictionaryEntry>> =
        withContext(Dispatchers.IO) {
            mutex.withLock {
                currentType = type
                ensureMigrated()
                val file = getDictionaryFile(type)

                if (!file.exists()) {
                    _entries.value = emptyList()
                    return@withLock Result.success(emptyList())
                }

                try {
                    val json = file.readText(Charsets.UTF_8)
                    val entries = DictionaryJson.parse(json)
                    _entries.value = entries
                    Result.success(entries)
                } catch (e: CancellationException) {
                    throw e
                } catch (e: Exception) {
                    logger.warn("Failed to load dictionary ${type.fileName}", e)
                    _entries.value = emptyList()
                    Result.failure(e)
                }
            }
        }

    /**
     * Save an entry to the current dictionary.
     * If the entry already exists (by ID), it will be updated.
     * Otherwise, it will be added.
     * @param entry The entry to save
     * @return Result indicating success or failure
     */
    suspend fun saveEntry(entry: DictionaryEntry): Result<Unit> = withContext(Dispatchers.IO) {
        mutex.withLock {
            try {
                ensureMigrated()
                val current = _entries.value.toMutableList()
                val existingIndex = current.indexOfFirst { it.id == entry.id }

                if (existingIndex >= 0) {
                    current[existingIndex] = entry
                } else {
                    current.add(entry)
                }

                saveDictionary(currentType, current)
                _entries.value = current
                Result.success(Unit)
            } catch (e: CancellationException) {
                throw e
            } catch (e: Exception) {
                logger.error("Failed to save dictionary entry", e)
                Result.failure(e)
            }
        }
    }

    /**
     * Delete an entry from the current dictionary.
     * @param entryId The ID of the entry to delete
     * @return Result indicating success or failure
     */
    suspend fun deleteEntry(entryId: String): Result<Unit> = withContext(Dispatchers.IO) {
        mutex.withLock {
            try {
                ensureMigrated()
                val current = _entries.value.filterNot { it.id == entryId }
                saveDictionary(currentType, current)
                _entries.value = current
                Result.success(Unit)
            } catch (e: CancellationException) {
                throw e
            } catch (e: Exception) {
                logger.error("Failed to delete dictionary entry", e)
                Result.failure(e)
            }
        }
    }

    /**
     * Get the current dictionary type.
     */
    fun getCurrentType(): DictionaryType = currentType

    /**
     * Save dictionary entries to file atomically.
     */
    private fun saveDictionary(type: DictionaryType, entries: List<DictionaryEntry>) {
        AtomicFiles.writeTextAtomically(getDictionaryFile(type), DictionaryJson.generate(entries))
    }
}
