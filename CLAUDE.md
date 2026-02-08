# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

LaprdusTTS is a Croatian/Serbian text-to-speech (TTS) engine supporting:
- **SAPI5** (Windows Speech API) for system-wide TTS integration
- **NVDA** screen reader synthesizer driver

It uses concatenative synthesis by joining pre-recorded phoneme WAV files to produce speech output.

## Architecture

### Build System
- **SCons** (Python-based) - see `SConstruct`
- Platforms: Windows, Linux, Android
- Architectures: x64, x86, ARM64, ARM
- Build: `scons --platform=windows --arch=x64 --build-config=release sapi5`

### Core C++ Components

| File | Purpose |
|------|---------|
| `src/core/tts_engine.cpp` | Main engine orchestrating synthesis pipeline |
| `src/core/phoneme_mapper.cpp` | UTF-8 text → phoneme token conversion |
| `src/core/croatian_numbers.cpp` | Number-to-words (supports up to centillions) |
| `src/core/inflection.cpp` | Punctuation-based pitch/emphasis modulation |
| `src/core/voice_registry.cpp` | Voice definitions (Josip, Vlado, derived voices) |
| `src/audio/audio_synthesizer.cpp` | Phoneme concatenation with crossfade |
| `src/audio/phoneme_data.cpp` | WAV file loading from .bin or directory |
| `src/c_api/laprdus_api.cpp` | C API for external consumers |

### Platform-Specific Code

| Path | Purpose |
|------|---------|
| `src/platform/windows/sapi5/` | SAPI5 COM driver (laprd32.dll, laprd64.dll) |
| `src/platform/linux/speechd/` | Speech Dispatcher module (sd_laprdus) |
| `src/platform/linux/cli/` | Linux command-line utility |
| `nvda-addon/` | NVDA Python synthesizer driver |

### Processing Pipeline

1. Text → `TTSEngine::preprocess_text()` → Number expansion
2. → `InflectionProcessor::analyze_text()` → Segment by punctuation
3. → `PhonemeMapper::map_text()` → UTF-8 to phoneme tokens
4. → `AudioSynthesizer::synthesize()` → Concatenate WAV samples with crossfade
5. → Apply volume, rate, pitch transformations
6. → `InflectionProcessor::apply_inflection()` → Pitch contours
7. → Output 16-bit PCM @ 22050Hz mono

### Phoneme System

- **Format**: 16-bit PCM WAV, 22050 Hz, mono
- **Storage**: Packed `.bin` files (Josip.bin, Vlado.bin) or raw WAV directory
- **Croatian phonemes**: A-Z + č, ć, đ, š, ž, lj, nj, dž
- **Truncation**: L, M, N, S, SH, V, Z, ZH capped at 2000 bytes
- **Crossfade**: 64 samples (~3ms) between phonemes

### Voice System

| Voice | Type | Language | base_pitch |
|-------|------|----------|------------|
| josip | Physical | Croatian | 1.0 |
| vlado | Physical | Serbian | 1.0 |
| detence | Derived (josip) | Croatian | 1.5 (child) |
| baba | Derived (josip) | Croatian | 1.2 (grandma) |
| djed | Derived (vlado) | Serbian | 0.75 (grandpa, Đedo) |

### Audio Parameters (types.hpp)

```cpp
struct VoiceParams {
    float speed = 1.0f;       // 0.5 - 2.0 (tempo, time-stretching)
    float pitch = 1.0f;       // 0.25 - 4.0 (voice character pitch)
    float user_pitch = 1.0f;  // 0.5 - 2.0 (user pitch preference)
    float volume = 1.0f;      // 0.0 - 1.0
    bool inflection_enabled = true;
};
```

## C API

```c
#include <laprdus/laprdus_api.h>

LaprdusHandle handle;
laprdus_create(&handle);
laprdus_set_voice(handle, "josip", "/path/to/data");
laprdus_set_speed(handle, 1.5f);
laprdus_set_pitch(handle, 1.0f);      // Voice character pitch
laprdus_set_user_pitch(handle, 1.2f); // User preference pitch

int16_t* samples;
LaprdusAudioFormat format;
laprdus_synthesize(handle, "Dobar dan!", &samples, &format);
// Use samples...
laprdus_free_audio(samples);
laprdus_destroy(handle);
```

## Audio Processing

### Sonic Library Integration
Rate and pitch are controlled independently using the Sonic library (`src/audio/sonic/`):
- **Speed** (rate): Time-stretching via PICOLA algorithm - changes tempo WITHOUT changing pitch
- **Pitch** (voice character): Pitch-shifting with formant shift - intentionally changes voice character (used for derived voices like child, grandma, grandpa)
- **User Pitch**: Pitch-shifting for user preference - intended to preserve voice character

The wrapper is in `src/audio/sonic_processor.cpp`.

### Dual Pitch System
Two separate pitch parameters serve different purposes:

| Parameter | Range | Purpose | Effect |
|-----------|-------|---------|--------|
| `pitch` | 0.25 - 4.0 | Voice character (derived voices) | Shifts formants - changes voice identity |
| `user_pitch` | 0.5 - 2.0 | User preference (SAPI5/NVDA slider) | Adjusts F0 - keeps voice identity |

