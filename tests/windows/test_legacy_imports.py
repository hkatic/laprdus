"""What the build for Windows 7, 8 and 8.1 imports.

`cargo xtask windows-legacy` stages the Windows programs for the systems
Rust's ordinary Windows targets no longer run on (`dist/windows-legacy-x64`,
`dist/windows-legacy-x86`). A program that imports from a DLL the system does
not have is refused by the loader before any of its code runs, so the import
tables say whether these files can load on Windows 7 at all; no machine with
Windows 7 is needed to read them. They must not import from:

- `combase.dll` (Windows 8; the `windows` crate takes COM's memory functions
  and the free-threaded marshaler from it),
- `bcryptprimitives.dll` (`ProcessPrng`, which Rust's standard library uses
  on Windows 10),
- an API set (`api-ms-win-*.dll`; `WaitOnAddress` comes from one, and the
  universal C runtime is a group of them),
- the Visual C++ runtime (`vcruntime*.dll`, `msvcp*.dll`, `ucrtbase.dll`),
  which those systems do not come with: it is linked statically.

Each import must also be a DLL of Windows 7 (`WINDOWS_7`).

`combase.dll` may be there as a delay-loaded DLL, which the loader does not
ask for: the linker's helper loads it at the first call, and the hook of
`src/platform/windows/legacy/compat` answers that with `ole32.dll`. Every
function taken from it that way must be one `ole32.dll` of Windows 7 exports
(`OLE32_OF_WINDOWS_7`), and the file must contain the hook's redirection (the
name `ole32.dll` next to the delay import).

Run: python tests/windows/test_legacy_imports.py [folder ...]
(default: the two legacy folders of dist/; needs `cargo xtask windows-legacy`.
Without arguments it also loads the driver of this Python's architecture and
calls its hook.)
"""

import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))

# The system DLLs of Windows 7 SP1 the programs may import from.
WINDOWS_7 = {
    "advapi32.dll", "comctl32.dll", "dwmapi.dll", "gdi32.dll", "kernel32.dll", "ntdll.dll", "ole32.dll",
    "oleaut32.dll", "shell32.dll", "user32.dll", "userenv.dll", "uxtheme.dll", "ws2_32.dll",
}

# What may be delay-loaded from combase.dll: exports of Windows 7's ole32.dll.
OLE32_OF_WINDOWS_7 = {"CoTaskMemAlloc", "CoTaskMemFree", "CoTaskMemRealloc", "CoCreateFreeThreadedMarshaler"}

FILES = {
    "windows-legacy-x64": ("laprd64.dll", "laprdus.exe", "laprdgui.exe"),
    "windows-legacy-x86": ("laprd32.dll", "laprdus.exe", "laprdgui.exe"),
}


def imports(path):
    """({dll: names}, {dll: names}) of a PE file: what the loader resolves
    when the file is loaded, and what is delay-loaded at the first call."""
    with open(path, "rb") as f:
        data = f.read()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    assert data[pe:pe + 4] == b"PE\0\0", path
    sections = struct.unpack_from("<H", data, pe + 6)[0]
    optional_size = struct.unpack_from("<H", data, pe + 20)[0]
    optional = pe + 24
    plus = struct.unpack_from("<H", data, optional)[0] == 0x20B
    directories = optional + (112 if plus else 96)
    table = optional + optional_size
    layout = [struct.unpack_from("<IIII", data, table + 40 * i + 8) for i in range(sections)]

    def offset(rva):
        for virtual_size, virtual_address, raw_size, raw in layout:
            if virtual_address <= rva < virtual_address + max(virtual_size, raw_size):
                return rva - virtual_address + raw
        raise ValueError("%s: no section holds RVA %#x" % (path, rva))

    def text(rva):
        start = offset(rva)
        return data[start:data.index(b"\0", start)].decode("ascii")

    def names(thunks):
        size, flag, pattern = (8, 1 << 63, "<Q") if plus else (4, 1 << 31, "<I")
        at = offset(thunks)
        found = []
        while True:
            value = struct.unpack_from(pattern, data, at)[0]
            if value == 0:
                return found
            found.append("#%d" % (value & 0xFFFF) if value & flag else text(value + 2))
            at += size

    result, delayed = {}, {}
    # The import directory (entry 1): 20-byte descriptors.
    rva = struct.unpack_from("<I", data, directories + 8 * 1)[0]
    at = offset(rva) if rva else None
    while at is not None:
        lookup, _, _, name, address = struct.unpack_from("<IIIII", data, at)
        if not name:
            break
        result.setdefault(text(name).lower(), []).extend(names(lookup or address))
        at += 20
    # The delay import directory (entry 13): 32-byte descriptors.
    rva = struct.unpack_from("<I", data, directories + 8 * 13)[0]
    at = offset(rva) if rva else None
    while at is not None:
        _, name, _, _, lookup = struct.unpack_from("<IIIII", data, at)
        if not name:
            break
        delayed.setdefault(text(name).lower(), []).extend(names(lookup))
        at += 32
    return result, delayed


