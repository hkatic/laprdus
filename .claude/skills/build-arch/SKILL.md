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
cd installers/linux/arch && makepkg -sf && cd ../../..
```

**Options:**
- `-s` - Install missing dependencies
- `-f` - Force rebuild even if package exists
- `-i` - Install package after building

## Expected Output (single package)

One package is created in `installers/linux/arch/`:
- `laprdus-1.0.0-1-x86_64.pkg.tar.zst` - Complete package

## Key Design Decisions

- **Single package**: CLI, library, Speech Dispatcher module, voice data, all 3 dictionaries, AND dev headers all in ONE package
- **ldconfig**: Called in `post_install()`, `post_upgrade()`, and `post_remove()` hooks via `laprdus.install`
- **speechd.conf**: Auto-configured in `post_install()`/`post_upgrade()` with BEGIN/END markers, cleaned up in `pre_remove()`
- **speech-dispatcher dependency**: Declared in `depends` array
- **README filename**: Must reference `readme.md` (lowercase), NOT `README.md`

## Package Contents

### laprdus
- `/usr/lib/liblaprdus.so*` - Shared library (versioned)
- `/usr/bin/laprdus` - CLI tool
- `/usr/share/laprdus/` - Voice data and dictionaries
- `/usr/lib/speech-dispatcher-modules/sd_laprdus` - Speech Dispatcher module
- `/etc/speech-dispatcher/modules/laprdus.conf` - Module config
- `/usr/include/laprdus/` - Development headers

## Copy to ~/downloads

```bash
cp installers/linux/arch/laprdus-1.0.0-1-x86_64.pkg.tar.zst ~/downloads/
```

## Test

```bash
sudo pacman -U installers/linux/arch/laprdus-1.0.0-1-x86_64.pkg.tar.zst
laprdus -l  # List voices
laprdus "Dobar dan"  # Speak text
spd-say -o laprdus "Zdravo"  # Via Speech Dispatcher
```

## AUR Submission

The PKGBUILD can be submitted to the Arch User Repository (AUR) for community distribution.
