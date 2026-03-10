# ProGuard/R8 rules for Laprdus TTS
# Only rules that are NOT already provided by library consumer rules.
# Compose, Hilt, DataStore, Coroutines, and AndroidX ship their own rules.

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

-keep class * extends android.speech.tts.TextToSpeechService {
    public *;
    protected *;
}

# ============================================================================
# Dictionary Data Classes - Field names preserved for JSON parsing
# ============================================================================

-keep class com.hrvojekatic.laprdus.data.DictionaryEntry {
    <init>(...);
    *;
}

-keep enum com.hrvojekatic.laprdus.data.DictionaryType {
    *;
}

# ============================================================================
# Compose preview annotations (not used in release)
# ============================================================================

-dontwarn androidx.compose.ui.tooling.preview.**

# ============================================================================
# Aggressive Obfuscation (where safe)
# ============================================================================

-repackageclasses ''
-allowaccessmodification

# Preserve stack traces for crash reporting
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

-dontwarn org.bouncycastle.**
-dontwarn org.conscrypt.**
-dontwarn org.openjsse.**
