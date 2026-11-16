#!/usr/bin/env python3
"""Collect the golden corpus: every string literal of the C++ tests.

    python tools/golden/corpus.py                 # writes tests/golden/corpus.jsonl
    python tools/golden/corpus.py --plain FILE    # the same texts for dump.cpp

One JSON string per line, sorted, without duplicates. The corpus is the input
of golden.py, which records what the engine makes of each text.
"""
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
SOURCES = ["tests/linux/test_formant.cpp", "tests/linux/test_concat.cpp",
           "tests/linux/test_frontend.cpp", "tests/linux/test_numbers.cpp",
           "tests/linux/spelling_checks.hpp", "tests/test_features.cpp"]
LITERAL = re.compile(r'(?:u8)?"((?:[^"\\\n]|\\.)*)"')
SIMPLE = {"n": 10, "t": 9, "r": 13, "0": 0, "\\": 92, '"': 34, "'": 39, "a": 7, "?": 63}


def decode(body: str):
    """The text of a C++ literal's body, or None when it is not UTF-8."""
    out, i = bytearray(), 0
    while i < len(body):
        c = body[i]
        if c != "\\":
            out += c.encode("utf-8")
            i += 1
            continue
        e = body[i + 1]
        if e == "x":
            m = re.match(r"[0-9a-fA-F]{1,2}", body[i + 2:])
            out.append(int(m.group(), 16))
            i += 2 + len(m.group())
        elif e in "uU":
            n = 4 if e == "u" else 8
            out += chr(int(body[i + 2:i + 2 + n], 16)).encode("utf-8")
            i += 2 + n
        elif e in SIMPLE:
            out.append(SIMPLE[e])
            i += 2
        else:
            return None
    try:
        return out.decode("utf-8")
    except UnicodeDecodeError:
        return None


def plain(path):
    """The corpus as one text per line with \\n, \\t, \\r and \\\\ escaped, for dump.cpp."""
    texts = [json.loads(line) for line in (ROOT / "tests/golden/corpus.jsonl").read_text(encoding="utf-8").splitlines()]
    escapes = str.maketrans({"\\": "\\\\", "\n": "\\n", "\t": "\\t", "\r": "\\r"})
    pathlib.Path(path).write_text("".join(t.translate(escapes) + "\n" for t in texts), encoding="utf-8", newline="\n")


def main():
    if len(sys.argv) == 3 and sys.argv[1] == "--plain":
        return plain(sys.argv[2])
    texts = set()
    for source in SOURCES:
        for line in (ROOT / source).read_text(encoding="utf-8").splitlines():
            if line.lstrip().startswith("#include"):
                continue
            for body in LITERAL.findall(line):
                text = decode(body)
                if text and "\0" not in text and any(ch.isalnum() for ch in text):
                    texts.add(text)
    out = ROOT / "tests/golden/corpus.jsonl"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("".join(json.dumps(t, ensure_ascii=False) + "\n" for t in sorted(texts)),
                   encoding="utf-8", newline="\n")
    print(f"{len(texts)} texts -> {out}")


if __name__ == "__main__":
    main()
