#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Converts the generated tables of src/formant/formant_slovenian.inc (the
C++ engine's) into the data files of the Rust engine,
src/common/languages/sl/tables/.

    python3 tools/formant/slovenian_tables_from_inc.py [formant_slovenian.inc [tables dir]]

The files, all UTF-8 text with LF line ends, embedded with include_str! and
searched in place:

Front-coded tables (forms.txt, stems.txt, names.txt, names_inside.txt,
head_imperatives.txt): entries in the notation of the Slovenian lexicon,
sorted by the spelling they are for (the letters without the marks, ə as e,
w and ł as l). One line is one block of 16 entries. The first entry of a
line is written whole; every other one begins with one byte 0x21 + n ('!'
for 0 ... '`' for 63) for the n leading bytes it shares with the entry
before it (whole characters), followed by the rest of its bytes. The entries themselves have
only bytes of 0x61 and above, so a byte below 0x61 always opens an entry.
(The .inc has the same blocks as C strings, with the byte 1 + n.)

Sorted lines (final_l.txt, given_names.txt, definite_adjectives.txt,
food.txt, verb_noun.txt), for crate::text::table: `key` or `key<TAB>value`,
sorted by the key in code point order.
    final_l.txt, given_names.txt, definite_adjectives.txt: the spelling.
    food.txt: spelling <TAB> cases (1 nominative | 2 accusative | 4 genitive).
    verb_noun.txt: spelling <TAB> the verb's reading <TAB> the other reading
        <TAB> flags: "l" when the verb's reading is the l-participle, then
        the share in percent of a genitive plural in -l when there is one.

neuter_o.txt and noun_a.txt are written by the numerals' step and are not
touched here.
"""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEFAULT_INC = os.path.join(ROOT, "src", "formant", "formant_slovenian.inc")
DEFAULT_OUT = os.path.join(ROOT, "src", "common", "languages", "sl", "tables")

BLOCK = 16
SHARED_BASE = 0x21          # the byte of an entry that shares nothing with the one before
FIRST_ENTRY_BYTE = 0x61     # every byte of an entry is this or more

# The notation's letters and the plain letters they stand for (notation_letter()
# of formant_slovenian.cpp); compare_key() skips * ' : and the combining grave
PLAIN = {
    "á": "a", "à": "a", "é": "e", "ê": "e", "è": "e", "í": "i", "ì": "i", "ó": "o", "ô": "o", "ò": "o",
    "ú": "u", "ù": "u", "ŕ": "r", "ə": "e", "w": "l", "ł": "l",
}
SKIPPED = {"*", "̀", "'", ":"}


def plain_key(entry):
    """The spelling an entry is for, as the engine's compare_key() reads it."""
    return "".join(PLAIN.get(c, c) for c in entry if c not in SKIPPED)


STRING = re.compile(r'(?:u8)?"((?:[^"\\]|\\.)*)"')
SIMPLE_ESCAPES = {"n": 10, "t": 9, "r": 13, "\\": 92, '"': 34, "'": 39, "0": 0, "a": 7, "b": 8, "f": 12, "v": 11}


def c_string_bytes(literal):
    """The bytes of the body of a C string literal (UTF-8 source, octal and simple escapes)."""
    out = bytearray()
    i = 0
    while i < len(literal):
        c = literal[i]
        if c != "\\":
            out += c.encode("utf-8")
            i += 1
            continue
        i += 1
        c = literal[i]
        if c in "01234567":
            j = i
            while j < len(literal) and j < i + 3 and literal[j] in "01234567":
                j += 1
            out.append(int(literal[i:j], 8))
            i = j
        elif c == "x":
            j = i + 1
            while j < len(literal) and literal[j] in "0123456789abcdefABCDEF":
                j += 1
            out.append(int(literal[i + 1:j], 16))
            i = j
        else:
            out.append(SIMPLE_ESCAPES[c])
            i += 1
    return bytes(out)


