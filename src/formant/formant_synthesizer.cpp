// -*- coding: utf-8 -*-
// formant_synthesizer.cpp - Rule-based formant speech synthesis implementation

#include "formant_synthesizer.hpp"
#include "formant_intonation.hpp"
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

// =============================================================================
// Songs and singing presets
// =============================================================================

// Traditional songs whose words and tunes are in the public domain. Every
// note carries one syllable (a tilde marks a melisma), so the first stanza
// of each song fits its tune syllable for syllable; any other text is sung
// to the same tune, one syllable per note, round and round.
//
// Croatia: "Vila Velebita" ("Oj ti vilo, vilo Velebita"; words and tune
// anonymous, first sung 1882, printed 1893). D major, 2/4, brisk, and
// without a rest from the first note to the last: one verse and the
// refrain, lines of 10, 7, 10, 6 (+ a melisma), 7, 7, 12, 7 and 7
// syllables. Taken from a public-domain two-voice MIDI transcription.
const Song SONG_VILA = {
    "Vila Velebita",
    "D4/8. E4/16 F#4/4 F#4/4 F#4/8 F#4/8 E4/8 F#4/8 A4/4 G4/4 "
    "G4/8 G4/8. F#4/16 E4/4 A4/4 A4/4 F#4/4. "
    "D4/8. E4/16 F#4/4 F#4/4 F#4/8 F#4/8 E4/8 F#4/8 A4/4 G4/4. "
    "G4/8. F#4/16 E4/4 A4/4 A4/8 ~G4/8 F#4/4 "
    "F#4/8 G4/8 A4/8 B4/4. B4/8 B4/4 A4/4 "
    "F#4/8 G4/8 A4/8 B4/4. B4/8 B4/4 A4/8. "
    "F#4/8. G4/16 A4/4 F#4/8. G4/16 A4/4 F#4/8. G4/16 A4/4 F#4/8. G4/16 A4/4 "
    "B4/8. B4/16 B4/4 D5/4 C#5/8. B4/16 A4/4 "
    "A4/8 B4/8 A4/8 G4/4 E4/4 A4/4 F#4/4.",
    113.0f};

// Slavonia: the bećarac, the traditional two-line (ten syllables a line)
// humorous song of Slavonia, Baranja and Srijem, on UNESCO's list since
// 2011. The tune is the traditional one as transcribed for the couplet
// "U mog strica osam kobasica, sedam prži, osmu strina drži" (2/4,
// quarter = 75, D minor with its raised seventh): the first line to bars
// 1-3, the second to bars 4-6, with the melismas on "mog", "sam", "ba",
// "ca", "dam", "smu", "stri", "dr" as sung. Any other couplet typed in
// lands on the same two lines.
const Song SONG_BECARAC = {
    "Be\xc4\x87" "arac",
    "F4/8 F4/16 ~E4/16 F4/8 C#4/8 "
    "F4/8 F4/16 ~G4/16 F4/16 E4/16 ~C#4/16 B3/16 "
    "C#4/8. ~B3/16 ~G3/8 "
    "D4/8 D4/16 ~B3/16 C#4/8 C#4/8 "
    "B3/8 B3/16 ~C#4/16 E4/16 ~C#4/16 B3/8 "
    "D4/8. ~C#4/16 B3/8",
    75.0f};

// Serbia: "Kreće se lađa francuska" (Branislav Milosavljević, d. 1944; the
// song of the Salonika front). A minor, 3/4, eight syllables a line, every
// line in the same rhythm. The first line is taken from a printed score;
// the other three follow the song's harmony (F-C-G7-C, C-G7-C-Dm,
// Am-E7-Am) and want checking against a recording.
const Song SONG_LADJA = {
    "Kre\xc4\x87" "e se la\xc4\x91" "a francuska",
    "E4/4 E4/4 E4/4 C5/2 C5/4 B4/2 B4/4 A4/2. "
    "A4/4 A4/4 A4/4 E5/2 E5/4 D5/2 D5/4 C5/2. "
    "C5/4 C5/4 C5/4 D5/2 D5/4 E5/2 E5/4 D5/2. "
    "C5/4 C5/4 C5/4 B4/2 B4/4 G#4/2 G#4/4 A4/2.",
    100.0f};

