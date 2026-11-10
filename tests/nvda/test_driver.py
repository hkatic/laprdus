"""Tests of the NVDA synthesizer driver without NVDA.

The driver (nvda-addon/addon/synthDrivers/laprdus) runs against stubbed NVDA
modules (nvda_stubs.py) and the DLL of the built add-on, so these tests need
the add-on's DLLs in place (scons --platform=windows ... sapi5 for both
architectures, or scripts\\build-all.cmd nvda). They feed the driver the
speech sequences NVDA builds (source/speech/speech.py in the NVDA
repository) and compare what it plays with what the engine renders when
asked directly.

    python tests/nvda/test_driver.py

What is covered:
- a spelled letter follows the spelling mode of settings.json (names or
  sounds), also after the file changes while the driver runs;
- NVDA's capital pitch change (PitchCommand(offset=...) before a spelled
  capital letter, PitchCommand() after it) raises that letter's pitch and
  only that letter's, with the Laprdus pitch forced or not; a change NVDA
  replays at the start of a sequence applies to its text;
- RateCommand and VolumeCommand;
- speech.volume_boost of settings.json switches the boost on;
- the voice quality keys of settings.json (head_size, breathiness,
  roughness, brightness) reach the engine and change the speech, and a
  file without them leaves the voice as it is;
- NVDA's delayed character descriptions: the break after a spelled
  character is played as silence and the description follows;
- the Language setting comes before the Voice setting.
"""
import json
import os
import struct
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
APPDATA = os.path.join(HERE, "_appdata")
os.environ["APPDATA"] = APPDATA
os.makedirs(os.path.join(APPDATA, "Laprdus"), exist_ok=True)

sys.path.insert(0, HERE)
import nvda_stubs as S  # noqa: E402  (installs the stub modules)

sys.path.insert(0, os.path.join(ROOT, "nvda-addon", "addon", "synthDrivers"))
import laprdus as drv  # noqa: E402
from speech.commands import (  # noqa: E402
    BreakCommand,
    CharacterModeCommand,
    IndexCommand,
    PitchCommand,
    RateCommand,
    VolumeCommand,
)

_mtime = [time.time()]
failures = []


def check(name, ok):
    print("%s %s" % ("ok  " if ok else "FAIL", name))
    if not ok:
        failures.append(name)


def write_settings(mode="names", force_pitch=False, force_volume=False, volume=1.0, volume_boost=None,
                   quality=None):
    """quality: the voice quality keys to write into the speech section
    (head_size, breathiness, roughness, brightness), none by default."""
    path = os.path.join(APPDATA, "Laprdus", "settings.json")
    with open(path, "w", encoding="utf-8") as f:
        json.dump({
            "version": "1.0",
            "voice": {"default": "zvonko"},
            "speech": dict({"speed": 1.0, "pitch": 1.0, "volume": volume, "inflection": True,
                            "inflection_level": 0.5, "acceleration": 1.0, "emoji": False},
                           **({} if volume_boost is None else {"volume_boost": volume_boost}),
                           **(quality or {})),
            "spelling": {"speed": 50, "mode": mode},
            "force": {"speed": False, "pitch": force_pitch, "volume": force_volume},
        }, f)
    # The driver reloads on a changed mtime: make every write a change
    _mtime[0] += 5
    os.utime(path, (_mtime[0], _mtime[0]))


def new_synth():
    synth = drv.SynthDriver()
    # What NVDA does after __init__ (loadSettings), with the saved values
    synth._set_voice("zvonko")
    synth._set_language("hr")
    synth._set_rate(80)
    synth._set_pitch(50)
    synth._set_volume(100)
    synth._set_inflection(50)
    synth._set_rateBoost(False)
    return synth


def run(synth, seq):
    """Speak a sequence and return the chunks the driver played."""
    del S.FED[:]
    synth.speak(seq)
    for _ in range(1000):
        time.sleep(0.01)
        if not synth.isSpeaking:
            break
    time.sleep(0.05)
    return list(S.FED)


def spelled(synth, text):
    return synth._engine.synthesize_spelled(text, final_pause=False)[0]


def spoken(synth, text):
    return synth._engine.synthesize(text)[0]


def test_spelling_mode():
    write_settings("sounds")
    synth = new_synth()
    e = synth._engine
    e.set_spelling_mode(True)
    sound = spelled(synth, "b")
    e.set_spelling_mode(False)
    name = spelled(synth, "b")
    e.set_spelling_mode(True)
    check("letter sounds differ from letter names", sound != name)

    typed = [CharacterModeCommand(True), "b", CharacterModeCommand(False), IndexCommand(1)]
    out = run(synth, typed)
    check("typed letter is spelled by its sound when settings.json says sounds",
          out and out[-1] == sound)

    write_settings("names")
    out = run(synth, typed)
    check("after the file changes to names the letter is named", out and out[-1] == name)

    write_settings("sounds")
    out = run(synth, typed)
    check("after the file changes back to sounds the letter is sounded", out and out[-1] == sound)
    synth.terminate()


