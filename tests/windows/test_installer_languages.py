"""Every language of the Windows installer has every message.

Checks, for installers/windows/laprdus_sapi5.iss and the message files it
names:

- each language has every Laprdus message (Laprdus*) that any language has,
  with the same %-placeholders as the English one;
- every message the script uses ({cm:Name}, CustomMessage('Name')) exists in
  every language;
- each of our own .isl files has every standard message of the installed
  Inno Setup's Default.isl that is not empty there (skipped when Inno Setup
  is not installed).

    python tests/windows/test_installer_languages.py
"""

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
INSTALLER = ROOT / "installers" / "windows"
INNO = pathlib.Path(r"C:\Program Files (x86)\Inno Setup 6")


def sections(path):
    """{section: {name: value}} of an .isl or .iss file."""
    found, current = {}, None
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        header = re.fullmatch(r"\[(\w+)\]\s*", line)
        if header:
            current = found.setdefault(header.group(1), {})
        elif current is not None and not line.startswith(";"):
            entry = re.match(r"([A-Za-z0-9_.]+)=(.*)", line)
            if entry:
                current[entry.group(1)] = entry.group(2)
    return found


def placeholders(text):
    return sorted(re.findall(r"%[0-9n]", text))


def main():
    script_path = INSTALLER / "laprdus_sapi5.iss"
    script_text = script_path.read_text(encoding="utf-8-sig")
    script = sections(script_path)
    languages = re.findall(r'^Name: "(\w+)"; MessagesFile: "([^"]+)"', script_text, re.M)
    failures = []

    custom = {}  # language -> {message: text}
    own_files = {}
    for name, source in languages:
        compiler_file = source.startswith("compiler:")
        path = INNO / source[len("compiler:"):] if compiler_file else INSTALLER / source
        messages = {}
        if path.exists():
            messages.update(sections(path).get("CustomMessages", {}))
            if not compiler_file:
                own_files[name] = path
        elif not compiler_file:
            failures.append(f"{name}: {source} does not exist")
        # The script's own [CustomMessages]: `language.Name=` for one language.
        for key, value in script.get("CustomMessages", {}).items():
            language, _, message = key.rpartition(".")
            if language == name:
                messages[message] = value
        custom[name] = messages

    ours = sorted({message for messages in custom.values() for message in messages if message.startswith("Laprdus")})
    used = sorted(set(re.findall(r"\{cm:(\w+)\}", script_text)) | set(re.findall(r"CustomMessage\('(\w+)'\)", script_text)))
    reference = custom.get("english", {})
    for name, messages in custom.items():
        for message in ours:
            if message not in messages:
                failures.append(f"{name}: {message} is missing")
            elif not messages[message].strip():
                failures.append(f"{name}: {message} is empty")
            elif message in reference and placeholders(messages[message]) != placeholders(reference[message]):
                failures.append(f"{name}: {message} has other placeholders than the English text")
        for message in used:
            if message not in messages and (INNO.exists() or message.startswith("Laprdus")):
                failures.append(f"{name}: the script uses {message}, which is missing")

    default = INNO / "Default.isl"
    if default.exists():
        standard = sections(default)
        for name, path in own_files.items():
            mine = sections(path)
            for section in ("Messages", "CustomMessages"):
                for message, text in standard.get(section, {}).items():
                    if text.strip() and message not in mine.get(section, {}):
                        failures.append(f"{name}: {path.name} lacks the standard message {message}")
                    elif text.strip() and placeholders(mine[section][message]) != placeholders(text) and "%n" not in text:
                        failures.append(f"{name}: {path.name}: {message} has other placeholders than Default.isl")
    else:
        print("Inno Setup is not installed: the standard messages were not compared")

    for failure in failures:
        print("FAIL " + failure)
    print(f"{len(languages)} languages, {len(ours)} Laprdus messages, {len(used)} used by the script: {len(failures)} failed")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
