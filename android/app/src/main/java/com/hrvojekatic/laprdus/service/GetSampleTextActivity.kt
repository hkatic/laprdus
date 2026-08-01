package com.hrvojekatic.laprdus.service

import android.app.Activity
import android.content.Intent
import android.os.Bundle
import android.speech.tts.TextToSpeech
import com.hrvojekatic.laprdus.R

/**
 * Responds to android.speech.tts.engine.GET_SAMPLE_TEXT.
 *
 * TTS settings launch this action to obtain the "Listen to an example"
 * text after the engine is selected. The framework passes the requested
 * locale as ISO3 "language"/"country"/"variant" string extras.
 */
class GetSampleTextActivity : Activity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val language = intent?.getStringExtra("language")?.lowercase() ?: ""
        val sampleText = when (language) {
            "sr", "srp" -> getString(R.string.tts_sample_text_sr)
            else -> getString(R.string.tts_sample_text_hr)
        }

        val returnData = Intent().apply {
            putExtra(TextToSpeech.Engine.EXTRA_SAMPLE_TEXT, sampleText)
        }
        setResult(TextToSpeech.LANG_AVAILABLE, returnData)
        finish()
    }
}
