// -*- coding: utf-8 -*-
// formant_synthesizer.cpp - Rule-based formant speech synthesis implementation

#include "formant_synthesizer.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <utility>

namespace laprdus {
namespace formant {

// =============================================================================
// Voices
// =============================================================================

namespace {

// Three different speakers, not one speaker with three labels: pitch, vocal
// tract length and phonation differ, as do the language-specific details
// (how far apart č and ć are, how strong /h/ is, how much of the
// post-accentual length and pitch movement is kept).
const FormantVoice VOICES[] = {
    // Zvonko: Croatian, mid-pitched, clear
    {"zvonko", VoiceLanguage::Croatian, 112.0f, 1.00f, 0.60f, 0.0f, 0.020f,
     1.00f, 1.00f, 1.10f, 0.90f, 1.03f, 0.97f, 0.93f},
    // Stojan: Serbian, lower and darker, slightly brisker
    {"stojan", VoiceLanguage::Serbian, 98.0f, 0.965f, 0.56f, 1.5f, 0.015f,
     0.95f, 1.04f, 1.20f, 0.90f, 0.94f, 1.00f, 1.00f},
    // Mirsad: Bosnian, higher and softer, more melodic, slower
    {"mirsad", VoiceLanguage::Bosnian, 124.0f, 1.02f, 0.64f, 0.5f, 0.030f,
     1.15f, 0.96f, 1.30f, 1.20f, 0.96f, 1.00f, 1.00f},
};

constexpr float SCHWA_F[3] = {500.0f, 1380.0f, 2600.0f};

// Output level, set so the formant voices are about as loud as the recorded
// ones. Samples below LIMIT_KNEE pass unchanged; the few peaks above it are
// rounded off instead of clipped.
constexpr float OUTPUT_GAIN = 0.28f;
constexpr float LIMIT_KNEE = 0.8f;

// Longest stretch synthesized in one go (see synthesize_clause).
constexpr size_t MAX_GROUP_WORDS = 48;
constexpr size_t MAX_GROUP_CHARS = 600;

// =============================================================================
// Segments
// =============================================================================

enum class SegKind : uint8_t {
    Plain,
    Closure,        // stop or affricate occlusion
    Release         // burst (stops) or friction (affricates)
};

struct Seg {
    Ph ph = Ph::SIL;
    SegKind kind = SegKind::Plain;
    int phone = -1;
    float dur = 0.0f;
    float start = 0.0f;

    bool has_target = false;    // vowel, glide, liquid, nasal
    bool abrupt = false;        // keeps its own formants up to the edge
    bool voiced = false;
    int rank = 0;
    float tr_out = 40.0f;

    float tgt[3] = {0, 0, 0};
    float anchor[3] = {0, 0, 0};
    float coart[3] = {1, 1, 1};
    float self[3] = {0, 0, 0};
    float lb[3] = {0, 0, 0};
    float rb[3] = {0, 0, 0};
    float tl[3] = {0, 0, 0};
    float tr[3] = {0, 0, 0};
    float f4 = 3400.0f;
    float bw[3] = {70, 100, 160};

    float av = 0.0f;
    float ah = 0.0f;
    float af = 0.0f;            // smoothed friction
    float burst = 0.0f;         // unsmoothed friction (sharp onset)
    float burst_decay = 0.0f;   // ms; 0 = sustained
    float burst_tail = 0.0f;    // slower second decay of a release into a pause:
    float tail_decay = 0.0f;    // its share of the burst and its time constant (ms)
    float tilt = 0.0f;
    float nasal = 0.0f;
    float f0_shift = 0.0f;      // semitones (intrinsic pitch, obstruent dip)
    NoiseSpec noise = {{0, 0, 0}, {2500, 4000, 6000}, {500, 800, 1200}, {0, 0, 0}, 0};
    bool has_noise = false;
};

struct Syllable {
    float t0 = -1.0f;           // nucleus start (ms)
    float t1 = -1.0f;           // nucleus end (ms)
    bool stressed = false;
    Accent accent = Accent::None;
    uint8_t prominence = 0;
    int word = 0;
};

// Piecewise-linear pitch movement, zero outside its own time span.
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

void smooth(std::vector<float>& track, int radius) {
    if (radius <= 0 || track.size() < 2) return;
    const int n = static_cast<int>(track.size());
    std::vector<float> prefix(track.size() + 1, 0.0f);
    for (int i = 0; i < n; ++i) {
        prefix[static_cast<size_t>(i) + 1] = prefix[static_cast<size_t>(i)] + track[static_cast<size_t>(i)];
    }
    for (int i = 0; i < n; ++i) {
        int lo = std::max(0, i - radius);
        int hi = std::min(n - 1, i + radius);
        track[static_cast<size_t>(i)] =
            (prefix[static_cast<size_t>(hi) + 1] - prefix[static_cast<size_t>(lo)]) /
            static_cast<float>(hi - lo + 1);
    }
}

// =============================================================================
// Clause builder
// =============================================================================

class ClauseBuilder {
public:
    ClauseBuilder(const FormantVoice& voice, const VoiceParams& params, const Utterance& utt)
        : m_voice(voice), m_params(params), m_utt(utt), m_ph(utt.phones) {
        m_speed = std::clamp(params.speed, 0.5f, 4.0f) * voice.tempo;
    }

    std::vector<Frame> build() {
        assign_durations();
        expand_segments();
        compute_formants();
        build_tracks();
        build_pitch();
        return std::move(m_frames);
    }

private:
    const Phone* phone_at(int i) const {
        return (i >= 0 && i < static_cast<int>(m_ph.size())) ? &m_ph[static_cast<size_t>(i)] : nullptr;
    }

    float rate(float d, float min_d) const {
        float floor_d = m_speed > 1.0f ? min_d / std::sqrt(m_speed) : min_d;
        return std::max(d / m_speed, floor_d);
    }

    // Formant transitions shorten less than segments do when speech is fast.
    float transition_rate(float t) const {
        return m_speed > 1.0f ? t / std::sqrt(m_speed) : t;
    }

    // -------------------------------------------------------------------------
    // Durations
    // -------------------------------------------------------------------------

