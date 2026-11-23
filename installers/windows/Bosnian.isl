; *** Inno Setup version 6.0+ Bosnian messages ***
;
; Bosnian translation for LaprdusTTS installer
; Made from the Croatian translation (Croatian.isl): keep the two in step

[LangOptions]
LanguageName=Bosanski
LanguageID=$141A
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
SetupLdrStartupMessage=Instalirat ćete program %1. Želite li nastaviti?
LdrCannotCreateTemp=Nije moguće kreirati privremenu datoteku. Instalacija je prekinuta
LdrCannotExecTemp=Nije moguće pokrenuti datoteku u privremenom folderu. Instalacija je prekinuta

; *** Startup error messages
LastErrorMessage=%1.%n%nGreška %2: %3
SetupFileMissing=Datoteka %1 nedostaje. Ispravite grešku ili nabavite novu kopiju programa.
SetupFileCorrupt=Instalacijske datoteke su oštećene. Nabavite novu kopiju programa.
SetupFileCorruptOrWrongVer=Datoteke su oštećene ili nekompatibilne s ovom verzijom instalacijskog programa. Ispravite grešku ili nabavite novu kopiju programa.
InvalidParameter=Naveden je neispravan parametar komandne linije:%n%n%1
SetupAlreadyRunning=Instalacijski program se već izvršava.
WindowsVersionNotSupported=Program ne radi na vašoj verziji sistema Windows.
WindowsServicePackRequired=Program zahtijeva %1 sa servisnim paketom %2 ili noviju verziju.
NotOnThisPlatform=Program nije namijenjen za upotrebu na %1.
OnlyOnThisPlatform=Program je namijenjen samo za upotrebu na %1.
OnlyOnTheseArchitectures=Program možete instalirati samo na Windows sistemima sa sljedećim vrstama procesora:%n%n%1
WinVersionTooLowError=Ovaj program zahtijeva %1 verziju %2 ili noviju.
WinVersionTooHighError=Ovaj program ne možete instalirati na %1 verzije %2 ili novije.
AdminPrivilegesRequired=Za instalaciju programa morate biti prijavljeni kao administrator.
PowerUserPrivilegesRequired=Za instalaciju programa morate biti prijavljeni kao administrator ili power user.
SetupAppRunningError=Program %1 je trenutno otvoren.%n%nZatvorite program, zatim kliknite U redu za nastavak ili Otkaži za izlaz.
UninstallAppRunningError=Program %1 je trenutno otvoren.%n%nZatvorite program, zatim kliknite U redu za nastavak ili Otkaži za izlaz.

; *** Startup questions
PrivilegesRequiredOverrideTitle=Odaberite način instalacije
PrivilegesRequiredOverrideInstruction=Odaberite način instalacije
PrivilegesRequiredOverrideText1=Program %1 možete instalirati za sve korisnike (potrebna su administratorska ovlaštenja) ili samo za sebe.
PrivilegesRequiredOverrideText2=Program %1 možete instalirati samo za sebe ili za sve korisnike (potrebna su administratorska ovlaštenja).
PrivilegesRequiredOverrideAllUsers=I&nstaliraj za sve korisnike
PrivilegesRequiredOverrideAllUsersRecommended=I&nstaliraj za sve korisnike (preporučeno)
PrivilegesRequiredOverrideCurrentUser=Instaliraj samo za &mene
PrivilegesRequiredOverrideCurrentUserRecommended=Instaliraj samo za &mene (preporučeno)

; *** Misc. errors
ErrorCreatingDir=Instalacijski program nije mogao kreirati folder »%1«
ErrorTooManyFilesInDir=Instalacijski program ne može kreirati novu datoteku u folderu »%1« jer sadrži previše datoteka

; *** Setup common messages
ExitSetupTitle=Prekini instalaciju
ExitSetupMessage=Instalacija nije završena. Ako je prekinete, program neće biti instaliran.%n%nInstalaciju možete ponoviti kasnije.%n%nŽelite li prekinuti instalaciju?
AboutSetupMenuItem=&O instalacijskom programu...
AboutSetupTitle=O instalacijskom programu
AboutSetupMessage=%1 verzija %2%n%3%n%n%1 početna stranica:%n%4
AboutSetupNote=
TranslatorNote=Bosanski prijevod

