#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
verb_accents.py - Entries for the VERBS table of src/formant/formant_lexicon.cpp.

For each infinitive given, looks the verb up in Hrvatski jezicni portal
(hjp.znanje.hr) and prints its stem with the accent of the infinitive, the
conjugation class and what the dictionary says about the present and the
passive participle, in the notation of the table:

    urediti    ur'e:d=i<pn     uréditi, prez. ùrēdīm, trp. ùrēđen; twin: ured

A verb whose accent is short and on the first syllable needs no entry and is
reported as such. The "n" flag (the imperative is also a form of a noun) comes
from asking the dictionary for the imperative form. Irregular verbs are
reported and left to the lexicon. Answers are cached in ~/.cache/laprdus-hjp;
requests are half a second apart.

Usage:
    verb_accents.py urediti otvoriti procitati ...    (with diacritics)
"""
import sys, re, os, html, time, hashlib, subprocess, unicodedata

CACHE = os.path.expanduser('~/.cache/laprdus-hjp')

def lookup(word):
    """Summary line of the dictionary's answer for a word or form ('' if none)."""
    os.makedirs(CACHE, exist_ok=True)
    f = os.path.join(CACHE, hashlib.md5(word.encode()).hexdigest() + '.txt')
    if os.path.exists(f):
        return open(f, encoding='utf-8').read()
    out = subprocess.run(['curl', '-s', '-m', '25', '-A', 'Mozilla/5.0',
                          '--data-urlencode', 'word=' + word, '--data', 'search=+',
                          'https://hjp.znanje.hr/index.php?show=search'],
                         capture_output=True).stdout.decode('utf-8', 'replace')
    m = re.search(r'<meta name="description" content="([^"]*)"', out)
    d = html.unescape(m.group(1)) if m else ''
    if d.startswith('Hrvatski jezični portal'):
        d = ''
    if out:
        open(f, 'w', encoding='utf-8').write(d)
    time.sleep(0.5)
    return d

GRAVE,ACUTE,DGRAVE,IBREVE,MACRON,CIRC,MACRON_BELOW='̀','́','̏','̑','̄','̂','̱'
STRESS={GRAVE:('rise',False),ACUTE:('rise',True),DGRAVE:('fall',False),IBREVE:('fall',True),CIRC:('fall',True)}
def analyse(acc):
    """accented form -> (plain, stress letter index or None, long?, set of long letter indexes)"""
    plain=[];stress=None;slong=False;longs=set()
    acc=re.sub(r'\d+$','',acc).replace('\u1e95','r'+GRAVE)
    for ch in unicodedata.normalize('NFD',acc):
        if ch==ACUTE and plain and plain[-1] in 'cCsSzZnN':
            plain[-1]=unicodedata.normalize('NFC',plain[-1]+ch)
        elif ch in STRESS:
            stress=len(plain)-1; slong=STRESS[ch][1]
            if slong: longs.add(stress)
        elif ch in (MACRON,MACRON_BELOW): longs.add(len(plain)-1)
        elif unicodedata.combining(ch):
            # caron etc. belong to the letter
            plain[-1]=unicodedata.normalize('NFC',plain[-1]+ch)
        else: plain.append(ch)
    return ''.join(plain),stress,slong,longs
V='aeiou'
def nuclei(w):
    out=[]
    for i,c in enumerate(w):
        if c in V: out.append(i)
        elif c=='r':
            p=w[i-1] if i>0 else ''; n=w[i+1] if i+1<len(w) else ''
            if p not in V and n not in V and p!='r' and n!='r' and len(w)>1: out.append(i)
    return out
def head(d,inf):
    # "urediti: uréditi (se) svrš. 〈prez. ùrēdīm ..." ; with several lemmas "a, b: X ... Y ..."
    m=re.match(r'^([^:]*): (\S+)',d)
    return (m.group(1).split(', '),m.group(2)) if m else ([],'')
def field(d,name):
    m=re.search(name+r'\.? (\S+?)[,〉 ]',d)
    return m.group(1) if m else None
def mark(stem,idx,long_):
    return stem[:idx]+"'"+stem[idx]+(':' if long_ else '')+stem[idx+1:]
