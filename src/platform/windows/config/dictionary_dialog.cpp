// -*- coding: utf-8 -*-
// dictionary_dialog.cpp - Dictionary management dialogs implementation

#ifdef _WIN32

#include "dictionary_dialog.hpp"
#include "dark_mode.hpp"
#include "resource.h"
#include "core/user_config.hpp"
#include "formant/formant_frontend.hpp"
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

// A string of the resource table, or "" when it is missing
std::wstring LoadResourceString(HINSTANCE hInstance, UINT id) {
    wchar_t buffer[512];
    int len = ::LoadStringW(hInstance, id, buffer, 512);
    if (len > 0) {
        return std::wstring(buffer, len);
    }
    return L"";
}

// The first "%s" of a resource string replaced by text of any length
std::wstring FormatResourceString(const std::wstring& format, const std::wstring& argument) {
    size_t pos = format.find(L"%s");
    if (pos == std::wstring::npos) return format + argument;
    return format.substr(0, pos) + argument + format.substr(pos + 2);
}

// The dark-mode brush for a WM_CTLCOLOR* message, or FALSE to let the
// system paint (light mode, or a message without a brush of its own)
INT_PTR DarkCtlColor(bool darkMode, UINT msg, HDC hdc) {
    if (!darkMode) return FALSE;

    HBRUSH hBrush = nullptr;
    switch (msg) {
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
        hBrush = OnCtlColorStatic(hdc, true);
        break;
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:  // the list of a combo box
        hBrush = OnCtlColorEdit(hdc, true);
        break;
    case WM_CTLCOLORDLG:
        hBrush = OnCtlColorDlg(true);
        break;
    }
    return hBrush ? (INT_PTR)hBrush : FALSE;
}

