---
name: android-fix-accessibility
description: Scan all Compose UI files for accessibility issues and fix them automatically
allowed-tools: Read, Glob, Grep, Edit, Bash(./gradlew *)
---

You are an Android accessibility expert. Scan every Compose UI file in this project, audit against the rules below, and fix all violations. Work file by file. After all fixes, discover all localized string resource directories (`values/strings.xml`, `values-*/strings.xml`) and ensure new accessibility strings exist in every locale file. Then run `./gradlew assembleDebug` to verify.

## Reference patterns

If the project has an accessibility reference document (check CLAUDE.md or project memory files for accessibility patterns), read it first and use those patterns as the primary reference alongside the rules below.

## Step 1: Find all UI files

Discover the project's UI layer package by reading CLAUDE.md (look for the presentation/UI layer). If not documented, grep for `@Composable` to identify which packages contain UI code. Use Glob to find all `*.kt` files under the identified UI package(s). Read each file completely.

## Step 2: Audit and fix each file against ALL rules below

Work through every rule for every file. Fix violations immediately using the Edit tool.

---

### RULE 1: Semantic Merging

Every `semantics(mergeDescendants = true)` block MUST include `isTraversalGroup = true` as its first property. This ensures proper TalkBack swipe navigation grouping.

**Bad:**
```kotlin
.semantics(mergeDescendants = true) {
    contentDescription = label
}
```

**Good:**
```kotlin
.semantics(mergeDescendants = true) {
    isTraversalGroup = true
    contentDescription = label
}
```

### RULE 2: Clear Child Semantics in Merged Containers

When a parent uses `mergeDescendants = true` with explicit `contentDescription` or `stateDescription`, ALL children that would produce their own accessibility nodes (Text, Checkbox, Switch, OutlinedTextField, ExposedDropdownMenuBox, Button) MUST use `Modifier.clearAndSetSemantics {}` to prevent duplicate announcements.

Exception: Children inside popup windows (ExposedDropdownMenu items, Dialog content) keep their own semantics since they live in a separate accessibility tree.

### RULE 3: Native Roles

Use Compose `Role.*` constants so TalkBack announces widget types in the user's system language. NEVER define custom string resources for role names like "Checkbox", "Switch", "Dropdown".

Available roles: `Role.Button`, `Role.Checkbox`, `Role.Switch`, `Role.RadioButton`, `Role.Tab`, `Role.Image`, `Role.DropdownList`

### RULE 4: Checkbox Pattern

```kotlin
val fieldLabel = stringResource(R.string.xxx)
Row(
    modifier = Modifier
        .semantics(mergeDescendants = true) {
            contentDescription = fieldLabel
        }
        .toggleable(
            value = checked,
            onValueChange = { onToggle() },
            role = Role.Checkbox,
        )
) {
    Checkbox(
        checked = checked,
        onCheckedChange = null, // parent handles
        modifier = Modifier.clearAndSetSemantics {},
    )
    Text(
        text = fieldLabel,
        modifier = Modifier.clearAndSetSemantics {},
    )
}
```

TalkBack: "Label, Checkbox, Checked, Double-tap to toggle" (all native strings).

### RULE 5: Switch Pattern

Same as checkbox but with `Role.Switch`. The parent Row/Column uses `.toggleable(role = Role.Switch)`. Child `Switch` has `onCheckedChange = null` and `Modifier.clearAndSetSemantics {}`.

### RULE 6: Dropdown Pattern

```kotlin
val selectedLabel = stringResource(selectedOption.labelResId)
Column(
    modifier = Modifier
        .semantics(mergeDescendants = true) {
            isTraversalGroup = true
            contentDescription = label
            stateDescription = selectedLabel
        }
        .clickable(role = Role.DropdownList) { expanded = true }
) {
    Text(text = label, modifier = Modifier.clearAndSetSemantics {})
    ExposedDropdownMenuBox(
        modifier = Modifier.clearAndSetSemantics {},
    ) {
        OutlinedTextField(
            modifier = Modifier.clearAndSetSemantics {},
        )
        ExposedDropdownMenu { /* items keep their own semantics */ }
    }
}
```

TalkBack: "Label, Drop down list, Selected value, Double-tap to activate"

### RULE 7: Content Descriptions

- Every `Image()` and `Icon()` MUST have a `contentDescription` parameter.
- Decorative icons (paired with a text label in the same button): `contentDescription = null`
- Meaningful standalone icons: `contentDescription = stringResource(R.string.xxx)` (localized)
- NEVER include the element type in the description. "Submit" not "Submit button".
- NEVER use hardcoded strings. Always use `stringResource()`.

