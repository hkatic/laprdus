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

# ć and đ: the letters of names from Croatian, Serbian and Bosnian (Petrović,
# Đorđević), which Slovenian writes as in the source language
LETTERS = set('abcčćdđefghijklmnoprsštuvzž')
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
        elif ch in 'ptkbdɡgszʃʒfxɣʦʣʧʤcʒhɕʑ':
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
        elif c in 'ćđ':
            o = [('C', 0), ('CC', 0)]      # [ʧ] or [tɕ]
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


def notation(orth, accented, ipa, trust_accent=False):
    """The lexicon notation of one form, or None when the form cannot be
    turned into it (foreign letters, inconsistent data). With trust_accent
    (an accentuation corrected by hand) a transcription that cannot be
    aligned with the spelling, or that puts its stress elsewhere, does not
    lose the form: the accent alone gives the notation, without the schwa
    and the [w] that only the transcription shows (Afganistan, whose
    transcription begins [aʋɡ], is Afgánistan)."""
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
    al = align(orth, simp[0]) if simp is not None else None
    if al is None:
        if not trust_accent:
            return None
        al = [''] * len(orth)       # no schwa, every l by the rules
        ipa_stress = -1
    else:
        ipa_stress = simp[1]
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
                if schwa and not trust_accent:
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
    # (or on the schwa of a syllabic r). Of two accents the transcription
    # marks the last as its main stress (géofízičen, Bánjalúka) or the
    # first (nàjrésnejših); either agrees.
    if ipa_stress >= 0 and not trust_accent:
        if not any(pos[si] <= ipa_stress < pos[si] + max(1, len(al[si])) for si in stressed):
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


# Common words written with a capital inside a sentence without being the
# name spelled like them: God and the Holy Spirit ("v Boga", "Svetega
# Duha" are bogá, duhá, not the names Bógo and Dúh)
COMMON_CAPITALIZED = {'bog', 'duh', 'gospod', 'oče', 'sin', 'mati', 'devica', 'stvarnik'}

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


# The surnames in -ović, -ević (-ovič, -evič, -ovic, -evic): Petrović,
# Petroviča, Petrovićem, Petrovićeva. The front end stresses them on the
# syllable before the suffix (Pétrović, Stojánović, Milóšević: the first
# syllable when there are three), whatever stem of the lexicon they begin
# with: formant_slovenian.cpp, vic_surname(), mirrors this.
VIC_SUFFIXES = ('ović', 'ovič', 'ovic', 'ević', 'evič', 'evic')
VIC_ENDINGS = sorted(('', 'a', 'u', 'em', 'om', 'i', 'e', 'ev', 'ih', 'ema', 'ama', 'eva', 'evo',
                      'evi', 'eve', 'evega', 'evemu', 'evem', 'evih', 'evim', 'evimi'),
                     key=len, reverse=True)


def vic_surname(word):
    """Where the -ović of such a surname begins in the word, or -1."""
    for ending in VIC_ENDINGS:
        if not word.endswith(ending):
            continue
        core = word[:len(word) - len(ending)]
        if len(core) >= 5 and core.endswith(VIC_SUFFIXES):
            return len(core) - 4
    return -1


# ---------------------------------------------------------------------------
# generate: the names of places of Slovenski pravopis 2001
# ---------------------------------------------------------------------------

PLACE_INFLECTIONS = {'a', 'e', 'i', 'u', 'o', 'ov', 'ev', 'ega', 'ih', 'em', 'om'}
SOFT_ENDS = ('c', 'č', 'ć', 'j', 'š', 'ž')


def plain_word(word):
    """An accented word's letters without the marks, lowercase."""
    return ''.join(ACCENTED[ch][0] if ch in ACCENTED else ch
                   for ch in unicodedata.normalize('NFC', word.lower()) if ch != GRAVE)


def unaccented(word):
    return ''.join(ACCENTED[ch][0] if ch in ACCENTED else ch
                   for ch in unicodedata.normalize('NFC', word) if ch != GRAVE)