// Center a dialog on its parent (or the desktop)
void CenterOnParent(HWND hDlg) {
    HWND hParent = GetParent(hDlg);
    if (!hParent) hParent = GetDesktopWindow();

    RECT rcDlg, rcParent;
    GetWindowRect(hDlg, &rcDlg);
    GetWindowRect(hParent, &rcParent);

    int x = rcParent.left + (rcParent.right - rcParent.left - (rcDlg.right - rcDlg.left)) / 2;
    int y = rcParent.top + (rcParent.bottom - rcParent.top - (rcDlg.bottom - rcDlg.top)) / 2;

    SetWindowPos(hDlg, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
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

// Find the key's position in a JSON object: a quoted name followed by a
// colon, so that a value equal to the name ("comment": "word") is skipped
size_t FindJsonKey(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    while (pos != std::string::npos) {
        size_t colon = json.find_first_not_of(" \t\n\r", pos + search.length());
        if (colon != std::string::npos && json[colon] == ':') return colon;
        pos = json.find(search, pos + search.length());
    }
    return std::string::npos;
}

// Find the value for a key in JSON
std::string FindJsonStringValue(const std::string& json, const std::string& key) {
    size_t pos = FindJsonKey(json, key);
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
    size_t pos = FindJsonKey(json, key);
    if (pos == std::string::npos) return defaultValue;

    pos = json.find_first_not_of(" \t\n\r", pos + 1);
    if (pos == std::string::npos) return defaultValue;

    if (json.compare(pos, 4, "true") == 0) return true;
    if (json.compare(pos, 5, "false") == 0) return false;

    return defaultValue;
}

// The JSON text of every object of the "entries" array
std::vector<std::string> SplitJsonEntries(const std::string& json) {
    std::vector<std::string> objects;

    // Find entries array
    size_t entriesPos = FindJsonKey(json, "entries");
    if (entriesPos == std::string::npos) return objects;

    size_t arrayStart = json.find('[', entriesPos);
    if (arrayStart == std::string::npos) return objects;

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

        objects.push_back(json.substr(objStart, objEnd - objStart));
        pos = objEnd;
    }

    return objects;
}

// Parse dictionary JSON file
std::vector<UserDictionaryEntry> ParseDictionaryJson(const std::string& json) {
    std::vector<UserDictionaryEntry> entries;

    for (const std::string& entryJson : SplitJsonEntries(json)) {
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
    }

    return entries;
}

// Parse the accent lexicon (accents.json): { "word": .. } or { "verb": .. },
// an optional "language" and "comment". Entries with neither key are dropped.
// A hand-written entry with both keys, which the engine rejects, is listed
// as a word and a verb, so that saving keeps both halves (as two entries the
// engine accepts).
std::vector<UserDictionaryEntry> ParseAccentJson(const std::string& json) {
    std::vector<UserDictionaryEntry> entries;

    for (const std::string& entryJson : SplitJsonEntries(json)) {
        UserDictionaryEntry entry;
        entry.language = Utf8ToWide(FindJsonStringValue(entryJson, "language"));
        entry.comment = Utf8ToWide(FindJsonStringValue(entryJson, "comment"));
        std::string word = FindJsonStringValue(entryJson, "word");
        if (!word.empty()) {
            entry.verb = false;
            entry.grapheme = Utf8ToWide(word);
            entries.push_back(entry);
        }
        std::string verb = FindJsonStringValue(entryJson, "verb");
        if (!verb.empty()) {
            entry.verb = true;
            entry.grapheme = Utf8ToWide(verb);
            entries.push_back(entry);
        }
    }

    return entries;
}

// One accent lexicon entry as a JSON object (also what the parser checks)
std::string AccentEntryJson(const UserDictionaryEntry& entry, const std::string& indent) {
    std::ostringstream json;
    json << indent << "{\n";
    json << indent << "    \"" << (entry.verb ? "verb" : "word") << "\": \""
         << EscapeJsonString(WideToUtf8(entry.grapheme)) << "\"";
    if (!entry.language.empty()) {
        json << ",\n" << indent << "    \"language\": \"" << EscapeJsonString(WideToUtf8(entry.language)) << "\"";
    }
    if (!entry.comment.empty()) {
        json << ",\n" << indent << "    \"comment\": \"" << EscapeJsonString(WideToUtf8(entry.comment)) << "\"";
    }
    json << "\n" << indent << "}";
    return json.str();
}

// Generate accent lexicon JSON from entries
std::string GenerateAccentJson(const std::vector<UserDictionaryEntry>& entries) {
    std::ostringstream json;
    json << "{\n";
    json << "    \"version\": \"1.0\",\n";
    json << "    \"entries\": [\n";

    for (size_t i = 0; i < entries.size(); i++) {
        json << AccentEntryJson(entries[i], "        ");
        if (i < entries.size() - 1) {
            json << ",";
        }
        json << "\n";
    }

    json << "    ]\n";
    json << "}\n";
    return json.str();
}

// Language codes of the accent entry dialog's language combo box, in its order
const char* const ACCENT_LANGUAGES[] = {"", "hr", "sr", "bs", "sl"};

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
    case WM_CTLCOLOREDIT:
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

    // Select the first item by default
    HWND hListView = GetDlgItem(hDlg, IDC_DICT_LISTVIEW);
    if (hListView && !m_entries.empty()) {
        ListView_SetItemState(hListView, 0, LVIS_SELECTED | LVIS_FOCUSED,
            LVIS_SELECTED | LVIS_FOCUSED);
    }

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
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)LoadLocalizedString(IDS_DICT_TYPE_ACCENTS).c_str());

    SendMessageW(hCombo, CB_SETCURSEL, static_cast<int>(m_currentType), 0);
}