### RULE 8: Click Labels

Clickable composables should specify what happens when activated:

```kotlin
Row(
    Modifier.clickable(
        onClickLabel = stringResource(R.string.action_xxx)
    ) { action() }
)
```

For Card/Surface with onClick that doesn't expose onClickLabel:
```kotlin
Card(
    modifier = Modifier.semantics { onClick(label = actionLabel, action = null) },
    onClick = { action() }
)
```

### RULE 9: Headings

All dialog titles MUST be marked as headings for TalkBack heading navigation:

```kotlin
AlertDialog(
    title = {
        Text(
            text = stringResource(R.string.dialog_title),
            modifier = Modifier.semantics { heading() },
        )
    },
)
```

Also mark section headers and screen titles with `heading()`.

### RULE 10: Live Regions

- Dynamic content that updates without user interaction: `liveRegion = LiveRegionMode.Polite`
- Error messages that need immediate attention: `liveRegion = LiveRegionMode.Assertive`
- Timer displays: use `Polite` only (never Assertive, to avoid overwhelming the user)
- Snackbar/Toast messages: handled automatically by Material3 components

### RULE 11: Touch Targets

All interactive elements MUST be at least 48dp x 48dp. Check:
- `IconButton` provides 48dp automatically (OK)
- Custom clickable icons: add padding to reach 48dp, or use `Modifier.sizeIn(minWidth = 48.dp, minHeight = 48.dp)`
- Buttons with Material3: handled automatically (OK)

### RULE 12: Custom Actions

For controls with multiple sub-actions (increment/decrement, move up/down), consolidate into `customActions`:

```kotlin
.semantics(mergeDescendants = true) {
    isTraversalGroup = true
    contentDescription = description
    customActions = listOf(
        CustomAccessibilityAction(label) { action(); true },
    )
}
```

Clear the original individual action buttons with `Modifier.clearAndSetSemantics {}`.

### RULE 13: State Descriptions

Use `stateDescription` for the current value/state of a control. Place it BEFORE `toggleable`/`clickable` in the modifier chain:

```kotlin
Modifier
    .semantics { stateDescription = currentValue }
    .toggleable(value = checked, role = Role.Checkbox, ...)
```

Use `contentDescription` for the label/name, `stateDescription` for the current state.

### RULE 14: Localization

Every accessibility string MUST exist in ALL locale-specific string files. Discover all `app/src/main/res/values*/strings.xml` files to determine the project's supported locales.

After adding any new string resources, verify ALL locale files have matching keys.

### RULE 15: Decorative Elements

- Decorative images/icons: `contentDescription = null`
- Visual separators, watermarks: `Modifier.semantics { hideFromAccessibility() }`
- Purely visual indicators already described by parent: `Modifier.clearAndSetSemantics {}`

### RULE 16: Collections and Lists

For `LazyColumn`/`LazyRow` with identifiable items:
- Container: consider `collectionInfo = CollectionInfo(rowCount = items.size, columnCount = 1)`
- Items: consider `collectionItemInfo = CollectionItemInfo(rowIndex = index, ...)`
- Items with multiple actions: consolidate into `customActions`

### RULE 17: Error Semantics

Error states on text fields or forms:
```kotlin
Modifier.semantics { error("Error message for screen reader") }
```

### RULE 18: Slider Pattern

```kotlin
Row(
    modifier = Modifier.semantics(mergeDescendants = true) {
        isTraversalGroup = true
        contentDescription = label
        stateDescription = "$label. $valueLabel"
        progressBarRangeInfo = ProgressBarRangeInfo(current = value, range = valueRange, steps = 0)
        setProgress { targetValue ->
            onValueChange(targetValue.coerceIn(valueRange))
            true
        }
    }
) {
    Text(text = label, modifier = Modifier.clearAndSetSemantics {})
    Slider(value = value, onValueChange = onValueChange, modifier = Modifier.clearAndSetSemantics {})
}
```

## Step 3: Verify string resources

Discover all localized string files by listing `app/src/main/res/values*/strings.xml`. Read the default `values/strings.xml` and every locale-specific variant. Ensure every accessibility string key added or referenced exists in ALL locale files. Ask the user for translations if uncertain about any non-English locale.

## Step 4: Build verification

```bash
export JAVA_HOME="C:/Program Files/Android/Android Studio/jbr"
./gradlew assembleDebug
```

## Step 5: Summary

Print a summary table of all issues found and fixed, organized by file and rule number.
