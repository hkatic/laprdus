package com.hrvojekatic.laprdus.service

import android.app.Activity
import android.content.Intent
import android.os.Bundle
import android.speech.tts.TextToSpeech

/**
 * Responds to android.speech.tts.engine.CHECK_TTS_DATA.
 *
 * TTS settings implementations (AOSP and OEM forks such as Honor MagicOS)
 * launch this action when the user selects the engine, and use the result
 * to build the engine's list of available languages. Without this activity
 * some OEM settings treat the engine as having no usable voice data, so it
 * appears in the engine list but never speaks.
 *
 * Voice data is bundled in the APK assets, so the check always passes.
 * Locales are reported as ISO3 language-COUNTRY tags, matching what the
 * Android TTS framework uses on the engine boundary.
 */
class CheckVoiceDataActivity : Activity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val returnData = Intent().apply {
            putStringArrayListExtra(
                TextToSpeech.Engine.EXTRA_AVAILABLE_VOICES,
                arrayListOf("hrv-HRV", "srp-SRB")
            )
            putStringArrayListExtra(
                TextToSpeech.Engine.EXTRA_UNAVAILABLE_VOICES,
                arrayListOf()
            )
        }
        setResult(TextToSpeech.Engine.CHECK_VOICE_DATA_PASS, returnData)
        finish()
    }
}
