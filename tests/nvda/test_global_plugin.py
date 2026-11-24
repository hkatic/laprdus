"""Tests of the NVDA add-on's speech filter, without NVDA and without a build.

The global plugin (src/platform/windows/nvda/addon/globalPlugins/laprdus)
registers a filter of NVDA's speech sequences (speech.extensions.
filter_speechSequence, applied before NVDA's symbol processing). While
Laprdus speaks, through its own driver or through NVDA's SAPI5 driver with
a Laprdus voice, the filter writes a date whose separators NVDA would drop
as "18. 11. 2026" (_dates.py) and a period between two digits that NVDA's
table would drop (the Slovenian one) as a one dot leader (U+2024), which the
engine reads as the period (_speechFilter.py). It runs here against the
stubbed NVDA modules of nvda_stubs.py.

    python tests/nvda/test_global_plugin.py

What is covered:
- the filter is registered once and unregistered with the plugin, and an
  NVDA without the filter still loads the plugin;
- it leaves every other synthesizer's speech alone, and a SAPI5 voice that
  is not Laprdus's; it works for the Laprdus driver and for NVDA's sapi5 and
  sapi5_32 drivers with a Laprdus voice, found by its id or its name;
- Slovenian, at every punctuation level: the periods between digits of
  dates, times, decimals, thousands and addresses are carried, no other;
- Croatian: nothing is carried;
- automatic language switching: a Slovenian stretch inside Croatian text is
  carried; a stretch SAPI gives another vendor's voice is left alone;
- applying the filter twice is applying it once;
- the dates of _dates.py, and the Slovenian symbols of the add-on;
- an NVDA before 2024.2 (2023.3.4 is the last for Windows 7 and 8), which
  has no such filter: the plugin puts the same rules in front of the
  processText() of NVDA's speech module (speech.speech, or the speech
  package before 2021.1), the text reaches the symbol processing as through
  the filter, and terminate() takes the hook away;
- an NVDA before 2024.4, which ignores the manifest's symbol dictionaries:
  the plugin loads the add-on's Slovenian one into NVDA's symbol data.
"""
import os
import sys
import types

# The names of the checks hold Slovenian letters (č, š, ž), which a console
# in cp1252, as on GitHub's Windows machines, cannot print
sys.stdout.reconfigure(encoding="utf-8")

HERE =os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
ADDON = os.path.join(ROOT, "src", "platform", "windows", "nvda", "addon")

sys.path.insert(0, HERE)
import nvda_stubs as S  # noqa: E402  (installs the stub modules)

# The add-on's globalPlugins folder as the package NVDA makes of it
_package = types.ModuleType("globalPlugins")
_package.__path__ = [os.path.join(ADDON, "globalPlugins")]
sys.modules["globalPlugins"] = _package
import globalPlugins.laprdus as plugin  # noqa: E402
from globalPlugins.laprdus import _dates, _speechFilter, _symbols  # noqa: E402
from speech.commands import IndexCommand, LangChangeCommand  # noqa: E402

CARRIER = "\u2024"
FILTER = S.extensions.filter_speechSequence
failures = []


def check(name, ok):
    print("%s %s" % ("ok  " if ok else "FAIL", name))
    if not ok:
        failures.append(name)


class Synth:
    def __init__(self, name, voice=None, voices=()):
        self.name, self.voice = name, voice
        self.availableVoices = dict((v.id, v) for v in voices)


LAPRDUS_SL = S.VoiceInfo(r"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusSlovenian",
                         "Laprdus Slovenian", "sl_SI")
LAPRDUS_HR = S.VoiceInfo(r"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Speech\Voices\Tokens\LaprdusCroatian",
                         "Laprdus Croatian", "hr_HR")
# Another vendor's voices
OTHER_EN = S.VoiceInfo(r"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Speech\Voices\Tokens\OtherEnglish",
                       "Another English voice", "en_US")
