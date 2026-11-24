---
paths:
  - "src/common/text/numbers/**"
  - "src/common/text/marks.rs"
  - "src/common/text/clauses.rs"
  - "src/common/languages/*/numerals.rs"
  - "src/common/languages/sl/nouns.rs"
  - "src/platform/windows/nvda/addon/globalPlugins/laprdus/*.py"
  - "tests/nvda/test_global_plugin.py"
  - "tests/numbers_against_cpp.rs"
---
# Numbers, telephone numbers, currency, dates

Moved from `CLAUDE.md` on 2026-11-23, unchanged. Claude Code loads this file when Claude reads a file matching the paths above; `CLAUDE.md` lists every topic file.

## The number words of each language

- **Number words follow the language**, for every voice: `Engine::set_language()` makes a `NumberReader` (`src/common/text/numbers/`) for the language, whose number words and grammar are the language's own `Grammar` (`src/common/languages/<code>/numerals.rs`: tisuća/milijun/dvjesto for hr, hiljada/milion/dvesta for sr, hiljada/milion/dvjesto for bs, the Slovenian system for sl), starting from the voice's native language in `Engine::set_voice()`; Josip speaking Serbian says *hiljada*, Stojan speaking Croatian *tisuća*. One and two agree with the feminine scale words in every dialect (dvije tisuće, dve hiljade, dvadeset jedna tisuća, dvije milijarde). Slovenian (Slovenski pravopis 2001 § 11, § 536, § 253): nič, ena, dve, tri, štiri, and 1 to 4 agree with what they count, by the next word's ending and the preposition before the number (`agree()` in `src/common/languages/sl/numerals.rs`, a listener's report of November 2026: en koš, ena knjiga, eno leto, ena stvar; dva koša, dve uri, sto dve leti; trije koši, tri hiše, tri stvari; z dvema žličkama, s tremi žlicami, v dveh urah, do enega leta, v eni uri, pred enim letom; en kg, dve min), the plain form where they count nothing or the next word could be something else: ena, tri, štiri, and dve, the name of the number (a listener's second report asked for it where the 2 stands alone; SP § 11, SSKJ² ena in ena je dve and Jezikovna svetovalnica "Uporaba dva in dve" give the rest: 2, 1, 2, 3, 0 1 2 3, 2 plus 2, 2 je sodo, pritisni 2, stran 2, slika 2, poglavje 2, the typed digit, digits read one by one); dva only in a longer number (sto dva, dva tisoč dva) or a name or after a masculine noun (T2, T-2, PS/2, A-2 liga, 2kg, 2-krat, Pentium 2, člen 2, monitor 2, 2 %); dve also on the clock and in a decimal (Ura je 2, 2:30, 12:02, dve celi pet); enaindvajset as one word; the invariable tisoč (dva tisoč, tristo en tisoč); en milijon, dva milijona, trije milijoni, štirje milijoni, pet milijonov, enaindvajset milijonov; ena milijarda, dve milijardi, tri milijarde, pet milijard; the decimal comma as cela (tri cele štirinajst, ena cela pet, nič celih pet), a period between groups of three digits as the thousands separator (132.457 sto dvaintrideset tisoč štiristo sedeminpetdeset; 3.14 stays tri pika štirinajst), a numeric date in the genitive (7. 10. 2026 sedmega desetega dva tisoč šestindvajset, 7. oktobra sedmega oktobra), but in the nominative where the sentence names the day (a listener's report of 2026-11-17: Danes je 16.4.2026 danes je šestnajsti četrti dva tisoč šestindvajset, with or without spaces, Danes je 16. 4.; `names_the_day()` in `sl/numerals.rs`: after danes, jutri, včeraj, pojutrišnjem, predvčerajšnjim, datum, to, dan and je or bo, and je bil; Danes smo 16.4.2026 and Rodil se je 16. 4. stay the genitive). Since 2026-11-03 (the maintainer's request that the numerals match the spelling and grammar; `number_at()` of `src/common/languages/sl/numerals.rs`, the language's hook (`Grammar::number_at()`) that the shared loop of `NumberReader` calls first, plus `agree()` and `ordinal_before()` there, the Slovenian side of the agreement and of the silent-period block; all in the Slovenian folder, hr/sr/bs audio unchanged): a number with a period before a lowercase word is the ordinal in the case the noun's ending and the preposition show (`1. mesto` prvo mesto, `na 1. mestu` na prvem mestu, `v 21. stoletju` v enaindvajsetem stoletju, `3. člen` tretji člen, `1. stran` prva stran, `7. oktober` sedmi oktober, `v 90. letih` v devetdesetih letih, `odstavka 3. člena` tretjega, `7. 10.` sedmega desetega, `za 3. nagrado` za tretjo nagrado but `3. mesto` tretje mesto by the generated table of neuter nouns in -o `NEUTER_O` (`sl/tables/neuter_o.txt`, read by `sl/nouns.rs`), `videl sem 3. otroka` tretjega; the cardinal where nothing is counted, `2. je`, `1.` alone); `videl sem 1 otroka` videl sem enega otroka and `1 kolega` en kolega by the generated `NOUN_A` (`sl/tables/noun_a.txt`; masculine genitives, animate accusatives and nominatives in -a; `ena miza` stays); 5 and above decline where a preposition or an unambiguous ending asks (`v 5 urah` v petih urah, `s 5 prsti` s petimi prsti, `pred 21 leti` pred enaindvajsetimi leti, `do 5 let` do petih let; sto and tisoč stay); the unit signs `%`, `€`, `$`, `°`, `°C` are nouns agreeing with the number (`1 %` en odstotek, `2 €` dva evra, `3 %` trije odstotki, `za 3 €` za tri evre, `5 °C` pet stopinj Celzija, `1,5 %` ena cela pet odstotka) and in the case of the preposition before them (`do 5 %` do petih odstotkov, `do 1 %` do enega odstotka, `pri 20 °C` pri dvajsetih stopinjah Celzija, `z 2 €` z dvema evroma, `k 5 €` k petim evrom, `za 1 °C` za eno stopinjo Celzija; `0 °C` nič stopinj in every case; `unit_case()`, `Case`); unit abbreviations by gender (`1 min` ena min, `1 MB` en MB, `2 tisoč` dva tisoč); a glued minus is minus (`-5 °C`); thousands with spaces are one number (`1 000 000` milijon; not after a digit, a bracket, telefon or a name); a range reads do (`1991–2000`, `2–3 dni`, `15.–20. 3.` petnajstega do dvajsetega tretjega, `3.–4. mesto`, `v 7. in 8. razredu`; a hyphen only between small numbers or two years), also written with words, both numbers in the preposition's case (`od 5 do 10 %` od petih do desetih odstotkov, `med 5 in 10 °C` med petimi in desetimi stopinjami Celzija, `od 2 do 3 ur` od dveh do treh ur, `starosti 2 do 3 let` dveh do treh; the first number plain when the second is, `2 do 3 ure` dve do tri ure, `za 2 do 5 kilogramov` dva do pet; `range_left()`); `2-krat`, `2x`, `2-letni`, `10-letnica` are the one word dvakrat, dvoletni, desetletnica; the hour after ob is the locative (`ob 5`, `ob 5h`, `ob 17:00` ob petih, ob sedemnajstih), but before a form of ura the feminine ordinal in its case, the ura read as written (a listener's report of 2026-11-24; `hour_before_ura()`: `ob 10:00 uri`, `ob 10.00 uri`, `ob 10 uri` ob deseti uri, `ob 10 minuti` ob deseti minuti, `do 17:00 ure` do sedemnajste ure, `pred 10:00 uro` pred deseto uro, `od 8:00 do 16:00 ure` od osme do šestnajste ure, `ob 10:00 in 11:00 uri` ob deseti in enajsti uri; with minutes the ura before them and the minutes in the nominative, `ob 10:30 uri` ob deseti uri in trideset minut, `ob 11:02 uri` in dve minuti, `za 23.35 uro`; a colon time with a leading zero as without it, `ob 08:00 uri` ob osmi uri, `ob 08:00` ob osmih; SSKJ² úra ob peti uri [5. uri] in dvajset minut, SP 2001 úra, sedemnájsti, devetnájsti, the Jezikovna svetovalnica; `v 1 uri` stays v eni uri; test `slovenian_listeners_report_of_2026_11_24_whole_hours_before_uri`), `ob 8.30` and `od 8.30 do 16.00` are times, `24:00` too. Sources: SP 2001 pét pêtih, stó stôtih tudi neskl., tísoč -- tudi -ih, pŕvi, trétji -a -e, sêdmi, stôti, tisóči, dvákrat, dvóléten, odstótek, évro, stopínja, Célzij, mínus; Pravopis 8.0 § 26. Tests: `tests/formant.rs` table `NUMERALS` (in `slovenian_is_spoken_by_every_voice`) and `src/common/text/numbers/tests.rs` `slovenian_ordinal_noun_agreement`; details and limits in `docs/formant.md` 4a "The numerals' grammar". **Croatian, Serbian and Bosnian (2026-11-03, the same request for the other three languages; `number_at()`, `agree()`, `ordinal()`, `ordinal_before()`, `count()` of `src/common/languages/hr/numerals.rs`, with a full copy in `sr/` and in `bs/`; Serbian Cyrillic transliterated for the word checks by `latin()` of `src/common/text/numbers/scan.rs`):** *jedan, jedna, jedno* and *dva, dvije* (sr *dve*) agree with the noun after them (`1 kuća` jedna kuća, `2 kuće` dvije kuće, `1 selo` jedno selo, `1 dijete` jedno dijete, `1 stvar` jedna stvar, `2 noći` dvije noći, `21 kuća` dvadeset jedna kuća, `1 auto` jedan auto), 1 declines after a preposition (`s 1 prijateljem` s jednim prijateljem, `u 1 satu` u jednom satu, `u 1 godini` u jednoj godini, `do 1 godine` do jedne godine, `od 1 sata` od jednog sata, `za 1 kunu` za jednu kunu), 3 and 4 and the oblique forms of 2 are left alone; nothing counted keeps *jedan, dva* (`1 i 2`, `stranica 2`, `T2`, `Pentium 2`, `u 1 ujutro`). Ordinals with a period before a noun take the case the noun's ending and the preposition show (`1. mjesto` prvo mjesto, `na 1. mjestu` na prvom mjestu, `u 2. razredu` u drugom razredu, `u 21. stoljeću` u dvadeset prvom stoljeću, `3. članak` treći članak, `1. stranica` prva stranica, `7. listopad` sedmi listopad, `7. listopada` sedmog listopada, `1990. godine` tisuću devetsto devedesete godine, `u 1990. godini` devedesetoj, `u 90. godinama` devedesetim, `konac 80. godina` osamdesetih, `do 3. razreda` trećeg, `početak 2. svjetskog rata` drugog, `osvojio je 1. nagradu` prvu, `To je 1. liga` prva; the cardinal where nothing is counted, `1. je`, `1.` alone, `2026.` before a capital); a full date is read in the genitive with the year as the feminine ordinal of *godine* (`7. 10. 2026.` sedmog desetog dvije tisuće dvadeset šeste, sr dve hiljade), in the nominative after *danas je / datum je* (`Danas je 7. 10.` sedmi deseti), and `7. 10.` without a year sedmog desetog; the unit signs are nouns (`1 €` jedan euro, `2 €` dva eura, `5 €` pet eura, `21 €` dvadeset jedan euro, `1,5 €` jedan zarez pet eura, `1 $` jedan dolar, `1 %` jedan posto, `1 °C` jedan stupanj Celzija, `2 °C` dva stupnja, `5 °C` pet stupnjeva Celzija; sr `1 €` jedan evro, sr/bs `5 °C` pet stepeni Celzijusa); unit abbreviations by gender (`1 min` jedna min, `2 min` dvije min, `1 kg` jedan kg); a glued minus is *minus* (`-5 °C` minus pet stupnjeva Celzija); thousands with a period or a space are one number (`1.000` tisuću, `1 000 000` milijun, `1.000,50` tisuću zarez pedeset, `1.000 €` tisuću eura; not after a digit, a bracket, *telefon*, *broj* or a name); a range reads *do* (`1991.–2000.` and `od 1991. do 2000.` tisuću devetsto devedeset prve do dvije tisućite, `2-3 dana` dva do tri dana, `10-15 %` deset do petnaest posto, `15.–20. ožujka` petnaestog do dvadesetog ožujka, `3.–4. mjesto` treće do četvrto mjesto, `u 7. i 8. razredu` u sedmom i osmom razredu; a hyphen only between small numbers or two years, `716-844` stays); `2x` dva puta, `1x` jedanput, `2-godišnji` dvogodišnji, `3-godišnja` trogodišnja, `10-godišnjica` desetogodišnjica, `4-sobni` četverosobni (sr/bs četvorosobni), `57-godišnji` pedesetsedmogodišnji; `u 8.30`, `od 8.30 do 16.00`, `Sada je 8.30` are times, `24:00` too; `u 5` stays *u pet*. Tests: `tests/formant.rs` `croatian_serbian_and_bosnian_numerals_agree_with_their_nouns`, `src/common/text/numbers/tests.rs` `croatian_serbian_bosnian_ordinal_cases`.

### Telephone numbers

`convert()` of `src/common/text/numbers/phone.rs` runs before grammatical number rules
in both reading modes. `+` / `00` international numbers and domestic leading-zero
numbers accept compact digits, spaces, slashes, dashes and parenthesized codes.
Country code, area/network code and the subscriber number after its third digit
are separated by the user's comma pause (`+385 91 1234567` → `+385 | 91 | 123 | 4567`,
six digits `123 | 456`; up to four digits are one group, eight are 4+4).
Digit mode uses standalone digit names (Slovenian *dve*); whole-number mode reads
cardinal groups. Surrounding words never inflect phone groups. Preserve every
leading zero, including optional `(0)` and Italy's significant zero. A screen
reader may take the plus sign before the engine sees it (NVDA at the
punctuation level "none" sends a space for it, at higher levels the word): a
number after the word *plus* / *плус* (`plus_word_before()`; not after a
number, `2 plus 385`) is international like one after the sign, and with
nothing at all a number that begins with 385, 386, 387 or 381 and goes on as
a national number of that country by first digit and length
(`bare_international_ok()`; written together or with the code as the first
group; not groups of three throughout, `385 912 345 678`) is one too:
` 385 91 123 4567  `, `385911234567`. `385 kuna`, `385 911 234` and another
country's number without its sign stay numbers. Country
codes and regional exceptions, ambiguity limits and sources are documented in
`docs/formant.md`, "Telephone numbers". Tests: the `telephone_*` cases of
`src/common/text/numbers/tests.rs` and the audio tests
`telephone_grouping_reaches_the_audio_and_honors_comma_pauses` (`tests/formant.rs`)
and `recorded_voices_pause_between_telephone_groups` (`tests/recorded.rs`).

