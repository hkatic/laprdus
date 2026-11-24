# -*- coding: UTF-8 -*-
# Laprdus NVDA add-on: its symbol dictionaries in an NVDA that does not read
# them.
#
# The add-on corrects NVDA's Slovenian symbol table with a mandatory symbol
# dictionary of its own (locale/sl/symbols-laprdus.dic: the quotation marks
# » and « at the level "most" instead of "none", so they are not spoken with
# the punctuation off; the report of 2026-11-03), named in its manifest's
# symbolDictionaries section. NVDA reads that section from 2024.4 on. An
# older NVDA (2019.3 to 2024.3; 2023.3.4 is the last for Windows 7 and 8)
# ignores it, so there NVDA's own Slovenian table was all a synthesizer got.
#
# SymbolDictionaryHook loads such a dictionary into NVDA's own symbol data
# of its locale. In NVDA 2019.3 to 2024.3 that data comes from one factory,
# SpeechSymbolProcessor.localeSymbols._localeDataFactory, which returns the
# pair (builtin, user) for a locale: NVDA's CLDR data and its symbols.dic
# loaded one after the other into `builtin` (each load adds to it, a later
# row replacing an earlier one of the same symbol), and the user's own
# symbols-<locale>.dic in `user`. The hook loads the add-on's file into
# `builtin` after NVDA's, and clears NVDA's caches so that the processors
# are made again. As in NVDA 2024.4, the user's own symbols still come
# first, the add-on's rows win over NVDA's table, and they apply to every
# synthesizer speaking that language.
#
# No NVDA module is imported here: the global plugin hands characterProcessing
# in (tests/nvda/test_global_plugin.py runs it without NVDA). Python 3.7.
import os

# The add-on's dictionary in a locale's folder, as the manifest names it
FILE_NAME = "symbols-laprdus.dic"


def addon_dictionaries(addon_dir):
	"""{locale: path} of the add-on's symbol dictionaries under `addon_dir`."""
	found = {}
	folder = os.path.join(addon_dir, "locale")
	try:
		locales = sorted(os.listdir(folder))
	except OSError:
		return found
	for locale in locales:
		path = os.path.join(folder, locale, FILE_NAME)
		if os.path.isfile(path):
			found[locale] = path
	return found


def reads_manifest_dictionaries(characterProcessing):
	"""Whether this NVDA loads an add-on's symbol dictionaries itself (2024.4+)."""
	return hasattr(characterProcessing, "listAvailableSymbolDictionaryDefinitions")


class SymbolDictionaryHook(object):
	"""The add-on's symbol dictionaries ({locale: path}) in NVDA's symbol data,
	for an NVDA before 2024.4. Raises AttributeError when this NVDA keeps its
	symbol data otherwise; remove() gives NVDA its own data back."""

	def __init__(self, characterProcessing, dictionaries, log=None):
		self._characterProcessing = characterProcessing
		self._map = characterProcessing.SpeechSymbolProcessor.localeSymbols
		self._original = original = self._map._localeDataFactory
		if not callable(original) or not callable(characterProcessing.clearSpeechSymbols):
			raise AttributeError("NVDA's symbol data has no factory to extend")
		self._dictionaries = dict(dictionaries)
		self._active = True
		hook = self

		def factory(locale):
			data = original(locale)
			path = hook._dictionaries.get(locale) if hook._active else None
			if path:
				try:
					data[0].load(path, allowComplexSymbols=False)
				except Exception:
					# NVDA's own data stays as it was loaded
					if log is not None:
						log.debugWarning("Laprdus: %s could not be loaded" % path, exc_info=True)
			return data

		self._factory = factory
		self._map._localeDataFactory = factory
		characterProcessing.clearSpeechSymbols()

	@property
	def installed(self):
		"""Whether the hook still adds the dictionaries."""
		return self._active

	def remove(self):
		self._active = False
		if self._map._localeDataFactory is self._factory:
			self._map._localeDataFactory = self._original
		self._characterProcessing.clearSpeechSymbols()
