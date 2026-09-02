package com.hrvojekatic.laprdus.data

import com.hrvojekatic.laprdus.data.migration.DictionaryMigrator
import com.hrvojekatic.laprdus.data.migration.DirectBootMigrationException
import com.hrvojekatic.laprdus.data.migration.LegacyMigrator
import com.hrvojekatic.laprdus.data.migration.MigrationResult
import com.hrvojekatic.laprdus.data.storage.StorageLogger
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.joinAll
import kotlinx.coroutines.launch
import kotlinx.coroutines.test.runTest
import org.json.JSONException
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertThrows
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config
import java.io.File

/**
 * Unit tests for [DictionaryRepository], [DictionaryJson] and their data classes.
 *
 * Runs under Robolectric only because the JSON layer uses android's `org.json`;
 * the repository itself is exercised through its internal constructor over a
 * temporary directory, so no Android storage singleton is touched.
 */
@RunWith(RobolectricTestRunner::class)
@Config(sdk = [36], manifest = Config.NONE)
class DictionaryRepositoryTest {

    @get:Rule
    val tempFolder = TemporaryFolder()

    private lateinit var dictionaryDir: File

    @Before
    fun setup() {
        dictionaryDir = tempFolder.newFolder("dictionaries")
    }

    private fun newRepository(migrator: LegacyMigrator? = null): DictionaryRepository =
        DictionaryRepository(
            dictionaryDir = dictionaryDir,
            migrator = migrator,
            logger = StorageLogger.None
        )

    private fun migratorFor(
        legacyDir: File,
        isUserUnlocked: () -> Boolean = { true }
    ): DictionaryMigrator = DictionaryMigrator(
        legacyDir = { legacyDir },
        targetDir = dictionaryDir,
        fileNames = listOf("user.json", "spelling.json", "emoji.json"),
        isUserUnlocked = isUserUnlocked,
        logger = StorageLogger.None
    )

    private fun userFile(): File = File(dictionaryDir, DictionaryType.MAIN.fileName)

    private fun writeDictionary(file: File, entries: List<DictionaryEntry>) {
        file.parentFile?.mkdirs()
        file.writeText(DictionaryJson.generate(entries), Charsets.UTF_8)
    }

    /** Ids are regenerated on load, so comparisons ignore them. */
    private fun DictionaryEntry.withoutId(): DictionaryEntry = copy(id = "")

    private fun List<DictionaryEntry>.withoutIds(): List<DictionaryEntry> = map { it.withoutId() }

    // ==========================================================================
    // DictionaryEntry Data Class Tests
    // ==========================================================================

    @Test
    fun `DictionaryEntry has correct default values`() {
        val entry = DictionaryEntry(grapheme = "test", phoneme = "tst")

        assertFalse(entry.caseSensitive) // default false
        assertTrue(entry.wholeWord) // default true
        assertEquals("", entry.comment) // default empty
        assertNotNull(entry.id) // UUID generated
    }

    @Test
    fun `DictionaryEntry preserves custom values`() {
        val entry = DictionaryEntry(
            id = "custom-id",
            grapheme = "Hello",
            phoneme = "Helo",
            caseSensitive = true,
            wholeWord = false,
            comment = "Test comment"
        )

        assertEquals("custom-id", entry.id)
        assertEquals("Hello", entry.grapheme)
        assertEquals("Helo", entry.phoneme)
        assertTrue(entry.caseSensitive)
        assertFalse(entry.wholeWord)
        assertEquals("Test comment", entry.comment)
    }

    @Test
    fun `DictionaryEntry copy works correctly`() {
        val original = DictionaryEntry(
            id = "orig-1",
            grapheme = "original",
            phoneme = "orig",
            caseSensitive = true,
            wholeWord = false,
            comment = "comment"
        )

        val copy = original.copy(phoneme = "modified")

        assertEquals(original.id, copy.id)
        assertEquals(original.grapheme, copy.grapheme)
        assertEquals("modified", copy.phoneme)
        assertEquals(original.caseSensitive, copy.caseSensitive)
        assertEquals(original.wholeWord, copy.wholeWord)
        assertEquals(original.comment, copy.comment)
    }