def place_genitive(word, token):
    """The genitive of an accented nominative as SP 2001 writes it: '~' the
    same, a whole form, or an ending that is appended (Kránj -a), replaces
    the final vowel (Ljubljána -e, Kŕško -ega) or replaces the end of the
    word from the last occurrence of its first letter (Kóper -pra, Trbôvlje
    -velj, Grádec -dca); an accented ending takes the accent (Bôhinj -hínja
    Bohínja, Domžále -ál)."""
    if token == '~':
        return word
    if token.startswith('~') and len(token) > 1:
        token = '-' + token[1:]             # Stári Tŕg: ~ega ~a for -ega -a
    if not token.startswith('-'):
        # A whole form (Pámeče Pámeč); one that does not begin as the
        # nominative does is no genitive of it (Dédek Mràz)
        return token if plain_word(token)[:2] == plain_word(word)[:2] else None
    ending = token[1:]
    pe, pw = plain_word(ending), plain_word(word)
    if not pe or not pw or not pe.isalpha():
        return None                         # an indeclinable name (Mêri --)
    word = unicodedata.normalize('NFC', word)
    ending = unicodedata.normalize('NFC', ending)
    if pe in PLACE_INFLECTIONS:
        k = len(word) - 1 if pw[-1] in 'aeiou' else len(word)
    elif pe[0] in 'jt' and pe[1:] in PLACE_INFLECTIONS:
        k = len(word)                       # Kozár -ja, Švájncer -ja, Jóže -ta
    else:
        k = pw.rfind(pe[0])
        k = k if k > 0 else len(word)
    base = word[:k]
    if any(ch in ACCENTED for ch in ending):
        return unaccented(base) + ending
    # The accent in the replaced part moves to the same letter of the
    # ending (Škofíje -ij: Škofíj)
    for o, ch in enumerate(word[k:]):
        if ch in ACCENTED and o < len(ending) and ending[o] == ACCENTED[ch][0]:
            ending = ending[:o] + ch + ending[o + 1:]
    return base + ending


def place_forms(head, genitive, gender, extra, ambiguous):
    """The case forms of one name of SP 2001 (the singular, or the plural
    of a plural name; the accusative is the nominative or a form already
    listed), per word: (spelling, notation) pairs. The oblique cases keep
    the accent of the genitive (masculine and neuter) or the nominative
    (feminine)."""
    words = head.split()
    tokens = genitive.split()
    plural = 'mn.' in gender
    kind = gender[:1]
    skip = set(ambiguous.split())
    out = []
    for i, word in enumerate(words):
        if word in skip:
            continue
        accents = sum(1 for ch in unicodedata.normalize('NFC', word.lower()) if ch in ACCENTED)
        if accents == 0 or accents > 2:
            continue                # a preposition (ob, pri, na) or a malformed word
        nom = unicodedata.normalize('NFC', word.lower())
        token = tokens[i] if i < len(tokens) else '~'
        forms = [nom]
        gen = place_genitive(nom, token.lower())
        if token != '~' and gen:
            pn, pg = plain_word(nom), plain_word(gen)
            forms.append(gen)
            if pg.endswith('ega'):                      # Nôvo -ega, Kŕško -ega, Zgórnji -ega
                stem = gen[:-3]
                forms += [stem + 'emu', stem + 'em', stem + 'im']
            elif plural and kind == 'ž':                # Trbôvlje -velj, Domžále -ál
                stem = nom[:-1] if pn.endswith(('e', 'i')) else nom
                if nom[-1] in ACCENTED:                 # Lescè Lèsc, v Lescáh
                    forms += [stem + 'ám', stem + 'áh', stem + 'ámi']
                else:
                    forms += [stem + 'am', stem + 'ah', stem + 'ami']
            elif plural and kind == 'm':                # Hotíči -ev
                stem = nom[:-1] if pn.endswith('i') else nom
                soft = plain_word(stem).endswith(SOFT_ENDS)
                forms += [stem + ('em' if soft else 'om'), stem + 'e', stem + 'ih']
            elif plural and kind == 's':                # Brda -ŕd
                stem = nom[:-1] if pn.endswith(('a', 'e')) else nom
                forms += [stem + 'om', stem + 'ih', stem + 'i']
            elif pn.endswith('a') and pg.endswith('e'):  # Ljubljána -e, Magájna -e (m)
                stem = nom[:-1]
                forms += [stem + 'i', stem + 'o']
                if any(plain_word(e) == plain_word(stem + 'em') for e in extra.split()):
                    forms.append(stem + 'em')           # na Gorénjskem
            elif kind == 'ž' and pn.endswith('ev') and pg.endswith('ve'):
                stem = gen[:-1]                         # Lókev -kve: Lókvi, Lókvijo
                forms += [stem + 'i', stem + 'ijo']
            elif kind == 'ž' and pn[-1] not in 'aeiou' and pg.endswith('i'):
                if gen[-1] in ACCENTED:                 # Vás -í: vasjó
                    forms += [unaccented(nom) + 'jó']
                else:                                   # a feminine of the i-declension
                    forms += [nom + 'jo']
            elif kind in 'ms' and pg.endswith('a'):     # Kránj -a, Cêlje -a, Grádec -dca
                stem = gen[:-1]
                soft = plain_word(stem).endswith(SOFT_ENDS)
                forms += [stem + 'u', stem + ('em' if soft else 'om')]
            elif kind == 'm' and pg.endswith('u') and gen[-1] in ACCENTED:
                soft = pn.endswith(SOFT_ENDS)           # Grád -ú: grádu, grádom
                forms += [nom + 'u', nom + ('em' if soft else 'om')]
        for form in forms:
            accents = sum(1 for ch in form if ch in ACCENTED)
            if 1 <= accents <= 2 and all(ch in LETTERS or ch in ACCENTED for ch in form):
                out.append((plain_word(form), form, plain_word(nom), i > 0))
    return out


