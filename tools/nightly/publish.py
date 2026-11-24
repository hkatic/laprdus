#!/usr/bin/env python3
"""Writes the nightly download site: the page, the lists and the night's folder.

Usage (by .github/workflows/nightly.yml, from the repository root):

    python3 -I tools/nightly/publish.py \
        --files night/ --out site/ \
        --version 2.0.0-nightly.20261124 --date 2026-11-24 \
        --commit <full sha> --number 612 \
        --changes changes.tsv [--nightlies nightlies.json] \
        [--testflight "2.0.0 (41.612)"] [--keep 14] \
        [--base-url https://nightly.example] [--repo-url https://github.com/hkatic/laprdus]

--files holds the night's files under their final names: nothing else, no
folders. --changes is "<short sha>\t<subject>" per line, newest first
(git log --format='%h%x09%s'). --nightlies is the site's nightlies.json from
the night before; without it, or when it cannot be read, the site starts over.

Writes, under --out:

    <date>/<every file>, <date>/SHA256SUMS   the night (the files hard-linked
                                             or copied)
    latest/<fixed name>                      the same files under names
                                             without the version, for links
                                             that always give the newest build
    index.html                               the page for people
    latest.json, nightlies.json              the lists for programs and for
                                             the next night
    prune.txt                                the dates of the nights that fell
                                             out of the list, one per line, for
                                             the workflow to delete

The page is for screen readers first: real headings for each night and each
platform, lists, link text that names the file, no scripts. The Python
standard library only (3.9 and later).
"""

from __future__ import annotations

import argparse
import datetime
import hashlib
import html
import json
import os
import re
import shutil
import sys
from pathlib import Path

# The platforms, in the order of the page, each with the file names that
# belong to it. `kind` is the key of the notes below; `latest` the fixed name
# in latest/ (\1 is the architecture, where the name has one).
PLATFORMS = [
    ("Windows", [
        (r"^Laprdus_SAPI5_Setup_.*\.exe$", "installer", "Laprdus-nightly-setup.exe"),
        (r"^laprdus-.*\.nvda-addon$", "nvda", "laprdus-nightly.nvda-addon"),
    ]),
    ("Android", [
        (r"^Laprdus-nightly-.*\.apk$", "apk", "Laprdus-nightly.apk"),
    ]),
    ("macOS", [
        (r"^Laprdus-.*-macOS\.dmg$", "dmg", "Laprdus-nightly-macOS.dmg"),
        (r"^laprdus-.*-macos-universal\.tar\.xz$", "macos-cli", "laprdus-nightly-macos-universal.tar.xz"),
    ]),
    ("Linux", [
        (r"^laprdus_.*_(amd64|arm64)\.deb$", "deb", r"laprdus-nightly_\1.deb"),
        (r"^laprdus-dev_.*_(amd64|arm64)\.deb$", "deb-dev", r"laprdus-dev-nightly_\1.deb"),
        (r"^laprdus-[0-9][^/]*\.(x86_64|aarch64)\.rpm$", "rpm", r"laprdus-nightly.\1.rpm"),
        (r"^laprdus-devel-.*\.(x86_64|aarch64)\.rpm$", "rpm-devel", r"laprdus-devel-nightly.\1.rpm"),
        (r"^laprdus-.*-(x86_64|aarch64)\.pkg\.tar\.zst$", "arch", r"laprdus-nightly-\1.pkg.tar.zst"),
        (r"^laprdus-.*-linux-(x86_64|aarch64)\.tar\.xz$", "tarball", r"laprdus-nightly-linux-\1.tar.xz"),
    ]),
]

# What a file is, and what to do with it: shown after the link, newest night only.
NOTES = {
    "installer": "Windows installer: the SAPI5 voices, the Laprdus Configurator and the NVDA add-on, "
                 "for Windows 7 SP1 and later. It is not signed, so Windows SmartScreen may stop it: "
                 "choose “More info”, then “Run anyway”. It installs over the stable version.",
    "nvda": "The NVDA add-on alone, for NVDA 2019.3 and later. Its version is the night's date, because "
            "NVDA's add-on store takes plain numbers only. Open it with NVDA installed and running.",
    "apk": "Android 7 and later. It installs next to Laprdus from Google Play as a separate app and speech "
           "engine, “Laprdus Nightly”, with its own settings; each nightly updates the one before. "
           "Allow your browser or file manager to install apps when Android asks.",
    "dmg": "macOS 13 and later, Apple silicon and Intel: the app with its system voices, signed and "
           "notarized. Open the disk image and drag Laprdus to Applications.",
    "macos-cli": "The command-line program and the library for macOS. Unpack it and run install.sh.",
    "deb": "Debian 12 and 13, Ubuntu 22.04 and later: sudo apt install ./ followed by the file name.",
    "deb-dev": "The C header and the library for developers (Debian, Ubuntu).",
    "rpm": "Fedora 44 and later: sudo dnf install ./ followed by the file name.",
    "rpm-devel": "The C header and the library for developers (Fedora).",
    "arch": "Arch Linux: sudo pacman -U followed by the file name.",
    "tarball": "Any distribution: unpack it and run sudo ./install.sh.",
}