    void assign_durations() {
        const int n = static_cast<int>(m_ph.size());
        const int last_syl = m_utt.syllable_count - 1;
        const float final_stretch =
            m_utt.kind == ClauseKind::Statement || m_utt.kind == ClauseKind::Exclamation
                ? 1.40f : 1.30f;

        // Phones after the last nucleus form the final coda.
        int last_nucleus = -1;
        for (int i = 0; i < n; ++i) {
            if (m_ph[static_cast<size_t>(i)].nucleus) last_nucleus = i;
        }

        m_dur.assign(m_ph.size(), 0.0f);
        for (int i = 0; i < n; ++i) {
            const Phone& p = m_ph[static_cast<size_t>(i)];
            const PhDef& def = ph_def(p.ph);
            const Phone* prev = phone_at(i - 1);
            const Phone* next = phone_at(i + 1);
            const Phone* next2 = phone_at(i + 2);
            const bool final_syl = p.syllable == last_syl;
            float d = def.dur;

            if (p.nucleus_tail || p.ph == Ph::SCHWA) {
                // Syllabic r is vocoid + tap + vocoid; the epenthetic schwa
                // of "bicikl" is a short vowel of its own.
                if (p.ph == Ph::R) {
                    d = def.dur;
                } else {
                    d = p.stressed ? (p.is_long ? 60.0f : 46.0f) : 38.0f;
                    if (p.nucleus_tail) d *= 0.75f;
                    if (final_syl) d *= 1.2f;
                }
            } else if (p.nucleus) {
                float factor;
                if (p.stressed) {
                    factor = p.is_long ? 1.90f : 1.35f;
                    if (p.prominence < 2) factor = p.is_long ? 1.50f : 1.15f;
                    if (p.word_syllables == 1 && p.prominence == 2) factor *= 1.12f;
                } else {
                    factor = p.is_long ? m_voice.postaccent_length : 1.0f;
                    if (p.prominence == 0) factor *= 0.90f;
                }
                if (p.word_syllables >= 6) {
                    factor *= 0.88f;
                } else if (p.word_syllables >= 4) {
                    factor *= 0.93f;
                }
                if (next && next->word == p.word && is_obstruent(next->ph) &&
                    !ph_def(next->ph).voiced) {
                    factor *= 0.93f;
                }
                if ((next && is_vowel(next->ph)) || (prev && is_vowel(prev->ph))) {
                    factor *= 0.90f;
                }
                if (next && is_consonant(next->ph) &&
                    (!next2 || is_consonant(next2->ph))) {
                    factor *= 0.93f;    // closed syllable
                }
                if (final_syl) {
                    factor *= final_stretch;
                } else if (p.syllable == last_syl - 1) {
                    factor *= 1.08f;
                }
                d = def.dur * factor;
            } else {
                float factor = 0.92f;
                bool prev_c = prev && is_consonant(prev->ph);
                bool next_c = next && is_consonant(next->ph);
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
                if (p.ph == Ph::R) d = def.dur;
            }

            m_dur[static_cast<size_t>(i)] = rate(d, def.min_dur);
        }
    }

    // -------------------------------------------------------------------------
    // Phones -> acoustic segments
    // -------------------------------------------------------------------------

    Seg& add_seg(Ph ph, SegKind kind, int phone, float dur) {
        Seg seg;
        const PhDef& def = ph_def(ph);
        seg.ph = ph;
        seg.kind = kind;
        seg.phone = phone;
        seg.dur = std::max(dur, FRAME_MS);
        seg.voiced = def.voiced;
        seg.rank = def.rank;
        seg.tr_out = transition_rate(def.tr_out);
        seg.f4 = def.f4 * m_voice.formant_scale;
        std::memcpy(seg.bw, def.bw, sizeof(seg.bw));
        seg.tilt = m_voice.tilt;
        m_segs.push_back(seg);
        return m_segs.back();
    }

    NoiseSpec noise_for(Ph ph) const {
        NoiseSpec spec = ph_def(ph).noise;
        float shift = 1.0f;
        switch (ph) {
            case Ph::CH: case Ph::DZH:
                shift = m_voice.hard_palatal_shift * m_voice.hard_affricate_shift;
                break;
            case Ph::SH: case Ph::ZH:
                shift = m_voice.hard_palatal_shift;
                break;
            case Ph::TJ: case Ph::DJ: case Ph::SJ: case Ph::ZJ:
                shift = m_voice.soft_palatal_shift;
                break;
            default:
                break;
        }
        for (int k = 0; k < NOISE_PEAKS; ++k) {
            spec.f[k] *= shift * m_voice.formant_scale;
        }
        return spec;
    }

