/*
 * laprdus_cli_windows.cpp - Windows command-line interface for LaprdusTTS
 *
 * Copyright (C) 2025 Hrvoje Katic
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * Usage:
 *   laprdus "Text to speak"
 *   laprdus -v josip -r 1.5 "Dobar dan!"
 *   laprdus -i input.txt -o output.wav
 *   echo "Hello" | laprdus
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <cstring>

/* Windows headers */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#include <io.h>
#include <fcntl.h>

/* Bundled getopt for Windows */
#include "getopt.h"

/* LaprdusTTS C API */
#include <laprdus/laprdus_api.h>

/* User configuration */
#include "core/user_config.hpp"

/* Link winmm library for audio playback */
#pragma comment(lib, "winmm.lib")

/* Version information */
#define CLI_VERSION "1.0.0"

/* Command-line options */
struct Options {
    std::string text;
    std::string voice = "josip";
    float speech_rate = 1.0f;
    float speech_pitch = 1.0f;
    float speech_volume = 1.0f;
    bool numbers_as_digits = false;
    uint32_t comma_pause = 40;
    uint32_t period_pause = 80;
    uint32_t exclamation_pause = 70;
    uint32_t question_pause = 60;
    uint32_t newline_pause = 100;
    std::string output_file;
    std::string input_file;
    std::string data_dir;
    bool show_help = false;
    bool show_version = false;
    bool list_voices = false;
    bool verbose = false;
};

/* Short options */
static const char *short_options = "v:r:p:V:dc:e:x:q:n:o:i:D:hlLw";

/* Long options */
static struct option long_options[] = {
    {"voice",               required_argument, nullptr, 'v'},
    {"speech-rate",         required_argument, nullptr, 'r'},
    {"speech-pitch",        required_argument, nullptr, 'p'},
    {"speech-volume",       required_argument, nullptr, 'V'},
    {"numbers-digits",      no_argument,       nullptr, 'd'},
    {"comma-pauses",        required_argument, nullptr, 'c'},
    {"period-pauses",       required_argument, nullptr, 'e'},
    {"exclamationmark-pauses", required_argument, nullptr, 'x'},
    {"questionmark-pauses", required_argument, nullptr, 'q'},
    {"newline-pauses",      required_argument, nullptr, 'n'},
    {"output-file",         required_argument, nullptr, 'o'},
    {"input-file",          required_argument, nullptr, 'i'},
    {"data-dir",            required_argument, nullptr, 'D'},
    {"help",                no_argument,       nullptr, 'h'},
    {"list-voices",         no_argument,       nullptr, 'l'},
    {"list",                no_argument,       nullptr, 'L'},
    {"verbose",             no_argument,       nullptr, 'w'},
    {nullptr,               0,                 nullptr, 0}
};

/**
 * Get the directory where the executable is located
 */
std::string get_exe_directory()
{
    char path[MAX_PATH];
    if (GetModuleFileNameA(nullptr, path, MAX_PATH) > 0) {
        std::string exe_path(path);
        size_t pos = exe_path.find_last_of("\\/");
        if (pos != std::string::npos) {
            return exe_path.substr(0, pos);
        }
    }
    return ".";
}

/**
 * Print help message
 */
