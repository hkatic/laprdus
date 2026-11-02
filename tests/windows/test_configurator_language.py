"""Tests of the Laprdus Configurator's languages, against the built laprdgui.exe.

    python tests/windows/test_configurator_language.py [path to laprdgui.exe] [--screenshots DIR]

What is covered:
- the string tables (English, Croatian, Serbian, Slovenian) hold the same
  strings, with the same format specifiers, short enough for the 512-character
  buffer of LoadStringW;
- no two controls of a dialog share an access key in any language;
- --language=<tag> picks the table (NVDA's forms, BCP 47; Bosnian reads
  Croatian, a language without a table English) on a machine with or without
  that language installed;
- all the text of the main dialog, the dictionary dialog and both entry
  dialogs comes from that table and fits its control.

The dialogs run on a desktop of their own, so no window appears on the user's
screen, takes the focus or gets announced by a screen reader. They are closed
with WM_CLOSE (Cancel), so nothing is saved.

Which language a Windows display language gets is tested by
test_ui_language.cpp.
"""
import ctypes
import ctypes.wintypes as w
import os
import re
import struct
import sys
import time
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RESOURCE_H = os.path.join(ROOT, "src", "platform", "windows", "config", "resource.h")

LANGUAGES = {"en": 0x0409, "hr": 0x041A, "sr": 0x241A, "sl": 0x0424}

# (the dialog's strings that can hold an access key)
DIALOG_KEYS = {
    "main": ["VOICE_LABEL", "SPEED_LABEL", "FORCE_SPEED", "ACCELERATION_LABEL", "PITCH_LABEL", "FORCE_PITCH",
             "INFLECTION_LABEL", "VOLUME_LABEL", "FORCE_VOLUME", "VOLUME_BOOST", "COMMA_LABEL", "SENTENCE_LABEL",
             "NEWLINE_LABEL", "SPELLING_SPEED_LABEL", "EMOJI", "DIGITS", "SPELLING_SOUNDS", "USER_DICT_CHECK",
             "DICT_BUTTON", "TEST", "APPLY"],
    "dictionary": ["DICT_TYPE_LABEL", "DICT_ADD", "DICT_EDIT", "DICT_DUPLICATE", "DICT_DELETE", "DICT_CLOSE"],
    "entry": ["ENTRY_ORIGINAL_LABEL", "ENTRY_REPLACEMENT_LABEL", "ENTRY_COMMENT_LABEL", "ENTRY_CASE_SENSITIVE",
              "ENTRY_WHOLE_WORD"],
    "accent": ["ACCENT_ENTRY_LABEL", "ACCENT_KIND_LABEL", "ACCENT_LANG_LABEL", "ENTRY_COMMENT_LABEL"],
}

user32 = ctypes.WinDLL("user32", use_last_error=True)
kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
gdi32 = ctypes.WinDLL("gdi32", use_last_error=True)