    void expand_segments() {
        const int n = static_cast<int>(m_ph.size());
        for (int i = 0; i < n; ++i) {
            const Phone& p = m_ph[static_cast<size_t>(i)];
            const PhDef& def = ph_def(p.ph);
            const Phone* prev = phone_at(i - 1);
            const Phone* next = phone_at(i + 1);
            const float d = m_dur[static_cast<size_t>(i)];
            const bool next_vocalic = next && (is_vowel(next->ph) ||
                ph_def(next->ph).cls == PhClass::Glide ||
                ph_def(next->ph).cls == PhClass::Liquid);

            switch (def.cls) {
                case PhClass::Vowel: {
                    Seg& seg = add_seg(p.ph, SegKind::Plain, i, d);
                    seg.has_target = true;
                    float level = p.stressed ? 1.0f : (p.prominence == 0 ? 0.80f : 0.86f);
                    if (p.nucleus_tail) level *= 0.45f;     // second half of syllabic r
                    seg.av = def.av * level;
                    seg.tilt = m_voice.tilt + (p.stressed ? 0.0f : 1.5f);
                    // Intrinsic pitch: high vowels are slightly higher.
                    seg.f0_shift = (p.ph == Ph::I || p.ph == Ph::U) ? 0.5f
                                 : (p.ph == Ph::A ? -0.4f : 0.0f);
                    break;
                }

                case PhClass::Glide:
                case PhClass::Liquid: {
                    Seg& seg = add_seg(p.ph, SegKind::Plain, i, d);
                    seg.has_target = true;
                    seg.av = def.av;
                    seg.tilt = m_voice.tilt + def.tilt;
                    break;
                }

                case PhClass::Nasal: {
                    Seg& seg = add_seg(p.ph, SegKind::Plain, i, d);
                    seg.has_target = true;
                    seg.abrupt = true;
                    seg.av = def.av;
                    seg.nasal = 1.0f;
                    seg.tilt = m_voice.tilt + 4.0f + def.tilt;
                    break;
                }

                case PhClass::Tap: {
                    // /r/ is a vocalic stretch only a few dB below the
                    // vowels, interrupted by brief tongue-tip contacts of
                    // about 15 ms, each followed by a faint release
                    // transient. Between vowels there is a single contact
                    // (a tap); next to a consonant or a pause two (a short
                    // trill, period about 35 ms). A long weak stretch is
                    // heard as /l/ or /d/, a silent gap as /d/.
                    const bool prev_vowel = prev && is_vowel(prev->ph);
                    const bool next_vowel = next && is_vowel(next->ph);
                    auto vocoid = [&](float ms, float level) {
                        Seg& seg = add_seg(Ph::R, SegKind::Plain, i, rate(ms, ms * 0.5f));
                        seg.av = level;
                        seg.tilt = m_voice.tilt + 2.0f;
                        seg.f0_shift = -0.3f;
                        return std::ref(seg);
                    };
                    auto contact = [&]() {
                        Seg& seg = add_seg(Ph::R, SegKind::Plain, i, rate(18.0f, 10.0f));
                        seg.av = def.av;
                        seg.tilt = m_voice.tilt + 6.0f;
                        seg.f0_shift = -0.3f;
                    };
                    auto release = [&](float ms, float level) {
                        Seg& seg = vocoid(ms, level);
                        seg.burst = def.af;
                        seg.burst_decay = 3.0f;
                        seg.noise = noise_for(Ph::R);
                        seg.has_noise = true;
                    };

                    if (p.nucleus_tail) {
                        // syllabic r: one contact between its two vocoids
                        contact();
                    } else if (prev_vowel && next_vowel && !p.word_start) {
                        vocoid(8.0f, 0.72f);
                        contact();
                        release(12.0f, 0.72f);
                    } else if (next_vowel && prev && !p.word_start) {
                        // after a consonant: a full vocalic onset
                        // (the svarabhakti vowel of "kra"), one contact
                        vocoid(20.0f, 0.74f);
                        contact();
                        release(10.0f, 0.72f);
                    } else if (next_vowel) {
                        // at the start of a word: two contacts, or the
                        // lone vocalic onset is heard as /v/ ("rastu")
                        vocoid(prev_vowel ? 6.0f : 8.0f, 0.70f);
                        contact();
                        release(16.0f, 0.70f);
                        contact();
                        release(10.0f, 0.72f);
                    } else {
                        // before a consonant or a pause: short trill
                        vocoid(prev_vowel ? 10.0f : 18.0f, 0.68f);
                        contact();
                        release(18.0f, 0.66f);
                        contact();
                        release(next ? 16.0f : 26.0f, next ? 0.62f : 0.45f);
                    }
                    break;
                }

                case PhClass::Stop: {
                    const bool voiced = def.voiced;
                    PhClass next_cls = next ? ph_def(next->ph).cls : PhClass::Silence;
                    bool weak_release = next_cls == PhClass::Stop ||
                                        next_cls == PhClass::Affricate;
                    // BCS voiceless stops are unaspirated: short voice onset
                    // times, longest for the velar.
                    float release = voiced
                        ? (p.ph == Ph::G ? 14.0f : 8.0f)
                        : (p.ph == Ph::K ? 34.0f : (p.ph == Ph::T ? 16.0f : 14.0f));
                    // Before another stop the release is short but audible
                    // (the recorded speaker's "atka", "akta": 24-40 ms of
                    // noise 16-28 dB below the vowels).
                    if (weak_release) release = voiced ? 7.0f : 14.0f;
                    release = std::min(release, d * 0.45f);
                    float closure = std::max(d - release, 12.0f);

                    // After friction the silent gap is the only thing that
                    // separates the stop from the noise before it: when it
                    // shrinks to 10-20 ms, "st", "sk", "št", "šk", "čk" are
                    // heard as the bare fricative. Eloquence keeps 40 ms
                    // there (28 ms at a 60% faster rate, 12-16 ms at three
                    // times the rate), so the gap has a floor of its own
                    // that gives way to speed only slowly.
                    PhClass prev_cls = prev ? ph_def(prev->ph).cls : PhClass::Silence;
                    if (!voiced && (prev_cls == PhClass::Fricative ||
                                    prev_cls == PhClass::Affricate)) {
                        closure = std::max(closure, rate(44.0f, 40.0f));
                    }

                    // Before a pause there is no vowel to carry the stop, so
                    // the release has to: after the burst the friction at
                    // the place of articulation dies away slowly (over 40-55
                    // ms in the recordings, falling about 15 dB; 90 ms in
                    // Eloquence). With only the 16 ms burst, final t and k
                    // were easy to miss. The velar's is weaker and flatter.
                    const bool prepausal = !voiced && !next;
                    if (prepausal) {
                        release = rate(p.ph == Ph::K ? 64.0f : 56.0f, 36.0f);
                    }

                    Seg& cl = add_seg(p.ph, SegKind::Closure, i, closure);
                    if (voiced) {
                        // Voice bar: low-frequency murmur during the closure.
                        cl.av = 0.60f;
                        cl.tilt = 26.0f;
                        cl.f0_shift = -0.8f;
                    }
                    cl.noise = noise_for(p.ph);
                    cl.has_noise = true;

                    Seg& rel = add_seg(p.ph, SegKind::Release, i, release);
                    rel.burst = def.af * (voiced ? 0.90f : 1.0f) *
                                (weak_release ? (voiced ? 0.4f : 0.7f) : 1.0f);
                    if (prepausal) {
                        rel.burst_tail = p.ph == Ph::K ? 0.20f : 0.50f;
                        rel.tail_decay = p.ph == Ph::K ? 40.0f : 25.0f;
                        // Breath through the open glottis goes with it. It
                        // is what a released stop has and an affricate has
                        // not: without it "pet", "brat" leaned towards
                        // "peć", "brać".
                        rel.ah = 0.30f;
                    }
                    rel.burst_decay = p.ph == Ph::K || p.ph == Ph::G ? 8.0f
                                    : (p.ph == Ph::T || p.ph == Ph::D ? 5.0f : 4.0f);
                    rel.noise = noise_for(p.ph);
                    rel.has_noise = true;
                    if (voiced) {
                        // Voicing runs through the release at nearly the
                        // vowel's strength and brightness, so burst and
                        // vowel onset are one event, as in the recordings.
                        rel.av = 0.70f;
                        rel.tilt = m_voice.tilt + 3.0f;
                        rel.f0_shift = -0.8f;
                    } else if ((next_vocalic || next_cls == PhClass::Tap) && p.ph != Ph::P) {
                        // /r/ after a stop opens with a vocalic stretch, so
                        // the stop is released into it as into a vowel.
                        // /p/ has no aspiration: its click is followed by a
                        // few milliseconds of silence and then the voice
                        // sets in at once, as in Eloquence and eSpeak.
                        rel.ah = p.ph == Ph::K ? 0.45f : 0.20f;
                    }
                    break;
                }

                case PhClass::Affricate: {
                    const bool voiced = def.voiced;
                    // Voiceless: closure a little longer than the friction.
                    // Voiced: a short closure, then mostly friction.
                    float closure = std::max(d * (voiced ? 0.30f : 0.54f), 12.0f);
                    float friction = std::max(d - closure, 15.0f);

                    Seg& cl = add_seg(p.ph, SegKind::Closure, i, closure);
                    if (voiced) {
                        cl.av = 0.60f;
                        cl.tilt = 26.0f;
                        cl.f0_shift = -0.8f;
                    }
                    cl.noise = noise_for(p.ph);
                    cl.has_noise = true;

                    Seg& rel = add_seg(p.ph, SegKind::Release, i, friction);
                    rel.burst = def.af * (voiced ? 0.50f : 1.0f);
                    rel.noise = noise_for(p.ph);
                    rel.has_noise = true;
                    if (voiced) {
                        rel.av = 0.30f;
                        rel.tilt = m_voice.tilt + 10.0f;
                        rel.f0_shift = -0.8f;
                    }
                    break;
                }

                case PhClass::Fricative: {
                    Seg& seg = add_seg(p.ph, SegKind::Plain, i, d);
                    seg.noise = noise_for(p.ph);
                    seg.has_noise = true;
                    float strength = (p.ph == Ph::H || p.ph == Ph::GH) ? m_voice.h_strength : 1.0f;
                    if (def.voiced) {
                        seg.av = 0.42f;
                        seg.af = def.af * (p.ph == Ph::Z ? 0.40f : 0.50f) * strength;
                        seg.ah = def.ah * 0.45f * strength;
                        seg.tilt = m_voice.tilt + 8.0f;
                        seg.f0_shift = -0.8f;
                    } else {
                        seg.af = def.af * strength;
                        seg.ah = def.ah * strength;
                    }
                    break;
                }

                case PhClass::Silence:
                default:
                    break;
            }
        }

        float t = 0.0f;
        for (Seg& seg : m_segs) {
            seg.start = t;
            t += seg.dur;
        }
        m_total_ms = t;
    }

