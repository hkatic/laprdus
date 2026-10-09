// -*- coding: utf-8 -*-
// klatt_synth.hpp - Cascade/parallel formant synthesizer (Klatt-style)
// Turns a stream of control frames into audio samples

#ifndef LAPRDUS_FORMANT_KLATT_SYNTH_HPP
#define LAPRDUS_FORMANT_KLATT_SYNTH_HPP

#include "laprdus/types.hpp"
#include <vector>
#include <cstdint>

namespace laprdus {
namespace formant {

// One control frame every 44 samples (~2 ms at 22050 Hz). Parameters are
// interpolated between frames, so tracks only need to be this fine.
constexpr int FRAME_SAMPLES = 44;
constexpr float FRAME_MS = 1000.0f * FRAME_SAMPLES / SAMPLE_RATE;

constexpr int NOISE_PEAKS = 3;         // fixed-frequency frication peaks (sibilants)
constexpr int PARALLEL_FORMANTS = 3;   // frication shaped by F2, F3, F4 (bursts, /h/)
constexpr int CASCADE_VARIABLE = 4;   // F1..F4 move, the rest are fixed per voice
constexpr int CASCADE_FIXED = 4;      // F5..F8

/**
 * Control frame: everything the synthesizer needs for one instant.
 * Amplitudes are linear (0 = off, 1 = nominal full level).
 */
struct Frame {
    float f0 = 110.0f;                  // Fundamental frequency (Hz)
    float av = 0.0f;                    // Voicing amplitude
    float ah = 0.0f;                    // Aspiration amplitude (noise through cascade)
    float af = 0.0f;                    // Frication amplitude (noise through parallel bank)
    float f[CASCADE_VARIABLE] = {500.0f, 1500.0f, 2500.0f, 3500.0f};
    float b[CASCADE_VARIABLE] = {60.0f, 90.0f, 150.0f, 250.0f};
    float nasal = 0.0f;                 // Nasal coupling 0..1 (separates zero from pole)
    float tilt = 0.0f;                  // Extra source spectral tilt (dB down at 3 kHz)
    float oq = 0.6f;                    // Glottal open quotient
    float pa[PARALLEL_FORMANTS] = {0.0f, 0.0f, 0.0f};        // Frication share at F2, F3, F4
    float np_f[NOISE_PEAKS] = {2500.0f, 4000.0f, 6000.0f};   // Fixed frication peaks
    float np_b[NOISE_PEAKS] = {500.0f, 800.0f, 1200.0f};
    float np_a[NOISE_PEAKS] = {0.0f, 0.0f, 0.0f};
    float bypass = 0.0f;                // Flat (unfiltered) frication share
};

/**
 * What the voiced sound source is. The speaking voices use the glottal
 * pulse; the singing presets replace it with an instrument. Every source
 * goes through the same cascade vocal tract, so the words stay audible.
 */
enum class SourceKind : uint8_t {
    Glottal,    // KLGLOTT88 pulse: the speaking voice
    Organ,      // pipe organ: a steady stack of partials over a 16' sub-octave
    Strings,    // bowed strings: sawtooth
    Brass,      // sawtooth that saturates as it gets louder (lip buzz)
    Reed        // accordion: sawtooth plus a second reed a few cents sharp
};

/**
 * Per-voice constants of the vocal tract and voice source.
 */
struct VoiceQuality {
    // F5 sits where Eloquence's does (a fixed resonance near 3.9 kHz in
    // every vowel), the rest are spread up to the voiced-path cut-off.
    float upper_f[CASCADE_FIXED] = {3950.0f, 4900.0f, 6000.0f, 7200.0f};
    float upper_b[CASCADE_FIXED] = {260.0f, 420.0f, 700.0f, 900.0f};
    float nasal_pole = 270.0f;          // Hz
    float nasal_zero = 450.0f;          // Hz, zero position at full coupling
    float brightness = 0.72f;           // Source high-frequency emphasis (0..0.9)
    float breathiness = 0.02f;          // Glottal noise mixed into voicing
    float flutter = 0.2f;               // Slow quasi-random F0 drift (0..1)
    float jitter = 0.004f;              // Period-to-period F0 perturbation
    float shimmer = 0.03f;              // Period-to-period amplitude perturbation

    // Singing presets. chorus adds two copies of the source detuned by
    // chorus_cents up and down (a section of players, a klapa); sub_octave
    // adds one an octave down (the bass singer, the 16' organ stop); reverb
    // is the wet level of a small hall.
    SourceKind source = SourceKind::Glottal;
    float chorus = 0.0f;
    float chorus_cents = 8.0f;
    float sub_octave = 0.0f;
    float reverb = 0.0f;
};

/**
 * KlattSynth - cascade/parallel formant synthesizer.
 *
 * Voiced sound and aspiration go through a cascade of resonators (nasal
 * pole/zero pair followed by eight formants). Frication and plosive bursts
 * go through parallel band-pass resonators: three that follow F2-F4, so a
 * burst carries the same formant movement as the vowel it releases into,
 * and three at fixed frequencies for sibilant noise. The voicing
 * source is a KLGLOTT88-style glottal pulse (flow derivative) with a
 * band-limited closing edge, spectral tilt, jitter, shimmer and flutter.
 */
class KlattSynth {
public:
    KlattSynth();