ARCH_NAMES = {"amd64": "x86_64", "arm64": "ARM64", "x86_64": "x86_64", "aarch64": "ARM64"}


def classify(name: str):
    """(platform, kind, latest name, architecture or None) of a file name, or None."""
    for platform, patterns in PLATFORMS:
        for pattern, kind, latest in patterns:
            match = re.match(pattern, name)
            if match:
                arch = match.group(1) if match.groups() else None
                return platform, kind, match.expand(latest), arch
    return None


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def place(source: Path, target: Path) -> None:
    """A hard link where the file system allows it, else a copy."""
    target.parent.mkdir(parents=True, exist_ok=True)
    if target.exists():
        target.unlink()
    try:
        os.link(source, target)
    except OSError:
        shutil.copy2(source, target)


def size_text(size: int) -> str:
    if size >= 1 << 20:
        return f"{size / (1 << 20):.1f} MB"
    return f"{max(1, round(size / 1024))} KB"


def long_date(iso: str) -> str:
    day = datetime.date.fromisoformat(iso)
    return f"{day.day} {day.strftime('%B')} {day.year}"


def read_changes(path: Path | None) -> list[dict]:
    changes = []
    if path and path.exists():
        for line in path.read_text(encoding="utf-8").splitlines():
            if "\t" in line:
                sha, subject = line.split("\t", 1)
                changes.append({"commit": sha.strip(), "subject": subject.strip()})
    return changes


def read_nightlies(path: Path | None) -> list[dict]:
    if not path or not path.exists():
        return []
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
        nights = data.get("nightlies", [])
        return [n for n in nights if isinstance(n, dict) and "date" in n and "files" in n]
    except (ValueError, AttributeError):
        print(f"publish.py: {path} could not be read; the list starts over", file=sys.stderr)
        return []


def file_entries(files_dir: Path, date: str, base_url: str) -> list[dict]:
    entries = []
    unknown = []
    for path in sorted(files_dir.iterdir()):
        if not path.is_file():
            continue
        found = classify(path.name)
        if not found:
            unknown.append(path.name)
            continue
        platform, kind, latest, arch = found
        entries.append({
            "platform": platform,
            "kind": kind,
            "arch": ARCH_NAMES.get(arch) if arch else None,
            "name": path.name,
            "size": path.stat().st_size,
            "sha256": sha256(path),
            "path": f"{date}/{path.name}",
            "latest": f"latest/{latest}",
            "url": f"{base_url}/{date}/{path.name}" if base_url else f"{date}/{path.name}",
        })
    if unknown:
        sys.exit(f"publish.py: files of no known platform: {', '.join(unknown)}")
    if not entries:
        sys.exit(f"publish.py: no files in {files_dir}")
    order = {kind: i for i, (_, patterns) in enumerate(PLATFORMS) for kind in [k for _, k, _ in patterns]}
    kinds = [k for _, patterns in PLATFORMS for _, k, _ in patterns]
    entries.sort(key=lambda e: (order[e["kind"]], kinds.index(e["kind"]), e["arch"] or "", e["name"]))
    return entries


# --- The page -------------------------------------------------------------

