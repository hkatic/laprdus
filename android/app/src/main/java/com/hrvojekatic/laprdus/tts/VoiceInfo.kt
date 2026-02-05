package com.hrvojekatic.laprdus.tts

/**
 * Voice information returned from native engine.
 * Matches LaprdusVoiceInfo structure in C++.
 *
 * @property id Internal voice ID: "josip", "vlado", "detence", "baba", "djedo"
 * @property displayName User-visible name: "Laprdus Josip (Croatian)"
 * @property languageCode BCP-47 language tag: "hr-HR" or "sr-RS"
 * @property gender Voice gender: "Male" or "Female"
 * @property age Voice age category: "Child", "Adult", or "Senior"
 * @property basePitch Base pitch multiplier for derived voices (1.0 for physical voices)
 */
data class VoiceInfo(
    val id: String,
    val displayName: String,
    val languageCode: String,
    val gender: String,
    val age: String,
    val basePitch: Float
) {
    /**
     * Whether this is a physical voice (has its own .bin file)
     * Physical voices: josip, vlado
     * Derived voices: detence (child), baba (grandma), djedo (grandpa)
     */
    val isPhysicalVoice: Boolean
        get() = basePitch == 1.0f

    /**
     * Whether this voice speaks Croatian
     */
    val isCroatian: Boolean
        get() = languageCode == "hr-HR"

    /**
     * Whether this voice speaks Serbian
     */
    val isSerbian: Boolean
        get() = languageCode == "sr-RS"

    /**
     * Get a user-friendly display name with language info
     */
    val localizedDisplayName: String
        get() = when (id) {
            "josip" -> "Josip"
            "vlado" -> "Vlado"
            "detence" -> "Dijete"
            "baba" -> "Baka"
            "djedo" -> "Djed"
            else -> displayName
        }
}
