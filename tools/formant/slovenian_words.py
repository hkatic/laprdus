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

Usage (the table in the repository is made with these values):
    python3 tools/formant/slovenian_words.py extract SLOLEKS_DIR forms.tsv 0
    python3 tools/formant/slovenian_words.py generate forms.tsv \\
        src/formant/formant_slovenian.inc --manual-min 0 --auto-min 100 --form-min 1

`extract` streams the XML once into a TSV of word forms (13 GB of XML,
about four minutes; the last argument is the lowest lemma frequency kept);
`generate` writes the table, reading HAND_ENTRIES from
src/formant/formant_slovenian.cpp of this repository so that every form is
checked as the engine will look it up. Entries are in the notation of
slovenian::Lexicon (formant_slovenian.hpp): the accented letters of the
dictionaries, ə for the schwa written e, w for an l pronounced [w]; a stem
covers its forms whose ending the rules of unstressed endings read
correctly. The tables are front-coded (see generate()).
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


MAX_ENDING = 5      # Lexicon::MAX_ENDING
BLOCK = 16          # entries per front-coded block (formant_slovenian.cpp)

# Readings the dictionaries give otherwise than Sloleks (SSKJ² and
# Slovenski pravopis 2001 on Fran.si), per lemma: a spelling's notation for
# one form, or under '*' the notation of the beginning every form of the
# lemma shares (the rest of the form loses its marks).
CORRECTIONS = {
    # The schwa the dictionaries write and Sloleks's transcription misses
    'danes': {'danes': 'dánəs'},                    # SSKJ² dánes [danəs]
    'razen': {'razen': 'rázən'},                    # SSKJ² rázen [razən]
    'konec': {'konec': 'kônəc'},                    # SSKJ² kônec 3 [konəc]
    'sedemdeset': {'*': 'sédəmdeset'},              # SP sédemdeset [də]
    'osemdeset': {'*': 'ósəmdeset'},                # SSKJ² ósemdeset [osəmdeset]
    'sedemsto': {'*': 'sédəmsto'},
    'osemsto': {'*': 'ósəmsto'},
    'seznam': {'*': 'səznám', 'seznam': 'səznàm'},  # SSKJ² seznàm -áma [səznam]
    # The teens on their first syllable, the first variant of SSKJ²
    # (dvánajst in dvanájst, pétnajst in petnájst) and the reading users
    # accepted; SP 2001 and Sloleks put the second first
    'dvanajst': {'*': 'dvánajst'},
    'trinajst': {'*': 'trínajst'},
    'štirinajst': {'*': 'štírinajst'},
    'petnajst': {'*': 'pétnajst'},
    'šestnajst': {'*': 'šéstnajst'},
    'sedemnajst': {'*': 'sédəmnajst'},
    'osemnajst': {'*': 'ósəmnajst'},
    'devetnajst': {'*': 'devétnajst'},
    'sedemnajsti': {'*': 'sédəmnajst'},
    'osemnajsti': {'*': 'ósəmnajst'},
}

PLAIN_OF = {'á': 'a', 'à': 'a', 'é': 'e', 'ê': 'e', 'è': 'e', 'í': 'i', 'ì': 'i',
            'ó': 'o', 'ô': 'o', 'ò': 'o', 'ú': 'u', 'ù': 'u', 'ŕ': 'r'}


def letter_spans(note):
    """The characters of a notation grouped per letter of the spelling (a
    letter and the grave that may follow it)."""
    spans = []
    for ch in note:
        if ch == GRAVE and spans:
            spans[-1] += ch
        else:
            spans.append(ch)
    return spans


def correct(lemma, orth, note):
    """A form's notation with CORRECTIONS applied."""
    fix = CORRECTIONS.get(lemma.lower())
    if not fix:
        return note
    if orth in fix:
        return fix[orth]
    if '*' in fix:
        head = fix['*']
        plain = strip_key(head)
        if orth.startswith(plain):
            rest = letter_spans(note)[len(plain):]
            return head + ''.join(PLAIN_OF.get(sp[0], sp[0]) for sp in rest)
    return note


