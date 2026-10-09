#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
proper_names.py - The place-name and personal-name tables of the formant voices.

Reads tools/formant/places.tsv (towns, villages and regions) and
tools/formant/persons.tsv (given names and surnames), both with their
dictionary accents, and writes src/formant/formant_proper_names.inc: the tables
PLACES_* and PERSONS_* (COMMON, CROATIAN, SERBIAN, BOSNIAN) in the notation of
src/formant/formant_lexicon.cpp. The front end uses these entries only for
words written with a capital letter (see Frontend::process), and a personal
name wins over a place of the same spelling.

Each row of places.tsv is one place for one or more languages:

    name  country  langs  nominative  genitive  dl  gender  number  mid  source  [without]

    Knin    HR  hr,sr,bs  Knȋn      Knína   -   m  -   -  HJP
    Rijeka  HR  hr,sr,bs  Rijéka    -       -   ž  -   -  HJP
    Vinkovci HR hr,sr,bs  Vȋnkōvci  Vȋnkovācā - m  pl  -  HJP

- nominative and genitive carry the dictionary's accents (ȁ à ȃ á, ā for a
  length after the accent); the genitive gives the accent of the other cases
  where it moves (Knȋn, Knína, Knínu, Knínom). A name of several words
  (Slàvōnskī Brȏd) declines its adjectives and its head noun; a word without
  accent marks (a personal name inside a place name) is left alone.
- dl: the dative and locative where the dictionary gives them (Líci).
- gender: m, ž, sr; number: pl for a plural name (Vinkovci, Delnice).
- mid: "=" for a name spelled like a common word (Bȃr, bar): its entries
  are not used for the first word of a clause; "=Ide,Idi" for some forms
  only (the cases of Ìda spelled like the verb forms ide, idi).
- without: forms left out, comma-separated (a form that is also a frequent
  word or a personal name of another accent: Nína of Nȋn against Nȉna).
- langs: the voices the row is for. The Croatian voice drops the length
  after the accent, as everywhere in the lexicon; the Serbian and Bosnian
  voices keep it.

Each row of persons.tsv is one given name or surname:

    name  kind  langs  nominative  genitive  mid  source  [without]

    Ivan     m  hr,sr,bs  Ìvan     -  -  HJP
    Marija   ž  hr,sr,bs  Màrija   -  -  HJP
    Kovačević p  hr,sr,bs Kováčević -  -  HJP

- kind: m (a man's name), ž (a woman's name), p (a surname). Names and
  surnames keep the accent of the nominative in every case (Ìvan, Ìvana;
  Màrija, Màrije; Kováčević, Kováčevića; Kránjec, Kránjca); a genitive is
  given where the dictionaries have one (Máte, Mátē; Đȏrđe, Đȏrđa). Names
  in -e without one (Ánte, Ánti or Pàvle, Pàvla?) and in -ar, -ak, -al
  (Pètar, Pètra but Nòvāk, Nòvāka) keep only their nominative.

The rows were collected in October 2026 from Hrvatski jezični portal,
Wiktionary and Vuk's Srpski rječnik (through Raskovnik) for the names whose
forms the rules of the front end got wrong; see "Place names" and "Personal
names" in docs/formant.md. Two names may not give one spelling two accents:
the script reports such a pair and leaves the form out.

Usage:
    tools/formant/proper_names.py      (writes src/formant/formant_proper_names.inc)
