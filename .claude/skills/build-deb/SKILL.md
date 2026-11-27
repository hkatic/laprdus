---
name: build-deb
description: Build Debian/Ubuntu .deb packages for LaprdusTTS
allowed-tools: Bash, Read
---

# Build Debian Packages

Build .deb packages for Debian-based Linux distributions (Debian, Ubuntu, Linux Mint, etc.).

**Note:** The native build below must be run on a Debian/Ubuntu system (native or WSL).

## Normal route: built and tested in containers

On any machine with Docker (a Mac with OrbStack or Docker Desktop included), and in CI (`.github/workflows/linux-packages.yml`):

```bash
scripts/linux-packages.sh deb                  # this machine's architecture
scripts/linux-packages.sh --arch x86_64 deb    # the other one (emulated on Apple silicon: slow)
```

It builds the package in a container and tests it on Debian 12 and 13 and Ubuntu 22.04, 24.04 and 26.04 (built in `ubuntu:22.04`, the oldest C library of them) (`installers/linux/ci/build.sh`, `test.sh`; the test is `tests/linux/test_package_install.sh`, strict). The package lands in `dist/linux-packages/<arch>/`. The steps below are the native build, for a machine running the distribution.

## Prerequisites

Install build dependencies:
```bash
sudo apt install -y build-essential debhelper devscripts cargo rustc pkg-config libasound2-dev
```

The engine needs Rust 1.85 or newer. Where the distribution's `cargo` is older, install Rust with rustup (https://rustup.rs) and pass `-d` to `dpkg-buildpackage`, as below, so the `cargo`/`rustc` build dependencies are not checked. Cargo downloads the crates of `Cargo.lock` during the build, so the build needs network access.

```bash
rustc --version   # 1.85 or newer
```

## Build Steps

The debian directory lives under `installers/linux/deb/debian/` but `dpkg-buildpackage` expects it at the project root alongside `Cargo.toml`. Use a symlink:

```bash
cd /home/hrvojekatic/.repo/hkatic/laprdus
ln -sfn installers/linux/deb/debian debian && dpkg-buildpackage -us -uc -b -rfakeroot -d; rm -f debian
```

`debian/rules` builds with `cargo xtask build` (packs the voice data, builds the CLI, the Speech Dispatcher module and the C API library) and installs with `cargo xtask install --destdir debian/laprdus --prefix /usr --libdir /usr/lib/<multiarch>`; the header is then moved to `laprdus-dev`.

**WSL note:** The build automatically detects WSL and handles NTFS permission issues (777 on all files) by copying package trees to `/tmp` for correct `dpkg-deb` packaging.

## Expected Output (single package + dev)

The build creates packages in the parent directory (`..`):
- `laprdus_2.1.0-1_amd64.deb` - Main package (CLI + library + Speech Dispatcher module + voice data + dictionaries)
- `laprdus-dev_2.1.0-1_amd64.deb` - Development headers

## Key Design Decisions

- **Single package**: CLI, library, Speech Dispatcher module, voice data, and the dictionaries (internal.json, spelling.json, emoji.json, emoji_sl.json) are in ONE `laprdus` package
- **Multiarch library path**: Library installs to `/usr/lib/$(DEB_HOST_MULTIARCH)/` for proper Debian multiarch support
- **ldconfig**: Handled automatically by debhelper triggers when library is in multiarch path
- **speechd.conf**: Auto-configured in `postinst` with BEGIN/END markers, cleaned up in `postrm`. Detects SD 0.12+ autodetection mode (no uncommented AddModule lines) and only adds LanguageDefaultModule entries to avoid disabling autodetection for other modules (the other installed modules)
- **Speech Dispatcher restart**: Uses `try-restart` for both system and user services, iterates logged-in users via `loginctl list-users`. On removal, also uses `killall speech-dispatcher` as fallback to ensure SD is fully stopped (socket activation restarts it clean)
- **speech-dispatcher dependency**: Declared in `Depends:` field; the library dependencies (`libc6`, `libgcc-s1`, `libasound2` for the CLI's playback) come from `${shlibs:Depends}`

## Package Contents

### laprdus
- `/usr/lib/<multiarch>/liblaprdus.so*` - Shared library
- `/usr/bin/laprdus` - CLI tool
- `/usr/share/laprdus/` - Voice data (Josip.bin, Vlado.bin) and dictionaries
- `/usr/lib/speech-dispatcher-modules/sd_laprdus` - Speech Dispatcher module
- `/etc/speech-dispatcher/modules/laprdus.conf` - Module config

### laprdus-dev
- `/usr/include/laprdus/laprdus_api.h` - C API header

## Copy to ~/Downloads

```bash
mv ../laprdus_2.1.0-1_amd64.deb ../laprdus-dev_2.1.0-1_amd64.deb ~/Downloads/
```

## Test

```bash
sudo dpkg -i laprdus_2.1.0-1_amd64.deb
laprdus -l  # List voices
laprdus "Dobar dan"  # Speak text
spd-say -o laprdus "Zdravo"  # Via Speech Dispatcher
```