// Bosnia: "Kad ja pođoh na Bembašu" (traditional sevdalinka, tune printed
// by Ludvík Kuba in 1906; Sarajevo's unofficial anthem). D minor, slow;
// lines of 8, 7, 8 and 7 syllables ("bijelo" has two), the tune of the
// first line repeated for the third. Durations are those of a measured
// performance and are only approximate: the song is sung rubato. The
// breaths between the lines are left to the punctuation.
const Song SONG_BEMBASA = {
    "Kad ja po\xc4\x91" "oh na Bemba\xc5\xa1" "u",
    "A4/4 A4/4 Bb4/8 A4/8 G4/4 F4/8 E4/8 D4/2 "
    "G4/4 F4/8 F4/8 E4/4 E4/8 D4/8 D4/2 "
    "A4/4 A4/4 Bb4/8 A4/8 G4/4 F4/8 E4/8 D4/2 "
    "F4/4 E4/8 E4/8 D4/4 C#4/8 D4/8 D4/2",
    66.0f};

// Singing presets: the speaking voice of the language with an instrument
// in place of the larynx. Vibrato depth is the value at inflection level
// 0.5; the inflection slider scales it and 0 switches it off.
// song, source, chorus, cents, sub, reverb, bright, breath, flutter,
// jitter, shimmer, oq, tilt, vib Hz, vib cents, vib delay, attack, release,
// decay, retrigger, portamento, transpose, gain, f5, b5, dry
const SingingStyle STYLE_ORGULJE = {
    &SONG_VILA, SourceKind::Organ, 0.0f, 8.0f, 0.5f, 0.35f, 0.15f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.60f, 0.0f, 0.0f, 0.0f, 0.0f, 25.0f, 30.0f,
    0.0f, 0.0f, 8.0f, -12.0f, 0.55f, 0.0f, 0.0f, false};
const SingingStyle STYLE_KLAPA = {
    &SONG_VILA, SourceKind::Glottal, 0.7f, 10.0f, 0.6f, 0.20f, 0.60f, 0.03f, 0.3f,
    0.004f, 0.03f, 0.62f, 1.0f, 5.5f, 30.0f, 350.0f, 60.0f, 80.0f,
    0.0f, 0.0f, 40.0f, -12.0f, 0.55f, 0.0f, 0.0f, false};
const SingingStyle STYLE_TRUBA = {
    &SONG_LADJA, SourceKind::Brass, 0.3f, 6.0f, 0.0f, 0.10f, 0.50f, 0.0f, 0.1f,
    0.001f, 0.01f, 0.60f, 0.0f, 6.0f, 25.0f, 400.0f, 45.0f, 40.0f,
    0.0f, 0.0f, 25.0f, -5.0f, 0.60f, 0.0f, 0.0f, false};
const SingingStyle STYLE_HARMONIKA = {
    &SONG_LADJA, SourceKind::Reed, 0.9f, 14.0f, 0.0f, 0.05f, 0.30f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.60f, 0.0f, 0.0f, 0.0f, 0.0f, 15.0f, 25.0f,
    0.0f, 0.0f, 6.0f, -7.0f, 0.70f, 0.0f, 0.0f, false};
const SingingStyle STYLE_SEVDAH = {
    &SONG_BEMBASA, SourceKind::Glottal, 0.0f, 8.0f, 0.0f, 0.15f, 0.72f, 0.04f, 0.2f,
    0.004f, 0.03f, 0.66f, 1.5f, 5.5f, 60.0f, 350.0f, 50.0f, 90.0f,
    0.0f, 0.0f, 90.0f, -12.0f, 0.80f, 3100.0f, 160.0f, false};
const SingingStyle STYLE_SAZ = {
    &SONG_BEMBASA, SourceKind::Strings, 0.5f, 5.0f, 0.3f, 0.10f, 0.40f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.60f, 0.0f, 0.0f, 0.0f, 0.0f, 3.0f, 20.0f,
    350.0f, 0.5f, 5.0f, -5.0f, 1.10f, 0.0f, 0.0f, false};