STYLE = """
:root { color-scheme: light dark; --bg: #ffffff; --fg: #1b1b1b; --muted: #555555;
        --link: #0b57d0; --rule: #d0d0d0; --code: #f2f2f2; }
@media (prefers-color-scheme: dark) {
  :root { --bg: #141414; --fg: #e8e8e8; --muted: #b0b0b0; --link: #8ab4f8;
          --rule: #3a3a3a; --code: #222222; } }
body { background: var(--bg); color: var(--fg); margin: 0;
       font: 1rem/1.55 system-ui, -apple-system, "Segoe UI", sans-serif; }
main { max-width: 52rem; margin: 0 auto; padding: 1.5rem 16px 3rem; }
a { color: var(--link); }
h1 { font-size: 1.8rem; margin: 0 0 .5rem; }
h2 { font-size: 1.4rem; margin: 2.2rem 0 .5rem; padding-top: 1rem; border-top: 1px solid var(--rule); }
h3 { font-size: 1.15rem; margin: 1.6rem 0 .4rem; }
h4 { font-size: 1rem; margin: 1.2rem 0 .3rem; }
ul { padding-left: 1.3rem; }
li { margin: .45rem 0; }
.note, .meta { color: var(--muted); }
code { background: var(--code); padding: 0 .2rem; border-radius: 3px; font-size: .85em;
       overflow-wrap: anywhere; }
"""


def link(href: str, text: str) -> str:
    return f'<a href="{html.escape(href, quote=True)}">{html.escape(text)}</a>'


def files_html(night: dict, newest: bool, level: int) -> list[str]:
    out = []
    for platform, _ in PLATFORMS:
        files = [f for f in night["files"] if f["platform"] == platform]
        if not files and not (platform == "macOS" and night.get("testflight")):
            continue
        out.append(f"<h{level}>{html.escape(platform)}</h{level}>")
        out.append("<ul>")
        for f in files:
            arch = f" ({f['arch']})" if f.get("arch") else ""
            item = f"{link(f['path'], f['name'])}{html.escape(arch)}, {size_text(f['size'])}."
            if newest:
                item += f" {html.escape(NOTES[f['kind']])}"
            item += f' <span class="meta">SHA-256: <code>{f["sha256"]}</code></span>'
            out.append(f"<li>{item}</li>")
        out.append("</ul>")
        if platform == "macOS" and night.get("testflight"):
            out.append(f"<h{level}>iPhone and iPad</h{level}>")
            out.append(f"<p>TestFlight build {html.escape(night['testflight'])}. Testers invited to "
                       "Laprdus on TestFlight get it in the TestFlight app; there is no file to download.</p>")
    return out


def changes_html(night: dict, repo_url: str, level: int) -> list[str]:
    changes = night.get("changes") or []
    if not changes:
        return []
    out = [f"<h{level}>Changes since the previous nightly</h{level}>", "<ul>"]
    for change in changes:
        sha = change["commit"]
        out.append(f"<li>{html.escape(change['subject'])} "
                   f"({link(f'{repo_url}/commit/{sha}', sha)})</li>")
    out.append("</ul>")
    return out


def night_heading(night: dict) -> str:
    return f"{html.escape(night['version'])}, {long_date(night['date'])}"


