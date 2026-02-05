// -*- coding: utf-8 -*-
// config_dialog.cpp - LaprdusTTS configuration dialog implementation

#ifdef _WIN32

#include "config_dialog.hpp"
#include "dictionary_dialog.hpp"
#include "dark_mode.hpp"
#include "resource.h"
#include <sapi.h>
#include <sphelper.h>
#include <cmath>
#include <cstdio>

// Link with SAPI
#pragma comment(lib, "sapi.lib")

namespace laprdus {

// Voice IDs mapped to combo box indices
static const char* VOICE_IDS[] = {
    "josip",
    "vlado",
    "detence",
    "baba",
    "djedo"
};
static const int NUM_VOICES = sizeof(VOICE_IDS) / sizeof(VOICE_IDS[0]);

// Slider ranges
static const int SPEED_MIN = 50;   // 0.5x
static const int SPEED_MAX = 200;  // 2.0x
static const int SPEED_DEFAULT = 100;

static const int PITCH_MIN = 50;   // 0.5x
static const int PITCH_MAX = 200;  // 2.0x
static const int PITCH_DEFAULT = 100;

static const int VOLUME_MIN = 0;
static const int VOLUME_MAX = 100;
static const int VOLUME_DEFAULT = 100;

static const int PAUSE_MIN = 0;
static const int PAUSE_MAX = 2000;
static const int PAUSE_DEFAULT = 100;

// =============================================================================
// ConfigDialog Implementation
// =============================================================================

ConfigDialog::ConfigDialog() {
}

ConfigDialog::~ConfigDialog() {
}

INT_PTR ConfigDialog::Show(HINSTANCE hInstance, HWND hParent) {
    m_hInstance = hInstance;

    // Load settings
    m_config.load_settings();
    m_settings = m_config.settings();

    // Check dark mode
    m_darkMode = IsSystemDarkMode();

    // Create dialog
    INT_PTR result = DialogBoxParamW(
        hInstance,
        MAKEINTRESOURCEW(IDD_CONFIG_DIALOG),
        hParent,
        DialogProc,
        reinterpret_cast<LPARAM>(this)
    );

    return result;
}

INT_PTR CALLBACK ConfigDialog::DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    ConfigDialog* pThis = nullptr;