    @Test
    fun `DictionaryEntry equals works correctly`() {
        val entry1 = DictionaryEntry(
            id = "id-1",
            grapheme = "test",
            phoneme = "tst"
        )
        val entry2 = DictionaryEntry(
            id = "id-1",
            grapheme = "test",
            phoneme = "tst"
        )
        val entry3 = DictionaryEntry(
            id = "id-2",
            grapheme = "test",
            phoneme = "tst"
        )

        assertEquals(entry1, entry2)
        assertFalse(entry1 == entry3)
    }

    // ==========================================================================
    // DictionaryType Enum Tests
    // ==========================================================================

    @Test
    fun `DictionaryType enum has three values`() {
        val types = DictionaryType.entries
        assertEquals(3, types.size)
    }

    @Test
    fun `DictionaryType MAIN is first value`() {
        assertEquals(DictionaryType.MAIN, DictionaryType.entries[0])
    }

    @Test
    fun `DictionaryType SPELLING is second value`() {
        assertEquals(DictionaryType.SPELLING, DictionaryType.entries[1])
    }

    @Test
    fun `DictionaryType EMOJI is third value`() {
        assertEquals(DictionaryType.EMOJI, DictionaryType.entries[2])
    }

    // ==========================================================================
    // Dictionary File Name Tests
    // ==========================================================================

    @Test
    fun `main dictionary uses user json filename`() {
        assertEquals("user.json", DictionaryType.MAIN.fileName)
    }

    @Test
    fun `spelling dictionary uses spelling json filename`() {
        assertEquals("spelling.json", DictionaryType.SPELLING.fileName)
    }

    @Test
    fun `emoji dictionary uses emoji json filename`() {
        assertEquals("emoji.json", DictionaryType.EMOJI.fileName)
    }

    @Test
    fun `dictionary file names are distinct`() {
        val names = DictionaryType.entries.map { it.fileName }
        assertEquals(names.size, names.toSet().size)
    }

    @Test
    fun `getDictionaryFile resolves every type inside the dictionary directory`() {
        val repository = newRepository()

        for (type in DictionaryType.entries) {
            assertEquals(File(dictionaryDir, type.fileName), repository.getDictionaryFile(type))
        }
    }

    // ==========================================================================
    // Load and Save Round Trip Tests
    // ==========================================================================

    @Test
    fun `loadDictionary returns empty success when the file is missing`() = runTest {
        val repository = newRepository()

        val result = repository.loadDictionary(DictionaryType.MAIN)

        assertTrue(result.isSuccess)
        assertTrue(result.getOrThrow().isEmpty())
        assertTrue(repository.entries.first().isEmpty())
        assertFalse(userFile().exists())
    }

    @Test
    fun `saveEntry then loadDictionary preserves entry content`() = runTest {
        val repository = newRepository()
        repository.loadDictionary(DictionaryType.MAIN)
        val entry = DictionaryEntry(
            grapheme = "Facebook",
            phoneme = "Fejzbuk",
            caseSensitive = true,
            wholeWord = false,
            comment = "Social media platform"
        )

        assertTrue(repository.saveEntry(entry).isSuccess)
        assertEquals(listOf(entry), repository.entries.first())

        // A fresh repository over the same directory must see the persisted entry.
        val loaded = newRepository().loadDictionary(DictionaryType.MAIN).getOrThrow()
        assertEquals(1, loaded.size)
        assertEquals(entry.withoutId(), loaded[0].withoutId())
    }

    @Test
    fun `saveEntry replaces an existing entry with the same id`() = runTest {
        val repository = newRepository()
        repository.loadDictionary(DictionaryType.MAIN)
        val entry = DictionaryEntry(id = "entry-1", grapheme = "old", phoneme = "old")
        repository.saveEntry(entry)

        assertTrue(repository.saveEntry(entry.copy(phoneme = "new")).isSuccess)

        val inMemory = repository.entries.first()
        assertEquals(1, inMemory.size)
        assertEquals("new", inMemory[0].phoneme)
        val loaded = newRepository().loadDictionary(DictionaryType.MAIN).getOrThrow()
        assertEquals(1, loaded.size)
        assertEquals("new", loaded[0].phoneme)
    }

