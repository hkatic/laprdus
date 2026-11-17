---
name: build-nvda
description: Build NVDA screen reader addon package for LaprdusTTS
allowed-tools: Bash, Read
---

# Build NVDA Addon

Build the NVDA screen reader synthesizer addon for LaprdusTTS.

## Prerequisites

The NVDA addon requires the following to be built first:
- Voice data and dictionaries (`data/voices/`, `data/dictionary/`)
- SAPI5 DLLs (both x64 and x86)
- Config GUI executables (both x64 and x86)

They are built by Cargo (Rust with the MSVC build tools and both targets: `rustup target add x86_64-pc-windows-msvc i686-pc-windows-msvc`). The add-on itself is built by its own SCons template (`pip install scons`).

The build process will automatically copy them.

## Build Steps

Execute these commands in sequence:

### 1. Pack the voice data and stage the dictionaries
```bash
cargo xtask data
```

### 2. Build the DLLs and the Config GUI for x64 and x86
```bash
cargo xtask windows
cargo xtask windows-legacy   # the same for Windows 7 SP1, 8 and 8.1 (dist/windows-legacy-*)
```

This stages `dist/windows-x64/` (`laprd64.dll`, `laprdus.exe`, `laprdgui.exe`) and `dist/windows-x86/` (`laprd32.dll`, `laprdus.exe`, `laprdgui.exe`), and the same files for Windows 7 SP1, 8 and 8.1 in `dist/windows-legacy-x64/` and `dist/windows-legacy-x86/`; the add-on carries the 32-bit legacy driver and Configurator as `laprd32_legacy.dll` and `laprdgui32_legacy.exe`. The legacy step needs the nightly toolchain named in `src/platform/windows/legacy/toolchain.txt` with `rust-src`.

### 3. Build NVDA Addon
```bash
(cd src/platform/windows/nvda && scons)
```

## Expected Output

- Addon package: `src/platform/windows/nvda/laprdus-2.0.0.nvda-addon`

## Automatic Resource Copying

The NVDA addon SCons build automatically:
- Copies DLLs (`laprd64.dll`, `laprd32.dll`) from `dist/windows-x64/` and `dist/windows-x86/`
- Copies GUI executables (`laprdgui.exe` of `dist/windows-x64/` and `dist/windows-x86/` as `laprdgui64.exe` and `laprdgui32.exe`) for the Laprdus Configurator
- Copies voice data from `data/voices/` to `addon/synthDrivers/laprdus/voices/`
- Copies dictionaries from `data/dictionary/` to `addon/synthDrivers/laprdus/dictionaries/`

## NVDA Menu Integration

The addon includes a globalPlugin that adds a "Laprdus" submenu to NVDA's Tools menu with:
- "Laprdus Configurator..." - Opens the configuration GUI (launches the correct 32/64-bit exe based on system architecture)
- "Laprdus on the Web" - Opens the Laprdus website in the default browser

## Verification

After building, verify the addon package exists:
```bash
ls -la src/platform/windows/nvda/laprdus-*.nvda-addon
```

## Launch Addon Installer for Testing

```bash
start "" "src/platform/windows/nvda/laprdus-2.0.0.nvda-addon"
```

## CRITICAL Build Rules

- **ALWAYS** use SCons to build the NVDA addon
- **NEVER** manually create the .nvda-addon file using zipfile or archive tools
- **NEVER** manually edit `addon/manifest.ini` - it's auto-generated from `buildVars.py`
- **NEVER** manually edit translation files in `addon/locale/*/manifest.ini` - they are auto-generated