OTHER_HR = S.VoiceInfo(r"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Speech\Voices\Tokens\OtherCroatian",
                       "Another Croatian voice", "hr_HR")


def settings(level=0, language="sl_SI", auto_language=True, auto_dialect=False):
    S.config.conf["speech"].update(symbolLevel=level, autoLanguageSwitching=auto_language,
                                   autoDialectSwitching=auto_dialect)
    S.CURRENT_LANGUAGE[0] = language


def filtered(synth, sequence):
    S.CURRENT_SYNTH[0] = synth
    return FILTER.apply(sequence)


def test_registration():
    del FILTER.handlers[:]
    first = plugin.GlobalPlugin()
    check("the plugin registers its filter", len(FILTER.handlers) == 1)
    first.terminate()
    check("terminate() unregisters it", FILTER.handlers == [])
    second = plugin.GlobalPlugin()
    check("a plugin made again registers it again", len(FILTER.handlers) == 1)
    second.terminate()
    real = plugin.filter_speechSequence
    plugin.filter_speechSequence = None
    try:
        old = plugin.GlobalPlugin()
        old.terminate()
        check("an NVDA without the filter (before 2024.2) still loads the plugin", FILTER.handlers == [])
    finally:
        plugin.filter_speechSequence = real


def test_synthesizers():
    settings()
    live = plugin.GlobalPlugin()
    text = ["Rojen je 18.11.2026 v Ljubljani.", IndexCommand(1)]
    carried = "Rojen je 18%s11%s2026 v Ljubljani." % (CARRIER, CARRIER)
    for name, synth in (
        ("no synthesizer", None),
        ("eSpeak", Synth("espeak", "sl")),
        ("oneCore", Synth("oneCore", "sl-SI")),
        ("sapi5 with another vendor's voice", Synth("sapi5", OTHER_EN.id, [OTHER_EN, LAPRDUS_SL])),
    ):
        check("%s: the sequence is left alone" % name, filtered(synth, text) is text)
    for name, synth in (
        ("the Laprdus driver", Synth("laprdus", "zvonko")),
        ("sapi5 with Laprdus Slovenian", Synth("sapi5", LAPRDUS_SL.id, [LAPRDUS_SL, OTHER_EN])),
        ("sapi5_32 with Laprdus Slovenian", Synth("sapi5_32", LAPRDUS_SL.id, [LAPRDUS_SL])),
        ("sapi5, a Laprdus voice known by its name only",
         Synth("sapi5", "Token7", [S.VoiceInfo("Token7", "Laprdus Slovenian", "sl_SI")])),
    ):
        out = filtered(synth, text)
        check("%s: the periods of the date are carried" % name, out[0] == carried and out[1] is text[1])

    class Broken(Synth):
        @property
        def voice(self):
            raise RuntimeError("the voice cannot be read")

        @voice.setter
        def voice(self, value):
            pass

    check("a synthesizer that cannot tell its voice is left alone", filtered(Broken("sapi5"), text) is text)
    live.terminate()


def test_slovenian():
    live = plugin.GlobalPlugin()
    synth = Synth("sapi5", LAPRDUS_SL.id, [LAPRDUS_SL])
    c = CARRIER
    for level in (0, 100, 200, 300):
        settings(level)
        for text, expected in (
            ("Rojen je 18.11.2026 v Ljubljani.", "Rojen je 18%s11%s2026 v Ljubljani." % (c, c)),
            ("Ura je 8.30.", "Ura je 8%s30." % c),
            ("od 8.30 do 16.00", "od 8%s30 do 16%s00" % (c, c)),
            ("3.14", "3%s14" % c),
            ("132.457", "132%s457" % c),
            ("192.168.1.1", "192%s168%s1%s1" % (c, c, c)),
            ("2026.11.18", "2026%s11%s18" % (c, c)),
            ("18.11.", "18%s11." % c),
        ):
            out = filtered(synth, [text])[0]
            check("level %d: %r is carried" % (level, text), out == expected)
        for text in ("a.5", "3. 4", ".5", "konec.", "www.rtvslo.si", "Dobar dan."):
            out = filtered(synth, [text])
            check("level %d: %r is left alone" % (level, text), out[0] == text)
    live.terminate()


