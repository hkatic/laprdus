plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.compose)
    alias(libs.plugins.ksp)
    alias(libs.plugins.hilt)
    alias(libs.plugins.baselineprofile)
}

android {
    namespace = "com.hrvojekatic.laprdus"
    compileSdk = 37

    // NDK r30 (LTS): 16 KB-aligned ELF segments by default, sysroot up to API 37.
    ndkVersion = "30.0.16248370"

    defaultConfig {
        applicationId = "com.hrvojekatic.laprdus"
        minSdk = 24
        targetSdk = 37
        versionCode = 37
        versionName = "2.0.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        ndk {
            abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86_64", "x86")
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            // Symbol tables of the native library in the bundle, so that
            // Google Play symbolicates native crashes and ANRs (Android vitals).
            ndk {
                debugSymbolLevel = "SYMBOL_TABLE"
            }
        }
    }
    compileOptions {
        // Built-in Kotlin takes its JVM target from here.
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    buildFeatures {
        compose = true
        buildConfig = true
    }
    packaging {
        resources {
            // JUnit 5 jars each bundle META-INF license files; exclude them
            // so the androidTest APK can merge its java resources
            excludes += "META-INF/LICENSE*.md"
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "4.1.2"
        }
    }
}

// The baseline profile (app start, Settings, the voice list) lives in
// src/main/generated/baselineProfiles; regenerate it with a device connected:
// ./gradlew :app:generateBaselineProfile (see baselineprofile/build.gradle.kts).
baselineProfile {
    mergeIntoMain = true
}

dependencies {
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.core.splashscreen)
    implementation(libs.androidx.lifecycle.runtime.ktx)
    implementation(libs.androidx.lifecycle.runtime.compose)
    implementation(libs.androidx.activity.compose)
    implementation(platform(libs.androidx.compose.bom))
    implementation(libs.androidx.compose.ui)
    implementation(libs.androidx.compose.ui.graphics)
    implementation(libs.androidx.compose.ui.tooling.preview)
    implementation(libs.androidx.compose.material3)
    implementation(libs.androidx.navigation.compose)
    // Brought in by Hilt and profileinstaller at old versions; current releases
    // (the instrumented tests' libraries need them, and the test classpath
    // must resolve to the app's versions).
    implementation(libs.androidx.fragment)
    implementation(libs.androidx.concurrent.futures)

    // ViewModel for Compose
    implementation(libs.androidx.lifecycle.viewmodel.compose)

    // DataStore for settings persistence
    implementation(libs.androidx.datastore.preferences)

    // Coroutines
    implementation(libs.kotlinx.coroutines.android)

    // Installs the baseline profile (app start, Settings) on devices without Play
    implementation(libs.androidx.profileinstaller)
    "baselineProfile"(project(":baselineprofile"))

    // Hilt Dependency Injection
    implementation(libs.hilt.android)
    ksp(libs.hilt.android.compiler)
    implementation(libs.hilt.lifecycle.viewmodel.compose)

    // Unit tests
    testImplementation(libs.junit)
    testImplementation(libs.mockk)
    testImplementation(libs.kotlinx.coroutines.test)
    testImplementation(libs.robolectric)

    // Instrumented tests
    androidTestImplementation(libs.androidx.junit)
    // Not used directly, but Compose's test rules run on Espresso and the
    // runner: without these the 3.5.0 / 1.5.0 they bring win, and Espresso
    // 3.5 calls InputManager.getInstance(), which Android 17 removed.
    androidTestImplementation(libs.androidx.espresso.core)
    androidTestImplementation(libs.androidx.test.runner)
    androidTestImplementation(platform(libs.androidx.compose.bom))
    androidTestImplementation(libs.androidx.compose.ui.test.junit4)
    androidTestImplementation(libs.androidx.compose.ui.test.junit4.accessibility)

    // Debug implementations
    debugImplementation(libs.androidx.compose.ui.tooling)
    debugImplementation(libs.androidx.compose.ui.test.manifest)
}

// Robolectric's Android 17 (SDK 37) sandbox sets up shared memory through
// FileDescriptor internals that JDK 21 keeps in a non-exported package.
tasks.withType<Test>().configureEach {
    jvmArgs(
        "--add-exports=java.base/jdk.internal.access=ALL-UNNAMED",
        "--add-opens=java.base/java.io=ALL-UNNAMED",
    )
}

