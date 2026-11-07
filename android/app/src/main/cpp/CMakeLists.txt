# CMakeLists.txt - LaprdusTTS Android Native Build
# Builds the native TTS engine for Android NDK

cmake_minimum_required(VERSION 3.22.1)
project(laprdus VERSION 2.0.0 LANGUAGES CXX C)

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
    ${LAPRDUS_ROOT}/src/audio/unit_bank.cpp
    ${LAPRDUS_ROOT}/src/audio/psola.cpp
    ${LAPRDUS_ROOT}/src/audio/concat_prosody.cpp
    ${LAPRDUS_ROOT}/src/formant/klatt_synth.cpp
    ${LAPRDUS_ROOT}/src/formant/formant_phonemes.cpp
    ${LAPRDUS_ROOT}/src/formant/formant_lexicon.cpp
    ${LAPRDUS_ROOT}/src/formant/formant_frontend.cpp
    ${LAPRDUS_ROOT}/src/formant/formant_slovenian.cpp
    ${LAPRDUS_ROOT}/src/formant/formant_intonation.cpp
    ${LAPRDUS_ROOT}/src/formant/formant_synthesizer.cpp
    ${LAPRDUS_ROOT}/src/c_api/laprdus_api.cpp
    ${LAPRDUS_ROOT}/src/platform/android/jni_bridge.cpp
)

# Build shared library
add_library(laprdus SHARED ${CORE_SOURCES})

# Compiler definitions
target_compile_definitions(laprdus PRIVATE
    ANDROID
    LAPRDUS_EXPORTS
    LAPRDUS_VERSION_MAJOR=2
    LAPRDUS_VERSION_MINOR=0
    LAPRDUS_VERSION_PATCH=0
    LAPRDUS_VERSION_STRING="2.0.0"
)

# Compiler flags
target_compile_options(laprdus PRIVATE
    -Wall
    -Wextra
    -fvisibility=hidden
)

# Memory safety while testing: debug builds trap out-of-bounds container and
# iterator use in libc++ (vector[], string_view, span, ...) instead of reading
# past the end, so the instrumented tests find such bugs. Release builds stay
# as they are: a trap there would silence the screen reader.
target_compile_definitions(laprdus PRIVATE
    $<$<CONFIG:Debug>:_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE>
)

# Link Android libraries
find_library(log-lib log)
find_library(android-lib android)

target_link_libraries(laprdus
    ${log-lib}
    ${android-lib}
)

# 16 KB page size alignment (Android 15+ devices with 16 KB pages; Google
# Play rejects updates without it from 1 February 2027). NDK r28+ aligns this
# way by default; the flag keeps it explicit.
target_link_options(laprdus PRIVATE "-Wl,-z,max-page-size=16384")
