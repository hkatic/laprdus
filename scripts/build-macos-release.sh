#!/bin/bash
# =============================================================================
# LaprdusTTS macOS release build
# =============================================================================
# Prepares and packages the macOS release for distribution outside the Mac
# App Store (Developer ID):
#
#   1. voice data (scons voice-data)
#   2. the command-line tools: a universal (Apple Silicon + Intel) laprdus CLI
#      and liblaprdus.dylib, signed with the Developer ID and the hardened
#      runtime, packed by `scons macos-dist` as
#      laprdus-<version>-macos-universal.tar.xz
#   3. the Laprdus app with its LaprdusVoices speech extension: xcodebuild
#      archive (Release, universal), export with the "developer-id" method
#   4. notarization of the app with notarytool, stapling of the ticket
#   5. a drag-to-Applications disk image Laprdus-<version>-macOS.dmg, signed,
#      notarized and stapled
#   6. Gatekeeper assessment of the results, dSYMs and SHA-256 checksums
#
# Everything lands in dist/macos/ (override with --output).
#
# Requirements:
#   - Xcode with the command line tools (xcodebuild, codesign, notarytool,
#     stapler, hdiutil), SCons
#   - the team's Apple Developer Program membership signed in to Xcode
#     (Xcode > Settings > Accounts): Xcode then signs the app with a
#     cloud-managed Developer ID certificate. A local "Developer ID
#     Application" certificate in the login keychain (created from a CSR at
#     developer.apple.com) also signs the command-line tools and the DMG.
#   - notarytool credentials stored once in the keychain:
#       xcrun notarytool store-credentials laprdus \
#           --apple-id <Apple ID> --team-id <TEAMID> --password <app-specific password>
#     (profile name "laprdus" by default, see --notary-profile)
#
# Usage:
#   ./scripts/build-macos-release.sh                 # full release
#   ./scripts/build-macos-release.sh --development   # pipeline check with the
#                                                    # Apple Development certificate,
#                                                    # no notarization
#   ./scripts/build-macos-release.sh --skip-notarization
#
# Options:
#   --development            Sign with "Apple Development", export with the
#                            "debugging" method, skip notarization and the
#                            Gatekeeper check. For testing the build itself.
#   --skip-notarization      Developer ID signing without notarytool.
#   --skip-cli               Do not build the command-line tools tarball.
#   --identity NAME          Code signing identity (default: the Developer ID
#                            Application certificate of the project's team,
#                            or Apple Development with --development). With no
#                            local Developer ID certificate the app is signed
#                            by Xcode's cloud-managed one and the command-line
#                            tools and the disk image stay unsigned.
#   --notary-profile NAME    notarytool keychain profile (default:
#                            $LAPRDUS_NOTARY_PROFILE or "laprdus").
#   --output DIR             Output directory (default: dist/macos).
#   --clean                  Delete the work directory (build/macos-release)
#                            first, for a from-scratch Xcode build.
#   -h, --help               This text.
# =============================================================================

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

XCODE_PROJECT="Lapplerdus/Laprdus/Laprdus.xcodeproj"
SCHEME="Laprdus"
WORK_DIR="$PROJECT_ROOT/build/macos-release"
OUTPUT_DIR="$PROJECT_ROOT/dist/macos"

DEVELOPMENT=0
NOTARIZE=1
BUILD_CLI=1
CLEAN=0
IDENTITY=""
NOTARY_PROFILE="${LAPRDUS_NOTARY_PROFILE:-laprdus}"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; BLUE='\033[0;34m'; NC='\033[0m'
log_info()    { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error()   { echo -e "${RED}[ERROR]${NC} $1" >&2; }
die()         { log_error "$1"; exit 1; }

usage() { awk 'NR > 4 { if ($0 ~ /^# =+$/) exit; sub(/^# ?/, ""); print }' "${BASH_SOURCE[0]}"; }

while [[ $# -gt 0 ]]; do
    case "$1" in
        --development)       DEVELOPMENT=1; NOTARIZE=0 ;;
        --skip-notarization) NOTARIZE=0 ;;
        --skip-cli)          BUILD_CLI=0 ;;
        --identity)          IDENTITY="${2:?--identity needs a value}"; shift ;;
        --notary-profile)    NOTARY_PROFILE="${2:?--notary-profile needs a value}"; shift ;;
        --output)            OUTPUT_DIR="$(cd "$(dirname "${2:?--output needs a value}")" && pwd)/$(basename "$2")"; shift ;;
        --clean)             CLEAN=1 ;;
        -h|--help)           usage; exit 0 ;;
        *)                   usage >&2; die "Unknown option: $1" ;;
    esac
    shift
