# The recorded voices: Josip and Vlado

Josip and Vlado are concatenative voices: one recording per sound (34 WAV
files each, `data/<language>/<Voice>/PHONEME_*.wav`, 16-bit mono at 22050 Hz), packed
into `Josip.bin` and `Vlado.bin` by `cargo xtask data`. This document describes the engine that
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
text  ─► languages::Frontend ─► Utterance (phones, stress, words, clause kind)
                                   │
                                   ▼
                     plan_clause()  (concatenative/prosody.rs)
                     durations, closures, word gaps, syllable timing,
                     pitch contour from prosody::Intonation
                                   │
                                   ▼
                     render()  (concatenative/psola.rs, TD-PSOLA)
                     one windowed period per output period, from
                     the pitch marks of UnitBank (concatenative/unit_bank.rs)
```

### Analysis of the recordings (`src/common/concatenative/unit_bank.rs`)

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
baba 1.2, djedo (Đedo) 0.75) resamples the recordings by the square root of the
factor before the analysis (`formant_warp_for_pitch()` in `src/common/concatenative/synthesizer.rs`): the
whole spectrum scales by 1.22 for detence, so the vocal tract sounds shorter,
and the pitch itself is then raised by the full factor by the renderer. A
child's formants are only about 20% above a man's while its pitch is 50-100%
higher, which is why the two no longer move together as they did with Sonic.

### Prosody (`src/common/concatenative/prosody.rs`)

The recorded voices use the text front end of the formant voices
(`languages::Frontend`, the front end of the language being spoken): the same letter-to-sound rules, accent lexicon,
suffix rules, user accent lexicon (`accents.json`), clitics and assimilation.
Each phone of the resulting `Utterance` is mapped to a recording
(`unit_for()`: the allophones [ŋ], [dz], [ɕ], [ʑ], [ɣ] use the plain
recordings, dž uses `DJ`, the schwa of a syllabic consonant has none).
Letters without a Croatian sound (q, w, x, y) are spelled out by the front
end as they are for Zvonko.

**Durations** follow the rules of `ClauseBuilder::assign_durations()` in
`src/common/formant/builder.rs`: the inherent durations and rate floors of
`src/common/formant/phones.rs`, stress and length (×1.35 / ×1.9), word length,
closed syllables, final lengthening (×1.4 before a full stop, ×1.3 before a
comma), clusters, the rate floor `min_dur / sqrt(speed)` above rate 1.0. The
recorded `R` is a trill and gets 50 ms rather than the tap's duration. The
segment of a vowel, nasal, liquid, glide or fricative is stretched or
compressed to its duration by the renderer. A **stop is never stretched**:
the burst and as much of the tail as the slot less a closure's time
(`CLOSURE_MS`, 18 ms, floor 8 ms) allows are played at their natural rate,
the time left over goes to the closure, first from the recording's own
closure or murmur, then as silence, and a fast rate cuts the tail but never
the burst (at least 20 ms of it). An **affricate** with less room than its
release is long is played faster instead, friction and all; a stop that
begins a word counts the word gap as a part of its closure (see "Clarity").
A
**22 ms gap** (floor 5 ms at fast rates) separates words; clitics join their
host without one unless two full vowels meet across the word boundary
(*vidio ih*). This keeps *video uradak* and *moju ulicu* distinct too,
without doubling the existing word gap. A word of one vowel (*i, a, u, o*)
also gets the gap next to *j* (*ja i ona*, *njoj i njenom*), or the sounds
run together into one. It is 1.30 times the vowel's inherent duration
instead of a clitic's 0.9, with no shortening for its neighbours (see
"Words of one vowel" in `docs/formant.md`). Vowels within a word and joined
compound components get no extra gap.

**Pitch** comes from `Intonation` (`src/common/prosody/intonation.rs`),
the model extracted from the formant synthesizer so that both kinds of voices
share one melody: a raised start of a sentence, a movement on the stressed
syllable of every content word (shape by accent type, the first one the
largest), a rise on the focused word of a yes/no question and a final rise,
a fall after the last accent of a statement, a small fall and a slight rise at a comma, and a
level that declines by 1.3 to 2 semitones over the clause (see "The melody
of read Croatian" in `docs/formant.md`). The contour is sampled every millisecond, raised by
0.9 semitones decaying over 18 ms after a voiceless consonant, smoothed with a
16 ms time constant in both directions, scaled by the inflection level
(0 monotone, 0.5 as measured, 1 doubled) and multiplied onto
`natural_f0 × pitch × user_pitch`. The formant voices' own output is
unchanged by the extraction (verified sample for sample).

### Rendering (`src/common/concatenative/psola.rs`)

Time-domain pitch-synchronous overlap-add (Moulines & Charpentier 1990), the
method of diphone synthesizers and of Praat's pitch manipulation:

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
  (a diphone synthesizer's trick).
- At a **join** between two voiced recordings, the output marks within 7 ms
  on either side of the boundary get windows from both recordings, weighted
  linearly across the 14 ms: the join is a crossfade of whole periods at the
  same pitch and the same phase (both marks sit on the main peak of their
  period), so there is no phase step and no click. A join with an unvoiced
  recording is the natural overlap of its windows.
- At a fresh start (clause start, after a pause or a closure) the first mark
  is one period in, and the first window's rising half is shortened to end
  at the segment's start if it would reach before it, so the window rises
  from zero instead of opening on a peak; the last window of a clause decays
  into the pause. (Before October 2026 the start was placed by the period of
  one mark and the window drawn with the period of another: Vlado's clauses
  opening on *u* began at -3197, a click at every one of them.)
- A window never reaches past its recording. Every recording is faded over
  its first and last 2 ms (`EDGE_FADE`), and the windows of marks close to
  the ends reached into that fade and beyond: the sound fell from full level
  to nothing within 2 ms, with digital silence after it (Josip's *dž*, -2 dB
  at its peak, then 3 ms of zeros before the *e*; the *j* of *pričekajte*
  into the closure of *t*). The renderer now takes the nearest mark further
  in whose whole window lies clear of the faded ends (one of the same kind
  if there is one within four marks), and only when there is none shortens
  the window's halves to what the recording holds.
- The sum is scaled by the volume and then held under 90% of full scale by
  the peak limiter of `src/common/audio/boost.rs` at unity gain (its gain moves only
  between syllables, so nothing is waveshaped; the peaks between the samples
  are counted too). Until November 2026 a soft knee bent everything above 90%:
  the recordings touch full scale, and at full volume Vlado and Đedo drove the
  knee so hard that above 5 kHz half of Đedo's sound was its distortion (the
  waveshaping residual of a paragraph, after the best gain per 2 ms, -31 dB
  for Đedo, -38 for Vlado, -45 for Josip; now -51, -55, -55, the true peak
  -0.6 to -0.9 dBTP, the vowels 0.1-0.5 dB quieter, Đedo's 1.2). An
  utterance that stays under the knee, at a lower volume for one, is what it
  was. In boost mode the boost's own limiter does the work instead.
- Before the volume, the hissing consonants are held under the vowels
  (`RECORDED_HISS_DB` in `src/common/params.rs`, through `limit_hiss_in()` of
  `audio/boost.rs`; -10 dB from 10 November 2026, **-4 dB and on the spans of
  the sibilants and f alone since 17 November**, see "Clarity"; the levels in
  the rest of this item are those of the -10 dB hold). In the recordings s, c, č, ć
  and the release of t stand level with the vowels (Josip's s -2.3 dB re the
  loudest vowel frame, his ć -0.1, Vlado's š, č and ć 0.0) and, weighted by
  the ear's sensitivity, 1-9 dB over them, where the softest of four
  commercial recorded Croatian voices has them 4-8 dB under. The hold puts
  Josip's s, c, č and ć and Vlado's š, č and ć at -5 to -10 dB and leaves
  Josip's š (-7.3 already), the vowels and every sample it does not cut as
  they were; boost mode keeps its own, lower hold (-22 dB). 0 turns it off.
  Vlado's s cannot be mended this way: his recordings hold nothing above
  5 kHz, so his s has its middle at 2.9 kHz, next to his š at 2.8. The
  measurements are in "The hiss where the ear is sharpest" in
  `docs/formant.md`; the test is
  `the_hiss_of_the_recorded_voices_is_held_under_the_vowels` in
  `tests/recorded.rs`.

Everything is deterministic: the same text and settings give the same
samples (`tests/recorded.rs`).

## Clarity (17 November 2026)

A listener's report: the recorded voices swallow their letters; the sounds
are unclear and hard to understand, all of them. What was measured, what was
changed and what cannot be.

**The measure.** `tools/formant/phones.py` renders vowel-consonant-vowel
items, syllables and words for every consonant (23 classes, 516 items for the
two voices) and gives them to a phoneme recognizer, which writes down the
phones it hears; an item counts when the consonant is among them. It is a
harsh judge of single syllables (it has no word to guess from) and a few
items either way are noise, but it showed what no level measurement had.
The right consonant, Josip and Vlado together:

| rate | before | now |
|---|---|---|
| 1.0 | 207 of 516 | 233 |
| 1.5 | 138 | 165 |
| 2.0 | 96 | 116 |

By class at 1.0, before and now: z 5 and 14 of 36, ž 6 and 11 of 34, š 15
and 18 of 18, c 9 and 12 of 22, ć 10 and 11 of 20, đ 10 and 12 of 20, p 11
and 14 of 20, t 10 and 8 of 20; at 1.5 č 1 and 6, ć 4 and 8, š 9 and 14, p 7
and 11; at 2.0 đ 1 and 6, ć 3 and 7, š 5 and 10, z 2 and 8, t 9 and 4. The
other classes moved by an item or two or not at all.

**The limit is in the recordings.** The same recordings put end to end with
no processing at all (the vowel twice, the consonant, the vowel twice) are
heard no better than the engine's rendering of the same item: Josip 57 of 115
raw and 58 rendered, Vlado 42 and 30. Every recording is one sound spoken or
cut out alone: the vowels are level stretches of 78 ms with no movement into
or out of a consonant, so the place of a stop or a nasal, which a listener
takes from the formant transitions of the vowels around it, is in the burst
or the murmur alone. What the recognizer hears instead says so: t and k as p,
g as b, d as t or p, n as m, nj as n, j as l, at every rate and raw as well.
No change to the planner or the renderer brings those back; recordings of
syllables or diphones would.

**What was changed** (each kept only where the recognizer heard more):

- *The release of an affricate that swells out of its closure.* Josip's c is
  within 9 dB of its loudest only from 45 ms of its 116 on, his đ from 45 of
  78, his ć from 20 of 78, with no abrupt rise, so the analysis left `onset`
  at the first sounding sample and the planner, which plays a stop from its
  onset and cuts the tail when the slot is short, kept the quiet build-up
  and cut the friction: at twice the rate c and đ had none left. The
  analysis now finds the strong part of every recording
  (`Unit::strong_begin..strong_end`: the 5 ms averages within
  `STRONG_LEVEL_DB`, 9 dB, of the loudest) and puts the onset at its start
  where no rise is found, and an affricate with less room than its release
  is long is played faster, from its start to its end, instead of cut
  (`plan_clause()`). The time the sound above 2 kHz stands within 20 dB of
  the vowel at 2x: c 60 to 84 ms, đ 40 to 80, ć 60 to 80 (the milder hold
  below has a share in it).
- *Closures.* A stop or affricate keeps `CLOSURE_MS` (18 ms, floor 8 ms at
  fast rates, at most 0.4 of the slot) before its release, from the
  recording's own closure or as silence: Vlado's p, k and affricates, whose
  releases are longer than their slots, had none at any rate. The closure of
  a stop that begins a word was not written at all (the word gap before it
  had reset the "after a sound" condition); the gap is now a part of it and
  the rest follows (*ona kaka*: 22 ms before the first k then, 58 now, 50
  inside a word). p gained at every rate; t lost at 2x (9 to 4 of 20, 6
  without the reserve), where the reserve takes 5 ms of its 31 ms release.
- *The voicing of z, ž and đ.* Their friction drowned the voicing in the
  correlation: Josip's z had no voiced mark, his ž and đ 2 and 3 of 15, and
  they were put together from 5 ms pieces out of step with their periods,
  with no join to the vowels. Their pitch is tracked on the sound under
  `MURMUR_HZ` (500 Hz), where the murmur is alone: 9 of 11, 9 of 11 and 10 of
  11 marks are voiced now. z went from 5 to 14 of 36.
- *The hiss hold* acts on the spans of the sibilants and f that the planner
  lays out (`Segment::hiss`, `limit_hiss_in()` of `audio/boost.rs`), not on
  whatever the detector finds: found by the sound alone it also took 6 dB off
  the burst of t and, its release being 15 ms, 2 to 8 dB off the band above
  2 kHz of the first 10 to 25 ms of the vowel after every sound it held. It
  now lets go within 1.5 ms of the span's end (the windows of a hiss reach
  5 ms into its neighbours, and the span is that much wider). And it holds
  less: `RECORDED_HISS_DB` is -4 dB, not -10. The sibilants and f at 1.0
  were right in 98 of 208 items at -10, 105 at -7, 108 at -4 and 111 unheld,
  at 1.5 in 68, 77, 80 and 81. The hold had been set on 10 November from
  level measurements against commercial voices, without a listening pass.
  Boost mode keeps its -22 dB, confined to the same spans.
- *The glide of "ije"* (34 ms) gives up the quiet build-up of the j recording
  (a hum for 38 ms, the glide for the last 20) before any of the glide.

**Tried and taken out again:** a join shorter than 14 ms where a segment is
short (no item changed at 1.5 or 2.0); giving up the quiet start of every
consonant short of time, not only the glide's (l, j, v and h a little better
at 1.5, l 14 to 9 of 38 and r 11 to 8 of 20 at 2.0); holding the burst of t
with the sibilants (no item changed).

The output of the recorded voices is no longer that of the C++ engine: of the
20,400 renderings of the `recorded` set of `tools/golden/golden.py` 1,616
keep their hash, and `tests/dsp_against_cpp.rs` no longer compares the
recorded voices. Tests: `affricates_keep_their_friction_at_fast_rates`,
`a_stop_that_begins_a_word_has_its_closure`,
`the_hiss_hold_lets_go_where_the_hiss_ends` and
`the_hiss_of_the_recorded_voices_is_held_under_the_vowels` in
`tests/recorded.rs`. To repeat the measurement (numpy, scipy, torch,
transformers; the model is 1.2 GB):

```bash
python tools/formant/phones.py --classes z,ž,s,š,c,č,ć,đ,t,k,p,b,g,d,f,h,r,j,v,l,nj,n,m \
    --voices josip,vlado --rates 1.0 --data data/voices old/laprdus.exe target/release/laprdus.exe
