# Known issues

Bugs reported by testers or found by the maintainer, and where each stands.
The work on them is planned in [TODO.md](TODO.md).

- An entry stays under **Open** until its fix is committed.
- It then moves to **Fixed, waiting for confirmation**, with the commit.
- It is removed once a tester or the maintainer confirms the fix.

Testers are named by role only.

## Open

### I-8 HSignal, ESignal

- **Where:** every language
- **Reported:** the maintainer, 2026-11-25
- **Was:**
  - "HSignal" is one word, "hsignal".
  - "ESignal" is one word, "esìgnal", with the stress of *signal* on "si", yet the maintainer hears it elsewhere.
- **Wanted:** "H Signal" or "HaSignal"; for ESignal, the stress on "Si"

## Fixed, waiting for confirmation

### I-7 `MP3 128 kbps` was read as one number

- **Where:** every language
- **Found:** 2026-11-26, while checking I-6
- **Was:** "MP tri tisuće sto dvadeset osam kbps". The rule for thousands separated by spaces joins the 3 of MP3 with 128.
- **Wanted:** "MP tri, sto dvadeset osam kbps"
- **Now:** "MP tri", the pause before a number after a designation, "sto dvadeset osam kbps", in every language
- **Fixed in:** the same commit as I-6
- **To confirm:** the maintainer

### I-6 MP3, MP4, MPEG4, ID3 were read as ordinals

- **Where:** every language
- **Reported:** the maintainer, 2026-11-25 ("never as ordinals, in every language")
- **Was:**
  - Plain `MP3`, `MP4`, `MPEG4` and `ID3` are cardinals ("em pe tri").
  - Ordinals appear in `MPEG-4. datoteka` ("MPEG četvrta datoteka"; Slovenian "četrta") and `MP 3. datoteka` ("MP treća datoteka").
  - `1.mp3` is read "jedan mp tri", with the period silent.
- **Open question:** where the maintainer heard it (which app, what exact text).
- **Now:** never an ordinal: `MPEG-4. datoteka` "MPEG četiri" (the period ends the clause), `MP 3. datoteka` "MP tri", `1.mp3` "jedan točka em pe tri", in every language. Where the maintainer heard the ordinal is still open (TODO.md); if it was another spelling, it goes here again.
- **Fixed in:** the commit "MP3, MPEG-4, ID3 are never ordinals; MP3 128 kbps is not one number"
- **To confirm:** the maintainer

### I-5 "šok" was read "šokej"

- **Where:** every language and platform; the bundled and the user's pronunciation dictionaries
- **Reported:** the maintainer, 2026-11-25
- **Cause:** the bundled entry `OK` → `Okej` matches inside "šok". A whole-word entry's word boundaries know only ASCII letters, so č, ć, đ, š and ž do not count as letters.
- **Now:** a whole-word entry's boundaries know every letter of Unicode (`is_word_boundary()` in `src/common/dictionary/pronunciation.rs`): "šok" stays "šok", "OK" is still "Okej", and an entry ending in č or ć now matches before a space too
- **Fixed in:** 9a0852f
- **To confirm:** the maintainer

### I-4 Android: TalkBack's deleted character was read as running text

- **Where:** Android with TalkBack, every language
- **Reported:** the maintainer, 2026-11-25
- **Was:** "Tekst w je izbrisan" was read "tekst duplo ve je izbrisan", not as the letter is spelled. A deleted vowel is worse: in "Tekst a je izbrisan" the *a* is the unstressed conjunction.
- **Wanted:** the deleted character spoken as when spelling or typing on the keyboard (spelling mode, spelling speed, the user's spelling entries)
- **Notes:** TalkBack's templates (`template_text_removed`) are "Tekst %1$s izbrisan" (hr; the phone says "Tekst %1$s je izbrisan"), "Текст %1$s је избрисан" / "Tekst %1$s je izbrisan" (sr), "%1$s je izbrisan" (bs) and "Besedilo %1$s je izbrisano" (sl). A deleted symbol arrives as its name.
- **Now:** the engine recognizes these messages (one character between the template's words) and spells the character as when spelling or typing; the words around it are read as text. A deleted word stays text.
- **Fixed in:** e63237e
- **To confirm:** on the phone, what TalkBack really sends (the debug build's `SpeechRequestLog`), then the maintainer by ear

### I-3 Croatian: the accents of 38 words

- **Where:** Croatian only
- **Reported:** a Croatian tester, 2026-11-26
- **Tester's notation:** stress on the syllable named, "long" = the stressed vowel must be long

Stress on the wrong syllable:

| Word | Wanted | Was |
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

Right syllable, but it must be long (was short): zarez, znak, naglasak, uvod, zaseban, taj, ta, nastavak, nastava, mali, savjet, kraj, spajanje, prazno, prostor, regija, pozicija (zi), redu, način, šum, disanje, umetanje (me).

- **Fixed in:** 3f91015
- **Now:** every word as the tester asked, with HJP's accent type and case forms. Four follow the tester against HJP (instrumént, pauzíra, pozícija, umétanje: HJP has instrùment, paùzīrām, pòzīcija, ùmetānje); see TODO.md. Serbian and Bosnian are unchanged.
- **To confirm:** the Croatian tester

### I-1 Slovenian: two whole hours after *od* were read as ordinals

- **Where:** Slovenian, every platform
- **Reported:** the Slovenian listener's answers of 2026-11-24 (second round on clock times, question 4)
- **Was:** `od 8:00 do 16:00` and `od 8:00–16:00` were read "od osme do šestnajste"
- **Now:** "od osmih do šestnajstih". With ura it stays "od osme do šestnajste ure"; `od 8:00` alone stays "od osme".
- **Fixed in:** b915469
- **To confirm:** the Slovenian listener

### I-2 Slovenian: minutes under ten after *do*

- **Where:** Slovenian, every platform
- **Reported:** the same answers, question 6
- **Was:** `do 17:05` was read "do sedemnajste in pet minut"
- **Now:** "do sedemnajste nič pet" (`do 17:02` "nič dve"). `ob 8:05` stays "ob osmih in pet minut" (first round); od, pred, po and med keep "in pet minut" until asked (see TODO.md).
- **Fixed in:** b915469
- **To confirm:** the Slovenian listener
