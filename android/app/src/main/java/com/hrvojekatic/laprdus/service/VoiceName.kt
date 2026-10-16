package com.hrvojekatic.laprdus.service

import com.hrvojekatic.laprdus.tts.Language
import com.hrvojekatic.laprdus.tts.VoiceInfo

/**
 * The name of a system voice: `<voice id>-<language>`, as in `zvonko-hr`,
 * `djedo-sr`, `lika_velebit-bs`. The system sees one voice per (voice x
 * language), 54 in all, since every Laprdus voice speaks every language.
 */
data class VoiceName(val voiceId: String, val language: String) {

    /** The name as the system sees it. */
    val name: String
        get() = "$voiceId-$language"

    companion object {
        /** All the names of a voice list, in the engine's order. */
        fun all(voices: List<VoiceInfo>): List<VoiceName> =
            voices.flatMap { voice -> Language.ALL.map { VoiceName(voice.id, it) } }

        /**
         * Parses a system voice name. A bare id (`zvonko`, also an id of
         * an earlier version such as `djed`) means the voice in its native
         * language. Returns null for a name that is not a Laprdus voice.
         *
         * @param voices the engine's voice list; a voice must be in it
         */
        fun parse(name: String?, voices: List<VoiceInfo>): VoiceName? {
            if (name.isNullOrBlank()) return null
            val dash = name.lastIndexOf('-')
            if (dash > 0) {
                val language = Language.fromTag(name.substring(dash + 1))
                val voice = find(name.substring(0, dash), voices)
                if (language != null && voice != null) return VoiceName(voice.id, language)
            }
            val voice = find(name, voices) ?: return null
            return VoiceName(voice.id, voice.nativeLanguage)
        }

        private fun find(id: String, voices: List<VoiceInfo>): VoiceInfo? {
            val canonical = VoiceInfo.normalizeId(id)
            return voices.firstOrNull { it.id == canonical }
        }
    }
}
