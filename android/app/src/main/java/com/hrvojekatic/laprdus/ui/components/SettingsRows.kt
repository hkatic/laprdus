package com.hrvojekatic.laprdus.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsDraggedAsState
import androidx.compose.foundation.interaction.collectIsPressedAsState
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.selection.selectable
import androidx.compose.foundation.selection.selectableGroup
import androidx.compose.foundation.selection.toggleable
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.RadioButton
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.Immutable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.draw.rotate
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.CustomAccessibilityAction
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.collapse
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.customActions
import androidx.compose.ui.semantics.expand
import androidx.compose.ui.semantics.heading
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import com.hrvojekatic.laprdus.R
import com.hrvojekatic.laprdus.ui.settings.SettingFormat

/*
 * The rows of the Settings screen, after Android's settings guidelines and
 * Material 3: grouped rows on rounded surfaces under a section heading.
 *
 * Every row is one TalkBack stop, and what TalkBack says follows what is
 * shown, top to bottom: the title, the summary under it, then the value or
 * state, then the kind of control ("Slova kao glasovi, Pri slovkanju i
 * tipkanju, on, switch"; "Speech rate, 1.0x, slider"). The texts of a row are merged
 * into the row's node rather than replaced by a content description, so
 * TalkBack reads them as written and with its own pauses; only a slider,
 * which is a node of its own inside the row, carries the row's title and
 * summary as its label.
 */

/** Corner radius of a group's outer corners. */
private val GroupShape = RoundedCornerShape(24.dp)

/** Corner radius between the rows of a group. */
private val ItemShape = RoundedCornerShape(4.dp)

private val RowPaddingHorizontal = 16.dp
private val RowPaddingVertical = 14.dp
private val RowMinHeight = 56.dp

/** The heading of a section, which TalkBack's heading navigation stops on. */
@Composable
fun SectionHeading(title: String, modifier: Modifier = Modifier) {
    Text(
        text = title,
        style = MaterialTheme.typography.titleSmall,
        color = MaterialTheme.colorScheme.primary,
        modifier = modifier
            .padding(start = RowPaddingHorizontal, end = RowPaddingHorizontal, top = 24.dp, bottom = 8.dp)
            .semantics { heading() },
    )
}

/** A section: a heading TalkBack can jump to, and its group of rows. */
@Composable
fun SettingsSection(
    title: String,
    modifier: Modifier = Modifier,
    content: @Composable ColumnScope.() -> Unit,
) {
    Column(modifier.fillMaxWidth()) {
        SectionHeading(title)
        Column(
            modifier = Modifier.fillMaxWidth().clip(GroupShape),
            verticalArrangement = Arrangement.spacedBy(2.dp),
            content = content,
        )
    }
}

/** The surface of one row: the group clips its outer corners. */
@Composable
private fun Modifier.rowSurface(): Modifier =
    this.fillMaxWidth()
        .clip(ItemShape)
        .background(MaterialTheme.colorScheme.surfaceContainer)

@Composable
private fun RowTexts(title: String, summary: String?, enabled: Boolean, modifier: Modifier = Modifier) {
    Column(modifier) {
        Text(
            text = title,
            style = MaterialTheme.typography.bodyLarge,
            color = if (enabled) MaterialTheme.colorScheme.onSurface
            else MaterialTheme.colorScheme.onSurface.copy(alpha = 0.38f),
        )
        if (!summary.isNullOrEmpty()) {
            Text(
                text = summary,
                style = MaterialTheme.typography.bodyMedium,
                color = if (enabled) MaterialTheme.colorScheme.onSurfaceVariant
                else MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = 0.38f),
            )
        }
    }
}

/**
 * An on/off setting: title and summary, the switch at the end; the whole
 * row toggles. TalkBack: "title, summary, on, switch".
 */
@Composable
fun SwitchRow(
    title: String,
    checked: Boolean,
    onCheckedChange: (Boolean) -> Unit,
    modifier: Modifier = Modifier,
    summary: String? = null,
    enabled: Boolean = true,
) {
    Row(
        modifier = modifier
            .rowSurface()
            .toggleable(
                value = checked,
                enabled = enabled,
                role = androidx.compose.ui.semantics.Role.Switch,
                onValueChange = onCheckedChange,
            )
            .heightIn(min = RowMinHeight)
            .padding(horizontal = RowPaddingHorizontal, vertical = RowPaddingVertical),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        RowTexts(title, summary, enabled, Modifier.weight(1f))
        Spacer(Modifier.width(16.dp))
        Switch(checked = checked, onCheckedChange = null, enabled = enabled)
    }
}

