---
name: build-sapi
description: Build Windows SAPI5 DLLs (x86 and x64) and InnoSetup installer for LaprdusTTS
allowed-tools: Bash, Read
---

# Build Windows SAPI5

Build the Windows SAPI5 text-to-speech driver with both 32-bit and 64-bit DLLs, then create the InnoSetup installer.

## Build Steps

Execute these commands in sequence:

### 1. Pack the voice data and stage the dictionaries
```bash
cargo xtask data
```

### 2. Build the SAPI5 DLLs, the CLI and the Configurator for x64 and x86
```bash
cargo xtask windows
cargo xtask windows-legacy   # the same for Windows 7 SP1, 8 and 8.1 (dist/windows-legacy-*)
```

Needs Rust with the MSVC build tools and both targets (`rustup target add x86_64-pc-windows-msvc i686-pc-windows-msvc`). The legacy step needs the nightly toolchain named in `src/platform/windows/legacy/toolchain.txt` with `rust-src` (`rustup toolchain install <name> --component rust-src`); the installer carries both builds and installs the one for the Windows it runs on.

### 3. Build the NVDA add-on, then the InnoSetup installer

The installer carries the add-on and offers to install it on its last page, so the add-on is built first.

```bash
(cd src/platform/windows/nvda && scons)
"/c/Program Files (x86)/Inno Setup 6/ISCC.exe" installers/windows/laprdus_sapi5.iss
```

## Expected Output

- `dist/windows-x64/`: `laprd64.dll`, `laprdus.exe`, `laprdgui.exe`
- `dist/windows-x86/`: `laprd32.dll`, `laprdus.exe`, `laprdgui.exe`
- `dist/windows-legacy-x64/`, `dist/windows-legacy-x86/`: the same files for Windows 7 SP1, 8 and 8.1
- Installer: `installers/windows/Output/Laprdus_SAPI5_Setup_2.1.0.exe`

## Voice Data

Step 1 packs the voice data (`data/voices/Josip.bin`, `data/voices/Vlado.bin`) and stages the dictionaries (`data/dictionary/`); the installer takes both from there.

## Verification

After building, verify the installer exists:
```bash
ls -la installers/windows/Output/Laprdus_SAPI5_Setup_2.1.0.exe
```

## Launch Installer for Testing

```bash
start "" "installers/windows/Output/Laprdus_SAPI5_Setup_2.1.0.exe"
```
