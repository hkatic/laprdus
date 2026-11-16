//! Every Win32 and COM call of the program, behind safe functions.
//!
//! This is the only module with unsafe code. Windows and device contexts are
//! handed out as plain numbers ([`Window`], [`Dc`], [`Brush`]): Windows checks
//! a handle it is given, so a stale one makes a call fail, never misbehave.
//! Every pointer a call takes is made here, from a buffer that outlives the
//! call.
#![allow(unsafe_code)]

use std::ffi::c_void;
use std::panic::{AssertUnwindSafe, catch_unwind};
use windows::Win32::Foundation::{COLORREF, HWND, LPARAM, RECT, WPARAM};
use windows::Win32::Globalization::{GetUserDefaultUILanguage, SetThreadUILanguage};
use windows::Win32::Graphics::Dwm::{DWMWINDOWATTRIBUTE, DwmSetWindowAttribute};
use windows::Win32::Graphics::Gdi::{CreateSolidBrush, HDC, InvalidateRect, SetBkColor, SetTextColor};
use windows::Win32::Media::Speech::{
    ISpObjectToken, ISpObjectTokenCategory, ISpVoice, SPF_ASYNC, SpObjectTokenCategory, SpVoice,
};
use windows::Win32::System::Com::{
    CLSCTX_ALL, COINIT_APARTMENTTHREADED, CoCreateInstance, CoInitializeEx, CoTaskMemFree, CoUninitialize,
};
use windows::Win32::System::LibraryLoader::GetModuleHandleW;
use windows::Win32::System::Registry::{HKEY_CURRENT_USER, RRF_RT_REG_DWORD, RegGetValueW};
use windows::Win32::UI::Controls::{
    BST_CHECKED, BST_UNCHECKED, CDDS_ITEMPREPAINT, CDDS_PREPAINT, CDRF_NEWFONT, CDRF_NOTIFYITEMDRAW, CheckDlgButton,
    ICC_BAR_CLASSES, ICC_STANDARD_CLASSES, INITCOMMONCONTROLSEX, InitCommonControlsEx, IsDlgButtonChecked,
    LIST_VIEW_ITEM_STATE_FLAGS, LVCF_SUBITEM, LVCF_TEXT, LVCF_WIDTH, LVCOLUMNW, LVIF_TEXT, LVIS_FOCUSED, LVIS_SELECTED,
    LVITEMW, LVM_DELETEALLITEMS, LVM_DELETECOLUMN, LVM_ENSUREVISIBLE, LVM_GETNEXTITEM, LVM_INSERTCOLUMNW,
    LVM_INSERTITEMW, LVM_SETBKCOLOR, LVM_SETEXTENDEDLISTVIEWSTYLE, LVM_SETITEMSTATE, LVM_SETITEMTEXTW,
    LVM_SETTEXTBKCOLOR, LVM_SETTEXTCOLOR, LVN_ITEMCHANGED, LVNI_SELECTED, LVS_EX_DOUBLEBUFFER, LVS_EX_FULLROWSELECT,
    LVS_EX_GRIDLINES, NM_CUSTOMDRAW, NM_DBLCLK, NMHDR, NMLVCUSTOMDRAW, SetWindowTheme, TBM_SETPOS, TBM_SETRANGE,
    TBM_SETTICFREQ,
};
use windows::Win32::UI::Input::KeyboardAndMouse::{EnableWindow, SetFocus};
use windows::Win32::UI::WindowsAndMessaging::{
    CB_ADDSTRING, CB_GETCURSEL, CB_SETCURSEL, DialogBoxParamW, EndDialog, EnumChildWindows, GWLP_USERDATA,
    GetClassNameW, GetDesktopWindow, GetDlgCtrlID, GetDlgItem, GetParent, GetWindowRect, GetWindowTextLengthW,
    GetWindowTextW, LoadStringW, MB_ICONERROR, MB_ICONWARNING, MB_OK, MessageBoxW, SWP_NOSIZE, SWP_NOZORDER,
    SendMessageW, SetWindowPos, SetWindowTextW, WINDOW_LONG_PTR_INDEX, WM_CLOSE, WM_COMMAND, WM_CTLCOLORBTN,
    WM_CTLCOLORDLG, WM_CTLCOLOREDIT, WM_CTLCOLORLISTBOX, WM_CTLCOLORSTATIC, WM_HSCROLL, WM_INITDIALOG, WM_NOTIFY,
    WM_SETTINGCHANGE,
};
#[cfg(target_pointer_width = "64")]
use windows::Win32::UI::WindowsAndMessaging::{
    GetWindowLongPtrW as get_window_long, SetWindowLongPtrW as set_window_long,
};
#[cfg(target_pointer_width = "32")]
use windows::Win32::UI::WindowsAndMessaging::{GetWindowLongW as get_window_long, SetWindowLongW as set_window_long};
use windows::core::{BOOL, PCWSTR, PWSTR, w};

