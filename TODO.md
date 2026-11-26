# TODO

The work planned and in progress, and the questions waiting for an answer.
The bugs themselves are described in [ISSUES.md](ISSUES.md).

- A done item gets `[x]` and its commit.
- It is removed, with its entry in ISSUES.md, once it is confirmed.

## The round of 2026-11-26

In the order the maintainer asked for:

1. [ ] Slovenian clock times, the listener's second answers (I-1, I-2)
2. [ ] Croatian accents of the tester's 38 words (I-3)
3. [ ] Android: TalkBack's deleted character (I-4), then the full local rebuild and a check on the phone
4. [ ] "šok" read "šokej": whole-word dictionary entries next to č, ć, đ, š, ž (I-5)
5. [ ] MP3, MP4, MPEG4, ID3 never ordinals; `MP3 128 kbps` (I-6, I-7)
6. [ ] HSignal, ESignal (I-8)
7. [ ] The full local rebuild; the installers and the APK for testing

## Questions waiting for an answer

- **For the Slovenian listener (next round):**
  - `ob 8:00 do 9:00` is read "ob osmih do devete". Should it be "ob osmih do devetih", like `ob 8:30–9:00`?
  - Should "nič pet" also go after od, pred, po and med? Today `od 8:05` is "od osme in pet minut" and `pred 10:05` "pred deseto in pet minut". The maintainer asked to change only *do* for now.
  - `od 8:05 do 16:00` is read "od osmih in pet minut do šestnajstih".
- **For the maintainer:**
  - Where was MP3 (or MP4, ID3) heard as an ordinal: which app, and what exact text?
  - ESignal: after the fix, is the stress on "si" right by ear?
