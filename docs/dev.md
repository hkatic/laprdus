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
│   ├── audio/              # Recorded voices: analysis, prosody, PSOLA
│   ├── formant/            # Formant voices: front end, rules, Klatt synthesizer
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
│  - synthesize_clause() → front end, plan_clause(), render()     │
│    (durations, pitch contour, TD-PSOLA over the pitch marks)    │
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
3. **Clause segmentation**: Split text at punctuation (chooses the melody and the pause)
4. **Front end**: `formant::Frontend` → phones with stress, word and syllable indices
5. **Prosody**: durations, closures, word gaps, pitch contour (`concat_prosody.cpp`, `formant_intonation.cpp`)
6. **Rendering**: TD-PSOLA from the pitch marks of the recordings (`psola.cpp`), volume, soft limit
7. **Output**: 16-bit PCM audio @ 22050 Hz mono

---

## 2. Core Engine Components

### 2.1 TTSEngine (`src/core/tts_engine.cpp`)

The main orchestrator class that coordinates the synthesis pipeline.

**Key Methods:**
```cpp
// Initialize with voice data (old voice ids accepted); starts in the voice's native language
bool initialize(const std::string& voice_id, const std::string& data_path);

// Language of any voice: front end rules, lexicon, letter names, number dialect
void set_language(VoiceLanguage language);
VoiceLanguage language() const;

// Main synthesis function
SynthesisResult synthesize(const std::string& text);

// Spelling mode (character-by-character); final_pause = false leaves the
// silence after the last character out and reports its length
SynthesisResult synthesize_spelled(const std::string& text, bool final_pause = true,
                                   uint32_t* final_pause_ms = nullptr);

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

Converts numeric strings to words in the current language: `set_dialect()`
is called by `TTSEngine::set_language()` for every voice (Croatian
*tisuća / milijun / dvjesto*, Serbian *hiljada / milion / dvesta*, Bosnian
*hiljada / milion / dvjesto*; one and two agree with the feminine scale
words: *dvije tisuće*, *dve hiljade*).

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

Manages voice definitions including physical, derived and formant voices.

**Voices and languages are separate.** Every voice speaks every language
(Croatian `hr-HR`, Serbian `sr-RS`, Bosnian `bs-BA`); the registry's
`language` of a voice is only its *native* language, spoken when the host
asks for nothing else. `TTSEngine::set_language()` switches the language of
any voice (letter-to-sound rules, accent lexicon, letter names, number
dialect), `TTSEngine::language()` reports it. The default voice is
`VoiceRegistry::default_voice_id()` = `zvonko`, for every language and
platform. The registry order below is part of the contract: code and saved
settings rely on the index of the first five.

| # | id | display_name | native | base | base_pitch | kind |
|---|----|--------------|--------|------|-----------|------|
| 0 | `josip` | Josip | hr-HR | - | 1.0 | recorded (Josip.bin) |
| 1 | `vlado` | Vlado | sr-RS | - | 1.0 | recorded (Vlado.bin) |
| 2 | `detence` | Detence | hr-HR | josip | 1.5 | derived, child |
| 3 | `baba` | Baba | hr-HR | josip | 1.2 | derived, grandmother (Female, Senior) |
| 4 | `djedo` | Đedo | sr-RS | vlado | 0.75 | derived, grandfather (Male, Senior) |
| 5 | `zvonko` | Zvonko | hr-HR | - | 1.0 | formant (the default voice) |
| 6 | `stojan` | Stojan | sr-RS | - | 1.0 | formant |
| 7 | `mirsad` | Mirsad | bs-BA | - | 1.0 | formant |
| 8 | `orguljas` | Orguljaš - Vila Velebita | hr-HR | zvonko | 1.0 | singing, pipe organ |
| 9 | `dalmacija_klapa` | Dalmacija Klapa | hr-HR | zvonko | 1.0 | singing, klapa (*Vila Velebita*) |
| 10 | `trubac` | Trubač - Kreće se lađa francuska | sr-RS | stojan | 1.0 | singing, trumpet |
| 11 | `harmonikas` | Harmonikaš - Kreće se lađa francuska | sr-RS | stojan | 1.0 | singing, accordion |
| 12 | `sevdalija` | Sevdalija - Kad ja pođoh na Bembašu | bs-BA | mirsad | 1.0 | singing, sevdah singer |
| 13 | `sazlija` | Sazlija - Kad ja pođoh na Bembašu | bs-BA | mirsad | 1.0 | singing, saz |
| 14 | `lika_velebit` | Lika Velebit | hr-HR | zvonko | 1.0 | singing, Zvonko's own voice (*Vila Velebita*) |
| 15 | `solunac` | Solunac | sr-RS | stojan | 1.0 | singing, Stojan's own voice (*Kreće se lađa francuska*) |
| 16 | `sarajevo_bembasa` | Sarajevo Bembaša | bs-BA | mirsad | 1.0 | singing, Mirsad's own voice (*Kad ja pođoh na Bembašu*) |
| 17 | `becarac` | Bećarac | hr-HR | zvonko | 1.0 | singing, Zvonko's own voice, the bećarac tune |

Recorded voices (0-4) need `Josip.bin` / `Vlado.bin` and accept speed and
user pitch 0.5-2.0; formant voices (5-17) need no data file and accept
0.25-4.0. Tell the kinds apart by the fields, not by id sets: `data_filename`
is NULL for formant voices, `base_voice_id` is NULL for josip, vlado,
zvonko, stojan and mirsad; a singing preset has a base voice and no data
file (or simply index >= 8). See `docs/formant.md` for the formant voices
and "Singing presets" there for the presets.

**Ids** are plain ASCII without diacritics on purpose (dj for đ, c for č/ć,
s for š, z for ž), so they register safely on every platform. Display
names come from the registry (UTF-8, no "Laprdus" prefix, no language
suffix); platform UIs must not hard-code them, though they may append a
localized description (child, grandma, formant, singing).

**Aliases.** The ids of earlier versions are accepted by
`VoiceRegistry::find_by_id()`, `laprdus_set_voice()` and
`laprdus_get_voice_info_by_id()` and resolve to the renamed voice;
`laprdus_get_current_voice()` returns the canonical id. UIs that compare a
saved id against the voice list should normalize it first.

| old id | id |
|--------|----|
| `djed` | `djedo` |
| `dijete` | `detence` |
| `baka` | `baba` |
| `klapa` | `dalmacija_klapa` |
| `pjevac` | `lika_velebit` |
| `pevac` | `solunac` |
| `solist` | `sarajevo_bembasa` |

**Voice Info Structure** (C API `LaprdusVoiceInfo`):
```cpp
typedef struct LaprdusVoiceInfo {
    const char* id;              // Plain ASCII: "josip", "djedo", "lika_velebit"
    const char* display_name;    // Shown to the user: "Josip", "Đedo", "Dalmacija Klapa"
    const char* language_code;   // Native language: "hr-HR", "sr-RS" or "bs-BA"
                                 // (every voice speaks all three; see laprdus_set_language)
    uint16_t language_lcid;      // Windows LCID of the native language: 0x041A, 0x081A, 0x141A
    const char* gender;          // "Male" or "Female"
    const char* age;             // "Child", "Adult", or "Senior"
    float base_pitch;            // Base pitch multiplier (1.0 for normal)
    const char* base_voice_id;   // Base voice ID for derived voices (NULL if physical)
    const char* data_filename;   // Phoneme data file (NULL for formant voices)
} LaprdusVoiceInfo;
```

**Languages** (`LaprdusLanguageInfo`, `laprdus_get_language_info()`;
`laprdus::VoiceLanguage` and `voice_language_code/short_code/lcid/english_name/native_name()`
in `types.hpp`; `laprdus::parse_voice_language()` parses tags and names):

| index | code | short | LCID | English | native |
|-------|------|-------|------|---------|--------|
| 0 | hr-HR | hr | 0x041A | Croatian | Hrvatski |
| 1 | sr-RS | sr | 0x081A | Serbian | Srpski |
| 2 | bs-BA | bs | 0x141A | Bosnian | Bosanski |

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

### 3.2 UnitBank (`src/audio/unit_bank.cpp`)

Analysis of every recording when a voice is loaded: DC removal and 2 ms edge
fades, a pitch track (normalised autocorrelation with the voice's median
pitch as prior), one pitch mark per period at the peaks of the low-passed
waveform, marks every 5 ms in unvoiced sound, the sounding part and the
burst onset of a stop. A voice character pitch other than 1.0 resamples the
recordings by `sqrt(pitch)` first (spectrum warp of the derived voices).

### 3.3 Prosody (`src/audio/concat_prosody.cpp`)

Durations by the rules of the formant voices (stress, length, final
lengthening, rate floors), stops never stretched (burst kept, closure takes
or gives the time), 22 ms gaps between words, and the pitch contour from the
shared `formant::Intonation` model: accents on stressed syllables, question
rises, final falls, declination; inflection level and enable, voice character
pitch and user pitch all multiply onto the voice's natural pitch.

### 3.4 PSOLA renderer (`src/audio/psola.cpp`)

Time-domain pitch-synchronous overlap-add: one Hann-windowed period of a
recording per output period, placed at output marks spaced by the target
pitch; periods repeated or skipped for duration; repeated unvoiced frames
played backwards; joins between voiced recordings crossfaded period by period
at the same phase. Rate 0.5x to 4.0x, user pitch 0.5x to 2.0x (the formant
voices take 0.25x to 4.0x for both, times their acceleration setting
(0.5-3.0) for the rate, capped at 8.0x).

See `docs/concatenative.md` for the full description and measurements.

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
LaprdusError laprdus_set_voice(handle, voice_id, data_directory);  // old ids accepted
LaprdusError laprdus_load_dictionary(handle, path);
const char* laprdus_get_default_voice(void);                       // "zvonko"

// Language (every voice speaks every language; see below)
uint32_t laprdus_get_language_count(void);                         // 3
LaprdusError laprdus_get_language_info(index, &info);              // LaprdusLanguageInfo: code, short_code, lcid, english_name, native_name
LaprdusError laprdus_set_language(handle, "sr");                   // "hr", "hr-HR", "hr_HR", "hrv", "sr-Latn-RS", "srp", "bs", "bos", "Croatian", "hrvatski"...
const char* laprdus_get_language(handle);                          // "hr-HR" | "sr-RS" | "bs-BA"

// Synthesis
int32_t laprdus_synthesize(handle, text, &samples, &format);
int32_t laprdus_synthesize_spelled(handle, text, &samples, &format);
int32_t laprdus_synthesize_spelled_ex(handle, text, final_pause, &final_pause_ms, &samples, &format);
void laprdus_free_buffer(samples);

// Configuration
LaprdusError laprdus_set_speed(handle, speed);
float laprdus_get_speed(handle);
LaprdusError laprdus_set_pitch(handle, pitch);
LaprdusError laprdus_set_user_pitch(handle, pitch);
float laprdus_get_user_pitch(handle);
LaprdusError laprdus_set_volume(handle, volume);
// Every voice
LaprdusError laprdus_set_inflection_level(handle, level);   // 0.0 monotone .. 1.0, default 0.5
LaprdusError laprdus_set_acceleration(handle, factor);      // 0.5 .. 3.0, multiplies the speed
float laprdus_get_nominal_wpm(voice_id);                    // words/min at speed 1.0 (Zvonko: 175)
```