; *** Buttons
ButtonBack=< &Nazad
ButtonNext=&Dalje >
ButtonInstall=&Instaliraj
ButtonOK=U redu
ButtonCancel=Otkaži
ButtonYes=&Da
ButtonYesToAll=Da za &sve
ButtonNo=&Ne
ButtonNoToAll=N&e za sve
ButtonFinish=&Završi
ButtonBrowse=&Pregledaj...
ButtonWizardBrowse=&Pregledaj...
ButtonNewFolder=&Kreiraj novi folder

; *** "Select Language" dialog messages
SelectLanguageTitle=Odabir jezika instalacije
SelectLanguageLabel=Odaberite jezik koji želite koristiti tokom instalacije.

; *** Common wizard text
ClickNext=Kliknite Dalje za nastavak instalacije ili Otkaži za prekid instalacije.
BeveledLabel=
BrowseDialogTitle=Odabir foldera
BrowseDialogLabel=Odaberite folder s liste, zatim kliknite U redu.
NewFolderName=Novi folder

; *** "Welcome" wizard page
WelcomeLabel1=Dobrodošli u instalaciju programa [name].
WelcomeLabel2=Na računar ćete instalirati program [name/ver].%n%nPreporučljivo je zatvoriti sve otvorene programe prije početka instalacije.

; *** "Password" wizard page
WizardPassword=Lozinka
PasswordLabel1=Instalacija je zaštićena lozinkom.
PasswordLabel3=Unesite lozinku, zatim kliknite Dalje za nastavak. Pazite na velika i mala slova.
PasswordEditLabel=&Lozinka:
IncorrectPassword=Unesena lozinka nije ispravna. Pokušajte ponovo.

; *** "License Agreement" wizard page
WizardLicense=Licencni ugovor
LicenseLabel=Prije nastavka pročitajte licencni ugovor.
LicenseLabel3=Pročitajte licencni ugovor. Program možete instalirati samo ako se u potpunosti slažete s uslovima ugovora.
LicenseAccepted=&Da, prihvatam sve uslove licencnog ugovora
LicenseNotAccepted=N&e, ne prihvatam uslove licencnog ugovora

; *** "Information" wizard pages
WizardInfoBefore=Informacije
InfoBeforeLabel=Prije nastavka pročitajte sljedeće važne informacije.
InfoBeforeClickLabel=Kad budete spremni za nastavak instalacije, kliknite Dalje.
WizardInfoAfter=Informacije
InfoAfterLabel=Prije nastavka pročitajte sljedeće važne informacije.
InfoAfterClickLabel=Kad budete spremni za nastavak instalacije, kliknite Dalje.

; *** "User Information" wizard page
WizardUserInfo=Podaci o korisniku
UserInfoDesc=Unesite svoje podatke.
UserInfoName=&Ime:
UserInfoOrg=&Firma:
UserInfoSerial=&Serijski broj:
UserInfoNameRequired=Morate unijeti ime.

; *** "Select Destination Location" wizard page
WizardSelectDir=Odabir odredišne lokacije
SelectDirDesc=Gdje želite instalirati program [name]?
SelectDirLabel3=Program [name] bit će instaliran u sljedeći folder.
SelectDirBrowseLabel=Za nastavak kliknite Dalje. Ako želite odabrati drugi folder, kliknite Pregledaj.
DiskSpaceGBLabel=Potrebno je najmanje [gb] GB slobodnog prostora na disku.
DiskSpaceMBLabel=Potrebno je najmanje [mb] MB slobodnog prostora na disku.
CannotInstallToNetworkDrive=Program nije moguće instalirati na mrežni pogon.
CannotInstallToUNCPath=Program nije moguće instalirati na UNC putanju.
InvalidPath=Morate upisati punu putanju uključujući oznaku pogona. Primjer:%n%nC:\PROGRAM%n%nili UNC putanju u obliku:%n%n\\server\dijeljeno
InvalidDrive=Odabrani pogon ili UNC putanja ne postoji ili nije dostupna. Odaberite drugu.
DiskSpaceWarningTitle=Nema dovoljno prostora na disku
DiskSpaceWarning=Instalacija zahtijeva najmanje %1 KB prostora, ali na odabranom pogonu dostupno je samo %2 KB.%n%nŽelite li ipak nastaviti?
DirNameTooLong=Ime foldera ili putanja su predugački.
InvalidDirName=Ime foldera nije ispravno.
BadDirName32=Ime foldera ne smije sadržavati sljedeće znakove:%n%n%1
DirExistsTitle=Folder već postoji
DirExists=Folder%n%n%1%n%nveć postoji. Želite li ipak instalirati program u taj folder?
DirDoesntExistTitle=Folder ne postoji
DirDoesntExist=Folder %n%n%1%n%nne postoji. Želite li ga kreirati?