done

# -----------------------------------------------------------------------------
# Prerequisites
# -----------------------------------------------------------------------------
[[ "$(uname -s)" == "Darwin" ]] || die "This script runs on macOS only."
for tool in xcodebuild scons codesign hdiutil ditto lipo; do
    command -v "$tool" >/dev/null 2>&1 || die "$tool not found. Install Xcode and SCons (pip install scons)."
done
xcrun --find notarytool >/dev/null 2>&1 || die "notarytool not found: Xcode 13 or newer is required."
xcrun --find stapler    >/dev/null 2>&1 || die "stapler not found in the Xcode command line tools."

# -----------------------------------------------------------------------------
# Versions and team, read from the two build systems so they cannot drift
# -----------------------------------------------------------------------------
ENGINE_VERSION="$(python3 - << 'PY'
import re
s = open('SConstruct', encoding='utf-8').read()
v = {k: re.search(rf'^VERSION_{k} = (\d+)', s, re.M).group(1) for k in ('MAJOR', 'MINOR', 'PATCH')}
print(f"{v['MAJOR']}.{v['MINOR']}.{v['PATCH']}")
PY
)"

log_info "Reading the Xcode project's build settings..."
BUILD_SETTINGS="$(xcodebuild -showBuildSettings -project "$XCODE_PROJECT" -scheme "$SCHEME" \
                  -configuration Release -destination 'generic/platform=macOS' 2>/dev/null)"
# No `| head -1` anywhere in this script: with pipefail, a reader that exits
# early kills the writer with SIGPIPE and the whole pipeline fails.
setting() { awk -v key="$1" '$1 == key && $2 == "=" { $1 = ""; $2 = ""; sub(/^ +/, ""); print; exit }' <<< "$BUILD_SETTINGS"; }
APP_VERSION="$(setting MARKETING_VERSION)"
APP_BUILD="$(setting CURRENT_PROJECT_VERSION)"
TEAM_ID="$(setting DEVELOPMENT_TEAM)"
HARDENED="$(setting ENABLE_HARDENED_RUNTIME)"
[[ -n "$APP_VERSION" && -n "$TEAM_ID" ]] || die "Could not read MARKETING_VERSION / DEVELOPMENT_TEAM from $XCODE_PROJECT."
[[ "$HARDENED" == "YES" ]] || die "ENABLE_HARDENED_RUNTIME is not YES in the Release configuration; notarization would reject the app."
if [[ "$APP_VERSION" != "$ENGINE_VERSION" ]]; then
    log_warning "The app's MARKETING_VERSION ($APP_VERSION) differs from the SConstruct version ($ENGINE_VERSION)."
fi

echo "=============================================="
echo "  LaprdusTTS macOS release build"
echo "=============================================="
echo "  App version:    $APP_VERSION (build $APP_BUILD)"
echo "  Engine version: $ENGINE_VERSION"
echo "  Team:           $TEAM_ID"
if (( DEVELOPMENT )); then
    echo "  Mode:           development (not for distribution)"
else
    echo "  Mode:           Developer ID$( (( NOTARIZE )) && echo ", notarized" || echo ", NOT notarized")"
fi
echo "  Output:         $OUTPUT_DIR"
echo ""

# -----------------------------------------------------------------------------
# Signing identity
# -----------------------------------------------------------------------------
if (( DEVELOPMENT )); then
    IDENTITY_KIND="Apple Development"
    EXPORT_METHOD="debugging"
else
    IDENTITY_KIND="Developer ID Application"
    EXPORT_METHOD="developer-id"
fi
if [[ -z "$IDENTITY" ]]; then
    # Prefer a certificate of the project's team when the keychain holds several.
    IDENTITIES="$(security find-identity -v -p codesigning | sed -n 's/^ *[0-9]*) [0-9A-F]* "\(.*\)"$/\1/p' | grep "^$IDENTITY_KIND:" || true)"
    IDENTITY="$(awk -v team="($TEAM_ID)" 'index($0, team) { print; exit }' <<< "$IDENTITIES")"
    [[ -n "$IDENTITY" ]] || IDENTITY="$(awk 'NR == 1' <<< "$IDENTITIES")"