"""
import os, re, sys, unicodedata

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TSV = os.path.join(ROOT, 'tools', 'formant', 'places.tsv')
PERSONS_TSV = os.path.join(ROOT, 'tools', 'formant', 'persons.tsv')
INC = os.path.join(ROOT, 'src', 'formant', 'formant_proper_names.inc')

GRAVE, ACUTE, DGRAVE, IBREVE, MACRON, CIRC = '̀', '́', '̏', '̑', '̄', '̂'
VOWELS = set('aeiouAEIOU')
LANGS = ('hr', 'sr', 'bs')
PREPOSITIONS = {'na', 'kod', 'pri', 'od', 'u', 'ob', 'pod', 'nad', 'v', 'do', 'iz', 'uz', 'za', 'k',
                's', 'sa', 'pored'}


class Form:
    """A word with its accent: letters, the stressed letter, tone (F falling,
    R rising) and the long letters."""

    def __init__(self, plain, stress, tone, longs):
        self.plain, self.stress, self.tone, self.longs = plain, stress, tone, set(longs)

    def stressed_long(self):
        return self.stress in self.longs

    def marked(self, post=True):
        """The form with dictionary accent marks."""
        out = []
        for i, ch in enumerate(self.plain):
            out.append(ch)
            if i == self.stress:
                if self.tone == 'F':
                    out.append(IBREVE if i in self.longs else DGRAVE)
                else:
                    out.append(ACUTE if i in self.longs else GRAVE)
            elif post and i in self.longs and i > self.stress:
                out.append(MACRON)
        return unicodedata.normalize('NFC', ''.join(out))

    def lex(self, post=True):
        """The form in the lexicon's notation: ^ or / before the stressed
        vowel, : after a long one."""
        out = []
        for i, ch in enumerate(self.plain):
            if i == self.stress:
                out.append('^' if self.tone == 'F' else '/')
            out.append(ch)
            if i in self.longs and (i == self.stress or (post and i > self.stress)):
                out.append(':')
        return ''.join(out)


def analyse(accented, vuk=False):
    """An accented form as a Form (None without an accent). With vuk=True an
    inverted breve after the accent is a length (Vuk's notation)."""
    text = unicodedata.normalize('NFC', accented.strip())
    letters, stress, tone, longs = [], None, None, set()
    for ch in unicodedata.normalize('NFD', text):
        if not unicodedata.combining(ch):
            letters.append(ch)
            continue
        if not letters:
            continue
        base = letters[-1]
        nucleus = base in VOWELS or base in 'rR'
        if ch == ACUTE and base in 'cC':
            letters[-1] = unicodedata.normalize('NFC', base + ch)      # ć
        elif ch in (GRAVE, ACUTE, DGRAVE, IBREVE, CIRC) and nucleus:
            index = len(letters) - 1
            if stress is not None:
                if ch in (IBREVE, CIRC, ACUTE) or vuk:
                    longs.add(index)
                continue
            stress = index
            tone = 'F' if ch in (DGRAVE, IBREVE, CIRC) else 'R'
            if ch in (ACUTE, IBREVE, CIRC):
                longs.add(index)
        elif ch == MACRON and nucleus:
            longs.add(len(letters) - 1)
        else:
            letters[-1] = unicodedata.normalize('NFC', base + ch)      # č, š, ž
    if stress is None:
        return None
    return Form(''.join(letters), stress, tone, longs)


def replace_end(form, cut, ending):
    """Drop `cut` letters and add an ending; the accent stays where it is."""
    if cut and len(form.plain) - cut <= form.stress:
        return None
    base = form.plain[:len(form.plain) - cut] if cut else form.plain
    return Form(base + ending, form.stress, form.tone, {i for i in form.longs if i < len(base)})


def instrumental(stem):
    return 'em' if stem.endswith(('c', 'č', 'ć', 'đ', 'j', 'š', 'ž', 'lj', 'nj', 'dž')) else 'om'


# The spelling of the consonant before c once the vowel between them has
# gone (Ostròžac, Ostròšca; Grȁbac, Grȁpca; Grádac, Gráca)
ASSIMILATED = {'ž': 'š', 'z': 's', 'b': 'p', 'g': 'k', 'd': ''}


def fleeting(nom):
    """Without the dictionary's genitive, a masculine name in -ac, -ec, -ak
    may lose that vowel (Ferdinàndovac, Ferdinàndovca) or keep it (Pȁkrac,
    Pȁkraca); the genitive with the vowel gone, for both to be listed."""
    p = nom.plain
    if len(p) < 4 or p[-2:] not in ('ac', 'ec', 'ak') or nom.stress >= len(p) - 2:
        return []
    before = p[-3]
    if before in VOWELS:
        return []
    stem = p[:-3] + ASSIMILATED.get(before, before) if p[-1] == 'c' else p[:-2]
    form = Form(stem + p[-1] + 'a', nom.stress, nom.tone, {i for i in nom.longs if i < len(stem)})
    return [form] if form.stress < len(stem) else []


def decline(nom, gen, gender, plural, adjectival, dl=None):
    """The case forms (no vocative) of one word as Forms. gen gives the
    accent of the oblique cases."""
    out = [nom]
    p = nom.plain

    def add(form):
        if form is not None and form.plain not in [f.plain for f in out]:
            out.append(form)

    if adjectival:
        if plural:
            endings = ('ih', 'im', 'ima')
        elif gender == 'ž':
            endings = ('e', 'oj', 'u', 'om')
        else:
            endings = ('og', 'oga', 'om', 'ome', 'im')
        if p.endswith(('i', 'a', 'o', 'e')):
            for e in endings:
                add(replace_end(nom, 1, e))
        return out
    if plural:
        if gender == 'm' and p.endswith('i'):
            if p.endswith('ci') and len(p) > 3 and p[-3] in VOWELS:
                # Bošnjáci, Bošnjáka, Bošnjáke: the plural of a noun in -k
                add(gen or replace_end(nom, 2, 'ka'))
                add(replace_end(nom, 2, 'ke'))
            elif p.endswith('ci') and len(p) > 3:
                # Vȋnkōvci, Vȋnkovācā: the vowel of -ac comes back
                add(gen or replace_end(nom, 2, 'aca'))
                add(replace_end(nom, 1, 'e'))
            else:
                add(gen or replace_end(nom, 1, 'a'))
                add(replace_end(nom, 1, 'e'))
            add(replace_end(nom, 1, 'ima'))
        elif gender == 'ž' and p.endswith('e'):
            add(gen or replace_end(nom, 1, 'a'))
            add(replace_end(nom, 1, 'ama'))
        elif gender == 'sr' and p.endswith('a'):
            add(gen)
            add(replace_end(nom, 1, 'ima'))
        return out
    if gender == 'm':
        if p.endswith('o'):                          # Đȁkovo-like masculines
            g = gen or replace_end(nom, 1, 'a')
        elif p[-1] in VOWELS:
            return out
        else:
            g = gen or replace_end(nom, 0, 'a')
        for genitive in [g] + ([] if gen else fleeting(nom)):
            add(genitive)
            if genitive is not None:
                add(replace_end(genitive, 1, 'u'))
                add(replace_end(genitive, 1, instrumental(genitive.plain[:-1])))
        return out
    if gender == 'ž':
        if not p.endswith('a'):
            return out
        g = gen or replace_end(nom, 1, 'e')
        add(g)
        base = g or nom
        add(replace_end(base, 1, 'u'))
        add(replace_end(base, 1, 'om'))
        if dl is not None:
            add(dl)
        else:
            add(replace_end(base, 1, 'i'))
            stem = base.plain[:-1]
            soft = {'k': 'c', 'g': 'z', 'h': 's'}
            if stem and stem[-1] in soft and len(stem) - 1 > base.stress:
                add(replace_end(base, 2, soft[stem[-1]] + 'i'))   # Rijéci, Pȍžezi
        return out
    if gender == 'sr':
        if not p.endswith(('o', 'e')):
            return out
        g = gen or replace_end(nom, 1, 'a')
        add(g)
        if g is not None:
            add(replace_end(g, 1, 'u'))
            add(replace_end(g, 1, 'em' if p.endswith('e') else 'om'))
        return out
    return out


ADJECTIVE_ENDINGS = ('skī', 'skā', 'skō', 'čkī', 'čkā', 'čkō', 'škī', 'škā', 'škō')

# Adjectives of many names (Nova Gradiška, Veliki Grđevac, Donji Miholjac,
# Sveti Ivan Zelina) are common words with their own accents; the names do
# not change them.
GENERIC = {stem + ending
           for stem in ('donj', 'gornj', 'dolnj', 'srednj', 'mal', 'velik', 'nov', 'star', 'svet',
                        'crn', 'bijel', 'bel', 'dug', 'suh', 'bosansk', 'hrvatsk', 'srpsk', 'lijev',
                        'desn', 'mrtv', 'zlatn', 'prv', 'drug', 'nov', 'gorn', 'dolj')
           for ending in ('i', 'a', 'o', 'e', 'og', 'oga', 'om', 'ome', 'oj', 'u', 'im', 'ih', 'ima',
                          'eg', 'ega', 'em')} | {'sv'}


ADJECTIVE_SUFFIXES = ('ski', 'ška', 'ški', 'ško', 'ške', 'ska', 'sko', 'ske', 'čki', 'čka', 'čko',
                      'čke', 'ćki', 'ćka', 'ćko', 'ćke')


def is_adjective(word, form):
    """A word of a name that is an adjective: Slàvōnskī, Bijȇlō, Bȁbina,
    Radobojski, Bánovā."""
    p = form.plain.lower()
    return word.endswith(('ī', 'ā', 'ō', 'ē')) or p.endswith(ADJECTIVE_SUFFIXES)


def words_of(nominative, genitive, dl, gender, plural, persons=()):
    """The declined forms of the words of a name, as lists of Forms. In a
    name of several words the head noun is the last word that is not an
    adjective (Bȁbina Gréda, Brȅgi Radobojskī); the adjectives are declined
    with it, generic adjectives (Nova, Veliki, Donji) and other words (a
    personal name: Marija Bistrica) are left to the rest of the lexicon."""
    words = []
    for w in nominative.split():
        if w.lower() in PREPOSITIONS:
            break                               # Biograd na Moru
        words += [x for x in w.split('-') if x]
    gwords = []
    for w in (genitive or '').split():
        gwords += [x for x in w.split('-') if x]
    forms = [analyse(w) for w in words]
    if len(words) == 1:
        nom = forms[0]
        if nom is None:
            return []
        gen = analyse(gwords[0]) if len(gwords) == 1 else None
        # adjectival names: Ìmotskī, Mȁkarskā, Vìsokō (G Vìsokōga)
        adjectival = words[0].endswith(ADJECTIVE_ENDINGS) or \
            (gender == 'ž' and words[0].endswith('ā') and not plural) or \
            (gen is not None and gen.plain.endswith(('og', 'oga', 'eg', 'ega')))
        d = analyse(dl) if dl else None
        return [decline(nom, None if adjectival else gen, gender, plural, adjectival, d)]
    def fits(form):
        p = form.plain
        if plural:
            return p.endswith({'m': 'i', 'ž': 'e', 'sr': 'a'}.get(gender, 'i'))
        if gender == 'ž':
            return p.endswith('a')
        if gender == 'sr':
            return p.endswith(('o', 'e'))
        return p[-1] not in VOWELS or p.endswith('o')
    # the head noun: the last word that is not an adjective and fits the
    # gender (Bȕk Vláka, Cȉsta Prȏvo: the word after the head is left alone)
    head = None
    for i in range(len(words) - 1, -1, -1):
        if forms[i] is not None and not is_adjective(words[i], forms[i]) and fits(forms[i]):
            head = i
            break
    result = []
    for i, (w, nom) in enumerate(zip(words, forms)):
        if nom is None or nom.plain.lower() in GENERIC or nom.plain in persons:
            continue
        if i == head:
            gen = analyse(gwords[i]) if len(gwords) == len(words) else None
            result.append(decline(nom, gen, gender, plural, False))
        elif head is not None and (is_adjective(w, nom) or (i < head and nom.plain[-1] in 'aeio')):
            result.append(decline(nom, None, gender, plural, True))
    return result


def decline_person(nom, gen, kind):
    """The case forms of a given name or surname: the accent of the
    nominative (or of the genitive where one is given) in every case."""
    out = [nom]
    p = nom.plain

    def add(form):
        if form is not None and form.plain not in [f.plain for f in out]:
            out.append(form)

    def a_declension(g):                             # -a, -u, -om/-em
        add(g)
        if g is not None:
            add(replace_end(g, 1, 'u'))
            add(replace_end(g, 1, instrumental(g.plain[:-1])))

    if p.endswith('a'):                              # Màrija, Nìkola, Kȑleža
        base = gen or replace_end(nom, 1, 'e')
        add(base)
        for e in ('i', 'u', 'om'):
            add(replace_end(base, 1, e))
    elif kind == 'ž':
        pass                                         # Ìnēs, Kàrmen: not declined
    elif p.endswith('ije'):                          # Ignácije, Ignácija, Ignácijem
        a_declension(gen or replace_end(nom, 1, 'a'))
    elif p.endswith('e'):
        if gen is not None and gen.plain.endswith('e'):
            for e in ('i', 'u', 'om'):               # Máte, Mátē: Máti, Mátu, Mátom
                add(replace_end(nom, 1, e))
        elif gen is not None:
            a_declension(gen)                        # Đȏrđe, Đȏrđa
        # otherwise Ánte, Ánti or Pàvle, Pàvla: the nominative only
    elif p.endswith('o'):                            # Mȃrko, Mȃrka; Ìvo, Ìve
        if gen is not None:
            g = gen
        elif p.endswith('ko') and kind == 'm':
            g = replace_end(nom, 1, 'a')
        else:
            return out                               # Ivo, Ive or Marko, Marka?
        if g.plain.endswith('a'):
            a_declension(g)
        else:
            add(g)
            for e in ('i', 'u', 'om'):
                add(replace_end(g, 1, e))
    elif p[-1] not in VOWELS:
        if gen is not None:
            a_declension(gen)
        elif p.endswith(('ec', 'ac')) and nom.stress < len(p) - 2:
            for g in fleeting(nom):                  # Kránjec, Kránjca; Cesárec, Cesárca
                a_declension(g)
        elif p.endswith(('ar', 'ak', 'al')) and nom.stress < len(p) - 2:
            return out                               # Pètar, Pètra but Nòvāk, Nòvāka
        else:
            a_declension(replace_end(nom, 0, 'a'))
    return out


def group(lex_forms):
    """Join forms that share everything up to their last mark into
    "stem|ending|ending" entries."""
    groups = {}
    for f in lex_forms:
        last = max(f.rfind('^'), f.rfind('/'), f.rfind(':'))
        key = f[:last + 1]
        groups.setdefault(key, []).append(f)
    out = []
    for key, forms in groups.items():
        if len(forms) == 1:
            out.append(forms[0])
            continue
        prefix = os.path.commonprefix(forms)
        if len(prefix) < len(key):
            prefix = key
        out.append(prefix + '|' + '|'.join(f[len(prefix):] for f in forms))
    return out


def read_rows(path):
    rows = []
    for number, line in enumerate(open(path, encoding='utf-8'), 1):
        if not line.strip() or line.startswith('#'):
            continue
        f = line.rstrip('\n').split('\t')
        if len(f) < 10:
            sys.exit('%s:%d: expected 10 columns' % (path, number))
        f = [x if x != '-' else '' for x in f]
        rows.append(dict(name=f[0], country=f[1], langs=f[2].split(','), nom=f[3], gen=f[4],
                         dl=f[5], gender=f[6], plural=f[7] == 'pl',
                         mid=f[8] == '=' or (set(f[8][1:].split(',')) if f[8].startswith('=') else False),
                         source=f[9], without=set(f[10].split(',')) if len(f) > 10 and f[10] else set()))
    return rows


def read_persons(path):
    rows = []
    if not os.path.exists(path):
        return rows
    for number, line in enumerate(open(path, encoding='utf-8'), 1):
        if not line.strip() or line.startswith('#'):
            continue
        f = line.rstrip('\n').split('\t')
        if len(f) < 7:
            sys.exit('%s:%d: expected 7 columns' % (path, number))
        f = [x if x != '-' else '' for x in f]
        rows.append(dict(name=f[0], kind=f[1], langs=f[2].split(','), nom=f[3], gen=f[4],
                         mid=f[5] == '=' or (set(f[5][1:].split(',')) if f[5].startswith('=') else False),
                         source=f[6],
                         without=set(f[7].split(',')) if len(f) > 7 and f[7] else set()))
    return rows


def main():
    # (kind, name) -> language -> (lex forms without and with the length
    # after the accent, mid); kind is "place" or "person"
    by_name = {}
    for row in read_rows(TSV):
        words = words_of(row['nom'], row['gen'], row['dl'], row['gender'], row['plural'])
        forms = [f for ws in words for f in ws if f.plain not in row['without']]
        if forms:
            for lang in row['langs']:
                by_name.setdefault(('place', row['name']), {})[lang] = (
                    list(dict.fromkeys(f.lex(False) for f in forms)),
                    list(dict.fromkeys(f.lex(True) for f in forms)), row['mid'])
    for row in read_persons(PERSONS_TSV):
        nom = analyse(row['nom'])
        if nom is None:
            sys.exit('%s: no accent in "%s"' % (row['name'], row['nom']))
        gen = analyse(row['gen']) if row['gen'] else None
        forms = [f for f in decline_person(nom, gen, row['kind']) if f.plain not in row['without']]
        for lang in row['langs']:
            by_name.setdefault(('person', row['name']), {})[lang] = (
                list(dict.fromkeys(f.lex(False) for f in forms)),
                list(dict.fromkeys(f.lex(True) for f in forms)), row['mid'])

    def plain(lex):
        return re.sub(r"[\^/:']", '', lex)

    def drop(key, lang, p):
        short, full, mid = by_name[key][lang]
        keep = [i for i, x in enumerate(short) if plain(x) != p]
        by_name[key][lang] = ([short[i] for i in keep], [full[i] for i in keep], mid)

    # One spelling may not have two accents (the later entry would silently
    # win). Between two places or two persons the form is left out and
    # reported; a person's name wins over a place.
    for lang in LANGS:
        seen = {}
        for key, langs in by_name.items():
            if lang in langs:
                short, full, mid = langs[lang]
                for f in (short if lang == 'hr' else full):
                    seen.setdefault(plain(f), set()).add((f, key))
        for p, uses in sorted(seen.items()):
            if len({f for f, _ in uses}) == 1:
                continue
            persons = {f for f, key in uses if key[0] == 'person'}
            if len(persons) == 1:
                for f, key in uses:
                    if key[0] == 'place':
                        drop(key, lang, p)
                continue
            print('%s: %s has two accents (%s); left out' %
                  (lang, p, ', '.join(sorted('%s in %s' % (f, key[1]) for f, key in uses))),
                  file=sys.stderr)
            for f, key in uses:
                drop(key, lang, p)
    def is_mid(mid, f):
        return mid is True or (bool(mid) and plain(f) in mid)
    mid_plain = {plain(f) for langs in by_name.values()
                 for short, full, mid in langs.values() for f in short if is_mid(mid, f)}

    def emit(forms, mid):
        marked = [f for f in forms if plain(f) in mid_plain]
        other = [f for f in forms if f not in marked]
        return ['=' + e for e in group(marked)] + group(other)

    tables = {}
    for kind, prefix in (('place', 'PLACES'), ('person', 'PERSONS')):
        t = {'COMMON': [], 'CROATIAN': [], 'SERBIAN': [], 'BOSNIAN': []}
        lang_table = {'hr': 'CROATIAN', 'sr': 'SERBIAN', 'bs': 'BOSNIAN'}
        for key in sorted((k for k in by_name if k[0] == kind), key=lambda k: k[1].lower()):
            langs = by_name[key]
            shorts = {tuple(v[0]) for v in langs.values()}
            if len(langs) == 3 and len(shorts) == 1:
                short, _, mid = langs['hr']
                t['COMMON'] += emit(short, mid)
                for lang in ('sr', 'bs'):
                    _, full, mid = langs[lang]
                    if full != short:
                        t[lang_table[lang]] += emit(full, mid)
            else:
                for lang, (short, full, mid) in langs.items():
                    t[lang_table[lang]] += emit(short if lang == 'hr' else full, mid)
        for name, entries in t.items():
            tables['%s_%s' % (prefix, name)] = entries

    out = ['// -*- coding: utf-8 -*-',
           '// formant_proper_names.inc - Generated by tools/formant/proper_names.py from',
           '// tools/formant/places.tsv and tools/formant/persons.tsv; do not edit by hand.',
           '//',
           '// Towns, villages and regions (PLACES_*) and given names and surnames',
           '// (PERSONS_*) whose case forms the rules get wrong, with the accents of the',
           '// dictionaries (see "Place names" and "Personal names" in docs/formant.md).',
           '// Used only for words written with a capital letter; "=" marks a name',
           '// spelled like a common word, not used for the first word of a clause.',
           '']
    for table, entries in tables.items():
        entries = list(dict.fromkeys(entries)) or ['']    # an array cannot be empty
        out.append('const char* const %s[] = {' % table)
        line = '   '
        for e in entries:
            item = ' u8"%s",' % e
            if len(line) + len(item) > 96:
                out.append(line)
                line = '   '
            line += item
        out.append(line)
        out.append('};')
        out.append('')
    open(INC, 'w', encoding='utf-8').write('\n'.join(out))
    print('%d places, %d persons; %s' % (
        sum(1 for k in by_name if k[0] == 'place'), sum(1 for k in by_name if k[0] == 'person'),
        ', '.join('%s %d' % (t, len(v)) for t, v in tables.items())))


if __name__ == '__main__':
    main()