def forms_of(l):
    out={l, l+'i', l+'a'}
    if l[-1] in 'aoe': out|={l[:-1]+'i', l[:-1]+'a'}
    if len(l)>3 and l[-2]=='a': out|={l[:-2]+l[-1]+'i'}      # isprazan -> isprazni
    sib={'k':'c','g':'z','h':'s'}
    base=l[:-1] if l[-1] in 'aoe' else l
    if base[-1] in sib: out.add(base[:-1]+sib[base[-1]]+'i')
    return out
def twin(form, inf):
    dd=lookup(form)
    if not dd: return []
    lem,_=head(dd,form)
    return [l for l in lem if l!=inf and not l.endswith(' se') and not re.search(r'(ti|ći)\d*$',l)
            and form in forms_of(re.sub(r'\d+$|\s*\(.*\)$','',l))]

def entry(inf):
    """Returns (table entries, note) for one infinitive."""
    d = lookup(inf)
    if not d:
        return [], 'not in the dictionary'
    lemmas, acc = head(d, inf)
    plain, si, slong, longs = analyse(acc)
    if plain != inf or si is None:
        return [], 'unexpected headword ' + acc
    prez = field(d, 'prez'); trp = field(d, 'prid. trp'); imp = field(d, 'imp')
    note = '%s, prez. %s, trp. %s' % (acc, prez, trp)
    nuc = nuclei(inf)
    if si not in nuc:
        return [], note + ' (accent not on a syllable?)'
    k = nuc.index(si)
    pp = None; pshift = False; pend = None; psi = None; plong = False
    if prez:
        if prez.startswith('-'):
            pend = analyse(prez[1:])[0]
        else:
            pp, psi, plong, _ = analyse(prez); pend = pp[-2:]
            pshift = psi is not None and psi < si
    if inf.endswith('nuti') and pend and pend.endswith('em'): cls, stem = 'u', inf[:-4]
    elif inf.endswith('iti') and pend and pend.endswith('im'): cls, stem = 'i', inf[:-3]
    elif inf.endswith('ati') and pend and pend.endswith('am'): cls, stem = 'a', inf[:-3]
    elif inf.endswith('ati') and pend and pend.endswith('em') and pp and not pp.endswith('ujem'): cls, stem = 't', inf[:-3]
    else:
        return [], note + ' (irregular: lexicon)'
    if si >= len(stem):
        return [], note + ' (accent on the ending)'
    pas = ''
    if trp:
        tp, tsi, _, _ = analyse(trp)
        if tsi is not None and tsi < si:
            pas = 'p' if (k > 0 and tsi == nuc[k - 1]) else 'P'
    initial_short = k == 0 and not slong
    if cls in 'iau':
        if initial_short:
            return [], note + ' (first syllable, short: no entry needed)'
        flags = cls + ('<' if pshift else '') + pas
        tw = []
        if cls == 'i': tw = twin(stem + 'i', inf)
        if cls == 'a': tw = [l for l in twin(stem + 'a', inf) if l == stem + 'a']
        if tw and k > 0:
            flags += 'n'; note += '; twin: ' + ', '.join(tw)
        return [mark(stem, si, slong) + '=' + flags], note
    entries = []
    if not initial_short:
        entries.append(mark(stem, si, slong) + '=t' + pas)
    pstem = pp[:-2]
    if imp:
        ip, isi, ilong, _ = analyse(imp)
        if isi is not None and isi < len(pstem) and ip.startswith(pstem):
            if not (isi == nuclei(ip)[0] and not ilong):
                entries.append(mark(pstem, isi, ilong) + '=e' + ('<' if psi is not None and psi < isi else ''))
    elif psi is not None and psi < len(pstem):
        if pshift and pstem[:si + 1] == stem[:si + 1]:
            entries.append(mark(pstem, si, slong) + '=e<')
        elif not (psi == nuclei(pp)[0] and not plong):
            entries.append(mark(pstem, psi, plong) + '=e')
    return entries, note + ', imp. ' + str(imp)

if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    for inf in sys.argv[1:]:
        entries, note = entry(inf)
        print('%-14s %-28s %s' % (inf, '  '.join(entries) or '-', note))
