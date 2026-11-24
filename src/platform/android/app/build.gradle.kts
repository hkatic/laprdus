plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.compose)
    alias(libs.plugins.ksp)
    alias(libs.plugins.hilt)
    alias(libs.plugins.baselineprofile)
}

// liblaprdus.so of every ABI, built from the Rust sources by cargo-ndk.
val nativeLibraryDir = layout.buildDirectory.dir("rustJniLibs")

android {
    namespace = "com.hrvojekatic.laprdus"
    compileSdk = 37

    // NDK r30 (LTS): 16 KB-aligned ELF segments by default, sysroot up to API 37.
    ndkVersion = "30.0.16248370"

    defaultConfig {
        applicationId = "com.hrvojekatic.laprdus"
        minSdk = 24
        targetSdk = 37
        versionCode = 42
        versionName = "2.0.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        ndk {
            abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86_64", "x86")
        }
    }

    signingConfigs {
        // The nightly builds' own key (never Google Play's upload key), from
        // the nightly workflow's secrets. Without it, a local build of the
        // nightly is signed with the debug key.
        System.getenv("LAPRDUS_NIGHTLY_KEYSTORE")?.let { keystore ->
            create("nightly") {
                storeFile = file(keystore)
                storePassword = System.getenv("LAPRDUS_NIGHTLY_KEYSTORE_PASSWORD")
                keyAlias = System.getenv("LAPRDUS_NIGHTLY_KEY_ALIAS")
                keyPassword = System.getenv("LAPRDUS_NIGHTLY_KEY_PASSWORD")
            }
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
        // The nightly builds (.github/workflows/nightly.yml): the release
        // build as an app of its own, "Laprdus Nightly" (src/nightly/res).
        // Google Play signs the store's app with a key of its own, so no
        // other build can update it. This one installs next to it, as a
        // second speech engine with its own settings, and each night's APK
        // updates the one before. The version comes from the workflow
        // (androidComponents below).
        create("nightly") {
            initWith(getByName("release"))
            applicationIdSuffix = ".nightly"
            matchingFallbacks += "release"
            signingConfig = signingConfigs.findByName("nightly") ?: signingConfigs.getByName("debug")
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

    // The native library is built by Cargo (buildNativeLibrary below).
    sourceSets["main"].jniLibs.directories.add(nativeLibraryDir.get().asFile.path)
}

// The nightly's version: -PlaprdusNightlyVersion=2.0.0-nightly.20261124 (the
// version name) and -PlaprdusNightlyNumber=612 (the version code: the number
// of commits of the nightly's branch, which only grows). Without them, a
// local build is "<versionName>-nightly.local" with the release's code.
androidComponents {
    onVariants(selector().withBuildType("nightly")) { variant ->
        val version = providers.gradleProperty("laprdusNightlyVersion")
        val number = providers.gradleProperty("laprdusNightlyNumber").map { it.toInt() }
        variant.outputs.forEach { output ->
            val local = "${output.versionName.orNull}-nightly.local"
            output.versionName.set(version.orElse(local))
            if (number.isPresent) {
                output.versionCode.set(number)
            }
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

// The repository root: the Cargo workspace and the data folder.
val repositoryDir = file("../../../..")

fun Exec.cargo(vararg arguments: String) {
    workingDir = repositoryDir
    val cargo = if (System.getProperty("os.name").lowercase().contains("windows")) "cargo.exe" else "cargo"
    commandLine(cargo, *arguments)
}

// Packs the recordings (data/<language>/<Voice>/*.wav) into data/voices and
// stages the dictionaries in data/dictionary.
val generateVoiceData = tasks.register<Exec>("generateVoiceData") {
    description = "Pack the recorded voices and stage the dictionaries (cargo xtask data)"
    cargo("run", "--quiet", "--release", "-p", "xtask", "--", "data")
    inputs.dir(File(repositoryDir, "data/hr"))
    inputs.dir(File(repositoryDir, "data/sr"))
    inputs.dir(File(repositoryDir, "data/sl"))
    outputs.dir(File(repositoryDir, "data/voices"))
    outputs.dir(File(repositoryDir, "data/dictionary"))
}

// Needs the Rust toolchain with the four Android targets and cargo-ndk:
//   rustup target add aarch64-linux-android armv7-linux-androideabi x86_64-linux-android i686-linux-android
//   cargo install cargo-ndk
// The library is optimized in debug builds of the app too: an unoptimized
// synthesizer is too slow for a screen reader, and Rust keeps its bounds
// checks in release builds.
val androidRustTargets = mapOf(
    "arm64-v8a" to "aarch64-linux-android",
    "armeabi-v7a" to "armv7-linux-androideabi",
    "x86_64" to "x86_64-linux-android",
    "x86" to "i686-linux-android",
)

val buildNativeLibrary = tasks.register<Exec>("buildNativeLibrary") {
    description = "Build liblaprdus.so for every ABI (cargo ndk)"
    val ndk = androidComponents.sdkComponents.ndkDirectory
    environment("ANDROID_NDK_HOME", ndk.get().asFile.path)
    environment("ANDROID_NDK_ROOT", ndk.get().asFile.path)
    cargo(
        "ndk", *androidRustTargets.keys.flatMap { listOf("-t", it) }.toTypedArray(),
        "--platform", "24", "-o", nativeLibraryDir.get().asFile.path,
        "build", "--release", "-p", "laprdus-android"
    )
    // A missing Rust target makes rustc fail with "can't find crate for `std`"
    // in the middle of the cargo output, and Gradle reports only cargo's exit
    // code: name the missing targets first. Without rustup (Rust from a
    // package manager) the check is skipped and cargo speaks for itself.
    val targets = androidRustTargets.values.toList()
    val workDir = repositoryDir
    val rustup = if (System.getProperty("os.name").lowercase().contains("windows")) "rustup.exe" else "rustup"
    doFirst {
        val installed = try {
            val process = ProcessBuilder(rustup, "target", "list", "--installed")
                .directory(workDir)
                .redirectError(ProcessBuilder.Redirect.DISCARD)
                .start()
            val output = process.inputStream.bufferedReader().readText()
            if (process.waitFor() == 0) output.lines().map { it.trim() }.toSet() else null
        } catch (e: java.io.IOException) {
            null
        }
        val missing = if (installed == null) emptyList() else targets.filterNot { it in installed }
        if (missing.isNotEmpty()) {
            throw GradleException(
                "The Rust toolchain lacks the Android target(s) ${missing.joinToString(", ")}. " +
                    "Install them with:\n    rustup target add ${missing.joinToString(" ")}"
            )
        }
    }
    inputs.dir(File(repositoryDir, "src/common"))
    inputs.dir(File(repositoryDir, "src/platform/android/native/src"))
    inputs.files(File(repositoryDir, "Cargo.toml"), File(repositoryDir, "Cargo.lock"))
    outputs.dir(nativeLibraryDir)
}

val copyVoiceData = tasks.register<Copy>("copyVoiceData") {
    description = "Copy voice data files to assets/voices subdirectory"
    dependsOn(cleanOldAssets, generateVoiceData)
    from("../../../../data/voices") {
        include("*.bin")
    }
    into("src/main/assets/voices")
}

val copyDictionaries = tasks.register<Copy>("copyDictionaries") {
    description = "Copy dictionary files to assets/dictionaries subdirectory"
    dependsOn(cleanOldAssets, generateVoiceData)
    from("../../../../data/dictionary") {
        include("*.json")
    }
    into("src/main/assets/dictionaries")
}

// Ensure voice data and dictionaries are copied before assets are merged
tasks.named("preBuild") {
    dependsOn(copyVoiceData, copyDictionaries, buildNativeLibrary)
}