**Processing order in AudioSynthesizer::synthesize():**
1. Volume adjustment
2. Speed/rate (Sonic time-stretching)
3. Voice character pitch (`pitch`) - Sonic pitch shift
4. User pitch preference (`user_pitch`) - formant_pitch.cpp

**Note:** The formant-preserving algorithm in `src/audio/formant_pitch.cpp` currently uses Sonic as a placeholder. The architecture is in place for a true formant-preserving implementation (e.g., stftPitchShift with cepstral analysis) when C++20 compatibility allows it.

## Dependencies

- C++17 compiler (MSVC on Windows, GCC/Clang on Linux)
- SCons build system (`pip install scons`)
- Windows: ATL (for SAPI5 COM) - included with Visual Studio
- InnoSetup 6+ (for SAPI5 installer) - https://jrsoftware.org/isinfo.php
- Python 3.9+ (for NVDA addon build)
- No external audio libraries (all processing is internal)

## Environment Configuration (Developer Machine)

### Tool Paths

| Tool | Path |
|------|------|
| **InnoSetup ISCC** | `"C:\Program Files (x86)\Inno Setup 6\ISCC.exe"` |
| **JAVA_HOME** | `C:\Program Files\Android\Android Studio\jbr` |

### Running InnoSetup from Command Line

```bash
# Use full path with quotes due to spaces
"/c/Program Files (x86)/Inno Setup 6/ISCC.exe" installers/windows/laprdus_sapi5.iss
```

### Setting JAVA_HOME for Android Builds

```bash
# Set JAVA_HOME before running Gradle
export JAVA_HOME="/c/Program Files/Android/Android Studio/jbr"
cd android && ./gradlew assembleRelease
```

## Building

### Automated Build System (Recommended)

**IMPORTANT: Use the master build script to ensure all dependencies are built correctly.**

The build system automatically handles:
1. **Voice data generation** - Phonemes are packed into `.bin` files and copied to `data/voices/`
2. **Library rebuilding** - DLLs/SOs are rebuilt with latest core changes
3. **Resource copying** - Dictionaries and voice data are copied to platform-specific locations

#### Master Build Scripts

```bash
# Build all platforms (recommended)
./scripts/build-all.sh                # Linux/macOS/WSL
scripts\build-all.cmd                 # Windows CMD

# Build specific platform
./scripts/build-all.sh sapi5          # Windows SAPI5 only
./scripts/build-all.sh nvda           # NVDA addon only
./scripts/build-all.sh android        # Android APK only
./scripts/build-all.sh linux          # Linux only
./scripts/build-all.sh voice-data     # Generate voice data only
```

#### What the Build System Does Automatically

When you run any SCons build target:
1. **phoneme_packer** tool is built first
2. **Voice data** (Josip.bin, Vlado.bin) is generated from `phonemes/*/` directories
3. **Voice data is copied** to `data/voices/` for all platforms to use
4. **Platform-specific build** proceeds with all dependencies in place

The SAPI5 installer, NVDA addon, and Android builds all source voice data from `data/voices/`.

#### Build Dependencies Flow

```
phonemes/Josip/*.wav  ──┐
                        ├──▶ phoneme_packer ──▶ build/*/Josip.bin ──▶ data/voices/Josip.bin
phonemes/Vlado/*.wav  ──┘                       build/*/Vlado.bin ──▶ data/voices/Vlado.bin
                                                                            │
        ┌───────────────────────────────────────────────────────────────────┤
        │                           │                           │           │
        ▼                           ▼                           ▼           ▼
   SAPI5 Installer            NVDA Addon                  Android APK    Linux Install
  (data/voices/*.bin)    (copies to addon dir)          (assets/voices)  (/usr/share)
```

### Prerequisites

1. **Visual Studio 2019+** with C++ Desktop workload and ATL
2. **SCons**: `pip install scons`
3. **InnoSetup 6+**: Install from https://jrsoftware.org/isdl.php (for SAPI5 installer)

### Build Commands

All builds use SCons from the project root directory.

#### Core Library / DLL

```bash
# Windows x64 release
scons --platform=windows --arch=x64 --build-config=release

# Windows x86 release (needed for 32-bit apps)
scons --platform=windows --arch=x86 --build-config=release

# Debug build
scons --platform=windows --arch=x64 --build-config=debug
```

#### SAPI5 DLLs Only

```bash
# Build SAPI5 target specifically
scons --platform=windows --arch=x64 --build-config=release sapi5
scons --platform=windows --arch=x86 --build-config=release sapi5
```

Output: `build/windows-x64-release/bin/laprd64.dll`, `build/windows-x86-release/bin/laprd32.dll`

#### SAPI5 Installer (InnoSetup)

```bash
# First build both x86 and x64 SAPI5 DLLs
scons --platform=windows --arch=x64 --build-config=release sapi5
scons --platform=windows --arch=x86 --build-config=release sapi5

# Then build installer
iscc installers/windows/laprdus_sapi5.iss
```

