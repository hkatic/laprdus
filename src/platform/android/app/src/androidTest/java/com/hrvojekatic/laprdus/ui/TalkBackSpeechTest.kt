package com.hrvojekatic.laprdus.ui

import android.app.UiAutomation
import android.os.SystemClock
import android.view.accessibility.AccessibilityNodeInfo
import androidx.test.core.app.ActivityScenario
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import com.hrvojekatic.laprdus.SettingsActivity
import com.hrvojekatic.laprdus.service.SpeechRequestLog
import com.hrvojekatic.laprdus.R
import com.hrvojekatic.laprdus.data.SettingsRepository
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Assume.assumeTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

/**
 * What TalkBack really says on the Settings screen, through Laprdus.
 *
 * Runs only where TalkBack is on and Laprdus is its engine (the developer's
 * test phone); skipped elsewhere. It changes no setting of TalkBack or of
 * the phone. The transcript goes to logcat (tag TalkBackWalker) and to
 * `Android/data/com.hrvojekatic.laprdus/files/talkback/`.
 */
@RunWith(AndroidJUnit4::class)
class TalkBackSpeechTest {

    private val instrumentation = InstrumentationRegistry.getInstrumentation()
    private val context = instrumentation.targetContext
    private lateinit var automation: UiAutomation
    private lateinit var walker: TalkBackWalker

    @Before
    fun setUp() {
        assumeTrue("TalkBack with Laprdus as its engine is needed", TalkBackWalker.isAvailable(context))
        // Must be the first UiAutomation of the process: without the flag
        // the system switches TalkBack off for the test.
        automation = instrumentation.getUiAutomation(UiAutomation.FLAG_DONT_SUPPRESS_ACCESSIBILITY_SERVICES)
        walker = TalkBackWalker(automation, context.packageName)
        SpeechRequestLog.clear()
    }

    /** Opens Settings and waits until TalkBack speaks through this process again. */
    private fun openSettings(): ActivityScenario<SettingsActivity> {
        val scenario = ActivityScenario.launch(SettingsActivity::class.java)
        // The instrumentation restarted the process, so TalkBack has to
        // reconnect to the engine; its window announcement shows it has.
        // TalkBack does not always reconnect by itself: it starts its engine
        // again when the default engine setting changes, so the setting is
        // switched away and back (it ends as it was).
        if (!awaitAnySpeech(8_000)) {
            shell("settings put secure tts_default_synth com.google.android.tts")
            SystemClock.sleep(1_500)
            shell("settings put secure tts_default_synth ${context.packageName}")
            scenario.recreate()
            awaitAnySpeech(20_000)
        }
        walker.awaitSpeech(0, quietMs = 1_500, maxWaitMs = 6_000)
        return scenario
    }

    private fun awaitAnySpeech(timeoutMs: Long): Boolean {
        val start = SystemClock.elapsedRealtime()
        while (SpeechRequestLog.entries().isEmpty() && SystemClock.elapsedRealtime() - start < timeoutMs) {
            SystemClock.sleep(100)
        }
        return SpeechRequestLog.entries().isNotEmpty()
    }

    private fun shell(command: String) {
        automation.executeShellCommand(command).close()
    }

    /**
     * Every setting is spoken in full, its title before its summary, as they
     * are shown; the slider's name and value both; nothing is silent. Also
     * the settings under "More voice settings" once it is expanded.
     */
    @Test
    fun everySettingIsSpokenInFull() {
        openSettings().use {
            val stops = walker.walk().toMutableList()
            val more = walker.findStopContaining(s(R.string.setting_more_voice))
            assertNotNull("no 'More voice settings' row", more)
            walker.click(more!!)
            stops += walker.walk().map { it.copy(index = it.index + 1000) }
            val transcript = TalkBackWalker.transcript("Settings (checked)", stops)
            TalkBackWalker.save(context, "settings-checked.txt", transcript)

            val silent = stops.filter { it.utterances.isEmpty() }
            assertTrue("silent stops: " + silent.joinToString("; ") { it.node.toString() }, silent.isEmpty())

            // The stop where TalkBack said [text] as a part of its own.
            fun spokenFor(text: String): List<String>? =
                stops.firstOrNull { stop -> stop.utterances.any { it.text.trim() == text } }
                    ?.utterances?.map { it.text.trim() }

            // Switches and rows: the title, then the summary under it.
            for ((title, summary) in listOf(
                R.string.setting_spelling_mode to R.string.setting_spelling_mode_subtitle,
                R.string.setting_force_language_title to R.string.setting_force_language_subtitle,
                R.string.setting_force_speed_title to R.string.setting_force_speed_subtitle,
                R.string.setting_number_mode_title to R.string.setting_number_mode_subtitle,
                R.string.setting_emoji_enabled_title to R.string.setting_emoji_enabled_subtitle,
                R.string.setting_user_dict_enabled_title to R.string.setting_user_dict_enabled_subtitle,
                R.string.setting_volume_boost_title to R.string.setting_volume_boost_subtitle,
                R.string.setting_manage_dictionaries to R.string.setting_manage_dictionaries_desc,
            )) {
                val spoken = spokenFor(s(title))
                assertNotNull("${s(title)} is never spoken", spoken)
                val t = spoken!!.indexOf(s(title))
                val u = spoken.indexOf(s(summary))
                assertTrue("${s(title)}: summary not spoken after the title: $spoken", u > t)
            }
            // Sliders: the name and the value, in one stop.
            for (title in listOf(
                R.string.setting_speech_rate, R.string.setting_speech_pitch, R.string.setting_speech_volume,
                R.string.setting_acceleration, R.string.setting_inflection_level, R.string.setting_head_size,
                R.string.setting_breathiness, R.string.setting_roughness, R.string.setting_brightness,
                R.string.setting_spelling_speed, R.string.setting_sentence_pause,
            )) {
                val stop = stops.firstOrNull { it.node.hasRange && it.node.contentDescription.startsWith(s(title)) }
                assertNotNull("no slider ${s(title)}", stop)
                val spoken = stop!!.utterances.map { it.text }
                assertTrue("${s(title)}: name not spoken: $spoken", spoken.any { it.startsWith(s(title)) })
                assertTrue("${s(title)}: value not spoken: $spoken", spoken.any { it == stop.node.stateDescription })
            }
            // TalkBack starts speaking soon after a focus change.
            val audio = stops.map { it.audioLatencyMs }.sorted()
            assertTrue("median focus-to-audio ${audio[audio.size / 2]} ms", audio[audio.size / 2] < 400)
        }
    }

