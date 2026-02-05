// -*- coding: utf-8 -*-
// dictionary_dialog.cpp - Dictionary management dialogs implementation

#ifdef _WIN32

#include "dictionary_dialog.hpp"
#include "dark_mode.hpp"
#include "resource.h"
#include "core/user_config.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace laprdus {

// =============================================================================
// JSON Helper Functions
// =============================================================================

namespace {

// Convert UTF-8 string to wide string
std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (size <= 0) return L"";
    std::wstring result(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, result.data(), size);
    return result;
}

// Convert wide string to UTF-8
std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return "";
    std::string result(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, result.data(), size, nullptr, nullptr);
    return result;
}

// Escape a string for JSON output
std::string EscapeJsonString(const std::string& str) {
    std::string result;
    result.reserve(str.length() + 10);
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

// Extract string value from JSON
std::string ExtractStringValue(const std::string& json, size_t start, size_t end) {
    std::string result;
    result.reserve(end - start);
    for (size_t i = start; i < end; i++) {
        if (json[i] == '\\' && i + 1 < end) {
            char next = json[i + 1];
            switch (next) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                default: result += next; break;
            }
            i++;
        } else {
            result += json[i];
        }
    }
    return result;
}

// Find the value for a key in JSON
std::string FindJsonStringValue(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";

    pos = json.find(':', pos + search.length());
    if (pos == std::string::npos) return "";

    pos = json.find_first_not_of(" \t\n\r", pos + 1);
    if (pos == std::string::npos || json[pos] != '"') return "";

    size_t start = pos + 1;
    size_t end = start;
    while (end < json.length()) {
        if (json[end] == '\\' && end + 1 < json.length()) {
            end += 2;
        } else if (json[end] == '"') {
            break;
        } else {
            end++;
        }
    }
    if (end >= json.length()) return "";

    return ExtractStringValue(json, start, end);
}

// Find boolean value in JSON
bool FindJsonBoolValue(const std::string& json, const std::string& key, bool defaultValue) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return defaultValue;

    pos = json.find(':', pos + search.length());
    if (pos == std::string::npos) return defaultValue;

    pos = json.find_first_not_of(" \t\n\r", pos + 1);
    if (pos == std::string::npos) return defaultValue;

    if (json.compare(pos, 4, "true") == 0) return true;
    if (json.compare(pos, 5, "false") == 0) return false;

    return defaultValue;
}

// Parse dictionary JSON file
std::vector<UserDictionaryEntry> ParseDictionaryJson(const std::string& json) {
    std::vector<UserDictionaryEntry> entries;

    // Find entries array
    size_t entriesPos = json.find("\"entries\"");
    if (entriesPos == std::string::npos) return entries;

    size_t arrayStart = json.find('[', entriesPos);
    if (arrayStart == std::string::npos) return entries;

    // Find each entry object
    size_t pos = arrayStart + 1;
    while (pos < json.length()) {
        // Find start of object
        size_t objStart = json.find('{', pos);
        if (objStart == std::string::npos) break;

        // Find end of object (handle nested braces)
        size_t objEnd = objStart + 1;
        int braceCount = 1;
        while (objEnd < json.length() && braceCount > 0) {
            if (json[objEnd] == '{') braceCount++;
            else if (json[objEnd] == '}') braceCount--;
            objEnd++;
        }

        if (braceCount != 0) break;

        // Extract this entry's JSON
        std::string entryJson = json.substr(objStart, objEnd - objStart);

        // Parse entry
        UserDictionaryEntry entry;
        entry.grapheme = Utf8ToWide(FindJsonStringValue(entryJson, "grapheme"));
        entry.phoneme = Utf8ToWide(FindJsonStringValue(entryJson, "phoneme"));
        entry.comment = Utf8ToWide(FindJsonStringValue(entryJson, "comment"));
        entry.caseSensitive = FindJsonBoolValue(entryJson, "caseSensitive", false);
        entry.wholeWord = FindJsonBoolValue(entryJson, "wholeWord", true);

        if (!entry.grapheme.empty()) {
            entries.push_back(entry);
        }

        pos = objEnd;
    }

    return entries;
}

