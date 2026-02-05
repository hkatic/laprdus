---
name: build-android
description: Build Android APK for LaprdusTTS (debug or release)
allowed-tools: Bash, Read
argument-hint: "[debug|release]"
---

# Build Android APK

Build the Android TTS engine app. Supports both debug and release builds.

## Build Steps

### Debug Build (default)
```bash
cd android && cmd.exe /c "set JAVA_HOME=C:\\Program Files\\Android\\Android Studio\\jbr&& gradlew.bat assembleDebug" && cd ..
```

### Release Build
```bash
cd android && cmd.exe /c "set JAVA_HOME=C:\\Program Files\\Android\\Android Studio\\jbr&& gradlew.bat assembleRelease" && cd ..
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

## Verification

After building, verify the APK contains voice data:
```python
python3 -c "
import zipfile
apk = 'android/app/build/outputs/apk/debug/app-debug.apk'
with zipfile.ZipFile(apk, 'r') as z:
    for name in z.namelist():
        if 'assets/voices' in name or 'assets/dictionaries' in name:
            print(name)
"
```

## Install on Device

```bash
"/c/Users/hrvoj/AppData/Local/Android/Sdk/platform-tools/adb.exe" install -r "android/app/build/outputs/apk/debug/app-debug.apk"
```
