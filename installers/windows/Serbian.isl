; *** Inno Setup version 6.0+ Serbian messages ***
;
; Serbian (Latin) translation for LaprdusTTS installer
; Based on Croatian translation, adapted for Serbian

[LangOptions]
LanguageName=Srpski
LanguageID=$081A
LanguageCodePage=1250

DialogFontName=
[Messages]

; *** Application titles
SetupAppTitle=Instalacija
SetupWindowTitle=Instalacija - %1
UninstallAppTitle=Deinstalacija
UninstallAppFullTitle=Deinstalacija programa %1

; *** Misc. common
InformationTitle=Informacija
ConfirmTitle=Potvrda
ErrorTitle=Greška

; *** SetupLdr messages
SetupLdrStartupMessage=Instaliraćete program %1. Želite li da nastavite?
LdrCannotCreateTemp=Nije moguće kreirati privremenu datoteku. Instalacija je prekinuta
LdrCannotExecTemp=Nije moguće pokrenuti datoteku u privremenom folderu. Instalacija je prekinuta

; *** Startup error messages
LastErrorMessage=%1.%n%nGreška %2: %3
SetupFileMissing=Datoteka %1 nedostaje. Ispravite grešku ili nabavite novu kopiju programa.
SetupFileCorrupt=Instalacione datoteke su oštećene. Nabavite novu kopiju programa.
SetupFileCorruptOrWrongVer=Datoteke su oštećene ili nekompatibilne sa ovom verzijom instalacionog programa. Ispravite grešku ili nabavite novu kopiju programa.
InvalidParameter=Naveden je neispravan parametar komandne linije:%n%n%1
SetupAlreadyRunning=Instalacioni program se već izvršava.
WindowsVersionNotSupported=Program ne radi na vašoj verziji sistema Windows.
WindowsServicePackRequired=Program zahteva %1 sa servisnim paketom %2 ili noviju verziju.
NotOnThisPlatform=Program nije namenjen za upotrebu na %1.
OnlyOnThisPlatform=Program je namenjen samo za upotrebu na %1.
OnlyOnTheseArchitectures=Program možete instalirati samo na Windows sistemima sa sledećim vrstama procesora:%n%n%1
WinVersionTooLowError=Ovaj program zahteva %1 verziju %2 ili noviju.
WinVersionTooHighError=Ovaj program ne možete instalirati na %1 verzije %2 ili novije.
AdminPrivilegesRequired=Za instalaciju programa morate biti prijavljeni kao administrator.
PowerUserPrivilegesRequired=Za instalaciju programa morate biti prijavljeni kao administrator ili power user.
SetupAppRunningError=Program %1 je trenutno otvoren.%n%nZatvorite program, zatim kliknite U redu za nastavak ili Odustani za izlaz.
UninstallAppRunningError=Program %1 je trenutno otvoren.%n%nZatvorite program, zatim kliknite U redu za nastavak ili Odustani za izlaz.

; *** Startup questions
PrivilegesRequiredOverrideTitle=Izaberite način instalacije
PrivilegesRequiredOverrideInstruction=Izaberite način instalacije
PrivilegesRequiredOverrideText1=Program %1 možete instalirati za sve korisnike (potrebna su administratorska ovlašćenja) ili samo za sebe.
PrivilegesRequiredOverrideText2=Program %1 možete instalirati samo za sebe ili za sve korisnike (potrebna su administratorska ovlašćenja).
PrivilegesRequiredOverrideAllUsers=I&nstaliraj za sve korisnike
PrivilegesRequiredOverrideAllUsersRecommended=I&nstaliraj za sve korisnike (preporučeno)
PrivilegesRequiredOverrideCurrentUser=Instaliraj samo za &mene
PrivilegesRequiredOverrideCurrentUserRecommended=Instaliraj samo za &mene (preporučeno)