void print_help(const char *program_name)
{
    std::string default_data_dir = get_exe_directory();

    std::cout << "LaprdusTTS - Croatian/Serbian Text-to-Speech Engine\n"
              << "Version " << CLI_VERSION << "\n\n"
              << "Usage:\n"
              << "  " << program_name << " [OPTIONS] \"Text to speak\"\n"
              << "  " << program_name << " [OPTIONS] -i input.txt\n"
              << "  echo \"Text\" | " << program_name << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  -v, --voice NAME           Select voice (default: josip)\n"
              << "                             Available: josip, vlado, detence, baba, djed\n"
              << "  -r, --speech-rate RATE     Speech rate 0.5-2.0 (default: 1.0)\n"
              << "  -p, --speech-pitch PITCH   Speech pitch 0.5-2.0 (default: 1.0)\n"
              << "  -V, --speech-volume VOL    Volume 0.0-1.0 (default: 1.0)\n"
              << "  -d, --numbers-digits       Speak numbers as digits (jedan-dva-tri)\n"
              << "  -c, --comma-pauses MS      Pause duration for commas (default: 40)\n"
              << "  -e, --period-pauses MS     Pause duration for periods (default: 80)\n"
              << "  -x, --exclamationmark-pauses MS\n"
              << "                             Pause duration for exclamation marks (default: 70)\n"
              << "  -q, --questionmark-pauses MS\n"
              << "                             Pause duration for question marks (default: 60)\n"
              << "  -n, --newline-pauses MS    Pause duration for newlines (default: 100)\n"
              << "  -o, --output-file FILE     Output to WAV file instead of speakers\n"
              << "  -i, --input-file FILE      Read text from file (- for stdin)\n"
              << "  -D, --data-dir DIR         Voice data directory (default: " << default_data_dir << ")\n"
              << "  -l, --list-voices          List available voices\n"
              << "  -w, --verbose              Enable verbose output\n"
              << "  -h, --help                 Show this help message\n\n"
              << "Examples:\n"
              << "  " << program_name << " \"Dobar dan!\"\n"
              << "  " << program_name << " -v vlado -r 1.5 \"Zdravo svete!\"\n"
              << "  " << program_name << " -i document.txt -o speech.wav\n"
              << "  echo \"Jedan, dva, tri\" | " << program_name << "\n\n"
              << "Voices:\n"
              << "  josip   - Croatian male adult (default)\n"
              << "  vlado   - Serbian male adult\n"
              << "  detence - Croatian child\n"
              << "  baba    - Croatian female senior\n"
              << "  djed    - Serbian male senior\n";
}

/**
 * List available voices
 */
void list_voices()
{
    std::cout << "Available voices:\n\n";
    std::cout << "ID        Language    Gender    Age       Description\n";
    std::cout << "--------  ----------  --------  --------  -------------------------\n";

    uint32_t count = laprdus_get_voice_count();
    for (uint32_t i = 0; i < count; i++) {
        LaprdusVoiceInfo info;
        if (laprdus_get_voice_info(i, &info) == LAPRDUS_OK) {
            printf("%-8s  %-10s  %-8s  %-8s  %s\n",
                   info.id,
                   info.language_code,
                   info.gender ? info.gender : "-",
                   info.age ? info.age : "-",
                   info.display_name);
        }
    }
}

/**
 * Parse command-line arguments
 */
