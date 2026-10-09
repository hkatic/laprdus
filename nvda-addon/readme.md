# Laprdus - Korisnički priručnik

Inačica 2.0

---

## Sadržaj

1. [Uvod](#1-uvod)
   - [1.1 Što je Laprdus?](#11-što-je-laprdus)
   - [1.2 Kome je Laprdus namijenjen?](#12-kome-je-laprdus-namijenjen)
   - [1.3 Podržane platforme](#13-podržane-platforme)
   - [1.4 Mogućnosti](#14-mogućnosti)
2. [Instalacija](#2-instalacija)
   - [2.1 Windows SAPI5](#21-windows-sapi5)
   - [2.2 NVDA dodatak](#22-nvda-dodatak)
   - [2.3 Linux](#23-linux)
   - [2.4 Android](#24-android)
3. [Uporaba](#3-uporaba)
   - [3.1 Osnovna uporaba](#31-osnovna-uporaba)
   - [3.2 Naredbena linija (Linux i Windows)](#32-naredbena-linija-linux-i-windows)
   - [3.3 NVDA postavke](#33-nvda-postavke)
   - [3.4 Android aplikacija](#34-android-aplikacija)
4. [Postavke](#4-postavke)
   - [4.1 Brzina govora](#41-brzina-govora)
   - [4.2 Visina glasa](#42-visina-glasa)
   - [4.3 Glasnoća](#43-glasnoća)
   - [4.4 Pauze](#44-pauze)
   - [4.5 Način čitanja brojeva](#45-način-čitanja-brojeva)
   - [4.6 Infleksija](#46-infleksija)
   - [4.7 Slovkanje](#47-slovkanje)
   - [4.8 Laprdus Konfigurator (Windows)](#48-laprdus-konfigurator-windows)
5. [Rječnici](#5-rječnici)
   - [5.1 Vrste rječnika](#51-vrste-rječnika)
   - [5.2 Lokacije datoteka rječnika](#52-lokacije-datoteka-rječnika)
   - [5.3 Glavni rječnik (user.json)](#53-glavni-rječnik-userjson)
   - [5.4 Rječnik slovkanja (spelling.json)](#54-rječnik-slovkanja-spellingjson)
   - [5.5 Rječnik emodžija (emoji.json)](#55-rječnik-emodžija-emojijson)
   - [5.6 Uređivanje rječnika na Windowsu](#56-uređivanje-rječnika-na-windowsu-laprdus-konfigurator)
   - [5.7 Uređivanje rječnika na Androidu](#57-uređivanje-rječnika-na-androidu)
   - [5.8 Ručno uređivanje JSON datoteka](#58-ručno-uređivanje-json-datoteka-napredno)
   - [5.9 Primjeri praktične uporabe](#59-primjeri-praktične-uporabe)
   - [5.10 Rješavanje problema s rječnicima](#510-rješavanje-problema-s-rječnicima)
   - [5.11 Naglasni rječnik (accents.json)](#511-naglasni-rječnik-accentsjson)
6. [Glasovi](#6-glasovi)
   - [6.1 Dostupni glasovi](#61-dostupni-glasovi)
   - [6.2 Osnovni i izvedeni glasovi](#62-osnovni-i-izvedeni-glasovi)
   - [6.3 Odabir glasa](#63-odabir-glasa)
7. [Rješavanje problema](#7-rješavanje-problema)
   - [7.1 Česti problemi](#71-česti-problemi)
   - [7.2 Dijagnostički zapisnici](#72-dijagnostički-zapisnici)
   - [7.3 Kontakt i podrška](#73-kontakt-i-podrška)

---

## 1. Uvod

### 1.1 Što je Laprdus?

Laprdus je sintetizator govora (TTS - Text-to-Speech) za hrvatski i srpski jezik. Koristi tehnologiju konkatenativne sinteze, spajajući unaprijed snimljene fonemske jedinice za proizvodnju govora. Iako ne doseže kvalitetu modernih neuronskih TTS sustava, Laprdus nudi visoke performanse i minimalnu potrošnju memorije.

Laprdus je razvijen kako bi korisnicima čitača ekrana pružio jednostavan i brz pristup računalima i mobilnim uređajima na njihovom materinjem jeziku, besplatno.

Laprdus, započet kao moj osobni hobi projekt, također je zamišljen kao eksperimentalan, što mi omogućuje da vidim dokle mogu dogurati s njim.

### 1.2 Kome je Laprdus namijenjen?

Laprdus nije namijenjen svima. Dobar je izbor za one koji uživaju u nostalgičnom zvuku retro sintetizatora govora iz 1980-ih i 1990-ih godina.

Možda neće odgovarati korisnicima naviklim na visoku kvalitetu glasa modernih sintetizatora koji koriste umjetnu inteligenciju i neuronske mreže. Međutim, Laprdus bi mogao biti idealan za one kojima su performanse i minimalna potrošnja resursa važniji od kvalitete glasa.

Ukratko: moderni sintetizatori nude kvalitetu po cijenu resursa; Laprdus nudi performanse po cijenu kvalitete.

### 1.3 Podržane platforme

Laprdus je dostupan na sljedećim platformama:

- **Windows 7 do Windows 11** - putem Microsoft SAPI5 standarda, što omogućuje korištenje Laprdusa u svim programima koji podržavaju Windows govor, što uključuje i čitače ekrana poput Narrator-a, NVDA-a i JAWS-a
- **Direktna integracija sa NVDA čitačem ekrana** - dostupan i kao poseban dodatak za besplatan Windows čitač ekrana NVDA
- **Linux** - putem Speech Dispatcher sustava za Orca čitač ekrana, te kao naredbeni program
- **Android** - kao ugrađen sintetizator govora za Android uređaje

### 1.4 Mogućnosti

Laprdus nudi sljedeće mogućnosti:

- Govor na hrvatskom i srpskom jeziku (podržano latinično i ćirilično pismo)
- Pet različitih glasova (dva osnovna i tri izvedena)
- Podešavanje brzine govora
- Podešavanje visine glasa
- Podešavanje glasnoće
- Prirodna intonacija koja prati interpunkciju
- Čitanje punih brojeva ili znamenku po znamenku
- Podešavanje trajanja pauza za različite interpunkcijske znakove
- Korisničke rječnike za prilagodbu izgovora riječi, fraza, simbola i emodžija

---

## 2. Instalacija

### 2.1 Windows SAPI5

Da biste instalirali Laprdus na Windows sustavu, slijedite ove korake:

#### Korak 1: Preuzimanje

1. Posjetite službenu stranicu Laprdusa: https://hrvojekatic.com/laprdus
2. Pronađite odjeljak za preuzimanje i kliknite na poveznicu za Windows SAPI5
3. Preuzmite instalacijski program (datoteka s nazivom `Laprdus_SAPI5_Setup.exe`)
4. Datoteka će se spremiti u vašu mapu Preuzimanja (Downloads)

#### Korak 2: Pokretanje instalacije

1. Otvorite Windows Explorer (tipka Windows+E)
2. Strelicom dolje pronađite mapu Preuzimanja i otvorite ju tipkom Enter
3. Pronađite preuzeti instalacijski program Laprdusa (datoteka s nazivom `Laprdus_SAPI5_Setup.exe`)
4. **Preporučeno:** Aplikacijska tipka ili Shift+F10 na datoteku, zatim odaberite "Pokreni kao administrator"
   - Alternativno: Pritisnite Enter za normalno pokretanje
5. Ako se pojavi prozor Kontrole korisničkog računa (UAC), odaberite "Da"

#### Korak 3: Proces instalacije

1. Odaberite jezik instalacije strelicom dolje i pritisnite Enter
2. Odaberite lokaciju instalacije (zadano: `C:\Program Files\Laprdus`) i tipkom Tab odaberite "Sljedeće" pa zatim Enter
3. Označite razmaknicom ako želite da vam se na radnoj površini kreira ikona za Laprdus konfigurator, a zatim tipkom Tab pronađite "Sljedeće" i pritisnite Enter.
4. Tipkom Tab pronađite "Instaliraj", pritisnite Enter i pričekajte završetak instalacije
5. Na kraju instalacije, pritisnite Enter na "Završi" za zatvaranje instalacijskog programa

#### Korak 4: Provjera instalacije

1. Otvorite Upravljačku ploču (Control Panel):
   - Pritisnite tipku Windows
   - Upišite "Upravljačka ploča" i pritisnite Enter
2. Odaberite "Olakšani pristup" > "Prepoznavanje govora"
3. Tipkom Tab pronađite "Tekst u govor" i pritisnite Enter
4. U padajućem izborniku "Odabir glasa" trebali biste vidjeti Laprdus glasove:
   - Laprdus Josip
   - Laprdus Vlado
   - Laprdus Detence
   - Laprdus Baba
   - Laprdus Djedo
   - Laprdus Zvonko
   - Laprdus Stojan
   - Laprdus Mirsad
   - Laprdus Zvonko Orguljaš, Klapa Zvonko, Zvonko Pjevač, Stojan Trubač, Stojan Harmonikaš, Stojan Pevač, Mirsad Sevdalija, Mirsad Sazlija, Mirsad Solist i Zvonko Bećarac (glasovi koji pjevaju)

### 2.2 NVDA dodatak

Za korištenje Laprdusa s NVDA čitačem ekrana:

#### Korak 1: Preuzimanje

1. Posjetite službenu stranicu Laprdusa: https://hrvojekatic.com/laprdus
2. Pronađite odjeljak za preuzimanje i kliknite na poveznicu za NVDA dodatak
3. Preuzmite NVDA dodatak (datoteka s nastavkom `.nvda-addon`)
4. Datoteka će se spremiti u vašu mapu Preuzimanja

#### Korak 2: Instalacija dodatka

1. Otvorite Windows Explorer (tipka Windows+E)
2. Pronađite mapu Preuzimanja
3. Pronađite preuzetu datoteku Laprdus NVDA dodatka (datoteka s nastavkom `.nvda-addon`)
4. Pritisnite Enter na datoteci
   - **Alternativno:** Pritisnite aplikacijsku tipku (ili Shift+F10) za kontekstni izbornik, zatim odaberite "Otvori"
5. NVDA će prikazati dijalog s pitanjem "Želite li instalirati ovaj dodatak?"
6. Tipkom Tab pronađite gumb "Da" i pritisnite Enter, ili jednostavno pritisnite D za DA
7. Pričekajte završetak instalacije

#### Korak 3: Ponovno pokretanje NVDA

1. NVDA će zatražiti ponovno pokretanje
2. Odaberite "Da" za trenutačno ponovno pokretanje
3. Pričekajte da se NVDA ponovno pokrene

#### Korak 4: Odabir Laprdusa kao sintetizatora

**Metoda 1: Putem NVDA izbornika**

1. Otvorite NVDA izbornik: pritisnite NVDA tipku + N (Napomena: NVDA tipka na vašem sustavu može biti Insert ili Caps Lock, zavisno o tome kako ste ju konfigurirali. Ako je NVDA tipka postavljena na Insert, možete pritisnuti Insert + N.)
2. Strelicom dolje pronađite "Opcije" i pritisnite Enter
3. Strelicom dolje pronađite "Postavke" i pritisnite Enter
4. U dijaloškom okviru Postavke, strelicom dolje pronađite kategoriju "Govor"
5. Tipkom Tab pronađite gumb "Promijeni..." koji se nalazi odmah pored opcije "Govorna jedinica"
6. Pritisnite Enter na gumbu "Promijeni..."
7. U popisu sintetizatora strelicom dolje pronađite "Laprdus" i pritisnite Enter
8. Pritisnite Escape za zatvaranje postavki

**Metoda 2: Brza prečica (preporučeno)**

1. Pritisnite prečicu Control + NVDA tipka + S (Napomena: NVDA tipka na vašem sustavu može biti Insert ili Caps Lock, zavisno o tome kako ste ju konfigurirali. Ako je NVDA tipka postavljena na Insert, možete pritisnuti Control + Insert + S.)
2. Otvorit će se dijalog za odabir sintetizatora
3. Strelicom dolje pronađite "Laprdus" i pritisnite Enter

### 2.3 Linux

#### Debian i Ubuntu

Otvorite terminal i pokrenite sljedeću naredbu (zamijenite naziv datoteke s nazivom preuzete .deb datoteke):

```bash
sudo dpkg -i laprdus_amd64.deb
```

#### Fedora

Pokrenite sljedeću naredbu (zamijenite naziv datoteke s nazivom preuzete .rpm datoteke):

```bash
sudo rpm -i laprdus.x86_64.rpm
```

#### Arch Linux

1. Preuzmite PKGBUILD datoteku s web stranice
2. U terminalu se pozicionirajte u direktorij s PKGBUILD datotekom
3. Pokrenite:

```bash
makepkg -si
```

#### Ručna instalacija (iz tarball arhive)

1. Raspakirajte arhivu (zamijenite naziv datoteke s nazivom preuzete arhive):
   ```bash
   tar xf laprdus-linux-x86_64.tar.xz
   cd laprdus-linux-x86_64
   ```

2. Pokrenite instalacijski skript:
   ```bash
   sudo ./install.sh
   ```

#### Konfiguracija za Orca čitač ekrana

Nakon instalacije, Laprdus se automatski konfigurira za Speech Dispatcher. Za korištenje s Orca čitačem ekrana:

1. Za svaki slučaj, preporuča se da ponovno pokrenete Speech Dispatcher:
   ```bash
   systemctl --user restart speech-dispatcher
   ```

2. Otvorite Orca postavke:
   - Pritisnite Insert+Razmaknica za otvaranje postavki (ili Caps Lock + Razmaknica ukoliko je Orca modifikator namješten na Caps Lock ili Laptop layout)
   - Ili u Gnome ili Mate desktop okruženju: Alt+F2, upišite orca -s pa zatim Enter

3. Tipkom Tab ili Shift + Tab pozicionirajte se na kartice postavki, strelicom desno pronađite karticu "Voice" (Glas), a zatim ponavljanjem pritiska na tipku Tab odaberite:
   - Speech system: Pritisnite Razmaknicu za proširenje padajućeg izbornika, Strelicom dolje odaberite Speech Dispatcher ukoliko već nije tako namješteno i pritisnite Enter
   - Speech synthesizer: Pritisnite Razmaknicu za proširenje padajućeg izbornika, Strelicom dolje odaberite Laprdus pa zatim Enter
   - Voice: Pritisnite Razmaknicu za proširenje padajućeg izbornika, Strelicom dolje odaberite željeni glas (josip, vlado, itd.) i pritisnite Enter

4. Tipkom Tab pronađite "Apply" (Primijeni) i zatvorite postavke

### 2.4 Android

#### Korak 1: Preuzimanje APK datoteke

1. Na svom Android uređaju otvorite web preglednik
2. Posjetite: https://hrvojekatic.com/laprdus
3. Preuzmite APK datoteku (datoteka s nastavkom `.apk`)

#### Korak 2: Omogućavanje instalacije iz nepoznatih izvora

Ovo je potrebno samo ako instalirate aplikaciju izvan Google Play trgovine.

**Za Android 8.0 i novije verzije:**

1. Otvorite Postavke na svom uređaju
2. Idite na: Aplikacije > Posebni pristup aplikacijama > Instaliranje nepoznatih aplikacija
   - Ili: Postavke > Sigurnost > Instaliraj nepoznate aplikacije
3. Pronađite web preglednik koji koristite (npr. Chrome)
4. Uključite opciju "Dopusti iz ovog izvora"

**Za starije verzije Androida:**

1. Otvorite Postavke
2. Idite na: Sigurnost
3. Uključite opciju "Nepoznati izvori"

#### Korak 3: Instalacija aplikacije

1. Otvorite upravitelj datoteka na svom uređaju
2. Pronađite preuzetu APK datoteku (obično u mapi "Download" ili "Preuzimanja")
3. Dodirnite APK datoteku
4. Kada se pojavi upit, dodirnite "Instaliraj"
5. Pričekajte završetak instalacije
6. Dodirnite "Otvori" za pokretanje aplikacije ili "Gotovo" za zatvaranje

#### Korak 4: Postavljanje Laprdusa kao zadanog TTS-a

1. Otvorite Postavke na svom uređaju
2. Idite na: Pristupačnost > Tekst u govor (Text-to-speech)
   - Na nekim uređajima: Postavke > Sustav > Jezik i unos > Tekst u govor
   - Na Samsung uređajima: Postavke > Opće upravljanje > Jezik i unos > Tekst u govor
3. Dodirnite "Preferirani mehanizam" ili ikonu zupčanika
4. Odaberite "Laprdus TTS" iz popisa
5. Vratite se na prethodni zaslon i dodirnite "Slušaj primjer" za testiranje

#### Napomena za korisnike TalkBack čitača ekrana

Ako koristite TalkBack, sva sučelja Laprdus aplikacije su potpuno pristupačna:

- Koristite standardne TalkBack geste za navigaciju
- Dvostruki dodir za aktivaciju gumba i opcija
- Povlačenje s dva prsta za pomicanje kroz popise

---

## 3. Uporaba

### 3.1 Osnovna uporaba

Nakon instalacije, Laprdus je automatski dostupan svim programima koji koriste govornu sintezu na vašem sustavu. To uključuje:

- Čitače ekrana (NVDA, Orca, Narrator)
- Web preglednike s mogućnošću čitanja
- Programe za čitanje dokumenata
- Bilo koji program koji koristi Windows SAPI5 ili Android TTS

### 3.2 Naredbena linija (Linux i Windows)

Laprdus uključuje naredbeni program koji možete koristiti za pretvaranje teksta u govor direktno iz terminala.

#### Osnovni primjeri

```bash
# Izgovor teksta
laprdus "Dobar dan!"

# Korištenje drugog glasa
laprdus -v vlado "Zdravo svete!"

# Podešavanje brzine govora
laprdus -r 1.5 "Brži govor"

# Spremanje u WAV datoteku
laprdus -o govor.wav "Tekst za snimanje"

# Čitanje iz datoteke
laprdus -i dokument.txt

# Čitanje iz standardnog ulaza
echo "Tekst" | laprdus
```

#### Opcije naredbenog programa

| Opcija | Opis |
|--------|------|
| `-v, --voice` | Odabir glasa (josip, vlado, detence, baba, djed, zvonko, stojan, mirsad ili pjevajući glas: orguljas, klapa, trubac, harmonikas, sevdalija, sazlija, pjevac, pevac, solist, becarac) |
| `-r, --speech-rate` | Brzina govora (0.5-2.0, zadano: 1.0; formantni glasovi 0.25-4.0) |
| `-p, --speech-pitch` | Visina glasa (0.5-2.0, zadano: 1.0; formantni glasovi 0.25-4.0) |
| `-I, --inflection` | Razina infleksije, 0-100 (zadano: 50; 0 je monotono) |
| `-a, --acceleration` | Ubrzanje, množitelj brzine 0.5-3.0 (zadano: 1.0) |
| `-V, --speech-volume` | Glasnoća (0.0-1.0, zadano: 1.0) |
| `-d, --numbers-digits` | Čitaj brojeve znamenka po znamenka |
| `-s, --spell` | Slovkaj tekst znak po znak |
| `-S, --spelling-speed` | Brzina slovkanja u postocima brzine govora, 0-100 (zadano: 50) |
| `-m, --spelling-mode` | Slovkanje: `names` (nazivi slova: be, ce, de; zadano) ili `sounds` (glasovi slova: b, c, d) |
| `-c, --comma-pauses` | Trajanje pauze za zarez u ms (zadano: 100) |
| `-e, --period-pauses` | Trajanje pauze za točku u ms (zadano: 80) |
| `-x, --exclamationmark-pauses` | Trajanje pauze za uskličnik u ms (zadano: 70) |
| `-q, --questionmark-pauses` | Trajanje pauze za upitnik u ms (zadano: 60) |
| `-n, --newline-pauses` | Trajanje pauze za novi red u ms (zadano: 100) |
| `-D, --data-dir` | Direktorij s glasovnim podacima |
| `-o, --output-file` | Spremi govor u WAV datoteku |
| `-i, --input-file` | Učitaj tekst iz datoteke |
| `-l, --list-voices` | Prikaži popis dostupnih glasova |
| `-w, --verbose` | Opširniji ispis (za dijagnostiku) |
| `-h, --help` | Prikaži pomoć |

### 3.3 NVDA postavke

Kada koristite Laprdus s NVDA čitačem ekrana, postavke glasa možete promijeniti na sljedeći način:

1. Otvorite NVDA izbornik (NVDA tipka + N)
2. Odaberite Opcije podizbornik pa zatim Postavke > kategorija Govor
3. U dijalogu možete podesiti:
   - Glas (odaberite jedan od 5 Laprdus glasova)
   - Brzinu govora
   - Visinu glasa
   - Glasnoću
   - Dodatnu brzinu ili Rate Boost (proširuje maksimalnu brzinu s 2x na 4x)

### 3.4 Android aplikacija

Laprdus Android aplikacija uključuje zaslon s postavkama gdje možete:

1. Odabrati glas
2. Podesiti brzinu govora
3. Podesiti visinu glasa
4. Podesiti glasnoću
5. Podesiti mogućnost čitanja emodžija
6. Podesiti način čitanja brojeva
7. Podesiti trajanje pauza i brzinu slovkanja
8. Odabrati slovkanje nazivima slova ili glasovima

Za pristup postavkama:

1. Otvorite Laprdus aplikaciju iz ladice aplikacija
2. Pritisnite na gumb "Laprdus postavke"
3. Promijenite postavke po želji (Promjene se automatski spremaju)

---

## 4. Postavke

### 4.1 Brzina govora

Brzina govora određuje koliko brzo Laprdus izgovara tekst. Raspon je od 0.5 (upola sporije) do 2.0 (dvostruko brže), pri čemu je 1.0 normalna brzina.

- **0.5** - Vrlo sporo, korisno za početnike ili kada trebate pažljivo slušati svaku riječ
- **1.0** - Normalna brzina, prikladna za većinu situacija
- **1.5** - Umjereno brzo, za iskusne korisnike
- **2.0** - Brzo, za napredne korisnike

Napomena: U NVDA dodatku, opcija "Dodatna brzina" ili "Rate Boost" proširuje maksimalnu brzinu do 4.0.

**Brzina u Laprdusu i brzina čitača ekrana.** Brzina iz postavki Laprdusa normalna je brzina glasa, a čitač ekrana ili aplikacija (TalkBack, VoiceOver, NVDA, programi koji koriste SAPI5, Speech Dispatcher) svoju brzinu zadaje u odnosu na nju. Dok čitač ekrana govori svojom normalnom brzinom, Laprdus govori upravo brzinom iz svojih postavki; dvostruko brži čitač ekrana daje dvostruko tu brzinu. Zato govor ubrzavaju i usporavaju i klizač brzine u Laprdusu i klizač u čitaču ekrana. Prekidač „Prisilno koristi Laprdus brzinu” (u Windows Konfiguratoru „Prisili Laprdusovu brzinu govora za sve aplikacije”) zanemaruje brzinu koju traži čitač ekrana ili aplikacija, pa Laprdus govori samo brzinom iz svojih postavki. [Ubrzanje](#46-infleksija) na kraju množi tu brzinu.

### 4.2 Visina glasa

Visina glasa omogućuje podešavanje osnovne frekvencije glasa. Raspon je od 0.5 (niži glas) do 2.0 (viši glas), pri čemu je 1.0 prirodna visina glasa. Kao i brzina (vidi [4.1](#41-brzina-govora)), visina iz postavki Laprdusa normalna je visina glasa koju čitač ekrana podiže ili spušta, osim kad je uključen prekidač „Prisilno koristi Laprdus visinu”.

- **0.5** - Niži glas
- **1.0** - Prirodna visina
- **2.0** - Viši glas

Napomena: Podešavanje visine glasa ne utječe na karakter glasa (ne dolazi do "efekta crtića").

### 4.3 Glasnoća

Glasnoća određuje koliko će govor biti glasan. Raspon je od 0.0 (tiho) do 1.0 (maksimalna glasnoća).

- **0.0** - Tiho (nema zvuka)
- **0.5** - Umjerena glasnoća
- **1.0** - Maksimalna glasnoća

### 4.4 Pauze

Laprdus automatski ubacuje pauze nakon interpunkcijskih znakova kako bi govor zvučao prirodnije. Možete podesiti trajanje pauza za:

- **Točka** - Pauza nakon točke (zadano: 100 ms u korisničkim postavkama, 80 ms u naredbenom programu)
- **Uskličnik** - Pauza nakon uskličnika (zadano: 100 ms u korisničkim postavkama, 70 ms u naredbenom programu)
- **Upitnik** - Pauza nakon upitnika (zadano: 100 ms u korisničkim postavkama, 60 ms u naredbenom programu)
- **Zarez** - Kratka pauza unutar rečenice (zadano: 100 ms)
- **Novi red** - Pauza za prijelom retka (zadano: 100 ms)

Vrijednosti se izražavaju u milisekundama (ms). Raspon je od 0 do 2000 ms.

**Napomena:** Korisnički konfigurator (Windows/Android) koristi jedinstveno podešavanje od 100 ms za sve znakove kraja rečenice (točka, uskličnik, upitnik), dok naredbeni program omogućuje zasebno podešavanje svakog znaka.

U istom odjeljku postavki nalazi se i **brzina slovkanja**: brzina kojom se izgovaraju znakovi pri slovkanju i pri tipkanju, u postocima brzine govora. 100% je brzina govora, 50% (zadano) nešto više od polovice, a 0% trećina. Nazivi i glasovi slova kratki su, pa se pri brzini tekućeg govora stapaju; polovica brzine zlatna je sredina. Pauza između slovkanih znakova (zadano 200 ms, `pauses.spelling` u `settings.json`) ne ovisi o toj brzini.

### 4.5 Način čitanja brojeva

Laprdus može čitati brojeve na dva načina:

1. **Riječima** (zadano) - Brojevi se čitaju kao pune riječi
   - Primjer: "123" se čita kao "sto dvadeset tri"

2. **Znamenkama** - Brojevi se čitaju znamenka po znamenka
   - Primjer: "123" se čita kao "jedan dva tri"

Način čitanja riječima podržava brojeve do centiljuna (10^303) i koristi ispravne hrvatske gramatičke oblike.

### 4.6 Infleksija

Infleksija je prirodna promjena visine glasa koja prati interpunkciju:

- Točka na kraju rečenice uzrokuje pad visine glasa
- Upitnik uzrokuje porast visine glasa na kraju
- Uskličnik daje naglasak
- Zarez uzrokuje blagi porast visine glasa

Veličinu infleksije određuje klizač, a prekidača za uključivanje i isključivanje iz inačice 1.0 više nema: monoton govor dobiva se klizačem na 0%. Za sve glasove vrijede dvije postavke:

- **Razina infleksije** (0-100%) određuje veličinu svih pomaka visine: 0% je monoton govor, 50% prirodna melodija, 100% udvostručuje svaki pomak.
- **Ubrzanje** (0,5-3) množi brzinu govora, pa klizač brzine u čitaču ekrana ili sustavu dosegne veću (ili manju) najveću brzinu. Josip, Vlado i glasovi izvedeni iz njih zaustavljaju se na četverostrukoj brzini; Zvonko, Stojan i Mirsad idu do osmerostruke.

### 4.7 Slovkanje

Kad čitač ekrana slovka tekst ili izgovara znakove dok tipkate, Laprdus svako slovo čita na jedan od dva načina:

- **Nazivima slova** (zadano): *a, be, ce, če, će, de, dže, đe, e, ef, ge, ha, i, je, ka, el, elj, em, en, enj, o, pe, er, es, eš, te, u, ve, ze, že*; strana slova *ku, duplo ve, iks, ipsilon*. To su nazivi iz hrvatskog pravopisa, a vrijede i za bosanski glas. Srpski glasovi (Stojan, Vlado, Đedo) slijede odluku Odbora za standardizaciju srpskog jezika: *lje, nje* i *ša* umjesto *elj, enj* i *eš*, ostala slova jednako, ćirilična kao i latinična (*Љ* je *lje*, *Ш* je *ša*).
- **Glasovima slova**: umjesto naziva izgovara se glas koji slovo označava, kao kad dijete uči čitati i kao što slova slovka hrvatski eSpeak: suglasnik i za njim kratak neutralni samoglasnik (*bə, cə, čə, fə, sə, mə*), samoglasnici kao samoglasnici. Strana slova *q, w, x* i *y* nisu slova naše abecede, pa se i ovdje čitaju nazivima (*ku, duplo ve, iks, ipsilon*). Engleski nazivi i engleski izgovor ne koriste se.

Način slovkanja prekidač je „Slovkaj slova glasovima” u naprednim postavkama (Windows: skupina Opcije u Konfiguratoru; Android i Apple: odjeljak Napredno; NVDA: Konfigurator; naredbeni program: `-m`). Odnosi se samo na slovkanje i tipkanje; riječi se i dalje izgovaraju normalno. Brojevi, interpunkcija i simboli u oba se načina čitaju imenom iz rječnika slovkanja. Brzina slovkanja opisana je u [4.4](#44-pauze).

**Velika slova.** Ako je čitač ekrana podešen da najavljuje velika slova riječju („veliko N” u NVDA-u, „veliko slovo N” u TalkBacku, „veliko početno slovo N” u VoiceOveru, „cap N” na engleskom), Laprdus riječ izgovara normalno, a slovo slovka kao i svako drugo: nazivom ili glasom, prema odabranom načinu, i brzinom slovkanja.

**Rječnik slovkanja i glasovi slova.** Vaši unosi u rječniku slovkanja (odjeljak [5.4](#54-rječnik-slovkanja-spellingjson)) vrijede u oba načina, s jednom iznimkom: kad se slova slovkaju glasovima, unos koji je samo naziv slova (*be*, *jot*, *lje*) također je naziv i ustupa mjesto glasu, dok se unos koji kaže nešto drugo (na primjer *B* → *bum*) izgovara kako ste ga napisali. Tako stari unosi kojima ste ispravljali nazive ne isključuju glasove.

### 4.8 Laprdus Konfigurator (Windows)

Na Windows sustavu, dodatne postavke možete podesiti putem Laprdus Konfiguratora:

1. Otvorite Windows Start izbornik, a zatim pronađite Laprdus konfigurator u programskoj mapi Laprdus i otvorite ga tipkom Enter.
2. Ili ako vam je Laprdus instaliran kao NVDA dodatak, otvorite NVDA izbornik > Laprdus podizbornik > Laprdus Konfigurator

Konfigurator omogućuje podešavanje:
- Brzine, visine i glasnoće govora (brzinu i visinu NVDA klizači ubrzavaju, usporavaju, podižu i spuštaju, osim kad je uključeno prisilno korištenje Laprdusovih vrijednosti; vidi [4.1](#41-brzina-govora))
- Intonacije i ubrzanja, za sve glasove: intonacija od 0% (monotono) preko 50% (zadano, prirodni pokreti) do 100% (dvostruko veći pokreti); ubrzanje množi brzinu govora (0.5x-3.0x, zadano 1.0x), pa vrh NVDA klizača brzine doseže veću ili manju brzinu, a uz klizač je ispisano koliko riječi u minuti tada doseže. Klizači brzine i visine za formantne glasove idu od 0.25x do 4.0x.
- Načina čitanja brojeva
- Trajanja pauza i brzine slovkanja
- Slovkanja slova glasovima umjesto nazivima
- Prilagodbu korisničkih rječnika

Postavke se spremaju u korisničkoj mapi `%APPDATA%\Laprdus` i dijele se između SAPI5 i NVDA dodatka.

---

## 5. Rječnici

Rječnici omogućuju prilagodbu načina na koji Laprdus izgovara određene riječi, kratice, simbole i emodžije. Pomoću rječnika možete ispraviti izgovor stranih riječi, dodati izgovor za kratice ili prilagoditi čitanje posebnih znakova.

### 5.1 Vrste rječnika

Laprdus koristi tri vrste rječnika, od kojih svaki ima posebnu namjenu, i uz njih naglasni rječnik formantnih glasova:

| Vrsta rječnika | Datoteka | Namjena |
|----------------|----------|---------|
| **Glavni rječnik** | `user.json` | Zamjena riječi i fraza prilagođenim izgovorom |
| **Rječnik slovkanja** | `spelling.json` | Izgovor pojedinačnih znakova (slova, brojeva, simbola) |
| **Rječnik emodžija** | `emoji.json` | Pretvaranje emodžija u tekstualni opis |
| **Naglasni rječnik** | `accents.json` | Naglasak, dužina i ton riječi za formantne glasove (vidi [5.11](#511-naglasni-rječnik-accentsjson)) |

### 5.2 Lokacije datoteka rječnika

Lokacija datoteka rječnika ovisi o platformi koju koristite:

#### Windows (SAPI5 i NVDA)

Sve datoteke rječnika nalaze se u korisničkom direktoriju Laprdusa:

```
%APPDATA%\Laprdus\
```

Puna putanja je obično: `C:\Users\VašeKorisničkoIme\AppData\Roaming\Laprdus\`

**Kako otvoriti ovaj direktorij:**

1. Pritisnite tipke Windows+R za otvaranje dijaloga "Pokreni"
2. Upišite: `%APPDATA%\Laprdus`
3. Pritisnite Enter

Datoteke u ovom direktoriju:

| Datoteka | Opis |
|----------|------|
| `settings.json` | Glavne postavke Laprdusa |
| `user.json` | Vaš korisnički rječnik izgovora |
| `spelling.json` | Rječnik slovkanja (izgovor znakova) |
| `emoji.json` | Rječnik emodžija |
| `accents.json` | Naglasni rječnik formantnih glasova |

**Napomena:** Ove datoteke se stvaraju automatski kada prvi put dodate unos putem Laprdus Konfiguratora. Također ih možete stvoriti ručno.

#### Linux

Datoteke rječnika nalaze se u:

```
~/.config/Laprdus/
```

Koristite iste nazive datoteka kao na Windowsu (`user.json`, `spelling.json`, `emoji.json`, `accents.json`).

#### Android

Na Androidu, rječnici se uređuju isključivo putem sučelja Laprdus aplikacije. Datoteke se pohranjuju u internoj memoriji aplikacije i nisu izravno dostupne korisniku.

### 5.3 Glavni rječnik (user.json)

Glavni rječnik služi za zamjenu riječi ili fraza prilagođenim izgovorom. Koristan je za:

- **Strane riječi i brandove** - npr. "Facebook" izgovarati kao "Fejzbuk"
- **Kratice** - npr. "TV" izgovarati kao "Te Ve"
- **Tehnički pojmovi** - npr. "ChatGPT" izgovarati kao "ČetDžipiti"
- **Imena s neuobičajenim izgovorom** - npr. "Sean" izgovarati kao "Šon"

#### Format datoteke

Datoteka `user.json` koristi JSON format. Evo primjera:

```json
{
    "version": "1.0",
    "entries": [
        {
            "grapheme": "Facebook",
            "phoneme": "Fejzbuk",
            "caseSensitive": false,
            "wholeWord": true,
            "comment": "Društvena mreža"
        },
        {
            "grapheme": "TV",
            "phoneme": "Te Ve",
            "caseSensitive": false,
            "wholeWord": true,
            "comment": "Kratica za televiziju"
        },
        {
            "grapheme": "ChatGPT",
            "phoneme": "ČetDžipiti",
            "caseSensitive": false,
            "wholeWord": true,
            "comment": "AI chatbot"
        }
    ]
}
```

#### Opis polja

| Polje | Obavezno | Opis | Zadana vrijednost |
|-------|----------|------|-------------------|
| `grapheme` | Da | Izvorni tekst koji se zamjenjuje | - |
| `phoneme` | Da | Zamjenski izgovor | - |
| `caseSensitive` | Ne | Razlikuje li se velika i mala slova | `false` |
| `wholeWord` | Ne | Zamjenjuje samo cijelu riječ | `true` |
| `comment` | Ne | Komentar za referencu (ne utječe na izgovor) | - |

#### Objašnjenje opcija

**caseSensitive (razlikovanje velikih i malih slova):**

- `false` (zadano): "Facebook", "facebook", "FACEBOOK" će svi biti zamijenjeni
- `true`: Samo točno podudaranje će biti zamijenjeno

**wholeWord (samo cijela riječ):**

- `true` (zadano): Zamjenjuje samo kada je tekst cijela riječ
  - "TV" u "TV program" će biti zamijenjeno
  - "TV" u "aktivator" neće biti zamijenjeno (jer je dio veće riječi)
- `false`: Zamjenjuje i unutar drugih riječi
  - Korisno za sufikse i prefikse

### 5.4 Rječnik slovkanja (spelling.json)

Rječnik slovkanja definira kako se izgovaraju pojedinačni znakovi kada čitač ekrana slovka tekst (čitanje znak po znak). Koristan je za:

- **Brojeve** - npr. "5" izgovarati kao "pet"
- **Interpunkciju** - npr. "." izgovarati kao "točka"
- **Posebne znakove** - npr. "@" izgovarati kao "at"
- **Slova abecede** - ako želite da se neko slovo zove drukčije nego što ga Laprdus zove sam, npr. "W" kao "dvostruko ve"

Slova abecede Laprdus imenuje sam, na jeziku glasa (vidi [4.7](#47-slovkanje)), pa ih ugrađeni rječnik ne sadrži; vaš unos za slovo ima prednost pred ugrađenim nazivom. Kad je uključeno slovkanje glasovima, unos koji je samo naziv slova zanemaruje se, a svaki drugi unos vrijedi.

#### Format datoteke

```json
{
    "version": "1.0",
    "entries": [
        { "character": "A", "pronunciation": "A" },
        { "character": "B", "pronunciation": "Be" },
        { "character": "Č", "pronunciation": "Če" },
        { "character": "1", "pronunciation": "jedan" },
        { "character": ".", "pronunciation": "točka" },
        { "character": "@", "pronunciation": "at" }
    ]
}
```

#### Ugrađeni izgovori znakova

Laprdus dolazi s ugrađenim rječnikom slovkanja brojeva, interpunkcije i simbola (nazivi su hrvatski). Slova se imenuju ovako:

**Slova abecede** (ugrađeno u Laprdus, ne u rječnik):

| Slovo | Hrvatski i bosanski | Srpski | Slovo | Hrvatski i bosanski | Srpski |
|-------|---------------------|--------|-------|---------------------|--------|
| A | a | a | N | en | en |
| B | be | be | NJ, Њ | enj | nje |
| C | ce | ce | O | o | o |
| Č | če | če | P | pe | pe |
| Ć | će | će | R | er | er |
| D | de | de | S | es | es |
| DŽ, Џ | dže | dže | Š | eš | ša |
| Đ | đe | đe | T | te | te |
| E | e | e | U | u | u |
| F | ef | ef | V | ve | ve |
| G | ge | ge | Z | ze | ze |
| H | ha | ha | Ž | že | že |
| I | i | i | Q | ku | ku |
| J | je | je | W | duplo ve | duplo ve |
| K | ka | ka | X | iks | iks |
| L | el | el | Y | ipsilon | ipsilon |
| LJ, Љ | elj | lje | | | |

Ćirilična slova imenuju se kao njihovi latinični parnjaci, a velika i mala slova jednako.

**Brojevi:**

| Znak | Izgovor |
|------|---------|
| 0 | nula |
| 1 | jedan |
| 2 | dva |
| 3 | tri |
| 4 | četiri |
| 5 | pet |
| 6 | šest |
| 7 | sedam |
| 8 | osam |
| 9 | devet |

**Uobičajena interpunkcija:**

| Znak | Izgovor |
|------|---------|
| . | točka |
| , | zarez |
| ! | uskličnik |
| ? | upitnik |
| : | dvotočka |
| ; | točka zarez |
| - | crtica |
| _ | donja crtica |
| ( | otvorena zagrada |
| ) | zatvorena zagrada |
| " | navodnik |
| ' | apostrof |

**Posebni znakovi:**

| Znak | Izgovor |
|------|---------|
| @ | at |
| # | ljestve |
| $ | dolar |
| % | posto |
| & | i |
| * | zvjezdica |
| + | plus |
| = | jednako |
| / | kosa crta |
| \ | obrnuta kosa crta |

### 5.5 Rječnik emodžija (emoji.json)

Rječnik emodžija pretvara emoji simbole u govorni tekst.

#### Uključivanje čitanja emodžija

**Windows (Laprdus Konfigurator):**

1. Otvorite Laprdus Konfigurator (Start > Laprdus > Laprdus Konfigurator)
2. Tipkom Tab pronađite opciju "Čitaj emodžije"
3. Označite potvrdni okvir
4. Tipkom Tab odaberite "U redu" i pritisnite Enter

**NVDA:**

1. Otvorite NVDA izbornik (NVDA tipka + N) > Laprdus podizbornik > Laprdus Konfigurator
2. Tipkom Tab pronađite opciju "Čitaj emodžije"
3. Označite potvrdni okvir
4. Tipkom Tab odaberite "U redu" i pritisnite Enter

**Android:**

1. Otvorite Laprdus aplikaciju
2. Dodirnite "Laprdus postavke"
3. Pronađite prekidač "Čitaj emodžije" i uključite ga

#### Format datoteke

```json
{
    "version": "1.0",
    "entries": [
        { "emoji": "😀", "text": "nasmijano lice" },
        { "emoji": "👍", "text": "palac gore" },
        { "emoji": "❤️", "text": "crveno srce" },
        { "emoji": "🎉", "text": "konfeti" }
    ]
}
```

#### Ugrađeni emodžiji

Laprdus dolazi s opsežnim ugrađenim rječnikom emodžija koji sadrži preko 1100 emodžija s hrvatskim opisima. Evo nekoliko primjera:

| Emodži | Opis |
|--------|------|
| 😀 | nasmijano lice |
| 😂 | lice sa suzama radosnicama |
| 😍 | nasmiješeno lice s očima u obliku srca |
| 🤔 | zamišljeno lice |
| 👍 | palac gore |
| 👎 | palac dolje |
| ❤️ | crveno srce |
| 🔥 | vatra |
| ⭐ | zvijezda |
| ✅ | kvačica |

### 5.6 Uređivanje rječnika na Windowsu (Laprdus Konfigurator)

Laprdus Konfigurator pruža grafičko sučelje za jednostavno upravljanje rječnicima. Ovo je **preporučena metoda** za korisnike koji preferiraju grafičko sučelje.

**Kako otvoriti Konfigurator:**

- **Iz Start izbornika:** Start > Laprdus > Laprdus Konfigurator
- **Iz NVDA izbornika:** NVDA tipka + N > Laprdus podizbornik > Laprdus Konfigurator

**Uređivanje rječnika:**

1. U Laprdus Konfiguratoru tipkom Tab ili Shift+Tab pronađite gumb **"Rječnici..."** i pritisnite Enter
2. Otvorit će se prozor Uređivač rječnika
3. Na vrhu prozora nalazi se padajući izbornik za odabir vrste rječnika koji želite prilagoditi:
   - **Glavni rječnik** - za zamjenu riječi i fraza
   - **Rječnik slovkanja** - za izgovor pojedinačnih znakova
   - **Rječnik emodžija** - za pretvaranje emodžija u tekst
4. Odaberite željenu vrstu rječnika strelicom dolje

**Dodavanje novog unosa:**

1. Kliknite gumb **"Dodaj..."**
2. Otvorit će se dijalog za unos:
   - **Originalan (izvorni) niz:** Upišite tekst koji želite zamijeniti
   - **Zamjenski niz:** Upišite kako želite da se tekst izgovara
   - **Razlikuj velika/mala slova:** Označite ako je bitno točno podudaranje
   - **Samo cijela riječ:** Označite ako ne želite zamjenu unutar drugih riječi
   - **Komentar:** Opcionalni opis unosa
3. Pritisnite Enter ili tipkom Tab pronađite gumb **"U redu"** pa zatim potvrdite tipkom Enter za spremanje unosa

**Uređivanje postojećeg unosa:**

1. Odaberite unos u popisu krećući se strelicama dolje ili gore
2. Tipkom Tab locirajte gumb **"Uredi..."**, a za korisnike miša moguć je i dvoklik na unos
3. Izmijenite željene vrijednosti
4. Pritisnite Enter, ili tipkom Tab pronađite gumb **"U redu"** i pritisnite Enter

**Brisanje unosa:**

1. Odaberite unos u popisu strelicama gore ili dolje
2. Tipkom Tab locirajte gumb **"Izbriši"** i pritisnite Enter

**Dupliciranje unosa:**

1. Odaberite unos koji želite duplicirati
2. Tipkom Tab locirajte gumb **"Dupliciraj"** i pritisnite Enter
3. Novi unos s kopiranim vrijednostima bit će dodan u popis
4. Uredite duplicirani unos prema potrebi

### 5.7 Uređivanje rječnika na Androidu

Android aplikacija pruža ugrađeno sučelje za upravljanje rječnicima.

**Korak 1: Otvorite rječnike**

1. Otvorite Laprdus aplikaciju iz ladice aplikacija
2. Dodirnite gumb **"Laprdus postavke"**
3. Dodirnite gumb **"Upravljanje rječnicima"**

**Korak 2: Odaberite vrstu rječnika**

Na vrhu zaslona nalazi se padajući izbornik za odabir vrste rječnika:

- **Glavni rječnik** - za zamjenu riječi i fraza
- **Rječnik slovkanja** - za izgovor znakova
- **Rječnik emodžija** - za pretvaranje emodžija

Dodirnite padajući izbornik i odaberite željenu vrstu.

**Korak 3: Dodavanje novog unosa**

1. Dodirnite gumb **"+"** (plus) u donjem desnom kutu zaslona
2. Otvorit će se zaslon za unos:
   - **Izvorni tekst:** Upišite riječ ili frazu koju želite zamijeniti
   - **Zamjenski izgovor:** Upišite kako želite da se izgovara
   - **Razlikuj velika i mala slova:** Uključite ako je bitno točno podudaranje
   - **Samo cijela riječ:** Uključite ako ne želite zamjenu unutar drugih riječi
   - **Komentar:** Opcionalni opis (npr. "Društvena mreža")
3. Dodirnite gumb **"Spremi"** na dnu zaslona

**Korak 4: Uređivanje postojećeg unosa**

1. Pronađite unos u popisu
2. Dodirnite unos za otvaranje zaslona za uređivanje
3. Izmijenite željene vrijednosti
4. Dodirnite **"Spremi"**

**Korak 5: Brisanje unosa**

1. Pronađite unos u popisu
2. Povucite unos ulijevo za prikaz opcije brisanja
3. Dodirnite ikonu koša za smeće
4. Potvrdite brisanje

**Napomena za TalkBack korisnike:**

- Koristite standardne TalkBack geste za navigaciju
- Dvostruki dodir za aktivaciju gumba
- Za brisanje: dvostruki dodir i zadržavanje na unosu, zatim odaberite "Izbriši"

### 5.8 Ručno uređivanje JSON datoteka (napredno)

Ova metoda je namijenjena **naprednim korisnicima** koji žele izravno uređivati JSON datoteke rječnika. Za većinu korisnika preporučujemo korištenje Laprdus Konfiguratora (Windows) ili sučelja Android aplikacije.

**Prednosti ručnog uređivanja:**
- Brže masovno dodavanje/uređivanje velikog broja unosa
- Mogućnost dijeljenja rječnika s drugim korisnicima
- Stvaranje sigurnosnih kopija rječnika

**Korak 1: Otvorite direktorij s rječnicima**

**Windows:**
1. Pritisnite Windows+R
2. Upišite: `%APPDATA%\Laprdus`
3. Pritisnite Enter

**Linux:**
1. Otvorite upravitelj datoteka
2. Otvorite skrivenu mapu `.config` u vašem osobnom direktoriju
3. Otvorite mapu `Laprdus`
   - Ili u terminalu: `cd ~/.config/Laprdus`

**Korak 2: Otvorite ili stvorite datoteku**

- Ako datoteka već postoji, otvorite je u tekstualnom editoru (Windows: Notepad, Linux: gedit, nano, vim)
- Ako datoteka ne postoji, stvorite novu tekstualnu datoteku s odgovarajućim imenom:
  - `user.json` za glavni rječnik
  - `spelling.json` za rječnik slovkanja
  - `emoji.json` za rječnik emodžija
  - `accents.json` za naglasni rječnik formantnih glasova (zapis u [5.11](#511-naglasni-rječnik-accentsjson))

**Korak 3: Uredite sadržaj**

Koristite JSON format opisan u prethodnim odjeljcima (5.3, 5.4, 5.5). Primjer za `user.json`:

```json
{
    "version": "1.0",
    "entries": [
        {
            "grapheme": "YouTube",
            "phoneme": "Jutjub",
            "caseSensitive": false,
            "wholeWord": true,
            "comment": "Video platforma"
        },
        {
            "grapheme": "Google",
            "phoneme": "Gugl",
            "caseSensitive": false,
            "wholeWord": true,
            "comment": "Tražilica"
        }
    ]
}
```

**VAŽNO - Pravila JSON sintakse:**
- Svi ključevi i tekstualne vrijednosti moraju biti unutar dvostrukih navodnika `"`
- Zarezi `,` između unosa, ALI NE nakon zadnjeg unosa
- Vitičaste zagrade `{}` za svaki unos
- Uglate zagrade `[]` za listu unosa

**Korak 4: Spremite datoteku**

- **Obavezno** spremite datoteku s **UTF-8 kodiranjem** (za podršku hrvatskih znakova)
- **Windows Notepad:** Datoteka > Spremi kao > Encoding: UTF-8
- **Linux:** Većina editora automatski koristi UTF-8

**Korak 5: Ponovno pokrenite sintetizator**

- **Windows SAPI5:** Zatvorite i ponovno otvorite aplikaciju koja koristi Laprdus
- **NVDA:** Pritisnite NVDA tipku + Q i ponovno pokrenite NVDA, ili prijeđite na drugi sintetizator pa se vratite na Laprdus
- **Linux:** Ponovno pokrenite aplikaciju ili Speech Dispatcher: `systemctl --user restart speech-dispatcher`

**Napomena:** Ako uredite rječnik dok je Android aplikacija pokrenuta, promjene neće biti vidljive sve dok ne osvježite popis ili ponovno ne otvorite aplikaciju.

### 5.9 Primjeri praktične uporabe

#### Primjer 1: Strane tvrtke i brandovi

Mnoge strane tvrtke i brandovi imaju izgovor koji se razlikuje od pisanog oblika:

```json
{
    "grapheme": "Microsoft",
    "phoneme": "Majkrosoft",
    "caseSensitive": false,
    "wholeWord": true
}
```

```json
{
    "grapheme": "Google",
    "phoneme": "Gugl",
    "caseSensitive": false,
    "wholeWord": true
}
```

```json
{
    "grapheme": "WhatsApp",
    "phoneme": "Watsap",
    "caseSensitive": false,
    "wholeWord": true
}
```

#### Primjer 2: Kratice

Kratice koje želite da se čitaju slovo po slovo:

```json
{
    "grapheme": "TV",
    "phoneme": "Te Ve",
    "caseSensitive": false,
    "wholeWord": true
}
```

```json
{
    "grapheme": "HR",
    "phoneme": "Ha Er",
    "caseSensitive": false,
    "wholeWord": true,
    "comment": "Kratica za Hrvatsku"
}
```

```json
{
    "grapheme": "EU",
    "phoneme": "E U",
    "caseSensitive": false,
    "wholeWord": true,
    "comment": "Europska unija"
}
```

#### Primjer 3: Tehnički pojmovi

```json
{
    "grapheme": "ChatGPT",
    "phoneme": "ČetDžipiti",
    "caseSensitive": false,
    "wholeWord": true
}
```

```json
{
    "grapheme": "WiFi",
    "phoneme": "Vajfaj",
    "caseSensitive": false,
    "wholeWord": true
}
```

```json
{
    "grapheme": "USB",
    "phoneme": "U Es Be",
    "caseSensitive": false,
    "wholeWord": true
}
```

#### Primjer 4: Imena s neuobičajenim izgovorom

```json
{
    "grapheme": "Sean",
    "phoneme": "Šon",
    "caseSensitive": false,
    "wholeWord": true,
    "comment": "Irsko ime"
}
```

```json
{
    "grapheme": "Elon",
    "phoneme": "Ilon",
    "caseSensitive": false,
    "wholeWord": true
}
```

#### Primjer 5: Zamjena unutar riječi (wholeWord: false)

Za zamjenu sufiksa ili prefiksa, postavite `wholeWord` na `false`:

```json
{
    "grapheme": "tion",
    "phoneme": "šen",
    "caseSensitive": false,
    "wholeWord": false,
    "comment": "Engleski sufiks -tion"
}
```

S ovim unosom, riječi poput "information" će sadržavati izgovor "šen" umjesto "tion".

### 5.10 Rješavanje problema s rječnicima

#### Problem: Promjene u rječniku se ne primjenjuju

**Mogući uzroci i rješenja:**

1. **Primjena korisničkih rječnika je isključena**
   - Uključite opciju **"Primjena korisničkih rječnika"** u Laprdus konfiguratoru (Windows) ili u Laprdus postavkama (Android)

2. **Sintaksna greška u JSON datoteci**
   - Provjerite jesu li svi navodnici ispravno zatvoreni
   - Provjerite jesu li zarezi na pravim mjestima
   - Koristite online JSON validator za provjeru sintakse

3. **Datoteka nije spremljena s UTF-8 kodiranjem**
   - U Notepadu: Datoteka > Spremi kao > Encoding: UTF-8
   - U drugim editorima: provjerite postavke kodiranja

4. **Sintetizator nije ponovno pokrenut**
   - Zatvorite i ponovno otvorite aplikaciju
   - Za NVDA: Insert+Q > Ponovno pokreni NVDA

5. **Datoteka je na krivoj lokaciji**
   - Windows: `%APPDATA%\Laprdus\`
   - Linux: `~/.config/Laprdus/`

#### Problem: Riječ se zamjenjuje na krivim mjestima

**Rješenje:** Uključite opciju **"Samo cijela riječ"** za problematični unos, ili ručno promijenite vrijednost za taj unos u konfiguracijskoj datoteci, `wholeWord: true`

Primjer problema: Unos za "TV" zamjenjuje i "aktivator" jer sadrži "TV".

```json
{
    "grapheme": "TV",
    "phoneme": "Te Ve",
    "wholeWord": true
}
```

#### Problem: Zamjena ne radi za velika/mala slova

**Rješenje:** Provjerite opciju **"Osjetljivost na velika i mala slova"** za problematičan unos, ili provjerite upis u konfiguracijskoj datoteci za taj unos, `caseSensitive`

- Za zamjenu neovisno o veličini slova: `caseSensitive: false` odnosno Isključeno
- Za točno podudaranje: `caseSensitive: true` odnosno Uključeno

#### Problem: JSON sintaksna greška

Česte greške u JSON formatu:

**Zaboravljeni zarez:**
```json
// POGREŠNO:
{
    "grapheme": "test"
    "phoneme": "test"
}

// ISPRAVNO:
{
    "grapheme": "test",
    "phoneme": "test"
}
```

**Zarez na kraju:**
```json
// POGREŠNO:
{
    "entries": [
        { "grapheme": "a", "phoneme": "b" },
    ]
}

// ISPRAVNO:
{
    "entries": [
        { "grapheme": "a", "phoneme": "b" }
    ]
}
```

**Nedostaju vitičaste zagrade:**
```json
// POGREŠNO:
"grapheme": "test",
"phoneme": "test"

// ISPRAVNO:
{
    "grapheme": "test",
    "phoneme": "test"
}
```

#### Problem: Emodžiji se ne čitaju

**Rješenje:** Provjerite je li čitanje emodžija uključeno

1. Otvorite Laprdus Konfigurator ili Android postavke
2. Pronađite opciju "Čitaj emodžije"
3. Uključite opciju
4. Spremite postavke

---

### 5.11 Naglasni rječnik (accents.json)

Formantni glasovi (Zvonko, Stojan, Mirsad i pjevački glasovi) sami određuju mjesto naglaska, dužinu i ton svake riječi: prvo iz naglasnih znakova u tekstu, zatim iz ugrađenog leksikona, pa po pravilima za nastavke i vrste riječi (posuđenice i izvedenice s dugim zadnjim slogom osnove, glagoli s prefiksom, glagoli na -ovati), a ako ništa ne pogodi, naglasak ide na prvi slog. Riječ koja tako ispadne krivo može se ispraviti u naglasnom rječniku, i to ne samo jedan njezin oblik: unos može pokriti cijelu osnovu, popis oblika ili cijeli glagol sa svim licima i vremenima. Snimljeni glasovi (Josip, Vlado) ga ne koriste.

Datoteka `accents.json` stoji uz ostale rječnike (`%APPDATA%\Laprdus` na Windowsu, `~/.config/Laprdus` na Linuxu) i učitava se zajedno s njima kad su korisnički rječnici uključeni. Učitava se jednom, pri pokretanju i pri promjeni datoteke; govor zbog nje nije ništa sporiji.

#### Zapis

Unosi se pišu u zapisu ugrađenog leksikona:

| Znak | Značenje |
|------|----------|
| `'` | ispred naglašenog samoglasnika (ili slogotvornog r); ton se bira sam |
| `^` | ispred naglašenog samoglasnika: silazni naglasak |
| `/` | ispred naglašenog samoglasnika: uzlazni naglasak |
| `:` | iza dugog samoglasnika |
| `*` | na kraju: osnova, koja vrijedi i za oblike s do tri slova više (kontróla, kontróle, kontrólom) |
| `\|` | `osnova\|nastavak\|nastavak`: po jedan točan oblik za svaki nastavak; prazan nastavak je sama osnova |

Primjer datoteke:

```json
{
    "version": "1.0",
    "entries": [
        { "word": "kontr'o:l*", "comment": "kontróla, kontróle, kontrólom" },
        { "word": "sign'a:l|a|u|om|e|i|ima", "comment": "signála, signálu...; sìgnal ostaje kako jest" },
        { "word": "p^oruk*", "comment": "pȍruka: silazni naglasak na prvom slogu" },
        { "word": "dir'ektor*", "language": "sr", "comment": "samo za Stojana" },
        { "verb": "ur'e:d=i<p", "comment": "uréditi, urédim, urédio, uređen" },
        { "verb": "dijel:i", "comment": "podijeliti, raspodijeliti, podijelim, podijeljen" }
    ]
}
```

- `word`: riječ, osnova (`*`) ili popis oblika (`|`).
- `verb`: glagol. Piše se cijela osnova s naglaskom infinitiva, znak `=` i vrsta: `i` za glagole na -iti/-im, `a` na -ati/-am, `u` na -nuti/-nem, `t` na -ati s drugom osnovom u prezentu i `e` za tu osnovu (`pok'a:z=t` i `pok'a:ž=e`). Iza vrste mogu stajati oznake: `<` pomiče naglasak jedan slog unatrag u prezentu, `p` i u trpnom pridjevu (tako govore Stojan i Mirsad, Zvonko zadržava naglasak infinitiva), `n` kad je imperativ jednak obliku neke imenice (potvrdi, uredi), pa se čita kao glagol samo na početku rečenice. Glagol s dugim *ije* u korijenu piše se kao `korijen:vrsta` i vrijedi sa svakim prefiksom.
- `language`: `hr`, `sr` ili `bs` ograničava unos na glas tog jezika; bez toga vrijedi za sva tri.
- `comment`: bilješka za vas, Laprdus je ne čita.

Točan oblik ima prednost pred osnovom, dulja osnova pred kraćom, a vaš unos pred ugrađenim. Naglasni znakovi napisani u samom tekstu, ili u zamjeni glavnog rječnika (`telèfon`), imaju prednost pred svime.

Neispravan unos (bez naglasnog znaka, s dva naglaska, s nepoznatim slovom ili nepoznatom vrstom glagola) preskače se, a ostali vrijede. Na naredbenoj liniji `laprdus -w` ispisuje koliko je unosa prihvaćeno i što je bilo krivo s prvim odbijenim; NVDA dodatak to upisuje u svoj dnevnik. Rezultat možete odmah čuti:

```bash
laprdus -v zvonko "kontrola, signala, uredio"
```

**Android i Apple:** datoteka `accents.json` čita se iz istog direktorija kao ostali rječnici kad su korisnički rječnici uključeni, ali aplikacije za sada nemaju uređivač za nju.

## 6. Glasovi

### 6.1 Dostupni glasovi

Laprdus uključuje osam glasova - dva osnovna, tri izvedena i tri formantna:

| Glas | Vrsta | Jezik | Opis |
|------|-------|-------|------|
| **Josip** | Osnovni | Hrvatski | Muški glas normalne visine. |
| **Vlado** | Osnovni | Srpski | Muški glas normalne visine. |
| **Detence** | Izvedeni | Hrvatski | Dječji glas. Izveden iz glasa Josip s povišenom visinom. |
| **Baba** | Izvedeni | Hrvatski | Ženski glas. Izveden iz glasa Josip s blago povišenom visinom. |
| **Djedo** | Izvedeni | Srpski | Stariji muški glas. Izveden iz glasa Vlado sa sniženom visinom. |
| **Zvonko** | Formantni | Hrvatski | Muški glas. Govor nastaje pravilima, bez snimaka. Zadani glas za hrvatski jezik. |
| **Stojan** | Formantni | Srpski | Muški glas, dublji i tamniji. Govor nastaje pravilima, bez snimaka. Zadani glas za srpski jezik. |
| **Mirsad** | Formantni | Bosanski | Muški glas, viši i melodičniji. Govor nastaje pravilima, bez snimaka. Zadani glas za bosanski jezik. |
| **Zvonko Orguljaš** | Pjevajući | Hrvatski | Zvonko za orguljama: pjeva *Vilu Velebita*. |
| **Klapa Zvonko** | Pjevajući | Hrvatski | Zvonko kao cijela klapa: pjeva *Vilu Velebita*. |
| **Zvonko Pjevač** | Pjevajući | Hrvatski | Zvonko pjeva svojim glasom, u svojoj visini: *Vila Velebita*. |
| **Stojan Trubač** | Pjevajući | Srpski | Stojan kao truba: pjeva *Kreće se lađa francuska*. |
| **Stojan Harmonikaš** | Pjevajući | Srpski | Stojan kao harmonika: pjeva *Kreće se lađa francuska*. |
| **Stojan Pevač** | Pjevajući | Srpski | Stojan pjeva svojim glasom: *Kreće se lađa francuska*. |
| **Mirsad Sevdalija** | Pjevajući | Bosanski | Mirsad pjeva sevdah: *Kad ja pođoh na Bembašu*. |
| **Mirsad Sazlija** | Pjevajući | Bosanski | Mirsad kao saz: pjeva *Kad ja pođoh na Bembašu*. |
| **Mirsad Solist** | Pjevajući | Bosanski | Mirsad pjeva svojim glasom: *Kad ja pođoh na Bembašu*. |
| **Zvonko Bećarac** | Pjevajući | Hrvatski | Zvonko svojim glasom pjeva bećarac: svaki deseterački dvostih sjeda na napjev. |

### 6.2 Osnovni i izvedeni glasovi

**Osnovni glasovi** (Josip i Vlado) koriste fizičke snimke fonema - stvarne zvučne zapise govornika.

**Izvedeni glasovi** (Detence, Baba, Djedo) koriste iste zvučne zapise kao osnovni glasovi, ali s prilagođenom visinom glasa:

- **Detence** koristi Josipove foneme s visinom 1.5x (viši glas)
- **Baba** koristi Josipove foneme s visinom 1.2x (blago viši glas)
- **Djedo** koristi Vladove foneme s visinom 0.75x (niži glas)

**Formantni glasovi** (Zvonko, Stojan, Mirsad) ne koriste snimke. Govor se u cijelosti računa pravilima, kao kod sintetizatora Eloquence i DECtalk: iz teksta se određuju glasovi, naglasak i rečenična intonacija, a zatim se zvuk stvara formantnim sintetizatorom. Zato ovi glasovi:

- ne trebaju datoteke s glasovnim podacima
- ostaju čisti i razumljivi pri vrlo velikim brzinama, jer se brzina i visina ne dobivaju naknadnom obradom zvuka
- imaju rečeničnu intonaciju (izjava, pitanje, zarez, usklik)
- brojeve izgovaraju prema jeziku glasa (Zvonko: *tisuća*, *milijun*; Stojan i Mirsad: *hiljada*, *milion*)

Ako formantni glas neku riječ naglasi na pogrešnom slogu, naglasak možete zadati u rječniku izgovora uobičajenim naglasnim znakovima, na primjer `telèfon`, `gláva`, `kȕća` ili `grȃd`.

**Pjevajući glasovi** (Zvonko Orguljaš, Klapa Zvonko, Zvonko Pjevač, Stojan Trubač, Stojan Harmonikaš, Stojan Pevač, Mirsad Sevdalija, Mirsad Sazlija, Mirsad Solist, Zvonko Bećarac) su formantni glasovi koji tekst ne govore nego pjevaju, kao što su to činili DECtalk i glasovi Pipe Organ i Cellos na Macintoshu. Svaki slog dobiva jednu notu narodne pjesme svoje zemlje (hrvatski glasovi pjevaju *Vilu Velebita*, srpski *Kreće se lađa francuska*, bosanski *Kad ja pođoh na Bembašu*); prva kitica pjesme točno sjeda na melodiju, a svaki drugi tekst pjeva se na istu melodiju, slog po slog, iz početka sa svakom novom rečenicom koju program pošalje. Brzina govora određuje tempo, visina transponira pjesmu, a intonacija određuje dubinu vibrata (0% ga isključuje). Umjesto grkljana svaki glas ima svoj "instrument": orgulje s odjekom crkve, klapu od nekoliko glasova s basom oktavu niže, trubu, dvije harmonikaške piskove ugođene za nekoliko centa različito, sevdalijski glas sa širokim vibratom i saz s tremolom. Pjevač, Pevač i Solist pjevaju običnim glasom Zvonka, Stojana i Mirsada, bez instrumenta i odjeka, a pjesma im je prenesena u visinu u kojoj inače govore.

### 6.3 Odabir glasa

#### Windows SAPI5

1. Otvorite Upravljačku ploču
2. Odaberite "Olakšani pristup" > "Prepoznavanje govora" > "Tekst u govor"
3. U padajućem izborniku "Odabir glasa" odaberite željeni Laprdus glas
4. Kliknite "Primijeni"

#### NVDA

**Metoda 1: Postavke govora**
1. Otvorite NVDA izbornik (NVDA tipka + N)
2. Odaberite Opcije > Postavke > Govor
3. U padajućem izborniku "Glas" odaberite željeni glas
4. Pritisnite "U redu" ili "Primijeni"

**Metoda 2: Brzi prsten postavki**
1. Pritisnite Control + NVDA tipka + strelica lijevo/desno za promjenu glasa
2. Ili koristite Control + NVDA tipka + V za otvaranje dijaloga za odabir glasa

#### Linux (Speech Dispatcher)

```bash
# Lista dostupnih glasova
spd-say -o laprdus -L

# Korištenje određenog glasa
spd-say -o laprdus -y josip "Tekst na hrvatskom"
spd-say -o laprdus -y vlado "Tekst na srpskom"
```

Za trajnu promjenu u Orca čitaču ekrana:
1. Otvorite Orca postavke (Insert+Razmaknica)
2. Na kartici "Voice" odaberite željeni glas iz padajućeg izbornika

#### Android

1. Otvorite Laprdus aplikaciju
2. Dodirnite "Laprdus postavke"
3. U odjeljku "Glas" dodirnite **"Glas"** padajući izbornik
4. Odaberite željeni glas iz popisa

Za promjenu zadanog glasa u sustavu:
1. Otvorite Postavke > Pristupačnost > Tekst u govor
2. Dodirnite ikonu zupčanika pored "Laprdus TTS"
3. Odaberite željeni glas

---

## 7. Rješavanje problema

### 7.1 Česti problemi

#### Nema zvuka

1. Provjerite je li Laprdus odabran kao aktivni sintetizator
2. Provjerite da glasnoća nije postavljena na 0
3. Provjerite sistemsku glasnoću računala
4. Ponovno pokrenite program koji koristite

#### Govor je prebrz ili prespor

Podesite brzinu govora:
- U NVDA: Control + NVDA tipka + N > Postavke > Govor > Brzina
- Na Androidu: Otvorite Laprdus aplikaciju i podesite klizač brzine u Laprdus postavkama
- U naredbenom programu: koristite opciju `-r`

#### NVDA ne pronalazi Laprdus

1. Provjerite je li dodatak ispravno instaliran (NVDA izbornik > Alati > Upravljanje dodacima)
2. Ponovno pokrenite NVDA
3. Provjerite da dodatak nije onemogućen

#### Speech Dispatcher ne radi na Linuxu

1. Provjerite je li Laprdus modul instaliran:
   ```bash
   ls /usr/lib/speech-dispatcher-modules/sd_laprdus
   ```

2. Provjerite konfiguraciju:
   ```bash
   grep laprdus /etc/speech-dispatcher/speechd.conf
   ```

3. Ponovno pokrenite Speech Dispatcher:
   ```bash
   systemctl --user restart speech-dispatcher
   ```

### 7.2 Dijagnostički zapisnici

#### NVDA dodatak

Za uključivanje dijagnostičkih zapisnika za NVDA dodatak:

1. Stvorite praznu datoteku `laprdus_debug` u privremenom direktoriju
   - Windows: `%TEMP%\laprdus_debug`
2. Ponovno pokrenite NVDA
3. Zapisnici će se spremati u `%TEMP%\laprdus_debug.log`

#### Linux

Koristite opciju `-w` za opširniji ispis:
```bash
laprdus -w "Test"
```

### 7.3 Kontakt i podrška

Za pomoć i podršku:

- **E-mail:** hrvojekatic@gmail.com
- **Web stranica:** https://hrvojekatic.com/laprdus
- **Izvorni kod:** https://github.com/hkatic/laprdus

Pri prijavi problema, molimo navedite:
- Verziju Laprdusa
- Operativni sustav
- Korišteni čitač ekrana (ako je primjenjivo)
- Opis problema
- Korake za reprodukciju problema

---

Zadnja izmjena: veljača 2026.