def read_places(paths):
    """spelling -> (notation, the nominatives of the words it is a form of)
    of the names of places and persons of SP 2001, without the spellings
    two names read differently, and the spellings of the words after the
    first of a name (Sóbota of Múrska Sóbota)."""
    readings = defaultdict(set)
    tails = set()
    lines = []
    for path in paths:
        if os.path.exists(path):
            lines += open(path, encoding='utf-8').readlines()
    for line in lines:
        if line.startswith('#') or not line.strip():
            continue
        parts = line.rstrip('\n').split('\t')
        parts += [''] * (6 - len(parts))
        _name, head, genitive, gender, extra, ambiguous = parts[:6]
        for spelling, note, nominative, tail in place_forms(head, genitive, gender, extra, ambiguous):
            readings[spelling].add((note, nominative))
            if tail:
                tails.add(spelling)
    return {sp: (next(iter(notes))[0], frozenset(n for _note, n in notes))
            for sp, notes in readings.items() if len({note for note, _n in notes}) == 1}, tails


# The endings of the declension of nouns and adjectives and of the
# possessive adjectives of names:
# a capitalized word inside a clause takes a stem of the lexicon only with
# one of them (formant_slovenian.cpp, name_base(), the same list)
NAME_ENDINGS = {'evimi', 'ovimi', 'inimi', 'evega', 'ovega', 'inega', 'evemu', 'ovemu', 'inemu',
                'evem', 'ovem', 'inem', 'evih', 'ovih', 'inih', 'evim', 'ovim', 'inim',
                'eva', 'evo', 'evi', 'eve', 'ova', 'ovo', 'ovi', 'ove', 'ina', 'ino', 'ini', 'ine',
                'ega', 'emu', 'imi', 'ima',
                'ama', 'ami', 'ema', 'oma', 'ah', 'am', 'em', 'om', 'ih', 'im', 'ov', 'ev', 'in',
                'ijo', 'jo', 'a', 'e', 'i', 'o', 'u', ''}


LONG_CLOSE = {'a': 'á', 'e': 'é', 'i': 'í', 'o': 'ó', 'u': 'ú'}


def l_suffix(stem_note, ending):
    """stress_derived_l_suffix() of formant_slovenian.cpp: a stem ending in
    a vowel before -lc, -lk, -ln, -ls, -lt (bralec, napajalnik) carries the
    stress on that vowel, long (and close), wherever its own was; before
    -lišč on the i (gledalíšče). Returns the stem's notation and the
    ending's (None: by the rules)."""
    if len(ending) < 2 or ending[0] != 'l':
        return stem_note, None
    spans = letter_spans(unicodedata.normalize('NFC', stem_note))
    plain = [PLAIN_OF.get(sp[0], sp[0]) + sp[1:].replace(GRAVE, '') for sp in spans]
    if not plain or plain[-1] not in LONG_CLOSE:
        return stem_note, None
    if ending.startswith('lišč'):
        return ''.join(plain), 'lí' + ending[2:]
    if ending[1] not in 'cknst':
        return stem_note, None
    plain[-1] = LONG_CLOSE[plain[-1]]
    return ''.join(plain), None