def test_capital_pitch(force_pitch):
    write_settings("names", force_pitch=force_pitch)
    synth = new_synth()
    e = synth._engine
    e.set_user_pitch(drv._nvda_to_laprdus_pitch(50))
    plain = spelled(synth, "B")
    e.set_user_pitch(drv._nvda_to_laprdus_pitch(80))
    raised = spelled(synth, "B")
    synth._applyPitch()
    label = " (Laprdus pitch forced)" if force_pitch else ""

    # speech.getSpellingSpeech for a capital with capPitchChange 30
    out = run(synth, [PitchCommand(offset=30), CharacterModeCommand(True), "B", PitchCommand(),
                      CharacterModeCommand(False), IndexCommand(1)])
    check("capital letter is spelled at the raised pitch" + label, out and out[-1] == raised)
    out = run(synth, [CharacterModeCommand(True), "B", CharacterModeCommand(False), IndexCommand(2)])
    check("the next letter is back at the slider pitch" + label, out and out[-1] == plain)

    # a change still in force is replayed at the start of the next sequence
    out = run(synth, [PitchCommand(offset=30), "Dobar dan", IndexCommand(3)])
    e.set_user_pitch(drv._nvda_to_laprdus_pitch(80))
    ref = spoken(synth, "Dobar dan")
    synth._applyPitch()
    check("a replayed pitch change applies to the text" + label, out and out[-1] == ref)

    # text, a raised capital and text again, each at its own pitch
    out = run(synth, ["veliko ", PitchCommand(offset=30), CharacterModeCommand(True), "B",
                      PitchCommand(), CharacterModeCommand(False), "slovo", IndexCommand(4)])
    ref_word = spoken(synth, "veliko")
    ref_after = spoken(synth, "slovo")
    check("text around the capital keeps the slider pitch" + label,
          len(out) == 4 and out[0] == ref_word and out[1] == raised and out[3] == ref_after)
    synth.terminate()


def test_rate_and_volume():
    write_settings("names")
    synth = new_synth()
    e = synth._engine
    out = run(synth, [RateCommand(multiplier=1.5), "brzo", RateCommand(), "sporo", IndexCommand(1)])
    e.set_speed(drv._nvda_to_rate_factor(100))     # 80 x 1.5, capped at the top of the slider
    fast = spoken(synth, "brzo")
    e.set_speed(drv._nvda_to_rate_factor(80))
    normal = spoken(synth, "sporo")
    synth._applyRate()
    check("RateCommand speeds up its text and the rest returns to the slider",
          len(out) == 2 and out[0] == fast and out[1] == normal)

    out = run(synth, [VolumeCommand(offset=-50), "tiho", VolumeCommand(), "glasno", IndexCommand(2)])
    e.set_volume(0.5)
    quiet = spoken(synth, "tiho")
    e.set_volume(1.0)
    loud = spoken(synth, "glasno")
    synth._applyVolume()
    check("VolumeCommand lowers its text and the rest returns to the slider",
          len(out) == 2 and out[0] == quiet and out[1] == loud)
    synth.terminate()

    write_settings("names", force_volume=True, volume=0.6)
    synth = new_synth()
    out = run(synth, ["pola", IndexCommand(1)])
    synth._engine.set_volume(0.6)
    ref = spoken(synth, "pola")
    check("a forced Laprdus volume is used as it is", len(out) == 1 and out[0] == ref)
    synth.terminate()


