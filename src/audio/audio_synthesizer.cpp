// -*- coding: utf-8 -*-
// audio_synthesizer.cpp - Synthesis with the recorded voices (see the header)

#include "audio_synthesizer.hpp"
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

namespace {

// Brightness for the recorded voices: a tilt around 1.5 kHz. The signal is
// split by a one-pole low-pass into its body and its top, and the top is
// scaled: 50 leaves the recording as it is, 0 folds the top away (the
// setting at which a phone's speaker at full volume stops biting), 100
// doubles it (+6 dB).
void tilt_brightness(std::vector<float>& wave, uint32_t fs, int brightness) {
    const int v = std::clamp(brightness, VOICE_QUALITY_MIN, VOICE_QUALITY_MAX);
    if (v == VOICE_QUALITY_DEFAULT || wave.empty()) return;
    const float top_gain = 1.0f + static_cast<float>(v - 50) / 50.0f;   // 0 .. 2
    const float coef = std::exp(-2.0f * 3.14159265f * 1500.0f / static_cast<float>(fs));
    float low = 0.0f;
    for (float& x : wave) {
        low = coef * low + (1.0f - coef) * x;
        x = low + top_gain * (x - low);
    }
}

} // namespace

// =============================================================================
// Synthesis
// =============================================================================

AudioBuffer AudioSynthesizer::synthesize_clause(const std::u32string& text, Punctuation punct,
                                                bool sentence_initial, bool letter_sounds) {
    AudioBuffer result;
    result.sample_rate = SAMPLE_RATE;
    result.bits_per_sample = BITS_PER_SAMPLE;
    result.channels = NUM_CHANNELS;

    if (text.empty() || !m_frontend) return result;
    ensure_bank();
    if (!m_bank.loaded()) return result;

    formant::Utterance utt = m_frontend->process(text, punct, letter_sounds);
    utt.sentence_initial = sentence_initial;
    return render_utterance(utt);
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
    // peaks under its ceiling (volume_boost.hpp); to_sample's soft knee is
    // for the plain output only.
    const bool boosted = m_voice_params.volume_boost;
    if (boosted) boost_volume(wave, fs);
    const float volume = m_voice_params.volume;
    result.sample_rate = fs;
    result.samples.resize(wave.size());
    for (size_t i = 0; i < wave.size(); ++i) {
        result.samples[i] = to_sample(wave[i] * volume, boosted);
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
