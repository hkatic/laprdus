// volume_boost.hpp - Louder output for both kinds of voices.
//
// The speech of the formant voices peaks close to full scale while its
// median level sits near -20 dBFS (a crest of 20 dB: vowel peaks 12 dB above
// the vowel's own level, and 8 dB between the loud and the quiet syllables),
// and the recorded voices already touch the ceiling, so a plain gain cannot
// make either louder without clipping. The volume boost is a gain of
// VOLUME_BOOST_GAIN (+14 dB) followed by a peak limiter whose gain moves
// only between syllables, never inside a glottal cycle:
//
//  1. every sample asks for the gain that would put it at VOLUME_BOOST_CEILING
//     (1 when it is already below);
//  2. the envelope is the minimum of those over a window that reaches
//     LOOKAHEAD_SECONDS ahead and HOLD_SECONDS back, so the gain is down
//     before a peak arrives and stays down for longer than a pitch period
//     after it (20 ms covers every period down to 50 Hz);
//  3. the attack is a box average of the envelope over the lookahead: a
//     linear ramp down to the peak, and every term of the average already
//     includes the peak's own wanted gain, so no peak passes the ceiling;
//  4. the release follows upward moves with a one-pole of RELEASE_SECONDS;
//     downward moves are taken at once;
//  5. a tanh soft clipper above VOLUME_BOOST_KNEE stays as a safety net; with
//     the ceiling under the knee it is idle;
//  6. the gain may rise at most VOLUME_BOOST_MAX_LIFT_DB above the smallest
//     gain of the last LIFT_WINDOW_SECONDS (the loudest recent syllable), so
//     the quiet consonants between loud vowels keep their distance from
//     them: without the cap the limiter lifted the friction frames of a
//     paragraph 3 dB closer to the voiced ones (Zvonko -4.6 to -1.6 dB,
//     Josip -1.9 to -1.1) and the sibilants bit on a phone at full volume;
//  7. before all that, in boost mode only, a second-order high shelf takes
//     the treble down by VOLUME_BOOST_TREBLE_DB from VOLUME_BOOST_TREBLE_HZ
//     up (half of it at that frequency, nothing an octave below), where a
//     phone's speaker has its own peak (the listener's choice). Holding the
//     vowels down and lifting the rest tilts the long-term spectrum of the
//     speech up by 5 dB at 4-5 kHz; the cap takes 1 dB of that back and
//     the shelf about 1.5 dB (the limiter lifts what the shelf lowers).
//
// Until November 2026 the boost was a 1 ms attack, a 30 ms release with no
// hold, a ceiling of 1.4 above full scale and a soft clipper from 0.6 up:
// every vowel peak was driven up to 7 dB into the tanh and the gain recovered
// inside a glottal cycle, which users of phones heard as distortion
// (measured on "Pozivam Marka Marića na mobitel" and a 37 s paragraph with
// tools/formant/spectrum.py --boost: the waveform of the loudest frames
// departed from a scaled copy of the plain one by -18 dB, 10 % distortion).
// The limiter alone keeps that residual under -50 dB, with 7-11 kHz products
// no higher than the plain speech has them. The price is loudness: Zvonko's
// median speech level rises from -20.6 to -11.0 dBFS (+10.5 dB, before
// +13.4) and his loud stretches from -15.5 to -7.9 dBFS (+2.8 dB, before
// +5.9). Letting the clipper work again buys little: a ceiling of 1.1 with
// the knee at 0.8 adds 1 dB to both at a residual of -33 dB, 1.25 with 0.7
// adds 2 dB at -24 dB; see "Hiss, crackle and the boost" in docs/formant.md.
// Header-only so that every build (SCons, CMake, Xcode) compiles it without
// a new source file.
//
// The boost sits before the volume (host or user) is applied, so a host at
// half volume with the boost on is still louder than without it. The
// renderers apply no soft knee of their own after it (the ceiling is under
// full scale), only the volume and a hard clamp.

#ifndef LAPRDUS_VOLUME_BOOST_HPP
#define LAPRDUS_VOLUME_BOOST_HPP

#include <laprdus/types.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <deque>
#include <vector>

