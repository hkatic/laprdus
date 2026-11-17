"""Drives the SAPI5 driver (laprd64.dll / laprd32.dll) through COM, unregistered.

The class factory is taken straight from the library (DllGetClassObject), so
nothing is registered and the installed driver, if any, is not involved. The
engine gets a voice token made for the test under HKEY_CURRENT_USER (removed
afterwards) and a site implemented here that collects the audio: nothing is
played. APPDATA points at an empty folder, so the user's settings.json is
neither read nor written (the engine writes a default one into that folder).

    cargo xtask windows
    python tests/windows/test_sapi5_driver.py [path to the DLL]

Needs comtypes (pip install comtypes) and a Python of the DLL's bitness.
"""

import ctypes
import os
import pathlib
import struct
import sys
import tempfile
from ctypes import POINTER, Structure, byref, c_int, c_int16, c_long, c_ulong, c_ulonglong, c_ushort, c_void_p, c_wchar_p

ROOT = pathlib.Path(__file__).resolve().parents[2]
BITS = struct.calcsize("P") * 8
DLL = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "dist" / ("windows-x64" if BITS == 64 else "windows-x86") / f"laprd{BITS}.dll"

SCRATCH = tempfile.mkdtemp(prefix="laprdus_sapi_test_")
os.environ["APPDATA"] = SCRATCH

import comtypes  # noqa: E402
from comtypes import COMMETHOD, GUID, HRESULT, STDMETHOD, COMObject, IUnknown  # noqa: E402

TOKEN_ID = "HKEY_CURRENT_USER\\Software\\LaprdusDriverTest\\Token"
CLSID_LAPRDUS = GUID("{D4B5A4E1-8B5C-4C5D-9E6F-1A2B3C4D5E6F}")
CLSID_SP_OBJECT_TOKEN = GUID("{EF411752-3736-4CB4-9C8C-8EF4CCB58EFE}")
SPDFID_WAVE_FORMAT_EX = GUID("{C31ADBAE-527F-4FF5-A230-F62BB61FF70C}")
SPVA_SPEAK, SPVA_SILENCE, SPVA_SPELL_OUT = 0, 1, 4
SPVES_ABORT = 1


class WAVEFORMATEX(Structure):
    _pack_ = 1
    _fields_ = [("wFormatTag", c_ushort), ("nChannels", c_ushort), ("nSamplesPerSec", c_ulong),
                ("nAvgBytesPerSec", c_ulong), ("nBlockAlign", c_ushort), ("wBitsPerSample", c_ushort),
                ("cbSize", c_ushort)]


class SPVPITCH(Structure):
    _fields_ = [("MiddleAdj", c_long), ("RangeAdj", c_long)]


class SPVCONTEXT(Structure):
    _fields_ = [("pCategory", c_wchar_p), ("pBefore", c_wchar_p), ("pAfter", c_wchar_p)]


class SPVSTATE(Structure):
    _fields_ = [("eAction", c_int), ("LangID", c_ushort), ("wReserved", c_ushort), ("EmphAdj", c_long),
                ("RateAdj", c_long), ("Volume", c_ulong), ("PitchAdj", SPVPITCH), ("SilenceMSecs", c_ulong),
                ("pPhoneIds", c_void_p), ("ePartOfSpeech", c_int), ("Context", SPVCONTEXT)]


class SPVTEXTFRAG(Structure):
    pass


SPVTEXTFRAG._fields_ = [("pNext", POINTER(SPVTEXTFRAG)), ("State", SPVSTATE), ("pTextStart", c_wchar_p),
                        ("ulTextLen", c_ulong), ("ulTextSrcOffset", c_ulong)]


class IClassFactory(IUnknown):
    _iid_ = GUID("{00000001-0000-0000-C000-000000000046}")
    _methods_ = [
        STDMETHOD(HRESULT, "CreateInstance", [c_void_p, POINTER(GUID), POINTER(c_void_p)]),
        STDMETHOD(HRESULT, "LockServer", [c_int]),
    ]


