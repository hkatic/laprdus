// -*- coding: utf-8 -*-
// sapi_driver.cpp - Windows SAPI5 TTS driver implementation

#ifdef _WIN32

#include "sapi_driver.hpp"
#include "core/voice_registry.hpp"
#include "core/user_config.hpp"
#include <string>
#include <filesystem>
#include <cstdlib>

// Resource ID for registry script
#define IDR_LAPRDUS 101

// Define the CLSID storage (DEFINE_GUID only declares, this defines the actual data)
// {D4B5A4E1-8B5C-4C5D-9E6F-1A2B3C4D5E6F}
// Same CLSID for both 32-bit and 64-bit - Windows registry virtualization handles routing
extern "C" const GUID CLSID_LaprdusTTS =
    { 0xD4B5A4E1, 0x8B5C, 0x4C5D, { 0x9E, 0x6F, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E, 0x6F } };

namespace laprdus {

// =============================================================================
// Constructor / Destructor
// =============================================================================

LaprdusSAPIDriver::LaprdusSAPIDriver() = default;

LaprdusSAPIDriver::~LaprdusSAPIDriver() = default;

HRESULT LaprdusSAPIDriver::FinalConstruct() {
    return S_OK;
}

void LaprdusSAPIDriver::FinalRelease() {
    m_engine.reset();
    m_token.Release();
}

// =============================================================================
// ISpObjectWithToken Implementation
// =============================================================================

STDMETHODIMP LaprdusSAPIDriver::SetObjectToken(ISpObjectToken* pToken) {
    if (!pToken) {
        return E_INVALIDARG;
    }

    m_token = pToken;
    return InitializeEngine();
}

STDMETHODIMP LaprdusSAPIDriver::GetObjectToken(ISpObjectToken** ppToken) {
    if (!ppToken) {
        return E_POINTER;
    }

    if (!m_token) {
        return E_FAIL;
    }

    *ppToken = m_token;
    (*ppToken)->AddRef();
    return S_OK;
}

// =============================================================================
// ISpTTSEngine Implementation
// =============================================================================

STDMETHODIMP LaprdusSAPIDriver::Speak(
    DWORD dwSpeakFlags,
    REFGUID rguidFormatId,
    const WAVEFORMATEX* pWaveFormatEx,
    const SPVTEXTFRAG* pTextFragList,
    ISpTTSEngineSite* pOutputSite) {

    // Suppress unused parameter warnings
    (void)dwSpeakFlags;
    (void)rguidFormatId;
    (void)pWaveFormatEx;

    // Check if engine is initialized
    if (!m_initialized || !m_engine) {
        return E_FAIL;
    }

    if (!pTextFragList || !pOutputSite) {
        return E_INVALIDARG;
    }

    // Get voice parameters from SAPI5
    // Rate and volume come from ISpTTSEngineSite (global settings)
    // Pitch comes from text fragment state (per-fragment adjustment)
    // Force flags can override SAPI5 values with user settings from settings.json
    laprdus::VoiceParams params = m_engine->voice_params();

    // Get rate - use Laprdus setting if forced, otherwise SAPI5
    if (m_forceSpeed) {
        params.speed = m_userSpeed;
    } else {
        long sapiRate = 0;
        if (SUCCEEDED(pOutputSite->GetRate(&sapiRate)) && sapiRate != 0) {
            // Map -10..+10 to 0.5..2.0 exponentially
            // Only apply if rate is not default (0)
            float rateMultiplier = std::pow(2.0f, sapiRate / 10.0f);
            params.speed = std::clamp(rateMultiplier, 0.5f, 2.0f);
        } else {
            params.speed = 1.0f;  // Default: no speed adjustment
        }
    }

    // Get volume - use Laprdus setting if forced, otherwise SAPI5
    if (m_forceVolume) {
        params.volume = m_userVolume;
    } else {
        USHORT sapiVolume = 100;
        if (SUCCEEDED(pOutputSite->GetVolume(&sapiVolume))) {
            params.volume = static_cast<float>(sapiVolume) / 100.0f;
            params.volume = std::clamp(params.volume, 0.0f, 1.0f);
        }
    }

    // Set voice character pitch (shifts formants - for derived voices)
    params.pitch = m_basePitch;

    // Get user pitch preference - use Laprdus setting if forced, otherwise SAPI5
    if (m_forcePitch) {
        params.user_pitch = m_userPitch;
    } else if (pTextFragList) {
        // Get user pitch preference from text fragment state (XML markup or app-specific)
        // This uses formant-preserving pitch shift - no chipmunk effect
        // SAPI5 PitchAdj.MiddleAdj range is -24 to +24
        // Map to full user_pitch range: -24 → 0.5x, 0 → 1.0x, +24 → 2.0x
        const SPVSTATE& state = pTextFragList->State;
        float pitchOffset = static_cast<float>(state.PitchAdj.MiddleAdj);
        float pitchMultiplier = std::pow(2.0f, pitchOffset / 24.0f);
        params.user_pitch = std::clamp(pitchMultiplier, 0.5f, 2.0f);
    }

    m_engine->set_voice_params(params);

    // Extract text and spell mode from fragments
    bool useSpelledSynthesis = false;
    std::wstring text = ExtractText(pTextFragList, useSpelledSynthesis);
    if (text.empty()) {
        return S_OK;  // Nothing to speak
    }

    // Trim whitespace from text (SAPI5 often adds trailing spaces/newlines)
    while (!text.empty() && (text.back() == L' ' || text.back() == L'\t' ||
                              text.back() == L'\r' || text.back() == L'\n')) {
        text.pop_back();
    }
    while (!text.empty() && (text.front() == L' ' || text.front() == L'\t' ||
                              text.front() == L'\r' || text.front() == L'\n')) {
        text.erase(0, 1);
    }

    if (text.empty()) {
        return S_OK;
    }

    // Convert to UTF-8
    std::string utf8_text = WideToUtf8(text);
    if (utf8_text.empty()) {
        return S_OK;
    }

    // Check if text is a single UTF-8 character
    // Many SAPI5 apps spell by sending individual characters with SPVA_Speak
    // (not SPVA_SpellOut), so we detect single characters and use spelled synthesis
    bool isSingleChar = false;
    {
        size_t charCount = 0;
        size_t pos = 0;
        while (pos < utf8_text.size() && charCount < 2) {
            unsigned char c = utf8_text[pos];
            size_t charLen = 1;
            if ((c & 0x80) == 0) charLen = 1;
            else if ((c & 0xE0) == 0xC0) charLen = 2;
            else if ((c & 0xF0) == 0xE0) charLen = 3;
            else if ((c & 0xF8) == 0xF0) charLen = 4;
            pos += charLen;
            charCount++;
        }
        isSingleChar = (charCount == 1);
    }

    // Synthesize (use spelled synthesis for SPVA_SpellOut or single characters)
    SynthesisResult result;
    if (useSpelledSynthesis || isSingleChar) {
        result = m_engine->synthesize_spelled(utf8_text);
    } else {
        result = m_engine->synthesize(utf8_text);
    }
    if (!result.success) {
        return E_FAIL;
    }

    // Check for cancellation
    if (ShouldStop(pOutputSite)) {
        return S_OK;
    }

    // Send audio to output site
    HRESULT hr = SendAudio(pOutputSite, result.audio);

    return hr;
}

STDMETHODIMP LaprdusSAPIDriver::GetOutputFormat(
    const GUID* pTargetFmtId,
    const WAVEFORMATEX* pTargetWaveFormatEx,
    GUID* pOutputFormatId,
    WAVEFORMATEX** ppCoMemOutputWaveFormatEx) {

    // Suppress unused parameter warnings
    (void)pTargetFmtId;
    (void)pTargetWaveFormatEx;

    if (!pOutputFormatId || !ppCoMemOutputWaveFormatEx) {
        return E_POINTER;
    }

    // We output 16-bit mono PCM at 22050Hz
    *pOutputFormatId = SPDFID_WaveFormatEx;

    // Allocate WAVEFORMATEX using COM allocator
    WAVEFORMATEX* pFormat = static_cast<WAVEFORMATEX*>(
        CoTaskMemAlloc(sizeof(WAVEFORMATEX)));

    if (!pFormat) {
        return E_OUTOFMEMORY;
    }

    pFormat->wFormatTag = WAVE_FORMAT_PCM;
    pFormat->nChannels = NUM_CHANNELS;
    pFormat->nSamplesPerSec = SAMPLE_RATE;
    pFormat->wBitsPerSample = BITS_PER_SAMPLE;
    pFormat->nBlockAlign = NUM_CHANNELS * BITS_PER_SAMPLE / 8;
    pFormat->nAvgBytesPerSec = SAMPLE_RATE * pFormat->nBlockAlign;
    pFormat->cbSize = 0;

    *ppCoMemOutputWaveFormatEx = pFormat;
    return S_OK;
}

// =============================================================================
// Private Methods
// =============================================================================

HRESULT LaprdusSAPIDriver::InitializeEngine() {
    if (m_initialized) {
        return S_OK;
    }

    if (!m_token) {
        return E_FAIL;
    }

    // Read VoiceId from token (for derived voice support)
    CSpDynamicString voiceId;
    m_token->GetStringValue(L"VoiceId", &voiceId);

    // Read BasePitch from token (for derived voices like child, grandma, grandpa)
    // Note: Use locale-independent parsing to handle decimal separator correctly
    // on systems with comma as decimal separator (e.g., Croatian, Serbian locales)
    CSpDynamicString basePitchStr;
    m_basePitch = 1.0f;
    if (SUCCEEDED(m_token->GetStringValue(L"BasePitch", &basePitchStr)) && basePitchStr) {
        // Parse using locale-independent method (period as decimal separator)
        wchar_t* endPtr = nullptr;
        double value = wcstod(basePitchStr.m_psz, &endPtr);
        // If wcstod failed to parse (locale uses comma), try manual parsing
        if (endPtr == basePitchStr.m_psz || value == 0.0) {
            // Manual parsing: look for period and parse integer and fractional parts
            std::wstring str(basePitchStr.m_psz);
            size_t dotPos = str.find(L'.');
            if (dotPos != std::wstring::npos) {
                int intPart = _wtoi(str.substr(0, dotPos).c_str());
                std::wstring fracStr = str.substr(dotPos + 1);
                double fracPart = static_cast<double>(_wtoi(fracStr.c_str()));
                for (size_t i = 0; i < fracStr.length(); ++i) {
                    fracPart /= 10.0;
                }
                value = static_cast<double>(intPart) + fracPart;
            } else {
                value = static_cast<double>(_wtoi(basePitchStr.m_psz));
            }
        }
        m_basePitch = static_cast<float>(value);
        // Clamp to valid range (0.25 - 4.0 for voice character pitch)
        if (m_basePitch < 0.25f) m_basePitch = 0.25f;
        if (m_basePitch > 4.0f) m_basePitch = 4.0f;
    }

    // Get the path to phoneme data from token attributes
    CSpDynamicString phonemeDataPath;
    HRESULT hr = m_token->GetStringValue(L"PhonemeDataPath", &phonemeDataPath);

    std::wstring dataPath;
    if (SUCCEEDED(hr) && phonemeDataPath) {
        dataPath = phonemeDataPath.m_psz;
    } else {
        // Default path: relative to DLL location
        // Try to build path based on voice ID
        wchar_t modulePath[MAX_PATH];
        HMODULE hModule = nullptr;

        // Get handle to this DLL using a static variable's address
        static int dummy = 0;
        GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&dummy),
            &hModule);

