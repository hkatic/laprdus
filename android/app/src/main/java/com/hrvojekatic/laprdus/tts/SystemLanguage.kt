package com.hrvojekatic.laprdus.tts

import android.app.LocaleManager
import android.content.Context
import android.content.res.Resources
import android.os.Build
import java.util.Locale

/** Speech follows the device language, independently of the app's UI locale. */
object SystemLanguage {
    fun locale(context: Context): Locale {
        val locales = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            context.getSystemService(LocaleManager::class.java)?.systemLocales
        } else null
        return (locales ?: Resources.getSystem().configuration.locales)[0] ?: Locale.getDefault()
    }
}
