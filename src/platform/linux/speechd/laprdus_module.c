/*
 * laprdus_module.c - Speech Dispatcher output module for LaprdusTTS
 *
 * Copyright (C) 2025 Hrvoje Katic
 *
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the BSD-2-Clause license. This allows integration with Speech
 * Dispatcher without GPL licensing concerns.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 */

/*
 * LaprdusTTS Speech Dispatcher Module
 *
 * This module provides Croatian/Serbian text-to-speech synthesis through
 * Speech Dispatcher, enabling TTS functionality in applications like Orca,
 * Emacspeak, and other SSIP clients.
 *
 * Features:
 * - 5 Croatian/Serbian voices (Josip, Vlado, Detence, Baba, Djedo)
 * - Full SSIP parameter support (rate, pitch, volume)
 * - Spelling mode support
 * - Punctuation mode support (via pauses)
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>  /* For strcasecmp */
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <dlfcn.h>

/* Speech Dispatcher module interface */
#include "spd_module_main.h"

/* LaprdusTTS C API */
#include <laprdus/laprdus_api.h>

/* Module identification */
#define MODULE_NAME "laprdus"
#define MODULE_VERSION "1.0.0"

/* Default paths */
#define DEFAULT_DATA_DIR "/usr/share/laprdus"
#define DEFAULT_VOICE "josip"

/* Configuration options */
static char *laprdus_data_dir = NULL;
static char *laprdus_default_voice = NULL;
static int laprdus_debug = 0;

/* Runtime state */
static LaprdusHandle engine = NULL;
static volatile int stop_requested = 0;
static volatile int speaking = 0;

/* Current voice parameters */
static char *current_voice_name = NULL;
static int current_rate = 0;      /* -100 to +100, default 0 */
static int current_pitch = 0;     /* -100 to +100, default 0 */
static int current_volume = 0;    /* -100 to +100, default 0 */
static int spelling_mode = 0;
static int punctuation_mode = 0;  /* 0=none, 1=some, 2=most, 3=all */

/* Voice list (static, built at init time) */
static SPDVoice **voice_list = NULL;

/* Logging helper */
#define DBG(fmt, ...) do { \
    if (laprdus_debug) { \
        fprintf(stderr, "[laprdus] " fmt "\n", ##__VA_ARGS__); \
    } \
} while(0)

#define ERR(fmt, ...) fprintf(stderr, "[laprdus ERROR] " fmt "\n", ##__VA_ARGS__)

/* --------------------------------------------------------------------------
 * Helper Functions
 * -------------------------------------------------------------------------- */

/**
 * Map Speech Dispatcher rate (-100 to +100) to Laprdus speed (0.5 to 2.0)
 */
static float map_rate_to_speed(int rate)
{
    /* rate -100 -> speed 0.5 (slow)
     * rate    0 -> speed 1.0 (normal)
     * rate +100 -> speed 2.0 (fast)
     */
    if (rate <= 0) {
        /* -100 to 0 maps to 0.5 to 1.0 */
        return 1.0f + (rate / 200.0f);
    } else {
        /* 0 to 100 maps to 1.0 to 2.0 */
        return 1.0f + (rate / 100.0f);
    }
}

/**
 * Map Speech Dispatcher pitch (-100 to +100) to Laprdus user_pitch (0.5 to 2.0)
 */
static float map_pitch_to_user_pitch(int pitch)
{
    /* pitch -100 -> user_pitch 0.5 (low)
     * pitch    0 -> user_pitch 1.0 (normal)
     * pitch +100 -> user_pitch 2.0 (high)
     */
    if (pitch <= 0) {
        return 1.0f + (pitch / 200.0f);
    } else {
        return 1.0f + (pitch / 100.0f);
    }
}

/**
 * Map Speech Dispatcher volume (-100 to +100) to Laprdus volume (0.0 to 1.0)
 */
static float map_volume(int volume)
{
    /* volume -100 -> 0.0 (silent)
     * volume    0 -> 0.5 (half)
     * volume +100 -> 1.0 (full)
     */
    return (volume + 100) / 200.0f;
}

/**
 * Apply current parameters to the engine
 */
static void apply_parameters(void)
{
    if (!engine) return;

    float speed = map_rate_to_speed(current_rate);
    float user_pitch = map_pitch_to_user_pitch(current_pitch);
    float vol = map_volume(current_volume);

    laprdus_set_speed(engine, speed);
    laprdus_set_user_pitch(engine, user_pitch);
    laprdus_set_volume(engine, vol);

    DBG("Applied params: rate=%d->speed=%.2f, pitch=%d->user_pitch=%.2f, volume=%d->vol=%.2f",
        current_rate, speed, current_pitch, user_pitch, current_volume, vol);
}

/**
 * Build the voice list from Laprdus API
 */
