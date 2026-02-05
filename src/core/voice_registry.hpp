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
 * - Derived voices (Detence, Baba, Djedo) that use physical voice data with pitch modification
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
     * Find voice by ID.
     * @param id Voice ID (e.g., "josip").
     * @return Pointer to voice definition, or nullptr if not found.
     */
    static const VoiceDefinition* find_by_id(const char* id);

    /**
     * Get voice by index.
     * @param index Voice index (0 to count-1).
     * @return Pointer to voice definition, or nullptr if out of range.
     */
    static const VoiceDefinition* get_by_index(size_t index);

    /**
     * Get the default voice.
     * @return Pointer to default voice definition (Josip).
     */
    static const VoiceDefinition* default_voice();

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
     * Check if a voice is physical (has its own phoneme data).
     * @param voice Voice definition.
     * @return true if physical, false if derived.
     */
    static bool is_physical_voice(const VoiceDefinition* voice);
};

} // namespace laprdus

#endif // LAPRDUS_VOICE_REGISTRY_HPP
