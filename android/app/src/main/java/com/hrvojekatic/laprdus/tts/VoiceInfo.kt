package com.hrvojekatic.laprdus.tts

/**
 * Voice information returned from native engine.
 * Matches LaprdusVoiceInfo structure in C++.
 *
 * @property id Internal voice ID: "josip", "vlado", "detence", "baba", "djed",
 *   or a formant voice: "zvonko", "stojan", "mirsad"
 * @property displayName User-visible name: "Laprdus Josip (Croatian)"
 * @property languageCode BCP-47 language tag: "hr-HR", "sr-RS" or "bs-BA"
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
     * Whether this voice is not derived from another one by a pitch change.
     * True for josip, vlado and the formant voices (zvonko, stojan, mirsad);
     * false for detence (child), baba (grandma), djed (grandpa).
     */
    val isPhysicalVoice: Boolean
        get() = basePitch == 1.0f

    /**
     * Whether this voice speaks Croatian
     */
    val isCroatian: Boolean
        get() = languageCode == "hr-HR"

    /**
     * Whether this voice speaks Bosnian
     */
    val isBosnian: Boolean
        get() = languageCode == "bs-BA"

    /**
     * Get a user-friendly display name with language info
     */
    val localizedDisplayName: String
        get() = when (id) {
            "josip" -> "Josip"
            "vlado" -> "Vlado"
            "detence" -> "Dijete"
            "baba" -> "Baka"
            "djed" -> "Đedo"
            "zvonko" -> "Zvonko"
            "stojan" -> "Stojan"
            "mirsad" -> "Mirsad"
            else -> displayName
        }
}
