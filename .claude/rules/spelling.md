---
paths:
  - "src/common/text/frontend/**"
  - "src/common/languages/*/words.rs"
  - "src/common/engine.rs"
  - "data/hr/spelling.json"
---
# Spelling

Moved from `CLAUDE.md` on 2026-11-23, unchanged. Claude Code loads this file when Claude reads a file matching the paths above; `CLAUDE.md` lists every topic file.

## Spelling

`Engine::synthesize_spelled()` (C API `laprdus_synthesize_spelled()`) reads text character by character, at the normal speech rate, with a pause after every character, including the last. It is what every platform calls for screen reader spelling, typed-character echo, SAPI5 `SPVA_SpellOut` and single-character requests. Per character (`spell_character()`):

- **A letter of the alphabet** (`spelling_letter()` in `src/common/text/frontend/letters.rs`: Latin or Cyrillic, any case, the ligatures and љ/њ/џ as digraphs, ä-type letters folded) is read, by default, by its **sound** (`SpellingMode::LetterSounds`, the default of `VoiceParams`, `UserSettings`, a `settings.json` without the key, the NVDA driver, Android and Apple since 2026-10-18; before that the names were the default and the switch went unnoticed) or, when `VoiceParams::spelling_mode` is `SpellingMode::LetterNames`, by its **name** in the language currently set (`Engine::language()`, for every voice; `letter_name()` of the language's `words.rs`: Croatian and Bosnian *a, be, ce, če, će, de, dže, đe, e, ef, ge, ha, i, je, ka, el, elj, em, en, enj, o, pe, er, es, eš, te, u, ve, ze, že*, foreign *ku, duplo ve, iks, ipsilon*; Serbian per decision 41 of the Odbor za standardizaciju srpskog jezika: *lje, nje, ša*, the same for Cyrillic and Latin; Slovenian per Slovenski pravopis § 7: the same names for its 25 letters, *dvojni ve* for w, *mehki če*/*mehki dže* for ć/đ). The sound: `Frontend::letter_sound()` builds an `Utterance` with `isolated_sound = true` (a vowel letter as a stressed syllable; a consonant letter as the consonant plus a stressed `Ph::Schwa` release vowel, "bə", "sə", "mə", as synthesizer B's Croatian voice sounds letters out: measured 70-90 ms of consonant and 110-130 ms of vowel at full level; the foreign letters *q, x, y* (`is_foreign_letter()` in `letters.rs`, which also has *w*) keep their names in sound mode too, and *w* is read by `expand_words()` as *duplo ve* / *dvojni ve* by names and *duplo v* / *dvojni v*, the v by its sound, by sounds, spelled and in running text alike, a listener's report), and `FormantSynthesizer::synthesize_letter_sound()` / `ConcatSynthesizer::synthesize_letter_sound()` render it with fixed durations. The same letter names are used for spelled-out abbreviations in running text ("USB", "HR").
- **The prefix of a web address** (`address_letters()` in `src/common/text/frontend/tokenizer.rs`) is read letter by letter in any case and every language and spelling mode, each letter with its accent: *www* is *ve ve ve*, *https* *ha te te pe es*, *http* *ha te te pe*, *ftp*, *ftps*, *sftp* likewise (a listener's report). In Slovenian any other abbreviation keeps one accent on its last letter, and two written apart are two abbreviations (USB PDF; they used to run together).
- **Everything else** (digits, punctuation, symbols) is read by its entry in the bundled spelling dictionary, or spoken as text when it has none; in Slovenian by the language's own names (`spelled_name()` in `src/common/languages/sl/words.rs`: ena, dva, pika, vejica, afna ...) before the bundled dictionary (a Slovenian 2 is *dve*). Since the migration to Rust a spelled `]` is named too: the C++ spelling dictionary parser could not read that entry and left it silent.
- **The user's spelling entries** (`add_spelling_entry()`, `append_spelling_dictionary()`, a second `SpellingDictionary` layer) win over all of that, with one exception: in sound mode a user entry that is merely a letter's name (`is_letter_name()` in `letters.rs`: the built-in names of any language, plus the old *jot*) gives way to the sound; any other entry is spoken as written. `load_spelling_dictionary*()` replaces the bundled layer and clears the user layer, so every platform's "load bundled, then add the user's" sequence keeps working.
- **Capital letters announced by a screen reader** arrive as text, not as a spelled character: NVDA "veliko N" (hr/sr/bs) and "cap N", TalkBack "veliko slovo N" (hr), "велико Н" (sr), "veliko N" (sr Latin, bs), VoiceOver "veliko početno slovo N" (hr). `Engine::synthesize()` (and `synthesize_streaming()`) recognize a whole request of *veliko*/*cap*/*capital*, optionally *slovo*/*početno*/*letter*, and one letter, and render the words as text followed by `synthesize_spelled()` of the letter, so the spelling mode, spelling speed and user spelling entries apply on every platform. Anything else ("Plan B", "veliko N je slovo") is ordinary text, where a lone letter follows the next rule.
- **A consonant letter standing alone in running text** (`lone_consonant_letter()` in `src/common/text/frontend/tokenizer.rs`: one consonant, or lj/nj/dž, as a word of its own) follows the spelling mode too, because that is how a screen reader reports a character inside a message: TalkBack "Obrisano B", VoiceOver's "B" and "deleted" as separate SSML parts, a capital announcement the host splits up. `Frontend::process(text, punct, letter_sounds)` (passed by both synthesizers from `VoiceParams::spelling_mode`) makes it the word "bə", the consonant and a stressed `Ph::Ax` (rendered by both kinds of voices inside running speech; the recorded voices use their e), a full word, never a clitic; in name mode it is the letter's name as before, and a lone digraph is one letter in both modes (Њ "nje", not "en je"). Not affected: a leaning preposition (s, k; Slovenian v, s, z, k, h before a word, see `leaning_preposition()`), the letters of an abbreviation (USB, BMW, s.a.r.s.), a letter before a hyphen (S-klasa), q/x/y (named in both modes) and vowels. Plain text therefore reads "Plan B", "F5" and "Mercedes S klasa" with the sound too, as synthesizer B's Croatian voice does. Tests: `src/common/languages/tests.rs` and `languages/sl/tests.rs` `a_lone_consonant_letter_is_its_sound_in_letter_sound_mode`, `tests/formant.rs` `a_lone_letter_in_running_text_follows_the_spelling_mode`, `tests/recorded.rs` `letter_sounds_work_with_a_recorded_voice`. NVDA marks a spelled capital by pitch itself: `PitchCommand(offset=capPitchChange)` before the letter and `PitchCommand()` after it, sent only to a synthesizer whose `supportedCommands` include `PitchCommand` (`speech.getSpellingSpeech()` sets the change to 0 otherwise). The add-on lists `PitchCommand`, `RateCommand` and `VolumeCommand`, carries the changes in force with every queued item and applies them in the speech thread just before synthesis (`_applyProsody()`: the slider value, or the middle of the slider when the Laprdus value is forced, plus the command's offset or times its multiplier, clamped to 0-100, then scaled as the slider would be), so NVDA's "Capital pitch change percentage" works for every voice, forced pitch included, and the next item comes back down; through the Laprdus SAPI5 voices too since 2026-11-23 (NVDA's SAPI5 driver writes the change as `<pitch absmiddle>` around the letter, and the SAPI5 driver now applies each fragment's pitch to that fragment alone: it took the first fragment's for the whole call, so a spelled capital raised the whole word or nothing); NVDA replays a change still in force at the start of every sequence it pushes, so each `speak()` starts from the sliders. `tests/nvda/test_driver.py` runs the driver under stubbed NVDA modules (`tests/nvda/nvda_stubs.py`, after NVDA's `speech/commands.py`) against the add-on's DLL and checks this, the spelling mode read from `settings.json` and its reload, rate and volume commands and the delayed character descriptions: `python tests/nvda/test_driver.py`.
- **Spelling speed** (`VoiceParams::spelling_speed`, 0-100, default 50): characters keep the normal speech rate and acceleration. `spelling_rate_factor()` (2^((s-100)/100): 1.0 at 100, 0.71 at 50, 0.57 at 20, 0.5 at 0) controls cadence only: after each character, append `pause_settings.spelling_pause_ms` plus its rendered duration × (1/factor − 1) of silence. Thus 100 adds no extra silence, 50 adds about 41% of the character duration, 20 about 74% (what 50 added before 2026-10-14, when the base was 3 and the default felt too slow), and 0 adds its whole duration. The last character gets the same pause, so whole-word and individual-character calls have identical spacing; `Engine::synthesize_spelled(text, false)` (it returns the audio and the pause's length in ms) / `laprdus_synthesize_spelled_ex()` leave that last pause out and report its length, for a host that times the gap after a spelled character itself. **NVDA's delayed character descriptions** ("Delayed descriptions for characters on cursor movement") speak the character as one utterance and then a second one of `BreakCommand(1000)` plus the description; NVDA pushes the second as soon as the first's index is reported. The add-on plays a `BreakCommand` as silence of its length (it used to become a space) and plays a spelled character without its trailing pause, owing that pause to the next item: another character or ordinary speech gets it (less the real time already spent in silence, so spelling a word keeps its cadence), a break cancels it, so NVDA's one second alone sets the gap before the description, and `cancel()` drops it. C API `laprdus_set_spelling_mode()` / `laprdus_set_spelling_speed()`; `settings.json` section `"spelling": { "speed": 50, "mode": "sounds" | "names" }` (parsed within the section, so its `speed` is not confused with `speech.speed`; only an explicit `"names"` turns the sounds off); keys `spelling_speed` / `spelling_mode` on Android and Apple; CLI `-s` (spell the text), `-S`, `-m`. Every settings screen has the spelling speed in its pauses section and a "spell letters by their sounds" switch, on by default, in its advanced/options section (Android: both in the Reading section, the switch named for what "on" does: *Slova kao glasovi* in hr/sr, *Črke kot glasovi* in sl, *Letters as sounds* in en, with the summary "Pri slovkanju i tipkanju" (sr "Pri sricanju i kucanju", sl "Pri črkovanju in tipkanju"); not *Sricanje*, which in Croatian and Serbian means spelling and suggested the letter names when on, and no example letters, which Laprdus reads by this very switch); NVDA takes both from `settings.json`. The voice settings are ordered speed, acceleration, pitch, inflection level, volume everywhere, then the volume boost and the four voice quality settings (head size, breathiness, roughness, brightness); Android shows speed, pitch and volume, then "More voice settings" with acceleration, inflection, volume boost and the four voice quality settings. The inflection on/off switch of version 1.0 is gone everywhere (`VoiceParams`, the C API, `settings.json`, the Android and Apple settings keys): the inflection level at 0 is the monotone, and a 1.0 file's `speech.inflection` is not read, since it controlled a different effect (a pitch jump at the punctuation).
- Tests: the spelling cases of `tests/formant.rs` and `tests/recorded.rs` (their names contain `spell` or `letter`).

## Spelling Dictionary

The spelling dictionary `data/hr/spelling.json` (staged as `data/dictionary/spelling.json`) maps individual characters to their pronunciations for screen reader spelling mode (character-by-character reading). It holds digits, punctuation and symbols only; letters are named by the engine (see "Spelling" above).

### Format

```json
{
    "version": "1.0",
    "description": "Character pronunciation dictionary for spelling mode",
    "entries": [
        { "character": "B", "pronunciation": "Be" },
        { "character": "Č", "pronunciation": "Če" },
        { "character": ".", "pronunciation": "točka" }
    ]
}
```

### Character Categories

The spelling dictionary includes:
- **Numbers**: 0-9 (spoken as Croatian words)
- **Punctuation**: Common symbols with Croatian names
- **Special characters**: @, #, $, etc.

### Usage

- **NVDA**: Automatically used in character mode (reading character-by-character)
- **SAPI5**: Used with SPVA_SpellOut action (spell commands)
- **Android**: Available via `synthesizeSpelled()` API

### Rust API

```rust
use laprdus::engine::Engine;
use std::path::Path;

let mut engine = Engine::new();
engine.set_voice("zvonko", Path::new("data/voices"))?;

// Load spelling dictionary
engine.load_spelling_dictionary_file(Path::new("/path/to/spelling.json"));

// Synthesize text in spelling mode: the samples and the length of the last pause
let (samples, _final_pause_ms) = engine.synthesize_spelled("ABC", true)?;
// Result audio: "A Be Ce"
```

### C API

```c
laprdus_load_spelling_dictionary(handle, "/path/to/spelling.json");
laprdus_synthesize_spelled(handle, "ABC", &samples, &format);
```
