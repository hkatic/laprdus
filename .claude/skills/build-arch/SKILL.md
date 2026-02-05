---
name: build-arch
description: Build Arch Linux packages using makepkg for LaprdusTTS
allowed-tools: Bash, Read
---

# Build Arch Linux Packages

Build packages for Arch-based Linux distributions (Arch Linux, Manjaro, EndeavourOS).

**Note:** This build must be run on an Arch-based system.

## Prerequisites

Install build dependencies:
```bash
sudo pacman -S --needed base-devel scons libpulse alsa-lib speech-dispatcher glib2
```

## Build Steps

```bash
cd installers/linux/arch && makepkg -s && cd ../../..
```

**Options:**
- `-s` - Install missing dependencies
- `-i` - Install package after building
- `-f` - Force rebuild even if package exists

## Expected Output

Two packages are created in `installers/linux/arch/`:
- `laprdus-1.0.0-1-x86_64.pkg.tar.zst` - Core package
- `laprdus-speechd-1.0.0-1-x86_64.pkg.tar.zst` - Speech Dispatcher module

## Package Contents

### laprdus
- `/usr/lib/liblaprdus.so*` - Shared library
- `/usr/bin/laprdus` - CLI tool
- `/usr/share/laprdus/` - Voice data and dictionaries
- `/usr/include/laprdus/` - Development headers

### laprdus-speechd
- `/usr/lib/speech-dispatcher-modules/sd_laprdus` - SD module
- `/etc/speech-dispatcher/modules/laprdus.conf` - Configuration
- Post-install hook auto-configures Speech Dispatcher

## Verification

```bash
ls -la installers/linux/arch/laprdus*.pkg.tar.zst
```

## Installation

```bash
sudo pacman -U installers/linux/arch/laprdus-1.0.0-1-x86_64.pkg.tar.zst
sudo pacman -U installers/linux/arch/laprdus-speechd-1.0.0-1-x86_64.pkg.tar.zst
```

## Test

```bash
laprdus -l  # List voices
laprdus "Dobar dan"  # Speak text
spd-say -o laprdus "Zdravo"  # Via Speech Dispatcher
```

## AUR Submission

The PKGBUILD can be submitted to the Arch User Repository (AUR) for community distribution.