def array_body(source, name):
    """The text between the braces of `name[] = { ... };`."""
    match = re.search(r"\b" + re.escape(name) + r"\[\]\s*=\s*\{", source)
    if not match:
        raise SystemExit("no table %s in the .inc" % name)
    end = source.index("\n};", match.end())
    return source[match.end():end]


def constant(source, name):
    return int(re.search(r"\b" + re.escape(name) + r"\s*=\s*(\d+)\s*;", source).group(1))


def block_entries(block):
    """The entries of one front-coded block of the .inc, as bytes."""
    entries = []
    entry = b""
    i = 0
    while i < len(block):
        if 1 <= block[i] <= 64:
            entry = entry[:block[i] - 1]
            i += 1
        else:
            entry = b""
        j = i
        while j < len(block) and block[j] > 64:
            j += 1
        entry += block[i:j]
        entries.append(entry)
        i = j
    return entries


def shared_prefix(a, b):
    n = 0
    while n < len(a) and n < len(b) and n < 63 and a[n] == b[n]:
        n += 1
    # Whole characters only, so that the file is text
    while n > 0 and ((n < len(b) and b[n] & 0xC0 == 0x80) or (n < len(a) and a[n] & 0xC0 == 0x80)):
        n -= 1
    return n


def front_coded(source, name, count_name, out_path):
    body = array_body(source, name)
    # One array element is one block; adjacent literals of one element are joined
    blocks = []
    for element in re.split(r",\s*\n", body):
        literals = STRING.findall(element)
        if literals:
            blocks.append(b"".join(c_string_bytes(literal) for literal in literals))
    entries = [entry for block in blocks for entry in block_entries(block)]
    count = constant(source, count_name)
    if len(entries) != count:
        raise SystemExit("%s: %d entries, %s says %d" % (name, len(entries), count_name, count))
    for block in blocks[:-1]:
        if len(block_entries(block)) != BLOCK:
            raise SystemExit("%s: a block without %d entries" % (name, BLOCK))
    keys = [plain_key(entry.decode("utf-8")) for entry in entries]
    for before, after in zip(keys, keys[1:]):
        if not before < after:
            raise SystemExit("%s: %r is not before %r" % (name, before, after))
    lines = []
    for start in range(0, len(entries), BLOCK):
        line = bytearray()
        previous = None
        for entry in entries[start:start + BLOCK]:
            if min(entry) < FIRST_ENTRY_BYTE:
                raise SystemExit("%s: %r has a byte below 0x61" % (name, entry))
            if previous is None:
                line += entry
            else:
                shared = shared_prefix(previous, entry)
                line.append(SHARED_BASE + shared)
                line += entry[shared:]
            previous = entry
        lines.append(bytes(line))
    data = b"\n".join(lines) + b"\n"
    data.decode("utf-8")        # the file is text
    # Read it back as the engine does
    back = []
    for line in data.split(b"\n")[:-1]:
        entry = b""
        i = 0
        while i < len(line):
            if line[i] < FIRST_ENTRY_BYTE:
                entry = entry[:line[i] - SHARED_BASE]
                i += 1
            j = i
            while j < len(line) and line[j] >= FIRST_ENTRY_BYTE:
                j += 1
            entry += line[i:j]
            back.append(entry)
            i = j
    if back != entries:
        raise SystemExit("%s: the written table does not read back" % name)
    write(out_path, data)
    return len(entries)


def write(path, data):
    with open(path, "wb") as f:
        f.write(data)
    print("%-28s %8d bytes" % (os.path.basename(path), len(data)))


def sorted_lines(name, rows, out_path):
    """rows: (key, value) pairs; the keys as the engine compares them."""
    keys = [key for key, _ in rows]
    if keys != sorted(keys) or len(set(keys)) != len(keys):
        print("note: %s was not in strict code point order by its plain keys in the .inc" % name)
    table = {}
    for key, value in rows:
        table.setdefault(key, value)        # the binary search of the .inc could find either: keep the first
    lines = ["%s\t%s" % (key, table[key]) if table[key] else key for key in sorted(table)]
    for line in lines:
        if "\n" in line or "\r" in line:
            raise SystemExit("%s: a line break in %r" % (name, line))
    write(out_path, ("\n".join(lines) + "\n").encode("utf-8"))
    return len(lines)


