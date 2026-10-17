# Laprdus for Android — What's New

## Version 2.0.0 (build 12) — in development

- Three new voices synthesized entirely by rule, with no recordings: Zvonko, Stojan and Mirsad. They stay clear and intelligible at very high speech rates. Zvonko is the default voice, for every language. Josip, Vlado, Detence, Baba and Đedo remain available.
- Every voice speaks every language. The language (Croatian, Serbian or Bosnian) is chosen separately from the voice, as in eSpeak or Eloquence: a new Language setting under the voice chooses the language of the Speak button, and TalkBack and other apps choose the language of their own speech while the voice stays the one you picked. The language sets the pronunciation rules, the word stress, the names of the letters when spelling and the number words (*tisuća* and *milijun* in Croatian, *hiljada* and *milion* in Serbian and Bosnian, *dvjesto* / *dvesta*). The system now lists each voice in all three languages, 54 voices in all, named like `zvonko-hr`, `zvonko-sr` or `josip-bs`.
- Bosnian is now a supported language, spoken by every voice; Mirsad is its native voice.
- Force language now means: always speak the language chosen in Laprdus's settings, whatever language an app asks for. Before, it reloaded the saved voice. The voice an app speaks with is always the one chosen in Laprdus's settings.
- Renamed voices. Their names now come from the engine, without a "Laprdus" prefix or a language in brackets, and the singing voices are named after the player and the song or after the region of their song: Zvonko Orguljaš is now *Orguljaš - Vila Velebita*, Klapa Zvonko *Dalmacija Klapa*, Zvonko Pjevač *Lika Velebit*, Zvonko Bećarac *Bećarac*, Stojan Trubač *Trubač - Kreće se lađa francuska*, Stojan Harmonikaš *Harmonikaš - Kreće se lađa francuska*, Stojan Pevač *Solunac*, Mirsad Sevdalija *Sevdalija - Kad ja pođoh na Bembašu*, Mirsad Sazlija *Sazlija - Kad ja pođoh na Bembašu* and Mirsad Solist *Sarajevo Bembaša*; the grandfather's name is spelled *Đedo*. A voice saved under its old name (`djed`, `klapa`, `pjevac`, `pevac`, `solist`) is still loaded.
- Word stress and sentence melody: every word is stressed on the right syllable (a built-in dictionary of several thousand words and names, rules for common endings, and the accent marks you can write in the pronunciation dictionary, such as `telèfon`), and every clause gets a natural contour: a rise at the end of a question, a fall at a full stop, a suspended rise at a comma. English words and names are stressed where English stresses them (*Croàtian*, *Ábleton*), and a word with capitals inside it is read as its parts (*ElevenLabs* as *Eleven Labs*).
- Josip, Vlado, Detence, Baba and Đedo have a new speech engine: the same word stress and melody as the new voices, a natural tempo, audible p, t, k, b, d and g at high rates, and no more clicks and crackles between sounds. Changing the rate no longer changes the pitch, changing the pitch no longer makes the voice rough, and the derived voices sound like a child, a grandmother and a grandfather instead of a sped-up or slowed-down recording.
- Ten singing voices: Orguljaš - Vila Velebita, Dalmacija Klapa, Lika Velebit and Bećarac (Zvonko's voice), Trubač - Kreće se lađa francuska, Harmonikaš - Kreće se lađa francuska and Solunac (Stojan's), Sevdalija - Kad ja pođoh na Bembašu, Sazlija - Kad ja pođoh na Bembašu and Sarajevo Bembaša (Mirsad's). They sing any text, in any language, to a folk song; the speech rate sets the tempo, the pitch transposes the song and the inflection level sets the vibrato.
- Two new settings for the new voices, shown only when one of them is selected: Inflection level (0% is a monotone, 50% the natural melody, 100% doubles every movement) and Acceleration (a multiplier of the speech rate, 0.5 to 3, so the rate slider reaches a higher or lower top speed). The new voices accept speech rate and pitch from a quarter to four times the normal value.
- The speech rate and pitch set in Laprdus now work together with those of your screen reader or app. They are the voice's normal rate and pitch, and the rate and pitch that TalkBack or another app asks for speed them up, slow them down, raise or lower them from there: while the screen reader speaks at its normal rate, Laprdus speaks at exactly the rate you set. Before, they were used only with the matching Force switch on, so moving the rate slider changed nothing TalkBack said. The Force switches now make Laprdus ignore the rate or pitch the app asks for.
- Accent dictionary: a new user dictionary, `accents.json`, corrects the stress of a word for all its forms at once, or of a whole verb. It is edited in the Dictionaries screen like the other dictionaries (dictionary type Accent Dictionary), with fields for the entry, its kind (word or verb), its language and a comment; the app checks every entry with the same rules Laprdus reads it by and refuses a malformed one with the reason. It is stored next to the other user dictionaries and loaded with them (see section 5.11 of the user guide).
- Numbers are read in the chosen language (Croatian: *tisuća*, *milijun*; Serbian and Bosnian: *hiljada*, *milion*), by every voice, and one and two agree with the number word (*dvije tisuće*, *dvadeset jedna tisuća*).
- Fixed: entries added to the user spelling and emoji dictionaries had no effect.
- Fixed: the newline pause setting had no effect and the lines of a post or a list ran together into one sentence. A line break now ends a phrase with that pause, and the next line starts a new sentence (a line beginning with "S ponosom" no longer starts with the letter *es*).
- Laprdus now offers Bosnian to the system, and no app can switch your voice any more: an app's language request changes only the language.
- Fixed: the spelling and emoji dictionaries edited in the app were not used by the speech service; all three user dictionaries now take effect as soon as a voice is loaded. The accent dictionary is read from the same folder.
- Fixed: the app and the speech service shared one engine, so the Speak button spoke at the rate, pitch and volume of TalkBack's last request, and choosing a voice in the app left TalkBack without your dictionary entries until a voice was loaded again. Each now has an engine of its own: the Speak button speaks with your settings and your dictionaries, and TalkBack switches to the voice you choose with your dictionaries in place.
- A dictionary entry you add, change or delete in the app, and switching the user dictionaries on or off, now take effect in TalkBack at once. Before, they waited until a voice was loaded again.

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
