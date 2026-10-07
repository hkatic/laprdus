# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

LaprdusTTS is a Croatian/Serbian/Bosnian text-to-speech (TTS) engine supporting:
- **SAPI5** (Windows Speech API) for system-wide TTS integration
- **NVDA** screen reader synthesizer driver

It has two kinds of voices:
- **Concatenative** voices (Josip, Vlado and the voices derived from them) are rendered from pre-recorded phoneme WAV files by TD-PSOLA. See "Recorded Voices" below and `docs/concatenative.md`.
- **Formant** voices (Zvonko, Stojan, Mirsad) are synthesized entirely by rule and need no data files. See "Formant Voices" below.

## Architecture

### Build System
- **SCons** (Python-based) - see `SConstruct`
- Platforms: Windows, Linux, macOS, Android
- Architectures: x64, x86, ARM64, ARM
- Build: `scons --platform=windows --arch=x64 --build-config=release sapi5`

### Core C++ Components

| File | Purpose |
|------|---------|
| `src/core/tts_engine.cpp` | Main engine orchestrating synthesis pipeline |
| `src/core/phoneme_mapper.cpp` | UTF-8 text → phoneme token conversion |
| `src/core/croatian_numbers.cpp` | Number-to-words (supports up to centillions) |
| `src/core/inflection.cpp` | Clause segmentation at punctuation, pause settings |
| `src/core/voice_registry.cpp` | Voice definitions (Josip, Vlado, derived voices) |
| `src/audio/audio_synthesizer.cpp` | Recorded voices: clause synthesis (front end → planner → renderer) |
| `src/audio/unit_bank.cpp` | Recorded voices: analysis of the recordings (pitch marks, voicing, bursts) |
| `src/audio/concat_prosody.cpp` | Recorded voices: durations, closures, word gaps, pitch contour |
| `src/audio/psola.cpp` | Recorded voices: TD-PSOLA renderer |
| `src/audio/phoneme_data.cpp` | WAV file loading from .bin or directory |
| `src/c_api/laprdus_api.cpp` | C API for external consumers |
| `src/formant/formant_frontend.cpp` | Formant voices: text → phones (letter-to-sound, stress, clitics, assimilation) |
| `src/formant/formant_lexicon.cpp` | Formant voices: built-in accent lexicon |
| `src/formant/formant_intonation.cpp` | Intonation model (accents, boundary movements, declination) shared by both kinds of voices |
| `src/formant/formant_phonemes.cpp` | Formant voices: acoustic targets of every phone |
| `src/formant/formant_synthesizer.cpp` | Formant voices: durations, formant tracks, intonation |
| `src/formant/klatt_synth.cpp` | Formant voices: cascade/parallel (Klatt) synthesizer |

### Platform-Specific Code

| Path | Purpose |
|------|---------|
| `src/platform/windows/sapi5/` | SAPI5 COM driver (laprd32.dll, laprd64.dll) |
| `src/platform/linux/speechd/` | Speech Dispatcher module (sd_laprdus) |
| `src/platform/linux/cli/` | Linux command-line utility |
| `nvda-addon/` | NVDA Python synthesizer driver |

### Processing Pipeline

