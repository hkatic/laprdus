# Formant voices: design and tuning notes

Zvonko (Croatian), Stojan (Serbian) and Mirsad (Bosnian) are synthesized by
rule. Nothing is recorded: the text is turned into sounds, the sounds into
control tracks (formants, amplitudes, pitch), and the tracks into audio by a
formant synthesizer. This is the approach of DECtalk and Eloquence.

This document explains how the pieces fit together, where the numbers come
from, and how to check a change. Code lives in `src/formant/`.

## 1. Pipeline

```
text ──▶ TTSEngine::preprocess_text()        emoji, pronunciation dictionary, numbers
     ──▶ InflectionProcessor::analyze_text() split into clauses at punctuation
     ──▶ Frontend::process()                 clause ──▶ phones with stress
     ──▶ ClauseBuilder                       phones ──▶ control frames (one per ~2 ms)
     ──▶ KlattSynth::render()                frames ──▶ 16-bit PCM, 22050 Hz
```

The first two steps are shared with the concatenative voices. A mark ends a
clause only when whitespace, a bracket or quote, or the end of the text follows
it (eSpeak's rule); glued to the next character ("12:30", "Hej!ti") it stays
in the clause and the front end's symbol table reads it by name
(*dvotočka*/*dvotačka*, *uskličnik*/*uzvičnik*). A glued period is read by
name (*točka*/*tačka*) only before a file extension or domain label, one to
five letters or digits in one case ("datoteka.txt", "pjesma.mp3",
"www.index.hr", "README.TXT"), and the decimal point is the number
converter's own mark (U+2024, "3.14" *tri točka četrnaest*); any other
glued period ends the clause like one before a space, because a screen
reader glues the items of a label that way ("Preslušano.Nestajuća poruka",
"1.Prvi"). A period after a single letter is the dot of an abbreviation
("s.a.r.s.", "U.S.A."): the tokenizer drops it and reads the letters by
name, as eSpeak and RHVoice do. A number glued to letters is set off from them by the number converter
("u20:03" *u dvadeset nula tri*; before, "udvadeset" was one word stressed
on the *u*), and the period that joins a word to such letters, as a screen
reader glues the parts of a label ("7.listopada.u20:03"), becomes a word
break instead of *točka*. The number converter reads every group of digits after a period as a
whole number ("1.317" *jedan točka tristo sedamnaest*, eSpeak's reading),
writes the decimal comma as *zarez*, except between the single digits of a
phone number spelled out by a screen reader ("+ 1 2 3,5 6,7 8 9,8 7 6": at
least six single digits separated by spaces or glued commas, at least two of
them touching no comma), where it writes a comma and a space, so the groups
are clauses with the comma pause. It reads clock times ("12:30") and dates
("7.10.2026", "7. 10. 2026.") without the separators, and a number of up to
four digits followed by a period before a lowercase word as the cardinal
("7. listopada" *sedam listopada*, "1990. godine") with the period silent;
before an uppercase word the period ends the sentence as usual. Ordinals
(*sedmi*, the day and month of a date as *sedmi deseti*) were read until
October 2026 and taken out: they need the case of the noun (*sedmog
listopada*), which the converter cannot know, and the nominative was wrong
more often than right. A line break ends the clause too (since
October 2026), with the newline pause and the contour of a clause without a
mark, so the lines of a post, a list or a label are phrases of their own; a
run of breaks and spaces (a blank line, CR LF) is one boundary, and after a
mark that already ended the clause it adds nothing. Until then a line break
was a space and the newline pause setting did nothing.

| File | Role |
|------|------|
| `formant_frontend.cpp` | Letter-to-sound, stress, clitics, assimilation |
| `formant_lexicon.cpp` | Accent lexicon (words the rules get wrong) |
| `formant_phonemes.cpp` | Acoustic definition of every phone |
| `formant_synthesizer.cpp` | Durations, transitions, amplitudes, intonation; the three speakers |
| `klatt_synth.cpp` | The synthesizer itself |

## 2. The synthesizer (`klatt_synth.cpp`)

A cascade/parallel formant synthesizer after Klatt (1980).

- **Voice source.** A KLGLOTT88-style glottal pulse, generated as flow
  derivative (so lip radiation is included). The closing edge is band-limited
  to avoid aliasing. A one-pole filter adds spectral tilt where wanted (voice
  bars, /v/), a first-order emphasis makes the source as bright as a real
  modal voice above 2 kHz. Jitter, shimmer and slow flutter keep sustained
  voicing from sounding mechanical; they are seeded, so output is
  reproducible.
- **Cascade branch** (voicing and aspiration): nasal zero and pole, then eight
  formants. F1-F4 move, F5-F8 are fixed per speaker.
- **Parallel branch** (frication, bursts): three band-pass resonators that
  follow F2-F4 and three at fixed frequencies, plus a flat share. The moving
  ones make a /k/ burst sit on F2 and a /t/ burst on F4 *of that moment*, so
  the burst and the following vowel transition tell the ear the same thing.
  The fixed ones shape sibilants. All frication noise first passes a
  fourth-order low-pass at 6.6 kHz (see "Sibilants" below).

Control frames are 44 samples long; parameters are interpolated in between
and filter coefficients are recomputed twice per frame.

## 3. Sounds (`formant_phonemes.cpp`)

Each phone has formant targets or loci, bandwidths, source levels, a noise
spectrum, an inherent duration and a transition time.

Transitions use locus equations: at a consonant-vowel boundary the formant
starts at `locus + k * (vowel - locus)`, where `k` says how much the consonant
gives way to the vowel (labials a lot, palatals hardly). The velar locus
follows the vowel, with F2 and F3 pinched together.

Points specific to Croatian/Serbian/Bosnian:

- Voiceless stops are **unaspirated** (short voice onset time, longest for
  /k/); voiced stops have a voice bar through the closure.
- /t d/ are **dental**; their burst is strongest at 3.5-4.5 kHz. After a
  fricative a stop keeps a silent gap of its own, and before a pause it is
  released audibly (see "Stops" below).
- **/r/** is a vocalic stretch only 4-8 dB below the vowels with brief
  (about 15 ms, 20 dB deep) tongue-tip contacts, each followed by a faint
  release transient; F3 drops about 300 Hz below the vowel's around it. Between
  vowels and after a consonant there is one contact (a tap), at the start of
  an utterance and before a consonant or a pause two (a short trill with a
  period of about 35 ms); with a single contact the lone vocalic onset of an
  initial /r/ was heard as /v/. This was measured on the
  recorded speaker and on Eloquence (Italian and Spanish *ara*, *arra*,
  *cara*): both keep the vowel nearly full between contacts. The earlier
  version, a 60 ms stretch 20 dB down with two contacts and no F3 movement,
  was heard as a muffled /l/ or /d/. Syllabic r is vocoid + contact + weaker
  vocoid.
- **/j/** is a real constriction, not a weak /i/: F1 about 270 Hz (Eloquence
  250-300, the recorded speaker 250-400), F2 near 2000 Hz between two a's,
  and F3 at 3050 Hz, well above the vowels' 2500-2700. The high F3 is what
  the recordings show in *ja*, *ji*, *moj* (2900-3400 Hz) and Eloquence in
  *ieri* (3000 Hz), and it is the only thing that separates /j/ from /i/ in
  *ji*, *ija*, *moj*: with F3 at 2750 the two were the same sound and *ji*
  was a long *i*. Transitions take about 50 ms. The earlier version had F1
  near 400 Hz and was 6-11 dB down, closer to a diphthong.
- **/v/** is an approximant: a weak low murmur with no friction.
- **č/ć and dž/đ** differ in noise spectrum and in formant loci (ć and đ have
  palatal transitions). How far apart they are is a property of the speaker
  (`hard_palatal_shift`, `soft_palatal_shift`). The affricates' friction sits
  about 200 Hz below that of š and [ɕ] (centroids for Zvonko: č 3.55 kHz,
  ć 3.95 kHz; before 3.75 and 4.15 kHz; č was later lowered to 3.3 kHz, see
  "l, n and č"), after a listening test asked for
  lower č, ć and đ. The voiced affricates' friction was also raised a little
  (đ was 17 dB down, the recorded speaker's is 8 dB down). **c** no longer
  shares the /s/ spectrum: its friction is centred at 4.5 kHz and 12 dB
  below the vowels (before 5.0 kHz and 9 dB; Eloquence's [ts] 4.4 kHz and
  17 dB), after a listening test found it too sharp.
- **/h/** is aspiration shaped by the neighbouring vowel.
- Unstressed vowels are only slightly centralized; there is no vowel
  reduction to speak of in these languages.

### Where the values come from

- **Vowel formants**: adult male speakers of standard Croatian, Bakran &
  Stamenković (1990) and Bakran (1996). Averages of F1/F2/F3 in running
  speech: i 282/2192/2713, e 471/1848/2456, a 664/1183/2433, o 482/850/2472,
  u 324/717/2544 Hz. The targets in the table are a little more peripheral,
  because short vowels in context undershoot their targets.
- **Durations**: Bakran (1984): consonants (p 85, t 76, k 81, b 65, d 54,
  m 56, c 113, č 90, ć 98 ms), relative vowel durations (i and u shortest, a
  longest), voiced fricatives about 25% shorter than voiceless, about five
  syllables per second.
- **Consonant spectra, levels and transitions**: measured with
  `tools/formant/analyze.py` on recordings of a male Croatian speaker (the
  MBROLA `cr1` diphone database, 16 kHz) and, for the range above 8 kHz, on a
  wide-band Croatian voice. Some of the values the table was matched to
  (levels relative to the loudest vowel frame):

  | Sound | Level | Spectrum, formants |
  |-------|-------|--------------------|
  | s | -3 to -9 dB | energy 5-6.5 kHz, little below 4 kHz (not followed, see "Sibilants") |
  | š | -2 to -8 dB | 2.5-6.5 kHz, centre near 4 kHz (not followed, see "Sibilants") |
  | c, č, ć (friction) | -1 to -5 dB | closure about as long as the friction |
  | z | -13 to -19 dB | voicing plus s-like noise |
  | ž | -4 to -8 dB | |
  | đ (friction) | -10 to -13 dB | 3-6.5 kHz |
  | f | -21 to -28 dB | flat, 2-6.5 kHz |
  | h | -14 to -21 dB | peaks at the vowel's formants |
  | v | -16 to -21 dB | F1 300, F2 1130, F3 2400; almost nothing above 500 Hz |
  | l | -5 to -8 dB | F1 455, F2 1150 next to a (1150-1300 next to e, i; 1000-1100 next to o, u), F3 2500, F4 2900 |
  | lj | -4 to -10 dB | F1 335, F2 1800, F3 2820 |
  | j | -1 to -5 dB | F2 1850 between a's (strongly coarticulated) |
  | m, n, nj (murmur) | -4 to -13 dB | F1 300; F2 1150 / 1350 / 1350-1500 |
  | r (between vowels) | -15 dB, dips to -24 dB | F1 400, F2 1400, F3 2550; 50 ms |
  | k burst | -20 dB | on F2 (and 3.5 kHz); voice onset 40 ms |
  | t burst | -20 dB | strongest at 3.5-4 kHz; voice onset 15 ms |
  | p burst | -2 to -20 dB | strongest below 500 Hz, flat floor up to 5 kHz; voice onset under 10 ms |
  | voice bar (b, d, g) | -13 to -25 dB | below 500 Hz; 10-15 dB weaker by the release |
  | vowel /a/ | 0 dB | 2.5-4 kHz about 15-25 dB below the total |

  To repeat a measurement:

  ```bash
  B=build/macos-arm64-release
  LAPRDUS_CLI=$B/laprdus LAPRDUS_DATA=$B \
      python3 tools/formant/analyze.py --voice zvonko "asa" "aša" "ara"
  python3 tools/formant/analyze.py recording.wav
  ```

  Vowel-consonant-vowel items with /a/ are the easiest to read.

### Sibilants

The sibilants deliberately do **not** follow natural speech. Matched to the
recordings (the /s/ centred at 6.4 kHz, 5 dB below the vowels, with energy up
to 11 kHz) they were tiring to listen to. They now follow the classic formant
synthesizers, which ran at 10-11 kHz and had nothing above 5 kHz. Measured
with `tools/formant/sibilants.py` (level relative to the loudest vowel frame,
spectral centroid of the friction):

| | s | š | č | ć |
|---|---|---|---|---|
| Recorded Croatian (MBROLA cr1, 16 kHz) | -6 dB, 5.6 kHz | -1 dB, 4.1 kHz | -1 dB, 4.4 kHz | -2 dB, 4.1 kHz |
| ETI Eloquence (Reed; English, Italian) | -13 to -17 dB, 4.1-4.4 kHz | -13 to -17 dB, 3.1-3.2 kHz | -12 to -13 dB, 3.1-3.2 kHz | - |
| Zvonko before | -5 dB, 6.4 kHz | -6 dB, 3.9 kHz | -2 dB, 4.0 kHz | -2 dB, 4.6 kHz |
| Zvonko now | -11 dB, 4.8 kHz | -12 dB, 3.6 kHz | -8 dB, 3.7 kHz | -8 dB, 4.1 kHz |

Eloquence has no energy at all above 5.5 kHz; Zvonko's /s/ now has about
20 dB less in 6.5-8 kHz than in its peak band. The peaks sit a little above
Eloquence's because an exact match was judged slightly too dull by ear. The distance between s and š is the same
as in Eloquence (about 1.2 kHz), and the order s > ć > č > š is kept. Stojan
and Mirsad differ from this only through their vocal tract length and their
č/ć separation.

Other things taken from the same comparison and from the published
descriptions of Eloquence and of Klatt's synthesizers:

- **Sharp friction edges.** In Eloquence's phone-and-transition model noise
  switches at the edge of the sound; Klatt interpolates noise amplitude over
  5 ms. Friction amplitude used to be smoothed over about 40 ms here, which
  smeared s, š, f into the neighbouring vowels; it is now about 14 ms.
- **Brighter vowels.** Eloquence's vowels carry 5-8 dB more energy in
  2.5-5 kHz than Zvonko's did; the source emphasis was raised by about 3 dB
  at 3 kHz (and the output gain lowered to keep the same peak level).
- **Stronger voice bar.** The murmur during b, d, g was 24-26 dB below the
  vowels, at the weak end of the recorded range (-13 to -25 dB) and far below
  Eloquence (-6 to -10 dB). It is now about -20 dB (falling to -28 dB before
  the release, see "Sharp releases"), so voiced and voiceless stops are
  further apart.
- Bursts of p, t, k before a vowel already matched Eloquence in level and
  length and were left alone (what surrounds them was not right yet, see
  "Stops", and the p burst was later raised towards the recorded one, see
  "Sharp releases"). Klatt's own advice applies: exaggerated cues ("super speech")
  have always tested worse than a closer match to natural data.

### Stops

A listening test found t and k hard to make out at the end of a word and
after s and š (*st*, *sk*, *št*, *šk*). Measured with `tools/formant/stops.py`
on the same items, the bursts before a vowel were fine, but what surrounds
them was not:

| | Recorded Croatian (MBROLA cr1) | ETI Eloquence (Reed) | Zvonko before | Zvonko now |
|---|---|---|---|---|
| Silent gap in *asta*, *aska* | 65-90 ms (timing is eSpeak's) | 40 ms | 20-32 ms | 36-40 ms |
| The same, 1.5 times faster | - | 28 ms (1.6 times) | 12 ms | 24-28 ms |
| The same, twice as fast | - | 12-16 ms (2.9 times) | under 5 ms | 20-22 ms |
| Release of final t (*pat*, *most*) | 40-45 ms, falling from -12/-22 to -36 dB | 90 ms at -22 to -29 dB | 16 ms | 56 ms, falling from -16/-20 to -36 dB |
| Release of final k (*pak*) | 55 ms at -27 to -39 dB | 90 ms at -26 to -30 dB | 30 ms | 64 ms, falling from -16 to -36 dB |
| Strongest part of the t burst | 3.5-4 kHz | 4-5 kHz | 2.5-3 kHz | 3.5-4.5 kHz |
| Burst of final k after s (*disk*) | - | 1-2 kHz (*risk*, *desk*) | 2.5-3 kHz | 1.5-2 kHz |
| First stop of *atka*, *akta* | 24-40 ms at -16 to -28 dB | hardly released | 8-12 ms at -28 to -32 dB | 16-20 ms at -20 to -32 dB |

- **The gap after friction.** A stop that follows s, š, f, h or an affricate
  (*mačka*) is separated from that noise only by its closure. The cluster
  rule shortened it like any other consonant, the smoothing of the friction
  amplitude took another 6 ms off it, and at the rates screen reader users
  work at nothing was left: *st* became *s*. The closure now has a floor of
  its own (44 ms, which gives way to speed only slowly), and neither
  friction nor aspiration reaches into the closure of a voiceless stop.
- **The release before a pause.** With no vowel after it, the release is all
  that is heard of a final stop. It was the 16 ms burst of a stop before a
  vowel; in the recordings the friction at the place of articulation dies
  away over 40-55 ms. It now does the same (a second, slower decay under the
  burst; weaker and flatter for k), together with a little breath through
  the open glottis. The breath is what a released stop has and an affricate
  has not: in both references the release carries energy below 500 Hz only
  5-7 dB under its total, here there was none, and a recognizer heard final
  t as ć (*pet* as *peć*, *brat* as *brać*). It is kept well below the
  references' level (15 dB under the total). The final t and p of *pat*,
  *most*, *plašt*, *top* now measure like the recorded ones in level and
  decay; the final k is still some 8 dB above the recording, as it was
  before. A first version held the friction level and then cut it off; that
  was heard as an affricate too (*Split* as *Splić*).
- **The t burst.** Its strongest component was F3, which put its peak at
  2.7 kHz, exactly where the k burst sits next to e and i (*tek*, *lik*:
  2.5-2.7 kHz). It is now carried by F4 and a fixed peak at 4.3 kHz, as in
  the recordings and in Eloquence. /d/ shares the spectrum; its burst is now
  24-30 dB below the vowels in 3-6 kHz (recorded 26-30, before 27-34).
- **The place of a final k.** A velar takes its place from the vowel after
  it, or, at the end of a clause, from the vowel before it. That vowel was
  used even when another consonant stood in between, so the k of *disk* was
  the fronted one of *ki*, with a burst at 2.7 kHz. Now only a neighbouring
  vowel counts; otherwise the velar is neutral (burst at 1.5-2 kHz).
- **Stop before stop.** The first stop of *tk*, *kt*, *pt* is released
  audibly in the recordings; here it was 8 ms long and 8 dB weaker.
  Eloquence hardly releases it (Finnish *matka*, Spanish *acto*), but the
  recorded speaker is the reference for the language.

Tried and dropped, all three on the k burst before a vowel: lowering it
before e and i to the recorded level (it is 5-10 dB above), damping F1 during
its aspiration, and reducing its F3 share. Each changed the spectrum by a few
dB at most, and a recognizer's errors on k went both ways (*kino* found,
*sok i* lost), so the burst itself stayed as it was.

A speech recognizer (see section 7) served as a check. Whole-sentence error
rates do not show a change to one class of sounds (40 sentences full of them:
15.0% of the characters wrong before, 14.8% after; at 1.5 times the rate
20.2% and 19.9%), so only the words concerned were counted. Of 40 words
ending in t or k at the end of a clause it found 17 before and 24 after, at
1.5 times the rate 11 and 17 (the recorded speaker: 31). Of 68 words with
these sounds anywhere in a sentence it found 37 before and 39 after, at 1.5
times the rate 34 and 36 (the recorded speaker: 46). On the 20 general
sentences of section 7 the three voices stayed within 0.6 points of their
earlier scores, with words flipping both ways.

```bash
B=build/macos-arm64-release
for w in asta aska pat pak; do $B/laprdus -D $B -v zvonko -o $w.wav $w; done
python3 tools/formant/stops.py asta.wav aska.wav pat.wav pak.wav
```

### Sharp releases: p, b, d

A second listening test found p, b and d a little soft, p most of all before
r. `tools/formant/onsets.py` prints the level every 2 ms around a release;
what it showed (levels relative to the loudest part of the item):

| | Recorded Croatian (MBROLA cr1) | ETI Eloquence (Reed) | Zvonko before | Zvonko now |
|---|---|---|---|---|
| p burst in *pa*, first 6 ms | -2 to -5 dB | -32 dB | -21 to -26 dB | -16 dB |
| p burst in *pra* | -8 to -13 dB | -14 to -18 dB | -23 to -28 dB | -14 to -19 dB |
| Its spectrum in *pri*: 0-0.5 / 0.5-1 / 1-1.5 / 1.5-2 kHz | -3 / -6 / -17 / -13 dB | 0 / -17 / -13 / -33 dB | -9 / -4 / -5 / -15 dB | -2 / -8 / -7 / -30 dB |
| After the p of *pra* | noise at -14 to -20 dB until the r | - | nothing | aspiration at -19 to -24 dB |
| *ibi*, *obo*: release to full vowel level | 8-12 ms | - | 40 / 24 ms | 12 ms |
| F1 8 ms after the release of *aba* (vowel: 700 Hz) | at the vowel's value | 485 of 600 Hz (*apa*) | 345 Hz | 470 Hz, 635 Hz 8 ms later |
| Voice bar of *ada* before the release | falls from -22 to -35 dB | -7 to -15 dB, dips to -20 | -20 dB, level | falls from -22 to -28 dB |
| F2 of the vowel of syllabic r in *prvi* | 1330-1380 Hz | - | 1470-1610 Hz | 1310-1450 Hz |

- **The opening.** After a release F1 was held at its low locus through the
  burst and then took another 20-30 ms to reach the vowel, so the vowel
  swelled instead of setting in. In the recordings F1 is more than half way
  there at the voice onset and the level is up within about 10 ms. The
  boundary between a stop's release and the next sound now lies 55% of the
  way to that sound's F1 (25% before), and the rest takes 12 ms. This holds
  for all six stops.
- **The release of b, d, g.** Voicing ran through the 8 ms of the release at
  a third of its strength and as muffled as the voice bar, because the bar's
  spectral tilt was smoothed 8 ms past the release. Now voicing is at 70%
  and the tilt switches at the release; the bar itself weakens over the
  second half of the closure, as it does in the recordings, so the release
  stands out against it.
- **The burst of p and b.** (Revised once more later, see "l, m, n and p
  after the classic synthesizers".) It was 6 dB weaker than now, peaked at 0.7-1.3 kHz
  (a fixed peak plus a large F2 share) and had a second peak at F3: a
  compact mid-frequency burst is what a velar has. It is now strongest below
  500 Hz and falls from there, with a flat floor (the click).
- **p, t, k before r.** (For p this was replaced later, see "l, m, n and p
  after the classic synthesizers".) A stop was aspirated only before vowels,
  glides and l. An r opens with a vocalic stretch, so the stop is released into it in
  the same way; without the aspiration the p of *pra* was a bare 8 ms burst.
- **Which sound colours a stop.** A stop took its formants from the next
  full vowel, however far away. In *pri* that put the F2 share of the p
  burst at 1.7 kHz, where t and k have theirs. A stop is now coloured by the
  sound it is released into: the r, l, j, v or nasal that follows, or the
  vowel of a syllabic r.
- **Syllabic r.** Its vowel was pulled a third of the way towards the
  neighbouring full vowels; before an i that gave F2 1600 Hz, an e-like
  vowel, and with the rising F2 the p of *prvi* was heard as t (*tervi*).
  The pull is now a tenth.

The same recognizer check as above, on 40 sentences full of b, d and p (120
words containing them, 41 with pr, br or dr): it found 65 of the 120
words before and 78 after, at 1.5 times the rate 45 and 56 (the recorded
speaker: 90), and of the 41 words 20 and 25, at 1.5 times the rate 12 and 17
(the recorded speaker: 29). The character error rate of these sentences went
from 13.9% to 10.8% (the recorded speaker: 11.2%), at 1.5 times the rate from
22.1% to 18.4%. The sets of the t and k round held: words with those sounds
inside a sentence 39 before and 43 after, clause-final words 24 and 21 (17
and 17 at 1.5 times the rate), which is inside the noise of that count. Words
with g, which shares the changes to the release, went from 25 to 32 of 53
(*Zagreb* is now found in all three voices). On the 20 general sentences of
section 7 Zvonko, Stojan and Mirsad moved by +0.8, +0.1 and -1.4 points.

```bash
B=build/macos-arm64-release
for w in pa pra aba ada; do $B/laprdus -D $B -v zvonko -o $w.wav $w; done
python3 tools/formant/onsets.py pa.wav pra.wav aba.wav ada.wav
```

### l, n and č

A third listening test asked for three things: an l that sounds Croatian and
Serbian rather than Slovenian, an n that is less like m, and a č that is
further from ć.

**l.** macOS has a Croatian, a Serbian and a Slovenian voice of the same kind
(Lana, Dragana, Tina), which shows what the difference is: in *ala* the
Slovenian l keeps F2 at 1650-1790 Hz, as high as the vowel's, while the
Croatian and the Serbian one drop it by 250-450 Hz (to 1040-1070 and
1170-1200 Hz). The recorded male speaker has a dark l in every context. F2 of
the l:

| | *ala* | *ili* | *ele* | *li* | *il* |
|---|---|---|---|---|---|
| Recorded Croatian (MBROLA cr1) | 1140 | 1280 | 1220 | 1170 | 1200 |
| Zvonko before | 1180 | 1460 | 1400 | 1460 | 1460 |
| Zvonko now | 1130 | 1270 | 1240 | 1270 | 1260 |

Next to a the old l was right; next to e and i it took a third of its colour
from the vowel and came out 200-300 Hz too bright, half way to the Slovenian
one. Its own F2 is now 1100 Hz and the vowels move it half as much. The
recordings also show F2 falling into an l within 25 ms and leaving it over
60-80 ms; the transition into l is now twice as fast as the one out of it.

**n.** (The values chosen here were replaced in the next round, see below;
the finding that a bright murmur is heard as l stands.) A recognizer heard n
as m in 4 of 76 words at the normal rate and in 7
at 1.5 times the rate (*stan* as *stam*, *vani* as *vami*, *nije* as *mije*).
The two murmurs were the same dull hum, 12 dB below the vowels (recorded: 8
to 10, Eloquence 6 to 7), with their upper resonances 23-38 dB down, so only
the short transitions told them apart. What was tried, with the number of
n-words found and how the others were heard:

| n murmur | found (of 76) | as m | as l |
|---|---|---|---|
| before: F2 1350, F3 2400 Hz, bandwidths 280/300 Hz, level 0.62 | 55 | 4 | 1 |
| F2 1450, F3 2550, bandwidths 150/180, 4 dB less tilt, level 0.85 | 52 | 0 | 10 |
| F2 1600, F3 2600, bandwidths 220/250, 2 dB less tilt, level 0.75 | 54 | 0 | 7 |
| old murmur, level 0.80, stiffer F2 locus | 59 | 4 | 1 |
| **F2 1450, F3 2550, bandwidths 220/250, level 0.75** | **60** | **0** | **3** |

A murmur as bright as the recorded one (its 1-1.5 and 2-3 kHz bands are only
13-16 dB down) is no longer taken for m but for l: in a formant synthesizer a
sonorant with clear upper formants *is* an l. The setting kept raises the
resonances of n a little, narrows them a little and makes all nasal murmurs
2 dB louder; m keeps its dull spectrum, as in Eloquence, where the murmur of
n has 15 dB more energy at 1.5-3 kHz than that of m. At 1.5 times the rate it
finds 46 words instead of 36 (as m: 4 instead of 7). The recorded speaker: 68.

**č.** For Zvonko the friction of č and ć had the same peak and centroids 400
Hz apart (3.55 and 3.95 kHz); Stojan's are 920 Hz apart, Mirsad's 810 Hz, the
Croatian voice Lana's 550 Hz (the recorded male speaker merges them, as many
Croatian speakers do). `hard_palatal_shift` could not be used, because it
moves š and ž as well. A new speaker property, `hard_affricate_shift`, lowers
the friction of č and dž alone: 0.93 for Zvonko, which puts č at 3.3 kHz, 670
Hz below ć, with its strongest band now 2-3 kHz instead of 3-4 kHz. ć, š and
the other two speakers are unchanged.

The recognizer check on 40 sentences full of n, m and l: for Zvonko it found
60 of the 76 words with n instead of 55 (46 instead of 36 at 1.5 times the
rate), 35 of the 51 words with l instead of 30 (28 instead of 25) and 27 of
the 35 words with m instead of 24; the character error rate went from 12.2%
to 8.7% (the recorded speaker: 10.4%), at 1.5 times the rate from 21.2% to
19.8%. For Stojan: 52 words with n instead of 45, heard as m 2 instead of 8.
The earlier sets held (words with b, d, p 78 before and 80 after; with t, k
43 and 43; clause-final 21 and 21), and on the 20 general sentences of
section 7 Zvonko, Stojan and Mirsad moved by -0.9, 0.0 and -0.6 points.

### l, m, n and p after the classic synthesizers

After the three rounds above l, m, n and p were still judged soft, and hard
to make out at high rates; the request was to follow Eloquence, DECtalk,
Orpheus and eSpeak. Eloquence and eSpeak are installed on macOS (`say -v
"Reed (Italian (Italy))"`, `say -v "(null) - ESpeak (Croatian (Croatia))"`,
with `-r 300` or `-r 450` for high rates); for DECtalk there are Klatt's
published tables. Segment durations at high rates turned out to be the same
as Eloquence's. What differs is the manner: the classic synthesizers switch
these sounds on and off within a few milliseconds and make them very unlike
the vowels around them, where the recorded speaker, and Zvonko after him,
glide.

| | Eloquence | eSpeak (Croatian) | DECtalk (Klatt 1980) | Zvonko before | Zvonko now |
|---|---|---|---|---|---|
| F1 of l in *ala* | 310-330 Hz | 390-430 Hz | 310 Hz | 490 Hz | 370 Hz |
| Energy above 1 kHz in that l | -24 to -40 dB | - | - | -12 to -24 dB | -20 to -32 dB |
| Second resonance of the n murmur | 1700 Hz | 1620 Hz | 1340 Hz | 1450 Hz | 1650 Hz |
| Second resonance of the m murmur | 1000 Hz | 1080 Hz | 1270 Hz | 1150 Hz | 1050 Hz |
| F1 from vowel to murmur in *ana* | within 10-20 ms | within 10 ms | - | over 30-40 ms | within 10 ms |
| Murmur level | -6 to -7 dB | -15 to -17 dB | - | -10 to -11 dB | -8 to -9 dB |
| p burst in *pa* | 10-16 ms at -30 dB, flat up to 5 kHz | 18 ms at -17 to -26 dB, falling from 500 Hz | flat (bypass path) | 4 ms at -12 dB, then aspiration into the vowel | 8 ms at -17 to -24 dB |
| The same burst before every vowel | yes | yes | yes | no (followed F2-F4) | yes |
| Between burst and voice | 6 ms of silence | 10 ms of silence | - | aspiration | about 6 ms near silence |

- **l.** F1 is now 330 Hz (the recorded speaker has 450 Hz, and that value
  stays in the table of recorded values above). The deep and quick dip of F1
  is what sets an l off from the vowels; with the shallow one a recognizer
  heard *mali Luka voli* as *valio uka volio*, the l as part of the vowels.
  The lower F1 also takes 8 dB off the upper formants, as in Eloquence. F2
  stays where the previous round put it (a dark l), which is also where
  eSpeak's Croatian l is (1360-1410 Hz in *ili*; Eloquence's Italian l is a
  clear one, 1750 Hz).
- **m and n.** The two murmurs differ by their second resonance, and the
  classic synthesizers put those further apart than a speaker does; so do
  the tables now. F1 of the vowel no longer slides down towards the nasal
  (its boundary value follows the vowel by 70% instead of 35%), nasality is
  smoothed over 6 ms each way instead of 16, and bandwidths over 6 ms instead
  of 10, so the murmur starts and stops at its edge. The murmur of n went
  back to wide bandwidths with 3 dB more tilt: with the edges sharp, the
  duller murmur was the better one (n found in 64 of 76 words against 61, and
  heard as l in none instead of 3). All murmurs are 1.5 dB louder.
- **p.** Three versions were compared at 1.7 times the rate, where the
  complaint was: the previous burst (strongest below 500 Hz, following the
  formants, with aspiration), a flat one as in Eloquence, and one that is
  strongest below 500 Hz with a flat floor 10 dB under it, the same before
  every vowel, followed by silence instead of aspiration. A flat burst of 8
  ms is a different sample of noise each time and now and then peaked at 3-4
  kHz, where a t burst is (*prema* heard as *trema*). The third version was
  the best (words with p found: 25 of 58 against 22 and 21; with pr, br, dr
  13 of 41 against 10 and 6) and is the one kept. /b/ shares the burst.

The recognizer check, Zvonko, on the 40 sentences full of n, m and l and the
40 full of b, d and p, at the normal rate and at 1.7 times the rate (words
found, before and after):

| | normal rate | 1.7 times |
|---|---|---|
| words with n (76) | 59 → 63 | 33 → 43 |
| n heard as m / as l | 0 / 3 → 0 / 0 | 4 / 1 → 0 / 2 |
| words with m (35) | 27 → 29 | 16 → 20 |
| words with l (51) | 35 → 43 | 27 → 29 |
| words with b, d, p (120) | 78 → 84 | 43 → 48 |
| characters wrong, n/m/l sentences | 9.0% → 8.0% | 25.4% → 23.3% |
| characters wrong, b/d/p sentences | 10.5% → 9.9% | 25.7% → 23.2% |

The recorded speaker finds 68 words with n and 41 with l at the normal rate,
so l is now level with him. For Stojan at 1.7 times the rate: words with n 25
→ 36, with m 15 → 19, with l 25 → 20 (the one count that fell), characters
wrong 29.8% → 27.6%. The earlier sets held (words with t, k 43 → 46;
clause-final 21 → 22), and on the 20 general sentences of section 7 Zvonko,
Stojan and Mirsad moved by -1.4, -0.9 and +2.0 points (Mirsad's losses are
word boundaries, *padala kiša* as *pada lakiša*, and one lost p in *ptice*).

### p and b once more

After the rounds above p and b were still judged not noticeable enough, with
Eloquence, DECtalk and eSpeak named as the reference. On the Mac there are
Eloquence (`say -v "Reed (Italian (Italy))"`), eSpeak (`say -v "(null) -
ESpeak (Croatian (Croatia))"`) and a third-party Klatt-type Croatian voice
(`say -v "(null) - Klatt (Croatian (Croatia))"`, the nearest thing to
DECtalk); the envelopes of *apa*, *aba*, *ipi*, *ibi*, *upu*, *ubu* every 2 ms
(whole signal and above 1 kHz, relative to the loudest 3 ms) were compared at
the normal rate and at 1.7 times it (`say -r 300`, `laprdus -r 1.7`). It was
not the burst's level, as the two earlier rounds had assumed: the classic
synthesizers make a stop out of sharp events, and Zvonko glided through
them. The three synthesizers do not agree with each other, though, and the
first attempt followed the wrong two.

| | Eloquence | eSpeak (Croatian) | Klatt voice | Zvonko before | Zvonko now |
|---|---|---|---|---|---|
| Vowel cut off before p (-8 to -60 dB) | 16 ms | 8-16 ms | within 2 ms | 22-26 ms | 16 ms |
| p release in *apa* | 22-28 ms of flat noise at -31 to -45 dB, into the vowel | 18 ms burst at -17 to -25 dB, then 12 ms of silence | 20 ms burst at -3 to -21 dB, then 10 ms of silence | 8 ms click from -20 dB, then the vowel | 24 ms: the click, then breath at -20 to -27 dB, into the vowel |
| The same at 1.7 times the rate | 16 ms | 18 ms, 2 ms of silence | 18 ms, 2-4 ms | 8 ms | 16 ms |
| Voice bar of b in *aba* | -6 to -20 dB, steady, nothing above 1 kHz | -16 to -43 dB, pulsing | -26 to -46 dB, weak | -21 to -27 dB, fading 7 dB | -13 to -21 dB, fading 4 dB |
| After the b release | a 24 ms rise | voice off under the burst, then on within 6 ms | burst, 8 ms of silence, vowel | burst under the voicing, 20 ms rise | burst, vowel within 10 ms |

- **The closing edge.** The voice stopped within 8 ms, but F1 kept the
  vowel's 70 Hz bandwidth through the closure and rang on for 20-25 ms (4 dB
  per 2 ms, the decay of that bandwidth), so the vowel faded into the stop.
  The closure of p and b now damps F1 (bandwidth 250 Hz; the lips are shut),
  and the vowel is cut off within 16 ms, as in Eloquence; F1 also moves into
  the closure of p and b within 14 ms instead of gliding over 36. (Applied to
  every stop, the fast F1 close did nothing for p and b and cost the
  recognizer a few t and k words, so t, d, k and g are untouched.)
- **The p release follows Eloquence**, not eSpeak: the click (14 ms, 2 dB
  stronger than before) and then breath through the opening glottis for the
  rest of 24 ms (16 ms when fast), running straight into the vowel or the r
  of *pra*. Before, p had no aspiration at all and its click was over in 8
  ms. The first attempt copied eSpeak and the Klatt voice instead, an 18 ms
  flat burst and then 8-10 ms of silence before the voice; the recognizer
  lost one word in ten with the flat burst (not with its level: a 14 ms
  burst at the new level cost nothing), and the user heard the silence as a
  gap in the word. Both are gone.
- **b** has the loud, steady voice bar of Eloquence and DECtalk (voicing at
  95% instead of 60%, 4 dB less tilt, fading by 4 dB over the second half
  of the closure instead of 7), and after both labials the vowel snaps in
  (F1 72% of the way at the boundary and the rest within 8 ms, against 55%
  and 12 ms for the other stops). A version that held the voicing back under
  the b release to 20%, so that the burst stood out as in eSpeak, was no
  better for the recognizer and no better to the ear; the burst now runs
  under voicing at 70% as before, 2 dB stronger. d and g keep the recorded
  speaker's weaker bar.

The recognizer check (section 7; Whisper "small", 40 sentences full of p
and b, 187 words containing them; synthesis is deterministic, so a repeated
run gives the same count and differences are between engines, not runs):
at the normal rate it found 156 of those words with the previous engine and
148 now (6.6% and 7.3% of the characters wrong), at 1.7 times the rate 93
and 83 (17.3% and 19.1%). On 20 sentences full of t and k (77 words) it
finds 55 against 56 before. The intermediate version with the eSpeak-style
burst and gap scored 152 and 87, and its 18 ms flat burst 141 and 78. The
recognizer is a guard against gross regressions, not the judge: the sound
the user asked for is that of the classic synthesizers, which it hears worse
than the recorded speaker too (section 7).

```bash
B=build/macos-arm64-release
for w in apa aba ipi ibi; do $B/laprdus -D $B -v zvonko -o $w.wav $w; done
python3 tools/formant/onsets.py apa.wav aba.wav ipi.wav ibi.wav
```

### Voice colour

Vowels of the same sentence (*papa*, *pipi*, *pupu*... in Italian for Reed,
in Croatian for Zvonko) were compared with `tools/formant/analyze.py` and a
long-term spectrum. Three differences were systematic and were taken over:

- **Formant bandwidths.** Eloquence's F1-F3 bandwidths measure 100-140 Hz;
  Zvonko's measured 20-100 Hz, and the narrow ones rang (the harmonic at F1
  of *i* stood 13 dB above its neighbours, in Reed 3 dB). The vowel
  bandwidths in `formant_phonemes.cpp` were widened by 20-30 Hz.
- **A fixed resonance near 3.9 kHz.** In every Eloquence vowel there is a
  peak at 3.8-3.9 kHz, the first of its fixed upper formants (the synthesizer
  runs at 11025 Hz). Ours sat at 4.4 kHz; F5 is now 3950 Hz and the others
  were moved down with it (`VoiceQuality::upper_f`). This also moved energy
  from 4-5 kHz, where Zvonko had 4-8 dB more than Reed, to 3-4 kHz, where it
  had less.
- **Nothing above 6 kHz.** Eloquence's vowels have no energy above 5.5 kHz;
  ours carried source brightness and breath noise up to 11 kHz, about 40 dB
  below the vowel peak, a hiss the classic voices never had. The voiced path
  now has a second-order low-pass at 6 kHz (`VOICE_CUTOFF`); the frication
  path keeps its own 6.6 kHz filter.

The same comparison showed the output limiter at work on plain vowels: the
formant voices were 11 dB louder than Reed and the loudest /a/ frames hit the
soft limiter, which put distortion products at 6-11 kHz. The output gain was
lowered from 0.39 to 0.28 and the limiter knee raised to 0.8, so limiting now
touches about 0.02% of the samples of a sentence (Zvonko is still a little
louder than Reed and about 7 dB quieter than the recorded Josip, whose
recordings clip).

Not taken over: Eloquence's default male pitch (90 Hz in the Italian Reed;
the user can set it), its complete lack of breathiness, and its sibilants
(see above).

The reference voices are part of macOS:

```bash
say -v "Reed (Italian (Italy))" -o assa.wav --data-format=LEI16@22050 "assa"
say -v "Reed (English (US))"    -o asha.wav --data-format=LEI16@22050 "asha"
python3 tools/formant/sibilants.py assa.wav asha.wav
```

## 4. Text to sounds (`formant_frontend.cpp`)

- Serbian Cyrillic is transliterated first.
- **Digraphs** lj, nj, dž are single sounds, except across a few prefix
  boundaries (*nadživjeti*, *injekcija*).
- **ije** as the long reflex of jat is one syllable, [i̯eː] (*lijep*,
  *mlijeko*). It stays two syllables at the end of a word (*nije*, *prije*),
  in verb and comparative endings (*pijem*, *starijeg*) and in loans
  (*klijent*).
- **Syllabic r**: r with no vowel next to it (*prst*, *Hrvatska*, *žanr*),
  but never before j: *rj* is the short jat after r (*rječnik*, *rješenje*,
  *pogrješka*). Until October 2026 these words got a syllable on the r,
  which then also took the accent.
- **Assimilation** inside words and clitic groups: voicing between obstruents
  (*predsjednik* → [pretsjednik], *s bratom* → [zbratom]), s/z before
  postalveolars (*s čim* → [ščim]), n before k/g, merging of identical
  consonants (*bez zuba*). /v/ and the sonorants neither cause nor undergo
  voicing assimilation, and there is no final devoicing.
- **Abbreviations** with no vowel and short all-caps tokens are spelled out
  (HR, USB, NVDA). None is expanded (since October 2026; *dr.* was *doktor*,
  *npr.* *na primjer*, *km* *kilometara* before): they are read as written,
  and a word whose only syllable would be a final *r* (*dr, mr, str, npr*)
  is spelled, as no native word ends in a syllabic r after consonants alone
  (*prst, vrt, rt* keep theirs). The colon of a web address (*https://*) is
  silent, like the slashes.
  The prepositions *s* and *k* are the exception: before another word they
  are words (*s njom*, *k meni*), alone they are letters. A capital *S* or
  *K* is the preposition where a capital says nothing, at the head of a
  sentence or after an opening quote or bracket (*S tobom sam htio sve*,
  „*S tobom*“) and in all-caps text (*ALI SAM S NJOM*), and the letter
  anywhere else (*Mercedes S klasa*, *pritisnite S za spremanje*). A line
  starts a sentence too, since a line break ends the clause (section 1,
  `analyze_text`): in a post VoiceOver reads as "prije 18 h, Javno⏎S ponosom
  predstavljam" the *S* used to sit mid-clause and was read *es*. Before an
  instrumental (*-om, -em, -im, -ima, -ama, -lju, -šću*) a capital *S* is
  the preposition anywhere, for hosts that join lines with a space (*Javno
  S ponosom*, *razgovarao sam S Ivanom*); no letter S stands before one,
  while *Mercedes S klasu* keeps the letter. Before a hyphen (*S-klasa*), a
  single letter (*S i M*) or a clitic (*S je slovo*, *slovo s je*) it is
  always the letter, since no preposition stands there. Spelling and typed
  characters do not go through this: they are named by the spelling code.

### Stress

Stress is lexical in these languages and cannot be derived from spelling.
The front end tries, in this order:

1. accent marks in the text itself (`telèfon`, `gláva`, `kȕća`, `grȃd`, `ā`),
2. the built-in lexicon (exact form, then stems),
3. suffix rules for loans and derived words (*-irati*, *-acija*, *-izam*,
   *-ura*, *-tika*, *-ator*, *-itet*, penultimate stress for *-ent*, *-ist*,
   *-fon* ...),
   for surnames in *-ović/-ević* of four or more syllables (stress on the
   syllable before the suffix: *Jovánović*, *Kováčević*, *Milénković*; the
   three-syllable *Pètrović*, *Màrković* keep the first), for feminine agent
   nouns in *-ačica* (*pjevàčica*) and for nouns in *-ina* of four or more
   syllables (*veličìna*, *Katarína*, *balerína*; *-ovina/-evina* on the
   syllable before: *dòmovina*, *králjevina*),
   then the word-class rules described under "Word classes" below (nouns
   with a long last stem syllable, prefixed verbs, verbs in *-ovati*, nouns
   in *-ica* and *-anin*, adjectives in *-izan/-ozan*),
4. the first syllable, the most common position in Neo-Štokavian.

The name and *-ina* rules and the lexicon entries for names, surnames and
towns were checked against the accented headwords of the Serbo-Croatian
entries of Wiktionary (which follow the standard dictionaries) for some 500
words in October 2026; the exceptions found there (*Ìvanović*, *Jòsipović*,
*gȍdina*, *ȉstina*, *svȉnjetina*, *Vȍjvodina*, *Kràgujevac*, *Varàždīn*...)
went into the lexicon. Croatian and Serbian differ for a few of them
(*dìrektor* / *dirèktor*, *Vȍjvodina* / *Vojvòdina*, *Ùkrajina* /
*Ukrajína*), which the language tables handle. Personal names are the least
predictable part: the Croatian name dictionary gives *Ìvana* and *Ivȁna*,
*Màrija* and *Marȉja* side by side, so a name outside the lexicon gets the
first syllable.

The standard's restrictions are applied afterwards: monosyllables are
falling, non-initial stress is rising.

**Clitics** (prepositions, conjunctions, short pronoun and verb forms) are
unstressed and lean on a neighbour. An enclitic cannot open a clause, so
*ti*, *je*, *mi* are full words there. *Ne* takes over the accent of a
following monosyllable (*nè znam, nè dam*) and of every present form of
*znati* (*nè znamo, nè znate, nè znaju*), in Serbian and Bosnian also of
common two-syllable verb forms (*nè mogu*). The two words are then one word
in timing as well: *ne znamo* is synthesized exactly like *nèznamo*. Zvonko
drops the length the dictionaries keep after the accent (*nè znām*), which is
not heard in Croatian as commonly spoken; Stojan and Mirsad keep it. Bosnian additionally moves the accent
onto prepositions before a fixed list of words (*ù grād*, *nà more*).

To correct a word, add an entry to `formant_lexicon.cpp` (notation at the top
of the file). Language-specific tables override the common one (*pròfesor* in
Croatian, *profèsor* in Serbian and Bosnian).

The Croatian *pobjednik* and *pobjednički* paradigms were checked on
8 October 2026 against [HJP's *pòbjednīk*](https://hjp.znanje.hr/index.php?show=search_by_id&id=eVxmXxU%3D)
and [*pòbjedničkī*](https://hjp.znanje.hr/index.php?show=search_by_id&id=eVxmXxM%3D).
Both retain short rising *o*: *pòbjednik, pòbjednika, pòbjedniku,
pòbjednikom, pòbjedniče, pòbjednici, pòbjednicima, pòbjednike* and
*pòbjednička, pòbjedničko, pòbjedničkoj, pòbjedničkima*, etc.
Exact noun and adjective paradigms prevent suffix rules from moving the
accent onto *-jed-* or *-nik-* and do not reach other derivatives.
As elsewhere in Croatian, the entries omit unstressed lengths.

The Croatian entries for *sučelje* and *otići* were checked on 8 October 2026.
[HJP gives *súčēlje*](https://hjp.znanje.hr/index.php?show=search_by_id&id=d1pjWBU%3D),
with long rising *u*; its exact case forms share that accent.
[HJP gives *òtīći, òtišao, òtišla*](https://hjp.znanje.hr/index.php?show=search_by_id&id=eFdmXhI%3D),
with short rising *o*, agreeing with
[Školski rječnik's *òtīći, òtišao*](https://rjecnik.hr/search/?q=oti%C4%87i&strict=yes).
The irregular participle is listed as *òtišao, òtišla, òtišlo, òtišli, òtišle*,
alongside the past adverb *òtišāvši*, rather than guessed by a suffix rule.
These exact entries do not reach *sučèliti* or the imperative *otíđi*.
Unstressed lengths follow the Croatian voice's existing policy, so it says
*súčelje, òtići, òtišavši* while preserving the stressed vowel's length and tone.

The full noun paradigms of *sat* and *rad* were checked on 8 October 2026
against [Školski rječnik: sat](https://rjecnik.hr/search/?q=sat&strict=yes)
and [rad](https://rjecnik.hr/search/?q=rad&strict=yes), cross-checked with
[HJP: sat](https://hjp.znanje.hr/index.php?show=search_by_id&id=dldvWhI%3D)
and [rad](https://hjp.znanje.hr/index.php?show=search_by_id&id=dl1vXhE%3D).
The singular has *sȃt, sȃta, sȃtu, sȃte, sȃtom* and
*rȃd, rȃda, rȃdu, rȃde, rȃdom*, except for locative *sátu, rádu*.
The short plural has N/V *sȃti*, G *sátī*, A *sȃte*, D/L/I *sátima*
(also discussed by [Kapović, Filologija 54 (2010)](https://hrcak.srce.hr/file/77788)).
The expanded plurals have short falling accents: *sȁtovi, sȁtove,
sȁtovima, sȁtōvā* and *rȁdovi, rȁdove, rȁdovima, rȁdōvā*.
Croatian continues to omit unstressed lengths (*sáti, sȁtova, rȁdova*).

These are exact forms, so they do not spread to unrelated derivatives.
The front end selects locative *sátu/rádu* after *u, na, o, po, pri*, also
across up to two adjacent modifiers with dative/locative endings
(*o svom novom rádu*). It selects genitive *sáti* after numbers from five
upward, common quantity/genitive cues (*nekoliko sáti, raspored sáti*),
an adjacent *-ih* modifier (*do kasnih sáti*), and in *koliko je sáti*.
These are limited context heuristics, not a syntactic parser: without such
cues the defaults remain dative *sȃtu/rȃdu* and nominative *sȃti*.
Written accents and user accent entries take precedence over these rules.
The homographic adjective *rȁd* ‘willing’ and personal names require an
explicit accent or user entry. The noun's vocative *rȃde* also matches the
stressed vowel of present *rȃdē* ‘they work’; the rest of *raditi* is a
separate verb paradigm.

**Word classes** (`StressRules::strong()`, after the suffix rules). The
single suffixes above cover a few hundred words each; the rules below cover
whole classes of words by their shape. Each covers an accent pattern that
the dictionaries give regularly, and is kept only where its shape is
reliable; where the same letters are also an inflection of common native
words, the rule is limited or left out, and the words that still collide
are lexicon entries.

- **Nouns with a long last stem syllable** (`long_stem()`): the loans
  (*telefon, restoran, kapetan, rezultat, aparat, programer, inženjer,
  kabinet, piramida, analiza, čokolada, motiv, Francuz*) and the derived
  nouns in *-ač, -ar, -aš, -njak, -ljak, -enik, -anac, -inac* (*prodavač,
  čuvar, novinar, košarkaš, stručnjak, zemljak, učenik, zarobljenik,
  Amerikanac, Dalmatinac*). In every case form the long vowel carries a
  rising accent, in all three voices: *telefóna, restorána, rezultáta,
  programéra, prodaváča, čuvára, zemljáka, učeníka, učeníci, Amerikánca*;
  so do the possessives (*čuvárov, zemljákova*). In the nominative, where
  that syllable is the last one and cannot carry a rising accent, the
  dictionaries move the accent one syllable back and leave the length
  behind it: *telèfōn, restòrān, prodàvāč, čaròbnjāk, inžènjēr, vitàmīn*.
  Stojan and Mirsad say exactly that; Zvonko leaves the unstressed length
  out (*restòran, prodàvač, čaròbnjak*), as with *sìgnal* and *kòntrolni*
  above. The nouns in *-enik/-anik* keep the accent of their participle in
  the nominative, the syllable before the suffix (*ùčenīk, pòslanīk,
  zaròbljenīk, zapòslenīk, osigùranīk*). The nouns in
  *-anac, -inac, -unac* keep the long vowel in the nominative too, because
  the fleeting *a* makes it the second-to-last syllable (*Amerikánac,
  Dalmatínac, bjegúnac*).

  The shapes and their limits: *-ač, -až, -ar, -ir, -un, -aš* always
  (*-aše* only from three syllables, because *gledaše, imaše* are the
  imperfect); *-on, -at, -ad, -iz* not after a verbal prefix (*zákon,
  nàslon, pòznat, zàhvat, nágrada, západ, ùgriz* keep the first syllable)
  and *-at* not before *-e/-i*, which are the present of every verb in
  *-ati* (*imate, gledati*); *-an* only with two syllables before it, not
  after a prefix and not after *-av-, -iv-, -ir-*, because of the passive
  participles (*pȍslana, pròdāvāna, planírana*) and the names (*Ìvana,
  Stjȅpana*); *-er, -et, -id* with two syllables before them (*programéra,
  kabinéta, piramída*, but *jȅzera, vȅčera, sȅrvera, prȅdmeta, dȅteta,
  ȕvida*), *-et* not before *-e* (*odaberete*); *-ak* only as *-njak,
  -ljak*, with the plural *-njaci* (*jùnāk, čàrdak, pȅtak* go both ways);
  *-ik* only as *-nik* after
  a vowel in a prefixed word (*rȁdnīk, pȕtnīk, kȍrisnīk* keep the first
  syllable; *spȍmenīk, zàmjenīk, ùdžbenīk* are lexicon entries); *-iv* only
  as *-tiv, -hiv, -siv* (not the adjectives in *-ljiv* and *náziv, pȍziv*);
  *-al* only before *-u, -om, -ima* and in the nominative (*kanálu,
  generálom, festìval*), never before *-a, -i, -o, -e*, which are the
  participles (*gledala, čitali*); *-uz* not before *-i, -e* (*zȁdruzi,
  ȕsluzi*); *-in* only in the nominative of longer words whose letter before
  the suffix is not *n, c, č, k, j* (*vitàmin, magàzin*; the possessives
  *Ìvanin, kràljičin, bȁkin, Mȁjin* are not touched) and in *-inima* and
  the possessives (*vitamínima*); *-itak, -utak* in the oblique cases
  (*dobítka, trenútka*). The *e* of *ije* never counts (*svȉjeta,
  cvȉjeta*).
- **Prefixed verbs** (`prefixed_verb()`): a word that begins with a verbal
  prefix, has a thematic vowel (*a, i, u*, or *je*) before one of the
  verbal endings and at least one root syllable between the two gets the
  accent on the syllable before the thematic vowel: *poglèdati, poglèdala,
  poglèdavši, poglèdasmo, poglèdam, poglèdate; napràvio, dočèkala,
  iskùsio, zahvàlio, pokùšamo, požèljeti*. The long infinitives already had
  that syllable from the antepenult rule; the other forms now follow the
  infinitive, as the verbs in `VERBS` do by name. The endings taken are the
  infinitive, the l-participle, the aorist and imperfect, the two verbal
  adverbs and the present in *-am, -aš, -amo, -ate*. The third person
  *-a/-aju*, the imperative in *-aj*, the present in *-im/-i/-e* and the
  passive participle in *-an* are left out: they are spelled like too many
  nouns and adjectives (*pȍkušāj, dȍgađāju, dòsljednim, pȍseban, dȍsadan,
  pȍznat*). (*ȍgledalo*, which looks like *dočekalo*, is a lexicon entry.)
- **Verbs in *-ovati*** (`ov_verb()`): the infinitive, the participle and
  the aorist have the accent on the *o* (*kupòvati, putòvao, stanòvala,
  darovàsmo*), next to the present in *-ujem* above. *Vjȅrovati, rȁdovati
  se, mìlovati, pȍštovati* keep the first syllable and are lexicon entries.
- **Nouns in *-ica*** (`ica_noun()`) of four or more syllables built on
  *-ar, -on, -un, -ovn, -ern, -arn, -telj* have the accent on that syllable
  (*čuvàrica, kuhàrica, radiònica, račùnica, putòvnica, cvjećàrnica,
  ravnatèljica, voditèljica*), like *-ačica*. The others are left alone,
  because many keep the first syllable (*pȍzornica, ȉskaznica, slȕžbenica,
  djèvōjčica*).
- **Inhabitants in *-čanin, -ćanin, -đanin, -šanin, -žanin, -janin,
  -ljanin, -njanin*** (`anin_noun()`): the accent is on the syllable before
  the suffix in the singular cases that keep the *-in-* (*gràđanin,
  Rìmljanin, kr̀šćanin, držàvljanin, Lìčanin*).
- **Adjectives in *-izan, -ozan*** (`izan_adjective()`): *precìzan,
  koncìzna, nervózan, religiózni, grandiózno*.
- **Negated participles** (`form_of()` with `negated`): *ne* + a passive
  participle of a verb from `VERBS` or `IJE_VERBS` is an adjective with the
  accent the dictionaries give the participle, in every voice: *nepròčitan,
  nedòvršen, nepòvezan, nedòkazan, nenàplaćen, neìspunjen, neòtvoren,
  neùređen, nepròmijenjen* (one syllable before the root vowel, as in
  *pròčitān, dòvršen, pòvezān*), *neprèslušan, neprègledan, nezàustavljen*
  where the verb already carries its accent on its prefix, and *nȅplaćen,
  nȅpisan, nèsrēđen* for an unprefixed verb. Zvonko makes no exception
  here: the negated forms are adjectives, not verb forms, and Croatian
  does not say *nepročìtan*. The lexicalized adjectives that put the accent
  on *ne-* against this pattern (*nȅpregledan, nȅpoznat, nȅpotreban,
  nȅprekidan*) are lexicon entries or come out of the default. The rule for
  nouns with a long last stem syllable skips every *ne* + prefixed word
  (`negated_prefixed()`), so *nepoznata, nepotpisana, nepotrebna,
  neutralna* no longer get a loan's long syllable (*nepoznáta*).
- The **-ina** rule now starts at four syllables and skips the possessives
  of nouns in *-ica, -ka, -ja* (*kraljičina, bakina, Majina*), the
  inhabitants (*građanina*) and the augmentatives in *-etina, -urina,
  -ština, -avina, -čina*, which have the accent on the syllable before the
  suffix (*kućètina, ptičùrina, čakàvština, mješàvina, junàčina*). The
  common three-syllable nouns (*brzìna, planìna, širìna, ravnìna, trećìna,
  čistìna*) are lexicon entries, because the rest of that shape are mostly
  possessives and nouns with the first syllable stressed (*mȁmina,
  sȅstrina, Ȁnina, nȍvina, mȁlina, Tùrčina*). The rule for *-ija* also
  covers the possessives (*Màrijin*).

A few verbs whose forms are spelled like the case forms of these nouns went
into `VERBS` so that the verb reading wins (*ùdara, ìspune, ùmire, ìskače,
spȁda*), and *računa, izračuna, obračuna* carry the noun-twin flag, so that
*računa* alone is the genitive of *ràčūn* (*račúna*) and the verb only at
the head of a clause.

**Users can do the same without recompiling.** `accents.json` next to the user
dictionaries (`%APPDATA%\Laprdus`, `~/.config/Laprdus`, the Android and Apple
dictionary directories) holds entries in the same notation, as JSON:
`{ "word": "kontr'o:l*" }`, `{ "word": "sign'a:l|a|u|om" }`,
`{ "verb": "ur'e:d=i<p" }`, `{ "verb": "dijel:i" }`, each optionally with
`"language": "hr" | "sr" | "bs"` and a `"comment"`. `UserLexicon::parse()`
(`formant_frontend.cpp`) reads the file once, checks every entry (one stress
mark, length only after a vowel, known verb classes) and counts the ones it
skips; `laprdus_get_accent_lexicon_report()` returns the counts and the first
reason. `Frontend::set_user_lexicon()` copies the entries for its language
into hash maps of its own that are consulted before the built-in ones at every
step (user exact form, built-in exact form, then stems by falling length, the
user's first at each length), and the user's verbs are tried before the
built-in verb tables. The whole cost is one or two extra hash probes per word;
the file is capped at 20 000 word forms and 2 000 verbs so that the linear
verb scan stays short. The lexicon lives in the engine, survives voice
changes, and is replaced wholesale by the next load, so a removed entry is
really gone. `laprdus_load_user_config()` and every platform's user-dictionary
loader pick the file up; the C API is `laprdus_load_accent_lexicon()`,
`laprdus_load_accent_lexicon_from_memory()`, `laprdus_clear_accent_lexicon()`.
Suffix rules (the next layer down) remain code: most of what they would say
can be written as a stem entry.

**A word is more than its dictionary form.** The accent often sits somewhere
else in the other cases and persons, and a stem entry (`kontr'o:l*`) gives
every form the same accent. Before adding a word, look at its whole paradigm:

- *Rules have a lower limit.* The rule for verbal nouns in *-enje* starts at
  four syllables, because the three-syllable ones go both ways: *rođénje,
  rješénje, kršténje, pošténje, snižénje* are lexicon entries, *ùčenje,
  mìšljenje, vȉđenje* are right by default. (*Rȍđendan* is not *rođénje*.)
- *Moving accent*: *sìgnal* but *signála, signálu, signáli*; *telèfon* but
  *telefóna, telefónom* (while *mȉkrofon, sȁksofon, mȅgafon* stay on the
  first syllable in every case); *novčànīk* but
  *novčaníka, novčaníci*; *podátak, podáci* but genitive plural *pòdātākā*;
  *obavijéstiti, obavijéstio* but present *obàvijēstīm*. Such words are
  written as a list of forms, `sign'a:l|a|u|om|e|i|ima`, which makes one exact
  entry per ending.
- *Forms a stem does not reach*: *podaci, zadaci, počeci* have lost the *t* of
  *podatk-*, so the stem `pod'a:tk*` never matched them.
- *Derived words caught by a stem*: *kòntrolnī* is not *kontróla*, *dȍdatno*
  is not *dodátak*. An exact form beats a stem and a longer stem beats a
  shorter one, so the derived word gets its own entry.
- *One spelling, two words*: *obavijesti* is the noun's genitive, dative and
  plural (*ȍbavijēsti*) and the verb's imperative (*obavijésti*). The noun
  gets the spelling; the verb's other forms (*obavijestiti, obavijestio,
  obavijestite*) have their own entries.

The words handled this way in October 2026 (*obavijest* with *obavijestiti*
and *obavještavati* and their ekavian forms, *dodatno, mogućnost, sigurnost,
privatnost, kontrolni, podatak, signal, pozadina, novčanik*, the plurals of
the nouns in *-tak*, the verbs, the names *Zvónko* and *Vládo*; later
*Tìjana, Zvȍnimir, emòtikon, nȅpregledan, zaùstavljanje* with *zaùstavljati,
prèslušati, prègledati* and the negated participles *nepròčitan,
neprèslušan*, see "Word classes") follow
Hrvatski jezični portal and the declension and conjugation
tables of Wiktionary. Two of them are not in any dictionary: *Tìjana* is
accented like the other female names of that shape (*Mìrjana, Bòjana,
Zòrana, Gòrdana, Dràgana*; the earlier entry had *Tijána*), *emòtikon* like
*emòcija*, and both keep that syllable in every case. *Pregledati* is the
perfective *prègledati* (*prègledaj, prègledao*), which is what a screen
reader meets; the imperfective *preglédati* is spelled the same. *Nàtaša*
(Wiktionary) is a lexicon entry, every case and the possessive. *Sad* is the
adverb *sȁd* everywhere, short; the town *Nȍvī Sȃd* (*Novog Sáda, Novom
Sádu*) gets its long vowel only from the preceding *novi* (a phrase rule in
the front end), because the adverb is the far commoner word. The numerals
were checked against HJP and the Serbo-Croatian entries of Wiktionary
(October 2026) in every voice: *nȕla, nȕltī* (the default gave a rising
*nùla*), *dȅvet, dȅset* (Zvonko without the final length, Stojan and Mirsad
with it, *dȅsēt*), *čȅtiristo* (was *čètiristo*), *tìsuća, tìsuću, tìsuće,
tìsućītī* (was *tȉsuća*), *milìjārda* (was *milijárda*), *milìjūntī*, the
ordinals *pȇtī, šȇstī, sȇdmī, ȏsmī, dèvētī, dèsētī, stȏtī, dvjȅstōtī*, and
*dvȍje, trȍje, ȍba, ȍbje, dvòjica, tròjica, četvòrica, petòrica*. After a
number the thousand and million words are in the genitive plural, whose
accent is not the dictionary form's: *pet tȉsūćā, sto hȉljādā, pet
milijúnā, pet milijárdī* are a phrase rule of the front end (any cardinal
from five up, the teens, the tens and the hundreds before *tisuća, hiljada,
milijuna, miliona, milijardi*); *tisuća* on its own, after one (*dvadeset
jedna tisuća*) and *tisuće* after two to four keep *tìsuća*.

Croatian *dvadeset* was checked again on 8 October 2026 against Mrežnik:
[*dvádesēt*](https://rjecnik.hr/mreznik/dvadeset/) is indeclinable;
the ordinal [*dvádesētī*](https://rjecnik.hr/mreznik/dvadeseti/) retains
that long rising accent in *dvádeseta, dvádesetog(a), dvádesetom(u),
dvádesetima*, etc. These exact entries replace the earlier Croatian
falling-accent rendering override. The stressed *a* stays long; Croatian
still omits the unstressed lengths (*dvádeset, dvádeseti*).
Related forms have separate entries: Mrežnik's
[*dvadesétak*](https://rjecnik.hr/mreznik/dvadesetak/) stresses the second
*e*, whereas [Školski rječnik's *dvádesetero*](https://rjecnik.hr/?letter=d&page=28)
keeps the first syllable. A blanket *dvadeset* stem would confuse them.
These entries also apply when the number converter produces *dvadeset*
from digits, including compound numbers such as *21* and *120*.

The remaining shared rendering of *dvadeset* (Stojan and Mirsad) and
*trideset* retains its earlier long falling accent, chosen in listening
tests to avoid a perceived split into two words. Stojan and Mirsad also
retain their separate ordinals *dvadèsētī, tridèsētī*. The earlier acoustic
fix for the closure of *d* remains: between vowels it dipped 21 dB
below the vowels, where Eloquence's *d* between vowels dips 7-9 dB and a
recorded speaker's 10 dB, so the voice bar of *d* and *g* now has the level
of *b*'s (see "p and b once more"). *Četrdeset* to *devedeset*
stress the *de* in every dictionary (*četrdèsēt*); Zvonko has them without
the final length, Stojan and Mirsad with it. *Ȉnače* (the rule for *-ač* gave *ináče*),
*uòstalom* and *ionàko* are lexicon entries (HJP). *Slȍbodan, slȍbodno* are told
apart from the noun *slobòda*, whose stem had taken them; the name
*Slobòdan* keeps its oblique cases, and the nominative, spelled like the
adjective, goes to the adjective. *Snága, snázi, snážan* (the default gave
*snȁga*). Adjectives in *-alan* keep the first syllable whatever their
length in HJP (*lègālan, nȍrmālan, lòjālan, ȉlegālan, ȉdeālan, ȍriginālan,
prȍfesionālan*); the rule for nouns in *-an* no longer takes them
(*legàlan, normàlan*), and *legalan, ilegalan, jednostavan* have entries
with their tone, their comparatives (*jednostàvniji*) and their nouns in
*-nost* (*legálnōst, jednostávnōst*). *Jednostavan* had been *jednòstavan*
by an earlier entry; HJP has *jȅdnostāvan*. *Ȉnstagram* is not in the
dictionaries and follows *ȉnternet* rather than the *-gram* rule
(*instàgram*). *Ubrzánje* (the stress had gone to the syllabic r), the
participle and adverb *ȕbrzan, ȕbrzano* and the present *ùbrzām* (Zvonko
keeps *ubr̀zam*, as for every verb); *mȍnoton, mȍnotono* with *monotònija*;
*pàuza* with *sȁuna* and *fàuna* (the long-stem rule had read "au" as a long
*u*, *paúza*); *kòrisnički* with *kòrisnīk, kòrisnica, kȍristan, kȍrisno,
kȍrīst, kòristiti*; *rédak, rétka, réci, récima, rȇdākā*, also spelled
*redka, redci* (Croatian orthography) and the Serbian adjective *rédak,
rétka* "rare", which has the same accent. *Reci* stays the imperative *rèci*
and *reče* the aorist *rȅče*, both far commoner than the noun's plural and
vocative. *Podèsiv, podèsivo* and *podesívōst* are in no dictionary and
follow the adjectives in *-iv* from prefixed verbs (*izvèdiv, dokàziv,
podnòšljiv, prilagòdljiv; održívōst*); the verb is *pòdesiti, pòdesīm,
pòdešen*. *Rȍđendān* (the default rules had *rođèndan, rođendána*) in every
case. *Zauzet* is *zȁuzēt, zȁuzēta, zȁuzēto* in every voice, as the
adjective "busy" and as the passive participle, with *zȁuzeo, zȁuzētōst* and
the verbal noun *zauzéće* (HJP, Školski rječnik hrvatskoga jezika); an
earlier entry had given it the infinitive's *zaùzet* like *preùzet*, and
*zauzéta*. The infinitive and imperative keep *zaùzēti, zaùzmi*, so the
plural adjective *zȁuzēti* ("zauzeti smo") is read as the infinitive.
Three choices go with common Croatian
speech rather than the dictionary, at the request of a native listener:

- unstressed length is left out where it is not heard (*dȍdatno, kòntrolni,
  sìgnal* for the dictionaries' *dȍdātno, kòntrōlnī, sìgnāl*);
- nouns in *-itet* keep the long *e* prominent in the nominative too:
  Zvonko says *kapacitét, identitét* next to *kapacitéta*, where the
  dictionaries have *kapacìtēt*. Stojan and Mirsad follow the dictionaries;
- of *obavijestimo* and *obavijestite*, which are both present and
  imperative, the first is read as present (*obàvijēstīmo*) and the second
  as imperative (*obavijéstite*), the more frequent use of each.
- verbs generally keep the accent of the infinitive in every form for
  Zvonko, with lexical exceptions (see below).

**Verbs.** The accent of a verb's infinitive stays on its root in most other
forms: *uréditi, urédi, urédio; otvòriti, otvòri, otvòrio; pročìtati,
pročìtaj; pokrénuti, pokréni; podijéliti, podijéli*. The rule for long
infinitives found the infinitive, but nothing told the front end that
*uredi, uredio, uredim, uređen* belong to the same verb, so every other form
got the first syllable: *Ùredi, Òtvori, Ìsključi, Pòdijeli* on every button.
Two tables in `formant_lexicon.cpp` now name the verbs, and
`StressRules::verb_form()` recognizes their forms (infinitive, future,
present, imperative, aorist, both participles, verbal noun; the passive
participle with its consonant change: *uređen, zamišljen, očišćen,
podijeljen*):

- `IJE_VERBS`: roots with a long *ije* (*dijel, mijen, lijep, cijen, slijed,
  miješ, htijev* ...). After a prefix the spelling *ije* marks the verb,
  because the nouns beside it have the short *je* (*podjela, promjena,
  zamjena*), so these roots take any verbal prefix: *raspodijeliti,
  iskorijeniti*. Where a free prefix would match a noun, the root lists its
  prefixes (*izvijestiti* but not *povijesti*; *zapovijedati* but not
  *zapovijedi*); *prelijepi, pretijesni* are lexicon entries.
- `VERBS`: about 360 common verbs as whole stems with the accent of the
  infinitive and the conjugation class (*ur'e:d=i*, *proč'it=a*, *pokr'e:=u*,
  *pok'a:z=t* with *pok'a:ž=e*), and the common ekavian counterparts of the
  *ije* verbs (*podéliti, proméniti*). The stems were generated from the
  entries of Hrvatski jezični portal; `=` is followed by what the dictionary
  says about the present and the passive participle.

Three things to know:

- **Dictionary and common speech.** In the present and the passive participle
  the dictionaries usually move the accent one syllable back (*ùrēdīm,
  ùrēđen, òtvorīm, pòdijēlīm, pòkrēnēm*). Stojan and Mirsad do that. Zvonko
  generally keeps the accent of the infinitive (*urédim, uréđen,
  otvòrim, pokrénem*), as Croatian is commonly spoken and as a
  native listener asked for the verbs with a long vowel; the verbs with a
  short one follow for consistency. *Obavijestiti, razumijevati* and the
  present of *započeti, preuzeti*, which are lexicon entries, do the same.
  The exceptions carry a `!` and follow the dictionaries in every voice:
  *uključiti, isključiti, zaključiti, priključiti* (*ùključeno*,
  *ìsključeno*, the status words of every settings screen, which the same
  listener heard as wrong with the accent on *-klju-*), *rasporediti*
  (*raspòređeni*), *osnažiti*, *potaknuti*, *zaštititi* (*zàštićen*),
  *podijeliti* (*pòdijelim, pòdijeljen*) and *proslijediti*
  (*pròslijedim, pròslijeđen*, October 2026). Zvonko gets
  the shift without the length after it (*ùključen* for the dictionaries'
  *ùkljūčen*).

  Checked on 8 October 2026: [Mrežnik's *ideja*](https://rjecnik.hr/mreznik/ideja/)
  gives *idéja, idéjē, idéji, idéju, idéjōm, idéjā, idéjama*;
  [*besplatan*](https://rjecnik.hr/mreznik/besplatan/) gives *bèsplatan,
  bèsplatna, bèsplatno, bèsplatnī*. These have exact Croatian paradigms so
  the entries do not catch derivatives.
  [HJP's *podijeliti*](https://hjp.znanje.hr/index.php?show=search_by_id&id=eVxlXhY%3D)
  gives *podijéliti*, present *pòdijēlīm* and passive *pòdijēljen*.
  The restricted `dijel:i!:po` entry precedes the generic `dijel:i` root;
  it corrects the present and passive without changing other prefixes.
  The infinitive, active participle and imperative keep their root accent.
  Ambiguous *podijeli/podijelite* retain the front end's imperative reading;
  *podijelimo* retains its present reading. The spelling *ije* supplies its
  length even after the accent; other unstressed lengths follow the voice's
  existing policy.

  The same check for [HJP's *proslijediti*](https://hjp.znanje.hr/index.php?show=search_by_id&id=dl5lWRA%3D)
  confirms *proslijéditi*, present *pròslijēdīm* and passive *pròslijēđen*.
  The infinitive was already correct; `slijed:i!:pro`, before the generic
  `slijed:i`, enables the present/passive shift in Croatian without
  changing other prefixes. It covers *pròslijedim, pròslijediš,
  pròslijedimo, pròslijede* and the passive's case/gender forms
  (*pròslijeđena, pròslijeđenima*). The active participle keeps
  *proslijédio, proslijédila*. As with *podijeliti*, ambiguous
  *proslijedi/proslijedite* retain their imperative reading
  (*proslijédi, proslijédite*); present readings of those spellings need
  explicit accent marks. The imperfective remains *prosljeđívati,
  prosljèđujem*, as given in [HJP](https://hjp.znanje.hr/index.php?show=search_by_id&id=dl5lWRQ%3D).
- **Noun twins.** The imperative is often spelled like a case of a noun:
  *potvrdi* (*potvrda*), *uredi* (*ured*), *otvori* (*otvor*), *objavi,
  prijavi, načini*. Such a form is read as the command only at the head of
  its clause, alone or after a word like *ne, i, molim* (*Potvrdi. Ne
  zaboravi lozinku.*); anywhere else the noun has priority (*u potvrdi, svi
  uredi*). The twins were found by asking the dictionary for the imperative
  form; they carry an `n`.
- **Imperfectives in *-ivati*, *-ovati* and *-avati*** are too many to list
  and regular enough for rules (`StressRules::strong()`):
  - the present, imperative and present participle in *-ujem* have the accent
    on the syllable before *-uj* in all three voices: *ukljùčujem, prikàzuje,
    urèđuju, kùpujem, pùtujući*. The few verbs in *-ovati* that keep the
    first syllable are lexicon entries (*nàpredujem, sùdjelujem, sávjetujem,
    pósjedujem*), as are *olúja* and *kravàta*, which look like such forms;
  - the same forms of the verbs that keep the suffix (*označavam,
    obavještavaju, uživaš, pokrivaj*): the dictionaries have the accent one
    syllable before it (*oznàčāvām, ùžīvām*), which Stojan and Mirsad say;
    Zvonko keeps the infinitive's (*označávam, užívam*), as with the other
    verbs. The third person is spelled like many nouns and adjectives
    (*država, zabava, krvava; osjetljiva, perspektiva*), so *-ava* counts
    only from four syllables on (*označava, održava, podržava*) and *-iva*
    not at all; the short verbs are in `VERBS` (*rješava, uživa, pokriva,
    dobiva*). *Poziva, naziva* stay with the nouns (*poziv, naziv*);
  - the infinitive, the future, the participle and the verbal noun were
    covered by the rule for *-ivati/-avati* before (*uključívati,
    uključívao, uključívanje*).
- **What is not covered.** A verb in neither table and outside these rules is
  right in the infinitive, or if its accent is on the first syllable anyway
  (*prìhvati, prèmjesti, pòdesi, òčisti, pròvjeri* are, by the dictionary).
  Irregular verbs have lexicon entries for the forms a screen shows most
  (*započni, preuzmi, pronađi, prevela*). The participle of verbs in
  *-ovati* (*putovao*) and the passive participles in *-ivan, -avan* are not
  covered: adjectives like *pozitivan* have the same ending.

**The list of October 2026.** A native listener sent about eighty words,
pronouns and names collected from what the voices read. Each was looked up
with its whole paradigm in Školski rječnik hrvatskoga jezika (rjecnik.hr),
Hrvatski mrežni rječnik, HJP, Wiktionary and, for Serbian, the scans of
Rečnik srpskoga jezika (Matica srpska, 2011); names without an entry got the
accent of names of the same shape. What came out of it:

- Most were plain lexicon gaps: *pòtpuno, mȁslinov, mòbitel* (the entry had
  *mobìtel*), *oáza, rakèta, krȃlj, ùistinu, higijéna, jȃvno, srȅdnjī,
  ekípa, žèljezničār, streljáštvo, dìrektno, međunárodnī, odr̀živ,
  kampànja, zȁjednički, òbrazovānje* (the entry had *obrázovanje*), *vȋd,
  iskústvo* (genitive plural *ìskūstāvā*), *àlāt / aláti, mòdel / modèli,
  progràmērskī, èkrān / ekrána, záslon, znánje, modèrātor, surádnja,
  saràdnja*, and the verbs *rasporéditi, osnážiti, potàknuti, podstàknuti,
  razvíjati, prèdstaviti, nàstaviti, inspirírati, inspìrisati* in `VERBS`.
  Their derivatives were checked too, where the accent differs or a stem
  would not reach the longer forms: *jávnōst* (against *jȃvnī*), *pòtpunōst,
  odr̀živōst, iskùstven* (against *iskústvo*), *srȅdnjoškolskī*.
  Where the dictionaries differ, the choice is the Školski rječnik's
  (*mȁslinov, surádnja, òbrazovānje*), with the Serbian dictionary's first
  form in the Serbian and Bosnian tables (*màslinov, rakéta*).
- The personal pronouns had no length: *jȃ, tȋ, ȏn, mȋ, vȋ, nȃs, vȃs, njȋh,
  njȋm, mnȏm* are long, *òni, òna, òno* rising. The clitics spelled like
  them (*Kažem ti, Daj mi*) keep a short vowel: an enclitic loses any length
  now. A one-syllable word spelled with a foreign letter skips the lexicon,
  so *Wi-Fi* is not read with the pronoun *vȋ*.
- *Danijel(a)* and the participles of *viti* (*razvìjen, zàvijen*) were read
  with the jat diphthong (*Danjel*, *razvjen*); they are now excluded from
  it.
- Names from the dictionaries: *Sȃndra* (the entry had the stress mark
  before the r), *Sìniša* (was *Siníša*), *Dànijel* (was *Danìjel*),
  *Marijàna* (was long), *Mìhājlo, Ànita, Ȇva, Mája, Máto, Mátić,
  Teodóra* (the man's *Tèodor* keeps only the nominative), *Crnògorac* /
  *Crnogórci, Ivànščica, Bȑkljača*, *Bèla* (Croatian voice only; in
  Serbian *bela, bele* is the adjective); by analogy *Nàida,
  Adrijàna, Mȉlosava, Márkovina, Prȗgovečkī, Vládić, Tȉhić,
  Rȍtić, Lȅtić, Jȕsić*.
- Several words were already right in place and tone (*istòvremeno,
  inteligèncija, sùdjelujete*) and were heard as wrong because the rising
  accent put its pitch peak on the next syllable; that was fixed in the
  intonation (see "Pitch" in section 5).
- *Proteza* is *protéza* in every form (HJP, Školski rječnik), with
  *protètika, protètičār* and the words it comes with, *zȗbnā, òčnā*.
- **Superlatives** follow the rule of Hrvatski mrežni rječnik and Školski
  rječnik (no exception among about fifty forms they print): the superlative
  of a two-syllable comparative with a short falling accent (*bȍljī, vȅćī,
  mȁnjē, ljȅpšī, jȁčī, bȑžī, gȍrī*) has the long falling accent on *naj-* and
  none on the rest (*nȃjboljī, nȃjmanjē, nȃjjačī*); the superlative of a
  comparative in *-iji* keeps the comparative's short rising accent, *naj-*
  unaccented (*najnòvijī, najvàžnijī, najjednostàvnijī*). Before, *naj-*
  had a short, neutral accent (and *najjači* came out *najjáči*). The rule is
  `StressRules::superlative_of_short_comparative()`: *naj* + a consonant +
  one syllable + an adjective ending, the stem ending in a palatal or being
  *gor* (*najam, najava, najlon, najmiti* are not superlatives). The
  grammars (Vukušić, Zoričić and Grasselli-Vukušić 2007, Jonke; Klajn for
  Serbian, Alić for Bosnian) also describe a second, long falling accent on
  *naj-* of the longer superlatives (*nȃjstàrijī*); the voices have one
  accent per word and follow the dictionaries. *Nȃjposlije, nȃjposlē,
  nȃjprē, nȁjzad, pònājprije* are lexicon entries.
- *modela* is read as the genitive singular *modèla*, not the genitive plural
  *modélā*; *Teodora* as the woman's name; *vidu* as the locative *vídu*
  (*u vidu*).

To add a verb, run `tools/formant/verb_accents.py urediti otvoriti ...`: it
looks each infinitive up and prints the table entry, the dictionary's forms
and any noun twin (or says that the verb needs no entry, or that it is
irregular and belongs in the lexicon). Then check the forms at the head of a
clause and inside one.

**The second list, checked 8 October 2026.** The following corrections use
the Institute for Croatian Language's normative *Školski rječnik hrvatskoga
jezika*, cross-checked against *Hrvatski jezični portal* (HJP). These are
lexical accents, not a new rule inferred from twenty words. The table gives
the dictionaries' lengths; Zvonko keeps the existing policy of shortening
unstressed vowels, so, for example, *dȍlāra* is synthesized as *dȍlara*,
*automòbīl* as *automòbil*, but the stressed vowel in *automobíla* stays long.

| Collected spelling | Dictionary accent and paradigm | Primary entry |
|---|---|---|
| dolara | *dȍlāra* (G singular), *dȍlārā* (G plural); initial falling accent throughout | [dolar](https://rjecnik.hr/search/?strict=yes&q=dolar) |
| eura | *ȅura* (G singular), *ȅūrā* (G plural) | [euro](https://rjecnik.hr/search/?strict=yes&q=euro) |
| naravno | *nárāvno*; HJP has *náravno*, same stressed vowel and tone | [naravno](https://rjecnik.hr/search/?strict=yes&q=naravno) |
| priča | N singular *prȋča*; G plural and present *prȋčā* | [priča](https://rjecnik.hr/search/?strict=yes&q=pri%C4%8Da), [pričati](https://rjecnik.hr/search/?strict=yes&q=pri%C4%8Dati) |
| djelomično | *djȅlomično*, *djȅlomičan*, *djȅlomična* | [djelomičan](https://rjecnik.hr/search/?strict=yes&q=djelomi%C4%8Dan) |
| činjenica | *čȉnjenica*, G plural *čȉnjenīcā* | [činjenica](https://rjecnik.hr/search/?strict=yes&q=%C4%8Dinjenica) |
| nemoguće | *nȅmogūće*, *nȅmogūć*, *nȅmogūća*; the noun keeps *nemogúćnost* | [nemoguć](https://rjecnik.hr/search/?strict=yes&q=nemogu%C4%87) |
| popodne | *popódnē*, *popódnēva*; same stressed long o in *popodnevima* | [popodne](https://rjecnik.hr/search/?strict=yes&q=popodne) |
| proslaviti | *pròslaviti*, *pròslavīm*, *pròslavio*, *pròslavljen* | [proslaviti](https://rjecnik.hr/search/?strict=yes&q=proslaviti) |
| europska | *èuropskā*, unlike *Európa* | [europski (HJP)](https://hjp.znanje.hr/index.php?show=search_by_id&id=fFtjWRI%3D) |
| osiguranja | *osiguránja* (G singular/N plural), *osiguránjā* (G plural) | [osiguranje](https://rjecnik.hr/search/?strict=yes&q=osiguranje) |
| veselimo | *vesèlīmo*, from *vesèliti*, *vesèlīm*; noun *vesélje* | [veseliti](https://rjecnik.hr/search/?strict=yes&q=veseliti) |
| automobil | *automòbīl*, G *automobíla*, N plural *automobíli*; adjective *automòbīlskī* | [automobil](https://rjecnik.hr/search/?strict=yes&q=automobil), [automobilski](https://rjecnik.hr/search/?strict=yes&q=automobilski) |
| tekučina | Standard spelling *tekućina*, accented *tekùćina* | [tekućina](https://rjecnik.hr/search/?strict=yes&q=teku%C4%87ina) |
| kafić | *kàfīć*, *kafíća*, *kafíći* | [kafić](https://rjecnik.hr/search/?strict=yes&q=kafi%C4%87) |
| pomoćnici | *pomoćníci*, from *pomòćnīk*, *pomoćníka*; vocative *pȍmoćnīče* | [pomoćnik](https://rjecnik.hr/search/?strict=yes&q=pomo%C4%87nik) |
| tamo | *tȁmo*, short falling, not the former *támo* | [tamo](https://rjecnik.hr/search/?strict=yes&q=tamo) |
| prognoza | *prognóza*, *prognózē*, *prognóze* | [prognoza](https://rjecnik.hr/search/?strict=yes&q=prognoza) |
| kabanica | *kabànica*, *kabànicē*, *kabànīcā* | [kabanica](https://rjecnik.hr/search/?strict=yes&q=kabanica) |
| slojevita | *slojèvita*, *slojèvit*, *slojèvito*; noun *slojèvitōst* | [slojevit](https://rjecnik.hr/search/?strict=yes&q=slojevit) |
| kišobran (follow-up) | *kȉšobrān*, *kȉšobrāna*, *kȉšobrāni*, *kȉšobrānima*: short falling i throughout | [Mrežnik](https://rjecnik.hr/mreznik/kisobran/), [HJP](https://hjp.znanje.hr/index.php?show=search_by_id&id=elxnXRg%3D) |
| zaštićen (follow-up) | *zàštīćen*, *zàštīćena*, *zàštīćeno*; infinitive *zaštítiti*, present *zàštītīm* | [zaštititi (HJP)](https://hjp.znanje.hr/index.php?show=search_by_id&id=f15kURN9) |

For *euro* the sources disagree in tone: Školski rječnik gives *ȅuro*,
[HJP gives *èuro*](https://hjp.znanje.hr/index.php?show=search_by_id&id=fFtiXhg%3D).
The Croatian entry follows Školski rječnik, as in the earlier list. Neither
source puts the accent on *u* in *eura*. The spelling *tekučina* is treated
as a typo in this report; the engine entry is for *tekućina*, without an
automatic č-to-ć substitution in input text.

The noun corrections are in `CROATIAN`, leaving the other language tables
alone. `VERBS` gains *proslaviti* and *veseliti*. Whole verb stems now accept
`/` and `^` as explicit rising/falling tone, just like word entries:
`pr/oslav=i` carries the rising accent through the paradigm. Existing `'`
entries retain their automatic tone. `pr/i:č=a` gives *príčati, príčao,
príčajū*; exact Croatian forms supply *prȋča, prȋčam, prȋčaj, prȋčan*.
A broad *prič-* stem would have swallowed both patterns.

Homographs still need a reading choice: *pomoćnici* defaults to the
masculine N plural (*pomoćníci*), not D/L singular of *pomòćnica*;
*proslavi/proslave* default to the noun *prȍslava*, not the verb. Explicit
accent marks override those choices. This change adds no grammatical
disambiguator. Regression checks compare synthesized audio with explicitly
accented text for all twenty items and the follow-ups *kišobran* and
*zaštićen*, their case and verb forms, contrasting derivatives, and short phrases. *Kišobran*
uses exact noun forms to avoid assigning its accent to derived words;
Zvonko renders *kȉšobran*, following the same unstressed-length policy.

For *zaštićen*, the existing `zašt'i:t=i<pn` verb entry already encoded
the dictionary's present and passive shifts, but applied them only outside
Croatian. Adding `!` enables them for Zvonko too: *zàštićen, zàštićena,
zàštićeno, zàštićenima, zàštitim*. The infinitive, active participle and
imperative retain *zaštítiti, zaštítio, zaštíti*. The noun
[*zàštīćenōst* (HJP)](https://hjp.znanje.hr/index.php?show=search_by_id&id=f15kURJ0)
has a separate Croatian entry, without unstressed length. The existing
negative *nezàštīćen* (also confirmed by
[Školski rječnik](https://rjecnik.hr/search/?strict=yes&q=neza%C5%A1ti%C4%87en))
and the Serbian/Bosnian shifted forms keep their existing behaviour.

**Battery, listening, reading and app names (8 October 2026 follow-up).**
The Croatian noun and verb forms were checked against Školski rječnik,
Mrežnik and HJP. The same policy of omitting unstressed lengths applies.

| Word | Chosen accent and related forms | Evidence |
|---|---|---|
| baterija | *batèrija*, *batèrijē*, *batèrije*, *batèrījā*, *batèrijama*; adjective *batèrījskī* | [Mrežnik](https://rjecnik.hr/mreznik/baterija/), [Školski rječnik](https://rjecnik.hr/search/?strict=yes&q=baterija), [baterijski](https://rjecnik.hr/search/?strict=yes&q=baterijski) |
| poslušati | *pòslušati*, *pòslušām*, *pòslušāj*, *pòslušao*, *pòslušān* | [Školski rječnik](https://rjecnik.hr/search/?strict=yes&q=poslu%C5%A1ati), [HJP](https://hjp.znanje.hr/index.php?show=search_by_id&id=eVpuWhI%3D) |
| pročitati | *pročìtati*, *pročìtām*, *pročìtāj*, *pročìtao*, but passive *prȍčitān* | [Školski rječnik](https://rjecnik.hr/search/?strict=yes&q=pro%C4%8Ditati), [HJP](https://hjp.znanje.hr/index.php?show=search_by_id&id=f1plWRA%3D) |
| Messenger | First-syllable stress; the existing *Mesindžer* transcription now has explicit *Mȅsindžer* | [Collins](https://www.collinsdictionary.com/us/dictionary/english/messenger), British /ˈmɛsɪndʒə/ |
| TikTok | First-syllable stress, rendered *Tȉktok* | [Oxford Advanced Learner's Dictionary](https://www.oxfordlearnersdictionaries.com/definition/english/tiktoktm), British /ˈtɪktɒk/ |
| WinTalker | *Vintòker*, with stress on *to*, also in *Vintòkera, Vintòkeru, Vintòkerom* | User's explicit pronunciation choice, **vin-TO-ker**; no authoritative accent entry found |

HJP gives *bàtērija*; the Croatian entry follows the Institute's two
dictionaries, which agree on *batèrija*. Exact noun forms and a separate
adjective stem cover the cases without assigning their accent to arbitrary
derivatives. `p/osluš=a` supplies the listening verb's rising tone throughout
its paradigm. The existing `proč'it=ap` already gives the correct infinitive,
present, imperative and active participle. A Croatian passive paradigm in
the lexicon supplies its exceptional falling tone, *prȍčitan*; the negative
*nepròčitan* and the Serbian/Bosnian readings are unchanged.

English stress does not specify a Croatian rising/falling pitch accent.
The short falling tones of *Mȅsindžer* and *Tȉktok* are explicit synthesis
adaptations of initial English stress, not claims of dictionary-prescribed
Croatian brand accents. *Vintòker* uses a short rising vowel to realize the
user's chosen non-initial stress. These name entries are shared by the
voices. `internal.json` transcribes Messenger and WinTalker with bounded
whole-word patterns and optional Croatian case endings; TikTok needs no
spelling replacement. Regression tests load the actual bundled dictionary
and compare the names, capitalization variants, cases and phrases against
explicitly accented input in all three formant voices. Longer unrelated
words are checked to remain outside the replacement patterns.

**Enida, Isaković, Greblički and Spasojević (8 October 2026 follow-up).** The listener specifies
a long stressed *i* in *Enída*. This replaces the earlier *Ènida* guess by
analogy; no dictionary entry for Enida was found. The shared lexicon now
keeps *Enída, Eníde, Enídi, Enídu, Enídom* and the possessive *Enídin*,
including longer forms such as *Enídinima*.

The listener clarified that *Isaković* is stressed on its **initial i**:
*Ìsaković*, with a short stressed *i* and a short **unstressed a**. Cases
and possessives retain this reading (*Ìsakovića, Ìsakovićem,
Ìsakovićevima*). The explicit entry overrides the generic surname rule in
`COMMON`, which every language loads; it was never Bosnian-only. The earlier long
reading came from [HJP's entry for Alija
Isaković](https://hjp.znanje.hr/index.php?show=search_by_id&id=fVllWhM%3D)
and Stipe Kekez's study of surname accents,
[*Prezime na naglasnoj razini kao razlikovni, identifikacijski čimbenik.
Kako ga očuvati?*](https://hrcak.srce.hr/file/122620), *Fluminensia* 23/2
(2011), 57–70, which records *Ìsāk → Isáković*. Those examples do not
establish the pronunciation of every bearer; the listener's correction
determines this entry.

The same listener places the stress of *Greblički* on the first *i*,
*gre-BLIČ-ki*. Its entry is now *Greblìčki*, retaining the existing short
vowel and using the engine's rising tone for non-initial stress. Its case
forms follow the same stem (*Greblìčkoga, Greblìčkim, Greblìčkima*).
No authoritative accented entry was found for this surname.

The listener clarified *Spasojević* as **SPA-so-je-vić**: *Spàsojević*,
with initial stress and a short **unstressed o**, also in *Spàsojevića,
Spàsojevićem, Spàsojevićevima*. Merely shortening the previously stressed
*o* did not implement that pronunciation. Explicit name and possessive
entries in `COMMON` prevent the generic *-ević* rule from restoring stress
and length on *o*. Both recorded and
formant voices use this front end. Tests cover cases, possessives,
capitalization, Cyrillic input and phrases in Zvonko, Stojan and Mirsad;
dedicated tests also check Josip, Vlado, Detence, Baba and Djed. Audio must
match explicitly accented input. Duration comparisons keep Enida's *i*
long, Isaković's unstressed *a* short and Spasojević's unstressed *o* short;
separate comparisons reject the previous stress on Isaković's *a* and on
Spasojević's *o* (both the short and long readings).

**Knezović and Knežević (8 October 2026 follow-up).** [HJP's onomastics
under *knez*](https://hjp.znanje.hr/index.php?show=search_by_id&id=elxuXBQ%3D)
records *Knȇzović* (long falling initial *e*) and *Knéžević* (long rising
initial *e*). Both match the listener's request for stress on *KNE* with
long *e*. HJP also records *Knezòvić*; the requested initial-stress variant
is used here. Shared entries retain the accent in cases and possessives
without matching the noun plurals *knezovi/kneževi*. Tests compare explicit
accents, vowel duration and tone in all eight speaking voices, including
Cyrillic input.

## 5. Timing and melody (`formant_synthesizer.cpp`)

**Durations** start from the inherent value and are scaled by context:
stressed and long vowels are longer, vowels before voiceless consonants and
in closed syllables shorter, consonants in clusters shorter, the last
syllable of a clause longer. In a statement or exclamation a short stressed
vowel of a full word takes only half of that final lengthening (since
October 2026): with
the full stretch a clause-final *sȁd* came out at 70% of *sȃd* and was heard
as long; now it is about 60%, the ratio of the measured short and long
accented vowels. Questions keep the full stretch, because the final rise of a
short question ("Tko je to?") needs the time, above all at high rates, and
so do final function words (*tȍ, tȉ*), whose fall needs it in the statement. The
same factor is in the recorded voices' planner. The rate setting scales durations directly, with
a floor per sound so that consonant cues survive at very high rates;
transitions shrink less than steady states.

The rate the builder uses is `VoiceParams::effective_speed()`: the speed (0.25
to 4.0, wider than the 0.5 to 2.0 of the recorded voices) times the
**acceleration** setting (0.5 to 3.0), capped at 8.0. Acceleration is a plain
multiplier, as eSpeak's rate boost is, so the centre of a host's rate slider
moves with its top; it exists so that a screen reader's slider, which stops
at 2.0, can reach the rates the formant voices are still intelligible at. The
recorded voices use the same product, narrowed to their own 0.5 to 4.0.
User interfaces print the rate the top of the slider reaches in words per
minute: Zvonko speaks 175 words per minute at speed 1.0 on running text
(measured on a 79-word paragraph: 27.1 s), Stojan and Mirsad that times
their tempo (`VoiceRegistry::nominal_wpm()`), so the top of a 2.0 slider is
350 words per minute at acceleration 1.0 and 700 at 2.0.

**Pitch** is the sum of:

- a level that drifts down only a little along the clause (declination),
  0.6 semitones per second, 1.3 to 2 semitones in all,
- at the head of a sentence, a raised start: the stretch before the first
  accent is 1.5 semitones up and comes down into that accent's peak,
- one movement per accented word: falling accents peak early in the stressed
  vowel and fall within it; rising accents are high and nearly level through
  the stressed vowel up to a peak at its end, and the following syllable
  starts as high and comes down
  (until October 2026 the peak lay inside the following syllable, which was
  then heard as the stressed one: *inteligenCIja*, *istoVREmeno*,
  *uključENo*; the stressed *o* of *istòvremeno* now averages +3.3 semitones
  and the *e* after it -0.6, against +1.5 and +0.3 before). The first is the
  largest (4.5 semitones; 3.4 in a clause that goes on a sentence after a
  comma), later ones are 2.6 and shrink slowly (to 80%), words of lower
  prominence 1.4,
- a boundary movement chosen by the punctuation that ended the clause:
  - statement: a drop of 4.5 semitones completed within 180 ms of the last
    accent, after which the voice stays low,
  - exclamation: wider movements throughout, a raised first and last accent
    and a drop of 5.5 semitones,
  - comma, semicolon, colon: a fall of 1.5 semitones after the last accent
    and a rise of 2.5 on the last syllable (from the middle of the last
    accent when that is the last syllable), so the clause ends about at the
    level: 4-5 semitones above the statement and 3 or more below the
    question,
  - yes/no question: low on the stressed syllable of the focused word, 7
    semitones up right after it (the "inverse" pattern of Croatian and
    Serbian questions); the focused word is the one before *li* if there is
    one, otherwise the last one,
  - wh-question: the highest peak on the question word, the statement's drop
    after the last accent, and then a rise on the last syllable,
  - every question ends going up (see below),
- small segmental effects (high vowels slightly higher, a dip in voiced
  obstruents, a raised onset after voiceless ones).

The contour is smoothed in both directions. With inflection switched off only
the segmental effects remain.

The **inflection level** setting (0 to 1, default 0.5) scales the finished
contour, in semitones, by `level / 0.5` just before it is turned into hertz:
0 is a monotone at the voice's mid pitch (the segmental effects go too), 0.5
the movements described above, 1 twice every movement (a first accent of 9
semitones, a question rise of 14). The user pitch setting multiplies the mid
pitch and takes 0.25 to 4.0 for these voices, with the result held between 45
and 480 Hz, so Zvonko bottoms out at about 0.4.

A clause of a single syllable (a letter name while spelling, *da*, *ne*) gets
less than half of these movements. A full sentence melody squeezed into one
syllable was a 6-semitone glide, steep enough to hide the pitch step a screen
reader uses to mark a capital letter. That holds for statements only: a
one-syllable question keeps its whole rise and a one-syllable exclamation
(*Ne!*, *Stoj!*) 70% of its movements, or it could not be told from *Ne.*

**The melody of read Croatian (October 2026).** The contour above replaced
one with a baseline falling two semitones per second (up to six), first
accents of 4.5 and later ones of 3.6 semitones, and a 4-semitone rise at
every comma. Asked for "the intonation of a Croatian radio announcer", it was
compared with the natural Croatian voices of macOS (Lana, Marija), with
Eloquence (Italian, the same news sentences translated) and with eSpeak, on
ten news sentences (`tools/formant/pitch.py`, plus plots of the engine's own
contour with the words marked):

- The natural voices and Eloquence keep the middle of a sentence on a level:
  averaged over eight statements and twelve slices of their length, Lana's
  pitch lies between -0.2 and +0.7 semitones of her median from the second
  slice to the tenth, Eloquence's between -0.7 and +0.5, and the slope of
  that stretch is -0.1 and 0.0 semitones. Zvonko fell steadily, 0.6 semitones
  over the same stretch and -1.9 in the eleventh slice already, so the
  second half of a long sentence sounded like its end.
- All of them fall on the last word: Lana from about 0 to -5, Eloquence and
  Marija to -4.5, the old Zvonko to -7.5 below the start of the plateau.
- The local movements were larger in Zvonko (standard deviation around the
  trend 1.73 semitones) than in Lana (1.42), Eloquence (1.22) and Marija
  (1.15); Lana and Eloquence open a sentence 3 to 5 semitones up and then
  move by about two.
- At a comma Lana and Marija fall to 2 to 3.5 semitones below the level
  (*statistiku,*, *stupnjeva,*, *financija,*) and start the next part of
  the sentence 2.5 to 4.5 up; Eloquence's Italian does the same. Zvonko
  rose 4 semitones from the level there, ending as high as the question.

The published measurements were checked as well. No study gives a
declination slope for Croatian or Serbian; Godjevac (2005) describes each
following word of a phrase as downstepped, with one peak raised again in a
long phrase. Readers of Croatian Radio (Langston 2018, *Govorimo hrvatski*
on HR1) raise the last word before a pause inside a sentence by 1.5 to 4.6
semitones, typically 2.4 to 3.2, from its accented to its next syllable,
and DECtalk's rules give a comma "a weaker fall followed by a slight
continuation rise" (12 Hz below the baseline, then two small impulses at
the end of the last vowel). The natural voices' fall at the comma is
therefore not followed: the comma has a small fall and a slight rise.
Eloquence ends a statement 6.1 semitones below its last peak (Hertz et al.
1999) and DECtalk 6.6; Zvonko's fall of 4.5 on top of the declination ends
5 to 6 below the level. Standard speakers realize a rising accent as a high,
nearly level stressed syllable with the next one starting as high and
falling (Pletikos Olof and Bradfield 2019), which is the new shape of the
rising accent below.

With the new values the plateau and the fall match the natural voices in
the plots; questions and clauses of a single syllable were left as they
were (see below), and the declination keeps its old floor of 1.3 semitones
so that a short statement still ends clearly lower than its question at 2x
speed. The values apply to the recorded voices too.

**Short questions.** In a clause of a few words the end is all there is to
hear the question mark by, and three things used to hide it:

- A wh-question fell like a statement. *Kako si ti?* and *Kako si ti.* had
  the same contour to within a semitone, as had *Zašto?* and *Zašto.*
  Natural Croatian often does fall there, but Eloquence ends every question
  with a rise, question word or not (*Come stai tu?* goes from 3 semitones
  below the median to 3 above on the last syllable, *Perché?* rises 7), and
  a listener who cannot see the punctuation needs it.
- A rise that had to fit into the last vowel (*Ti?*, *A ti?*) stayed flat
  for half of the vowel and reached its top in the final 20 ms, where the
  voice has already faded: 3 to 4 audible semitones out of 7. It now starts
  2 semitones low at the beginning of the vowel, is at the top after three
  quarters of it and stays there. Eloquence's *Tu?*, in steps of 30 ms:
  -3 -3 -1 +1 +3 +3. Zvonko's *Ti?*, in steps of 20 ms: 0 0 0 0 +1 +3 +4 +5
  before, -1 -1 0 +1 +3 +5 +5 +5 now.
- A yes/no question with its peak early (*Jesi li dobro?*) drifted down to 2
  semitones below the median by the end. Its last syllable now rises too.

So the last syllable of every question goes up: by the question's own rise
when the last accent is at the end (*Ti?*, *Dobro?*, *Što?*, *Tko je to?*),
otherwise by a separate rise of 3 semitones (6.5 in a wh-question, which
comes up from its low). Clauses of up to five words get all of it; from six
words on it shrinks, to 40% at nine or more, where the question word and the
sentence itself carry more of the load and a strong final rise would sound
unnatural. *Kako si ti?* now ends 6 semitones above *Kako si ti.* (5 at 2x
speed). The last syllable of a wh-question is no longer made quieter
the way a statement's is.

The sizes were set against contours of Eloquence measured with
`tools/formant/pitch.py` (same sentence as statement, question and
exclamation: accent peaks of 3-5 semitones, a final low 4-8 semitones below
the median, a question rise of 6-7 semitones) and against the published rules
of DECtalk (a rise-fall on every stressed syllable, baseline falling 16 Hz
per second) and of the Dutch IPO model (declination, standard rises and
falls). L&H TruVoice, often praised for its intonation, was a diphone
synthesizer whose intonation rules were never published, so it could not be
followed directly.

**Words of one vowel.** The conjunctions *i* and *a* and the prepositions *u*
and *o* are clitics like any other, but they have nothing besides their
vowel, and the clitic rules took that away: shortened as an unstressed
clitic (×0.9) and once more next to a vowel (×0.9), weakened to 80% of a
full vowel, centralized by 15%, and with 45% of their length given to the
formant transition on each side, they came out at 30-40 ms, 6-9 dB below
their neighbours, and never reached their own formants (Zvonko's *i* in *ja i
ona* peaked at F2 2010 Hz instead of 2150). Between two vowels such a word
was only a glide: *ja i ona* was heard as *jajona*, *mislim o tome* as
*mislimo tome*, *ona u vlaku* as *ona vlaku*. Eloquence gives the same words
of Spanish and Italian (*y, o, u, a; e, o, a*) 50-70 ms at the level of the
vowels around them with the target held (*siete u ocho*: F2 at 870 Hz for
60 ms); the natural Croatian voice Lana gives the *i* of *ja i ona* about
100 ms and sets it off from both vowels with a break in the voice.

Since October 2026 the front end marks such a word (`Phone::vowel_word`: the
whole word is one unstressed vowel; a lone *u*, a clause-final one and
spelled letters are stressed words and are not marked) and both kinds of
voices give it a syllable of its own:

- 1.30 times the inherent duration, a little more than the stressed vowel
  of a function word, with no shortening for a neighbouring vowel or for the
  consonants after it, which belong to the next word. In Zvonko's voice at
  rate 1 *i* in *ja i ona* now adds 86 ms to the clause and *u* in *idemo u
  Vukovar* 78 ms, against 54 and 46 before;
- the level and colour of a stressed vowel (no centralization, no extra
  tilt);
- next to a vowel of another word, a quarter of its length for the formant
  transition on its side instead of 45%, so it holds its target in the
  middle;
- next to a vowel or *j* of another word, the voicing dips to 15% for 18 ms
  at the join (shrinking with the rate, at least 8 ms), as at a soft glottal
  onset: *ja | i | ona*, *njoj | i njenom*, *ona | u vlaku*. In the recorded
  voices the 22 ms word gap does the same (`docs/concatenative.md`).

The values were chosen with a speech recognizer (Whisper small) on 40
sentences with 88 of these words, scoring each one by word alignment:

| Zvonko | words of one vowel, 1.0x | other words, 1.0x | CER 1.0x | words of one vowel, 1.7x | CER 1.7x |
|---|---|---|---|---|---|
| before | 31 | 98 of 177 | 16.8% | 12 | 30.8% |
| level and colour only | 30 | 92 | 17.5% | | |
| ×1.15, dip to 30% for 12 ms | 47 | 92 | 14.0% | 26 | 27.8% |
| the same without the dip | 40 | 94 | 15.0% | | |
| ×1.15, voice 15% louder | 44 | 95 | 14.1% | | |
| ×1.30, dip to 30% for 12 ms | 51 | 92 | 13.7% | 25 | 26.5% |
| ×1.30, dip to 15% for 18 ms | 56 | 101 | 12.3% | 25 | 27.6% |
| ×1.45, dip to 15% for 18 ms | 52 | 100 | 12.9% | 27 | 27.4% |

Level and colour alone did nothing; the length, the held target and the
dip did. A deeper dip that shrank only like the transitions (with the square
root of the rate) was relatively longer at 1.7x and lost two words and
three points of CER there; shrinking it with the rate itself took that back
without changing anything at 1.0x. With the final values at 1.0x Stojan
went from 27 to 46 of the 88 words (CER 18.3% to 14.3%) and Josip, with the
word gap, from 26 to 40 (29.3% to 26.8%).

## 6. The three speakers

| | Zvonko | Stojan | Mirsad |
|---|---|---|---|
| Language | Croatian | Serbian | Bosnian |
| Mid pitch | 112 Hz | 98 Hz | 124 Hz |
| Vocal tract (formant scale) | 1.00 | 0.965 (longer) | 1.02 (shorter) |
| Phonation | neutral | firmer, darker | softer, breathier |
| Pitch movements | reference | slightly flatter | wider |
| Tempo | reference | a little faster | a little slower |
| Post-accentual length | mostly neutralized | kept | kept most clearly |
| č / ć (friction centroids apart) | 670 Hz | 920 Hz | 810 Hz |
| /h/ | weak | weak | strong |
| Numbers | tisuća, milijun, dvije tisuće | hiljada, milion, dvesta, dve hiljade | hiljada, milion, dvije hiljade |
| *ne* + verb | *nè znām* only | also *nè mogu* | also *nè mogu* |
| Accent on prepositions | no | no | yes (*ù grād*) |

## 6a. Singing presets

Each speaking voice has two singing presets, after the singing voices of
DECtalk (`[dey<400,22>]`: a phoneme, a duration and a note code) and of
the Macintosh (Pipe Organ, Cellos, Bad News, Good News), which sing any
text to a fixed tune, one syllable per note, and start the tune over with
every utterance. Ours do the same with a public-domain folk song of the
voice's country:

| Preset | ID | Base | Source | Song |
|---|---|---|---|---|
| Zvonko Orguljaš | `orguljas` | Zvonko | pipe organ (stack of partials over a 16' sub-octave, church reverb, no vibrato) | *Vila Velebita* (anonymous, 1882; from a public-domain MIDI transcription) |
| Klapa Zvonko | `klapa` | Zvonko | three glottal voices ±10 cents and a bass an octave down, light reverb, vibrato 5.5 Hz ±30 cents | the same |
| Stojan Trubač | `trubac` | Stojan | sawtooth through a tanh that saturates with level (the lips), slow attack, vibrato 6 Hz ±25 cents setting in late | *Kreće se lađa francuska* (Milosavljević d. 1944) |
| Stojan Harmonikaš | `harmonikas` | Stojan | two sawtooth reeds, the second 14 cents sharp (musette beating), no vibrato | the same |
| Mirsad Sevdalija | `sevdalija` | Mirsad | the glottal voice, breathier, F5 pulled down to 3.1 kHz and narrowed (singer's formant, DECtalk's f5/b5 trick), vibrato 5.5 Hz ±60 cents, 90 ms portamento | *Kad ja pođoh na Bembašu* (traditional, printed 1906) |
| Mirsad Sazlija | `sazlija` | Mirsad | sawtooth with a chorus and a sub-octave, plucked: 350 ms decay, re-picked every eighth note (tremolo), brightness falling as it decays | the same |
| Zvonko Bećarac | `becarac` | Zvonko | dry voice, vibrato 6 Hz ±20 cents, 30 ms portamento | the bećarac tune of Slavonia (traditional; 2/4, quarter = 75), as transcribed for "U mog strica osam kobasica, sedam prži, osmu strina drži": two ten-syllable lines, so any bećarac couplet fits |
| Zvonko Pjevač, Stojan Pevač, Mirsad Solist | `pjevac`, `pevac`, `solist` | each voice | the voice itself ("dry": its own source and phonation, no chorus or hall), vibrato 5.5 Hz ±40 cents, 60 ms portamento; the song transposed in whole semitones so the middle of its range lies five semitones above the voice's speaking pitch (Zvonko then sings *Vila Velebita* between G2 and G3, 98-196 Hz) | the song of the language |

The songs were chosen as the best-known songs of each country whose words
and tune are both out of copyright (an agent checked the authors' dates
and the statutes on 2026-10-07; *Tamo daleko* was rejected because its
recognised author, Đorđe Marinković, died in 1977). The first Croatian
choice was the anthem *Lijepa naša domovino* (Mihanović d. 1861, Runjanin
d. 1878; the anthem law permits musical and educational use when it is
sung as written). The user found it full of pauses: at 80 bpm its half
notes and the rests I had written at the line ends, together with a
per-syllable attack envelope that dipped the voice to nothing between
every two notes (fixed: the attack now applies only after a rest), made it
crawl. *Vila Velebita* (2/4, 113 bpm, no rests) replaced it; the anthem's
notation, should it be wanted again, is `D5/4 D5/4 D5/4. C5/8 C5/8 Bb4/8
Bb4/4 F4/2 | Eb4/8 D4/8 Eb4/8 ~F4/8 G4/2 | F4/8 Eb4/8 D4/8 ~Eb4/8 F4/2 |
(bars 1-2) | Eb4/8 D4/8 Eb4/8 ~F4/8 G4/2 | A4/8 A4/8 C5/4 Bb4/2 | A4/4
A4/4 A4/4. G4/8 | A4/4 A4/8 ~Bb4/8 C5/4. A4/8 | C5/8 C5/8 C5/8 C5/8 C5/4
Bb4/4 | A4/4 G4/4 ~F4/2 | (bars 5-8)`, one note per syllable of the first
two stanzas. The rests between the lines of the other two songs were
dropped for the same reason; the punctuation supplies the breaths. The
first line of *Kreće se lađa* comes from a printed score, the other three
follow the harmony and still want checking against a recording; the
rhythm of *Bembaša* is that of a performance, the song is sung rubato.

How it works (`ClauseBuilder` with `m_singing`):

- **Notes.** `plan_notes()` takes the next notes of the song for every
  syllable of the clause: one note, any notes tied to it (`~`, a melisma)
  and the rests after it. The cursor lives in `FormantSynthesizer` so the
  clauses of one utterance continue the song; `TTSEngine::begin_utterance()`
  rewinds it, so every synthesis call (every line a screen reader sends)
  starts the song from its first note, as the Macintosh voices do at a
  period. The notation is `pitch/value` (`G4/4`, `Bb3/8.`, `R/2`), parsed
  once by `parse_song()`.
- **Durations.** The consonants keep their spoken durations (at the tempo
  clamped to 0.6-2.0x: they are spoken, not sung). The syllable's vowel
  starts on the beat: the consonants that open the next syllable are taken
  off the end of this vowel, or off the rest after it, as singers do
  (Sundberg's rule). The vowel gets what is left, never less than a third
  of the note. Unstressed vowels are sung in full (no centralization, no
  level drop), and the speech loudness contour (declination, final drop,
  prepausal fade) is replaced by a note envelope: an attack on the first
  note after a rest or pause (notes that follow each other are legato),
  a release into a rest and a short one at the end of the clause, and for
  the saz an exponential decay re-triggered every eighth.
- **Pitch.** The note frequency is `440 * 2^((midi + transpose - 69)/12)`
  times the pitch and user pitch settings (so the pitch slider transposes),
  held through the note. A consonant takes the note of the vowel after it
  (KTH), the step between notes is smoothed with the style's portamento
  time constant (8 ms for the organ and accordion, 25-40 ms for trumpet and
  klapa, 90 ms for the sevdah voice: Sundberg measured 70-100 ms for the
  central three quarters of a sung pitch change). Vibrato is a sinusoid
  in cents whose depth is the style's value at inflection level 0.5, grows
  over `vibrato_delay_ms` after each syllable start, and whose rate wanders
  by 3% (a perfectly even vibrato sounds mechanical). Inflection 0 switches
  it off. Rate sets the tempo; the voice's own `tempo` is ignored.
- **Sources** (`KlattSynth::source_sample()`): the glottal pulse, a
  PolyBLEP sawtooth (strings, reed, brass with a drive of 1 + 4·AV into
  tanh) and an additive organ (partials 1, 2, 4, 6, 8, 10, 12, 16 of the
  16' pitch; the fundamental weak and the energy on the 4th-8th harmonics,
  which is what MacinTalk's organ measures as). `chorus` adds two copies
  ±`chorus_cents` (the accordion only the sharp one), `sub_octave` one an
  octave down, `reverb` a Schroeder reverberator (four combs, two
  allpasses, Freeverb's lengths halved for 22 kHz). All of it goes through
  the cascade vocal tract, so the vowels and consonants are those of the
  voice: a talking organ, not an organ.
- **Levels.** `gain` per style brings the presets to about -14 dBFS RMS
  on running text (the speaking voices are at -18; sung vowels dominate).

Checking: `scratchpad/sing_check.py` of the 2026-10-07 session printed
the pitch track in 100 ms blocks; for each preset the blocks fall on the
written notes (C4+06, D4+02, E4-03 ... with the tracker's quantization),
the klapa and organ show the octave below as well, and the sevdah voice
±30-45 cents of vibrato. The speaking voices' output is byte-identical
before and after the change (A/B against a worktree build of HEAD).

## 7. Checking a change

```bash
B=build/macos-arm64-release
scons --platform=macos --build-config=release macos-all
clang++ -std=c++17 -I include -I tests/linux tests/linux/test_formant.cpp \
    -o $B/test_formant -L $B -llaprdus -Wl,-rpath,@loader_path
LAPRDUS_DATA=$B $B/test_formant
```

The tests cover behaviour (registry, parameters, intonation direction,
determinism, edge cases), not sound quality. For sound quality:

1. Measure, and compare with a recording of the same item (section 3).
2. Listen. Minimal pairs are the quickest check: *čaša / ćaća*, *sir / šir*,
   *kap / tap / pap*, *rad / lad / jad / vad*, *grad / glad*, *more / mole*.
3. Listen at a high rate. Screen reader users run these voices fast; a change
   that sounds better at normal speed can cost intelligibility there.

During development an automatic speech recognizer was also used as a rough,
objective intelligibility check (same sentences, several voices, character
error rate). It is useful for catching regressions and gross errors, but it
is noisy for Croatian and it also mis-hears recorded speech, so it cannot
settle fine phonetic questions. It does not replace listening.

For the record, when the voices were introduced a small general-purpose
recognizer (Whisper "small", Croatian/Serbian/Bosnian) made these character
errors on 20 everyday sentences that had not been used for tuning: Zvonko
10.5%, Stojan 10.8%, Mirsad 11.8%; a diphone voice built from recordings of a
Croatian speaker 11.1%; eSpeak NG Croatian 23.2%; Josip 33.1%.

## 8. Known limits

- Stress and vowel length are guessed for words outside the lexicon and the
  suffix rules. The four-accent system is fully realized only where the
  lexicon or the text marks it.
- Foreign words are read by Croatian letter-to-sound rules.
- The spelling dictionary (`data/dictionary/spelling.json`) names digits,
  punctuation and symbols in Croatian for all voices (*točka*, not *tačka*).
  Letters are named by the front end in the language of the voice
  (`formant::letter_name`), or spelled by their sounds
  (`Frontend::letter_sound`: an `Utterance` with `isolated_sound` set, whose
  durations the builder fixes: the consonant and a neutral release vowel
  after it, "bə", "sə", as eSpeak's Croatian voice sounds letters out).

## Sources

- D. H. Klatt, "Software for a cascade/parallel formant synthesizer",
  J. Acoust. Soc. Am. 67 (1980).
- D. H. Klatt, L. C. Klatt, "Analysis, synthesis, and perception of voice
  quality variations among female and male talkers", J. Acoust. Soc. Am. 87
  (1990) - the KLGLOTT88 source.
- S. R. Hertz, R. J. Younes, N. Zinovieva, "Language-universal and
  language-specific components in the multi-language ETI-Eloquence
  text-to-speech system", ICPhS 1999; S. R. Hertz, "Integration of rule-based
  formant synthesis and waveform concatenation", IEEE Workshop on Speech
  Synthesis 2002 - how Eloquence is built (phones and transitions, a
  KLSYN88-like synthesizer at 11025 Hz).
- D. H. Klatt, "Review of text-to-speech conversion for English",
  J. Acoust. Soc. Am. 82 (1987).
- A. Cohen, R. Collier, J. 't Hart, "Declination: construct or intrinsic
  feature of speech pitch?", Phonetica 39 (1982) - the IPO model.
- S. Godjevac, "Intonation, word order, and focus projection in
  Serbo-Croatian", Ohio State University 2000.
- J. Bakran, N. Stamenković, "Formanti prirodnih i sintetiziranih vokala
  hrvatskoga standardnoga govora", Govor VII (1990).
- J. Bakran, *Zvučna slika hrvatskoga govora*, Zagreb 1996.
- J. Bakran, studies of segment durations and speech tempo in Croatian
  (1984-1989), as cited in later work of the Zagreb Department of Phonetics.
- Descriptions of Serbo-Croatian phonology and accentuation (assimilation,
  allophones, clitics, distribution of the four accents).
- Wiktionary, Serbo-Croatian entries (accented headwords of names, surnames,
  towns and nouns in -ina), consulted October 2026; N. Anđić, *Naglasne
  osobitosti imena i prezimena*, Osijek 2021 (on the variation in personal
  names).
