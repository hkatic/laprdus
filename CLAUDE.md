# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

LaprdusTTS is a Croatian/Serbian/Bosnian/Slovenian text-to-speech (TTS) engine supporting:
- **SAPI5** (Windows Speech API) for system-wide TTS integration
- **NVDA** screen reader synthesizer driver

It has two kinds of voices:
- **Concatenative** voices (Josip, Vlado and the voices derived from them) are rendered from pre-recorded phoneme WAV files by TD-PSOLA. See "Recorded Voices" below and `docs/concatenative.md`.
- **Formant** voices (Zvonko, Stojan, Mirsad) are synthesized entirely by rule and need no data files. See "Formant Voices" below.

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
| `src/platform/apple/` | The Apple app and speech extension (Xcode project in `Laprdus/`), `build-rust-engine.sh`; not yet built on a Mac since the migration |
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
- **Wider ranges; inflection level and acceleration are for every voice.** `VoiceParams::clamp()` allows speed and `user_pitch` from 0.25 to 4.0; the concatenative `ConcatSynthesizer` narrows them to its 0.5-4.0 / 0.5-2.0 itself, so every platform may pass the wide range and let the engine sort it out. `inflection_level` (0-1, default 0.5) scales the whole F0 contour in semitones (0 is a monotone, 1 doubles every movement) and `acceleration` (0.5-3.0, default 1.0) multiplies the speed (`VoiceParams::effective_speed()`, capped at 8.0; the concat planner narrows the product to 0.5-4.0), so a host's rate slider reaches a higher or lower top rate. Both apply to the recorded voices too (`src/common/concatenative/prosody.rs`), are `laprdus_set_inflection_level()` / `laprdus_set_acceleration()` in the C API, `speech.inflection_level` / `speech.acceleration` in `settings.json`, keys `inflection_level` / `acceleration` on Android and Apple, and every settings screen shows them with the voice settings, for every voice (on Android under "More voice settings", collapsed by default). `laprdus_get_nominal_wpm(voice)` (175 words/min for Zvonko at speed 1.0, times the voice's tempo) lets a UI print the top rate in words per minute: nominal × 2.0 (the host slider's top) × the Laprdus rate (the rate alone when forced) × acceleration, capped at 8.0 (recorded voices 4.0). The Speech Dispatcher module maps SSIP `pitch_range` onto the inflection level. The NVDA add-on uses NVDA's own controls instead: NVDA's Inflection slider sets the inflection level (0-100 → 0-1, inflection always enabled; `speech.inflection` and `speech.inflection_level` are not read), and Rate Boost multiplies every rate slider position by 3 through the acceleration (`speech.acceleration` is not read), as other synthesizers' rate boost does.
- **Voice quality: head size, breathiness, roughness, brightness** (`VoiceParams::head_size` etc., integers 0-100, default 50 = the voice as its tables define it, so the defaults change nothing; `laprdus_set_head_size()` / `laprdus_get_head_size()` and the same for `breathiness`, `roughness`, `brightness` in the C API; `speech.head_size` ... in `settings.json`; keys `head_size`, `breathiness`, `roughness`, `brightness` on Android and Apple; CLI `--head-size`, `--breathiness`, `--roughness`, `--brightness PERCENT`; sliders after the volume boost on Android (under "More voice settings") and Apple, a "Voice quality..." dialog under the volume boost in the Windows Configurator for SAPI5, and four `NumericDriverSetting` sliders of the NVDA add-on's own, `headSize`, `breathiness`, `roughness`, `brightness`, in NVDA's voice settings after the volume and in the synth settings ring, saved by NVDA; the Configurator's four apply to SAPI5 only, as its inflection settings do, and the NVDA driver does not read those keys of `settings.json`). After the voice-parameter model of a classic formant synthesizer (gender, head size, pitch baseline, pitch fluctuation, roughness, breathiness, speed, volume, each 0-100, read from its binary in November 2026: pitch baseline is 40-422 Hz exponential, speed 70-1297 words/min, volume about 1 dB per unit; pitch baseline, fluctuation, speed and volume are Laprdus's pitch, inflection level, rate and volume already). Head size multiplies every formant by `head_size_factor()` (0.71 at 0, 1.4 at 100, `HEAD_SIZE_SPAN`; 0.8 and 1.25 until a listener found the steps hard to hear, November 2026: the higher value is the higher voice, at the maintainer's word, the reverse of synthesizer A; `src/common/params.rs`): the formant voices through `FormantVoice::formant_scale` of a per-utterance copy of the voice (`FormantVoice::with_quality()` in `src/common/formant/voice.rs`; `FormantVoice::quality()` also scales the fixed F5-F8), the recorded voices through the bank warp (`ConcatSynthesizer::ensure_bank()`, rebuilt only when the warp moves; the pitch stays). Breathiness runs from a pressed, hard voice to a whisper (2026-11-07; a listener heard nothing from the old 0-5 times scale of the voice's faint breath noise): below 50 `breathiness_tension()` makes a pressed, hard voice: it closes the glottis for longer (open quotient towards 0.30), flattens the source by up to 8 dB, raises the source emphasis towards `BREATHINESS_TENSE_EMPHASIS` 0.88 and gives up `BREATHINESS_TENSE_BASS_CUT` 80 % of the low shelf (a paragraph moved 0.4 dB with the open quotient and tilt alone, inaudible; now 3.7 dB), and takes the voice's own breath noise away (`breathiness_noise_scale()`); above 50 `breathiness_whisper()` opens it (towards 0.85), steepens the source by up to 10 dB, widens B1 by up to 150 Hz, takes the voicing away with the square of the share and adds breath through the cascade (`WHISPER_BREATH` 0.6 times the voicing amplitude, unmodulated by the glottal cycle), so 100 is a whisper (`VoiceQuality::whisper` / `tension`, `src/common/formant/klatt.rs`; measured in "Breathiness, from a tense voice to a whisper" in `docs/formant.md`); roughness multiplies the jitter and shimmer by `voice_quality_scale()` (0 at 0, 1 at 50, 5 at 100) times up to 1.6 (eight times at 100), from 50 up adds diplophonia, every second glottal period up to 10 % shorter and 80 % weaker (`VoiceQuality::roughness` from `roughness_diplophonia()`, the square root of the distance above 50 so that 60 is heard already; 5 % and 60 % until a listener found 75 and 100 too faint on a phone, November 2026; five times the random jitter alone was not heard), and below 50 also takes the slow pitch drift (`VoiceQuality::flutter`) and the breath noise away, so 0 is a perfectly steady voice whose glottal pulses are all alike (the subtlest of the eight half-ranges: the voice has little irregularity to lose); formant voices only. Brightness tilts the whole output of every voice (`tilt_brightness()` in `src/common/audio/brightness.rs`: the top above a pivot folded away at 0, the pivot moving from 1.5 kHz at 50 to `BRIGHTNESS_DULL_HZ` 800 Hz at 0; the top above 1.5 kHz tripled at 100, `BRIGHTNESS_SHARP_GAIN`; until November 2026 folded at 1.5 kHz and doubled), so 0 also softens the hissing consonants and the bursts (8 dB on Zvonko's s), and moves the source emphasis of the formant voices between 0 and 0.85 around the voice's own (`brightness_emphasis()`); until November 2026 the formant voices had the emphasis alone, which never reached the frication. Tests: `tests/formant.rs` `voice_quality_settings_shape_the_voice_and_50_leaves_it_alone`, `tests/recorded.rs` `head_size_and_brightness_shape_the_recorded_voices`; see "Voice quality settings" in `docs/formant.md`.
- **The reference balance (November 2026).** The speaking voices' default spectrum is matched to the English voice of a classic commercial formant synthesizer at its default settings (measured on the development machine; the tool that drives that engine is kept outside the repository, in `build/measure/reference-tool/`): a low shelf of 7 dB from 230 Hz down on the voicing (`VoiceQuality::bass_db` / `bass_hz`, `Biquad` in `src/common/formant/klatt.rs`), a 6th-order ceiling at 5300 Hz on the voiced path (`VoiceQuality::ceiling_hz`, `SPEECH_CEILING_Q`; on the whole output until 2026-11-10) in place of the 6 kHz voice low-pass, which only the singing presets keep (gated to 0 for them in `FormantVoice::quality()`), the source emphasis 0.55, F5's bandwidth 650 and F4's 220 Hz for speech (`SPEAKING_F5_BANDWIDTH`, `SPEAKING_F4_BANDWIDTH`), and the consonants at synthesizer A's levels re the vowel (s -14.5 dB, š -17, c -14.7, č -16.7, ć -15.1, f -26, h -20, t burst -23, p -22, k -17 before a and -19/-20 before i and u, scaled by the vowel with `VELAR_CENTRAL_BURST` / `VELAR_FRONT_BURST`; voice bars `VOICE_BAR_AV` 0.70). The presets' vowels are byte-identical to before. Tests: the reference balance cases of `tests/formant.rs` (`the_speaking_voices_have_the_reference_balance`, `friction_sits_about_ten_decibels_under_the_voice`, `f_h_t_p_k_and_the_sibilants_sit_at_the_reference_levels`, `the_singing_presets_keep_their_top`); the tables and every number in "The reference balance" in `docs/formant.md`.
- **The hiss where the ear is sharpest (2026-11-10).** Users still heard harsh frequencies whatever they set, and c, z, s as a slight č, ž, š: matched to an 11 kHz synthesizer's centroids under the 5.3 kHz ceiling, four fifths of the s lay in 3.8-5.0 kHz (middle 4.5 kHz; every recorded s 5.2 kHz or higher), the band the ear is most sensitive in, so lowering its level never cured it. The sibilants keep synthesizer A's *levels* and have the recorded *place*: the friction of every voice has its own fourth-order low-pass at `FRICATION_CUTOFF` 8 kHz (`src/common/formant/klatt.rs`; the ceiling is the voiced path's alone), s/z/c/dz peak at 5.5 kHz between 4.6 and 6.9 (1 % under 4 kHz, 42 % above 6; the recorded Josip 2 % and 40 %), š/ž/č/dž/ć/đ keep their main peaks and get the recorded tail above 4.5 kHz, č and ć are 1.5-2 dB down; the noise peaks of a phone must be in rising order (the bank alternates signs). Measured with the ear weighting of `tools/formant/sibilants.py` ("ear" column: Terhardt's threshold in quiet and a 500 Hz high-pass; friction is about 6 dB louder to the ear at 4 kHz than at 6.3) and the last line of `spectrum.py`: the s is 3 dB softer to the ear at the same level. **The recorded voices** hold their hissing consonants `RECORDED_HISS_DB` (-10 dB until 2026-11-17, now -4 dB and on the spans of the sibilants and f alone, see "Clarity" under "Recorded Voices"; `src/common/params.rs`; 0 = off) under the nearby vowel in the plain output too, through `limit_hiss()` (`src/common/audio/boost.rs`, called by `ConcatSynthesizer::render_utterance()`): in the recordings s, c, č, ć and t's release stand level with the vowels, where commercial recorded voices have them 4-8 dB under; boost mode keeps its -22. Vlado's and Đedo's s cannot be mended: the recordings hold nothing above 5 kHz (s middle 2.9 kHz, š 2.8). The vowels of every voice were measured inside the range of four commercial voices and left alone. The comparison set: the recorded Josip, synthesizer B and the commercial recorded voices D, E, G and H through SAPI. `LAPRDUS_TEST_INFO=1` (with `-- --nocapture`) makes the tests print the measured values behind their thresholds. Tests: `tests/formant.rs` `s_and_c_are_not_heard_as_sh_and_ch`, `tests/recorded.rs` `the_hiss_of_the_recorded_voices_is_held_under_the_vowels`; see "The hiss where the ear is sharpest" in `docs/formant.md`.
- **settings.json of version 1.0** (no `inflection_level` key) is migrated when read (`UserSettings::parse()` in `src/common/config.rs`, mirrored in the NVDA driver): its `inflection` switch is ignored (it controlled 1.0's pitch jump at the punctuation, not the melody, so the level starts at the default 0.5), and an unforced `speed` / `pitch`, which 1.0 ignored, reads as 1.0 so the upgrade does not change the host's rate. Pause values mean the same in both versions; a colon pauses for the comma pause.
- **A host's rate and pitch scale the user's own.** The rate and pitch in Laprdus's settings are the voice's normal; TalkBack, VoiceOver, NVDA, SAPI5 and Speech Dispatcher ask relative to their normal (Android `speechRate` 100, SSML 1.0, SAPI rate 0, NVDA 50, SSIP 0), so every platform sets `laprdus_set_speed(setting × host)` and `laprdus_set_user_pitch(setting × host)`, clamped to the voice's range (`VoiceInfo.requestValue()` on Android, `VoiceCatalog.requestValue()` on Apple). The force switches (`force.speed` / `force.pitch`, `force_speed` / `force_pitch`) mean "ignore the host": the setting alone. Volume is not scaled (host volume, or 1.0 on Android, unless forced; on Apple the host's volume is VoiceOver's `<prosody volume="...dB">` of the request, read by `SSMLParser` per part, which used to be ignored). **Volume boost** (`VoiceParams::volume_boost`, `laprdus_set_volume_boost()` / `laprdus_get_volume_boost()`, `speech.volume_boost` in `settings.json`, key `volume_boost` on Android and Apple, CLI `-B`, a checkbox under the volume in every settings screen, off by default, every voice): louder speech after the amplified speech volume of phone systems: both renderers drive the clause through `src/common/audio/boost.rs` before the volume is applied, a gain of `VOLUME_BOOST_GAIN` (5.0, +14 dB) and a lookahead peak limiter (5 ms lookahead, 20 ms hold, 50 ms release, ceiling `VOLUME_BOOST_CEILING` 0.944 on the true peak) whose gain moves only between syllables, never inside a glottal cycle, so nothing is waveshaped and a host's volume control still works; the tanh soft clipper above `VOLUME_BOOST_KNEE` 0.9 is an idle safety net, and the renderers apply no knee of their own after it. The plain output of both renderers goes through the same limiter at unity gain after the volume, its ceiling at the renderer's knee (0.9 recorded, 0.8 formant; `ConcatSynthesizer::render_utterance()`, `FormantSynthesizer::render_utterance()`): at full volume Vlado and Đedo had driven the knee into heavy distortion (above 5 kHz half of Đedo's sound; a vowel check of November 2026), breathiness 0 and brightness 100 drove the formant voices' knee to a waveshaping residual of -16 dB, and seven singing presets bent their loudest notes at the defaults; an utterance that stays under the knee is unchanged (the speaking formant voices peak near -4 dBFS at the defaults). In boost mode the gain may rise at most `VOLUME_BOOST_MAX_LIFT_DB` (2 dB) above the gain of the loudest syllable of the last 0.6 s, so the consonants keep their distance from the vowels (the limiter alone lifted the friction frames 3 dB and the bursts 6 dB closer; 6 dB still let the bursts 4 dB closer), and a second-order high shelf takes the treble down by `VOLUME_BOOST_TREBLE_DB` (3 dB) from `VOLUME_BOOST_TREBLE_HZ` (4 kHz) up (a listener's report of November 2026: the boost bit on a phone at full volume, on Josip too). Then the hiss limiter (`limit_hiss()`): while the band above `VOLUME_BOOST_HISS_HZ` (3 kHz) outweighs the rest, the part above `VOLUME_BOOST_HISS_SPLIT_HZ` (2 kHz, a fourth-order split) is held `VOLUME_BOOST_HISS_DB` (-22 dB) under the loudest nearby vowel (20 ms average, ±250 ms), cut by at most 24 dB, so the hissing consonants of every voice sit 16-21 dB under the vowels through a phone speaker's 500 Hz high-pass (Zvonko's s and c were 13 dB under, Josip's s, č and ć level with the vowels; a listener heard them pierce at full volume). The ceiling `VOLUME_BOOST_CEILING` 0.944 is on the true peak (`inter_sample_peak()`, an 8-tap Lanczos interpolator), so nothing goes over full scale when the phone resamples (Josip had reached +1.4 dBTP at 0.97 sample by sample). Zvonko's median level rises by 6 dB and his loud stretches by 5 dB, Josip's by 1 dB (compressed recordings at full scale already); until November 2026 the ceiling was 1.4 and the clipper started at 0.6, 3 dB louder at 10 % distortion, which phone users heard. See "Volume boost" in `docs/dev.md`. Speech Dispatcher has no force switches and reads the settings back from `Engine::params()` after `Engine::load_user_config()`. Before this, an unforced Laprdus rate was ignored by every host, so the rate slider seemed dead next to the acceleration slider.
- **Android: the app and the TTS service have separate native engines** in one process: `LaprdusTTS.service` and `LaprdusTTS.app` (Hilt provides the latter). The JNI library (`src/platform/android/native/src/lib.rs`) keeps one engine per handle (`nativeCreate` returns it: a number that names an engine in a table, never a pointer) and every native call takes that handle; there is no global engine. The service follows the app through saved state only: a newly saved default voice is loaded at once, and the user dictionaries switch or any dictionary write (`DictionaryRepository.changes`) reloads the dictionaries (`LaprdusTTS.reloadDictionaries()`: bundled, then the user's from `userDictionaryDir`). Sequences such as "set the request's rate and pitch, then synthesize" and voice loads run inside `LaprdusTTS.exclusive { }`.
- **Number words follow the language**, for every voice: `Engine::set_language()` makes a `NumberReader` (`src/common/text/numbers/`) for the language, whose number words and grammar are the language's own `Grammar` (`src/common/languages/<code>/numerals.rs`: tisuća/milijun/dvjesto for hr, hiljada/milion/dvesta for sr, hiljada/milion/dvjesto for bs, the Slovenian system for sl), starting from the voice's native language in `Engine::set_voice()`; Josip speaking Serbian says *hiljada*, Stojan speaking Croatian *tisuća*. One and two agree with the feminine scale words in every dialect (dvije tisuće, dve hiljade, dvadeset jedna tisuća, dvije milijarde). Slovenian (Slovenski pravopis 2001 § 11, § 536, § 253): nič, ena, dve, tri, štiri, and 1 to 4 agree with what they count, by the next word's ending and the preposition before the number (`agree()` in `src/common/languages/sl/numerals.rs`, a listener's report of November 2026: en koš, ena knjiga, eno leto, ena stvar; dva koša, dve uri, sto dve leti; trije koši, tri hiše, tri stvari; z dvema žličkama, s tremi žlicami, v dveh urah, do enega leta, v eni uri, pred enim letom; en kg, dve min), the plain form where they count nothing or the next word could be something else: ena, tri, štiri, and dve, the name of the number (a listener's second report asked for it where the 2 stands alone; SP § 11, SSKJ² ena in ena je dve and Jezikovna svetovalnica "Uporaba dva in dve" give the rest: 2, 1, 2, 3, 0 1 2 3, 2 plus 2, 2 je sodo, pritisni 2, stran 2, slika 2, poglavje 2, the typed digit, digits read one by one); dva only in a longer number (sto dva, dva tisoč dva) or a name or after a masculine noun (T2, T-2, PS/2, A-2 liga, 2kg, 2-krat, Pentium 2, člen 2, monitor 2, 2 %); dve also on the clock and in a decimal (Ura je 2, 2:30, 12:02, dve celi pet); enaindvajset as one word; the invariable tisoč (dva tisoč, tristo en tisoč); en milijon, dva milijona, trije milijoni, štirje milijoni, pet milijonov, enaindvajset milijonov; ena milijarda, dve milijardi, tri milijarde, pet milijard; the decimal comma as cela (tri cele štirinajst, ena cela pet, nič celih pet), a period between groups of three digits as the thousands separator (132.457 sto dvaintrideset tisoč štiristo sedeminpetdeset; 3.14 stays tri pika štirinajst), a numeric date in the genitive (7. 10. 2026 sedmega desetega dva tisoč šestindvajset, 7. oktobra sedmega oktobra), but in the nominative where the sentence names the day (a listener's report of 2026-11-17: Danes je 16.4.2026 danes je šestnajsti četrti dva tisoč šestindvajset, with or without spaces, Danes je 16. 4.; `names_the_day()` in `sl/numerals.rs`: after danes, jutri, včeraj, pojutrišnjem, predvčerajšnjim, datum, to, dan and je or bo, and je bil; Danes smo 16.4.2026 and Rodil se je 16. 4. stay the genitive). Since 2026-11-03 (the maintainer's request that the numerals match the spelling and grammar; `number_at()` of `src/common/languages/sl/numerals.rs`, the language's hook (`Grammar::number_at()`) that the shared loop of `NumberReader` calls first, plus `agree()` and `ordinal_before()` there, the Slovenian side of the agreement and of the silent-period block; all in the Slovenian folder, hr/sr/bs audio unchanged): a number with a period before a lowercase word is the ordinal in the case the noun's ending and the preposition show (`1. mesto` prvo mesto, `na 1. mestu` na prvem mestu, `v 21. stoletju` v enaindvajsetem stoletju, `3. člen` tretji člen, `1. stran` prva stran, `7. oktober` sedmi oktober, `v 90. letih` v devetdesetih letih, `odstavka 3. člena` tretjega, `7. 10.` sedmega desetega, `za 3. nagrado` za tretjo nagrado but `3. mesto` tretje mesto by the generated table of neuter nouns in -o `NEUTER_O` (`sl/tables/neuter_o.txt`, read by `sl/nouns.rs`), `videl sem 3. otroka` tretjega; the cardinal where nothing is counted, `2. je`, `1.` alone); `videl sem 1 otroka` videl sem enega otroka and `1 kolega` en kolega by the generated `NOUN_A` (`sl/tables/noun_a.txt`; masculine genitives, animate accusatives and nominatives in -a; `ena miza` stays); 5 and above decline where a preposition or an unambiguous ending asks (`v 5 urah` v petih urah, `s 5 prsti` s petimi prsti, `pred 21 leti` pred enaindvajsetimi leti, `do 5 let` do petih let; sto and tisoč stay); the unit signs `%`, `€`, `$`, `°`, `°C` are nouns agreeing with the number (`1 %` en odstotek, `2 €` dva evra, `3 %` trije odstotki, `za 3 €` za tri evre, `5 °C` pet stopinj Celzija, `1,5 %` ena cela pet odstotka) and in the case of the preposition before them (`do 5 %` do petih odstotkov, `do 1 %` do enega odstotka, `pri 20 °C` pri dvajsetih stopinjah Celzija, `z 2 €` z dvema evroma, `k 5 €` k petim evrom, `za 1 °C` za eno stopinjo Celzija; `0 °C` nič stopinj in every case; `unit_case()`, `Case`); unit abbreviations by gender (`1 min` ena min, `1 MB` en MB, `2 tisoč` dva tisoč); a glued minus is minus (`-5 °C`); thousands with spaces are one number (`1 000 000` milijon; not after a digit, a bracket, telefon or a name); a range reads do (`1991–2000`, `2–3 dni`, `15.–20. 3.` petnajstega do dvajsetega tretjega, `3.–4. mesto`, `v 7. in 8. razredu`; a hyphen only between small numbers or two years), also written with words, both numbers in the preposition's case (`od 5 do 10 %` od petih do desetih odstotkov, `med 5 in 10 °C` med petimi in desetimi stopinjami Celzija, `od 2 do 3 ur` od dveh do treh ur, `starosti 2 do 3 let` dveh do treh; the first number plain when the second is, `2 do 3 ure` dve do tri ure, `za 2 do 5 kilogramov` dva do pet; `range_left()`); `2-krat`, `2x`, `2-letni`, `10-letnica` are the one word dvakrat, dvoletni, desetletnica; the hour after ob is the locative (`ob 5`, `ob 5h`, `ob 17:00` ob petih, ob sedemnajstih), `ob 8.30` and `od 8.30 do 16.00` are times, `24:00` too. Sources: SP 2001 pét pêtih, stó stôtih tudi neskl., tísoč -- tudi -ih, pŕvi, trétji -a -e, sêdmi, stôti, tisóči, dvákrat, dvóléten, odstótek, évro, stopínja, Célzij, mínus; Pravopis 8.0 § 26. Tests: `tests/formant.rs` table `NUMERALS` (in `slovenian_is_spoken_by_every_voice`) and `src/common/text/numbers/tests.rs` `slovenian_ordinal_noun_agreement`; details and limits in `docs/formant.md` 4a "The numerals' grammar". **Croatian, Serbian and Bosnian (2026-11-03, the same request for the other three languages; `number_at()`, `agree()`, `ordinal()`, `ordinal_before()`, `count()` of `src/common/languages/hr/numerals.rs`, with a full copy in `sr/` and in `bs/`; Serbian Cyrillic transliterated for the word checks by `latin()` of `src/common/text/numbers/scan.rs`):** *jedan, jedna, jedno* and *dva, dvije* (sr *dve*) agree with the noun after them (`1 kuća` jedna kuća, `2 kuće` dvije kuće, `1 selo` jedno selo, `1 dijete` jedno dijete, `1 stvar` jedna stvar, `2 noći` dvije noći, `21 kuća` dvadeset jedna kuća, `1 auto` jedan auto), 1 declines after a preposition (`s 1 prijateljem` s jednim prijateljem, `u 1 satu` u jednom satu, `u 1 godini` u jednoj godini, `do 1 godine` do jedne godine, `od 1 sata` od jednog sata, `za 1 kunu` za jednu kunu), 3 and 4 and the oblique forms of 2 are left alone; nothing counted keeps *jedan, dva* (`1 i 2`, `stranica 2`, `T2`, `Pentium 2`, `u 1 ujutro`). Ordinals with a period before a noun take the case the noun's ending and the preposition show (`1. mjesto` prvo mjesto, `na 1. mjestu` na prvom mjestu, `u 2. razredu` u drugom razredu, `u 21. stoljeću` u dvadeset prvom stoljeću, `3. članak` treći članak, `1. stranica` prva stranica, `7. listopad` sedmi listopad, `7. listopada` sedmog listopada, `1990. godine` tisuću devetsto devedesete godine, `u 1990. godini` devedesetoj, `u 90. godinama` devedesetim, `konac 80. godina` osamdesetih, `do 3. razreda` trećeg, `početak 2. svjetskog rata` drugog, `osvojio je 1. nagradu` prvu, `To je 1. liga` prva; the cardinal where nothing is counted, `1. je`, `1.` alone, `2026.` before a capital); a full date is read in the genitive with the year as the feminine ordinal of *godine* (`7. 10. 2026.` sedmog desetog dvije tisuće dvadeset šeste, sr dve hiljade), in the nominative after *danas je / datum je* (`Danas je 7. 10.` sedmi deseti), and `7. 10.` without a year sedmog desetog; the unit signs are nouns (`1 €` jedan euro, `2 €` dva eura, `5 €` pet eura, `21 €` dvadeset jedan euro, `1,5 €` jedan zarez pet eura, `1 $` jedan dolar, `1 %` jedan posto, `1 °C` jedan stupanj Celzija, `2 °C` dva stupnja, `5 °C` pet stupnjeva Celzija; sr `1 €` jedan evro, sr/bs `5 °C` pet stepeni Celzijusa); unit abbreviations by gender (`1 min` jedna min, `2 min` dvije min, `1 kg` jedan kg); a glued minus is *minus* (`-5 °C` minus pet stupnjeva Celzija); thousands with a period or a space are one number (`1.000` tisuću, `1 000 000` milijun, `1.000,50` tisuću zarez pedeset, `1.000 €` tisuću eura; not after a digit, a bracket, *telefon*, *broj* or a name); a range reads *do* (`1991.–2000.` and `od 1991. do 2000.` tisuću devetsto devedeset prve do dvije tisućite, `2-3 dana` dva do tri dana, `10-15 %` deset do petnaest posto, `15.–20. ožujka` petnaestog do dvadesetog ožujka, `3.–4. mjesto` treće do četvrto mjesto, `u 7. i 8. razredu` u sedmom i osmom razredu; a hyphen only between small numbers or two years, `716-844` stays); `2x` dva puta, `1x` jedanput, `2-godišnji` dvogodišnji, `3-godišnja` trogodišnja, `10-godišnjica` desetogodišnjica, `4-sobni` četverosobni (sr/bs četvorosobni), `57-godišnji` pedesetsedmogodišnji; `u 8.30`, `od 8.30 do 16.00`, `Sada je 8.30` are times, `24:00` too; `u 5` stays *u pet*. Tests: `tests/formant.rs` `croatian_serbian_and_bosnian_numerals_agree_with_their_nouns`, `src/common/text/numbers/tests.rs` `croatian_serbian_bosnian_ordinal_cases`.
- **Slovenian** (`src/common/languages/sl/`, `process()` in its `frontend.rs`; "4a. Slovenian" in `docs/formant.md`; the items named below are in that folder: `phonemize.rs`, `lexicon.rs`, `readings.rs`, `clause.rs`, `marks.rs`, `words.rs`, `nouns.rs`, `numerals.rs`, and the generated tables are the files of `tables/`, embedded as statics of the same names): the shared tokenizer and clause structure with rules and a lexicon of its own. Eight vowels: the phones `EH` [ɛ], `OH` [ɔ] (every unstressed and short stressed e, o), `EE` [eː], `OO` [oː] (long stressed close é, ó), `AX` (the schwa written e: veter, pes, misel, the fleeting -er/-el/-en/-ek/-ec), and `W` for the l and v that are [w] (bil, delal, stol, nov, avto; word-initial v before an obstruent is the vowel u: vsak [usak]); lj/nj are l+j, n+j before a vowel and plain l, n otherwise; syllabic r is [ər]; final devoicing (grad [grat], but a preposition keeps its voicing before its host: od orača), regressive voicing assimilation, s/z before š/ž/č, n before k/g/h. Stress is dynamic (long or short, no tones: every word has `Accent::Neutral`): marks in the text (acute = long close, circumflex = long open, grave = short), then the lexicon, then rules (the always-stressed suffixes -írati, -ováti, -éti, -íšče, -ílo, -ízem, -álen, -ljív ...; the syllable before -ica, -ščina, -niti, -cija; otherwise the second-last syllable). The lexicon: the generated `tables/forms.txt` and `tables/stems.txt` (96,000 forms and 80,000 stems covering the word forms of 101,000 Sloleks 3.1 lemmas: every hand-accented one and the automatic ones of frequency 100 or more; front-coded in blocks of 16 and binary-searched in place: no start-up cost, 0.8 MB of text embedded as it is) plus `HAND_ENTRIES` in `lexicon.rs`, which win and hold only words Sloleks lacks and dictionary-backed deviations (about 60 entries since 2026-10-30). The generator reads a homograph by the corpus frequency of the form, not of the lemma (veselím, not the adjective's vesélim), never reads a proper name where a common word has the spelling (sobóta, jeléna; the names go to the tables of names below), makes a fleeting e a schwa (rékel rêkla, začétek začétka, not vídel vídela or a verb's -em), applies `CORRECTIONS` (lemma-level fixes from SSKJ²/SP: dánəs, the first-syllable teens) and then simulates the C++ lookup, hand entries included, over every form, listing whole any form a stem misreads (postavít*, the stem of postavítev, had read postaviti as postavíti) or whose transcribed final [w] it would read [l] (`loses_final_w()`: sledíl* of sledíla had read sledil like the loanwords kanál, hotél; a stem and the ending -l alone is [w] in `phonemize()`; since November 2026, 269 of the 6,000 commonest masculine participles were wrong). The reverse is the table `FINAL_L` (`tables/final_l.txt`, generated: Sloleks's [l] minus the lemmas SSKJ²/SP mark [-u̯], `tools/formant/slovenian_final_w.tsv`), read by `finish_word()`: a word that only looks like a participle keeps [l] (ángel, módul, móbitel, stíl, kábel, ansámbel, Izrael, the genitive plurals sil, škatel), as does a capitalized word in no lexicon (Michael); mísel, vesél, tôpel, žival stay [u̯]; a native listener confirmed the class. Measured on the tagged corpus SUK 1.1 with Sloleks as the reference (`docs/formant.md`, 4a): 99.4 % of the stressed syllables of 297,000 content words right, 98.2 % before. Notation (`Lexicon` in `sl/lexicon.rs`): the word spelled as pronounced with the dictionaries' accented letters (mésto, člôvek, otròk), ə for a schwa, w for an l that is [w], ł for one that stays [l], `*` stem, `|` form list, `spelling=pronunciation`; `'` and `:` as ASCII marks. Users write it in `accents.json` with `"language": "sl"` (an entry without a language is used for Slovenian too when it reads in the notation; Slovenian has no verb entries). Regenerate the table with `python3 tools/formant/slovenian_words.py extract <Sloleks.3.1 dir> forms.tsv 0` (13 GB of XML, about 4 minutes) and `... generate forms.tsv --manual-min 0 --auto-min 100 --form-min 1` (it writes the fourteen tables into `src/common/languages/sl/tables/`, or into a folder named after `forms.tsv`, and reads `HAND_ENTRIES` from `src/common/languages/sl/lexicon.rs` and the SP name files `tools/formant/slovenian_places.tsv` / `slovenian_persons.tsv`, so regenerate after changing them; `--names-report FILE` lists the tables of names for review). **Names** ("Names" in 4a of `docs/formant.md`; a listener reported 2026-10-30 that surnames in -vić/-vic are stressed on the first syllable): a surname in -ović/-ević (-ovič, -ovic ..., any case, `vic_surname()`) is stressed on the syllable before the suffix, the first of three syllables (Pétrović, Túrković, Srébrović) and the second of four (Stojánović, Milóšević, as the hand-accented names of Sloleks), for a capitalized word or any word with ć/đ, ignoring the stems of common words; only a whole form of the table overrides it, and the generator drops Sloleks's network-accented (AUTOMATIC) -vić names. A capitalized word (not all caps, `Capital` in `sl/phonemize.rs`) is looked up first in the generated tables of names (`name_reading()` in `sl/readings.rs`: `NAMES` anywhere, `NAMES_INSIDE` not at the head of a clause or after an opening quote): the hand-accented Sloleks names that a common word is spelled like (Múrska Sóbota/sobóta, Róman/román; at the head only a name three times as frequent as the common word and not the second word of a place, so "Sobota je lep dan" is the day; never adjectives more frequent than the name, Stare/Bele, nor `COMMON_CAPITALIZED`: v Boga, Svetega Duha), and the places and persons of Slovenski pravopis 2001 harvested from Fran (1,003 places, 167 persons; declined by `place_forms()`: Bôhinj Bohínja, Trbôvlje Trbôvelj, Litíja, Sôlkan; SP wins over Sloleks except where SP accents the word otherwise in another entry or the spelling is a form of another Sloleks name). Inside a clause a capitalized word takes a stem only with a declension or possessive ending, also after one consonant (`declension_ending()`: Koróšca, Cánkarjev; not Ferd*+olenc). Names the lexicon lacks follow the patterns measured on Sloleks's names (`default_stress()` with `Capital`): -Cčič second-last (Zupánčič), other -ič/-ić of 3+ syllables third-last (Gólobič), -nc/-vc/-jc last (Dolénc), -ovec before it, -mir/-bor/-slav first, -in/-ak/-aj/-ej on that vowel, -ij before it, -ija before it only inside a clause; rules alone 74 → 80 % on those names. Measured: the forms of Sloleks's hand-accented names, capitalized inside a sentence, 94.2 → 97.3 % (99.9 % by frequency); GaMS got 4 of 16 test names. Test: `src/common/languages/sl/tests.rs` `slovenian_names_of_persons_and_places_october_2026`. After a given name (`tools/formant/slovenian_given_names.tsv`, Wikidata, written into the generated table; not after a title, all six cases in SUK were wrong; the list leaves out names in -a that are mostly a man's genitive, Miroslava Krleže) a capitalized word in a consonant and -e, spelled the Slovenian way and not in the tables of names, is a surname stressed on its open final e, whatever common word it is spelled like (`surname_reading()`: Vesna Smode Smodè, Janez Stare Starè, as Sloleks's Peršè, Lekšè, Peterlè; a listener's report of 2026-11-02); hand-accented exceptions such as Lapájne stay, and alone Smode keeps the second-last syllable, because the capitalized -e words the lexicon lacks are mostly foreign (Melbourne, Hesse) or oblique. `CORRECTIONS` also holds the two deviations this listener asked for: seznàm with the open e (SSKJ²/SP allow [sə] and [se]) and posodobiti on the i in every form of the verb (posodobíti, posodobím, posodobí, posodobíl, posodobíla; SSKJ²/SP posodôbiti; the passive posodôbljen stays). Clitics: the monosyllabic prepositions and conjunctions, ne, naj, the forms of biti and the future, bi, the short pronouns; the preposition v is [w] before a vowel and [u] before a consonant; a spelled acronym is stressed on its last letter (SP [espé]). A compound numeral (enaindvajset) is read as three words, ena in dvajset, with an accent on the ones and on the tens and no pause (SP ênaindvájset; `split_compound_numeral()`). A spelling with two readings (`HEAD_READINGS`) is the greeting or the imperative at the head of a clause and after in/pa/zdaj/prosim/takoj/nemudoma (žívijo, narêdi, urêdi, dêli), the present tense or the other word elsewhere (živíjo, naredí, déli); the head is the first word with only those adverbs before it (Nemudoma posodobi to aplikacijo!, but to naj se takoj izvrši is the present), and govori/odgovori/prosi before a comma and a clause of da are the present (Govori, da bo prišel). The verbs of software commands do the same from the generated `HEAD_IMPERATIVES` (`tables/head_imperatives.txt`;`tools/formant/slovenian_commands.tsv`, about 90 lemmas chosen by hand; Sloleks's hand-accented 2sg/2pl imperatives where they differ from the present's spelling, SP's pattern namestíti namésti -íte, gradíti grádi, vršíti vŕši: Posodobi posodôbi, but Posodobite posodobíte at the listener's word, Nadgradi nadgrádi, Namesti namésti, Izvrši izvŕši, Dovolite dovolíte; Aplikacija naj se posodobi posodobí; a listener's report of 2026-11-03). Not the whole class: at the head of a sentence of SUK the retracting class is the present 68 times and the imperative 10 (Sledi, Živi, Vodi). Veseli is the verb veselí with an object clitic anywhere before it or right after it (Me zelo veseli vaš obisk; in veseli me, da; the head-plus-clitic rule of the whole verb/noun table also applies after a conjunction: Toda žalosti nas) and the imperative vesêli at the head before se in a sentence ending in `!` (Veseli se!, Veseli se novega dneva!; `IN_EXCLAMATION_MARK` carries the `!` across commas like `IN_QUESTION_MARK`), the present without the `!`. An address, one capitalized word before or after a comma (Pero, kje si?, Pero, pridi sem., Hvala, Pero.; `VOCATIVE_MARK` from `mark_clauses()` in `sl/marks.rs`, which `Engine::segment()` calls; `is_address()`), is read by the tables of names as inside a clause, as is a given name (`is_given_name()`) at the head followed by a capitalized word, its surname (Pero Novak je prišel, Roman Jančič; not samo: Samo Janez je prišel): Pêro the name (a persons.tsv row with Sloleks's accent; SP has no entry), peró the pen at the head otherwise (Pero je na mizi) and in small letters. Meni (`clause_reading()`, SSKJ²/SP) is the menu mení alone in its clause ("Meni", "Meni, gumb"), after a preposition whose pronoun would be mano or mene (pod meni, za meni), after v or na with a verb of motion or clicking before them or a capital after it (Pojdi v meni, Kliknite na meni, v meni Start), after an adjective, possessive or demonstrative (glavni meni, vaš meni, padajoči meni, dober meni, ta meni), a noun it names (Gumb meni), a verb that takes a menu (Odpri meni, ima meni), with a capital inside a clause (Gumb Meni) and, itself capitalized, before a capital (Meni Datoteka; small meni before a capital is the news verb, meni Kučan), and in what a menu does or is (Meni se odpre, Meni je prazen, Meni ima, Meni aplikacij, Meni za kosilo); the pronoun mêni after another preposition (k meni, Ti si v meni), after a word stressing it, an auxiliary or a participle, which a present cannot follow (Tudi meni, Daj meni, To je meni znano, Ponudili so meni), or before what is said of whom (meni ne, meni se zdi, meni je všeč, Meni za to ni mar); the verb méni elsewhere and last in a clause before da (Meni, da je prav; minister meni). SUK: 194 → 231 of 242. Lahko is the modal lahkó (2,703 of 2,718 in SUK) unless there is evidence for the adjective láhko: lahko noč, a preposition before it (z lahko nogo, na lahko), a verb of obligation before it and no infinitive after (Jesti moraš lahko hrano), or one of about 170 nouns things are called light by (SSKJ², SP, the Kolokacije of CJVT: torbo, obleko, hrano, vino, nalogo ...), alone or after an adjective of its own (lahko rdečo torbo, lahko in hladno obleko), ending the clause or before a preposition, a conjunction or a copula, with before lahko an imperfective verb that carries, eats or wears such a thing in any person (On nosi lahko torbo, Nosim lahko torbo, Pijem lahko vino) or a degree word (Zelo lahko torbo imam), and no present of a perfective verb or other first or second person (Kupim lahko hrano, Kupi lahko hrano, Izberete lahko nalogo are the modal, as is a verb after the noun in the middle of a clause); a sentence opening with lahko and such a noun goes by its end, as a native listener answered: a question is the modal (Lahko delo opravi vsak?, Lahko delo?), a statement the adjective (Lahko delo opravi vsak., Lahko torbo nosi.) unless the verb is in another person (Lahko hrano kupite, Lahko hrano kupijo); the adjective has a content word's accent. Roka is the hand rôka (hand entry; the genitive of rók is commoner in the corpus) except after a preposition, a genitive in -ega or a noun of time limits (do roka, vojaškega roka: róka), and at the head of a clause where no hand can be meant (`roka_is_absent_or_object()`): the genitive after a negated verb of being (a listener's Roka že ni, Roka ni več; Roka ni doma, Roka ni bilo na sestanku, but Roka ni zlomljena) or the object of a verb with another subject (Roka nisem videl, Roka poznam); Róka the name inside a clause (Čakam na Roka). Roki and roke are the hand's unless an adjective of deadlines, an instrumental or a plural verb says deadlines (plačilni roki, z roki, roki so kratki, izpitne roke: róki, róke); rok is close either way (rók the deadline, iz rók of the hand: SP). glasu is two forms of glás: the genitive glasú and the dative and locative glásu (SSKJ² glás -ú tudi -a; SP 2001 glás -ú tudi glás -a, "prva oblika dalje -u -- -u -om"; Sloleks glasú, glásu; a listener's report of 2026-11-10: Nisem imel glasu and Glasu ne morem preveriti on the last syllable, V glasu je hripavost and V tvojem glasu je slišati toplino on the first), read from the clause by `genitive_u_reading()` / `case_before()`: a locative or dative preposition right before it or before its adjectives in -em (v glasu, po glasu, na dobrem glasu, v tvojem glasu, v tihem in mirnem glasu, proti glasu), an adjective in -emu (svojemu glasu) or a word that takes the dative (`governs_dative()`: Prisluhni glasu, Sledi glasu vesti, Glasu ne zaupam; the Kolokacije of CJVT) has the stem, and everything else is the genitive: after a genitive preposition or an adjective in -ega (brez glasu, iz glasu, do njegovega glasu), of another noun (barva glasu, hitrost glasu), of a negation, a quantity or its verb (Zmanjkalo ji je glasu); a noun joined by in/ali/ter has the case of the one before it (v strahu in glasu). The same for the other 35 nouns of `GENITIVE_U`, the masculine nouns of one syllable SP declines so (strah, mir, grad, pas, most, zid, led, rod, nos, prah, trak, vrat, med ...: pri míru and želimo si mirú, v stráhu and brez strahú, na grádu and do gradú, na lédu and kocka ledú, na môstu and z mostú, po rôdu, with the open o of SSKJ²/SP in the last two), which the lexicon had read by the commoner form everywhere; not rob, val, cvet, drog, drob, žar, žleb, sin, which keep the stem, and a capitalized word inside a clause with no cue is left to the lexicon and the tables of names (Povej to Miru). SUK 1.1: 200 → 304 of the 305 forms, all 14 of glasu; a dative without any of these cues reads as the genitive (Gradu grozi propad). **A listener's report of 2026-11-17** (all in `sl/clause.rs`, `sl/readings.rs`, `sl/frontend.rs`, `sl/marks.rs`; measured with the new `tools/formant/slovenian_suk.py`, which runs the SUK sentences of given spellings through `examples/slovenian_stress.rs` and scores the phones by lemma and tag, the old side being the same example built from the commit before). meja is the nominative mêja and the genitive plural mejá (SSKJ² mêja -e, rod. mn. mêj in mejá; `GENITIVE_PLURAL_A`, `genitive_plural_a_reading()`): the genitive plural after a preposition, right before it or before its adjectives in -ih (brez meja, do skrajnih meja, onkraj naših meja), after such an adjective alone, a quantity (veliko meja), and for meja and želja a noun of an action (spreminjanje meja) or a verb that has none (ne pozna meja, nima meja, Tu ni meja; To ni meja names it); the nominative elsewhere (Levo od tebe je meja); the same cues for želja, žena, gora, sestra, solza, zemlja, steza (not voda, also the genitive of the masculine vod); SUK 116 → 140 of 149, meja 19 → 36 of 38. vasi is the genitive and plural vasí and the dative and locative vási (SSKJ² vás -í; SP 2001 -i -- -i -jó; `GENITIVE_I`, `genitive_i_reading()`, `feminine_case_before()`), and so are 36 more frequent feminine nouns of Sloleks (strani, poti, pomoči, stvari, moči, ravni, rasti, oblasti, noči, smeri, luči, lasti, krvi, skrbi, snovi, klopi, jedi, soli, časti, vesti, kleti, plati, vezi, cevi, kosti, sledi, plasti, dlani, strasti, laži, peči, pesti, zvrsti, vrvi, osi, zmesi): the stem after v, na, o, ob, po, pri, k, proti right before the noun or before words in -i that agree with it (v tej vasi, na drugi strani, po očetovi strani, na državni in lokalni ravni; not a noun there: v bližini vasi, v dobi rasti), after a dative adjective alone (sosednji vasi) or a word that takes the dative (bližje pomoči); the ending after an adjective in -e, another preposition or a numeral (iz te vasi, z druge strani, v tri vasi), for na stvari and v, na kosti (accusative plural), and with no cue (prebivalci vasi, vasi so majhne, del poti); a spelling that is also a verb's form (moči, rasti, peči, poti, noči) is the verb where `context_reading()` says so; SUK 1,334 → 1,727 of 1,761 (strani 266 → 351 of 352). The imperatives: izgovoriti, sprostiti and oprostiti are in `slovenian_commands.tsv` (Izgovori to besedo izgovôri, Sprosti prostor na telefonu! sprôsti, Oprosti oprôsti; Glasba me sprosti sprostí; SP 2001 govôri -íte, sprôsti -íte, oprôsti -íte); with prosim before the verb, after it, or alone between commas next to its clause (`REQUEST_MARK`, U+E004, from `mark_clauses()`) any verb at the head of a clause is the imperative (Izgovori prosim pravilno!, Razloži prosim to razlôži, Položi, prosim, telefon polôži; Razloži to stays the present razloží), from the generated `IMPERATIVES` (`tables/imperatives.txt`, the 4,187 hand-accented second person imperatives of Sloleks the lexicon reads otherwise; `imperative_reading()`); an exclamation mark alone is no such cue (Gori!, Drži! are presents). A head reading is not taken before so, niso, bodo or a genitive plural in -ih (Odgovori so bili taki, Izgovori odgovornih: the nouns), and a verb of saying not before se (Govori se o tem, the impersonal present) unless the sentence exclaims. A third person in -i that a noun's plural is spelled like (izgovorí and izgôvori, odgovorí, pogovorí) is the verb after a subject in the singular earlier in the clause (on, ona, kdo, nekdo ...), an adverb of how or how often right before it (pogosto, vedno, pravilno, denimo ...) or naj (On to pogosto izgovori), never with a plural verb of being in the clause (To so samo izgovori) or after an adjective (v prvi vrsti), and a verb of saying last in a clause before its clause of da (Einhard trdi, da); SUK 2,963 → 2,986 of 3,201 tokens of the other verb/noun spellings. Found while checking: vas is the village vás (never read for a word written small before) after a word that agrees with a feminine noun (ta vas, naša vas, Srednja vas, v našo vas), after skozi, after v with a verb of going (Grem v vas) and at the head before what a village does or is (Vas leži ob reki, Vas je majhna), `vas_is_village()`, SUK 20 of 28 and no pronoun touched; the pronoun after a preposition (Zaupam v vas, Pridi k nam, V nas, za vas) is the full vàs, nàs, nàm, vàm and a consonant preposition leans on it (`LanguageWords::is_pronoun_after_preposition()`, Slovenian only: v vas had been read "vé vas", the letter's name); Pogovori se z njim, Dogovori se z njo at the head are the commands unless a plural verb follows the se (Pogovori se bodo nadaljevali); and `GENITIVE_I` has 13 rarer nouns too (zveri, gredi, brvi, masti ...). petega, petemu, petima have the open e of the ordinal pêti (`PREFERRED` in `slovenian_words.py`: Sloleks counts the participle pét of péti "to sing" higher for those spellings). Prosto, named in the report without a sentence, is unchanged (SP 2001 prôsto tudi prostó). Tests: `src/common/languages/sl/tests.rs` `slovenian_listeners_report_of_2026_11_17_meja_vasi_izgovori_sprosti`, the Slovenian date rows of `src/common/text/numbers/tests.rs`, and the `READINGS` and `NUMERALS` rows of `tests/formant.rs`. An adjective before meni, whatever it means, makes it the menu (zaprti meni nedavno, odprti meni, najboljši meni, a listener's report): by its ending (-ni, -ski, -ški, -čki, -nji, -oči, -eči) or the generated table `DEFINITE_ADJECTIVES` (`tables/definite_adjectives.txt`;1,479 masculine definite adjectives of Sloleks that the ending misses, without those another lemma's verb form outweighs, such as pravi; skriti by hand), as does a state before or after it (Zaprt meni, Meni zaprt). No token of the verb or the pronoun in SUK stands after one. Since 2026-11-03 (a listener's report): the pronoun mêni also before a verb that gives or tells to someone (Meni daj žogo, meni prinesi kavo, Meni podari ljubezen), before an auxiliary (Meni bo hladno, Meni so povedali) and as a capitalized Meni at the head of a clause before any word (SUK: the verb there is written small after a comma, meni župan, or stands before a particle and a da clause, Meni tudi, da; Meni pa, da bo: the verb, by the complement mark); the menu after an acronym, in capitals or spelled (NVDA meni, JAWS meni, USB meni; `acronym` of `clause_reading()`, from `all_caps` or `letter_name`), after the kind of a menu (kontekst/context/start/Start/Windows meni) and before a menu's name (meni start, Meni orodja; also through the names a screen reader speaks for the quotation marks or brackets around it when its punctuation is on, meni narekovaj Start narekovaj, meni okrogli oklepaj start okrogli zaklepaj, narekovaj start narekovaj meni: `after_symbol_names()`; the marks themselves are silent in the engine, so meni "start" is the menu already) or a role word (Meni gumb), and a capitalized Meni before a number alone (Meni 1, Meni 21: the listener's answer; `is_number_word()`); SDS meni, da je prav keeps the verb. je is the auxiliary of biti, unstressed [jɛ] and at the head of a clause the short open jè (SP 2001 poudarjeno jè, Sloleks; the hand entry jé that read every head je long and close is gone since 2026-11-03), and the verb of eating jé (SSKJ² jésti jém, jé, long close e; a listener's report: On je krompir, Ona je palačinke, Marko zelo veliko je) only where the clause says so, `je_reading()`: after ne (Otrok ne je; the negated auxiliary is ni), before a food object (On je krompir, Mama je juho, On je veliko sadja; the table `FOOD`, `tables/food.txt`, every Sloleks form of the lemmas of `tools/formant/slovenian_food.tsv` with its cases; an object that is also a nominative, krompir, meso, kosilo, needs a subject, a pronoun or any word that is not a function word or an adverb of time (Hrvoje je krompir, Priloga je krompir; Zdaj je kosilo and To je krompir stay "is"); an accusative that is only that, juho, hrano, needs none), before an adverb of eating ending the clause after a subject (Marko zelo veliko je, On rad je; Lepo je stays), and before a utensil (ki je z vilicami); never at the head of a clause (Je krompir., Je veliko solate.), after a clitic or a question word (se je, Strah ga je, Kaj je), with a food nominative as the subject (Kosilo je krompir) or with an l-participle in the clause (Videla je ribe). SUK 1.1: 29 of 107 "eats" read jé, 2 of 24,471 auxiliaries (poljščina je krompir). vprašaj is the command vprášaj (SP vprášaj -te) unless alone in its clause (the character's name; Vprašaj! is the command), after a preposition, a quantifier, an adjective or a verb that writes one (pod vprašaj, velik vprašaj, napiši vprašaj), or last in a clause of a question with no object clitic before it (NVDA's Kako si vprašaj?): `vprasaj_reading()`; vprášala, vprášali, vprášale, vprášalo via `CORRECTIONS` (SP vprášal -ála tudi vprášala; Sloleks had vprašála first for this verb alone). podariti is in the commands list (SP podári -íte): Podari mi to! podári, naj podari podarí; a capitalized dative pronoun opening the sentence (Meni, Nam, Vam ..., not the accusatives, not ti) is a head word before a command (`HEAD_PRONOUNS` in `sl/frontend.rs`: Meni podari ljubezen, Nam pokaži pot). A spelling in -l that is a noun's genitive plural [l] and a verb's l-participle [w] (obvestíl and obvéstil, vozíl and vôzil, also predál and predáw: 153 in `VERB_NOUN`, `tables/verb_noun.txt`, with the noun's share in percent of Gigafida) is read by `participle_or_genitive()`: a participle needs an auxiliary, so no auxiliary before it, a preposition, a number or quantity, -ih, nimam, seznam, another participle holding the auxiliary (Nisem dobil obvestil), and ni alone or after an adverb that says nothing is there (a listener's Ni obvestil; Trenutno ni obvestil) make it the noun; an auxiliary and clitics right before it (On me ni obvestil, Ni me obvestil), an object after ni (Ni obvestil policije), the head of a clause before clitics and an auxiliary (Obvestil me je) make it the verb, each only where the noun is common enough (opravil 12 %: Ni opravil stays the verb; pojasnil 3 %); SUK: 2,208 → 2,245 of 2,309. A clause whose comma comes before da, naj, kdaj, kako ... gets `BEFORE_COMPLEMENT_MARK` (`sl/marks.rs`) from `mark_clauses()`, which `Engine::segment()` calls, a private use character consumed by the Slovenian `process()`, since the front end sees one clause at a time: Meni, da je prav (méni, alone it would be the menu), Ni obvestil, da pride (the verb); `IN_QUESTION_MARK` likewise for a clause before a comma or semicolon in a sentence ending in a question mark. Afna is the noun áfna (also the name of @, hand entry), the verb's afná only next to se. Gori is the verb gorí by default, gôri after a preposition or an adjective (na gori, v Kranjski Gori), after tam, before a form of biti (Gori je lepo) and at the head of a clause before a preposition of place when the clause goes on past the place and the sentence is no exclamation (Gori na gori gori. Gori na seniku je lepo.; Gori na podstrešju! and Gori v hiši. are the fire; SSKJ² gôri prisl., a listener's report of 2026-11-03; `context_reading()`, which now takes the exclamation flag). The participle of biti is bilà biló bilí bilè (SSKJ² standard; the stylistic bíla was a hand entry until 2026-10-30), and bila/bilo/bili/bile are not function words, so their last syllable gets a verb's accent. Lepo and the other spellings of an adverb on -ó and an adjective on its stem (težko, močno, slabo, hudo, tesno, ostro, trdo, toplo, grdo, grenko, tenko, their pre- forms; `END_READINGS`, `adverb_before()` in `sl/clause.rs`) are the adverb at the end of their clause and before a pronoun or a verb (lepó, težkó: Hvala lepo, Lepo mi je s tabo, Lepo se imejte, Lepo bodi, Lepo vas pozdravljam, Lepo pišeš, Težko je reči, močno vpliva; a listener's reports of 2026-10-30 and 2026-11-18, the second with SP 2001 lepó nač. prisl., povdk.) and the adjective before any other word and after a preposition (lépo, têžko: Lepo majico nosiš, lepo obalo imajo, Lepo vreme, z lepo mislijo; also before an adjective or participle, Lepo pozdravljeni, where the first report departs from the dictionaries). A verb is a short pronoun, se, a form of biti, ne, an ending -ti/-či/-l/-la/-li/-le, or a form of the generated `FINITE_VERBS` (`tables/finite_verbs.txt`, `is_finite_verb()`: 16,729 presents, imperatives and neuter participles of Sloleks, without the spellings an agreeing noun has, zimo, mamo, prodajo); SUK: 319 tokens moved, 2 wrong; not lahko, read by its own rule above. A weak Slovenian word stressed after its first syllable (potém, sevéda, zeló) gets a 2.4-semitone accent, above the raised sentence start when first (`src/common/prosody/intonation.rs`, only when `Utterance::slovenian` is set, so the other languages are untouched). A spelling that is a verb's form in one reading and a noun's, adjective's or adverb's in another (941 in the generated `VERB_NOUN`: veselí/veséli, žalostí/žálosti, letí/léti, vôzil/vozíl) is read by `context_reading()`: at the head of a clause before an object clitic → the verb (Veseli me, da sva se spoznala; Žalosti me, da nisi prišel; SSKJ² veseli nas, da delo napreduje; 20 of 20 in SUK), an object clitic right before a present, not ti/te and not after an auxiliary → the verb (To me žalosti; so nas levi stays), a preposition or dve/obe right before it → the noun, ne → the verb, an agreeing auxiliary right before a participle (je vozil, so sporočili; not -lo) → the verb, an -ih adjective before a participle's spelling → the noun; veseli is the verb with se/si in the clause, an object clitic or ne before it or at the clause end, the adjective after a form of biti; leti is the noun only with a preposition, a number or an -imi/-ema word in the three words before it or a form of biti after it. Cues were kept only where they beat the more frequent reading on SUK (93.3 % → 94.8 % of these tokens); clause-final position and an earlier se did worse than nothing in general. A stem of the lexicon followed by -lec/-lka/-lnik/-lnica/-lišče moves its stress before the l (zagánjati → zaganjálnik, brálec, učílnica, gledalíšče). `HAND_ENTRIES` must agree with the Sloleks table unless SSKJ/SP say otherwise (281 of its forms contradicted Sloleks until 2026-10-29, 183 more on 2026-10-30, among them béseda, víšina, séstra, septémber, and the dictionaries sided with Sloleks nearly every time); a hand stem also catches derived words (gôr* read goreti as gôreti), so prefer whole forms, and a deviation for every form of a word goes into `CORRECTIONS`; emoji are named from `emoji_sl.json` (CLDR sl), loaded by the engine next to `emoji.json` and consulted before it while Slovenian is spoken. Letter names (Pravopis § 7): a, be, ce, če, de, e, ef, ge, ha, i, je, ka, el, em, en, o, pe, er, es, eš, te, u, ve, ze, že; ku, dvojni ve, iks, ipsilon; mehki če/dže for ć/đ. Symbols inside a clause: pika, dvopičje, klicaj, odstotkov, evrov, afna; the spelled names of digits and punctuation (ena, pika, vejica, podpičje, vprašaj, oklepaj ...) come from `spelled_name()` (`sl/words.rs`), used by `Engine::spell_character()` before the Croatian `spelling.json`. Tests: `src/common/languages/sl/tests.rs`, `tests/formant.rs` `slovenian_is_spoken_by_every_voice`.
- **z and ž are not heard as v (2026-11-17).** A listener's report; s and c were good and are the samples they were. The friction and voicing of the voiced fricatives are set in `ClauseBuilder` (`src/common/formant/builder.rs`, the `PhClass::Fricative` branch), not in the phone table: `Z_FRICTION` 0.85 and `ZH_FRICTION` 1.0 of the friction of s and š (0.40 and 0.50 since the first version, never measured, while every later round lowered the friction they are a share of), `VOICED_SIBILANT_AV` 0.30 and `VOICED_SIBILANT_TILT` 10 (0.42 and 8). Above 2 kHz z and ž had hissed 0.4-0.7 dB over v under the same murmur (-25.5, -25.8 and -26.2 dB re the loudest frame); now -19.7 and -21.0, 3 and 2 dB under s and š. ʑ and ɣ keep the old values. Every clause without a z or ž is sample for sample what it was (`tests/dsp_against_cpp.rs` skips the clauses with one). Test: `tests/formant.rs` `z_and_zh_are_not_heard_as_v` (`vcv_hiss_db()` of `tests/support`); see "z and ž are not heard as v" in `docs/formant.md`.
- **Synthesis is deterministic**: the same text and settings give the same samples (`tests/formant.rs` relies on it).
- **Acoustic values come from measurements**, not from taste: vowel formants and durations from Bakran's work on standard Croatian, consonant spectra, levels and transitions from analysis of recorded Croatian speech. `docs/formant.md` records the sources, the measured values and how to repeat the measurements. Change numbers in `src/common/formant/phones.rs` only with a measurement or a listening test behind them.
- **Stress** is lexical and cannot be fully predicted. To fix a word, add it to the lexicon, `src/common/languages/hr/lexicon/entries.rs` and its copies in `sr/` and `bs/` (the notation is described in `src/common/text/frontend/notation.rs`), and check its whole paradigm, not just the dictionary form: the accent often moves in the other cases and persons (sìgnal/signála, podátak/pòdātākā, obavijéstiti/obàvijēstīm), and a stem entry also catches derived words with a different accent (see "Stress" in `docs/formant.md`). Verbs are not listed form by form: `IJE_VERBS` has the roots with a long *ije* (podijeliti, promijeniti), `VERBS` whole stems with accent and conjugation class (`ur'e:d=i`), and `StressRules::verb_form()` builds the forms; an imperative that is also a noun's case (potvrdi, uredi) is the verb only at the head of a clause; users can do the same from a pronunciation dictionary by writing accent marks in the replacement (`telèfon`). Place names and personal names are tables of their own (`tables/places.txt` and `persons.txt` of `hr`, `sr` and `bs`; edit `tools/formant/places.tsv` or `persons.tsv` and run `tools/formant/proper_names.py`, which writes all six), used only for words written with a capital letter (not in all-capitals text, and after the user's own entries) so that a village or a name never changes the common word it is spelled like; a personal name wins over a place; an entry marked `=` (spelled like a frequent word) is also skipped for the first word of a clause; `proper_names.py` refuses two accents for one spelling. Number compounds of age and time (petogòdišnjī, šestòmjesečnī, malòljetan) are a rule, `StressRules::time_compound()`, and so are Slavic names in -mir/-dar/-zar (Rȁdomīr, Bȍžidār), `StressRules::slavic_compound_name()`.
- **English words** keep the voices' letter sounds but get English stress with a plain (neutral) accent, never the rising one, wherever it falls (Croàtian, Ábleton, Elèven): the table `tables/english.txt` (the same file in `languages/hr`, `sr` and `bs`; looked up after the native exact lexicon, before its stems), and for words spelled the English way (`english_spelling()`: q/w/x/y, th, ch, ph, ck, -tion, i before a/e/u, doubled consonants, with native prefix boundaries excluded) the rules in `english_rule()`. A capital after two small letters splits a word into its parts (ElevenLabs → Eleven Labs, ČetDžipiti → Čet Džipiti) unless a lexicon knows the whole word (TikTok) or every part is one syllable (TalkBack); never after one small letter (iPhone, BiH, and the Lj/Nj/Dž of transliterated Cyrillic capitals). `english_word()` strips stress-neutral endings, the -s/-ed/-er/-ing/-ment ones only from English-spelled words because native words end that way too (operater, distributer), and Croatian case endings from any word; native forms that would match (reforma ← reform) are unmarked "stop" entries in the table. Native derivations of English stems keep -irati/-izam (chatirati), and the user's verbs win. To add or correct a word, add a row to `tools/formant/english_extra.tsv` and rerun `tools/formant/english_words.py` (needs `pip install wordfreq spylls`, `cmudict.dict` and the LibreOffice Hunspell hr_HR/sr-Latn/bs_BA files); it keeps native words out (Hunspell) and only writes words the rules get wrong. `english_words.py` mirrors `english_word()` (`english.rs` of `hr`, `sr` and `bs`) in `lookup()`, and asks the engine where it stresses a word without the table through `examples/english_stress.rs` (`cargo run --release --example english_stress`): change the four together. See "English words" in `docs/formant.md`.
- **Users have their own accent lexicon**: `accents.json` next to the user dictionaries, entries in the built-in notation as JSON (`{ "word": "kontr'o:l*" }`, `{ "verb": "ur'e:d=i<p" }`, optional `"language"`). `UserLexicon` (`src/common/text/frontend/user_lexicon.rs`) parses and validates it once; `Frontend::set_user_lexicon()` consults the user's forms, stems and verbs before the built-in ones. The engine keeps it across voice changes (`Engine::load_accent_lexicon()`, C API `laprdus_load_accent_lexicon()`, report via `laprdus_get_accent_lexicon_report()`); `laprdus_load_user_config()`, both CLIs, SAPI5, NVDA, Android (`LaprdusTTS.loadAccentLexicon`) and Apple (`DictionaryState.accentLexiconURL`) load it with the user dictionaries, and user dictionaries off removes it. Every platform's dictionary editor offers it as the fourth dictionary type (Windows `DictionaryType::Accents` with the entry editor of `dictionary_dialog.rs`, Android `DictionaryType.ACCENTS`, Apple `DictionaryType.accents`): an entry is word or verb (`verb` flag) plus `language` and `comment`, its text kept in the entry's grapheme field, written under the `word`/`verb` keys; before saving, the editor checks the entry with the engine's own parser (`laprdus_check_accent_lexicon()`, handle-free, returns the number of rejected entries and the first reason; Windows calls `UserLexicon::parse()` directly, Android `LaprdusTTS.checkAccentLexicon`, Apple `LaprdusEngine.checkAccentLexicon`) and shows the reason instead of saving. Duplicating an accent entry does not append " (copy)". See "Stress" in `docs/formant.md` and 5.11 in `docs/laprdus.md`.
- **Croatian tiflo- compounds** follow the user's requested two-component pronunciation, including established dictionary terms. `split_tiflo()` (`src/common/languages/hr/clause.rs`) splits after `tiflo` when the remaining letters contain a vowel or syllabic r; both parts get their ordinary accent analysis, and the second has `joined = true` so they share one prosodic word without an internal boundary pause. Lowercase, uppercase and inner capitals behave alike. The short falling `t^iflo` is a listener preference; HJP gives single accents in *tiflòlog, tiflològija, tiflòloškī*. Written accent marks and the user's whole-word exact, stem and verb entries override the split. See the source discussion in `docs/formant.md`.
- **Frontend structure tests** (`src/common/languages/tests.rs`, `src/common/languages/sl/tests.rs`, `src/common/text/frontend/tests.rs`) check accents and joined word boundaries directly. They are unit tests inside the engine crate, because these internal items are not public: `cargo test --release -p laprdus --lib`. Run these alongside the synthesis tests when changing frontend structure. The commands are in `docs/formant.md`, section 7.
- Platform code written in Rust does not go through `laprdus_set_voice()`: it calls `Engine::set_voice()` (the SAPI5 driver) or `Engine::set_voice_from()` (the Android JNI library, which reads a recorded voice's file from the APK's assets). Both load a formant voice without data (`Voice::is_formant()`) and keep the language chosen with `Engine::set_language()`; call `set_language()` when the host chose one.