// The accessibility checks bring Guava, which carries ListenableFuture
// itself and asks for the empty "9999.0" listenablefuture; the app has the
// stand-alone 1.0, and the test classpath must resolve to the app's versions.
configurations.matching { it.name.contains("AndroidTest") }.configureEach {
    exclude(group = "com.google.guava", module = "listenablefuture")
}

// =============================================================================
// Copy voice data and dictionaries to assets subdirectories
// These tasks run before the asset merging step to ensure proper folder structure
// =============================================================================

// Clean old asset files from previous folder structure (pre-1.0.0)
val cleanOldAssets = tasks.register<Delete>("cleanOldAssets") {
    description = "Remove old asset files from previous folder structure"
    delete(
        // Old voice files at root
        "src/main/assets/Josip.bin",
        "src/main/assets/Vlado.bin",
        // Old dictionary files at root
        "src/main/assets/internal.json",
        "src/main/assets/spelling.json",
        "src/main/assets/emoji.json"
    )
}

// Generate voice data using SCons if not already present
val generateVoiceData = tasks.register<Exec>("generateVoiceData") {
    description = "Generate voice data files using SCons phoneme packer"
    workingDir = file("../../")

    // Determine the correct SCons command based on OS. The phoneme packer is a
    // host tool, so it has to be built for the host platform.
    val osName = System.getProperty("os.name").lowercase()
    when {
        osName.contains("windows") ->
            commandLine("cmd", "/c", "scons", "--platform=windows", "--arch=x64", "--build-config=release", "voice-data")
        osName.contains("mac") || osName.contains("darwin") ->
            commandLine("scons", "--platform=macos", "--build-config=release", "voice-data")
        else ->
            commandLine("scons", "--platform=linux", "--arch=x64", "--build-config=release", "voice-data")
    }

    // Only run if voice data files don't exist
    val josipBin = file("../../data/voices/Josip.bin")
    val vladoBin = file("../../data/voices/Vlado.bin")

    onlyIf {
        !josipBin.exists() || !vladoBin.exists()
    }

    // Don't fail the build if SCons isn't available - just warn
    isIgnoreExitValue = true

    doLast {
        if (executionResult.get().exitValue != 0) {
            logger.warn("WARNING: Failed to generate voice data with SCons.")
            logger.warn("Voice data files may be missing. Install SCons with: pip install scons")
        }
    }
}

// Verify voice data exists before copying
val verifyVoiceData = tasks.register("verifyVoiceData") {
    description = "Verify that voice data files exist"
    dependsOn(generateVoiceData)

    doLast {
        val josipBin = file("../../data/voices/Josip.bin")
        val vladoBin = file("../../data/voices/Vlado.bin")

        if (!josipBin.exists() || !vladoBin.exists()) {
            throw GradleException(
                """
                |
                |ERROR: Voice data files not found in data/voices/
                |
                |The Android build requires voice data files (Josip.bin, Vlado.bin).
                |
                |To generate them, run one of these commands from the project root:
                |
                |  Windows:  scons --platform=windows --arch=x64 --build-config=release voice-data
                |  Linux:    scons --platform=linux --arch=x64 --build-config=release voice-data
                |  macOS:    scons --platform=macos --build-config=release voice-data
                |
                |Or use the master build script:
                |  ./scripts/build-all.sh android
                |
                """.trimMargin()
            )
        }
        logger.lifecycle("Voice data files verified: Josip.bin, Vlado.bin")
    }
}

val copyVoiceData = tasks.register<Copy>("copyVoiceData") {
    description = "Copy voice data files to assets/voices subdirectory"
    dependsOn(cleanOldAssets, verifyVoiceData)
    from("../../data/voices") {
        include("*.bin")
    }
    into("src/main/assets/voices")
}

val copyDictionaries = tasks.register<Copy>("copyDictionaries") {
    description = "Copy dictionary files to assets/dictionaries subdirectory"
    dependsOn(cleanOldAssets)
    from("../../data/dictionary") {
        include("*.json")
    }
    into("src/main/assets/dictionaries")
}

// Ensure voice data and dictionaries are copied before assets are merged
tasks.named("preBuild") {
    dependsOn(copyVoiceData, copyDictionaries)
}