1. Text → `TTSEngine::preprocess_text()` → Number expansion, dictionaries
2. → `InflectionProcessor::analyze_text()` → Segment into clauses by punctuation (a mark glued to the next character, as in `datoteka.txt`, `3.14`, `12:30`, is not a clause end: it stays in the text and is read by name by the front end's `SYMBOLS` table; the number converter reads the decimal comma as *zarez*, and clock times `12:30` and dates `7.10.2026` without the separators, the day and month as ordinals)
3. → `formant::Frontend::process()` → phones with stress, words, clause kind (both kinds of voices)
4. → Recorded voices: `concat::plan_clause()` (durations, pitch contour) → `concat::render()` (TD-PSOLA)
   Formant voices: `ClauseBuilder` → `KlattSynth`
5. → Output 16-bit PCM @ 22050Hz mono; rate, pitch, volume and intonation are applied at the source by both

### Phoneme System

- **Format**: 16-bit PCM WAV, 22050 Hz, mono
- **Storage**: Packed `.bin` files (Josip.bin, Vlado.bin) or raw WAV directory
- **Croatian phonemes**: A-Z + č, ć, đ, š, ž, lj, nj, dž
- **Analysis at load**: DC removal, pitch marks, voicing, burst onset per recording (`unit_bank.cpp`)
- **Joins**: pitch-synchronous crossfade of whole periods (no fixed crossfade, no truncation)

### Voice System

| Voice | Type | Language | base_pitch |
|-------|------|----------|------------|
| josip | Physical | Croatian | 1.0 |
| vlado | Physical | Serbian | 1.0 |
| detence | Derived (josip) | Croatian | 1.5 (child) |
| baba | Derived (josip) | Croatian | 1.2 (grandma) |
| djed | Derived (vlado) | Serbian | 0.75 (grandpa, Đedo) |
| zvonko | Formant | Croatian | 1.0 |
| stojan | Formant | Serbian | 1.0 |
| mirsad | Formant | Bosnian | 1.0 |
| orguljas | Singing preset (zvonko) | Croatian | 1.0 (pipe organ) |
| klapa | Singing preset (zvonko) | Croatian | 1.0 (klapa choir) |
| trubac | Singing preset (stojan) | Serbian | 1.0 (trumpet) |
| harmonikas | Singing preset (stojan) | Serbian | 1.0 (accordion) |
| sevdalija | Singing preset (mirsad) | Bosnian | 1.0 (sevdah singer) |
| sazlija | Singing preset (mirsad) | Bosnian | 1.0 (saz) |
| pjevac | Singing preset (zvonko) | Croatian | 1.0 (own voice, own pitch) |
| pevac | Singing preset (stojan) | Serbian | 1.0 (own voice, own pitch) |
| solist | Singing preset (mirsad) | Bosnian | 1.0 (own voice, own pitch) |
| becarac | Singing preset (zvonko) | Croatian | 1.0 (own voice, bećarac tune) |

**Default voices.** The formant voice of a language is its default on every platform: Zvonko for Croatian, Stojan for Serbian, Mirsad for Bosnian (CLI `-v` default, Speech Dispatcher `MALE1` and language requests, Android `DEFAULT_VOICE` / `onLoadLanguage` / fallback voice, NVDA by NVDA's language, Apple by the device's preferred language, Windows config GUI). A voice the user has chosen is never replaced when it already speaks the requested language. The recorded voices stay available. SAPI5 has no Laprdus-side default: Windows picks among the registered tokens.

`laprdus_set_voice()` applies the voice's base pitch itself (times the last `laprdus_set_pitch()` value), so derived voices sound right without a pitch call after every voice change.

`VoiceDefinition::synthesis` (`VoiceSynthesis::Concatenative` / `Formant`) tells the two kinds apart. Formant voices have no `data_filename`; `laprdus_set_voice()` accepts any data directory for them. New voices must be appended to the registry: existing code and saved settings rely on the order of the first five.

### Audio Parameters (types.hpp)

```cpp
struct VoiceParams {
    float speed = 1.0f;       // 0.5 - 2.0 (tempo, time-stretching); formant voices 0.25 - 4.0
    float pitch = 1.0f;       // 0.25 - 4.0 (voice character pitch)
    float user_pitch = 1.0f;  // 0.5 - 2.0 (user pitch preference); formant voices 0.25 - 4.0
    float volume = 1.0f;      // 0.0 - 1.0
    bool inflection_enabled = true;
    float inflection_level = 0.5f;  // formant voices: 0 monotone, 0.5 as measured, 1 twice the movements
    float acceleration = 1.0f;      // formant voices: rate multiplier 0.5 - 3.0
};
```

## Formant Voices

Zvonko (hr), Stojan (sr) and Mirsad (bs) live in `src/formant/` and share the text preprocessing of the concatenative voices (emoji and pronunciation dictionaries, number expansion, clause segmentation). From there `TTSEngine::synthesize_segments()` hands each clause to `FormantSynthesizer::synthesize_clause()`:

1. `Frontend::process()` - Cyrillic → Latin, digraphs (lj, nj, dž), the ijekavian *ije* diphthong, syllabic r, spelled-out abbreviations; stress from accent marks in the text, the lexicon, suffix rules, or the first syllable; proclitics/enclitics; voicing and place assimilation.
2. `ClauseBuilder` - segment durations, formant targets and locus-based transitions, source amplitudes, and the F0 contour (word accents, declination, boundary movement chosen by the clause's punctuation).
3. `KlattSynth::render()` - glottal source, cascade formants for voiced sound and aspiration, parallel resonators for frication and bursts.

Things to know before changing them:

- **Speed, pitch and volume are applied at the source** (durations, F0, gain), so `VoiceParams::pitch` and `user_pitch` both simply scale F0.
- **Wider ranges and two settings of their own.** `VoiceParams::clamp()` allows speed and `user_pitch` from 0.25 to 4.0; the concatenative `AudioSynthesizer` narrows them to its 0.5-4.0 / 0.5-2.0 itself, so every platform may pass the wide range and let the engine sort it out. `inflection_level` (0-1, default 0.5) scales the whole F0 contour in semitones (0 is a monotone, 1 doubles every movement) and `acceleration` (0.5-3.0, default 1.0) multiplies the speed (`VoiceParams::formant_speed()`, capped at 8.0), so a host's rate slider reaches a higher or lower top rate. Both are `laprdus_set_inflection_level()` / `laprdus_set_acceleration()` in the C API, `speech.inflection_level` / `speech.acceleration` in `settings.json`, keys `inflection_level` / `acceleration` on Android and Apple, and are shown only for formant voices. `laprdus_get_nominal_wpm(voice)` (175 words/min for Zvonko at speed 1.0, times the voice's tempo) lets a UI print the top rate in words per minute: nominal × 2.0 (the slider's top) × acceleration. The Speech Dispatcher module maps SSIP `pitch_range` onto the inflection level.
- **Number words follow the voice language**: `CroatianNumbers::set_dialect()` is set by `TTSEngine::initialize_formant()` (tisuća/milijun/dvjesto, hiljada/milion/dvesta, hiljada/milion/dvjesto) and reset to Croatian when a concatenative voice is loaded, so Josip and Vlado are unchanged. One and two agree with the feminine scale words in every dialect (dvije tisuće, dve hiljade, dvadeset jedna tisuća, dvije milijarde).
- **Synthesis is deterministic**: the same text and settings give the same samples (`tests/linux/test_formant.cpp` relies on it).
- **Acoustic values come from measurements**, not from taste: vowel formants and durations from Bakran's work on standard Croatian, consonant spectra, levels and transitions from analysis of recorded Croatian speech. `docs/formant.md` records the sources, the measured values and how to repeat the measurements. Change numbers in `formant_phonemes.cpp` only with a measurement or a listening test behind them.
- **Stress** is lexical and cannot be fully predicted. To fix a word, add it to `formant_lexicon.cpp` (notation at the top of the file) and check its whole paradigm, not just the dictionary form: the accent often moves in the other cases and persons (sìgnal/signála, podátak/pòdātākā, obavijéstiti/obàvijēstīm), and a stem entry also catches derived words with a different accent (see "Stress" in `docs/formant.md`). Verbs are not listed form by form: `IJE_VERBS` has the roots with a long *ije* (podijeliti, promijeniti), `VERBS` whole stems with accent and conjugation class (`ur'e:d=i`), and `StressRules::verb_form()` builds the forms; an imperative that is also a noun's case (potvrdi, uredi) is the verb only at the head of a clause; users can do the same from a pronunciation dictionary by writing accent marks in the replacement (`telèfon`).
- **Users have their own accent lexicon**: `accents.json` next to the user dictionaries, entries in the built-in notation as JSON (`{ "word": "kontr'o:l*" }`, `{ "verb": "ur'e:d=i<p" }`, optional `"language"`). `formant::UserLexicon` parses and validates it once; `Frontend::set_user_lexicon()` consults the user's forms, stems and verbs before the built-in ones. The engine keeps it across voice changes (`TTSEngine::load_accent_lexicon()`, C API `laprdus_load_accent_lexicon()`, report via `laprdus_get_accent_lexicon_report()`); `laprdus_load_user_config()`, both CLIs, SAPI5, NVDA, Android (`LaprdusTTS.loadAccentLexicon`) and Apple (`DictionaryState.accentLexiconURL`) load it with the user dictionaries, and user dictionaries off removes it. See "Stress" in `docs/formant.md` and 5.11 in `docs/laprdus.md`.
- Platform code that bypasses `laprdus_set_voice()` must check `VoiceRegistry::is_formant_voice()` and call `TTSEngine::initialize_formant()` (see the Android JNI bridge and the SAPI5 driver).

**Singing presets.** `orguljas`, `klapa`, `pjevac` (Zvonko), `trubac`, `harmonikas`, `pevac` (Stojan), `sevdalija`, `sazlija`, `solist` (Mirsad), `becarac` (Zvonko, the bećarac tune) are formant voices whose `FormantVoice::singing` points at a `SingingStyle` (song, sound source, chorus, sub-octave, reverb, vibrato, note envelope, transpose; all in `formant_synthesizer.cpp`). They sing the text to a public-domain folk song, one syllable per note (a `~` note in the song notation is a melisma, `R` a rest), the vowel on the beat with the consonants before it; the song restarts with every synthesis call (`TTSEngine::begin_utterance()`), and clauses of one call continue it. Rate = tempo, pitch = transposition, inflection level = vibrato depth. `SingingStyle::dry` (pjevac, pevac, solist) keeps the voice's own source and phonation and transposes the song so the middle of its range sits five semitones above the voice's `base_f0`. Notes without a rest between them are legato: the attack envelope is applied only after a rest or at the start of a clause. Base voice is in `base_voice_id` (platform code uses it only as metadata; `data_filename` stays null). The instrument sources (`SourceKind`), chorus, sub-octave and reverb live in `klatt_synth.cpp`; the speaking voices do not use them and their output is unchanged. See "Singing presets" in `docs/formant.md`.

```bash
B=build/macos-arm64-release
$B/laprdus -D $B -v zvonko "Dobar dan, kako ste?"
$B/laprdus -D $B -v stojan -o out.wav "Добар дан!"
$B/laprdus -D $B -v mirsad -r 2.0 "Brzo, ali razgovijetno."
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

### Recorded Voices (TD-PSOLA)

Rate, pitch and intonation of Josip and Vlado are produced by time-domain pitch-synchronous overlap-add over the pitch marks of the recordings (`src/audio/psola.cpp`), with no third-party library. `docs/concatenative.md` has the full description.

- **Speed** (rate): the planner shortens or lengthens every segment's duration (same rules as the formant voices, rate floors for consonants); the renderer repeats or skips periods. Bursts are never stretched; closures take the time at slow rates and lose it first at fast ones.
- **Pitch** (voice character, derived voices): the spectrum of the recordings is warped by `sqrt(pitch)` at analysis time (a child's vocal tract), and the pitch itself by `pitch` at rendering.
- **User Pitch**: the output period spacing alone; formants stay where they are.
- **Intonation**: the shared `formant::Intonation` model draws the contour from the front end's stress and clause kind.

### Dual Pitch System
Two separate pitch parameters serve different purposes:

| Parameter | Range | Purpose | Effect |
|-----------|-------|---------|--------|
| `pitch` | 0.25 - 4.0 | Voice character (derived voices) | Recorded: formants × sqrt(pitch), F0 × pitch. Formant voices: F0 × pitch |
| `user_pitch` | 0.5 - 2.0 (formant 0.25 - 4.0) | User preference (SAPI5/NVDA slider) | F0 only - keeps voice identity |

`TTSEngine::set_language()` tells the recorded voices' front end which language's rules and lexicon to use (`laprdus_set_voice()`, the SAPI5 driver and the Android bridge call it); number words stay Croatian for Josip and Vlado.

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
| **JAVA_HOME (macOS)** | `/Applications/Android Studio.app/Contents/jbr/Contents/Home` |
| **adb (macOS)** | `~/Library/Android/sdk/platform-tools/adb` |

On the macOS machine `./gradlew` runs directly from `android/` once `JAVA_HOME` points at the Android Studio JBR; no wrapper or path tricks are needed.

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

# macOS
export JAVA_HOME="/Applications/Android Studio.app/Contents/jbr/Contents/Home"
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
./scripts/build-all.sh macos          # macOS only
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

Output: `installers/windows/Output/Laprdus_SAPI5_Setup_2.0.0.exe`

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
| `-v, --voice` | Select voice (zvonko (default), stojan, mirsad, josip, vlado, detence, baba, djed) |
| `-r, --speech-rate` | Speech rate 0.5-2.0 (default: 1.0); formant voices 0.25-4.0 |
| `-p, --speech-pitch` | Speech pitch 0.5-2.0 (default: 1.0); formant voices 0.25-4.0 |
| `-I, --inflection` | Inflection of the formant voices 0-100 (default: 50; 0 is a monotone) |
| `-a, --acceleration` | Rate multiplier of the formant voices 0.5-3.0 (default: 1.0) |
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
| `--platform` | `windows`, `linux`, `macos`, `android` |
| `--arch` | `auto` (default, host arch), `x64`, `x86`, `arm64`, `arm` |
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

### Android storage layout (Direct Boot)

`LaprdusTTSService` is `android:directBootAware="true"` so TalkBack can use Laprdus on the lock screen after a reboot, before the first unlock. Everything the service needs at that point lives in **device-protected (DE) storage** under `/data/user_de/0/<pkg>/files`:

- settings DataStore: `datastore/laprdus_settings.preferences_pb` (opened via `deviceProtectedDataStoreFile`, process singleton in `LaprdusStorage`)
- user dictionaries: `user.json`, `spelling.json`, `emoji.json` (written atomically via `AtomicFiles`), and the accent lexicon `accents.json` if present (no editor yet)

Voice data and bundled dictionaries load from APK assets, so synthesis itself never depends on storage.

**Migration.** Builds 10 and earlier stored this data in credential-encrypted (CE) storage (`/data/user/0/<pkg>/files`), which is unreadable while locked. `SettingsMigrator` and `DictionaryMigrator` copy it once after unlock: DE values win, legacy values only fill missing keys, every step is crash-safe and idempotent (re-running after a crash converges), and the migrators are re-armed on `ACTION_USER_UNLOCKED`, on a rate-limited delayed retry, and on the next process start. Debug builds can force a crash at a named step by writing a `MigrationCrashPoint` name into `files/debug/crash_point` in the DE directory.

**Failure policy.** Storage problems (unreadable or corrupt DE/legacy files, failed copies) never crash: they are logged, defaults are used, a localized notice is shown in the Settings screen (settings) or the Dictionaries screen (dictionaries), and migration is retried later. Only "no voice loadable at all" (requested voice and the `josip` fallback both fail) throws `LaprdusEngineUnavailableException`, and only from the synthesis path, so `speak()` returns ERROR and TalkBack can switch engines. A DE crash marker bounds this to 3 throws per 10 minutes; after that the service reports `callback.error()` per utterance instead.

**Backup rules.** `backup_rules.xml` and `data_extraction_rules.xml` must stay empty: with no rules the framework backs up both CE and DE files, while adding any `<include>` flips the file to include-only and silently drops everything not listed.

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
| `-v` | `--voice` | Select voice (zvonko (default), stojan, mirsad, josip, vlado, detence, baba, djed) |
| `-r` | `--speech-rate` | Speech rate (0.5-2.0, default 1.0; formant voices 0.25-4.0) |
| `-p` | `--speech-pitch` | Speech pitch (0.5-2.0, default 1.0; formant voices 0.25-4.0) |
| `-I` | `--inflection` | Inflection of the formant voices (0-100, default 50) |
| `-a` | `--acceleration` | Rate multiplier of the formant voices (0.5-3.0, default 1.0) |
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
sudo dpkg -i laprdus_2.0.0_amd64.deb
sudo dpkg -i laprdus-speechd_2.0.0_amd64.deb

# Fedora/RHEL
sudo rpm -i laprdus-2.0.0.x86_64.rpm
sudo rpm -i laprdus-speechd-2.0.0.x86_64.rpm

# Arch Linux
makepkg -si  # From PKGBUILD directory
```

#### From tarball

```bash
tar xf laprdus-2.0.0-linux-x86_64.tar.xz
cd laprdus-2.0.0-linux-x86_64
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
| Windows Installer | `installers/windows/Output/Laprdus_SAPI5_Setup_2.0.0.exe` |
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
- **C++ core engine** (`src/core/`, `src/audio/`, `src/formant/`, `src/c_api/`)
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
start "" "installers/windows/Output/Laprdus_SAPI5_Setup_2.0.0.exe"

# Launch NVDA addon installer
start "" "nvda-addon/laprdus-2.0.0.nvda-addon"
```

### Android Device Testing

**IMPORTANT**: After building the Android APK, ALWAYS install and test on the user's physical device.

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

#### Android Test Commands

Run from `android/` with `JAVA_HOME` set (Windows: `/c/Program Files/Android/Android Studio/jbr`, macOS: `/Applications/Android Studio.app/Contents/jbr/Contents/Home`):

```bash
cd android
./gradlew testDebugUnitTest            # JVM unit tests (storage, migration, engine runtime)
./gradlew connectedDebugAndroidTest    # instrumented tests on the connected, unlocked device
```

Notes:
- The first `testDebugUnitTest` run downloads Robolectric's android-all SDK 36 jar (~213 MB) into `~/.m2`; the Robolectric tests pin `@Config(sdk = [36])`.
- `SettingsScreenAccessibilityTest` (pre-existing Compose UI tests) fails on the S22 test device regardless of code changes (TalkBack is active there). Everything else is green; to run only the storage/engine classes use `-Pandroid.testInstrumentationRunnerArguments.package=com.hrvojekatic.laprdus.data` (and `.service`).

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

# Formant voice tests (need no voice data except for one voice-switching test)
g++ -std=c++17 -I include -I tests/linux tests/linux/test_formant.cpp \
    -o build/linux-x64-release/test_formant -L build/linux-x64-release -llaprdus -lpthread
LD_LIBRARY_PATH=build/linux-x64-release LAPRDUS_DATA=./build/linux-x64-release \
    ./build/linux-x64-release/test_formant

# Recorded voice tests (rate, pitch, derived voices, melody, streaming)
g++ -std=c++17 -I include -I tests/linux tests/linux/test_concat.cpp \
    -o build/linux-x64-release/test_concat -L build/linux-x64-release -llaprdus -lpthread
LD_LIBRARY_PATH=build/linux-x64-release LAPRDUS_DATA=./build/linux-x64-release \
    ./build/linux-x64-release/test_concat

# The same tests on macOS (the dylib is found through @rpath next to the binary)
B=build/macos-arm64-release
clang++ -std=c++17 -I include -I tests/linux tests/linux/test_formant.cpp \
    -o $B/test_formant -L $B -llaprdus -Wl,-rpath,@loader_path
LAPRDUS_DATA=$B $B/test_formant

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
- [ ] CLI `-l` lists all 8 voices
- [ ] CLI synthesizes audio to WAV file correctly
- [ ] All voices synthesize correctly (josip, vlado, detence, baba, djed, zvonko, stojan, mirsad)
- [ ] Formant voice tests pass (`test_formant`)
- [ ] Recorded voice tests pass (`test_concat`, needs `LAPRDUS_DATA`)

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
tar --transform='s,^\.,laprdus-2.0.0,' -czf ~/rpmbuild/SOURCES/laprdus-2.0.0.tar.gz \
    --exclude='.git' --exclude='build' --exclude='*.pyc' --exclude='__pycache__' \
    --exclude='android/.gradle' --exclude='android/app/build' \
    --exclude='nvda-addon/*.nvda-addon' --exclude='.sconsign*' .

# Copy spec and build
cp installers/linux/rpm/laprdus.spec ~/rpmbuild/SPECS/
rpmbuild -ba ~/rpmbuild/SPECS/laprdus.spec
```

**4. Output files:**
```
~/rpmbuild/RPMS/x86_64/laprdus-2.0.0-1.fc*.x86_64.rpm      # Main package
~/rpmbuild/RPMS/x86_64/laprdus-devel-2.0.0-1.fc*.x86_64.rpm # Dev headers
~/rpmbuild/SRPMS/laprdus-2.0.0-1.fc*.src.rpm               # Source RPM
```

**5. Copy to project directory:**
```bash
cp ~/rpmbuild/RPMS/x86_64/laprdus*.rpm ~/rpmbuild/SRPMS/laprdus*.rpm \
    installers/linux/rpm/
```

**6. Install and test:**
```bash
# Install (use rpm directly due to self-dependency on liblaprdus.so)
sudo rpm -ivh --nodeps ~/rpmbuild/RPMS/x86_64/laprdus-2.0.0-1.fc*.x86_64.rpm \
    ~/rpmbuild/RPMS/x86_64/laprdus-devel-2.0.0-1.fc*.x86_64.rpm

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

## macOS (native library and CLI)

Separate from the Apple app in `Lapplerdus/` (which builds with Xcode), the SCons
build supports macOS as a first-class platform for the shared library and CLI.

```bash
# Builds liblaprdus.dylib + laprdus CLI. --arch defaults to the host
# architecture (arm64 on Apple Silicon, x64 on Intel).
scons --platform=macos --build-config=release macos-all

# Or via the master script
./scripts/build-all.sh macos
```

| Component | Output |
|-----------|--------|
| Shared library | `build/macos-<arch>-release/liblaprdus.dylib` |
| CLI | `build/macos-<arch>-release/laprdus` |

The CLI is the same source as Linux (`src/platform/linux/cli/laprdus_cli.cpp`)
and plays audio through **CoreAudio** (`HAVE_COREAUDIO`), alongside the existing
PulseAudio and ALSA backends:

```bash
B=build/macos-arm64-release
$B/laprdus -D $B -l                         # list voices
$B/laprdus -D $B "Dobar dan!"               # speak via CoreAudio
$B/laprdus -D $B -o out.wav "Dobar dan!"    # write a WAV
```

The dylib records `@rpath/liblaprdus.dylib` as its install name and the CLI is
linked with `-rpath @loader_path`, so the binary finds the library sitting next
to it without `DYLD_LIBRARY_PATH`.

Cross-compiling to Intel from Apple Silicon works: `--arch=x64`.

**Not built on macOS:** SAPI5, the NVDA addon, the Windows config GUI and the
Speech Dispatcher module are all platform-specific and remain Windows/Linux only.
`./scripts/build-all.sh all` detects the host and skips what it cannot build,
warning rather than failing.

## Apple (iOS / iPadOS / macOS)

### Overview

The Apple port lives in `Lapplerdus/Laprdus/Laprdus.xcodeproj` (Xcode 26+) and mirrors the Android app. Deployment targets: **iOS 16.0** and **macOS 13.0** — the minimum versions supporting `AVSpeechSynthesisProviderAudioUnit`; the SwiftUI code deliberately uses only iOS 16 / macOS 13 APIs (`ObservableObject`/`@Published`, single-parameter `onChange`, no `@Observable`/`@Bindable`, no `navigationDestination(item:)`). visionOS is not supported.

| Target | Purpose |
|--------|---------|
| `Laprdus` | Multiplatform SwiftUI app (iOS, iPadOS, macOS) with a 4-tab layout: Main (sample text + play button), Settings, Dictionaries, About |
| `LaprdusVoices` | Speech synthesis provider app extension (`AVSpeechSynthesisProviderAudioUnit`, `ausp` Audio Unit) — exposes the 8 voices system-wide to VoiceOver/Spoken Content, like the Android `TextToSpeechService` |
| `LaprdusTests` | Unit tests (Swift Testing), hosted in the app |

### Structure

```
Lapplerdus/Laprdus/
├── Laprdus/         App-only code (views, AppModel, AudioPlayer, assets, Localizable.xcstrings)
├── Shared/          Code compiled into BOTH app and extension:
│                    LaprdusC.h (bridging header), LaprdusEngine.swift (C API wrapper),
│                    SettingsStore.swift, DictionaryStore.swift, VoiceCatalog.swift, AppGroup.swift
├── LaprdusVoices/   Extension-only code (LaprdusAudioUnit.swift, SSMLParser.swift)
├── Config/          LaprdusVoices-Info.plist + entitlements (app group: group.com.hrvojekatic.laprdus)
└── LaprdusTests/    Unit tests
```

- The C++ engine is compiled **directly into both targets** from `src/core`, `src/audio`, `src/c_api` (same approach as the Android CMake build; file references in the Xcode project point at `../../src/...`). C++17, `LAPRDUS_VERSION_*` defines set in build settings.
- Voice data (`data/voices/*.bin`) and dictionaries (`data/dictionary/*.json`) are bundle resources of both targets. **On a fresh checkout, generate voice data first** (`scons ... voice-data` or `./scripts/build-all.sh voice-data`) — `.bin` files are gitignored.
- **What the system needs before it lists the voices** (from Apple's "Creating a custom speech synthesizer" sample and from other working speech extensions; all three were missing until October 2026). Note: on macOS 27 the voice load was still failing after these were added (`TextToSpeech.MultiError`, extension never launched), with the cause not yet found:
  - the extension's `AudioComponents` entry declares `sandboxSafe` (`Config/LaprdusVoices-Info.plist`);
  - the app calls `AVSpeechSynthesisProviderVoice.updateSpeechVoices()` (`SystemVoices.update()` in `LaprdusApp.swift`); registering the extension alone is not enough;
  - the app group is one the platform actually grants: `group.com.hrvojekatic.laprdus` on iOS, `<team id>.com.hrvojekatic.laprdus` on macOS. Both come from the build setting `LAPRDUS_APP_GROUP` (per-SDK in the project file), are expanded into the entitlements, and are read back at run time by `AppGroup.identifier`. A `group.` identifier that the macOS provisioning profile does not list is refused by the system.
- To check what the system did with the voices: `/usr/bin/log show --last 10m --predicate 'process == "axassetsd" AND category == "VoiceDB"'` ("Set N records for loader: ausp_lprd_HKTC" is success).
- Settings use the shared app-group `UserDefaults` suite with the **same keys and defaults as the Android DataStore** (`default_voice`, `speed`, `pitch`, `volume`, `force_speed`/`force_pitch`/`force_volume`, `emoji_enabled`, `inflection_enabled`, `*_pause`, `number_mode`, `user_dictionaries_enabled`). The user pitch slider maps to `laprdus_set_user_pitch`; voice-character pitch comes from the registry via `laprdus_set_voice`.
- User dictionaries are stored in the app group container (`Dictionaries/user.json`, `spelling.json`, `emoji.json`, plus `accents.json` for the formant voices' accent lexicon, which has no editor yet) in the **same JSON format as Android**, and are applied both in-app and in the extension. All three files use `grapheme`/`phoneme` entries (that is what every platform's editor writes); the engine's spelling and emoji parsers accept those keys next to the bundled files' `character`/`pronunciation` and `emoji`/`text`.
- One speech request is not one phrase. VoiceOver describes an item as a row of `<voice>` elements (name, value, type), puts `<break>` between some of them, and marks only a part as `<say-as interpret-as="characters">` (a one-character badge count next to a button name). `SSMLParser` therefore returns ordered parts (text, spelled text, pause, phrase boundary), each with its own rate and pitch, and `LaprdusAudioUnit` synthesizes them one by one: a `<break>` is silence of its length, a boundary between `<voice>`/`<p>`/`<s>` elements is the user's comma pause. Never decide spelling, rate or pitch for a whole request.
- VoiceOver nests `<prosody>` elements (a neutral outer one, an inner `pitch="+50.0%"` for capital letters); `SSMLParser` treats inner values as relative to the outer ones. The extension logs the markup and the parts of each request (never the text): `log show --info --predicate 'subsystem == "com.hrvojekatic.laprdus"'`.
- Localization: `Localizable.xcstrings` string catalog with en/hr/sr (translations taken from the Android `values-hr`/`values-sr`).

### Building

```bash
cd Lapplerdus/Laprdus

# macOS app (signed, automatic provisioning)
xcodebuild -scheme Laprdus -destination 'platform=macOS' build -allowProvisioningUpdates

# iOS Simulator
xcodebuild -scheme Laprdus -destination 'generic/platform=iOS Simulator' build

# Unit tests (engine synthesis, dictionary store, SSML, settings)
xcodebuild test -scheme Laprdus -destination 'platform=macOS'
```

### Testing the system voices

After installing/running the app once, the `LaprdusVoices` extension registers with the system. Voices appear under:
- **iOS**: Settings → Accessibility → Spoken Content → Voices
- **macOS**: System Settings → Accessibility → Spoken Content → System voice → Manage Voices

The extension mirrors the Android service behavior: honors host rate/pitch unless the "Force" settings are on, pins volume to 1.0 unless forced, and speaks single grapheme clusters through the spelling dictionary. It always synthesizes with the voice the host requested — Android's "Force language" setting has no Apple equivalent (Spoken Content always names a concrete voice) and is not part of this port.