fi
# Xcode can sign the app with a cloud-managed Developer ID certificate (the
# key stays with Apple; -allowProvisioningUpdates creates and uses it), which
# never shows up in the keychain. codesign cannot use it, so without a local
# certificate the command-line tools and the disk image stay unsigned.
CLOUD_SIGNING=0
if [[ -z "$IDENTITY" ]]; then
    if (( DEVELOPMENT )); then
        die "No \"Apple Development\" certificate in the keychain: sign in to your Apple account in Xcode > Settings > Accounts."
    fi
    CLOUD_SIGNING=1
    log_warning "No \"Developer ID Application\" certificate in the keychain."
    log_warning "The app will be signed by Xcode with the team's cloud-managed Developer ID certificate."
    log_warning "The command-line tools and the disk image get no Developer ID signature (codesign cannot use a"
    log_warning "cloud-managed certificate), so the tools are not notarized. For a fully signed release create a"
    log_warning "local certificate (developer.apple.com > Certificates > + > Developer ID Application, from a CSR),"
    log_warning "import the .cer into the login keychain, or pass --identity."
else
    log_info "Signing identity: $IDENTITY"
fi

if (( NOTARIZE )); then
    if ! xcrun notarytool history --keychain-profile "$NOTARY_PROFILE" >/dev/null 2>&1; then
        log_error "notarytool has no credentials under the keychain profile \"$NOTARY_PROFILE\". Store them once with:"
        log_error "  xcrun notarytool store-credentials $NOTARY_PROFILE --apple-id <Apple ID> --team-id $TEAM_ID --password <app-specific password>"
        die "or run with --skip-notarization."
    fi
fi