Output: `installers/windows/Output/Laprdus_SAPI5_Setup_1.0.0.exe`

#### Windows CLI

The Windows CLI (`laprdus.exe`) provides command-line access to TTS, identical to the Linux CLI.

```bash
# Build Windows CLI (both architectures)
scons --platform=windows --arch=x64 --build-config=release cli
scons --platform=windows --arch=x86 --build-config=release cli
```

Output:
- `build/windows-x64-release/laprdus.exe` (64-bit)
- `build/windows-x86-release/laprdus.exe` (32-bit)

The CLI is automatically included in the SAPI5 installer. For standalone use:

```bash
# Show help
laprdus.exe -h

# List available voices
laprdus.exe -l

# Speak text to audio device
laprdus.exe "Dobar dan!"

# Output to WAV file
laprdus.exe -o output.wav "Text to speak"

# Use specific voice with options
laprdus.exe -v vlado -r 1.5 -p 1.2 "Zdravo svete!"

# Read from file
laprdus.exe -i input.txt -o output.wav

# Read from stdin
echo "Text" | laprdus.exe -o output.wav
```

**CLI Options:**
| Option | Description |
|--------|-------------|
| `-v, --voice` | Select voice (josip, vlado, detence, baba, djed) |
| `-r, --speech-rate` | Speech rate 0.5-2.0 (default: 1.0) |
| `-p, --speech-pitch` | Speech pitch 0.5-2.0 (default: 1.0) |
| `-V, --speech-volume` | Volume 0.0-1.0 (default: 1.0) |
| `-d, --numbers-digits` | Speak numbers as digits |
| `-c, --comma-pauses` | Comma pause duration in ms |
| `-e, --period-pauses` | Period pause duration in ms |
| `-o, --output-file` | Output to WAV file |
| `-i, --input-file` | Read text from file |
| `-D, --data-dir` | Voice data directory |
| `-l, --list-voices` | List available voices |
| `-w, --verbose` | Enable verbose output |
| `-h, --help` | Show help message |

#### NVDA Addon

**CRITICAL BUILD RULES - FOLLOW EXACTLY:**

1. **ALWAYS use SCons to build the NVDA addon:**
   ```bash
   cd nvda-addon
   scons
   ```

2. **NEVER do any of the following:**
   - NEVER manually create the .nvda-addon file using Python's zipfile or any archive tool
   - NEVER manually edit `addon/manifest.ini` - it is AUTO-GENERATED by SCons from `buildVars.py`
   - NEVER manually edit `addon/locale/*/manifest.ini` - these are AUTO-GENERATED
   - NEVER manually edit `addon/doc/en/readme.html` - it is AUTO-GENERATED from `readme.md`

3. **To modify addon metadata** (name, version, description, etc.):
   - Edit `nvda-addon/buildVars.py` - this is the ONLY place to change addon info
   - SCons reads buildVars.py and generates manifest.ini automatically

4. **To modify addon documentation**:
   - Edit `nvda-addon/readme.md` - SCons converts this to HTML automatically

5. **To add/modify translations**:
   - Edit `.po` files in `nvda-addon/addon/locale/*/LC_MESSAGES/`
   - SCons compiles .po to .mo files automatically

Output: `nvda-addon/laprdus-*.nvda-addon`

**Automatic Resource Handling**: The NVDA addon SCons build automatically:
- Copies DLLs from `build/windows-x64-release/` and `build/windows-x86-release/`
- Copies GUI executables (`laprdgui.exe` as `laprdgui64.exe` and `laprdgui32.exe`) for the Laprdus Configurator
- Copies voice data from `data/voices/` to `addon/synthDrivers/laprdus/voices/`
- Copies dictionaries from `data/dictionary/` to `addon/synthDrivers/laprdus/dictionaries/`

**NVDA Menu Integration**: The addon includes a globalPlugin that adds a "Laprdus" submenu to NVDA's Tools menu with:
- "Laprdus Configurator..." - Opens the configuration GUI (launches the correct 32/64-bit exe)
- "Laprdus on the Web" - Opens the Laprdus website

**Complete Build Command** (recommended):
```bash
# Build everything needed for NVDA addon
./scripts/build-all.sh nvda
# Or on Windows CMD:
scripts\build-all.cmd nvda
```

**Manual Build** (if master script unavailable):
```bash
# 1. Build voice data first
scons --platform=windows --arch=x64 --build-config=release voice-data

# 2. Build both SAPI5 DLLs
scons --platform=windows --arch=x64 --build-config=release sapi5
scons --platform=windows --arch=x86 --build-config=release sapi5

# 3. Build config GUI for both architectures
scons --platform=windows --arch=x64 --build-config=release config
scons --platform=windows --arch=x86 --build-config=release config

# 4. Build NVDA addon (automatically copies DLLs, GUI executables, voice data, and dictionaries)
cd nvda-addon && scons
```

**Localization**: The addon supports Croatian (hr) and Serbian (sr) translations. Translation files are in:
- `nvda-addon/addon/locale/hr/LC_MESSAGES/nvda.po`
- `nvda-addon/addon/locale/sr/LC_MESSAGES/nvda.po`