; *** "Select Components" wizard page
WizardSelectComponents=Odabir komponenti
SelectComponentsDesc=Koje komponente želite instalirati?
SelectComponentsLabel2=Označite komponente koje želite instalirati; odznačite one koje ne želite instalirati. Kliknite Dalje kad budete spremni za nastavak.
FullInstallation=Potpuna instalacija
CompactInstallation=Osnovna instalacija
CustomInstallation=Prilagođena instalacija
NoUninstallWarningTitle=Komponente već postoje
NoUninstallWarning=Instalacijski program je utvrdio da su sljedeće komponente već instalirane na računaru:%n%n%1%n%nInstalacijski program neće ukloniti te već instalirane komponente.%n%nŽelite li ipak nastaviti?
ComponentSize1=%1 KB
ComponentSize2=%1 MB
ComponentsDiskSpaceGBLabel=Za odabranu instalaciju potrebno je najmanje [gb] GB prostora na disku.
ComponentsDiskSpaceMBLabel=Za odabranu instalaciju potrebno je najmanje [mb] MB prostora na disku.

; *** "Select Additional Tasks" wizard page
WizardSelectTasks=Odabir dodatnih zadataka
SelectTasksDesc=Koje dodatne zadatke želite izvršiti?
SelectTasksLabel2=Odaberite dodatne zadatke koje će instalacijski program izvršiti tokom instalacije programa [name], zatim kliknite Dalje.

; *** "Select Start Menu Folder" wizard page
WizardSelectProgramGroup=Odabir foldera u meniju Start
SelectStartMenuFolderDesc=Gdje želite da instalacijski program kreira prečice?
SelectStartMenuFolderLabel3=Instalacijski program će kreirati prečice u sljedećem folderu u meniju Start.
SelectStartMenuFolderBrowseLabel=Za nastavak kliknite Dalje. Ako želite odabrati drugi folder, kliknite Pregledaj.
MustEnterGroupName=Morate unijeti ime grupe.
GroupNameTooLong=Ime foldera ili putanja su predugački.
InvalidGroupName=Ime foldera nije ispravno.
BadGroupName=Ime grupe ne smije sadržavati sljedeće znakove:%n%n%1
NoProgramGroupCheck2=&Ne kreiraj folder u meniju Start

; *** "Ready to Install" wizard page
WizardReady=Spremno za instalaciju
ReadyLabel1=Instalacijski program je spreman za instalaciju programa [name] na vaš računar.
ReadyLabel2a=Kliknite Instaliraj za početak instalacije. Kliknite Nazad ako želite pregledati ili promijeniti bilo koju postavku.
ReadyLabel2b=Kliknite Instaliraj za početak instalacije.
ReadyMemoUserInfo=Podaci o korisniku:
ReadyMemoDir=Odredišna lokacija:
ReadyMemoType=Vrsta instalacije:
ReadyMemoComponents=Odabrane komponente:
ReadyMemoGroup=Folder u meniju Start:
ReadyMemoTasks=Dodatni zadaci:

