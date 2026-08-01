# Laprdus

A Retro-type speech synthesizer for Croatian and Serbian languages using concatenative synthesis.

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

## About

Laprdus is a text-to-speech (TTS) synthesizer for Croatian and Serbian languages. It uses concatenative synthesis technology, joining pre-recorded phoneme units to produce speech output. While not matching modern neural TTS quality, Laprdus offers high performance and minimal memory usage.

Laprdus was developed to provide screen reader users with simple and fast access to computers and mobile devices in their native language, for free.

Laprdus, started as my personal hobby project, is also meant to be experimental, allowing me to see how far I can go with it.

## Who is Laprdus for?

Laprdus is not intended for everyone. It's a good choice for those who enjoy the nostalgic sound of retro speech synthesizers from the 1980s and 1990s.

It may not suit users accustomed to the high voice quality of modern synthesizers that use AI and neural networks. However, Laprdus could be ideal for those who prioritize performance and minimal resource consumption over voice quality.

In short: modern synthesizers offer quality at the cost of resources; Laprdus offers performance at the cost of quality.

## Supported Platforms

- **Windows 7-11** - via Microsoft SAPI5 standard, works with Narrator, NVDA, JAWS, and other SAPI5-compatible applications
- **[NVDA](https://github.com/nvaccess/nvda) Screen Reader** - dedicated addon for the free and open-source NVDA screen reader
- **Linux** - via Speech Dispatcher for Orca screen reader, plus command-line interface
- **Android** - built-in TTS engine for Android devices
- **Apple (iOS, iPadOS, macOS, visionOS)** - SwiftUI app with a speech synthesis provider extension that makes Laprdus voices available system-wide to VoiceOver and Spoken Content

## Features

- Croatian and Serbian speech synthesis (Latin and Cyrillic scripts)
- Five voices (two physical, three derived)
- Adjustable speech rate, pitch, and volume
- Natural inflection based on punctuation
- Number reading as words (up to centillions) or digit-by-digit
- Configurable pause durations for punctuation marks
- Custom pronunciation dictionaries for words, symbols, and emoji

## Voices

| Voice | Type | Language | Description |
|-------|------|----------|-------------|
| **Josip** | Physical | Croatian | Adult male, default Croatian voice |
| **Vlado** | Physical | Serbian | Adult male, default Serbian voice |
| **Detence** | Derived | Croatian | Child voice (higher pitch) |
| **Baba** | Derived | Croatian | Grandmother voice (slightly higher pitch) |
| **Djedo** | Derived | Serbian | Grandfather voice (lower pitch) |

## Installation

### Windows SAPI5

1. Download `Laprdus_SAPI5_Setup.exe` from the [releases page](https://github.com/hkatic/laprdus/releases)
2. Run the installer (recommended: Run as Administrator)
3. Laprdus voices will appear in Windows TTS settings

### NVDA Addon

1. Download `laprdus-*.nvda-addon` from the [releases page](https://github.com/hkatic/laprdus/releases)
2. Double-click the file to install
3. Restart NVDA when prompted
4. Select Laprdus in NVDA Settings > Speech

### Linux

**Debian/Ubuntu:**
```bash
sudo dpkg -i laprdus_amd64.deb
systemctl --user restart speech-dispatcher
```

**Fedora:**
```bash
sudo rpm -i laprdus.x86_64.rpm
```

**Arch Linux:**
```bash
makepkg -si
```

**From tarball:**
```bash
tar xf laprdus-linux-x86_64.tar.xz
cd laprdus-linux-x86_64
sudo ./install.sh
```

### Android

1. Download and install the APK file
2. Go to Settings > Accessibility > Text-to-speech
3. Select "Laprdus TTS" as the preferred engine

### Apple (iOS, iPadOS, macOS, visionOS)

Laprdus for Apple platforms is currently distributed as source code only — it is not yet on the App Store or TestFlight. Build and install it with Xcode as described in [Apple (iOS / iPadOS / macOS / visionOS)](#apple-ios--ipados--macos--visionos) below.

After the app has been launched once, the Laprdus voices become available system-wide:

- **iOS/iPadOS**: Settings > Accessibility > Spoken Content > Voices > Others
- **macOS**: System Settings > Accessibility > Spoken Content > System voice > Manage Voices
- VoiceOver users can select the voices in the VoiceOver speech settings

## Command Line Usage

```bash
# Speak text
laprdus "Dobar dan!"

# Use different voice
laprdus -v vlado "Zdravo svete!"

# Adjust rate and pitch
laprdus -r 1.5 -p 1.2 "Brži i viši govor"

# Output to WAV file
laprdus -o output.wav "Text to save"

# Read from file
laprdus -i document.txt

# List available voices
laprdus -l
```

### CLI Options

| Option | Description |
|--------|-------------|
| `-v, --voice` | Voice (josip, vlado, detence, baba, djed) |
| `-r, --speech-rate` | Speech rate (0.5-2.0, default: 1.0) |
| `-p, --speech-pitch` | Speech pitch (0.5-2.0, default: 1.0) |
| `-V, --speech-volume` | Volume (0.0-1.0, default: 1.0) |
| `-d, --numbers-digits` | Read numbers digit-by-digit |
| `-c, --comma-pauses` | Comma pause duration in ms |
| `-e, --period-pauses` | Period pause duration in ms |
| `-o, --output-file` | Output to WAV file |
| `-i, --input-file` | Read text from file |
| `-l, --list-voices` | List available voices |
| `-h, --help` | Show help |

## Configuration and Dictionaries

Laprdus stores user settings and custom dictionaries in platform-specific locations:

| Platform | Location |
|----------|----------|
| Windows | `%APPDATA%\Laprdus\` |
| Linux | `~/.config/Laprdus/` |
| Android | Managed through the app interface |
| Apple | Managed through the app interface (stored in the app group container, shared with the speech extension) |

### Configuration Files

| File | Purpose |
|------|---------|
| `settings.json` | User preferences (voice, rate, pitch, volume, pauses, etc.) |
| `user.json` | Custom pronunciation dictionary for words and phrases |
| `spelling.json` | Character pronunciations for spelling mode |
| `emoji.json` | Emoji-to-text mappings for emoji reading |

### Dictionary Format

Dictionaries use JSON format. Example `user.json` entry:

```json
{
    "version": "1.0",
    "entries": [
        {
            "grapheme": "GitHub",
            "phoneme": "Githab",
            "caseSensitive": false,
            "wholeWord": true
        }
    ]
}
```

For detailed dictionary documentation, see the [User Guide (Croatian language only)](docs/laprdus.md#5-rječnici).

## Building from Source

### Prerequisites

| Requirement | Purpose |
|-------------|---------|
| C++17 compiler | MSVC 2019+ (Windows), GCC 9+ (Linux) |
| SCons | Build system (`pip install scons`) |
| Python 3.9+ | NVDA addon build |
| InnoSetup 6+ | Windows installer |
| Android SDK/NDK | Android build |
| libpulse-dev, libasound2-dev | Linux audio |
| libspeechd-dev, libglib2.0-dev | Linux Speech Dispatcher |
| Xcode 26+ (on macOS) | Apple build (iOS, iPadOS, macOS, visionOS) |

### Voice Data Generation

Laprdus voices are stored as packed binary files (`.bin`) containing pre-recorded phoneme WAV samples. The build system automatically generates these from source phoneme recordings.

**Source structure:**
```
phonemes/
├── Josip/          # Croatian voice phonemes (WAV files)
│   ├── A.wav
│   ├── B.wav
│   └── ...
└── Vlado/          # Serbian voice phonemes (WAV files)
    ├── A.wav
    ├── B.wav
    └── ...
```

**Generate voice data:**
```bash
scons --platform=windows --arch=x64 --build-config=release voice-data
```

**Output:**
- `data/voices/Josip.bin` - Packed Croatian voice data
- `data/voices/Vlado.bin` - Packed Serbian voice data

The build system uses the `phoneme_packer` tool to combine individual WAV files into optimized binary packages. All platform builds (SAPI5, NVDA, Linux, Android) automatically source voice data from `data/voices/`.

**Note:** Voice data generation runs automatically when building platform targets. Manual generation is only needed when modifying phoneme recordings.

### Quick Build (Recommended)

Use the master build script which handles all dependencies automatically:

**Linux/macOS/WSL:**
```bash
./scripts/build-all.sh all          # Build all platforms
./scripts/build-all.sh sapi5        # Windows SAPI5 + installer
./scripts/build-all.sh nvda         # NVDA addon
./scripts/build-all.sh linux        # Linux components
./scripts/build-all.sh android      # Android APK
./scripts/build-all.sh voice-data   # Generate voice data only
```

**Windows (CMD or PowerShell):**
```cmd
scripts\build-all.cmd all           # Build all platforms
scripts\build-all.cmd sapi5         # Windows SAPI5 + installer
scripts\build-all.cmd nvda          # NVDA addon
scripts\build-all.cmd android       # Android APK
scripts\build-all.cmd voice-data    # Generate voice data only
```

> **Note:** Windows users should use the `.cmd` scripts. The `.sh` scripts require a Unix shell (Linux, macOS, or WSL).

### Manual Build Commands

#### Windows SAPI5

```bash
# Build DLLs for both architectures
scons --platform=windows --arch=x64 --build-config=release sapi5
scons --platform=windows --arch=x86 --build-config=release sapi5

# Build installer (requires InnoSetup)
iscc installers/windows/laprdus_sapi5.iss
```

Output: `installers/windows/Output/Laprdus_SAPI5_Setup_1.0.0.exe`

#### Windows CLI

```bash
scons --platform=windows --arch=x64 --build-config=release cli
```

Output: `build/windows-x64-release/laprdus.exe`

#### NVDA Addon

```bash
# Build prerequisites first
scons --platform=windows --arch=x64 --build-config=release sapi5
scons --platform=windows --arch=x86 --build-config=release sapi5
scons --platform=windows --arch=x64 --build-config=release config
scons --platform=windows --arch=x86 --build-config=release config

# Build addon (automatically copies DLLs, voice data, dictionaries)
cd nvda-addon && scons
```

Output: `nvda-addon/laprdus-*.nvda-addon`

**Important:** Always use SCons to build the NVDA addon. Never manually create the archive or edit generated files like `manifest.ini`.

#### Linux

```bash
# Install dependencies (Debian/Ubuntu)
sudo apt install libpulse-dev libasound2-dev libspeechd-dev libglib2.0-dev

# Build all Linux components
scons --platform=linux --build-config=release linux-all

# Or build individual targets
scons --platform=linux --build-config=release cli      # CLI only
scons --platform=linux --build-config=release speechd  # Speech Dispatcher module
```

Output:
- `build/linux-x64-release/liblaprdus.so`
- `build/linux-x64-release/laprdus`
- `build/linux-x64-release/sd_laprdus`

#### Android

```bash
cd android
./gradlew assembleDebug    # Debug build
./gradlew assembleRelease  # Release build
```

Output: `android/app/build/outputs/apk/*/app-*.apk`

#### Apple (iOS / iPadOS / macOS / visionOS)

The Apple port lives in `Lapplerdus/Laprdus/Laprdus.xcodeproj` and contains three targets:

| Target | Purpose |
|--------|---------|
| `Laprdus` | Multiplatform SwiftUI app (main screen, settings, dictionaries, about) |
| `LaprdusVoices` | Speech synthesis provider extension — registers the voices with the system for VoiceOver/Spoken Content |
| `LaprdusTests` | Unit tests |

##### Prerequisites

1. A Mac with **Xcode 26 or newer**. For visionOS, also install the visionOS platform in Xcode > Settings > Components.
2. An **Apple ID** signed into Xcode (Xcode > Settings > Accounts). A free account is enough for development on your own devices; a paid Apple Developer Program membership is only needed for App Store/TestFlight distribution and removes the 7-day provisioning expiry of free accounts.
3. **Voice data**. The packed voice files (`data/voices/*.bin`) are generated, not checked in. On a fresh checkout, generate them once from the repository root:

   ```bash
   pip install scons
   scons --platform=linux --arch=x64 --build-config=release voice-data
   # or: ./scripts/build-all.sh voice-data
   ```

   The `linux` platform configuration compiles fine on macOS with clang; it is only used to build the `phoneme_packer` tool that packs the voices. The Xcode project references `data/voices/Josip.bin`, `data/voices/Vlado.bin` and the dictionaries in `data/dictionary/` directly, so this step must happen before the first build.

##### Code signing setup (first time only)

1. Open `Lapplerdus/Laprdus/Laprdus.xcodeproj` in Xcode.
2. Select the project in the navigator, then for **each** of the `Laprdus` and `LaprdusVoices` targets, open **Signing & Capabilities** and pick your team under **Team**. Signing is automatic.
3. If Xcode reports that the bundle identifier is already in use (it is registered to the original author), change the bundle identifiers to your own reverse-DNS prefix. Keep the pattern: the extension identifier must be prefixed by the app identifier (e.g. `com.example.Laprdus` and `com.example.Laprdus.voices`).
4. The app and the extension share settings and user dictionaries through the app group `group.com.hrvojekatic.laprdus`. If you changed the bundle identifiers, also change the app group identifier in both targets' Signing & Capabilities and in `Shared/AppGroup.swift` so they match. The app still works without a valid app group — it falls back to local storage — but then the extension cannot see settings and dictionaries saved by the app.

##### Running on a Mac

1. In Xcode's run destination chooser (toolbar, next to the scheme), select **My Mac**.
2. Press **Run** (Cmd+R). The app launches with the debugger attached.
3. Or from the command line:

   ```bash
   cd Lapplerdus/Laprdus
   xcodebuild -scheme Laprdus -destination 'platform=macOS' build -allowProvisioningUpdates
   ```

4. On the first launch macOS may ask you to confirm opening an app from an identified developer — allow it in System Settings > Privacy & Security if prompted.
5. After the first launch, the voices appear in System Settings > Accessibility > Spoken Content > System voice > Manage Voices, and in VoiceOver's voice settings.

##### Preparing a physical iPhone or iPad

1. **Pair the device with Xcode first**: connect the iPhone/iPad to the Mac with a cable (the first pairing is much more reliable over USB than over Wi-Fi). Unlock the device, tap **Trust This Computer** and enter the passcode. In Xcode, open Window > Devices and Simulators and wait until the device shows as ready — the first-time preparation copies debug symbols and can take several minutes.
2. **Enable Developer Mode on the device** (required since iOS 16): Settings > Privacy & Security > Developer Mode > on. The device restarts and asks you to confirm.

   **If the Developer Mode item is not visible** in Settings > Privacy & Security: iOS hides it until the device has been paired with Xcode or has received a development-signed app. Complete step 1 first, then check again. If it still does not appear:
   - try pressing **Run** in Xcode once anyway — the install attempt itself usually makes the option appear, or
   - reboot the device while it is still connected to the Mac, or
   - run `xcrun devmodectl streaming` on the Mac with the device plugged in — it prompts the device to offer Developer Mode.
3. Optional — **wireless debugging**: in Window > Devices and Simulators, select the device and check **Connect via network**. After that, the cable is only needed for the initial pairing.
4. **Select the right scheme and destination**: in the Xcode toolbar, make sure the **Laprdus** scheme is selected (Product > Scheme > Laprdus), then select your device as the run destination next to it, and press **Run** (Cmd+R).

   > **Note:** If Xcode asks you to "choose an app to run", the **LaprdusVoices** scheme is selected instead of **Laprdus**. LaprdusVoices is an app extension, which cannot run on its own, so Xcode asks for a host app. Switch to the Laprdus scheme (that dialog is only useful when deliberately debugging the extension — see [Debugging](#debugging)).
5. **Free (personal team) accounts only**: the first install fails to launch until you trust the developer certificate on the device: Settings > General > VPN & Device Management > select your developer certificate > Trust. Then launch again. Apps signed with a free account expire after 7 days — just run from Xcode again to refresh.
6. After the first launch of the app, the LaprdusVoices extension registers with the system. Enable the voices: Settings > Accessibility > Spoken Content > Voices > Others > Laprdus. VoiceOver users: VoiceOver Settings > Speech > Voice.

Command-line build for a connected device:

```bash
cd Lapplerdus/Laprdus
xcodebuild -scheme Laprdus -destination 'generic/platform=iOS' build -allowProvisioningUpdates
```

##### Preparing an Apple Vision Pro

1. Install the visionOS platform in Xcode > Settings > Components.
2. On the headset, enable Developer Mode: Settings > Privacy & Security > Developer Mode.
3. Pair over Wi-Fi: both devices on the same network, then Xcode > Window > Devices and Simulators > Discovered, select the Vision Pro and enter the pairing code shown in the headset.
4. Select the Vision Pro as the run destination and press **Run**.

##### Debugging

- **App**: run from Xcode (Cmd+R); breakpoints, the console, and the memory/thread debugger work as usual. The C++ engine sources (`src/core`, `src/audio`, `src/c_api`) are part of the project, so you can set breakpoints in C++ files too — the Swift and C++ code run in the same process.
- **Speech extension**: the `LaprdusVoices` extension runs in its own process, started on demand by the system. To debug it, use the `LaprdusVoices` scheme (Run > it asks for a host app — choose Settings on iOS or any speech client on macOS), or attach to the running process via Debug > Attach to Process when a client (e.g. VoiceOver or Spoken Content) is using a Laprdus voice.
- **Logs**: the extension's `os_log`/`print` output is visible in Console.app — filter by process `LaprdusVoices` (on iOS, select the device in Console.app's sidebar).
- **Unit tests**: Product > Test in Xcode, or:

  ```bash
  cd Lapplerdus/Laprdus
  xcodebuild test -scheme Laprdus -destination 'platform=macOS'
  ```

  The tests exercise real synthesis through the C++ engine, the dictionary store, and the settings store.
- **After changing the engine or voices**: if you change a voice's phoneme recordings, regenerate voice data (see prerequisites) and rebuild — the `.bin` files are copied into the app and extension bundles at build time. C++ engine changes are picked up automatically since the sources are compiled directly into both targets.

##### Testing checklist for Apple

- [ ] App builds and launches on macOS and on a physical iPhone/iPad
- [ ] Speak/Stop works on the main screen with each of the 5 voices
- [ ] Settings changes (rate, pitch, volume, pauses) are audible in the app
- [ ] Dictionary entries affect pronunciation
- [ ] Laprdus voices appear in the system voice list after the first app launch
- [ ] VoiceOver can read with a Laprdus voice, including character-by-character (spelling) navigation
- [ ] UI is fully usable with VoiceOver on iOS and macOS

### Build Output Locations

| Platform | Output |
|----------|--------|
| Windows SAPI5 x64 | `build/windows-x64-release/laprd64.dll` |
| Windows SAPI5 x86 | `build/windows-x86-release/laprd32.dll` |
| Windows Installer | `installers/windows/Output/Laprdus_SAPI5_Setup_1.0.0.exe` |
| Windows CLI | `build/windows-x64-release/laprdus.exe` |
| NVDA Addon | `nvda-addon/laprdus-*.nvda-addon` |
| Linux Library | `build/linux-x64-release/liblaprdus.so` |
| Linux CLI | `build/linux-x64-release/laprdus` |
| Linux Speech Dispatcher | `build/linux-x64-release/sd_laprdus` |
| Android APK | `android/app/build/outputs/apk/*/app-*.apk` |
| Apple app + extension | Xcode DerivedData (`Laprdus.app` with embedded `LaprdusVoices.appex`); use Product > Archive for distributable builds |

### Clean Build

```bash
scons -c                                    # Clean all
scons -c --platform=windows --arch=x64      # Clean specific config
```

## Documentation

- **[User Guide (Croatian)](docs/laprdus.md)** - Complete user manual
- **[Developer Documentation](docs/dev.md)** - Technical documentation for contributors
- **[CLAUDE.md](CLAUDE.md)** - AI assistant context and build reference

## License

GNU General Public License v3.0 - see [LICENSE](LICENSE) for details.

## Author

**Hrvoje Katić**

- **Email**: [hrvojekatic@gmail.com](mailto:hrvojekatic@gmail.com)
- **Website**: https://hrvojekatic.com/laprdus (alternative download location)

## Links

- **Source Code**: https://github.com/hkatic/laprdus
- **Releases**: https://github.com/hkatic/laprdus/releases

## Contributing

Contributions are welcome. Please read [CLAUDE.md](CLAUDE.md) for technical details before submitting pull requests.