def test_croatian():
    live = plugin.GlobalPlugin()
    settings(0, "hr_HR")
    sequence = ["Rođen je 18.11.2026. u Zagrebu, 3.14 i 192.168.1.1."]
    for synth in (Synth("laprdus", "zvonko"), Synth("sapi5", LAPRDUS_HR.id, [LAPRDUS_HR])):
        check("%s in Croatian: nothing is carried" % synth.name, filtered(synth, sequence) is sequence)
    check("Croatian: a slashed date is still spaced",
          filtered(Synth("laprdus", "zvonko"), ["18/11/2026"]) == ["18. 11. 2026"])
    live.terminate()


def test_language_switching():
    live = plugin.GlobalPlugin()
    settings(0, "hr_HR")
    laprdus = Synth("laprdus", "zvonko")
    sequence = ["Datum 18.11.2026", LangChangeCommand("sl"), "Rojen 18.11.2026", LangChangeCommand(None), "3.14"]
    out = filtered(laprdus, sequence)
    check("a Slovenian stretch inside Croatian text is carried, the rest is not",
          [out[0], out[2], out[4]] == ["Datum 18.11.2026", "Rojen 18%s11%s2026" % (CARRIER, CARRIER), "3.14"])
    settings(0, "hr_HR", auto_language=False)
    check("without automatic language switching the stretch has the default language",
          filtered(laprdus, sequence) is sequence)

    settings(0, "sl_SI")
    english = [LangChangeCommand("en"), "18/11/2026 3.14", LangChangeCommand("sl_SI"), "3.14"]
    sapi = Synth("sapi5", LAPRDUS_SL.id, [LAPRDUS_SL, OTHER_EN])
    out = filtered(sapi, english)
    check("an English stretch SAPI gives another vendor's voice is left alone, the Slovenian one is not",
          out[1] == "18/11/2026 3.14" and out[3] == "3%s14" % CARRIER)
    alone = Synth("sapi5", LAPRDUS_SL.id, [LAPRDUS_SL])
    out = filtered(alone, english)
    check("without an English voice Laprdus speaks the English stretch: its date is spaced",
          out[1] == "18. 11. 2026 3.14")
    out = filtered(laprdus, english)
    check("the Laprdus driver speaks the English stretch too", out[1] == "18. 11. 2026 3.14")
    croatian = Synth("sapi5", LAPRDUS_SL.id, [LAPRDUS_SL, OTHER_HR])
    out = filtered(croatian, [LangChangeCommand("hr"), "18/11/2026"])
    check("a Croatian stretch goes to the Laprdus Croatian voice", out[1] == "18. 11. 2026")
    live.terminate()


def test_idempotent():
    settings(0, "sl_SI")
    synth = Synth("laprdus", "zvonko")
    context = plugin._speechContext(_speechFilter.laprdus_speaker(synth))
    sequence = ["Rojen je 18.11.2026, ob 8.30, 18/11/2026, 18.XI.2026 in 2026-11-18T10:30."]
    once = _speechFilter.filter_sequence(sequence, context)
    twice = _speechFilter.filter_sequence(once, context)
    check("the filter changed the text", once != sequence)
    check("applying it twice is applying it once", twice == once)


