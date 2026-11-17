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
├── Cargo.toml              # Cargo workspace; the root crate `laprdus` is the engine
├── src/
│   ├── common/             # The engine (Rust crate `laprdus`, lib.rs)
│   │   ├── engine.rs       # Engine: the synthesis pipeline
│   │   ├── params.rs, config.rs, voice.rs, language.rs
│   │   ├── dictionary/     # Pronunciation, spelling and emoji dictionaries
│   │   ├── text/           # Clauses, marks, numbers, the shared front end
│   │   ├── languages/      # hr, sr, bs, sl: each language complete and independent
│   │   │                   # (front end, stress, lexicon, numerals, tables, voices)
│   │   ├── prosody/        # Utterance, intonation model (both kinds of voices)
│   │   ├── formant/        # Formant voices: rules, Klatt synthesizer, singing
│   │   ├── concatenative/  # Recorded voices: voice data, analysis, prosody, PSOLA
│   │   └── audio/          # Volume boost, brightness tilt
│   └── platform/           # Platform-specific code
│       ├── c_api/          # Public C API (crate laprdus-capi)
│       │   └── include/laprdus/laprdus_api.h   # C API declarations
│       ├── cli/            # Command-line program (Windows, Linux, macOS)
│       ├── windows/        # sapi5/ (SAPI5 driver), configurator/ (config GUI), legacy/ (the build for Windows 7, 8, 8.1), nvda/ (NVDA add-on)
│       ├── linux/          # speechd/ (Speech Dispatcher module)
│       ├── android/        # Android app (Kotlin, Gradle); native/ is the JNI crate
│       └── apple/          # Xcode project (app + speech extension)
├── src/platform/windows/nvda/             # NVDA screen reader addon
├── installers/             # Platform installers
│   └── linux/              # deb, rpm, PKGBUILD, tarball
│   └── windows/            # InnoSetup
├── data/                   # Data
│   ├── hr/                 # Josip/*.wav, dictionary.json, spelling.json, emoji.json
│   ├── sr/                 # Vlado/*.wav
│   ├── sl/                 # emoji.json
│   ├── voices/             # Generated voice .bin files (cargo xtask data)
│   └── dictionary/         # Generated: the dictionaries as the platforms ship them
├── tests/                  # Engine tests (formant.rs, recorded.rs, golden data)
└── tools/                  # xtask/ (build steps), formant/ (generators, analysis), golden/
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
│                 Engine (src/common/engine.rs)                   │
│  - preprocess() → dictionary lookup, number expansion           │
│  - synthesize() → full text-to-speech pipeline                  │
│  - synthesize_spelled() → character-by-character spelling       │
└─────────────┬───────────────────────────────────────┬───────────┘
              │                                       │
              ▼                                       ▼
┌─────────────────────────┐         ┌─────────────────────────────┐
│  text::numbers          │         │  text::clauses              │
│  - NumberReader         │         │  - split_clauses()          │
│  languages::<code>      │         │  - segment by punctuation   │
│  - front end, numerals  │         │                             │
└───────────┬─────────────┘         └─────────────────────────────┘
            │
            ▼
┌─────────────────────────────────────────────────────────────────┐
│          ConcatSynthesizer  /  FormantSynthesizer               │
│  - synthesize_clause() → front end, plan_clause(), render()     │
│    (durations, pitch contour, TD-PSOLA over the pitch marks;    │
│    the formant voices: ClauseBuilder, KlattSynth)               │
└─────────────┬───────────────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────────────┐
│                       VoiceData                                 │
│  - from_file() / from_packed() / from_directory()               │
│  - sound() → the samples of one recording                       │
└─────────────────────────────────────────────────────────────────┘
```

The programs written in Rust (the CLI, the SAPI5 driver, the Speech
Dispatcher module, the Android JNI library, the Configurator) use the
`laprdus` crate directly; the C API is for programs in other languages (the
NVDA add-on through the SAPI5 DLL, which exports it too, and the Apple app
through the static library).

### 1.3 Data Flow

1. **Input**: UTF-8 text string
2. **Preprocessing**: Number expansion, dictionary lookup, emoji replacement
3. **Clause segmentation**: Split text at punctuation (chooses the melody and the pause)
4. **Front end**: `languages::Frontend` (the rules of the language spoken, `src/common/languages/<code>/`) → phones with stress, word and syllable indices
5. **Prosody**: durations, closures, word gaps, pitch contour (`concatenative/prosody.rs`, `prosody/intonation.rs`)
6. **Rendering**: TD-PSOLA from the pitch marks of the recordings (`concatenative/psola.rs`), volume, soft limit
7. **Output**: 16-bit PCM audio @ 22050 Hz mono

---

## 2. Core Engine Components

### 2.1 Engine (`src/common/engine.rs`)

The main orchestrator type that coordinates the synthesis pipeline.

**Key Methods:**
```rust
// Switch to a voice (old voice ids accepted); a recorded voice loads its data
// file from the directory, a formant voice needs none. The voice speaks the
// language chosen with set_language(), else its native language
pub fn set_voice(&mut self, id: &str, data_directory: &Path) -> Result<(), Error>;