; *** Misc. errors
ErrorCreatingDir=Instalacioni program nije mogao kreirati folder »%1«
ErrorTooManyFilesInDir=Instalacioni program ne može kreirati novu datoteku u folderu »%1« jer sadrži previše datoteka

; *** Setup common messages
ExitSetupTitle=Prekini instalaciju
ExitSetupMessage=Instalacija nije završena. Ako je prekinete, program neće biti instaliran.%n%nInstalaciju možete ponoviti kasnije.%n%nŽelite li da prekinete instalaciju?
AboutSetupMenuItem=&O instalacionom programu...
AboutSetupTitle=O instalacionom programu
AboutSetupMessage=%1 verzija %2%n%3%n%n%1 početna stranica:%n%4
AboutSetupNote=
TranslatorNote=Srpski prevod

; *** Buttons
ButtonBack=< &Nazad
ButtonNext=&Dalje >
ButtonInstall=&Instaliraj
ButtonOK=U redu
ButtonCancel=Odustani
ButtonYes=&Da
ButtonYesToAll=Da za &sve
ButtonNo=&Ne
ButtonNoToAll=N&e za sve
ButtonFinish=&Završi
ButtonBrowse=&Pregledaj...
ButtonWizardBrowse=&Pregledaj...
ButtonNewFolder=&Kreiraj novi folder

; *** "Select Language" dialog messages
SelectLanguageTitle=Izbor jezika instalacije
SelectLanguageLabel=Izaberite jezik koji želite da koristite tokom instalacije.

; *** Common wizard text
ClickNext=Kliknite Dalje za nastavak instalacije ili Odustani za prekid instalacije.
BeveledLabel=
BrowseDialogTitle=Izbor foldera
BrowseDialogLabel=Izaberite folder sa spiska, zatim kliknite U redu.
NewFolderName=Novi folder

; *** "Welcome" wizard page
WelcomeLabel1=Dobrodošli u instalaciju programa [name].
WelcomeLabel2=U računar ćete instalirati program [name/ver].%n%nPreporučljivo je da zatvorite sve otvorene programe pre početka instalacije.

; *** "Password" wizard page
WizardPassword=Lozinka
PasswordLabel1=Instalacija je zaštićena lozinkom.
PasswordLabel3=Unesite lozinku, zatim kliknite Dalje za nastavak. Pazite na velika i mala slova.
PasswordEditLabel=&Lozinka:
IncorrectPassword=Uneta lozinka nije ispravna. Pokušajte ponovo.

; *** "License Agreement" wizard page
WizardLicense=Licencni ugovor
LicenseLabel=Pre nastavka pročitajte licencni ugovor.
LicenseLabel3=Pročitajte licencni ugovor. Program možete instalirati samo ako se u potpunosti slažete sa uslovima ugovora.
LicenseAccepted=&Da, prihvatam sve uslove licencnog ugovora
LicenseNotAccepted=N&e, ne prihvatam uslove licencnog ugovora

; *** "Information" wizard pages
WizardInfoBefore=Informacije
InfoBeforeLabel=Pre nastavka pročitajte sledeće važne informacije.
InfoBeforeClickLabel=Kad budete spremni za nastavak instalacije, kliknite Dalje.
WizardInfoAfter=Informacije
InfoAfterLabel=Pre nastavka pročitajte sledeće važne informacije.
InfoAfterClickLabel=Kad budete spremni za nastavak instalacije, kliknite Dalje.

; *** "User Information" wizard page
WizardUserInfo=Podaci o korisniku
UserInfoDesc=Unesite svoje podatke.
UserInfoName=&Ime:
UserInfoOrg=&Firma:
UserInfoSerial=&Serijski broj:
UserInfoNameRequired=Morate uneti ime.