def test_dates():
    """A date in a written form NVDA would break goes to NVDA's symbol
    processing as "18. 11. 2026", which it leaves alone; a period between
    digits is the carrier's; what is no date is untouched, and a separator
    NVDA speaks at the user's level is left to NVDA."""
    spaced = _dates.spaced_dates
    for text in (
        "18/11/2026", "18-11-2026", "18‑11‑2026", "2026-11-18", "2026/11/18", "11/18/2026",
        "18.XI.2026", "18. XI. 2026", "18/XI/2026", "18-XI-2026", "18. XI. 2026",
    ):
        check("%a is the spaced date, got %a" % (text, spaced(text)), spaced(text) == "18. 11. 2026")
        check("%a inside a sentence" % text, spaced("Danes je %s." % text) == "Danes je 18. 11. 2026.")
    check("a slashed date with leading zeros", spaced("05/03/1985") == "5. 3. 1985")
    check("a two-digit year after slashes", spaced("18/11/26") == "18. 11. 26")
    check("the time of ISO 8601", spaced("2026-11-18T10:30") == "18. 11. 2026 10:30")
    for text in (
        # The periods between digits are carried, not rewritten
        "18.11.2026", "2026.11.18", "18.11.", "05.03.1985", "18. 11. 2026", "18. 11. 2026",
        # No dates
        "1/2", "24/7", "50/50", "PS/2", "10-15", "716-844", "10-12-14", "1/2/2026/3", "2026-11-18-4",
        "32/11/2026", "18/13/2026", "2026-13-18", "3.14", "192.168.1.1", "10.10.10.10", "1.000.000",
        "A18/11/2026", "18/11/2026a", "18/11/2026.5", "tel. 05-03-1985", "1. I. Kant", "12:30", "www.rtvslo.si",
        "Dobar dan.",
    ):
        check("%a is left alone, got %a" % (text, spaced(text)), spaced(text) == text)
    # At "some" NVDA names the period and the slash, at "most" the hyphen too
    check("slashes are NVDA's at the level some", spaced("18/11/2026", 100) == "18/11/2026")
    check("periods before a Roman month are NVDA's at the level some", spaced("18.XI.2026", 100) == "18.XI.2026")
    check("hyphens are silent at the level some", spaced("2026-11-18", 100) == "18. 11. 2026")
    check("hyphens are NVDA's at the level most", spaced("2026-11-18", 200) == "2026-11-18")


def test_symbols():
    path = os.path.join(ADDON, "locale", "sl", "symbols-laprdus.dic")
    with open(path, encoding="utf-8") as f:
        rows = [line.rstrip("\n").split("\t") for line in f if "\t" in line and not line.startswith("#")]
    symbols = dict((row[0], row[1:]) for row in rows)
    for symbol, name in (("«", "dvojni začetni narekovaj"), ("»", "dvojni zaključni narekovaj"),
                         ("-", "kratka črtica"), ("—", "črtica")):
        check("the Slovenian %s is %s at the level most and preserved" % (ascii(symbol), name),
              symbols.get(symbol) == [name, "most", "always"])


# --- An NVDA before 2024.2 (2023.3.4 is the last for Windows 7 and 8): no
# filter_speechSequence; speak() hands each text, with the language of its
# stretch, to processText() of its own module, which processes the symbols.
# speak() below is the two loops of speak() in source/speech/speech.py of
# NVDA 2023.3.4 (the same in 2019.3 to 2024.1), returning what it would hand
# the synthesizer (without the chunk separators).
def old_speech_module(module=None):
    """`module` (a new speech.speech by default) with the processText() and
    speak() of NVDA before 2024.2."""
    if module is None:
        module = types.ModuleType("speech.speech")

    def processText(locale, text, symbolLevel):
        # NVDA applies the user's speech dictionaries first (none here)
        return S.processSpeechSymbols(locale, text, symbolLevel).strip()

    def speak(speechSequence, symbolLevel=None):
        conf = S.config.conf["speech"]
        auto, dialect = conf["autoLanguageSwitching"], conf["autoDialectSwitching"]
        curLanguage = defaultLanguage = S.speech.getCurrentLanguage()
        prevLanguage = None
        root = defaultLanguage.split("_")[0]
        sequence = []
        for item in speechSequence:
            if isinstance(item, LangChangeCommand):
                if not auto:
                    continue
                curLanguage = item.lang
                if not curLanguage or (not dialect and curLanguage.split("_")[0] == root):
                    curLanguage = defaultLanguage
            elif isinstance(item, str):
                if not item:
                    continue
                if auto and curLanguage != prevLanguage:
                    sequence.append(LangChangeCommand(curLanguage))
                    prevLanguage = curLanguage
                sequence.append(item)
            else:
                sequence.append(item)
        if symbolLevel is None:
            symbolLevel = conf["symbolLevel"]
        curLanguage = defaultLanguage
        for index, item in enumerate(sequence):
            if auto and isinstance(item, LangChangeCommand):
                curLanguage = item.lang
            if isinstance(item, str):
                # A global of the module, looked up at every call, as in NVDA
                sequence[index] = module.processText(curLanguage, item, symbolLevel)
        return sequence

    module.processText, module.speak = processText, speak
    return module


