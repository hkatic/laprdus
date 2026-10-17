package com.hrvojekatic.laprdus.data

import org.json.JSONArray
import org.json.JSONException
import org.json.JSONObject

/**
 * JSON (de)serialization of user dictionaries. Shared by [DictionaryRepository]
 * (editing) and the TTS service (loading), so both agree on the format.
 *
 * Format (same as Android and Apple user dictionaries):
 * `{"version":"1.0","entries":[{"grapheme":..,"phoneme":..,"caseSensitive":..,"wholeWord":..,"comment":..}]}`
 *
 * The accent lexicon ([DictionaryType.ACCENTS]) has entries of its own, read
 * by the engine itself: `{"word":"kontr'o:l*"}` or `{"verb":"ur'e:d=i<p"}`,
 * with an optional `"language"` ("hr", "sr", "bs") and `"comment"`. The
 * entry's text is kept in [DictionaryEntry.grapheme].
 */
internal object DictionaryJson {
    const val VERSION = "1.0"

    /**
     * Parses dictionary JSON. Entries without a grapheme (for the accent
     * lexicon: with neither a word nor a verb) are skipped.
     * @throws JSONException if the document is not valid dictionary JSON
     */
    @Throws(JSONException::class)
    fun parse(json: String, type: DictionaryType = DictionaryType.MAIN): List<DictionaryEntry> {
        val root = JSONObject(json)
        val entriesArray = root.optJSONArray("entries") ?: return emptyList()
        val entries = ArrayList<DictionaryEntry>(entriesArray.length())
        for (i in 0 until entriesArray.length()) {
            val entryObj = entriesArray.optJSONObject(i) ?: continue
            if (type == DictionaryType.ACCENTS) {
                val word = entryObj.optString("word", "")
                val verb = entryObj.optString("verb", "")
                if (word.isEmpty() && verb.isEmpty()) continue
                entries.add(
                    DictionaryEntry(
                        grapheme = word.ifEmpty { verb },
                        phoneme = "",
                        verb = word.isEmpty(),
                        language = entryObj.optString("language", ""),
                        comment = entryObj.optString("comment", "")
                    )
                )
                continue
            }
            val grapheme = entryObj.optString("grapheme", "")
            if (grapheme.isEmpty()) continue
            entries.add(
                DictionaryEntry(
                    grapheme = grapheme,
                    phoneme = entryObj.optString("phoneme", ""),
                    caseSensitive = entryObj.optBoolean("caseSensitive", false),
                    wholeWord = entryObj.optBoolean("wholeWord", true),
                    comment = entryObj.optString("comment", "")
                )
            )
        }
        return entries
    }

    fun generate(entries: List<DictionaryEntry>, type: DictionaryType = DictionaryType.MAIN): String {
        val root = JSONObject()
        root.put("version", VERSION)
        val entriesArray = JSONArray()
        entries.forEach { entry ->
            entriesArray.put(if (type == DictionaryType.ACCENTS) accentObject(entry) else entryObject(entry))
        }
        root.put("entries", entriesArray)
        return root.toString(4)
    }

    /** One accent lexicon entry as the engine reads it; also what [checkAccentEntry] checks. */
    fun accentObject(entry: DictionaryEntry): JSONObject {
        val entryObj = JSONObject()
        entryObj.put(if (entry.verb) "verb" else "word", entry.grapheme)
        if (entry.language.isNotEmpty()) {
            entryObj.put("language", entry.language)
        }
        if (entry.comment.isNotEmpty()) {
            entryObj.put("comment", entry.comment)
        }
        return entryObj
    }

    private fun entryObject(entry: DictionaryEntry): JSONObject {
        val entryObj = JSONObject()
        entryObj.put("grapheme", entry.grapheme)
        entryObj.put("phoneme", entry.phoneme)
        entryObj.put("caseSensitive", entry.caseSensitive)
        entryObj.put("wholeWord", entry.wholeWord)
        if (entry.comment.isNotEmpty()) {
            entryObj.put("comment", entry.comment)
        }
        return entryObj
    }

    /** A one-entry accent lexicon, for the engine's check before an entry is saved. */
    fun accentLexiconOf(entry: DictionaryEntry): String {
        val root = JSONObject()
        root.put("entries", JSONArray().put(accentObject(entry)))
        return root.toString()
    }
}