; *** "Select Destination Location" wizard page
WizardSelectDir=Izbor odredišne lokacije
SelectDirDesc=Gde želite da instalirate program [name]?
SelectDirLabel3=Program [name] biće instaliran u sledeći folder.
SelectDirBrowseLabel=Za nastavak kliknite Dalje. Ako želite da izaberete drugi folder, kliknite Pregledaj.
DiskSpaceGBLabel=Potrebno je najmanje [gb] GB slobodnog prostora na disku.
DiskSpaceMBLabel=Potrebno je najmanje [mb] MB slobodnog prostora na disku.
CannotInstallToNetworkDrive=Program nije moguće instalirati na mrežni disk.
CannotInstallToUNCPath=Program nije moguće instalirati na UNC putanju.
InvalidPath=Morate upisati punu putanju uključujući oznaku diska. Primer:%n%nC:\PROGRAM%n%nili UNC putanju u obliku:%n%n\\server\deljeno
InvalidDrive=Izabrani disk ili UNC putanja ne postoji ili nije dostupna. Izaberite drugu.
DiskSpaceWarningTitle=Nema dovoljno prostora na disku
DiskSpaceWarning=Instalacija zahteva najmanje %1 KB prostora, ali na izabranom disku dostupno je samo %2 KB.%n%nŽelite li ipak da nastavite?
DirNameTooLong=Ime foldera ili putanja je predugačka.
InvalidDirName=Ime foldera nije validno.
BadDirName32=Ime foldera ne sme da sadrži sledeće znakove:%n%n%1
DirExistsTitle=Folder već postoji
DirExists=Folder%n%n%1%n%nveć postoji. Želite li ipak da instalirate program u taj folder?
DirDoesntExistTitle=Folder ne postoji
DirDoesntExist=Folder %n%n%1%n%nne postoji. Želite li da ga kreirate?

; *** "Select Components" wizard page
WizardSelectComponents=Izbor komponenti
SelectComponentsDesc=Koje komponente želite da instalirate?
SelectComponentsLabel2=Označite komponente koje želite da instalirate; odznačite one koje ne želite da instalirate. Kliknite Dalje kad budete spremni za nastavak.
FullInstallation=Potpuna instalacija
CompactInstallation=Osnovna instalacija
CustomInstallation=Prilagođena instalacija
NoUninstallWarningTitle=Komponente već postoje
NoUninstallWarning=Instalacioni program je utvrdio da su sledeće komponente već instalirane u računaru:%n%n%1%n%nInstalacioni program neće ukloniti te već instalirane komponente.%n%nŽelite li ipak da nastavite?
ComponentSize1=%1 KB
ComponentSize2=%1 MB
ComponentsDiskSpaceGBLabel=Za izabranu instalaciju potrebno je najmanje [gb] GB prostora na disku.
ComponentsDiskSpaceMBLabel=Za izabranu instalaciju potrebno je najmanje [mb] MB prostora na disku.

; *** "Select Additional Tasks" wizard page
WizardSelectTasks=Izbor dodatnih zadataka
SelectTasksDesc=Koje dodatne zadatke želite da izvršite?
SelectTasksLabel2=Izaberite dodatne zadatke koje će instalacioni program izvršiti tokom instalacije programa [name], zatim kliknite Dalje.

; *** "Select Start Menu Folder" wizard page
WizardSelectProgramGroup=Izbor foldera u Start meniju
SelectStartMenuFolderDesc=Gde želite da instalacioni program kreira prečice?
SelectStartMenuFolderLabel3=Instalacioni program će kreirati prečice u sledećem folderu u Start meniju.
SelectStartMenuFolderBrowseLabel=Za nastavak kliknite Dalje. Ako želite da izaberete drugi folder, kliknite Pregledaj.
MustEnterGroupName=Morate uneti ime grupe.
GroupNameTooLong=Ime foldera ili putanja je predugačka.
InvalidGroupName=Ime foldera nije validno.
BadGroupName=Ime grupe ne sme da sadrži sledeće znakove:%n%n%1
NoProgramGroupCheck2=&Ne kreiraj folder u Start meniju

