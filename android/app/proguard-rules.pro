# Clavi Keyboard ProGuard rules

# Keep all Clavi classes
-keep class com.clavi.keyboard.** { *; }

# ML Kit Translate — keep reflection-accessed classes and native bridges
-keep class com.google.mlkit.** { *; }
-keep class com.google.android.gms.internal.mlkit_translate.** { *; }
-dontwarn com.google.mlkit.**

# Google Play Services (GMS) Tasks API used by ML Kit
-keep class com.google.android.gms.tasks.** { *; }
-dontwarn com.google.android.gms.**

# Keep InputMethodService and related Android framework classes
-keep class * extends android.inputmethodservice.InputMethodService { *; }
-keep class * extends android.view.View { *; }

# Kotlin
-keepattributes *Annotation*
-keep class kotlin.** { *; }
-dontwarn kotlin.**
