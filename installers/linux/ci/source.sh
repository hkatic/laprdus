#!/bin/bash
#
# source.sh - the source tarball every Linux package is built from
#
# Usage: installers/linux/ci/source.sh OUTPUT_DIR
#
# Writes OUTPUT_DIR/laprdus-<version>.tar.xz with the top folder
# laprdus-<version>/, the tarball the RPM spec (Source0) and the PKGBUILD
# (source) take, and prints its path. The version is the workspace's
# (Cargo.toml).
#
# The tarball holds the working tree as Git sees it: committed files, changes
# not yet committed and new files not yet added, without what .gitignore
# leaves out (target/, dist/, the generated voice data). The real index is
# not touched: the files are added to a copy of it. In a clean checkout (CI)
# this is exactly the commit.
#
# Before writing it, the version is checked: Cargo.toml, the head of
# debian/changelog, the spec's Version and the PKGBUILD's pkgver must agree,
# and with LAPRDUS_RELEASE_TAG set (vX.Y.Z or vX.Y.Z-anything, the tag a
# release is built for) the tag must name that version too.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
OUT="${1:?usage: source.sh OUTPUT_DIR}"
cd "${ROOT}"

VERSION="$(sed -n '/^\[workspace.package\]/,/^\[/s/^version = "\(.*\)"/\1/p' Cargo.toml)"
[ -n "${VERSION}" ] || { echo "source.sh: no version in Cargo.toml [workspace.package]" >&2; exit 1; }

# The version every package file names.
deb="$(sed -n '1s/^laprdus (\(.*\)-[^-]*).*/\1/p' installers/linux/deb/debian/changelog)"
rpm="$(sed -n 's/^Version:[[:space:]]*//p' installers/linux/rpm/laprdus.spec | head -n 1)"
arch="$(sed -n 's/^pkgver=//p' installers/linux/arch/PKGBUILD)"
mismatch=0
for pair in "debian/changelog:${deb}" "laprdus.spec:${rpm}" "PKGBUILD:${arch}"; do
    if [ "${pair#*:}" != "${VERSION}" ]; then
        echo "source.sh: ${pair%%:*} says ${pair#*:}, Cargo.toml says ${VERSION}" >&2
        mismatch=1
    fi
done
if [ -n "${LAPRDUS_RELEASE_TAG:-}" ]; then
    tag_version="${LAPRDUS_RELEASE_TAG#v}"
    tag_version="${tag_version%%-*}"
    if [ "${tag_version}" != "${VERSION}" ]; then
        echo "source.sh: the tag ${LAPRDUS_RELEASE_TAG} does not name version ${VERSION}" >&2
        mismatch=1
    fi
fi
[ "${mismatch}" -eq 0 ] || exit 1

# The working tree through a copy of the index.
index="$(mktemp)"
trap 'rm -f "${index}"' EXIT
cp "$(git rev-parse --git-path index)" "${index}"
GIT_INDEX_FILE="${index}" git add -A
tree="$(GIT_INDEX_FILE="${index}" git write-tree)"

mkdir -p "${OUT}"
TARBALL="$(cd "${OUT}" && pwd)/laprdus-${VERSION}.tar.xz"
if command -v xz >/dev/null 2>&1; then
    git archive --format=tar --prefix="laprdus-${VERSION}/" "${tree}" | xz -T0 -6 >"${TARBALL}"
else
    # macOS has no xz program; its tar (libarchive) recompresses the archive.
    git archive --format=tar --prefix="laprdus-${VERSION}/" "${tree}" | tar -cJf "${TARBALL}" @-
fi
echo "${TARBALL}"
