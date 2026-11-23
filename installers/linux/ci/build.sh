#!/bin/bash
#
# build.sh - builds one Linux package inside a distribution's container
#
# Usage (as root, in the container scripts/linux-packages.sh starts):
#   build.sh deb|rpm|arch|tarball
#
# /pkg is the host's dist/linux-packages: the source tarball
# laprdus-<version>.tar.xz (installers/linux/ci/source.sh) is read from
# there and the packages are written to /pkg/<arch>/ (x86_64 or aarch64).
# The sources are unpacked inside the container and built there, never in
# the checkout, so the host's target/ is left alone.
#
#   deb      ubuntu:22.04 (the oldest C library of the systems it is tested
#            on, so the one package installs on all of them), Rust from
#            rustup: Debian 12's own rustc is 1.63 and the engine needs 1.85
#   tarball  ubuntu:22.04 for the same reason, Rust from rustup
#   rpm      the older of the two Fedora releases, Fedora's own Rust
#   arch     archlinux:latest, Arch's own Rust; makepkg runs as a user
#            "builder", because it refuses to run as root
#
# LAPRDUS_UID and LAPRDUS_GID, when set, are given the written files, so a
# Linux host does not end up with root's files in dist/.

set -euo pipefail

FORMAT="${1:?usage: build.sh deb|rpm|arch|tarball}"
ARCH="$(uname -m)"
OUT="/pkg/${ARCH}"
SOURCE="$(ls /pkg/laprdus-*.tar.xz 2>/dev/null | head -n 1 || true)"
[ -n "${SOURCE}" ] && [ -f "${SOURCE}" ] || { echo "build.sh: no /pkg/laprdus-*.tar.xz (run installers/linux/ci/source.sh first)" >&2; exit 1; }
NAME="$(basename "${SOURCE}" .tar.xz)"   # laprdus-<version>
VERSION="${NAME#laprdus-}"
mkdir -p "${OUT}"

echo "=== Building the ${FORMAT} package of Laprdus ${VERSION} for ${ARCH} ($(. /etc/os-release && echo "${PRETTY_NAME}"))"

# Unpacks the sources into /build.
unpack() {
    mkdir -p /build
    tar -xJf "${SOURCE}" -C /build
}

# Rust from rustup, for the systems whose own Rust is too old.
rustup_stable() {
    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs |
        sh -s -- -y --profile minimal --default-toolchain stable
    export PATH="${HOME}/.cargo/bin:${PATH}"
    rustc --version
}

apt_install() {
    export DEBIAN_FRONTEND=noninteractive
    apt-get update -q
    apt-get install -y -q --no-install-recommends "$@"
}

case "${FORMAT}" in
    deb)
        apt_install build-essential debhelper dpkg-dev pkg-config libasound2-dev \
            ca-certificates curl xz-utils
        rustup_stable
        unpack
        cd "/build/${NAME}"
        ln -sfn installers/linux/deb/debian debian
        # -d: the Build-Depends name the distribution's rustc and cargo,
        # which are too old here; rustup's are used instead.
        dpkg-buildpackage -us -uc -b -d
        cp -v /build/laprdus_*.deb /build/laprdus-dev_*.deb "${OUT}/"
        ;;

    tarball)
        apt_install build-essential pkg-config libasound2-dev ca-certificates curl xz-utils
        rustup_stable
        unpack
        cd "/build/${NAME}"
        # The last line cargo xtask dist prints is the tarball's path.
        TARBALL="$(cargo xtask dist --locked | tail -n 1)"
        cp -v "${TARBALL}" "${OUT}/"
        ;;

    rpm)
        dnf install -y rpm-build rpmdevtools cargo rust gcc pkgconf-pkg-config alsa-lib-devel xz tar
        rustc --version
        rpmdev-setuptree
        cp "${SOURCE}" "${HOME}/rpmbuild/SOURCES/"
        tar -xJf "${SOURCE}" -C /tmp "${NAME}/installers/linux/rpm/laprdus.spec"
        rpmbuild -ba "/tmp/${NAME}/installers/linux/rpm/laprdus.spec"
        cp -v "${HOME}"/rpmbuild/RPMS/*/laprdus*.rpm "${OUT}/"
        # The source RPM is the same for every architecture: one copy, with
        # the x86_64 packages.
        if [ "${ARCH}" = x86_64 ]; then
            cp -v "${HOME}"/rpmbuild/SRPMS/laprdus-*.src.rpm "${OUT}/"
        fi
        ;;

    arch)
        # pacman's downloader sandbox: the image turns its filesystem part
        # off (DisableSandboxFilesystem), and its syscall filter cannot be
        # set up under QEMU's user emulation (an x86_64 container on Apple
        # silicon: "error restricting syscalls via seccomp: 22"). This is a
        # throwaway container, and the sandbox guards only pacman's downloads.
        sed -i 's/^#DisableSandboxSyscalls/DisableSandboxSyscalls/' /etc/pacman.conf
        # The package's dependencies (makepkg -s would need sudo for them):
        # base-devel and Rust to build, alsa-lib and speech-dispatcher, which
        # makepkg checks are installed.
        pacman -Syu --noconfirm --needed base-devel rust pkgconf alsa-lib speech-dispatcher
        rustc --version
        useradd --create-home builder
        mkdir -p /build/arch
        tar -xJf "${SOURCE}" -C /tmp "${NAME}/installers/linux/arch"
        cp "/tmp/${NAME}/installers/linux/arch/PKGBUILD" "/tmp/${NAME}/installers/linux/arch/laprdus.install" /build/arch/
        cp "${SOURCE}" /build/arch/
        chown -R builder: /build/arch
        runuser -u builder -- bash -c 'cd /build/arch && makepkg --noconfirm -f'
        # Not the laprdus-debug package makepkg may add.
        cp -v /build/arch/laprdus-"${VERSION}"-*.pkg.tar.zst "${OUT}/"
        ;;

    *)
        echo "build.sh: unknown format ${FORMAT} (deb, rpm, arch or tarball)" >&2
        exit 1
        ;;
esac

if [ -n "${LAPRDUS_UID:-}" ]; then
    chown -R "${LAPRDUS_UID}:${LAPRDUS_GID:-${LAPRDUS_UID}}" "${OUT}"
fi
echo "=== Built:"
ls -l "${OUT}"