bool parse_args(int argc, char *argv[], Options &opts)
{
    int opt;
    int option_index = 0;

    /* Reset getopt */
    getopt_reset();
    optind = 1;

    while ((opt = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
        switch (opt) {
            case 'v':
                opts.voice = optarg;
                break;
            case 'r':
                opts.speech_rate = std::stof(optarg);
                if (opts.speech_rate < 0.5f) opts.speech_rate = 0.5f;
                if (opts.speech_rate > 2.0f) opts.speech_rate = 2.0f;
                break;
            case 'p':
                opts.speech_pitch = std::stof(optarg);
                if (opts.speech_pitch < 0.5f) opts.speech_pitch = 0.5f;
                if (opts.speech_pitch > 2.0f) opts.speech_pitch = 2.0f;
                break;
            case 'V':
                opts.speech_volume = std::stof(optarg);
                if (opts.speech_volume < 0.0f) opts.speech_volume = 0.0f;
                if (opts.speech_volume > 1.0f) opts.speech_volume = 1.0f;
                break;
            case 'd':
                opts.numbers_as_digits = true;
                break;
            case 'c':
                opts.comma_pause = static_cast<uint32_t>(std::stoul(optarg));
                break;
            case 'e':
                opts.period_pause = static_cast<uint32_t>(std::stoul(optarg));
                break;
            case 'x':
                opts.exclamation_pause = static_cast<uint32_t>(std::stoul(optarg));
                break;
            case 'q':
                opts.question_pause = static_cast<uint32_t>(std::stoul(optarg));
                break;
            case 'n':
                opts.newline_pause = static_cast<uint32_t>(std::stoul(optarg));
                break;
            case 'o':
                opts.output_file = optarg;
                break;
            case 'i':
                opts.input_file = optarg;
                break;
            case 'D':
                opts.data_dir = optarg;
                break;
            case 'h':
                opts.show_help = true;
                return true;
            case 'l':
            case 'L':
                opts.list_voices = true;
                return true;
            case 'w':
                opts.verbose = true;
                break;
            case '?':
            default:
                return false;
        }
    }

    /* Get text from remaining arguments */
    if (optind < argc) {
        opts.text = argv[optind];
        /* Concatenate remaining arguments as text */
        for (int i = optind + 1; i < argc; i++) {
            opts.text += " ";
            opts.text += argv[i];
        }
    }

    return true;
}

/**
 * Read text from file or stdin
 */
std::string read_text_from_file(const std::string &filename)
{
    std::stringstream buffer;

    if (filename == "-") {
        /* Read from stdin */
        buffer << std::cin.rdbuf();
    } else {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file: " << filename << "\n";
            return "";
        }
        buffer << file.rdbuf();
    }

    return buffer.str();
}

/**
 * Write WAV file header
 */
void write_wav_header(std::ofstream &file, uint32_t sample_rate, uint16_t bits_per_sample,
                      uint16_t channels, uint32_t data_size)
{
    uint32_t byte_rate = sample_rate * channels * bits_per_sample / 8;
    uint16_t block_align = channels * bits_per_sample / 8;
    uint32_t chunk_size = 36 + data_size;

    /* RIFF header */
    file.write("RIFF", 4);
    file.write(reinterpret_cast<const char*>(&chunk_size), 4);
    file.write("WAVE", 4);

    /* fmt subchunk */
    file.write("fmt ", 4);
    uint32_t subchunk1_size = 16;
    file.write(reinterpret_cast<const char*>(&subchunk1_size), 4);
    uint16_t audio_format = 1; /* PCM */
    file.write(reinterpret_cast<const char*>(&audio_format), 2);
    file.write(reinterpret_cast<const char*>(&channels), 2);
    file.write(reinterpret_cast<const char*>(&sample_rate), 4);
    file.write(reinterpret_cast<const char*>(&byte_rate), 4);
    file.write(reinterpret_cast<const char*>(&block_align), 2);
    file.write(reinterpret_cast<const char*>(&bits_per_sample), 2);

    /* data subchunk */
    file.write("data", 4);
    file.write(reinterpret_cast<const char*>(&data_size), 4);
}

/**
 * Write audio to WAV file
 */
bool write_wav_file(const std::string &filename, const int16_t *samples, int32_t num_samples,
                    const LaprdusAudioFormat &format)
{
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot create output file: " << filename << "\n";
        return false;
    }

    uint32_t data_size = num_samples * sizeof(int16_t);
    write_wav_header(file, format.sample_rate, format.bits_per_sample, format.channels, data_size);
    file.write(reinterpret_cast<const char*>(samples), data_size);
    file.close();

    return true;
}

/**
 * Play audio using Windows WinMM API
 */