    @Test
    fun `deleteEntry removes the entry from memory and disk`() = runTest {
        val repository = newRepository()
        repository.loadDictionary(DictionaryType.MAIN)
        repository.saveEntry(DictionaryEntry(id = "keep", grapheme = "keep", phoneme = "k"))
        repository.saveEntry(DictionaryEntry(id = "drop", grapheme = "drop", phoneme = "d"))

        assertTrue(repository.deleteEntry("drop").isSuccess)

        assertEquals(listOf("keep"), repository.entries.first().map { it.grapheme })
        val loaded = newRepository().loadDictionary(DictionaryType.MAIN).getOrThrow()
        assertEquals(listOf("keep"), loaded.map { it.grapheme })
    }

    @Test
    fun `deleteEntry with an unknown id leaves entries unchanged`() = runTest {
        val repository = newRepository()
        repository.loadDictionary(DictionaryType.MAIN)
        repository.saveEntry(DictionaryEntry(id = "keep", grapheme = "keep", phoneme = "k"))

        assertTrue(repository.deleteEntry("missing").isSuccess)

        assertEquals(listOf("keep"), repository.entries.first().map { it.grapheme })
    }

    @Test
    fun `getCurrentType follows loadDictionary`() = runTest {
        val repository = newRepository()
        assertEquals(DictionaryType.MAIN, repository.getCurrentType())

        repository.loadDictionary(DictionaryType.SPELLING)
        assertEquals(DictionaryType.SPELLING, repository.getCurrentType())

        repository.loadDictionary(DictionaryType.EMOJI)
        assertEquals(DictionaryType.EMOJI, repository.getCurrentType())

        repository.loadDictionary(DictionaryType.MAIN)
        assertEquals(DictionaryType.MAIN, repository.getCurrentType())
    }

    @Test
    fun `saveEntry writes to the file of the currently loaded type`() = runTest {
        val repository = newRepository()
        repository.loadDictionary(DictionaryType.SPELLING)

        assertTrue(repository.saveEntry(DictionaryEntry(grapheme = "Č", phoneme = "Če")).isSuccess)

        assertTrue(File(dictionaryDir, "spelling.json").isFile)
        assertFalse(userFile().exists())
        // Loading another type does not carry the entries over.
        assertTrue(repository.loadDictionary(DictionaryType.MAIN).getOrThrow().isEmpty())
        assertEquals(
            listOf("Č"),
            repository.loadDictionary(DictionaryType.SPELLING).getOrThrow().map { it.grapheme }
        )
    }

    // ==========================================================================
    // On-Disk Format and Atomic Write Tests
    // ==========================================================================

    @Test
    fun `saved file has version 1_0 and an entries array`() = runTest {
        val repository = newRepository()
        repository.loadDictionary(DictionaryType.MAIN)
        repository.saveEntry(
            DictionaryEntry(
                grapheme = "test",
                phoneme = "tst",
                caseSensitive = true,
                wholeWord = false,
                comment = "note"
            )
        )

        val root = JSONObject(userFile().readText(Charsets.UTF_8))

        assertEquals("1.0", root.getString("version"))
        val entries = root.getJSONArray("entries")
        assertEquals(1, entries.length())
        val obj = entries.getJSONObject(0)
        assertEquals("test", obj.getString("grapheme"))
        assertEquals("tst", obj.getString("phoneme"))
        assertTrue(obj.getBoolean("caseSensitive"))
        assertFalse(obj.getBoolean("wholeWord"))
        assertEquals("note", obj.getString("comment"))
    }