def mark_fleeting_e(orth, note, orths, cat):
    """The e of a last syllable that the other forms of the word (orths)
    drop (rékel rêkla, začétek začétka, hláden hládna, septêmber
    septêmbra, cérkev cérkve, vès vsà) is the schwa (SP 2001, § 1061);
    Sloleks's transcription writes it only for some words. Of a verb only
    the participle in -el has one (črkujem is not črkujmo's). Returns the
    notation with that e as ə."""
    n = len(orth)
    if cat == 'verb' and not orth.endswith('el'):
        return note
    if n >= 3 and orth[-2] == 'e' and orth[-1] not in 'aeiouj' and orth[-3] not in 'aeiou':
        dropped = orth[:-2] + orth[-1]
        if any(o.startswith(dropped) and len(o) > len(dropped) and o[len(dropped)] in 'aeiou'
               for o in orths):
            spans = letter_spans(note)
            e = spans[n - 2]
            if e == 'e':
                spans[n - 2] = SCHWA
            elif e in ('è', 'e' + GRAVE):
                spans[n - 2] = SCHWA + GRAVE
            note = ''.join(spans)
    return note


def norm(note):
    """A notation as compared by the check below: an l that the rules read
    as [w] anyway is not a difference."""
    return note.replace('w', 'l')


def hand_entries(cpp_path):
    """The hand-written entries of formant_slovenian.cpp (HAND_ENTRIES), as
    Lexicon::add reads them: (exact, stems), spelling -> notation."""
    src = open(cpp_path, encoding='utf-8').read()
    body = src[src.index('const char* const HAND_ENTRIES[] = {'):]
    body = re.sub(r'//[^\n]*', '', body[:body.index('\n};')])
    exact, stems = {}, {}
    for entry in re.findall(r'u8"([^"]*)"', body):
        entry = unicodedata.normalize('NFC', entry)
        if '|' in entry:
            parts = entry.split('|')
            forms = [parts[0] + p for p in parts[1:]]
        else:
            forms = [entry]
        for form in forms:
            stem = form.endswith('*')
            if stem:
                form = form[:-1]
            if '=' in form:
                spelling, form = form.split('=', 1)
                key = strip_key(spelling)
            else:
                key = strip_key(form)
            (stems if stem else exact)[key] = form
    return exact, stems


class Reader:
    """Lexicon::find_exact and find_stem of formant_slovenian.cpp: the hand
    entries, then the table; for stems the longest one the word starts
    with, the hand's first at each length, followed by an ending of up to
    five letters read by the rules of unstressed endings."""
    def __init__(self, hand_exact, hand_stems, exact, stems):
        self.hand_exact = hand_exact
        self.hand_stems = hand_stems
        self.exact = exact
        self.stems = stems

    def read(self, word):
        """(notation, decided by a hand entry) or (None, False)."""
        if word in self.hand_exact:
            return self.hand_exact[word], True
        if word in self.exact:
            return self.exact[word], False
        n = len(word)
        shortest = max(n - MAX_ENDING if n > MAX_ENDING else 1, 2)
        for length in range(n, shortest - 1, -1):
            for table, hand in ((self.hand_stems, True), (self.stems, False)):
                stem = table.get(word[:length])
                if stem is not None:
                    return stem + ending_by_rule(stem, word[length:]), hand
        return None, False


