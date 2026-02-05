@echo off
REM =============================================================================
REM LaprdusTTS Master Build Script (Windows)
REM =============================================================================
REM This script automates the complete build process for all platforms.
REM It ensures voice data, dictionaries, and libraries are built and copied
REM in the correct order.
REM
REM Usage:
REM   scripts\build-all.cmd                    Build all platforms
REM   scripts\build-all.cmd sapi5             Build Windows SAPI5 only
REM   scripts\build-all.cmd nvda              Build NVDA addon only
REM   scripts\build-all.cmd android           Build Android APK only
REM   scripts\build-all.cmd linux             Build Linux only
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
where scons >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] SCons not found. Install with: pip install scons
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

:voice_data
call :generate_voice_data
goto :end

:sapi5
call :generate_voice_data
call :build_sapi5_dlls
call :build_sapi5_installer
goto :end

:nvda
call :generate_voice_data
call :build_sapi5_dlls
call :copy_dlls_to_nvda
call :build_nvda_addon
goto :end

:android
call :generate_voice_data
call :build_android
goto :end

:linux
call :generate_voice_data
call :build_linux
goto :end

:all
call :generate_voice_data
call :build_sapi5_dlls
call :build_sapi5_installer
call :copy_dlls_to_nvda
call :build_nvda_addon
call :build_linux
call :build_android
goto :end

REM =============================================================================
REM Subroutines
REM =============================================================================

:generate_voice_data
echo.
echo [INFO] Step 1: Generating voice data...

REM Ensure data/voices directory exists
if not exist "%PROJECT_ROOT%\data\voices" mkdir "%PROJECT_ROOT%\data\voices"

REM Build phoneme packer and pack voice data
scons --platform=windows --arch=x64 --build-config=release voice-data
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

echo   Building x64 DLL...
scons --platform=windows --arch=x64 --build-config=release sapi5
if %errorlevel% neq 0 (
    echo [ERROR] Failed to build x64 DLL
    exit /b 1
)

echo   Building x86 DLL...
scons --platform=windows --arch=x86 --build-config=release sapi5
if %errorlevel% neq 0 (
    echo [ERROR] Failed to build x86 DLL
    exit /b 1
)

REM Verify DLLs
if not exist "%PROJECT_ROOT%\build\windows-x64-release\laprd64.dll" (
    echo [ERROR] laprd64.dll not found
    exit /b 1
)
if not exist "%PROJECT_ROOT%\build\windows-x86-release\laprd32.dll" (
    echo [ERROR] laprd32.dll not found
    exit /b 1
)

echo [SUCCESS] SAPI5 DLLs built successfully
exit /b 0

:build_sapi5_installer
echo.
echo [INFO] Step 3: Building SAPI5 Installer...

set "ISCC=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if not exist "%ISCC%" (
    echo [WARNING] InnoSetup not found at %ISCC%
    echo [WARNING] Skipping SAPI5 installer build
    exit /b 0
)

"%ISCC%" "%PROJECT_ROOT%\installers\windows\laprdus_sapi5.iss"
if %errorlevel% neq 0 (
    echo [ERROR] Failed to build SAPI5 Installer
    exit /b 1
)

if not exist "%PROJECT_ROOT%\installers\windows\Output\Laprdus_SAPI5_Setup_1.0.0.exe" (
    echo [ERROR] Installer not found
    exit /b 1
)

echo [SUCCESS] SAPI5 Installer built successfully
exit /b 0

:copy_dlls_to_nvda
echo.
echo [INFO] Step 4: Copying DLLs to NVDA addon directory...

set "NVDA_DIR=%PROJECT_ROOT%\nvda-addon\addon\synthDrivers\laprdus"
if not exist "%NVDA_DIR%" mkdir "%NVDA_DIR%"

copy /y "%PROJECT_ROOT%\build\windows-x64-release\laprd64.dll" "%NVDA_DIR%\" >nul
copy /y "%PROJECT_ROOT%\build\windows-x86-release\laprd32.dll" "%NVDA_DIR%\" >nul

echo [SUCCESS] DLLs copied to NVDA addon
exit /b 0

:build_nvda_addon
echo.
echo [INFO] Step 5: Building NVDA Addon...

cd /d "%PROJECT_ROOT%\nvda-addon"
scons
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
echo [INFO] Step 6: Building Linux components...

scons --platform=linux --arch=x64 --build-config=release linux-all
if %errorlevel% neq 0 (
    echo [WARNING] Linux build may have failed (expected on Windows)
)

echo [INFO] Linux build step completed
exit /b 0

:build_android
echo.
echo [INFO] Step 7: Building Android APK...

REM Set JAVA_HOME if not set
if "%JAVA_HOME%"=="" (
    if exist "C:\Program Files\Android\Android Studio\jbr" (
        set "JAVA_HOME=C:\Program Files\Android\Android Studio\jbr"
    )
)

cd /d "%PROJECT_ROOT%\android"
call gradlew.bat assembleDebug
if %errorlevel% neq 0 (
    echo [ERROR] Failed to build Android APK
    cd /d "%PROJECT_ROOT%"
    exit /b 1
)
cd /d "%PROJECT_ROOT%"

if not exist "%PROJECT_ROOT%\android\app\build\outputs\apk\debug\app-debug.apk" (
    echo [ERROR] APK not found
    exit /b 1
)

echo [SUCCESS] Android APK built successfully
exit /b 0

:end
echo.
echo [SUCCESS] Build completed successfully!
endlocal
