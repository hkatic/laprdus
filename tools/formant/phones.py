#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
phones.py - Which consonant a listener-like phoneme recognizer hears.

Synthesizes short items (VCV, CV, words) and spelled letters with the
laprdus CLI and runs them through a multilingual phoneme recognizer
(wav2vec2 XLSR-53 fine-tuned on Common Voice with phoneme labels,
"facebook/wav2vec2-xlsr-53-espeak-cv-ft"), which writes out the IPA phones
it hears. A word recognizer such as Whisper guesses whole words from its
language model and hides what happened to one consonant; this shows it:
the old Zvonko's "voli" came out as "l o l i", its spelled V as "m e".
Items count as right when the expected consonant is among the phones.

Every item is synthesized in three voices at two rates (spelled letters at
two spelling speeds, in both spelling modes, and also high-passed at 500 Hz
as a phone speaker plays them), so one run gives 6-24 trials per item.
Recordings of other voices can be checked with --say (the name of a macOS
voice). See "v, l, nj and d" in
docs/formant.md.

Usage:
    phones.py [--classes v,l,nj,d,n,m,z,ž,s,š,c,č,ć,đ,t,k,p,b,g,f,h,r,j] [--voices josip,vlado]
              [--rates 1.0,1.5,2.0] [--data data/voices] [--letters] CLI [CLI2 ...]
    phones.py --say "<a macOS voice>" --classes nj

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
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

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
# The obstruents, r and j (2026-11-17: z and ž heard as v in the formant
# voices, consonants swallowed by the recorded ones). An affricate counts when
# its phone or its two parts in a row are heard.
HEARD.update({
    "z": {"z"},
    "ž": {"ʒ", "ʐ", "ʑ", "z."},
    "s": {"s"},
    "š": {"ʃ", "ʂ", "ɕ", "s."},  # "s." and "ɕ": the recognizer's Mandarin retroflex and palatal
    "c": {"ts", "t͡s"},
    "č": {"tʃ", "t͡ʃ", "tʂ", "ʈʂ", "tɕ", "t͡ɕ", "ts."},
    "ć": {"tɕ", "t͡ɕ", "tʃ", "t͡ʃ", "c", "tʂ", "ts."},
    "đ": {"dʑ", "d͡ʑ", "dʒ", "d͡ʒ", "ɟ", "dʐ", "dz.", "tɕ", "ts."},
    "t": {"t", "t̪", "ʈ"},
    "k": {"k"},
    "p": {"p"},
    "b": {"b"},
    "g": {"ɡ", "g"},
    "f": {"f"},
    "h": {"x", "h", "χ", "ɣ"},
    "r": {"r", "ɾ"},
    "j": {"j"},
})
ITEMS = {
    "v": "ava ivi uvu eve ovo va vi vu ve vo voli vila vuk vlak vrata lav plavi svaki dva novac",
    "l": "ala ili ulu ele olo la li lu le lo loli lila luk plav sol bila malo selo pol",
    "nj": "anja inji unju enje onjo nja nje nju konj knjiga sanja njega panj manje kuhinja",
    "d": "da de di do du ada ede idi odo udu dan dobro voda dva",
    "n": "ana ini nos na ne ni",
    "m": "ama imi mos",
    "z": "aza izi uzu eze ozo za zi zu ze zo zima zub voziti jezik koza mraz zvono zdravo",
    "ž": "aža iži užu eže ožo ža ži žu že žo žaba život muž nože koža želim može",
    "s": "asa isi usu sa si su sam selo nos pas",
    "š": "aša iši ušu ša ši šu šuma naš koš",
    "c": "aca ici ucu ca ci cu cesta lice otac novac cijena",
    "č": "ača iči uču ča či ču čaj mačka oči ključ",
    "ć": "aća ići uću ća ći ću kuća noć već ćup",
    "đ": "ađa iđi uđu đa đi đu đak međa leđa rođen",
    "t": "ata iti utu ta ti tu tata sat put tri",
    "k": "aka iki uku ka ki ku kuka rak oko kraj",
    "p": "apa ipi upu pa pi pu papa lipa top prst",
    "b": "aba ibi ubu ba bi bu baba riba dobro",
    "g": "aga igi ugu ga gi gu gdje noga grad",
    "f": "afa ifi ufu fa fi fu fin kafa šef",
    "h": "aha ihi uhu ha hi hu hvala kruh uho",
    "r": "ara iri uru ra ri ru riba more prst krv",
    "j": "aja iji uju ja ju je jaje moj lijepo mlijeko",
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
    ap.add_argument("--language", default=None,
                    help="language every voice speaks (hr, sr, bs; CLI -L); default: its native one")
    ap.add_argument("--rates", default="1.0,1.7")
    ap.add_argument("--letters", action="store_true", help="spelled letters instead of words")
    ap.add_argument("--say", default=None, help="a macOS voice to check instead of the CLI")
    args = ap.parse_args()
    classes = args.classes.split(",")
    rec = Recognizer()
    tmp = tempfile.mkdtemp()
    wav = os.path.join(tmp, "x.wav")
    txt = os.path.join(tmp, "x.txt")
    sources = [("say", args.say)] if args.say else [("cli", c) for c in args.cli]
    if not sources:
        sys.exit("give a CLI or --say VOICE")
    for kind, src in sources:
        score, missed = {}, {}
        def check(cls, label, x, rate, highpass=False):
            ph = rec.phones(x, rate, highpass)
            ok = any(p in HEARD[cls] for p in ph) or any(want in "".join(ph) for want in HEARD[cls])
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
            base = [src] + (["-D", args.data] if args.data else [])                 + (["-L", args.language] if args.language else [])
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
                        # the text through a file: argv loses diacritics on Windows
                        with open(txt, "w", encoding="utf-8") as f:
                            f.write(word)
                        subprocess.run(base + ["-v", voice, "-r", r, "-o", wav, "-i", txt],
                                       check=True, capture_output=True)
                        check(cls, word, *read(wav))
        print(f"{src}: " + "  ".join(f"{c} {v[0]}/{v[1]}" for c, v in score.items()), flush=True)
        for c, v in missed.items():
            print(f"    {c:3s} " + " ".join(v[:40]), flush=True)


if __name__ == "__main__":
    main()
