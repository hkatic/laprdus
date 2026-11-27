---
name: build-tarball
description: Build generic Linux tarball with install script for LaprdusTTS (source and binary)
allowed-tools: Bash, Read
---

# Build Generic Linux Tarball

Build a portable tarball package that works on any Linux distribution. Includes compiled binaries and install/uninstall scripts.

## Normal route: built and tested in containers

On any machine with Docker (a Mac with OrbStack or Docker Desktop included), and in CI (`.github/workflows/linux-packages.yml`):

```bash
scripts/linux-packages.sh tarball                  # this machine's architecture
scripts/linux-packages.sh --arch x86_64 tarball    # the other one (emulated on Apple silicon: slow)
```

It builds the package in a container and tests it on Debian 12, Fedora 45 and Arch Linux (built in `ubuntu:22.04`, so the binaries run with that C library or a newer one; its own `install.sh` with the default prefix `/usr/local`, after the system's Speech Dispatcher) (`installers/linux/ci/build.sh`, `test.sh`; the test is `tests/linux/test_package_install.sh`, strict). The package lands in `dist/linux-packages/<arch>/`. The steps below build it natively, for the C library of the machine they run on.

## Build Steps

```bash
cargo xtask dist
```

This packs the voice data, builds the CLI, the Speech Dispatcher module and the C API library, stages the tree in `dist/linux-x86_64/laprdus-2.1.0-linux-x86_64/` together with `install.sh`, `uninstall.sh` and `README` of `installers/linux/tarball/`, and packs it. The version is the Cargo workspace's; the architecture is the host's (`x86_64`, `aarch64`).

`installers/linux/tarball/build-tarball.sh` runs the same step and copies the tarball into the directory it was started from.

Build dependencies: Rust 1.85 or newer, a C linker, `pkg-config` and the ALSA development files (`libasound2-dev`, `alsa-lib-devel`, `alsa-lib`).

## Expected Output

- `dist/linux-x86_64/laprdus-2.1.0-linux-x86_64.tar.xz`

## Tarball Contents

```
laprdus-2.1.0-linux-x86_64/
├── bin/laprdus              # CLI tool
├── lib/
│   ├── liblaprdus.so        # Symlink → .so.2
│   ├── liblaprdus.so.2      # Symlink → .so.2.1.0
│   └── liblaprdus.so.2.1.0  # Library (with SONAME)
├── lib/speech-dispatcher-modules/
│   └── sd_laprdus           # Speech Dispatcher module
├── share/laprdus/
│   ├── Josip.bin            # Voice data
│   ├── Vlado.bin
│   ├── internal.json        # Pronunciation dictionary
│   ├── spelling.json        # Spelling dictionary
│   ├── emoji.json           # Emoji dictionary
│   └── emoji_sl.json        # Slovenian emoji names
├── etc/speech-dispatcher/modules/
│   └── laprdus.conf         # Module config
├── include/laprdus/         # C API header (laprdus_api.h)
├── share/doc/laprdus/       # Documentation
├── install.sh               # Installation script
├── uninstall.sh             # Convenience removal script
└── README                   # Usage instructions
```

## Key Design Decisions

- **Single package**: Everything in one tarball
- **Speech Dispatcher module**: Install script detects system speechd module dir via `pkg-config --variable=modulebindir speech-dispatcher`, else the first known folder that holds one of Speech Dispatcher's own modules (Fedora `/usr/lib64/speech-dispatcher-modules/`, Debian and Ubuntu `/usr/lib/speech-dispatcher-modules/`, Arch `/usr/lib/speech-dispatcher/speech-dispatcher-modules/`, which has no `.pc` file), and installs `sd_laprdus` THERE (not under the prefix). This is critical — SD scans only its own module dir, the user's (`~/.local/libexec/speech-dispatcher-modules`) and, for compatibility, `${libdir}/speech-dispatcher-modules`; never the tarball's prefix. Until 2026-11-23 the fallback knew only `/usr/lib64/` and `/usr/lib/speech-dispatcher-modules/` and required the folder to exist, so on Arch (no `.pc` file, neither folder) the module was not installed
- **ldconfig**: For non-standard prefixes (e.g., `/usr/local`), install script creates `/etc/ld.so.conf.d/laprdus.conf` so the library is discoverable
- **Persistent uninstall**: After install, uninstall script is saved to `<prefix>/share/laprdus/uninstall.sh` so it survives tarball directory deletion
- **Multiarch library paths**: Install script detects Debian/Ubuntu multiarch via `dpkg-architecture -qDEB_HOST_MULTIARCH`, Fedora `/usr/lib64/`, or falls back to `<prefix>/lib`
- **speechd.conf**: Auto-configured with BEGIN/END markers. Detects SD 0.12+ autodetection mode (no uncommented AddModule lines) and only adds LanguageDefaultModule entries to avoid disabling autodetection for other modules
- **Speech Dispatcher restart**: Uses `try-restart` for both system and user services, iterates logged-in users via `loginctl list-users`. On removal, also uses `killall speech-dispatcher` as fallback to ensure SD is fully stopped (socket activation restarts it clean)
- **Data dir auto-detection**: The CLI checks `../share/laprdus` relative to itself, then `/usr/share/laprdus` and `/usr/local/share/laprdus`; the speechd module checks the last two
- **Runtime dependencies**: glibc, libgcc and, for the CLI's playback, ALSA (`libasound.so.2`)
- **Library symlinks**: Use `cp -Pv` (preserve symlinks) when copying library files

## Copy to ~/Downloads

```bash
cp dist/linux-x86_64/laprdus-2.1.0-linux-x86_64.tar.xz ~/Downloads/
```

## Installation

```bash
# Extract
tar xf laprdus-2.1.0-linux-x86_64.tar.xz
cd laprdus-2.1.0-linux-x86_64

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
