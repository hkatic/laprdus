# The recorded voices: Josip and Vlado

Josip and Vlado are concatenative voices: one recording per sound (34 WAV
files each, `phonemes/<Voice>/PHONEME_*.wav`, 16-bit mono at 22050 Hz), packed
into `Josip.bin` and `Vlado.bin`. This document describes the engine that
turns those recordings into speech since October 2026, when the old engine
(raw concatenation with a 3 ms crossfade, rate and pitch through the Sonic
and Signalsmith Stretch libraries, a pitch "inflection" pasted onto the last
30% of each clause) was replaced. Nothing in it depends on a third-party
library.

## Why the old engine sounded the way it did

Measurements of the recordings (`tools/formant/` style analysis, repeated by
the `analyse_unit()` function at load time) explain the complaints:

- **Pops and crackles.** Most of Josip's files are cut in the middle of a
  period: `PHONEME_S.wav` starts at sample value -11201, `PHONEME_T.wav` ends
  at 25078, `PHONEME_B.wav` at -8176. A 3 ms linear crossfade cannot hide a
  step of that size, and the Sonic pitch envelope processed the audio in
  512-sample chunks with a discontinuity at every chunk edge.
- **Hard to understand.** The files are isolated sounds with no
  coarticulation, there was no pause or lengthening at word boundaries,
  Vlado's vowels are 200 ms long (a normal vowel in running speech is
  60-100 ms) and were played whole, so every word was a slow string of
  separate sounds, and stops lost their closures.
- **No accentuation, bad intonation.** The pitch was flat except for the
  last 30% of a clause, which was shifted as a block; the stressed syllable
  was never marked.
- **Pitch and rate artifacts.** Sonic's time-domain pitch shift resamples
  and time-stretches whole buffers (formants move with the pitch, the voice
  turns into a chipmunk or a giant), and the Signalsmith path fell back to
  Sonic for every segment shorter than 150 ms, which is most of them.

## The new engine

```
text  ─► formant::Frontend ─► Utterance (phones, stress, words, clause kind)
                                   │
                                   ▼
                     concat::plan_clause()  (concat_prosody.cpp)
                     durations, closures, word gaps, syllable timing,
                     pitch contour from formant::Intonation
                                   │
                                   ▼
                     concat::render()  (psola.cpp, TD-PSOLA)
                     one windowed period per output period, from
                     the pitch marks of concat::UnitBank (unit_bank.cpp)
```

### Analysis of the recordings (`src/audio/unit_bank.cpp`)

When a recorded voice is loaded (and again when its voice character pitch
changes), every recording is analysed once:

1. The DC offset is removed (Josip's `F` sits at +1115) and the first and
   last 2 ms are faded with a raised cosine, so a file cut inside a period
   cannot click at a clause edge.
2. A pitch track is computed every 5 ms by normalised autocorrelation over
   15 ms (60-400 Hz). The five vowels are analysed first without a prior,
   and the median of their pitch (Josip 139 Hz, Vlado 100 Hz) becomes the
   prior for the second pass: the search is narrowed to an octave around it
   and a local maximum near the prior wins over a slightly better one far
   from it. A frame is voiced when its correlation is at least 0.60, its
   level at least 5% of the loudest frame, and its period within ±45% of the
   prior (a burst that happens to correlate is not voicing).
3. In every voiced run, pitch marks are placed one per period at the peaks
   of the waveform low-passed to 2.5 times the voice's pitch (one peak per
   period; with the first formant left in, there can be two). The polarity
   with the larger peaks is chosen per recording, the strongest peak is the
   first mark, and the next is the best peak 0.7-1.3 periods away, forwards
   and backwards. Unvoiced parts get marks every 5 ms.
4. The sounding part (blocks above -34 dB of the loudest), and the sharpest
   rise in level over 3 ms within 12 dB of the peak: in a stop this is the
   burst. Josip's `B`, `D` and `G` are 52 ms of voiced murmur, then the
   burst and a short tail; his `K` has 52 ms of silence before it.

