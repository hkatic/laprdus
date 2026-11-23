#!/bin/bash
#
# test.sh - installs a built package in a fresh container, tests it, removes it
#
# Usage (as root, in the container scripts/linux-packages.sh starts):
#   test.sh deb|rpm|arch|tarball
#
# The packages are read from /pkg/<arch>/ (build.sh wrote them there), the
# test from /src (the checkout, read-only).
#
# 1. The package is installed with the distribution's own package manager,
#    which also has to find its dependencies (speech-dispatcher among them):
#    apt, dnf, pacman; the tarball with its install.sh, after the system's
#    Speech Dispatcher, which a tarball cannot pull in.
# 2. tests/linux/test_package_install.sh runs with LAPRDUS_TEST_STRICT=1:
#    the CLI, the library, the voice data, the module through the module
#    protocol and through a running Speech Dispatcher; a skip is a failure.
# 3. The package is removed, and nothing of it may be left: the CLI, the
#    library, the module, the block in speechd.conf.

set -euo pipefail

FORMAT="${1:?usage: test.sh deb|rpm|arch|tarball}"
ARCH="$(uname -m)"
PKG="/pkg/${ARCH}"
SPEECHD_CONF=/etc/speech-dispatcher/speechd.conf
. /etc/os-release
echo "=== Testing the ${FORMAT} package on ${PRETTY_NAME} (${ARCH})"

# The one file of a kind in /pkg/<arch>.
one() {
    local files
    # shellcheck disable=SC2086 # $1 is a glob
    files=$(ls "${PKG}"/$1 2>/dev/null || true)
    if [ -z "${files}" ] || [ "$(echo "${files}" | wc -l)" -ne 1 ]; then
        echo "test.sh: expected one ${PKG}/$1, found: ${files:-none}" >&2
        exit 1
    fi
    echo "${files}"
}

# The distribution's family, from os-release.
family() {
    case " ${ID} ${ID_LIKE:-} " in
        *" debian "* | *" ubuntu "*) echo debian ;;
        *" fedora "*) echo fedora ;;
        *" arch "*) echo arch ;;
        *) echo "test.sh: unknown distribution ${ID}" >&2; exit 1 ;;
    esac
}

apt_install() {
    export DEBIAN_FRONTEND=noninteractive
    apt-get update -q
    apt-get install -y -q "$@"
}

# spd-say, which the test talks to Speech Dispatcher with: Fedora has it in a
# package of its own.
test_tools() {
    case "$(family)" in
        fedora) dnf install -y speech-dispatcher-utils ;;
        *) ;;
    esac
}

# pacman's downloader sandbox: its syscall filter cannot be set up under
# QEMU's user emulation (an x86_64 container on Apple silicon); see build.sh.
if [ "$(family)" = arch ]; then
    sed -i 's/^#DisableSandboxSyscalls/DisableSandboxSyscalls/' /etc/pacman.conf
fi

PREFIX=/usr
case "${FORMAT}" in
    deb)
        apt_install "$(one 'laprdus_*.deb')" "$(one 'laprdus-dev_*.deb')"
        ;;
    rpm)
        # Not the source RPM, which lies next to the x86_64 packages.
        dnf install -y "$(one "laprdus-[0-9]*.${ARCH}.rpm")" "$(one "laprdus-devel-*.${ARCH}.rpm")"
        ;;
    arch)
        # A full upgrade first: Arch does not support partial upgrades.
        pacman -Syu --noconfirm
        pacman -U --noconfirm "$(one 'laprdus-*.pkg.tar.zst')"
        ;;
    tarball)
        # What install.sh expects to be there already.
        case "$(family)" in
            debian) apt_install speech-dispatcher libasound2 xz-utils ;;
            fedora) dnf install -y speech-dispatcher alsa-lib xz tar ;;
            arch) pacman -Syu --noconfirm speech-dispatcher alsa-lib ;;
        esac
        TARBALL="$(one 'laprdus-*-linux-*.tar.xz')"
        mkdir -p /tmp/tarball
        tar -xJf "${TARBALL}" -C /tmp/tarball
        # The documented way: sudo ./install.sh, with its default prefix.
        PREFIX=/usr/local
        (cd /tmp/tarball/laprdus-* && ./install.sh)
        ;;
    *)
        echo "test.sh: unknown format ${FORMAT}" >&2
        exit 1
        ;;
esac
test_tools

echo "=== The installed package"
LAPRDUS_TEST_STRICT=1 bash /src/tests/linux/test_package_install.sh "${PREFIX}"

echo "=== Removing it"
case "${FORMAT}" in
    deb) apt-get remove -y -q laprdus-dev laprdus ;;
    rpm) dnf remove -y laprdus-devel laprdus ;;
    arch) pacman -R --noconfirm laprdus ;;
    tarball) "${PREFIX}/share/laprdus/uninstall.sh" ;;
esac

left=0
leftover() {
    echo "FAIL: left after removal: $1"
    left=1
}
[ ! -e "${PREFIX}/bin/laprdus" ] || leftover "${PREFIX}/bin/laprdus"
for module in /usr/lib/speech-dispatcher-modules/sd_laprdus /usr/lib64/speech-dispatcher-modules/sd_laprdus \
    /usr/lib/*/speech-dispatcher-modules/sd_laprdus; do
    # (/usr/lib/*/ covers Debian's multiarch folders and Arch's
    # /usr/lib/speech-dispatcher/speech-dispatcher-modules.)
    [ ! -e "${module}" ] || leftover "${module}"
done
if ldconfig -p | grep -q 'liblaprdus\.so'; then
    leftover "liblaprdus in the dynamic linker's cache: $(ldconfig -p | grep 'liblaprdus\.so' | tr -s ' ' | head -n 1)"
fi
if [ -f "${SPEECHD_CONF}" ] && grep -q "# BEGIN LAPRDUS TTS" "${SPEECHD_CONF}"; then
    leftover "the LAPRDUS TTS block in ${SPEECHD_CONF}"
fi
[ ! -d "${PREFIX}/share/laprdus" ] || leftover "${PREFIX}/share/laprdus"
if [ "${left}" -ne 0 ]; then
    exit 1
fi
echo "PASS: nothing is left after removal"
echo "=== ${FORMAT} on ${PRETTY_NAME} (${ARCH}): all passed"