def faults(path):
    found = []
    name = os.path.basename(path)
    loaded, delayed = imports(path)
    for dll, functions in sorted(loaded.items()):
        if dll not in WINDOWS_7:
            found.append("%s imports %s from %s, which Windows 7 does not have or does not come with"
                         % (name, ", ".join(functions[:6]), dll))
    for dll, functions in sorted(delayed.items()):
        if dll != "combase.dll":
            found.append("%s delay-loads %s, which nothing redirects" % (name, dll))
            continue
        for function in sorted(set(functions) - OLE32_OF_WINDOWS_7):
            found.append("%s takes %s from combase.dll, which ole32.dll of Windows 7 does not export"
                         % (name, function))
        with open(path, "rb") as f:
            if b"ole32.dll\0" not in f.read():
                found.append("%s delay-loads combase.dll without the hook that answers with ole32.dll" % name)
    return found


def hook_faults(path):
    """Loads the driver and asks its delay-load hook what it does for
    combase.dll: the answer must be the handle of ole32.dll. (The Windows 10
    build of the same driver has no such export.)"""
    import ctypes

    class DelayLoadInfo(ctypes.Structure):
        _fields_ = [("size", ctypes.c_uint32), ("descriptor", ctypes.c_void_p),
                    ("address", ctypes.c_void_p), ("dll", ctypes.c_char_p)]

    kind = ctypes.WINFUNCTYPE(ctypes.c_void_p, ctypes.c_uint32, ctypes.POINTER(DelayLoadInfo))
    library = ctypes.WinDLL(path)
    try:
        hook = kind.in_dll(library, "__pfnDliNotifyHook2")
    except ValueError:
        return ["%s does not define the delay-load hook" % os.path.basename(path)]
    if not ctypes.cast(hook, ctypes.c_void_p).value:
        return ["the delay-load hook of %s is empty" % os.path.basename(path)]
    ole32 = ctypes.WinDLL("ole32.dll")._handle
    found = []
    for dll, expected in ((b"combase.dll", ole32), (b"COMBASE.DLL", ole32), (b"kernel32.dll", None)):
        info = DelayLoadInfo(ctypes.sizeof(DelayLoadInfo), None, None, dll)
        answer = hook(1, ctypes.byref(info))
        if answer != expected:
            found.append("the hook of %s answers %r for %s, not %r"
                         % (os.path.basename(path), answer, dll.decode(), expected))
    return found


def main():
    folders = sys.argv[1:] or [os.path.join(ROOT, "dist", folder) for folder in FILES]
    failures = []
    checked = 0
    for folder in folders:
        names = FILES.get(os.path.basename(os.path.normpath(folder)))
        if names is None:
            names = sorted(n for n in os.listdir(folder) if n.lower().endswith((".dll", ".exe")))
        for name in names:
            path = os.path.join(folder, name)
            if not os.path.exists(path):
                failures.append("%s is missing: run cargo xtask windows-legacy" % path)
                continue
            checked += 1
            found = faults(path)
            failures.extend(found)
            loaded, delayed = imports(path)
            print("%s %s: %s%s" % ("FAIL" if found else "ok  ", os.path.relpath(path, ROOT),
                                   ", ".join(sorted(loaded)),
                                   "".join("; delayed: %s (%s)" % (dll, ", ".join(sorted(set(names))))
                                           for dll, names in sorted(delayed.items()))))
    # The hook itself, in the driver this Python can load.
    bits = "64" if sys.maxsize > 2**32 else "32"
    driver = os.path.join(ROOT, "dist", "windows-legacy-x" + ("64" if bits == "64" else "86"), "laprd%s.dll" % bits)
    if not sys.argv[1:] and os.path.exists(driver):
        found = hook_faults(driver)
        failures.extend(found)
        if not found:
            print("ok   the hook of %s answers combase.dll with ole32.dll" % os.path.relpath(driver, ROOT))
    # The parser must see what is known to be there: the build for Windows 10
    # imports from all three, and a parser that saw nothing would pass above.
    modern = os.path.join(ROOT, "dist", "windows-x64", "laprd64.dll")
    if os.path.exists(modern):
        seen = imports(modern)[0]
        for dll in ("combase.dll", "bcryptprimitives.dll", "api-ms-win-core-synch-l1-2-0.dll"):
            if dll not in seen:
                failures.append("the import reader did not find %s in the Windows 10 build of laprd64.dll" % dll)
        print("ok   the reader finds combase.dll, bcryptprimitives.dll and the API sets in dist/windows-x64/laprd64.dll")
    for failure in failures:
        print("FAIL " + failure)
    print("%d files checked, %d failures" % (checked, len(failures)))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
