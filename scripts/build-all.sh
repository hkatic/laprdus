#!/bin/bash
# =============================================================================
# LaprdusTTS Master Build Script
# =============================================================================
# This script automates the complete build process for all platforms.
# It ensures voice data, dictionaries, and libraries are built and copied
# in the correct order.
#
# The engine and every program are built by Cargo; `cargo xtask` does the
# steps Cargo does not do by itself (voice data, staging, installation).
#
# Usage:
#   ./scripts/build-all.sh                    # Build all platforms
#   ./scripts/build-all.sh sapi5              # Build Windows SAPI5 only
#   ./scripts/build-all.sh nvda               # Build NVDA addon only
#   ./scripts/build-all.sh android            # Build Android APK only
#   ./scripts/build-all.sh linux              # Build Linux only
#   ./scripts/build-all.sh macos              # Build macOS only
#   ./scripts/build-all.sh macos-release      # macOS release: signed, notarized DMG + CLI tarball
#   ./scripts/build-all.sh voice-data         # Generate voice data only
#
# =============================================================================

set -e  # Exit on error

# Project root directory (script is in scripts/)
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Helper functions
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Check prerequisites
check_prerequisites() {
    log_info "Checking prerequisites..."

    # Check Cargo
    if ! command -v cargo &> /dev/null; then
        log_error "Cargo not found. Install Rust from https://rustup.rs"
        exit 1
    fi

    log_success "Prerequisites check passed"
}

# Detect the host so we only attempt builds this machine can actually do.
detect_host() {
    case "$(uname -s)" in
        Darwin)             HOST_PLATFORM="macos"   ;;
        Linux)              HOST_PLATFORM="linux"   ;;
        MINGW*|MSYS*|CYGWIN*) HOST_PLATFORM="windows" ;;
        *)
            if [[ -n "$WINDIR" ]]; then
                HOST_PLATFORM="windows"
            else
                HOST_PLATFORM="linux"
            fi
            ;;
    esac
}

# Where Cargo puts what it builds for the host.
target_dir() {
    echo "${CARGO_TARGET_DIR:-$PROJECT_ROOT/target}/release"
}

# Step 1: Generate voice data (pack phonemes into .bin files)
generate_voice_data() {
    log_info "Step 1: Generating voice data..."

    # Packs data/<language>/<Voice>/*.wav into data/voices/<Voice>.bin and
    # stages the dictionaries in data/dictionary.
    cargo xtask data

    # Verify voice data was created
    if [[ -f "$PROJECT_ROOT/data/voices/Josip.bin" ]] && [[ -f "$PROJECT_ROOT/data/voices/Vlado.bin" ]]; then
        log_success "Voice data generated: Josip.bin, Vlado.bin"
    else
        log_error "Failed to generate voice data"
        exit 1
    fi
}

# Step 2: Build the Windows programs (both architectures): the SAPI5 DLLs,
# the CLI and the Configurator, staged in dist/windows-x64 and dist/windows-x86
build_sapi5_dlls() {
    log_info "Step 2: Building Windows SAPI5 DLLs..."

    # Needs the MSVC build tools and both targets:
    #   rustup target add x86_64-pc-windows-msvc i686-pc-windows-msvc
    cargo xtask windows

    # Verify DLLs
    if [[ -f "$PROJECT_ROOT/dist/windows-x64/laprd64.dll" ]] && \
       [[ -f "$PROJECT_ROOT/dist/windows-x86/laprd32.dll" ]]; then
        log_success "SAPI5 DLLs built successfully"
    else
        log_error "Failed to build SAPI5 DLLs"
        exit 1
    fi
}

# Step 3: Build SAPI5 Installer
build_sapi5_installer() {
    log_info "Step 3: Building SAPI5 Installer..."

    # Check for InnoSetup
    ISCC="/c/Program Files (x86)/Inno Setup 6/ISCC.exe"
    if [[ ! -f "$ISCC" ]]; then
        log_warning "InnoSetup not found at $ISCC"
        log_warning "Skipping SAPI5 installer build"
        return
    fi

    "$ISCC" "$PROJECT_ROOT/installers/windows/laprdus_sapi5.iss"

    if [[ -n "$(ls "$PROJECT_ROOT/installers/windows/Output/Laprdus_SAPI5_Setup_"*.exe 2>/dev/null)" ]]; then
        log_success "SAPI5 Installer built successfully"
    else
        log_error "Failed to build SAPI5 Installer"
        exit 1
    fi
}

# Step 4: Build NVDA Addon
# The add-on has its own SCons build (the NVDA add-on template); it takes the
# DLLs and the Configurator from dist/windows-*, the voice data from
# data/voices and the dictionaries from data/dictionary by itself.
build_nvda_addon() {
    log_info "Step 4: Building NVDA Addon..."

    if ! command -v scons &> /dev/null; then
        log_error "SCons not found (the NVDA add-on is built with it). Install with: pip install scons"
        exit 1
    fi

    cd "$PROJECT_ROOT/nvda-addon"
    scons
    cd "$PROJECT_ROOT"

    # Find the built addon
    ADDON_FILE=$(ls "$PROJECT_ROOT/nvda-addon/laprdus-"*.nvda-addon 2>/dev/null | head -1)
    if [[ -f "$ADDON_FILE" ]]; then
        log_success "NVDA Addon built: $(basename "$ADDON_FILE")"
    else
        log_error "Failed to build NVDA Addon"
        exit 1
    fi
}

