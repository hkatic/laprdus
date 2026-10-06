#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
stops.py - Closures and releases of stops in short recordings.

For each WAV file (16-bit mono, e.g. "ata", "asta", "pat", "pak") prints a
level strip, one character per 4 ms: a digit n means the frame is 4n to 4n+4
dB below the loudest frame, "." more than 44 dB below. Under it, every silent
gap inside the item with its length, and for the 16 ms that follow it (the
burst) the level and the energy per 500 Hz band in dB relative to the burst.
The strip shows how long a release lasts; the bands show where the burst sits.
Used to compare the stops of the formant voices with recorded speech and with
other synthesizers (see docs/formant.md).

Usage:
    stops.py asta.wav pat.wav ...

Requires numpy.
"""
import sys, wave, numpy as np
EDGES=list(range(0,7000,500))
QUIET=30    # dB below the loudest frame
def read(p):
    w=wave.open(p); d=np.frombuffer(w.readframes(w.getnframes()),dtype=np.int16).astype(float)/32768
    return d,w.getframerate()
def summ(p):
    x,r=read(p); win=int(r*0.006); step=int(r*0.004)
    lv=np.array([10*np.log10((x[s:s+win]**2).mean()+1e-12) for s in range(0,len(x)-win,step)])
    if not len(lv): return
    loud=lv.max(); act=np.nonzero(lv>loud-45)[0]; a,b=act[0],act[-1]+1
    strip="".join("." if loud-l>=44 else "0123456789A"[int((loud-l)//4)] for l in lv[a:b])
    print(f"{p}  ({(b-a)*4} ms)\n  {strip}")
    quiet=lv<loud-QUIET; i=a
    while i<b:
        if not quiet[i]: i+=1; continue
        j=i
        while j<b and quiet[j]: j+=1
        if i>a and j<b and j-i>=3:
            s=j*step; n=int(r*0.016); seg=x[s:s+n]
            if len(seg)==n:
                sp=np.abs(np.fft.rfft(seg*np.hanning(n),1024))**2; f=np.fft.rfftfreq(1024,1/r)
                tot=sp[f<EDGES[-1]].sum()+1e-12
                bands=" ".join(f"{10*np.log10(sp[(f>=lo)&(f<lo+500)].sum()/tot+1e-9):4.0f}" for lo in EDGES[:-1])
                lvl=10*np.log10((seg**2).mean()+1e-12)-loud
                print(f"  gap {(j-i)*4:3d} ms at {i*4:4d} ms, burst {lvl:5.1f} dB | {bands}")
        i=j
print(" "*37+"| "+" ".join(f"{(e+500)/1000:4.1f}" for e in EDGES[:-1])+" kHz")
for p in sys.argv[1:]: summ(p)