; *** TDownloadWizardPage wizard page and DownloadTemporaryFile
DownloadingLabel2=Preuzimanje datoteka...
ButtonStopDownload=Prekini &preuzimanje
StopDownload=Jeste li sigurni da želite prekinuti preuzimanje?
ErrorDownloadAborted=Preuzimanje prekinuto
ErrorDownloadFailed=Preuzimanje nije uspjelo: %1 %2
ErrorDownloadSizeFailed=Dohvatanje veličine nije uspjelo: %1 %2
ErrorProgress=Neispravan napredak: %1 od %2
ErrorFileSize=Neispravna veličina datoteke: očekivano %1, dobiveno %2

; *** TExtractionWizardPage wizard page and ExtractArchive
ExtractingLabel=Raspakivanje datoteka...
ButtonStopExtraction=Z&austavi raspakivanje
StopExtraction=Jeste li sigurni da želite zaustaviti raspakivanje datoteka?
ErrorExtractionAborted=Raspakivanje datoteka prekinuto
ErrorExtractionFailed=Greška pri raspakivanju: %1

; *** Archive extraction failure details
ArchiveIncorrectPassword=Pogrešna lozinka
ArchiveIsCorrupted=Arhivska datoteka je oštećena
ArchiveUnsupportedFormat=Format arhiva nije podržan

; *** "Preparing to Install" wizard page
WizardPreparing=Priprema za instalaciju
PreparingDesc=Instalacijski program se priprema za instalaciju programa [name] na vaš računar.
PreviousInstallNotCompleted=Instalacija ili uklanjanje prethodnog programa nije završena. Morate ponovo pokrenuti računar da biste je dovršili.%n%nNakon ponovnog pokretanja računara, ponovo pokrenite instalacijski program da biste završili instalaciju programa [name].
CannotContinue=Instalacijski program ne može nastaviti. Kliknite Otkaži za izlaz.

; *** "Installing" wizard page
ApplicationsFound=Sljedeći programi koriste datoteke koje instalacijski program mora ažurirati. Preporučljivo je dozvoliti instalacijskom programu da zatvori te programe.
ApplicationsFound2=Sljedeći programi koriste datoteke koje instalacijski program mora ažurirati. Preporučljivo je dozvoliti instalacijskom programu da zatvori te programe. Nakon završetka instalacije, instalacijski program će pokušati ponovo pokrenuti te programe.
CloseApplications=&Automatski zatvori programe
DontCloseApplications=&Ne zatvaraj programe
ErrorCloseApplications=Instalacijskom programu nije uspjelo automatski zatvoriti sve programe. Preporučljivo je zatvoriti sve programe koji koriste datoteke koje instalacija mora ažurirati prije nastavka.
PrepareToInstallNeedsRestart=Instalacijski program mora ponovo pokrenuti vaš računar. Za dovršetak instalacije programa [name], nakon ponovnog pokretanja ponovo pokrenite instalacijski program.%n%nŽelite li sada ponovo pokrenuti računar?

WizardInstalling=Instaliranje
InstallingLabel=Sačekajte dok se program [name] instalira na vaš računar.

; *** "Setup Completed" wizard page
FinishedHeadingLabel=Završetak instalacije programa [name]
FinishedLabelNoIcons=Program [name] je instaliran na vaš računar.
FinishedLabel=Program [name] je instaliran na vaš računar. Program možete pokrenuti otvaranjem upravo kreiranih ikona.
ClickFinish=Kliknite Završi za završetak instalacije.
FinishedRestartLabel=Za dovršetak instalacije programa [name] morate ponovo pokrenuti računar. Želite li ga sada ponovo pokrenuti?
FinishedRestartMessage=Za dovršetak instalacije programa [name] morate ponovo pokrenuti računar.%n%nŽelite li ga sada ponovo pokrenuti?
ShowReadmeCheck=Želim pročitati README datoteku
YesRadio=&Da, ponovo pokreni računar sada
NoRadio=&Ne, računar ću ponovo pokrenuti kasnije

; used for example as 'Run MyProg.exe'
RunEntryExec=Pokreni %1
; used for example as 'View Readme.txt'
RunEntryShellExec=Pregledaj %1

