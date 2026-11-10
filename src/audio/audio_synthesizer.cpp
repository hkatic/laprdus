// -*- coding: utf-8 -*-
// audio_synthesizer.cpp - Synthesis with the recorded voices (see the header)

#include "audio_synthesizer.hpp"
#include "brightness_tilt.hpp"
#include "concat_prosody.hpp"
#include "psola.hpp"
#include "volume_boost.hpp"
#include <algorithm>
#include <cmath>

namespace laprdus {

namespace {

// Output above this level is bent back softly instead of clipped
constexpr float LIMIT_KNEE = 0.90f;

// `limited` output (the volume boost's) is already held under full scale
// and is not bent again: a second knee would only add distortion.
int16_t to_sample(float v, bool limited = false) {
    float a = std::abs(v);
    if (!limited && a > LIMIT_KNEE) {
        const float over = (a - LIMIT_KNEE) / (1.0f - LIMIT_KNEE);
        a = LIMIT_KNEE + (1.0f - LIMIT_KNEE) * std::tanh(over);
    }
    const float s = (v < 0.0f ? -a : a) * 32767.0f;
    return static_cast<int16_t>(std::lround(std::clamp(s, -32768.0f, 32767.0f)));
}

} // namespace

// =============================================================================
// Construction
// =============================================================================

AudioSynthesizer::AudioSynthesizer(const PhonemeData& phoneme_data, VoiceLanguage language)
    : m_phoneme_data(phoneme_data), m_language(language),
      m_frontend(std::make_unique<formant::Frontend>(language)) {
    ensure_bank();
}

AudioSynthesizer::~AudioSynthesizer() = default;

void AudioSynthesizer::set_language(VoiceLanguage language) {
    if (m_frontend && language == m_language) return;
    m_language = language;
    m_frontend = std::make_unique<formant::Frontend>(language);
    m_frontend->set_user_lexicon(m_user_lexicon);
}

void AudioSynthesizer::set_user_lexicon(const std::shared_ptr<const formant::UserLexicon>& lexicon) {
    m_user_lexicon = lexicon;
    if (m_frontend) m_frontend->set_user_lexicon(lexicon);
}

// =============================================================================
// Voice parameters
// =============================================================================

void AudioSynthesizer::set_voice_params(const VoiceParams& params) {
    m_voice_params = params;
    m_voice_params.clamp();
    ensure_bank();
}

float AudioSynthesizer::formant_warp_for_pitch(float pitch) {
    // A child's vocal tract is shorter than an adult's by less than its
    // pitch is higher: the formants move by the square root of the pitch
    // factor (1.5 -> 1.22, 0.75 -> 0.87).
    if (!(pitch > 0.0f)) return 1.0f;
    return std::sqrt(pitch);
}

void AudioSynthesizer::ensure_bank() {
    // The head size setting lengthens or shortens the vocal tract on top of
    // the character pitch's warp; the bank is rebuilt only when the warp
    // moves (a settings change, not every clause).
    const float warp = formant_warp_for_pitch(m_voice_params.pitch) *
                       head_size_factor(m_voice_params.head_size);
    if (m_bank_warp > 0.0f && std::abs(warp - m_bank_warp) < 1e-3f) return;
    if (!m_phoneme_data.is_loaded()) return;
    m_bank.build(m_phoneme_data, warp);
    m_bank_warp = warp;
    // The bank's pitch is that of the warped recordings; the voice's own is
    // the unwarped one, and the character pitch is applied by the planner.
    m_natural_f0 = m_bank.base_f0() / warp;
}

// Brightness: tilt_brightness() in brightness_tilt.hpp, shared with the
// formant voices.

// =============================================================================
// Synthesis
// =============================================================================

AudioBuffer AudioSynthesizer::synthesize_clause(const std::u32string& text, Punctuation punct,
                                                bool sentence_initial, bool letter_sounds,
                                                uint32_t minor_pause_ms, uint32_t item_pause_ms) {
    AudioBuffer result;
    result.sample_rate = SAMPLE_RATE;
    result.bits_per_sample = BITS_PER_SAMPLE;
    result.channels = NUM_CHANNELS;

    if (text.empty() || !m_frontend) return result;
    ensure_bank();
    if (!m_bank.loaded()) return result;

    formant::Utterance utt = m_frontend->process(text, punct, letter_sounds);
    utt.sentence_initial = sentence_initial;
    // The short pauses inside the clause (a clock time, a numeric date, a
    // cell's address), as in the formant voices: the parts one after
    // another with a silence between them
    const std::vector<formant::Utterance> parts = formant::split_at_breaks(utt);
    if (parts.size() <= 1) return render_utterance(utt);
    for (size_t k = 0; k < parts.size(); ++k) {
        const AudioBuffer part = render_utterance(parts[k]);
        if (part.samples.empty()) continue;
        result.sample_rate = part.sample_rate;
        result.samples.insert(result.samples.end(), part.samples.begin(), part.samples.end());
        if (k + 1 < parts.size()) result.append_silence(parts[k].item_break ? item_pause_ms : minor_pause_ms);
    }
    return result;
}

AudioBuffer AudioSynthesizer::synthesize_letter_sound(const std::u32string& letter) {
    AudioBuffer result;
    result.sample_rate = SAMPLE_RATE;
    result.bits_per_sample = BITS_PER_SAMPLE;
    result.channels = NUM_CHANNELS;
    if (letter.empty() || !m_frontend) return result;
    ensure_bank();
    if (!m_bank.loaded()) return result;
    return render_utterance(m_frontend->letter_sound(letter));
}

AudioBuffer AudioSynthesizer::render_utterance(const formant::Utterance& utt) {
    AudioBuffer result;
    result.sample_rate = SAMPLE_RATE;
    result.bits_per_sample = BITS_PER_SAMPLE;
    result.channels = NUM_CHANNELS;
    if (utt.phones.empty()) return result;

    const uint32_t fs = m_phoneme_data.sample_rate() > 0 ? m_phoneme_data.sample_rate() : SAMPLE_RATE;
    const concat::ClausePlan plan = concat::plan_clause(utt, m_bank, m_voice_params, m_natural_f0, fs);
    if (plan.segments.empty()) return result;

    std::vector<float> wave = concat::render(plan.segments, plan.contour, fs, plan.total_samples);

    // Cut the tail back to the clause plus whatever the last window left
    size_t end = static_cast<size_t>(std::ceil(plan.total_samples));
    size_t last = wave.size();
    while (last > end && std::abs(wave[last - 1]) < 1e-4f) --last;
    wave.resize(std::max(end, last));

    tilt_brightness(wave, fs, m_voice_params.brightness);

    // The volume boost raises the level before the volume and holds the
    // peaks under its ceiling (volume_boost.hpp). The plain output goes
    // through the same limiter at unity gain after the volume, with its
    // ceiling at the soft knee: the recordings touch full scale, and at full
    // volume Vlado and Đedo drove the knee so hard that above 5 kHz half of
    // Đedo's sound was its distortion (November 2026). The limiter's gain
    // moves only between syllables, so nothing is waveshaped, and an
    // utterance that stays under the knee, between the samples too, is bit
    // for bit what it was (every lower volume included).
    const bool boosted = m_voice_params.volume_boost;
    const float volume = m_voice_params.volume;
    // The hissing consonants of the recordings stand level with the vowels
    // (RECORDED_HISS_DB in types.hpp): the plain output holds them where a
    // commercial recorded voice has them; boost mode has its own, lower hold.
    if (!boosted && RECORDED_HISS_DB < 0.0f) {
        boost_detail::limit_hiss(wave, static_cast<float>(fs), RECORDED_HISS_DB);
    }
    if (boosted) boost_volume(wave, fs);
    for (float& x : wave) x *= volume;
    if (!boosted) boost_volume(wave, fs, 1.0f, LIMIT_KNEE, 0.0f, 0.0f, 0.0f, 0.0f);
    result.sample_rate = fs;
    result.samples.resize(wave.size());
    for (size_t i = 0; i < wave.size(); ++i) {
        result.samples[i] = to_sample(wave[i], boosted);
    }
    return result;
}

AudioBuffer AudioSynthesizer::generate_silence(uint32_t duration_ms) const {
    AudioBuffer silence;
    silence.sample_rate = SAMPLE_RATE;
    silence.bits_per_sample = BITS_PER_SAMPLE;
    silence.channels = NUM_CHANNELS;
    size_t num_samples = (static_cast<size_t>(SAMPLE_RATE) * duration_ms) / 1000;
    silence.samples.resize(num_samples, 0);
    return silence;
}

} // namespace laprdus