### Currency amounts

`convert()` of `src/common/text/numbers/currency.rs` recognizes clear grouped decimals such as
`2.150,00` before the phone/identifier guards, so UI labels such as *Stanje*
and *Račun* cannot turn the thousands separator into a spoken dot. Explicit
`€` / `$` / `EUR` / `USD` amounts accept prefix/suffix currency, European and
US separators, space/NBSP/thin-space and apostrophe grouping, Unicode minus,
negative suffixes and accounting parentheses. Minus, decimal and currency
become words before punctuation segmentation; currency follows the amount
with the existing language's grammatical forms. Preserve fractional zeros
and the number mode. Digit mode also speaks an ordinary unary minus. Do not
infer a negative value from parentheses without a currency marker or mistake
an identifier/range hyphen for a minus. Sources, ambiguities and limits are in
`docs/formant.md`, "Currency amounts"; the currency tests cover normalization
(`currency_*` in `src/common/text/numbers/tests.rs`) and both audio renderers
(`currency_amounts_speak_their_value_minus_and_unit_before_punctuation` in
`tests/formant.rs`, `recorded_voices_speak_currency_signs_and_localized_amounts`
in `tests/recorded.rs`).

### Short pauses inside a clause: clock times, dates, cell addresses

A listener's report of November 2026: a screen reader says a spreadsheet
cell's contents and then its address, and "Marjan A17" was heard as "Marijana
17" (the A was the conjunction *a*, unstressed and leaning on the number); the
hours of a clock time ran into its minutes and the day of a date into the
month. The number converter now leaves three marks in the text
(`src/common/text/marks.rs`; Unicode noncharacters, which no host's text
contains, where a private use character can come from an icon font):

