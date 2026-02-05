package com.hrvojekatic.laprdus

import android.app.Application
import dagger.hilt.android.HiltAndroidApp

/**
 * Application class for Laprdus TTS.
 * Annotated with @HiltAndroidApp to enable Hilt dependency injection.
 */
@HiltAndroidApp
class LaprdusApplication : Application()
