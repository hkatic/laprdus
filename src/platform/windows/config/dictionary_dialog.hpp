// -*- coding: utf-8 -*-
// dictionary_dialog.hpp - Dictionary management dialogs for LaprdusTTS

#ifndef LAPRDUS_DICTIONARY_DIALOG_HPP
#define LAPRDUS_DICTIONARY_DIALOG_HPP

#ifdef _WIN32

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>

namespace laprdus {

/**
 * Dictionary type enumeration.
 */
enum class DictionaryType {
    Main = 0,      // Main user pronunciation dictionary (user.json)
    Spelling = 1,  // Spelling dictionary for character pronunciation (spelling.json)
    Emoji = 2      // Emoji dictionary (emoji.json)
};

/**
 * User dictionary entry structure.
 * Matches the JSON format used in dictionary files.
 */
struct UserDictionaryEntry {
    std::wstring grapheme;      // Original text to match
    std::wstring phoneme;       // Replacement/pronunciation
    std::wstring comment;       // Optional comment
    bool caseSensitive = false; // Case-sensitive matching
    bool wholeWord = true;      // Whole word matching only
};

/**
 * DictionaryDialog - Dialog for viewing and managing dictionary entries.
 *
 * Features:
 * - Dictionary type selector (Main, Spelling, Emoji)
 * - ListView showing all entries
 * - Add, Edit, Duplicate, Delete buttons
 * - Loads/saves JSON files from user config directory
 */
class DictionaryDialog {
public:
    DictionaryDialog();
    ~DictionaryDialog();

    /**
     * Show the dictionary management dialog.
     * @param hInstance Application instance
     * @param hParent Parent window handle
     * @return IDOK if changes were saved, IDCANCEL otherwise
     */
    INT_PTR Show(HINSTANCE hInstance, HWND hParent);

private:
    // Dialog procedure
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

    // Message handlers
    INT_PTR OnInitDialog(HWND hDlg);
    INT_PTR OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);
    INT_PTR OnNotify(HWND hDlg, LPNMHDR pnmh);
    INT_PTR OnCtlColor(HWND hDlg, UINT msg, HDC hdc, HWND hCtrl);

    // Control initialization
    void InitializeControls(HWND hDlg);
    void InitializeDictionaryTypeCombo(HWND hDlg);
    void InitializeListView(HWND hDlg);
    void UpdateButtonStates(HWND hDlg);

    // Dictionary operations
    bool LoadDictionary(DictionaryType type);
    bool SaveDictionary();
    void PopulateListView(HWND hDlg);
    void OnDictionaryTypeChanged(HWND hDlg);

    // CRUD operations
    void OnAddEntry(HWND hDlg);
    void OnEditEntry(HWND hDlg);
    void OnDuplicateEntry(HWND hDlg);
    void OnDeleteEntry(HWND hDlg);

    // Helpers
    std::wstring GetDictionaryPath(DictionaryType type);
    std::wstring LoadLocalizedString(UINT id);
    void SetControlText(HWND hDlg, int ctrlId, UINT stringId);
    int GetSelectedIndex(HWND hListView);
    void ShowError(HWND hDlg, UINT stringId);

    // Dark mode
    void ApplyDarkMode(HWND hDlg);
    bool IsDarkModeEnabled();

    // Member variables
    HINSTANCE m_hInstance = nullptr;
    HWND m_hDlg = nullptr;
    bool m_darkMode = false;

    DictionaryType m_currentType = DictionaryType::Main;
    std::vector<UserDictionaryEntry> m_entries;
    bool m_modified = false;
};

/**
 * DictionaryEntryDialog - Dialog for adding/editing a dictionary entry.
 *
 * Features:
 * - Original and replacement text fields
 * - Comment text field
 * - Case sensitive and whole word checkboxes
 */
class DictionaryEntryDialog {
public:
    DictionaryEntryDialog();
    ~DictionaryEntryDialog();

    /**
     * Show the entry editor dialog.
     * @param hInstance Application instance
     * @param hParent Parent window handle
     * @param entry Existing entry to edit (nullptr for new entry)
     * @param isEdit True if editing, false if adding
     * @return IDOK if entry was saved, IDCANCEL otherwise
     */
    INT_PTR Show(HINSTANCE hInstance, HWND hParent,
                 const UserDictionaryEntry* entry = nullptr,
                 bool isEdit = false);

    /**
     * Get the entry data after dialog closes.
     * @return The entry data entered by user
     */
    const UserDictionaryEntry& GetEntry() const { return m_entry; }

private:
    // Dialog procedure
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

    // Message handlers
    INT_PTR OnInitDialog(HWND hDlg);
    INT_PTR OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);
    INT_PTR OnCtlColor(HWND hDlg, UINT msg, HDC hdc, HWND hCtrl);

    // Control handling
    void InitializeControls(HWND hDlg);
    bool ValidateAndCollect(HWND hDlg);

    // Helpers
    std::wstring LoadLocalizedString(UINT id);
    void SetControlText(HWND hDlg, int ctrlId, UINT stringId);

    // Dark mode
    void ApplyDarkMode(HWND hDlg);
    bool IsDarkModeEnabled();

    // Member variables
    HINSTANCE m_hInstance = nullptr;
    HWND m_hDlg = nullptr;
    bool m_isEdit = false;
    bool m_darkMode = false;
    UserDictionaryEntry m_entry;
};

} // namespace laprdus

#endif // _WIN32
#endif // LAPRDUS_DICTIONARY_DIALOG_HPP