    if (msg == WM_INITDIALOG) {
        pThis = reinterpret_cast<ConfigDialog*>(lParam);
        SetWindowLongPtrW(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hDlg = hDlg;
        return pThis->OnInitDialog(hDlg);
    }

    pThis = reinterpret_cast<ConfigDialog*>(GetWindowLongPtrW(hDlg, GWLP_USERDATA));
    if (!pThis) {
        return FALSE;
    }

    switch (msg) {
    case WM_COMMAND:
        return pThis->OnCommand(hDlg, wParam, lParam);

    case WM_HSCROLL:
        return pThis->OnHScroll(hDlg, wParam, lParam);

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
        return pThis->OnCtlColor(hDlg, msg, (HDC)wParam, (HWND)lParam);

    case WM_CTLCOLORDLG:
        return pThis->OnCtlColor(hDlg, msg, (HDC)wParam, hDlg);

    case WM_SETTINGCHANGE:
        return pThis->OnSettingChange(hDlg, lParam);

    case WM_DESTROY:
        pThis->OnDestroy();
        return TRUE;

    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        return TRUE;
    }

    return FALSE;
}

INT_PTR ConfigDialog::OnInitDialog(HWND hDlg) {
    // Set dialog title
    SetWindowTextW(hDlg, LoadLocalizedString(IDS_APP_TITLE).c_str());

    // Set group box texts
    SetControlText(hDlg, IDC_GROUP_VOICE, IDS_GROUP_VOICE);
    SetControlText(hDlg, IDC_GROUP_PAUSES, IDS_GROUP_PAUSES);
    SetControlText(hDlg, IDC_GROUP_OPTIONS, IDS_GROUP_OPTIONS);
    SetControlText(hDlg, IDC_GROUP_DICTIONARIES, IDS_GROUP_DICTIONARIES);

    // Set label texts
    SetControlText(hDlg, IDC_VOICE_LABEL, IDS_VOICE_LABEL);
    SetControlText(hDlg, IDC_SPEED_LABEL, IDS_SPEED_LABEL);
    SetControlText(hDlg, IDC_FORCE_SPEED, IDS_FORCE_SPEED);
    SetControlText(hDlg, IDC_PITCH_LABEL, IDS_PITCH_LABEL);
    SetControlText(hDlg, IDC_FORCE_PITCH, IDS_FORCE_PITCH);
    SetControlText(hDlg, IDC_VOLUME_LABEL, IDS_VOLUME_LABEL);
    SetControlText(hDlg, IDC_FORCE_VOLUME, IDS_FORCE_VOLUME);
    SetControlText(hDlg, IDC_COMMA_LABEL, IDS_COMMA_LABEL);
    SetControlText(hDlg, IDC_SENTENCE_LABEL, IDS_SENTENCE_LABEL);
    SetControlText(hDlg, IDC_NEWLINE_LABEL, IDS_NEWLINE_LABEL);

    // Set checkbox texts
    SetControlText(hDlg, IDC_INFLECTION_CHECK, IDS_INFLECTION);
    SetControlText(hDlg, IDC_EMOJI_CHECK, IDS_EMOJI);
    SetControlText(hDlg, IDC_DIGITS_CHECK, IDS_DIGITS);

    // Set user dictionary controls
    SetControlText(hDlg, IDC_USER_DICT_CHECK, IDS_USER_DICT_CHECK);
    SetControlText(hDlg, IDC_DICT_BUTTON, IDS_DICT_BUTTON);

    // Set button texts
    SetControlText(hDlg, IDC_TEST_BUTTON, IDS_TEST);
    SetControlText(hDlg, IDOK, IDS_OK);
    SetControlText(hDlg, IDCANCEL, IDS_CANCEL);
    SetControlText(hDlg, IDC_APPLY_BUTTON, IDS_APPLY);

    // Initialize voice combo box
    InitializeVoiceCombo(hDlg);

    // Initialize sliders
    InitializeSliders(hDlg);

    // Load current settings into controls
    LoadSettingsToControls(hDlg);

    // Apply dark mode if enabled
    ApplyDarkMode(hDlg);

    // Center dialog on parent or screen
    HWND hParent = GetParent(hDlg);
    if (!hParent) hParent = GetDesktopWindow();

    RECT rcDlg, rcParent;
    GetWindowRect(hDlg, &rcDlg);
    GetWindowRect(hParent, &rcParent);

    int x = rcParent.left + (rcParent.right - rcParent.left - (rcDlg.right - rcDlg.left)) / 2;
    int y = rcParent.top + (rcParent.bottom - rcParent.top - (rcDlg.bottom - rcDlg.top)) / 2;

    SetWindowPos(hDlg, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    return TRUE;
}

void ConfigDialog::InitializeVoiceCombo(HWND hDlg) {
    HWND hCombo = GetDlgItem(hDlg, IDC_VOICE_COMBO);
    if (!hCombo) return;

    // Add voice names
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)LoadLocalizedString(IDS_VOICE_JOSIP).c_str());
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)LoadLocalizedString(IDS_VOICE_VLADO).c_str());
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)LoadLocalizedString(IDS_VOICE_DETENCE).c_str());
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)LoadLocalizedString(IDS_VOICE_BABA).c_str());
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)LoadLocalizedString(IDS_VOICE_DJEDO).c_str());
}

