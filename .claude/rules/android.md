---
paths:
  - "src/platform/android/**"
---
# Android

Moved from `CLAUDE.md` on 2026-11-23, unchanged. Claude Code loads this file when Claude reads a file matching the paths above; `CLAUDE.md` lists every topic file.

## The app and the TTS service

- **Android: the app and the TTS service have separate native engines** in one process: `LaprdusTTS.service` and `LaprdusTTS.app` (Hilt provides the latter). The JNI library (`src/platform/android/native/src/lib.rs`) keeps one engine per handle (`nativeCreate` returns it: a number that names an engine in a table, never a pointer) and every native call takes that handle; there is no global engine. The service follows the app through saved state only: a newly saved default voice is loaded at once, and the user dictionaries switch or any dictionary write (`DictionaryRepository.changes`) reloads the dictionaries (`LaprdusTTS.reloadDictionaries()`: bundled, then the user's from `userDictionaryDir`). Sequences such as "set the request's rate and pitch, then synthesize" and voice loads run inside `LaprdusTTS.exclusive { }`.

## Android

The Gradle project is `src/platform/android/` (it was `android/` at the root before the migration to Rust).

### Build Architecture

The Android app uses:
- **Hilt** for dependency injection
- **Jetpack Compose** for UI
- **cargo-ndk** for the native library: `liblaprdus.so` is the JNI crate `src/platform/android/native` (`laprdus-android`), which links the engine
- **Shared voice data** from `data/voices/`

### Building Android

```bash
# Once: the Rust targets and cargo-ndk
rustup target add aarch64-linux-android armv7-linux-androideabi x86_64-linux-android i686-linux-android
cargo install cargo-ndk

# Simply run Gradle - it generates the voice data and builds the native library itself
cd src/platform/android
./gradlew assembleDebug
```

**What Gradle does by itself** (`app/build.gradle.kts`):
1. `generateVoiceData` runs `cargo xtask data` (voice data in `data/voices/`, dictionaries in `data/dictionary/`)
2. `buildNativeLibrary` runs `cargo ndk` for `arm64-v8a`, `armeabi-v7a`, `x86_64` and `x86` (`--platform 24`, `build --release -p laprdus-android`) into `app/build/rustJniLibs`. It first asks rustup whether the four Rust targets are installed and fails with the `rustup target add` command for the missing ones (without rustup the check is skipped); a missing target otherwise shows only as rustc's "can't find crate for `std`" in the middle of the cargo output, under a Gradle error that names nothing but cargo's exit code. The library is built optimized for debug builds of the app too: an unoptimized synthesizer is too slow for a screen reader, and Rust keeps its bounds checks in release builds
3. `copyVoiceData` and `copyDictionaries` copy voice data and dictionaries to the APK assets

**Manual steps** (to see where a failing build stops):
```bash
# 1. Voice data and dictionaries (from project root)
cargo xtask data

# 2. The native library for one ABI (from project root; ANDROID_NDK_HOME must name the NDK)
cargo ndk -t arm64-v8a --platform 24 build --release -p laprdus-android

# 3. Then build Android app (from src/platform/android/)
cd src/platform/android
./gradlew assembleDebug
```

### Voice Data Pipeline

```
data/hr/Josip/*.wav  ──┐
                       ├──▶ cargo xtask data (the engine's pack())
data/sr/Vlado/*.wav  ──┘
                              │
                              ▼
                        data/voices/Josip.bin
                        data/voices/Vlado.bin
                              │
        ┌─────────────────────┼─────────────────────┐
        ▼                     ▼                     ▼
    Android app          NVDA addon           SAPI5 installer
  (assets from           (copied by           (copied by
   data/voices/)          SCons)               InnoSetup)
```

### Toolchain, target and Play requirements (build 35, November 2026)

AGP 9.5.1 (built-in Kotlin; there is no `kotlin-android` plugin), Gradle 9.9.0, Kotlin 2.4.21, KSP 2.3.13, Compose BOM 2026.09.00 (ui 1.12.1, material3 1.4.0), NDK r30 (30.0.16248370), which cargo-ndk uses as the linker and sysroot of the four Android Rust targets (the native library is the Rust crate `src/platform/android/native`; the NDK compiles no source of ours any more), Java/Kotlin target 17, `compileSdk` and `targetSdk` 37 (Android 17). Icons are Material Symbols vector drawables in `res/drawable/ic_*.xml` (`material-icons-extended` is unmaintained and gone); `hiltViewModel()` comes from `hilt-lifecycle-viewmodel-compose`, and `navigation-compose` is a dependency of its own. The androidTest classpath pins `espresso-core` 3.7.0 and `runner` 1.7.0, which nothing calls directly: without them Compose's test rules bring Espresso 3.5, which calls `InputManager.getInstance()`, gone in Android 17 (every Compose test fails there). The accessibility checks bring Guava, so `listenablefuture` is excluded from the androidTest configurations, and `fragment` 1.9.1 / `concurrent-futures` 1.3.0 are pinned in the app (Hilt and profileinstaller brought old ones). Robolectric 4.17 runs the unit tests on SDK 37 with `--add-exports=java.base/jdk.internal.access=ALL-UNNAMED` (set in `app/build.gradle.kts`). `gradle.properties` keeps no AGP 9 opt-out; R8 runs in strict full mode.

- **16 KB pages** (Google Play blocks updates without them from 1 February 2027): the Rust library is linked with `-Wl,-z,max-page-size=16384` on every ABI (`.cargo/config.toml`, `[target.'cfg(target_os = "android")']`), together with `-Wl,-z,now` and `-Wl,-z,relro` (symbols bound at load, read-only relocations). How to check: `zipalign -c -P 16 -v 4 <apk>`, `llvm-readelf -lW liblaprdus.so` (LOAD 0x4000 on all four ABIs), and the instrumented tests on an API 37 16 KB emulator (`system-images;android-37.0;google_apis_playstore_ps16k;x86_64`, AVD `Laprdus_API37_16KB`); these checks were recorded for the C++ library of build 35 and are to be repeated on the Rust one.
- **Memory safety:** the engine is safe Rust (`unsafe` is forbidden in it; the JNI crate has it only for its exported names), bounds checks stay on in release builds, and the JNI library catches a panic and returns the call's failure value, so a fault does not take the screen reader's voice down. Cargo builds the library with `--release` for debug builds of the app too: an unoptimized synthesizer is too slow for a screen reader. Play has no MTE or hardening requirement. Release bundles carry native symbol tables (`ndk.debugSymbolLevel = "SYMBOL_TABLE"`) for Play's crash and ANR reports.
- **Memory and DEX (Play's core vitals from February 2027):** the release-like build uses about 69 MB PSS with Settings open and TalkBack speaking (the debug build 127 MB), DEX 2.6 MB (the 25 % shrinking rule applies above 10 MB). No wake locks.
- **Android 17 background audio:** the engine process plays `speak()` audio itself. On Android 17 with target 37, TalkBack's speech and a background app's speech through Laprdus play (`dumpsys audio`: the AudioTrack is `started`, `mutedState:none`; no `AudioHardening` entry): a client's binding keeps the process at bound-foreground-service state. `BackgroundSpeechTest` in `baselineprofile/` speaks from another process.
- **Baseline profile:** module `src/platform/android/baselineprofile` (`BaselineProfileGenerator`, `SettingsBenchmark`, `BackgroundSpeechTest`); the profile is checked in under `app/src/main/generated/baselineProfiles/` (`mergeIntoMain`). Do not regenerate it with Gradle on the S22 (Gradle's connected tasks uninstall the app): install `app-nonMinifiedRelease.apk` and `baselineprofile-nonMinifiedRelease.apk` with `adb install -r` and run `am instrument -w -e class com.hrvojekatic.laprdus.baselineprofile.BaselineProfileGenerator -e targetAppId com.hrvojekatic.laprdus com.hrvojekatic.laprdus.baselineprofile/androidx.test.runner.AndroidJUnitRunner`, then pull `/sdcard/Android/media/com.hrvojekatic.laprdus.baselineprofile/BaselineProfileGenerator_generate-startup-prof.txt` into both `baseline-prof.txt` and `startup-prof.txt`. Settings scroll on the S22, benchmarkRelease: frame CPU P50 3.0 ms, P99 13.8 ms with the profile (3.2 / 14.4 without).

### Settings screen and TalkBack

`ui/screens/SettingsScreen.kt` follows Android's settings guidelines: sections with `heading()` titles (Voice; Apps and screen readers, the four force switches; Reading; Reading pauses; Dictionaries; About) over groups of rounded rows (`ui/components/SettingsRows.kt`: `SettingsSection`, `SwitchRow`, `SliderRow`, `ClickableRow`, `ExpandRow`, `ChoiceDialog`), a plain `TopAppBar` (a collapsing one draws its title twice and moves while TalkBack speaks), a `Column` with `verticalScroll` (every node is in the accessibility tree), `widthIn(max = 840.dp)`. "More voice settings" (`ExpandRow`, `rememberSaveable`, collapsed on every visit) holds acceleration, inflection, volume boost and the four voice quality settings; TalkBack says "collapsed"/"expanded" from its expand/collapse actions, so it has no state description of its own. The screen takes `SettingsActions` (implemented by `SettingsViewModel`); the view model updates the state first and saves through one ordered writer on IO that outlives it, and makes no engine calls of its own (the main screen's engine and the service follow the saved settings; `LaprdusTTS.applySettings()` sets only what changed, `distinctUntilChanged()` in both collectors). Sliders commit on `onValueChangeFinished` (TalkBack's `setProgress` calls it too) and move in steps (`ui/settings/SettingFormat.kt`: 0.05x rate/pitch, 0.1x acceleration, 5 % percentages, 50 ms pauses; one TalkBack swipe is one step); "Restore default" is a custom action of the slider and a visible button with cleared semantics. Numbers use the locale's decimal separator, `format_percent` / `format_milliseconds` the CLDR spacing ("50 %" hr and sl).

What Samsung TalkBack 16 says (`TalkBackSpeechTest`): "Uključeno, Slova kao glasovi, Pri slovkanju i tipkanju, Prekidač, Dodirnite dvaput da biste prebacili"; "1,0x, Brzina govora, Klizač, Prelazak prstom prema gore ili dolje za prilagodbu"; "Glas, Zvonko, Dodirnite dvaput…"; "sažeto, Više postavki glasa, Ubrzanje, Intonacija, …". The state first is TalkBack's own element description order (a TalkBack setting); title and summary follow in the order shown. Two Compose 1.12 traps: a merging node with semantics children gets its `contentDescription` moved to a fake child node, which TalkBack does not read on a SeekBar, and `Modifier.background(color, shape)` is a semantics node (a `Shape` property), so the stock M3 slider thumb made every slider "1,0x, Klizač" without its name: `SliderThumb` draws the thumb instead, and `everySlider_reachesTalkBackWithItsName` checks the platform `AccessibilityNodeInfo` of every slider. Texts of a row are merged children, never cleared and replaced by a content description.

### Android storage layout (Direct Boot)

`LaprdusTTSService` is `android:directBootAware="true"` so TalkBack can use Laprdus on the lock screen after a reboot, before the first unlock. Everything the service needs at that point lives in **device-protected (DE) storage** under `/data/user_de/0/<pkg>/files`:

- settings DataStore: `datastore/laprdus_settings.preferences_pb` (opened via `deviceProtectedDataStoreFile`, process singleton in `LaprdusStorage`)
- user dictionaries: `user.json`, `spelling.json`, `emoji.json` (written atomically via `AtomicFiles`), and the accent lexicon `accents.json` (edited as the fourth dictionary type, `DictionaryType.ACCENTS`)

Voice data and bundled dictionaries load from APK assets, so synthesis itself never depends on storage.

**Migration.** Builds 10 and earlier stored this data in credential-encrypted (CE) storage (`/data/user/0/<pkg>/files`), which is unreadable while locked. `SettingsMigrator` and `DictionaryMigrator` copy it once after unlock: DE values win, legacy values only fill missing keys, every step is crash-safe and idempotent (re-running after a crash converges), and the migrators are re-armed on `ACTION_USER_UNLOCKED`, on a rate-limited delayed retry, and on the next process start. Debug builds can force a crash at a named step by writing a `MigrationCrashPoint` name into `files/debug/crash_point` in the DE directory.

**Failure policy.** Storage problems (unreadable or corrupt DE/legacy files, failed copies) never crash: they are logged, defaults are used, a localized notice is shown in the Settings screen (settings) or the Dictionaries screen (dictionaries), and migration is retried later. Only "no voice loadable at all" (requested voice and the default-voice fallback, `zvonko`, both fail) throws `LaprdusEngineUnavailableException`, and only from the synthesis path, so `speak()` returns ERROR and TalkBack can switch engines. A DE crash marker bounds this to 3 throws per 10 minutes; after that the service reports `callback.error()` per utterance instead.

**Backup rules.** `backup_rules.xml` and `data_extraction_rules.xml` must stay empty: with no rules the framework backs up both CE and DE files, while adding any `<include>` flips the file to include-only and silently drops everything not listed.

## Android Device Testing

#### ADB Path (Developer Machine)

The Android SDK platform-tools are located at:
```
C:\Users\hrvoj\AppData\Local\Android\Sdk\platform-tools\adb.exe
```

On the macOS machine:
```
~/Library/Android/sdk/platform-tools/adb
```

#### Device Testing Commands

```bash
# Check for connected devices
"/c/Users/hrvoj/AppData/Local/Android/Sdk/platform-tools/adb.exe" devices

# Install debug APK on connected device
"/c/Users/hrvoj/AppData/Local/Android/Sdk/platform-tools/adb.exe" install -r "src/platform/android/app/build/outputs/apk/debug/app-debug.apk"

# Install release APK on connected device
"/c/Users/hrvoj/AppData/Local/Android/Sdk/platform-tools/adb.exe" install -r "src/platform/android/app/build/outputs/apk/release/app-release.apk"

# Uninstall app from device
"/c/Users/hrvoj/AppData/Local/Android/Sdk/platform-tools/adb.exe" uninstall com.hrvojekatic.laprdus

# View device logs (filter by app)
"/c/Users/hrvoj/AppData/Local/Android/Sdk/platform-tools/adb.exe" logcat -s "Laprdus"
```

#### Complete Android Build and Test Workflow

```bash
# From project root directory

# 1. Build the debug APK
cd src/platform/android && export JAVA_HOME="/c/Program Files/Android/Android Studio/jbr" && ./gradlew assembleDebug && cd ../../..

# 2. Install on connected device
"/c/Users/hrvoj/AppData/Local/Android/Sdk/platform-tools/adb.exe" install -r "src/platform/android/app/build/outputs/apk/debug/app-debug.apk"
```

#### Android Test Commands

Run from `src/platform/android/` with `JAVA_HOME` set (Windows: `/c/Program Files/Android/Android Studio/jbr`, macOS: `/Applications/Android Studio.app/Contents/jbr/Contents/Home`):

```bash
cd src/platform/android
./gradlew testDebugUnitTest lintDebug          # JVM unit tests, lint
./gradlew assembleDebug assembleDebugAndroidTest
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb install -r -t app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk
adb shell am instrument -w com.hrvojekatic.laprdus.test/androidx.test.runner.AndroidJUnitRunner
```

Notes:
- On the S22 do not use `connectedDebugAndroidTest`: Gradle's connected tasks uninstall the app afterwards, which loses the phone's settings and TalkBack's engine. Install with `adb install -r` and run `am instrument` (add `-s <serial>` with the emulator connected). The device must be unlocked for the UI tests ("No compose hierarchies found" when it is locked).
- `am instrument` kills the app process, and TalkBack often does not reconnect to its engine: afterwards run `adb shell "settings put secure tts_default_synth com.google.android.tts; sleep 2; settings put secure tts_default_synth com.hrvojekatic.laprdus"`. Anything using UiAutomator without `FLAG_DONT_SUPPRESS_ACCESSIBILITY_SERVICES` (the baseline profile generator, the macrobenchmarks) leaves `enabled_accessibility_services` empty, which switches TalkBack off: restore it with `settings put secure enabled_accessibility_services com.samsung.android.accessibility.talkback/com.samsung.android.marvin.talkback.TalkBackService` and `settings put secure accessibility_enabled 1`.
- The first `testDebugUnitTest` run downloads Robolectric's android-all SDK 37 jar into `~/.m2`; the Robolectric tests pin `@Config(sdk = [37])`.
- `SettingsScreenAccessibilityTest` checks the Compose semantics locale-independently (strings from the resources), each slider's platform `AccessibilityNodeInfo`, one stop per setting (21 collapsed, 28 expanded), and runs the Accessibility Test Framework (`enableAccessibilityChecks()`, API 34+).
- `ui/TalkBackSpeechTest` (with `TalkBackWalker`) runs only where TalkBack is on and Laprdus is its engine: it focuses every node with `UiAutomation` (`FLAG_DONT_SUPPRESS_ACCESSIBILITY_SERVICES`) and records what TalkBack sends to the service (`service/SpeechRequestLog.kt`, debug builds only; also logcat tag `LaprdusSpeech` while `files/debug/log_speech` exists in device-protected storage). `everySettingIsSpokenInFull` asserts the title before the summary, the name and value of every slider and the focus-to-audio latency; `recordSettingsTranscript` writes the transcript to logcat (tag `TalkBackWalker`). Injected `input swipe` gestures are not TalkBack gestures. Settings, November 2026, debug build: 28 stops (43 before), focus to first audio median 198 ms.

#### Testing Checklist for Android

- [ ] Device connected and recognized by `adb devices`
- [ ] APK installs successfully
- [ ] App launches without crashes
- [ ] TTS engine appears in Android TTS settings
- [ ] Voice synthesis works correctly
- [ ] Settings screen is accessible with TalkBack
- [ ] All controls (sliders, switches, dropdowns) work with TalkBack
- [ ] Direct Boot: with a PIN set, reboot and do NOT unlock; TalkBack must speak through Laprdus on the lock screen with the saved voice/speed; `logcat -s LaprdusTTSService` shows `userUnlocked=false`; after unlock the receiver fires and dictionaries reload
- [ ] Migration: install the previous build, save settings and a dictionary entry, update, verify values survive (`SettingsMigrator`/`DictionaryMigrator` "Migrated" in logcat) and that a forced crash via `/data/user_de/0/com.hrvojekatic.laprdus/files/debug/crash_point` (debug builds) recovers on the next start
