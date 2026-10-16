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
    - Renames a saved voice id of an earlier version to its 2.0 id
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

    # Map a saved voice id of an earlier version onto the current one
    _migrateVoiceId()


# Voice ids of earlier versions and the ids that replaced them in 2.0
# (the same table as LEGACY_VOICE_IDS in the synth driver)
_LEGACY_VOICE_IDS = {
    "djed": "djedo",
    "dijete": "detence",
    "baka": "baba",
    "klapa": "dalmacija_klapa",
    "pjevac": "lika_velebit",
    "pevac": "solunac",
    "solist": "sarajevo_bembasa",
}


def _migrateVoiceId():
    """Rename the saved Laprdus voice in NVDA's config to its 2.0 id.

    Version 2.0 renamed some voices (djed -> djedo, klapa -> dalmacija_klapa,
    pjevac -> lika_velebit, pevac -> solunac, solist -> sarajevo_bembasa;
    dijete and baka were earlier names of detence and baba). The DLL still
    accepts the old ids, but NVDA's voice combo compares the saved id with
    the driver's voice list, so the saved value is renamed here. Nothing
    else changes: a voice chosen under version 1.0 stays the voice (the
    language is a separate setting now, and every voice speaks every
    language).
    """
    try:
        import config

        if "speechSynths" not in config.conf:
            return

        if "laprdus" not in config.conf["speechSynths"]:
            return

        laprdus_conf = config.conf["speechSynths"]["laprdus"]
        voice = laprdus_conf.get("voice")
        if voice in _LEGACY_VOICE_IDS:
            laprdus_conf["voice"] = _LEGACY_VOICE_IDS[voice]

    except Exception:
        # Don't fail installation if config migration fails
        pass


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