/**
 * A setting that opens a choice (voice, language) or another screen: the
 * title, the current value or a description under it.
 * TalkBack: "title, value", then its hint to activate.
 */
@Composable
fun ClickableRow(
    title: String,
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
    summary: String? = null,
    enabled: Boolean = true,
    showChevron: Boolean = false,
) {
    Row(
        modifier = modifier
            .rowSurface()
            .clickable(enabled = enabled, onClick = onClick)
            .heightIn(min = RowMinHeight)
            .padding(horizontal = RowPaddingHorizontal, vertical = RowPaddingVertical),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        RowTexts(title, summary, enabled, Modifier.weight(1f))
        if (showChevron) {
            Spacer(Modifier.width(16.dp))
            Icon(
                painter = painterResource(R.drawable.ic_chevron_right),
                contentDescription = null,
                tint = MaterialTheme.colorScheme.onSurfaceVariant,
            )
        }
    }
}

/**
 * The row that shows or hides the settings under it. Collapsed, its summary
 * names what it hides. TalkBack: "collapsed, title, summary", from the
 * expand and collapse actions.
 */
@Composable
fun ExpandRow(
    title: String,
    expanded: Boolean,
    onExpandedChange: (Boolean) -> Unit,
    modifier: Modifier = Modifier,
    collapsedSummary: String? = null,
) {
    Row(
        modifier = modifier
            .rowSurface()
            .clickable { onExpandedChange(!expanded) }
            // TalkBack says "collapsed" or "expanded" from these actions
            // itself; a state description of our own would be said twice.
            .semantics {
                if (expanded) {
                    collapse { onExpandedChange(false); true }
                } else {
                    expand { onExpandedChange(true); true }
                }
            }
            .heightIn(min = RowMinHeight)
            .padding(horizontal = RowPaddingHorizontal, vertical = RowPaddingVertical),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        RowTexts(title, if (expanded) null else collapsedSummary, true, Modifier.weight(1f))
        Spacer(Modifier.width(16.dp))
        Icon(
            painter = painterResource(R.drawable.ic_expand_more),
            contentDescription = null,
            tint = MaterialTheme.colorScheme.onSurfaceVariant,
            modifier = Modifier.rotate(if (expanded) 180f else 0f),
        )
    }
}

/**
 * A numeric setting: the title, the summary, then the slider with the value
 * at its end and a button that restores the default.
 *
 * The slider is the row's only TalkBack stop: its label is the title and the
 * summary, its state the value ("Speech rate, 1.0x, slider"), and "Restore
 * default" is one of its actions, so the visible reset button is not a stop
 * of its own. The value is only committed when the finger lifts (or a
 * TalkBack adjustment ends), so dragging does not write the settings at
 * every step. One TalkBack adjustment moves one [step].
 *
 * @param spokenValue what TalkBack says for a value, when it says more than
 *   is shown (the acceleration's top rate in words per minute)
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SliderRow(
    title: String,
    value: Float,
    onValueChange: (Float) -> Unit,
    valueRange: ClosedFloatingPointRange<Float>,
    step: Float,
    valueText: (Float) -> String,
    modifier: Modifier = Modifier,
    summary: String? = null,
    defaultValue: Float? = null,
    spokenValue: ((Float) -> String)? = null,
    note: String? = null,
    enabled: Boolean = true,
) {
    // The thumb follows the finger here; the setting changes when it lifts.
    var dragging by remember { mutableStateOf<Float?>(null) }
    val interactionSource = remember { MutableInteractionSource() }
    val shown = dragging ?: value
    val shownText = valueText(shown)
    val stateText = spokenValue?.invoke(shown) ?: shownText
    val label = if (summary.isNullOrEmpty()) title else "$title, $summary"
    val steps = SettingFormat.steps(valueRange, step)
    val canRestore = defaultValue != null && !SettingFormat.sameStep(shown, defaultValue, step)
    val restoreLabel = defaultValue?.let { stringResource(R.string.action_restore_default, valueText(it)) }

    Column(
        modifier = modifier
            .rowSurface()
            .padding(start = RowPaddingHorizontal, end = 4.dp, top = RowPaddingVertical, bottom = 4.dp),
    ) {
        // Shown, and spoken as the slider's label.
        Column(Modifier.padding(end = 12.dp).clearAndSetSemantics { }) {
            RowTexts(title, summary, enabled)
        }
        Row(verticalAlignment = Alignment.CenterVertically) {
            Slider(
                value = shown,
                onValueChange = { dragging = SettingFormat.snap(it, valueRange, step) },
                onValueChangeFinished = {
                    dragging?.let(onValueChange)
                    dragging = null
                },
                valueRange = valueRange,
                steps = steps,
                enabled = enabled,
                interactionSource = interactionSource,
                thumb = { SliderThumb(interactionSource, enabled) },
                track = { state ->
                    SliderDefaults.Track(sliderState = state, enabled = enabled, drawTick = { _, _ -> })
                },
                modifier = Modifier
                    .weight(1f)
                    .semantics {
                        contentDescription = label
                        stateDescription = stateText
                        if (canRestore && restoreLabel != null) {
                            customActions = listOf(
                                CustomAccessibilityAction(restoreLabel) {
                                    onValueChange(defaultValue)
                                    true
                                }
                            )
                        }
                    },
            )
            Text(
                text = shownText,
                style = MaterialTheme.typography.labelLarge,
                textAlign = TextAlign.End,
                modifier = Modifier
                    .padding(start = 12.dp)
                    .widthIn(min = 52.dp)
                    .clearAndSetSemantics { },
            )
            if (defaultValue != null) {
                // Reached through the slider's "restore default" action by
                // TalkBack and Switch Access; a button for touch.
                IconButton(
                    onClick = { onValueChange(defaultValue) },
                    enabled = enabled && canRestore,
                    modifier = Modifier.clearAndSetSemantics { },
                ) {
                    Icon(painter = painterResource(R.drawable.ic_restart_alt), contentDescription = null)
                }
            } else {
                Spacer(Modifier.width(12.dp))
            }
        }
        if (!note.isNullOrEmpty()) {
            Text(
                text = note,
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
                modifier = Modifier.padding(end = 12.dp, bottom = 10.dp).clearAndSetSemantics { },
            )
        }
    }
}

/**
 * The slider's thumb, Material 3's narrow handle, drawn rather than given a
 * background: since Compose 1.12 a shaped background is a semantics node,
 * which made the thumb a child of the slider, and Compose then moves the
 * slider's label onto a child node that TalkBack does not read on a slider
 * ("1.0x, slider" without the name). Narrower while pressed or dragged, as
 * Material's own.
 */
