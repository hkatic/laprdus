// -*- coding: utf-8 -*-
// audio_synthesizer.hpp - Synthesis with the recorded voices
//
// Speaks a clause with a recorded (concatenative) voice: the text goes
// through the formant voices' front end (letter-to-sound, stress, clitics),
// the planner lays the recordings out in time and draws the pitch contour
// (concat_prosody.*), and the TD-PSOLA renderer (psola.*) produces the
// waveform from the analysed recordings (unit_bank.*). Rate, pitch and
// intonation are all applied by the renderer from the pitch marks of the
// recordings; no time-stretching or pitch-shifting library is involved.

#ifndef LAPRDUS_AUDIO_SYNTHESIZER_HPP
#define LAPRDUS_AUDIO_SYNTHESIZER_HPP

#include "laprdus/types.hpp"
#include "phoneme_data.hpp"
#include "unit_bank.hpp"
#include "../formant/formant_frontend.hpp"
#include <memory>
#include <string>

namespace laprdus {

/**
 * AudioSynthesizer - clause synthesis with a recorded voice.
 *
 * The recordings are analysed when the synthesizer is created (pitch marks,
 * voicing, sounding part) and again whenever the voice character pitch
 * changes, because that pitch warps the spectrum of the recordings.
 */
class AudioSynthesizer {
public:
    /**
     * Create the synthesizer and analyse the recordings.
     * @param phoneme_data Loaded recordings (must outlive the synthesizer).
     * @param language Language of the voice, for the text front end.
     */
    explicit AudioSynthesizer(const PhonemeData& phoneme_data,
                              VoiceLanguage language = VoiceLanguage::Croatian);
    ~AudioSynthesizer();

    // Non-copyable
    AudioSynthesizer(const AudioSynthesizer&) = delete;
    AudioSynthesizer& operator=(const AudioSynthesizer&) = delete;

    /**
     * Change the language of the text front end (ije, lexicon).
     */
    void set_language(VoiceLanguage language);

    /**
     * Use the user's accent entries (nullptr removes them).
     */
    void set_user_lexicon(const std::shared_ptr<const formant::UserLexicon>& lexicon);

    /**
     * Synthesize one clause.
     * @param text The clause (UTF-32), without its punctuation.
     * @param punct The punctuation that ended it (chooses the melody).
     * @param sentence_initial false when the clause goes on a sentence (the
     *        clause before it ended with a comma, semicolon or colon).
     * @return The clause's sound; no pause is appended.
     */
    AudioBuffer synthesize_clause(const std::u32string& text, Punctuation punct,
                                  bool sentence_initial = true);

    /**
     * The sound of one letter alone, for spelling by sounds (see
     * formant::Frontend::letter_sound).
     * @param letter The letter as formant::spelling_letter() gives it.
     * @return The sound; empty for an unknown letter.
     */
    AudioBuffer synthesize_letter_sound(const std::u32string& letter);

    /**
     * Generate silence of specified duration.
     * @param duration_ms Duration in milliseconds.
     * @return Audio buffer containing silence.
     */
    AudioBuffer generate_silence(uint32_t duration_ms) const;

    /**
     * Set voice parameters.
     * @param params Voice parameters (rate, pitch, volume).
     */
    void set_voice_params(const VoiceParams& params);

    /**
     * Get current voice parameters.
     */
    const VoiceParams& voice_params() const { return m_voice_params; }

    /**
     * Pitch of the voice as recorded (Hz), 0 if no recordings are loaded.
     */
    float natural_f0() const { return m_natural_f0; }

    /**
     * Spectrum warp applied to the recordings for a voice character pitch.
     */
    static float formant_warp_for_pitch(float pitch);

private:
    void ensure_bank();
    AudioBuffer render_utterance(const formant::Utterance& utt);

    const PhonemeData& m_phoneme_data;
    VoiceParams m_voice_params{};
    VoiceLanguage m_language;
    std::unique_ptr<formant::Frontend> m_frontend;
    std::shared_ptr<const formant::UserLexicon> m_user_lexicon;

    concat::UnitBank m_bank;
    float m_bank_warp = 0.0f;       // warp the bank was built with (0: not built)
    float m_natural_f0 = 0.0f;
};

} // namespace laprdus

#endif // LAPRDUS_AUDIO_SYNTHESIZER_HPP