    @Test
    fun `saves leave no temp files behind`() = runTest {
        val repository = newRepository()
        repository.loadDictionary(DictionaryType.MAIN)

        repeat(5) { i ->
            assertTrue(repository.saveEntry(DictionaryEntry(grapheme = "word$i", phoneme = "p$i")).isSuccess)
        }
        assertTrue(repository.deleteEntry(repository.entries.first()[0].id).isSuccess)

        val names = dictionaryDir.list()!!.toList()
        assertEquals(listOf("user.json"), names)
        assertTrue(names.none { it.endsWith(AtomicFiles.TEMP_SUFFIX) })
        assertEquals(4, DictionaryJson.parse(userFile().readText(Charsets.UTF_8)).size)
    }

    // ==========================================================================
    // Corrupt File Tests
    // ==========================================================================

    @Test
    fun `malformed JSON yields failure and clears the entries flow`() = runTest {
        val repository = newRepository()
        repository.loadDictionary(DictionaryType.MAIN)
        repository.saveEntry(DictionaryEntry(grapheme = "a", phoneme = "b"))
        assertEquals(1, repository.entries.first().size)

        userFile().writeText("{ this is not json", Charsets.UTF_8)
        val result = repository.loadDictionary(DictionaryType.MAIN)

        assertTrue(result.isFailure)
        assertNotNull(result.exceptionOrNull())
        assertTrue(repository.entries.first().isEmpty())
    }

    @Test
    fun `dictionary without an entries array loads as empty`() = runTest {
        userFile().writeText("""{"version":"1.0"}""", Charsets.UTF_8)
        val repository = newRepository()

        val result = repository.loadDictionary(DictionaryType.MAIN)

        assertTrue(result.isSuccess)
        assertTrue(result.getOrThrow().isEmpty())
    }

    // ==========================================================================
    // Legacy Storage Migration Tests
    // ==========================================================================

    @Test
    fun `first load migrates the legacy dictionary`() = runTest {
        val legacyDir = tempFolder.newFolder("legacy")
        val legacyEntries = listOf(
            DictionaryEntry(grapheme = "Facebook", phoneme = "Fejzbuk"),
            DictionaryEntry(
                grapheme = "WhatsApp",
                phoneme = "Vocap",
                caseSensitive = true,
                wholeWord = false,
                comment = "chat"
            )
        )
        writeDictionary(File(legacyDir, "user.json"), legacyEntries)
        val repository = newRepository(migratorFor(legacyDir))

        val loaded = repository.loadDictionary(DictionaryType.MAIN).getOrThrow()

        assertEquals(legacyEntries.withoutIds(), loaded.withoutIds())
        assertEquals(legacyEntries.withoutIds(), repository.entries.first().withoutIds())
        assertFalse(File(legacyDir, "user.json").exists())
        assertTrue(userFile().isFile)
        val migration = repository.lastMigrationResult
        assertTrue("expected Migrated but was $migration", migration is MigrationResult.Migrated)
        assertEquals(1, (migration as MigrationResult.Migrated).itemCount)
        assertNull(repository.storageError.value)
        assertTrue(dictionaryDir.list()!!.none { it.endsWith(AtomicFiles.TEMP_SUFFIX) })
    }

    @Test
    fun `migration runs once and later operations report NotNeeded`() = runTest {
        val legacyDir = tempFolder.newFolder("legacy")
        writeDictionary(File(legacyDir, "user.json"), listOf(DictionaryEntry(grapheme = "a", phoneme = "b")))
        val repository = newRepository(migratorFor(legacyDir))
        repository.loadDictionary(DictionaryType.MAIN)
        assertTrue(repository.lastMigrationResult is MigrationResult.Migrated)

        repository.loadDictionary(DictionaryType.MAIN)
        assertEquals(MigrationResult.NotNeeded, repository.lastMigrationResult)

        repository.saveEntry(DictionaryEntry(grapheme = "c", phoneme = "d"))
        assertEquals(MigrationResult.NotNeeded, repository.lastMigrationResult)
        assertEquals(listOf("a", "c"), repository.entries.first().map { it.grapheme })
    }

