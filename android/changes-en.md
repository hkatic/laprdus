# Laprdus for Android — What's New

## Version 2.0.0 (build 12) — in development

- Three new voices synthesized entirely by rule, with no recordings: Zvonko (Croatian), Stojan (Serbian) and Mirsad (Bosnian). They stay clear and intelligible at very high speech rates. They are the default voice of each language; a voice you have chosen yourself is never replaced. Josip, Vlado, Detence, Baba and Đedo remain available.
- Bosnian is now a supported language, with Mirsad as its voice.
- Word stress and sentence melody: every word is stressed on the right syllable (a built-in dictionary of several thousand words and names, rules for common endings, and the accent marks you can write in the pronunciation dictionary, such as `telèfon`), and every clause gets a natural contour: a rise at the end of a question, a fall at a full stop, a suspended rise at a comma.
- Josip, Vlado, Detence, Baba and Đedo have a new speech engine: the same word stress and melody as the new voices, a natural tempo, audible p, t, k, b, d and g at high rates, and no more clicks and crackles between sounds. Changing the rate no longer changes the pitch, changing the pitch no longer makes the voice rough, and the derived voices sound like a child, a grandmother and a grandfather instead of a sped-up or slowed-down recording.
- Ten singing voices: Zvonko Orguljaš, Klapa Zvonko, Zvonko Pjevač and Zvonko Bećarac (Croatian), Stojan Trubač, Stojan Harmonikaš and Stojan Pevač (Serbian), Mirsad Sevdalija, Mirsad Sazlija and Mirsad Solist (Bosnian). They sing any text to a folk song of their country; the speech rate sets the tempo, the pitch transposes the song and the inflection level sets the vibrato.
- Two new settings for the new voices, shown only when one of them is selected: Inflection level (0% is a monotone, 50% the natural melody, 100% doubles every movement) and Acceleration (a multiplier of the speech rate, 0.5 to 3, so the rate slider reaches a higher or lower top speed). The new voices accept speech rate and pitch from a quarter to four times the normal value.
- Accent dictionary: a new user dictionary, `accents.json`, corrects the stress of a word for all its forms at once, or of a whole verb. It is stored next to the other user dictionaries and loaded with them (see section 5.11 of the user guide).
- Numbers are read in the language of the voice (Zvonko: *tisuća*, *milijun*; Stojan and Mirsad: *hiljada*, *milion*), and one and two agree with the number word (*dvije tisuće*, *dvadeset jedna tisuća*).
- Fixed: entries added to the user spelling and emoji dictionaries had no effect.
- Fixed: the newline pause setting had no effect and the lines of a post or a list ran together into one sentence. A line break now ends a phrase with that pause, and the next line starts a new sentence (a line beginning with "S ponosom" no longer starts with the letter *es*).
- Laprdus now offers Bosnian to the system. When an app asks for a language your chosen voice already speaks, that voice is kept instead of being switched.
- Fixed: the spelling and emoji dictionaries edited in the app were not used by the speech service; all three user dictionaries now take effect as soon as a voice is loaded. The accent dictionary is read from the same folder.

## Version 1.0.0 (build 11) — 2 September 2026

- Laprdus now works on the lock screen right after the phone restarts, before the PIN, pattern or password is entered. TalkBack users can unlock their device with Laprdus speech, using their chosen voice, speed, pitch, pauses, number reading and dictionaries.
- Settings and dictionaries saved by earlier versions are carried over automatically the first time the app or the speech engine starts after the update. No action is needed.
- If the Laprdus voices cannot be loaded at all, the engine now reports the failure to Android instead of staying silent, so the system can try another speech engine when one is available.
- To speak on the lock screen, Laprdus must be set as the preferred text-to-speech engine in the system settings.
- Fixed a problem where a space was silent when reading character by character (for example while moving through text with TalkBack); it is now announced as "razmak".
- Laprdus no longer records the text it speaks in the system log, where it could previously end up in device logs and bug reports — including the characters announced while a PIN is entered on the lock screen.

## Version 1.0.0 (build 10) — 27 August 2026

- Updated the app's internal components to their latest versions for better reliability and compatibility with the newest Android releases.

## Version 1.0.0 (build 9) — 1 August 2026

- Fixed a problem where Laprdus appeared in the list of system text-to-speech engines but never spoke when selected on some devices (reported on Honor phones with MagicOS 10).
- The "Listen to an example" option in Android's text-to-speech settings now works.
- Laprdus no longer stays silent when the device language is not set to Croatian or Serbian — it now always speaks using the selected voice, regardless of the system language.
- Added troubleshooting tips for Honor and Huawei devices to the user guide.

## Version 1.0.0 (build 8) — 10 March 2026

- Initial public release.
