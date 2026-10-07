// -*- coding: utf-8 -*-
// voice_registry.cpp - Voice definitions and registry implementation

#include "voice_registry.hpp"
#include "../formant/formant_synthesizer.hpp"
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
    // Derived voice: Đedo (grandpa, based on Vlado)
    {
        "djed",                             // id
        "Laprdus Đedo (Serbian)",           // display_name
        VoiceLanguage::Serbian,             // language
        VoiceGender::Male,                  // gender
        VoiceAge::Senior,                   // age
        "vlado",                            // base_voice_id
        0.75f,                              // base_pitch (lower for grandpa)
        nullptr                             // data_filename (uses base voice)
    },
    // Formant voice: Zvonko (Croatian, male, adult)
    {
        "zvonko",                           // id
        "Laprdus Zvonko (Croatian)",        // display_name
        VoiceLanguage::Croatian,            // language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        nullptr,                            // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename (none: synthesized by rule)
        VoiceSynthesis::Formant             // synthesis
    },
    // Formant voice: Stojan (Serbian, male, adult)
    {
        "stojan",                           // id
        "Laprdus Stojan (Serbian)",         // display_name
        VoiceLanguage::Serbian,             // language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        nullptr,                            // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename (none: synthesized by rule)
        VoiceSynthesis::Formant             // synthesis
    },
    // Formant voice: Mirsad (Bosnian, male, adult)
    {
        "mirsad",                           // id
        "Laprdus Mirsad (Bosnian)",         // display_name
        VoiceLanguage::Bosnian,             // language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        nullptr,                            // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename (none: synthesized by rule)
        VoiceSynthesis::Formant             // synthesis
    },
    // Singing presets: a formant voice with an instrument for a larynx and
    // a folk song for a melody (src/formant/formant_synthesizer.cpp). The
    // base voice gives the language and the vocal tract.
    {
        "orguljas",                         // id
        "Laprdus Zvonko Orgulja\xc5\xa1 (Croatian)",   // display_name (Orguljaš: the organist)
        VoiceLanguage::Croatian,            // language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        "zvonko",                           // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename
        VoiceSynthesis::Formant             // synthesis
    },
    {
        "klapa",                            // id
        "Laprdus Klapa Zvonko (Croatian)",  // display_name (a one-man klapa)
        VoiceLanguage::Croatian,            // language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        "zvonko",                           // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename
        VoiceSynthesis::Formant             // synthesis
    },
    {
        "trubac",                           // id
        "Laprdus Stojan Truba\xc4\x8d (Serbian)",      // display_name (Trubač: the trumpeter)
        VoiceLanguage::Serbian,             // language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        "stojan",                           // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename
        VoiceSynthesis::Formant             // synthesis
    },
    {
        "harmonikas",                       // id
        "Laprdus Stojan Harmonika\xc5\xa1 (Serbian)",  // display_name (Harmonikaš: the accordionist)
        VoiceLanguage::Serbian,             // language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        "stojan",                           // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename
        VoiceSynthesis::Formant             // synthesis
    },
    {
        "sevdalija",                        // id
        "Laprdus Mirsad Sevdalija (Bosnian)",   // display_name (the sevdah singer)
        VoiceLanguage::Bosnian,             // language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        "mirsad",                           // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename
        VoiceSynthesis::Formant             // synthesis
    },
    {
        "sazlija",                          // id
        "Laprdus Mirsad Sazlija (Bosnian)", // display_name (the saz player)
        VoiceLanguage::Bosnian,             // language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        "mirsad",                           // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename
        VoiceSynthesis::Formant             // synthesis
    },
    // The speaking voices singing as themselves, at their own pitch.
    {
        "pjevac",                           // id
        "Laprdus Zvonko Pjeva\xc4\x8d (Croatian)",     // display_name (Pjevač: the singer)
        VoiceLanguage::Croatian,            // language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        "zvonko",                           // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename
        VoiceSynthesis::Formant             // synthesis
    },
    {
        "pevac",                            // id
        "Laprdus Stojan Peva\xc4\x8d (Serbian)",       // display_name (Pevač: the singer)
        VoiceLanguage::Serbian,             // language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        "stojan",                           // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename
        VoiceSynthesis::Formant             // synthesis
    },
    {
        "solist",                           // id
        "Laprdus Mirsad Solist (Bosnian)",  // display_name (the soloist)
        VoiceLanguage::Bosnian,             // language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        "mirsad",                           // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename
        VoiceSynthesis::Formant             // synthesis
    },
    {
        "becarac",                          // id
        "Laprdus Zvonko Be\xc4\x87" "arac (Croatian)",  // display_name (sings the bećarac)
        VoiceLanguage::Croatian,            // language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        "zvonko",                           // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename
        VoiceSynthesis::Formant             // synthesis
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

bool VoiceRegistry::is_formant_voice(const VoiceDefinition* voice) {
    return voice && voice->synthesis == VoiceSynthesis::Formant;
}

float VoiceRegistry::nominal_wpm(const char* id) {
    const VoiceDefinition* voice = find_by_id(id);
    if (!voice) {
        return 0.0f;
    }
    if (is_formant_voice(voice)) {
        const formant::FormantVoice* fv = formant::find_formant_voice(voice->id);
        return FORMANT_NOMINAL_WPM * (fv ? fv->tempo : 1.0f);
    }
    // Recorded voices, measured on the same text as the formant voices.
    const VoiceDefinition* physical = get_physical_voice(voice);
    if (physical && std::strcmp(physical->id, "vlado") == 0) {
        return 97.0f;
    }
    return 186.0f;
}

} // namespace laprdus
