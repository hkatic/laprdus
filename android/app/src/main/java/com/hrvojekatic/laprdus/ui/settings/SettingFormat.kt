package com.hrvojekatic.laprdus.ui.settings

import java.text.NumberFormat
import java.util.Locale
import kotlin.math.abs
import kotlin.math.roundToInt

/**
 * Steps and number formats of the settings sliders. Plain Kotlin, so the JVM
 * tests check them.
 *
 * Every slider moves in steps of a size that suits its setting (0.05x for
 * the rate, 5 % for a percentage, 50 ms for a pause). One TalkBack swipe or
 * volume key press moves one step: without steps Compose moves a slider by
 * a twentieth of its range, which gave rates such as 1.19x.
 */
object SettingFormat {

    /** The step of the rate and pitch sliders. */
    const val MULTIPLIER_STEP = 0.05f

    /** The step of the acceleration slider. */
    const val ACCELERATION_STEP = 0.1f

    /** The step of a slider over 0.0-1.0 shown in percent (volume, inflection). */
    const val FRACTION_STEP = 0.05f

    /** The step of a 0-100 slider (voice quality, spelling speed). */
    const val PERCENT_STEP = 5f

    /** The step of a pause slider, ms. */
    const val PAUSE_STEP = 50f

    /**
     * The number of positions between the ends of [range] for [step], as
     * Compose's `Slider(steps = ...)` counts them (the ends excluded).
     */
    fun steps(range: ClosedFloatingPointRange<Float>, step: Float): Int {
        val intervals = ((range.endInclusive - range.start) / step).roundToInt()
        return (intervals - 1).coerceAtLeast(0)
    }

    /**
     * [value] moved to the nearest step of [range], with the float error of
     * the step arithmetic rounded away (1.05, not 1.0500001).
     */
    fun snap(value: Float, range: ClosedFloatingPointRange<Float>, step: Float): Float {
        val clamped = value.coerceIn(range)
        val index = ((clamped - range.start) / step).roundToInt()
        val snapped = range.start + index * step
        return ((snapped * 10_000f).roundToInt() / 10_000f).coerceIn(range)
    }

    /** Whether two slider values are the same step. */
    fun sameStep(a: Float, b: Float, step: Float): Boolean = abs(a - b) < step / 2

    /**
     * A rate or pitch multiplier in the language of the screen: "1,0x" in
     * Croatian, Serbian and Slovenian, "1.0x" in English, with a second
     * decimal only where the value has one ("1,05x").
     */
    fun multiplier(value: Float, locale: Locale): String {
        val hundredths = (value * 100).roundToInt()
        val decimals = if (hundredths % 10 == 0) 1 else 2
        val format = NumberFormat.getNumberInstance(locale).apply {
            minimumFractionDigits = decimals
            maximumFractionDigits = decimals
            isGroupingUsed = false
        }
        return format.format(hundredths / 100.0) + "x"
    }

    /** A 0.0-1.0 value as a whole percentage (0.5 → 50). */
    fun fractionToPercent(value: Float): Int = (value * 100).roundToInt()

    /**
     * Words per minute at the top of an app's rate range: the app's 2.0x
     * scales the Laprdus rate (the Laprdus rate alone when it is forced),
     * times the acceleration, up to the fastest rate the voice renders.
     */
    fun topWordsPerMinute(
        nominalWpm: Float,
        speed: Float,
        acceleration: Float,
        forceSpeed: Boolean,
        speedRange: ClosedFloatingPointRange<Float>,
        formantVoice: Boolean,
    ): Int {
        val rate = if (forceSpeed) speed else (2.0f * speed).coerceIn(speedRange)
        val fastest = if (formantVoice) 8.0f else 4.0f
        return (nominalWpm * (rate * acceleration).coerceAtMost(fastest)).roundToInt()
    }
}
