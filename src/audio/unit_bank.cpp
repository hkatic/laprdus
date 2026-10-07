// -*- coding: utf-8 -*-
// unit_bank.cpp - Analysis of the recordings of a concatenative voice
//
// See unit_bank.hpp for what is computed and why. The methods are the
// classic ones of pitch-synchronous synthesis (Moulines & Charpentier 1990;
// MBROLA): a normalised-autocorrelation pitch track with a voice prior,
// pitch marks at the peaks of the low-passed waveform, marks at a fixed
// spacing in noise.

#include "unit_bank.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace laprdus {
namespace concat {

namespace {

constexpr float PI_F = 3.14159265358979f;

// Pitch search range of the recorded voices
constexpr float F0_MIN = 60.0f;
constexpr float F0_MAX = 400.0f;

// Pitch track
constexpr int TRACK_HOP = UNVOICED_HOP;     // 5 ms
constexpr int CORR_LENGTH = 331;            // 15 ms of correlation
constexpr float VOICED_CORRELATION = 0.60f; // normalised autocorrelation of voiced sound
constexpr float VOICED_LEVEL = 0.05f;       // of the loudest frame

// Edges and sounding part
constexpr int EDGE_FADE = 44;               // 2 ms
constexpr float SOUNDING_LEVEL = 0.02f;     // -34 dB below the loudest block

float median(std::vector<float> v) {
    if (v.empty()) return 0.0f;
    std::sort(v.begin(), v.end());
    size_t n = v.size();
    return n % 2 ? v[n / 2] : 0.5f * (v[n / 2 - 1] + v[n / 2]);
}

// Resample by a factor (output[n] = x(n * ratio)) with a windowed sinc,
// band-limited to the narrower of the two Nyquist frequencies.
std::vector<float> resample(const std::vector<float>& x, float ratio) {
    const int n_in = static_cast<int>(x.size());
    if (n_in == 0 || std::abs(ratio - 1.0f) < 1e-4f) return x;
    const int n_out = std::max(1, static_cast<int>(std::floor(static_cast<float>(n_in - 1) / ratio)) + 1);
    const float cutoff = 0.5f * std::min(1.0f, 1.0f / ratio);   // cycles per input sample
    constexpr int HALF_TAPS = 16;
    std::vector<float> out(static_cast<size_t>(n_out), 0.0f);
    for (int n = 0; n < n_out; ++n) {
        const float pos = static_cast<float>(n) * ratio;
        const int centre = static_cast<int>(std::floor(pos));
        float sum = 0.0f;
        float norm = 0.0f;
        for (int k = centre - HALF_TAPS + 1; k <= centre + HALF_TAPS; ++k) {
            const float d = pos - static_cast<float>(k);
            const float arg = 2.0f * PI_F * cutoff * d;
            float sinc = std::abs(arg) < 1e-6f ? 1.0f : std::sin(arg) / arg;
            // Hann window over the taps
            const float w = 0.5f * (1.0f + std::cos(PI_F * d / static_cast<float>(HALF_TAPS)));
            const float h = 2.0f * cutoff * sinc * w;
            norm += h;
            if (k >= 0 && k < n_in) sum += h * x[static_cast<size_t>(k)];
        }
        out[static_cast<size_t>(n)] = norm > 1e-6f ? sum / norm : 0.0f;
    }
    return out;
}

// Second-order low-pass, run forwards and backwards (zero phase)
std::vector<float> lowpass_zero_phase(const std::vector<float>& x, float cutoff_hz, float fs) {
    const float w0 = 2.0f * PI_F * cutoff_hz / fs;
    const float cw = std::cos(w0);
    const float sw = std::sin(w0);
    const float alpha = sw / (2.0f * 0.7071f);
    const float a0 = 1.0f + alpha;
    const float b0 = (1.0f - cw) / 2.0f / a0;
    const float b1 = (1.0f - cw) / a0;
    const float b2 = b0;
    const float a1 = -2.0f * cw / a0;
    const float a2 = (1.0f - alpha) / a0;

    auto run = [&](std::vector<float>& v) {
        float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;
        for (float& s : v) {
            float x0 = s;
            float y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = x0; y2 = y1; y1 = y0;
            s = y0;
        }
    };
    std::vector<float> y = x;
    run(y);
    std::reverse(y.begin(), y.end());
    run(y);
    std::reverse(y.begin(), y.end());
    return y;
}

struct Frame {
    int centre = 0;
    int lag = 0;            // best period (samples), 0 if none
    float corr = 0.0f;      // its normalised autocorrelation
    float rms = 0.0f;
    bool voiced = false;
};

// Normalised cross-correlation of the signal around centre at one lag
float ncc_at(const std::vector<float>& x, int centre, int lag) {
    const int n = static_cast<int>(x.size());
    const int start = centre - CORR_LENGTH / 2;
    double num = 0.0, e1 = 0.0, e2 = 0.0;
    for (int i = 0; i < CORR_LENGTH; ++i) {
        const int a = start + i;
        const int b = a + lag;
        const float xa = (a >= 0 && a < n) ? x[static_cast<size_t>(a)] : 0.0f;
        const float xb = (b >= 0 && b < n) ? x[static_cast<size_t>(b)] : 0.0f;
        num += static_cast<double>(xa) * xb;
        e1 += static_cast<double>(xa) * xa;
        e2 += static_cast<double>(xb) * xb;
    }
    const double den = std::sqrt(e1 * e2);
    return den > 1e-12 ? static_cast<float>(num / den) : 0.0f;
}

// Pitch track of one recording. With a prior, the search is narrowed to
// about an octave around it and a candidate near the prior wins over a
// slightly better one far from it (octave errors).
std::vector<Frame> pitch_track(const std::vector<float>& x, float fs, float f0_prior) {
    const int n = static_cast<int>(x.size());
    const int lag_min_all = std::max(2, static_cast<int>(std::floor(fs / F0_MAX)));
    const int lag_max_all = static_cast<int>(std::ceil(fs / F0_MIN));
    int lag_min = lag_min_all, lag_max = lag_max_all;
    int prior_lag = 0;
    if (f0_prior > 0.0f) {
        prior_lag = static_cast<int>(std::lround(fs / f0_prior));
        lag_min = std::max(lag_min_all, static_cast<int>(prior_lag / 1.6f));
        lag_max = std::min(lag_max_all, static_cast<int>(prior_lag * 1.6f));
    }

    std::vector<Frame> frames;
    float loudest = 0.0f;
    for (int c = TRACK_HOP / 2; c < n; c += TRACK_HOP) {
        Frame f;
        f.centre = c;
        double e = 0.0;
        int count = 0;
        for (int i = c - TRACK_HOP / 2; i < c + TRACK_HOP / 2; ++i) {
            if (i >= 0 && i < n) { e += static_cast<double>(x[static_cast<size_t>(i)]) * x[static_cast<size_t>(i)]; ++count; }
        }
        f.rms = count > 0 ? static_cast<float>(std::sqrt(e / count)) : 0.0f;
        loudest = std::max(loudest, f.rms);
        frames.push_back(f);
    }

    for (Frame& f : frames) {
        if (f.rms < VOICED_LEVEL * loudest || f.rms < 1e-4f) continue;
        std::vector<float> corr(static_cast<size_t>(lag_max + 1), -1.0f);
        int best = 0;
        for (int lag = lag_min; lag <= lag_max; ++lag) {
            corr[static_cast<size_t>(lag)] = ncc_at(x, f.centre, lag);
            if (best == 0 || corr[static_cast<size_t>(lag)] > corr[static_cast<size_t>(best)]) best = lag;
        }
        if (best == 0) continue;
        float best_corr = corr[static_cast<size_t>(best)];
        if (prior_lag > 0) {
            // Prefer the local maximum nearest the prior if it is nearly as good
            int near = 0;
            for (int lag = lag_min + 1; lag < lag_max; ++lag) {
                float c0 = corr[static_cast<size_t>(lag)];
                if (c0 < corr[static_cast<size_t>(lag - 1)] || c0 < corr[static_cast<size_t>(lag + 1)]) continue;
                if (c0 < 0.85f * best_corr) continue;
                if (near == 0 || std::abs(lag - prior_lag) < std::abs(near - prior_lag)) near = lag;
            }
            if (near > 0) {
                best = near;
                best_corr = corr[static_cast<size_t>(best)];
            }
        } else {
            // Without a prior, an octave-down error is the usual one: take
            // the half period if it correlates nearly as well.
            int half = best / 2;
            if (half >= lag_min && corr[static_cast<size_t>(half)] >= 0.90f * best_corr) {
                best = half;
                best_corr = corr[static_cast<size_t>(best)];
            }
        }
        f.lag = best;
        f.corr = best_corr;
        f.voiced = best_corr >= VOICED_CORRELATION;
        // A period far from the voice's own is a burst or noise that
        // happens to correlate, not voicing.
        if (prior_lag > 0 && (best < prior_lag / 1.45f || best > prior_lag * 1.45f)) f.voiced = false;
    }

    // A single voiced frame between unvoiced ones, or a single unvoiced
    // frame inside a voiced run, is a decision error.
    for (size_t i = 1; i + 1 < frames.size(); ++i) {
        if (frames[i].voiced && !frames[i - 1].voiced && !frames[i + 1].voiced) frames[i].voiced = false;
    }
    for (size_t i = 1; i + 1 < frames.size(); ++i) {
        if (!frames[i].voiced && frames[i - 1].voiced && frames[i + 1].voiced && frames[i].lag > 0) {
            frames[i].voiced = true;
        }
    }
    return frames;
}

// Period at a sample position, from the nearest voiced frames
int period_at(const std::vector<Frame>& frames, int pos, int fallback) {
    int best = -1;
    int best_dist = 0;
    for (size_t i = 0; i < frames.size(); ++i) {
        if (!frames[i].voiced || frames[i].lag <= 0) continue;
        int d = std::abs(frames[i].centre - pos);
        if (best < 0 || d < best_dist) { best = static_cast<int>(i); best_dist = d; }
    }
    return best >= 0 ? frames[static_cast<size_t>(best)].lag : fallback;
}

} // namespace

// =============================================================================
// Analysis of one recording
// =============================================================================

Unit analyse_unit(span<const AudioSample> samples, uint32_t sample_rate,
                  float formant_warp, float f0_prior) {
    Unit unit;
    const float fs = static_cast<float>(sample_rate);
    const int n_raw = static_cast<int>(samples.size());
    if (n_raw == 0) return unit;

    // Float, DC removed
    std::vector<float> x(static_cast<size_t>(n_raw));
    double mean = 0.0;
    for (int i = 0; i < n_raw; ++i) mean += samples[static_cast<size_t>(i)];
    mean /= n_raw;
    for (int i = 0; i < n_raw; ++i) {
        x[static_cast<size_t>(i)] = (static_cast<float>(samples[static_cast<size_t>(i)]) - static_cast<float>(mean)) / 32768.0f;
    }

    // Formant warp: the whole spectrum scaled by the factor
    if (std::abs(formant_warp - 1.0f) > 1e-3f) {
        x = resample(x, formant_warp);
    }
    const int n = static_cast<int>(x.size());

    // Edge fades: a file cut inside a period must not click
    const int fade = std::min(EDGE_FADE, n / 2);
    for (int i = 0; i < fade; ++i) {
        float w = 0.5f * (1.0f - std::cos(PI_F * (static_cast<float>(i) + 0.5f) / static_cast<float>(fade)));
        x[static_cast<size_t>(i)] *= w;
        x[static_cast<size_t>(n - 1 - i)] *= w;
    }

    // Sounding part: blocks of 2 ms above -34 dB of the loudest block
    {
        const int block = 44, hop = 22;
        std::vector<float> env;
        float peak = 0.0f;
        for (int s = 0; s < n; s += hop) {
            double e = 0.0; int c = 0;
            for (int i = s; i < s + block && i < n; ++i) { e += static_cast<double>(x[static_cast<size_t>(i)]) * x[static_cast<size_t>(i)]; ++c; }
            float r = c > 0 ? static_cast<float>(std::sqrt(e / c)) : 0.0f;
            env.push_back(r);
            peak = std::max(peak, r);
        }
        const float thr = std::max(peak * SOUNDING_LEVEL, 1e-4f);
        int first = -1, last = -1;
        for (size_t b = 0; b < env.size(); ++b) {
            if (env[b] >= thr) { if (first < 0) first = static_cast<int>(b); last = static_cast<int>(b); }
        }
        if (first < 0) {
            unit.lead = 0;
            unit.trail = 0;
        } else {
            unit.lead = first * hop;
            unit.trail = std::min(n, last * hop + block);
        }
    }

    unit.samples = std::move(x);
    unit.loaded = true;
    if (unit.trail <= unit.lead) {
        // Silence (the pause recording)
        unit.lead = unit.trail = 0;
        return unit;
    }

    // Sharpest rise in level: the burst of a stop (or affricate). Blocks of
    // 2 ms every millisecond, the rise measured over 3 ms, at least 10 dB
    // and landing within 12 dB of the loudest block.
    unit.onset = unit.lead;
    {
        const int block = 44, hop = 22;
        std::vector<float> env_db;
        float peak_db = -120.0f;
        for (int s0 = unit.lead; s0 + block <= unit.trail; s0 += hop) {
            double e = 0.0;
            for (int i = s0; i < s0 + block; ++i) e += static_cast<double>(unit.samples[static_cast<size_t>(i)]) * unit.samples[static_cast<size_t>(i)];
            float db = 10.0f * std::log10(static_cast<float>(e / block) + 1e-10f);
            env_db.push_back(db);
            peak_db = std::max(peak_db, db);
        }
        float best_rise = 10.0f;
        for (size_t b = 1; b + 3 < env_db.size(); ++b) {
            float rise = env_db[b + 3] - env_db[b - 1];
            if (rise > best_rise && env_db[b + 3] >= peak_db - 12.0f) {
                best_rise = rise;
                unit.onset = unit.lead + static_cast<int32_t>(b) * hop;
            }
        }
    }

    // Level of the sounding part
    {
        double e = 0.0;
        for (int i = unit.lead; i < unit.trail; ++i) e += static_cast<double>(unit.samples[static_cast<size_t>(i)]) * unit.samples[static_cast<size_t>(i)];
        unit.rms = static_cast<float>(std::sqrt(e / (unit.trail - unit.lead)));
    }

    // Pitch track and voicing
    const std::vector<Frame> frames = pitch_track(unit.samples, fs, f0_prior);
    const int lag_min = std::max(2, static_cast<int>(std::floor(fs / F0_MAX)));
    const int lag_max = static_cast<int>(std::ceil(fs / F0_MIN));

    // Voiced runs of frames -> sample regions
    struct Region { int begin; int end; };
    std::vector<Region> voiced_regions;
    for (size_t i = 0; i < frames.size();) {
        if (!frames[i].voiced) { ++i; continue; }
        size_t j = i;
        while (j < frames.size() && frames[j].voiced) ++j;
        Region r;
        r.begin = std::max(unit.lead, frames[i].centre - TRACK_HOP / 2);
        r.end = std::min(unit.trail, frames[j - 1].centre + TRACK_HOP / 2);
        if (r.end > r.begin) voiced_regions.push_back(r);
        i = j;
    }

    // Pitch marks: peaks of the low-passed waveform, one per period
    std::vector<int32_t> marks;
    std::vector<uint8_t> voiced;
    if (!voiced_regions.empty()) {
        // Low-passed to the first harmonics, the waveform has one peak per
        // period; with more of the first formant in it, it can have two.
        const float cutoff = f0_prior > 0.0f ? std::clamp(2.5f * f0_prior, 250.0f, 600.0f) : 400.0f;
        const std::vector<float> lp = lowpass_zero_phase(unit.samples, cutoff, fs);

        // Polarity: the side with the larger extrema in voiced sound
        float pos_sum = 0.0f, neg_sum = 0.0f;
        for (const Region& r : voiced_regions) {
            for (int i = r.begin; i < r.end; ++i) {
                float v = lp[static_cast<size_t>(i)];
                if (v > 0.0f) pos_sum += v * v; else neg_sum += v * v;
            }
        }
        const float polarity = pos_sum >= neg_sum ? 1.0f : -1.0f;
        auto value = [&](int i) { return polarity * lp[static_cast<size_t>(i)]; };

        for (const Region& r : voiced_regions) {
            // Strongest peak in the region
            int start = r.begin;
            for (int i = r.begin; i < r.end; ++i) {
                if (value(i) > value(start)) start = i;
            }
            std::vector<int> region_marks;
            region_marks.push_back(start);

            // Forwards: the best peak about one period on
            int m = start;
            while (true) {
                int T = std::clamp(period_at(frames, m, lag_min), lag_min, lag_max);
                int lo = m + static_cast<int>(0.7f * T);
                int hi = m + static_cast<int>(1.3f * T);
                if (lo >= r.end) break;
                hi = std::min(hi, r.end - 1);
                int best = lo;
                for (int i = lo; i <= hi; ++i) if (value(i) > value(best)) best = i;
                region_marks.push_back(best);
                m = best;
            }
            // Backwards
            m = start;
            while (true) {
                int T = std::clamp(period_at(frames, m, lag_min), lag_min, lag_max);
                int hi = m - static_cast<int>(0.7f * T);
                int lo = m - static_cast<int>(1.3f * T);
                if (hi < r.begin) break;
                lo = std::max(lo, r.begin);
                int best = hi;
                for (int i = lo; i <= hi; ++i) if (value(i) > value(best)) best = i;
                region_marks.push_back(best);
                m = best;
            }
            std::sort(region_marks.begin(), region_marks.end());
            for (int rm : region_marks) {
                marks.push_back(rm);
                voiced.push_back(1);
            }
        }
    }

    // Unvoiced marks at a fixed spacing wherever the sounding part is not voiced
    {
        std::vector<Region> gaps;
        int cursor = unit.lead;
        for (const Region& r : voiced_regions) {
            if (r.begin > cursor) gaps.push_back({cursor, r.begin});
            cursor = std::max(cursor, r.end);
        }
        if (unit.trail > cursor) gaps.push_back({cursor, unit.trail});
        for (const Region& g : gaps) {
            for (int m = g.begin + UNVOICED_HOP / 2; m < g.end; m += UNVOICED_HOP) {
                marks.push_back(m);
                voiced.push_back(0);
            }
            // A gap shorter than half a hop still needs a mark, or the
            // sound in it would be lost.
            if (g.end - g.begin > 0 && g.end - g.begin < UNVOICED_HOP / 2 + 1) {
                marks.push_back((g.begin + g.end) / 2);
                voiced.push_back(0);
            }
        }
    }

    // Sort marks with their flags
    std::vector<size_t> order(marks.size());
    std::iota(order.begin(), order.end(), size_t{0});
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) { return marks[a] < marks[b]; });
    unit.marks.reserve(marks.size());
    unit.voiced.reserve(marks.size());
    for (size_t k : order) {
        if (!unit.marks.empty() && marks[k] == unit.marks.back()) continue;
        unit.marks.push_back(marks[k]);
        unit.voiced.push_back(voiced[k]);
    }

    // Local period of every mark
    const size_t count = unit.marks.size();
    unit.period.assign(count, UNVOICED_HOP);
    std::vector<float> periods;
    for (size_t k = 0; k < count; ++k) {
        if (!unit.voiced[k]) continue;
        const bool prev_v = k > 0 && unit.voiced[k - 1];
        const bool next_v = k + 1 < count && unit.voiced[k + 1];
        int T;
        if (prev_v && next_v) {
            T = (unit.marks[k + 1] - unit.marks[k - 1]) / 2;
        } else if (next_v) {
            T = unit.marks[k + 1] - unit.marks[k];
        } else if (prev_v) {
            T = unit.marks[k] - unit.marks[k - 1];
        } else {
            T = period_at(frames, unit.marks[k], lag_min);
        }
        unit.period[k] = std::clamp(T, lag_min, lag_max);
        periods.push_back(static_cast<float>(unit.period[k]));
    }
    unit.f0 = periods.size() >= 2 ? fs / median(periods) : 0.0f;
    return unit;
}

