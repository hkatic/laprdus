#!/bin/bash
# =============================================================================
# LaprdusTTS: an iPhone and iPad build to TestFlight
# =============================================================================
# Archives the Laprdus app with its LaprdusVoices speech extension for iOS
# (Release), signs it for App Store Connect and uploads it, where it appears
# in TestFlight once Apple has processed it (some minutes). The nightly
# builds (.github/workflows/nightly.yml) run it on GitHub Actions with an
# App Store Connect API key; on a Mac with the team's account signed in to
# Xcode it runs without one.
#
# Requirements: Xcode, Rust (rustup) with the iOS target
# (rustup target add aarch64-apple-ios), the team's Apple Developer Program
# membership, and the app's record in App Store Connect (bundle id
# com.hrvojekatic.Laprdus).
#
# Usage:
#   ./scripts/upload-testflight.sh --build-number 41.612 \
#       --api-key AuthKey_XXXX.p8 --api-key-id XXXX --api-issuer <issuer id>
#
# Options:
#   --build-number N    The build number (CURRENT_PROJECT_VERSION), instead of
#                       the project's. App Store Connect refuses a number it
#                       has had for the version (MARKETING_VERSION), and one
#                       lower than the last: the nightlies use
#                       <project's number>.<commit count>, so a release whose
#                       number was raised must be uploaded before the next
#                       nightly.
#   --api-key FILE      An App Store Connect API key (AuthKey_*.p8), with
#   --api-key-id ID     its key ID and issuer ID, instead of the Xcode
#   --api-issuer ID     account. The three go together.
#   --export-only       Sign and export the .ipa into the work directory
#                       without uploading it.
#   --clean             Delete the work directory (build/ios-release) first.
#   -h, --help          This text.
# =============================================================================

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

XCODE_PROJECT="src/platform/apple/Laprdus/Laprdus.xcodeproj"
SCHEME="Laprdus"
WORK_DIR="$PROJECT_ROOT/build/ios-release"

BUILD_NUMBER=""
API_KEY=""
API_KEY_ID=""
API_ISSUER=""
DESTINATION="upload"
CLEAN=0

die() { echo "[ERROR] $1" >&2; exit 1; }
usage() { awk 'NR > 4 { if ($0 ~ /^# =+$/) exit; sub(/^# ?/, ""); print }' "${BASH_SOURCE[0]}"; }

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-number) BUILD_NUMBER="${2:?--build-number needs a value}"; shift ;;
        --api-key)      API_KEY="${2:?--api-key needs a file}"; shift ;;
        --api-key-id)   API_KEY_ID="${2:?--api-key-id needs a value}"; shift ;;
        --api-issuer)   API_ISSUER="${2:?--api-issuer needs a value}"; shift ;;
        --export-only)  DESTINATION="export" ;;
        --clean)        CLEAN=1 ;;
        -h|--help)      usage; exit 0 ;;
        *)              usage >&2; die "Unknown option: $1" ;;
    esac
    shift
done

[[ "$(uname -s)" == "Darwin" ]] || die "This script runs on macOS only."
command -v xcodebuild >/dev/null 2>&1 || die "xcodebuild not found: install Xcode."

# Arrays, expanded as ${a[@]+"${a[@]}"}: macOS's bash 3.2 calls an empty
# array unbound under set -u.
XCODE_AUTH=()
if [[ -n "$API_KEY$API_KEY_ID$API_ISSUER" ]]; then
    [[ -n "$API_KEY" && -n "$API_KEY_ID" && -n "$API_ISSUER" ]] || die "--api-key, --api-key-id and --api-issuer go together."
    [[ -f "$API_KEY" ]] || die "No API key file $API_KEY."
    XCODE_AUTH=(-authenticationKeyPath "$API_KEY" -authenticationKeyID "$API_KEY_ID" -authenticationKeyIssuerID "$API_ISSUER")
fi
XCODE_SETTINGS=()
[[ -n "$BUILD_NUMBER" ]] && XCODE_SETTINGS=(CURRENT_PROJECT_VERSION="$BUILD_NUMBER")

BUILD_SETTINGS="$(xcodebuild -showBuildSettings -project "$XCODE_PROJECT" -scheme "$SCHEME" \
                  -configuration Release -destination 'generic/platform=iOS' 2>/dev/null)"
setting() { awk -v key="$1" '$1 == key && $2 == "=" { $1 = ""; $2 = ""; sub(/^ +/, ""); print; exit }' <<< "$BUILD_SETTINGS"; }
APP_VERSION="$(setting MARKETING_VERSION)"
TEAM_ID="$(setting DEVELOPMENT_TEAM)"
[[ -n "$APP_VERSION" && -n "$TEAM_ID" ]] || die "Could not read MARKETING_VERSION / DEVELOPMENT_TEAM from $XCODE_PROJECT."
APP_BUILD="${BUILD_NUMBER:-$(setting CURRENT_PROJECT_VERSION)}"
echo "Laprdus $APP_VERSION ($APP_BUILD) for iPhone and iPad, team $TEAM_ID: $DESTINATION"

(( CLEAN )) && rm -rf "$WORK_DIR"
ARCHIVE="$WORK_DIR/Laprdus.xcarchive"
EXPORT_DIR="$WORK_DIR/export"
rm -rf "$ARCHIVE" "$EXPORT_DIR" "$WORK_DIR/ExportOptions.plist"
mkdir -p "$WORK_DIR"

# Both targets bundle the voice data and the dictionaries.
cargo xtask data >/dev/null

xcodebuild archive \
    -project "$XCODE_PROJECT" -scheme "$SCHEME" -configuration Release \
    -destination 'generic/platform=iOS' \
    -archivePath "$ARCHIVE" \
    -derivedDataPath "$WORK_DIR/DerivedData" \
    -allowProvisioningUpdates \
    ${XCODE_AUTH[@]+"${XCODE_AUTH[@]}"} \
    ${XCODE_SETTINGS[@]+"${XCODE_SETTINGS[@]}"} \
    -quiet
[[ -d "$ARCHIVE/Products/Applications/Laprdus.app" ]] || die "The archive has no Laprdus.app."
[[ -d "$ARCHIVE/Products/Applications/Laprdus.app/PlugIns/LaprdusVoices.appex" ]] ||
    die "The archive has no LaprdusVoices.appex: the system voices would be missing."

# manageAppVersionAndBuildNumber off: the build number is ours, and Xcode
# would otherwise raise it by itself.
cat > "$WORK_DIR/ExportOptions.plist" << PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>method</key>
	<string>app-store-connect</string>
	<key>teamID</key>
	<string>$TEAM_ID</string>
	<key>signingStyle</key>
	<string>automatic</string>
	<key>destination</key>
	<string>$DESTINATION</string>
	<key>manageAppVersionAndBuildNumber</key>
	<false/>
	<key>uploadSymbols</key>
	<true/>
</dict>
</plist>
PLIST
xcodebuild -exportArchive \
    -archivePath "$ARCHIVE" \
    -exportOptionsPlist "$WORK_DIR/ExportOptions.plist" \
    -exportPath "$EXPORT_DIR" \
    -allowProvisioningUpdates \
    ${XCODE_AUTH[@]+"${XCODE_AUTH[@]}"}

if [[ "$DESTINATION" == upload ]]; then
    echo "Uploaded Laprdus $APP_VERSION ($APP_BUILD): it appears in TestFlight when App Store Connect has processed it."
else
    echo "Exported to $EXPORT_DIR (not uploaded)."
fi
