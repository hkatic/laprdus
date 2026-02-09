---
name: build-tarball
description: Build generic Linux tarball with install script for LaprdusTTS (source and binary)
allowed-tools: Bash, Read
argument-hint: "[version]"
---

# Build Generic Linux Tarball

Build a portable tarball package that works on any Linux distribution. Includes compiled binaries and install/uninstall scripts.

## Build Steps

```bash
cd installers/linux/tarball && ./build-tarball.sh 1.0.0 && cd ../../..
```

**Arguments:**
- `$ARGUMENTS[0]` or default: Version number (e.g., "1.0.0")

## Expected Output

- `installers/linux/tarball/laprdus-1.0.0-linux-x86_64.tar.xz`

## Tarball Contents

```
laprdus-1.0.0-linux-x86_64/
├── bin/laprdus              # CLI tool
├── lib/
│   ├── liblaprdus.so        # Symlink → .so.1
│   ├── liblaprdus.so.1      # Symlink → .so.1.0.0
│   └── liblaprdus.so.1.0.0  # Library (with SONAME)
├── lib/speech-dispatcher-modules/
│   └── sd_laprdus           # Speech Dispatcher module
├── share/laprdus/
│   ├── Josip.bin            # Voice data
│   ├── Vlado.bin
│   ├── internal.json        # Pronunciation dictionary
│   ├── spelling.json        # Spelling dictionary
│   └── emoji.json           # Emoji dictionary
├── etc/speech-dispatcher/modules/
│   └── laprdus.conf         # Module config
├── include/laprdus/         # Headers
├── share/doc/laprdus/       # Documentation
├── install.sh               # Installation script
├── uninstall.sh             # Convenience removal script
└── README                   # Usage instructions
```

## Key Design Decisions

- **Single package**: Everything in one tarball
- **Speech Dispatcher module**: Install script detects system speechd module dir via `pkg-config --variable=modulebindir speech-dispatcher` and installs `sd_laprdus` THERE (not under the prefix). This is critical — SD only looks in its own module dir (e.g., `/usr/lib64/speech-dispatcher-modules/`)
- **ldconfig**: For non-standard prefixes (e.g., `/usr/local`), install script creates `/etc/ld.so.conf.d/laprdus.conf` so the library is discoverable
- **Persistent uninstall**: After install, uninstall script is saved to `<prefix>/share/laprdus/uninstall.sh` so it survives tarball directory deletion
- **Multiarch library paths**: Install script detects Debian/Ubuntu multiarch via `dpkg-architecture -qDEB_HOST_MULTIARCH`, Fedora `/usr/lib64/`, or falls back to `<prefix>/lib`
- **speechd.conf**: Auto-configured with BEGIN/END markers. Detects SD 0.12+ autodetection mode (no uncommented AddModule lines) and only adds LanguageDefaultModule entries to avoid disabling autodetection for other modules
- **Speech Dispatcher restart**: Uses `try-restart` for both system and user services, iterates logged-in users via `loginctl list-users`. On removal, also uses `killall speech-dispatcher` as fallback to ensure SD is fully stopped (socket activation restarts it clean)
- **Data dir auto-detection**: Both CLI and speechd module check `/usr/share/laprdus` and `/usr/local/share/laprdus`
- **Library symlinks**: Use `cp -Pv` (preserve symlinks) when copying library files

## Copy to ~/Downloads

```bash
cp installers/linux/tarball/laprdus-1.0.0-linux-x86_64.tar.xz ~/Downloads/
```

## Installation

```bash
# Extract
tar xf laprdus-1.0.0-linux-x86_64.tar.xz
cd laprdus-1.0.0-linux-x86_64

# System-wide install (default: /usr/local, requires root)
sudo ./install.sh

# Custom prefix
sudo ./install.sh /usr
```

## Uninstallation

```bash
# After install, use the persisted script:
sudo /usr/local/share/laprdus/uninstall.sh

# Or if tarball directory still exists:
sudo ./uninstall.sh
```

## Test

```bash
laprdus -l  # List voices
laprdus "Dobar dan"  # Speak text
spd-say -o laprdus "Zdravo"  # Via Speech Dispatcher
```
