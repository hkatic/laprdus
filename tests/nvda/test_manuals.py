"""Checks the add-on's user manuals, rendered as the add-on build renders them.

    python tests/nvda/test_manuals.py [hr en sr sl]

The Croatian manual (nvda-addon/addon/doc/hr/readme.md) is the source; the
English one (nvda-addon/readme.md, the add-on's base language) and the Serbian
and Slovenian ones are its translations. What is covered, for each of them:
- the HTML is clean: every internal link has its heading, no raw code fence,
  no raw table, no list run into a paragraph, and one <h1> (a shell comment
  in a broken code block would be another);
- a translation has the structure of the source: the same headings, tables,
  code blocks, list items and links, the same URLs, the same inline code;
- code blocks are the same but for the comments, which are translated.
"""
import io
import os
import re
import sys
from collections import Counter
from html.parser import HTMLParser

import markdown

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
ADDON = os.path.join(ROOT, "nvda-addon")
sys.path.insert(0, ADDON)
import buildVars  # noqa: E402

MANUALS = {
    "hr": os.path.join(ADDON, "addon", "doc", "hr", "readme.md"),
    "en": os.path.join(ADDON, "readme.md"),
    "sr": os.path.join(ADDON, "addon", "doc", "sr", "readme.md"),
    "sl": os.path.join(ADDON, "addon", "doc", "sl", "readme.md"),
}

failures = []


def check(name, ok, detail=""):
    print("%s %s%s" % ("ok  " if ok else "FAIL", name, (": %s" % (detail,)) if detail and not ok else ""))
    if not ok:
        failures.append(name)


class Facts(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.ids = []
        self.links = []
        self.tags = Counter()
        self.table_shape = []
        self.pre = []
        self.inline_code = []
        self.paragraphs = []
        self._pre = None
        self._code = None
        self._p = None
        self._cells = 0
        self._rows = []

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        self.tags[tag] += 1
        if "id" in attrs:
            self.ids.append(attrs["id"])
        if tag == "a" and "href" in attrs:
            self.links.append(attrs["href"])
        if tag == "pre":
            self._pre = []
        elif tag == "code" and self._pre is None:
            self._code = []
        elif tag == "p":
            self._p = []
        elif tag == "tr":
            self._cells = 0
        elif tag in ("td", "th"):
            self._cells += 1
        elif tag == "table":
            self._rows = []

    def handle_endtag(self, tag):
        if tag == "pre":
            self.pre.append("".join(self._pre))
            self._pre = None
        elif tag == "code" and self._code is not None:
            self.inline_code.append("".join(self._code))
            self._code = None
        elif tag == "p":
            self.paragraphs.append("".join(self._p or []))
            self._p = None
        elif tag == "tr":
            self._rows.append(self._cells)
        elif tag == "table":
            self.table_shape.append(tuple(self._rows))

    def handle_data(self, data):
        if self._pre is not None:
            self._pre.append(data)
        if self._code is not None:
            self._code.append(data)
        if self._p is not None:
            self._p.append(data)


def facts_of(path):
    with open(path, encoding="utf-8") as f:
        html = markdown.markdown(f.read(), extensions=buildVars.markdownExtensions)
    facts = Facts()
    facts.feed(html)
    facts.html = html
    return facts


def code_without_comments(block):
    """A code block with what may be translated in it taken out: comment lines
    and the free text of a dictionary entry's comment."""
    lines = []
    for line in block.splitlines():
        line = re.sub(r"^(\s*)(#|//).*$", r"\1\2", line)
        line = re.sub(r'("comment"\s*:\s*)".*?"', r'\1""', line)
        lines.append(line.rstrip())
    return "\n".join(lines)


def test_clean(code, facts):
    dead = sorted({link for link in facts.links if link.startswith("#") and link[1:] not in facts.ids})
    check("%s: every internal link has its heading" % code, not dead, dead[:8])
    check("%s: has internal links" % code, sum(1 for link in facts.links if link.startswith("#")) > 50)
    check("%s: no raw code fence" % code, "```" not in facts.html)
    raw_tables = [p[:60] for p in facts.paragraphs if p.lstrip().startswith("|")]
    check("%s: no raw table" % code, not raw_tables, raw_tables[:3])
    run_in = [p[:80] for p in facts.paragraphs if re.search(r"\n\s*(?:[-*+]|\d+\.)\s", p)]
    check("%s: no list run into a paragraph" % code, not run_in, run_in[:3])
    check("%s: one top-level heading" % code, facts.tags["h1"] == 1, facts.tags["h1"])


def test_same_structure(code, facts, source):
    for tag in ("h1", "h2", "h3", "h4", "h5", "table", "ul", "ol", "li", "pre", "blockquote", "strong", "hr"):
        check("%s: as many <%s> as Croatian" % (code, tag), facts.tags[tag] == source.tags[tag],
              (facts.tags[tag], source.tags[tag]))
    check("%s: tables have the same shape" % code, facts.table_shape == source.table_shape)
    external = lambda f: Counter(link for link in f.links if not link.startswith("#"))
    check("%s: the same external links" % code, external(facts) == external(source),
          (external(facts) - external(source), external(source) - external(facts)))
    internal = lambda f: sum(1 for link in f.links if link.startswith("#"))
    check("%s: as many internal links" % code, internal(facts) == internal(source), (internal(facts), internal(source)))
    different = [(a[:70], b[:70]) for a, b in zip(map(code_without_comments, facts.pre), map(code_without_comments, source.pre)) if a != b]
    check("%s: code blocks are those of Croatian" % code, not different, different[:3])
    inline = Counter(facts.inline_code)
    original = Counter(source.inline_code)
    check("%s: the same inline code" % code, inline == original,
          (list((inline - original).items())[:8], list((original - inline).items())[:8]))


if __name__ == "__main__":
    wanted = sys.argv[1:] or list(MANUALS)
    source = facts_of(MANUALS["hr"])
    for code in wanted:
        if not os.path.isfile(MANUALS[code]):
            check("%s: manual exists" % code, False, MANUALS[code])
            continue
        facts = source if code == "hr" else facts_of(MANUALS[code])
        test_clean(code, facts)
        if code != "hr":
            test_same_structure(code, facts, source)
    print("%d failed" % len(failures))
    sys.exit(1 if failures else 0)