**Language.** The voice chooses the sound; the language chooses the text
front end (letter-to-sound rules, accent lexicon, letter names when
spelling) and the number words (*tisuća* / *milijun* for hr, *hiljada* /
*milion* / *dvesta* for sr, *hiljada* / *milion* / *dvjesto* for bs), for
every voice, Josip and Vlado included. A voice speaks its native language
(`LaprdusVoiceInfo.language_code`) until `laprdus_set_language()` asks for
another; the chosen language then sticks across later `laprdus_set_voice()`
calls ("set the language once, switch voices freely"). `NULL` or `""`
returns to the voice's native language, now and after every later voice
change; an unknown tag is `LAPRDUS_ERROR_INVALID_PARAMETER`. Platform code
that bypasses the C API (the Android JNI bridge, the SAPI5 driver) calls
`TTSEngine::set_language(VoiceLanguage)` after `initialize()` /
`initialize_formant()`, which start in the voice's native language.

**Host rate and pitch.** The user's own rate and pitch (`speech.speed`,
`speech.pitch` in `settings.json`, `speed`/`pitch` on Android and Apple) are
the voice's normal. A host asks for its rate and pitch relative to its own
normal (Android `speechRate` 100, SSML prosody 1.0, SAPI5 rate 0, NVDA
slider 50, SSIP 0), so every platform passes `setting × host` to
`laprdus_set_speed()` / `laprdus_set_user_pitch()`, clamped to the voice's
range; with the force flag on (`force.speed`, `force_speed`, ...) it passes
the setting alone. The acceleration then multiplies the rate inside the
engine. Speech Dispatcher has no force flags and reads the setting back
with `laprdus_get_speed()` / `laprdus_get_user_pitch()` after
`laprdus_load_user_config()`. Volume is not scaled: without force the host's
volume is used (Android and Apple: 1.0, the system volume governs).

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
Three voice tokens, one per language, instead of one per Laprdus voice:
- "Laprdus Croatian" - LCID 0x041A (hr-HR)
- "Laprdus Serbian" - LCID 0x081A (sr-RS)
- "Laprdus Bosnian" - LCID 0x141A (bs-BA)