class OldNVDA:
    """NVDA before 2024.2 for the length of a with block: `layout` is
    "speech.speech" (2021.1 to 2024.1) or "speech" (2019.3 to 2020.4, where
    speak() is in the package itself). The module whose speak() to call is
    `self.module`."""

    def __init__(self, layout="speech.speech"):
        self.layout = layout

    def __enter__(self):
        self.saved = plugin.filter_speechSequence
        plugin.filter_speechSequence = None
        if self.layout == "speech":
            # No speech.speech: the package itself has speak() and processText()
            self.module = old_speech_module(S.speech)
        else:
            self.module = old_speech_module()
            S.speech.speech = self.module
            sys.modules["speech.speech"] = self.module
        return self

    def __exit__(self, *exc):
        plugin.filter_speechSequence = self.saved
        for name in ("speech", "processText", "speak"):
            if hasattr(S.speech, name):
                delattr(S.speech, name)
        sys.modules.pop("speech.speech", None)
        return False


def spoken(nvda, synth, sequence, symbolLevel=None):
    """What `nvda` hands `synth` for `sequence`: the texts only."""
    S.CURRENT_SYNTH[0] = synth
    return [item for item in nvda.module.speak(list(sequence), symbolLevel) if isinstance(item, str)]


def test_old_nvda():
    c = CARRIER
    laprdus = Synth("laprdus", "zvonko")
    sapi = Synth("sapi5", LAPRDUS_SL.id, [LAPRDUS_SL, OTHER_EN])
    other = Synth("espeak", "sl")
    text = ["Rojen je 16.4.1986.", IndexCommand(1)]
    for layout in ("speech.speech", "speech"):
        with OldNVDA(layout) as nvda:
            original = nvda.module.processText
            del S.WARNINGS[:]
            live = plugin.GlobalPlugin()
            check("%s: an NVDA without the filter gets the hook in front of processText()" % layout,
                  live._processTextHook is not None and nvda.module.processText is not original)
            check("%s: and no filter is registered" % layout, FILTER.handlers == [])
            check("%s: and nothing is logged" % layout, S.WARNINGS == [])
            settings(0, "sl_SI")
            check("%s, the Laprdus driver: the periods of the date are carried" % layout,
                  spoken(nvda, laprdus, text) == ["Rojen je 16%s4%s1986." % (c, c)])
            check("%s, sapi5 with Laprdus Slovenian: the periods are carried" % layout,
                  spoken(nvda, sapi, text) == ["Rojen je 16%s4%s1986." % (c, c)])
            check("%s, another synthesizer: NVDA's Slovenian table drops them, as before" % layout,
                  spoken(nvda, other, text) == ["Rojen je 16 4 1986."])
            live.terminate()
            check("%s: terminate() puts NVDA's processText() back" % layout, nvda.module.processText is original)
            check("%s: and the periods are NVDA's again" % layout,
                  spoken(nvda, laprdus, text) == ["Rojen je 16 4 1986."])

    with OldNVDA() as nvda:
        live = plugin.GlobalPlugin()
        for level in (0, 100, 200, 300):
            settings(level, "sl_SI")
            check("level %d: ob 8.30, 3.14 and 132.457 are carried" % level,
                  spoken(nvda, sapi, ["ob 8.30, 3.14 in 132.457"]) == ["ob 8%s30, 3%s14 in 132%s457" % (c, c, c)])
        settings(0, "hr_HR")
        check("Croatian: nothing is carried",
              spoken(nvda, laprdus, ["Rođen je 16.4.1986. u Zagrebu."]) == ["Rođen je 16.4.1986. u Zagrebu."])
        check("Croatian: a slashed date is spaced", spoken(nvda, laprdus, ["18/11/2026"]) == ["18. 11. 2026"])
        check("the level speak() is given is the level of the dates: slashes are NVDA's at some",
              spoken(nvda, laprdus, ["18/11/2026"], symbolLevel=100) == ["18/11/2026"])
        check("a Slovenian stretch inside Croatian text is carried, the rest is not",
              spoken(nvda, laprdus, ["Datum 18.11.2026", LangChangeCommand("sl"), "Rojen 18.11.2026"])
              == ["Datum 18.11.2026", "Rojen 18%s11%s2026" % (c, c)])
        settings(0, "sl_SI")
        english = [LangChangeCommand("en"), "18/11/2026 3.14", LangChangeCommand("sl_SI"), "3.14"]
        check("an English stretch SAPI gives another vendor's voice is left alone",
              spoken(nvda, sapi, english) == ["18/11/2026 3.14", "3%s14" % c])

        class Broken(Synth):
            @property
            def name(self):
                raise RuntimeError("the synthesizer cannot be read")

            @name.setter
            def name(self, value):
                pass

        del S.WARNINGS[:]
        check("an error in the filter hands NVDA the text as it was",
              spoken(nvda, Broken("x"), text) == ["Rojen je 16 4 1986."] and len(S.WARNINGS) == 1)

        # An add-on that wraps processText() after the plugin did
        ours = nvda.module.processText
        nvda.module.processText = lambda *a, **k: ours(*a, **k)
        wrapper = nvda.module.processText
        live.terminate()
        check("terminate() leaves a later add-on's wrapper in place", nvda.module.processText is wrapper)
        check("and the hook under it no longer filters", spoken(nvda, laprdus, text) == ["Rojen je 16 4 1986."])

    def failing(locale, text, level):
        raise RuntimeError("the filter failed")

    module = old_speech_module()
    hook = _speechFilter.ProcessTextHook(module, failing)
    check("ProcessTextHook: a filter that raises hands on the text as it was",
          module.processText("sl", "16.4.1986", 0) == "16 4 1986")
    hook.remove()

    # With the filter, nothing is put before processText()
    with OldNVDA() as nvda:
        plugin.filter_speechSequence = FILTER
        original = nvda.module.processText
        live = plugin.GlobalPlugin()
        check("an NVDA with the filter (2024.2+) gets no hook", live._processTextHook is None
              and nvda.module.processText is original and len(FILTER.handlers) == 1)
        live.terminate()


