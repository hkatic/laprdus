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

The first two steps are shared with the concatenative voices.

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
  (HR, USB, NVDA); a few common ones are expanded (npr., tj., itd.).

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
   three-syllable *Pètrović*, *Màrković* keep the first), for agent nouns in
   *-ač* and *-ačica* (*prodàvāč*, *pjevàčica*) and for nouns in *-ina*
   (*brzìna*, *planìna*, *veličìna*, *Katarína*; *-ovina/-evina* on the
   syllable before: *dòmovina*, *králjevina*),
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

**A word is more than its dictionary form.** The accent often sits somewhere
else in the other cases and persons, and a stem entry (`kontr'o:l*`) gives
every form the same accent. Before adding a word, look at its whole paradigm:

- *Rules have a lower limit.* The rule for verbal nouns in *-enje* starts at
  four syllables, because the three-syllable ones go both ways: *rođénje,
  rješénje, kršténje, pošténje, snižénje* are lexicon entries, *ùčenje,
  mìšljenje, vȉđenje* are right by default. (*Rȍđendan* is not *rođénje*.)
- *Moving accent*: *sìgnal* but *signála, signálu, signáli*; *telèfon* but
  *telefóna, telefónom*; *novčànīk* but
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
the nouns in *-tak*, the verbs, the names *Zvónko* and *Vládo*) follow
Hrvatski jezični portal and the declension and conjugation
tables of Wiktionary. Three choices go with common Croatian
speech rather than the dictionary, at the request of a native listener:

- unstressed length is left out where it is not heard (*dȍdatno, kòntrolni,
  sìgnal* for the dictionaries' *dȍdātno, kòntrōlnī, sìgnāl*);
- nouns in *-itet* keep the long *e* prominent in the nominative too:
  Zvonko says *kapacitét, identitét* next to *kapacitéta*, where the
  dictionaries have *kapacìtēt*. Stojan and Mirsad follow the dictionaries;
- of *obavijestimo* and *obavijestite*, which are both present and
  imperative, the first is read as present (*obàvijēstīmo*) and the second
  as imperative (*obavijéstite*), the more frequent use of each.
- verbs keep the accent of the infinitive in every form for Zvonko (see
  below).

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
  keeps the accent of the infinitive in every form (*urédim, uréđen,
  otvòrim, podijélim, pokrénem*), as Croatian is commonly spoken and as a
  native listener asked for the verbs with a long vowel; the verbs with a
  short one follow for consistency. *Obavijestiti, razumijevati* and the
  present of *započeti, preuzeti*, which are lexicon entries, do the same.
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

To add a verb, run `tools/formant/verb_accents.py urediti otvoriti ...`: it
looks each infinitive up and prints the table entry, the dictionary's forms
and any noun twin (or says that the verb needs no entry, or that it is
irregular and belongs in the lexicon). Then check the forms at the head of a
clause and inside one.

## 5. Timing and melody (`formant_synthesizer.cpp`)

**Durations** start from the inherent value and are scaled by context:
stressed and long vowels are longer, vowels before voiceless consonants and
in closed syllables shorter, consonants in clusters shorter, the last
syllable of a clause longer. The rate setting scales durations directly, with
a floor per sound so that consonant cues survive at very high rates;
transitions shrink less than steady states.

**Pitch** is the sum of:

- a baseline that drifts down along the clause (declination), about two
  semitones per second, at most six in all,
- one movement per accented word: falling accents peak early in the stressed
  vowel, rising accents peak in the following syllable. The first is the
  largest (4.5 semitones), later ones shrink gradually,
- a boundary movement chosen by the punctuation that ended the clause:
  - statement: a drop of 4 semitones completed within 180 ms of the last
    accent, after which the voice stays low,
  - exclamation: wider movements throughout, a raised first and last accent
    and a drop of 5.5 semitones,
  - comma, semicolon, colon: a rise of 4 semitones over the last syllable,
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

A clause of a single syllable (a letter name while spelling, *da*, *ne*) gets
less than half of these movements. A full sentence melody squeezed into one
syllable was a 6-semitone glide, steep enough to hide the pitch step a screen
reader uses to mark a capital letter. That holds for statements only: a
one-syllable question keeps its whole rise and a one-syllable exclamation
(*Ne!*, *Stoj!*) 70% of its movements, or it could not be told from *Ne.*

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
- The spelling dictionary (`data/dictionary/spelling.json`) is Croatian for
  all voices (*točka*, not *tačka*).

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