def build(forms_path, manual_min, auto_min, form_min):
    """The reading of every spelling (by its most frequent form) and the
    stems, from the Sloleks forms: (lemma count, truth, stems)."""
    lemmas = {}         # (lemma, cat) -> dict(freq, forms{orth: [note, ff, graded, proper]})
    skipped = 0
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
            orth = orth.lower()
            # A form with two spellings (mesca, meseca) lists the accents
            # and transcriptions of both: take the first that spells this one
            note = None
            ipa_list = ipas.split('|')
            for k, acc in enumerate(accs.split('|')):
                note = notation(orth, acc, ipa_list[min(k, len(ipa_list) - 1)])
                if note is not None and strip_key(note) == orth:
                    break
                note = None
            if note is None:
                skipped += 1
                continue
            entry = lemmas.setdefault((lemma, cat), {'freq': freq, 'forms': {}})
            # Comparatives and superlatives (najlepši, lepšega) are not part
            # of the adjective's stem: listed on their own when frequent
            graded = cat == 'adjective' and len(msd) > 1 and msd[1] in 'rs'
            # One spelling may be two forms of the word read differently
            # (lásti the dative, lastí the genitive of lást). The word class
            # of the form: V a verb's form other than the participle, P the
            # l-participle, N a noun, an adjective or an adverb, '' the rest
            if msd.startswith('Gg'):
                word_class = 'P' if msd[3:4] == 'd' else 'V'
            elif msd.startswith(('S', 'P', 'R')) and not msd.startswith('Sl'):
                word_class = 'N'
            else:
                word_class = ''
            form = entry['forms'].setdefault(orth, {}).setdefault(
                (note, word_class), [0, graded, msd.startswith('Sl')])
            form[0] += ff
    print('lemmas', len(lemmas), 'forms skipped', skipped, file=sys.stderr)

    # The readings of every spelling, weighted by the corpus frequency of
    # the form (Gigafida 2.0, given by Sloleks for every form): a spelling
    # shared by two words is read as its more frequent form, not as the
    # form of the more frequent lemma. The adjective vesél is more frequent
    # than the verb veselíti, but veselím "I look forward" is eight times
    # as frequent as the adjective's vesélim. A proper name counts a tenth:
    # the lexicon is looked up without the capital, and the common word
    # (sobóta) is the one spelled small (Múrska Sóbota).
    weight = defaultdict(dict)      # spelling -> notation -> [weight, lemma freq]
    by_class = defaultdict(lambda: defaultdict(lambda: defaultdict(int)))
    listed = []                     # per lemma: (freq, [(orth, note)]) for the stems
    for (lemma, cat), entry in lemmas.items():
        orths = entry['forms'].keys()
        forms = []
        for orth, readings in entry['forms'].items():
            best = None
            for (note, word_class), (ff, graded, proper) in readings.items():
                # The rare forms (the dual, the rarer cases) are left to the
                # stems and the rules
                if ff < form_min and orth != lemma.lower():
                    continue
                if graded and ff < 20:
                    continue
                note = correct(lemma, orth, mark_fleeting_e(orth, note, orths, cat))
                w = weight[orth].setdefault(note, [0.0, 0])
                w[0] += ff / 10.0 if proper else ff
                w[1] = max(w[1], entry['freq'])
                if word_class and not proper:
                    by_class[orth][word_class][note] += ff
                if not graded and (best is None or ff > best[0]):
                    best = (ff, note)
            if best:
                forms.append((orth, best[1]))
        listed.append((entry['freq'], forms))
    truth = {orth: max(rs.items(), key=lambda kv: (kv[1][0], kv[1][1]))[0]
             for orth, rs in weight.items()}

    stems = {}          # key -> (lemma freq, notation without the '*')
    for freq, forms in listed:
        s, _e = compress(forms)
        for note in s:
            key = strip_key(note)
            if key not in stems or stems[key][0] < freq:
                stems[key] = (freq, note[:-1])
    stems = {key: note for key, (_freq, note) in stems.items()}

    # The spellings that are a verb's form in one reading and a noun's, an
    # adjective's or an adverb's in another, read differently (veselí and
    # veséli, letí and léti, vôzil and vozíl), both readings at least 20
    # times in the corpus: the front end picks one by the words around it.
    # spelling -> (verb's notation, other notation, participle)
    verb_noun = {}
    for orth, classes in by_class.items():
        if 'N' not in classes:
            continue
        nominal, n_weight = max(classes['N'].items(), key=lambda kv: kv[1])
        best = None
        for word_class in ('V', 'P'):
            if word_class not in classes:
                continue
            verb, v_weight = max(classes[word_class].items(), key=lambda kv: kv[1])
            total = sum(classes[word_class].values())
            if norm(verb) == norm(nominal) or min(total, sum(classes['N'].values())) < 20:
                continue
            if best is None or total > best[0]:
                best = (total, verb, word_class == 'P')
        if best:
            verb_noun[orth] = (best[1], nominal, best[2])
    return len(lemmas), truth, stems, verb_noun