void ConfigDialog::InitializeSliders(HWND hDlg) {
    // Speed slider
    HWND hSpeed = GetDlgItem(hDlg, IDC_SPEED_SLIDER);
    if (hSpeed) {
        SendMessageW(hSpeed, TBM_SETRANGE, TRUE, MAKELPARAM(SPEED_MIN, SPEED_MAX));
        SendMessageW(hSpeed, TBM_SETTICFREQ, 25, 0);
    }

    // Pitch slider
    HWND hPitch = GetDlgItem(hDlg, IDC_PITCH_SLIDER);
    if (hPitch) {
        SendMessageW(hPitch, TBM_SETRANGE, TRUE, MAKELPARAM(PITCH_MIN, PITCH_MAX));
        SendMessageW(hPitch, TBM_SETTICFREQ, 25, 0);
    }

    // Volume slider
    HWND hVolume = GetDlgItem(hDlg, IDC_VOLUME_SLIDER);
    if (hVolume) {
        SendMessageW(hVolume, TBM_SETRANGE, TRUE, MAKELPARAM(VOLUME_MIN, VOLUME_MAX));
        SendMessageW(hVolume, TBM_SETTICFREQ, 10, 0);
    }

    // Comma pause slider
    HWND hComma = GetDlgItem(hDlg, IDC_COMMA_SLIDER);
    if (hComma) {
        SendMessageW(hComma, TBM_SETRANGE, TRUE, MAKELPARAM(PAUSE_MIN, PAUSE_MAX));
        SendMessageW(hComma, TBM_SETTICFREQ, 200, 0);
    }

    // Sentence pause slider
    HWND hSentence = GetDlgItem(hDlg, IDC_SENTENCE_SLIDER);
    if (hSentence) {
        SendMessageW(hSentence, TBM_SETRANGE, TRUE, MAKELPARAM(PAUSE_MIN, PAUSE_MAX));
        SendMessageW(hSentence, TBM_SETTICFREQ, 200, 0);
    }

    // Newline pause slider
    HWND hNewline = GetDlgItem(hDlg, IDC_NEWLINE_SLIDER);
    if (hNewline) {
        SendMessageW(hNewline, TBM_SETRANGE, TRUE, MAKELPARAM(PAUSE_MIN, PAUSE_MAX));
        SendMessageW(hNewline, TBM_SETTICFREQ, 200, 0);
    }
}

void ConfigDialog::LoadSettingsToControls(HWND hDlg) {
    // Voice combo box
    int voiceIndex = 0;  // Default to josip
    for (int i = 0; i < NUM_VOICES; i++) {
        if (m_settings.default_voice == VOICE_IDS[i]) {
            voiceIndex = i;
            break;
        }
    }
    SendDlgItemMessageW(hDlg, IDC_VOICE_COMBO, CB_SETCURSEL, voiceIndex, 0);

    // Speed slider (0.5-2.0 mapped to 50-200)
    int speedValue = static_cast<int>(m_settings.speed * 100.0f);
    SendDlgItemMessageW(hDlg, IDC_SPEED_SLIDER, TBM_SETPOS, TRUE, speedValue);
    UpdateSliderValue(hDlg, IDC_SPEED_SLIDER, IDC_SPEED_VALUE, true);

    // Pitch slider
    int pitchValue = static_cast<int>(m_settings.user_pitch * 100.0f);
    SendDlgItemMessageW(hDlg, IDC_PITCH_SLIDER, TBM_SETPOS, TRUE, pitchValue);
    UpdateSliderValue(hDlg, IDC_PITCH_SLIDER, IDC_PITCH_VALUE, true);

    // Volume slider
    int volumeValue = static_cast<int>(m_settings.volume * 100.0f);
    SendDlgItemMessageW(hDlg, IDC_VOLUME_SLIDER, TBM_SETPOS, TRUE, volumeValue);
    UpdateSliderValue(hDlg, IDC_VOLUME_SLIDER, IDC_VOLUME_VALUE);

    // Force checkboxes
    CheckDlgButton(hDlg, IDC_FORCE_SPEED, m_settings.force_speed ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_FORCE_PITCH, m_settings.force_pitch ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_FORCE_VOLUME, m_settings.force_volume ? BST_CHECKED : BST_UNCHECKED);

    // Pause sliders
    SendDlgItemMessageW(hDlg, IDC_COMMA_SLIDER, TBM_SETPOS, TRUE, m_settings.comma_pause_ms);
    UpdateSliderValue(hDlg, IDC_COMMA_SLIDER, IDC_COMMA_VALUE, false, true);

    SendDlgItemMessageW(hDlg, IDC_SENTENCE_SLIDER, TBM_SETPOS, TRUE, m_settings.sentence_pause_ms);
    UpdateSliderValue(hDlg, IDC_SENTENCE_SLIDER, IDC_SENTENCE_VALUE, false, true);

    SendDlgItemMessageW(hDlg, IDC_NEWLINE_SLIDER, TBM_SETPOS, TRUE, m_settings.newline_pause_ms);
    UpdateSliderValue(hDlg, IDC_NEWLINE_SLIDER, IDC_NEWLINE_VALUE, false, true);

    // Option checkboxes
    CheckDlgButton(hDlg, IDC_INFLECTION_CHECK, m_settings.inflection_enabled ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_EMOJI_CHECK, m_settings.emoji_enabled ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_DIGITS_CHECK, m_settings.number_mode == NumberMode::DigitByDigit ? BST_CHECKED : BST_UNCHECKED);

    // User dictionary checkbox
    CheckDlgButton(hDlg, IDC_USER_DICT_CHECK, m_settings.user_dictionaries_enabled ? BST_CHECKED : BST_UNCHECKED);
}

