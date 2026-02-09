#!/bin/bash
#
# Build LaprdusTTS generic Linux tarball package
#
# Usage: ./build-tarball.sh [version]
#
# This creates a tar.xz archive suitable for manual installation
# on any Linux distribution.

set -e

VERSION="${1:-1.0.0}"
PACKAGE_NAME="laprdus-${VERSION}-linux-x86_64"
BUILD_DIR="$(pwd)/build-tarball"
OUTPUT_DIR="$(pwd)"

# Source directory (project root)
PROJECT_ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"

echo "Building LaprdusTTS ${VERSION} tarball..."
echo "Project root: ${PROJECT_ROOT}"

# Clean previous build
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}/${PACKAGE_NAME}"

# Build the project
cd "${PROJECT_ROOT}"
scons --platform=linux --build-config=release linux-all

# Create package structure
PKG="${BUILD_DIR}/${PACKAGE_NAME}"
mkdir -p "${PKG}/bin"
mkdir -p "${PKG}/lib"
mkdir -p "${PKG}/share/laprdus"
mkdir -p "${PKG}/share/doc/laprdus"
mkdir -p "${PKG}/include/laprdus"
mkdir -p "${PKG}/lib/speech-dispatcher-modules"
mkdir -p "${PKG}/etc/speech-dispatcher/modules"

# Copy files
echo "Copying files..."

# Library (versioned with symlinks)
cp build/linux-x64-release/liblaprdus.so "${PKG}/lib/liblaprdus.so.${VERSION}"
cd "${PKG}/lib"
ln -sf "liblaprdus.so.${VERSION}" "liblaprdus.so.1"
ln -sf "liblaprdus.so.1" "liblaprdus.so"
cd "${PROJECT_ROOT}"

# CLI
cp build/linux-x64-release/laprdus "${PKG}/bin/"

# Voice data
cp build/linux-x64-release/Josip.bin "${PKG}/share/laprdus/"
cp build/linux-x64-release/Vlado.bin "${PKG}/share/laprdus/"

# Dictionaries
cp data/dictionary/internal.json "${PKG}/share/laprdus/"
cp data/dictionary/spelling.json "${PKG}/share/laprdus/"
cp data/dictionary/emoji.json "${PKG}/share/laprdus/"

# Speech Dispatcher module (if built)
if [ -f build/linux-x64-release/sd_laprdus ]; then
    cp build/linux-x64-release/sd_laprdus "${PKG}/lib/speech-dispatcher-modules/"
    cp src/platform/linux/speechd/laprdus.conf "${PKG}/etc/speech-dispatcher/modules/"
fi

# Headers
cp include/laprdus/laprdus_api.h "${PKG}/include/laprdus/"
cp include/laprdus/types.hpp "${PKG}/include/laprdus/"
cp include/laprdus/laprdus.hpp "${PKG}/include/laprdus/"

# Documentation
cp readme.md "${PKG}/share/doc/laprdus/"
[ -f LICENSE ] && cp LICENSE "${PKG}/share/doc/laprdus/"

# Create install script
cat > "${PKG}/install.sh" << 'INSTALL_EOF'
#!/bin/bash
#
# LaprdusTTS Installation Script
#

set -e

PREFIX="${1:-/usr/local}"
SPEECHD_CONF="/etc/speech-dispatcher/speechd.conf"
MARKER_START="# BEGIN LAPRDUS TTS"
MARKER_END="# END LAPRDUS TTS"

echo "Installing LaprdusTTS to ${PREFIX}..."

# Check for root if installing to system directories
if [[ "${PREFIX}" == "/usr" || "${PREFIX}" == "/usr/local" ]]; then
    if [[ $EUID -ne 0 ]]; then
        echo "Error: Installing to ${PREFIX} requires root privileges."
        echo "Run: sudo ./install.sh ${PREFIX}"
        exit 1
    fi
fi

# Detect the correct library directory.
# Debian/Ubuntu use multiarch paths (e.g. /usr/lib/x86_64-linux-gnu/).
# Fedora/RHEL use /usr/lib64/ for 64-bit.
detect_lib_dir() {
    local prefix="$1"

    # For /usr prefix, try to use the distro's native library path
    if [[ "${prefix}" == "/usr" ]]; then
        # Try Debian/Ubuntu multiarch path first
        local multiarch
        multiarch=$(dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null || true)
        if [ -n "$multiarch" ] && [ -d "/usr/lib/${multiarch}" ]; then
            echo "/usr/lib/${multiarch}"
            return
        fi
        # Try Fedora/RHEL lib64
        if [ -d "/usr/lib64" ] && [ "$(uname -m)" = "x86_64" ]; then
            echo "/usr/lib64"
            return
        fi
    fi

    # Default: PREFIX/lib
    echo "${prefix}/lib"
}

