#!/bin/bash
#
# linux-packages.sh - builds the Linux packages in distribution containers and
# tests each on the systems it is for
#
# Usage:
#   scripts/linux-packages.sh [options] [FORMAT ...]
#
#   FORMAT       deb, rpm, arch, tarball or all (the default)
#   --arch A     x86_64 or aarch64 (default: this machine's; on a Mac with
#                Apple silicon aarch64 runs natively, x86_64 under emulation,
#                which is many times slower)
#   --no-test    build only
#   --test-only  test the packages already built in dist/linux-packages/<arch>/
#   --image I    test on image I only, instead of the format's list (may be
#                given more than once)
#
# Needs Docker (or Podman: DOCKER=podman) and Git. Everything else, the
# compilers and Rust included, comes inside the containers. Each format is
# built in one container and tested in fresh containers of every system
# listed below; the GitHub workflow .github/workflows/linux-packages.yml runs
# this same script, one job per format and architecture.
#
# Output: dist/linux-packages/laprdus-<version>.tar.xz (the sources, from
# installers/linux/ci/source.sh) and the packages in dist/linux-packages/<arch>/.
#
# The containers run installers/linux/ci/build.sh and test.sh; the test is
# tests/linux/test_package_install.sh, strict.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PKG="${ROOT}/dist/linux-packages"
DOCKER="${DOCKER:-}"

# The container a format is built in: the oldest C library it is tested
# with, so one package installs on every system of its list.
build_image() {
    case "$1" in
        deb | tarball) echo "ubuntu:22.04" ;;
        rpm) echo "fedora:44" ;;
        arch) echo "archlinux:latest" ;;
    esac
}

# The systems a format is tested on: the current and the previous release.
# Ubuntu 22.04 has standard support until April 2027. Bump Fedora when a new
# release comes (fedora:latest was 45 in November 2026); rpm builds on the
# older one.
test_images() {
    case "$1" in
        deb) echo "debian:12 debian:13 ubuntu:22.04 ubuntu:24.04 ubuntu:26.04" ;;
        rpm) echo "fedora:44 fedora:45" ;;
        arch) echo "archlinux:latest" ;;
        tarball) echo "debian:12 fedora:45 archlinux:latest" ;;
    esac
}

# Images that exist for x86_64 only: Arch Linux has no official arm64 image.
x86_64_only() {
    case "$1" in
        archlinux:*) return 0 ;;
        *) return 1 ;;
    esac
}

usage() {
    sed -n '3,16p' "$0" | sed 's/^# \{0,1\}//'
    exit "${1:-0}"
}

# --- Arguments ---------------------------------------------------------------

ARCH=""
BUILD=1
TEST=1
IMAGES=""
FORMATS=""
while [ $# -gt 0 ]; do
    case "$1" in
        --arch) ARCH="${2:?--arch needs x86_64 or aarch64}"; shift 2 ;;
        --no-test) TEST=0; shift ;;
        --test-only) BUILD=0; shift ;;
        --image) IMAGES="${IMAGES} ${2:?--image needs an image}"; shift 2 ;;
        -h | --help) usage 0 ;;
        all) FORMATS="deb rpm arch tarball"; shift ;;
        deb | rpm | arch | tarball) FORMATS="${FORMATS} $1"; shift ;;
        *) echo "linux-packages.sh: unknown argument $1" >&2; usage 1 ;;
    esac
done
FORMATS="${FORMATS:-deb rpm arch tarball}"

if [ -z "${ARCH}" ]; then
    case "$(uname -m)" in
        x86_64 | amd64) ARCH=x86_64 ;;
        aarch64 | arm64) ARCH=aarch64 ;;
        *) echo "linux-packages.sh: unknown machine $(uname -m); give --arch" >&2; exit 1 ;;
    esac
fi
case "${ARCH}" in
    x86_64) PLATFORM=linux/amd64 ;;
    aarch64) PLATFORM=linux/arm64 ;;
    *) echo "linux-packages.sh: --arch is x86_64 or aarch64, not ${ARCH}" >&2; exit 1 ;;
esac

