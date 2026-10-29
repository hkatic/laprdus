; -*- coding: utf-8 -*-
; LaprdusTTS SAPI5 Unified Installer
; Installs both 32-bit and 64-bit DLLs in a single directory
; Voice data is shared between architectures to avoid duplication
; Registers one SAPI5 voice per language (Laprdus Croatian, Laprdus Serbian,
; Laprdus Bosnian, Laprdus Slovenian); each speaks with the voice chosen in the Laprdus
; Configurator (Zvonko unless changed). Every Laprdus voice speaks every
; language. Tokens of earlier versions (one per voice) are removed on
; install, upgrade and repair.
; Works with NVDA, Balabolka, and all SAPI5-compatible applications

#define AppName "Laprdus"
#define AppVersion "2.0.0"
#define AppPublisher "Hrvoje Katić"
#define AppURL "https://hrvojekatic.com/laprdus"

; Single CLSID for both architectures - Windows handles routing via registry virtualization
#define CLSID "D4B5A4E1-8B5C-4C5D-9E6F-1A2B3C4D5E6F"

; Language codes
#define LANG_CROATIAN "41A"
#define LANG_SERBIAN "81A"
#define LANG_BOSNIAN "141A"
#define LANG_SLOVENIAN "424"

[Setup]
AppId={{{#CLSID}}
AppName={#AppName} SAPI5 Voices
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#AppURL}
; Use single install directory for all architectures
DefaultDirName={commonpf}\{#AppName}
DefaultGroupName=Laprdus
OutputBaseFilename=Laprdus_SAPI5_Setup_{#AppVersion}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
; Always show directory page, but pre-fill with previous location if upgrading
DisableDirPage=no
UsePreviousAppDir=yes
; Allow running on both 32-bit and 64-bit Windows
ArchitecturesAllowed=x86compatible x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
; Close applications that are using Laprdus files
CloseApplications=force
CloseApplicationsFilter=*.dll,*.exe
RestartApplications=yes

[Languages]
Name: "croatian"; MessagesFile: "Croatian.isl"
Name: "serbian"; MessagesFile: "Serbian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[CustomMessages]
; English defaults
english.LaprdusDesktopShortcut=Create desktop shortcut
english.LaprdusAdditionalOptions=Additional options:
english.LaprdusConfiguratorName=Laprdus Configurator
english.LaprdusConfiguratorComment=Configure Laprdus settings
english.LaprdusWebsiteName=Laprdus Website
english.LaprdusWebsiteComment=Visit Laprdus website
english.LaprdusUninstallName=Uninstall Laprdus
english.LaprdusUninstallComment=Remove Laprdus from your computer
english.LaprdusOldVersionFound=A previous version of Laprdus is already installed. Do you want to uninstall it before continuing?%n%nClick Yes to uninstall the previous version or No to cancel installation.
english.LaprdusUpgradeFound=A previous version of Laprdus is already installed. The installer will upgrade your existing installation.%n%nClick Yes to continue with the upgrade or No to cancel.

[Files]
; ============================================================================
; DLLs - both architectures in same directory
; ============================================================================
; 32-bit DLL - always installed (for 32-bit apps like NVDA)
Source: "..\..\build\windows-x86-release\laprd32.dll"; DestDir: "{app}"; Flags: ignoreversion
; 64-bit DLL - only on 64-bit Windows
Source: "..\..\build\windows-x64-release\laprd64.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode

; ============================================================================
; CLI executable
; - 64-bit: statically linked (no extra DLL needed)
; - 32-bit: dynamically linked (requires laprdus.dll to avoid Defender false positive)
; ============================================================================
; 64-bit CLI - statically linked
Source: "..\..\build\windows-x64-release\laprdus.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
; 32-bit CLI - dynamically linked (needs laprdus.dll)
Source: "..\..\build\windows-x86-release\laprdus.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "..\..\build\windows-x86-release\laprdus.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode

; ============================================================================
; Configuration GUI
; ============================================================================
; 64-bit config GUI
Source: "..\..\build\windows-x64-release\laprdgui.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
; 32-bit config GUI
Source: "..\..\build\windows-x86-release\laprdgui.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode

; ============================================================================
; Voice data - in separate voices subdirectory
; ============================================================================
Source: "..\..\data\voices\Josip.bin"; DestDir: "{app}\voices"; Flags: ignoreversion
Source: "..\..\data\voices\Vlado.bin"; DestDir: "{app}\voices"; Flags: ignoreversion

; ============================================================================
; Dictionary - in separate dictionaries subdirectory
; ============================================================================
Source: "..\..\data\dictionary\internal.json"; DestDir: "{app}\dictionaries"; DestName: "dictionary.json"; Flags: ignoreversion
Source: "..\..\data\dictionary\spelling.json"; DestDir: "{app}\dictionaries"; Flags: ignoreversion
Source: "..\..\data\dictionary\emoji.json"; DestDir: "{app}\dictionaries"; Flags: ignoreversion

[InstallDelete]
; Remove old voice data files from root directory (pre-1.0.0 structure)
Type: files; Name: "{app}\Josip.bin"
Type: files; Name: "{app}\Vlado.bin"
; Remove old dictionary files from root directory (pre-1.0.0 structure)
Type: files; Name: "{app}\dictionary.json"
Type: files; Name: "{app}\spelling.json"
Type: files; Name: "{app}\emoji.json"
; Remove old "LaprdusTTS" directory contents during upgrade (directory renamed to "Laprdus")
Type: files; Name: "{commonpf}\LaprdusTTS\laprd32.dll"
Type: files; Name: "{commonpf}\LaprdusTTS\laprd64.dll"
Type: files; Name: "{commonpf}\LaprdusTTS\laprdus.exe"
Type: files; Name: "{commonpf}\LaprdusTTS\laprdus.dll"
Type: files; Name: "{commonpf}\LaprdusTTS\laprdgui.exe"
Type: files; Name: "{commonpf}\LaprdusTTS\voices\Josip.bin"
Type: files; Name: "{commonpf}\LaprdusTTS\voices\Vlado.bin"
Type: filesandordirs; Name: "{commonpf}\LaprdusTTS\voices"
Type: files; Name: "{commonpf}\LaprdusTTS\dictionaries\dictionary.json"
Type: files; Name: "{commonpf}\LaprdusTTS\dictionaries\spelling.json"
Type: files; Name: "{commonpf}\LaprdusTTS\dictionaries\emoji.json"
Type: filesandordirs; Name: "{commonpf}\LaprdusTTS\dictionaries"
Type: dirifempty; Name: "{commonpf}\LaprdusTTS"

[Registry]
; ============================================================================
; 32-bit COM CLSID registration
; HKCR32 targets WOW6432Node\CLSID on 64-bit Windows
; ============================================================================
Root: HKCR32; Subkey: "CLSID\{{{#CLSID}}"; ValueType: string; ValueName: ""; ValueData: "LaprdusTTS"; Flags: uninsdeletekey
Root: HKCR32; Subkey: "CLSID\{{{#CLSID}}\InprocServer32"; ValueType: string; ValueName: ""; ValueData: "{app}\laprd32.dll"
Root: HKCR32; Subkey: "CLSID\{{{#CLSID}}\InprocServer32"; ValueType: string; ValueName: "ThreadingModel"; ValueData: "Both"

; ============================================================================
; 64-bit COM CLSID registration (only on 64-bit Windows)
; ============================================================================
Root: HKCR; Subkey: "CLSID\{{{#CLSID}}"; ValueType: string; ValueName: ""; ValueData: "LaprdusTTS"; Flags: uninsdeletekey; Check: Is64BitInstallMode
Root: HKCR; Subkey: "CLSID\{{{#CLSID}}\InprocServer32"; ValueType: string; ValueName: ""; ValueData: "{app}\laprd64.dll"; Check: Is64BitInstallMode
Root: HKCR; Subkey: "CLSID\{{{#CLSID}}\InprocServer32"; ValueType: string; ValueName: "ThreadingModel"; ValueData: "Both"; Check: Is64BitInstallMode

; ============================================================================
; Voice tokens of earlier versions (one per voice): removed on every
; install, upgrade and repair before the tokens below are written. The
; [Code] section also sweeps any other token that points at our CLSID.
; ============================================================================
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusJosip"; Flags: deletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusVlado"; Flags: deletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusDetence"; Flags: deletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusBaba"; Flags: deletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusDjed"; Flags: deletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusZvonko"; Flags: deletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusStojan"; Flags: deletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusMirsad"; Flags: deletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusOrguljas"; Flags: deletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusKlapa"; Flags: deletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusTrubac"; Flags: deletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusHarmonikas"; Flags: deletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSevdalija"; Flags: deletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSazlija"; Flags: deletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusPjevac"; Flags: deletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusPevac"; Flags: deletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSolist"; Flags: deletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusBecarac"; Flags: deletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\Laprdus"; Flags: deletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusTTS"; Flags: deletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusDjedo"; Flags: deletekey
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusJosip"; Flags: deletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusVlado"; Flags: deletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusDetence"; Flags: deletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusBaba"; Flags: deletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusDjed"; Flags: deletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusZvonko"; Flags: deletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusStojan"; Flags: deletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusMirsad"; Flags: deletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusOrguljas"; Flags: deletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusKlapa"; Flags: deletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusTrubac"; Flags: deletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusHarmonikas"; Flags: deletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSevdalija"; Flags: deletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSazlija"; Flags: deletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusPjevac"; Flags: deletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusPevac"; Flags: deletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSolist"; Flags: deletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusBecarac"; Flags: deletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\Laprdus"; Flags: deletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusTTS"; Flags: deletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusDjedo"; Flags: deletekey; Check: Is64BitInstallMode

; ============================================================================
; LANGUAGE 1: Laprdus Croatian - 32-bit
; Speaks with the voice from settings.json (voice.default, Zvonko by default)
; ============================================================================
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusCroatian"; ValueType: string; ValueName: ""; ValueData: "Laprdus Croatian"; Flags: uninsdeletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusCroatian"; ValueType: string; ValueName: "CLSID"; ValueData: "{{{#CLSID}}"
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusCroatian"; ValueType: string; ValueName: "Language"; ValueData: "hr"
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusCroatian\Attributes"; ValueType: string; ValueName: "Language"; ValueData: "{#LANG_CROATIAN}"; Flags: uninsdeletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusCroatian\Attributes"; ValueType: string; ValueName: "Gender"; ValueData: "Male"
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusCroatian\Attributes"; ValueType: string; ValueName: "Age"; ValueData: "Adult"
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusCroatian\Attributes"; ValueType: string; ValueName: "Vendor"; ValueData: "Laprdus"
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusCroatian\Attributes"; ValueType: string; ValueName: "Name"; ValueData: "Laprdus Croatian"

; ============================================================================
; LANGUAGE 1: Laprdus Croatian - 64-bit
; Speaks with the voice from settings.json (voice.default, Zvonko by default)
; ============================================================================
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusCroatian"; ValueType: string; ValueName: ""; ValueData: "Laprdus Croatian"; Flags: uninsdeletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusCroatian"; ValueType: string; ValueName: "CLSID"; ValueData: "{{{#CLSID}}"; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusCroatian"; ValueType: string; ValueName: "Language"; ValueData: "hr"; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusCroatian\Attributes"; ValueType: string; ValueName: "Language"; ValueData: "{#LANG_CROATIAN}"; Flags: uninsdeletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusCroatian\Attributes"; ValueType: string; ValueName: "Gender"; ValueData: "Male"; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusCroatian\Attributes"; ValueType: string; ValueName: "Age"; ValueData: "Adult"; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusCroatian\Attributes"; ValueType: string; ValueName: "Vendor"; ValueData: "Laprdus"; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusCroatian\Attributes"; ValueType: string; ValueName: "Name"; ValueData: "Laprdus Croatian"; Check: Is64BitInstallMode

; ============================================================================
; LANGUAGE 2: Laprdus Serbian - 32-bit
; Speaks with the voice from settings.json (voice.default, Zvonko by default)
; ============================================================================
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSerbian"; ValueType: string; ValueName: ""; ValueData: "Laprdus Serbian"; Flags: uninsdeletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSerbian"; ValueType: string; ValueName: "CLSID"; ValueData: "{{{#CLSID}}"
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSerbian"; ValueType: string; ValueName: "Language"; ValueData: "sr"
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSerbian\Attributes"; ValueType: string; ValueName: "Language"; ValueData: "{#LANG_SERBIAN}"; Flags: uninsdeletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSerbian\Attributes"; ValueType: string; ValueName: "Gender"; ValueData: "Male"
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSerbian\Attributes"; ValueType: string; ValueName: "Age"; ValueData: "Adult"
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSerbian\Attributes"; ValueType: string; ValueName: "Vendor"; ValueData: "Laprdus"
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSerbian\Attributes"; ValueType: string; ValueName: "Name"; ValueData: "Laprdus Serbian"

; ============================================================================
; LANGUAGE 2: Laprdus Serbian - 64-bit
; Speaks with the voice from settings.json (voice.default, Zvonko by default)
; ============================================================================
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSerbian"; ValueType: string; ValueName: ""; ValueData: "Laprdus Serbian"; Flags: uninsdeletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSerbian"; ValueType: string; ValueName: "CLSID"; ValueData: "{{{#CLSID}}"; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSerbian"; ValueType: string; ValueName: "Language"; ValueData: "sr"; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSerbian\Attributes"; ValueType: string; ValueName: "Language"; ValueData: "{#LANG_SERBIAN}"; Flags: uninsdeletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSerbian\Attributes"; ValueType: string; ValueName: "Gender"; ValueData: "Male"; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSerbian\Attributes"; ValueType: string; ValueName: "Age"; ValueData: "Adult"; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSerbian\Attributes"; ValueType: string; ValueName: "Vendor"; ValueData: "Laprdus"; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSerbian\Attributes"; ValueType: string; ValueName: "Name"; ValueData: "Laprdus Serbian"; Check: Is64BitInstallMode

; ============================================================================
; LANGUAGE 3: Laprdus Bosnian - 32-bit
; Speaks with the voice from settings.json (voice.default, Zvonko by default)
; ============================================================================
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusBosnian"; ValueType: string; ValueName: ""; ValueData: "Laprdus Bosnian"; Flags: uninsdeletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusBosnian"; ValueType: string; ValueName: "CLSID"; ValueData: "{{{#CLSID}}"
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusBosnian"; ValueType: string; ValueName: "Language"; ValueData: "bs"
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusBosnian\Attributes"; ValueType: string; ValueName: "Language"; ValueData: "{#LANG_BOSNIAN}"; Flags: uninsdeletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusBosnian\Attributes"; ValueType: string; ValueName: "Gender"; ValueData: "Male"
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusBosnian\Attributes"; ValueType: string; ValueName: "Age"; ValueData: "Adult"
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusBosnian\Attributes"; ValueType: string; ValueName: "Vendor"; ValueData: "Laprdus"
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusBosnian\Attributes"; ValueType: string; ValueName: "Name"; ValueData: "Laprdus Bosnian"

; ============================================================================
; LANGUAGE 3: Laprdus Bosnian - 64-bit
; Speaks with the voice from settings.json (voice.default, Zvonko by default)
; ============================================================================
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusBosnian"; ValueType: string; ValueName: ""; ValueData: "Laprdus Bosnian"; Flags: uninsdeletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusBosnian"; ValueType: string; ValueName: "CLSID"; ValueData: "{{{#CLSID}}"; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusBosnian"; ValueType: string; ValueName: "Language"; ValueData: "bs"; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusBosnian\Attributes"; ValueType: string; ValueName: "Language"; ValueData: "{#LANG_BOSNIAN}"; Flags: uninsdeletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusBosnian\Attributes"; ValueType: string; ValueName: "Gender"; ValueData: "Male"; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusBosnian\Attributes"; ValueType: string; ValueName: "Age"; ValueData: "Adult"; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusBosnian\Attributes"; ValueType: string; ValueName: "Vendor"; ValueData: "Laprdus"; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusBosnian\Attributes"; ValueType: string; ValueName: "Name"; ValueData: "Laprdus Bosnian"; Check: Is64BitInstallMode

; ============================================================================
; LANGUAGE 4: Laprdus Slovenian - 32-bit
; Speaks with the voice from settings.json (voice.default, Zvonko by default)
; ============================================================================
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSlovenian"; ValueType: string; ValueName: ""; ValueData: "Laprdus Slovenian"; Flags: uninsdeletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSlovenian"; ValueType: string; ValueName: "CLSID"; ValueData: "{{{#CLSID}}"
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSlovenian"; ValueType: string; ValueName: "Language"; ValueData: "sl"
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSlovenian\Attributes"; ValueType: string; ValueName: "Language"; ValueData: "{#LANG_SLOVENIAN}"; Flags: uninsdeletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSlovenian\Attributes"; ValueType: string; ValueName: "Gender"; ValueData: "Male"
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSlovenian\Attributes"; ValueType: string; ValueName: "Age"; ValueData: "Adult"
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSlovenian\Attributes"; ValueType: string; ValueName: "Vendor"; ValueData: "Laprdus"
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSlovenian\Attributes"; ValueType: string; ValueName: "Name"; ValueData: "Laprdus Slovenian"

; ============================================================================
; LANGUAGE 4: Laprdus Slovenian - 64-bit
; Speaks with the voice from settings.json (voice.default, Zvonko by default)
; ============================================================================
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSlovenian"; ValueType: string; ValueName: ""; ValueData: "Laprdus Slovenian"; Flags: uninsdeletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSlovenian"; ValueType: string; ValueName: "CLSID"; ValueData: "{{{#CLSID}}"; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSlovenian"; ValueType: string; ValueName: "Language"; ValueData: "sl"; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSlovenian\Attributes"; ValueType: string; ValueName: "Language"; ValueData: "{#LANG_SLOVENIAN}"; Flags: uninsdeletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSlovenian\Attributes"; ValueType: string; ValueName: "Gender"; ValueData: "Male"; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSlovenian\Attributes"; ValueType: string; ValueName: "Age"; ValueData: "Adult"; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSlovenian\Attributes"; ValueType: string; ValueName: "Vendor"; ValueData: "Laprdus"; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSlovenian\Attributes"; ValueType: string; ValueName: "Name"; ValueData: "Laprdus Slovenian"; Check: Is64BitInstallMode

[Tasks]
; Optional desktop shortcut
Name: "desktopicon"; Description: "{cm:LaprdusDesktopShortcut}"; GroupDescription: "{cm:LaprdusAdditionalOptions}"; Flags: unchecked

[Icons]
; Start Menu shortcuts - always recreate on reinstall
Name: "{group}\{cm:LaprdusConfiguratorName}"; Filename: "{app}\laprdgui.exe"; Comment: "{cm:LaprdusConfiguratorComment}"
Name: "{group}\{cm:LaprdusWebsiteName}"; Filename: "https://hrvojekatic.com/laprdus"; Comment: "{cm:LaprdusWebsiteComment}"
Name: "{group}\{cm:LaprdusUninstallName}"; Filename: "{uninstallexe}"; Comment: "{cm:LaprdusUninstallComment}"
; Desktop shortcut (optional)
Name: "{commondesktop}\{cm:LaprdusConfiguratorName}"; Filename: "{app}\laprdgui.exe"; Comment: "{cm:LaprdusConfiguratorComment}"; Tasks: desktopicon

[Code]
// Get the uninstall string from registry
function GetUninstallString(): String;
var
  sUnInstPath: String;
  sUnInstallString: String;
begin
  // Note: In Pascal code, { does NOT need escaping (unlike .iss file sections)
  sUnInstPath := 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{D4B5A4E1-8B5C-4C5D-9E6F-1A2B3C4D5E6F}_is1';
  sUnInstallString := '';
  // Check 64-bit registry first (for 64-bit installations)
  if not RegQueryStringValue(HKLM64, sUnInstPath, 'UninstallString', sUnInstallString) then
    // Then check 32-bit registry
    if not RegQueryStringValue(HKLM32, sUnInstPath, 'UninstallString', sUnInstallString) then
      // Finally check current user
      RegQueryStringValue(HKCU, sUnInstPath, 'UninstallString', sUnInstallString);
  Result := sUnInstallString;
end;

// Check if this is an upgrade (previous version exists)
function IsUpgrade(): Boolean;
begin
  Result := (GetUninstallString() <> '');
end;

// Uninstall previous version silently
function UnInstallOldVersion(): Integer;
var
  sUnInstallString: String;
  iResultCode: Integer;
begin
  Result := 0;
  sUnInstallString := GetUninstallString();
  if sUnInstallString <> '' then begin
    sUnInstallString := RemoveQuotes(sUnInstallString);
    if Exec(sUnInstallString, '/SILENT /NORESTART /SUPPRESSMSGBOXES', '', SW_HIDE, ewWaitUntilTerminated, iResultCode) then
      Result := 3
    else
      Result := 2;
  end else
    Result := 1;
end;

// Voice tokens this version registers (one per language)
function IsCurrentToken(const Name: String): Boolean;
begin
  Result := (CompareText(Name, 'LaprdusCroatian') = 0) or
            (CompareText(Name, 'LaprdusSerbian') = 0) or
            (CompareText(Name, 'LaprdusBosnian') = 0) or
            (CompareText(Name, 'LaprdusSlovenian') = 0);
end;

// Remove every SAPI5 voice token under RootKey that points at our CLSID and
// is not one of the tokens of this version: tokens of earlier versions (one
// per voice, under names that may no longer be known here), or tokens left
// over from a failed registration. KeepCurrent = False removes ours too
// (uninstall).
procedure RemoveLaprdusTokens(RootKey: Integer; KeepCurrent: Boolean);
var
  Tokens: String;
  Names: TArrayOfString;
  Clsid: String;
  I: Integer;
begin
  Tokens := 'SOFTWARE\Microsoft\Speech\Voices\Tokens';
  if not RegGetSubkeyNames(RootKey, Tokens, Names) then
    Exit;
  for I := 0 to GetArrayLength(Names) - 1 do
  begin
    if KeepCurrent and IsCurrentToken(Names[I]) then
      Continue;
    Clsid := '';
    if RegQueryStringValue(RootKey, Tokens + '\' + Names[I], 'CLSID', Clsid) then
    begin
      if CompareText(Clsid, '{D4B5A4E1-8B5C-4C5D-9E6F-1A2B3C4D5E6F}') = 0 then
      begin
        Log('Removing Laprdus voice token ' + Names[I]);
        RegDeleteKeyIncludingSubkeys(RootKey, Tokens + '\' + Names[I]);
      end;
    end;
  end;
end;

procedure RemoveAllLaprdusTokens(KeepCurrent: Boolean);
begin
  RemoveLaprdusTokens(HKLM32, KeepCurrent);
  if IsWin64 then
    RemoveLaprdusTokens(HKLM64, KeepCurrent);
end;

// Before the files and registry entries are written (install, upgrade and
// repair alike): drop the voice tokens of earlier versions, so that only
// the four language tokens remain registered afterwards.
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssInstall then
    RemoveAllLaprdusTokens(True);
end;

// On uninstall: drop every Laprdus token, including any an earlier version
// left behind.
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then
    RemoveAllLaprdusTokens(False);
end;

function InitializeSetup(): Boolean;
var
  Version: TWindowsVersion;
  InstallDir: String;
begin
  Result := True;
  // Check for Windows 7 or later (version 6.1)
  GetWindowsVersionEx(Version);
  if (Version.Major < 6) or ((Version.Major = 6) and (Version.Minor < 1)) then
  begin
    MsgBox('LaprdusTTS requires Windows 7 or later.', mbError, MB_OK);
    Result := False;
    Exit;
  end;

  // Check for previous installation via registry
  if IsUpgrade() then
  begin
    // Inform the user about upgrade - but don't call uninstaller!
    // InnoSetup will overwrite files and update registry entries.
    // CloseApplications=force will handle any locked files.
    // NOT calling UnInstallOldVersion() prevents Start Menu shortcuts from disappearing.
    if MsgBox(CustomMessage('LaprdusUpgradeFound'), mbConfirmation, MB_YESNO) = IDNO then
    begin
      Result := False;
      Exit;
    end;
  end
  else
  begin
    // No registry entry found - check if there's an orphaned empty folder
    // and remove it to avoid confusing "folder exists" warning
    // Check both new "Laprdus" and old "LaprdusTTS" folder names
    InstallDir := ExpandConstant('{commonpf}\Laprdus');
    if DirExists(InstallDir) then
    begin
      // Try to remove if empty (will fail silently if not empty, which is fine)
      RemoveDir(InstallDir);
    end;
    // Also check and clean up old "LaprdusTTS" folder if empty
    InstallDir := ExpandConstant('{commonpf}\LaprdusTTS');
    if DirExists(InstallDir) then
    begin
      RemoveDir(InstallDir);
    end;
  end;
end;