The SCons build automatically compiles .po files to .mo files using either `msgfmt` (if available) or Python's `polib` library as a fallback. Install polib with: `pip install polib`

### Clean Build

```bash
scons -c  # Clean all targets
scons -c --platform=windows --arch=x64  # Clean specific config
```

### Build Targets

| Target | Description |
|--------|-------------|
| (default) | Build all targets for platform/arch |
| `sapi5` | SAPI5 COM DLL only |
| `cli` | Command-line test tool |

### Platform/Architecture Options

| Option | Values |
|--------|--------|
| `--platform` | `windows`, `linux`, `android` |
| `--arch` | `x64`, `x86`, `arm64`, `arm` |
| `--build-config` | `release`, `debug` |

## Testing

### SAPI5 Voices

After installing SAPI5, test with PowerShell:
```powershell
Add-Type -AssemblyName System.Speech
$synth = New-Object System.Speech.Synthesis.SpeechSynthesizer
$synth.SelectVoice("Laprdus Josip")
$synth.Speak("Dobar dan!")
```

### NVDA Addon

1. Install the `.nvda-addon` file
2. Restart NVDA
3. Select "Laprdus Croatian/Serbian" in NVDA voice settings

## Android

### Build Architecture

The Android app uses:
- **Hilt** for dependency injection
- **Jetpack Compose** for UI
- **CMake/NDK** for native library build
- **Shared voice data** from `data/voices/`

### Building Android

```bash
# Simply run Gradle - it automatically generates voice data if needed
cd android
./gradlew assembleDebug
```

**Automatic Voice Data Generation**: The Android Gradle build now automatically:
1. Checks if voice data exists in `data/voices/`
2. If missing, runs SCons to generate voice data (requires SCons installed)
3. Fails with a clear error message if voice data cannot be generated
4. Copies voice data and dictionaries to the APK assets

**Manual build** (if automatic generation fails):
```bash
# 1. First, build voice data with SCons (from project root)
scons --platform=windows --arch=x64 --build-config=release voice-data

# 2. Then build Android app (from android/ directory)
cd android
./gradlew assembleDebug
```

### Voice Data Pipeline

```
phonemes/Josip/*.wav  ──┐
                        ├──▶ SCons builds phoneme_packer tool
phonemes/Vlado/*.wav  ──┘
                              │
                              ▼
                        phoneme_packer runs
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

### 16KB Page Size Compatibility

The native library uses `-Wl,-z,max-page-size=16384` linker flag for Android 15+ compatibility with 16KB page size devices (required for Google Play starting November 2025).

## Linux

### Overview

LaprdusTTS supports Linux through:
- **Speech Dispatcher module** (`sd_laprdus`) - Enables integration with Orca screen reader and other SSIP clients
- **Command-line interface** (`laprdus`) - Direct TTS synthesis from terminal

### Building for Linux

```bash
# Build all Linux components
scons --platform=linux --build-config=release linux-all

# Build only the CLI
scons --platform=linux --build-config=release cli

# Build only the Speech Dispatcher module (requires libspeechd-dev)
scons --platform=linux --build-config=release speechd
```

### Linux Dependencies

| Package | Purpose |
|---------|---------|
| `libpulse-dev` | PulseAudio audio output |
| `libasound2-dev` | ALSA audio output (fallback) |
| `libspeechd-dev` | Speech Dispatcher module development |
| `libglib2.0-dev` | GLib (required by Speech Dispatcher) |

Install on Debian/Ubuntu:
```bash
sudo apt install libpulse-dev libasound2-dev libspeechd-dev libglib2.0-dev
```

Install on Fedora/RHEL:
```bash
sudo dnf install pulseaudio-libs-devel alsa-lib-devel speech-dispatcher-devel glib2-devel
```

### Command-Line Interface Usage

```bash
# Basic usage
laprdus "Dobar dan!"

# Select voice
laprdus -v vlado "Zdravo svete!"

# Adjust parameters
laprdus -r 1.5 -p 1.2 -V 0.8 "Brzo i visoko"

# Output to WAV file
laprdus -o output.wav "Text to save"

# Read from file
laprdus -i input.txt -o speech.wav

# Pipe from stdin
echo "Tekst" | laprdus

