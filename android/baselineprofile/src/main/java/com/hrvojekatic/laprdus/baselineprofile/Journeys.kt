package com.hrvojekatic.laprdus.baselineprofile

import android.content.Intent
import androidx.benchmark.macro.MacrobenchmarkScope
import androidx.test.platform.app.InstrumentationRegistry
import androidx.test.uiautomator.By
import androidx.test.uiautomator.Direction
import androidx.test.uiautomator.Until

/** The app under test, passed by the build (`targetAppId`). */
val targetPackage: String
    get() = InstrumentationRegistry.getArguments().getString("targetAppId")
        ?: "com.hrvojekatic.laprdus"

private const val TIMEOUT_MS = 5_000L

/**
 * Opens the Settings screen. The screen exposes its test tags as resource
 * ids (`testTagsAsResourceId`), so the journeys do not depend on the
 * language of the device.
 */
fun MacrobenchmarkScope.openSettings() {
    startActivityAndWait(
        Intent().setClassName(packageName, "$packageName.SettingsActivity")
            .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
    )
    device.wait(Until.hasObject(By.res("settings_column")), TIMEOUT_MS)
}

/** Scrolls Settings to the end and back, as a user reading it does. */
fun MacrobenchmarkScope.scrollSettings() {
    val column = device.findObject(By.res("settings_column")) ?: return
    column.setGestureMargin(device.displayWidth / 5)
    repeat(2) { column.fling(Direction.DOWN) }
    device.waitForIdle()
    repeat(2) { column.fling(Direction.UP) }
    device.waitForIdle()
}

/** Opens the "more voice settings" group, scrolls through it, and closes it. */
fun MacrobenchmarkScope.expandMoreVoiceSettings() {
    device.findObject(By.res("settings_more_voice"))?.click() ?: return
    device.waitForIdle()
    scrollSettings()
    device.findObject(By.res("settings_more_voice"))?.click()
    device.waitForIdle()
}

/** Opens the voice list, scrolls it, and closes it without choosing. */
fun MacrobenchmarkScope.browseVoices() {
    device.findObject(By.res("settings_voice"))?.click() ?: return
    device.wait(Until.hasObject(By.res("settings_choice_list")), TIMEOUT_MS)
    device.findObject(By.res("settings_choice_list"))?.let { list ->
        list.setGestureMargin(device.displayWidth / 5)
        list.fling(Direction.DOWN)
        list.fling(Direction.UP)
    }
    device.pressBack()
    device.waitForIdle()
}
