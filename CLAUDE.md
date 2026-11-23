# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

LaprdusTTS is a Croatian/Serbian/Bosnian/Slovenian text-to-speech (TTS) engine supporting:
- **SAPI5** (Windows Speech API) for system-wide TTS integration
- **NVDA** screen reader synthesizer driver

It has two kinds of voices:
- **Concatenative** voices (Josip, Vlado and the voices derived from them) are rendered from pre-recorded phoneme WAV files by TD-PSOLA. See "Recorded Voices" below and `docs/concatenative.md`.
- **Formant** voices (Zvonko, Stojan, Mirsad) are synthesized entirely by rule and need no data files. See "Formant Voices" below.

## Topic files (`.claude/rules/`)

This file holds what every task needs, and Claude Code loads all of it at every start. The details of each area are in `.claude/rules/`, one file per topic: Claude Code loads a topic file by itself when Claude reads a file matching its `paths:`. When a task is about a topic, read its file first anyway, before the code. New details go into the topic file, not here: this file must stay well under Claude Code's 150k-character limit (it was 220k before the split of 2026-11-23).

| File | Topic |
|------|-------|
| `slovenian.md` | Slovenian: the front end, the lexicon and its generated tables, readings by context, names, every listener report |
| `numbers-dates.md` | Number words and their grammar in all four languages, telephone numbers, currency amounts, the short pauses of clock times, dates and cell addresses, every written form of a date |
| `synthesis.md` | The sound of the voices: ranges, voice quality, the reference balance, the hiss, settings.json 1.0, the host's rate and pitch, the volume boost, z and ž, singing presets, performance, the recorded voices (TD-PSOLA), the dual pitch |
| `spelling.md` | Spelling: letter sounds and names, capitals announced by screen readers, lone letters in running text, spelling speed, the spelling dictionary |
| `streaming.md` | Stopping a synthesis and audio clause by clause, on every platform; the SAPI5 bookmarks |
| `windows.md` | The build for Windows 7 SP1, 8 and 8.1, the SAPI5 installer, the Configurator's languages, the NVDA add-on, testing SAPI5 and NVDA by hand |
| `android.md` | The Android app: building, toolchain and Play requirements, the settings screen and TalkBack, Direct Boot storage, the two native engines, device testing |
| `apple.md` | The macOS library, CLI and release build; the Apple app and speech extension |
| `linux.md` | Linux: Speech Dispatcher, building, dependencies, installation, packages and their tests |
| `cli.md` | The command-line program: usage and options |

## Architecture

The engine and every program are written in Rust (migrated from C++ in November 2026). There is no C or C++ left. The golden corpus (`tests/golden/corpus.jsonl`, the texts of the former C++ test programs) and the hashes of the last C++ engine's output for it are frozen; `tools/golden/golden.py check` compares a build with them.

