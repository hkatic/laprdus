// -*- coding: utf-8 -*-
// voice_registry.cpp - Voice definitions and registry implementation

#include "voice_registry.hpp"
#include "../formant/formant_synthesizer.hpp"
#include <cstring>

namespace laprdus {

// =============================================================================
// Static Voice Definitions
// =============================================================================

// Every voice speaks every language; `language` below is the voice's native
// one, spoken when no other is asked for. Display names carry no language
// and no "Laprdus" prefix: the platforms add what they need ("Laprdus
// Croatian" is a SAPI5 token, not a voice). Ids are plain ASCII (dj for đ,
// c for č and ć, s for š, z for ž) so that no registry, settings file or
// locale object ever has to carry a diacritic. New voices are appended:
// code and saved settings rely on the order of the first eight.
static constexpr VoiceDefinition VOICES[] = {
    // Recorded voice: Josip (Croatian, male, adult)
    {
        "josip",                            // id
        "Josip",                            // display_name
        VoiceLanguage::Croatian,            // native language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        nullptr,                            // base_voice_id (physical voice)
        1.0f,                               // base_pitch
        "Josip.bin"                         // data_filename
    },
    // Recorded voice: Vlado (Serbian, male, adult)
    {
        "vlado",                            // id
        "Vlado",                            // display_name
        VoiceLanguage::Serbian,             // native language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        nullptr,                            // base_voice_id (physical voice)
        1.0f,                               // base_pitch
        "Vlado.bin"                         // data_filename
    },
    // Derived voice: Detence (child, based on Josip)
    {
        "detence",                          // id
        "Detence",                          // display_name
        VoiceLanguage::Croatian,            // native language
        VoiceGender::Male,                  // gender
        VoiceAge::Child,                    // age
        "josip",                            // base_voice_id
        1.5f,                               // base_pitch (higher for child)
        nullptr                             // data_filename (uses base voice)
    },
    // Derived voice: Baba (grandma, based on Josip)
    {
        "baba",                             // id
        "Baba",                             // display_name
        VoiceLanguage::Croatian,            // native language
        VoiceGender::Female,                // gender
        VoiceAge::Senior,                   // age
        "josip",                            // base_voice_id
        1.2f,                               // base_pitch (slightly higher)
        nullptr                             // data_filename (uses base voice)
    },
    // Derived voice: Đedo (grandpa, based on Vlado); was "djed" before 2.0
    {
        "djedo",                            // id
        "\xc4\x90" "edo",                   // display_name (Đedo)
        VoiceLanguage::Serbian,             // native language
        VoiceGender::Male,                  // gender
        VoiceAge::Senior,                   // age
        "vlado",                            // base_voice_id
        0.75f,                              // base_pitch (lower for grandpa)
        nullptr                             // data_filename (uses base voice)
    },
    // Formant voice: Zvonko (Croatian, male, adult); the default voice
    {
        "zvonko",                           // id
        "Zvonko",                           // display_name
        VoiceLanguage::Croatian,            // native language
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
        "Stojan",                           // display_name
        VoiceLanguage::Serbian,             // native language
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
        "Mirsad",                           // display_name
        VoiceLanguage::Bosnian,             // native language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        nullptr,                            // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename (none: synthesized by rule)
        VoiceSynthesis::Formant             // synthesis
    },
    // Singing presets: a formant voice with an instrument for a larynx and
    // a folk song for a melody (src/formant/formant_synthesizer.cpp). The
    // base voice gives the vocal tract; an instrument voice is named after
    // the player and the song, a voice singing as itself after the region
    // of its song.
    {
        "orguljas",                         // id
        "Orgulja\xc5\xa1 - Vila Velebita",  // display_name (Orguljaš: the organist)
        VoiceLanguage::Croatian,            // native language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        "zvonko",                           // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename
        VoiceSynthesis::Formant             // synthesis
    },
    {
        "dalmacija_klapa",                  // id (was "klapa")
        "Dalmacija Klapa",                  // display_name (a one-man klapa)
        VoiceLanguage::Croatian,            // native language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        "zvonko",                           // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename
        VoiceSynthesis::Formant             // synthesis
    },
    {
        "trubac",                           // id
        "Truba\xc4\x8d - Kre\xc4\x87" "e se la\xc4\x91" "a francuska",  // display_name (Trubač: the trumpeter)
        VoiceLanguage::Serbian,             // native language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        "stojan",                           // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename
        VoiceSynthesis::Formant             // synthesis
    },
    {
        "harmonikas",                       // id
        "Harmonika\xc5\xa1 - Kre\xc4\x87" "e se la\xc4\x91" "a francuska",  // display_name (Harmonikaš: the accordionist)
        VoiceLanguage::Serbian,             // native language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        "stojan",                           // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename
        VoiceSynthesis::Formant             // synthesis
    },
    {
        "sevdalija",                        // id
        "Sevdalija - Kad ja po\xc4\x91" "oh na Bemba\xc5\xa1" "u",  // display_name (the sevdah singer)
        VoiceLanguage::Bosnian,             // native language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        "mirsad",                           // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename
        VoiceSynthesis::Formant             // synthesis
    },
    {
        "sazlija",                          // id
        "Sazlija - Kad ja po\xc4\x91" "oh na Bemba\xc5\xa1" "u",  // display_name (the saz player)
        VoiceLanguage::Bosnian,             // native language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        "mirsad",                           // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename
        VoiceSynthesis::Formant             // synthesis
    },
    // The speaking voices singing as themselves, at their own pitch, named
    // after the region of their song.
    {
        "lika_velebit",                     // id (was "pjevac")
        "Lika Velebit",                     // display_name (Zvonko sings Vila Velebita)
        VoiceLanguage::Croatian,            // native language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        "zvonko",                           // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename
        VoiceSynthesis::Formant             // synthesis
    },
    {
        "solunac",                          // id (was "pevac")
        "Solunac",                          // display_name (Stojan sings Kreće se lađa francuska)
        VoiceLanguage::Serbian,             // native language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        "stojan",                           // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename
        VoiceSynthesis::Formant             // synthesis
    },
    {
        "sarajevo_bembasa",                 // id (was "solist")
        "Sarajevo Bemba\xc5\xa1" "a",       // display_name (Mirsad sings Kad ja pođoh na Bembašu)
        VoiceLanguage::Bosnian,             // native language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        "mirsad",                           // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename
        VoiceSynthesis::Formant             // synthesis
    },
    {
        "becarac",                          // id
        "Be\xc4\x87" "arac",                // display_name (Zvonko sings the bećarac)
        VoiceLanguage::Croatian,            // native language
        VoiceGender::Male,                  // gender
        VoiceAge::Adult,                    // age
        "zvonko",                           // base_voice_id
        1.0f,                               // base_pitch
        nullptr,                            // data_filename
        VoiceSynthesis::Formant             // synthesis
    }
};

// Ids of earlier versions, still accepted by find_by_id() so that saved
// settings and scripts keep working.
struct VoiceAlias {
    const char* old_id;
    const char* id;
};

static constexpr VoiceAlias ALIASES[] = {
    {"djed", "djedo"},
    {"dijete", "detence"},
    {"baka", "baba"},
    {"klapa", "dalmacija_klapa"},
    {"pjevac", "lika_velebit"},
    {"pevac", "solunac"},
    {"solist", "sarajevo_bembasa"},
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

    for (const auto& alias : ALIASES) {
        if (std::strcmp(alias.old_id, id) == 0) {
            return find_by_id(alias.id);
        }
    }

    return nullptr;
}

const char* VoiceRegistry::default_voice_id() {
    return "zvonko";
}

const VoiceDefinition* VoiceRegistry::default_voice() {
    return find_by_id(default_voice_id());
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
