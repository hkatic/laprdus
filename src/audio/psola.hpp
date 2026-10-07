// -*- coding: utf-8 -*-
// psola.hpp - Time-domain pitch-synchronous overlap-add renderer
//
// Renders a clause from analysed recordings (unit_bank.hpp) at the pitch
// and the durations the prosody asks for, with nothing but the recordings
// themselves: every output pitch period is one Hann-windowed period of a
// recording, centred on one of its pitch marks, placed at the output mark
// (Moulines & Charpentier 1990). Pitch comes from the spacing of the output
// marks, duration from how often a recording's marks are reused or skipped,
// and the two recordings at a join are windowed at the same output marks,
// so the join itself is a crossfade of whole periods and cannot click.
//
// Unvoiced sound is overlap-added at a fixed spacing; a frame that has to
// be repeated to lengthen it is played backwards the second time, which
// keeps a stretched fricative from turning into a buzz (the MBROLA trick).

#ifndef LAPRDUS_PSOLA_HPP
#define LAPRDUS_PSOLA_HPP

#include "unit_bank.hpp"
#include <cstdint>
#include <vector>

namespace laprdus {
namespace concat {

/** One stretch of the output: a recording played over a time span, or silence. */
struct Segment {
    const Unit* unit = nullptr;     // nullptr: silence
    float start = 0.0f;             // output start (samples)
    float length = 0.0f;            // output length (samples)
    int32_t src_begin = 0;          // source range used, [src_begin, src_end)
    int32_t src_end = 0;
    bool stretch = true;            // false: natural rate, cut off at the end of the span
    bool join_prev = false;         // crossfade periods with the previous segment's recording
    bool join_next = false;         // ... and with the next one's
};

/**
 * Pitch contour of a clause: one value per millisecond of output, in Hz.
 * Values are interpolated between the grid points.
 */
struct PitchContour {
    std::vector<float> hz;          // hz[k] at k ms

    float at(float t_samples, uint32_t sample_rate) const;
};

/**
 * Render the segments of a clause.
 * @param segments Segments in time order, laid out by the planner.
 * @param contour Target pitch along the clause.
 * @param sample_rate Output sample rate (that of the recordings).
 * @param total_samples Length of the clause's sound; the output is a little
 *        longer, so the last window can fade out.
 * @return Samples in -1..1 (louder passages may exceed it; the caller limits).
 */
std::vector<float> render(const std::vector<Segment>& segments,
                          const PitchContour& contour,
                          uint32_t sample_rate,
                          float total_samples);

} // namespace concat
} // namespace laprdus

#endif // LAPRDUS_PSOLA_HPP
