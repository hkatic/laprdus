# -*- coding: UTF-8 -*-
# Laprdus NVDA add-on: the text NVDA hands Laprdus is the text as written.
#
# NVDA processes the symbols of a text by the table of the language being
# spoken before any synthesizer gets it, and its tables differ: the
# Croatian, Serbian, Bosnian and English ones keep a period between two
# digits (their "decimal point" pattern), the Slovenian one makes the comma
# its decimal point and turns the period into a space, or into "pika" from
# the level "some" up. Laprdus then got "18 11 2026" for 18.11.2026, and
# "ob 8 30", "3 14" and "132 457", in Slovenian only, while every other
# platform and the other languages got the digits with their period.
#
# filter_sequence() runs before NVDA's symbol processing (the global
# plugin registers it with speech.extensions.filter_speechSequence, NVDA
# 2024.2 and later) whenever Laprdus speaks: through its own NVDA driver or
# through NVDA's SAPI5 driver with a Laprdus voice (the report of
# 2026-11-23 came from there). For each text it
#
# 1. writes a date whose separators NVDA would drop as "18. 11. 2026"
#    (_dates.py), and then
# 2. writes a period between two digits as a one dot leader (U+2024), which
#    no table of NVDA names, wherever NVDA's table for that text would not
#    keep the period at the user's level; the engine reads a one dot leader
#    between digits as the period again (restore_carried_points() in
#    src/common/text/numbers/mod.rs), before its dictionaries and numbers.
#
# Laprdus therefore gets in Slovenian exactly what it gets in Croatian. A
# stretch NVDA's automatic language switching gives another synthesizer's
# voice through SAPI5 is left as NVDA would send it.
#
# No NVDA module is imported here: the global plugin hands everything in
# (tests/nvda/test_global_plugin.py runs it without NVDA). Python 3.7.
import re

from . import _dates

CARRIER = "\u2024"
_PERIOD_BETWEEN_DIGITS = re.compile(r"(?<=[0-9])\.(?=[0-9])")

# The languages Laprdus speaks, and the three-letter codes of them
LAPRDUS_LANGUAGES = ("hr", "sr", "bs", "sl")
_ALIASES = {"hrv": "hr", "srp": "sr", "bos": "bs", "slv": "sl"}
# NVDA's SAPI5 drivers: sapi5_32 is the 32-bit one of a 64-bit NVDA
_SAPI5_DRIVERS = ("sapi5", "sapi5_32")


def _primary(tag):
	"""The primary language of a tag ("sl" for "sl_SI", "sl-SI", "slv"), or None."""
	if not tag:
		return None
	primary = str(tag).strip().lower().replace("-", "_").split("_")[0]
	return _ALIASES.get(primary, primary) or None


def _is_laprdus(voice_id, voice_info):
	names = (voice_id, getattr(voice_info, "displayName", None))
	return any(name and "laprdus" in str(name).lower() for name in names)


def laprdus_speaker(synth):
	"""None when Laprdus does not speak through `synth`; otherwise a function
	that tells, for the language a LangChangeCommand chose (None: none), whether
	Laprdus still speaks that text."""
	name = getattr(synth, "name", None)
	if name == "laprdus":
		# The driver speaks every text, in one of its four languages
		return lambda language: True
	if name not in _SAPI5_DRIVERS:
		return None
	try:
		voice = synth.voice
		voices = synth.availableVoices
	except Exception:
		return None
	if not _is_laprdus(voice, voices.get(voice) if hasattr(voices, "get") else None):
		return None

	def speaks(language):
		# SAPI picks the voice of a <lang> tag's language: a Laprdus voice for
		# the four, another one for any other language when one is installed
		primary = _primary(language)
		if primary is None or primary in LAPRDUS_LANGUAGES:
			return True
		try:
			others = list(voices.items())
		except Exception:
			return True
		return not any(
			_primary(getattr(info, "language", None)) == primary and not _is_laprdus(voice_id, info)
			for voice_id, info in others
		)

	return speaks


class Context(object):
	"""What filter_sequence() needs to know of NVDA.

	level: the punctuation level (characterProcessing.SymbolLevel);
	language: the language NVDA speaks by default (speech.getCurrentLanguage());
	auto_language, auto_dialect: NVDA's automatic language and dialect switching;
	keeps_period(locale): whether NVDA's table for `locale` keeps a period
	between digits at `level`;
	speaks(language): what laprdus_speaker() returned;
	lang_change: NVDA's LangChangeCommand class, or None.
	"""

	def __init__(self, level, language, auto_language, auto_dialect, keeps_period, speaks, lang_change):
		self.level = level
		self.language = language or ""
		self.auto_language = auto_language
		self.auto_dialect = auto_dialect
		self.keeps_period = keeps_period
		self.speaks = speaks
		self.lang_change = lang_change


def carried_periods(text):
	"""`text` with every period between two digits written as the carrier."""
	return _PERIOD_BETWEEN_DIGITS.sub(CARRIER, text)


def filter_sequence(sequence, context):
	"""The speech sequence with the texts Laprdus speaks rewritten for NVDA's
	symbol processing; the same list when nothing changed. Applying it twice
	is applying it once."""
	# The language of each stretch, as speech.speak() of NVDA works it out:
	# a LangChangeCommand counts only with automatic language switching, an
	# empty one, or one of the default language's other dialects without
	# dialect switching, means the default language
	default = context.language
	root = default.split("_")[0]
	language = default
	result = []
	changed = False
	for item in sequence:
		if context.lang_change is not None and isinstance(item, context.lang_change):
			if context.auto_language:
				language = getattr(item, "lang", None)
				if not language or (not context.auto_dialect and language.split("_")[0] == root):
					language = default
		elif isinstance(item, str) and item:
			# With automatic language switching NVDA hands the synthesizer
			# the language of every stretch, the default one too
			if context.speaks(language if context.auto_language else None):
				text = _dates.spaced_dates(item, context.level)
				if not context.keeps_period(language):
					text = carried_periods(text)
				if text != item:
					item = text
					changed = True
		result.append(item)
	return result if changed else sequence
