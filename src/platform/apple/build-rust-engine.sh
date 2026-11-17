#!/bin/sh
# Builds the Laprdus engine (the Rust crate laprdus-capi, a static library
# with the C API) for the platform and architectures Xcode is building, and
# puts it into the products folder as liblaprdus_capi.a, where the app and
# the LaprdusVoices extension link it.
#
# Run by the "Build the Rust engine" phase of both targets. Needs rustup with
# the targets of the platforms built:
#   rustup target add aarch64-apple-darwin x86_64-apple-darwin \
#       aarch64-apple-ios aarch64-apple-ios-sim x86_64-apple-ios
set -eu

# Xcode starts scripts with a bare PATH.
PATH="$HOME/.cargo/bin:/opt/homebrew/bin:/usr/local/bin:$PATH"
export PATH

repository="$(cd "$SRCROOT/../../../.." && pwd)"

# Always optimized: the synthesizer is too slow unoptimized for a screen
# reader, so debug builds of the app link the release engine too.
libraries=
for arch in $ARCHS; do
    case "$arch" in
        arm64) cpu=aarch64 ;;
        x86_64) cpu=x86_64 ;;
        *) echo "error: no Rust target for the architecture $arch" >&2; exit 1 ;;
    esac
    case "$PLATFORM_NAME" in
        macosx) triple="$cpu-apple-darwin" ;;
        iphoneos) triple="$cpu-apple-ios" ;;
        iphonesimulator)
            if [ "$cpu" = aarch64 ]; then triple=aarch64-apple-ios-sim; else triple=x86_64-apple-ios; fi ;;
        *) echo "error: no Rust target for the platform $PLATFORM_NAME" >&2; exit 1 ;;
    esac
    # Xcode's own compiler settings are for its targets, not for Cargo's
    # build scripts, which run on the Mac.
    (cd "$repository" && env -u SDKROOT -u MACOSX_DEPLOYMENT_TARGET -u IPHONEOS_DEPLOYMENT_TARGET \
        MACOSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-13.0}" \
        IPHONEOS_DEPLOYMENT_TARGET="${IPHONEOS_DEPLOYMENT_TARGET:-16.0}" \
        cargo build --release -p laprdus-capi --target "$triple")
    libraries="$libraries $repository/target/$triple/release/liblaprdus_capi.a"
done

mkdir -p "$BUILT_PRODUCTS_DIR"
# shellcheck disable=SC2086
lipo -create $libraries -output "$BUILT_PRODUCTS_DIR/liblaprdus_capi.a"