def declension_ending(ending):
    """An ending of NAME_ENDINGS, also after one consonant: the j or t of
    names in -r and -e (Cánkarjev, Jóžetov), the consonant after a fleeting
    e (Koróšca, Júpitrov, Štúkljem)."""
    if ending in NAME_ENDINGS:
        return True
    consonants = 2 if ending.startswith(('lj', 'nj')) else 1
    return len(ending) > consonants and ending[0] not in 'aeiou' \
        and ending[consonants:] in NAME_ENDINGS


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

    def read(self, word, name=False):
        """(notation, decided by a hand entry) or (None, False). `name`: a
        word with a capital inside a clause, which takes a stem only with
        an ending of NAME_ENDINGS."""
        if word in self.hand_exact:
            return self.hand_exact[word], True
        if word in self.exact:
            return self.exact[word], False
        if vic_surname(word) >= 0:
            return None, False      # the front end's rule, not the stems
        n = len(word)
        shortest = max(n - MAX_ENDING if n > MAX_ENDING else 1, 2)
        for length in range(n, shortest - 1, -1):
            for table, hand in ((self.hand_stems, True), (self.stems, False)):
                if name and not declension_ending(word[length:]):
                    continue
                stem = table.get(word[:length])
                if stem is not None:
                    stem, ending = l_suffix(stem, word[length:])
                    return stem + (ending or ending_by_rule(stem, word[length:])), hand
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
            proper = msd.startswith('Sl')
            # The neural network's accents of the surnames in -ović are
            # mostly wrong (Akrapóvič, Laškevíč): the front end's rule reads
            # them as the hand-corrected ones are (Pétrovič, Stojánović)
            if proper and status != 'MANUAL' and vic_surname(lemma.lower()) >= 0:
                continue
            # A form with two spellings (mesca, meseca) lists the accents
            # and transcriptions of both: take the first that spells this
            # one. Of two accentuations (dežjù, dèžju) the one its
            # transcription agrees with; a hand-corrected accent whose
            # transcription agrees with none still gives the form.
            note = None
            ipa_list = ipas.split('|')
            for trust in ((False, True) if status == 'MANUAL' else (False,)):
                for k, acc in enumerate(accs.split('|')):
                    note = notation(orth, acc, ipa_list[min(k, len(ipa_list) - 1)], trust)
                    if note is not None and strip_key(note) == orth:
                        break
                    note = None
                if note is not None:
                    break
            if note is None:
                skipped += 1
                continue
            entry = lemmas.setdefault((lemma, cat), {'freq': freq, 'forms': {}})
            # Comparatives and superlatives (lepši, najlepši, nàjpopôlnejši)
            # are not part of the adjective's stem: listed on their own when
            # frequent. The degree is the third letter of the JOS tag (Ppn...
            # positive, Ppp... comparative, Pps... superlative); the second is
            # the type, and until 2026-10-30 this tested it, so possessive
            # adjectives (Psnm...) were taken for graded ones and superlatives
            # were not
            graded = cat == 'adjective' and len(msd) > 2 and msd[2] in 'ps'
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
                (note, word_class), [0, graded, proper, False])
            form[0] += ff
            if status == 'MANUAL':
                form[3] = True
    print('lemmas', len(lemmas), 'forms skipped', skipped, file=sys.stderr)

    # The readings of every spelling, weighted by the corpus frequency of
    # the form (Gigafida 2.0, given by Sloleks for every form): a spelling
    # shared by two words is read as its more frequent form, not as the
    # form of the more frequent lemma. The adjective vesél is more frequent
    # than the verb veselíti, but veselím "I look forward" is eight times
    # as frequent as the adjective's vesélim. A proper name is read only
    # where no common word is spelled like it: the lexicon is looked up
    # without the capital, and the common word (sobóta, jeléna) is the one
    # spelled small; a capital gets the name from the tables of names
    # (Múrska Sóbota, Jêlena), names_w below.
    weight = defaultdict(dict)      # spelling -> notation -> [weight, lemma freq]
    proper_weight = defaultdict(dict)
    names_w = defaultdict(lambda: defaultdict(int))     # spelling -> notation -> hand-accented name's ff
    names_lemma = {}                                    # (spelling, notation) -> a name; spelling -> all
    common_w = defaultdict(int)                         # spelling -> ff of the common words' forms
    adjective_w = defaultdict(int)                      # ... of the adjectives' among them
    unnamed = set()                                     # spellings of COMMON_CAPITALIZED
    by_class = defaultdict(lambda: defaultdict(lambda: defaultdict(int)))
    listed = []                     # per lemma: (freq, [(orth, note)]) for the stems
    for (lemma, cat), entry in lemmas.items():
        orths = entry['forms'].keys()
        forms = []
        for orth, readings in entry['forms'].items():
            best = None
            for (note, word_class), (ff, graded, proper, manual) in readings.items():
                # The rare forms (the dual, the rarer cases) are left to the
                # stems and the rules
                if ff < form_min and orth != lemma.lower():
                    continue
                if graded and ff < 20:
                    continue
                note = correct(lemma, orth, mark_fleeting_e(orth, note, orths, cat))
                w = (proper_weight if proper else weight)[orth].setdefault(note, [0, 0])
                w[0] += ff
                w[1] = max(w[1], entry['freq'])
                if proper:
                    if manual:
                        names_w[orth][note] += ff
                        names_lemma.setdefault(orth, set()).add(lemma.lower())
                        names_lemma.setdefault((orth, note), lemma)
                else:
                    common_w[orth] += ff
                    if cat == 'adjective':
                        adjective_w[orth] += ff
                    if lemma in COMMON_CAPITALIZED:
                        unnamed.add(orth)
                if word_class and not proper:
                    by_class[orth][word_class][note] += ff
                if not graded and (best is None or ff > best[0]):
                    best = (ff, note)
            if best:
                forms.append((orth, best[1]))
        listed.append((entry['freq'], forms))
    for orth, rs in proper_weight.items():
        if orth not in weight:
            weight[orth] = rs
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
    names = {'weights': names_w, 'lemma': names_lemma, 'common': common_w,
             'adjective': adjective_w, 'unnamed': unnamed}
    return len(lemmas), truth, stems, verb_noun, names


