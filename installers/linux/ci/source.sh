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
#
# A nightly build (LAPRDUS_NIGHTLY=YYYYMMDD, set by .github/workflows/
# nightly.yml) gets a package release below the version's own, by each
# distribution's rules, so a nightly sorts under the release of its version
# and above the nightly of the night before:
#   deb 2.0.0-0~nightly20261124, rpm 2.0.0-0.nightly20261124, Arch 2.0.0-0.20261124
# (the release itself is 2.0.0-1 in all three). The upstream version, and so
# the tarball's name, stay the version of Cargo.toml. Only the tarball's
# copies of debian/changelog, the spec and the PKGBUILD are changed, never
# the files of the checkout.

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

if [ -n "${LAPRDUS_NIGHTLY:-}" ]; then
    case "${LAPRDUS_NIGHTLY}" in
        [0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]) ;;
        *) echo "source.sh: LAPRDUS_NIGHTLY is a date YYYYMMDD, not ${LAPRDUS_NIGHTLY}" >&2; exit 1 ;;
    esac
    night="${LAPRDUS_NIGHTLY}"
    # A file of the copied index, and the same file with new content
    # (standard input) in its place, with the mode it had.
    indexed() { GIT_INDEX_FILE="${index}" git show ":$1"; }
    replace() {
        local mode blob
        mode="$(GIT_INDEX_FILE="${index}" git ls-files -s -- "$1" | cut -d' ' -f1)"
        blob="$(git hash-object -w --stdin)"
        GIT_INDEX_FILE="${index}" git update-index --cacheinfo "${mode},${blob},$1"
    }
    changelog=installers/linux/deb/debian/changelog
    signature="$(indexed "${changelog}" | sed -n '/^ -- /{s/>.*/>/;p;q;}')"
    {
        printf 'laprdus (%s-0~nightly%s) unstable; urgency=medium\n\n' "${VERSION}" "${night}"
        printf '  * Nightly build of commit %s.\n\n' "$(git rev-parse --short HEAD)"
        printf '%s  %s\n\n' "${signature}" "$(LC_ALL=C date -R)"
        indexed "${changelog}"
    } | replace "${changelog}"
    indexed installers/linux/rpm/laprdus.spec |
        sed "s/^\(Release:[[:space:]]*\)[^%]*/\10.nightly${night}/" | replace installers/linux/rpm/laprdus.spec
    indexed installers/linux/arch/PKGBUILD |
        sed "s/^pkgrel=.*/pkgrel=0.${night}/" | replace installers/linux/arch/PKGBUILD
    echo "source.sh: nightly ${night}: deb ${VERSION}-0~nightly${night}, rpm ${VERSION}-0.nightly${night}, Arch ${VERSION}-0.${night}" >&2
fi

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