// The speaking voice singing as itself: nothing in place of the larynx,
// no hall, a moderate vibrato, the song brought to the voice's own pitch.
// Phonation (open quotient, tilt, breathiness) stays the voice's; see
// make_preset().
constexpr SingingStyle dry_style(const Song* song) {
    return {song, SourceKind::Glottal, 0.0f, 8.0f, 0.0f, 0.0f, 0.72f, 0.0f, 0.2f,
            0.004f, 0.03f, 0.0f, 0.0f, 5.5f, 40.0f, 300.0f, 40.0f, 60.0f,
            0.0f, 0.0f, 60.0f, 0.0f, 0.90f, 0.0f, 0.0f, true};
}
const SingingStyle STYLE_PJEVAC = dry_style(&SONG_VILA);
// The bećarac is belted out straight, with hardly any vibrato and crisp
// steps between the notes.
const SingingStyle STYLE_BECARAC = {
    &SONG_BECARAC, SourceKind::Glottal, 0.0f, 8.0f, 0.0f, 0.0f, 0.72f, 0.0f, 0.2f,
    0.004f, 0.03f, 0.0f, 0.0f, 6.0f, 20.0f, 250.0f, 30.0f, 40.0f,
    0.0f, 0.0f, 30.0f, 0.0f, 0.95f, 0.0f, 0.0f, true};
const SingingStyle STYLE_PEVAC = dry_style(&SONG_LADJA);
const SingingStyle STYLE_SOLIST = dry_style(&SONG_BEMBASA);


struct Preset {
    const char* id;
    const char* base;
    const SingingStyle* style;
};

const Preset PRESETS[] = {
    {"orguljas", "zvonko", &STYLE_ORGULJE},
    {"klapa", "zvonko", &STYLE_KLAPA},
    {"trubac", "stojan", &STYLE_TRUBA},
    {"harmonikas", "stojan", &STYLE_HARMONIKA},
    {"sevdalija", "mirsad", &STYLE_SEVDAH},
    {"sazlija", "mirsad", &STYLE_SAZ},
    {"pjevac", "zvonko", &STYLE_PJEVAC},
    {"pevac", "stojan", &STYLE_PEVAC},
    {"solist", "mirsad", &STYLE_SOLIST},
    {"becarac", "zvonko", &STYLE_BECARAC},
};


FormantVoice make_preset(const Preset& preset) {
    const FormantVoice* base = nullptr;
    for (const auto& voice : VOICES) {
        if (std::strcmp(voice.id, preset.base) == 0) base = &voice;
    }
    FormantVoice voice = base ? *base : VOICES[0];
    voice.id = preset.id;
    voice.singing = preset.style;
    if (!preset.style->dry) {
        voice.open_quotient = preset.style->open_quotient;
        voice.tilt = preset.style->tilt;
        voice.breathiness = preset.style->breathiness;
    }
    return voice;
}

std::vector<FormantVoice> make_presets() {
    std::vector<FormantVoice> voices;
    for (const auto& preset : PRESETS) voices.push_back(make_preset(preset));
    return voices;
}

const std::vector<FormantVoice>& preset_voices() {
    static const std::vector<FormantVoice> voices = make_presets();
    return voices;
}

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

// The notes one syllable is sung on, and the rest after them.
struct SungSyllable {
    std::vector<Note> notes;    // first note and the tied ones after it
    float note_ms = 0.0f;       // total of the notes
    float rest_ms = 0.0f;
    float t0 = 0.0f;            // start of the syllable in the clause (ms)
    float sound_end = 0.0f;     // end of its last sound (its rest begins here)
    bool after_rest = true;     // first of the clause or after a rest: a fresh attack
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
    ClauseBuilder(const FormantVoice& voice, const VoiceParams& params, const Utterance& utt,
                  const std::vector<Note>* melody, size_t* cursor)
        : m_voice(voice), m_params(params), m_utt(utt), m_ph(utt.phones),
          m_style(voice.singing), m_melody(melody), m_cursor(cursor) {
        m_singing = m_style != nullptr && melody != nullptr && !melody->empty() && cursor != nullptr;
        if (m_singing) {
            // The rate sets the tempo. Consonants follow it only part of
            // the way: they are spoken, not sung.
            m_tempo = params.formant_speed();
            m_speed = std::clamp(m_tempo, 0.6f, 2.0f);
        } else {
            m_speed = params.formant_speed() * voice.tempo;
        }
    }