The token sets the language (`TTSEngine::set_language()`); the voice comes
from `settings.json` (`voice.default`, empty = `zvonko`, old ids accepted),
so all three tokens speak with the voice chosen in the Laprdus
Configurator. There is no language key in `settings.json`: the host chooses
the language. The installer removes the per-voice tokens of earlier
versions on install, upgrade and repair and re-registers the three language
tokens.

### 4.3 NVDA Addon (`nvda-addon/`)

Python-based synthesizer driver for NVDA screen reader.

**Key Files:**
- `addon/synthDrivers/laprdus/__init__.py` - SynthDriver implementation
- `addon/synthDrivers/laprdus/_laprdus.py` - ctypes bindings to DLL
- `addon/globalPlugins/laprdus/__init__.py` - NVDA menu integration

**Features:**
- NVDA's rate and pitch sliders scale `speech.speed` and `speech.pitch` from settings.json (unless forced); rate boost multiplies every slider position by 3 through the engine's acceleration (`speech.acceleration` is not used), as eSpeak's and OneCore's boost does
- NVDA's inflection slider sets the inflection level (0-100 → 0.0-1.0) and keeps inflection enabled; `speech.inflection` and `speech.inflection_level` from settings.json are not used under NVDA
- A separate **Language** setting (NVDA's standard `LanguageSetting`: hr, sr, bs; default from NVDA's UI language, else hr); the voice list shows all 18 voices by registry `display_name` for any language, and the default voice is `zvonko`. NVDA's automatic language switching (`LangChangeCommand`) switches among hr/sr/bs and ignores other languages
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
LanguageDefaultModule "bs" "laprdus"
```

**Voices and languages:** `LIST VOICES` returns every voice for every
language (hr, sr, bs); `SET LANGUAGE` calls `laprdus_set_language()` and
keeps the voice (`spd-say -l sr -y "Zvonko"`: Zvonko speaking Serbian).
The symbolic voices are the same in all three languages: MALE1 = `zvonko`,
MALE2 = `josip`, MALE3 = `djedo`, FEMALE1 = `baba`, CHILD_MALE = `detence`.
SSIP `pitch_range` maps onto the inflection level.

### 4.5 Android (`android/` and `src/platform/android/`)

Native library + Kotlin TTS Service.

**JNI Bridge (`src/platform/android/jni_bridge.cpp`):**
- One native engine per `LaprdusTTS` object, each with its own mutex: `nativeCreate()` returns a handle that every other call takes as its first argument (a handle, not a field read from `thiz`, so R8 renaming cannot break it). The engine exists from creation, so settings applied before the first voice are kept
- Two engines in one process: `LaprdusTTS.service` (the TTS service: TalkBack and other apps) and `LaprdusTTS.app` (the app's preview and settings, provided by Hilt), so a voice, rate or dictionary set by one never reaches the other
- Asset manager integration for voice data
- Proper JNI type conversions

**Keeping the two engines in step:** the service follows the app through the
saved settings and the dictionary files, not through the engine: a newly
saved default voice is loaded at once, the user dictionaries switch and every
dictionary write (`DictionaryRepository.changes`, bumped in the process on each
save, delete or migration) reload the dictionaries.
`LaprdusTTS.setVoice()` and `reloadDictionaries()` load the bundled
dictionaries and then the user's from `userDictionaryDir`
(`UserDictionaries.load()`), inside `exclusive { }`, which is also where the
service sets a request's rate and pitch and synthesizes.

**TTS Service (`android/app/.../LaprdusTTSService.kt`):**
- Implements `TextToSpeechService`
- Hilt dependency injection
- Jetpack Compose settings UI
- Offers every voice for every locale: 54 system voices named `<id>-<lang>` (`zvonko-sr`). The requested locale sets the engine language (`TTSEngine::set_language()` through the JNI bridge), the voice stays the one configured in the app, and the default voice is `zvonko`. The app has a Language setting for its in-app playback; "Force language" means: ignore the requested language and speak the one chosen in Laprdus settings

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
$synth.SelectVoice("Laprdus Croatian")   # or "Laprdus Serbian", "Laprdus Bosnian"
$synth.Speak("Dobar dan!")
```

**NVDA:**
1. Install `.nvda-addon` file
2. Restart NVDA
3. Select "Laprdus" in voice settings, a voice in Voice and a language in Language
4. Test synthesis and spelling mode

**Linux CLI:**
```bash
laprdus "Dobar dan!"                       # Zvonko, Croatian
laprdus -v vlado -r 1.5 "Zdravo svete!"    # Vlado in its native Serbian
laprdus -v vlado -L hr "Dobar dan svima!"  # Vlado speaking Croatian
laprdus -L bs "Dobar dan!"                 # Zvonko speaking Bosnian
laprdus -l                                 # every voice with its native language
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
