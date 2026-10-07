#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
sibilants.py - Summary of the friction in vowel-consonant-vowel recordings.

For each WAV file (16-bit mono, e.g. "asa", "aša", "ača") prints the length of
the noisy stretch, its level relative to the loudest vowel frame, its spectral
centroid and peak, and the energy per band in dB relative to the whole noise.
Used to compare the sibilants of the formant voices with recorded speech and
with other synthesizers (see docs/formant.md).

Usage:
    sibilants.py asa.wav asha.wav ...

Requires numpy.
"""
import sys, wave, numpy as np
EDGES=[0,1000,2000,3000,4000,5000,6500,8000,11025]
def read(p):
    w=wave.open(p); d=np.frombuffer(w.readframes(w.getnframes()),dtype=np.int16).astype(float)/32768
    return d,w.getframerate()
def summ(p):
    x,r=read(p); win=int(r*0.012); step=int(r*0.004)
    fr=[]
    for s in range(0,len(x)-win,step):
        sp=np.abs(np.fft.rfft(x[s:s+win]*np.hanning(win)))**2/win
        f=np.fft.rfftfreq(win,1/r); tot=sp.sum()+1e-12
        fr.append((10*np.log10(tot),(sp*f).sum()/tot,sp))
    if not fr: return
    loud=max(a for a,_,_ in fr)
    fric=[(a,c,sp) for a,c,sp in fr if a>loud-32 and c>2200]
    if not fric:
        print(f"{p:34s} no noisy frames"); return
    f=np.fft.rfftfreq(win,1/r)
    avg=np.mean([sp for _,_,sp in fric],axis=0); tot=avg.sum()
    lvl=10*np.log10(np.max([10**(a/10) for a,_,_ in fric]))-loud
    cent=(avg*f).sum()/tot; peak=f[np.argmax(np.convolve(avg,np.ones(5)/5,'same'))]
    bands=" ".join(f"{10*np.log10(avg[(f>=lo)&(f<hi)].sum()/tot+1e-9):4.0f}" for lo,hi in zip(EDGES[:-1],EDGES[1:]))
    print(f"{p:34s} n={len(fric)*4:4d}ms lvl={lvl:5.1f} cent={cent:5.0f} peak={peak:5.0f} | {bands}")
print(" "*34+"                                      |   1k   2k   3k   4k   5k  6.5k   8k  11k")
for p in sys.argv[1:]: summ(p)
