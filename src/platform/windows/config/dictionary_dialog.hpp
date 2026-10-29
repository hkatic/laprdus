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
    Emoji = 2,     // Emoji dictionary (emoji.json)
    Accents = 3    // Accent lexicon: word stress, length and tone (accents.json)
};

/**
 * User dictionary entry structure.
 * Matches the JSON format used in dictionary files.
 *
 * An entry of the accent lexicon (accents.json) keeps its text (a word, a
 * stem or a paradigm in the notation of the built-in lexicon, or a verb) in
 * grapheme, with phoneme empty; verb tells which of the two JSON keys it is
 * saved under, and language ("hr", "sr", "bs", "sl" or "" for every language)
 * limits it to one language.
 */
struct UserDictionaryEntry {
    std::wstring grapheme;      // Original text to match (accent lexicon: the entry)
    std::wstring phoneme;       // Replacement/pronunciation
    std::wstring comment;       // Optional comment
    bool caseSensitive = false; // Case-sensitive matching
    bool wholeWord = true;      // Whole word matching only
    bool verb = false;          // Accent lexicon: a "verb" entry, not a "word"
    std::wstring language;      // Accent lexicon: "hr", "sr", "bs", "sl" or "" (all)
};

/**
 * DictionaryDialog - Dialog for viewing and managing dictionary entries.
 *
 * Features:
 * - Dictionary type selector (Main, Spelling, Emoji, Accents)
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
    void SetupListViewColumns(HWND hDlg);
    void InsertColumn(HWND hListView, int index, UINT titleId, int width);
    void UpdateButtonStates(HWND hDlg);
    bool IsAccentLexicon() const { return m_currentType == DictionaryType::Accents; }

    // Dictionary operations
    bool LoadDictionary(DictionaryType type);
    bool SaveDictionary();
    void PopulateListView(HWND hDlg);
    void OnDictionaryTypeChanged(HWND hDlg);

    // CRUD operations
    bool EditEntry(HWND hDlg, const UserDictionaryEntry* entry, bool isEdit,
                   UserDictionaryEntry& result);
    void OnAddEntry(HWND hDlg);
    void OnEditEntry(HWND hDlg);
    void OnDuplicateEntry(HWND hDlg);
    void OnDeleteEntry(HWND hDlg);

    // Helpers
    std::wstring GetDictionaryPath(DictionaryType type);
    std::wstring AccentLanguageName(const std::wstring& code);
    std::wstring LoadLocalizedString(UINT id);
    void SetControlText(HWND hDlg, int ctrlId, UINT stringId);
    int GetSelectedIndex(HWND hListView);
    void ShowError(HWND hDlg, UINT stringId);

    // Dark mode
    void ApplyDarkMode(HWND hDlg);

    // Member variables
    HINSTANCE m_hInstance = nullptr;
    HWND m_hDlg = nullptr;
    bool m_darkMode = false;

    DictionaryType m_currentType = DictionaryType::Main;
    std::vector<UserDictionaryEntry> m_entries;
    bool m_modified = false;
};

/**
 * EntryDialogBase - What the two entry editors share: the modal dialog loop,
 * dark mode, centering on the parent, localized strings. A derived class
 * names its dialog resource, fills its controls and collects the entry.
 */
class EntryDialogBase {
public:
    virtual ~EntryDialogBase() = default;

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

    /** The entry data after the dialog closes with IDOK. */
    const UserDictionaryEntry& GetEntry() const { return m_entry; }

protected:
    virtual int DialogResourceId() const = 0;
    /** Fill the controls from m_entry (title, labels, values). */
    virtual void InitializeControls(HWND hDlg) = 0;
    /** Read the controls into m_entry; false (after a message) keeps the dialog open. */
    virtual bool ValidateAndCollect(HWND hDlg) = 0;

    std::wstring LoadLocalizedString(UINT id);
    void SetControlText(HWND hDlg, int ctrlId, UINT stringId);

    HINSTANCE m_hInstance = nullptr;
    HWND m_hDlg = nullptr;
    bool m_isEdit = false;
    bool m_darkMode = false;
    UserDictionaryEntry m_entry;

private:
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    INT_PTR OnInitDialog(HWND hDlg);
    INT_PTR OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);
};

/**
 * DictionaryEntryDialog - Dialog for adding/editing a dictionary entry.
 *
 * Features:
 * - Original and replacement text fields
 * - Comment text field
 * - Case sensitive and whole word checkboxes
 */
class DictionaryEntryDialog : public EntryDialogBase {
protected:
    int DialogResourceId() const override;
    void InitializeControls(HWND hDlg) override;
    bool ValidateAndCollect(HWND hDlg) override;
};

/**
 * AccentEntryDialog - Dialog for adding/editing an accent lexicon entry.
 *
 * Features:
 * - Entry text in the notation of the built-in lexicon
 * - Kind (word or verb) and language (all, hr, sr, bs) selectors
 * - Comment text field, a short reminder of the notation
 * - The entry is checked by the engine's own parser before it is accepted
 */
class AccentEntryDialog : public EntryDialogBase {
protected:
    int DialogResourceId() const override;
    void InitializeControls(HWND hDlg) override;
    bool ValidateAndCollect(HWND hDlg) override;
};

} // namespace laprdus

#endif // _WIN32
#endif // LAPRDUS_DICTIONARY_DIALOG_HPP
