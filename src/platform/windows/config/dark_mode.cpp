// -*- coding: utf-8 -*-
// dark_mode.cpp - Windows 10/11 dark mode support implementation

#ifdef _WIN32

#include "dark_mode.hpp"
#include <dwmapi.h>

namespace laprdus {

// Dark mode constants (not defined in older SDKs)
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 19
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

// Function pointer types for dynamic loading
typedef HRESULT (WINAPI *pfnDwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);
typedef HRESULT (WINAPI *pfnSetWindowTheme)(HWND, LPCWSTR, LPCWSTR);

// Global function pointers and resources
static pfnDwmSetWindowAttribute g_DwmSetWindowAttribute = nullptr;
static pfnSetWindowTheme g_SetWindowTheme = nullptr;
static HMODULE g_hDwmapi = nullptr;
static HMODULE g_hUxtheme = nullptr;
static HBRUSH g_hDarkBgBrush = nullptr;

// Dark mode colors
static constexpr COLORREF DARK_BACKGROUND = RGB(32, 32, 32);
static constexpr COLORREF DARK_TEXT = RGB(255, 255, 255);
static constexpr COLORREF DARK_CONTROL = RGB(45, 45, 45);

void InitializeDarkMode() {
    // Load dwmapi.dll for dark title bar
    g_hDwmapi = LoadLibraryW(L"dwmapi.dll");
    if (g_hDwmapi) {
        g_DwmSetWindowAttribute = (pfnDwmSetWindowAttribute)
            GetProcAddress(g_hDwmapi, "DwmSetWindowAttribute");
    }

    // Load uxtheme.dll for control themes
    g_hUxtheme = LoadLibraryW(L"uxtheme.dll");
    if (g_hUxtheme) {
        g_SetWindowTheme = (pfnSetWindowTheme)
            GetProcAddress(g_hUxtheme, "SetWindowTheme");
    }
}

void CleanupDarkMode() {
    if (g_hDarkBgBrush) {
        DeleteObject(g_hDarkBgBrush);
        g_hDarkBgBrush = nullptr;
    }

    if (g_hDwmapi) {
        FreeLibrary(g_hDwmapi);
        g_hDwmapi = nullptr;
    }

    if (g_hUxtheme) {
        FreeLibrary(g_hUxtheme);
        g_hUxtheme = nullptr;
    }

    g_DwmSetWindowAttribute = nullptr;
    g_SetWindowTheme = nullptr;
}

bool IsSystemDarkMode() {
    // Check Windows 10 1809+ dark mode registry
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD value = 1;  // Default to light mode
        DWORD size = sizeof(value);
        RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr,
            (LPBYTE)&value, &size);
        RegCloseKey(hKey);
        return (value == 0);  // 0 = dark mode
    }
    return false;  // Default to light mode on older Windows
}

void ApplyDarkTitleBar(HWND hWnd, bool dark) {
    if (!g_DwmSetWindowAttribute) return;

    BOOL useDark = dark ? TRUE : FALSE;

    // Try Windows 10 2004+ attribute first
    HRESULT hr = g_DwmSetWindowAttribute(hWnd,
        DWMWA_USE_IMMERSIVE_DARK_MODE, &useDark, sizeof(useDark));

    // Fall back to pre-20H1 attribute if that fails
    if (FAILED(hr)) {
        g_DwmSetWindowAttribute(hWnd,
            DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &useDark, sizeof(useDark));
    }
}

void ApplyDarkModeToControl(HWND hCtrl, bool dark) {
    if (!g_SetWindowTheme) return;

    if (dark) {
        // DarkMode_Explorer theme provides partial dark mode for common controls
        g_SetWindowTheme(hCtrl, L"DarkMode_Explorer", nullptr);
    } else {
        // Reset to default theme
        g_SetWindowTheme(hCtrl, nullptr, nullptr);
    }
}

// Callback for EnumChildWindows
static BOOL CALLBACK ApplyDarkModeCallback(HWND hChild, LPARAM lParam) {
    bool dark = (lParam != 0);
    ApplyDarkModeToControl(hChild, dark);
    return TRUE;
}

void ApplyDarkModeToAllControls(HWND hWnd, bool dark) {
    EnumChildWindows(hWnd, ApplyDarkModeCallback, dark ? 1 : 0);
}

HBRUSH GetDarkBackgroundBrush() {
    if (!g_hDarkBgBrush) {
        g_hDarkBgBrush = CreateSolidBrush(DARK_BACKGROUND);
    }
    return g_hDarkBgBrush;
}

COLORREF GetDarkBackgroundColor() {
    return DARK_BACKGROUND;
}

COLORREF GetDarkTextColor() {
    return DARK_TEXT;
}

COLORREF GetDarkControlColor() {
    return DARK_CONTROL;
}

HBRUSH OnCtlColorStatic(HDC hdc, bool dark) {
    if (dark) {
        SetTextColor(hdc, DARK_TEXT);
        SetBkColor(hdc, DARK_BACKGROUND);
        return GetDarkBackgroundBrush();
    }
    return nullptr;
}

HBRUSH OnCtlColorEdit(HDC hdc, bool dark) {
    if (dark) {
        SetTextColor(hdc, DARK_TEXT);
        SetBkColor(hdc, DARK_CONTROL);
        static HBRUSH hEditBrush = nullptr;
        if (!hEditBrush) {
            hEditBrush = CreateSolidBrush(DARK_CONTROL);
        }
        return hEditBrush;
    }
    return nullptr;
}

HBRUSH OnCtlColorDlg(bool dark) {
    if (dark) {
        return GetDarkBackgroundBrush();
    }
    return nullptr;
}

} // namespace laprdus

#endif // _WIN32
