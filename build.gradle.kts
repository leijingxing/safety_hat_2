// Top-level build file where you can add configuration options common to all sub-projects/modules.
plugins {
    alias(libs.plugins.android.application) apply false
    alias(libs.plugins.kotlin.android) apply false
    alias(libs.plugins.kotlin.compose) apply false
}

// Extra properties for legacy Groovy modules (e.g., libuvccamera)
extra["versionCompiler"] = 36
extra["versionTarget"] = 36
extra["versionMin"] = 30
extra["compileSdkVersion"] = 36
extra["targetSdkVersion"] = 36
extra["buildToolsVersion"] = "36.0.0"
extra["minSdkVersion"] = 30
extra["javaSourceCompatibility"] = JavaVersion.VERSION_1_8
extra["javaTargetCompatibility"] = JavaVersion.VERSION_1_8
