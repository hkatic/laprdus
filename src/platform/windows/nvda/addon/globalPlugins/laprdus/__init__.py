# -*- coding: utf-8 -*-
# LaprdusTTS Global Plugin
# Adds the Laprdus menu to NVDA's main menu, hands Laprdus the text as
# written whichever NVDA driver it speaks through (_speechFilter.py), and
# gives an NVDA before 2024.4 the add-on's symbol dictionaries (_symbols.py)

from __future__ import unicode_literals

import os
import sys
import subprocess
import globalPluginHandler
import gui
import languageHandler
import wx
import addonHandler
from logHandler import log

from . import _speechFilter, _symbols

addonHandler.initTranslation()

# NVDA 2024.2 and later; an older NVDA gets _speechFilter.ProcessTextHook
try:
    from speech.extensions import filter_speechSequence
except ImportError:
    filter_speechSequence = None
try:
    from speech.commands import LangChangeCommand
except ImportError:
    LangChangeCommand = None

# The add-on's folder: this file is in addon/globalPlugins/laprdus/
_ADDON_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def _speakModule():
    """The module whose speak() hands each text to its processText(): in
    NVDA 2021.1 and later speech.speech, in 2019.3 to 2020.4 the speech
    package itself; None when this NVDA has no such pair."""
    try:
        from speech import speech as module
    except ImportError:
        import speech as module
    if callable(getattr(module, "speak", None)) and callable(getattr(module, "processText", None)):
        return module
    return None


def _speechContext(speaks, level=None):
    """NVDA's settings for _speechFilter.filter_sequence() and filter_text(),
    read for one sequence or text; `level` is the punctuation level NVDA
    processes the text at, when it is known (else the user's)."""
    import config
    import speech
    conf = config.conf["speech"]
    level = int(conf["symbolLevel"] if level is None else level)
    try:
        language = speech.getCurrentLanguage()
    except Exception:
        language = languageHandler.getLanguage()
    kept = {}

    def keepsPeriod(locale):
        # Whether NVDA's table for the locale leaves a period between digits
        # alone at this level (it does in hr, sr, bs and en, not in sl);
        # when NVDA cannot tell, the period is carried, which is harmless
        if locale not in kept:
            try:
                import characterProcessing
                kept[locale] = characterProcessing.processSpeechSymbols(locale, "0.0", level) == "0.0"
            except Exception:
                kept[locale] = False
        return kept[locale]

    return _speechFilter.Context(
        level=level,
        language=language,
        auto_language=bool(conf["autoLanguageSwitching"]),
        auto_dialect=bool(conf["autoDialectSwitching"]),
        keeps_period=keepsPeriod,
        speaks=speaks,
        lang_change=LangChangeCommand,
    )