void ConfigDialog::UpdateSliderValue(HWND hDlg, int sliderId, int valueId, bool isSpeed, bool isMs) {
    int pos = static_cast<int>(SendDlgItemMessageW(hDlg, sliderId, TBM_GETPOS, 0, 0));
    wchar_t text[32];

    if (isSpeed) {
        // Format as multiplier (e.g., "1.0x")
        float value = pos / 100.0f;
        swprintf_s(text, L"%.1fx", value);
    } else if (isMs) {
        // Format as milliseconds
        swprintf_s(text, L"%d ms", pos);
    } else {
        // Format as percentage
        swprintf_s(text, L"%d%%", pos);
    }

    SetDlgItemTextW(hDlg, valueId, text);
}

void ConfigDialog::CollectSettingsFromControls(HWND hDlg) {
    // Voice
    int voiceIndex = static_cast<int>(SendDlgItemMessageW(hDlg, IDC_VOICE_COMBO, CB_GETCURSEL, 0, 0));
    if (voiceIndex >= 0 && voiceIndex < NUM_VOICES) {
        m_settings.default_voice = VOICE_IDS[voiceIndex];
    }

    // Speed (50-200 mapped to 0.5-2.0)
    int speedValue = static_cast<int>(SendDlgItemMessageW(hDlg, IDC_SPEED_SLIDER, TBM_GETPOS, 0, 0));
    m_settings.speed = speedValue / 100.0f;

    // Pitch
    int pitchValue = static_cast<int>(SendDlgItemMessageW(hDlg, IDC_PITCH_SLIDER, TBM_GETPOS, 0, 0));
    m_settings.user_pitch = pitchValue / 100.0f;

    // Volume
    int volumeValue = static_cast<int>(SendDlgItemMessageW(hDlg, IDC_VOLUME_SLIDER, TBM_GETPOS, 0, 0));
    m_settings.volume = volumeValue / 100.0f;

    // Force checkboxes
    m_settings.force_speed = (IsDlgButtonChecked(hDlg, IDC_FORCE_SPEED) == BST_CHECKED);
    m_settings.force_pitch = (IsDlgButtonChecked(hDlg, IDC_FORCE_PITCH) == BST_CHECKED);
    m_settings.force_volume = (IsDlgButtonChecked(hDlg, IDC_FORCE_VOLUME) == BST_CHECKED);

    // Pause settings
    m_settings.comma_pause_ms = static_cast<uint32_t>(SendDlgItemMessageW(hDlg, IDC_COMMA_SLIDER, TBM_GETPOS, 0, 0));
    m_settings.sentence_pause_ms = static_cast<uint32_t>(SendDlgItemMessageW(hDlg, IDC_SENTENCE_SLIDER, TBM_GETPOS, 0, 0));
    m_settings.newline_pause_ms = static_cast<uint32_t>(SendDlgItemMessageW(hDlg, IDC_NEWLINE_SLIDER, TBM_GETPOS, 0, 0));

    // Option checkboxes
    m_settings.inflection_enabled = (IsDlgButtonChecked(hDlg, IDC_INFLECTION_CHECK) == BST_CHECKED);
    m_settings.emoji_enabled = (IsDlgButtonChecked(hDlg, IDC_EMOJI_CHECK) == BST_CHECKED);
    m_settings.number_mode = (IsDlgButtonChecked(hDlg, IDC_DIGITS_CHECK) == BST_CHECKED)
        ? NumberMode::DigitByDigit
        : NumberMode::WholeNumbers;

    // User dictionary checkbox
    m_settings.user_dictionaries_enabled = (IsDlgButtonChecked(hDlg, IDC_USER_DICT_CHECK) == BST_CHECKED);
}

