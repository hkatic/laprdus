// -*- coding: utf-8 -*-
// voice_registry.hpp - Voice definitions and registry
// Manages available voices for LaprdusTTS

#ifndef LAPRDUS_VOICE_REGISTRY_HPP
#define LAPRDUS_VOICE_REGISTRY_HPP

#include "laprdus/types.hpp"
#include <cstddef>

namespace laprdus {

/**
 * VoiceRegistry - Static registry of available voices.
 *
 * Provides access to voice definitions including:
 * - Physical voices (Josip, Vlado) with their own phoneme data
 * - Derived voices (Detence, Baba, Đedo) that use physical voice data with pitch modification
 * - Formant voices (Zvonko, Stojan, Mirsad) synthesized by rule, with no data file
 * - Singing presets of the formant voices
 *
 * Every voice speaks every language (Croatian, Serbian, Bosnian); a voice's
 * `language` is only its native one. The default voice is Zvonko.
 */
class VoiceRegistry {
public:
    /**
     * Get all voice definitions.
     * @return Span of voice definitions.
     */
    static span<const VoiceDefinition> all_voices();

    /**
     * Get the number of available voices.
     * @return Voice count.
     */
    static size_t voice_count();

    /**
     * Find voice by ID. The ids of earlier versions ("djed", "klapa",
     * "pjevac", "pevac", "solist") are accepted and give the renamed voice.
     * @param id Voice ID (e.g., "josip").
     * @return Pointer to voice definition, or nullptr if not found.
     */
    static const VoiceDefinition* find_by_id(const char* id);

    /** Id of the default voice of every language and platform: "zvonko". */
    static const char* default_voice_id();

    /** The default voice's definition. */
    static const VoiceDefinition* default_voice();

    /**
     * Get voice by index.
     * @param index Voice index (0 to count-1).
     * @return Pointer to voice definition, or nullptr if out of range.
     */
    static const VoiceDefinition* get_by_index(size_t index);

    /**
     * Get the physical voice for a voice definition.
     * For physical voices, returns the same voice.
     * For derived voices, returns the base physical voice.
     * @param voice Voice definition.
     * @return Pointer to physical voice definition.
     */
    static const VoiceDefinition* get_physical_voice(const VoiceDefinition* voice);

    /**
     * Get the data filename for a voice.
     * Resolves derived voices to their physical voice's data file.
     * @param voice Voice definition.
     * @return Data filename (e.g., "Josip.bin"), or nullptr on error.
     */
    static const char* get_data_filename(const VoiceDefinition* voice);

    /**
     * Check if a voice is a formant voice (synthesized by rule, no data file).
     * @param voice Voice definition.
     * @return true if the voice uses formant synthesis.
     */
    static bool is_formant_voice(const VoiceDefinition* voice);

    /**
     * Words per minute a voice speaks at speed 1.0 and acceleration 1.0,
     * measured on running Croatian text (formant voices:
     * FORMANT_NOMINAL_WPM times the voice's own tempo).
     * @param id Voice ID.
     * @return Words per minute, or 0.0f for an unknown voice.
     */
    static float nominal_wpm(const char* id);
};

} // namespace laprdus

#endif // LAPRDUS_VOICE_REGISTRY_HPP
