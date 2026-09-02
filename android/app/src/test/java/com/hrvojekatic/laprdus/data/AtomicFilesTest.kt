package com.hrvojekatic.laprdus.data

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import kotlinx.coroutines.awaitAll
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.test.runTest
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertThrows
import org.junit.Assert.assertTrue
import org.junit.Assume.assumeFalse
import org.junit.Assume.assumeTrue
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File
import java.io.IOException
import java.util.UUID

/**
 * Unit tests for [AtomicFiles]: write-temp-then-rename semantics used by the
 * dictionary repository and the dictionary migration.
 *
 * The JVM cannot fsync a directory, so these tests assert the documented
 * guarantee only: a reader never sees a partial file and a completed write
 * never leaves a temp file behind. Power-loss durability is out of scope.
 */
class AtomicFilesTest {

    @get:Rule
    val tempFolder = TemporaryFolder()

    private lateinit var dir: File

    @Before
    fun setup() {
        dir = tempFolder.newFolder("dictionaries")
    }

    // ==========================================================================
    // Writing
    // ==========================================================================

    @Test
    fun `writeTextAtomically creates the target with exact content and no temp file`() {
        val target = File(dir, "user.json")
        val text = "{\"grapheme\":\"Čćžšđ\",\"phoneme\":\"😀 nasmijano lice\"}\n"

        AtomicFiles.writeTextAtomically(target, text)

        assertEquals(text, target.readText(Charsets.UTF_8))
        assertArrayEquals(text.toByteArray(Charsets.UTF_8), target.readBytes())
        assertEquals(listOf("user.json"), namesIn(dir))
    }

    @Test
    fun `writeBytesAtomically preserves every byte value`() {
        val target = File(dir, "blob.bin")
        val bytes = ByteArray(256 * 4) { (it % 256).toByte() }

        AtomicFiles.writeBytesAtomically(target, bytes)

        assertArrayEquals(bytes, target.readBytes())
        assertEquals(listOf("blob.bin"), namesIn(dir))
    }

    @Test
    fun `writeTextAtomically replaces existing content atomically`() {
        val target = File(dir, "user.json")
        AtomicFiles.writeTextAtomically(target, "old content")

        AtomicFiles.writeTextAtomically(target, "new content") {
            // Until the rename, readers still see the complete old file.
            assertEquals("old content", target.readText())
            assertEquals(1, tempFilesIn(dir).size)
        }

        assertEquals("new content", target.readText())
        assertEquals(listOf("user.json"), namesIn(dir))
    }

    @Test
    fun `temp file is named after the target and is unique per write`() {
        val target = File(dir, "user.json")
        val tempNames = mutableListOf<String>()

        repeat(3) { i ->
            AtomicFiles.writeTextAtomically(target, "content $i") {
                val temps = tempFilesIn(dir)
                assertEquals(1, temps.size)
                assertTrue(AtomicFiles.isTempFileFor(temps[0], "user.json"))
                assertEquals("content $i", temps[0].readText())
                tempNames += temps[0].name
            }
        }

        assertEquals(3, tempNames.distinct().size)
        assertEquals("content 2", target.readText())
        assertEquals(listOf("user.json"), namesIn(dir))
    }

    @Test
    fun `writing into a missing nested directory creates it`() {
        val target = File(dir, "a/b/c/user.json")

        AtomicFiles.writeTextAtomically(target, "nested")

        assertTrue(target.isFile)
        assertEquals("nested", target.readText())
        assertEquals(listOf("user.json"), namesIn(target.parentFile))
    }

    // ==========================================================================
    // Failure Before Rename
    // ==========================================================================

    @Test
    fun `throwing beforeRename hook leaves the temp file behind and the old target untouched`() {
        val target = File(dir, "user.json")
        target.writeText("old content")

        val error = assertThrows(IllegalStateException::class.java) {
            AtomicFiles.writeTextAtomically(target, "new content") {
                throw IllegalStateException("simulated crash")
            }
        }

        assertEquals("simulated crash", error.message)
        assertEquals("old content", target.readText())
        val temps = tempFilesIn(dir)
        assertEquals("temp file is intentionally left behind", 1, temps.size)
        assertTrue(AtomicFiles.isTempFileFor(temps[0], "user.json"))
        assertEquals("new content", temps[0].readText())
    }

    @Test
    fun `throwing beforeRename hook on a new target creates no target`() {
        val target = File(dir, "user.json")

        assertThrows(IllegalStateException::class.java) {
            AtomicFiles.writeTextAtomically(target, "new content") {
                throw IllegalStateException("simulated crash")
            }
        }

        assertFalse(target.exists())
        val temps = tempFilesIn(dir)
        assertEquals(1, temps.size)
        assertTrue(AtomicFiles.isTempFileFor(temps[0], "user.json"))
        assertEquals(listOf(temps[0].name), namesIn(dir))
    }

    // ==========================================================================
    // Unwritable Destinations
    // ==========================================================================

    @Test
    fun `parent path that is a regular file throws IOException`() {
        val blocker = File(dir, "blocker")
        blocker.writeText("I am a file, not a directory")
        val target = File(blocker, "user.json")

        assertThrows(IOException::class.java) {
            AtomicFiles.writeTextAtomically(target, "x")
        }

        assertEquals("I am a file, not a directory", blocker.readText())
        assertEquals(listOf("blocker"), namesIn(dir))
    }

