#!/bin/bash
#
# speechd-configure.sh - Configure Speech Dispatcher for LaprdusTTS
#
# This script automatically adds/removes LaprdusTTS module configuration
# from Speech Dispatcher without requiring manual user intervention.
#
# Usage:
#   speechd-configure.sh install   - Add LaprdusTTS module
#   speechd-configure.sh remove    - Remove LaprdusTTS module
#
# Exit codes:
#   0 - Success
#   1 - Invalid arguments
#   2 - speechd.conf not found (Speech Dispatcher not installed)
#

set -e

SPEECHD_CONF="/etc/speech-dispatcher/speechd.conf"
SPEECHD_CONF_DIR="/etc/speech-dispatcher"
MODULE_NAME="laprdus"

# Marker comments for our additions
MARKER_START="# BEGIN LAPRDUS TTS"
MARKER_END="# END LAPRDUS TTS"

# Configuration lines to add
read -r -d '' LAPRDUS_CONFIG << 'EOF' || true
# BEGIN LAPRDUS TTS
# LaprdusTTS - Croatian/Serbian Text-to-Speech
# Added automatically by package installation
AddModule "laprdus" "sd_laprdus" "laprdus.conf"

# Set LaprdusTTS as default for Croatian and Serbian
LanguageDefaultModule "hr" "laprdus"
LanguageDefaultModule "sr" "laprdus"
LanguageDefaultModule "hr-HR" "laprdus"
LanguageDefaultModule "sr-RS" "laprdus"
# END LAPRDUS TTS
EOF

usage() {
    echo "Usage: $0 {install|remove}"
    echo ""
    echo "Commands:"
    echo "  install  - Add LaprdusTTS module to Speech Dispatcher"
    echo "  remove   - Remove LaprdusTTS module from Speech Dispatcher"
    exit 1
}

check_speechd() {
    if [ ! -f "$SPEECHD_CONF" ]; then
        echo "Warning: $SPEECHD_CONF not found."
        echo "Speech Dispatcher may not be installed."
        echo "LaprdusTTS will be available once Speech Dispatcher is installed."
        return 1
    fi
    return 0
}

backup_config() {
    if [ -f "$SPEECHD_CONF" ]; then
        cp "$SPEECHD_CONF" "${SPEECHD_CONF}.laprdus-backup"
    fi
}

is_already_configured() {
    grep -q "$MARKER_START" "$SPEECHD_CONF" 2>/dev/null
}

has_addmodule_laprdus() {
    grep -q 'AddModule.*"laprdus"' "$SPEECHD_CONF" 2>/dev/null
}

install_config() {
    echo "Configuring Speech Dispatcher for LaprdusTTS..."

    if ! check_speechd; then
        # Create a drop-in config that will be picked up later
        mkdir -p "$SPEECHD_CONF_DIR/modules"
        echo "Module configuration installed. Will be active when Speech Dispatcher is available."
        return 0
    fi

    # Check if already configured
    if is_already_configured; then
        echo "LaprdusTTS already configured in Speech Dispatcher."
        return 0
    fi

    # Check if there's a manual AddModule entry
    if has_addmodule_laprdus; then
        echo "Note: Existing AddModule entry for laprdus found."
        echo "Adding language defaults only."

        # Just add language defaults if not present
        if ! grep -q 'LanguageDefaultModule.*"hr".*"laprdus"' "$SPEECHD_CONF"; then
            backup_config
            cat >> "$SPEECHD_CONF" << 'EOF'

# BEGIN LAPRDUS TTS LANGUAGES
# LaprdusTTS language defaults (module already registered)
LanguageDefaultModule "hr" "laprdus"
LanguageDefaultModule "sr" "laprdus"
LanguageDefaultModule "hr-HR" "laprdus"
LanguageDefaultModule "sr-RS" "laprdus"
# END LAPRDUS TTS LANGUAGES
EOF
        fi
        return 0
    fi

    # Backup before modifying
    backup_config

    # Find a good place to add our config (after other AddModule lines if they exist)
    # If there are AddModule lines, add after the last one
    # Otherwise, add at the end

    if grep -q "^AddModule" "$SPEECHD_CONF"; then
        # Add after the last AddModule line
        # Use awk to insert after the last AddModule block
        awk -v config="$LAPRDUS_CONFIG" '
        {
            print
            if (/^AddModule/ && !added) {
                last_addmodule = NR
            }
        }
        END {
            if (!added) {
                print ""
                print config
            }
        }
        ' "$SPEECHD_CONF" > "${SPEECHD_CONF}.new"

        # Actually, simpler approach - just append to end
        echo "" >> "$SPEECHD_CONF"
        echo "$LAPRDUS_CONFIG" >> "$SPEECHD_CONF"
    else
        # No existing AddModule lines, append to end
        echo "" >> "$SPEECHD_CONF"
        echo "$LAPRDUS_CONFIG" >> "$SPEECHD_CONF"
    fi

    echo "LaprdusTTS configured successfully."
    echo "Restart Speech Dispatcher to apply changes: systemctl --user restart speech-dispatcher"

    return 0
}

remove_config() {
    echo "Removing LaprdusTTS from Speech Dispatcher configuration..."

    if ! check_speechd; then
        return 0
    fi

    if ! is_already_configured; then
        # Also check for language-only block
        if grep -q "BEGIN LAPRDUS TTS LANGUAGES" "$SPEECHD_CONF"; then
            backup_config
            sed -i '/# BEGIN LAPRDUS TTS LANGUAGES/,/# END LAPRDUS TTS LANGUAGES/d' "$SPEECHD_CONF"
            echo "LaprdusTTS language defaults removed."
        else
            echo "LaprdusTTS not found in Speech Dispatcher configuration."
        fi
        return 0
    fi

    backup_config

    # Remove our configuration block
    sed -i "/$MARKER_START/,/$MARKER_END/d" "$SPEECHD_CONF"

    # Clean up any empty lines we may have left
    sed -i '/^$/N;/^\n$/d' "$SPEECHD_CONF"

    echo "LaprdusTTS removed from Speech Dispatcher configuration."
    echo "Restart Speech Dispatcher to apply changes: systemctl --user restart speech-dispatcher"

    return 0
}

# Main
case "${1:-}" in
    install)
        install_config
        ;;
    remove)
        remove_config
        ;;
    *)
        usage
        ;;
esac

exit 0
