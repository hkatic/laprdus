// -*- coding: utf-8 -*-
// formant_synthesizer.hpp - Rule-based formant speech synthesis
// Phones -> durations, formant tracks, source amplitudes and intonation ->
// Klatt synthesizer

#ifndef LAPRDUS_FORMANT_SYNTHESIZER_HPP
#define LAPRDUS_FORMANT_SYNTHESIZER_HPP

#include "laprdus/types.hpp"
#include "formant_frontend.hpp"
#include "klatt_synth.hpp"
#include <string>

namespace laprdus {
namespace formant {

/**
 * Speaker definition of a formant voice. No data files are involved: the
 * voice is fully described by these parameters and the phone tables.
 */
struct FormantVoice {
    const char* id;
    VoiceLanguage language;
    float base_f0;              // Mid-range speaking pitch (Hz)
    float formant_scale;        // Vocal tract length (1.0 = reference speaker)
    float open_quotient;        // Glottal open quotient (higher = softer)
    float tilt;                 // Source spectral tilt (dB at 3 kHz)
    float breathiness;
    float pitch_range;          // Size of accent and boundary movements
    float tempo;                // Speaker's own rate relative to the reference
    float postaccent_length;    // Duration factor of unstressed long vowels
    float h_strength;           // Friction of /h/
    float hard_palatal_shift;   // Noise frequency factor of č, dž, š, ž
    float soft_palatal_shift;   // Noise frequency factor of ć, đ
    float hard_affricate_shift; // Further factor of č, dž alone: how hard they are
};

/** Find a formant voice by voice ID ("zvonko", "stojan", "mirsad"). */
const FormantVoice* find_formant_voice(const char* voice_id);

/**
 * FormantSynthesizer - synthesizes one clause at a time.
 *
 * Rate, pitch and volume are applied at the source (segment durations,
 * fundamental frequency, output gain), so there is no time-stretching or
 * pitch-shifting artefact at any setting.
 */
class FormantSynthesizer {
public:
    explicit FormantSynthesizer(const FormantVoice& voice);

    /**
     * Synthesize a clause.
     * @param text Clause text without its final punctuation mark.
     * @param punct Punctuation that ended the clause (selects intonation).
     * @param params Voice parameters (speed, pitch, user_pitch, volume, inflection).
     * @return 16-bit mono audio at SAMPLE_RATE, without a trailing pause.
     */
    AudioBuffer synthesize_clause(const std::u32string& text, Punctuation punct,
                                  const VoiceParams& params);

private:
    void append_group(const std::u32string& text, Punctuation punct,
                      const VoiceParams& params, AudioBuffer& audio);

    FormantVoice m_voice;
    Frontend m_frontend;
    KlattSynth m_synth;
};

} // namespace formant
} // namespace laprdus

#endif // LAPRDUS_FORMANT_SYNTHESIZER_HPP