`unit.period[k]` is the local period of every mark (half the distance between
its neighbours), `unit.f0` the median pitch of the recording, and
`UnitBank::base_f0()` the median over the vowels.

A **voice character pitch** other than 1.0 (the derived voices: detence 1.5,
baba 1.2, djed 0.75) resamples the recordings by the square root of the
factor before the analysis (`AudioSynthesizer::formant_warp_for_pitch`): the
whole spectrum scales by 1.22 for detence, so the vocal tract sounds shorter,
and the pitch itself is then raised by the full factor by the renderer. A
child's formants are only about 20% above a man's while its pitch is 50-100%
higher, which is why the two no longer move together as they did with Sonic.

### Prosody (`src/audio/concat_prosody.cpp`)

The recorded voices use the text front end of the formant voices
(`formant::Frontend`): the same letter-to-sound rules, accent lexicon,
suffix rules, user accent lexicon (`accents.json`), clitics and assimilation.
Each phone of the resulting `Utterance` is mapped to a recording
(`unit_for()`: the allophones [ŋ], [dz], [ɕ], [ʑ], [ɣ] use the plain
recordings, dž uses `DJ`, the schwa of a syllabic consonant has none).
Letters without a Croatian sound (q, w, x, y) are spelled out by the front
end as they are for Zvonko.

**Durations** follow the rules of `ClauseBuilder::assign_durations()` in
`formant_synthesizer.cpp`: the inherent durations and rate floors of
`formant_phonemes.cpp`, stress and length (×1.35 / ×1.9), word length,
closed syllables, final lengthening (×1.4 before a full stop, ×1.3 before a
comma), clusters, the rate floor `min_dur / sqrt(speed)` above rate 1.0. The
recorded `R` is a trill and gets 50 ms rather than the tap's duration. The
segment of a vowel, nasal, liquid, glide or fricative is stretched or
compressed to its duration by the renderer. A **stop or affricate is never
stretched**: the burst and as much of the tail as the slot allows are played
at their natural rate, the time left over goes to the closure, first from
the recording's own closure or murmur, then as silence, and a fast rate cuts
the tail and the closure but never the burst (at least 20 ms of it). A
**22 ms gap** (floor 5 ms at fast rates) separates words; clitics join their
host without one.

**Pitch** comes from `formant::Intonation` (`src/formant/formant_intonation.cpp`),
the model extracted from the formant synthesizer so that both kinds of voices
share one melody: a movement on the stressed syllable of every content word
(shape by accent type, shrinking along the clause), a rise on the focused
word of a yes/no question and a final rise, a fall after the last accent of
a statement, the suspended rise of a comma, and a declining baseline of about
two semitones per second. The contour is sampled every millisecond, raised by
0.9 semitones decaying over 18 ms after a voiceless consonant, smoothed with a
16 ms time constant in both directions, scaled by the inflection level
(0 monotone, 0.5 as measured, 1 doubled) and multiplied onto
`natural_f0 × pitch × user_pitch`. The formant voices' own output is
unchanged by the extraction (verified sample for sample).

### Rendering (`src/audio/psola.cpp`)

Time-domain pitch-synchronous overlap-add (Moulines & Charpentier 1990), the
method of MBROLA, Festival's diphone voices and Praat's pitch manipulation:

- Output pitch marks are placed one target period apart (the period from the
  contour at that time). At each, one period of the recording is taken: a
  Hann window two local periods long centred on the analysis mark nearest
  the source position, where the source position is the output time mapped
  linearly onto the segment's source range (so a long recording is thinned
  and a short one has its periods repeated). Each window is scaled by
  `Ts/Ta` (clamped to 0.5-1.6), which keeps the mean gain at one whatever
  the pitch change.
- Unvoiced marks are placed every 5 ms regardless of pitch, with unit gain.
  When a frame must be repeated to lengthen a fricative, the repeat is played
  **backwards**, which prevents the buzz of a periodically repeated noise
  (the MBROLA trick).