class GlobalPlugin(globalPluginHandler.GlobalPlugin):
    """Global plugin to add Laprdus menu to NVDA main menu."""

    def __init__(self):
        super(GlobalPlugin, self).__init__()
        self._laprdusMenu = None
        self._laprdusMenuItem = None
        # The text NVDA hands Laprdus, before NVDA's symbol processing
        self._speechFilterRegistered = False
        self._processTextHook = None
        if filter_speechSequence is not None:
            try:
                filter_speechSequence.register(self._filterSpeech)
                self._speechFilterRegistered = True
            except Exception:
                log.debugWarning("Laprdus: the speech filter could not be registered", exc_info=True)
        if not self._speechFilterRegistered:
            # NVDA before 2024.2 (Windows 7 and 8 have 2023.3.4 at most)
            try:
                module = _speakModule()
                if module is not None:
                    self._processTextHook = _speechFilter.ProcessTextHook(module, self._filterText)
            except Exception:
                log.debugWarning("Laprdus: the speech filter could not be put before processText", exc_info=True)
        # NVDA before 2024.4 ignores the manifest's symbol dictionaries
        self._symbolDictionaryHook = None
        try:
            import characterProcessing
            if not _symbols.reads_manifest_dictionaries(characterProcessing):
                dictionaries = _symbols.addon_dictionaries(_ADDON_DIR)
                if dictionaries:
                    self._symbolDictionaryHook = _symbols.SymbolDictionaryHook(characterProcessing, dictionaries, log)
        except Exception:
            log.debugWarning("Laprdus: the add-on's symbol dictionaries could not be loaded", exc_info=True)
        # Schedule menu creation on main thread
        wx.CallAfter(self._createMenu)

    def _filterSpeech(self, speechSequence):
        """speech.extensions.filter_speechSequence: while Laprdus speaks, its
        texts as written (see _speechFilter.py); any other synthesizer's
        sequence is left as it is."""
        try:
            import synthDriverHandler
            speaks = _speechFilter.laprdus_speaker(synthDriverHandler.getSynth())
            if speaks is None:
                return speechSequence
            return _speechFilter.filter_sequence(speechSequence, _speechContext(speaks))
        except Exception:
            log.debugWarning("Laprdus: the speech filter failed", exc_info=True)
            return speechSequence

    def _filterText(self, locale, text, symbolLevel):
        """_speechFilter.ProcessTextHook, NVDA before 2024.2: one text on its
        way to NVDA's symbol processing, in the language NVDA speaks it in,
        as _filterSpeech() would hand it on; any other synthesizer's text is
        left as it is."""
        # Most texts have no digit, and nothing to rewrite
        if not isinstance(text, str) or not any(c.isdigit() for c in text):
            return text
        try:
            import synthDriverHandler
            speaks = _speechFilter.laprdus_speaker(synthDriverHandler.getSynth())
            if speaks is None:
                return text
            return _speechFilter.filter_text(text, locale or "", _speechContext(speaks, symbolLevel))
        except Exception:
            log.debugWarning("Laprdus: the speech filter failed", exc_info=True)
            return text

    def _createMenu(self):
        """Create the Laprdus submenu in NVDA main menu."""
        try:
            mainMenu = gui.mainFrame.sysTrayIcon.menu

            # Create Laprdus submenu
            self._laprdusMenu = wx.Menu()

            # Add "Laprdus Configurator" item
            # Translators: Menu item to open Laprdus configuration GUI
            configuratorItem = self._laprdusMenu.Append(
                wx.ID_ANY,
                # Translators: Menu item label for Laprdus Configurator
                _("Laprdus &Configurator..."),
                # Translators: Description for Laprdus Configurator menu item
                _("Open the Laprdus TTS configuration window")
            )
            gui.mainFrame.sysTrayIcon.Bind(
                wx.EVT_MENU,
                self._onConfigurator,
                configuratorItem
            )

            # Add separator
            self._laprdusMenu.AppendSeparator()

            # Add "Laprdus on the Web" item
            webItem = self._laprdusMenu.Append(
                wx.ID_ANY,
                # Translators: Menu item to open Laprdus website
                _("Laprdus on the &Web"),
                # Translators: Description for Laprdus on the Web menu item
                _("Open the Laprdus website in your browser")
            )
            gui.mainFrame.sysTrayIcon.Bind(
                wx.EVT_MENU,
                self._onWeb,
                webItem
            )

            # Find Help menu position and insert before it
            # This places Laprdus after Tools and before Help
            helpIndex = None
            for i in range(mainMenu.GetMenuItemCount()):
                item = mainMenu.FindItemByPosition(i)
                if item:
                    label = item.GetItemLabelText().replace('&', '').lower()
                    # English; Croatian and Serbian; Serbian in Cyrillic; Slovenian
                    if label in ('help', 'pomoć', 'помоћ', 'pomoč'):
                        helpIndex = i
                        break

            # Translators: Name of the Laprdus submenu in NVDA main menu
            if helpIndex is not None:
                # Insert before Help menu
                self._laprdusMenuItem = mainMenu.Insert(
                    helpIndex,
                    wx.ID_ANY,
                    _("&Laprdus"),
                    self._laprdusMenu
                )
            else:
                # Fallback: append if Help not found
                self._laprdusMenuItem = mainMenu.AppendSubMenu(
                    self._laprdusMenu,
                    _("&Laprdus")
                )
        except Exception:
            # Don't fail if menu creation fails
            pass

    def _onConfigurator(self, evt):
        """Launch the Laprdus Configurator GUI."""
        # Detect architecture
        is64bit = sys.maxsize > 2**32

        synth_path = os.path.join(_ADDON_DIR, "synthDrivers", "laprdus")

        # Select appropriate executable. On Windows 7, 8 and 8.1 (a 32-bit
        # NVDA there) it is the build made for those systems, when the
        # add-on carries it.
        if is64bit:
            exe_name = "laprdgui64.exe"
        else:
            exe_name = "laprdgui32.exe"
            try:
                version = sys.getwindowsversion()
                major = getattr(version, "platform_version", (version.major,))[0]
            except Exception:
                major = 10
            if major < 10 and os.path.exists(os.path.join(synth_path, "laprdgui32_legacy.exe")):
                exe_name = "laprdgui32_legacy.exe"

        exe_path = os.path.join(synth_path, exe_name)

        if os.path.exists(exe_path):
            try:
                # The Configurator opened from NVDA speaks NVDA's language
                subprocess.Popen([exe_path, "--language=" + languageHandler.getLanguage()], shell=False)
            except Exception as e:
                gui.messageBox(
                    # Translators: Error message when configurator fails to launch
                    _("Failed to launch Laprdus Configurator: {}").format(str(e)),
                    # Translators: Error dialog title
                    _("Error"),
                    wx.OK | wx.ICON_ERROR
                )
        else:
            gui.messageBox(
                # Translators: Error message when configurator executable not found
                _("Laprdus Configurator not found at: {}").format(exe_path),
                # Translators: Error dialog title
                _("Error"),
                wx.OK | wx.ICON_ERROR
            )

    def _onWeb(self, evt):
        """Open the Laprdus website."""
        import webbrowser
        webbrowser.open("https://hrvojekatic.com/laprdus")

    def terminate(self):
        """Clean up when plugin is terminated."""
        if self._speechFilterRegistered:
            try:
                filter_speechSequence.unregister(self._filterSpeech)
            except Exception:
                pass
            self._speechFilterRegistered = False
        for name in ("_processTextHook", "_symbolDictionaryHook"):
            hook = getattr(self, name, None)
            if hook is not None:
                try:
                    hook.remove()
                except Exception:
                    log.debugWarning("Laprdus: %s could not be removed" % name, exc_info=True)
                setattr(self, name, None)
        try:
            if self._laprdusMenuItem:
                mainMenu = gui.mainFrame.sysTrayIcon.menu
                mainMenu.Remove(self._laprdusMenuItem)
        except Exception:
            pass
        super(GlobalPlugin, self).terminate()