// Generate dictionary JSON from entries
std::string GenerateDictionaryJson(const std::vector<UserDictionaryEntry>& entries) {
    std::ostringstream json;
    json << "{\n";
    json << "    \"version\": \"1.0\",\n";
    json << "    \"entries\": [\n";

    for (size_t i = 0; i < entries.size(); i++) {
        const auto& entry = entries[i];
        json << "        {\n";
        json << "            \"grapheme\": \"" << EscapeJsonString(WideToUtf8(entry.grapheme)) << "\",\n";
        json << "            \"phoneme\": \"" << EscapeJsonString(WideToUtf8(entry.phoneme)) << "\",\n";
        json << "            \"caseSensitive\": " << (entry.caseSensitive ? "true" : "false") << ",\n";
        json << "            \"wholeWord\": " << (entry.wholeWord ? "true" : "false");
        if (!entry.comment.empty()) {
            json << ",\n            \"comment\": \"" << EscapeJsonString(WideToUtf8(entry.comment)) << "\"";
        }
        json << "\n        }";
        if (i < entries.size() - 1) {
            json << ",";
        }
        json << "\n";
    }

    json << "    ]\n";
    json << "}\n";
    return json.str();
}

} // anonymous namespace

// =============================================================================
// DictionaryDialog Implementation
// =============================================================================

DictionaryDialog::DictionaryDialog() {
}

DictionaryDialog::~DictionaryDialog() {
}

INT_PTR DictionaryDialog::Show(HINSTANCE hInstance, HWND hParent) {
    m_hInstance = hInstance;
    m_darkMode = IsSystemDarkMode();

    // Load current dictionary
    LoadDictionary(m_currentType);

    INT_PTR result = DialogBoxParamW(
        hInstance,
        MAKEINTRESOURCEW(IDD_DICTIONARY_DIALOG),
        hParent,
        DialogProc,
        reinterpret_cast<LPARAM>(this)
    );

    return result;
}

INT_PTR CALLBACK DictionaryDialog::DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    DictionaryDialog* pThis = nullptr;

    if (msg == WM_INITDIALOG) {
        pThis = reinterpret_cast<DictionaryDialog*>(lParam);
        SetWindowLongPtrW(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hDlg = hDlg;
        return pThis->OnInitDialog(hDlg);
    }

    pThis = reinterpret_cast<DictionaryDialog*>(GetWindowLongPtrW(hDlg, GWLP_USERDATA));
    if (!pThis) {
        return FALSE;
    }

    switch (msg) {
    case WM_COMMAND:
        return pThis->OnCommand(hDlg, wParam, lParam);

    case WM_NOTIFY:
        return pThis->OnNotify(hDlg, reinterpret_cast<LPNMHDR>(lParam));

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
        return pThis->OnCtlColor(hDlg, msg, (HDC)wParam, (HWND)lParam);

    case WM_CTLCOLORDLG:
        return pThis->OnCtlColor(hDlg, msg, (HDC)wParam, hDlg);

    case WM_CLOSE:
        if (pThis->m_modified) {
            pThis->SaveDictionary();
        }
        EndDialog(hDlg, IDOK);
        return TRUE;
    }

    return FALSE;
}

INT_PTR DictionaryDialog::OnInitDialog(HWND hDlg) {
    // Set dialog title
    SetWindowTextW(hDlg, LoadLocalizedString(IDS_DICT_DIALOG_TITLE).c_str());

    // Initialize controls
    InitializeControls(hDlg);
    InitializeDictionaryTypeCombo(hDlg);
    InitializeListView(hDlg);

    // Populate with current dictionary entries
    PopulateListView(hDlg);
    UpdateButtonStates(hDlg);

    // Apply dark mode if enabled
    ApplyDarkMode(hDlg);

    // Center dialog on parent
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

void DictionaryDialog::InitializeControls(HWND hDlg) {
    SetControlText(hDlg, IDC_DICT_TYPE_LABEL, IDS_DICT_TYPE_LABEL);
    SetControlText(hDlg, IDC_DICT_ADD, IDS_DICT_ADD);
    SetControlText(hDlg, IDC_DICT_EDIT, IDS_DICT_EDIT);
    SetControlText(hDlg, IDC_DICT_DUPLICATE, IDS_DICT_DUPLICATE);
    SetControlText(hDlg, IDC_DICT_DELETE, IDS_DICT_DELETE);
    SetControlText(hDlg, IDC_DICT_CLOSE, IDS_DICT_CLOSE);
}

void DictionaryDialog::InitializeDictionaryTypeCombo(HWND hDlg) {
    HWND hCombo = GetDlgItem(hDlg, IDC_DICT_TYPE_COMBO);
    if (!hCombo) return;

    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)LoadLocalizedString(IDS_DICT_TYPE_MAIN).c_str());
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)LoadLocalizedString(IDS_DICT_TYPE_SPELLING).c_str());
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)LoadLocalizedString(IDS_DICT_TYPE_EMOJI).c_str());

    SendMessageW(hCombo, CB_SETCURSEL, static_cast<int>(m_currentType), 0);
}

