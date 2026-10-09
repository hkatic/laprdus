package com.hrvojekatic.laprdus.tts

/**
 * Voice information returned from native engine.
 * Matches LaprdusVoiceInfo structure in C++.
 *
 * @property id Internal voice ID: "josip", "vlado", "detence", "baba", "djed",
 *   a formant voice: "zvonko", "stojan", "mirsad", or a singing preset:
 *   "orguljas", "klapa", "trubac", "harmonikas", "sevdalija", "sazlija",
 *   "pjevac", "pevac", "solist", "becarac"
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
     * Whether this voice is synthesized by rule (Zvonko, Stojan, Mirsad and
     * the singing presets). Formant voices accept the wider rate and pitch
     * ranges and have the inflection level and acceleration settings.
     */
    val isFormantVoice: Boolean
        get() = id in FORMANT_VOICE_IDS

    /** Whether this voice sings the text to a folk song instead of speaking it. */
    val isSingingVoice: Boolean
        get() = id in SINGING_VOICE_IDS

    companion object {
        val SINGING_VOICE_IDS = setOf(
            "orguljas", "klapa", "trubac", "harmonikas", "sevdalija", "sazlija",
            "pjevac", "pevac", "solist", "becarac"
        )
        val FORMANT_VOICE_IDS = setOf("zvonko", "stojan", "mirsad") + SINGING_VOICE_IDS

        /** Speed and pitch range of the recorded voices. */
        val RECORDED_RANGE = 0.5f..2.0f

        /** Speed and pitch range of the formant voices. */
        val FORMANT_RANGE = 0.25f..4.0f

        fun isFormantVoice(id: String?): Boolean = id in FORMANT_VOICE_IDS

        /** Speed and pitch range of a voice. */
        fun rangeFor(id: String?): ClosedFloatingPointRange<Float> =
            if (isFormantVoice(id)) FORMANT_RANGE else RECORDED_RANGE

        /**
         * Rate or pitch of a request from another app (TalkBack, a reader):
         * the app asks relative to its normal (1.0), and the user's own
         * Laprdus setting is that normal, so the two multiply. A forced
         * setting ignores the app. The result stays in the voice's range.
         */
        fun requestValue(setting: Float, requested: Float, forced: Boolean, id: String?): Float {
            val value = if (forced) setting else setting * requested
            return if (value.isFinite()) value.coerceIn(rangeFor(id)) else 1.0f
        }
    }

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
            "orguljas" -> "Zvonko Orguljaš"
            "klapa" -> "Klapa Zvonko"
            "trubac" -> "Stojan Trubač"
            "harmonikas" -> "Stojan Harmonikaš"
            "sevdalija" -> "Mirsad Sevdalija"
            "sazlija" -> "Mirsad Sazlija"
            "pjevac" -> "Zvonko Pjevač"
            "pevac" -> "Stojan Pevač"
            "solist" -> "Mirsad Solist"
            "becarac" -> "Zvonko Bećarac"
            else -> displayName
        }
}
