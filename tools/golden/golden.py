#!/usr/bin/env python3
"""Record or check the golden audio of the engine through its C API.

    python tools/golden/golden.py record <library> <data dir> <dictionary dir>
    python tools/golden/golden.py check  <library> <data dir> <dictionary dir> [set ...]

`record` renders tests/golden/corpus.jsonl (see corpus.py) in the sets below
and writes tests/golden/<set>.tsv: voice, language, settings, mode, the text's
line in the corpus, the number of samples and a hash of them. `check` renders
again and reports every line that differs. The reference was recorded from the
C++ engine on Windows x64 before the port to Rust.
"""
import ctypes
import hashlib
import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
GOLDEN = ROOT / "tests/golden"
LANGUAGES = ["hr", "sr", "bs", "sl"]
VOICES = [("josip", "hr"), ("vlado", "sr"), ("detence", "hr"), ("baba", "hr"), ("djedo", "sr"),
          ("zvonko", "hr"), ("stojan", "sr"), ("mirsad", "bs"), ("orguljas", "hr"),
          ("dalmacija_klapa", "hr"), ("trubac", "sr"), ("harmonikas", "sr"), ("sevdalija", "bs"),
          ("sazlija", "bs"), ("lika_velebit", "hr"), ("solunac", "sr"), ("sarajevo_bembasa", "bs"),
          ("becarac", "hr")]
FORMANT_SETTINGS = ["speed=0.5", "speed=2", "speed=4", "user_pitch=0.5", "user_pitch=2",
                    "inflection_level=0", "inflection_level=1", "acceleration=3", "volume=0.5",
                    "volume_boost=1", "head_size=0", "head_size=100", "breathiness=0",
                    "breathiness=100", "roughness=0", "roughness=100", "brightness=0",
                    "brightness=100"]
RECORDED_SETTINGS = ["speed=0.5", "speed=2", "user_pitch=0.5", "user_pitch=2", "inflection_level=0",
                     "acceleration=3", "volume_boost=1", "head_size=0", "head_size=100",
                     "brightness=0", "brightness=100"]
INTEGER = {"volume_boost", "head_size", "breathiness", "roughness", "brightness",
           "spelling_mode", "spelling_speed", "number_mode"}


def sets(texts):
    """name -> list of (voice, language, settings, mode, text index)."""
    every = lambda n: range(0, len(texts), n)
    digits = [i for i, t in enumerate(texts) if any(c.isdigit() for c in t)]
    short = [i for i, t in enumerate(texts) if len(t) <= 40]
    return {
        "formant": [("zvonko", l, "", "text", i) for l in LANGUAGES for i in every(1)],
        "recorded": [(v, l, "", "text", i) for v, l in VOICES[:2] for i in every(1)]
                    + [(v, l, "", "text", i) for v, l in (("josip", "sl"), ("vlado", "bs")) for i in every(8)],
        "voices": [(v, l, "", "text", i) for v, l in VOICES for i in every(25)],
        "digits": [("zvonko", l, "number_mode=1", "text", i) for l in LANGUAGES for i in digits],
        "spelled": [("zvonko", l, f"spelling_mode={m}", "spelled", i)
                    for l in LANGUAGES for m in (0, 1) for i in short[::10]]
                   + [("josip", "hr", f"spelling_mode={m}", "spelled", i) for m in (0, 1) for i in short[::40]]
                   + [("zvonko", l, "spelling_speed=0", "spelled", i) for l in LANGUAGES for i in short[::100]],
        "names": [("zvonko", l, "spelling_mode=0", "text", i) for l in LANGUAGES for i in every(10)],
        "settings": [("zvonko", "hr", s, "text", i) for s in FORMANT_SETTINGS for i in every(100)]
                    + [("josip", "hr", s, "text", i) for s in RECORDED_SETTINGS for i in every(100)],
    }