# -----------------------------------------------------------------------------
# Work and output directories
# -----------------------------------------------------------------------------
(( CLEAN )) && rm -rf "$WORK_DIR"
ARCHIVE="$WORK_DIR/Laprdus.xcarchive"
EXPORT_DIR="$WORK_DIR/export"
rm -rf "$ARCHIVE" "$EXPORT_DIR" "$WORK_DIR/dmg" "$WORK_DIR"/*.zip "$WORK_DIR"/*.plist
mkdir -p "$WORK_DIR" "$OUTPUT_DIR"

DMG_NAME="Laprdus-$APP_VERSION-macOS.dmg"
DMG="$OUTPUT_DIR/$DMG_NAME"
DSYM_ZIP="$OUTPUT_DIR/Laprdus-$APP_VERSION-macOS-dSYMs.zip"
CLI_TARBALL_NAME="laprdus-$ENGINE_VERSION-macos-universal.tar.xz"
rm -f "$DMG" "$DSYM_ZIP" "$OUTPUT_DIR/$CLI_TARBALL_NAME" "$OUTPUT_DIR/SHA256SUMS.txt"

notarize() {
    # $1: a .zip or .dmg; $2: a description for the log
    local file="$1" what="$2" result
    log_info "Submitting $what for notarization (this takes a few minutes)..."
    result="$(xcrun notarytool submit "$file" --keychain-profile "$NOTARY_PROFILE" --wait 2>&1)" || true
    echo "$result"
    if [[ "$result" != *"status: Accepted"* ]]; then
        local id
        id="$(awk '/^ *id: / { print $2; exit }' <<< "$result")"
        [[ -n "$id" ]] && xcrun notarytool log "$id" --keychain-profile "$NOTARY_PROFILE" || true
        die "Notarization of $what was not accepted."
    fi
    log_success "$what notarized"
}

# -----------------------------------------------------------------------------
# Step 1: voice data
# -----------------------------------------------------------------------------
log_info "Step 1: Generating voice data..."
scons --platform=macos --build-config=release voice-data >/dev/null
[[ -f data/voices/Josip.bin && -f data/voices/Vlado.bin ]] || die "Voice data was not generated."
log_success "Voice data ready"

# -----------------------------------------------------------------------------
# Step 2: command-line tools (universal, signed, tarball)
# -----------------------------------------------------------------------------
if (( BUILD_CLI )); then
    log_info "Step 2: Building the universal command-line tools..."
    # Without a local certificate the binaries get an ad hoc signature ("-"):
    # the linker signs only the arm64 slice by itself, and a fat binary with
    # an unsigned slice fails codesign --verify.
    scons --platform=macos --arch=universal --build-config=release \
          --codesign-identity="${IDENTITY:--}" macos-dist
    CLI_BUILD="build/macos-universal-release"
    CLI_DIST="dist/macos-universal"
    for f in "$CLI_BUILD/laprdus" "$CLI_BUILD/liblaprdus.dylib"; do
        archs="$(lipo -archs "$f")"
        [[ "$archs" == *arm64* && "$archs" == *x86_64* ]] || die "$f is not universal ($archs)."
        codesign --verify --strict --verbose=1 "$f" 2>&1 | sed 's/^/    /'
    done
    cp "$CLI_DIST/$CLI_TARBALL_NAME" "$OUTPUT_DIR/"
    if (( CLOUD_SIGNING )); then
        log_warning "Command-line tools built without a Developer ID signature (no local certificate)."
    elif (( NOTARIZE )); then
        # A tarball cannot be notarized or stapled, but the binaries inside
        # can: submitting them in a zip registers their signatures with
        # Apple, so Gatekeeper accepts them online if a download quarantines
        # them (tar itself does not).
        CLI_ZIP="$WORK_DIR/laprdus-cli.zip"
        ditto -c -k --keepParent "$CLI_DIST/laprdus-$ENGINE_VERSION-macos-universal" "$CLI_ZIP"
        notarize "$CLI_ZIP" "the command-line tools"
    fi
    log_success "Command-line tools: $OUTPUT_DIR/$CLI_TARBALL_NAME"
else
    log_info "Step 2: Skipping the command-line tools (--skip-cli)"
fi

# -----------------------------------------------------------------------------
# Step 3: archive and export the app
# -----------------------------------------------------------------------------
log_info "Step 3: Archiving the Laprdus app (Release, Apple Silicon + Intel)..."
xcodebuild archive \
    -project "$XCODE_PROJECT" -scheme "$SCHEME" -configuration Release \
    -destination 'generic/platform=macOS' \
    -archivePath "$ARCHIVE" \
    -derivedDataPath "$WORK_DIR/DerivedData" \
    -allowProvisioningUpdates \
    -quiet
[[ -d "$ARCHIVE/Products/Applications/Laprdus.app" ]] || die "The archive has no Laprdus.app."
log_success "Archive: $ARCHIVE"

log_info "Exporting with the \"$EXPORT_METHOD\" method..."
EXPORT_OPTIONS="$WORK_DIR/ExportOptions.plist"
SIGNING_CERTIFICATE_XML=""
if [[ -n "$IDENTITY" ]]; then
    SIGNING_CERTIFICATE_XML="	<key>signingCertificate</key>
	<string>$IDENTITY</string>"
fi
cat > "$EXPORT_OPTIONS" << PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>method</key>
	<string>$EXPORT_METHOD</string>
	<key>teamID</key>
	<string>$TEAM_ID</string>
	<key>signingStyle</key>
	<string>automatic</string>
$SIGNING_CERTIFICATE_XML
	<key>destination</key>
	<string>export</string>
</dict>
</plist>
PLIST
xcodebuild -exportArchive \
    -archivePath "$ARCHIVE" \
    -exportOptionsPlist "$EXPORT_OPTIONS" \
    -exportPath "$EXPORT_DIR" \
    -allowProvisioningUpdates \
    -quiet
APP="$EXPORT_DIR/Laprdus.app"
[[ -d "$APP" ]] || die "Export produced no Laprdus.app in $EXPORT_DIR."

APPEX="$APP/Contents/PlugIns/LaprdusVoices.appex"
[[ -d "$APPEX" ]] || die "The exported app has no LaprdusVoices.appex: the system voices would be missing."
for f in "$APP/Contents/MacOS/Laprdus" "$APPEX/Contents/MacOS/LaprdusVoices"; do
    archs="$(lipo -archs "$f")"
    [[ "$archs" == *arm64* && "$archs" == *x86_64* ]] || die "$f is not universal ($archs)."
done
codesign --verify --deep --strict --verbose=2 "$APP" 2>&1 | sed 's/^/    /'
codesign -dvv "$APP" 2>&1 | grep -E "^(Authority=|TeamIdentifier=|Timestamp=)" | sed 's/^/    /'
if ! (( DEVELOPMENT )); then
    authority="$(codesign -dvv "$APP" 2>&1 | grep '^Authority=Developer ID Application' || true)"
    [[ -n "$authority" ]] || die "The exported app is not signed with a Developer ID Application certificate."
fi
for f in "$APP" "$APPEX"; do
    flags="$(codesign -d --verbose "$f" 2>&1 | grep '^CodeDirectory' || true)"
    [[ "$flags" == *"runtime"* ]] || die "$f does not have the hardened runtime enabled ($flags)."
done
log_success "Exported and signed: $APP"

# -----------------------------------------------------------------------------
# Step 4: notarize and staple the app
# -----------------------------------------------------------------------------
if (( NOTARIZE )); then
    log_info "Step 4: Notarizing the app..."
    APP_ZIP="$WORK_DIR/Laprdus.zip"
    ditto -c -k --keepParent "$APP" "$APP_ZIP"
    notarize "$APP_ZIP" "the app"
    xcrun stapler staple "$APP"
    xcrun stapler validate "$APP"
else
    log_info "Step 4: Skipping notarization"
fi

# -----------------------------------------------------------------------------
# Step 5: disk image
# -----------------------------------------------------------------------------
log_info "Step 5: Creating the disk image..."
DMG_STAGE="$WORK_DIR/dmg"
mkdir -p "$DMG_STAGE"
ditto "$APP" "$DMG_STAGE/Laprdus.app"
ln -s /Applications "$DMG_STAGE/Applications"
# macOS 26 and later print a deprecation warning here in favour of
# `diskutil image create fromFolder`, which older systems do not have; hdiutil
# still works on both, so the warning can be ignored.
hdiutil create -volname "Laprdus $APP_VERSION" -srcfolder "$DMG_STAGE" -ov \
    -fs HFS+ -format UDZO -imagekey zlib-level=9 "$DMG" >/dev/null
if [[ -n "$IDENTITY" ]]; then
    codesign --force --sign "$IDENTITY" --timestamp "$DMG"
else
    log_warning "Disk image left unsigned (no local certificate); the app inside is signed."
fi
if (( NOTARIZE )); then
    notarize "$DMG" "the disk image"
    xcrun stapler staple "$DMG"
    xcrun stapler validate "$DMG"
fi
log_success "Disk image: $DMG"

# -----------------------------------------------------------------------------
# Step 6: Gatekeeper assessment, dSYMs, checksums
# -----------------------------------------------------------------------------
if (( DEVELOPMENT )); then
    log_warning "Development build: Gatekeeper would reject this on another Mac; it is for testing the pipeline only."
elif (( NOTARIZE )); then
    log_info "Step 6: Gatekeeper assessment..."
    spctl --assess --type execute --verbose=2 "$APP" 2>&1 | sed 's/^/    /'
    spctl --assess --type open --context context:primary-signature --verbose=2 "$DMG" 2>&1 | sed 's/^/    /'
    log_success "Gatekeeper accepts the app and the disk image"
else
    log_warning "Not notarized: Gatekeeper will refuse this build on other Macs until it is notarized."
fi

if [[ -d "$ARCHIVE/dSYMs" ]] && [[ -n "$(ls -A "$ARCHIVE/dSYMs")" ]]; then
    ditto -c -k --norsrc --keepParent "$ARCHIVE/dSYMs" "$DSYM_ZIP"
    log_info "Debug symbols: $DSYM_ZIP (keep them to symbolicate crash reports of this build)"
fi

(cd "$OUTPUT_DIR" && shasum -a 256 "$DMG_NAME" $( (( BUILD_CLI )) && echo "$CLI_TARBALL_NAME" ) > SHA256SUMS.txt && cat SHA256SUMS.txt | sed 's/^/    /')

echo ""
log_success "macOS release build completed"
echo "  $DMG"
(( BUILD_CLI )) && echo "  $OUTPUT_DIR/$CLI_TARBALL_NAME"
echo "  $OUTPUT_DIR/SHA256SUMS.txt"