; *** "Ready to Install" wizard page
WizardReady=Spremno za instalaciju
ReadyLabel1=Instalacioni program je spreman za instalaciju programa [name] u vaš računar.
ReadyLabel2a=Kliknite Instaliraj za početak instalacije. Kliknite Nazad ako želite da pregledate ili promenite bilo koje podešavanje.
ReadyLabel2b=Kliknite Instaliraj za početak instalacije.
ReadyMemoUserInfo=Podaci o korisniku:
ReadyMemoDir=Odredišna lokacija:
ReadyMemoType=Vrsta instalacije:
ReadyMemoComponents=Izabrane komponente:
ReadyMemoGroup=Folder u Start meniju:
ReadyMemoTasks=Dodatni zadaci:

; *** TDownloadWizardPage wizard page and DownloadTemporaryFile
DownloadingLabel2=Preuzimanje datoteka...
ButtonStopDownload=Prekini &preuzimanje
StopDownload=Da li ste sigurni da želite da prekinete preuzimanje?
ErrorDownloadAborted=Preuzimanje prekinuto
ErrorDownloadFailed=Preuzimanje nije uspelo: %1 %2
ErrorDownloadSizeFailed=Dobijanje veličine nije uspelo: %1 %2
ErrorProgress=Neispravan napredak: %1 od %2
ErrorFileSize=Neispravna veličina datoteke: očekivano %1, dobijeno %2

; *** TExtractionWizardPage wizard page and ExtractArchive
ExtractingLabel=Raspakivanje datoteka...
ButtonStopExtraction=Z&austavi raspakivanje
StopExtraction=Da li ste sigurni da želite da zaustavite raspakivanje datoteka?
ErrorExtractionAborted=Raspakivanje datoteka prekinuto
ErrorExtractionFailed=Greška pri raspakivanju: %1

; *** Archive extraction failure details
ArchiveIncorrectPassword=Pogrešna lozinka
ArchiveIsCorrupted=Arhivska datoteka je oštećena
ArchiveUnsupportedFormat=Format arhive nije podržan

; *** "Preparing to Install" wizard page
WizardPreparing=Priprema za instalaciju
PreparingDesc=Instalacioni program se priprema za instalaciju programa [name] u vaš računar.
PreviousInstallNotCompleted=Instalacija ili uklanjanje prethodnog programa nije završena. Morate ponovo pokrenuti računar da biste je dovršili.%n%nNakon ponovnog pokretanja računara, ponovo pokrenite instalacioni program da biste završili instalaciju programa [name].
CannotContinue=Instalacioni program ne može da nastavi. Kliknite Odustani za izlaz.

; *** "Installing" wizard page
ApplicationsFound=Sledeći programi koriste datoteke koje instalacioni program mora da ažurira. Preporučljivo je da dozvolite instalacionom programu da zatvori te programe.
ApplicationsFound2=Sledeći programi koriste datoteke koje instalacioni program mora da ažurira. Preporučljivo je da dozvolite instalacionom programu da zatvori te programe. Nakon završetka instalacije, instalacioni program će pokušati ponovo da pokrene te programe.
CloseApplications=&Automatski zatvori programe
DontCloseApplications=&Ne zatvaraj programe
ErrorCloseApplications=Instalacionom programu nije uspelo da automatski zatvori sve programe. Preporučljivo je da zatvorite sve programe koji koriste datoteke koje instalacija mora da ažurira pre nastavka.
PrepareToInstallNeedsRestart=Instalacioni program mora ponovo da pokrene vaš računar. Za dovršetak instalacije programa [name], nakon ponovnog pokretanja ponovo pokrenite instalacioni program.%n%nŽelite li sada ponovo da pokrenete računar?

WizardInstalling=Instaliranje
InstallingLabel=Sačekajte dok se program [name] instalira u vaš računar.

