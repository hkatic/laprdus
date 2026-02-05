@echo off
REM Run Windows CLI tests against the build directory
setlocal

set PROJECT_ROOT=C:\Users\hrvoj\Sync\Programming\Projects\LaprdusTTS
set LAPRDUS_CLI=%PROJECT_ROOT%\build\windows-x64-release\laprdus.exe
set LAPRDUS_DATA=%PROJECT_ROOT%\build\windows-x64-release
set PATH=%PROJECT_ROOT%\build\windows-x64-release;%PATH%

echo LAPRDUS_CLI=%LAPRDUS_CLI%
echo LAPRDUS_DATA=%LAPRDUS_DATA%
echo.

"%PROJECT_ROOT%\build\windows-x64-release\test_cli.exe" %*

endlocal
