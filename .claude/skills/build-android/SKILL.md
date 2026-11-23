---
name: build-android
description: Build Android APK for LaprdusTTS (debug or release)
allowed-tools: Bash, Read
argument-hint: "[debug|release]"
---

# Build Android APK

Build the Android TTS engine app. Supports both debug and release builds.
Works on Windows (Git Bash/MINGW) and Linux.

## Build Steps

### Debug Build (default)
```bash
cd "$(git rev-parse --show-toplevel)/src/platform/android" && case "$(uname -s)" in MINGW*|MSYS*) export JAVA_HOME="/c/Program Files/Android/Android Studio/jbr";; Darwin) export JAVA_HOME="/Applications/Android Studio.app/Contents/jbr/Contents/Home";; *) export JAVA_HOME="${JAVA_HOME:-/usr/lib/jvm/java-17-openjdk-amd64}";; esac && ./gradlew assembleDebug
```

### Release Build
```bash
cd "$(git rev-parse --show-toplevel)/src/platform/android" && case "$(uname -s)" in MINGW*|MSYS*) export JAVA_HOME="/c/Program Files/Android/Android Studio/jbr";; Darwin) export JAVA_HOME="/Applications/Android Studio.app/Contents/jbr/Contents/Home";; *) export JAVA_HOME="${JAVA_HOME:-/usr/lib/jvm/java-17-openjdk-amd64}";; esac && ./gradlew assembleRelease
```

### Release-like Build for the Device
`assembleBenchmarkRelease` (same command, other task) builds the release code (R8, optimized native engine, baseline profile) signed with the debug key, so it installs over a debug build with `adb install -r` and keeps the app's data.

## Arguments

- `$ARGUMENTS` or no argument: Build debug APK
- `release`: Build release APK
- `benchmark`: Build the release-like, debug-signed APK

If user specifies "release", use `assembleRelease` instead of `assembleDebug`; "benchmark", `assembleBenchmarkRelease`.

## Expected Output

- Debug APK: `src/platform/android/app/build/outputs/apk/debug/app-debug.apk`
- Release APK: `src/platform/android/app/build/outputs/apk/release/app-release-unsigned.apk` (the project has no signing config; sign it with the release key before distributing)
- Release-like APK: `src/platform/android/app/build/outputs/apk/benchmarkRelease/app-benchmarkRelease.apk`

## What Gradle Does by Itself

The Gradle build automatically:
1. Packs the voice data and stages the dictionaries (`cargo xtask data`, into `data/voices/` and `data/dictionary/`)
2. Builds the native engine `liblaprdus.so` for every ABI from the Rust sources (`cargo ndk`)
3. Copies voice data and dictionaries to APK assets

It needs the Rust toolchain with the Android targets and cargo-ndk:
```bash
rustup target add aarch64-linux-android armv7-linux-androideabi x86_64-linux-android i686-linux-android
cargo install cargo-ndk
```

## Install on Device

Detect platform for ADB path:
```bash
if [[ "$(uname -s)" == MINGW* ]] || [[ "$(uname -s)" == MSYS* ]]; then
  ADB="/c/Users/hrvoj/AppData/Local/Android/Sdk/platform-tools/adb.exe"
else
  ADB="${ANDROID_HOME:-$HOME/Android/Sdk}/platform-tools/adb"
fi
"$ADB" install -r "src/platform/android/app/build/outputs/apk/debug/app-debug.apk"
```