**Singing presets.** `orguljas`, `dalmacija_klapa`, `lika_velebit` (Zvonko), `trubac`, `harmonikas`, `solunac` (Stojan), `sevdalija`, `sazlija`, `sarajevo_bembasa` (Mirsad), `becarac` (Zvonko, the bećarac tune) are formant voices whose `FormantVoice::singing` points at a `SingingStyle` (song, sound source, chorus, sub-octave, reverb, vibrato, note envelope, transpose; `src/common/formant/singing.rs`, with the presets and their songs in `src/common/languages/<code>/formant/mod.rs`). They sing the text to a public-domain folk song, one syllable per note (a `~` note in the song notation is a melisma, `R` a rest), the vowel on the beat with the consonants before it; the song restarts with every synthesis call (`Engine::begin_utterance()`), and clauses of one call continue it. Rate = tempo, pitch = transposition, inflection level = vibrato depth. A preset sings in whichever language is set; its native language is that of its song. `SingingStyle::dry` (lika_velebit, solunac, sarajevo_bembasa) keeps the voice's own source and phonation and transposes the song so the middle of its range sits five semitones above the voice's `base_f0`. Notes without a rest between them are legato: the attack envelope is applied only after a rest or at the start of a clause. Base voice is in `base_voice_id` (platform code uses it only as metadata; `data_filename` stays null). The instrument sources (`SourceKind`), chorus, sub-octave and reverb live in `src/common/formant/klatt.rs`; the speaking voices do not use them and their output is unchanged. See "Singing presets" in `docs/formant.md`.