### Build System
- **Cargo** workspace at the repository root (Rust 1.85 or newer, edition 2024). The root crate `laprdus` is the engine (`[lib] path = "src/common/lib.rs"`); every program is a member crate under `src/platform/`
- **`cargo xtask <step>`** (`tools/xtask`) runs the build steps Cargo does not do by itself: `data`, `windows`, `windows-legacy`, `build`, `install`, `dist` (`cargo xtask` alone lists them)
- Platforms: Windows 7 SP1 and later (Windows 10 and later with Rust's ordinary Windows targets; Windows 7 SP1, 8 and 8.1, which those targets no longer run on, with a second build of the same programs: see "Windows 7 SP1, 8 and 8.1 (the legacy build)" below), Linux, macOS, Android, Apple
- Architectures: x64, x86, ARM64, ARM (the Rust targets are listed under "Packages and targets" below)
- Build: `cargo xtask data`, then `cargo xtask windows` and `cargo xtask windows-legacy` (Windows) or `cargo xtask build` (Linux, macOS)
- The NVDA add-on keeps its own SCons build (`src/platform/windows/nvda/sconstruct`); nothing else uses SCons

### Workspace layout

| Path | Purpose |
|------|---------|
| `src/common/` | The engine (crate `laprdus`): everything that is the same for every language and platform |
| `src/common/languages/<code>/` | One folder per language (`hr`, `sr`, `bs`, `sl`), complete and independent: front end, stress, lexicon, numerals, words, the generated data files in `tables/`, and its voices (`formant/`, `concatenative/`) |
| `src/platform/` | One crate or project per platform: `c_api`, `cli`, `windows/sapi5`, `windows/configurator`, `windows/legacy`, `windows/nvda`, `linux/speechd`, `android/`, `apple/` |
| `data/<code>/` | Checked-in data of a language: the recordings (`data/hr/Josip/*.wav`, `data/sr/Vlado/*.wav`), `data/hr/dictionary.json`, `spelling.json`, `emoji.json`, `data/sl/emoji.json`. `cargo xtask data` makes `data/voices/*.bin` and `data/dictionary/*.json` from them; both folders are build products and are not checked in |
| `tools/xtask/` | The build steps (`src/main.rs`: `data`, `windows`; `src/install.rs`: `build`, `install`, `dist`) |
| `tools/formant/` | Table generators and measurement scripts (Python) |
| `tools/golden/`, `tests/golden/` | Hashes of the last C++ engine's output and the scripts that record and check them |
| `tests/` | Integration tests of the engine (`formant.rs`, `recorded.rs`, `*_against_cpp.rs`), and the Python tests of the NVDA driver and the Windows programs |

Rules of the layout:

- **A language is complete in its folder, and the folders do not use one another.** Croatian, Serbian and Bosnian are three full copies by the owner's decision: **a fix to rules they share is made three times**, in `languages/hr/`, `languages/sr/` and `languages/bs/` (the generated `tables/english.txt`, `places.txt` and `persons.txt` are written into the three folders by their generators). Slovenian has rules of its own.
- **Adding a language**: a folder `src/common/languages/<code>/` that implements `Rules` (`src/common/languages/mod.rs`: front end, words, numbers), its `mod` line and one match arm in `rules()` there, and a variant of `Language` (`src/common/language.rs`, with its codes, LCID and names); its data, if it has any, in `data/<code>/`. `languages/mod.rs` is the only door the rest of the engine uses.
- **`unsafe` is forbidden in the engine**: `unsafe_code = "forbid"` is a workspace lint (`Cargo.toml`), inherited by the engine, the CLI, the Speech Dispatcher module and xtask. It lives only in the platform boundary crates: the C API (`src/platform/c_api`), the JNI exports of `src/platform/android/native` (`#[unsafe(no_mangle)]` and nothing else), the COM code of `src/platform/windows/sapi5`, the one module `src/win32.rs` of the Configurator (the rest of that crate denies it), and the delay-load hook of `src/platform/windows/legacy/compat`.
- **Exported C API functions catch panics**: every `laprdus_*` function checks its pointers, catches a panic and returns its error value (`src/platform/c_api/src/lib.rs`), so nothing the engine does can take a screen reader down. The JNI functions and the SAPI5 driver's entry points do the same.
- Formatting is `rustfmt` with `rustfmt.toml` (`max_width = 120`); `clippy::all` warns in every crate.

### Core Components (`src/common/`)

| File | Purpose |
|------|---------|
| `engine.rs` | `Engine`: orchestrates the synthesis pipeline; voices, language, dictionaries, spelling, the user's configuration |
| `params.rs` | `VoiceParams`, `PauseSettings`, the ranges and scales of the settings |
| `config.rs` | The user's `settings.json` (`UserSettings`, `UserConfig`) |
| `voice.rs` | Voice definitions (`VOICES`: Josip, Vlado, derived voices, formant voices, singing presets), `find()`, `default_voice()` |
| `language.rs` | `Language`: codes, LCIDs, names, `Language::parse()` |
| `dictionary/` | Pronunciation, spelling and emoji dictionaries |
| `text/clauses.rs` | Clause segmentation at punctuation (`split_clauses()`), pause settings |
| `text/marks.rs` | Marks the number converter leaves for the front end: a short pause inside a clause, the longer pause before a cell's address, the letters of a designation such as A17 |
| `text/numbers/` | `NumberReader`, the language-neutral part of number reading: number-to-words up to centillions (`cardinal.rs`), the scan of the text (`mod.rs`, `scan.rs`), telephone numbers (`phone.rs`), currency amounts (`currency.rs`), cell addresses (`cells.rs`). The grammar of a language is its `Grammar` in `languages/<code>/numerals.rs` |
| `text/frontend/` | The shared part of the text front end: tokenizer (`tokenizer.rs`), word model (`mod.rs`), letters and letter names (`letters.rs`), the lexicon notation (`notation.rs`), the user's accent lexicon (`user_lexicon.rs`), `finish_utterance()` (`utterance.rs`) |
| `text/cyrillic.rs` | Serbian and Macedonian Cyrillic to Latin |
| `languages/mod.rs` | The front door of the languages: `Rules`, `rules()`, `Frontend` |
| `languages/{hr,sr,bs}/` | Croatian, Serbian, Bosnian, each with `frontend.rs` and `clause.rs` (text → phones: clitics, homographs, assimilation), `phonemize.rs` (letter-to-sound), `stress.rs` (`StressRules`), `lexicon/` (built-in accent lexicon; `verbs.rs` the verb tables), `verbs.rs`, `english.rs`, `numerals.rs`, `words.rs` (clitics, symbol and letter names) |
| `languages/{hr,sr,bs}/tables/places.txt`, `persons.txt` | Place names (towns, villages, regions) and given names and surnames with their case forms; generated by `tools/formant/proper_names.py` from `tools/formant/places.tsv` and `persons.tsv` |
| `languages/{hr,sr,bs}/tables/english.txt` | English words with their stressed vowel letter, for words the English rules miss; generated by `tools/formant/english_words.py` from the CMU Pronouncing Dictionary and `tools/formant/english_extra.tsv` |
| `languages/sl/` | Slovenian front end: letter-to-sound rules (open/close e and o, schwa, l and v as [w], final devoicing) and stress rules (`phonemize.rs`), letter, symbol and spelled names (`words.rs`), the hand-written lexicon (`lexicon.rs`), readings by context (`readings.rs`, `clause.rs`, `marks.rs`), numerals (`numerals.rs`, `nouns.rs`); `tables/` holds the pronunciation tables `forms.txt` and `stems.txt` and eleven word lists generated by `tools/formant/slovenian_words.py` from Sloleks 3.1 (CC BY-SA 4.0) |
| `languages/<code>/formant/`, `concatenative/` | The voices of a language: formant voices and singing presets (Zvonko in `hr`, Stojan in `sr`, Mirsad in `bs`), recorded and derived voices (`hr`: Josip, Detence, Baba; `sr`: Vlado, Đedo) |
| `prosody/intonation.rs` | Intonation model (accents, boundary movements, declination) shared by both kinds of voices; `prosody/mod.rs` has `Utterance` and the short pauses |
| `formant/phones.rs` | Formant voices: acoustic targets of every phone |
| `formant/builder.rs` | Formant voices: `ClauseBuilder`: durations, formant tracks, intonation |
| `formant/klatt.rs` | Formant voices: cascade/parallel (Klatt) synthesizer |
| `formant/synthesizer.rs`, `voice.rs`, `singing.rs` | Formant voices: `FormantSynthesizer`, `FormantVoice` and the voice quality settings, songs and `SingingStyle` |
| `concatenative/synthesizer.rs` | Recorded voices: clause synthesis (front end → planner → renderer) |
| `concatenative/unit_bank.rs` | Recorded voices: analysis of the recordings (pitch marks, voicing, bursts) |
| `concatenative/prosody.rs` | Recorded voices: durations, closures, word gaps, pitch contour |
| `concatenative/psola.rs` | Recorded voices: TD-PSOLA renderer |
| `concatenative/voice_data.rs` | WAV file loading from .bin or directory; `pack()` writes the .bin files |
| `audio/boost.rs`, `audio/brightness.rs` | Volume boost, peak limiter and hiss limiter; the brightness tilt |

### Platform-Specific Code

| Path | Purpose |
|------|---------|
| `src/platform/c_api/` | The C API (crate `laprdus-capi`, `src/lib.rs`), header `include/laprdus/laprdus_api.h`; a cdylib and a staticlib for hosts in other languages, an rlib for the SAPI5 driver |
| `src/platform/cli/` | Command-line program `laprdus` (crate `laprdus-cli`): one CLI for Windows, Linux and macOS |
| `src/platform/windows/sapi5/` | SAPI5 COM driver (laprd32.dll, laprd64.dll); it also exports the C API, which the NVDA add-on loads it for |
| `src/platform/windows/configurator/` | Laprdus Configurator (laprdgui.exe) |
| `src/platform/linux/speechd/` | Speech Dispatcher module (sd_laprdus) |
| `src/platform/android/` | The Android app (Gradle project); `native/` is the JNI crate (`laprdus-android`, liblaprdus.so) |
| `src/platform/apple/` | The Apple app and speech extension (Xcode project in `Laprdus/`), `build-rust-engine.sh`; built for the iPhone and as the notarized macOS release on 2026-11-18, not yet tested with VoiceOver |
| `src/platform/windows/legacy/` | The build for Windows 7 SP1, 8 and 8.1: `README.md`, `toolchain.txt` (the pinned nightly) and `compat/` (crate `laprdus-win7`, the hook that answers `combase.dll` with `ole32.dll`) |
| `src/platform/windows/nvda/` | NVDA add-on: the Python synthesizer driver and its SCons build (it was `nvda-addon/` at the root until November 2026) |

### Processing Pipeline

1. Text → `Engine::preprocess()` (`src/common/engine.rs`) → Number expansion, dictionaries
2. → `split_clauses()` (`src/common/text/clauses.rs`, called by `Engine::segment()`) → Segment into clauses by punctuation and at line breaks (a line break ends the clause with the newline pause, so the next line starts a sentence: a capital S or K there is the preposition; a colon or exclamation mark glued to the next character, as in `12:30`, is not a clause end: it stays in the text and is read by name by the front end's `SYMBOLS` table; a glued period stays only before a file extension or domain label of up to five letters or digits in one case, `datoteka.txt`, `www.index.hr`, and after a single letter, where it is an abbreviation dot, silent, with the letters read by name: `s.a.r.s.`, `U.S.A.`; every other glued period ends the clause, `Preslušano.Nestajuća`; the decimal point is written by the number converter as U+2024 so it survives, `3.14` tri točka četrnaest; a period between groups of three digits is the thousands separator in every language (`1.317` tisuću tristo sedamnaest, `1.000.000` milijun; since 2026-11-03, before that read by name), any other group after a period is a whole number (`3.14` tri točka četrnaest, `192.168.1.1` group by group), the decimal comma as *zarez* (but a comma between the single digits of a phone number spelled out by a screen reader, `+ 1 2 3,5 6,7 8 9,8 7 6`, as a comma pause), clock times `12:30` and dates `7.10.2026` / `7. 10. 2026.` without the separators and with a short pause between their parts (see "Short pauses inside a clause"), a number of up to four digits with a period before a lowercase word as the cardinal with the period silent: `7. listopada` sedam listopada, `1990. godine` (no ordinals: they would need the noun's case), and sets a number glued to letters off from them (`u20:03` u dvadeset nula tri, `5kg`), silencing the period that joins a word to those letters: `7.listopada.u20:03` sedam listopada u dvadeset nula tri; the front end expands no abbreviation, spells a vowelless one (`dr.` de er), and silences the colon of `https://`)
3. → `Frontend::process()` (the trait `text::Frontend`; `languages::Frontend` hands the clause to the front end of the language being spoken, `src/common/languages/<code>/frontend.rs`) → phones with stress, words, clause kind (both kinds of voices); for Slovenian `process()` of `languages/sl/frontend.rs` with the rules and lexicon of `src/common/languages/sl/`
4. → Recorded voices: `plan_clause()` (`src/common/concatenative/prosody.rs`: durations, pitch contour) → `render()` (`concatenative/psola.rs`: TD-PSOLA)
   Formant voices: `ClauseBuilder` → `KlattSynth`
5. → Output 16-bit PCM @ 22050Hz mono; rate, pitch, volume and intonation are applied at the source by both

### Phoneme System

- **Format**: 16-bit PCM WAV, 22050 Hz, mono
- **Storage**: Packed `.bin` files (Josip.bin, Vlado.bin) or raw WAV directory
- **Croatian phonemes**: A-Z + č, ć, đ, š, ž, lj, nj, dž (the Slovenian open e/o, close e/o, schwa and [w] of the formant voices are rendered by the recorded e, o, e and u)
- **Analysis at load**: DC removal, pitch marks, voicing, burst onset per recording (`src/common/concatenative/unit_bank.rs`)
- **Joins**: pitch-synchronous crossfade of whole periods (no fixed crossfade, no truncation)

### Voice System