    std::vector<Frame> build() {
        if (m_singing) plan_notes();
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
                    // In a statement a short stressed vowel takes only half
                    // of the final lengthening, so that a clause-final
                    // monosyllable keeps the short/long contrast (sȁd : sȃd,
                    // brȁt : grȃd); with the full stretch the short one was
                    // 70% of the long one and heard as long. A question
                    // keeps the full stretch (its rise needs the time), and
                    // so does a final function word (tȍ, tȉ), whose fall
                    // would otherwise not reach its target.
                    const bool statement = m_utt.kind == ClauseKind::Statement ||
                                           m_utt.kind == ClauseKind::Exclamation;
                    factor *= p.stressed && !p.is_long && statement && p.prominence == 2
                                  ? 1.0f + (final_stretch - 1.0f) * 0.5f
                                  : final_stretch;
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

        if (m_singing) fit_durations_to_notes();
    }

    // -------------------------------------------------------------------------
    // Singing: notes
    // -------------------------------------------------------------------------

    // Take the next notes of the song for every syllable of the clause: a
    // note, the notes tied to it, and the rests that follow.
    void plan_notes() {
        const std::vector<Note>& melody = *m_melody;
        const float beat_ms = 60000.0f / (m_style->song->bpm * m_tempo);
        bool has_note = false;
        for (const Note& n : melody) has_note = has_note || n.midi > 0;
        if (!has_note) {
            m_singing = false;
            return;
        }
        auto next = [&]() -> const Note& {
            const Note& n = melody[*m_cursor % melody.size()];
            *m_cursor = (*m_cursor + 1) % melody.size();
            return n;
        };
        auto peek = [&]() -> const Note& { return melody[*m_cursor % melody.size()]; };

        m_sung.assign(static_cast<size_t>(std::max(m_utt.syllable_count, 0)), SungSyllable{});
        for (SungSyllable& syl : m_sung) {
            // Rests before a syllable's note are dropped (the clause pause
            // stands in for them); rests after it are kept.
            while (peek().midi <= 0) next();
            Note first = next();
            first.tie = false;
            syl.notes.push_back(first);
            while (peek().tie && peek().midi > 0) syl.notes.push_back(next());
            while (peek().midi <= 0 && !peek().tie) {
                syl.rest_ms += next().beats * beat_ms;
                // A rest at the very end of the song must not swallow the
                // one at its start as well.
                if (*m_cursor == 0) break;
            }
            for (const Note& n : syl.notes) syl.note_ms += n.beats * beat_ms;
        }
    }