INT_PTR ConfigDialog::OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;

    switch (LOWORD(wParam)) {
    case IDOK:
        OnOK(hDlg);
        return TRUE;

    case IDCANCEL:
        OnCancel(hDlg);
        return TRUE;

    case IDC_APPLY_BUTTON:
        OnApply(hDlg);
        return TRUE;

    case IDC_TEST_BUTTON:
        OnTest(hDlg);
        return TRUE;

    case IDC_DICT_BUTTON:
        OnDictionaries(hDlg);
        return TRUE;

    // Mark settings as changed for checkboxes and combo
    case IDC_FORCE_SPEED:
    case IDC_FORCE_PITCH:
    case IDC_FORCE_VOLUME:
    case IDC_INFLECTION_CHECK:
    case IDC_EMOJI_CHECK:
    case IDC_DIGITS_CHECK:
    case IDC_USER_DICT_CHECK:
        if (HIWORD(wParam) == BN_CLICKED) {
            m_settingsChanged = true;
        }
        return TRUE;

    case IDC_VOICE_COMBO:
        if (HIWORD(wParam) == CBN_SELCHANGE) {
            m_settingsChanged = true;
        }
        return TRUE;
    }

    return FALSE;
}

INT_PTR ConfigDialog::OnHScroll(HWND hDlg, WPARAM wParam, LPARAM lParam) {
    (void)wParam;

    HWND hSlider = (HWND)lParam;
    int sliderId = GetDlgCtrlID(hSlider);

    switch (sliderId) {
    case IDC_SPEED_SLIDER:
        UpdateSliderValue(hDlg, IDC_SPEED_SLIDER, IDC_SPEED_VALUE, true);
        m_settingsChanged = true;
        break;

    case IDC_PITCH_SLIDER:
        UpdateSliderValue(hDlg, IDC_PITCH_SLIDER, IDC_PITCH_VALUE, true);
        m_settingsChanged = true;
        break;

    case IDC_VOLUME_SLIDER:
        UpdateSliderValue(hDlg, IDC_VOLUME_SLIDER, IDC_VOLUME_VALUE);
        m_settingsChanged = true;
        break;

    case IDC_COMMA_SLIDER:
        UpdateSliderValue(hDlg, IDC_COMMA_SLIDER, IDC_COMMA_VALUE, false, true);
        m_settingsChanged = true;
        break;

    case IDC_SENTENCE_SLIDER:
        UpdateSliderValue(hDlg, IDC_SENTENCE_SLIDER, IDC_SENTENCE_VALUE, false, true);
        m_settingsChanged = true;
        break;

    case IDC_NEWLINE_SLIDER:
        UpdateSliderValue(hDlg, IDC_NEWLINE_SLIDER, IDC_NEWLINE_VALUE, false, true);
        m_settingsChanged = true;
        break;
    }

    return TRUE;
}

INT_PTR ConfigDialog::OnCtlColor(HWND hDlg, UINT msg, HDC hdc, HWND hCtrl) {
    (void)hDlg;
    (void)hCtrl;

    if (!m_darkMode) {
        return FALSE;
    }

    switch (msg) {
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        HBRUSH hBrush = OnCtlColorStatic(hdc, true);
        if (hBrush) {
            return (INT_PTR)hBrush;
        }
        break;
    }
    case WM_CTLCOLORDLG: {
        HBRUSH hBrush = OnCtlColorDlg(true);
        if (hBrush) {
            return (INT_PTR)hBrush;
        }
        break;
    }
    }

    return FALSE;
}

INT_PTR ConfigDialog::OnSettingChange(HWND hDlg, LPARAM lParam) {
    // Check if immersive color set changed (dark/light mode toggle)
    if (lParam) {
        const wchar_t* setting = reinterpret_cast<const wchar_t*>(lParam);
        if (wcscmp(setting, L"ImmersiveColorSet") == 0) {
            RefreshDarkMode(hDlg);
            return TRUE;
        }
    }
    return FALSE;
}

void ConfigDialog::OnDestroy() {
    // Nothing to cleanup
}

void ConfigDialog::OnOK(HWND hDlg) {
    CollectSettingsFromControls(hDlg);
    m_config.set_settings(m_settings);
    m_config.save_settings();
    EndDialog(hDlg, IDOK);
}

void ConfigDialog::OnCancel(HWND hDlg) {
    EndDialog(hDlg, IDCANCEL);
}

void ConfigDialog::OnApply(HWND hDlg) {
    CollectSettingsFromControls(hDlg);
    m_config.set_settings(m_settings);
    m_config.save_settings();
    m_settingsChanged = false;
}

