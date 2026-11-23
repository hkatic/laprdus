---
paths:
  - "src/platform/linux/**"
  - "installers/linux/**"
  - "tests/linux/**"
---
# Linux

Moved from `CLAUDE.md` on 2026-11-23, unchanged. Claude Code loads this file when Claude reads a file matching the paths above; `CLAUDE.md` lists every topic file.

## Linux

### Overview

LaprdusTTS supports Linux through:
- **Speech Dispatcher module** (`sd_laprdus`) - Enables integration with Orca screen reader and other SSIP clients
- **Command-line interface** (`laprdus`) - Direct TTS synthesis from terminal

### Building for Linux

```bash
# Voice data, CLI, Speech Dispatcher module and the C API library
cargo xtask build

# Build only the CLI
cargo build --release -p laprdus-cli

# Build only the Speech Dispatcher module
cargo build --release -p laprdus-speechd
```

Output: `target/release/laprdus`, `target/release/sd_laprdus`, `target/release/liblaprdus_capi.so` (installed as `liblaprdus.so.2.0.0` with the links `liblaprdus.so.2` and `liblaprdus.so`).

### Linux Dependencies

| Package | Purpose |
|---------|---------|
| Rust 1.85+ (`cargo`, `rustc`) | The engine and every program |
| `pkg-config`, `libasound2-dev` | ALSA audio output of the CLI (through the `rodio` crate; PulseAudio and PipeWire serve ALSA) |

The Speech Dispatcher module is a Rust program that speaks the module protocol over its standard input and output; it needs neither `libspeechd-dev` nor GLib to build.

Install on Debian/Ubuntu:
```bash
sudo apt install pkg-config libasound2-dev
```

Install on Fedora/RHEL:
```bash
sudo dnf install pkgconf-pkg-config alsa-lib-devel
```

### Speech Dispatcher Integration

**Automatic Configuration**: When installing LaprdusTTS via packages (deb, rpm, PKGBUILD) or the tarball's `install.sh`, Speech Dispatcher is configured automatically. No manual configuration is needed.

The installation:
1. Installs the module binary to `/usr/lib/speech-dispatcher-modules/sd_laprdus`
2. Installs the config to `/etc/speech-dispatcher/modules/laprdus.conf`
3. Automatically adds `AddModule "laprdus"` to `/etc/speech-dispatcher/speechd.conf`
4. Sets LaprdusTTS as the default for Croatian (hr), Serbian (sr), Bosnian (bs) and Slovenian (sl)

The module lists every voice for every language; `SET LANGUAGE` calls `Engine::set_language()` and keeps the voice; `MALE1` is Zvonko for all three languages (MALE2 Josip, MALE3 Đedo, FEMALE1 Baba, CHILD_MALE Detence).

After installation, restart Speech Dispatcher:
```bash
systemctl --user restart speech-dispatcher
```

Test with spd-say:
```bash
spd-say -o laprdus "Dobar dan!"
spd-say -o laprdus -l hr "Hrvatski tekst"
spd-say -o laprdus -l sr "Srpski tekst"
spd-say -o laprdus -l sr -y "Zvonko" "Zvonko govori srpski"
spd-say -o laprdus -l bs -y "Josip" "Josip govori bosanski"
```

**Manual Configuration** (only if automatic configuration fails):
```
# Add to /etc/speech-dispatcher/speechd.conf:
AddModule "laprdus" "sd_laprdus" "laprdus.conf"
DefaultModule laprdus  # Optional: set as default
```

### Linux Package Locations

| Component | System Path |
|-----------|-------------|
| Library | `/usr/lib/liblaprdus.so` (Arch), `/usr/lib/<multiarch>/` (Debian, Ubuntu), `/usr/lib64/` (Fedora) |
| CLI | `/usr/bin/laprdus` |
| Voice data | `/usr/share/laprdus/` |
| Speech Dispatcher module | where the distribution's Speech Dispatcher loads its modules: `/usr/lib/speech-dispatcher-modules/sd_laprdus` (Debian, Ubuntu, amd64 and arm64 alike), `/usr/lib64/speech-dispatcher-modules/` (Fedora), `/usr/lib/speech-dispatcher/speech-dispatcher-modules/` (Arch, built with `--libexecdir=/usr/lib/speech-dispatcher/`; the PKGBUILD passes `--moduledir`). Speech Dispatcher (`src/server/speechd.c` upstream) scans three folders: the user's (`~/.local/libexec/speech-dispatcher-modules`), its `MODULEBINDIR` (`${libexecdir}/speech-dispatcher-modules`, where its own modules are) and, for compatibility, `OLDMODULEBINDIR` (`${libdir}/speech-dispatcher-modules`). Until 2026-11-23 the Arch package put the module in the last (`/usr/lib/speech-dispatcher-modules`), where it was found too; it now goes next to Arch's own modules |
| Module config | `/etc/speech-dispatcher/modules/laprdus.conf` |