void DictionaryDialog::InitializeListView(HWND hDlg) {
    HWND hListView = GetDlgItem(hDlg, IDC_DICT_LISTVIEW);
    if (!hListView) return;

    // Set extended styles
    ListView_SetExtendedListViewStyle(hListView,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

    SetupListViewColumns(hDlg);
}

void DictionaryDialog::SetupListViewColumns(HWND hDlg) {
    HWND hListView = GetDlgItem(hDlg, IDC_DICT_LISTVIEW);
    if (!hListView) return;

    // The columns depend on the dictionary: remove the previous ones
    while (ListView_DeleteColumn(hListView, 0)) {
    }

    if (IsAccentLexicon()) {
        InsertColumn(hListView, 0, IDS_DICT_COL_ENTRY, 160);
        InsertColumn(hListView, 1, IDS_DICT_COL_KIND, 60);
        InsertColumn(hListView, 2, IDS_DICT_COL_LANGUAGE, 80);
        InsertColumn(hListView, 3, IDS_DICT_COL_COMMENT, 150);
        return;
    }

    InsertColumn(hListView, 0, IDS_DICT_COL_ORIGINAL, 120);
    InsertColumn(hListView, 1, IDS_DICT_COL_REPLACEMENT, 120);
    InsertColumn(hListView, 2, IDS_DICT_COL_CASE, 80);
    InsertColumn(hListView, 3, IDS_DICT_COL_WHOLE_WORD, 80);
    InsertColumn(hListView, 4, IDS_DICT_COL_COMMENT, 150);
}

void DictionaryDialog::InsertColumn(HWND hListView, int index, UINT titleId, int width) {
    // The title has to outlive the insert: the control copies it only then
    std::wstring title = LoadLocalizedString(titleId);
    LVCOLUMNW col = {};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    col.pszText = const_cast<LPWSTR>(title.c_str());
    col.cx = width;
    col.iSubItem = index;
    ListView_InsertColumn(hListView, index, &col);
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

    if (IsAccentLexicon()) {
        std::wstring wordStr = LoadLocalizedString(IDS_ACCENT_KIND_WORD);
        std::wstring verbStr = LoadLocalizedString(IDS_ACCENT_KIND_VERB);
        for (size_t i = 0; i < m_entries.size(); i++) {
            const auto& entry = m_entries[i];
            item.iItem = static_cast<int>(i);
            item.iSubItem = 0;
            item.pszText = const_cast<LPWSTR>(entry.grapheme.c_str());
            ListView_InsertItem(hListView, &item);
            ListView_SetItemText(hListView, static_cast<int>(i), 1,
                const_cast<LPWSTR>(entry.verb ? verbStr.c_str() : wordStr.c_str()));
            std::wstring languageStr = AccentLanguageName(entry.language);
            ListView_SetItemText(hListView, static_cast<int>(i), 2,
                const_cast<LPWSTR>(languageStr.c_str()));
            ListView_SetItemText(hListView, static_cast<int>(i), 3,
                const_cast<LPWSTR>(entry.comment.c_str()));
        }
        return;
    }

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

    // Read file (a filesystem::path: a stream from a wstring is MSVC-only)
    std::ifstream file(std::filesystem::path(path), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    // Parse JSON
    m_entries = IsAccentLexicon() ? ParseAccentJson(buffer.str())
                                  : ParseDictionaryJson(buffer.str());

    return true;
}

bool DictionaryDialog::SaveDictionary() {
    std::wstring path = GetDictionaryPath(m_currentType);
    if (path.empty()) return false;

    // Ensure directory exists
    UserConfig config;
    config.ensure_config_directory();

    // Generate JSON
    std::string json = IsAccentLexicon() ? GenerateAccentJson(m_entries)
                                         : GenerateDictionaryJson(m_entries);

    // Write file
    std::ofstream file(std::filesystem::path(path), std::ios::binary);
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

    // Refresh list view (the accent lexicon has columns of its own)
    SetupListViewColumns(hDlg);
    PopulateListView(hDlg);

    // Select the first item by default
    HWND hListView = GetDlgItem(hDlg, IDC_DICT_LISTVIEW);
    if (hListView && !m_entries.empty()) {
        ListView_SetItemState(hListView, 0, LVIS_SELECTED | LVIS_FOCUSED,
            LVIS_SELECTED | LVIS_FOCUSED);
    }

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

    case IDCANCEL:
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

        case NM_CUSTOMDRAW:
            if (m_darkMode) {
                LPNMLVCUSTOMDRAW lpcd = reinterpret_cast<LPNMLVCUSTOMDRAW>(pnmh);
                switch (lpcd->nmcd.dwDrawStage) {
                case CDDS_PREPAINT:
                    SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
                    return TRUE;
                case CDDS_ITEMPREPAINT: {
                    lpcd->clrText = GetDarkTextColor();
                    lpcd->clrTextBk = GetDarkControlColor();
                    SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, CDRF_NEWFONT);
                    return TRUE;
                }
                }
            }
            break;
        }
    }

    return FALSE;
}

INT_PTR DictionaryDialog::OnCtlColor(HWND hDlg, UINT msg, HDC hdc, HWND hCtrl) {
    (void)hDlg;
    (void)hCtrl;
    return DarkCtlColor(m_darkMode, msg, hdc);
}

// Open the editor of the current dictionary's entries; true with the result
// in `result` when the user pressed OK
bool DictionaryDialog::EditEntry(HWND hDlg, const UserDictionaryEntry* entry, bool isEdit,
                                 UserDictionaryEntry& result) {
    AccentEntryDialog accentDlg;
    DictionaryEntryDialog replacementDlg;
    EntryDialogBase& dlg = IsAccentLexicon()
        ? static_cast<EntryDialogBase&>(accentDlg)
        : static_cast<EntryDialogBase&>(replacementDlg);
    if (dlg.Show(m_hInstance, hDlg, entry, isEdit) != IDOK) return false;
    result = dlg.GetEntry();
    return true;
}

