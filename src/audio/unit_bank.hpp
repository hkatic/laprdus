// -*- coding: utf-8 -*-
// unit_bank.hpp - Analysed recordings of a concatenative voice
//
// The recorded voices (Josip, Vlado) are one WAV file per sound. Before they
// can be joined, stretched or sung at another pitch, each recording is
// analysed once, when the voice is loaded:
//
//   - the DC offset is removed and the raw edges of the file are faded, so
//     a recording that was cut in the middle of a period cannot click;
//   - a pitch track (normalised autocorrelation, with the voice's median
//     pitch as a prior against octave errors) decides which parts are voiced;
//   - voiced parts get one pitch mark per period, at the main peak of the
//     low-passed waveform (the glottal closure); unvoiced parts get marks
//     every few milliseconds;
//   - the sounding part of the recording (lead and trail silence) is found,
//     and the sharpest rise in level, which in a stop is the burst that
//     must survive whatever the rate does to the rest.
//
// The PSOLA renderer (psola.hpp) works from these marks only.

#ifndef LAPRDUS_UNIT_BANK_HPP
#define LAPRDUS_UNIT_BANK_HPP

#include "laprdus/types.hpp"
#include "phoneme_data.hpp"
#include <array>
#include <cstdint>
#include <vector>

namespace laprdus {
namespace concat {

/** Spacing of the marks in unvoiced sound (samples at 22050 Hz, 5 ms). */
constexpr int UNVOICED_HOP = 110;

/** One analysed recording. */
struct Unit {
    std::vector<float> samples;     // -1..1, DC-free, edges faded, formant-warped
    std::vector<int32_t> marks;     // analysis marks, ascending
    std::vector<int32_t> period;    // local period at each mark (samples)
    std::vector<uint8_t> voiced;    // 1 if the mark lies in voiced sound
    int32_t lead = 0;               // first sounding sample
    int32_t trail = 0;              // one past the last sounding sample
    int32_t onset = 0;              // sharpest rise in level: the burst of a stop
    float f0 = 0.0f;                // median pitch of the voiced marks (Hz), 0 if unvoiced
    float rms = 0.0f;               // level of the sounding part
    bool loaded = false;

    int32_t length() const { return static_cast<int32_t>(samples.size()); }
    bool has_voicing() const { return f0 > 0.0f; }
    int32_t sounding_length() const { return trail - lead; }
};

/**
 * UnitBank - the analysed recordings of one voice.
 *
 * build() analyses the recordings of a PhonemeData. A formant warp other
 * than 1.0 resamples every recording first, scaling its whole spectrum by
 * the factor (1.2 makes the vocal tract sound a fifth shorter); the voice
 * character of the derived voices (child, grandmother) comes from this. The
 * pitch itself is not fixed here: the renderer sets it from the prosody.
 */
class UnitBank {
public:
    UnitBank();

    /**
     * Analyse the recordings.
     * @param data Loaded recordings.
     * @param formant_warp Spectrum scale factor (1.0 = as recorded).
     * @return true if at least one recording was analysed.
     */
    bool build(const PhonemeData& data, float formant_warp);

    const Unit& unit(Phoneme phoneme) const;

    /** Median pitch of the vowels after warping (Hz); 0 if nothing is loaded. */
    float base_f0() const { return m_base_f0; }

    float formant_warp() const { return m_warp; }

    bool loaded() const { return m_loaded; }

private:
    std::array<Unit, static_cast<size_t>(Phoneme::COUNT)> m_units;
    float m_warp = 1.0f;
    float m_base_f0 = 0.0f;
    bool m_loaded = false;
};

/**
 * Analyse one recording. Exposed for tests and tools.
 * @param samples Raw samples.
 * @param sample_rate Sample rate of the recording.
 * @param formant_warp Spectrum scale factor.
 * @param f0_prior Expected pitch of the voice (Hz) or 0 if unknown.
 */
Unit analyse_unit(span<const AudioSample> samples, uint32_t sample_rate,
                  float formant_warp, float f0_prior);

} // namespace concat
} // namespace laprdus

#endif // LAPRDUS_UNIT_BANK_HPP