    // Every syllable lasts as long as its notes, and its vowel starts on
    // the beat: the consonants keep their spoken durations, those that open
    // the next syllable are taken off the end of this one's vowel (or its
    // rest), as singers do (Sundberg), and the vowel takes what is left, at
    // least a third of the note.
    void fit_durations_to_notes() {
        const int n = static_cast<int>(m_ph.size());
        const size_t count = m_sung.size();
        std::vector<int> nucleus(count, -1);
        std::vector<float> onset(count, 0.0f), coda(count, 0.0f);
        for (int i = 0; i < n; ++i) {
            const Phone& p = m_ph[static_cast<size_t>(i)];
            if (p.syllable < 0 || static_cast<size_t>(p.syllable) >= count) continue;
            size_t s = static_cast<size_t>(p.syllable);
            if (p.nucleus && nucleus[s] < 0) {
                nucleus[s] = i;
            } else if (nucleus[s] < 0) {
                onset[s] += m_dur[static_cast<size_t>(i)];
            } else {
                coda[s] += m_dur[static_cast<size_t>(i)];
            }
        }
        auto scale_consonants = [&](size_t s, bool onset_part, float factor) {
            for (int i = 0; i < n; ++i) {
                const Phone& p = m_ph[static_cast<size_t>(i)];
                if (p.syllable != static_cast<int>(s) || i == nucleus[s]) continue;
                bool before = i < nucleus[s];
                if (before == onset_part) m_dur[static_cast<size_t>(i)] *= factor;
            }
            (onset_part ? onset[s] : coda[s]) *= factor;
        };

        for (size_t s = 0; s < count; ++s) {
            if (nucleus[s] < 0) continue;
            SungSyllable& syl = m_sung[s];
            const float note = syl.note_ms;
            const float next_onset = s + 1 < count ? onset[s + 1] : 0.0f;
            const float vowel_min = std::max(40.0f, 0.35f * note);
            float taken = coda[s];
            if (syl.rest_ms > 0.0f) {
                // The next syllable's consonants eat into the rest first.
                float from_rest = std::min(next_onset, syl.rest_ms);
                syl.rest_ms -= from_rest;
                taken += next_onset - from_rest;
            } else {
                taken += next_onset;
            }
            float vowel = note - taken;
            if (vowel < vowel_min && taken > 0.0f) {
                float factor = std::max(note - vowel_min, 0.0f) / taken;
                scale_consonants(s, false, factor);
                if (s + 1 < count) scale_consonants(s + 1, true, factor);
                vowel = vowel_min;
            }
            m_dur[static_cast<size_t>(nucleus[s])] = vowel;
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
                    if (m_singing && !p.nucleus_tail) level = 1.0f;   // every vowel is sung in full
                    seg.av = def.av * level;
                    seg.tilt = m_voice.tilt + (p.stressed || m_singing ? 0.0f : 1.5f);
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

            // Singing: the rest written after this syllable's notes.
            if (m_singing && p.syllable >= 0 &&
                static_cast<size_t>(p.syllable) < m_sung.size() &&
                (!next || next->syllable != p.syllable)) {
                float rest = m_sung[static_cast<size_t>(p.syllable)].rest_ms;
                if (rest >= FRAME_MS) {
                    Seg& seg = add_seg(Ph::SIL, SegKind::Plain, i, rest);
                    seg.av = 0.0f;
                }
            }
        }

        float t = 0.0f;
        for (Seg& seg : m_segs) {
            seg.start = t;
            t += seg.dur;
        }
        m_total_ms = t;

        if (m_singing) {
            // Where each syllable starts and where its sound ends, for the
            // notes and their envelopes.
            std::vector<bool> seen(m_sung.size(), false);
            for (const Seg& seg : m_segs) {
                const Phone* p = phone_at(seg.phone);
                if (!p || p->syllable < 0 || static_cast<size_t>(p->syllable) >= m_sung.size()) continue;
                SungSyllable& syl = m_sung[static_cast<size_t>(p->syllable)];
                if (!seen[static_cast<size_t>(p->syllable)]) {
                    seen[static_cast<size_t>(p->syllable)] = true;
                    syl.t0 = seg.start;
                }
                if (seg.ph != Ph::SIL) syl.sound_end = std::max(syl.sound_end, seg.start + seg.dur);
            }
            for (size_t i = 1; i < m_sung.size(); ++i) {
                m_sung[i].after_rest = m_sung[i - 1].rest_ms > 0.0f;
            }
        }
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
        float central = (p && p->stressed) || m_singing ? 0.0f : ((p && p->prominence == 0) ? 0.15f : 0.11f);
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

        // A rest between notes is silence, not a sound to glide towards:
        // the vowel before it holds its colour to the end.
        for (int i = 0; i < n; ++i) {
            Seg& seg = m_segs[static_cast<size_t>(i)];
            if (seg.ph != Ph::SIL) continue;
            if (i > 0) {
                Seg& x = m_segs[static_cast<size_t>(i) - 1];
                for (int k = 0; k < 3; ++k) {
                    x.rb[k] = x.has_target ? x.tgt[k] : x.self[k];
                    x.tr[k] = 0.0f;
                    seg.lb[k] = x.rb[k];
                }
            }
            if (i + 1 < n) {
                Seg& y = m_segs[static_cast<size_t>(i) + 1];
                for (int k = 0; k < 3; ++k) {
                    y.lb[k] = y.has_target ? y.tgt[k] : y.self[k];
                    y.tl[k] = 0.0f;
                    seg.rb[k] = y.lb[k];
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
        // Let the resonators ring out; a hall needs longer to fall silent,
        // but not so long that the next clause comes in late.
        const int trail = m_singing && m_style->reverb > 0.0f
            ? static_cast<int>(std::lround(250.0f / FRAME_MS)) : 14;
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

                float gain = 1.0f;
                if (!m_singing) {
                    gain = 1.0f - 0.12f * (seg.start + t) / std::max(m_total_ms, 1.0f);
                    if (p && falling_end && p->syllable == last_syl && m_utt.syllable_count > 1) {
                        gain *= 0.84f;
                        tilt[idx] += 3.0f;
                    }
                    // A vowel before a pause fades instead of stopping dead.
                    if (last_seg && seg.av > 0.0f && seg.dur > 0.0f) {
                        float x = t / seg.dur;
                        if (x > 0.5f) gain *= 1.0f - 0.75f * (x - 0.5f) / 0.5f;
                    }
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
            if (m_singing) {
                float extra_tilt = 0.0f;
                float env = note_envelope(j, extra_tilt);
                fr.av *= env;
                fr.ah *= env;
                fr.tilt += extra_tilt;
            }
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
    // Singing: envelope and pitch of the notes
    // -------------------------------------------------------------------------

    // Time of a frame from the start of the clause (ms).
    float frame_time(int j) const {
        return (static_cast<float>(j - m_lead) + 0.5f) * FRAME_MS;
    }

    // The syllable a frame belongs to, -1 outside speech.
    int syllable_of_frame(int j) const {
        int si = m_seg_of_frame[static_cast<size_t>(j)];
        if (si < 0) return -1;
        const Phone* p = phone_at(m_segs[static_cast<size_t>(si)].phone);
        if (!p || p->syllable < 0 || static_cast<size_t>(p->syllable) >= m_sung.size()) return -1;
        return p->syllable;
    }

    // Where in its notes a frame of a syllable is.
    struct NotePos {
        int midi = 0;
        float t_note = 0.0f;        // ms since the note (or its tied group) started
        float t_syl = 0.0f;         // ms since the syllable started
        float remaining = 0.0f;     // ms until the syllable's sound ends
        bool in_rest = false;
    };

    NotePos note_at(int s, float t) const {
        const SungSyllable& syl = m_sung[static_cast<size_t>(s)];
        NotePos pos;
        pos.t_syl = t - syl.t0;
        pos.remaining = syl.sound_end - t;
        float beat_ms = 60000.0f / (m_style->song->bpm * m_tempo);
        float at = 0.0f;
        pos.midi = syl.notes.empty() ? 60 : syl.notes.front().midi;
        for (const Note& n : syl.notes) {
            float len = n.beats * beat_ms;
            if (pos.t_syl < at + len || &n == &syl.notes.back()) {
                pos.midi = n.midi;
                break;
            }
            at += len;
        }
        pos.t_note = pos.t_syl;     // vibrato and attack run through a melisma
        pos.in_rest = t >= syl.sound_end;
        return pos;
    }

    // Attack, release and (for a plucked string) decay of a note. Notes
    // that follow each other without a rest are joined (legato): the
    // attack is only for the first note after a rest or a pause, or the
    // voice would dip to nothing between every two syllables.
    float note_envelope(int j, float& extra_tilt) const {
        int s = syllable_of_frame(j);
        if (s < 0) return 1.0f;
        NotePos pos = note_at(s, frame_time(j));
        const SungSyllable& syl = m_sung[static_cast<size_t>(s)];
        float env = m_style->gain;
        if (m_style->attack_ms > 0.0f && syl.after_rest) {
            env *= std::clamp(pos.t_syl / m_style->attack_ms, 0.0f, 1.0f);
        }
        const bool last = static_cast<size_t>(s) + 1 == m_sung.size();
        if (syl.rest_ms > 0.0f || last) {
            // Into a rest with the style's release; at the end of the
            // clause a short one, so the last vowel does not stop dead.
            float release = syl.rest_ms > 0.0f ? m_style->release_ms : std::min(m_style->release_ms, 40.0f);
            if (release > 0.0f) env *= std::clamp(pos.remaining / release, 0.0f, 1.0f);
        }
        if (m_style->decay_ms > 0.0f) {
            float since_pluck = pos.t_syl;
            if (m_style->retrigger_beats > 0.0f) {
                float period = m_style->retrigger_beats * 60000.0f / (m_style->song->bpm * m_tempo);
                since_pluck = std::fmod(pos.t_syl, period);
            }
            float decay = std::exp(-since_pluck / m_style->decay_ms);
            env *= std::max(decay, 0.02f);
            extra_tilt = 9.0f * (1.0f - decay);
        }
        return env;
    }

    // Transposition of a dry preset: the middle of the song's range five
    // semitones above the voice's speaking pitch (a voice sings a little
    // higher than it speaks; a song an octave wide then runs from about a
    // semitone below the speaking pitch to an octave above it), in whole
    // semitones so the key stays in tune with itself.
    float natural_transpose() const {
        int low = 127, high = 0;
        for (const Note& n : *m_melody) {
            if (n.midi <= 0) continue;
            low = std::min(low, static_cast<int>(n.midi));
            high = std::max(high, static_cast<int>(n.midi));
        }
        if (high == 0) return 0.0f;
        float centre = 0.5f * static_cast<float>(low + high);
        float target = 69.0f + 12.0f * std::log2(m_voice.base_f0 / 440.0f) + 5.0f;
        return std::round(target - centre);
    }

    void build_sung_pitch() {
        const size_t count = m_frames.size();
        std::vector<float> st(count, 0.0f);   // semitones above A4
        const float transpose = m_style->transpose + (m_style->dry ? natural_transpose() : 0.0f);
        float last = 60.0f + transpose - 69.0f;
        std::vector<float> note_t(count, 0.0f);
        std::vector<bool> voiced(count, false);
        for (size_t j = 0; j < count; ++j) {
            int s = syllable_of_frame(static_cast<int>(j));
            if (s >= 0) {
                NotePos pos = note_at(s, frame_time(static_cast<int>(j)));
                if (!pos.in_rest) last = static_cast<float>(pos.midi) + transpose - 69.0f;
                note_t[j] = pos.t_note;
            }
            st[j] = last;
            voiced[j] = m_frames[j].av > 0.0f;
        }

        // Portamento: the glide between notes, zero-lag.
        const float a = std::exp(-FRAME_MS / std::max(m_style->portamento_ms, 1.0f));
        for (size_t j = 1; j < count; ++j) st[j] = a * st[j - 1] + (1.0f - a) * st[j];
        for (size_t j = count - 1; j-- > 0;) st[j] = a * st[j + 1] + (1.0f - a) * st[j];

        // Vibrato: the inflection slider sets its depth, 0 switches it off.
        const float depth = m_params.inflection_enabled
            ? m_style->vibrato_cents * (std::clamp(m_params.inflection_level, 0.0f, 1.0f) / INFLECTION_LEVEL_DEFAULT)
            : 0.0f;
        const float base = m_params.pitch * m_params.user_pitch;
        float phase = 0.0f;
        for (size_t j = 0; j < count; ++j) {
            float t = frame_time(static_cast<int>(j));
            float onset = m_style->vibrato_delay_ms > 0.0f
                ? std::clamp(note_t[j] / m_style->vibrato_delay_ms, 0.0f, 1.0f) : 1.0f;
            // A singer's vibrato rate wanders by a few percent; a perfectly
            // even one sounds mechanical (Sundberg).
            float rate = m_style->vibrato_hz * (1.0f + 0.03f * std::sin(2.0f * 3.14159265f * 0.41f * t / 1000.0f));
            phase += 2.0f * 3.14159265f * rate * FRAME_MS / 1000.0f;
            float vib = depth * onset * onset * std::sin(phase);
            float hz = 440.0f * base * std::pow(2.0f, (st[j] + vib / 100.0f) / 12.0f);
            m_frames[j].f0 = std::clamp(hz, 45.0f, 1000.0f);
        }
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
        if (m_singing) {
            build_sung_pitch();
            return;
        }
        const size_t count = m_frames.size();
        std::vector<float> st(count, 0.0f);

        const Intonation intonation(m_utt, collect_syllables(), m_total_ms,
                                    m_voice.pitch_range, m_params.inflection_enabled);
        for (size_t j = 0; j < count; ++j) {
            float t = (static_cast<float>(static_cast<int>(j) - m_lead) + 0.5f) * FRAME_MS;
            st[j] = intonation.at(t);
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

        // Inflection level: 0.5 keeps the movements as measured, 0 flattens
        // the contour to a monotone (segmental effects included), 1 doubles
        // every movement.
        const float level = std::clamp(m_params.inflection_level, 0.0f, 1.0f) / INFLECTION_LEVEL_DEFAULT;
        const float base = m_voice.base_f0 * m_params.pitch * m_params.user_pitch;
        for (size_t j = 0; j < count; ++j) {
            float hz = base * std::pow(2.0f, level * st[j] / 12.0f);
            m_frames[j].f0 = std::clamp(hz, 45.0f, 480.0f);
        }
    }

    const FormantVoice& m_voice;
    const VoiceParams& m_params;
    const Utterance& m_utt;
    const std::vector<Phone>& m_ph;
    const SingingStyle* m_style = nullptr;
    const std::vector<Note>* m_melody = nullptr;
    size_t* m_cursor = nullptr;
    bool m_singing = false;
    float m_tempo = 1.0f;
    float m_speed = 1.0f;
    float m_total_ms = 0.0f;
    int m_lead = 0;
    std::vector<SungSyllable> m_sung;

    std::vector<float> m_dur;
    std::vector<Seg> m_segs;
    std::vector<int> m_seg_of_frame;
    std::vector<Frame> m_frames;
};

} // namespace

// =============================================================================
// Public API
// =============================================================================

float midi_hz(float midi) {
    return 440.0f * std::pow(2.0f, (midi - 69.0f) / 12.0f);
}

std::vector<Note> parse_song(const Song& song) {
    std::vector<Note> notes;
    const char* p = song.notation ? song.notation : "";
    while (*p) {
        while (*p == ' ' || *p == '\n' || *p == '\t') ++p;
        if (!*p) break;
        const char* start = p;
        while (*p && *p != ' ' && *p != '\n' && *p != '\t') ++p;
        std::string tok(start, p);
        if (tok == "|") continue;

        Note note;
        size_t i = 0;
        if (tok[i] == '~') {
            note.tie = true;
            ++i;
        }
        if (i >= tok.size()) continue;
        char letter = tok[i++];
        int semis = -1;
        switch (letter) {
            case 'C': semis = 0; break;
            case 'D': semis = 2; break;
            case 'E': semis = 4; break;
            case 'F': semis = 5; break;
            case 'G': semis = 7; break;
            case 'A': semis = 9; break;
            case 'B': semis = 11; break;
            case 'R': case 'r': semis = -2; break;
            default: break;
        }
        if (semis == -1) continue;
        if (semis >= 0) {
            if (i < tok.size() && tok[i] == '#') { ++semis; ++i; }
            else if (i < tok.size() && tok[i] == 'b') { --semis; ++i; }
            if (i >= tok.size() || tok[i] < '0' || tok[i] > '9') continue;
            int octave = tok[i++] - '0';
            note.midi = static_cast<int8_t>(12 * (octave + 1) + semis);
        }
        if (i >= tok.size() || tok[i] != '/') continue;
        ++i;
        int value = 0;
        while (i < tok.size() && tok[i] >= '0' && tok[i] <= '9') {
            value = value * 10 + (tok[i++] - '0');
        }
        if (value <= 0) continue;
        note.beats = 4.0f / static_cast<float>(value);
        float dot = note.beats * 0.5f;
        while (i < tok.size() && tok[i] == '.') {
            note.beats += dot;
            dot *= 0.5f;
            ++i;
        }
        if (i < tok.size() && tok[i] == 't') {
            note.beats *= 2.0f / 3.0f;
            ++i;
        }
        if (i != tok.size()) continue;
        notes.push_back(note);
    }
    return notes;
}

const FormantVoice* find_formant_voice(const char* voice_id) {
    if (!voice_id) {
        return nullptr;
    }
    for (const auto& voice : VOICES) {
        if (std::strcmp(voice.id, voice_id) == 0) {
            return &voice;
        }
    }
    for (const auto& voice : preset_voices()) {
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
    if (voice.singing) {
        const SingingStyle& style = *voice.singing;
        quality.source = style.source;
        quality.chorus = style.chorus;
        quality.chorus_cents = style.chorus_cents;
        quality.sub_octave = style.sub_octave;
        quality.reverb = style.reverb;
        quality.brightness = style.brightness;
        quality.flutter = style.flutter;
        quality.jitter = style.jitter;
        quality.shimmer = style.shimmer;
        if (style.f5 > 0.0f) {
            // The singer's formant: F5 pulled down towards F4 and narrowed
            // (DECtalk's f5/b5 trick), a ring the speaking voice has not.
            quality.upper_f[0] = style.f5 * voice.formant_scale;
            quality.upper_b[0] = style.b5;
        }
        m_melody = parse_song(*style.song);
    }
    m_synth.set_quality(quality);
}

void FormantSynthesizer::rewind_song() {
    m_cursor = 0;
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

    ClauseBuilder builder(m_voice, params, utt, m_voice.singing ? &m_melody : nullptr,
                          m_voice.singing ? &m_cursor : nullptr);
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