; *** "Setup Needs the Next Disk" stuff
ChangeDiskTitle=Instalacijski program treba sljedeći disk
SelectDiskLabel2=Umetnite disk %1 i kliknite U redu.%n%nAko se datoteke s ovog diska nalaze u drugom folderu od navedenog, unesite ispravnu putanju ili kliknite Pregledaj.
PathLabel=&Putanja:
FileNotInDir2=Datoteka »%1« nije u folderu »%2«. Umetnite ispravan disk ili odaberite drugi folder.
SelectDirectoryLabel=Navedite lokaciju sljedećeg diska.

; *** Installation phase messages
SetupAborted=Instalacija nije završena.%n%nIspravite problem i ponovo pokrenite instalacijski program.
AbortRetryIgnoreSelectAction=Odaberite radnju
AbortRetryIgnoreRetry=Pokušaj &ponovo
AbortRetryIgnoreIgnore=&Zanemari grešku i nastavi
AbortRetryIgnoreCancel=Prekini instalaciju
RetryCancelSelectAction=Odaberite radnju
RetryCancelRetry=Pokušaj &ponovo
RetryCancelCancel=Otkaži

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
ErrorFunctionFailedNoCode=%1 nije uspio/uspjela
ErrorFunctionFailed=%1 nije uspio/uspjela; kod %2
ErrorFunctionFailedWithMessage=%1 nije uspio/uspjela; kod %2.%n%3
ErrorExecutingProgram=Ne mogu pokrenuti program:%n%1

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
SourceVerificationFailed=Provjera izvorne datoteke nije uspjela: %1
VerificationSignatureDoesntExist=Datoteka potpisa »%1« ne postoji
VerificationSignatureInvalid=Datoteka potpisa »%1« nije ispravna
VerificationKeyNotFound=Datoteka potpisa »%1« koristi nepoznati ključ
VerificationFileNameIncorrect=Ime datoteke nije ispravno
VerificationFileTagIncorrect=Oznaka datoteke nije ispravna
VerificationFileSizeIncorrect=Veličina datoteke nije ispravna
VerificationFileHashIncorrect=Hash datoteke nije ispravan
ExistingFileReadOnly2=Postojeću datoteku nije moguće zamijeniti jer je označena samo za čitanje.
ExistingFileReadOnlyRetry=Uk&loni oznaku samo za čitanje i pokušaj ponovo
ExistingFileReadOnlyKeepExisting=&Zadrži postojeću datoteku
ErrorReadingExistingDest=Došlo je do greške pri čitanju postojeće datoteke:
FileExistsSelectAction=Odaberite radnju
FileExists2=Datoteka već postoji.
FileExistsOverwriteExisting=&Prepiši postojeću datoteku
FileExistsKeepExisting=&Zadrži trenutnu datoteku
FileExistsOverwriteOrKeepAll=&Učini isto za preostale sukobe
ExistingFileNewerSelectAction=Odaberite radnju
ExistingFileNewer2=Postojeća datoteka je novija od one koja se instalira.
ExistingFileNewerOverwriteExisting=&Prepiši postojeću datoteku
ExistingFileNewerKeepExisting=&Zadrži trenutnu datoteku (preporučeno)
ExistingFileNewerOverwriteOrKeepAll=&Učini isto za preostale sukobe
ErrorChangingAttr=Došlo je do greške pri pokušaju promjene atributa datoteke:
ErrorCreatingTemp=Došlo je do greške pri kreiranju datoteke u odredišnom folderu:
ErrorReadingSource=Došlo je do greške pri čitanju izvorne datoteke:
ErrorCopying=Došlo je do greške pri kopiranju datoteke:
ErrorDownloading=Došlo je do greške pri preuzimanju datoteke:
ErrorExtracting=Došlo je do greške pri raspakivanju datoteke:
ErrorReplacingExistingFile=Došlo je do greške pri pokušaju zamjene postojeće datoteke:
ErrorRestartReplace=Greška RestartReplace:
ErrorRenamingTemp=Došlo je do greške pri pokušaju preimenovanja datoteke u odredišnom folderu:
ErrorRegisterServer=Registracija DLL/OCX nije uspjela: %1
ErrorRegSvr32Failed=RegSvr32 nije uspio s kodom greške %1
ErrorRegisterTypeLib=Registracija TypeLib nije uspjela: %1