// Language of any voice: front end rules, lexicon, letter names, number words;
// None goes back to the voice's native language
pub fn set_language(&mut self, language: Option<Language>);
pub fn language(&self) -> Language;

// Main synthesis function
pub fn synthesize(&mut self, text: &str) -> Result<Vec<i16>, Error>;

// Spelling mode (character-by-character); final_pause = false leaves the
// silence after the last character out; its length (ms) is returned
pub fn synthesize_spelled(&mut self, text: &str, final_pause: bool) -> Result<(Vec<i16>, u32), Error>;
```

Text preprocessing (emoji and pronunciation dictionaries, then numbers to
words) is the private `Engine::preprocess()`.

**Internal Components:**
- `VoiceData` (`concatenative/voice_data.rs`) - Loads and provides access to the recordings of a voice
- `languages::Frontend` - Converts text to phones (letter-to-sound, stress) by the rules of the language spoken
- `ConcatSynthesizer` / `FormantSynthesizer` - Render a clause with a recorded or a formant voice
- `split_clauses()` (`text/clauses.rs`) - Segments text into clauses at punctuation
- `PronunciationDictionary` - Word/phrase replacements
- `SpellingDictionary` - Character-to-pronunciation mapping
- `EmojiDictionary` - Emoji-to-text conversion

**Thread Safety:**
An `Engine` is used by one thread at a time (its methods take `&mut self`, so the compiler enforces it). Create one instance per thread or use external synchronization, as the platform crates do with a mutex. This is documented and intentional for performance.

### 2.2 Text to sounds (`src/common/text/`, `src/common/languages/`)

The phoneme mapper of the first engine is gone: text becomes phones in the
front end of the language spoken (`src/common/languages/<code>/`), over the
shared tokenizer and word model of `src/common/text/frontend/`.

**Croatian Alphabet Support:**
- Standard letters: A-Z
- Special characters: Č, Ć, Đ, Š, Ž
- Digraphs: LJ, NJ, DŽ (treated as single phonemes)

**Key Items:**
```rust
// The front end of a language: a clause of text to an Utterance (phones with
// stress, words, clause kind); src/common/text/mod.rs
pub trait Frontend {
    fn process(&self, text: &[char], punct: Punctuation, letter_sounds: bool) -> Utterance;
    fn letter_sound(&self, letter: &[char]) -> Utterance;
}

