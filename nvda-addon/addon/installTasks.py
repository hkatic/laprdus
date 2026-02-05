# -*- coding: utf-8 -*-
"""Installation tasks for LaprdusTTS NVDA addon.

Handles cleanup of deprecated files and config values during addon upgrade.
"""

import os


def onInstall():
    """Handle addon installation/upgrade.

    Called after NVDA extracts the addon to the addons directory.
    - Removes deprecated files from pre-1.0.0 folder structure
    - Removes deprecated config values from NVDA config
    """
    # Get the path to the synthDrivers/laprdus directory
    # This file is in addon/, so we go to addon/synthDrivers/laprdus/
    addon_dir = os.path.dirname(os.path.abspath(__file__))
    synth_dir = os.path.join(addon_dir, "synthDrivers", "laprdus")

    # Old files to remove (were at root level, now in subdirectories)
    deprecated_files = [
        # Old voice data files (now in voices/)
        "Josip.bin",
        "Vlado.bin",
        # Old dictionary files (now in dictionaries/)
        "internal.json",
        "spelling.json",
        "emoji.json",
    ]

    for filename in deprecated_files:
        old_path = os.path.join(synth_dir, filename)
        if os.path.exists(old_path):
            try:
                os.remove(old_path)
            except Exception:
                # Don't fail installation if cleanup fails
                pass

    # Remove deprecated config values
    _removeDeprecatedConfigValues()


def _removeDeprecatedConfigValues():
    """Remove deprecated Laprdus synth driver config values.

    These settings are now managed via the shared settings.json file
    (edited by laprdgui.exe) and are no longer controlled via NVDA
    synth driver settings:
    - sentencePause
    - commaPause
    - newlinePause
    - digitByDigit
    - inflectionEnabled
    """
    try:
        import config

        # Check if speechSynths section exists
        if "speechSynths" not in config.conf:
            return

        if "laprdus" not in config.conf["speechSynths"]:
            return

        laprdus_conf = config.conf["speechSynths"]["laprdus"]

        # List of deprecated keys to remove
        deprecated_keys = [
            "sentencePause",
            "commaPause",
            "newlinePause",
            "digitByDigit",
            "inflectionEnabled",
        ]

        for key in deprecated_keys:
            if key in laprdus_conf:
                del laprdus_conf[key]

    except Exception:
        # Don't fail installation if config cleanup fails
        pass