/// The buttons every dialog knows.
pub const IDOK: u16 = 1;
pub const IDCANCEL: u16 = 2;
/// Notification codes of `Event::Command`.
pub const BN_CLICKED: u16 = 0;
pub const CBN_SELCHANGE: u16 = 1;

/// The trackbar's WM_USER, which the bindings leave out.
const TBM_GETPOS: u32 = 0x0400;

/// A window: a dialog or one of its controls.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Window(isize);

/// The device context of a `WM_CTLCOLOR*` message.
#[derive(Clone, Copy, Debug)]
pub struct Dc(isize);

/// A brush that lives as long as the program.
#[derive(Clone, Copy, Debug)]
pub struct Brush(isize);

/// What a dialog is told.
#[derive(Debug)]
pub enum Event {
    /// The dialog exists and is about to be shown.
    Init,
    /// A control's notification, or a command (`id` is then `IDOK`, `IDCANCEL` ...).
    Command { id: u16, code: u16 },
    /// A slider moved.
    Scroll { id: u16 },
    /// A control or the dialog is about to be painted.
    Color { of: ColorOf, dc: Dc },
    /// A system setting changed; the text names it.
    SettingChange(String),
    /// A list view's notification.
    List { id: u16, what: ListEvent },
    /// The close button of the title bar, Alt+F4.
    Close,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ColorOf {
    Static,
    Button,
    Edit,
    ListBox,
    Dialog,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ListEvent {
    ItemChanged,
    DoubleClick,
    /// The list is drawing itself; answer with `Reply::ListColors` to colour it.
    Draw,
}

/// What a dialog answers.
#[derive(Clone, Copy, Debug)]
pub enum Reply {
    Unhandled,
    Handled,
    /// The background of the control being painted.
    Brush(Brush),
    /// The colours of a list view's rows.
    ListColors {
        text: u32,
        back: u32,
    },
}

/// A dialog's behaviour. It is called again while one of its own calls is
/// still running (a control repaints, a nested dialog is open), hence `&self`.
pub trait Dialog {
    fn event(&self, dialog: Window, event: Event) -> Reply;
}

fn wide(text: &str) -> Vec<u16> {
    text.encode_utf16().take_while(|&unit| unit != 0).chain([0]).collect()
}

fn module() -> windows::Win32::Foundation::HINSTANCE {
    // SAFETY: no arguments; the handle of the running program is always there.
    unsafe { GetModuleHandleW(None) }.map(Into::into).unwrap_or_default()
}

/// Takes ownership of a string COM allocated.
///
/// SAFETY: `text` must be null or a NUL-terminated string from `CoTaskMemAlloc`
/// that nothing else frees.
unsafe fn take_string(text: PWSTR) -> String {
    if text.is_null() {
        return String::new();
    }
    // SAFETY: the caller's contract.
    unsafe {
        let string = String::from_utf16_lossy(text.as_wide());
        CoTaskMemFree(Some(text.0 as *const c_void));
        string
    }
}

/// Shows a modal dialog of the resources and returns what it ended with.
pub fn run_dialog(template: u16, parent: Option<Window>, handler: &dyn Dialog) -> isize {
    let state: *const &dyn Dialog = &handler;
    // SAFETY: the template name is a resource id (MAKEINTRESOURCE). `state`
    // points at a local that outlives the call, and the dialog, the only
    // user of the pointer, is destroyed before the call returns.
    unsafe {
        DialogBoxParamW(
            Some(module()),
            PCWSTR(template as usize as *const u16),
            parent.map(Window::hwnd),
            Some(dialog_proc),
            LPARAM(state as isize),
        )
    }
}

/// SAFETY: called by Windows for a dialog made by `run_dialog`.
unsafe extern "system" fn dialog_proc(hwnd: HWND, message: u32, wparam: WPARAM, lparam: LPARAM) -> isize {
    if message == WM_INITDIALOG {
        // SAFETY: a window handle Windows just passed in.
        unsafe { set_window_long(hwnd, GWLP_USERDATA, lparam.0 as _) };
    }
    // SAFETY: as above.
    let state = unsafe { get_window_long(hwnd, GWLP_USERDATA) } as *const &dyn Dialog;
    if state.is_null() {
        return 0; // the messages before WM_INITDIALOG
    }
    // SAFETY: `state` is the pointer `run_dialog` passed, alive until
    // `DialogBoxParamW` returns; the handler is only shared, never mutable.
    let handler: &dyn Dialog = unsafe { *state };

    let event = match message {
        WM_INITDIALOG => Event::Init,
        WM_COMMAND => Event::Command { id: wparam.0 as u16, code: (wparam.0 >> 16) as u16 },
        // SAFETY: the handle of the slider, or null, both fine for the call.
        WM_HSCROLL => Event::Scroll { id: unsafe { GetDlgCtrlID(HWND(lparam.0 as *mut c_void)) } as u16 },
        WM_CTLCOLORSTATIC | WM_CTLCOLORBTN | WM_CTLCOLOREDIT | WM_CTLCOLORLISTBOX | WM_CTLCOLORDLG => Event::Color {
            of: match message {
                WM_CTLCOLORSTATIC => ColorOf::Static,
                WM_CTLCOLORBTN => ColorOf::Button,
                WM_CTLCOLOREDIT => ColorOf::Edit,
                WM_CTLCOLORLISTBOX => ColorOf::ListBox,
                _ => ColorOf::Dialog,
            },
            dc: Dc(wparam.0 as isize),
        },
        WM_SETTINGCHANGE => Event::SettingChange(if lparam.0 == 0 {
            String::new()
        } else {
            // SAFETY: a non-null lParam of WM_SETTINGCHANGE is a NUL-terminated string.
            unsafe { PCWSTR(lparam.0 as *const u16).to_string() }.unwrap_or_default()
        }),
        WM_NOTIFY => {
            // SAFETY: the lParam of WM_NOTIFY points at an NMHDR.
            let header = unsafe { &*(lparam.0 as *const NMHDR) };
            let what = match header.code {
                LVN_ITEMCHANGED => ListEvent::ItemChanged,
                NM_DBLCLK => ListEvent::DoubleClick,
                NM_CUSTOMDRAW => ListEvent::Draw,
                _ => return 0,
            };
            // Other controls send NM_CUSTOMDRAW too, with a smaller structure.
            if Window(header.hwndFrom.0 as isize).class_name() != "SysListView32" {
                return 0;
            }
            Event::List { id: header.idFrom as u16, what }
        }
        WM_CLOSE => Event::Close,
        _ => return 0,
    };
    let is_draw = matches!(event, Event::List { what: ListEvent::Draw, .. });

    // A panic must not cross into Windows; the message then counts as not handled.
    let dialog = Window(hwnd.0 as isize);
    let reply = catch_unwind(AssertUnwindSafe(|| handler.event(dialog, event))).unwrap_or(Reply::Unhandled);
    let result = match reply {
        Reply::Unhandled => 0,
        Reply::Handled => 1,
        Reply::Brush(brush) => brush.0,
        Reply::ListColors { text, back } if is_draw => {
            // SAFETY: NM_CUSTOMDRAW of a list view (checked above) carries an NMLVCUSTOMDRAW.
            let draw = unsafe { &mut *(lparam.0 as *mut NMLVCUSTOMDRAW) };
            let answer = if draw.nmcd.dwDrawStage == CDDS_PREPAINT {
                CDRF_NOTIFYITEMDRAW
            } else if draw.nmcd.dwDrawStage == CDDS_ITEMPREPAINT {
                draw.clrText = COLORREF(text);
                draw.clrTextBk = COLORREF(back);
                CDRF_NEWFONT
            } else {
                return 0;
            };
            // SAFETY: the dialog's own handle; DWLP_MSGRESULT (0) holds a notification's result.
            unsafe { set_window_long(hwnd, WINDOW_LONG_PTR_INDEX(0), answer as _) };
            1
        }
        Reply::ListColors { .. } => 0,
    };
    // TRUE from WM_INITDIALOG lets Windows put the focus on the first control.
    if message == WM_INITDIALOG { 1 } else { result }
}

impl Window {
    fn hwnd(self) -> HWND {
        HWND(self.0 as *mut c_void)
    }

    /// For the messages of this module: their parameters are numbers, or
    /// pointers to buffers and structures that outlive the call.
    fn send(self, message: u32, wparam: usize, lparam: isize) -> isize {
        // SAFETY: see above; a handle that is not a window makes the call return 0.
        unsafe { SendMessageW(self.hwnd(), message, Some(WPARAM(wparam)), Some(LPARAM(lparam))).0 }
    }

    fn class_name(self) -> String {
        let mut buffer = [0u16; 64];
        // SAFETY: the buffer is passed with its length.
        let length = unsafe { GetClassNameW(self.hwnd(), &mut buffer) };
        String::from_utf16_lossy(&buffer[..length.clamp(0, 64) as usize])
    }

    /// A control of a dialog; a window that does nothing when there is none.
    pub fn item(self, id: u16) -> Window {
        // SAFETY: plain values.
        Window(unsafe { GetDlgItem(Some(self.hwnd()), i32::from(id)) }.map_or(0, |control| control.0 as isize))
    }

    pub fn set_text(self, text: &str) {
        let text = wide(text);
        // SAFETY: `text` is NUL-terminated and outlives the call.
        let _ = unsafe { SetWindowTextW(self.hwnd(), PCWSTR(text.as_ptr())) };
    }

    pub fn text(self) -> String {
        // SAFETY: plain values.
        let length = unsafe { GetWindowTextLengthW(self.hwnd()) }.max(0) as usize;
        let mut buffer = vec![0u16; length + 1];
        // SAFETY: the buffer is passed with its length.
        let copied = unsafe { GetWindowTextW(self.hwnd(), &mut buffer) }.clamp(0, length as i32) as usize;
        String::from_utf16_lossy(&buffer[..copied])
    }

    /// Ends a modal dialog with a result.
    pub fn end(self, result: u16) {
        // SAFETY: plain values.
        let _ = unsafe { EndDialog(self.hwnd(), result as isize) };
    }

    pub fn set_checked(self, id: u16, checked: bool) {
        // SAFETY: plain values.
        let _ =
            unsafe { CheckDlgButton(self.hwnd(), i32::from(id), if checked { BST_CHECKED } else { BST_UNCHECKED }) };
    }

    pub fn checked(self, id: u16) -> bool {
        // SAFETY: plain values.
        unsafe { IsDlgButtonChecked(self.hwnd(), i32::from(id)) == BST_CHECKED.0 }
    }

    pub fn enable(self, enabled: bool) {
        // SAFETY: plain values.
        let _ = unsafe { EnableWindow(self.hwnd(), enabled) };
    }

    pub fn focus(self) {
        // SAFETY: plain values.
        let _ = unsafe { SetFocus(Some(self.hwnd())) };
    }

    /// Asks for the whole window to be painted again.
    pub fn invalidate(self) {
        // SAFETY: no rectangle is passed.
        let _ = unsafe { InvalidateRect(Some(self.hwnd()), None, true) };
    }

    /// Centers a dialog on its parent, or on the desktop.
    pub fn center_on_parent(self) {
        let (mut own, mut outer) = (RECT::default(), RECT::default());
        // SAFETY: both rectangles are locals that outlive the calls.
        unsafe {
            let parent = GetParent(self.hwnd()).unwrap_or_else(|_| GetDesktopWindow());
            if GetWindowRect(self.hwnd(), &mut own).is_err() || GetWindowRect(parent, &mut outer).is_err() {
                return;
            }
            let x = outer.left + (outer.right - outer.left - (own.right - own.left)) / 2;
            let y = outer.top + (outer.bottom - outer.top - (own.bottom - own.top)) / 2;
            let _ = SetWindowPos(self.hwnd(), None, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }

    // --- sliders ---

    pub fn set_slider_range(self, min: i32, max: i32) {
        self.send(TBM_SETRANGE, 1, ((max as u16 as isize) << 16) | min as u16 as isize);
    }

    pub fn set_slider_ticks(self, every: i32) {
        self.send(TBM_SETTICFREQ, every as usize, 0);
    }

    pub fn set_slider(self, position: i32) {
        self.send(TBM_SETPOS, 1, position as isize);
    }

    pub fn slider(self) -> i32 {
        self.send(TBM_GETPOS, 0, 0) as i32
    }

    // --- combo boxes ---

    pub fn add_choice(self, text: &str) {
        let text = wide(text);
        self.send(CB_ADDSTRING, 0, text.as_ptr() as isize);
    }

    pub fn choose(self, index: usize) {
        self.send(CB_SETCURSEL, index, 0);
    }

    pub fn choice(self) -> Option<usize> {
        usize::try_from(self.send(CB_GETCURSEL, 0, 0)).ok()
    }

    // --- list views (report mode) ---

    /// Whole-row selection, grid lines, flicker-free drawing.
    pub fn set_list_style(self) {
        let style = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER;
        self.send(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, style as isize);
    }

    /// Replaces the columns: each a title and a width in pixels.
    pub fn set_list_columns(self, columns: &[(String, i32)]) {
        while self.send(LVM_DELETECOLUMN, 0, 0) != 0 {}
        for (index, (title, width)) in columns.iter().enumerate() {
            let mut title = wide(title);
            let column = LVCOLUMNW {
                mask: LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM,
                pszText: PWSTR(title.as_mut_ptr()),
                cx: *width,
                iSubItem: index as i32,
                ..Default::default()
            };
            self.send(LVM_INSERTCOLUMNW, index, &raw const column as isize);
        }
    }

    /// Replaces the rows: each the texts of its columns.
    pub fn set_list_rows(self, rows: &[Vec<String>]) {
        self.send(LVM_DELETEALLITEMS, 0, 0);
        for (row, cells) in rows.iter().enumerate() {
            for (column, cell) in cells.iter().enumerate() {
                let mut text = wide(cell);
                let item = LVITEMW {
                    mask: LVIF_TEXT,
                    iItem: row as i32,
                    iSubItem: column as i32,
                    pszText: PWSTR(text.as_mut_ptr()),
                    ..Default::default()
                };
                let message = if column == 0 { LVM_INSERTITEMW } else { LVM_SETITEMTEXTW };
                self.send(message, row, &raw const item as isize);
            }
        }
    }

    /// Selects a row, gives it the list's focus and scrolls it into view.
    pub fn select_list_row(self, row: usize) {
        let state = LIST_VIEW_ITEM_STATE_FLAGS(LVIS_SELECTED.0 | LVIS_FOCUSED.0);
        let item = LVITEMW { state, stateMask: state, ..Default::default() };
        self.send(LVM_SETITEMSTATE, row, &raw const item as isize);
        self.send(LVM_ENSUREVISIBLE, row, 0);
    }

    pub fn selected_list_row(self) -> Option<usize> {
        usize::try_from(self.send(LVM_GETNEXTITEM, usize::MAX, LVNI_SELECTED as isize)).ok()
    }

    pub fn set_list_colors(self, text: u32, back: u32) {
        self.send(LVM_SETBKCOLOR, 0, back as isize);
        self.send(LVM_SETTEXTBKCOLOR, 0, back as isize);
        self.send(LVM_SETTEXTCOLOR, 0, text as isize);
    }

    // --- dark mode ---

    /// A dark or light title bar.
    pub fn set_dark_title_bar(self, dark: bool) {
        let dark = BOOL::from(dark);
        // The attribute is 20 since Windows 10 2004 and was 19 before it.
        for attribute in [20, 19] {
            // SAFETY: the value is a local BOOL, passed with its size.
            let set = unsafe {
                DwmSetWindowAttribute(self.hwnd(), DWMWINDOWATTRIBUTE(attribute), (&raw const dark).cast(), 4)
            };
            if set.is_ok() {
                break;
            }
        }
    }

    /// The dark or the default theme for every control of a dialog.
    pub fn set_dark_controls(self, dark: bool) {
        /// SAFETY: called by `EnumChildWindows` with a child window.
        unsafe extern "system" fn theme(child: HWND, dark: LPARAM) -> BOOL {
            let name = if dark.0 != 0 { w!("DarkMode_Explorer") } else { PCWSTR::null() };
            // SAFETY: a static string or null, and null for the id list.
            let _ = unsafe { SetWindowTheme(child, name, PCWSTR::null()) };
            true.into()
        }
        // SAFETY: the callback only uses its arguments.
        let _ = unsafe { EnumChildWindows(Some(self.hwnd()), Some(theme), LPARAM(dark as isize)) };
    }
}

impl Dc {
    pub fn set_colors(self, text: u32, back: u32) {
        let dc = HDC(self.0 as *mut c_void);
        // SAFETY: plain values; a handle that is not a device context makes the calls fail.
        unsafe {
            SetTextColor(dc, COLORREF(text));
            SetBkColor(dc, COLORREF(back));
        }
    }
}

impl Brush {
    /// A brush of one colour. Never freed: make it once.
    pub fn solid(color: u32) -> Brush {
        // SAFETY: plain values.
        Brush(unsafe { CreateSolidBrush(COLORREF(color)) }.0 as isize)
    }
}

/// A string of the string table of the thread's UI language, or "" when it is missing.
pub fn string(id: u16) -> String {
    let mut buffer = [0u16; 512];
    // SAFETY: the buffer is passed with its length.
    let length = unsafe { LoadStringW(Some(module()), u32::from(id), PWSTR(buffer.as_mut_ptr()), 512) };
    String::from_utf16_lossy(&buffer[..length.clamp(0, 512) as usize])
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Icon {
    Warning,
    Error,
}

/// A message box with an OK button.
pub fn message_box(owner: Window, text: &str, title: &str, icon: Icon) {
    let (text, title) = (wide(text), wide(title));
    let icon = if icon == Icon::Error { MB_ICONERROR } else { MB_ICONWARNING };
    // SAFETY: both strings are NUL-terminated and outlive the call.
    unsafe { MessageBoxW(Some(owner.hwnd()), PCWSTR(text.as_ptr()), PCWSTR(title.as_ptr()), MB_OK | icon) };
}

/// Registers the control classes the dialogs use.
pub fn init_common_controls() {
    let classes = INITCOMMONCONTROLSEX {
        dwSize: size_of::<INITCOMMONCONTROLSEX>() as u32,
        dwICC: ICC_BAR_CLASSES | ICC_STANDARD_CLASSES,
    };
    // SAFETY: the structure is a local with its size set.
    let _ = unsafe { InitCommonControlsEx(&classes) };
}

/// Whether Windows's apps are set to the dark theme.
pub fn apps_use_dark_theme() -> bool {
    let (mut light, mut size) = (1u32, 4u32);
    // SAFETY: static strings; the value is a local DWORD passed with its size.
    let status = unsafe {
        RegGetValueW(
            HKEY_CURRENT_USER,
            w!("Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
            w!("AppsUseLightTheme"),
            RRF_RT_REG_DWORD,
            None,
            Some((&raw mut light).cast()),
            Some(&raw mut size),
        )
    };
    status.is_ok() && light == 0
}

/// The language id of Windows's display language.
pub fn display_language() -> u16 {
    // SAFETY: no arguments.
    unsafe { GetUserDefaultUILanguage() }
}

/// Makes the resource loader of this thread read the string table of a
/// language, whether or not Windows has that language installed.
pub fn set_thread_language(id: u16) {
    // SAFETY: plain values.
    unsafe { SetThreadUILanguage(id) };
}

/// A SAPI5 voice token: its id (a registry path) and its `Language` value.
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct VoiceToken {
    pub id: String,
    pub language: Option<String>,
}

/// Speaks a text through SAPI5 and waits until it is spoken. `pick` chooses
/// the voice among the installed tokens; without a choice SAPI's default
/// voice speaks.
pub fn speak(text: &str, pick: &dyn Fn(&[VoiceToken]) -> Option<usize>) {
    let text = wide(text);
    // SAFETY: COM is initialized on this thread for as long as the objects
    // live: the closure drops them all before `CoUninitialize`. Every string
    // passed in is NUL-terminated and outlives its call; every string COM
    // returns is taken over by `take_string` exactly once.
    unsafe {
        let initialized = CoInitializeEx(None, COINIT_APARTMENTTHREADED).is_ok();
        let _ = (|| -> windows::core::Result<()> {
            let voice: ISpVoice = CoCreateInstance(&SpVoice, None, CLSCTX_ALL)?;
            let mut tokens: Vec<(VoiceToken, ISpObjectToken)> = Vec::new();
            let category: windows::core::Result<ISpObjectTokenCategory> =
                CoCreateInstance(&SpObjectTokenCategory, None, CLSCTX_ALL);
            if let Ok(category) = category
                && category.SetId(w!("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech\\Voices"), false).is_ok()
                && let Ok(list) = category.EnumTokens(PCWSTR::null(), PCWSTR::null())
            {
                loop {
                    let mut token = None;
                    let _ = list.Next(1, &mut token, None);
                    let Some(token) = token else { break };
                    if let Ok(id) = token.GetId() {
                        let language = token.GetStringValue(w!("Language")).ok().map(|text| take_string(text));
                        tokens.push((VoiceToken { id: take_string(id), language }, token));
                    }
                }
            }
            let described: Vec<VoiceToken> = tokens.iter().map(|(description, _)| description.clone()).collect();
            if let Some((_, token)) = pick(&described).and_then(|index| tokens.get(index)) {
                let _ = voice.SetVoice(token);
            }
            voice.Speak(PCWSTR(text.as_ptr()), SPF_ASYNC.0 as u32, None)?;
            voice.WaitUntilDone(u32::MAX)
        })();
        if initialized {
            CoUninitialize();
        }
    }
}
