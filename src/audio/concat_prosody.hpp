// -*- coding: utf-8 -*-
// concat_prosody.hpp - Durations and pitch of a clause for the recorded voices
//
// The recorded voices use the same text front end as the formant voices
// (src/formant/formant_frontend.*): letter-to-sound rules, the accent
// lexicon, stress, clitics and assimilation. From its phones the planner
// lays the recordings out in time (segment durations by the measured rules
// of formant_phonemes.cpp, stress and final lengthening, closures before
// stops, short gaps between words) and builds the pitch contour with the
// shared intonation model (formant_intonation.*), so that Josip and Vlado
// get the word accents, question rises and final falls of Zvonko.

#ifndef LAPRDUS_CONCAT_PROSODY_HPP
#define LAPRDUS_CONCAT_PROSODY_HPP

#include "laprdus/types.hpp"
#include "psola.hpp"
#include "unit_bank.hpp"
#include "../formant/formant_frontend.hpp"
#include <vector>

namespace laprdus {
namespace concat {

/** The laid-out clause: segments to render and the pitch to render them at. */
struct ClausePlan {
    std::vector<Segment> segments;
    PitchContour contour;
    float total_samples = 0.0f;
};

/**
 * Recording that speaks a phone of the front end.
 * @return The phoneme, or Phoneme::COUNT for a phone without a recording
 *         (the schwa of a syllabic consonant).
 */
Phoneme unit_for(formant::Ph ph);

/**
 * Lay out one clause.
 * @param utt The clause from the front end.
 * @param bank Analysed recordings of the voice.
 * @param params Rate, pitch, inflection.
 * @param natural_f0 Pitch of the voice as recorded (Hz), before the voice
 *        character and user pitch.
 * @param sample_rate Sample rate of the recordings.
 */
ClausePlan plan_clause(const formant::Utterance& utt, const UnitBank& bank,
                       const VoiceParams& params, float natural_f0,
                       uint32_t sample_rate);

} // namespace concat
} // namespace laprdus

#endif // LAPRDUS_CONCAT_PROSODY_HPP