        if (hModule && GetModuleFileNameW(hModule, modulePath, MAX_PATH)) {
            std::filesystem::path path(modulePath);
            std::filesystem::path parentDir = path.parent_path();

            // If we have a voice ID, look up its data filename
            if (voiceId && voiceId.m_psz) {
                std::string voiceIdUtf8 = WideToUtf8(voiceId.m_psz);
                const laprdus::VoiceDefinition* voice = laprdus::VoiceRegistry::find_by_id(voiceIdUtf8.c_str());
                if (voice) {
                    const char* dataFilename = laprdus::VoiceRegistry::get_data_filename(voice);
                    if (dataFilename) {
                        // Voice data is in voices/ subdirectory
                        path = parentDir / "voices" / dataFilename;
                        dataPath = path.wstring();
                    }
                }
            }

            // Fallback to default voice in voices subdirectory
            // Note: dataFilename from voice_registry is just the filename (e.g., "Josip.bin")
            // We need to prepend "voices/" subdirectory
            if (dataPath.empty()) {
                path = parentDir / "voices" / "Josip.bin";
                dataPath = path.wstring();
            }
        }
    }

    if (dataPath.empty()) {
        return E_FAIL;
    }

    // Create engine
    m_engine = std::make_unique<TTSEngine>();

    // Convert path to UTF-8 and initialize
    std::string utf8Path = WideToUtf8(dataPath);
    if (!m_engine->initialize(utf8Path)) {
        m_engine.reset();
        return E_FAIL;
    }

    // Apply base pitch for derived voices
    if (m_basePitch != 1.0f) {
        laprdus::VoiceParams params = m_engine->voice_params();
        params.pitch = m_basePitch;
        m_engine->set_voice_params(params);
    }

    // Load dictionaries from the dictionaries subdirectory
    // Voice data is in {app}/voices/, dictionaries are in {app}/dictionaries/
    std::filesystem::path dataFilePath(dataPath);
    std::filesystem::path installDir = dataFilePath.parent_path().parent_path();  // Go up from voices/
    std::filesystem::path dictsDir = installDir / "dictionaries";

    // Load pronunciation dictionary
    std::filesystem::path dictPath = dictsDir / "dictionary.json";
    if (std::filesystem::exists(dictPath)) {
        std::string dictPathUtf8 = WideToUtf8(dictPath.wstring());
        m_engine->load_dictionary(dictPathUtf8);
    }

    // Load spelling dictionary for character-by-character pronunciation
    std::filesystem::path spellingDictPath = dictsDir / "spelling.json";
    if (std::filesystem::exists(spellingDictPath)) {
        std::string spellingDictPathUtf8 = WideToUtf8(spellingDictPath.wstring());
        m_engine->load_spelling_dictionary(spellingDictPathUtf8);
    }

    // Load emoji dictionary (disabled by default)
    std::filesystem::path emojiDictPath = dictsDir / "emoji.json";
    if (std::filesystem::exists(emojiDictPath)) {
        std::string emojiDictPathUtf8 = WideToUtf8(emojiDictPath.wstring());
        m_engine->load_emoji_dictionary(emojiDictPathUtf8);
        m_engine->set_emoji_enabled(false);  // Disabled by default
    }

    // Load user configuration from %APPDATA%\Laprdus
    laprdus::UserConfig userConfig;
    if (userConfig.load_settings()) {
        // Apply user settings to engine
        const laprdus::UserSettings& settings = userConfig.settings();

        // Store user settings for Speak() to use with force flags
        m_userSpeed = settings.speed;
        m_userPitch = settings.user_pitch;
        m_userVolume = settings.volume;
        m_forceSpeed = settings.force_speed;
        m_forcePitch = settings.force_pitch;
        m_forceVolume = settings.force_volume;

        laprdus::VoiceParams params = m_engine->voice_params();
        // Keep the base pitch from voice definition, but apply user settings
        params.speed = settings.speed;
        params.user_pitch = settings.user_pitch;
        params.volume = settings.volume;
        params.inflection_enabled = settings.inflection_enabled;
        params.emoji_enabled = settings.emoji_enabled;
        params.number_mode = settings.number_mode;
        params.pause_settings = settings.get_pause_settings();
        params.clamp();
        m_engine->set_voice_params(params);

        // Load user dictionaries if they exist and are enabled (they extend/override internal ones)
        if (settings.user_dictionaries_enabled) {
            if (userConfig.user_dictionary_exists("user.json")) {
                std::string userDictPath = userConfig.get_user_dictionary_path();
                m_engine->append_dictionary(userDictPath);
            }

            if (userConfig.user_dictionary_exists("spelling.json")) {
                std::string userSpellingPath = userConfig.get_user_spelling_dictionary_path();
                m_engine->append_spelling_dictionary(userSpellingPath);
            }

            if (userConfig.user_dictionary_exists("emoji.json")) {
                std::string userEmojiPath = userConfig.get_user_emoji_dictionary_path();
                m_engine->append_emoji_dictionary(userEmojiPath);
            }
        }
    }

    m_initialized = true;
    return S_OK;
}

