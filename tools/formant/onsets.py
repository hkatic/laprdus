#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
onsets.py - How abruptly sound sets in at the release of a stop.

For each WAV file (16-bit mono, e.g. "pa", "pra", "aba", "ada") finds the
releases: the places where the energy above 1 kHz comes up from under -30 dB
by 6 dB or more within 6 ms (a voice bar has next to nothing up there, so
voiced stops are found too). For each one prints the level every 2 ms, from
12 ms before to 48 ms after, in dB relative to the loudest 3 ms of the file:
the whole signal, and the part above 1 kHz. Then the spectrum of the first
10 ms after the release in 500 Hz bands, in dB relative to their sum.
A sharp stop steps up within a few milliseconds; a soft one swells.
Used to compare the stops of the formant voices with recorded speech and
with other synthesizers (see docs/formant.md).

Usage:
    onsets.py pa.wav aba.wav ...

Requires numpy.
"""
import sys, wave, numpy as np
EDGES=list(range(0,5500,500))
def read(p):
    w=wave.open(p); d=np.frombuffer(w.readframes(w.getnframes()),dtype=np.int16).astype(float)/32768
    return d,w.getframerate()
def highpass(x,r,fc=1000.0):
    X=np.fft.rfft(x); X[np.fft.rfftfreq(len(x),1/r)<fc]=0; return np.fft.irfft(X,len(x))
def envelope(x,r):
    n=int(r*0.003); s=int(r*0.002)
    return np.array([10*np.log10((x[a:a+n]**2).mean()+1e-12) for a in range(0,len(x)-n,s)])
def summ(p):
    x,r=read(p); e=envelope(x,r); h=envelope(highpass(x,r),r)
    if not len(e): return
    loud=e.max(); e-=loud; h-=loud
    print(p)
    i=3
    while i<len(h)-5:
        if h[i-3:i].max()<-30 and h[i:i+3].max()-h[i-3:i].max()>=6:
            a,b=max(0,i-6),min(len(h),i+24)
            print(f"  release at {i*2:4d} ms   all "+" ".join(f"{max(v,-99):3.0f}" for v in e[a:b]))
            print(f"                     >1 kHz "+" ".join(f"{max(v,-99):3.0f}" for v in h[a:b]))
            s=int(i*0.002*r); n=int(r*0.010); seg=x[s:s+n]
            if len(seg)==n:
                sp=np.abs(np.fft.rfft(seg*np.hanning(n),1024))**2; f=np.fft.rfftfreq(1024,1/r)
                tot=sp[f<EDGES[-1]].sum()+1e-12
                print("        first 10 ms, bands "+" ".join(f"{10*np.log10(sp[(f>=lo)&(f<lo+500)].sum()/tot+1e-9):3.0f}" for lo in EDGES[:-1])
                      +"   (0.5 kHz steps up to 5 kHz)")
            i+=40
        else: i+=1
for p in sys.argv[1:]: summ(p)
