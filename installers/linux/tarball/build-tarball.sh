#!/bin/bash
#
# Build LaprdusTTS generic Linux tarball package
#
# Usage: ./build-tarball.sh
#
# This creates a tar.xz archive suitable for manual installation
# on any Linux distribution, in the directory it is run from.
#
# The work is done by `cargo xtask dist`: it builds the programs, packs the
# voice data and stages bin/, lib/, include/, share/ and etc/ together with
# install.sh, uninstall.sh and README of this directory. The version is the
# workspace's (Cargo.toml).

set -e

OUTPUT_DIR="$(pwd)"

# Source directory (project root)
PROJECT_ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"

if [ -n "${1:-}" ]; then
    echo "Note: the version is read from Cargo.toml; the argument \"$1\" is ignored."
fi

echo "Building the LaprdusTTS tarball..."
echo "Project root: ${PROJECT_ROOT}"

cd "${PROJECT_ROOT}"
# The last line xtask prints is the tarball's path.
TARBALL="$(cargo xtask dist --locked | tail -n 1)"
[ -f "${TARBALL}" ] || { echo "Error: cargo xtask dist produced no tarball." >&2; exit 1; }
PACKAGE_FILE="$(basename "${TARBALL}")"
PACKAGE_NAME="${PACKAGE_FILE%.tar.xz}"
cp "${TARBALL}" "${OUTPUT_DIR}/"

echo ""
echo "Package created: ${OUTPUT_DIR}/${PACKAGE_FILE}"
echo ""
echo "To install:"
echo "  tar xf ${PACKAGE_FILE}"
echo "  cd ${PACKAGE_NAME}"
echo "  sudo ./install.sh"
