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
| Library | `/usr/lib/liblaprdus.so` |
| CLI | `/usr/bin/laprdus` |
| Voice data | `/usr/share/laprdus/` |
| Speech Dispatcher module | `/usr/lib/speech-dispatcher-modules/sd_laprdus` |
| Module config | `/etc/speech-dispatcher/modules/laprdus.conf` |

### Linux Installation

#### From packages

```bash
# Debian/Ubuntu
sudo dpkg -i laprdus_2.0.0-1_amd64.deb     # one package: CLI, library, Speech Dispatcher module, voice data, dictionaries

# Fedora/RHEL
sudo rpm -i laprdus-2.0.0-1.fc*.x86_64.rpm  # one package as well

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

#### Linux Package Testing (if building packages)

The packages build with Cargo (`cargo xtask build`, then `cargo xtask install --destdir ...`); the exact steps are in `.claude/skills/build-deb`, `build-rpm`, `build-arch` and `build-tarball`.

```bash
# Debian package (the debian directory is linked to the project root for the build)
ln -sfn installers/linux/deb/debian debian && dpkg-buildpackage -us -uc -b -rfakeroot -d; rm -f debian

# Arch package (the PKGBUILD takes laprdus-2.0.0.tar.xz next to it: see the build-arch skill)
cd installers/linux/arch && makepkg -sf

# Tarball: dist/linux-<arch>/laprdus-<version>-linux-<arch>.tar.xz
cargo xtask dist

# An installed package or tarball
tests/linux/test_package_install.sh /usr
```

#### Building RPM Package (Fedora)

The RPM build requires running on a Fedora system (native or WSL).

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
tar --transform='s,^\.,laprdus-2.0.0,' -cJf ~/rpmbuild/SOURCES/laprdus-2.0.0.tar.xz \
    --exclude='./.git' --exclude='./target' --exclude='./dist' --exclude='./build' \
    --exclude='*.pyc' --exclude='__pycache__' \
    --exclude='./src/platform/android/.gradle' --exclude='./src/platform/android/app/build' \
    --exclude='./src/platform/windows/nvda' .

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
