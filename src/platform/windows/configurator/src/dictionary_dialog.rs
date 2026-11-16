//! The dictionary editor: a list of the entries of one of the four
//! dictionaries with Add, Edit, Duplicate and Delete, and the two dialogs
//! that edit one entry (a replacement, or an entry of the accent lexicon).
//! Changes are saved when the dictionary type changes and when the dialog
//! closes.

use crate::dark_mode;
use crate::dictionary::{self, ACCENT_LANGUAGES, Entry, Kind};
use crate::ids::*;
use crate::text::{insert, set_texts};
use crate::win32::{self, CBN_SELCHANGE, ColorOf, Dialog, Event, IDCANCEL, IDOK, Icon, ListEvent, Reply, Window};
use std::cell::{Cell, RefCell};

const DIALOG_TEXTS: [(u16, u16); 6] = [
    (IDC_DICT_TYPE_LABEL, IDS_DICT_TYPE_LABEL),
    (IDC_DICT_ADD, IDS_DICT_ADD),
    (IDC_DICT_EDIT, IDS_DICT_EDIT),
    (IDC_DICT_DUPLICATE, IDS_DICT_DUPLICATE),
    (IDC_DICT_DELETE, IDS_DICT_DELETE),
    (IDC_DICT_CLOSE, IDS_DICT_CLOSE),
];
const TYPE_NAMES: [u16; 4] = [IDS_DICT_TYPE_MAIN, IDS_DICT_TYPE_SPELLING, IDS_DICT_TYPE_EMOJI, IDS_DICT_TYPE_ACCENTS];

// The list's columns: title and width in pixels.
const REPLACEMENT_COLUMNS: [(u16, i32); 5] = [
    (IDS_DICT_COL_ORIGINAL, 120),
    (IDS_DICT_COL_REPLACEMENT, 120),
    (IDS_DICT_COL_CASE, 100),
    (IDS_DICT_COL_WHOLE_WORD, 80),
    (IDS_DICT_COL_COMMENT, 150),
];
const ACCENT_COLUMNS: [(u16, i32); 4] =
    [(IDS_DICT_COL_ENTRY, 160), (IDS_DICT_COL_KIND, 60), (IDS_DICT_COL_LANGUAGE, 80), (IDS_DICT_COL_COMMENT, 150)];

/// The names of [`ACCENT_LANGUAGES`], in its order.
const ACCENT_LANGUAGE_NAMES: [u16; 5] =
    [IDS_ACCENT_LANG_ALL, IDS_ACCENT_LANG_HR, IDS_ACCENT_LANG_SR, IDS_ACCENT_LANG_BS, IDS_ACCENT_LANG_SL];

const REPLACEMENT_TEXTS: [(u16, u16); 7] = [
    (IDC_ENTRY_ORIGINAL_LABEL, IDS_ENTRY_ORIGINAL_LABEL),
    (IDC_ENTRY_REPLACEMENT_LABEL, IDS_ENTRY_REPLACEMENT_LABEL),
    (IDC_ENTRY_COMMENT_LABEL, IDS_ENTRY_COMMENT_LABEL),
    (IDC_ENTRY_CASE_SENSITIVE, IDS_ENTRY_CASE_SENSITIVE),
    (IDC_ENTRY_WHOLE_WORD, IDS_ENTRY_WHOLE_WORD),
    (IDOK, IDS_OK),
    (IDCANCEL, IDS_CANCEL),
];
const ACCENT_TEXTS: [(u16, u16); 7] = [
    (IDC_ACCENT_ENTRY_LABEL, IDS_ACCENT_ENTRY_LABEL),
    (IDC_ACCENT_KIND_LABEL, IDS_ACCENT_KIND_LABEL),
    (IDC_ACCENT_LANG_LABEL, IDS_ACCENT_LANG_LABEL),
    (IDC_ACCENT_COMMENT_LABEL, IDS_ENTRY_COMMENT_LABEL),
    (IDC_ACCENT_HELP, IDS_ACCENT_HELP),
    (IDOK, IDS_OK),
    (IDCANCEL, IDS_CANCEL),
];

/// The name of an accent entry's language; a code the engine does not know
/// (a hand-edited file) is shown as itself.
fn accent_language_name(code: &str) -> String {
    match ACCENT_LANGUAGES.iter().position(|&known| known == code) {
        Some(index) => win32::string(ACCENT_LANGUAGE_NAMES[index]),
        None => code.to_owned(),
    }
}

fn warn(owner: Window, text: &str) {
    win32::message_box(owner, text, &win32::string(IDS_ERROR_TITLE), Icon::Warning);
}

/// Opens the dictionary editor over the settings dialog.
pub fn run(parent: Window) {
    let dialog = Dictionaries {
        kind: Cell::new(Kind::Main),
        entries: RefCell::new(dictionary::load(Kind::Main)),
        modified: Cell::new(false),
        dark: dark_mode::enabled(),
    };
    win32::run_dialog(IDD_DICTIONARY_DIALOG, Some(parent), &dialog);
}

