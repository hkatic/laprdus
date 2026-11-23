"""The add-on's code is still Python 3.7.

NVDA 2023.3, the last NVDA for Windows 7 SP1 and Windows 8, runs add-ons
under Python 3.7 (NVDA 2024.1 to 2025.3, the last for Windows 8.1, under
3.11; NVDA 2026.1 and later under 3.13). Laprdus supports those systems, so
every file NVDA loads from the add-on must parse with the grammar of 3.7 and
must not use what the standard library got later.

The build files (sconstruct, buildVars.py, site_scons) run under the
developer's Python and are not checked.

    python tests/nvda/test_python37.py
"""

import ast
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
ADDON = os.path.join(ROOT, "src", "platform", "windows", "nvda", "addon")

# Additions to the standard library after 3.7 that are easy to reach for.
NEWER = (
    r"\.removeprefix\(", r"\.removesuffix\(", r"\bcached_property\b", r"\bfunctools\.cache\b",
    r"\badd_dll_directory\b", r"\bwinmode\s*=", r"\bmath\.prod\b", r"\bzoneinfo\b", r"\btomllib\b",
)


def sources():
    for folder, _, names in os.walk(ADDON):
        for name in sorted(names):
            if name.endswith(".py"):
                yield os.path.join(folder, name)


def faults(path):
    with open(path, encoding="utf-8") as f:
        text = f.read()
    try:
        tree = ast.parse(text, filename=path, feature_version=(3, 7))
    except SyntaxError as error:
        return ["line %s: not Python 3.7: %s" % (error.lineno, error.msg)]
    found = []
    for node in ast.walk(tree):
        # Evaluated when the module is loaded, where 3.7 fails on them.
        if isinstance(node, ast.Subscript) and isinstance(node.value, ast.Name) \
                and node.value.id in ("list", "dict", "set", "tuple", "type"):
            found.append("line %d: %s[...] needs Python 3.9" % (node.lineno, node.value.id))
    for number, line in enumerate(text.splitlines(), 1):
        for pattern in NEWER:
            if re.search(pattern, line.split("#")[0]):
                found.append("line %d: %s is newer than Python 3.7" % (number, pattern))
    return found


def main():
    failures = 0
    paths = list(sources())
    for path in paths:
        found = faults(path)
        failures += len(found)
        print("%s %s" % ("FAIL" if found else "ok  ", os.path.relpath(path, ROOT)))
        for fault in found:
            print("     " + fault)
    if not paths:
        print("FAIL no Python files under %s" % ADDON)
        failures += 1
    print("%d files, %d failed" % (len(paths), failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
