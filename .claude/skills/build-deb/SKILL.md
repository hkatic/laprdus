---
name: build-deb
description: Build Debian/Ubuntu .deb packages for LaprdusTTS
allowed-tools: Bash, Read
---

# Build Debian Packages

Build .deb packages for Debian-based Linux distributions (Debian, Ubuntu, Linux Mint, etc.).

## Prerequisites

Install build dependencies:
```bash
sudo apt install -y build-essential debhelper devscripts scons \
    libpulse-dev libasound2-dev libspeechd-dev libglib2.0-dev
```

## Build Steps

The debian directory lives under `installers/linux/deb/debian/` but `dpkg-buildpackage` expects it at the project root alongside `SConstruct`. Use a symlink:

```bash
ln -sfn installers/linux/deb/debian debian && dpkg-buildpackage -us -uc -b -rfakeroot -d; rm -f debian
```

**WSL note:** The build automatically detects WSL and handles NTFS permission issues (777 on all files) by copying package trees to `/tmp` for correct `dpkg-deb` packaging.

## Expected Output

The build creates three packages in the parent directory (`..`):
- `laprdus_1.0.0-1_amd64.deb` - Core library and CLI
- `laprdus-speechd_1.0.0-1_amd64.deb` - Speech Dispatcher module
- `laprdus-dev_1.0.0-1_amd64.deb` - Development headers

After building, move the packages to `installers/linux/`:
```bash
mv ../laprdus_1.0.0-1_amd64.deb ../laprdus-speechd_1.0.0-1_amd64.deb ../laprdus-dev_1.0.0-1_amd64.deb ../laprdus_1.0.0-1_amd64.buildinfo ../laprdus_1.0.0-1_amd64.changes installers/linux/ 2>/dev/null || true
```

## Package Contents

### laprdus
- `/usr/lib/liblaprdus.so*` - Shared library (with ldconfig trigger)
- `/usr/bin/laprdus` - CLI tool
- `/usr/share/laprdus/` - Voice data and dictionaries

### laprdus-speechd
- `/usr/lib/speech-dispatcher-modules/sd_laprdus` - SD module
- `/etc/speech-dispatcher/modules/laprdus.conf` - Configuration
- Auto-configures Speech Dispatcher on install

### laprdus-dev
- `/usr/include/laprdus/` - C API headers

## Verification

```bash
# Check package contents and permissions
dpkg-deb -c installers/linux/laprdus_1.0.0-1_amd64.deb
# Verify control scripts (should show postinst, postrm, triggers)
dpkg-deb -I installers/linux/laprdus_1.0.0-1_amd64.deb
```

## Installation

```bash
sudo dpkg -i installers/linux/laprdus_1.0.0-1_amd64.deb
sudo dpkg -i installers/linux/laprdus-speechd_1.0.0-1_amd64.deb
```

## Test

```bash
laprdus -l  # List voices
laprdus "Dobar dan"  # Speak text
spd-say -o laprdus "Zdravo"  # Via Speech Dispatcher
```
