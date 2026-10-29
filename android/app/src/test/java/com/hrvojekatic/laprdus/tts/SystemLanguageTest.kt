package com.hrvojekatic.laprdus.tts

import android.content.res.Configuration
import android.content.res.Resources
import android.os.LocaleList
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.RuntimeEnvironment
import org.robolectric.annotation.Config
import java.util.Locale

@RunWith(RobolectricTestRunner::class)
@Config(sdk = [32, 36])
class SystemLanguageTest {
    @Test
    fun speechFollowsSystemEvenWhenAppUsesCroatian() {
        val system = Resources.getSystem()
        val original = Configuration(system.configuration)
        val originalDefault = Locale.getDefault()
        val appConfig = Configuration(original).apply { setLocale(Locale("hr", "HR")) }
        val context = RuntimeEnvironment.getApplication().createConfigurationContext(appConfig)
        try {
            setSystemLocales(Locale("sl", "SI"), Locale("hr", "HR"))
            Locale.setDefault(Locale("hr", "HR"))
            assertEquals("sl", Language.deviceLanguage(SystemLanguage.locale(context)))
            assertEquals("sl", Language.effective(false, "hr", SystemLanguage.locale(context)))
            assertEquals("hr", Language.effective(true, "hr", SystemLanguage.locale(context)))

            // A running process must see a changed system language too.
            setSystemLocales(Locale("bs", "BA"))
            assertEquals("bs", Language.deviceLanguage(SystemLanguage.locale(context)))
            setSystemLocales(Locale.US)
            assertEquals("sl", Language.effective(false, "sl", SystemLanguage.locale(context)))
        } finally {
            @Suppress("DEPRECATION")
            system.updateConfiguration(original, system.displayMetrics)
            Locale.setDefault(originalDefault)
        }
    }

    private fun setSystemLocales(vararg locales: Locale) {
        val system = Resources.getSystem()
        val config = Configuration(system.configuration).apply { setLocales(LocaleList(*locales)) }
        @Suppress("DEPRECATION")
        system.updateConfiguration(config, system.displayMetrics)
    }
}