void DictionaryDialog::InitializeListView(HWND hDlg) {
    HWND hListView = GetDlgItem(hDlg, IDC_DICT_LISTVIEW);
    if (!hListView) return;

    // Set extended styles
    ListView_SetExtendedListViewStyle(hListView,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

    // Add columns
    LVCOLUMNW col = {};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    // Column 0: Original
    col.pszText = const_cast<LPWSTR>(LoadLocalizedString(IDS_DICT_COL_ORIGINAL).c_str());
    col.cx = 120;
    col.iSubItem = 0;
    ListView_InsertColumn(hListView, 0, &col);

    // Column 1: Replacement
    col.pszText = const_cast<LPWSTR>(LoadLocalizedString(IDS_DICT_COL_REPLACEMENT).c_str());
    col.cx = 120;
    col.iSubItem = 1;
    ListView_InsertColumn(hListView, 1, &col);

    // Column 2: Case sensitive
    col.pszText = const_cast<LPWSTR>(LoadLocalizedString(IDS_DICT_COL_CASE).c_str());
    col.cx = 80;
    col.iSubItem = 2;
    ListView_InsertColumn(hListView, 2, &col);

    // Column 3: Whole word
    col.pszText = const_cast<LPWSTR>(LoadLocalizedString(IDS_DICT_COL_WHOLE_WORD).c_str());
    col.cx = 80;
    col.iSubItem = 3;
    ListView_InsertColumn(hListView, 3, &col);

    // Column 4: Comment
    col.pszText = const_cast<LPWSTR>(LoadLocalizedString(IDS_DICT_COL_COMMENT).c_str());
    col.cx = 150;
    col.iSubItem = 4;
    ListView_InsertColumn(hListView, 4, &col);
}

void DictionaryDialog::PopulateListView(HWND hDlg) {
    HWND hListView = GetDlgItem(hDlg, IDC_DICT_LISTVIEW);
    if (!hListView) return;

    // Clear existing items
    ListView_DeleteAllItems(hListView);

    // Get localized Yes/No strings
    std::wstring yesStr = LoadLocalizedString(IDS_YES);
    std::wstring noStr = LoadLocalizedString(IDS_NO);

    // Add entries
    LVITEMW item = {};
    item.mask = LVIF_TEXT;

    for (size_t i = 0; i < m_entries.size(); i++) {
        const auto& entry = m_entries[i];

        // Column 0: Original
        item.iItem = static_cast<int>(i);
        item.iSubItem = 0;
        item.pszText = const_cast<LPWSTR>(entry.grapheme.c_str());
        ListView_InsertItem(hListView, &item);

        // Column 1: Replacement
        ListView_SetItemText(hListView, static_cast<int>(i), 1,
            const_cast<LPWSTR>(entry.phoneme.c_str()));

        // Column 2: Case sensitive
        ListView_SetItemText(hListView, static_cast<int>(i), 2,
            const_cast<LPWSTR>(entry.caseSensitive ? yesStr.c_str() : noStr.c_str()));

        // Column 3: Whole word
        ListView_SetItemText(hListView, static_cast<int>(i), 3,
            const_cast<LPWSTR>(entry.wholeWord ? yesStr.c_str() : noStr.c_str()));

        // Column 4: Comment
        ListView_SetItemText(hListView, static_cast<int>(i), 4,
            const_cast<LPWSTR>(entry.comment.c_str()));
    }
}

void DictionaryDialog::UpdateButtonStates(HWND hDlg) {
    HWND hListView = GetDlgItem(hDlg, IDC_DICT_LISTVIEW);
    int selectedIndex = GetSelectedIndex(hListView);

    BOOL hasSelection = (selectedIndex >= 0);

    EnableWindow(GetDlgItem(hDlg, IDC_DICT_EDIT), hasSelection);
    EnableWindow(GetDlgItem(hDlg, IDC_DICT_DUPLICATE), hasSelection);
    EnableWindow(GetDlgItem(hDlg, IDC_DICT_DELETE), hasSelection);
}

bool DictionaryDialog::LoadDictionary(DictionaryType type) {
    m_currentType = type;
    m_entries.clear();

    std::wstring path = GetDictionaryPath(type);
    if (path.empty()) return false;

    // Check if file exists
    if (!std::filesystem::exists(path)) {
        return true; // Empty dictionary is valid
    }

    // Read file
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    // Parse JSON
    m_entries = ParseDictionaryJson(buffer.str());

    return true;
}

bool DictionaryDialog::SaveDictionary() {
    std::wstring path = GetDictionaryPath(m_currentType);
    if (path.empty()) return false;

    // Ensure directory exists
    UserConfig config;
    config.ensure_config_directory();

    // Generate JSON
    std::string json = GenerateDictionaryJson(m_entries);

    // Write file
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        ShowError(m_hDlg, IDS_ERROR_FILE_WRITE);
        return false;
    }

    file.write(json.c_str(), static_cast<std::streamsize>(json.size()));
    file.close();

    m_modified = false;
    return true;
}

