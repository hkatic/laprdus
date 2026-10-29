package com.hrvojekatic.laprdus.tts

import android.content.Context
import com.hrvojekatic.laprdus.R

/** Samples are in the spoken language even when the interface uses another. */
object SpeechSamples {
    fun preview(context: Context, language: String): String = context.getString(when (Language.fromTag(language)) {
        Language.SERBIAN -> R.string.preview_sample_text_sr
        Language.BOSNIAN -> R.string.preview_sample_text_bs
        Language.SLOVENIAN -> R.string.preview_sample_text_sl
        else -> R.string.preview_sample_text_hr
    })

    fun system(context: Context, language: String): String = context.getString(when (Language.fromTag(language)) {
        Language.SERBIAN -> R.string.tts_sample_text_sr
        Language.BOSNIAN -> R.string.tts_sample_text_bs
        Language.SLOVENIAN -> R.string.tts_sample_text_sl
        else -> R.string.tts_sample_text_hr
    })
}
