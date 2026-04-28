# Android Frameworks Benchmark

A high-precision performance monitoring SDK for Android that measures first-frame render time across different UI frameworks (Jetpack Compose, Flutter, React Native, and XML Views) using pixel sampling at 60fps.

## 🎯 Purpose

Compare the actual render performance of different Android UI frameworks by measuring the time from window creation to visual stability using a non-intrusive pixel-sampling technique.

## 📊 Supported Frameworks

| Framework | Detection Method | Sampling Technique |
|-----------|-----------------|-------------------|
| **Jetpack Compose** | ComposeView detection | 1x1 pixel sampling |
| **Flutter** | FlutterView detection | 1x1 pixel sampling |
| **React Native** | ReactRootView detection | 1x1 pixel sampling |
| **XML Views** | DecorView detection | 1x1 pixel sampling |

## ✨ Key Features

- **Low overhead** - Samples every 5th frame (≈83ms) using 1x1 pixel capture
- **60fps sampling** - Time-based sampling consistent across all devices
- **Framework detection** - Automatically identifies the UI framework in use
- **NDK implementation** - High-performance native code for pixel capture
- **4-frame stability** - Waits for 4 consecutive unchanged frames before completion
- **Cross-framework** - Works with any Android UI framework

## 📦 Installation

TBD