std::wstring LaprdusSAPIDriver::ExtractText(const SPVTEXTFRAG* pTextFragList, bool& useSpelledSynthesis) {
    std::wstring result;
    useSpelledSynthesis = false;

    const SPVTEXTFRAG* pFrag = pTextFragList;
    while (pFrag) {
        // Handle different fragment actions
        switch (pFrag->State.eAction) {
            case SPVA_Speak:
                if (pFrag->pTextStart && pFrag->ulTextLen > 0) {
                    result.append(pFrag->pTextStart, pFrag->ulTextLen);
                }
                break;

            case SPVA_SpellOut:
                // Spell mode - use spelled synthesis
                useSpelledSynthesis = true;
                if (pFrag->pTextStart && pFrag->ulTextLen > 0) {
                    result.append(pFrag->pTextStart, pFrag->ulTextLen);
                }
                break;

            case SPVA_Silence:
                // Add pause marker
                // TODO: Handle silence duration from pFrag->State.SilenceMSecs
                result += L" ";
                break;

            case SPVA_Pronounce:
                // Phoneme pronunciation hint - not directly supported
                // Fall back to normal speech
                if (pFrag->pTextStart && pFrag->ulTextLen > 0) {
                    result.append(pFrag->pTextStart, pFrag->ulTextLen);
                }
                break;

            case SPVA_Bookmark:
                // Bookmark markers - not directly supported
                break;

            default:
                // Unknown action, try to speak if there's text
                if (pFrag->pTextStart && pFrag->ulTextLen > 0) {
                    result.append(pFrag->pTextStart, pFrag->ulTextLen);
                }
                break;
        }

        pFrag = pFrag->pNext;
    }

    return result;
}

