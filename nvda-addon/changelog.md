# Laprdus for NVDA — What's New

## Version 2.0.0

- Three new voices synthesized entirely by rule, with no recordings: Zvonko (Croatian), Stojan (Serbian) and Mirsad (Bosnian). They stay clear and intelligible at very high speech rates. They are the default voice of each language; a voice you have chosen yourself is never replaced. Josip, Vlado, Detence, Baba and Đedo remain available.
- Bosnian is now a supported language, with Mirsad as its voice.
- Word stress and sentence melody: every word is stressed on the right syllable (a built-in dictionary of several thousand words and names, rules for common endings, and the accent marks you can write in the pronunciation dictionary, such as `telèfon`), and every clause gets a natural contour: a rise at the end of a question, a fall at a full stop, a suspended rise at a comma.
- Josip, Vlado, Detence, Baba and Đedo have a new speech engine: the same word stress and melody as the new voices, a natural tempo, audible p, t, k, b, d and g at high rates, and no more clicks and crackles between sounds. Changing the rate no longer changes the pitch, changing the pitch no longer makes the voice rough, and the derived voices sound like a child, a grandmother and a grandfather instead of a sped-up or slowed-down recording.
- Ten singing voices: Zvonko Orguljaš, Klapa Zvonko, Zvonko Pjevač and Zvonko Bećarac (Croatian), Stojan Trubač, Stojan Harmonikaš and Stojan Pevač (Serbian), Mirsad Sevdalija, Mirsad Sazlija and Mirsad Solist (Bosnian). They sing any text to a folk song of their country; the speech rate sets the tempo, the pitch transposes the song and the inflection level sets the vibrato.
- Two new settings for every voice in the Laprdus Configurator: Inflection level (0% is a monotone, 50% the natural melody, 100% doubles every movement) and Acceleration (a multiplier of the speech rate, 0.5 to 3, so the rate slider reaches a higher or lower top speed; SAPI5 only, NVDA uses its rate boost instead). The new voices accept speech rate and pitch from a quarter to four times the normal value.
- Accent dictionary: a new user dictionary, `accents.json`, corrects the stress of a word for all its forms at once, or of a whole verb. It is stored next to the other user dictionaries and loaded with them (see section 5.11 of the user guide).
- Numbers are read in the language of the voice (Zvonko: *tisuća*, *milijun*; Stojan and Mirsad: *hiljada*, *milion*), and one and two agree with the number word (*dvije tisuće*, *dvadeset jedna tisuća*).
- Fixed: entries added to the user spelling and emoji dictionaries had no effect.
- The add-on picks the new voice of NVDA's language by default (Zvonko for Croatian, Stojan for Serbian, Mirsad for Bosnian) and keeps a voice you have chosen yourself.
- NVDA's Inflection slider sets the inflection level of every voice: 50 is the natural melody, 0 a monotone, 100 twice every movement. The inflection settings of the Laprdus Configurator apply to SAPI5 only. The Acceleration setting is not used under NVDA; NVDA's Rate boost makes every position of the rate slider three times faster, as with eSpeak and OneCore (the recorded voices stop at four times the normal rate).
- Settings saved by version 1.0 keep their meaning: an unforced Laprdus rate or pitch, which version 1.0 ignored, stays normal instead of changing the rate NVDA asks for, and intonation switched off in version 1.0 becomes an inflection level of 0% in the Laprdus Configurator (for SAPI5).
- The accent dictionary is loaded from the Laprdus folder with the other user dictionaries and reloaded when the file changes.

## Version 1.0.0

- Initial release of Laprdus for NVDA: Croatian and Serbian speech with the voices Josip and Vlado and the presets Detence, Baba and Đedo; adjustable rate, pitch and volume.
