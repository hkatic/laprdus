#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
analyze.py - Acoustic measurements for tuning the formant voices.

Prints, for every 10 ms of a recording: level relative to the loudest frame,
spectral centroid, energy per frequency band (dB relative to the frame total)
and LPC formant estimates. The values in src/formant/formant_phonemes.cpp were
set by comparing such tables for the formant voices against recordings of
Croatian speech (see docs/formant.md).

Usage:
    analyze.py file.wav [file2.wav ...]
    analyze.py --voice zvonko "asa" "aša" "aka"     # synthesize with the CLI
    analyze.py --step 5 file.wav                     # finer time resolution

Requires numpy. WAV input must be 16-bit mono.
"""

import argparse
import os
import subprocess
import sys
import tempfile
import wave

import numpy as np

BAND_EDGES = [0, 500, 1000, 1500, 2000, 2500, 3000, 4000, 5000, 6500, 8000, 11025]


def read_wav(path):
    with wave.open(path) as wav:
        if wav.getsampwidth() != 2 or wav.getnchannels() != 1:
            sys.exit(f"{path}: expected 16-bit mono WAV")
        data = np.frombuffer(wav.readframes(wav.getnframes()), dtype=np.int16)
        return data.astype(np.float64) / 32768.0, wav.getframerate()


def lpc_formants(frame, rate, order=24):
    """Formant estimates below 5 kHz from the roots of an LPC polynomial."""
    frame = frame * np.hamming(len(frame))
    frame = np.append(frame[0], frame[1:] - 0.97 * frame[:-1])
    r = np.correlate(frame, frame, "full")[len(frame) - 1:len(frame) + order]
    if r[0] < 1e-9:
        return []
    a = np.zeros(order + 1)
    a[0] = 1.0
    error = r[0]
    for i in range(1, order + 1):
        k = -(r[i] + np.dot(a[1:i], r[i - 1:0:-1])) / error
        a[1:i] = a[1:i] + k * a[i - 1:0:-1]
        a[i] = k
        error *= 1.0 - k * k
        if error <= 0:
            break
    roots = np.roots(a)
    roots = roots[np.imag(roots) > 0.01]
    freqs = np.angle(roots) * rate / (2 * np.pi)
    widths = -rate / np.pi * np.log(np.abs(roots))
    return sorted(int(f) for f, b in zip(freqs, widths) if 150 < f < 5000 and b < 450)[:4]


def analyze(samples, rate, step_ms):
    window = int(rate * 0.012)
    step = int(rate * step_ms / 1000)
    rows = []
    for start in range(0, len(samples) - window, step):
        frame = samples[start:start + window]
        spectrum = np.abs(np.fft.rfft(frame * np.hanning(window))) ** 2 / window
        freqs = np.fft.rfftfreq(window, 1.0 / rate)
        total = spectrum.sum() + 1e-12
        bands = [10 * np.log10(spectrum[(freqs >= lo) & (freqs < hi)].sum() / total + 1e-9)
                 for lo, hi in zip(BAND_EDGES[:-1], BAND_EDGES[1:])]
        formants = lpc_formants(samples[start:start + int(rate * 0.02)], rate)
        rows.append((start * 1000.0 / rate, 10 * np.log10(total),
                     (spectrum * freqs).sum() / total, bands, formants))
    return rows


def report(title, samples, rate, step_ms):
    rows = analyze(samples, rate, step_ms)
    if not rows:
        return
    loudest = max(row[1] for row in rows)
    active = [i for i, row in enumerate(rows) if row[1] > loudest - 45]
    print(f"== {title}  ({len(samples) * 1000 // rate} ms)")
    print("    ms  lvl  cent | " + " ".join(f"{edge / 1000:4.1f}" for edge in BAND_EDGES[1:])
          + "   formants")
    for ms, level, centroid, bands, formants in rows[active[0]:active[-1] + 1]:
        print(f"{ms:6.0f} {level - loudest:4.0f} {centroid:5.0f} | "
              + " ".join(f"{band:4.0f}" for band in bands) + f"   {formants}")


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("items", nargs="+", help="WAV files, or texts with --voice")
    parser.add_argument("--voice", help="synthesize the texts with this Laprdus voice")
    parser.add_argument("--cli", default=os.environ.get("LAPRDUS_CLI", "laprdus"),
                        help="path to the laprdus CLI (default: $LAPRDUS_CLI or laprdus)")
    parser.add_argument("--data-dir", default=os.environ.get("LAPRDUS_DATA"),
                        help="voice data directory for the CLI (default: $LAPRDUS_DATA)")
    parser.add_argument("--step", type=float, default=10.0, help="time step in ms (default 10)")
    args = parser.parse_args()

    for item in args.items:
        if args.voice:
            with tempfile.TemporaryDirectory() as tmp:
                path = os.path.join(tmp, "out.wav")
                command = [args.cli, "-v", args.voice, "-o", path]
                if args.data_dir:
                    command += ["-D", args.data_dir]
                subprocess.run(command + [item], check=True, stdout=subprocess.DEVNULL)
                samples, rate = read_wav(path)
            report(f'{args.voice}: "{item}"', samples, rate, args.step)
        else:
            samples, rate = read_wav(item)
            report(item, samples, rate, args.step)


if __name__ == "__main__":
    main()
