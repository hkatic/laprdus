// -*- coding: utf-8 -*-
// concat_prosody.cpp - Clause layout for the recorded voices (see the header)

#include "concat_prosody.hpp"
#include "../formant/formant_intonation.hpp"
#include "../formant/formant_phonemes.hpp"
#include <algorithm>
#include <cmath>

namespace laprdus {
namespace concat {

using formant::Accent;
using formant::ClauseKind;
using formant::Ph;
using formant::PhClass;
using formant::Phone;
using formant::ph_def;

namespace {

// Gap between two words (ms at rate 1.0); clitics join their host without one
constexpr float WORD_GAP_MS = 22.0f;
constexpr float WORD_GAP_MIN_MS = 5.0f;

// The burst of a stop is played whole even when the rate leaves less time
constexpr float BURST_MIN_MS = 20.0f;

// A trill recording: the duration of /r/ between vowels
constexpr float TRILL_MS = 50.0f;
constexpr float TRILL_MIN_MS = 22.0f;

// Grid of the pitch contour
constexpr float CONTOUR_STEP_MS = 1.0f;

bool is_burst(Ph ph) {
    PhClass c = ph_def(ph).cls;
    return c == PhClass::Stop || c == PhClass::Affricate;
}

bool is_consonant_ph(Ph ph) {
    PhClass c = ph_def(ph).cls;
    return c != PhClass::Vowel && c != PhClass::Silence;
}

} // namespace

// =============================================================================
// unit_for
// =============================================================================

Phoneme unit_for(Ph ph) {
    switch (ph) {
        case Ph::SIL: return Phoneme::SILENCE;
        case Ph::A: return Phoneme::A;
        case Ph::E: return Phoneme::E;
        case Ph::I: return Phoneme::I;
        case Ph::O: return Phoneme::O;
        case Ph::U: return Phoneme::U;
        case Ph::SCHWA: return Phoneme::COUNT;
        case Ph::J: return Phoneme::J;
        case Ph::V: return Phoneme::V;
        case Ph::L: return Phoneme::L;
        case Ph::LJ: return Phoneme::LJ;
        case Ph::M: return Phoneme::M;
        case Ph::N: return Phoneme::N;
        case Ph::NJ: return Phoneme::NJ;
        case Ph::NG: return Phoneme::N;
        case Ph::R: return Phoneme::R;
        case Ph::P: return Phoneme::P;
        case Ph::B: return Phoneme::B;
        case Ph::T: return Phoneme::T;
        case Ph::D: return Phoneme::D;
        case Ph::K: return Phoneme::K;
        case Ph::G: return Phoneme::G;
        case Ph::C: return Phoneme::C;
        case Ph::DZ: return Phoneme::C;
        case Ph::CH: return Phoneme::CH;
        case Ph::DZH: return Phoneme::DJ;
        case Ph::TJ: return Phoneme::TJ;
        case Ph::DJ: return Phoneme::DJ;
        case Ph::F: return Phoneme::F;
        case Ph::S: return Phoneme::S;
        case Ph::Z: return Phoneme::Z;
        case Ph::SH: return Phoneme::SH;
        case Ph::ZH: return Phoneme::ZH;
        case Ph::SJ: return Phoneme::SH;
        case Ph::ZJ: return Phoneme::ZH;
        case Ph::H: return Phoneme::H;
        case Ph::GH: return Phoneme::H;
        default: return Phoneme::COUNT;
    }
}

// =============================================================================
// plan_clause
// =============================================================================

ClausePlan plan_clause(const formant::Utterance& utt, const UnitBank& bank,
                       const VoiceParams& params, float natural_f0,
                       uint32_t sample_rate) {
    ClausePlan plan;
    const std::vector<Phone>& ph = utt.phones;
    const int n = static_cast<int>(ph.size());
    const float fs = static_cast<float>(sample_rate);
    const float samples_per_ms = fs / 1000.0f;

    const float speed = std::clamp(params.speed, CONCAT_SPEED_MIN, CONCAT_SPEED_MAX);
    auto rate = [&](float d, float min_d) {
        float floor_d = speed > 1.0f ? min_d / std::sqrt(speed) : min_d;
        return std::max(d / speed, floor_d);
    };
    auto phone_at = [&](int i) -> const Phone* {
        return (i >= 0 && i < n) ? &ph[static_cast<size_t>(i)] : nullptr;
    };

    // ---------------------------------------------------------------------
    // Durations (ms): the rules of the formant voices
    // ---------------------------------------------------------------------
    const int last_syl = utt.syllable_count - 1;
    const float final_stretch =
        utt.kind == ClauseKind::Statement || utt.kind == ClauseKind::Exclamation ? 1.40f : 1.30f;
    int last_nucleus = -1;
    for (int i = 0; i < n; ++i) {
        if (ph[static_cast<size_t>(i)].nucleus) last_nucleus = i;
    }

    std::vector<float> dur(static_cast<size_t>(n), 0.0f);
    for (int i = 0; i < n; ++i) {
        const Phone& p = ph[static_cast<size_t>(i)];
        const formant::PhDef& def = ph_def(p.ph);
        const Phone* prev = phone_at(i - 1);
        const Phone* next = phone_at(i + 1);
        const Phone* next2 = phone_at(i + 2);
        const bool final_syl = p.syllable == last_syl;
        float d = def.dur;
        float min_d = def.min_dur;

        if (p.ph == Ph::SCHWA) {
            // No recording: the syllabic consonant carries the syllable
            dur[static_cast<size_t>(i)] = 0.0f;
            continue;
        }
        if (p.ph == Ph::R) {
            // The recording is a trill; a syllabic r a little longer
            d = TRILL_MS * (p.nucleus_tail ? 1.3f : 1.0f);
            if (p.nucleus_tail && p.stressed) d *= 1.2f;
            if (p.nucleus_tail && final_syl) d *= 1.2f;
            min_d = TRILL_MIN_MS;
        } else if (p.nucleus) {
            float factor;
            if (p.stressed) {
                factor = p.is_long ? 1.90f : 1.35f;
                if (p.prominence < 2) factor = p.is_long ? 1.50f : 1.15f;
                if (p.word_syllables == 1 && p.prominence == 2) factor *= 1.12f;
            } else {
                factor = p.is_long ? 1.25f : 1.0f;
                if (p.prominence == 0) factor *= 0.90f;
            }
            if (p.word_syllables >= 6) {
                factor *= 0.88f;
            } else if (p.word_syllables >= 4) {
                factor *= 0.93f;
            }
            if (next && next->word == p.word && formant::is_obstruent(next->ph) &&
                !ph_def(next->ph).voiced) {
                factor *= 0.93f;
            }
            if ((next && formant::is_vowel(next->ph)) || (prev && formant::is_vowel(prev->ph))) {
                factor *= 0.90f;
            }
            if (next && is_consonant_ph(next->ph) && (!next2 || is_consonant_ph(next2->ph))) {
                factor *= 0.93f;    // closed syllable
            }
            if (final_syl) {
                // Half the final lengthening for a short stressed vowel in
                // a statement, as in the formant voices: sȁd stays short
                // next to sȃd.
                const bool statement = utt.kind == ClauseKind::Statement ||
                                       utt.kind == ClauseKind::Exclamation;
                factor *= p.stressed && !p.is_long && statement && p.prominence == 2
                              ? 1.0f + (final_stretch - 1.0f) * 0.5f
                              : final_stretch;
            } else if (p.syllable == last_syl - 1) {
                factor *= 1.08f;
            }
            d = def.dur * factor;
        } else {
            float factor = 0.92f;
            const bool prev_c = prev && is_consonant_ph(prev->ph);
            const bool next_c = next && is_consonant_ph(next->ph);
            if (prev_c && next_c) {
                factor *= 0.80f;
            } else if (prev_c || next_c) {
                factor *= 0.88f;
            }
            if (p.word_start) factor *= 1.05f;
            if (next && next->nucleus && next->stressed) factor *= 1.08f;
            if (i > last_nucleus) factor *= 1.20f;
            if (prev && prev->ph == p.ph) factor *= 0.60f;
            d = def.dur * factor;
            if (p.short_glide) d = 34.0f;
        }
        dur[static_cast<size_t>(i)] = rate(d, min_d);
    }

    // ---------------------------------------------------------------------
    // Segments
    // ---------------------------------------------------------------------
    std::vector<formant::Syllable> syls(static_cast<size_t>(std::max(utt.syllable_count, 0)));
    float cursor = 0.0f;    // samples

    auto add_silence = [&](float samples) {
        if (samples <= 0.0f) return;
        Segment s;
        s.start = cursor;
        s.length = samples;
        plan.segments.push_back(s);
        cursor += samples;
    };
    auto note_syllable = [&](const Phone& p, float t0, float t1) {
        if (p.syllable < 0 || static_cast<size_t>(p.syllable) >= syls.size()) return;
        if (!(p.nucleus || p.nucleus_tail)) return;
        formant::Syllable& syl = syls[static_cast<size_t>(p.syllable)];
        if (syl.t0 < 0.0f || t0 < syl.t0) syl.t0 = t0;
        syl.t1 = std::max(syl.t1, t1);
        syl.word = p.word;
        syl.prominence = p.prominence;
        if (p.nucleus && p.stressed) {
            syl.stressed = true;
            syl.accent = p.accent;
        }
    };

    bool prev_is_unit = false;
    for (int i = 0; i < n; ++i) {
        const Phone& p = ph[static_cast<size_t>(i)];
        const float d_ms = dur[static_cast<size_t>(i)];

        // A short gap before a word that is not a clitic
        if (i > 0 && p.word_start && p.prominence >= 1 && prev_is_unit) {
            add_silence(rate(WORD_GAP_MS, WORD_GAP_MIN_MS) * samples_per_ms);
            prev_is_unit = false;
        }

        const Phoneme phoneme = unit_for(p.ph);
        const Unit* unit = phoneme == Phoneme::COUNT ? nullptr : &bank.unit(phoneme);
        const bool playable = unit && unit->loaded && unit->sounding_length() > 0 &&
                              phoneme != Phoneme::SILENCE;
        const float t0_ms = cursor / samples_per_ms;

        if (!playable) {
            if (d_ms > 0.0f) {
                add_silence(d_ms * samples_per_ms);
                prev_is_unit = false;
            }
            note_syllable(p, t0_ms, cursor / samples_per_ms);
            continue;
        }

        const float slot = d_ms * samples_per_ms;
        Segment s;
        s.unit = unit;
        s.src_begin = unit->lead;
        s.src_end = unit->trail;
        if (is_burst(p.ph)) {
            // A stop is never stretched. The recording is closure (silent or
            // murmured), burst and tail: the burst and as much of the tail
            // as the slot allows are played at their natural rate, the time
            // left goes to the closure, first from the recording's own, then
            // as silence. A fast rate cuts the tail and the closure, never
            // the burst.
            const float release_len = static_cast<float>(unit->trail - unit->onset);
            const float release = std::min(release_len, std::max(slot, std::min(release_len, BURST_MIN_MS * samples_per_ms)));
            const float pre_len = static_cast<float>(unit->onset - unit->lead);
            const float pre = std::clamp(slot - release, 0.0f, pre_len);
            const float closure = slot - release - pre;
            if (closure > 0.0f && prev_is_unit) {
                add_silence(closure);
                prev_is_unit = false;
            }
            s.src_begin = unit->onset - static_cast<int32_t>(pre);
            s.src_end = unit->onset + static_cast<int32_t>(release);
            s.stretch = false;
            s.length = static_cast<float>(s.src_end - s.src_begin);
        } else {
            s.stretch = true;
            s.length = slot;
        }
        s.start = cursor;
        if (prev_is_unit && !plan.segments.empty()) {
            Segment& before = plan.segments.back();
            if (before.unit && before.unit->has_voicing() && unit->has_voicing()) {
                before.join_next = true;
                s.join_prev = true;
            }
        }
        plan.segments.push_back(s);
        cursor += s.length;
        prev_is_unit = true;
        note_syllable(p, t0_ms, cursor / samples_per_ms);
    }
    plan.total_samples = cursor;

    // ---------------------------------------------------------------------
    // Pitch contour
    // ---------------------------------------------------------------------
    const float total_ms = cursor / samples_per_ms;
    const formant::Intonation intonation(utt, syls, total_ms, 1.0f, params.inflection_enabled);

    const size_t points = static_cast<size_t>(std::ceil(total_ms / CONTOUR_STEP_MS)) + 2;
    std::vector<float> st(points, 0.0f);
    for (size_t k = 0; k < points; ++k) {
        const float t = (static_cast<float>(k) + 0.5f) * CONTOUR_STEP_MS;
        st[k] = intonation.at(t);
    }

    // Pitch starts higher after a voiceless consonant and settles quickly
    if (params.inflection_enabled) {
        for (size_t si = 1; si < plan.segments.size(); ++si) {
            const Segment& seg = plan.segments[si];
            const Segment& before = plan.segments[si - 1];
            if (!seg.unit || !seg.unit->has_voicing()) continue;
            if (!before.unit || before.unit->has_voicing()) continue;
            const float start_ms = seg.start / samples_per_ms;
            const size_t k0 = static_cast<size_t>(start_ms / CONTOUR_STEP_MS);
            for (size_t k = k0; k < points && k < k0 + 150; ++k) {
                const float t = static_cast<float>(k - k0) * CONTOUR_STEP_MS;
                st[k] += 0.9f * std::exp(-t / 18.0f);
            }
        }
    }

    // The larynx cannot follow corners: smooth forwards and backwards
    const float a = std::exp(-CONTOUR_STEP_MS / 16.0f);
    for (size_t k = 1; k < points; ++k) st[k] = a * st[k - 1] + (1.0f - a) * st[k];
    for (size_t k = points - 1; k-- > 0;) st[k] = a * st[k + 1] + (1.0f - a) * st[k];

    const float level = std::clamp(params.inflection_level, 0.0f, 1.0f) / INFLECTION_LEVEL_DEFAULT;
    const float user_pitch = std::clamp(params.user_pitch, CONCAT_USER_PITCH_MIN, CONCAT_USER_PITCH_MAX);
    const float base = natural_f0 * params.pitch * user_pitch;
    plan.contour.hz.resize(points);
    for (size_t k = 0; k < points; ++k) {
        float hz = base * std::pow(2.0f, level * st[k] / 12.0f);
        plan.contour.hz[k] = std::clamp(hz, 45.0f, 480.0f);
    }
    return plan;
}

} // namespace concat
} // namespace laprdus
