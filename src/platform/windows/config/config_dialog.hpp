// -*- coding: utf-8 -*-
// config_dialog.hpp - LaprdusTTS configuration dialog

#ifndef LAPRDUS_CONFIG_DIALOG_HPP
#define LAPRDUS_CONFIG_DIALOG_HPP

#ifdef _WIN32

#include <windows.h>
#include <commctrl.h>
#include "core/user_config.hpp"

namespace laprdus {

/**
 * ConfigDialog - Main configuration dialog for LaprdusTTS.
 *
 * Features:
 * - Voice selection dropdown
 * - Speed, pitch, volume sliders with force checkboxes
 * - Pause settings sliders (comma, sentence, newline)
 * - Options checkboxes (inflection, emoji, digits)
 * - Test button to speak sample text
 * - OK, Cancel, Apply buttons
 * - Dark mode support on Windows 10/11
 * - Localization (Croatian, Serbian, English)
 */
class ConfigDialog {
public:
    ConfigDialog();
    ~ConfigDialog();

    /**
     * Show the configuration dialog.
     * @param hInstance Application instance
     * @param hParent Optional parent window
     * @return IDOK if settings were saved, IDCANCEL otherwise
     */
    INT_PTR Show(HINSTANCE hInstance, HWND hParent = nullptr);

private:
    // Dialog procedure (static wrapper)
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

    // Instance methods for dialog handling
    INT_PTR OnInitDialog(HWND hDlg);
    INT_PTR OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);
    INT_PTR OnHScroll(HWND hDlg, WPARAM wParam, LPARAM lParam);
    INT_PTR OnCtlColor(HWND hDlg, UINT msg, HDC hdc, HWND hCtrl);
    INT_PTR OnSettingChange(HWND hDlg, LPARAM lParam);
    void OnDestroy();

    // Control initialization
    void CreateControls(HWND hDlg);
    void InitializeVoiceCombo(HWND hDlg);
    void InitializeSliders(HWND hDlg);
    void LoadSettingsToControls(HWND hDlg);

    // Control value handling
    void UpdateSliderValue(HWND hDlg, int sliderId, int valueId, bool isSpeed = false, bool isMs = false);
    void CollectSettingsFromControls(HWND hDlg);

    // Button handlers
    void OnOK(HWND hDlg);
    void OnCancel(HWND hDlg);
    void OnApply(HWND hDlg);
    void OnTest(HWND hDlg);
    void OnDictionaries(HWND hDlg);

    // Dark mode
    void ApplyDarkMode(HWND hDlg);
    void RefreshDarkMode(HWND hDlg);

    // Localization
    std::wstring LoadLocalizedString(UINT id);
    void SetControlText(HWND hDlg, int ctrlId, UINT stringId);

    // Settings management
    UserConfig m_config;
    UserSettings m_settings;
    bool m_settingsChanged = false;
    bool m_darkMode = false;

    // Instance handle for resources
    HINSTANCE m_hInstance = nullptr;

    // Dialog handle (for callbacks)
    HWND m_hDlg = nullptr;
};

/**
 * Run the configuration dialog.
 * Entry point for the configuration GUI.
 * @param hInstance Application instance
 * @return Exit code
 */
int RunConfigDialog(HINSTANCE hInstance);

} // namespace laprdus

#endif // _WIN32
#endif // LAPRDUS_CONFIG_DIALOG_HPP
