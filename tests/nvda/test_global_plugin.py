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
- the dates of _dates.py, and the Slovenian symbols of the add-on.
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
from globalPlugins.laprdus import _dates, _speechFilter  # noqa: E402
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


if __name__ == "__main__":
    test_registration()
    test_synthesizers()
    test_slovenian()
    test_croatian()
    test_language_switching()
    test_idempotent()
    test_dates()
    test_symbols()
    print("%d failed" % len(failures))
    sys.exit(1 if failures else 0)