struct Dictionaries {
    kind: Cell<Kind>,
    entries: RefCell<Vec<Entry>>,
    modified: Cell<bool>,
    dark: bool,
}

impl Dictionaries {
    fn accents(&self) -> bool {
        self.kind.get() == Kind::Accents
    }

    /// The list's columns, which depend on the dictionary, its rows, the
    /// first row selected, and the buttons that need a selection.
    fn show_entries(&self, dialog: Window) {
        let list = dialog.item(IDC_DICT_LISTVIEW);
        let columns: &[(u16, i32)] = if self.accents() { &ACCENT_COLUMNS } else { &REPLACEMENT_COLUMNS };
        let columns: Vec<_> = columns.iter().map(|&(title, width)| (win32::string(title), width)).collect();
        list.set_list_columns(&columns);
        self.show_rows(dialog);
        if !self.entries.borrow().is_empty() {
            list.select_list_row(0);
        }
        self.update_buttons(dialog);
    }

    fn show_rows(&self, dialog: Window) {
        let (yes, no) = (win32::string(IDS_YES), win32::string(IDS_NO));
        let yes_no = |value: bool| if value { yes.clone() } else { no.clone() };
        let (word, verb) = (win32::string(IDS_ACCENT_KIND_WORD), win32::string(IDS_ACCENT_KIND_VERB));
        let accents = self.accents();
        let row = |entry: &Entry| {
            let (text, comment) = (entry.grapheme.clone(), entry.comment.clone());
            if accents {
                let kind = if entry.verb { verb.clone() } else { word.clone() };
                vec![text, kind, accent_language_name(&entry.language), comment]
            } else {
                vec![text, entry.phoneme.clone(), yes_no(entry.case_sensitive), yes_no(entry.whole_word), comment]
            }
        };
        // The list notifies the dialog while it is filled: the entries are
        // not borrowed then.
        let rows: Vec<Vec<String>> = self.entries.borrow().iter().map(row).collect();
        dialog.item(IDC_DICT_LISTVIEW).set_list_rows(&rows);
    }

    /// Edit, Duplicate and Delete need a selected entry.
    fn update_buttons(&self, dialog: Window) {
        let selected = dialog.item(IDC_DICT_LISTVIEW).selected_list_row().is_some();
        for button in [IDC_DICT_EDIT, IDC_DICT_DUPLICATE, IDC_DICT_DELETE] {
            dialog.item(button).enable(selected);
        }
    }

    /// The selected entry's index.
    fn selected(&self, dialog: Window) -> Option<usize> {
        let index = dialog.item(IDC_DICT_LISTVIEW).selected_list_row()?;
        (index < self.entries.borrow().len()).then_some(index)
    }

    /// Shows the changed entries with one of them selected and focused.
    fn changed(&self, dialog: Window, select: Option<usize>) {
        self.modified.set(true);
        self.show_rows(dialog);
        self.select(dialog, select);
        self.update_buttons(dialog);
    }

    /// Puts the focus back on the list, on a row when there is one.
    fn select(&self, dialog: Window, row: Option<usize>) {
        let list = dialog.item(IDC_DICT_LISTVIEW);
        if let Some(row) = row {
            list.select_list_row(row);
        }
        list.focus();
    }

    fn save_if_modified(&self, dialog: Window) {
        if !self.modified.get() {
            return;
        }
        let entries = self.entries.borrow().clone();
        if dictionary::save(self.kind.get(), &entries) {
            self.modified.set(false);
        } else {
            win32::message_box(
                dialog,
                &win32::string(IDS_ERROR_FILE_WRITE),
                &win32::string(IDS_ERROR_TITLE),
                Icon::Error,
            );
        }
    }

    fn type_changed(&self, dialog: Window) {
        self.save_if_modified(dialog);
        let chosen = dialog.item(IDC_DICT_TYPE_COMBO).choice().and_then(|index| Kind::ALL.get(index).copied());
        let Some(kind) = chosen else { return };
        self.kind.set(kind);
        self.entries.replace(dictionary::load(kind));
        self.show_entries(dialog);
    }

    fn add(&self, dialog: Window) {
        let before = self.selected(dialog);
        match edit_entry(dialog, self.accents(), None) {
            Some(entry) => {
                self.entries.borrow_mut().push(entry);
                self.changed(dialog, Some(self.entries.borrow().len() - 1));
            }
            // Cancelled: back to the entry that was selected
            None => self.select(dialog, before),
        }
    }

