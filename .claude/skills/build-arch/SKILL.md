---
name: build-arch
description: Build Arch Linux packages using makepkg for LaprdusTTS
allowed-tools: Bash, Read
---

# Build Arch Linux Packages

Build packages for Arch-based Linux distributions (Arch Linux, Manjaro, EndeavourOS).

**Note:** The native build below must be run on an Arch-based system.

## Normal route: built and tested in containers

On any machine with Docker (a Mac with OrbStack or Docker Desktop included), and in CI (`.github/workflows/linux-packages.yml`):

```bash
scripts/linux-packages.sh arch                  # this machine's architecture
scripts/linux-packages.sh --arch x86_64 arch    # the other one (emulated on Apple silicon: slow)
```

It builds the package in a container and tests it on Arch Linux (`archlinux:latest`; x86_64 only: there is no official arm64 image, so the PKGBUILD's `aarch64` is untested) (`installers/linux/ci/build.sh`, `test.sh`; the test is `tests/linux/test_package_install.sh`, strict). The package lands in `dist/linux-packages/<arch>/`. The steps below are the native build, for a machine running the distribution.

## Prerequisites

Install build dependencies:
```bash
sudo pacman -S --needed base-devel rust alsa-lib speech-dispatcher
```

`rust` provides `cargo`; a rustup toolchain works too, but then makepkg does not see the `cargo` build dependency: add `-d`.

## Build Steps

The PKGBUILD takes the sources from `laprdus-2.0.0.tar.xz` next to it (top folder `laprdus-2.0.0/`). Make it from the checkout, then build:

```bash
installers/linux/ci/source.sh installers/linux/arch    # laprdus-2.0.0.tar.xz, top folder laprdus-2.0.0/
cd installers/linux/arch && makepkg -sf && cd ../../..
```

`source.sh` packs the working tree as Git sees it (uncommitted changes and new files included, nothing `.gitignore` leaves out) and first checks that `Cargo.toml`, `debian/changelog`, the spec and the PKGBUILD name the same version.

The PKGBUILD builds with Cargo: `cargo fetch --locked` in `prepare()`, `cargo xtask build` in `build()` (packs the voice data, builds the CLI, the Speech Dispatcher module and the C API library) and `cargo xtask install --destdir "$pkgdir" --prefix /usr --moduledir /usr/lib/speech-dispatcher/speech-dispatcher-modules` in `package()`.

**Options:**
- `-s` - Install missing dependencies
- `-f` - Force rebuild even if package exists
- `-i` - Install package after building

## Expected Output (single package)

One package is created in `installers/linux/arch/`:
- `laprdus-2.0.0-1-x86_64.pkg.tar.zst` - Complete package

## Key Design Decisions

- **Single package**: CLI, library, Speech Dispatcher module, voice data, the dictionaries (internal.json, spelling.json, emoji.json, emoji_sl.json), AND the C API header all in ONE package
- **Runtime dependencies**: `glibc`, `gcc-libs`, `alsa-lib` (the CLI plays through ALSA, which PulseAudio and PipeWire serve), `speech-dispatcher`. The Speech Dispatcher module links only the C library
- **ldconfig**: Called in `post_install()`, `post_upgrade()`, and `post_remove()` hooks via `laprdus.install`
- **speechd.conf**: Auto-configured in `post_install()`/`post_upgrade()` with BEGIN/END markers, cleaned up in `pre_remove()`. Detects SD 0.12+ autodetection mode (no uncommented AddModule lines) and only adds LanguageDefaultModule entries to avoid disabling autodetection for other modules
- **Speech Dispatcher restart**: Uses `try-restart` for both system and user services, iterates logged-in users via `loginctl list-users`. On removal, also uses `killall speech-dispatcher` as fallback to ensure SD is fully stopped (socket activation restarts it clean)
- **speech-dispatcher dependency**: Declared in `depends` array
- **README filename**: Must reference `readme.md` (lowercase), NOT `README.md`

## Package Contents

### laprdus
- `/usr/lib/liblaprdus.so*` - Shared library with the C API (`liblaprdus.so.2.0.0`, links `.so.2` and `.so`)
- `/usr/bin/laprdus` - CLI tool
- `/usr/share/laprdus/` - Voice data and dictionaries
- `/usr/lib/speech-dispatcher/speech-dispatcher-modules/sd_laprdus` - Speech Dispatcher module, next to Arch's own (`sd_dummy`, `sd_espeak-ng`): Arch builds Speech Dispatcher with `--libexecdir=/usr/lib/speech-dispatcher/`, which makes this its module folder. Until 2026-11-23 the package put the module in `/usr/lib/speech-dispatcher-modules/`, the folder of older versions (`${libdir}/speech-dispatcher-modules`), which Speech Dispatcher still scans for compatibility, so it was found there too
- `/etc/speech-dispatcher/modules/laprdus.conf` - Module config
- `/usr/include/laprdus/laprdus_api.h` - C API header

## Copy to ~/Downloads

```bash
cp installers/linux/arch/laprdus-2.0.0-1-x86_64.pkg.tar.zst ~/Downloads/
```

## Test

```bash
sudo pacman -U installers/linux/arch/laprdus-2.0.0-1-x86_64.pkg.tar.zst
laprdus -l  # List voices
laprdus "Dobar dan"  # Speak text
spd-say -o laprdus "Zdravo"  # Via Speech Dispatcher
```

## AUR Submission

The PKGBUILD can be submitted to the Arch User Repository (AUR) for community distribution.
