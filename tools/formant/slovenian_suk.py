#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Measure a rule of the Slovenian front end on the tagged corpus SUK 1.1.

Source: SUK 1.1 (Arhar Holdt, Krek, Dobrovoljc, Erjavec, Gantar, Čibej,
Pori, Terčon, Munda, Žitnik, Robida, Blagus, Može, Ledinek, Holz, Zupan,
Kuzman, Kavčič, Škrjanec, Marko, Jezeršek, Zajc; CLARIN.SI,
http://hdl.handle.net/11356/1959), its *.jos.conllu files (lemma and
MULTEXT-East/JOS tag in Slovenian for every token).

    python3 tools/formant/slovenian_suk.py read SUK_DIR FORMS OUT.tsv [EXE]
    python3 tools/formant/slovenian_suk.py score ROWS.tsv EXPECTED.tsv
    python3 tools/formant/slovenian_suk.py moved OLD.tsv NEW.tsv

`read` takes every sentence with one of the spellings FORMS (comma-separated,
or @FILE with one per line; any case), runs it through the engine
(examples/slovenian_stress.rs, built with `cargo build --release --example
slovenian_stress`; EXE names another build) and writes one row per token:

    spelling  lemma  tag  phones  sentence id  token number  sentence

with the phones in the notation of the example. Only sentences whose tokens
are words and punctuation are read (a number or a symbol becomes several
words in the engine, so its words could not be counted off); the count of
those left out is printed.

`score` counts the rows whose phones are the expected ones. EXPECTED.tsv
has a row per reading, the first that matches a token decides:

    spelling  lemma (regular expression)  tag (regular expression)  phones

and a token no row matches is not counted. `moved` lists the tokens two
runs of `read` read differently: every one of them is to be looked at.
"""
import glob
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
EXE = os.path.join(ROOT, 'target', 'release', 'examples', 'slovenian_stress' + ('.exe' if os.name == 'nt' else ''))
PUNCTUATION = set('.,;:!?…"\'()[]«»„“”‘’–—-/')


def sentences(suk_dir):
    """(sentence id, text, [(spelling, lemma, tag)]) of every sentence."""
    for path in sorted(glob.glob(os.path.join(suk_dir, '*.jos.conllu'))):
        sent_id = text = None
        tokens = []
        with open(path, encoding='utf-8') as f:
            for line in f:
                line = line.rstrip('\n')
                if line.startswith('# sent_id = '):
                    sent_id = line[12:]
                elif line.startswith('# text = '):
                    text = line[9:]
                elif not line:
                    if tokens:
                        yield sent_id, text, tokens
                    tokens = []
                elif not line.startswith('#'):
                    cols = line.split('\t')
                    if cols[0].isdigit():
                        tokens.append((cols[1], cols[2], cols[4]))
        if tokens:
            yield sent_id, text, tokens


def read(suk_dir, forms, out_path, exe):
    wanted = set(w.lower() for w in forms)
    picked = []
    left_out = 0
    for sent_id, text, tokens in sentences(suk_dir):
        if not any(t[0].lower() in wanted for t in tokens):
            continue
        if not all(t[0].isalpha() or all(c in PUNCTUATION for c in t[0]) for t in tokens) or '\t' in text:
            left_out += 1
            continue
        picked.append((sent_id, text, tokens))
    result = subprocess.run([exe], input='\n'.join(p[1] for p in picked) + '\n', capture_output=True,
                            encoding='utf-8', check=True)
    lines = result.stdout.split('\n')
    rows = miscounted = 0
    with open(out_path, 'w', encoding='utf-8') as out:
        for (sent_id, text, tokens), line in zip(picked, lines):
            phones = [w for w in line.rstrip('\r').split('\t')[-1].split(' ') if w and w != '|']
            words = [t for t in tokens if t[0].isalpha()]
            if len(phones) != len(words):
                miscounted += 1
                continue
            for k, ((spelling, lemma, tag), ph) in enumerate(zip(words, phones)):
                if spelling.lower() in wanted:
                    out.write('\t'.join([spelling, lemma, tag, ph, sent_id, str(k), text]) + '\n')
                    rows += 1
    print('sentences', len(picked), 'with a number or symbol (left out)', left_out,
          'words not counted off (left out)', miscounted, 'tokens', rows, file=sys.stderr)


def rows_of(path):
    with open(path, encoding='utf-8') as f:
        return [line.rstrip('\n').split('\t') for line in f if line.strip() and not line.startswith('#')]


def score(rows_path, expected_path):
    expected = [(r[0], re.compile(r[1]), re.compile(r[2]), r[3]) for r in rows_of(expected_path)]
    counts = {}
    wrong = []
    for row in rows_of(rows_path):
        spelling, lemma, tag, phones = row[:4]
        for form, lemma_re, tag_re, want in expected:
            if form == spelling.lower() and lemma_re.fullmatch(lemma) and tag_re.fullmatch(tag):
                c = counts.setdefault((form, want), [0, 0])
                c[1] += 1
                if phones == want:
                    c[0] += 1
                else:
                    wrong.append(row)
                break
    right = total = 0
    for (form, want), (r, t) in sorted(counts.items()):
        print('%s\t%s\t%d of %d' % (form, want, r, t))
        right += r
        total += t
    print('all\t\t%d of %d' % (right, total))
    for row in wrong:
        print('WRONG\t' + '\t'.join(row[:4] + row[6:]))


def moved(old_path, new_path):
    old = {(r[4], r[5]): r for r in rows_of(old_path)}
    for row in rows_of(new_path):
        before = old.get((row[4], row[5]))
        if before and before[3] != row[3]:
            print('\t'.join([row[0], row[1], row[2], before[3] + ' -> ' + row[3], row[6]]))


def main():
    args = sys.argv[1:]
    if len(args) >= 4 and args[0] == 'read':
        forms = args[2]
        if forms.startswith('@'):
            with open(forms[1:], encoding='utf-8') as f:
                forms = [line.strip() for line in f if line.strip()]
        else:
            forms = forms.split(',')
        read(args[1], forms, args[3], args[4] if len(args) > 4 else EXE)
    elif len(args) == 3 and args[0] == 'score':
        score(args[1], args[2])
    elif len(args) == 3 and args[0] == 'moved':
        moved(args[1], args[2])
    else:
        print(__doc__, file=sys.stderr)
        sys.exit(2)


if __name__ == '__main__':
    main()
