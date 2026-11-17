package com.hrvojekatic.laprdus.tts

/**
 * Voice information returned from native engine.
 * Matches the VoiceDefinition of the C++ voice registry.
 *
 * Every voice speaks every language (see [Language]); [languageCode] is only
 * the voice's native language, the one it speaks when no other is chosen.
 *
 * @property id Internal voice ID, plain ASCII: the recorded voices "josip",
 *   "vlado", "detence", "baba", "djedo"; the formant voices "zvonko",
 *   "stojan", "mirsad"; the singing presets "orguljas", "dalmacija_klapa",
 *   "trubac", "harmonikas", "sevdalija", "sazlija", "lika_velebit", "solunac",
 *   "sarajevo_bembasa", "becarac"
 * @property displayName The name shown to the user, from the registry:
 *   "Josip", "Đedo", "Dalmacija Klapa"
 * @property languageCode BCP-47 tag of the native language: "hr-HR", "sr-RS", "bs-BA" or "sl-SI"
 * @property gender Voice gender: "Male" or "Female"
 * @property age Voice age category: "Child", "Adult", or "Senior"
 * @property basePitch Base pitch multiplier for derived voices (1.0 for physical voices)
 * @property baseVoiceId The voice this one is derived from ("josip", "vlado",
 *   "zvonko", ...), null for josip, vlado, zvonko, stojan and mirsad
 * @property isFormant Whether the voice is synthesized by rule (no data file)
 */
data class VoiceInfo(
    val id: String,
    val displayName: String,
    val languageCode: String,
    val gender: String,
    val age: String,
    val basePitch: Float,
    val baseVoiceId: String? = null,
    val isFormant: Boolean = false
) {
    /**
     * Whether this voice is not derived from another one by a pitch change.
     * True for josip, vlado and the formant voices (zvonko, stojan, mirsad);
     * false for detence (child), baba (grandma), djedo (grandpa).
     */
    val isPhysicalVoice: Boolean
        get() = basePitch == 1.0f

    /**
     * Whether this voice is synthesized by rule (Zvonko, Stojan, Mirsad and
     * the singing presets). Formant voices accept the wider rate and pitch
     * ranges.
     */
    val isFormantVoice: Boolean
        get() = isFormant

    /**
     * Whether this voice sings the text to a folk song instead of speaking
     * it: a formant voice derived from another formant voice.
     */
    val isSingingVoice: Boolean
        get() = isFormant && baseVoiceId != null

    /** Short code of the voice's native language: "hr", "sr", "bs" or "sl". */
    val nativeLanguage: String
        get() = Language.fromTag(languageCode) ?: Language.DEFAULT

    companion object {
        /** The ids of the formant voices that sing instead of speaking. */
        val SINGING_VOICE_IDS = setOf(
            "orguljas", "dalmacija_klapa", "trubac", "harmonikas", "sevdalija", "sazlija",
            "lika_velebit", "solunac", "sarajevo_bembasa", "becarac"
        )

        /** The ids of every voice synthesized by rule. */
        val FORMANT_VOICE_IDS = setOf("zvonko", "stojan", "mirsad") + SINGING_VOICE_IDS

        /**
         * The ids of earlier versions and the ids of the renamed voices.
         * The engine accepts the old ids too; this map is for comparing a
         * saved id against the voice list.
         */
        val LEGACY_IDS: Map<String, String> = mapOf(
            "djed" to "djedo",
            "dijete" to "detence",
            "baka" to "baba",
            "klapa" to "dalmacija_klapa",
            "pjevac" to "lika_velebit",
            "pevac" to "solunac",
            "solist" to "sarajevo_bembasa"
        )

        /** The canonical id of a voice: an old id gives the renamed voice's. */
        fun normalizeId(id: String): String = LEGACY_IDS[id] ?: id

        /** Speed and pitch range of the recorded voices. */
        val RECORDED_RANGE = 0.5f..2.0f

        /** Speed and pitch range of the formant voices. */
        val FORMANT_RANGE = 0.25f..4.0f

        /** Whether [id] (an old id too) names a voice synthesized by rule. */
        fun isFormantVoice(id: String?): Boolean =
            id != null && normalizeId(id) in FORMANT_VOICE_IDS

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
}
