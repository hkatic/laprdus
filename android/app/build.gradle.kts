plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)
    alias(libs.plugins.kotlin.compose)
    alias(libs.plugins.ksp)
    alias(libs.plugins.hilt)
}

android {
    namespace = "com.hrvojekatic.laprdus"
    compileSdk = 36

    ndkVersion = "26.1.10909125"

    defaultConfig {
        applicationId = "com.hrvojekatic.laprdus"
        minSdk = 24
        targetSdk = 36
        versionCode = 4
        versionName = "1.0"

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
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    kotlinOptions {
        jvmTarget = "11"
    }
    buildFeatures {
        compose = true
    }

    // Only use main assets directory - voice data and dictionaries are copied
    // to subdirectories by the copyVoiceData and copyDictionaries tasks below
    sourceSets {
        getByName("main") {
            assets.srcDirs("src/main/assets")
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}

dependencies {
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.lifecycle.runtime.ktx)
    implementation(libs.androidx.activity.compose)
    implementation(platform(libs.androidx.compose.bom))
    implementation(libs.androidx.compose.ui)
    implementation(libs.androidx.compose.ui.graphics)
    implementation(libs.androidx.compose.ui.tooling.preview)
    implementation(libs.androidx.compose.material3)

    // ViewModel for Compose
    implementation(libs.androidx.lifecycle.viewmodel.compose)

    // DataStore for settings persistence
    implementation(libs.androidx.datastore.preferences)

    // Coroutines
    implementation(libs.kotlinx.coroutines.android)

    // Material Icons Extended
    implementation(libs.androidx.compose.material.icons.extended)

    // Hilt Dependency Injection
    implementation(libs.hilt.android)
    ksp(libs.hilt.android.compiler)
    implementation(libs.hilt.navigation.compose)

    // Unit tests
    testImplementation(libs.junit)
    testImplementation(libs.mockk)
    testImplementation(libs.kotlinx.coroutines.test)
    testImplementation(libs.turbine)
    testImplementation(libs.robolectric)
    testImplementation(libs.hilt.android.testing)
    testImplementation(libs.arch.core.testing)
    kspTest(libs.hilt.android.compiler)

    // Instrumented tests
    androidTestImplementation(libs.androidx.junit)
    androidTestImplementation(libs.androidx.espresso.core)
    androidTestImplementation(platform(libs.androidx.compose.bom))
    androidTestImplementation(libs.androidx.compose.ui.test.junit4)
    androidTestImplementation(libs.mockk.android)
    androidTestImplementation(libs.kotlinx.coroutines.test)
    androidTestImplementation(libs.turbine)
    androidTestImplementation(libs.hilt.android.testing)
    kspAndroidTest(libs.hilt.android.compiler)

    // Debug implementations
    debugImplementation(libs.androidx.compose.ui.tooling)
    debugImplementation(libs.androidx.compose.ui.test.manifest)
}

// =============================================================================
// Copy voice data and dictionaries to assets subdirectories
// These tasks run before the asset merging step to ensure proper folder structure
// =============================================================================

// Clean old asset files from previous folder structure (pre-1.0.0)
val cleanOldAssets by tasks.registering(Delete::class) {
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
val generateVoiceData by tasks.registering(Exec::class) {
    description = "Generate voice data files using SCons phoneme packer"
    workingDir = file("../../")

    // Determine the correct SCons command based on OS
    val isWindows = System.getProperty("os.name").lowercase().contains("windows")
    if (isWindows) {
        commandLine("cmd", "/c", "scons", "--platform=windows", "--arch=x64", "--build-config=release", "voice-data")
    } else {
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
val verifyVoiceData by tasks.registering {
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

val copyVoiceData by tasks.registering(Copy::class) {
    description = "Copy voice data files to assets/voices subdirectory"
    dependsOn(cleanOldAssets, verifyVoiceData)
    from("../../data/voices") {
        include("*.bin")
    }
    into("src/main/assets/voices")
}

val copyDictionaries by tasks.registering(Copy::class) {
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