; *** "Setup Completed" wizard page
FinishedHeadingLabel=Završetak instalacije programa [name]
FinishedLabelNoIcons=Program [name] je instaliran u vaš računar.
FinishedLabel=Program [name] je instaliran u vaš računar. Program možete pokrenuti otvaranjem upravo kreiranih ikona.
ClickFinish=Kliknite Završi za završetak instalacije.
FinishedRestartLabel=Za dovršetak instalacije programa [name] morate ponovo da pokrenete računar. Želite li da ga sada ponovo pokrenete?
FinishedRestartMessage=Za dovršetak instalacije programa [name] morate ponovo da pokrenete računar.%n%nŽelite li da ga sada ponovo pokrenete?
ShowReadmeCheck=Želim da pročitam README datoteku
YesRadio=&Da, ponovo pokreni računar sada
NoRadio=&Ne, računar ću ponovo pokrenuti kasnije

; used for example as 'Run MyProg.exe'
RunEntryExec=Pokreni %1
; used for example as 'View Readme.txt'
RunEntryShellExec=Pregledaj %1

; *** "Setup Needs the Next Disk" stuff
ChangeDiskTitle=Instalacioni program treba sledeći disk
SelectDiskLabel2=Ubacite disk %1 i kliknite U redu.%n%nAko se datoteke sa ovog diska nalaze u drugom folderu od navedenog, unesite ispravnu putanju ili kliknite Pregledaj.
PathLabel=&Putanja:
FileNotInDir2=Datoteka »%1« nije u folderu »%2«. Ubacite ispravan disk ili izaberite drugi folder.
SelectDirectoryLabel=Navedite lokaciju sledećeg diska.

; *** Installation phase messages
SetupAborted=Instalacija nije završena.%n%nIspravite problem i ponovo pokrenite instalacioni program.
AbortRetryIgnoreSelectAction=Izaberite radnju
AbortRetryIgnoreRetry=Pokušaj &ponovo
AbortRetryIgnoreIgnore=&Zanemari grešku i nastavi
AbortRetryIgnoreCancel=Prekini instalaciju
RetryCancelSelectAction=Izaberite radnju
RetryCancelRetry=Pokušaj &ponovo
RetryCancelCancel=Odustani

; *** Installation status messages
StatusClosingApplications=Zatvaranje programa...
StatusCreateDirs=Kreiranje foldera...
StatusExtractFiles=Raspakivanje datoteka...
StatusDownloadFiles=Preuzimanje datoteka...
StatusCreateIcons=Kreiranje prečica...
StatusCreateIniEntries=Upisivanje u INI datoteke...
StatusCreateRegistryEntries=Kreiranje unosa u registar...
StatusRegisterFiles=Registrovanje datoteka...
StatusSavingUninstall=Čuvanje podataka za deinstalaciju...
StatusRunProgram=Završavanje instalacije...
StatusRestartingApplications=Pokretanje programa...
StatusRollback=Vraćanje na prethodno stanje...

; *** Misc. errors
ErrorInternal2=Unutrašnja greška: %1
ErrorFunctionFailedNoCode=%1 nije uspeo/uspela
ErrorFunctionFailed=%1 nije uspeo/uspela; kod %2
ErrorFunctionFailedWithMessage=%1 nije uspeo/uspela; kod %2.%n%3
ErrorExecutingProgram=Ne mogu da pokrenem program:%n%1

; *** Registry errors
ErrorRegOpenKey=Greška pri otvaranju ključa registra:%n%1\%2
ErrorRegCreateKey=Greška pri kreiranju ključa registra:%n%1\%2
ErrorRegWriteKey=Greška pri pisanju ključa registra:%n%1\%2

; *** INI errors
ErrorIniEntry=Greška pri upisu u INI datoteku »%1«.