@Composable
private fun SliderThumb(interactionSource: MutableInteractionSource, enabled: Boolean) {
    val pressed by interactionSource.collectIsPressedAsState()
    val dragged by interactionSource.collectIsDraggedAsState()
    val color = if (enabled) MaterialTheme.colorScheme.primary
    else MaterialTheme.colorScheme.onSurface.copy(alpha = 0.38f)
    Spacer(
        Modifier
            .size(width = if (pressed || dragged) 2.dp else 4.dp, height = 44.dp)
            .drawBehind {
                drawRoundRect(color = color, cornerRadius = CornerRadius(size.width / 2, size.width / 2))
            }
    )
}

/** One option of a [ChoiceDialog]. */
@Immutable
data class ChoiceOption(val id: String, val title: String, val summary: String? = null)

/**
 * A single choice in a dialog with radio buttons, as Android's list
 * settings do; choosing closes it. TalkBack reads each option with its
 * summary and whether it is selected.
 */
@Composable
fun ChoiceDialog(
    title: String,
    options: List<ChoiceOption>,
    selectedId: String?,
    onSelect: (String) -> Unit,
    onDismiss: () -> Unit,
) {
    val selectedIndex = options.indexOfFirst { it.id == selectedId }.coerceAtLeast(0)
    val listState = rememberLazyListState(initialFirstVisibleItemIndex = selectedIndex)
    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text(title) },
        text = {
            LazyColumn(
                state = listState,
                modifier = Modifier.selectableGroup().testTag("settings_choice_list"),
            ) {
                itemsIndexed(options, key = { _, option -> option.id }) { _, option ->
                    val selected = option.id == selectedId
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .clip(RoundedCornerShape(12.dp))
                            .selectable(
                                selected = selected,
                                role = androidx.compose.ui.semantics.Role.RadioButton,
                                onClick = { onSelect(option.id) },
                            )
                            .heightIn(min = RowMinHeight)
                            .padding(horizontal = 8.dp, vertical = 8.dp),
                        verticalAlignment = Alignment.CenterVertically,
                    ) {
                        RadioButton(selected = selected, onClick = null)
                        Spacer(Modifier.width(16.dp))
                        RowTexts(option.title, option.summary, true, Modifier.weight(1f))
                    }
                }
            }
        },
        confirmButton = {},
        dismissButton = {
            TextButton(onClick = onDismiss) { Text(stringResource(R.string.cancel)) }
        },
    )
}
