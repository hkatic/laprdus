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
  ones make a /k/ burst sit on F2 and a /t/ burst on F3-F5 *of that moment*, so
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
- /t d/ are **dental**: the burst is diffuse, not concentrated high up.
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
  ć 3.95 kHz; before 3.75 and 4.15 kHz), after a listening test asked for
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
  | l | -5 to -8 dB | F1 455, F2 1150 (follows the vowel before it), F3 2500, F4 2900 |
  | lj | -4 to -10 dB | F1 335, F2 1800, F3 2820 |
  | j | -1 to -5 dB | F2 1850 between a's (strongly coarticulated) |
  | m, n, nj (murmur) | -4 to -13 dB | F1 300; F2 1150 / 1350 / 1350-1500 |
  | r (between vowels) | -15 dB, dips to -24 dB | F1 400, F2 1400, F3 2550; 50 ms |
  | k burst | -20 dB | on F2 (and 3.5 kHz); voice onset 40 ms |
  | t burst | -20 dB | F2, F3, F4 all excited; voice onset 15 ms |
  | p burst | very weak | voice onset under 10 ms |
  | voice bar (b, d, g) | -13 to -25 dB | below 500 Hz |
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
  Eloquence (-6 to -10 dB). It is now about -20 dB, so voiced and voiceless
  stops are further apart.
- Bursts of p, t, k already matched Eloquence in level and length and were
  left alone. Klatt's own advice applies: exaggerated cues ("super speech")
  have always tested worse than a closer match to natural data.

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
- **Syllabic r**: r with no vowel next to it (*prst*, *Hrvatska*, *žanr*).
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
   *-ura*, *-tika*, *-ator*, penultimate stress for *-ent*, *-ist*, *-fon* ...),
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
following monosyllable (*nè znām*), in Serbian and Bosnian also of common
two-syllable verb forms (*nè mogu*). Bosnian additionally moves the accent
onto prepositions before a fixed list of words (*ù grād*, *nà more*).

To correct a word, add an entry to `formant_lexicon.cpp` (notation at the top
of the file). Language-specific tables override the common one (*pròfesor* in
Croatian, *profèsor* in Serbian and Bosnian).

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
  - statement and wh-question: a drop of 4 (3.5) semitones completed within
    180 ms of the last accent, after which the voice stays low; a wh-question
    also has its highest peak on the question word,
  - exclamation: wider movements throughout, a raised first and last accent
    and a drop of 5.5 semitones,
  - comma, semicolon, colon: a rise of 4 semitones over the last syllable,
  - yes/no question: low on the stressed syllable of the focused word, 7
    semitones up right after it (the "inverse" pattern of Croatian and
    Serbian questions); the focused word is the one before *li* if there is
    one, otherwise the last one,
- small segmental effects (high vowels slightly higher, a dip in voiced
  obstruents, a raised onset after voiceless ones).

The contour is smoothed in both directions. With inflection switched off only
the segmental effects remain.

A clause of a single syllable (a letter name while spelling, *da*, *ne*) gets
less than half of these movements. A full sentence melody squeezed into one
syllable was a 6-semitone glide, steep enough to hide the pitch step a screen
reader uses to mark a capital letter.

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
| č / ć | closer together | clearly apart | clearly apart |
| /h/ | weak | weak | strong |
| Numbers | tisuća, milijun | hiljada, milion, dvesta | hiljada, milion |
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
