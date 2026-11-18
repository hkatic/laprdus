#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""The formats of the generated Slovenian tables of the engine,
src/common/languages/sl/tables/, which tools/formant/slovenian_words.py
writes through this module.

The files are UTF-8 text with LF line ends and nothing but entries (no
comments, no empty lines), embedded with include_str! and searched in place:

Front-coded tables (forms.txt, stems.txt, names.txt, names_inside.txt,
head_imperatives.txt, imperatives.txt; read by find_in_table() of sl/lexicon.rs): entries in
the notation of the Slovenian lexicon, sorted by the spelling they are for
(the letters without the marks, ə as e, w and ł as l: compare_key() of
sl/lexicon.rs reads the spelling off the notation while it searches). One
line is one block of 16 entries. The first entry of a line is written whole;
every other one begins with one byte 0x21 + n ('!' for 0 ... '`' for 63) for
the n leading bytes it shares with the entry before it (whole characters),
followed by the rest of its bytes. The entries themselves have only bytes of
0x61 and above, so a byte below 0x61 always opens an entry. The stems carry
no '*'.

Sorted lines (final_l.txt, given_names.txt, definite_adjectives.txt,
food.txt, verb_noun.txt, neuter_o.txt, noun_a.txt, finite_verbs.txt; read by lookup() and
contains() of src/common/text/table.rs): `key` or `key<TAB>value`, sorted by
the key in code point order; the key is the spelling, lowercase, as the word
stands in the clause.
    final_l.txt, given_names.txt, definite_adjectives.txt, neuter_o.txt,
    finite_verbs.txt: the spelling alone.
    food.txt: spelling <TAB> cases (1 nominative | 2 accusative | 4 genitive).
    noun_a.txt: spelling <TAB> kind (1 a masculine noun's animate accusative
        or a masculine or neuter noun's genitive singular, 2 a masculine
        noun's nominative).
    verb_noun.txt: spelling <TAB> the verb's reading <TAB> the other reading
        <TAB> flags: "l" when the verb's reading is the l-participle, then
        the share in percent of a genitive plural in -l when there is one.

    python3 tools/formant/slovenian_tables.py [tables dir]

reads every table of the folder (default: src/common/languages/sl/tables),
writes it again from its entries and reports whether the bytes are the same:
a check of this module against the files the engine has.
"""

import os
import sys
import unicodedata

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TABLES = os.path.join(ROOT, 'src', 'common', 'languages', 'sl', 'tables')

BLOCK = 16                  # entries per line of a front-coded table (find_in_table())
SHARED_BASE = 0x21          # the byte of an entry that shares nothing with the one before
FIRST_ENTRY_BYTE = 0x61     # every byte of an entry is this or more
MAX_SHARED = 63

FRONT_CODED = ('forms.txt', 'stems.txt', 'names.txt', 'names_inside.txt', 'head_imperatives.txt',
               'imperatives.txt')
SORTED_LINES = ('final_l.txt', 'given_names.txt', 'definite_adjectives.txt', 'food.txt',
                'verb_noun.txt', 'neuter_o.txt', 'noun_a.txt', 'finite_verbs.txt')

# The notation's letters and the plain letters they stand for (notation_letter()
# of sl/lexicon.rs); compare_key() skips * ' : and the combining grave
PLAIN = {
    'á': 'a', 'à': 'a', 'é': 'e', 'ê': 'e', 'è': 'e', 'í': 'i', 'ì': 'i', 'ó': 'o', 'ô': 'o', 'ò': 'o',
    'ú': 'u', 'ù': 'u', 'ŕ': 'r', 'ə': 'e', 'w': 'l', 'ł': 'l',
}
SKIPPED = {'*', '̀', "'", ':'}


def plain_key(entry):
    """The spelling an entry is for, as the engine's compare_key() reads it."""
    return ''.join(PLAIN.get(c, c) for c in entry if c not in SKIPPED)


def code_points(key):
    return [ord(c) for c in key]


def shared_prefix(a, b):
    """The leading bytes two entries share, whole characters only, so that
    the file is text."""
    n = 0
    while n < len(a) and n < len(b) and n < MAX_SHARED and a[n] == b[n]:
        n += 1
    while n > 0 and ((n < len(b) and b[n] & 0xC0 == 0x80) or (n < len(a) and a[n] & 0xC0 == 0x80)):
        n -= 1
    return n


def decode_front_coded(data):
    """The entries of a front-coded table, as the engine reads them."""
    entries = []
    for line in data.split(b'\n')[:-1]:
        entry = b''
        i = 0
        while i < len(line):
            if line[i] < FIRST_ENTRY_BYTE:
                entry = entry[:line[i] - SHARED_BASE]
                i += 1
            j = i
            while j < len(line) and line[j] >= FIRST_ENTRY_BYTE:
                j += 1
            entry += line[i:j]
            entries.append(entry)
            i = j
    return [entry.decode('utf-8') for entry in entries]


def encode_front_coded(name, notes):
    """The bytes of a front-coded table of entries in the notation, in any
    order."""
    notes = sorted((unicodedata.normalize('NFC', note) for note in notes),
                   key=lambda note: code_points(plain_key(note)))
    keys = [plain_key(note) for note in notes]
    for before, after in zip(keys, keys[1:]):
        if not before < after:
            raise SystemExit('%s: %r is not before %r' % (name, before, after))
    entries = [note.encode('utf-8') for note in notes]
    lines = []
    for start in range(0, len(entries), BLOCK):
        line = bytearray()
        previous = None
        for entry in entries[start:start + BLOCK]:
            if not entry or min(entry) < FIRST_ENTRY_BYTE:
                raise SystemExit('%s: %r is empty or has a byte below 0x61' % (name, entry))
            if previous is None:
                line += entry
            else:
                shared = shared_prefix(previous, entry)
                line.append(SHARED_BASE + shared)
                line += entry[shared:]
            previous = entry
        lines.append(bytes(line))
    data = b''.join(line + b'\n' for line in lines)
    data.decode('utf-8')        # the file is text
    if decode_front_coded(data) != notes:
        raise SystemExit('%s: the table does not read back' % name)
    return data


def decode_sorted_lines(data):
    """(key, value) pairs of a table of sorted lines; the value '' for a key
    alone."""
    rows = []
    for line in data.decode('utf-8').split('\n')[:-1]:
        key, _tab, value = line.partition('\t')
        rows.append((key, value))
    return rows


def encode_sorted_lines(name, rows):
    """The bytes of a table of lines `key` or `key<TAB>value`, from (key,
    value) pairs in any order; the value '' for a key alone."""
    table = {}
    for key, value in rows:
        value = unicodedata.normalize('NFC', str(value))
        key = unicodedata.normalize('NFC', key)
        if not key or '\t' in key or '\n' in key or '\r' in key or '\n' in value or '\r' in value:
            raise SystemExit('%s: %r is not a key and a value of one line' % (name, (key, value)))
        if table.setdefault(key, value) != value:
            raise SystemExit('%s: two values for %r' % (name, key))
    lines = ['%s\t%s' % (key, table[key]) if table[key] else key for key in sorted(table, key=code_points)]
    return ''.join(line + '\n' for line in lines).encode('utf-8')


def write(out_dir, filename, data):
    os.makedirs(out_dir, exist_ok=True)
    with open(os.path.join(out_dir, filename), 'wb') as f:
        f.write(data)
    print('%-28s %8d bytes' % (filename, len(data)), file=sys.stderr)


def write_front_coded(out_dir, filename, notes):
    """notes: the entries in the notation (the values of a spelling ->
    notation table)."""
    notes = list(notes)
    write(out_dir, filename, encode_front_coded(filename, notes))
    return len(notes)


def write_sorted_lines(out_dir, filename, rows):
    """rows: spellings, or (spelling, value) pairs."""
    rows = [(row, '') if isinstance(row, str) else row for row in rows]
    write(out_dir, filename, encode_sorted_lines(filename, rows))
    return len(rows)


def verb_noun_value(verb, nominal, participle, percent):
    """The value of a line of verb_noun.txt (readings.rs reads it)."""
    flags = ('l' if participle else '') + (str(percent) if percent >= 0 else '')
    return '%s\t%s\t%s' % (verb, nominal, flags)


def check(tables_dir):
    """Every table of the folder written again from its entries: are the
    bytes the same?"""
    ok = True
    for filename in FRONT_CODED + SORTED_LINES:
        with open(os.path.join(tables_dir, filename), 'rb') as f:
            data = f.read()
        if filename in FRONT_CODED:
            entries = decode_front_coded(data)
            again = encode_front_coded(filename, reversed(entries))
        else:
            entries = decode_sorted_lines(data)
            again = encode_sorted_lines(filename, reversed(entries))
        same = again == data
        ok &= same
        print('%-28s %6d entries  %s' % (filename, len(entries), 'identical' if same else 'DIFFERENT'))
    return ok


if __name__ == '__main__':
    sys.exit(0 if check(sys.argv[1] if len(sys.argv) > 1 else TABLES) else 1)