    private fun s(id: Int): String = context.getString(id)

    private fun savedSpellingMode(): String =
        runBlocking { SettingsRepository.getInstance(context).readSettingsNow().spellingMode }


    /**
     * The letter-sounds switch reads its title, then its summary, and its
     * state follows a toggle. Toggled twice, and put back whatever happens, so
     * the phone's setting ends as it was.
     */
    @Test
    fun spellingSwitchFollowsItsState() {
        val before = savedSpellingMode()
        val other = if (before == SettingsRepository.SPELLING_MODE_SOUNDS) SettingsRepository.SPELLING_MODE_NAMES
        else SettingsRepository.SPELLING_MODE_SOUNDS
        fun awaitSaved(mode: String): Boolean {
            val deadline = SystemClock.elapsedRealtime() + 5_000
            while (savedSpellingMode() != mode && SystemClock.elapsedRealtime() < deadline) SystemClock.sleep(50)
            return savedSpellingMode() == mode
        }
        try {
            openSettings().use {
                val title = s(R.string.setting_spelling_mode)
                val row = walker.findStopContaining(title)
                assertNotNull("no spelling switch", row)
                val spoken = mutableListOf<String>()
                fun listen(label: String): List<String> {
                    // Focus away and back, so TalkBack reads the row afresh.
                    row!!.refresh()
                    row.performAction(AccessibilityNodeInfo.ACTION_CLEAR_ACCESSIBILITY_FOCUS)
                    SystemClock.sleep(300)
                    val said = walker.focus(0, row).utterances.map { it.text.trim() }
                    spoken += "$label: " + said.joinToString(" | ")
                    return said
                }
                fun toggle(label: String) {
                    row!!.refresh()
                    val said = walker.click(row).map { it.text.trim() }
                    spoken += "$label: " + said.joinToString(" | ")
                }

                val first = listen("focus")
                toggle("toggle")
                assertTrue("the switch did not change the setting", awaitSaved(other))
                val toggled = listen("focus after toggle")
                toggle("toggle back")
                assertTrue("the switch did not change the setting back", awaitSaved(before))
                val back = listen("focus after toggling back")
                TalkBackWalker.save(context, "spelling-switch.txt", spoken.joinToString("\n"))

                val report = spoken.joinToString("; ")
                // Each time the title and the summary, in that order; TalkBack's
                // state word (first) changes with the switch and comes back.
                val summary = s(R.string.setting_spelling_mode_subtitle)
                for (said in listOf(first, toggled, back)) {
                    assertTrue(report, said.indexOf(title) in 0 until said.indexOf(summary))
                }
                assertTrue(report, first.first() != toggled.first())
                assertEquals(report, first.first(), back.first())
            }
        } finally {
            if (savedSpellingMode() != before) {
                runBlocking { SettingsRepository.getInstance(context).setSpellingMode(before) }
            }
        }
        assertEquals("the spelling mode must end as it was", before, savedSpellingMode())
    }

    @Test
    fun recordSettingsTranscript() {
        val label = InstrumentationRegistry.getArguments().getString("talkbackLabel") ?: "current"
        openSettings().use {
            val opening = SpeechRequestLog.entries().joinToString(" | ") { it.text }
            val stops = walker.walk()
            val text = "window: \"$opening\"\n\n" + TalkBackWalker.transcript("Settings ($label)", stops)
            TalkBackWalker.save(context, "settings-$label.txt", text)
        }
    }
}
