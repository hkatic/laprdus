# Laprdus - Uporabniški priročnik

Različica 2.0

---

## Kazalo

1. [Uvod](#1-uvod)
    - [1.1 Kaj je Laprdus?](#11-kaj-je-laprdus)
    - [1.2 Komu je Laprdus namenjen?](#12-komu-je-laprdus-namenjen)
    - [1.3 Podprte platforme](#13-podprte-platforme)
    - [1.4 Zmožnosti](#14-zmožnosti)
2. [Nameščanje](#2-nameščanje)
    - [2.1 Windows SAPI5](#21-windows-sapi5)
    - [2.2 Dodatek za NVDA](#22-dodatek-za-nvda)
    - [2.3 Linux](#23-linux)
    - [2.4 Android](#24-android)
3. [Uporaba](#3-uporaba)
    - [3.1 Osnovna uporaba](#31-osnovna-uporaba)
    - [3.2 Ukazna vrstica (Linux in Windows)](#32-ukazna-vrstica-linux-in-windows)
    - [3.3 Nastavitve v NVDA](#33-nastavitve-v-nvda)
    - [3.4 Aplikacija za Android](#34-aplikacija-za-android)
4. [Nastavitve](#4-nastavitve)
    - [4.1 Hitrost govora](#41-hitrost-govora)
    - [4.2 Višina glasu](#42-višina-glasu)
    - [4.3 Glasnost](#43-glasnost)
    - [4.4 Premori](#44-premori)
    - [4.5 Način branja števil](#45-način-branja-števil)
    - [4.6 Intonacija](#46-intonacija)
    - [4.7 Črkovanje](#47-črkovanje)
    - [4.8 Konfigurator Laprdus (Windows)](#48-konfigurator-laprdus-windows)
    - [4.9 Kakovost glasu](#49-kakovost-glasu)
5. [Slovarji](#5-slovarji)
    - [5.1 Vrste slovarjev](#51-vrste-slovarjev)
    - [5.2 Kje so datoteke slovarjev](#52-kje-so-datoteke-slovarjev)
    - [5.3 Glavni slovar (user.json)](#53-glavni-slovar-userjson)
    - [5.4 Slovar črkovanja (spelling.json)](#54-slovar-črkovanja-spellingjson)
    - [5.5 Slovar emodžijev (emoji.json)](#55-slovar-emodžijev-emojijson)
    - [5.6 Urejanje slovarjev v sistemu Windows](#56-urejanje-slovarjev-v-sistemu-windows-konfigurator-laprdus)
    - [5.7 Urejanje slovarjev v Androidu](#57-urejanje-slovarjev-v-androidu)
    - [5.8 Ročno urejanje datotek JSON](#58-ročno-urejanje-datotek-json-napredno)
    - [5.9 Primeri praktične uporabe](#59-primeri-praktične-uporabe)
    - [5.10 Odpravljanje težav s slovarji](#510-odpravljanje-težav-s-slovarji)
    - [5.11 Naglasni slovar (accents.json)](#511-naglasni-slovar-accentsjson)
6. [Glasovi](#6-glasovi)
    - [6.1 Razpoložljivi glasovi](#61-razpoložljivi-glasovi)
    - [6.2 Osnovni in izpeljani glasovi](#62-osnovni-in-izpeljani-glasovi)
    - [6.3 Izbira glasu](#63-izbira-glasu)
7. [Odpravljanje težav](#7-odpravljanje-težav)
    - [7.1 Pogoste težave](#71-pogoste-težave)
    - [7.2 Diagnostični dnevniki](#72-diagnostični-dnevniki)
    - [7.3 Stik in podpora](#73-stik-in-podpora)

---

## 1. Uvod

### 1.1 Kaj je Laprdus?

Laprdus je govorni sintetizator (TTS – Text-to-Speech) za hrvaščino, srbščino, bosanščino in slovenščino. Njegovi prvotni glasovi uporabljajo tehnologijo konkatenativne sinteze in govor sestavljajo iz vnaprej posnetih fonemskih enot, formantni glasovi pa govor v celoti izračunajo po pravilih, brez posnetkov. Vsak glas govori vse štiri jezike. Čeprav ne doseže kakovosti sodobnih nevronskih sistemov TTS, ponuja Laprdus visoko zmogljivost in zelo majhno porabo pomnilnika.

Laprdus je bil razvit, da bi uporabnikom bralnikov zaslona brezplačno omogočil preprost in hiter dostop do računalnikov in mobilnih naprav v njihovem maternem jeziku.

Laprdus, ki se je začel kot moj osebni konjiček, je zamišljen tudi kot poskus: z njim preverjam, kako daleč lahko pridem.

### 1.2 Komu je Laprdus namenjen?

Laprdus ni namenjen vsem. Dobra izbira je za tiste, ki uživajo v nostalgičnem zvoku retro govornih sintetizatorjev iz osemdesetih in devetdesetih let 20. stoletja.

Morda ne bo ustrezal uporabnikom, ki so vajeni visoke kakovosti glasu sodobnih sintetizatorjev z umetno inteligenco in nevronskimi mrežami. Lahko pa je idealen za tiste, ki sta jim zmogljivost in majhna poraba virov pomembnejši od kakovosti glasu.

Na kratko: sodobni sintetizatorji ponujajo kakovost na račun virov, Laprdus pa zmogljivost na račun kakovosti.

### 1.3 Podprte platforme

Laprdus je na voljo na teh platformah:

- **Windows 7 do Windows 11** – prek standarda Microsoft SAPI5, zato lahko Laprdus uporabljate v vseh programih, ki podpirajo govor v sistemu Windows, tudi v bralnikih zaslona, kot so Pripovedovalec, NVDA in JAWS
- **Neposredna integracija z bralnikom zaslona NVDA** – na voljo je tudi kot poseben dodatek za brezplačni bralnik zaslona NVDA za Windows
- **Linux** – prek sistema Speech Dispatcher za bralnik zaslona Orca in kot program za ukazno vrstico
- **Android** – kot vgrajen govorni sintetizator za naprave Android

### 1.4 Zmožnosti

Laprdus ponuja te zmožnosti:

- Govor v hrvaščini, srbščini, bosanščini in slovenščini (podprti sta latinica in cirilica)
- Osemnajst glasov (dva osnovna, trije izpeljani, trije formantni in deset pojočih), od katerih vsak govori vse štiri jezike (hrvaščino, srbščino, bosanščino in slovenščino); jezik izberete ločeno od glasu
- Nastavljanje hitrosti govora
- Nastavljanje višine glasu
- Nastavljanje glasnosti
- Naravna intonacija, ki sledi ločilom
- Branje celih števil ali števko za števko
- Nastavljanje trajanja premorov za različna ločila
- Uporabniški slovarji za prilagajanje izgovarjave besed, fraz, simbolov in emodžijev

---

## 2. Nameščanje

### 2.1 Windows SAPI5

Če želite namestiti Laprdus v sistem Windows, sledite tem korakom:

#### 1. korak: prenos

1. Obiščite uradno spletno mesto Laprdusa: https://hrvojekatic.com/laprdus
2. Poiščite razdelek za prenos in izberite povezavo za Windows SAPI5
3. Prenesite namestitveni program (datoteka z imenom `Laprdus_SAPI5_Setup.exe`)
4. Datoteka se shrani v mapo Prenosi (Downloads)

#### 2. korak: zagon namestitve

1. Odprite Raziskovalec (tipki Windows+E)
2. S puščico dol poiščite mapo Prenosi in jo odprite s tipko Enter
3. Poiščite preneseni namestitveni program Laprdusa (datoteka z imenom `Laprdus_SAPI5_Setup.exe`)
4. **Priporočeno:** Na datoteki pritisnite tipko za aplikacije ali tipki Shift+F10, nato izberite »Zaženi kot skrbnik«
    - Druga možnost: Pritisnite Enter za običajen zagon
5. Če se prikaže okno Nadzor uporabniškega računa (UAC), izberite »Da«

#### 3. korak: potek namestitve

1. S puščico dol izberite jezik namestitve in pritisnite Enter
2. Izberite mesto namestitve (privzeto: `C:\Program Files\Laprdus`), s tipko Tab pojdite na »Naprej« in pritisnite Enter
3. Če želite na namizju ikono za Konfigurator Laprdus, s preslednico označite »Ustvari bližnjico na namizju«, nato s tipko Tab poiščite »Naprej« in pritisnite Enter.
4. S tipko Tab poiščite »Namesti«, pritisnite Enter in počakajte, da se namestitev konča
5. Na koncu namestitve pritisnite Enter na gumbu »Končaj«, da zaprete namestitveni program

#### 4. korak: preverjanje namestitve

1. Odprite Nadzorno ploščo (Control Panel):
    - Pritisnite tipko Windows
    - Vnesite »Nadzorna plošča« in pritisnite Enter
2. Izberite »Dostopnost« > »Prepoznavanje govora«
3. S tipko Tab poiščite »Pretvorba besedila v govor« in pritisnite Enter
4. V spustnem seznamu »Izbira glasu« bi morali videti štiri glasove Laprdus:
    - Laprdus Croatian (hrvaščina)
    - Laprdus Serbian (srbščina)
    - Laprdus Bosnian (bosanščina)
    - Laprdus Slovenian (slovenščina)

    Glas SAPI5 izbere samo jezik. Vsi štirje govorijo z glasom, ki ga izberete v Konfiguratorju Laprdus (dokler ga ne spremenite, je to Zvonko): to je lahko Josip, Vlado, Detence, Baba, Đedo, Zvonko, Stojan, Mirsad ali eden od desetih pojočih glasov (glejte [6.1](#61-razpoložljivi-glasovi)). Namestitveni program pri namestitvi, nadgradnji in popravilu odstrani glasove, ki so jih registrirale prejšnje različice (po enega za vsak glas Laprdusa).

### 2.2 Dodatek za NVDA

Za uporabo Laprdusa z bralnikom zaslona NVDA:

#### 1. korak: prenos

1. Obiščite uradno spletno mesto Laprdusa: https://hrvojekatic.com/laprdus
2. Poiščite razdelek za prenos in izberite povezavo za dodatek za NVDA
3. Prenesite dodatek za NVDA (datoteka s pripono `.nvda-addon`)
4. Datoteka se shrani v mapo Prenosi

#### 2. korak: nameščanje dodatka

1. Odprite Raziskovalec (tipki Windows+E)
2. Poiščite mapo Prenosi
3. Poiščite preneseno datoteko dodatka Laprdus za NVDA (datoteka s pripono `.nvda-addon`)
4. Na datoteki pritisnite Enter
    - **Druga možnost:** Pritisnite tipko za aplikacije (ali tipki Shift+F10), da odprete priročni meni, nato izberite »Odpri«
5. NVDA prikaže pogovorno okno z vprašanjem »Ste prepričani, da želite namestiti ta dodatek?«
6. S tipko Tab poiščite gumb »Da« in pritisnite Enter ali preprosto pritisnite D za »Da«
7. Počakajte, da se nameščanje konča

#### 3. korak: ponovni zagon NVDA

1. NVDA vas pozove k ponovnemu zagonu
2. Izberite »Da« za takojšen ponovni zagon
3. Počakajte, da se NVDA znova zažene

#### 4. korak: izbira Laprdusa kot sinteze

**1. način: prek menija NVDA**

1. Odprite meni NVDA: pritisnite tipko NVDA + N (Opomba: tipka NVDA je v vašem sistemu lahko Insert ali Caps Lock, odvisno od tega, kako ste jo nastavili. Če je tipka NVDA nastavljena na Insert, lahko pritisnete Insert + N.)
2. S puščico dol poiščite »Izbire« in pritisnite Enter
3. S puščico dol poiščite »Nastavitve...« in pritisnite Enter
4. V pogovornem oknu Nastavitve s puščico dol poiščite kategorijo »Govor«
5. S tipko Tab poiščite gumb »Spremeni...«, ki je takoj ob možnosti »Sinteza«
6. Na gumbu »Spremeni...« pritisnite Enter
7. Na seznamu sintez s puščico dol poiščite »Laprdus« in pritisnite Enter
8. Pritisnite Escape, da zaprete nastavitve

**2. način: hitra bližnjica (priporočeno)**

1. Pritisnite bližnjico Control + tipka NVDA + S (Opomba: tipka NVDA je v vašem sistemu lahko Insert ali Caps Lock, odvisno od tega, kako ste jo nastavili. Če je tipka NVDA nastavljena na Insert, lahko pritisnete Control + Insert + S.)
2. Odpre se pogovorno okno »Izberi Sintezo«
3. S puščico dol poiščite »Laprdus« in pritisnite Enter

### 2.3 Linux

#### Debian in Ubuntu

Odprite terminal in zaženite ta ukaz (ime datoteke zamenjajte z imenom prenesene datoteke .deb):

```bash
sudo dpkg -i laprdus_amd64.deb
```

#### Fedora

Zaženite ta ukaz (ime datoteke zamenjajte z imenom prenesene datoteke .rpm):

```bash
sudo rpm -i laprdus.x86_64.rpm
```

#### Arch Linux

1. S spletnega mesta prenesite datoteko PKGBUILD
2. V terminalu se premaknite v mapo z datoteko PKGBUILD
3. Zaženite:

```bash
makepkg -si
```

#### Ročna namestitev (iz arhiva tarball)

1. Razširite arhiv (ime datoteke zamenjajte z imenom prenesenega arhiva):

        tar xf laprdus-linux-x86_64.tar.xz
        cd laprdus-linux-x86_64

2. Zaženite namestitveni skript:

        sudo ./install.sh

#### Nastavitev za bralnik zaslona Orca

Laprdus se po namestitvi samodejno nastavi za Speech Dispatcher. Za uporabo z bralnikom zaslona Orca:

1. Za vsak primer priporočamo, da znova zaženete Speech Dispatcher:

        systemctl --user restart speech-dispatcher

2. Odprite nastavitve bralnika Orca:
    - Pritisnite Insert+preslednica, da odprete nastavitve (ali Caps Lock + preslednica, če je modifikator bralnika Orca nastavljen na Caps Lock ali postavitev Laptop)
    - Ali pa v namiznem okolju Gnome ali Mate pritisnite Alt+F2, vnesite orca -s in pritisnite Enter

3. S tipko Tab ali Shift + Tab se premaknite na zavihke nastavitev, s puščico desno poiščite zavihek »Voice« (Glas), nato z večkratnim pritiskom tipke Tab izberite:
    - Speech system: Pritisnite preslednico, da razširite spustni seznam, s puščico dol izberite Speech Dispatcher, če še ni izbran, in pritisnite Enter
    - Speech synthesizer: Pritisnite preslednico, da razširite spustni seznam, s puščico dol izberite Laprdus in pritisnite Enter
    - Voice: Pritisnite preslednico, da razširite spustni seznam, s puščico dol izberite želeni glas (Zvonko, Josip, Vlado itd.; vsak glas je naveden za hrvaščino, srbščino, bosanščino in slovenščino, jezik, ki ga zahteva Orca, pa določa, v katerem jeziku bo govoril) in pritisnite Enter

4. S tipko Tab poiščite »Apply« (Uporabi) in zaprite nastavitve

### 2.4 Android

#### 1. korak: prenos datoteke APK

1. V napravi Android odprite spletni brskalnik
2. Obiščite: https://hrvojekatic.com/laprdus
3. Prenesite datoteko APK (datoteka s pripono `.apk`)

#### 2. korak: omogočanje nameščanja iz neznanih virov

To je potrebno samo, če aplikacijo nameščate zunaj trgovine Google Play.

**Za Android 8.0 in novejše različice:**

1. V napravi odprite Nastavitve
2. Pojdite na: Aplikacije > Posebni dostop za aplikacije > Nameščanje neznanih aplikacij
    - Ali: Nastavitve > Varnost > Nameščanje neznanih aplikacij
3. Poiščite spletni brskalnik, ki ga uporabljate (npr. Chrome)
4. Vklopite možnost »Dovoli iz tega vira«

**Za starejše različice Androida:**

1. Odprite Nastavitve
2. Pojdite na: Varnost
3. Vklopite možnost »Neznani viri«

#### 3. korak: nameščanje aplikacije

1. V napravi odprite upravitelja datotek
2. Poiščite preneseno datoteko APK (običajno v mapi »Download« ali »Prenosi«)
3. Tapnite datoteko APK
4. Ko se prikaže poziv, tapnite »Namesti«
5. Počakajte, da se nameščanje konča
6. Tapnite »Odpri«, da zaženete aplikacijo, ali »Končano«, da okno zaprete

#### 4. korak: nastavitev Laprdusa kot privzetega govornega sintetizatorja

1. V napravi odprite Nastavitve
2. Pojdite na: Funkcije za ljudi s posebnimi potrebami > Pretvorba besedila v govor (Text-to-speech)
    - V nekaterih napravah: Nastavitve > Sistem > Jeziki in vnos > Pretvorba besedila v govor
    - V napravah Samsung: Nastavitve > Splošno upravljanje > Jezik in vnos > Pretvorba besedila v govor
3. Tapnite »Prednostni mehanizem« ali ikono zobnika
4. Na seznamu izberite »Laprdus TTS«
5. Vrnite se na prejšnji zaslon in tapnite »Poslušaj primer«, da preizkusite govor

#### Opomba za uporabnike bralnika zaslona TalkBack

Če uporabljate TalkBack, so vsi vmesniki aplikacije Laprdus v celoti dostopni:

- Za premikanje uporabljajte običajne poteze bralnika TalkBack
- Dvakrat tapnite, da aktivirate gumbe in možnosti
- Povlecite z dvema prstoma, da se pomikate po seznamih

---

## 3. Uporaba

### 3.1 Osnovna uporaba

Po namestitvi je Laprdus samodejno na voljo vsem programom, ki v vašem sistemu uporabljajo sintezo govora. To so:

- Bralniki zaslona (NVDA, Orca, Pripovedovalec)
- Spletni brskalniki z možnostjo branja
- Programi za branje dokumentov
- Kateri koli program, ki uporablja Windows SAPI5 ali Android TTS

### 3.2 Ukazna vrstica (Linux in Windows)

Laprdus vključuje program za ukazno vrstico, s katerim lahko besedilo pretvorite v govor neposredno iz terminala.

#### Osnovni primeri

```bash
# Izgovorjava besedila
laprdus "Dobar dan!"

# Uporaba drugega glasu (brez -L govori glas svoj izvorni jezik, Vlado srbsko)
laprdus -v vlado "Zdravo svete!"

# Vsak glas v vsakem jeziku: Zvonko govori srbsko, Vlado hrvaško
laprdus -L sr "Zdravo svete!"
laprdus -v vlado -L hr "Dobar dan svima!"

# Nastavitev hitrosti govora
laprdus -r 1.5 "Brži govor"

# Shranjevanje v datoteko WAV
laprdus -o govor.wav "Tekst za snimanje"

# Branje iz datoteke
laprdus -i dokument.txt

# Branje s standardnega vhoda
echo "Tekst" | laprdus
```

#### Možnosti programa za ukazno vrstico

| Možnost | Opis |
|--------|------|
| `-v, --voice` | Izbira glasu (zvonko (privzeto), stojan, mirsad, josip, vlado, detence, baba, djedo ali pojoči glas: orguljas, dalmacija_klapa, trubac, harmonikas, sevdalija, sazlija, lika_velebit, solunac, sarajevo_bembasa, becarac) |
| `-L, --language` | Jezik: `hr`, `sr`, `bs` ali `sl`. Vsak glas govori vsak jezik; brez te možnosti glas govori svoj izvorni jezik |
| `-r, --speech-rate` | Hitrost govora (0.5-2.0, privzeto: 1.0; formantni glasovi 0.25-4.0) |
| `-p, --speech-pitch` | Višina glasu (0.5-2.0, privzeto: 1.0; formantni glasovi 0.25-4.0) |
| `-I, --inflection` | Raven intonacije, 0–100 (privzeto: 50; 0 je enoličen govor) |
| `-a, --acceleration` | Pospešek, množitelj hitrosti 0.5-3.0 (privzeto: 1.0) |
| `--head-size PERCENT` | Kakovost glasu: dolžina govornega trakta, 0–100 (privzeto: 50 = glas, kakršen je) |
| `--breathiness PERCENT` | Dih, 0–100: 0 napet glas, 100 šepet (privzeto: 50; formantni glasovi) |
| `--roughness PERCENT` | Hrapavost, 0–100: 0 povsem miren glas, nad 50 hripavost (privzeto: 50; formantni glasovi) |
| `--brightness PERCENT` | Barva tona, 0–100 (privzeto: 50; 0 zamolklo, z mehkejšima s in š, 100 ostro) |
| `-V, --speech-volume` | Glasnost (0.0-1.0, privzeto: 1.0) |
| `-B, --volume-boost` | Povečanje glasnosti: glasnejši govor prek omejevalnika vrhov, za hrupna okolja |
| `-d, --numbers-digits` | Beri števila po števkah |
| `-s, --spell` | Črkuj besedilo znak za znakom |
| `-S, --spelling-speed` | Hitrost črkovanja, 0–100; nižje vrednosti podaljšajo premore (privzeto: 50) |
| `-m, --spelling-mode` | Črkovanje: `sounds` (glasovi črk: b, c, d; privzeto) ali `names` (imena črk: be, ce, de) |
| `-c, --comma-pauses` | Trajanje premora za vejico v ms (privzeto: 100) |
| `-e, --period-pauses` | Trajanje premora za piko v ms (privzeto: 80) |
| `-x, --exclamationmark-pauses` | Trajanje premora za klicajem v ms (privzeto: 70) |
| `-q, --questionmark-pauses` | Trajanje premora za vprašajem v ms (privzeto: 60) |
| `-n, --newline-pauses` | Trajanje premora ob novi vrstici v ms (privzeto: 100) |
| `-D, --data-dir` | Mapa s podatki glasov |
| `-o, --output-file` | Shrani govor v datoteko WAV |
| `-i, --input-file` | Naloži besedilo iz datoteke |
| `-l, --list-voices` | Prikaži seznam razpoložljivih glasov z njihovim izvornim jezikom |
| `-w, --verbose` | Podrobnejši izpis (za diagnostiko) |
| `-h, --help` | Prikaži pomoč |

### 3.3 Nastavitve v NVDA

Ko Laprdus uporabljate z bralnikom zaslona NVDA, nastavitve glasu spremenite takole:

1. Odprite meni NVDA (tipka NVDA + N)
2. Izberite podmeni Izbire, nato Nastavitve > kategorija Govor
3. V pogovornem oknu lahko nastavite:
    - Jezik (hrvaščina, srbščina, bosanščina ali slovenščina)
    - Glas (seznam vsebuje vseh 18 glasov Laprdusa z imeni iz [6.1](#61-razpoložljivi-glasovi); privzeti je Zvonko)
    - Hitrost
    - Višino
    - Glasovno modulacijo (raven intonacije, glejte [4.6](#46-intonacija))
    - Glasnost
    - Dvig hitrosti ali Rate boost (vsak položaj drsnika hitrosti naredi trikrat hitrejši)

**Jezik je ločen od glasu.** Vsak glas Laprdusa govori vse štiri jezike, zato je seznam glasov za vsak jezik enak, jezik pa izberete v posebnem spustnem seznamu »Jezik«, kot pri drugih sintetizatorjih. Jezik določa pravila izgovarjave, naglasni slovar, imena črk pri črkovanju in besede za števila (*tisuća*, *milijun* v hrvaščini; *hiljada*, *milion* v srbščini in bosanščini; *tisoč*, *milijon* v slovenščini): Josip lahko tako govori srbsko, Stojan pa hrvaško ali slovensko. Nova namestitev se začne v jeziku NVDA, če je to hrvaščina, srbščina, bosanščina ali slovenščina, sicer v hrvaščini; jezik, ki ga izberete sami, si NVDA zapomni kot druge nastavitve. Jezik je tudi v krogu nastavitev sinteze (Control + tipka NVDA + puščice).

**Samodejna menjava jezika.** Če je v nastavitvah govora NVDA vklopljena možnost »Samodejna menjava jezika, (ko je to možno)«, Laprdus besedilo, označeno kot hrvaško, srbsko, bosansko ali slovensko, izgovori v tem jeziku in z istim glasom; besedilo v katerem koli drugem jeziku izgovori v jeziku, ki ste ga izbrali. Glas se pri tem nikoli ne spremeni.

### 3.4 Aplikacija za Android

Aplikacija Laprdus za Android ima zaslon z nastavitvami, kjer lahko:

1. Izberete glas
2. Izberete jezik (hrvaščina, srbščina, bosanščina ali slovenščina) za govor v sami aplikaciji; govor, ki ga zahtevajo druge aplikacije in TalkBack, je v jeziku, ki ga te zahtevajo, razen če je vklopljena možnost »Vsili izbrani jezik«
3. Nastavite hitrost govora
4. Nastavite višino glasu
5. Nastavite glasnost
6. Nastavite branje emodžijev
7. Nastavite način branja števil
8. Nastavite trajanje premorov in hitrost črkovanja
9. Izberete črkovanje z imeni ali glasovi črk

Do nastavitev pridete takole:

1. Odprite aplikacijo Laprdus v predalu z aplikacijami
2. Tapnite gumb »Nastavitve Laprdusa«
3. Po želji spremenite nastavitve (spremembe se shranijo samodejno)

---

## 4. Nastavitve

### 4.1 Hitrost govora

Hitrost govora določa, kako hitro Laprdus izgovarja besedilo. Razpon je od 0,5 (polovična hitrost) do 2,0 (dvojna hitrost), pri čemer je 1,0 običajna hitrost.

- **0,5** – Zelo počasi, koristno za začetnike ali kadar želite natančno slišati vsako besedo
- **1,0** – Običajna hitrost, primerna za večino okoliščin
- **1,5** – Zmerno hitro, za izkušene uporabnike
- **2,0** – Hitro, za napredne uporabnike

Opomba: V dodatku za NVDA možnost »Dvig hitrosti« ali »Rate boost« naredi vsak položaj drsnika hitrosti trikrat hitrejši, kot pri drugih sintetizatorjih; Josip, Vlado in glasovi, izpeljani iz njih, se ustavijo pri štirikratni hitrosti.

**Hitrost v Laprdusu in hitrost bralnika zaslona.** Hitrost iz nastavitev Laprdusa je običajna hitrost glasu, bralnik zaslona ali aplikacija (TalkBack, VoiceOver, NVDA, programi, ki uporabljajo SAPI5, Speech Dispatcher) pa svojo hitrost določa glede nanjo. Dokler bralnik zaslona govori s svojo običajno hitrostjo, Laprdus govori natanko s hitrostjo iz svojih nastavitev; dvakrat hitrejši bralnik zaslona pomeni dvakrat večjo hitrost. Zato govor pospešujeta in upočasnjujeta drsnik hitrosti v Laprdusu in drsnik v bralniku zaslona. Stikalo »Vsili hitrost govora v Laprdusu« (v Konfiguratorju za Windows »Vsili hitrost govora Laprdusa v vseh aplikacijah«) prezre hitrost, ki jo zahteva bralnik zaslona ali aplikacija, zato Laprdus govori samo s hitrostjo iz svojih nastavitev. [Pospešek](#46-intonacija) to hitrost na koncu še pomnoži.

### 4.2 Višina glasu

Višina glasu omogoča nastavljanje osnovne frekvence glasu. Razpon je od 0,5 (nižji glas) do 2,0 (višji glas), pri čemer je 1,0 naravna višina glasu. Tako kot hitrost (glejte [4.1](#41-hitrost-govora)) je tudi višina iz nastavitev Laprdusa običajna višina glasu, ki jo bralnik zaslona zvišuje ali znižuje, razen kadar je vklopljeno stikalo »Vsili višino glasu v Laprdusu«.

- **0,5** – Nižji glas
- **1,0** – Naravna višina
- **2,0** – Višji glas

Opomba: Nastavljanje višine glasu ne vpliva na značaj glasu (ni »učinka risanke«).

### 4.3 Glasnost

Glasnost določa, kako glasen bo govor. Razpon je od 0,0 (tišina) do 1,0 (največja glasnost).

- **0,0** – Tišina (ni zvoka)
- **0,5** – Zmerna glasnost
- **1,0** – Največja glasnost

#### Povečanje glasnosti

Stikalo **Povečaj glasnost govora** v Konfiguratorju Laprdus naredi govor glasnejši, kot ga dopušča drsnik glasnosti pri 100 %. Namenjeno je hrupnim okoljem (promet, ulica) in telefonski slušalki, ko je zvočnik izklopljen: na primer, da slišite, koga kličete, medtem ko se klic vzpostavlja, ali da med klicem najdete gumb za vklop zvočnika. Govor gre skozi močno ojačanje in omejevalnik vrhov, ki ojačanje spreminja le med zlogi, kot pri povečanju glasnosti govora v telefonih: tihi deli postanejo glasnejši za približno 8 dB, glasni za približno 3 dB, znotraj zloga pa se nič ne reže in ne stiska, zato zvok ostane čist. Soglasniki ohranijo razdaljo do samoglasnikov, visoki toni nad 4 kHz pa so utišani za 3 dB, ker ima zvočnik telefona pri največji glasnosti tam svoj vrh. Drsnik glasnosti v NVDA še vedno spreminja ojačani govor. Stikalo je privzeto izklopljeno in velja za vse glasove.

### 4.4 Premori

Laprdus za ločili samodejno vstavi premore, da je govor bolj naraven. Nastavite lahko trajanje premorov za:

- **Pika** – Premor za piko (privzeto: 100 ms v uporabniških nastavitvah, 80 ms v programu za ukazno vrstico)
- **Klicaj** – Premor za klicajem (privzeto: 100 ms v uporabniških nastavitvah, 70 ms v programu za ukazno vrstico)
- **Vprašaj** – Premor za vprašajem (privzeto: 100 ms v uporabniških nastavitvah, 60 ms v programu za ukazno vrstico)
- **Vejica** – Kratek premor znotraj povedi (privzeto: 100 ms)
- **Nova vrstica** – Premor ob prelomu vrstice (privzeto: 100 ms)

Vrednosti so izražene v milisekundah (ms). Razpon je od 0 do 2000 ms.

**Opomba:** Uporabniški konfigurator (Windows/Android) uporablja enotno nastavitev 100 ms za vse znake na koncu povedi (pika, klicaj, vprašaj), program za ukazno vrstico pa omogoča ločeno nastavitev vsakega znaka.

V istem razdelku nastavitev je tudi **hitrost črkovanja** (0–100 %, privzeto 50 %): nižja vrednost podaljša premor za vsakim črkovanim ali vtipkanim znakom, sam znak pa se vedno izgovori z običajno hitrostjo govora. Pri 100 % ni dodatnega premora; pri 50 % se doda približno 41 % trajanja izgovarjave znaka, pri 20 % približno 74 %, pri 0 % pa celotno trajanje. Ta tišina se prišteje osnovnemu premoru črkovanja (privzeto 200 ms, `pauses.spelling` v `settings.json`). Premor sledi tudi zadnjemu znaku, zato je razmik enak, ne glede na to, ali bralnik zaslona pošlje celo besedo ali znak za znakom.

V NVDA ima prednost možnost »Delayed descriptions for characters on cursor movement« (zakasnjeni opisi znakov ob premiku kazalke) v nastavitvah govora: ko je vklopljena, NVDA po črki, na katero ste se premaknili, počaka eno sekundo in nato izgovori njen opis (npr. »Alan« za A). Laprdus takrat za to črko ne doda premora hitrosti črkovanja, zato opis pride natanko po sekundi NVDA, ne glede na hitrost črkovanja. Črkovanje besed in tipkanje ohranita ritem hitrosti črkovanja.

### 4.5 Način branja števil

Laprdus lahko števila bere na dva načina:

1. **Z besedami** (privzeto) – Števila se berejo kot cele besede
    - Primer: »123« se prebere kot »sto dvadeset tri«

2. **S števkami** – Števila se berejo števko za števko
    - Primer: »123« se prebere kot »jedan dva tri«

Branje z besedami podpira števila do centilijona (10^303) in uporablja pravilne gramatične oblike izbranega jezika.

### 4.6 Intonacija

Intonacija je naravno spreminjanje višine glasu, ki sledi ločilom:

- Pika na koncu povedi povzroči padec višine glasu
- Vprašaj povzroči dvig višine glasu na koncu
- Klicaj doda poudarek
- Vejica povzroči rahel dvig višine glasu

Obseg intonacije določa drsnik, stikala za vklop in izklop iz različice 1.0 pa ni več: enoličen govor dobite, če drsnik nastavite na 0 %. Za vse glasove veljata dve nastavitvi:

- **Raven intonacije** (0–100 %) določa obseg vseh premikov višine: 0 % je enoličen govor, 50 % naravna melodija, 100 % pa podvoji vsak premik.
- **Pospešek** (0,5–3) pomnoži hitrost govora, tako da drsnik hitrosti v bralniku zaslona ali sistemu doseže večjo (ali manjšo) največjo hitrost. Josip, Vlado in glasovi, izpeljani iz njih, se ustavijo pri štirikratni hitrosti; Zvonko, Stojan in Mirsad gredo do osemkratne.

V NVDA raven intonacije določa drsnik »Glasovna modulacija« v nastavitvah govora: 50 je naravna melodija, 0 enoličen govor, pri 100 pa je vsak premik dvojen. Intonacija iz Konfiguratorja se takrat ne uporablja. Pospešek se v NVDA ne uporablja; večje hitrosti dosežete z možnostjo »Dvig hitrosti« (Rate boost) v NVDA.

### 4.7 Črkovanje

Kadar bralnik zaslona črkuje besedilo ali izgovarja znake med tipkanjem, Laprdus vsako črko prebere na enega od dveh načinov:

- **Z imeni črk**: *a, be, ce, če, će, de, dže, đe, e, ef, ge, ha, i, je, ka, el, elj, em, en, enj, o, pe, er, es, eš, te, u, ve, ze, že*; tuje črke *ku, duplo ve, iks, ipsilon*. To so imena iz hrvaškega pravopisa, veljajo pa tudi v bosanščini. V srbščini vsak glas sledi odločitvi Odbora za standardizacijo srbskega jezika: *lje, nje* in *ša* namesto *elj, enj* in *eš*, druge črke enako, cirilične in latinične (*Љ* je *lje*, *Ш* je *ša*). V slovenščini veljajo imena iz Slovenskega pravopisa: enaka imena za njegovih 25 črk, *dvojni ve* za W ter *mehki če* in *mehki dže* za Ć in Đ. Imena določa izbrani jezik, ne glas: Josip v srbščini reče *lje*, Stojan v hrvaščini *elj*.
- **Z glasovi črk** (privzeto): namesto imena se izgovori glas, ki ga črka označuje, kot pri otroku, ki se uči brati, in kot jih črkujejo nekateri sintetizatorji: soglasnik, za njim pa kratek nevtralni samoglasnik (*bə, cə, čə, fə, sə, mə*), samoglasniki pa kot samoglasniki. Tuje črke *q, w, x* in *y* niso črke naše abecede, zato se tudi tukaj berejo z imeni (*ku, duplo ve, iks, ipsilon*). Angleška imena in angleška izgovarjava se ne uporabljajo.

Način črkovanja določa stikalo v naprednih nastavitvah, ki je pri novi namestitvi vklopljeno (Windows: skupina Možnosti v Konfiguratorju, »Pri črkovanju beri glasove črk (b, c, d) namesto njihovih imen (be, ce, de)«; Android: razdelek Napredno, »Črkovanje«; Apple: razdelek Napredno, »Črkuj z glasovi črk«; NVDA: Konfigurator; program za ukazno vrstico: `-m`). Velja za črkovanje, tipkanje in črko, ki v sporočilu stoji sama; besede se še naprej izgovarjajo običajno. Števila, ločila in simboli se v obeh načinih berejo z imenom iz slovarja črkovanja. Hitrost črkovanja je opisana v [4.4](#44-premori).

**Velike črke.** Če je bralnik zaslona nastavljen tako, da velike črke napove z besedo (»veliko N« v NVDA, »veliko slovo N« v bralniku TalkBack, »veliko početno slovo N« v bralniku VoiceOver, »cap N« v angleščini), Laprdus besedo izgovori običajno, črko pa črkuje kot vsako drugo: z imenom ali glasom, glede na izbrani način, in s hitrostjo črkovanja. NVDA velike črke najpogosteje označi s povišanim glasom: nastavitev »Sprememba višine glasu pri velikih črkah v odstotkih« v nastavitvah govora (privzeto 30) zviša višino črkovane velike črke, naslednji znak ali beseda pa se vrne na višino drsnika. To velja tudi, če je vklopljeno vsiljevanje višine glasu Laprdusa: sprememba se takrat računa od sredine drsnika višine v NVDA.

**Črka sama v sporočilu.** Bralnik zaslona pogosto sporoči znak znotraj sporočila: »Obrisano B«, »B odabrano«, »veliko slovo B«, kadar dele napovedi pošlje ločeno. Soglasnik, ki tako stoji sam, kot samostojna beseda, Laprdus prebere na izbrani način kot črkovano črko: z glasom (*bə*), če so vklopljeni glasovi črk, z imenom (*be*), če niso. Enako velja za takšno črko v navadnem besedilu (*Plan B*, *tipka F 5*). Predlogi *s* in *k* (v slovenščini tudi *v, z, h*) pred besedo ostanejo predlogi, kratice (*USB*, *s.a.r.s.*) in črke v zloženkah (*S-klasa*) se berejo z imeni, tuje črke *q, w, x, y* pa vedno z imeni.

**Slovar črkovanja in glasovi črk.** Vaši vnosi v slovarju črkovanja (razdelek [5.4](#54-slovar-črkovanja-spellingjson)) veljajo v obeh načinih, z eno izjemo: kadar se črke črkujejo z glasovi, je vnos, ki je samo ime črke (*be*, *jot*, *lje*), prav tako ime in se umakne glasu, vnos, ki pove kaj drugega (na primer *B* → *bum*), pa se izgovori, kot ste ga napisali. Tako stari vnosi, s katerimi ste popravljali imena, ne izklopijo glasov.

### 4.8 Konfigurator Laprdus (Windows)

V sistemu Windows lahko dodatne nastavitve prilagodite s Konfiguratorjem Laprdus:

1. Odprite začetni meni sistema Windows, v programski mapi Laprdus poiščite Konfigurator Laprdus in ga odprite s tipko Enter.
2. Če imate Laprdus nameščen kot dodatek za NVDA, odprite meni NVDA > podmeni Laprdus > Konfigurator Laprdus

Konfigurator je v hrvaščini, srbščini, slovenščini in angleščini. Sledi jeziku prikaza sistema Windows, če pa ga odprete iz menija NVDA, sledi jeziku NVDA. Pri bosanskem jeziku prikaza je besedilo hrvaško, pri vsakem drugem pa angleško. Namestitveni program prav tako ponuja te štiri jezike, dodatek za NVDA pa sledi jeziku NVDA.

Konfigurator omogoča nastavljanje:

- Hitrosti, višine in glasnosti govora (hitrost in višino drsniki NVDA pospešujejo, upočasnjujejo, zvišujejo in znižujejo, razen kadar je vklopljeno vsiljevanje vrednosti Laprdusa; glejte [4.1](#41-hitrost-govora))
- Intonacije in pospeška, za vse glasove: intonacija od 0 % (enolično) prek 50 % (privzeto, naravni premiki) do 100 % (dvakrat večji premiki), v programih SAPI5 (v NVDA jo določa drsnik »Glasovna modulacija«); pospešek pomnoži hitrost govora v programih SAPI5 (0,5x–3,0x, privzeto 1,0x), v NVDA pa se ne uporablja, ker večje hitrosti da možnost »Dvig hitrosti«. Drsnika hitrosti in višine gresta pri formantnih glasovih od 0,25x do 4,0x.
- Kakovosti glasu: velikosti glave, diha, hrapavosti in svetlosti, z gumbom »Kakovost glasu...« pod povečanjem glasnosti (glejte [4.9](#49-kakovost-glasu))
- Načina branja števil
- Trajanja premorov in hitrosti črkovanja
- Črkovanja z glasovi črk namesto z imeni
- Prilagajanja uporabniških slovarjev

Nastavitve se shranijo v uporabniško mapo `%APPDATA%\Laprdus`, uporabljata pa jih SAPI5 in dodatek za NVDA.

### 4.9 Kakovost glasu

Štiri nastavitve kakovosti glasu sledijo modelu glasovnih parametrov klasičnih formantnih sintetizatorjev: velikost glave, dih, hrapavost in svetlost. V NVDA so to NVDA-jeve lastne nastavitve glasu: drsniki Velikost glave, Dih, Hrapavost in Svetlost v NVDA-jevih nastavitvah glasu, za glasnostjo, in v krogu nastavitev sinteze (Control + tipka NVDA + puščice), vsak od 0 do 100, pri čemer je 50 glas, kakršen je; NVDA si jih zapomni kot druge svoje nastavitve. Konfigurator Laprdus ima iste štiri za gumbom »Kakovost glasu...«, vendar te vrednosti veljajo le za SAPI5 (v datoteki `settings.json` so zapisane kot `speech.head_size`, `speech.breathiness`, `speech.roughness` in `speech.brightness`), tako kot njegove nastavitve intonacije.

- **Velikost glave** – dolžina govornega trakta: 0 je za četrtino daljši (nižji formanti, polnejši glas), 100 za četrtino krajši (višji formanti, drobnejši glas). Deluje na vse glasove.
- **Dih** – dih v glasu: 0 je napet, čist glas brez diha, 50 glas, kakršen je; nad 50 je v glasu vse več diha, 100 pa je šepet, samo dih brez zvenečnosti. Deluje na formantne glasove (Zvonko, Stojan, Mirsad in pojoči glasovi).
- **Hrapavost** – nepravilnosti višine in glasnosti od nihaja do nihaja (jitter in shimmer): 0 jih ni, pa tudi višina ne niha (povsem miren glas), 50 je glas, kakršen je; nad 50 vsak drugi nihaj glasu pride prej in šibkejši, hripavost, ki narašča do 100. Deluje na formantne glasove.
- **Svetlost** – barva tona: 0 je temno in zamolklo, 100 ostro in svetlo. Deluje na vse glasove prek nagiba celotnega zvoka okoli 1,5 kHz (na formantne tudi prek vira glasu), zato nižje vrednosti omilijo tudi glasove, kot sta s in š.

---

## 5. Slovarji

Slovarji omogočajo prilagajanje načina, kako Laprdus izgovarja določene besede, kratice, simbole in emodžije. S slovarji lahko popravite izgovarjavo tujih besed, dodate izgovarjavo kratic ali prilagodite branje posebnih znakov.

### 5.1 Vrste slovarjev

Laprdus uporablja tri vrste slovarjev, vsak s svojim namenom, poleg njih pa še naglasni slovar formantnih glasov:

| Vrsta slovarja | Datoteka | Namen |
|----------------|----------|---------|
| **Glavni slovar** | `user.json` | Zamenjava besed in fraz s prilagojeno izgovarjavo |
| **Slovar črkovanja** | `spelling.json` | Izgovarjava posameznih znakov (črk, števk, simbolov) |
| **Slovar emodžijev** | `emoji.json` | Pretvorba emodžijev v besedilni opis |
| **Naglasni slovar** | `accents.json` | Naglas, dolžina in ton besed za formantne glasove (glejte [5.11](#511-naglasni-slovar-accentsjson)) |

### 5.2 Kje so datoteke slovarjev

Mesto datotek slovarjev je odvisno od platforme, ki jo uporabljate:

#### Windows (SAPI5 in NVDA)

Vse datoteke slovarjev so v uporabniški mapi Laprdusa:

```
%APPDATA%\Laprdus\
```

Celotna pot je običajno: `C:\Users\VašeKorisničkoIme\AppData\Roaming\Laprdus\`

**Kako odprete to mapo:**

1. Pritisnite tipki Windows+R, da odprete pogovorno okno »Zaženi«
2. Vnesite: `%APPDATA%\Laprdus`
3. Pritisnite Enter

Datoteke v tej mapi:

| Datoteka | Opis |
|----------|------|
| `settings.json` | Glavne nastavitve Laprdusa |
| `user.json` | Vaš uporabniški slovar izgovarjave |
| `spelling.json` | Slovar črkovanja (izgovarjava znakov) |
| `emoji.json` | Slovar emodžijev |
| `accents.json` | Naglasni slovar formantnih glasov |

**Opomba:** Te datoteke se ustvarijo samodejno, ko prvič dodate vnos s Konfiguratorjem Laprdus. Ustvarite jih lahko tudi ročno.

#### Linux

Datoteke slovarjev so v mapi:

```
~/.config/Laprdus/
```

Uporabite ista imena datotek kot v sistemu Windows (`user.json`, `spelling.json`, `emoji.json`, `accents.json`).

#### Android

V Androidu se slovarji urejajo izključno prek vmesnika aplikacije Laprdus. Datoteke so shranjene v notranjem pomnilniku aplikacije in uporabniku niso neposredno dostopne.

### 5.3 Glavni slovar (user.json)

Glavni slovar je namenjen zamenjavi besed ali fraz s prilagojeno izgovarjavo. Uporaben je za:

- **Tuje besede in blagovne znamke** – npr. »Facebook« naj se izgovori kot »Fejzbuk«
- **Kratice** – npr. »TV« naj se izgovori kot »Te Ve«
- **Tehnične izraze** – npr. »ChatGPT« naj se izgovori kot »ČetDžipiti«
- **Imena z neobičajno izgovarjavo** – npr. »Sean« naj se izgovori kot »Šon«

#### Oblika datoteke

Datoteka `user.json` uporablja obliko JSON. Tukaj je primer:

```json
{
    "version": "1.0",
    "entries": [
        {
            "grapheme": "Facebook",
            "phoneme": "Fejzbuk",
            "caseSensitive": false,
            "wholeWord": true,
            "comment": "Družbeno omrežje"
        },
        {
            "grapheme": "TV",
            "phoneme": "Te Ve",
            "caseSensitive": false,
            "wholeWord": true,
            "comment": "Kratica za televizijo"
        },
        {
            "grapheme": "ChatGPT",
            "phoneme": "ČetDžipiti",
            "caseSensitive": false,
            "wholeWord": true,
            "comment": "Klepetalni robot z umetno inteligenco"
        }
    ]
}
```

#### Opis polj

| Polje | Obvezno | Opis | Privzeta vrednost |
|-------|----------|------|-------------------|
| `grapheme` | Da | Izvirno besedilo, ki se zamenja | - |
| `phoneme` | Da | Nadomestna izgovarjava | - |
| `caseSensitive` | Ne | Ali se razlikujejo velike in male črke | `false` |
| `wholeWord` | Ne | Zamenja samo celo besedo | `true` |
| `comment` | Ne | Komentar za vas (ne vpliva na izgovarjavo) | - |

#### Razlaga možnosti

**caseSensitive (razlikovanje velikih in malih črk):**

- `false` (privzeto): »Facebook«, »facebook«, »FACEBOOK« se zamenjajo vsi
- `true`: Zamenja se samo natančno ujemanje

**wholeWord (samo cela beseda):**

- `true` (privzeto): Zamenja samo, kadar je besedilo cela beseda
    - »TV« v »TV program« se zamenja
    - »TV« v »Latvija« se ne zamenja (ker je del daljše besede)
- `false`: Zamenja tudi znotraj drugih besed
    - Uporabno za pripone in predpone

### 5.4 Slovar črkovanja (spelling.json)

Slovar črkovanja določa, kako se izgovarjajo posamezni znaki, kadar bralnik zaslona črkuje besedilo (branje znak za znakom). Uporaben je za:

- **Števke** – npr. »5« naj se izgovori kot »pet«
- **Ločila** – npr. ».« naj se izgovori kot »točka«
- **Posebne znake** – npr. »@« naj se izgovori kot »at«
- **Črke abecede** – če želite, da se neka črka imenuje drugače, kot jo imenuje Laprdus sam, npr. »W« kot »dvostruko ve«

Črke abecede Laprdus poimenuje sam, v izbranem jeziku (glejte [4.7](#47-črkovanje)), zato jih vgrajeni slovar ne vsebuje; vaš vnos za črko ima prednost pred vgrajenim imenom. Kadar je vklopljeno črkovanje z glasovi, se vnos, ki je samo ime črke, prezre, vsak drug vnos pa velja.

#### Oblika datoteke

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

#### Vgrajene izgovarjave znakov

Laprdus ima vgrajen slovar črkovanja za števke, ločila in simbole (imena so hrvaška). Črke se imenujejo takole:

**Črke abecede** (vgrajeno v Laprdus, ne v slovar):

| Črka | Hrvaščina in bosanščina | Srbščina | Črka | Hrvaščina in bosanščina | Srbščina |
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

Cirilične črke se imenujejo kot njihove latinične ustreznice, velike in male črke pa enako.

**Števke:**

| Znak | Izgovarjava |
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

**Običajna ločila:**

| Znak | Izgovarjava |
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

**Posebni znaki:**

| Znak | Izgovarjava |
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

### 5.5 Slovar emodžijev (emoji.json)

Slovar emodžijev pretvarja emodžije v govorjeno besedilo.

#### Vklop branja emodžijev

**Windows (Konfigurator Laprdus):**

1. Odprite Konfigurator Laprdus (začetni meni > Laprdus > Konfigurator Laprdus)
2. S tipko Tab poiščite možnost »Beri emodžije«
3. Označite potrditveno polje
4. S tipko Tab izberite »V redu« in pritisnite Enter

**NVDA:**

1. Odprite meni NVDA (tipka NVDA + N) > podmeni Laprdus > Konfigurator Laprdus
2. S tipko Tab poiščite možnost »Beri emodžije«
3. Označite potrditveno polje
4. S tipko Tab izberite »V redu« in pritisnite Enter

**Android:**

1. Odprite aplikacijo Laprdus
2. Tapnite »Nastavitve Laprdusa«
3. Poiščite stikalo »Omogoči branje emodžijev« in ga vklopite

#### Oblika datoteke

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

#### Vgrajeni emodžiji

Laprdus ima obsežen vgrajen slovar emodžijev z več kot 1100 emodžiji s hrvaškimi opisi. Nekaj primerov:

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

### 5.6 Urejanje slovarjev v sistemu Windows (Konfigurator Laprdus)

Konfigurator Laprdus ponuja grafični vmesnik za preprosto upravljanje slovarjev. To je **priporočeni način** za uporabnike, ki imajo raje grafični vmesnik.

**Kako odprete Konfigurator:**

- **Iz začetnega menija:** začetni meni > Laprdus > Konfigurator Laprdus
- **Iz menija NVDA:** tipka NVDA + N > podmeni Laprdus > Konfigurator Laprdus

**Urejanje slovarjev:**

1. V Konfiguratorju Laprdus s tipko Tab ali Shift+Tab poiščite gumb **»Slovarji...«** in pritisnite Enter
2. Odpre se okno Upravljanje slovarjev
3. Na vrhu okna je spustni seznam za izbiro vrste slovarja, ki ga želite prilagoditi:
    - **Glavni uporabniški slovar** – za zamenjavo besed in fraz
    - **Slovar črkovanja** – za izgovarjavo posameznih znakov
    - **Slovar emodžijev** – za pretvorbo emodžijev v besedilo
    - **Naglasni slovar** – za naglas, dolžino in ton besede, za vse njene oblike hkrati (zapis v [5.11](#511-naglasni-slovar-accentsjson))
4. S puščico dol izberite želeno vrsto slovarja

**Dodajanje novega vnosa:**

1. Izberite gumb **»Dodaj«**
2. Odpre se pogovorno okno za vnos:
    - **Izvirno besedilo:** Vnesite besedilo, ki ga želite zamenjati
    - **Nadomestno besedilo:** Vnesite, kako naj se besedilo izgovori
    - **Razlikuj velike in male črke:** Označite, če je pomembno natančno ujemanje
    - **Samo cela beseda:** Označite, če ne želite zamenjave znotraj drugih besed
    - **Komentar:** Neobvezen opis vnosa
3. Pritisnite Enter ali s tipko Tab poiščite gumb **»V redu«** in potrdite s tipko Enter, da shranite vnos

**Urejanje obstoječega vnosa:**

1. Na seznamu izberite vnos s puščico dol ali gor
2. S tipko Tab poiščite gumb **»Uredi«**, uporabniki miške pa lahko vnos tudi dvokliknejo
3. Spremenite želene vrednosti
4. Pritisnite Enter ali s tipko Tab poiščite gumb **»V redu«** in pritisnite Enter

**Brisanje vnosa:**

1. Na seznamu izberite vnos s puščico gor ali dol
2. S tipko Tab poiščite gumb **»Izbriši«** in pritisnite Enter

**Podvajanje vnosa:**

1. Izberite vnos, ki ga želite podvojiti
2. S tipko Tab poiščite gumb **»Podvoji«** in pritisnite Enter
3. Nov vnos s kopiranimi vrednostmi se doda na seznam
4. Po potrebi uredite podvojeni vnos

**Vnos v naglasni slovar:**

Kadar je izbran naglasni slovar, ima pogovorno okno za vnos druga polja:

- **Vnos:** beseda, osnova, seznam oblik ali glagol z naglasnimi znamenji, v zapisu iz [5.11](#511-naglasni-slovar-accentsjson), npr. `kontr'o:l*` ali `ur'e:d=i<p`
- **Vrsta vnosa:** *Beseda* (beseda, osnova z `*` ali oblike z `|`) ali *Glagol* (celotna osnova z znakom `=` in razredom ali koren z dolgim *ije*)
- **Jezik:** *Vsi jeziki* ali hrvaščina, srbščina, bosanščina oz. slovenščina, če vnos velja samo za en jezik
- **Komentar:** neobvezna opomba, npr. oblike, ki jih vnos zajema

Pod polji je kratek opomnik o oznakah. Vnos se preverja po istih pravilih, po katerih ga bere Laprdus: vnos brez naglasnega znamenja, z dvema naglasoma ali z neznanim razredom glagola Konfigurator zavrne in pove, kaj je narobe, zato v datoteko ne more priti vnos, ki bi bil ob nalaganju preskočen. Podvajanje ne spremeni besedila vnosa.

### 5.7 Urejanje slovarjev v Androidu

Aplikacija za Android ima vgrajen vmesnik za upravljanje slovarjev.

**1. korak: odprite slovarje**

1. Odprite aplikacijo Laprdus v predalu z aplikacijami
2. Tapnite gumb **»Nastavitve Laprdusa«**
3. Tapnite gumb **»Upravljanje slovarjev«**

**2. korak: izberite vrsto slovarja**

Na vrhu zaslona je spustni seznam za izbiro vrste slovarja:

- **Glavni slovar** – za zamenjavo besed in fraz
- **Slovar črkovanja** – za izgovarjavo znakov
- **Slovar emodžijev** – za pretvorbo emodžijev
- **Naglasni slovar** – za naglas besed (zapis v [5.11](#511-naglasni-slovar-accentsjson))

Tapnite spustni seznam in izberite želeno vrsto.

**3. korak: dodajanje novega vnosa**

1. Tapnite gumb **»+«** (plus) v spodnjem desnem kotu zaslona
2. Odpre se zaslon za vnos:
    - **Izvirno besedilo:** Vnesite besedo ali frazo, ki jo želite zamenjati
    - **Nadomestna izgovarjava:** Vnesite, kako naj se izgovori
    - **Razlikuj velike in male črke:** Vklopite, če je pomembno natančno ujemanje
    - **Ujemanje samo s celo besedo:** Vklopite, če ne želite zamenjave znotraj drugih besed
    - **Komentar (neobvezno):** Opis (npr. »Družbeno omrežje«)
3. Tapnite gumb **»Shrani«** na dnu zaslona

Za naglasni slovar ima zaslon za vnos polja **Vnos** (beseda, osnova, oblike ali glagol z naglasom, npr. `kontr'o:l*`), **Vrsta vnosa** (beseda ali glagol), **Jezik** (vsi jeziki ali eden od štirih) in **Komentar**, pod njimi pa je opomnik o oznakah. Neveljaven vnos aplikacija zavrne in razlog izpiše pod poljem.

**4. korak: urejanje obstoječega vnosa**

1. Poiščite vnos na seznamu
2. Tapnite vnos, da odprete zaslon za urejanje
3. Spremenite želene vrednosti
4. Tapnite **»Shrani«**

**5. korak: brisanje vnosa**

1. Poiščite vnos na seznamu
2. Povlecite vnos v levo, da se prikaže možnost brisanja
3. Tapnite ikono koša
4. Potrdite brisanje

**Opomba za uporabnike bralnika TalkBack:**

- Za premikanje uporabljajte običajne poteze bralnika TalkBack
- Dvakrat tapnite, da aktivirate gumb
- Za brisanje: dvakrat tapnite in pridržite vnos, nato izberite »Izbriši«

### 5.8 Ročno urejanje datotek JSON (napredno)

Ta način je namenjen **naprednim uporabnikom**, ki želijo datoteke JSON slovarjev urejati neposredno. Večini uporabnikov priporočamo Konfigurator Laprdus (Windows) ali vmesnik aplikacije za Android.

**Prednosti ročnega urejanja:**

- Hitrejše množično dodajanje in urejanje velikega števila vnosov
- Možnost deljenja slovarjev z drugimi uporabniki
- Ustvarjanje varnostnih kopij slovarjev

**1. korak: odprite mapo s slovarji**

**Windows:**

1. Pritisnite tipki Windows+R
2. Vnesite: `%APPDATA%\Laprdus`
3. Pritisnite Enter

**Linux:**

1. Odprite upravitelja datotek
2. Odprite skrito mapo `.config` v domači mapi
3. Odprite mapo `Laprdus`
    - Ali v terminalu: `cd ~/.config/Laprdus`

**2. korak: odprite ali ustvarite datoteko**

- Če datoteka že obstaja, jo odprite v urejevalniku besedila (Windows: Beležnica, Linux: gedit, nano, vim)
- Če datoteka ne obstaja, ustvarite novo besedilno datoteko z ustreznim imenom:
    - `user.json` za glavni slovar
    - `spelling.json` za slovar črkovanja
    - `emoji.json` za slovar emodžijev
    - `accents.json` za naglasni slovar formantnih glasov (zapis v [5.11](#511-naglasni-slovar-accentsjson))

**3. korak: uredite vsebino**

Uporabite obliko JSON, opisano v prejšnjih razdelkih (5.3, 5.4, 5.5). Primer za `user.json`:

```json
{
    "version": "1.0",
    "entries": [
        {
            "grapheme": "YouTube",
            "phoneme": "Jutjub",
            "caseSensitive": false,
            "wholeWord": true,
            "comment": "Videoplatforma"
        },
        {
            "grapheme": "Google",
            "phoneme": "Gugl",
            "caseSensitive": false,
            "wholeWord": true,
            "comment": "Spletni iskalnik"
        }
    ]
}
```

**POMEMBNO – pravila sintakse JSON:**

- Vsi ključi in besedilne vrednosti morajo biti v dvojnih narekovajih `"`
- Vejice `,` med vnosi, VENDAR NE za zadnjim vnosom
- Zaviti oklepaji `{}` za vsak vnos
- Oglati oklepaji `[]` za seznam vnosov

**4. korak: shranite datoteko**

- **Obvezno** shranite datoteko s **kodiranjem UTF-8** (za podporo slovenskih znakov)
- **Beležnica v sistemu Windows:** Datoteka > Shrani kot > Kodiranje: UTF-8
- **Linux:** Večina urejevalnikov samodejno uporablja UTF-8

**5. korak: znova zaženite sintetizator**

- **Windows SAPI5:** Zaprite in znova odprite aplikacijo, ki uporablja Laprdus
- **NVDA:** Pritisnite tipko NVDA + Q in znova zaženite NVDA ali preklopite na drugo sintezo in se vrnite na Laprdus
- **Linux:** Znova zaženite aplikacijo ali Speech Dispatcher: `systemctl --user restart speech-dispatcher`

**Opomba:** Če uredite slovar, medtem ko je aplikacija za Android zagnana, spremembe ne bodo vidne, dokler ne osvežite seznama ali znova odprete aplikacije.

### 5.9 Primeri praktične uporabe

#### 1. primer: tuja podjetja in blagovne znamke

Številna tuja podjetja in blagovne znamke imajo izgovarjavo, ki se razlikuje od zapisa:

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

#### 2. primer: kratice

Kratice, ki naj se berejo črko za črko:

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
    "comment": "Kratica za Hrvaško"
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

#### 3. primer: tehnični izrazi

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

#### 4. primer: imena z neobičajno izgovarjavo

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

#### 5. primer: zamenjava znotraj besede (wholeWord: false)

Če želite zamenjati pripono ali predpono, nastavite `wholeWord` na `false`:

```json
{
    "grapheme": "tion",
    "phoneme": "šen",
    "caseSensitive": false,
    "wholeWord": false,
    "comment": "Angleška pripona -tion"
}
```

S tem vnosom bodo besede, kot je »information«, vsebovale izgovarjavo »šen« namesto »tion«.

### 5.10 Odpravljanje težav s slovarji

#### Težava: spremembe v slovarju se ne uveljavijo

**Možni vzroki in rešitve:**

1. **Uporaba uporabniških slovarjev je izklopljena**
    - Vklopite možnost **»Uporabi uporabniške slovarje«** v Konfiguratorju Laprdus (Windows) ali v nastavitvah Laprdusa (Android)

2. **Sintaktična napaka v datoteki JSON**
    - Preverite, ali so vsi narekovaji pravilno zaprti
    - Preverite, ali so vejice na pravih mestih
    - Sintakso preverite s spletnim preverjevalnikom JSON

3. **Datoteka ni shranjena s kodiranjem UTF-8**
    - V Beležnici: Datoteka > Shrani kot > Kodiranje: UTF-8
    - V drugih urejevalnikih: preverite nastavitve kodiranja

4. **Sintetizator ni bil znova zagnan**
    - Zaprite in znova odprite aplikacijo
    - Za NVDA: Insert+Q > Ponovni zagon

5. **Datoteka je na napačnem mestu**
    - Windows: `%APPDATA%\Laprdus\`
    - Linux: `~/.config/Laprdus/`

#### Težava: beseda se zamenja na napačnih mestih

**Rešitev:** Za problematični vnos vklopite možnost **»Samo cela beseda«** ali za ta vnos ročno spremenite vrednost v konfiguracijski datoteki, `wholeWord: true`

Primer težave: Vnos za »TV« zamenja tudi del besede »Latvija«, ker vsebuje »TV«.

```json
{
    "grapheme": "TV",
    "phoneme": "Te Ve",
    "wholeWord": true
}
```

#### Težava: zamenjava ne deluje za velike in male črke

**Rešitev:** Za problematični vnos preverite možnost **»Razlikuj velike in male črke«** ali v konfiguracijski datoteki preverite vnos `caseSensitive`

- Za zamenjavo ne glede na velikost črk: `caseSensitive: false` oz. izklopljeno
- Za natančno ujemanje: `caseSensitive: true` oz. vklopljeno

#### Težava: sintaktična napaka JSON

Pogoste napake v obliki JSON:

**Pozabljena vejica:**

```json
// NAPAČNO:
{
    "grapheme": "test"
    "phoneme": "test"
}

// PRAVILNO:
{
    "grapheme": "test",
    "phoneme": "test"
}
```

**Vejica na koncu:**

```json
// NAPAČNO:
{
    "entries": [
        { "grapheme": "a", "phoneme": "b" },
    ]
}

// PRAVILNO:
{
    "entries": [
        { "grapheme": "a", "phoneme": "b" }
    ]
}
```

**Manjkata zavita oklepaja:**

```json
// NAPAČNO:
"grapheme": "test",
"phoneme": "test"

// PRAVILNO:
{
    "grapheme": "test",
    "phoneme": "test"
}
```

#### Težava: emodžiji se ne berejo

**Rešitev:** Preverite, ali je branje emodžijev vklopljeno

1. Odprite Konfigurator Laprdus ali nastavitve v Androidu
2. Poiščite možnost »Beri emodžije« (v Androidu »Omogoči branje emodžijev«)
3. Vklopite možnost
4. Shranite nastavitve

---

### 5.11 Naglasni slovar (accents.json)

Formantni glasovi (Zvonko, Stojan, Mirsad in pojoči glasovi) sami določijo mesto naglasa, dolžino in ton vsake besede: najprej iz naglasnih znamenj v besedilu, nato iz vgrajenega leksikona, nato po pravilih za končnice in besedne vrste (prevzete besede in izpeljanke z dolgim zadnjim zlogom osnove, glagoli s predpono, glagoli na -ovati), če pa nič od tega ne ustreza, gre naglas na prvi zlog. Besedo, ki se tako izgovori napačno, lahko popravite v naglasnem slovarju, in to ne samo eno njeno obliko: vnos lahko zajame celotno osnovo, seznam oblik ali cel glagol z vsemi osebami in časi. Posneti glasovi (Josip, Vlado in izpeljani) naglas določajo na enak način in uporabljajo isti slovar; slovar velja za izbrani jezik, s katerim koli glasom se govori.

Datoteka `accents.json` je poleg drugih slovarjev (`%APPDATA%\Laprdus` v sistemu Windows, `~/.config/Laprdus` v sistemu Linux) in se naloži skupaj z njimi, kadar so uporabniški slovarji vklopljeni. Naloži se enkrat, ob zagonu in ob spremembi datoteke; govor zaradi nje ni nič počasnejši.

#### Zapis

Vnosi se pišejo v zapisu vgrajenega leksikona:

| Znak | Pomen |
|------|----------|
| `'` | pred naglašenim samoglasnikom (ali zložnim r); ton se izbere samodejno |
| `^` | pred naglašenim samoglasnikom: padajoči naglas |
| `/` | pred naglašenim samoglasnikom: rastoči naglas |
| `:` | za dolgim samoglasnikom |
| `*` | na koncu: osnova, ki velja tudi za oblike z do tremi črkami več (kontróla, kontróle, kontrólom) |
| `\|` | `osnova\|nastavak\|nastavak`: po ena natančna oblika za vsako končnico; prazna končnica je sama osnova |

Primer datoteke:

```json
{
    "version": "1.0",
    "entries": [
        { "word": "kontr'o:l*", "comment": "kontróla, kontróle, kontrólom" },
        { "word": "sign'a:l|a|u|om|e|i|ima", "comment": "signála, signálu ...; sìgnal ostane, kot je" },
        { "word": "p^oruk*", "comment": "pȍruka: padajoči naglas na prvem zlogu" },
        { "word": "dir'ektor*", "language": "sr", "comment": "samo v srbščini, s katerim koli glasom" },
        { "verb": "ur'e:d=i<p", "comment": "uréditi, urédim, urédio, uređen" },
        { "verb": "dijel:i", "comment": "podijeliti, raspodijeliti, podijelim, podijeljen" }
    ]
}
```

- `word`: beseda, osnova (`*`) ali seznam oblik (`|`).
- `verb`: glagol. Piše se celotna osnova z naglasom nedoločnika, znak `=` in razred: `i` za glagole na -iti/-im, `a` na -ati/-am, `u` na -nuti/-nem, `t` na -ati z drugo osnovo v sedanjiku in `e` za to osnovo (`pok'a:z=t` in `pok'a:ž=e`). Za razredom so lahko oznake: `<` premakne naglas za en zlog nazaj v sedanjiku, `p` tudi v trpnem deležniku (tako govorita Stojan in Mirsad, Zvonko ohrani naglas nedoločnika), `n`, kadar je velelnik enak obliki nekega samostalnika (potvrdi, uredi), zato se bere kot glagol samo na začetku povedi. Glagol z dolgim *ije* v korenu se piše kot `korijen:vrsta` in velja s vsako predpono.
- `language`: `hr`, `sr`, `bs` ali `sl` omeji vnos na ta jezik, s katerim koli glasom se govori; brez tega velja za vse jezike. Slovenski vnos se piše v zapisu slovenskih slovarjev (`mésto`, `člôvek`, `p'əs`, `d'é:la*`): ostrivec za dolgi naglašeni samoglasnik (ozka é, ó), strešica za dolga široka ê, ô, krativec za kratki naglašeni, `ə` za polglasnik in `w` za l, ki se izgovori kot u (bíw).
- `comment`: opomba za vas, Laprdus je ne bere.

Natančna oblika ima prednost pred osnovo, daljša osnova pred krajšo, vaš vnos pa pred vgrajenim. Naglasna znamenja, napisana v samem besedilu ali v zamenjavi glavnega slovarja (`telèfon`), imajo prednost pred vsem.

Neveljaven vnos (brez naglasnega znamenja, z dvema naglasoma, z neznano črko ali neznanim razredom glagola) se preskoči, drugi pa veljajo. V ukazni vrstici `laprdus -w` izpiše, koliko vnosov je bilo sprejetih in kaj je bilo narobe s prvim zavrnjenim; dodatek za NVDA to zapiše v svoj dnevnik. Rezultat lahko takoj slišite:

```bash
laprdus -v zvonko "kontrola, signala, uredio"
```

**Urejanje:** naglasni slovar se ureja kot drugi slovarji: v Konfiguratorju Laprdus v sistemu Windows (gumb **Slovarji...**, vrsta slovarja *Naglasni slovar*, glejte [5.6](#56-urejanje-slovarjev-v-sistemu-windows-konfigurator-laprdus)), v aplikaciji za Android ([5.7](#57-urejanje-slovarjev-v-androidu)) in v aplikaciji za iPhone, iPad in Mac (zavihek Slovarji, vrsta *Naglasni slovar*). Vsak vnos ima polja Vnos, Vrsta vnosa (beseda ali glagol), Jezik in Komentar, urejevalnik pa ga preverja po istih pravilih, po katerih ga bere Laprdus, zato neveljavnega vnosa ni mogoče shraniti. Datoteko lahko še vedno urejate ročno ([5.8](#58-ročno-urejanje-datotek-json-napredno)); aplikaciji za Android in Apple jo bereta iz iste mape kot druge slovarje, kadar so uporabniški slovarji vklopljeni.

## 6. Glasovi

### 6.1 Razpoložljivi glasovi

Laprdus vključuje osemnajst glasov – dva osnovna, tri izpeljane, tri formantne in deset pojočih. **Vsak glas govori v vseh jezikih.** Glas določa samo zvok; jezik (hrvaščina, srbščina, bosanščina ali slovenščina) določa pravila izgovarjave, naglasni slovar, besede za števila (*tisuća* / *hiljada* / *tisoč*) in imena črk pri črkovanju, izbere pa se ločeno od glasu, na vsaki platformi na svoj način (glejte [6.3](#63-izbira-glasu)). *Izvorni jezik* v tabeli je jezik, v katerem glas govori, če ni zahtevano nič drugega. **Privzeti glas je Zvonko**, na vseh platformah in za vse jezike. Oznaka glasu (v stolpcu Oznaka) je namenoma zapisana brez diakritičnih znamenj, da se glasovi zanesljivo registrirajo na vsaki platformi; oznake iz prejšnjih različic (`djed`, `klapa`, `pjevac`, `pevac`, `solist`) še vedno veljajo v shranjenih nastavitvah in skriptih.

| Glas | Oznaka | Vrsta | Izvorni jezik | Opis |
|------|--------|-------|---------------|------|
| **Josip** | `josip` | Osnovni | Hrvaščina | Moški glas normalne višine. |
| **Vlado** | `vlado` | Osnovni | Srbščina | Moški glas normalne višine. |
| **Detence** | `detence` | Izpeljani | Hrvaščina | Otroški glas. Izpeljan iz glasu Josip s povišano višino. |
| **Baba** | `baba` | Izpeljani | Hrvaščina | Ženski glas. Izpeljan iz glasu Josip z rahlo povišano višino. |
| **Đedo** | `djedo` | Izpeljani | Srbščina | Starejši moški glas. Izpeljan iz glasu Vlado z znižano višino. |
| **Zvonko** | `zvonko` | Formantni | Hrvaščina | Moški glas. Govor nastaja po pravilih, brez posnetkov. Privzeti glas. |
| **Stojan** | `stojan` | Formantni | Srbščina | Moški glas, globlji in temnejši. Govor nastaja po pravilih, brez posnetkov. |
| **Mirsad** | `mirsad` | Formantni | Bosanščina | Moški glas, višji in bolj melodičen. Govor nastaja po pravilih, brez posnetkov. |
| **Orguljaš - Vila Velebita** | `orguljas` | Pojoči | Hrvaščina | Zvonko za orglami: poje pesem *Vila Velebita*. |
| **Dalmacija Klapa** | `dalmacija_klapa` | Pojoči | Hrvaščina | Zvonko kot cela klapa: poje pesem *Vila Velebita*. |
| **Lika Velebit** | `lika_velebit` | Pojoči | Hrvaščina | Zvonko poje s svojim glasom, v svoji višini: *Vila Velebita*. |
| **Trubač - Kreće se lađa francuska** | `trubac` | Pojoči | Srbščina | Stojan kot trobenta: poje pesem *Kreće se lađa francuska*. |
| **Harmonikaš - Kreće se lađa francuska** | `harmonikas` | Pojoči | Srbščina | Stojan kot harmonika: poje pesem *Kreće se lađa francuska*. |
| **Solunac** | `solunac` | Pojoči | Srbščina | Stojan poje s svojim glasom: *Kreće se lađa francuska*. |
| **Sevdalija - Kad ja pođoh na Bembašu** | `sevdalija` | Pojoči | Bosanščina | Mirsad poje sevdah: *Kad ja pođoh na Bembašu*. |
| **Sazlija - Kad ja pođoh na Bembašu** | `sazlija` | Pojoči | Bosanščina | Mirsad kot saz: poje pesem *Kad ja pođoh na Bembašu*. |
| **Sarajevo Bembaša** | `sarajevo_bembasa` | Pojoči | Bosanščina | Mirsad poje s svojim glasom: *Kad ja pođoh na Bembašu*. |
| **Bećarac** | `becarac` | Pojoči | Hrvaščina | Zvonko s svojim glasom poje bećarac: vsako dvostišje v desetercih se ujame z napevom. |

### 6.2 Osnovni in izpeljani glasovi

**Osnovni glasovi** (Josip in Vlado) uporabljajo fizične posnetke fonemov – resnične zvočne posnetke govorcev.

**Izpeljani glasovi** (Detence, Baba, Đedo) uporabljajo iste zvočne posnetke kot osnovni glasovi, vendar s prilagojeno višino glasu:

- **Detence** uporablja foneme glasu Josip z višino 1,5x (višji glas)
- **Baba** uporablja foneme glasu Josip z višino 1,2x (rahlo višji glas)
- **Đedo** uporablja foneme glasu Vlado z višino 0,75x (nižji glas)

Osnovni in izpeljani glasovi, tako kot vsi drugi, govorijo vse štiri jezike: Josip v srbščini reče *hiljada* in črkuje *lje*, Vlado v hrvaščini *tisuća* in *elj*, v slovenščini pa *tisoč*.

**Formantni glasovi** (Zvonko, Stojan, Mirsad) ne uporabljajo posnetkov. Govor se v celoti izračuna po pravilih, kot pri klasičnih formantnih sintetizatorjih: iz besedila se določijo glasovi, naglas in stavčna intonacija, nato pa zvok ustvari formantni sintetizator. Zato ti glasovi:

- ne potrebujejo datotek s podatki glasov
- ostanejo čisti in razumljivi tudi pri zelo velikih hitrostih, ker se hitrost in višina ne dobita z naknadno obdelavo zvoka
- imajo stavčno intonacijo (izjava, vprašanje, vejica, vzklik)
- števila izgovarjajo, tako kot vsi glasovi, glede na izbrani jezik (hrvaščina: *tisuća*, *milijun*; srbščina: *hiljada*, *milion*, *dvesta*; bosanščina: *hiljada*, *milion*, *dvjesto*; slovenščina: *tisoč*, *milijon*, *enaindvajset*)

Če formantni glas kakšno besedo naglasi na napačnem zlogu, lahko naglas določite v slovarju izgovarjave z običajnimi naglasnimi znamenji, na primer `telèfon`, `gláva`, `kȕća` ali `grȃd`.

**Pojoči glasovi** (Orguljaš - Vila Velebita, Dalmacija Klapa, Lika Velebit in Bećarac z Zvonkovim glasom, Trubač - Kreće se lađa francuska, Harmonikaš - Kreće se lađa francuska in Solunac s Stojanovim, Sevdalija - Kad ja pođoh na Bembašu, Sazlija - Kad ja pođoh na Bembašu in Sarajevo Bembaša z Mirsadovim) so formantni glasovi, ki besedila ne govorijo, ampak ga pojejo, kot so to počeli pojoči glasovi klasičnih sintetizatorjev in namiznih računalnikov. Vsak zlog dobi eno noto ljudske pesmi (Zvonkovi glasovi pojejo *Vila Velebita*, Stojanovi *Kreće se lađa francuska*, Mirsadovi *Kad ja pođoh na Bembašu*), v tistem od štirih jezikov, ki je izbran; prva kitica pesmi natančno sede na melodijo, vsako drugo besedilo pa se poje na isto melodijo, zlog za zlogom, od začetka z vsako novo povedjo, ki jo program pošlje. Hitrost govora določa tempo, višina transponira pesem, intonacija pa določa globino vibrata (0 % ga izklopi). Namesto grla ima vsak glas svoj »inštrument«: orgle s cerkvenim odmevom, klapo iz več glasov z basom oktavo nižje, trobento, dva jezička harmonike, uglašena nekaj centov narazen, sevdalijski glas s širokim vibratom in saz s tremolom. Lika Velebit, Solunac in Sarajevo Bembaša pojejo z običajnim glasom Zvonka, Stojana in Mirsada, brez inštrumenta in odmeva, pesem pa jim je transponirana v višino, v kateri sicer govorijo.

### 6.3 Izbira glasu

#### Windows SAPI5

SAPI5 ponuja štiri glasove, po enega za vsak jezik: Laprdus Croatian, Laprdus Serbian, Laprdus Bosnian in Laprdus Slovenian. Ti izberejo jezik; s katerim glasom bodo govorili, izberete v Konfiguratorju Laprdus in to velja za vse štiri (v `settings.json` je to `voice.default`; ključa za jezik ni, ker jezik izbere program, ki govori). Privzeti glas je Zvonko.

1. Odprite Konfigurator Laprdus in v skupini Nastavitve glasu izberite želeni glas (Josip, Vlado, Detence, Baba, Đedo, Zvonko, Stojan, Mirsad ali pojoči glas), nato shranite nastavitve
2. Odprite Nadzorno ploščo
3. Izberite »Dostopnost« > »Prepoznavanje govora« > »Pretvorba besedila v govor«
4. V spustnem seznamu »Izbira glasu« izberite jezik: Laprdus Croatian, Laprdus Serbian, Laprdus Bosnian ali Laprdus Slovenian
5. Izberite »Uporabi«

#### NVDA

V NVDA so vsi glasovi Laprdusa na voljo za vsak jezik: glas in jezik izberete ločeno (glejte [3.3](#33-nastavitve-v-nvda)). Privzeti glas je Zvonko.

**1. način: nastavitve govora**

1. Odprite meni NVDA (tipka NVDA + N)
2. Izberite Izbire > Nastavitve > Govor
3. V spustnem seznamu »Glas« izberite želeni glas (seznam vsebuje vseh 18 glasov)
4. V spustnem seznamu »Jezik« izberite hrvaščino, srbščino, bosanščino ali slovenščino
5. Pritisnite »V redu« ali »Uporabi«

**2. način: hitri krog nastavitev**

1. Pritisnite Control + tipka NVDA + puščica levo/desno, dokler ne pridete do nastavitve »Glas« ali »Jezik«, nato s puščico gor/dol spremenite vrednost
2. Ali pa uporabite Control + tipka NVDA + V, da odprete pogovorno okno za izbiro glasu

Če je vklopljena možnost NVDA »Samodejna menjava jezika, (ko je to možno)«, Laprdus med hrvaščino, srbščino, bosanščino in slovenščino preklaplja sam glede na jezik besedila, glasu pa ne spremeni.

#### Linux (Speech Dispatcher)

Speech Dispatcher vsak glas navede za vsak jezik. Jezik zahteve (`-l`) določa jezik, glas (`-y`) pa ostane tisti, ki ste ga izbrali; brez izbranega glasu govori Zvonko, v vseh štirih jezikih. Simbolični glasovi so za vse jezike enaki: MALE1 je Zvonko, MALE2 Josip, MALE3 Đedo, FEMALE1 Baba, CHILD_MALE Detence.

```bash
# Seznam razpoložljivih glasov
spd-say -o laprdus -L

# Uporaba določenega glasu (govori svoj izvorni jezik)
spd-say -o laprdus -y Josip "Tekst na hrvatskom"
spd-say -o laprdus -y Vlado "Tekst na srpskom"

# Kateri koli glas v katerem koli jeziku
spd-say -o laprdus -l sr -y Zvonko "Zvonko govori srpski"
spd-say -o laprdus -l hr -y Vlado "Vlado govori hrvatski"
spd-say -o laprdus -l bs "Zvonko govori bosanski"
spd-say -o laprdus -l sl "Zvonko govori slovensko"
```

Za trajno spremembo v bralniku zaslona Orca:

1. Odprite nastavitve bralnika Orca (Insert+preslednica)
2. Na zavihku »Voice« v spustnem seznamu izberite želeni glas; jezik, v katerem govori, določa jezik, ki ga zahteva Orca

#### Android

Laprdus sistemu ponuja vsak glas pod hrvaščino, srbščino, bosanščino in slovenščino (72 sistemskih glasov). Jezik, ki ga zahteva TalkBack ali druga aplikacija, določa jezik, glas pa ostane tisti, ki je izbran v aplikaciji; privzeti je Zvonko.

1. Odprite aplikacijo Laprdus
2. Tapnite »Nastavitve Laprdusa«
3. V razdelku »Glas« tapnite spustni seznam **»Glas«**
4. Na seznamu izberite želeni glas
5. V meniju **»Jezik«** izberite jezik za govor v sami aplikaciji; če je vklopljena možnost »Vsili izbrani jezik«, se v tem jeziku govori tudi, kadar aplikacije zahtevajo drugega

Za spremembo privzetega glasu v sistemu:

1. Odprite Nastavitve > Funkcije za ljudi s posebnimi potrebami > Pretvorba besedila v govor
2. Tapnite ikono zobnika ob »Laprdus TTS«
3. Izberite želeni glas

#### Apple (iPhone, iPad in Mac)

V funkciji Branje in govorjenje (iOS: Nastavitve > Dostopnost > Branje in govorjenje > Glasovi; macOS: Nastavitve sistema > Dostopnost > Branje in govorjenje > Sistemski glas > Upravljanje glasov) in v nastavitvah glasu funkcije VoiceOver je vsak glas Laprdusa naveden pod hrvaščino, srbščino, bosanščino in slovenščino; element, ki ga izberete, določi glas in jezik. V aplikaciji Laprdus meni »Glas« izbere glas, meni »Jezik« pa jezik za govor v sami aplikaciji. Privzeti glas je Zvonko.

---

## 7. Odpravljanje težav

### 7.1 Pogoste težave

#### Ni zvoka

1. Preverite, ali je Laprdus izbran kot aktivna sinteza
2. Prepričajte se, da glasnost ni nastavljena na 0
3. Preverite glasnost sistema v računalniku
4. Znova zaženite program, ki ga uporabljate

#### Govor je prehiter ali prepočasen

Nastavite hitrost govora:

- V NVDA: tipka NVDA + N > Izbire > Nastavitve > Govor > Hitrost
- V Androidu: Odprite aplikacijo Laprdus in v nastavitvah Laprdusa nastavite drsnik hitrosti
- V programu za ukazno vrstico: uporabite možnost `-r`

#### NVDA ne najde Laprdusa

1. Preverite, ali je dodatek pravilno nameščen (meni NVDA > Orodja > Add-on store)
2. Znova zaženite NVDA
3. Prepričajte se, da dodatek ni onemogočen

#### Speech Dispatcher ne deluje v sistemu Linux

1. Preverite, ali je modul Laprdus nameščen:

        ls /usr/lib/speech-dispatcher-modules/sd_laprdus

2. Preverite konfiguracijo:

        grep laprdus /etc/speech-dispatcher/speechd.conf

3. Znova zaženite Speech Dispatcher:

        systemctl --user restart speech-dispatcher

### 7.2 Diagnostični dnevniki

#### Dodatek za NVDA

Če želite vklopiti diagnostične dnevnike za dodatek za NVDA:

1. Ustvarite prazno datoteko `laprdus_debug` v začasni mapi
    - Windows: `%TEMP%\laprdus_debug`
2. Znova zaženite NVDA
3. Dnevniki se shranjujejo v `%TEMP%\laprdus_debug.log`

#### Linux

Za podrobnejši izpis uporabite možnost `-w`:

```bash
laprdus -w "Test"
```

### 7.3 Stik in podpora

Za pomoč in podporo:

- **E-pošta:** hrvojekatic@gmail.com
- **Spletno mesto:** https://hrvojekatic.com/laprdus
- **Izvorna koda:** https://github.com/hkatic/laprdus

Pri prijavi težave navedite:

- Različico Laprdusa
- Operacijski sistem
- Uporabljeni bralnik zaslona (če je smiselno)
- Opis težave
- Korake za ponovitev težave

---

Zadnja sprememba: november 2026.
