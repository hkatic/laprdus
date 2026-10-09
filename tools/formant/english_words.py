#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
english_words.py - The English table of the formant voices' front end.

English words in Croatian, Serbian or Bosnian text are read with the letters'
own sounds but stressed where English stresses them (see "English words" in
src/formant/formant_frontend.cpp and in docs/formant.md). The front end finds
the stress of most of them by its English rules; this script writes
src/formant/formant_english.inc, the table of the frequent words for which
those rules, or the rules for native words, give another syllable.

How the table is made:

1. The most frequent English words (the wordfreq package, --words of them)
   that are written with the letters a-z only, are in the Carnegie Mellon
   University Pronouncing Dictionary (cmudict.dict) and are not words of
   Croatian, Serbian or Bosnian: the Hunspell dictionaries hr_HR, sr-Latn
   and bs_BA (from LibreOffice) accept none of them, so no native word
   (general, hotel, more, time) ever gets an English stress. The rows of
   tools/formant/english_extra.tsv are added: names and words the
   pronouncing dictionary does not have (Ableton), or has otherwise.
2. The dictionary's vowels are aligned with the vowel letters of the
   spelling, which gives the letter of the primary stress (cro'atian,
   e'leven, comp'uter). The voices read every vowel letter as a syllable,
   so that letter is all the front end needs.
3. tools/formant/english_stress.cpp, built against the front end with an
   empty table, tells for each word where the voices stress it without the
   table. A word goes into the table when that is another syllable, or
   when the syllable is right but carries the rising accent of a native
   word (only English words have the plain accent of the table off the
   first syllable), unless the front end already finds it through a
   shorter word of the table and an ending that keeps the stress
   (english_word() in formant_frontend.cpp, mirrored by lookup() below).
4. Every spelling that reaches a word of the table through a Croatian
   case ending (manager: managera, manageru, managerom) is checked with
   the Hunspell dictionaries, and those that are native words (reforma,
   republici, nadala: reform, republic, Nadal) go into the table without
   a mark, which keeps them native.

Words spelled like a place or personal name of the native tables, also
without the j of -ija/-ije/-iju (Sofia, Emilia), are left out. The script
ends by building the helper again with the new table and checking every
word, takes out the words a native lexicon entry overrides, and reports
anything the front end still reads otherwise than expected.

Requirements: a C++17 compiler (c++), and

    pip install wordfreq spylls

cmudict.dict from https://github.com/cmusphinx/cmudict, and hr_HR.dic/.aff,
sr-Latn.dic/.aff and bs_BA.dic/.aff from
https://github.com/LibreOffice/dictionaries (hr_HR/, sr/, bs_BA/) in one
directory.

Usage:
    tools/formant/english_words.py CMUDICT HUNSPELL_DIR [--words 30000]
