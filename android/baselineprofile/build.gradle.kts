/*
 * Baseline profile generator and macrobenchmarks for the app.
 *
 * The generator walks the screens TalkBack users wait on (app start,
 * Settings with every group, the voice list) so that ART compiles that code
 * ahead of time: `./gradlew :app:generateBaselineProfile` on a connected
 * device writes app/src/main/generated/baselineProfiles/. The benchmarks
 * measure the same journeys without and with the profile:
 * `./gradlew :baselineprofile:connectedBenchmarkReleaseAndroidTest`.
 * They run in a process of their own, so they also act as an ordinary
 * background client of the TTS service (BackgroundSpeechTest).
 */
plugins {
    alias(libs.plugins.android.test)
    alias(libs.plugins.baselineprofile)
}

android {
    namespace = "com.hrvojekatic.laprdus.baselineprofile"
    compileSdk = 37

    defaultConfig {
        // Profile collection without root needs API 33; benchmarks run from 28.
        minSdk = 28
        targetSdk = 37
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    targetProjectPath = ":app"
}

baselineProfile {
    useConnectedDevices = true
}

dependencies {
    implementation(libs.androidx.junit)
    implementation(libs.androidx.test.runner)
    implementation(libs.androidx.uiautomator)
    implementation(libs.androidx.benchmark.macro.junit4)
}

androidComponents {
    onVariants { variant ->
        val artifactsLoader = variant.artifacts.getBuiltArtifactsLoader()
        variant.instrumentationRunnerArguments.put(
            "targetAppId",
            variant.testedApks.map { artifactsLoader.load(it)?.applicationId ?: "" }
        )
    }
}