cargo run --release --example recorded_units -- data/hr/Josip   # what the analysis makes of every recording
```

## Ranges and settings

| Setting | Recorded voices | Where applied |
|---------|-----------------|---------------|
| speed | 0.5 - 4.0 | durations (planner); bursts and consonant floors keep a fast rate intelligible |
| acceleration | 0.5 - 3.0 | multiplies the speed (`VoiceParams::effective_speed()`), the product narrowed to 0.5 - 4.0 |
| pitch (voice character) | 0.25 - 4.0 | spectrum warp `sqrt(pitch)` at analysis, pitch × `pitch` at rendering |
| user_pitch | 0.5 - 2.0 | pitch × `user_pitch` at rendering, formants unchanged |
| inflection_level | 0 - 1 | scales the whole contour, as for the formant voices |
| head_size | 0 - 100 | the bank warp times `head_size_factor()` (1.25 at 0, 0.8 at 100), formants only, the pitch stays |
| brightness | 0 - 100 | a one-pole tilt after rendering (`tilt_brightness()`): the top folded away from an 800 Hz pivot at 0, the top above 1.5 kHz tripled at 100 |
| volume | 0 - 1 | gain before the limiter |

Spelling by letter sounds (`ConcatSynthesizer::synthesize_letter_sound()`) plays
the consonant's recording with the planner's fixed durations for an isolated
sound and the "e" recording after it as the release vowel ("bə", "sə", as
Synthesizer B sounds letters out); a vowel letter is the vowel alone.

## Testing and tuning

```bash
cargo xtask data                       # data/voices/Josip.bin, Vlado.bin (LAPRDUS_DATA names another directory)
cargo test --release --test recorded   # 28 tests: rate, pitch, derived voices, melody, streaming, clarity ...
```

Things that are worth checking by ear after a change:

- a sentence with stops in every position (`Tata kupuje kaput.`, `Dobar dan.`)
  at rates 1, 2 and 4: the bursts must stay, the closures shrink first;
- the same sentence with a question mark and with a comma;
- `detence` and `djedo` for the voice character, `-p 0.5` and `-p 2.0` for
  the user pitch (the lower limit is where PSOLA leaves gaps between periods
  and the voice gets hollow; the recordings allow about an octave each way);
- Vlado at rate 0.5, where every vowel period is repeated twice.

Clicks show without listening where the output meets digital silence: list
every run of at least 1 ms of zero samples and the largest sample within
0.5 ms on either side of it. A window that rises and falls properly leaves
less than about 500 there; a cut-off one leaves thousands. On 20 sentences
(the corpus of the October 2026 round) Josip had 357 such edges above 150
and Vlado 57, with peaks of 4150 and 3197; after the fixes above 113 and 16,
none above 490, all of them the tails of windows.

The analysis of every recording is printed by `examples/recorded_units.rs`
(`cargo run --release --example recorded_units -- data/hr/Josip`): length,
`lead`, `onset`, `trail`, the strong part, `f0`, how many marks are voiced,
and the level every 5 ms. A stop's `onset` sits on its burst, an affricate's
at the start of its friction, and z, ž and đ have voiced marks.

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
