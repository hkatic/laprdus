# Laprdus for iPhone, iPad and Mac — What's New

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
