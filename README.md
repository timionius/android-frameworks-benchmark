# 🚀 PixelSampler SDK (Android)

PixelSampler SDK is a lightweight, low-overhead performance monitoring tool for Android. It captures the true visual app-start time by monitoring the screen's frame production pipeline. The SDK supports Jetpack Compose, XML Views, React Native, and Flutter.

# 📱 Device Compatibility & Testing

⚠️ **Important Note**: This SDK has been tested on Redmi Note 8 Pro. Other devices may exhibit different behavior due to variations in:

- VirtualDisplay implementation
- Graphics driver behavior (Gralloc, Mali, Adreno)
- Android vendor modifications

# ✨ Key Features

- 🔌 Simple Integration: Minimal setup — just initialize, start, and receive results
- ⚡ Ultra-Low Overhead: Native C++ processing with Choreographer vsync callbacks
- 🎯 Frame Pipeline Monitoring: Detects stability when the VirtualDisplay stops producing frames (3 consecutive empty buffers)
- 📱 Multi-Framework Support: Works with Compose UI, XML Views, React Native, Flutter
- 📊 Console Logging: Automatic benchmark events to Logcat
- 🛡️ Permission Management: Handles MediaProjection permission with foreground service (required for Android 14+)

# 📦 Installation

TBD

# 🛠 Integration

## 1. Initialize SDK
The SDK auto-initializes when the PixelSampler object is first accessed. APP_START is logged immediately and native library is loaded. The best place is Activity init block
```kotlin
class Application : Application() {
    init {
        PixelSampler.init()
    }
}
```
## 2. Start Sampling
Call start() when your UI is ready (e.g., in Activity.onCreate()):
```kotlin
class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        PixelSampler.start(this)
```
## 3. Receive Results
The SDK logs benchmark events directly to Logcat by '[BENCHMARK]' filter:
```shell
✅ [BENCHMARK] APP_START: 0.000ms
✅ [BENCHMARK] FRAMEWORK_ENTRY: 342.56ms
✅ [BENCHMARK] RENDER_COMPLETE: 1247.89ms
```

# 🧪 Testing with ADB (Permission Bypass)

For automated testing, you should pre-grant permissions via ADB:
```bash
# 1. Build the demo APK
./gradlew :examples:compose:assembleDebug

# 2. Install with permissions (-g grants all runtime permissions, -t allows test packages)
adb install -g -t path/to/your-app-debug.apk

# 3. Grant MediaProjection permission directly (bypasses dialog)
adb shell cmd appops set your.package.name PROJECT_MEDIA allow

# 4. Launch the app
adb shell am start -n your.package.name/.MainActivity
```
# 🔍 How It Works

1. T=0: APP_START captured when PixelSampler object is first accessed (monotonic clock via SystemClock.elapsedRealtimeNanos()).
2. Permission Request: When start() is called, the SDK:
   - Starts a foreground service (required for Android 14+)
   - Requests MediaProjection permission via system dialog
   - Once granted, calls native code
3. Native Frame Pipeline (C++):
   - Creates AImageReader (100x100, RGBA_8888)
   - Creates VirtualDisplay with AUTO_MIRROR flag
   - Registers Choreographer callback for vsync synchronization
   - Each frame: attempts to acquire latest image from AImageReader
4. Stability Detection:
   - Tracks g_lastAnimation timestamp when frames are available
   - Counts consecutive AMEDIA_IMGREADER_NO_BUFFER_AVAILABLE responses
   - After 3 consecutive empty buffers, declares scene stable
   - Notifies Kotlin with g_lastAnimation (time of last actual frame)
5. Result: RENDER_COMPLETE logged with total elapsed time.

# 🔧 Requirements

- Android 12.0 (API 29), API 24+ supported but not tested
- Kotlin 1.9+
- Android Gradle Plugin 9.5+
- NDK 28.2.13676358
- CMake 3.22+

# Build Requirements

⚠️ **Important Note**: This SDK has been built and tested in **macOS** environment only.
Windows and Linux builds may require additional configuration.

Set ANDROID_HOME environment variable:
```bash
export ANDROID_HOME=/Users/$USER/Library/Android/sdk
```

# 📝 License

MIT License

Copyright (c) 2026 Dmitrii Nikishov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions...

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
