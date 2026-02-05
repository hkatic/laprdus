# CMakeLists.txt - LaprdusTTS Android Native Build
# Builds the native TTS engine for Android NDK

cmake_minimum_required(VERSION 3.22.1)
project(laprdus VERSION 1.0.0 LANGUAGES CXX C)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define source root relative to CMakeLists.txt
# Path: android/app/src/main/cpp -> project root (5 levels up)
get_filename_component(LAPRDUS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)

message(STATUS "LaprdusTTS root: ${LAPRDUS_ROOT}")

# Include directories
include_directories(
    ${LAPRDUS_ROOT}/include
    ${LAPRDUS_ROOT}/src
    ${LAPRDUS_ROOT}/src/audio/sonic
)

# Core library sources
set(CORE_SOURCES
    ${LAPRDUS_ROOT}/src/core/phoneme_mapper.cpp
    ${LAPRDUS_ROOT}/src/core/croatian_numbers.cpp
    ${LAPRDUS_ROOT}/src/core/inflection.cpp
    ${LAPRDUS_ROOT}/src/core/tts_engine.cpp
    ${LAPRDUS_ROOT}/src/core/voice_registry.cpp
    ${LAPRDUS_ROOT}/src/core/pronunciation_dict.cpp
    ${LAPRDUS_ROOT}/src/core/spelling_dict.cpp
    ${LAPRDUS_ROOT}/src/core/emoji_dict.cpp
    ${LAPRDUS_ROOT}/src/core/user_config.cpp
    ${LAPRDUS_ROOT}/src/audio/phoneme_data.cpp
    ${LAPRDUS_ROOT}/src/audio/audio_synthesizer.cpp
    ${LAPRDUS_ROOT}/src/audio/sonic_processor.cpp
    ${LAPRDUS_ROOT}/src/audio/sonic/sonic.c
    ${LAPRDUS_ROOT}/src/audio/formant_pitch.cpp
    ${LAPRDUS_ROOT}/src/c_api/laprdus_api.cpp
    ${LAPRDUS_ROOT}/src/platform/android/jni_bridge.cpp
)

# Build shared library
add_library(laprdus SHARED ${CORE_SOURCES})

# Suppress warnings for third-party sonic library (external code)
# The unused parameter warnings are from deprecated/stub functions in the upstream library
set_source_files_properties(
    ${LAPRDUS_ROOT}/src/audio/sonic/sonic.c
    PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter"
)

# Compiler definitions
target_compile_definitions(laprdus PRIVATE
    ANDROID
    LAPRDUS_EXPORTS
    LAPRDUS_VERSION_MAJOR=1
    LAPRDUS_VERSION_MINOR=0
    LAPRDUS_VERSION_PATCH=0
    LAPRDUS_VERSION_STRING="1.0.0"
)

# Compiler flags
target_compile_options(laprdus PRIVATE
    -Wall
    -Wextra
    -fvisibility=hidden
)

# Link Android libraries
find_library(log-lib log)
find_library(android-lib android)

target_link_libraries(laprdus
    ${log-lib}
    ${android-lib}
)

# 16KB page size alignment for Android 15+ compatibility
# Required starting November 2025 for Google Play submissions
target_link_options(laprdus PRIVATE "-Wl,-z,max-page-size=16384")