    @Test
    fun `existing device-protected dictionary wins over the legacy copy`() = runTest {
        val legacyDir = tempFolder.newFolder("legacy")
        writeDictionary(File(legacyDir, "user.json"), listOf(DictionaryEntry(grapheme = "old", phoneme = "o")))
        writeDictionary(userFile(), listOf(DictionaryEntry(grapheme = "new", phoneme = "n")))
        val repository = newRepository(migratorFor(legacyDir))

        val loaded = repository.loadDictionary(DictionaryType.MAIN).getOrThrow()

        assertEquals(listOf("new"), loaded.map { it.grapheme })
        assertFalse(File(legacyDir, "user.json").exists())
        assertEquals(MigrationResult.Migrated(0), repository.lastMigrationResult)
    }

    @Test
    fun `migration is skipped while the user is locked`() = runTest {
        val legacyDir = tempFolder.newFolder("legacy")
        val legacyFile = File(legacyDir, "user.json")
        writeDictionary(legacyFile, listOf(DictionaryEntry(grapheme = "a", phoneme = "b")))
        var unlocked = false
        val repository = newRepository(migratorFor(legacyDir) { unlocked })

        val lockedResult = repository.loadDictionary(DictionaryType.MAIN)

        assertTrue(lockedResult.isSuccess)
        assertTrue(lockedResult.getOrThrow().isEmpty())
        assertEquals(MigrationResult.SkippedLocked, repository.lastMigrationResult)
        assertTrue(legacyFile.isFile)
        assertFalse(userFile().exists())

        // After unlock the next operation migrates.
        unlocked = true
        val unlockedResult = repository.loadDictionary(DictionaryType.MAIN)

        assertEquals(listOf("a"), unlockedResult.getOrThrow().map { it.grapheme })
        assertEquals(MigrationResult.Migrated(1), repository.lastMigrationResult)
        assertFalse(legacyFile.exists())
    }

    @Test
    fun `migration failure is contained and reported through storageError`() = runTest {
        val migrator = object : LegacyMigrator(isUserUnlocked = { true }, logger = StorageLogger.None) {
            override suspend fun performMigration(): MigrationResult {
                throw DirectBootMigrationException("boom")
            }
        }
        val repository = newRepository(migrator)

        val result = repository.loadDictionary(DictionaryType.MAIN)

        assertTrue(result.isSuccess)
        assertTrue(result.getOrThrow().isEmpty())
        assertEquals("boom", repository.storageError.value)
        assertTrue(
            "a contained failure is reported as RetryLater",
            repository.lastMigrationResult is MigrationResult.RetryLater
        )
        // The repository keeps working with the current (device-protected) files.
        assertTrue(repository.saveEntry(DictionaryEntry(grapheme = "a", phoneme = "b")).isSuccess)
        assertTrue(userFile().isFile)
        assertEquals("boom", repository.storageError.value)
    }

    @Test
    fun `successful retry after a failure clears storageError`() = runTest {
        var now = 0L
        var fail = true
        val migrator = object : LegacyMigrator(
            isUserUnlocked = { true },
            logger = StorageLogger.None,
            clock = { now }
        ) {
            override suspend fun performMigration(): MigrationResult {
                if (fail) throw DirectBootMigrationException("boom")
                return MigrationResult.Migrated(1)
            }
        }
        val repository = newRepository(migrator)

        repository.loadDictionary(DictionaryType.MAIN)
        assertEquals("boom", repository.storageError.value)

        // Within the retry interval the migrator does not run again.
        fail = false
        repository.loadDictionary(DictionaryType.MAIN)
        assertTrue(repository.lastMigrationResult is MigrationResult.RetryLater)
        assertEquals("boom", repository.storageError.value)

        // Once the interval elapsed the retry succeeds and the error is cleared.
        now = LegacyMigrator.DEFAULT_RETRY_INTERVAL_NANOS + 1
        repository.loadDictionary(DictionaryType.MAIN)
        assertEquals(MigrationResult.Migrated(1), repository.lastMigrationResult)
        assertNull(repository.storageError.value)
    }

    // ==========================================================================
    // Concurrency Tests
    // ==========================================================================