def test_old_and_new_nvda_hand_laprdus_the_same_text():
    """Through the hook (NVDA before 2024.2) Laprdus gets what it gets
    through the filter (2024.2 and later), at every level and language."""
    sequences = [
        ["Rojen je 18.11.2026 v Ljubljani.", IndexCommand(1)],
        ["Ura je 8.30.", "od 8.30 do 16.00", "3.14", "132.457", "192.168.1.1", "2026.11.18", "18.11."],
        ["a.5", "3. 4", ".5", "konec.", "www.rtvslo.si", "Dobar dan."],
        ["Rojen je 18.11.2026, ob 8.30, 18/11/2026, 18.XI.2026 in 2026-11-18T10:30."],
        ["Datum 18.11.2026", LangChangeCommand("sl"), "Rojen 18.11.2026", LangChangeCommand(None), "3.14"],
        [LangChangeCommand("en"), "18/11/2026 3.14", LangChangeCommand("sl_SI"), "3.14"],
        [LangChangeCommand("hr"), "18/11/2026"],
    ]
    synths = (
        Synth("laprdus", "zvonko"),
        Synth("sapi5", LAPRDUS_SL.id, [LAPRDUS_SL, OTHER_EN]),
        Synth("sapi5", LAPRDUS_SL.id, [LAPRDUS_SL]),
        Synth("sapi5", LAPRDUS_SL.id, [LAPRDUS_SL, OTHER_HR]),
        Synth("sapi5", OTHER_EN.id, [OTHER_EN, LAPRDUS_SL]),
        Synth("espeak", "sl"),
    )
    differ = []
    with OldNVDA() as nvda:
        for level in (0, 100, 200, 300):
            for language in ("sl_SI", "hr_HR"):
                for auto in (True, False):
                    settings(level, language, auto_language=auto)
                    for synth in synths:
                        for sequence in sequences:
                            plugin.filter_speechSequence = FILTER
                            new = plugin.GlobalPlugin()
                            S.CURRENT_SYNTH[0] = synth
                            through_filter = spoken(nvda, synth, FILTER.apply(list(sequence)))
                            new.terminate()
                            plugin.filter_speechSequence = None
                            old = plugin.GlobalPlugin()
                            through_hook = spoken(nvda, synth, sequence)
                            old.terminate()
                            if through_filter != through_hook:
                                differ.append((level, language, auto, synth.name, through_filter, through_hook))
    for case in differ[:5]:
        print("     differs: %r" % (case,))
    check("the hook and the filter hand Laprdus the same text in %d cases" % (4 * 2 * 2 * 6 * 7), differ == [])