# Step 5: Build Linux components (CLI, Speech Dispatcher module, C API library)
build_linux() {
    log_info "Step 5: Building Linux components..."

    cargo xtask build

    local built
    built="$(target_dir)"
    if [[ -f "$built/liblaprdus_capi.so" ]] && [[ -f "$built/laprdus" ]] && [[ -f "$built/sd_laprdus" ]]; then
        log_success "Linux components built successfully in $built"
        log_info "Install them with: sudo cargo xtask install --prefix /usr"
    else
        log_warning "Some Linux components may not have been built"
    fi
}

# Build macOS components (C API library + CLI) for the host architecture
build_macos() {
    log_info "Building macOS components..."

    cargo xtask build

    local built
    built="$(target_dir)"
    if [[ -f "$built/liblaprdus_capi.dylib" ]] && [[ -f "$built/laprdus" ]]; then
        log_success "macOS components built successfully in $built"
    else
        log_warning "Some macOS components may not have been built"
    fi
}

# macOS release packages for distribution outside the Mac App Store: the
# notarized app DMG and the universal command-line tools tarball, in dist/macos.
# Everything is in scripts/build-macos-release.sh; extra arguments are passed
# through (--development, --skip-notarization, --identity, ...).
build_macos_release() {
    log_info "Building the macOS release packages..."

    if [[ "$HOST_PLATFORM" != "macos" ]]; then
        log_error "The macOS release can only be built on a Mac with Xcode"
        exit 1
    fi

    "$PROJECT_ROOT/scripts/build-macos-release.sh" "$@"
}

# Step 6: Build Android APK
# Gradle packs the voice data and builds the native library itself (cargo
# xtask data, cargo ndk).
build_android() {
    log_info "Step 6: Building Android APK..."

    # Check for JAVA_HOME
    if [[ -z "$JAVA_HOME" ]]; then
        # Try common locations
        if [[ -d "/c/Program Files/Android/Android Studio/jbr" ]]; then
            export JAVA_HOME="/c/Program Files/Android/Android Studio/jbr"
        elif [[ -d "/Applications/Android Studio.app/Contents/jbr/Contents/Home" ]]; then
            export JAVA_HOME="/Applications/Android Studio.app/Contents/jbr/Contents/Home"
        else
            log_warning "JAVA_HOME not set. Android build may fail."
        fi
    fi

    cd "$PROJECT_ROOT/src/platform/android"
    ./gradlew assembleDebug
    cd "$PROJECT_ROOT"

    if [[ -f "$PROJECT_ROOT/src/platform/android/app/build/outputs/apk/debug/app-debug.apk" ]]; then
        log_success "Android APK built successfully"
    else
        log_error "Failed to build Android APK"
        exit 1
    fi
}

# Main entry point
main() {
    echo "=============================================="
    echo "  LaprdusTTS Master Build Script"
    echo "=============================================="
    echo ""

    detect_host
    check_prerequisites
    log_info "Host platform: $HOST_PLATFORM"

    TARGET="${1:-all}"
    [[ $# -gt 0 ]] && shift

    case "$TARGET" in
        voice-data)
            generate_voice_data
            ;;
        sapi5)
            generate_voice_data
            build_sapi5_dlls
            # The installer carries the NVDA add-on: the add-on is built first.
            build_nvda_addon
            build_sapi5_installer
            ;;
        nvda)
            generate_voice_data
            build_sapi5_dlls
            build_nvda_addon
            ;;
        android)
            generate_voice_data
            build_android
            ;;
        linux)
            generate_voice_data
            build_linux
            ;;
        macos)
            generate_voice_data
            build_macos
            ;;
        macos-release)
            build_macos_release "$@"
            ;;
        all)
            # Only build what this host can: the SAPI5/NVDA targets need MSVC,
            # and each of linux/macos needs its own toolchain. Skipping is a
            # warning, not an error, so 'all' stays useful on every platform.
            generate_voice_data
            case "$HOST_PLATFORM" in
                windows)
                    build_sapi5_dlls
                    build_nvda_addon
                    build_sapi5_installer
                    ;;
                linux)
                    build_linux
                    ;;
                macos)
                    build_macos
                    ;;
            esac
            if [[ "$HOST_PLATFORM" != "windows" ]]; then
                log_warning "Skipping SAPI5 and NVDA targets: they require MSVC on Windows"
            fi
            build_android
            ;;
        *)
            log_error "Unknown target: $TARGET"
            echo "Usage: $0 [all|sapi5|nvda|android|linux|macos|macos-release|voice-data]"
            exit 1
            ;;
    esac

    echo ""
    log_success "Build completed successfully!"
}

main "$@"
