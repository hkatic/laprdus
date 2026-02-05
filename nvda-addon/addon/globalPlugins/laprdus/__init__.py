# -*- coding: utf-8 -*-
# LaprdusTTS Global Plugin
# Adds Laprdus menu to NVDA main menu

from __future__ import unicode_literals

import os
import sys
import subprocess
import globalPluginHandler
import gui
import wx
import addonHandler

addonHandler.initTranslation()


class GlobalPlugin(globalPluginHandler.GlobalPlugin):
    """Global plugin to add Laprdus menu to NVDA main menu."""

    def __init__(self):
        super(GlobalPlugin, self).__init__()
        self._laprdusMenu = None
        self._laprdusMenuItem = None
        # Schedule menu creation on main thread
        wx.CallAfter(self._createMenu)

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
                    if label in ('help', 'pomoć', 'помоћ'):  # English, Croatian, Serbian
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

        # Get addon path - this file is in addon/globalPlugins/laprdus/
        # We need to go up 3 levels to addon/, then to synthDrivers/laprdus/
        addon_path = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
        synth_path = os.path.join(addon_path, "synthDrivers", "laprdus")

        # Select appropriate executable
        if is64bit:
            exe_name = "laprdgui64.exe"
        else:
            exe_name = "laprdgui32.exe"

        exe_path = os.path.join(synth_path, exe_name)

        if os.path.exists(exe_path):
            try:
                subprocess.Popen([exe_path], shell=False)
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
        try:
            if self._laprdusMenuItem:
                mainMenu = gui.mainFrame.sysTrayIcon.menu
                mainMenu.Remove(self._laprdusMenuItem)
        except Exception:
            pass
        super(GlobalPlugin, self).terminate()