# --- An NVDA before 2024.4 ignores the manifest's symbolDictionaries; the
# plugin loads the add-on's dictionary into NVDA's own symbol data. The fake
# below has the parts of characterProcessing that NVDA 2019.3 to 2024.3 share:
# SpeechSymbolProcessor.localeSymbols, a LocaleDataMap whose factory returns
# (builtin, user); SpeechSymbols.load(), which adds to the symbols; and
# clearSpeechSymbols(), which empties the caches.
class FakeSymbols:
    def __init__(self):
        self.symbols = {}
        self.files = []

    def load(self, fileName, allowComplexSymbols=True):
        self.files.append(os.path.basename(fileName))
        with open(fileName, encoding="utf-8-sig") as f:
            section = None
            for line in f:
                line = line.rstrip("\r\n")
                if not line or line.startswith("#"):
                    continue
                if line.endswith(":") and "\t" not in line:
                    section = line
                elif section == "symbols:":
                    row = line.split("\t")
                    self.symbols[row[0]] = row[1:]


class FakeLocaleDataMap:
    def __init__(self, factory):
        self._localeDataFactory = factory
        self._dataMap = {}

    def fetchLocaleData(self, locale):
        if locale not in self._dataMap:
            self._dataMap[locale] = self._localeDataFactory(locale)
        return self._dataMap[locale]


NVDA_SL = {"«": ["double dvojni začetni narekovaj", "none"], "»": ["double dvojni zaključni narekovaj", "none"],
           ".": ["pika", "some"]}


def fake_character_processing():
    module = types.ModuleType("characterProcessing")
    module.cleared = 0

    def factory(locale):
        if locale not in ("sl", "hr", "en"):
            raise LookupError(locale)
        builtin, user = FakeSymbols(), FakeSymbols()
        if locale == "sl":
            builtin.symbols.update((k, list(v)) for k, v in NVDA_SL.items())
        return builtin, user

    class SpeechSymbolProcessor:
        localeSymbols = FakeLocaleDataMap(factory)

    def clearSpeechSymbols():
        module.cleared += 1
        SpeechSymbolProcessor.localeSymbols._dataMap.clear()

    module.SpeechSymbolProcessor = SpeechSymbolProcessor
    module.clearSpeechSymbols = clearSpeechSymbols
    module.processSpeechSymbols = S.processSpeechSymbols
    return module