```bash
B=target/release      # after: cargo xtask data && cargo build --release -p laprdus-cli
$B/laprdus -D data/voices -v zvonko "Dobar dan, kako ste?"
$B/laprdus -D data/voices -v stojan -o out.wav "Добар дан!"
$B/laprdus -D data/voices -v mirsad -r 2.0 "Brzo, ali razgovijetno."
$B/laprdus -D data/voices -v zvonko -L sr "Zvonko govori srpski: dve hiljade."
$B/laprdus -D data/voices -v vlado -L hr "Vlado govori hrvatski: dvije tisuće."
```

### Telephone numbers

`convert()` of `src/common/text/numbers/phone.rs` runs before grammatical number rules
in both reading modes. `+` / `00` international numbers and domestic leading-zero
numbers accept compact digits, spaces, slashes, dashes and parenthesized codes.
Country code, area/network code and the subscriber number after its third digit
are separated by the user's comma pause (`+385 91 1234567` → `+385 | 91 | 123 | 4567`,
six digits `123 | 456`; up to four digits are one group, eight are 4+4).
Digit mode uses standalone digit names (Slovenian *dve*); whole-number mode reads
cardinal groups. Surrounding words never inflect phone groups. Preserve every
leading zero, including optional `(0)` and Italy's significant zero. A screen
reader may take the plus sign before the engine sees it (NVDA at the
punctuation level "none" sends a space for it, at higher levels the word): a
number after the word *plus* / *плус* (`plus_word_before()`; not after a
number, `2 plus 385`) is international like one after the sign, and with
nothing at all a number that begins with 385, 386, 387 or 381 and goes on as
a national number of that country by first digit and length
(`bare_international_ok()`; written together or with the code as the first
group; not groups of three throughout, `385 912 345 678`) is one too:
` 385 91 123 4567  `, `385911234567`. `385 kuna`, `385 911 234` and another
country's number without its sign stay numbers. Country
codes and regional exceptions, ambiguity limits and sources are documented in
`docs/formant.md`, "Telephone numbers". Tests: the `telephone_*` cases of
`src/common/text/numbers/tests.rs` and the audio tests
`telephone_grouping_reaches_the_audio_and_honors_comma_pauses` (`tests/formant.rs`)
and `recorded_voices_pause_between_telephone_groups` (`tests/recorded.rs`).