; *** Uninstall display name markings
UninstallDisplayNameMark=%1 (%2)
UninstallDisplayNameMarks=%1 (%2, %3)
UninstallDisplayNameMark32Bit=32-bitno
UninstallDisplayNameMark64Bit=64-bitno
UninstallDisplayNameMarkAllUsers=svi korisnici
UninstallDisplayNameMarkCurrentUser=trenutni korisnik

; *** Post-installation errors
ErrorOpeningReadme=Došlo je do greške pri otvaranju README datoteke.
ErrorRestartingComputer=Instalacijskom programu nije uspjelo ponovo pokrenuti računar. Molimo ponovo pokrenite računar ručno.

; *** Uninstaller messages
UninstallNotFound=Datoteka »%1« ne postoji. Deinstalacija nije moguća.
UninstallOpenError=Datoteku »%1« nije moguće otvoriti. Deinstalacija nije moguća
UninstallUnsupportedVer=Datoteka dnevnika »%1« je u formatu koji ova verzija programa za deinstalaciju ne razumije. Program nije moguće deinstalirati
UninstallUnknownEntry=U datoteci dnevnika pronađen je nepoznati unos (%1)
ConfirmUninstall=Jeste li sigurni da želite u potpunosti ukloniti program %1 i sve njegove komponente?
UninstallOnlyOnWin64=Ovu instalaciju moguće je deinstalirati samo na 64-bitnoj verziji sistema Windows.
OnlyAdminCanUninstall=Za deinstalaciju ovog programa morate imati administratorska ovlaštenja.
UninstallStatusLabel=Sačekajte dok se program %1 uklanja s vašeg računara.
UninstalledAll=Program %1 je uspješno uklonjen s vašeg računara.
UninstalledMost=Deinstalacija programa %1 je završena.%n%nNeke datoteke nisu uklonjene i možete ih ukloniti ručno.
UninstalledAndNeedsRestart=Za dovršetak deinstalacije programa %1 morate ponovo pokrenuti računar.%n%nŽelite li ga sada ponovo pokrenuti?
UninstallDataCorrupted=Datoteka »%1« je oštećena. Deinstalacija nije moguća

; *** Uninstallation phase messages
ConfirmDeleteSharedFileTitle=Želite li ukloniti dijeljenu datoteku?
ConfirmDeleteSharedFile2=Dolje navedenu dijeljenu datoteku više ne koristi nijedan program. Želite li ukloniti tu datoteku?%n%nAko je koristi bilo koji program i uklonite je, taj program vjerovatno neće više raditi ispravno. Ako niste sigurni, kliknite Ne. Zadržavanje datoteke na računaru neće uzrokovati nikakve probleme.
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
AssocFileExtension=&Poveži %1 s ekstenzijom %2
AssocingFileExtension=Povezujem %1 s ekstenzijom %2...
AutoStartProgramGroupDescription=Pokretanje:
AutoStartProgram=Automatski pokreni %1
AddonHostProgramNotFound=Program %1 nije pronađen u odabranom folderu.%n%nŽelite li ipak nastaviti?

; LaprdusTTS custom messages
LaprdusDesktopShortcut=Napravi prečicu na radnoj površini
LaprdusAdditionalOptions=Dodatne opcije:
LaprdusConfiguratorName=Laprdus konfigurator
LaprdusConfiguratorComment=Konfigurišite Laprdus postavke
LaprdusWebsiteName=Laprdus Web stranica
LaprdusWebsiteComment=Posjetite Laprdus web stranicu
LaprdusUninstallName=Deinstaliraj Laprdus
LaprdusUninstallComment=Uklonite Laprdus s računara
LaprdusOldVersionFound=Prethodna verzija programa Laprdus je već instalirana. Želite li je deinstalirati prije nastavka?%n%nKliknite Da za deinstalaciju prethodne verzije ili Ne za prekid instalacije.
LaprdusUpgradeFound=Prethodna verzija programa Laprdus je već instalirana. Instalacijski program će nadograditi postojeću instalaciju.%n%nKliknite Da za nastavak nadogradnje ili Ne za prekid.
LaprdusInstallNvdaAddon=Instaliraj dodatak Laprdus za NVDA
