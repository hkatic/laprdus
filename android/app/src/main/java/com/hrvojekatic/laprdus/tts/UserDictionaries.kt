package com.hrvojekatic.laprdus.tts

import android.util.Log
import com.hrvojekatic.laprdus.BuildConfig
import com.hrvojekatic.laprdus.data.DictionaryJson
import com.hrvojekatic.laprdus.data.DictionaryType
import com.hrvojekatic.laprdus.data.storage.LaprdusStorage
import java.io.File

/**
 * Loads the user's own dictionaries from the device-protected dictionary
 * folder into an engine, on top of the bundled ones. The speech service and
 * the app's preview read the same files into their own engines.
 */
internal object UserDictionaries {

    private const val TAG = "UserDictionaries"

    /**
     * The accent lexicon (accents.json) replaces the engine's previous one; a
     * missing file clears it. The entries of user.json, spelling.json and
     * emoji.json are added one by one, which does NOT clear existing entries;
     * a spelling or emoji entry replaces the bundled one for the same
     * character or emoji. A file that cannot be read is logged and skipped.
     */
    fun load(engine: LaprdusTTS, dictionaryDir: File) {
        // The accent lexicon of the formant voices is one file the engine
        // parses itself.
        val accents = File(dictionaryDir, LaprdusStorage.ACCENT_LEXICON_FILE_NAME)
        if (accents.isFile) {
            try {
                val accepted = engine.loadAccentLexicon(accents.readText(Charsets.UTF_8))
                Log.i(TAG, "Loaded user ${accents.name}: accepted=$accepted")
            } catch (e: Exception) {
                Log.e(TAG, "Failed to load user ${accents.name}: ${e.message}")
                engine.clearAccentLexicon()
            }
        } else {
            engine.clearAccentLexicon()
        }

        // Every dictionary type is saved in the same entry format.
        for (type in DictionaryType.entries) {
            val file = File(dictionaryDir, type.fileName)
            if (!file.isFile) {
                if (BuildConfig.DEBUG) Log.d(TAG, "No user ${type.fileName} found")
                continue
            }

            try {
                val entries = DictionaryJson.parse(file.readText(Charsets.UTF_8))
                var count = 0
                for (entry in entries) {
                    if (entry.grapheme.isEmpty() || entry.phoneme.isEmpty()) continue
                    when (type) {
                        DictionaryType.MAIN -> engine.addPronunciation(
                            entry.grapheme, entry.phoneme, entry.caseSensitive, entry.wholeWord
                        )
                        DictionaryType.SPELLING -> engine.addSpellingEntry(entry.grapheme, entry.phoneme)
                        DictionaryType.EMOJI -> engine.addEmojiEntry(entry.grapheme, entry.phoneme)
                    }
                    count++
                }
                Log.i(TAG, "Loaded $count user entries from ${type.fileName}")
            } catch (e: Exception) {
                Log.e(TAG, "Failed to load user ${type.fileName}: ${e.message}")
            }
        }
    }
}
