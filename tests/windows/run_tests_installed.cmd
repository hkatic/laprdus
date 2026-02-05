@echo off
REM Run Windows CLI tests against the installed version
setlocal

set PROJECT_ROOT=C:\Users\hrvoj\Sync\Programming\Projects\LaprdusTTS
set LAPRDUS_CLI=C:\Program Files\Laprdus\laprdus.exe
set LAPRDUS_DATA=C:\Program Files\Laprdus\voices
set PATH=%PROJECT_ROOT%\build\windows-x64-release;%PATH%

echo Testing against INSTALLED version
echo LAPRDUS_CLI=%LAPRDUS_CLI%
echo LAPRDUS_DATA=%LAPRDUS_DATA%
echo.

if not exist "%LAPRDUS_CLI%" (
    echo ERROR: Laprdus is not installed at %LAPRDUS_CLI%
    exit /b 1
)

"%PROJECT_ROOT%\build\windows-x64-release\test_cli.exe" %*

endlocal
