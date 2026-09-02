package com.hrvojekatic.laprdus.viewmodel

import android.content.Context
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.hrvojekatic.laprdus.R
import com.hrvojekatic.laprdus.data.DictionaryEntry
import com.hrvojekatic.laprdus.data.DictionaryRepository
import com.hrvojekatic.laprdus.data.DictionaryType
import dagger.hilt.android.lifecycle.HiltViewModel
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import java.util.UUID
import javax.inject.Inject

/**
 * UI state for the dictionary management screens.
 */
data class DictionaryUiState(
    /** Whether data is currently loading */
    val isLoading: Boolean = true,
    /** List of entries in the current dictionary */
    val entries: List<DictionaryEntry> = emptyList(),
    /** Currently selected dictionary type */
    val selectedType: DictionaryType = DictionaryType.MAIN,
    /** Entry currently being edited (null for new entry) */
    val editingEntry: DictionaryEntry? = null,
    /** Error message to display */
    val error: String? = null
)

/**
 * ViewModel for dictionary management screens.
 * Handles loading, saving, and manipulating dictionary entries.
 */
@HiltViewModel
class DictionaryViewModel @Inject constructor(
    @param:ApplicationContext private val context: Context,
    private val repository: DictionaryRepository
) : ViewModel() {

    private val _uiState = MutableStateFlow(DictionaryUiState())
    val uiState: StateFlow<DictionaryUiState> = _uiState.asStateFlow()

    init {
        loadDictionary(DictionaryType.MAIN)
        observeEntries()
        observeStorageErrors()
    }

    /**
     * Surface a failed migration of dictionaries saved by an older version
     * (the repository logs the technical details).
     */
    private fun observeStorageErrors() {
        viewModelScope.launch {
            repository.storageError.collect { message ->
                if (message != null) {
                    _uiState.update { it.copy(error = context.getString(R.string.error_dictionary_migration)) }
                }
            }
        }
    }

    /**
     * Observe changes to the repository's entries.
     */
    private fun observeEntries() {
        viewModelScope.launch {
            repository.entries.collect { entries ->
                _uiState.update { it.copy(entries = entries, isLoading = false) }
            }
        }
    }

    /**
     * Load a dictionary by type.
     * @param type The type of dictionary to load
     */
    fun loadDictionary(type: DictionaryType) {
        _uiState.update { it.copy(isLoading = true, selectedType = type, editingEntry = null) }
        viewModelScope.launch {
            repository.loadDictionary(type)
                .onSuccess { entries ->
                    _uiState.update { it.copy(entries = entries, isLoading = false) }
                }
                .onFailure { e ->
                    _uiState.update { it.copy(error = e.message, isLoading = false) }
                }
        }
    }

    /**
     * Save an entry to the current dictionary.
     * @param entry The entry to save
     */
    fun saveEntry(entry: DictionaryEntry) {
        viewModelScope.launch {
            repository.saveEntry(entry)
                .onFailure { e ->
                    _uiState.update { it.copy(error = e.message) }
                }
        }
    }

    /**
     * Delete an entry from the current dictionary.
     * @param entryId The ID of the entry to delete
     */
    fun deleteEntry(entryId: String) {
        viewModelScope.launch {
            repository.deleteEntry(entryId)
                .onFailure { e ->
                    _uiState.update { it.copy(error = e.message) }
                }
        }
    }

    /**
     * Duplicate an entry.
     * Creates a copy of the entry with a new ID and modified grapheme.
     * @param entry The entry to duplicate
     */
    fun duplicateEntry(entry: DictionaryEntry) {
        val duplicate = entry.copy(
            id = UUID.randomUUID().toString(),
            grapheme = entry.grapheme + " (copy)"
        )
        saveEntry(duplicate)
    }

    /**
     * Set the entry being edited.
     * @param entry The entry to edit, or null for a new entry
     */
    fun setEditingEntry(entry: DictionaryEntry?) {
        _uiState.update { it.copy(editingEntry = entry) }
    }

    /**
     * Clear any error message.
     */
    fun clearError() {
        _uiState.update { it.copy(error = null) }
    }
}
