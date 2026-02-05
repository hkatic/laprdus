---
name: build-sapi
description: Build Windows SAPI5 DLLs (x86 and x64) and InnoSetup installer for LaprdusTTS
allowed-tools: Bash, Read
---

# Build Windows SAPI5

Build the Windows SAPI5 text-to-speech driver with both 32-bit and 64-bit DLLs, then create the InnoSetup installer.

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

### 3. Build InnoSetup Installer
```bash
"/c/Program Files (x86)/Inno Setup 6/ISCC.exe" installers/windows/laprdus_sapi5.iss
```

## Expected Output

- DLLs:
  - `build/windows-x64-release/laprd64.dll`
  - `build/windows-x86-release/laprd32.dll`
- Installer: `installers/windows/Output/Laprdus_SAPI5_Setup_1.0.0.exe`

## Voice Data

The build automatically generates voice data (`data/voices/Josip.bin`, `data/voices/Vlado.bin`) if not present.

## Verification

After building, verify the installer exists:
```bash
ls -la installers/windows/Output/Laprdus_SAPI5_Setup_1.0.0.exe
```

## Launch Installer for Testing

```bash
start "" "installers/windows/Output/Laprdus_SAPI5_Setup_1.0.0.exe"
```
