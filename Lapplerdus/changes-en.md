# Laprdus for iPhone, iPad and Mac — What's New

## Version 2.0.0 (build 10) — in development

- Three new voices synthesized entirely by rule, with no recordings: Zvonko (Croatian), Stojan (Serbian) and Mirsad (Bosnian). They stay clear and intelligible at very high speech rates. They are the default voice of each language; a voice you have chosen yourself is never replaced. Josip, Vlado, Detence, Baba and Đedo remain available.
- Bosnian is now a supported language, with Mirsad as its voice.
- Word stress and sentence melody: every word is stressed on the right syllable (a built-in dictionary of several thousand words and names, rules for common endings, and the accent marks you can write in the pronunciation dictionary, such as `telèfon`), and every clause gets a natural contour: a rise at the end of a question, a fall at a full stop, a suspended rise at a comma. English words and names are stressed where English stresses them (*Croàtian*, *Ábleton*), and a word with capitals inside it is read as its parts (*ElevenLabs* as *Eleven Labs*).
- Josip, Vlado, Detence, Baba and Đedo have a new speech engine: the same word stress and melody as the new voices, a natural tempo, audible p, t, k, b, d and g at high rates, and no more clicks and crackles between sounds. Changing the rate no longer changes the pitch, changing the pitch no longer makes the voice rough, and the derived voices sound like a child, a grandmother and a grandfather instead of a sped-up or slowed-down recording.
- Ten singing voices: Zvonko Orguljaš, Klapa Zvonko, Zvonko Pjevač and Zvonko Bećarac (Croatian), Stojan Trubač, Stojan Harmonikaš and Stojan Pevač (Serbian), Mirsad Sevdalija, Mirsad Sazlija and Mirsad Solist (Bosnian). They sing any text to a folk song of their country; the speech rate sets the tempo, the pitch transposes the song and the inflection level sets the vibrato.
- Two new settings for the new voices, shown only when one of them is selected: Inflection level (0% is a monotone, 50% the natural melody, 100% doubles every movement) and Acceleration (a multiplier of the speech rate, 0.5 to 3, so the rate slider reaches a higher or lower top speed). The new voices accept speech rate and pitch from a quarter to four times the normal value.
- The speech rate and pitch set in Laprdus now work together with those of your screen reader or app. They are the voice's normal rate and pitch, and the rate and pitch that VoiceOver or another app asks for speed them up, slow them down, raise or lower them from there: while the screen reader speaks at its normal rate, Laprdus speaks at exactly the rate you set. Before, they were used only with the matching Force switch on, so moving the rate slider changed nothing VoiceOver said. The Force switches now make Laprdus ignore the rate or pitch the app asks for.
- Accent dictionary: a new user dictionary, `accents.json`, corrects the stress of a word for all its forms at once, or of a whole verb. It is stored next to the other user dictionaries and loaded with them (see section 5.11 of the user guide).
- Numbers are read in the language of the voice (Zvonko: *tisuća*, *milijun*; Stojan and Mirsad: *hiljada*, *milion*), and one and two agree with the number word (*dvije tisuće*, *dvadeset jedna tisuća*).
- Fixed: entries added to the user spelling and emoji dictionaries had no effect.
- Fixed: the newline pause setting had no effect and the lines of a post or a list ran together into one sentence. A line break now ends a phrase with that pause, and the next line starts a new sentence (a line beginning with "S ponosom" no longer starts with the letter *es*).
- The accent dictionary is read from the app's dictionary folder, together with the other user dictionaries.
- Fixed: when VoiceOver read an item that contains a single character to be spelled out, such as a button with a badge count, the whole item was spelled letter by letter, and the name and the type of a control ran together into one phrase. Each part is now read as VoiceOver asks: text as text, the character spelled, pauses as pauses.
- VoiceOver's pitch change for capital letters is now heard, and a request that VoiceOver cancels is no longer spoken.

## Version 1.0.0 (build 9) — 28 August 2026

- The app now has a standard tab layout: Main, Settings, Dictionaries, and About, making everything easier to reach.
- The Main tab contains the sample text and the play button.
- Dictionaries got their own tab, with a familiar editing toolbar, swipe actions, and a + button for adding new entries. Adding and editing entries now opens in a standard form with Cancel and Save buttons.
- Removed the button for opening system speech settings.
- Removed the "Force language" setting. On iPhone, iPad and Mac the system always asks for a specific voice, so this setting only overrode your choice; Laprdus now always speaks with the voice the app or VoiceOver asked for.
- Settings are now grouped into Voice, Speech, Application Overrides, Advanced, Reading Pauses and Dictionaries, so a screen reader can jump straight to the group you want.
- VoiceOver navigation in Settings is much shorter: each slider and switch is now a single stop. Speech rate, pitch, volume and the pause sliders announce their name and value once, instead of taking three swipes to reach the control.
- Sliders now move in steps that match what is announced, so the value you hear is exactly the value that gets saved.
- Explanations under switches are now spoken as VoiceOver hints instead of being read before the switch itself.
- When editing a dictionary entry, each field now keeps its name visible above it. Previously the names were only placeholder text, so an entry you were editing showed two unlabeled boxes.
- Dictionary entries now show a chevron on the right, so it is clear that tapping one opens it for editing.
- The whole app was checked at the largest accessibility text sizes and in dark mode. Setting names and their values now stack vertically at large text sizes instead of being squeezed side by side, and switch descriptions no longer get cut off.
- On Mac, settings sliders and dictionary fields now sit next to their names in a single row, the way Mac settings normally look. Previously they were pushed to the right edge, far from the name they belonged to, and text typed into a field was pushed against its right border.
- On Mac in dark mode the sample text box was invisible against the window background. It now has a visible outline.
- The app now works on iPhones and iPads running iOS 16 or newer, and on Macs running macOS 13 Ventura or newer.
- In the system voice list, Laprdus voices are now grouped under "Laprdus" instead of the developer's name.
- The child, grandma and grandpa voices are now called Detence, Baba and Đedo everywhere, matching the names used on the other platforms.
- Pronunciation entries you add or change in the app now take effect in the system voices right away. Previously VoiceOver and Spoken Content kept using the old pronunciation until you switched voices.
- Fixed the speech rate and pitch changing on their own when reading text that itself contains markup, such as a web page's source or a code listing.
- The About screen now warns you if the app and the system voices have stopped sharing settings and dictionaries, instead of letting it pass unnoticed.
- Reworked how synthesized audio is handed to the system, removing a rare source of audio glitches.
