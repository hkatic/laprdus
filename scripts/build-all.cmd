@echo off
REM =============================================================================
REM LaprdusTTS Master Build Script (Windows)
REM =============================================================================
REM This script automates the complete build process for all platforms.
REM It ensures voice data, dictionaries, and libraries are built and copied
REM in the correct order.
REM
REM The engine and every program are built by Cargo; "cargo xtask" does the
REM steps Cargo does not do by itself (voice data, staging).
REM
REM Usage:
REM   scripts\build-all.cmd                    Build all platforms
REM   scripts\build-all.cmd sapi5             Build Windows SAPI5 only
REM   scripts\build-all.cmd nvda              Build NVDA addon only
REM   scripts\build-all.cmd android           Build Android APK only
REM   scripts\build-all.cmd linux             Build Linux only (needs a Linux host)
REM   scripts\build-all.cmd voice-data        Generate voice data only
REM
REM =============================================================================

setlocal enabledelayedexpansion

REM Project root directory
set "PROJECT_ROOT=%~dp0.."
cd /d "%PROJECT_ROOT%"

REM Default target
set "TARGET=%1"
if "%TARGET%"=="" set "TARGET=all"

echo ==============================================
echo   LaprdusTTS Master Build Script (Windows)
echo ==============================================
echo.

REM Check prerequisites
echo [INFO] Checking prerequisites...
where cargo >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Cargo not found. Install Rust from https://rustup.rs
    exit /b 1
)
echo [SUCCESS] Prerequisites check passed

REM Route to appropriate build target
if "%TARGET%"=="voice-data" goto :voice_data
if "%TARGET%"=="sapi5" goto :sapi5
if "%TARGET%"=="nvda" goto :nvda
if "%TARGET%"=="android" goto :android
if "%TARGET%"=="linux" goto :linux
if "%TARGET%"=="all" goto :all

echo [ERROR] Unknown target: %TARGET%
echo Usage: %0 [all^|sapi5^|nvda^|android^|linux^|voice-data]
exit /b 1

REM A failed step ends the build: "call" alone would go on to the next one.
:voice_data
call :generate_voice_data || exit /b 1
goto :end

:sapi5
call :generate_voice_data || exit /b 1
call :build_sapi5_dlls || exit /b 1
REM The installer carries the NVDA add-on: the add-on is built first.
call :build_nvda_addon || exit /b 1
call :build_sapi5_installer || exit /b 1
goto :end

:nvda
call :generate_voice_data || exit /b 1
call :build_sapi5_dlls || exit /b 1
call :build_nvda_addon || exit /b 1
goto :end

:android
call :generate_voice_data || exit /b 1
call :build_android || exit /b 1
goto :end

:linux
call :build_linux
goto :end

:all
call :generate_voice_data || exit /b 1
call :build_sapi5_dlls || exit /b 1
call :build_nvda_addon || exit /b 1
call :build_sapi5_installer || exit /b 1
call :build_linux
call :build_android || exit /b 1
goto :end

REM =============================================================================
REM Subroutines
REM =============================================================================

:generate_voice_data
echo.
echo [INFO] Step 1: Generating voice data...

REM Packs data\<language>\<Voice>\*.wav into data\voices\<Voice>.bin and
REM stages the dictionaries in data\dictionary.
cargo xtask data
if %errorlevel% neq 0 (
    echo [ERROR] Failed to generate voice data
    exit /b 1
)

REM Verify voice data was created
if not exist "%PROJECT_ROOT%\data\voices\Josip.bin" (
    echo [ERROR] Josip.bin not found
    exit /b 1
)
if not exist "%PROJECT_ROOT%\data\voices\Vlado.bin" (
    echo [ERROR] Vlado.bin not found
    exit /b 1
)

echo [SUCCESS] Voice data generated: Josip.bin, Vlado.bin
exit /b 0

:build_sapi5_dlls
echo.
echo [INFO] Step 2: Building Windows SAPI5 DLLs...

REM The SAPI5 DLLs, the CLI and the Configurator for x64 and x86, staged in
REM dist\windows-x64 and dist\windows-x86. Needs the MSVC build tools and
REM   rustup target add x86_64-pc-windows-msvc i686-pc-windows-msvc
cargo xtask windows
if %errorlevel% neq 0 (
    echo [ERROR] Failed to build the Windows programs
    exit /b 1
)