void DictionaryDialog::OnDictionaryTypeChanged(HWND hDlg) {
    // Save current dictionary if modified
    if (m_modified) {
        SaveDictionary();
    }

    // Get new selection
    HWND hCombo = GetDlgItem(hDlg, IDC_DICT_TYPE_COMBO);
    int sel = static_cast<int>(SendMessageW(hCombo, CB_GETCURSEL, 0, 0));

    // Load new dictionary
    DictionaryType newType = static_cast<DictionaryType>(sel);
    LoadDictionary(newType);

    // Refresh list view
    PopulateListView(hDlg);
    UpdateButtonStates(hDlg);
}

INT_PTR DictionaryDialog::OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;

    switch (LOWORD(wParam)) {
    case IDC_DICT_TYPE_COMBO:
        if (HIWORD(wParam) == CBN_SELCHANGE) {
            OnDictionaryTypeChanged(hDlg);
        }
        return TRUE;

    case IDC_DICT_ADD:
        OnAddEntry(hDlg);
        return TRUE;

    case IDC_DICT_EDIT:
        OnEditEntry(hDlg);
        return TRUE;

    case IDC_DICT_DUPLICATE:
        OnDuplicateEntry(hDlg);
        return TRUE;

    case IDC_DICT_DELETE:
        OnDeleteEntry(hDlg);
        return TRUE;

    case IDC_DICT_CLOSE:
        if (m_modified) {
            SaveDictionary();
        }
        EndDialog(hDlg, IDOK);
        return TRUE;
    }

    return FALSE;
}

INT_PTR DictionaryDialog::OnNotify(HWND hDlg, LPNMHDR pnmh) {
    if (pnmh->idFrom == IDC_DICT_LISTVIEW) {
        switch (pnmh->code) {
        case LVN_ITEMCHANGED:
            UpdateButtonStates(hDlg);
            return TRUE;

        case NM_DBLCLK:
            OnEditEntry(hDlg);
            return TRUE;
        }
    }

    return FALSE;
}