### Linux Installation

#### From packages

```bash
# Debian/Ubuntu
sudo apt install ./laprdus_2.0.0-1_amd64.deb   # one package (apt, unlike dpkg -i, also installs speech-dispatcher): CLI, library, Speech Dispatcher module, voice data, dictionaries

# Fedora/RHEL
sudo dnf install ./laprdus-2.0.0-1.fc*.x86_64.rpm  # one package as well

# Arch Linux
makepkg -si  # From PKGBUILD directory
```

#### From tarball

```bash
tar xf laprdus-2.0.0-linux-x86_64.tar.xz
cd laprdus-2.0.0-linux-x86_64
sudo ./install.sh
```

#### From source

```bash
cargo xtask build
sudo cargo xtask install --prefix /usr
```

`cargo xtask install` copies the files (default prefix `/usr/local`; `--destdir` stages for a package). Adding the module to `speechd.conf` and restarting Speech Dispatcher is done by the packages' and the tarball's own scripts, not by this step: see "Manual Configuration" above.

## Linux tests and packages

#### Testing Checklist for Linux

- [ ] `cargo xtask build` builds the C API library (`liblaprdus_capi.so`), the CLI (`laprdus`) and the Speech Dispatcher module (`sd_laprdus`) without errors
- [ ] `cargo test --release --workspace` passes
- [ ] CLI `-h` displays help correctly
- [ ] CLI `-l` lists all 18 voices with their native language
- [ ] CLI synthesizes audio to WAV file correctly
- [ ] All voices synthesize correctly (josip, vlado, detence, baba, djedo, zvonko, stojan, mirsad, the singing presets), and `-L hr|sr|bs|sl` switches the language of any of them
- [ ] Formant voice tests pass (`cargo test --release --test formant`)
- [ ] Recorded voice tests pass (`cargo test --release --test recorded`, needs `data/voices` or `LAPRDUS_DATA`)

#### Packages built and tested in containers (2026-11-23)

The maintainer does not always have the four distributions at hand, so the packages are built and tested in Docker containers, on the developer's machine and in GitHub Actions alike:

```bash
scripts/linux-packages.sh                      # all four formats, this machine's architecture
scripts/linux-packages.sh deb rpm              # some of them
scripts/linux-packages.sh --arch x86_64 arch   # the other architecture (on Apple silicon emulated: slow)
scripts/linux-packages.sh --no-test tarball    # build only
scripts/linux-packages.sh --test-only --image debian:13 deb   # test what is built, on one system
```

Needs only Docker (or Podman, `DOCKER=podman`) and Git; on the Mac OrbStack, Docker Desktop or colima. `scripts/build-all.sh linux-packages [options]` runs the same.

- `installers/linux/ci/source.sh` packs the working tree as Git sees it (uncommitted changes and new files included; the real index is not touched) into `dist/linux-packages/laprdus-<version>.tar.xz`, the tarball the spec's `Source0` and the PKGBUILD take, after checking that `Cargo.toml`, the head of `debian/changelog`, the spec's `Version` and the PKGBUILD's `pkgver` agree (and, with `LAPRDUS_RELEASE_TAG`, the tag).
- `installers/linux/ci/build.sh <format>` builds in one container from that tarball (never in the checkout), into `dist/linux-packages/<arch>/`.
- `installers/linux/ci/test.sh <format>` installs the package in a fresh container with the distribution's own package manager (so the dependencies must resolve, `speech-dispatcher` included; the tarball with its `install.sh` and the default prefix `/usr/local`), runs `tests/linux/test_package_install.sh` with `LAPRDUS_TEST_STRICT=1` (a skip is a failure), removes the package and checks nothing is left (the CLI, the library in the linker's cache, the module, the block in `speechd.conf`).

