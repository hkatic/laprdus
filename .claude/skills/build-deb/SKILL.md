---
name: build-deb
description: Build Debian/Ubuntu .deb packages for LaprdusTTS
allowed-tools: Bash, Read
---

# Build Debian Packages

Build .deb packages for Debian-based Linux distributions (Debian, Ubuntu, Linux Mint, etc.).

**Note:** This build must be run on a Debian/Ubuntu system (native or WSL).

## Prerequisites

Install build dependencies:
```bash
sudo apt install -y build-essential debhelper devscripts scons \
    libpulse-dev libasound2-dev libspeechd-dev libglib2.0-dev
```

## Build Steps

The debian directory lives under `installers/linux/deb/debian/` but `dpkg-buildpackage` expects it at the project root alongside `SConstruct`. Use a symlink:

```bash
cd /home/hrvojekatic/.repo/hkatic/laprdus
ln -sfn installers/linux/deb/debian debian && dpkg-buildpackage -us -uc -b -rfakeroot -d; rm -f debian
```

**WSL note:** The build automatically detects WSL and handles NTFS permission issues (777 on all files) by copying package trees to `/tmp` for correct `dpkg-deb` packaging.

## Expected Output (single package + dev)

The build creates packages in the parent directory (`..`):
- `laprdus_1.0.0-1_amd64.deb` - Main package (CLI + library + Speech Dispatcher module + voice data + dictionaries)
- `laprdus-dev_1.0.0-1_amd64.deb` - Development headers

## Key Design Decisions

- **Single package**: CLI, library, Speech Dispatcher module, voice data, and all 3 dictionaries (internal.json, spelling.json, emoji.json) are in ONE `laprdus` package
- **Multiarch library path**: Library installs to `/usr/lib/$(DEB_HOST_MULTIARCH)/` for proper Debian multiarch support
- **ldconfig**: Handled automatically by debhelper triggers when library is in multiarch path
- **speechd.conf**: Auto-configured in `postinst` with BEGIN/END markers, cleaned up in `postrm`. Detects SD 0.12+ autodetection mode (no uncommented AddModule lines) and only adds LanguageDefaultModule entries to avoid disabling autodetection for other modules (espeak-ng, RHVoice, etc.)
- **Speech Dispatcher restart**: Uses `try-restart` for both system and user services (SD runs as user service on modern distros). Iterates logged-in users via `loginctl list-users`
- **speech-dispatcher dependency**: Declared in `Depends:` field

## Package Contents

### laprdus
- `/usr/lib/<multiarch>/liblaprdus.so*` - Shared library
- `/usr/bin/laprdus` - CLI tool
- `/usr/share/laprdus/` - Voice data (Josip.bin, Vlado.bin) and dictionaries
- `/usr/lib/speech-dispatcher-modules/sd_laprdus` - Speech Dispatcher module
- `/etc/speech-dispatcher/modules/laprdus.conf` - Module config

### laprdus-dev
- `/usr/include/laprdus/` - C API headers

## Copy to ~/Downloads

```bash
mv ../laprdus_1.0.0-1_amd64.deb ../laprdus-dev_1.0.0-1_amd64.deb ~/Downloads/
```

## Test

```bash
sudo dpkg -i laprdus_1.0.0-1_amd64.deb
laprdus -l  # List voices
laprdus "Dobar dan"  # Speak text
spd-say -o laprdus "Zdravo"  # Via Speech Dispatcher
```
