#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
phones.py - Which consonant a listener-like phoneme recognizer hears.

Synthesizes short items (VCV, CV, words) and spelled letters with the
laprdus CLI and runs them through a multilingual phoneme recognizer
(wav2vec2 XLSR-53 fine-tuned on Common Voice with eSpeak phoneme labels,
"facebook/wav2vec2-xlsr-53-espeak-cv-ft"), which writes out the IPA phones
it hears. A word recognizer such as Whisper guesses whole words from its
language model and hides what happened to one consonant; this shows it:
the old Zvonko's "voli" came out as "l o l i", its spelled V as "m e".
Items count as right when the expected consonant is among the phones.

Every item is synthesized in three voices at two rates (spelled letters at
two spelling speeds, in both spelling modes, and also high-passed at 500 Hz
as a phone speaker plays them), so one run gives 6-24 trials per item.
Recordings of other voices can be checked with --say ("MBROLA NG Croatian
male (cr1)", "Lana (Croatian (Croatia))", ...). See "v, l, nj and d" in
docs/formant.md.

Usage:
    phones.py [--classes v,l,nj,d,n,m] [--letters] CLI [CLI2 ...]
    phones.py --say "Lana (Croatian (Croatia))" --classes nj

Requires numpy, scipy, torch, transformers (the model is about 1.2 GB and
is downloaded on first use).
"""
import argparse
import json
import os
import subprocess
import sys
import tempfile
import warnings

import numpy as np
import scipy.io.wavfile as wavfile
import scipy.signal as sg

warnings.filterwarnings("ignore")

MODEL = "facebook/wav2vec2-xlsr-53-espeak-cv-ft"

# What counts as the consonant, by class
HEARD = {
    "v": {"v", "ʋ", "w", "β"},
    "l": {"l", "ɫ", "ɭ"},
    "nj": {"ɲ"},
    "d": {"d", "ɖ", "d̪"},
    "n": {"n"},
    "m": {"m"},
}
ITEMS = {
    "v": "ava ivi uvu eve ovo va vi vu ve vo voli vila vuk vlak vrata lav plavi svaki dva novac",
    "l": "ala ili ulu ele olo la li lu le lo loli lila luk plav sol bila malo selo pol",
    "nj": "anja inji unju enje onjo nja nje nju konj knjiga sanja njega panj manje kuhinja",
    "d": "da de di do du ada ede idi odo udu dan dobro voda dva",
    "n": "ana ini nos na ne ni",
    "m": "ama imi mos",
}
LETTERS = {"v": "v", "l": "l", "nj": "ǌ", "d": "d", "n": "n", "m": "m"}


class Recognizer:
    def __init__(self):
        import torch
        from huggingface_hub import hf_hub_download
        from transformers import Wav2Vec2FeatureExtractor, Wav2Vec2ForCTC
        self.torch = torch
        self.features = Wav2Vec2FeatureExtractor.from_pretrained(MODEL)
        self.model = Wav2Vec2ForCTC.from_pretrained(MODEL).eval()
        with open(hf_hub_download(MODEL, "vocab.json"), encoding="utf-8") as f:
            self.vocab = {v: k for k, v in json.load(f).items()}

    def phones(self, x, rate, highpass=False):
        if highpass:
            b, a = sg.butter(4, 500 / (rate / 2), "high")
            x = sg.lfilter(b, a, x)
        x = sg.resample_poly(x, 16000, rate).astype(np.float32)
        x = np.concatenate([np.zeros(4000, np.float32), x, np.zeros(4000, np.float32)])
        inputs = self.features(x, sampling_rate=16000, return_tensors="pt")
        with self.torch.no_grad():
            ids = self.model(inputs.input_values).logits.argmax(-1)[0].tolist()
        out, prev = [], None
        for i in ids:
            p = self.vocab.get(i, "")
            if i != prev and p not in ("<pad>", "<s>", "</s>", "<unk>"):
                out.append(p.rstrip("ː"))
            prev = i
        return out


def read(path):
    rate, x = wavfile.read(path)
    x = x.astype(np.float64) / 32768.0
    return (x[:, 0] if x.ndim > 1 else x), rate


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("cli", nargs="*", help="laprdus CLI binaries to compare")
    ap.add_argument("--data", default=None, help="voice data directory (-D)")
    ap.add_argument("--classes", default="v,l,nj,d,n,m")
    ap.add_argument("--voices", default="zvonko,stojan,mirsad")
    ap.add_argument("--rates", default="1.0,1.7")
    ap.add_argument("--letters", action="store_true", help="spelled letters instead of words")
    ap.add_argument("--say", default=None, help="a macOS voice to check instead of the CLI")
    args = ap.parse_args()
    classes = args.classes.split(",")
    rec = Recognizer()
    tmp = tempfile.mkdtemp()
    wav = os.path.join(tmp, "x.wav")
    sources = [("say", args.say)] if args.say else [("cli", c) for c in args.cli]
    if not sources:
        sys.exit("give a CLI or --say VOICE")
    for kind, src in sources:
        score, missed = {}, {}
        def check(cls, label, x, rate, highpass=False):
            ph = rec.phones(x, rate, highpass)
            ok = any(p in HEARD[cls] for p in ph)
            s = score.setdefault(cls, [0, 0])
            s[0] += ok
            s[1] += 1
            if not ok:
                missed.setdefault(cls, []).append(f"{label}:{''.join(ph) or '-'}")
        for cls in classes:
            if kind == "say":
                for word in ITEMS[cls].split():
                    subprocess.run(["say", "-v", src, "-o", wav, "--data-format=LEI16@22050", word],
                                   check=True, capture_output=True)
                    check(cls, word, *read(wav))
                continue
            base = [src] + (["-D", args.data] if args.data else [])
            for voice in args.voices.split(","):
                if args.letters:
                    for mode in ("names", "sounds"):
                        if mode == "sounds" and cls == "nj":
                            continue
                        for speed in ("50", "100"):
                            subprocess.run(base + ["-v", voice, "-s", "-S", speed, "-m", mode,
                                                   "-o", wav, LETTERS[cls]], check=True, capture_output=True)
                            x, rate = read(wav)
                            for hp in (False, True):
                                check(cls, f"{mode[0]}{speed}{'hp' if hp else ''}", x, rate, hp)
                    continue
                for r in args.rates.split(","):
                    for word in ITEMS[cls].split():
                        subprocess.run(base + ["-v", voice, "-r", r, "-o", wav, word],
                                       check=True, capture_output=True)
                        check(cls, word, *read(wav))
        print(f"{src}: " + "  ".join(f"{c} {v[0]}/{v[1]}" for c, v in score.items()), flush=True)
        for c, v in missed.items():
            print(f"    {c:3s} " + " ".join(v[:40]), flush=True)


if __name__ == "__main__":
    main()
