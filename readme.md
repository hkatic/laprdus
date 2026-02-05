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
| `-v, --voice` | Voice (josip, vlado, detence, baba, djedo) |
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