for dll, name, res, args in (
    (kernel32, "LoadLibraryExW", w.HMODULE, [w.LPCWSTR, w.HANDLE, w.DWORD]),
    (kernel32, "FindResourceExW", w.HRSRC, [w.HMODULE, w.LPCWSTR, w.LPCWSTR, w.WORD]),
    (kernel32, "LoadResource", w.HGLOBAL, [w.HMODULE, w.HRSRC]),
    (kernel32, "LockResource", ctypes.c_void_p, [w.HGLOBAL]),
    (kernel32, "SizeofResource", w.DWORD, [w.HMODULE, w.HRSRC]),
    (user32, "CreateDesktopW", w.HANDLE, [w.LPCWSTR, w.LPCWSTR, ctypes.c_void_p, w.DWORD, w.DWORD, ctypes.c_void_p]),
    (user32, "SendMessageTimeoutW", ctypes.c_ssize_t, [w.HWND, w.UINT, ctypes.c_size_t, ctypes.c_void_p, w.UINT, w.UINT, ctypes.c_void_p]),
    (user32, "PostMessageW", w.BOOL, [w.HWND, w.UINT, ctypes.c_size_t, ctypes.c_ssize_t]),
    (user32, "GetDC", w.HDC, [w.HWND]),
    (user32, "ReleaseDC", ctypes.c_int, [w.HWND, w.HDC]),
    (user32, "PrintWindow", w.BOOL, [w.HWND, w.HDC, w.UINT]),
    (user32, "GetDlgItem", w.HWND, [w.HWND, ctypes.c_int]),
    (gdi32, "CreateCompatibleDC", w.HDC, [w.HDC]),
    (gdi32, "CreateCompatibleBitmap", w.HBITMAP, [w.HDC, ctypes.c_int, ctypes.c_int]),
    (gdi32, "SelectObject", w.HGDIOBJ, [w.HDC, w.HGDIOBJ]),
    (gdi32, "DeleteObject", w.BOOL, [w.HGDIOBJ]),
    (gdi32, "DeleteDC", w.BOOL, [w.HDC]),
    (gdi32, "CreateFontW", w.HFONT, [ctypes.c_int] * 5 + [w.DWORD] * 8 + [w.LPCWSTR]),
    (gdi32, "GetTextExtentPoint32W", w.BOOL, [w.HDC, w.LPCWSTR, ctypes.c_int, ctypes.POINTER(w.SIZE)]),
    (gdi32, "GetDIBits", ctypes.c_int, [w.HDC, w.HBITMAP, w.UINT, w.UINT, ctypes.c_void_p, ctypes.c_void_p, w.UINT]),
):
    fn = getattr(dll, name)
    fn.restype = res
    fn.argtypes = args

WM_GETTEXT, WM_CLOSE, WM_COMMAND = 0x000D, 0x0010, 0x0111
CB_GETCOUNT, CB_GETLBTEXT = 0x0146, 0x0148
EnumProc = ctypes.WINFUNCTYPE(w.BOOL, w.HWND, w.LPARAM)

failures = []


def check(name, ok, detail=""):
    print("%s %s%s" % ("ok  " if ok else "FAIL", name, (": %s" % detail) if detail and not ok else ""))
    if not ok:
        failures.append(name)


# --- string tables ---------------------------------------------------------

def string_ids():
    ids = {}
    with open(RESOURCE_H, encoding="utf-8") as f:
        for match in re.finditer(r"^#define\s+IDS_(\w+)\s+(\d+)", f.read(), re.M):
            ids[match.group(1)] = int(match.group(2))
    return ids


def read_table(module, lang, ids):
    """The strings of one language, read from the resources themselves, so a
    language missing from a table is not hidden by the loader's fallback."""
    bundles = {}
    table = {}
    for name, number in ids.items():
        block = (number >> 4) + 1
        if block not in bundles:
            res = kernel32.FindResourceExW(module, ctypes.cast(6, w.LPCWSTR), ctypes.cast(block, w.LPCWSTR), lang)
            if not res:
                bundles[block] = None
            else:
                ptr = kernel32.LockResource(kernel32.LoadResource(module, res))
                bundles[block] = ctypes.string_at(ptr, kernel32.SizeofResource(module, res))
        data = bundles[block]
        if data is None:
            continue
        pos = 0
        text = None
        for index in range(16):
            length = struct.unpack_from("<H", data, pos)[0]
            pos += 2
            if index == number & 15 and length:
                text = data[pos:pos + 2 * length].decode("utf-16-le")
            pos += 2 * length
        if text is not None:
            table[name] = text
    return table


def access_key(text):
    match = re.search(r"&(.)", text.replace("&&", ""))
    return match.group(1).lower() if match else None