class ISpDataKey(IUnknown):
    _iid_ = GUID("{14056581-E16C-11D2-BB90-00C04F8EE6C0}")
    _methods_ = [
        STDMETHOD(HRESULT, "SetData", [c_wchar_p, c_ulong, c_void_p]),
        STDMETHOD(HRESULT, "GetData", [c_wchar_p, POINTER(c_ulong), c_void_p]),
        STDMETHOD(HRESULT, "SetStringValue", [c_wchar_p, c_wchar_p]),
        STDMETHOD(HRESULT, "GetStringValue", [c_wchar_p, POINTER(c_void_p)]),
        STDMETHOD(HRESULT, "SetDWORD", [c_wchar_p, c_ulong]),
        STDMETHOD(HRESULT, "GetDWORD", [c_wchar_p, POINTER(c_ulong)]),
        STDMETHOD(HRESULT, "OpenKey", [c_wchar_p, POINTER(c_void_p)]),
        STDMETHOD(HRESULT, "CreateKey", [c_wchar_p, POINTER(c_void_p)]),
        STDMETHOD(HRESULT, "DeleteKey", [c_wchar_p]),
        STDMETHOD(HRESULT, "DeleteValue", [c_wchar_p]),
        STDMETHOD(HRESULT, "EnumKeys", [c_ulong, POINTER(c_void_p)]),
        STDMETHOD(HRESULT, "EnumValues", [c_ulong, POINTER(c_void_p)]),
    ]


class ISpObjectToken(ISpDataKey):
    _iid_ = GUID("{14056589-E16C-11D2-BB90-00C04F8EE6C0}")
    _methods_ = [
        STDMETHOD(HRESULT, "SetId", [c_wchar_p, c_wchar_p, c_int]),
        STDMETHOD(HRESULT, "GetId", [POINTER(c_void_p)]),
        STDMETHOD(HRESULT, "GetCategory", [POINTER(c_void_p)]),
        STDMETHOD(HRESULT, "CreateInstance", [c_void_p, c_ulong, POINTER(GUID), POINTER(c_void_p)]),
        STDMETHOD(HRESULT, "GetStorageFileName", [POINTER(GUID), c_wchar_p, c_wchar_p, c_ulong, POINTER(c_void_p)]),
        STDMETHOD(HRESULT, "RemoveStorageFileName", [POINTER(GUID), c_wchar_p, c_int]),
        STDMETHOD(HRESULT, "Remove", [POINTER(GUID)]),
    ]


class ISpObjectWithToken(IUnknown):
    _iid_ = GUID("{5B559F40-E952-11D2-BB91-00C04F8EE6C0}")
    _methods_ = [
        STDMETHOD(HRESULT, "SetObjectToken", [POINTER(ISpObjectToken)]),
        STDMETHOD(HRESULT, "GetObjectToken", [POINTER(POINTER(ISpObjectToken))]),
    ]


class ISpEventSink(IUnknown):
    _iid_ = GUID("{BE7A9CC9-5F9E-11D2-960F-00C04F8EE628}")
    _methods_ = [
        STDMETHOD(HRESULT, "AddEvents", [c_void_p, c_ulong]),
        STDMETHOD(HRESULT, "GetEventInterest", [POINTER(c_ulonglong)]),
    ]


class ISpTTSEngineSite(ISpEventSink):
    _iid_ = GUID("{9880499B-CCE9-11D2-B503-00C04F797396}")
    _methods_ = [
        STDMETHOD(c_ulong, "GetActions", []),
        STDMETHOD(HRESULT, "Write", [c_void_p, c_ulong, POINTER(c_ulong)]),
        STDMETHOD(HRESULT, "GetRate", [POINTER(c_long)]),
        STDMETHOD(HRESULT, "GetVolume", [POINTER(c_ushort)]),
        STDMETHOD(HRESULT, "GetSkipInfo", [POINTER(c_int), POINTER(c_long)]),
        STDMETHOD(HRESULT, "CompleteSkip", [c_long]),
    ]


class ISpTTSEngine(IUnknown):
    _iid_ = GUID("{A74D7C8E-4CC5-4F2F-A6EB-804DEE18500E}")
    _methods_ = [
        STDMETHOD(HRESULT, "Speak", [c_ulong, POINTER(GUID), POINTER(WAVEFORMATEX), POINTER(SPVTEXTFRAG), POINTER(ISpTTSEngineSite)]),
        STDMETHOD(HRESULT, "GetOutputFormat", [POINTER(GUID), POINTER(WAVEFORMATEX), POINTER(GUID), POINTER(POINTER(WAVEFORMATEX))]),
    ]


class Site(COMObject):
    """Collects what the driver writes; can ask it to stop after some bytes."""

    _com_interfaces_ = [ISpTTSEngineSite]

    def __init__(self, rate=0, volume=100, abort_after=None):
        super().__init__()
        self.rate, self.volume, self.abort_after = rate, volume, abort_after
        self.audio = bytearray()
        self.writes = []

    def AddEvents(self, this, events, count):
        return 0

    def GetEventInterest(self, this, interest):
        interest[0] = 0
        return 0

    def GetActions(self, this):
        return SPVES_ABORT if self.abort_after is not None and len(self.audio) >= self.abort_after else 0

    def Write(self, this, buffer, size, written):
        self.audio += ctypes.string_at(buffer, size)
        self.writes.append(size)
        if written:
            written[0] = size
        return 0

    def GetRate(self, this, rate):
        rate[0] = self.rate
        return 0

    def GetVolume(self, this, volume):
        volume[0] = self.volume
        return 0

    def GetSkipInfo(self, this, kind, count):
        kind[0], count[0] = 0, 0
        return 0

    def CompleteSkip(self, this, skipped):
        return 0


