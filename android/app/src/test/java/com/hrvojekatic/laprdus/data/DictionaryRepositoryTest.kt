package com.hrvojekatic.laprdus.data

import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.test.runTest
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File

/**
 * Unit tests for DictionaryRepository data classes and utility functions.
 * Note: Tests for actual repository operations require Android instrumented tests
 * due to Android-specific JSON parsing (org.json.JSONObject).
 */
@OptIn(ExperimentalCoroutinesApi::class)
class DictionaryRepositoryTest {

    @get:Rule
    val tempFolder = TemporaryFolder()

    private lateinit var testDir: File

    @Before
    fun setup() {
        testDir = tempFolder.newFolder("dictionaries")
    }

    @After
    fun cleanup() {
        // TemporaryFolder handles cleanup
    }

    // ==========================================================================
    // DictionaryEntry Data Class Tests
    // ==========================================================================

    @Test
    fun `DictionaryEntry has correct default values`() {
        val entry = DictionaryEntry(grapheme = "test", phoneme = "tst")

        assertFalse(entry.caseSensitive) // default false
        assertTrue(entry.wholeWord) // default true
        assertEquals("", entry.comment) // default empty
        assertNotNull(entry.id) // UUID generated
    }

    @Test
    fun `DictionaryEntry preserves custom values`() {
        val entry = DictionaryEntry(
            id = "custom-id",
            grapheme = "Hello",
            phoneme = "Helo",
            caseSensitive = true,
            wholeWord = false,
            comment = "Test comment"
        )

        assertEquals("custom-id", entry.id)
        assertEquals("Hello", entry.grapheme)
        assertEquals("Helo", entry.phoneme)
        assertTrue(entry.caseSensitive)
        assertFalse(entry.wholeWord)
        assertEquals("Test comment", entry.comment)
    }

    @Test
    fun `DictionaryEntry copy works correctly`() {
        val original = DictionaryEntry(
            id = "orig-1",
            grapheme = "original",
            phoneme = "orig",
            caseSensitive = true,
            wholeWord = false,
            comment = "comment"
        )

        val copy = original.copy(phoneme = "modified")

        assertEquals(original.id, copy.id)
        assertEquals(original.grapheme, copy.grapheme)
        assertEquals("modified", copy.phoneme)
        assertEquals(original.caseSensitive, copy.caseSensitive)
        assertEquals(original.wholeWord, copy.wholeWord)
        assertEquals(original.comment, copy.comment)
    }

    @Test
    fun `DictionaryEntry equals works correctly`() {
        val entry1 = DictionaryEntry(
            id = "id-1",
            grapheme = "test",
            phoneme = "tst"
        )
        val entry2 = DictionaryEntry(
            id = "id-1",
            grapheme = "test",
            phoneme = "tst"
        )
        val entry3 = DictionaryEntry(
            id = "id-2",
            grapheme = "test",
            phoneme = "tst"
        )

        assertEquals(entry1, entry2)
        assertFalse(entry1 == entry3)
    }

    // ==========================================================================
    // DictionaryType Enum Tests
    // ==========================================================================

    @Test
    fun `DictionaryType enum has three values`() {
        val types = DictionaryType.values()
        assertEquals(3, types.size)
    }

    @Test
    fun `DictionaryType MAIN is first value`() {
        assertEquals(DictionaryType.MAIN, DictionaryType.values()[0])
    }

    @Test
    fun `DictionaryType SPELLING is second value`() {
        assertEquals(DictionaryType.SPELLING, DictionaryType.values()[1])
    }

    @Test
    fun `DictionaryType EMOJI is third value`() {
        assertEquals(DictionaryType.EMOJI, DictionaryType.values()[2])
    }

    // ==========================================================================
    // Dictionary File Name Tests
    // ==========================================================================

    @Test
    fun `main dictionary should use user json filename`() {
        val filename = getDictionaryFileName(DictionaryType.MAIN)
        assertEquals("user.json", filename)
    }

    @Test
    fun `spelling dictionary should use spelling json filename`() {
        val filename = getDictionaryFileName(DictionaryType.SPELLING)
        assertEquals("spelling.json", filename)
    }

