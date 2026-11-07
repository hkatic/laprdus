# Laprdus - Korisničko uputstvo

Verzija 2.0

---

## Sadržaj

1. [Uvod](#1-uvod)
    - [1.1 Šta je Laprdus?](#11-šta-je-laprdus)
    - [1.2 Kome je Laprdus namenjen?](#12-kome-je-laprdus-namenjen)
    - [1.3 Podržane platforme](#13-podržane-platforme)
    - [1.4 Mogućnosti](#14-mogućnosti)
2. [Instalacija](#2-instalacija)
    - [2.1 Windows SAPI5](#21-windows-sapi5)
    - [2.2 NVDA dodatak](#22-nvda-dodatak)
    - [2.3 Linux](#23-linux)
    - [2.4 Android](#24-android)
3. [Upotreba](#3-upotreba)
    - [3.1 Osnovna upotreba](#31-osnovna-upotreba)
    - [3.2 Komandna linija (Linux i Windows)](#32-komandna-linija-linux-i-windows)
    - [3.3 NVDA podešavanja](#33-nvda-podešavanja)
    - [3.4 Android aplikacija](#34-android-aplikacija)
4. [Podešavanja](#4-podešavanja)
    - [4.1 Brzina govora](#41-brzina-govora)
    - [4.2 Visina glasa](#42-visina-glasa)
    - [4.3 Jačina](#43-jačina)
    - [4.4 Pauze](#44-pauze)
    - [4.5 Način čitanja brojeva](#45-način-čitanja-brojeva)
    - [4.6 Intonacija](#46-intonacija)
    - [4.7 Sricanje](#47-sricanje)
    - [4.8 Laprdus konfigurator (Windows)](#48-laprdus-konfigurator-windows)
    - [4.9 Kvalitet glasa](#49-kvalitet-glasa)
5. [Rečnici](#5-rečnici)
    - [5.1 Vrste rečnika](#51-vrste-rečnika)
    - [5.2 Lokacije datoteka rečnika](#52-lokacije-datoteka-rečnika)
    - [5.3 Glavni rečnik (user.json)](#53-glavni-rečnik-userjson)
    - [5.4 Rečnik sricanja (spelling.json)](#54-rečnik-sricanja-spellingjson)
    - [5.5 Rečnik emodžija (emoji.json)](#55-rečnik-emodžija-emojijson)
    - [5.6 Uređivanje rečnika na Windowsu](#56-uređivanje-rečnika-na-windowsu-laprdus-konfigurator)
    - [5.7 Uređivanje rečnika na Androidu](#57-uređivanje-rečnika-na-androidu)
    - [5.8 Ručno uređivanje JSON datoteka](#58-ručno-uređivanje-json-datoteka-napredno)
    - [5.9 Primeri praktične upotrebe](#59-primeri-praktične-upotrebe)
    - [5.10 Rešavanje problema sa rečnicima](#510-rešavanje-problema-sa-rečnicima)
    - [5.11 Naglasni rečnik (accents.json)](#511-naglasni-rečnik-accentsjson)
6. [Glasovi](#6-glasovi)
    - [6.1 Dostupni glasovi](#61-dostupni-glasovi)
    - [6.2 Osnovni i izvedeni glasovi](#62-osnovni-i-izvedeni-glasovi)
    - [6.3 Izbor glasa](#63-izbor-glasa)
7. [Rešavanje problema](#7-rešavanje-problema)
    - [7.1 Česti problemi](#71-česti-problemi)
    - [7.2 Dijagnostički dnevnici](#72-dijagnostički-dnevnici)
    - [7.3 Kontakt i podrška](#73-kontakt-i-podrška)

---

## 1. Uvod

### 1.1 Šta je Laprdus?

Laprdus je sintetizator govora (TTS - Text-to-Speech) za hrvatski, srpski, bosanski i slovenački jezik. Njegovi izvorni glasovi koriste tehnologiju konkatenativne sinteze i proizvode govor spajanjem unapred snimljenih fonemskih jedinica, a formantni glasovi govor u celosti izračunavaju po pravilima, bez snimaka. Svaki glas govori sva četiri jezika. Iako ne dostiže kvalitet savremenih neuronskih TTS sistema, Laprdus nudi visoke performanse i minimalnu potrošnju memorije.

Laprdus je razvijen da korisnicima čitača ekrana obezbedi jednostavan i brz pristup računarima i mobilnim uređajima na njihovom maternjem jeziku, besplatno.

Laprdus je započet kao moj lični hobi projekat, a zamišljen je i kao eksperimentalan, što mi omogućava da vidim dokle mogu da doguram s njim.

### 1.2 Kome je Laprdus namenjen?

Laprdus nije namenjen svima. Dobar je izbor za one koji uživaju u nostalgičnom zvuku retro sintetizatora govora iz 1980-ih i 1990-ih godina.

Možda neće odgovarati korisnicima koji su navikli na visok kvalitet glasa savremenih sintetizatora koji koriste veštačku inteligenciju i neuronske mreže. Međutim, Laprdus može da bude idealan za one kojima su performanse i minimalna potrošnja resursa važniji od kvaliteta glasa.

Ukratko: savremeni sintetizatori nude kvalitet po cenu resursa; Laprdus nudi performanse po cenu kvaliteta.

### 1.3 Podržane platforme

Laprdus je dostupan na sledećim platformama:

- **Windows 7 do Windows 11** - preko standarda Microsoft SAPI5, što omogućava korišćenje Laprdusa u svim programima koji podržavaju Windows govor, uključujući i čitače ekrana kao što su Narrator, NVDA i JAWS
- **Direktna integracija sa NVDA čitačem ekrana** - dostupan je i kao poseban dodatak za besplatni Windows čitač ekrana NVDA
- **Linux** - preko sistema Speech Dispatcher za čitač ekrana Orca i kao program za komandnu liniju
- **Android** - kao ugrađeni sintetizator govora za Android uređaje

### 1.4 Mogućnosti

Laprdus nudi sledeće mogućnosti:

- Govor na hrvatskom, srpskom, bosanskom i slovenačkom jeziku (podržani su latinica i ćirilica)
- Osamnaest glasova (dva osnovna, tri izvedena, tri formantna i deset pevajućih), a svaki od njih govori sva četiri jezika (hrvatski, srpski, bosanski i slovenački); jezik se bira odvojeno od glasa
- Podešavanje brzine govora
- Podešavanje visine glasa
- Podešavanje jačine
- Prirodna intonacija koja prati interpunkciju
- Čitanje celih brojeva ili cifru po cifru
- Podešavanje trajanja pauza za različite znakove interpunkcije
- Korisničke rečnike za prilagođavanje izgovora reči, fraza, simbola i emodžija

---

## 2. Instalacija

### 2.1 Windows SAPI5

Da biste instalirali Laprdus u operativnom sistemu Windows, uradite sledeće:

#### Korak 1: Preuzimanje

1. Posetite zvaničnu stranicu Laprdusa: https://hrvojekatic.com/laprdus
2. Pronađite odeljak za preuzimanje i kliknite na vezu za Windows SAPI5
3. Preuzmite instalacioni program (datoteka pod nazivom `Laprdus_SAPI5_Setup.exe`)
4. Datoteka se čuva u fascikli Preuzimanja (Downloads)

#### Korak 2: Pokretanje instalacije

1. Otvorite Windows Explorer (tasteri Windows+E)
2. Strelicom nadole pronađite fasciklu Preuzimanja i otvorite je tasterom Enter
3. Pronađite preuzeti instalacioni program Laprdusa (datoteka pod nazivom `Laprdus_SAPI5_Setup.exe`)
4. **Preporučeno:** Pritisnite taster menija ili Shift+F10 na datoteci, a zatim izaberite „Pokreni kao administrator”
    - Alternativa: Pritisnite Enter za normalno pokretanje
5. Ako se pojavi prozor Kontrola korisničkih naloga (UAC), izaberite „Da”

#### Korak 3: Postupak instalacije

1. Izaberite jezik instalacije strelicom nadole i pritisnite Enter
2. Izaberite lokaciju instalacije (podrazumevano: `C:\Program Files\Laprdus`), tabulatorom izaberite „Dalje” i pritisnite Enter
3. Razmaknicom potvrdite izbor ako želite da se na radnoj površini napravi prečica za Laprdus konfigurator, a zatim tabulatorom pronađite „Dalje” i pritisnite Enter.
4. Tabulatorom pronađite „Instaliraj”, pritisnite Enter i sačekajte da se instalacija završi
5. Na kraju instalacije pritisnite Enter na dugmetu „Završi” da biste zatvorili instalacioni program

#### Korak 4: Provera instalacije

1. Otvorite Kontrolnu tablu (Control Panel):
    - Pritisnite Windows taster
    - Upišite „Kontrolna tabla” i pritisnite Enter
2. Izaberite „Lakši pristup” > „Prepoznavanje govora”
3. Tabulatorom pronađite „Pretvaranje teksta u govor” i pritisnite Enter
4. U padajućem meniju „Izbor glasa” treba da vidite četiri Laprdus glasa:
    - Laprdus Croatian (hrvatski)
    - Laprdus Serbian (srpski)
    - Laprdus Bosnian (bosanski)
    - Laprdus Slovenian (slovenački)

    SAPI5 glas bira samo jezik. Sva četiri govore glasom koji izaberete u Laprdus konfiguratoru (Zvonko dok ga ne promenite): Josipom, Vladom, Detencetom, Babom, Đedom, Zvonkom, Stojanom, Mirsadom ili nekim od deset pevajućih glasova (pogledajte [6.1](#61-dostupni-glasovi)). Instalacioni program pri instalaciji, nadogradnji i popravci uklanja glasove koje su registrovale prethodne verzije (po jedan za svaki Laprdus glas).

### 2.2 NVDA dodatak

Da biste koristili Laprdus sa NVDA čitačem ekrana, uradite sledeće:

#### Korak 1: Preuzimanje

1. Posetite zvaničnu stranicu Laprdusa: https://hrvojekatic.com/laprdus
2. Pronađite odeljak za preuzimanje i kliknite na vezu za NVDA dodatak
3. Preuzmite NVDA dodatak (datoteka sa nastavkom `.nvda-addon`)
4. Datoteka se čuva u fascikli Preuzimanja

#### Korak 2: Instalacija dodatka

1. Otvorite Windows Explorer (tasteri Windows+E)
2. Pronađite fasciklu Preuzimanja
3. Pronađite preuzetu datoteku Laprdus NVDA dodatka (datoteka sa nastavkom `.nvda-addon`)
4. Pritisnite Enter na datoteci
    - **Alternativa:** Pritisnite taster menija (ili Shift+F10) da biste otvorili kontekstualni meni, a zatim izaberite „Otvori”
5. NVDA prikazuje dijalog sa pitanjem „Da li ste sigurni da želite da instalirate ovaj dodatak?”
6. Tabulatorom pronađite dugme „Da” i pritisnite Enter ili jednostavno pritisnite taster D za DA
7. Sačekajte da se instalacija završi

#### Korak 3: Ponovno pokretanje NVDA-a

1. NVDA traži da ga ponovo pokrenete
2. Izaberite „Da” da biste ga odmah ponovo pokrenuli
3. Sačekajte da se NVDA ponovo pokrene

#### Korak 4: Izbor Laprdusa kao sintetizatora

**Metod 1: Preko NVDA menija**

1. Otvorite NVDA meni: pritisnite NVDA taster + N (Napomena: NVDA taster na vašem sistemu može da bude Insert ili Caps Lock, zavisno od toga kako ste ga konfigurisali. Ako je NVDA taster podešen na Insert, možete da pritisnete Insert + N.)
2. Strelicom nadole pronađite „Opcije” i pritisnite Enter
3. Strelicom nadole pronađite „Podešavanja” i pritisnite Enter
4. U dijalogu Podešavanja strelicom nadole pronađite kategoriju „Govor”
5. Tabulatorom pronađite dugme „Promeni...” koje se nalazi odmah pored opcije „Sintetizator”
6. Pritisnite Enter na dugmetu „Promeni...”
7. Na spisku sintetizatora strelicom nadole pronađite „Laprdus” i pritisnite Enter
8. Pritisnite Escape da biste zatvorili podešavanja

**Metod 2: Brza prečica (preporučeno)**

1. Pritisnite prečicu Control + NVDA taster + S (Napomena: NVDA taster na vašem sistemu može da bude Insert ili Caps Lock, zavisno od toga kako ste ga konfigurisali. Ako je NVDA taster podešen na Insert, možete da pritisnete Control + Insert + S.)
2. Otvara se dijalog za izbor sintetizatora
3. Strelicom nadole pronađite „Laprdus” i pritisnite Enter

### 2.3 Linux

#### Debian i Ubuntu

Otvorite terminal i pokrenite sledeću komandu (zamenite naziv datoteke nazivom preuzete .deb datoteke):

```bash
sudo dpkg -i laprdus_amd64.deb
```

#### Fedora

Pokrenite sledeću komandu (zamenite naziv datoteke nazivom preuzete .rpm datoteke):

```bash
sudo rpm -i laprdus.x86_64.rpm
```

#### Arch Linux

1. Preuzmite PKGBUILD datoteku sa veb stranice
2. U terminalu pređite u direktorijum sa PKGBUILD datotekom
3. Pokrenite:

```bash
makepkg -si
```

#### Ručna instalacija (iz tarball arhive)

1. Raspakujte arhivu (zamenite naziv datoteke nazivom preuzete arhive):

        tar xf laprdus-linux-x86_64.tar.xz
        cd laprdus-linux-x86_64

2. Pokrenite instalacionu skriptu:

        sudo ./install.sh

#### Konfigurisanje za čitač ekrana Orca

Posle instalacije Laprdus se automatski konfiguriše za Speech Dispatcher. Da biste ga koristili sa čitačem ekrana Orca:

1. Za svaki slučaj, preporučuje se da ponovo pokrenete Speech Dispatcher:

        systemctl --user restart speech-dispatcher

2. Otvorite Orca podešavanja:
    - Pritisnite Insert+razmaknica da biste otvorili podešavanja (ili Caps Lock + razmaknica ako je Orca modifikator podešen na Caps Lock ili Laptop raspored)
    - Ili u okruženju radne površine Gnome ili Mate: pritisnite Alt+F2, upišite orca -s, a zatim pritisnite Enter

3. Tabulatorom ili kombinacijom Shift + Tab pređite na kartice podešavanja, strelicom nadesno pronađite karticu „Voice” (Glas), a zatim uzastopnim pritiskanjem tabulatora izaberite:
    - Speech system: Pritisnite razmaknicu da biste proširili padajući meni, strelicom nadole izaberite Speech Dispatcher ako već nije podešen i pritisnite Enter
    - Speech synthesizer: Pritisnite razmaknicu da biste proširili padajući meni, strelicom nadole izaberite Laprdus i pritisnite Enter
    - Voice: Pritisnite razmaknicu da biste proširili padajući meni, strelicom nadole izaberite željeni glas (Zvonko, Josip, Vlado itd.; svaki glas je naveden za hrvatski, srpski, bosanski i slovenački, a jezik koji Orca traži određuje kojim jezikom govori) i pritisnite Enter

4. Tabulatorom pronađite „Apply” (Primeni) i zatvorite podešavanja

### 2.4 Android

#### Korak 1: Preuzimanje APK datoteke

1. Na Android uređaju otvorite veb pregledač
2. Posetite: https://hrvojekatic.com/laprdus
3. Preuzmite APK datoteku (datoteka sa nastavkom `.apk`)

#### Korak 2: Omogućavanje instalacije iz nepoznatih izvora

Ovo je potrebno samo ako aplikaciju instalirate izvan prodavnice Google Play.

**Za Android 8.0 i novije verzije:**

1. Otvorite Podešavanja na uređaju
2. Idite na: Aplikacije > Poseban pristup aplikacijama > Instaliranje nepoznatih aplikacija
    - Ili: Podešavanja > Bezbednost > Instaliranje nepoznatih aplikacija
3. Pronađite veb pregledač koji koristite (npr. Chrome)
4. Uključite opciju „Dozvoli iz ovog izvora”

**Za starije verzije Androida:**

1. Otvorite Podešavanja
2. Idite na: Bezbednost
3. Uključite opciju „Nepoznati izvori”

#### Korak 3: Instalacija aplikacije

1. Otvorite menadžer datoteka na uređaju
2. Pronađite preuzetu APK datoteku (obično u fascikli „Download” ili „Preuzimanja”)
3. Dodirnite APK datoteku
4. Kada se pojavi upit, dodirnite „Instaliraj”
5. Sačekajte da se instalacija završi
6. Dodirnite „Otvori” da biste pokrenuli aplikaciju ili „Gotovo” da biste zatvorili prozor

#### Korak 4: Podešavanje Laprdusa kao podrazumevanog TTS-a

1. Otvorite Podešavanja na uređaju
2. Idite na: Pristupačnost > Izlaz za pretvaranje teksta u govor (Text-to-speech)
    - Na nekim uređajima: Podešavanja > Sistem > Jezici i unos > Izlaz za pretvaranje teksta u govor
    - Na Samsung uređajima: Podešavanja > Opšte upravljanje > Jezik i unos > Izlaz za pretvaranje teksta u govor
3. Dodirnite „Željeni mehanizam” ili ikonu zupčanika
4. Izaberite „Laprdus TTS” sa spiska
5. Vratite se na prethodni ekran i dodirnite „Poslušajte primer” da biste testirali govor

#### Napomena za korisnike čitača ekrana TalkBack

Ako koristite TalkBack, svi interfejsi aplikacije Laprdus potpuno su pristupačni:

- Koristite standardne TalkBack pokrete za navigaciju
- Dvostruki dodir za aktiviranje dugmadi i opcija
- Prevlačenje sa dva prsta za pomeranje kroz liste

---

## 3. Upotreba

### 3.1 Osnovna upotreba

Posle instalacije Laprdus je automatski dostupan svim programima koji koriste sintezu govora na vašem sistemu. To uključuje:

- Čitače ekrana (NVDA, Orca, Narrator)
- Veb pregledače sa mogućnošću čitanja
- Programe za čitanje dokumenata
- Bilo koji program koji koristi Windows SAPI5 ili Android TTS

### 3.2 Komandna linija (Linux i Windows)

Laprdus sadrži program za komandnu liniju pomoću kog možete da pretvarate tekst u govor direktno iz terminala.

#### Osnovni primeri

```bash
# Izgovor teksta
laprdus "Dobar dan!"

# Korišćenje drugog glasa (bez -L glas govori svoj izvorni jezik, Vlado srpski)
laprdus -v vlado "Zdravo svete!"

# Svaki glas na svakom jeziku: Zvonko govori srpski, Vlado hrvatski
laprdus -L sr "Zdravo svete!"
laprdus -v vlado -L hr "Dobar dan svima!"

# Podešavanje brzine govora
laprdus -r 1.5 "Brži govor"

# Čuvanje u WAV datoteci
laprdus -o govor.wav "Tekst za snimanje"

# Čitanje iz datoteke
laprdus -i dokument.txt

# Čitanje sa standardnog ulaza
echo "Tekst" | laprdus
```

#### Opcije programa za komandnu liniju

| Opcija | Opis |
|--------|------|
| `-v, --voice` | Izbor glasa (zvonko (podrazumevano), stojan, mirsad, josip, vlado, detence, baba, djedo ili pevajući glas: orguljas, dalmacija_klapa, trubac, harmonikas, sevdalija, sazlija, lika_velebit, solunac, sarajevo_bembasa, becarac) |
| `-L, --language` | Jezik: `hr`, `sr`, `bs` ili `sl`. Svaki glas govori svaki jezik; bez ove opcije glas govori svoj izvorni jezik |
| `-r, --speech-rate` | Brzina govora (0.5-2.0, podrazumevano: 1.0; formantni glasovi 0.25-4.0) |
| `-p, --speech-pitch` | Visina glasa (0.5-2.0, podrazumevano: 1.0; formantni glasovi 0.25-4.0) |
| `-I, --inflection` | Nivo intonacije, 0-100 (podrazumevano: 50; 0 je monotono) |
| `-a, --acceleration` | Ubrzanje, množilac brzine 0.5-3.0 (podrazumevano: 1.0) |
| `--head-size PERCENT` | Kvalitet glasa: dužina govornog trakta, 0-100 (podrazumevano: 50 = glas kakav jeste) |
| `--breathiness PERCENT` | Šum daha, 0-100 (podrazumevano: 50; formantni glasovi) |
| `--roughness PERCENT` | Hrapavost (jitter i shimmer), 0-100 (podrazumevano: 50; formantni glasovi) |
| `--brightness PERCENT` | Boja tona, 0-100 (podrazumevano: 50; 0 tamno, 100 oštro) |
| `-V, --speech-volume` | Jačina (0.0-1.0, podrazumevano: 1.0) |
| `-B, --volume-boost` | Pojačanje jačine: glasniji govor kroz ograničavač vrhova, za bučna mesta |
| `-d, --numbers-digits` | Čitaj brojeve cifru po cifru |
| `-s, --spell` | Sriči tekst znak po znak |
| `-S, --spelling-speed` | Brzina sricanja, 0-100; niže vrednosti produžavaju pauze (podrazumevano: 50) |
| `-m, --spelling-mode` | Sricanje: `sounds` (glasovi slova: b, c, d; podrazumevano) ili `names` (nazivi slova: be, ce, de) |
| `-c, --comma-pauses` | Trajanje pauze za zarez u ms (podrazumevano: 100) |
| `-e, --period-pauses` | Trajanje pauze za tačku u ms (podrazumevano: 80) |
| `-x, --exclamationmark-pauses` | Trajanje pauze za uzvičnik u ms (podrazumevano: 70) |
| `-q, --questionmark-pauses` | Trajanje pauze za upitnik u ms (podrazumevano: 60) |
| `-n, --newline-pauses` | Trajanje pauze za novi red u ms (podrazumevano: 100) |
| `-D, --data-dir` | Direktorijum sa glasovnim podacima |
| `-o, --output-file` | Sačuvaj govor u WAV datoteci |
| `-i, --input-file` | Učitaj tekst iz datoteke |
| `-l, --list-voices` | Prikaži spisak dostupnih glasova sa njihovim izvornim jezikom |
| `-w, --verbose` | Opširniji ispis (za dijagnostiku) |
| `-h, --help` | Prikaži pomoć |

### 3.3 NVDA podešavanja

Kada koristite Laprdus sa NVDA čitačem ekrana, podešavanja glasa možete da promenite na sledeći način:

1. Otvorite NVDA meni (NVDA taster + N)
2. Izaberite podmeni Opcije, a zatim Podešavanja > kategorija Govor
3. U dijalogu možete da podesite:
    - Jezik (hrvatski, srpski, bosanski ili slovenački)
    - Glas (spisak sadrži svih 18 Laprdus glasova, pod imenima iz [6.1](#61-dostupni-glasovi); podrazumevani je Zvonko)
    - Brzinu govora
    - Visinu glasa
    - Modulaciju (nivo rečenične melodije, pogledajte [4.6](#46-intonacija))
    - Jačinu
    - Povećanje brzine ili Rate Boost (svaki položaj klizača brzine čini tri puta bržim)

**Jezik je odvojen od glasa.** Svaki Laprdus glas govori sva četiri jezika, pa je spisak glasova isti za svaki jezik, a jezik birate u zasebnom padajućem meniju „Jezik”, kao kod drugih sintetizatora. Jezik određuje pravila izgovora, naglasni rečnik, nazive slova pri sricanju i reči za brojeve (*tisuća*, *milijun* na hrvatskom; *hiljada*, *milion* na srpskom i bosanskom; *tisoč*, *milijon* na slovenačkom): Josip tako može da govori srpski, a Stojan hrvatski ili slovenački. Nova instalacija počinje na jeziku NVDA-a ako je to hrvatski, srpski, bosanski ili slovenački, inače na hrvatskom; jezik koji sami izaberete NVDA pamti kao i ostala podešavanja. Jezik se nalazi i u krugu govornih podešavanja (Control + NVDA taster + strelice).

**Automatska promena jezika.** Kada je u NVDA podešavanjima govora uključena opcija „Automatska promena jezika (kada je podržana)”, tekst označen kao hrvatski, srpski, bosanski ili slovenački Laprdus izgovara na tom jeziku, istim glasom; tekst na bilo kom drugom jeziku izgovara na jeziku koji ste izabrali. Glas se pritom nikada ne menja.

### 3.4 Android aplikacija

Android aplikacija Laprdus ima ekran sa podešavanjima na kom možete da:

1. Izaberete glas
2. Izaberete jezik (hrvatski, srpski, bosanski ili slovenački) za govor u samoj aplikaciji; govor koji traže druge aplikacije i TalkBack ide na jeziku koji one zatraže, osim kada je uključeno „Prinudno koristi jezik”
3. Podesite brzinu govora
4. Podesite visinu glasa
5. Podesite jačinu
6. Podesite čitanje emodžija
7. Podesite način čitanja brojeva
8. Podesite trajanje pauza i brzinu sricanja
9. Izaberete sricanje nazivima slova ili glasovima

Da biste otvorili podešavanja:

1. Otvorite aplikaciju Laprdus iz fioke aplikacija
2. Dodirnite dugme „Laprdus podešavanja”
3. Promenite podešavanja po želji (promene se automatski čuvaju)

---

## 4. Podešavanja

### 4.1 Brzina govora

Brzina govora određuje koliko brzo Laprdus izgovara tekst. Opseg je od 0,5 (upola sporije) do 2,0 (dvostruko brže), pri čemu je 1,0 normalna brzina.

- **0,5** - Veoma sporo, korisno za početnike ili kada treba pažljivo da slušate svaku reč
- **1,0** - Normalna brzina, pogodna za većinu situacija
- **1,5** - Umereno brzo, za iskusne korisnike
- **2,0** - Brzo, za napredne korisnike

Napomena: U NVDA dodatku opcija „Povećanje brzine” ili „Rate Boost” svaki položaj klizača brzine čini tri puta bržim, kao kod drugih sintetizatora; Josip, Vlado i glasovi izvedeni iz njih zaustavljaju se na četvorostrukoj brzini.

**Brzina u Laprdusu i brzina čitača ekrana.** Brzina iz Laprdus podešavanja normalna je brzina glasa, a čitač ekrana ili aplikacija (TalkBack, VoiceOver, NVDA, programi koji koriste SAPI5, Speech Dispatcher) svoju brzinu zadaje u odnosu na nju. Dok čitač ekrana govori svojom normalnom brzinom, Laprdus govori upravo brzinom iz svojih podešavanja; dvostruko brži čitač ekrana daje dvostruko veću brzinu. Zato govor ubrzavaju i usporavaju i klizač brzine u Laprdusu i klizač u čitaču ekrana. Prekidač „Prinudno koristi Laprdus brzinu” (u Windows konfiguratoru „Prinudno koristi Laprdus brzinu govora u svim aplikacijama”) zanemaruje brzinu koju traži čitač ekrana ili aplikacija, pa Laprdus govori samo brzinom iz svojih podešavanja. [Ubrzanje](#46-intonacija) na kraju množi tu brzinu.

### 4.2 Visina glasa

Visina glasa omogućava podešavanje osnovne frekvencije glasa. Opseg je od 0,5 (niži glas) do 2,0 (viši glas), pri čemu je 1,0 prirodna visina glasa. Kao i brzina (pogledajte [4.1](#41-brzina-govora)), visina iz Laprdus podešavanja normalna je visina glasa koju čitač ekrana podiže ili spušta, osim kada je uključen prekidač „Prinudno koristi Laprdus visinu”.

- **0,5** - Niži glas
- **1,0** - Prirodna visina
- **2,0** - Viši glas

Napomena: Podešavanje visine glasa ne utiče na karakter glasa (nema „efekta crtaća”).

### 4.3 Jačina

Jačina određuje koliko će govor biti glasan. Opseg je od 0,0 (tiho) do 1,0 (maksimalna jačina).

- **0,0** - Tiho (nema zvuka)
- **0,5** - Umerena jačina
- **1,0** - Maksimalna jačina

#### Pojačanje jačine govora

Prekidač **Pojačaj jačinu govora (za bučna mesta)** u Laprdus konfiguratoru čini govor glasnijim nego što to dozvoljava klizač jačine na 100%. Namenjen je bučnim mestima (saobraćaj, ulica) i slušalici telefona kada je zvučnik isključen: na primer da čujete koga zovete dok se poziv uspostavlja ili da pronađete dugme za uključivanje zvučnika tokom poziva. Govor prolazi kroz jako pojačanje i ograničavač vrhova koji pojačanje menja samo između slogova, kao kod pojačavanja jačine govora u telefonima: tihi delovi postaju glasniji za oko 8 dB, glasni za oko 3 dB, a unutar sloga ništa se ne odseca niti sabija, pa zvuk ostaje čist. Suglasnici zadržavaju razmak prema samoglasnicima, a visine iznad 4 kHz utišane su za 3 dB, jer zvučnik telefona na najvećoj jačini tamo ima sopstveni vrh. NVDA klizač jačine i dalje menja pojačani govor. Prekidač je podrazumevano isključen i važi za sve glasove.

### 4.4 Pauze

Laprdus automatski ubacuje pauze posle znakova interpunkcije da bi govor zvučao prirodnije. Možete da podesite trajanje pauza za:

- **Tačka** - Pauza posle tačke (podrazumevano: 100 ms u korisničkim podešavanjima, 80 ms u programu za komandnu liniju)
- **Uzvičnik** - Pauza posle uzvičnika (podrazumevano: 100 ms u korisničkim podešavanjima, 70 ms u programu za komandnu liniju)
- **Upitnik** - Pauza posle upitnika (podrazumevano: 100 ms u korisničkim podešavanjima, 60 ms u programu za komandnu liniju)
- **Zarez** - Kratka pauza unutar rečenice (podrazumevano: 100 ms)
- **Novi red** - Pauza za prelom reda (podrazumevano: 100 ms)

Vrednosti se izražavaju u milisekundama (ms). Opseg je od 0 do 2000 ms.

**Napomena:** Korisnički konfigurator (Windows/Android) koristi jedinstveno podešavanje od 100 ms za sve znakove kraja rečenice (tačka, uzvičnik, upitnik), dok program za komandnu liniju omogućava zasebno podešavanje svakog znaka.

U istom odeljku podešavanja nalazi se i **brzina sricanja** (0–100%, podrazumevano 50%): niža vrednost produžava pauzu posle svakog sricanog ili otkucanog znaka, a sam znak se uvek izgovara uobičajenom brzinom govora. Na 100% nema dodatne pauze; na 50% dodaje se oko 41% trajanja izgovora znaka, na 20% oko 74%, a na 0% celo trajanje. Ta tišina se dodaje osnovnoj pauzi sricanja (podrazumevano 200 ms, `pauses.spelling` u `settings.json`). Pauza sledi i posle poslednjeg znaka, pa je razmak isti bez obzira na to da li čitač ekrana šalje celu reč ili znak po znak.

U NVDA-u prednost ima NVDA opcija „Odloženi opisi znakova pri pomeranju kursora” (podešavanja govora): kada je uključena, NVDA posle slova na koje ste se pomerili čeka jednu sekundu, a zatim izgovara njegov opis (npr. „Alan” za A). Laprdus tada ne dodaje pauzu brzine sricanja posle tog slova, pa opis dolazi tačno posle te sekunde, bez obzira na brzinu sricanja. Sricanje reči i kucanje zadržavaju ritam brzine sricanja.

### 4.5 Način čitanja brojeva

Laprdus može da čita brojeve na dva načina:

1. **Rečima** (podrazumevano) - Brojevi se čitaju kao cele reči
    - Primer: „123” se čita kao „sto dvadeset tri”

2. **Ciframa** - Brojevi se čitaju cifru po cifru
    - Primer: „123” se čita kao „jedan dva tri”

Čitanje rečima podržava brojeve do centiliona (10^303) i koristi ispravne gramatičke oblike izabranog jezika.

### 4.6 Intonacija

Intonacija je prirodna promena visine glasa koja prati interpunkciju:

- Tačka na kraju rečenice izaziva pad visine glasa
- Upitnik izaziva porast visine glasa na kraju
- Uzvičnik daje naglasak
- Zarez izaziva blag porast visine glasa

Veličinu intonacije određuje klizač, a prekidača za uključivanje i isključivanje iz verzije 1.0 više nema: monoton govor dobija se klizačem na 0%. Za sve glasove važe dva podešavanja:

- **Nivo intonacije** (0-100%) određuje veličinu svih pokreta visine: 0% je monoton govor, 50% prirodna melodija, a 100% udvostručuje svaki pokret.
- **Ubrzanje** (0,5-3) množi brzinu govora, pa klizač brzine u čitaču ekrana ili sistemu dostiže veću (ili manju) najveću brzinu. Josip, Vlado i glasovi izvedeni iz njih zaustavljaju se na četvorostrukoj brzini; Zvonko, Stojan i Mirsad idu do osmostruke.

U NVDA-u nivo intonacije određuje NVDA klizač Modulacija u podešavanjima glasa: 50 je prirodna melodija, 0 monoton govor, a na 100 svaki pokret je dvostruk. Intonacija iz konfiguratora se tada ne koristi. Ubrzanje se u NVDA-u ne koristi; veće brzine dostižu se NVDA opcijom Povećanje brzine (Rate Boost).

### 4.7 Sricanje

Kada čitač ekrana sriče tekst ili izgovara znakove dok kucate, Laprdus svako slovo čita na jedan od dva načina:

- **Nazivima slova**: *a, be, ce, če, će, de, dže, đe, e, ef, ge, ha, i, je, ka, el, elj, em, en, enj, o, pe, er, es, eš, te, u, ve, ze, že*; strana slova *ku, duplo ve, iks, ipsilon*. To su nazivi iz hrvatskog pravopisa, a važe i za bosanski. Na srpskom svaki glas sledi odluku Odbora za standardizaciju srpskog jezika: *lje, nje* i *ša* umesto *elj, enj* i *eš*, ostala slova isto, ćirilična kao i latinična (*Љ* je *lje*, *Ш* je *ša*). Na slovenačkom važe nazivi iz Slovenačkog pravopisa: isti nazivi za njegovih 25 slova, *dvojni ve* za W i *mehki če* i *mehki dže* za Ć i Đ. Nazive određuje izabrani jezik, ne glas: Josip na srpskom kaže *lje*, Stojan na hrvatskom *elj*.
- **Glasovima slova** (podrazumevano): umesto naziva izgovara se glas koji slovo označava, kao kada dete uči da čita i kao što ih sriču neki sintetizatori: suglasnik i posle njega kratak neutralni samoglasnik (*bə, cə, čə, fə, sə, mə*), samoglasnici kao samoglasnici. Strana slova *q, w, x* i *y* nisu slova naše abecede, pa se i ovde čitaju nazivima (*ku, duplo ve, iks, ipsilon*). Engleski nazivi i engleski izgovor se ne koriste.

Način sricanja je prekidač „Sriči slova glasovima”, uključen pri novoj instalaciji, u naprednim podešavanjima (Windows: grupa Opcije u konfiguratoru; Android i Apple: odeljak Napredno; NVDA: konfigurator; program za komandnu liniju: `-m`). Odnosi se na sricanje, kucanje i slovo koje u poruci stoji samo; reči se i dalje izgovaraju normalno. Brojevi, interpunkcija i simboli se u oba načina čitaju imenom iz rečnika sricanja. Brzina sricanja opisana je u [4.4](#44-pauze).

**Velika slova.** Ako je čitač ekrana podešen da najavljuje velika slova rečju („veliko N” u NVDA-u, „veliko slovo N” u TalkBacku, „veliko početno slovo N” u VoiceOveru, „cap N” na engleskom), Laprdus reč izgovara normalno, a slovo sriče kao i svako drugo: nazivom ili glasom, prema izabranom načinu, i brzinom sricanja. NVDA velika slova najčešće označava povišenim glasom: podešavanje „Procenat promene visine za velika slova” u podešavanjima glasa (podrazumevano 30) podiže visinu sricanog velikog slova, a sledeći znak ili reč vraćaju se na visinu klizača. To važi i kada je uključeno prinudno korišćenje Laprdus visine: promena se tada računa od sredine NVDA klizača visine.

**Slovo samo u poruci.** Čitač ekrana često javlja znak unutar poruke: „Obrisano B”, „B odabrano”, „veliko slovo B” kada delove najave pošalje odvojeno. Suglasnik koji tako stoji sam, kao reč za sebe, Laprdus čita na izabrani način kao i sricano slovo: glasom (*bə*) kada su uključeni glasovi slova, nazivom (*be*) kada nisu. Isto važi za takvo slovo u običnom tekstu (*Plan B*, *tipka F 5*). Predlozi *s* i *k* (na slovenačkom i *v, z, h*) ispred reči ostaju predlozi, skraćenice (*USB*, *s.a.r.s.*) i slova u složenicama (*S-klasa*) čitaju se nazivima, a strana slova *q, w, x, y* uvek nazivima.

**Rečnik sricanja i glasovi slova.** Vaši unosi u rečniku sricanja (odeljak [5.4](#54-rečnik-sricanja-spellingjson)) važe u oba načina, uz jedan izuzetak: kada se slova sriču glasovima, unos koji je samo naziv slova (*be*, *jot*, *lje*) smatra se nazivom i ustupa mesto glasu, dok se unos koji kaže nešto drugo (na primer *B* → *bum*) izgovara onako kako ste ga napisali. Tako stari unosi kojima ste ispravljali nazive ne isključuju glasove.

### 4.8 Laprdus konfigurator (Windows)

U operativnom sistemu Windows dodatna podešavanja možete da promenite u Laprdus konfiguratoru:

1. Otvorite Windows meni Start, pronađite Laprdus konfigurator u programskoj fascikli Laprdus i otvorite ga tasterom Enter.
2. Ili, ako je Laprdus instaliran kao NVDA dodatak, otvorite NVDA meni > podmeni Laprdus > Laprdus konfigurator

Konfigurator je na hrvatskom, srpskom, slovenačkom i engleskom jeziku. Prati jezik prikaza Windowsa, a kada ga otvorite iz NVDA menija, prati jezik NVDA-a. Bosanski jezik prikaza daje hrvatski tekst, a svaki drugi jezik engleski. I instalacioni program nudi ta četiri jezika, a NVDA dodatak prati jezik NVDA-a.

Konfigurator omogućava podešavanje:

- Brzine, visine i jačine govora (brzinu i visinu NVDA klizači ubrzavaju, usporavaju, podižu i spuštaju, osim kada je uključeno prinudno korišćenje Laprdus vrednosti; pogledajte [4.1](#41-brzina-govora))
- Intonacije i ubrzanja, za sve glasove: intonacija od 0% (monotono) preko 50% (podrazumevano, prirodni pokreti) do 100% (dvostruko veći pokreti), u SAPI5 programima (u NVDA-u je određuje NVDA klizač Modulacija); ubrzanje množi brzinu govora u SAPI5 programima (0,5x-3,0x, podrazumevano 1,0x), a u NVDA-u se ne koristi jer veće brzine daje NVDA opcija Povećanje brzine. Klizači brzine i visine za formantne glasove idu od 0,25x do 4,0x.
- Kvaliteta glasa: veličine glave, daha, hrapavosti i svetline, dugmetom „Kvalitet glasa...” ispod pojačanja jačine (pogledajte [4.9](#49-kvalitet-glasa))
- Načina čitanja brojeva
- Trajanja pauza i brzine sricanja
- Sricanja slova glasovima umesto nazivima
- Prilagođavanja korisničkih rečnika

Podešavanja se čuvaju u korisničkoj fascikli `%APPDATA%\Laprdus` i dele se između SAPI5 i NVDA dodatka.

### 4.9 Kvalitet glasa

Četiri podešavanja kvaliteta glasa slede model glasovnih parametara klasičnih formantnih sintetizatora: veličina glave, dah, hrapavost i svetlina. U NVDA to su NVDA sopstvena podešavanja glasa: klizači Veličina glave, Dah, Hrapavost i Svetlina u NVDA podešavanjima glasa, iza jačine, i u krugu govornih podešavanja (Control + NVDA taster + strelice), svako od 0 do 100, a 50 je glas kakav jeste; NVDA ih pamti kao i ostala svoja podešavanja. Laprdus konfigurator ima ista četiri iza dugmeta „Kvalitet glasa...”, ali te vrednosti važe samo za SAPI5 (u datoteci `settings.json` zapisana su kao `speech.head_size`, `speech.breathiness`, `speech.roughness` i `speech.brightness`), kao i njegova podešavanja infleksije.

- **Veličina glave** – dužina govornog trakta: 0 je četvrtinu duži (niži formanti, krupniji glas), 100 četvrtinu kraći (viši formanti, sitniji glas). Deluje na sve glasove.
- **Dah** – šum daha u glasu: 0 ga nema, 100 ga je pet puta više nego u glasu kakav jeste. Deluje na formantne glasove (Zvonko, Stojan, Mirsad i pevajući glasovi).
- **Hrapavost** – nepravilnosti visine i jačine od titraja do titraja (jitter i shimmer): 0 ih nema, 50 je glas kakav jeste; iznad 50 svaki drugi titraj glasa dolazi ranije i slabiji je, promuklost koja raste do 100. Deluje na formantne glasove.
- **Svetlina** – boja tona: 0 je tamno i prigušeno, 100 oštro i svetlo. Deluje na sve glasove: na formantne kroz izvor glasa, a na snimljene kroz nagib spektra oko 1,5 kHz.

---

## 5. Rečnici

Rečnici omogućavaju da prilagodite način na koji Laprdus izgovara određene reči, skraćenice, simbole i emodžije. Pomoću rečnika možete da ispravite izgovor stranih reči, dodate izgovor za skraćenice ili prilagodite čitanje posebnih znakova.

### 5.1 Vrste rečnika

Laprdus koristi tri vrste rečnika, od kojih svaka ima posebnu namenu, i uz njih naglasni rečnik formantnih glasova:

| Vrsta rečnika | Datoteka | Namena |
|----------------|----------|---------|
| **Glavni rečnik** | `user.json` | Zamena reči i fraza prilagođenim izgovorom |
| **Rečnik sricanja** | `spelling.json` | Izgovor pojedinačnih znakova (slova, brojeva, simbola) |
| **Rečnik emodžija** | `emoji.json` | Pretvaranje emodžija u tekstualni opis |
| **Naglasni rečnik** | `accents.json` | Naglasak, dužina i ton reči za formantne glasove (pogledajte [5.11](#511-naglasni-rečnik-accentsjson)) |

### 5.2 Lokacije datoteka rečnika

Lokacija datoteka rečnika zavisi od platforme koju koristite:

#### Windows (SAPI5 i NVDA)

Sve datoteke rečnika nalaze se u korisničkoj fascikli Laprdusa:

```
%APPDATA%\Laprdus\
```

Puna putanja je obično: `C:\Users\VašeKorisničkoIme\AppData\Roaming\Laprdus\`

**Kako da otvorite ovu fasciklu:**

1. Pritisnite tastere Windows+R da biste otvorili dijalog „Pokretanje”
2. Upišite: `%APPDATA%\Laprdus`
3. Pritisnite Enter

Datoteke u ovoj fascikli:

| Datoteka | Opis |
|----------|------|
| `settings.json` | Glavna podešavanja Laprdusa |
| `user.json` | Vaš korisnički rečnik izgovora |
| `spelling.json` | Rečnik sricanja (izgovor znakova) |
| `emoji.json` | Rečnik emodžija |
| `accents.json` | Naglasni rečnik formantnih glasova |

**Napomena:** Ove datoteke se kreiraju automatski kada prvi put dodate unos u Laprdus konfiguratoru. Možete i ručno da ih kreirate.

#### Linux

Datoteke rečnika nalaze se u:

```
~/.config/Laprdus/
```

Koristite iste nazive datoteka kao na Windowsu (`user.json`, `spelling.json`, `emoji.json`, `accents.json`).

#### Android

Na Androidu se rečnici uređuju isključivo preko interfejsa aplikacije Laprdus. Datoteke se čuvaju u internoj memoriji aplikacije i nisu direktno dostupne korisniku.

### 5.3 Glavni rečnik (user.json)

Glavni rečnik služi za zamenu reči ili fraza prilagođenim izgovorom. Koristan je za:

- **Strane reči i brendove** - npr. da se „Facebook” izgovara kao „Fejzbuk”
- **Skraćenice** - npr. da se „TV” izgovara kao „Te Ve”
- **Tehničke pojmove** - npr. da se „ChatGPT” izgovara kao „ČetDžipiti”
- **Imena sa neuobičajenim izgovorom** - npr. da se „Sean” izgovara kao „Šon”

#### Format datoteke

Datoteka `user.json` koristi JSON format. Evo primera:

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
            "comment": "Skraćenica za televiziju"
        },
        {
            "grapheme": "ChatGPT",
            "phoneme": "ČetDžipiti",
            "caseSensitive": false,
            "wholeWord": true,
            "comment": "AI četbot"
        }
    ]
}
```

#### Opis polja

| Polje | Obavezno | Opis | Podrazumevana vrednost |
|-------|----------|------|-------------------|
| `grapheme` | Da | Izvorni tekst koji se zamenjuje | - |
| `phoneme` | Da | Zamenski izgovor | - |
| `caseSensitive` | Ne | Da li se razlikuju velika i mala slova | `false` |
| `wholeWord` | Ne | Zamenjuje se samo cela reč | `true` |
| `comment` | Ne | Komentar za referencu (ne utiče na izgovor) | - |

#### Objašnjenje opcija

**caseSensitive (razlikovanje velikih i malih slova):**

- `false` (podrazumevano): „Facebook”, „facebook” i „FACEBOOK” se zamenjuju
- `true`: Zamenjuje se samo tačno podudaranje

**wholeWord (samo cela reč):**

- `true` (podrazumevano): Zamenjuje se samo kada je tekst cela reč
    - „TV” u „TV program” se zamenjuje
    - „TV” u „žrtva” se ne zamenjuje (jer je deo veće reči)
- `false`: Zamenjuje se i unutar drugih reči
    - Korisno za sufikse i prefikse

### 5.4 Rečnik sricanja (spelling.json)

Rečnik sricanja definiše kako se izgovaraju pojedinačni znakovi kada čitač ekrana sriče tekst (čitanje znak po znak). Koristan je za:

- **Brojeve** - npr. da se „5” izgovara kao „pet”
- **Interpunkciju** - npr. da se „.” izgovara kao „točka”
- **Posebne znakove** - npr. da se „@” izgovara kao „at”
- **Slova abecede** - ako želite da se neko slovo zove drugačije nego što ga Laprdus sam zove, npr. „W” kao „dvostruko ve”

Slova abecede Laprdus sam imenuje, na izabranom jeziku (pogledajte [4.7](#47-sricanje)), pa ih ugrađeni rečnik ne sadrži; vaš unos za slovo ima prednost nad ugrađenim nazivom. Kada je uključeno sricanje glasovima, unos koji je samo naziv slova se zanemaruje, a svaki drugi unos važi.

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

Laprdus dolazi sa ugrađenim rečnikom sricanja brojeva, interpunkcije i simbola (nazivi su hrvatski). Slova se imenuju ovako:

**Slova abecede** (ugrađena u Laprdus, ne u rečnik):

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

Ćirilična slova imenuju se kao njihovi latinični parnjaci, a velika i mala slova isto.

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

### 5.5 Rečnik emodžija (emoji.json)

Rečnik emodžija pretvara emodži simbole u izgovoreni tekst.

#### Uključivanje čitanja emodžija

**Windows (Laprdus konfigurator):**

1. Otvorite Laprdus konfigurator (Start > Laprdus > Laprdus konfigurator)
2. Tabulatorom pronađite opciju „Čitaj emodžije”
3. Potvrdite izbor u polju za potvrdu
4. Tabulatorom izaberite „U redu” i pritisnite Enter

**NVDA:**

1. Otvorite NVDA meni (NVDA taster + N) > podmeni Laprdus > Laprdus konfigurator
2. Tabulatorom pronađite opciju „Čitaj emodžije”
3. Potvrdite izbor u polju za potvrdu
4. Tabulatorom izaberite „U redu” i pritisnite Enter

**Android:**

1. Otvorite aplikaciju Laprdus
2. Dodirnite „Laprdus podešavanja”
3. Pronađite prekidač „Omogući čitanje emodžija” i uključite ga

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

Laprdus dolazi sa obimnim ugrađenim rečnikom emodžija koji sadrži preko 1100 emodžija sa hrvatskim opisima. Evo nekoliko primera:

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

### 5.6 Uređivanje rečnika na Windowsu (Laprdus konfigurator)

Laprdus konfigurator pruža grafički interfejs za jednostavno upravljanje rečnicima. Ovo je **preporučeni metod** za korisnike koji više vole grafički interfejs.

**Kako da otvorite konfigurator:**

- **Iz menija Start:** Start > Laprdus > Laprdus konfigurator
- **Iz NVDA menija:** NVDA taster + N > podmeni Laprdus > Laprdus konfigurator

**Uređivanje rečnika:**

1. U Laprdus konfiguratoru tabulatorom ili kombinacijom Shift+Tab pronađite dugme **„Rečnici...”** i pritisnite Enter
2. Otvara se prozor Upravljanje rečnicima
3. Na vrhu prozora nalazi se padajući meni za izbor vrste rečnika koji želite da prilagodite:
    - **Glavni korisnički rečnik** - za zamenu reči i fraza
    - **Rečnik sricanja** - za izgovor pojedinačnih znakova
    - **Rečnik emodžija** - za pretvaranje emodžija u tekst
    - **Naglasni rečnik** - za naglasak, dužinu i ton reči, za sve njene oblike odjednom (zapis u [5.11](#511-naglasni-rečnik-accentsjson))
4. Izaberite željenu vrstu rečnika strelicom nadole

**Dodavanje novog unosa:**

1. Kliknite na dugme **„Dodaj”**
2. Otvara se dijalog za unos:
    - **Izvorni tekst:** Upišite tekst koji želite da zamenite
    - **Zamenski tekst:** Upišite kako želite da se tekst izgovara
    - **Razlikuj velika i mala slova:** Potvrdite izbor ako je važno tačno podudaranje
    - **Samo cela reč:** Potvrdite izbor ako ne želite zamenu unutar drugih reči
    - **Komentar:** Opcioni opis unosa
3. Pritisnite Enter ili tabulatorom pronađite dugme **„U redu”** i potvrdite tasterom Enter da biste sačuvali unos

**Uređivanje postojećeg unosa:**

1. Izaberite unos na spisku strelicama nadole ili nagore
2. Tabulatorom pronađite dugme **„Uredi”**, a korisnici miša mogu i da kliknu dvaput na unos
3. Izmenite željene vrednosti
4. Pritisnite Enter ili tabulatorom pronađite dugme **„U redu”** i pritisnite Enter

**Brisanje unosa:**

1. Izaberite unos na spisku strelicama nagore ili nadole
2. Tabulatorom pronađite dugme **„Izbriši”** i pritisnite Enter

**Dupliciranje unosa:**

1. Izaberite unos koji želite da duplicirate
2. Tabulatorom pronađite dugme **„Dupliciraj”** i pritisnite Enter
3. Novi unos sa kopiranim vrednostima dodaje se na spisak
4. Uredite duplicirani unos po potrebi

**Unos u naglasni rečnik:**

Kada je izabran naglasni rečnik, dijalog za unos ima druga polja:

- **Unos:** reč, osnova, spisak oblika ili glagol sa naglasnim znakovima, u zapisu iz [5.11](#511-naglasni-rečnik-accentsjson), npr. `kontr'o:l*` ili `ur'e:d=i<p`
- **Vrsta unosa:** *Reč* (reč, osnova sa `*` ili oblici sa `|`) ili *Glagol* (cela osnova sa znakom `=` i vrstom ili koren sa dugim *ije*)
- **Jezik:** *Svi jezici*, ili hrvatski, srpski, bosanski odnosno slovenački ako unos važi samo za jedan jezik
- **Komentar:** opciona beleška, npr. oblici koje unos pokriva

Ispod polja stoji kratak podsetnik na znakove. Unos se proverava istim pravilima po kojima ga Laprdus čita: unos bez naglasnog znaka, sa dva naglaska ili sa nepoznatom vrstom glagola konfigurator odbija i kaže šta nije u redu, pa u datoteku ne može da dospe unos koji bi se pri učitavanju preskočio. Dupliciranje ne menja tekst unosa.

### 5.7 Uređivanje rečnika na Androidu

Android aplikacija pruža ugrađeni interfejs za upravljanje rečnicima.

**Korak 1: Otvorite rečnike**

1. Otvorite aplikaciju Laprdus iz fioke aplikacija
2. Dodirnite dugme **„Laprdus podešavanja”**
3. Dodirnite dugme **„Upravljanje rečnicima”**

**Korak 2: Izaberite vrstu rečnika**

Na vrhu ekrana nalazi se padajući meni za izbor vrste rečnika:

- **Glavni rečnik** - za zamenu reči i fraza
- **Rečnik sricanja** - za izgovor znakova
- **Rečnik emodžija** - za pretvaranje emodžija
- **Naglasni rečnik** - za naglasak reči (zapis u [5.11](#511-naglasni-rečnik-accentsjson))

Dodirnite padajući meni i izaberite željenu vrstu.

**Korak 3: Dodavanje novog unosa**

1. Dodirnite dugme **„+”** (plus) u donjem desnom uglu ekrana
2. Otvara se ekran za unos:
    - **Izvorni tekst:** Upišite reč ili frazu koju želite da zamenite
    - **Zamenski izgovor:** Upišite kako želite da se izgovara
    - **Razlikuj velika i mala slova:** Uključite ako je važno tačno podudaranje
    - **Samo cela reč:** Uključite ako ne želite zamenu unutar drugih reči
    - **Komentar:** Opcioni opis (npr. „Društvena mreža”)
3. Dodirnite dugme **„Sačuvaj”** na dnu ekrana

Za naglasni rečnik ekran za unos ima polja **Unos** (reč, osnova, oblici ili glagol sa naglaskom, npr. `kontr'o:l*`), **Vrsta unosa** (reč ili glagol), **Jezik** (svi jezici ili jedan od četiri) i **Komentar**, sa podsetnikom na znakove ispod njih. Unos koji nije ispravan aplikacija odbija i ispisuje razlog ispod polja.

**Korak 4: Uređivanje postojećeg unosa**

1. Pronađite unos na spisku
2. Dodirnite unos da biste otvorili ekran za uređivanje
3. Izmenite željene vrednosti
4. Dodirnite **„Sačuvaj”**

**Korak 5: Brisanje unosa**

1. Pronađite unos na spisku
2. Prevucite unos ulevo da bi se prikazala opcija za brisanje
3. Dodirnite ikonu kante za smeće
4. Potvrdite brisanje

**Napomena za korisnike čitača ekrana TalkBack:**

- Koristite standardne TalkBack pokrete za navigaciju
- Dvostruki dodir za aktiviranje dugmadi
- Za brisanje: dvostruki dodir i zadržavanje na unosu, a zatim izaberite „Izbriši”

### 5.8 Ručno uređivanje JSON datoteka (napredno)

Ovaj metod je namenjen **naprednim korisnicima** koji žele da direktno uređuju JSON datoteke rečnika. Većini korisnika preporučujemo da koriste Laprdus konfigurator (Windows) ili interfejs Android aplikacije.

**Prednosti ručnog uređivanja:**

- Brže masovno dodavanje/uređivanje velikog broja unosa
- Mogućnost deljenja rečnika sa drugim korisnicima
- Pravljenje rezervnih kopija rečnika

**Korak 1: Otvorite fasciklu sa rečnicima**

**Windows:**

1. Pritisnite Windows+R
2. Upišite: `%APPDATA%\Laprdus`
3. Pritisnite Enter

**Linux:**

1. Otvorite menadžer datoteka
2. Otvorite skrivenu fasciklu `.config` u ličnom direktorijumu
3. Otvorite fasciklu `Laprdus`
    - Ili u terminalu: `cd ~/.config/Laprdus`

**Korak 2: Otvorite ili kreirajte datoteku**

- Ako datoteka već postoji, otvorite je u uređivaču teksta (Windows: Notepad, Linux: gedit, nano, vim)
- Ako datoteka ne postoji, kreirajte novu tekstualnu datoteku sa odgovarajućim imenom:
    - `user.json` za glavni rečnik
    - `spelling.json` za rečnik sricanja
    - `emoji.json` za rečnik emodžija
    - `accents.json` za naglasni rečnik formantnih glasova (zapis u [5.11](#511-naglasni-rečnik-accentsjson))

**Korak 3: Uredite sadržaj**

Koristite JSON format opisan u prethodnim odeljcima (5.3, 5.4, 5.5). Primer za `user.json`:

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
            "comment": "Pretraživač"
        }
    ]
}
```

**VAŽNO - Pravila JSON sintakse:**

- Svi ključevi i tekstualne vrednosti moraju da budu unutar dvostrukih navodnika `"`
- Zarezi `,` između unosa, ALI NE posle poslednjeg unosa
- Vitičaste zagrade `{}` za svaki unos
- Uglaste zagrade `[]` za listu unosa

**Korak 4: Sačuvajte datoteku**

- **Obavezno** sačuvajte datoteku sa **UTF-8 kodiranjem** (za podršku srpskih znakova)
- **Windows Notepad:** Datoteka > Sačuvaj kao > Kodiranje: UTF-8
- **Linux:** Većina uređivača automatski koristi UTF-8

**Korak 5: Ponovo pokrenite sintetizator**

- **Windows SAPI5:** Zatvorite i ponovo otvorite aplikaciju koja koristi Laprdus
- **NVDA:** Pritisnite NVDA taster + Q i ponovo pokrenite NVDA ili pređite na drugi sintetizator, pa se vratite na Laprdus
- **Linux:** Ponovo pokrenite aplikaciju ili Speech Dispatcher: `systemctl --user restart speech-dispatcher`

**Napomena:** Ako uredite rečnik dok je Android aplikacija pokrenuta, promene se neće videti sve dok ne osvežite spisak ili ponovo ne otvorite aplikaciju.

### 5.9 Primeri praktične upotrebe

#### Primer 1: Strane kompanije i brendovi

Mnoge strane kompanije i brendovi imaju izgovor koji se razlikuje od pisanog oblika:

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

#### Primer 2: Skraćenice

Skraćenice za koje želite da se čitaju slovo po slovo:

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
    "comment": "Skraćenica za Hrvatsku"
}
```

```json
{
    "grapheme": "EU",
    "phoneme": "E U",
    "caseSensitive": false,
    "wholeWord": true,
    "comment": "Evropska unija"
}
```

#### Primer 3: Tehnički pojmovi

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

#### Primer 4: Imena sa neuobičajenim izgovorom

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

#### Primer 5: Zamena unutar reči (wholeWord: false)

Da biste zamenili sufikse ili prefikse, podesite `wholeWord` na `false`:

```json
{
    "grapheme": "tion",
    "phoneme": "šen",
    "caseSensitive": false,
    "wholeWord": false,
    "comment": "Engleski sufiks -tion"
}
```

Sa ovim unosom reči kao što je „information” sadrže izgovor „šen” umesto „tion”.

### 5.10 Rešavanje problema sa rečnicima

#### Problem: Promene u rečniku se ne primenjuju

**Mogući uzroci i rešenja:**

1. **Primena korisničkih rečnika je isključena**
    - Uključite opciju **„Primena korisničkih rečnika”** u Laprdus konfiguratoru (Windows) ili u Laprdus podešavanjima (Android)

2. **Sintaksna greška u JSON datoteci**
    - Proverite da li su svi navodnici pravilno zatvoreni
    - Proverite da li su zarezi na pravim mestima
    - Koristite JSON validator na mreži da biste proverili sintaksu

3. **Datoteka nije sačuvana sa UTF-8 kodiranjem**
    - U Notepadu: Datoteka > Sačuvaj kao > Kodiranje: UTF-8
    - U drugim uređivačima: proverite podešavanja kodiranja

4. **Sintetizator nije ponovo pokrenut**
    - Zatvorite i ponovo otvorite aplikaciju
    - Za NVDA: Insert+Q > Pokreni ponovo

5. **Datoteka je na pogrešnoj lokaciji**
    - Windows: `%APPDATA%\Laprdus\`
    - Linux: `~/.config/Laprdus/`

#### Problem: Reč se zamenjuje na pogrešnim mestima

**Rešenje:** Uključite opciju **„Samo cela reč”** za problematični unos ili ručno promenite vrednost za taj unos u konfiguracionoj datoteci, `wholeWord: true`

Primer problema: Unos za „TV” zamenjuje i deo reči „žrtva” jer sadrži „TV”.

```json
{
    "grapheme": "TV",
    "phoneme": "Te Ve",
    "wholeWord": true
}
```

#### Problem: Zamena ne radi za velika/mala slova

**Rešenje:** Proverite opciju **„Razlikuj velika i mala slova”** za problematični unos ili proverite upis u konfiguracionoj datoteci za taj unos, `caseSensitive`

- Za zamenu nezavisno od veličine slova: `caseSensitive: false`, odnosno isključeno
- Za tačno podudaranje: `caseSensitive: true`, odnosno uključeno

#### Problem: Sintaksna greška u JSON-u

Česte greške u JSON formatu:

**Zaboravljen zarez:**

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

**Rešenje:** Proverite da li je čitanje emodžija uključeno

1. Otvorite Laprdus konfigurator ili Android podešavanja
2. Pronađite opciju „Čitaj emodžije” (na Androidu „Omogući čitanje emodžija”)
3. Uključite opciju
4. Sačuvajte podešavanja

---

### 5.11 Naglasni rečnik (accents.json)

Formantni glasovi (Zvonko, Stojan, Mirsad i pevajući glasovi) sami određuju mesto naglaska, dužinu i ton svake reči: prvo iz naglasnih znakova u tekstu, zatim iz ugrađenog leksikona, pa po pravilima za nastavke i vrste reči (pozajmljenice i izvedenice sa dugim poslednjim slogom osnove, glagoli sa prefiksom, glagoli na -ovati), a ako ništa ne odgovara, naglasak ide na prvi slog. Reč koja tako bude pogrešno naglašena može da se ispravi u naglasnom rečniku, i to ne samo jedan njen oblik: unos može da pokrije celu osnovu, spisak oblika ili ceo glagol sa svim licima i vremenima. Snimljeni glasovi (Josip, Vlado i izvedeni) naglasak određuju na isti način i koriste isti rečnik; rečnik važi za izabrani jezik, kojim god glasom da se govori.

Datoteka `accents.json` stoji uz ostale rečnike (`%APPDATA%\Laprdus` na Windowsu, `~/.config/Laprdus` na Linuxu) i učitava se zajedno s njima kada su korisnički rečnici uključeni. Učitava se jednom, pri pokretanju i pri promeni datoteke; govor zbog nje nije nimalo sporiji.

#### Zapis

Unosi se pišu u zapisu ugrađenog leksikona:

| Znak | Značenje |
|------|----------|
| `'` | ispred naglašenog samoglasnika (ili slogotvornog r); ton se bira sam |
| `^` | ispred naglašenog samoglasnika: silazni naglasak |
| `/` | ispred naglašenog samoglasnika: uzlazni naglasak |
| `:` | iza dugog samoglasnika |
| `*` | na kraju: osnova, koja važi i za oblike sa do tri slova više (kontróla, kontróle, kontrólom) |
| `\|` | `osnova\|nastavak\|nastavak`: po jedan tačan oblik za svaki nastavak; prazan nastavak je sama osnova |

Primer datoteke:

```json
{
    "version": "1.0",
    "entries": [
        { "word": "kontr'o:l*", "comment": "kontróla, kontróle, kontrólom" },
        { "word": "sign'a:l|a|u|om|e|i|ima", "comment": "signála, signálu...; sìgnal ostaje kako jeste" },
        { "word": "p^oruk*", "comment": "pȍruka: silazni naglasak na prvom slogu" },
        { "word": "dir'ektor*", "language": "sr", "comment": "samo na srpskom, kojim god glasom" },
        { "verb": "ur'e:d=i<p", "comment": "uréditi, urédim, urédio, uređen" },
        { "verb": "dijel:i", "comment": "podijeliti, raspodijeliti, podijelim, podijeljen" }
    ]
}
```

- `word`: reč, osnova (`*`) ili spisak oblika (`|`).
- `verb`: glagol. Piše se cela osnova sa naglaskom infinitiva, znak `=` i vrsta: `i` za glagole na -iti/-im, `a` na -ati/-am, `u` na -nuti/-nem, `t` na -ati sa drugom osnovom u prezentu i `e` za tu osnovu (`pok'a:z=t` i `pok'a:ž=e`). Iza vrste mogu da stoje oznake: `<` pomera naglasak jedan slog unazad u prezentu, `p` i u trpnom pridevu (tako govore Stojan i Mirsad, Zvonko zadržava naglasak infinitiva), `n` kada je imperativ jednak obliku neke imenice (potvrdi, uredi), pa se čita kao glagol samo na početku rečenice. Glagol sa dugim *ije* u korenu piše se kao `korijen:vrsta` i važi sa svakim prefiksom.
- `language`: `hr`, `sr`, `bs` ili `sl` ograničava unos na taj jezik, kojim god glasom da se govori; bez toga važi za sve jezike. Slovenački unos piše se u zapisu slovenačkih rečnika (`mésto`, `člôvek`, `p'əs`, `d'é:la*`): ostrivec za dugi naglašeni samoglasnik (é, ó uski), strešica za dugi široki ê, ô, krativec za kratki naglašeni, `ə` za poluglasnik i `w` za l koji se izgovara kao u (bíw).
- `comment`: beleška za vas, Laprdus je ne čita.

Tačan oblik ima prednost nad osnovom, duža osnova nad kraćom, a vaš unos nad ugrađenim. Naglasni znakovi napisani u samom tekstu ili u zameni glavnog rečnika (`telèfon`) imaju prednost nad svime.

Neispravan unos (bez naglasnog znaka, sa dva naglaska, sa nepoznatim slovom ili nepoznatom vrstom glagola) se preskače, a ostali važe. Na komandnoj liniji `laprdus -w` ispisuje koliko je unosa prihvaćeno i šta nije bilo u redu sa prvim odbijenim; NVDA dodatak to upisuje u svoj dnevnik. Rezultat možete odmah da čujete:

```bash
laprdus -v zvonko "kontrola, signala, uredio"
```

**Uređivanje:** naglasni rečnik se uređuje kao i ostali rečnici: u Laprdus konfiguratoru na Windowsu (dugme **Rečnici...**, vrsta rečnika *Naglasni rečnik*, pogledajte [5.6](#56-uređivanje-rečnika-na-windowsu-laprdus-konfigurator)), u Android aplikaciji ([5.7](#57-uređivanje-rečnika-na-androidu)) i u aplikaciji za iPhone, iPad i Mac (kartica Rečnici, vrsta *Naglasni rečnik*). Svaki unos ima polja Unos, Vrsta unosa (reč ili glagol), Jezik i Komentar, a uređivač ga proverava istim pravilima po kojima ga Laprdus čita, pa neispravan unos ne može da bude sačuvan. Datoteku možete i dalje da uređujete ručno ([5.8](#58-ručno-uređivanje-json-datoteka-napredno)); aplikacije na Androidu i Appleu čitaju je iz iste fascikle kao ostale rečnike kada su korisnički rečnici uključeni.

## 6. Glasovi

### 6.1 Dostupni glasovi

Laprdus sadrži osamnaest glasova - dva osnovna, tri izvedena, tri formantna i deset pevajućih. **Svaki glas govori svaki jezik.** Glas određuje samo zvuk; jezik (hrvatski, srpski, bosanski ili slovenački) određuje pravila izgovora, naglasni rečnik, reči za brojeve (*tisuća* / *hiljada* / *tisoč*) i nazive slova pri sricanju, a bira se odvojeno od glasa, na svakoj platformi na svoj način (pogledajte [6.3](#63-izbor-glasa)). *Izvorni jezik* u tabeli je jezik kojim glas govori kada se ništa drugo ne zatraži. **Podrazumevani glas je Zvonko**, na svim platformama i za sve jezike. Oznaka glasa (u koloni Oznaka) namerno se piše bez dijakritičkih znakova, da bi se glasovi sigurno registrovali na svakoj platformi; oznake iz prethodnih verzija (`djed`, `klapa`, `pjevac`, `pevac`, `solist`) i dalje važe u sačuvanim podešavanjima i skriptama.

| Glas | Oznaka | Vrsta | Izvorni jezik | Opis |
|------|--------|-------|---------------|------|
| **Josip** | `josip` | Osnovni | Hrvatski | Muški glas normalne visine. |
| **Vlado** | `vlado` | Osnovni | Srpski | Muški glas normalne visine. |
| **Detence** | `detence` | Izvedeni | Hrvatski | Dečji glas. Izveden iz glasa Josip sa povišenom visinom. |
| **Baba** | `baba` | Izvedeni | Hrvatski | Ženski glas. Izveden iz glasa Josip sa blago povišenom visinom. |
| **Đedo** | `djedo` | Izvedeni | Srpski | Stariji muški glas. Izveden iz glasa Vlado sa sniženom visinom. |
| **Zvonko** | `zvonko` | Formantni | Hrvatski | Muški glas. Govor nastaje po pravilima, bez snimaka. Podrazumevani glas. |
| **Stojan** | `stojan` | Formantni | Srpski | Muški glas, dublji i tamniji. Govor nastaje po pravilima, bez snimaka. |
| **Mirsad** | `mirsad` | Formantni | Bosanski | Muški glas, viši i melodičniji. Govor nastaje po pravilima, bez snimaka. |
| **Orguljaš - Vila Velebita** | `orguljas` | Pevajući | Hrvatski | Zvonko za orguljama: peva *Vilu Velebita*. |
| **Dalmacija Klapa** | `dalmacija_klapa` | Pevajući | Hrvatski | Zvonko kao cela klapa: peva *Vilu Velebita*. |
| **Lika Velebit** | `lika_velebit` | Pevajući | Hrvatski | Zvonko peva svojim glasom, u svojoj visini: *Vila Velebita*. |
| **Trubač - Kreće se lađa francuska** | `trubac` | Pevajući | Srpski | Stojan kao truba: peva *Kreće se lađa francuska*. |
| **Harmonikaš - Kreće se lađa francuska** | `harmonikas` | Pevajući | Srpski | Stojan kao harmonika: peva *Kreće se lađa francuska*. |
| **Solunac** | `solunac` | Pevajući | Srpski | Stojan peva svojim glasom: *Kreće se lađa francuska*. |
| **Sevdalija - Kad ja pođoh na Bembašu** | `sevdalija` | Pevajući | Bosanski | Mirsad peva sevdah: *Kad ja pođoh na Bembašu*. |
| **Sazlija - Kad ja pođoh na Bembašu** | `sazlija` | Pevajući | Bosanski | Mirsad kao saz: peva *Kad ja pođoh na Bembašu*. |
| **Sarajevo Bembaša** | `sarajevo_bembasa` | Pevajući | Bosanski | Mirsad peva svojim glasom: *Kad ja pođoh na Bembašu*. |
| **Bećarac** | `becarac` | Pevajući | Hrvatski | Zvonko svojim glasom peva bećarac: svaki deseterački dvostih leže na napev. |

### 6.2 Osnovni i izvedeni glasovi

**Osnovni glasovi** (Josip i Vlado) koriste fizičke snimke fonema - stvarne zvučne zapise govornika.

**Izvedeni glasovi** (Detence, Baba, Đedo) koriste iste zvučne zapise kao osnovni glasovi, ali sa prilagođenom visinom glasa:

- **Detence** koristi Josipove foneme sa visinom 1,5x (viši glas)
- **Baba** koristi Josipove foneme sa visinom 1,2x (blago viši glas)
- **Đedo** koristi Vladove foneme sa visinom 0,75x (niži glas)

Osnovni i izvedeni glasovi, kao i svi ostali, govore sva četiri jezika: Josip na srpskom kaže *hiljada* i sriče *lje*, Vlado na hrvatskom *tisuća* i *elj*, a na slovenačkom *tisoč*.

**Formantni glasovi** (Zvonko, Stojan, Mirsad) ne koriste snimke. Govor se u celosti izračunava po pravilima, kao kod klasičnih formantnih sintetizatora: iz teksta se određuju glasovi, naglasak i rečenična intonacija, a zatim formantni sintetizator stvara zvuk. Zato ovi glasovi:

- ne traže datoteke sa glasovnim podacima
- ostaju čisti i razumljivi pri veoma velikim brzinama, jer se brzina i visina ne dobijaju naknadnom obradom zvuka
- imaju rečeničnu intonaciju (izjava, pitanje, zarez, uzvik)
- brojeve izgovaraju, kao i svi glasovi, prema izabranom jeziku (hrvatski: *tisuća*, *milijun*; srpski: *hiljada*, *milion*, *dvesta*; bosanski: *hiljada*, *milion*, *dvjesto*; slovenački: *tisoč*, *milijon*, *enaindvajset*)

Ako formantni glas neku reč naglasi na pogrešnom slogu, naglasak možete da odredite u rečniku izgovora uobičajenim naglasnim znakovima, na primer `telèfon`, `gláva`, `kȕća` ili `grȃd`.

**Pevajući glasovi** (Orguljaš - Vila Velebita, Dalmacija Klapa, Lika Velebit i Bećarac Zvonkovim glasom, Trubač - Kreće se lađa francuska, Harmonikaš - Kreće se lađa francuska i Solunac Stojanovim, Sevdalija - Kad ja pođoh na Bembašu, Sazlija - Kad ja pođoh na Bembašu i Sarajevo Bembaša Mirsadovim) formantni su glasovi koji tekst ne govore nego pevaju, kao što su to radili pevajući glasovi klasičnih sintetizatora i stonih računara. Svaki slog dobija jednu notu narodne pesme (Zvonkovi glasovi pevaju *Vilu Velebita*, Stojanovi *Kreće se lađa francuska*, Mirsadovi *Kad ja pođoh na Bembašu*), na onom od četiri jezika koji je izabran; prva strofa pesme tačno leže na melodiju, a svaki drugi tekst peva se na istu melodiju, slog po slog, ispočetka sa svakom novom rečenicom koju program pošalje. Brzina govora određuje tempo, visina transponuje pesmu, a intonacija određuje dubinu vibrata (0% ga isključuje). Umesto grkljana svaki glas ima svoj „instrument”: orgulje sa odjekom crkve, klapu od nekoliko glasova sa basom oktavu niže, trubu, dva harmonikaška piska štimovana s razlikom od nekoliko centi, sevdalijski glas sa širokim vibratom i saz sa tremolom. Lika Velebit, Solunac i Sarajevo Bembaša pevaju običnim glasom Zvonka, Stojana i Mirsada, bez instrumenta i odjeka, a pesma im je preneta u visinu u kojoj inače govore.

### 6.3 Izbor glasa

#### Windows SAPI5

SAPI5 nudi četiri glasa, po jedan za svaki jezik: Laprdus Croatian, Laprdus Serbian, Laprdus Bosnian i Laprdus Slovenian. Oni biraju jezik; glas kojim će govoriti bira se u Laprdus konfiguratoru i važi za sva četiri (u `settings.json` to je `voice.default`; ključa za jezik nema, jer jezik bira program koji govori). Podrazumevani glas je Zvonko.

1. Otvorite Laprdus konfigurator i u grupi Podešavanja glasa izaberite željeni glas (Josip, Vlado, Detence, Baba, Đedo, Zvonko, Stojan, Mirsad ili pevajući glas), a zatim sačuvajte
2. Otvorite Kontrolnu tablu
3. Izaberite „Lakši pristup” > „Prepoznavanje govora” > „Pretvaranje teksta u govor”
4. U padajućem meniju „Izbor glasa” izaberite jezik: Laprdus Croatian, Laprdus Serbian, Laprdus Bosnian ili Laprdus Slovenian
5. Kliknite na „Primeni”

#### NVDA

U NVDA-u su svi Laprdus glasovi dostupni za svaki jezik: glas i jezik biraju se odvojeno (pogledajte [3.3](#33-nvda-podešavanja)). Podrazumevani glas je Zvonko.

**Metod 1: Podešavanja govora**

1. Otvorite NVDA meni (NVDA taster + N)
2. Izaberite Opcije > Podešavanja > Govor
3. U padajućem meniju „Glas” izaberite željeni glas (spisak sadrži svih 18 glasova)
4. U padajućem meniju „Jezik” izaberite hrvatski, srpski, bosanski ili slovenački
5. Pritisnite „U redu” ili „Primeni”

**Metod 2: Krug govornih podešavanja**

1. Pritiskajte Control + NVDA taster + strelica nalevo/nadesno dok ne dođete do podešavanja „Glas” ili „Jezik”, a zatim strelicom nagore/nadole promenite vrednost
2. Ili koristite Control + NVDA taster + V da biste otvorili dijalog za izbor glasa

Ako je uključena NVDA opcija „Automatska promena jezika (kada je podržana)”, Laprdus među hrvatskim, srpskim, bosanskim i slovenačkim sam prelazi prema jeziku teksta, ne menjajući glas.

#### Linux (Speech Dispatcher)

Speech Dispatcher svaki glas navodi za svaki jezik. Jezik zahteva (`-l`) određuje jezik, a glas (`-y`) ostaje onaj koji ste izabrali; bez izabranog glasa govori Zvonko, na sva četiri jezika. Simbolički glasovi su isti za sve jezike: MALE1 je Zvonko, MALE2 Josip, MALE3 Đedo, FEMALE1 Baba, CHILD_MALE Detence.

```bash
# Spisak dostupnih glasova
spd-say -o laprdus -L

# Korišćenje određenog glasa (govori svoj izvorni jezik)
spd-say -o laprdus -y Josip "Tekst na hrvatskom"
spd-say -o laprdus -y Vlado "Tekst na srpskom"

# Bilo koji glas na bilo kom jeziku
spd-say -o laprdus -l sr -y Zvonko "Zvonko govori srpski"
spd-say -o laprdus -l hr -y Vlado "Vlado govori hrvatski"
spd-say -o laprdus -l bs "Zvonko govori bosanski"
spd-say -o laprdus -l sl "Zvonko govori slovensko"
```

Za trajnu promenu u čitaču ekrana Orca:

1. Otvorite Orca podešavanja (Insert+razmaknica)
2. Na kartici „Voice” izaberite željeni glas iz padajućeg menija; jezik kojim govori određuje jezik koji Orca traži

#### Android

Laprdus sistemu nudi svaki glas pod hrvatskim, srpskim, bosanskim i slovenačkim (72 sistemska glasa). Jezik koji zatraži TalkBack ili druga aplikacija određuje jezik, a glas ostaje onaj izabran u aplikaciji; podrazumevani je Zvonko.

1. Otvorite aplikaciju Laprdus
2. Dodirnite „Laprdus podešavanja”
3. U odeljku „Glas” dodirnite padajući meni **„Glas”**
4. Izaberite željeni glas sa spiska
5. U meniju **„Jezik”** izaberite jezik za govor u samoj aplikaciji; kada je uključeno „Prinudno koristi jezik”, tim jezikom se govori i kada aplikacije zatraže drugi

Da biste promenili podrazumevani glas u sistemu:

1. Otvorite Podešavanja > Pristupačnost > Izlaz za pretvaranje teksta u govor
2. Dodirnite ikonu zupčanika pored „Laprdus TTS”
3. Izaberite željeni glas

#### Apple (iPhone, iPad i Mac)

U Izgovorenom sadržaju (iOS: Podešavanja > Pristupačnost > Izgovoreni sadržaj > Glasovi; macOS: Sistemska podešavanja > Pristupačnost > Izgovoreni sadržaj > Sistemski glas > Upravljaj glasovima) i u VoiceOver podešavanjima glasa svaki Laprdus glas je naveden pod hrvatskim, srpskim, bosanskim i slovenačkim; stavka koju izaberete bira i glas i jezik. U aplikaciji Laprdus meni „Glas” bira glas, a meni „Jezik” jezik za govor u samoj aplikaciji. Podrazumevani glas je Zvonko.

---

## 7. Rešavanje problema

### 7.1 Česti problemi

#### Nema zvuka

1. Proverite da li je Laprdus izabran kao aktivni sintetizator
2. Proverite da jačina nije podešena na 0
3. Proverite sistemsku jačinu zvuka na računaru
4. Ponovo pokrenite program koji koristite

#### Govor je prebrz ili prespor

Podesite brzinu govora:

- U NVDA-u: NVDA taster + N > Opcije > Podešavanja > Govor > Brzina
- Na Androidu: Otvorite aplikaciju Laprdus i podesite klizač brzine u Laprdus podešavanjima
- U programu za komandnu liniju: koristite opciju `-r`

#### NVDA ne pronalazi Laprdus

1. Proverite da li je dodatak ispravno instaliran (NVDA meni > Alati > Prodavnica dodataka)
2. Ponovo pokrenite NVDA
3. Proverite da dodatak nije onemogućen

#### Speech Dispatcher ne radi na Linuxu

1. Proverite da li je Laprdus modul instaliran:

        ls /usr/lib/speech-dispatcher-modules/sd_laprdus

2. Proverite konfiguraciju:

        grep laprdus /etc/speech-dispatcher/speechd.conf

3. Ponovo pokrenite Speech Dispatcher:

        systemctl --user restart speech-dispatcher

### 7.2 Dijagnostički dnevnici

#### NVDA dodatak

Da biste uključili dijagnostičke dnevnike za NVDA dodatak:

1. Kreirajte praznu datoteku `laprdus_debug` u privremenoj fascikli
    - Windows: `%TEMP%\laprdus_debug`
2. Ponovo pokrenite NVDA
3. Dnevnici se čuvaju u `%TEMP%\laprdus_debug.log`

#### Linux

Koristite opciju `-w` za opširniji ispis:

```bash
laprdus -w "Test"
```

### 7.3 Kontakt i podrška

Za pomoć i podršku:

- **E-pošta:** hrvojekatic@gmail.com
- **Veb stranica:** https://hrvojekatic.com/laprdus
- **Izvorni kod:** https://github.com/hkatic/laprdus

Kada prijavljujete problem, navedite:

- Verziju Laprdusa
- Operativni sistem
- Korišćeni čitač ekrana (ako je primenljivo)
- Opis problema
- Korake za reprodukciju problema

---

Poslednja izmena: novembar 2026.