| Format | Built in | Tested on |
|--------|----------|-----------|
| deb | `ubuntu:22.04`, Rust from rustup (Debian 12's rustc is 1.63) | Debian 12, 13; Ubuntu 22.04, 24.04, 26.04 |
| rpm | `fedora:44`, Fedora's Rust | Fedora 44, 45 |
| arch | `archlinux:latest`, Arch's Rust, makepkg as a user | Arch Linux |
| tarball | `ubuntu:22.04`, Rust from rustup | Debian 12, Fedora 45, Arch Linux |

One package per format serves every system of its row: the programs are Rust and link only glibc, libgcc and ALSA, so they are built on the oldest C library of the row (glibc 2.35 of Ubuntu 22.04). The lists are in `build_image()` and `test_images()` of `scripts/linux-packages.sh`, the only place: bump Fedora there when a release comes (`fedora:latest` was 45 in November 2026). x86_64 and aarch64 both, except Arch: there is no official arm64 `archlinux` image, so the PKGBUILD's `aarch64` is untested.

The package test checks the Speech Dispatcher module three ways: it lies in the folder where Speech Dispatcher's own modules are; it answers the module protocol (`INIT`, `SPEAK`, the text, `.`, then `QUIT` after `702 END`, since a `QUIT` stops the speech) with `705 AUDIO` blocks, which needs neither a server nor a sound card; and a Speech Dispatcher started by `spd-say` lists it (`spd-say -O`) and its voices (`spd-say -o laprdus -L`). Until 2026-11-23 the script ended after its first result (`((PASSED++))` under `set -e`), looked for the library only in `<prefix>/lib` and the module only in `/usr/lib/speech-dispatcher-modules`, and listed voices where it meant modules.

**GitHub Actions** (`.github/workflows/linux-packages.yml`): one job per format and architecture (x86_64 on `ubuntu-24.04`, aarch64 on `ubuntu-24.04-arm`; free for the public repository), each running `scripts/linux-packages.sh --arch <arch> <format>`; the packages are kept as artifacts for 14 days. It runs on a push to `dev` that touches the engine, the C API, the CLI, the Linux programs, the data, the packaging or the build configuration, by hand (Actions > Linux packages > Run workflow, or `gh workflow run linux-packages.yml --ref dev`), and on a tag `v*`: then, when every job has passed, a **draft** release gets every package, the source tarball and `SHA256SUMS-linux` (added to the release if the tag has one already); the maintainer writes the notes and publishes it. A tag with a hyphen (`v2.0.0-rc1`) makes a pre-release.

#### Linux Package Testing (if building packages)

The packages build with Cargo (`cargo xtask build`, then `cargo xtask install --destdir ...`); the exact steps are in `.claude/skills/build-deb`, `build-rpm`, `build-arch` and `build-tarball`.

```bash
# Debian package (the debian directory is linked to the project root for the build)
ln -sfn installers/linux/deb/debian debian && dpkg-buildpackage -us -uc -b -rfakeroot -d; rm -f debian

# Arch package (the PKGBUILD takes laprdus-2.0.0.tar.xz next to it)
installers/linux/ci/source.sh installers/linux/arch && cd installers/linux/arch && makepkg -sf

# Tarball: dist/linux-<arch>/laprdus-<version>-linux-<arch>.tar.xz
cargo xtask dist

# An installed package or tarball
tests/linux/test_package_install.sh /usr
```

#### Building RPM Package (Fedora)

Natively, the RPM build requires running on a Fedora system (native or WSL); `scripts/linux-packages.sh rpm` builds and tests it in containers on any machine with Docker.

**1. Install build dependencies:**
```bash
sudo dnf install -y rpm-build rpmdevtools cargo rust gcc pkgconf-pkg-config alsa-lib-devel
```

The engine needs Rust 1.85 or newer. Cargo downloads the crates of `Cargo.lock` during the build, so the build needs network access.

**2. Set up rpmbuild tree:**
```bash
rpmdev-setuptree
```

**3. Create source tarball and build:**
```bash
# From project root (xz: the spec's Source0 is .tar.xz)
installers/linux/ci/source.sh ~/rpmbuild/SOURCES

# Copy spec and build (--nodeps when Rust comes from rustup, not dnf)
cp installers/linux/rpm/laprdus.spec ~/rpmbuild/SPECS/
rpmbuild -ba ~/rpmbuild/SPECS/laprdus.spec
```

The spec builds with `cargo xtask build` and installs with `cargo xtask install --destdir %{buildroot} ...`.

**4. Output files:**
```
~/rpmbuild/RPMS/x86_64/laprdus-2.0.0-1.fc*.x86_64.rpm      # Main package (CLI, library, Speech Dispatcher module, voice data, dictionaries)
~/rpmbuild/RPMS/x86_64/laprdus-devel-2.0.0-1.fc*.x86_64.rpm # Dev headers
~/rpmbuild/SRPMS/laprdus-2.0.0-1.fc*.src.rpm               # Source RPM
```

**5. Copy to project directory:**
```bash
cp ~/rpmbuild/RPMS/x86_64/laprdus*.rpm ~/rpmbuild/SRPMS/laprdus*.rpm \
    installers/linux/rpm/
```

**6. Install and test:**
```bash
sudo rpm -ivh ~/rpmbuild/RPMS/x86_64/laprdus-2.0.0-1.fc*.x86_64.rpm

# Test
laprdus -l              # List voices
laprdus "Dobar dan"     # Speak text
laprdus -o test.wav "Test"  # Save to file
spd-say -o laprdus "Zdravo" # Via Speech Dispatcher
```

**Note:** The Speech Dispatcher module is part of the main RPM (`%{_libdir}/speech-dispatcher-modules/sd_laprdus`): it is a Rust program that needs no Speech Dispatcher headers to build. The library has the soname `liblaprdus.so.2`.