static void build_voice_list(void)
{
    uint32_t count = laprdus_get_voice_count();
    if (count == 0) {
        voice_list = malloc(sizeof(SPDVoice*));
        voice_list[0] = NULL;
        return;
    }

    voice_list = malloc((count + 1) * sizeof(SPDVoice*));
    if (!voice_list) {
        ERR("Failed to allocate voice list");
        return;
    }

    for (uint32_t i = 0; i < count; i++) {
        LaprdusVoiceInfo info;
        if (laprdus_get_voice_info(i, &info) == LAPRDUS_OK) {
            voice_list[i] = malloc(sizeof(SPDVoice));
            if (voice_list[i]) {
                voice_list[i]->name = strdup(info.id);
                voice_list[i]->language = strdup(info.language_code);
                /* Variant describes the voice type */
                if (info.base_voice_id) {
                    /* Derived voice */
                    char variant[64];
                    snprintf(variant, sizeof(variant), "%s-%s",
                             info.age ? info.age : "adult",
                             info.gender ? info.gender : "unknown");
                    voice_list[i]->variant = strdup(variant);
                } else {
                    voice_list[i]->variant = strdup(info.gender ? info.gender : "male");
                }
                DBG("Registered voice: %s (%s) [%s]",
                    voice_list[i]->name, voice_list[i]->language,
                    voice_list[i]->variant);
            }
        } else {
            voice_list[i] = NULL;
        }
    }
    voice_list[count] = NULL;
}

/**
 * Free the voice list
 */
static void free_voice_list(void)
{
    if (!voice_list) return;

    for (int i = 0; voice_list[i] != NULL; i++) {
        free((char*)voice_list[i]->name);
        free((char*)voice_list[i]->language);
        free((char*)voice_list[i]->variant);
        free(voice_list[i]);
    }
    free(voice_list);
    voice_list = NULL;
}

/**
 * Set voice by name
 */
static int set_voice(const char *name)
{
    if (!engine || !name) return -1;

    /* Check if voice exists */
    LaprdusVoiceInfo info;
    if (laprdus_get_voice_info_by_id(name, &info) != LAPRDUS_OK) {
        ERR("Voice not found: %s", name);
        return -1;
    }

    /* Set the voice */
    const char *data_dir = laprdus_data_dir ? laprdus_data_dir : DEFAULT_DATA_DIR;
    if (laprdus_set_voice(engine, name, data_dir) != LAPRDUS_OK) {
        ERR("Failed to set voice: %s (%s)", name, laprdus_get_error_message(engine));
        return -1;
    }

    /* Remember current voice */
    if (current_voice_name) free(current_voice_name);
    current_voice_name = strdup(name);

    DBG("Set voice: %s", name);
    return 0;
}

/* --------------------------------------------------------------------------
 * Speech Dispatcher Module Interface Implementation
 * -------------------------------------------------------------------------- */

/**
 * Called at startup to parse configuration file
 */
int module_config(const char *configfile)
{
    DBG("Configuring from: %s", configfile ? configfile : "(null)");

    /* TODO: Parse dotconf configuration file for module options:
     * - LaprdusDataDir: path to voice data files
     * - LaprdusDefaultVoice: default voice name
     * - LaprdusDebug: enable debug output
     */

    /* For now, use defaults */
    if (!laprdus_data_dir) {
        laprdus_data_dir = strdup(DEFAULT_DATA_DIR);
    }
    if (!laprdus_default_voice) {
        laprdus_default_voice = strdup(DEFAULT_VOICE);
    }

    return 0;
}

/**
 * Called after server sends INIT
 */
int module_init(char **msg)
{
    DBG("Initializing Laprdus TTS module");

    /* Request audio output through server */
    module_audio_set_server();

    /* Create engine instance */
    engine = laprdus_create();
    if (!engine) {
        *msg = strdup("Failed to create Laprdus TTS engine");
        ERR("%s", *msg);
        return -1;
    }

    /* Set default voice (which initializes phoneme data) */
    const char *data_dir = laprdus_data_dir ? laprdus_data_dir : DEFAULT_DATA_DIR;
    const char *voice = laprdus_default_voice ? laprdus_default_voice : DEFAULT_VOICE;

    if (laprdus_set_voice(engine, voice, data_dir) != LAPRDUS_OK) {
        *msg = strdup("Failed to initialize voice. Check data directory.");
        ERR("%s: %s", *msg, laprdus_get_error_message(engine));
        laprdus_destroy(engine);
        engine = NULL;
        return -1;
    }

    current_voice_name = strdup(voice);

    /* Load dictionaries if present */
    char dict_path[512];
    snprintf(dict_path, sizeof(dict_path), "%s/internal.json", data_dir);
    laprdus_load_dictionary(engine, dict_path);

    snprintf(dict_path, sizeof(dict_path), "%s/spelling.json", data_dir);
    laprdus_load_spelling_dictionary(engine, dict_path);

    /* Load user configuration from ~/.config/Laprdus */
    laprdus_load_user_config(engine);

    /* Build voice list for module_list_voices */
    build_voice_list();

    *msg = strdup("Laprdus TTS initialized successfully");
    DBG("%s", *msg);
    return 0;
}

