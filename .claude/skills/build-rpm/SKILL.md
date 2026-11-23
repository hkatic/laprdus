---
name: build-rpm
description: Build Fedora/RHEL RPM packages for LaprdusTTS
allowed-tools: Bash, Read
---

# Build RPM Packages

Build .rpm packages for RPM-based Linux distributions (Fedora, RHEL, CentOS, openSUSE).

**Note:** This build must be run on a Fedora/RHEL system (native or WSL).

## Prerequisites

Install build dependencies:
```bash
sudo dnf install -y rpm-build rpmdevtools cargo rust gcc pkgconf-pkg-config alsa-lib-devel
```

The engine needs Rust 1.85 or newer. Cargo downloads the crates of `Cargo.lock` during the build, so the build needs network access (it does not work in `mock` without vendoring).

## Build Steps

### 1. Set up rpmbuild directory structure
```bash
rpmdev-setuptree
```

### 2. Create source tarball (xz compression to match spec)
```bash
cd /home/hrvojekatic/.repo/hkatic/laprdus
tar --transform='s,^\.,laprdus-2.0.0,' -cJf ~/rpmbuild/SOURCES/laprdus-2.0.0.tar.xz \
    --exclude='./.git' --exclude='./target' --exclude='./dist' --exclude='./build' \
    --exclude='*.pyc' --exclude='__pycache__' \
    --exclude='./src/platform/android/.gradle' --exclude='./src/platform/android/app/build' \
    --exclude='./src/platform/windows/nvda' .
```

**IMPORTANT:** Use `-cJf` (xz) — the spec file `Source0` expects `.tar.xz`. Using `-czf` (gzip) creates a `.tar.gz` which rpmbuild will not find.

### 3. Copy spec file and build
```bash
cp installers/linux/rpm/laprdus.spec ~/rpmbuild/SPECS/
rpmbuild -ba ~/rpmbuild/SPECS/laprdus.spec
```

The spec builds with `cargo xtask build` (packs the voice data, builds the CLI, the Speech Dispatcher module and the C API library) and installs with `cargo xtask install --destdir %{buildroot} --prefix %{_prefix} --libdir %{_libdir} --moduledir %{_libdir}/speech-dispatcher-modules`.

**Note:** If Rust was installed with rustup (not dnf), rpmbuild fails on `BuildRequires: cargo`. Use `rpmbuild -ba --nodeps` as a workaround.

## Expected Output (single package + devel)

- `~/rpmbuild/RPMS/x86_64/laprdus-2.0.0-1.fc*.x86_64.rpm` - Main package (CLI + library + Speech Dispatcher module + voice data + dictionaries)
- `~/rpmbuild/RPMS/x86_64/laprdus-devel-2.0.0-1.fc*.x86_64.rpm` - Dev headers
- `~/rpmbuild/SRPMS/laprdus-2.0.0-1.fc*.src.rpm` - Source RPM

## Key Design Decisions

- **Single package**: CLI, library, Speech Dispatcher module, voice data, and the dictionaries (internal.json, spelling.json, emoji.json, emoji_sl.json) are in ONE package
- **SONAME**: Library has the soname `liblaprdus.so.2` (set by `cargo xtask build`) so RPM auto-dependency resolves correctly
- **Versioned library**: `liblaprdus.so.2.0.0` ← `liblaprdus.so.2` ← `liblaprdus.so` (unversioned in -devel only)
- **speechd.conf**: Auto-configured in `%post` with BEGIN/END markers, cleaned up in `%preun`. Detects SD 0.12+ autodetection mode (no uncommented AddModule lines) and only adds LanguageDefaultModule entries to avoid disabling autodetection for other modules
- **Speech Dispatcher restart**: Uses `try-restart` for both system and user services, iterates logged-in users via `loginctl list-users`. On removal, also uses `killall speech-dispatcher` as fallback to ensure SD is fully stopped (socket activation restarts it clean)
- **Speech Dispatcher module**: Included (`%{_libdir}/speech-dispatcher-modules/sd_laprdus`); it is a Rust program that needs no Speech Dispatcher headers to build
- **Spec must have Unix line endings (LF)** — CRLF causes `$'\r': command not found`

## Copy to ~/Downloads

```bash
cp ~/rpmbuild/RPMS/x86_64/laprdus*.rpm ~/Downloads/
```

## Installation & Test

```bash
sudo rpm -ivh ~/rpmbuild/RPMS/x86_64/laprdus-2.0.0-1.fc*.x86_64.rpm
laprdus -l              # List voices
laprdus "Dobar dan"     # Speak text
spd-say -o laprdus "Zdravo"  # Via Speech Dispatcher
```