def test_old_nvda_symbol_dictionaries():
    found = _symbols.addon_dictionaries(ADDON)
    check("the add-on has a symbol dictionary for sl, and only for sl", sorted(found) == ["sl"])

    new = plugin.GlobalPlugin()
    check("NVDA 2024.4 and later load the dictionary themselves: no hook", new._symbolDictionaryHook is None)
    new.terminate()

    fake = fake_character_processing()
    symbols = fake.SpeechSymbolProcessor.localeSymbols
    original = symbols._localeDataFactory
    sys.modules["characterProcessing"] = fake
    try:
        symbols.fetchLocaleData("sl")
        del S.WARNINGS[:]
        live = plugin.GlobalPlugin()
        check("an NVDA before 2024.4 gets the hook", live._symbolDictionaryHook is not None
              and symbols._localeDataFactory is not original and S.WARNINGS == [])
        check("and NVDA's cached symbol data is cleared", fake.cleared == 1 and symbols._dataMap == {})
        builtin, user = symbols.fetchLocaleData("sl")
        check("the Slovenian data has the add-on's dictionary after NVDA's table",
              builtin.files == ["symbols-laprdus.dic"])
        check("» and « have the level most and are preserved",
              builtin.symbols["«"] == ["dvojni začetni narekovaj", "most", "always"]
              and builtin.symbols["»"] == ["dvojni zaključni narekovaj", "most", "always"])
        check("NVDA's other Slovenian symbols stay", builtin.symbols["."] == ["pika", "some"])
        check("the user's own symbols are not touched", user.files == [] and user.symbols == {})
        check("the Croatian data is NVDA's own", symbols.fetchLocaleData("hr")[0].files == [])
        try:
            symbols.fetchLocaleData("sl_SI")
            unknown = False
        except LookupError:
            unknown = True
        check("a locale NVDA has no data for is still unknown", unknown)
        live.terminate()
        check("terminate() gives NVDA its factory back and clears the caches again",
              symbols._localeDataFactory is original and fake.cleared == 2)
        check("and the Slovenian data is NVDA's own again",
              symbols.fetchLocaleData("sl")[0].symbols["«"] == ["double dvojni začetni narekovaj", "none"])

        # A dictionary that cannot be read leaves NVDA's data as it was
        hook = _symbols.SymbolDictionaryHook(fake, {"sl": os.path.join(HERE, "no such file.dic")}, S.logHandler.log)
        del S.WARNINGS[:]
        check("a missing dictionary leaves NVDA's table and is logged",
              symbols.fetchLocaleData("sl")[0].symbols["«"] == ["double dvojni začetni narekovaj", "none"]
              and len(S.WARNINGS) == 1)
        hook.remove()

        # An NVDA whose symbol data is kept otherwise gets no hook, and still loads the plugin
        del fake.SpeechSymbolProcessor
        del S.WARNINGS[:]
        other = plugin.GlobalPlugin()
        check("an NVDA without that factory gets no hook, and says so in its log",
              other._symbolDictionaryHook is None and len(S.WARNINGS) == 1)
        other.terminate()
    finally:
        sys.modules["characterProcessing"] = S.characterProcessing


if __name__ == "__main__":
    test_registration()
    test_synthesizers()
    test_slovenian()
    test_croatian()
    test_language_switching()
    test_idempotent()
    test_dates()
    test_symbols()
    test_old_nvda()
    test_old_and_new_nvda_hand_laprdus_the_same_text()
    test_old_nvda_symbol_dictionaries()
    print("%d failed" % len(failures))
    sys.exit(1 if failures else 0)