    fn edit(&self, dialog: Window) {
        let Some(index) = self.selected(dialog) else { return };
        let current = self.entries.borrow()[index].clone();
        if let Some(entry) = edit_entry(dialog, self.accents(), Some(current)) {
            if let Some(slot) = self.entries.borrow_mut().get_mut(index) {
                *slot = entry;
            }
            self.modified.set(true);
            self.show_rows(dialog);
        }
        // The edited entry stays selected, whether OK or Cancel
        self.select(dialog, Some(index));
    }

    fn duplicate(&self, dialog: Window) {
        let Some(index) = self.selected(dialog) else { return };
        let mut copy = self.entries.borrow()[index].clone();
        // An accent entry is kept as it is: the suffix would make it a
        // different, unaccented word.
        if !self.accents() {
            copy.grapheme += &win32::string(IDS_DUPLICATE_SUFFIX);
        }
        self.entries.borrow_mut().push(copy);
        self.changed(dialog, Some(self.entries.borrow().len() - 1));
    }

    fn delete(&self, dialog: Window) {
        let Some(index) = self.selected(dialog) else { return };
        self.entries.borrow_mut().remove(index);
        // The next entry takes the selection, else the previous one
        let left = self.entries.borrow().len();
        self.changed(dialog, left.checked_sub(1).map(|last| index.min(last)));
    }
}

impl Dialog for Dictionaries {
    fn event(&self, dialog: Window, event: Event) -> Reply {
        match event {
            Event::Init => {
                dialog.set_text(&win32::string(IDS_DICT_DIALOG_TITLE));
                set_texts(dialog, &DIALOG_TEXTS);
                let types = dialog.item(IDC_DICT_TYPE_COMBO);
                for name in TYPE_NAMES {
                    types.add_choice(&win32::string(name));
                }
                types.choose(0);
                let list = dialog.item(IDC_DICT_LISTVIEW);
                list.set_list_style();
                self.show_entries(dialog);
                if self.dark {
                    list.set_list_colors(dark_mode::TEXT, dark_mode::CONTROL);
                    dark_mode::apply(dialog, true);
                }
                dialog.center_on_parent();
            }
            Event::Command { id: IDC_DICT_TYPE_COMBO, code } => {
                if code == CBN_SELCHANGE {
                    self.type_changed(dialog);
                }
            }
            Event::Command { id: IDC_DICT_ADD, .. } => self.add(dialog),
            Event::Command { id: IDC_DICT_EDIT, .. } => self.edit(dialog),
            Event::Command { id: IDC_DICT_DUPLICATE, .. } => self.duplicate(dialog),
            Event::Command { id: IDC_DICT_DELETE, .. } => self.delete(dialog),
            Event::Command { id: IDCANCEL | IDC_DICT_CLOSE, .. } | Event::Close => {
                self.save_if_modified(dialog);
                dialog.end(IDOK);
            }
            Event::List { id: IDC_DICT_LISTVIEW, what } => match what {
                ListEvent::ItemChanged => self.update_buttons(dialog),
                ListEvent::DoubleClick => self.edit(dialog),
                ListEvent::Draw if self.dark => {
                    return Reply::ListColors { text: dark_mode::TEXT, back: dark_mode::CONTROL };
                }
                ListEvent::Draw => return Reply::Unhandled,
            },
            Event::Color { of, dc } => {
                let kinds = [ColorOf::Static, ColorOf::Button, ColorOf::Edit, ColorOf::Dialog];
                return dark_mode::color(self.dark, &kinds, of, dc);
            }
            _ => return Reply::Unhandled,
        }
        Reply::Handled
    }
}

/// Opens the editor of one entry, of a replacement dictionary or of the
/// accent lexicon; the entry when the user pressed OK.
fn edit_entry(parent: Window, accent: bool, existing: Option<Entry>) -> Option<Entry> {
    let editor = EntryEditor {
        accent,
        edit: existing.is_some(),
        entry: RefCell::new(existing.unwrap_or_default()),
        dark: dark_mode::enabled(),
    };
    let template = if accent { IDD_ACCENT_ENTRY_DIALOG } else { IDD_DICT_ENTRY_DIALOG };
    (win32::run_dialog(template, Some(parent), &editor) == IDOK as isize).then(|| editor.entry.into_inner())
}

struct EntryEditor {
    accent: bool,
    /// Editing an entry, not adding one: the title says which.
    edit: bool,
    entry: RefCell<Entry>,
    dark: bool,
}

