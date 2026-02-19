package com.hrvojekatic.laprdus.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.ContentCopy
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Checkbox
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.foundation.selection.toggleable
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.isTraversalGroup
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.heading
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.dp
import com.hrvojekatic.laprdus.R
import com.hrvojekatic.laprdus.data.DictionaryEntry
import java.util.UUID

/**
 * Screen for adding or editing a dictionary entry.
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun DictionaryEditScreen(
    entry: DictionaryEntry?,
    onNavigateBack: () -> Unit,
    onSave: (DictionaryEntry) -> Unit,
    onDelete: (String) -> Unit,
    onDuplicate: (DictionaryEntry) -> Unit
) {
    val isEdit = entry != null

    // Form state
    var grapheme by remember { mutableStateOf(entry?.grapheme ?: "") }
    var phoneme by remember { mutableStateOf(entry?.phoneme ?: "") }
    var comment by remember { mutableStateOf(entry?.comment ?: "") }
    var caseSensitive by remember { mutableStateOf(entry?.caseSensitive ?: false) }
    var wholeWord by remember { mutableStateOf(entry?.wholeWord ?: true) }

    // Validation state
    var graphemeError by remember { mutableStateOf(false) }
    var phonemeError by remember { mutableStateOf(false) }

    // Delete confirmation dialog
    var showDeleteDialog by remember { mutableStateOf(false) }

    val title = if (isEdit) {
        stringResource(R.string.dict_entry_title_edit)
    } else {
        stringResource(R.string.dict_entry_title_add)
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = {
                    Text(
                        text = title,
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
                },
                actions = {
                    if (isEdit && entry != null) {
                        IconButton(
                            onClick = {
                                onDuplicate(entry)
                                onNavigateBack()
                            }
                        ) {
                            Icon(
                                imageVector = Icons.Default.ContentCopy,
                                contentDescription = stringResource(R.string.dict_duplicate)
                            )
                        }
                        IconButton(onClick = { showDeleteDialog = true }) {
                            Icon(
                                imageVector = Icons.Default.Delete,
                                contentDescription = stringResource(R.string.dict_delete)
                            )
                        }
                    }
                }
            )
        }
    ) { paddingValues ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
                .padding(16.dp)
                .verticalScroll(rememberScrollState()),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            // Original string field
            OutlinedTextField(
                value = grapheme,
                onValueChange = {
                    grapheme = it
                    graphemeError = false
                },
                label = { Text(stringResource(R.string.dict_entry_original)) },
                isError = graphemeError,
                supportingText = if (graphemeError) {
                    { Text(stringResource(R.string.dict_error_empty_fields)) }
                } else null,
                singleLine = true,
                modifier = Modifier.fillMaxWidth()
            )

            // Replacement string field
            OutlinedTextField(
                value = phoneme,
                onValueChange = {
                    phoneme = it
                    phonemeError = false
                },
                label = { Text(stringResource(R.string.dict_entry_replacement)) },
                isError = phonemeError,
                supportingText = if (phonemeError) {
                    { Text(stringResource(R.string.dict_error_empty_fields)) }
                } else null,
                singleLine = true,
                modifier = Modifier.fillMaxWidth()
            )

            // Comment field
            OutlinedTextField(
                value = comment,
                onValueChange = { comment = it },
                label = { Text(stringResource(R.string.dict_entry_comment)) },
                singleLine = false,
                maxLines = 3,
                modifier = Modifier.fillMaxWidth()
            )

            Spacer(modifier = Modifier.height(8.dp))

            // Case sensitive checkbox
            val caseSensitiveLabel = stringResource(R.string.dict_entry_case_sensitive)
            Row(
                verticalAlignment = Alignment.CenterVertically,
                modifier = Modifier
                    .fillMaxWidth()
                    .semantics(mergeDescendants = true) {
                        isTraversalGroup = true
                        contentDescription = caseSensitiveLabel
                    }
                    .toggleable(
                        value = caseSensitive,
                        onValueChange = { caseSensitive = it },
                        role = Role.Checkbox
                    )
            ) {
                Checkbox(
                    checked = caseSensitive,
                    onCheckedChange = null,
                    modifier = Modifier.clearAndSetSemantics { }
                )
                Text(
                    text = caseSensitiveLabel,
                    style = MaterialTheme.typography.bodyLarge,
                    modifier = Modifier.clearAndSetSemantics { }
                )
            }

            // Whole word checkbox
            val wholeWordLabel = stringResource(R.string.dict_entry_whole_word)
            Row(
                verticalAlignment = Alignment.CenterVertically,
                modifier = Modifier
                    .fillMaxWidth()
                    .semantics(mergeDescendants = true) {
                        isTraversalGroup = true
                        contentDescription = wholeWordLabel
                    }
                    .toggleable(
                        value = wholeWord,
                        onValueChange = { wholeWord = it },
                        role = Role.Checkbox
                    )
            ) {
                Checkbox(
                    checked = wholeWord,
                    onCheckedChange = null,
                    modifier = Modifier.clearAndSetSemantics { }
                )
                Text(
                    text = wholeWordLabel,
                    style = MaterialTheme.typography.bodyLarge,
                    modifier = Modifier.clearAndSetSemantics { }
                )
            }

            Spacer(modifier = Modifier.weight(1f))

            // Save button
            Button(
                onClick = {
                    // Validate
                    graphemeError = grapheme.isBlank()
                    phonemeError = phoneme.isBlank()

                    if (!graphemeError && !phonemeError) {
                        val newEntry = DictionaryEntry(
                            id = entry?.id ?: UUID.randomUUID().toString(),
                            grapheme = grapheme.trim(),
                            phoneme = phoneme.trim(),
                            caseSensitive = caseSensitive,
                            wholeWord = wholeWord,
                            comment = comment.trim()
                        )
                        onSave(newEntry)
                        onNavigateBack()
                    }
                },
                modifier = Modifier.fillMaxWidth()
            ) {
                Text(stringResource(R.string.dict_save))
            }
        }
    }

    // Delete confirmation dialog
    if (showDeleteDialog && entry != null) {
        AlertDialog(
            onDismissRequest = { showDeleteDialog = false },
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
                        showDeleteDialog = false
                        onDelete(entry.id)
                        onNavigateBack()
                    }
                ) {
                    Text(stringResource(R.string.dict_delete))
                }
            },
            dismissButton = {
                TextButton(onClick = { showDeleteDialog = false }) {
                    Text(stringResource(R.string.cancel))
                }
            }
        )
    }
}
