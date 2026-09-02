# Laprdus for Android — What's New

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