def generate(forms_path, out_path, manual_min=50, auto_min=2000, form_min=3, cpp_path=None,
             places_path=None, names_report=None):
    lemma_count, truth, stems, verb_noun, names = build(forms_path, manual_min, auto_min, form_min)

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
        # a word with ć or đ takes a stem only with an ending of the
        # declension, as a name does (formant_slovenian.cpp, has_name_letter())
        got, hand = reader.read(orth, name='ć' in orth or 'đ' in orth)
        if got is None or norm(got) != norm(note):
            if not (hand and orth in hand_exact):
                exact[orth] = note
    print('spellings', len(truth), 'exact', len(exact), 'stems', len(stems),
          'verb/noun', len(verb_noun), file=sys.stderr)

    # The tables of names, for a word written with a capital: the forms of
    # the hand-accented names of Sloleks (persons, places, regions) whose
    # reading the lexicon does not give, mostly because a common word is
    # spelled like them (Sóbota and sobóta, Jêlena and jeléna, Róman and
    # román). A form of a name at least three times as frequent as the
    # common word is read at the head of a clause too, where any word has
    # its capital (Zóran, Jêlena, Angéla); a less frequent one only inside
    # the clause, and so is the second word of a name of a place of SP
    # 2001, which hardly ever opens a clause (Múrska Sóbota: "Sobota je
    # lep dan" is the day); a name whose
    # clashing forms are together a hundred times rarer than the common
    # word's not at all (Kót and kòt "as"). Not where a common adjective
    # more frequent than the name is spelled like it, since adjectives
    # begin many names of places with their own reading (Stáre Žále, Béla
    # krajína; Starè and Belè are surnames; but Zóran and Déjan, not the
    # participles zorán, deján), and not the words of COMMON_CAPITALIZED.
    names_w, common_w = names['weights'], names['common']
    places, place_tails = read_places(places_path or [])
    clashes = defaultdict(list)     # name -> [(spelling, notation, its weight, the common words')]
    for orth, notes in names_w.items():
        pnote, pw = max(notes.items(), key=lambda kv: (kv[1], kv[0]))
        got, _hand = reader.read(orth, name=True)
        if got is not None and norm(got) == norm(pnote):
            continue
        cw = common_w.get(orth, 0)
        adjective = names['adjective'].get(orth, 0)
        if orth in names['unnamed'] or (adjective * 2 > cw and adjective > pw):
            continue
        clashes[names['lemma'][(orth, pnote)]].append((orth, pnote, pw, cw))
    names_any, names_mid = {}, {}
    for forms in clashes.values():
        if sum(cw for *_r, cw in forms) > 100 * sum(pw for _o, _n, pw, _c in forms):
            continue
        for orth, pnote, pw, cw in forms:
            (names_any if pw >= 3 * cw and orth not in place_tails else names_mid)[orth] = pnote

    # The names of places and persons of Slovenski pravopis 2001
    # (slovenian_places.tsv, slovenian_persons.tsv), the norm, in all their
    # cases: they replace the readings of Sloleks
    # (Bôhinj, Bohínja, not Bohínj) except where SP itself accents the
    # word otherwise in another name (Cerkljè, Cérklje) or a person's name
    # (the surname Léskovec, Leskôvec pri Krškem), which the file leaves out,
    # and where the spelling is a form of another name in Sloleks (Koréna,
    # of the surname Korén; Zgórnja Kórena). A spelling of a common word
    # is read so only inside a clause, an adjective of names (Stáre,
    # Vélike) not at all where it is a frequent common adjective.
    place_count = 0
    def same(a, b):
        """The same reading but for the schwa and the [w], which only the
        lexicon's notation knows (cérkəv, SP Cérkev)."""
        return norm(a).replace(SCHWA, 'e') == norm(b).replace(SCHWA, 'e')

    for orth, (note, nominatives) in places.items():
        if any(lemma not in nominatives for lemma in names['lemma'].get(orth, ())):
            continue
        got, _hand = reader.read(orth, name=True)
        if got is not None and same(got, note):
            names_any.pop(orth, None)
            names_mid.pop(orth, None)
            continue
        if orth in names_any or orth in names_mid:
            (names_any if orth in names_any else names_mid)[orth] = note
            place_count += 1
            continue
        if orth in place_tails and got is not None:
            names_mid[orth] = note          # Lóška dolína; not at the head
            place_count += 1
            continue
        cw = common_w.get(orth, 0)
        adjective = names['adjective'].get(orth, 0)
        if orth in names['unnamed'] or (adjective * 2 > cw > 200):
            continue
        # SP gives no frequencies: a spelling of any common word (Rávnam
        # of Rávne, ravnám "I act") is the name only inside a clause
        (names_mid if cw else names_any)[orth] = note
        place_count += 1
    print('names', len(names_any), 'inside a clause', len(names_mid),
          'from the places of SP', place_count, file=sys.stderr)
    if names_report:
        with open(names_report, 'w', encoding='utf-8') as rep_out:
            for table, where in ((names_any, 'anywhere'), (names_mid, 'inside')):
                for orth in sorted(table, key=lambda o: -common_w.get(o, 0)):
                    got, _hand = reader.read(orth, name=True)
                    pw = max(names_w[orth].values()) if orth in names_w else ''
                    rep_out.write('%s\t%s\t%s\t%s\tname %s\tcommon %s%s\n' % (
                        where, orth, table[orth], got, pw, common_w.get(orth, 0),
                        '\tSP' if orth in places else ''))

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
    any_keys, any_blocks = front_coded(names_any)
    mid_keys, mid_blocks = front_coded(names_mid)
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
        out.write('// Names (persons, places, regions) read so only when written with a\n')
        out.write('// capital: anywhere, and only inside a clause (not at its head)\n')
        for name, keys, blocks in (('NAMES', any_keys, any_blocks),
                                   ('NAMES_INSIDE', mid_keys, mid_blocks)):
            out.write('const size_t SLOVENIAN_%s_COUNT = %d;\n' % (name, len(keys)))
            out.write('const char* const SLOVENIAN_%s[] = {\n' % name)
            for block in blocks:
                out.write('    u8"%s",\n' % block)
            if not blocks:
                out.write('    u8"",\n')
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
        names_report = None
        args = argv[4:]
        while args:
            if args[0] == '--manual-min':
                manual_min = int(args[1]); args = args[2:]
            elif args[0] == '--auto-min':
                auto_min = int(args[1]); args = args[2:]
            elif args[0] == '--form-min':
                form_min = int(args[1]); args = args[2:]
            elif args[0] == '--names-report':
                names_report = args[1]; args = args[2:]
            else:
                print('unknown option', args[0], file=sys.stderr); return 2
        root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
        generate(argv[2], argv[3], manual_min, auto_min, form_min,
                 os.path.join(root, 'src', 'formant', 'formant_slovenian.cpp'),
                 [os.path.join(root, 'tools', 'formant', 'slovenian_places.tsv'),
                  os.path.join(root, 'tools', 'formant', 'slovenian_persons.tsv')], names_report)
    else:
        print(__doc__, file=sys.stderr)
        return 2
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
