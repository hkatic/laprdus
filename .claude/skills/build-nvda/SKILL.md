---
name: build-nvda
description: Build NVDA screen reader addon package for LaprdusTTS
allowed-tools: Bash, Read
---

# Build NVDA Addon

Build the NVDA screen reader synthesizer addon for LaprdusTTS.

## Prerequisites

The NVDA addon requires the following to be built first:
- SAPI5 DLLs (both x64 and x86)
- Config GUI executables (both x64 and x86)

The build process will automatically copy them.

## Build Steps

Execute these commands in sequence:

### 1. Build x64 SAPI5 DLL
```bash
scons --platform=windows --arch=x64 --build-config=release sapi5
```

### 2. Build x86 SAPI5 DLL
```bash
scons --platform=windows --arch=x86 --build-config=release sapi5
```

### 3. Build x64 Config GUI
```bash
scons --platform=windows --arch=x64 --build-config=release config
```

### 4. Build x86 Config GUI
```bash
scons --platform=windows --arch=x86 --build-config=release config
```

### 5. Build NVDA Addon
```bash
cd nvda-addon && scons && cd ..
```

## Expected Output

- Addon package: `nvda-addon/laprdus-1.0.0.nvda-addon`

## Automatic Resource Copying

The NVDA addon SCons build automatically:
- Copies DLLs (`laprd64.dll`, `laprd32.dll`) from build directories
- Copies GUI executables (`laprdgui.exe` as `laprdgui64.exe` and `laprdgui32.exe`) for the Laprdus Configurator
- Copies voice data from `data/voices/` to `addon/synthDrivers/laprdus/voices/`
- Copies dictionaries from `data/dictionary/` to `addon/synthDrivers/laprdus/dictionaries/`

## NVDA Menu Integration

The addon includes a globalPlugin that adds a "Laprdus" submenu to NVDA's Tools menu with:
- "Laprdus Configurator..." - Opens the configuration GUI (launches the correct 32/64-bit exe based on system architecture)
- "Laprdus on the Web" - Opens the Laprdus website in the default browser

## Verification

After building, verify the addon package exists:
```bash
ls -la nvda-addon/laprdus-*.nvda-addon
```

## Launch Addon Installer for Testing

```bash
start "" "nvda-addon/laprdus-1.0.0.nvda-addon"
```

## CRITICAL Build Rules

- **ALWAYS** use SCons to build the NVDA addon
- **NEVER** manually create the .nvda-addon file using zipfile or archive tools
- **NEVER** manually edit `addon/manifest.ini` - it's auto-generated from `buildVars.py`
- **NEVER** manually edit translation files in `addon/locale/*/manifest.ini` - they are auto-generated