impl EntryEditor {
    fn init(&self, dialog: Window) {
        let entry = self.entry.borrow().clone();
        if !self.accent {
            let title = if self.edit { IDS_ENTRY_DIALOG_TITLE_EDIT } else { IDS_ENTRY_DIALOG_TITLE_ADD };
            dialog.set_text(&win32::string(title));
            set_texts(dialog, &REPLACEMENT_TEXTS);
            dialog.item(IDC_ENTRY_ORIGINAL_EDIT).set_text(&entry.grapheme);
            dialog.item(IDC_ENTRY_REPLACEMENT_EDIT).set_text(&entry.phoneme);
            dialog.item(IDC_ENTRY_COMMENT_EDIT).set_text(&entry.comment);
            dialog.set_checked(IDC_ENTRY_CASE_SENSITIVE, entry.case_sensitive);
            dialog.set_checked(IDC_ENTRY_WHOLE_WORD, entry.whole_word);
            return;
        }
        let title = if self.edit { IDS_ACCENT_DIALOG_TITLE_EDIT } else { IDS_ACCENT_DIALOG_TITLE_ADD };
        dialog.set_text(&win32::string(title));
        set_texts(dialog, &ACCENT_TEXTS);
        dialog.item(IDC_ACCENT_ENTRY_EDIT).set_text(&entry.grapheme);
        dialog.item(IDC_ACCENT_COMMENT_EDIT).set_text(&entry.comment);

        let kinds = dialog.item(IDC_ACCENT_KIND_COMBO);
        kinds.add_choice(&win32::string(IDS_ACCENT_KIND_WORD));
        kinds.add_choice(&win32::string(IDS_ACCENT_KIND_VERB));
        kinds.choose(usize::from(entry.verb));

        // All languages, Croatian, Serbian, Bosnian, Slovenian; a code the
        // engine does not know is a sixth item, shown as itself, so that OK
        // keeps it and the engine's check names it instead of the dialog
        // silently saving "all languages".
        let languages = dialog.item(IDC_ACCENT_LANG_COMBO);
        for name in ACCENT_LANGUAGE_NAMES {
            languages.add_choice(&win32::string(name));
        }
        let known = ACCENT_LANGUAGES.iter().position(|&code| code == entry.language);
        if known.is_none() {
            languages.add_choice(&entry.language);
        }
        languages.choose(known.unwrap_or(ACCENT_LANGUAGES.len()));
    }

    /// The entry of the controls, or None after telling the user what is
    /// wrong with it.
    fn collect(&self, dialog: Window) -> Option<Entry> {
        let mut entry = self.entry.borrow().clone(); // an unknown language code stays
        if !self.accent {
            entry.grapheme = dialog.item(IDC_ENTRY_ORIGINAL_EDIT).text();
            entry.phoneme = dialog.item(IDC_ENTRY_REPLACEMENT_EDIT).text();
            if entry.grapheme.is_empty() || entry.phoneme.is_empty() {
                warn(dialog, &win32::string(IDS_ERROR_INVALID_ENTRY));
                return None;
            }
            entry.comment = dialog.item(IDC_ENTRY_COMMENT_EDIT).text();
            entry.case_sensitive = dialog.checked(IDC_ENTRY_CASE_SENSITIVE);
            entry.whole_word = dialog.checked(IDC_ENTRY_WHOLE_WORD);
            return Some(entry);
        }
        let text = dialog.item(IDC_ACCENT_ENTRY_EDIT);
        entry.grapheme = text.text().trim_matches([' ', '\t', '\r', '\n']).to_owned();
        entry.verb = dialog.item(IDC_ACCENT_KIND_COMBO).choice() == Some(1);
        if let Some(&code) = dialog.item(IDC_ACCENT_LANG_COMBO).choice().and_then(|index| ACCENT_LANGUAGES.get(index)) {
            entry.language = code.to_owned();
        }
        entry.comment = dialog.item(IDC_ACCENT_COMMENT_EDIT).text();
        // The engine's reason quotes the whole entry
        let problem = if entry.grapheme.is_empty() {
            Some(win32::string(IDS_ERROR_ACCENT_EMPTY))
        } else {
            dictionary::accent_error(&entry).map(|reason| insert(&win32::string(IDS_ERROR_ACCENT_INVALID), &reason))
        };
        if let Some(problem) = problem {
            warn(dialog, &problem);
            text.focus();
            return None;
        }
        Some(entry)
    }
}

impl Dialog for EntryEditor {
    fn event(&self, dialog: Window, event: Event) -> Reply {
        match event {
            Event::Init => {
                self.init(dialog);
                if self.dark {
                    dark_mode::apply(dialog, true);
                }
                dialog.center_on_parent();
            }
            Event::Command { id: IDOK, .. } => {
                if let Some(entry) = self.collect(dialog) {
                    self.entry.replace(entry);
                    dialog.end(IDOK);
                }
            }
            Event::Command { id: IDCANCEL, .. } | Event::Close => dialog.end(IDCANCEL),
            Event::Color { of, dc } => {
                let kinds = [ColorOf::Static, ColorOf::Button, ColorOf::Edit, ColorOf::ListBox, ColorOf::Dialog];
                return dark_mode::color(self.dark, &kinds, of, dc);
            }
            _ => return Reply::Unhandled,
        }
        Reply::Handled
    }
}