    @Test
    fun `concurrent saves are serialized and every entry persists`() = runTest {
        val repository = newRepository()
        repository.loadDictionary(DictionaryType.MAIN)

        val jobs = (1..20).map { i ->
            launch(Dispatchers.Default) {
                val result = repository.saveEntry(
                    DictionaryEntry(id = "id-$i", grapheme = "word$i", phoneme = "p$i")
                )
                assertTrue("save $i failed: ${result.exceptionOrNull()}", result.isSuccess)
            }
        }
        jobs.joinAll()

        assertEquals(20, repository.entries.first().size)
        val loaded = newRepository().loadDictionary(DictionaryType.MAIN).getOrThrow()
        assertEquals((1..20).map { "word$it" }.toSet(), loaded.map { it.grapheme }.toSet())
        assertEquals(listOf("user.json"), dictionaryDir.list()!!.toList())
    }

    // ==========================================================================
    // DictionaryJson Tests
    // ==========================================================================

    @Test
    fun `DictionaryJson round trip preserves Croatian Cyrillic and emoji content`() {
        val entries = listOf(
            DictionaryEntry(
                grapheme = "čćžšđ",
                phoneme = "č ć ž š đ",
                caseSensitive = true,
                wholeWord = false,
                comment = "hrvatski"
            ),
            DictionaryEntry(grapheme = "ћџљњ", phoneme = "ћ џ љ њ"),
            DictionaryEntry(grapheme = "😀", phoneme = "smiling face", comment = "emoji"),
            DictionaryEntry(grapheme = "  spaces  ", phoneme = "spaces"),
            DictionaryEntry(grapheme = "quote\"and\\slash", phoneme = "escaped")
        )

        val parsed = DictionaryJson.parse(DictionaryJson.generate(entries))

        assertEquals(entries.withoutIds(), parsed.withoutIds())
    }

    @Test
    fun `DictionaryJson generate emits version and entries and omits empty comments`() {
        val json = DictionaryJson.generate(
            listOf(
                DictionaryEntry(grapheme = "a", phoneme = "b"),
                DictionaryEntry(grapheme = "c", phoneme = "d", comment = "note")
            )
        )

        val root = JSONObject(json)

        assertEquals(DictionaryJson.VERSION, root.getString("version"))
        assertEquals("1.0", root.getString("version"))
        val entries = root.getJSONArray("entries")
        assertEquals(2, entries.length())
        assertFalse(entries.getJSONObject(0).has("comment"))
        assertEquals("note", entries.getJSONObject(1).getString("comment"))
    }

    @Test
    fun `DictionaryJson generate of an empty list produces an empty entries array`() {
        val root = JSONObject(DictionaryJson.generate(emptyList()))

        assertEquals("1.0", root.getString("version"))
        assertEquals(0, root.getJSONArray("entries").length())
        assertTrue(DictionaryJson.parse(DictionaryJson.generate(emptyList())).isEmpty())
    }

    @Test
    fun `DictionaryJson parse skips entries without a grapheme`() {
        val json = DictionaryJson.generate(
            listOf(
                DictionaryEntry(grapheme = "", phoneme = "ignored"),
                DictionaryEntry(grapheme = "ok", phoneme = "kept")
            )
        )

        val parsed = DictionaryJson.parse(json)

        assertEquals(1, parsed.size)
        assertEquals("ok", parsed[0].grapheme)
        assertEquals("kept", parsed[0].phoneme)
    }

    @Test
    fun `DictionaryJson parse applies defaults for missing fields`() {
        val parsed = DictionaryJson.parse("""{"version":"1.0","entries":[{"grapheme":"a"}]}""")

        assertEquals(1, parsed.size)
        assertEquals("a", parsed[0].grapheme)
        assertEquals("", parsed[0].phoneme)
        assertFalse(parsed[0].caseSensitive)
        assertTrue(parsed[0].wholeWord)
        assertEquals("", parsed[0].comment)
        assertNotNull(parsed[0].id)
    }

    @Test
    fun `DictionaryJson parse tolerates a missing entries array and non-object items`() {
        assertTrue(DictionaryJson.parse("""{"version":"1.0"}""").isEmpty())

        val parsed = DictionaryJson.parse(
            """{"version":"1.0","entries":[1,"text",null,{"grapheme":"a","phoneme":"b"}]}"""
        )

        assertEquals(1, parsed.size)
        assertEquals("a", parsed[0].grapheme)
    }

