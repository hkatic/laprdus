---
name: android-code-review
description: Full Android codebase review — bugs, crashes, compatibility, resources, tests
allowed-tools: Read, Glob, Grep, Edit, Write, Bash(./gradlew *), Bash(export *)
---

You are a senior Android engineer performing a pre-release code review. Scan every file in the project, find and fix all bugs, warnings, potential crashes, and code quality issues. Work methodically through each step below.

## Step 1: Scan all source files by architecture layer

Use Glob to find all `*.kt` files. Read `CLAUDE.md` to discover the project's architecture layers and key classes, then read every source file organized by those layers.

If `CLAUDE.md` does not describe the architecture, discover it from the source tree — list the top-level packages under `app/src/main/java/` and group files by package (e.g., domain, data, presentation, di, etc.).

For each file, check:
- Logic errors, off-by-one bugs, null safety issues
- Coroutine misuse (missing cancellation, wrong dispatcher, leaked jobs)
- Memory leaks (Context references, unregistered observers)
- Silent `catch` blocks that swallow exceptions without logging
- KDoc accuracy (do comments match actual behavior?)

## Step 2: Review build configuration

Read and verify:
- `build.gradle.kts` (root and app) — plugin versions, dependency versions, compile/target SDK
- `libs.versions.toml` — version catalog consistency
- `gradle.properties` — no deprecated properties
- `AndroidManifest.xml` — permissions, components, service declarations, intent filters
- `proguard-rules.pro` — rules if minification is enabled

Check that AGP, Kotlin, Hilt, KSP, and Compose BOM versions are compatible with each other. Reference CLAUDE.md compatibility notes.

## Step 3: Review resources

Read and verify:
- **strings.xml** — discover all localized `values-*/strings.xml` directories. Verify all keys exist in every locale file, no orphan keys, no hardcoded strings in code. Check CLAUDE.md for supported languages.
- **Drawables/mipmaps** — all referenced in code exist, no unreferenced files
- **Raw resources** — all `R.raw.*` references in code map to actual files in `res/raw/`
- **Themes/colors** — no unused colors, theme properly applied
- **data_extraction_rules.xml / backup_rules.xml** — correct if present

## Step 4: Check API compatibility

Target: minSdk through targetSdk (check `build.gradle.kts` for exact values).

- Every API call gated on SDK version must use `Build.VERSION.SDK_INT >= Build.VERSION_CODES.X`
- Use `ServiceCompat`, `ContextCompat`, `NotificationCompat` for backward-compatible APIs
- No 3-argument `startForeground()` without version check (added in API 29)
- No `getParcelableExtra(key, Class)` without version check (added in API 33)

## Step 5: Check for warnings and deprecated APIs

- Zero compile warnings target
- No deprecated Compose APIs — check CLAUDE.md for known deprecations; also grep for common deprecated APIs
- No deprecated Android APIs without `@Suppress("DEPRECATION")` + version-gated fallback
- Hilt annotation targets — `@ApplicationContext` and `@ActivityContext` on constructor params should use `@param:` target

## Step 6: Verify runtime correctness

Read CLAUDE.md for project-specific runtime patterns, then verify:

- Thread safety of all persistence read/write paths (DataStore, Room, SharedPreferences, etc.)
- Correct clock sources for timing (`SystemClock.elapsedRealtime()` for duration measurement, not `System.currentTimeMillis()`)
- Proper null/error handling on I/O operations (file import/export, network calls, stream handling)
- Debounce and throttle correctness (verify delay values and cancellation behavior)
- Foreground service lifecycle matches the component it serves (starts/stops in sync)
- Any other project-specific patterns documented in CLAUDE.md

## Step 7: Run tests and expand coverage

```bash
export JAVA_HOME="C:/Program Files/Android/Android Studio/jbr"
./gradlew testDebugUnitTest
```

- Fix any test failures, fixing implementation code if the test is correct
- Identify untested code paths
- Write new unit tests for uncovered use cases and ViewModel methods
- Follow existing patterns: Fake implementations, `StandardTestDispatcher`, `advanceUntilIdle()`
- Re-run tests to confirm all pass

## Step 8: Build verification

```bash
export JAVA_HOME="C:/Program Files/Android/Android Studio/jbr"
./gradlew assembleDebug
./gradlew testDebugUnitTest
```

Both must succeed with zero errors.

## Step 9: Summary

Print a findings table organized by severity:

| # | Severity | File | Description | Status |
|---|----------|------|-------------|--------|
| 1 | CRITICAL | ... | ... | FIXED |
| 2 | HIGH | ... | ... | FIXED |

Severity levels:
- **CRITICAL** — app crash on supported API levels
- **HIGH** — incorrect behavior visible to users
- **MEDIUM** — code quality, robustness, maintainability
- **LOW** — defensive improvements, documentation

Also list items verified as OK (build config, resources, API compat, etc.) so the user knows what was checked.