# List available voices
laprdus -l
```

#### CLI Options

| Option | Long Form | Description |
|--------|-----------|-------------|
| `-v` | `--voice` | Select voice (josip, vlado, detence, baba, djed) |
| `-r` | `--speech-rate` | Speech rate (0.5-2.0, default 1.0) |
| `-p` | `--speech-pitch` | Speech pitch (0.5-2.0, default 1.0) |
| `-V` | `--speech-volume` | Volume (0.0-1.0, default 1.0) |
| `-d` | `--numbers-digits` | Speak numbers as digits |
| `-c` | `--comma-pauses` | Comma pause duration in ms |
| `-e` | `--period-pauses` | Period pause duration in ms |
| `-x` | `--exclamationmark-pauses` | Exclamation pause duration in ms |
| `-q` | `--questionmark-pauses` | Question mark pause duration in ms |
| `-n` | `--newline-pauses` | Newline pause duration in ms |
| `-o` | `--output-file` | Output to WAV file |
| `-i` | `--input-file` | Read text from file |
| `-h` | `--help` | Show help |

### Speech Dispatcher Integration

**Automatic Configuration**: When installing LaprdusTTS via packages (deb, rpm, PKGBUILD) or using `scons install`, Speech Dispatcher is configured automatically. No manual configuration is needed.

The installation:
1. Installs the module binary to `/usr/lib/speech-dispatcher-modules/sd_laprdus`
2. Installs the config to `/etc/speech-dispatcher/modules/laprdus.conf`
3. Automatically adds `AddModule "laprdus"` to `/etc/speech-dispatcher/speechd.conf`
4. Sets LaprdusTTS as the default for Croatian (hr) and Serbian (sr) languages

After installation, restart Speech Dispatcher:
```bash
systemctl --user restart speech-dispatcher
```

Test with spd-say:
```bash
spd-say -o laprdus "Dobar dan!"
spd-say -o laprdus -l hr "Hrvatski tekst"
spd-say -o laprdus -l sr "Srpski tekst"
```

**Manual Configuration** (only if automatic configuration fails):
```
# Add to /etc/speech-dispatcher/speechd.conf:
AddModule "laprdus" "sd_laprdus" "laprdus.conf"
DefaultModule laprdus  # Optional: set as default
```

### Linux Package Locations

| Component | System Path |
|-----------|-------------|
| Library | `/usr/lib/liblaprdus.so` |
| CLI | `/usr/bin/laprdus` |
| Voice data | `/usr/share/laprdus/` |
| Speech Dispatcher module | `/usr/lib/speech-dispatcher-modules/sd_laprdus` |
| Module config | `/etc/speech-dispatcher/modules/laprdus.conf` |

### Linux Installation

#### From packages

```bash
# Debian/Ubuntu
sudo dpkg -i laprdus_1.0.0_amd64.deb
sudo dpkg -i laprdus-speechd_1.0.0_amd64.deb

# Fedora/RHEL
sudo rpm -i laprdus-1.0.0.x86_64.rpm
sudo rpm -i laprdus-speechd-1.0.0.x86_64.rpm

# Arch Linux
makepkg -si  # From PKGBUILD directory
```

#### From tarball

```bash
tar xf laprdus-1.0.0-linux-x86_64.tar.xz
cd laprdus-1.0.0-linux-x86_64
sudo ./install.sh
```

#### From source

```bash
scons --platform=linux --build-config=release linux-all
sudo scons --platform=linux --build-config=release install
```

## Full Rebuild Instructions

When making changes to core engine components, pronunciation dictionary, or voice data, rebuild all platforms using these commands:

### Complete Rebuild (All Platforms)

```bash
# From project root directory

# 1. Build Windows SAPI5 DLLs (both architectures)
scons --platform=windows --arch=x64 --build-config=release sapi5
scons --platform=windows --arch=x86 --build-config=release sapi5

# 2. Build SAPI5 Installer (requires InnoSetup 6+)
iscc installers/windows/laprdus_sapi5.iss

# 3. Build Linux (library, CLI, Speech Dispatcher module)
scons --platform=linux --arch=x64 --build-config=release linux-all

# 4. Build Android APK (requires JDK 17+ and Android SDK)
cd android
./gradlew assembleRelease
# or for debug: ./gradlew assembleDebug
cd ..