### Currency amounts

`convert()` of `src/common/text/numbers/currency.rs` recognizes clear grouped decimals such as
`2.150,00` before the phone/identifier guards, so UI labels such as *Stanje*
and *Račun* cannot turn the thousands separator into a spoken dot. Explicit
`€` / `$` / `EUR` / `USD` amounts accept prefix/suffix currency, European and
US separators, space/NBSP/thin-space and apostrophe grouping, Unicode minus,
negative suffixes and accounting parentheses. Minus, decimal and currency
become words before punctuation segmentation; currency follows the amount
with the existing language's grammatical forms. Preserve fractional zeros
and the number mode. Digit mode also speaks an ordinary unary minus. Do not
infer a negative value from parentheses without a currency marker or mistake
an identifier/range hyphen for a minus. Sources, ambiguities and limits are in
`docs/formant.md`, "Currency amounts"; the currency tests cover normalization
(`currency_*` in `src/common/text/numbers/tests.rs`) and both audio renderers
(`currency_amounts_speak_their_value_minus_and_unit_before_punctuation` in
`tests/formant.rs`, `recorded_voices_speak_currency_signs_and_localized_amounts`
in `tests/recorded.rs`).

### Short pauses inside a clause: clock times, dates, cell addresses

A listener's report of November 2026: a screen reader says a spreadsheet
cell's contents and then its address, and "Marjan A17" was heard as "Marijana
17" (the A was the conjunction *a*, unstressed and leaning on the number); the
hours of a clock time ran into its minutes and the day of a date into the
month. The number converter now leaves three marks in the text
(`src/common/text/marks.rs`; Unicode noncharacters, which no host's text
contains, where a private use character can come from an icon font):

