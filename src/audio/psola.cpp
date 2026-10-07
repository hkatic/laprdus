// -*- coding: utf-8 -*-
// psola.cpp - TD-PSOLA rendering of planned segments (see psola.hpp)

#include "psola.hpp"
#include <algorithm>
#include <cmath>

namespace laprdus {
namespace concat {

namespace {

constexpr float PI_F = 3.14159265358979f;

// Span of the period crossfade at a join between two voiced recordings
constexpr float JOIN_MS = 14.0f;

// Longest window the renderer places (two periods at the lowest pitch)
constexpr int MAX_HALF_WINDOW = 400;

// Pitch limits of the output marks
constexpr float OUT_F0_MIN = 40.0f;
constexpr float OUT_F0_MAX = 600.0f;

// Output time -> source sample of a segment (extrapolates past its span)
float source_pos(const Segment& s, float t) {
    if (s.stretch) {
        const float x = s.length > 0.0f ? (t - s.start) / s.length : 0.0f;
        return static_cast<float>(s.src_begin) + x * static_cast<float>(s.src_end - s.src_begin);
    }
    return static_cast<float>(s.src_begin) + (t - s.start);
}

// Index of the analysis mark nearest a source position, within the
// segment's source range; -1 if the recording has no marks there.
int nearest_mark(const Segment& s, float pos) {
    const Unit& u = *s.unit;
    if (u.marks.empty()) return -1;
    const float lo = static_cast<float>(s.src_begin);
    const float hi = static_cast<float>(std::max(s.src_begin, s.src_end - 1));
    pos = std::clamp(pos, lo, hi);
    auto it = std::lower_bound(u.marks.begin(), u.marks.end(), static_cast<int32_t>(std::lround(pos)));
    int k = static_cast<int>(it - u.marks.begin());
    int best = -1;
    float best_d = 0.0f;
    for (int c = k - 1; c <= k; ++c) {
        if (c < 0 || c >= static_cast<int>(u.marks.size())) continue;
        const int32_t m = u.marks[static_cast<size_t>(c)];
        // Only marks inside the segment's source range
        if (m < s.src_begin - u.period[static_cast<size_t>(c)] || m >= s.src_end + u.period[static_cast<size_t>(c)]) continue;
        float d = std::abs(static_cast<float>(m) - pos);
        if (best < 0 || d < best_d) { best = c; best_d = d; }
    }
    if (best >= 0) return best;
    // Nothing within range: the nearest mark anyway
    if (k >= static_cast<int>(u.marks.size())) k = static_cast<int>(u.marks.size()) - 1;
    if (k > 0 && std::abs(static_cast<float>(u.marks[static_cast<size_t>(k - 1)]) - pos) <
                     std::abs(static_cast<float>(u.marks[static_cast<size_t>(k)]) - pos)) {
        k -= 1;
    }
    return k;
}

// One Hann-windowed period of a recording, centred on its mark k, added at
// the output sample `centre`.
void add_window(std::vector<float>& out, const Unit& u, int k, int centre,
                float gain, bool reversed) {
    const int32_t mark = u.marks[static_cast<size_t>(k)];
    const int half = std::clamp(static_cast<int>(u.period[static_cast<size_t>(k)]), 8, MAX_HALF_WINDOW);
    const int n = u.length();
    const int out_n = static_cast<int>(out.size());
    const float scale = PI_F / static_cast<float>(half);
    for (int i = -half + 1; i < half; ++i) {
        const int o = centre + i;
        if (o < 0 || o >= out_n) continue;
        const int s = mark + (reversed ? -i : i);
        if (s < 0 || s >= n) continue;
        const float w = 0.5f * (1.0f + std::cos(scale * static_cast<float>(i)));
        out[static_cast<size_t>(o)] += gain * w * u.samples[static_cast<size_t>(s)];
    }
}

} // namespace

// =============================================================================
// PitchContour
// =============================================================================

float PitchContour::at(float t_samples, uint32_t sample_rate) const {
    if (hz.empty()) return 120.0f;
    const float ms = t_samples * 1000.0f / static_cast<float>(sample_rate);
    if (ms <= 0.0f) return hz.front();
    const size_t k = static_cast<size_t>(ms);
    if (k + 1 >= hz.size()) return hz.back();
    const float x = ms - static_cast<float>(k);
    return hz[k] + (hz[k + 1] - hz[k]) * x;
}

// =============================================================================
// render
// =============================================================================

std::vector<float> render(const std::vector<Segment>& segments,
                          const PitchContour& contour,
                          uint32_t sample_rate,
                          float total_samples) {
    const float fs = static_cast<float>(sample_rate);
    const int tail = MAX_HALF_WINDOW;
    const int out_len = std::max(0, static_cast<int>(std::ceil(total_samples))) + tail;
    std::vector<float> out(static_cast<size_t>(out_len), 0.0f);
    if (segments.empty()) return out;

    const float join = JOIN_MS * fs / 1000.0f;
    const float half_join = 0.5f * join;

    // Adds the window of a neighbouring segment at the same output mark.
    auto add_neighbour = [&](const Segment& nb, float t, int centre, float ts, float weight) -> bool {
        if (!nb.unit) return false;
        int k = nearest_mark(nb, source_pos(nb, t));
        if (k < 0 || !nb.unit->voiced[static_cast<size_t>(k)]) return false;
        const float ta = static_cast<float>(nb.unit->period[static_cast<size_t>(k)]);
        const float gain = weight * std::clamp(ts / ta, 0.5f, 1.6f);
        add_window(out, *nb.unit, k, centre, gain, false);
        return true;
    };

    float t = 0.0f;     // next output mark (samples)
    for (size_t i = 0; i < segments.size(); ++i) {
        const Segment& seg = segments[i];
        const float seg_end = seg.start + seg.length;
        if (!seg.unit || seg.length <= 0.0f) {
            // Silence: the marks start afresh after it
            if (seg_end > t) t = seg_end;
            continue;
        }
        const Unit& u = *seg.unit;
        if (t <= seg.start) {
            // A fresh start (clause start, after a pause): the first window
            // rises from zero at the segment's start instead of opening on
            // the peak of a period.
            int k0 = nearest_mark(seg, source_pos(seg, seg.start));
            float half = k0 >= 0 ? static_cast<float>(u.period[static_cast<size_t>(k0)]) : static_cast<float>(UNVOICED_HOP);
            t = seg.start + std::min(half, 0.5f * seg.length);
        }

        int last_k = -1;
        int placed = 0;
        while (t < seg_end || placed == 0) {
            if (placed == 0 && t >= seg_end) {
                // Too short for a whole period: still one window, in the middle
                t = seg.start + 0.5f * seg.length;
            }
            const int k = nearest_mark(seg, source_pos(seg, t));
            if (k < 0) break;
            const bool voiced = u.voiced[static_cast<size_t>(k)] != 0;
            const float ta = static_cast<float>(u.period[static_cast<size_t>(k)]);
            float ts;
            if (voiced) {
                const float f0 = std::clamp(contour.at(t, sample_rate), OUT_F0_MIN, OUT_F0_MAX);
                ts = fs / f0;
            } else {
                ts = static_cast<float>(UNVOICED_HOP);
            }
            const int centre = static_cast<int>(std::lround(t));

            // Period crossfade across a join with a voiced neighbour
            float weight = 1.0f;
            if (voiced) {
                if (seg.join_prev && i > 0 && t < seg.start + half_join) {
                    const float x = std::clamp((t - (seg.start - half_join)) / join, 0.0f, 1.0f);
                    if (add_neighbour(segments[i - 1], t, centre, ts, 1.0f - x)) weight = x;
                } else if (seg.join_next && i + 1 < segments.size() && t > seg_end - half_join) {
                    const float x = std::clamp((t - (seg_end - half_join)) / join, 0.0f, 1.0f);
                    if (add_neighbour(segments[i + 1], t, centre, ts, x)) weight = 1.0f - x;
                }
            }

            const float gain = voiced ? weight * std::clamp(ts / ta, 0.5f, 1.6f) : 1.0f;
            const bool reversed = !voiced && k == last_k;
            add_window(out, u, k, centre, gain, reversed);
            last_k = k;
            ++placed;
            t += ts;
        }
    }
    return out;
}

} // namespace concat
} // namespace laprdus