def fragments(*parts):
    """A fragment list from (action, text or milliseconds, pitch) tuples."""
    frags = [SPVTEXTFRAG() for _ in parts]
    for index, (frag, (action, value, pitch)) in enumerate(zip(frags, parts)):
        frag.State.eAction = action
        frag.State.PitchAdj.MiddleAdj = pitch
        if action == SPVA_SILENCE:
            frag.State.SilenceMSecs = value
        else:
            frag.pTextStart, frag.ulTextLen = value, len(value)
        if index + 1 < len(frags):
            frag.pNext = ctypes.pointer(frags[index + 1])
    return frags


class Reference:
    """The same library's C API, for the audio the driver should produce."""

    def __init__(self, lib):
        self.lib = lib
        lib.laprdus_create.restype = c_void_p
        lib.laprdus_set_voice.argtypes = [c_void_p, ctypes.c_char_p, ctypes.c_char_p]
        lib.laprdus_set_language.argtypes = [c_void_p, ctypes.c_char_p]
        for name in ("synthesize", "synthesize_spelled"):
            getattr(lib, "laprdus_" + name).argtypes = [c_void_p, ctypes.c_char_p, POINTER(POINTER(c_int16)), c_void_p]
        for name in ("speed", "user_pitch", "volume"):
            getattr(lib, "laprdus_set_" + name).argtypes = [c_void_p, ctypes.c_float]
        lib.laprdus_free_buffer.argtypes = [POINTER(c_int16)]
        lib.laprdus_destroy.argtypes = [c_void_p]

    def render(self, language, text, spelled=False, speed=1.0, pitch=1.0, volume=1.0):
        lib, handle = self.lib, self.lib.laprdus_create()
        assert lib.laprdus_set_voice(handle, b"zvonko", b"") == 0
        assert lib.laprdus_set_language(handle, language.encode()) == 0
        lib.laprdus_set_speed(handle, speed)
        lib.laprdus_set_user_pitch(handle, pitch)
        lib.laprdus_set_volume(handle, volume)
        samples, fmt = POINTER(c_int16)(), (ctypes.c_uint32 * 2)()
        call = lib.laprdus_synthesize_spelled if spelled else lib.laprdus_synthesize
        count = call(handle, text.encode("utf-8"), byref(samples), fmt)
        assert count > 0, (text, count)
        audio = ctypes.string_at(samples, count * 2)
        lib.laprdus_free_buffer(samples)
        lib.laprdus_destroy(handle)
        return audio


failed = 0


def check(name, condition, detail=""):
    global failed
    print(("ok   " if condition else "FAIL ") + name + (f": {detail}" if detail and not condition else ""))
    failed += not condition


