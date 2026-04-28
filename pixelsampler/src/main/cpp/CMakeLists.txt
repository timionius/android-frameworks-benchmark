cmake_minimum_required(VERSION 3.22.1)

# Project name
project("pixelsampler")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set optimization flags for release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

# Source files
set(SOURCES
        pixel_sampler.cpp
        choreographer_callback.cpp
        view_locator.cpp
        jni_helpers.cpp
)

# Create shared library
add_library(pixelsampler SHARED ${SOURCES})

# Include directories
target_include_directories(pixelsampler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Find required Android libraries
find_library(log-lib log)           # Android logging
find_library(android-lib android)   # Android system
find_library(nativewindow-lib nativewindow)  # Graphics/native window

# Link libraries
target_link_libraries(pixelsampler
        ${log-lib}
        ${android-lib}
        ${nativewindow-lib}
)

# Link JNI (automatically available)
# JNI headers are included automatically by the Android Gradle plugin