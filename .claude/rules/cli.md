---
paths:
  - "src/platform/cli/**"
---
# The command-line program

Moved from `CLAUDE.md` on 2026-11-23, unchanged. Claude Code loads this file when Claude reads a file matching the paths above; `CLAUDE.md` lists every topic file.

## The CLI on Windows

#### Windows CLI

There is one command-line program, `laprdus` (`src/platform/cli`, crate `laprdus-cli`), for Windows, Linux and macOS. It plays through the system's audio device with the `rodio` crate (WASAPI on Windows).

```bash
cargo xtask windows                    # both architectures, staged in dist/
cargo xtask windows-legacy   # the same for Windows 7 SP1, 8 and 8.1 (dist/windows-legacy-*)
cargo build --release -p laprdus-cli   # the host architecture only
```

Output:
- `dist/windows-x64/laprdus.exe` (64-bit)
- `dist/windows-x86/laprdus.exe` (32-bit)
- `target/release/laprdus.exe` (the host architecture, from `cargo build`)

The CLI is automatically included in the SAPI5 installer. For standalone use:
```bash
# Show help
laprdus.exe -h

# List available voices (each with its native language; every voice speaks every language)
laprdus.exe -l

# Speak text to audio device (Zvonko, Croatian)
laprdus.exe "Dobar dan!"

# Any voice in any language
laprdus.exe -v vlado -L hr "Dobar dan svima!"
laprdus.exe -L sr "Zdravo svete!"

# Output to WAV file
laprdus.exe -o output.wav "Text to speak"

# Use specific voice with options
laprdus.exe -v vlado -r 1.5 -p 1.2 "Zdravo svete!"

# Read from file
laprdus.exe -i input.txt -o output.wav

# Read from stdin
echo "Text" | laprdus.exe -o output.wav
```

**CLI Options:**
| Option | Description |
|--------|-------------|
| `-v, --voice` | Select voice (zvonko (default), stojan, mirsad, josip, vlado, detence, baba, djedo, or a singing preset; old ids accepted) |
| `-L, --language` | Language hr, sr, bs or sl; any voice speaks any language, without it a voice speaks its native language |
| `-r, --speech-rate` | Speech rate 0.5-2.0 (default: 1.0); formant voices 0.25-4.0 |
| `-p, --speech-pitch` | Speech pitch 0.5-2.0 (default: 1.0); formant voices 0.25-4.0 |
| `-I, --inflection` | Inflection level 0-100 (default: 50; 0 is a monotone) |
| `-a, --acceleration` | Rate multiplier 0.5-3.0 (default: 1.0) |
| `--head-size` | Voice quality: vocal tract length 0-100 (default: 50 = the voice as it is; 0 40 % longer, formants x0.71, a deeper voice; 100 30 % shorter, x1.4, a higher voice) |
| `--breathiness` | Breath 0-100: 0 a tense voice, 100 a whisper (default: 50; formant voices) |
| `--roughness` | Roughness 0-100: 0 perfectly steady, above 50 a rasp (default: 50; formant voices) |
| `--brightness` | Tone 0-100 (default: 50; 0 dull, with softer s and š, 100 sharp) |
| `-V, --speech-volume` | Volume 0.0-1.0 (default: 1.0) |
| `-B, --volume-boost` | Louder speech through a peak limiter, for noisy places |
| `-d, --numbers-digits` | Speak numbers as digits |
| `-s, --spell` | Spell the text character by character |
| `-S, --spelling-speed` | Spelling cadence 0-100; lower adds longer pauses (default: 50) |
| `-m, --spelling-mode` | `names` (be, ce, de; default) or `sounds` (b, c, d) |
| `-c, --comma-pauses` | Comma pause duration in ms |
| `-e, --period-pauses` | Period pause duration in ms |
| `-o, --output-file` | Output to WAV file |
| `-i, --input-file` | Read text from file |
| `-D, --data-dir` | Voice data directory |
| `-l, --list-voices` | List available voices |
| `-w, --verbose` | Enable verbose output |
| `-h, --help` | Show help message |

## The CLI on Linux

### Command-Line Interface Usage

```bash
# Basic usage
laprdus "Dobar dan!"

# Select voice (speaks its native language, Serbian for Vlado)
laprdus -v vlado "Zdravo svete!"

# Select the language separately: any voice speaks any language
laprdus -v vlado -L hr "Dobar dan svima!"
laprdus -L bs "Dobar dan!"

# Adjust parameters
laprdus -r 1.5 -p 1.2 -V 0.8 "Brzo i visoko"

# Output to WAV file
laprdus -o output.wav "Text to save"

# Read from file
laprdus -i input.txt -o speech.wav

# Pipe from stdin
echo "Tekst" | laprdus

# List available voices
laprdus -l
```

#### CLI Options

| Option | Long Form | Description |
|--------|-----------|-------------|
| `-v` | `--voice` | Select voice (zvonko (default), stojan, mirsad, josip, vlado, detence, baba, djedo, or a singing preset; old ids accepted) |
| `-L` | `--language` | Language hr, sr, bs or sl; any voice speaks any language, without it a voice speaks its native language |
| `-r` | `--speech-rate` | Speech rate (0.5-2.0, default 1.0; formant voices 0.25-4.0) |
| `-p` | `--speech-pitch` | Speech pitch (0.5-2.0, default 1.0; formant voices 0.25-4.0) |
| `-I` | `--inflection` | Inflection level (0-100, default 50) |
| `-a` | `--acceleration` | Rate multiplier (0.5-3.0, default 1.0) |
| | `--head-size` | Voice quality: vocal tract length 0-100 (default 50 = the voice as it is) |
| | `--breathiness` | Breath 0-100: 0 a tense voice, 100 a whisper (default 50; formant voices) |
| | `--roughness` | Roughness 0-100: 0 perfectly steady, above 50 a rasp (default 50; formant voices) |
| | `--brightness` | Tone 0-100 (default 50; 0 dull, with softer s and š, 100 sharp) |
| `-V` | `--speech-volume` | Volume (0.0-1.0, default 1.0) |
| `-B` | `--volume-boost` | Louder speech through a peak limiter, for noisy places |
| `-d` | `--numbers-digits` | Speak numbers as digits |
| `-s` | `--spell` | Spell the text character by character |
| `-S` | `--spelling-speed` | Spelling cadence 0-100; lower adds longer pauses (default: 50) |
| `-m` | `--spelling-mode` | `names` (default) or `sounds` |
| `-c` | `--comma-pauses` | Comma pause duration in ms |
| `-e` | `--period-pauses` | Period pause duration in ms |
| `-x` | `--exclamationmark-pauses` | Exclamation pause duration in ms |
| `-q` | `--questionmark-pauses` | Question mark pause duration in ms |
| `-n` | `--newline-pauses` | Newline pause duration in ms |
| `-o` | `--output-file` | Output to WAV file |
| `-i` | `--input-file` | Read text from file |
| `-h` | `--help` | Show help |