- `MINOR_BREAK_MARK` (U+FDD0, written as a word of its own): a short pause
  before the next word. Where: between the hours, minutes and seconds of a
  clock time (`12:30:45`, also `u 8.30`, `24:00`, Slovenian `ob 8.30`) and
  between the day, month and year of a numeric date (`10.11.2026.`,
  `10. 11. 2026.`, `5.3.`, the right side of `15.–20. 3.`).
- `ITEM_BREAK_MARK` (U+FDD2, written the same way): the pause between two
  items a screen reader says in one breath, the **whole comma pause** at every
  rate (`item_break_pause_ms()`; `Word::item_break`, `Phone::item_break`,
  `Utterance::item_break` next to the short pause's flags). Where: before a
  cell's address that follows other words (`Marjan A17`, `1250 C3`, `A1 B2`)
  and before a number that follows an address (`A20 5` is not A25). Not at the
  head of a clause, not after a preposition or conjunction (`u A1`,
  `od A1 do B5`, `v A1`; `leans_on_next()`), not in a range or formula
  (`A1:B5`, `=A1+B2`). It was the short pause until 2026-11-10: NVDA sends a
  cell as `Marjan   A17  ` (two items of one sequence, two spaces after each,
  joined by the driver), the engine paused for 54 ms at NVDA's rate 60, and
  the listener heard no pause in Excel. And still none after that: the
  driver's debug log showed that NVDA's Excel support says the address as two
  words, `'Marijan  '` then `'A 3  '`. `spaced_code_begin()` takes that form
  as an address too, but only as an item of its own, since written apart the
  letters are usually a word (`U 5 sati`, `A 3 su došla`, `NA 5 mjesta`): one
  space between one or two capitals and the number, and after the number the
  end of the text, of the line, or two spaces (the end of a screen reader's
  item); not after a word of three or more capitals (`DOLAZIM U 5`). To see
  what NVDA sends, create `%TEMP%\laprdus_debug` and restart NVDA: the driver
  logs every sequence to `%TEMP%\laprdus_debug.log`. Do that before believing
  a replay of what NVDA "should" send.
- `LETTER_CODE_MARK` (U+FDD1, right after the letters): one or two capital
  letters of the English alphabet glued to a number of up to seven digits
  (`A17`, `F5`, `S22`, `U21`, `AB12`, `H2O`, `A4-format`; `code_begin()`) are
  letters, not the words they spell: one letter by the spelling mode like any
  lone letter (*bə* or *be*; a vowel is a stressed word of its own), two by
  their names (*NA5* is *en a pet*). Not small letters (`u20:03` is the
  preposition), not three letters (`TOP10`, `HRT1`), not after a hyphen or
  slash (`R2-D2`, `PS/2`), not before a time or decimal (`U20:03`, `V1.5`).

The tokenizer (`tokenize()`) turns them into `Word::break_before` /
`Word::code`, `expand_words()` reads the letters, `finish_utterance()` puts
`Phone::break_before` on the word's first phone. The front end still reads the
clause whole, so every context rule sees all the words; the synthesizers then
call `split_at_breaks()` and render the parts one after another
(`FormantSynthesizer::synthesize_clause()`, `ConcatSynthesizer::synthesize_clause()`)
with a silence of **half the user's comma pause** between them, shorter by
the square root of the rate when speech is fast (`minor_break_pause_ms()`,
`MINOR_BREAK_PAUSE_PERCENT` 50: 50 ms at the default 100, 35 ms at 2x, 29 ms
at 3x; the whole comma pause at first and 65 % of it next, both of which the
listener found too long; `Engine::synthesize_clauses()` passes it). A part before a pause is a
continuation with under half of a comma's boundary movement
(`Utterance::minor_break`, `MINOR_BREAK_MOVEMENT` 0.45, last syllable
lengthened by `MINOR_BREAK_STRETCH` 1.15 instead of 1.30), a part after one
does not start again (`Utterance::continued`: no high first accent, no
wh-question peak), the last part keeps the clause's kind. A comma would have
ended the clause ("u dvanaest, trideset u uredu"). Singing presets ignore the
pauses. Both number modes; the digit mode now also sets a number off from the
letters it is glued to (`A17` was "Ajedan sedam", `mp3` "mptri").

Dates fixed in the same round (hr, sr, bs unless noted): the period after a
date's year before a lowercase word is the ordinal's and ends no sentence
(`10.11.2026. godine`, `od 1.12.2026. do 5.12.2026.`, `Rođen je 10.11.1985. u
Zagrebu`; `date_period_end()`); a day and month with leading zeros are a date
(`05.03.`, also Slovenian) and so are `Danas je 07.10.2026.` (nominative) and
`u 08.30` (a time); a day and month written together before *do, i* are a
date, as are two written apart before *do* and another date (`od 1.12. do
5.12.`, `od 1. 12. do 5. 12.`, also Slovenian; `1. 2. i 3. mjesto` stays an
enumeration); a hyphen after an ordinal's period stands for the dash
(`10.-15.11.2026.`, `3.-4. mjesto`, `1991.-2000.`). A date written together
and one written with spaces were already the same words; they are the same
samples. Tests: `src/common/text/numbers/tests.rs`
(`clock_times_and_numeric_dates_have_a_short_pause_between_their_parts`,
`numeric_dates_inside_a_sentence_with_leading_zeros_and_in_ranges`,
`a_cells_address_is_set_off_from_the_cells_contents`),
`src/common/languages/tests.rs` and `languages/sl/tests.rs`
(`a_short_pause_inside_a_clause_splits_it_into_parts`,
`the_letters_of_a_designation_are_letters_whatever_word_they_spell`),
`tests/formant.rs` `clock_times_dates_and_cell_addresses_have_short_pauses`,
`tests/recorded.rs` `recorded_voices_pause_inside_a_time_and_before_a_cells_address`;
details and measurements in `docs/formant.md`, "Short pauses inside
a clause".

### A date in every written form (2026-11-18)

A listener's report: a date must be read as a date however it is written.
`dates::normalize()` (`src/common/text/numbers/dates.rs`) rewrites the other
written forms into the dotted one before the numbers are read, at the top of
`convert_numbers()` and `convert_digits()`, so every form has the words, cases,
pauses, nominative (*Danas je*, *Danes je*) and silent final period of
`18.11.2026.` in all four languages, and is taken before the telephone numbers
and the ranges (`05/03/1985` was a telephone number, `18-11-2026` half a
range): slashes (`18/11/2026`, `18/11/26`), hyphens (`18-11-2026`, U+2011
too), the year first (`2026-11-18`, `2026/11/18`, `2026.11.18`, with the time
of ISO 8601 `2026-11-18T10:30`), the month first where the second number is no
month (`11/18/2026`), a Roman month I-XII in capitals (`18.XI.2026.`,
`18. XI. 2026.`, `18/XI/2026`), and no-break, narrow no-break or thin spaces
after the periods. Not dates: a day and month without a year with slashes or
hyphens (`1/2`, `24/7`, `10-15`), a two-digit year with hyphens (`10-12-14`),
a Roman month without a year (`1. I. Kant`), numbers glued to more groups or
letters, a number after *telefon, tel, mob, faks*. `parse_date()` refuses a
date followed by more dotted groups (`10.10.10.10`). hr, sr, bs: the year
after a month's name in the genitive is the ordinal of *godine*
(`7. listopada 2026.` sedmog listopada dvije tisuće dvadeset šeste, with or
without the period; three copies in `number_at()`); Slovenian
`Danes je 07.10.2026` has the nominative.

**NVDA** was where the report came from: NVDA processes symbols by the table
of the language being spoken before the synthesizer gets the text, and at
the level "none" drops what is not preserved. Its Slovenian table drops a
period between digits (`18.11.2026` arrived as `18 11 2026`; hr, sr, bs, en
preserve it), every table the slashes. An add-on's symbol dictionary cannot
define patterns, so the add-on registers a filter with
`speech.extensions.filter_speechSequence` (NVDA 2024.2+; applied before the
symbol processing). Since 2026-11-23 the **global plugin** registers it
(`globalPlugins/laprdus/__init__.py`, unregistered in `terminate()`; the
synth driver registers nothing), so it works whenever Laprdus speaks: through
its own NVDA driver and through NVDA's SAPI5 driver (`sapi5`, `sapi5_32` in a
64-bit NVDA) with a Laprdus voice, found by "laprdus" in the voice's id or
name (`laprdus_speaker()` in `_speechFilter.py`). A tester's report of
2026-11-23: through SAPI5 dates were read in Croatian and not in Slovenian,
because the filter lived in the synth driver; the installed SAPI5 engine
itself read `18.11.2026` and `18. 11. 2026` to the same samples. For each text
Laprdus speaks, `filter_sequence()` (`_speechFilter.py`, no NVDA imports)