INT_PTR DictionaryDialog::OnCtlColor(HWND hDlg, UINT msg, HDC hdc, HWND hCtrl) {
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

void DictionaryDialog::OnAddEntry(HWND hDlg) {
    HWND hListView = GetDlgItem(hDlg, IDC_DICT_LISTVIEW);
    int currentIndex = GetSelectedIndex(hListView);

    DictionaryEntryDialog entryDlg;
    if (entryDlg.Show(m_hInstance, hDlg, nullptr, false) == IDOK) {
        m_entries.push_back(entryDlg.GetEntry());
        m_modified = true;
        PopulateListView(hDlg);

        // Select the newly added item
        int newIndex = static_cast<int>(m_entries.size()) - 1;
        ListView_SetItemState(hListView, newIndex, LVIS_SELECTED | LVIS_FOCUSED,
            LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(hListView, newIndex, FALSE);
        SetFocus(hListView);
        UpdateButtonStates(hDlg);
    } else {
        // User cancelled - restore focus to previously selected item or listview
        if (currentIndex >= 0 && currentIndex < static_cast<int>(m_entries.size())) {
            ListView_SetItemState(hListView, currentIndex, LVIS_SELECTED | LVIS_FOCUSED,
                LVIS_SELECTED | LVIS_FOCUSED);
        }
        SetFocus(hListView);
    }
}

void DictionaryDialog::OnEditEntry(HWND hDlg) {
    HWND hListView = GetDlgItem(hDlg, IDC_DICT_LISTVIEW);
    int index = GetSelectedIndex(hListView);
    if (index < 0 || index >= static_cast<int>(m_entries.size())) return;

    DictionaryEntryDialog entryDlg;
    if (entryDlg.Show(m_hInstance, hDlg, &m_entries[index], true) == IDOK) {
        m_entries[index] = entryDlg.GetEntry();
        m_modified = true;
        PopulateListView(hDlg);
    }

    // Always re-select the edited item (whether OK or Cancel)
    ListView_SetItemState(hListView, index, LVIS_SELECTED | LVIS_FOCUSED,
        LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(hListView, index, FALSE);
    SetFocus(hListView);
}

void DictionaryDialog::OnDuplicateEntry(HWND hDlg) {
    HWND hListView = GetDlgItem(hDlg, IDC_DICT_LISTVIEW);
    int index = GetSelectedIndex(hListView);
    if (index < 0 || index >= static_cast<int>(m_entries.size())) return;

    // Create a copy
    UserDictionaryEntry copy = m_entries[index];
    copy.grapheme += L" (copy)";

    m_entries.push_back(copy);
    m_modified = true;
    PopulateListView(hDlg);

    // Select the new item
    int newIndex = static_cast<int>(m_entries.size()) - 1;
    ListView_SetItemState(hListView, newIndex, LVIS_SELECTED | LVIS_FOCUSED,
        LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(hListView, newIndex, FALSE);
    SetFocus(hListView);
    UpdateButtonStates(hDlg);
}

void DictionaryDialog::OnDeleteEntry(HWND hDlg) {
    HWND hListView = GetDlgItem(hDlg, IDC_DICT_LISTVIEW);
    int index = GetSelectedIndex(hListView);
    if (index < 0 || index >= static_cast<int>(m_entries.size())) return;

    // Delete the entry
    m_entries.erase(m_entries.begin() + index);
    m_modified = true;
    PopulateListView(hDlg);

    // Set focus to adjacent item (prefer next, then previous, then listview)
    int newIndex = -1;
    if (!m_entries.empty()) {
        if (index < static_cast<int>(m_entries.size())) {
            // There's an item at the same index (previously next item)
            newIndex = index;
        } else {
            // Select the last item (previous item)
            newIndex = static_cast<int>(m_entries.size()) - 1;
        }
    }

    if (newIndex >= 0) {
        ListView_SetItemState(hListView, newIndex, LVIS_SELECTED | LVIS_FOCUSED,
            LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(hListView, newIndex, FALSE);
    }
    SetFocus(hListView);
    UpdateButtonStates(hDlg);
}

std::wstring DictionaryDialog::GetDictionaryPath(DictionaryType type) {
    UserConfig config;
    switch (type) {
    case DictionaryType::Main:
        return Utf8ToWide(config.get_user_dictionary_path());
    case DictionaryType::Spelling:
        return Utf8ToWide(config.get_user_spelling_dictionary_path());
    case DictionaryType::Emoji:
        return Utf8ToWide(config.get_user_emoji_dictionary_path());
    }
    return L"";
}

int DictionaryDialog::GetSelectedIndex(HWND hListView) {
    return ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
}

void DictionaryDialog::ShowError(HWND hDlg, UINT stringId) {
    std::wstring msg = LoadLocalizedString(stringId);
    MessageBoxW(hDlg, msg.c_str(), L"Error", MB_OK | MB_ICONERROR);
}

void DictionaryDialog::ApplyDarkMode(HWND hDlg) {
    if (!m_darkMode) return;

    ApplyDarkTitleBar(hDlg, true);
    ApplyDarkModeToAllControls(hDlg, true);
    InvalidateRect(hDlg, nullptr, TRUE);
}

bool DictionaryDialog::IsDarkModeEnabled() {
    return IsSystemDarkMode();
}

std::wstring DictionaryDialog::LoadLocalizedString(UINT id) {
    wchar_t buffer[512];
    int len = ::LoadStringW(m_hInstance, id, buffer, 512);
    if (len > 0) {
        return std::wstring(buffer, len);
    }
    return L"";
}

void DictionaryDialog::SetControlText(HWND hDlg, int ctrlId, UINT stringId) {
    std::wstring text = LoadLocalizedString(stringId);
    if (!text.empty()) {
        SetDlgItemTextW(hDlg, ctrlId, text.c_str());
    }
}

// =============================================================================
// DictionaryEntryDialog Implementation
// =============================================================================

DictionaryEntryDialog::DictionaryEntryDialog() {
}

DictionaryEntryDialog::~DictionaryEntryDialog() {
}

INT_PTR DictionaryEntryDialog::Show(HINSTANCE hInstance, HWND hParent,
    const UserDictionaryEntry* entry, bool isEdit) {
    m_hInstance = hInstance;
    m_isEdit = isEdit;
    m_darkMode = IsSystemDarkMode();

    if (entry) {
        m_entry = *entry;
    } else {
        m_entry = UserDictionaryEntry();
    }

    INT_PTR result = DialogBoxParamW(
        hInstance,
        MAKEINTRESOURCEW(IDD_DICT_ENTRY_DIALOG),
        hParent,
        DialogProc,
        reinterpret_cast<LPARAM>(this)
    );

    return result;
}

INT_PTR CALLBACK DictionaryEntryDialog::DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    DictionaryEntryDialog* pThis = nullptr;

    if (msg == WM_INITDIALOG) {
        pThis = reinterpret_cast<DictionaryEntryDialog*>(lParam);
        SetWindowLongPtrW(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hDlg = hDlg;
        return pThis->OnInitDialog(hDlg);
    }

    pThis = reinterpret_cast<DictionaryEntryDialog*>(GetWindowLongPtrW(hDlg, GWLP_USERDATA));
    if (!pThis) {
        return FALSE;
    }

    switch (msg) {
    case WM_COMMAND:
        return pThis->OnCommand(hDlg, wParam, lParam);

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
        return pThis->OnCtlColor(hDlg, msg, (HDC)wParam, (HWND)lParam);

    case WM_CTLCOLORDLG:
        return pThis->OnCtlColor(hDlg, msg, (HDC)wParam, hDlg);

    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        return TRUE;
    }

    return FALSE;
}

INT_PTR DictionaryEntryDialog::OnInitDialog(HWND hDlg) {
    // Set dialog title
    SetWindowTextW(hDlg, LoadLocalizedString(
        m_isEdit ? IDS_ENTRY_DIALOG_TITLE_EDIT : IDS_ENTRY_DIALOG_TITLE_ADD).c_str());

    // Initialize control texts
    InitializeControls(hDlg);

    // Populate with existing entry data
    SetDlgItemTextW(hDlg, IDC_ENTRY_ORIGINAL_EDIT, m_entry.grapheme.c_str());
    SetDlgItemTextW(hDlg, IDC_ENTRY_REPLACEMENT_EDIT, m_entry.phoneme.c_str());
    SetDlgItemTextW(hDlg, IDC_ENTRY_COMMENT_EDIT, m_entry.comment.c_str());
    CheckDlgButton(hDlg, IDC_ENTRY_CASE_SENSITIVE, m_entry.caseSensitive ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_ENTRY_WHOLE_WORD, m_entry.wholeWord ? BST_CHECKED : BST_UNCHECKED);

    // Apply dark mode
    ApplyDarkMode(hDlg);

    // Center dialog on parent
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

void DictionaryEntryDialog::InitializeControls(HWND hDlg) {
    SetControlText(hDlg, IDC_ENTRY_ORIGINAL_LABEL, IDS_ENTRY_ORIGINAL_LABEL);
    SetControlText(hDlg, IDC_ENTRY_REPLACEMENT_LABEL, IDS_ENTRY_REPLACEMENT_LABEL);
    SetControlText(hDlg, IDC_ENTRY_COMMENT_LABEL, IDS_ENTRY_COMMENT_LABEL);
    SetControlText(hDlg, IDC_ENTRY_CASE_SENSITIVE, IDS_ENTRY_CASE_SENSITIVE);
    SetControlText(hDlg, IDC_ENTRY_WHOLE_WORD, IDS_ENTRY_WHOLE_WORD);
    SetControlText(hDlg, IDOK, IDS_OK);
    SetControlText(hDlg, IDCANCEL, IDS_CANCEL);
}

INT_PTR DictionaryEntryDialog::OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;

    switch (LOWORD(wParam)) {
    case IDOK:
        if (ValidateAndCollect(hDlg)) {
            EndDialog(hDlg, IDOK);
        }
        return TRUE;

    case IDCANCEL:
        EndDialog(hDlg, IDCANCEL);
        return TRUE;
    }

    return FALSE;
}

INT_PTR DictionaryEntryDialog::OnCtlColor(HWND hDlg, UINT msg, HDC hdc, HWND hCtrl) {
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

bool DictionaryEntryDialog::ValidateAndCollect(HWND hDlg) {
    // Get text lengths
    int originalLen = GetWindowTextLengthW(GetDlgItem(hDlg, IDC_ENTRY_ORIGINAL_EDIT));
    int replacementLen = GetWindowTextLengthW(GetDlgItem(hDlg, IDC_ENTRY_REPLACEMENT_EDIT));

    // Validate
    if (originalLen == 0 || replacementLen == 0) {
        std::wstring msg = LoadLocalizedString(IDS_ERROR_INVALID_ENTRY);
        MessageBoxW(hDlg, msg.c_str(), L"Error", MB_OK | MB_ICONWARNING);
        return false;
    }

    // Collect values
    wchar_t buffer[1024];

    GetDlgItemTextW(hDlg, IDC_ENTRY_ORIGINAL_EDIT, buffer, 1024);
    m_entry.grapheme = buffer;

    GetDlgItemTextW(hDlg, IDC_ENTRY_REPLACEMENT_EDIT, buffer, 1024);
    m_entry.phoneme = buffer;

    GetDlgItemTextW(hDlg, IDC_ENTRY_COMMENT_EDIT, buffer, 1024);
    m_entry.comment = buffer;

    m_entry.caseSensitive = (IsDlgButtonChecked(hDlg, IDC_ENTRY_CASE_SENSITIVE) == BST_CHECKED);
    m_entry.wholeWord = (IsDlgButtonChecked(hDlg, IDC_ENTRY_WHOLE_WORD) == BST_CHECKED);

    return true;
}

void DictionaryEntryDialog::ApplyDarkMode(HWND hDlg) {
    if (!m_darkMode) return;

    ApplyDarkTitleBar(hDlg, true);
    ApplyDarkModeToAllControls(hDlg, true);
    InvalidateRect(hDlg, nullptr, TRUE);
}

bool DictionaryEntryDialog::IsDarkModeEnabled() {
    return IsSystemDarkMode();
}

std::wstring DictionaryEntryDialog::LoadLocalizedString(UINT id) {
    wchar_t buffer[512];
    int len = ::LoadStringW(m_hInstance, id, buffer, 512);
    if (len > 0) {
        return std::wstring(buffer, len);
    }
    return L"";
}

void DictionaryEntryDialog::SetControlText(HWND hDlg, int ctrlId, UINT stringId) {
    std::wstring text = LoadLocalizedString(stringId);
    if (!text.empty()) {
        SetDlgItemTextW(hDlg, ctrlId, text.c_str());
    }
}

} // namespace laprdus

#endif // _WIN32