def test_volume_boost():
    # speech.volume_boost from settings.json switches the engine's boost on;
    # a file without the key leaves it off; the slider still scales it
    write_settings("names")
    synth = new_synth()
    check("a settings.json without volume_boost leaves the boost off",
          not synth._engine.get_volume_boost())
    plain = run(synth, ["glasno", IndexCommand(1)])
    write_settings("names", volume_boost=True)
    synth._checkConfigChanged()
    check("speech.volume_boost switches the boost on when the file changes",
          synth._engine.get_volume_boost())
    boosted = run(synth, ["glasno", IndexCommand(2)])
    import struct
    def rms(data):
        samples = struct.unpack("<%dh" % (len(data) // 2), data)
        return (sum(s * s for s in samples) / max(1, len(samples))) ** 0.5
    check("the boost makes the speech louder",
          len(plain) == 1 and len(boosted) == 1 and rms(boosted[0]) > rms(plain[0]) * 1.3)
    synth._set_volume(50)
    half = run(synth, ["glasno", IndexCommand(3)])
    check("NVDA's volume slider still scales the boosted speech",
          len(half) == 1 and rms(half[0]) < rms(boosted[0]) * 0.6)
    synth.terminate()


def test_voice_quality():
    # Head size, breathiness, roughness and brightness (0-100, 50 = the voice
    # as it is) are NVDA's own settings: sliders of the voice settings and the
    # synth settings ring, restored by NVDA through the _set_ methods. The
    # keys of settings.json are SAPI5's and leave NVDA's values alone.
    write_settings("names", quality={"head_size": 40, "roughness": 90})
    synth = new_synth()
    e = synth._engine

    def quality():
        return (e.get_head_size(), e.get_breathiness(), e.get_roughness(), e.get_brightness())
    ids = [s if isinstance(s, str) else getattr(s, "id", None) for s in synth.supportedSettings]
    check("the four sliders are NVDA settings, after the volume",
          ids[-4:] == ["headSize", "breathiness", "roughness", "brightness"] and "volume" in ids
          and ids.index("volume") < ids.index("headSize"))
    check("they start at 50, whatever settings.json says",
          quality() == (50, 50, 50, 50)
          and (synth._get_headSize(), synth._get_breathiness(), synth._get_roughness(), synth._get_brightness())
          == (50, 50, 50, 50))
    plain = run(synth, ["Dobar dan", IndexCommand(1)])
    synth._set_headSize(20)
    synth._set_breathiness(80)
    synth._set_roughness(70)
    synth._set_brightness(10)
    check("the sliders reach the engine", quality() == (20, 80, 70, 10))
    changed = run(synth, ["Dobar dan", IndexCommand(2)])
    check("the voice quality changes the rendered speech",
          len(plain) == 1 and len(changed) == 1 and changed[0] != plain[0])
    synth._set_headSize(120)
    synth._set_brightness(-5)
    check("voice quality values outside 0-100 are clamped",
          quality() == (100, 80, 70, 0) and synth._get_headSize() == 100 and synth._get_brightness() == 0)
    write_settings("names", quality={"head_size": 30, "breathiness": 30, "roughness": 30, "brightness": 30})
    synth._checkConfigChanged()
    check("the voice quality keys of settings.json are SAPI5's and leave NVDA's values alone",
          quality() == (100, 80, 70, 0))
    synth._set_voice("stojan")
    check("a voice change keeps the voice quality", quality() == (100, 80, 70, 0))
    synth.terminate()


def test_delayed_description():
    write_settings("sounds")
    synth = new_synth()
    S.synthIndexReached.calls.clear()
    out1 = run(synth, [CharacterModeCommand(True), "b", CharacterModeCommand(False), IndexCommand(1)])
    out2 = run(synth, [BreakCommand(1000), "Biokovo", IndexCommand(2)])
    desc = spoken(synth, "Biokovo")
    check("spelled character is played without its trailing pause",
          len(out1) == 1 and out1[0] == spelled(synth, "b"))
    check("the break is a second of silence and the description follows",
          len(out2) == 2 and len(out2[0]) == 22050 * 2 and out2[1] == desc)
    check("both indexes were reported",
          [c["index"] for c in S.synthIndexReached.calls] == [1, 2])
    synth.terminate()


def _silences(data, min_ms):
    """Lengths in ms of the silences of at least min_ms inside the audio."""
    samples = struct.unpack("<%dh" % (len(data) // 2), data)
    runs, start = [], None
    for k, v in enumerate(samples):
        if abs(v) < 40:
            if start is None:
                start = k
        else:
            if start is not None and start > 0 and (k - start) * 1000 // 22050 >= min_ms:
                runs.append((k - start) * 1000 // 22050)
            start = None
    return runs


def test_items_and_phone_numbers():
    """What NVDA really sends (a listener's report of November 2026): every
    item of a sequence ends with two spaces, an Excel cell is its contents and
    its address as two items, and at the punctuation level "none" the plus
    sign of a telephone number is a space."""
    write_settings("sounds")
    synth = new_synth()
    cell = b"".join(run(synth, ["Marjan  ", "A17  ", IndexCommand(1)]))
    check("a cell's address comes after the whole comma pause",
          cell == spoken(synth, "Marjan A17") and len(_silences(cell, 100)) == 1)
    # The driver's log in Excel: [0] Text: 'Marijan  '  [1] Text: 'A 3  '
    excel = b"".join(run(synth, ["Marijan  ", "A 3  ", IndexCommand(3)]))
    check("Excel's address, said as two words, is the same address",
          excel == spoken(synth, "Marijan A3") and len(_silences(excel, 100)) == 1)
    empty = b"".join(run(synth, ["A 5  ", IndexCommand(4)]))
    check("the address of an empty cell is a letter and a number", empty == spoken(synth, "A5"))
    for text in (" 385 91 123 4567  ", " 385911234567  ", "plus 385 91 1234567  "):
        phone = b"".join(run(synth, [text, IndexCommand(2)]))
        check("the groups of %r are set off by comma pauses" % text, len(_silences(phone, 100)) == 3)
    synth.terminate()


def test_settings_order():
    ids = [getattr(s, "id", s) for s in drv.SynthDriver.supportedSettings]
    check("the Language setting comes before the Voice setting",
          ids[:2] == ["language", "voice"])


if __name__ == "__main__":
    test_settings_order()
    test_spelling_mode()
    test_capital_pitch(force_pitch=False)
    test_capital_pitch(force_pitch=True)
    test_rate_and_volume()
    test_volume_boost()
    test_voice_quality()
    test_delayed_description()
    test_items_and_phone_numbers()
    print("%d failed" % len(failures))
    sys.exit(1 if failures else 0)
