# LaprdusTTS Developer Documentation

This document provides comprehensive technical documentation for developers working on the LaprdusTTS codebase.

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [Core Engine Components](#2-core-engine-components)
3. [Audio Processing Pipeline](#3-audio-processing-pipeline)
4. [Platform Integration](#4-platform-integration)
5. [Build System](#5-build-system)
6. [Testing](#6-testing)
7. [Contributing](#7-contributing)

---

## 1. Architecture Overview

### 1.1 Project Structure

```
LaprdusTTS/
├── src/                    # Core C++ source code
│   ├── core/               # TTS engine, phoneme mapping, numbers
│   ├── audio/              # Audio synthesis, Sonic library
│   ├── c_api/              # Public C API
│   └── platform/           # Platform-specific code
│       ├── windows/        # SAPI5, CLI, config GUI
│       ├── linux/          # CLI, Speech Dispatcher
│       └── android/        # JNI bridge
├── include/                # Public header files
│   └── laprdus/
│       ├── laprdus_api.h   # C API declarations
│       ├── types.hpp       # Type definitions
│       └── laprdus.hpp     # C++ header (optional)
├── nvda-addon/             # NVDA screen reader addon
├── android/                # Android app (Kotlin + CMake)
├── installers/             # Platform installers
│   └── linux/              # deb, rpm, PKGBUILD, tarball
│   └── windows/            # InnoSetup
├── data/                   # Runtime data
│   ├── voices/             # Generated voice .bin files
│   └── dictionary/         # Pronunciation dictionaries
├── phonemes/               # Source phoneme WAV files
│   ├── Josip/              # Croatian voice phonemes
│   └── Vlado/              # Serbian voice phonemes
└── tools/                  # Build tools (phoneme_packer)
```

### 1.2 Component Relationships

```
┌─────────────────────────────────────────────────────────────────┐
│                        Applications                             │
├─────────────┬─────────────┬─────────────┬─────────────┬────────┤
│  SAPI5      │   NVDA      │   Linux     │  Android    │  CLI   │
│  Driver     │   Addon     │   SpeechD   │  Service    │        │
└──────┬──────┴──────┬──────┴──────┬──────┴──────┬──────┴────┬───┘
       │             │             │             │           │
       ▼             ▼             ▼             ▼           ▼
┌─────────────────────────────────────────────────────────────────┐
│                        C API Layer                              │
│                     (laprdus_api.h)                             │
└──────────────────────────┬──────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                      TTSEngine                                  │
│  - preprocess_text() → number expansion, dictionary lookup      │
│  - synthesize() → full text-to-speech pipeline                  │
│  - synthesize_spelled() → character-by-character spelling       │
└─────────────┬───────────────────────────────────────┬───────────┘
              │                                       │
              ▼                                       ▼
┌─────────────────────────┐         ┌─────────────────────────────┐
│     PhonemeMapper       │         │    InflectionProcessor      │
│  - map_text()           │         │  - analyze_text()           │
│  - utf8_to_utf32()      │         │  - apply_inflection()       │
│  - cyrillic_to_latin()  │         │  - segment by punctuation   │
└───────────┬─────────────┘         └─────────────────────────────┘
            │
            ▼
┌─────────────────────────────────────────────────────────────────┐
│                    AudioSynthesizer                             │
│  - synthesize() → concatenate phonemes with crossfade           │
│  - apply_params() → volume, rate (Sonic), pitch (Sonic/formant) │
└─────────────┬───────────────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      PhonemeData                                │
│  - load_from_file() / load_from_memory()                        │
│  - get_phoneme_samples() → return WAV data for phoneme          │
└─────────────────────────────────────────────────────────────────┘
```

### 1.3 Data Flow

1. **Input**: UTF-8 text string
2. **Preprocessing**: Number expansion, dictionary lookup, emoji replacement
3. **Inflection Analysis**: Segment text by punctuation, assign pitch modulation
4. **Phoneme Mapping**: Convert text to phoneme tokens (A-Z + Croatian special chars)
5. **Audio Synthesis**: Concatenate phoneme WAV samples with 64-sample crossfade
6. **Audio Processing**: Apply volume, rate (Sonic time-stretching), pitch (Sonic/formant)
7. **Output**: 16-bit PCM audio @ 22050 Hz mono

---

## 2. Core Engine Components

### 2.1 TTSEngine (`src/core/tts_engine.cpp`)

The main orchestrator class that coordinates the synthesis pipeline.

**Key Methods:**
```cpp
// Initialize with voice data
bool initialize(const std::string& voice_id, const std::string& data_path);

// Main synthesis function
SynthesisResult synthesize(const std::string& text);

// Spelling mode (character-by-character)
SynthesisResult synthesize_spelled(const std::string& text);

// Text preprocessing
std::string preprocess_text(const std::string& text);
```

**Internal Components:**
- `PhonemeData` - Loads and provides access to phoneme audio samples
- `PhonemeMapper` - Converts text to phoneme sequences
- `AudioSynthesizer` - Concatenates phonemes into audio
- `InflectionProcessor` - Applies pitch modulation based on punctuation
- `PronunciationDictionary` - Word/phrase replacements
- `SpellingDictionary` - Character-to-pronunciation mapping
- `EmojiDictionary` - Emoji-to-text conversion

**Thread Safety:**
TTSEngine is NOT thread-safe by design. Create one instance per thread or use external synchronization. This is documented and intentional for performance.

### 2.2 PhonemeMapper (`src/core/phoneme_mapper.cpp`)

Converts UTF-8 text to phoneme token sequences.

**Croatian Alphabet Support:**
- Standard letters: A-Z
- Special characters: Č, Ć, Đ, Š, Ž
- Digraphs: LJ, NJ, DŽ (treated as single phonemes)

**Key Functions:**
```cpp
// Map text to phoneme sequence
std::vector<Phoneme> map_text(const std::string& text);

// UTF-8 to UTF-32 conversion
std::u32string utf8_to_utf32(const std::string& str);

// Serbian Cyrillic to Latin conversion
std::string cyrillic_to_latin(const std::string& text);
```

**Phoneme Enum (`include/laprdus/types.hpp`):**
```cpp
enum class Phoneme : uint8_t {
    A = 0, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, R, S, T, U, V, Z,
    CH,    // Č
    CHj,   // Ć
    DH,    // Đ
    SH,    // Š
    ZH,    // Ž
    LJ,    // LJ digraph
    NJ,    // NJ digraph
    DZH,   // DŽ digraph
    SILENCE,
    COUNT
};
```

### 2.3 CroatianNumbers (`src/core/croatian_numbers.cpp`)

Converts numeric strings to Croatian words.

**Features:**
- Supports numbers from 0 to 10^303 (centillions)
- Proper Croatian grammar for gender and plural forms
- String-based processing to prevent numeric overflow
- Supports both "words" mode and "digits" mode

**Example:**
```
123 → "sto dvadeset tri"
2025 → "dvije tisuće dvadeset pet"
1000000 → "jedan milijun"
```

### 2.4 InflectionProcessor (`src/core/inflection.cpp`)

Applies natural-sounding pitch variation based on punctuation.

**Punctuation Effects:**
- Period (.) → falling pitch, longer pause
- Comma (,) → slight rising pitch, short pause
- Question mark (?) → rising pitch at end
- Exclamation mark (!) → emphasis, longer pause
- Newline → configurable pause

**Configuration:**
```cpp
struct PauseSettings {
    uint32_t sentence_pause_ms = 100;  // After . ! ?
    uint32_t comma_pause_ms = 100;     // After ,
    uint32_t newline_pause_ms = 100;   // After \n
};
```

### 2.5 VoiceRegistry (`src/core/voice_registry.cpp`)

Manages voice definitions including physical and derived voices.

**Voice Types:**
1. **Physical voices** - Have their own phoneme data files
   - `josip` - Croatian male adult (Josip.bin)
   - `vlado` - Serbian male adult (Vlado.bin)

2. **Derived voices** - Use base voice data with pitch modification
   - `detence` - Child voice (base: josip, pitch: 1.5)
   - `baba` - Grandmother voice (base: josip, pitch: 1.2)
   - `djed` - Grandfather voice (base: vlado, pitch: 0.75)

**Voice Info Structure:**
```cpp
struct VoiceInfo {
    std::string id;           // "josip", "vlado", etc.
    std::string display_name; // "Laprdus Josip (Croatian)"
    std::string language;     // "hr-HR" or "sr-RS"
    uint16_t lcid;            // Windows LCID
    std::string gender;       // "Male" or "Female"
    std::string age;          // "Child", "Adult", "Senior"
    float base_pitch;         // Pitch multiplier (1.0 = normal)
    std::string base_voice;   // Base voice ID for derived voices
    std::string data_file;    // Phoneme data filename
};
```

### 2.6 Dictionary System (`src/core/`)

Three dictionary types for text transformation:

**Pronunciation Dictionary (`data/dictionary/internal.json`):**
```json
{
  "version": "1.0",
  "entries": [
    {
      "grapheme": "Facebook",
      "phoneme": "Fejzbuk",
      "caseSensitive": false,
      "wholeWord": true
    }
  ]
}
```

**Spelling Dictionary (`data/dictionary/spelling.json`):**
```json
{
  "version": "1.0",
  "entries": [
    { "character": "A", "pronunciation": "A" },
    { "character": "B", "pronunciation": "Be" },
    { "character": "Č", "pronunciation": "Če" }
  ]
}
```

**Emoji Dictionary (`data/dictionary/emoji.json`):**
```json
{
  "version": "1.0",
  "entries": [
    { "emoji": "😀", "text": "nasmijano lice" },
    { "emoji": "👍", "text": "palac gore" }
  ]
}
```

---

## 3. Audio Processing Pipeline

### 3.1 PhonemeData (`src/audio/phoneme_data.cpp`)

Manages loading and access to phoneme audio samples.

**Binary Format (64-byte header + 32-byte entries):**
```cpp
struct PhonemeFileHeader {
    uint32_t magic;           // 'LPRD'
    uint32_t version;         // 1
    uint32_t phoneme_count;   // Number of phonemes
    uint32_t total_size;      // Total file size
    uint32_t sample_rate;     // 22050
    uint16_t bits_per_sample; // 16
    uint16_t channels;        // 1 (mono)
    uint8_t reserved[44];     // Padding to 64 bytes
};

struct PhonemeEntry {
    uint32_t phoneme_id;      // Phoneme enum value
    uint32_t data_offset;     // Offset in file
    uint32_t data_size;       // Size in bytes
    uint8_t reserved[20];     // Padding to 32 bytes
};
```

**Audio Properties:**
- Sample rate: 22050 Hz
- Bit depth: 16-bit signed PCM
- Channels: 1 (mono)
- Phoneme truncation: L, M, N, S, SH, V, Z, ZH capped at 2000 bytes

### 3.2 AudioSynthesizer (`src/audio/audio_synthesizer.cpp`)

Concatenates phoneme samples and applies audio processing.

**Crossfade Algorithm:**
- 64 samples overlap (~3ms at 22050 Hz)
- Linear crossfade between adjacent phonemes
- Prevents clicks/pops at phoneme boundaries

```cpp
// Crossfade calculation
for (size_t i = 0; i < overlap_samples; i++) {
    float t = (float)i / overlap_samples;
    float blended = prev_sample * (1.0f - t) + curr_sample * t;
    // Clamp to 16-bit range
    blended = std::clamp(blended, -32768.0f, 32767.0f);
}
```

### 3.3 SonicProcessor (`src/audio/sonic_processor.cpp`)

Wrapper around the Sonic library for rate and pitch control.

**Rate Control (Time-Stretching):**
- Uses PICOLA algorithm
- Changes speed WITHOUT changing pitch
- Range: 0.5x to 4.0x (with rate boost)

**Pitch Control (Pitch-Shifting with Formant Shift):**
- Changes pitch AND formants (chipmunk effect)
- Used for derived voices (child, grandma, grandpa)
- Range: 0.25x to 4.0x

### 3.4 FormantPitch (`src/audio/formant_pitch.cpp`)

User pitch preference with formant preservation.

**Purpose:**
- Adjust pitch WITHOUT changing voice character
- For user-controlled pitch slider in SAPI5/NVDA
- Range: 0.5x to 2.0x

**Current Implementation:**
Uses Sonic as placeholder. Architecture supports future STFT-based formant preservation when C++20 compatibility allows (stftPitchShift with cepstral analysis).

---

## 4. Platform Integration

### 4.1 C API (`src/c_api/laprdus_api.cpp`)

Clean C interface for all platforms.

**Key Functions:**
```c
// Lifecycle
LaprdusHandle laprdus_create(void);
void laprdus_destroy(LaprdusHandle handle);

// Initialization
LaprdusError laprdus_set_voice(handle, voice_id, data_directory);
LaprdusError laprdus_load_dictionary(handle, path);

// Synthesis
int32_t laprdus_synthesize(handle, text, &samples, &format);
int32_t laprdus_synthesize_spelled(handle, text, &samples, &format);
void laprdus_free_buffer(samples);

// Configuration
LaprdusError laprdus_set_speed(handle, speed);
LaprdusError laprdus_set_pitch(handle, pitch);
LaprdusError laprdus_set_user_pitch(handle, pitch);
LaprdusError laprdus_set_volume(handle, volume);
```

**Thread Safety:**
- Error messages use thread-local storage with mutex protection
- Each handle should be used from single thread

### 4.2 Windows SAPI5 (`src/platform/windows/sapi5/`)

COM-based Speech API 5 driver.

**Files:**
- `sapi_driver.cpp` - ISpTTSEngine implementation
- `laprdus_sapi.idl` - Interface definition
- `laprdus_sapi.rgs` - Registry script

**Registration:**
- 32-bit: `laprd32.dll` → HKLM\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices
- 64-bit: `laprd64.dll` → HKLM\SOFTWARE\Microsoft\Speech\Voices
- Single CLSID with registry virtualization

**Voice Enumeration:**
All 5 voices registered with appropriate LCIDs:
- Croatian: 0x041A (hr-HR)
- Serbian: 0x081A (sr-RS)

### 4.3 NVDA Addon (`nvda-addon/`)

Python-based synthesizer driver for NVDA screen reader.

**Key Files:**
- `addon/synthDrivers/laprdus/__init__.py` - SynthDriver implementation
- `addon/synthDrivers/laprdus/_laprdus.py` - ctypes bindings to DLL
- `addon/globalPlugins/laprdus/__init__.py` - NVDA menu integration

**Features:**
- Rate boost (extends max rate from 2x to 4x)
- Character mode for spelling
- Shared settings with SAPI5 via settings.json
- Croatian/Serbian translations

**NVDA API Compatibility:**
- Minimum: NVDA 2019.3
- Maximum tested: NVDA 2025.3
- Uses `synthDriverHandler.SynthDriver` base class

### 4.4 Linux Speech Dispatcher (`src/platform/linux/speechd/`)

Module for Speech Dispatcher (used by Orca screen reader).

**Files:**
- `sd_laprdus.c` - Speech Dispatcher module
- `laprdus.conf` - Module configuration

**Configuration:**
Automatically added to `/etc/speech-dispatcher/speechd.conf`:
```
AddModule "laprdus" "sd_laprdus" "laprdus.conf"
LanguageDefaultModule "hr" "laprdus"
LanguageDefaultModule "sr" "laprdus"
```

### 4.5 Android (`android/` and `src/platform/android/`)

Native library + Kotlin TTS Service.

**JNI Bridge (`src/platform/android/jni_bridge.cpp`):**
- Thread-safe global engine with mutex
- Asset manager integration for voice data
- Proper JNI type conversions

**TTS Service (`android/app/.../LaprdusTTSService.kt`):**
- Implements `TextToSpeechService`
- Hilt dependency injection
- Jetpack Compose settings UI

**Build Requirements:**
- NDK with CMake
- 16KB page size alignment (Android 15+)
- API level 24-36 support

---

## 5. Build System

### 5.1 SCons Configuration (`SConstruct`)

Python-based build system with multi-platform support.

**Platforms:**
- `windows` - MSVC compiler
- `linux` - GCC compiler
- `android` - NDK toolchain

**Architectures:**
- `x64`, `x86` (Windows/Linux)
- `arm64`, `arm` (Android)

**Build Configurations:**
- `release` - Optimized, no debug symbols
- `debug` - Debug symbols, no optimization

### 5.2 Build Targets

| Target | Description |
|--------|-------------|
| (default) | Build library + phoneme data |
| `sapi5` | Windows SAPI5 DLL |
| `cli` | Command-line interface |
| `config` | Windows configuration GUI |
| `speechd` | Linux Speech Dispatcher module |
| `linux-all` | All Linux targets |
| `voice-data` | Generate voice .bin files |
| `install` | Linux installation |

### 5.3 Build Order

1. **phoneme_packer** tool is built first
2. **Voice data** generated from `phonemes/*/` directories
3. **Voice data copied** to `data/voices/` for all platforms
4. **Platform-specific builds** proceed with dependencies

### 5.4 Building

**Windows:**
```bash
# SAPI5 DLLs
scons --platform=windows --arch=x64 --build-config=release sapi5
scons --platform=windows --arch=x86 --build-config=release sapi5

# Installer (requires InnoSetup)
iscc installers/windows/laprdus_sapi5.iss

# NVDA Addon
cd nvda-addon && scons
```

**Linux:**
```bash
scons --platform=linux --arch=x64 --build-config=release linux-all
```

**Android:**
```bash
cd android
./gradlew assembleRelease
```

---

## 6. Testing

### 6.1 Linux Tests

**CLI Tests (`tests/linux/test_cli.cpp`):**
- 20 tests covering CLI and API functionality
- Voice selection, synthesis, file output

**Speech Dispatcher Tests (`tests/linux/test_speechd_module.cpp`):**
- 5 tests for module parameter mapping

**Running Tests:**
```bash
# Build and run
scons --platform=linux --arch=x64 --build-config=release linux-all

# Compile tests
g++ -std=c++17 -I include tests/linux/test_cli.cpp \
    -o build/linux-x64-release/test_cli \
    -L build/linux-x64-release -llaprdus -lpthread

# Run
LD_LIBRARY_PATH=build/linux-x64-release \
    LAPRDUS_CLI=./build/linux-x64-release/laprdus \
    LAPRDUS_DATA=./build/linux-x64-release \
    ./build/linux-x64-release/test_cli
```

### 6.2 Manual Verification

**Windows SAPI5:**
```powershell
Add-Type -AssemblyName System.Speech
$synth = New-Object System.Speech.Synthesis.SpeechSynthesizer
$synth.SelectVoice("Laprdus Josip")
$synth.Speak("Dobar dan!")
```

**NVDA:**
1. Install `.nvda-addon` file
2. Restart NVDA
3. Select "Laprdus" in voice settings
4. Test synthesis and spelling mode

**Linux CLI:**
```bash
laprdus "Dobar dan!"
laprdus -v vlado -r 1.5 "Zdravo svete!"
laprdus -o output.wav "Test"
```

---

## 7. Contributing

### 7.1 Code Style

**C++ (C++17):**
- 4-space indentation
- `snake_case` for functions and variables
- `PascalCase` for classes and types
- Braces on same line
- Use `std::unique_ptr` for ownership

**Python (Python 3):**
- Tab indentation (NVDA convention)
- Follow existing code style
- Use type hints where possible

### 7.2 Pull Request Process

1. Fork the repository
2. Create feature branch
3. Make changes with tests
4. Run all builds and tests
5. Submit PR with description

### 7.3 Translation Workflow

**NVDA Addon:**
1. Edit `.po` files in `nvda-addon/addon/locale/<lang>/LC_MESSAGES/`
2. SCons compiles to `.mo` automatically

**Supported Languages:**
- Croatian (hr)
- Serbian (sr)

---

## Appendix: Error Codes

| Code | Name | Description |
|------|------|-------------|
| 0 | LAPRDUS_OK | Success |
| -1 | LAPRDUS_ERROR_INVALID_HANDLE | Invalid engine handle |
| -2 | LAPRDUS_ERROR_NOT_INITIALIZED | Engine not initialized |
| -3 | LAPRDUS_ERROR_INVALID_PATH | Invalid file path |
| -4 | LAPRDUS_ERROR_LOAD_FAILED | Failed to load resource |
| -5 | LAPRDUS_ERROR_SYNTHESIS_FAILED | Synthesis failed |
| -6 | LAPRDUS_ERROR_OUT_OF_MEMORY | Memory allocation failed |
| -7 | LAPRDUS_ERROR_CANCELLED | Operation cancelled |
| -8 | LAPRDUS_ERROR_INVALID_PARAMETER | Invalid parameter value |
| -9 | LAPRDUS_ERROR_DECRYPTION_FAILED | Decryption failed |
| -10 | LAPRDUS_ERROR_FILE_NOT_FOUND | File not found |
| -11 | LAPRDUS_ERROR_INVALID_FORMAT | Invalid file format |