REM Verify DLLs
if not exist "%PROJECT_ROOT%\dist\windows-x64\laprd64.dll" (
    echo [ERROR] laprd64.dll not found
    exit /b 1
)
if not exist "%PROJECT_ROOT%\dist\windows-x86\laprd32.dll" (
    echo [ERROR] laprd32.dll not found
    exit /b 1
)

REM The same programs for Windows 7 SP1, 8 and 8.1, staged in
REM dist\windows-legacy-x64 and dist\windows-legacy-x86; the installer and the
REM NVDA add-on carry both builds. Needs the nightly toolchain named in
REM src\platform\windows\legacy\toolchain.txt, with the component rust-src
cargo xtask windows-legacy
if %errorlevel% neq 0 (
    echo [ERROR] Failed to build the Windows programs for Windows 7, 8 and 8.1
    exit /b 1
)
if not exist "%PROJECT_ROOT%\dist\windows-legacy-x64\laprd64.dll" (
    echo [ERROR] the legacy laprd64.dll not found
    exit /b 1
)
if not exist "%PROJECT_ROOT%\dist\windows-legacy-x86\laprd32.dll" (
    echo [ERROR] the legacy laprd32.dll not found
    exit /b 1
)

echo [SUCCESS] SAPI5 DLLs built successfully
exit /b 0

:build_sapi5_installer
echo.
echo [INFO] Step 3: Building SAPI5 Installer...

set "ISCC=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
REM Quote %ISCC% inside the block below: the closing parenthesis of "(x86)"
REM in the path would end the block.
if not exist "%ISCC%" (
    echo [WARNING] InnoSetup not found at "%ISCC%"
    echo [WARNING] Skipping SAPI5 installer build
    exit /b 0
)

"%ISCC%" "%PROJECT_ROOT%\installers\windows\laprdus_sapi5.iss"
if %errorlevel% neq 0 (
    echo [ERROR] Failed to build SAPI5 Installer
    exit /b 1
)

if not exist "%PROJECT_ROOT%\installers\windows\Output\Laprdus_SAPI5_Setup_*.exe" (
    echo [ERROR] Installer not found
    exit /b 1
)

echo [SUCCESS] SAPI5 Installer built successfully
exit /b 0

:build_nvda_addon
echo.
echo [INFO] Step 4: Building NVDA Addon...

REM The add-on has its own SCons build (the NVDA add-on template); it takes
REM the DLLs and the Configurator from dist\windows-*, the voice data from
REM data\voices and the dictionaries from data\dictionary by itself.
where scons >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] SCons not found ^(the NVDA add-on is built with it^). Install with: pip install scons
    exit /b 1
)

cd /d "%PROJECT_ROOT%\src\platform\windows\nvda"
call scons
if %errorlevel% neq 0 (
    echo [ERROR] Failed to build NVDA Addon
    cd /d "%PROJECT_ROOT%"
    exit /b 1
)
cd /d "%PROJECT_ROOT%"

echo [SUCCESS] NVDA Addon built
exit /b 0

:build_linux
echo.
echo [INFO] Step 5: Linux components...

REM The Linux programs are built on Linux (or in WSL) with
REM   ./scripts/build-all.sh linux
echo [WARNING] Linux components are not built on Windows. On Linux or in WSL run: ./scripts/build-all.sh linux
exit /b 0

:build_android
echo.
echo [INFO] Step 6: Building Android APK...

REM Set JAVA_HOME if not set
if "%JAVA_HOME%"=="" (
    if exist "C:\Program Files\Android\Android Studio\jbr" (
        set "JAVA_HOME=C:\Program Files\Android\Android Studio\jbr"
    )
)

REM Gradle packs the voice data and builds the native library itself
REM (cargo xtask data, cargo ndk).
cd /d "%PROJECT_ROOT%\src\platform\android"
REM .\ so that the wrapper is found even where cmd does not search the
REM current directory (NoDefaultCurrentDirectoryInExePath).
call .\gradlew.bat assembleDebug
if %errorlevel% neq 0 (
    echo [ERROR] Failed to build Android APK
    cd /d "%PROJECT_ROOT%"
    exit /b 1
)
cd /d "%PROJECT_ROOT%"

if not exist "%PROJECT_ROOT%\src\platform\android\app\build\outputs\apk\debug\app-debug.apk" (
    echo [ERROR] APK not found
    exit /b 1
)

echo [SUCCESS] Android APK built successfully
exit /b 0

:end
echo.
echo [SUCCESS] Build completed successfully!
endlocal