class Engine:
    def __init__(self, library, data_dir, dictionary_dir):
        self.lib = ctypes.WinDLL(library) if sys.platform == "win32" else ctypes.CDLL(library)
        self.data_dir, self.dictionary_dir = data_dir.encode(), pathlib.Path(dictionary_dir)
        lib, c = self.lib, ctypes
        lib.laprdus_create.restype = c.c_void_p
        lib.laprdus_destroy.argtypes = [c.c_void_p]
        for name in ("set_voice", "set_language", "load_dictionary", "load_spelling_dictionary",
                     "load_emoji_dictionary"):
            getattr(lib, "laprdus_" + name).argtypes = [c.c_void_p, c.c_char_p] + [c.c_char_p] * (name == "set_voice")
        for name in ("synthesize", "synthesize_spelled"):
            getattr(lib, "laprdus_" + name).argtypes = [
                c.c_void_p, c.c_char_p, c.POINTER(c.POINTER(c.c_int16)), c.c_void_p]
        lib.laprdus_free_buffer.argtypes = [c.POINTER(c.c_int16)]
        self.handle, self.config = None, None

    def configure(self, voice, language, settings):
        if (voice, language, settings) == self.config:
            return
        lib = self.lib
        if self.handle:
            lib.laprdus_destroy(self.handle)
        h = self.handle = lib.laprdus_create()
        assert lib.laprdus_set_voice(h, voice.encode(), self.data_dir) == 0, voice
        assert lib.laprdus_set_language(h, language.encode()) == 0, language
        lib.laprdus_load_dictionary(h, str(self.dictionary_dir / "internal.json").encode())
        lib.laprdus_load_spelling_dictionary(h, str(self.dictionary_dir / "spelling.json").encode())
        lib.laprdus_load_emoji_dictionary(h, str(self.dictionary_dir / "emoji.json").encode())
        for setting in filter(None, settings.split(",")):
            name, value = setting.split("=")
            setter = getattr(lib, "laprdus_set_" + name)
            integer = name in INTEGER
            setter.argtypes = [ctypes.c_void_p, ctypes.c_int if integer else ctypes.c_float]
            assert setter(h, int(value) if integer else float(value)) == 0, setting
        self.config = (voice, language, settings)

    def render(self, mode, text):
        """(sample count or error code, hash of the samples)."""
        samples = ctypes.POINTER(ctypes.c_int16)()
        fmt = (ctypes.c_uint32 * 2)()
        call = self.lib.laprdus_synthesize_spelled if mode == "spelled" else self.lib.laprdus_synthesize
        n = call(self.handle, text.encode("utf-8"), ctypes.byref(samples), fmt)
        if n <= 0 or not samples:
            return n, "-"
        digest = hashlib.blake2b(ctypes.string_at(samples, n * 2), digest_size=8).hexdigest()
        self.lib.laprdus_free_buffer(samples)
        return n, digest


def main():
    command, library, data_dir, dictionary_dir, *only = sys.argv[1:]
    texts = [json.loads(line) for line in (GOLDEN / "corpus.jsonl").read_text(encoding="utf-8").splitlines()]
    engine = Engine(str(pathlib.Path(library).resolve()), data_dir, dictionary_dir)
    failed = 0
    for name, cases in sets(texts).items():
        if only and name not in only:
            continue
        lines = []
        for voice, language, settings, mode, index in cases:
            engine.configure(voice, language, settings)
            n, digest = engine.render(mode, texts[index])
            lines.append(f"{voice}\t{language}\t{settings}\t{mode}\t{index + 1}\t{n}\t{digest}")
        path = GOLDEN / f"{name}.tsv"
        if command == "record":
            path.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
            print(f"{name}: {len(lines)} renderings recorded")
            continue
        wrong = [(old, new) for old, new in zip(path.read_text(encoding="utf-8").splitlines(), lines) if old != new]
        failed += len(wrong)
        print(f"{name}: {len(lines) - len(wrong)} of {len(lines)} match")
        for old, new in wrong[:20]:
            index = int(old.split("\t")[4])
            print(f"  {json.dumps(texts[index - 1], ensure_ascii=False)}\n    was {old}\n    is  {new}")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
