// -*- coding: utf-8 -*-
// laprdus.cpp - Public C++ interface implementation

#include "laprdus/laprdus.hpp"
#include "core/tts_engine.hpp"

namespace laprdus {

// =============================================================================
// Constructor / Destructor
// =============================================================================

Laprdus::Laprdus()
    : m_engine(std::make_unique<TTSEngine>())
{
}

Laprdus::~Laprdus() = default;

Laprdus::Laprdus(Laprdus&& other) noexcept = default;

Laprdus& Laprdus::operator=(Laprdus&& other) noexcept = default;

// =============================================================================
// Initialization
// =============================================================================

bool Laprdus::initialize(const std::string& path,
                        span<const uint8_t> key) {
    if (!m_engine) {
        m_engine = std::make_unique<TTSEngine>();
    }
    return m_engine->initialize(path, key);
}

bool Laprdus::initialize(const uint8_t* data, size_t size,
                        span<const uint8_t> key) {
    if (!m_engine) {
        m_engine = std::make_unique<TTSEngine>();
    }
    return m_engine->initialize_from_memory(data, size, key);
}

bool Laprdus::isReady() const {
    return m_engine && m_engine->is_initialized();
}

// =============================================================================
// Synthesis
// =============================================================================

SynthesisResult Laprdus::speak(const std::string& text) {
    if (!isReady()) {
        SynthesisResult result;
        result.success = false;
        result.error_message = "Engine not initialized";
        return result;
    }
    return m_engine->synthesize(text);
}

SynthesisResult Laprdus::speakStreaming(
    const std::string& text,
    std::function<void(const AudioBuffer&)> callback,
    uint32_t chunkMs) {

    if (!isReady()) {
        SynthesisResult result;
        result.success = false;
        result.error_message = "Engine not initialized";
        return result;
    }
    return m_engine->synthesize_streaming(text, callback, chunkMs);
}

// =============================================================================
// Voice Parameters
// =============================================================================

void Laprdus::setSpeed(float speed) {
    if (m_engine) {
        VoiceParams params = m_engine->voice_params();
        params.speed = speed;
        m_engine->set_voice_params(params);
    }
}

float Laprdus::speed() const {
    if (m_engine) {
        return m_engine->voice_params().speed;
    }
    return 1.0f;
}

void Laprdus::setPitch(float pitch) {
    if (m_engine) {
        VoiceParams params = m_engine->voice_params();
        params.pitch = pitch;
        m_engine->set_voice_params(params);
    }
}

float Laprdus::pitch() const {
    if (m_engine) {
        return m_engine->voice_params().pitch;
    }
    return 1.0f;
}

void Laprdus::setVolume(float volume) {
    if (m_engine) {
        VoiceParams params = m_engine->voice_params();
        params.volume = volume;
        m_engine->set_voice_params(params);
    }
}

float Laprdus::volume() const {
    if (m_engine) {
        return m_engine->voice_params().volume;
    }
    return 1.0f;
}

void Laprdus::setInflectionLevel(float level) {
    if (m_engine) {
        VoiceParams params = m_engine->voice_params();
        params.inflection_level = level;
        m_engine->set_voice_params(params);
    }
}

float Laprdus::inflectionLevel() const {
    if (m_engine) {
        return m_engine->voice_params().inflection_level;
    }
    return INFLECTION_LEVEL_DEFAULT;
}

void Laprdus::setVolumeBoost(bool enabled) {
    if (m_engine) {
        VoiceParams params = m_engine->voice_params();
        params.volume_boost = enabled;
        m_engine->set_voice_params(params);
    }
}

bool Laprdus::volumeBoost() const {
    if (m_engine) {
        return m_engine->voice_params().volume_boost;
    }
    return false;
}

void Laprdus::setAcceleration(float acceleration) {
    if (m_engine) {
        VoiceParams params = m_engine->voice_params();
        params.acceleration = acceleration;
        m_engine->set_voice_params(params);
    }
}

float Laprdus::acceleration() const {
    if (m_engine) {
        return m_engine->voice_params().acceleration;
    }
    return ACCELERATION_DEFAULT;
}

#define LAPRDUS_VOICE_QUALITY_ACCESSORS(Setter, Getter, field)                       \
    void Laprdus::Setter(int value) {                                                \
        if (m_engine) {                                                              \
            VoiceParams params = m_engine->voice_params();                           \
            params.field = value;                                                    \
            m_engine->set_voice_params(params);                                      \
        }                                                                            \
    }                                                                                \
    int Laprdus::Getter() const {                                                    \
        return m_engine ? m_engine->voice_params().field : VOICE_QUALITY_DEFAULT;    \
    }
LAPRDUS_VOICE_QUALITY_ACCESSORS(setHeadSize, headSize, head_size)
LAPRDUS_VOICE_QUALITY_ACCESSORS(setBreathiness, breathiness, breathiness)
LAPRDUS_VOICE_QUALITY_ACCESSORS(setRoughness, roughness, roughness)
LAPRDUS_VOICE_QUALITY_ACCESSORS(setBrightness, brightness, brightness)
#undef LAPRDUS_VOICE_QUALITY_ACCESSORS

void Laprdus::setSpellingMode(SpellingMode mode) {
    if (m_engine) {
        m_engine->set_spelling_mode(mode);
    }
}

SpellingMode Laprdus::spellingMode() const {
    return m_engine ? m_engine->spelling_mode() : SpellingMode::LetterSounds;
}

void Laprdus::setSpellingSpeed(int percent) {
    if (m_engine) {
        m_engine->set_spelling_speed(percent);
    }
}

int Laprdus::spellingSpeed() const {
    return m_engine ? m_engine->spelling_speed() : SPELLING_SPEED_DEFAULT;
}

// =============================================================================
// Utility
// =============================================================================

uint32_t Laprdus::sampleRate() const {
    if (m_engine) {
        return m_engine->sample_rate();
    }
    return SAMPLE_RATE;
}

const char* Laprdus::version() {
    return TTSEngine::version();
}

} // namespace laprdus
