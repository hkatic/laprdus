package com.hrvojekatic.laprdus.viewmodel

import com.hrvojekatic.laprdus.data.DictionaryEntry
import com.hrvojekatic.laprdus.data.DictionaryRepository
import com.hrvojekatic.laprdus.data.DictionaryType
import io.mockk.coEvery
import io.mockk.coVerify
import io.mockk.mockk
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.test.StandardTestDispatcher
import kotlinx.coroutines.test.advanceUntilIdle
import kotlinx.coroutines.test.resetMain
import kotlinx.coroutines.test.runTest
import kotlinx.coroutines.test.setMain
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test

/**
 * Unit tests for DictionaryViewModel.
 * Tests cover state management, CRUD operations, and error handling.
 */
@OptIn(ExperimentalCoroutinesApi::class)
class DictionaryViewModelTest {

    private val testDispatcher = StandardTestDispatcher()
    private lateinit var mockRepository: DictionaryRepository
    private lateinit var viewModel: DictionaryViewModel

    @Before
    fun setup() {
        Dispatchers.setMain(testDispatcher)
        mockRepository = mockk(relaxed = true)
    }

    @After
    fun cleanup() {
        Dispatchers.resetMain()
    }

    // ==========================================================================
    // Initial State Tests
    // ==========================================================================

    @Test
    fun `initial state has correct defaults`() = runTest {
        coEvery { mockRepository.loadDictionary(any()) } returns Result.success(emptyList())
        viewModel = DictionaryViewModel(mockRepository)
        advanceUntilIdle()

        val state = viewModel.uiState.value

        assertFalse(state.isLoading)
        assertEquals(DictionaryType.MAIN, state.selectedType)
        assertTrue(state.entries.isEmpty())
        assertNull(state.editingEntry)
        assertNull(state.error)
    }

    // ==========================================================================
    // Load Dictionary Tests
    // ==========================================================================

    @Test
    fun `loadDictionary updates entries on success`() = runTest {
        val testEntries = listOf(
            DictionaryEntry(id = "1", grapheme = "test", phoneme = "tst"),
            DictionaryEntry(id = "2", grapheme = "hello", phoneme = "helo")
        )
        coEvery { mockRepository.loadDictionary(DictionaryType.MAIN) } returns Result.success(testEntries)

        viewModel = DictionaryViewModel(mockRepository)
        advanceUntilIdle()

        val state = viewModel.uiState.value
        assertEquals(2, state.entries.size)
        assertEquals("test", state.entries[0].grapheme)
    }

    @Test
    fun `loadDictionary sets error on failure`() = runTest {
        coEvery { mockRepository.loadDictionary(any()) } returns Result.failure(Exception("Load failed"))

        viewModel = DictionaryViewModel(mockRepository)
        advanceUntilIdle()

        val state = viewModel.uiState.value
        assertTrue(state.entries.isEmpty())
        // Error should be set
        assertEquals("Load failed", state.error)
    }

    @Test
    fun `loadDictionary changes type correctly`() = runTest {
        coEvery { mockRepository.loadDictionary(DictionaryType.MAIN) } returns Result.success(emptyList())
        coEvery { mockRepository.loadDictionary(DictionaryType.SPELLING) } returns Result.success(
            listOf(DictionaryEntry(id = "spell-1", grapheme = "A", phoneme = "a"))
        )

        viewModel = DictionaryViewModel(mockRepository)
        advanceUntilIdle()

        viewModel.loadDictionary(DictionaryType.SPELLING)
        advanceUntilIdle()

        val state = viewModel.uiState.value
        assertEquals(DictionaryType.SPELLING, state.selectedType)
        assertEquals(1, state.entries.size)
    }

    // ==========================================================================
    // Save Entry Tests
    // ==========================================================================

    @Test
    fun `saveEntry adds new entry to state`() = runTest {
        coEvery { mockRepository.loadDictionary(any()) } returns Result.success(emptyList())
        coEvery { mockRepository.saveEntry(any()) } returns Result.success(Unit)

        viewModel = DictionaryViewModel(mockRepository)
        advanceUntilIdle()

        val newEntry = DictionaryEntry(
            id = "new-1",
            grapheme = "new",
            phoneme = "nju"
        )
        viewModel.saveEntry(newEntry)
        advanceUntilIdle()

        coVerify { mockRepository.saveEntry(newEntry) }
    }

    @Test
    fun `saveEntry updates existing entry in state`() = runTest {
        val existingEntry = DictionaryEntry(id = "exist-1", grapheme = "old", phoneme = "old")
        coEvery { mockRepository.loadDictionary(any()) } returns Result.success(listOf(existingEntry))
        coEvery { mockRepository.saveEntry(any()) } returns Result.success(Unit)

        viewModel = DictionaryViewModel(mockRepository)
        advanceUntilIdle()

        val updatedEntry = existingEntry.copy(phoneme = "new")
        viewModel.saveEntry(updatedEntry)
        advanceUntilIdle()

        coVerify { mockRepository.saveEntry(updatedEntry) }
    }