/**
 * List available voices
 */
SPDVoice **module_list_voices(void)
{
    return voice_list;
}

/**
 * Set a parameter
 */
int module_set(const char *var, const char *val)
{
    DBG("SET %s = %s", var, val);

    if (!var || !val) return -1;

    if (strcmp(var, "voice") == 0) {
        /* Voice type (MALE1, FEMALE1, etc.) - map to our voices */
        if (strcasecmp(val, "male1") == 0) {
            return set_voice("josip");
        } else if (strcasecmp(val, "male2") == 0) {
            return set_voice("vlado");
        } else if (strcasecmp(val, "male3") == 0) {
            return set_voice("djedo");
        } else if (strcasecmp(val, "female1") == 0 || strcasecmp(val, "female3") == 0) {
            return set_voice("baba");
        } else if (strcasecmp(val, "child_male") == 0 || strcasecmp(val, "child_female") == 0) {
            return set_voice("detence");
        }
        /* Try as direct voice name */
        return set_voice(val);
    }
    else if (strcmp(var, "synthesis_voice") == 0) {
        /* Synthesis voice - NULL means keep current voice */
        if (strcmp(val, "NULL") == 0 || strlen(val) == 0) {
            return 0;  /* Keep current voice */
        }
        return set_voice(val);
    }
    else if (strcmp(var, "language") == 0) {
        /* Set voice by language code */
        /* Find first voice matching language */
        uint32_t count = laprdus_get_voice_count();
        for (uint32_t i = 0; i < count; i++) {
            LaprdusVoiceInfo info;
            if (laprdus_get_voice_info(i, &info) == LAPRDUS_OK) {
                if (strstr(info.language_code, val)) {
                    return set_voice(info.id);
                }
            }
        }
        /* No voice found for language, continue with current */
        return 0;
    }
    else if (strcmp(var, "rate") == 0) {
        current_rate = atoi(val);
        if (current_rate < -100) current_rate = -100;
        if (current_rate > 100) current_rate = 100;
        apply_parameters();
        return 0;
    }
    else if (strcmp(var, "pitch") == 0) {
        current_pitch = atoi(val);
        if (current_pitch < -100) current_pitch = -100;
        if (current_pitch > 100) current_pitch = 100;
        apply_parameters();
        return 0;
    }
    else if (strcmp(var, "pitch_range") == 0) {
        /* Pitch range affects inflection intensity */
        /* For now we don't have a direct mapping */
        return 0;
    }
    else if (strcmp(var, "volume") == 0) {
        current_volume = atoi(val);
        if (current_volume < -100) current_volume = -100;
        if (current_volume > 100) current_volume = 100;
        apply_parameters();
        return 0;
    }
    else if (strcmp(var, "punctuation_mode") == 0) {
        /* Map punctuation mode to pause settings */
        if (strcmp(val, "none") == 0) {
            punctuation_mode = 0;
        } else if (strcmp(val, "some") == 0) {
            punctuation_mode = 1;
        } else if (strcmp(val, "most") == 0) {
            punctuation_mode = 2;
        } else if (strcmp(val, "all") == 0) {
            punctuation_mode = 3;
        }
        /* TODO: Adjust pause settings based on punctuation mode */
        return 0;
    }
    else if (strcmp(var, "spelling_mode") == 0) {
        spelling_mode = (strcmp(val, "on") == 0) ? 1 : 0;
        DBG("Spelling mode: %d", spelling_mode);
        return 0;
    }
    else if (strcmp(var, "cap_let_recogn") == 0) {
        /* Capital letter recognition */
        /* We don't have specific capital letter sounds, but could add pitch */
        return 0;
    }
    else if (strcmp(var, "voice_type") == 0) {
        /* Voice type (MALE, FEMALE, CHILD, etc.) - we handle this via language/voice */
        return 0;
    }

    /* Accept unknown parameters gracefully */
    DBG("Ignoring unknown parameter: %s = %s", var, val);
    return 0;
}

/**
 * Set audio parameter
 */
int module_audio_set(const char *var, const char *val)
{
    DBG("AUDIO_SET %s = %s", var, val);

    /* We use module_audio_set_server(), so most audio params are handled by server */
    /* Accept all audio parameters */
    return 0;
}

/**
 * Initialize audio
 */
int module_audio_init(char **status)
{
    /* Audio is handled by the server */
    *status = NULL;
    return 0;
}

/**
 * Set log level
 */