1. writes a date with slashes, hyphens or a Roman month as `18. 11. 2026`
   (`spaced_dates()` in `globalPlugins/laprdus/_dates.py`; the shapes of
   `dates.rs`, **change the two together**), only where NVDA would not speak
   the separator at the user's level (slashes and the periods of a Roman
   month below "some", hyphens below "most"), then
2. writes every period between two digits as a one dot leader (U+2024),
   which no table of NVDA names, wherever NVDA's table for that stretch would
   not keep it at the user's level, at every level (at "some" the Slovenian
   table says *18 pika 11 pika 2026*; the Croatian one keeps the period at
   every level): the probe is
   `characterProcessing.processSpeechSymbols(locale, "0.0", level) != "0.0"`,
   so in practice Slovenian and nothing else. The engine reads a one dot
   leader between two ASCII digits as the period again, first thing in
   `Engine::preprocess()` (`restore_carried_points()` in
   `src/common/text/numbers/mod.rs`; before the dictionaries, so a user entry
   `1.5` matches too), so dates, `ob 8.30`, `3.14`, `132.457` and addresses
   get exactly the text they get in Croatian.

The filter follows `LangChangeCommand`s as `speech.speak()` does (automatic
language and dialect switching): the stretch's locale decides the probe, and
through SAPI5 a stretch in a language other than hr, sr, bs, sl is left alone
when another vendor's voice of that language is installed (SAPI switches to
it on `<lang>`; with a Laprdus token SAPI switches to the Laprdus token of the
language, verified). Applying it twice is applying it once; an error leaves
the sequence as it was. Not covered: NVDA before 2024.2, and SAPI5 hosts other
than NVDA (they do no such symbol processing). The add-on's mandatory
Slovenian symbol dictionary (`locale/sl/symbols-laprdus.dic`) also preserves
`-` and `—` (`most always`, as the hr, sr, bs and en tables do; NVDA's
Slovenian table turned them into spaces below "most": `2-krat`, `10-letnica`,
`2-3 dni`, `1991-2000`); it applies to every synthesizer that speaks
Slovenian in NVDA. Tests: `a_date_is_read_in_every_written_form` (all 15
written forms in all four languages since 2026-11-23, Slovenian had six) and
`a_one_dot_leader_between_digits_is_a_period`
(`src/common/text/numbers/tests.rs`), the date rows of `tests/formant.rs`
and `a_period_nvda_carries_as_a_one_dot_leader_is_a_period`,
`tests/nvda/test_global_plugin.py` (no build needed: registration, the
synthesizers, Slovenian at every level, Croatian, language switching, the
dates, the symbols file), `test_carried_periods_and_the_settings_file` of
`tests/nvda/test_driver.py`; `tests/numbers_against_cpp.rs` and
`tests/frontend_against_cpp.rs` list the intended differences
(`INTENDED`). See "Every written form of a date" in `docs/formant.md`.
