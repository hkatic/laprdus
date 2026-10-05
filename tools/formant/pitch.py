#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
pitch.py - Pitch contour of a recording as a row of numbers.

For each WAV file (16-bit mono) prints the median pitch, the pitch span and
one value per 30 ms: semitones above or below the median, "." where there is
no voicing. Isolated large values next to a "." are tracking errors at
voiceless consonants. Used to compare the intonation of the formant voices
with other synthesizers (see docs/formant.md).

Usage:
    pitch.py statement.wav question.wav ...

Requires numpy.
"""
import sys, wave, numpy as np
def read(p):
    w=wave.open(p); d=np.frombuffer(w.readframes(w.getnframes()),dtype=np.int16).astype(float)/32768
    return d,w.getframerate()
def track(x,r,step=0.010):
    win=int(r*0.04); out=[]
    lo,hi=int(r/400),int(r/55)
    pk=np.max(np.abs(x))+1e-9
    for s in range(0,len(x)-win,int(r*step)):
        fr=x[s:s+win]; fr=fr-fr.mean()
        if np.sqrt((fr**2).mean())<0.03*pk: out.append(0); continue
        ac=np.correlate(fr,fr,'full')[win-1:]
        ac=ac/(ac[0]+1e-12)/(1-np.arange(win)/win)
        k=lo+np.argmax(ac[lo:hi])
        # prefer shortest lag within 90% of max (avoid octave-down)
        cand=[j for j in range(lo,hi) if ac[j]>0.9*ac[k] and ac[j]>=ac[j-1] and ac[j]>=ac[j+1]]
        if cand: k=cand[0]
        out.append(r/k if ac[k]>0.45 else 0)
    return np.array(out)
for p in sys.argv[1:]:
    x,r=read(p); f=track(x,r); v=f[f>0]
    if len(v)<5: print(p,"unvoiced"); continue
    med=np.median(v); st=12*np.log2(np.where(f>0,f,med)/med)
    cells=[]
    for i in range(0,len(f),3):
        seg=f[i:i+3]; vv=seg[seg>0]
        cells.append("  ." if len(vv)==0 else f"{12*np.log2(np.median(vv)/med):+3.0f}")
    # strip leading/trailing unvoiced
    while cells and cells[0]=="  .": cells.pop(0)
    while cells and cells[-1]=="  .": cells.pop()
    print(f"{p:26s} med={med:4.0f}Hz span={12*np.log2(np.percentile(v,97)/np.percentile(v,3)):4.1f}st dur={len(cells)*30:4d}ms\n   "+"".join(cells))
