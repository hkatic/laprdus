# Laprdus - User Guide

Version 2.0

---

## Contents

1. [Introduction](#1-introduction)
    - [1.1 What is Laprdus?](#11-what-is-laprdus)
    - [1.2 Who is Laprdus for?](#12-who-is-laprdus-for)
    - [1.3 Supported platforms](#13-supported-platforms)
    - [1.4 Features](#14-features)
2. [Installation](#2-installation)
    - [2.1 Windows SAPI5](#21-windows-sapi5)
    - [2.2 NVDA add-on](#22-nvda-add-on)
    - [2.3 Linux](#23-linux)
    - [2.4 Android](#24-android)
3. [Usage](#3-usage)
    - [3.1 Basic usage](#31-basic-usage)
    - [3.2 Command line (Linux and Windows)](#32-command-line-linux-and-windows)
    - [3.3 NVDA settings](#33-nvda-settings)
    - [3.4 Android app](#34-android-app)
4. [Settings](#4-settings)
    - [4.1 Speech rate](#41-speech-rate)
    - [4.2 Pitch](#42-pitch)
    - [4.3 Volume](#43-volume)
    - [4.4 Pauses](#44-pauses)
    - [4.5 How numbers are read](#45-how-numbers-are-read)
    - [4.6 Inflection](#46-inflection)
    - [4.7 Spelling](#47-spelling)
    - [4.8 Laprdus Configurator (Windows)](#48-laprdus-configurator-windows)
    - [4.9 Voice quality](#49-voice-quality)
5. [Dictionaries](#5-dictionaries)
    - [5.1 Types of dictionaries](#51-types-of-dictionaries)
    - [5.2 Dictionary file locations](#52-dictionary-file-locations)
    - [5.3 Main dictionary (user.json)](#53-main-dictionary-userjson)
    - [5.4 Spelling dictionary (spelling.json)](#54-spelling-dictionary-spellingjson)
    - [5.5 Emoji dictionary (emoji.json)](#55-emoji-dictionary-emojijson)
    - [5.6 Editing dictionaries on Windows](#56-editing-dictionaries-on-windows-laprdus-configurator)
    - [5.7 Editing dictionaries on Android](#57-editing-dictionaries-on-android)
    - [5.8 Editing JSON files by hand](#58-editing-json-files-by-hand-advanced)
    - [5.9 Practical examples](#59-practical-examples)
    - [5.10 Troubleshooting dictionaries](#510-troubleshooting-dictionaries)
    - [5.11 Accent dictionary (accents.json)](#511-accent-dictionary-accentsjson)
6. [Voices](#6-voices)
    - [6.1 Available voices](#61-available-voices)
    - [6.2 Base and derived voices](#62-base-and-derived-voices)
    - [6.3 Choosing a voice](#63-choosing-a-voice)
7. [Troubleshooting](#7-troubleshooting)
    - [7.1 Common problems](#71-common-problems)
    - [7.2 Diagnostic logs](#72-diagnostic-logs)
    - [7.3 Contact and support](#73-contact-and-support)

---

## 1. Introduction

### 1.1 What is Laprdus?

Laprdus is a text-to-speech (TTS) synthesizer for Croatian, Serbian, Bosnian and Slovenian. Its original voices use concatenative synthesis, joining prerecorded phoneme units to produce speech, while its formant voices compute speech entirely by rule, with no recordings. Every voice speaks all four languages. Although it doesn't reach the quality of modern neural TTS systems, Laprdus offers high performance and minimal memory use.

Laprdus was developed to give screen reader users simple, fast access to computers and mobile devices in their native language, free of charge.

Laprdus started as my personal hobby project and is also meant to be experimental, which lets me see how far I can take it.

### 1.2 Who is Laprdus for?

Laprdus isn't for everyone. It's a good choice if you enjoy the nostalgic sound of the retro speech synthesizers of the 1980s and 1990s.

It may not suit you if you're used to the high voice quality of modern synthesizers that use artificial intelligence and neural networks. However, Laprdus could be ideal if performance and minimal resource use matter more to you than voice quality.

In short: modern synthesizers offer quality at the cost of resources; Laprdus offers performance at the cost of quality.

### 1.3 Supported platforms

Laprdus is available on the following platforms:

- **Windows 7 to Windows 11** - through the Microsoft SAPI5 standard, so you can use Laprdus in any program that supports Windows speech, including screen readers such as Narrator, NVDA and JAWS
- **Direct integration with the NVDA screen reader** - also available as a separate add-on for NVDA, the free Windows screen reader
- **Linux** - through Speech Dispatcher for the Orca screen reader, and as a command-line program
- **Android** - as a built-in speech synthesizer for Android devices

### 1.4 Features

Laprdus offers the following features:

- Speech in Croatian, Serbian, Bosnian and Slovenian (Latin and Cyrillic scripts are supported)
- Eighteen voices (two base, three derived, three formant and ten singing), each of which speaks all four languages (Croatian, Serbian, Bosnian and Slovenian); you choose the language separately from the voice
- Adjustable speech rate
- Adjustable pitch
- Adjustable volume
- Natural intonation that follows punctuation
- Numbers read in full or digit by digit
- Adjustable pause length for different punctuation marks
- User dictionaries for customizing the pronunciation of words, phrases, symbols and emoji

---

## 2. Installation

### 2.1 Windows SAPI5

To install Laprdus on Windows, follow these steps:

#### Step 1: Download

1. Go to the official Laprdus website: https://hrvojekatic.com/laprdus
2. Find the download section and click the link for Windows SAPI5
3. Download the installer (the file named `Laprdus_SAPI5_Setup.exe`)
4. The file is saved in your Downloads folder

#### Step 2: Run the installer

1. Open File Explorer (Windows+E)
2. Use the Down Arrow key to find the Downloads folder and press Enter to open it
3. Find the Laprdus installer you downloaded (the file named `Laprdus_SAPI5_Setup.exe`)
4. **Recommended:** Press the Applications key or Shift+F10 on the file, then select "Run as administrator"
    - Alternatively: Press Enter to run it normally
5. If a User Account Control (UAC) window appears, select "Yes"

#### Step 3: Installation process

1. Use the Down Arrow key to select the setup language and press Enter
2. Choose the installation location (default: `C:\Program Files\Laprdus`), then press Tab to select "Next" and press Enter
3. If you want a desktop icon for the Laprdus Configurator, press Spacebar to select "Create desktop shortcut". Then press Tab to find "Next" and press Enter.
4. Press Tab to find "Install", press Enter and wait for the installation to finish
5. When the installation is complete, press Enter on "Finish" to close the installer

#### Step 4: Verify the installation

1. Open Control Panel:
    - Press the Windows key
    - Type "Control Panel" and press Enter
2. Select "Ease of Access" > "Speech Recognition"
3. Press Tab to find "Text to Speech" and press Enter
4. In the "Voice selection" drop-down list, you should see four Laprdus voices:
    - Laprdus Croatian (Croatian)
    - Laprdus Serbian (Serbian)
    - Laprdus Bosnian (Bosnian)
    - Laprdus Slovenian (Slovenian)

    The SAPI5 voice chooses only the language. All four speak with the voice you choose in the Laprdus Configurator (Zvonko until you change it): Josip, Vlado, Detence, Baba, Đedo, Zvonko, Stojan, Mirsad or one of the ten singing voices (see [6.1](#61-available-voices)). During installation, upgrade and repair, the installer removes the voices registered by earlier versions (one for each Laprdus voice).

### 2.2 NVDA add-on

To use Laprdus with the NVDA screen reader:

#### Step 1: Download

1. Go to the official Laprdus website: https://hrvojekatic.com/laprdus
2. Find the download section and click the link for the NVDA add-on
3. Download the NVDA add-on (the file with the `.nvda-addon` extension)
4. The file is saved in your Downloads folder

#### Step 2: Install the add-on

1. Open File Explorer (Windows+E)
2. Find the Downloads folder
3. Find the Laprdus NVDA add-on file you downloaded (the file with the `.nvda-addon` extension)
4. Press Enter on the file
    - **Alternatively:** Press the Applications key (or Shift+F10) to open the context menu, then select "Open"
5. NVDA shows a dialog that asks "Are you sure you want to install this add-on?"
6. Press Tab to find the "Yes" button and press Enter, or just press Y for Yes
7. Wait for the installation to finish

#### Step 3: Restart NVDA

1. NVDA asks you to restart it
2. Select "Yes" to restart right away
3. Wait for NVDA to restart

#### Step 4: Select Laprdus as the synthesizer

**Method 1: From the NVDA menu**

1. Open the NVDA menu: press NVDA+N (Note: The NVDA key on your system can be Insert or Caps Lock, depending on how you configured it. If the NVDA key is set to Insert, you can press Insert+N.)
2. Use the Down Arrow key to find "Preferences" and press Enter
3. Use the Down Arrow key to find "Settings..." and press Enter
4. In the NVDA Settings dialog, use the Down Arrow key to find the "Speech" category
5. Press Tab to find the "Change..." button, right next to the "Synthesizer" option
6. Press Enter on the "Change..." button
7. In the list of synthesizers, use the Down Arrow key to find "Laprdus" and press Enter
8. Press Escape to close the settings

**Method 2: Quick shortcut (recommended)**

1. Press Ctrl+NVDA+S (Note: The NVDA key on your system can be Insert or Caps Lock, depending on how you configured it. If the NVDA key is set to Insert, you can press Ctrl+Insert+S.)
2. The Select Synthesizer dialog opens
3. Use the Down Arrow key to find "Laprdus" and press Enter

### 2.3 Linux

#### Debian and Ubuntu

Open a terminal and run the following command (replace the file name with the name of the .deb file you downloaded):

```bash
sudo dpkg -i laprdus_amd64.deb
```

#### Fedora

Run the following command (replace the file name with the name of the .rpm file you downloaded):

```bash
sudo rpm -i laprdus.x86_64.rpm
```

#### Arch Linux

1. Download the PKGBUILD file from the website
2. In a terminal, go to the folder that contains the PKGBUILD file
3. Run:

```bash
makepkg -si
```

#### Manual installation (from a tarball)

1. Extract the archive (replace the file name with the name of the archive you downloaded):

        tar xf laprdus-linux-x86_64.tar.xz
        cd laprdus-linux-x86_64

2. Run the installation script:

        sudo ./install.sh

#### Configuring the Orca screen reader

After installation, Laprdus is configured for Speech Dispatcher automatically. To use it with the Orca screen reader:

1. To be safe, we recommend that you restart Speech Dispatcher:

        systemctl --user restart speech-dispatcher

2. Open the Orca settings:
    - Press Insert+Spacebar to open the settings (or Caps Lock+Spacebar if the Orca modifier is set to Caps Lock or Laptop layout)
    - Or, on the Gnome or Mate desktop: press Alt+F2, type orca -s and press Enter

3. Use Tab or Shift+Tab to move to the settings tabs, use the Right Arrow key to find the "Voice" tab, then press Tab repeatedly to select:
    - Speech system: Press Spacebar to expand the drop-down list, use the Down Arrow key to select Speech Dispatcher if it isn't already set, and press Enter
    - Speech synthesizer: Press Spacebar to expand the drop-down list, use the Down Arrow key to select Laprdus, and press Enter
    - Voice: Press Spacebar to expand the drop-down list, use the Down Arrow key to select the voice you want (Zvonko, Josip, Vlado and so on; each voice is listed for Croatian, Serbian, Bosnian and Slovenian, and the language Orca requests determines which language it speaks), and press Enter

4. Press Tab to find "Apply" and close the settings

### 2.4 Android

#### Step 1: Download the APK file

1. On your Android device, open a web browser
2. Go to: https://hrvojekatic.com/laprdus
3. Download the APK file (the file with the `.apk` extension)

#### Step 2: Allow installation from unknown sources

You need this only if you install the app from outside the Google Play Store.

**For Android 8.0 and later:**

1. Open Settings on your device
2. Go to: Apps > Special app access > Install unknown apps
    - Or: Settings > Security > Install unknown apps
3. Find the web browser you use (for example, Chrome)
4. Turn on "Allow from this source"

**For older versions of Android:**

1. Open Settings
2. Go to: Security
3. Turn on "Unknown sources"

#### Step 3: Install the app

1. Open a file manager on your device
2. Find the APK file you downloaded (usually in the "Download" or "Downloads" folder)
3. Tap the APK file
4. When prompted, tap "Install"
5. Wait for the installation to finish
6. Tap "Open" to start the app or "Done" to close the installer

#### Step 4: Set Laprdus as the default TTS engine

1. Open Settings on your device
2. Go to: Accessibility > Text-to-speech output
    - On some devices: Settings > System > Languages & input > Text-to-speech output
    - On Samsung devices: Settings > General management > Language and input > Text-to-speech
3. Tap "Preferred engine" or the gear icon
4. Select "Laprdus TTS" from the list
5. Go back to the previous screen and tap "Listen to an example" to test it

#### Note for TalkBack users

If you use TalkBack, all the screens of the Laprdus app are fully accessible:

- Use the standard TalkBack gestures to navigate
- Double-tap to activate buttons and options
- Swipe with two fingers to scroll through lists

---

## 3. Usage

### 3.1 Basic usage

After installation, Laprdus is automatically available to every program on your system that uses speech synthesis. This includes:

- Screen readers (NVDA, Orca, Narrator)
- Web browsers that can read pages aloud
- Programs that read documents
- Any program that uses Windows SAPI5 or Android TTS

### 3.2 Command line (Linux and Windows)

Laprdus includes a command-line program you can use to convert text to speech directly from a terminal.

#### Basic examples

```bash
# Speak text
laprdus "Dobar dan!"

# Use another voice (without -L a voice speaks its native language, Vlado speaks Serbian)
laprdus -v vlado "Zdravo svete!"

# Any voice in any language: Zvonko speaks Serbian, Vlado speaks Croatian
laprdus -L sr "Zdravo svete!"
laprdus -v vlado -L hr "Dobar dan svima!"

# Adjust the speech rate
laprdus -r 1.5 "Brži govor"

# Save to a WAV file
laprdus -o govor.wav "Tekst za snimanje"

# Read from a file
laprdus -i dokument.txt

# Read from standard input
echo "Tekst" | laprdus
```

#### Command-line options

| Option | Description |
|--------|------|
| `-v, --voice` | Select a voice (zvonko (default), stojan, mirsad, josip, vlado, detence, baba, djedo or a singing voice: orguljas, dalmacija_klapa, trubac, harmonikas, sevdalija, sazlija, lika_velebit, solunac, sarajevo_bembasa, becarac) |
| `-L, --language` | Language: `hr`, `sr`, `bs` or `sl`. Every voice speaks every language; without this option, a voice speaks its native language |
| `-r, --speech-rate` | Speech rate (0.5-2.0, default: 1.0; formant voices 0.25-4.0) |
| `-p, --speech-pitch` | Pitch (0.5-2.0, default: 1.0; formant voices 0.25-4.0) |
| `-I, --inflection` | Inflection level, 0-100 (default: 50; 0 is a monotone) |
| `-a, --acceleration` | Acceleration, a speech rate multiplier 0.5-3.0 (default: 1.0) |
| `--head-size PERCENT` | Voice quality: vocal tract length, 0-100 (default: 50 = the voice as it is) |
| `--breathiness PERCENT` | Breath noise, 0-100 (default: 50; formant voices) |
| `--roughness PERCENT` | Jitter and shimmer, 0-100 (default: 50; formant voices) |
| `--brightness PERCENT` | Tone, 0-100 (default: 50; 0 dull, 100 sharp) |
| `-V, --speech-volume` | Volume (0.0-1.0, default: 1.0) |
| `-B, --volume-boost` | Volume boost: louder speech through a peak limiter, for noisy places |
| `-d, --numbers-digits` | Read numbers digit by digit |
| `-s, --spell` | Spell the text character by character |
| `-S, --spelling-speed` | Spelling speed, 0-100; lower values make the pauses longer (default: 50) |
| `-m, --spelling-mode` | Spelling: `sounds` (letter sounds: b, c, d; default) or `names` (letter names: be, ce, de) |
| `-c, --comma-pauses` | Comma pause length in ms (default: 100) |
| `-e, --period-pauses` | Period pause length in ms (default: 80) |
| `-x, --exclamationmark-pauses` | Exclamation mark pause length in ms (default: 70) |
| `-q, --questionmark-pauses` | Question mark pause length in ms (default: 60) |
| `-n, --newline-pauses` | Newline pause length in ms (default: 100) |
| `-D, --data-dir` | Voice data folder |
| `-o, --output-file` | Save the speech to a WAV file |
| `-i, --input-file` | Load text from a file |
| `-l, --list-voices` | List the available voices with their native language |
| `-w, --verbose` | Verbose output (for diagnostics) |
| `-h, --help` | Show help |

### 3.3 NVDA settings

When you use Laprdus with the NVDA screen reader, you can change the voice settings as follows:

1. Open the NVDA menu (NVDA+N)
2. Select the Preferences submenu, then Settings > the Speech category
3. In the dialog, you can adjust:
    - Language (Croatian, Serbian, Bosnian or Slovenian)
    - Voice (the list contains all 18 Laprdus voices, under the names in [6.1](#61-available-voices); the default is Zvonko)
    - Rate
    - Pitch
    - Inflection (the level of the sentence melody, see [4.6](#46-inflection))
    - Volume
    - Rate boost (makes every position of the rate slider three times faster)

**The language is separate from the voice.** Every Laprdus voice speaks all four languages, so the list of voices is the same for every language, and you choose the language in a separate "Language" drop-down list, as in synthesizer B. The language determines the pronunciation rules, the accent dictionary, the letter names when spelling and the words for numbers (*tisuća*, *milijun* in Croatian; *hiljada*, *milion* in Serbian and Bosnian; *tisoč*, *milijon* in Slovenian): this way, Josip can speak Serbian, and Stojan Croatian or Slovenian. A new installation starts in NVDA's language if that's Croatian, Serbian, Bosnian or Slovenian, and otherwise in Croatian; NVDA remembers the language you choose, like its other settings. Language is also in the synth settings ring (Ctrl+NVDA+arrow keys).

**Automatic language switching.** When "Automatic language switching (when supported)" is turned on in NVDA's speech settings, Laprdus speaks text marked as Croatian, Serbian, Bosnian or Slovenian in that language, with the same voice; it speaks text in any other language in the language you chose. The voice never changes.

### 3.4 Android app

The Laprdus Android app includes a settings screen where you can:

1. Choose a voice
2. Choose the language (Croatian, Serbian, Bosnian or Slovenian) for speech in the app itself; speech requested by other apps and TalkBack uses the language they request, unless "Force language" is on
3. Adjust the speech rate
4. Adjust the pitch
5. Adjust the volume
6. Turn emoji reading on or off
7. Set how numbers are read
8. Adjust the pause lengths and the spelling speed
9. Choose whether letters are spelled by their names or by their sounds

To open the settings:

1. Open the Laprdus app from the app drawer
2. Tap the "Laprdus Settings" button
3. Change the settings as you like (changes are saved automatically)

---

## 4. Settings

### 4.1 Speech rate

Speech rate determines how fast Laprdus speaks text. The range is 0.5 (half speed) to 2.0 (twice as fast), and 1.0 is the normal rate.

- **0.5** - Very slow, useful for beginners or when you need to listen carefully to every word
- **1.0** - Normal rate, suitable for most situations
- **1.5** - Moderately fast, for experienced users
- **2.0** - Fast, for advanced users

Note: In the NVDA add-on, the "Rate boost" option makes every position of the rate slider three times faster, as in other synthesizers; Josip, Vlado and the voices derived from them stop at four times the normal rate.

**The Laprdus rate and the screen reader's rate.** The rate in the Laprdus settings is the voice's normal rate, and the screen reader or app (TalkBack, VoiceOver, NVDA, programs that use SAPI5, Speech Dispatcher) sets its own rate relative to it. While the screen reader speaks at its normal rate, Laprdus speaks at exactly the rate in its settings; a screen reader running twice as fast gives twice that rate. That's why both the rate slider in Laprdus and the slider in the screen reader speed up and slow down the speech. The "Force Laprdus speech rate" switch ("Force Laprdus speech speed for all applications" in the Windows Configurator) ignores the rate the screen reader or app asks for, so Laprdus speaks at the rate in its settings alone. [Acceleration](#46-inflection) then multiplies that rate.

### 4.2 Pitch

Pitch lets you adjust the base frequency of the voice. The range is 0.5 (lower voice) to 2.0 (higher voice), and 1.0 is the natural pitch of the voice. Like the rate (see [4.1](#41-speech-rate)), the pitch in the Laprdus settings is the voice's normal pitch, which the screen reader raises or lowers, unless the "Force Laprdus speech pitch" switch is on.

- **0.5** - Lower voice
- **1.0** - Natural pitch
- **2.0** - Higher voice

Note: Adjusting the pitch doesn't affect the character of the voice (there's no "cartoon effect").

### 4.3 Volume

Volume determines how loud the speech is. The range is 0.0 (silent) to 1.0 (maximum volume).

- **0.0** - Silent (no sound)
- **0.5** - Moderate volume
- **1.0** - Maximum volume

#### Volume boost

The **Boost speech volume (for noisy places)** switch in the Laprdus Configurator makes speech louder than the volume slider allows at 100%. It's meant for noisy places (traffic, the street) and for the phone's earpiece when the speaker is off: for example, to hear who you're calling while the call connects, or to find the button that turns on the speaker during a call. Speech goes through strong amplification and a peak limiter that changes its gain only between syllables, as in the speech amplification setting of phone systems: quiet parts get about 8 dB louder and loud parts about 3 dB louder, and nothing inside a syllable is clipped or compressed, so the sound stays clean. The consonants keep their distance from the vowels, and the treble above 4 kHz is turned down by 3 dB, since a phone's speaker at full volume has a peak of its own there. NVDA's volume slider still changes the boosted speech. The switch is off by default and applies to all voices.

### 4.4 Pauses

Laprdus automatically inserts pauses after punctuation marks to make speech sound more natural. You can adjust the length of the pauses for:

- **Period** - Pause after a period (default: 100 ms in the user settings, 80 ms in the command-line program)
- **Exclamation mark** - Pause after an exclamation mark (default: 100 ms in the user settings, 70 ms in the command-line program)
- **Question mark** - Pause after a question mark (default: 100 ms in the user settings, 60 ms in the command-line program)
- **Comma** - Short pause within a sentence (default: 100 ms)
- **Newline** - Pause at a line break (default: 100 ms)

Values are in milliseconds (ms). The range is 0 to 2000 ms.

**Note:** The user settings (Windows/Android) use a single value of 100 ms for all sentence-ending characters (period, exclamation mark, question mark), while the command-line program lets you set each character separately.

The same section of the settings also has the **spelling speed** (0–100%, default 50%): a lower value makes the pause after each spelled or typed character longer, and the character itself is always spoken at the normal speech rate. At 100% there's no extra pause; at 50% about 41% of the character's duration is added, at 20% about 74%, and at 0% its whole duration. This silence is added to the base spelling pause (default 200 ms, `pauses.spelling` in `settings.json`). The pause also follows the last character, so the spacing is the same whether the screen reader sends a whole word or one character at a time.

In NVDA, NVDA's "Delayed descriptions for characters on cursor movement" option (speech settings) takes precedence: when it's on, NVDA waits one second after the letter you moved to and then speaks its description (for example, "Alan" for A). Laprdus then doesn't add the spelling speed pause after that letter, so the description comes right after NVDA's one second, whatever the spelling speed. Spelling words and typing keep the rhythm of the spelling speed.

### 4.5 How numbers are read

Laprdus can read numbers in two ways:

1. **As words** (default) - Numbers are read as full words
    - Example: "123" is read as "sto dvadeset tri"

2. **As digits** - Numbers are read digit by digit
    - Example: "123" is read as "jedan dva tri"

Reading as words supports numbers up to a centillion (10^303) and uses the correct grammatical forms of the selected language.

### 4.6 Inflection

Inflection is the natural change in pitch that follows punctuation:

- A period at the end of a sentence makes the pitch fall
- A question mark makes the pitch rise at the end
- An exclamation mark adds emphasis
- A comma makes the pitch rise slightly

A slider sets the amount of inflection, and the on/off switch from version 1.0 is gone: for monotone speech, set the slider to 0%. Two settings apply to all voices:

- **Inflection level** (0-100%) sets the size of all pitch movements: 0% is monotone speech, 50% the natural melody, and 100% doubles every movement.
- **Acceleration** (0.5-3) multiplies the speech rate, so the rate slider in the screen reader or system reaches a higher (or lower) top rate. Josip, Vlado and the voices derived from them stop at four times the normal rate; Zvonko, Stojan and Mirsad go up to eight times.

In NVDA, NVDA's Inflection slider in the voice settings sets the inflection level: 50 is the natural melody, 0 is monotone speech, and at 100 every movement is doubled. The inflection from the Configurator isn't used in that case. Acceleration isn't used in NVDA; you reach higher rates with NVDA's Rate boost.

### 4.7 Spelling

When the screen reader spells text or speaks characters as you type, Laprdus reads each letter in one of two ways:

- **By letter name**: *a, be, ce, če, će, de, dže, đe, e, ef, ge, ha, i, je, ka, el, elj, em, en, enj, o, pe, er, es, eš, te, u, ve, ze, že*; foreign letters *ku, duplo ve, iks, ipsilon*. These are the names from the Croatian orthography, and they also apply in Bosnian. In Serbian, every voice follows the decision of the Committee for the Standardization of the Serbian Language: *lje, nje* and *ša* instead of *elj, enj* and *eš*, with the other letters the same, Cyrillic as well as Latin (*Љ* is *lje*, *Ш* is *ša*). In Slovenian, the names follow Slovenski pravopis: the same names for its 25 letters, *dvojni ve* for W, and *mehki če* and *mehki dže* for Ć and Đ. The selected language determines the names, not the voice: Josip says *lje* in Serbian, Stojan says *elj* in Croatian.
- **By letter sound** (default): instead of the name, Laprdus speaks the sound the letter stands for, as when a child learns to read and as synthesizer B's Croatian voice spells letters: the consonant followed by a short neutral vowel (*bə, cə, čə, fə, sə, mə*), and vowels as vowels. The foreign letters *q, w, x* and *y* aren't part of the native alphabet, so they're read by name here too (*ku, duplo ve, iks, ipsilon*). English names and English pronunciation aren't used.

The spelling mode is the "Spell letters by their sounds" switch, on in a new installation, in the advanced settings (Windows: the Options group in the Configurator; Android and Apple: the Advanced section; NVDA: the Configurator; command line: `-m`). It applies to spelling, typing and a letter standing alone in a message; words are still spoken normally. In both modes, numbers, punctuation and symbols are read by their names from the spelling dictionary. Spelling speed is described in [4.4](#44-pauses).

**Capital letters.** If the screen reader is set to announce capitals with a word ("veliko N" in NVDA, "veliko slovo N" in TalkBack, "veliko početno slovo N" in VoiceOver, "cap N" in English), Laprdus speaks the word normally and spells the letter like any other: by name or by sound, according to the selected mode, and at the spelling speed. NVDA most often marks capitals with a higher pitch: the "Capital pitch change percentage" setting in the voice settings (default 30) raises the pitch of a spelled capital, and the next character or word returns to the pitch of the slider. This also applies when forcing the Laprdus pitch is on: the change is then calculated from the middle of NVDA's pitch slider.

**A letter alone in a message.** Screen readers often report a character inside a message: "Obrisano B", "B odabrano", "veliko slovo B" when they send the parts of an announcement separately. A consonant that stands alone like this, as a word of its own, is read in the selected mode, like a spelled letter: by sound (*bə*) when letter sounds are on, by name (*be*) when they're off. The same goes for such a letter in ordinary text (*Plan B*, *tipka F 5*). The prepositions *s* and *k* (in Slovenian also *v, z, h*) before a word stay prepositions, abbreviations (*USB*, *s.a.r.s.*) and letters in compounds (*S-klasa*) are read by name, and the foreign letters *q, w, x, y* always by name.

**The spelling dictionary and letter sounds.** Your entries in the spelling dictionary (section [5.4](#54-spelling-dictionary-spellingjson)) apply in both modes, with one exception: when letters are spelled by sound, an entry that is only a letter's name (*be*, *jot*, *lje*) counts as a name too and gives way to the sound, while an entry that says something else (for example, *B* → *bum*) is spoken as you wrote it. This way, old entries you used to correct the names don't turn off the sounds.

### 4.8 Laprdus Configurator (Windows)

On Windows, you can adjust additional settings in the Laprdus Configurator:

1. Open the Windows Start menu, find Laprdus Configurator in the Laprdus program folder and press Enter to open it.
2. Or, if you have Laprdus installed as an NVDA add-on, open the NVDA menu > Laprdus submenu > Laprdus Configurator

The Configurator is available in Croatian, Serbian, Slovenian and English. It follows the Windows display language, and when you open it from the NVDA menu, it follows NVDA's language. A Bosnian display language gives Croatian text, and any other language gives English. The installer also offers those four languages, and the NVDA add-on follows NVDA's language.

The Configurator lets you adjust:

- Speech rate, pitch and volume (NVDA's sliders speed up, slow down, raise and lower the rate and pitch, unless forcing the Laprdus values is on; see [4.1](#41-speech-rate))
- Inflection and acceleration, for all voices: inflection from 0% (monotone) through 50% (default, natural movements) to 100% (movements twice as large), in SAPI5 programs (in NVDA, NVDA's Inflection slider sets it); acceleration multiplies the speech rate in SAPI5 programs (0.5x-3.0x, default 1.0x) and isn't used in NVDA, because NVDA's Rate boost provides higher rates. The speed and pitch sliders go from 0.25x to 4.0x for the formant voices.
- Voice quality: head size, breathiness, roughness and brightness, with the "Voice quality..." button under the volume boost (see [4.9](#49-voice-quality))
- How numbers are read
- Pause lengths and spelling speed
- Spelling letters by their sounds instead of their names
- Customizing the user dictionaries

Settings are saved in the user folder `%APPDATA%\Laprdus` and are shared between SAPI5 and the NVDA add-on.

### 4.9 Voice quality

The four voice quality settings follow the voice-parameter model of a classic formant synthesizer: head size, breathiness, roughness and brightness. Under NVDA they are NVDA's own voice settings: the sliders Head size, Breathiness, Roughness and Brightness in NVDA's voice settings, after the volume, and in the synth settings ring (Ctrl+NVDA+arrow keys), each from 0 to 100, where 50 is the voice as it is; NVDA saves them like its other settings. The Laprdus Configurator has the same four behind its "Voice quality..." button, but those values apply to SAPI5 only (stored in `settings.json` as `speech.head_size`, `speech.breathiness`, `speech.roughness` and `speech.brightness`), as its inflection settings do.

- **Head size** – the length of the vocal tract: 0 is a quarter longer (lower formants, a bigger voice), 100 a quarter shorter (higher formants, a smaller voice). Works for every voice.
- **Breathiness** – the breath noise in the voice: 0 is none, 100 five times as much as in the voice as it is. Works for the formant voices (Zvonko, Stojan, Mirsad and the singing voices).
- **Roughness** – the cycle-to-cycle irregularities of pitch and loudness (jitter and shimmer): 0 is none, 50 the voice as it is; above 50 every second cycle of the voice comes sooner and weaker, a rasp that grows up to 100. Works for the formant voices.
- **Brightness** – the tone: 0 is dark and muffled, 100 sharp and bright. Works for every voice: for the formant voices through the voice source, for the recorded voices through a spectral tilt around 1.5 kHz.

---

## 5. Dictionaries

Dictionaries let you customize how Laprdus pronounces particular words, abbreviations, symbols and emoji. With dictionaries, you can correct the pronunciation of foreign words, add a pronunciation for abbreviations or customize how special characters are read.

### 5.1 Types of dictionaries

Laprdus uses three types of dictionaries, each with its own purpose, plus the accent dictionary for the formant voices:

| Dictionary type | File | Purpose |
|----------------|----------|---------|
| **Main dictionary** | `user.json` | Replaces words and phrases with a custom pronunciation |
| **Spelling dictionary** | `spelling.json` | Sets the pronunciation of individual characters (letters, numbers, symbols) |
| **Emoji dictionary** | `emoji.json` | Converts emoji to a text description |
| **Accent dictionary** | `accents.json` | Sets the stress, length and tone of words for formant voices (see [5.11](#511-accent-dictionary-accentsjson)) |

### 5.2 Dictionary file locations

The location of the dictionary files depends on your platform:

#### Windows (SAPI5 and NVDA)

All dictionary files are in the Laprdus user folder:

```
%APPDATA%\Laprdus\
```

The full path is usually: `C:\Users\VašeKorisničkoIme\AppData\Roaming\Laprdus\`

**How to open this folder:**

1. Press Windows+R to open the "Run" dialog
2. Type: `%APPDATA%\Laprdus`
3. Press Enter

Files in this folder:

| File | Description |
|----------|------|
| `settings.json` | Main Laprdus settings |
| `user.json` | Your user pronunciation dictionary |
| `spelling.json` | Spelling dictionary (character pronunciation) |
| `emoji.json` | Emoji dictionary |
| `accents.json` | Accent dictionary for the formant voices |

**Note:** These files are created automatically the first time you add an entry in the Laprdus Configurator. You can also create them by hand.

#### Linux

Dictionary files are in:

```
~/.config/Laprdus/
```

Use the same file names as on Windows (`user.json`, `spelling.json`, `emoji.json`, `accents.json`).

#### Android

On Android, you edit dictionaries only through the interface of the Laprdus app. The files are stored in the app's internal storage and aren't directly accessible to you.

### 5.3 Main dictionary (user.json)

The main dictionary replaces words or phrases with a custom pronunciation. It's useful for:

- **Foreign words and brands** - for example, pronouncing "Facebook" as "Fejzbuk"
- **Abbreviations** - for example, pronouncing "TV" as "Te Ve"
- **Technical terms** - for example, pronouncing "ChatGPT" as "ČetDžipiti"
- **Names with an unusual pronunciation** - for example, pronouncing "Sean" as "Šon"

#### File format

The `user.json` file uses the JSON format. Here's an example:

```json
{
    "version": "1.0",
    "entries": [
        {
            "grapheme": "Facebook",
            "phoneme": "Fejzbuk",
            "caseSensitive": false,
            "wholeWord": true,
            "comment": "Social network"
        },
        {
            "grapheme": "TV",
            "phoneme": "Te Ve",
            "caseSensitive": false,
            "wholeWord": true,
            "comment": "Abbreviation for television"
        },
        {
            "grapheme": "ChatGPT",
            "phoneme": "ČetDžipiti",
            "caseSensitive": false,
            "wholeWord": true,
            "comment": "AI chatbot"
        }
    ]
}
```

#### Field descriptions

| Field | Required | Description | Default value |
|-------|----------|------|-------------------|
| `grapheme` | Yes | The original text to replace | - |
| `phoneme` | Yes | The replacement pronunciation | - |
| `caseSensitive` | No | Whether uppercase and lowercase letters are treated as different | `false` |
| `wholeWord` | No | Replaces only whole words | `true` |
| `comment` | No | A comment for reference (doesn't affect pronunciation) | - |

#### Options explained

**caseSensitive (case sensitivity):**

- `false` (default): "Facebook", "facebook" and "FACEBOOK" are all replaced
- `true`: Only an exact match is replaced

**wholeWord (whole word only):**

- `true` (default): Replaces the text only when it's a whole word
    - "TV" in "TV program" is replaced
    - "TV" in "Latvia" isn't replaced (because it's part of a larger word)
- `false`: Also replaces the text inside other words
    - Useful for suffixes and prefixes

### 5.4 Spelling dictionary (spelling.json)

The spelling dictionary defines how individual characters are pronounced when the screen reader spells text (reads it character by character). It's useful for:

- **Numbers** - for example, pronouncing "5" as "pet"
- **Punctuation** - for example, pronouncing "." as "točka"
- **Special characters** - for example, pronouncing "@" as "at"
- **Letters of the alphabet** - if you want a letter to have a different name than the one Laprdus gives it, for example, "W" as "dvostruko ve"

Laprdus names the letters of the alphabet itself, in the selected language (see [4.7](#47-spelling)), so the built-in dictionary doesn't contain them; your entry for a letter takes precedence over the built-in name. When spelling by sounds is on, an entry that is only a letter's name is ignored, and any other entry applies.

#### File format

```json
{
    "version": "1.0",
    "entries": [
        { "character": "A", "pronunciation": "A" },
        { "character": "B", "pronunciation": "Be" },
        { "character": "Č", "pronunciation": "Če" },
        { "character": "1", "pronunciation": "jedan" },
        { "character": ".", "pronunciation": "točka" },
        { "character": "@", "pronunciation": "at" }
    ]
}
```

#### Built-in character pronunciations

Laprdus comes with a built-in spelling dictionary of numbers, punctuation and symbols (the names are Croatian). Letters are named as follows:

**Letters of the alphabet** (built into Laprdus, not into the dictionary):

| Letter | Croatian and Bosnian | Serbian | Letter | Croatian and Bosnian | Serbian |
|-------|---------------------|--------|-------|---------------------|--------|
| A | a | a | N | en | en |
| B | be | be | NJ, Њ | enj | nje |
| C | ce | ce | O | o | o |
| Č | če | če | P | pe | pe |
| Ć | će | će | R | er | er |
| D | de | de | S | es | es |
| DŽ, Џ | dže | dže | Š | eš | ša |
| Đ | đe | đe | T | te | te |
| E | e | e | U | u | u |
| F | ef | ef | V | ve | ve |
| G | ge | ge | Z | ze | ze |
| H | ha | ha | Ž | že | že |
| I | i | i | Q | ku | ku |
| J | je | je | W | duplo ve | duplo ve |
| K | ka | ka | X | iks | iks |
| L | el | el | Y | ipsilon | ipsilon |
| LJ, Љ | elj | lje | | | |

Cyrillic letters are named like their Latin counterparts, and uppercase and lowercase letters are named the same.

**Numbers:**

| Character | Pronunciation |
|------|---------|
| 0 | nula |
| 1 | jedan |
| 2 | dva |
| 3 | tri |
| 4 | četiri |
| 5 | pet |
| 6 | šest |
| 7 | sedam |
| 8 | osam |
| 9 | devet |

**Common punctuation:**

| Character | Pronunciation |
|------|---------|
| . | točka |
| , | zarez |
| ! | uskličnik |
| ? | upitnik |
| : | dvotočka |
| ; | točka zarez |
| - | crtica |
| _ | donja crtica |
| ( | otvorena zagrada |
| ) | zatvorena zagrada |
| " | navodnik |
| ' | apostrof |

**Special characters:**

| Character | Pronunciation |
|------|---------|
| @ | at |
| # | ljestve |
| $ | dolar |
| % | posto |
| & | i |
| * | zvjezdica |
| + | plus |
| = | jednako |
| / | kosa crta |
| \ | obrnuta kosa crta |

### 5.5 Emoji dictionary (emoji.json)

The emoji dictionary converts emoji symbols to spoken text.

#### Turning on emoji reading

**Windows (Laprdus Configurator):**

1. Open the Laprdus Configurator (Start > Laprdus > Laprdus Configurator)
2. Press Tab to find the "Speak emoji" option
3. Select the check box
4. Press Tab to select "OK" and press Enter

**NVDA:**

1. Open the NVDA menu (NVDA+N) > Laprdus submenu > Laprdus Configurator
2. Press Tab to find the "Speak emoji" option
3. Select the check box
4. Press Tab to select "OK" and press Enter

**Android:**

1. Open the Laprdus app
2. Tap "Laprdus Settings"
3. Find the "Enable emoji reading" switch and turn it on

#### File format

```json
{
    "version": "1.0",
    "entries": [
        { "emoji": "😀", "text": "nasmijano lice" },
        { "emoji": "👍", "text": "palac gore" },
        { "emoji": "❤️", "text": "crveno srce" },
        { "emoji": "🎉", "text": "konfeti" }
    ]
}
```

#### Built-in emoji

Laprdus comes with an extensive built-in emoji dictionary that contains over 1,100 emoji with Croatian descriptions. Here are a few examples:

| Emoji | Description |
|--------|------|
| 😀 | nasmijano lice |
| 😂 | lice sa suzama radosnicama |
| 😍 | nasmiješeno lice s očima u obliku srca |
| 🤔 | zamišljeno lice |
| 👍 | palac gore |
| 👎 | palac dolje |
| ❤️ | crveno srce |
| 🔥 | vatra |
| ⭐ | zvijezda |
| ✅ | kvačica |

### 5.6 Editing dictionaries on Windows (Laprdus Configurator)

The Laprdus Configurator provides a graphical interface for managing dictionaries easily. This is the **recommended method** if you prefer a graphical interface.

**How to open the Configurator:**

- **From the Start menu:** Start > Laprdus > Laprdus Configurator
- **From the NVDA menu:** NVDA+N > Laprdus submenu > Laprdus Configurator

**Editing dictionaries:**

1. In the Laprdus Configurator, use Tab or Shift+Tab to find the **"Dictionaries..."** button and press Enter
2. The Dictionary Management window opens
3. At the top of the window is a drop-down list for choosing the type of dictionary you want to customize:
    - **Main user dictionary** - for replacing words and phrases
    - **Spelling dictionary** - for the pronunciation of individual characters
    - **Emoji dictionary** - for converting emoji to text
    - **Accent dictionary** - for the stress, length and tone of a word, for all its forms at once (notation in [5.11](#511-accent-dictionary-accentsjson))
4. Use the Down Arrow key to select the dictionary type you want

**Adding a new entry:**

1. Click the **"Add"** button
2. The entry dialog opens:
    - **Original string:** Type the text you want to replace
    - **Replacement string:** Type how you want the text pronounced
    - **Case sensitive:** Select this if an exact match matters
    - **Whole word only:** Select this if you don't want replacements inside other words
    - **Comment:** An optional description of the entry
3. Press Enter, or press Tab to find the **"OK"** button and press Enter, to save the entry

**Editing an existing entry:**

1. Select the entry in the list with the Down or Up Arrow key
2. Press Tab to find the **"Edit"** button; if you use a mouse, you can also double-click the entry
3. Change the values you want
4. Press Enter, or press Tab to find the **"OK"** button and press Enter

**Deleting an entry:**

1. Select the entry in the list with the Up or Down Arrow key
2. Press Tab to find the **"Delete"** button and press Enter

**Duplicating an entry:**

1. Select the entry you want to duplicate
2. Press Tab to find the **"Duplicate"** button and press Enter
3. A new entry with the copied values is added to the list
4. Edit the duplicated entry as needed

**Accent dictionary entries:**

When the accent dictionary is selected, the entry dialog has different fields:

- **Entry:** a word, stem, list of forms or verb with accent marks, in the notation from [5.11](#511-accent-dictionary-accentsjson), for example, `kontr'o:l*` or `ur'e:d=i<p`
- **Kind of entry:** *Word* (a word, a stem with `*` or forms with `|`) or *Verb* (a whole stem with the `=` sign and the class, or a root with a long *ije*)
- **Language:** *All languages*, or Croatian, Serbian, Bosnian or Slovenian if the entry applies to only one language
- **Comment:** an optional note, for example, the forms the entry covers

A short reminder of the marks appears below the fields. The entry is checked with the same rules Laprdus reads it by: the Configurator rejects an entry with no accent mark, with two accents or with an unknown verb class and tells you what's wrong, so an entry that would be skipped at load time can't get into the file. Duplicating doesn't change the text of the entry.

### 5.7 Editing dictionaries on Android

The Android app provides a built-in interface for managing dictionaries.

**Step 1: Open the dictionaries**

1. Open the Laprdus app from the app drawer
2. Tap the **"Laprdus Settings"** button
3. Tap the **"Manage dictionaries"** button

**Step 2: Choose the dictionary type**

At the top of the screen is a drop-down list for choosing the dictionary type:

- **Main Dictionary** - for replacing words and phrases
- **Spelling Dictionary** - for pronouncing characters
- **Emoji Dictionary** - for converting emoji
- **Accent Dictionary** - for word stress (notation in [5.11](#511-accent-dictionary-accentsjson))

Tap the drop-down list and select the type you want.

**Step 3: Add a new entry**

1. Tap the **"+"** (plus) button in the lower-right corner of the screen
2. The entry screen opens:
    - **Original text:** Type the word or phrase you want to replace
    - **Replacement pronunciation:** Type how you want it pronounced
    - **Case sensitive:** Turn this on if an exact match matters
    - **Match whole word only:** Turn this on if you don't want replacements inside other words
    - **Comment (optional):** A description of the entry (for example, "Social network")
3. Tap the **"Save"** button at the bottom of the screen

For the accent dictionary, the entry screen has the fields **Entry** (a word, stem, forms or verb with its accent, for example, `kontr'o:l*`), **Kind of entry** (word or verb), **Language** (all languages or one of the four) and **Comment (optional)**, with a reminder of the marks below them. The app rejects an entry that isn't valid and shows the reason below the field.

**Step 4: Edit an existing entry**

1. Find the entry in the list
2. Tap the entry to open the editing screen
3. Change the values you want
4. Tap **"Save"**

**Step 5: Delete an entry**

1. Find the entry in the list
2. Swipe the entry left to show the delete option
3. Tap the trash can icon
4. Confirm the deletion

**Note for TalkBack users:**

- Use the standard TalkBack gestures to navigate
- Double-tap to activate buttons
- To delete: double-tap and hold the entry, then select "Delete"

### 5.8 Editing JSON files by hand (advanced)

This method is for **advanced users** who want to edit the JSON dictionary files directly. For most users, we recommend the Laprdus Configurator (Windows) or the interface of the Android app.

**Benefits of editing by hand:**

- Faster bulk adding and editing of a large number of entries
- You can share dictionaries with other users
- You can make backup copies of your dictionaries

**Step 1: Open the dictionaries folder**

**Windows:**

1. Press Windows+R
2. Type: `%APPDATA%\Laprdus`
3. Press Enter

**Linux:**

1. Open a file manager
2. Open the hidden `.config` folder in your home folder
3. Open the `Laprdus` folder
    - Or in a terminal: `cd ~/.config/Laprdus`

**Step 2: Open or create the file**

- If the file already exists, open it in a text editor (Windows: Notepad, Linux: gedit, nano, vim)
- If the file doesn't exist, create a new text file with the matching name:
    - `user.json` for the main dictionary
    - `spelling.json` for the spelling dictionary
    - `emoji.json` for the emoji dictionary
    - `accents.json` for the accent dictionary of the formant voices (notation in [5.11](#511-accent-dictionary-accentsjson))

**Step 3: Edit the content**

Use the JSON format described in the previous sections (5.3, 5.4, 5.5). Example for `user.json`:

```json
{
    "version": "1.0",
    "entries": [
        {
            "grapheme": "YouTube",
            "phoneme": "Jutjub",
            "caseSensitive": false,
            "wholeWord": true,
            "comment": "Video platform"
        },
        {
            "grapheme": "Google",
            "phoneme": "Gugl",
            "caseSensitive": false,
            "wholeWord": true,
            "comment": "Search engine"
        }
    ]
}
```

**IMPORTANT - JSON syntax rules:**

- All keys and text values must be inside double quotation marks `"`
- Commas `,` between entries, BUT NOT after the last entry
- Curly braces `{}` around each entry
- Square brackets `[]` around the list of entries

**Step 4: Save the file**

- You **must** save the file with **UTF-8 encoding** (to support Croatian characters)
- **Windows Notepad:** File > Save As > Encoding: UTF-8
- **Linux:** Most editors use UTF-8 automatically

**Step 5: Restart the synthesizer**

- **Windows SAPI5:** Close and reopen the app that uses Laprdus
- **NVDA:** Press NVDA+Q and restart NVDA, or switch to another synthesizer and then back to Laprdus
- **Linux:** Restart the app or Speech Dispatcher: `systemctl --user restart speech-dispatcher`

**Note:** If you edit a dictionary while the Android app is running, the changes don't appear until you refresh the list or reopen the app.

### 5.9 Practical examples

#### Example 1: Foreign companies and brands

Many foreign companies and brands are pronounced differently from how they're written:

```json
{
    "grapheme": "Microsoft",
    "phoneme": "Majkrosoft",
    "caseSensitive": false,
    "wholeWord": true
}
```

```json
{
    "grapheme": "Google",
    "phoneme": "Gugl",
    "caseSensitive": false,
    "wholeWord": true
}
```

```json
{
    "grapheme": "WhatsApp",
    "phoneme": "Watsap",
    "caseSensitive": false,
    "wholeWord": true
}
```

#### Example 2: Abbreviations

Abbreviations you want read letter by letter:

```json
{
    "grapheme": "TV",
    "phoneme": "Te Ve",
    "caseSensitive": false,
    "wholeWord": true
}
```

```json
{
    "grapheme": "HR",
    "phoneme": "Ha Er",
    "caseSensitive": false,
    "wholeWord": true,
    "comment": "Abbreviation for Croatia"
}
```

```json
{
    "grapheme": "EU",
    "phoneme": "E U",
    "caseSensitive": false,
    "wholeWord": true,
    "comment": "European Union"
}
```

#### Example 3: Technical terms

```json
{
    "grapheme": "ChatGPT",
    "phoneme": "ČetDžipiti",
    "caseSensitive": false,
    "wholeWord": true
}
```

```json
{
    "grapheme": "WiFi",
    "phoneme": "Vajfaj",
    "caseSensitive": false,
    "wholeWord": true
}
```

```json
{
    "grapheme": "USB",
    "phoneme": "U Es Be",
    "caseSensitive": false,
    "wholeWord": true
}
```

#### Example 4: Names with an unusual pronunciation

```json
{
    "grapheme": "Sean",
    "phoneme": "Šon",
    "caseSensitive": false,
    "wholeWord": true,
    "comment": "Irish name"
}
```

```json
{
    "grapheme": "Elon",
    "phoneme": "Ilon",
    "caseSensitive": false,
    "wholeWord": true
}
```

#### Example 5: Replacing inside a word (wholeWord: false)

To replace a suffix or prefix, set `wholeWord` to `false`:

```json
{
    "grapheme": "tion",
    "phoneme": "šen",
    "caseSensitive": false,
    "wholeWord": false,
    "comment": "English suffix -tion"
}
```

With this entry, words such as "information" contain the pronunciation "šen" instead of "tion".

### 5.10 Troubleshooting dictionaries

#### Problem: Dictionary changes don't apply

**Possible causes and solutions:**

1. **Applying user dictionaries is turned off**
    - Turn on the **"Apply user dictionaries"** option in the Laprdus Configurator (Windows) or in Laprdus Settings (Android)

2. **Syntax error in the JSON file**
    - Check that all quotation marks are closed correctly
    - Check that the commas are in the right places
    - Use an online JSON validator to check the syntax

3. **The file wasn't saved with UTF-8 encoding**
    - In Notepad: File > Save As > Encoding: UTF-8
    - In other editors: check the encoding settings

4. **The synthesizer wasn't restarted**
    - Close and reopen the app
    - For NVDA: Insert+Q > Restart

5. **The file is in the wrong location**
    - Windows: `%APPDATA%\Laprdus\`
    - Linux: `~/.config/Laprdus/`

#### Problem: A word is replaced in the wrong places

**Solution:** Turn on the **"Whole word only"** option for the entry that causes the problem, or set `wholeWord: true` for that entry by hand in the configuration file

Example of the problem: The entry for "TV" also replaces part of "Latvia" because it contains "TV".

```json
{
    "grapheme": "TV",
    "phoneme": "Te Ve",
    "wholeWord": true
}
```

#### Problem: Replacement doesn't work for uppercase or lowercase letters

**Solution:** Check the **"Case sensitive"** option for the entry that causes the problem, or check the `caseSensitive` value for that entry in the configuration file

- To replace regardless of case: `caseSensitive: false`, that is, off
- For an exact match: `caseSensitive: true`, that is, on

#### Problem: JSON syntax error

Common errors in the JSON format:

**Missing comma:**

```json
// WRONG:
{
    "grapheme": "test"
    "phoneme": "test"
}

// CORRECT:
{
    "grapheme": "test",
    "phoneme": "test"
}
```

**Trailing comma:**

```json
// WRONG:
{
    "entries": [
        { "grapheme": "a", "phoneme": "b" },
    ]
}

// CORRECT:
{
    "entries": [
        { "grapheme": "a", "phoneme": "b" }
    ]
}
```

**Missing curly braces:**

```json
// WRONG:
"grapheme": "test",
"phoneme": "test"

// CORRECT:
{
    "grapheme": "test",
    "phoneme": "test"
}
```

#### Problem: Emoji aren't read

**Solution:** Check that emoji reading is turned on

1. Open the Laprdus Configurator or the Android settings
2. Find the "Speak emoji" option ("Enable emoji reading" on Android)
3. Turn the option on
4. Save the settings

---

### 5.11 Accent dictionary (accents.json)

The formant voices (Zvonko, Stojan, Mirsad and the singing voices) work out the position of the stress and the length and tone of every word themselves: first from accent marks in the text, then from the built-in lexicon, then by rules for endings and word types (loanwords and derived words with a long last stem syllable, prefixed verbs, verbs in -ovati), and if nothing matches, the stress goes on the first syllable. You can correct a word that comes out wrong in the accent dictionary, and not just one of its forms: an entry can cover a whole stem, a list of forms or a whole verb with all its persons and tenses. The recorded voices (Josip, Vlado and the derived voices) determine stress the same way and use the same dictionary; the dictionary applies to the selected language, whichever voice is speaking.

The `accents.json` file is next to the other dictionaries (`%APPDATA%\Laprdus` on Windows, `~/.config/Laprdus` on Linux) and is loaded with them when user dictionaries are on. It's loaded once, at startup and when the file changes, so it doesn't slow speech down at all.

#### Notation

Entries are written in the notation of the built-in lexicon:

| Mark | Meaning |
|------|----------|
| `'` | before the stressed vowel (or syllabic r); the tone is chosen automatically |
| `^` | before the stressed vowel: falling accent |
| `/` | before the stressed vowel: rising accent |
| `:` | after a long vowel |
| `*` | at the end: a stem, which also covers forms with up to three more letters (kontróla, kontróle, kontrólom) |
| `\|` | `osnova\|nastavak\|nastavak` (stem, then endings): one exact form for each ending; an empty ending is the stem itself |

Example file:

```json
{
    "version": "1.0",
    "entries": [
        { "word": "kontr'o:l*", "comment": "kontróla, kontróle, kontrólom" },
        { "word": "sign'a:l|a|u|om|e|i|ima", "comment": "signála, signálu...; sìgnal stays as it is" },
        { "word": "p^oruk*", "comment": "pȍruka: falling accent on the first syllable" },
        { "word": "dir'ektor*", "language": "sr", "comment": "Serbian only, with any voice" },
        { "verb": "ur'e:d=i<p", "comment": "uréditi, urédim, urédio, uređen" },
        { "verb": "dijel:i", "comment": "podijeliti, raspodijeliti, podijelim, podijeljen" }
    ]
}
```

- `word`: a word, stem (`*`) or list of forms (`|`).
- `verb`: a verb. Write the whole stem with the accent of the infinitive, the `=` sign and the class: `i` for verbs in -iti/-im, `a` for -ati/-am, `u` for -nuti/-nem, `t` for -ati with a different stem in the present tense and `e` for that stem (`pok'a:z=t` and `pok'a:ž=e`). Markers can follow the class: `<` moves the accent one syllable back in the present tense, `p` also in the passive participle (that's how Stojan and Mirsad speak; Zvonko keeps the accent of the infinitive), and `n` is for an imperative that is the same as a form of a noun (potvrdi, uredi), so it's read as a verb only at the start of a sentence. A verb with a long *ije* in its root is written as `korijen:vrsta` (root:class) and applies with every prefix.
- `language`: `hr`, `sr`, `bs` or `sl` limits the entry to that language, whichever voice is speaking; without it, the entry applies to all languages. A Slovenian entry is written in the notation of Slovenian dictionaries (`mésto`, `člôvek`, `p'əs`, `d'é:la*`): an acute for a long stressed vowel (close é, ó), a circumflex for long open ê, ô, a grave for a short stressed vowel, `ə` for the schwa and `w` for an l pronounced like u (bíw).
- `comment`: a note for you; Laprdus doesn't read it.

An exact form takes precedence over a stem, a longer stem over a shorter one, and your entry over a built-in one. Accent marks written in the text itself, or in a main dictionary replacement (`telèfon`), take precedence over everything.

An invalid entry (no accent mark, two accents, an unknown letter or an unknown verb class) is skipped, and the others apply. On the command line, `laprdus -w` prints how many entries were accepted and what was wrong with the first one rejected; the NVDA add-on writes this to its log. You can hear the result right away:

```bash
laprdus -v zvonko "kontrola, signala, uredio"
```

**Editing:** you edit the accent dictionary like the other dictionaries: in the Laprdus Configurator on Windows (**Dictionaries...** button, dictionary type *Accent dictionary*, see [5.6](#56-editing-dictionaries-on-windows-laprdus-configurator)), in the Android app ([5.7](#57-editing-dictionaries-on-android)) and in the app for iPhone, iPad and Mac (Dictionaries tab, type *Accent Dictionary*). Every entry has the fields Entry, Kind of entry (word or verb), Language and Comment, and the editor checks it with the same rules Laprdus reads it by, so an invalid entry can't be saved. You can still edit the file by hand ([5.8](#58-editing-json-files-by-hand-advanced)); the apps on Android and Apple read it from the same folder as the other dictionaries when user dictionaries are on.

## 6. Voices

### 6.1 Available voices

Laprdus includes eighteen voices - two base, three derived, three formant and ten singing. **Every voice speaks every language.** The voice determines only the sound; the language (Croatian, Serbian, Bosnian or Slovenian) determines the pronunciation rules, the accent dictionary, the number words (*tisuća* / *hiljada* / *tisoč*) and the letter names when spelling, and you choose it separately from the voice, in a different way on each platform (see [6.3](#63-choosing-a-voice)). *Native language* in the table is the language a voice speaks when nothing else is requested. **The default voice is Zvonko**, on all platforms and for all languages. The voice ID (in the ID column) is written without diacritics on purpose, so that voices register safely on every platform; IDs from earlier versions (`djed`, `klapa`, `pjevac`, `pevac`, `solist`) still work in saved settings and scripts.

| Voice | ID | Type | Native language | Description |
|------|--------|-------|---------------|------|
| **Josip** | `josip` | Base | Croatian | Male voice of normal pitch. |
| **Vlado** | `vlado` | Base | Serbian | Male voice of normal pitch. |
| **Detence** | `detence` | Derived | Croatian | Child's voice. Derived from the Josip voice with a raised pitch. |
| **Baba** | `baba` | Derived | Croatian | Female voice. Derived from the Josip voice with a slightly raised pitch. |
| **Đedo** | `djedo` | Derived | Serbian | Older male voice. Derived from the Vlado voice with a lowered pitch. |
| **Zvonko** | `zvonko` | Formant | Croatian | Male voice. Speech is generated by rule, without recordings. Default voice. |
| **Stojan** | `stojan` | Formant | Serbian | Male voice, deeper and darker. Speech is generated by rule, without recordings. |
| **Mirsad** | `mirsad` | Formant | Bosnian | Male voice, higher and more melodic. Speech is generated by rule, without recordings. |
| **Orguljaš - Vila Velebita** | `orguljas` | Singing | Croatian | Zvonko at the organ: sings *Vila Velebita*. |
| **Dalmacija Klapa** | `dalmacija_klapa` | Singing | Croatian | Zvonko as a whole klapa: sings *Vila Velebita*. |
| **Lika Velebit** | `lika_velebit` | Singing | Croatian | Zvonko sings in his own voice, at his own pitch: *Vila Velebita*. |
| **Trubač - Kreće se lađa francuska** | `trubac` | Singing | Serbian | Stojan as a trumpet: sings *Kreće se lađa francuska*. |
| **Harmonikaš - Kreće se lađa francuska** | `harmonikas` | Singing | Serbian | Stojan as an accordion: sings *Kreće se lađa francuska*. |
| **Solunac** | `solunac` | Singing | Serbian | Stojan sings in his own voice: *Kreće se lađa francuska*. |
| **Sevdalija - Kad ja pođoh na Bembašu** | `sevdalija` | Singing | Bosnian | Mirsad sings sevdah: *Kad ja pođoh na Bembašu*. |
| **Sazlija - Kad ja pođoh na Bembašu** | `sazlija` | Singing | Bosnian | Mirsad as a saz: sings *Kad ja pođoh na Bembašu*. |
| **Sarajevo Bembaša** | `sarajevo_bembasa` | Singing | Bosnian | Mirsad sings in his own voice: *Kad ja pođoh na Bembašu*. |
| **Bećarac** | `becarac` | Singing | Croatian | Zvonko sings a bećarac in his own voice: every ten-syllable couplet fits the tune. |

### 6.2 Base and derived voices

**Base voices** (Josip and Vlado) use physical phoneme recordings: real sound recordings of a speaker.

**Derived voices** (Detence, Baba, Đedo) use the same sound recordings as the base voices, but with an adjusted pitch:

- **Detence** uses Josip's phonemes at a pitch of 1.5x (higher voice)
- **Baba** uses Josip's phonemes at a pitch of 1.2x (slightly higher voice)
- **Đedo** uses Vlado's phonemes at a pitch of 0.75x (lower voice)

Base and derived voices, like all the others, speak all four languages: Josip says *hiljada* and spells *lje* in Serbian, Vlado says *tisuća* and *elj* in Croatian, and *tisoč* in Slovenian.

**Formant voices** (Zvonko, Stojan, Mirsad) use no recordings. Speech is computed entirely by rule, as with classic formant synthesizers: the sounds, stress and sentence intonation are worked out from the text, and then a formant synthesizer generates the audio. As a result, these voices:

- don't need voice data files
- stay clean and intelligible at very high rates, because rate and pitch don't come from audio processing after the fact
- have sentence intonation (statement, question, comma, exclamation)
- say numbers, like all voices, according to the selected language (Croatian: *tisuća*, *milijun*; Serbian: *hiljada*, *milion*, *dvesta*; Bosnian: *hiljada*, *milion*, *dvjesto*; Slovenian: *tisoč*, *milijon*, *enaindvajset*)

If a formant voice stresses a word on the wrong syllable, you can set the stress in the pronunciation dictionary with the usual accent marks, for example, `telèfon`, `gláva`, `kȕća` or `grȃd`.

**Singing voices** (Orguljaš - Vila Velebita, Dalmacija Klapa, Lika Velebit and Bećarac in Zvonko's voice; Trubač - Kreće se lađa francuska, Harmonikaš - Kreće se lađa francuska and Solunac in Stojan's; Sevdalija - Kad ja pođoh na Bembašu, Sazlija - Kad ja pođoh na Bembašu and Sarajevo Bembaša in Mirsad's) are formant voices that sing text instead of speaking it, as the singing voices of classic synthesizers and desktop computers did. Each syllable gets one note of a folk song (Zvonko's voices sing *Vila Velebita*, Stojan's *Kreće se lađa francuska*, Mirsad's *Kad ja pođoh na Bembašu*), in whichever of the four languages is selected; the first verse of the song fits the melody exactly, and any other text is sung to the same melody, syllable by syllable, starting over with each new sentence the program sends. Speech rate sets the tempo, pitch transposes the song, and inflection sets the vibrato depth (0% turns it off). Instead of a larynx, each voice has its own "instrument": an organ with church reverb, a klapa of several voices with a bass an octave lower, a trumpet, two accordion reeds tuned a few cents apart, a sevdah singer's voice with a wide vibrato, and a saz with tremolo. Lika Velebit, Solunac and Sarajevo Bembaša sing in the ordinary voices of Zvonko, Stojan and Mirsad, with no instrument or reverb, and their song is transposed to the pitch at which they normally speak.

### 6.3 Choosing a voice

#### Windows SAPI5

SAPI5 offers four voices, one for each language: Laprdus Croatian, Laprdus Serbian, Laprdus Bosnian and Laprdus Slovenian. They choose the language; you choose the voice they speak with in the Laprdus Configurator, and it applies to all four (in `settings.json` it's `voice.default`; there's no key for the language, because the program that's speaking chooses it). The default voice is Zvonko.

1. Open the Laprdus Configurator, select the voice you want in the Voice Settings group (Josip, Vlado, Detence, Baba, Đedo, Zvonko, Stojan, Mirsad or a singing voice), and save
2. Open Control Panel
3. Select "Ease of Access" > "Speech Recognition" > "Text to Speech"
4. In the "Voice selection" drop-down list, select the language: Laprdus Croatian, Laprdus Serbian, Laprdus Bosnian or Laprdus Slovenian
5. Click "Apply"

#### NVDA

In NVDA, all Laprdus voices are available for every language: you choose the voice and the language separately (see [3.3](#33-nvda-settings)). The default voice is Zvonko.

**Method 1: Speech settings**

1. Open the NVDA menu (NVDA+N)
2. Select Preferences > Settings > Speech
3. In the "Voice" drop-down list, select the voice you want (the list contains all 18 voices)
4. In the "Language" drop-down list, select Croatian, Serbian, Bosnian or Slovenian
5. Press "OK" or "Apply"

**Method 2: Synth settings ring**

1. Press Ctrl+NVDA+Left Arrow or Right Arrow until you reach the "Voice" or "Language" setting, then use the Up or Down Arrow key to change the value
2. Or press Ctrl+NVDA+V to open the voice selection dialog

If NVDA's "Automatic language switching (when supported)" option is on, Laprdus switches among Croatian, Serbian, Bosnian and Slovenian on its own, according to the language of the text, without changing the voice.

#### Linux (Speech Dispatcher)

Speech Dispatcher lists every voice for every language. The language of the request (`-l`) sets the language, and the voice (`-y`) stays the one you chose; with no voice chosen, Zvonko speaks, in all four languages. The symbolic voices are the same for all languages: MALE1 is Zvonko, MALE2 Josip, MALE3 Đedo, FEMALE1 Baba, CHILD_MALE Detence.

```bash
# List the available voices
spd-say -o laprdus -L

# Use a specific voice (it speaks its native language)
spd-say -o laprdus -y Josip "Tekst na hrvatskom"
spd-say -o laprdus -y Vlado "Tekst na srpskom"

# Any voice in any language
spd-say -o laprdus -l sr -y Zvonko "Zvonko govori srpski"
spd-say -o laprdus -l hr -y Vlado "Vlado govori hrvatski"
spd-say -o laprdus -l bs "Zvonko govori bosanski"
spd-say -o laprdus -l sl "Zvonko govori slovensko"
```

To make a permanent change in the Orca screen reader:

1. Open the Orca settings (Insert+Spacebar)
2. On the "Voice" tab, select the voice you want from the drop-down list; the language Orca requests determines the language it speaks

#### Android

Laprdus offers the system every voice under Croatian, Serbian, Bosnian and Slovenian (72 system voices). The language requested by TalkBack or another app sets the language, and the voice stays the one selected in the app; the default is Zvonko.

1. Open the Laprdus app
2. Tap "Laprdus Settings"
3. In the "Voice" section, tap the **"Voice"** drop-down list
4. Select the voice you want from the list
5. In the **"Language"** menu, select the language for speech in the app itself; with "Force language" on, that language is also used when apps request another one

To change the default voice in the system:

1. Open Settings > Accessibility > Text-to-speech output
2. Tap the gear icon next to "Laprdus TTS"
3. Select the voice you want

#### Apple (iPhone, iPad and Mac)

In Spoken Content (iOS: Settings > Accessibility > Spoken Content > Voices; macOS: System Settings > Accessibility > Spoken Content > System voice > Manage Voices) and in VoiceOver's voice settings, every Laprdus voice is listed under Croatian, Serbian, Bosnian and Slovenian; the item you select chooses both the voice and the language. In the Laprdus app, the "Voice" menu chooses the voice and the "Language" menu chooses the language for speech in the app itself. The default voice is Zvonko.

---

## 7. Troubleshooting

### 7.1 Common problems

#### No sound

1. Check that Laprdus is selected as the active synthesizer
2. Check that the volume isn't set to 0
3. Check the computer's system volume
4. Restart the program you're using

#### Speech is too fast or too slow

Adjust the speech rate:

- In NVDA: NVDA+N > Preferences > Settings > Speech > Rate
- On Android: Open the Laprdus app and adjust the rate slider in Laprdus Settings
- In the command-line program: use the `-r` option

#### NVDA can't find Laprdus

1. Check that the add-on is installed correctly (NVDA menu > Tools > Add-on store...)
2. Restart NVDA
3. Check that the add-on isn't disabled

#### Speech Dispatcher doesn't work on Linux

1. Check that the Laprdus module is installed:

        ls /usr/lib/speech-dispatcher-modules/sd_laprdus

2. Check the configuration:

        grep laprdus /etc/speech-dispatcher/speechd.conf

3. Restart Speech Dispatcher:

        systemctl --user restart speech-dispatcher

### 7.2 Diagnostic logs

#### NVDA add-on

To turn on diagnostic logs for the NVDA add-on:

1. Create an empty file named `laprdus_debug` in the temporary folder
    - Windows: `%TEMP%\laprdus_debug`
2. Restart NVDA
3. Logs are saved to `%TEMP%\laprdus_debug.log`

#### Linux

Use the `-w` option for verbose output:

```bash
laprdus -w "Test"
```

### 7.3 Contact and support

For help and support:

- **Email:** hrvojekatic@gmail.com
- **Website:** https://hrvojekatic.com/laprdus
- **Source code:** https://github.com/hkatic/laprdus

When you report a problem, please include:

- Your Laprdus version
- Your operating system
- The screen reader you use (if applicable)
- A description of the problem
- The steps to reproduce the problem

---

Last updated: November 2026.