def plain_rows(name, spellings, exact):
    """Spellings as keys. `exact`: the engine compares the string as it is;
    otherwise through compare_key(), which reads the notation's letters."""
    rows = []
    for spelling in spellings:
        key = spelling if exact else plain_key(spelling)
        if key != spelling:
            print("note: %s: %r is looked up as %r" % (name, spelling, key))
        rows.append((key, ""))
    return rows


def strings_of(source, name):
    return [c_string_bytes(literal).decode("utf-8") for literal in STRING.findall(array_body(source, name))]


def main():
    inc = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_INC
    out = sys.argv[2] if len(sys.argv) > 2 else DEFAULT_OUT
    with open(inc, encoding="utf-8") as f:
        source = f.read()
    if constant(source, "SLOVENIAN_BLOCK") != BLOCK:
        raise SystemExit("the block size is not %d" % BLOCK)
    os.makedirs(out, exist_ok=True)

    for name, count_name, filename in [
        ("SLOVENIAN_FORMS", "SLOVENIAN_FORM_COUNT", "forms.txt"),
        ("SLOVENIAN_STEMS", "SLOVENIAN_STEM_COUNT", "stems.txt"),
        ("SLOVENIAN_NAMES", "SLOVENIAN_NAMES_COUNT", "names.txt"),
        ("SLOVENIAN_NAMES_INSIDE", "SLOVENIAN_NAMES_INSIDE_COUNT", "names_inside.txt"),
        ("SLOVENIAN_HEAD_IMPERATIVES", "SLOVENIAN_HEAD_IMPERATIVES_COUNT", "head_imperatives.txt"),
    ]:
        print("  %d entries" % front_coded(source, name, count_name, os.path.join(out, filename)))

    for name, filename, exact in [
        ("SLOVENIAN_FINAL_L", "final_l.txt", True),
        ("SLOVENIAN_GIVEN_NAMES", "given_names.txt", False),
        ("SLOVENIAN_DEFINITE_ADJECTIVES", "definite_adjectives.txt", False),
    ]:
        rows = plain_rows(name, strings_of(source, name), exact)
        print("  %d entries" % sorted_lines(name, rows, os.path.join(out, filename)))

    food = re.findall(r'\{\s*(?:u8)?"((?:[^"\\]|\\.)*)"\s*,\s*(\d+)\s*\}', array_body(source, "SLOVENIAN_FOOD"))
    rows = [(plain_key(c_string_bytes(spelling).decode("utf-8")), cases) for spelling, cases in food]
    print("  %d entries" % sorted_lines("SLOVENIAN_FOOD", rows, os.path.join(out, "food.txt")))

    verb_noun = re.findall(
        r'\{\s*(?:u8)?"((?:[^"\\]|\\.)*)"\s*,\s*(?:u8)?"((?:[^"\\]|\\.)*)"\s*,\s*(true|false)\s*,\s*(-?\d+)\s*\}',
        array_body(source, "SLOVENIAN_VERB_NOUN"))
    rows = []
    for verb, nominal, participle, percent in verb_noun:
        verb = c_string_bytes(verb).decode("utf-8")
        nominal = c_string_bytes(nominal).decode("utf-8")
        flags = ("l" if participle == "true" else "") + (percent if int(percent) >= 0 else "")
        rows.append((plain_key(verb), "%s\t%s\t%s" % (verb, nominal, flags)))
    print("  %d entries" % sorted_lines("SLOVENIAN_VERB_NOUN", rows, os.path.join(out, "verb_noun.txt")))


if __name__ == "__main__":
    main()