- At a **join** between two voiced recordings, the output marks within 7 ms
  on either side of the boundary get windows from both recordings, weighted
  linearly across the 14 ms: the join is a crossfade of whole periods at the
  same pitch and the same phase (both marks sit on the main peak of their
  period), so there is no phase step and no click. A join with an unvoiced
  recording is the natural overlap of its windows.
- At a fresh start (clause start, after a pause or a closure) the first mark
  is one period in, so the window rises from zero instead of opening on a
  peak; the last window of a clause decays into the pause.
- The sum is limited softly above 90% of full scale (the recordings are
  clipped at the peaks already) and scaled by the volume.

Everything is deterministic: the same text and settings give the same
samples (`tests/linux/test_concat.cpp`).

## Ranges and settings

| Setting | Recorded voices | Where applied |
|---------|-----------------|---------------|
| speed | 0.5 - 4.0 | durations (planner); bursts and consonant floors keep a fast rate intelligible |
| acceleration | 0.5 - 3.0 | multiplies the speed (`VoiceParams::effective_speed()`), the product narrowed to 0.5 - 4.0 |
| pitch (voice character) | 0.25 - 4.0 | spectrum warp `sqrt(pitch)` at analysis, pitch × `pitch` at rendering |
| user_pitch | 0.5 - 2.0 | pitch × `user_pitch` at rendering, formants unchanged |
| inflection_enabled | | off: a flat contour at the voice's pitch (durations unchanged) |
| inflection_level | 0 - 1 | scales the whole contour, as for the formant voices |
| volume | 0 - 1 | gain before the limiter |

Spelling by letter sounds (`AudioSynthesizer::synthesize_letter_sound`) plays
the consonant's recording with the planner's fixed durations for an isolated
sound and the "e" recording after it as the release vowel ("bə", "sə", as
eSpeak sounds letters out); a vowel letter is the vowel alone.

## Testing and tuning

```bash
B=build/macos-arm64-release
clang++ -std=c++17 -I include -I tests/linux tests/linux/test_concat.cpp \
    -o $B/test_concat -L $B -llaprdus -Wl,-rpath,@loader_path
LAPRDUS_DATA=$B $B/test_concat      # 10 tests: rate, pitch, derived voices, melody, streaming
```

Things that are worth checking by ear after a change:

- a sentence with stops in every position (`Tata kupuje kaput.`, `Dobar dan.`)
  at rates 1, 2 and 4: the bursts must stay, the closures shrink first;
- the same sentence with a question mark and with a comma;
- `detence` and `djed` for the voice character, `-p 0.5` and `-p 2.0` for
  the user pitch (the lower limit is where PSOLA leaves gaps between periods
  and the voice gets hollow; the recordings allow about an octave each way);
- Vlado at rate 0.5, where every vowel period is repeated twice.

The analysis of every recording can be printed with a small tool compiled
from `unit_bank.cpp`, `phoneme_data.cpp` and `phoneme_mapper.cpp` (call
`concat::UnitBank::build()` and print `lead`, `onset`, `trail`, `f0`, the
marks and their periods); a regular vowel shows periods within ±2 samples of
each other, and a stop's `onset` sits on its burst.

## Sources

- E. Moulines, F. Charpentier, "Pitch-synchronous waveform processing
  techniques for text-to-speech synthesis using diphones", Speech
  Communication 9 (1990): TD-PSOLA, two-period Hann windows, marks every
  10 ms in unvoiced speech.
- T. Dutoit, H. Leich, "MBR-PSOLA: text-to-speech synthesis based on an MBE
  re-synthesis of the segments database", Speech Communication 13 (1993):
  pitch-synchronous joins, time reversal of repeated unvoiced frames.
- M. Legát, J. Matoušek, D. Tihelka, "On the detection of pitch marks using
  a robust multi-phase algorithm" (2007): pitch marks at the peaks of the
  low-passed waveform, tracked period by period.
- Duration and intonation rules: `docs/formant.md`.
