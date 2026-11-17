package com.hrvojekatic.laprdus.service

import android.app.Activity
import android.content.Intent
import android.os.Bundle
import android.speech.tts.TextToSpeech
import com.hrvojekatic.laprdus.data.SettingsRepository
import com.hrvojekatic.laprdus.tts.Language
import com.hrvojekatic.laprdus.tts.SpeechSamples
import com.hrvojekatic.laprdus.tts.SystemLanguage
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withTimeoutOrNull

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

        // A NoDisplay activity must finish before onResume; keep the settings
        // read bounded, as the speech service does during startup.
        val settings = runCatching {
            runBlocking {
                withTimeoutOrNull(2_000) { SettingsRepository.getInstance(this@GetSampleTextActivity).readSettingsNow() }
            }
        }.getOrNull() ?: SettingsRepository.TTSSettings()
        val requested = Language.fromTag(intent?.getStringExtra("language"))
        val language = if (!settings.forceLanguage && requested != null) requested
            else Language.effective(settings.forceLanguage, settings.language, SystemLanguage.locale(this))
        val sampleText = SpeechSamples.system(this, language)

        val returnData = Intent().apply {
            putExtra(TextToSpeech.Engine.EXTRA_SAMPLE_TEXT, sampleText)
        }
        setResult(TextToSpeech.LANG_AVAILABLE, returnData)
        finish()
    }
}