    // -------------------------------------------------------------------------
    // Formant targets, loci and transitions
    // -------------------------------------------------------------------------

    bool is_full_vowel(const Seg& seg) const {
        return seg.kind == SegKind::Plain && is_vowel(seg.ph) && seg.ph != Ph::SCHWA;
    }

    void vowel_target(const Seg& seg, float* out) const {
        const PhDef& def = ph_def(seg.ph);
        const Phone* p = phone_at(seg.phone);
        // Unstressed vowels are slightly centralized; BCS has no real vowel
        // reduction, so the effect is kept small.
        float central = (p && p->stressed) ? 0.0f : ((p && p->prominence == 0) ? 0.15f : 0.11f);
        for (int k = 0; k < 3; ++k) {
            float f = def.f[k] + central * (SCHWA_F[k] - def.f[k]);
            out[k] = f * m_voice.formant_scale;
        }
    }

    void compute_formants() {
        const int n = static_cast<int>(m_segs.size());
        const float scale = m_voice.formant_scale;

        // Nearest full vowel for each segment: the following one if there is
        // any, because consonants anticipate the vowel they release into.
        std::vector<int> ctx(m_segs.size(), -1);
        std::vector<int> before(m_segs.size(), -1);
        int found = -1;
        for (int i = n - 1; i >= 0; --i) {
            if (is_full_vowel(m_segs[static_cast<size_t>(i)])) found = i;
            ctx[static_cast<size_t>(i)] = found;
        }
        found = -1;
        for (int i = 0; i < n; ++i) {
            before[static_cast<size_t>(i)] = found;
            if (is_full_vowel(m_segs[static_cast<size_t>(i)])) found = i;
            if (ctx[static_cast<size_t>(i)] < 0) ctx[static_cast<size_t>(i)] = found;
        }

        // From the last segment to the first: a stop looks at what the
        // sound after it has already been given.
        for (int i = n - 1; i >= 0; --i) {
            Seg& seg = m_segs[static_cast<size_t>(i)];
            const PhDef& def = ph_def(seg.ph);

            float cv[3] = {SCHWA_F[0] * scale, SCHWA_F[1] * scale, SCHWA_F[2] * scale};
            int ci = ctx[static_cast<size_t>(i)];
            if (ci >= 0 && ci != i) {
                vowel_target(m_segs[static_cast<size_t>(ci)], cv);
            }

            // A stop is coloured by the sound it is released into. Before
            // r, l, j, v, a nasal or the vowel of syllabic r that is this
            // sound, not the vowel beyond it: coloured by the i of "pri",
            // "prvi", the burst of p sat at 1.7 kHz, where those of t and k
            // are, and was taken for them.
            if (def.cls == PhClass::Stop) {
                int ni = i + 1;
                while (ni < n && m_segs[static_cast<size_t>(ni)].phone == seg.phone) ++ni;
                if (ni < n) {
                    const Seg& into = m_segs[static_cast<size_t>(ni)];
                    if (!is_full_vowel(into) &&
                        (into.has_target || ph_def(into.ph).cls == PhClass::Tap)) {
                        for (int k = 0; k < 3; ++k) cv[k] = into.self[k];
                    }
                }
            }

            if (is_full_vowel(seg)) {
                vowel_target(seg, seg.tgt);
                for (int k = 0; k < 3; ++k) {
                    seg.anchor[k] = seg.self[k] = seg.tgt[k];
                    seg.coart[k] = def.coart[k];
                }
                continue;
            }

            // Loci; the velar place follows the vowel (front before front
            // vowels, back before back vowels), with F2 and F3 close together.
            // A vowel that lies behind another consonant has no hold on it:
            // the k of "disk" is neutral (as Eloquence's in "risk", "desk"),
            // not the fronted one of "ki", whose burst is close to a /t/'s.
            float loc[3] = {def.loc[0] * scale, def.loc[1] * scale, def.loc[2] * scale};
            const bool velar = seg.ph == Ph::K || seg.ph == Ph::G || seg.ph == Ph::NG;
            if (velar && ci >= 0 && ci < i &&
                m_segs[static_cast<size_t>(ci)].phone < seg.phone - 1) {
                for (int k = 0; k < 3; ++k) cv[k] = SCHWA_F[k] * scale;
            }
            if (velar) {
                loc[1] = std::clamp(1500.0f * scale + (cv[1] - 700.0f * scale) * 0.55f,
                                    1450.0f * scale, 2350.0f * scale);
                loc[2] = cv[1] > 1500.0f * scale ? loc[1] + 450.0f * scale : 2050.0f * scale;
            }

            if (seg.has_target) {
                // Sonorants take colour from the vowels on both sides; /l/
                // mostly continues the vowel before it.
                float mix[3] = {cv[0], cv[1], cv[2]};
                int bi = before[static_cast<size_t>(i)];
                if (bi >= 0 && ci > i) {
                    float pv[3];
                    vowel_target(m_segs[static_cast<size_t>(bi)], pv);
                    float w = seg.ph == Ph::L ? 0.7f : 0.5f;
                    for (int k = 0; k < 3; ++k) {
                        mix[k] = w * pv[k] + (1.0f - w) * cv[k];
                    }
                }
                for (int k = 0; k < 3; ++k) {
                    float base = def.f[k] * scale;
                    float colour = k == 0 ? def.tgt_coart * 0.5f : def.tgt_coart;
                    seg.tgt[k] = base + colour * (mix[k] - base);
                    seg.coart[k] = def.coart[k];
                    if (def.cls == PhClass::Nasal) {
                        seg.tgt[k] = base;
                        seg.anchor[k] = loc[k];
                        seg.self[k] = loc[k] + def.coart[k] * (cv[k] - loc[k]);
                    } else {
                        seg.anchor[k] = seg.self[k] = seg.tgt[k];
                    }
                }
            } else {
                for (int k = 0; k < 3; ++k) {
                    seg.anchor[k] = loc[k];
                    seg.coart[k] = def.coart[k];
                    seg.self[k] = loc[k] + def.coart[k] * (cv[k] - loc[k]);
                }
                // Before front vowels the velar burst sits where F2 and F3
                // meet, so both carry it.
                if (seg.has_noise && (seg.ph == Ph::K || seg.ph == Ph::G) &&
                    cv[1] > 1500.0f * scale) {
                    seg.noise.fa[1] = 0.9f;
                }
            }
        }

        // Boundary values and transition times
        for (int i = 0; i < n; ++i) {
            Seg& seg = m_segs[static_cast<size_t>(i)];
            for (int k = 0; k < 3; ++k) {
                seg.lb[k] = seg.rb[k] = seg.self[k];
            }
        }
        for (int i = 0; i + 1 < n; ++i) {
            Seg& x = m_segs[static_cast<size_t>(i)];
            Seg& y = m_segs[static_cast<size_t>(i) + 1];
            for (int k = 0; k < 3; ++k) {
                float speedup = k == 0 ? 0.6f : 1.0f;
                if (x.rank == y.rank) {
                    float v = 0.5f * (x.self[k] + y.self[k]);
                    x.rb[k] = v;
                    y.lb[k] = v;
                    x.tr[k] = 0.45f * x.dur;
                    y.tl[k] = 0.45f * y.dur;
                } else {
                    Seg& dom = x.rank > y.rank ? x : y;
                    Seg& other = x.rank > y.rank ? y : x;
                    float v = dom.anchor[k] + dom.coart[k] * (other.self[k] - dom.anchor[k]);
                    float t_other = dom.tr_out * speedup;
                    float t_dom = std::min(0.45f * dom.dur, 35.0f);
                    // The tongue tip makes its contact for /l/ quickly and
                    // lets go slowly: in the recordings F2 drops into an l
                    // within 25 ms and takes 60-80 ms to leave it.
                    if (&dom == &y && y.ph == Ph::L) t_other *= 0.5f;
                    // The mouth opens fast when a stop is released: by the
                    // end of the burst F1 is more than half way to the next
                    // sound (490 Hz at the voice onset of the recorded "pa",
                    // "apa" against 330 Hz here), and the rest follows
                    // within a few milliseconds. With F1 held low the vowel
                    // swelled over 20-30 ms and p, b, d sounded soft.
                    if (k == 0 && &dom == &x && x.kind == SegKind::Release &&
                        ph_def(x.ph).cls == PhClass::Stop) {
                        v = x.anchor[0] + 0.55f * (y.self[0] - x.anchor[0]);
                        t_other = std::min(t_other, transition_rate(12.0f));
                    }
                    if (&dom == &x) {
                        x.rb[k] = v;
                        y.lb[k] = v;
                        x.tr[k] = t_dom;
                        y.tl[k] = t_other;
                    } else {
                        x.rb[k] = v;
                        y.lb[k] = v;
                        x.tr[k] = t_other;
                        y.tl[k] = t_dom;
                    }
                }
            }
        }
        for (Seg& seg : m_segs) {
            for (int k = 0; k < 3; ++k) {
                if (seg.abrupt) {
                    seg.lb[k] = seg.rb[k] = seg.tgt[k];
                }
                float sum = seg.tl[k] + seg.tr[k];
                float room = seg.dur * 0.95f;
                if (sum > room && sum > 0.0f) {
                    seg.tl[k] *= room / sum;
                    seg.tr[k] *= room / sum;
                }
            }
        }
    }

