// brightness_tilt.hpp - The brightness setting as a tilt of the whole output.
//
// The signal is split by a one-pole low-pass at 1.5 kHz into its body and
// its top, and the top is scaled: 50 leaves the sound as it is, 0 folds the
// top away (the setting at which a phone's speaker at full volume stops
// biting), 100 doubles it (+6 dB). The recorded voices have used it since
// the setting came; the formant voices apply it too since November 2026, on
// top of their source emphasis, so that brightness also reaches the hissing
// consonants and the bursts, which never pass through the voice source (a
// listener heard brightness 0 leave the s and š as sharp as before).
// Header-only so that every build (SCons, CMake, Xcode) compiles it without
// a new source file.

#ifndef LAPRDUS_BRIGHTNESS_TILT_HPP
#define LAPRDUS_BRIGHTNESS_TILT_HPP

#include <laprdus/types.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace laprdus {

constexpr float BRIGHTNESS_TILT_HZ = 1500.0f;

inline void tilt_brightness(std::vector<float>& wave, uint32_t fs, int brightness) {
    const int v = std::clamp(brightness, VOICE_QUALITY_MIN, VOICE_QUALITY_MAX);
    if (v == VOICE_QUALITY_DEFAULT || wave.empty()) return;
    const float top_gain = 1.0f + static_cast<float>(v - 50) / 50.0f;   // 0 .. 2
    const float coef = std::exp(-2.0f * 3.14159265f * BRIGHTNESS_TILT_HZ / static_cast<float>(fs));
    float low = 0.0f;
    for (float& x : wave) {
        low = coef * low + (1.0f - coef) * x;
        x = low + top_gain * (x - low);
    }
}

} // namespace laprdus

#endif // LAPRDUS_BRIGHTNESS_TILT_HPP