def generate(forms_path, out_path, manual_min=50, auto_min=2000, form_min=3, cpp_path=None):
    lemma_count, truth, stems, verb_noun = build(forms_path, manual_min, auto_min, form_min)

    # Every form must read as it should. The stem of one word catches the
    # forms of others (postavít*, the stem of postavítev, read postaviti as
    # postavíti), the stem of a homograph the forms of the other word, and a
    # hand-written stem the words derived from it: where the lookup gives a
    # wrong reading or none, the form is listed. A hand-written whole form
    # is the hand's decision and stays.
    hand_exact, hand_stems = hand_entries(cpp_path) if cpp_path else ({}, {})
    exact = {}
    reader = Reader(hand_exact, hand_stems, exact, stems)
    for orth, note in truth.items():
        got, hand = reader.read(orth)
        if got is None or norm(got) != norm(note):
            if not (hand and orth in hand_exact):
                exact[orth] = note
    print('spellings', len(truth), 'exact', len(exact), 'stems', len(stems),
          'verb/noun', len(verb_noun), file=sys.stderr)

    def front_coded(table):
        """Blocks of BLOCK entries in key order: the first entry whole, each
        later one as a byte 1 + n (an octal escape) for the n leading bytes
        it shares with the entry before, then the bytes that differ. The
        entries hold no byte below 0x61, so the next byte of 1 to 64 ends
        an entry."""
        keys = sorted(table, key=lambda k: [ord(c) for c in k])
        blocks = []
        for b in range(0, len(keys), BLOCK):
            parts = []
            prev = ''
            for i, key in enumerate(keys[b:b + BLOCK]):
                note = unicodedata.normalize('NFC', table[key])
                data = note.encode('utf-8')
                assert all(c >= 0x61 for c in data), note
                if i == 0:
                    parts.append(note)
                else:
                    shared = 0      # whole characters, so the rest stays UTF-8
                    while shared < min(len(note), len(prev)) and note[shared] == prev[shared]:
                        shared += 1
                    count = len(note[:shared].encode('utf-8'))
                    assert count < 64
                    parts.append('\\%03o' % (count + 1) + note[shared:])
                prev = note
            blocks.append(''.join(parts))
        return keys, blocks

    form_keys, form_blocks = front_coded(exact)
    stem_keys, stem_blocks = front_coded(stems)
    with open(out_path, 'w', encoding='utf-8') as out:
        out.write('// -*- coding: utf-8 -*-\n')
        out.write('// formant_slovenian.inc - Slovenian pronunciation lexicon of the formant\n')
        out.write('// voices. Generated by tools/formant/slovenian_words.py from Sloleks 3.1\n')
        out.write('// (CJVT, University of Ljubljana; CLARIN.SI, http://hdl.handle.net/11356/2080;\n')
        out.write('// CC BY-SA 4.0): the word forms of %d lemmas, in the notation of\n' % lemma_count)
        out.write('// slovenian::Lexicon (formant_slovenian.hpp). Do not edit: edit the tool\n')
        out.write('// and run it. Both tables are sorted by the spelling the entry is for (its\n')
        out.write('// letters without the marks, ə as e, w as l), as the binary search in\n')
        out.write('// formant_slovenian.cpp compares them, and front-coded in blocks of %d:\n' % BLOCK)
        out.write('// an entry after the first of its block begins with a byte 1 + n for the\n')
        out.write('// n leading bytes it shares with the entry before. Stems have no \'*\'.\n')
        out.write('// clang-format off\n')
        out.write('const size_t SLOVENIAN_BLOCK = %d;\n' % BLOCK)
        out.write('const size_t SLOVENIAN_FORM_COUNT = %d;\n' % len(form_keys))
        out.write('const char* const SLOVENIAN_FORMS[] = {\n')
        for block in form_blocks:
            out.write('    u8"%s",\n' % block)
        out.write('};\n')
        out.write('const size_t SLOVENIAN_STEM_COUNT = %d;\n' % len(stem_keys))
        out.write('const char* const SLOVENIAN_STEMS[] = {\n')
        for block in stem_blocks:
            out.write('    u8"%s",\n' % block)
        out.write('};\n')
        out.write('// Spellings read as a verb\'s form or as another word: the verb\'s\n')
        out.write('// reading, the other, and whether the verb\'s is the l-participle\n')
        out.write('const VerbNoun SLOVENIAN_VERB_NOUN[] = {\n')
        for key in sorted(verb_noun, key=lambda k: [ord(c) for c in k]):
            verb, nominal, participle = verb_noun[key]
            out.write('    {u8"%s", u8"%s", %s},\n' % (unicodedata.normalize('NFC', verb),
                                                      unicodedata.normalize('NFC', nominal),
                                                      'true' if participle else 'false'))
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
        root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
        generate(argv[2], argv[3], manual_min, auto_min, form_min,
                 os.path.join(root, 'src', 'formant', 'formant_slovenian.cpp'))
    else:
        print(__doc__, file=sys.stderr)
        return 2
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
