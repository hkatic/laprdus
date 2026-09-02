package com.hrvojekatic.laprdus.data

import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.util.UUID

/**
 * Write-temp-then-rename helpers so readers never observe a partially written
 * file. The temp name is unique per call, so concurrent writers cannot
 * interleave into one temp file.
 *
 * Limitation: the JVM cannot fsync a directory, so the rename itself is not
 * guaranteed durable across a power loss. What is guaranteed: the target is
 * either the previous complete content or the new complete content.
 *
 * Contains no android.* references so it can run in plain JVM tests.
 */
object AtomicFiles {
    const val TEMP_SUFFIX = ".tmp"
    private const val RENAME_ATTEMPTS = 10
    private const val RENAME_RETRY_DELAY_MS = 2L

    /**
     * Writes [bytes] to [target] atomically.
     *
     * @param tempTag optional tag embedded in the temp file name
     *   (`<name>.<tag>-<uuid>.tmp`) so a writer can later clean up only its own
     *   leftovers with [deleteStaleTempFiles] without touching another
     *   writer's in-flight temp files.
     * @param beforeRename hook invoked after the temp file is written and
     *   synced but before it is renamed over [target]; used by fault injection.
     *   If it throws, the temp file is intentionally left behind, exactly as a
     *   real crash at that point would leave it.
     */
    @Throws(IOException::class)
    fun writeBytesAtomically(
        target: File,
        bytes: ByteArray,
        tempTag: String? = null,
        beforeRename: () -> Unit = {},
    ) {
        val dir = target.absoluteFile.parentFile ?: throw IOException("No parent directory for $target")
        if (!dir.isDirectory && !dir.mkdirs() && !dir.isDirectory) {
            throw IOException("Cannot create directory $dir")
        }
        val tmp = File(dir, "${tempPrefix(target.name, tempTag)}${UUID.randomUUID()}$TEMP_SUFFIX")
        try {
            FileOutputStream(tmp).use { out ->
                out.write(bytes)
                out.flush()
                out.fd.sync()
            }
        } catch (e: IOException) {
            tmp.delete()
            throw e
        }
        beforeRename()
        if (!renameWithRetry(tmp, target)) {
            tmp.delete()
            throw IOException("Cannot rename $tmp to $target")
        }
    }

    /**
     * rename(2) is atomic and reliable on Android/Linux, but some desktop
     * filesystems (macOS, where the JVM tests run) occasionally fail a rename
     * that races with another rename onto the same target. A few short retries
     * make the helper behave the same everywhere without needing java.nio.file
     * (API 26+).
     */
    private fun renameWithRetry(tmp: File, target: File): Boolean {
        repeat(RENAME_ATTEMPTS) { attempt ->
            if (tmp.renameTo(target)) return true
            if (attempt < RENAME_ATTEMPTS - 1) {
                try {
                    Thread.sleep(RENAME_RETRY_DELAY_MS)
                } catch (_: InterruptedException) {
                    Thread.currentThread().interrupt()
                    return false
                }
            }
        }
        return false
    }

    @Throws(IOException::class)
    fun writeTextAtomically(
        target: File,
        text: String,
        tempTag: String? = null,
        beforeRename: () -> Unit = {},
    ) {
        writeBytesAtomically(target, text.toByteArray(Charsets.UTF_8), tempTag, beforeRename)
    }

    /**
     * True for temp files produced by [writeBytesAtomically] for [targetName].
     * With a [tempTag] only temp files written with that tag match; without
     * one, every temp file for the target matches.
     */
    fun isTempFileFor(file: File, targetName: String, tempTag: String? = null): Boolean =
        file.name.startsWith(tempPrefix(targetName, tempTag)) && file.name.endsWith(TEMP_SUFFIX)

    /**
     * Deletes leftover temp files (from a crash mid-write) in [dir], optionally
     * only those written with [tempTag]. Returns the number deleted.
     */
    fun deleteStaleTempFiles(dir: File, targetNames: Collection<String>, tempTag: String? = null): Int {
        val files = dir.listFiles() ?: return 0
        var deleted = 0
        for (file in files) {
            if (targetNames.any { isTempFileFor(file, it, tempTag) } && file.delete()) deleted++
        }
        return deleted
    }

    private fun tempPrefix(targetName: String, tempTag: String?): String =
        if (tempTag.isNullOrEmpty()) "$targetName." else "$targetName.$tempTag-"
}
