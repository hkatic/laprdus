// -*- coding: utf-8 -*-
// formant_intonation.cpp - Intonation model of one clause (see the header).
//
// The rules were tuned on the formant voices against recordings of read
// Croatian (docs/formant.md, "Intonation") and are used unchanged by the
// recorded voices, whose pitch is set by the PSOLA renderer.

#include "formant_intonation.hpp"
#include <algorithm>
#include <cmath>

namespace laprdus {
namespace formant {

Intonation::Intonation(const Utterance& utt, const std::vector<Syllable>& syls,
                       float total_ms, float pitch_range, bool enabled)
    : m_total(std::max(total_ms, 1.0f)), m_enabled(enabled) {
    if (!enabled) return;
    const float total = m_total;
    std::vector<int> accents;
    for (size_t s = 0; s < syls.size(); ++s) {
        if (syls[s].stressed && syls[s].t0 >= 0.0f) accents.push_back(static_cast<int>(s));
    }

    const ClauseKind kind = utt.kind;
    const bool question = kind == ClauseKind::YesNoQuestion ||
                          kind == ClauseKind::WhQuestion;
    float range = pitch_range * (kind == ClauseKind::Exclamation ? 1.35f : 1.0f);
    // A lone syllable (a letter name while spelling, "da", "ne") has
    // no room for a full sentence melody: squeezed into it, the
    // movement is a steep glide that also hides the pitch steps
    // screen readers use to mark capital letters. A question keeps
    // its rise and an exclamation ("Ne!", "Stoj!") most of its fall,
    // or the punctuation could not be heard at all.
    if (syls.size() == 1 && !question) {
        range *= kind == ClauseKind::Exclamation ? 0.70f : 0.45f;
    }

    // In a short clause the end is all there is to tell a question
    // from a statement; up to five words get the full final rise,
    // longer clauses gradually less.
    const int words = utt.phones.empty() ? 0 : static_cast<int>(utt.phones.back().word) + 1;
    const float brevity =
        std::clamp((9.0f - static_cast<float>(words)) / 4.0f, 0.4f, 1.0f);
    const int final_index = static_cast<int>(syls.size()) - 1;

    // The syllable that carries a question's rise.
    int question_syl = -1;
    if (kind == ClauseKind::YesNoQuestion && !accents.empty()) {
        question_syl = accents.back();
        if (utt.focus_word >= 0) {
            for (int a : accents) {
                if (syls[static_cast<size_t>(a)].word == utt.focus_word) question_syl = a;
            }
        }
    } else if (kind == ClauseKind::WhQuestion && !accents.empty() &&
               accents.back() == final_index) {
        // "Što?", "A gdje?", "Tko je to?": the last accent is the
        // last syllable, so the final rise is that accent's own.
        question_syl = accents.back();
    }

    for (size_t ai = 0; ai < accents.size(); ++ai) {
        const int s = accents[ai];
        const Syllable& syl = syls[static_cast<size_t>(s)];
        const float t0 = syl.t0;
        const float t1 = syl.t1;
        const float d = t1 - t0;

        // The next syllable, if unstressed, carries the tail of the accent.
        const Syllable* post = nullptr;
        if (static_cast<size_t>(s) + 1 < syls.size() &&
            !syls[static_cast<size_t>(s) + 1].stressed &&
            syls[static_cast<size_t>(s) + 1].t0 >= 0.0f) {
            post = &syls[static_cast<size_t>(s) + 1];
        }
        const bool more_after = static_cast<size_t>(s) + 2 < syls.size();

        Bump b;
        if (s == question_syl) {
            // Low on the stressed syllable, high right after it.
            float q = 7.0f * std::min(range, 1.2f);
            if (kind == ClauseKind::WhQuestion) {
                // After the peak on the question word the rise is
                // the smaller of the two movements.
                q *= brevity * (ai > 0 ? 0.8f : 1.0f);
            }
            const float low = 0.3f * q;
            if (post) {
                b.add(t0 - 30.0f, 0.0f);
                b.add(t0 + 0.4f * d, -low);
                b.add(t1, 0.45f * q);
                b.add(post->t0 + 0.5f * (post->t1 - post->t0), q);
                b.add(total, more_after ? 0.30f * q : 0.85f * q);
            } else {
                // The whole movement has to fit into one vowel, and
                // its end fades: low from the start of the vowel, at
                // the top by three quarters of it and held there.
                b.add(t0 - 60.0f, 0.0f);
                b.add(t0 - 10.0f, -low);
                b.add(t0 + 0.15f * d, -low);
                b.add(t0 + 0.75f * d, 0.85f * q);
                b.add(total, 0.85f * q);
            }
            m_bumps.push_back(std::move(b));
            continue;
        }

        // Every content word gets a clearly audible movement (the
        // "pointed hats" of the classic rule-based synthesizers),
        // largest on the first one and shrinking along the clause.
        float p = syl.prominence >= 2 ? (ai == 0 ? 4.5f : 3.6f) : 1.8f;
        p *= std::max(0.75f, 1.0f - 0.06f * static_cast<float>(ai));
        if (ai == 0 && kind == ClauseKind::WhQuestion) p = 5.5f;
        if (kind == ClauseKind::Exclamation) {
            // Emphatic: a high start and a strong last accent to
            // fall from.
            if (ai == 0) p += 1.0f;
            if (ai + 1 == accents.size() && syl.prominence >= 2) p = std::max(p + 1.5f, 4.0f);
        }
        if (question_syl >= 0 && s > question_syl) p *= 0.4f;
        p *= range;

        Accent accent = syl.accent;
        if (accent == Accent::Rising && !post) accent = Accent::Neutral;

        switch (accent) {
            case Accent::Falling:
                // High early in the stressed vowel, falling within it.
                b.add(t0 - 40.0f, 0.0f);
                b.add(t0 + 0.25f * d, p);
                b.add(t1, 0.40f * p);
                b.add(post ? post->t0 + 0.5f * (post->t1 - post->t0) : t1 + 70.0f, -0.3f);
                b.add(post ? post->t1 + 20.0f : t1 + 110.0f, 0.0f);
                break;
            case Accent::Rising:
                // Rising through the stressed vowel; the peak is in
                // the following syllable.
                b.add(t0 - 25.0f, 0.0f);
                b.add(t0 + 0.15f * d, -0.4f);
                b.add(t1, 0.75f * p);
                b.add(post->t0 + 0.4f * (post->t1 - post->t0), p);
                b.add(post->t1 + 40.0f, 0.0f);
                break;
            case Accent::Neutral:
            default:
                b.add(t0 - 30.0f, 0.0f);
                b.add(t0 + 0.55f * d, p);
                b.add(t1, 0.85f * p);
                b.add(post ? post->t0 + 0.5f * (post->t1 - post->t0) : t1 + 70.0f,
                      0.20f * p);
                b.add(post ? post->t1 + 30.0f : t1 + 120.0f, 0.0f);
                break;
        }
        m_bumps.push_back(std::move(b));
    }

    // Boundary movement at the end of the clause
    if (!syls.empty() && !accents.empty()) {
        const Syllable& nuclear = syls[static_cast<size_t>(accents.back())];
        const bool nuclear_is_last = accents.back() == static_cast<int>(syls.size()) - 1;
        const Syllable& final_syl = syls.back();
        // A fall to the bottom of the range right after the last
        // accent, where the voice then stays instead of sliding down
        // all the way to the end.
        auto fall_after_nucleus = [&](float depth) {
            Bump fall;
            float start = nuclear_is_last
                ? nuclear.t0 + 0.35f * (nuclear.t1 - nuclear.t0)
                : nuclear.t1;
            float length = std::clamp(total - start, 60.0f, 180.0f);
            fall.add(start, 0.0f);
            fall.add(start + length, -depth * range);
            m_bumps.push_back(std::move(fall));
        };

        Bump b;
        if (kind == ClauseKind::Continuation) {
            // The rise has to win against declination and the tail
            // of the last accent to be heard as "more follows".
            float start = nuclear_is_last
                ? nuclear.t0 + 0.5f * (nuclear.t1 - nuclear.t0)
                : (final_syl.t0 >= 0.0f ? final_syl.t0 - 40.0f : total - 160.0f);
            b.add(start, 0.0f);
            b.add(total, 4.0f * range);
        } else if (question) {
            // Every question ends going up. Where the main movement
            // lies earlier (on the word before "li", on a question
            // word), the last syllable rises on its own; in a
            // wh-question it comes back up from the fall that
            // follows the last accent.
            const bool rise_at_end = question_syl >= final_index - 1;
            if (question_syl < 0 || !rise_at_end) {
                float lift = 3.0f;
                if (kind == ClauseKind::WhQuestion) {
                    fall_after_nucleus(3.5f);
                    lift += 3.5f;
                }
                lift *= range * brevity;
                const bool timed = final_syl.t0 >= 0.0f;
                b.add(timed ? final_syl.t0 - 30.0f : total - 150.0f, 0.0f);
                b.add(timed ? final_syl.t0 + 0.75f * (final_syl.t1 - final_syl.t0)
                            : total, lift);
                b.add(total, lift);
            }
        } else {
            fall_after_nucleus(kind == ClauseKind::Exclamation ? 5.5f : 4.0f);
        }
        if (!b.points.empty()) m_bumps.push_back(std::move(b));
    }

    // Declination: the baseline drifts down along the clause, about
    // two semitones per second, so a long clause covers more of the
    // range than a short one.
    m_drop = std::clamp(total * 0.002f, 1.3f, 6.0f);
}

float Intonation::at(float t) const {
    if (!m_enabled) return 0.0f;
    float x = std::clamp(t / m_total, 0.0f, 1.0f);
    float value = m_drop * (0.37f - x);
    for (const Bump& bump : m_bumps) {
        value += bump.at(t);
    }
    return value;
}

} // namespace formant
} // namespace laprdus
