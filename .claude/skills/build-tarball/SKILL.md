---
name: build-tarball
description: Build generic Linux tarball with install script for LaprdusTTS (source and binary)
allowed-tools: Bash, Read
argument-hint: "[version]"
---

# Build Generic Linux Tarball

Build a portable tarball package that works on any Linux distribution. Includes both compiled binaries and an install script.

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
│   ├── liblaprdus.so        # Symlink
│   ├── liblaprdus.so.1      # Symlink
│   └── liblaprdus.so.1.0.0  # Library
├── lib/speech-dispatcher-modules/
│   └── sd_laprdus           # Speech Dispatcher module
├── share/laprdus/
│   ├── Josip.bin            # Voice data
│   ├── Vlado.bin
│   └── *.json               # Dictionaries
├── etc/speech-dispatcher/modules/
│   └── laprdus.conf         # Module config
├── include/laprdus/         # Headers
├── share/doc/laprdus/       # Documentation
├── install.sh               # Installation script
├── uninstall.sh             # Removal script
└── README                   # Usage instructions
```

## Verification

```bash
ls -la installers/linux/tarball/laprdus-*.tar.xz
tar -tvf installers/linux/tarball/laprdus-1.0.0-linux-x86_64.tar.xz | head -20
```

## Installation from Tarball

```bash
# Extract
tar xf laprdus-1.0.0-linux-x86_64.tar.xz
cd laprdus-1.0.0-linux-x86_64

# System-wide install (requires root)
sudo ./install.sh /usr/local

# User install (no root needed)
./install.sh ~/.local
```

## Uninstallation

```bash
sudo ./uninstall.sh /usr/local
# or
./uninstall.sh ~/.local
```

## Test

```bash
laprdus -l  # List voices
laprdus "Dobar dan"  # Speak text
```

## Source Tarball

For a source-only tarball (for compilation on target system):
```bash
tar --transform='s,^\.,laprdus-1.0.0,' -czf laprdus-1.0.0-source.tar.gz \
    --exclude='.git' --exclude='build' --exclude='*.pyc' \
    --exclude='android/.gradle' --exclude='android/app/build' .
```
