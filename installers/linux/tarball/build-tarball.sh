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

# Library
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
cp README.md "${PKG}/share/doc/laprdus/"
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

# Create directories
mkdir -p "${PREFIX}/bin"
mkdir -p "${PREFIX}/lib"
mkdir -p "${PREFIX}/share/laprdus"
mkdir -p "${PREFIX}/share/doc/laprdus"
mkdir -p "${PREFIX}/include/laprdus"

# Install files
cp -v bin/* "${PREFIX}/bin/"
cp -v lib/*.so* "${PREFIX}/lib/"
cp -v share/laprdus/* "${PREFIX}/share/laprdus/"
cp -v share/doc/laprdus/* "${PREFIX}/share/doc/laprdus/"
cp -v include/laprdus/* "${PREFIX}/include/laprdus/"

# Install Speech Dispatcher module if present
if [ -d lib/speech-dispatcher-modules ]; then
    SD_MODULE_DIR="${PREFIX}/lib/speech-dispatcher-modules"
    mkdir -p "${SD_MODULE_DIR}"
    cp -v lib/speech-dispatcher-modules/* "${SD_MODULE_DIR}/"
fi

# Install Speech Dispatcher config (only if we have permissions)
if [ -d etc/speech-dispatcher/modules ]; then
    if [ -w /etc/speech-dispatcher/modules ] || [ $EUID -eq 0 ]; then
        mkdir -p /etc/speech-dispatcher/modules
        cp -v etc/speech-dispatcher/modules/* /etc/speech-dispatcher/modules/
    else
        echo "Note: Skipping /etc/speech-dispatcher/modules/ (requires root)"
        echo "      For Speech Dispatcher, run: sudo cp etc/speech-dispatcher/modules/* /etc/speech-dispatcher/modules/"
    fi
fi

# Update library cache (only if we have permissions)
if command -v ldconfig &> /dev/null && [ $EUID -eq 0 ]; then
    ldconfig
fi

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
        echo "      To configure manually, add to $SPEECHD_CONF:"
        echo '      AddModule "laprdus" "sd_laprdus" "laprdus.conf"'
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

    echo "LaprdusTTS configured in Speech Dispatcher."
}

# Configure Speech Dispatcher if module was installed
if [ -d lib/speech-dispatcher-modules ]; then
    configure_speechd
fi

echo ""
echo "LaprdusTTS installed successfully!"
echo ""
echo "To test, run:"
echo "  laprdus \"Dobar dan!\""
echo ""
echo "For Orca screen reader users:"
echo "  Speech Dispatcher has been configured automatically."
echo "  Restart Speech Dispatcher: systemctl --user restart speech-dispatcher"
INSTALL_EOF

chmod +x "${PKG}/install.sh"

# Create uninstall script
cat > "${PKG}/uninstall.sh" << 'UNINSTALL_EOF'
#!/bin/bash
#
# LaprdusTTS Uninstallation Script
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
rm -fv "${PREFIX}/lib/liblaprdus.so"*
rm -rfv "${PREFIX}/share/laprdus"
rm -rfv "${PREFIX}/share/doc/laprdus"
rm -rfv "${PREFIX}/include/laprdus"
rm -fv "${PREFIX}/lib/speech-dispatcher-modules/sd_laprdus"

# Remove system config files (only if we have permissions)
if [ -w /etc/speech-dispatcher/modules ] || [ $EUID -eq 0 ]; then
    rm -fv /etc/speech-dispatcher/modules/laprdus.conf
fi

# Update library cache (only if we have permissions)
if command -v ldconfig &> /dev/null && [ $EUID -eq 0 ]; then
    ldconfig
fi

echo "LaprdusTTS uninstalled."
echo "Restart Speech Dispatcher to apply changes: systemctl --user restart speech-dispatcher"
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
  djedo   - Serbian male senior

Uninstallation
--------------

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
