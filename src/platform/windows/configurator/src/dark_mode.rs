//! Dark mode of Windows 10 and 11: a dark title bar, the dark theme of the
//! common controls, and dark colours where a dialog paints for its controls.

use crate::win32::{self, Brush, ColorOf, Dc, Reply, Window};
use std::sync::OnceLock;

// Colours as COLORREF (0x00BBGGRR); all three are greys.
pub const BACKGROUND: u32 = 0x0020_2020;
pub const CONTROL: u32 = 0x002D_2D2D;
pub const TEXT: u32 = 0x00FF_FFFF;

/// Whether Windows's apps are set to the dark theme.
pub fn enabled() -> bool {
    win32::apps_use_dark_theme()
}

/// Shows a dialog in the dark or the light theme.
pub fn apply(dialog: Window, dark: bool) {
    dialog.set_dark_title_bar(dark);
    dialog.set_dark_controls(dark);
    dialog.invalidate();
}

/// The answer to a dialog's `Event::Color`: dark colours for the kinds of
/// controls the dialog paints dark, the system's otherwise.
pub fn color(dark: bool, kinds: &[ColorOf], of: ColorOf, dc: Dc) -> Reply {
    static BACKGROUND_BRUSH: OnceLock<Brush> = OnceLock::new();
    static CONTROL_BRUSH: OnceLock<Brush> = OnceLock::new();
    if !dark || !kinds.contains(&of) {
        return Reply::Unhandled;
    }
    Reply::Brush(match of {
        ColorOf::Static | ColorOf::Button => {
            dc.set_colors(TEXT, BACKGROUND);
            *BACKGROUND_BRUSH.get_or_init(|| Brush::solid(BACKGROUND))
        }
        // The list of a combo box is a list box
        ColorOf::Edit | ColorOf::ListBox => {
            dc.set_colors(TEXT, CONTROL);
            *CONTROL_BRUSH.get_or_init(|| Brush::solid(CONTROL))
        }
        ColorOf::Dialog => *BACKGROUND_BRUSH.get_or_init(|| Brush::solid(BACKGROUND)),
    })
}