std::string LaprdusSAPIDriver::WideToUtf8(const std::wstring& wide) {
    if (wide.empty()) {
        return {};
    }

    int size = WideCharToMultiByte(
        CP_UTF8, 0,
        wide.data(), static_cast<int>(wide.size()),
        nullptr, 0,
        nullptr, nullptr);

    if (size <= 0) {
        return {};
    }

    std::string result(size, '\0');
    WideCharToMultiByte(
        CP_UTF8, 0,
        wide.data(), static_cast<int>(wide.size()),
        result.data(), size,
        nullptr, nullptr);

    return result;
}

HRESULT LaprdusSAPIDriver::SendAudio(
    ISpTTSEngineSite* pSite,
    const AudioBuffer& audio) {

    if (audio.empty()) {
        return S_OK;
    }

    // Write audio in chunks to allow for cancellation checks
    constexpr size_t CHUNK_SAMPLES = 4096;

    size_t remaining = audio.samples.size();
    size_t offset = 0;

    while (remaining > 0) {
        // Check for stop
        if (ShouldStop(pSite)) {
            return S_OK;
        }

        size_t chunk_size = std::min(remaining, CHUNK_SAMPLES);

        // Write to output site
        ULONG bytesWritten = 0;
        HRESULT hr = pSite->Write(
            audio.samples.data() + offset,
            static_cast<ULONG>(chunk_size * sizeof(AudioSample)),
            &bytesWritten);

        if (FAILED(hr)) {
            return hr;
        }

        offset += chunk_size;
        remaining -= chunk_size;
    }

    return S_OK;
}

bool LaprdusSAPIDriver::ShouldStop(ISpTTSEngineSite* pSite) {
    if (!pSite) {
        return false;
    }

    // GetActions returns DWORD directly, not through an out parameter
    DWORD dwActions = pSite->GetActions();

    return (dwActions & SPVES_ABORT) != 0;
}

} // namespace laprdus

#endif // _WIN32