    float formant_at(const Seg& seg, int k, float t) const {
        if (!seg.has_target) {
            float x = seg.dur > 0.0f ? t / seg.dur : 0.0f;
            return seg.lb[k] + (seg.rb[k] - seg.lb[k]) * x;
        }
        if (t < seg.tl[k]) {
            // leaving a consonant: fast at first, settling into the target
            float x = 1.0f - t / seg.tl[k];
            return seg.tgt[k] + (seg.lb[k] - seg.tgt[k]) * x * x;
        }
        float tail = seg.dur - seg.tr[k];
        if (t > tail && seg.tr[k] > 0.0f) {
            // approaching a consonant: slow at first, fastest at the closure
            float x = (t - tail) / seg.tr[k];
            return seg.tgt[k] + (seg.rb[k] - seg.tgt[k]) * x * x;
        }
        return seg.tgt[k];
    }

    // -------------------------------------------------------------------------
    // Control tracks
    // -------------------------------------------------------------------------

    void build_tracks() {
        const int lead = 3;                 // frames of silence before speech
        const int trail = 14;               // let the resonators ring out
        const int body = std::max(1, static_cast<int>(std::lround(m_total_ms / FRAME_MS)));
        const int total = lead + body + trail;
        const size_t count = static_cast<size_t>(total);

        std::vector<float> f[3], bw[3], f4(count), av(count, 0.0f), ah(count, 0.0f),
            af(count, 0.0f), burst(count, 0.0f), tilt(count, m_voice.tilt),
            nasal(count, 0.0f), level(count, 1.0f);
        for (int k = 0; k < 3; ++k) {
            f[k].assign(count, SCHWA_F[k]);
            bw[k].assign(count, 100.0f);
        }
        m_seg_of_frame.assign(count, -1);
        m_frames.assign(count, Frame{});
        std::vector<int> noise_seg(count, -1);

        // Loudness: stressed syllables stand out a little, the level drifts
        // down along the clause and drops on the last syllable of a statement.
        const int last_syl = m_utt.syllable_count - 1;
        const bool falling_end = m_utt.kind == ClauseKind::Statement;

        const int nsegs = static_cast<int>(m_segs.size());
        for (int si = 0; si < nsegs; ++si) {
            const Seg& seg = m_segs[static_cast<size_t>(si)];
            int j0 = lead + static_cast<int>(std::lround(seg.start / FRAME_MS));
            int j1 = lead + static_cast<int>(std::lround((seg.start + seg.dur) / FRAME_MS));
            j1 = std::min(j1, lead + body);
            const Phone* p = phone_at(seg.phone);
            const bool last_seg = si == nsegs - 1;

            for (int j = j0; j < j1; ++j) {
                const size_t idx = static_cast<size_t>(j);
                float t = (static_cast<float>(j - lead) + 0.5f) * FRAME_MS - seg.start;
                t = std::clamp(t, 0.0f, seg.dur);
                m_seg_of_frame[idx] = si;

                for (int k = 0; k < 3; ++k) {
                    f[k][idx] = formant_at(seg, k, t);
                    bw[k][idx] = seg.bw[k];
                }
                f4[idx] = seg.f4;
                av[idx] = seg.av;
                // The voice bar of b, d, g weakens over the second half of
                // the closure (by 10-15 dB in the recordings, 7 dB here), so
                // the release stands out against it.
                if (seg.kind == SegKind::Closure && seg.av > 0.0f &&
                    ph_def(seg.ph).cls == PhClass::Stop && seg.dur > 0.0f) {
                    av[idx] *= 1.0f - 0.55f * std::clamp(2.0f * t / seg.dur - 1.0f, 0.0f, 1.0f);
                }
                ah[idx] = seg.ah;
                // A release into a pause dies away; its last few
                // milliseconds close it off.
                float tail = 0.0f;
                if (seg.burst_tail > 0.0f) {
                    tail = std::exp(-t / seg.tail_decay) *
                           std::min(1.0f, (seg.dur - t) / 8.0f);
                    ah[idx] *= tail;
                }
                af[idx] = seg.af;
                tilt[idx] = seg.tilt;
                nasal[idx] = seg.nasal;

                if (seg.burst > 0.0f) {
                    if (seg.burst_decay > 0.0f) {
                        burst[idx] = seg.burst * std::max(std::exp(-t / seg.burst_decay),
                                                          seg.burst_tail * tail);
                    } else {
                        // affricate friction: abrupt start, short fall
                        // with the transient of the stop release in front
                        float fall = std::min(12.0f, seg.dur * 0.3f);
                        float remain = seg.dur - t;
                        float attack = t < 6.0f ? (seg.ph == Ph::C ? 1.25f : 1.5f) : 1.0f;
                        burst[idx] = seg.burst * attack * (remain < fall ? remain / fall : 1.0f);
                    }
                }
                if (seg.has_noise) noise_seg[idx] = si;

                float gain = 1.0f - 0.12f * (seg.start + t) / std::max(m_total_ms, 1.0f);
                if (p && falling_end && p->syllable == last_syl && m_utt.syllable_count > 1) {
                    gain *= 0.84f;
                    tilt[idx] += 3.0f;
                }
                // A vowel before a pause fades instead of stopping dead.
                if (last_seg && seg.av > 0.0f && seg.dur > 0.0f) {
                    float x = t / seg.dur;
                    if (x > 0.5f) gain *= 1.0f - 0.75f * (x - 0.5f) / 0.5f;
                }
                level[idx] = gain;
            }
        }

        // Frames outside any segment keep the nearest segment's vocal tract.
        for (int k = 0; k < 3; ++k) {
            for (int j = lead - 1; j >= 0; --j) {
                f[k][static_cast<size_t>(j)] = f[k][static_cast<size_t>(lead)];
            }
            for (int j = lead + body; j < total; ++j) {
                f[k][static_cast<size_t>(j)] = f[k][static_cast<size_t>(lead + body - 1)];
                bw[k][static_cast<size_t>(j)] = bw[k][static_cast<size_t>(lead + body - 1)];
            }
        }
        for (int j = 0; j < total; ++j) {
            if (j < lead) f4[static_cast<size_t>(j)] = f4[static_cast<size_t>(lead)];
            if (j >= lead + body) f4[static_cast<size_t>(j)] = f4[static_cast<size_t>(lead + body - 1)];
        }

        for (int k = 0; k < 3; ++k) {
            smooth(f[k], 2);
            smooth(bw[k], 3);
        }
        smooth(f4, 6);
        smooth(av, 2);
        smooth(ah, 3);
        // Friction starts and stops at the edge of its sound (within a few
        // milliseconds), not gradually across it: sharp edges are a large
        // part of what makes consonants easy to tell apart.
        smooth(af, 3);
        // No noise may spill into the closure of a voiceless stop: the
        // smoothing would take 6 ms off each end of a silent gap that is
        // short already.
        for (size_t j = 0; j < count; ++j) {
            int si = m_seg_of_frame[j];
            if (si >= 0 && m_segs[static_cast<size_t>(si)].kind == SegKind::Closure &&
                !m_segs[static_cast<size_t>(si)].voiced) {
                af[j] = 0.0f;
                ah[j] = 0.0f;
            }
        }
        // The voice bar of b, d, g is heavily muffled. Smoothed like the
        // rest, that muffling reached 8 ms past the release and dulled the
        // very start of the next sound; from the release on, the tilt is
        // smoothed as if the closure had the release's own.
        std::vector<float> open_tilt = tilt;
        std::vector<std::pair<int, int>> releases;
        for (int si = 0; si + 1 < nsegs; ++si) {
            const Seg& seg = m_segs[static_cast<size_t>(si)];
            if (seg.kind != SegKind::Closure || seg.av <= 0.0f) continue;
            const Seg& rel = m_segs[static_cast<size_t>(si) + 1];
            int j0 = lead + static_cast<int>(std::lround(seg.start / FRAME_MS));
            int j1 = std::min(lead + static_cast<int>(std::lround(rel.start / FRAME_MS)), total);
            for (int j = j0; j < j1; ++j) open_tilt[static_cast<size_t>(j)] = rel.tilt;
            releases.emplace_back(j1, std::min(j1 + 4, total));
        }
        smooth(tilt, 4);
        smooth(open_tilt, 4);
        for (const auto& range : releases) {
            for (int j = range.first; j < range.second; ++j) {
                tilt[static_cast<size_t>(j)] = open_tilt[static_cast<size_t>(j)];
            }
        }
        // Nasality reaches a little into the neighbouring vowels, and no
        // further: spread over 16 ms each way it blurred the edges of m and
        // n, which Eloquence and eSpeak switch within a few milliseconds.
        smooth(nasal, 3);
        smooth(level, 4);

        // Frication spectrum: silent frames take the spectrum of the next
        // noisy segment, so the filter bank is already in place at a burst.
        int upcoming = -1;
        for (int j = total - 1; j >= 0; --j) {
            if (noise_seg[static_cast<size_t>(j)] >= 0) {
                upcoming = noise_seg[static_cast<size_t>(j)];
            } else {
                noise_seg[static_cast<size_t>(j)] = upcoming;
            }
        }
        int previous = -1;
        for (int j = 0; j < total; ++j) {
            if (noise_seg[static_cast<size_t>(j)] >= 0) {
                previous = noise_seg[static_cast<size_t>(j)];
            } else {
                noise_seg[static_cast<size_t>(j)] = previous;
            }
        }

        for (int j = 0; j < total; ++j) {
            const size_t idx = static_cast<size_t>(j);
            Frame& fr = m_frames[idx];
            for (int k = 0; k < 3; ++k) {
                fr.f[k] = f[k][idx];
                fr.b[k] = bw[k][idx];
            }
            // Keep formants ordered and apart even in extreme contexts.
            fr.f[1] = std::max(fr.f[1], fr.f[0] + 200.0f);
            fr.f[2] = std::max(fr.f[2], fr.f[1] + 250.0f);
            fr.f[3] = std::max(f4[idx], fr.f[2] + 300.0f);
            fr.b[3] = 180.0f;
            fr.av = av[idx] * level[idx];
            fr.ah = ah[idx] * level[idx];
            fr.af = (af[idx] + burst[idx]) * level[idx];
            fr.tilt = tilt[idx];
            fr.nasal = nasal[idx];
            fr.oq = m_voice.open_quotient;

            int ns = noise_seg[idx];
            if (ns >= 0) {
                const NoiseSpec& spec = m_segs[static_cast<size_t>(ns)].noise;
                for (int k = 0; k < PARALLEL_FORMANTS; ++k) {
                    fr.pa[k] = spec.fa[k];
                }
                for (int k = 0; k < NOISE_PEAKS; ++k) {
                    fr.np_f[k] = spec.f[k];
                    fr.np_b[k] = spec.b[k];
                    fr.np_a[k] = spec.a[k];
                }
                fr.bypass = spec.bypass;
            }
        }

        m_lead = lead;
    }

