package com.hrvojekatic.laprdus

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.hrvojekatic.laprdus.ui.screens.DictionaryEditScreen
import com.hrvojekatic.laprdus.ui.screens.DictionaryListScreen
import com.hrvojekatic.laprdus.ui.theme.LaprdusTheme
import com.hrvojekatic.laprdus.viewmodel.DictionaryViewModel
import dagger.hilt.android.AndroidEntryPoint

/**
 * Activity for managing user dictionaries.
 * Provides screens for viewing, adding, editing, and deleting dictionary entries.
 */
@AndroidEntryPoint
class DictionaryActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        setContent {
            LaprdusTheme {
                val navController = rememberNavController()
                val viewModel: DictionaryViewModel = hiltViewModel()
                val uiState by viewModel.uiState.collectAsState()

                NavHost(
                    navController = navController,
                    startDestination = "list"
                ) {
                    composable("list") {
                        DictionaryListScreen(
                            uiState = uiState,
                            onNavigateBack = { finish() },
                            onTypeSelected = viewModel::loadDictionary,
                            onEntryClick = { entry ->
                                viewModel.setEditingEntry(entry)
                                navController.navigate("edit")
                            },
                            onAddEntry = {
                                viewModel.setEditingEntry(null)
                                navController.navigate("edit")
                            },
                            onClearError = viewModel::clearError
                        )
                    }
                    composable("edit") {
                        DictionaryEditScreen(
                            entry = uiState.editingEntry,
                            onNavigateBack = { navController.popBackStack() },
                            onSave = { entry ->
                                viewModel.saveEntry(entry)
                            },
                            onDelete = { entryId ->
                                viewModel.deleteEntry(entryId)
                            },
                            onDuplicate = { entry ->
                                viewModel.duplicateEntry(entry)
                            }
                        )
                    }
                }
            }
        }
    }
}
