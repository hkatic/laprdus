// -*- coding: utf-8 -*-
// audio_synthesizer.cpp - Synthesis with the recorded voices (see the header)

#include "audio_synthesizer.hpp"
#include "concat_prosody.hpp"
#include "psola.hpp"
#include <algorithm>
#include <cmath>

namespace laprdus {

namespace {

// Output above this level is bent back softly instead of clipped
constexpr float LIMIT_KNEE = 0.90f;

int16_t to_sample(float v) {
    float a = std::abs(v);
    if (a > LIMIT_KNEE) {
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
    const float warp = formant_warp_for_pitch(m_voice_params.pitch);
    if (m_bank_warp > 0.0f && std::abs(warp - m_bank_warp) < 1e-3f) return;
    if (!m_phoneme_data.is_loaded()) return;
    m_bank.build(m_phoneme_data, warp);
    m_bank_warp = warp;
    // The bank's pitch is that of the warped recordings; the voice's own is
    // the unwarped one, and the character pitch is applied by the planner.
    m_natural_f0 = m_bank.base_f0() / warp;
}

// =============================================================================
// Synthesis
// =============================================================================

AudioBuffer AudioSynthesizer::synthesize_clause(const std::u32string& text, Punctuation punct) {
    AudioBuffer result;
    result.sample_rate = SAMPLE_RATE;
    result.bits_per_sample = BITS_PER_SAMPLE;
    result.channels = NUM_CHANNELS;

    if (text.empty() || !m_frontend) return result;
    ensure_bank();
    if (!m_bank.loaded()) return result;

    const formant::Utterance utt = m_frontend->process(text, punct);
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

    const float volume = m_voice_params.volume;
    result.sample_rate = fs;
    result.samples.resize(wave.size());
    for (size_t i = 0; i < wave.size(); ++i) {
        result.samples[i] = to_sample(wave[i] * volume);
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
