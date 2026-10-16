package com.hrvojekatic.laprdus.tts

import java.util.Locale

/**
 * The languages Laprdus speaks. Every voice speaks every one of them; the
 * language (letter-to-sound rules, accent lexicon, letter names, number
 * words) is chosen separately from the voice, as a short code: "hr", "sr"
 * or "bs". These codes are what the settings store and the engine's
 * [LaprdusTTS.setLanguage] take.
 */
object Language {
    const val CROATIAN = "hr"
    const val SERBIAN = "sr"
    const val BOSNIAN = "bs"

    /** Every language, in the engine's order. */
    val ALL: List<String> = listOf(CROATIAN, SERBIAN, BOSNIAN)

    /** The language spoken when nothing else is asked for. */
    const val DEFAULT = CROATIAN

    /**
     * The short code of a language tag or name, or null for any other
     * language: "hr", "hr-HR", "hr_HR", "hrv" (the ISO3 code the Android
     * framework passes on the engine boundary), "sr-Latn-RS", "srp", "bs",
     * "bos", "Croatian", "hrvatski", in any case. Only the first subtag counts.
     */
    fun fromTag(tag: String?): String? {
        if (tag.isNullOrBlank()) return null
        val first = tag.trim().split('-', '_', '.', '@')[0].lowercase(Locale.ROOT)
        return when (first) {
            "hr", "hrv", "croatian", "hrvatski" -> CROATIAN
            "sr", "srp", "serbian", "srpski" -> SERBIAN
            "bs", "bos", "bosnian", "bosanski" -> BOSNIAN
            else -> null
        }
    }

    /** Whether [code] is one of the three short codes. */
    fun isValid(code: String?): Boolean = code in ALL

    /** The BCP-47 tag of a language: "hr-HR", "sr-RS", "bs-BA". */
    fun bcp47(code: String): String = when (code) {
        SERBIAN -> "sr-RS"
        BOSNIAN -> "bs-BA"
        else -> "hr-HR"
    }

    /** The locale of a language, for the system's voice list. */
    fun locale(code: String): Locale = Locale.forLanguageTag(bcp47(code))

    /**
     * The language as the Android TTS framework names it on the engine
     * boundary: ISO3 language, ISO3 country, variant.
     */
    fun iso3(code: String): Array<String> = when (code) {
        SERBIAN -> arrayOf("srp", "SRB", "")
        BOSNIAN -> arrayOf("bos", "BIH", "")
        else -> arrayOf("hrv", "HRV", "")
    }

    /**
     * The language of the device when it is one Laprdus speaks, else
     * Croatian: the default of the Language setting.
     */
    fun deviceDefault(locale: Locale = Locale.getDefault()): String =
        fromTag(locale.language) ?: DEFAULT
}
