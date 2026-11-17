//! `combase.dll` on a system that has none.
//!
//! The `windows` crate takes COM's memory functions and the free-threaded
//! marshaler (`CoTaskMemAlloc`, `CoTaskMemFree`,
//! `CoCreateFreeThreadedMarshaler`) from `combase.dll`, which Windows 8
//! introduced. A program that imports from it is refused by the loader of
//! Windows 7 before any of its code runs. `ole32.dll` has exported all three
//! since COM exists; from Windows 8 on it forwards them to `combase.dll`.
//!
//! The build for Windows 7, 8 and 8.1 (`cargo xtask windows-legacy`)
//! therefore links `combase.dll` as a delay-loaded DLL
//! (`/DELAYLOAD:combase.dll`): the loader no longer asks for it, and the
//! linker's delay-load helper loads it at the first call of one of its
//! functions. Before it does, it asks the hook defined here, which answers
//! with `ole32.dll` instead. The helper then finds the function there.
//!
//! The SAPI5 driver and the Configurator link this crate only when they are
//! built for the Windows 7 targets (`cfg(target_vendor = "win7")`); their
//! code is the same in both builds, and the build for Windows 10 and later
//! does not contain this crate.
#![cfg(windows)]

use std::ffi::{CStr, c_char, c_void};

/// `DelayLoadInfo` of the linker's `delayimp.h`, up to the fields read here.
#[repr(C)]
pub struct DelayLoadInfo {
    size: u32,
    descriptor: *const c_void,
    address: *mut *const c_void,
    /// The name of the DLL about to be loaded.
    dll: *const c_char,
}

/// `dliNotePreLoadLibrary`: the helper is about to load a DLL; a module
/// handle returned by the hook is used in place of loading it.
const BEFORE_LOAD: u32 = 1;

#[link(name = "kernel32")]
unsafe extern "system" {
    fn LoadLibraryA(name: *const c_char) -> *mut c_void;
}

/// Whether the delay-load helper is asking for `combase.dll`.
fn is_combase(dll: &CStr) -> bool {
    dll.to_bytes().eq_ignore_ascii_case(b"combase.dll")
}

/// The delay-load helper's notification hook: `ole32.dll` for `combase.dll`,
/// and null, "do what you would have done", for everything else.
unsafe extern "system" fn redirect(notification: u32, info: *const DelayLoadInfo) -> *mut c_void {
    if notification != BEFORE_LOAD || info.is_null() {
        return std::ptr::null_mut();
    }
    // SAFETY: the helper passes its own `DelayLoadInfo`, whose `dll` is the
    // NUL-terminated name from the delay import table (checked for null).
    let dll = unsafe { (*info).dll };
    if dll.is_null() || !is_combase(unsafe { CStr::from_ptr(dll) }) {
        return std::ptr::null_mut();
    }
    // SAFETY: a NUL-terminated name of a system DLL, which every process
    // that uses COM has loaded already. Null (not found) makes the helper
    // load combase.dll itself.
    unsafe { LoadLibraryA(c"ole32.dll".as_ptr()) }
}

/// The hook the delay-load helper of `delayimp.lib` calls, under the name it
/// looks for; defining it here replaces the library's empty one.
#[unsafe(no_mangle)]
#[allow(non_upper_case_globals)]
pub static __pfnDliNotifyHook2: Option<unsafe extern "system" fn(u32, *const DelayLoadInfo) -> *mut c_void> =
    Some(redirect);

#[cfg(test)]
mod tests {
    use super::*;

    fn ask(notification: u32, dll: &CStr) -> *mut c_void {
        let info = DelayLoadInfo {
            size: size_of::<DelayLoadInfo>() as u32,
            descriptor: std::ptr::null(),
            address: std::ptr::null_mut(),
            dll: dll.as_ptr(),
        };
        // SAFETY: a complete `DelayLoadInfo` with a NUL-terminated name.
        unsafe { redirect(notification, &info) }
    }

    #[test]
    fn combase_is_answered_with_ole32_and_nothing_else_is_touched() {
        assert!(!ask(BEFORE_LOAD, c"combase.dll").is_null());
        assert!(!ask(BEFORE_LOAD, c"COMBASE.DLL").is_null());
        // SAFETY: the same handle LoadLibraryA gives for ole32.dll.
        assert_eq!(ask(BEFORE_LOAD, c"combase.dll"), unsafe { LoadLibraryA(c"ole32.dll".as_ptr()) });
        assert!(ask(BEFORE_LOAD, c"ole32.dll").is_null());
        assert!(ask(BEFORE_LOAD, c"combase.dll.mui").is_null());
        // Other notifications (the function lookup, failures) are left alone.
        assert!(ask(0, c"combase.dll").is_null());
        assert!(ask(2, c"combase.dll").is_null());
        // SAFETY: the hook checks for null.
        assert!(unsafe { redirect(BEFORE_LOAD, std::ptr::null()) }.is_null());
    }
}