# 5. Build NVDA Addon (optional)
cd nvda-addon
scons
cd ..
```

### Output Locations

| Platform | Output File |
|----------|-------------|
| Windows SAPI5 x64 | `build/windows-x64-release/laprd64.dll` |
| Windows SAPI5 x86 | `build/windows-x86-release/laprd32.dll` |
| Windows Installer | `installers/windows/Output/Laprdus_SAPI5_Setup_1.0.0.exe` |
| Linux Library | `build/linux-x64-release/liblaprdus.so` |
| Linux CLI | `build/linux-x64-release/laprdus` |
| Linux Speech Dispatcher | `build/linux-x64-release/sd_laprdus` |
| Linux Voice Data | `build/linux-x64-release/Josip.bin`, `Vlado.bin` |
| Android APK (debug) | `android/app/build/outputs/apk/debug/app-debug.apk` |
| Android APK (release) | `android/app/build/outputs/apk/release/app-release.apk` |
| NVDA Addon | `nvda-addon/laprdus-*.nvda-addon` |

### Dictionary Updates

The pronunciation dictionary at `data/dictionary/internal.json` is automatically included in all builds:
- **Windows SAPI5**: Copied to install directory as `dictionary.json`
- **Android**: Included in APK assets as `internal.json`
- **NVDA Addon**: Copied to addon directory as `internal.json`

The spelling dictionary at `data/dictionary/spelling.json` is also included in all builds:
- **Windows SAPI5**: Copied to install directory as `spelling.json`
- **Android**: Included in APK assets as `spelling.json`
- **NVDA Addon**: Copied to addon directory as `spelling.json`

No manual copying is required - the build systems handle this automatically.

## Claude Code: Post-Change Rebuild Automation

**IMPORTANT**: After completing any major change to the codebase (C++ engine, NVDA addon, dictionaries, or build configuration), Claude Code MUST automatically rebuild and test all platforms before considering the task complete.

### When to Trigger Full Rebuild

Rebuild all platforms after changes to:
- **C++ core engine** (`src/core/`, `src/audio/`, `src/c_api/`)
- **SAPI5 driver** (`src/platform/windows/sapi5/`)
- **NVDA addon** (`nvda-addon/addon/synthDrivers/laprdus/`)
- **Dictionaries** (`data/dictionary/*.json`)
- **Voice data** (`phonemes/`, `data/voices/`)
- **Build configuration** (`SConstruct`, `CMakeLists.txt`, `build.gradle.kts`)
- **Android JNI bridge** (`src/platform/android/`)
- **Linux CLI** (`src/platform/linux/cli/`)
- **Linux Speech Dispatcher module** (`src/platform/linux/speechd/`)

### Automated Rebuild Commands

**RECOMMENDED: Use the master build script** which handles all dependencies automatically:

```bash
# Build all platforms (most comprehensive)
./scripts/build-all.sh all          # Linux/macOS/WSL
scripts\build-all.cmd all           # Windows CMD

# Or build specific platforms
./scripts/build-all.sh sapi5        # Windows SAPI5 + installer
./scripts/build-all.sh nvda         # NVDA addon (builds DLLs first)
./scripts/build-all.sh android      # Android APK
./scripts/build-all.sh linux        # Linux components
```

**Manual build commands** (if master script unavailable):

```bash
# 1. Generate voice data (REQUIRED FIRST STEP)
scons --platform=windows --arch=x64 --build-config=release voice-data

# 2. Build SAPI5 DLLs (both architectures)
scons --platform=windows --arch=x64 --build-config=release sapi5
scons --platform=windows --arch=x86 --build-config=release sapi5

# 3. Build SAPI5 Installer
"/c/Program Files (x86)/Inno Setup 6/ISCC.exe" installers/windows/laprdus_sapi5.iss

# 4. Build NVDA Addon (automatically copies DLLs, voice data, and dictionaries)
cd nvda-addon && scons && cd ..

# 5. Build Linux (library, CLI, Speech Dispatcher module)
scons --platform=linux --arch=x64 --build-config=release linux-all

# 6. Build and run Linux tests
g++ -std=c++17 -I include -I tests/linux tests/linux/test_cli.cpp -o build/linux-x64-release/test_cli -L build/linux-x64-release -llaprdus -lpthread
g++ -std=c++17 -I include -I tests/linux tests/linux/test_speechd_module.cpp -o build/linux-x64-release/test_speechd_module -L build/linux-x64-release -llaprdus -lpthread
LD_LIBRARY_PATH=build/linux-x64-release LAPRDUS_CLI=./build/linux-x64-release/laprdus LAPRDUS_DATA=./build/linux-x64-release ./build/linux-x64-release/test_cli
LD_LIBRARY_PATH=build/linux-x64-release ./build/linux-x64-release/test_speechd_module

# 7. Build Android APK
cd android && export JAVA_HOME="/c/Program Files/Android/Android Studio/jbr" && ./gradlew assembleDebug && cd ..
```

### CRITICAL: NVDA Addon Build Requirements

**ALWAYS use SCons to build the NVDA addon:**
```bash
cd nvda-addon && scons && cd ..
```

**NEVER do ANY of the following - these will corrupt the addon:**
- NEVER manually create the .nvda-addon file using Python's zipfile, 7-Zip, or any archive tool
- NEVER manually edit `addon/manifest.ini` - it is AUTO-GENERATED by SCons from `buildVars.py`
- NEVER manually edit `addon/locale/*/manifest.ini` - these are AUTO-GENERATED from translations
- NEVER manually edit `addon/doc/en/readme.html` - it is AUTO-GENERATED from `readme.md`

**To make changes:**
- Addon metadata (name, version, description): Edit `nvda-addon/buildVars.py`
- Documentation: Edit `nvda-addon/readme.md`
- Translations: Edit `.po` files in `nvda-addon/addon/locale/*/LC_MESSAGES/`

SCons handles all file generation automatically. Manual edits to generated files will be overwritten or cause corruption.

### Automatic Resource Handling

**The build system now automatically handles all resource copying:**

- **Voice Data**: SCons `sapi5`, `linux-all`, and Android targets automatically run `voice-data` to generate and copy `.bin` files to `data/voices/`
- **NVDA Addon**: The NVDA SCons build automatically copies:
  - DLLs from `build/windows-x64-release/laprd64.dll` and `build/windows-x86-release/laprd32.dll`
  - GUI executables from `build/windows-x64-release/laprdgui.exe` (as `laprdgui64.exe`) and `build/windows-x86-release/laprdgui.exe` (as `laprdgui32.exe`)
  - Voice data from `data/voices/`
  - Dictionaries from `data/dictionary/`
- **SAPI5 Installer**: InnoSetup sources voice data from `data/voices/` and dictionaries from `data/dictionary/`
- **Android**: Gradle tasks automatically copy voice data and dictionaries to `assets/` subdirectories

**No manual file copying is required** when using the build scripts or SCons targets correctly.

**Android**: The Android build uses CMake to compile the native library directly from source, so no manual copying is needed - it always uses the latest C++ code.

### Post-Build Testing

After rebuilding, launch installers for user testing:

```bash
# Launch SAPI5 installer
start "" "installers/windows/Output/Laprdus_SAPI5_Setup_1.0.0.exe"

# Launch NVDA addon installer
start "" "nvda-addon/laprdus-1.0.0.nvda-addon"
```

### Android Device Testing

**IMPORTANT**: After building the Android APK, ALWAYS install and test on the user's physical device.

#### ADB Path (Developer Machine)

The Android SDK platform-tools are located at:
```
C:\Users\hrvoj\AppData\Local\Android\Sdk\platform-tools\adb.exe
```

#### Device Testing Commands

```bash
# Check for connected devices
"/c/Users/hrvoj/AppData/Local/Android/Sdk/platform-tools/adb.exe" devices

# Install debug APK on connected device
"/c/Users/hrvoj/AppData/Local/Android/Sdk/platform-tools/adb.exe" install -r "android/app/build/outputs/apk/debug/app-debug.apk"

# Install release APK on connected device
"/c/Users/hrvoj/AppData/Local/Android/Sdk/platform-tools/adb.exe" install -r "android/app/build/outputs/apk/release/app-release.apk"

# Uninstall app from device
"/c/Users/hrvoj/AppData/Local/Android/Sdk/platform-tools/adb.exe" uninstall com.hrvojekatic.laprdus

# View device logs (filter by app)
"/c/Users/hrvoj/AppData/Local/Android/Sdk/platform-tools/adb.exe" logcat -s "Laprdus"
```

#### Complete Android Build and Test Workflow

```bash
# From project root directory

# 1. Build the debug APK
cd android && export JAVA_HOME="/c/Program Files/Android/Android Studio/jbr" && ./gradlew assembleDebug && cd ..

# 2. Install on connected device
"/c/Users/hrvoj/AppData/Local/Android/Sdk/platform-tools/adb.exe" install -r "android/app/build/outputs/apk/debug/app-debug.apk"
```

#### Testing Checklist for Android

- [ ] Device connected and recognized by `adb devices`
- [ ] APK installs successfully
- [ ] App launches without crashes
- [ ] TTS engine appears in Android TTS settings
- [ ] Voice synthesis works correctly
- [ ] Settings screen is accessible with TalkBack
- [ ] All controls (sliders, switches, dropdowns) work with TalkBack

### Linux Testing

#### Build and Test Commands

```bash
# Build Linux components
scons --platform=linux --arch=x64 --build-config=release linux-all

# Compile tests
g++ -std=c++17 -I include -I tests/linux tests/linux/test_cli.cpp \
    -o build/linux-x64-release/test_cli -L build/linux-x64-release -llaprdus -lpthread
g++ -std=c++17 -I include -I tests/linux tests/linux/test_speechd_module.cpp \
    -o build/linux-x64-release/test_speechd_module -L build/linux-x64-release -llaprdus -lpthread

# Run CLI and API tests (should show "20 passed, 0 failed")
LD_LIBRARY_PATH=build/linux-x64-release \
    LAPRDUS_CLI=./build/linux-x64-release/laprdus \
    LAPRDUS_DATA=./build/linux-x64-release \
    ./build/linux-x64-release/test_cli

# Run Speech Dispatcher tests (should show "5 passed, 0 failed")
LD_LIBRARY_PATH=build/linux-x64-release \
    ./build/linux-x64-release/test_speechd_module

# Test CLI manually
LD_LIBRARY_PATH=build/linux-x64-release \
    ./build/linux-x64-release/laprdus -D build/linux-x64-release -o /tmp/test.wav "Dobar dan!"
```

#### Testing Checklist for Linux

- [ ] Linux library (`liblaprdus.so`) builds without errors
- [ ] Linux CLI (`laprdus`) builds without errors
- [ ] CLI tests pass (20/20)
- [ ] Speech Dispatcher tests pass (5/5 mapping tests, integration tests may skip)
- [ ] CLI `-h` displays help correctly
- [ ] CLI `-l` lists all 5 voices
- [ ] CLI synthesizes audio to WAV file correctly
- [ ] All voices synthesize correctly (josip, vlado, detence, baba, djed)

#### Linux Package Testing (if building packages)

```bash
# Test Debian package build (requires dpkg-buildpackage)
cd installers/linux/deb && dpkg-buildpackage -us -uc

# Test Arch package build (requires makepkg)
cd installers/linux/arch && makepkg -s

# Test tarball creation
cd installers/linux/tarball && ./build-tarball.sh
```

#### Building RPM Package (Fedora)

The RPM build requires running on a Fedora system (native or WSL).

**1. Install build dependencies:**
```bash
sudo dnf install -y rpm-build rpmdevtools gcc-c++ scons \
    pulseaudio-libs-devel alsa-lib-devel glib2-devel
```

**2. Set up rpmbuild tree:**
```bash
rpmdev-setuptree
```

**3. Create source tarball and build:**
```bash
# From project root
tar --transform='s,^\.,laprdus-1.0.0,' -czf ~/rpmbuild/SOURCES/laprdus-1.0.0.tar.gz \
    --exclude='.git' --exclude='build' --exclude='*.pyc' --exclude='__pycache__' \
    --exclude='android/.gradle' --exclude='android/app/build' \
    --exclude='nvda-addon/*.nvda-addon' --exclude='.sconsign*' .

# Copy spec and build
cp installers/linux/rpm/laprdus.spec ~/rpmbuild/SPECS/
rpmbuild -ba ~/rpmbuild/SPECS/laprdus.spec
```

**4. Output files:**
```
~/rpmbuild/RPMS/x86_64/laprdus-1.0.0-1.fc*.x86_64.rpm      # Main package
~/rpmbuild/RPMS/x86_64/laprdus-devel-1.0.0-1.fc*.x86_64.rpm # Dev headers
~/rpmbuild/SRPMS/laprdus-1.0.0-1.fc*.src.rpm               # Source RPM
```

**5. Copy to project directory:**
```bash
cp ~/rpmbuild/RPMS/x86_64/laprdus*.rpm ~/rpmbuild/SRPMS/laprdus*.rpm \
    installers/linux/rpm/
```

**6. Install and test:**
```bash
# Install (use rpm directly due to self-dependency on liblaprdus.so)
sudo rpm -ivh --nodeps ~/rpmbuild/RPMS/x86_64/laprdus-1.0.0-1.fc*.x86_64.rpm \
    ~/rpmbuild/RPMS/x86_64/laprdus-devel-1.0.0-1.fc*.x86_64.rpm

# Test
laprdus -l              # List voices
laprdus "Dobar dan"     # Speak text
laprdus -o test.wav "Test"  # Save to file
```

**Note:** The Speech Dispatcher module (`laprdus-speechd`) is not included in the current RPM spec due to header detection issues with SCons. The main TTS library and CLI work correctly.

### Build Verification Checklist

Before marking a task complete, verify:
- [ ] SAPI5 x64 DLL builds without errors
- [ ] SAPI5 x86 DLL builds without errors
- [ ] SAPI5 installer builds successfully
- [ ] NVDA addon builds successfully
- [ ] Linux library builds without errors
- [ ] Linux CLI builds without errors
- [ ] Linux tests pass (25 total: 20 CLI/API + 5 speechd)
- [ ] Android APK builds successfully (warnings OK)
- [ ] Android APK installed on user's physical device
- [ ] Installers launched for user testing

## Pronunciation Dictionary

### Format

The dictionary uses JSON format at `data/dictionary/internal.json`:

```json
{
    "version": "1.0",
    "entries": [
        {
            "grapheme": "Facebook",
            "phoneme": "Fejzbuk",
            "caseSensitive": false,
            "wholeWord": true,
            "comment": "Social media platform"
        }
    ]
}
```

### Dictionary Entry Options

| Field | Type | Description |
|-------|------|-------------|
| `grapheme` | string | Text to match and replace |
| `phoneme` | string | Replacement pronunciation |
| `caseSensitive` | bool | Whether matching is case-sensitive (default: false) |
| `wholeWord` | bool | Whether to match whole words only (default: true) |
| `comment` | string | Optional description (ignored by engine) |

## Spelling Dictionary

The spelling dictionary at `data/dictionary/spelling.json` maps individual characters to their pronunciations for screen reader spelling mode (character-by-character reading).

### Format

```json
{
    "version": "1.0",
    "description": "Character pronunciation dictionary for spelling mode",
    "entries": [
        { "character": "B", "pronunciation": "Be" },
        { "character": "Č", "pronunciation": "Če" },
        { "character": ".", "pronunciation": "točka" }
    ]
}
```

### Character Categories

The spelling dictionary includes:
- **Croatian alphabet**: A-Z plus Č, Ć, Đ, Š, Ž (case-insensitive)
- **Digraphs**: LJ, NJ, DŽ
- **Numbers**: 0-9 (spoken as Croatian words)
- **Punctuation**: Common symbols with Croatian names
- **Special characters**: @, #, $, etc.

### Usage

- **NVDA**: Automatically used in character mode (reading character-by-character)
- **SAPI5**: Used with SPVA_SpellOut action (spell commands)
- **Android**: Available via `synthesizeSpelled()` API

### C++ API

```cpp
// Load spelling dictionary
engine.load_spelling_dictionary("/path/to/spelling.json");

// Synthesize text in spelling mode
SynthesisResult result = engine.synthesize_spelled("ABC");
// Result audio: "A Be Ce"
```

### C API

```c
laprdus_load_spelling_dictionary(handle, "/path/to/spelling.json");
laprdus_synthesize_spelled(handle, "ABC", &samples, &format);
```