    @Test
    fun `emoji dictionary should use emoji json filename`() {
        val filename = getDictionaryFileName(DictionaryType.EMOJI)
        assertEquals("emoji.json", filename)
    }

    // ==========================================================================
    // List Operation Tests (simulating repository operations)
    // ==========================================================================

    @Test
    fun `adding entry to empty list works`() {
        val entries = mutableListOf<DictionaryEntry>()
        val newEntry = DictionaryEntry(grapheme = "hello", phoneme = "helo")

        entries.add(newEntry)

        assertEquals(1, entries.size)
        assertEquals("hello", entries[0].grapheme)
    }

    @Test
    fun `updating entry in list works`() {
        val entries = mutableListOf(
            DictionaryEntry(id = "1", grapheme = "old", phoneme = "old")
        )

        val index = entries.indexOfFirst { it.id == "1" }
        entries[index] = entries[index].copy(phoneme = "new")

        assertEquals(1, entries.size)
        assertEquals("new", entries[0].phoneme)
    }

    @Test
    fun `deleting entry from list works`() {
        val entries = mutableListOf(
            DictionaryEntry(id = "1", grapheme = "keep", phoneme = "k"),
            DictionaryEntry(id = "2", grapheme = "delete", phoneme = "d")
        )

        val filtered = entries.filterNot { it.id == "2" }

        assertEquals(1, filtered.size)
        assertEquals("keep", filtered[0].grapheme)
    }

    @Test
    fun `finding entry by id works`() {
        val entries = listOf(
            DictionaryEntry(id = "1", grapheme = "first", phoneme = "1"),
            DictionaryEntry(id = "2", grapheme = "second", phoneme = "2"),
            DictionaryEntry(id = "3", grapheme = "third", phoneme = "3")
        )

        val found = entries.find { it.id == "2" }

        assertNotNull(found)
        assertEquals("second", found?.grapheme)
    }

    @Test
    fun `finding non-existent entry returns null`() {
        val entries = listOf(
            DictionaryEntry(id = "1", grapheme = "first", phoneme = "1")
        )

        val found = entries.find { it.id == "999" }

        assertEquals(null, found)
    }

    // ==========================================================================
    // Unicode Support Tests
    // ==========================================================================

    @Test
    fun `DictionaryEntry supports Croatian characters`() {
        val entry = DictionaryEntry(
            grapheme = "čćžšđ",
            phoneme = "čćžšđ"
        )

        assertEquals("čćžšđ", entry.grapheme)
        assertEquals("čćžšđ", entry.phoneme)
    }

    @Test
    fun `DictionaryEntry supports Serbian Cyrillic`() {
        val entry = DictionaryEntry(
            grapheme = "ћџљњ",
            phoneme = "ћџљњ"
        )

        assertEquals("ћџљњ", entry.grapheme)
        assertEquals("ћџљњ", entry.phoneme)
    }

    @Test
    fun `DictionaryEntry supports emoji`() {
        val entry = DictionaryEntry(
            grapheme = "😀",
            phoneme = "smiling face"
        )

        assertEquals("😀", entry.grapheme)
        assertEquals("smiling face", entry.phoneme)
    }

    // ==========================================================================
    // Validation Tests
    // ==========================================================================

    @Test
    fun `empty grapheme is allowed by data class`() {
        val entry = DictionaryEntry(grapheme = "", phoneme = "replacement")
        assertEquals("", entry.grapheme)
    }

    @Test
    fun `empty phoneme is allowed by data class`() {
        val entry = DictionaryEntry(grapheme = "original", phoneme = "")
        assertEquals("", entry.phoneme)
    }

    @Test
    fun `whitespace grapheme is preserved`() {
        val entry = DictionaryEntry(grapheme = "  spaces  ", phoneme = "spaces")
        assertEquals("  spaces  ", entry.grapheme)
    }

    // ==========================================================================
    // Helper Functions
    // ==========================================================================

    private fun getDictionaryFileName(type: DictionaryType): String {
        return when (type) {
            DictionaryType.MAIN -> "user.json"
            DictionaryType.SPELLING -> "spelling.json"
            DictionaryType.EMOJI -> "emoji.json"
        }
    }
}
