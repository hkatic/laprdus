package com.hrvojekatic.laprdus.service

import com.hrvojekatic.laprdus.tts.Language
import com.hrvojekatic.laprdus.tts.VoiceInfo

/**
 * Which voice and which language the speech service speaks with.
 *
 * The voice is the one configured in Laprdus's settings; the apps choose
 * only the language. An app's `setLanguage()` (onLoadLanguage) or
 * `setVoice()` (onLoadVoice, with a name such as `zvonko-sr`) sets the
 * requested language and leaves the voice alone, and every request carries
 * its own locale. "Force language" makes the service ignore all of that and
 * speak the language chosen in Laprdus's settings.
 *
 * Plain Kotlin, so the policy is unit-tested without the native engine.
 */
class SpeechSelection(
    voiceId: String,
    language: String = Language.DEFAULT
) {
    /** The configured voice (canonical id). */
    @Volatile
    var voiceId: String = VoiceInfo.normalizeId(voiceId)
        private set

    /** The language the last app asked for (short code). */
    @Volatile
    var requestedLanguage: String = Language.fromTag(language) ?: Language.DEFAULT
        private set

    /** The user chose a voice in Laprdus's settings. */
    fun setConfiguredVoice(id: String) {
        voiceId = VoiceInfo.normalizeId(id)
    }

    /**
     * An app asked for a language (onLoadLanguage). A language Laprdus
     * speaks is remembered and returned as its short code; any other leaves
     * the requested language alone and returns null. The voice stays.
     */
    fun requestLanguage(tag: String?): String? {
        val language = Language.fromTag(tag) ?: return null
        requestedLanguage = language
        return language
    }

    /**
     * An app chose a system voice by name (onLoadVoice). Only the name's
     * language is taken: the configured voice always speaks (see the class
     * comment). Returns false for a name that is not a Laprdus voice.
     */
    fun requestVoiceName(name: String?, voices: List<VoiceInfo>): Boolean {
        val parsed = VoiceName.parse(name, voices) ?: return false
        requestedLanguage = parsed.language
        return true
    }

    /**
     * The language to speak a request with: the request's own locale (an
     * app that set none gets the last requested language), unless the
     * language is forced, in which case the Laprdus setting.
     */
    fun languageFor(requestTag: String?, forceLanguage: Boolean, forcedLanguage: String): String {
        if (forceLanguage) return Language.fromTag(forcedLanguage) ?: Language.DEFAULT
        return Language.fromTag(requestTag) ?: requestedLanguage
    }

    /** The language reported to the system (onGetLanguage). */
    fun currentLanguage(forceLanguage: Boolean, forcedLanguage: String): String =
        languageFor(null, forceLanguage, forcedLanguage)

    /**
     * The default system voice of a language (onGetDefaultVoiceNameFor): the
     * configured voice in that language, in Croatian for any other language.
     */
    fun defaultVoiceNameFor(tag: String?): String =
        VoiceName(voiceId, Language.fromTag(tag) ?: Language.DEFAULT).name
}