def test_tables(exe):
    module = kernel32.LoadLibraryExW(exe, None, 0x2 | 0x20)  # data file, image resource
    if not module:
        raise OSError("cannot read %s" % exe)
    ids = string_ids()
    tables = {code: read_table(module, lang, ids) for code, lang in LANGUAGES.items()}
    for code, table in tables.items():
        missing = sorted(set(ids) - set(table))
        check("%s has every string" % code, not missing, missing)
        too_long = [n for n, t in table.items() if len(t) >= 512]
        check("%s fits LoadStringW's buffer" % code, not too_long, too_long)
        specifiers = [n for n, t in table.items()
                      if re.findall(r"%[.\d]*[a-z%]", t) != re.findall(r"%[.\d]*[a-z%]", tables["en"][n])]
        check("%s has the format specifiers of English" % code, not specifiers, specifiers)
        for dialog, keys in DIALOG_KEYS.items():
            letters = [access_key(table[k]) for k in keys if access_key(table[k])]
            repeated = sorted(set(k for k in letters if letters.count(k) > 1))
            check("%s %s dialog: access keys are unique" % (code, dialog), not repeated, repeated)
            ascii_only = [k for k in letters if ord(k) > 127]
            check("%s %s dialog: access keys are on every keyboard" % (code, dialog), not ascii_only, ascii_only)
    for code in ("hr", "sr", "sl"):
        same = [n for n, t in tables[code].items()
                if t == tables["en"][n] and n not in ("VALUE_SPEED", "VALUE_MS", "VALUE_PERCENT", "TEST", "DICT_ADD",
                                                       "TEST_TEXT", "ACCENT_LANG_ALL")
                and not re.fullmatch(r"[\W\d%a-z]*", t)]
        check("%s is translated" % code, not same, same)
    return tables


# --- dialogs on a desktop of their own --------------------------------------

class STARTUPINFO(ctypes.Structure):
    _fields_ = [("cb", w.DWORD), ("lpReserved", w.LPWSTR), ("lpDesktop", w.LPWSTR), ("lpTitle", w.LPWSTR),
                ("dwX", w.DWORD), ("dwY", w.DWORD), ("dwXSize", w.DWORD), ("dwYSize", w.DWORD),
                ("dwXCountChars", w.DWORD), ("dwYCountChars", w.DWORD), ("dwFillAttribute", w.DWORD),
                ("dwFlags", w.DWORD), ("wShowWindow", w.WORD), ("cbReserved2", w.WORD),
                ("lpReserved2", ctypes.c_void_p), ("hStdInput", w.HANDLE), ("hStdOutput", w.HANDLE),
                ("hStdError", w.HANDLE)]


class PROCESS_INFORMATION(ctypes.Structure):
    _fields_ = [("hProcess", w.HANDLE), ("hThread", w.HANDLE), ("dwProcessId", w.DWORD), ("dwThreadId", w.DWORD)]


def get_text(hwnd):
    buffer = ctypes.create_unicode_buffer(2048)
    result = ctypes.c_size_t()
    user32.SendMessageTimeoutW(hwnd, WM_GETTEXT, 2048, buffer, 2, 3000, ctypes.byref(result))
    return buffer.value


def class_name(hwnd):
    buffer = ctypes.create_unicode_buffer(64)
    user32.GetClassNameW(hwnd, buffer, 64)
    return buffer.value


def children(dialog):
    found = []

    def collect(hwnd, _):
        found.append(hwnd)
        return True
    user32.EnumChildWindows(dialog, EnumProc(collect), 0)
    return found


def dialogs_of(desktop, pid):
    found = []

    def collect(hwnd, _):
        owner = w.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
        if owner.value == pid and class_name(hwnd) == "#32770" and user32.IsWindowVisible(hwnd):
            found.append(hwnd)
        return True
    user32.EnumDesktopWindows(desktop, EnumProc(collect), 0)
    return found


def wait_for_new_dialog(desktop, pid, known):
    for _ in range(100):
        new = [d for d in dialogs_of(desktop, pid) if d not in known]
        if new:
            return new[0]
        time.sleep(0.05)
    return None