    void set_quality(const VoiceQuality& quality);

    /** Reset all filter and oscillator state (start of an utterance). */
    void reset();

    /**
     * Render frames and append the samples to out.
     * Output is float, nominally within [-1, 1] for normal speech levels.
     */
    void render(const Frame* frames, size_t count, std::vector<float>& out);

private:
    struct Resonator {
        double a = 1.0, b = 0.0, c = 0.0, y1 = 0.0, y2 = 0.0;
        void set(double freq, double bw);
        void set_bandpass(double freq, double bw);
        double tick(double x) {
            double y = a * x + b * y1 + c * y2;
            y2 = y1;
            y1 = y;
            return y;
        }
        void clear() { y1 = y2 = 0.0; }
    };

    // A fixed frication peak. Above 1 kHz it is a true band-pass, with zeros
    // at 0 Hz and at the Nyquist frequency: an all-pole resonator at 4.5 kHz
    // passes low frequencies only 17 dB below its peak, and the c of "ce"
    // carried a rumble 15 dB under its hiss (Eloquence's 30 dB, recorded
    // Croatian 36 dB). The low labial peak keeps its all-pole skirt.
    struct NoisePeak {
        double a = 1.0, z = 0.0, b = 0.0, c = 0.0, x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;
        void set(double freq, double bw);
        double tick(double x) {
            double y = a * (x - z * x2) + b * y1 + c * y2;
            x2 = x1;
            x1 = x;
            y2 = y1;
            y1 = y;
            return y;
        }
        void clear() { x1 = x2 = y1 = y2 = 0.0; }
    };

    struct AntiResonator {
        double a = 1.0, b = 0.0, c = 0.0, x1 = 0.0, x2 = 0.0;
        void set(double freq, double bw);
        double tick(double x) {
            double y = a * x + b * x1 + c * x2;
            x2 = x1;
            x1 = x;
            return y;
        }
        void clear() { x1 = x2 = 0.0; }
    };

    struct LowPass {
        double b0 = 1.0, b1 = 0.0, a1 = 0.0, a2 = 0.0, x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;
        void set(double freq, double q);
        double tick(double x) {
            double y = b0 * (x + x2) + b1 * x1 - a1 * y1 - a2 * y2;
            x2 = x1;
            x1 = x;
            y2 = y1;
            y1 = y;
            return y;
        }
        void clear() { x1 = x2 = y1 = y2 = 0.0; }
    };

    // Schroeder reverberator: four parallel combs into two allpasses.
    struct Reverb {
        static constexpr int COMBS = 4;
        static constexpr int ALLPASSES = 2;
        std::vector<float> comb[COMBS];
        std::vector<float> allpass[ALLPASSES];
        size_t comb_pos[COMBS] = {0, 0, 0, 0};
        size_t allpass_pos[ALLPASSES] = {0, 0};
        double comb_state[COMBS] = {0, 0, 0, 0};
        Reverb();
        void clear();
        double tick(double x);
    };

    void update_coefficients(const Frame& fr);
    double noise();
    double source_sample(double f0_hz, double oq_now, double av, bool& open_phase);

    VoiceQuality m_quality;
    Frame m_prev;
    bool m_have_prev = false;

    Resonator m_cascade[CASCADE_VARIABLE];
    Resonator m_upper[CASCADE_FIXED];
    Resonator m_nasal_pole;
    AntiResonator m_nasal_zero;
    NoisePeak m_noise_bank[NOISE_PEAKS];
    Resonator m_parallel[PARALLEL_FORMANTS];
    LowPass m_fric_lp[2];
    LowPass m_voice_lp;

    // Voice source state. The extra oscillators carry the chorus copies
    // (up, down) and the sub-octave.
    double m_phase = 0.0;
    double m_phase_up = 0.0;
    double m_phase_down = 0.0;
    double m_phase_sub = 0.0;
    Reverb m_reverb;
    double m_period_jitter = 1.0;
    double m_period_shimmer = 1.0;
    double m_period_oq = 0.6;
    double m_tilt_state = 0.0;
    double m_tilt_coef = 0.0;
    double m_bright_x1 = 0.0;
    double m_time = 0.0;

    // Output conditioning
    double m_dc_x1 = 0.0;
    double m_dc_y1 = 0.0;

    uint32_t m_rng = 0;
};

} // namespace formant
} // namespace laprdus

#endif // LAPRDUS_FORMANT_KLATT_SYNTH_HPP