LIB_DIR=$(detect_lib_dir "${PREFIX}")

# Create directories
mkdir -p "${PREFIX}/bin"
mkdir -p "${LIB_DIR}"
mkdir -p "${PREFIX}/share/laprdus"
mkdir -p "${PREFIX}/share/doc/laprdus"
mkdir -p "${PREFIX}/include/laprdus"

# Install files
cp -v bin/* "${PREFIX}/bin/"
cp -Pv lib/*.so* "${LIB_DIR}/"
cp -v share/laprdus/* "${PREFIX}/share/laprdus/"
cp -v share/doc/laprdus/* "${PREFIX}/share/doc/laprdus/"
cp -v include/laprdus/* "${PREFIX}/include/laprdus/"

# Install Speech Dispatcher module to the SYSTEM modules directory
# Speech Dispatcher only looks in its own module dir (e.g. /usr/lib64/speech-dispatcher-modules/)
if [ -d lib/speech-dispatcher-modules ] && [ -n "$(ls -A lib/speech-dispatcher-modules/ 2>/dev/null)" ]; then
    # Detect the system speechd module directory
    SD_MODULE_DIR=$(pkg-config --variable=modulebindir speech-dispatcher 2>/dev/null)
    if [ -z "$SD_MODULE_DIR" ]; then
        # Fallback: check common locations
        for dir in /usr/lib64/speech-dispatcher-modules /usr/lib/speech-dispatcher-modules; do
            if [ -d "$dir" ]; then
                SD_MODULE_DIR="$dir"
                break
            fi
        done
    fi
    if [ -n "$SD_MODULE_DIR" ] && { [ -w "$SD_MODULE_DIR" ] || [ $EUID -eq 0 ]; }; then
        mkdir -p "${SD_MODULE_DIR}"
        cp -v lib/speech-dispatcher-modules/* "${SD_MODULE_DIR}/"
    else
        echo "Note: Cannot install Speech Dispatcher module (requires root or speechd not found)"
    fi
fi

# Install Speech Dispatcher config (only if we have permissions)
if [ -d etc/speech-dispatcher/modules ] && [ -n "$(ls -A etc/speech-dispatcher/modules/ 2>/dev/null)" ]; then
    if [ -w /etc/speech-dispatcher/modules ] || [ $EUID -eq 0 ]; then
        mkdir -p /etc/speech-dispatcher/modules
        cp -v etc/speech-dispatcher/modules/* /etc/speech-dispatcher/modules/
    else
        echo "Note: Skipping /etc/speech-dispatcher/modules/ (requires root)"
        echo "      For Speech Dispatcher, run: sudo cp etc/speech-dispatcher/modules/* /etc/speech-dispatcher/modules/"
    fi
fi

# Ensure the library is findable by the dynamic linker
if [ $EUID -eq 0 ]; then
    # Standard paths that ldconfig always searches don't need ld.so.conf.d entries
    case "${LIB_DIR}" in
        /lib|/usr/lib|/lib64|/usr/lib64|/lib/x86_64-linux-gnu|/usr/lib/x86_64-linux-gnu)
            # Built-in or already configured path, no extra config needed
            ;;
        *)
            if [ -d /etc/ld.so.conf.d ]; then
                echo "${LIB_DIR}" > /etc/ld.so.conf.d/laprdus.conf
                echo "Added ${LIB_DIR} to library search path (/etc/ld.so.conf.d/laprdus.conf)"
            fi
            ;;
    esac

    ldconfig
fi

# Install uninstall script to a known location so it persists after tarball cleanup
UNINSTALL_DEST="${PREFIX}/share/laprdus/uninstall.sh"
cat > "${UNINSTALL_DEST}" << 'UNINSTALL_INNER_EOF'
#!/bin/bash
#
# LaprdusTTS Uninstallation Script
#

set -e

# Determine PREFIX from this script's location
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PREFIX="$(cd "${SCRIPT_DIR}/../.." && pwd)"

SPEECHD_CONF="/etc/speech-dispatcher/speechd.conf"
MARKER_START="# BEGIN LAPRDUS TTS"
MARKER_END="# END LAPRDUS TTS"

echo "Uninstalling LaprdusTTS from ${PREFIX}..."

# Check for root if uninstalling from system directories
if [[ "${PREFIX}" == "/usr" || "${PREFIX}" == "/usr/local" ]]; then
    if [[ $EUID -ne 0 ]]; then
        echo "Error: Uninstalling from ${PREFIX} requires root privileges."
        echo "Run: sudo $0"
        exit 1
    fi
fi

# Remove Speech Dispatcher configuration
if [ -f "$SPEECHD_CONF" ]; then
    if grep -q "$MARKER_START" "$SPEECHD_CONF" 2>/dev/null; then
        echo "Removing LaprdusTTS from Speech Dispatcher configuration..."
        sed -i "/$MARKER_START/,/$MARKER_END/d" "$SPEECHD_CONF"
    fi
    # Also remove language-only block if present
    sed -i '/# BEGIN LAPRDUS TTS LANGUAGES/,/# END LAPRDUS TTS LANGUAGES/d' "$SPEECHD_CONF" 2>/dev/null || true
fi

# Remove files
rm -fv "${PREFIX}/bin/laprdus"

# Remove library from all possible locations
rm -fv "${PREFIX}/lib/liblaprdus.so"*
# Debian/Ubuntu multiarch path
multiarch=$(dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null || true)
if [ -n "$multiarch" ]; then
    rm -fv "${PREFIX}/lib/${multiarch}/liblaprdus.so"*
fi
# Fedora lib64 path
rm -fv "${PREFIX}/lib64/liblaprdus.so"* 2>/dev/null || true

# Remove Speech Dispatcher module from system modules directory
SD_MODULE_DIR=$(pkg-config --variable=modulebindir speech-dispatcher 2>/dev/null)
if [ -z "$SD_MODULE_DIR" ]; then
    for dir in /usr/lib64/speech-dispatcher-modules /usr/lib/speech-dispatcher-modules; do
        if [ -d "$dir" ]; then
            SD_MODULE_DIR="$dir"
            break
        fi
    done
fi
if [ -n "$SD_MODULE_DIR" ]; then
    rm -fv "${SD_MODULE_DIR}/sd_laprdus"
fi
# Also remove from prefix in case it was installed there
rm -fv "${PREFIX}/lib/speech-dispatcher-modules/sd_laprdus"

# Remove system config files (only if we have permissions)
if [ -w /etc/speech-dispatcher/modules ] || [ $EUID -eq 0 ]; then
    rm -fv /etc/speech-dispatcher/modules/laprdus.conf
fi

# Remove ldconfig conf if it exists
if [ $EUID -eq 0 ] && [ -f /etc/ld.so.conf.d/laprdus.conf ]; then
    rm -fv /etc/ld.so.conf.d/laprdus.conf
fi

# Remove include files
rm -rfv "${PREFIX}/include/laprdus"

# Remove data and doc directories (removes uninstall script too)
rm -rfv "${PREFIX}/share/doc/laprdus"
rm -rfv "${PREFIX}/share/laprdus"

# Update library cache (only if we have permissions)
if command -v ldconfig &> /dev/null && [ $EUID -eq 0 ]; then
    ldconfig
fi

# Restart Speech Dispatcher so it stops using the removed module
restart_speechd() {
    systemctl try-restart speech-dispatcher 2>/dev/null || true
    for uid in $(loginctl list-users --no-legend 2>/dev/null | awk '{print $1}'); do
        systemctl --user -M "${uid}@" try-restart speech-dispatcher 2>/dev/null || true
    done
}
restart_speechd

echo ""
echo "LaprdusTTS uninstalled."
UNINSTALL_INNER_EOF
chmod +x "${UNINSTALL_DEST}"
echo "Installed uninstall script to ${UNINSTALL_DEST}"

# Restart Speech Dispatcher so it picks up the new module
restart_speechd() {
    systemctl try-restart speech-dispatcher 2>/dev/null || true
    for uid in $(loginctl list-users --no-legend 2>/dev/null | awk '{print $1}'); do
        systemctl --user -M "${uid}@" try-restart speech-dispatcher 2>/dev/null || true
    done
}

# Configure Speech Dispatcher automatically
configure_speechd() {
    if [ ! -f "$SPEECHD_CONF" ]; then
        echo "Note: Speech Dispatcher config not found."
        echo "LaprdusTTS will be available once Speech Dispatcher is configured."
        return 0
    fi

    # Check if we have write permissions
    if [ ! -w "$SPEECHD_CONF" ] && [ $EUID -ne 0 ]; then
        echo "Note: Cannot configure Speech Dispatcher (requires root)"
        return 0
    fi

    # Check if already configured
    if grep -q "$MARKER_START" "$SPEECHD_CONF" 2>/dev/null; then
        echo "LaprdusTTS already configured in Speech Dispatcher."
        return 0
    fi

    # Check for existing manual configuration
    if grep -q 'AddModule.*"laprdus"' "$SPEECHD_CONF" 2>/dev/null; then
        echo "Note: Existing LaprdusTTS module entry found."
        return 0
    fi

    echo "Configuring Speech Dispatcher for LaprdusTTS..."

    # Speech Dispatcher 0.12+ uses module autodetection when no AddModule
    # lines are present. Adding an AddModule disables autodetection and
    # hides other modules (espeak-ng, RHVoice, etc.).
    if grep -q '^[[:space:]]*AddModule' "$SPEECHD_CONF" 2>/dev/null; then
        # Explicit module mode: other modules are listed, add ours too
        cat >> "$SPEECHD_CONF" << 'SPEECHD_EOF'

# BEGIN LAPRDUS TTS
# LaprdusTTS - Croatian/Serbian Text-to-Speech
# Added automatically by package installation
AddModule "laprdus" "sd_laprdus" "laprdus.conf"

# Set LaprdusTTS as default for Croatian and Serbian
LanguageDefaultModule "hr" "laprdus"
LanguageDefaultModule "sr" "laprdus"
LanguageDefaultModule "hr-HR" "laprdus"
LanguageDefaultModule "sr-RS" "laprdus"
# END LAPRDUS TTS
SPEECHD_EOF
    else
        # Autodetection mode (SD 0.12+): do NOT add AddModule
        cat >> "$SPEECHD_CONF" << 'SPEECHD_EOF'

# BEGIN LAPRDUS TTS
# LaprdusTTS - Croatian/Serbian Text-to-Speech
# Module is auto-detected by Speech Dispatcher (no AddModule needed)

# Set LaprdusTTS as default for Croatian and Serbian
LanguageDefaultModule "hr" "laprdus"
LanguageDefaultModule "sr" "laprdus"
LanguageDefaultModule "hr-HR" "laprdus"
LanguageDefaultModule "sr-RS" "laprdus"
# END LAPRDUS TTS
SPEECHD_EOF
    fi

    echo "LaprdusTTS configured in Speech Dispatcher."
}

# Configure Speech Dispatcher if module was installed
if [ -d lib/speech-dispatcher-modules ] && [ -n "$(ls -A lib/speech-dispatcher-modules/ 2>/dev/null)" ]; then
    configure_speechd
fi

# Restart Speech Dispatcher to pick up new module
restart_speechd

echo ""
echo "LaprdusTTS installed successfully!"
echo ""
echo "To test, run:"
echo "  laprdus \"Dobar dan!\""
echo ""
echo "For Orca screen reader users:"
echo "  Speech Dispatcher has been configured and restarted."
echo "  If Orca still doesn't see LaprdusTTS, restart Orca or run:"
echo "    systemctl --user restart speech-dispatcher"
echo ""
echo "To uninstall later, run:"
echo "  sudo ${PREFIX}/share/laprdus/uninstall.sh"
INSTALL_EOF

chmod +x "${PKG}/install.sh"

# Create uninstall script (included in tarball for convenience)
cat > "${PKG}/uninstall.sh" << 'UNINSTALL_EOF'
#!/bin/bash
#
# LaprdusTTS Uninstallation Script
#
# This is a convenience copy. After installation, the authoritative
# uninstall script is at: <PREFIX>/share/laprdus/uninstall.sh
#

set -e

PREFIX="${1:-/usr/local}"
SPEECHD_CONF="/etc/speech-dispatcher/speechd.conf"
MARKER_START="# BEGIN LAPRDUS TTS"
MARKER_END="# END LAPRDUS TTS"

echo "Uninstalling LaprdusTTS from ${PREFIX}..."

# Check for root if uninstalling from system directories
if [[ "${PREFIX}" == "/usr" || "${PREFIX}" == "/usr/local" ]]; then
    if [[ $EUID -ne 0 ]]; then
        echo "Error: Uninstalling from ${PREFIX} requires root privileges."
        echo "Run: sudo ./uninstall.sh ${PREFIX}"
        exit 1
    fi
fi

# Remove Speech Dispatcher configuration
if [ -f "$SPEECHD_CONF" ]; then
    if grep -q "$MARKER_START" "$SPEECHD_CONF" 2>/dev/null; then
        echo "Removing LaprdusTTS from Speech Dispatcher configuration..."
        sed -i "/$MARKER_START/,/$MARKER_END/d" "$SPEECHD_CONF"
    fi
    # Also remove language-only block if present
    sed -i '/# BEGIN LAPRDUS TTS LANGUAGES/,/# END LAPRDUS TTS LANGUAGES/d' "$SPEECHD_CONF" 2>/dev/null || true
fi

# Remove files
rm -fv "${PREFIX}/bin/laprdus"

# Remove library from all possible locations
rm -fv "${PREFIX}/lib/liblaprdus.so"*
# Debian/Ubuntu multiarch path
multiarch=$(dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null || true)
if [ -n "$multiarch" ]; then
    rm -fv "${PREFIX}/lib/${multiarch}/liblaprdus.so"*
fi
# Fedora lib64 path
rm -fv "${PREFIX}/lib64/liblaprdus.so"* 2>/dev/null || true

# Remove Speech Dispatcher module from system modules directory
SD_MODULE_DIR=$(pkg-config --variable=modulebindir speech-dispatcher 2>/dev/null)
if [ -z "$SD_MODULE_DIR" ]; then
    for dir in /usr/lib64/speech-dispatcher-modules /usr/lib/speech-dispatcher-modules; do
        if [ -d "$dir" ]; then
            SD_MODULE_DIR="$dir"
            break
        fi
    done
fi
if [ -n "$SD_MODULE_DIR" ]; then
    rm -fv "${SD_MODULE_DIR}/sd_laprdus"
fi
rm -fv "${PREFIX}/lib/speech-dispatcher-modules/sd_laprdus"
rm -rfv "${PREFIX}/share/laprdus"
rm -rfv "${PREFIX}/share/doc/laprdus"
rm -rfv "${PREFIX}/include/laprdus"

# Remove system config files (only if we have permissions)
if [ -w /etc/speech-dispatcher/modules ] || [ $EUID -eq 0 ]; then
    rm -fv /etc/speech-dispatcher/modules/laprdus.conf
fi

# Remove ldconfig conf if it exists
if [ $EUID -eq 0 ] && [ -f /etc/ld.so.conf.d/laprdus.conf ]; then
    rm -fv /etc/ld.so.conf.d/laprdus.conf
fi

# Update library cache (only if we have permissions)
if command -v ldconfig &> /dev/null && [ $EUID -eq 0 ]; then
    ldconfig
fi

# Restart Speech Dispatcher so it stops using the removed module
restart_speechd() {
    systemctl try-restart speech-dispatcher 2>/dev/null || true
    for uid in $(loginctl list-users --no-legend 2>/dev/null | awk '{print $1}'); do
        systemctl --user -M "${uid}@" try-restart speech-dispatcher 2>/dev/null || true
    done
}
restart_speechd

echo ""
echo "LaprdusTTS uninstalled."
UNINSTALL_EOF

chmod +x "${PKG}/uninstall.sh"

# Create README
cat > "${PKG}/README" << 'README_EOF'
LaprdusTTS - Croatian/Serbian Text-to-Speech Engine
====================================================

This package contains LaprdusTTS, a concatenative text-to-speech
engine for Croatian and Serbian languages.

Installation
------------

1. Extract the archive:
   tar xf laprdus-VERSION-linux-x86_64.tar.xz

2. Install to /usr/local (requires root):
   cd laprdus-VERSION-linux-x86_64
   sudo ./install.sh

3. Or install to a custom location:
   ./install.sh $HOME/.local

Usage
-----

Command-line:
  laprdus "Dobar dan!"
  laprdus -v vlado -r 1.5 "Zdravo svete!"
  laprdus -i document.txt -o speech.wav

Speech Dispatcher (for Orca):
  Speech Dispatcher is configured automatically during installation.
  Just restart Speech Dispatcher after installing:
    systemctl --user restart speech-dispatcher

Voices
------

  josip   - Croatian male adult (default)
  vlado   - Serbian male adult
  detence - Croatian child
  baba    - Croatian female senior
  djed    - Serbian male senior

Uninstallation
--------------

After installation, run:
  sudo /usr/local/share/laprdus/uninstall.sh

Or if you still have the extracted tarball directory:
  cd laprdus-VERSION-linux-x86_64
  sudo ./uninstall.sh

For more information, see the documentation in share/doc/laprdus/
README_EOF

# Create the tarball
cd "${BUILD_DIR}"
echo "Creating tarball..."
tar cJf "${OUTPUT_DIR}/${PACKAGE_NAME}.tar.xz" "${PACKAGE_NAME}"

# Cleanup
rm -rf "${BUILD_DIR}"

echo ""
echo "Package created: ${OUTPUT_DIR}/${PACKAGE_NAME}.tar.xz"
echo ""
echo "To install:"
echo "  tar xf ${PACKAGE_NAME}.tar.xz"
echo "  cd ${PACKAGE_NAME}"
echo "  sudo ./install.sh"