// =============================================================================
// UnitBank
// =============================================================================

UnitBank::UnitBank() = default;

bool UnitBank::build(const PhonemeData& data, float formant_warp) {
    m_warp = formant_warp;
    m_loaded = false;
    m_base_f0 = 0.0f;
    for (Unit& u : m_units) u = Unit{};

    const uint32_t fs = data.sample_rate() > 0 ? data.sample_rate() : SAMPLE_RATE;

    // Pass 1: the vowels, without a prior, give the voice's pitch
    static const Phoneme VOWELS[] = {Phoneme::A, Phoneme::E, Phoneme::I, Phoneme::O, Phoneme::U};
    std::vector<float> vowel_f0;
    for (Phoneme v : VOWELS) {
        span<const AudioSample> s = data.get_phoneme(v);
        if (s.empty()) continue;
        Unit u = analyse_unit(s, fs, formant_warp, 0.0f);
        if (u.f0 > 0.0f) vowel_f0.push_back(u.f0);
    }
    const float prior = median(vowel_f0);

    // Pass 2: every recording, with the prior
    vowel_f0.clear();
    for (size_t i = 0; i < m_units.size(); ++i) {
        const Phoneme p = static_cast<Phoneme>(i);
        if (p == Phoneme::COUNT) break;
        span<const AudioSample> s = data.get_phoneme(p);
        if (s.empty()) continue;
        m_units[i] = analyse_unit(s, fs, formant_warp, prior);
        m_loaded = m_loaded || m_units[i].loaded;
        for (Phoneme v : VOWELS) {
            if (v == p && m_units[i].f0 > 0.0f) vowel_f0.push_back(m_units[i].f0);
        }
    }
    m_base_f0 = median(vowel_f0);
    if (m_base_f0 <= 0.0f) m_base_f0 = prior > 0.0f ? prior : 120.0f;
    return m_loaded;
}

const Unit& UnitBank::unit(Phoneme phoneme) const {
    static const Unit EMPTY;
    const size_t i = static_cast<size_t>(phoneme);
    if (i >= m_units.size()) return EMPTY;
    return m_units[i];
}

} // namespace concat
} // namespace laprdus