def page(nights: list[dict], repo_url: str) -> str:
    newest = nights[0]
    out = [
        "<!doctype html>",
        '<html lang="en">',
        "<head>",
        '<meta charset="utf-8">',
        '<meta name="viewport" content="width=device-width, initial-scale=1">',
        "<title>Laprdus nightly builds</title>",
        f"<style>{STYLE}</style>",
        "</head>",
        "<body>",
        "<main>",
        "<h1>Laprdus nightly builds</h1>",
        "<p>Laprdus is a text-to-speech engine for Croatian, Serbian, Bosnian and Slovenian. "
        "These builds are made automatically every night from the development branch, when it has "
        "changed since the night before, and only when every test passes. They are for testing: "
        "they can have faults that the stable versions do not. Stable versions are on "
        f"{link(f'{repo_url}/releases', 'the GitHub releases page')}.</p>",
        "<p>Please say in every report which nightly you used: its version is in the file names.</p>",
        f"<h2>Newest: {night_heading(newest)}</h2>",
        f'<p class="meta">Commit {link(repo_url + "/commit/" + newest["commit"], newest["commit"][:9])}. '
        f'{link(newest["date"] + "/SHA256SUMS", "SHA-256 checksums")} of every file.</p>',
    ]
    out += files_html(newest, newest=True, level=3)
    out += changes_html(newest, repo_url, level=3)

    out += ["<h2>Links that always give the newest build</h2>",
            "<p>These addresses do not change from night to night: each holds the newest file of its kind.</p>",
            "<ul>"]
    for f in newest["files"]:
        arch = f" ({f['arch']})" if f.get("arch") else ""
        label = {"installer": "Windows installer", "nvda": "NVDA add-on", "apk": "Android app",
                 "dmg": "macOS app", "macos-cli": "macOS command-line tools", "deb": "Debian and Ubuntu package",
                 "deb-dev": "Debian and Ubuntu developer package", "rpm": "Fedora package",
                 "rpm-devel": "Fedora developer package", "arch": "Arch Linux package",
                 "tarball": "Linux tarball"}[f["kind"]]
        out.append(f"<li>{label}{html.escape(arch)}: {link(f['latest'], f['latest'].split('/', 1)[1])}</li>")
    out.append("</ul>")

    if len(nights) > 1:
        out.append("<h2>Earlier nightlies</h2>")
        for night in nights[1:]:
            out.append(f"<h3>{night_heading(night)}</h3>")
            out.append(f'<p class="meta">Commit '
                       f'{link(repo_url + "/commit/" + night["commit"], night["commit"][:9])}. '
                       f'{link(night["date"] + "/SHA256SUMS", "SHA-256 checksums")}.</p>')
            out += files_html(night, newest=False, level=4)
            out += changes_html(night, repo_url, level=4)

    out += ["<h2>Checking a download</h2>",
            "<p>Each night has a file SHA256SUMS. On Windows: <code>certutil -hashfile FILE SHA256</code>; "
            "on Linux: <code>sha256sum -c SHA256SUMS --ignore-missing</code>; "
            "on macOS: <code>shasum -a 256 FILE</code>.</p>",
            f"<p class=\"meta\">Source code: {link(repo_url, repo_url)}. "
            f"For programs: {link('latest.json', 'latest.json')}, "
            f"{link('nightlies.json', 'nightlies.json')}.</p>",
            "</main>", "</body>", "</html>", ""]
    return "\n".join(out)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--files", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--date", required=True, help="YYYY-MM-DD")
    parser.add_argument("--commit", required=True)
    parser.add_argument("--number", required=True)
    parser.add_argument("--changes", type=Path)
    parser.add_argument("--nightlies", type=Path)
    parser.add_argument("--testflight", default="")
    parser.add_argument("--keep", type=int, default=14)
    parser.add_argument("--base-url", default="")
    parser.add_argument("--repo-url", default="https://github.com/hkatic/laprdus")
    args = parser.parse_args()

    datetime.date.fromisoformat(args.date)  # refuses anything but YYYY-MM-DD
    base_url = args.base_url.rstrip("/")
    repo_url = args.repo_url.rstrip("/")

    files = file_entries(args.files, args.date, base_url)
    night_dir = args.out / args.date
    if night_dir.exists():
        shutil.rmtree(night_dir)
    latest_dir = args.out / "latest"
    if latest_dir.exists():
        shutil.rmtree(latest_dir)
    for f in files:
        place(args.files / f["name"], args.out / f["path"])
        place(args.files / f["name"], args.out / f["latest"])
    (night_dir / "SHA256SUMS").write_text(
        "".join(f"{f['sha256']}  {f['name']}\n" for f in files), encoding="utf-8", newline="\n")

    night = {
        "version": args.version,
        "date": args.date,
        "commit": args.commit,
        "number": int(args.number),
        "testflight": args.testflight or None,
        "changes": read_changes(args.changes),
        "files": files,
    }
    # A second run on the same day replaces that day's night.
    earlier = [n for n in read_nightlies(args.nightlies) if n["date"] != args.date]
    nights = sorted([night] + earlier, key=lambda n: n["date"], reverse=True)
    kept, dropped = nights[:args.keep], nights[args.keep:]

    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / "index.html").write_text(page(kept, repo_url), encoding="utf-8", newline="\n")
    latest = {k: night[k] for k in ("version", "date", "commit", "number", "testflight")}
    latest["files"] = [{k: f[k] for k in ("platform", "kind", "arch", "name", "size", "sha256", "url")}
                       for f in files]
    (args.out / "latest.json").write_text(json.dumps(latest, indent=2) + "\n", encoding="utf-8", newline="\n")
    (args.out / "nightlies.json").write_text(json.dumps({"nightlies": kept}, indent=2) + "\n",
                                             encoding="utf-8", newline="\n")
    (args.out / "prune.txt").write_text("".join(f"{n['date']}\n" for n in dropped), encoding="utf-8",
                                        newline="\n")
    print(f"publish.py: {len(files)} files for {args.version}; {len(kept)} nights kept, "
          f"{len(dropped)} to delete")


if __name__ == "__main__":
    main()