def measure_font(dpi):
    return gdi32.CreateFontW(-((8 * dpi + 36) // 72), 0, 0, 0, 400, 0, 0, 0, 1, 0, 0, 0, 0, "MS Shell Dlg 2")


def text_width(dialog, text):
    dpi = user32.GetDpiForWindow(dialog) or 96
    screen = user32.GetDC(None)
    dc = gdi32.CreateCompatibleDC(screen)
    font = measure_font(dpi)
    gdi32.SelectObject(dc, font)
    size = w.SIZE()
    gdi32.GetTextExtentPoint32W(dc, text, len(text), ctypes.byref(size))
    gdi32.DeleteObject(font)
    gdi32.DeleteDC(dc)
    user32.ReleaseDC(None, screen)
    return size.cx


def client_width(hwnd):
    rect = w.RECT()
    user32.GetClientRect(hwnd, ctypes.byref(rect))
    return rect.right


def overflowing(dialog):
    """The controls whose text does not fit them."""
    dpi = user32.GetDpiForWindow(dialog) or 96
    screen = user32.GetDC(None)
    dc = gdi32.CreateCompatibleDC(screen)
    font = measure_font(dpi)
    gdi32.SelectObject(dc, font)
    problems = []
    for hwnd in children(dialog):
        kind = class_name(hwnd)
        if kind not in ("Static", "Button"):
            continue
        text = get_text(hwnd).replace("&", "")
        if not text:
            continue
        rect = w.RECT()
        user32.GetClientRect(hwnd, ctypes.byref(rect))
        width, height = rect.right, rect.bottom
        style = user32.GetWindowLongW(hwnd, -16) & 0xF
        if kind == "Button" and style == 7:  # group box
            margin = 16 * dpi // 96
        elif kind == "Button" and style in (2, 3):  # check box
            margin = 18 * dpi // 96
        elif kind == "Button":
            margin = 8 * dpi // 96
        else:
            margin = 0
        size = w.SIZE()
        gdi32.GetTextExtentPoint32W(dc, text, len(text), ctypes.byref(size))
        lines = max(1, height // max(1, size.cy))
        # A wrapped text needs some slack: lines break between words
        if size.cx + margin > (width - margin) * lines * (0.9 if lines > 1 else 1.0) + margin:
            problems.append((text[:50], size.cx + margin, width))
    gdi32.DeleteObject(font)
    gdi32.DeleteDC(dc)
    user32.ReleaseDC(None, screen)
    return problems


def screenshot(dialog, path):
    rect = w.RECT()
    user32.GetWindowRect(dialog, ctypes.byref(rect))
    width, height = rect.right - rect.left, rect.bottom - rect.top
    screen = user32.GetDC(None)
    dc = gdi32.CreateCompatibleDC(screen)
    bitmap = gdi32.CreateCompatibleBitmap(screen, width, height)
    gdi32.SelectObject(dc, bitmap)
    user32.PrintWindow(dialog, dc, 0)
    header = struct.pack("<IiiHHIIiiII", 40, width, -height, 1, 32, 0, 0, 0, 0, 0, 0)
    pixels = ctypes.create_string_buffer(width * height * 4)
    gdi32.GetDIBits(dc, bitmap, 0, height, pixels, header, 0)
    gdi32.DeleteObject(bitmap)
    gdi32.DeleteDC(dc)
    user32.ReleaseDC(None, screen)
    rows = bytearray()
    for y in range(height):
        rows.append(0)
        row = pixels.raw[y * width * 4:(y + 1) * width * 4]
        rows += bytes(b for x in range(width) for b in (row[x * 4 + 2], row[x * 4 + 1], row[x * 4]))

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(bytes(rows))) + chunk(b"IEND", b""))


class Configurator:
    def __init__(self, exe, desktop_name, desktop, language):
        self.desktop = desktop
        startup = STARTUPINFO()
        startup.cb = ctypes.sizeof(STARTUPINFO)
        startup.lpDesktop = desktop_name
        info = PROCESS_INFORMATION()
        option = " --language=%s" % language if language else ""
        command = ctypes.create_unicode_buffer('"%s"%s' % (exe, option))
        if not kernel32.CreateProcessW(None, command, None, None, False, 0, None, None,
                                       ctypes.byref(startup), ctypes.byref(info)):
            raise ctypes.WinError(ctypes.get_last_error())
        self.info = info
        self.pid = info.dwProcessId
        self.main = wait_for_new_dialog(desktop, self.pid, [])

    def open(self, parent, command_id):
        """Presses a button of a dialog (without activating it) and returns the dialog it opens."""
        known = dialogs_of(self.desktop, self.pid)
        user32.PostMessageW(parent, WM_COMMAND, command_id, 0)
        return wait_for_new_dialog(self.desktop, self.pid, known)

    def close(self, dialog):
        user32.PostMessageW(dialog, WM_CLOSE, 0, 0)
        for _ in range(100):
            if dialog not in dialogs_of(self.desktop, self.pid):
                return
            time.sleep(0.05)

    def stop(self):
        self.close(self.main)
        if kernel32.WaitForSingleObject(self.info.hProcess, 5000) != 0:
            kernel32.TerminateProcess(self.info.hProcess, 1)
        kernel32.CloseHandle(self.info.hProcess)
        kernel32.CloseHandle(self.info.hThread)


def texts(dialog):
    return {user32.GetDlgCtrlID(h): get_text(h) for h in children(dialog)}


def check_dialog(name, dialog, table, expected, shots):
    if not dialog:
        check("%s opens" % name, False)
        return
    wanted = dict(expected)
    title = wanted.pop("TITLE", None)
    if title is not None:
        check("%s: title" % name, get_text(dialog) == table[title], get_text(dialog))
    shown = "\n".join(texts(dialog).values())
    missing = [k for k in wanted.values() if table[k] not in shown]
    check("%s: text of its language" % name, not missing, missing)
    problems = overflowing(dialog)
    check("%s: text fits" % name, not problems, problems)
    if shots:
        screenshot(dialog, os.path.join(shots, name.replace(" ", "_") + ".png"))


def test_dialogs(exe, tables, shots):
    IDC_DICT_BUTTON, IDC_DICT_ADD, IDC_DICT_TYPE_COMBO = 1096, 2004, 2002
    name = "laprdus_test_%d" % os.getpid()
    desktop = user32.CreateDesktopW(name, None, None, 0, 0x10000000, None)  # GENERIC_ALL
    if not desktop:
        raise ctypes.WinError(ctypes.get_last_error())
    if not user32.SetThreadDesktop(desktop):
        raise ctypes.WinError(ctypes.get_last_error())

    main_expected = {"TITLE": "APP_TITLE", "a": "GROUP_VOICE", "b": "VOICE_LABEL", "c": "SPEED_LABEL", "d": "FORCE_SPEED",
                     "e": "ACCELERATION_LABEL", "f": "PITCH_LABEL", "g": "INFLECTION_LABEL", "h": "VOLUME_LABEL",
                     "i": "VOLUME_BOOST", "j": "COMMA_LABEL", "k": "SENTENCE_LABEL", "l": "NEWLINE_LABEL",
                     "m": "SPELLING_SPEED_LABEL", "n": "GROUP_OPTIONS", "o": "EMOJI", "p": "DIGITS",
                     "q": "SPELLING_SOUNDS", "r": "GROUP_DICTIONARIES", "s": "USER_DICT_CHECK", "t": "DICT_BUTTON",
                     "u": "TEST", "v": "OK", "w": "CANCEL", "x": "APPLY"}
    # Without the option: Windows's display language (the machine's, so only the
    # ones with a table of their own can be told from here)
    display = {0x0409: "en", 0x041A: "hr", 0x241A: "sr", 0x0424: "sl"}.get(kernel32.GetUserDefaultUILanguage())
    if display:
        app = Configurator(exe, name, desktop, None)
        check("without --language it reads Windows's display language (%s)" % display,
              app.main and get_text(app.main) == tables[display]["APP_TITLE"], app.main and get_text(app.main))
        app.stop()

    for tag, table_code in (("en", "en"), ("hr", "hr"), ("sr", "sr"), ("sl", "sl"), ("hr_HR", "hr"), ("sr_Latn", "sr"),
                            ("sr-Cyrl-RS", "sr"), ("sl_SI", "sl"), ("bs", "hr"), ("de", "en"), ("pt_BR", "en")):
        table = tables[table_code]
        app = Configurator(exe, name, desktop, tag)
        if not app.main:
            check("--language=%s starts" % tag, False)
            continue
        check("--language=%s reads %s" % (tag, table_code), get_text(app.main) == table["APP_TITLE"], get_text(app.main))
        if tag == table_code:
            check_dialog("%s main" % tag, app.main, table, main_expected, shots)
            voice_combo = user32.GetDlgItem(app.main, 1011)
            count = user32.SendMessageW(voice_combo, CB_GETCOUNT, 0, 0)
            check("%s lists all voices" % tag, count == 18, count)
            buffer = ctypes.create_unicode_buffer(256)
            user32.SendMessageTimeoutW(voice_combo, CB_GETLBTEXT, 5, buffer, 2, 3000, None)
            check("%s Zvonko is named with its kind" % tag,
                  buffer.value == "Zvonko (%s, %s)" % (table["VOICE_KIND_FORMANT"], table["VOICE_DEFAULT"]), buffer.value)
            value = get_text(user32.GetDlgItem(app.main, 1077))
            check("%s inflection value is written the language's way" % tag,
                  re.fullmatch(r"\d+" + (" " if tag in ("hr", "sl") else "") + "%", value) is not None, value)
            value = get_text(user32.GetDlgItem(app.main, 1022))
            check("%s rate value has its decimal separator" % tag,
                  re.fullmatch(r"\d+" + re.escape(table["DECIMAL_SEPARATOR"]) + r"\d+x", value) is not None, value)

            # The widest value the acceleration label can show: 3.0x, the top rate capped at 8x
            widest = table["VALUE_WPM"].replace("%.1f", "3" + table["DECIMAL_SEPARATOR"] + "0").replace("%d", "1400")
            value_label = user32.GetDlgItem(app.main, 1084)
            check("%s the widest acceleration value fits" % tag, text_width(app.main, widest) <= client_width(value_label),
                  (widest, text_width(app.main, widest), client_width(value_label)))

            dictionaries = app.open(app.main, IDC_DICT_BUTTON)
            check_dialog("%s dictionaries" % tag, dictionaries, table,
                         {"TITLE": "DICT_DIALOG_TITLE", "a": "DICT_TYPE_LABEL", "b": "DICT_ADD", "c": "DICT_EDIT",
                          "d": "DICT_DUPLICATE", "e": "DICT_DELETE", "f": "DICT_CLOSE"}, shots)
            if dictionaries:
                entry = app.open(dictionaries, IDC_DICT_ADD)
                check_dialog("%s entry" % tag, entry, table,
                             {"TITLE": "ENTRY_DIALOG_TITLE_ADD", "a": "ENTRY_ORIGINAL_LABEL", "b": "ENTRY_REPLACEMENT_LABEL",
                              "c": "ENTRY_COMMENT_LABEL", "d": "ENTRY_CASE_SENSITIVE", "e": "ENTRY_WHOLE_WORD",
                              "f": "OK", "g": "CANCEL"}, shots)
                if entry:
                    app.close(entry)
                # The accent dictionary is the fourth type of the combo
                combo = user32.GetDlgItem(dictionaries, IDC_DICT_TYPE_COMBO)
                user32.SendMessageW(combo, 0x014E, 3, 0)  # CB_SETCURSEL
                user32.PostMessageW(dictionaries, WM_COMMAND, IDC_DICT_TYPE_COMBO | (1 << 16), combo)  # CBN_SELCHANGE
                time.sleep(0.2)
                accent = app.open(dictionaries, IDC_DICT_ADD)
                check_dialog("%s accent entry" % tag, accent, table,
                             {"TITLE": "ACCENT_DIALOG_TITLE_ADD", "a": "ACCENT_ENTRY_LABEL", "b": "ACCENT_KIND_LABEL",
                              "c": "ACCENT_LANG_LABEL", "d": "ENTRY_COMMENT_LABEL", "e": "ACCENT_HELP",
                              "f": "OK", "g": "CANCEL"}, shots)
                if accent:
                    app.close(accent)
                app.close(dictionaries)
        app.stop()
    user32.CloseDesktop(desktop)


if __name__ == "__main__":
    args = sys.argv[1:]
    shots = None
    if "--screenshots" in args:
        i = args.index("--screenshots")
        shots = args[i + 1]
        del args[i:i + 2]
        os.makedirs(shots, exist_ok=True)
    exe = os.path.abspath(args[0] if args else os.path.join(ROOT, "build", "windows-x64-release", "laprdgui.exe"))
    user32.SetProcessDpiAwarenessContext(ctypes.c_void_p(-4))
    test_dialogs(exe, test_tables(exe), shots)
    print("%d failed" % len(failures))
    sys.exit(1 if failures else 0)
