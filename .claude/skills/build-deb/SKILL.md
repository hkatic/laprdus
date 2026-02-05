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

```bash
cd installers/linux/deb && dpkg-buildpackage -us -uc && cd ../../..
```

## Expected Output

The build creates three packages in `installers/linux/`:
- `laprdus_1.0.0_amd64.deb` - Core library and CLI
- `laprdus-speechd_1.0.0_amd64.deb` - Speech Dispatcher module
- `laprdus-dev_1.0.0_amd64.deb` - Development headers

## Package Contents

### laprdus
- `/usr/lib/liblaprdus.so*` - Shared library
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
ls -la installers/linux/laprdus*.deb
```

## Installation

```bash
sudo dpkg -i installers/linux/laprdus_1.0.0_amd64.deb
sudo dpkg -i installers/linux/laprdus-speechd_1.0.0_amd64.deb
```

## Test

```bash
laprdus -l  # List voices
laprdus "Dobar dan"  # Speak text
spd-say -o laprdus "Zdravo"  # Via Speech Dispatcher
```