; *** File copying errors
FileAbortRetryIgnoreSkipNotRecommended=Pre&skoči ovu datoteku (ne preporučuje se)
FileAbortRetryIgnoreIgnoreNotRecommended=Zanem&ari grešku i nastavi (ne preporučuje se)
SourceIsCorrupted=Izvorna datoteka je oštećena
SourceDoesntExist=Izvorna datoteka »%1« ne postoji
SourceVerificationFailed=Provera izvorne datoteke nije uspela: %1
VerificationSignatureDoesntExist=Datoteka potpisa »%1« ne postoji
VerificationSignatureInvalid=Datoteka potpisa »%1« nije validna
VerificationKeyNotFound=Datoteka potpisa »%1« koristi nepoznati ključ
VerificationFileNameIncorrect=Ime datoteke nije ispravno
VerificationFileTagIncorrect=Oznaka datoteke nije ispravna
VerificationFileSizeIncorrect=Veličina datoteke nije ispravna
VerificationFileHashIncorrect=Hash datoteke nije ispravan
ExistingFileReadOnly2=Postojeću datoteku nije moguće zameniti jer je označena samo za čitanje.
ExistingFileReadOnlyRetry=Uk&loni oznaku samo za čitanje i pokušaj ponovo
ExistingFileReadOnlyKeepExisting=&Zadrži postojeću datoteku
ErrorReadingExistingDest=Došlo je do greške pri čitanju postojeće datoteke:
FileExistsSelectAction=Izaberite radnju
FileExists2=Datoteka već postoji.
FileExistsOverwriteExisting=&Prepiši postojeću datoteku
FileExistsKeepExisting=&Zadrži trenutnu datoteku
FileExistsOverwriteOrKeepAll=&Učini isto za preostale sukobe
ExistingFileNewerSelectAction=Izaberite radnju
ExistingFileNewer2=Postojeća datoteka je novija od one koja se instalira.
ExistingFileNewerOverwriteExisting=&Prepiši postojeću datoteku
ExistingFileNewerKeepExisting=&Zadrži trenutnu datoteku (preporučeno)
ExistingFileNewerOverwriteOrKeepAll=&Učini isto za preostale sukobe
ErrorChangingAttr=Došlo je do greške pri pokušaju promene atributa datoteke:
ErrorCreatingTemp=Došlo je do greške pri kreiranju datoteke u odredišnom folderu:
ErrorReadingSource=Došlo je do greške pri čitanju izvorne datoteke:
ErrorCopying=Došlo je do greške pri kopiranju datoteke:
ErrorDownloading=Došlo je do greške pri preuzimanju datoteke:
ErrorExtracting=Došlo je do greške pri raspakivanju datoteke:
ErrorReplacingExistingFile=Došlo je do greške pri pokušaju zamene postojeće datoteke:
ErrorRestartReplace=Greška RestartReplace:
ErrorRenamingTemp=Došlo je do greške pri pokušaju preimenovanja datoteke u odredišnom folderu:
ErrorRegisterServer=Registracija DLL/OCX nije uspela: %1
ErrorRegSvr32Failed=RegSvr32 nije uspeo sa kodom greške %1
ErrorRegisterTypeLib=Registracija TypeLib nije uspela: %1

; *** Uninstall display name markings
UninstallDisplayNameMark=%1 (%2)
UninstallDisplayNameMarks=%1 (%2, %3)
UninstallDisplayNameMark32Bit=32-bitno
UninstallDisplayNameMark64Bit=64-bitno
UninstallDisplayNameMarkAllUsers=svi korisnici
UninstallDisplayNameMarkCurrentUser=trenutni korisnik

; *** Post-installation errors
ErrorOpeningReadme=Došlo je do greške pri otvaranju README datoteke.
ErrorRestartingComputer=Instalacionom programu nije uspelo ponovo da pokrene računar. Molimo ponovo pokrenite računar ručno.