// Serbian (and Macedonian) Cyrillic to Latin; src/common/text/cyrillic.rs
pub fn to_latin(text: &[char]) -> Vec<char>;
```

Text is a Rust `&str` (UTF-8) at the API and a slice of `char` inside the
front end.

**The recordings of a voice (`Sound` in `src/common/concatenative/voice_data.rs`):**
```rust
#[repr(u8)]
pub enum Sound {
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Ch,    // Č
    Tj,    // Ć
    Dj,    // Đ and DŽ
    Sh,    // Š
    Zh,    // Ž
    Lj,    // LJ digraph
    Nj,    // NJ digraph
    Silence,
}
```

The phones the front end produces are `Ph` in `src/common/formant/phones.rs`;
`unit_for()` in `concatenative/prosody.rs` maps a phone to its recording.

### 2.3 NumberReader (`src/common/text/numbers/`)

Converts numeric strings to words in the current language: the engine makes
a `NumberReader` for the language in `Engine::set_language()` for every
voice; the language-neutral part (scanning, phone numbers, currency, cell
addresses, cardinals) is in `text/numbers/`, the grammar of each language
(the `Grammar` trait) in `src/common/languages/<code>/numerals.rs` (Croatian
*tisuća / milijun / dvjesto*, Serbian *hiljada / milion / dvesta*, Bosnian
*hiljada / milion / dvjesto*; one and two agree with the feminine scale
words: *dvije tisuće*, *dve hiljade*). Slovenian has a system of its own
(*nič, ena, dve, tri; 1 to 4 agree with what they count, en koš, dve uri, trije
koši, z dvema žličkama; enaindvajset; dva tisoč; dva milijona, trije milijoni, pet milijonov; dve
milijardi; tri cele štirinajst; 132.457 read as one number; dates in the
genitive, sedmega oktobra*).

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

### 2.4 Clauses (`src/common/text/clauses.rs`)

`split_clauses()` segments the text into clauses at punctuation; the
punctuation of a clause chooses its melody (the shared intonation model,
`src/common/prosody/intonation.rs`) and the pause after it.

**Punctuation Effects:**
- Period (.) → falling pitch, longer pause
- Comma (,) → slight rising pitch, short pause
- Question mark (?) → rising pitch at end
- Exclamation mark (!) → emphasis, longer pause
- Newline → configurable pause

**Configuration** (`src/common/params.rs`; the defaults are 100, 100, 100 and 200 ms):
```rust
pub struct PauseSettings {
    pub sentence_pause_ms: u32,  // After . ! ?
    pub comma_pause_ms: u32,     // After ,
    pub newline_pause_ms: u32,   // After \n
    pub spelling_pause_ms: u32,  // Between spelled characters
}
```

### 2.5 Voice registry (`src/common/voice.rs`)

Manages voice definitions including physical, derived and formant voices.

**Voices and languages are separate.** Every voice speaks every language
(Croatian `hr-HR`, Serbian `sr-RS`, Bosnian `bs-BA`, Slovenian `sl-SI`); the registry's
`language` of a voice is only its *native* language, spoken when the host
asks for nothing else. `Engine::set_language()` switches the language of
any voice (letter-to-sound rules, accent lexicon, letter names, number
words), `Engine::language()` reports it. The default voice is
`voice::default_voice()` (`DEFAULT_VOICE_ID`) = `zvonko`, for every language and
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
`voice::find()`, `laprdus_set_voice()` and
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
```c
typedef struct LaprdusVoiceInfo {
    const char* id;              // Plain ASCII: "josip", "djedo", "lika_velebit"
    const char* display_name;    // Shown to the user: "Josip", "Đedo", "Dalmacija Klapa"
    const char* language_code;   // Native language: "hr-HR", "sr-RS" or "bs-BA" (no voice is natively Slovenian)
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
`laprdus::language::Language` with `code()`, `short_code()`, `lcid()`, `english_name()`, `native_name()`
in `src/common/language.rs`; `Language::parse()` parses tags and names):

| index | code | short | LCID | English | native |
|-------|------|-------|------|---------|--------|
| 0 | hr-HR | hr | 0x041A | Croatian | Hrvatski |
| 1 | sr-RS | sr | 0x081A | Serbian | Srpski |
| 2 | bs-BA | bs | 0x141A | Bosnian | Bosanski |
| 3 | sl-SI | sl | 0x0424 | Slovenian | Slovenščina |

### 2.6 Dictionary System (`src/common/dictionary/`)

Three dictionary types for text transformation. The sources are per
language (`data/hr/dictionary.json`, `spelling.json`, `emoji.json`,
`data/sl/emoji.json`); `cargo xtask data` stages them in `data/dictionary/`
under the names the platforms ship (`internal.json`, `spelling.json`,
`emoji.json`, `emoji_sl.json`), which is a build product and not checked in.

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

### 3.1 VoiceData (`src/common/concatenative/voice_data.rs`)

Manages loading and access to the recordings of a voice (`VoiceData::from_file()`,
`from_packed()`, `from_directory()`); `pack()` writes the packed file, which is
what `cargo xtask data` calls.

**Binary Format (64-byte header + 32-byte entries, little-endian), as `pack()` writes and `VoiceData::parse()` reads it:**
```text
Header (HEADER_SIZE = 64 bytes)
  offset  0  u32  magic            FILE_MAGIC 0x4C505244, 'LPRD'
  offset  4  u16  version          FILE_VERSION 1
  offset  6  u16  flags            FLAG_ENCRYPTED 0x0001
  offset  8  u32  count            Number of recordings
  offset 12  u32  index offset     64
  offset 16  u32  audio offset
  offset 20  u32  total size       Total file size
  offset 24  u32  sample rate      22050
  offset 28  u16  bits per sample  16
  offset 30  u16  channels         1 (mono)
  offset 32  u32  CRC-32 of the audio
  ...             unused, zero, to 64 bytes

Index entry (INDEX_ENTRY_SIZE = 32 bytes)
  offset  0  u32  id               Sound enum value
  offset  4  u32  name hash
  offset  8  u32  data offset      Within the audio
  offset 12  u32  packed size      Bytes
  offset 16  u32  size             Bytes
  offset 20  u32  duration         Samples
  offset 24  u16  flags            0x0004 truncated
  ...             reserved, to 32 bytes
```

**Audio Properties:**
- Sample rate: 22050 Hz
- Bit depth: 16-bit signed PCM
- Channels: 1 (mono)
- Phoneme truncation: L, M, N, S, SH, V, Z, ZH capped at 2000 bytes

