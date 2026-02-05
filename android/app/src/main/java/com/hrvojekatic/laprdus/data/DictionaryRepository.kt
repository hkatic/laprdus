package com.hrvojekatic.laprdus.data

import android.content.Context
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.withContext
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.util.UUID
import javax.inject.Inject
import javax.inject.Singleton

/**
 * Types of user dictionaries supported by Laprdus.
 */
enum class DictionaryType {
    /** Main pronunciation dictionary (user.json) */
    MAIN,
    /** Spelling dictionary for character-by-character reading (spelling.json) */
    SPELLING,
    /** Emoji dictionary for emoji to text conversion (emoji.json) */
    EMOJI
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
 * Dictionary files are stored in the app's internal files directory.
 */
@Singleton
class DictionaryRepository @Inject constructor(
    @ApplicationContext private val context: Context
) {
    private val _entries = MutableStateFlow<List<DictionaryEntry>>(emptyList())

    /** Flow of current dictionary entries */
    val entries: Flow<List<DictionaryEntry>> = _entries.asStateFlow()

    private var currentType: DictionaryType = DictionaryType.MAIN

    /**
     * Get the file for a dictionary type.
     */
    private fun getDictionaryFile(type: DictionaryType): File {
        val filename = when (type) {
            DictionaryType.MAIN -> "user.json"
            DictionaryType.SPELLING -> "spelling.json"
            DictionaryType.EMOJI -> "emoji.json"
        }
        return File(context.filesDir, filename)
    }

    /**
     * Load a dictionary from file.
     * @param type The type of dictionary to load
     * @return Result containing the list of entries or an error
     */
    suspend fun loadDictionary(type: DictionaryType): Result<List<DictionaryEntry>> =
        withContext(Dispatchers.IO) {
            currentType = type
            val file = getDictionaryFile(type)

            if (!file.exists()) {
                _entries.value = emptyList()
                return@withContext Result.success(emptyList())
            }

            try {
                val json = file.readText(Charsets.UTF_8)
                val entries = parseDictionaryJson(json)
                _entries.value = entries
                Result.success(entries)
            } catch (e: Exception) {
                _entries.value = emptyList()
                Result.failure(e)
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
        try {
            val current = _entries.value.toMutableList()
            val existingIndex = current.indexOfFirst { it.id == entry.id }

            if (existingIndex >= 0) {
                current[existingIndex] = entry
            } else {
                current.add(entry)
            }

            _entries.value = current
            saveDictionary(currentType, current)
            Result.success(Unit)
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    /**
     * Delete an entry from the current dictionary.
     * @param entryId The ID of the entry to delete
     * @return Result indicating success or failure
     */
    suspend fun deleteEntry(entryId: String): Result<Unit> = withContext(Dispatchers.IO) {
        try {
            val current = _entries.value.filterNot { it.id == entryId }
            _entries.value = current
            saveDictionary(currentType, current)
            Result.success(Unit)
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    /**
     * Get the current dictionary type.
     */
    fun getCurrentType(): DictionaryType = currentType

    /**
     * Save dictionary entries to file.
     */
    private fun saveDictionary(type: DictionaryType, entries: List<DictionaryEntry>) {
        val file = getDictionaryFile(type)
        val json = generateDictionaryJson(entries)
        file.writeText(json, Charsets.UTF_8)
    }

    /**
     * Parse dictionary JSON into entries.
     */
    private fun parseDictionaryJson(json: String): List<DictionaryEntry> {
        val entries = mutableListOf<DictionaryEntry>()

        try {
            val jsonObj = JSONObject(json)
            val entriesArray = jsonObj.optJSONArray("entries") ?: return entries

            for (i in 0 until entriesArray.length()) {
                val entryObj = entriesArray.getJSONObject(i)
                val grapheme = entryObj.optString("grapheme", "")
                val phoneme = entryObj.optString("phoneme", "")

                if (grapheme.isNotEmpty()) {
                    entries.add(
                        DictionaryEntry(
                            grapheme = grapheme,
                            phoneme = phoneme,
                            caseSensitive = entryObj.optBoolean("caseSensitive", false),
                            wholeWord = entryObj.optBoolean("wholeWord", true),
                            comment = entryObj.optString("comment", "")
                        )
                    )
                }
            }
        } catch (e: Exception) {
            // Return empty list on parse error
        }

        return entries
    }

    /**
     * Generate dictionary JSON from entries.
     */
    private fun generateDictionaryJson(entries: List<DictionaryEntry>): String {
        val root = JSONObject()
        root.put("version", "1.0")

        val entriesArray = JSONArray()
        entries.forEach { entry ->
            val entryObj = JSONObject()
            entryObj.put("grapheme", entry.grapheme)
            entryObj.put("phoneme", entry.phoneme)
            entryObj.put("caseSensitive", entry.caseSensitive)
            entryObj.put("wholeWord", entry.wholeWord)
            if (entry.comment.isNotEmpty()) {
                entryObj.put("comment", entry.comment)
            }
            entriesArray.put(entryObj)
        }
        root.put("entries", entriesArray)

        return root.toString(4)
    }
}