; *** Uninstaller messages
UninstallNotFound=Datoteka »%1« ne postoji. Deinstalacija nije moguća.
UninstallOpenError=Datoteku »%1« nije moguće otvoriti. Deinstalacija nije moguća
UninstallUnsupportedVer=Datoteka dnevnika »%1« je u formatu koji ova verzija programa za deinstalaciju ne razume. Program nije moguće deinstalirati
UninstallUnknownEntry=U datoteci dnevnika pronađen je nepoznati unos (%1)
ConfirmUninstall=Da li ste sigurni da želite u potpunosti da uklonite program %1 i sve njegove komponente?
UninstallOnlyOnWin64=Ovu instalaciju moguće je deinstalirati samo na 64-bitnoj verziji sistema Windows.
OnlyAdminCanUninstall=Za deinstalaciju ovog programa morate imati administratorska ovlašćenja.
UninstallStatusLabel=Sačekajte dok se program %1 uklanja iz vašeg računara.
UninstalledAll=Program %1 je uspešno uklonjen iz vašeg računara.
UninstalledMost=Deinstalacija programa %1 je završena.%n%nNeke datoteke nisu uklonjene i možete ih ukloniti ručno.
UninstalledAndNeedsRestart=Za dovršetak deinstalacije programa %1 morate ponovo da pokrenete računar.%n%nŽelite li da ga sada ponovo pokrenete?
UninstallDataCorrupted=Datoteka »%1« je oštećena. Deinstalacija nije moguća

; *** Uninstallation phase messages
ConfirmDeleteSharedFileTitle=Želite li da uklonite deljenu datoteku?
ConfirmDeleteSharedFile2=Dole navedenu deljenu datoteku više ne koristi nijedan program. Želite li da uklonite tu datoteku?%n%nAko je koristi bilo koji program i uklonite je, taj program verovatno neće više raditi ispravno. Ako niste sigurni, kliknite Ne. Zadržavanje datoteke u računaru neće uzrokovati nikakve probleme.
SharedFileNameLabel=Ime datoteke:
SharedFileLocationLabel=Lokacija:
WizardUninstalling=Deinstalacija programa
StatusUninstalling=Deinstaliram %1...

ShutdownBlockReasonInstallingApp=Instaliram %1.
ShutdownBlockReasonUninstallingApp=Deinstaliram %1.

[CustomMessages]

NameAndVersion=%1 verzija %2
AdditionalIcons=Dodatne ikone:
CreateDesktopIcon=Kreiraj ikonu na &radnoj površini
CreateQuickLaunchIcon=Kreiraj ikonu za &brzo pokretanje
ProgramOnTheWeb=%1 na webu
UninstallProgram=Deinstaliraj %1
LaunchProgram=Pokreni %1
AssocFileExtension=&Poveži %1 sa ekstenzijom %2
AssocingFileExtension=Povezujem %1 sa ekstenzijom %2...
AutoStartProgramGroupDescription=Pokretanje:
AutoStartProgram=Automatski pokreni %1
AddonHostProgramNotFound=Program %1 nije pronađen u izabranom folderu.%n%nŽelite li ipak da nastavite?

; LaprdusTTS custom messages
LaprdusDesktopShortcut=Napravi prečicu na radnoj površini
LaprdusAdditionalOptions=Dodatne opcije:
LaprdusConfiguratorName=Laprdus konfigurator
LaprdusConfiguratorComment=Konfigurišite Laprdus podešavanja
LaprdusWebsiteName=Laprdus Web stranica
LaprdusWebsiteComment=Posetite Laprdus web stranicu
LaprdusUninstallName=Deinstaliraj Laprdus
LaprdusUninstallComment=Uklonite Laprdus sa računara
LaprdusOldVersionFound=Prethodna verzija programa Laprdus je već instalirana. Želite li da je deinstalirate pre nastavka?%n%nKliknite Da za deinstalaciju prethodne verzije ili Ne za prekid instalacije.
LaprdusUpgradeFound=Prethodna verzija programa Laprdus je već instalirana. Instalacioni program će nadograditi postojeću instalaciju.%n%nKliknite Da za nastavak nadogradnje ili Ne za prekid.