bool play_audio_winmm(const int16_t *samples, int32_t num_samples, const LaprdusAudioFormat &format)
{
    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = format.channels;
    wfx.nSamplesPerSec = format.sample_rate;
    wfx.wBitsPerSample = format.bits_per_sample;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    HWAVEOUT hwo;
    MMRESULT result = waveOutOpen(&hwo, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "Error: Failed to open audio device (error " << result << ")\n";
        return false;
    }

    WAVEHDR whdr = {};
    whdr.lpData = reinterpret_cast<LPSTR>(const_cast<int16_t*>(samples));
    whdr.dwBufferLength = num_samples * sizeof(int16_t);

    result = waveOutPrepareHeader(hwo, &whdr, sizeof(whdr));
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "Error: Failed to prepare audio header (error " << result << ")\n";
        waveOutClose(hwo);
        return false;
    }

    result = waveOutWrite(hwo, &whdr, sizeof(whdr));
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "Error: Failed to write audio (error " << result << ")\n";
        waveOutUnprepareHeader(hwo, &whdr, sizeof(whdr));
        waveOutClose(hwo);
        return false;
    }

    /* Wait for playback to complete */
    while (!(whdr.dwFlags & WHDR_DONE)) {
        Sleep(10);
    }

    waveOutUnprepareHeader(hwo, &whdr, sizeof(whdr));
    waveOutClose(hwo);
    return true;
}

/**
 * Play audio (wrapper for Windows)
 */
bool play_audio(const int16_t *samples, int32_t num_samples, const LaprdusAudioFormat &format)
{
    return play_audio_winmm(samples, num_samples, format);
}

/**
 * Main entry point
 */
