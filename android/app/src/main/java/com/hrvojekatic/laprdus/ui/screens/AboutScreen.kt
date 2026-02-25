package com.hrvojekatic.laprdus.ui.screens

import android.content.Intent
import android.net.Uri
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.ChevronRight
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.heading
import androidx.compose.ui.semantics.isTraversalGroup
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.unit.dp
import com.hrvojekatic.laprdus.BuildConfig
import com.hrvojekatic.laprdus.R

/**
 * About screen for the Laprdus TTS application.
 * Shows application information, legal links, and support contact.
 *
 * Accessibility: Uses the same patterns as SettingsScreen — merged semantics,
 * heading() on section titles, Role.Button on interactive items.
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun AboutScreen(
    onNavigateBack: () -> Unit
) {
    val context = LocalContext.current
    val backButtonDescription = stringResource(R.string.cd_back_button)

    Scaffold(
        topBar = {
            TopAppBar(
                title = {
                    Text(
                        text = stringResource(R.string.about_title),
                        modifier = Modifier.semantics { heading() }
                    )
                },
                navigationIcon = {
                    IconButton(
                        onClick = onNavigateBack,
                        modifier = Modifier.semantics {
                            contentDescription = backButtonDescription
                        }
                    ) {
                        Icon(
                            imageVector = Icons.AutoMirrored.Filled.ArrowBack,
                            contentDescription = null
                        )
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = MaterialTheme.colorScheme.primaryContainer,
                    titleContentColor = MaterialTheme.colorScheme.onPrimaryContainer
                )
            )
        }
    ) { paddingValues ->
        LazyColumn(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
                .padding(horizontal = 16.dp),
            verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            // Application Information section
            item {
                Spacer(modifier = Modifier.height(8.dp))
                SettingsCategoryHeader(title = stringResource(R.string.about_heading_app_info))
            }

            item {
                AboutTextItem(text = stringResource(R.string.about_app_name))
            }

            item {
                AboutTextItem(
                    text = stringResource(
                        R.string.about_version,
                        BuildConfig.VERSION_NAME,
                        BuildConfig.VERSION_CODE
                    )
                )
            }

            item {
                AboutTextItem(text = stringResource(R.string.about_copyright))
            }

            // Legal section
            item {
                Spacer(modifier = Modifier.height(16.dp))
                SettingsCategoryHeader(title = stringResource(R.string.about_heading_legal))
            }

            item {
                AboutLinkItem(
                    title = stringResource(R.string.about_privacy_policy),
                    onClick = {
                        context.startActivity(
                            Intent(
                                Intent.ACTION_VIEW,
                                Uri.parse("https://hrvojekatic.com/laprdus/privacy-statement.php")
                            )
                        )
                    }
                )
            }

            item {
                AboutLinkItem(
                    title = stringResource(R.string.about_license),
                    onClick = {
                        context.startActivity(
                            Intent(
                                Intent.ACTION_VIEW,
                                Uri.parse("https://www.gnu.org/licenses/gpl-3.0.en.html")
                            )
                        )
                    }
                )
            }

            // Support section
            item {
                Spacer(modifier = Modifier.height(16.dp))
                SettingsCategoryHeader(title = stringResource(R.string.about_heading_support))
            }

            item {
                AboutLinkItem(
                    title = stringResource(R.string.about_contact_email),
                    onClick = {
                        context.startActivity(
                            Intent(
                                Intent.ACTION_SENDTO,
                                Uri.parse("mailto:hrvojekatic@gmail.com")
                            )
                        )
                    }
                )
            }

            // Bottom padding
            item {
                Spacer(modifier = Modifier.height(24.dp))
            }
        }
    }
}

/**
 * Static text item for the About screen (non-interactive).
 * Displayed as a single TalkBack item.
 */
@Composable
private fun AboutTextItem(text: String) {
    Text(
        text = text,
        style = MaterialTheme.typography.bodyLarge,
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 4.dp)
    )
}

/**
 * Clickable link item for the About screen.
 *
 * Accessibility for TalkBack:
 * - Uses Role.Button for native role announcement
 * - Uses isTraversalGroup to ensure proper navigation order
 * - TalkBack: "Title, Button, Double-tap to activate"
 */
@Composable
private fun AboutLinkItem(
    title: String,
    onClick: () -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 12.dp)
            .semantics(mergeDescendants = true) {
                isTraversalGroup = true
                contentDescription = title
            }
            .clickable(
                role = Role.Button,
                onClick = onClick
            ),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(
            text = title,
            style = MaterialTheme.typography.bodyLarge,
            modifier = Modifier
                .weight(1f)
                .clearAndSetSemantics { }
        )
        Icon(
            imageVector = Icons.Default.ChevronRight,
            contentDescription = null,
            tint = MaterialTheme.colorScheme.onSurfaceVariant,
            modifier = Modifier.clearAndSetSemantics { }
        )
    }
}
