plugins {
    alias(libs.plugins.android.library)
    // id("maven-publish")   // uncomment when needed
}

android {
    namespace = "io.timon.android.pixelsampler"
    compileSdk =
        libs.versions.compileSdk
            .get()
            .toInt()

    ndkVersion =
        libs.versions.ndkVersion
            .get()

    val sdkPath: String? = System.getenv("ANDROID_HOME")
    val osName = System.getProperty("os.name").lowercase()
    val hostTag = if (osName.contains("mac")) "darwin-x86_64" else "linux-x86_64"
    val clangTidyPath = "$sdkPath/ndk/$ndkVersion/toolchains/llvm/prebuilt/$hostTag/bin/clang-tidy"

    defaultConfig {
        minSdk =
            libs.versions.minSdk
                .get()
                .toInt()

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        externalNativeBuild {
            cmake {
                arguments +=
                    listOf(
                        "-DANDROID_STL=c++_shared",
                        "-DANDROID_ARM_NEON=TRUE",
                        "-DCMAKE_CXX_CLANG_TIDY=$clangTidyPath;-checks=-*,bugprone-*,modernize-*,performance-*,readability-*",
                    )
                cppFlags += "-std=c++17"
            }
        }

        ndk {
            abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86_64")
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro",
            )

            externalNativeBuild {
                cmake {
                    arguments += "-DCMAKE_BUILD_TYPE=Release"
                }
            }
        }

        debug {
            externalNativeBuild {
                cmake {
                    arguments += "-DCMAKE_BUILD_TYPE=Debug"
                }
            }
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    packaging {
        resources {
            excludes += "/META-INF/{AL2.0,LGPL2.1}"
        }
    }

    publishing {
        singleVariant("release") {
            withSourcesJar()
            withJavadocJar()
        }
    }
}

dependencies {
    implementation(libs.androidx.activity.ktx)
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.lifecycle.runtime.ktx)

    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.test.junit)
    androidTestImplementation(libs.androidx.test.espresso)
}
