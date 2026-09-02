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
 */
internal object DictionaryJson {
    const val VERSION = "1.0"

    /**
     * Parses dictionary JSON. Entries without a grapheme are skipped.
     * @throws JSONException if the document is not valid dictionary JSON
     */
    @Throws(JSONException::class)
    fun parse(json: String): List<DictionaryEntry> {
        val root = JSONObject(json)
        val entriesArray = root.optJSONArray("entries") ?: return emptyList()
        val entries = ArrayList<DictionaryEntry>(entriesArray.length())
        for (i in 0 until entriesArray.length()) {
            val entryObj = entriesArray.optJSONObject(i) ?: continue
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

    fun generate(entries: List<DictionaryEntry>): String {
        val root = JSONObject()
        root.put("version", VERSION)
        val entriesArray = JSONArray()
        entries.forEach { entry ->
            val entryObj = JSONObject()
            entryObj.put("grapheme", entry.grapheme)
            entryObj.put("phoneme", entry.phoneme)
            entryObj.put("caseSensitive", entry.caseSensitive)
            entryObj.put("wholeWord", entry.wholeWord)
            if (entry.comment.isNotEmpty()) {
                entryObj.put("comment", entry.comment)
            }
            entriesArray.put(entryObj)
        }
        root.put("entries", entriesArray)
        return root.toString(4)
    }
}
