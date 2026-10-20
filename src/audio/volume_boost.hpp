// volume_boost.hpp - Louder output for both kinds of voices.
//
// The speech of the formant voices peaks close to full scale while its
// median level sits near -20 dBFS, and the recorded voices already touch
// the ceiling, so a plain gain cannot make either louder without clipping.
// The volume boost is a loudness maximizer in three stages, as a phone's
// earpiece in traffic needs (the model is Google TTS's "amplify speech
// volume", a single switch with a difference one cannot miss):
//
//  1. a gain of VOLUME_BOOST_GAIN (+14 dB);
//  2. a peak limiter whose gain envelope sees the peaks coming (a short
//     lookahead) and lets go quickly afterwards, holding them under
//     VOLUME_BOOST_CEILING; the ceiling is above full scale on purpose, so
//     the limiter only tames the strongest peaks and the level between them
//     rides up (a limiter alone stalls at about -8 dBFS on speech, whose
//     peaks stand 12 dB above its level);
//  3. a soft clipper that rounds off what is still above VOLUME_BOOST_KNEE
//     into the room below full scale (tanh), so nothing clips hard.
//
// Stages 1 and 2 are linear time-varying gain; stage 3 adds harmonics to
// the strongest vowel peaks (about -20 dB of distortion energy on the
// formant voices, -17 dB on the recorded ones at these settings), which is
// the price of the loudness and still speech, not a buzz. Measured on
// "Pozivam Marka Marića na mobitel": Zvonko's median speech level rises
// from -19.9 to -8.8 dBFS and his loud stretches from -13.6 to -6.0 dBFS;
// Josip's from -11.6 to -6.2 and from -8.0 to -4.3 dBFS. Header-only so that
// every build (SCons, CMake, Xcode) compiles it without a new source file.
//
// The boost sits before the volume (host or user) is applied, so a host at
// half volume with the boost on is still louder than without it, and the
// final soft knee of each renderer stays as the safety net.

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

constexpr float LOOKAHEAD_SECONDS = 0.005f;  // the envelope leads the peak by 5 ms
constexpr float ATTACK_SECONDS = 0.001f;     // and reaches it within the lookahead
constexpr float RELEASE_SECONDS = 0.030f;    // gain recovers over about 30 ms

} // namespace boost_detail

/**
 * Multiply `wave` by `gain`, hold the peaks of the result under `ceiling`
 * with a lookahead peak limiter, and round what is still above `knee` off
 * below full scale with a soft clipper. In place; deterministic. The
 * limiter starts at unity gain, so a quiet clause is the plain product.
 */
inline void boost_volume(std::vector<float>& wave, uint32_t sample_rate,
                         float gain = VOLUME_BOOST_GAIN,
                         float ceiling = VOLUME_BOOST_CEILING,
                         float knee = VOLUME_BOOST_KNEE) {
    using namespace boost_detail;
    if (wave.empty() || !(gain > 0.0f) || !(ceiling > 0.0f)) return;
    const float fs = sample_rate > 0 ? static_cast<float>(sample_rate) : 22050.0f;
    const size_t lookahead = std::max<size_t>(1, static_cast<size_t>(LOOKAHEAD_SECONDS * fs));
    const float attack = std::exp(-1.0f / (ATTACK_SECONDS * fs));
    const float release = std::exp(-1.0f / (RELEASE_SECONDS * fs));
    const size_t n = wave.size();

    // The gain each sample wants on its own: 1 below the ceiling, less above.
    std::vector<float> want(n);
    for (size_t i = 0; i < n; ++i) {
        wave[i] *= gain;
        const float a = std::abs(wave[i]);
        want[i] = a > ceiling ? ceiling / a : 1.0f;
    }

    // Sliding minimum over the next `lookahead` samples (monotonic deque),
    // so the envelope starts down before the peak arrives.
    std::deque<size_t> window;
    float g = 1.0f;
    const bool clip = knee > 0.0f && knee < 1.0f;
    for (size_t i = 0; i < n + lookahead; ++i) {
        if (i < n) {
            while (!window.empty() && want[window.back()] >= want[i]) window.pop_back();
            window.push_back(i);
        }
        if (i < lookahead) continue;
        const size_t j = i - lookahead;
        while (!window.empty() && window.front() < j) window.pop_front();
        const float target = window.empty() ? 1.0f : want[window.front()];
        if (target < g) {
            g = attack * g + (1.0f - attack) * target;
        } else {
            g = release * g + (1.0f - release) * target;
        }
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
