// -*- coding: utf-8 -*-
// dark_mode.hpp - Windows 10/11 dark mode support

#ifndef LAPRDUS_CONFIG_DARK_MODE_HPP
#define LAPRDUS_CONFIG_DARK_MODE_HPP

#ifdef _WIN32

#include <windows.h>

namespace laprdus {

/**
 * Initialize dark mode API functions.
 * Loads DwmSetWindowAttribute and SetWindowTheme dynamically
 * to maintain Windows 7 compatibility.
 */
void InitializeDarkMode();

/**
 * Cleanup dark mode resources.
 */
void CleanupDarkMode();

/**
 * Check if the system is using dark mode.
 * Returns false on Windows 7/8/8.1 or if dark mode is disabled.
 */
bool IsSystemDarkMode();

/**
 * Apply dark mode to a window's title bar.
 * Only works on Windows 10 1809 and later.
 * @param hWnd Window handle
 * @param dark True to enable dark title bar
 */
void ApplyDarkTitleBar(HWND hWnd, bool dark);

/**
 * Apply dark mode theme to a control.
 * Uses DarkMode_Explorer theme for supported controls.
 * @param hCtrl Control handle
 * @param dark True to enable dark mode
 */
void ApplyDarkModeToControl(HWND hCtrl, bool dark);

/**
 * Apply dark mode to all child controls of a window.
 * @param hWnd Parent window handle
 * @param dark True to enable dark mode
 */
void ApplyDarkModeToAllControls(HWND hWnd, bool dark);

/**
 * Get dark mode background brush.
 * Creates brush on first call.
 */
HBRUSH GetDarkBackgroundBrush();

/**
 * Get dark mode colors.
 */
COLORREF GetDarkBackgroundColor();
COLORREF GetDarkTextColor();
COLORREF GetDarkControlColor();

/**
 * Handle WM_CTLCOLORSTATIC for dark mode.
 * Returns brush handle if handled, NULL otherwise.
 */
HBRUSH OnCtlColorStatic(HDC hdc, bool dark);

/**
 * Handle WM_CTLCOLORDLG for dark mode.
 * Returns brush handle if handled, NULL otherwise.
 */
HBRUSH OnCtlColorDlg(bool dark);

} // namespace laprdus

#endif // _WIN32
#endif // LAPRDUS_CONFIG_DARK_MODE_HPP