| # | id | display_name | Type | Native language | base_pitch |
|---|----|--------------|------|-----------------|------------|
| 0 | josip | Josip | Physical | hr-HR | 1.0 |
| 1 | vlado | Vlado | Physical | sr-RS | 1.0 |
| 2 | detence | Detence | Derived (josip) | hr-HR | 1.5 (child) |
| 3 | baba | Baba | Derived (josip) | hr-HR | 1.2 (grandma, Female Senior) |
| 4 | djedo | Đedo | Derived (vlado) | sr-RS | 0.75 (grandpa, Male Senior) |
| 5 | zvonko | Zvonko | Formant | hr-HR | 1.0 (the default voice) |
| 6 | stojan | Stojan | Formant | sr-RS | 1.0 |
| 7 | mirsad | Mirsad | Formant | bs-BA | 1.0 |
| 8 | orguljas | Orguljaš - Vila Velebita | Singing preset (zvonko) | hr-HR | 1.0 (pipe organ) |
| 9 | dalmacija_klapa | Dalmacija Klapa | Singing preset (zvonko) | hr-HR | 1.0 (klapa choir) |
| 10 | trubac | Trubač - Kreće se lađa francuska | Singing preset (stojan) | sr-RS | 1.0 (trumpet) |
| 11 | harmonikas | Harmonikaš - Kreće se lađa francuska | Singing preset (stojan) | sr-RS | 1.0 (accordion) |
| 12 | sevdalija | Sevdalija - Kad ja pođoh na Bembašu | Singing preset (mirsad) | bs-BA | 1.0 (sevdah singer) |
| 13 | sazlija | Sazlija - Kad ja pođoh na Bembašu | Singing preset (mirsad) | bs-BA | 1.0 (saz) |
| 14 | lika_velebit | Lika Velebit | Singing preset (zvonko) | hr-HR | 1.0 (own voice, own pitch) |
| 15 | solunac | Solunac | Singing preset (stojan) | sr-RS | 1.0 (own voice, own pitch) |
| 16 | sarajevo_bembasa | Sarajevo Bembaša | Singing preset (mirsad) | bs-BA | 1.0 (own voice, own pitch) |
| 17 | becarac | Bećarac | Singing preset (zvonko) | hr-HR | 1.0 (own voice, bećarac tune) |

