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
sudo dnf install -y rpm-build rpmdevtools gcc-c++ scons \
    pulseaudio-libs-devel alsa-lib-devel speech-dispatcher-devel glib2-devel
```

## Build Steps

### 1. Set up rpmbuild directory structure
```bash
rpmdev-setuptree
```

### 2. Create source tarball
```bash
tar --transform='s,^\.,laprdus-1.0.0,' -czf ~/rpmbuild/SOURCES/laprdus-1.0.0.tar.gz \
    --exclude='.git' --exclude='build' --exclude='*.pyc' --exclude='__pycache__' \
    --exclude='android/.gradle' --exclude='android/app/build' \
    --exclude='nvda-addon/*.nvda-addon' --exclude='.sconsign*' .
```

### 3. Copy spec file and build
```bash
cp installers/linux/rpm/laprdus.spec ~/rpmbuild/SPECS/
rpmbuild -ba ~/rpmbuild/SPECS/laprdus.spec
```

## Expected Output

- `~/rpmbuild/RPMS/x86_64/laprdus-1.0.0-1.fc*.x86_64.rpm` - Main package
- `~/rpmbuild/RPMS/x86_64/laprdus-devel-1.0.0-1.fc*.x86_64.rpm` - Dev headers
- `~/rpmbuild/SRPMS/laprdus-1.0.0-1.fc*.src.rpm` - Source RPM

## Verification

```bash
ls -la ~/rpmbuild/RPMS/x86_64/laprdus*.rpm
```

## Installation

```bash
sudo rpm -ivh ~/rpmbuild/RPMS/x86_64/laprdus-1.0.0-1.fc*.x86_64.rpm
```

## Copy to Project Directory

```bash
cp ~/rpmbuild/RPMS/x86_64/laprdus*.rpm ~/rpmbuild/SRPMS/laprdus*.rpm \
    installers/linux/rpm/
```

## Test

```bash
laprdus -l  # List voices
laprdus "Dobar dan"  # Speak text
```