    // -------------------------------------------------------------------------
    // Intonation
    // -------------------------------------------------------------------------

    std::vector<Syllable> collect_syllables() const {
        std::vector<Syllable> syls(static_cast<size_t>(std::max(m_utt.syllable_count, 0)));
        for (const Seg& seg : m_segs) {
            const Phone* p = phone_at(seg.phone);
            if (!p || p->syllable < 0 || !(p->nucleus || p->nucleus_tail)) continue;
            Syllable& syl = syls[static_cast<size_t>(p->syllable)];
            if (syl.t0 < 0.0f || seg.start < syl.t0) syl.t0 = seg.start;
            syl.t1 = std::max(syl.t1, seg.start + seg.dur);
            syl.word = p->word;
            syl.prominence = p->prominence;
            if (p->nucleus && p->stressed) {
                syl.stressed = true;
                syl.accent = p->accent;
            }
        }
        return syls;
    }

    void build_pitch() {
        const size_t count = m_frames.size();
        const float total = std::max(m_total_ms, 1.0f);
        std::vector<float> st(count, 0.0f);

        if (m_params.inflection_enabled) {
            std::vector<Syllable> syls = collect_syllables();
            std::vector<int> accents;
            for (size_t s = 0; s < syls.size(); ++s) {
                if (syls[s].stressed && syls[s].t0 >= 0.0f) accents.push_back(static_cast<int>(s));
            }

            const ClauseKind kind = m_utt.kind;
            const bool question = kind == ClauseKind::YesNoQuestion ||
                                  kind == ClauseKind::WhQuestion;
            float range = m_voice.pitch_range * (kind == ClauseKind::Exclamation ? 1.35f : 1.0f);
            // A lone syllable (a letter name while spelling, "da", "ne") has
            // no room for a full sentence melody: squeezed into it, the
            // movement is a steep glide that also hides the pitch steps
            // screen readers use to mark capital letters. A question keeps
            // its rise and an exclamation ("Ne!", "Stoj!") most of its fall,
            // or the punctuation could not be heard at all.
            if (syls.size() == 1 && !question) {
                range *= kind == ClauseKind::Exclamation ? 0.70f : 0.45f;
            }
            std::vector<Bump> bumps;

            // In a short clause the end is all there is to tell a question
            // from a statement; up to five words get the full final rise,
            // longer clauses gradually less.
            const int words = m_ph.empty() ? 0 : static_cast<int>(m_ph.back().word) + 1;
            const float brevity =
                std::clamp((9.0f - static_cast<float>(words)) / 4.0f, 0.4f, 1.0f);
            const int final_index = static_cast<int>(syls.size()) - 1;

            // The syllable that carries a question's rise.
            int question_syl = -1;
            if (kind == ClauseKind::YesNoQuestion && !accents.empty()) {
                question_syl = accents.back();
                if (m_utt.focus_word >= 0) {
                    for (int a : accents) {
                        if (syls[static_cast<size_t>(a)].word == m_utt.focus_word) question_syl = a;
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
                    bumps.push_back(std::move(b));
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
                bumps.push_back(std::move(b));
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
                    bumps.push_back(std::move(fall));
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
                if (!b.points.empty()) bumps.push_back(std::move(b));
            }

            // Declination: the baseline drifts down along the clause, about
            // two semitones per second, so a long clause covers more of the
            // range than a short one.
            const float drop = std::clamp(total * 0.002f, 1.3f, 6.0f);
            for (size_t j = 0; j < count; ++j) {
                float t = (static_cast<float>(static_cast<int>(j) - m_lead) + 0.5f) * FRAME_MS;
                float x = std::clamp(t / total, 0.0f, 1.0f);
                float value = drop * (0.37f - x);
                for (const Bump& bump : bumps) {
                    value += bump.at(t);
                }
                st[j] = value;
            }
        }

        // Segmental effects on pitch
        for (size_t j = 0; j < count; ++j) {
            int si = m_seg_of_frame[j];
            if (si >= 0) st[j] += m_segs[static_cast<size_t>(si)].f0_shift;
        }

        // Smooth the contour (forward and backward for zero lag): the larynx
        // cannot follow corners.
        const float a = std::exp(-FRAME_MS / 16.0f);
        for (size_t j = 1; j < count; ++j) st[j] = a * st[j - 1] + (1.0f - a) * st[j];
        for (size_t j = count - 1; j-- > 0;) st[j] = a * st[j + 1] + (1.0f - a) * st[j];

        // Pitch starts higher after a voiceless consonant and settles quickly.
        const int nsegs = static_cast<int>(m_segs.size());
        for (int si = 1; si < nsegs; ++si) {
            const Seg& seg = m_segs[static_cast<size_t>(si)];
            const Seg& before = m_segs[static_cast<size_t>(si) - 1];
            if (seg.av <= 0.0f || before.voiced || before.av > 0.0f) continue;
            int j0 = m_lead + static_cast<int>(std::lround(seg.start / FRAME_MS));
            for (int j = j0; j < static_cast<int>(count) && j < j0 + 30; ++j) {
                float t = static_cast<float>(j - j0) * FRAME_MS;
                st[static_cast<size_t>(j)] += 0.9f * std::exp(-t / 18.0f);
            }
        }

        const float base = m_voice.base_f0 * m_params.pitch * m_params.user_pitch;
        for (size_t j = 0; j < count; ++j) {
            float hz = base * std::pow(2.0f, st[j] / 12.0f);
            m_frames[j].f0 = std::clamp(hz, 45.0f, 480.0f);
        }
    }

    const FormantVoice& m_voice;
    const VoiceParams& m_params;
    const Utterance& m_utt;
    const std::vector<Phone>& m_ph;
    float m_speed = 1.0f;
    float m_total_ms = 0.0f;
    int m_lead = 0;

    std::vector<float> m_dur;
    std::vector<Seg> m_segs;
    std::vector<int> m_seg_of_frame;
    std::vector<Frame> m_frames;
};

} // namespace

// =============================================================================
// Public API
// =============================================================================

const FormantVoice* find_formant_voice(const char* voice_id) {
    if (!voice_id) {
        return nullptr;
    }
    for (const auto& voice : VOICES) {
        if (std::strcmp(voice.id, voice_id) == 0) {
            return &voice;
        }
    }
    return nullptr;
}

FormantSynthesizer::FormantSynthesizer(const FormantVoice& voice)
    : m_voice(voice), m_frontend(voice.language) {
    VoiceQuality quality;
    for (int i = 0; i < CASCADE_FIXED; ++i) {
        quality.upper_f[i] *= voice.formant_scale;
    }
    quality.breathiness = voice.breathiness;
    m_synth.set_quality(quality);
}

AudioBuffer FormantSynthesizer::synthesize_clause(const std::u32string& text,
                                                  Punctuation punct,
                                                  const VoiceParams& params) {
    AudioBuffer audio;
    audio.sample_rate = SAMPLE_RATE;
    audio.bits_per_sample = BITS_PER_SAMPLE;
    audio.channels = NUM_CHANNELS;

    // A clause without any punctuation can be arbitrarily long (a screen
    // reader may send a whole unpunctuated paragraph). It is spoken in breath
    // groups, each with continuation intonation, the last one with the
    // clause's own.
    size_t start = 0;
    while (start < text.size()) {
        size_t end = start;
        size_t words = 0;
        bool in_word = false;
        while (end < text.size() && end - start < MAX_GROUP_CHARS) {
            bool space = text[end] == U' ' || text[end] == U'\n' || text[end] == U'\t';
            if (!space && !in_word) {
                if (words == MAX_GROUP_WORDS) break;
                ++words;
            }
            in_word = !space;
            ++end;
        }
        // Never cut inside a word: back up to the last space of the group.
        if (end < text.size() && in_word && text[end] != U' ') {
            size_t space = text.rfind(U' ', end);
            if (space != std::u32string::npos && space > start) end = space;
        }
        const bool last = end >= text.size();
        append_group(text.substr(start, end - start),
                     last ? punct : Punctuation::COMMA, params, audio);
        start = end;
    }
    return audio;
}

void FormantSynthesizer::append_group(const std::u32string& text, Punctuation punct,
                                      const VoiceParams& params, AudioBuffer& audio) {
    Utterance utt = m_frontend.process(text, punct);
    if (utt.phones.empty()) {
        return;
    }

    ClauseBuilder builder(m_voice, params, utt);
    std::vector<Frame> frames = builder.build();
    if (frames.empty()) {
        return;
    }

    std::vector<float> pcm;
    m_synth.reset();
    m_synth.render(frames.data(), frames.size(), pcm);

    const float gain = OUTPUT_GAIN * std::clamp(params.volume, 0.0f, 1.0f);
    audio.samples.reserve(audio.samples.size() + pcm.size());
    for (float sample : pcm) {
        float x = sample * gain;
        float magnitude = std::abs(x);
        if (magnitude > LIMIT_KNEE) {
            float over = (magnitude - LIMIT_KNEE) / (1.0f - LIMIT_KNEE);
            magnitude = LIMIT_KNEE + (1.0f - LIMIT_KNEE) * std::tanh(over);
            x = x < 0.0f ? -magnitude : magnitude;
        }
        audio.samples.push_back(static_cast<AudioSample>(std::lround(x * 32000.0f)));
    }
}

} // namespace formant
} // namespace laprdus
