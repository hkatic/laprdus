#!/bin/bash
#
# LaprdusTTS command-line tools for macOS: removal script
#
# Removes what install.sh put under the prefix (default /usr/local).
# Usage: sudo ./uninstall.sh [prefix]
#
set -e

PREFIX="${1:-/usr/local}"

echo "Removing LaprdusTTS from ${PREFIX}..."
rm -fv "${PREFIX}/bin/laprdus" "${PREFIX}/lib/liblaprdus.dylib"
rm -rfv "${PREFIX}/include/laprdus" "${PREFIX}/share/doc/laprdus"
# The uninstall script itself lives in share/laprdus; the directory goes last.
rm -rfv "${PREFIX}/share/laprdus"
echo "Done."