int module_loglevel_set(const char *var, const char *val)
{
    if (strcmp(var, "log_level") == 0) {
        int level = atoi(val);
        laprdus_debug = (level >= 3);
    }
    return 0;
}

/**
 * Enable/disable debug output
 */
int module_debug(int enable, const char *file)
{
    laprdus_debug = enable;
    return 0;
}

/**
 * Main loop - let module_process handle SSIP protocol
 */
int module_loop(void)
{
    DBG("Entering main loop");

    int ret = module_process(STDIN_FILENO, 1);

    if (ret != 0) {
        ERR("Broken pipe, exiting...");
    }

    return ret;
}

/**
 * Synchronous speak function
 */
void module_speak_sync(const char *data, size_t bytes, SPDMessageType msgtype)
{
    DBG("SPEAK_SYNC: type=%d, len=%zu, text='%.50s%s'",
        msgtype, bytes, data, bytes > 50 ? "..." : "");

    if (!engine) {
        module_speak_error();
        return;
    }

    /* Quick validation */
    if (!data || bytes == 0) {
        module_speak_error();
        return;
    }

    stop_requested = 0;
    speaking = 1;

    /* Confirm we're ready to speak */
    module_speak_ok();

    /* Apply current parameters */
    apply_parameters();

    /* Handle different message types */
    int16_t *samples = NULL;
    LaprdusAudioFormat format;
    int32_t num_samples = 0;

    switch (msgtype) {
        case SPD_MSGTYPE_SPELL:
        case SPD_MSGTYPE_CHAR:
        case SPD_MSGTYPE_KEY:
            /* Use spelling mode for character/key announcements */
            num_samples = laprdus_synthesize_spelled(engine, data, &samples, &format);
            break;

        case SPD_MSGTYPE_TEXT:
        case SPD_MSGTYPE_SOUND_ICON:
        default:
            /* Normal text synthesis */
            if (spelling_mode) {
                num_samples = laprdus_synthesize_spelled(engine, data, &samples, &format);
            } else {
                num_samples = laprdus_synthesize(engine, data, &samples, &format);
            }
            break;
    }

    if (num_samples <= 0 || !samples) {
        ERR("Synthesis failed: %s", laprdus_get_error_message(engine));
        speaking = 0;
        module_report_event_stop();
        return;
    }

    /* Report begin */
    module_report_event_begin();

    /* Check for stop request before sending audio */
    module_process(STDIN_FILENO, 0);
    if (stop_requested) {
        DBG("Stop requested before audio output");
        laprdus_free_buffer(samples);
        speaking = 0;
        module_report_event_stop();
        return;
    }

    /* Create audio track */
    AudioTrack track;
    track.bits = format.bits_per_sample;
    track.num_channels = format.channels;
    track.sample_rate = format.sample_rate;
    track.num_samples = num_samples;
    track.samples = samples;

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    AudioFormat audio_format = SPD_AUDIO_BE;
#else
    AudioFormat audio_format = SPD_AUDIO_LE;
#endif

    /* Send audio to server */
    DBG("Sending %d samples to server (rate=%d, bits=%d, ch=%d)",
        num_samples, format.sample_rate, format.bits_per_sample, format.channels);
    module_tts_output_server(&track, audio_format);

    /* Free audio buffer */
    laprdus_free_buffer(samples);

    /* Check for stop again */
    module_process(STDIN_FILENO, 0);
    if (stop_requested) {
        DBG("Stop requested after audio output");
        speaking = 0;
        module_report_event_stop();
        return;
    }

    /* Report end */
    speaking = 0;
    module_report_event_end();

    DBG("SPEAK_SYNC complete");
}

/**
 * Pause speech (not fully supported - we just stop)
 */
size_t module_pause(void)
{
    DBG("PAUSE requested");

    if (speaking) {
        stop_requested = 1;
        laprdus_cancel(engine);
        module_report_event_pause();
    }

    return 0;
}

/**
 * Stop speech
 */
int module_stop(void)
{
    DBG("STOP requested");

    if (speaking) {
        stop_requested = 1;
        laprdus_cancel(engine);
    }

    return 0;
}

/**
 * Close module and clean up
 */
int module_close(void)
{
    DBG("Closing module");

    /* Stop any ongoing synthesis */
    if (speaking) {
        stop_requested = 1;
        laprdus_cancel(engine);
    }

    /* Free voice list */
    free_voice_list();

    /* Destroy engine */
    if (engine) {
        laprdus_destroy(engine);
        engine = NULL;
    }

    /* Free configuration */
    if (laprdus_data_dir) {
        free(laprdus_data_dir);
        laprdus_data_dir = NULL;
    }
    if (laprdus_default_voice) {
        free(laprdus_default_voice);
        laprdus_default_voice = NULL;
    }
    if (current_voice_name) {
        free(current_voice_name);
        current_voice_name = NULL;
    }

    DBG("Module closed");
    return 0;
}
