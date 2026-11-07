package com.hrvojekatic.laprdus.baselineprofile

import androidx.benchmark.macro.junit4.BaselineProfileRule
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Collects the baseline profile: app start, the main screen, and every part
 * of Settings (the screen TalkBack users move through most).
 *
 * `./gradlew :app:generateBaselineProfile` with a device connected writes
 * the profile into app/src/main/generated/baselineProfiles/.
 */
@RunWith(AndroidJUnit4::class)
class BaselineProfileGenerator {

    @get:Rule
    val rule = BaselineProfileRule()

    @Test
    fun generate() = rule.collect(
        packageName = targetPackage,
        includeInStartupProfile = true,
    ) {
        pressHome()
        startActivityAndWait()
        openSettings()
        scrollSettings()
        expandMoreVoiceSettings()
        browseVoices()
    }
}
