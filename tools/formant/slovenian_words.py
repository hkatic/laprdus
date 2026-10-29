#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Generate src/formant/formant_slovenian.inc, the Slovenian pronunciation
lexicon of the formant voices, from the Sloleks morphological lexicon.

Source: Sloleks 3.1 (Čibej, Gantar, Dobrovoljc, Krek, Holozan, Erjavec,
Romih, Arhar Holdt, Krsnik, Robnik-Šikonja; CJVT, University of Ljubljana;
CLARIN.SI, http://hdl.handle.net/11356/2080), licensed CC BY-SA 4.0. Every
word form of its entries carries the orthography, the Gigafida 2.0
frequency, the accented form in the notation of Slovenian dictionaries
(á é ê è ó ô ò ...) and an IPA transcription. The accents of the 100,000
entries inherited from Sloleks 2.0 were corrected by hand (status MANUAL);
the rest were set by a neural network (status AUTOMATIC, 91% correct in
Krsnik 2017). The IPA comes from an automatic grapheme-to-phoneme system
and is used here only for what the accented form does not show: which e is
a schwa and which l is [w].

Usage:
    python3 tools/formant/slovenian_words.py extract SLOLEKS_DIR forms.tsv
    python3 tools/formant/slovenian_words.py generate forms.tsv \\
        src/formant/formant_slovenian.inc [--manual-min 50] [--auto-min 2000] [--form-min 3]

`extract` streams the XML once into a TSV of word forms (slow: 13 GB of
XML); `generate` writes the table. Entries are in the notation of
slovenian::Lexicon (formant_slovenian.hpp): the accented letters of the
dictionaries, ə for the schwa written e, w for an l pronounced [w]; a stem
ends in `*` and covers its forms whose ending the rules of unstressed
endings read correctly.
"""
import glob
import os
import re
import sys
import unicodedata
import xml.etree.ElementTree as ET
from collections import defaultdict

# ---------------------------------------------------------------------------
# extract
# ---------------------------------------------------------------------------

def extract(src, out_path, min_freq=200):
    out = open(out_path, 'w', encoding='utf-8')
    files = sorted(glob.glob(os.path.join(src, 'sloleks_3.1_*.xml')))
    seen = kept = 0
    for path in files:
        for _event, elem in ET.iterparse(path, events=('end',)):
            if elem.tag != 'entry':
                continue
            seen += 1
            head = elem.find('head')
            status = head.findtext('status') or ''
            lemma = head.findtext('headword/lemma') or ''
            cat = head.findtext('grammar/category') or ''
            ptype = ''
            for sub in head.findall('grammar/subcategory'):
                if sub.get('type') == 'pronunciation':
                    ptype = sub.text or ''
            feats = ';'.join('%s=%s' % (g.get('name'), g.text)
                             for g in head.findall('grammar/grammarFeature'))
            freq = 0
            for m in head.findall('measureList/measure'):
                if m.get('type') == 'frequency':
                    try:
                        freq = int(m.text)
                    except (TypeError, ValueError):
                        pass
            keep = freq >= min_freq or (status == 'MANUAL' and freq >= min_freq // 4)
            if keep and ptype not in ('Acronym', 'Abbreviation', 'Numeral', 'Symbol'):
                for wf in elem.findall('body/wordFormList/wordForm'):
                    msd = wf.findtext('msd') or ''
                    fr = wf.find('formRepresentations')
                    if fr is None:
                        continue
                    orths = []
                    for o in fr.findall('orthographyList/orthography'):
                        f = o.findtext('form') or ''
                        ff = 0
                        for m in o.findall('measureList/measure'):
                            if m.get('type') == 'frequency':
                                try:
                                    ff = int(m.text)
                                except (TypeError, ValueError):
                                    pass
                        orths.append((f, ff))
                    accs = [a.findtext('form') or '' for a in fr.findall('accentuationList/accentuation')]
                    ipas = []
                    for p in fr.findall('pronunciationList/pronunciation'):
                        for f in p.findall('form'):
                            if f.get('script') == 'IPA':
                                ipas.append(f.text or '')
                    for f, ff in orths:
                        out.write('\t'.join([lemma, cat, status, str(freq), feats, msd, f,
                                             str(ff), '|'.join(accs), '|'.join(ipas)]) + '\n')
                kept += 1
            elem.clear()
        print(path, seen, kept, file=sys.stderr, flush=True)
    out.close()


# ---------------------------------------------------------------------------
# generate: notation
# ---------------------------------------------------------------------------

LETTERS = set('abcčdefghijklmnoprsštuvzž')
ACCENTED = {
    'á': ('a', 'long'), 'à': ('a', 'short'),
    'é': ('e', 'close'), 'ê': ('e', 'open'), 'è': ('e', 'short'),
    'í': ('i', 'long'), 'ì': ('i', 'short'),
    'ó': ('o', 'close'), 'ô': ('o', 'open'), 'ò': ('o', 'short'),
    'ú': ('u', 'long'), 'ù': ('u', 'short'),
    'ŕ': ('r', 'long'),
}
SCHWA = 'ə'
GRAVE = '̀'

# IPA simplification for the alignment: consonants to classes, vowels kept
IPA_MAP = {
    'a': 'a', 'i': 'i', 'u': 'u', 'ɛ': 'E', 'e': 'e', 'ɔ': 'O', 'o': 'o', 'ə': '@',
    'l': 'L', 'w': 'W', 'ʍ': 'W', 'ʋ': 'V', 'v': 'V', 'j': 'J', 'ɪ': 'J', 'ɾ': 'R', 'r': 'R',
    'm': 'M', 'ɱ': 'M', 'n': 'N', 'ŋ': 'N',
}
RELEASE = set('ⁿˡʷʲ')


def simplify_ipa(ipa):
    """IPA to (reduced string, stress index, long set) where stress and
    length refer to vowel positions in the reduced string."""
    out = []
    stress = -1
    longs = set()
    pending_stress = False
    prev_u = False
    for ch in ipa:
        if ch == 'ˈ':
            pending_stress = True
            continue
        if ch == 'ː':
            if out:
                longs.add(len(out) - 1)
            continue
        if ch in RELEASE or ch in '.ˌ':
            continue
        if ch == '̯':      # combining inverted breve below: u̯
            if out and out[-1] == 'u':
                out[-1] = 'W'
                if len(out) - 1 in longs:
                    longs.discard(len(out) - 1)
            continue
        if ch in IPA_MAP:
            sym = IPA_MAP[ch]
        elif ch in 'ptkbdɡgszʃʒfxɣʦʣʧʤcʒh':
            sym = 'C'
        else:
            return None
        if pending_stress and sym in 'aiuEeOo@':
            stress = len(out)
            pending_stress = False
        out.append(sym)
    return ''.join(out), stress, longs


def align(letters, reduced):
    """Align the letters of a word with the reduced IPA. Returns a list
    with, for each letter, the reduced symbols it produced (a string), or
    None when the two cannot be aligned.

    Allowed productions:
      a i u -> itself; e -> E e @; o -> O o
      i -> 'iJ' (pacient), 'J' (diphthong offglide, rare)
      r -> R, or '@R' (syllabic)
      l -> L, W; lj -> 'LJ', 'L'; nj -> 'NJ', 'N'
      j -> J, or nothing after l/n (lʲ), J
      v -> V, W, u
      m -> M; n -> N (also M before b/p)
      dž -> C; other consonant -> C
    """
    n = len(letters)
    m = len(reduced)
    # dynamic programming over (letter index, ipa index) with a small set
    # of productions; cost = number of non-default productions
    INF = 10 ** 9
    best = {(0, 0): (0, None)}
    frontier = [(0, 0)]
    prods = {}
    def options(i):
        c = letters[i]
        nxt = letters[i + 1] if i + 1 < n else ''
        o = []
        if c == 'a':
            o = [('a', 0)]
        elif c == 'i':
            o = [('i', 0), ('iJ', 1), ('J', 2)]
        elif c == 'u':
            o = [('u', 0), ('W', 2)]
        elif c == 'e':
            o = [('E', 0), ('e', 0), ('@', 1)]
        elif c == 'o':
            o = [('O', 0), ('o', 0)]
        elif c == 'r':
            o = [('R', 0), ('@R', 1)]
        elif c == 'l':
            o = [('L', 0), ('W', 1)]
            if nxt == 'j':
                o += [('LJ', 0, 2), ('L', 1, 2)]
        elif c == 'n':
            o = [('N', 0), ('M', 1)]
            if nxt == 'j':
                o += [('NJ', 0, 2), ('N', 1, 2)]
        elif c == 'j':
            o = [('J', 0), ('', 2)]
        elif c == 'v':
            o = [('V', 0), ('W', 0), ('u', 1)]
        elif c == 'm':
            o = [('M', 0)]
        elif c == 'd' and nxt == 'ž':
            o = [('C', 0, 2), ('CC', 0, 2)]
        elif c in 'td' and nxt in 'sšzž':
            # t + s is the affricate [ʦ] in the transcription (abonentski)
            o = [('C', 0), ('C', 0, 2)]
        elif c in LETTERS:
            o = [('C', 0)]
        else:
            return []
        return [(p[0], p[1], p[2] if len(p) > 2 else 1) for p in o]
    # DP
    dp = [[(INF, None)] * (m + 1) for _ in range(n + 1)]
    dp[0][0] = (0, None)
    for i in range(n):
        for j in range(m + 1):
            cost, _ = dp[i][j]
            if cost >= INF:
                continue
            for prod, pc, used in options(i):
                k = len(prod)
                if j + k <= m and reduced[j:j + k] == prod and i + used <= n:
                    nc = cost + pc
                    if nc < dp[i + used][j + k][0]:
                        dp[i + used][j + k] = (nc, (i, j, prod, used))
    if dp[n][m][0] >= INF:
        return None
    # backtrack
    result = [''] * n
    i, j = n, m
    while i > 0 or j > 0:
        _cost, back = dp[i][j]
        if back is None:
            return None
        pi, pj, prod, used = back
        result[pi] = prod
        for k in range(1, used):
            result[pi + k] = ''
        i, j = pi, pj
    return result


def notation(orth, accented, ipa):
    """The lexicon notation of one form, or None when the form cannot be
    turned into it (foreign letters, inconsistent data)."""
    orth = orth.lower()
    accented = accented.lower()
    if not orth or any(c not in LETTERS for c in orth):
        return None
    # the accented form must spell the same word
    plain = []
    marks = []
    for ch in unicodedata.normalize('NFC', accented):
        if ch in ACCENTED:
            base, kind = ACCENTED[ch]
            plain.append(base)
            marks.append(kind)
        elif ch in LETTERS:
            plain.append(ch)
            marks.append('')
        else:
            return None
    if ''.join(plain) != orth:
        return None
    stressed = [i for i, k in enumerate(marks) if k]
    if not stressed or len(stressed) > 2:
        return None       # no stress (an unaccented name), or more than two
    # A compound with two accents (àbiótski, nàjlépši) keeps both marks; the
    # engine takes the last as the main stress
    simp = simplify_ipa(ipa)
    if simp is None:
        return None
    reduced, ipa_stress, longs = simp
    al = align(orth, reduced)
    if al is None:
        return None
    # letter -> position of its first reduced symbol
    pos = []
    p = 0
    for prod in al:
        pos.append(p)
        p += len(prod)
    out = []
    for i, ch in enumerate(orth):
        prod = al[i]
        kind = marks[i]
        if ch == 'e':
            schwa = prod == '@'
            if kind == 'close':
                if schwa:
                    return None
                out.append('é')
            elif kind == 'open':
                out.append('ê')
            elif kind == 'short':
                out.append(SCHWA + GRAVE if schwa else 'è')
            else:
                out.append(SCHWA if schwa else 'e')
        elif ch == 'o':
            out.append({'close': 'ó', 'open': 'ô', 'short': 'ò'}.get(kind, 'o'))
        elif ch in 'aiu':
            out.append({'long': {'a': 'á', 'i': 'í', 'u': 'ú'}[ch],
                        'short': {'a': 'à', 'i': 'ì', 'u': 'ù'}[ch]}.get(kind, ch))
        elif ch == 'r':
            if kind == 'long':
                out.append('ŕ')
            elif kind == 'short':
                return None
            else:
                out.append('r')
        elif ch == 'l':
            out.append('w' if prod == 'W' else 'l')
        else:
            out.append(ch)
    # consistency: the IPA stress should sit on the accented letter's vowel
    # (or on the schwa of a syllabic r)
    si = stressed[0]
    if ipa_stress >= 0:
        sp = pos[si]
        prod = al[si]
        if not (sp <= ipa_stress < sp + max(1, len(prod))):
            return None
    return ''.join(out)


# ---------------------------------------------------------------------------
# generate: the rules of unstressed endings, mirrored from finish_word()
# ---------------------------------------------------------------------------

def strip_key(note):
    """The spelling an entry is for."""
    out = []
    for ch in unicodedata.normalize('NFC', note):
        if ch in ACCENTED:
            out.append(ACCENTED[ch][0])
        elif ch == SCHWA:
            out.append('e')
        elif ch == GRAVE:
            continue
        elif ch == 'w':
            out.append('l')
        elif ch in '*':
            continue
        else:
            out.append(ch)
    return ''.join(out)


def ending_by_rule(stem_note, ending):
    """What the engine reads for stem + ending, as notation (the ending is
    unstressed: e and o open, the final -eC schwa, the final l [w])."""
    word = strip_key(stem_note) + ending
    out = list(ending)
    n = len(word)
    # schwa of a final -er -el -en -ek -ec after a consonant, of -ev after k
    # or t (cerkev, molitev; not the genitive plural delavcev) and of -em
    # after z or t (absolutizem, algoritem; not the instrumental delavcem)
    if len(ending) >= 2 and ending[-2] == 'e' and n >= 3 \
            and word[-3] not in 'aeiou' and word[-3] != 'j' \
            and (ending[-1] in 'rlnkc' or (ending[-1] == 'v' and word[-3] in 'kt')
                 or (ending[-1] == 'm' and word[-3] in 'zt')):
        out[-2] = SCHWA
    elif len(ending) == 1 and ending == 'l':
        pass
    # final l after a vowel: [w] (the stress is in the stem)
    if ending.endswith('l') and n >= 2 and (word[-2] in 'aeiu' or out[-2:-1] == [SCHWA]):
        out[-1] = 'w'
    return ''.join(out)


def compress(lemma_forms):
    """Entries for one lemma: (stems, exact) lists of notation strings."""
    forms = {}          # key -> notation
    for orth, note in lemma_forms:
        key = strip_key(note)
        if key != orth:
            continue
        forms.setdefault(key, note)
    if not forms:
        return [], []
    keys = sorted(forms)
    if len(keys) == 1:
        return [], [forms[keys[0]]]
    # the longest common prefix of the spellings
    prefix = keys[0]
    for k in keys[1:]:
        i = 0
        while i < len(prefix) and i < len(k) and prefix[i] == k[i]:
            i += 1
        prefix = prefix[:i]
    # try a stem: the notation of the prefix must be the same in every form,
    # carry the stress, and every ending must read by rule
    if len(prefix) >= 4:
        stem_note = None
        ok = True
        for k in keys:
            note = forms[k]
            # the notation prefix covering the first len(prefix) letters
            count = 0
            cut = 0
            for ch in note:
                if ch == GRAVE:
                    cut += 1
                    continue
                if count == len(prefix):
                    break
                count += 1
                cut += 1
            cand = note[:cut]
            rest = note[cut:]
            if stem_note is None:
                stem_note = cand
            if cand != stem_note or len(k) - len(prefix) > 5:
                ok = False
                break
            if rest != ending_by_rule(stem_note, k[len(prefix):]):
                ok = False
                break
        if ok and stem_note and any(ch in ACCENTED or ch == GRAVE for ch in stem_note):
            return [stem_note + '*'], []
    return [], [forms[k] for k in keys]


def generate(forms_path, out_path, manual_min=50, auto_min=2000, form_min=3):
    lemmas = {}         # (lemma, cat) -> dict(freq, status, forms[])
    with open(forms_path, encoding='utf-8') as f:
        for line in f:
            parts = line.rstrip('\n').split('\t')
            if len(parts) < 10:
                continue
            lemma, cat, status, freq, _feats, msd, orth, ff, accs, ipas = parts[:10]
            freq = int(freq)
            ff = int(ff)
            if status == 'MANUAL':
                if freq < manual_min:
                    continue
            elif freq < auto_min:
                continue
            if not accs or not ipas:
                continue
            # The rare forms (the dual, the rarer cases) are left to the
            # rules: a stem covers them anyway, and an irregular paradigm
            # is listed only in the forms that occur
            if ff < form_min and orth.lower() != lemma.lower():
                continue
            # Comparatives and superlatives (najlepši, lepšega) are not part
            # of the adjective's stem: listed on their own when frequent
            graded = cat == 'adjective' and len(msd) > 1 and msd[1] in 'rs'
            key = (lemma, cat)
            entry = lemmas.setdefault(key, {'freq': freq, 'status': status, 'forms': [], 'graded': []})
            (entry['graded'] if graded else entry['forms']).append(
                (orth.lower(), accs.split('|')[0], ipas.split('|')[0], ff))
    print('lemmas', len(lemmas), file=sys.stderr)

    exact = {}      # key -> (freq, notation)
    stems = {}
    skipped = 0
    for (lemma, cat), entry in lemmas.items():
        notes = []
        for orth, acc, ipa, _ff in entry['forms']:
            note = notation(orth, acc, ipa)
            if note is None:
                skipped += 1
                continue
            notes.append((orth, note))
        s, e = compress(notes)
        for orth, acc, ipa, ff in entry['graded']:
            if ff < 20:
                continue
            note = notation(orth, acc, ipa)
            if note is not None and strip_key(note) == orth:
                e.append(note)
        for note in s:
            key = strip_key(note)
            if key not in stems or stems[key][0] < entry['freq']:
                stems[key] = (entry['freq'], note)
        for note in e:
            key = strip_key(note)
            if key not in exact or exact[key][0] < entry['freq']:
                exact[key] = (entry['freq'], note)
    print('forms skipped', skipped, 'exact', len(exact), 'stems', len(stems), file=sys.stderr)

    def c_string(s):
        return 'u8"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'

    with open(out_path, 'w', encoding='utf-8') as out:
        out.write('// -*- coding: utf-8 -*-\n')
        out.write('// formant_slovenian.inc - Slovenian pronunciation lexicon of the formant\n')
        out.write('// voices. Generated by tools/formant/slovenian_words.py from Sloleks 3.1\n')
        out.write('// (CJVT, University of Ljubljana; CLARIN.SI, http://hdl.handle.net/11356/2080;\n')
        out.write('// CC BY-SA 4.0): the word forms of the %d most frequent lemmas, in the\n' % len(lemmas))
        out.write('// notation of slovenian::Lexicon (formant_slovenian.hpp). Do not edit:\n')
        out.write('// edit the tool and run it. Both tables are sorted by the spelling the\n')
        out.write('// entry is for (its letters without the marks, ə as e, w as l), as the\n')
        out.write('// binary search in formant_slovenian.cpp compares them.\n')
        out.write('// clang-format off\n')
        out.write('const char* const SLOVENIAN_FORMS[] = {\n')
        for key in sorted(exact, key=lambda k: [ord(c) for c in k]):
            out.write('    %s,\n' % c_string(exact[key][1]))
        out.write('};\n')
        out.write('const char* const SLOVENIAN_STEMS[] = {\n')
        for key in sorted(stems, key=lambda k: [ord(c) for c in k]):
            out.write('    %s,\n' % c_string(stems[key][1]))
        out.write('};\n')
        out.write('// clang-format on\n')


def main(argv):
    if len(argv) >= 4 and argv[1] == 'extract':
        extract(argv[2], argv[3], int(argv[4]) if len(argv) > 4 else 200)
    elif len(argv) >= 4 and argv[1] == 'generate':
        manual_min = 50
        auto_min = 2000
        form_min = 3
        args = argv[4:]
        while args:
            if args[0] == '--manual-min':
                manual_min = int(args[1]); args = args[2:]
            elif args[0] == '--auto-min':
                auto_min = int(args[1]); args = args[2:]
            elif args[0] == '--form-min':
                form_min = int(args[1]); args = args[2:]
            else:
                print('unknown option', args[0], file=sys.stderr); return 2
        generate(argv[2], argv[3], manual_min, auto_min, form_min)
    else:
        print(__doc__, file=sys.stderr)
        return 2
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