def main():
    comtypes.CoInitialize()
    lib = ctypes.WinDLL(str(DLL))
    reference = Reference(lib)

    lib.DllGetClassObject.argtypes = [POINTER(GUID), POINTER(GUID), POINTER(c_void_p)]
    lib.DllGetClassObject.restype = ctypes.c_long
    raw = c_void_p()
    hr = lib.DllGetClassObject(byref(CLSID_LAPRDUS), byref(IClassFactory._iid_), byref(raw))
    check("the library hands out its class factory", hr == 0 and raw.value, hex(hr & 0xFFFFFFFF))
    factory = ctypes.cast(raw, POINTER(IClassFactory))
    other = c_void_p()
    hr = lib.DllGetClassObject(byref(GUID("{00000000-0000-0000-0000-000000000001}")), byref(IClassFactory._iid_), byref(other))
    check("another class id is refused", hr & 0xFFFFFFFF == 0x80040111, hex(hr & 0xFFFFFFFF))

    def engine_for(language, voice=None):
        token = comtypes.CoCreateInstance(CLSID_SP_OBJECT_TOKEN, ISpObjectToken, comtypes.CLSCTX_INPROC_SERVER)
        token.SetId(None, TOKEN_ID, 1)
        token.SetStringValue("Language", language)
        if voice:
            token.SetStringValue("VoiceId", voice)
        raw = c_void_p()
        factory.CreateInstance(None, byref(ISpTTSEngine._iid_), byref(raw))
        engine = ctypes.cast(raw, POINTER(ISpTTSEngine))
        engine.QueryInterface(ISpObjectWithToken).SetObjectToken(token)
        return engine, token

    def speak(engine, site, *parts):
        frags = fragments(*parts)
        pointer = site.QueryInterface(ISpTTSEngineSite)
        engine.Speak(0, byref(SPDFID_WAVE_FORMAT_EX), None, byref(frags[0]), pointer)
        return bytes(site.audio)

    token = None
    try:
        engine, token = engine_for("hr")
        format_id, wave = GUID(), POINTER(WAVEFORMATEX)()
        engine.GetOutputFormat(None, None, byref(format_id), byref(wave))
        check("the output format is 22050 Hz, 16 bit, mono",
              format_id == SPDFID_WAVE_FORMAT_EX and (wave[0].wFormatTag, wave[0].nChannels, wave[0].nSamplesPerSec,
                                                     wave[0].wBitsPerSample, wave[0].nBlockAlign) == (1, 1, 22050, 16, 2))
        ctypes.windll.ole32.CoTaskMemFree(wave)

        text = "Dobar dan, kako ste? Imam 25 kuna."
        site = Site()
        audio = speak(engine, site, (SPVA_SPEAK, text, 0))
        check("Speak gives the engine's audio for the text", audio == reference.render("hr", text), f"{len(audio)} bytes")
        check("the audio is written in pieces of at most 4096 samples", max(site.writes) <= 8192 and len(site.writes) > 1)

        check("fragments of one sentence are spoken as one sentence",
              speak(engine, Site(), (SPVA_SPEAK, "Dobar dan, ", 0), (SPVA_SPEAK, "kako ste? Imam 25 kuna.  \r\n", 0)) == audio)

        fast = speak(engine, Site(rate=10), (SPVA_SPEAK, text, 0))
        check("SAPI rate 10 is twice the rate", fast == reference.render("hr", text, speed=2.0), f"{len(fast)} bytes")
        quiet = speak(engine, Site(volume=50), (SPVA_SPEAK, text, 0))
        check("SAPI volume 50 is half the volume", quiet == reference.render("hr", text, volume=0.5))
        high = speak(engine, Site(), (SPVA_SPEAK, text, 12))
        check("a pitch adjustment of 12 is half an octave up", high == reference.render("hr", text, pitch=2 ** 0.5))

        spelled = speak(engine, Site(), (SPVA_SPELL_OUT, "abc", 0))
        check("a spell-out fragment is spelled", spelled == reference.render("hr", "abc", spelled=True))
        one = speak(engine, Site(), (SPVA_SPEAK, "b", 0))
        check("a single character is spelled", one == reference.render("hr", "b", spelled=True))

        pause = speak(engine, Site(), (SPVA_SPEAK, "Dobar dan.", 0), (SPVA_SILENCE, 500, 0), (SPVA_SPEAK, "Laku noć.", 0))
        expected = reference.render("hr", "Dobar dan.") + bytes(2 * 11025) + reference.render("hr", "Laku noć.")
        check("a silence fragment is silence of its length between the texts", pause == expected, f"{len(pause)} / {len(expected)}")

        stopped = Site(abort_after=8192)
        speak(engine, stopped, (SPVA_SPEAK, text, 0))
        check("an abort stops the writing", 0 < len(stopped.audio) < len(audio), f"{len(stopped.audio)} of {len(audio)}")
        check("empty text is no audio and no error", speak(engine, Site(), (SPVA_SPEAK, "  \r\n", 0)) == b"")

        for language, sample in (("sr", "Имам 2000 динара."), ("bs", "Imam 2000 maraka."), ("sl", "Imam 2 uri časa.")):
            engine, token = engine_for(language)
            check(f"a token of language {language} speaks {language}",
                  speak(engine, Site(), (SPVA_SPEAK, sample, 0)) == reference.render(language, sample))
        check("the languages differ", reference.render("hr", "2000") != reference.render("sr", "2000"))

        # As the C++ driver did, the engine writes a settings.json of defaults
        # where there is none; that is all it may write.
        written = sorted(str(path.relative_to(SCRATCH)) for path in pathlib.Path(SCRATCH).rglob("*") if path.is_file())
        check("only a default settings.json is written to the settings folder",
              written == [os.path.join("Laprdus", "settings.json")], str(written))
    finally:
        if token is not None:
            try:
                token.Remove(None)
            except comtypes.COMError:
                pass
        import winreg

        for key in ("Software\\LaprdusDriverTest\\Token", "Software\\LaprdusDriverTest"):
            try:
                winreg.DeleteKey(winreg.HKEY_CURRENT_USER, key)
            except OSError:
                pass

    print(f"{failed} failed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
