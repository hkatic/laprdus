package com.hrvojekatic.laprdus.ui.screens

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.Add
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults
import androidx.compose.material3.FloatingActionButton
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.ListItem
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.MenuAnchorType
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.heading
import androidx.compose.ui.semantics.isTraversalGroup
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.unit.dp
import com.hrvojekatic.laprdus.R
import com.hrvojekatic.laprdus.data.DictionaryEntry
import com.hrvojekatic.laprdus.data.DictionaryType
import com.hrvojekatic.laprdus.viewmodel.DictionaryUiState

/**
 * Screen for listing and managing dictionary entries.
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun DictionaryListScreen(
    uiState: DictionaryUiState,
    onNavigateBack: () -> Unit,
    onTypeSelected: (DictionaryType) -> Unit,
    onEntryClick: (DictionaryEntry) -> Unit,
    onAddEntry: () -> Unit,
    onDeleteEntry: (String) -> Unit,
    onDuplicateEntry: (DictionaryEntry) -> Unit,
    onClearError: () -> Unit
) {
    val snackbarHostState = remember { SnackbarHostState() }

    // Action menu state
    var menuEntry by remember { mutableStateOf<DictionaryEntry?>(null) }
    var showDeleteConfirm by remember { mutableStateOf(false) }

    // Show error in snackbar
    LaunchedEffect(uiState.error) {
        uiState.error?.let { error ->
            snackbarHostState.showSnackbar(error)
            onClearError()
        }
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = {
                    Text(
                        text = stringResource(R.string.dict_screen_title),
                        modifier = Modifier.semantics { heading() }
                    )
                },
                navigationIcon = {
                    IconButton(onClick = onNavigateBack) {
                        Icon(
                            imageVector = Icons.AutoMirrored.Filled.ArrowBack,
                            contentDescription = stringResource(R.string.cd_back_button)
                        )
                    }
                }
            )
        },
        floatingActionButton = {
            val addEntryDesc = stringResource(R.string.dict_add_entry)
            FloatingActionButton(
                onClick = onAddEntry,
                modifier = Modifier.semantics {
                    contentDescription = addEntryDesc
                }
            ) {
                Icon(
                    imageVector = Icons.Default.Add,
                    contentDescription = null
                )
            }
        },
        snackbarHost = { SnackbarHost(snackbarHostState) }
    ) { paddingValues ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
        ) {
            // Dictionary type selector
            DictionaryTypeSelector(
                selectedType = uiState.selectedType,
                onTypeSelected = onTypeSelected
            )

            when {
                uiState.isLoading -> {
                    Box(
                        modifier = Modifier.fillMaxSize(),
                        contentAlignment = Alignment.Center
                    ) {
                        CircularProgressIndicator()
                    }
                }
                uiState.entries.isEmpty() -> {
                    Box(
                        modifier = Modifier.fillMaxSize(),
                        contentAlignment = Alignment.Center
                    ) {
                        Text(
                            text = stringResource(R.string.dict_empty_list),
                            style = MaterialTheme.typography.bodyLarge,
                            color = MaterialTheme.colorScheme.onSurfaceVariant
                        )
                    }
                }
                else -> {
                    LazyColumn(
                        modifier = Modifier.fillMaxSize()
                    ) {
                        items(
                            items = uiState.entries,
                            key = { it.id }
                        ) { entry ->
                            DictionaryEntryItem(
                                entry = entry,
                                onClick = { menuEntry = entry }
                            )
                        }
                    }
                }
            }
        }
    }

    // Action menu dialog
    menuEntry?.let { entry ->
        if (!showDeleteConfirm) {
            AlertDialog(
                onDismissRequest = { menuEntry = null },
                title = {
                    Text(
                        text = entry.grapheme,
                        modifier = Modifier.semantics { heading() }
                    )
                },
                text = {
                    Column {
                        TextButton(
                            onClick = {
                                menuEntry = null
                                onEntryClick(entry)
                            },
                            modifier = Modifier.fillMaxWidth()
                        ) {
                            Text(stringResource(R.string.dict_action_edit))
                        }
                        TextButton(
                            onClick = {
                                menuEntry = null
                                onDuplicateEntry(entry)
                            },
                            modifier = Modifier.fillMaxWidth()
                        ) {
                            Text(stringResource(R.string.dict_action_duplicate))
                        }
                        TextButton(
                            onClick = { showDeleteConfirm = true },
                            modifier = Modifier.fillMaxWidth()
                        ) {
                            Text(
                                text = stringResource(R.string.dict_action_delete),
                                color = MaterialTheme.colorScheme.error
                            )
                        }
                    }
                },
                confirmButton = {}
            )
        } else {
            // Delete confirmation dialog
            AlertDialog(
                onDismissRequest = {
                    showDeleteConfirm = false
                    menuEntry = null
                },
                title = {
                    Text(
                        text = stringResource(R.string.dict_delete_confirm_title),
                        modifier = Modifier.semantics { heading() }
                    )
                },
                text = { Text(stringResource(R.string.dict_delete_confirm_message)) },
                confirmButton = {
                    TextButton(
                        onClick = {
                            showDeleteConfirm = false
                            val entryId = entry.id
                            menuEntry = null
                            onDeleteEntry(entryId)
                        }
                    ) {
                        Text(stringResource(R.string.dict_delete))
                    }
                },
                dismissButton = {
                    TextButton(
                        onClick = {
                            showDeleteConfirm = false
                            menuEntry = null
                        }
                    ) {
                        Text(stringResource(R.string.cancel))
                    }
                }
            )
        }
    }
}

/**
 * Dropdown for selecting dictionary type.
 *
 * Accessibility: Uses Role.DropdownList for native TalkBack role announcement.
 * Parent Row merges descendants and provides contentDescription + stateDescription.
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun DictionaryTypeSelector(
    selectedType: DictionaryType,
    onTypeSelected: (DictionaryType) -> Unit
) {
    var expanded by remember { mutableStateOf(false) }

    val typeLabel = when (selectedType) {
        DictionaryType.MAIN -> stringResource(R.string.dict_type_main)
        DictionaryType.SPELLING -> stringResource(R.string.dict_type_spelling)
        DictionaryType.EMOJI -> stringResource(R.string.dict_type_emoji)
    }
    val label = stringResource(R.string.dict_type_label)

    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(16.dp)
            .semantics(mergeDescendants = true) {
                isTraversalGroup = true
                contentDescription = label
                stateDescription = typeLabel
            }
            .clickable(role = Role.DropdownList) { expanded = true },
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(
            text = label,
            style = MaterialTheme.typography.bodyLarge,
            modifier = Modifier.clearAndSetSemantics {}
        )
        Spacer(modifier = Modifier.width(8.dp))
        ExposedDropdownMenuBox(
            expanded = expanded,
            onExpandedChange = { expanded = it },
            modifier = Modifier
                .weight(1f)
                .clearAndSetSemantics {}
        ) {
            OutlinedTextField(
                value = typeLabel,
                onValueChange = {},
                readOnly = true,
                singleLine = true,
                trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = expanded) },
                modifier = Modifier
                    .fillMaxWidth()
                    .menuAnchor(MenuAnchorType.PrimaryNotEditable)
                    .clearAndSetSemantics {}
            )
            ExposedDropdownMenu(
                expanded = expanded,
                onDismissRequest = { expanded = false }
            ) {
                DropdownMenuItem(
                    text = { Text(stringResource(R.string.dict_type_main)) },
                    onClick = {
                        onTypeSelected(DictionaryType.MAIN)
                        expanded = false
                    }
                )
                DropdownMenuItem(
                    text = { Text(stringResource(R.string.dict_type_spelling)) },
                    onClick = {
                        onTypeSelected(DictionaryType.SPELLING)
                        expanded = false
                    }
                )
                DropdownMenuItem(
                    text = { Text(stringResource(R.string.dict_type_emoji)) },
                    onClick = {
                        onTypeSelected(DictionaryType.EMOJI)
                        expanded = false
                    }
                )
            }
        }
    }
}

/**
 * List item for a dictionary entry.
 */
@Composable
private fun DictionaryEntryItem(
    entry: DictionaryEntry,
    onClick: () -> Unit
) {
    val entryDescription = "${entry.grapheme}, ${entry.phoneme}"
    val actionLabel = stringResource(R.string.dict_action_edit)

    ListItem(
        headlineContent = { Text(entry.grapheme) },
        supportingContent = { Text(entry.phoneme) },
        modifier = Modifier
            .clickable(onClickLabel = actionLabel, onClick = onClick)
            .semantics { contentDescription = entryDescription }
    )
    HorizontalDivider()
}
