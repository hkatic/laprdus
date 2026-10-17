#!/bin/bash
#
# LaprdusTTS command-line tools for macOS: installation script
#
# Installs the laprdus CLI, liblaprdus.dylib, the C/C++ headers, the voice
# data and the dictionaries from this tarball under a prefix (default
# /usr/local). The CLI finds the library through @rpath (../lib relative to
# the executable) and the voice data in <prefix>/share/laprdus, so nothing
# else has to be configured.
#
# Usage:
#   sudo ./install.sh              # /usr/local
#   ./install.sh ~/.local          # a prefix of your own, no sudo needed
#
set -e

PREFIX="${1:-/usr/local}"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Installing LaprdusTTS to ${PREFIX}..."

if [[ ! -w "${PREFIX}" && ! -w "$(dirname "${PREFIX}")" ]] && [[ $EUID -ne 0 ]]; then
    echo "Error: ${PREFIX} is not writable. Run: sudo ./install.sh ${PREFIX}" >&2
    exit 1
fi

mkdir -p "${PREFIX}/bin" "${PREFIX}/lib" "${PREFIX}/include/laprdus" \
         "${PREFIX}/share/laprdus" "${PREFIX}/share/doc/laprdus"

# ditto keeps the code signature intact (cp would too, but ditto also keeps
# the extended attributes codesign stores on a bare executable).
ditto -v "${HERE}/bin/laprdus"        "${PREFIX}/bin/laprdus"
ditto -v "${HERE}/lib/liblaprdus.dylib" "${PREFIX}/lib/liblaprdus.dylib"
ditto -v "${HERE}/include/laprdus"    "${PREFIX}/include/laprdus"
ditto -v "${HERE}/share/laprdus"      "${PREFIX}/share/laprdus"
ditto -v "${HERE}/share/doc/laprdus"  "${PREFIX}/share/doc/laprdus"
ditto -v "${HERE}/uninstall.sh"       "${PREFIX}/share/laprdus/uninstall.sh"
chmod 755 "${PREFIX}/bin/laprdus" "${PREFIX}/share/laprdus/uninstall.sh"

# A download through a browser carries the quarantine attribute into the
# extracted files; the binaries are signed and notarized, but clearing it on
# our own copies spares the first-run dialog for a command-line tool.
xattr -dr com.apple.quarantine "${PREFIX}/bin/laprdus" "${PREFIX}/lib/liblaprdus.dylib" 2>/dev/null || true

echo ""
echo "Installed. Try:"
echo "  ${PREFIX}/bin/laprdus \"Dobar dan!\""
if [[ "${PREFIX}" != "/usr/local" && "${PREFIX}" != "/usr" ]]; then
    echo "The CLI looks for its voice data in /usr/share/laprdus and /usr/local/share/laprdus;"
    echo "with this prefix pass it explicitly: laprdus -D ${PREFIX}/share/laprdus \"Dobar dan!\""
fi
echo "To remove: sudo ${PREFIX}/share/laprdus/uninstall.sh"
