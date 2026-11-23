# -*- coding: UTF-8 -*-
# Laprdus NVDA add-on: a date keeps its separators.
#
# NVDA processes the symbols of a text before the synthesizer gets it, by
# the table of the language being spoken. At the punctuation level "none" a
# symbol that is not preserved becomes a space, so a date arrived as plain
# numbers: every table drops the slashes of "18/11/2026", some the hyphens of
# "2026-11-18", and every table the periods of "18.XI.2026", which stand
# between a digit and a letter (a listener's report of 2026-11-18). The same
# date written "18. 11. 2026." was read right, because NVDA preserves a
# period before a space at every level.
#
# spaced_dates() therefore rewrites such a date into that form before NVDA's
# symbol processing: day, period, space, month, period, space, year. Only a
# separator NVDA would not speak at the user's punctuation level is
# rewritten: where the user asked to hear "kosa crta", NVDA says it as before.
#
# A period between two digits ("18.11.2026", "2026.11.18", "18.11.") is not
# rewritten here: _speechFilter.py carries it through NVDA whole, as a one
# dot leader, wherever NVDA's table would drop it, and the engine reads it as
# the period again, so the engine's own date rules see the date as written.
#
# The shapes are those of the engine (src/common/text/numbers/dates.rs):
# change the two together. Python 3.7 (NVDA 2023.3).
import re

# NVDA's symbol levels (characterProcessing.SymbolLevel)
LEVEL_SOME = 100
LEVEL_MOST = 200

_ROMAN = {
	"I": 1, "II": 2, "III": 3, "IV": 4, "V": 5, "VI": 6,
	"VII": 7, "VIII": 8, "IX": 9, "X": 10, "XI": 11, "XII": 12,
}
_ROMAN_RE = "XII|VIII|III|VII|XI|IX|IV|VI|II|X|V|I"
_TELEPHONE = (
	"telefon", "telefona", "telefonu", "tel", "mob", "mobitel", "mobitela", "faks", "fax", "gsm",
	"телефон", "тел", "моб", "факс",
)

# Not glued to a letter, a digit or a separator of another number before it ...
_BEFORE = r"(?<![\w./,+‑-])"
# ... nor after it: no letter or digit, no separator with a digit after it
_AFTER = r"(?![\w]|[/.:,‑-]\d)"
# A space after a period: an ordinary, no-break, narrow no-break or thin one
_SPACE = "[    ]?"

# 2026-11-18, 2026/11/18, and the T before the time of ISO 8601
_YEAR_FIRST = re.compile(
	_BEFORE + r"(\d{4})([-/‑])(\d{2})\2(\d{2})(?:(T)(?=\d{2}:)|" + _AFTER + ")"
)
# 18/11/2026, 18-11-2026, 18/11/26, 18/XI/2026, 11/18/2026
_SLASHED = re.compile(
	_BEFORE + r"(\d{1,2})([-/‑])(\d{1,2}|" + _ROMAN_RE + r")\2(\d{4}|\d{2})" + _AFTER
)
# 18.XI.2026, 18. XI. 2026.
_ROMAN_DOTTED = re.compile(
	_BEFORE + r"(\d{1,2})\." + _SPACE + "(" + _ROMAN_RE + r")\." + _SPACE + r"(\d{4}|\d{2})" + _AFTER
)
_WORD_BEFORE = re.compile(r"([^\W\d_]+)[ :.\t]*$")


def _telephone_before(text, start):
	"""Whether the number at `start` is named a telephone number."""
	match = _WORD_BEFORE.search(text, 0, start)
	return bool(match) and match.group(1).lower() in _TELEPHONE


def _spaced(day, month, year):
	return "%d. %d. %s" % (day, month, year)


def spaced_dates(text, level=0):
	"""`text` with its dates written as "18. 11. 2026", the form NVDA's
	symbol processing leaves alone. `level` is NVDA's punctuation level: a
	separator NVDA speaks at that level is left to NVDA."""
	if not any(c.isdigit() for c in text):
		return text
	dots = level < LEVEL_SOME
	slashes = level < LEVEL_SOME
	hyphens = level < LEVEL_MOST

	def allowed(separator):
		return {"/": slashes}.get(separator, hyphens)

	def year_first(match):
		year, separator, month, day, t = match.group(1, 2, 3, 4, 5)
		if not allowed(separator) or _telephone_before(text, match.start()):
			return match.group(0)
		if not (1000 <= int(year) <= 2999 and 1 <= int(month) <= 12 and 1 <= int(day) <= 31):
			return match.group(0)
		return _spaced(int(day), int(month), year) + (" " if t else "")

	def slashed(match):
		first, separator, second, year = match.group(1, 2, 3, 4)
		if not allowed(separator) or _telephone_before(text, match.start()):
			return match.group(0)
		if len(year) == 2 and separator != "/":
			return match.group(0)
		if len(year) == 4 and year[0] == "0":
			return match.group(0)
		roman = second in _ROMAN
		day, month = int(first), _ROMAN[second] if roman else int(second)
		if 1 <= day <= 31 and 1 <= month <= 12:
			return _spaced(day, month, year)
		if separator == "/" and not roman and 1 <= day <= 12 and 13 <= month <= 31:
			return _spaced(month, day, year)
		return match.group(0)

	def roman_dotted(match):
		first, second, year = match.group(1, 2, 3)
		if not dots or not 1 <= int(first) <= 31:
			return match.group(0)
		return "%s. %s. %s" % (first, _ROMAN[second], year)

	text = _YEAR_FIRST.sub(year_first, text)
	text = _SLASHED.sub(slashed, text)
	return _ROMAN_DOTTED.sub(roman_dotted, text)
