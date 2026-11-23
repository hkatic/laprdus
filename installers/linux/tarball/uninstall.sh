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
# "|| true": without pkg-config, or without Speech Dispatcher's .pc file
# (it comes with the development package on Debian), the failed command
# would end the script under "set -e" before the fallback below.
SD_MODULE_DIR=$(pkg-config --variable=modulebindir speech-dispatcher 2>/dev/null || true)
if [ -n "$SD_MODULE_DIR" ]; then
    rm -fv "${SD_MODULE_DIR}/sd_laprdus"
fi
# Every location install.sh may have chosen (Fedora, Debian and Ubuntu,
# Arch, other multiarch folders)
for dir in /usr/lib64/speech-dispatcher-modules /usr/lib/speech-dispatcher-modules \
    /usr/lib/speech-dispatcher/speech-dispatcher-modules /usr/lib/*/speech-dispatcher-modules \
    /usr/libexec/speech-dispatcher-modules; do
    rm -fv "${dir}/sd_laprdus"
done
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
    # Fallback: kill any remaining speech-dispatcher processes to force
    # a clean restart on next client connection (via socket activation)
    killall speech-dispatcher 2>/dev/null || true
}
restart_speechd

echo ""
echo "LaprdusTTS uninstalled."