**Every voice speaks every language.** Language (hr, sr, bs, sl) is chosen separately from the voice, as in other synthesizers; a voice's `language` / `language_code` in the registry is only its *native* language, spoken until the host asks for another. The language selects the text front end (letter-to-sound rules, accent lexicon, letter names when spelling) and the number words (tisuća/milijun hr, hiljada/milion/dvesta sr, hiljada/milion/dvjesto bs, tisoč/milijon/dvesto sl) for every voice, Josip and Vlado included. No voice has Slovenian as its native language: every voice speaks it when asked. C API: `laprdus_set_language(h, tag)` ("hr", "hr-HR", "hr_HR", "hrv", "sr-Latn-RS", "srp", "bs", "bos", "sl", "sl-SI", "slv", "Croatian", "hrvatski", "Slovenian", "slovenščina", any case; NULL/"" = back to the voice's native language), `laprdus_get_language(h)` ("hr-HR" | "sr-RS" | "bs-BA" | "sl-SI"), `laprdus_get_language_count()` (4), `laprdus_get_language_info(index, &LaprdusLanguageInfo)` (code, short_code, lcid 0x041A/0x081A/0x141A/0x0424, english_name, native_name Hrvatski/Srpski/Bosanski/Slovenščina). Once set, the language sticks across later `laprdus_set_voice()` calls. Rust: `Engine::set_language(Option<Language>)` works for every voice and sets the language of the number words (`None` goes back to the voice's native language), `Engine::language()` reads it, `Engine::set_voice()` keeps a language the host chose and otherwise starts in the voice's native language, `Language::parse()` and `Language::code()`, `short_code()`, `lcid()`, `english_name()`, `native_name()` live in `src/common/language.rs`. Per platform: NVDA "Language" combo (hr/sr/bs/sl, default from NVDA's UI language; `LangChangeCommand` switches among the four and ignores other languages); SAPI5 four tokens "Laprdus Croatian" / "Laprdus Serbian" / "Laprdus Bosnian" / "Laprdus Slovenian" that set the language and speak with the voice from `settings.json` (`voice.default`, empty = zvonko; no language key); Android 72 system voices `<id>-<lang>` (the requested locale sets the language, the configured voice is kept; a request without a locale and the in-app Speak button get the system language when Laprdus speaks it, else the Language setting (`Language.effective()`); "Force language" = speak the Language setting everywhere, whatever the system and the apps ask for; with it off and a supported system language the Language dropdown is shown disabled with the system language); Apple one `AVSpeechSynthesisProviderVoice` per voice × language, identifier `com.hrvojekatic.laprdus.<id>.<lang short>`, a Language picker in the app; CLI `-L hr|sr|bs|sl` (without it a voice speaks its native language; `-l` lists voices with their native language); Speech Dispatcher LIST VOICES returns every voice for every language, SET LANGUAGE calls `Engine::set_language()` and keeps the voice (`spd-say -l sr -y "Zvonko"`), MALE1 = zvonko, MALE2 josip, MALE3 djedo, FEMALE1 baba, CHILD_MALE detence for all four languages.

**Default voice: Zvonko everywhere**, for every language and platform (`laprdus_get_default_voice()` / `voice::default_voice()`, `DEFAULT_VOICE_ID` = "zvonko": CLI `-v` default, Speech Dispatcher `MALE1`, Android `DEFAULT_VOICE` / fallback voice, NVDA, Apple, the Windows config GUI, SAPI5 `voice.default` empty). A voice the user has chosen is never replaced. The recorded voices stay available.

**Ids and names.** Ids are plain ASCII without diacritics on purpose (dj for đ, c for č/ć, s for š, z for ž): registration safety. Display names come from the registry (`LaprdusVoiceInfo.display_name`, UTF-8, no "Laprdus" prefix, no language suffix); platform UIs must not hard-code them (they may append a localized description: child, grandma, formant, singing). Old ids resolve to the renamed voice in `voice::find()` (`ALIASES` in `src/common/voice.rs`), `laprdus_set_voice()` and `laprdus_get_voice_info_by_id()` (djed → djedo, dijete → detence, baka → baba, klapa → dalmacija_klapa, pjevac → lika_velebit, pevac → solunac, solist → sarajevo_bembasa); `laprdus_get_current_voice()` returns the canonical id; UIs normalize a saved id before comparing it with the list. The spellings "Djed", "Djedo", "Baka", "Dijete", "Detenče", "Zvonko Orguljaš", "Klapa Zvonko", "Stojan Trubač", "Mirsad Solist" etc. must not appear anywhere: Detence, Baba, Đedo, and the preset names of the table.

`laprdus_set_voice()` applies the voice's base pitch itself (times the last `laprdus_set_pitch()` value), so derived voices sound right without a pitch call after every voice change.

`Voice::synthesis` (`Synthesis::Concatenative` / `Synthesis::Formant`, `src/common/voice.rs`) tells the two kinds apart. Formant voices have no data file (`Voice::data_file()` is `None`, the C API's `data_filename` null); `laprdus_set_voice()` accepts any data directory for them. A singing preset has a `base_voice_id` and no data file (or index >= 8). New voices must be appended to the registry (`VOICES`): existing code and saved settings rely on the order of the first five.

### Audio Parameters (`src/common/params.rs`)

```rust
pub struct VoiceParams {              // the defaults are those of `VoiceParams::default()`
    pub speed: f32,                   // 1.0; 0.5 - 2.0 (tempo, time-stretching); formant voices 0.25 - 4.0
    pub pitch: f32,                   // 1.0; 0.25 - 4.0 (voice character pitch)
    pub user_pitch: f32,              // 1.0; 0.5 - 2.0 (user pitch preference); formant voices 0.25 - 4.0
    pub volume: f32,                  // 1.0; 0.0 - 1.0
    pub volume_boost: bool,           // false; louder speech: +14 dB through a peak limiter whose gain moves only between syllables, before the volume (every voice)
    pub inflection_level: f32,        // 0.5; 0 monotone, 0.5 as measured, 1 twice the movements (every voice)
    pub acceleration: f32,            // 1.0; rate multiplier 0.5 - 3.0 (every voice; VoiceParams::effective_speed())
    pub emoji_enabled: bool,          // false; emoji to text
    pub number_mode: NumberMode,      // whole numbers or digit by digit
    pub pause_settings: PauseSettings,
    pub spelling_mode: SpellingMode,  // SpellingMode::LetterSounds; spelled letters: sounds ([b], [ts]; the default) or names (be, ce)
    pub spelling_speed: i32,          // 50; spelling cadence: lower values add silence; speech rate stays unchanged
    pub head_size: i32,               // 50; voice quality, 0-100 each, 50 = the voice as it is: vocal tract length (every voice)
    pub breathiness: i32,             // 50; breath: 0 a tense voice, 100 a whisper (formant voices)
    pub roughness: i32,               // 50; jitter and shimmer (formant voices)
    pub brightness: i32,              // 50; tone: the source emphasis (formant voices), a tilt around 1.5 kHz (recorded voices)
}
```

## Working on Slovenian (read this first)

The maintainer asked for this to be standing practice, so it need not be repeated in every conversation. Slovenian reports come from a native speaker testing the beta, relayed by the maintainer and often written in Croatian. Base every Slovenian decision on trusted Slovenian linguistic and phonetic sources, and name the source in the code comment and in `docs/formant.md` (4a):

- **Sloleks 3.1** (the maintainer's "SloLex"; CJVT, CLARIN.SI, hdl 11356/2080): word forms, accents, the generated tables. Only its hand-accented (MANUAL) entries are a reference; the rest are a neural network's.
- **SSKJ² and Slovenski pravopis 2001 on [Fran](https://fran.si)**, with the **Jezikovna svetovalnica**: what is right, including where they mark length, quality and final [u̯]. They settle what Sloleks's transcriptions cannot.
- **SUK 1.1** (hdl 11356/1959): every context rule is measured on it by lemma and tag before it stays, and every word the change moves is read, not only the percentage. It has few UI texts, so it shows false alarms better than it shows that a menu rule is complete.
- **Kolokacije of CJVT** (viri.cjvt.si/kolokacije): what goes with what.
- **GaMS** (CJVT's language model; local GGUF with `llama-server`): the maintainer asks that it be consulted too, as a second opinion on grammar and sense. It is an LLM, not a dictionary, and has been wrong on accents in every round: it decides nothing, and where it disagrees with the dictionaries, say so in the report.
- Not sources: forums, general web pages, and memory. Never write an accent or a rule from memory: hand entries written that way were the main fault of the first version.

The native speaker's own description is the specification; where it departs from the dictionaries, follow it and say so. Check a word's whole paradigm and its class. Changes to shared code are gated to Slovenian and checked with an old/new audio hash for hr, sr and bs, unless the report asks for the other languages too.

## Formant Voices

Zvonko (hr), Stojan (sr) and Mirsad (bs) are defined in `src/common/languages/<code>/formant/mod.rs` and rendered by `src/common/formant/`; they share the text preprocessing of the concatenative voices (emoji and pronunciation dictionaries, number expansion, clause segmentation). From there `Engine::synthesize_clauses()` hands each clause to `FormantSynthesizer::synthesize_clause()`:

1. `Frontend::process()` - Cyrillic → Latin, digraphs (lj, nj, dž), the ijekavian *ije* diphthong, syllabic r, spelled-out abbreviations; stress from accent marks in the text, the lexicon, suffix rules, or the first syllable; proclitics/enclitics; voicing and place assimilation.
2. `ClauseBuilder` - segment durations, formant targets and locus-based transitions, source amplitudes, and the F0 contour (word accents, declination, boundary movement chosen by the clause's punctuation).
3. `KlattSynth::render()` - glottal source, cascade formants for voiced sound and aspiration, parallel resonators for frication and bursts.

Things to know before changing them:

- **Speed, pitch and volume are applied at the source** (durations, F0, gain), so `VoiceParams::pitch` and `user_pitch` both simply scale F0.
- **Ranges, voice quality, the reference balance, the hiss, settings.json 1.0, the host's rate and pitch, the volume boost, z and ž**: `.claude/rules/synthesis.md`. In short: every platform may pass the wide ranges (speed and `user_pitch` 0.25-4.0) and the engine narrows them per voice; the inflection level and the acceleration are for every voice; the four voice quality settings (0-100) leave the voice as it is at 50; a host's rate and pitch scale the user's own (`laprdus_set_speed(setting × host)`, `laprdus_set_user_pitch(setting × host)`), the force switches mean "ignore the host", and the volume is not scaled.
- **Android: the app and the TTS service have separate native engines** in one process: `.claude/rules/android.md`.
- **Number words follow the language**, for every voice (`Engine::set_language()` makes the `NumberReader` of the language, whose grammar is the language's `Grammar` in `src/common/languages/<code>/numerals.rs`; the numerals agree with their nouns in all four languages): `.claude/rules/numbers-dates.md`.
- **Slovenian** (`src/common/languages/sl/`): the front end, the lexicon and its generated tables, the readings by context, the names and every listener report: `.claude/rules/slovenian.md`. Read it, with "Working on Slovenian" above, before any Slovenian change.
- **Synthesis is deterministic**: the same text and settings give the same samples (`tests/formant.rs` relies on it).
- **Acoustic values come from measurements**, not from taste: vowel formants and durations from Bakran's work on standard Croatian, consonant spectra, levels and transitions from analysis of recorded Croatian speech. `docs/formant.md` records the sources, the measured values and how to repeat the measurements. Change numbers in `src/common/formant/phones.rs` only with a measurement or a listening test behind them.
- **Stress** is lexical and cannot be fully predicted. To fix a word, add it to the lexicon, `src/common/languages/hr/lexicon/entries.rs` and its copies in `sr/` and `bs/` (the notation is described in `src/common/text/frontend/notation.rs`), and check its whole paradigm, not just the dictionary form: the accent often moves in the other cases and persons (sìgnal/signála, podátak/pòdātākā, obavijéstiti/obàvijēstīm), and a stem entry also catches derived words with a different accent (see "Stress" in `docs/formant.md`). Verbs are not listed form by form: `IJE_VERBS` has the roots with a long *ije* (podijeliti, promijeniti), `VERBS` whole stems with accent and conjugation class (`ur'e:d=i`), and `StressRules::verb_form()` builds the forms; an imperative that is also a noun's case (potvrdi, uredi) is the verb only at the head of a clause; users can do the same from a pronunciation dictionary by writing accent marks in the replacement (`telèfon`). Place names and personal names are tables of their own (`tables/places.txt` and `persons.txt` of `hr`, `sr` and `bs`; edit `tools/formant/places.tsv` or `persons.tsv` and run `tools/formant/proper_names.py`, which writes all six), used only for words written with a capital letter (not in all-capitals text, and after the user's own entries) so that a village or a name never changes the common word it is spelled like; a personal name wins over a place; an entry marked `=` (spelled like a frequent word) is also skipped for the first word of a clause; `proper_names.py` refuses two accents for one spelling. Number compounds of age and time (petogòdišnjī, šestòmjesečnī, malòljetan) are a rule, `StressRules::time_compound()`, and so are Slavic names in -mir/-dar/-zar (Rȁdomīr, Bȍžidār), `StressRules::slavic_compound_name()`.
- **English words** keep the voices' letter sounds but get English stress with a plain (neutral) accent, never the rising one, wherever it falls (Croàtian, Ábleton, Elèven): the table `tables/english.txt` (the same file in `languages/hr`, `sr` and `bs`; looked up after the native exact lexicon, before its stems), and for words spelled the English way (`english_spelling()`: q/w/x/y, th, ch, ph, ck, -tion, i before a/e/u, doubled consonants, with native prefix boundaries excluded) the rules in `english_rule()`. A capital after two small letters splits a word into its parts (ElevenLabs → Eleven Labs, ČetDžipiti → Čet Džipiti) unless a lexicon knows the whole word (TikTok) or every part is one syllable (TalkBack); never after one small letter (iPhone, BiH, and the Lj/Nj/Dž of transliterated Cyrillic capitals). `english_word()` strips stress-neutral endings, the -s/-ed/-er/-ing/-ment ones only from English-spelled words because native words end that way too (operater, distributer), and Croatian case endings from any word; native forms that would match (reforma ← reform) are unmarked "stop" entries in the table. Native derivations of English stems keep -irati/-izam (chatirati), and the user's verbs win. To add or correct a word, add a row to `tools/formant/english_extra.tsv` and rerun `tools/formant/english_words.py` (needs `pip install wordfreq spylls`, `cmudict.dict` and the LibreOffice Hunspell hr_HR/sr-Latn/bs_BA files); it keeps native words out (Hunspell) and only writes words the rules get wrong. `english_words.py` mirrors `english_word()` (`english.rs` of `hr`, `sr` and `bs`) in `lookup()`, and asks the engine where it stresses a word without the table through `examples/english_stress.rs` (`cargo run --release --example english_stress`): change the four together. See "English words" in `docs/formant.md`.
- **Users have their own accent lexicon**: `accents.json` next to the user dictionaries, entries in the built-in notation as JSON (`{ "word": "kontr'o:l*" }`, `{ "verb": "ur'e:d=i<p" }`, optional `"language"`). `UserLexicon` (`src/common/text/frontend/user_lexicon.rs`) parses and validates it once; `Frontend::set_user_lexicon()` consults the user's forms, stems and verbs before the built-in ones. The engine keeps it across voice changes (`Engine::load_accent_lexicon()`, C API `laprdus_load_accent_lexicon()`, report via `laprdus_get_accent_lexicon_report()`); `laprdus_load_user_config()`, both CLIs, SAPI5, NVDA, Android (`LaprdusTTS.loadAccentLexicon`) and Apple (`DictionaryState.accentLexiconURL`) load it with the user dictionaries, and user dictionaries off removes it. Every platform's dictionary editor offers it as the fourth dictionary type (Windows `DictionaryType::Accents` with the entry editor of `dictionary_dialog.rs`, Android `DictionaryType.ACCENTS`, Apple `DictionaryType.accents`): an entry is word or verb (`verb` flag) plus `language` and `comment`, its text kept in the entry's grapheme field, written under the `word`/`verb` keys; before saving, the editor checks the entry with the engine's own parser (`laprdus_check_accent_lexicon()`, handle-free, returns the number of rejected entries and the first reason; Windows calls `UserLexicon::parse()` directly, Android `LaprdusTTS.checkAccentLexicon`, Apple `LaprdusEngine.checkAccentLexicon`) and shows the reason instead of saving. Duplicating an accent entry does not append " (copy)". See "Stress" in `docs/formant.md` and 5.11 in `docs/laprdus.md`.
- **Croatian tiflo- compounds** follow the user's requested two-component pronunciation, including established dictionary terms. `split_tiflo()` (`src/common/languages/hr/clause.rs`) splits after `tiflo` when the remaining letters contain a vowel or syllabic r; both parts get their ordinary accent analysis, and the second has `joined = true` so they share one prosodic word without an internal boundary pause. Lowercase, uppercase and inner capitals behave alike. The short falling `t^iflo` is a listener preference; HJP gives single accents in *tiflòlog, tiflològija, tiflòloškī*. Written accent marks and the user's whole-word exact, stem and verb entries override the split. See the source discussion in `docs/formant.md`.
- **Frontend structure tests** (`src/common/languages/tests.rs`, `src/common/languages/sl/tests.rs`, `src/common/text/frontend/tests.rs`) check accents and joined word boundaries directly. They are unit tests inside the engine crate, because these internal items are not public: `cargo test --release -p laprdus --lib`. Run these alongside the synthesis tests when changing frontend structure. The commands are in `docs/formant.md`, section 7.
- Platform code written in Rust does not go through `laprdus_set_voice()`: it calls `Engine::set_voice()` (the SAPI5 driver) or `Engine::set_voice_from()` (the Android JNI library, which reads a recorded voice's file from the APK's assets). Both load a formant voice without data (`Voice::is_formant()`) and keep the language chosen with `Engine::set_language()`; call `set_language()` when the host chose one.

**Singing presets** (`SingingStyle`, `src/common/formant/singing.rs`; the presets and their songs in `src/common/languages/<code>/formant/mod.rs`): `.claude/rules/synthesis.md`.

```bash
B=target/release      # after: cargo xtask data && cargo build --release -p laprdus-cli
$B/laprdus -D data/voices -v zvonko "Dobar dan, kako ste?"
$B/laprdus -D data/voices -v stojan -o out.wav "Добар дан!"
$B/laprdus -D data/voices -v mirsad -r 2.0 "Brzo, ali razgovijetno."
$B/laprdus -D data/voices -v zvonko -L sr "Zvonko govori srpski: dve hiljade."
$B/laprdus -D data/voices -v vlado -L hr "Vlado govori hrvatski: dvije tisuće."
```

### Telephone numbers, currency amounts, short pauses, dates

Telephone numbers (`src/common/text/numbers/phone.rs`), currency amounts (`currency.rs`), the short pauses inside a clause of clock times, dates and cell addresses (`src/common/text/marks.rs`) and every written form of a date (`dates.rs`, mirrored by the NVDA driver's `_dates.py`: change the two together): `.claude/rules/numbers-dates.md`.

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

The header is `src/platform/c_api/include/laprdus/laprdus_api.h`, written by hand (`tests/header.rs` of the crate checks it against what the library exports); the functions are in `src/platform/c_api/src/lib.rs` (crate `laprdus-capi`). Every exported function checks its pointers and catches a panic, which becomes the function's error value. The C API is what hosts in other languages use: the NVDA add-on (through the SAPI5 DLLs, which export it) and the Apple app and extension (through `liblaprdus_capi.a`). The Rust programs do not go through it: the CLI, the SAPI5 driver, the Speech Dispatcher module and the Android JNI library use `laprdus::engine::Engine` directly (the Configurator the engine's `config` and `params` modules), so where this file names a `laprdus_*` function for what every platform does, they call the `Engine` method or set the `VoiceParams` field behind it.

### Stopping a synthesis, and audio clause by clause

`Engine::cancel_handle()` and `laprdus_cancel()` (the one function that may be called from another thread while the handle's thread synthesizes), `Engine::synthesize_streaming()` and `laprdus_synthesize_streaming()`; what every platform does with them, and the SAPI5 bookmarks: `.claude/rules/streaming.md`.

## Performance and audio processing

Performance (`cargo run --release --example bench`; a recursive filter fed exact zeros stalls at denormal numbers and must be cleared, as `frication_rung_out()` does), the recorded voices (TD-PSOLA, clarity) and the dual pitch system (`pitch` the voice character, `user_pitch` the user's preference): `.claude/rules/synthesis.md`.

## Dependencies

- **Rust 1.85 or newer** (rustup), edition 2024; Cargo is the build system
- Windows: the MSVC build tools (the linker of the `x86_64-pc-windows-msvc` and `i686-pc-windows-msvc` targets; `rustup target add` both). No ATL: COM is written with the `windows` crate. The build for Windows 7 SP1, 8 and 8.1 also needs the nightly toolchain named in `src/platform/windows/legacy/toolchain.txt` with the standard library's sources (`rustup toolchain install nightly-2026-10-30 --component rust-src`); nothing else uses a nightly
- Crates (`Cargo.lock` pins them): the engine uses `regex` and `serde_json`; the CLI `rodio` for playback (WASAPI, CoreAudio, ALSA); the SAPI5 driver `windows`, `windows-core` and `windows-registry`; the Configurator `windows`; both Windows crates `embed-resource` in their build scripts, which compiles `laprdus_sapi5.rc` and `laprdus_config.rc`; the Android JNI crate `jni`
- Linux: `pkg-config` and the ALSA development files (`libasound2-dev`) for the CLI's playback; the Speech Dispatcher module needs no Speech Dispatcher headers
- Android: the Android SDK and NDK, `cargo-ndk` and the four Android Rust targets
- Apple: Xcode 26+ and the Apple Rust targets (see "Apple" below)
- SCons and Python 3.9+ (for the NVDA addon build only: `pip install scons`)
- InnoSetup 6+ (for SAPI5 installer) - https://jrsoftware.org/isinfo.php
- No external audio processing libraries (all synthesis and signal processing is internal; `rodio` only plays the CLI's output)

## Environment Configuration (Developer Machine)

### Tool Paths

| Tool | Path |
|------|------|
| **InnoSetup ISCC** | `"C:\Program Files (x86)\Inno Setup 6\ISCC.exe"` |
| **JAVA_HOME** | `C:\Program Files\Android\Android Studio\jbr` |
| **JAVA_HOME (macOS)** | `/Applications/Android Studio.app/Contents/jbr/Contents/Home` |
| **adb (macOS)** | `~/Library/Android/sdk/platform-tools/adb` |

On the macOS machine `./gradlew` runs directly from `src/platform/android/` once `JAVA_HOME` points at the Android Studio JBR; no wrapper or path tricks are needed.

### Running InnoSetup from Command Line

```bash
# Use full path with quotes due to spaces
"/c/Program Files (x86)/Inno Setup 6/ISCC.exe" installers/windows/laprdus_sapi5.iss
```

### Setting JAVA_HOME for Android Builds

```bash
# Set JAVA_HOME before running Gradle
export JAVA_HOME="/c/Program Files/Android/Android Studio/jbr"
cd src/platform/android && ./gradlew assembleRelease

# macOS
export JAVA_HOME="/Applications/Android Studio.app/Contents/jbr/Contents/Home"
cd src/platform/android && ./gradlew assembleRelease
```

## Building

Everything is built by Cargo. `cargo xtask <step>` (an alias in `.cargo/config.toml` for `cargo run --quiet --release -p xtask --`) runs the steps Cargo does not do by itself; the options are those of `tools/xtask/src/main.rs` and `install.rs`:

| Step | What it does |
|------|--------------|
| `cargo xtask data` | Packs the recordings `data/<language>/<Voice>/*.wav` into `data/voices/<Voice>.bin` and stages the dictionaries in `data/dictionary/`: `internal.json` (from `data/hr/dictionary.json`), `spelling.json`, `emoji.json`, and `emoji_sl.json` (from `data/sl/emoji.json`). `--key HEX` or `LAPRDUS_PHONEME_KEY`: the key the voice files are packed with |
| `cargo xtask windows` | Builds the SAPI5 driver, the CLI and the Configurator for x64 and x86 and stages them in `dist/windows-x64/` (`laprd64.dll`, `laprdus.exe`, `laprdgui.exe`) and `dist/windows-x86/` (`laprd32.dll`, `laprdus.exe`, `laprdgui.exe`), where the installer and the NVDA add-on take them from |
| `cargo xtask windows-legacy` | Builds the same three programs for Windows 7 SP1, 8 and 8.1 (targets `x86_64-win7-windows-msvc`, `i686-win7-windows-msvc`, the pinned nightly, `-Zbuild-std`) and stages them under the same names in `dist/windows-legacy-x64/` and `dist/windows-legacy-x86/`, where the installer and the NVDA add-on take them from |
| `cargo xtask build` | Linux, macOS: runs `data`, then builds the CLI, the C API library and, on Linux, the Speech Dispatcher module |
| `cargo xtask install` | Linux, macOS: builds and installs them with the C API header, the voice data and the dictionaries: `--destdir DIR` (a package's staging root), `--prefix DIR` (default `/usr/local`), `--libdir DIR`, `--moduledir DIR` |
| `cargo xtask dist` | Linux, macOS: the release tarball with its install script, in `dist/<os>-<arch>/` |

`build`, `install` and `dist` also take `--universal` and `--codesign-identity ID` (macOS), `--locked`, `--offline`, `--frozen` (handed on to Cargo) and `--key HEX`.

### Automated Build System (Recommended)

**IMPORTANT: Use the master build script to ensure all dependencies are built correctly.**

The build system automatically handles:
1. **Voice data generation** - The recordings are packed into `.bin` files in `data/voices/`
2. **Library rebuilding** - DLLs/SOs are rebuilt with latest engine changes
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

The scripts kept their command names; they call `cargo xtask` now.

#### What the Build System Does Automatically

- `cargo xtask build`, `install` and `dist` and the Android Gradle build run `cargo xtask data` themselves.
- `cargo xtask windows` and `cargo xtask windows-legacy` do not: run `cargo xtask data` before building the installer or the NVDA add-on (the master script does).
- The voice files are packed by the engine's own `pack()` (`src/common/concatenative/voice_data.rs`); there is no separate packing tool to build.

The SAPI5 installer, NVDA addon, and Android builds all source voice data from `data/voices/` and the dictionaries from `data/dictionary/`.

#### Build Dependencies Flow

```
data/hr/Josip/*.wav  ──┐
                       ├──▶ cargo xtask data ──▶ data/voices/Josip.bin
data/sr/Vlado/*.wav  ──┘                         data/voices/Vlado.bin
data/hr/*.json, data/sl/emoji.json ──────────▶   data/dictionary/*.json
                                                                            │
        ┌───────────────────────────────────────────────────────────────────┤
        │                           │                           │           │
        ▼                           ▼                           ▼           ▼
   SAPI5 Installer            NVDA Addon                  Android APK    Linux Install
  (data/voices/*.bin)    (copies to addon dir)          (assets/voices)  (/usr/share)
```

### Prerequisites

1. **Rust 1.85+** (rustup) with the targets of the platform (Windows: `rustup target add x86_64-pc-windows-msvc i686-pc-windows-msvc`) and, on Windows, the **MSVC build tools** (Visual Studio 2019+ with the C++ Desktop workload: the linker)
2. **SCons**: `pip install scons` (NVDA add-on only)
3. **InnoSetup 6+**: Install from https://jrsoftware.org/isdl.php (for SAPI5 installer)

### Build Commands

All builds run from the project root directory.

#### Windows: SAPI5 DLLs, CLI and Configurator

```bash
cargo xtask data       # voice data and dictionaries
cargo xtask windows    # x64 and x86, release
cargo xtask windows-legacy   # the same for Windows 7 SP1, 8 and 8.1 (dist/windows-legacy-*)
```

Output: `dist/windows-x64/laprd64.dll`, `dist/windows-x86/laprd32.dll`, and `laprdus.exe` and `laprdgui.exe` in both folders.

One crate for the host architecture, while working on it:

```bash
cargo build --release -p laprdus-sapi5          # target/release/laprdus_sapi5.dll
cargo build --release -p laprdus-configurator   # target/release/laprdgui.exe
cargo build -p laprdus-sapi5                    # debug build in target/debug/ (too slow for a screen reader)
```

#### Windows 7 SP1, 8 and 8.1, the installer, the Configurator's languages

The legacy build (`cargo xtask windows-legacy` with the nightly named in `src/platform/windows/legacy/toolchain.txt`; the code is the same in both builds: no Windows-version branches), the SAPI5 installer (`iscc installers/windows/laprdus_sapi5.iss`; five languages, a new message goes into all five) and the Configurator's string tables: `.claude/rules/windows.md`.

#### CLI

There is one command-line program, `laprdus` (`src/platform/cli`), for Windows, Linux and macOS; its usage and options: `.claude/rules/cli.md`.

#### NVDA Addon

Built with SCons only (`cd src/platform/windows/nvda && scons`; see "CRITICAL: NVDA Addon Build Requirements" below); its resources, menu, manuals and localization: `.claude/rules/windows.md`.

### Clean Build

```bash
cargo clean                                # Everything Cargo built (target/)
rm -rf dist data/voices data/dictionary    # The staged files and the generated data; cargo xtask rebuilds them
```

### Packages and targets

| Package (`-p`) | Builds |
|----------------|--------|
| `laprdus` | The engine library (root crate, `src/common`) |
| `laprdus-capi` | The C API: `laprdus_capi.dll` / `liblaprdus_capi.so` / `.dylib` and the static library |
| `laprdus-cli` | `laprdus` (command-line program) |
| `laprdus-sapi5` | `laprdus_sapi5.dll`, staged as `laprd64.dll` / `laprd32.dll` |
| `laprdus-configurator` | `laprdgui.exe` |
| `laprdus-win7` | The delay-load hook of the build for Windows 7 SP1, 8 and 8.1 (`src/platform/windows/legacy/compat`); linked by the two crates above only for the `*-win7-windows-msvc` targets |
| `laprdus-speechd` | `sd_laprdus` |
| `laprdus-android` | `liblaprdus.so` for the Android app (built by Gradle with cargo-ndk) |
| `xtask` | The build steps (`cargo xtask`) |

| Platform | Rust targets |
|----------|--------------|
| Windows | `x86_64-pc-windows-msvc`, `i686-pc-windows-msvc` (`cargo xtask windows` builds both) |
| Windows 7 SP1, 8, 8.1 | `x86_64-win7-windows-msvc`, `i686-win7-windows-msvc` (`cargo xtask windows-legacy` builds both, with the pinned nightly) |
| Linux | the host's target |
| macOS | the host's target; `--universal` builds `aarch64-apple-darwin` and `x86_64-apple-darwin` |
| Android | `aarch64-linux-android`, `armv7-linux-androideabi`, `x86_64-linux-android`, `i686-linux-android` |
| iOS | `aarch64-apple-ios`, `aarch64-apple-ios-sim`, `x86_64-apple-ios` |

Release builds (`--release`: `opt-level = 3`, LTO, one codegen unit) are what every platform ships and what the tests run; a debug build (`cargo build` without `--release`, in `target/debug/`) is for debugging only.

## Testing by hand

The SAPI5 voices from PowerShell and the NVDA add-on: `.claude/rules/windows.md`. The Cargo and Python tests are under "Engine and Program Tests (Cargo)" below.

## Android

The Gradle project `src/platform/android/`: building, toolchain and Play requirements, the settings screen and TalkBack, Direct Boot storage, device testing: `.claude/rules/android.md`.

## Linux

The Speech Dispatcher module, building, dependencies, installation and packages: `.claude/rules/linux.md`; the CLI: `.claude/rules/cli.md`.

## Full Rebuild Instructions

When making changes to core engine components, pronunciation dictionary, or voice data, rebuild all platforms using these commands:

### Complete Rebuild (All Platforms)

```bash
# From project root directory

# 1. Voice data and dictionaries
cargo xtask data

# 2. Build Windows SAPI5 DLLs, CLI and Configurator (both architectures)
cargo xtask windows
cargo xtask windows-legacy   # the same for Windows 7 SP1, 8 and 8.1 (dist/windows-legacy-*)

# 3. Build the NVDA add-on (the installer carries it), then the SAPI5 installer (requires InnoSetup 6+)
(cd src/platform/windows/nvda && scons)
iscc installers/windows/laprdus_sapi5.iss

# 4. Build Linux (C API library, CLI, Speech Dispatcher module); on Linux or in WSL
cargo xtask build

# 5. Build Android APK (requires JDK 17+, the Android SDK and NDK, cargo-ndk)
cd src/platform/android
./gradlew assembleRelease
# or for debug: ./gradlew assembleDebug
cd ../../..

# 6. Build NVDA Addon (optional)
(cd src/platform/windows/nvda && scons)
```

### Output Locations

| Platform | Output File |
|----------|-------------|
| Windows SAPI5 x64 | `dist/windows-x64/laprd64.dll` |
| Windows SAPI5 x86 | `dist/windows-x86/laprd32.dll` |
| Windows CLI and Configurator | `dist/windows-x64/laprdus.exe`, `laprdgui.exe`; the same in `dist/windows-x86/` |
| Windows 7 SP1, 8, 8.1 | `dist/windows-legacy-x64/` and `dist/windows-legacy-x86/`: the same file names |
| Windows Installer | `installers/windows/Output/Laprdus_SAPI5_Setup_2.0.0.exe` |
| Linux Library | `target/release/liblaprdus_capi.so` (installed as `liblaprdus.so`) |
| Linux CLI | `target/release/laprdus` |
| Linux Speech Dispatcher | `target/release/sd_laprdus` |
| Linux / macOS tarball | `dist/<os>-<arch>/laprdus-<version>-<os>-<arch>.tar.xz` |
| Voice Data | `data/voices/Josip.bin`, `Vlado.bin` |
| Android APK (debug) | `src/platform/android/app/build/outputs/apk/debug/app-debug.apk` |
| Android APK (release) | `src/platform/android/app/build/outputs/apk/release/app-release-unsigned.apk` (the project has no signing config) |
| NVDA Addon | `src/platform/windows/nvda/laprdus-*.nvda-addon` |

### Dictionary Updates

The pronunciation dictionary `data/hr/dictionary.json` (staged by `cargo xtask data` as `data/dictionary/internal.json`) is automatically included in all builds:
- **Windows SAPI5**: Copied to install directory as `dictionary.json`
- **Android**: Included in APK assets as `internal.json`
- **NVDA Addon**: Copied to addon directory as `internal.json`

The spelling dictionary `data/hr/spelling.json` (staged as `data/dictionary/spelling.json`) is also included in all builds:
- **Windows SAPI5**: Copied to install directory as `spelling.json`
- **Android**: Included in APK assets as `spelling.json`
- **NVDA Addon**: Copied to addon directory as `spelling.json`

The emoji dictionaries `data/hr/emoji.json` (Croatian names, CLDR hr edited; used for hr, sr and bs; staged as `data/dictionary/emoji.json`) and `data/sl/emoji.json` (Slovenian names, the CLDR sl `tts` annotations for the same emoji; staged as `data/dictionary/emoji_sl.json`) ship together everywhere (SAPI5 installer, NVDA add-on, Android assets, both Apple targets, the Linux packages and the macOS tarball). The engine loads `emoji_<short code>.json` by itself next to the `emoji.json` it is given (`laprdus_load_emoji_dictionary()`; Android loads the asset through `Engine::load_language_emoji_dictionary()`), uses it while that language is spoken and lets the user's emoji entries (`laprdus_append_emoji_dictionary()`) win in every language.

No manual copying is required - the build systems handle this automatically.

## Claude Code: Post-Change Rebuild Automation

**IMPORTANT**: After completing any major change to the codebase (the Rust engine, NVDA addon, dictionaries, or build configuration), Claude Code MUST automatically rebuild and test all platforms before considering the task complete.

### When to Trigger Full Rebuild

Rebuild all platforms after changes to:
- **The engine** (`src/common/`, the languages in `src/common/languages/` and their generated `tables/` included)
- **C API** (`src/platform/c_api/`)
- **SAPI5 driver** (`src/platform/windows/sapi5/`) and **Configurator** (`src/platform/windows/configurator/`)
- **NVDA addon** (`src/platform/windows/nvda/addon/synthDrivers/laprdus/`)
- **Dictionaries** (`data/hr/*.json`, `data/sl/emoji.json`)
- **Voice data** (`data/hr/Josip/`, `data/sr/Vlado/`)
- **Build configuration** (`Cargo.toml`, `Cargo.lock`, `.cargo/config.toml`, `tools/xtask/`, `build.gradle.kts`)
- **Android JNI library** (`src/platform/android/native/`)
- **CLI** (`src/platform/cli/`)
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
# 1. Generate voice data and stage the dictionaries (REQUIRED FIRST STEP)
cargo xtask data

# 2. Build SAPI5 DLLs, CLI and Configurator (both architectures)
cargo xtask windows
cargo xtask windows-legacy   # the same for Windows 7 SP1, 8 and 8.1 (dist/windows-legacy-*)

# 3. Run the tests (release: the suites render a lot of audio)
cargo test --release --workspace

# 4. Build the NVDA add-on (the installer carries it), then the SAPI5 installer
(cd src/platform/windows/nvda && scons)
"/c/Program Files (x86)/Inno Setup 6/ISCC.exe" installers/windows/laprdus_sapi5.iss

# 5. Build NVDA Addon (automatically copies DLLs, voice data, and dictionaries)
(cd src/platform/windows/nvda && scons)

# 6. The Python tests of the Windows programs and the NVDA driver
python tests/nvda/test_driver.py
python tests/nvda/test_manuals.py
python tests/windows/test_configurator_language.py
python tests/windows/test_sapi5_driver.py
python tests/windows/test_legacy_imports.py
python tests/nvda/test_python37.py

# 7. Build Linux (C API library, CLI, Speech Dispatcher module); on Linux or in WSL
cargo xtask build

# 8. Build Android APK
cd src/platform/android && export JAVA_HOME="/c/Program Files/Android/Android Studio/jbr" && ./gradlew assembleDebug && cd ../../..
```

### CRITICAL: NVDA Addon Build Requirements

**ALWAYS use SCons to build the NVDA addon:**
```bash
(cd src/platform/windows/nvda && scons)
```

**NEVER do ANY of the following - these will corrupt the addon:**
- NEVER manually create the .nvda-addon file using Python's zipfile, 7-Zip, or any archive tool
- NEVER manually edit `addon/manifest.ini` - it is AUTO-GENERATED by SCons from `buildVars.py`
- NEVER manually edit `addon/locale/*/manifest.ini` - these are AUTO-GENERATED from translations
- NEVER manually edit `addon/doc/en/readme.html` - it is AUTO-GENERATED from `readme.md`

**To make changes:**
- Addon metadata (name, version, description): Edit `src/platform/windows/nvda/buildVars.py`
- Documentation: Edit the manuals listed above (`src/platform/windows/nvda/readme.md` is the English one)
- Translations: Edit `.po` files in `src/platform/windows/nvda/addon/locale/*/LC_MESSAGES/`

SCons handles all file generation automatically. Manual edits to generated files will be overwritten or cause corruption.

### Automatic Resource Handling

**The build system now automatically handles all resource copying:**

- **Voice Data**: `cargo xtask build`, `install`, `dist` and the Android Gradle build run `cargo xtask data` themselves, which packs the `.bin` files into `data/voices/` and stages the dictionaries in `data/dictionary/`; before `cargo xtask windows`, the installer and the NVDA add-on, run it once
- **NVDA Addon**: The NVDA SCons build automatically copies:
  - DLLs from `dist/windows-x64/laprd64.dll` and `dist/windows-x86/laprd32.dll`
  - GUI executables from `dist/windows-x64/laprdgui.exe` (as `laprdgui64.exe`) and `dist/windows-x86/laprdgui.exe` (as `laprdgui32.exe`)
  - The 32-bit legacy driver and Configurator from `dist/windows-legacy-x86/` (as `laprd32_legacy.dll` and `laprdgui32_legacy.exe`)
  - Voice data from `data/voices/`
  - Dictionaries from `data/dictionary/`
- **SAPI5 Installer**: InnoSetup sources the programs from `dist/windows-x64/` and `dist/windows-x86/`, voice data from `data/voices/` and dictionaries from `data/dictionary/`
- **Android**: Gradle tasks automatically copy voice data and dictionaries to `assets/` subdirectories

**No manual file copying is required** when using the build scripts or the `cargo xtask` steps correctly.

**Android**: Gradle builds the native library from the Rust sources with cargo-ndk on every build (`buildNativeLibrary`), so no manual copying is needed - it always uses the latest engine code.

### Post-Build Testing

After rebuilding, launch installers for user testing:

```bash
# Launch SAPI5 installer
start "" "installers/windows/Output/Laprdus_SAPI5_Setup_2.0.0.exe"

# Launch NVDA addon installer
start "" "src/platform/windows/nvda/laprdus-2.0.0.nvda-addon"
```

### Android Device Testing

**IMPORTANT**: After building the Android APK, ALWAYS install and test on the user's physical device.

ADB paths, the commands, the instrumented tests and the checklist: `.claude/rules/android.md`.

### Engine and Program Tests (Cargo)

The tests of the engine and of the programs are Cargo tests and run the same way on Windows, Linux and macOS (all three pass since 2026-11-23, the first run on macOS since the migration); GitHub Actions runs them on the three on every push (see "GitHub Actions" below).

#### Build and Test Commands

```bash
# The recorded voices and the bundled dictionaries the tests read
cargo xtask data

# Everything (release, because the suites render a lot of audio)
cargo test --release --workspace

# One suite
cargo test --release --test formant               # tests/formant.rs: formant voices (96 cases)
cargo test --release --test recorded              # tests/recorded.rs: recorded voices (28 cases; rate, pitch, derived voices, melody, streaming)
cargo test --release -p laprdus --lib             # unit tests inside the engine: front end, numbers, tables
cargo test --release -p laprdus-cli --test cli    # src/platform/cli/tests/cli.rs: the CLI (14 cases)
cargo test --release -p laprdus-capi              # src/platform/c_api/tests/c_api.rs (27 cases) and header.rs
cargo test --release -p laprdus-speechd           # the unit tests of the Speech Dispatcher module (3 cases)

# One test by (a part of) its name
cargo test --release --test formant slovenian_is_spoken_by_every_voice

# Test CLI manually
cargo build --release -p laprdus-cli
target/release/laprdus -D data/voices -o /tmp/test.wav "Dobar dan!"
```

What is where:

- `tests/formant.rs` and `tests/recorded.rs` are the former C++ test programs of the formant and of the recorded voices; a test function carries the old test's title in snake case (`voice_quality_settings_shape_the_voice_and_50_leaves_it_alone`). There are no tags any more: a part of a name selects tests.
- The cases of the former front end test program are unit tests in `src/common/languages/tests.rs` and `src/common/languages/sl/tests.rs` (plus `src/common/text/frontend/tests.rs`), those of the number converter's in `src/common/text/numbers/tests.rs`: inside the engine crate, where its internal items are visible.
- `src/platform/c_api/tests/header.rs` checks that the hand-written header declares exactly what the library exports.
- The recorded voices need the packed recordings: `data/voices/Josip.bin` and `Vlado.bin` from `cargo xtask data`, or the directory `LAPRDUS_DATA` names; without them those tests fail and say so. The bundled dictionaries are read from `data/dictionary`.
- `LAPRDUS_TEST_INFO=1` (with `-- --nocapture`) prints the measured values behind the thresholds.
- **Against the C++ engine.** `tests/dsp_against_cpp.rs`, `tests/frontend_against_cpp.rs` and `tests/numbers_against_cpp.rs` compare the formant synthesizer (since 2026-11-17 without the clauses that have a z or ž, and without the recorded voices, which sound different on purpose), the front end and the number converter with dumps the last C++ engine wrote. The dumps are local files in `build/golden` (`LAPRDUS_GOLDEN` names another folder for the front end and the numbers), too large for the repository, and the program that wrote them went with the C++ sources; **the three tests pass without the dumps**, having checked nothing. `tools/golden/golden.py check <library> <data dir> <dictionary dir> [set ...]` renders the corpus `tests/golden/corpus.jsonl` through the C API (the library is `laprdus-capi`'s, e.g. `target/release/laprdus_capi.dll`) and compares it with the hashes in `tests/golden/*.tsv`, recorded from the C++ engine on Windows x64 before the port. One intended difference: a spelled `]` is now named (the C++ spelling dictionary parser could not read that entry and left it silent).
- Python tests: `python tests/nvda/test_driver.py`, `python tests/nvda/test_manuals.py`, `python tests/windows/test_configurator_language.py`, `python tests/windows/test_sapi5_driver.py` (the last needs `cargo xtask windows` and `comtypes`), `python tests/windows/test_legacy_imports.py` (needs `cargo xtask windows-legacy`), `python tests/nvda/test_python37.py`.

#### Linux tests and packages

The Linux checklist, the package tests and the RPM build: `.claude/rules/linux.md`.

### GitHub Actions

Two workflows build and test on GitHub's machines (free for the public repository, since 2026-11-23). Nothing is signed, and only the Linux workflow publishes anything (a draft release, on a tag).

| Workflow | When | What |
|----------|------|------|
| `.github/workflows/build.yml` "Build and test" | a push to `dev` or `main` (not one that changes only `docs/`, `.claude/`, `CLAUDE.md`, `AGENTS.md` or the `changes-*.md` files), a pull request, by hand (`gh workflow run build.yml --ref dev`) | `cargo test --release --workspace` on `ubuntu-24.04`, `windows-2025` and `macos-26`. **Windows:** `cargo xtask windows` and `windows-legacy`, the NVDA add-on (`scons`), the installer (Inno Setup 6.7.1 from Chocolatey: GitHub's Windows images have none), and every Python test of `tests/windows` and `tests/nvda`; the SAPI5 and the NVDA driver tests run with a 64-bit and with a 32-bit Python, the SAPI5 test also on the legacy x64 driver. **Android:** the debug and the release APK, the unit tests, lint. **Apple:** the macOS app (Release, arm64 and x86_64) and the iPhone app without signing (`CODE_SIGNING_ALLOWED=NO`), the unit tests in an iPhone simulator. The installer, the add-on and the debug APK are kept as the run's artifacts for 14 days |
| `.github/workflows/linux-packages.yml` "Linux packages" | see `.claude/rules/linux.md` | the .deb, .rpm, Arch package and tarball, built and tested in containers of their distributions |

Not covered: listening; NVDA, VoiceOver and TalkBack by hand; the phones; Windows 7, 8 and 8.1 themselves (their build is checked by its import tables and the driver test); the Android instrumented tests (several need TalkBack, which the emulator images lack); signing, notarization and the stores, which stay on the maintainer's machines. A failed run: `gh run list --limit 5`, then `gh run view <id> --log-failed`.

### Build Verification Checklist

Before marking a task complete, verify:
- [ ] `cargo xtask windows` builds the SAPI5 x64 and x86 DLLs, the CLI and the Configurator without errors
- [ ] `cargo xtask windows-legacy` builds the same for Windows 7 SP1, 8 and 8.1, and `python tests/windows/test_legacy_imports.py` passes
- [ ] `cargo test --release --workspace` passes
- [ ] SAPI5 installer builds successfully
- [ ] NVDA addon builds successfully
- [ ] `python tests/nvda/test_driver.py` and the other Python tests pass
- [ ] Linux: `cargo xtask build` builds the library, the CLI and the Speech Dispatcher module without errors
- [ ] Linux packages: `scripts/linux-packages.sh` (Docker, any host) or the "Linux packages" GitHub workflow builds the .deb, .rpm, Arch package and tarball and tests each on its distributions (`.claude/rules/linux.md`)
- [ ] After a push: the "Build and test" workflow passes for the commit (`gh run list --workflow build.yml --limit 3`); it covers the items above that a machine of the maintainer's is not at hand for
- [ ] Android APK builds successfully (warnings OK)
- [ ] Android APK installed on user's physical device
- [ ] Installers launched for user testing

## Pronunciation Dictionary

### Format

The dictionary uses JSON format; its source is `data/hr/dictionary.json`, which `cargo xtask data` stages as `data/dictionary/internal.json`:

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

## Spelling

`Engine::synthesize_spelled()` (C API `laprdus_synthesize_spelled()`) reads text character by character; a letter by its sound by default (`SpellingMode::LetterSounds`) or by its name in the language set; the spelling dictionary `data/hr/spelling.json` holds digits, punctuation and symbols only. Everything else: `.claude/rules/spelling.md`.

## macOS and Apple

The macOS library and CLI (`cargo xtask build`, `--universal`), the macOS release (`scripts/build-macos-release.sh`) and the Apple app and speech extension (`src/platform/apple/Laprdus/`): `.claude/rules/apple.md`.
