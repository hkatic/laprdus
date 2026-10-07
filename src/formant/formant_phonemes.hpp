// -*- coding: utf-8 -*-
// formant_phonemes.hpp - Phoneme inventory and acoustic targets for the
// formant voices (Croatian, Serbian, Bosnian)

#ifndef LAPRDUS_FORMANT_PHONEMES_HPP
#define LAPRDUS_FORMANT_PHONEMES_HPP

#include "klatt_synth.hpp"
#include <cstdint>

namespace laprdus {
namespace formant {

/**
 * Phones of standard Croatian/Serbian/Bosnian, including the allophones the
 * synthesizer produces on its own ([ŋ], [ɕ], [ʑ], [ɣ], [dz], schwa).
 */
enum class Ph : uint8_t {
    SIL = 0,
    A, E, I, O, U,
    SCHWA,          // vocalic element of syllabic r, letter names
    J,              // j
    V,              // v - labiodental approximant [ʋ]
    L,              // l
    LJ,             // lj [ʎ]
    M, N,
    NJ,             // nj [ɲ]
    NG,             // n before k, g [ŋ]
    R,              // alveolar tap (single contact of the trill)
    P, B, T, D, K, G,
    C,              // c [ts]
    DZ,             // c before a voiced obstruent [dz]
    CH,             // č
    DZH,            // dž
    TJ,             // ć
    DJ,             // đ
    F, S, Z,
    SH,             // š
    ZH,             // ž
    SJ,             // š before ć [ɕ]
    ZJ,             // ž before đ [ʑ]
    H,              // h [x]
    GH,             // h before a voiced obstruent [ɣ]
    COUNT
};

enum class PhClass : uint8_t {
    Silence, Vowel, Glide, Liquid, Nasal, Tap, Stop, Affricate, Fricative
};

/** Word accent type. Neutral is used when only the stress position is known. */
enum class Accent : uint8_t { None, Neutral, Falling, Rising };

/**
 * Spectrum of frication noise: shares that follow the formants F2-F4, up to
 * three peaks at fixed frequencies, and a flat share.
 */
struct NoiseSpec {
    float fa[PARALLEL_FORMANTS];
    float f[NOISE_PEAKS];
    float b[NOISE_PEAKS];
    float a[NOISE_PEAKS];
    float bypass;
};

/**
 * Acoustic definition of one phone.
 *
 * f[]     steady-state formants (vowels, sonorants, nasal murmur)
 * loc[]   formant locus that neighbouring sounds are pulled towards
 * coart[] how far the boundary value follows the neighbour (0 = fixed locus,
 *         1 = neighbour's own value)
 */
struct PhDef {
    PhClass cls = PhClass::Silence;
    bool voiced = false;
    float f[3] = {490.0f, 1340.0f, 2350.0f};
    float loc[3] = {490.0f, 1340.0f, 2350.0f};
    float coart[3] = {1.0f, 1.0f, 1.0f};
    float bw[3] = {70.0f, 100.0f, 160.0f};
    float f4 = 3400.0f;
    float av = 0.0f;            // voicing level
    float ah = 0.0f;            // aspiration level
    float af = 0.0f;            // frication level
    float tilt = 0.0f;          // extra source tilt (dB), e.g. the muffled /v/
    float dur = 60.0f;          // inherent duration (ms)
    float min_dur = 25.0f;      // floor at very fast rates (ms)
    float tr_out = 40.0f;       // transition time imposed on neighbours (ms)
    int rank = 0;               // dominance in transitions
    float tgt_coart = 0.0f;     // colouring of own F2/F3 target by the context vowel
    NoiseSpec noise = {{0.0f, 0.0f, 0.0f}, {2500.0f, 4000.0f, 6000.0f},
                       {500.0f, 800.0f, 1200.0f}, {0.0f, 0.0f, 0.0f}, 0.0f};
};

const PhDef& ph_def(Ph ph);

inline bool is_vowel(Ph ph) {
    return ph_def(ph).cls == PhClass::Vowel;
}

/** Obstruents take part in voicing assimilation; sonorants and /v/ do not. */
inline bool is_obstruent(Ph ph) {
    PhClass c = ph_def(ph).cls;
    return c == PhClass::Stop || c == PhClass::Affricate || c == PhClass::Fricative;
}

inline bool is_consonant(Ph ph) {
    PhClass c = ph_def(ph).cls;
    return c != PhClass::Vowel && c != PhClass::Silence;
}

/**
 * One phone of an utterance, as produced by the text front end.
 */
struct Phone {
    Ph ph = Ph::SIL;
    bool nucleus = false;       // carries a syllable (vowel, first part of syllabic r)
    bool nucleus_tail = false;  // rest of a syllabic r (tap and second vocoid)
    bool stressed = false;
    bool is_long = false;
    bool short_glide = false;   // first half of the "ije" diphthong
    bool word_start = false;
    Accent accent = Accent::None;
    uint8_t prominence = 0;     // 0 clitic, 1 function word, 2 content word
    uint16_t word = 0;          // word index within the clause
    int16_t syllable = -1;      // syllable index within the clause
    uint8_t word_syllables = 1; // syllable count of the containing word
};

} // namespace formant
} // namespace laprdus

#endif // LAPRDUS_FORMANT_PHONEMES_HPP
