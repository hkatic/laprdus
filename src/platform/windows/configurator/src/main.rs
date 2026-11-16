//! Laprdus Configurator (`laprdgui.exe`): the Windows settings program of
//! the Laprdus text-to-speech engine.
//!
//! It edits `settings.json` and the user's dictionaries in `%APPDATA%\Laprdus`.
//! The dialogs are the templates of `laprdus_config.rc`; all their text comes
//! from its string tables at run time, in the language `ui_language` picks.
//! The manifest of the resources turns on the visual styles and per-monitor
//! DPI awareness.
//!
//! Every Win32 and COM call is made in `win32`; the lint that Cargo.toml sets
//! for the crate keeps it that way, and that module alone lifts it.
#![cfg_attr(all(windows, not(test)), windows_subsystem = "windows")]

#[cfg(windows)]
mod dark_mode;
#[cfg(windows)]
mod dictionary;
#[cfg(windows)]
mod dictionary_dialog;
#[cfg(windows)]
mod settings_dialog;
#[cfg(windows)]
mod text;
#[cfg(windows)]
mod ui_language;
#[cfg(windows)]
mod win32;

/// The ids of `resource.h`, written by the build script.
#[cfg(windows)]
#[allow(dead_code)]
mod ids {
    include!(concat!(env!("OUT_DIR"), "/resource.rs"));
}

#[cfg(windows)]
fn main() -> std::process::ExitCode {
    win32::init_common_controls();
    // Windows's display language, or the language the NVDA add-on asks for
    let arguments = std::env::args_os().skip(1).map(|argument| argument.to_string_lossy().into_owned());
    let language = ui_language::select(arguments);
    // 0 when the dialog was closed with OK
    std::process::ExitCode::from(u8::from(!settings_dialog::run(language.speech)))
}

#[cfg(not(windows))]
fn main() {}
