cmake_minimum_required(VERSION 3.22.1)

project(pixelsampler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

add_library(pixelsampler SHARED
        pixel_sampler.cpp
        choreographer_callback.cpp
        view_locator.cpp
        jni_helpers.cpp
)

target_include_directories(pixelsampler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Android libraries
find_library(log-lib log)
find_library(android-lib android)
find_library(mediandk-lib mediandk)   # ← This was missing!

target_link_libraries(pixelsampler
        PRIVATE
        ${log-lib}
        ${android-lib}
        ${mediandk-lib}
)

target_compile_options(pixelsampler PRIVATE
        -Wall -Wextra -Wpedantic -Werror=return-type
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(pixelsampler PRIVATE -O3 -DNDEBUG -ffast-math -flto)
    target_link_options(pixelsampler PRIVATE -flto)
else()
    target_compile_options(pixelsampler PRIVATE -O0 -g -DDEBUG)
endif()