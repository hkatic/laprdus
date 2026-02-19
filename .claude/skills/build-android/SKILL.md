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
cd /c/Users/hrvoj/source/repos/github/hkatic/laprdus/android && if [[ "$(uname -s)" == MINGW* ]] || [[ "$(uname -s)" == MSYS* ]]; then export JAVA_HOME="/c/Program Files/Android/Android Studio/jbr"; else export JAVA_HOME="${JAVA_HOME:-/usr/lib/jvm/java-17-openjdk-amd64}"; fi && ./gradlew assembleDebug
```

### Release Build
```bash
cd /c/Users/hrvoj/source/repos/github/hkatic/laprdus/android && if [[ "$(uname -s)" == MINGW* ]] || [[ "$(uname -s)" == MSYS* ]]; then export JAVA_HOME="/c/Program Files/Android/Android Studio/jbr"; else export JAVA_HOME="${JAVA_HOME:-/usr/lib/jvm/java-17-openjdk-amd64}"; fi && ./gradlew assembleRelease
```

## Arguments

- `$ARGUMENTS` or no argument: Build debug APK
- `release`: Build release APK

If user specifies "release", use `assembleRelease` instead of `assembleDebug`.

## Expected Output

- Debug APK: `android/app/build/outputs/apk/debug/app-debug.apk`
- Release APK: `android/app/build/outputs/apk/release/app-release.apk`

## Automatic Voice Data Generation

The Gradle build automatically:
1. Checks if voice data exists in `data/voices/`
2. Runs SCons to generate voice data if missing
3. Copies voice data and dictionaries to APK assets

## Install on Device

Detect platform for ADB path:
```bash
if [[ "$(uname -s)" == MINGW* ]] || [[ "$(uname -s)" == MSYS* ]]; then
  ADB="/c/Users/hrvoj/AppData/Local/Android/Sdk/platform-tools/adb.exe"
else
  ADB="${ANDROID_HOME:-$HOME/Android/Sdk}/platform-tools/adb"
fi
"$ADB" install -r "android/app/build/outputs/apk/debug/app-debug.apk"
```
