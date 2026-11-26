# TODO

The work planned and in progress, and the questions waiting for an answer.
The bugs themselves are described in [ISSUES.md](ISSUES.md).

- A done item gets `[x]` and its commit.
- It is removed, with its entry in ISSUES.md, once it is confirmed.

## The round of 2026-11-26

In the order the maintainer asked for:

1. [x] Slovenian clock times, the listener's second answers (I-1, I-2): b915469
2. [x] Croatian accents of the tester's 38 words (I-3): 3f91015
3. [x] Android: TalkBack's deleted character (I-4): e63237e
   - [ ] On the phone: install the debug APK, delete characters with TalkBack in a text field, and read in `SpeechRequestLog` what TalkBack sends (the phone was not connected on 2026-11-26)
4. [x] "šok" read "šokej": whole-word dictionary entries next to č, ć, đ, š, ž (I-5): the commit "Whole-word dictionary entries know every letter (šok was read šokej)"
5. [ ] MP3, MP4, MPEG4, ID3 never ordinals; `MP3 128 kbps` (I-6, I-7)
6. [ ] HSignal, ESignal (I-8)
7. [ ] The full local rebuild; the installers and the APK for testing

## Later

- Rerunning `tools/formant/english_words.py` drops *control* from the English table shared by hr, sr and bs, because the Croatian lexicon now overrides it. Serbian and Bosnian would then read it on the first syllable too. Decide before the next rerun.

## Questions waiting for an answer

- **For the Slovenian listener (next round):**
  - `ob 8:00 do 9:00` is read "ob osmih do devete". Should it be "ob osmih do devetih", like `ob 8:30–9:00`?
  - Should "nič pet" also go after od, pred, po and med? Today `od 8:05` is "od osme in pet minut" and `pred 10:05` "pred deseto in pet minut". The maintainer asked to change only *do* for now.
  - `od 8:05 do 16:00` is read "od osmih in pet minut do šestnajstih".
- **For the maintainer:**
  - Croatian: four words follow the tester against HJP, each stressed one syllable after HJP's rising accent: instrumént (HJP instrùment), pauzíra (paùzīrām), pozícija (pòzīcija), umétanje (ùmetānje). Keep them, or ask the tester again?
  - Croatian: *kraj* is long now (HJP krȃj, the noun), also where it is the preposition, which HJP has short (krȁj kuće).
  - Croatian: *control* is stressed on its first syllable now, but its *c* is still read [ts] ("tsontrol"), as the English words keep the voices' letter sounds. Should it be "kontrol"?
  - Where was MP3 (or MP4, ID3) heard as an ordinal: which app, and what exact text?
  - ESignal: after the fix, is the stress on "si" right by ear?