    @Test
    fun `DictionaryJson parse rejects malformed documents`() {
        assertThrows(JSONException::class.java) { DictionaryJson.parse("not json") }
        assertThrows(JSONException::class.java) { DictionaryJson.parse("") }
    }

    // ==========================================================================
    // List Operation Tests (simulating repository operations)
    // ==========================================================================

    @Test
    fun `adding entry to empty list works`() {
        val entries = mutableListOf<DictionaryEntry>()
        val newEntry = DictionaryEntry(grapheme = "hello", phoneme = "helo")

        entries.add(newEntry)

        assertEquals(1, entries.size)
        assertEquals("hello", entries[0].grapheme)
    }

    @Test
    fun `updating entry in list works`() {
        val entries = mutableListOf(
            DictionaryEntry(id = "1", grapheme = "old", phoneme = "old")
        )

        val index = entries.indexOfFirst { it.id == "1" }
        entries[index] = entries[index].copy(phoneme = "new")

        assertEquals(1, entries.size)
        assertEquals("new", entries[0].phoneme)
    }

    @Test
    fun `deleting entry from list works`() {
        val entries = mutableListOf(
            DictionaryEntry(id = "1", grapheme = "keep", phoneme = "k"),
            DictionaryEntry(id = "2", grapheme = "delete", phoneme = "d")
        )

        val filtered = entries.filterNot { it.id == "2" }

        assertEquals(1, filtered.size)
        assertEquals("keep", filtered[0].grapheme)
    }

    @Test
    fun `finding entry by id works`() {
        val entries = listOf(
            DictionaryEntry(id = "1", grapheme = "first", phoneme = "1"),
            DictionaryEntry(id = "2", grapheme = "second", phoneme = "2"),
            DictionaryEntry(id = "3", grapheme = "third", phoneme = "3")
        )

        val found = entries.find { it.id == "2" }

        assertNotNull(found)
        assertEquals("second", found?.grapheme)
    }

    @Test
    fun `finding non-existent entry returns null`() {
        val entries = listOf(
            DictionaryEntry(id = "1", grapheme = "first", phoneme = "1")
        )

        val found = entries.find { it.id == "999" }

        assertEquals(null, found)
    }

    // ==========================================================================
    // Unicode Support Tests
    // ==========================================================================

    @Test
    fun `DictionaryEntry supports Croatian characters`() {
        val entry = DictionaryEntry(
            grapheme = "čćžšđ",
            phoneme = "čćžšđ"
        )

        assertEquals("čćžšđ", entry.grapheme)
        assertEquals("čćžšđ", entry.phoneme)
    }

    @Test
    fun `DictionaryEntry supports Serbian Cyrillic`() {
        val entry = DictionaryEntry(
            grapheme = "ћџљњ",
            phoneme = "ћџљњ"
        )

        assertEquals("ћџљњ", entry.grapheme)
        assertEquals("ћџљњ", entry.phoneme)
    }

    @Test
    fun `DictionaryEntry supports emoji`() {
        val entry = DictionaryEntry(
            grapheme = "😀",
            phoneme = "smiling face"
        )

        assertEquals("😀", entry.grapheme)
        assertEquals("smiling face", entry.phoneme)
    }

    // ==========================================================================
    // Validation Tests
    // ==========================================================================

    @Test
    fun `empty grapheme is allowed by data class`() {
        val entry = DictionaryEntry(grapheme = "", phoneme = "replacement")
        assertEquals("", entry.grapheme)
    }

    @Test
    fun `empty phoneme is allowed by data class`() {
        val entry = DictionaryEntry(grapheme = "original", phoneme = "")
        assertEquals("", entry.phoneme)
    }

    @Test
    fun `whitespace grapheme is preserved`() {
        val entry = DictionaryEntry(grapheme = "  spaces  ", phoneme = "spaces")
        assertEquals("  spaces  ", entry.grapheme)
    }
}