    @Test
    fun `unwritable parent directory throws IOException and leaves nothing behind`() {
        assumeTrue("directory permissions are not enforced here", dir.setWritable(false, false))
        assumeFalse("running as root; directory permissions are ignored", dir.canWrite())

        try {
            assertThrows(IOException::class.java) {
                AtomicFiles.writeTextAtomically(File(dir, "user.json"), "x")
            }
            assertEquals(emptyList<String>(), namesIn(dir))
        } finally {
            dir.setWritable(true)
        }
    }

    // ==========================================================================
    // Temp File Detection And Cleanup
    // ==========================================================================

    @Test
    fun `isTempFileFor matches only temp files of the given target name`() {
        val uuid = UUID.randomUUID()

        assertTrue(AtomicFiles.isTempFileFor(File(dir, "user.json.$uuid.tmp"), "user.json"))
        assertFalse(AtomicFiles.isTempFileFor(File(dir, "user.json"), "user.json"))
        assertFalse(AtomicFiles.isTempFileFor(File(dir, "other.json.x.tmp"), "user.json"))
        assertFalse(AtomicFiles.isTempFileFor(File(dir, "user.jsonx.$uuid.tmp"), "user.json"))
        assertFalse(AtomicFiles.isTempFileFor(File(dir, "user.json.$uuid.bak"), "user.json"))
        assertFalse(AtomicFiles.isTempFileFor(File(dir, "unrelated.tmp"), "user.json"))
    }

    @Test
    fun `deleteStaleTempFiles removes only temps for the given names and returns the count`() {
        val userTemp1 = File(dir, "user.json.${UUID.randomUUID()}.tmp").apply { writeText("1") }
        val userTemp2 = File(dir, "user.json.${UUID.randomUUID()}.tmp").apply { writeText("2") }
        val spellingTemp = File(dir, "spelling.json.${UUID.randomUUID()}.tmp").apply { writeText("3") }
        val emojiTemp = File(dir, "emoji.json.${UUID.randomUUID()}.tmp").apply { writeText("4") }
        val otherTemp = File(dir, "other.json.${UUID.randomUUID()}.tmp").apply { writeText("5") }
        val unrelated = File(dir, "unrelated.tmp").apply { writeText("6") }
        val real = File(dir, "user.json").apply { writeText("real") }

        val deleted = AtomicFiles.deleteStaleTempFiles(dir, listOf("user.json", "spelling.json"))

        assertEquals(3, deleted)
        assertFalse(userTemp1.exists())
        assertFalse(userTemp2.exists())
        assertFalse(spellingTemp.exists())
        assertTrue("name not in the list must stay", emojiTemp.exists())
        assertTrue(otherTemp.exists())
        assertTrue(unrelated.exists())
        assertEquals("real", real.readText())
    }

    @Test
    fun `deleteStaleTempFiles returns zero for a missing directory or no matches`() {
        assertEquals(0, AtomicFiles.deleteStaleTempFiles(File(dir, "missing"), listOf("user.json")))

        File(dir, "user.json").writeText("real")
        assertEquals(0, AtomicFiles.deleteStaleTempFiles(dir, listOf("user.json")))
        assertEquals(listOf("user.json"), namesIn(dir))
    }

    // ==========================================================================
    // Concurrency
    // ==========================================================================

    @Test
    fun `concurrent writers never produce a torn file`() = runTest {
        val target = File(dir, "user.json")
        val payloads = List(16) { writer ->
            buildString {
                repeat(2000) { line ->
                    append("writer ").append(writer).append(" line ").append(line).append(" čćžšđ\n")
                }
            }
        }

        coroutineScope {
            payloads.map { payload ->
                async(Dispatchers.Default) { AtomicFiles.writeTextAtomically(target, payload) }
            }.awaitAll()
        }

        val content = target.readText(Charsets.UTF_8)
        assertTrue("target must hold exactly one writer's complete payload", payloads.contains(content))
        assertEquals(listOf("user.json"), namesIn(dir))
    }

    // ==========================================================================
    // Tagged temp files
    // ==========================================================================

    @Test
    fun `tagged temp files are recognised by tag and untagged cleanup sees them too`() {
        val target = File(dir, "user.json")
        val leftover = mutableListOf<File>()
        try {
            AtomicFiles.writeTextAtomically(target, "x", tempTag = "migration") {
                leftover.addAll(tempFilesIn(dir))
                throw IllegalStateException("crash before rename")
            }
        } catch (_: IllegalStateException) {
        }
        assertEquals(1, leftover.size)
        val temp = leftover[0]
        assertTrue(temp.name.startsWith("user.json.migration-"))
        assertTrue(AtomicFiles.isTempFileFor(temp, "user.json", "migration"))
        assertTrue(AtomicFiles.isTempFileFor(temp, "user.json"))
        assertFalse(AtomicFiles.isTempFileFor(temp, "user.json", "other"))

        val untagged = File(dir, "user.json.${UUID.randomUUID()}.tmp").apply { writeText("repo") }
        assertFalse(AtomicFiles.isTempFileFor(untagged, "user.json", "migration"))

        assertEquals(1, AtomicFiles.deleteStaleTempFiles(dir, listOf("user.json"), "migration"))
        assertFalse(temp.exists())
        assertTrue("untagged temp must survive a tagged cleanup", untagged.exists())
        assertEquals(1, AtomicFiles.deleteStaleTempFiles(dir, listOf("user.json")))
        assertFalse(untagged.exists())
    }

    // ==========================================================================
    // Helpers
    // ==========================================================================

    private fun namesIn(dir: File): List<String> = dir.listFiles().orEmpty().map { it.name }.sorted()

    private fun tempFilesIn(dir: File): List<File> =
        dir.listFiles().orEmpty().filter { it.name.endsWith(AtomicFiles.TEMP_SUFFIX) }
}
