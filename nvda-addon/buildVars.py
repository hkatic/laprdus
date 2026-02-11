# -*- coding: utf-8 -*-
# Build customizations for LaprdusTTS NVDA Add-on
# Change this file instead of sconstruct or manifest files, whenever possible.

from site_scons.site_tools.NVDATool.typings import AddonInfo, BrailleTables, SymbolDictionaries

# Since some strings in `addon_info` are translatable,
# we need to include them in the .po files.
# Gettext recognizes only strings given as parameters to the `_` function.
# To avoid initializing translations in this module we simply import a "fake" `_` function
# which returns whatever is given to it as an argument.
from site_scons.site_tools.NVDATool.utils import _


# Add-on information variables
addon_info = AddonInfo(
    # add-on Name/identifier, internal for NVDA
    addon_name="laprdus",
    # Add-on summary/title, usually the user visible name of the add-on
    # Translators: Summary/title for this add-on
    addon_summary=_("Laprdus TTS"),
    # Add-on description
    # Translators: Long description to be shown for this add-on
    addon_description=_("""Text-to-speech synthesizer for NVDA.
Laprdus is a concatenative speech synthesizer using pre-recorded phoneme samples.
Supports Croatian and Serbian language with natural-sounding prosody and inflection."""),
    # version
    addon_version="1.0.0",
    # Brief changelog for this version
    # Translators: what's new content for the add-on version
    addon_changelog=_("""Initial release of Laprdus TTS for NVDA.
Features:
- Croatian and Serbian text-to-speech synthesis
- Available voices: Josip (Croatian) and Vlado (Serbian), plus additional voice presets (Child, Grandma and Grandpa)
- Adjustable rate, pitch, and volume
"""),
    # Author(s)
    addon_author="Hrvoje Katić <hrvojekatic@gmail.com>",
    # URL for the add-on documentation support
    addon_url="https://hrvojekatic.com/laprdus",
    # URL for the add-on repository where the source code can be found
    addon_sourceURL="https://github.com/hkatic/laprdus",
    # Documentation file name
    addon_docFileName="readme.html",
    # Minimum NVDA version supported (e.g. "2019.3.0", minor version is optional)
    addon_minimumNVDAVersion="2019.3",
    # Last NVDA version supported/tested (e.g. "2024.4.0", ideally more recent than minimum version)
    addon_lastTestedNVDAVersion="2026.1",
    # Add-on update channel (default is None, denoting stable releases,
    # and for development releases, use "dev".)
    addon_updateChannel=None,
    # Add-on license such as GPL 2
    addon_license="GPL 2",
    # URL for the license document the ad-on is licensed under
    addon_licenseURL=None,
)

# Define the python files that are the sources of your add-on.
# You can either list every file (using ""/") as a path separator,
# or use glob expressions.
pythonSources: list[str] = [
    "addon/synthDrivers/laprdus/*.py",
    "addon/globalPlugins/laprdus/*.py",
]

# Files that contain strings for translation. Usually your python sources
i18nSources: list[str] = pythonSources + ["buildVars.py"]

# Files that will be ignored when building the nvda-addon file
# Paths are relative to the addon directory, not to the root directory of your addon sources.
excludedFiles: list[str] = [
    "*.pyc",
    "*.pyo",
    "__pycache__/*",
    "*.exp",
    "*.lib",
    "*.pdb",
]

# Base language for the NVDA add-on
baseLanguage: str = "en"

# Markdown extensions for add-on documentation
markdownExtensions: list[str] = []

# Custom braille translation tables
brailleTables: BrailleTables = {}

# Custom speech symbol dictionaries
symbolDictionaries: SymbolDictionaries = {}
