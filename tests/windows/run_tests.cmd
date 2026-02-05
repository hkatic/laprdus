@echo off
REM Windows CLI Test Runner for LaprdusTTS
REM
REM Usage:
REM   run_tests.cmd                    - Run tests against installed version
REM   run_tests.cmd build              - Run tests against build directory
REM

setlocal

REM Get script directory
set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..\..

REM Check for build mode
if "%1"=="build" (
    echo Running tests against build directory...
    set LAPRDUS_CLI=%PROJECT_ROOT%\build\windows-x64-release\laprdus.exe
    set LAPRDUS_DATA=%PROJECT_ROOT%\build\windows-x64-release\voices
    set TEST_EXE=%PROJECT_ROOT%\build\windows-x64-release\test_cli.exe
    set LIB_PATH=%PROJECT_ROOT%\build\windows-x64-release
) else (
    echo Running tests against installed version...
    set LAPRDUS_CLI=C:\Program Files\Laprdus\laprdus.exe
    set LAPRDUS_DATA=C:\Program Files\Laprdus\voices
    set TEST_EXE=%PROJECT_ROOT%\build\windows-x64-release\test_cli.exe
    set LIB_PATH=%PROJECT_ROOT%\build\windows-x64-release
)

REM Check if CLI exists
if not exist "%LAPRDUS_CLI%" (
    echo ERROR: CLI not found at %LAPRDUS_CLI%
    exit /b 1
)

REM Check if data exists
if not exist "%LAPRDUS_DATA%\Josip.bin" (
    echo ERROR: Voice data not found at %LAPRDUS_DATA%
    exit /b 1
)

REM Check if test executable exists
if not exist "%TEST_EXE%" (
    echo ERROR: Test executable not found at %TEST_EXE%
    echo Please build tests first with: scons --platform=windows --arch=x64 --build-config=release test
    exit /b 1
)

REM Add library path to PATH for dynamic linking
set PATH=%LIB_PATH%;%PATH%

echo.
echo LAPRDUS_CLI=%LAPRDUS_CLI%
echo LAPRDUS_DATA=%LAPRDUS_DATA%
echo TEST_EXE=%TEST_EXE%
echo.

REM Run tests
"%TEST_EXE%"

endlocal