    // ==========================================================================
    // Delete Entry Tests
    // ==========================================================================

    @Test
    fun `deleteEntry removes entry from state`() = runTest {
        val entry = DictionaryEntry(id = "del-1", grapheme = "delete", phoneme = "del")
        coEvery { mockRepository.loadDictionary(any()) } returns Result.success(listOf(entry))
        coEvery { mockRepository.deleteEntry(any()) } returns Result.success(Unit)

        viewModel = DictionaryViewModel(mockRepository)
        advanceUntilIdle()

        viewModel.deleteEntry("del-1")
        advanceUntilIdle()

        coVerify { mockRepository.deleteEntry("del-1") }
    }

    // ==========================================================================
    // Duplicate Entry Tests
    // ==========================================================================

    @Test
    fun `duplicateEntry creates copy with modified grapheme`() = runTest {
        val original = DictionaryEntry(
            id = "orig-1",
            grapheme = "original",
            phoneme = "orig",
            comment = "test comment",
            caseSensitive = true,
            wholeWord = false
        )
        coEvery { mockRepository.loadDictionary(any()) } returns Result.success(listOf(original))
        coEvery { mockRepository.saveEntry(any()) } returns Result.success(Unit)

        viewModel = DictionaryViewModel(mockRepository)
        advanceUntilIdle()

        viewModel.duplicateEntry(original)
        advanceUntilIdle()

        // Should save a new entry with modified grapheme
        coVerify {
            mockRepository.saveEntry(
                match { entry ->
                    entry.grapheme.contains("original") &&
                    entry.grapheme.contains("(") &&
                    entry.phoneme == "orig" &&
                    entry.comment == "test comment" &&
                    entry.caseSensitive &&
                    !entry.wholeWord &&
                    entry.id != "orig-1"
                }
            )
        }
    }

    // ==========================================================================
    // Editing Entry State Tests
    // ==========================================================================

    @Test
    fun `setEditingEntry updates editing state`() = runTest {
        coEvery { mockRepository.loadDictionary(any()) } returns Result.success(emptyList())

        viewModel = DictionaryViewModel(mockRepository)
        advanceUntilIdle()

        val entry = DictionaryEntry(id = "edit-1", grapheme = "edit", phoneme = "edt")
        viewModel.setEditingEntry(entry)

        assertEquals(entry, viewModel.uiState.value.editingEntry)
    }

    @Test
    fun `setEditingEntry null clears editing state`() = runTest {
        coEvery { mockRepository.loadDictionary(any()) } returns Result.success(emptyList())

        viewModel = DictionaryViewModel(mockRepository)
        advanceUntilIdle()

        val entry = DictionaryEntry(id = "edit-1", grapheme = "edit", phoneme = "edt")
        viewModel.setEditingEntry(entry)
        viewModel.setEditingEntry(null)

        assertNull(viewModel.uiState.value.editingEntry)
    }

    // ==========================================================================
    // Error Handling Tests
    // ==========================================================================

    @Test
    fun `clearError removes error from state`() = runTest {
        coEvery { mockRepository.loadDictionary(any()) } returns Result.failure(Exception("Error"))

        viewModel = DictionaryViewModel(mockRepository)
        advanceUntilIdle()

        assertEquals("Error", viewModel.uiState.value.error)

        viewModel.clearError()

        assertNull(viewModel.uiState.value.error)
    }

    @Test
    fun `saveEntry error sets error state`() = runTest {
        coEvery { mockRepository.loadDictionary(any()) } returns Result.success(emptyList())
        coEvery { mockRepository.saveEntry(any()) } returns Result.failure(Exception("Save failed"))

        viewModel = DictionaryViewModel(mockRepository)
        advanceUntilIdle()

        val entry = DictionaryEntry(id = "err-1", grapheme = "error", phoneme = "err")
        viewModel.saveEntry(entry)
        advanceUntilIdle()

        assertEquals("Save failed", viewModel.uiState.value.error)
    }

    @Test
    fun `deleteEntry error sets error state`() = runTest {
        val entry = DictionaryEntry(id = "del-err-1", grapheme = "delete", phoneme = "del")
        coEvery { mockRepository.loadDictionary(any()) } returns Result.success(listOf(entry))
        coEvery { mockRepository.deleteEntry(any()) } returns Result.failure(Exception("Delete failed"))

        viewModel = DictionaryViewModel(mockRepository)
        advanceUntilIdle()

        viewModel.deleteEntry("del-err-1")
        advanceUntilIdle()

        assertEquals("Delete failed", viewModel.uiState.value.error)
    }
}
