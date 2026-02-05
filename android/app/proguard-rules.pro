# ProGuard/R8 rules for Laprdus TTS
# These rules ensure proper code obfuscation while preserving critical functionality

# ============================================================================
# JNI Native Methods - CRITICAL: Do not obfuscate
# ============================================================================

# Keep the LaprdusTTS class and all its members (native methods must retain names)
-keep class com.hrvojekatic.laprdus.tts.LaprdusTTS {
    *;
}

# Keep VoiceInfo data class - returned from native code via JNI
-keep class com.hrvojekatic.laprdus.tts.VoiceInfo {
    <init>(...);
    *;
}

# Keep native method signatures
-keepclasseswithmembernames class * {
    native <methods>;
}

# ============================================================================
# TextToSpeechService - System Service (must be accessible by Android framework)
# ============================================================================

-keep class com.hrvojekatic.laprdus.service.LaprdusTTSService {
    <init>();
    public *;
    protected *;
}

# Keep TextToSpeechService callbacks (inherited from parent)
-keep class * extends android.speech.tts.TextToSpeechService {
    public *;
    protected *;
}

# ============================================================================
# Hilt Dependency Injection
# ============================================================================

# Keep Hilt-generated components
-keep class dagger.hilt.** { *; }
-keep class javax.inject.** { *; }
-keep class * extends dagger.hilt.android.internal.managers.ComponentSupplier { *; }

# Keep classes annotated with Hilt annotations
-keep @dagger.hilt.android.HiltAndroidApp class * { *; }
-keep @dagger.hilt.android.AndroidEntryPoint class * { *; }
-keep @dagger.hilt.InstallIn class * { *; }
-keep @dagger.Module class * { *; }
-keep @javax.inject.Singleton class * { *; }
-keep @dagger.hilt.android.lifecycle.HiltViewModel class * { *; }

# Keep Application class with HiltAndroidApp
-keep class com.hrvojekatic.laprdus.LaprdusApplication { *; }

# Keep Activities with AndroidEntryPoint
-keep class com.hrvojekatic.laprdus.MainActivity { *; }
-keep class com.hrvojekatic.laprdus.SettingsActivity { *; }

# Keep AppModule (Hilt module)
-keep class com.hrvojekatic.laprdus.di.AppModule { *; }

# Keep ViewModels with HiltViewModel
-keep class com.hrvojekatic.laprdus.viewmodel.TTSViewModel { *; }
-keep class com.hrvojekatic.laprdus.viewmodel.TTSUiState { *; }
-keep class com.hrvojekatic.laprdus.viewmodel.SettingsViewModel { *; }
-keep class com.hrvojekatic.laprdus.viewmodel.SettingsUiState { *; }

# Keep Hilt-generated Hilt_* and *_Factory classes
-keep class **_Factory { *; }
-keep class **_HiltModules* { *; }
-keep class *_HiltComponents* { *; }
-keep class *Hilt_* { *; }

# Keep @Inject annotated constructors and fields
-keepclassmembers class * {
    @javax.inject.Inject <init>(...);
    @javax.inject.Inject <fields>;
}

# Keep @Provides annotated methods
-keepclassmembers class * {
    @dagger.Provides <methods>;
}

# ============================================================================
# Jetpack Compose
# ============================================================================

# Keep Compose runtime
-keep class androidx.compose.runtime.** { *; }

# Keep @Composable functions (preserve annotations for runtime)
-keep @androidx.compose.runtime.Composable class * { *; }

# Keep Compose UI classes
-keep class androidx.compose.ui.** { *; }
-keep class androidx.compose.material3.** { *; }
-keep class androidx.compose.material.icons.** { *; }
-keep class androidx.compose.foundation.** { *; }

# Keep state classes (used by remember/mutableStateOf)
-keep class androidx.compose.runtime.snapshots.** { *; }

# Keep Modifier implementations
-keep class * implements androidx.compose.ui.Modifier { *; }

# Don't warn about Compose preview annotations (not used in release)
-dontwarn androidx.compose.ui.tooling.preview.**

# Keep screen composables
-keep class com.hrvojekatic.laprdus.ui.screens.** { *; }
-keep class com.hrvojekatic.laprdus.ui.theme.** { *; }

# ============================================================================
# DataStore Preferences
# ============================================================================

# Keep DataStore classes
-keep class androidx.datastore.** { *; }

# Keep SettingsRepository and its inner classes
-keep class com.hrvojekatic.laprdus.data.SettingsRepository { *; }
-keep class com.hrvojekatic.laprdus.data.SettingsRepository$* { *; }
-keep class com.hrvojekatic.laprdus.data.SettingsRepository$TTSSettings { *; }

# Keep Preferences keys
-keepclassmembers class * {
    static ** KEY_*;
}

# ============================================================================
# Kotlin
# ============================================================================

# Keep Kotlin metadata (needed for reflection-based APIs)
-keep class kotlin.Metadata { *; }

# Keep Kotlin coroutines
-keep class kotlinx.coroutines.** { *; }
-dontwarn kotlinx.coroutines.**

# Keep data classes (used throughout the app)
-keep class * implements java.io.Serializable {
    static final long serialVersionUID;
    private static final java.io.ObjectStreamField[] serialPersistentFields;
    private void writeObject(java.io.ObjectOutputStream);
    private void readObject(java.io.ObjectInputStream);
    java.lang.Object writeReplace();
    java.lang.Object readResolve();
}

# Keep companion objects
-keepclassmembers class ** {
    public static ** Companion;
}

# ============================================================================
# Android Framework Components
# ============================================================================

# Keep Activities, Services, and Application
-keep public class * extends android.app.Activity
-keep public class * extends android.app.Application
-keep public class * extends android.app.Service
-keep public class * extends android.content.BroadcastReceiver
-keep public class * extends android.content.ContentProvider

# Keep View constructors (for XML inflation)
-keepclassmembers class * extends android.view.View {
    public <init>(android.content.Context);
    public <init>(android.content.Context, android.util.AttributeSet);
    public <init>(android.content.Context, android.util.AttributeSet, int);
}

# Keep onClick handlers
-keepclassmembers class * extends android.app.Activity {
    public void *(android.view.View);
}

# Keep Parcelable implementations
-keep class * implements android.os.Parcelable {
    public static final android.os.Parcelable$Creator *;
}

# Keep R class references
-keepclassmembers class **.R$* {
    public static <fields>;
}

# ============================================================================
# Aggressive Obfuscation (where safe)
# ============================================================================

# Use aggressive obfuscation
-repackageclasses ''
-allowaccessmodification

# Optimize bytecode
-optimizationpasses 5

# Remove debug info for smaller size and harder reverse engineering
-renamesourcefileattribute SourceFile
-keepattributes SourceFile,LineNumberTable

# Remove logging in release builds (optional - uncomment for production)
# -assumenosideeffects class android.util.Log {
#     public static boolean isLoggable(java.lang.String, int);
#     public static int v(...);
#     public static int d(...);
#     public static int i(...);
# }

# ============================================================================
# Warnings Suppression
# ============================================================================

# Don't warn about missing classes that are not used
-dontwarn org.bouncycastle.**
-dontwarn org.conscrypt.**
-dontwarn org.openjsse.**
