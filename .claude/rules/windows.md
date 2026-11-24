---
paths:
  - "src/platform/windows/**"
  - "installers/windows/**"
  - "tests/windows/**"
  - "tests/nvda/**"
  - "scripts/build-all.cmd"
---
# Windows: legacy build, installer, Configurator languages, NVDA add-on

Moved from `CLAUDE.md` on 2026-11-23, unchanged. Claude Code loads this file when Claude reads a file matching the paths above; `CLAUDE.md` lists every topic file.

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
- **NVDA on old Windows:** NVDA 2023.3.4 is the last for Windows 7 SP1 and 8 (Python 3.7, 32-bit), NVDA 2025.3.3 the last for Windows 8.1 and for 32 bits (Python 3.11), NVDA 2026.1 and later are 64-bit on Windows 10 and later (Python 3.13). The add-on's files must therefore stay Python 3.7 (`python tests/nvda/test_python37.py`); its manifest (`minimumNVDAVersion` 2019.3) admits all of them. NVDA before 2024.2 has no `speech.extensions.filter_speechSequence` and NVDA before 2024.4 ignores the manifest's `symbolDictionaries` section, so on Windows 7 and 8 the global plugin puts its speech filter in front of NVDA's `processText()` and loads the Slovenian symbol dictionary into NVDA's own symbol data itself (since 2026-11-24, a tester's report from Windows 7: dates in Croatian, not in Slovenian; `.claude/rules/numbers-dates.md`, "NVDA before 2024.2 and 2024.4").
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
- **Every one of these inputs is required** (since 2026-11-23): each is a source of the add-on's target, so a missing `dist/` or `data/` file fails `scons` ("Source ... not found, needed by target ...", after a hint naming the step that makes it) instead of packing an older copy left in the add-on's folder, as it did until then (the packer zips whatever is under `addon/`). `scons pot` and `scons -c` need none of them. `scons legacy=no` builds an add-on without the build for Windows 7 SP1, 8 and 8.1 and deletes stale legacy copies before packing; CI and both `build-all` scripts build the legacy programs first, so they use the default.

**The global plugin's speech filter**: `globalPlugins/laprdus/__init__.py` registers `_speechFilter.filter_sequence()` with `speech.extensions.filter_speechSequence` (NVDA 2024.2+; in NVDA 2019.3 to 2024.1 `_speechFilter.ProcessTextHook` puts the same rules in front of the `processText()` of NVDA's speech module), which runs before NVDA's symbol processing whenever Laprdus speaks: through the add-on's driver or through NVDA's SAPI5 driver (`sapi5`, `sapi5_32`) with a Laprdus voice. It keeps the dates and the periods between digits that NVDA's Slovenian table would drop (`.claude/rules/numbers-dates.md`, "A date in every written form"). It was the synth driver's until 2026-11-23, so the SAPI5 voices under NVDA did not have it (a tester's report: dates in Croatian, not in Slovenian). Test without NVDA or a build: `python tests/nvda/test_global_plugin.py`. The add-on's mandatory Slovenian symbol dictionary (`locale/sl/symbols-laprdus.dic`) corrects NVDA's Slovenian table for every synthesizer: « and » at the level "most", `-` and `—` preserved as in the hr, sr, bs and en tables. NVDA 2024.4 and later load it from the manifest; before that the global plugin loads it into NVDA's data (`_symbols.SymbolDictionaryHook`).

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

## Testing

### SAPI5 Voices

After installing SAPI5, test with PowerShell:
```powershell
Add-Type -AssemblyName System.Speech
$synth = New-Object System.Speech.Synthesis.SpeechSynthesizer
$synth.SelectVoice("Laprdus Croatian")   # or "Laprdus Serbian" / "Laprdus Bosnian"; the voice is settings.json voice.default
$synth.Speak("Dobar dan!")
```

SAPI5 registers four voice tokens, one per language ("Laprdus Croatian" 0x041A, "Laprdus Serbian" 0x081A, "Laprdus Bosnian" 0x141A, "Laprdus Slovenian" 0x0424), not one per Laprdus voice. SAPI switches between them by itself on a `<lang langid>` tag (NVDA's automatic language switching writes one): the Slovenian token with `<lang langid="41a">` speaks the Croatian token's samples (checked 2026-11-23). The token sets the language; the voice comes from `settings.json` `voice.default` (empty = zvonko, old ids accepted), chosen in the Laprdus Configurator. There is no language key in `settings.json`. The installer removes the per-voice tokens of earlier versions on install, upgrade and repair, and re-registers the four.

**The driver's setup is every platform's** (since 2026-11-23): `Voice::new()` in `src/platform/windows/sapi5/src/lib.rs` loads the bundled dictionaries from the install directory and then calls `Engine::apply_user_config()`, the function `laprdus_load_user_config()` and Speech Dispatcher use: every setting of `settings.json` with the sentence, comma and newline pauses (the driver applied it with `set_params()` alone, which leaves the clause pauses alone, so the Configurator's pause settings were ignored by SAPI5 from the C++ version on) and the user's dictionaries on top of the bundled ones. A change to any of `settings.json`, `user.json`, `spelling.json`, `emoji.json`, `accents.json` (`UserConfig::modified_times()`) makes the engine again at the next `Speak`; it was `settings.json` alone. **Prosody per fragment** (SAPI SDK, `SPVSTATE`): a text run ends where a fragment's `RateAdj`, `Volume` or `PitchAdj.MiddleAdj` changes (a fragment of blanks never ends one), and each run gets speed = setting × 2^(clamp(`GetRate()` + `RateAdj`, -10, 10)/10), volume = `GetVolume()`/100 × `Volume`/100, user pitch = setting × 2^(`MiddleAdj`/24), each ignored under its force switch; the driver used to take the first fragment's pitch for the whole call (a leading bookmark's, when NVDA wrote one before its first `<pitch>`) and ignored the XML's rate and volume, so NVDA's capital pitch change raised a whole spelled word or nothing. With one prosody throughout, the audio is sample for sample what it was (`python tests/windows/test_sapi5_driver.py --hashes <old dll>` and `--hashes <new dll>` print the same).

### NVDA Addon

1. Install the `.nvda-addon` file
2. Restart NVDA
3. Select "Laprdus" in NVDA's synthesizer settings; the Language setting, which comes before the Voice setting, chooses hr, sr, bs or sl, and the Voice list holds all 18 voices (default Zvonko) for any of them