namespace laprdus {
namespace boost_detail {

constexpr float LOOKAHEAD_SECONDS = 0.005f;  // the gain ramps down over this time before a peak
constexpr float HOLD_SECONDS = 0.020f;       // and stays down at least this long after it
constexpr float RELEASE_SECONDS = 0.050f;    // then recovers with this time constant
constexpr float LIFT_WINDOW_SECONDS = 0.6f;  // the loudest syllable this far back caps the lift

// The hiss limiter (VOLUME_BOOST_HISS_*): the band powers are followed over
// these times, the vowel reference is the loudest low band this far either
// side of a sample, and the cut acts only while the hiss band stands at
// least HISS_GATE_LOW_DB re the low band of the same moment (fully from
// HISS_GATE_HIGH_DB), which no vowel reaches (their band above 3 kHz lies
// 15-25 dB under the rest) and every sibilant passes.
constexpr float HISS_TAU_SECONDS = 0.002f;
constexpr float HISS_LOW_TAU_SECONDS = 0.005f;
constexpr float HISS_VOWEL_WINDOW_SECONDS = 0.25f;
constexpr float HISS_VOWEL_AVERAGE_SECONDS = 0.020f;  // two glottal periods: the vowel, not its pulses
constexpr float HISS_GATE_LOW_DB = -12.0f;
constexpr float HISS_GATE_HIGH_DB = -3.0f;
constexpr float HISS_LOOKAHEAD_SECONDS = 0.002f;
constexpr float HISS_HOLD_SECONDS = 0.005f;
constexpr float HISS_RELEASE_SECONDS = 0.015f;

// A second-order section of the Audio EQ Cookbook (Q = 1/sqrt 2: Butterworth).
struct Section {
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
    double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;
    static Section butterworth(double freq, double fs, bool highpass) {
        const double w0 = 2.0 * 3.14159265358979 * freq / fs;
        const double cw = std::cos(w0);
        const double alpha = std::sin(w0) / 2.0 * std::sqrt(2.0);
        const double a0 = 1.0 + alpha;
        Section s;
        if (highpass) {
            s.b0 = (1.0 + cw) / 2.0 / a0;
            s.b1 = -(1.0 + cw) / a0;
        } else {
            s.b0 = (1.0 - cw) / 2.0 / a0;
            s.b1 = (1.0 - cw) / a0;
        }
        s.b2 = s.b0;
        s.a1 = -2.0 * cw / a0;
        s.a2 = (1.0 - alpha) / a0;
        return s;
    }
    double tick(double x) {
        const double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = x; y2 = y1; y1 = y;
        return y;
    }
};

// Hold the hiss band of `wave` under `hiss_db` re the loudest nearby vowel
// while it dominates the sound (see VOLUME_BOOST_HISS_DB). The band is the
// signal minus its low-passed part, so at unity gain the output is the input.
inline void limit_hiss(std::vector<float>& wave, float fs, float hiss_db) {
    const size_t n = wave.size();
    if (n == 0) return;
    Section split1 = Section::butterworth(VOLUME_BOOST_HISS_SPLIT_HZ, fs, false);
    Section split2 = split1;
    Section det_lp = Section::butterworth(VOLUME_BOOST_HISS_HZ, fs, false);
    Section det_hp = Section::butterworth(VOLUME_BOOST_HISS_HZ, fs, true);
    std::vector<float> low(n), hi_pow(n), low_pow(n), low_sq(n);
    const double k_hi = 1.0 - std::exp(-1.0 / (HISS_TAU_SECONDS * fs));
    const double k_lo = 1.0 - std::exp(-1.0 / (HISS_LOW_TAU_SECONDS * fs));
    double ph = 0.0, pl = 0.0;
    for (size_t i = 0; i < n; ++i) {
        low[i] = static_cast<float>(split2.tick(split1.tick(wave[i])));
        const double h = det_hp.tick(wave[i]);
        const double l = det_lp.tick(wave[i]);
        ph += (h * h - ph) * k_hi;
        pl += (l * l - pl) * k_lo;
        hi_pow[i] = static_cast<float>(ph);
        low_pow[i] = static_cast<float>(pl);
        low_sq[i] = static_cast<float>(l * l);
    }
    // The vowel reference: the low band's power averaged over two glottal
    // periods (a shorter follower rides on the pulses and overstates the
    // vowel by a few dB), the loudest within the window either side.
    const size_t avg_len = std::max<size_t>(1, static_cast<size_t>(HISS_VOWEL_AVERAGE_SECONDS * fs));
    std::vector<float> average(n);
    {
        double sum = 0.0;
        const size_t lead = avg_len / 2;
        for (size_t i = 0; i < n + lead; ++i) {
            if (i < n) sum += low_sq[i];
            if (i >= avg_len && i - avg_len < n) sum -= low_sq[i - avg_len];
            if (i < lead) continue;
            const size_t j = i - lead;
            const size_t from = i >= avg_len ? i - avg_len + 1 : 0;
            const size_t to = std::min(i, n - 1);
            average[j] = static_cast<float>(std::max(0.0, sum) / static_cast<double>(to - from + 1));
        }
    }
    const size_t half = static_cast<size_t>(HISS_VOWEL_WINDOW_SECONDS * fs);
    std::vector<float> vowel(n);
    std::deque<size_t> window;
    for (size_t i = 0; i < n + half; ++i) {
        if (i < n) {
            while (!window.empty() && average[window.back()] <= average[i]) window.pop_back();
            window.push_back(i);
        }
        if (i < half) continue;
        const size_t j = i - half;
        while (!window.empty() && window.front() + 2 * half < i) window.pop_front();
        vowel[j] = average[window.front()];
    }
    // The gain the hiss band wants, sample by sample.
    const float target = std::pow(10.0f, hiss_db / 10.0f);              // in power
    const float floor_gain = std::pow(10.0f, -VOLUME_BOOST_HISS_MAX_CUT_DB / 20.0f);
    std::vector<float> want(n, 1.0f);
    for (size_t i = 0; i < n; ++i) {
        if (!(hi_pow[i] > 0.0f) || !(vowel[i] > 0.0f)) continue;
        const float dominance_db = 10.0f * std::log10(hi_pow[i] / (low_pow[i] + 1e-20f));
        const float share = std::clamp((dominance_db - HISS_GATE_LOW_DB) /
                                       (HISS_GATE_HIGH_DB - HISS_GATE_LOW_DB), 0.0f, 1.0f);
        if (share <= 0.0f) continue;
        const float level = std::min(1.0f, std::sqrt(vowel[i] * target / hi_pow[i]));
        const float g = std::max(floor_gain, level);
        want[i] = 1.0f - share * (1.0f - g);
    }
    // Down before the hiss arrives and held a little after it, then released.
    const size_t ahead = static_cast<size_t>(HISS_LOOKAHEAD_SECONDS * fs);
    const size_t hold = static_cast<size_t>(HISS_HOLD_SECONDS * fs);
    const float release = std::exp(-1.0f / (HISS_RELEASE_SECONDS * fs));
    window.clear();
    float g = 1.0f;
    for (size_t i = 0; i < n + ahead; ++i) {
        if (i < n) {
            while (!window.empty() && want[window.back()] >= want[i]) window.pop_back();
            window.push_back(i);
        }
        if (i < ahead) continue;
        const size_t j = i - ahead;
        while (!window.empty() && window.front() + hold < j) window.pop_front();
        const float t = window.empty() ? 1.0f : want[window.front()];
        g = t < g ? t : g + (t - g) * (1.0f - release);
        wave[j] = low[j] + g * (wave[j] - low[j]);
    }
}

// The largest magnitude between samples i and i+1 as a resampler will
// reconstruct it: an 8-tap Lanczos interpolator (a = 4) at a quarter, a half
// and three quarters of the way. A cubic through four samples missed 0.3 dB
// of Josip's peaks, which then went over -1 dBTP.
inline float inter_sample_peak(const std::vector<float>& x, size_t i) {
    static const float W[3][8] = {
        {-0.0150542f, 0.0554490f, -0.1523039f, 0.8933886f, 0.2826839f, -0.0916606f, 0.0314677f, -0.0039706f},
        {-0.0126302f, 0.0597641f, -0.1660114f, 0.6188774f, 0.6188774f, -0.1660114f, 0.0597641f, -0.0126302f},
        {-0.0039706f, 0.0314677f, -0.0916606f, 0.2826839f, 0.8933886f, -0.1523039f, 0.0554490f, -0.0150542f},
    };
    const long n = static_cast<long>(x.size());
    float taps[8];
    for (int k = 0; k < 8; ++k) {
        const long j = std::clamp(static_cast<long>(i) + k - 3, 0L, n - 1);
        taps[k] = x[static_cast<size_t>(j)];
    }
    float peak = std::abs(x[i]);
    for (const auto& w : W) {
        float v = 0.0f;
        for (int k = 0; k < 8; ++k) v += w[k] * taps[k];
        peak = std::max(peak, std::abs(v));
    }
    return peak;
}

} // namespace boost_detail

/**
 * Multiply `wave` by `gain` and hold the peaks of the result under `ceiling`
 * with a lookahead peak limiter whose gain never moves inside a glottal
 * cycle; what is still above `knee` is rounded off below full scale with a
 * soft clipper. In place; deterministic. The limiter starts at unity gain,
 * so a quiet clause is the plain product.
 */
inline void boost_volume(std::vector<float>& wave, uint32_t sample_rate,
                         float gain = VOLUME_BOOST_GAIN,
                         float ceiling = VOLUME_BOOST_CEILING,
                         float knee = VOLUME_BOOST_KNEE,
                         float max_lift_db = VOLUME_BOOST_MAX_LIFT_DB,
                         float treble_db = VOLUME_BOOST_TREBLE_DB,
                         float hiss_db = VOLUME_BOOST_HISS_DB) {
    using namespace boost_detail;
    if (wave.empty() || !(gain > 0.0f) || !(ceiling > 0.0f)) return;
    const float fs = sample_rate > 0 ? static_cast<float>(sample_rate) : 22050.0f;
    const size_t lookahead = std::max<size_t>(1, static_cast<size_t>(LOOKAHEAD_SECONDS * fs));
    const size_t hold = static_cast<size_t>(HOLD_SECONDS * fs);
    const size_t lift_window = static_cast<size_t>(LIFT_WINDOW_SECONDS * fs);
    const float release = std::exp(-1.0f / (RELEASE_SECONDS * fs));
    const float max_lift = max_lift_db > 0.0f ? std::pow(10.0f, max_lift_db / 20.0f) : 0.0f;
    const size_t n = wave.size();

    // The treble shelf (the high shelf of the Audio EQ Cookbook with the
    // slope 1: treble_db at the top, half of it at VOLUME_BOOST_TREBLE_HZ).
    if (treble_db < 0.0f) {
        const double A = std::pow(10.0, treble_db / 40.0);
        const double w0 = 2.0 * 3.14159265358979 * VOLUME_BOOST_TREBLE_HZ / fs;
        const double cw = std::cos(w0);
        const double sa = 2.0 * std::sqrt(A) * std::sin(w0) / 2.0 * std::sqrt(2.0);
        const double a0 = (A + 1.0) - (A - 1.0) * cw + sa;
        const double b0 = A * ((A + 1.0) + (A - 1.0) * cw + sa) / a0;
        const double b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cw) / a0;
        const double b2 = A * ((A + 1.0) + (A - 1.0) * cw - sa) / a0;
        const double a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cw) / a0;
        const double a2 = ((A + 1.0) - (A - 1.0) * cw - sa) / a0;
        double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;
        for (size_t i = 0; i < n; ++i) {
            const double x = wave[i];
            const double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = x; y2 = y1; y1 = y;
            wave[i] = static_cast<float>(y);
        }
    }

    // The hissing consonants held under the vowels (VOLUME_BOOST_HISS_DB).
    if (hiss_db < 0.0f) limit_hiss(wave, fs, hiss_db);

    // The gain each sample wants on its own: 1 below the ceiling, less above,
    // with the peak between this sample and the next counted too.
    for (size_t i = 0; i < n; ++i) wave[i] *= gain;
    std::vector<float> want(n);
    for (size_t i = 0; i < n; ++i) {
        const float a = inter_sample_peak(wave, i);
        want[i] = a > ceiling ? ceiling / a : 1.0f;
    }

    // The envelope: the sliding minimum of `want` over the next `lookahead`
    // and the last `hold` samples (monotonic deque).
    std::vector<float> envelope(n);
    std::deque<size_t> window;
    for (size_t i = 0; i < n + lookahead; ++i) {
        if (i < n) {
            while (!window.empty() && want[window.back()] >= want[i]) window.pop_back();
            window.push_back(i);
        }
        if (i < lookahead) continue;
        const size_t j = i - lookahead;
        while (!window.empty() && window.front() + hold < j) window.pop_front();
        envelope[j] = window.empty() ? 1.0f : want[window.front()];
    }

    // The lift cap: the envelope may not exceed `max_lift` times its own
    // running minimum over the last `lift_window` samples. Applied to the
    // envelope, before the attack averages it, so the gain still moves only
    // in the attack's ramps and the release's curve.
    if (max_lift > 0.0f) {
        const std::vector<float> raw = envelope;
        window.clear();
        for (size_t j = 0; j < n; ++j) {
            while (!window.empty() && raw[window.back()] >= raw[j]) window.pop_back();
            window.push_back(j);
            while (!window.empty() && window.front() + lift_window < j) window.pop_front();
            const float cap = raw[window.front()] * max_lift;
            if (envelope[j] > cap) envelope[j] = cap;
        }
    }

    // The attack (a box average over the lookahead) and the release.
    const bool clip = knee > 0.0f && knee < 1.0f;
    double sum = 0.0;
    float g = 1.0f;
    for (size_t j = 0; j < n; ++j) {
        sum += envelope[j];
        if (j >= lookahead) sum -= envelope[j - lookahead];
        const float target = static_cast<float>(sum / static_cast<double>(std::min(j + 1, lookahead)));
        g = target < g ? target : g + (target - g) * (1.0f - release);
        float x = wave[j] * g;
        if (clip) {
            const float a = std::abs(x);
            if (a > knee) {
                const float over = (a - knee) / (1.0f - knee);
                const float m = knee + (1.0f - knee) * std::tanh(over);
                x = x < 0.0f ? -m : m;
            }
        }
        wave[j] = x;
    }
}

} // namespace laprdus

#endif // LAPRDUS_VOLUME_BOOST_HPP
