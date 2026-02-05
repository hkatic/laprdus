#!/bin/bash
# =============================================================================
# LaprdusTTS Master Build Script
# =============================================================================
# This script automates the complete build process for all platforms.
# It ensures voice data, dictionaries, and libraries are built and copied
# in the correct order.
#
# Usage:
#   ./scripts/build-all.sh                    # Build all platforms
#   ./scripts/build-all.sh sapi5              # Build Windows SAPI5 only
#   ./scripts/build-all.sh nvda               # Build NVDA addon only
#   ./scripts/build-all.sh android            # Build Android APK only
#   ./scripts/build-all.sh linux              # Build Linux only
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

    # Check SCons
    if ! command -v scons &> /dev/null; then
        log_error "SCons not found. Install with: pip install scons"
        exit 1
    fi

    # Check for MSVC on Windows
    if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]] || [[ -n "$WINDIR" ]]; then
        if ! command -v cl &> /dev/null; then
            log_warning "MSVC not in PATH. Windows builds may fail."
        fi
    fi

    log_success "Prerequisites check passed"
}

# Step 1: Generate voice data (pack phonemes into .bin files)
generate_voice_data() {
    log_info "Step 1: Generating voice data..."

    # Ensure data/voices directory exists
    mkdir -p "$PROJECT_ROOT/data/voices"

    # Build phoneme packer and pack voice data
    # This builds the packer, then uses it to create .bin files
    scons --platform=linux --build-config=release voice-data 2>/dev/null || \
    scons --platform=windows --arch=x64 --build-config=release voice-data

    # Verify voice data was created
    if [[ -f "$PROJECT_ROOT/data/voices/Josip.bin" ]] && [[ -f "$PROJECT_ROOT/data/voices/Vlado.bin" ]]; then
        log_success "Voice data generated: Josip.bin, Vlado.bin"
    else
        log_error "Failed to generate voice data"
        exit 1
    fi
}

# Step 2: Build Windows SAPI5 DLLs (both architectures)
build_sapi5_dlls() {
    log_info "Step 2: Building Windows SAPI5 DLLs..."

    # Build x64
    log_info "  Building x64 DLL..."
    scons --platform=windows --arch=x64 --build-config=release sapi5

    # Build x86
    log_info "  Building x86 DLL..."
    scons --platform=windows --arch=x86 --build-config=release sapi5

    # Verify DLLs
    if [[ -f "$PROJECT_ROOT/build/windows-x64-release/laprd64.dll" ]] && \
       [[ -f "$PROJECT_ROOT/build/windows-x86-release/laprd32.dll" ]]; then
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

    if [[ -f "$PROJECT_ROOT/installers/windows/Output/Laprdus_SAPI5_Setup_1.0.0.exe" ]]; then
        log_success "SAPI5 Installer built successfully"
    else
        log_error "Failed to build SAPI5 Installer"
        exit 1
    fi
}

# Step 4: Copy DLLs to NVDA addon directory
copy_dlls_to_nvda() {
    log_info "Step 4: Copying DLLs to NVDA addon directory..."

    NVDA_DIR="$PROJECT_ROOT/nvda-addon/addon/synthDrivers/laprdus"
    mkdir -p "$NVDA_DIR"

    cp "$PROJECT_ROOT/build/windows-x64-release/laprd64.dll" "$NVDA_DIR/"
    cp "$PROJECT_ROOT/build/windows-x86-release/laprd32.dll" "$NVDA_DIR/"

    log_success "DLLs copied to NVDA addon"
}

# Step 5: Build NVDA Addon
build_nvda_addon() {
    log_info "Step 5: Building NVDA Addon..."

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

# Step 6: Build Linux components
build_linux() {
    log_info "Step 6: Building Linux components..."

    scons --platform=linux --arch=x64 --build-config=release linux-all

    if [[ -f "$PROJECT_ROOT/build/linux-x64-release/liblaprdus.so" ]] && \
       [[ -f "$PROJECT_ROOT/build/linux-x64-release/laprdus" ]]; then
        log_success "Linux components built successfully"
    else
        log_warning "Some Linux components may not have been built"
    fi
}

# Step 7: Build Android APK
build_android() {
    log_info "Step 7: Building Android APK..."

    # Check for JAVA_HOME
    if [[ -z "$JAVA_HOME" ]]; then
        # Try common locations
        if [[ -d "/c/Program Files/Android/Android Studio/jbr" ]]; then
            export JAVA_HOME="/c/Program Files/Android/Android Studio/jbr"
        else
            log_warning "JAVA_HOME not set. Android build may fail."
        fi
    fi

    cd "$PROJECT_ROOT/android"
    ./gradlew assembleDebug
    cd "$PROJECT_ROOT"

    if [[ -f "$PROJECT_ROOT/android/app/build/outputs/apk/debug/app-debug.apk" ]]; then
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

    check_prerequisites

    TARGET="${1:-all}"

    case "$TARGET" in
        voice-data)
            generate_voice_data
            ;;
        sapi5)
            generate_voice_data
            build_sapi5_dlls
            build_sapi5_installer
            ;;
        nvda)
            generate_voice_data
            build_sapi5_dlls
            copy_dlls_to_nvda
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
        all)
            generate_voice_data
            build_sapi5_dlls
            build_sapi5_installer
            copy_dlls_to_nvda
            build_nvda_addon
            build_linux
            build_android
            ;;
        *)
            log_error "Unknown target: $TARGET"
            echo "Usage: $0 [all|sapi5|nvda|android|linux|voice-data]"
            exit 1
            ;;
    esac

    echo ""
    log_success "Build completed successfully!"
}

main "$@"