void ConfigDialog::OnTest(HWND hDlg) {
    // Apply current settings before testing
    CollectSettingsFromControls(hDlg);

    // Get the test text
    std::wstring testText = LoadLocalizedString(IDS_TEST_TEXT);

    // Use SAPI to speak the test text
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    ISpVoice* pVoice = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_ALL,
        IID_ISpVoice, (void**)&pVoice);

    if (SUCCEEDED(hr) && pVoice) {
        // Find and select the appropriate Laprdus voice
        ISpObjectTokenCategory* pCategory = nullptr;
        hr = SpGetCategoryFromId(SPCAT_VOICES, &pCategory);

        if (SUCCEEDED(hr) && pCategory) {
            IEnumSpObjectTokens* pEnum = nullptr;
            hr = pCategory->EnumTokens(nullptr, nullptr, &pEnum);

            if (SUCCEEDED(hr) && pEnum) {
                ISpObjectToken* pToken = nullptr;
                while (pEnum->Next(1, &pToken, nullptr) == S_OK) {
                    LPWSTR pId = nullptr;
                    if (SUCCEEDED(pToken->GetId(&pId)) && pId) {
                        // Look for Laprdus voice token
                        if (wcsstr(pId, L"Laprdus") != nullptr) {
                            // Check if it matches the selected voice
                            LPWSTR pVoiceId = nullptr;
                            if (SUCCEEDED(pToken->GetStringValue(L"VoiceId", &pVoiceId)) && pVoiceId) {
                                std::string voiceIdStr;
                                int size = WideCharToMultiByte(CP_UTF8, 0, pVoiceId, -1, nullptr, 0, nullptr, nullptr);
                                if (size > 0) {
                                    voiceIdStr.resize(size - 1);
                                    WideCharToMultiByte(CP_UTF8, 0, pVoiceId, -1, voiceIdStr.data(), size, nullptr, nullptr);
                                }
                                CoTaskMemFree(pVoiceId);

                                if (voiceIdStr == m_settings.default_voice || m_settings.default_voice.empty()) {
                                    pVoice->SetVoice(pToken);
                                    CoTaskMemFree(pId);
                                    pToken->Release();
                                    break;
                                }
                            }
                        }
                        CoTaskMemFree(pId);
                    }
                    pToken->Release();
                }
                pEnum->Release();
            }
            pCategory->Release();
        }

        // Speak the test text
        pVoice->Speak(testText.c_str(), SPF_ASYNC, nullptr);

        // Wait for speech to complete
        pVoice->WaitUntilDone(INFINITE);

        pVoice->Release();
    }

    CoUninitialize();
}

void ConfigDialog::OnDictionaries(HWND hDlg) {
    DictionaryDialog dictDlg;
    dictDlg.Show(m_hInstance, hDlg);
}

void ConfigDialog::ApplyDarkMode(HWND hDlg) {
    if (!m_darkMode) {
        return;
    }

    // Apply dark title bar
    ApplyDarkTitleBar(hDlg, true);

    // Apply dark mode to all controls
    ApplyDarkModeToAllControls(hDlg, true);

    // Force redraw
    InvalidateRect(hDlg, nullptr, TRUE);
}

void ConfigDialog::RefreshDarkMode(HWND hDlg) {
    bool wasDark = m_darkMode;
    m_darkMode = IsSystemDarkMode();

    if (wasDark != m_darkMode) {
        ApplyDarkTitleBar(hDlg, m_darkMode);
        ApplyDarkModeToAllControls(hDlg, m_darkMode);
        InvalidateRect(hDlg, nullptr, TRUE);
    }
}

std::wstring ConfigDialog::LoadLocalizedString(UINT id) {
    wchar_t buffer[512];
    int len = ::LoadStringW(m_hInstance, id, buffer, 512);
    if (len > 0) {
        return std::wstring(buffer, len);
    }
    return L"";
}

void ConfigDialog::SetControlText(HWND hDlg, int ctrlId, UINT stringId) {
    std::wstring text = LoadLocalizedString(stringId);
    if (!text.empty()) {
        SetDlgItemTextW(hDlg, ctrlId, text.c_str());
    }
}

// =============================================================================
// Entry Point
// =============================================================================

int RunConfigDialog(HINSTANCE hInstance) {
    // Initialize common controls
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    // Initialize dark mode
    InitializeDarkMode();

    // Show dialog
    ConfigDialog dialog;
    INT_PTR result = dialog.Show(hInstance, nullptr);

    // Cleanup
    CleanupDarkMode();

    return (result == IDOK) ? 0 : 1;
}

} // namespace laprdus

#endif // _WIN32
