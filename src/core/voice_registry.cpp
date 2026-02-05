// -*- coding: utf-8 -*-
// voice_registry.cpp - Voice definitions and registry implementation

#include "voice_registry.hpp"
#include <cstring>

namespace laprdus {

// =============================================================================
// Static Voice Definitions
// =============================================================================

static constexpr VoiceDefinition VOICES[] = {
    // Physical voice: Josip (Croatian, male, adult)
    {
        "josip",                            // id
        "Laprdus Josip (Croatian)",         // display_name
        VoiceLanguage::Croatian,            // language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        nullptr,                            // base_voice_id (physical voice)
        1.0f,                               // base_pitch
        "Josip.bin"                         // data_filename
    },
    // Physical voice: Vlado (Serbian, male, adult)
    {
        "vlado",                            // id
        "Laprdus Vlado (Serbian)",          // display_name
        VoiceLanguage::Serbian,             // language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        nullptr,                            // base_voice_id (physical voice)
        1.0f,                               // base_pitch
        "Vlado.bin"                         // data_filename
    },
    // Derived voice: Detence (child, based on Josip)
    {
        "detence",                          // id
        "Laprdus Detence (Croatian)",       // display_name
        VoiceLanguage::Croatian,            // language
        VoiceGender::Male,                  // gender
        VoiceAge::Child,                    // age
        "josip",                            // base_voice_id
        1.5f,                               // base_pitch (higher for child)
        nullptr                             // data_filename (uses base voice)
    },
    // Derived voice: Baba (grandma, based on Josip)
    {
        "baba",                             // id
        "Laprdus Baba (Croatian)",          // display_name
        VoiceLanguage::Croatian,            // language
        VoiceGender::Female,                // gender
        VoiceAge::Senior,                   // age
        "josip",                            // base_voice_id
        1.2f,                               // base_pitch (slightly higher)
        nullptr                             // data_filename (uses base voice)
    },
    // Derived voice: Djedo (grandpa, based on Vlado)
    {
        "djedo",                            // id
        "Laprdus Djedo (Serbian)",          // display_name
        VoiceLanguage::Serbian,             // language
        VoiceGender::Male,                  // gender
        VoiceAge::Senior,                   // age
        "vlado",                            // base_voice_id
        0.75f,                              // base_pitch (lower for grandpa)
        nullptr                             // data_filename (uses base voice)
    }
};

static_assert(sizeof(VOICES) / sizeof(VOICES[0]) == VOICE_COUNT,
              "VOICES array size must match VOICE_COUNT");

// =============================================================================
// VoiceRegistry Implementation
// =============================================================================

span<const VoiceDefinition> VoiceRegistry::all_voices() {
    return span<const VoiceDefinition>(VOICES, VOICE_COUNT);
}

size_t VoiceRegistry::voice_count() {
    return VOICE_COUNT;
}

const VoiceDefinition* VoiceRegistry::find_by_id(const char* id) {
    if (!id) {
        return nullptr;
    }

    for (const auto& voice : VOICES) {
        if (std::strcmp(voice.id, id) == 0) {
            return &voice;
        }
    }

    return nullptr;
}

const VoiceDefinition* VoiceRegistry::get_by_index(size_t index) {
    if (index >= VOICE_COUNT) {
        return nullptr;
    }
    return &VOICES[index];
}

const VoiceDefinition* VoiceRegistry::default_voice() {
    return &VOICES[0];  // Josip
}

const VoiceDefinition* VoiceRegistry::get_physical_voice(const VoiceDefinition* voice) {
    if (!voice) {
        return nullptr;
    }

    // If already physical, return it
    if (voice->base_voice_id == nullptr) {
        return voice;
    }

    // Find the base voice
    return find_by_id(voice->base_voice_id);
}

const char* VoiceRegistry::get_data_filename(const VoiceDefinition* voice) {
    if (!voice) {
        return nullptr;
    }

    // If physical voice, return its data filename
    if (voice->data_filename != nullptr) {
        return voice->data_filename;
    }

    // For derived voice, get the physical voice's filename
    const VoiceDefinition* physical = get_physical_voice(voice);
    if (physical) {
        return physical->data_filename;
    }

    return nullptr;
}

bool VoiceRegistry::is_physical_voice(const VoiceDefinition* voice) {
    if (!voice) {
        return false;
    }
    return voice->base_voice_id == nullptr;
}

} // namespace laprdus
