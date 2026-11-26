# Known issues

Bugs reported by testers or found by the maintainer, and where each stands.
The work on them is planned in [TODO.md](TODO.md).

- An entry stays under **Open** until its fix is committed.
- It then moves to **Fixed, waiting for confirmation**, with the commit.
- It is removed once a tester or the maintainer confirms the fix.

Testers are named by role only.

## Open

### I-1 Slovenian: two whole hours after *od* are read as ordinals

- **Where:** Slovenian, every platform
- **Reported:** the Slovenian listener's answers of 2026-11-24 (second round on clock times, question 4)
- **Now:** `od 8:00 do 16:00` and `od 8:00–16:00` are read "od osme do šestnajste"
- **Wanted:** "od osmih do šestnajstih". With ura it stays "od osme do šestnajste ure".

### I-2 Slovenian: minutes under ten after *do*

- **Where:** Slovenian, every platform
- **Reported:** the same answers, question 6
- **Now:** `do 17:05` is read "do sedemnajste in pet minut"
- **Wanted:** "do sedemnajste nič pet". `ob 8:05` stays "ob osmih in pet minut" (first round).

### I-3 Croatian: the accents of 38 words

- **Where:** Croatian only
- **Reported:** a Croatian tester, 2026-11-26
- **Tester's notation:** stress on the syllable named, "long" = the stressed vowel must be long

Stress on the wrong syllable:

| Word | Wanted | Now |
|---|---|---|
| namještaj | na | mje, long |
| precizno | pre | ci |
| pomicanje | po | mi |
| control | con | trol |
| neprekinuti (one word) | pre | ki |
| duljina | lji | du |
| oktava | ta | ok |
| napraviti | na | pra |
| metoda | to, long | me |
| instrument | ment | ru |
| korištenjem | ko | šte, long |
| pauzira | zi, long | pa |
| nastalu | na | sta, long |
| ostavljanja | o | sta |
| praznina | ni | pra |
| pokriveni | kri | po |

Right syllable, but it must be long (today short): zarez, znak, naglasak, uvod, zaseban, taj, ta, nastavak, nastava, mali, savjet, kraj, spajanje, prazno, prostor, regija, pozicija (zi), redu, način, šum, disanje, umetanje (me).

### I-4 Android: TalkBack's deleted character is read as running text

- **Where:** Android with TalkBack, every language
- **Reported:** the maintainer, 2026-11-25
- **Now:** "Tekst w je izbrisan" is read "tekst duplo ve je izbrisan", not as the letter is spelled. A deleted vowel is worse: in "Tekst a je izbrisan" the *a* is the unstressed conjunction.
- **Wanted:** the deleted character spoken as when spelling or typing on the keyboard (spelling mode, spelling speed, the user's spelling entries)
- **Notes:** TalkBack's templates (`template_text_removed`) are "Tekst %1$s izbrisan" (hr; the phone says "Tekst %1$s je izbrisan"), "Текст %1$s је избрисан" / "Tekst %1$s je izbrisan" (sr), "%1$s je izbrisan" (bs) and "Besedilo %1$s je izbrisano" (sl). A deleted symbol arrives as its name.

### I-5 "šok" is read "šokej"

- **Where:** every language and platform; the bundled and the user's pronunciation dictionaries
- **Reported:** the maintainer, 2026-11-25
- **Cause:** the bundled entry `OK` → `Okej` matches inside "šok". A whole-word entry's word boundaries know only ASCII letters, so č, ć, đ, š and ž do not count as letters.

### I-6 MP3, MP4, MPEG4, ID3 read as ordinals

- **Where:** every language
- **Reported:** the maintainer, 2026-11-25 ("never as ordinals, in every language")
- **Now:**
  - Plain `MP3`, `MP4`, `MPEG4` and `ID3` are cardinals ("em pe tri").
  - Ordinals appear in `MPEG-4. datoteka` ("MPEG četvrta datoteka"; Slovenian "četrta") and `MP 3. datoteka` ("MP treća datoteka").
  - `1.mp3` is read "jedan mp tri", with the period silent.
- **Open question:** where the maintainer heard it (which app, what exact text).

### I-7 `MP3 128 kbps` is read as one number

- **Where:** every language
- **Found:** 2026-11-26, while checking I-6
- **Now:** "MP tri tisuće sto dvadeset osam kbps". The rule for thousands separated by spaces joins the 3 of MP3 with 128.
- **Wanted:** "MP tri, sto dvadeset osam kbps"

### I-8 HSignal, ESignal

- **Where:** every language
- **Reported:** the maintainer, 2026-11-25
- **Now:**
  - "HSignal" is one word, "hsignal".
  - "ESignal" is one word, "esìgnal", with the stress of *signal* on "si", yet the maintainer hears it elsewhere.
- **Wanted:** "H Signal" or "HaSignal"; for ESignal, the stress on "Si"

## Fixed, waiting for confirmation

None yet.
