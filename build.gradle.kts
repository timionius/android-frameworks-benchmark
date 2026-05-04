import com.android.build.api.dsl.LibraryExtension
import com.android.build.api.dsl.ApplicationExtension
import org.jlleitschuh.gradle.ktlint.KtlintExtension

plugins {
    alias(libs.plugins.android.application) apply false
    alias(libs.plugins.android.library) apply false
    alias(libs.plugins.compose.compiler) apply false
    alias(libs.plugins.kotlin.android) apply false
    alias(libs.plugins.kotlin.kapt) apply false
    alias(libs.plugins.ktlint) apply false
}

fun Any.configureLint() {
    val lint = when (this) {
        is LibraryExtension -> lint
        is ApplicationExtension -> lint
        else -> null
    }
    lint?.apply {
        abortOnError = true
        baseline = file("lint-baseline.xml")
        xmlReport = false
        htmlReport = false
    }
}

subprojects {
    apply(plugin = "org.jlleitschuh.gradle.ktlint")

    extensions.configure<KtlintExtension> {
        android = false
        outputToConsole = true
    }

    pluginManager.withPlugin("com.android.library") { configureLint() }
    pluginManager.withPlugin("com.android.application") { configureLint() }
}