int main(int argc, char *argv[])
{
    /* Set console output to UTF-8 */
    SetConsoleOutputCP(CP_UTF8);

    Options opts;

    /* Set default data directory to voices subdirectory of executable location */
    opts.data_dir = get_exe_directory() + "\\voices";

    /* Load user configuration first (CLI args will override) */
    laprdus::UserConfig userConfig;
    bool hasUserConfig = userConfig.load_settings();
    if (hasUserConfig) {
        const laprdus::UserSettings& settings = userConfig.settings();
        /* Apply user settings as defaults (CLI args can override) */
        if (!settings.default_voice.empty()) {
            opts.voice = settings.default_voice;
        }
        opts.speech_rate = settings.speed;
        opts.speech_pitch = settings.user_pitch;
        opts.speech_volume = settings.volume;
        opts.numbers_as_digits = (settings.number_mode == laprdus::NumberMode::DigitByDigit);
        opts.comma_pause = settings.comma_pause_ms;
        opts.period_pause = settings.sentence_pause_ms;
        opts.newline_pause = settings.newline_pause_ms;
    }

    /* Parse command-line arguments (overrides user config) */
    if (!parse_args(argc, argv, opts)) {
        std::cerr << "Use -h for help.\n";
        return 1;
    }

    /* Handle special modes */
    if (opts.show_help) {
        print_help(argv[0]);
        return 0;
    }

    if (opts.list_voices) {
        list_voices();
        return 0;
    }

    /* Read text from input file if specified */
    if (!opts.input_file.empty()) {
        opts.text = read_text_from_file(opts.input_file);
        if (opts.text.empty()) {
            return 1;
        }
    }

    /* Read from stdin if no text provided and stdin is not a terminal */
    if (opts.text.empty() && !_isatty(_fileno(stdin))) {
        std::stringstream buffer;
        buffer << std::cin.rdbuf();
        opts.text = buffer.str();
    }

    /* Check for text */
    if (opts.text.empty()) {
        std::cerr << "Error: No text to speak. Provide text as argument, use -i, or pipe to stdin.\n";
        std::cerr << "Use -h for help.\n";
        return 1;
    }

    if (opts.verbose) {
        std::cout << "Voice: " << opts.voice << "\n";
        std::cout << "Rate: " << opts.speech_rate << "\n";
        std::cout << "Pitch: " << opts.speech_pitch << "\n";
        std::cout << "Volume: " << opts.speech_volume << "\n";
        std::cout << "Text length: " << opts.text.length() << " characters\n";
    }

    /* Create TTS engine */
    LaprdusHandle engine = laprdus_create();
    if (!engine) {
        std::cerr << "Error: Failed to create TTS engine\n";
        return 1;
    }

    /* Set voice (which loads phoneme data) */
    if (laprdus_set_voice(engine, opts.voice.c_str(), opts.data_dir.c_str()) != LAPRDUS_OK) {
        std::cerr << "Error: Failed to set voice '" << opts.voice << "': "
                  << laprdus_get_error_message(engine) << "\n";
        laprdus_destroy(engine);
        return 1;
    }

    /* Load dictionaries from exe directory (not voices subdirectory) */
    std::string exe_dir = get_exe_directory();
    std::string dict_path = exe_dir + "\\dictionaries\\dictionary.json";
    laprdus_load_dictionary(engine, dict_path.c_str());

    std::string spelling_path = exe_dir + "\\dictionaries\\spelling.json";
    laprdus_load_spelling_dictionary(engine, spelling_path.c_str());

    /* Load emoji dictionary */
    std::string emoji_path = exe_dir + "\\dictionaries\\emoji.json";
    laprdus_load_emoji_dictionary(engine, emoji_path.c_str());

    /* Load user dictionaries if they exist and are enabled (extend/override internal ones) */
    if (hasUserConfig) {
        const auto& settings = userConfig.settings();

        if (settings.user_dictionaries_enabled) {
            if (userConfig.user_dictionary_exists("user.json")) {
                std::string userDictPath = userConfig.get_user_dictionary_path();
                laprdus_append_dictionary(engine, userDictPath.c_str());
            }

            if (userConfig.user_dictionary_exists("spelling.json")) {
                std::string userSpellingPath = userConfig.get_user_spelling_dictionary_path();
                laprdus_append_spelling_dictionary(engine, userSpellingPath.c_str());
            }

            if (userConfig.user_dictionary_exists("emoji.json")) {
                std::string userEmojiPath = userConfig.get_user_emoji_dictionary_path();
                laprdus_append_emoji_dictionary(engine, userEmojiPath.c_str());
            }
        }

        /* Enable emoji if configured in user settings */
        if (settings.emoji_enabled) {
            laprdus_set_emoji_enabled(engine, 1);
        }
    }

    /* Configure parameters */
    laprdus_set_speed(engine, opts.speech_rate);
    laprdus_set_user_pitch(engine, opts.speech_pitch);
    laprdus_set_volume(engine, opts.speech_volume);

    /* Configure pause settings */
    laprdus_set_comma_pause(engine, opts.comma_pause);
    laprdus_set_sentence_pause(engine, opts.period_pause);  /* Period uses sentence pause */
    laprdus_set_newline_pause(engine, opts.newline_pause);
    /* Note: exclamation and question marks currently use sentence_pause */

    /* Set number mode */
    if (opts.numbers_as_digits) {
        laprdus_set_number_mode(engine, LAPRDUS_NUMBER_MODE_DIGIT);
    }

    /* Synthesize text */
    int16_t *samples = nullptr;
    LaprdusAudioFormat format;
    int32_t num_samples = laprdus_synthesize(engine, opts.text.c_str(), &samples, &format);

    if (num_samples <= 0 || !samples) {
        std::cerr << "Error: Synthesis failed: " << laprdus_get_error_message(engine) << "\n";
        laprdus_destroy(engine);
        return 1;
    }

    if (opts.verbose) {
        std::cout << "Synthesized " << num_samples << " samples "
                  << "(" << format.sample_rate << " Hz, "
                  << format.bits_per_sample << " bit, "
                  << format.channels << " ch)\n";
    }

    /* Output audio */
    bool success = false;
    if (!opts.output_file.empty()) {
        /* Write to file */
        success = write_wav_file(opts.output_file, samples, num_samples, format);
        if (success && opts.verbose) {
            std::cout << "Wrote " << opts.output_file << "\n";
        }
    } else {
        /* Play to audio device */
        success = play_audio(samples, num_samples, format);
    }

    /* Clean up */
    laprdus_free_buffer(samples);
    laprdus_destroy(engine);

    return success ? 0 : 1;
}
