# Laprdus project instructions

Read `CLAUDE.md` for the project architecture, build rules, and testing guidance.

## User's standing testing preference

On this user's Mac, when changes are ready for the user to test, build, install,
and launch the updated native Laprdus app on **both the Mac and the physical
iPhone**. The user explicitly requested this workflow on 8 October 2026 and
asked that it persist across sessions. Do not stop after building the CLI or
an Android APK, and do not ask the user to repeat this deployment request.

- Use `Lapplerdus/Laprdus/Laprdus.xcodeproj`, scheme `Laprdus`, and the existing
  automatic signing configuration. Build the app and its `LaprdusVoices`
  extension from the current working tree.
- Update the existing Mac installation at `/Applications/Laprdus.app`, quit
  the previous instance, and launch the updated copy.
- Discover the currently available physical iPhone with `xcrun devicectl list
  devices`; build for iOS hardware, install in place, and launch
  `com.hrvojekatic.Laprdus` with `--terminate-existing`.
- Preserve settings and dictionaries: do not uninstall the app or clear its
  containers as part of a routine update.
- Verify the builds, installation, and launch. If the iPhone is disconnected
  or locked, finish the Mac deployment and report the specific remaining
  device action. Never report an unverified launch as successful.

This preference applies to Laprdus development and yields to a later explicit
instruction from the user.
