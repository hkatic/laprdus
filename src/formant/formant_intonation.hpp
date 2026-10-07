// -*- coding: utf-8 -*-
// formant_intonation.hpp - Intonation model shared by the formant and the
// recorded voices: word accents, question and boundary movements and
// declination of one clause, as a pitch contour in semitones.

#ifndef LAPRDUS_FORMANT_INTONATION_HPP
#define LAPRDUS_FORMANT_INTONATION_HPP

#include "formant_frontend.hpp"
#include <utility>
#include <vector>

namespace laprdus {
namespace formant {

/** Timing and accent of one syllable of a clause (nucleus start and end). */
struct Syllable {
    float t0 = -1.0f;           // nucleus start (ms)
    float t1 = -1.0f;           // nucleus end (ms)
    bool stressed = false;
    Accent accent = Accent::None;
    uint8_t prominence = 0;
    int word = 0;
};

/** Piecewise-linear pitch movement, zero outside its own time span. */
struct Bump {
    std::vector<std::pair<float, float>> points;

    void add(float t, float v) { points.emplace_back(t, v); }

    float at(float t) const {
        if (points.empty() || t <= points.front().first || t >= points.back().first) {
            if (!points.empty() && t >= points.back().first) return points.back().second;
            return 0.0f;
        }
        for (size_t i = 1; i < points.size(); ++i) {
            if (t < points[i].first) {
                float span = points[i].first - points[i - 1].first;
                float x = span > 0.0f ? (t - points[i - 1].first) / span : 1.0f;
                return points[i - 1].second + (points[i].second - points[i - 1].second) * x;
            }
        }
        return points.back().second;
    }
};

/**
 * Intonation - the pitch contour of one clause, built from its syllables.
 *
 * Every stressed syllable of a content word gets an accent movement (its
 * shape chosen by the word's accent type), a question its rise on the
 * focused word, the clause its boundary movement (fall, suspended rise,
 * final rise) and a declining baseline. at(t) sums them all at time t (ms
 * from the start of the clause's sound) in semitones around the voice's base
 * pitch, before the inflection level is applied.
 */
class Intonation {
public:
    /**
     * @param utt The clause (its kind, focus word and phones).
     * @param syls Its syllables with nucleus timing.
     * @param total_ms Length of the clause's sound.
     * @param pitch_range Size of the movements (1.0 = as measured).
     * @param enabled false gives a monotone (at() returns 0).
     */
    Intonation(const Utterance& utt, const std::vector<Syllable>& syls,
               float total_ms, float pitch_range, bool enabled);

    float at(float t_ms) const;

    bool enabled() const { return m_enabled; }

private:
    std::vector<Bump> m_bumps;
    float m_total = 1.0f;
    float m_drop = 0.0f;
    bool m_enabled = false;
};

} // namespace formant
} // namespace laprdus

#endif // LAPRDUS_FORMANT_INTONATION_HPP
