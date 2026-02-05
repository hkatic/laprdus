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

void Laprdus::setInflectionEnabled(bool enabled) {
    if (m_engine) {
        VoiceParams params = m_engine->voice_params();
        params.inflection_enabled = enabled;
        m_engine->set_voice_params(params);
    }
}

bool Laprdus::inflectionEnabled() const {
    if (m_engine) {
        return m_engine->voice_params().inflection_enabled;
    }
    return true;
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
