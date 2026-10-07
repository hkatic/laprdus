// -*- coding: utf-8 -*-
// klatt_synth.cpp - Cascade/parallel formant synthesizer implementation

#include "klatt_synth.hpp"
#include <cmath>
#include <algorithm>

namespace laprdus {
namespace formant {

namespace {

constexpr double PI = 3.14159265358979323846;
constexpr double T = 1.0 / SAMPLE_RATE;
constexpr double MAX_FREQ = SAMPLE_RATE * 0.46;

// Relative levels of the three sound sources, set so that vowels, aspiration
// and sibilants come out with natural loudness ratios at amplitude 1.0.
constexpr double VOICE_GAIN = 1.0;
constexpr double ASPIRATION_GAIN = 1.0;
constexpr double FRICATION_GAIN = 3.0;

// Coefficients are refreshed twice per frame.
constexpr int SUBFRAME = FRAME_SAMPLES / 2;

constexpr uint32_t RNG_SEED = 0x2545F491u;

// Frication noise is band-limited before it reaches the parallel bank. The
// classic formant synthesizers ran at 10-11 kHz and so had nothing above
// 5 kHz; their sibilants are soft for that reason. Two second-order sections
// (fourth-order Butterworth) keep that character without the dull, telephone
// quality of a hard 5 kHz limit.
constexpr double FRICATION_CUTOFF = 6600.0;
constexpr double FRICATION_LP_Q[2] = {0.54119610, 1.30656296};

// The voiced path (cascade output: voicing, breath, aspiration) gets a gentler
// second-order roll-off. Eloquence's vowels have nothing above 5.5 kHz; ours
// carried source brightness and breath noise up to 11 kHz, a hiss the classic
// voices never had.
constexpr double VOICE_CUTOFF = 6000.0;
constexpr double VOICE_LP_Q = 0.65;

// Bandwidths of the parallel formants F2-F4 (wider than in the cascade).
constexpr double PARALLEL_BW[PARALLEL_FORMANTS] = {190.0, 260.0, 360.0};

inline double lerp(double a, double b, double t) { return a + (b - a) * t; }

} // namespace

// =============================================================================
// Filters
// =============================================================================

void KlattSynth::Resonator::set(double freq, double bw) {
    freq = std::clamp(freq, 50.0, MAX_FREQ);
    bw = std::max(bw, 20.0);
    double r = std::exp(-PI * bw * T);
    c = -r * r;
    b = 2.0 * r * std::cos(2.0 * PI * freq * T);
    a = 1.0 - b - c;    // unity gain at DC
}

void KlattSynth::Resonator::set_bandpass(double freq, double bw) {
    freq = std::clamp(freq, 50.0, MAX_FREQ);
    bw = std::max(bw, 20.0);
    double r = std::exp(-PI * bw * T);
    double theta = 2.0 * PI * freq * T;
    c = -r * r;
    b = 2.0 * r * std::cos(theta);
    // unity gain at the resonance peak
    a = (1.0 - r) * std::sqrt(1.0 - 2.0 * r * std::cos(2.0 * theta) + r * r);
}

void KlattSynth::LowPass::set(double freq, double q) {
    double w = 2.0 * PI * freq * T;
    double alpha = std::sin(w) / (2.0 * q);
    double cw = std::cos(w);
    double a0 = 1.0 + alpha;
    b0 = (1.0 - cw) * 0.5 / a0;
    b1 = (1.0 - cw) / a0;
    a1 = -2.0 * cw / a0;
    a2 = (1.0 - alpha) / a0;
}

void KlattSynth::AntiResonator::set(double freq, double bw) {
    freq = std::clamp(freq, 50.0, MAX_FREQ);
    bw = std::max(bw, 20.0);
    double r = std::exp(-PI * bw * T);
    double rc = -r * r;
    double rb = 2.0 * r * std::cos(2.0 * PI * freq * T);
    double ra = 1.0 - rb - rc;
    a = 1.0 / ra;
    b = -rb / ra;
    c = -rc / ra;
}

// =============================================================================
// Construction / State
// =============================================================================

KlattSynth::KlattSynth() {
    set_quality(VoiceQuality{});
}

void KlattSynth::set_quality(const VoiceQuality& quality) {
    m_quality = quality;
    for (int i = 0; i < CASCADE_FIXED; ++i) {
        m_upper[i].set(m_quality.upper_f[i], m_quality.upper_b[i]);
    }
    m_nasal_pole.set(m_quality.nasal_pole, 90.0);
    m_nasal_zero.set(m_quality.nasal_pole, 90.0);
    for (int i = 0; i < 2; ++i) {
        m_fric_lp[i].set(FRICATION_CUTOFF, FRICATION_LP_Q[i]);
    }
    m_voice_lp.set(VOICE_CUTOFF, VOICE_LP_Q);
}

void KlattSynth::reset() {
    for (auto& r : m_cascade) r.clear();
    for (auto& r : m_upper) r.clear();
    for (auto& r : m_noise_bank) r.clear();
    for (auto& r : m_parallel) r.clear();
    for (auto& r : m_fric_lp) r.clear();
    m_voice_lp.clear();
    m_nasal_pole.clear();
    m_nasal_zero.clear();
    m_phase = 0.0;
    m_phase_up = m_phase_down = m_phase_sub = 0.0;
    m_reverb.clear();
    m_period_jitter = 1.0;
    m_period_shimmer = 1.0;
    m_tilt_state = 0.0;
    m_bright_x1 = 0.0;
    m_dc_x1 = m_dc_y1 = 0.0;
    m_have_prev = false;
    // Same text, same audio: restart the noise and flutter sequences too.
    m_time = 0.0;
    m_rng = RNG_SEED;
}

double KlattSynth::noise() {
    // Sum of four uniform values: cheap, close enough to Gaussian.
    double sum = 0.0;
    for (int i = 0; i < 4; ++i) {
        m_rng = m_rng * 1664525u + 1013904223u;
        sum += static_cast<double>(m_rng >> 8) * (1.0 / 8388608.0) - 1.0;
    }
    return sum * 0.5;
}

void KlattSynth::update_coefficients(const Frame& fr) {
    for (int i = 0; i < CASCADE_VARIABLE; ++i) {
        m_cascade[i].set(fr.f[i], fr.b[i]);
    }

    double zero = m_quality.nasal_pole +
        std::clamp(static_cast<double>(fr.nasal), 0.0, 1.0) *
        (m_quality.nasal_zero - m_quality.nasal_pole);
    m_nasal_zero.set(zero, 90.0);

    for (int i = 0; i < NOISE_PEAKS; ++i) {
        m_noise_bank[i].set_bandpass(fr.np_f[i], fr.np_b[i]);
    }
    for (int i = 0; i < PARALLEL_FORMANTS; ++i) {
        m_parallel[i].set_bandpass(fr.f[i + 1], PARALLEL_BW[i]);
    }

    // One-pole low-pass giving `tilt` dB of attenuation at 3 kHz.
    double tilt = std::clamp(static_cast<double>(fr.tilt), 0.0, 40.0);
    if (tilt < 0.05) {
        m_tilt_coef = 0.0;
    } else {
        double g2 = std::pow(10.0, -tilt / 10.0);
        double cw = std::cos(2.0 * PI * 3000.0 * T);
        double p = 1.0 - g2 * cw;
        double q = 1.0 - g2;
        m_tilt_coef = (p - std::sqrt(std::max(p * p - q * q, 0.0))) / q;
    }
}

// =============================================================================
// Voice source
// =============================================================================

namespace {

// KLGLOTT88 glottal flow derivative over one period (phase 0..1) with the
// closing edge (a step of +1 at ph == oq) band-limited over one sample.
inline double glottal_pulse(double ph, double oq, double dt) {
    double g = 0.0;
    if (ph < oq) {
        double x = ph / oq;
        g = 2.0 * x - 3.0 * x * x;
    }
    double d = ph - oq;
    if (d >= 0.0 && d < dt) {
        double u = 1.0 - d / dt;
        g -= 0.5 * u * u;
    } else if (d < 0.0 && d > -dt) {
        double u = 1.0 + d / dt;
        g += 0.5 * u * u;
    }
    return g;
}

// Sawtooth with the discontinuity smoothed by a polynomial (PolyBLEP), so
// it does not alias at the high notes.
inline double sawtooth(double ph, double dt) {
    double y = 2.0 * ph - 1.0;
    if (ph < dt) {
        double x = ph / dt;
        y -= x + x - x * x - 1.0;
    } else if (ph > 1.0 - dt) {
        double x = (ph - 1.0) / dt;
        y -= x * x + x + x + 1.0;
    }
    return y;
}

inline void advance(double& phase, double dt) {
    phase += dt;
    if (phase >= 1.0) phase -= 1.0;
}

} // namespace

double KlattSynth::source_sample(double f0, double oq_now, double av, bool& open_phase) {
    const VoiceQuality& q = m_quality;
    const double dt = f0 * T;
    const double oq = m_period_oq;
    const double up = std::pow(2.0, q.chorus_cents / 1200.0);
    const double down = 1.0 / up;
    double g = 0.0;

    switch (q.source) {
        case SourceKind::Glottal:
            g = glottal_pulse(m_phase, oq, dt);
            if (q.chorus > 0.0f) {
                g += q.chorus * (glottal_pulse(m_phase_up, oq, dt * up) +
                                 glottal_pulse(m_phase_down, oq, dt * down));
            }
            if (q.sub_octave > 0.0f) {
                g += q.sub_octave * glottal_pulse(m_phase_sub, oq, dt * 0.5);
            }
            break;

        case SourceKind::Organ: {
            // Stops over the 16' phase: 16', 8', 4', 2 2/3', 2', 1 3/5',
            // 1 1/3', 1'. As in MacinTalk's organ the fundamental is weak
            // and the energy sits on the fourth to eighth harmonics, where
            // the upper stops of a principal chorus put it.
            static const double HARMONIC[] = {1, 2, 4, 6, 8, 10, 12, 16};
            static const double LEVEL[] = {0.0, 0.45, 0.90, 0.70, 0.80, 0.40, 0.35, 0.22};
            for (int k = 0; k < 8; ++k) {
                double level = k == 0 ? q.sub_octave : LEVEL[k];
                if (level <= 0.0) continue;
                g += level * std::sin(2.0 * PI * HARMONIC[k] * m_phase_sub);
            }
            g *= 0.30;
            break;
        }

        case SourceKind::Strings:
        case SourceKind::Brass:
        case SourceKind::Reed:
            g = sawtooth(m_phase, dt);
            if (q.chorus > 0.0f) {
                g += q.chorus * sawtooth(m_phase_up, dt * up);
                // An accordion's second reed is only tuned sharp.
                if (q.source != SourceKind::Reed) {
                    g += q.chorus * sawtooth(m_phase_down, dt * down);
                }
            }
            if (q.sub_octave > 0.0f) {
                g += q.sub_octave * sawtooth(m_phase_sub, dt * 0.5);
            }
            if (q.source == SourceKind::Brass) {
                // The lips saturate: the louder the note, the brighter.
                double drive = 1.0 + 4.0 * std::clamp(av, 0.0, 1.0);
                g = std::tanh(drive * g) / std::tanh(drive);
            }
            g *= 0.45;
            break;
    }

    open_phase = m_phase < oq;

    advance(m_phase_up, dt * up);
    advance(m_phase_down, dt * down);
    advance(m_phase_sub, dt * 0.5);
    m_phase += dt;
    if (m_phase >= 1.0) {
        m_phase -= 1.0;
        m_period_jitter = 1.0 + q.jitter * noise() * 2.0;
        m_period_shimmer = 1.0 + q.shimmer * noise() * 2.0;
        m_period_oq = std::clamp(oq_now, 0.3, 0.9);
    }
    return g;
}

// =============================================================================
// Reverb
// =============================================================================

KlattSynth::Reverb::Reverb() {
    // Freeverb's tuning, halved for 22050 Hz.
    static const size_t COMB_LEN[COMBS] = {558, 594, 639, 678};
    static const size_t ALLPASS_LEN[ALLPASSES] = {113, 278};
    for (int i = 0; i < COMBS; ++i) comb[i].assign(COMB_LEN[i], 0.0f);
    for (int i = 0; i < ALLPASSES; ++i) allpass[i].assign(ALLPASS_LEN[i], 0.0f);
}

void KlattSynth::Reverb::clear() {
    for (int i = 0; i < COMBS; ++i) {
        std::fill(comb[i].begin(), comb[i].end(), 0.0f);
        comb_pos[i] = 0;
        comb_state[i] = 0.0;
    }
    for (int i = 0; i < ALLPASSES; ++i) {
        std::fill(allpass[i].begin(), allpass[i].end(), 0.0f);
        allpass_pos[i] = 0;
    }
}

double KlattSynth::Reverb::tick(double x) {
    constexpr double FEEDBACK = 0.82;
    constexpr double DAMP = 0.3;
    constexpr double ALLPASS_G = 0.5;
    double out = 0.0;
    for (int i = 0; i < COMBS; ++i) {
        float& cell = comb[i][comb_pos[i]];
        double y = cell;
        comb_state[i] = y * (1.0 - DAMP) + comb_state[i] * DAMP;
        cell = static_cast<float>(x + comb_state[i] * FEEDBACK);
        if (++comb_pos[i] >= comb[i].size()) comb_pos[i] = 0;
        out += y;
    }
    out *= 0.25;
    for (int i = 0; i < ALLPASSES; ++i) {
        float& cell = allpass[i][allpass_pos[i]];
        double y = cell;
        cell = static_cast<float>(out + y * ALLPASS_G);
        out = y - out;
        if (++allpass_pos[i] >= allpass[i].size()) allpass_pos[i] = 0;
    }
    return out;
}

// =============================================================================
// Render
// =============================================================================

void KlattSynth::render(const Frame* frames, size_t count, std::vector<float>& out) {
    if (!frames || count == 0) {
        return;
    }

    out.reserve(out.size() + count * FRAME_SAMPLES);

    for (size_t fi = 0; fi < count; ++fi) {
        const Frame& next = frames[fi];
        if (!m_have_prev) {
            m_prev = next;
            m_have_prev = true;
            m_period_oq = next.oq;
        }
        const Frame& prev = m_prev;

        for (int half = 0; half < 2; ++half) {
            // Filter coefficients for this half-frame, taken at its midpoint.
            double tm = (half * SUBFRAME + SUBFRAME * 0.5) / FRAME_SAMPLES;
            Frame mid;
            for (int i = 0; i < CASCADE_VARIABLE; ++i) {
                mid.f[i] = static_cast<float>(lerp(prev.f[i], next.f[i], tm));
                mid.b[i] = static_cast<float>(lerp(prev.b[i], next.b[i], tm));
            }
            for (int i = 0; i < NOISE_PEAKS; ++i) {
                mid.np_f[i] = static_cast<float>(lerp(prev.np_f[i], next.np_f[i], tm));
                mid.np_b[i] = static_cast<float>(lerp(prev.np_b[i], next.np_b[i], tm));
            }
            mid.nasal = static_cast<float>(lerp(prev.nasal, next.nasal, tm));
            mid.tilt = static_cast<float>(lerp(prev.tilt, next.tilt, tm));
            update_coefficients(mid);

            for (int s = 0; s < SUBFRAME; ++s) {
                double t = static_cast<double>(half * SUBFRAME + s + 1) / FRAME_SAMPLES;

                double f0 = lerp(prev.f0, next.f0, t);
                double av = lerp(prev.av, next.av, t);
                double ah = lerp(prev.ah, next.ah, t);
                double af = lerp(prev.af, next.af, t);

                // Slow pitch drift so sustained voicing never sounds machine-flat.
                double tt = m_time * T;
                double flutter = 1.0 + m_quality.flutter * 0.012 *
                    (std::sin(2.0 * PI * 12.7 * tt) +
                     std::sin(2.0 * PI * 7.1 * tt) +
                     std::sin(2.0 * PI * 4.7 * tt));
                m_time += 1.0;

                // ---- Voice source ----
                double f0_eff = std::clamp(f0, 40.0, 1000.0) * flutter * m_period_jitter;
                bool open_phase = false;
                double g = source_sample(f0_eff, lerp(prev.oq, next.oq, t), av, open_phase);

                m_tilt_state = (1.0 - m_tilt_coef) * g + m_tilt_coef * m_tilt_state;
                // The bare pulse falls 6 dB/octave, duller than a real modal
                // voice above 2 kHz; a first-order emphasis restores the
                // upper formants that carry consonant transitions.
                double k = m_quality.brightness;
                double bright = (m_tilt_state - k * m_bright_x1) / (1.0 - k);
                m_bright_x1 = m_tilt_state;
                double voice = bright * av * m_period_shimmer * VOICE_GAIN;

                // ---- Aspiration and breath noise (through the cascade) ----
                double n1 = noise();
                double breath = m_quality.breathiness * av;
                double asp_mod = (av > 0.01 && !open_phase) ? 0.35 : 1.0;
                double source = voice + n1 * (ah + breath) * asp_mod * ASPIRATION_GAIN;

                // ---- Cascade vocal tract ----
                double y = m_nasal_zero.tick(source);
                y = m_nasal_pole.tick(y);
                for (int i = CASCADE_FIXED - 1; i >= 0; --i) {
                    y = m_upper[i].tick(y);
                }
                for (int i = CASCADE_VARIABLE - 1; i >= 0; --i) {
                    y = m_cascade[i].tick(y);
                }
                y = m_voice_lp.tick(y);

                // ---- Parallel frication branch ----
                double fric = 0.0;
                if (af > 1e-5 || prev.af > 1e-5 || next.af > 1e-5) {
                    double n2 = noise();
                    // Voiced fricatives: noise is pulsed by the glottal cycle.
                    double fmod = (av > 0.05 && !open_phase) ? 0.45 : 1.0;
                    double src = n2 * af * fmod * FRICATION_GAIN;
                    src = m_fric_lp[1].tick(m_fric_lp[0].tick(src));
                    double sign = 1.0;
                    for (int i = 0; i < PARALLEL_FORMANTS; ++i) {
                        double amp = lerp(prev.pa[i], next.pa[i], t);
                        fric += sign * amp * m_parallel[i].tick(src);
                        sign = -sign;
                    }
                    for (int i = 0; i < NOISE_PEAKS; ++i) {
                        double amp = lerp(prev.np_a[i], next.np_a[i], t);
                        fric += sign * amp * m_noise_bank[i].tick(src);
                        sign = -sign;
                    }
                    fric += lerp(prev.bypass, next.bypass, t) * src;
                } else {
                    m_fric_lp[1].tick(m_fric_lp[0].tick(0.0));
                    for (auto& r : m_parallel) {
                        r.tick(0.0);
                    }
                    for (auto& r : m_noise_bank) {
                        r.tick(0.0);
                    }
                }

                double sample = y + fric;
                if (m_quality.reverb > 0.0f) {
                    sample += m_quality.reverb * m_reverb.tick(sample);
                }

                // DC blocker (~35 Hz)
                double hp = sample - m_dc_x1 + 0.99 * m_dc_y1;
                m_dc_x1 = sample;
                m_dc_y1 = hp;

                out.push_back(static_cast<float>(hp));
            }
        }

        m_prev = next;
    }
}

} // namespace formant
} // namespace laprdus
