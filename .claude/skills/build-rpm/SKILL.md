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
sudo dnf install -y rpm-build rpmdevtools gcc-c++ \
    pulseaudio-libs-devel alsa-lib-devel speech-dispatcher-devel glib2-devel
```

SCons (if not packaged): `python3 -m pip install --user scons`

## Build Steps

### 1. Set up rpmbuild directory structure
```bash
rpmdev-setuptree
```

### 2. Create source tarball (xz compression to match spec)
```bash
cd /home/hrvojekatic/.repo/hkatic/laprdus
tar --transform='s,^\.,laprdus-1.0.0,' -cJf ~/rpmbuild/SOURCES/laprdus-1.0.0.tar.xz \
    --exclude='.git' --exclude='build' --exclude='*.pyc' --exclude='__pycache__' \
    --exclude='android/.gradle' --exclude='android/app/build' \
    --exclude='nvda-addon/*.nvda-addon' --exclude='.sconsign*' .
```

**IMPORTANT:** Use `-cJf` (xz) — the spec file `Source0` expects `.tar.xz`. Using `-czf` (gzip) creates a `.tar.gz` which rpmbuild will not find.

### 3. Copy spec file and build
```bash
cp installers/linux/rpm/laprdus.spec ~/rpmbuild/SPECS/
rpmbuild -ba ~/rpmbuild/SPECS/laprdus.spec
```

**Note:** If SCons was installed via pip (not dnf), rpmbuild may fail on `BuildRequires: scons`. Use `rpmbuild -ba --nodeps` as a workaround.

## Expected Output (single package + devel)

- `~/rpmbuild/RPMS/x86_64/laprdus-1.0.0-1.fc*.x86_64.rpm` - Main package (CLI + library + Speech Dispatcher module + voice data + dictionaries)
- `~/rpmbuild/RPMS/x86_64/laprdus-devel-1.0.0-1.fc*.x86_64.rpm` - Dev headers
- `~/rpmbuild/SRPMS/laprdus-1.0.0-1.fc*.src.rpm` - Source RPM

## Key Design Decisions

- **Single package**: CLI, library, Speech Dispatcher module, voice data, and all 3 dictionaries (internal.json, spelling.json, emoji.json) are in ONE package
- **SONAME**: Library has `-Wl,-soname,liblaprdus.so.1` so RPM auto-dependency resolves correctly
- **Versioned library**: `liblaprdus.so.1.0.0` → `liblaprdus.so.1` → `liblaprdus.so` (unversioned in -devel only)
- **speechd.conf**: Auto-configured in `%post` with BEGIN/END markers, cleaned up in `%preun`. Detects SD 0.12+ autodetection mode (no uncommented AddModule lines) and only adds LanguageDefaultModule entries to avoid disabling autodetection for other modules
- **Speech Dispatcher restart**: Uses `try-restart` for both system and user services, iterates logged-in users via `loginctl list-users`. On removal, also uses `killall speech-dispatcher` as fallback to ensure SD is fully stopped (socket activation restarts it clean)
- **Spec must have Unix line endings (LF)** — CRLF causes `$'\r': command not found`

## Copy to ~/Downloads

```bash
cp ~/rpmbuild/RPMS/x86_64/laprdus*.rpm ~/Downloads/
```

## Installation & Test

```bash
sudo rpm -ivh ~/rpmbuild/RPMS/x86_64/laprdus-1.0.0-1.fc*.x86_64.rpm
laprdus -l              # List voices
laprdus "Dobar dan"     # Speak text
spd-say -o laprdus "Zdravo"  # Via Speech Dispatcher
```
