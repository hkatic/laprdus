#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
spectrum.py - Long-term spectrum, levels, clicks and boost distortion.

The measurements behind the sound-quality round of November 2026 (hiss at high
phone volume, crackle, distortion with the volume boost; see "Hiss, crackle and
the boost" in docs/formant.md). Every report works on 16-bit mono WAV files;
with --voice the items are synthesized first with the laprdus CLI, as
analyze.py does.

Reports (one per invocation):

  spectrum.py FILE...                       level statistics and the long-term
                                            1/3-octave spectrum of the speech
                                            frames, flat and through a 500 Hz
                                            high-pass (a phone's speaker), and
                                            how much of the speech sits in the
                                            band the ear is sharpest in
  spectrum.py --clicks FILE...              short bursts above 7 kHz (clicks)
                                            and runs of digital silence with
                                            the samples next to them
  spectrum.py --tail FILE...                the 8 ms after the friction of a
                                            VCV item or a word ends: level in
                                            the band below 600 Hz (the thump
                                            of a wrong filter bank) and above
                                            7 kHz
  spectrum.py --floor FILE...               a sustained vowel: harmonics
                                            against the floor between them
                                            (noise and aliasing under the
                                            vowel) per band
  spectrum.py --boost PLAIN BOOSTED         gain, waveform residual and
                                            high-frequency products of the
                                            volume boost
  spectrum.py --alias                       the aliasing of the glottal pulse
                                            as klatt_synth.cpp computes it
                                            (no audio needed)

Synthesis: --voice zvonko [--language hr] [--rate 1.0] [--volume-boost]
[--cli PATH] [--data-dir DIR] [--out-dir DIR] turns every item into a WAV first
(kept in --out-dir when given). Requires numpy; scipy speeds the filters up.

Usage examples:
    spectrum.py --voice zvonko --voice josip -i paragraph.txt
    spectrum.py zvonko.wav josip.wav reference.wav
    spectrum.py --clicks --voice zvonko sapa sata saka spavati "sve" zvono
    spectrum.py --tail --voice zvonko sapa sata saka
    spectrum.py --floor --voice zvonko aaa
    spectrum.py --boost plain.wav boosted.wav
"""

import argparse
import math
import os
import subprocess
import sys
import tempfile
import wave

import numpy as np

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

try:
    import scipy.signal as sg
except ImportError:  # the filters then run in Python, slower but the same
    sg = None

THIRD_OCTAVES = [100, 125, 160, 200, 250, 315, 400, 500, 630, 800, 1000, 1250,
                 1600, 2000, 2500, 3150, 4000, 5000, 6300, 8000, 10000]
ACTIVE_DB = 45.0          # a frame counts as speech above this under the loudest
BUTTERWORTH_Q = (0.54119610, 1.30656296)   # the fourth-order pair of klatt_synth.cpp


# -----------------------------------------------------------------------------
# WAV and synthesis
# -----------------------------------------------------------------------------

def read_wav(path):
    with wave.open(path) as wav:
        width, channels, rate = wav.getsampwidth(), wav.getnchannels(), wav.getframerate()
        raw = wav.readframes(wav.getnframes())
    if width != 2:
        sys.exit(f"{path}: expected 16-bit WAV")
    data = np.frombuffer(raw, dtype=np.int16).astype(np.float64) / 32768.0
    if channels > 1:
        data = data.reshape(-1, channels).mean(axis=1)
    return data, rate


def synthesize(args, voice, item, label):
    """Run the CLI on one item; return the WAV path (kept in --out-dir)."""
    directory = args.out_dir or tempfile.mkdtemp(prefix="spectrum-")
    os.makedirs(directory, exist_ok=True)
    # ASCII file names (the Windows CLI gets its arguments in the ANSI code
    # page, so the text goes through a UTF-8 file, never the command line).
    folded = label
    for a, b in (("č", "ch"), ("ć", "cj"), ("đ", "dj"), ("š", "sh"), ("ž", "zh"),
                 ("Č", "Ch"), ("Ć", "Cj"), ("Đ", "Dj"), ("Š", "Sh"), ("Ž", "Zh")):
        folded = folded.replace(a, b)
    safe = "".join(c if c.isascii() and c.isalnum() else "_" for c in folded)[:40]
    path = os.path.join(directory, f"{voice}_{safe}.wav")
    cli = os.path.abspath(args.cli) if os.sep in args.cli or "/" in args.cli else args.cli
    command = [cli, "-v", voice, "-o", path, "-r", str(args.rate),
               "-I", str(int(round(args.inflection)))]
    if args.language:
        command += ["-L", args.language]
    if args.data_dir:
        command += ["-D", args.data_dir]
    if args.volume_boost:
        command += ["-B"]
    if os.path.isfile(item):
        command += ["-i", item]
    else:
        text_path = os.path.join(directory, f"{safe}.txt")
        with open(text_path, "w", encoding="utf-8") as f:
            f.write(item)
        command += ["-i", text_path]
    subprocess.run(command, check=True, stdout=subprocess.DEVNULL)
    return path


# -----------------------------------------------------------------------------
# Filters
# -----------------------------------------------------------------------------

def biquad(x, b, a):
    if sg is not None:
        return sg.lfilter(b, a, x)
    y = np.zeros_like(x)
    x1 = x2 = y1 = y2 = 0.0
    for i, v in enumerate(x):
        out = b[0] * v + b[1] * x1 + b[2] * x2 - a[1] * y1 - a[2] * y2
        x2, x1, y2, y1 = x1, v, y1, out
        y[i] = out
    return y


def rbj(kind, fc, q, rate):
    w = 2 * math.pi * fc / rate
    alpha = math.sin(w) / (2 * q)
    cw = math.cos(w)
    if kind == "high":
        b = [(1 + cw) / 2, -(1 + cw), (1 + cw) / 2]
    else:
        b = [(1 - cw) / 2, 1 - cw, (1 - cw) / 2]
    a = [1 + alpha, -2 * cw, 1 - alpha]
    return [v / a[0] for v in b], [v / a[0] for v in a]


def butter4(x, fc, rate, kind="high"):
    """Fourth-order Butterworth as two biquads, as the synthesizer does it."""
    for q in BUTTERWORTH_Q:
        b, a = rbj(kind, fc, q, rate)
        x = biquad(x, b, a)
    return x


def phone_speaker(x, rate):
    """What a phone's small speaker keeps: nothing under 500 Hz (phones.py)."""
    return butter4(x, 500.0, rate, "high")


# -----------------------------------------------------------------------------
# Frames and levels
# -----------------------------------------------------------------------------

def frame_db(x, rate, win_ms=12.0, hop_ms=4.0):
    win = int(rate * win_ms / 1000)
    hop = int(rate * hop_ms / 1000)
    starts = np.arange(0, max(1, len(x) - win), hop)
    levels = np.array([10 * np.log10(np.mean(x[s:s + win] ** 2) + 1e-12) for s in starts])
    return starts, levels


def active_frames(x, rate, win_ms=12.0, hop_ms=4.0):
    starts, levels = frame_db(x, rate, win_ms, hop_ms)
    loudest = levels.max() if len(levels) else -100.0
    return starts, levels, levels > loudest - ACTIVE_DB


def true_peak(x):
    """Peak of the 4x oversampled signal (inter-sample peaks, as a phone's
    resampler and limiter see them)."""
    n = len(x)
    spectrum = np.fft.rfft(x)
    padded = np.zeros(4 * n // 2 + 1, dtype=complex)
    padded[:len(spectrum)] = spectrum
    up = np.fft.irfft(padded, 4 * n) * 4
    return np.max(np.abs(up))


def level_stats(x, rate):
    starts, levels, active = active_frames(x, rate)
    speech = levels[active]
    peak = np.max(np.abs(x)) if len(x) else 0.0
    median = np.median(speech) if len(speech) else -100.0
    loud = np.percentile(speech, 90) if len(speech) else -100.0
    return {
        "median": median,
        "loud": loud,
        "peak": 20 * np.log10(peak + 1e-12),
        "true_peak": 20 * np.log10(true_peak(x) + 1e-12),
        "crest": 20 * np.log10(peak + 1e-12) - median,
        "over80": 100.0 * np.mean(np.abs(x) > 0.8),
        "over98": 100.0 * np.mean(np.abs(x) > 0.98),
        "speech_s": len(speech) * 0.004,
    }


# -----------------------------------------------------------------------------
# Long-term spectrum
# -----------------------------------------------------------------------------

def average_spectrum(x, rate, n=2048, kind="all"):
    """Mean power spectrum of the speech frames (Hann, half overlap).
    kind: "all", "voiced" (spectral centroid under 2.2 kHz: vowels, nasals,
    liquids) or "friction" (centroid above it: sibilants, bursts, h)."""
    window = np.hanning(n)
    total = np.zeros(n // 2 + 1)
    count = 0
    frames = []
    for s in range(0, len(x) - n, n // 2):
        frame = x[s:s + n]
        frames.append((s, 10 * np.log10(np.mean(frame ** 2) + 1e-12)))
    freqs = np.fft.rfftfreq(n, 1.0 / rate)
    if not frames:
        return freqs, total
    loudest = max(level for _, level in frames)
    for s, level in frames:
        if level < loudest - 40.0:
            continue
        spectrum = np.abs(np.fft.rfft(x[s:s + n] * window)) ** 2
        if kind != "all":
            centroid = (spectrum * freqs).sum() / (spectrum.sum() + 1e-30)
            if (kind == "friction") != (centroid > 2200.0):
                continue
        total += spectrum
        count += 1
    return freqs, total / max(count, 1)


def band_energy(freqs, spectrum, lo, hi):
    mask = (freqs >= lo) & (freqs < hi)
    return float(spectrum[mask].sum())


def third_octave_levels(freqs, spectrum):
    """Energy per 1/3-octave band in dB relative to the whole spectrum."""
    total = spectrum.sum() + 1e-30
    levels = []
    for centre in THIRD_OCTAVES:
        lo, hi = centre / 2 ** (1 / 6), centre * 2 ** (1 / 6)
        levels.append(10 * np.log10(band_energy(freqs, spectrum, lo, hi) / total + 1e-12))
    return levels


def balance(freqs, spectrum):
    """Two numbers that say where the hiss is: 2.5-6 kHz and 6-11 kHz relative
    to 300-1000 Hz (the vowel body), in dB."""
    body = band_energy(freqs, spectrum, 300, 1000) + 1e-30
    return (10 * np.log10(band_energy(freqs, spectrum, 2500, 6000) / body + 1e-12),
            10 * np.log10(band_energy(freqs, spectrum, 6000, 11025) / body + 1e-12))


def report_spectrum(label, x, rate):
    stats = level_stats(x, rate)
    print(f"== {label}  ({len(x) / rate:.1f} s, speech {stats['speech_s']:.1f} s)")
    print(f"   median {stats['median']:6.1f} dBFS  loud (p90) {stats['loud']:6.1f}  "
          f"peak {stats['peak']:5.1f}  true peak {stats['true_peak']:5.1f}  "
          f"crest {stats['crest']:4.1f} dB  >0.8: {stats['over80']:.3f} %  >0.98: {stats['over98']:.3f} %")
    header = "   " + " ".join(f"{c / 1000:4.2g}" if c < 1000 else f"{c / 1000:4.1f}" for c in THIRD_OCTAVES)
    print("   1/3-octave bands, dB relative to the total (kHz):")
    print(header + "   | 2.5-6k  6-11k rel. 0.3-1k")
    for name, signal, kind in (("flat ", x, "all"), ("500Hz", phone_speaker(x, rate), "all"),
                               ("voicd", x, "voiced"), ("frict", x, "friction")):
        freqs, spectrum = average_spectrum(signal, rate, kind=kind)
        levels = third_octave_levels(freqs, spectrum)
        top, hf = balance(freqs, spectrum)
        print(f"   {name} " + " ".join(f"{v:4.0f}" for v in levels) + f"   | {top:6.1f} {hf:6.1f}")
    # How loud the friction frames are against the voiced ones, in dB
    starts, levels, active = active_frames(x, rate)
    noisy, _ = noisy_frames(x, rate)
    noisy_set = set(noisy)
    voiced = [lvl for s, lvl, act in zip(starts, levels, active) if act and s not in noisy_set]
    fric = [lvl for s, lvl, act in zip(starts, levels, active) if act and s in noisy_set]
    if voiced and fric:
        print(f"   friction frames: {len(fric) * 0.004:4.1f} s, median {np.median(fric) - np.median(voiced):5.1f} dB "
              f"re the voiced median, p90 {np.percentile(fric, 90) - np.percentile(voiced, 90):5.1f} dB re the voiced p90")
    p50, p90, p99, share = ear_band(x, rate)
    print(f"   the ear's band (2-6 kHz, ear-weighted, 500 Hz high-pass) re the loud level: "
          f"median {p50:5.1f}  p90 {p90:5.1f}  p99 {p99:5.1f} dB, {share:4.1f} % of the frames above -10 dB")


def ear_band(x, rate, win_ms=12.0, hop_ms=4.0):
    """How much of the speech sits where the ear is sharpest: per 12 ms frame
    the energy in 2-6 kHz, weighted by the inverse of the threshold in quiet
    (Terhardt 1979, the dip of 6.5 dB around 3.3 kHz) and by a phone speaker's
    500 Hz high-pass, re the loud level of the file (the 90th percentile of
    the flat frame levels). Returns the median, the 90th and 99th percentile
    in dB and the share of the speech frames above -10 dB, in percent. The
    same weighting as the "ear" column of sibilants.py."""
    win = int(rate * win_ms / 1000)
    hop = int(rate * hop_ms / 1000)
    freqs = np.fft.rfftfreq(win, 1 / rate)
    khz = np.maximum(freqs, 20.0) / 1000.0
    threshold = 3.64 * khz ** -0.8 - 6.5 * np.exp(-0.6 * (khz - 3.3) ** 2) + 1e-3 * khz ** 4
    weight = 10 ** (-threshold / 10) / (1.0 + (500.0 / np.maximum(freqs, 1.0)) ** 8)
    weight = weight * ((freqs >= 2000) & (freqs < 6000))
    window = np.hanning(win)
    hot, flat = [], []
    for start in range(0, len(x) - win, hop):
        spectrum = np.abs(np.fft.rfft(x[start:start + win] * window)) ** 2 / win
        hot.append(10 * np.log10((spectrum * weight).sum() + 1e-12))
        flat.append(10 * np.log10(spectrum.sum() + 1e-12))
    if not flat:
        return 0.0, 0.0, 0.0, 0.0
    hot, flat = np.array(hot), np.array(flat)
    active = flat > flat.max() - ACTIVE_DB
    level = hot[active] - np.percentile(flat[active], 90)
    return (np.percentile(level, 50), np.percentile(level, 90), np.percentile(level, 99),
            100.0 * np.mean(level > -10.0))


# -----------------------------------------------------------------------------
# Clicks
# -----------------------------------------------------------------------------

def hf_spikes(x, rate, frame_ms=2.0, rise_db=12.0, floor_db=-55.0):
    """Frames whose energy above 7 kHz stands `rise_db` above the median of
    the surrounding 100 ms. The synthesizer's low-passes leave nothing steady
    up there, so a burst is a click or a filter-bank switch."""
    hp = butter4(x, 7000.0, rate, "high")
    n = int(rate * frame_ms / 1000)
    count = len(hp) // n
    hf = np.array([10 * np.log10(np.mean(hp[i * n:(i + 1) * n] ** 2) + 1e-12) for i in range(count)])
    spikes = []
    half = int(50 / frame_ms)
    for i in range(count):
        lo, hi = max(0, i - half), min(count, i + half + 1)
        local = np.median(hf[lo:hi])
        if hf[i] > floor_db and hf[i] > local + rise_db:
            spikes.append((i * frame_ms, hf[i], hf[i] - local))
    return spikes


def sample_steps(x, rate, frame_ms=2.0, ratio=8.0, floor=0.004):
    """Frames with a sample-to-sample step at least `ratio` times the RMS of
    the 20 ms before them: a discontinuity, as opposed to the broadband but
    smooth onset of a burst."""
    n = int(rate * frame_ms / 1000)
    back = int(rate * 0.020)
    diff = np.abs(np.diff(x, prepend=x[:1]))
    steps = []
    for i in range(len(x) // n):
        lo = i * n
        local = np.sqrt(np.mean(x[max(0, lo - back):lo] ** 2) + 1e-12) if lo > 0 else 1e-6
        biggest = diff[lo:lo + n].max()
        if biggest > floor and biggest > ratio * local:
            steps.append((lo * 1000.0 / rate, biggest, biggest / local))
    return steps


def zero_runs(x, rate, run_ms=1.0, look_ms=0.5):
    """docs/concatenative.md: runs of digital silence of 1 ms or more and the
    largest sample within 0.5 ms on either side (a cut-off window)."""
    samples = np.round(x * 32768).astype(int)
    run = int(rate * run_ms / 1000)
    look = int(rate * look_ms / 1000)
    zero = samples == 0
    edges = []
    i = 0
    n = len(samples)
    while i < n:
        if zero[i]:
            j = i
            while j < n and zero[j]:
                j += 1
            if j - i >= run:
                before = np.max(np.abs(samples[max(0, i - look):i])) if i > 0 else 0
                after = np.max(np.abs(samples[j:min(n, j + look)])) if j < n else 0
                edges.append((i / rate * 1000, (j - i) / rate * 1000, int(max(before, after))))
            i = j
        else:
            i += 1
    return edges


def report_peaks(label, x, rate, count=12):
    """The largest samples and what they are in: friction (a burst or a
    sibilant) or voiced sound. Says what sets the peak level of a voice."""
    noisy, _ = noisy_frames(x, rate)
    hop = int(rate * 0.004)
    noisy_set = set(noisy)
    order = np.argsort(-np.abs(x))
    chosen = []
    for i in order:
        if all(abs(int(i) - j) > int(rate * 0.030) for j, _ in chosen):
            chosen.append((int(i), float(abs(x[i]))))
        if len(chosen) >= count:
            break
    loud = np.abs(x) > 0.5
    loud_idx = np.nonzero(loud)[0]
    in_friction = sum(1 for i in loud_idx if (int(i) // hop) * hop in noisy_set or (int(i) // hop - 1) * hop in noisy_set)
    print(f"== {label}: {len(loud_idx)} samples above 0.5, {100.0 * in_friction / max(1, len(loud_idx)):4.1f} % of them in friction frames")
    for i, a in chosen:
        frame = (i // hop) * hop
        kind = "friction" if frame in noisy_set or frame - hop in noisy_set else "voiced"
        print(f"   {i * 1000.0 / rate:8.0f} ms  {a:5.3f}  {kind}")


def report_clicks(label, x, rate):
    spikes = hf_spikes(x, rate)
    steps = sample_steps(x, rate)
    edges = zero_runs(x, rate)
    loud = [e for e in edges if e[2] > 150]
    print(f"== {label}: {len(steps)} sample steps, {len(spikes)} spikes above 7 kHz; "
          f"{len(edges)} zero runs, {len(loud)} with a sample above 150 next to them"
          + (f" (max {max(e[2] for e in loud)})" if loud else ""))
    for ms, size, factor in steps[:12]:
        print(f"   step  {ms:7.0f} ms  {size * 32768:6.0f} units, {factor:5.1f} x the level before it")
    big = [sp for sp in spikes if sp[2] > 25.0]
    for ms, level, rise in big[:12]:
        print(f"   spike {ms:7.0f} ms  {level:6.1f} dBFS  +{rise:4.1f} dB over its surroundings")
    if len(big) > 12:
        print(f"   ... {len(big) - 12} more spikes over +25 dB")


# -----------------------------------------------------------------------------
# Friction tails
# -----------------------------------------------------------------------------

def noisy_frames(x, rate):
    """Frames of friction: within 32 dB of the loudest and a centroid above
    2.2 kHz (sibilants.py's rule)."""
    win, hop = int(rate * 0.012), int(rate * 0.004)
    freqs = np.fft.rfftfreq(win, 1 / rate)
    result = []
    levels = []
    for s in range(0, len(x) - win, hop):
        sp = np.abs(np.fft.rfft(x[s:s + win] * np.hanning(win))) ** 2
        tot = sp.sum() + 1e-12
        levels.append(10 * np.log10(tot))
        result.append((s, (sp * freqs).sum() / tot))
    if not levels:
        return [], -100.0
    loudest = max(levels)
    noisy = [s for (s, c), lvl in zip(result, levels) if lvl > loudest - 32 and c > 2200]
    return noisy, loudest


def report_tail(label, x, rate):
    noisy, loudest = noisy_frames(x, rate)
    if not noisy:
        print(f"== {label}: no friction found")
        return
    # The end of the first stretch of friction
    end = noisy[0]
    for s in noisy[1:]:
        if s - end > int(rate * 0.02):
            break
        end = s
    end += int(rate * 0.012)
    after = x[end:end + int(rate * 0.008)]
    if len(after) < 8:
        print(f"== {label}: friction runs to the end")
        return
    spectrum = np.abs(np.fft.rfft(after * np.hanning(len(after)))) ** 2
    freqs = np.fft.rfftfreq(len(after), 1 / rate)
    total = spectrum.sum() + 1e-12
    low = 10 * np.log10(band_energy(freqs, spectrum, 0, 600) / total + 1e-12)
    high = 10 * np.log10(band_energy(freqs, spectrum, 7000, 11025) / total + 1e-12)
    level = 10 * np.log10(np.mean(after ** 2) + 1e-12) - loudest
    spikes = [s for s in hf_spikes(x, rate) if end / rate * 1000 - 4 <= s[0] <= end / rate * 1000 + 12]
    print(f"== {label}: friction ends at {end / rate * 1000:5.0f} ms; the next 8 ms: "
          f"{level:5.1f} dB re loudest frame, below 600 Hz {low:5.1f} dB of it, "
          f"above 7 kHz {high:5.1f} dB; {len(spikes)} spike(s) there")


# -----------------------------------------------------------------------------
# Vowel floor
# -----------------------------------------------------------------------------

def estimate_f0(x, rate):
    seg = x[len(x) // 3:len(x) // 3 + int(rate * 0.05)]
    seg = seg - seg.mean()
    corr = np.correlate(seg, seg, "full")[len(seg) - 1:]
    lo, hi = int(rate / 400), int(rate / 60)
    lag = lo + int(np.argmax(corr[lo:hi]))
    return rate / lag


def report_floor(label, x, rate):
    """Harmonic peaks against the floor between them, per band, on the middle
    of a sustained vowel."""
    f0 = estimate_f0(x, rate)
    n = 2048
    window = np.hanning(n)
    centre = len(x) // 2
    spectra = []
    for s in range(centre - 3 * n // 2, centre + n, n // 2):
        if 0 <= s and s + n <= len(x):
            spectra.append(np.abs(np.fft.rfft(x[s:s + n] * window)) ** 2)
    if not spectra:
        print(f"== {label}: too short")
        return
    spectrum = np.mean(spectra, axis=0)
    freqs = np.fft.rfftfreq(n, 1 / rate)
    print(f"== {label}: F0 {f0:5.1f} Hz; harmonics against the floor between them")
    for lo, hi in ((500, 1000), (1000, 2000), (2000, 4000), (4000, 6000), (6000, 8000)):
        peaks, floor = harmonics_and_floor(spectrum, freqs, f0, lo, hi)
        if peaks:
            p = 10 * np.log10(np.mean(peaks) + 1e-30)
            f = 10 * np.log10(floor + 1e-30)
            print(f"   {lo / 1000:3.1f}-{hi / 1000:3.1f} kHz: harmonics {p - 10 * np.log10(spectrum.max()):6.1f} dB re the strongest, "
                  f"floor {f - p:6.1f} dB under them")


def harmonics_and_floor(spectrum, freqs, f0, lo, hi):
    """The peak at every harmonic of the band and the median of all bins
    farther than f0/4 from any harmonic (where aliases and noise land,
    wherever an F0 puts them)."""
    bin_hz = freqs[1]
    peaks = []
    k = max(1, int(math.ceil(lo / f0)))
    while k * f0 < hi:
        c = int(round(k * f0 / bin_hz))
        w = int(round(f0 / 4 / bin_hz))
        peaks.append(spectrum[max(0, c - w):c + w + 1].max())
        k += 1
    band = (freqs >= lo) & (freqs < hi)
    distance = np.abs(freqs - np.round(freqs / f0) * f0)
    between = band & (distance > 0.4 * f0)
    floor = float(np.median(spectrum[between])) if between.any() else 0.0
    return peaks, floor


# -----------------------------------------------------------------------------
# Boost
# -----------------------------------------------------------------------------

def allpass_chain(x, rate, corners):
    for fc in corners:
        t = math.tan(math.pi * fc / rate)
        a = (1 - t) / (1 + t)
        x = biquad(x, [a, 1.0, 0.0], [1.0, a, 0.0])   # H = (a + z^-1) / (1 + a z^-1)
    return x


def report_boost(plain, boosted, rate, rotate=()):
    n = min(len(plain), len(boosted))
    p, b = plain[:n], boosted[:n]
    if rotate:
        p = allpass_chain(p, rate, rotate)
    starts, levels, active = active_frames(p, rate)
    gains = []
    for s, lvl, act in zip(starts, levels, active):
        if not act:
            continue
        win = int(rate * 0.012)
        gains.append(10 * np.log10((np.mean(b[s:s + win] ** 2) + 1e-12) / (np.mean(p[s:s + win] ** 2) + 1e-12)))
    gains = np.array(gains)
    # Waveform residual per 10 ms frame against the best linear gain
    win = int(rate * 0.010)
    frames = []
    for s in range(0, n - win, win):
        pf, bf = p[s:s + win], b[s:s + win]
        energy = np.sum(pf ** 2)
        if energy <= 0:
            continue
        g = np.dot(bf, pf) / energy
        fit = g * pf
        res = np.sum((bf - fit) ** 2) / (np.sum(fit ** 2) + 1e-20)
        frames.append((energy, 10 * np.log10(res + 1e-12), g))
    frames.sort(key=lambda f: -f[0])
    loud = frames[:max(1, len(frames) // 5)]
    residual = np.median([r for _, r, _ in loud])
    worst = max(r for _, r, _ in loud)
    fp, sp_p = average_spectrum(p, rate)
    fb, sp_b = average_spectrum(b, rate)

    def hf_ratio(freqs, spectrum):
        return 10 * np.log10(band_energy(freqs, spectrum, 7000, 11025) / (band_energy(freqs, spectrum, 300, 3000) + 1e-30) + 1e-12)
    sp_stats, sb_stats = level_stats(p, rate), level_stats(b, rate)
    print(f"== boost: median gain {np.median(gains):5.1f} dB, loud frames (p90 of the plain) "
          f"{np.percentile(gains, 10):5.1f} dB, quiet (p10) {np.percentile(gains, 90):5.1f} dB")
    print(f"   plain  median {sp_stats['median']:6.1f} dBFS  loud {sp_stats['loud']:6.1f}  peak {sp_stats['peak']:5.1f}  crest {sp_stats['crest']:4.1f}")
    print(f"   boosted median {sb_stats['median']:6.1f} dBFS  loud {sb_stats['loud']:6.1f}  peak {sb_stats['peak']:5.1f}  "
          f"true peak {sb_stats['true_peak']:5.1f}  crest {sb_stats['crest']:4.1f}  >0.98: {sb_stats['over98']:.3f} %")
    print(f"   waveform residual on the loudest fifth of the 10 ms frames: median {residual:6.1f} dB, worst {worst:6.1f} dB")
    print(f"   7-11 kHz relative to 0.3-3 kHz: plain {hf_ratio(fp, sp_p):6.1f} dB, boosted {hf_ratio(fb, sp_b):6.1f} dB")


# -----------------------------------------------------------------------------
# Aliasing of the glottal pulse
# -----------------------------------------------------------------------------

def glottal_train(f0, rate, seconds, oq=0.6, brightness=0.72, blep_samples=1.0):
    """klatt_synth.cpp's glottal_pulse() with its polyBLEP closing edge (the
    edge is smoothed over `blep_samples` on either side), followed by the
    first-order brightness emphasis. Pure Python, sample by sample, so that
    it matches the C++ exactly."""
    dt = f0 / rate
    n = int(seconds * rate)
    out = np.zeros(n)
    ph = 0.0
    x1 = 0.0
    edge = dt * blep_samples
    for i in range(n):
        g = 0.0
        if ph < oq:
            x = ph / oq
            g = 2.0 * x - 3.0 * x * x
        d = ph - oq
        if 0.0 <= d < edge:
            u = 1.0 - d / edge
            g -= 0.5 * u * u
        elif -edge < d < 0.0:
            u = 1.0 + d / edge
            g += 0.5 * u * u
        out[i] = (g - brightness * x1) / (1.0 - brightness)
        x1 = g
        ph += dt
        if ph >= 1.0:
            ph -= 1.0
    return out


def inharmonic_floor(x, rate, f0, lo, hi):
    n = 8192
    window = np.hanning(n)
    spectrum = np.zeros(n // 2 + 1)
    count = 0
    for s in range(0, len(x) - n, n // 2):
        spectrum += np.abs(np.fft.rfft(x[s:s + n] * window)) ** 2
        count += 1
    spectrum /= max(count, 1)
    freqs = np.fft.rfftfreq(n, 1 / rate)
    peaks, floor = harmonics_and_floor(spectrum, freqs, f0, lo, hi)
    return 10 * np.log10(floor / np.mean(peaks) + 1e-30)


def report_alias(rate=22050, brightness=0.72):
    print(f"== glottal pulse at {rate} Hz, brightness {brightness}: floor between the harmonics, dB under them")
    print("   F0     edge   1-2 kHz  2-4 kHz  4-6 kHz   (8x oversampled, decimated: the true floor)")
    for f0 in (100.0, 125.0):
        for blep in (1.0, 2.0):
            direct = glottal_train(f0, rate, 2.0, brightness=brightness, blep_samples=blep)
            over = glottal_train(f0, rate * 8, 2.0, brightness=brightness, blep_samples=blep)
            # Decimate with a brick wall at 10 kHz, so only what the direct
            # computation aliases differs.
            spectrum = np.fft.rfft(over)
            freqs = np.fft.rfftfreq(len(over), 1 / (rate * 8))
            spectrum[freqs > rate * 0.46] = 0
            over = np.fft.irfft(spectrum, len(over))[::8] / 8
            cols = []
            for lo, hi in ((1000, 2000), (2000, 4000), (4000, 6000)):
                cols.append(f"{inharmonic_floor(direct, rate, f0, lo, hi):6.1f} ({inharmonic_floor(over, rate, f0, lo, hi):6.1f})")
            print(f"   {f0:5.0f}  {blep:4.1f}   " + "  ".join(cols))


# -----------------------------------------------------------------------------
# Main
# -----------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("items", nargs="*", help="WAV files, or texts with --voice")
    parser.add_argument("--voice", action="append", help="synthesize the items with this voice (repeatable)")
    parser.add_argument("--language", help="language the voice speaks (hr, sr, bs, sl)")
    parser.add_argument("--rate", type=float, default=1.0, help="speech rate for the synthesis")
    parser.add_argument("--volume-boost", action="store_true", help="synthesize with the volume boost")
    parser.add_argument("--inflection", type=float, default=50.0,
                        help="inflection level 0-100 for the synthesis (0: monotone)")
    parser.add_argument("--cli", default=os.environ.get("LAPRDUS_CLI", "laprdus"))
    parser.add_argument("--data-dir", default=os.environ.get("LAPRDUS_DATA"))
    parser.add_argument("--out-dir", help="keep the synthesized WAVs here")
    parser.add_argument("-i", "--input-file", action="append", default=[],
                        help="text file to synthesize as one item (repeatable)")
    parser.add_argument("--clicks", action="store_true")
    parser.add_argument("--peaks", action="store_true")
    parser.add_argument("--tail", action="store_true")
    parser.add_argument("--floor", action="store_true")
    parser.add_argument("--boost", action="store_true", help="items are PLAIN BOOSTED pairs")
    parser.add_argument("--rotate", default="", help="allpass corners (Hz, comma-separated) the boost applied; "
                        "the plain signal gets them too before the residual")
    parser.add_argument("--alias", action="store_true")
    parser.add_argument("--brightness", type=float, default=0.72, help="for --alias")
    args = parser.parse_args()

    if args.alias:
        report_alias(brightness=args.brightness)
        return

    items = [(path, path) for path in args.input_file] + [(item, item) for item in args.items]
    sources = []
    if args.voice:
        for voice in args.voice:
            for item, label in items:
                name = os.path.basename(item) if os.path.isfile(item) else item
                sources.append((f"{voice}: {name}", synthesize(args, voice, item, name)))
    else:
        sources = [(label, path) for path, label in items]

    if args.boost:
        rotate = tuple(float(v) for v in args.rotate.split(",") if v.strip())
        for i in range(0, len(sources) - 1, 2):
            plain, rate = read_wav(sources[i][1])
            boosted, rate_b = read_wav(sources[i + 1][1])
            print(f"== {sources[i][0]} -> {sources[i + 1][0]}")
            report_boost(plain, boosted, rate, rotate)
        return

    for label, path in sources:
        x, rate = read_wav(path)
        if args.clicks:
            report_clicks(label, x, rate)
        elif args.peaks:
            report_peaks(label, x, rate)
        elif args.tail:
            report_tail(label, x, rate)
        elif args.floor:
            report_floor(label, x, rate)
        else:
            report_spectrum(label, x, rate)


if __name__ == "__main__":
    main()