void DictionaryDialog::OnAddEntry(HWND hDlg) {
    HWND hListView = GetDlgItem(hDlg, IDC_DICT_LISTVIEW);
    int currentIndex = GetSelectedIndex(hListView);

    UserDictionaryEntry entry;
    bool added = EditEntry(hDlg, nullptr, false, entry);
    if (added) {
        m_entries.push_back(entry);
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

    UserDictionaryEntry entry;
    if (EditEntry(hDlg, &m_entries[index], true, entry)) {
        m_entries[index] = entry;
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

    // Create a copy. An accent entry is kept as it is: " (copy)" would make
    // it a different, unaccented word.
    UserDictionaryEntry copy = m_entries[index];
    if (!IsAccentLexicon()) {
        copy.grapheme += L" (copy)";
    }

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
    case DictionaryType::Accents:
        return Utf8ToWide(config.get_user_accent_lexicon_path());
    }
    return L"";
}

std::wstring DictionaryDialog::AccentLanguageName(const std::wstring& code) {
    if (code == L"hr") return LoadLocalizedString(IDS_ACCENT_LANG_HR);
    if (code == L"sr") return LoadLocalizedString(IDS_ACCENT_LANG_SR);
    if (code == L"bs") return LoadLocalizedString(IDS_ACCENT_LANG_BS);
    if (code == L"sl") return LoadLocalizedString(IDS_ACCENT_LANG_SL);
    if (code.empty()) return LoadLocalizedString(IDS_ACCENT_LANG_ALL);
    return code;
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

    // Set ListView background color
    HWND hListView = GetDlgItem(hDlg, IDC_DICT_LISTVIEW);
    if (hListView) {
        ListView_SetBkColor(hListView, GetDarkControlColor());
        ListView_SetTextBkColor(hListView, GetDarkControlColor());
        ListView_SetTextColor(hListView, GetDarkTextColor());
    }

    InvalidateRect(hDlg, nullptr, TRUE);
}

std::wstring DictionaryDialog::LoadLocalizedString(UINT id) {
    return LoadResourceString(m_hInstance, id);
}

void DictionaryDialog::SetControlText(HWND hDlg, int ctrlId, UINT stringId) {
    std::wstring text = LoadLocalizedString(stringId);
    if (!text.empty()) {
        SetDlgItemTextW(hDlg, ctrlId, text.c_str());
    }
}

// =============================================================================
// EntryDialogBase Implementation
// =============================================================================

INT_PTR EntryDialogBase::Show(HINSTANCE hInstance, HWND hParent,
    const UserDictionaryEntry* entry, bool isEdit) {
    m_hInstance = hInstance;
    m_isEdit = isEdit;
    m_darkMode = IsSystemDarkMode();
    m_entry = entry ? *entry : UserDictionaryEntry();

    return DialogBoxParamW(
        hInstance,
        MAKEINTRESOURCEW(DialogResourceId()),
        hParent,
        DialogProc,
        reinterpret_cast<LPARAM>(this)
    );
}

INT_PTR CALLBACK EntryDialogBase::DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    EntryDialogBase* pThis = nullptr;

    if (msg == WM_INITDIALOG) {
        pThis = reinterpret_cast<EntryDialogBase*>(lParam);
        SetWindowLongPtrW(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hDlg = hDlg;
        return pThis->OnInitDialog(hDlg);
    }

    pThis = reinterpret_cast<EntryDialogBase*>(GetWindowLongPtrW(hDlg, GWLP_USERDATA));
    if (!pThis) {
        return FALSE;
    }

    switch (msg) {
    case WM_COMMAND:
        return pThis->OnCommand(hDlg, wParam, lParam);

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORDLG:
        return DarkCtlColor(pThis->m_darkMode, msg, (HDC)wParam);

    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        return TRUE;
    }

    return FALSE;
}

INT_PTR EntryDialogBase::OnInitDialog(HWND hDlg) {
    InitializeControls(hDlg);

    if (m_darkMode) {
        ApplyDarkTitleBar(hDlg, true);
        ApplyDarkModeToAllControls(hDlg, true);
        InvalidateRect(hDlg, nullptr, TRUE);
    }

    CenterOnParent(hDlg);
    return TRUE;
}

INT_PTR EntryDialogBase::OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam) {
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

std::wstring EntryDialogBase::LoadLocalizedString(UINT id) {
    return LoadResourceString(m_hInstance, id);
}

void EntryDialogBase::SetControlText(HWND hDlg, int ctrlId, UINT stringId) {
    std::wstring text = LoadLocalizedString(stringId);
    if (!text.empty()) {
        SetDlgItemTextW(hDlg, ctrlId, text.c_str());
    }
}

// =============================================================================
// DictionaryEntryDialog Implementation
// =============================================================================

int DictionaryEntryDialog::DialogResourceId() const {
    return IDD_DICT_ENTRY_DIALOG;
}

void DictionaryEntryDialog::InitializeControls(HWND hDlg) {
    SetWindowTextW(hDlg, LoadLocalizedString(
        m_isEdit ? IDS_ENTRY_DIALOG_TITLE_EDIT : IDS_ENTRY_DIALOG_TITLE_ADD).c_str());

    SetControlText(hDlg, IDC_ENTRY_ORIGINAL_LABEL, IDS_ENTRY_ORIGINAL_LABEL);
    SetControlText(hDlg, IDC_ENTRY_REPLACEMENT_LABEL, IDS_ENTRY_REPLACEMENT_LABEL);
    SetControlText(hDlg, IDC_ENTRY_COMMENT_LABEL, IDS_ENTRY_COMMENT_LABEL);
    SetControlText(hDlg, IDC_ENTRY_CASE_SENSITIVE, IDS_ENTRY_CASE_SENSITIVE);
    SetControlText(hDlg, IDC_ENTRY_WHOLE_WORD, IDS_ENTRY_WHOLE_WORD);
    SetControlText(hDlg, IDOK, IDS_OK);
    SetControlText(hDlg, IDCANCEL, IDS_CANCEL);

    // Populate with existing entry data
    SetDlgItemTextW(hDlg, IDC_ENTRY_ORIGINAL_EDIT, m_entry.grapheme.c_str());
    SetDlgItemTextW(hDlg, IDC_ENTRY_REPLACEMENT_EDIT, m_entry.phoneme.c_str());
    SetDlgItemTextW(hDlg, IDC_ENTRY_COMMENT_EDIT, m_entry.comment.c_str());
    CheckDlgButton(hDlg, IDC_ENTRY_CASE_SENSITIVE, m_entry.caseSensitive ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_ENTRY_WHOLE_WORD, m_entry.wholeWord ? BST_CHECKED : BST_UNCHECKED);
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

// =============================================================================
// AccentEntryDialog Implementation
// =============================================================================

namespace {

// The language combo: all, Croatian, Serbian, Bosnian, Slovenian (the order of
// ACCENT_LANGUAGES); a code the engine does not know (a hand-edited file)
// is shown as itself in a sixth item, so that OK keeps it and the engine's
// check names it instead of the dialog silently saving "all languages".
constexpr int ACCENT_LANGUAGE_COUNT = 5;
constexpr int ACCENT_LANGUAGE_UNKNOWN = ACCENT_LANGUAGE_COUNT;

} // namespace

int AccentEntryDialog::DialogResourceId() const {
    return IDD_ACCENT_ENTRY_DIALOG;
}

void AccentEntryDialog::InitializeControls(HWND hDlg) {
    SetWindowTextW(hDlg, LoadLocalizedString(
        m_isEdit ? IDS_ACCENT_DIALOG_TITLE_EDIT : IDS_ACCENT_DIALOG_TITLE_ADD).c_str());

    SetControlText(hDlg, IDC_ACCENT_ENTRY_LABEL, IDS_ACCENT_ENTRY_LABEL);
    SetControlText(hDlg, IDC_ACCENT_KIND_LABEL, IDS_ACCENT_KIND_LABEL);
    SetControlText(hDlg, IDC_ACCENT_LANG_LABEL, IDS_ACCENT_LANG_LABEL);
    SetControlText(hDlg, IDC_ACCENT_COMMENT_LABEL, IDS_ENTRY_COMMENT_LABEL);
    SetControlText(hDlg, IDC_ACCENT_HELP, IDS_ACCENT_HELP);
    SetControlText(hDlg, IDOK, IDS_OK);
    SetControlText(hDlg, IDCANCEL, IDS_CANCEL);

    SetDlgItemTextW(hDlg, IDC_ACCENT_ENTRY_EDIT, m_entry.grapheme.c_str());
    SetDlgItemTextW(hDlg, IDC_ACCENT_COMMENT_EDIT, m_entry.comment.c_str());

    // Kind: word or verb
    HWND hKind = GetDlgItem(hDlg, IDC_ACCENT_KIND_COMBO);
    if (hKind) {
        SendMessageW(hKind, CB_ADDSTRING, 0, (LPARAM)LoadLocalizedString(IDS_ACCENT_KIND_WORD).c_str());
        SendMessageW(hKind, CB_ADDSTRING, 0, (LPARAM)LoadLocalizedString(IDS_ACCENT_KIND_VERB).c_str());
        SendMessageW(hKind, CB_SETCURSEL, m_entry.verb ? 1 : 0, 0);
    }

    HWND hLang = GetDlgItem(hDlg, IDC_ACCENT_LANG_COMBO);
    if (hLang) {
        SendMessageW(hLang, CB_ADDSTRING, 0, (LPARAM)LoadLocalizedString(IDS_ACCENT_LANG_ALL).c_str());
        SendMessageW(hLang, CB_ADDSTRING, 0, (LPARAM)LoadLocalizedString(IDS_ACCENT_LANG_HR).c_str());
        SendMessageW(hLang, CB_ADDSTRING, 0, (LPARAM)LoadLocalizedString(IDS_ACCENT_LANG_SR).c_str());
        SendMessageW(hLang, CB_ADDSTRING, 0, (LPARAM)LoadLocalizedString(IDS_ACCENT_LANG_BS).c_str());
        SendMessageW(hLang, CB_ADDSTRING, 0, (LPARAM)LoadLocalizedString(IDS_ACCENT_LANG_SL).c_str());
        int sel = -1;
        std::string code = WideToUtf8(m_entry.language);
        for (int i = 0; i < ACCENT_LANGUAGE_COUNT; i++) {
            if (code == ACCENT_LANGUAGES[i]) sel = i;
        }
        if (sel < 0) {
            SendMessageW(hLang, CB_ADDSTRING, 0, (LPARAM)m_entry.language.c_str());
            sel = ACCENT_LANGUAGE_UNKNOWN;
        }
        SendMessageW(hLang, CB_SETCURSEL, sel, 0);
    }
}

bool AccentEntryDialog::ValidateAndCollect(HWND hDlg) {
    wchar_t buffer[1024];
    GetDlgItemTextW(hDlg, IDC_ACCENT_ENTRY_EDIT, buffer, 1024);
    std::wstring text = buffer;
    // Trim surrounding whitespace
    size_t first = text.find_first_not_of(L" \t\r\n");
    size_t last = text.find_last_not_of(L" \t\r\n");
    text = first == std::wstring::npos ? L"" : text.substr(first, last - first + 1);

    if (text.empty()) {
        std::wstring msg = LoadLocalizedString(IDS_ERROR_ACCENT_EMPTY);
        MessageBoxW(hDlg, msg.c_str(), L"Error", MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(hDlg, IDC_ACCENT_ENTRY_EDIT));
        return false;
    }

    UserDictionaryEntry entry = m_entry;  // an unknown language code stays
    entry.grapheme = text;
    entry.verb = SendMessageW(GetDlgItem(hDlg, IDC_ACCENT_KIND_COMBO), CB_GETCURSEL, 0, 0) == 1;
    int lang = static_cast<int>(SendMessageW(GetDlgItem(hDlg, IDC_ACCENT_LANG_COMBO), CB_GETCURSEL, 0, 0));
    if (lang >= 0 && lang < ACCENT_LANGUAGE_COUNT) {
        entry.language = Utf8ToWide(ACCENT_LANGUAGES[lang]);
    }

    GetDlgItemTextW(hDlg, IDC_ACCENT_COMMENT_EDIT, buffer, 1024);
    entry.comment = buffer;

    // The engine's own parser decides whether the entry is well formed, so
    // a typo is caught here and not silently skipped at load time
    formant::UserLexicon::Report report;
    formant::UserLexicon::parse("{\"entries\":[" + AccentEntryJson(entry, "") + "]}", &report);
    if (report.rejected > 0) {
        // The reason quotes the whole entry, so the message has no fixed size
        std::wstring msg = FormatResourceString(
            LoadLocalizedString(IDS_ERROR_ACCENT_INVALID), Utf8ToWide(report.first_error));
        MessageBoxW(hDlg, msg.c_str(), L"Error", MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(hDlg, IDC_ACCENT_ENTRY_EDIT));
        return false;
    }

    m_entry = entry;
    return true;
}

} // namespace laprdus

#endif // _WIN32
