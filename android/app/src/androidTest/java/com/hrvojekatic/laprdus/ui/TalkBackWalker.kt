package com.hrvojekatic.laprdus.ui

import android.app.UiAutomation
import android.content.Context
import android.os.Build
import android.os.SystemClock
import android.provider.Settings
import android.util.Log
import android.view.accessibility.AccessibilityNodeInfo
import android.view.accessibility.AccessibilityWindowInfo
import com.hrvojekatic.laprdus.service.SpeechRequestLog
import java.io.File

/**
 * Drives the real TalkBack through a screen and records what it says.
 *
 * Every control TalkBack can stop on gets accessibility focus in turn (the
 * same event TalkBack's own swipe produces), and what TalkBack then sends to
 * Laprdus, its default engine, is read back from [SpeechRequestLog], with the
 * time from the focus change to the request and to the first audio. That is
 * exactly what the user hears, in TalkBack's own words for roles and states.
 *
 * Needs TalkBack on and Laprdus as the default engine (see [isAvailable]);
 * the [UiAutomation] must be obtained with
 * [UiAutomation.FLAG_DONT_SUPPRESS_ACCESSIBILITY_SERVICES] or TalkBack is
 * switched off for the test.
 */
class TalkBackWalker(
    private val automation: UiAutomation,
    private val packageName: String,
) {

    /** One stop of the walk: the node and what TalkBack said for it. */
    data class Stop(
        val index: Int,
        val node: NodeSummary,
        val utterances: List<SpeechRequestLog.Entry>,
        val focusedAtNanos: Long,
    ) {
        /** From the focus change to the first request, ms; -1 if TalkBack said nothing. */
        val requestLatencyMs: Long
            get() = utterances.firstOrNull()?.let { (it.receivedAtNanos - focusedAtNanos) / 1_000_000 } ?: -1

        /** From the focus change to the first audio, ms; -1 if TalkBack said nothing. */
        val audioLatencyMs: Long
            get() = utterances.firstOrNull()
                ?.let { (it.receivedAtNanos + it.firstAudioNanos - focusedAtNanos) / 1_000_000 } ?: -1

        /** The first utterance: the node itself; later ones are hints. */
        val spoken: String get() = utterances.firstOrNull()?.text.orEmpty()
    }

    /** What the accessibility tree exposes for a node, kept after the node is gone. */
    data class NodeSummary(
        val className: String,
        val text: String,
        val contentDescription: String,
        val stateDescription: String,
        val heading: Boolean,
        val checkable: Boolean,
        val checked: Boolean,
        val clickable: Boolean,
        val enabled: Boolean,
        val hasRange: Boolean,
        val actions: List<String>,
    ) {
        val key: String get() = "$className|$text|$contentDescription"
        val label: String get() = listOf(contentDescription, text).firstOrNull { it.isNotEmpty() }.orEmpty()
    }

    /**
     * Walks every focusable node of the app's window, scrolling forward when
     * the visible ones are done, up to [maxStops].
     *
     * @param quietMs how long TalkBack must stay silent before the next node
     *   (its usage hints come after a pause, and they belong to this node)
     */
    fun walk(maxStops: Int = 80, quietMs: Long = 1_500, maxWaitMs: Long = 15_000): List<Stop> {
        val stops = mutableListOf<Stop>()
        // A node is the same node while its source id (hashCode/equals of
        // AccessibilityNodeInfo) and its content stay; two sliders with the
        // same text are still two stops.
        val seen = mutableSetOf<String>()
        fun identity(node: AccessibilityNodeInfo) = "${node.hashCode()}|${summarize(node).key}"
        var scrolls = 0
        while (stops.size < maxStops) {
            val root = appRoot() ?: break
            val nodes = focusableNodes(root).filter { identity(it) !in seen }
            if (nodes.isEmpty()) {
                if (scrolls++ > 30 || !scrollForward(root)) break
                SystemClock.sleep(600)
                continue
            }
            val node = nodes.first()
            val summary = summarize(node)
            seen += identity(node)
            stops += focus(stops.size, node, summary, quietMs, maxWaitMs)
        }
        // An entry is recorded when its audio is done: give each stop what
        // TalkBack asked for between its focus and the next one.
        val all = SpeechRequestLog.entries()
        return stops.mapIndexed { i, stop ->
            val until = stops.getOrNull(i + 1)?.focusedAtNanos ?: Long.MAX_VALUE
            stop.copy(utterances = all.filter { it.receivedAtNanos >= stop.focusedAtNanos && it.receivedAtNanos < until })
        }
    }

    /** Puts accessibility focus on [node] and waits for TalkBack to finish speaking. */
    fun focus(
        index: Int,
        node: AccessibilityNodeInfo,
        summary: NodeSummary = summarize(node),
        quietMs: Long = 1_500,
        maxWaitMs: Long = 15_000,
    ): Stop {
        // TalkBack scrolls first and focuses once the scroll has settled.
        if (node.performAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_SHOW_ON_SCREEN.id)) {
            SystemClock.sleep(SCROLL_SETTLE_MS)
            node.refresh()
        }
        val focusedAt = SpeechRequestLog.now()
        node.performAction(AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS)
        val utterances = awaitSpeech(focusedAt, quietMs, maxWaitMs)
        return Stop(index, summary, utterances, focusedAt)
    }

    /** Clicks [node] (TalkBack's double tap) and returns what TalkBack says next. */
    fun click(
        node: AccessibilityNodeInfo,
        quietMs: Long = 1_800,
        maxWaitMs: Long = 8_000,
    ): List<SpeechRequestLog.Entry> {
        val at = SpeechRequestLog.now()
        node.performAction(AccessibilityNodeInfo.ACTION_CLICK)
        return awaitSpeech(at, quietMs, maxWaitMs)
    }

    /**
     * Waits until TalkBack has said something and stayed quiet for [quietMs]
     * after the end of what it said. TalkBack sends each part of an
     * announcement once the part before it has been heard, so quiet is
     * measured from the end of the last part's audio, not from its request.
     */
    fun awaitSpeech(sinceNanos: Long, quietMs: Long, maxWaitMs: Long): List<SpeechRequestLog.Entry> {
        val deadline = SystemClock.elapsedRealtime() + maxWaitMs
        while (SystemClock.elapsedRealtime() < deadline) {
            val entries = SpeechRequestLog.entriesSince(sinceNanos)
            val last = entries.lastOrNull()
            val busy = SpeechRequestLog.inFlightSinceNanos != 0L
            if (!busy && last != null && SpeechRequestLog.now() - last.endNanos >= quietMs * 1_000_000) {
                return entries
            }
            SystemClock.sleep(50)
        }
        return SpeechRequestLog.entriesSince(sinceNanos)
    }

    /** The root of the app's own window, not TalkBack's overlays or the system bars. */
    fun appRoot(): AccessibilityNodeInfo? {
        automation.rootInActiveWindow?.takeIf { it.packageName == packageName }?.let { return it }
        return automation.windows
            .filter { it.type == AccessibilityWindowInfo.TYPE_APPLICATION }
            .mapNotNull { it.root }
            .firstOrNull { it.packageName == packageName }
    }

    /** Scrolls the screen back to its top (Compose reports only the nodes on screen). */
    fun scrollToTop() {
        repeat(30) {
            val root = appRoot() ?: return
            if (!scroll(root, AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD)) return
            SystemClock.sleep(400)
        }
    }

    /**
     * The stop (a node TalkBack stops on) that shows [text] itself or in its
     * children, scrolling down from the top until it is on screen (Compose
     * reports only the nodes on screen).
     */
    fun findStopContaining(text: String): AccessibilityNodeInfo? {
        scrollToTop()
        fun contains(node: AccessibilityNodeInfo): Boolean {
            if (node.text?.toString() == text || node.contentDescription?.toString() == text) return true
            for (i in 0 until node.childCount) node.getChild(i)?.let { if (contains(it)) return true }
            return false
        }
        repeat(30) {
            val root = appRoot() ?: return null
            focusableNodes(root).lastOrNull { contains(it) }?.let { return it }
            if (!scrollForward(root)) return null
            SystemClock.sleep(400)
        }
        return null
    }

    /** Finds the first node whose label (content description or text) starts with [label]. */
    fun findByLabel(label: String): AccessibilityNodeInfo? {
        val root = appRoot() ?: return null
        return focusableNodes(root).firstOrNull { summarize(it).label.startsWith(label) }
    }

    /** The nodes TalkBack stops on, in tree (traversal) order. */
    fun focusableNodes(root: AccessibilityNodeInfo): List<AccessibilityNodeInfo> {
        val out = mutableListOf<AccessibilityNodeInfo>()
        fun visit(node: AccessibilityNodeInfo, coveredByFocusable: Boolean) {
            if (!node.isVisibleToUser) return
            val hasContent = !node.text.isNullOrEmpty() || !node.contentDescription.isNullOrEmpty()
            // What TalkBack stops on: a node marked focusable for screen
            // readers (Compose marks every merging node so) or one that can be
            // acted on, whether its words are its own or its children's; a
            // heading; and text that no such node above it speaks.
            val actionable = node.isScreenReaderFocusable || node.isClickable || node.isCheckable ||
                node.isLongClickable || node.rangeInfo != null
            val stop = actionable ||
                (node.isHeading && hasContent) ||
                (!coveredByFocusable && hasContent && node.childCount == 0)
            if (stop) out += node
            for (i in 0 until node.childCount) {
                node.getChild(i)?.let { visit(it, coveredByFocusable || stop) }
            }
        }
        visit(root, false)
        return out
    }

    private fun scrollForward(root: AccessibilityNodeInfo): Boolean =
        scroll(root, AccessibilityNodeInfo.ACTION_SCROLL_FORWARD)

    private fun scroll(root: AccessibilityNodeInfo, action: Int): Boolean {
        fun find(node: AccessibilityNodeInfo): AccessibilityNodeInfo? {
            if (node.isScrollable && node.actionList.any { it.id == action }) return node
            for (i in 0 until node.childCount) node.getChild(i)?.let { find(it) }?.let { return it }
            return null
        }
        return find(root)?.performAction(action) ?: false
    }

    companion object {
        private const val TAG = "TalkBackWalker"
        private const val SCROLL_SETTLE_MS = 700L

        /** Whether TalkBack is running and speaks through Laprdus. */
        fun isAvailable(context: Context): Boolean {
            val resolver = context.contentResolver
            val services = Settings.Secure.getString(
                resolver, Settings.Secure.ENABLED_ACCESSIBILITY_SERVICES
            ).orEmpty()
            val engine = Settings.Secure.getString(resolver, "tts_default_synth").orEmpty()
            return services.contains("talkback", ignoreCase = true) && engine == context.packageName
        }

        fun summarize(node: AccessibilityNodeInfo): NodeSummary = NodeSummary(
            className = node.className?.toString().orEmpty(),
            text = node.text?.toString().orEmpty(),
            contentDescription = node.contentDescription?.toString().orEmpty(),
            stateDescription = node.stateDescription?.toString().orEmpty(),
            heading = node.isHeading,
            checkable = node.isCheckable,
            checked = if (Build.VERSION.SDK_INT >= 36) {
                node.checked == AccessibilityNodeInfo.CHECKED_STATE_TRUE
            } else {
                @Suppress("DEPRECATION") node.isChecked
            },
            clickable = node.isClickable,
            enabled = node.isEnabled,
            hasRange = node.rangeInfo != null,
            actions = node.actionList.map { it.label?.toString() ?: actionName(it.id) },
        )

        private fun actionName(id: Int): String = when (id) {
            AccessibilityNodeInfo.ACTION_CLICK -> "click"
            AccessibilityNodeInfo.ACTION_LONG_CLICK -> "long click"
            AccessibilityNodeInfo.ACTION_SCROLL_FORWARD -> "scroll forward"
            AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD -> "scroll backward"
            AccessibilityNodeInfo.ACTION_EXPAND -> "expand"
            AccessibilityNodeInfo.ACTION_COLLAPSE -> "collapse"
            AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS -> "a11y focus"
            AccessibilityNodeInfo.ACTION_CLEAR_ACCESSIBILITY_FOCUS -> "clear a11y focus"
            AccessibilityNodeInfo.ACTION_FOCUS -> "focus"
            AccessibilityNodeInfo.ACTION_CLEAR_FOCUS -> "clear focus"
            AccessibilityNodeInfo.ACTION_SELECT -> "select"
            AccessibilityNodeInfo.ACTION_SET_SELECTION -> "set selection"
            android.R.id.accessibilityActionSetProgress -> "set progress"
            android.R.id.accessibilityActionShowOnScreen -> "show on screen"
            android.R.id.accessibilityActionScrollUp -> "scroll up"
            android.R.id.accessibilityActionScrollDown -> "scroll down"
            else -> "action $id"
        }

        /** A readable transcript: one block per stop, then the latency summary. */
        fun transcript(title: String, stops: List<Stop>): String = buildString {
            appendLine("# $title")
            appendLine()
            for (stop in stops) {
                val n = stop.node
                appendLine(
                    "${stop.index + 1}. [${n.className.substringAfterLast('.')}" +
                        (if (n.heading) ", heading" else "") +
                        (if (n.checkable) ", checked=${n.checked}" else "") +
                        (if (n.hasRange) ", range" else "") +
                        (if (!n.enabled) ", disabled" else "") +
                        "] cd=\"${n.contentDescription}\" text=\"${n.text}\" state=\"${n.stateDescription}\"" +
                        " actions=${n.actions}"
                )
                if (stop.utterances.isEmpty()) {
                    appendLine("   (silent)")
                }
                stop.utterances.forEachIndexed { i, u ->
                    val at = (u.receivedAtNanos - stop.focusedAtNanos) / 1_000_000
                    appendLine(
                        "   ${if (i == 0) "says" else "then"} +$at ms " +
                            "(audio +${at + u.firstAudioNanos / 1_000_000} ms, " +
                            "synth ${u.synthesisNanos / 1_000_000} ms" +
                            "${if (u.interrupted) ", interrupted" else ""}): \"${u.text}\""
                    )
                }
            }
            append(summary(stops))
        }

        /** Medians and maxima of the latencies. */
        fun summary(stops: List<Stop>): String = buildString {
            val spoken = stops.filter { it.utterances.isNotEmpty() }
            appendLine()
            appendLine("stops: ${stops.size}, spoken: ${spoken.size}")
            if (spoken.isEmpty()) return@buildString
            val req = spoken.map { it.requestLatencyMs }.sorted()
            val audio = spoken.map { it.audioLatencyMs }.sorted()
            val synth = spoken.map { it.utterances.first().synthesisNanos / 1_000_000 }.sorted()
            val chars = spoken.map { it.spoken.length }.sorted()
            appendLine("focus -> request ms: median ${req[req.size / 2]}, max ${req.last()}")
            appendLine("focus -> audio ms: median ${audio[audio.size / 2]}, max ${audio.last()}")
            appendLine("synthesis ms: median ${synth[synth.size / 2]}, max ${synth.last()}")
            appendLine("utterance chars: median ${chars[chars.size / 2]}, max ${chars.last()}")
        }

        /** Writes [text] to the app's external files and to logcat (tag TalkBackWalker). */
        fun save(context: Context, name: String, text: String): File {
            val dir = File(context.getExternalFilesDir(null), "talkback").apply { mkdirs() }
            val file = File(dir, name)
            file.writeText(text)
            text.lines().forEach { Log.i(TAG, it) }
            return file
        }
    }
}
