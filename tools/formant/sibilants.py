#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
sibilants.py - Summary of the friction in vowel-consonant-vowel recordings.

For each WAV file (16-bit mono, e.g. "asa", "aša", "ača") prints the length of
the noisy stretch, its level relative to the loudest vowel frame, the same
level weighted by the ear's sensitivity and heard through a small speaker
("ear", see below), its spectral centroid, the frequencies under which a
tenth, a half and nine tenths of its energy lie, and the energy per band in dB
relative to the whole noise. Used to compare the sibilants of the voices with
recorded speech and with other synthesizers (see "Sibilants" and "The hiss
where the ear is sharpest" in docs/formant.md).

The "ear" column: every frame's spectrum is weighted by the inverse of the
threshold in quiet (Terhardt 1979: 3.64 f^-0.8 - 6.5 exp(-0.6 (f - 3.3)^2) +
0.001 f^4 dB, f in kHz, the ear's dip of 6.5 dB around 3.3 kHz) and by a
fourth-order 500 Hz high-pass that stands in for a phone's speaker; the
loudest friction frame is then given re the loudest vowel frame. Friction of
the same level is some 6 dB louder to the ear at 4 kHz than at 6.3 kHz.

Usage:
    sibilants.py asa.wav asha.wav ...

Requires numpy.
"""
import sys, wave, numpy as np

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

EDGES=[0,1000,2000,3000,4000,5000,6500,8000,11025]

def read(p):
    w=wave.open(p); d=np.frombuffer(w.readframes(w.getnframes()),dtype=np.int16).astype(float)/32768
    return d,w.getframerate()

def ear_weight(f):
    """The ear's sensitivity (power) through a small speaker's 500 Hz high-pass."""
    k=np.maximum(f,20.0)/1000.0
    threshold=3.64*k**-0.8-6.5*np.exp(-0.6*(k-3.3)**2)+1e-3*k**4
    speaker=1.0/(1.0+(500.0/np.maximum(f,1.0))**8)
    return 10**(-threshold/10)*speaker

def summ(p):
    x,r=read(p); win=int(r*0.012); step=int(r*0.004)
    f=np.fft.rfftfreq(win,1/r); ear=ear_weight(f)
    fr=[]
    for s in range(0,len(x)-win,step):
        sp=np.abs(np.fft.rfft(x[s:s+win]*np.hanning(win)))**2/win
        tot=sp.sum()+1e-12
        fr.append((10*np.log10(tot),(sp*f).sum()/tot,sp,10*np.log10((sp*ear).sum()+1e-12)))
    if not fr: return
    loud=max(a for a,_,_,_ in fr)
    fric=[(a,c,sp,e) for a,c,sp,e in fr if a>loud-32 and c>2200]
    vowel=[e for a,c,sp,e in fr if a>loud-12 and c<1800]
    if not fric:
        print(f"{p:34s} no noisy frames"); return
    avg=np.mean([sp for _,_,sp,_ in fric],axis=0); tot=avg.sum()
    lvl=10*np.log10(np.max([10**(a/10) for a,_,_,_ in fric]))-loud
    heard=max(e for _,_,_,e in fric)-max(vowel) if vowel else float("nan")
    cent=(avg*f).sum()/tot
    cum=np.cumsum(avg)/tot
    q10,q50,q90=(f[min(np.searchsorted(cum,a),len(f)-1)] for a in (0.1,0.5,0.9))
    bands=" ".join(f"{10*np.log10(avg[(f>=lo)&(f<hi)].sum()/tot+1e-9):4.0f}" for lo,hi in zip(EDGES[:-1],EDGES[1:]))
    print(f"{p:34s} n={len(fric)*4:4d}ms lvl={lvl:5.1f} ear={heard:5.1f} cent={cent:5.0f} 10/50/90%={q10:5.0f} {q50:5.0f} {q90:5.0f} | {bands}")

if __name__=="__main__":
    print(" "*34+"                                                                        |   1k   2k   3k   4k   5k  6.5k   8k  11k")
    for p in sys.argv[1:]: summ(p)