- `MINOR_BREAK_MARK` (U+FDD0, written as a word of its own): a short pause
  before the next word. Where: between the hours, minutes and seconds of a
  clock time (`12:30:45`, also `u 8.30`, `24:00`, Slovenian `ob 8.30`) and
  between the day, month and year of a numeric date (`10.11.2026.`,
  `10. 11. 2026.`, `5.3.`, the right side of `15.–20. 3.`).
- `ITEM_BREAK_MARK` (U+FDD2, written the same way): the pause between two
  items a screen reader says in one breath, the **whole comma pause** at every
  rate (`item_break_pause_ms()`; `Word::item_break`, `Phone::item_break`,
  `Utterance::item_break` next to the short pause's flags). Where: before a
  cell's address that follows other words (`Marjan A17`, `1250 C3`, `A1 B2`)
  and before a number that follows an address (`A20 5` is not A25). Not at the
  head of a clause, not after a preposition or conjunction (`u A1`,
  `od A1 do B5`, `v A1`; `leans_on_next()`), not in a range or formula
  (`A1:B5`, `=A1+B2`). It was the short pause until 2026-11-10: NVDA sends a
  cell as `Marjan   A17  ` (two items of one sequence, two spaces after each,
  joined by the driver), the engine paused for 54 ms at NVDA's rate 60, and
  the listener heard no pause in Excel. And still none after that: the
  driver's debug log showed that NVDA's Excel support says the address as two
  words, `'Marijan  '` then `'A 3  '`. `spaced_code_begin()` takes that form
  as an address too, but only as an item of its own, since written apart the
  letters are usually a word (`U 5 sati`, `A 3 su došla`, `NA 5 mjesta`): one
  space between one or two capitals and the number, and after the number the
  end of the text, of the line, or two spaces (the end of a screen reader's
  item); not after a word of three or more capitals (`DOLAZIM U 5`). To see
  what NVDA sends, create `%TEMP%\laprdus_debug` and restart NVDA: the driver
  logs every sequence to `%TEMP%\laprdus_debug.log`. Do that before believing
  a replay of what NVDA "should" send.
- `LETTER_CODE_MARK` (U+FDD1, right after the letters): one or two capital
  letters of the English alphabet glued to a number of up to seven digits
  (`A17`, `F5`, `S22`, `U21`, `AB12`, `H2O`, `A4-format`; `code_begin()`) are
  letters, not the words they spell: one letter by the spelling mode like any
  lone letter (*bə* or *be*; a vowel is a stressed word of its own), two by
  their names (*NA5* is *en a pet*). Not small letters (`u20:03` is the
  preposition), not three letters (`TOP10`, `HRT1`), not after a hyphen or
  slash (`R2-D2`, `PS/2`), not before a time or decimal (`U20:03`, `V1.5`).

The tokenizer (`tokenize()`) turns them into `Word::break_before` /
`Word::code`, `expand_words()` reads the letters, `finish_utterance()` puts
`Phone::break_before` on the word's first phone. The front end still reads the
clause whole, so every context rule sees all the words; the synthesizers then
call `split_at_breaks()` and render the parts one after another
(`FormantSynthesizer::synthesize_clause()`, `ConcatSynthesizer::synthesize_clause()`)
with a silence of **half the user's comma pause** between them, shorter by
the square root of the rate when speech is fast (`minor_break_pause_ms()`,
`MINOR_BREAK_PAUSE_PERCENT` 50: 50 ms at the default 100, 35 ms at 2x, 29 ms
at 3x; the whole comma pause at first and 65 % of it next, both of which the
listener found too long; `Engine::synthesize_clauses()` passes it). A part before a pause is a
continuation with under half of a comma's boundary movement
(`Utterance::minor_break`, `MINOR_BREAK_MOVEMENT` 0.45, last syllable
lengthened by `MINOR_BREAK_STRETCH` 1.15 instead of 1.30), a part after one
does not start again (`Utterance::continued`: no high first accent, no
wh-question peak), the last part keeps the clause's kind. A comma would have
ended the clause ("u dvanaest, trideset u uredu"). Singing presets ignore the
pauses. Both number modes; the digit mode now also sets a number off from the
letters it is glued to (`A17` was "Ajedan sedam", `mp3` "mptri").

Dates fixed in the same round (hr, sr, bs unless noted): the period after a
date's year before a lowercase word is the ordinal's and ends no sentence
(`10.11.2026. godine`, `od 1.12.2026. do 5.12.2026.`, `Rođen je 10.11.1985. u
Zagrebu`; `date_period_end()`); a day and month with leading zeros are a date
(`05.03.`, also Slovenian) and so are `Danas je 07.10.2026.` (nominative) and
`u 08.30` (a time); a day and month written together before *do, i* are a
date, as are two written apart before *do* and another date (`od 1.12. do
5.12.`, `od 1. 12. do 5. 12.`, also Slovenian; `1. 2. i 3. mjesto` stays an
enumeration); a hyphen after an ordinal's period stands for the dash
(`10.-15.11.2026.`, `3.-4. mjesto`, `1991.-2000.`). A date written together
and one written with spaces were already the same words; they are the same
samples. Tests: `src/common/text/numbers/tests.rs`
(`clock_times_and_numeric_dates_have_a_short_pause_between_their_parts`,
`numeric_dates_inside_a_sentence_with_leading_zeros_and_in_ranges`,
`a_cells_address_is_set_off_from_the_cells_contents`),
`src/common/languages/tests.rs` and `languages/sl/tests.rs`
(`a_short_pause_inside_a_clause_splits_it_into_parts`,
`the_letters_of_a_designation_are_letters_whatever_word_they_spell`),
`tests/formant.rs` `clock_times_dates_and_cell_addresses_have_short_pauses`,
`tests/recorded.rs` `recorded_voices_pause_inside_a_time_and_before_a_cells_address`;
details and measurements in `docs/formant.md`, "Short pauses inside
a clause".

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

### Stopping a synthesis, and audio clause by clause (November 2026)

A tester's report: after "stop speaking" (the Ctrl key in a screen reader, a touch on a phone) Laprdus finished the old utterance before it spoke the new one. Nothing could stop a synthesis: every platform rendered the whole text in one call, `laprdus_cancel` and the JNI `nativeCancel` were empty, and the NVDA driver lost a `cancel()` outright when it came while the item was still being synthesized (the player had nothing to stop yet, and the whole item was fed to it afterwards). All of that was as old as the first C++ version.

- **Engine:** `Engine::cancel_handle()` gives a `CancelHandle` (`src/common/engine.rs`; clonable, for any thread). `cancel()` makes the synthesis under way return `Error::Cancelled` at its next look: between clauses, breath groups, the parts of a clause and spelled characters, every 512 frames (about a second of sound) of a formant voice, and between the passes over a recorded clause. Every synthesis call starts with the request cleared, so **a stop that comes before a call begins stops nothing: the host remembers it** (NVDA's cancel count; the Android framework and SAPI drop such a request themselves). `Engine::synthesize_streaming(text, chunk_ms, callback)` hands every clause over as soon as it is rendered; the callback returns `ControlFlow`, and `Break` ends the synthesis as a cancel does. The streamed audio is the audio of `synthesize()`.
- **C API:** `laprdus_cancel(handle)` is the one function that may be called from another thread while the handle's thread synthesizes (it finds the engine's cancel handle in a table by the handle's address and never touches the engine); `laprdus_synthesize_streaming(handle, text, chunk_ms, callback, user_data)` with a `LaprdusAudioCallback` that returns non-zero to stop; a stopped call returns `LAPRDUS_ERROR_CANCELLED`. `laprdus_stream_begin` still renders everything first.
- **NVDA:** every queued item carries the cancel count of the moment it was queued, and the speech thread drops a stale item wherever it has got to with it (`_stale()`: before the synthesis, between clauses, before anything is fed or reported); text is synthesized through `laprdus_synthesize_streaming` and each clause is fed as soon as it is rendered; `cancel()` calls `laprdus_cancel`. A setting NVDA changes on its own thread while the speech thread holds the engine waits in `_pendingEngineCalls` (`_onEngine()`): a voice change in the middle of a synthesis used to replace the voice under it. A change of voice no longer reloads the dictionaries, and the user dictionaries' times are recorded also when they are switched off or `settings.json` is missing (everything was reloaded before every item then). Tests: `test_cancel` of `tests/nvda/test_driver.py`.
- **SAPI5:** `Voice::speak()` writes each clause to the site as it is rendered and ends the synthesis on `SPVES_ABORT` (test: the long-text abort case of `tests/windows/test_sapi5_driver.py`).
- **Android:** `nativeCancel(handle)` stops that engine's synthesis without waiting for its lock (`Slot` in `src/platform/android/native/src/lib.rs`; `nativeIsInitialized` does not take the lock either); `LaprdusTTS.synthesizeStreaming(text, spelled, chunkMs, sink)` hands the audio to an `AudioSink` (kept from R8 in `proguard-rules.pro`), and the service passes each piece to `callback.audioAvailable()` and stops the engine when the framework refuses it. The sink runs with the engine held and must not call the engine; the service's `exclusive { }` block now lasts as long as the audio is taken, so a settings change made meanwhile is applied when the utterance ends.
- **Speech Dispatcher:** the thread that reads the server's lines calls the cancel handle at `STOP`, `PAUSE` and `QUIT`, the audio goes out clause by clause, and other lines that come while a message is spoken are carried out after it (`deferred`).
- **Apple:** `laprdus_cancel` works through the C API; the Swift sources are unchanged (the extension still renders a whole SSML part) and nothing there has been built since the migration.

### Performance (November 2026)

`cargo run --release --example bench` (`examples/bench.rs`) prints, for Zvonko, Josip and Đedo, the time to load the voice, to speak a short phrase and a paragraph (and how many times faster than real time), the time to the first clause, and the time of voice and language changes; run it before and after a change, several times (one run varies by 15 %). Every change of that round left the audio as it was: the 80,267 renderings of `tools/golden/golden.py` have the same hashes.

- **A filter fed silence does not arrive at zero: it stalls at the smallest numbers a double can hold, and arithmetic on those is many times slower.** The frication branch of `KlattSynth` got there a few milliseconds after every hissing consonant and stayed there until the next one: three fifths of a formant paragraph's time. `frication_rung_out()` clears the branch once nothing in it is above `FRICATION_RUNG_OUT` (1e-25) and skips it until noise comes again; `Section::settle()` does the same for the hiss limiter's filters (`src/common/audio/boost.rs`). Any new recursive filter that can be fed exact zeros needs the same.
- Formant voices: the chorus ratio (`powf`) once per quality instead of once per sample, filter coefficients not recomputed for the same input (`SetFrom`), the reverb cleared in place. A paragraph 173 → 78 ms (360 times real time), a short phrase 4.5 → 2.7 ms.
- Recorded voices: the bank is analysed before the first synthesis and not at every change of the parameters (`ConcatSynthesizer::deferred()`: a derived voice was analysed two or three times), the pitch search sums four lags side by side (`ncc_over_lags()`), the vowels are prepared once; Hann window halves are tables (`window_half()` in `psola.rs`), the hiss limiter skips the logarithm where the band is clearly under its gate. Loading Josip 91 → 40 ms, Đedo 507 → 125 ms; a paragraph 61 → 52 ms.
- The limiter of the plain output forms the inter-sample peak only near samples that could reach the ceiling and returns when none does; the built-in lexicon of a language is built when a front end first reads it, not when an engine is made (a host that never speaks Croatian no longer builds the Croatian one).

## Audio Processing

### Recorded Voices (TD-PSOLA)

Rate, pitch and intonation of Josip and Vlado are produced by time-domain pitch-synchronous overlap-add over the pitch marks of the recordings (`src/common/concatenative/psola.rs`), with no third-party library. `docs/concatenative.md` has the full description.

- **Speed** (rate): the planner shortens or lengthens every segment's duration (same rules as the formant voices, rate floors for consonants); the renderer repeats or skips periods. Bursts are never stretched; closures take the time at slow rates and lose it first at fast ones.
- **Pitch** (voice character, derived voices): the spectrum of the recordings is warped by `sqrt(pitch)` at analysis time (a child's vocal tract), and the pitch itself by `pitch` at rendering.
- **User Pitch**: the output period spacing alone; formants stay where they are.
- **Intonation**: the shared `Intonation` model (`src/common/prosody/intonation.rs`) draws the contour from the front end's stress and clause kind.
- **Clarity (2026-11-17)**, after a listener's report that the recorded voices swallow their letters; "Clarity" in `docs/concatenative.md` has the measurements. Measured with the phoneme recognizer of `tools/formant/phones.py` (classes for every consonant; needs torch and transformers, on the Windows machine in `build/measure/venv`): the right consonant in 207 → 233 of 516 items at rate 1.0, 138 → 165 at 1.5, 96 → 116 at 2.0. **The recordings are the limit**: put end to end without any processing they are heard no better than the engine's rendering (Josip 57 of 115 raw, 58 rendered), because every recording is one sound alone and the vowels have no transitions (t and k are heard as p, g as b, n as m, raw as well); only recordings of syllables or diphones would mend that. Changed: the analysis finds the strong part of a recording (`Unit::strong_begin..strong_end`) and puts the `onset` of an affricate without an abrupt rise there (Josip's c, ć, đ: the planner had kept their quiet build-up and cut the friction at fast rates); an affricate short of time is played faster, not cut; a stop keeps `CLOSURE_MS` 18 ms (floor 8) of closure, and one that begins a word has its closure (the word gap counts as a part of it); z, ž and đ get voiced marks (pitch tracked under `MURMUR_HZ` 500 Hz); the hiss hold acts on the spans of the sibilants and f alone (`Segment::hiss`, `limit_hiss_in()`), lets go where they end (it had taken 2-8 dB off the start of the next vowel above 2 kHz and 6 dB off the burst of t) and is `RECORDED_HISS_DB` -4 dB instead of -10 (the recognizer heard the sibilants right more often the less they were held); the glide of *ije* gives up the hum of the j recording first. Tried and taken out: shorter joins, trimming every consonant's quiet start, holding t's burst. The recorded voices no longer match the C++ engine's output (`tools/golden/golden.py check`: 1,616 of 20,400 of the `recorded` set, and the formant renderings with z or ž), and `tests/dsp_against_cpp.rs` compares the formant synthesizer alone. `cargo run --release --example recorded_units -- data/hr/Josip` prints what the analysis makes of every recording. Tests: `tests/recorded.rs` `affricates_keep_their_friction_at_fast_rates`, `a_stop_that_begins_a_word_has_its_closure`, `the_hiss_hold_lets_go_where_the_hiss_ends`.

### Dual Pitch System
Two separate pitch parameters serve different purposes:

| Parameter | Range | Purpose | Effect |
|-----------|-------|---------|--------|
| `pitch` | 0.25 - 4.0 | Voice character (derived voices) | Recorded: formants × sqrt(pitch), F0 × pitch. Formant voices: F0 × pitch |
| `user_pitch` | 0.5 - 2.0 (formant 0.25 - 4.0) | User preference (SAPI5/NVDA slider) | F0 only - keeps voice identity |

`Engine::set_language()` tells the front end of any voice, recorded or formant, which language's rules, lexicon and letter names to use and sets the number dialect (`laprdus_set_language()`, the SAPI5 driver and the Android bridge call it); Josip and Vlado speaking Serbian or Bosnian say *hiljada*.

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

#### Windows 7 SP1, 8 and 8.1 (the legacy build)

Rust's Windows targets need Windows 10 since Rust 1.78, and some users are on older systems. The Windows programs are therefore built a second time, from the same crates, for Windows 7 SP1, 8 and 8.1 (`src/platform/windows/legacy/README.md` has the whole story):

```bash
rustup toolchain install nightly-2026-10-30 --component rust-src   # once; the name is in src/platform/windows/legacy/toolchain.txt
cargo xtask windows-legacy
python tests/windows/test_legacy_imports.py
```

Output: `dist/windows-legacy-x64/` (`laprd64.dll`, `laprdus.exe`, `laprdgui.exe`) and `dist/windows-legacy-x86/` (`laprd32.dll` and the same two programs).

- **Targets** `x86_64-win7-windows-msvc` and `i686-win7-windows-msvc` (tier 3: no standard library is shipped, so it is built from its sources with `-Zbuild-std` by the nightly `toolchain.txt` names, run through rustup; `LAPRDUS_LEGACY_TOOLCHAIN` overrides it). The workspace keeps the stable compiler: there is no `rust-toolchain.toml`, and nothing but this step uses the nightly.
- **Flags** (`LEGACY_RUSTFLAGS` in `tools/xtask/src/main.rs`): the C runtime linked statically (`+crt-static`; those systems do not come with it) and `combase.dll` delay-loaded (`/DELAYLOAD:combase.dll`, `delayimp.lib`). The `windows` crate takes `CoTaskMemAlloc`, `CoTaskMemFree` and `CoCreateFreeThreadedMarshaler` from `combase.dll`, which Windows 7 does not have; the crate `laprdus-win7` (`src/platform/windows/legacy/compat`) defines the delay-load helper's hook `__pfnDliNotifyHook2`, which answers the request for `combase.dll` with `ole32.dll`, where all three have always been.
- **The code is the same in both builds.** The SAPI5 driver and the Configurator link `laprdus-win7` under `cfg(target_vendor = "win7")` (one `use laprdus_win7 as _;` each) and differ in nothing else; do not add Windows-version branches to them. The ordinary build does not contain the crate, and its import table is what it was.
- **Who gets which build:** the installer carries both and installs by the Windows version (`IsLegacyWindows` in its `[Code]`: major version below 10), under the same file names; `MinVersion=6.1sp1`. The NVDA add-on carries the 32-bit legacy driver and Configurator as `laprd32_legacy.dll` and `laprdgui32_legacy.exe` and loads them in a 32-bit NVDA on Windows below 10 (`_is_legacy_windows()` in `_laprdus.py`, the same check in the global plugin); there is no 64-bit NVDA for those systems. A legacy program also runs on Windows 10 and 11.
- **NVDA on old Windows:** NVDA 2023.3.4 is the last for Windows 7 SP1 and 8 (Python 3.7, 32-bit), NVDA 2025.3.3 the last for Windows 8.1 and for 32 bits (Python 3.11), NVDA 2026.1 and later are 64-bit on Windows 10 and later (Python 3.13). The add-on's files must therefore stay Python 3.7 (`python tests/nvda/test_python37.py`); its manifest (`minimumNVDAVersion` 2019.3) admits all of them, and NVDA 2023.3 ignores the manifest's `symbolDictionaries` section (the Slovenian symbol dictionary is simply not loaded there).
- **Checked without a Windows 7 machine:** `tests/windows/test_legacy_imports.py` reads the import tables of the six staged files (nothing from a DLL Windows 7 lacks; `combase.dll` only delay-loaded, and only for functions of Windows 7's `ole32.dll`) and calls the driver's hook. The other Windows tests take a legacy file as an argument (`python tests/windows/test_sapi5_driver.py dist/windows-legacy-x64/laprd64.dll`), and the legacy driver gives the same audio as the ordinary one.
- **Not verified (November 2026):** nothing has been run on Windows 7, 8 or 8.1 themselves: installing, SAPI5 hosts, NVDA 2023.3.4 and 2025.3.3, the Configurator, the CLI's playback (`cpal` 0.16). The targets are tier 3, so a newer nightly may break the build: change `toolchain.txt` only to a nightly that builds, and rerun the tests.

#### SAPI5 Installer (InnoSetup)

```bash
# First the voice data and both architectures of the Windows programs
cargo xtask data
cargo xtask windows
cargo xtask windows-legacy   # the same for Windows 7 SP1, 8 and 8.1 (dist/windows-legacy-*)

# The NVDA add-on, which the installer carries
(cd src/platform/windows/nvda && scons)

# Then build installer
iscc installers/windows/laprdus_sapi5.iss
```

Output: `installers/windows/Output/Laprdus_SAPI5_Setup_2.0.0.exe`

The installer is in English, Croatian, Bosnian, Serbian and Slovenian (Slovenian: Inno Setup's own `Languages\Slovenian.isl`, with the Laprdus messages as `slovenian.*` in the script's `[CustomMessages]`; the others carry theirs in `Croatian.isl` / `Bosnian.isl` / `Serbian.isl`; `Bosnian.isl`, 0x141A, bs-Latn-BA, was made from `Croatian.isl`, and the two are changed together). A new message goes into all five; `python tests/windows/test_installer_languages.py` checks that every language has every Laprdus message and every message the script uses, and that our `.isl` files have the standard messages of the installed Inno Setup. Setup shows no language dialog (`ShowLanguageDialog=no`): it takes the display language of Windows, with an exact match on `LanguageID`, then the first entry of the same primary language (hr, sr and bs share 0x1A, so `Serbian.isl` has 0x241A, sr-Latn-RS, and Serbian and Bosnian in Cyrillic get Croatian), then the first entry, English; `/LANG=croatian` and the like force one. The installer carries both builds of the programs, `dist/windows-x64|x86` for Windows 10 and later and `dist/windows-legacy-x64|x86` for Windows 7 SP1, 8 and 8.1, and installs the one for the Windows it runs on (`Check: IsModernWindows` / `IsLegacyWindows`, `MinVersion=6.1sp1`; an installer made by Inno Setup 6.3 or later runs on Windows 7 SP1 and later), so build both before it. The installer carries the NVDA add-on (`src/platform/windows/nvda/laprdus-<version>.nvda-addon`, so build the add-on before the installer; it is installed into the install directory): where NVDA is installed (the `.nvda-addon` file type is registered), the last page has a checkbox, on by default, "Install the Laprdus add-on for NVDA" (`[Run]`, `postinstall shellexec runasoriginaluser skipifsilent`), which opens the add-on with NVDA, and NVDA asks whether to install it, as when the file is opened by hand. The add-on can still be installed on its own.

#### Laprdus Configurator languages

`laprdgui.exe` has string tables in English, Croatian, Serbian (Latin, ekavian) and Slovenian in `laprdus_config.rc`; the dialog templates are language-neutral and all their text is set from the tables at run time. `select()` (`src/platform/windows/configurator/src/ui_language.rs`) picks the table from Windows's display language (`GetUserDefaultUILanguage()`) or from `--language=<tag>`, which the NVDA add-on passes with NVDA's language, and sets it with `SetThreadUILanguage()`, which works without that language pack installed. Bosnian reads Croatian; any other language reads English. There must be no `LANG_NEUTRAL` string table: the loader prefers it over the thread's language (with one, the configurator was Croatian under an English Windows). The Test button speaks through the SAPI5 token of that language (Croatian under English), with `IDS_TEST_TEXT` in the language it speaks. The decimal separator and the percent format come from the tables too (`1,5x`, `50 %` in hr and sl, `50%` in sr; CLDR). Tests: `python tests/windows/test_configurator_language.py` (runs the real dialogs on a desktop of their own, checks the tables, access keys and text fit) and the unit tests of `ui_language.rs` (`cargo test -p laprdus-configurator`: display language and tag to table).

#### Windows CLI

There is one command-line program, `laprdus` (`src/platform/cli`, crate `laprdus-cli`), for Windows, Linux and macOS. It plays through the system's audio device with the `rodio` crate (WASAPI on Windows).

```bash
cargo xtask windows                    # both architectures, staged in dist/
cargo xtask windows-legacy   # the same for Windows 7 SP1, 8 and 8.1 (dist/windows-legacy-*)
cargo build --release -p laprdus-cli   # the host architecture only
```

Output:
- `dist/windows-x64/laprdus.exe` (64-bit)
- `dist/windows-x86/laprdus.exe` (32-bit)
- `target/release/laprdus.exe` (the host architecture, from `cargo build`)

The CLI is automatically included in the SAPI5 installer. For standalone use:
```bash
# Show help
laprdus.exe -h

# List available voices (each with its native language; every voice speaks every language)
laprdus.exe -l

# Speak text to audio device (Zvonko, Croatian)
laprdus.exe "Dobar dan!"

# Any voice in any language
laprdus.exe -v vlado -L hr "Dobar dan svima!"
laprdus.exe -L sr "Zdravo svete!"

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
| `-v, --voice` | Select voice (zvonko (default), stojan, mirsad, josip, vlado, detence, baba, djedo, or a singing preset; old ids accepted) |
| `-L, --language` | Language hr, sr, bs or sl; any voice speaks any language, without it a voice speaks its native language |
| `-r, --speech-rate` | Speech rate 0.5-2.0 (default: 1.0); formant voices 0.25-4.0 |
| `-p, --speech-pitch` | Speech pitch 0.5-2.0 (default: 1.0); formant voices 0.25-4.0 |
| `-I, --inflection` | Inflection level 0-100 (default: 50; 0 is a monotone) |
| `-a, --acceleration` | Rate multiplier 0.5-3.0 (default: 1.0) |
| `--head-size` | Voice quality: vocal tract length 0-100 (default: 50 = the voice as it is; 0 40 % longer, formants x0.71, a deeper voice; 100 30 % shorter, x1.4, a higher voice) |
| `--breathiness` | Breath 0-100: 0 a tense voice, 100 a whisper (default: 50; formant voices) |
| `--roughness` | Roughness 0-100: 0 perfectly steady, above 50 a rasp (default: 50; formant voices) |
| `--brightness` | Tone 0-100 (default: 50; 0 dull, with softer s and š, 100 sharp) |
| `-V, --speech-volume` | Volume 0.0-1.0 (default: 1.0) |
| `-B, --volume-boost` | Louder speech through a peak limiter, for noisy places |
| `-d, --numbers-digits` | Speak numbers as digits |
| `-s, --spell` | Spell the text character by character |
| `-S, --spelling-speed` | Spelling cadence 0-100; lower adds longer pauses (default: 50) |
| `-m, --spelling-mode` | `names` (be, ce, de; default) or `sounds` (b, c, d) |
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
   cd src/platform/windows/nvda
   scons
   ```

2. **NEVER do any of the following:**
   - NEVER manually create the .nvda-addon file using Python's zipfile or any archive tool
   - NEVER manually edit `addon/manifest.ini` - it is AUTO-GENERATED by SCons from `buildVars.py`
   - NEVER manually edit `addon/locale/*/manifest.ini` - these are AUTO-GENERATED
   - NEVER manually edit `addon/doc/en/readme.html` - it is AUTO-GENERATED from `readme.md`

3. **To modify addon metadata** (name, version, description, etc.):
   - Edit `src/platform/windows/nvda/buildVars.py` - this is the ONLY place to change addon info
   - SCons reads buildVars.py and generates manifest.ini automatically

4. **To modify addon documentation**:
   - The manual exists in four languages, and SCons converts each to HTML automatically: `src/platform/windows/nvda/addon/doc/hr/readme.md` (Croatian, the source), `src/platform/windows/nvda/readme.md` (English, the base language, copied to `addon/doc/en/` by the build), `src/platform/windows/nvda/addon/doc/sr/readme.md` and `src/platform/windows/nvda/addon/doc/sl/readme.md`. NVDA opens the one of its language, else English. Change them together, then run `python tests/nvda/test_manuals.py`
   - It is rendered by Python-Markdown with the extensions in `buildVars.markdownExtensions` (tables, fenced code, heading anchors that keep diacritics), not GitHub's renderer: nested list items take 4 spaces, a list or table needs a blank line before it, and a code block inside a list is indented, not fenced

5. **To add/modify translations**:
   - Edit `.po` files in `src/platform/windows/nvda/addon/locale/*/LC_MESSAGES/`
   - SCons compiles .po to .mo files automatically

Output: `src/platform/windows/nvda/laprdus-*.nvda-addon`

**Automatic Resource Handling**: The NVDA addon SCons build automatically:
- Copies DLLs from `dist/windows-x64/` and `dist/windows-x86/` (staged by `cargo xtask windows`)
- Copies GUI executables (`laprdgui.exe` as `laprdgui64.exe` and `laprdgui32.exe`) for the Laprdus Configurator
- Copies the 32-bit driver and Configurator for Windows 7 SP1, 8 and 8.1 from `dist/windows-legacy-x86/` (staged by `cargo xtask windows-legacy`) as `laprd32_legacy.dll` and `laprdgui32_legacy.exe`
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
# 1. Build voice data and stage the dictionaries first
cargo xtask data

# 2. Build both SAPI5 DLLs, the CLI and the config GUI for both architectures (dist/windows-x64, dist/windows-x86)
cargo xtask windows
cargo xtask windows-legacy   # the same for Windows 7 SP1, 8 and 8.1 (dist/windows-legacy-*)

# 3. Build NVDA addon (automatically copies DLLs, GUI executables, voice data, and dictionaries)
cd src/platform/windows/nvda && scons
```

**Localization**: The addon is in English (its base language), Croatian (hr), Serbian (sr, Latin, ekavian) and Slovenian (sl), by NVDA's language through `addonHandler.initTranslation()`. The manifest's description and changelog are translated only while the msgids match `buildVars.py` to the letter: change them together. Translation files are in:
- `src/platform/windows/nvda/addon/locale/hr/LC_MESSAGES/nvda.po`
- `src/platform/windows/nvda/addon/locale/sr/LC_MESSAGES/nvda.po`
- `src/platform/windows/nvda/addon/locale/sl/LC_MESSAGES/nvda.po`

The SCons build automatically compiles .po files to .mo files using either `msgfmt` (if available) or Python's `polib` library as a fallback. Install polib with: `pip install polib`

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

## Testing

### SAPI5 Voices

After installing SAPI5, test with PowerShell:
```powershell
Add-Type -AssemblyName System.Speech
$synth = New-Object System.Speech.Synthesis.SpeechSynthesizer
$synth.SelectVoice("Laprdus Croatian")   # or "Laprdus Serbian" / "Laprdus Bosnian"; the voice is settings.json voice.default
$synth.Speak("Dobar dan!")
```

SAPI5 registers four voice tokens, one per language ("Laprdus Croatian" 0x041A, "Laprdus Serbian" 0x081A, "Laprdus Bosnian" 0x141A, "Laprdus Slovenian" 0x0424), not one per Laprdus voice. The token sets the language; the voice comes from `settings.json` `voice.default` (empty = zvonko, old ids accepted), chosen in the Laprdus Configurator. There is no language key in `settings.json`. The installer removes the per-voice tokens of earlier versions on install, upgrade and repair, and re-registers the four.

### NVDA Addon

1. Install the `.nvda-addon` file
2. Restart NVDA
3. Select "Laprdus" in NVDA's synthesizer settings; the Language setting, which comes before the Voice setting, chooses hr, sr, bs or sl, and the Voice list holds all 18 voices (default Zvonko) for any of them

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
2. `buildNativeLibrary` runs `cargo ndk` for `arm64-v8a`, `armeabi-v7a`, `x86_64` and `x86` (`--platform 24`, `build --release -p laprdus-android`) into `app/build/rustJniLibs`. The library is built optimized for debug builds of the app too: an unoptimized synthesizer is too slow for a screen reader, and Rust keeps its bounds checks in release builds
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

## Linux

### Overview

LaprdusTTS supports Linux through:
- **Speech Dispatcher module** (`sd_laprdus`) - Enables integration with Orca screen reader and other SSIP clients
- **Command-line interface** (`laprdus`) - Direct TTS synthesis from terminal

### Building for Linux

```bash
# Voice data, CLI, Speech Dispatcher module and the C API library
cargo xtask build

# Build only the CLI
cargo build --release -p laprdus-cli

# Build only the Speech Dispatcher module
cargo build --release -p laprdus-speechd
```

Output: `target/release/laprdus`, `target/release/sd_laprdus`, `target/release/liblaprdus_capi.so` (installed as `liblaprdus.so.2.0.0` with the links `liblaprdus.so.2` and `liblaprdus.so`).

### Linux Dependencies

| Package | Purpose |
|---------|---------|
| Rust 1.85+ (`cargo`, `rustc`) | The engine and every program |
| `pkg-config`, `libasound2-dev` | ALSA audio output of the CLI (through the `rodio` crate; PulseAudio and PipeWire serve ALSA) |

The Speech Dispatcher module is a Rust program that speaks the module protocol over its standard input and output; it needs neither `libspeechd-dev` nor GLib to build.

Install on Debian/Ubuntu:
```bash
sudo apt install pkg-config libasound2-dev
```

Install on Fedora/RHEL:
```bash
sudo dnf install pkgconf-pkg-config alsa-lib-devel
```

### Command-Line Interface Usage

```bash
# Basic usage
laprdus "Dobar dan!"

# Select voice (speaks its native language, Serbian for Vlado)
laprdus -v vlado "Zdravo svete!"

# Select the language separately: any voice speaks any language
laprdus -v vlado -L hr "Dobar dan svima!"
laprdus -L bs "Dobar dan!"

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
| `-v` | `--voice` | Select voice (zvonko (default), stojan, mirsad, josip, vlado, detence, baba, djedo, or a singing preset; old ids accepted) |
| `-L` | `--language` | Language hr, sr, bs or sl; any voice speaks any language, without it a voice speaks its native language |
| `-r` | `--speech-rate` | Speech rate (0.5-2.0, default 1.0; formant voices 0.25-4.0) |
| `-p` | `--speech-pitch` | Speech pitch (0.5-2.0, default 1.0; formant voices 0.25-4.0) |
| `-I` | `--inflection` | Inflection level (0-100, default 50) |
| `-a` | `--acceleration` | Rate multiplier (0.5-3.0, default 1.0) |
| | `--head-size` | Voice quality: vocal tract length 0-100 (default 50 = the voice as it is) |
| | `--breathiness` | Breath 0-100: 0 a tense voice, 100 a whisper (default 50; formant voices) |
| | `--roughness` | Roughness 0-100: 0 perfectly steady, above 50 a rasp (default 50; formant voices) |
| | `--brightness` | Tone 0-100 (default 50; 0 dull, with softer s and š, 100 sharp) |
| `-V` | `--speech-volume` | Volume (0.0-1.0, default 1.0) |
| `-B` | `--volume-boost` | Louder speech through a peak limiter, for noisy places |
| `-d` | `--numbers-digits` | Speak numbers as digits |
| `-s` | `--spell` | Spell the text character by character |
| `-S` | `--spelling-speed` | Spelling cadence 0-100; lower adds longer pauses (default: 50) |
| `-m` | `--spelling-mode` | `names` (default) or `sounds` |
| `-c` | `--comma-pauses` | Comma pause duration in ms |
| `-e` | `--period-pauses` | Period pause duration in ms |
| `-x` | `--exclamationmark-pauses` | Exclamation pause duration in ms |
| `-q` | `--questionmark-pauses` | Question mark pause duration in ms |
| `-n` | `--newline-pauses` | Newline pause duration in ms |
| `-o` | `--output-file` | Output to WAV file |
| `-i` | `--input-file` | Read text from file |
| `-h` | `--help` | Show help |

### Speech Dispatcher Integration

**Automatic Configuration**: When installing LaprdusTTS via packages (deb, rpm, PKGBUILD) or the tarball's `install.sh`, Speech Dispatcher is configured automatically. No manual configuration is needed.

The installation:
1. Installs the module binary to `/usr/lib/speech-dispatcher-modules/sd_laprdus`
2. Installs the config to `/etc/speech-dispatcher/modules/laprdus.conf`
3. Automatically adds `AddModule "laprdus"` to `/etc/speech-dispatcher/speechd.conf`
4. Sets LaprdusTTS as the default for Croatian (hr), Serbian (sr), Bosnian (bs) and Slovenian (sl)

The module lists every voice for every language; `SET LANGUAGE` calls `Engine::set_language()` and keeps the voice; `MALE1` is Zvonko for all three languages (MALE2 Josip, MALE3 Đedo, FEMALE1 Baba, CHILD_MALE Detence).

After installation, restart Speech Dispatcher:
```bash
systemctl --user restart speech-dispatcher
```

Test with spd-say:
```bash
spd-say -o laprdus "Dobar dan!"
spd-say -o laprdus -l hr "Hrvatski tekst"
spd-say -o laprdus -l sr "Srpski tekst"
spd-say -o laprdus -l sr -y "Zvonko" "Zvonko govori srpski"
spd-say -o laprdus -l bs -y "Josip" "Josip govori bosanski"
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
sudo dpkg -i laprdus_2.0.0-1_amd64.deb     # one package: CLI, library, Speech Dispatcher module, voice data, dictionaries

# Fedora/RHEL
sudo rpm -i laprdus-2.0.0-1.fc*.x86_64.rpm  # one package as well

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
cargo xtask build
sudo cargo xtask install --prefix /usr
```

`cargo xtask install` copies the files (default prefix `/usr/local`; `--destdir` stages for a package). Adding the module to `speechd.conf` and restarting Speech Dispatcher is done by the packages' and the tarball's own scripts, not by this step: see "Manual Configuration" above.

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

### Engine and Program Tests (Cargo)

The tests of the engine and of the programs are Cargo tests and run the same way on Windows, Linux and macOS (not yet run on macOS since the migration).

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

#### Testing Checklist for Linux

- [ ] `cargo xtask build` builds the C API library (`liblaprdus_capi.so`), the CLI (`laprdus`) and the Speech Dispatcher module (`sd_laprdus`) without errors
- [ ] `cargo test --release --workspace` passes
- [ ] CLI `-h` displays help correctly
- [ ] CLI `-l` lists all 18 voices with their native language
- [ ] CLI synthesizes audio to WAV file correctly
- [ ] All voices synthesize correctly (josip, vlado, detence, baba, djedo, zvonko, stojan, mirsad, the singing presets), and `-L hr|sr|bs|sl` switches the language of any of them
- [ ] Formant voice tests pass (`cargo test --release --test formant`)
- [ ] Recorded voice tests pass (`cargo test --release --test recorded`, needs `data/voices` or `LAPRDUS_DATA`)

#### Linux Package Testing (if building packages)

The packages build with Cargo (`cargo xtask build`, then `cargo xtask install --destdir ...`); the exact steps are in `.claude/skills/build-deb`, `build-rpm`, `build-arch` and `build-tarball`.

```bash
# Debian package (the debian directory is linked to the project root for the build)
ln -sfn installers/linux/deb/debian debian && dpkg-buildpackage -us -uc -b -rfakeroot -d; rm -f debian

# Arch package (the PKGBUILD takes laprdus-2.0.0.tar.xz next to it: see the build-arch skill)
cd installers/linux/arch && makepkg -sf

# Tarball: dist/linux-<arch>/laprdus-<version>-linux-<arch>.tar.xz
cargo xtask dist

# An installed package or tarball
tests/linux/test_package_install.sh /usr
```

#### Building RPM Package (Fedora)

The RPM build requires running on a Fedora system (native or WSL).

**1. Install build dependencies:**
```bash
sudo dnf install -y rpm-build rpmdevtools cargo rust gcc pkgconf-pkg-config alsa-lib-devel
```

The engine needs Rust 1.85 or newer. Cargo downloads the crates of `Cargo.lock` during the build, so the build needs network access.

**2. Set up rpmbuild tree:**
```bash
rpmdev-setuptree
```

**3. Create source tarball and build:**
```bash
# From project root (xz: the spec's Source0 is .tar.xz)
tar --transform='s,^\.,laprdus-2.0.0,' -cJf ~/rpmbuild/SOURCES/laprdus-2.0.0.tar.xz \
    --exclude='./.git' --exclude='./target' --exclude='./dist' --exclude='./build' \
    --exclude='*.pyc' --exclude='__pycache__' \
    --exclude='./src/platform/android/.gradle' --exclude='./src/platform/android/app/build' \
    --exclude='./src/platform/windows/nvda' .

# Copy spec and build (--nodeps when Rust comes from rustup, not dnf)
cp installers/linux/rpm/laprdus.spec ~/rpmbuild/SPECS/
rpmbuild -ba ~/rpmbuild/SPECS/laprdus.spec
```

The spec builds with `cargo xtask build` and installs with `cargo xtask install --destdir %{buildroot} ...`.

**4. Output files:**
```
~/rpmbuild/RPMS/x86_64/laprdus-2.0.0-1.fc*.x86_64.rpm      # Main package (CLI, library, Speech Dispatcher module, voice data, dictionaries)
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
sudo rpm -ivh ~/rpmbuild/RPMS/x86_64/laprdus-2.0.0-1.fc*.x86_64.rpm

# Test
laprdus -l              # List voices
laprdus "Dobar dan"     # Speak text
laprdus -o test.wav "Test"  # Save to file
spd-say -o laprdus "Zdravo" # Via Speech Dispatcher
```

**Note:** The Speech Dispatcher module is part of the main RPM (`%{_libdir}/speech-dispatcher-modules/sd_laprdus`): it is a Rust program that needs no Speech Dispatcher headers to build. The library has the soname `liblaprdus.so.2`.

### Build Verification Checklist

Before marking a task complete, verify:
- [ ] `cargo xtask windows` builds the SAPI5 x64 and x86 DLLs, the CLI and the Configurator without errors
- [ ] `cargo xtask windows-legacy` builds the same for Windows 7 SP1, 8 and 8.1, and `python tests/windows/test_legacy_imports.py` passes
- [ ] `cargo test --release --workspace` passes
- [ ] SAPI5 installer builds successfully
- [ ] NVDA addon builds successfully
- [ ] `python tests/nvda/test_driver.py` and the other Python tests pass
- [ ] Linux: `cargo xtask build` builds the library, the CLI and the Speech Dispatcher module without errors
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

`Engine::synthesize_spelled()` (C API `laprdus_synthesize_spelled()`) reads text character by character, at the normal speech rate, with a pause after every character, including the last. It is what every platform calls for screen reader spelling, typed-character echo, SAPI5 `SPVA_SpellOut` and single-character requests. Per character (`spell_character()`):

- **A letter of the alphabet** (`spelling_letter()` in `src/common/text/frontend/letters.rs`: Latin or Cyrillic, any case, the ligatures and љ/њ/џ as digraphs, ä-type letters folded) is read, by default, by its **sound** (`SpellingMode::LetterSounds`, the default of `VoiceParams`, `UserSettings`, a `settings.json` without the key, the NVDA driver, Android and Apple since 2026-10-18; before that the names were the default and the switch went unnoticed) or, when `VoiceParams::spelling_mode` is `SpellingMode::LetterNames`, by its **name** in the language currently set (`Engine::language()`, for every voice; `letter_name()` of the language's `words.rs`: Croatian and Bosnian *a, be, ce, če, će, de, dže, đe, e, ef, ge, ha, i, je, ka, el, elj, em, en, enj, o, pe, er, es, eš, te, u, ve, ze, že*, foreign *ku, duplo ve, iks, ipsilon*; Serbian per decision 41 of the Odbor za standardizaciju srpskog jezika: *lje, nje, ša*, the same for Cyrillic and Latin; Slovenian per Slovenski pravopis § 7: the same names for its 25 letters, *dvojni ve* for w, *mehki če*/*mehki dže* for ć/đ). The sound: `Frontend::letter_sound()` builds an `Utterance` with `isolated_sound = true` (a vowel letter as a stressed syllable; a consonant letter as the consonant plus a stressed `Ph::Schwa` release vowel, "bə", "sə", "mə", as synthesizer B's Croatian voice sounds letters out: measured 70-90 ms of consonant and 110-130 ms of vowel at full level; the foreign letters *q, x, y* (`is_foreign_letter()` in `letters.rs`, which also has *w*) keep their names in sound mode too, and *w* is read by `expand_words()` as *duplo ve* / *dvojni ve* by names and *duplo v* / *dvojni v*, the v by its sound, by sounds, spelled and in running text alike, a listener's report), and `FormantSynthesizer::synthesize_letter_sound()` / `ConcatSynthesizer::synthesize_letter_sound()` render it with fixed durations. The same letter names are used for spelled-out abbreviations in running text ("USB", "HR").
- **The prefix of a web address** (`address_letters()` in `src/common/text/frontend/tokenizer.rs`) is read letter by letter in any case and every language and spelling mode, each letter with its accent: *www* is *ve ve ve*, *https* *ha te te pe es*, *http* *ha te te pe*, *ftp*, *ftps*, *sftp* likewise (a listener's report). In Slovenian any other abbreviation keeps one accent on its last letter, and two written apart are two abbreviations (USB PDF; they used to run together).
- **Everything else** (digits, punctuation, symbols) is read by its entry in the bundled spelling dictionary, or spoken as text when it has none; in Slovenian by the language's own names (`spelled_name()` in `src/common/languages/sl/words.rs`: ena, dva, pika, vejica, afna ...) before the bundled dictionary (a Slovenian 2 is *dve*). Since the migration to Rust a spelled `]` is named too: the C++ spelling dictionary parser could not read that entry and left it silent.
- **The user's spelling entries** (`add_spelling_entry()`, `append_spelling_dictionary()`, a second `SpellingDictionary` layer) win over all of that, with one exception: in sound mode a user entry that is merely a letter's name (`is_letter_name()` in `letters.rs`: the built-in names of any language, plus the old *jot*) gives way to the sound; any other entry is spoken as written. `load_spelling_dictionary*()` replaces the bundled layer and clears the user layer, so every platform's "load bundled, then add the user's" sequence keeps working.
- **Capital letters announced by a screen reader** arrive as text, not as a spelled character: NVDA "veliko N" (hr/sr/bs) and "cap N", TalkBack "veliko slovo N" (hr), "велико Н" (sr), "veliko N" (sr Latin, bs), VoiceOver "veliko početno slovo N" (hr). `Engine::synthesize()` (and `synthesize_streaming()`) recognize a whole request of *veliko*/*cap*/*capital*, optionally *slovo*/*početno*/*letter*, and one letter, and render the words as text followed by `synthesize_spelled()` of the letter, so the spelling mode, spelling speed and user spelling entries apply on every platform. Anything else ("Plan B", "veliko N je slovo") is ordinary text, where a lone letter follows the next rule.
- **A consonant letter standing alone in running text** (`lone_consonant_letter()` in `src/common/text/frontend/tokenizer.rs`: one consonant, or lj/nj/dž, as a word of its own) follows the spelling mode too, because that is how a screen reader reports a character inside a message: TalkBack "Obrisano B", VoiceOver's "B" and "deleted" as separate SSML parts, a capital announcement the host splits up. `Frontend::process(text, punct, letter_sounds)` (passed by both synthesizers from `VoiceParams::spelling_mode`) makes it the word "bə", the consonant and a stressed `Ph::Ax` (rendered by both kinds of voices inside running speech; the recorded voices use their e), a full word, never a clitic; in name mode it is the letter's name as before, and a lone digraph is one letter in both modes (Њ "nje", not "en je"). Not affected: a leaning preposition (s, k; Slovenian v, s, z, k, h before a word, see `leaning_preposition()`), the letters of an abbreviation (USB, BMW, s.a.r.s.), a letter before a hyphen (S-klasa), q/x/y (named in both modes) and vowels. Plain text therefore reads "Plan B", "F5" and "Mercedes S klasa" with the sound too, as synthesizer B's Croatian voice does. Tests: `src/common/languages/tests.rs` and `languages/sl/tests.rs` `a_lone_consonant_letter_is_its_sound_in_letter_sound_mode`, `tests/formant.rs` `a_lone_letter_in_running_text_follows_the_spelling_mode`, `tests/recorded.rs` `letter_sounds_work_with_a_recorded_voice`. NVDA marks a spelled capital by pitch itself: `PitchCommand(offset=capPitchChange)` before the letter and `PitchCommand()` after it, sent only to a synthesizer whose `supportedCommands` include `PitchCommand` (`speech.getSpellingSpeech()` sets the change to 0 otherwise). The add-on lists `PitchCommand`, `RateCommand` and `VolumeCommand`, carries the changes in force with every queued item and applies them in the speech thread just before synthesis (`_applyProsody()`: the slider value, or the middle of the slider when the Laprdus value is forced, plus the command's offset or times its multiplier, clamped to 0-100, then scaled as the slider would be), so NVDA's "Capital pitch change percentage" works for every voice, forced pitch included, and the next item comes back down; NVDA replays a change still in force at the start of every sequence it pushes, so each `speak()` starts from the sliders. `tests/nvda/test_driver.py` runs the driver under stubbed NVDA modules (`tests/nvda/nvda_stubs.py`, after NVDA's `speech/commands.py`) against the add-on's DLL and checks this, the spelling mode read from `settings.json` and its reload, rate and volume commands and the delayed character descriptions: `python tests/nvda/test_driver.py`.
- **Spelling speed** (`VoiceParams::spelling_speed`, 0-100, default 50): characters keep the normal speech rate and acceleration. `spelling_rate_factor()` (2^((s-100)/100): 1.0 at 100, 0.71 at 50, 0.57 at 20, 0.5 at 0) controls cadence only: after each character, append `pause_settings.spelling_pause_ms` plus its rendered duration × (1/factor − 1) of silence. Thus 100 adds no extra silence, 50 adds about 41% of the character duration, 20 about 74% (what 50 added before 2026-10-14, when the base was 3 and the default felt too slow), and 0 adds its whole duration. The last character gets the same pause, so whole-word and individual-character calls have identical spacing; `Engine::synthesize_spelled(text, false)` (it returns the audio and the pause's length in ms) / `laprdus_synthesize_spelled_ex()` leave that last pause out and report its length, for a host that times the gap after a spelled character itself. **NVDA's delayed character descriptions** ("Delayed descriptions for characters on cursor movement") speak the character as one utterance and then a second one of `BreakCommand(1000)` plus the description; NVDA pushes the second as soon as the first's index is reported. The add-on plays a `BreakCommand` as silence of its length (it used to become a space) and plays a spelled character without its trailing pause, owing that pause to the next item: another character or ordinary speech gets it (less the real time already spent in silence, so spelling a word keeps its cadence), a break cancels it, so NVDA's one second alone sets the gap before the description, and `cancel()` drops it. C API `laprdus_set_spelling_mode()` / `laprdus_set_spelling_speed()`; `settings.json` section `"spelling": { "speed": 50, "mode": "sounds" | "names" }` (parsed within the section, so its `speed` is not confused with `speech.speed`; only an explicit `"names"` turns the sounds off); keys `spelling_speed` / `spelling_mode` on Android and Apple; CLI `-s` (spell the text), `-S`, `-m`. Every settings screen has the spelling speed in its pauses section and a "spell letters by their sounds" switch, on by default, in its advanced/options section (Android: both in the Reading section, the switch named for what "on" does: *Slova kao glasovi* in hr/sr, *Črke kot glasovi* in sl, *Letters as sounds* in en, with the summary "Pri slovkanju i tipkanju" (sr "Pri sricanju i kucanju", sl "Pri črkovanju in tipkanju"); not *Sricanje*, which in Croatian and Serbian means spelling and suggested the letter names when on, and no example letters, which Laprdus reads by this very switch); NVDA takes both from `settings.json`. The voice settings are ordered speed, acceleration, pitch, inflection level, volume everywhere, then the volume boost and the four voice quality settings (head size, breathiness, roughness, brightness); Android shows speed, pitch and volume, then "More voice settings" with acceleration, inflection, volume boost and the four voice quality settings. The inflection on/off switch of version 1.0 is gone everywhere (`VoiceParams`, the C API, `settings.json`, the Android and Apple settings keys): the inflection level at 0 is the monotone, and a 1.0 file's `speech.inflection` is not read, since it controlled a different effect (a pitch jump at the punctuation).
- Tests: the spelling cases of `tests/formant.rs` and `tests/recorded.rs` (their names contain `spell` or `letter`).

## Spelling Dictionary

The spelling dictionary `data/hr/spelling.json` (staged as `data/dictionary/spelling.json`) maps individual characters to their pronunciations for screen reader spelling mode (character-by-character reading). It holds digits, punctuation and symbols only; letters are named by the engine (see "Spelling" above).

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
- **Numbers**: 0-9 (spoken as Croatian words)
- **Punctuation**: Common symbols with Croatian names
- **Special characters**: @, #, $, etc.

### Usage

- **NVDA**: Automatically used in character mode (reading character-by-character)
- **SAPI5**: Used with SPVA_SpellOut action (spell commands)
- **Android**: Available via `synthesizeSpelled()` API

### Rust API

```rust
use laprdus::engine::Engine;
use std::path::Path;

let mut engine = Engine::new();
engine.set_voice("zvonko", Path::new("data/voices"))?;

// Load spelling dictionary
engine.load_spelling_dictionary_file(Path::new("/path/to/spelling.json"));

// Synthesize text in spelling mode: the samples and the length of the last pause
let (samples, _final_pause_ms) = engine.synthesize_spelled("ABC", true)?;
// Result audio: "A Be Ce"
```

### C API

```c
laprdus_load_spelling_dictionary(handle, "/path/to/spelling.json");
laprdus_synthesize_spelled(handle, "ABC", &samples, &format);
```

## macOS (native library and CLI)

Separate from the Apple app in `src/platform/apple/` (which builds with Xcode), the
Cargo build supports macOS as a first-class platform for the C API library and the CLI.

**Not yet built on a Mac since the migration to Rust**: what this section and the next
say about the Cargo build is what `tools/xtask/src/install.rs` and
`scripts/build-macos-release.sh` do, unverified on macOS.

```bash
# Builds the voice data, liblaprdus_capi.dylib and the laprdus CLI for the host
# architecture (arm64 on Apple Silicon, x64 on Intel).
cargo xtask build

# Or via the master script
./scripts/build-all.sh macos
```

| Component | Output |
|-----------|--------|
| C API library | `target/release/liblaprdus_capi.dylib` (installed and packed as `liblaprdus.dylib`) |
| CLI | `target/release/laprdus` |

The CLI is the same crate as on Windows and Linux (`src/platform/cli`)
and plays audio through **CoreAudio** by way of the `rodio` crate:

```bash
B=target/release
$B/laprdus -D data/voices -l                         # list voices
$B/laprdus -D data/voices "Dobar dan!"               # speak via CoreAudio
$B/laprdus -D data/voices -o out.wav "Dobar dan!"    # write a WAV
```

The engine is linked into the CLI, so the program needs no library next to it. The
dylib is for programs written in other languages: `cargo xtask` links it with the
install name `@rpath/liblaprdus.dylib` (`install` and `dist` fail when it is missing),
so a program that links it finds it through its own run path.

Both architectures: `--universal` (`cargo xtask build|install|dist --universal`)
builds `aarch64-apple-darwin` and `x86_64-apple-darwin` and joins them into one binary
(`rustup target add aarch64-apple-darwin x86_64-apple-darwin`).

### macOS release build (distribution outside the Mac App Store)

```bash
./scripts/build-macos-release.sh                 # = ./scripts/build-all.sh macos-release
./scripts/build-macos-release.sh --development   # pipeline check without a Developer ID
```

`scripts/build-macos-release.sh` produces, in `dist/macos/`:

- `Laprdus-<version>-macOS.dmg`: the app with the `LaprdusVoices` extension, archived by
  `xcodebuild archive` (Release, arm64 + x86_64), exported with the `developer-id`
  method, notarized with `notarytool`, stapled, in a signed, notarized and stapled
  drag-to-Applications disk image; plus `Laprdus-<version>-macOS-dSYMs.zip`.
- `laprdus-<version>-macos-universal.tar.xz`: the CLI, `liblaprdus.dylib`, headers,
  voice data, dictionaries, `install.sh` / `uninstall.sh` (`installers/macos/tarball/`),
  built by `cargo xtask dist --universal --codesign-identity ...`
  and submitted for notarization as a zip (a tarball cannot be stapled).
- `SHA256SUMS.txt`.

It needs the team's Apple Developer Program membership signed in to Xcode: Xcode then
signs the app with the team's **cloud-managed Developer ID certificate** (the key stays with
Apple, `-allowProvisioningUpdates` uses it; it never appears in `security find-identity`).
`codesign` cannot use that certificate, so without a local "Developer ID Application"
certificate in the login keychain the command-line tools and the DMG stay unsigned and the
tools are not notarized (the script says so). A local certificate is made from a CSR at
developer.apple.com > Certificates > + > Developer ID Application and imported into the
login keychain. Notarization needs credentials stored once with `xcrun notarytool
store-credentials laprdus` (`--notary-profile` / `LAPRDUS_NOTARY_PROFILE` for another
name). The distributor's own setup notes live outside the repository. `--skip-notarization` signs without
notarizing, `--development` signs with Apple Development and exports with the
`debugging` method for testing the pipeline, `--identity` picks the certificate,
`--skip-cli`, `--output`, `--clean` (Xcode work directory `build/macos-release/`).
The script checks that both binaries of the app are universal, that the hardened
runtime is on (`ENABLE_HARDENED_RUNTIME = YES` in the project's Release configuration,
required by notarization), verifies the signatures and runs `spctl` on the results.
The version comes from `MARKETING_VERSION` in the Xcode project (the Cargo workspace's
version names the tarball; the script warns when they differ).

`cargo xtask` options behind it: `--universal`, `--codesign-identity ID` (env
`LAPRDUS_CODESIGN_IDENTITY`; the library and the CLI are signed with a secure
timestamp and, for the CLI, the hardened runtime; `-` is an ad hoc signature
without either, since it has no certificate) and the `dist` step, which stages
`dist/macos-universal/laprdus-<version>-macos-universal/` (`dist/macos-<arch>/...`
without `--universal`) with the Linux tarball's `bin/lib/include/share` layout and
packs it. The engine is linked into the CLI, so it runs from `target/` and from the
tarball alike without a run path. As said above, none of this has been run on a Mac
since the migration.

**Not built on macOS:** SAPI5, the NVDA addon, the Windows config GUI and the
Speech Dispatcher module are all platform-specific and remain Windows/Linux only.
`./scripts/build-all.sh all` detects the host and skips what it cannot build,
warning rather than failing.

## Apple (iOS / iPadOS / macOS)

### Overview

The Apple port lives in `src/platform/apple/Laprdus/Laprdus.xcodeproj` (Xcode 26+; the project moved here from a folder of its own at the root with the migration to Rust) and mirrors the Android app. **The project has not been built on a Mac since the migration**: the paths, the script phase and the build settings below are what the project file and `build-rust-engine.sh` say, unverified. Deployment targets: **iOS 16.0** and **macOS 13.0** — the minimum versions supporting `AVSpeechSynthesisProviderAudioUnit`; the SwiftUI code deliberately uses only iOS 16 / macOS 13 APIs (`ObservableObject`/`@Published`, single-parameter `onChange`, no `@Observable`/`@Bindable`, no `navigationDestination(item:)`). visionOS is not supported.

| Target | Purpose |
|--------|---------|
| `Laprdus` | Multiplatform SwiftUI app (iOS, iPadOS, macOS) with a 4-tab layout: Main (sample text + play button), Settings, Dictionaries, About |
| `LaprdusVoices` | Speech synthesis provider app extension (`AVSpeechSynthesisProviderAudioUnit`, `ausp` Audio Unit) — exposes every voice under each of the three languages system-wide to VoiceOver/Spoken Content (one `AVSpeechSynthesisProviderVoice` per voice × language, identifier `com.hrvojekatic.laprdus.<id>.<lang short>`; the request's voice identifier gives both voice and language), like the Android `TextToSpeechService` |
| `LaprdusTests` | Unit tests (Swift Testing), hosted in the app |

### Structure

```
src/platform/apple/Laprdus/     (next to it: src/platform/apple/build-rust-engine.sh)
├── Laprdus/         App-only code (views, AppModel, AudioPlayer, assets, Localizable.xcstrings)
├── Shared/          Code compiled into BOTH app and extension:
│                    LaprdusC.h (bridging header), LaprdusEngine.swift (C API wrapper),
│                    SettingsStore.swift, DictionaryStore.swift, VoiceCatalog.swift, AppGroup.swift
├── LaprdusVoices/   Extension-only code (LaprdusAudioUnit.swift, SSMLParser.swift)
├── Config/          LaprdusVoices-Info.plist + entitlements (app group: group.com.hrvojekatic.laprdus)
└── LaprdusTests/    Unit tests
```

- The engine is the Rust static library `liblaprdus_capi.a` (crate `laprdus-capi`, the C API). Both targets have a "Build the Rust engine" script phase that runs `src/platform/apple/build-rust-engine.sh`: it builds the crate with Cargo for the platform and the architectures Xcode is building (always `--release`, also for Debug builds of the app: an unoptimized synthesizer is too slow for a screen reader), joins the architectures and puts the library into the products folder, where both targets link it (`LIBRARY_SEARCH_PATHS = $(BUILT_PRODUCTS_DIR)`, `OTHER_LDFLAGS = -llaprdus_capi`). Swift reaches it through the bridging header `Shared/LaprdusC.h`. It needs rustup with the targets of the platforms built: `rustup target add aarch64-apple-darwin x86_64-apple-darwin aarch64-apple-ios aarch64-apple-ios-sim x86_64-apple-ios`. Not yet built on a Mac: unverified.
- Voice data (`data/voices/*.bin`) and dictionaries (`data/dictionary/*.json`) are bundle resources of both targets. **On a fresh checkout, generate voice data first** (`cargo xtask data` or `./scripts/build-all.sh voice-data`) — `.bin` files are gitignored.
- **What the system needs before it lists the voices** (from Apple's "Creating a custom speech synthesizer" sample and from other working speech extensions; all three were missing until October 2026). Note: on macOS 27 the voice load was still failing after these were added (`TextToSpeech.MultiError`, extension never launched), with the cause not yet found:
  - the extension's `AudioComponents` entry declares `sandboxSafe` (`Config/LaprdusVoices-Info.plist`);
  - the app calls `AVSpeechSynthesisProviderVoice.updateSpeechVoices()` (`SystemVoices.update()` in `LaprdusApp.swift`); registering the extension alone is not enough;
  - the app group is one the platform actually grants: `group.com.hrvojekatic.laprdus` on iOS, `<team id>.com.hrvojekatic.laprdus` on macOS. Both come from the build setting `LAPRDUS_APP_GROUP` (per-SDK in the project file), are expanded into the entitlements, and are read back at run time by `AppGroup.identifier`. A `group.` identifier that the macOS provisioning profile does not list is refused by the system.
- To check what the system did with the voices: `/usr/bin/log show --last 10m --predicate 'process == "axassetsd" AND category == "VoiceDB"'` ("Set N records for loader: ausp_lprd_HKTC" is success).
- Settings use the shared app-group `UserDefaults` suite with the **same keys and defaults as the Android DataStore** (`default_voice` (default zvonko), `speed`, `pitch`, `volume`, `force_speed`/`force_pitch`/`force_volume`, `emoji_enabled`, `inflection_level`, `acceleration`, `head_size`, `breathiness`, `roughness`, `brightness`, `*_pause`, `number_mode`, `user_dictionaries_enabled`). The app's Language picker chooses the language of in-app playback only; a system request's language comes from the voice identifier it names. The user pitch slider maps to `laprdus_set_user_pitch`; voice-character pitch comes from the registry via `laprdus_set_voice`.
- User dictionaries are stored in the app group container (`Dictionaries/user.json`, `spelling.json`, `emoji.json`, plus `accents.json` for the formant voices' accent lexicon, edited in the Dictionaries tab as `DictionaryType.accents`) in the **same JSON format as Android**, and are applied both in-app and in the extension. All three files use `grapheme`/`phoneme` entries (that is what every platform's editor writes); the engine's spelling and emoji parsers accept those keys next to the bundled files' `character`/`pronunciation` and `emoji`/`text`.
- One speech request is not one phrase. VoiceOver describes an item as a row of `<voice>` elements (name, value, type), puts `<break>` between some of them, and marks only a part as `<say-as interpret-as="characters">` (a one-character badge count next to a button name). `SSMLParser` therefore returns ordered parts (text, spelled text, pause, phrase boundary), each with its own rate and pitch, and `LaprdusAudioUnit` synthesizes them one by one: a `<break>` is silence of its length, a boundary between `<voice>`/`<p>`/`<s>` elements is the user's comma pause. A spelled part is synthesized without its trailing spelling pause (`laprdus_synthesize_spelled_ex`, reported in `AudioChunk.finalPause`) and owes it to what follows: text with nothing between gets it, a `<break>` or a phrase boundary replaces it (the badge count of "Notifications 1 Tab" is followed by the comma pause alone; before, the spelling pause and the comma pause added up to a gap four times the one before the count), and at the end of the request it is appended, so characters VoiceOver sends one by one keep the spelling cadence. Never decide spelling, rate or pitch for a whole request.
- VoiceOver nests `<prosody>` elements (a neutral outer one, an inner `pitch="+50.0%"` for capital letters); `SSMLParser` treats inner values as relative to the outer ones. The extension logs the markup and the parts of each request (never the text): `log show --info --predicate 'subsystem == "com.hrvojekatic.laprdus"'`.
- Localization: `Localizable.xcstrings` string catalog with en/hr/sr/sl (translations aligned with Android's `values-hr`/`values-sr`/`values-sl`). The built-in demo follows the selected speech language, including Slovenian; once the user edits the text, language changes preserve it. Android does the same with `SpeechSamples`, using the effective speech language (system or forced selection), also for the system's "Listen to an example" text.

### Building

```bash
cd src/platform/apple/Laprdus

# macOS app (signed, automatic provisioning)
xcodebuild -scheme Laprdus -destination 'platform=macOS' build -allowProvisioningUpdates

# iOS Simulator
xcodebuild -scheme Laprdus -destination 'generic/platform=iOS Simulator' build

# Unit tests (engine synthesis, dictionary store, SSML, settings)
xcodebuild test -scheme Laprdus -destination 'platform=macOS'
```

### App icons

`Laprdus/Assets.xcassets/AppIcon.appiconset` contains checked-in opaque RGB PNGs
for iPhone, iPad, the App Store (1024px), and macOS (16–1024px). They are copies
of `images/laprdus_icon_1024x1024.png`, the same source used for Android, without
Android's rounded mask. Regenerate them from the repository root with
`swift scripts/generate_apple_icons.swift` (macOS/Xcode only, no extra packages).
Keep every `Contents.json` slot connected to its PNG. Both build configurations
already select `AppIcon`; Xcode compiles the icons and generates the corresponding
`CFBundleIcons` / `CFBundleIconName` metadata and the macOS `AppIcon.icns`.
When checking distribution builds, inspect the Release archive's compiled icons
and Info.plist, not just the source catalog or a device-thinned Debug build.

### Testing the system voices

After installing/running the app once, the `LaprdusVoices` extension registers with the system. Voices appear under:
- **iOS**: Settings → Accessibility → Spoken Content → Voices
- **macOS**: System Settings → Accessibility → Spoken Content → System voice → Manage Voices

The extension mirrors the Android service behavior: honors host rate/pitch unless the "Force" settings are on, pins volume to 1.0 unless forced, and speaks single grapheme clusters through the spelling dictionary. It always synthesizes with the voice and the language the host's voice identifier names (every voice is listed under hr, sr and bs) — Android's "Force language" setting has no Apple equivalent (Spoken Content always names a concrete voice and language) and is not part of this port.