if [ -z "${DOCKER}" ]; then
    if command -v docker >/dev/null 2>&1; then
        DOCKER=docker
    elif command -v podman >/dev/null 2>&1; then
        DOCKER=podman
    else
        echo "linux-packages.sh: needs Docker or Podman (on a Mac: OrbStack, Docker Desktop or colima)" >&2
        exit 1
    fi
fi

# --- Helpers -----------------------------------------------------------------

# Collapsible sections in a GitHub Actions log.
group() {
    if [ -n "${GITHUB_ACTIONS:-}" ]; then echo "::group::$*"; else echo; echo "### $*"; fi
}
endgroup() {
    if [ -n "${GITHUB_ACTIONS:-}" ]; then echo "::endgroup::"; fi
}

# A container of the image for the architecture, with the checkout read-only
# at /src and dist/linux-packages at /pkg.
container() {
    local image="$1" pkg_mode="$2"
    shift 2
    local owner=()
    # The files the build writes belong to the user on a Linux host (Docker
    # on a Mac gives them to the user by itself).
    if [ "$(uname -s)" = Linux ]; then
        owner=(-e "LAPRDUS_UID=$(id -u)" -e "LAPRDUS_GID=$(id -g)")
    fi
    "${DOCKER}" run --rm --platform "${PLATFORM}" \
        -v "${ROOT}:/src:ro" -v "${PKG}:/pkg:${pkg_mode}" \
        ${owner[@]+"${owner[@]}"} "${image}" "$@"
}

# The packages of a format in /pkg/<arch>, removed before it is built again
# (test.sh insists on exactly one of each).
remove_old() {
    local dir="${PKG}/${ARCH}"
    case "$1" in
        deb) rm -f "${dir}"/laprdus_*.deb "${dir}"/laprdus-dev_*.deb ;;
        rpm) rm -f "${dir}"/laprdus*.rpm ;;
        arch) rm -f "${dir}"/laprdus-*.pkg.tar.zst ;;
        tarball) rm -f "${dir}"/laprdus-*-linux-*.tar.xz ;;
    esac
}

# --- Build and test ----------------------------------------------------------

mkdir -p "${PKG}/${ARCH}"
RESULTS=()
FAILED=0
record() {
    RESULTS+=("$1")
    case "$1" in FAIL*) FAILED=1 ;; esac
}

if [ "${BUILD}" = 1 ]; then
    group "Source tarball"
    "${ROOT}/installers/linux/ci/source.sh" "${PKG}"
    endgroup
fi

for format in ${FORMATS}; do
    if [ "${BUILD}" = 1 ]; then
        image="$(build_image "${format}")"
        if [ "${ARCH}" = aarch64 ] && x86_64_only "${image}"; then
            record "SKIP  ${format}: no ${image} image for aarch64"
            continue
        fi
        remove_old "${format}"
        group "Build ${format} in ${image} (${ARCH})"
        if container "${image}" rw bash /src/installers/linux/ci/build.sh "${format}"; then
            record "PASS  build ${format} in ${image}"
            endgroup
        else
            endgroup
            record "FAIL  build ${format} in ${image}"
            continue
        fi
    fi

    if [ "${TEST}" = 1 ]; then
        for image in ${IMAGES:-$(test_images "${format}")}; do
            if [ "${ARCH}" = aarch64 ] && x86_64_only "${image}"; then
                record "SKIP  test ${format} on ${image}: no image for aarch64"
                continue
            fi
            group "Test ${format} on ${image} (${ARCH})"
            if container "${image}" ro bash /src/installers/linux/ci/test.sh "${format}"; then
                endgroup
                record "PASS  test ${format} on ${image}"
            else
                endgroup
                record "FAIL  test ${format} on ${image}"
            fi
        done
    fi
done

echo
echo "=========================================="
echo "Linux packages (${ARCH})"
echo "=========================================="
for result in ${RESULTS[@]+"${RESULTS[@]}"}; do
    echo "${result}"
done
echo
ls -l "${PKG}/${ARCH}" 2>/dev/null || true
exit "${FAILED}"
