# Android Frameworks Benchmark

**High-precision render time measurement SDK** for Android using NDK pixel sampling.

Measures the real visual render completion time across **Jetpack Compose, Flutter, React Native, and traditional XML Views**.

---

## 🎯 Purpose

This SDK helps developers and framework authors objectively compare **first meaningful paint** and **visual stability** time across different UI frameworks by sampling the center pixel of the root view until the frame stabilizes.

## ✨ Key Features

- **Ultra-low overhead** — Samples only every 5th frame (~12 FPS) to minimize impact
- **Zero-copy pixel capture** — Uses `ImageReader` + `AHardwareBuffer` (modern, fast)
- **Automatic framework detection** — Detects Compose, Flutter, React Native, or XML
- **Frame stability detection** — Requires 4 consecutive identical frames before reporting
- **Choreographer-driven** — Perfectly aligned with vsync
- **Pure NDK core** — High-performance C++ implementation
- **Kotlin-first API** — Simple callback-based usage

## 📊 Supported Frameworks

| Framework         | Detection                     | Status     |
|-------------------|-------------------------------|------------|
| Jetpack Compose   | `AndroidComposeView`          | ✅ Full    |
| Flutter           | `FlutterView` / Impeller      | ✅ Full    |
| React Native      | `ReactRootView`               | ✅ Full    |
| Traditional Views | `DecorView`                   | ✅ Full    |

---

## 📦 Installation

TBD
