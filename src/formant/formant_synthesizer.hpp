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
#include <vector>

namespace laprdus {
namespace formant {

/**
 * One note of a built-in song. midi 0 is a rest. A tied note continues the
 * syllable of the note before it (a melisma) instead of taking a new one.
 */
struct Note {
    int8_t midi = 0;
    bool tie = false;
    float beats = 1.0f;         // quarter notes
};

/**
 * A song the singing presets know. The notation is a space-separated list
 * of notes: pitch (C4, F#3, Bb2, R for a rest), a slash and the note value
 * (1, 2, 4, 8, 16; a trailing dot lengthens it by half), with a tilde in
 * front of a note that is sung on the syllable of the note before it:
 * "G4/4 ~A4/8 R/8". Bar lines (|) are ignored.
 */
struct Song {
    const char* title;
    const char* notation;
    float bpm;                  // quarter notes per minute at speed 1.0
};

/**
 * How a singing preset sings: which song, what the voice source is, and
 * the envelope of every note. Every preset is one of the speaking voices
 * (its vocal tract and language) with one of these.
 */
struct SingingStyle {
    const Song* song;
    SourceKind source;
    float chorus;               // detuned copies of the source (0..1)
    float chorus_cents;
    float sub_octave;           // copy an octave down (0..1)
    float reverb;               // wet level (0..1)
    float brightness;           // source high-frequency emphasis (0..0.9)
    float breathiness;
    float flutter;
    float jitter;
    float shimmer;
    float open_quotient;
    float tilt;                 // dB at 3 kHz
    float vibrato_hz;
    float vibrato_cents;        // peak deviation at inflection level 0.5
    float vibrato_delay_ms;     // vibrato grows over this time after a note starts
    float attack_ms;            // rise of every note
    float release_ms;           // fall before a rest
    float decay_ms;             // 0: sustained; else a plucked string's decay
    float retrigger_beats;      // tremolo: the string is plucked again every so often (0: once)
    float portamento_ms;        // time constant of the glide between notes
    float transpose;            // semitones added to the written notes
    float gain;                 // level relative to the speaking voice
    float f5;                   // singer's formant: F5 (Hz, 0 keeps the voice's own)
    float b5;                   // its bandwidth (Hz)
    bool dry;                   // the speaking voice itself, unchanged, singing
                                // around its own pitch (transpose is then
                                // chosen so the song's middle note sits a
                                // little above the voice's speaking pitch)
};

/** Parse a song's notation. Unparseable tokens are skipped. */
std::vector<Note> parse_song(const Song& song);

/** Frequency of a MIDI note number (69 = A4 = 440 Hz). */
float midi_hz(float midi);

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
    const SingingStyle* singing = nullptr;  // set on the singing presets only
};

/**
 * Find a formant voice by voice ID: a speaking voice ("zvonko", "stojan",
 * "mirsad") or a singing preset ("orguljas", "klapa", "trubac",
 * "harmonikas", "sevdalija", "sazlija", "pjevac", "pevac", "solist",
 * "becarac").
 */
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

    /**
     * Singing presets: start the song from its first note again. Called at
     * the start of every utterance, so each one begins the song, and the
     * clauses of one utterance continue it.
     */
    void rewind_song();

    /** True for the singing presets. */
    bool is_singing() const { return m_voice.singing != nullptr; }

    /** The user's accent entries (see UserLexicon); nullptr removes them. */
    void set_user_lexicon(const std::shared_ptr<const UserLexicon>& lexicon) {
        m_frontend.set_user_lexicon(lexicon);
    }

private:
    void append_group(const std::u32string& text, Punctuation punct,
                      const VoiceParams& params, AudioBuffer& audio);

    FormantVoice m_voice;
    Frontend m_frontend;
    KlattSynth m_synth;
    std::vector<Note> m_melody;     // singing presets: the song
    size_t m_cursor = 0;            // next note to sing
};

} // namespace formant
} // namespace laprdus

#endif // LAPRDUS_FORMANT_SYNTHESIZER_HPP