### 3.2 UnitBank (`src/common/concatenative/unit_bank.rs`)

Analysis of every recording when a voice is loaded: DC removal and 2 ms edge
fades, a pitch track (normalised autocorrelation with the voice's median
pitch as prior), one pitch mark per period at the peaks of the low-passed
waveform, marks every 5 ms in unvoiced sound, the sounding part and the
burst onset of a stop. A voice character pitch other than 1.0 resamples the
recordings by `sqrt(pitch)` first (spectrum warp of the derived voices).

### 3.3 Prosody (`src/common/concatenative/prosody.rs`)

Durations by the rules of the formant voices (stress, length, final
lengthening, rate floors), stops never stretched (burst kept, closure takes
or gives the time), 22 ms gaps between words, and the pitch contour from the
shared `Intonation` model (`src/common/prosody/intonation.rs`): accents on stressed syllables, question
rises, final falls, declination; inflection level and enable, voice character
pitch and user pitch all multiply onto the voice's natural pitch.

### 3.4 PSOLA renderer (`src/common/concatenative/psola.rs`)

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

### 4.1 C API (`src/platform/c_api/`)

Clean C interface for programs written in other languages: the crate
`laprdus-capi` (`src/lib.rs`), declared in
`include/laprdus/laprdus_api.h`. It builds as a shared library, a static
library and a Rust library (`liblaprdus_capi`, installed on Linux and macOS
under the name it has always had, `liblaprdus`); the SAPI5 DLLs export it
too. Every exported function checks its pointers and catches a panic, which
becomes the function's error value, so nothing the engine does can take a
host (a screen reader) down with it.

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
const char* laprdus_get_language(handle);                          // "hr-HR" | "sr-RS" | "bs-BA" | "sl-SI"

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
LaprdusError laprdus_set_head_size(handle, value);          // voice quality, 0 .. 100 each, 50 = the voice as it is
LaprdusError laprdus_set_breathiness(handle, value);        // (int laprdus_get_head_size(handle) and the like read them)
LaprdusError laprdus_set_roughness(handle, value);
LaprdusError laprdus_set_brightness(handle, value);
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
that uses the engine directly (the Android JNI library, the SAPI5 driver,
the CLI, the Speech Dispatcher module) calls `Engine::set_language()`; the
language then holds across `Engine::set_voice()` / `set_voice_from()`, which
without it start in the voice's native language.

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
volume is used (Android: 1.0, the system volume governs; Apple: the volume
VoiceOver puts in the request's `<prosody volume>`, in dB).

**Volume boost** (`VoiceParams::volume_boost`, `laprdus_set_volume_boost()`,
`speech.volume_boost` in settings.json, key `volume_boost` on Android and
Apple, CLI `-B`): louder speech for a phone's earpiece in traffic, after the amplified speech volume of phone systems. Both renderers drive the clause through
`src/common/audio/boost.rs` before the volume is applied: a gain of
`VOLUME_BOOST_GAIN` (5.0, +14 dB), then a lookahead peak limiter (5 ms
lookahead, a 20 ms hold, a 50 ms release) with its ceiling
`VOLUME_BOOST_CEILING` at 0.944 on the true peak (`inter_sample_peak()`, an
8-tap Lanczos interpolator at a quarter, a half and three quarters of each
sample step: Josip, held at 0.97 sample by sample, reached +1.4 dBFS between
the samples, which a phone's resampler clips; now -0.35 dBTP, the formant
voices -0.7). The hold is longer than a glottal period down
to 50 Hz, so the gain moves only between syllables and the waveform of a
syllable stays a scaled copy of the plain one; the tanh soft clipper above
`VOLUME_BOOST_KNEE` 0.9 is a safety net the ceiling keeps idle, and the
renderers apply no knee of their own after the boost (the recorded voices'
plain output goes through the same limiter at unity gain after the volume,
its ceiling at their 0.9 knee, since at full volume Vlado and Đedo drove the
knee into heavy distortion; see `docs/concatenative.md`). In boost mode two more
things happen, since the listening of 7 November 2026 on a phone at full
volume: the gain may rise at most `VOLUME_BOOST_MAX_LIFT_DB` (2 dB; 6 for a
day, which still let the bursts of k, t and p come 4 dB closer to the vowels)
above the gain of the loudest syllable of the last 0.6 s, so the consonants
between loud vowels keep their distance from them (without the cap the limiter
lifted Zvonko's friction frames 3.4 dB closer to the voiced median; with 2 dB
they end 1.2 dB further away), and a second-order high shelf takes the treble
down by `VOLUME_BOOST_TREBLE_DB` (3 dB) from `VOLUME_BOOST_TREBLE_HZ` (4 kHz)
up before the limiter, for every voice. After the shelf, the hiss limiter
(`limit_hiss()`, since a listener heard the hissing consonants pierce at full
phone volume with every voice): while the band above `VOLUME_BOOST_HISS_HZ`
(3 kHz) outweighs the rest of the sound (gate from -12 to -3 dB; no vowel
gets there, every sibilant does), the part above `VOLUME_BOOST_HISS_SPLIT_HZ`
(2 kHz, a fourth-order split, so the 2-5 kHz of š, č and ć come down too) is
held `VOLUME_BOOST_HISS_DB` (-22 dB) under the loudest nearby vowel (the low
band averaged over 20 ms, the loudest within 250 ms either side), cut by at
most `VOLUME_BOOST_HISS_MAX_CUT_DB` (24 dB). The band is the signal minus its
low-passed part, so at unity gain the output is the input. Through a phone
speaker's 500 Hz high-pass every voice's sibilants now sit 16-21 dB under the
vowels (Zvonko's s and c were 13 dB under, Josip's s, č and ć level with
them); see "The hissing consonants in boost mode" in `docs/formant.md`.
Measured on a 37 s paragraph
(`tools/formant/spectrum.py --boost`) with the voice of "The reference balance":
Zvonko's median speech level -17.6 → -11.2 dBFS, his loud stretches -13.9 →
-9.0 dBFS, the residual of the loudest frames against a scaled copy of the
shelved plain output under -40 dB (against the unshelved one -27 dB, the
shelf's own work); Josip +1.4 dB with his treble 0.2-1.0 dB down instead of
up, and Baba +2 dB, since their recordings are compressed and at full scale
already. Until November 2026 the limiter had a 1 ms attack, a 30 ms
release and a ceiling of 1.4, and a soft clipper from 0.6 up rounded every
vowel peak off: 3 dB louder, at a residual of -18 dB (10 % distortion), which
phone users heard. The output is still deterministic, and a host at half
volume with the boost on is louder than without it. Tests: the volume boost cases of
`tests/formant.rs` and `tests/recorded.rs`, `test_volume_boost` in
`tests/nvda/test_driver.py`; the sweep behind the settings is in "Hiss,
crackle and the boost" in `docs/formant.md`.

**Voice quality** (`VoiceParams::head_size`, `breathiness`, `roughness`,
`brightness`, integers 0-100, default 50; `laprdus_set_head_size()` and the
like; `speech.head_size` ... in settings.json; keys `head_size`, `breathiness`,
`roughness`, `brightness` on Android and Apple; CLI `--head-size` ...): four
settings after the voice-parameter model of a classic formant synthesizer. 50 is the voice as
its tables define it, so the defaults are bit-identical to the output before
them. Head size multiplies every formant by 0.71 at 0 and 1.4 at 100 (0.8 and
1.25 until November 2026; the
higher value is the higher voice, at the maintainer's word; synthesizer A's runs
the other way)
(`head_size_factor()` in `src/common/params.rs`): the formant voices through a
per-utterance copy of the voice with a scaled `formant_scale` (also the fixed
F5-F8), the recorded voices through the bank warp of `ensure_bank()`, which
leaves the pitch where it was. Breathiness runs from a pressed, hard voice at 0
(a pressed voice: the glottis closed for longer, a flatter and sharper
source, a weaker fundamental, no breath noise;
`breathiness_tension()`) to a whisper at 100 (the voicing replaced by breath
through the cascade; `breathiness_whisper()`, `VoiceQuality::whisper`);
roughness its jitter and shimmer (0 none, 100 eight
times) and from 50 up adds diplophonia, every second glottal period shorter
and weaker (`VoiceQuality::roughness`, `roughness_diplophonia()`), since five
times the random jitter alone was not heard (formant voices only); since
November 2026 the diplophonia is twice as strong (every second period up to
10 % shorter and 80 % weaker) and below 50 the slow pitch drift goes too, so
0 is a perfectly steady voice. Brightness is a one-pole tilt of the whole
output around 1.5 kHz for every voice (`tilt_brightness()` in
`src/common/audio/brightness.rs`: 0 the top folded away from an 800 Hz pivot,
100 the top above 1.5 kHz tripled), which
also reaches the hissing consonants and the bursts (Zvonko's s 8 dB further
under the vowel at 0), plus, for the formant voices, the source emphasis (0
the bare pulse, 100 the 0.85 cap; the voice's own is 0.55 since the match to
the reference balance, see "The reference balance" in `docs/formant.md`).
Until November 2026 the formant voices had the emphasis alone, and 0 left
their s as sharp as before. The engine applies them
per clause, so a change is heard with the next utterance. Tests: the voice quality cases of
`tests/formant.rs`, `head_size_and_brightness_shape_the_recorded_voices` in
`tests/recorded.rs`, `test_voice_quality` in `tests/nvda/test_driver.py`.

**Thread Safety:**
- The last error message is kept with the handle; strings handed to the caller are held per thread
- A handle must not be used from two threads at once

### 4.2 Windows SAPI5 (`src/platform/windows/sapi5/`)

COM-based Speech API 5 driver (crate `laprdus-sapi5`, the `windows` crate
for COM). The ordinary build needs Windows 10 or later, as Rust's Windows
targets do; Windows 7 SP1, 8 and 8.1 get a build of their own
(`cargo xtask windows-legacy`, `src/platform/windows/legacy/README.md`).

**Files:**
- `src/lib.rs` - ISpTTSEngine implementation, the class factory, `DllRegisterServer` (registers the COM class; the voice tokens are the installer's)
- `laprdus_sapi5.rc`, `build.rs` - version resource, embedded with `embed-resource`

The DLL also exports the C API (`laprdus_*`), which the NVDA add-on loads it for.

**Registration:**
- 32-bit: `laprd32.dll` → HKLM\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices
- 64-bit: `laprd64.dll` → HKLM\SOFTWARE\Microsoft\Speech\Voices
- Single CLSID with registry virtualization

**Voice Enumeration:**
Three voice tokens, one per language, instead of one per Laprdus voice:
- "Laprdus Croatian" - LCID 0x041A (hr-HR)
- "Laprdus Serbian" - LCID 0x081A (sr-RS)
- "Laprdus Bosnian" - LCID 0x141A (bs-BA)
- "Laprdus Slovenian" - LCID 0x0424 (sl-SI)

The token sets the language (`Engine::set_language()`); the voice comes
from `settings.json` (`voice.default`, empty = `zvonko`, old ids accepted),
so all three tokens speak with the voice chosen in the Laprdus
Configurator. There is no language key in `settings.json`: the host chooses
the language. The installer removes the per-voice tokens of earlier
versions on install, upgrade and repair and re-registers the three language
tokens.

### 4.3 NVDA Addon (`src/platform/windows/nvda/`)

Python-based synthesizer driver for NVDA screen reader.

**Key Files:**
- `addon/synthDrivers/laprdus/__init__.py` - SynthDriver implementation
- `addon/synthDrivers/laprdus/_laprdus.py` - ctypes bindings to DLL
- `addon/globalPlugins/laprdus/__init__.py` - NVDA menu integration

**Features:**
- NVDA's rate and pitch sliders scale `speech.speed` and `speech.pitch` from settings.json (unless forced); rate boost multiplies every slider position by 3 through the engine's acceleration (`speech.acceleration` is not used), as other synthesizers' rate boost does
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
- `src/main.rs` - Speech Dispatcher module (crate `laprdus-speechd`, binary `sd_laprdus`): a process that speaks the module protocol over its standard input and output; it needs no Speech Dispatcher headers to build
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

### 4.5 Android (`src/platform/android/`)

Native library + Kotlin TTS Service.

**JNI library (`src/platform/android/native/src/lib.rs`, crate `laprdus-android`, built as `liblaprdus.so`):**
- One native engine per `LaprdusTTS` object, each with its own mutex: `nativeCreate()` returns a handle that every other call takes as its first argument (a handle, not a field read from `thiz`, so R8 renaming cannot break it; a number that names an engine in a table, never a pointer, so a stale handle finds nothing). The engine exists from creation, so settings applied before the first voice are kept
- Two engines in one process: `LaprdusTTS.service` (the TTS service: TalkBack and other apps) and `LaprdusTTS.app` (the app's preview and settings, provided by Hilt), so a voice, rate or dictionary set by one never reaches the other
- Asset manager integration for voice data
- Proper JNI type conversions (the `jni` crate); a panic is caught and gives the call's failure value

**Keeping the two engines in step:** the service follows the app through the
saved settings and the dictionary files, not through the engine: a newly
saved default voice is loaded at once, the user dictionaries switch and every
dictionary write (`DictionaryRepository.changes`, bumped in the process on each
save, delete or migration) reload the dictionaries.
`LaprdusTTS.setVoice()` and `reloadDictionaries()` load the bundled
dictionaries and then the user's from `userDictionaryDir`
(`UserDictionaries.load()`), inside `exclusive { }`, which is also where the
service sets a request's rate and pitch and synthesizes.

**TTS Service (`src/platform/android/app/.../LaprdusTTSService.kt`):**
- Implements `TextToSpeechService`
- Hilt dependency injection
- Jetpack Compose settings UI
- Offers every voice for every locale: 54 system voices named `<id>-<lang>` (`zvonko-sr`). The requested locale sets the engine language (`Engine::set_language()` through the JNI library), the voice stays the one configured in the app, and the default voice is `zvonko`. The app has a Language setting for its in-app playback; "Force language" means: ignore the requested language and speak the one chosen in Laprdus settings

**Build Requirements:**
- The Rust toolchain with the four Android targets, cargo-ndk and the NDK (the linker); Gradle's `buildNativeLibrary` task runs `cargo ndk ... build --release -p laprdus-android`, so the library is optimized in debug builds of the app too
- 16KB page size alignment (Android 15+), set in `.cargo/config.toml`
- API level 24-36 support

---

## 5. Build System

### 5.1 Cargo workspace (`Cargo.toml`)

The engine and every program are Rust (1.85 or later, edition 2024); Cargo is
the build system. The workspace's members are the engine (the root crate
`laprdus`, `src/common/lib.rs`), the platform crates under `src/platform/`
(`laprdus-capi`, `laprdus-cli`, `laprdus-sapi5`, `laprdus-configurator`,
`laprdus-speechd`, `laprdus-android`) and `tools/xtask`. `cargo xtask <step>`
(an alias in `.cargo/config.toml`) runs the build steps Cargo does not do by
itself. The NVDA add-on keeps its own SCons build (the NVDA add-on template),
the Android app its Gradle build, the Apple app its Xcode project.

**Platforms:**
- Windows - the MSVC build tools as the linker (targets `x86_64-pc-windows-msvc`, `i686-pc-windows-msvc`): Windows 10 or later. Windows 7 SP1, 8 and 8.1: the targets `x86_64-win7-windows-msvc` and `i686-win7-windows-msvc`, built by `cargo xtask windows-legacy` with the nightly toolchain named in `src/platform/windows/legacy/toolchain.txt` (see the README there)
- Linux - the host target; `pkg-config` and `libasound2-dev` for the CLI's audio
- macOS - `aarch64-apple-darwin`, `x86_64-apple-darwin` (library and CLI; the app is built by Xcode, whose script phase `src/platform/apple/build-rust-engine.sh` builds the engine as `liblaprdus_capi.a`). Not yet run on a Mac since the engine moved to Rust
- Android - the four Android targets through cargo-ndk and the NDK

**Architectures:**
- x64, x86 (Windows)
- the host's (Linux)
- arm64-v8a, armeabi-v7a, x86_64, x86 (Android)
- arm64, x64, or both in one binary with `--universal` (macOS)

**Build Configurations:**
- `--release` - Optimized (`opt-level = 3`, LTO, one codegen unit); what `cargo xtask` and the Gradle build use, and what the tests should run with
- the default dev profile - Debug symbols, no optimization

### 5.2 Build Steps

| Step | Description |
|------|-------------|
| `cargo xtask data` | Pack the recordings into `data/voices/*.bin` and stage the dictionaries in `data/dictionary/` (`--key HEX` or `LAPRDUS_PHONEME_KEY` packs the voice files with a key) |
| `cargo xtask windows` | SAPI5 DLLs, `laprdus.exe` and `laprdgui.exe` (the configuration GUI) for x64 and x86, staged in `dist/windows-x64/` and `dist/windows-x86/` |
| `cargo xtask windows-legacy` | The same programs for Windows 7 SP1, 8 and 8.1 (nightly toolchain, `-Zbuild-std`), staged in `dist/windows-legacy-x64/` and `dist/windows-legacy-x86/`; see `src/platform/windows/legacy/README.md` |
| `cargo xtask build` | Linux, macOS: the CLI, the C API library and on Linux the Speech Dispatcher module (runs `data` first) |
| `cargo xtask install` | Linux, macOS: build and install (`--destdir`, `--prefix`, `--libdir`, `--moduledir`) |
| `cargo xtask dist` | Linux, macOS: the release tarball (CLI, library, header, voice data, dictionaries, install script) in `dist/<os>-<arch>/` |
| `cargo build --release -p <crate>` | One crate by itself |

`build`, `install` and `dist` also take `--universal` and
`--codesign-identity ID` (macOS) and `--locked`, `--offline`, `--frozen`.

### 5.3 Build Order

1. **Voice data** packed from `data/<language>/<Voice>/*.wav` (`cargo xtask data`; the packer is `pack()` in `src/common/concatenative/voice_data.rs`)
2. **Voice data and dictionaries** land in `data/voices/` and `data/dictionary/` for all platforms
3. **Platform-specific builds** proceed with dependencies (`cargo xtask build`, `install`, `dist` and the Android Gradle build run step 1 themselves)

### 5.4 Building

**Windows:**
```bash
# Voice data and dictionaries
cargo xtask data

# SAPI5 DLLs, CLI and Configurator, both architectures, into dist/windows-x64 and dist/windows-x86
rustup target add x86_64-pc-windows-msvc i686-pc-windows-msvc
cargo xtask windows
cargo xtask windows-legacy   # the same for Windows 7 SP1, 8 and 8.1 (dist/windows-legacy-*)

# NVDA add-on (the installer carries it and offers to install it on its last page)
(cd src/platform/windows/nvda && scons)

# Installer (requires InnoSetup); it starts in the display language of Windows, without a language dialog
iscc installers/windows/laprdus_sapi5.iss
```

**Linux:**
```bash
cargo xtask build
sudo cargo xtask install --prefix /usr
```

**Android:**
```bash
cd src/platform/android
./gradlew assembleRelease
```

**macOS** (not yet run on a Mac since the engine moved to Rust):
```bash
# Library and CLI for the host architecture
cargo xtask build

# Release for distribution outside the Mac App Store: the notarized app DMG and
# the universal, Developer ID signed command-line tools tarball in dist/macos/.
# Needs a "Developer ID Application" certificate and notarytool credentials
# (xcrun notarytool store-credentials laprdus ...); --development checks the
# pipeline without them, --skip-notarization signs only.
./scripts/build-macos-release.sh

# The Cargo half of it by hand: universal binaries, signed after linking, packed
# as dist/macos-universal/laprdus-<version>-macos-universal.tar.xz
cargo xtask dist --universal --codesign-identity "Developer ID Application: Name (TEAMID)"
```

---

## 6. Testing

### 6.1 Automated Tests

**Engine (`tests/formant.rs`, 94 cases; `tests/recorded.rs`, 24 cases; unit tests inside `src/common/`):**
- Formant and recorded voices: languages, numerals, accents, rate, pitch, voice quality, pauses, spelling, streaming, singing
- `tests/dsp_against_cpp.rs`, `tests/frontend_against_cpp.rs`, `tests/numbers_against_cpp.rs` compare with dumps of the last C++ engine's output (local files in `build/golden`); they pass without the dumps

**CLI Tests (`src/platform/cli/tests/cli.rs`):**
- 14 tests of the command-line program
- Voice selection, synthesis, file output

**C API Tests (`src/platform/c_api/tests/c_api.rs`, `header.rs`):**
- 27 tests of the exported functions, and one of the header against the library

**Speech Dispatcher Tests (unit tests in `src/platform/linux/speechd/src/main.rs`):**
- 3 tests for module parameter mapping

**Running Tests:**
```bash
# The recorded voice cases need the packed voices and dictionaries
cargo xtask data

# Release, because the suites render a lot of audio
cargo test --release --workspace

# One suite; LAPRDUS_TEST_INFO=1 prints the measured values behind the thresholds
cargo test --release --test formant
LAPRDUS_TEST_INFO=1 cargo test --release --test recorded -- --nocapture

# Python tests of the Windows programs and the NVDA driver (after cargo xtask windows)
python tests/nvda/test_driver.py
python tests/nvda/test_manuals.py
python tests/windows/test_configurator_language.py
python tests/windows/test_sapi5_driver.py
```

`LAPRDUS_DATA` names the directory of the packed voices when it is not
`data/voices`.

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

**Rust (edition 2024):**
- `cargo fmt` with the repository's `rustfmt.toml` (`max_width = 120`, `use_small_heuristics = "Max"`)
- `cargo clippy`: the workspace warns on `clippy::all`
- `unsafe` is forbidden in the engine and in every crate that takes the workspace lints (`unsafe_code = "forbid"`: the engine, the CLI, the Speech Dispatcher module, xtask). It lives only in the platform boundary crates: the C API (`laprdus-capi`), the JNI library (`laprdus-android`), the SAPI5 driver's COM code (`laprdus-sapi5`), and in the Configurator only its Win32 wrapper `win32.rs` (the crate denies it elsewhere)
- Exported C API functions and the JNI functions catch panics and return the call's failure value
- Croatian, Serbian and Bosnian are three independent copies under `src/common/languages/` by the owner's decision: a fix to rules they share is made three times
- Adding a language: a folder `src/common/languages/<code>/`, one match arm in `languages/mod.rs` (`rules()`), a `Language` variant in `language.rs`

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
1. Edit `.po` files in `src/platform/windows/nvda/addon/locale/<lang>/LC_MESSAGES/`
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
