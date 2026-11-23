---
paths:
  - "src/platform/apple/**"
  - "installers/macos/**"
  - "scripts/build-macos-release.sh"
  - "scripts/generate_apple_icons.swift"
---
# macOS and Apple

Moved from `CLAUDE.md` on 2026-11-23, unchanged. Claude Code loads this file when Claude reads a file matching the paths above; `CLAUDE.md` lists every topic file.

## macOS (native library and CLI)

Separate from the Apple app in `src/platform/apple/` (which builds with Xcode), the
Cargo build supports macOS as a first-class platform for the C API library and the CLI.

**First built on a Mac since the migration to Rust on 2026-11-18**: `scripts/build-macos-release.sh`
ran end to end (universal CLI and library, app archive, three notarizations, `spctl` accepts
the app and the DMG), and the tarball's CLI speaks Zvonko, Josip and Vlado. The app's voices
have not yet been tried with VoiceOver.

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
tarball alike without a run path. All of this ran on a Mac for the first time since the
migration on 2026-11-18 (see above).

**Not built on macOS:** SAPI5, the NVDA addon, the Windows config GUI and the
Speech Dispatcher module are all platform-specific and remain Windows/Linux only.
`./scripts/build-all.sh all` detects the host and skips what it cannot build,
warning rather than failing.

## Apple (iOS / iPadOS / macOS)

### Overview

The Apple port lives in `src/platform/apple/Laprdus/Laprdus.xcodeproj` (Xcode 26+; the project moved here from a folder of its own at the root with the migration to Rust) and mirrors the Android app. **First built after the migration on 2026-11-18** (iOS device Release build, installed on the iPhone; macOS archive of the release script): the paths, the script phase and the build settings below work; the voices have not yet been tried with VoiceOver since. Deployment targets: **iOS 16.0** and **macOS 13.0** — the minimum versions supporting `AVSpeechSynthesisProviderAudioUnit`; the SwiftUI code deliberately uses only iOS 16 / macOS 13 APIs (`ObservableObject`/`@Published`, single-parameter `onChange`, no `@Observable`/`@Bindable`, no `navigationDestination(item:)`). visionOS is not supported.

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

- The engine is the Rust static library `liblaprdus_capi.a` (crate `laprdus-capi`, the C API). Both targets have a "Build the Rust engine" script phase that runs `src/platform/apple/build-rust-engine.sh`: it builds the crate with Cargo for the platform and the architectures Xcode is building (always `--release`, also for Debug builds of the app: an unoptimized synthesizer is too slow for a screen reader), joins the architectures and puts the library into the target's own derived files folder, where that target links it (`LIBRARY_SEARCH_PATHS = $(DERIVED_FILE_DIR)`, `OTHER_LDFLAGS = -llaprdus_capi`; one copy in the shared products folder, as it was at first, is produced by two tasks, and Xcode refuses the build with "Multiple commands produce"). Swift reaches it through the bridging header `Shared/LaprdusC.h`. It needs rustup with the targets of the platforms built: `rustup target add aarch64-apple-darwin x86_64-apple-darwin aarch64-apple-ios aarch64-apple-ios-sim x86_64-apple-ios`. Built and linked this way for the first time on 2026-11-18.
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