"""

import argparse
import json
import os
import re
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUTPUT = os.path.join(ROOT, 'src', 'formant', 'formant_english.inc')
EXTRA = os.path.join(ROOT, 'tools', 'formant', 'english_extra.tsv')
HELPER = os.path.join(ROOT, 'tools', 'formant', 'english_stress.cpp')
HELPER_SOURCES = [
    'src/formant/formant_lexicon.cpp',
    'src/formant/formant_phonemes.cpp',
    'src/core/phoneme_mapper.cpp',
]

# ---------------------------------------------------------------------------
# Stress letter from the pronouncing dictionary
# ---------------------------------------------------------------------------

VOWEL_LETTERS = set('aeiouy')
VOWEL_PHONES = {'AA', 'AE', 'AH', 'AO', 'AW', 'AY', 'EH', 'ER', 'EY', 'IH', 'IY',
                'OW', 'OY', 'UH', 'UW'}
# The vowels a letter or a run of vowel letters usually spells
SINGLE = {
    'a': 'AE EY AA AH AO EH IH', 'e': 'EH IY IH AH ER EY', 'i': 'IH AY IY AH ER',
    'o': 'AA OW AO AH UW UH ER AW', 'u': 'AH UW UH ER IH', 'y': 'IY AY IH',
}
RUNS = {
    'ai': 'EY EH AY', 'ay': 'EY', 'au': 'AO AA', 'ea': 'IY EH EY', 'ee': 'IY',
    'ei': 'EY IY AY', 'ey': 'IY EY', 'eu': 'UW', 'ie': 'IY AY', 'oa': 'OW',
    'oe': 'OW UW', 'oi': 'OY', 'oy': 'OY', 'oo': 'UW UH', 'ou': 'AW AH UW OW AO UH ER',
    'ue': 'UW', 'ui': 'UW IH AY', 'ia': 'AH', 'io': 'AH', 'eo': 'AH IY', 'iou': 'AH',
    'eou': 'AH', 'ye': 'AY', 'uy': 'AY', 'eye': 'AY', 'aye': 'EY', 'ae': 'IY EH',
    'ua': 'AH', 'iu': 'AH IH UW',
}
SINGLE = {k: set(v.split()) for k, v in SINGLE.items()}
RUNS = {k: set(v.split()) for k, v in RUNS.items()}
# Endings that carry the main stress where the dictionary marks two
# (engineer: EH1 ... IH1)
STRESSED_ENDINGS = ('eer', 'eers', 'ee', 'ees', 'ese', 'ette', 'ettes', 'oon', 'oons',
                    'ique', 'aire', 'esque')


def vowels_of(pron):
    out = []
    for p in pron.split():
        m = re.match(r'([A-Z]+)(\d)?$', p)
        if m and m.group(1) in VOWEL_PHONES:
            out.append((m.group(1), int(m.group(2) or 0)))
    return out


def silent_cost(s, i):
    c, n = s[i], len(s)
    if i == 0 and c != 'y':
        return 3.0          # a vowel letter that starts the word is heard
    if c == 'e':
        if i == n - 1:
            return 0.2
        if re.match(r'^(s|d|r|n|ly|ment|ments|ful|less|ness)$', s[i + 1:]):
            return 0.8
        return 1.4
    if c == 'u' and i > 0 and s[i - 1] in 'qg' and i + 1 < n and s[i + 1] in VOWEL_LETTERS:
        return 0.3
    if c == 'y' and (i == 0 or (i + 1 < n and s[i + 1] in VOWEL_LETTERS)):
        return 0.1
    return 2.5


def run_cost(run, phone, after=''):
    if len(run) == 1:
        if run == 'e' and after == 'w' and phone in ('UW', 'OW'):
            return 0.0      # new, news
        if run == 'a' and phone in ('ER', 'EH'):
            return 1.0      # arrears, any; costly, or certain becomes cer-TA-in
        return 0.0 if phone in SINGLE[run] else 2.0
    if run in RUNS:
        return 0.0 if phone in RUNS[run] else 1.5
    return 1.8


def vowel_groups(word):
    """The runs of vowel letters, without a silent final e (make, table) and
    with y a vowel only after a consonant (yes, player, myth)."""
    def vowel(i):
        c = word[i]
        if c == 'u' and i > 0 and word[i - 1] == 'q':
            return False
        if c == 'y':
            return i > 0 and not (i + 1 < len(word) and word[i + 1] in 'aeiou')
        return c in 'aeiou'
    groups, i = [], 0
    while i < len(word):
        if vowel(i):
            j = i
            while j < len(word) and vowel(j):
                j += 1
            groups.append(i)
            i = j
        else:
            i += 1
    if (len(groups) > 1 and groups[-1] == len(word) - 1 and word[-1] == 'e' and
            word[-2] not in VOWEL_LETTERS):
        groups.pop()
    return groups


def stress_letter(word, pron):
    """The index of the letter that starts the primary-stressed vowel."""
    vowels = vowels_of(pron)
    primary = [k for k, v in enumerate(vowels) if v[1] == 1]
    if not primary:
        return None
    main = primary[-1] if len(primary) > 1 and word.endswith(STRESSED_ENDINGS) else primary[0]
    # As many runs of vowel letters as vowels: one run per vowel (certain,
    # especially, inaccessible, newsroom). Otherwise the alignment below.
    groups = vowel_groups(word)
    if len(groups) == len(vowels):
        return groups[main]
    letters = [i for i, c in enumerate(word) if c in VOWEL_LETTERS]
    inf = float('inf')
    nl, nv = len(letters), len(vowels)
    best = [[inf] * (nv + 1) for _ in range(nl + 1)]
    back = [[None] * (nv + 1) for _ in range(nl + 1)]
    best[0][0] = 0.0
    for i in range(nl + 1):
        for j in range(nv + 1):
            cur = best[i][j]
            if cur == inf:
                continue
            if i < nl:
                # of two equal alignments, the one that keeps the earlier
                # letter (research: the first e, not the e of "ea")
                cost = cur + silent_cost(word, letters[i]) + 0.01 * (len(word) - letters[i])
                if cost < best[i + 1][j]:
                    best[i + 1][j], back[i + 1][j] = cost, (i, j, None)
            if j < nv:
                # a vowel without a letter (rhythm, -ism), likelier towards
                # the end of the word
                weak = vowels[j][0] in ('AH', 'IH', 'ER') and vowels[j][1] == 0
                cost = cur + (1.2 if weak else 3.0) + 0.1 * (nl - i)
                if cost < best[i][j + 1]:
                    best[i][j + 1], back[i][j + 1] = cost, (i, j, None)
                for k in (1, 2, 3):
                    if i + k > nl or letters[i + k - 1] - letters[i] != k - 1:
                        break
                    run = word[letters[i]:letters[i] + k]
                    after = word[letters[i] + k] if letters[i] + k < len(word) else ''
                    cost = cur + run_cost(run, vowels[j][0], after) + 0.05 * (k - 1)
                    # a final e after a consonant is silent but in a few
                    # words (cafe, recipe)
                    if (run == 'e' and letters[i] == len(word) - 1 and len(word) > 2 and
                            word[-2] not in VOWEL_LETTERS):
                        cost += 1.5
                    if cost < best[i + k][j + 1]:
                        best[i + k][j + 1], back[i + k][j + 1] = cost, (i, j, letters[i])
    if best[nl][nv] == inf:
        return None
    assign = [None] * nv
    i, j = nl, nv
    while (i, j) != (0, 0):
        pi, pj, letter = back[i][j]
        if pj != j:
            assign[pj] = letter
        i, j = pi, pj
    letter = assign[main]
    if letter is None:
        return None
    # A vowel the dictionary spells with two letters of which the first is
    # silent to the aligner (heart, gear, guard) is stressed on the first:
    # the voices read both letters, and the vowel starts on the first.
    taken = {a for a in assign if a is not None}
    while letter > 0 and word[letter - 1] in VOWEL_LETTERS and letter - 1 not in taken:
        letter -= 1
    return letter


def primary_index(pron):
    vowels = vowels_of(pron)
    return next((k for k, v in enumerate(vowels) if v[1] == 1), len(vowels))


def load_cmudict(path):
    """{word: pronunciation}: the first pronunciation of each word, except
    that where a noun and a verb of two syllables differ (record, update,
    present, import), the noun's, with the stress on the first syllable,
    which a label or a heading usually is."""
    entries = {}
    with open(path, encoding='utf-8') as f:
        for line in f:
            line = line.split('#')[0].strip()
            if not line:
                continue
            word, pron = line.split(' ', 1)
            word = word.split('(')[0]
            if word not in entries:
                entries[word] = pron
            elif (len(vowels_of(pron)) == 2 == len(vowels_of(entries[word])) and
                  primary_index(pron) == 0 and primary_index(entries[word]) == 1):
                entries[word] = pron
    return entries


def initialism(word, pron):
    """Read letter by letter (usa, fbi): more vowels than vowel letters."""
    return len(word) <= 5 and len(vowels_of(pron)) > sum(c in VOWEL_LETTERS for c in word)


# ---------------------------------------------------------------------------
# The front end's lookup (english_word() in formant_frontend.cpp)
# ---------------------------------------------------------------------------

# (ending, letter put back, also a native ending)
ENDINGS = [
    ('ies', 'y', False), ('ied', 'y', False), ('ily', 'y', False),
    ('iness', 'y', False), ('ier', 'y', False), ('ments', '', False),
    ('ment', '', True), ('ness', '', False), ('less', '', False),
    ('ful', '', False), ('ally', '', False), ('ly', '', False),
    ('ing', '', True), ('ing', 'e', True), ('ed', '', True), ('ed', 'e', True),
    ('ers', '', True), ('ers', 'e', True), ('er', '', True), ('er', 'e', True),
    ('est', '', True), ('est', 'e', True), ('es', '', True), ('s', '', True),
]
CASES = ['ovima', 'ovom', 'ovoj', 'ovih', 'ovim', 'ova', 'ove', 'ovi', 'ovu',
         'ima', 'om', 'em', 'a', 'u', 'e', 'i']
CONSONANTS = set('bcdfghjklmnpqrstvwxyz')     # is_consonant_letter() has y


def lookup(table, s, english_shape, depth=0):
    """The stress letter, or None. A table value of None is a native word
    (a stop entry, written without a mark)."""
    if s in table:
        return table[s]
    if depth >= 2:
        return None
    for ending, restore, native in ENDINGS:
        if native and not english_shape:
            continue
        if len(s) < len(ending) + 3 or not s.endswith(ending):
            continue
        stem = s[:len(s) - len(ending)]
        if ending == 's' and (stem.endswith('s') or stem.endswith('u')):
            continue
        found = lookup(table, stem + restore, english_shape, depth + 1)
        if found is not None:
            return found
        if (not restore and len(stem) >= 3 and stem[-1] == stem[-2] and
                stem[-1] in CONSONANTS):
            found = lookup(table, stem[:-1], english_shape, depth + 1)
            if found is not None:
                return found
    if depth == 0:
        for ending in CASES:
            if len(s) < len(ending) + 3 or not s.endswith(ending):
                continue
            base = s[:len(s) - len(ending)]
            for restore in ('', 'o', 'e', 'a'):
                word = base + restore
                found = lookup(table, word, True, depth + 1) if english_shape else table.get(word)
                if found is not None and found < len(base):
                    return found
    return None


def case_forms(word):
    """The spellings that reach a word of the table through a case ending
    (lookup(), depth 0): the word, or the word without a final o, e or a,
    with each ending."""
    forms = set()
    for restore in ('', 'o', 'e', 'a'):
        if restore and not word.endswith(restore):
            continue
        base = word[:len(word) - len(restore)]
        for ending in CASES:
            if len(base) + len(ending) >= len(ending) + 3:
                forms.add(base + ending)
    forms.discard(word)
    return forms


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

_DICTIONARIES = None


def _native_chunk(job):
    """Worker: which words of the chunk the Hunspell dictionaries accept."""
    global _DICTIONARIES
    hunspell_dir, words = job
    if _DICTIONARIES is None:
        from spylls.hunspell import Dictionary
        _DICTIONARIES = [Dictionary.from_files(os.path.join(hunspell_dir, name))
                         for name in ('hr_HR', 'sr-Latn', 'bs_BA')]
    return {w: any(d.lookup(w) or d.lookup(w.capitalize()) for d in _DICTIONARIES)
            for w in words}


def native_filter(words, hunspell_dir, cache_path):
    """The words that are words of Croatian, Serbian or Bosnian (also when
    written with a capital: names). Slow (spylls is pure Python), so it runs
    on every core and keeps its answers in the cache file."""
    cache = {}
    if cache_path and os.path.exists(cache_path):
        with open(cache_path) as f:
            cache = json.load(f)
    todo = sorted({w for w in words if w not in cache})
    if todo:
        import multiprocessing
        chunks = [(hunspell_dir, todo[i:i + 500]) for i in range(0, len(todo), 500)]
        with multiprocessing.Pool() as pool:
            for n, result in enumerate(pool.imap_unordered(_native_chunk, chunks)):
                cache.update(result)
                if n % 20 == 19:
                    print(f'  native check {min((n + 1) * 500, len(todo))}/{len(todo)}',
                          file=sys.stderr)
        if cache_path:
            with open(cache_path, 'w') as f:
                json.dump(cache, f)
    return {w for w in words if cache[w]}


def native_names():
    """The place and personal names of the native tables, and the same
    names spelled without the j of -ija, -ije, -iju (Emilija, Emilia): a
    name spelled that way in native text is the native name. Returns both
    sets."""
    names, without_j = set(), set()
    for name in ('places.tsv', 'persons.tsv'):
        with open(os.path.join(ROOT, 'tools', 'formant', name), encoding='utf-8') as f:
            for line in f:
                if line.startswith('#') or not line.strip():
                    continue
                n = line.split('\t')[0].strip().lower()
                names.add(n)
                if re.fullmatch(r'[a-z]+', n) and re.search(r'ij[aeu]', n):
                    without_j.add(re.sub(r'ij([aeu])', r'i\1', n))
    return names, without_j


def write_table(entries, path):
    """entries: {word: stress letter, or None for a native word}. Lines of
    up to ~96 characters."""
    marked = [w if i is None else w[:i] + "'" + w[i:] for w, i in sorted(entries.items())]
    lines, line = [], []
    for m in marked:
        if line and len(' '.join(line + [m])) > 92:
            lines.append(line)
            line = []
        line.append(m)
    if line:
        lines.append(line)
    with open(path, 'w', encoding='utf-8') as f:
        f.write('// -*- coding: utf-8 -*-\n')
        f.write('// formant_english.inc - Generated by tools/formant/english_words.py; do not\n')
        f.write('// edit by hand (add a row to tools/formant/english_extra.tsv instead).\n')
        f.write('//\n')
        f.write('// English words whose stress the front end does not find by its rules,\n')
        f.write("// with ' before the stressed vowel, in alphabetical order, from the\n")
        f.write('// Carnegie Mellon University Pronouncing Dictionary. A word without a mark\n')
        f.write('// is a native word that is one of these with a case ending (reforma).\n')
        f.write('//\n')
        f.write('//\n')
        f.write('//   Copyright (C) 1993-2015 Carnegie Mellon University. All rights reserved.\n')
        f.write('//   Redistribution and use in source and binary forms, with or without\n')
        f.write('//   modification, are permitted provided that the following conditions are\n')
        f.write('//   met: 1. Redistributions of source code must retain the above copyright\n')
        f.write('//   notice, this list of conditions and the following disclaimer. The\n')
        f.write('//   contents of this file are deemed to be source code. 2. Redistributions\n')
        f.write('//   in binary form must reproduce the above copyright notice, this list of\n')
        f.write('//   conditions and the following disclaimer in the documentation and/or\n')
        f.write('//   other materials provided with the distribution. THIS SOFTWARE IS\n')
        f.write('//   PROVIDED BY CARNEGIE MELLON UNIVERSITY "AS IS" AND ANY EXPRESSED OR\n')
        f.write('//   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED\n')
        f.write('//   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n')
        f.write('//   DISCLAIMED. IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY NOR ITS\n')
        f.write('//   EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,\n')
        f.write('//   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,\n')
        f.write('//   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR\n')
        f.write('//   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF\n')
        f.write('//   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING\n')
        f.write('//   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\n')
        f.write('//   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n')
        f.write('\n')
        f.write('const char* const ENGLISH[] = {\n')
        for line in lines:
            f.write('    "' + ' '.join(line) + '",\n')
        if not lines:
            f.write('    "",\n')
        f.write('};\n')


def run_helper(words, workdir):
    exe = os.path.join(workdir, 'english_stress')
    cmd = ['c++', '-O2', '-std=c++17', '-I', 'include', '-I', 'src', HELPER] + HELPER_SOURCES + ['-o', exe]
    subprocess.run(cmd, cwd=ROOT, check=True)
    out = subprocess.run([exe], input='\n'.join(words) + '\n', capture_output=True,
                         text=True, check=True).stdout
    result = {}
    for line in out.splitlines():
        word, shape, letters, *langs = line.split('\t')
        result[word] = {
            'shape': shape == '1',
            'letters': [int(x) for x in letters.split(',')] if letters else [],
            'langs': [(int(x[:-1]) if x[0] != '-' else None, x[-1]) for x in langs],
        }
    return result


def target_nucleus(letters, stress):
    for k, letter in enumerate(letters):
        if letter >= stress:
            return k
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    parser.add_argument('cmudict')
    parser.add_argument('hunspell_dir')
    parser.add_argument('--words', type=int, default=30000)
    parser.add_argument('--cache', help='file to keep the native-word checks in between runs')
    args = parser.parse_args()

    from wordfreq import top_n_list
    cmu = load_cmudict(args.cmudict)

    extra = {}
    with open(EXTRA, encoding='utf-8') as f:
        for line in f:
            line = line.split('#')[0].strip()
            if not line:
                continue
            marked = line.split('\t')[0].strip().lower()
            if marked.count("'") != 1:
                sys.exit(f'english_extra.tsv: no single stress mark in {marked!r}')
            extra[marked.replace("'", '')] = marked.index("'")

    names, without_j = native_names()
    words = [w for w in top_n_list('en', args.words)
             if re.fullmatch(r'[a-z]+', w) and w in cmu and w not in extra and
             w not in names and w not in without_j and not re.search(r'(.)\1\1', w)]
    native = native_filter(words, args.hunspell_dir, args.cache)
    stress = {}
    for w in words:
        if w in native or initialism(w, cmu[w]):
            continue
        letter = stress_letter(w, cmu[w])
        if letter is not None:
            stress[w] = letter
    stress.update(extra)
    print(f'{len(stress)} English words ({len(native)} native ones left out)', file=sys.stderr)

    backup = open(OUTPUT, encoding='utf-8').read() if os.path.exists(OUTPUT) else None
    with tempfile.TemporaryDirectory() as workdir:
        try:
            write_table({}, OUTPUT)
            plain = run_helper(sorted(stress), workdir)
        except Exception:
            if backup is not None:
                with open(OUTPUT, 'w', encoding='utf-8') as f:
                    f.write(backup)
            raise

        def wanted(w):
            info = plain.get(w)
            if not info or len(info['letters']) < 2:
                return None
            return target_nucleus(info['letters'], stress[w])

        def right(w, table):
            """Does the front end with this table stress w as English does?"""
            info, target = plain[w], wanted(w)
            letter = lookup(table, w, info['shape'])
            if letter is not None:
                return target_nucleus(info['letters'], letter) == target
            for nucleus, accent in info['langs']:
                if nucleus != target or (target > 0 and accent != 'N'):
                    return False
            return True

        candidates = sorted((w for w in stress if wanted(w) is not None),
                            key=lambda w: (len(w), w))
        table = {}
        for w in candidates:
            if not right(w, table):
                table[w] = stress[w]
        while True:
            wrong = [w for w in candidates if not right(w, table)]
            if not wrong:
                break
            for w in wrong:
                table[w] = stress[w]
        english = len(table)

        # The native words a word of the table would take for itself with a
        # case ending (reforma, republici, nadala: reform, republic, Nadal)
        # go in without a mark.
        forms = set()
        for w in table:
            forms |= case_forms(w)
        forms -= set(stress)
        forms -= set(table)
        stops = native_filter(sorted(forms), args.hunspell_dir, args.cache)
        # The native names spelled without their j: a word of the table
        # without a mark also keeps the English rules away (Emilia)
        stops |= {w for w in without_j if w not in table and w not in stress}
        for w in stops:
            table[w] = None
        write_table(table, OUTPUT)

        # Check the table as the front end reads it. An entry of the native
        # lexicon comes before the table (the name Antonia): such words go
        # out of it, and the check runs again.
        left = []
        while True:
            final = run_helper(candidates, workdir)
            bad = [w for w in candidates if w in table and any(
                nucleus != wanted(w) or (wanted(w) > 0 and accent != 'N')
                for nucleus, accent in final[w]['langs'])]
            wrong = [w for w in candidates if w not in table and w not in left and any(
                nucleus != wanted(w) or (wanted(w) > 0 and accent != 'N')
                for nucleus, accent in final[w]['langs'])]
            if wrong:
                print(f'{len(wrong)} words stressed otherwise than the script expected: ' +
                      ' '.join(wrong[:50]), file=sys.stderr)
            if not bad:
                break
            for w in bad:
                table.pop(w)
            left += bad
            english -= len(bad)
            write_table(table, OUTPUT)
        if left:
            print(f'{len(left)} words left to the native lexicon: ' + ' '.join(left[:50]),
                  file=sys.stderr)
        # The native words must stay native.
        check = run_helper(sorted(stops), workdir)
        english_read = [w for w in stops if any(
            nucleus is not None and nucleus > 0 and accent == 'N'
            for nucleus, accent in check[w]['langs'])]
        print(f'{english} English words and {len(stops)} native ones in '
              f'{os.path.relpath(OUTPUT, ROOT)}', file=sys.stderr)
        if english_read:
            print('native words read as English: ' + ' '.join(english_read[:50]),
                  file=sys.stderr)


if __name__ == '__main__':
    main()
